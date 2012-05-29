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

HRTFPanner::HRTFPanner(double sampleRate)
    : Panner(PanningModelHRTF)
    , m_sampleRate(sampleRate)
    , m_isFirstRender(true)
    , m_azimuthIndex(0)
    , m_convolverL(fftSizeForSampleRate(sampleRate))
    , m_convolverR(fftSizeForSampleRate(sampleRate))
    , m_delayLineL(MaxDelayTimeSeconds, sampleRate)
    , m_delayLineR(MaxDelayTimeSeconds, sampleRate)
{ 
}

HRTFPanner::~HRTFPanner()
{
}

size_t HRTFPanner::fftSizeForSampleRate(double sampleRate)
{
    // The HRTF impulse responses (loaded as audio resources) are 512 sample-frames @44.1KHz.
    // Currently, we truncate the impulse responses to half this size, but an FFT-size of twice impulse response size is needed (for convolution).
    // So for sample rates around 44.1KHz an FFT size of 512 is good.  We double that size for higher sample rates.
    ASSERT(sampleRate >= 44100 && sampleRate <= 96000.0);
    return (sampleRate <= 48000.0) ? 512 : 1024;
}

void HRTFPanner::reset()
{
    m_isFirstRender = true;
    m_convolverL.reset();
    m_convolverR.reset();
    m_delayLineL.reset();
    m_delayLineR.reset();
}

static bool wrapDistance(int i, int j, int length)
{
    int directDistance = abs(i - j);
    int indirectDistance = length - directDistance;

    return indirectDistance < directDistance;
}

int HRTFPanner::calculateDesiredAzimuthIndexAndBlend(double azimuth, double& azimuthBlend)
{
    // Convert the azimuth angle from the range -180 -> +180 into the range 0 -> 360.
    // The azimuth index may then be calculated from this positive value.
    if (azimuth < 0)
        azimuth += 360.0;
    
    HRTFDatabase* database = HRTFDatabaseLoader::defaultHRTFDatabase();    
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

void HRTFPanner::pan(double desiredAzimuth, double elevation, AudioBus* inputBus, AudioBus* outputBus, size_t framesToProcess)
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
    HRTFDatabase* database = HRTFDatabaseLoader::defaultHRTFDatabase();    
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
    AudioChannel* inputChannelL = inputBus->channelByType(AudioBus::ChannelLeft);
    AudioChannel* inputChannelR = numInputChannels > 1 ? inputBus->channelByType(AudioBus::ChannelRight) : 0;

    // Get source and destination pointers.
    float* sourceL = inputChannelL->data();
    float* sourceR = numInputChannels > 1 ? inputChannelR->data() : sourceL;
    float* destinationL = outputBus->channelByType(AudioBus::ChannelLeft)->data();
    float* destinationR = outputBus->channelByType(AudioBus::ChannelRight)->data();

    double azimuthBlend;
    int desiredAzimuthIndex = calculateDesiredAzimuthIndexAndBlend(azimuth, azimuthBlend);

    // This algorithm currently requires that we process in power-of-two size chunks at least 128.
    ASSERT(1UL << static_cast<int>(log2(framesToProcess)) == framesToProcess);
    ASSERT(framesToProcess >= 128);
    
    const unsigned framesPerSegment = 128;
    const unsigned numberOfSegments = framesToProcess / framesPerSegment;

    for (unsigned segment = 0; segment < numberOfSegments; ++segment) {
        if (m_isFirstRender) {
            // Snap exactly to desired position (first time and after reset()).
            m_azimuthIndex = desiredAzimuthIndex;
            m_isFirstRender = false;
        } else {
            // Each segment renders with an azimuth index closer by one to the desired azimuth index.
            // Because inter-aural time delay is mostly a factor of azimuth and the delay is where the clicks and graininess come from,
            // we don't bother smoothing the elevations.
            int numberOfAzimuths = database->numberOfAzimuths();
            bool wrap = wrapDistance(m_azimuthIndex, desiredAzimuthIndex, numberOfAzimuths);
            if (wrap) {
                if (m_azimuthIndex < desiredAzimuthIndex)
                    m_azimuthIndex = (m_azimuthIndex - 1 + numberOfAzimuths) % numberOfAzimuths;
                else if (m_azimuthIndex > desiredAzimuthIndex)
                    m_azimuthIndex = (m_azimuthIndex + 1) % numberOfAzimuths;
            } else {
                if (m_azimuthIndex < desiredAzimuthIndex)
                    m_azimuthIndex = (m_azimuthIndex + 1) % numberOfAzimuths;
                else if (m_azimuthIndex > desiredAzimuthIndex)
                    m_azimuthIndex = (m_azimuthIndex - 1 + numberOfAzimuths) % numberOfAzimuths;
            }
        }
        
        // Get the HRTFKernels and interpolated delays.    
        HRTFKernel* kernelL;
        HRTFKernel* kernelR;
        double frameDelayL;
        double frameDelayR;
        database->getKernelsFromAzimuthElevation(azimuthBlend, m_azimuthIndex, elevation, kernelL, kernelR, frameDelayL, frameDelayR);

        ASSERT(kernelL && kernelR);
        if (!kernelL || !kernelR) {
            outputBus->zero();
            return;
        }
        
        ASSERT(frameDelayL / sampleRate() < MaxDelayTimeSeconds && frameDelayR / sampleRate() < MaxDelayTimeSeconds);
            
        // Calculate the source and destination pointers for the current segment.
        unsigned offset = segment * framesPerSegment;
        float* segmentSourceL = sourceL + offset;
        float* segmentSourceR = sourceR + offset;
        float* segmentDestinationL = destinationL + offset;
        float* segmentDestinationR = destinationR + offset;

        // First run through delay lines for inter-aural time difference.
        m_delayLineL.setDelayFrames(frameDelayL);
        m_delayLineR.setDelayFrames(frameDelayR);
        m_delayLineL.process(segmentSourceL, segmentDestinationL, framesPerSegment);
        m_delayLineR.process(segmentSourceR, segmentDestinationR, framesPerSegment);

        // Now do the convolutions in-place.
        m_convolverL.process(kernelL->fftFrame(), segmentDestinationL, segmentDestinationL, framesPerSegment);
        m_convolverR.process(kernelR->fftFrame(), segmentDestinationR, segmentDestinationR, framesPerSegment);
    }
}

} // namespace WebCore

#endif // ENABLE(WEB_AUDIO)
