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

#include "config.h"

#if ENABLE(WEB_AUDIO)

#include "Biquad.h"

#include <algorithm>
#include <stdio.h>
#include <wtf/MathExtras.h>

#if OS(DARWIN)
#include <Accelerate/Accelerate.h>
#endif

namespace WebCore {

const int kBufferSize = 1024;

Biquad::Biquad()
{
#if OS(DARWIN)
    // Allocate two samples more for filter history
    m_inputBuffer.resize(kBufferSize + 2);
    m_outputBuffer.resize(kBufferSize + 2);
#endif

    // Initialize as pass-thru (straight-wire, no filter effect)
    m_a0 = 1.0;
    m_a1 = 0.0;
    m_a2 = 0.0;
    m_b1 = 0.0;
    m_b2 = 0.0;

    m_g = 1.0;

    reset(); // clear filter memory
}

void Biquad::process(const float* sourceP, float* destP, size_t framesToProcess)
{
#if OS(DARWIN)
    // Use vecLib if available
    processFast(sourceP, destP, framesToProcess);
#else
    int n = framesToProcess;

    // Create local copies of member variables
    double x1 = m_x1;
    double x2 = m_x2;
    double y1 = m_y1;
    double y2 = m_y2;

    double a0 = m_a0;
    double a1 = m_a1;
    double a2 = m_a2;
    double b1 = m_b1;
    double b2 = m_b2;

    while (n--) {
        // FIXME: this can be optimized by pipelining the multiply adds...
        float x = *sourceP++;
        float y = a0*x + a1*x1 + a2*x2 - b1*y1 - b2*y2;

        y *= m_g;

        *destP++ = y;

        // Update state variables
        x2 = x1;
        x1 = x;
        y2 = y1;
        y1 = y;
    }

    // Local variables back to member
    m_x1 = x1;
    m_x2 = x2;
    m_y1 = y1;
    m_y2 = y2;

    m_a0 = a0;
    m_a1 = a1;
    m_a2 = a2;
    m_b1 = b1;
    m_b2 = b2;
#endif
}

#if OS(DARWIN)

// Here we have optimized version using Accelerate.framework

void Biquad::processFast(const float* sourceP, float* destP, size_t framesToProcess)
{
    // Filter coefficients
    double B[5];
    B[0] = m_a0;
    B[1] = m_a1;
    B[2] = m_a2;
    B[3] = m_b1;
    B[4] = m_b2;

    double* inputP = m_inputBuffer.data();
    double* outputP = m_outputBuffer.data();

    double* input2P = inputP + 2;
    double* output2P = outputP + 2;

    // Break up processing into smaller slices (kBufferSize) if necessary.

    int n = framesToProcess;

    while (n > 0) {
        int framesThisTime = n < kBufferSize ? n : kBufferSize;

        // Copy input to input buffer
        for (int i = 0; i < framesThisTime; ++i)
            input2P[i] = *sourceP++;

        processSliceFast(inputP, outputP, B, framesThisTime);

        // Copy output buffer to output (converts float -> double).
        for (int i = 0; i < framesThisTime; ++i)
            *destP++ = static_cast<float>(output2P[i]);

        n -= framesThisTime;
    }
}

void Biquad::processSliceFast(double* sourceP, double* destP, double* coefficientsP, size_t framesToProcess)
{
    // Use double-precision for filter stability
    vDSP_deq22D(sourceP, 1, coefficientsP, destP, 1, framesToProcess);

    // Save history.  Note that sourceP and destP reference m_inputBuffer and m_outputBuffer respectively.
    // These buffers are allocated (in the constructor) with space for two extra samples so it's OK to access
    // array values two beyond framesToProcess.
    sourceP[0] = sourceP[framesToProcess - 2 + 2];
    sourceP[1] = sourceP[framesToProcess - 1 + 2];
    destP[0] = destP[framesToProcess - 2 + 2];
    destP[1] = destP[framesToProcess - 1 + 2];
}

#endif // OS(DARWIN)


void Biquad::reset()
{
    m_x1 = m_x2 = m_y1 = m_y2 = 0.0;

#if OS(DARWIN)
    // Two extra samples for filter history
    double* inputP = m_inputBuffer.data();
    inputP[0] = 0.0;
    inputP[1] = 0.0;

    double* outputP = m_outputBuffer.data();
    outputP[0] = 0.0;
    outputP[1] = 0.0;
#endif
}

void Biquad::setLowpassParams(double cutoff, double resonance)
{
    resonance = std::max(0.0, resonance); // can't go negative

    double g = pow(10.0, 0.05 * resonance);
    double d = sqrt((4.0 - sqrt(16.0 - 16.0 / (g * g))) / 2.0);

    // Compute biquad coefficients for lopass filter
    double theta = piDouble * cutoff;
    double sn = 0.5 * d * sin(theta);
    double beta = 0.5 * (1.0 - sn) / (1.0 + sn);
    double gamma = (0.5 + beta) * cos(theta);
    double alpha = 0.25 * (0.5 + beta - gamma);

    m_a0 = 2.0 * alpha;
    m_a1 = 2.0 * 2.0*alpha;
    m_a2 = 2.0 * alpha;
    m_b1 = 2.0 * -gamma;
    m_b2 = 2.0 * beta;
}

void Biquad::setHighpassParams(double cutoff, double resonance)
{
    resonance = std::max(0.0, resonance); // can't go negative

    double g = pow(10.0, 0.05 * resonance);
    double d = sqrt((4.0 - sqrt(16.0 - 16.0 / (g * g))) / 2.0);

    // Compute biquad coefficients for highpass filter
    double theta = piDouble * cutoff;
    double sn = 0.5 * d * sin(theta);
    double beta = 0.5 * (1.0 - sn) / (1.0 + sn);
    double gamma = (0.5 + beta) * cos(theta);
    double alpha = 0.25 * (0.5 + beta + gamma);

    m_a0 = 2.0 * alpha;
    m_a1 = 2.0 * -2.0*alpha;
    m_a2 = 2.0 * alpha;
    m_b1 = 2.0 * -gamma;
    m_b2 = 2.0 * beta;
}

void Biquad::setLowShelfParams(double cutoff, double dbGain)
{
    double theta = piDouble * cutoff;

    double A = pow(10.0, dbGain / 40.0);
    double S = 1.0; // filter slope (1.0 is max value)
    double alpha = 0.5 * sin(theta) * sqrt((A + 1.0 / A) * (1.0 / S - 1.0) + 2.0);

    double k = cos(theta);
    double k2 = 2.0 * sqrt(A) * alpha;

    double b0 = A * ((A + 1.0) - (A - 1.0) * k + k2);
    double b1 = 2.0 * A * ((A - 1.0) - (A + 1.0) * k);
    double b2 = A * ((A + 1.0) - (A - 1.0) * k - k2);
    double a0 = (A + 1.0) + (A - 1.0) * k + k2;
    double a1 = -2.0 * ((A - 1.0) + (A + 1.0) * k);
    double a2 = (A + 1.0) + (A - 1.0) * k - k2;

    double a0Inverse = 1.0 / a0;
    
    m_a0 = b0 * a0Inverse;
    m_a1 = b1 * a0Inverse;
    m_a2 = b2 * a0Inverse;
    m_b1 = a1 * a0Inverse;
    m_b2 = a2 * a0Inverse;
}

void Biquad::setZeroPolePairs(const Complex &zero, const Complex &pole)
{
    m_a0 = 1.0;
    m_a1 = -2.0 * zero.real();

    double zeroMag = abs(zero);
    m_a2 = zeroMag * zeroMag;

    m_b1 = -2.0 * pole.real();

    double poleMag = abs(pole);
    m_b2 = poleMag * poleMag;
}

void Biquad::setAllpassPole(const Complex &pole)
{
    Complex zero = Complex(1.0, 0.0) / pole;
    setZeroPolePairs(zero, pole);
}

} // namespace WebCore

#endif // ENABLE(WEB_AUDIO)
