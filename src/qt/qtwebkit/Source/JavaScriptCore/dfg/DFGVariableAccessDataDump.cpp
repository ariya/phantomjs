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

#include "config.h"
#include "DFGVariableAccessDataDump.h"

#if ENABLE(DFG_JIT)

#include "DFGGraph.h"
#include "DFGVariableAccessData.h"

namespace JSC { namespace DFG {

VariableAccessDataDump::VariableAccessDataDump(Graph& graph, VariableAccessData* data)
    : m_graph(graph)
    , m_data(data)
{
}

void VariableAccessDataDump::dump(PrintStream& out) const
{
    unsigned index = std::numeric_limits<unsigned>::max();
    for (unsigned i = 0; i < m_graph.m_variableAccessData.size(); ++i) {
        if (&m_graph.m_variableAccessData[i] == m_data) {
            index = i;
            break;
        }
    }
    
    ASSERT(index != std::numeric_limits<unsigned>::max());
    
    if (!index) {
        out.print("a");
        return;
    }

    while (index) {
        out.print(CharacterDump('A' + (index % 26)));
        index /= 26;
    }
    
    if (m_data->isCaptured())
        out.print("*");
    else if (m_data->shouldNeverUnbox())
        out.print("!");
    else if (!m_data->shouldUnboxIfPossible())
        out.print("~");

    out.print(AbbreviatedSpeculationDump(m_data->prediction()));
}

} } // namespace JSC::DFG

#endif // ENABLE(DFG_JIT)


