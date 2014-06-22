/*
 * Copyright (C) 2012, 2013 Apple Inc. All Rights Reserved.
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

#ifndef CodeCache_h
#define CodeCache_h

#include "CodeSpecializationKind.h"
#include "ParserModes.h"
#include "SourceCode.h"
#include "Strong.h"
#include "WeakRandom.h"
#include <wtf/CurrentTime.h>
#include <wtf/FixedArray.h>
#include <wtf/Forward.h>
#include <wtf/PassOwnPtr.h>
#include <wtf/RandomNumber.h>
#include <wtf/text/WTFString.h>

namespace JSC {

class EvalExecutable;
class FunctionBodyNode;
class Identifier;
class JSScope;
class ProgramExecutable;
class UnlinkedCodeBlock;
class UnlinkedEvalCodeBlock;
class UnlinkedFunctionCodeBlock;
class UnlinkedFunctionExecutable;
class UnlinkedProgramCodeBlock;
class VM;
struct ParserError;
class SourceCode;
class SourceProvider;

class SourceCodeKey {
public:
    enum CodeType { EvalType, ProgramType, FunctionType };

    SourceCodeKey()
    {
    }

    SourceCodeKey(const SourceCode& sourceCode, const String& name, CodeType codeType, JSParserStrictness jsParserStrictness)
        : m_sourceCode(sourceCode)
        , m_name(name)
        , m_flags((codeType << 1) | jsParserStrictness)
        , m_hash(string().impl()->hash())
    {
    }

    SourceCodeKey(WTF::HashTableDeletedValueType)
        : m_sourceCode(WTF::HashTableDeletedValue)
    {
    }

    bool isHashTableDeletedValue() const { return m_sourceCode.isHashTableDeletedValue(); }

    unsigned hash() const { return m_hash; }

    size_t length() const { return m_sourceCode.length(); }

    bool isNull() const { return m_sourceCode.isNull(); }

    // To save memory, we compute our string on demand. It's expected that source
    // providers cache their strings to make this efficient.
    String string() const { return m_sourceCode.toString(); }

    bool operator==(const SourceCodeKey& other) const
    {
        return m_hash == other.m_hash
            && length() == other.length()
            && m_flags == other.m_flags
            && m_name == other.m_name
            && string() == other.string();
    }

private:
    SourceCode m_sourceCode;
    String m_name;
    unsigned m_flags;
    unsigned m_hash;
};

struct SourceCodeKeyHash {
    static unsigned hash(const SourceCodeKey& key) { return key.hash(); }
    static bool equal(const SourceCodeKey& a, const SourceCodeKey& b) { return a == b; }
    static const bool safeToCompareToEmptyOrDeleted = false;
};

struct SourceCodeKeyHashTraits : SimpleClassHashTraits<SourceCodeKey> {
    static const bool hasIsEmptyValueFunction = true;
    static bool isEmptyValue(const SourceCodeKey& sourceCodeKey) { return sourceCodeKey.isNull(); }
};

struct SourceCodeValue {
    SourceCodeValue()
    {
    }

    SourceCodeValue(VM& vm, JSCell* cell, int64_t age)
        : cell(vm, cell)
        , age(age)
    {
    }

    Strong<JSCell> cell;
    int64_t age;
};

class CodeCacheMap {
public:
    typedef HashMap<SourceCodeKey, SourceCodeValue, SourceCodeKeyHash, SourceCodeKeyHashTraits> MapType;
    typedef MapType::iterator iterator;
    typedef MapType::AddResult AddResult;

    CodeCacheMap(int64_t workingSetMaxBytes, size_t workingSetMaxEntries)
        : m_size(0)
        , m_sizeAtLastPrune(0)
        , m_timeAtLastPrune(monotonicallyIncreasingTime())
        , m_minCapacity(0)
        , m_capacity(0)
        , m_age(0)
        , m_workingSetMaxBytes(workingSetMaxBytes)
        , m_workingSetMaxEntries(workingSetMaxEntries)
    {
    }

    AddResult add(const SourceCodeKey& key, const SourceCodeValue& value)
    {
        prune();

        AddResult addResult = m_map.add(key, value);
        if (addResult.isNewEntry) {
            m_size += key.length();
            m_age += key.length();
            return addResult;
        }

        int64_t age = m_age - addResult.iterator->value.age;
        if (age > m_capacity) {
            // A requested object is older than the cache's capacity. We can
            // infer that requested objects are subject to high eviction probability,
            // so we grow the cache to improve our hit rate.
            m_capacity += recencyBias * oldObjectSamplingMultiplier * key.length();
        } else if (age < m_capacity / 2) {
            // A requested object is much younger than the cache's capacity. We can
            // infer that requested objects are subject to low eviction probability,
            // so we shrink the cache to save memory.
            m_capacity -= recencyBias * key.length();
            if (m_capacity < m_minCapacity)
                m_capacity = m_minCapacity;
        }

        addResult.iterator->value.age = m_age;
        m_age += key.length();
        return addResult;
    }

    void remove(iterator it)
    {
        m_size -= it->key.length();
        m_map.remove(it);
    }

    void clear()
    {
        m_size = 0;
        m_age = 0;
        m_map.clear();
    }

    int64_t age() { return m_age; }

    static const int64_t globalWorkingSetMaxBytes;
    static const size_t globalWorkingSetMaxEntries;

    // We have a smaller cap for the per-codeblock CodeCache that approximates the
    // linked EvalCodeCache limits, but still allows us to keep large string based
    // evals at least partially cached.
    static const unsigned nonGlobalWorkingSetScale;
    static const int64_t nonGlobalWorkingSetMaxBytes;
    static const size_t nonGlobalWorkingSetMaxEntries;

private:
    // This constant factor biases cache capacity toward allowing a minimum
    // working set to enter the cache before it starts evicting.
    static const double workingSetTime;

    // This constant factor biases cache capacity toward recent activity. We
    // want to adapt to changing workloads.
    static const int64_t recencyBias = 4;

    // This constant factor treats a sampled event for one old object as if it
    // happened for many old objects. Most old objects are evicted before we can
    // sample them, so we need to extrapolate from the ones we do sample.
    static const int64_t oldObjectSamplingMultiplier = 32;

    size_t numberOfEntries() const { return static_cast<size_t>(m_map.size()); }
    bool canPruneQuickly() const { return numberOfEntries() < m_workingSetMaxEntries; }

    void pruneSlowCase();
    void prune()
    {
        if (m_size <= m_capacity && canPruneQuickly())
            return;

        if (monotonicallyIncreasingTime() - m_timeAtLastPrune < workingSetTime
            && m_size - m_sizeAtLastPrune < m_workingSetMaxBytes
            && canPruneQuickly())
                return;

        pruneSlowCase();
    }

    MapType m_map;
    int64_t m_size;
    int64_t m_sizeAtLastPrune;
    double m_timeAtLastPrune;
    int64_t m_minCapacity;
    int64_t m_capacity;
    int64_t m_age;
    const int64_t m_workingSetMaxBytes;
    const size_t m_workingSetMaxEntries;
};

// Caches top-level code such as <script>, eval(), new Function, and JSEvaluateScript().
class CodeCache : public RefCounted<CodeCache> {
public:
    enum CodeCacheKind { GlobalCodeCache, NonGlobalCodeCache };
    static PassRefPtr<CodeCache> create(CodeCacheKind kind) { return adoptRef(new CodeCache(kind)); }

    UnlinkedProgramCodeBlock* getProgramCodeBlock(VM&, ProgramExecutable*, const SourceCode&, JSParserStrictness, DebuggerMode, ProfilerMode, ParserError&);
    UnlinkedEvalCodeBlock* getEvalCodeBlock(VM&, JSScope*, EvalExecutable*, const SourceCode&, JSParserStrictness, DebuggerMode, ProfilerMode, ParserError&);
    UnlinkedFunctionExecutable* getFunctionExecutableFromGlobalCode(VM&, const Identifier&, const SourceCode&, ParserError&);
    ~CodeCache();

    void clear()
    {
        m_sourceCode.clear();
    }

private:
    CodeCache(CodeCacheKind);

    template <class UnlinkedCodeBlockType, class ExecutableType> 
    UnlinkedCodeBlockType* getCodeBlock(VM&, JSScope*, ExecutableType*, const SourceCode&, JSParserStrictness, DebuggerMode, ProfilerMode, ParserError&);

    template <class UnlinkedCodeBlockType, class ExecutableType>
    UnlinkedCodeBlockType* generateBytecode(VM&, JSScope*, ExecutableType*, const SourceCode&, JSParserStrictness, DebuggerMode, ProfilerMode, ParserError&);

    CodeCacheMap m_sourceCode;
};

}

#endif // CodeCache_h
