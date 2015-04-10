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

#include "AnalyserNode.h"
#include "AsyncAudioDecoder.h"
#include "AudioBuffer.h"
#include "AudioBufferCallback.h"
#include "AudioBufferSourceNode.h"
#include "AudioListener.h"
#include "AudioNodeInput.h"
#include "AudioNodeOutput.h"
#include "BiquadFilterNode.h"
#include "ChannelMergerNode.h"
#include "ChannelSplitterNode.h"
#include "ConvolverNode.h"
#include "DefaultAudioDestinationNode.h"
#include "DelayNode.h"
#include "Document.h"
#include "DynamicsCompressorNode.h"
#include "ExceptionCode.h"
#include "FFTFrame.h"
#include "GainNode.h"
#include "HRTFDatabaseLoader.h"
#include "HRTFPanner.h"
#include "OfflineAudioCompletionEvent.h"
#include "OfflineAudioDestinationNode.h"
#include "OscillatorNode.h"
#include "Page.h"
#include "PannerNode.h"
#include "PeriodicWave.h"
#include "ScriptCallStack.h"
#include "ScriptController.h"
#include "ScriptProcessorNode.h"
#include "WaveShaperNode.h"

#if ENABLE(MEDIA_STREAM)
#include "MediaStream.h"
#include "MediaStreamAudioDestinationNode.h"
#include "MediaStreamAudioSourceNode.h"
#endif

#if ENABLE(VIDEO)
#include "HTMLMediaElement.h"
#include "MediaElementAudioSourceNode.h"
#endif

#if DEBUG_AUDIONODE_REFERENCES
#include <stdio.h>
#endif

#if USE(GSTREAMER)
#include "GStreamerUtilities.h"
#endif

#include <wtf/ArrayBuffer.h>
#include <wtf/Atomics.h>
#include <wtf/MainThread.h>
#include <wtf/OwnPtr.h>
#include <wtf/PassOwnPtr.h>
#include <wtf/RefCounted.h>
#include <wtf/text/WTFString.h>

// FIXME: check the proper way to reference an undefined thread ID
const int UndefinedThreadIdentifier = 0xffffffff;

const unsigned MaxNodesToDeletePerQuantum = 10;
const unsigned MaxPeriodicWaveLength = 4096;

namespace WebCore {
    
bool AudioContext::isSampleRateRangeGood(float sampleRate)
{
    // FIXME: It would be nice if the minimum sample-rate could be less than 44.1KHz,
    // but that will require some fixes in HRTFPanner::fftSizeForSampleRate(), and some testing there.
    return sampleRate >= 44100 && sampleRate <= 96000;
}

// Don't allow more than this number of simultaneous AudioContexts talking to hardware.
const unsigned MaxHardwareContexts = 4;
unsigned AudioContext::s_hardwareContextCount = 0;
    
PassRefPtr<AudioContext> AudioContext::create(Document* document, ExceptionCode& ec)
{
    UNUSED_PARAM(ec);

    ASSERT(document);
    ASSERT(isMainThread());
    if (s_hardwareContextCount >= MaxHardwareContexts)
        return 0;

    RefPtr<AudioContext> audioContext(adoptRef(new AudioContext(document)));
    audioContext->suspendIfNeeded();
    return audioContext.release();
}

// Constructor for rendering to the audio hardware.
AudioContext::AudioContext(Document* document)
    : ActiveDOMObject(document)
    , m_isStopScheduled(false)
    , m_isInitialized(false)
    , m_isAudioThreadFinished(false)
    , m_destinationNode(0)
    , m_isDeletionScheduled(false)
    , m_automaticPullNodesNeedUpdating(false)
    , m_connectionCount(0)
    , m_audioThread(0)
    , m_graphOwnerThread(UndefinedThreadIdentifier)
    , m_isOfflineContext(false)
    , m_activeSourceCount(0)
    , m_restrictions(NoRestrictions)
{
    constructCommon();

    m_destinationNode = DefaultAudioDestinationNode::create(this);

    // This sets in motion an asynchronous loading mechanism on another thread.
    // We can check m_hrtfDatabaseLoader->isLoaded() to find out whether or not it has been fully loaded.
    // It's not that useful to have a callback function for this since the audio thread automatically starts rendering on the graph
    // when this has finished (see AudioDestinationNode).
    m_hrtfDatabaseLoader = HRTFDatabaseLoader::createAndLoadAsynchronouslyIfNecessary(sampleRate());
}

// Constructor for offline (non-realtime) rendering.
AudioContext::AudioContext(Document* document, unsigned numberOfChannels, size_t numberOfFrames, float sampleRate)
    : ActiveDOMObject(document)
    , m_isStopScheduled(false)
    , m_isInitialized(false)
    , m_isAudioThreadFinished(false)
    , m_destinationNode(0)
    , m_automaticPullNodesNeedUpdating(false)
    , m_connectionCount(0)
    , m_audioThread(0)
    , m_graphOwnerThread(UndefinedThreadIdentifier)
    , m_isOfflineContext(true)
    , m_activeSourceCount(0)
    , m_restrictions(NoRestrictions)
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
    // According to spec AudioContext must die only after page navigate.
    // Lets mark it as ActiveDOMObject with pending activity and unmark it in clear method.
    setPendingActivity(this);

#if USE(GSTREAMER)
    initializeGStreamer();
#endif

