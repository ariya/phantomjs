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

#ifndef AudioNode_h
#define AudioNode_h

#include "AudioBus.h"
#include "EventTarget.h"
#include <wtf/Forward.h>
#include <wtf/OwnPtr.h>
#include <wtf/PassOwnPtr.h>
#include <wtf/RefPtr.h>
#include <wtf/Vector.h>

#define DEBUG_AUDIONODE_REFERENCES 0

namespace WebCore {

class AudioContext;
class AudioNodeInput;
class AudioNodeOutput;
class AudioParam;

typedef int ExceptionCode;

// An AudioNode is the basic building block for handling audio within an AudioContext.
// It may be an audio source, an intermediate processing module, or an audio destination.
// Each AudioNode can have inputs and/or outputs. An AudioSourceNode has no inputs and a single output.
// An AudioDestinationNode has one input and no outputs and represents the final destination to the audio hardware.
// Most processing nodes such as filters will have one input and one output, although multiple inputs and outputs are possible.

class AudioNode : public EventTarget {
public:
    enum { ProcessingSizeInFrames = 128 };

    AudioNode(AudioContext*, float sampleRate);
    virtual ~AudioNode();

    AudioContext* context() { return m_context.get(); }
    const AudioContext* context() const { return m_context.get(); }

    enum NodeType {
        NodeTypeUnknown,
        NodeTypeDestination,
        NodeTypeOscillator,
        NodeTypeAudioBufferSource,
        NodeTypeMediaElementAudioSource,
        NodeTypeMediaStreamAudioDestination,
        NodeTypeMediaStreamAudioSource,
        NodeTypeJavaScript,
        NodeTypeBiquadFilter,
        NodeTypePanner,
        NodeTypeConvolver,
        NodeTypeDelay,
        NodeTypeGain,
        NodeTypeChannelSplitter,
        NodeTypeChannelMerger,
        NodeTypeAnalyser,
        NodeTypeDynamicsCompressor,
        NodeTypeWaveShaper,
        NodeTypeEnd
    };

    enum ChannelCountMode {
        Max,
        ClampedMax,
        Explicit
    };

    NodeType nodeType() const { return m_nodeType; }
    void setNodeType(NodeType);

    // We handle our own ref-counting because of the threading issues and subtle nature of
    // how AudioNodes can continue processing (playing one-shot sound) after there are no more
    // JavaScript references to the object.
    enum RefType { RefTypeNormal, RefTypeConnection };

    // Can be called from main thread or context's audio thread.
    void ref(RefType refType = RefTypeNormal);
    void deref(RefType refType = RefTypeNormal);

    // Can be called from main thread or context's audio thread.  It must be called while the context's graph lock is held.
    void finishDeref(RefType refType);

    // The AudioNodeInput(s) (if any) will already have their input data available when process() is called.
    // Subclasses will take this input data and put the results in the AudioBus(s) of its AudioNodeOutput(s) (if any).
    // Called from context's audio thread.
    virtual void process(size_t framesToProcess) = 0;

    // Resets DSP processing state (clears delay lines, filter memory, etc.)
    // Called from context's audio thread.
    virtual void reset() = 0;

    // No significant resources should be allocated until initialize() is called.
    // Processing may not occur until a node is initialized.
    virtual void initialize();
    virtual void uninitialize();

    bool isInitialized() const { return m_isInitialized; }
    void lazyInitialize();

    unsigned numberOfInputs() const { return m_inputs.size(); }
    unsigned numberOfOutputs() const { return m_outputs.size(); }

    AudioNodeInput* input(unsigned);
    AudioNodeOutput* output(unsigned);

    // Called from main thread by corresponding JavaScript methods.
    virtual void connect(AudioNode*, unsigned outputIndex, unsigned inputIndex, ExceptionCode&);
    void connect(AudioParam*, unsigned outputIndex, ExceptionCode&);
    virtual void disconnect(unsigned outputIndex, ExceptionCode&);

    virtual float sampleRate() const { return m_sampleRate; }

