/*
 * Copyright (C) 2010, Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1.  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef HRTFPanner_h
#define HRTFPanner_h

#include "DelayDSPKernel.h"
#include "FFTConvolver.h"
#include "Panner.h"

namespace WebCore {

class HRTFPanner : public Panner {
public:
    explicit HRTFPanner(double sampleRate);
    virtual ~HRTFPanner();

    // Panner
    virtual void pan(double azimuth, double elevation, AudioBus* inputBus, AudioBus* outputBus, size_t framesToProcess);
    virtual void reset();

    size_t fftSize() { return fftSizeForSampleRate(m_sampleRate); }
    static size_t fftSizeForSampleRate(double sampleRate);

    double sampleRate() const { return m_sampleRate; }
    
private:
    // Given an azimuth angle in the range -180 -> +180, returns the corresponding azimuth index for the database,
    // and azimuthBlend which is an interpolation value from 0 -> 1.
    int calculateDesiredAzimuthIndexAndBlend(double azimuth, double& azimuthBlend);

    double m_sampleRate;
    
    // m_isFirstRender and m_azimuthIndex are used to avoid harshly changing from rendering at one azimuth angle to another angle very far away.
    // Changing the azimuth gradually produces a smoother sound.
    bool m_isFirstRender;
    int m_azimuthIndex;

    FFTConvolver m_convolverL;
    FFTConvolver m_convolverR;
    DelayDSPKernel m_delayLineL;
    DelayDSPKernel m_delayLineR;
};

} // namespace WebCore

#endif // HRTFPanner_h
