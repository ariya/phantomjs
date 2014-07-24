/*
 * Copyright (C) 2006 Don Gibson <dgibson77@gmail.com>
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "FrameLoader.h"

#include "DocumentLoader.h"
#include "FrameLoadRequest.h"
#include "FrameWin.h"
#include "ResourceRequest.h"

namespace WebCore {

void FrameLoader::urlSelected(const FrameLoadRequest& request, Event* /*triggering Event*/)
{
    FrameWin* frameWin = static_cast<FrameWin*>(m_frame);
    if (frameWin->client())
        frameWin->client()->openURL(request.resourceRequest().url().string(), request.lockHistory());
}

void FrameLoader::submitForm(const FrameLoadRequest& request, Event*)
{
    const ResourceRequest& resourceRequest = request.resourceRequest();

#ifdef MULTIPLE_FORM_SUBMISSION_PROTECTION
    // FIXME: this is a hack inherited from FrameMac, and should be pushed into Frame
    if (m_submittedFormURL == resourceRequest.url())
        return;
    m_submittedFormURL = resourceRequest.url();
#endif

    FrameWin* frameWin = static_cast<FrameWin*>(m_frame);
    if (frameWin->client())
        frameWin->client()->submitForm(resourceRequest.httpMethod(), resourceRequest.url(), resourceRequest.httpBody());

    clearRecordedFormValues();
}

}
