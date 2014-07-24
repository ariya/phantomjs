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

#ifndef AudioContext_h
#define AudioContext_h

#include "ActiveDOMObject.h"
#include "AsyncAudioDecoder.h"
#include "AudioBus.h"
#include "AudioDestinationNode.h"
#include "EventListener.h"
#include "EventTarget.h"
#include "MediaCanStartListener.h"
#include <wtf/HashSet.h>
#include <wtf/MainThread.h>
#include <wtf/OwnPtr.h>
#include <wtf/PassRefPtr.h>
#include <wtf/RefCounted.h>
#include <wtf/RefPtr.h>
#include <wtf/ThreadSafeRefCounted.h>
#include <wtf/Threading.h>
#include <wtf/Vector.h>
#include <wtf/text/AtomicStringHash.h>

namespace WebCore {

class AudioBuffer;
class AudioBufferCallback;
class AudioBufferSourceNode;
class MediaElementAudioSourceNode;
class MediaStreamAudioDestinationNode;
class MediaStreamAudioSourceNode;
class HRTFDatabaseLoader;
class HTMLMediaElement;
class ChannelMergerNode;
class ChannelSplitterNode;
class GainNode;
class PannerNode;
class AudioListener;
class AudioSummingJunction;
class BiquadFilterNode;
class DelayNode;
class Document;
class ConvolverNode;
class DynamicsCompressorNode;
class AnalyserNode;
class WaveShaperNode;
class ScriptProcessorNode;
class OscillatorNode;
class PeriodicWave;

// AudioContext is the cornerstone of the web audio API and all AudioNodes are created from it.
// For thread safety between the audio thread and the main thread, it has a rendering graph locking mechanism. 

class AudioContext : public ActiveDOMObject, public ThreadSafeRefCounted<AudioContext>, public EventTarget, public MediaCanStartListener {
public:
    // Create an AudioContext for rendering to the audio hardware.
    static PassRefPtr<AudioContext> create(Document*, ExceptionCode&);

    // Create an AudioContext for offline (non-realtime) rendering.
    static PassRefPtr<AudioContext> createOfflineContext(Document*, unsigned numberOfChannels, size_t numberOfFrames, float sampleRate, ExceptionCode&);

    virtual ~AudioContext();

    bool isInitialized() const;
    
    bool isOfflineContext() { return m_isOfflineContext; }

    // Returns true when initialize() was called AND all asynchronous initialization has completed.
    bool isRunnable() const;

    HRTFDatabaseLoader* hrtfDatabaseLoader() const { return m_hrtfDatabaseLoader.get(); }

    // Document notification
    virtual void stop();

    Document* document() const; // ASSERTs if document no longer exists.
    bool hasDocument();

    AudioDestinationNode* destination() { return m_destinationNode.get(); }
    size_t currentSampleFrame() const { return m_destinationNode->currentSampleFrame(); }
    double currentTime() const { return m_destinationNode->currentTime(); }
    float sampleRate() const { return m_destinationNode->sampleRate(); }
    unsigned long activeSourceCount() const { return static_cast<unsigned long>(m_activeSourceCount); }

    void incrementActiveSourceCount();
    void decrementActiveSourceCount();
    
    PassRefPtr<AudioBuffer> createBuffer(unsigned numberOfChannels, size_t numberOfFrames, float sampleRate, ExceptionCode&);
    PassRefPtr<AudioBuffer> createBuffer(ArrayBuffer*, bool mixToMono, ExceptionCode&);

    // Asynchronous audio file data decoding.
    void decodeAudioData(ArrayBuffer*, PassRefPtr<AudioBufferCallback>, PassRefPtr<AudioBufferCallback>, ExceptionCode& ec);

    AudioListener* listener() { return m_listener.get(); }

    // The AudioNode create methods are called on the main thread (from JavaScript).
    PassRefPtr<AudioBufferSourceNode> createBufferSource();
#if ENABLE(VIDEO)
    PassRefPtr<MediaElementAudioSourceNode> createMediaElementSource(HTMLMediaElement*, ExceptionCode&);
#endif
#if ENABLE(MEDIA_STREAM)
    PassRefPtr<MediaStreamAudioSourceNode> createMediaStreamSource(MediaStream*, ExceptionCode&);
    PassRefPtr<MediaStreamAudioDestinationNode> createMediaStreamDestination();
#endif
    PassRefPtr<GainNode> createGain();
    PassRefPtr<BiquadFilterNode> createBiquadFilter();
    PassRefPtr<WaveShaperNode> createWaveShaper();
    PassRefPtr<DelayNode> createDelay(ExceptionCode&);
    PassRefPtr<DelayNode> createDelay(double maxDelayTime, ExceptionCode&);
    PassRefPtr<PannerNode> createPanner();
    PassRefPtr<ConvolverNode> createConvolver();
    PassRefPtr<DynamicsCompressorNode> createDynamicsCompressor();    
    PassRefPtr<AnalyserNode> createAnalyser();
    PassRefPtr<ScriptProcessorNode> createScriptProcessor(size_t bufferSize, ExceptionCode&);
    PassRefPtr<ScriptProcessorNode> createScriptProcessor(size_t bufferSize, size_t numberOfInputChannels, ExceptionCode&);
    PassRefPtr<ScriptProcessorNode> createScriptProcessor(size_t bufferSize, size_t numberOfInputChannels, size_t numberOfOutputChannels, ExceptionCode&);
    PassRefPtr<ChannelSplitterNode> createChannelSplitter(ExceptionCode&);
    PassRefPtr<ChannelSplitterNode> createChannelSplitter(size_t numberOfOutputs, ExceptionCode&);
    PassRefPtr<ChannelMergerNode> createChannelMerger(ExceptionCode&);
    PassRefPtr<ChannelMergerNode> createChannelMerger(size_t numberOfInputs, ExceptionCode&);
    PassRefPtr<OscillatorNode> createOscillator();
    PassRefPtr<PeriodicWave> createPeriodicWave(Float32Array* real, Float32Array* imag, ExceptionCode&);

