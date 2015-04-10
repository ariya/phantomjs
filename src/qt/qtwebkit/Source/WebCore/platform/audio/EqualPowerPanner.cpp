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
#include <algorithm>
#include <wtf/MathExtras.h>

// Use a 50ms smoothing / de-zippering time-constant.
const float SmoothingTimeConstant = 0.050f;

using namespace std;

namespace WebCore {

EqualPowerPanner::EqualPowerPanner(float sampleRate)
    : Panner(PanningModelEqualPower)
    , m_isFirstRender(true)
    , m_gainL(0.0)
    , m_gainR(0.0)
{
    m_smoothingConstant = AudioUtilities::discreteTimeConstantForSampleRate(SmoothingTimeConstant, sampleRate);
}

void EqualPowerPanner::pan(double azimuth, double /*elevation*/, const AudioBus* inputBus, AudioBus* outputBus, size_t framesToProcess)
{
    bool isInputSafe = inputBus && (inputBus->numberOfChannels() == 1 || inputBus->numberOfChannels() == 2) && framesToProcess <= inputBus->length();
    ASSERT(isInputSafe);
    if (!isInputSafe)
        return;

    unsigned numberOfInputChannels = inputBus->numberOfChannels();

    bool isOutputSafe = outputBus && outputBus->numberOfChannels() == 2 && framesToProcess <= outputBus->length();
    ASSERT(isOutputSafe);
    if (!isOutputSafe)
        return;

    const float* sourceL = inputBus->channel(0)->data();                               
    const float* sourceR = numberOfInputChannels > 1 ? inputBus->channel(1)->data() : sourceL;
    float* destinationL = outputBus->channelByType(AudioBus::ChannelLeft)->mutableData();
    float* destinationR = outputBus->channelByType(AudioBus::ChannelRight)->mutableData();
    
    if (!sourceL || !sourceR || !destinationL || !destinationR)
        return;
    
    // Clamp azimuth to allowed range of -180 -> +180.
    azimuth = max(-180.0, azimuth);
    azimuth = min(180.0, azimuth);
    
    // Alias the azimuth ranges behind us to in front of us:
    // -90 -> -180 to -90 -> 0 and 90 -> 180 to 90 -> 0
    if (azimuth < -90)
        azimuth = -180 - azimuth;
    else if (azimuth > 90)
        azimuth = 180 - azimuth;

    double desiredPanPosition;
    double desiredGainL;
    double desiredGainR;

    if (numberOfInputChannels == 1) { // For mono source case.
        // Pan smoothly from left to right with azimuth going from -90 -> +90 degrees.
        desiredPanPosition = (azimuth + 90) / 180;
    } else { // For stereo source case.
        if (azimuth <= 0) { // from -90 -> 0
            // sourceL -> destL and "equal-power pan" sourceR as in mono case
            // by transforming the "azimuth" value from -90 -> 0 degrees into the range -90 -> +90.
            desiredPanPosition = (azimuth + 90) / 90;
        } else { // from 0 -> +90
            // sourceR -> destR and "equal-power pan" sourceL as in mono case
            // by transforming the "azimuth" value from 0 -> +90 degrees into the range -90 -> +90.
            desiredPanPosition = azimuth / 90;
        }
    }

    desiredGainL = cos(0.5 * piDouble * desiredPanPosition);
    desiredGainR = sin(0.5 * piDouble * desiredPanPosition);
   
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

    if (numberOfInputChannels == 1) { // For mono source case.
        while (n--) {
            float inputL = *sourceL++;
            gainL += (desiredGainL - gainL) * SmoothingConstant;
            gainR += (desiredGainR - gainR) * SmoothingConstant;
            *destinationL++ = static_cast<float>(inputL * gainL);
            *destinationR++ = static_cast<float>(inputL * gainR);
        }
    } else { // For stereo source case.
        if (azimuth <= 0) { // from -90 -> 0
            while (n--) {
                float inputL = *sourceL++;
                float inputR = *sourceR++;
                gainL += (desiredGainL - gainL) * SmoothingConstant;
                gainR += (desiredGainR - gainR) * SmoothingConstant;
                *destinationL++ = static_cast<float>(inputL + inputR * gainL);
                *destinationR++ = static_cast<float>(inputR * gainR);
            }
        } else { // from 0 -> +90
            while (n--) {
                float inputL = *sourceL++;
                float inputR = *sourceR++;
                gainL += (desiredGainL - gainL) * SmoothingConstant;
                gainR += (desiredGainR - gainR) * SmoothingConstant;
                *destinationL++ = static_cast<float>(inputL * gainL);
                *destinationR++ = static_cast<float>(inputR + inputL * gainR);
            }
        }
    }

    m_gainL = gainL;
    m_gainR = gainR;
}

} // namespace WebCore

#endif // ENABLE(WEB_AUDIO)
