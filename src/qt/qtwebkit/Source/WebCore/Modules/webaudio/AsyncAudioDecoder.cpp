/*
 * Copyright (C) 2011, Google Inc. All rights reserved.
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

#include "AsyncAudioDecoder.h"

#include "AudioBuffer.h"
#include "AudioBufferCallback.h"
#include <wtf/ArrayBuffer.h>
#include <wtf/MainThread.h>
#include <wtf/OwnPtr.h>
#include <wtf/PassOwnPtr.h>

namespace WebCore {

AsyncAudioDecoder::AsyncAudioDecoder()
{
    // Start worker thread.
    MutexLocker lock(m_threadCreationMutex);
    m_threadID = createThread(AsyncAudioDecoder::threadEntry, this, "Audio Decoder");
}

AsyncAudioDecoder::~AsyncAudioDecoder()
{
    m_queue.kill();
    
    // Stop thread.
    waitForThreadCompletion(m_threadID);
    m_threadID = 0;
}

void AsyncAudioDecoder::decodeAsync(ArrayBuffer* audioData, float sampleRate, PassRefPtr<AudioBufferCallback> successCallback, PassRefPtr<AudioBufferCallback> errorCallback)
{
    ASSERT(isMainThread());
    ASSERT(audioData);
    if (!audioData)
        return;

    OwnPtr<DecodingTask> decodingTask = DecodingTask::create(audioData, sampleRate, successCallback, errorCallback);
    m_queue.append(decodingTask.release()); // note that ownership of the task is effectively taken by the queue.
}

// Asynchronously decode in this thread.
void AsyncAudioDecoder::threadEntry(void* threadData)
{
    ASSERT(threadData);
    AsyncAudioDecoder* decoder = reinterpret_cast<AsyncAudioDecoder*>(threadData);
    decoder->runLoop();
}

void AsyncAudioDecoder::runLoop()
{
    ASSERT(!isMainThread());

    {
        // Wait for until we have m_threadID established before starting the run loop.
        MutexLocker lock(m_threadCreationMutex);
    }

    // Keep running decoding tasks until we're killed.
    while (OwnPtr<DecodingTask> decodingTask = m_queue.waitForMessage()) {
        // Let the task take care of its own ownership.
        // See DecodingTask::notifyComplete() for cleanup.
        decodingTask.leakPtr()->decode();
    }
}

PassOwnPtr<AsyncAudioDecoder::DecodingTask> AsyncAudioDecoder::DecodingTask::create(ArrayBuffer* audioData, float sampleRate, PassRefPtr<AudioBufferCallback> successCallback, PassRefPtr<AudioBufferCallback> errorCallback)
{
    return adoptPtr(new DecodingTask(audioData, sampleRate, successCallback, errorCallback));
}

AsyncAudioDecoder::DecodingTask::DecodingTask(ArrayBuffer* audioData, float sampleRate, PassRefPtr<AudioBufferCallback> successCallback, PassRefPtr<AudioBufferCallback> errorCallback)
    : m_audioData(audioData)
    , m_sampleRate(sampleRate)
    , m_successCallback(successCallback)
    , m_errorCallback(errorCallback)
{
}

void AsyncAudioDecoder::DecodingTask::decode()
{
    ASSERT(m_audioData.get());
    if (!m_audioData.get())
        return;

    // Do the actual decoding and invoke the callback.
    m_audioBuffer = AudioBuffer::createFromAudioFileData(m_audioData->data(), m_audioData->byteLength(), false, sampleRate());
    
    // Decoding is finished, but we need to do the callbacks on the main thread.
    callOnMainThread(notifyCompleteDispatch, this);
}

void AsyncAudioDecoder::DecodingTask::notifyCompleteDispatch(void* userData)
{
    AsyncAudioDecoder::DecodingTask* task = reinterpret_cast<AsyncAudioDecoder::DecodingTask*>(userData);
    ASSERT(task);
    if (!task)
        return;

    task->notifyComplete();
}

void AsyncAudioDecoder::DecodingTask::notifyComplete()
{
    if (audioBuffer() && successCallback())
        successCallback()->handleEvent(audioBuffer());
    else if (errorCallback())
        errorCallback()->handleEvent(audioBuffer());

    // Our ownership was given up in AsyncAudioDecoder::runLoop()
    // Make sure to clean up here.
    delete this;
}

} // namespace WebCore

#endif // ENABLE(WEB_AUDIO)
