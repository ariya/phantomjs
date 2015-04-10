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

#include "AudioBasicProcessorNode.h"

#include "AudioBus.h"
#include "AudioContext.h"
#include "AudioNodeInput.h"
#include "AudioNodeOutput.h"
#include "AudioProcessor.h"

namespace WebCore {

AudioBasicProcessorNode::AudioBasicProcessorNode(AudioContext* context, float sampleRate)
    : AudioNode(context, sampleRate)
{
    addInput(adoptPtr(new AudioNodeInput(this)));
    addOutput(adoptPtr(new AudioNodeOutput(this, 1)));

    // The subclass must create m_processor.
}

void AudioBasicProcessorNode::initialize()
{
    if (isInitialized())
        return;

    ASSERT(processor());
    processor()->initialize();

    AudioNode::initialize();
}

void AudioBasicProcessorNode::uninitialize()
{
    if (!isInitialized())
        return;

    ASSERT(processor());
    processor()->uninitialize();

    AudioNode::uninitialize();
}

void AudioBasicProcessorNode::process(size_t framesToProcess)
{
    AudioBus* destinationBus = output(0)->bus();
    
    if (!isInitialized() || !processor() || processor()->numberOfChannels() != numberOfChannels())
        destinationBus->zero();
    else {
        AudioBus* sourceBus = input(0)->bus();

        // FIXME: if we take "tail time" into account, then we can avoid calling processor()->process() once the tail dies down.
        if (!input(0)->isConnected())
            sourceBus->zero();

        processor()->process(sourceBus, destinationBus, framesToProcess);  
    }
}

// Nice optimization in the very common case allowing for "in-place" processing
void AudioBasicProcessorNode::pullInputs(size_t framesToProcess)
{
    // Render input stream - suggest to the input to render directly into output bus for in-place processing in process() if possible.
    input(0)->pull(output(0)->bus(), framesToProcess);
}

void AudioBasicProcessorNode::reset()
{
    if (processor())
        processor()->reset();
}

// As soon as we know the channel count of our input, we can lazily initialize.
// Sometimes this may be called more than once with different channel counts, in which case we must safely
// uninitialize and then re-initialize with the new channel count.
void AudioBasicProcessorNode::checkNumberOfChannelsForInput(AudioNodeInput* input)
{
    ASSERT(context()->isAudioThread() && context()->isGraphOwner());
    
    ASSERT(input == this->input(0));
    if (input != this->input(0))
        return;

    ASSERT(processor());
    if (!processor())
        return;

    unsigned numberOfChannels = input->numberOfChannels();
    
    if (isInitialized() && numberOfChannels != output(0)->numberOfChannels()) {
        // We're already initialized but the channel count has changed.
        uninitialize();
    }
    
    if (!isInitialized()) {
        // This will propagate the channel count to any nodes connected further down the chain...
        output(0)->setNumberOfChannels(numberOfChannels);

        // Re-initialize the processor with the new channel count.
        processor()->setNumberOfChannels(numberOfChannels);
        initialize();
    }

    AudioNode::checkNumberOfChannelsForInput(input);
}

unsigned AudioBasicProcessorNode::numberOfChannels()
{
    return output(0)->numberOfChannels();
}

double AudioBasicProcessorNode::tailTime() const
{
    return m_processor->tailTime();
}

double AudioBasicProcessorNode::latencyTime() const
{
    return m_processor->latencyTime();
}

} // namespace WebCore

#endif // ENABLE(WEB_AUDIO)
