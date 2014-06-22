/*
 * Copyright (C) 2013 Apple Inc. All rights reserved.
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

#ifndef StaticPropertyAnalyzer_h
#define StaticPropertyAnalyzer_h

#include "StaticPropertyAnalysis.h"
#include <wtf/HashMap.h>

namespace JSC {

// Used for flow-insensitive static analysis of the number of properties assigned to an object.
// We use this analysis with other runtime data to produce an optimization guess. This analysis
// is understood to be lossy, and it's OK if it turns out to be wrong sometimes.
class StaticPropertyAnalyzer {
public:
    StaticPropertyAnalyzer(Vector<UnlinkedInstruction, 0, UnsafeVectorOverflow>*);

    void createThis(int dst, unsigned offsetOfInlineCapacityOperand);
    void newObject(int dst, unsigned offsetOfInlineCapacityOperand);
    void putById(int dst, unsigned propertyIndex); // propertyIndex is an index into a uniqued set of strings.
    void mov(int dst, int src);

    void kill();
    void kill(int dst);

private:
    void kill(StaticPropertyAnalysis*);

    Vector<UnlinkedInstruction, 0, UnsafeVectorOverflow>* m_instructions;
    typedef HashMap<int, RefPtr<StaticPropertyAnalysis>, WTF::IntHash<int>, WTF::UnsignedWithZeroKeyHashTraits<int> > AnalysisMap;
    AnalysisMap m_analyses;
};

inline StaticPropertyAnalyzer::StaticPropertyAnalyzer(Vector<UnlinkedInstruction, 0, UnsafeVectorOverflow>* instructions)
    : m_instructions(instructions)
{
}

inline void StaticPropertyAnalyzer::createThis(int dst, unsigned offsetOfInlineCapacityOperand)
{
    AnalysisMap::AddResult addResult = m_analyses.add(
        dst, StaticPropertyAnalysis::create(m_instructions, offsetOfInlineCapacityOperand));
    ASSERT_UNUSED(addResult, addResult.isNewEntry); // Can't have two 'this' in the same constructor.
}

inline void StaticPropertyAnalyzer::newObject(int dst, unsigned offsetOfInlineCapacityOperand)
{
    RefPtr<StaticPropertyAnalysis> analysis = StaticPropertyAnalysis::create(m_instructions, offsetOfInlineCapacityOperand);
    AnalysisMap::AddResult addResult = m_analyses.add(dst, analysis);
    if (!addResult.isNewEntry) {
        kill(addResult.iterator->value.get());
        addResult.iterator->value = analysis.release();
    }
}

inline void StaticPropertyAnalyzer::putById(int dst, unsigned propertyIndex)
{
    StaticPropertyAnalysis* analysis = m_analyses.get(dst);
    if (!analysis)
        return;
    analysis->addPropertyIndex(propertyIndex);
}

inline void StaticPropertyAnalyzer::mov(int dst, int src)
{
    RefPtr<StaticPropertyAnalysis> analysis = m_analyses.get(src);
    if (!analysis) {
        kill(dst);
        return;
    }

    AnalysisMap::AddResult addResult = m_analyses.add(dst, analysis);
    if (!addResult.isNewEntry) {
        kill(addResult.iterator->value.get());
        addResult.iterator->value = analysis.release();
    }
}

inline void StaticPropertyAnalyzer::kill(StaticPropertyAnalysis* analysis)
{
    if (!analysis)
        return;
    if (!analysis->hasOneRef()) // Aliases for this object still exist, so it might acquire more properties.
        return;
    analysis->record();
}

inline void StaticPropertyAnalyzer::kill(int dst)
{
    // We observe kills in order to avoid piling on properties to an object after
    // its bytecode register has been recycled.

    // Consider these cases:

    // (1) Aliased temporary
    // var o1 = { name: name };
    // var o2 = { name: name };

    // (2) Aliased local -- no control flow
    // var local;
    // local = new Object;
    // local.name = name;
    // ...

    // local = lookup();
    // local.didLookup = true;
    // ...

    // (3) Aliased local -- control flow
    // var local;
    // if (condition)
    //     local = { };
    // else {
    //     local = new Object;
    // }
    // local.name = name;

    // (Note: our default codegen for "new Object" looks like case (3).)

    // Case (1) is easy because temporaries almost never survive across control flow.

    // Cases (2) and (3) are hard. Case (2) should kill "local", while case (3) should
    // not. There is no great way to solve these cases with simple static analysis.

    // Since this is a simple static analysis, we just try to catch the simplest cases,
    // so we accept kills to any registers except for registers that have no inferred
    // properties yet.

    AnalysisMap::iterator it = m_analyses.find(dst);
    if (it == m_analyses.end())
        return;
    if (!it->value->propertyIndexCount())
        return;

    kill(it->value.get());
    m_analyses.remove(it);
}

inline void StaticPropertyAnalyzer::kill()
{
    while (m_analyses.size())
        kill(m_analyses.take(m_analyses.begin()->key).get());
}

} // namespace JSC

#endif // StaticPropertyAnalyzer_h
