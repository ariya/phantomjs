/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 *           (C) 2001 Dirk Mueller ( mueller@kde.org )
 * Copyright (C) 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2013 Apple Inc. All rights reserved.
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
#include <wtf/ProcessID.h>
#include <wtf/StdLibExtras.h>
#include <wtf/WTFThreadData.h>
#include <wtf/unicode/CharacterNames.h>

#ifdef STRING_STATS
#include <unistd.h>
#include <wtf/DataLog.h>
#endif

using namespace std;

namespace WTF {

using namespace Unicode;

COMPILE_ASSERT(sizeof(StringImpl) == 2 * sizeof(int) + 3 * sizeof(void*), StringImpl_should_stay_small);

#ifdef STRING_STATS
StringStats StringImpl::m_stringStats;

unsigned StringStats::s_stringRemovesTillPrintStats = StringStats::s_printStringStatsFrequency;

void StringStats::removeString(StringImpl* string)
{
    unsigned length = string->length();
    bool isSubString = string->isSubString();

    --m_totalNumberStrings;

    if (string->has16BitShadow()) {
        --m_numberUpconvertedStrings;
        if (!isSubString)
            m_totalUpconvertedData -= length;
    }

    if (string->is8Bit()) {
        --m_number8BitStrings;
        if (!isSubString)
            m_total8BitData -= length;
    } else {
        --m_number16BitStrings;
        if (!isSubString)
            m_total16BitData -= length;
    }

    if (!--s_stringRemovesTillPrintStats) {
        s_stringRemovesTillPrintStats = s_printStringStatsFrequency;
        printStats();
    }
}

void StringStats::printStats()
{
    dataLogF("String stats for process id %d:\n", getCurrentProcessID());

    unsigned long long totalNumberCharacters = m_total8BitData + m_total16BitData;
    double percent8Bit = m_totalNumberStrings ? ((double)m_number8BitStrings * 100) / (double)m_totalNumberStrings : 0.0;
    double average8bitLength = m_number8BitStrings ? (double)m_total8BitData / (double)m_number8BitStrings : 0.0;
    dataLogF("%8u (%5.2f%%) 8 bit        %12llu chars  %12llu bytes  avg length %6.1f\n", m_number8BitStrings, percent8Bit, m_total8BitData, m_total8BitData, average8bitLength);

    double percent16Bit = m_totalNumberStrings ? ((double)m_number16BitStrings * 100) / (double)m_totalNumberStrings : 0.0;
    double average16bitLength = m_number16BitStrings ? (double)m_total16BitData / (double)m_number16BitStrings : 0.0;
    dataLogF("%8u (%5.2f%%) 16 bit       %12llu chars  %12llu bytes  avg length %6.1f\n", m_number16BitStrings, percent16Bit, m_total16BitData, m_total16BitData * 2, average16bitLength);

    double percentUpconverted = m_totalNumberStrings ? ((double)m_numberUpconvertedStrings * 100) / (double)m_number8BitStrings : 0.0;
    double averageUpconvertedLength = m_numberUpconvertedStrings ? (double)m_totalUpconvertedData / (double)m_numberUpconvertedStrings : 0.0;
    dataLogF("%8u (%5.2f%%) upconverted  %12llu chars  %12llu bytes  avg length %6.1f\n", m_numberUpconvertedStrings, percentUpconverted, m_totalUpconvertedData, m_totalUpconvertedData * 2, averageUpconvertedLength);

    double averageLength = m_totalNumberStrings ? (double)totalNumberCharacters / (double)m_totalNumberStrings : 0.0;
    unsigned long long totalDataBytes = m_total8BitData + (m_total16BitData + m_totalUpconvertedData) * 2;
    dataLogF("%8u Total                 %12llu chars  %12llu bytes  avg length %6.1f\n", m_totalNumberStrings, totalNumberCharacters, totalDataBytes, averageLength);
    unsigned long long totalSavedBytes = m_total8BitData - m_totalUpconvertedData;
    double percentSavings = totalSavedBytes ? ((double)totalSavedBytes * 100) / (double)(totalDataBytes + totalSavedBytes) : 0.0;
    dataLogF("         Total savings %12llu bytes (%5.2f%%)\n", totalSavedBytes, percentSavings);
}
#endif


StringImpl::~StringImpl()
{
    ASSERT(!isStatic());

    STRING_STATS_REMOVE_STRING(this);

    if (isAtomic())
        AtomicString::remove(this);
    if (isIdentifier()) {
        if (!wtfThreadData().currentIdentifierTable()->remove(this))
            CRASH();
    }

    BufferOwnership ownership = bufferOwnership();

    if (has16BitShadow()) {
        ASSERT(m_copyData16);
        fastFree(m_copyData16);
    }

    if (ownership == BufferInternal)
        return;
    if (ownership == BufferOwned) {
        // We use m_data8, but since it is a union with m_data16 this works either way.
        ASSERT(m_data8);
        fastFree(const_cast<LChar*>(m_data8));
        return;
    }
#if PLATFORM(QT)
    if (ownership == BufferAdoptedQString) {
        if (!m_qStringData->ref.deref())
            QStringData::deallocate(m_qStringData);
        return;
    }
#endif

    ASSERT(ownership == BufferSubstring);
    ASSERT(m_substringBuffer);
    m_substringBuffer->deref();
}

void StringImpl::destroy(StringImpl* stringImpl)
{
    stringImpl->~StringImpl();
    fastFree(stringImpl);
}

PassRefPtr<StringImpl> StringImpl::createFromLiteral(const char* characters, unsigned length)
{
    ASSERT_WITH_MESSAGE(length, "Use StringImpl::empty() to create an empty string");
    ASSERT(charactersAreAllASCII<LChar>(reinterpret_cast<const LChar*>(characters), length));
    return adoptRef(new StringImpl(reinterpret_cast<const LChar*>(characters), length, DoesHaveTerminatingNullCharacter, ConstructWithoutCopying));
}

PassRefPtr<StringImpl> StringImpl::createFromLiteral(const char* characters)
{
    return createFromLiteral(characters, strlen(characters));
}

PassRefPtr<StringImpl> StringImpl::createWithoutCopying(const UChar* characters, unsigned length, HasTerminatingNullCharacter hasTerminatingNullCharacter)
{
    if (!length)
        return empty();

    return adoptRef(new StringImpl(characters, length, hasTerminatingNullCharacter, ConstructWithoutCopying));
}

PassRefPtr<StringImpl> StringImpl::createWithoutCopying(const LChar* characters, unsigned length, HasTerminatingNullCharacter hasTerminatingNullCharacter)
{
    if (!length)
        return empty();

    return adoptRef(new StringImpl(characters, length, hasTerminatingNullCharacter, ConstructWithoutCopying));
}

template <typename CharType>
inline PassRefPtr<StringImpl> StringImpl::createUninitializedInternal(unsigned length, CharType*& data)
{
    if (!length) {
        data = 0;
        return empty();
    }

    // Allocate a single buffer large enough to contain the StringImpl
    // struct as well as the data which it contains. This removes one
    // heap allocation from this call.
    if (length > ((std::numeric_limits<unsigned>::max() - sizeof(StringImpl)) / sizeof(CharType)))
        CRASH();
    size_t size = sizeof(StringImpl) + length * sizeof(CharType);
    StringImpl* string = static_cast<StringImpl*>(fastMalloc(size));

    data = reinterpret_cast<CharType*>(string + 1);
    return constructInternal<CharType>(string, length);
}

PassRefPtr<StringImpl> StringImpl::createUninitialized(unsigned length, LChar*& data)
{
    return createUninitializedInternal(length, data);
}

PassRefPtr<StringImpl> StringImpl::createUninitialized(unsigned length, UChar*& data)
{
    return createUninitializedInternal(length, data);
}

template <typename CharType>
inline PassRefPtr<StringImpl> StringImpl::reallocateInternal(PassRefPtr<StringImpl> originalString, unsigned length, CharType*& data)
{   
    ASSERT(originalString->hasOneRef());
    ASSERT(originalString->bufferOwnership() == BufferInternal);

    if (!length) {
        data = 0;
        return empty();
    }

    // Same as createUninitialized() except here we use fastRealloc.
    if (length > ((std::numeric_limits<unsigned>::max() - sizeof(StringImpl)) / sizeof(CharType)))
        CRASH();
    size_t size = sizeof(StringImpl) + length * sizeof(CharType);
    originalString->~StringImpl();
    StringImpl* string = static_cast<StringImpl*>(fastRealloc(originalString.leakRef(), size));

    data = reinterpret_cast<CharType*>(string + 1);
    return constructInternal<CharType>(string, length);
}

PassRefPtr<StringImpl> StringImpl::reallocate(PassRefPtr<StringImpl> originalString, unsigned length, LChar*& data)
{
    ASSERT(originalString->is8Bit());
    return reallocateInternal(originalString, length, data);
}

PassRefPtr<StringImpl> StringImpl::reallocate(PassRefPtr<StringImpl> originalString, unsigned length, UChar*& data)
{
    ASSERT(!originalString->is8Bit());
    return reallocateInternal(originalString, length, data);
}

template <typename CharType>
inline PassRefPtr<StringImpl> StringImpl::createInternal(const CharType* characters, unsigned length)
{
    if (!characters || !length)
        return empty();

    CharType* data;
    RefPtr<StringImpl> string = createUninitialized(length, data);
    memcpy(data, characters, length * sizeof(CharType));
    return string.release();
}

PassRefPtr<StringImpl> StringImpl::create(const UChar* characters, unsigned length)
{
    return createInternal(characters, length);
}

PassRefPtr<StringImpl> StringImpl::create(const LChar* characters, unsigned length)
{
    return createInternal(characters, length);
}

PassRefPtr<StringImpl> StringImpl::create8BitIfPossible(const UChar* characters, unsigned length)
{
    if (!characters || !length)
        return empty();

    LChar* data;
    RefPtr<StringImpl> string = createUninitialized(length, data);

    for (size_t i = 0; i < length; ++i) {
        if (characters[i] & 0xff00)
            return create(characters, length);
        data[i] = static_cast<LChar>(characters[i]);
    }

    return string.release();
}

PassRefPtr<StringImpl> StringImpl::create8BitIfPossible(const UChar* string)
{
    return StringImpl::create8BitIfPossible(string, lengthOfNullTerminatedString(string));
}

PassRefPtr<StringImpl> StringImpl::create(const LChar* string)
{
    if (!string)
        return empty();
    size_t length = strlen(reinterpret_cast<const char*>(string));
    if (length > numeric_limits<unsigned>::max())
        CRASH();
    return create(string, length);
}

const UChar* StringImpl::getData16SlowCase() const
{
    if (has16BitShadow())
        return m_copyData16;

    if (bufferOwnership() == BufferSubstring) {
        // If this is a substring, return a pointer into the parent string.
        // TODO: Consider severing this string from the parent string
        unsigned offset = m_data8 - m_substringBuffer->characters8();
        return m_substringBuffer->characters() + offset;
    }

    STRING_STATS_ADD_UPCONVERTED_STRING(m_length);
    
    unsigned len = length();
    if (hasTerminatingNullCharacter())
        ++len;

    m_copyData16 = static_cast<UChar*>(fastMalloc(len * sizeof(UChar)));

    m_hashAndFlags |= s_hashFlagHas16BitShadow;

    upconvertCharacters(0, len);

    return m_copyData16;
}

void StringImpl::upconvertCharacters(unsigned start, unsigned end) const
{
    ASSERT(is8Bit());
    ASSERT(has16BitShadow());

    for (size_t i = start; i < end; ++i)
        m_copyData16[i] = m_data8[i];
}
    

bool StringImpl::containsOnlyWhitespace()
{
    // FIXME: The definition of whitespace here includes a number of characters
    // that are not whitespace from the point of view of RenderText; I wonder if
    // that's a problem in practice.
    if (is8Bit()) {
        for (unsigned i = 0; i < m_length; ++i) {
            UChar c = m_data8[i];
            if (!isASCIISpace(c))
                return false;
        }

        return true;
    }

    for (unsigned i = 0; i < m_length; ++i) {
        UChar c = m_data16[i];
        if (!isASCIISpace(c))
            return false;
    }
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
    if (is8Bit())
        return create(m_data8 + start, length);

    return create(m_data16 + start, length);
}

UChar32 StringImpl::characterStartingAt(unsigned i)
{
    if (is8Bit())
        return m_data8[i];
    if (U16_IS_SINGLE(m_data16[i]))
        return m_data16[i];
    if (i + 1 < m_length && U16_IS_LEAD(m_data16[i]) && U16_IS_TRAIL(m_data16[i + 1]))
        return U16_GET_SUPPLEMENTARY(m_data16[i], m_data16[i + 1]);
    return 0;
}

PassRefPtr<StringImpl> StringImpl::lower()
{
    // Note: This is a hot function in the Dromaeo benchmark, specifically the
    // no-op code path up through the first 'return' statement.

    // First scan the string for uppercase and non-ASCII characters:
    bool noUpper = true;
    if (is8Bit()) {
        unsigned failingIndex;
        for (unsigned i = 0; i < m_length; ++i) {
            LChar character = m_data8[i];
            if (UNLIKELY((character & ~0x7F) || isASCIIUpper(character))) {
                failingIndex = i;
                goto SlowPath8bitLower;
            }
        }
        return this;

SlowPath8bitLower:
        LChar* data8;
        RefPtr<StringImpl> newImpl = createUninitialized(m_length, data8);

        for (unsigned i = 0; i < failingIndex; ++i)
            data8[i] = m_data8[i];

        for (unsigned i = failingIndex; i < m_length; ++i) {
            LChar character = m_data8[i];
            if (!(character & ~0x7F))
                data8[i] = toASCIILower(character);
            else
                data8[i] = static_cast<LChar>(Unicode::toLower(character));
        }

        return newImpl.release();
    }
    unsigned ored = 0;

    for (unsigned i = 0; i < m_length; ++i) {
        UChar character = m_data16[i];
        if (UNLIKELY(isASCIIUpper(character)))
            noUpper = false;
        ored |= character;
    }
    // Nothing to do if the string is all ASCII with no uppercase.
    if (noUpper && !(ored & ~0x7F))
        return this;

    if (!(ored & ~0x7F)) {
        UChar* data16;
        RefPtr<StringImpl> newImpl = createUninitialized(m_length, data16);
        
        for (unsigned i = 0; i < m_length; ++i) {
            UChar c = m_data16[i];
            data16[i] = toASCIILower(c);
        }
        return newImpl.release();
    }

    if (m_length > static_cast<unsigned>(numeric_limits<int32_t>::max()))
        CRASH();
    int32_t length = m_length;

    // Do a slower implementation for cases that include non-ASCII characters.
    UChar* data16;
    RefPtr<StringImpl> newImpl = createUninitialized(m_length, data16);

    bool error;
    int32_t realLength = Unicode::toLower(data16, length, m_data16, m_length, &error);
    if (!error && realLength == length)
        return newImpl.release();

    newImpl = createUninitialized(realLength, data16);
    Unicode::toLower(data16, realLength, m_data16, m_length, &error);
    if (error)
        return this;
    return newImpl.release();
}

PassRefPtr<StringImpl> StringImpl::upper()
{
    // This function could be optimized for no-op cases the way lower() is,
    // but in empirical testing, few actual calls to upper() are no-ops, so
    // it wouldn't be worth the extra time for pre-scanning.

    if (m_length > static_cast<unsigned>(numeric_limits<int32_t>::max()))
        CRASH();
    int32_t length = m_length;

    if (is8Bit()) {
        LChar* data8;
        RefPtr<StringImpl> newImpl = createUninitialized(m_length, data8);
        
        // Do a faster loop for the case where all the characters are ASCII.
        unsigned ored = 0;
        for (int i = 0; i < length; ++i) {
            LChar c = m_data8[i];
            ored |= c;
#if CPU(X86) && defined(_MSC_VER) && _MSC_VER >=1700
            // Workaround for an MSVC 2012 x86 optimizer bug. Remove once the bug is fixed.
            // See https://connect.microsoft.com/VisualStudio/feedback/details/780362/optimization-bug-of-range-comparison
            // for more details.
            data8[i] = c >= 'a' && c <= 'z' ? c & ~0x20 : c;
#else
            data8[i] = toASCIIUpper(c);
#endif
        }
        if (!(ored & ~0x7F))
            return newImpl.release();

        // Do a slower implementation for cases that include non-ASCII Latin-1 characters.
        int numberSharpSCharacters = 0;

        // There are two special cases.
        //  1. latin-1 characters when converted to upper case are 16 bit characters.
        //  2. Lower case sharp-S converts to "SS" (two characters)
        for (int32_t i = 0; i < length; ++i) {
            LChar c = m_data8[i];
            if (UNLIKELY(c == smallLetterSharpS))
                ++numberSharpSCharacters;
            UChar upper = Unicode::toUpper(c);
            if (UNLIKELY(upper > 0xff)) {
                // Since this upper-cased character does not fit in an 8-bit string, we need to take the 16-bit path.
                goto upconvert;
            }
            data8[i] = static_cast<LChar>(upper);
        }

        if (!numberSharpSCharacters)
            return newImpl.release();

        // We have numberSSCharacters sharp-s characters, but none of the other special characters.
        newImpl = createUninitialized(m_length + numberSharpSCharacters, data8);

        LChar* dest = data8;

        for (int32_t i = 0; i < length; ++i) {
            LChar c = m_data8[i];
            if (c == smallLetterSharpS) {
                *dest++ = 'S';
                *dest++ = 'S';
            } else
                *dest++ = static_cast<LChar>(Unicode::toUpper(c));
        }

        return newImpl.release();
    }

upconvert:
    const UChar* source16 = characters();

    UChar* data16;
    RefPtr<StringImpl> newImpl = createUninitialized(m_length, data16);
    
    // Do a faster loop for the case where all the characters are ASCII.
    unsigned ored = 0;
    for (int i = 0; i < length; ++i) {
        UChar c = source16[i];
        ored |= c;
        data16[i] = toASCIIUpper(c);
    }
    if (!(ored & ~0x7F))
        return newImpl.release();

    // Do a slower implementation for cases that include non-ASCII characters.
    bool error;
    newImpl = createUninitialized(m_length, data16);
    int32_t realLength = Unicode::toUpper(data16, length, source16, m_length, &error);
    if (!error && realLength == length)
        return newImpl;
    newImpl = createUninitialized(realLength, data16);
    Unicode::toUpper(data16, realLength, source16, m_length, &error);
    if (error)
        return this;
    return newImpl.release();
}

PassRefPtr<StringImpl> StringImpl::fill(UChar character)
{
    if (!m_length)
        return this;

    if (!(character & ~0x7F)) {
        LChar* data;
        RefPtr<StringImpl> newImpl = createUninitialized(m_length, data);
        for (unsigned i = 0; i < m_length; ++i)
            data[i] = character;
        return newImpl.release();
    }
    UChar* data;
    RefPtr<StringImpl> newImpl = createUninitialized(m_length, data);
    for (unsigned i = 0; i < m_length; ++i)
        data[i] = character;
    return newImpl.release();
}

PassRefPtr<StringImpl> StringImpl::foldCase()
{
    if (m_length > static_cast<unsigned>(numeric_limits<int32_t>::max()))
        CRASH();
    int32_t length = m_length;

    if (is8Bit()) {
        // Do a faster loop for the case where all the characters are ASCII.
        LChar* data;
        RefPtr <StringImpl>newImpl = createUninitialized(m_length, data);
        LChar ored = 0;

        for (int32_t i = 0; i < length; ++i) {
            LChar c = m_data8[i];
            data[i] = toASCIILower(c);
            ored |= c;
        }

        if (!(ored & ~0x7F))
            return newImpl.release();

        // Do a slower implementation for cases that include non-ASCII Latin-1 characters.
        for (int32_t i = 0; i < length; ++i)
            data[i] = static_cast<LChar>(Unicode::toLower(m_data8[i]));

        return newImpl.release();
    }

    // Do a faster loop for the case where all the characters are ASCII.
    UChar* data;
    RefPtr<StringImpl> newImpl = createUninitialized(m_length, data);
    UChar ored = 0;
    for (int32_t i = 0; i < length; ++i) {
        UChar c = m_data16[i];
        ored |= c;
        data[i] = toASCIILower(c);
    }
    if (!(ored & ~0x7F))
        return newImpl.release();

    // Do a slower implementation for cases that include non-ASCII characters.
    bool error;
    int32_t realLength = Unicode::foldCase(data, length, m_data16, m_length, &error);
    if (!error && realLength == length)
        return newImpl.release();
    newImpl = createUninitialized(realLength, data);
    Unicode::foldCase(data, realLength, m_data16, m_length, &error);
    if (error)
        return this;
    return newImpl.release();
}

template <class UCharPredicate>
inline PassRefPtr<StringImpl> StringImpl::stripMatchedCharacters(UCharPredicate predicate)
{
    if (!m_length)
        return empty();

    unsigned start = 0;
    unsigned end = m_length - 1;
    
    // skip white space from start
    while (start <= end && predicate(is8Bit() ? m_data8[start] : m_data16[start]))
        ++start;
    
    // only white space
    if (start > end) 
        return empty();

    // skip white space from end
    while (end && predicate(is8Bit() ? m_data8[end] : m_data16[end]))
        --end;

    if (!start && end == m_length - 1)
        return this;
    if (is8Bit())
        return create(m_data8 + start, end + 1 - start);
    return create(m_data16 + start, end + 1 - start);
}

class UCharPredicate {
public:
    inline UCharPredicate(CharacterMatchFunctionPtr function): m_function(function) { }

