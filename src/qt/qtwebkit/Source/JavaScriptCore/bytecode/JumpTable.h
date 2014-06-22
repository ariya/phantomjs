/*
 * Copyright (C) 2008 Apple Inc. All rights reserved.
 * Copyright (C) 2008 Cameron Zwarich <cwzwarich@uwaterloo.ca>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef JumpTable_h
#define JumpTable_h

#include "MacroAssembler.h"
#include <wtf/HashMap.h>
#include <wtf/Vector.h>
#include <wtf/text/StringImpl.h>

namespace JSC {

    struct OffsetLocation {
        int32_t branchOffset;
#if ENABLE(JIT)
        CodeLocationLabel ctiOffset;
#endif
    };

    struct StringJumpTable {
        typedef HashMap<RefPtr<StringImpl>, OffsetLocation> StringOffsetTable;
        StringOffsetTable offsetTable;
#if ENABLE(JIT)
        CodeLocationLabel ctiDefault; // FIXME: it should not be necessary to store this.
#endif

        inline int32_t offsetForValue(StringImpl* value, int32_t defaultOffset)
        {
            StringOffsetTable::const_iterator end = offsetTable.end();
            StringOffsetTable::const_iterator loc = offsetTable.find(value);
            if (loc == end)
                return defaultOffset;
            return loc->value.branchOffset;
        }

#if ENABLE(JIT)
        inline CodeLocationLabel ctiForValue(StringImpl* value)
        {
            StringOffsetTable::const_iterator end = offsetTable.end();
            StringOffsetTable::const_iterator loc = offsetTable.find(value);
            if (loc == end)
                return ctiDefault;
            return loc->value.ctiOffset;
        }
#endif
    };

    struct SimpleJumpTable {
        // FIXME: The two Vectors can be combind into one Vector<OffsetLocation>
        Vector<int32_t> branchOffsets;
        int32_t min;
#if ENABLE(JIT)
        Vector<CodeLocationLabel> ctiOffsets;
        CodeLocationLabel ctiDefault;
#endif

        int32_t offsetForValue(int32_t value, int32_t defaultOffset);
        void add(int32_t key, int32_t offset)
        {
            if (!branchOffsets[key])
                branchOffsets[key] = offset;
        }

#if ENABLE(JIT)
        inline CodeLocationLabel ctiForValue(int32_t value)
        {
            if (value >= min && static_cast<uint32_t>(value - min) < ctiOffsets.size())
                return ctiOffsets[value - min];
            return ctiDefault;
        }
#endif
    };

} // namespace JSC

#endif // JumpTable_h
