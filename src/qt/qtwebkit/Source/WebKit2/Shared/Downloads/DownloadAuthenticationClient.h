/*
 * Copyright (C) 2011 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef DownloadAuthenticationClient_h
#define DownloadAuthenticationClient_h

#include <WebCore/AuthenticationClient.h>
#include <wtf/PassRefPtr.h>
#include <wtf/RefCounted.h>

namespace WebCore {
    class AuthenticationChallenge;
    class Credential;
}

namespace WebKit {

class Download;

class DownloadAuthenticationClient : public RefCounted<DownloadAuthenticationClient>, public WebCore::AuthenticationClient {
public:
    static PassRefPtr<DownloadAuthenticationClient> create(Download* download) { return adoptRef(new DownloadAuthenticationClient(download)); }

    void detach() { m_download = 0; }

    using RefCounted<DownloadAuthenticationClient>::ref;
    using RefCounted<DownloadAuthenticationClient>::deref;

private:
    DownloadAuthenticationClient(Download*);

    virtual void receivedCredential(const WebCore::AuthenticationChallenge&, const WebCore::Credential&);
    virtual void receivedRequestToContinueWithoutCredential(const WebCore::AuthenticationChallenge&);
    virtual void receivedCancellation(const WebCore::AuthenticationChallenge&);

    virtual void refAuthenticationClient() { ref(); }
    virtual void derefAuthenticationClient() { deref(); }

    Download* m_download;
};

} // namespace WebKit

#endif // DownloadAuthenticationClient_h
