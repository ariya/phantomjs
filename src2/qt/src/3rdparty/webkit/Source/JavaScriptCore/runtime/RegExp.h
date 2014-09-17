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

#include "UString.h"
#include "ExecutableAllocator.h"
#include "RegExpKey.h"
#include <wtf/Forward.h>
#include <wtf/RefCounted.h>

namespace JSC {

    struct RegExpRepresentation;
    class JSGlobalData;

    RegExpFlags regExpFlags(const UString&);

    class RegExp : public RefCounted<RegExp> {
    public:
        static PassRefPtr<RegExp> create(JSGlobalData* globalData, const UString& pattern, RegExpFlags);
        ~RegExp();

        bool global() const { return m_flags & FlagGlobal; }
        bool ignoreCase() const { return m_flags & FlagIgnoreCase; }
        bool multiline() const { return m_flags & FlagMultiline; }

        const UString& pattern() const { return m_patternString; }

        bool isValid() const { return !m_constructionError && m_flags != InvalidFlags; }
        const char* errorMessage() const { return m_constructionError; }

        int match(const UString&, int startOffset, Vector<int, 32>* ovector = 0);
        unsigned numSubpatterns() const { return m_numSubpatterns; }
        
#if ENABLE(REGEXP_TRACING)
        void printTraceData();
#endif

    private:
        RegExp(JSGlobalData* globalData, const UString& pattern, RegExpFlags);

        enum RegExpState {
            ParseError,
            JITCode,
            ByteCode
        } m_state;

        RegExpState compile(JSGlobalData*);

#if ENABLE(YARR_JIT_DEBUG)
        void matchCompareWithInterpreter(const UString&, int startOffset, int* offsetVector, int jitResult);
#endif

        UString m_patternString;
        RegExpFlags m_flags;
        const char* m_constructionError;
        unsigned m_numSubpatterns;
#if ENABLE(REGEXP_TRACING)
        unsigned m_rtMatchCallCount;
        unsigned m_rtMatchFoundCount;
#endif

        OwnPtr<RegExpRepresentation> m_representation;
    };

} // namespace JSC

#endif // RegExp_h
