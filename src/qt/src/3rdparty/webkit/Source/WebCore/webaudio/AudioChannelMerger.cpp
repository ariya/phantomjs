/*
 * Copyright (C) 2010 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"

#if ENABLE(WEB_AUDIO)

#include "AudioChannelMerger.h"

#include "AudioNodeInput.h"
#include "AudioNodeOutput.h"

namespace WebCore {
    
// This is considering that 5.1 (6 channels) is the largest we'll ever deal with.
// It can easily be increased to support more if the web audio specification is updated.
const unsigned NumberOfInputs = 6;

AudioChannelMerger::AudioChannelMerger(AudioContext* context, double sampleRate)
    : AudioNode(context, sampleRate)
{
    // Create a fixed number of inputs (able to handle the maximum number of channels we deal with).
    for (unsigned i = 0; i < NumberOfInputs; ++i)
        addInput(adoptPtr(new AudioNodeInput(this)));

    addOutput(adoptPtr(new AudioNodeOutput(this, 1)));
    
    setType(NodeTypeChannelMerger);
    
    initialize();
}

void AudioChannelMerger::process(size_t framesToProcess)
{
    AudioNodeOutput* output = this->output(0);
    ASSERT(output);
    ASSERT_UNUSED(framesToProcess, framesToProcess == output->bus()->length());    
    
    // Count how many channels we have all together from all of the inputs.
    unsigned numberOfOutputChannels = 0;
    for (unsigned i = 0; i < numberOfInputs(); ++i) {
        AudioNodeInput* input = this->input(i);
        if (input->isConnected())
            numberOfOutputChannels += input->bus()->numberOfChannels();
    }

    // Set the correct number of channels on the output
    output->setNumberOfChannels(numberOfOutputChannels);
    
    // Now merge the channels back into one output.
    unsigned outputChannelIndex = 0;
    for (unsigned i = 0; i < numberOfInputs(); ++i) {
        AudioNodeInput* input = this->input(i);
        if (input->isConnected()) {
            unsigned numberOfInputChannels = input->bus()->numberOfChannels();
            
            // Merge channels from this particular input.
            for (unsigned j = 0; j < numberOfInputChannels; ++j) {
                AudioChannel* inputChannel = input->bus()->channel(j);
                AudioChannel* outputChannel = output->bus()->channel(outputChannelIndex);
                outputChannel->copyFrom(inputChannel);
                
                ++outputChannelIndex;
            }
        }
    }
    
    ASSERT(outputChannelIndex == numberOfOutputChannels);
}

void AudioChannelMerger::reset()
{
}

} // namespace WebCore

#endif // ENABLE(WEB_AUDIO)
