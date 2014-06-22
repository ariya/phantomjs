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
#include "ExecutionCounter.h"

#include "CodeBlock.h"
#include "ExecutableAllocator.h"
#include <wtf/StringExtras.h>

namespace JSC {

ExecutionCounter::ExecutionCounter()
{
    reset();
}

bool ExecutionCounter::checkIfThresholdCrossedAndSet(CodeBlock* codeBlock)
{
    if (hasCrossedThreshold(codeBlock))
        return true;
    
    if (setThreshold(codeBlock))
        return true;
    
    return false;
}

void ExecutionCounter::setNewThreshold(int32_t threshold, CodeBlock* codeBlock)
{
    reset();
    m_activeThreshold = threshold;
    setThreshold(codeBlock);
}

void ExecutionCounter::deferIndefinitely()
{
    m_totalCount = 0;
    m_activeThreshold = std::numeric_limits<int32_t>::max();
    m_counter = std::numeric_limits<int32_t>::min();
}

double ExecutionCounter::applyMemoryUsageHeuristics(int32_t value, CodeBlock* codeBlock)
{
#if ENABLE(JIT)
    double multiplier =
        ExecutableAllocator::memoryPressureMultiplier(
            codeBlock->predictedMachineCodeSize());
#else
    // This code path will probably not be taken, but if it is, we fake it.
    double multiplier = 1.0;
    UNUSED_PARAM(codeBlock);
#endif
    ASSERT(multiplier >= 1.0);
    return multiplier * value;
}

int32_t ExecutionCounter::applyMemoryUsageHeuristicsAndConvertToInt(
    int32_t value, CodeBlock* codeBlock)
{
    double doubleResult = applyMemoryUsageHeuristics(value, codeBlock);
    
    ASSERT(doubleResult >= 0);
    
    if (doubleResult > std::numeric_limits<int32_t>::max())
        return std::numeric_limits<int32_t>::max();
    
    return static_cast<int32_t>(doubleResult);
}

bool ExecutionCounter::hasCrossedThreshold(CodeBlock* codeBlock) const
{
    // This checks if the current count rounded up to the threshold we were targeting.
    // For example, if we are using half of available executable memory and have
    // m_activeThreshold = 1000, applyMemoryUsageHeuristics(m_activeThreshold) will be
    // 2000, but we will pretend as if the threshold was crossed if we reach 2000 -
    // 1000 / 2, or 1500. The reasoning here is that we want to avoid thrashing. If
    // this method returns false, then the JIT's threshold for when it will again call
    // into the slow path (which will call this method a second time) will be set
    // according to the difference between the current count and the target count
    // according to *current* memory usage. But by the time we call into this again, we
    // may have JIT'ed more code, and so the target count will increase slightly. This
    // may lead to a repeating pattern where the target count is slightly incremented,
    // the JIT immediately matches that increase, calls into the slow path again, and
    // again the target count is slightly incremented. Instead of having this vicious
    // cycle, we declare victory a bit early if the difference between the current
    // total and our target according to memory heuristics is small. Our definition of
    // small is arbitrarily picked to be half of the original threshold (i.e.
    // m_activeThreshold).
    
    double modifiedThreshold = applyMemoryUsageHeuristics(m_activeThreshold, codeBlock);
    
    return static_cast<double>(m_totalCount) + m_counter >=
        modifiedThreshold - static_cast<double>(
            std::min(m_activeThreshold, Options::maximumExecutionCountsBetweenCheckpoints())) / 2;
}

bool ExecutionCounter::setThreshold(CodeBlock* codeBlock)
{
    if (m_activeThreshold == std::numeric_limits<int32_t>::max()) {
        deferIndefinitely();
        return false;
    }
        
    ASSERT(!hasCrossedThreshold(codeBlock));
        
    // Compute the true total count.
    double trueTotalCount = count();
        
    // Correct the threshold for current memory usage.
    double threshold = applyMemoryUsageHeuristics(m_activeThreshold, codeBlock);
        
    // Threshold must be non-negative and not NaN.
    ASSERT(threshold >= 0);
        
    // Adjust the threshold according to the number of executions we have already
    // seen. This shouldn't go negative, but it might, because of round-off errors.
    threshold -= trueTotalCount;
        
    if (threshold <= 0) {
        m_counter = 0;
        m_totalCount = trueTotalCount;
        return true;
    }

    threshold = clippedThreshold(codeBlock->globalObject(), threshold);
    
    m_counter = static_cast<int32_t>(-threshold);
        
    m_totalCount = trueTotalCount + threshold;
        
    return false;
}

void ExecutionCounter::reset()
{
    m_counter = 0;
    m_totalCount = 0;
    m_activeThreshold = 0;
}

void ExecutionCounter::dump(PrintStream& out) const
{
    out.printf("%lf/%lf, %d", count(), static_cast<double>(m_activeThreshold), m_counter);
}

} // namespace JSC