    // When a source node has no more processing to do (has finished playing), then it tells the context to dereference it.
    void notifyNodeFinishedProcessing(AudioNode*);

    // Called at the start of each render quantum.
    void handlePreRenderTasks();

    // Called at the end of each render quantum.
    void handlePostRenderTasks();

    // Called periodically at the end of each render quantum to dereference finished source nodes.
    void derefFinishedSourceNodes();

    // We schedule deletion of all marked nodes at the end of each realtime render quantum.
    void markForDeletion(AudioNode*);
    void deleteMarkedNodes();

    // AudioContext can pull node(s) at the end of each render quantum even when they are not connected to any downstream nodes.
    // These two methods are called by the nodes who want to add/remove themselves into/from the automatic pull lists.
    void addAutomaticPullNode(AudioNode*);
    void removeAutomaticPullNode(AudioNode*);

    // Called right before handlePostRenderTasks() to handle nodes which need to be pulled even when they are not connected to anything.
    void processAutomaticPullNodes(size_t framesToProcess);

    // Keeps track of the number of connections made.
    void incrementConnectionCount()
    {
        ASSERT(isMainThread());
        m_connectionCount++;
    }

    unsigned connectionCount() const { return m_connectionCount; }

    //
    // Thread Safety and Graph Locking:
    //
    
    void setAudioThread(ThreadIdentifier thread) { m_audioThread = thread; } // FIXME: check either not initialized or the same
    ThreadIdentifier audioThread() const { return m_audioThread; }
    bool isAudioThread() const;

    // Returns true only after the audio thread has been started and then shutdown.
    bool isAudioThreadFinished() { return m_isAudioThreadFinished; }
    
    // mustReleaseLock is set to true if we acquired the lock in this method call and caller must unlock(), false if it was previously acquired.
    void lock(bool& mustReleaseLock);

    // Returns true if we own the lock.
    // mustReleaseLock is set to true if we acquired the lock in this method call and caller must unlock(), false if it was previously acquired.
    bool tryLock(bool& mustReleaseLock);

    void unlock();

    // Returns true if this thread owns the context's lock.
    bool isGraphOwner() const;

    // Returns the maximum numuber of channels we can support.
    static unsigned maxNumberOfChannels() { return MaxNumberOfChannels;}

    class AutoLocker {
    public:
        AutoLocker(AudioContext* context)
            : m_context(context)
        {
            ASSERT(context);
            context->lock(m_mustReleaseLock);
        }
        
        ~AutoLocker()
        {
            if (m_mustReleaseLock)
                m_context->unlock();
        }
    private:
        AudioContext* m_context;
        bool m_mustReleaseLock;
    };
    
    // In AudioNode::deref() a tryLock() is used for calling finishDeref(), but if it fails keep track here.
    void addDeferredFinishDeref(AudioNode*);

    // In the audio thread at the start of each render cycle, we'll call handleDeferredFinishDerefs().
    void handleDeferredFinishDerefs();

    // Only accessed when the graph lock is held.
    void markSummingJunctionDirty(AudioSummingJunction*);
    void markAudioNodeOutputDirty(AudioNodeOutput*);

    // Must be called on main thread.
    void removeMarkedSummingJunction(AudioSummingJunction*);

    // EventTarget
    virtual const AtomicString& interfaceName() const;
    virtual ScriptExecutionContext* scriptExecutionContext() const;
    virtual EventTargetData* eventTargetData() { return &m_eventTargetData; }
    virtual EventTargetData* ensureEventTargetData() { return &m_eventTargetData; }

    DEFINE_ATTRIBUTE_EVENT_LISTENER(complete);

    // Reconcile ref/deref which are defined both in ThreadSafeRefCounted and EventTarget.
    using ThreadSafeRefCounted<AudioContext>::ref;
    using ThreadSafeRefCounted<AudioContext>::deref;

    void startRendering();
    void fireCompletionEvent();
    
    static unsigned s_hardwareContextCount;


    // Restrictions to change default behaviors.
    enum BehaviorRestrictionFlags {
        NoRestrictions = 0,
        RequireUserGestureForAudioStartRestriction = 1 << 0,
        RequirePageConsentForAudioStartRestriction = 1 << 1,
    };
    typedef unsigned BehaviorRestrictions;

