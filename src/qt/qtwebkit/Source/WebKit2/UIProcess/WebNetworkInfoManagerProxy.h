/*
 * Copyright (C) 2012 Intel Corporation. All rights reserved.
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

#ifndef WebNetworkInfoManagerProxy_h
#define WebNetworkInfoManagerProxy_h

#if ENABLE(NETWORK_INFO)

#include "APIObject.h"
#include "MessageReceiver.h"
#include "WebContextSupplement.h"
#include "WebNetworkInfoProvider.h"
#include <wtf/Forward.h>

namespace WebKit {

class WebContext;
class WebNetworkInfo;

class WebNetworkInfoManagerProxy : public TypedAPIObject<APIObject::TypeNetworkInfoManager>, public WebContextSupplement, private CoreIPC::MessageReceiver {
public:
    static const char* supplementName();

    static PassRefPtr<WebNetworkInfoManagerProxy> create(WebContext*);
    virtual ~WebNetworkInfoManagerProxy();

    void initializeProvider(const WKNetworkInfoProvider*);

    void providerDidChangeNetworkInformation(const WTF::AtomicString& eventType, WebNetworkInfo*);

    using APIObject::ref;
    using APIObject::deref;

private:
    explicit WebNetworkInfoManagerProxy(WebContext*);

    // WebContextSupplement
    virtual void contextDestroyed() OVERRIDE;
    virtual void processDidClose(WebProcessProxy*) OVERRIDE;
    virtual void refWebContextSupplement() OVERRIDE;
    virtual void derefWebContextSupplement() OVERRIDE;

    // CoreIPC::MessageReceiver
    virtual void didReceiveMessage(CoreIPC::Connection*, CoreIPC::MessageDecoder&) OVERRIDE;
    virtual void didReceiveSyncMessage(CoreIPC::Connection*, CoreIPC::MessageDecoder&, OwnPtr<CoreIPC::MessageEncoder>&) OVERRIDE;

    void startUpdating();
    void stopUpdating();

    void getBandwidth(double&);
    void isMetered(bool&);

    bool m_isUpdating;

    WebContext* m_context;
    WebNetworkInfoProvider m_provider;
};

} // namespace WebKit

#endif // ENABLE(NETWORK_INFO)

#endif // WebNetworkInfoManagerProxy_h
