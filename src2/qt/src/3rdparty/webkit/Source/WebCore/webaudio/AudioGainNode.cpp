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

#include "AudioGainNode.h"

#include "AudioBus.h"
#include "AudioNodeInput.h"
#include "AudioNodeOutput.h"

namespace WebCore {

AudioGainNode::AudioGainNode(AudioContext* context, double sampleRate)
    : AudioNode(context, sampleRate)
    , m_lastGain(1.0)
{
    m_gain = AudioGain::create("gain", 1.0, 0.0, 1.0);

    addInput(adoptPtr(new AudioNodeInput(this)));
    addOutput(adoptPtr(new AudioNodeOutput(this, 1)));
    
    setType(NodeTypeGain);
    
    initialize();
}

void AudioGainNode::process(size_t /*framesToProcess*/)
{
    // FIXME: there is a nice optimization to avoid processing here, and let the gain change
    // happen in the summing junction input of the AudioNode we're connected to.
    // Then we can avoid all of the following:

    AudioBus* outputBus = output(0)->bus();
    ASSERT(outputBus);

    // The realtime thread can't block on this lock, so we call tryLock() instead.
    if (m_processLock.tryLock()) {
        if (!isInitialized() || !input(0)->isConnected())
            outputBus->zero();
        else {
            AudioBus* inputBus = input(0)->bus();

            // Apply the gain with de-zippering into the output bus.
            outputBus->copyWithGainFrom(*inputBus, &m_lastGain, gain()->value());
        }

        m_processLock.unlock();
    } else {
        // Too bad - the tryLock() failed.  We must be in the middle of re-connecting and were already outputting silence anyway...
        outputBus->zero();
    }
}

void AudioGainNode::reset()
{
    // Snap directly to desired gain.
    m_lastGain = gain()->value();
}

// FIXME: this can go away when we do mixing with gain directly in summing junction of AudioNodeInput
//
// As soon as we know the channel count of our input, we can lazily initialize.
// Sometimes this may be called more than once with different channel counts, in which case we must safely
// uninitialize and then re-initialize with the new channel count.
void AudioGainNode::checkNumberOfChannelsForInput(AudioNodeInput* input)
{
    ASSERT(input && input == this->input(0));
    if (input != this->input(0))
        return;
        
    unsigned numberOfChannels = input->numberOfChannels();    

    if (isInitialized() && numberOfChannels != output(0)->numberOfChannels()) {
        // We're already initialized but the channel count has changed.
        // We need to be careful since we may be actively processing right now, so synchronize with process().
        MutexLocker locker(m_processLock);
        uninitialize();
    }

    if (!isInitialized()) {
        // This will propagate the channel count to any nodes connected further downstream in the graph.
        output(0)->setNumberOfChannels(numberOfChannels);
        initialize();
    }
}

} // namespace WebCore

#endif // ENABLE(WEB_AUDIO)
