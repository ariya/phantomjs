/*
 * Copyright (C) 2012 Apple Inc. All rights reserved.
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

#ifndef RemoteNetworkingContext_h
#define RemoteNetworkingContext_h

#include <WebCore/NetworkingContext.h>

namespace WebKit {

class RemoteNetworkingContext : public WebCore::NetworkingContext {
public:
    static PassRefPtr<RemoteNetworkingContext> create(bool needsSiteSpecificQuirks, bool localFileContentSniffingEnabled, bool privateBrowsingEnabled, bool shouldClearReferrerOnHTTPSToHTTPRedirect)
    {
        return adoptRef(new RemoteNetworkingContext(needsSiteSpecificQuirks, localFileContentSniffingEnabled, privateBrowsingEnabled, shouldClearReferrerOnHTTPSToHTTPRedirect));
    }
    virtual ~RemoteNetworkingContext();

    static void setPrivateBrowsingStorageSessionIdentifierBase(const String&);
    static void ensurePrivateBrowsingSession();
    static void destroyPrivateBrowsingSession();

    static WebCore::NetworkStorageSession* privateBrowsingSession();

    virtual bool shouldClearReferrerOnHTTPSToHTTPRedirect() const OVERRIDE;

private:
    RemoteNetworkingContext(bool needsSiteSpecificQuirks, bool localFileContentSniffingEnabled, bool privateBrowsingEnabled, bool m_shouldClearReferrerOnHTTPSToHTTPRedirect);

    virtual bool isValid() const OVERRIDE;

    virtual bool needsSiteSpecificQuirks() const OVERRIDE;
    virtual bool localFileContentSniffingEnabled() const OVERRIDE;
    virtual WebCore::NetworkStorageSession& storageSession() const OVERRIDE;
    virtual RetainPtr<CFDataRef> sourceApplicationAuditData() const OVERRIDE;
    virtual WebCore::ResourceError blockedError(const WebCore::ResourceRequest&) const OVERRIDE;

    bool m_needsSiteSpecificQuirks;
    bool m_localFileContentSniffingEnabled;
    bool m_privateBrowsingEnabled;
    bool m_shouldClearReferrerOnHTTPSToHTTPRedirect;
};

}

#endif // RemoteNetworkingContext_h
