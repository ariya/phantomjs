/*
 * Copyright (C) 2011, 2012 Apple Inc. All rights reserved.
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

#ifndef DFGExitProfile_h
#define DFGExitProfile_h

#include "ExitKind.h"
#include <wtf/HashSet.h>
#include <wtf/OwnPtr.h>
#include <wtf/Vector.h>

namespace JSC { namespace DFG {

class FrequentExitSite {
public:
    FrequentExitSite()
        : m_bytecodeOffset(0) // 0 = empty value
        , m_kind(ExitKindUnset)
    {
    }
    
    FrequentExitSite(WTF::HashTableDeletedValueType)
        : m_bytecodeOffset(1) // 1 = deleted value
        , m_kind(ExitKindUnset)
    {
    }
    
    explicit FrequentExitSite(unsigned bytecodeOffset, ExitKind kind)
        : m_bytecodeOffset(bytecodeOffset)
        , m_kind(kind)
    {
        ASSERT(exitKindIsCountable(kind));
    }
    
    // Use this constructor if you wish for the exit site to be counted globally within its
    // code block.
    explicit FrequentExitSite(ExitKind kind)
        : m_bytecodeOffset(0)
        , m_kind(kind)
    {
        ASSERT(exitKindIsCountable(kind));
    }
    
    bool operator!() const
    {
        return m_kind == ExitKindUnset;
    }
    
    bool operator==(const FrequentExitSite& other) const
    {
        return m_bytecodeOffset == other.m_bytecodeOffset
            && m_kind == other.m_kind;
    }
    
    unsigned hash() const
    {
        return WTF::intHash(m_bytecodeOffset) + m_kind;
    }
    
    unsigned bytecodeOffset() const { return m_bytecodeOffset; }
    ExitKind kind() const { return m_kind; }

    bool isHashTableDeletedValue() const
    {
        return m_kind == ExitKindUnset && m_bytecodeOffset;
    }

private:
    unsigned m_bytecodeOffset;
    ExitKind m_kind;
};

struct FrequentExitSiteHash {
    static unsigned hash(const FrequentExitSite& key) { return key.hash(); }
    static bool equal(const FrequentExitSite& a, const FrequentExitSite& b) { return a == b; }
    static const bool safeToCompareToEmptyOrDeleted = true;
};

} } // namespace JSC::DFG

namespace WTF {

template<typename T> struct DefaultHash;
template<> struct DefaultHash<JSC::DFG::FrequentExitSite> {
    typedef JSC::DFG::FrequentExitSiteHash Hash;
};

template<typename T> struct HashTraits;
template<> struct HashTraits<JSC::DFG::FrequentExitSite> : SimpleClassHashTraits<JSC::DFG::FrequentExitSite> { };

} // namespace WTF

namespace JSC { namespace DFG {

class QueryableExitProfile;

class ExitProfile {
public:
    ExitProfile();
    ~ExitProfile();
    
    // Add a new frequent exit site. Return true if this is a new one, or false
    // if we already knew about it. This is an O(n) operation, because it errs
    // on the side of keeping the data structure compact. Also, this will only
    // be called a fixed number of times per recompilation. Recompilation is
    // rare to begin with, and implies doing O(n) operations on the CodeBlock
    // anyway.
    bool add(const FrequentExitSite&);
    
    // Get the frequent exit sites for a bytecode index. This is O(n), and is
    // meant to only be used from debugging/profiling code.
    Vector<FrequentExitSite> exitSitesFor(unsigned bytecodeIndex);
    
    // This is O(n) and should be called on less-frequently executed code paths
    // in the compiler. It should be strictly cheaper than building a
    // QueryableExitProfile, if you really expect this to be called infrequently
    // and you believe that there are few exit sites.
    bool hasExitSite(const FrequentExitSite&) const;
    bool hasExitSite(ExitKind kind) const
    {
        return hasExitSite(FrequentExitSite(kind));
    }
    bool hasExitSite(unsigned bytecodeIndex, ExitKind kind) const
    {
        return hasExitSite(FrequentExitSite(bytecodeIndex, kind));
    }
    
private:
    friend class QueryableExitProfile;
    
    OwnPtr<Vector<FrequentExitSite> > m_frequentExitSites;
};

class QueryableExitProfile {
public:
    explicit QueryableExitProfile(const ExitProfile&);
    ~QueryableExitProfile();
    
    bool hasExitSite(const FrequentExitSite& site) const
    {
        return m_frequentExitSites.find(site) != m_frequentExitSites.end();
    }
    
    bool hasExitSite(ExitKind kind) const
    {
        return hasExitSite(FrequentExitSite(kind));
    }
    
    bool hasExitSite(unsigned bytecodeIndex, ExitKind kind) const
    {
        return hasExitSite(FrequentExitSite(bytecodeIndex, kind));
    }
private:
    HashSet<FrequentExitSite> m_frequentExitSites;
};

} } // namespace JSC::DFG

#endif // DFGExitProfile_h
