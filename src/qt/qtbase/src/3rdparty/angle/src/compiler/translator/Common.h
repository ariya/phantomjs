//
// Copyright (c) 2002-2010 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#ifndef _COMMON_INCLUDED_
#define _COMMON_INCLUDED_

#include <map>
#include <sstream>
#include <string>
#include <vector>
#include <limits>
#include <stdio.h>

#include "compiler/translator/PoolAlloc.h"
#include "compiler/translator/compilerdebug.h"
#include "common/angleutils.h"

struct TSourceLoc {
    int first_file;
    int first_line;
    int last_file;
    int last_line;
};

//
// Put POOL_ALLOCATOR_NEW_DELETE in base classes to make them use this scheme.
//
#define POOL_ALLOCATOR_NEW_DELETE()                                                  \
    void* operator new(size_t s) { return GetGlobalPoolAllocator()->allocate(s); }   \
    void* operator new(size_t, void *_Where) { return (_Where); }                    \
    void operator delete(void*) { }                                                  \
    void operator delete(void *, void *) { }                                         \
    void* operator new[](size_t s) { return GetGlobalPoolAllocator()->allocate(s); } \
    void* operator new[](size_t, void *_Where) { return (_Where); }                  \
    void operator delete[](void*) { }                                                \
    void operator delete[](void *, void *) { }

//
// Pool version of string.
//
typedef pool_allocator<char> TStringAllocator;
typedef std::basic_string <char, std::char_traits<char>, TStringAllocator> TString;
typedef std::basic_ostringstream<char, std::char_traits<char>, TStringAllocator> TStringStream;
inline TString* NewPoolTString(const char* s)
{
	void* memory = GetGlobalPoolAllocator()->allocate(sizeof(TString));
	return new(memory) TString(s);
}

//
// Persistent string memory.  Should only be used for strings that survive
// across compiles.
//
#define TPersistString std::string
#define TPersistStringStream std::ostringstream

//
// Pool allocator versions of vectors, lists, and maps
//
template <class T> class TVector : public std::vector<T, pool_allocator<T> > {
public:
    typedef typename std::vector<T, pool_allocator<T> >::size_type size_type;
    TVector() : std::vector<T, pool_allocator<T> >() {}
    TVector(const pool_allocator<T>& a) : std::vector<T, pool_allocator<T> >(a) {}
    TVector(size_type i): std::vector<T, pool_allocator<T> >(i) {}
};

template <class K, class D, class CMP = std::less<K> > 
class TMap : public std::map<K, D, CMP, pool_allocator<std::pair<const K, D> > > {
public:
    typedef pool_allocator<std::pair<const K, D> > tAllocator;

    TMap() : std::map<K, D, CMP, tAllocator>() {}
    // use correct two-stage name lookup supported in gcc 3.4 and above
    TMap(const tAllocator& a) : std::map<K, D, CMP, tAllocator>(std::map<K, D, CMP, tAllocator>::key_compare(), a) {}
};

// Integer to TString conversion
template <typename T>
inline TString str(T i)
{
    ASSERT(std::numeric_limits<T>::is_integer);
    char buffer[((8 * sizeof(T)) / 3) + 3];
    const char *formatStr = std::numeric_limits<T>::is_signed ? "%d" : "%u";
    snprintf(buffer, sizeof(buffer), formatStr, i);
    return buffer;
}

#endif // _COMMON_INCLUDED_
