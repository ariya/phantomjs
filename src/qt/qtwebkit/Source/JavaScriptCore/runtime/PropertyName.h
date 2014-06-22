/*
 * Copyright (C) 2012 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef PropertyName_h
#define PropertyName_h

#include "Identifier.h"
#include "PrivateName.h"

namespace JSC {

template <typename CharType>
ALWAYS_INLINE uint32_t toUInt32FromCharacters(const CharType* characters, unsigned length)
{
    // An empty string is not a number.
    if (!length)
        return UINT_MAX;

    // Get the first character, turning it into a digit.
    uint32_t value = characters[0] - '0';
    if (value > 9)
        return UINT_MAX;
    
    // Check for leading zeros. If the first characher is 0, then the
    // length of the string must be one - e.g. "042" is not equal to "42".
    if (!value && length > 1)
        return UINT_MAX;
    
    while (--length) {
        // Multiply value by 10, checking for overflow out of 32 bits.
        if (value > 0xFFFFFFFFU / 10)
            return UINT_MAX;
        value *= 10;
        
        // Get the next character, turning it into a digit.
        uint32_t newValue = *(++characters) - '0';
        if (newValue > 9)
            return UINT_MAX;
        
        // Add in the old value, checking for overflow out of 32 bits.
        newValue += value;
        if (newValue < value)
            return UINT_MAX;
        value = newValue;
    }
    
    return value;
}

ALWAYS_INLINE uint32_t toUInt32FromStringImpl(StringImpl* impl)
{
    if (impl->is8Bit())
        return toUInt32FromCharacters(impl->characters8(), impl->length());
    return toUInt32FromCharacters(impl->characters16(), impl->length());
}

class PropertyName {
public:
    PropertyName(const Identifier& propertyName)
        : m_impl(propertyName.impl())
    {
        ASSERT(!m_impl || m_impl->isIdentifier());
    }

    PropertyName(const PrivateName& propertyName)
        : m_impl(propertyName.uid())
    {
        ASSERT(m_impl && m_impl->isEmptyUnique());
    }

    StringImpl* uid() const
    {
        ASSERT(!m_impl || (m_impl->isIdentifier() == !m_impl->isEmptyUnique()));
        return m_impl;
    }

    StringImpl* publicName() const
    {
        ASSERT(!m_impl || (m_impl->isIdentifier() == !m_impl->isEmptyUnique()));
        return m_impl->isIdentifier() ? m_impl : 0;
    }

    static const uint32_t NotAnIndex = UINT_MAX;

    uint32_t asIndex()
    {
        ASSERT(!m_impl || (m_impl->isIdentifier() == !m_impl->isEmptyUnique()));
        return m_impl ? toUInt32FromStringImpl(m_impl) : NotAnIndex;
    }

private:
    StringImpl* m_impl;
};

inline bool operator==(PropertyName a, const Identifier& b)
{
    return a.uid() == b.impl();
}

inline bool operator==(const Identifier& a, PropertyName b)
{
    return a.impl() == b.uid();
}

inline bool operator==(PropertyName a, PropertyName b)
{
    return a.uid() == b.uid();
}

inline bool operator!=(PropertyName a, const Identifier& b)
{
    return a.uid() != b.impl();
}

inline bool operator!=(const Identifier& a, PropertyName b)
{
    return a.impl() != b.uid();
}

inline bool operator!=(PropertyName a, PropertyName b)
{
    return a.uid() != b.uid();
}

}

#endif
