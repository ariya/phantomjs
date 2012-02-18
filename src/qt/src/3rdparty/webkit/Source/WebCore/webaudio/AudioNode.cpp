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

#include "AudioNode.h"

#include "AudioContext.h"
#include "AudioNodeInput.h"
#include "AudioNodeOutput.h"
#include <wtf/Atomics.h>

namespace WebCore {

AudioNode::AudioNode(AudioContext* context, double sampleRate)
    : m_isInitialized(false)
    , m_type(NodeTypeUnknown)
    , m_context(context)
    , m_sampleRate(sampleRate)
    , m_lastProcessingTime(-1.0)
    , m_normalRefCount(1) // start out with normal refCount == 1 (like WTF::RefCounted class)
    , m_connectionRefCount(0)
    , m_disabledRefCount(0)
    , m_isMarkedForDeletion(false)
    , m_isDisabled(false)
{
#if DEBUG_AUDIONODE_REFERENCES
    if (!s_isNodeCountInitialized) {
        s_isNodeCountInitialized = true;
        atexit(AudioNode::printNodeCounts);
    }
#endif
}

AudioNode::~AudioNode()
{
#if DEBUG_AUDIONODE_REFERENCES
    --s_nodeCount[type()];
    printf("%p: %d: AudioNode::~AudioNode() %d %d %d\n", this, type(), m_normalRefCount, m_connectionRefCount, m_disabledRefCount);
#endif
}

void AudioNode::initialize()
{
    m_isInitialized = true;
}

void AudioNode::uninitialize()
{
    m_isInitialized = false;
}

void AudioNode::setType(NodeType type)
{
    m_type = type;

#if DEBUG_AUDIONODE_REFERENCES
    ++s_nodeCount[type];
#endif
}

void AudioNode::lazyInitialize()
{
    if (!isInitialized())
        initialize();
}

void AudioNode::addInput(PassOwnPtr<AudioNodeInput> input)
{
    m_inputs.append(input);
}

void AudioNode::addOutput(PassOwnPtr<AudioNodeOutput> output)
{
    m_outputs.append(output);
}

AudioNodeInput* AudioNode::input(unsigned i)
{
    return m_inputs[i].get();
}

AudioNodeOutput* AudioNode::output(unsigned i)
{
    return m_outputs[i].get();
}

bool AudioNode::connect(AudioNode* destination, unsigned outputIndex, unsigned inputIndex)
{
    ASSERT(isMainThread()); 
    AudioContext::AutoLocker locker(context());
    
    // Sanity check input and output indices.
    if (outputIndex >= numberOfOutputs())
        return false;
    if (destination && inputIndex >= destination->numberOfInputs())
        return false;

    AudioNodeOutput* output = this->output(outputIndex);
    if (!destination) {
        // Disconnect output from any inputs it may be currently connected to.
        output->disconnectAllInputs();
        return true;
    }

    AudioNodeInput* input = destination->input(inputIndex);
    input->connect(output);

    // Let context know that a connection has been made.
    context()->incrementConnectionCount();

    return true;
}

bool AudioNode::disconnect(unsigned outputIndex)
{
    ASSERT(isMainThread());
    AudioContext::AutoLocker locker(context());
    
    return connect(0, outputIndex);
}

void AudioNode::processIfNecessary(size_t framesToProcess)
{
    ASSERT(context()->isAudioThread());
    
    if (!isInitialized())
        return;

    // Ensure that we only process once per rendering quantum.
    // This handles the "fanout" problem where an output is connected to multiple inputs.
    // The first time we're called during this time slice we process, but after that we don't want to re-process,
    // instead our output(s) will already have the results cached in their bus;
    double currentTime = context()->currentTime();
    if (m_lastProcessingTime != currentTime) {
        m_lastProcessingTime = currentTime; // important to first update this time because of feedback loops in the rendering graph
        pullInputs(framesToProcess);
        process(framesToProcess);
    }
}

void AudioNode::pullInputs(size_t framesToProcess)
{
    ASSERT(context()->isAudioThread());
    
    // Process all of the AudioNodes connected to our inputs.
    for (unsigned i = 0; i < m_inputs.size(); ++i)
        input(i)->pull(0, framesToProcess);
}

void AudioNode::ref(RefType refType)
{
    switch (refType) {
    case RefTypeNormal:
        atomicIncrement(&m_normalRefCount);
        break;
    case RefTypeConnection:
        atomicIncrement(&m_connectionRefCount);
        break;
    case RefTypeDisabled:
        atomicIncrement(&m_disabledRefCount);
        break;
    default:
        ASSERT_NOT_REACHED();
    }

#if DEBUG_AUDIONODE_REFERENCES
    printf("%p: %d: AudioNode::ref(%d) %d %d %d\n", this, type(), refType, m_normalRefCount, m_connectionRefCount, m_disabledRefCount);
#endif

    if (m_connectionRefCount == 1 && refType == RefTypeConnection) {
        // FIXME: implement wake-up - this is an advanced feature and is not necessary in a simple implementation.
        // We should not be "actively" connected to anything, but now we're "waking up"
        // For example, a note which has finished playing, but is now being played again.
        // Note that if this is considered a worthwhile feature to add, then an evaluation of the locking considerations must be made.
    }
}

void AudioNode::deref(RefType refType)
{
    // The actually work for deref happens completely within the audio context's graph lock.
    // In the case of the audio thread, we must use a tryLock to avoid glitches.
    bool hasLock = false;
    bool mustReleaseLock = false;
    
    if (context()->isAudioThread()) {
        // Real-time audio thread must not contend lock (to avoid glitches).
        hasLock = context()->tryLock(mustReleaseLock);
    } else {
        context()->lock(mustReleaseLock);
        hasLock = true;
    }
    
    if (hasLock) {
        // This is where the real deref work happens.
        finishDeref(refType);

        if (mustReleaseLock)
            context()->unlock();
    } else {
        // We were unable to get the lock, so put this in a list to finish up later.
        ASSERT(context()->isAudioThread());
        context()->addDeferredFinishDeref(this, refType);
    }

    // Once AudioContext::uninitialize() is called there's no more chances for deleteMarkedNodes() to get called, so we call here.
    // We can't call in AudioContext::~AudioContext() since it will never be called as long as any AudioNode is alive
    // because AudioNodes keep a reference to the context.
    if (context()->isAudioThreadFinished())
        context()->deleteMarkedNodes();
}

void AudioNode::finishDeref(RefType refType)
{
    ASSERT(context()->isGraphOwner());
    
    switch (refType) {
    case RefTypeNormal:
        ASSERT(m_normalRefCount > 0);
        atomicDecrement(&m_normalRefCount);
        break;
    case RefTypeConnection:
        ASSERT(m_connectionRefCount > 0);
        atomicDecrement(&m_connectionRefCount);
        break;
    case RefTypeDisabled:
        ASSERT(m_disabledRefCount > 0);
        atomicDecrement(&m_disabledRefCount);
        break;
    default:
        ASSERT_NOT_REACHED();
    }
    
#if DEBUG_AUDIONODE_REFERENCES
    printf("%p: %d: AudioNode::deref(%d) %d %d %d\n", this, type(), refType, m_normalRefCount, m_connectionRefCount, m_disabledRefCount);
#endif

    if (!m_connectionRefCount) {
        if (!m_normalRefCount && !m_disabledRefCount) {
            if (!m_isMarkedForDeletion) {
                // All references are gone - we need to go away.
                for (unsigned i = 0; i < m_outputs.size(); ++i)
                    output(i)->disconnectAllInputs(); // this will deref() nodes we're connected to...

                // Mark for deletion at end of each render quantum or when context shuts down.
                context()->markForDeletion(this);
                m_isMarkedForDeletion = true;
            }
        } else if (refType == RefTypeConnection) {
            if (!m_isDisabled) {
                // Still may have JavaScript references, but no more "active" connection references, so put all of our outputs in a "dormant" disabled state.
                // Garbage collection may take a very long time after this time, so the "dormant" disabled nodes should not bog down the rendering...

                // As far as JavaScript is concerned, our outputs must still appear to be connected.
                // But internally our outputs should be disabled from the inputs they're connected to.
                // disable() can recursively deref connections (and call disable()) down a whole chain of connected nodes.

                // FIXME: we special case the convolver and delay since they have a significant tail-time and shouldn't be disconnected simply
                // because they no longer have any input connections.  This needs to be handled more generally where AudioNodes have
                // a tailTime attribute.  Then the AudioNode only needs to remain "active" for tailTime seconds after there are no
                // longer any active connections.
                if (type() != NodeTypeConvolver && type() != NodeTypeDelay) {
                    m_isDisabled = true;
                    for (unsigned i = 0; i < m_outputs.size(); ++i)
                        output(i)->disable();
                }
            }
        }
    }
}

#if DEBUG_AUDIONODE_REFERENCES

bool AudioNode::s_isNodeCountInitialized = false;
int AudioNode::s_nodeCount[NodeTypeEnd];

void AudioNode::printNodeCounts()
{
    printf("\n\n");
    printf("===========================\n");
    printf("AudioNode: reference counts\n");
    printf("===========================\n");

    for (unsigned i = 0; i < NodeTypeEnd; ++i)
        printf("%d: %d\n", i, s_nodeCount[i]);

    printf("===========================\n\n\n");
}

#endif // DEBUG_AUDIONODE_REFERENCES

} // namespace WebCore

#endif // ENABLE(WEB_AUDIO)
