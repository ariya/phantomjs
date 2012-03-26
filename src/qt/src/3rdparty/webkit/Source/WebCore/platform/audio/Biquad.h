/*
 * Copyright (C) 2010 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef Biquad_h
#define Biquad_h

#include "AudioArray.h"
#include <sys/types.h>
#include <wtf/Complex.h>
#include <wtf/Platform.h>
 
namespace WebCore {

// A basic biquad (two-zero / two-pole digital filter)
//
// It can be configured to a number of common and very useful filters:
//    lowpass, highpass, shelving, parameteric, notch, allpass, ...

class Biquad {
public:   
    Biquad();
    virtual ~Biquad() { }

    void process(const float* sourceP, float* destP, size_t framesToProcess);

    // cutoff is 0-1 normalized, resonance is in dB >= 0.0
    void setLowpassParams(double cutoff, double resonance);
    void setHighpassParams(double cutoff, double resonance);

    void setLowShelfParams(double cutoff, double dbGain);

    // FIXME: need to implement a few more common filters
    // void setHighShelfParams(double cutoff, double dbGain);
    // void setParametricEQParams(double cutoff, double resonance);

    // Set the biquad coefficients given a single zero (other zero will be conjugate)
    // and a single pole (other pole will be conjugate)
    void setZeroPolePairs(const Complex& zero, const Complex& pole);

    // Set the biquad coefficients given a single pole (other pole will be conjugate)
    // (The zeroes will be the inverse of the poles)
    void setAllpassPole(const Complex& pole);

    // Resets filter state
    void reset();

private:
    // Filter coefficients
    double m_a0;
    double m_a1;
    double m_a2;
    double m_b1;
    double m_b2;

    double m_g;

    // Filter memory
    double m_x1; // input delayed by 1 sample
    double m_x2; // input delayed by 2 samples
    double m_y1; // output delayed by 1 sample
    double m_y2; // output delayed by 2 samples

#if OS(DARWIN)
    void processFast(const float* sourceP, float* destP, size_t framesToProcess);
    void processSliceFast(double* sourceP, double* destP, double* coefficientsP, size_t framesToProcess);

    AudioDoubleArray m_inputBuffer;
    AudioDoubleArray m_outputBuffer;
#endif
};

} // namespace WebCore

#endif // Biquad_h
