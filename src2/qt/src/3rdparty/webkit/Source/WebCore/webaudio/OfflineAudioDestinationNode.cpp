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

#include "OfflineAudioDestinationNode.h"

#include "AudioBus.h"
#include "AudioContext.h"
#include "HRTFDatabaseLoader.h"
#include <algorithm>
#include <wtf/Threading.h>

using namespace std;
 
namespace WebCore {
    
const size_t renderQuantumSize = 128;    

OfflineAudioDestinationNode::OfflineAudioDestinationNode(AudioContext* context, AudioBuffer* renderTarget)
    : AudioDestinationNode(context, renderTarget->sampleRate())
    , m_renderTarget(renderTarget)
    , m_startedRendering(false)
{
    m_renderBus = adoptPtr(new AudioBus(renderTarget->numberOfChannels(), renderQuantumSize));
    
    initialize();
}

OfflineAudioDestinationNode::~OfflineAudioDestinationNode()
{
    uninitialize();
}

void OfflineAudioDestinationNode::initialize()
{
    if (isInitialized())
        return;

    AudioNode::initialize();
}

void OfflineAudioDestinationNode::uninitialize()
{
    if (!isInitialized())
        return;

    AudioNode::uninitialize();
}

void OfflineAudioDestinationNode::startRendering()
{
    ASSERT(isMainThread());
    ASSERT(m_renderTarget.get());
    if (!m_renderTarget.get())
        return;
    
    if (!m_startedRendering) {
        m_startedRendering = true;
        m_renderThread = createThread(OfflineAudioDestinationNode::renderEntry, this, "offline renderer");
    }
}

// Do offline rendering in this thread.
void* OfflineAudioDestinationNode::renderEntry(void* threadData)
{
    OfflineAudioDestinationNode* destinationNode = reinterpret_cast<OfflineAudioDestinationNode*>(threadData);
    ASSERT(destinationNode);
    destinationNode->render();
    
    return 0;
}

void OfflineAudioDestinationNode::render()
{
    ASSERT(!isMainThread());
    ASSERT(m_renderBus.get());
    if (!m_renderBus.get())
        return;
    
    bool channelsMatch = m_renderBus->numberOfChannels() == m_renderTarget->numberOfChannels();
    ASSERT(channelsMatch);
    if (!channelsMatch)
        return;
        
    bool isRenderBusAllocated = m_renderBus->length() >= renderQuantumSize;
    ASSERT(isRenderBusAllocated);
    if (!isRenderBusAllocated)
        return;
        
    // Synchronize with HRTFDatabaseLoader.
    // The database must be loaded before we can proceed.
    HRTFDatabaseLoader* loader = HRTFDatabaseLoader::loader();
    ASSERT(loader);
    if (!loader)
        return;
    
    loader->waitForLoaderThreadCompletion();
        
    // Break up the render target into smaller "render quantize" sized pieces.
    // Render until we're finished.
    size_t framesToProcess = m_renderTarget->length();
    unsigned numberOfChannels = m_renderTarget->numberOfChannels();

    unsigned n = 0;
    while (framesToProcess > 0) {
        // Render one render quantum.
        provideInput(m_renderBus.get(), renderQuantumSize);
        
        size_t framesAvailableToCopy = min(framesToProcess, renderQuantumSize);
        
        for (unsigned channelIndex = 0; channelIndex < numberOfChannels; ++channelIndex) {
            float* source = m_renderBus->channel(channelIndex)->data();
            float* destination = m_renderTarget->getChannelData(channelIndex)->data();
            memcpy(destination + n, source, sizeof(float) * framesAvailableToCopy);
        }
        
        n += framesAvailableToCopy;
        framesToProcess -= framesAvailableToCopy;
    }
    
    // Our work is done. Let the AudioContext know.
    callOnMainThread(notifyCompleteDispatch, this);
}

void OfflineAudioDestinationNode::notifyCompleteDispatch(void* userData)
{
    OfflineAudioDestinationNode* destinationNode = static_cast<OfflineAudioDestinationNode*>(userData);
    ASSERT(destinationNode);
    if (!destinationNode)
        return;

    destinationNode->notifyComplete();
}

void OfflineAudioDestinationNode::notifyComplete()
{
    context()->fireCompletionEvent();
}

} // namespace WebCore

#endif // ENABLE(WEB_AUDIO)
