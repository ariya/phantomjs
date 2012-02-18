/*
 * Copyright (C) 2011 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
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
#include "MediaStreamFrameController.h"

#if ENABLE(MEDIA_STREAM)

#include "DOMWindow.h"
#include "Document.h"
#include "Frame.h"
#include "MediaStreamController.h"
#include "NavigatorUserMediaErrorCallback.h"
#include "NavigatorUserMediaSuccessCallback.h"
#include "Page.h"
#include "SecurityOrigin.h"
#include <wtf/RefCounted.h>

namespace WebCore {

class MediaStreamFrameController::Request : public RefCounted<Request> {
public:
    Request(ScriptExecutionContext* scriptExecutionContext)
        : m_scriptExecutionContext(scriptExecutionContext) { }

    virtual ~Request() { }

    ScriptExecutionContext* scriptExecutionContext() const { return m_scriptExecutionContext; }
    virtual bool isGenerateStreamRequest() const { return false; }
    virtual bool isRecordedDataRequest() const { return false; }

    virtual void abort() = 0;

private:
    // This is guaranteed to have the lifetime of the Frame, and it's only used to make
    // the callback asynchronous. The original callback context is used in the call.
    ScriptExecutionContext* m_scriptExecutionContext;
};

void MediaStreamFrameController::RequestMap::abort(int requestId)
{
    get(requestId)->abort();
    remove(requestId);
}

void MediaStreamFrameController::RequestMap::abortAll()
{
    while (!isEmpty()) {
        begin()->second->abort();
        remove(begin());
    }
}

MediaStreamFrameController::MediaStreamFrameController(Frame* frame)
    : m_frame(frame)
    , m_isInDetachedState(false)
{
}

MediaStreamFrameController::~MediaStreamFrameController()
{
}

SecurityOrigin* MediaStreamFrameController::securityOrigin() const
{
    return m_frame ? m_frame->existingDOMWindow()->securityOrigin() : 0;
}

ScriptExecutionContext* MediaStreamFrameController::scriptExecutionContext() const
{
    return m_frame ? m_frame->existingDOMWindow()->scriptExecutionContext() : 0;
}

MediaStreamController* MediaStreamFrameController::pageController() const
{
    return !m_isInDetachedState && m_frame && m_frame->page() ? m_frame->page()->mediaStreamController() : 0;
}

void MediaStreamFrameController::enterDetachedState()
{
    if (m_isInDetachedState) {
        ASSERT(m_requests.isEmpty());
        return;
    }

    m_requests.abortAll();
    m_isInDetachedState = true;
}

// Called also when the frame is detached from the page, in which case the page controller will remain alive.
void MediaStreamFrameController::disconnectPage()
{
    if (pageController())
        pageController()->unregisterFrameController(this);

    enterDetachedState();
}

// Called when the frame is being destroyed. Since the frame controller is owned by the frame it will die shortly after this.
void MediaStreamFrameController::disconnectFrame()
{
    disconnectPage();

    ASSERT(m_requests.isEmpty());

    m_frame = 0;
}

void MediaStreamFrameController::transferToNewPage(Page*)
{
    // FIXME: In the future we should keep running the media stream services while transfering frames between pages.
    // However, until a proper way to do this is decided, we're shutting down services.
    disconnectPage();
}

} // namespace WebCore

#endif // ENABLE(MEDIA_STREAM)
