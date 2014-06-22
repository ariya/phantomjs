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

#ifndef DFGPhase_h
#define DFGPhase_h

#include <wtf/Platform.h>

#if ENABLE(DFG_JIT)

#include "DFGCommon.h"
#include "DFGGraph.h"

namespace JSC { namespace DFG {

class Phase {
public:
    Phase(Graph& graph, const char* name)
        : m_graph(graph)
        , m_name(name)
    {
        beginPhase();
    }
    
    ~Phase()
    {
        endPhase();
    }
    
    const char* name() const { return m_name; }
    
    // Each phase must have a run() method.
    
protected:
    // Things you need to have a DFG compiler phase.
    Graph& m_graph;
    
    VM& vm() { return m_graph.m_vm; }
    CodeBlock* codeBlock() { return m_graph.m_codeBlock; }
    CodeBlock* profiledBlock() { return m_graph.m_profiledBlock; }
    
    const char* m_name;
    
private:
    // Call these hooks when starting and finishing.
    void beginPhase();
    void endPhase();
};

template<typename PhaseType>
bool runAndLog(PhaseType& phase)
{
    bool result = phase.run();
    if (result && logCompilationChanges())
        dataLogF("Phase %s changed the IR.\n", phase.name());
    return result;
}

template<typename PhaseType>
bool runPhase(Graph& graph)
{
    PhaseType phase(graph);
    return runAndLog(phase);
}

} } // namespace JSC::DFG

#endif // ENABLE(DFG_JIT)

#endif // DFGPhase_h

