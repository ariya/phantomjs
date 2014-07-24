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

#ifndef SimpleStats_h
#define SimpleStats_h

#include <wtf/MathExtras.h>
#include <wtf/StdLibExtras.h>

namespace WTF {

// Simple and cheap way of tracking statistics if you're not worried about chopping on
// the sum of squares (i.e. the sum of squares is unlikely to exceed 2^52).
class SimpleStats {
public:
    SimpleStats()
        : m_count(0)
        , m_sum(0)
        , m_sumOfSquares(0)
    {
    }
    
    void add(double value)
    {
        m_count++;
        m_sum += value;
        m_sumOfSquares += value * value;
    }
    
    bool operator!() const
    {
        return !m_count;
    }
    
    double count() const
    {
        return m_count;
    }
    
    double sum() const
    {
        return m_sum;
    }
    
    double sumOfSquares() const
    {
        return m_sumOfSquares;
    }
    
    double mean() const
    {
        return m_sum / m_count;
    }
    
    // NB. This gives a biased variance as it divides by the number of samples rather
    // than the degrees of freedom. This is fine once the count grows large, which in
    // our case will happen rather quickly.
    double variance() const
    {
        if (m_count < 2)
            return 0;
        
        // Compute <x^2> - <x>^2
        double secondMoment = m_sumOfSquares / m_count;
        double firstMoment = m_sum / m_count;
        
        double result = secondMoment - firstMoment * firstMoment;
        
        // It's possible to get -epsilon. Protect against this and turn it into
        // +0.
        if (result <= 0)
            return 0;
        
        return result;
    }
    
    // NB. This gives a biased standard deviation. See above.
    double standardDeviation() const
    {
        return sqrt(variance());
    }
    
private:
    double m_count;
    double m_sum;
    double m_sumOfSquares;
};

} // namespace WTF

#endif // SimpleStats_h