    FFTFrame::initialize();
    
    m_listener = AudioListener::create();

#if PLATFORM(IOS)
    if (!document()->settings() || document()->settings()->mediaPlaybackRequiresUserGesture())
        addBehaviorRestriction(RequireUserGestureForAudioStartRestriction);
    else
        m_restrictions = NoRestrictions;
#endif

#if PLATFORM(MAC)
    addBehaviorRestriction(RequirePageConsentForAudioStartRestriction);
#endif
}

AudioContext::~AudioContext()
{
#if DEBUG_AUDIONODE_REFERENCES
    fprintf(stderr, "%p: AudioContext::~AudioContext()\n", this);
#endif
    // AudioNodes keep a reference to their context, so there should be no way to be in the destructor if there are still AudioNodes around.
    ASSERT(!m_isInitialized);
    ASSERT(m_isStopScheduled);
    ASSERT(!m_nodesToDelete.size());
    ASSERT(!m_referencedNodes.size());
    ASSERT(!m_finishedNodes.size());
    ASSERT(!m_automaticPullNodes.size());
    if (m_automaticPullNodesNeedUpdating)
        m_renderingAutomaticPullNodes.resize(m_automaticPullNodes.size());
    ASSERT(!m_renderingAutomaticPullNodes.size());
}

void AudioContext::lazyInitialize()
{
    if (!m_isInitialized) {
        // Don't allow the context to initialize a second time after it's already been explicitly uninitialized.
        ASSERT(!m_isAudioThreadFinished);
        if (!m_isAudioThreadFinished) {
            if (m_destinationNode.get()) {
                m_destinationNode->initialize();

                if (!isOfflineContext()) {
                    // This starts the audio thread. The destination node's provideInput() method will now be called repeatedly to render audio.
                    // Each time provideInput() is called, a portion of the audio stream is rendered. Let's call this time period a "render quantum".
                    // NOTE: for now default AudioContext does not need an explicit startRendering() call from JavaScript.
                    // We may want to consider requiring it for symmetry with OfflineAudioContext.
                    startRendering();
                    ++s_hardwareContextCount;
                }

            }
            m_isInitialized = true;
        }
    }
}

void AudioContext::clear()
{
    // We have to release our reference to the destination node before the context will ever be deleted since the destination node holds a reference to the context.
    if (m_destinationNode)
        m_destinationNode.clear();

    // Audio thread is dead. Nobody will schedule node deletion action. Let's do it ourselves.
    do {
        deleteMarkedNodes();
        m_nodesToDelete.appendVector(m_nodesMarkedForDeletion);
        m_nodesMarkedForDeletion.clear();
    } while (m_nodesToDelete.size());

    // It was set in constructCommon.
    unsetPendingActivity(this);
}

