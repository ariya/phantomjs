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

#ifndef ExecutionCounter_h
#define ExecutionCounter_h

#include "JSGlobalObject.h"
#include "Options.h"
#include <wtf/PrintStream.h>
#include <wtf/SimpleStats.h>

namespace JSC {

class CodeBlock;

class ExecutionCounter {
public:
    ExecutionCounter();
    bool checkIfThresholdCrossedAndSet(CodeBlock*);
    void setNewThreshold(int32_t threshold, CodeBlock*);
    void deferIndefinitely();
    double count() const { return static_cast<double>(m_totalCount) + m_counter; }
    void dump(PrintStream&) const;
    static double applyMemoryUsageHeuristics(int32_t value, CodeBlock*);
    static int32_t applyMemoryUsageHeuristicsAndConvertToInt(int32_t value, CodeBlock*);
    template<typename T>
    static T clippedThreshold(JSGlobalObject* globalObject, T threshold)
    {
        int32_t maxThreshold;
        if (Options::randomizeExecutionCountsBetweenCheckpoints())
            maxThreshold = globalObject->weakRandomInteger() % Options::maximumExecutionCountsBetweenCheckpoints();
        else
            maxThreshold = Options::maximumExecutionCountsBetweenCheckpoints();
        if (threshold > maxThreshold)
            threshold = maxThreshold;
        return threshold;
    }

    static int32_t formattedTotalCount(float value)
    {
        union {
            int32_t i;
            float f;
        } u;
        u.f = value;
        return u.i;
    }
    
private:
    bool hasCrossedThreshold(CodeBlock*) const;
    bool setThreshold(CodeBlock*);
    void reset();

public:

    // NB. These are intentionally public because it will be modified from machine code.
    
    // This counter is incremented by the JIT or LLInt. It starts out negative and is
    // counted up until it becomes non-negative. At the start of a counting period,
    // the threshold we wish to reach is m_totalCount + m_counter, in the sense that
    // we will add X to m_totalCount and subtract X from m_counter.
    int32_t m_counter;

    // Counts the total number of executions we have seen plus the ones we've set a
    // threshold for in m_counter. Because m_counter's threshold is negative, the
    // total number of actual executions can always be computed as m_totalCount +
    // m_counter.
    float m_totalCount;

    // This is the threshold we were originally targetting, without any correction for
    // the memory usage heuristics.
    int32_t m_activeThreshold;
};

} // namespace JSC

#endif // ExecutionCounter_h

