// -*- mode: c++; c-basic-offset: 4 -*-
/*
 * Copyright (C) 2008 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#ifndef JSTypeInfo_h
#define JSTypeInfo_h

// This file would be called TypeInfo.h, but that conflicts with <typeinfo.h>
// in the STL on systems without case-sensitive file systems. 

#include "JSType.h"

namespace JSC {

    // WebCore uses MasqueradesAsUndefined to make document.all and style.filter undetectable.
    static const unsigned MasqueradesAsUndefined = 1;
    static const unsigned ImplementsHasInstance = 1 << 1;
    static const unsigned OverridesHasInstance = 1 << 2;
    static const unsigned ImplementsDefaultHasInstance = 1 << 3;
    static const unsigned NeedsThisConversion = 1 << 4;
    static const unsigned OverridesGetOwnPropertySlot = 1 << 5;
    static const unsigned OverridesVisitChildren = 1 << 6;
    static const unsigned OverridesGetPropertyNames = 1 << 7;
    static const unsigned IsJSFinalObject = 1 << 8;

    class TypeInfo {
    public:
        TypeInfo(JSType type, unsigned flags = 0)
            : m_type(type)
            , m_flags(flags & 0xFF)
            , m_flags2(flags >> 8)
        {
            ASSERT(flags <= 0x1FF);
            ASSERT(type <= 0xFF);
            ASSERT(type >= CompoundType || !(flags & OverridesVisitChildren));
            // ImplementsDefaultHasInstance means (ImplementsHasInstance & !OverridesHasInstance)
            if ((m_flags & (ImplementsHasInstance | OverridesHasInstance)) == ImplementsHasInstance)
                m_flags |= ImplementsDefaultHasInstance;
        }

        JSType type() const { return (JSType)m_type; }

        bool masqueradesAsUndefined() const { return m_flags & MasqueradesAsUndefined; }
        bool implementsHasInstance() const { return m_flags & ImplementsHasInstance; }
        bool overridesHasInstance() const { return m_flags & OverridesHasInstance; }
        bool needsThisConversion() const { return m_flags & NeedsThisConversion; }
        bool overridesGetOwnPropertySlot() const { return m_flags & OverridesGetOwnPropertySlot; }
        bool overridesVisitChildren() const { return m_flags & OverridesVisitChildren; }
        bool overridesGetPropertyNames() const { return m_flags & OverridesGetPropertyNames; }
        unsigned flags() const { return m_flags; }
        unsigned isFinal() const { return m_flags2 && (IsJSFinalObject >> 8); }

        static ptrdiff_t flagsOffset()
        {
            return OBJECT_OFFSETOF(TypeInfo, m_flags);
        }

        static ptrdiff_t typeOffset()
        {
            return OBJECT_OFFSETOF(TypeInfo, m_type);
        }

    private:
        unsigned char m_type;
        unsigned char m_flags;
        unsigned char m_flags2;
    };

}

#endif // JSTypeInfo_h
