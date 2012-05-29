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

#include "DelayDSPKernel.h"

#include "AudioUtilities.h"
#include <algorithm>

using namespace std;
  
const double DefaultMaxDelayTime = 1.0;
const double SmoothingTimeConstant = 0.020; // 20ms
  
namespace WebCore {

DelayDSPKernel::DelayDSPKernel(DelayProcessor* processor)
    : AudioDSPKernel(processor)
    , m_maxDelayTime(DefaultMaxDelayTime)
    , m_writeIndex(0)
    , m_firstTime(true)
{
    ASSERT(processor && processor->sampleRate() > 0);
    if (!processor)
        return;

    m_buffer.resize(static_cast<size_t>(processor->sampleRate() * DefaultMaxDelayTime));
    m_buffer.zero();

    m_smoothingRate = AudioUtilities::discreteTimeConstantForSampleRate(SmoothingTimeConstant, processor->sampleRate());
}

DelayDSPKernel::DelayDSPKernel(double maxDelayTime, double sampleRate)
    : AudioDSPKernel(sampleRate)
    , m_maxDelayTime(maxDelayTime)
    , m_writeIndex(0)
    , m_firstTime(true)
{
    ASSERT(maxDelayTime > 0.0);
    if (maxDelayTime <= 0.0)
        return;
        
    size_t bufferLength = static_cast<size_t>(sampleRate * maxDelayTime);
    ASSERT(bufferLength);
    if (!bufferLength)
        return;
    
    m_buffer.resize(bufferLength);
    m_buffer.zero();

    m_smoothingRate = AudioUtilities::discreteTimeConstantForSampleRate(SmoothingTimeConstant, sampleRate);
}

void DelayDSPKernel::process(const float* source, float* destination, size_t framesToProcess)
{
    size_t bufferLength = m_buffer.size();
    float* buffer = m_buffer.data();

    ASSERT(bufferLength);
    if (!bufferLength)
        return;
        
    ASSERT(source && destination);
    if (!source || !destination)
        return;
        
    double sampleRate = this->sampleRate();
    double delayTime = delayProcessor() ? delayProcessor()->delayTime()->value() : m_desiredDelayFrames / sampleRate;

    // Make sure the delay time is in a valid range.
    delayTime = min(maxDelayTime(), delayTime);
    delayTime = max(0.0, delayTime);

    if (m_firstTime) {
        m_currentDelayTime = delayTime;
        m_firstTime = false;
    }
    
    int n = framesToProcess;
    while (n--) {
        // Approach desired delay time.
        m_currentDelayTime += (delayTime - m_currentDelayTime) * m_smoothingRate;

        double desiredDelayFrames = m_currentDelayTime * sampleRate;

        double readPosition = m_writeIndex + bufferLength - desiredDelayFrames;
        if (readPosition > bufferLength)
            readPosition -= bufferLength;

        // Linearly interpolate in-between delay times.
        int readIndex1 = static_cast<int>(readPosition);
        int readIndex2 = (readIndex1 + 1) % bufferLength;
        double interpolationFactor = readPosition - readIndex1;

        double input = static_cast<float>(*source++);
        buffer[m_writeIndex] = static_cast<float>(input);
        m_writeIndex = (m_writeIndex + 1) % bufferLength;        
        
        double sample1 = buffer[readIndex1];
        double sample2 = buffer[readIndex2];
        
        double output = (1.0 - interpolationFactor) * sample1 + interpolationFactor * sample2;

        *destination++ = static_cast<float>(output);
    }        
}

void DelayDSPKernel::reset()
{
    m_firstTime = true;
    m_buffer.zero();
}

} // namespace WebCore

#endif // ENABLE(WEB_AUDIO)