    bool userGestureRequiredForAudioStart() const { return m_restrictions & RequireUserGestureForAudioStartRestriction; }
    bool pageConsentRequiredForAudioStart() const { return m_restrictions & RequirePageConsentForAudioStartRestriction; }

    void addBehaviorRestriction(BehaviorRestrictions restriction) { m_restrictions |= restriction; }
    void removeBehaviorRestriction(BehaviorRestrictions restriction) { m_restrictions &= ~restriction; }

protected:
    explicit AudioContext(Document*);
    AudioContext(Document*, unsigned numberOfChannels, size_t numberOfFrames, float sampleRate);
    
    static bool isSampleRateRangeGood(float sampleRate);
    
private:
    void constructCommon();

    void lazyInitialize();
    void uninitialize();

    // ScriptExecutionContext calls stop twice.
    // We'd like to schedule only one stop action for them.
    bool m_isStopScheduled;
    static void stopDispatch(void* userData);
    void clear();

    void scheduleNodeDeletion();
    static void deleteMarkedNodesDispatch(void* userData);

    virtual void mediaCanStart() OVERRIDE;

    bool m_isInitialized;
    bool m_isAudioThreadFinished;

    // The context itself keeps a reference to all source nodes.  The source nodes, then reference all nodes they're connected to.
    // In turn, these nodes reference all nodes they're connected to.  All nodes are ultimately connected to the AudioDestinationNode.
    // When the context dereferences a source node, it will be deactivated from the rendering graph along with all other nodes it is
    // uniquely connected to.  See the AudioNode::ref() and AudioNode::deref() methods for more details.
    void refNode(AudioNode*);
    void derefNode(AudioNode*);

    // When the context goes away, there might still be some sources which haven't finished playing.
    // Make sure to dereference them here.
    void derefUnfinishedSourceNodes();

    RefPtr<AudioDestinationNode> m_destinationNode;
    RefPtr<AudioListener> m_listener;

    // Only accessed in the audio thread.
    Vector<AudioNode*> m_finishedNodes;

    // We don't use RefPtr<AudioNode> here because AudioNode has a more complex ref() / deref() implementation
    // with an optional argument for refType.  We need to use the special refType: RefTypeConnection
    // Either accessed when the graph lock is held, or on the main thread when the audio thread has finished.
    Vector<AudioNode*> m_referencedNodes;

    // Accumulate nodes which need to be deleted here.
    // This is copied to m_nodesToDelete at the end of a render cycle in handlePostRenderTasks(), where we're assured of a stable graph
    // state which will have no references to any of the nodes in m_nodesToDelete once the context lock is released
    // (when handlePostRenderTasks() has completed).
    Vector<AudioNode*> m_nodesMarkedForDeletion;

    // They will be scheduled for deletion (on the main thread) at the end of a render cycle (in realtime thread).
    Vector<AudioNode*> m_nodesToDelete;
    bool m_isDeletionScheduled;

    // Only accessed when the graph lock is held.
    HashSet<AudioSummingJunction*> m_dirtySummingJunctions;
    HashSet<AudioNodeOutput*> m_dirtyAudioNodeOutputs;
    void handleDirtyAudioSummingJunctions();
    void handleDirtyAudioNodeOutputs();

    // For the sake of thread safety, we maintain a seperate Vector of automatic pull nodes for rendering in m_renderingAutomaticPullNodes.
    // It will be copied from m_automaticPullNodes by updateAutomaticPullNodes() at the very start or end of the rendering quantum.
    HashSet<AudioNode*> m_automaticPullNodes;
    Vector<AudioNode*> m_renderingAutomaticPullNodes;
    // m_automaticPullNodesNeedUpdating keeps track if m_automaticPullNodes is modified.
    bool m_automaticPullNodesNeedUpdating;
    void updateAutomaticPullNodes();

    unsigned m_connectionCount;

    // Graph locking.
    Mutex m_contextGraphMutex;
    volatile ThreadIdentifier m_audioThread;
    volatile ThreadIdentifier m_graphOwnerThread; // if the lock is held then this is the thread which owns it, otherwise == UndefinedThreadIdentifier
    
    // Only accessed in the audio thread.
    Vector<AudioNode*> m_deferredFinishDerefList;
    
    // HRTF Database loader
    RefPtr<HRTFDatabaseLoader> m_hrtfDatabaseLoader;

    // EventTarget
    virtual void refEventTarget() { ref(); }
    virtual void derefEventTarget() { deref(); }
    EventTargetData m_eventTargetData;

    RefPtr<AudioBuffer> m_renderTarget;
    
    bool m_isOfflineContext;

    AsyncAudioDecoder m_audioDecoder;

    // This is considering 32 is large enough for multiple channels audio. 
    // It is somewhat arbitrary and could be increased if necessary.
    enum { MaxNumberOfChannels = 32 };

    // Number of AudioBufferSourceNodes that are active (playing).
    int m_activeSourceCount;

    BehaviorRestrictions m_restrictions;
};

} // WebCore

#endif // AudioContext_h
