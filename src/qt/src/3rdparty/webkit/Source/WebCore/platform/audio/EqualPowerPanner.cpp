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

#include "EqualPowerPanner.h"

#include "AudioBus.h"
#include "AudioUtilities.h"
#include <wtf/MathExtras.h>

// Use a 50ms smoothing / de-zippering time-constant.
const double SmoothingTimeConstant = 0.050;
 
namespace WebCore {

EqualPowerPanner::EqualPowerPanner(double sampleRate)
    : Panner(PanningModelEqualPower)
    , m_isFirstRender(true)
    , m_gainL(0.0)
    , m_gainR(0.0)
{
    m_smoothingConstant = AudioUtilities::discreteTimeConstantForSampleRate(SmoothingTimeConstant, sampleRate);
}

void EqualPowerPanner::pan(double azimuth, double /*elevation*/, AudioBus* inputBus, AudioBus* outputBus, size_t framesToProcess)
{
    // FIXME: implement stereo sources
    bool isInputSafe = inputBus && inputBus->numberOfChannels() == 1 && framesToProcess <= inputBus->length();
    ASSERT(isInputSafe);
    if (!isInputSafe)
        return;

    bool isOutputSafe = outputBus && outputBus->numberOfChannels() == 2 && framesToProcess <= outputBus->length();
    ASSERT(isOutputSafe);
    if (!isOutputSafe)
        return;

    AudioChannel* channel = inputBus->channel(0);
    float* sourceP = channel->data();                               
    float* destinationL = outputBus->channelByType(AudioBus::ChannelLeft)->data();
    float* destinationR = outputBus->channelByType(AudioBus::ChannelRight)->data();

    if (!sourceP || !destinationL || !destinationR)
        return;

    // Pan smoothly from left to right with azimuth going from -30 -> +30 degrees.
    double desiredPanPosition;
    if (azimuth > 30.0)
        desiredPanPosition = 1.0;
    else if (azimuth < -30.0)
        desiredPanPosition = 0.0;
    else
        desiredPanPosition = (azimuth + 30.0) / 60.0;

    double desiredGainL = 0.5 * cos(piDouble * desiredPanPosition) + 0.5;
    double desiredGainR = sqrt(1.0 - desiredGainL*desiredGainL);

    // Don't de-zipper on first render call.
    if (m_isFirstRender) {
        m_isFirstRender = false;
        m_gainL = desiredGainL;
        m_gainR = desiredGainR;
    }

    // Cache in local variables.
    double gainL = m_gainL;
    double gainR = m_gainR;

    // Get local copy of smoothing constant.
    const double SmoothingConstant = m_smoothingConstant;

    int n = framesToProcess;

    while (n--) {
        float input = *sourceP++;
        gainL += (desiredGainL - gainL) * SmoothingConstant;
        gainR += (desiredGainR - gainR) * SmoothingConstant;
        *destinationL++ = static_cast<float>(input * gainL);
        *destinationR++ = static_cast<float>(input * gainR);
    }

    m_gainL = gainL;
    m_gainR = gainR;
}

} // namespace WebCore

#endif // ENABLE(WEB_AUDIO)
