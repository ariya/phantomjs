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

#include "AudioContext.h"

#include "ArrayBuffer.h"
#include "AudioBuffer.h"
#include "AudioBufferSourceNode.h"
#include "AudioChannelMerger.h"
#include "AudioChannelSplitter.h"
#include "AudioGainNode.h"
#include "AudioListener.h"
#include "AudioNodeInput.h"
#include "AudioNodeOutput.h"
#include "AudioPannerNode.h"
#include "ConvolverNode.h"
#include "DefaultAudioDestinationNode.h"
#include "DelayNode.h"
#include "Document.h"
#include "FFTFrame.h"
#include "HRTFDatabaseLoader.h"
#include "HRTFPanner.h"
#include "HighPass2FilterNode.h"
#include "JavaScriptAudioNode.h"
#include "LowPass2FilterNode.h"
#include "OfflineAudioCompletionEvent.h"
#include "OfflineAudioDestinationNode.h"
#include "PlatformString.h"
#include "RealtimeAnalyserNode.h"

#if DEBUG_AUDIONODE_REFERENCES
#include <stdio.h>
#endif

#include <wtf/OwnPtr.h>
#include <wtf/PassOwnPtr.h>
#include <wtf/RefCounted.h>

// FIXME: check the proper way to reference an undefined thread ID
const int UndefinedThreadIdentifier = 0xffffffff;

const unsigned MaxNodesToDeletePerQuantum = 10;

