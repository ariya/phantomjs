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

#include "AudioDestinationNode.h"

#include "AudioBus.h"
#include "AudioContext.h"
#include "AudioNodeInput.h"
#include "AudioNodeOutput.h"

namespace WebCore {
    
AudioDestinationNode::AudioDestinationNode(AudioContext* context, double sampleRate)
    : AudioNode(context, sampleRate)
    , m_currentTime(0.0)
{
    addInput(adoptPtr(new AudioNodeInput(this)));
    
    setType(NodeTypeDestination);
}

AudioDestinationNode::~AudioDestinationNode()
{
    uninitialize();
}

// The audio hardware calls us back here to gets its input stream.
void AudioDestinationNode::provideInput(AudioBus* destinationBus, size_t numberOfFrames)
{
    context()->setAudioThread(currentThread());
    
    if (!context()->isRunnable()) {
        destinationBus->zero();
        return;
    }

    // Let the context take care of any business at the start of each render quantum.
    context()->handlePreRenderTasks();

    // This will cause the node(s) connected to us to process, which in turn will pull on their input(s),
    // all the way backwards through the rendering graph.
    AudioBus* renderedBus = input(0)->pull(destinationBus, numberOfFrames);
    
    if (!renderedBus)
        destinationBus->zero();
    else if (renderedBus != destinationBus) {
        // in-place processing was not possible - so copy
        destinationBus->copyFrom(*renderedBus);
    }

    // Let the context take care of any business at the end of each render quantum.
    context()->handlePostRenderTasks();
    
    // Advance current time.
    m_currentTime += numberOfFrames / sampleRate();
}

} // namespace WebCore

#endif // ENABLE(WEB_AUDIO)
