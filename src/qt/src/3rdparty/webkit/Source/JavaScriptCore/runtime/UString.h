/*
 * Copyright (C) 1999-2000 Harri Porten (porten@kde.org)
 * Copyright (C) 2004, 2005, 2006, 2007, 2008, 2009 Apple Inc. All rights reserved.
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

#ifndef UString_h
#define UString_h

#include <wtf/text/StringImpl.h>

namespace JSC {

class UString {
public:
    // Construct a null string, distinguishable from an empty string.
    UString() { }

    // Construct a string with UTF-16 data.
    UString(const UChar* characters, unsigned length);

    // Construct a string with UTF-16 data, from a null-terminated source.
    UString(const UChar*);

    // Construct a string with latin1 data.
    UString(const char* characters, unsigned length);

    // Construct a string with latin1 data, from a null-terminated source.
    UString(const char* characters);

    // Construct a string referencing an existing StringImpl.
    UString(StringImpl* impl) : m_impl(impl) { }
    UString(PassRefPtr<StringImpl> impl) : m_impl(impl) { }
    UString(RefPtr<StringImpl> impl) : m_impl(impl) { }

    // Inline the destructor.
    ALWAYS_INLINE ~UString() { }

    void swap(UString& o) { m_impl.swap(o.m_impl); }

    template<size_t inlineCapacity>
    static UString adopt(Vector<UChar, inlineCapacity>& vector) { return StringImpl::adopt(vector); }

    bool isNull() const { return !m_impl; }
    bool isEmpty() const { return !m_impl || !m_impl->length(); }

    StringImpl* impl() const { return m_impl.get(); }

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

    CString ascii() const;
    CString latin1() const;
    CString utf8(bool strict = false) const;

    UChar operator[](unsigned index) const
    {
        if (!m_impl || index >= m_impl->length())
            return 0;
        return m_impl->characters()[index];
    }

    static UString number(int);
    static UString number(unsigned);
    static UString number(long);
    static UString number(long long);
    static UString number(double);

    // Find a single character or string, also with match function & latin1 forms.
    size_t find(UChar c, unsigned start = 0) const
        { return m_impl ? m_impl->find(c, start) : notFound; }
    size_t find(const UString& str, unsigned start = 0) const
        { return m_impl ? m_impl->find(str.impl(), start) : notFound; }
    size_t find(const char* str, unsigned start = 0) const
        { return m_impl ? m_impl->find(str, start) : notFound; }

    // Find the last instance of a single character or string.
    size_t reverseFind(UChar c, unsigned start = UINT_MAX) const
        { return m_impl ? m_impl->reverseFind(c, start) : notFound; }
    size_t reverseFind(const UString& str, unsigned start = UINT_MAX) const
        { return m_impl ? m_impl->reverseFind(str.impl(), start) : notFound; }

    UString substringSharingImpl(unsigned pos, unsigned len = UINT_MAX) const;

private:
    RefPtr<StringImpl> m_impl;
};

ALWAYS_INLINE bool operator==(const UString& s1, const UString& s2)
{
    StringImpl* rep1 = s1.impl();
    StringImpl* rep2 = s2.impl();
    unsigned size1 = 0;
    unsigned size2 = 0;

    if (rep1 == rep2) // If they're the same rep, they're equal.
        return true;
    
    if (rep1)
        size1 = rep1->length();
        
    if (rep2)
        size2 = rep2->length();
        
    if (size1 != size2) // If the lengths are not the same, we're done.
        return false;
    
    if (!size1)
        return true;
    
    // At this point we know 
    //   (a) that the strings are the same length and
    //   (b) that they are greater than zero length.
    const UChar* d1 = rep1->characters();
    const UChar* d2 = rep2->characters();
    
    if (d1 == d2) // Check to see if the data pointers are the same.
        return true;
    
    // Do quick checks for sizes 1 and 2.
    switch (size1) {
    case 1:
        return d1[0] == d2[0];
    case 2:
        return (d1[0] == d2[0]) & (d1[1] == d2[1]);
    default:
        return memcmp(d1, d2, size1 * sizeof(UChar)) == 0;
    }
}


inline bool operator!=(const UString& s1, const UString& s2)
{
    return !JSC::operator==(s1, s2);
}

bool operator<(const UString& s1, const UString& s2);
bool operator>(const UString& s1, const UString& s2);

bool operator==(const UString& s1, const char* s2);

inline bool operator!=(const UString& s1, const char* s2)
{
    return !JSC::operator==(s1, s2);
}

inline bool operator==(const char *s1, const UString& s2)
{
    return operator==(s2, s1);
}

inline bool operator!=(const char *s1, const UString& s2)
{
    return !JSC::operator==(s1, s2);
}

inline int codePointCompare(const UString& s1, const UString& s2)
{
    return codePointCompare(s1.impl(), s2.impl());
}

struct UStringHash {
    static unsigned hash(StringImpl* key) { return key->hash(); }
    static bool equal(const StringImpl* a, const StringImpl* b)
    {
        if (a == b)
            return true;
        if (!a || !b)
            return false;

        unsigned aLength = a->length();
        unsigned bLength = b->length();
        if (aLength != bLength)
            return false;

        // FIXME: perhaps we should have a more abstract macro that indicates when
        // going 4 bytes at a time is unsafe
#if CPU(ARM) || CPU(SH4) || CPU(MIPS) || CPU(SPARC)
        const UChar* aChars = a->characters();
        const UChar* bChars = b->characters();
        for (unsigned i = 0; i != aLength; ++i) {
            if (*aChars++ != *bChars++)
                return false;
        }
        return true;
#else
        /* Do it 4-bytes-at-a-time on architectures where it's safe */
        const uint32_t* aChars = reinterpret_cast<const uint32_t*>(a->characters());
        const uint32_t* bChars = reinterpret_cast<const uint32_t*>(b->characters());

        unsigned halfLength = aLength >> 1;
        for (unsigned i = 0; i != halfLength; ++i)
            if (*aChars++ != *bChars++)
                return false;

        if (aLength & 1 && *reinterpret_cast<const uint16_t*>(aChars) != *reinterpret_cast<const uint16_t*>(bChars))
            return false;

        return true;
#endif
    }

    static unsigned hash(const RefPtr<StringImpl>& key) { return key->hash(); }
    static bool equal(const RefPtr<StringImpl>& a, const RefPtr<StringImpl>& b)
    {
        return equal(a.get(), b.get());
    }

    static unsigned hash(const UString& key) { return key.impl()->hash(); }
    static bool equal(const UString& a, const UString& b)
    {
        return equal(a.impl(), b.impl());
    }

    static const bool safeToCompareToEmptyOrDeleted = false;
};

} // namespace JSC

namespace WTF {

// UStringHash is the default hash for UString
template<typename T> struct DefaultHash;
template<> struct DefaultHash<JSC::UString> {
    typedef JSC::UStringHash Hash;
};

template <> struct VectorTraits<JSC::UString> : SimpleClassVectorTraits { };

} // namespace WTF

#endif

