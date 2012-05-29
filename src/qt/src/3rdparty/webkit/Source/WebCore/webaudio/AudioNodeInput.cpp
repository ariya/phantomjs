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
    : m_node(node)
    , m_renderingStateNeedUpdating(false)
{
    m_monoSummingBus = adoptPtr(new AudioBus(1, AudioNode::ProcessingSizeInFrames));
    m_stereoSummingBus = adoptPtr(new AudioBus(2, AudioNode::ProcessingSizeInFrames));
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
        node()->deref(AudioNode::RefTypeDisabled); // Note: it's important to return immediately after all deref() calls since the node may be deleted.
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

    node()->ref(AudioNode::RefTypeDisabled);
    node()->deref(AudioNode::RefTypeConnection); // Note: it's important to return immediately after all deref() calls since the node may be deleted.
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

    node()->ref(AudioNode::RefTypeConnection);
    node()->deref(AudioNode::RefTypeDisabled); // Note: it's important to return immediately after all deref() calls since the node may be deleted.
}

void AudioNodeInput::changedOutputs()
{
    ASSERT(context()->isGraphOwner());
    if (!m_renderingStateNeedUpdating && !node()->isMarkedForDeletion()) {    
        context()->markAudioNodeInputDirty(this);
        m_renderingStateNeedUpdating = true;
    }
}

void AudioNodeInput::updateRenderingState()
{
    ASSERT(context()->isAudioThread() && context()->isGraphOwner());
    
    if (m_renderingStateNeedUpdating && !node()->isMarkedForDeletion()) {
        // Copy from m_outputs to m_renderingOutputs.
        m_renderingOutputs.resize(m_outputs.size());
        unsigned j = 0;
        for (HashSet<AudioNodeOutput*>::iterator i = m_outputs.begin(); i != m_outputs.end(); ++i, ++j) {
            AudioNodeOutput* output = *i;
            m_renderingOutputs[j] = output;
            output->updateRenderingState();
        }

        node()->checkNumberOfChannelsForInput(this);
        
        m_renderingStateNeedUpdating = false;
    }
}

unsigned AudioNodeInput::numberOfChannels() const
{
    // Find the number of channels of the connection with the largest number of channels.
    unsigned maxChannels = 1; // one channel is the minimum allowed

    for (HashSet<AudioNodeOutput*>::iterator i = m_outputs.begin(); i != m_outputs.end(); ++i) {
        AudioNodeOutput* output = *i;
        maxChannels = max(maxChannels, output->bus()->numberOfChannels());
    }
    
    return maxChannels;
}

unsigned AudioNodeInput::numberOfRenderingChannels()
{
    ASSERT(context()->isAudioThread());

    // Find the number of channels of the rendering connection with the largest number of channels.
    unsigned maxChannels = 1; // one channel is the minimum allowed

    for (unsigned i = 0; i < numberOfRenderingConnections(); ++i)
        maxChannels = max(maxChannels, renderingOutput(i)->bus()->numberOfChannels());
    
    return maxChannels;
}

AudioBus* AudioNodeInput::bus()
{
    ASSERT(context()->isAudioThread());

    // Handle single connection specially to allow for in-place processing.
    if (numberOfRenderingConnections() == 1)
        return renderingOutput(0)->bus();

    // Multiple connections case (or no connections).
    return internalSummingBus();
}

AudioBus* AudioNodeInput::internalSummingBus()
{
    ASSERT(context()->isAudioThread());

    // We must pick a summing bus which is the right size to handle the largest connection.
    switch (numberOfRenderingChannels()) {
    case 1:
        return m_monoSummingBus.get();
    case 2:
        return m_stereoSummingBus.get();
    // FIXME: could implement more than just mono and stereo mixing in the future
    }
    
    ASSERT_NOT_REACHED();
    return 0;
}

void AudioNodeInput::sumAllConnections(AudioBus* summingBus, size_t framesToProcess)
{
    ASSERT(context()->isAudioThread());

    // We shouldn't be calling this method if there's only one connection, since it's less efficient.
    ASSERT(numberOfRenderingConnections() > 1);

    ASSERT(summingBus);
    if (!summingBus)
        return;
        
    summingBus->zero();

    for (unsigned i = 0; i < numberOfRenderingConnections(); ++i) {
        AudioNodeOutput* output = renderingOutput(i);
        ASSERT(output);

        // Render audio from this output.
        AudioBus* connectionBus = output->pull(0, framesToProcess);

        // Sum, with unity-gain.
        summingBus->sumFrom(*connectionBus);
    }
}

AudioBus* AudioNodeInput::pull(AudioBus* inPlaceBus, size_t framesToProcess)
{
    ASSERT(context()->isAudioThread());

    // Handle single connection case.
    if (numberOfRenderingConnections() == 1) {
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
