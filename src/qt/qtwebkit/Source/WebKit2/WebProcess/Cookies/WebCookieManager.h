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

#ifndef WebCookieManager_h
#define WebCookieManager_h

#include "HTTPCookieAcceptPolicy.h"
#include "MessageReceiver.h"
#include "NetworkProcessSupplement.h"
#include "WebProcessSupplement.h"
#include <stdint.h>
#include <wtf/Forward.h>
#include <wtf/Noncopyable.h>

#if USE(SOUP)
#include "SoupCookiePersistentStorageType.h"
#endif

namespace WebKit {

class ChildProcess;

class WebCookieManager : public WebProcessSupplement, public NetworkProcessSupplement, public CoreIPC::MessageReceiver {
    WTF_MAKE_NONCOPYABLE(WebCookieManager);
public:
    WebCookieManager(ChildProcess*);

    static const char* supplementName();

    void setHTTPCookieAcceptPolicy(HTTPCookieAcceptPolicy);
#if USE(SOUP)
    void setCookiePersistentStorage(const String& storagePath, uint32_t storageType);
#endif

private:
    // CoreIPC::MessageReceiver
    virtual void didReceiveMessage(CoreIPC::Connection*, CoreIPC::MessageDecoder&) OVERRIDE;

    void getHostnamesWithCookies(uint64_t callbackID);
    void deleteCookiesForHostname(const String&);
    void deleteAllCookies();

    void platformSetHTTPCookieAcceptPolicy(HTTPCookieAcceptPolicy);
    void getHTTPCookieAcceptPolicy(uint64_t callbackID);
    HTTPCookieAcceptPolicy platformGetHTTPCookieAcceptPolicy();

    void startObservingCookieChanges();
    void stopObservingCookieChanges();

    static void cookiesDidChange();
    void dispatchCookiesDidChange();


    ChildProcess* m_process;
};

} // namespace WebKit

#endif // WebCookieManager_h
