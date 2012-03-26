/*
 *  Copyright (C) 1999-2001, 2004 Harri Porten (porten@kde.org)
 *  Copyright (c) 2007, 2008 Apple Inc. All rights reserved.
 *  Copyright (C) 2009 Torch Mobile, Inc.
 *  Copyright (C) 2010 Peter Varga (pvarga@inf.u-szeged.hu), University of Szeged
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#include "config.h"
#include "RegExp.h"

#include "Lexer.h"
#include "yarr/Yarr.h"
#include "yarr/YarrJIT.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wtf/Assertions.h>
#include <wtf/OwnArrayPtr.h>

namespace JSC {

RegExpFlags regExpFlags(const UString& string)
{
    RegExpFlags flags = NoFlags;

    for (unsigned i = 0; i < string.length(); ++i) {
        switch (string.characters()[i]) {
        case 'g':
            if (flags & FlagGlobal)
                return InvalidFlags;
            flags = static_cast<RegExpFlags>(flags | FlagGlobal);
            break;

        case 'i':
            if (flags & FlagIgnoreCase)
                return InvalidFlags;
            flags = static_cast<RegExpFlags>(flags | FlagIgnoreCase);
            break;

        case 'm':
            if (flags & FlagMultiline)
                return InvalidFlags;
            flags = static_cast<RegExpFlags>(flags | FlagMultiline);
            break;

        default:
            return InvalidFlags;
        }
    }

    return flags;
}
  
struct RegExpRepresentation {
#if ENABLE(YARR_JIT)
    Yarr::YarrCodeBlock m_regExpJITCode;
#endif
    OwnPtr<Yarr::BytecodePattern> m_regExpBytecode;
};

inline RegExp::RegExp(JSGlobalData* globalData, const UString& patternString, RegExpFlags flags)
    : m_patternString(patternString)
    , m_flags(flags)
    , m_constructionError(0)
    , m_numSubpatterns(0)
#if ENABLE(REGEXP_TRACING)
    , m_rtMatchCallCount(0)
    , m_rtMatchFoundCount(0)
#endif
    , m_representation(adoptPtr(new RegExpRepresentation))
{
    m_state = compile(globalData);
}

RegExp::~RegExp()
{
}

PassRefPtr<RegExp> RegExp::create(JSGlobalData* globalData, const UString& patternString, RegExpFlags flags)
{
    RefPtr<RegExp> res = adoptRef(new RegExp(globalData, patternString, flags));
#if ENABLE(REGEXP_TRACING)
    globalData->addRegExpToTrace(res);
#endif
    return res.release();
}

RegExp::RegExpState RegExp::compile(JSGlobalData* globalData)
{
    Yarr::YarrPattern pattern(m_patternString, ignoreCase(), multiline(), &m_constructionError);
    if (m_constructionError)
        return ParseError;

    m_numSubpatterns = pattern.m_numSubpatterns;

    RegExpState res = ByteCode;

#if ENABLE(YARR_JIT)
    if (!pattern.m_containsBackreferences && globalData->canUseJIT()) {
        Yarr::jitCompile(pattern, globalData, m_representation->m_regExpJITCode);
#if ENABLE(YARR_JIT_DEBUG)
        if (!m_representation->m_regExpJITCode.isFallBack())
            res = JITCode;
        else
            res = ByteCode;
#else
        if (!m_representation->m_regExpJITCode.isFallBack())
            return JITCode;
#endif
    }
#endif

    m_representation->m_regExpBytecode = Yarr::byteCompile(pattern, &globalData->m_regExpAllocator);

    return res;
}

int RegExp::match(const UString& s, int startOffset, Vector<int, 32>* ovector)
{
    if (startOffset < 0)
        startOffset = 0;

#if ENABLE(REGEXP_TRACING)
    m_rtMatchCallCount++;
#endif

    if (static_cast<unsigned>(startOffset) > s.length() || s.isNull())
        return -1;

    if (m_state != ParseError) {
        int offsetVectorSize = (m_numSubpatterns + 1) * 2;
        int* offsetVector;
        Vector<int, 32> nonReturnedOvector;
        if (ovector) {
            ovector->resize(offsetVectorSize);
            offsetVector = ovector->data();
        } else {
            nonReturnedOvector.resize(offsetVectorSize);
            offsetVector = nonReturnedOvector.data();
        }

        ASSERT(offsetVector);
        // Initialize offsetVector with the return value (index 0) and the 
        // first subpattern start indicies (even index values) set to -1.
        // No need to init the subpattern end indicies.
        for (unsigned j = 0, i = 0; i < m_numSubpatterns + 1; j += 2, i++)            
            offsetVector[j] = -1;

        int result;
#if ENABLE(YARR_JIT)
        if (m_state == JITCode) {
            result = Yarr::execute(m_representation->m_regExpJITCode, s.characters(), startOffset, s.length(), offsetVector);
#if ENABLE(YARR_JIT_DEBUG)
            matchCompareWithInterpreter(s, startOffset, offsetVector, result);
#endif
        } else
#endif
            result = Yarr::interpret(m_representation->m_regExpBytecode.get(), s.characters(), startOffset, s.length(), offsetVector);
        ASSERT(result >= -1);

#if ENABLE(REGEXP_TRACING)
        if (result != -1)
            m_rtMatchFoundCount++;
#endif

        return result;
    }

    return -1;
}


#if ENABLE(YARR_JIT_DEBUG)
void RegExp::matchCompareWithInterpreter(const UString& s, int startOffset, int* offsetVector, int jitResult)
{
    int offsetVectorSize = (m_numSubpatterns + 1) * 2;
    Vector<int, 32> interpreterOvector;
    interpreterOvector.resize(offsetVectorSize);
    int* interpreterOffsetVector = interpreterOvector.data();
    int interpreterResult = 0;
    int differences = 0;

    // Initialize interpreterOffsetVector with the return value (index 0) and the 
    // first subpattern start indicies (even index values) set to -1.
    // No need to init the subpattern end indicies.
    for (unsigned j = 0, i = 0; i < m_numSubpatterns + 1; j += 2, i++)
        interpreterOffsetVector[j] = -1;

    interpreterResult = Yarr::interpret(m_representation->m_regExpBytecode.get(), s.characters(), startOffset, s.length(), interpreterOffsetVector);

    if (jitResult != interpreterResult)
        differences++;

    for (unsigned j = 2, i = 0; i < m_numSubpatterns; j +=2, i++)
        if ((offsetVector[j] != interpreterOffsetVector[j])
            || ((offsetVector[j] >= 0) && (offsetVector[j+1] != interpreterOffsetVector[j+1])))
            differences++;

    if (differences) {
        fprintf(stderr, "RegExp Discrepency for /%s/\n    string input ", pattern().utf8().data());
        unsigned segmentLen = s.length() - static_cast<unsigned>(startOffset);

        fprintf(stderr, (segmentLen < 150) ? "\"%s\"\n" : "\"%148s...\"\n", s.utf8().data() + startOffset);

        if (jitResult != interpreterResult) {
            fprintf(stderr, "    JIT result = %d, blah interpreted result = %d\n", jitResult, interpreterResult);
            differences--;
        } else {
            fprintf(stderr, "    Correct result = %d\n", jitResult);
        }

        if (differences) {
            for (unsigned j = 2, i = 0; i < m_numSubpatterns; j +=2, i++) {
                if (offsetVector[j] != interpreterOffsetVector[j])
                    fprintf(stderr, "    JIT offset[%d] = %d, interpreted offset[%d] = %d\n", j, offsetVector[j], j, interpreterOffsetVector[j]);
                if ((offsetVector[j] >= 0) && (offsetVector[j+1] != interpreterOffsetVector[j+1]))
                    fprintf(stderr, "    JIT offset[%d] = %d, interpreted offset[%d] = %d\n", j+1, offsetVector[j+1], j+1, interpreterOffsetVector[j+1]);
            }
        }
    }
}
#endif

#if ENABLE(REGEXP_TRACING)
    void RegExp::printTraceData()
    {
        char formattedPattern[41];
        char rawPattern[41];

        strncpy(rawPattern, pattern().utf8().data(), 40);
        rawPattern[40]= '\0';

        int pattLen = strlen(rawPattern);

        snprintf(formattedPattern, 41, (pattLen <= 38) ? "/%.38s/" : "/%.36s...", rawPattern);

#if ENABLE(YARR_JIT)
        Yarr::YarrCodeBlock& codeBlock = m_representation->m_regExpJITCode;

        const size_t jitAddrSize = 20;
        char jitAddr[jitAddrSize];
        if (m_state == JITCode)
            snprintf(jitAddr, jitAddrSize, "fallback");
        else
            snprintf(jitAddr, jitAddrSize, "0x%014lx", reinterpret_cast<unsigned long int>(codeBlock.getAddr()));
#else
        const char* jitAddr = "JIT Off";
#endif

        printf("%-40.40s %16.16s %10d %10d\n", formattedPattern, jitAddr, m_rtMatchCallCount, m_rtMatchFoundCount);
    }
#endif
    
} // namespace JSC
