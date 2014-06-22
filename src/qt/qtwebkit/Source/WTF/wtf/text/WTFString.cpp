/*
 * (C) 1999 Lars Knoll (knoll@kde.org)
 * Copyright (C) 2004, 2005, 2006, 2007, 2008, 2010, 2012 Apple Inc. All rights reserved.
 * Copyright (C) 2007-2009 Torch Mobile, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include "config.h"
#include "WTFString.h"

#include "IntegerToStringConversion.h"
#include <stdarg.h>
#include <wtf/ASCIICType.h>
#include <wtf/DataLog.h>
#include <wtf/HexNumber.h>
#include <wtf/MathExtras.h>
#include <wtf/NeverDestroyed.h>
#include <wtf/text/CString.h>
#include <wtf/StringExtras.h>
#include <wtf/Vector.h>
#include <wtf/dtoa.h>
#include <wtf/unicode/CharacterNames.h>
#include <wtf/unicode/UTF8.h>
#include <wtf/unicode/Unicode.h>

using namespace std;

namespace WTF {

using namespace Unicode;
using namespace std;

// Construct a string with UTF-16 data.
String::String(const UChar* characters, unsigned length)
    : m_impl(characters ? StringImpl::create(characters, length) : 0)
{
}

// Construct a string with UTF-16 data, from a null-terminated source.
String::String(const UChar* str)
{
    if (!str)
        return;

    m_impl = StringImpl::create(str, lengthOfNullTerminatedString(str));
}

// Construct a string with latin1 data.
String::String(const LChar* characters, unsigned length)
    : m_impl(characters ? StringImpl::create(characters, length) : 0)
{
}

String::String(const char* characters, unsigned length)
    : m_impl(characters ? StringImpl::create(reinterpret_cast<const LChar*>(characters), length) : 0)
{
}

// Construct a string with latin1 data, from a null-terminated source.
String::String(const LChar* characters)
    : m_impl(characters ? StringImpl::create(characters) : 0)
{
}

String::String(const char* characters)
    : m_impl(characters ? StringImpl::create(reinterpret_cast<const LChar*>(characters)) : 0)
{
}

String::String(ASCIILiteral characters)
    : m_impl(StringImpl::createFromLiteral(characters))
{
}

void String::append(const String& str)
{
    if (str.isEmpty())
       return;

    // FIXME: This is extremely inefficient. So much so that we might want to take this
    // out of String's API. We can make it better by optimizing the case where exactly
    // one String is pointing at this StringImpl, but even then it's going to require a
    // call to fastMalloc every single time.
    if (str.m_impl) {
        if (m_impl) {
            if (m_impl->is8Bit() && str.m_impl->is8Bit()) {
                LChar* data;
                if (str.length() > numeric_limits<unsigned>::max() - m_impl->length())
                    CRASH();
                RefPtr<StringImpl> newImpl = StringImpl::createUninitialized(m_impl->length() + str.length(), data);
                memcpy(data, m_impl->characters8(), m_impl->length() * sizeof(LChar));
                memcpy(data + m_impl->length(), str.characters8(), str.length() * sizeof(LChar));
                m_impl = newImpl.release();
                return;
            }
            UChar* data;
            if (str.length() > numeric_limits<unsigned>::max() - m_impl->length())
                CRASH();
            RefPtr<StringImpl> newImpl = StringImpl::createUninitialized(m_impl->length() + str.length(), data);
            memcpy(data, m_impl->characters(), m_impl->length() * sizeof(UChar));
            memcpy(data + m_impl->length(), str.characters(), str.length() * sizeof(UChar));
            m_impl = newImpl.release();
        } else
            m_impl = str.m_impl;
    }
}

template <typename CharacterType>
inline void String::appendInternal(CharacterType c)
{
    // FIXME: This is extremely inefficient. So much so that we might want to take this
    // out of String's API. We can make it better by optimizing the case where exactly
    // one String is pointing at this StringImpl, but even then it's going to require a
    // call to fastMalloc every single time.
    if (m_impl) {
        UChar* data;
        if (m_impl->length() >= numeric_limits<unsigned>::max())
            CRASH();
        RefPtr<StringImpl> newImpl = StringImpl::createUninitialized(m_impl->length() + 1, data);
        memcpy(data, m_impl->characters(), m_impl->length() * sizeof(UChar));
        data[m_impl->length()] = c;
        m_impl = newImpl.release();
    } else
        m_impl = StringImpl::create(&c, 1);
}

void String::append(LChar c)
{
    appendInternal(c);
}

void String::append(UChar c)
{
    appendInternal(c);
}

int codePointCompare(const String& a, const String& b)
{
    return codePointCompare(a.impl(), b.impl());
}

void String::insert(const String& str, unsigned pos)
{
    if (str.isEmpty()) {
        if (str.isNull())
            return;
        if (isNull())
            m_impl = str.impl();
        return;
    }
    insert(str.characters(), str.length(), pos);
}

void String::append(const LChar* charactersToAppend, unsigned lengthToAppend)
{
    if (!m_impl) {
        if (!charactersToAppend)
            return;
        m_impl = StringImpl::create(charactersToAppend, lengthToAppend);
        return;
    }

    if (!lengthToAppend)
        return;

    ASSERT(charactersToAppend);

    unsigned strLength = m_impl->length();

    if (m_impl->is8Bit()) {
        if (lengthToAppend > numeric_limits<unsigned>::max() - strLength)
            CRASH();
        LChar* data;
        RefPtr<StringImpl> newImpl = StringImpl::createUninitialized(strLength + lengthToAppend, data);
        StringImpl::copyChars(data, m_impl->characters8(), strLength);
        StringImpl::copyChars(data + strLength, charactersToAppend, lengthToAppend);
        m_impl = newImpl.release();
        return;
    }

    if (lengthToAppend > numeric_limits<unsigned>::max() - strLength)
        CRASH();
    UChar* data;
    RefPtr<StringImpl> newImpl = StringImpl::createUninitialized(length() + lengthToAppend, data);
    StringImpl::copyChars(data, m_impl->characters16(), strLength);
    StringImpl::copyChars(data + strLength, charactersToAppend, lengthToAppend);
    m_impl = newImpl.release();
}

void String::append(const UChar* charactersToAppend, unsigned lengthToAppend)
{
    if (!m_impl) {
        if (!charactersToAppend)
            return;
        m_impl = StringImpl::create(charactersToAppend, lengthToAppend);
        return;
    }

    if (!lengthToAppend)
        return;

    unsigned strLength = m_impl->length();
    
    ASSERT(charactersToAppend);
    if (lengthToAppend > numeric_limits<unsigned>::max() - strLength)
        CRASH();
    UChar* data;
    RefPtr<StringImpl> newImpl = StringImpl::createUninitialized(strLength + lengthToAppend, data);
    if (m_impl->is8Bit())
        StringImpl::copyChars(data, characters8(), strLength);
    else
        StringImpl::copyChars(data, characters16(), strLength);
    StringImpl::copyChars(data + strLength, charactersToAppend, lengthToAppend);
    m_impl = newImpl.release();
}


void String::insert(const UChar* charactersToInsert, unsigned lengthToInsert, unsigned position)
{
    if (position >= length()) {
        append(charactersToInsert, lengthToInsert);
        return;
    }

    ASSERT(m_impl);

    if (!lengthToInsert)
        return;

    ASSERT(charactersToInsert);
    UChar* data;
    if (lengthToInsert > numeric_limits<unsigned>::max() - length())
        CRASH();
    RefPtr<StringImpl> newImpl = StringImpl::createUninitialized(length() + lengthToInsert, data);
    memcpy(data, characters(), position * sizeof(UChar));
    memcpy(data + position, charactersToInsert, lengthToInsert * sizeof(UChar));
    memcpy(data + position + lengthToInsert, characters() + position, (length() - position) * sizeof(UChar));
    m_impl = newImpl.release();
}

UChar32 String::characterStartingAt(unsigned i) const
{
    if (!m_impl || i >= m_impl->length())
        return 0;
    return m_impl->characterStartingAt(i);
}

void String::truncate(unsigned position)
{
    if (position >= length())
        return;
    UChar* data;
    RefPtr<StringImpl> newImpl = StringImpl::createUninitialized(position, data);
    memcpy(data, characters(), position * sizeof(UChar));
    m_impl = newImpl.release();
}

template <typename CharacterType>
inline void String::removeInternal(const CharacterType* characters, unsigned position, int lengthToRemove)
{
    CharacterType* data;
    RefPtr<StringImpl> newImpl = StringImpl::createUninitialized(length() - lengthToRemove, data);
    memcpy(data, characters, position * sizeof(CharacterType));
    memcpy(data + position, characters + position + lengthToRemove,
        (length() - lengthToRemove - position) * sizeof(CharacterType));

    m_impl = newImpl.release();
}

void String::remove(unsigned position, int lengthToRemove)
{
    if (lengthToRemove <= 0)
        return;
    if (position >= length())
        return;
    if (static_cast<unsigned>(lengthToRemove) > length() - position)
        lengthToRemove = length() - position;

    if (is8Bit()) {
        removeInternal(characters8(), position, lengthToRemove);

        return;
    }

    removeInternal(characters16(), position, lengthToRemove);
}

String String::substring(unsigned pos, unsigned len) const
{
    if (!m_impl) 
        return String();
    return m_impl->substring(pos, len);
}

String String::substringSharingImpl(unsigned offset, unsigned length) const
{
    // FIXME: We used to check against a limit of Heap::minExtraCost / sizeof(UChar).

    unsigned stringLength = this->length();
    offset = min(offset, stringLength);
    length = min(length, stringLength - offset);

    if (!offset && length == stringLength)
        return *this;
    return String(StringImpl::create(m_impl, offset, length));
}

String String::lower() const
{
    if (!m_impl)
        return String();
    return m_impl->lower();
}

String String::upper() const
{
    if (!m_impl)
        return String();
    return m_impl->upper();
}

String String::stripWhiteSpace() const
{
    if (!m_impl)
        return String();
    return m_impl->stripWhiteSpace();
}

String String::stripWhiteSpace(IsWhiteSpaceFunctionPtr isWhiteSpace) const
{
    if (!m_impl)
        return String();
    return m_impl->stripWhiteSpace(isWhiteSpace);
}

String String::simplifyWhiteSpace() const
{
    if (!m_impl)
        return String();
    return m_impl->simplifyWhiteSpace();
}

String String::simplifyWhiteSpace(IsWhiteSpaceFunctionPtr isWhiteSpace) const
{
    if (!m_impl)
        return String();
    return m_impl->simplifyWhiteSpace(isWhiteSpace);
}

String String::removeCharacters(CharacterMatchFunctionPtr findMatch) const
{
    if (!m_impl)
        return String();
    return m_impl->removeCharacters(findMatch);
}

String String::foldCase() const
{
    if (!m_impl)
        return String();
    return m_impl->foldCase();
}

bool String::percentage(int& result) const
{
    if (!m_impl || !m_impl->length())
        return false;

    if ((*m_impl)[m_impl->length() - 1] != '%')
       return false;

    result = charactersToIntStrict(m_impl->characters(), m_impl->length() - 1);
    return true;
}

Vector<UChar> String::charactersWithNullTermination() const
{
    Vector<UChar> result;

    if (m_impl) {
        result.reserveInitialCapacity(length() + 1);

        if (is8Bit()) {
            const LChar* characters8 = m_impl->characters8();
            for (size_t i = 0; i < length(); ++i)
                result.uncheckedAppend(characters8[i]);
        } else {
            const UChar* characters16 = m_impl->characters16();
            result.append(characters16, m_impl->length());
        }

        result.append(0);
    }

    return result;
}

const UChar* String::deprecatedCharactersWithNullTermination()
{
    if (!m_impl)
        return 0;
    if (m_impl->hasTerminatingNullCharacter())
        return m_impl->characters();
    m_impl = StringImpl::createWithTerminatingNullCharacter(*m_impl);
    return m_impl->characters();
}

String String::format(const char *format, ...)
{
#if PLATFORM(QT)
    // Use QString::vsprintf to avoid the locale dependent formatting of vsnprintf.
    // https://bugs.webkit.org/show_bug.cgi?id=18994
    va_list args;
    va_start(args, format);

    QString buffer;
    buffer.vsprintf(format, args);

    va_end(args);

    QByteArray ba = buffer.toUtf8();
    return StringImpl::create(reinterpret_cast<const LChar*>(ba.constData()), ba.length());

#elif OS(WINCE)
    va_list args;
    va_start(args, format);

    Vector<char, 256> buffer;

    int bufferSize = 256;
    buffer.resize(bufferSize);
    for (;;) {
        int written = vsnprintf(buffer.data(), bufferSize, format, args);
        va_end(args);

        if (written == 0)
            return String("");
        if (written > 0)
            return StringImpl::create(reinterpret_cast<const LChar*>(buffer.data()), written);
        
        bufferSize <<= 1;
        buffer.resize(bufferSize);
        va_start(args, format);
    }

#else
    va_list args;
    va_start(args, format);

    Vector<char, 256> buffer;

    // Do the format once to get the length.
#if COMPILER(MSVC)
    int result = _vscprintf(format, args);
#else
    char ch;
    int result = vsnprintf(&ch, 1, format, args);
    // We need to call va_end() and then va_start() again here, as the
    // contents of args is undefined after the call to vsnprintf
    // according to http://man.cx/snprintf(3)
    //
    // Not calling va_end/va_start here happens to work on lots of
    // systems, but fails e.g. on 64bit Linux.
    va_end(args);
    va_start(args, format);
#endif

    if (result == 0)
        return String("");
    if (result < 0)
        return String();
    unsigned len = result;
    buffer.grow(len + 1);
    
    // Now do the formatting again, guaranteed to fit.
    vsnprintf(buffer.data(), buffer.size(), format, args);

    va_end(args);
    
    return StringImpl::create(reinterpret_cast<const LChar*>(buffer.data()), len);
#endif
}

String String::number(int number)
{
    return numberToStringSigned<String>(number);
}

String String::number(unsigned int number)
{
    return numberToStringUnsigned<String>(number);
}

String String::number(long number)
{
    return numberToStringSigned<String>(number);
}

String String::number(unsigned long number)
{
    return numberToStringUnsigned<String>(number);
}

String String::number(long long number)
{
    return numberToStringSigned<String>(number);
}

String String::number(unsigned long long number)
{
    return numberToStringUnsigned<String>(number);
}

String String::number(double number, unsigned precision, TrailingZerosTruncatingPolicy trailingZerosTruncatingPolicy)
{
    NumberToStringBuffer buffer;
    return String(numberToFixedPrecisionString(number, precision, buffer, trailingZerosTruncatingPolicy == TruncateTrailingZeros));
}

String String::numberToStringECMAScript(double number)
{
    NumberToStringBuffer buffer;
    return String(numberToString(number, buffer));
}

String String::numberToStringFixedWidth(double number, unsigned decimalPlaces)
{
    NumberToStringBuffer buffer;
    return String(numberToFixedWidthString(number, decimalPlaces, buffer));
}

int String::toIntStrict(bool* ok, int base) const
{
    if (!m_impl) {
        if (ok)
            *ok = false;
        return 0;
    }
    return m_impl->toIntStrict(ok, base);
}

unsigned String::toUIntStrict(bool* ok, int base) const
{
    if (!m_impl) {
        if (ok)
            *ok = false;
        return 0;
    }
    return m_impl->toUIntStrict(ok, base);
}

int64_t String::toInt64Strict(bool* ok, int base) const
{
    if (!m_impl) {
        if (ok)
            *ok = false;
        return 0;
    }
    return m_impl->toInt64Strict(ok, base);
}

uint64_t String::toUInt64Strict(bool* ok, int base) const
{
    if (!m_impl) {
        if (ok)
            *ok = false;
        return 0;
    }
    return m_impl->toUInt64Strict(ok, base);
}

intptr_t String::toIntPtrStrict(bool* ok, int base) const
{
    if (!m_impl) {
        if (ok)
            *ok = false;
        return 0;
    }
    return m_impl->toIntPtrStrict(ok, base);
}

int String::toInt(bool* ok) const
{
    if (!m_impl) {
        if (ok)
            *ok = false;
        return 0;
    }
    return m_impl->toInt(ok);
}

unsigned String::toUInt(bool* ok) const
{
    if (!m_impl) {
        if (ok)
            *ok = false;
        return 0;
    }
    return m_impl->toUInt(ok);
}

int64_t String::toInt64(bool* ok) const
{
    if (!m_impl) {
        if (ok)
            *ok = false;
        return 0;
    }
    return m_impl->toInt64(ok);
}

uint64_t String::toUInt64(bool* ok) const
{
    if (!m_impl) {
        if (ok)
            *ok = false;
        return 0;
    }
    return m_impl->toUInt64(ok);
}

intptr_t String::toIntPtr(bool* ok) const
{
    if (!m_impl) {
        if (ok)
            *ok = false;
        return 0;
    }
    return m_impl->toIntPtr(ok);
}

double String::toDouble(bool* ok) const
{
    if (!m_impl) {
        if (ok)
            *ok = false;
        return 0.0;
    }
    return m_impl->toDouble(ok);
}

float String::toFloat(bool* ok) const
{
    if (!m_impl) {
        if (ok)
            *ok = false;
        return 0.0f;
    }
    return m_impl->toFloat(ok);
}

#if COMPILER_SUPPORTS(CXX_REFERENCE_QUALIFIED_FUNCTIONS)
String String::isolatedCopy() const &
{
    if (!m_impl)
        return String();
    return m_impl->isolatedCopy();
}

String String::isolatedCopy() const &&
{
    if (isSafeToSendToAnotherThread()) {
        // Since we know that our string is a temporary that will be destroyed
        // we can just steal the m_impl from it, thus avoiding a copy.
        return String(std::move(*this));
    }

    if (!m_impl)
        return String();

    return m_impl->isolatedCopy();
}
#else
String String::isolatedCopy() const
{
    if (!m_impl)
        return String();
    return m_impl->isolatedCopy();
}
#endif

bool String::isSafeToSendToAnotherThread() const
{
    if (!impl())
        return true;
    // AtomicStrings are not safe to send between threads as ~StringImpl()
    // will try to remove them from the wrong AtomicStringTable.
    if (impl()->isAtomic())
        return false;
    if (impl()->hasOneRef())
        return true;
    if (isEmpty())
        return true;
    return false;
}

void String::split(const String& separator, bool allowEmptyEntries, Vector<String>& result) const
{
    result.clear();

    unsigned startPos = 0;
    size_t endPos;
    while ((endPos = find(separator, startPos)) != notFound) {
        if (allowEmptyEntries || startPos != endPos)
            result.append(substring(startPos, endPos - startPos));
        startPos = endPos + separator.length();
    }
    if (allowEmptyEntries || startPos != length())
        result.append(substring(startPos));
}

void String::split(UChar separator, bool allowEmptyEntries, Vector<String>& result) const
{
    result.clear();

    unsigned startPos = 0;
    size_t endPos;
    while ((endPos = find(separator, startPos)) != notFound) {
        if (allowEmptyEntries || startPos != endPos)
            result.append(substring(startPos, endPos - startPos));
        startPos = endPos + 1;
    }
    if (allowEmptyEntries || startPos != length())
        result.append(substring(startPos));
}

CString String::ascii() const
{
    // Printable ASCII characters 32..127 and the null character are
    // preserved, characters outside of this range are converted to '?'.

    unsigned length = this->length();
    if (!length) { 
        char* characterBuffer;
        return CString::newUninitialized(length, characterBuffer);
    }

    if (this->is8Bit()) {
        const LChar* characters = this->characters8();

        char* characterBuffer;
        CString result = CString::newUninitialized(length, characterBuffer);

        for (unsigned i = 0; i < length; ++i) {
            LChar ch = characters[i];
            characterBuffer[i] = ch && (ch < 0x20 || ch > 0x7f) ? '?' : ch;
        }

        return result;        
    }

    const UChar* characters = this->characters16();

    char* characterBuffer;
    CString result = CString::newUninitialized(length, characterBuffer);

    for (unsigned i = 0; i < length; ++i) {
        UChar ch = characters[i];
        characterBuffer[i] = ch && (ch < 0x20 || ch > 0x7f) ? '?' : ch;
    }

    return result;
}

CString String::latin1() const
{
    // Basic Latin1 (ISO) encoding - Unicode characters 0..255 are
    // preserved, characters outside of this range are converted to '?'.

    unsigned length = this->length();

    if (!length)
        return CString("", 0);

    if (is8Bit())
        return CString(reinterpret_cast<const char*>(this->characters8()), length);

    const UChar* characters = this->characters16();

    char* characterBuffer;
    CString result = CString::newUninitialized(length, characterBuffer);

    for (unsigned i = 0; i < length; ++i) {
        UChar ch = characters[i];
        characterBuffer[i] = ch > 0xff ? '?' : ch;
    }

    return result;
}

// Helper to write a three-byte UTF-8 code point to the buffer, caller must check room is available.
static inline void putUTF8Triple(char*& buffer, UChar ch)
{
    ASSERT(ch >= 0x0800);
    *buffer++ = static_cast<char>(((ch >> 12) & 0x0F) | 0xE0);
    *buffer++ = static_cast<char>(((ch >> 6) & 0x3F) | 0x80);
    *buffer++ = static_cast<char>((ch & 0x3F) | 0x80);
}

CString String::utf8(ConversionMode mode) const
{
    unsigned length = this->length();

    if (!length)
        return CString("", 0);

    // Allocate a buffer big enough to hold all the characters
    // (an individual UTF-16 UChar can only expand to 3 UTF-8 bytes).
    // Optimization ideas, if we find this function is hot:
    //  * We could speculatively create a CStringBuffer to contain 'length' 
    //    characters, and resize if necessary (i.e. if the buffer contains
    //    non-ascii characters). (Alternatively, scan the buffer first for
    //    ascii characters, so we know this will be sufficient).
    //  * We could allocate a CStringBuffer with an appropriate size to
    //    have a good chance of being able to write the string into the
    //    buffer without reallocing (say, 1.5 x length).
    if (length > numeric_limits<unsigned>::max() / 3)
        return CString();
    Vector<char, 1024> bufferVector(length * 3);

    char* buffer = bufferVector.data();

    if (is8Bit()) {
        const LChar* characters = this->characters8();

        ConversionResult result = convertLatin1ToUTF8(&characters, characters + length, &buffer, buffer + bufferVector.size());
        ASSERT_UNUSED(result, result != targetExhausted); // (length * 3) should be sufficient for any conversion
    } else {
        const UChar* characters = this->characters16();

        if (mode == StrictConversionReplacingUnpairedSurrogatesWithFFFD) {
            const UChar* charactersEnd = characters + length;
            char* bufferEnd = buffer + bufferVector.size();
            while (characters < charactersEnd) {
                // Use strict conversion to detect unpaired surrogates.
                ConversionResult result = convertUTF16ToUTF8(&characters, charactersEnd, &buffer, bufferEnd, true);
                ASSERT(result != targetExhausted);
                // Conversion fails when there is an unpaired surrogate.
                // Put replacement character (U+FFFD) instead of the unpaired surrogate.
                if (result != conversionOK) {
                    ASSERT((0xD800 <= *characters && *characters <= 0xDFFF));
                    // There should be room left, since one UChar hasn't been converted.
                    ASSERT((buffer + 3) <= bufferEnd);
                    putUTF8Triple(buffer, replacementCharacter);
                    ++characters;
                }
            }
        } else {
            bool strict = mode == StrictConversion;
            ConversionResult result = convertUTF16ToUTF8(&characters, characters + length, &buffer, buffer + bufferVector.size(), strict);
            ASSERT(result != targetExhausted); // (length * 3) should be sufficient for any conversion

            // Only produced from strict conversion.
            if (result == sourceIllegal) {
                ASSERT(strict);
                return CString();
            }

            // Check for an unconverted high surrogate.
            if (result == sourceExhausted) {
                if (strict)
                    return CString();
                // This should be one unpaired high surrogate. Treat it the same
                // was as an unpaired high surrogate would have been handled in
                // the middle of a string with non-strict conversion - which is
                // to say, simply encode it to UTF-8.
                ASSERT((characters + 1) == (this->characters() + length));
                ASSERT((*characters >= 0xD800) && (*characters <= 0xDBFF));
                // There should be room left, since one UChar hasn't been converted.
                ASSERT((buffer + 3) <= (buffer + bufferVector.size()));
                putUTF8Triple(buffer, *characters);
            }
        }
    }

    return CString(bufferVector.data(), buffer - bufferVector.data());
}

String String::make8BitFrom16BitSource(const UChar* source, size_t length)
{
    if (!length)
        return String();

    LChar* destination;
    String result = String::createUninitialized(length, destination);

    copyLCharsFromUCharSource(destination, source, length);

    return result;
}

String String::make16BitFrom8BitSource(const LChar* source, size_t length)
{
    if (!length)
        return String();
    
    UChar* destination;
    String result = String::createUninitialized(length, destination);
    
    StringImpl::copyChars(destination, source, length);
    
    return result;
}

String String::fromUTF8(const LChar* stringStart, size_t length)
{
    if (length > numeric_limits<unsigned>::max())
        CRASH();

    if (!stringStart)
        return String();

    if (!length)
        return emptyString();

    if (charactersAreAllASCII(stringStart, length))
        return StringImpl::create(stringStart, length);

    Vector<UChar, 1024> buffer(length);
    UChar* bufferStart = buffer.data();
 
    UChar* bufferCurrent = bufferStart;
    const char* stringCurrent = reinterpret_cast<const char*>(stringStart);
    if (convertUTF8ToUTF16(&stringCurrent, reinterpret_cast<const char *>(stringStart + length), &bufferCurrent, bufferCurrent + buffer.size()) != conversionOK)
        return String();

    unsigned utf16Length = bufferCurrent - bufferStart;
    ASSERT(utf16Length < length);
    return StringImpl::create(bufferStart, utf16Length);
}

String String::fromUTF8(const LChar* string)
{
    if (!string)
        return String();
    return fromUTF8(string, strlen(reinterpret_cast<const char*>(string)));
}

String String::fromUTF8(const CString& s)
{
    return fromUTF8(s.data());
}

String String::fromUTF8WithLatin1Fallback(const LChar* string, size_t size)
{
    String utf8 = fromUTF8(string, size);
    if (!utf8)
        return String(string, size);
    return utf8;
}

// String Operations

static bool isCharacterAllowedInBase(UChar c, int base)
{
    if (c > 0x7F)
        return false;
    if (isASCIIDigit(c))
        return c - '0' < base;
    if (isASCIIAlpha(c)) {
        if (base > 36)
            base = 36;
        return (c >= 'a' && c < 'a' + base - 10)
            || (c >= 'A' && c < 'A' + base - 10);
    }
    return false;
}

template <typename IntegralType, typename CharType>
static inline IntegralType toIntegralType(const CharType* data, size_t length, bool* ok, int base)
{
    static const IntegralType integralMax = numeric_limits<IntegralType>::max();
    static const bool isSigned = numeric_limits<IntegralType>::is_signed;
    const IntegralType maxMultiplier = integralMax / base;

    IntegralType value = 0;
    bool isOk = false;
    bool isNegative = false;

    if (!data)
        goto bye;

    // skip leading whitespace
    while (length && isSpaceOrNewline(*data)) {
        --length;
        ++data;
    }

    if (isSigned && length && *data == '-') {
        --length;
        ++data;
        isNegative = true;
    } else if (length && *data == '+') {
        --length;
        ++data;
    }

    if (!length || !isCharacterAllowedInBase(*data, base))
        goto bye;

    while (length && isCharacterAllowedInBase(*data, base)) {
        --length;
        IntegralType digitValue;
        CharType c = *data;
        if (isASCIIDigit(c))
            digitValue = c - '0';
        else if (c >= 'a')
            digitValue = c - 'a' + 10;
        else
            digitValue = c - 'A' + 10;

        if (value > maxMultiplier || (value == maxMultiplier && digitValue > (integralMax % base) + isNegative))
            goto bye;

        value = base * value + digitValue;
        ++data;
    }

#if COMPILER(MSVC)
#pragma warning(push, 0)
#pragma warning(disable:4146)
#endif

    if (isNegative)
        value = -value;

#if COMPILER(MSVC)
#pragma warning(pop)
#endif

    // skip trailing space
    while (length && isSpaceOrNewline(*data)) {
        --length;
        ++data;
    }

    if (!length)
        isOk = true;
bye:
    if (ok)
        *ok = isOk;
    return isOk ? value : 0;
}

template <typename CharType>
static unsigned lengthOfCharactersAsInteger(const CharType* data, size_t length)
{
    size_t i = 0;

    // Allow leading spaces.
    for (; i != length; ++i) {
        if (!isSpaceOrNewline(data[i]))
            break;
    }
    
    // Allow sign.
    if (i != length && (data[i] == '+' || data[i] == '-'))
        ++i;
    
    // Allow digits.
    for (; i != length; ++i) {
        if (!isASCIIDigit(data[i]))
            break;
    }

    return i;
}

int charactersToIntStrict(const LChar* data, size_t length, bool* ok, int base)
{
    return toIntegralType<int, LChar>(data, length, ok, base);
}

int charactersToIntStrict(const UChar* data, size_t length, bool* ok, int base)
{
    return toIntegralType<int, UChar>(data, length, ok, base);
}

unsigned charactersToUIntStrict(const LChar* data, size_t length, bool* ok, int base)
{
    return toIntegralType<unsigned, LChar>(data, length, ok, base);
}

unsigned charactersToUIntStrict(const UChar* data, size_t length, bool* ok, int base)
{
    return toIntegralType<unsigned, UChar>(data, length, ok, base);
}

int64_t charactersToInt64Strict(const LChar* data, size_t length, bool* ok, int base)
{
    return toIntegralType<int64_t, LChar>(data, length, ok, base);
}

int64_t charactersToInt64Strict(const UChar* data, size_t length, bool* ok, int base)
{
    return toIntegralType<int64_t, UChar>(data, length, ok, base);
}

uint64_t charactersToUInt64Strict(const LChar* data, size_t length, bool* ok, int base)
{
    return toIntegralType<uint64_t, LChar>(data, length, ok, base);
}

uint64_t charactersToUInt64Strict(const UChar* data, size_t length, bool* ok, int base)
{
    return toIntegralType<uint64_t, UChar>(data, length, ok, base);
}

intptr_t charactersToIntPtrStrict(const LChar* data, size_t length, bool* ok, int base)
{
    return toIntegralType<intptr_t, LChar>(data, length, ok, base);
}

intptr_t charactersToIntPtrStrict(const UChar* data, size_t length, bool* ok, int base)
{
    return toIntegralType<intptr_t, UChar>(data, length, ok, base);
}

int charactersToInt(const LChar* data, size_t length, bool* ok)
{
    return toIntegralType<int, LChar>(data, lengthOfCharactersAsInteger<LChar>(data, length), ok, 10);
}

int charactersToInt(const UChar* data, size_t length, bool* ok)
{
    return toIntegralType<int, UChar>(data, lengthOfCharactersAsInteger(data, length), ok, 10);
}

unsigned charactersToUInt(const LChar* data, size_t length, bool* ok)
{
    return toIntegralType<unsigned, LChar>(data, lengthOfCharactersAsInteger<LChar>(data, length), ok, 10);
}

unsigned charactersToUInt(const UChar* data, size_t length, bool* ok)
{
    return toIntegralType<unsigned, UChar>(data, lengthOfCharactersAsInteger<UChar>(data, length), ok, 10);
}

int64_t charactersToInt64(const LChar* data, size_t length, bool* ok)
{
    return toIntegralType<int64_t, LChar>(data, lengthOfCharactersAsInteger<LChar>(data, length), ok, 10);
}

int64_t charactersToInt64(const UChar* data, size_t length, bool* ok)
{
    return toIntegralType<int64_t, UChar>(data, lengthOfCharactersAsInteger<UChar>(data, length), ok, 10);
}

uint64_t charactersToUInt64(const LChar* data, size_t length, bool* ok)
{
    return toIntegralType<uint64_t, LChar>(data, lengthOfCharactersAsInteger<LChar>(data, length), ok, 10);
}

uint64_t charactersToUInt64(const UChar* data, size_t length, bool* ok)
{
    return toIntegralType<uint64_t, UChar>(data, lengthOfCharactersAsInteger<UChar>(data, length), ok, 10);
}

intptr_t charactersToIntPtr(const LChar* data, size_t length, bool* ok)
{
    return toIntegralType<intptr_t, LChar>(data, lengthOfCharactersAsInteger<LChar>(data, length), ok, 10);
}

intptr_t charactersToIntPtr(const UChar* data, size_t length, bool* ok)
{
    return toIntegralType<intptr_t, UChar>(data, lengthOfCharactersAsInteger<UChar>(data, length), ok, 10);
}

enum TrailingJunkPolicy { DisallowTrailingJunk, AllowTrailingJunk };

template <typename CharType, TrailingJunkPolicy policy>
static inline double toDoubleType(const CharType* data, size_t length, bool* ok, size_t& parsedLength)
{
    size_t leadingSpacesLength = 0;
    while (leadingSpacesLength < length && isASCIISpace(data[leadingSpacesLength]))
        ++leadingSpacesLength;

    double number = parseDouble(data + leadingSpacesLength, length - leadingSpacesLength, parsedLength);
    if (!parsedLength) {
        if (ok)
            *ok = false;
        return 0.0;
    }

    parsedLength += leadingSpacesLength;
    if (ok)
        *ok = policy == AllowTrailingJunk || parsedLength == length;
    return number;
}

double charactersToDouble(const LChar* data, size_t length, bool* ok)
{
    size_t parsedLength;
    return toDoubleType<LChar, DisallowTrailingJunk>(data, length, ok, parsedLength);
}

double charactersToDouble(const UChar* data, size_t length, bool* ok)
{
    size_t parsedLength;
    return toDoubleType<UChar, DisallowTrailingJunk>(data, length, ok, parsedLength);
}

float charactersToFloat(const LChar* data, size_t length, bool* ok)
{
    // FIXME: This will return ok even when the string fits into a double but not a float.
    size_t parsedLength;
    return static_cast<float>(toDoubleType<LChar, DisallowTrailingJunk>(data, length, ok, parsedLength));
}

float charactersToFloat(const UChar* data, size_t length, bool* ok)
{
    // FIXME: This will return ok even when the string fits into a double but not a float.
    size_t parsedLength;
    return static_cast<float>(toDoubleType<UChar, DisallowTrailingJunk>(data, length, ok, parsedLength));
}

float charactersToFloat(const LChar* data, size_t length, size_t& parsedLength)
{
    // FIXME: This will return ok even when the string fits into a double but not a float.
    return static_cast<float>(toDoubleType<LChar, AllowTrailingJunk>(data, length, 0, parsedLength));
}

float charactersToFloat(const UChar* data, size_t length, size_t& parsedLength)
{
    // FIXME: This will return ok even when the string fits into a double but not a float.
    return static_cast<float>(toDoubleType<UChar, AllowTrailingJunk>(data, length, 0, parsedLength));
}

const String& emptyString()
{
    static NeverDestroyed<String> emptyString(StringImpl::empty());

    return emptyString;
}

} // namespace WTF

#ifndef NDEBUG
// For use in the debugger
String* string(const char*);
Vector<char> asciiDebug(StringImpl* impl);
Vector<char> asciiDebug(String& string);

void String::show() const
{
    dataLogF("%s\n", asciiDebug(impl()).data());
}

String* string(const char* s)
{
    // leaks memory!
    return new String(s);
}

Vector<char> asciiDebug(StringImpl* impl)
{
    if (!impl)
        return asciiDebug(String("[null]").impl());

    Vector<char> buffer;
    for (unsigned i = 0; i < impl->length(); ++i) {
        UChar ch = (*impl)[i];
        if (isASCIIPrintable(ch)) {
            if (ch == '\\')
                buffer.append(ch);
            buffer.append(ch);
        } else {
            buffer.append('\\');
            buffer.append('u');
            appendUnsignedAsHexFixedSize(ch, buffer, 4);
        }
    }
    buffer.append('\0');
    return buffer;
}

Vector<char> asciiDebug(String& string)
{
    return asciiDebug(string.impl());
}

#endif
