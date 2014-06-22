/*
 * Copyright (C) 2011 Apple Inc. All rights reserved.
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

#ifndef DFGPredictionPropagationPhase_h
#define DFGPredictionPropagationPhase_h

#include <wtf/Platform.h>

#if ENABLE(DFG_JIT)

#include "SpeculatedType.h"

namespace JSC { namespace DFG {

class Graph;

// Propagate predictions gathered at heap load sites by the value profiler, and
// from slow path executions, to generate a prediction for each node in the graph.
// This is a crucial phase of compilation, since before running this phase, we
// have no idea what types any node (or most variables) could possibly have, unless
// that node is either a heap load, a call, a GetLocal for an argument, or an
// arithmetic op that had definitely taken slow path. Most nodes (even most
// arithmetic nodes) do not qualify for any of these categories. But after running
// this phase, we'll have full information for the expected type of each node.

bool performPredictionPropagation(Graph&);

// Helper used for FixupPhase for computing the predicted type of a ToPrimitive.
SpeculatedType resultOfToPrimitive(SpeculatedType type);

} } // namespace JSC::DFG::Phase

#endif // ENABLE(DFG_JIT)

#endif // DFGPredictionPropagationPhase_h
