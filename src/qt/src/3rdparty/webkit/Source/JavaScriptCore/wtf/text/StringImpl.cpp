/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 *           (C) 2001 Dirk Mueller ( mueller@kde.org )
 * Copyright (C) 2003, 2004, 2005, 2006, 2007, 2008, 2009 Apple Inc. All rights reserved.
 * Copyright (C) 2006 Andrew Wellington (proton@wiretapped.net)
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
 *
 */

#include "config.h"
#include "StringImpl.h"

#include "AtomicString.h"
#include "StringBuffer.h"
#include "StringHash.h"
#include <wtf/StdLibExtras.h>
#include <wtf/WTFThreadData.h>

using namespace std;

namespace WTF {

using namespace Unicode;

static const unsigned minLengthToShare = 20;

COMPILE_ASSERT(sizeof(StringImpl) == 2 * sizeof(int) + 3 * sizeof(void*), StringImpl_should_stay_small);

StringImpl::~StringImpl()
{
    ASSERT(!isStatic());

    if (isAtomic())
        AtomicString::remove(this);
#if USE(JSC)
    if (isIdentifier()) {
        if (!wtfThreadData().currentIdentifierTable()->remove(this))
            CRASH();
    }
#endif

    BufferOwnership ownership = bufferOwnership();
    if (ownership != BufferInternal) {
        if (ownership == BufferOwned) {
            ASSERT(!m_sharedBuffer);
            ASSERT(m_data);
            fastFree(const_cast<UChar*>(m_data));
        } else if (ownership == BufferSubstring) {
            ASSERT(m_substringBuffer);
            m_substringBuffer->deref();
        } else {
            ASSERT(ownership == BufferShared);
            ASSERT(m_sharedBuffer);
            m_sharedBuffer->deref();
        }
    }
}

PassRefPtr<StringImpl> StringImpl::createUninitialized(unsigned length, UChar*& data)
{
    if (!length) {
        data = 0;
        return empty();
    }

    // Allocate a single buffer large enough to contain the StringImpl
    // struct as well as the data which it contains. This removes one 
    // heap allocation from this call.
    if (length > ((std::numeric_limits<unsigned>::max() - sizeof(StringImpl)) / sizeof(UChar)))
        CRASH();
    size_t size = sizeof(StringImpl) + length * sizeof(UChar);
    StringImpl* string = static_cast<StringImpl*>(fastMalloc(size));

    data = reinterpret_cast<UChar*>(string + 1);
    return adoptRef(new (string) StringImpl(length));
}

PassRefPtr<StringImpl> StringImpl::create(const UChar* characters, unsigned length)
{
    if (!characters || !length)
        return empty();

    UChar* data;
    RefPtr<StringImpl> string = createUninitialized(length, data);
    memcpy(data, characters, length * sizeof(UChar));
    return string.release();
}

PassRefPtr<StringImpl> StringImpl::create(const char* characters, unsigned length)
{
    if (!characters || !length)
        return empty();

    UChar* data;
    RefPtr<StringImpl> string = createUninitialized(length, data);
    for (unsigned i = 0; i != length; ++i) {
        unsigned char c = characters[i];
        data[i] = c;
    }
    return string.release();
}

PassRefPtr<StringImpl> StringImpl::create(const char* string)
{
    if (!string)
        return empty();
    size_t length = strlen(string);
    if (length > numeric_limits<unsigned>::max())
        CRASH();
    return create(string, length);
}

PassRefPtr<StringImpl> StringImpl::create(const UChar* characters, unsigned length, PassRefPtr<SharedUChar> sharedBuffer)
{
    ASSERT(characters);
    ASSERT(minLengthToShare && length >= minLengthToShare);
    return adoptRef(new StringImpl(characters, length, sharedBuffer));
}

SharedUChar* StringImpl::sharedBuffer()
{
    if (m_length < minLengthToShare)
        return 0;
    // All static strings are smaller that the minimim length to share.
    ASSERT(!isStatic());

    BufferOwnership ownership = bufferOwnership();

    if (ownership == BufferInternal)
        return 0;
    if (ownership == BufferSubstring)
        return m_substringBuffer->sharedBuffer();
    if (ownership == BufferOwned) {
        ASSERT(!m_sharedBuffer);
        m_sharedBuffer = SharedUChar::create(new SharableUChar(m_data)).leakRef();
        m_refCountAndFlags = (m_refCountAndFlags & ~s_refCountMaskBufferOwnership) | BufferShared;
    }

    ASSERT(bufferOwnership() == BufferShared);
    ASSERT(m_sharedBuffer);
    return m_sharedBuffer;
}

bool StringImpl::containsOnlyWhitespace()
{
    // FIXME: The definition of whitespace here includes a number of characters
    // that are not whitespace from the point of view of RenderText; I wonder if
    // that's a problem in practice.
    for (unsigned i = 0; i < m_length; i++)
        if (!isASCIISpace(m_data[i]))
            return false;
    return true;
}

PassRefPtr<StringImpl> StringImpl::substring(unsigned start, unsigned length)
{
    if (start >= m_length)
        return empty();
    unsigned maxLength = m_length - start;
    if (length >= maxLength) {
        if (!start)
            return this;
        length = maxLength;
    }
    return create(m_data + start, length);
}

UChar32 StringImpl::characterStartingAt(unsigned i)
{
    if (U16_IS_SINGLE(m_data[i]))
        return m_data[i];
    if (i + 1 < m_length && U16_IS_LEAD(m_data[i]) && U16_IS_TRAIL(m_data[i + 1]))
        return U16_GET_SUPPLEMENTARY(m_data[i], m_data[i + 1]);
    return 0;
}

PassRefPtr<StringImpl> StringImpl::lower()
{
    // Note: This is a hot function in the Dromaeo benchmark, specifically the
    // no-op code path up through the first 'return' statement.
    
    // First scan the string for uppercase and non-ASCII characters:
    UChar ored = 0;
    bool noUpper = true;
    const UChar *end = m_data + m_length;
    for (const UChar* chp = m_data; chp != end; chp++) {
        if (UNLIKELY(isASCIIUpper(*chp)))
            noUpper = false;
        ored |= *chp;
    }
    
    // Nothing to do if the string is all ASCII with no uppercase.
    if (noUpper && !(ored & ~0x7F))
        return this;

    if (m_length > static_cast<unsigned>(numeric_limits<int32_t>::max()))
        CRASH();
    int32_t length = m_length;

    UChar* data;
    RefPtr<StringImpl> newImpl = createUninitialized(m_length, data);

    if (!(ored & ~0x7F)) {
        // Do a faster loop for the case where all the characters are ASCII.
        for (int i = 0; i < length; i++) {
            UChar c = m_data[i];
            data[i] = toASCIILower(c);
        }
        return newImpl;
    }
    
    // Do a slower implementation for cases that include non-ASCII characters.
    bool error;
    int32_t realLength = Unicode::toLower(data, length, m_data, m_length, &error);
    if (!error && realLength == length)
        return newImpl;
    newImpl = createUninitialized(realLength, data);
    Unicode::toLower(data, realLength, m_data, m_length, &error);
    if (error)
        return this;
    return newImpl;
}

PassRefPtr<StringImpl> StringImpl::upper()
{
    // This function could be optimized for no-op cases the way lower() is,
    // but in empirical testing, few actual calls to upper() are no-ops, so
    // it wouldn't be worth the extra time for pre-scanning.
    UChar* data;
    RefPtr<StringImpl> newImpl = createUninitialized(m_length, data);

    if (m_length > static_cast<unsigned>(numeric_limits<int32_t>::max()))
        CRASH();
    int32_t length = m_length;

    // Do a faster loop for the case where all the characters are ASCII.
    UChar ored = 0;
    for (int i = 0; i < length; i++) {
        UChar c = m_data[i];
        ored |= c;
        data[i] = toASCIIUpper(c);
    }
    if (!(ored & ~0x7F))
        return newImpl.release();

    // Do a slower implementation for cases that include non-ASCII characters.
    bool error;
    int32_t realLength = Unicode::toUpper(data, length, m_data, m_length, &error);
    if (!error && realLength == length)
        return newImpl;
    newImpl = createUninitialized(realLength, data);
    Unicode::toUpper(data, realLength, m_data, m_length, &error);
    if (error)
        return this;
    return newImpl.release();
}

PassRefPtr<StringImpl> StringImpl::fill(UChar character)
{
    if (!m_length)
        return this;

    UChar* data;
    RefPtr<StringImpl> newImpl = createUninitialized(m_length, data);
    for (unsigned i = 0; i < m_length; ++i)
        data[i] = character;
    return newImpl.release();
}

PassRefPtr<StringImpl> StringImpl::foldCase()
{
    UChar* data;
    RefPtr<StringImpl> newImpl = createUninitialized(m_length, data);

    if (m_length > static_cast<unsigned>(numeric_limits<int32_t>::max()))
        CRASH();
    int32_t length = m_length;

    // Do a faster loop for the case where all the characters are ASCII.
    UChar ored = 0;
    for (int32_t i = 0; i < length; i++) {
        UChar c = m_data[i];
        ored |= c;
        data[i] = toASCIILower(c);
    }
    if (!(ored & ~0x7F))
        return newImpl.release();

    // Do a slower implementation for cases that include non-ASCII characters.
    bool error;
    int32_t realLength = Unicode::foldCase(data, length, m_data, m_length, &error);
    if (!error && realLength == length)
        return newImpl.release();
    newImpl = createUninitialized(realLength, data);
    Unicode::foldCase(data, realLength, m_data, m_length, &error);
    if (error)
        return this;
    return newImpl.release();
}

PassRefPtr<StringImpl> StringImpl::stripWhiteSpace()
{
    if (!m_length)
        return empty();

    unsigned start = 0;
    unsigned end = m_length - 1;
    
    // skip white space from start
    while (start <= end && isSpaceOrNewline(m_data[start]))
        start++;
    
    // only white space
    if (start > end) 
        return empty();

    // skip white space from end
    while (end && isSpaceOrNewline(m_data[end]))
        end--;

    if (!start && end == m_length - 1)
        return this;
    return create(m_data + start, end + 1 - start);
}

PassRefPtr<StringImpl> StringImpl::removeCharacters(CharacterMatchFunctionPtr findMatch)
{
    const UChar* from = m_data;
    const UChar* fromend = from + m_length;

    // Assume the common case will not remove any characters
    while (from != fromend && !findMatch(*from))
        from++;
    if (from == fromend)
        return this;

    StringBuffer data(m_length);
    UChar* to = data.characters();
    unsigned outc = from - m_data;

    if (outc)
        memcpy(to, m_data, outc * sizeof(UChar));

    while (true) {
        while (from != fromend && findMatch(*from))
            from++;
        while (from != fromend && !findMatch(*from))
            to[outc++] = *from++;
        if (from == fromend)
            break;
    }

    data.shrink(outc);

    return adopt(data);
}

PassRefPtr<StringImpl> StringImpl::simplifyWhiteSpace()
{
    StringBuffer data(m_length);

    const UChar* from = m_data;
    const UChar* fromend = from + m_length;
    int outc = 0;
    bool changedToSpace = false;
    
    UChar* to = data.characters();
    
    while (true) {
        while (from != fromend && isSpaceOrNewline(*from)) {
            if (*from != ' ')
                changedToSpace = true;
            from++;
        }
        while (from != fromend && !isSpaceOrNewline(*from))
            to[outc++] = *from++;
        if (from != fromend)
            to[outc++] = ' ';
        else
            break;
    }
    
    if (outc > 0 && to[outc - 1] == ' ')
        outc--;
    
    if (static_cast<unsigned>(outc) == m_length && !changedToSpace)
        return this;
    
    data.shrink(outc);
    
    return adopt(data);
}

int StringImpl::toIntStrict(bool* ok, int base)
{
    return charactersToIntStrict(m_data, m_length, ok, base);
}

unsigned StringImpl::toUIntStrict(bool* ok, int base)
{
    return charactersToUIntStrict(m_data, m_length, ok, base);
}

int64_t StringImpl::toInt64Strict(bool* ok, int base)
{
    return charactersToInt64Strict(m_data, m_length, ok, base);
}

uint64_t StringImpl::toUInt64Strict(bool* ok, int base)
{
    return charactersToUInt64Strict(m_data, m_length, ok, base);
}

intptr_t StringImpl::toIntPtrStrict(bool* ok, int base)
{
    return charactersToIntPtrStrict(m_data, m_length, ok, base);
}

int StringImpl::toInt(bool* ok)
{
    return charactersToInt(m_data, m_length, ok);
}

unsigned StringImpl::toUInt(bool* ok)
{
    return charactersToUInt(m_data, m_length, ok);
}

int64_t StringImpl::toInt64(bool* ok)
{
    return charactersToInt64(m_data, m_length, ok);
}

uint64_t StringImpl::toUInt64(bool* ok)
{
    return charactersToUInt64(m_data, m_length, ok);
}

intptr_t StringImpl::toIntPtr(bool* ok)
{
    return charactersToIntPtr(m_data, m_length, ok);
}

double StringImpl::toDouble(bool* ok, bool* didReadNumber)
{
    return charactersToDouble(m_data, m_length, ok, didReadNumber);
}

float StringImpl::toFloat(bool* ok, bool* didReadNumber)
{
    return charactersToFloat(m_data, m_length, ok, didReadNumber);
}

static bool equal(const UChar* a, const char* b, int length)
{
    ASSERT(length >= 0);
    while (length--) {
        unsigned char bc = *b++;
        if (*a++ != bc)
            return false;
    }
    return true;
}

bool equalIgnoringCase(const UChar* a, const char* b, unsigned length)
{
    while (length--) {
        unsigned char bc = *b++;
        if (foldCase(*a++) != foldCase(bc))
            return false;
    }
    return true;
}

static inline bool equalIgnoringCase(const UChar* a, const UChar* b, int length)
{
    ASSERT(length >= 0);
    return umemcasecmp(a, b, length) == 0;
}

int codePointCompare(const StringImpl* s1, const StringImpl* s2)
{
    const unsigned l1 = s1 ? s1->length() : 0;
    const unsigned l2 = s2 ? s2->length() : 0;
    const unsigned lmin = l1 < l2 ? l1 : l2;
    const UChar* c1 = s1 ? s1->characters() : 0;
    const UChar* c2 = s2 ? s2->characters() : 0;
    unsigned pos = 0;
    while (pos < lmin && *c1 == *c2) {
        c1++;
        c2++;
        pos++;
    }

    if (pos < lmin)
        return (c1[0] > c2[0]) ? 1 : -1;

    if (l1 == l2)
        return 0;

    return (l1 > l2) ? 1 : -1;
}

size_t StringImpl::find(UChar c, unsigned start)
{
    return WTF::find(m_data, m_length, c, start);
}

size_t StringImpl::find(CharacterMatchFunctionPtr matchFunction, unsigned start)
{
    return WTF::find(m_data, m_length, matchFunction, start);
}

size_t StringImpl::find(const char* matchString, unsigned index)
{
    // Check for null or empty string to match against
    if (!matchString)
        return notFound;
    size_t matchStringLength = strlen(matchString);
    if (matchStringLength > numeric_limits<unsigned>::max())
        CRASH();
    unsigned matchLength = matchStringLength;
    if (!matchLength)
        return min(index, length());

    // Optimization 1: fast case for strings of length 1.
    if (matchLength == 1)
        return WTF::find(characters(), length(), *(const unsigned char*)matchString, index);

    // Check index & matchLength are in range.
    if (index > length())
        return notFound;
    unsigned searchLength = length() - index;
    if (matchLength > searchLength)
        return notFound;
    // delta is the number of additional times to test; delta == 0 means test only once.
    unsigned delta = searchLength - matchLength;

    const UChar* searchCharacters = characters() + index;
    const unsigned char* matchCharacters = (const unsigned char*)matchString;

    // Optimization 2: keep a running hash of the strings,
    // only call memcmp if the hashes match.
    unsigned searchHash = 0;
    unsigned matchHash = 0;
    for (unsigned i = 0; i < matchLength; ++i) {
        searchHash += searchCharacters[i];
        matchHash += matchCharacters[i];
    }

    unsigned i = 0;
    // keep looping until we match
    while (searchHash != matchHash || !equal(searchCharacters + i, matchString, matchLength)) {
        if (i == delta)
            return notFound;
        searchHash += searchCharacters[i + matchLength];
        searchHash -= searchCharacters[i];
        ++i;
    }
    return index + i;
}

size_t StringImpl::findIgnoringCase(const char* matchString, unsigned index)
{
    // Check for null or empty string to match against
    if (!matchString)
        return notFound;
    size_t matchStringLength = strlen(matchString);
    if (matchStringLength > numeric_limits<unsigned>::max())
        CRASH();
    unsigned matchLength = matchStringLength;
    if (!matchLength)
        return min(index, length());

    // Check index & matchLength are in range.
    if (index > length())
        return notFound;
    unsigned searchLength = length() - index;
    if (matchLength > searchLength)
        return notFound;
    // delta is the number of additional times to test; delta == 0 means test only once.
    unsigned delta = searchLength - matchLength;

    const UChar* searchCharacters = characters() + index;

    unsigned i = 0;
    // keep looping until we match
    while (!equalIgnoringCase(searchCharacters + i, matchString, matchLength)) {
        if (i == delta)
            return notFound;
        ++i;
    }
    return index + i;
}

size_t StringImpl::find(StringImpl* matchString, unsigned index)
{
    // Check for null or empty string to match against
    if (!matchString)
        return notFound;
    unsigned matchLength = matchString->length();
    if (!matchLength)
        return min(index, length());

    // Optimization 1: fast case for strings of length 1.
    if (matchLength == 1)
        return WTF::find(characters(), length(), matchString->characters()[0], index);

    // Check index & matchLength are in range.
    if (index > length())
        return notFound;
    unsigned searchLength = length() - index;
    if (matchLength > searchLength)
        return notFound;
    // delta is the number of additional times to test; delta == 0 means test only once.
    unsigned delta = searchLength - matchLength;

    const UChar* searchCharacters = characters() + index;
    const UChar* matchCharacters = matchString->characters();

    // Optimization 2: keep a running hash of the strings,
    // only call memcmp if the hashes match.
    unsigned searchHash = 0;
    unsigned matchHash = 0;
    for (unsigned i = 0; i < matchLength; ++i) {
        searchHash += searchCharacters[i];
        matchHash += matchCharacters[i];
    }

    unsigned i = 0;
    // keep looping until we match
    while (searchHash != matchHash || memcmp(searchCharacters + i, matchCharacters, matchLength * sizeof(UChar))) {
        if (i == delta)
            return notFound;
        searchHash += searchCharacters[i + matchLength];
        searchHash -= searchCharacters[i];
        ++i;
    }
    return index + i;
}

size_t StringImpl::findIgnoringCase(StringImpl* matchString, unsigned index)
{
    // Check for null or empty string to match against
    if (!matchString)
        return notFound;
    unsigned matchLength = matchString->length();
    if (!matchLength)
        return min(index, length());

    // Check index & matchLength are in range.
    if (index > length())
        return notFound;
    unsigned searchLength = length() - index;
    if (matchLength > searchLength)
        return notFound;
    // delta is the number of additional times to test; delta == 0 means test only once.
    unsigned delta = searchLength - matchLength;

    const UChar* searchCharacters = characters() + index;
    const UChar* matchCharacters = matchString->characters();

    unsigned i = 0;
    // keep looping until we match
    while (!equalIgnoringCase(searchCharacters + i, matchCharacters, matchLength)) {
        if (i == delta)
            return notFound;
        ++i;
    }
    return index + i;
}

size_t StringImpl::reverseFind(UChar c, unsigned index)
{
    return WTF::reverseFind(m_data, m_length, c, index);
}

size_t StringImpl::reverseFind(StringImpl* matchString, unsigned index)
{
    // Check for null or empty string to match against
    if (!matchString)
        return notFound;
    unsigned matchLength = matchString->length();
    if (!matchLength)
        return min(index, length());

    // Optimization 1: fast case for strings of length 1.
    if (matchLength == 1)
        return WTF::reverseFind(characters(), length(), matchString->characters()[0], index);

    // Check index & matchLength are in range.
    if (matchLength > length())
        return notFound;
    // delta is the number of additional times to test; delta == 0 means test only once.
    unsigned delta = min(index, length() - matchLength);

    const UChar *searchCharacters = characters();
    const UChar *matchCharacters = matchString->characters();

    // Optimization 2: keep a running hash of the strings,
    // only call memcmp if the hashes match.
    unsigned searchHash = 0;
    unsigned matchHash = 0;
    for (unsigned i = 0; i < matchLength; ++i) {
        searchHash += searchCharacters[delta + i];
        matchHash += matchCharacters[i];
    }

    // keep looping until we match
    while (searchHash != matchHash || memcmp(searchCharacters + delta, matchCharacters, matchLength * sizeof(UChar))) {
        if (!delta)
            return notFound;
        delta--;
        searchHash -= searchCharacters[delta + matchLength];
        searchHash += searchCharacters[delta];
    }
    return delta;
}

size_t StringImpl::reverseFindIgnoringCase(StringImpl* matchString, unsigned index)
{
    // Check for null or empty string to match against
    if (!matchString)
        return notFound;
    unsigned matchLength = matchString->length();
    if (!matchLength)
        return min(index, length());

    // Check index & matchLength are in range.
    if (matchLength > length())
        return notFound;
    // delta is the number of additional times to test; delta == 0 means test only once.
    unsigned delta = min(index, length() - matchLength);
    
    const UChar *searchCharacters = characters();
    const UChar *matchCharacters = matchString->characters();

    // keep looping until we match
    while (!equalIgnoringCase(searchCharacters + delta, matchCharacters, matchLength)) {
        if (!delta)
            return notFound;
        delta--;
    }
    return delta;
}

bool StringImpl::endsWith(StringImpl* matchString, bool caseSensitive)
{
    ASSERT(matchString);
    if (m_length >= matchString->m_length) {
        unsigned start = m_length - matchString->m_length;
        return (caseSensitive ? find(matchString, start) : findIgnoringCase(matchString, start)) == start;
    }
    return false;
}

PassRefPtr<StringImpl> StringImpl::replace(UChar oldC, UChar newC)
{
    if (oldC == newC)
        return this;
    unsigned i;
    for (i = 0; i != m_length; ++i)
        if (m_data[i] == oldC)
            break;
    if (i == m_length)
        return this;

    UChar* data;
    RefPtr<StringImpl> newImpl = createUninitialized(m_length, data);

    for (i = 0; i != m_length; ++i) {
        UChar ch = m_data[i];
        if (ch == oldC)
            ch = newC;
        data[i] = ch;
    }
    return newImpl.release();
}

PassRefPtr<StringImpl> StringImpl::replace(unsigned position, unsigned lengthToReplace, StringImpl* str)
{
    position = min(position, length());
    lengthToReplace = min(lengthToReplace, length() - position);
    unsigned lengthToInsert = str ? str->length() : 0;
    if (!lengthToReplace && !lengthToInsert)
        return this;
    UChar* data;

    if ((length() - lengthToReplace) >= (numeric_limits<unsigned>::max() - lengthToInsert))
        CRASH();

    RefPtr<StringImpl> newImpl =
        createUninitialized(length() - lengthToReplace + lengthToInsert, data);
    memcpy(data, characters(), position * sizeof(UChar));
    if (str)
        memcpy(data + position, str->characters(), lengthToInsert * sizeof(UChar));
    memcpy(data + position + lengthToInsert, characters() + position + lengthToReplace,
        (length() - position - lengthToReplace) * sizeof(UChar));
    return newImpl.release();
}

PassRefPtr<StringImpl> StringImpl::replace(UChar pattern, StringImpl* replacement)
{
    if (!replacement)
        return this;
        
    unsigned repStrLength = replacement->length();
    size_t srcSegmentStart = 0;
    unsigned matchCount = 0;
    
    // Count the matches
    while ((srcSegmentStart = find(pattern, srcSegmentStart)) != notFound) {
        ++matchCount;
        ++srcSegmentStart;
    }
    
    // If we have 0 matches, we don't have to do any more work
    if (!matchCount)
        return this;
    
    if (repStrLength && matchCount > numeric_limits<unsigned>::max() / repStrLength)
        CRASH();

    unsigned replaceSize = matchCount * repStrLength;
    unsigned newSize = m_length - matchCount;
    if (newSize >= (numeric_limits<unsigned>::max() - replaceSize))
        CRASH();

    newSize += replaceSize;

    UChar* data;
    RefPtr<StringImpl> newImpl = createUninitialized(newSize, data);

    // Construct the new data
    size_t srcSegmentEnd;
    unsigned srcSegmentLength;
    srcSegmentStart = 0;
    unsigned dstOffset = 0;
    
    while ((srcSegmentEnd = find(pattern, srcSegmentStart)) != notFound) {
        srcSegmentLength = srcSegmentEnd - srcSegmentStart;
        memcpy(data + dstOffset, m_data + srcSegmentStart, srcSegmentLength * sizeof(UChar));
        dstOffset += srcSegmentLength;
        memcpy(data + dstOffset, replacement->m_data, repStrLength * sizeof(UChar));
        dstOffset += repStrLength;
        srcSegmentStart = srcSegmentEnd + 1;
    }

    srcSegmentLength = m_length - srcSegmentStart;
    memcpy(data + dstOffset, m_data + srcSegmentStart, srcSegmentLength * sizeof(UChar));

    ASSERT(dstOffset + srcSegmentLength == newImpl->length());

    return newImpl.release();
}

PassRefPtr<StringImpl> StringImpl::replace(StringImpl* pattern, StringImpl* replacement)
{
    if (!pattern || !replacement)
        return this;

    unsigned patternLength = pattern->length();
    if (!patternLength)
        return this;
        
    unsigned repStrLength = replacement->length();
    size_t srcSegmentStart = 0;
    unsigned matchCount = 0;
    
    // Count the matches
    while ((srcSegmentStart = find(pattern, srcSegmentStart)) != notFound) {
        ++matchCount;
        srcSegmentStart += patternLength;
    }
    
    // If we have 0 matches, we don't have to do any more work
    if (!matchCount)
        return this;
    
    unsigned newSize = m_length - matchCount * patternLength;
    if (repStrLength && matchCount > numeric_limits<unsigned>::max() / repStrLength)
        CRASH();

    if (newSize > (numeric_limits<unsigned>::max() - matchCount * repStrLength))
        CRASH();

    newSize += matchCount * repStrLength;

    UChar* data;
    RefPtr<StringImpl> newImpl = createUninitialized(newSize, data);
    
    // Construct the new data
    size_t srcSegmentEnd;
    unsigned srcSegmentLength;
    srcSegmentStart = 0;
    unsigned dstOffset = 0;
    
    while ((srcSegmentEnd = find(pattern, srcSegmentStart)) != notFound) {
        srcSegmentLength = srcSegmentEnd - srcSegmentStart;
        memcpy(data + dstOffset, m_data + srcSegmentStart, srcSegmentLength * sizeof(UChar));
        dstOffset += srcSegmentLength;
        memcpy(data + dstOffset, replacement->m_data, repStrLength * sizeof(UChar));
        dstOffset += repStrLength;
        srcSegmentStart = srcSegmentEnd + patternLength;
    }

    srcSegmentLength = m_length - srcSegmentStart;
    memcpy(data + dstOffset, m_data + srcSegmentStart, srcSegmentLength * sizeof(UChar));

    ASSERT(dstOffset + srcSegmentLength == newImpl->length());

    return newImpl.release();
}

bool equal(const StringImpl* a, const StringImpl* b)
{
    return StringHash::equal(a, b);
}

bool equal(const StringImpl* a, const char* b)
{
    if (!a)
        return !b;
    if (!b)
        return !a;

    unsigned length = a->length();
    const UChar* as = a->characters();
    for (unsigned i = 0; i != length; ++i) {
        unsigned char bc = b[i];
        if (!bc)
            return false;
        if (as[i] != bc)
            return false;
    }

    return !b[length];
}

bool equalIgnoringCase(StringImpl* a, StringImpl* b)
{
    return CaseFoldingHash::equal(a, b);
}

bool equalIgnoringCase(StringImpl* a, const char* b)
{
    if (!a)
        return !b;
    if (!b)
        return !a;

    unsigned length = a->length();
    const UChar* as = a->characters();

    // Do a faster loop for the case where all the characters are ASCII.
    UChar ored = 0;
    bool equal = true;
    for (unsigned i = 0; i != length; ++i) {
        char bc = b[i];
        if (!bc)
            return false;
        UChar ac = as[i];
        ored |= ac;
        equal = equal && (toASCIILower(ac) == toASCIILower(bc));
    }

    // Do a slower implementation for cases that include non-ASCII characters.
    if (ored & ~0x7F) {
        equal = true;
        for (unsigned i = 0; i != length; ++i) {
            unsigned char bc = b[i];
            equal = equal && (foldCase(as[i]) == foldCase(bc));
        }
    }

    return equal && !b[length];
}

bool equalIgnoringNullity(StringImpl* a, StringImpl* b)
{
    if (StringHash::equal(a, b))
        return true;
    if (!a && b && !b->length())
        return true;
    if (!b && a && !a->length())
        return true;

    return false;
}

WTF::Unicode::Direction StringImpl::defaultWritingDirection(bool* hasStrongDirectionality)
{
    for (unsigned i = 0; i < m_length; ++i) {
        WTF::Unicode::Direction charDirection = WTF::Unicode::direction(m_data[i]);
        if (charDirection == WTF::Unicode::LeftToRight) {
            if (hasStrongDirectionality)
                *hasStrongDirectionality = true;
            return WTF::Unicode::LeftToRight;
        }
        if (charDirection == WTF::Unicode::RightToLeft || charDirection == WTF::Unicode::RightToLeftArabic) {
            if (hasStrongDirectionality)
                *hasStrongDirectionality = true;
            return WTF::Unicode::RightToLeft;
        }
    }
    if (hasStrongDirectionality)
        *hasStrongDirectionality = false;
    return WTF::Unicode::LeftToRight;
}

// This is a hot function because it's used when parsing HTML.
PassRefPtr<StringImpl> StringImpl::createStrippingNullCharactersSlowCase(const UChar* characters, unsigned length)
{
    StringBuffer strippedCopy(length);
    unsigned strippedLength = 0;
    for (unsigned i = 0; i < length; i++) {
        if (int c = characters[i])
            strippedCopy[strippedLength++] = c;
    }
    ASSERT(strippedLength < length);  // Only take the slow case when stripping.
    strippedCopy.shrink(strippedLength);
    return adopt(strippedCopy);
}

PassRefPtr<StringImpl> StringImpl::adopt(StringBuffer& buffer)
{
    unsigned length = buffer.length();
    if (length == 0)
        return empty();
    return adoptRef(new StringImpl(buffer.release(), length));
}

PassRefPtr<StringImpl> StringImpl::createWithTerminatingNullCharacter(const StringImpl& string)
{
    // Use createUninitialized instead of 'new StringImpl' so that the string and its buffer
    // get allocated in a single memory block.
    UChar* data;
    unsigned length = string.m_length;
    if (length >= numeric_limits<unsigned>::max())
        CRASH();
    RefPtr<StringImpl> terminatedString = createUninitialized(length + 1, data);
    memcpy(data, string.m_data, length * sizeof(UChar));
    data[length] = 0;
    terminatedString->m_length--;
    terminatedString->m_hash = string.m_hash;
    terminatedString->m_refCountAndFlags |= s_refCountFlagHasTerminatingNullCharacter;
    return terminatedString.release();
}

PassRefPtr<StringImpl> StringImpl::threadsafeCopy() const
{
    return create(m_data, m_length);
}

PassRefPtr<StringImpl> StringImpl::crossThreadString()
{
    if (SharedUChar* sharedBuffer = this->sharedBuffer())
        return adoptRef(new StringImpl(m_data, m_length, sharedBuffer->crossThreadCopy()));

    // If no shared buffer is available, create a copy.
    return threadsafeCopy();
}

} // namespace WTF
