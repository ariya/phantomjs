/*
 * Copyright (C) 2011, 2013 Apple Inc. All rights reserved.
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

#ifndef WebResourceCacheManagerProxy_h
#define WebResourceCacheManagerProxy_h

#include "APIObject.h"
#include "Arguments.h"
#include "GenericCallback.h"
#include "MessageReceiver.h"
#include "ResourceCachesToClear.h"
#include "WebContextSupplement.h"
#include <wtf/HashMap.h>
#include <wtf/PassRefPtr.h>

namespace WebKit {

struct SecurityOriginData;
class WebContext;
class WebProcessProxy;
class WebSecurityOrigin;

typedef GenericCallback<WKArrayRef> ArrayCallback;

class WebResourceCacheManagerProxy : public TypedAPIObject<APIObject::TypeCacheManager>, public WebContextSupplement, private CoreIPC::MessageReceiver {
public:
    static const char* supplementName();

    static PassRefPtr<WebResourceCacheManagerProxy> create(WebContext*);
    virtual ~WebResourceCacheManagerProxy();

    void getCacheOrigins(PassRefPtr<ArrayCallback>);
    void clearCacheForOrigin(WebSecurityOrigin*, ResourceCachesToClear);
    void clearCacheForAllOrigins(ResourceCachesToClear);

    using APIObject::ref;
    using APIObject::deref;

private:
    explicit WebResourceCacheManagerProxy(WebContext*);

    // WebContextSupplement
    virtual void contextDestroyed() OVERRIDE;
    virtual void processDidClose(WebProcessProxy*) OVERRIDE;
    virtual bool shouldTerminate(WebProcessProxy*) const OVERRIDE;
    virtual void refWebContextSupplement() OVERRIDE;
    virtual void derefWebContextSupplement() OVERRIDE;

    // CoreIPC::MessageReceiver
    virtual void didReceiveMessage(CoreIPC::Connection*, CoreIPC::MessageDecoder&) OVERRIDE;

    // Message handlers.
    void didGetCacheOrigins(const Vector<SecurityOriginData>& originIdentifiers, uint64_t callbackID);

    HashMap<uint64_t, RefPtr<ArrayCallback> > m_arrayCallbacks;
};

} // namespace WebKit

#endif // DatabaseManagerProxy_h
