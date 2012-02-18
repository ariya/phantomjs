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

#include "JavaScriptAudioNode.h"

#include "AudioBuffer.h"
#include "AudioBus.h"
#include "AudioContext.h"
#include "AudioNodeInput.h"
#include "AudioNodeOutput.h"
#include "AudioProcessingEvent.h"
#include "Document.h"
#include "Float32Array.h"
#include <wtf/MainThread.h>

namespace WebCore {

const size_t DefaultBufferSize = 4096;

PassRefPtr<JavaScriptAudioNode> JavaScriptAudioNode::create(AudioContext* context, double sampleRate, size_t bufferSize, unsigned numberOfInputs, unsigned numberOfOutputs)
{
    return adoptRef(new JavaScriptAudioNode(context, sampleRate, bufferSize, numberOfInputs, numberOfOutputs));
}

JavaScriptAudioNode::JavaScriptAudioNode(AudioContext* context, double sampleRate, size_t bufferSize, unsigned numberOfInputs, unsigned numberOfOutputs)
    : AudioNode(context, sampleRate)
    , m_doubleBufferIndex(0)
    , m_doubleBufferIndexForEvent(0)
    , m_bufferSize(bufferSize)
    , m_bufferReadWriteIndex(0)
    , m_isRequestOutstanding(false)
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
        m_bufferSize = bufferSize;
        break;
    default:
        m_bufferSize = DefaultBufferSize;
    }
        
    // Regardless of the allowed buffer sizes above, we still need to process at the granularity of the AudioNode.
    if (m_bufferSize < AudioNode::ProcessingSizeInFrames)
        m_bufferSize = AudioNode::ProcessingSizeInFrames;

    // FIXME: Right now we're hardcoded to single input and single output.
    // Although the specification says this is OK for a simple implementation, multiple inputs and outputs would be good.
    ASSERT_UNUSED(numberOfInputs, numberOfInputs == 1);
    ASSERT_UNUSED(numberOfOutputs, numberOfOutputs == 1);
    addInput(adoptPtr(new AudioNodeInput(this)));
    addOutput(adoptPtr(new AudioNodeOutput(this, 2)));

    setType(NodeTypeJavaScript);

    initialize();
}

JavaScriptAudioNode::~JavaScriptAudioNode()
{
    uninitialize();
}

void JavaScriptAudioNode::initialize()
{
    if (isInitialized())
        return;

    double sampleRate = context()->sampleRate();

    // Create double buffers on both the input and output sides.
    // These AudioBuffers will be directly accessed in the main thread by JavaScript.
    for (unsigned i = 0; i < 2; ++i) {
        m_inputBuffers.append(AudioBuffer::create(2, bufferSize(), sampleRate));
        m_outputBuffers.append(AudioBuffer::create(2, bufferSize(), sampleRate));
    }

    AudioNode::initialize();
}

void JavaScriptAudioNode::uninitialize()
{
    if (!isInitialized())
        return;

    m_inputBuffers.clear();
    m_outputBuffers.clear();

    AudioNode::uninitialize();
}

JavaScriptAudioNode* JavaScriptAudioNode::toJavaScriptAudioNode()
{
    return this;
}