    // processIfNecessary() is called by our output(s) when the rendering graph needs this AudioNode to process.
    // This method ensures that the AudioNode will only process once per rendering time quantum even if it's called repeatedly.
    // This handles the case of "fanout" where an output is connected to multiple AudioNode inputs.
    // Called from context's audio thread.
    void processIfNecessary(size_t framesToProcess);

    // Called when a new connection has been made to one of our inputs or the connection number of channels has changed.
    // This potentially gives us enough information to perform a lazy initialization or, if necessary, a re-initialization.
    // Called from main thread.
    virtual void checkNumberOfChannelsForInput(AudioNodeInput*);

#if DEBUG_AUDIONODE_REFERENCES
    static void printNodeCounts();
#endif

    bool isMarkedForDeletion() const { return m_isMarkedForDeletion; }

    // tailTime() is the length of time (not counting latency time) where non-zero output may occur after continuous silent input.
    virtual double tailTime() const = 0;
    // latencyTime() is the length of time it takes for non-zero output to appear after non-zero input is provided. This only applies to
    // processing delay which is an artifact of the processing algorithm chosen and is *not* part of the intrinsic desired effect. For 
    // example, a "delay" effect is expected to delay the signal, and thus would not be considered latency.
    virtual double latencyTime() const = 0;

    // propagatesSilence() should return true if the node will generate silent output when given silent input. By default, AudioNode
    // will take tailTime() and latencyTime() into account when determining whether the node will propagate silence.
    virtual bool propagatesSilence() const;
    bool inputsAreSilent();
    void silenceOutputs();
    void unsilenceOutputs();

    void enableOutputsIfNecessary();
    void disableOutputsIfNecessary();

    unsigned long channelCount();
    virtual void setChannelCount(unsigned long, ExceptionCode&);

    String channelCountMode();
    void setChannelCountMode(const String&, ExceptionCode&);

    String channelInterpretation();
    void setChannelInterpretation(const String&, ExceptionCode&);

    ChannelCountMode internalChannelCountMode() const { return m_channelCountMode; }
    AudioBus::ChannelInterpretation internalChannelInterpretation() const { return m_channelInterpretation; }

    // EventTarget
    virtual const AtomicString& interfaceName() const OVERRIDE;
    virtual ScriptExecutionContext* scriptExecutionContext() const OVERRIDE;
    virtual EventTargetData* eventTargetData() OVERRIDE { return &m_eventTargetData; }
    virtual EventTargetData* ensureEventTargetData() OVERRIDE { return &m_eventTargetData; }

protected:
    // Inputs and outputs must be created before the AudioNode is initialized.
    void addInput(PassOwnPtr<AudioNodeInput>);
    void addOutput(PassOwnPtr<AudioNodeOutput>);
    
    // Called by processIfNecessary() to cause all parts of the rendering graph connected to us to process.
    // Each rendering quantum, the audio data for each of the AudioNode's inputs will be available after this method is called.
    // Called from context's audio thread.
    virtual void pullInputs(size_t framesToProcess);

    // Force all inputs to take any channel interpretation changes into account.
    void updateChannelsForInputs();

private:
    volatile bool m_isInitialized;
    NodeType m_nodeType;
    RefPtr<AudioContext> m_context;
    float m_sampleRate;
    Vector<OwnPtr<AudioNodeInput> > m_inputs;
    Vector<OwnPtr<AudioNodeOutput> > m_outputs;

    EventTargetData m_eventTargetData;

    double m_lastProcessingTime;
    double m_lastNonSilentTime;

    // Ref-counting
    volatile int m_normalRefCount;
    volatile int m_connectionRefCount;
    
    bool m_isMarkedForDeletion;
    bool m_isDisabled;

#if DEBUG_AUDIONODE_REFERENCES
    static bool s_isNodeCountInitialized;
    static int s_nodeCount[NodeTypeEnd];
#endif

    virtual void refEventTarget() OVERRIDE { ref(); }
    virtual void derefEventTarget() OVERRIDE { deref(); }

protected:
    unsigned m_channelCount;
    ChannelCountMode m_channelCountMode;
    AudioBus::ChannelInterpretation m_channelInterpretation;
};

} // namespace WebCore

#endif // AudioNode_h
