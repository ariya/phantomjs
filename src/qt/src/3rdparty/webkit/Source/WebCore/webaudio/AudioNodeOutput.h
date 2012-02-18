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

#ifndef AudioNodeOutput_h
#define AudioNodeOutput_h

#include "AudioBus.h"
#include "AudioNode.h"
#include <wtf/HashSet.h>
#include <wtf/OwnPtr.h>
#include <wtf/Vector.h>

namespace WebCore {

class AudioContext;
class AudioNodeInput;

// AudioNodeOutput represents a single output for an AudioNode.
// It may be connected to one or more AudioNodeInputs.

class AudioNodeOutput {
public:
    // It's OK to pass 0 for numberOfChannels in which case setNumberOfChannels() must be called later on.
    AudioNodeOutput(AudioNode*, unsigned numberOfChannels);

    // Can be called from any thread.
    AudioNode* node() const { return m_node; }
    AudioContext* context() { return m_node->context(); }
    
    // Causes our AudioNode to process if it hasn't already for this render quantum.
    // It returns the bus containing the processed audio for this output, returning inPlaceBus if in-place processing was possible.
    // Called from context's audio thread.
    AudioBus* pull(AudioBus* inPlaceBus, size_t framesToProcess);

    // bus() will contain the rendered audio after pull() is called for each rendering time quantum.
    // Called from context's audio thread.
    AudioBus* bus() const;

    // fanOutCount() is the number of AudioNodeInputs that we're connected to.
    // This function should not be called in audio thread rendering code, instead renderingFanOutCount() should be used.
    // It must be called with the context's graph lock.
    unsigned fanOutCount();

    // renderingFanOutCount() is the number of AudioNodeInputs that we're connected to during rendering.
    // Unlike fanOutCount() it will not change during the course of a render quantum.
    unsigned renderingFanOutCount() const;

    // It must be called with the context's graph lock.
    void disconnectAllInputs();

    void setNumberOfChannels(unsigned);
    unsigned numberOfChannels() const { return m_numberOfChannels; }
    bool isChannelCountKnown() const { return numberOfChannels() > 0; }

    // Disable/Enable happens when there are still JavaScript references to a node, but it has otherwise "finished" its work.
    // For example, when a note has finished playing.  It is kept around, because it may be played again at a later time.
    // They must be called with the context's graph lock.
    void disable();
    void enable();

    // updateRenderingState() is called in the audio thread at the start or end of the render quantum to handle any recent changes to the graph state.
    // It must be called with the context's graph lock.
    void updateRenderingState();
    
private:
    AudioNode* m_node;

    friend class AudioNodeInput;
    
    // These are called from AudioNodeInput.
    // They must be called with the context's graph lock.
    void addInput(AudioNodeInput*);
    void removeInput(AudioNodeInput*);

    // setInternalBus() sets m_internalOutputBus appropriately for the number of channels.
    // It is called in the constructor or in the audio thread with the context's graph lock.
    void setInternalBus();

    // Announce to any nodes we're connected to that we changed our channel count for its input.
    // It must be called in the audio thread with the context's graph lock.
    void propagateChannelCount();

    // updateNumberOfChannels() is called in the audio thread at the start or end of the render quantum to pick up channel changes.
    // It must be called with the context's graph lock.
    void updateNumberOfChannels();

    // m_numberOfChannels will only be changed in the audio thread.
    // The main thread sets m_desiredNumberOfChannels which will later get picked up in the audio thread in updateNumberOfChannels().
    unsigned m_numberOfChannels;
    unsigned m_desiredNumberOfChannels;
    
    // m_internalOutputBus will point to either m_monoInternalBus or m_stereoInternalBus.
    // It must only be changed in the audio thread (or constructor).
    AudioBus* m_internalOutputBus;
    OwnPtr<AudioBus> m_monoInternalBus;
    OwnPtr<AudioBus> m_stereoInternalBus;

    // m_actualDestinationBus is set in pull() and will either point to one of our internal busses or to the in-place bus.
    // It must only be changed in the audio thread (or constructor).
    AudioBus* m_actualDestinationBus;

    HashSet<AudioNodeInput*> m_inputs;
    typedef HashSet<AudioNodeInput*>::iterator InputsIterator;
    bool m_isEnabled;
    
    // For the purposes of rendering, keeps track of the number of inputs we're connected to.
    // This value should only be changed at the very start or end of the rendering quantum.
    unsigned m_renderingFanOutCount;
};

} // namespace WebCore

#endif // AudioNodeOutput_h
