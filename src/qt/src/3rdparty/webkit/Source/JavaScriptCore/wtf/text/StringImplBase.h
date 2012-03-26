/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#ifndef StringImplBase_h
#define StringImplBase_h

#include <wtf/unicode/Unicode.h>

namespace WTF {

class StringImplBase {
    WTF_MAKE_NONCOPYABLE(StringImplBase); WTF_MAKE_FAST_ALLOCATED;
public:
    bool isStringImpl() { return (m_refCountAndFlags & s_refCountInvalidForStringImpl) != s_refCountInvalidForStringImpl; }
    unsigned length() const { return m_length; }
    void ref() { m_refCountAndFlags += s_refCountIncrement; }

protected:
    enum BufferOwnership {
        BufferInternal,
        BufferOwned,
        BufferSubstring,
        BufferShared,
    };

    // For SmallStringStorage, which allocates an array and uses an in-place new.
    StringImplBase() { }

    StringImplBase(unsigned length, BufferOwnership ownership)
        : m_refCountAndFlags(s_refCountIncrement | s_refCountFlagShouldReportedCost | ownership)
        , m_length(length)
    {
        ASSERT(isStringImpl());
    }

    enum StaticStringConstructType { ConstructStaticString };
    StringImplBase(unsigned length, StaticStringConstructType)
        : m_refCountAndFlags(s_refCountFlagStatic | s_refCountFlagIsIdentifier | BufferOwned)
        , m_length(length)
    {
        ASSERT(isStringImpl());
    }

    // This constructor is not used when creating StringImpl objects,
    // and sets the flags into a state marking the object as such.
    enum NonStringImplConstructType { ConstructNonStringImpl };
    StringImplBase(NonStringImplConstructType)
        : m_refCountAndFlags(s_refCountIncrement | s_refCountInvalidForStringImpl)
        , m_length(0)
    {
        ASSERT(!isStringImpl());
    }

    // The bottom 7 bits hold flags, the top 25 bits hold the ref count.
    // When dereferencing StringImpls we check for the ref count AND the
    // static bit both being zero - static strings are never deleted.
    static const unsigned s_refCountMask = 0xFFFFFF80;
    static const unsigned s_refCountIncrement = 0x80;
    static const unsigned s_refCountFlagStatic = 0x40;
    static const unsigned s_refCountFlagHasTerminatingNullCharacter = 0x20;
    static const unsigned s_refCountFlagIsAtomic = 0x10;
    static const unsigned s_refCountFlagShouldReportedCost = 0x8;
    static const unsigned s_refCountFlagIsIdentifier = 0x4;
    static const unsigned s_refCountMaskBufferOwnership = 0x3;
    // An invalid permutation of flags (static & shouldReportedCost - static strings do not
    // set shouldReportedCost in the constructor, and this bit is only ever cleared, not set).
    // Used by "ConstructNonStringImpl" constructor, above.
    static const unsigned s_refCountInvalidForStringImpl = s_refCountFlagStatic | s_refCountFlagShouldReportedCost;

    unsigned m_refCountAndFlags;
    unsigned m_length;
};

} // namespace WTF

using WTF::StringImplBase;

#endif
