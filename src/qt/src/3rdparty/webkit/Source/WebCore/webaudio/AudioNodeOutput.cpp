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

#include "AudioNodeOutput.h"

#include "AudioBus.h"
#include "AudioContext.h"
#include "AudioNodeInput.h"
#include <wtf/Threading.h>

namespace WebCore {

AudioNodeOutput::AudioNodeOutput(AudioNode* node, unsigned numberOfChannels)
    : m_node(node)
    , m_numberOfChannels(numberOfChannels)
    , m_desiredNumberOfChannels(numberOfChannels)
    , m_internalOutputBus(0)
    , m_actualDestinationBus(0)
    , m_isEnabled(true)
    , m_renderingFanOutCount(0)
{
    m_monoInternalBus = adoptPtr(new AudioBus(1, AudioNode::ProcessingSizeInFrames));
    m_stereoInternalBus = adoptPtr(new AudioBus(2, AudioNode::ProcessingSizeInFrames));
    setInternalBus();
}

void AudioNodeOutput::setNumberOfChannels(unsigned numberOfChannels)
{
    ASSERT(context()->isGraphOwner());
    
    m_desiredNumberOfChannels = numberOfChannels;

    if (context()->isAudioThread()) {
        // If we're in the audio thread then we can take care of it right away (we should be at the very start or end of a rendering quantum).
        updateNumberOfChannels();
    } else {
        // Let the context take care of it in the audio thread in the pre and post render tasks.
        context()->markAudioNodeOutputDirty(this);
    }
}

void AudioNodeOutput::setInternalBus()
{
    switch (m_numberOfChannels) {
    case 0:
    case 1:
        m_internalOutputBus = m_monoInternalBus.get();
        break;
    case 2:
        m_internalOutputBus = m_stereoInternalBus.get();
        break;
    default:
        // FIXME: later we can fully implement more than stereo, 5.1, etc.
        ASSERT_NOT_REACHED();            
    }

    // This may later be changed in pull() to point to an in-place bus with the same number of channels.
    m_actualDestinationBus = m_internalOutputBus;
}

void AudioNodeOutput::updateRenderingState()
{
    updateNumberOfChannels();
    m_renderingFanOutCount = fanOutCount();
}

void AudioNodeOutput::updateNumberOfChannels()
{
    ASSERT(context()->isAudioThread() && context()->isGraphOwner());

    if (m_numberOfChannels != m_desiredNumberOfChannels) {
        m_numberOfChannels = m_desiredNumberOfChannels;
        setInternalBus();
        propagateChannelCount();
    }
}

void AudioNodeOutput::propagateChannelCount()
{
    ASSERT(context()->isAudioThread() && context()->isGraphOwner());
    
    if (isChannelCountKnown()) {
        // Announce to any nodes we're connected to that we changed our channel count for its input.
        for (InputsIterator i = m_inputs.begin(); i != m_inputs.end(); ++i) {
            AudioNodeInput* input = *i;
            AudioNode* connectionNode = input->node();
            connectionNode->checkNumberOfChannelsForInput(input);
        }
    }
}

AudioBus* AudioNodeOutput::pull(AudioBus* inPlaceBus, size_t framesToProcess)
{
    ASSERT(context()->isAudioThread());
    ASSERT(m_renderingFanOutCount > 0);
    
    // Causes our AudioNode to process if it hasn't already for this render quantum.
    // We try to do in-place processing (using inPlaceBus) if at all possible,
    // but we can't process in-place if we're connected to more than one input (fan-out > 1).
    // In this case pull() is called multiple times per rendering quantum, and the processIfNecessary() call below will
    // cause our node to process() only the first time, caching the output in m_internalOutputBus for subsequent calls.    
    
    bool isInPlace = inPlaceBus && inPlaceBus->numberOfChannels() == numberOfChannels() && m_renderingFanOutCount == 1;

    // Setup the actual destination bus for processing when our node's process() method gets called in processIfNecessary() below.
    m_actualDestinationBus = isInPlace ? inPlaceBus : m_internalOutputBus;

    node()->processIfNecessary(framesToProcess);
    return m_actualDestinationBus;
}

AudioBus* AudioNodeOutput::bus() const
{
    ASSERT(const_cast<AudioNodeOutput*>(this)->context()->isAudioThread());
    ASSERT(m_actualDestinationBus);
    return m_actualDestinationBus;
}

unsigned AudioNodeOutput::renderingFanOutCount() const
{
    return m_renderingFanOutCount;
}

unsigned AudioNodeOutput::fanOutCount()
{
    ASSERT(context()->isGraphOwner());
    return m_inputs.size();
}

void AudioNodeOutput::addInput(AudioNodeInput* input)
{
    ASSERT(context()->isGraphOwner());

    ASSERT(input);
    if (!input)
        return;

    m_inputs.add(input);
}

void AudioNodeOutput::removeInput(AudioNodeInput* input)
{
    ASSERT(context()->isGraphOwner());

    ASSERT(input);
    if (!input)
        return;

    m_inputs.remove(input);
}

void AudioNodeOutput::disconnectAllInputs()
{
    ASSERT(context()->isGraphOwner());
    
    // AudioNodeInput::disconnect() changes m_inputs by calling removeInput().
    while (!m_inputs.isEmpty()) {
        AudioNodeInput* input = *m_inputs.begin();
        input->disconnect(this);
    }
}

void AudioNodeOutput::disable()
{
    ASSERT(context()->isGraphOwner());

    if (m_isEnabled) {
        for (InputsIterator i = m_inputs.begin(); i != m_inputs.end(); ++i) {
            AudioNodeInput* input = *i;
            input->disable(this);
        }
        m_isEnabled = false;
    }
}

void AudioNodeOutput::enable()
{
    ASSERT(context()->isGraphOwner());

    if (!m_isEnabled) {
        for (InputsIterator i = m_inputs.begin(); i != m_inputs.end(); ++i) {
            AudioNodeInput* input = *i;
            input->enable(this);
        }
        m_isEnabled = true;
    }
}

} // namespace WebCore

#endif // ENABLE(WEB_AUDIO)
