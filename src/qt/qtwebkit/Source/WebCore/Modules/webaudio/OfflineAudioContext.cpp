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

#include "OfflineAudioContext.h"

#include "Document.h"
#include "ExceptionCode.h"
#include "ScriptExecutionContext.h"

namespace WebCore {

PassRefPtr<OfflineAudioContext> OfflineAudioContext::create(ScriptExecutionContext* context, unsigned numberOfChannels, size_t numberOfFrames, float sampleRate, ExceptionCode& ec)
{
    // FIXME: add support for workers.
    if (!context || !context->isDocument()) {
        ec = NOT_SUPPORTED_ERR;
        return 0;
    }

    Document* document = toDocument(context);

    if (numberOfChannels > 10 || !isSampleRateRangeGood(sampleRate)) {
        ec = SYNTAX_ERR;
        return 0;
    }

    RefPtr<OfflineAudioContext> audioContext(adoptRef(new OfflineAudioContext(document, numberOfChannels, numberOfFrames, sampleRate)));
    audioContext->suspendIfNeeded();
    return audioContext.release();
}

OfflineAudioContext::OfflineAudioContext(Document* document, unsigned numberOfChannels, size_t numberOfFrames, float sampleRate)
    : AudioContext(document, numberOfChannels, numberOfFrames, sampleRate)
{
}

OfflineAudioContext::~OfflineAudioContext()
{
}

} // namespace WebCore

#endif // ENABLE(WEB_AUDIO)