namespace WebCore {

PassRefPtr<AudioContext> AudioContext::create(Document* document)
{
    return adoptRef(new AudioContext(document));
}

PassRefPtr<AudioContext> AudioContext::createOfflineContext(Document* document, unsigned numberOfChannels, size_t numberOfFrames, double sampleRate)
{
    return adoptRef(new AudioContext(document, numberOfChannels, numberOfFrames, sampleRate));
}

// Constructor for rendering to the audio hardware.
AudioContext::AudioContext(Document* document)
    : ActiveDOMObject(document, this)
    , m_isInitialized(false)
    , m_isAudioThreadFinished(false)
    , m_document(document)
    , m_destinationNode(0)
    , m_connectionCount(0)
    , m_audioThread(0)
    , m_graphOwnerThread(UndefinedThreadIdentifier)
    , m_isOfflineContext(false)
{
    constructCommon();

    m_destinationNode = DefaultAudioDestinationNode::create(this);

    // This sets in motion an asynchronous loading mechanism on another thread.
    // We can check m_hrtfDatabaseLoader->isLoaded() to find out whether or not it has been fully loaded.
    // It's not that useful to have a callback function for this since the audio thread automatically starts rendering on the graph
    // when this has finished (see AudioDestinationNode).
    m_hrtfDatabaseLoader = HRTFDatabaseLoader::createAndLoadAsynchronouslyIfNecessary(sampleRate());

    // FIXME: for now default AudioContext does not need an explicit startRendering() call.
    // We may want to consider requiring it for symmetry with OfflineAudioContext
    m_destinationNode->startRendering();
}

// Constructor for offline (non-realtime) rendering.
AudioContext::AudioContext(Document* document, unsigned numberOfChannels, size_t numberOfFrames, double sampleRate)
    : ActiveDOMObject(document, this)
    , m_isInitialized(false)
    , m_isAudioThreadFinished(false)
    , m_document(document)
    , m_destinationNode(0)
    , m_connectionCount(0)
    , m_audioThread(0)
    , m_graphOwnerThread(UndefinedThreadIdentifier)
    , m_isOfflineContext(true)
{
    constructCommon();

    // FIXME: the passed in sampleRate MUST match the hardware sample-rate since HRTFDatabaseLoader is a singleton.
    m_hrtfDatabaseLoader = HRTFDatabaseLoader::createAndLoadAsynchronouslyIfNecessary(sampleRate);

    // Create a new destination for offline rendering.
    m_renderTarget = AudioBuffer::create(numberOfChannels, numberOfFrames, sampleRate);
    m_destinationNode = OfflineAudioDestinationNode::create(this, m_renderTarget.get());
}

void AudioContext::constructCommon()
{
    // Note: because adoptRef() won't be called until we leave this constructor, but code in this constructor needs to reference this context,
    // relax the check.
    relaxAdoptionRequirement();
    
    FFTFrame::initialize();
    
    m_listener = AudioListener::create();
    m_temporaryMonoBus = adoptPtr(new AudioBus(1, AudioNode::ProcessingSizeInFrames));
    m_temporaryStereoBus = adoptPtr(new AudioBus(2, AudioNode::ProcessingSizeInFrames));
}

AudioContext::~AudioContext()
{
#if DEBUG_AUDIONODE_REFERENCES
    printf("%p: AudioContext::~AudioContext()\n", this);
#endif    
    // AudioNodes keep a reference to their context, so there should be no way to be in the destructor if there are still AudioNodes around.
    ASSERT(!m_nodesToDelete.size());
    ASSERT(!m_referencedNodes.size());
    ASSERT(!m_finishedNodes.size());
}

void AudioContext::lazyInitialize()
{
    if (!m_isInitialized) {
        // Don't allow the context to initialize a second time after it's already been explicitly uninitialized.
        ASSERT(!m_isAudioThreadFinished);
        if (!m_isAudioThreadFinished) {
            if (m_destinationNode.get()) {
                // This starts the audio thread.  The destination node's provideInput() method will now be called repeatedly to render audio.
                // Each time provideInput() is called, a portion of the audio stream is rendered.  Let's call this time period a "render quantum".
                m_destinationNode->initialize();
            }
            m_isInitialized = true;
        }
    }
}

void AudioContext::uninitialize()
{
    if (m_isInitialized) {    
        // This stops the audio thread and all audio rendering.
        m_destinationNode->uninitialize();

        // Don't allow the context to initialize a second time after it's already been explicitly uninitialized.
        m_isAudioThreadFinished = true;

        // We have to release our reference to the destination node before the context will ever be deleted since the destination node holds a reference to the context.
        m_destinationNode.clear();
        
        // Get rid of the sources which may still be playing.
        derefUnfinishedSourceNodes();
        
        // Because the AudioBuffers are garbage collected, we can't delete them here.
        // Instead, at least release the potentially large amount of allocated memory for the audio data.
        // Note that we do this *after* the context is uninitialized and stops processing audio.
        for (unsigned i = 0; i < m_allocatedBuffers.size(); ++i)
            m_allocatedBuffers[i]->releaseMemory();
        m_allocatedBuffers.clear();
    
        m_isInitialized = false;
    }
}

bool AudioContext::isInitialized() const
{
    return m_isInitialized;
}

bool AudioContext::isRunnable() const
{
    if (!isInitialized())
        return false;
    
    // Check with the HRTF spatialization system to see if it's finished loading.
    return m_hrtfDatabaseLoader->isLoaded();
}

void AudioContext::stop()
{
    m_document = 0; // document is going away
    uninitialize();
}

Document* AudioContext::document() const
{
    ASSERT(m_document);
    return m_document;
}

bool AudioContext::hasDocument()
{
    return m_document;
}

void AudioContext::refBuffer(PassRefPtr<AudioBuffer> buffer)
{
    m_allocatedBuffers.append(buffer);
}

PassRefPtr<AudioBuffer> AudioContext::createBuffer(unsigned numberOfChannels, size_t numberOfFrames, double sampleRate)
{
    return AudioBuffer::create(numberOfChannels, numberOfFrames, sampleRate);
}

PassRefPtr<AudioBuffer> AudioContext::createBuffer(ArrayBuffer* arrayBuffer, bool mixToMono)
{
    ASSERT(arrayBuffer);
    if (!arrayBuffer)
        return 0;
    
    return AudioBuffer::createFromAudioFileData(arrayBuffer->data(), arrayBuffer->byteLength(), mixToMono, sampleRate());
}

PassRefPtr<AudioBufferSourceNode> AudioContext::createBufferSource()
{
    ASSERT(isMainThread());
    lazyInitialize();
    RefPtr<AudioBufferSourceNode> node = AudioBufferSourceNode::create(this, m_destinationNode->sampleRate());

    refNode(node.get()); // context keeps reference until source has finished playing
    return node;
}

PassRefPtr<JavaScriptAudioNode> AudioContext::createJavaScriptNode(size_t bufferSize)
{
    ASSERT(isMainThread());
    lazyInitialize();
    RefPtr<JavaScriptAudioNode> node = JavaScriptAudioNode::create(this, m_destinationNode->sampleRate(), bufferSize);

    refNode(node.get()); // context keeps reference until we stop making javascript rendering callbacks
    return node;
}

PassRefPtr<LowPass2FilterNode> AudioContext::createLowPass2Filter()
{
    ASSERT(isMainThread());
    lazyInitialize();
    return LowPass2FilterNode::create(this, m_destinationNode->sampleRate());
}

PassRefPtr<HighPass2FilterNode> AudioContext::createHighPass2Filter()
{
    ASSERT(isMainThread());
    lazyInitialize();
    return HighPass2FilterNode::create(this, m_destinationNode->sampleRate());
}

PassRefPtr<AudioPannerNode> AudioContext::createPanner()
{
    ASSERT(isMainThread());
    lazyInitialize();
    return AudioPannerNode::create(this, m_destinationNode->sampleRate());
}

PassRefPtr<ConvolverNode> AudioContext::createConvolver()
{
    ASSERT(isMainThread());
    lazyInitialize();
    return ConvolverNode::create(this, m_destinationNode->sampleRate());
}

PassRefPtr<RealtimeAnalyserNode> AudioContext::createAnalyser()
{
    ASSERT(isMainThread());
    lazyInitialize();
    return RealtimeAnalyserNode::create(this, m_destinationNode->sampleRate());
}

PassRefPtr<AudioGainNode> AudioContext::createGainNode()
{
    ASSERT(isMainThread());
    lazyInitialize();
    return AudioGainNode::create(this, m_destinationNode->sampleRate());
}

PassRefPtr<DelayNode> AudioContext::createDelayNode()
{
    ASSERT(isMainThread());
    lazyInitialize();
    return DelayNode::create(this, m_destinationNode->sampleRate());
}

PassRefPtr<AudioChannelSplitter> AudioContext::createChannelSplitter()
{
    ASSERT(isMainThread());
    lazyInitialize();
    return AudioChannelSplitter::create(this, m_destinationNode->sampleRate());
}

PassRefPtr<AudioChannelMerger> AudioContext::createChannelMerger()
{
    ASSERT(isMainThread());
    lazyInitialize();
    return AudioChannelMerger::create(this, m_destinationNode->sampleRate());
}

void AudioContext::notifyNodeFinishedProcessing(AudioNode* node)
{
    ASSERT(isAudioThread());
    m_finishedNodes.append(node);
}

void AudioContext::derefFinishedSourceNodes()
{
    ASSERT(isGraphOwner());
    ASSERT(isAudioThread() || isAudioThreadFinished());
    for (unsigned i = 0; i < m_finishedNodes.size(); i++)
        derefNode(m_finishedNodes[i]);

    m_finishedNodes.clear();
}

void AudioContext::refNode(AudioNode* node)
{
    ASSERT(isMainThread());
    AutoLocker locker(this);
    
    node->ref(AudioNode::RefTypeConnection);
    m_referencedNodes.append(node);
}

void AudioContext::derefNode(AudioNode* node)
{
    ASSERT(isGraphOwner());
    
    node->deref(AudioNode::RefTypeConnection);

    for (unsigned i = 0; i < m_referencedNodes.size(); ++i) {
        if (node == m_referencedNodes[i]) {
            m_referencedNodes.remove(i);
            break;
        }
    }
}

void AudioContext::derefUnfinishedSourceNodes()
{
    ASSERT(isMainThread() && isAudioThreadFinished());
    for (unsigned i = 0; i < m_referencedNodes.size(); ++i)
        m_referencedNodes[i]->deref(AudioNode::RefTypeConnection);

    m_referencedNodes.clear();
}

void AudioContext::lock(bool& mustReleaseLock)
{
    // Don't allow regular lock in real-time audio thread.
    ASSERT(isMainThread());

    ThreadIdentifier thisThread = currentThread();

    if (thisThread == m_graphOwnerThread) {
        // We already have the lock.
        mustReleaseLock = false;
    } else {
        // Acquire the lock.
        m_contextGraphMutex.lock();
        m_graphOwnerThread = thisThread;
        mustReleaseLock = true;
    }
}

bool AudioContext::tryLock(bool& mustReleaseLock)
{
    ThreadIdentifier thisThread = currentThread();
    bool isAudioThread = thisThread == audioThread();

    // Try to catch cases of using try lock on main thread - it should use regular lock.
    ASSERT(isAudioThread || isAudioThreadFinished());
    
    if (!isAudioThread) {
        // In release build treat tryLock() as lock() (since above ASSERT(isAudioThread) never fires) - this is the best we can do.
        lock(mustReleaseLock);
        return true;
    }
    
    bool hasLock;
    
    if (thisThread == m_graphOwnerThread) {
        // Thread already has the lock.
        hasLock = true;
        mustReleaseLock = false;
    } else {
        // Don't already have the lock - try to acquire it.
        hasLock = m_contextGraphMutex.tryLock();
        
        if (hasLock)
            m_graphOwnerThread = thisThread;

        mustReleaseLock = hasLock;
    }
    
    return hasLock;
}

void AudioContext::unlock()
{
    ASSERT(currentThread() == m_graphOwnerThread);

    m_graphOwnerThread = UndefinedThreadIdentifier;
    m_contextGraphMutex.unlock();
}

bool AudioContext::isAudioThread() const
{
    return currentThread() == m_audioThread;
}

bool AudioContext::isGraphOwner() const
{
    return currentThread() == m_graphOwnerThread;
}

void AudioContext::addDeferredFinishDeref(AudioNode* node, AudioNode::RefType refType)
{
    ASSERT(isAudioThread());
    m_deferredFinishDerefList.append(AudioContext::RefInfo(node, refType));
}

void AudioContext::handlePreRenderTasks()
{
    ASSERT(isAudioThread());
 
    // At the beginning of every render quantum, try to update the internal rendering graph state (from main thread changes).
    // It's OK if the tryLock() fails, we'll just take slightly longer to pick up the changes.
    bool mustReleaseLock;
    if (tryLock(mustReleaseLock)) {
        // Fixup the state of any dirty AudioNodeInputs and AudioNodeOutputs.
        handleDirtyAudioNodeInputs();
        handleDirtyAudioNodeOutputs();
        
        if (mustReleaseLock)
            unlock();
    }
}

void AudioContext::handlePostRenderTasks()
{
    ASSERT(isAudioThread());
 
    // Must use a tryLock() here too.  Don't worry, the lock will very rarely be contended and this method is called frequently.
    // The worst that can happen is that there will be some nodes which will take slightly longer than usual to be deleted or removed
    // from the render graph (in which case they'll render silence).
    bool mustReleaseLock;
    if (tryLock(mustReleaseLock)) {
        // Take care of finishing any derefs where the tryLock() failed previously.
        handleDeferredFinishDerefs();

        // Dynamically clean up nodes which are no longer needed.
        derefFinishedSourceNodes();

        // Finally actually delete.
        deleteMarkedNodes();

        // Fixup the state of any dirty AudioNodeInputs and AudioNodeOutputs.
        handleDirtyAudioNodeInputs();
        handleDirtyAudioNodeOutputs();
        
        if (mustReleaseLock)
            unlock();
    }
}

void AudioContext::handleDeferredFinishDerefs()
{
    ASSERT(isAudioThread() && isGraphOwner());
    for (unsigned i = 0; i < m_deferredFinishDerefList.size(); ++i) {
        AudioNode* node = m_deferredFinishDerefList[i].m_node;
        AudioNode::RefType refType = m_deferredFinishDerefList[i].m_refType;
        node->finishDeref(refType);
    }
    
    m_deferredFinishDerefList.clear();
}

void AudioContext::markForDeletion(AudioNode* node)
{
    ASSERT(isGraphOwner());
    m_nodesToDelete.append(node);
}

void AudioContext::deleteMarkedNodes()
{
    ASSERT(isGraphOwner() || isAudioThreadFinished());

    // Note: deleting an AudioNode can cause m_nodesToDelete to grow.
    size_t nodesDeleted = 0;
    while (size_t n = m_nodesToDelete.size()) {
        AudioNode* node = m_nodesToDelete[n - 1];
        m_nodesToDelete.removeLast();

        // Before deleting the node, clear out any AudioNodeInputs from m_dirtyAudioNodeInputs.
        unsigned numberOfInputs = node->numberOfInputs();
        for (unsigned i = 0; i < numberOfInputs; ++i)
            m_dirtyAudioNodeInputs.remove(node->input(i));

        // Before deleting the node, clear out any AudioNodeOutputs from m_dirtyAudioNodeOutputs.
        unsigned numberOfOutputs = node->numberOfOutputs();
        for (unsigned i = 0; i < numberOfOutputs; ++i)
            m_dirtyAudioNodeOutputs.remove(node->output(i));

        // Finally, delete it.
        delete node;

        // Don't delete too many nodes per render quantum since we don't want to do too much work in the realtime audio thread.
        if (++nodesDeleted > MaxNodesToDeletePerQuantum)
            break;
    }
}

void AudioContext::markAudioNodeInputDirty(AudioNodeInput* input)
{
    ASSERT(isGraphOwner());    
    m_dirtyAudioNodeInputs.add(input);
}

void AudioContext::markAudioNodeOutputDirty(AudioNodeOutput* output)
{
    ASSERT(isGraphOwner());    
    m_dirtyAudioNodeOutputs.add(output);
}

void AudioContext::handleDirtyAudioNodeInputs()
{
    ASSERT(isGraphOwner());    

    for (HashSet<AudioNodeInput*>::iterator i = m_dirtyAudioNodeInputs.begin(); i != m_dirtyAudioNodeInputs.end(); ++i)
        (*i)->updateRenderingState();

    m_dirtyAudioNodeInputs.clear();
}

void AudioContext::handleDirtyAudioNodeOutputs()
{
    ASSERT(isGraphOwner());    

    for (HashSet<AudioNodeOutput*>::iterator i = m_dirtyAudioNodeOutputs.begin(); i != m_dirtyAudioNodeOutputs.end(); ++i)
        (*i)->updateRenderingState();

    m_dirtyAudioNodeOutputs.clear();
}

ScriptExecutionContext* AudioContext::scriptExecutionContext() const
{
    return document();
}

AudioContext* AudioContext::toAudioContext()
{
    return this;
}

void AudioContext::startRendering()
{
    destination()->startRendering();
}

void AudioContext::fireCompletionEvent()
{
    ASSERT(isMainThread());
    if (!isMainThread())
        return;
        
    AudioBuffer* renderedBuffer = m_renderTarget.get();

    ASSERT(renderedBuffer);
    if (!renderedBuffer)
        return;

    // Avoid firing the event if the document has already gone away.
    if (hasDocument()) {
        // Call the offline rendering completion event listener.
        dispatchEvent(OfflineAudioCompletionEvent::create(renderedBuffer));
    }
}

} // namespace WebCore

#endif // ENABLE(WEB_AUDIO)
