/*
 * Copyright (C) 2011 Google Inc. All rights reserved.
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

#ifndef DynamicsCompressorKernel_h
#define DynamicsCompressorKernel_h

#include "AudioArray.h"

#include <wtf/OwnPtr.h>
#include <wtf/PassOwnPtr.h>

namespace WebCore {

class DynamicsCompressorKernel {
public:
    DynamicsCompressorKernel(float sampleRate, unsigned numberOfChannels);

    void setNumberOfChannels(unsigned);

    // Performs stereo-linked compression.
    void process(float* sourceChannels[],
                 float* destinationChannels[],
                 unsigned numberOfChannels,
                 unsigned framesToProcess,

                 float dbThreshold,
                 float dbKnee,
                 float ratio,
                 float attackTime,
                 float releaseTime,
                 float preDelayTime,
                 float dbPostGain,
                 float effectBlend,

                 float releaseZone1,
                 float releaseZone2,
                 float releaseZone3,
                 float releaseZone4
                 );

    void reset();

    unsigned latencyFrames() const { return m_lastPreDelayFrames; }

    float sampleRate() const { return m_sampleRate; }

    float meteringGain() const { return m_meteringGain; }

protected:
    float m_sampleRate;

    float m_detectorAverage;
    float m_compressorGain;

    // Metering
    float m_meteringReleaseK;
    float m_meteringGain;

    // Lookahead section.
    enum { MaxPreDelayFrames = 1024 };
    enum { MaxPreDelayFramesMask = MaxPreDelayFrames - 1 };
    enum { DefaultPreDelayFrames = 256 }; // setPreDelayTime() will override this initial value
    unsigned m_lastPreDelayFrames;
    void setPreDelayTime(float);

    Vector<OwnPtr<AudioFloatArray> > m_preDelayBuffers;
    int m_preDelayReadIndex;
    int m_preDelayWriteIndex;

    float m_maxAttackCompressionDiffDb;

    // Static compression curve.
    float kneeCurve(float x, float k);
    float saturate(float x, float k);
    float slopeAt(float x, float k);
    float kAtSlope(float desiredSlope);

    float updateStaticCurveParameters(float dbThreshold, float dbKnee, float ratio);

    // Amount of input change in dB required for 1 dB of output change.
    // This applies to the portion of the curve above m_kneeThresholdDb (see below).
    float m_ratio;
    float m_slope; // Inverse ratio.

    // The input to output change below the threshold is linear 1:1.
    float m_linearThreshold;
    float m_dbThreshold;

    // m_dbKnee is the number of dB above the threshold before we enter the "ratio" portion of the curve.
    // m_kneeThresholdDb = m_dbThreshold + m_dbKnee
    // The portion between m_dbThreshold and m_kneeThresholdDb is the "soft knee" portion of the curve
    // which transitions smoothly from the linear portion to the ratio portion.
    float m_dbKnee;
    float m_kneeThreshold;
    float m_kneeThresholdDb;
    float m_ykneeThresholdDb;

    // Internal parameter for the knee portion of the curve.
    float m_K;
};

} // namespace WebCore

#endif // DynamicsCompressorKernel_h
