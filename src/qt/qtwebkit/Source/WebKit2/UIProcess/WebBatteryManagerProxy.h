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

#ifndef WebBatteryManagerProxy_h
#define WebBatteryManagerProxy_h

#if ENABLE(BATTERY_STATUS)

#include "APIObject.h"
#include "MessageReceiver.h"
#include "WebBatteryProvider.h"
#include "WebContextSupplement.h"
#include <wtf/Forward.h>

namespace WebKit {

class WebContext;
class WebBatteryStatus;

class WebBatteryManagerProxy : public TypedAPIObject<APIObject::TypeBatteryManager>, public WebContextSupplement, private CoreIPC::MessageReceiver {
public:
    static const char* supplementName();

    static PassRefPtr<WebBatteryManagerProxy> create(WebContext*);
    virtual ~WebBatteryManagerProxy();

    void initializeProvider(const WKBatteryProvider*);

    void providerDidChangeBatteryStatus(const WTF::AtomicString&, WebBatteryStatus*);
    void providerUpdateBatteryStatus(WebBatteryStatus*);

    using APIObject::ref;
    using APIObject::deref;

private:
    explicit WebBatteryManagerProxy(WebContext*);

    // WebContextSupplement
    virtual void contextDestroyed() OVERRIDE;
    virtual void processDidClose(WebProcessProxy*) OVERRIDE;
    virtual void refWebContextSupplement() OVERRIDE;
    virtual void derefWebContextSupplement() OVERRIDE;

    // CoreIPC::MessageReceiver
    virtual void didReceiveMessage(CoreIPC::Connection*, CoreIPC::MessageDecoder&) OVERRIDE;

    void startUpdating();
    void stopUpdating();

    bool m_isUpdating;

    WebBatteryProvider m_provider;
};

} // namespace WebKit

#endif // ENABLE(BATTERY_STATUS)

#endif // WebBatteryManagerProxy_h
