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

#include "AudioChannelSplitter.h"

#include "AudioNodeInput.h"
#include "AudioNodeOutput.h"

namespace WebCore {
    
// This is considering that 5.1 (6 channels) is the largest we'll ever deal with.
// It can easily be increased to support more if the web audio specification is updated.
const unsigned NumberOfOutputs = 6;

AudioChannelSplitter::AudioChannelSplitter(AudioContext* context, double sampleRate)
    : AudioNode(context, sampleRate)
{
    addInput(adoptPtr(new AudioNodeInput(this)));

    // Create a fixed number of outputs (able to handle the maximum number of channels fed to an input).
    for (unsigned i = 0; i < NumberOfOutputs; ++i)
        addOutput(adoptPtr(new AudioNodeOutput(this, 1)));
    
    setType(NodeTypeChannelSplitter);
    
    initialize();
}

void AudioChannelSplitter::process(size_t framesToProcess)
{
    AudioBus* source = input(0)->bus();
    ASSERT(source);
    ASSERT_UNUSED(framesToProcess, framesToProcess == source->length());
    
    unsigned numberOfSourceChannels = source->numberOfChannels();
    
    ASSERT(numberOfOutputs() == NumberOfOutputs);
    for (unsigned i = 0; i < NumberOfOutputs; ++i) {
        AudioBus* destination = output(i)->bus();
        ASSERT(destination);
        
        if (i < numberOfSourceChannels) {
            // Split the channel out if it exists in the source.
            // It would be nice to avoid the copy and simply pass along pointers, but this becomes extremely difficult with fanout and fanin.
            destination->channel(0)->copyFrom(source->channel(i));
        } else if (output(i)->renderingFanOutCount() > 0) {
            // Only bother zeroing out the destination if it's connected to anything
            destination->zero();
        }
    }
}

void AudioChannelSplitter::reset()
{
}

} // namespace WebCore

#endif // ENABLE(WEB_AUDIO)
