/*
 * Copyright (C) 2012, 2013 Apple Inc. All rights reserved.
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

#include "config.h"
#include "JSStringJoiner.h"

#include "ExceptionHelpers.h"
#include "JSScope.h"
#include "JSString.h"
#include "Operations.h"
#include <wtf/text/StringImpl.h>

namespace JSC {

// The destination is 16bits, at least one string is 16 bits.
static inline void appendStringToData(UChar*& data, const String& string)
{
    if (string.isNull())
        return;

    unsigned length = string.length();
    const StringImpl* stringImpl = string.impl();

    if (stringImpl->is8Bit()) {
        for (unsigned i = 0; i < length; ++i) {
            *data = stringImpl->characters8()[i];
            ++data;
        }
    } else {
        for (unsigned i = 0; i < length; ++i) {
            *data = stringImpl->characters16()[i];
            ++data;
        }
    }
}

// If the destination is 8bits, we know every string has to be 8bit.
static inline void appendStringToData(LChar*& data, const String& string)
{
    if (string.isNull())
        return;
    ASSERT(string.is8Bit());

    unsigned length = string.length();
    const StringImpl* stringImpl = string.impl();

    for (unsigned i = 0; i < length; ++i) {
        *data = stringImpl->characters8()[i];
        ++data;
    }
}

template<typename CharacterType>
static inline PassRefPtr<StringImpl> joinStrings(const Vector<String>& strings, const String& separator, unsigned outputLength)
{
    ASSERT(outputLength);

    CharacterType* data;
    RefPtr<StringImpl> outputStringImpl = StringImpl::tryCreateUninitialized(outputLength, data);
    if (!outputStringImpl)
        return PassRefPtr<StringImpl>();

    const String firstString = strings.first();
    appendStringToData(data, firstString);

    for (size_t i = 1; i < strings.size(); ++i) {
        appendStringToData(data, separator);
        appendStringToData(data, strings[i]);
    }

    ASSERT(data == (outputStringImpl->getCharacters<CharacterType>() + outputStringImpl->length()));
    return outputStringImpl.release();
}

JSValue JSStringJoiner::join(ExecState* exec)
{
    if (!m_isValid)
        return throwOutOfMemoryError(exec);

    if (!m_strings.size())
        return jsEmptyString(exec);

    Checked<size_t, RecordOverflow> separatorLength = m_separator.length();
    // FIXME: add special cases of joinStrings() for (separatorLength == 0) and (separatorLength == 1).
    ASSERT(m_strings.size() > 0);
    Checked<size_t, RecordOverflow> totalSeparactorsLength = separatorLength * (m_strings.size() - 1);
    Checked<size_t, RecordOverflow> outputStringSize = totalSeparactorsLength + m_accumulatedStringsLength;

    size_t finalSize;
    if (outputStringSize.safeGet(finalSize) == CheckedState::DidOverflow)
        return throwOutOfMemoryError(exec);
        
    if (!outputStringSize)
        return jsEmptyString(exec);

    RefPtr<StringImpl> outputStringImpl;
    if (m_is8Bits)
        outputStringImpl = joinStrings<LChar>(m_strings, m_separator, finalSize);
    else
        outputStringImpl = joinStrings<UChar>(m_strings, m_separator, finalSize);

    if (!outputStringImpl)
        return throwOutOfMemoryError(exec);

    return JSString::create(exec->vm(), outputStringImpl.release());
}

}
