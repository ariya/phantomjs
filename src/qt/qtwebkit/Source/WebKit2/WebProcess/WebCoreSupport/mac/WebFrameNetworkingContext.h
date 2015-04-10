/*
 * Copyright (C) 2013 Apple Inc. All rights reserved.
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

#ifndef WebFrameNetworkingContext_h
#define WebFrameNetworkingContext_h

#include "HTTPCookieAcceptPolicy.h"
#include "WebFrame.h"
#include <WebCore/FrameNetworkingContext.h>

namespace WebKit {

class WebFrameNetworkingContext : public WebCore::FrameNetworkingContext {
public:
    static PassRefPtr<WebFrameNetworkingContext> create(WebFrame* frame)
    {
        return adoptRef(new WebFrameNetworkingContext(frame));
    }

    static void setPrivateBrowsingStorageSessionIdentifierBase(const String&);
    static void ensurePrivateBrowsingSession();
    static void destroyPrivateBrowsingSession();
    static void setCookieAcceptPolicyForAllContexts(HTTPCookieAcceptPolicy);

private:
    WebFrameNetworkingContext(WebFrame* frame)
        : WebCore::FrameNetworkingContext(frame->coreFrame())
    {
    }

    virtual bool needsSiteSpecificQuirks() const OVERRIDE;
    virtual bool localFileContentSniffingEnabled() const OVERRIDE;
    virtual SchedulePairHashSet* scheduledRunLoopPairs() const OVERRIDE;
    virtual RetainPtr<CFDataRef> sourceApplicationAuditData() const OVERRIDE;
    virtual WebCore::ResourceError blockedError(const WebCore::ResourceRequest&) const OVERRIDE;
    virtual WebCore::NetworkStorageSession& storageSession() const OVERRIDE;
};

}

#endif
