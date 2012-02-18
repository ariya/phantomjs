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

#ifndef MediaStreamFrameController_h
#define MediaStreamFrameController_h

#if ENABLE(MEDIA_STREAM)

#include <wtf/Forward.h>
#include <wtf/HashMap.h>
#include <wtf/Noncopyable.h>

namespace WebCore {

class Frame;
class MediaStreamController;
class Page;
class ScriptExecutionContext;
class SecurityOrigin;

class MediaStreamFrameController {
    WTF_MAKE_NONCOPYABLE(MediaStreamFrameController);
public:
    MediaStreamFrameController(Frame*);
    virtual ~MediaStreamFrameController();

    SecurityOrigin* securityOrigin() const;
    ScriptExecutionContext* scriptExecutionContext() const;

    void disconnectPage();
    void disconnectFrame();
    void transferToNewPage(Page*);

private:
    class Request;

    class RequestMap : public HashMap<int, RefPtr<Request> > {
    public:
        void abort(int requestId);
        void abortAll();
    };

    // Detached from a page, and hence from a embedder client.
    void enterDetachedState();

    MediaStreamController* pageController() const;

    RequestMap m_requests;

    Frame* m_frame;
    bool m_isInDetachedState;
};

} // namespace WebCore

#endif // ENABLE(MEDIA_STREAM)

#endif // MediaStreamFrameController_h
