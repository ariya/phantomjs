/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 * Copyright (C) 2005, 2006, 2007, 2008, 2009, 2010 Apple Inc. All rights reserved.
 * Copyright (C) 2009 Google Inc. All rights reserved.
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

#ifndef StringImpl_h
#define StringImpl_h

#include <limits.h>
#include <wtf/ASCIICType.h>
#include <wtf/CrossThreadRefCounted.h>
#include <wtf/Forward.h>
#include <wtf/OwnFastMallocPtr.h>
#include <wtf/StdLibExtras.h>
#include <wtf/StringHasher.h>
#include <wtf/Vector.h>
#include <wtf/text/StringImplBase.h>
#include <wtf/unicode/Unicode.h>

#if USE(CF)
typedef const struct __CFString * CFStringRef;
#endif

#ifdef __OBJC__
@class NSString;
#endif

// FIXME: This is a temporary layering violation while we move string code to WTF.
// Landing the file moves in one patch, will follow on with patches to change the namespaces.
namespace JSC {
struct IdentifierCStringTranslator;
struct IdentifierUCharBufferTranslator;
}

namespace WTF {

struct CStringTranslator;
struct HashAndCharactersTranslator;
struct HashAndUTF8CharactersTranslator;
struct UCharBufferTranslator;

enum TextCaseSensitivity { TextCaseSensitive, TextCaseInsensitive };

typedef OwnFastMallocPtr<const UChar> SharableUChar;
typedef CrossThreadRefCounted<SharableUChar> SharedUChar;
typedef bool (*CharacterMatchFunctionPtr)(UChar);

class StringImpl : public StringImplBase {
    friend struct JSC::IdentifierCStringTranslator;
    friend struct JSC::IdentifierUCharBufferTranslator;
    friend struct WTF::CStringTranslator;
    friend struct WTF::HashAndCharactersTranslator;
    friend struct WTF::HashAndUTF8CharactersTranslator;
    friend struct WTF::UCharBufferTranslator;
    friend class AtomicStringImpl;
private:
    // Used to construct static strings, which have an special refCount that can never hit zero.
    // This means that the static string will never be destroyed, which is important because
    // static strings will be shared across threads & ref-counted in a non-threadsafe manner.
    StringImpl(const UChar* characters, unsigned length, StaticStringConstructType)
        : StringImplBase(length, ConstructStaticString)
        , m_data(characters)
        , m_buffer(0)
        , m_hash(0)
    {
        // Ensure that the hash is computed so that AtomicStringHash can call existingHash()
        // with impunity. The empty string is special because it is never entered into
        // AtomicString's HashKey, but still needs to compare correctly.
        hash();
    }

    // Create a normal string with internal storage (BufferInternal)
    StringImpl(unsigned length)
        : StringImplBase(length, BufferInternal)
        , m_data(reinterpret_cast<const UChar*>(this + 1))
        , m_buffer(0)
        , m_hash(0)
    {
        ASSERT(m_data);
        ASSERT(m_length);
    }

    // Create a StringImpl adopting ownership of the provided buffer (BufferOwned)
    StringImpl(const UChar* characters, unsigned length)
        : StringImplBase(length, BufferOwned)
        , m_data(characters)
        , m_buffer(0)
        , m_hash(0)
    {
        ASSERT(m_data);
        ASSERT(m_length);
    }

    // Used to create new strings that are a substring of an existing StringImpl (BufferSubstring)
    StringImpl(const UChar* characters, unsigned length, PassRefPtr<StringImpl> base)
        : StringImplBase(length, BufferSubstring)
        , m_data(characters)
        , m_substringBuffer(base.leakRef())
        , m_hash(0)
    {
        ASSERT(m_data);
        ASSERT(m_length);
        ASSERT(m_substringBuffer->bufferOwnership() != BufferSubstring);
    }

    // Used to construct new strings sharing an existing SharedUChar (BufferShared)
    StringImpl(const UChar* characters, unsigned length, PassRefPtr<SharedUChar> sharedBuffer)
        : StringImplBase(length, BufferShared)
        , m_data(characters)
        , m_sharedBuffer(sharedBuffer.leakRef())
        , m_hash(0)
    {
        ASSERT(m_data);
        ASSERT(m_length);
    }

