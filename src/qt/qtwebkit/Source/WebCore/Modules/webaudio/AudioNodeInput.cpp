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

#include "AudioNodeInput.h"

#include "AudioContext.h"
#include "AudioNode.h"
#include "AudioNodeOutput.h"
#include <algorithm>

using namespace std;
 
namespace WebCore {

AudioNodeInput::AudioNodeInput(AudioNode* node)
    : AudioSummingJunction(node->context())
    , m_node(node)
{
    // Set to mono by default.
    m_internalSummingBus = AudioBus::create(1, AudioNode::ProcessingSizeInFrames);
}

void AudioNodeInput::connect(AudioNodeOutput* output)
{
    ASSERT(context()->isGraphOwner());
    
    ASSERT(output && node());
    if (!output || !node())
        return;

    // Check if we're already connected to this output.
    if (m_outputs.contains(output))
        return;
        
    output->addInput(this);
    m_outputs.add(output);
    changedOutputs();

    // Sombody has just connected to us, so count it as a reference.
    node()->ref(AudioNode::RefTypeConnection);
}

void AudioNodeInput::disconnect(AudioNodeOutput* output)
{
    ASSERT(context()->isGraphOwner());

    ASSERT(output && node());
    if (!output || !node())
        return;

    // First try to disconnect from "active" connections.
    if (m_outputs.contains(output)) {
        m_outputs.remove(output);
        changedOutputs();
        output->removeInput(this);
        node()->deref(AudioNode::RefTypeConnection); // Note: it's important to return immediately after all deref() calls since the node may be deleted.
        return;
    }
    
    // Otherwise, try to disconnect from disabled connections.
    if (m_disabledOutputs.contains(output)) {
        m_disabledOutputs.remove(output);
        output->removeInput(this);
        node()->deref(AudioNode::RefTypeConnection); // Note: it's important to return immediately after all deref() calls since the node may be deleted.
        return;
    }

    ASSERT_NOT_REACHED();
}

void AudioNodeInput::disable(AudioNodeOutput* output)
{
    ASSERT(context()->isGraphOwner());

    ASSERT(output && node());
    if (!output || !node())
        return;

    ASSERT(m_outputs.contains(output));
    
    m_disabledOutputs.add(output);
    m_outputs.remove(output);
    changedOutputs();

    // Propagate disabled state to outputs.
    node()->disableOutputsIfNecessary();
}

void AudioNodeInput::enable(AudioNodeOutput* output)
{
    ASSERT(context()->isGraphOwner());

    ASSERT(output && node());
    if (!output || !node())
        return;

    ASSERT(m_disabledOutputs.contains(output));

    // Move output from disabled list to active list.
    m_outputs.add(output);
    m_disabledOutputs.remove(output);
    changedOutputs();

    // Propagate enabled state to outputs.
    node()->enableOutputsIfNecessary();
}

void AudioNodeInput::didUpdate()
{
    node()->checkNumberOfChannelsForInput(this);
}

void AudioNodeInput::updateInternalBus()
{
    ASSERT(context()->isAudioThread() && context()->isGraphOwner());

    unsigned numberOfInputChannels = numberOfChannels();

    if (numberOfInputChannels == m_internalSummingBus->numberOfChannels())
        return;

    m_internalSummingBus = AudioBus::create(numberOfInputChannels, AudioNode::ProcessingSizeInFrames);
}

unsigned AudioNodeInput::numberOfChannels() const
{
    AudioNode::ChannelCountMode mode = node()->internalChannelCountMode();
    if (mode == AudioNode::Explicit)
        return node()->channelCount();

    // Find the number of channels of the connection with the largest number of channels.
    unsigned maxChannels = 1; // one channel is the minimum allowed

    for (HashSet<AudioNodeOutput*>::iterator i = m_outputs.begin(); i != m_outputs.end(); ++i) {
        AudioNodeOutput* output = *i;
        // Use output()->numberOfChannels() instead of output->bus()->numberOfChannels(),
        // because the calling of AudioNodeOutput::bus() is not safe here.
        maxChannels = max(maxChannels, output->numberOfChannels());
    }

    if (mode == AudioNode::ClampedMax)
        maxChannels = min(maxChannels, static_cast<unsigned>(node()->channelCount()));

    return maxChannels;
}

AudioBus* AudioNodeInput::bus()
{
    ASSERT(context()->isAudioThread());

    // Handle single connection specially to allow for in-place processing.
    if (numberOfRenderingConnections() == 1 && node()->internalChannelCountMode() == AudioNode::Max)
        return renderingOutput(0)->bus();

    // Multiple connections case or complex ChannelCountMode (or no connections).
    return internalSummingBus();
}

AudioBus* AudioNodeInput::internalSummingBus()
{
    ASSERT(context()->isAudioThread());

    return m_internalSummingBus.get();
}

void AudioNodeInput::sumAllConnections(AudioBus* summingBus, size_t framesToProcess)
{
    ASSERT(context()->isAudioThread());

    // We shouldn't be calling this method if there's only one connection, since it's less efficient.
    ASSERT(numberOfRenderingConnections() > 1 || node()->internalChannelCountMode() != AudioNode::Max);

    ASSERT(summingBus);
    if (!summingBus)
        return;
        
    summingBus->zero();

    AudioBus::ChannelInterpretation interpretation = node()->internalChannelInterpretation();

    for (unsigned i = 0; i < numberOfRenderingConnections(); ++i) {
        AudioNodeOutput* output = renderingOutput(i);
        ASSERT(output);

        // Render audio from this output.
        AudioBus* connectionBus = output->pull(0, framesToProcess);

        // Sum, with unity-gain.
        summingBus->sumFrom(*connectionBus, interpretation);
    }
}

AudioBus* AudioNodeInput::pull(AudioBus* inPlaceBus, size_t framesToProcess)
{
    ASSERT(context()->isAudioThread());

    // Handle single connection case.
    if (numberOfRenderingConnections() == 1 && node()->internalChannelCountMode() == AudioNode::Max) {
        // The output will optimize processing using inPlaceBus if it's able.
        AudioNodeOutput* output = this->renderingOutput(0);
        return output->pull(inPlaceBus, framesToProcess);
    }

    AudioBus* internalSummingBus = this->internalSummingBus();

    if (!numberOfRenderingConnections()) {
        // At least, generate silence if we're not connected to anything.
        // FIXME: if we wanted to get fancy, we could propagate a 'silent hint' here to optimize the downstream graph processing.
        internalSummingBus->zero();
        return internalSummingBus;
    }

    // Handle multiple connections case.
    sumAllConnections(internalSummingBus, framesToProcess);
    
    return internalSummingBus;
}

} // namespace WebCore

#endif // ENABLE(WEB_AUDIO)
