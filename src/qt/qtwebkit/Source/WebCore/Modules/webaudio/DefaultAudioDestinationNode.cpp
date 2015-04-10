/*
 * Copyright (C) 2011, Google Inc. All rights reserved.
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

#include "DefaultAudioDestinationNode.h"

#include "ExceptionCode.h"
#include "Logging.h"
#include <wtf/MainThread.h>

const unsigned EnabledInputChannels = 2;

namespace WebCore {
    
DefaultAudioDestinationNode::DefaultAudioDestinationNode(AudioContext* context)
    : AudioDestinationNode(context, AudioDestination::hardwareSampleRate())
    , m_numberOfInputChannels(0)
{
    // Node-specific default mixing rules.
    m_channelCount = 2;
    m_channelCountMode = Explicit;
    m_channelInterpretation = AudioBus::Speakers;
}

DefaultAudioDestinationNode::~DefaultAudioDestinationNode()
{
    uninitialize();
}

void DefaultAudioDestinationNode::initialize()
{
    ASSERT(isMainThread()); 
    if (isInitialized())
        return;

    createDestination();
    AudioNode::initialize();
}

void DefaultAudioDestinationNode::uninitialize()
{
    ASSERT(isMainThread()); 
    if (!isInitialized())
        return;

    m_destination->stop();
    m_numberOfInputChannels = 0;

    AudioNode::uninitialize();
}

void DefaultAudioDestinationNode::createDestination()
{
    float hardwareSampleRate = AudioDestination::hardwareSampleRate();
    LOG(WebAudio, ">>>> hardwareSampleRate = %f\n", hardwareSampleRate);
    
    m_destination = AudioDestination::create(*this, m_inputDeviceId, m_numberOfInputChannels, channelCount(), hardwareSampleRate);
}

void DefaultAudioDestinationNode::enableInput(const String& inputDeviceId)
{
    ASSERT(isMainThread());
    if (m_numberOfInputChannels != EnabledInputChannels) {
        m_numberOfInputChannels = EnabledInputChannels;
        m_inputDeviceId = inputDeviceId;

        if (isInitialized()) {
            // Re-create destination.
            m_destination->stop();
            createDestination();
            m_destination->start();
        }
    }
}

void DefaultAudioDestinationNode::startRendering()
{
    ASSERT(isInitialized());
    if (isInitialized())
        m_destination->start();
}

unsigned long DefaultAudioDestinationNode::maxChannelCount() const
{
    return AudioDestination::maxChannelCount();
}

void DefaultAudioDestinationNode::setChannelCount(unsigned long channelCount, ExceptionCode& ec)
{
    // The channelCount for the input to this node controls the actual number of channels we
    // send to the audio hardware. It can only be set depending on the maximum number of
    // channels supported by the hardware.

    ASSERT(isMainThread());

    if (!maxChannelCount() || channelCount > maxChannelCount()) {
        ec = INVALID_STATE_ERR;
        return;
    }

    unsigned long oldChannelCount = this->channelCount();
    AudioNode::setChannelCount(channelCount, ec);

    if (!ec && this->channelCount() != oldChannelCount && isInitialized()) {
        // Re-create destination.
        m_destination->stop();
        createDestination();
        m_destination->start();
    }
}

} // namespace WebCore

#endif // ENABLE(WEB_AUDIO)
