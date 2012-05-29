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
#include "AudioBus.h"
#include "AudioDestinationNode.h"
#include "EventListener.h"
#include "EventTarget.h"
#include "HRTFDatabaseLoader.h"
#include <wtf/HashSet.h>
#include <wtf/OwnPtr.h>
#include <wtf/PassRefPtr.h>
#include <wtf/RefCounted.h>
#include <wtf/RefPtr.h>
#include <wtf/Threading.h>
#include <wtf/Vector.h>
#include <wtf/text/AtomicStringHash.h>

namespace WebCore {

class ArrayBuffer;
class AudioBuffer;
class AudioBufferSourceNode;
class AudioChannelMerger;
class AudioChannelSplitter;
class AudioGainNode;
class AudioPannerNode;
class AudioListener;
class DelayNode;
class Document;
class LowPass2FilterNode;
class HighPass2FilterNode;
class ConvolverNode;
class RealtimeAnalyserNode;
class JavaScriptAudioNode;

// AudioContext is the cornerstone of the web audio API and all AudioNodes are created from it.
// For thread safety between the audio thread and the main thread, it has a rendering graph locking mechanism. 

class AudioContext : public ActiveDOMObject, public RefCounted<AudioContext>, public EventTarget {
public:
    // Create an AudioContext for rendering to the audio hardware.
    static PassRefPtr<AudioContext> create(Document*);

    // Create an AudioContext for offline (non-realtime) rendering.
    static PassRefPtr<AudioContext> createOfflineContext(Document*, unsigned numberOfChannels, size_t numberOfFrames, double sampleRate);

    virtual ~AudioContext();

    bool isInitialized() const;
    
    bool isOfflineContext() { return m_isOfflineContext; }

    // Returns true when initialize() was called AND all asynchronous initialization has completed.
    bool isRunnable() const;

    // Document notification
    virtual void stop();

    Document* document() const; // ASSERTs if document no longer exists.
    bool hasDocument();

    AudioDestinationNode* destination() { return m_destinationNode.get(); }
    double currentTime() { return m_destinationNode->currentTime(); }
    double sampleRate() { return m_destinationNode->sampleRate(); }

    PassRefPtr<AudioBuffer> createBuffer(unsigned numberOfChannels, size_t numberOfFrames, double sampleRate);
    PassRefPtr<AudioBuffer> createBuffer(ArrayBuffer* arrayBuffer, bool mixToMono);

    // Keep track of this buffer so we can release memory after the context is shut down...
    void refBuffer(PassRefPtr<AudioBuffer> buffer);

    AudioListener* listener() { return m_listener.get(); }

    // The AudioNode create methods are called on the main thread (from JavaScript).
    PassRefPtr<AudioBufferSourceNode> createBufferSource();
    PassRefPtr<AudioGainNode> createGainNode();
    PassRefPtr<DelayNode> createDelayNode();
    PassRefPtr<LowPass2FilterNode> createLowPass2Filter();
    PassRefPtr<HighPass2FilterNode> createHighPass2Filter();
    PassRefPtr<AudioPannerNode> createPanner();
    PassRefPtr<ConvolverNode> createConvolver();
    PassRefPtr<RealtimeAnalyserNode> createAnalyser();
    PassRefPtr<JavaScriptAudioNode> createJavaScriptNode(size_t bufferSize);
    PassRefPtr<AudioChannelSplitter> createChannelSplitter();
    PassRefPtr<AudioChannelMerger> createChannelMerger();

    AudioBus* temporaryMonoBus() { return m_temporaryMonoBus.get(); }
    AudioBus* temporaryStereoBus() { return m_temporaryStereoBus.get(); }

    // When a source node has no more processing to do (has finished playing), then it tells the context to dereference it.
    void notifyNodeFinishedProcessing(AudioNode*);

    // Called at the start of each render quantum.
    void handlePreRenderTasks();

    // Called at the end of each render quantum.
    void handlePostRenderTasks();

