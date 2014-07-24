/*
 * Copyright (C) 2012 Google Inc. All rights reserved.
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

#ifndef UserMediaController_h
#define UserMediaController_h

#if ENABLE(MEDIA_STREAM)

#include "Page.h"
#include "UserMediaClient.h"
#include <wtf/PassOwnPtr.h>

namespace WebCore {

class UserMediaController : public Supplement<Page> {
public:
    ~UserMediaController();

    UserMediaClient* client() const { return m_client; }
    void requestUserMedia(PassRefPtr<UserMediaRequest>, const MediaStreamSourceVector& audioSources,  const MediaStreamSourceVector& videoSources);
    void cancelUserMediaRequest(UserMediaRequest*);

    static PassOwnPtr<UserMediaController> create(UserMediaClient*);
    static const char* supplementName();
    static UserMediaController* from(Page* page) { return static_cast<UserMediaController*>(Supplement<Page>::from(page, supplementName())); }

protected:
    explicit UserMediaController(UserMediaClient*);

private:
    UserMediaClient* m_client;
};

inline void UserMediaController::requestUserMedia(PassRefPtr<UserMediaRequest> request, const MediaStreamSourceVector& audioSources,  const MediaStreamSourceVector& videoSources)
{
    m_client->requestUserMedia(request, audioSources, videoSources);
}

inline void UserMediaController::cancelUserMediaRequest(UserMediaRequest* request)
{
    m_client->cancelUserMediaRequest(request);
}

} // namespace WebCore

#endif // ENABLE(MEDIA_STREAM)

#endif // UserMediaController_h
