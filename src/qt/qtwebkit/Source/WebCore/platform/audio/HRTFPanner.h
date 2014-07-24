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
#include "HRTFDatabaseLoader.h"
#include "Panner.h"

namespace WebCore {

class HRTFPanner : public Panner {
public:
    explicit HRTFPanner(float sampleRate, HRTFDatabaseLoader*);
    virtual ~HRTFPanner();

    // Panner
    virtual void pan(double azimuth, double elevation, const AudioBus* inputBus, AudioBus* outputBus, size_t framesToProcess);
    virtual void reset();

    size_t fftSize() const { return fftSizeForSampleRate(m_sampleRate); }
    static size_t fftSizeForSampleRate(float sampleRate);

    float sampleRate() const { return m_sampleRate; }

    virtual double tailTime() const OVERRIDE;
    virtual double latencyTime() const OVERRIDE;

private:
    // Given an azimuth angle in the range -180 -> +180, returns the corresponding azimuth index for the database,
    // and azimuthBlend which is an interpolation value from 0 -> 1.
    int calculateDesiredAzimuthIndexAndBlend(double azimuth, double& azimuthBlend);

    RefPtr<HRTFDatabaseLoader> m_databaseLoader;

    float m_sampleRate;

    // We maintain two sets of convolvers for smooth cross-faded interpolations when
    // then azimuth and elevation are dynamically changing.
    // When the azimuth and elevation are not changing, we simply process with one of the two sets.
    // Initially we use CrossfadeSelection1 corresponding to m_convolverL1 and m_convolverR1.
    // Whenever the azimuth or elevation changes, a crossfade is initiated to transition
    // to the new position. So if we're currently processing with CrossfadeSelection1, then
    // we transition to CrossfadeSelection2 (and vice versa).
    // If we're in the middle of a transition, then we wait until it is complete before
    // initiating a new transition.

    // Selects either the convolver set (m_convolverL1, m_convolverR1) or (m_convolverL2, m_convolverR2).
    enum CrossfadeSelection {
        CrossfadeSelection1,
        CrossfadeSelection2
    };

    CrossfadeSelection m_crossfadeSelection;

    // azimuth/elevation for CrossfadeSelection1.
    int m_azimuthIndex1;
    double m_elevation1;

    // azimuth/elevation for CrossfadeSelection2.
    int m_azimuthIndex2;
    double m_elevation2;

    // A crossfade value 0 <= m_crossfadeX <= 1.
    float m_crossfadeX;

    // Per-sample-frame crossfade value increment.
    float m_crossfadeIncr;

    FFTConvolver m_convolverL1;
    FFTConvolver m_convolverR1;
    FFTConvolver m_convolverL2;
    FFTConvolver m_convolverR2;

    DelayDSPKernel m_delayLineL;
    DelayDSPKernel m_delayLineR;

    AudioFloatArray m_tempL1;
    AudioFloatArray m_tempR1;
    AudioFloatArray m_tempL2;
    AudioFloatArray m_tempR2;
};

} // namespace WebCore

#endif // HRTFPanner_h