    // Called periodically at the end of each render quantum to dereference finished source nodes.
    void derefFinishedSourceNodes();

    // We reap all marked nodes at the end of each realtime render quantum in deleteMarkedNodes().
    void markForDeletion(AudioNode*);
    void deleteMarkedNodes();

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
    void addDeferredFinishDeref(AudioNode*, AudioNode::RefType);

    // In the audio thread at the start of each render cycle, we'll call handleDeferredFinishDerefs().
    void handleDeferredFinishDerefs();

    // Only accessed when the graph lock is held.
    void markAudioNodeInputDirty(AudioNodeInput*);
    void markAudioNodeOutputDirty(AudioNodeOutput*);

    // EventTarget
    virtual ScriptExecutionContext* scriptExecutionContext() const;
    virtual AudioContext* toAudioContext();
    virtual EventTargetData* eventTargetData() { return &m_eventTargetData; }
    virtual EventTargetData* ensureEventTargetData() { return &m_eventTargetData; }

    DEFINE_ATTRIBUTE_EVENT_LISTENER(complete);

    // Reconcile ref/deref which are defined both in AudioNode and EventTarget.
    using RefCounted<AudioContext>::ref;
    using RefCounted<AudioContext>::deref;

    void startRendering();
    void fireCompletionEvent();
    
private:
    AudioContext(Document*);
    AudioContext(Document*, unsigned numberOfChannels, size_t numberOfFrames, double sampleRate);
    void constructCommon();

    void lazyInitialize();
    void uninitialize();
    
    bool m_isInitialized;
    bool m_isAudioThreadFinished;
    bool m_isAudioThreadShutdown;

    Document* m_document;

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

    // Only accessed in the main thread.
    Vector<RefPtr<AudioBuffer> > m_allocatedBuffers;

    // Only accessed in the audio thread.
    Vector<AudioNode*> m_finishedNodes;

    // We don't use RefPtr<AudioNode> here because AudioNode has a more complex ref() / deref() implementation
    // with an optional argument for refType.  We need to use the special refType: RefTypeConnection
    // Either accessed when the graph lock is held, or on the main thread when the audio thread has finished.
    Vector<AudioNode*> m_referencedNodes;

    // Accumulate nodes which need to be deleted at the end of a render cycle (in realtime thread) here.
    Vector<AudioNode*> m_nodesToDelete;

    // Only accessed when the graph lock is held.
    HashSet<AudioNodeInput*> m_dirtyAudioNodeInputs;
    HashSet<AudioNodeOutput*> m_dirtyAudioNodeOutputs;
    void handleDirtyAudioNodeInputs();
    void handleDirtyAudioNodeOutputs();

    OwnPtr<AudioBus> m_temporaryMonoBus;
    OwnPtr<AudioBus> m_temporaryStereoBus;

    unsigned m_connectionCount;

    // Graph locking.
    Mutex m_contextGraphMutex;
    volatile ThreadIdentifier m_audioThread;
    volatile ThreadIdentifier m_graphOwnerThread; // if the lock is held then this is the thread which owns it, otherwise == UndefinedThreadIdentifier
    
    // Deferred de-referencing.
    struct RefInfo {
        RefInfo(AudioNode* node, AudioNode::RefType refType)
            : m_node(node)
            , m_refType(refType)
        {
        }
        AudioNode* m_node;
        AudioNode::RefType m_refType;
    };    

    // Only accessed in the audio thread.
    Vector<RefInfo> m_deferredFinishDerefList;
    
    // HRTF Database loader
    RefPtr<HRTFDatabaseLoader> m_hrtfDatabaseLoader;

    // EventTarget
    virtual void refEventTarget() { ref(); }
    virtual void derefEventTarget() { deref(); }
    EventTargetData m_eventTargetData;

    RefPtr<AudioBuffer> m_renderTarget;
    
    bool m_isOfflineContext;
};

} // WebCore

#endif // AudioContext_h
