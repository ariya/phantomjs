/*
 * (C) 1999 Lars Knoll (knoll@kde.org)
 * Copyright (C) 2004, 2005, 2006, 2007, 2008, 2009, 2010, 2012, 2013 Apple Inc. All rights reserved.
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

#ifndef WTFString_h
#define WTFString_h

// This file would be called String.h, but that conflicts with <string.h>
// on systems without case-sensitive file systems.

#include <wtf/text/ASCIIFastPath.h>
#include <wtf/text/StringImpl.h>

#ifdef __OBJC__
#include <objc/objc.h>
#endif

#if PLATFORM(QT)
QT_BEGIN_NAMESPACE
class QString;
QT_END_NAMESPACE
#endif

#if PLATFORM(BLACKBERRY)
namespace BlackBerry {
namespace Platform {
class String;
}
}
#endif

namespace WTF {

class CString;
struct StringHash;

// Declarations of string operations

WTF_EXPORT_STRING_API int charactersToIntStrict(const LChar*, size_t, bool* ok = 0, int base = 10);
WTF_EXPORT_STRING_API int charactersToIntStrict(const UChar*, size_t, bool* ok = 0, int base = 10);
WTF_EXPORT_STRING_API unsigned charactersToUIntStrict(const LChar*, size_t, bool* ok = 0, int base = 10);
WTF_EXPORT_STRING_API unsigned charactersToUIntStrict(const UChar*, size_t, bool* ok = 0, int base = 10);
int64_t charactersToInt64Strict(const LChar*, size_t, bool* ok = 0, int base = 10);
int64_t charactersToInt64Strict(const UChar*, size_t, bool* ok = 0, int base = 10);
uint64_t charactersToUInt64Strict(const LChar*, size_t, bool* ok = 0, int base = 10);
uint64_t charactersToUInt64Strict(const UChar*, size_t, bool* ok = 0, int base = 10);
intptr_t charactersToIntPtrStrict(const LChar*, size_t, bool* ok = 0, int base = 10);
intptr_t charactersToIntPtrStrict(const UChar*, size_t, bool* ok = 0, int base = 10);

int charactersToInt(const LChar*, size_t, bool* ok = 0); // ignores trailing garbage
WTF_EXPORT_STRING_API int charactersToInt(const UChar*, size_t, bool* ok = 0); // ignores trailing garbage
unsigned charactersToUInt(const LChar*, size_t, bool* ok = 0); // ignores trailing garbage
unsigned charactersToUInt(const UChar*, size_t, bool* ok = 0); // ignores trailing garbage
int64_t charactersToInt64(const LChar*, size_t, bool* ok = 0); // ignores trailing garbage
int64_t charactersToInt64(const UChar*, size_t, bool* ok = 0); // ignores trailing garbage
uint64_t charactersToUInt64(const LChar*, size_t, bool* ok = 0); // ignores trailing garbage
uint64_t charactersToUInt64(const UChar*, size_t, bool* ok = 0); // ignores trailing garbage
intptr_t charactersToIntPtr(const LChar*, size_t, bool* ok = 0); // ignores trailing garbage
intptr_t charactersToIntPtr(const UChar*, size_t, bool* ok = 0); // ignores trailing garbage

// FIXME: Like the strict functions above, these give false for "ok" when there is trailing garbage.
// Like the non-strict functions above, these return the value when there is trailing garbage.
// It would be better if these were more consistent with the above functions instead.
WTF_EXPORT_STRING_API double charactersToDouble(const LChar*, size_t, bool* ok = 0);
WTF_EXPORT_STRING_API double charactersToDouble(const UChar*, size_t, bool* ok = 0);
float charactersToFloat(const LChar*, size_t, bool* ok = 0);
WTF_EXPORT_STRING_API float charactersToFloat(const UChar*, size_t, bool* ok = 0);
WTF_EXPORT_STRING_API float charactersToFloat(const LChar*, size_t, size_t& parsedLength);
WTF_EXPORT_STRING_API float charactersToFloat(const UChar*, size_t, size_t& parsedLength);

class ASCIILiteral;

enum TrailingZerosTruncatingPolicy {
    KeepTrailingZeros,
    TruncateTrailingZeros
};

template<bool isSpecialCharacter(UChar), typename CharacterType>
bool isAllSpecialCharacters(const CharacterType*, size_t);

class String {
public:
    // Construct a null string, distinguishable from an empty string.
    String() { }

    // Construct a string with UTF-16 data.
    WTF_EXPORT_STRING_API String(const UChar* characters, unsigned length);

    // Construct a string by copying the contents of a vector.  To avoid
    // copying, consider using String::adopt instead.
    // This method will never create a null string. Vectors with size() == 0
    // will return the empty string.
    // NOTE: This is different from String(vector.data(), vector.size())
    // which will sometimes return a null string when vector.data() is null
    // which can only occur for vectors without inline capacity.
    // See: https://bugs.webkit.org/show_bug.cgi?id=109792
    template<size_t inlineCapacity, typename OverflowHandler>
    explicit String(const Vector<UChar, inlineCapacity, OverflowHandler>&);

    // Construct a string with UTF-16 data, from a null-terminated source.
    WTF_EXPORT_STRING_API String(const UChar*);

    // Construct a string with latin1 data.
    WTF_EXPORT_STRING_API String(const LChar* characters, unsigned length);
    WTF_EXPORT_STRING_API String(const char* characters, unsigned length);

    // Construct a string with latin1 data, from a null-terminated source.
    WTF_EXPORT_STRING_API String(const LChar* characters);
    WTF_EXPORT_STRING_API String(const char* characters);

    // Construct a string referencing an existing StringImpl.
    String(StringImpl* impl) : m_impl(impl) { }
    String(PassRefPtr<StringImpl> impl) : m_impl(impl) { }
    String(RefPtr<StringImpl> impl) : m_impl(impl) { }

    // Construct a string from a constant string literal.
    WTF_EXPORT_STRING_API String(ASCIILiteral characters);

    // Construct a string from a constant string literal.
    // This constructor is the "big" version, as it put the length in the function call and generate bigger code.
    enum ConstructFromLiteralTag { ConstructFromLiteral };
    template<unsigned charactersCount>
    String(const char (&characters)[charactersCount], ConstructFromLiteralTag) : m_impl(StringImpl::createFromLiteral<charactersCount>(characters)) { }

#if COMPILER_SUPPORTS(CXX_RVALUE_REFERENCES)
    // We have to declare the copy constructor and copy assignment operator as well, otherwise
    // they'll be implicitly deleted by adding the move constructor and move assignment operator.
    String(const String& other) : m_impl(other.m_impl) { }
    String(String&& other) : m_impl(other.m_impl.release()) { }
    String& operator=(const String& other) { m_impl = other.m_impl; return *this; }
    String& operator=(String&& other) { m_impl = other.m_impl.release(); return *this; }
#endif

    // Inline the destructor.
    ALWAYS_INLINE ~String() { }

    void swap(String& o) { m_impl.swap(o.m_impl); }

    static String adopt(StringBuffer<LChar>& buffer) { return StringImpl::adopt(buffer); }
    static String adopt(StringBuffer<UChar>& buffer) { return StringImpl::adopt(buffer); }
    template<typename CharacterType, size_t inlineCapacity, typename OverflowHandler>
    static String adopt(Vector<CharacterType, inlineCapacity, OverflowHandler>& vector) { return StringImpl::adopt(vector); }

    bool isNull() const { return !m_impl; }
    bool isEmpty() const { return !m_impl || !m_impl->length(); }

    StringImpl* impl() const { return m_impl.get(); }
    PassRefPtr<StringImpl> releaseImpl() { return m_impl.release(); }

    unsigned length() const
    {
        if (!m_impl)
            return 0;
        return m_impl->length();
    }

    const UChar* characters() const
    {
        if (!m_impl)
            return 0;
        return m_impl->characters();
    }
    
    const LChar* characters8() const
    {
        if (!m_impl)
            return 0;
        ASSERT(m_impl->is8Bit());
        return m_impl->characters8();
    }

    const UChar* characters16() const
    {
        if (!m_impl)
            return 0;
        ASSERT(!m_impl->is8Bit());
        return m_impl->characters16();
    }

    // Return characters8() or characters16() depending on CharacterType.
    template <typename CharacterType>
    inline const CharacterType* getCharacters() const;

    // Like getCharacters() and upconvert if CharacterType is UChar on a 8bit string.
    template <typename CharacterType>
    inline const CharacterType* getCharactersWithUpconvert() const;

    bool is8Bit() const { return m_impl->is8Bit(); }

    unsigned sizeInBytes() const
    {
        if (!m_impl)
            return 0;
        return m_impl->length() * (is8Bit() ? sizeof(LChar) : sizeof(UChar));
    }

    WTF_EXPORT_STRING_API CString ascii() const;
    WTF_EXPORT_STRING_API CString latin1() const;

    typedef enum {
        LenientConversion,
        StrictConversion,
        StrictConversionReplacingUnpairedSurrogatesWithFFFD,
    } ConversionMode;

    WTF_EXPORT_STRING_API CString utf8(ConversionMode = LenientConversion) const;

    UChar operator[](unsigned index) const
    {
        if (!m_impl || index >= m_impl->length())
            return 0;
        return (*m_impl)[index];
    }

    WTF_EXPORT_STRING_API static String number(int);
    WTF_EXPORT_STRING_API static String number(unsigned int);
    WTF_EXPORT_STRING_API static String number(long);
    WTF_EXPORT_STRING_API static String number(unsigned long);
    WTF_EXPORT_STRING_API static String number(long long);
    WTF_EXPORT_STRING_API static String number(unsigned long long);

    WTF_EXPORT_STRING_API static String number(double, unsigned precision = 6, TrailingZerosTruncatingPolicy = TruncateTrailingZeros);

    // Number to String conversion following the ECMAScript definition.
    WTF_EXPORT_STRING_API static String numberToStringECMAScript(double);
    WTF_EXPORT_STRING_API static String numberToStringFixedWidth(double, unsigned decimalPlaces);

    // Find a single character or string, also with match function & latin1 forms.
    size_t find(UChar c, unsigned start = 0) const
        { return m_impl ? m_impl->find(c, start) : notFound; }

    size_t find(const String& str) const
        { return m_impl ? m_impl->find(str.impl()) : notFound; }
    size_t find(const String& str, unsigned start) const
        { return m_impl ? m_impl->find(str.impl(), start) : notFound; }

    size_t find(CharacterMatchFunctionPtr matchFunction, unsigned start = 0) const
        { return m_impl ? m_impl->find(matchFunction, start) : notFound; }
    size_t find(const LChar* str, unsigned start = 0) const
        { return m_impl ? m_impl->find(str, start) : notFound; }

    size_t findNextLineStart(unsigned start = 0) const
        { return m_impl ? m_impl->findNextLineStart(start) : notFound; }

    // Find the last instance of a single character or string.
    size_t reverseFind(UChar c, unsigned start = UINT_MAX) const
        { return m_impl ? m_impl->reverseFind(c, start) : notFound; }
    size_t reverseFind(const String& str, unsigned start = UINT_MAX) const
        { return m_impl ? m_impl->reverseFind(str.impl(), start) : notFound; }

    // Case insensitive string matching.
    size_t findIgnoringCase(const LChar* str, unsigned start = 0) const
        { return m_impl ? m_impl->findIgnoringCase(str, start) : notFound; }
    size_t findIgnoringCase(const String& str, unsigned start = 0) const
        { return m_impl ? m_impl->findIgnoringCase(str.impl(), start) : notFound; }
    size_t reverseFindIgnoringCase(const String& str, unsigned start = UINT_MAX) const
        { return m_impl ? m_impl->reverseFindIgnoringCase(str.impl(), start) : notFound; }

    // Wrappers for find & reverseFind adding dynamic sensitivity check.
    size_t find(const LChar* str, unsigned start, bool caseSensitive) const
        { return caseSensitive ? find(str, start) : findIgnoringCase(str, start); }
    size_t find(const String& str, unsigned start, bool caseSensitive) const
        { return caseSensitive ? find(str, start) : findIgnoringCase(str, start); }
    size_t reverseFind(const String& str, unsigned start, bool caseSensitive) const
        { return caseSensitive ? reverseFind(str, start) : reverseFindIgnoringCase(str, start); }

    WTF_EXPORT_STRING_API Vector<UChar> charactersWithNullTermination() const;
    WTF_EXPORT_STRING_API const UChar* deprecatedCharactersWithNullTermination();
    
    WTF_EXPORT_STRING_API UChar32 characterStartingAt(unsigned) const; // Ditto.
    
    bool contains(UChar c) const { return find(c) != notFound; }
    bool contains(const LChar* str, bool caseSensitive = true) const { return find(str, 0, caseSensitive) != notFound; }
    bool contains(const String& str, bool caseSensitive = true) const { return find(str, 0, caseSensitive) != notFound; }

    bool startsWith(const String& s) const
        { return m_impl ? m_impl->startsWith(s.impl()) : s.isEmpty(); }
    bool startsWith(const String& s, bool caseSensitive) const
        { return m_impl ? m_impl->startsWith(s.impl(), caseSensitive) : s.isEmpty(); }
    bool startsWith(UChar character) const
        { return m_impl ? m_impl->startsWith(character) : false; }
    template<unsigned matchLength>
    bool startsWith(const char (&prefix)[matchLength], bool caseSensitive = true) const
        { return m_impl ? m_impl->startsWith<matchLength>(prefix, caseSensitive) : !matchLength; }

    bool endsWith(const String& s, bool caseSensitive = true) const
        { return m_impl ? m_impl->endsWith(s.impl(), caseSensitive) : s.isEmpty(); }
    bool endsWith(UChar character) const
        { return m_impl ? m_impl->endsWith(character) : false; }
    template<unsigned matchLength>
    bool endsWith(const char (&prefix)[matchLength], bool caseSensitive = true) const
        { return m_impl ? m_impl->endsWith<matchLength>(prefix, caseSensitive) : !matchLength; }

    WTF_EXPORT_STRING_API void append(const String&);
    WTF_EXPORT_STRING_API void append(LChar);
    void append(char c) { append(static_cast<LChar>(c)); };
    WTF_EXPORT_STRING_API void append(UChar);
    WTF_EXPORT_STRING_API void append(const LChar*, unsigned length);
    WTF_EXPORT_STRING_API void append(const UChar*, unsigned length);
    WTF_EXPORT_STRING_API void insert(const String&, unsigned pos);
    void insert(const UChar*, unsigned length, unsigned pos);

    String& replace(UChar a, UChar b) { if (m_impl) m_impl = m_impl->replace(a, b); return *this; }
    String& replace(UChar a, const String& b) { if (m_impl) m_impl = m_impl->replace(a, b.impl()); return *this; }
    String& replace(const String& a, const String& b) { if (m_impl) m_impl = m_impl->replace(a.impl(), b.impl()); return *this; }
    String& replace(unsigned index, unsigned len, const String& b) { if (m_impl) m_impl = m_impl->replace(index, len, b.impl()); return *this; }

    template<unsigned charactersCount>
    ALWAYS_INLINE String& replaceWithLiteral(UChar a, const char (&characters)[charactersCount])
    {
        if (m_impl)
            m_impl = m_impl->replace(a, characters, charactersCount - 1);

        return *this;
    }

    void makeLower() { if (m_impl) m_impl = m_impl->lower(); }
    void makeUpper() { if (m_impl) m_impl = m_impl->upper(); }
    void fill(UChar c) { if (m_impl) m_impl = m_impl->fill(c); }

    WTF_EXPORT_STRING_API void truncate(unsigned len);
    WTF_EXPORT_STRING_API void remove(unsigned pos, int len = 1);

    WTF_EXPORT_STRING_API String substring(unsigned pos, unsigned len = UINT_MAX) const;
    WTF_EXPORT_STRING_API String substringSharingImpl(unsigned pos, unsigned len = UINT_MAX) const;
    String left(unsigned len) const { return substring(0, len); }
    String right(unsigned len) const { return substring(length() - len, len); }

    // Returns a lowercase/uppercase version of the string
    WTF_EXPORT_STRING_API String lower() const;
    WTF_EXPORT_STRING_API String upper() const;

    WTF_EXPORT_STRING_API String stripWhiteSpace() const;
    WTF_EXPORT_STRING_API String stripWhiteSpace(IsWhiteSpaceFunctionPtr) const;
    WTF_EXPORT_STRING_API String simplifyWhiteSpace() const;
    WTF_EXPORT_STRING_API String simplifyWhiteSpace(IsWhiteSpaceFunctionPtr) const;

    WTF_EXPORT_STRING_API String removeCharacters(CharacterMatchFunctionPtr) const;
    template<bool isSpecialCharacter(UChar)> bool isAllSpecialCharacters() const;

    // Return the string with case folded for case insensitive comparison.
    WTF_EXPORT_STRING_API String foldCase() const;

#if !PLATFORM(QT)
    WTF_EXPORT_STRING_API static String format(const char *, ...) WTF_ATTRIBUTE_PRINTF(1, 2);
#else
    WTF_EXPORT_STRING_API static String format(const char *, ...);
#endif

    // Returns an uninitialized string. The characters needs to be written
    // into the buffer returned in data before the returned string is used.
    // Failure to do this will have unpredictable results.
    static String createUninitialized(unsigned length, UChar*& data) { return StringImpl::createUninitialized(length, data); }
    static String createUninitialized(unsigned length, LChar*& data) { return StringImpl::createUninitialized(length, data); }

    WTF_EXPORT_STRING_API void split(const String& separator, bool allowEmptyEntries, Vector<String>& result) const;
    void split(const String& separator, Vector<String>& result) const
    {
        split(separator, false, result);
    }
    WTF_EXPORT_STRING_API void split(UChar separator, bool allowEmptyEntries, Vector<String>& result) const;
    void split(UChar separator, Vector<String>& result) const
    {
        split(separator, false, result);
    }

    WTF_EXPORT_STRING_API int toIntStrict(bool* ok = 0, int base = 10) const;
    WTF_EXPORT_STRING_API unsigned toUIntStrict(bool* ok = 0, int base = 10) const;
    WTF_EXPORT_STRING_API int64_t toInt64Strict(bool* ok = 0, int base = 10) const;
    uint64_t toUInt64Strict(bool* ok = 0, int base = 10) const;
    intptr_t toIntPtrStrict(bool* ok = 0, int base = 10) const;

    WTF_EXPORT_STRING_API int toInt(bool* ok = 0) const;
    WTF_EXPORT_STRING_API unsigned toUInt(bool* ok = 0) const;
    int64_t toInt64(bool* ok = 0) const;
    WTF_EXPORT_STRING_API uint64_t toUInt64(bool* ok = 0) const;
    WTF_EXPORT_STRING_API intptr_t toIntPtr(bool* ok = 0) const;

    // FIXME: Like the strict functions above, these give false for "ok" when there is trailing garbage.
    // Like the non-strict functions above, these return the value when there is trailing garbage.
    // It would be better if these were more consistent with the above functions instead.
    WTF_EXPORT_STRING_API double toDouble(bool* ok = 0) const;
    WTF_EXPORT_STRING_API float toFloat(bool* ok = 0) const;

    bool percentage(int& percentage) const;

#if COMPILER_SUPPORTS(CXX_REFERENCE_QUALIFIED_FUNCTIONS)
    WTF_EXPORT_STRING_API String isolatedCopy() const &;
    WTF_EXPORT_STRING_API String isolatedCopy() const &&;
#else
    WTF_EXPORT_STRING_API String isolatedCopy() const;
#endif

    WTF_EXPORT_STRING_API bool isSafeToSendToAnotherThread() const;

    // Prevent Strings from being implicitly convertable to bool as it will be ambiguous on any platform that
    // allows implicit conversion to another pointer type (e.g., Mac allows implicit conversion to NSString*).
    typedef struct ImplicitConversionFromWTFStringToBoolDisallowedA* (String::*UnspecifiedBoolTypeA);
    typedef struct ImplicitConversionFromWTFStringToBoolDisallowedB* (String::*UnspecifiedBoolTypeB);
    operator UnspecifiedBoolTypeA() const;
    operator UnspecifiedBoolTypeB() const;

#if USE(CF)
    String(CFStringRef);
    RetainPtr<CFStringRef> createCFString() const;
#endif

#ifdef __OBJC__
    String(NSString*);
    
    // This conversion maps NULL to "", which loses the meaning of NULL, but we 
    // need this mapping because AppKit crashes when passed nil NSStrings.
    operator NSString*() const { if (!m_impl) return @""; return *m_impl; }
#endif

#if PLATFORM(QT)
    WTF_EXPORT_STRING_API String(const QString&);
    WTF_EXPORT_STRING_API String(const QStringRef&);
    WTF_EXPORT_STRING_API operator QString() const;
#endif

#if PLATFORM(BLACKBERRY)
    String(const BlackBerry::Platform::String&);
    operator BlackBerry::Platform::String() const;
#endif

    WTF_EXPORT_STRING_API static String make8BitFrom16BitSource(const UChar*, size_t);
    template<size_t inlineCapacity>
    static String make8BitFrom16BitSource(const Vector<UChar, inlineCapacity>& buffer)
    {
        return make8BitFrom16BitSource(buffer.data(), buffer.size());
    }

    WTF_EXPORT_STRING_API static String make16BitFrom8BitSource(const LChar*, size_t);

    // String::fromUTF8 will return a null string if
    // the input data contains invalid UTF-8 sequences.
    WTF_EXPORT_STRING_API static String fromUTF8(const LChar*, size_t);
    WTF_EXPORT_STRING_API static String fromUTF8(const LChar*);
    static String fromUTF8(const char* s, size_t length) { return fromUTF8(reinterpret_cast<const LChar*>(s), length); };
    static String fromUTF8(const char* s) { return fromUTF8(reinterpret_cast<const LChar*>(s)); };
    WTF_EXPORT_STRING_API static String fromUTF8(const CString&);

    // Tries to convert the passed in string to UTF-8, but will fall back to Latin-1 if the string is not valid UTF-8.
    WTF_EXPORT_STRING_API static String fromUTF8WithLatin1Fallback(const LChar*, size_t);
    static String fromUTF8WithLatin1Fallback(const char* s, size_t length) { return fromUTF8WithLatin1Fallback(reinterpret_cast<const LChar*>(s), length); };
    
    // Determines the writing direction using the Unicode Bidi Algorithm rules P2 and P3.
    WTF::Unicode::Direction defaultWritingDirection(bool* hasStrongDirectionality = 0) const
    {
        if (m_impl)
            return m_impl->defaultWritingDirection(hasStrongDirectionality);
        if (hasStrongDirectionality)
            *hasStrongDirectionality = false;
        return WTF::Unicode::LeftToRight;
    }

    bool containsOnlyASCII() const;
    bool containsOnlyLatin1() const;
    bool containsOnlyWhitespace() const { return !m_impl || m_impl->containsOnlyWhitespace(); }

    // Hash table deleted values, which are only constructed and never copied or destroyed.
    String(WTF::HashTableDeletedValueType) : m_impl(WTF::HashTableDeletedValue) { }
    bool isHashTableDeletedValue() const { return m_impl.isHashTableDeletedValue(); }

#ifndef NDEBUG
    WTF_EXPORT_STRING_API void show() const;
#endif

    // Workaround for a compiler bug. Use operator[] instead.
    UChar characterAt(unsigned index) const
    {
        if (!m_impl || index >= m_impl->length())
            return 0;
        return (*m_impl)[index];
    }

private:
    template <typename CharacterType>
    void removeInternal(const CharacterType*, unsigned, int);

    template <typename CharacterType>
    void appendInternal(CharacterType);

    RefPtr<StringImpl> m_impl;
};

inline bool operator==(const String& a, const String& b) { return equal(a.impl(), b.impl()); }
inline bool operator==(const String& a, const LChar* b) { return equal(a.impl(), b); }
inline bool operator==(const String& a, const char* b) { return equal(a.impl(), reinterpret_cast<const LChar*>(b)); }
inline bool operator==(const LChar* a, const String& b) { return equal(a, b.impl()); }
inline bool operator==(const char* a, const String& b) { return equal(reinterpret_cast<const LChar*>(a), b.impl()); }
template<size_t inlineCapacity>
inline bool operator==(const Vector<char, inlineCapacity>& a, const String& b) { return equal(b.impl(), a.data(), a.size()); }
template<size_t inlineCapacity>
inline bool operator==(const String& a, const Vector<char, inlineCapacity>& b) { return b == a; }


inline bool operator!=(const String& a, const String& b) { return !equal(a.impl(), b.impl()); }
inline bool operator!=(const String& a, const LChar* b) { return !equal(a.impl(), b); }
inline bool operator!=(const String& a, const char* b) { return !equal(a.impl(), reinterpret_cast<const LChar*>(b)); }
inline bool operator!=(const LChar* a, const String& b) { return !equal(a, b.impl()); }
inline bool operator!=(const char* a, const String& b) { return !equal(reinterpret_cast<const LChar*>(a), b.impl()); }
template<size_t inlineCapacity>
inline bool operator!=(const Vector<char, inlineCapacity>& a, const String& b) { return !(a == b); }
template<size_t inlineCapacity>
inline bool operator!=(const String& a, const Vector<char, inlineCapacity>& b) { return b != a; }

inline bool equalIgnoringCase(const String& a, const String& b) { return equalIgnoringCase(a.impl(), b.impl()); }
inline bool equalIgnoringCase(const String& a, const LChar* b) { return equalIgnoringCase(a.impl(), b); }
inline bool equalIgnoringCase(const String& a, const char* b) { return equalIgnoringCase(a.impl(), reinterpret_cast<const LChar*>(b)); }
inline bool equalIgnoringCase(const LChar* a, const String& b) { return equalIgnoringCase(a, b.impl()); }
inline bool equalIgnoringCase(const char* a, const String& b) { return equalIgnoringCase(reinterpret_cast<const LChar*>(a), b.impl()); }

inline bool equalPossiblyIgnoringCase(const String& a, const String& b, bool ignoreCase) 
{
    return ignoreCase ? equalIgnoringCase(a, b) : (a == b);
}

inline bool equalIgnoringNullity(const String& a, const String& b) { return equalIgnoringNullity(a.impl(), b.impl()); }

template<size_t inlineCapacity>
inline bool equalIgnoringNullity(const Vector<UChar, inlineCapacity>& a, const String& b) { return equalIgnoringNullity(a, b.impl()); }

inline bool operator!(const String& str) { return str.isNull(); }

inline void swap(String& a, String& b) { a.swap(b); }

// Definitions of string operations

template<size_t inlineCapacity, typename OverflowHandler>
String::String(const Vector<UChar, inlineCapacity, OverflowHandler>& vector)
    : m_impl(vector.size() ? StringImpl::create(vector.data(), vector.size()) : StringImpl::empty())
{
}

template<>
inline const LChar* String::getCharacters<LChar>() const
{
    ASSERT(is8Bit());
    return characters8();
}

template<>
inline const UChar* String::getCharacters<UChar>() const
{
    ASSERT(!is8Bit());
    return characters16();
}

template<>
inline const LChar* String::getCharactersWithUpconvert<LChar>() const
{
    ASSERT(is8Bit());
    return characters8();
}

template<>
inline const UChar* String::getCharactersWithUpconvert<UChar>() const
{
    return characters();
}

inline bool String::containsOnlyLatin1() const
{
    if (isEmpty())
        return true;

    if (is8Bit())
        return true;

    const UChar* characters = characters16();
    UChar ored = 0;
    for (size_t i = 0; i < m_impl->length(); ++i)
        ored |= characters[i];
    return !(ored & 0xFF00);
}


#ifdef __OBJC__
// This is for situations in WebKit where the long standing behavior has been
// "nil if empty", so we try to maintain longstanding behavior for the sake of
// entrenched clients
inline NSString* nsStringNilIfEmpty(const String& str) {  return str.isEmpty() ? nil : (NSString*)str; }
#endif

inline bool String::containsOnlyASCII() const
{
    if (isEmpty())
        return true;

    if (is8Bit())
        return charactersAreAllASCII(characters8(), m_impl->length());

    return charactersAreAllASCII(characters16(), m_impl->length());
}

WTF_EXPORT_STRING_API int codePointCompare(const String&, const String&);

inline bool codePointCompareLessThan(const String& a, const String& b)
{
    return codePointCompare(a.impl(), b.impl()) < 0;
}

template<size_t inlineCapacity>
inline void append(Vector<UChar, inlineCapacity>& vector, const String& string)
{
    vector.append(string.characters(), string.length());
}

template<typename CharacterType>
inline void appendNumber(Vector<CharacterType>& vector, unsigned char number)
{
    int numberLength = number > 99 ? 3 : (number > 9 ? 2 : 1);
    size_t vectorSize = vector.size();
    vector.grow(vectorSize + numberLength);

    switch (numberLength) {
    case 3:
        vector[vectorSize + 2] = number % 10 + '0';
        number /= 10;

    case 2:
        vector[vectorSize + 1] = number % 10 + '0';
        number /= 10;

    case 1:
        vector[vectorSize] = number % 10 + '0';
    }
}

template<bool isSpecialCharacter(UChar), typename CharacterType>
inline bool isAllSpecialCharacters(const CharacterType* characters, size_t length)
{
    for (size_t i = 0; i < length; ++i) {
        if (!isSpecialCharacter(characters[i]))
            return false;
    }
    return true;
}

template<bool isSpecialCharacter(UChar)>
inline bool String::isAllSpecialCharacters() const
{
    size_t len = length();

    if (!len)
        return true;

    if (is8Bit())
        return WTF::isAllSpecialCharacters<isSpecialCharacter, LChar>(characters8(), len);
    return WTF::isAllSpecialCharacters<isSpecialCharacter, UChar>(characters(), len);
}

// StringHash is the default hash for String
template<typename T> struct DefaultHash;
template<> struct DefaultHash<String> {
    typedef StringHash Hash;
};

template <> struct VectorTraits<String> : SimpleClassVectorTraits { };

class ASCIILiteral {
public:
    explicit ASCIILiteral(const char* characters) : m_characters(characters) { }
    operator const char*() { return m_characters; }

private:
    const char* m_characters;
};

// Shared global empty string.
WTF_EXPORT_STRING_API const String& emptyString();

}

using WTF::CString;
using WTF::KeepTrailingZeros;
using WTF::String;
using WTF::emptyString;
using WTF::append;
using WTF::appendNumber;
using WTF::charactersAreAllASCII;
using WTF::charactersToIntStrict;
using WTF::charactersToUIntStrict;
using WTF::charactersToInt64Strict;
using WTF::charactersToUInt64Strict;
using WTF::charactersToIntPtrStrict;
using WTF::charactersToInt;
using WTF::charactersToUInt;
using WTF::charactersToInt64;
using WTF::charactersToUInt64;
using WTF::charactersToIntPtr;
using WTF::charactersToDouble;
using WTF::charactersToFloat;
using WTF::equal;
using WTF::equalIgnoringCase;
using WTF::find;
using WTF::isAllSpecialCharacters;
using WTF::isSpaceOrNewline;
using WTF::reverseFind;
using WTF::ASCIILiteral;

#include <wtf/text/AtomicString.h>
#endif