void JavaScriptAudioNode::process(size_t framesToProcess)
{
    // Discussion about inputs and outputs:
    // As in other AudioNodes, JavaScriptAudioNode uses an AudioBus for its input and output (see inputBus and outputBus below).
    // Additionally, there is a double-buffering for input and output which is exposed directly to JavaScript (see inputBuffer and outputBuffer below).
    // This node is the producer for inputBuffer and the consumer for outputBuffer.
    // The JavaScript code is the consumer of inputBuffer and the producer for outputBuffer.
    
    // Get input and output busses.
    AudioBus* inputBus = this->input(0)->bus();
    AudioBus* outputBus = this->output(0)->bus();

    // Get input and output buffers.  We double-buffer both the input and output sides.
    unsigned doubleBufferIndex = this->doubleBufferIndex();
    bool isDoubleBufferIndexGood = doubleBufferIndex < 2 && doubleBufferIndex < m_inputBuffers.size() && doubleBufferIndex < m_outputBuffers.size();
    ASSERT(isDoubleBufferIndexGood);
    if (!isDoubleBufferIndexGood)
        return;
    
    AudioBuffer* inputBuffer = m_inputBuffers[doubleBufferIndex].get();
    AudioBuffer* outputBuffer = m_outputBuffers[doubleBufferIndex].get();

    // Check the consistency of input and output buffers.
    bool buffersAreGood = inputBuffer && outputBuffer && bufferSize() == inputBuffer->length() && bufferSize() == outputBuffer->length()
        && m_bufferReadWriteIndex + framesToProcess <= bufferSize();
    ASSERT(buffersAreGood);
    if (!buffersAreGood)
        return;

    // We assume that bufferSize() is evenly divisible by framesToProcess - should always be true, but we should still check.
    bool isFramesToProcessGood = framesToProcess && bufferSize() >= framesToProcess && !(bufferSize() % framesToProcess);
    ASSERT(isFramesToProcessGood);
    if (!isFramesToProcessGood)
        return;
        
    unsigned numberOfInputChannels = inputBus->numberOfChannels();
    
    bool channelsAreGood = (numberOfInputChannels == 1 || numberOfInputChannels == 2) && outputBus->numberOfChannels() == 2;
    ASSERT(channelsAreGood);
    if (!channelsAreGood)
        return;

    float* sourceL = inputBus->channel(0)->data();
    float* sourceR = numberOfInputChannels > 1 ? inputBus->channel(1)->data() : 0;
    float* destinationL = outputBus->channel(0)->data();
    float* destinationR = outputBus->channel(1)->data();

    // Copy from the input to the input buffer.  See "buffersAreGood" check above for safety.
    size_t bytesToCopy = sizeof(float) * framesToProcess;
    memcpy(inputBuffer->getChannelData(0)->data() + m_bufferReadWriteIndex, sourceL, bytesToCopy);
    
    if (numberOfInputChannels == 2)
        memcpy(inputBuffer->getChannelData(1)->data() + m_bufferReadWriteIndex, sourceR, bytesToCopy);
    else if (numberOfInputChannels == 1) {
        // If the input is mono, then also copy the mono input to the right channel of the AudioBuffer which the AudioProcessingEvent uses.
        // FIXME: it is likely the audio API will evolve to present an AudioBuffer with the same number of channels as our input.
        memcpy(inputBuffer->getChannelData(1)->data() + m_bufferReadWriteIndex, sourceL, bytesToCopy);
    }
    
    // Copy from the output buffer to the output.  See "buffersAreGood" check above for safety.
    memcpy(destinationL, outputBuffer->getChannelData(0)->data() + m_bufferReadWriteIndex, bytesToCopy);
    memcpy(destinationR, outputBuffer->getChannelData(1)->data() + m_bufferReadWriteIndex, bytesToCopy);

    // Update the buffering index.
    m_bufferReadWriteIndex = (m_bufferReadWriteIndex + framesToProcess) % bufferSize();

    // m_bufferReadWriteIndex will wrap back around to 0 when the current input and output buffers are full.
    // When this happens, fire an event and swap buffers.
    if (!m_bufferReadWriteIndex) {
        // Avoid building up requests on the main thread to fire process events when they're not being handled.
        // This could be a problem if the main thread is very busy doing other things and is being held up handling previous requests.
        if (m_isRequestOutstanding) {
            // We're late in handling the previous request.  The main thread must be very busy.
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

void JavaScriptAudioNode::fireProcessEventDispatch(void* userData)
{
    JavaScriptAudioNode* jsAudioNode = static_cast<JavaScriptAudioNode*>(userData);
    ASSERT(jsAudioNode);
    if (!jsAudioNode)
        return;

    jsAudioNode->fireProcessEvent();

    // De-reference to match the ref() call in process().
    jsAudioNode->deref();
}

void JavaScriptAudioNode::fireProcessEvent()
{
    ASSERT(isMainThread() && m_isRequestOutstanding);
    
    bool isIndexGood = m_doubleBufferIndexForEvent < 2;
    ASSERT(isIndexGood);
    if (!isIndexGood)
        return;
        
    AudioBuffer* inputBuffer = m_inputBuffers[m_doubleBufferIndexForEvent].get();
    AudioBuffer* outputBuffer = m_outputBuffers[m_doubleBufferIndexForEvent].get();
    ASSERT(inputBuffer && outputBuffer);
    if (!inputBuffer || !outputBuffer)
        return;

    // Avoid firing the event if the document has already gone away.
    if (context()->hasDocument()) {
        // Let the audio thread know we've gotten to the point where it's OK for it to make another request.
        m_isRequestOutstanding = false;
        
        // Call the JavaScript event handler which will do the audio processing.
        dispatchEvent(AudioProcessingEvent::create(inputBuffer, outputBuffer));
    }
}

void JavaScriptAudioNode::reset()
{
    m_bufferReadWriteIndex = 0;
    m_doubleBufferIndex = 0;

    for (unsigned i = 0; i < 2; ++i) {
        m_inputBuffers[i]->zero();
        m_outputBuffers[i]->zero();
    }
}

ScriptExecutionContext* JavaScriptAudioNode::scriptExecutionContext() const
{
    return const_cast<JavaScriptAudioNode*>(this)->context()->document();
}

} // namespace WebCore

#endif // ENABLE(WEB_AUDIO)