void AudioContext::uninitialize()
{
    ASSERT(isMainThread());

    if (!m_isInitialized)
        return;

    // This stops the audio thread and all audio rendering.
    m_destinationNode->uninitialize();

    // Don't allow the context to initialize a second time after it's already been explicitly uninitialized.
    m_isAudioThreadFinished = true;

    if (!isOfflineContext()) {
        ASSERT(s_hardwareContextCount);
        --s_hardwareContextCount;
    }

    // Get rid of the sources which may still be playing.
    derefUnfinishedSourceNodes();

    m_isInitialized = false;
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

void AudioContext::stopDispatch(void* userData)
{
    AudioContext* context = reinterpret_cast<AudioContext*>(userData);
    ASSERT(context);
    if (!context)
        return;

    context->uninitialize();
    context->clear();
}

void AudioContext::stop()
{
    // Usually ScriptExecutionContext calls stop twice.
    if (m_isStopScheduled)
        return;
    m_isStopScheduled = true;

    // Don't call uninitialize() immediately here because the ScriptExecutionContext is in the middle
    // of dealing with all of its ActiveDOMObjects at this point. uninitialize() can de-reference other
    // ActiveDOMObjects so let's schedule uninitialize() to be called later.
    // FIXME: see if there's a more direct way to handle this issue.
    callOnMainThread(stopDispatch, this);
}

Document* AudioContext::document() const
{
    ASSERT(m_scriptExecutionContext && m_scriptExecutionContext->isDocument());
    return static_cast<Document*>(m_scriptExecutionContext);
}

PassRefPtr<AudioBuffer> AudioContext::createBuffer(unsigned numberOfChannels, size_t numberOfFrames, float sampleRate, ExceptionCode& ec)
{
    RefPtr<AudioBuffer> audioBuffer = AudioBuffer::create(numberOfChannels, numberOfFrames, sampleRate);
    if (!audioBuffer.get()) {
        ec = NOT_SUPPORTED_ERR;
        return 0;
    }

    return audioBuffer;
}

PassRefPtr<AudioBuffer> AudioContext::createBuffer(ArrayBuffer* arrayBuffer, bool mixToMono, ExceptionCode& ec)
{
    ASSERT(arrayBuffer);
    if (!arrayBuffer) {
        ec = SYNTAX_ERR;
        return 0;
    }

    RefPtr<AudioBuffer> audioBuffer = AudioBuffer::createFromAudioFileData(arrayBuffer->data(), arrayBuffer->byteLength(), mixToMono, sampleRate());
    if (!audioBuffer.get()) {
        ec = SYNTAX_ERR;
        return 0;
    }

    return audioBuffer;
}

void AudioContext::decodeAudioData(ArrayBuffer* audioData, PassRefPtr<AudioBufferCallback> successCallback, PassRefPtr<AudioBufferCallback> errorCallback, ExceptionCode& ec)
{
    if (!audioData) {
        ec = SYNTAX_ERR;
        return;
    }
    m_audioDecoder.decodeAsync(audioData, sampleRate(), successCallback, errorCallback);
}

PassRefPtr<AudioBufferSourceNode> AudioContext::createBufferSource()
{
    ASSERT(isMainThread());
    lazyInitialize();
    RefPtr<AudioBufferSourceNode> node = AudioBufferSourceNode::create(this, m_destinationNode->sampleRate());

    // Because this is an AudioScheduledSourceNode, the context keeps a reference until it has finished playing.
    // When this happens, AudioScheduledSourceNode::finish() calls AudioContext::notifyNodeFinishedProcessing().
    refNode(node.get());

    return node;
}

#if ENABLE(VIDEO)
PassRefPtr<MediaElementAudioSourceNode> AudioContext::createMediaElementSource(HTMLMediaElement* mediaElement, ExceptionCode& ec)
{
    ASSERT(mediaElement);
    if (!mediaElement) {
        ec = INVALID_STATE_ERR;
        return 0;
    }
        
    ASSERT(isMainThread());
    lazyInitialize();
    
    // First check if this media element already has a source node.
    if (mediaElement->audioSourceNode()) {
        ec = INVALID_STATE_ERR;
        return 0;
    }
        
    RefPtr<MediaElementAudioSourceNode> node = MediaElementAudioSourceNode::create(this, mediaElement);

    mediaElement->setAudioSourceNode(node.get());

    refNode(node.get()); // context keeps reference until node is disconnected
    return node;
}
#endif

#if ENABLE(MEDIA_STREAM)
PassRefPtr<MediaStreamAudioSourceNode> AudioContext::createMediaStreamSource(MediaStream* mediaStream, ExceptionCode& ec)
{
    ASSERT(mediaStream);
    if (!mediaStream) {
        ec = INVALID_STATE_ERR;
        return 0;
    }

    ASSERT(isMainThread());
    lazyInitialize();

    AudioSourceProvider* provider = 0;

    MediaStreamTrackVector audioTracks = mediaStream->getAudioTracks();
    if (mediaStream->isLocal() && audioTracks.size()) {
        // Enable input for the specific local audio device specified in the MediaStreamSource.
        RefPtr<MediaStreamTrack> localAudio = audioTracks[0];
        MediaStreamSource* source = localAudio->component()->source();
        destination()->enableInput(source->deviceId());
        provider = destination()->localAudioInputProvider();
    } else {
        // FIXME: get a provider for non-local MediaStreams (like from a remote peer).
        provider = 0;
    }

    RefPtr<MediaStreamAudioSourceNode> node = MediaStreamAudioSourceNode::create(this, mediaStream, provider);

    // FIXME: Only stereo streams are supported right now. We should be able to accept multi-channel streams.
    node->setFormat(2, sampleRate());

    refNode(node.get()); // context keeps reference until node is disconnected
    return node;
}

PassRefPtr<MediaStreamAudioDestinationNode> AudioContext::createMediaStreamDestination()
{
    // FIXME: Add support for an optional argument which specifies the number of channels.
    // FIXME: The default should probably be stereo instead of mono.
    return MediaStreamAudioDestinationNode::create(this, 1);
}

#endif

PassRefPtr<ScriptProcessorNode> AudioContext::createScriptProcessor(size_t bufferSize, ExceptionCode& ec)
{
    // Set number of input/output channels to stereo by default.
    return createScriptProcessor(bufferSize, 2, 2, ec);
}

PassRefPtr<ScriptProcessorNode> AudioContext::createScriptProcessor(size_t bufferSize, size_t numberOfInputChannels, ExceptionCode& ec)
{
    // Set number of output channels to stereo by default.
    return createScriptProcessor(bufferSize, numberOfInputChannels, 2, ec);
}

PassRefPtr<ScriptProcessorNode> AudioContext::createScriptProcessor(size_t bufferSize, size_t numberOfInputChannels, size_t numberOfOutputChannels, ExceptionCode& ec)
{
    ASSERT(isMainThread());
    lazyInitialize();
    RefPtr<ScriptProcessorNode> node = ScriptProcessorNode::create(this, m_destinationNode->sampleRate(), bufferSize, numberOfInputChannels, numberOfOutputChannels);

    if (!node.get()) {
        ec = INDEX_SIZE_ERR;
        return 0;
    }

    refNode(node.get()); // context keeps reference until we stop making javascript rendering callbacks
    return node;
}

PassRefPtr<BiquadFilterNode> AudioContext::createBiquadFilter()
{
    ASSERT(isMainThread());
    lazyInitialize();
    return BiquadFilterNode::create(this, m_destinationNode->sampleRate());
}

PassRefPtr<WaveShaperNode> AudioContext::createWaveShaper()
{
    ASSERT(isMainThread());
    lazyInitialize();
    return WaveShaperNode::create(this);
}

PassRefPtr<PannerNode> AudioContext::createPanner()
{
    ASSERT(isMainThread());
    lazyInitialize();
    return PannerNode::create(this, m_destinationNode->sampleRate());
}

PassRefPtr<ConvolverNode> AudioContext::createConvolver()
{
    ASSERT(isMainThread());
    lazyInitialize();
    return ConvolverNode::create(this, m_destinationNode->sampleRate());
}

PassRefPtr<DynamicsCompressorNode> AudioContext::createDynamicsCompressor()
{
    ASSERT(isMainThread());
    lazyInitialize();
    return DynamicsCompressorNode::create(this, m_destinationNode->sampleRate());
}

PassRefPtr<AnalyserNode> AudioContext::createAnalyser()
{
    ASSERT(isMainThread());
    lazyInitialize();
    return AnalyserNode::create(this, m_destinationNode->sampleRate());
}

PassRefPtr<GainNode> AudioContext::createGain()
{
    ASSERT(isMainThread());
    lazyInitialize();
    return GainNode::create(this, m_destinationNode->sampleRate());
}

PassRefPtr<DelayNode> AudioContext::createDelay(ExceptionCode& ec)
{
    const double defaultMaxDelayTime = 1;
    return createDelay(defaultMaxDelayTime, ec);
}

PassRefPtr<DelayNode> AudioContext::createDelay(double maxDelayTime, ExceptionCode& ec)
{
    ASSERT(isMainThread());
    lazyInitialize();
    RefPtr<DelayNode> node = DelayNode::create(this, m_destinationNode->sampleRate(), maxDelayTime, ec);
    if (ec)
        return 0;
    return node;
}

PassRefPtr<ChannelSplitterNode> AudioContext::createChannelSplitter(ExceptionCode& ec)
{
    const unsigned ChannelSplitterDefaultNumberOfOutputs = 6;
    return createChannelSplitter(ChannelSplitterDefaultNumberOfOutputs, ec);
}

PassRefPtr<ChannelSplitterNode> AudioContext::createChannelSplitter(size_t numberOfOutputs, ExceptionCode& ec)
{
    ASSERT(isMainThread());
    lazyInitialize();

    RefPtr<ChannelSplitterNode> node = ChannelSplitterNode::create(this, m_destinationNode->sampleRate(), numberOfOutputs);

    if (!node.get()) {
        ec = SYNTAX_ERR;
        return 0;
    }

    return node;
}

PassRefPtr<ChannelMergerNode> AudioContext::createChannelMerger(ExceptionCode& ec)
{
    const unsigned ChannelMergerDefaultNumberOfInputs = 6;
    return createChannelMerger(ChannelMergerDefaultNumberOfInputs, ec);
}

PassRefPtr<ChannelMergerNode> AudioContext::createChannelMerger(size_t numberOfInputs, ExceptionCode& ec)
{
    ASSERT(isMainThread());
    lazyInitialize();

    RefPtr<ChannelMergerNode> node = ChannelMergerNode::create(this, m_destinationNode->sampleRate(), numberOfInputs);

    if (!node.get()) {
        ec = SYNTAX_ERR;
        return 0;
    }

    return node;
}

PassRefPtr<OscillatorNode> AudioContext::createOscillator()
{
    ASSERT(isMainThread());
    lazyInitialize();

    RefPtr<OscillatorNode> node = OscillatorNode::create(this, m_destinationNode->sampleRate());

    // Because this is an AudioScheduledSourceNode, the context keeps a reference until it has finished playing.
    // When this happens, AudioScheduledSourceNode::finish() calls AudioContext::notifyNodeFinishedProcessing().
    refNode(node.get());

    return node;
}

PassRefPtr<PeriodicWave> AudioContext::createPeriodicWave(Float32Array* real, Float32Array* imag, ExceptionCode& ec)
{
    ASSERT(isMainThread());
    
    if (!real || !imag || (real->length() != imag->length() || (real->length() > MaxPeriodicWaveLength) || (real->length() <= 0))) {
        ec = SYNTAX_ERR;
        return 0;
    }
    
    lazyInitialize();
    return PeriodicWave::create(sampleRate(), real, imag);
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

void AudioContext::addDeferredFinishDeref(AudioNode* node)
{
    ASSERT(isAudioThread());
    m_deferredFinishDerefList.append(node);
}

void AudioContext::handlePreRenderTasks()
{
    ASSERT(isAudioThread());
 
    // At the beginning of every render quantum, try to update the internal rendering graph state (from main thread changes).
    // It's OK if the tryLock() fails, we'll just take slightly longer to pick up the changes.
    bool mustReleaseLock;
    if (tryLock(mustReleaseLock)) {
        // Fixup the state of any dirty AudioSummingJunctions and AudioNodeOutputs.
        handleDirtyAudioSummingJunctions();
        handleDirtyAudioNodeOutputs();

        updateAutomaticPullNodes();

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

        // Don't delete in the real-time thread. Let the main thread do it.
        // Ref-counted objects held by certain AudioNodes may not be thread-safe.
        scheduleNodeDeletion();

        // Fixup the state of any dirty AudioSummingJunctions and AudioNodeOutputs.
        handleDirtyAudioSummingJunctions();
        handleDirtyAudioNodeOutputs();

        updateAutomaticPullNodes();

        if (mustReleaseLock)
            unlock();
    }
}

void AudioContext::handleDeferredFinishDerefs()
{
    ASSERT(isAudioThread() && isGraphOwner());
    for (unsigned i = 0; i < m_deferredFinishDerefList.size(); ++i) {
        AudioNode* node = m_deferredFinishDerefList[i];
        node->finishDeref(AudioNode::RefTypeConnection);
    }
    
    m_deferredFinishDerefList.clear();
}

void AudioContext::markForDeletion(AudioNode* node)
{
    ASSERT(isGraphOwner());

    if (isAudioThreadFinished())
        m_nodesToDelete.append(node);
    else
        m_nodesMarkedForDeletion.append(node);

    // This is probably the best time for us to remove the node from automatic pull list,
    // since all connections are gone and we hold the graph lock. Then when handlePostRenderTasks()
    // gets a chance to schedule the deletion work, updateAutomaticPullNodes() also gets a chance to
    // modify m_renderingAutomaticPullNodes.
    removeAutomaticPullNode(node);
}

void AudioContext::scheduleNodeDeletion()
{
    bool isGood = m_isInitialized && isGraphOwner();
    ASSERT(isGood);
    if (!isGood)
        return;

    // Make sure to call deleteMarkedNodes() on main thread.    
    if (m_nodesMarkedForDeletion.size() && !m_isDeletionScheduled) {
        m_nodesToDelete.appendVector(m_nodesMarkedForDeletion);
        m_nodesMarkedForDeletion.clear();

        m_isDeletionScheduled = true;

        // Don't let ourself get deleted before the callback.
        // See matching deref() in deleteMarkedNodesDispatch().
        ref();
        callOnMainThread(deleteMarkedNodesDispatch, this);
    }
}

void AudioContext::deleteMarkedNodesDispatch(void* userData)
{
    AudioContext* context = reinterpret_cast<AudioContext*>(userData);
    ASSERT(context);
    if (!context)
        return;

    context->deleteMarkedNodes();
    context->deref();
}

void AudioContext::deleteMarkedNodes()
{
    ASSERT(isMainThread());

    // Protect this object from being deleted before we release the mutex locked by AutoLocker.
    RefPtr<AudioContext> protect(this);
    {
        AutoLocker locker(this);

        while (size_t n = m_nodesToDelete.size()) {
            AudioNode* node = m_nodesToDelete[n - 1];
            m_nodesToDelete.removeLast();

            // Before deleting the node, clear out any AudioNodeInputs from m_dirtySummingJunctions.
            unsigned numberOfInputs = node->numberOfInputs();
            for (unsigned i = 0; i < numberOfInputs; ++i)
                m_dirtySummingJunctions.remove(node->input(i));

            // Before deleting the node, clear out any AudioNodeOutputs from m_dirtyAudioNodeOutputs.
            unsigned numberOfOutputs = node->numberOfOutputs();
            for (unsigned i = 0; i < numberOfOutputs; ++i)
                m_dirtyAudioNodeOutputs.remove(node->output(i));

            // Finally, delete it.
            delete node;
        }
        m_isDeletionScheduled = false;
    }
}

void AudioContext::markSummingJunctionDirty(AudioSummingJunction* summingJunction)
{
    ASSERT(isGraphOwner());    
    m_dirtySummingJunctions.add(summingJunction);
}

void AudioContext::removeMarkedSummingJunction(AudioSummingJunction* summingJunction)
{
    ASSERT(isMainThread());
    AutoLocker locker(this);
    m_dirtySummingJunctions.remove(summingJunction);
}

void AudioContext::markAudioNodeOutputDirty(AudioNodeOutput* output)
{
    ASSERT(isGraphOwner());    
    m_dirtyAudioNodeOutputs.add(output);
}

void AudioContext::handleDirtyAudioSummingJunctions()
{
    ASSERT(isGraphOwner());    

    for (HashSet<AudioSummingJunction*>::iterator i = m_dirtySummingJunctions.begin(); i != m_dirtySummingJunctions.end(); ++i)
        (*i)->updateRenderingState();

    m_dirtySummingJunctions.clear();
}

void AudioContext::handleDirtyAudioNodeOutputs()
{
    ASSERT(isGraphOwner());    

    for (HashSet<AudioNodeOutput*>::iterator i = m_dirtyAudioNodeOutputs.begin(); i != m_dirtyAudioNodeOutputs.end(); ++i)
        (*i)->updateRenderingState();

    m_dirtyAudioNodeOutputs.clear();
}

void AudioContext::addAutomaticPullNode(AudioNode* node)
{
    ASSERT(isGraphOwner());

    if (!m_automaticPullNodes.contains(node)) {
        m_automaticPullNodes.add(node);
        m_automaticPullNodesNeedUpdating = true;
    }
}

void AudioContext::removeAutomaticPullNode(AudioNode* node)
{
    ASSERT(isGraphOwner());

    if (m_automaticPullNodes.contains(node)) {
        m_automaticPullNodes.remove(node);
        m_automaticPullNodesNeedUpdating = true;
    }
}

void AudioContext::updateAutomaticPullNodes()
{
    ASSERT(isGraphOwner());

    if (m_automaticPullNodesNeedUpdating) {
        // Copy from m_automaticPullNodes to m_renderingAutomaticPullNodes.
        m_renderingAutomaticPullNodes.resize(m_automaticPullNodes.size());

        unsigned j = 0;
        for (HashSet<AudioNode*>::iterator i = m_automaticPullNodes.begin(); i != m_automaticPullNodes.end(); ++i, ++j) {
            AudioNode* output = *i;
            m_renderingAutomaticPullNodes[j] = output;
        }

        m_automaticPullNodesNeedUpdating = false;
    }
}

void AudioContext::processAutomaticPullNodes(size_t framesToProcess)
{
    ASSERT(isAudioThread());

    for (unsigned i = 0; i < m_renderingAutomaticPullNodes.size(); ++i)
        m_renderingAutomaticPullNodes[i]->processIfNecessary(framesToProcess);
}

const AtomicString& AudioContext::interfaceName() const
{
    return eventNames().interfaceForAudioContext;
}

ScriptExecutionContext* AudioContext::scriptExecutionContext() const
{
    return m_isStopScheduled ? 0 : ActiveDOMObject::scriptExecutionContext();
}

void AudioContext::startRendering()
{
    if (ScriptController::processingUserGesture())
        removeBehaviorRestriction(AudioContext::RequireUserGestureForAudioStartRestriction);

    if (pageConsentRequiredForAudioStart()) {
        Page* page = document()->page();
        if (page && !page->canStartMedia())
            document()->addMediaCanStartListener(this);
        else
            removeBehaviorRestriction(AudioContext::RequirePageConsentForAudioStartRestriction);
    }
    destination()->startRendering();
}

void AudioContext::mediaCanStart()
{
    removeBehaviorRestriction(AudioContext::RequirePageConsentForAudioStartRestriction);
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
    if (scriptExecutionContext()) {
        // Call the offline rendering completion event listener.
        dispatchEvent(OfflineAudioCompletionEvent::create(renderedBuffer));
    }
}

void AudioContext::incrementActiveSourceCount()
{
    atomicIncrement(&m_activeSourceCount);
}

void AudioContext::decrementActiveSourceCount()
{
    atomicDecrement(&m_activeSourceCount);
}

} // namespace WebCore

#endif // ENABLE(WEB_AUDIO)
