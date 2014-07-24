/*
 *  Copyright (C) 1999-2000 Harri Porten (porten@kde.org)
 *  Copyright (C) 2007, 2008, 2009 Apple Inc. All rights reserved.
 *  Copyright (C) 2009 Torch Mobile, Inc.
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

#ifndef RegExp_h
#define RegExp_h

#include "ExecutableAllocator.h"
#include "MatchResult.h"
#include "RegExpKey.h"
#include "Structure.h"
#include "yarr/Yarr.h"
#include <wtf/Forward.h>
#include <wtf/RefCounted.h>
#include <wtf/text/WTFString.h>

#if ENABLE(YARR_JIT)
#include "yarr/YarrJIT.h"
#endif

namespace JSC {

    struct RegExpRepresentation;
    class VM;

    JS_EXPORT_PRIVATE RegExpFlags regExpFlags(const String&);

    class RegExp : public JSCell {
    public:
        typedef JSCell Base;

        JS_EXPORT_PRIVATE static RegExp* create(VM&, const String& pattern, RegExpFlags);
        static const bool needsDestruction = true;
        static const bool hasImmortalStructure = true;
        static void destroy(JSCell*);

        bool global() const { return m_flags & FlagGlobal; }
        bool ignoreCase() const { return m_flags & FlagIgnoreCase; }
        bool multiline() const { return m_flags & FlagMultiline; }

        const String& pattern() const { return m_patternString; }

        bool isValid() const { return !m_constructionError && m_flags != InvalidFlags; }
        const char* errorMessage() const { return m_constructionError; }

        JS_EXPORT_PRIVATE int match(VM&, const String&, unsigned startOffset, Vector<int, 32>& ovector);
        MatchResult match(VM&, const String&, unsigned startOffset);
        unsigned numSubpatterns() const { return m_numSubpatterns; }

        bool hasCode()
        {
            return m_state != NotCompiled;
        }

        void invalidateCode();
        
#if ENABLE(REGEXP_TRACING)
        void printTraceData();
#endif

        static Structure* createStructure(VM& vm, JSGlobalObject* globalObject, JSValue prototype)
        {
            return Structure::create(vm, globalObject, prototype, TypeInfo(LeafType, 0), &s_info);
        }
        
        static const ClassInfo s_info;

        RegExpKey key() { return RegExpKey(m_flags, m_patternString); }

    protected:
        void finishCreation(VM&);

    private:
        friend class RegExpCache;
        RegExp(VM&, const String&, RegExpFlags);

        static RegExp* createWithoutCaching(VM&, const String&, RegExpFlags);

        enum RegExpState {
            ParseError,
            JITCode,
            ByteCode,
            NotCompiled
        } m_state;

        void compile(VM*, Yarr::YarrCharSize);
        void compileIfNecessary(VM&, Yarr::YarrCharSize);

        void compileMatchOnly(VM*, Yarr::YarrCharSize);
        void compileIfNecessaryMatchOnly(VM&, Yarr::YarrCharSize);

#if ENABLE(YARR_JIT_DEBUG)
        void matchCompareWithInterpreter(const String&, int startOffset, int* offsetVector, int jitResult);
#endif

        String m_patternString;
        RegExpFlags m_flags;
        const char* m_constructionError;
        unsigned m_numSubpatterns;
#if ENABLE(REGEXP_TRACING)
        unsigned m_rtMatchCallCount;
        unsigned m_rtMatchFoundCount;
#endif

#if ENABLE(YARR_JIT)
        Yarr::YarrCodeBlock m_regExpJITCode;
#endif
        OwnPtr<Yarr::BytecodePattern> m_regExpBytecode;
    };

} // namespace JSC

#endif // RegExp_h
