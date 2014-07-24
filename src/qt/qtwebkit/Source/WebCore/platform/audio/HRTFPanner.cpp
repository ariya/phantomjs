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

#include "config.h"

#if ENABLE(WEB_AUDIO)

#include "HRTFPanner.h"

#include "AudioBus.h"
#include "FFTConvolver.h"
#include "HRTFDatabase.h"
#include "HRTFDatabaseLoader.h"
#include <algorithm>
#include <wtf/MathExtras.h>
#include <wtf/RefPtr.h>

using namespace std;

namespace WebCore {

// The value of 2 milliseconds is larger than the largest delay which exists in any HRTFKernel from the default HRTFDatabase (0.0136 seconds).
// We ASSERT the delay values used in process() with this value.
const double MaxDelayTimeSeconds = 0.002;

const int UninitializedAzimuth = -1;
const unsigned RenderingQuantum = 128;

HRTFPanner::HRTFPanner(float sampleRate, HRTFDatabaseLoader* databaseLoader)
    : Panner(PanningModelHRTF)
    , m_databaseLoader(databaseLoader)
    , m_sampleRate(sampleRate)
    , m_crossfadeSelection(CrossfadeSelection1)
    , m_azimuthIndex1(UninitializedAzimuth)
    , m_elevation1(0)
    , m_azimuthIndex2(UninitializedAzimuth)
    , m_elevation2(0)
    , m_crossfadeX(0)
    , m_crossfadeIncr(0)
    , m_convolverL1(fftSizeForSampleRate(sampleRate))
    , m_convolverR1(fftSizeForSampleRate(sampleRate))
    , m_convolverL2(fftSizeForSampleRate(sampleRate))
    , m_convolverR2(fftSizeForSampleRate(sampleRate))
    , m_delayLineL(MaxDelayTimeSeconds, sampleRate)
    , m_delayLineR(MaxDelayTimeSeconds, sampleRate)
    , m_tempL1(RenderingQuantum)
    , m_tempR1(RenderingQuantum)
    , m_tempL2(RenderingQuantum)
    , m_tempR2(RenderingQuantum)
{
    ASSERT(databaseLoader);
}

HRTFPanner::~HRTFPanner()
{
}

size_t HRTFPanner::fftSizeForSampleRate(float sampleRate)
{
    // The HRTF impulse responses (loaded as audio resources) are 512 sample-frames @44.1KHz.
    // Currently, we truncate the impulse responses to half this size, but an FFT-size of twice impulse response size is needed (for convolution).
    // So for sample rates around 44.1KHz an FFT size of 512 is good. We double the FFT-size only for sample rates at least double this.
    ASSERT(sampleRate >= 44100 && sampleRate <= 96000.0);
    return (sampleRate < 88200.0) ? 512 : 1024;
}

void HRTFPanner::reset()
{
    m_convolverL1.reset();
    m_convolverR1.reset();
    m_convolverL2.reset();
    m_convolverR2.reset();
    m_delayLineL.reset();
    m_delayLineR.reset();
}

int HRTFPanner::calculateDesiredAzimuthIndexAndBlend(double azimuth, double& azimuthBlend)
{
    // Convert the azimuth angle from the range -180 -> +180 into the range 0 -> 360.
    // The azimuth index may then be calculated from this positive value.
    if (azimuth < 0)
        azimuth += 360.0;

    HRTFDatabase* database = m_databaseLoader->database();
    ASSERT(database);

    int numberOfAzimuths = database->numberOfAzimuths();
    const double angleBetweenAzimuths = 360.0 / numberOfAzimuths;

    // Calculate the azimuth index and the blend (0 -> 1) for interpolation.
    double desiredAzimuthIndexFloat = azimuth / angleBetweenAzimuths;
    int desiredAzimuthIndex = static_cast<int>(desiredAzimuthIndexFloat);
    azimuthBlend = desiredAzimuthIndexFloat - static_cast<double>(desiredAzimuthIndex);

    // We don't immediately start using this azimuth index, but instead approach this index from the last index we rendered at.
    // This minimizes the clicks and graininess for moving sources which occur otherwise.
    desiredAzimuthIndex = max(0, desiredAzimuthIndex);
    desiredAzimuthIndex = min(numberOfAzimuths - 1, desiredAzimuthIndex);
    return desiredAzimuthIndex;
}

void HRTFPanner::pan(double desiredAzimuth, double elevation, const AudioBus* inputBus, AudioBus* outputBus, size_t framesToProcess)
{
    unsigned numInputChannels = inputBus ? inputBus->numberOfChannels() : 0;

    bool isInputGood = inputBus &&  numInputChannels >= 1 && numInputChannels <= 2;
    ASSERT(isInputGood);

    bool isOutputGood = outputBus && outputBus->numberOfChannels() == 2 && framesToProcess <= outputBus->length();
    ASSERT(isOutputGood);

    if (!isInputGood || !isOutputGood) {
        if (outputBus)
            outputBus->zero();
        return;
    }

    // This code only runs as long as the context is alive and after database has been loaded.
    HRTFDatabase* database = m_databaseLoader->database();
    ASSERT(database);
    if (!database) {
        outputBus->zero();
        return;
    }

    // IRCAM HRTF azimuths values from the loaded database is reversed from the panner's notion of azimuth.
    double azimuth = -desiredAzimuth;

    bool isAzimuthGood = azimuth >= -180.0 && azimuth <= 180.0;
    ASSERT(isAzimuthGood);
    if (!isAzimuthGood) {
        outputBus->zero();
        return;
    }

    // Normally, we'll just be dealing with mono sources.
    // If we have a stereo input, implement stereo panning with left source processed by left HRTF, and right source by right HRTF.
    const AudioChannel* inputChannelL = inputBus->channelByType(AudioBus::ChannelLeft);
    const AudioChannel* inputChannelR = numInputChannels > 1 ? inputBus->channelByType(AudioBus::ChannelRight) : 0;

    // Get source and destination pointers.
    const float* sourceL = inputChannelL->data();
    const float* sourceR = numInputChannels > 1 ? inputChannelR->data() : sourceL;
    float* destinationL = outputBus->channelByType(AudioBus::ChannelLeft)->mutableData();
    float* destinationR = outputBus->channelByType(AudioBus::ChannelRight)->mutableData();

    double azimuthBlend;
    int desiredAzimuthIndex = calculateDesiredAzimuthIndexAndBlend(azimuth, azimuthBlend);

    // Initially snap azimuth and elevation values to first values encountered.
    if (m_azimuthIndex1 == UninitializedAzimuth) {
        m_azimuthIndex1 = desiredAzimuthIndex;
        m_elevation1 = elevation;
    }
    if (m_azimuthIndex2 == UninitializedAzimuth) {
        m_azimuthIndex2 = desiredAzimuthIndex;
        m_elevation2 = elevation;
    }

    // Cross-fade / transition over a period of around 45 milliseconds.
    // This is an empirical value tuned to be a reasonable trade-off between
    // smoothness and speed.
    const double fadeFrames = sampleRate() <= 48000 ? 2048 : 4096;

    // Check for azimuth and elevation changes, initiating a cross-fade if needed.
    if (!m_crossfadeX && m_crossfadeSelection == CrossfadeSelection1) {
        if (desiredAzimuthIndex != m_azimuthIndex1 || elevation != m_elevation1) {
            // Cross-fade from 1 -> 2
            m_crossfadeIncr = 1 / fadeFrames;
            m_azimuthIndex2 = desiredAzimuthIndex;
            m_elevation2 = elevation;
        }
    }
    if (m_crossfadeX == 1 && m_crossfadeSelection == CrossfadeSelection2) {
        if (desiredAzimuthIndex != m_azimuthIndex2 || elevation != m_elevation2) {
            // Cross-fade from 2 -> 1
            m_crossfadeIncr = -1 / fadeFrames;
            m_azimuthIndex1 = desiredAzimuthIndex;
            m_elevation1 = elevation;
        }
    }

    // This algorithm currently requires that we process in power-of-two size chunks at least RenderingQuantum.
    ASSERT(1UL << static_cast<int>(log2(framesToProcess)) == framesToProcess);
    ASSERT(framesToProcess >= RenderingQuantum);

    const unsigned framesPerSegment = RenderingQuantum;
    const unsigned numberOfSegments = framesToProcess / framesPerSegment;

    for (unsigned segment = 0; segment < numberOfSegments; ++segment) {
        // Get the HRTFKernels and interpolated delays.
        HRTFKernel* kernelL1;
        HRTFKernel* kernelR1;
        HRTFKernel* kernelL2;
        HRTFKernel* kernelR2;
        double frameDelayL1;
        double frameDelayR1;
        double frameDelayL2;
        double frameDelayR2;
        database->getKernelsFromAzimuthElevation(azimuthBlend, m_azimuthIndex1, m_elevation1, kernelL1, kernelR1, frameDelayL1, frameDelayR1);
        database->getKernelsFromAzimuthElevation(azimuthBlend, m_azimuthIndex2, m_elevation2, kernelL2, kernelR2, frameDelayL2, frameDelayR2);

        bool areKernelsGood = kernelL1 && kernelR1 && kernelL2 && kernelR2;
        ASSERT(areKernelsGood);
        if (!areKernelsGood) {
            outputBus->zero();
            return;
        }

        ASSERT(frameDelayL1 / sampleRate() < MaxDelayTimeSeconds && frameDelayR1 / sampleRate() < MaxDelayTimeSeconds);
        ASSERT(frameDelayL2 / sampleRate() < MaxDelayTimeSeconds && frameDelayR2 / sampleRate() < MaxDelayTimeSeconds);

        // Crossfade inter-aural delays based on transitions.
        double frameDelayL = (1 - m_crossfadeX) * frameDelayL1 + m_crossfadeX * frameDelayL2;
        double frameDelayR = (1 - m_crossfadeX) * frameDelayR1 + m_crossfadeX * frameDelayR2;

        // Calculate the source and destination pointers for the current segment.
        unsigned offset = segment * framesPerSegment;
        const float* segmentSourceL = sourceL + offset;
        const float* segmentSourceR = sourceR + offset;
        float* segmentDestinationL = destinationL + offset;
        float* segmentDestinationR = destinationR + offset;

        // First run through delay lines for inter-aural time difference.
        m_delayLineL.setDelayFrames(frameDelayL);
        m_delayLineR.setDelayFrames(frameDelayR);
        m_delayLineL.process(segmentSourceL, segmentDestinationL, framesPerSegment);
        m_delayLineR.process(segmentSourceR, segmentDestinationR, framesPerSegment);

        bool needsCrossfading = m_crossfadeIncr;
        
        // Have the convolvers render directly to the final destination if we're not cross-fading.
        float* convolutionDestinationL1 = needsCrossfading ? m_tempL1.data() : segmentDestinationL;
        float* convolutionDestinationR1 = needsCrossfading ? m_tempR1.data() : segmentDestinationR;
        float* convolutionDestinationL2 = needsCrossfading ? m_tempL2.data() : segmentDestinationL;
        float* convolutionDestinationR2 = needsCrossfading ? m_tempR2.data() : segmentDestinationR;

        // Now do the convolutions.
        // Note that we avoid doing convolutions on both sets of convolvers if we're not currently cross-fading.
        
        if (m_crossfadeSelection == CrossfadeSelection1 || needsCrossfading) {
            m_convolverL1.process(kernelL1->fftFrame(), segmentDestinationL, convolutionDestinationL1, framesPerSegment);
            m_convolverR1.process(kernelR1->fftFrame(), segmentDestinationR, convolutionDestinationR1, framesPerSegment);
        }

        if (m_crossfadeSelection == CrossfadeSelection2 || needsCrossfading) {
            m_convolverL2.process(kernelL2->fftFrame(), segmentDestinationL, convolutionDestinationL2, framesPerSegment);
            m_convolverR2.process(kernelR2->fftFrame(), segmentDestinationR, convolutionDestinationR2, framesPerSegment);
        }
        
        if (needsCrossfading) {
            // Apply linear cross-fade.
            float x = m_crossfadeX;
            float incr = m_crossfadeIncr;
            for (unsigned i = 0; i < framesPerSegment; ++i) {
                segmentDestinationL[i] = (1 - x) * convolutionDestinationL1[i] + x * convolutionDestinationL2[i];
                segmentDestinationR[i] = (1 - x) * convolutionDestinationR1[i] + x * convolutionDestinationR2[i];
                x += incr;
            }
            // Update cross-fade value from local.
            m_crossfadeX = x;

            if (m_crossfadeIncr > 0 && fabs(m_crossfadeX - 1) < m_crossfadeIncr) {
                // We've fully made the crossfade transition from 1 -> 2.
                m_crossfadeSelection = CrossfadeSelection2;
                m_crossfadeX = 1;
                m_crossfadeIncr = 0;
            } else if (m_crossfadeIncr < 0 && fabs(m_crossfadeX) < -m_crossfadeIncr) {
                // We've fully made the crossfade transition from 2 -> 1.
                m_crossfadeSelection = CrossfadeSelection1;
                m_crossfadeX = 0;
                m_crossfadeIncr = 0;
            }
        }
    }
}

double HRTFPanner::tailTime() const
{
    // Because HRTFPanner is implemented with a DelayKernel and a FFTConvolver, the tailTime of the HRTFPanner
    // is the sum of the tailTime of the DelayKernel and the tailTime of the FFTConvolver, which is MaxDelayTimeSeconds
    // and fftSize() / 2, respectively.
    return MaxDelayTimeSeconds + (fftSize() / 2) / static_cast<double>(sampleRate());
}

double HRTFPanner::latencyTime() const
{
    // The latency of a FFTConvolver is also fftSize() / 2, and is in addition to its tailTime of the
    // same value.
    return (fftSize() / 2) / static_cast<double>(sampleRate());
}

} // namespace WebCore

#endif // ENABLE(WEB_AUDIO)
