/*
 * Copyright (C) 2012, Google Inc. All rights reserved.
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

#include "AudioSummingJunction.h"

#include "AudioContext.h"
#include "AudioNodeOutput.h"
#include <algorithm>

using namespace std;

namespace WebCore {

AudioSummingJunction::AudioSummingJunction(AudioContext* context)
    : m_context(context)
    , m_renderingStateNeedUpdating(false)
{
}

AudioSummingJunction::~AudioSummingJunction()
{
    if (m_renderingStateNeedUpdating && m_context.get())
        m_context->removeMarkedSummingJunction(this);
}

void AudioSummingJunction::changedOutputs()
{
    ASSERT(context()->isGraphOwner());
    if (!m_renderingStateNeedUpdating && canUpdateState()) {
        context()->markSummingJunctionDirty(this);
        m_renderingStateNeedUpdating = true;
    }
}

void AudioSummingJunction::updateRenderingState()
{
    ASSERT(context()->isAudioThread() && context()->isGraphOwner());

    if (m_renderingStateNeedUpdating && canUpdateState()) {
        // Copy from m_outputs to m_renderingOutputs.
        m_renderingOutputs.resize(m_outputs.size());
        unsigned j = 0;
        for (HashSet<AudioNodeOutput*>::iterator i = m_outputs.begin(); i != m_outputs.end(); ++i, ++j) {
            AudioNodeOutput* output = *i;
            m_renderingOutputs[j] = output;
            output->updateRenderingState();
        }

        didUpdate();

        m_renderingStateNeedUpdating = false;
    }
}

} // namespace WebCore

#endif // ENABLE(WEB_AUDIO)
