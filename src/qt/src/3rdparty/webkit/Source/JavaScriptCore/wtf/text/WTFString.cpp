/*
 * (C) 1999 Lars Knoll (knoll@kde.org)
 * Copyright (C) 2004, 2005, 2006, 2007, 2008, 2010 Apple Inc. All rights reserved.
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

#include <stdarg.h>
#include <wtf/ASCIICType.h>
#include <wtf/text/CString.h>
#include <wtf/StringExtras.h>
#include <wtf/Vector.h>
#include <wtf/dtoa.h>
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
        
    size_t len = 0;
    while (str[len] != UChar(0))
        len++;

    if (len > numeric_limits<unsigned>::max())
        CRASH();
    
    m_impl = StringImpl::create(str, len);
}

// Construct a string with latin1 data.
String::String(const char* characters, unsigned length)
    : m_impl(characters ? StringImpl::create(characters, length) : 0)
{
}

// Construct a string with latin1 data, from a null-terminated source.
String::String(const char* characters)
    : m_impl(characters ? StringImpl::create(characters) : 0)
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

void String::append(char c)
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

void String::append(UChar c)
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

String operator+(const String& a, const String& b)
{
    if (a.isEmpty())
        return b;
    if (b.isEmpty())
        return a;
    String c = a;
    c += b;
    return c;
}

String operator+(const String& s, const char* cs)
{
    return s + String(cs);
}

String operator+(const char* cs, const String& s)
{
    return String(cs) + s;
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

    ASSERT(charactersToAppend);
    UChar* data;
    if (lengthToAppend > numeric_limits<unsigned>::max() - length())
        CRASH();
    RefPtr<StringImpl> newImpl = StringImpl::createUninitialized(length() + lengthToAppend, data);
    memcpy(data, characters(), length() * sizeof(UChar));
    memcpy(data + length(), charactersToAppend, lengthToAppend * sizeof(UChar));
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

void String::remove(unsigned position, int lengthToRemove)
{
    if (lengthToRemove <= 0)
        return;
    if (position >= length())
        return;
    if (static_cast<unsigned>(lengthToRemove) > length() - position)
        lengthToRemove = length() - position;
    UChar* data;
    RefPtr<StringImpl> newImpl = StringImpl::createUninitialized(length() - lengthToRemove, data);
    memcpy(data, characters(), position * sizeof(UChar));
    memcpy(data + position, characters() + position + lengthToRemove,
        (length() - lengthToRemove - position) * sizeof(UChar));
    m_impl = newImpl.release();
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

String String::simplifyWhiteSpace() const
{
    if (!m_impl)
        return String();
    return m_impl->simplifyWhiteSpace();
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

const UChar* String::charactersWithNullTermination()
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
    return StringImpl::create(ba.constData(), ba.length());

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
            return StringImpl::create(buffer.data(), written);
        
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
    
    return StringImpl::create(buffer.data(), len);
#endif
}

String String::number(short n)
{
    return String::format("%hd", n);
}

String String::number(unsigned short n)
{
    return String::format("%hu", n);
}

String String::number(int n)
{
    return String::format("%d", n);
}

String String::number(unsigned n)
{
    return String::format("%u", n);
}

String String::number(long n)
{
    return String::format("%ld", n);
}

String String::number(unsigned long n)
{
    return String::format("%lu", n);
}

String String::number(long long n)
{
#if OS(WINDOWS) && !PLATFORM(QT)
    return String::format("%I64i", n);
#else
    return String::format("%lli", n);
#endif
}

String String::number(unsigned long long n)
{
#if OS(WINDOWS) && !PLATFORM(QT)
    return String::format("%I64u", n);
#else
    return String::format("%llu", n);
#endif
}
    
String String::number(double n)
{
    return String::format("%.6lg", n);
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

double String::toDouble(bool* ok, bool* didReadNumber) const
{
    if (!m_impl) {
        if (ok)
            *ok = false;
        if (didReadNumber)
            *didReadNumber = false;
        return 0.0;
    }
    return m_impl->toDouble(ok, didReadNumber);
}

float String::toFloat(bool* ok, bool* didReadNumber) const
{
    if (!m_impl) {
        if (ok)
            *ok = false;
        if (didReadNumber)
            *didReadNumber = false;
        return 0.0f;
    }
    return m_impl->toFloat(ok, didReadNumber);
}

String String::threadsafeCopy() const
{
    if (!m_impl)
        return String();
    return m_impl->threadsafeCopy();
}

String String::crossThreadString() const
{
    if (!m_impl)
        return String();
    return m_impl->crossThreadString();
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

void String::split(const String& separator, Vector<String>& result) const
{
    split(separator, false, result);
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

void String::split(UChar separator, Vector<String>& result) const
{
    split(String(&separator, 1), false, result);
}

CString String::ascii() const
{
    // Printable ASCII characters 32..127 and the null character are
    // preserved, characters outside of this range are converted to '?'.

    unsigned length = this->length();
    const UChar* characters = this->characters();

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
    const UChar* characters = this->characters();

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

CString String::utf8(bool strict) const
{
    unsigned length = this->length();
    const UChar* characters = this->characters();

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
    ConversionResult result = convertUTF16ToUTF8(&characters, characters + length, &buffer, buffer + bufferVector.size(), strict);
    ASSERT(result != targetExhausted); // (length * 3) should be sufficient for any conversion

    // Only produced from strict conversion.
    if (result == sourceIllegal)
        return CString();

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

    return CString(bufferVector.data(), buffer - bufferVector.data());
}

String String::fromUTF8(const char* stringStart, size_t length)
{
    if (length > numeric_limits<unsigned>::max())
        CRASH();

    if (!stringStart)
        return String();

    // We'll use a StringImpl as a buffer; if the source string only contains ascii this should be
    // the right length, if there are any multi-byte sequences this buffer will be too large.
    UChar* buffer;
    String stringBuffer(StringImpl::createUninitialized(length, buffer));
    UChar* bufferEnd = buffer + length;

    // Try converting into the buffer.
    const char* stringCurrent = stringStart;
    if (convertUTF8ToUTF16(&stringCurrent, stringStart + length, &buffer, bufferEnd) != conversionOK)
        return String();

    // stringBuffer is full (the input must have been all ascii) so just return it!
    if (buffer == bufferEnd)
        return stringBuffer;

    // stringBuffer served its purpose as a buffer, copy the contents out into a new string.
    unsigned utf16Length = buffer - stringBuffer.characters();
    ASSERT(utf16Length < length);
    return String(stringBuffer.characters(), utf16Length);
}

String String::fromUTF8(const char* string)
{
    if (!string)
        return String();
    return fromUTF8(string, strlen(string));
}

String String::fromUTF8WithLatin1Fallback(const char* string, size_t size)
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

template <typename IntegralType>
static inline IntegralType toIntegralType(const UChar* data, size_t length, bool* ok, int base)
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
        length--;
        data++;
    }

    if (isSigned && length && *data == '-') {
        length--;
        data++;
        isNegative = true;
    } else if (length && *data == '+') {
        length--;
        data++;
    }

    if (!length || !isCharacterAllowedInBase(*data, base))
        goto bye;

    while (length && isCharacterAllowedInBase(*data, base)) {
        length--;
        IntegralType digitValue;
        UChar c = *data;
        if (isASCIIDigit(c))
            digitValue = c - '0';
        else if (c >= 'a')
            digitValue = c - 'a' + 10;
        else
            digitValue = c - 'A' + 10;

        if (value > maxMultiplier || (value == maxMultiplier && digitValue > (integralMax % base) + isNegative))
            goto bye;

        value = base * value + digitValue;
        data++;
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
        length--;
        data++;
    }

    if (!length)
        isOk = true;
bye:
    if (ok)
        *ok = isOk;
    return isOk ? value : 0;
}

static unsigned lengthOfCharactersAsInteger(const UChar* data, size_t length)
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

int charactersToIntStrict(const UChar* data, size_t length, bool* ok, int base)
{
    return toIntegralType<int>(data, length, ok, base);
}

unsigned charactersToUIntStrict(const UChar* data, size_t length, bool* ok, int base)
{
    return toIntegralType<unsigned>(data, length, ok, base);
}

int64_t charactersToInt64Strict(const UChar* data, size_t length, bool* ok, int base)
{
    return toIntegralType<int64_t>(data, length, ok, base);
}

uint64_t charactersToUInt64Strict(const UChar* data, size_t length, bool* ok, int base)
{
    return toIntegralType<uint64_t>(data, length, ok, base);
}

intptr_t charactersToIntPtrStrict(const UChar* data, size_t length, bool* ok, int base)
{
    return toIntegralType<intptr_t>(data, length, ok, base);
}

int charactersToInt(const UChar* data, size_t length, bool* ok)
{
    return toIntegralType<int>(data, lengthOfCharactersAsInteger(data, length), ok, 10);
}

unsigned charactersToUInt(const UChar* data, size_t length, bool* ok)
{
    return toIntegralType<unsigned>(data, lengthOfCharactersAsInteger(data, length), ok, 10);
}

int64_t charactersToInt64(const UChar* data, size_t length, bool* ok)
{
    return toIntegralType<int64_t>(data, lengthOfCharactersAsInteger(data, length), ok, 10);
}

uint64_t charactersToUInt64(const UChar* data, size_t length, bool* ok)
{
    return toIntegralType<uint64_t>(data, lengthOfCharactersAsInteger(data, length), ok, 10);
}

intptr_t charactersToIntPtr(const UChar* data, size_t length, bool* ok)
{
    return toIntegralType<intptr_t>(data, lengthOfCharactersAsInteger(data, length), ok, 10);
}

double charactersToDouble(const UChar* data, size_t length, bool* ok, bool* didReadNumber)
{
    if (!length) {
        if (ok)
            *ok = false;
        if (didReadNumber)
            *didReadNumber = false;
        return 0.0;
    }

    Vector<char, 256> bytes(length + 1);
    for (unsigned i = 0; i < length; ++i)
        bytes[i] = data[i] < 0x7F ? data[i] : '?';
    bytes[length] = '\0';
    char* start = bytes.data();
    char* end;
    double val = WTF::strtod(start, &end);
    if (ok)
        *ok = (end == 0 || *end == '\0');
    if (didReadNumber)
        *didReadNumber = end - start;
    return val;
}

float charactersToFloat(const UChar* data, size_t length, bool* ok, bool* didReadNumber)
{
    // FIXME: This will return ok even when the string fits into a double but not a float.
    return static_cast<float>(charactersToDouble(data, length, ok, didReadNumber));
}

} // namespace WTF

#ifndef NDEBUG
// For use in the debugger
String* string(const char*);
Vector<char> asciiDebug(StringImpl* impl);
Vector<char> asciiDebug(String& string);

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
    unsigned length = impl->length();
    const UChar* characters = impl->characters();

    buffer.resize(length + 1);
    for (unsigned i = 0; i < length; ++i) {
        UChar ch = characters[i];
        buffer[i] = ch && (ch < 0x20 || ch > 0x7f) ? '?' : ch;
    }
    buffer[length] = '\0';

    return buffer;
}

Vector<char> asciiDebug(String& string)
{
    return asciiDebug(string.impl());
}

#endif