    // For use only by AtomicString's XXXTranslator helpers.
    void setHash(unsigned hash)
    {
        ASSERT(!isStatic());
        ASSERT(!m_hash);
        ASSERT(hash == StringHasher::computeHash(m_data, m_length));
        m_hash = hash;
    }

public:
    ~StringImpl();

    static PassRefPtr<StringImpl> create(const UChar*, unsigned length);
    static PassRefPtr<StringImpl> create(const char*, unsigned length);
    static PassRefPtr<StringImpl> create(const char*);
    static PassRefPtr<StringImpl> create(const UChar*, unsigned length, PassRefPtr<SharedUChar> sharedBuffer);
    static ALWAYS_INLINE PassRefPtr<StringImpl> create(PassRefPtr<StringImpl> rep, unsigned offset, unsigned length)
    {
        ASSERT(rep);
        ASSERT(length <= rep->length());

        if (!length)
            return empty();

        StringImpl* ownerRep = (rep->bufferOwnership() == BufferSubstring) ? rep->m_substringBuffer : rep.get();
        return adoptRef(new StringImpl(rep->m_data + offset, length, ownerRep));
    }

    static PassRefPtr<StringImpl> createUninitialized(unsigned length, UChar*& data);
    static ALWAYS_INLINE PassRefPtr<StringImpl> tryCreateUninitialized(unsigned length, UChar*& output)
    {
        if (!length) {
            output = 0;
            return empty();
        }

        if (length > ((std::numeric_limits<unsigned>::max() - sizeof(StringImpl)) / sizeof(UChar))) {
            output = 0;
            return 0;
        }
        StringImpl* resultImpl;
        if (!tryFastMalloc(sizeof(UChar) * length + sizeof(StringImpl)).getValue(resultImpl)) {
            output = 0;
            return 0;
        }
        output = reinterpret_cast<UChar*>(resultImpl + 1);
        return adoptRef(new(resultImpl) StringImpl(length));
    }

    static unsigned dataOffset() { return OBJECT_OFFSETOF(StringImpl, m_data); }
    static PassRefPtr<StringImpl> createWithTerminatingNullCharacter(const StringImpl&);
    static PassRefPtr<StringImpl> createStrippingNullCharacters(const UChar*, unsigned length);

    template<size_t inlineCapacity>
    static PassRefPtr<StringImpl> adopt(Vector<UChar, inlineCapacity>& vector)
    {
        if (size_t size = vector.size()) {
            ASSERT(vector.data());
            if (size > std::numeric_limits<unsigned>::max())
                CRASH();
            return adoptRef(new StringImpl(vector.releaseBuffer(), size));
        }
        return empty();
    }
    static PassRefPtr<StringImpl> adopt(StringBuffer&);

    SharedUChar* sharedBuffer();
    const UChar* characters() const { return m_data; }

    size_t cost()
    {
        // For substrings, return the cost of the base string.
        if (bufferOwnership() == BufferSubstring)
            return m_substringBuffer->cost();

        if (m_refCountAndFlags & s_refCountFlagShouldReportedCost) {
            m_refCountAndFlags &= ~s_refCountFlagShouldReportedCost;
            return m_length;
        }
        return 0;
    }

    bool isIdentifier() const { return m_refCountAndFlags & s_refCountFlagIsIdentifier; }
    void setIsIdentifier(bool isIdentifier)
    {
        ASSERT(!isStatic());
        if (isIdentifier)
            m_refCountAndFlags |= s_refCountFlagIsIdentifier;
        else
            m_refCountAndFlags &= ~s_refCountFlagIsIdentifier;
    }

    bool hasTerminatingNullCharacter() const { return m_refCountAndFlags & s_refCountFlagHasTerminatingNullCharacter; }

