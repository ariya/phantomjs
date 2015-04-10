/*
 * Copyright (C) 2012 Apple Inc. All rights reserved.
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

#ifndef LazyOperandValueProfile_h
#define LazyOperandValueProfile_h

#include <wtf/Platform.h>

#if ENABLE(VALUE_PROFILER)

#include "ValueProfile.h"
#include <wtf/HashMap.h>
#include <wtf/Noncopyable.h>
#include <wtf/OwnPtr.h>
#include <wtf/SegmentedVector.h>

namespace JSC {

class ScriptExecutable;

class LazyOperandValueProfileKey {
public:
    LazyOperandValueProfileKey()
        : m_bytecodeOffset(0) // 0 = empty value
        , m_operand(-1) // not a valid operand index in our current scheme
    {
    }
    
    LazyOperandValueProfileKey(WTF::HashTableDeletedValueType)
        : m_bytecodeOffset(1) // 1 = deleted value
        , m_operand(-1) // not a valid operand index in our current scheme
    {
    }
    
    LazyOperandValueProfileKey(unsigned bytecodeOffset, int operand)
        : m_bytecodeOffset(bytecodeOffset)
        , m_operand(operand)
    {
        ASSERT(operand != -1);
    }
    
    bool operator!() const
    {
        return m_operand == -1;
    }
    
    bool operator==(const LazyOperandValueProfileKey& other) const
    {
        return m_bytecodeOffset == other.m_bytecodeOffset
            && m_operand == other.m_operand;
    }
    
    unsigned hash() const
    {
        return WTF::intHash(m_bytecodeOffset) + m_operand;
    }
    
    unsigned bytecodeOffset() const
    {
        ASSERT(!!*this);
        return m_bytecodeOffset;
    }
    int operand() const
    {
        ASSERT(!!*this);
        return m_operand;
    }
    
    bool isHashTableDeletedValue() const
    {
        return m_operand == -1 && m_bytecodeOffset;
    }
private: 
    unsigned m_bytecodeOffset;
    int m_operand;
};

struct LazyOperandValueProfileKeyHash {
    static unsigned hash(const LazyOperandValueProfileKey& key) { return key.hash(); }
    static bool equal(
        const LazyOperandValueProfileKey& a,
        const LazyOperandValueProfileKey& b) { return a == b; }
    static const bool safeToCompareToEmptyOrDeleted = true;
};

} // namespace JSC

namespace WTF {

template<typename T> struct DefaultHash;
template<> struct DefaultHash<JSC::LazyOperandValueProfileKey> {
    typedef JSC::LazyOperandValueProfileKeyHash Hash;
};

template<typename T> struct HashTraits;
template<> struct HashTraits<JSC::LazyOperandValueProfileKey> : public GenericHashTraits<JSC::LazyOperandValueProfileKey> {
    static void constructDeletedValue(JSC::LazyOperandValueProfileKey& slot) { new (NotNull, &slot) JSC::LazyOperandValueProfileKey(HashTableDeletedValue); }
    static bool isDeletedValue(const JSC::LazyOperandValueProfileKey& value) { return value.isHashTableDeletedValue(); }
};

} // namespace WTF

namespace JSC {

struct LazyOperandValueProfile : public MinimalValueProfile {
    LazyOperandValueProfile()
        : MinimalValueProfile()
        , m_operand(-1)
    {
    }
    
    explicit LazyOperandValueProfile(const LazyOperandValueProfileKey& key)
        : MinimalValueProfile(key.bytecodeOffset())
        , m_operand(key.operand())
    {
    }
    
    LazyOperandValueProfileKey key() const
    {
        return LazyOperandValueProfileKey(m_bytecodeOffset, m_operand);
    }
    
    int m_operand;
    
    typedef SegmentedVector<LazyOperandValueProfile, 8> List;
};

class LazyOperandValueProfileParser;

class CompressedLazyOperandValueProfileHolder {
    WTF_MAKE_NONCOPYABLE(CompressedLazyOperandValueProfileHolder);
public:
    CompressedLazyOperandValueProfileHolder();
    ~CompressedLazyOperandValueProfileHolder();
    
    void computeUpdatedPredictions(OperationInProgress);
    
    LazyOperandValueProfile* add(const LazyOperandValueProfileKey& key);
    
private:
    friend class LazyOperandValueProfileParser;
    OwnPtr<LazyOperandValueProfile::List> m_data;
};

class LazyOperandValueProfileParser {
    WTF_MAKE_NONCOPYABLE(LazyOperandValueProfileParser);
public:
    explicit LazyOperandValueProfileParser(
        CompressedLazyOperandValueProfileHolder& holder);
    ~LazyOperandValueProfileParser();
    
    LazyOperandValueProfile* getIfPresent(
        const LazyOperandValueProfileKey& key) const;
    
    SpeculatedType prediction(const LazyOperandValueProfileKey& key) const;
private:
    CompressedLazyOperandValueProfileHolder& m_holder;
    HashMap<LazyOperandValueProfileKey, LazyOperandValueProfile*> m_map;
};

} // namespace JSC

#endif // ENABLE(VALUE_PROFILER)

#endif // LazyOperandValueProfile_h


