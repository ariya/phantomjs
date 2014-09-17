/*
 * Copyright (C) 2004, 2005, 2006, 2008 Apple Inc. All rights reserved.
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

#ifndef AtomicString_h
#define AtomicString_h

#include "AtomicStringImpl.h"
#include "WTFString.h"

// Define 'NO_IMPLICIT_ATOMICSTRING' before including this header,
// to disallow (expensive) implicit String-->AtomicString conversions.
#ifdef NO_IMPLICIT_ATOMICSTRING
#define ATOMICSTRING_CONVERSION explicit
#else
#define ATOMICSTRING_CONVERSION
#endif

namespace WTF {

struct AtomicStringHash;

class AtomicString {
public:
    static void init();

    AtomicString() { }
    AtomicString(const char* s) : m_string(add(s)) { }
    AtomicString(const UChar* s, unsigned length) : m_string(add(s, length)) { }
    AtomicString(const UChar* s, unsigned length, unsigned existingHash) : m_string(add(s, length, existingHash)) { }
    AtomicString(const UChar* s) : m_string(add(s)) { }
    ATOMICSTRING_CONVERSION AtomicString(StringImpl* imp) : m_string(add(imp)) { }
    AtomicString(AtomicStringImpl* imp) : m_string(imp) { }
    ATOMICSTRING_CONVERSION AtomicString(const String& s) : m_string(add(s.impl())) { }

    // Hash table deleted values, which are only constructed and never copied or destroyed.
    AtomicString(WTF::HashTableDeletedValueType) : m_string(WTF::HashTableDeletedValue) { }
    bool isHashTableDeletedValue() const { return m_string.isHashTableDeletedValue(); }

    static AtomicStringImpl* find(const UChar* s, unsigned length, unsigned existingHash);

    operator const String&() const { return m_string; }
    const String& string() const { return m_string; };

    AtomicStringImpl* impl() const { return static_cast<AtomicStringImpl *>(m_string.impl()); }
    
    const UChar* characters() const { return m_string.characters(); }
    unsigned length() const { return m_string.length(); }
    
    UChar operator[](unsigned int i) const { return m_string[i]; }
    
    bool contains(UChar c) const { return m_string.contains(c); }
    bool contains(const char* s, bool caseSensitive = true) const
        { return m_string.contains(s, caseSensitive); }
    bool contains(const String& s, bool caseSensitive = true) const
        { return m_string.contains(s, caseSensitive); }

    size_t find(UChar c, size_t start = 0) const { return m_string.find(c, start); }
    size_t find(const char* s, size_t start = 0, bool caseSentitive = true) const
        { return m_string.find(s, start, caseSentitive); }
    size_t find(const String& s, size_t start = 0, bool caseSentitive = true) const
        { return m_string.find(s, start, caseSentitive); }
    
    bool startsWith(const String& s, bool caseSensitive = true) const
        { return m_string.startsWith(s, caseSensitive); }
    bool endsWith(const String& s, bool caseSensitive = true) const
        { return m_string.endsWith(s, caseSensitive); }
    
    AtomicString lower() const;
    AtomicString upper() const { return AtomicString(impl()->upper()); }
    
    int toInt(bool* ok = 0) const { return m_string.toInt(ok); }
    double toDouble(bool* ok = 0) const { return m_string.toDouble(ok); }
    float toFloat(bool* ok = 0) const { return m_string.toFloat(ok); }
    bool percentage(int& p) const { return m_string.percentage(p); }

    bool isNull() const { return m_string.isNull(); }
    bool isEmpty() const { return m_string.isEmpty(); }

    static void remove(StringImpl*);
    
#if USE(CF)
    AtomicString(CFStringRef s) :  m_string(add(String(s).impl())) { }
    CFStringRef createCFString() const { return m_string.createCFString(); }
#endif    
#ifdef __OBJC__
    AtomicString(NSString* s) : m_string(add(String(s).impl())) { }
    operator NSString*() const { return m_string; }
#endif
#if PLATFORM(QT)
    AtomicString(const QString& s) : m_string(add(String(s).impl())) { }
    operator QString() const { return m_string; }
#endif

    // AtomicString::fromUTF8 will return a null string if
    // the input data contains invalid UTF-8 sequences.
    static AtomicString fromUTF8(const char*, size_t);
    static AtomicString fromUTF8(const char*);

#if COMPILER(WINSCW)
    static const AtomicString& nullAtom2();
    static const AtomicString& emptyAtom2();
    static const AtomicString& textAtom2();
    static const AtomicString& commentAtom2();
    static const AtomicString& starAtom2();
    static const AtomicString& xmlAtom2();
    static const AtomicString& xmlnsAtom2();
#endif

private:
    String m_string;
    
    static PassRefPtr<StringImpl> add(const char*);
    static PassRefPtr<StringImpl> add(const UChar*, unsigned length);
    static PassRefPtr<StringImpl> add(const UChar*, unsigned length, unsigned existingHash);
    static PassRefPtr<StringImpl> add(const UChar*);
    ALWAYS_INLINE PassRefPtr<StringImpl> add(StringImpl* r)
    {
        if (!r || r->isAtomic())
            return r;
        return addSlowCase(r);
    }
    static PassRefPtr<StringImpl> addSlowCase(StringImpl*);
    static AtomicString fromUTF8Internal(const char*, const char*);
};

inline bool operator==(const AtomicString& a, const AtomicString& b) { return a.impl() == b.impl(); }
bool operator==(const AtomicString& a, const char* b);
bool operator==(const AtomicString& a, const Vector<UChar>& b);
inline bool operator==(const AtomicString& a, const String& b) { return equal(a.impl(), b.impl()); }
inline bool operator==(const char* a, const AtomicString& b) { return b == a; }
inline bool operator==(const String& a, const AtomicString& b) { return equal(a.impl(), b.impl()); }
inline bool operator==(const Vector<UChar>& a, const AtomicString& b) { return b == a; }

inline bool operator!=(const AtomicString& a, const AtomicString& b) { return a.impl() != b.impl(); }
inline bool operator!=(const AtomicString& a, const char *b) { return !(a == b); }
inline bool operator!=(const AtomicString& a, const String& b) { return !equal(a.impl(), b.impl()); }
inline bool operator!=(const AtomicString& a, const Vector<UChar>& b) { return !(a == b); }
inline bool operator!=(const char* a, const AtomicString& b) { return !(b == a); }
inline bool operator!=(const String& a, const AtomicString& b) { return !equal(a.impl(), b.impl()); }
inline bool operator!=(const Vector<UChar>& a, const AtomicString& b) { return !(a == b); }

inline bool equalIgnoringCase(const AtomicString& a, const AtomicString& b) { return equalIgnoringCase(a.impl(), b.impl()); }
inline bool equalIgnoringCase(const AtomicString& a, const char* b) { return equalIgnoringCase(a.impl(), b); }
inline bool equalIgnoringCase(const AtomicString& a, const String& b) { return equalIgnoringCase(a.impl(), b.impl()); }
inline bool equalIgnoringCase(const char* a, const AtomicString& b) { return equalIgnoringCase(a, b.impl()); }
inline bool equalIgnoringCase(const String& a, const AtomicString& b) { return equalIgnoringCase(a.impl(), b.impl()); }

// Define external global variables for the commonly used atomic strings.
// These are only usable from the main thread.
#ifndef ATOMICSTRING_HIDE_GLOBALS

#if COMPILER(WINSCW)
extern const JS_EXPORTDATA AtomicString nullAtom1;
extern const JS_EXPORTDATA AtomicString emptyAtom1;
extern const JS_EXPORTDATA AtomicString textAtom1;
extern const JS_EXPORTDATA AtomicString commentAtom1;
extern const JS_EXPORTDATA AtomicString starAtom1;
extern const JS_EXPORTDATA AtomicString xmlAtom1;
extern const JS_EXPORTDATA AtomicString xmlnsAtom1;

#define nullAtom AtomicString::nullAtom2()
#define emptyAtom AtomicString::emptyAtom2()
#define textAtom AtomicString::textAtom2()
#define commentAtom AtomicString::commentAtom2()
#define starAtom AtomicString::starAtom2()
#define xmlAtom AtomicString::xmlAtom2()
#define xmlnsAtom AtomicString::xmlnsAtom2()
#else
extern const JS_EXPORTDATA AtomicString nullAtom;
extern const JS_EXPORTDATA AtomicString emptyAtom;
extern const JS_EXPORTDATA AtomicString textAtom;
extern const JS_EXPORTDATA AtomicString commentAtom;
extern const JS_EXPORTDATA AtomicString starAtom;
extern const JS_EXPORTDATA AtomicString xmlAtom;
extern const JS_EXPORTDATA AtomicString xmlnsAtom;
#endif

inline AtomicString AtomicString::fromUTF8(const char* characters, size_t length)
{
    if (!characters)
        return nullAtom;
    if (!length)
        return emptyAtom;
    return fromUTF8Internal(characters, characters + length);
}

inline AtomicString AtomicString::fromUTF8(const char* characters)
{
    if (!characters)
        return nullAtom;
    if (!*characters)
        return emptyAtom;
    return fromUTF8Internal(characters, 0);
}
#endif

// AtomicStringHash is the default hash for AtomicString
template<typename T> struct DefaultHash;
template<> struct DefaultHash<AtomicString> {
    typedef AtomicStringHash Hash;
};

} // namespace WTF

#if !COMPILER(WINSCW)
#ifndef ATOMICSTRING_HIDE_GLOBALS
using WTF::AtomicString;
using WTF::nullAtom;
using WTF::emptyAtom;
using WTF::textAtom;
using WTF::commentAtom;
using WTF::starAtom;
using WTF::xmlAtom;
using WTF::xmlnsAtom;
#endif
#endif

#endif // AtomicString_h