    bool isAtomic() const { return m_refCountAndFlags & s_refCountFlagIsAtomic; }
    void setIsAtomic(bool isIdentifier)
    {
        ASSERT(!isStatic());
        if (isIdentifier)
            m_refCountAndFlags |= s_refCountFlagIsAtomic;
        else
            m_refCountAndFlags &= ~s_refCountFlagIsAtomic;
    }

    unsigned hash() const { if (!m_hash) m_hash = StringHasher::computeHash(m_data, m_length); return m_hash; }
    unsigned existingHash() const { ASSERT(m_hash); return m_hash; }

    ALWAYS_INLINE void deref() { m_refCountAndFlags -= s_refCountIncrement; if (!(m_refCountAndFlags & (s_refCountMask | s_refCountFlagStatic))) delete this; }
    ALWAYS_INLINE bool hasOneRef() const { return (m_refCountAndFlags & (s_refCountMask | s_refCountFlagStatic)) == s_refCountIncrement; }

    static StringImpl* empty();

    static void copyChars(UChar* destination, const UChar* source, unsigned numCharacters)
    {
        if (numCharacters <= s_copyCharsInlineCutOff) {
            for (unsigned i = 0; i < numCharacters; ++i)
                destination[i] = source[i];
        } else
            memcpy(destination, source, numCharacters * sizeof(UChar));
    }

    // Returns a StringImpl suitable for use on another thread.
    PassRefPtr<StringImpl> crossThreadString();
    // Makes a deep copy. Helpful only if you need to use a String on another thread
    // (use crossThreadString if the method call doesn't need to be threadsafe).
    // Since StringImpl objects are immutable, there's no other reason to make a copy.
    PassRefPtr<StringImpl> threadsafeCopy() const;

    PassRefPtr<StringImpl> substring(unsigned pos, unsigned len = UINT_MAX);

    UChar operator[](unsigned i) { ASSERT(i < m_length); return m_data[i]; }
    UChar32 characterStartingAt(unsigned);

    bool containsOnlyWhitespace();

    int toIntStrict(bool* ok = 0, int base = 10);
    unsigned toUIntStrict(bool* ok = 0, int base = 10);
    int64_t toInt64Strict(bool* ok = 0, int base = 10);
    uint64_t toUInt64Strict(bool* ok = 0, int base = 10);
    intptr_t toIntPtrStrict(bool* ok = 0, int base = 10);

    int toInt(bool* ok = 0); // ignores trailing garbage
    unsigned toUInt(bool* ok = 0); // ignores trailing garbage
    int64_t toInt64(bool* ok = 0); // ignores trailing garbage
    uint64_t toUInt64(bool* ok = 0); // ignores trailing garbage
    intptr_t toIntPtr(bool* ok = 0); // ignores trailing garbage

    double toDouble(bool* ok = 0, bool* didReadNumber = 0);
    float toFloat(bool* ok = 0, bool* didReadNumber = 0);

    PassRefPtr<StringImpl> lower();
    PassRefPtr<StringImpl> upper();

    PassRefPtr<StringImpl> fill(UChar);
    PassRefPtr<StringImpl> foldCase();

    PassRefPtr<StringImpl> stripWhiteSpace();
    PassRefPtr<StringImpl> simplifyWhiteSpace();

    PassRefPtr<StringImpl> removeCharacters(CharacterMatchFunctionPtr);

    size_t find(UChar, unsigned index = 0);
    size_t find(CharacterMatchFunctionPtr, unsigned index = 0);
    size_t find(const char*, unsigned index = 0);
    size_t find(StringImpl*, unsigned index = 0);
    size_t findIgnoringCase(const char*, unsigned index = 0);
    size_t findIgnoringCase(StringImpl*, unsigned index = 0);

    size_t reverseFind(UChar, unsigned index = UINT_MAX);
    size_t reverseFind(StringImpl*, unsigned index = UINT_MAX);
    size_t reverseFindIgnoringCase(StringImpl*, unsigned index = UINT_MAX);

    bool startsWith(StringImpl* str, bool caseSensitive = true) { return (caseSensitive ? reverseFind(str, 0) : reverseFindIgnoringCase(str, 0)) == 0; }
    bool endsWith(StringImpl*, bool caseSensitive = true);