    inline bool operator()(UChar ch) const
    {
        return m_function(ch);
    }

private:
    const CharacterMatchFunctionPtr m_function;
};

class SpaceOrNewlinePredicate {
public:
    inline bool operator()(UChar ch) const
    {
        return isSpaceOrNewline(ch);
    }
};

PassRefPtr<StringImpl> StringImpl::stripWhiteSpace()
{
    return stripMatchedCharacters(SpaceOrNewlinePredicate());
}

PassRefPtr<StringImpl> StringImpl::stripWhiteSpace(IsWhiteSpaceFunctionPtr isWhiteSpace)
{
    return stripMatchedCharacters(UCharPredicate(isWhiteSpace));
}

template <typename CharType>
ALWAYS_INLINE PassRefPtr<StringImpl> StringImpl::removeCharacters(const CharType* characters, CharacterMatchFunctionPtr findMatch)
{
    const CharType* from = characters;
    const CharType* fromend = from + m_length;
    
    // Assume the common case will not remove any characters
    while (from != fromend && !findMatch(*from))
        ++from;
    if (from == fromend)
        return this;
    
    StringBuffer<CharType> data(m_length);
    CharType* to = data.characters();
    unsigned outc = from - characters;
    
    if (outc)
        memcpy(to, characters, outc * sizeof(CharType));

    while (true) {
        while (from != fromend && findMatch(*from))
            ++from;
        while (from != fromend && !findMatch(*from))
            to[outc++] = *from++;
        if (from == fromend)
            break;
    }

    data.shrink(outc);

    return adopt(data);
}

PassRefPtr<StringImpl> StringImpl::removeCharacters(CharacterMatchFunctionPtr findMatch)
{
    if (is8Bit())
        return removeCharacters(characters8(), findMatch);
    return removeCharacters(characters16(), findMatch);
}

template <typename CharType, class UCharPredicate>
inline PassRefPtr<StringImpl> StringImpl::simplifyMatchedCharactersToSpace(UCharPredicate predicate)
{
    StringBuffer<CharType> data(m_length);

    const CharType* from = getCharacters<CharType>();
    const CharType* fromend = from + m_length;
    int outc = 0;
    bool changedToSpace = false;
    
    CharType* to = data.characters();
    
    while (true) {
        while (from != fromend && predicate(*from)) {
            if (*from != ' ')
                changedToSpace = true;
            ++from;
        }
        while (from != fromend && !predicate(*from))
            to[outc++] = *from++;
        if (from != fromend)
            to[outc++] = ' ';
        else
            break;
    }
    
    if (outc > 0 && to[outc - 1] == ' ')
        --outc;
    
    if (static_cast<unsigned>(outc) == m_length && !changedToSpace)
        return this;
    
    data.shrink(outc);
    
    return adopt(data);
}

PassRefPtr<StringImpl> StringImpl::simplifyWhiteSpace()
{
    if (is8Bit())
        return StringImpl::simplifyMatchedCharactersToSpace<LChar>(SpaceOrNewlinePredicate());
    return StringImpl::simplifyMatchedCharactersToSpace<UChar>(SpaceOrNewlinePredicate());
}

PassRefPtr<StringImpl> StringImpl::simplifyWhiteSpace(IsWhiteSpaceFunctionPtr isWhiteSpace)
{
    if (is8Bit())
        return StringImpl::simplifyMatchedCharactersToSpace<LChar>(UCharPredicate(isWhiteSpace));
    return StringImpl::simplifyMatchedCharactersToSpace<UChar>(UCharPredicate(isWhiteSpace));
}

int StringImpl::toIntStrict(bool* ok, int base)
{
    if (is8Bit())
        return charactersToIntStrict(characters8(), m_length, ok, base);
    return charactersToIntStrict(characters16(), m_length, ok, base);
}

unsigned StringImpl::toUIntStrict(bool* ok, int base)
{
    if (is8Bit())
        return charactersToUIntStrict(characters8(), m_length, ok, base);
    return charactersToUIntStrict(characters16(), m_length, ok, base);
}

int64_t StringImpl::toInt64Strict(bool* ok, int base)
{
    if (is8Bit())
        return charactersToInt64Strict(characters8(), m_length, ok, base);
    return charactersToInt64Strict(characters16(), m_length, ok, base);
}

uint64_t StringImpl::toUInt64Strict(bool* ok, int base)
{
    if (is8Bit())
        return charactersToUInt64Strict(characters8(), m_length, ok, base);
    return charactersToUInt64Strict(characters16(), m_length, ok, base);
}

intptr_t StringImpl::toIntPtrStrict(bool* ok, int base)
{
    if (is8Bit())
        return charactersToIntPtrStrict(characters8(), m_length, ok, base);
    return charactersToIntPtrStrict(characters16(), m_length, ok, base);
}

int StringImpl::toInt(bool* ok)
{
    if (is8Bit())
        return charactersToInt(characters8(), m_length, ok);
    return charactersToInt(characters16(), m_length, ok);
}

unsigned StringImpl::toUInt(bool* ok)
{
    if (is8Bit())
        return charactersToUInt(characters8(), m_length, ok);
    return charactersToUInt(characters16(), m_length, ok);
}

int64_t StringImpl::toInt64(bool* ok)
{
    if (is8Bit())
        return charactersToInt64(characters8(), m_length, ok);
    return charactersToInt64(characters16(), m_length, ok);
}

uint64_t StringImpl::toUInt64(bool* ok)
{
    if (is8Bit())
        return charactersToUInt64(characters8(), m_length, ok);
    return charactersToUInt64(characters16(), m_length, ok);
}

intptr_t StringImpl::toIntPtr(bool* ok)
{
    if (is8Bit())
        return charactersToIntPtr(characters8(), m_length, ok);
    return charactersToIntPtr(characters16(), m_length, ok);
}

double StringImpl::toDouble(bool* ok)
{
    if (is8Bit())
        return charactersToDouble(characters8(), m_length, ok);
    return charactersToDouble(characters16(), m_length, ok);
}

float StringImpl::toFloat(bool* ok)
{
    if (is8Bit())
        return charactersToFloat(characters8(), m_length, ok);
    return charactersToFloat(characters16(), m_length, ok);
}

bool equalIgnoringCase(const LChar* a, const LChar* b, unsigned length)
{
    while (length--) {
        LChar bc = *b++;
        if (foldCase(*a++) != foldCase(bc))
            return false;
    }
    return true;
}

bool equalIgnoringCase(const UChar* a, const LChar* b, unsigned length)
{
    while (length--) {
        LChar bc = *b++;
        if (foldCase(*a++) != foldCase(bc))
            return false;
    }
    return true;
}

size_t StringImpl::find(CharacterMatchFunctionPtr matchFunction, unsigned start)
{
    if (is8Bit())
        return WTF::find(characters8(), m_length, matchFunction, start);
    return WTF::find(characters16(), m_length, matchFunction, start);
}

size_t StringImpl::find(const LChar* matchString, unsigned index)
{
    // Check for null or empty string to match against
    if (!matchString)
        return notFound;
    size_t matchStringLength = strlen(reinterpret_cast<const char*>(matchString));
    if (matchStringLength > numeric_limits<unsigned>::max())
        CRASH();
    unsigned matchLength = matchStringLength;
    if (!matchLength)
        return min(index, length());

    // Optimization 1: fast case for strings of length 1.
    if (matchLength == 1)
        return WTF::find(characters16(), length(), *matchString, index);

    // Check index & matchLength are in range.
    if (index > length())
        return notFound;
    unsigned searchLength = length() - index;
    if (matchLength > searchLength)
        return notFound;
    // delta is the number of additional times to test; delta == 0 means test only once.
    unsigned delta = searchLength - matchLength;

    const UChar* searchCharacters = characters() + index;

    // Optimization 2: keep a running hash of the strings,
    // only call equal if the hashes match.
    unsigned searchHash = 0;
    unsigned matchHash = 0;
    for (unsigned i = 0; i < matchLength; ++i) {
        searchHash += searchCharacters[i];
        matchHash += matchString[i];
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

size_t StringImpl::findIgnoringCase(const LChar* matchString, unsigned index)
{
    // Check for null or empty string to match against
    if (!matchString)
        return notFound;
    size_t matchStringLength = strlen(reinterpret_cast<const char*>(matchString));
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

template <typename SearchCharacterType, typename MatchCharacterType>
ALWAYS_INLINE static size_t findInner(const SearchCharacterType* searchCharacters, const MatchCharacterType* matchCharacters, unsigned index, unsigned searchLength, unsigned matchLength)
{
    // Optimization: keep a running hash of the strings,
    // only call equal() if the hashes match.

    // delta is the number of additional times to test; delta == 0 means test only once.
    unsigned delta = searchLength - matchLength;

    unsigned searchHash = 0;
    unsigned matchHash = 0;

    for (unsigned i = 0; i < matchLength; ++i) {
        searchHash += searchCharacters[i];
        matchHash += matchCharacters[i];
    }

    unsigned i = 0;
    // keep looping until we match
    while (searchHash != matchHash || !equal(searchCharacters + i, matchCharacters, matchLength)) {
        if (i == delta)
            return notFound;
        searchHash += searchCharacters[i + matchLength];
        searchHash -= searchCharacters[i];
        ++i;
    }
    return index + i;        
}

size_t StringImpl::find(StringImpl* matchString)
{
    // Check for null string to match against
    if (UNLIKELY(!matchString))
        return notFound;
    unsigned matchLength = matchString->length();

    // Optimization 1: fast case for strings of length 1.
    if (matchLength == 1) {
        if (is8Bit()) {
            if (matchString->is8Bit())
                return WTF::find(characters8(), length(), matchString->characters8()[0]);
            return WTF::find(characters8(), length(), matchString->characters16()[0]);
        }
        if (matchString->is8Bit())
            return WTF::find(characters16(), length(), matchString->characters8()[0]);
        return WTF::find(characters16(), length(), matchString->characters16()[0]);
    }

    // Check matchLength is in range.
    if (matchLength > length())
        return notFound;

    // Check for empty string to match against
    if (UNLIKELY(!matchLength))
        return 0;

    if (is8Bit()) {
        if (matchString->is8Bit())
            return findInner(characters8(), matchString->characters8(), 0, length(), matchLength);
        return findInner(characters8(), matchString->characters16(), 0, length(), matchLength);
    }

    if (matchString->is8Bit())
        return findInner(characters16(), matchString->characters8(), 0, length(), matchLength);

    return findInner(characters16(), matchString->characters16(), 0, length(), matchLength);
}

size_t StringImpl::find(StringImpl* matchString, unsigned index)
{
    // Check for null or empty string to match against
    if (UNLIKELY(!matchString))
        return notFound;

    unsigned matchLength = matchString->length();

    // Optimization 1: fast case for strings of length 1.
    if (matchLength == 1) {
        if (is8Bit())
            return WTF::find(characters8(), length(), (*matchString)[0], index);
        return WTF::find(characters16(), length(), (*matchString)[0], index);
    }

    if (UNLIKELY(!matchLength))
        return min(index, length());

    // Check index & matchLength are in range.
    if (index > length())
        return notFound;
    unsigned searchLength = length() - index;
    if (matchLength > searchLength)
        return notFound;

    if (is8Bit()) {
        if (matchString->is8Bit())
            return findInner(characters8() + index, matchString->characters8(), index, searchLength, matchLength);
        return findInner(characters8() + index, matchString->characters16(), index, searchLength, matchLength);
    }

    if (matchString->is8Bit())
        return findInner(characters16() + index, matchString->characters8(), index, searchLength, matchLength);

    return findInner(characters16() + index, matchString->characters16(), index, searchLength, matchLength);
}

template <typename SearchCharacterType, typename MatchCharacterType>
ALWAYS_INLINE static size_t findIgnoringCaseInner(const SearchCharacterType* searchCharacters, const MatchCharacterType* matchCharacters, unsigned index, unsigned searchLength, unsigned matchLength)
{
    // delta is the number of additional times to test; delta == 0 means test only once.
    unsigned delta = searchLength - matchLength;

    unsigned i = 0;
    // keep looping until we match
    while (!equalIgnoringCase(searchCharacters + i, matchCharacters, matchLength)) {
        if (i == delta)
            return notFound;
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

    if (is8Bit()) {
        if (matchString->is8Bit())
            return findIgnoringCaseInner(characters8() + index, matchString->characters8(), index, searchLength, matchLength);
        return findIgnoringCaseInner(characters8() + index, matchString->characters16(), index, searchLength, matchLength);
    }

    if (matchString->is8Bit())
        return findIgnoringCaseInner(characters16() + index, matchString->characters8(), index, searchLength, matchLength);

    return findIgnoringCaseInner(characters16() + index, matchString->characters16(), index, searchLength, matchLength);
}

size_t StringImpl::findNextLineStart(unsigned index)
{
    if (is8Bit())
        return WTF::findNextLineStart(characters8(), m_length, index);
    return WTF::findNextLineStart(characters16(), m_length, index);
}

size_t StringImpl::reverseFind(UChar c, unsigned index)
{
    if (is8Bit())
        return WTF::reverseFind(characters8(), m_length, c, index);
    return WTF::reverseFind(characters16(), m_length, c, index);
}

template <typename SearchCharacterType, typename MatchCharacterType>
ALWAYS_INLINE static size_t reverseFindInner(const SearchCharacterType* searchCharacters, const MatchCharacterType* matchCharacters, unsigned index, unsigned length, unsigned matchLength)
{
    // Optimization: keep a running hash of the strings,
    // only call equal if the hashes match.

    // delta is the number of additional times to test; delta == 0 means test only once.
    unsigned delta = min(index, length - matchLength);
    
    unsigned searchHash = 0;
    unsigned matchHash = 0;
    for (unsigned i = 0; i < matchLength; ++i) {
        searchHash += searchCharacters[delta + i];
        matchHash += matchCharacters[i];
    }

    // keep looping until we match
    while (searchHash != matchHash || !equal(searchCharacters + delta, matchCharacters, matchLength)) {
        if (!delta)
            return notFound;
        --delta;
        searchHash -= searchCharacters[delta + matchLength];
        searchHash += searchCharacters[delta];
    }
    return delta;
}

size_t StringImpl::reverseFind(StringImpl* matchString, unsigned index)
{
    // Check for null or empty string to match against
    if (!matchString)
        return notFound;
    unsigned matchLength = matchString->length();
    unsigned ourLength = length();
    if (!matchLength)
        return min(index, ourLength);

    // Optimization 1: fast case for strings of length 1.
    if (matchLength == 1) {
        if (is8Bit())
            return WTF::reverseFind(characters8(), ourLength, (*matchString)[0], index);
        return WTF::reverseFind(characters16(), ourLength, (*matchString)[0], index);
    }

    // Check index & matchLength are in range.
    if (matchLength > ourLength)
        return notFound;

    if (is8Bit()) {
        if (matchString->is8Bit())
            return reverseFindInner(characters8(), matchString->characters8(), index, ourLength, matchLength);
        return reverseFindInner(characters8(), matchString->characters16(), index, ourLength, matchLength);
    }
    
    if (matchString->is8Bit())
        return reverseFindInner(characters16(), matchString->characters8(), index, ourLength, matchLength);

    return reverseFindInner(characters16(), matchString->characters16(), index, ourLength, matchLength);
}

template <typename SearchCharacterType, typename MatchCharacterType>
ALWAYS_INLINE static size_t reverseFindIgnoringCaseInner(const SearchCharacterType* searchCharacters, const MatchCharacterType* matchCharacters, unsigned index, unsigned length, unsigned matchLength)
{
    // delta is the number of additional times to test; delta == 0 means test only once.
    unsigned delta = min(index, length - matchLength);

    // keep looping until we match
    while (!equalIgnoringCase(searchCharacters + delta, matchCharacters, matchLength)) {
        if (!delta)
            return notFound;
        --delta;
    }
    return delta;
}

size_t StringImpl::reverseFindIgnoringCase(StringImpl* matchString, unsigned index)
{
    // Check for null or empty string to match against
    if (!matchString)
        return notFound;
    unsigned matchLength = matchString->length();
    unsigned ourLength = length();
    if (!matchLength)
        return min(index, ourLength);

    // Check index & matchLength are in range.
    if (matchLength > ourLength)
        return notFound;

    if (is8Bit()) {
        if (matchString->is8Bit())
            return reverseFindIgnoringCaseInner(characters8(), matchString->characters8(), index, ourLength, matchLength);
        return reverseFindIgnoringCaseInner(characters8(), matchString->characters16(), index, ourLength, matchLength);
    }

    if (matchString->is8Bit())
        return reverseFindIgnoringCaseInner(characters16(), matchString->characters8(), index, ourLength, matchLength);

    return reverseFindIgnoringCaseInner(characters16(), matchString->characters16(), index, ourLength, matchLength);
}

ALWAYS_INLINE static bool equalInner(const StringImpl* stringImpl, unsigned startOffset, const char* matchString, unsigned matchLength, bool caseSensitive)
{
    ASSERT(stringImpl);
    ASSERT(matchLength <= stringImpl->length());
    ASSERT(startOffset + matchLength <= stringImpl->length());

    if (caseSensitive) {
        if (stringImpl->is8Bit())
            return equal(stringImpl->characters8() + startOffset, reinterpret_cast<const LChar*>(matchString), matchLength);
        return equal(stringImpl->characters16() + startOffset, reinterpret_cast<const LChar*>(matchString), matchLength);
    }
    if (stringImpl->is8Bit())
        return equalIgnoringCase(stringImpl->characters8() + startOffset, reinterpret_cast<const LChar*>(matchString), matchLength);
    return equalIgnoringCase(stringImpl->characters16() + startOffset, reinterpret_cast<const LChar*>(matchString), matchLength);
}

bool StringImpl::startsWith(const StringImpl* str) const
{
    if (!str)
        return false;

    if (str->length() > length())
        return false;

    if (is8Bit()) {
        if (str->is8Bit())
            return equal(characters8(), str->characters8(), str->length());
        return equal(characters8(), str->characters16(), str->length());
    }
    if (str->is8Bit())
        return equal(characters16(), str->characters8(), str->length());
    return equal(characters16(), str->characters16(), str->length());
}

bool StringImpl::startsWith(UChar character) const
{
    return m_length && (*this)[0] == character;
}

bool StringImpl::startsWith(const char* matchString, unsigned matchLength, bool caseSensitive) const
{
    ASSERT(matchLength);
    if (matchLength > length())
        return false;
    return equalInner(this, 0, matchString, matchLength, caseSensitive);
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

bool StringImpl::endsWith(UChar character) const
{
    return m_length && (*this)[m_length - 1] == character;
}

bool StringImpl::endsWith(const char* matchString, unsigned matchLength, bool caseSensitive) const
{
    ASSERT(matchLength);
    if (matchLength > length())
        return false;
    unsigned startOffset = length() - matchLength;
    return equalInner(this, startOffset, matchString, matchLength, caseSensitive);
}

PassRefPtr<StringImpl> StringImpl::replace(UChar oldC, UChar newC)
{
    if (oldC == newC)
        return this;
    unsigned i;
    for (i = 0; i != m_length; ++i) {
        UChar c = is8Bit() ? m_data8[i] : m_data16[i];
        if (c == oldC)
            break;
    }
    if (i == m_length)
        return this;

    if (is8Bit()) {
        if (oldC > 0xff)
            // Looking for a 16 bit char in an 8 bit string, we're done.
            return this;

        if (newC <= 0xff) {
            LChar* data;
            LChar oldChar = static_cast<LChar>(oldC);
            LChar newChar = static_cast<LChar>(newC);

            RefPtr<StringImpl> newImpl = createUninitialized(m_length, data);

            for (i = 0; i != m_length; ++i) {
                LChar ch = m_data8[i];
                if (ch == oldChar)
                    ch = newChar;
                data[i] = ch;
            }
            return newImpl.release();
        }

        // There is the possibility we need to up convert from 8 to 16 bit,
        // create a 16 bit string for the result.
        UChar* data;
        RefPtr<StringImpl> newImpl = createUninitialized(m_length, data);

        for (i = 0; i != m_length; ++i) {
            UChar ch = m_data8[i];
            if (ch == oldC)
                ch = newC;
            data[i] = ch;
        }

        return newImpl.release();
    }

    UChar* data;
    RefPtr<StringImpl> newImpl = createUninitialized(m_length, data);

    for (i = 0; i != m_length; ++i) {
        UChar ch = m_data16[i];
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

    if ((length() - lengthToReplace) >= (numeric_limits<unsigned>::max() - lengthToInsert))
        CRASH();

    if (is8Bit() && (!str || str->is8Bit())) {
        LChar* data;
        RefPtr<StringImpl> newImpl =
        createUninitialized(length() - lengthToReplace + lengthToInsert, data);
        memcpy(data, m_data8, position * sizeof(LChar));
        if (str)
            memcpy(data + position, str->m_data8, lengthToInsert * sizeof(LChar));
        memcpy(data + position + lengthToInsert, m_data8 + position + lengthToReplace,
               (length() - position - lengthToReplace) * sizeof(LChar));
        return newImpl.release();
    }
    UChar* data;
    RefPtr<StringImpl> newImpl =
        createUninitialized(length() - lengthToReplace + lengthToInsert, data);
    if (is8Bit())
        for (unsigned i = 0; i < position; ++i)
            data[i] = m_data8[i];
    else
        memcpy(data, m_data16, position * sizeof(UChar));
    if (str) {
        if (str->is8Bit())
            for (unsigned i = 0; i < lengthToInsert; ++i)
                data[i + position] = str->m_data8[i];
        else
            memcpy(data + position, str->m_data16, lengthToInsert * sizeof(UChar));
    }
    if (is8Bit()) {
        for (unsigned i = 0; i < length() - position - lengthToReplace; ++i)
            data[i + position + lengthToInsert] = m_data8[i + position + lengthToReplace];
    } else {
        memcpy(data + position + lengthToInsert, characters() + position + lengthToReplace,
            (length() - position - lengthToReplace) * sizeof(UChar));
    }
    return newImpl.release();
}

PassRefPtr<StringImpl> StringImpl::replace(UChar pattern, StringImpl* replacement)
{
    if (!replacement)
        return this;

    if (replacement->is8Bit())
        return replace(pattern, replacement->m_data8, replacement->length());

    return replace(pattern, replacement->m_data16, replacement->length());
}

PassRefPtr<StringImpl> StringImpl::replace(UChar pattern, const LChar* replacement, unsigned repStrLength)
{
    ASSERT(replacement);

    size_t srcSegmentStart = 0;
    unsigned matchCount = 0;

    // Count the matches.
    while ((srcSegmentStart = find(pattern, srcSegmentStart)) != notFound) {
        ++matchCount;
        ++srcSegmentStart;
    }

    // If we have 0 matches then we don't have to do any more work.
    if (!matchCount)
        return this;

    if (repStrLength && matchCount > numeric_limits<unsigned>::max() / repStrLength)
        CRASH();

    unsigned replaceSize = matchCount * repStrLength;
    unsigned newSize = m_length - matchCount;
    if (newSize >= (numeric_limits<unsigned>::max() - replaceSize))
        CRASH();

    newSize += replaceSize;

    // Construct the new data.
    size_t srcSegmentEnd;
    unsigned srcSegmentLength;
    srcSegmentStart = 0;
    unsigned dstOffset = 0;

    if (is8Bit()) {
        LChar* data;
        RefPtr<StringImpl> newImpl = createUninitialized(newSize, data);

        while ((srcSegmentEnd = find(pattern, srcSegmentStart)) != notFound) {
            srcSegmentLength = srcSegmentEnd - srcSegmentStart;
            memcpy(data + dstOffset, m_data8 + srcSegmentStart, srcSegmentLength * sizeof(LChar));
            dstOffset += srcSegmentLength;
            memcpy(data + dstOffset, replacement, repStrLength * sizeof(LChar));
            dstOffset += repStrLength;
            srcSegmentStart = srcSegmentEnd + 1;
        }

        srcSegmentLength = m_length - srcSegmentStart;
        memcpy(data + dstOffset, m_data8 + srcSegmentStart, srcSegmentLength * sizeof(LChar));

        ASSERT(dstOffset + srcSegmentLength == newImpl->length());

        return newImpl.release();
    }

    UChar* data;
    RefPtr<StringImpl> newImpl = createUninitialized(newSize, data);

    while ((srcSegmentEnd = find(pattern, srcSegmentStart)) != notFound) {
        srcSegmentLength = srcSegmentEnd - srcSegmentStart;
        memcpy(data + dstOffset, m_data16 + srcSegmentStart, srcSegmentLength * sizeof(UChar));

        dstOffset += srcSegmentLength;
        for (unsigned i = 0; i < repStrLength; ++i)
            data[i + dstOffset] = replacement[i];

        dstOffset += repStrLength;
        srcSegmentStart = srcSegmentEnd + 1;
    }

    srcSegmentLength = m_length - srcSegmentStart;
    memcpy(data + dstOffset, m_data16 + srcSegmentStart, srcSegmentLength * sizeof(UChar));

    ASSERT(dstOffset + srcSegmentLength == newImpl->length());

    return newImpl.release();
}

PassRefPtr<StringImpl> StringImpl::replace(UChar pattern, const UChar* replacement, unsigned repStrLength)
{
    ASSERT(replacement);

    size_t srcSegmentStart = 0;
    unsigned matchCount = 0;

    // Count the matches.
    while ((srcSegmentStart = find(pattern, srcSegmentStart)) != notFound) {
        ++matchCount;
        ++srcSegmentStart;
    }

    // If we have 0 matches then we don't have to do any more work.
    if (!matchCount)
        return this;

    if (repStrLength && matchCount > numeric_limits<unsigned>::max() / repStrLength)
        CRASH();

    unsigned replaceSize = matchCount * repStrLength;
    unsigned newSize = m_length - matchCount;
    if (newSize >= (numeric_limits<unsigned>::max() - replaceSize))
        CRASH();

    newSize += replaceSize;

    // Construct the new data.
    size_t srcSegmentEnd;
    unsigned srcSegmentLength;
    srcSegmentStart = 0;
    unsigned dstOffset = 0;

    if (is8Bit()) {
        UChar* data;
        RefPtr<StringImpl> newImpl = createUninitialized(newSize, data);

        while ((srcSegmentEnd = find(pattern, srcSegmentStart)) != notFound) {
            srcSegmentLength = srcSegmentEnd - srcSegmentStart;
            for (unsigned i = 0; i < srcSegmentLength; ++i)
                data[i + dstOffset] = m_data8[i + srcSegmentStart];

            dstOffset += srcSegmentLength;
            memcpy(data + dstOffset, replacement, repStrLength * sizeof(UChar));

            dstOffset += repStrLength;
            srcSegmentStart = srcSegmentEnd + 1;
        }

        srcSegmentLength = m_length - srcSegmentStart;
        for (unsigned i = 0; i < srcSegmentLength; ++i)
            data[i + dstOffset] = m_data8[i + srcSegmentStart];

        ASSERT(dstOffset + srcSegmentLength == newImpl->length());

        return newImpl.release();
    }

    UChar* data;
    RefPtr<StringImpl> newImpl = createUninitialized(newSize, data);

    while ((srcSegmentEnd = find(pattern, srcSegmentStart)) != notFound) {
        srcSegmentLength = srcSegmentEnd - srcSegmentStart;
        memcpy(data + dstOffset, m_data16 + srcSegmentStart, srcSegmentLength * sizeof(UChar));

        dstOffset += srcSegmentLength;
        memcpy(data + dstOffset, replacement, repStrLength * sizeof(UChar));

        dstOffset += repStrLength;
        srcSegmentStart = srcSegmentEnd + 1;
    }

    srcSegmentLength = m_length - srcSegmentStart;
    memcpy(data + dstOffset, m_data16 + srcSegmentStart, srcSegmentLength * sizeof(UChar));

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
    
    // Count the matches.
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

    
    // Construct the new data
    size_t srcSegmentEnd;
    unsigned srcSegmentLength;
    srcSegmentStart = 0;
    unsigned dstOffset = 0;
    bool srcIs8Bit = is8Bit();
    bool replacementIs8Bit = replacement->is8Bit();
    
    // There are 4 cases:
    // 1. This and replacement are both 8 bit.
    // 2. This and replacement are both 16 bit.
    // 3. This is 8 bit and replacement is 16 bit.
    // 4. This is 16 bit and replacement is 8 bit.
    if (srcIs8Bit && replacementIs8Bit) {
        // Case 1
        LChar* data;
        RefPtr<StringImpl> newImpl = createUninitialized(newSize, data);
        while ((srcSegmentEnd = find(pattern, srcSegmentStart)) != notFound) {
            srcSegmentLength = srcSegmentEnd - srcSegmentStart;
            memcpy(data + dstOffset, m_data8 + srcSegmentStart, srcSegmentLength * sizeof(LChar));
            dstOffset += srcSegmentLength;
            memcpy(data + dstOffset, replacement->m_data8, repStrLength * sizeof(LChar));
            dstOffset += repStrLength;
            srcSegmentStart = srcSegmentEnd + patternLength;
        }

        srcSegmentLength = m_length - srcSegmentStart;
        memcpy(data + dstOffset, m_data8 + srcSegmentStart, srcSegmentLength * sizeof(LChar));

        ASSERT(dstOffset + srcSegmentLength == newImpl->length());

        return newImpl.release();
    }

    UChar* data;
    RefPtr<StringImpl> newImpl = createUninitialized(newSize, data);
    while ((srcSegmentEnd = find(pattern, srcSegmentStart)) != notFound) {
        srcSegmentLength = srcSegmentEnd - srcSegmentStart;
        if (srcIs8Bit) {
            // Case 3.
            for (unsigned i = 0; i < srcSegmentLength; ++i)
                data[i + dstOffset] = m_data8[i + srcSegmentStart];
        } else {
            // Case 2 & 4.
            memcpy(data + dstOffset, m_data16 + srcSegmentStart, srcSegmentLength * sizeof(UChar));
        }
        dstOffset += srcSegmentLength;
        if (replacementIs8Bit) {
            // Cases 2 & 3.
            for (unsigned i = 0; i < repStrLength; ++i)
                data[i + dstOffset] = replacement->m_data8[i];
        } else {
            // Case 4
            memcpy(data + dstOffset, replacement->m_data16, repStrLength * sizeof(UChar));
        }
        dstOffset += repStrLength;
        srcSegmentStart = srcSegmentEnd + patternLength;
    }

    srcSegmentLength = m_length - srcSegmentStart;
    if (srcIs8Bit) {
        // Case 3.
        for (unsigned i = 0; i < srcSegmentLength; ++i)
            data[i + dstOffset] = m_data8[i + srcSegmentStart];
    } else {
        // Cases 2 & 4.
        memcpy(data + dstOffset, m_data16 + srcSegmentStart, srcSegmentLength * sizeof(UChar));
    }

    ASSERT(dstOffset + srcSegmentLength == newImpl->length());

    return newImpl.release();
}

static inline bool stringImplContentEqual(const StringImpl* a, const StringImpl* b)
{
    unsigned aLength = a->length();
    unsigned bLength = b->length();
    if (aLength != bLength)
        return false;

    if (a->is8Bit()) {
        if (b->is8Bit())
            return equal(a->characters8(), b->characters8(), aLength);

        return equal(a->characters8(), b->characters16(), aLength);
    }

    if (b->is8Bit())
        return equal(a->characters16(), b->characters8(), aLength);

    return equal(a->characters16(), b->characters16(), aLength);
}

bool equal(const StringImpl* a, const StringImpl* b)
{
    if (a == b)
        return true;
    if (!a || !b)
        return false;

    return stringImplContentEqual(a, b);
}

template <typename CharType>
inline bool equalInternal(const StringImpl* a, const CharType* b, unsigned length)
{
    if (!a)
        return !b;
    if (!b)
        return false;

    if (a->length() != length)
        return false;
    if (a->is8Bit())
        return equal(a->characters8(), b, length);
    return equal(a->characters16(), b, length);
}

bool equal(const StringImpl* a, const LChar* b, unsigned length)
{
    return equalInternal(a, b, length);
}

bool equal(const StringImpl* a, const UChar* b, unsigned length)
{
    return equalInternal(a, b, length);
}

bool equal(const StringImpl* a, const LChar* b)
{
    if (!a)
        return !b;
    if (!b)
        return !a;

    unsigned length = a->length();

    if (a->is8Bit()) {
        const LChar* aPtr = a->characters8();
        for (unsigned i = 0; i != length; ++i) {
            LChar bc = b[i];
            LChar ac = aPtr[i];
            if (!bc)
                return false;
            if (ac != bc)
                return false;
        }

        return !b[length];
    }

    const UChar* aPtr = a->characters16();
    for (unsigned i = 0; i != length; ++i) {
        LChar bc = b[i];
        if (!bc)
            return false;
        if (aPtr[i] != bc)
            return false;
    }

    return !b[length];
}

bool equalNonNull(const StringImpl* a, const StringImpl* b)
{
    ASSERT(a && b);
    if (a == b)
        return true;

    return stringImplContentEqual(a, b);
}

bool equalIgnoringCase(const StringImpl* a, const StringImpl* b)
{
    if (a == b)
        return true;
    if (!a || !b)
        return false;

    return CaseFoldingHash::equal(a, b);
}

bool equalIgnoringCase(const StringImpl* a, const LChar* b)
{
    if (!a)
        return !b;
    if (!b)
        return !a;

    unsigned length = a->length();

    // Do a faster loop for the case where all the characters are ASCII.
    UChar ored = 0;
    bool equal = true;
    if (a->is8Bit()) {
        const LChar* as = a->characters8();
        for (unsigned i = 0; i != length; ++i) {
            LChar bc = b[i];
            if (!bc)
                return false;
            UChar ac = as[i];
            ored |= ac;
            equal = equal && (toASCIILower(ac) == toASCIILower(bc));
        }
        
        // Do a slower implementation for cases that include non-ASCII characters.
        if (ored & ~0x7F) {
            equal = true;
            for (unsigned i = 0; i != length; ++i)
                equal = equal && (foldCase(as[i]) == foldCase(b[i]));
        }
        
        return equal && !b[length];        
    }

    const UChar* as = a->characters16();
    for (unsigned i = 0; i != length; ++i) {
        LChar bc = b[i];
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
            equal = equal && (foldCase(as[i]) == foldCase(b[i]));
        }
    }

    return equal && !b[length];
}

bool equalIgnoringCaseNonNull(const StringImpl* a, const StringImpl* b)
{
    ASSERT(a && b);
    if (a == b)
        return true;

    unsigned length = a->length();
    if (length != b->length())
        return false;

    if (a->is8Bit()) {
        if (b->is8Bit())
            return equalIgnoringCase(a->characters8(), b->characters8(), length);

        return equalIgnoringCase(b->characters16(), a->characters8(), length);
    }

    if (b->is8Bit())
        return equalIgnoringCase(a->characters16(), b->characters8(), length);

    return equalIgnoringCase(a->characters16(), b->characters16(), length);
}

bool equalIgnoringNullity(StringImpl* a, StringImpl* b)
{
    if (!a && b && !b->length())
        return true;
    if (!b && a && !a->length())
        return true;
    return equal(a, b);
}

WTF::Unicode::Direction StringImpl::defaultWritingDirection(bool* hasStrongDirectionality)
{
    for (unsigned i = 0; i < m_length; ++i) {
        WTF::Unicode::Direction charDirection = WTF::Unicode::direction(is8Bit() ? m_data8[i] : m_data16[i]);
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

PassRefPtr<StringImpl> StringImpl::adopt(StringBuffer<LChar>& buffer)
{
unsigned length = buffer.length();
if (!length)
    return empty();
return adoptRef(new StringImpl(buffer.release(), length));
}

PassRefPtr<StringImpl> StringImpl::adopt(StringBuffer<UChar>& buffer)
{
    unsigned length = buffer.length();
    if (!length)
        return empty();
    return adoptRef(new StringImpl(buffer.release(), length));
}

#if PLATFORM(QT)
PassRefPtr<StringImpl> StringImpl::adopt(QStringData* qStringData)
{
    ASSERT(qStringData);

    if (!qStringData->size)
        return empty();

    return adoptRef(new StringImpl(qStringData, ConstructAdoptedQString));
}
#endif

PassRefPtr<StringImpl> StringImpl::createWithTerminatingNullCharacter(const StringImpl& string)
{
    // Use createUninitialized instead of 'new StringImpl' so that the string and its buffer
    // get allocated in a single memory block.
    unsigned length = string.m_length;
    if (length >= numeric_limits<unsigned>::max())
        CRASH();
    RefPtr<StringImpl> terminatedString;
    if (string.is8Bit()) {
        LChar* data;
        terminatedString = createUninitialized(length + 1, data);
        memcpy(data, string.m_data8, length * sizeof(LChar));
        data[length] = 0;
    } else {
        UChar* data;
        terminatedString = createUninitialized(length + 1, data);
        memcpy(data, string.m_data16, length * sizeof(UChar));
        data[length] = 0;
    }
    --(terminatedString->m_length);
    terminatedString->m_hashAndFlags = (string.m_hashAndFlags & (~s_flagMask | s_hashFlag8BitBuffer)) | s_hashFlagHasTerminatingNullCharacter;
    return terminatedString.release();
}

size_t StringImpl::sizeInBytes() const
{
    // FIXME: support substrings
    size_t size = length();
    if (is8Bit()) {
        if (has16BitShadow()) {
            size += 2 * size;
            if (hasTerminatingNullCharacter())
                size += 2;
        }
    } else
        size *= 2;
    return size + sizeof(*this);
}

} // namespace WTF
