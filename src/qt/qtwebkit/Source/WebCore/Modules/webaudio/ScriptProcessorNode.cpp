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

#include "ScriptProcessorNode.h"

#include "AudioBuffer.h"
#include "AudioBus.h"
#include "AudioContext.h"
#include "AudioNodeInput.h"
#include "AudioNodeOutput.h"
#include "AudioProcessingEvent.h"
#include "Document.h"
#include <wtf/Float32Array.h>
#include <wtf/MainThread.h>

namespace WebCore {

const size_t DefaultBufferSize = 4096;

PassRefPtr<ScriptProcessorNode> ScriptProcessorNode::create(AudioContext* context, float sampleRate, size_t bufferSize, unsigned numberOfInputChannels, unsigned numberOfOutputChannels)
{
    // Check for valid buffer size.
    switch (bufferSize) {
    case 256:
    case 512:
    case 1024:
    case 2048:
    case 4096:
    case 8192:
    case 16384:
        break;
    default:
        return 0;
    }

    if (!numberOfInputChannels && !numberOfOutputChannels)
        return 0;

    if (numberOfInputChannels > AudioContext::maxNumberOfChannels())
        return 0;

    if (numberOfOutputChannels > AudioContext::maxNumberOfChannels())
        return 0;

    return adoptRef(new ScriptProcessorNode(context, sampleRate, bufferSize, numberOfInputChannels, numberOfOutputChannels));
}

ScriptProcessorNode::ScriptProcessorNode(AudioContext* context, float sampleRate, size_t bufferSize, unsigned numberOfInputChannels, unsigned numberOfOutputChannels)
    : AudioNode(context, sampleRate)
    , m_doubleBufferIndex(0)
    , m_doubleBufferIndexForEvent(0)
    , m_bufferSize(bufferSize)
    , m_bufferReadWriteIndex(0)
    , m_isRequestOutstanding(false)
    , m_numberOfInputChannels(numberOfInputChannels)
    , m_numberOfOutputChannels(numberOfOutputChannels)
    , m_internalInputBus(AudioBus::create(numberOfInputChannels, AudioNode::ProcessingSizeInFrames, false))
    , m_hasAudioProcessListener(false)
{
    // Regardless of the allowed buffer sizes, we still need to process at the granularity of the AudioNode.
    if (m_bufferSize < AudioNode::ProcessingSizeInFrames)
        m_bufferSize = AudioNode::ProcessingSizeInFrames;

    ASSERT(numberOfInputChannels <= AudioContext::maxNumberOfChannels());

    addInput(adoptPtr(new AudioNodeInput(this)));
    addOutput(adoptPtr(new AudioNodeOutput(this, numberOfOutputChannels)));

    setNodeType(NodeTypeJavaScript);

    initialize();
}

ScriptProcessorNode::~ScriptProcessorNode()
{
    uninitialize();
}

void ScriptProcessorNode::initialize()
{
    if (isInitialized())
        return;

    float sampleRate = context()->sampleRate();

    // Create double buffers on both the input and output sides.
    // These AudioBuffers will be directly accessed in the main thread by JavaScript.
    for (unsigned i = 0; i < 2; ++i) {
        RefPtr<AudioBuffer> inputBuffer = m_numberOfInputChannels ? AudioBuffer::create(m_numberOfInputChannels, bufferSize(), sampleRate) : 0;
        RefPtr<AudioBuffer> outputBuffer = m_numberOfOutputChannels ? AudioBuffer::create(m_numberOfOutputChannels, bufferSize(), sampleRate) : 0;

        m_inputBuffers.append(inputBuffer);
        m_outputBuffers.append(outputBuffer);
    }

    AudioNode::initialize();
}

void ScriptProcessorNode::uninitialize()
{
    if (!isInitialized())
        return;

    m_inputBuffers.clear();
    m_outputBuffers.clear();

    AudioNode::uninitialize();
}

void ScriptProcessorNode::process(size_t framesToProcess)
{
    // Discussion about inputs and outputs:
    // As in other AudioNodes, ScriptProcessorNode uses an AudioBus for its input and output (see inputBus and outputBus below).
    // Additionally, there is a double-buffering for input and output which is exposed directly to JavaScript (see inputBuffer and outputBuffer below).
    // This node is the producer for inputBuffer and the consumer for outputBuffer.
    // The JavaScript code is the consumer of inputBuffer and the producer for outputBuffer.

    // Check if audioprocess listener is set.
    if (!m_hasAudioProcessListener)
        return;

    // Get input and output busses.
    AudioBus* inputBus = this->input(0)->bus();
    AudioBus* outputBus = this->output(0)->bus();

    // Get input and output buffers. We double-buffer both the input and output sides.
    unsigned doubleBufferIndex = this->doubleBufferIndex();
    bool isDoubleBufferIndexGood = doubleBufferIndex < 2 && doubleBufferIndex < m_inputBuffers.size() && doubleBufferIndex < m_outputBuffers.size();
    ASSERT(isDoubleBufferIndexGood);
    if (!isDoubleBufferIndexGood)
        return;
    
    AudioBuffer* inputBuffer = m_inputBuffers[doubleBufferIndex].get();
    AudioBuffer* outputBuffer = m_outputBuffers[doubleBufferIndex].get();

    // Check the consistency of input and output buffers.
    unsigned numberOfInputChannels = m_internalInputBus->numberOfChannels();
    bool buffersAreGood = outputBuffer && bufferSize() == outputBuffer->length() && m_bufferReadWriteIndex + framesToProcess <= bufferSize();

    // If the number of input channels is zero, it's ok to have inputBuffer = 0.
    if (m_internalInputBus->numberOfChannels())
        buffersAreGood = buffersAreGood && inputBuffer && bufferSize() == inputBuffer->length();

    ASSERT(buffersAreGood);
    if (!buffersAreGood)
        return;

    // We assume that bufferSize() is evenly divisible by framesToProcess - should always be true, but we should still check.
    bool isFramesToProcessGood = framesToProcess && bufferSize() >= framesToProcess && !(bufferSize() % framesToProcess);
    ASSERT(isFramesToProcessGood);
    if (!isFramesToProcessGood)
        return;

    unsigned numberOfOutputChannels = outputBus->numberOfChannels();

    bool channelsAreGood = (numberOfInputChannels == m_numberOfInputChannels) && (numberOfOutputChannels == m_numberOfOutputChannels);
    ASSERT(channelsAreGood);
    if (!channelsAreGood)
        return;

    for (unsigned i = 0; i < numberOfInputChannels; i++)
        m_internalInputBus->setChannelMemory(i, inputBuffer->getChannelData(i)->data() + m_bufferReadWriteIndex, framesToProcess);

    if (numberOfInputChannels)
        m_internalInputBus->copyFrom(*inputBus);

    // Copy from the output buffer to the output. 
    for (unsigned i = 0; i < numberOfOutputChannels; ++i)
        memcpy(outputBus->channel(i)->mutableData(), outputBuffer->getChannelData(i)->data() + m_bufferReadWriteIndex, sizeof(float) * framesToProcess);

    // Update the buffering index.
    m_bufferReadWriteIndex = (m_bufferReadWriteIndex + framesToProcess) % bufferSize();

    // m_bufferReadWriteIndex will wrap back around to 0 when the current input and output buffers are full.
    // When this happens, fire an event and swap buffers.
    if (!m_bufferReadWriteIndex) {
        // Avoid building up requests on the main thread to fire process events when they're not being handled.
        // This could be a problem if the main thread is very busy doing other things and is being held up handling previous requests.
        if (m_isRequestOutstanding) {
            // We're late in handling the previous request. The main thread must be very busy.
            // The best we can do is clear out the buffer ourself here.
            outputBuffer->zero();            
        } else {
            // Reference ourself so we don't accidentally get deleted before fireProcessEvent() gets called.
            ref();
            
            // Fire the event on the main thread, not this one (which is the realtime audio thread).
            m_doubleBufferIndexForEvent = m_doubleBufferIndex;
            m_isRequestOutstanding = true;
            callOnMainThread(fireProcessEventDispatch, this);
        }

        swapBuffers();
    }
}

void ScriptProcessorNode::setOnaudioprocess(PassRefPtr<EventListener> listener)
{
    m_hasAudioProcessListener = listener;
    setAttributeEventListener(eventNames().audioprocessEvent, listener);
}

void ScriptProcessorNode::fireProcessEventDispatch(void* userData)
{
    ScriptProcessorNode* jsAudioNode = static_cast<ScriptProcessorNode*>(userData);
    ASSERT(jsAudioNode);
    if (!jsAudioNode)
        return;

    jsAudioNode->fireProcessEvent();

    // De-reference to match the ref() call in process().
    jsAudioNode->deref();
}

void ScriptProcessorNode::fireProcessEvent()
{
    ASSERT(isMainThread() && m_isRequestOutstanding);
    
    bool isIndexGood = m_doubleBufferIndexForEvent < 2;
    ASSERT(isIndexGood);
    if (!isIndexGood)
        return;
        
    AudioBuffer* inputBuffer = m_inputBuffers[m_doubleBufferIndexForEvent].get();
    AudioBuffer* outputBuffer = m_outputBuffers[m_doubleBufferIndexForEvent].get();
    ASSERT(outputBuffer);
    if (!outputBuffer)
        return;

    // Avoid firing the event if the document has already gone away.
    if (context()->scriptExecutionContext()) {
        // Let the audio thread know we've gotten to the point where it's OK for it to make another request.
        m_isRequestOutstanding = false;
        
        // Call the JavaScript event handler which will do the audio processing.
        dispatchEvent(AudioProcessingEvent::create(inputBuffer, outputBuffer));
    }
}

void ScriptProcessorNode::reset()
{
    m_bufferReadWriteIndex = 0;
    m_doubleBufferIndex = 0;

    for (unsigned i = 0; i < 2; ++i) {
        m_inputBuffers[i]->zero();
        m_outputBuffers[i]->zero();
    }
}

double ScriptProcessorNode::tailTime() const
{
    return std::numeric_limits<double>::infinity();
}

double ScriptProcessorNode::latencyTime() const
{
    return std::numeric_limits<double>::infinity();
}

} // namespace WebCore

#endif // ENABLE(WEB_AUDIO)