    PassRefPtr<StringImpl> replace(UChar, UChar);
    PassRefPtr<StringImpl> replace(UChar, StringImpl*);
    PassRefPtr<StringImpl> replace(StringImpl*, StringImpl*);
    PassRefPtr<StringImpl> replace(unsigned index, unsigned len, StringImpl*);

    WTF::Unicode::Direction defaultWritingDirection(bool* hasStrongDirectionality = 0);

#if USE(CF)
    CFStringRef createCFString();
#endif
#ifdef __OBJC__
    operator NSString*();
#endif

private:
    // This number must be at least 2 to avoid sharing empty, null as well as 1 character strings from SmallStrings.
    static const unsigned s_copyCharsInlineCutOff = 20;

    static PassRefPtr<StringImpl> createStrippingNullCharactersSlowCase(const UChar*, unsigned length);
    
    BufferOwnership bufferOwnership() const { return static_cast<BufferOwnership>(m_refCountAndFlags & s_refCountMaskBufferOwnership); }
    bool isStatic() const { return m_refCountAndFlags & s_refCountFlagStatic; }
    const UChar* m_data;
    union {
        void* m_buffer;
        StringImpl* m_substringBuffer;
        SharedUChar* m_sharedBuffer;
    };
    mutable unsigned m_hash;
};

bool equal(const StringImpl*, const StringImpl*);
bool equal(const StringImpl*, const char*);
inline bool equal(const char* a, StringImpl* b) { return equal(b, a); }

bool equalIgnoringCase(StringImpl*, StringImpl*);
bool equalIgnoringCase(StringImpl*, const char*);
inline bool equalIgnoringCase(const char* a, StringImpl* b) { return equalIgnoringCase(b, a); }
bool equalIgnoringCase(const UChar* a, const char* b, unsigned length);
inline bool equalIgnoringCase(const char* a, const UChar* b, unsigned length) { return equalIgnoringCase(b, a, length); }

bool equalIgnoringNullity(StringImpl*, StringImpl*);

template<size_t inlineCapacity>
bool equalIgnoringNullity(const Vector<UChar, inlineCapacity>& a, StringImpl* b)
{
    if (!b)
        return !a.size();
    if (a.size() != b->length())
        return false;
    return !memcmp(a.data(), b->characters(), b->length());
}

int codePointCompare(const StringImpl*, const StringImpl*);

static inline bool isSpaceOrNewline(UChar c)
{
    // Use isASCIISpace() for basic Latin-1.
    // This will include newlines, which aren't included in Unicode DirWS.
    return c <= 0x7F ? WTF::isASCIISpace(c) : WTF::Unicode::direction(c) == WTF::Unicode::WhiteSpaceNeutral;
}

// This is a hot function because it's used when parsing HTML.
inline PassRefPtr<StringImpl> StringImpl::createStrippingNullCharacters(const UChar* characters, unsigned length)
{
    ASSERT(characters);
    ASSERT(length);

    // Optimize for the case where there are no Null characters by quickly
    // searching for nulls, and then using StringImpl::create, which will
    // memcpy the whole buffer.  This is faster than assigning character by
    // character during the loop. 

    // Fast case.
    int foundNull = 0;
    for (unsigned i = 0; !foundNull && i < length; i++) {
        int c = characters[i]; // more efficient than using UChar here (at least on Intel Mac OS)
        foundNull |= !c;
    }
    if (!foundNull)
        return StringImpl::create(characters, length);

    return StringImpl::createStrippingNullCharactersSlowCase(characters, length);
}

struct StringHash;

// StringHash is the default hash for StringImpl* and RefPtr<StringImpl>
template<typename T> struct DefaultHash;
template<> struct DefaultHash<StringImpl*> {
    typedef StringHash Hash;
};
template<> struct DefaultHash<RefPtr<StringImpl> > {
    typedef StringHash Hash;
};

}

using WTF::StringImpl;
using WTF::equal;
using WTF::TextCaseSensitivity;
using WTF::TextCaseSensitive;
using WTF::TextCaseInsensitive;

#endif
