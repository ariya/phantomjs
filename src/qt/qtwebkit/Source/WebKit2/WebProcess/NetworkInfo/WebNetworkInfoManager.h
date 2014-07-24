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

#ifndef WebNetworkInfoManager_h
#define WebNetworkInfoManager_h

#if ENABLE(NETWORK_INFO)

#include "MessageReceiver.h"
#include "WebCoreArgumentCoders.h"
#include "WebNetworkInfo.h"
#include "WebProcessSupplement.h"
#include <wtf/HashSet.h>
#include <wtf/Noncopyable.h>
#include <wtf/text/AtomicString.h>

namespace WebKit {

class WebPage;
class WebProcess;

class WebNetworkInfoManager : public WebProcessSupplement, private CoreIPC::MessageReceiver {
    WTF_MAKE_NONCOPYABLE(WebNetworkInfoManager);
public:
    explicit WebNetworkInfoManager(WebProcess*);
    ~WebNetworkInfoManager();

    static const char* supplementName();

    void registerWebPage(WebPage*);
    void unregisterWebPage(WebPage*);

    double bandwidth(WebPage*) const;
    bool metered(WebPage*) const;
private:
    // CoreIPC::MessageReceiver
    void didReceiveMessage(CoreIPC::Connection*, CoreIPC::MessageDecoder&) OVERRIDE;

    void didChangeNetworkInformation(const AtomicString& eventType, const WebNetworkInfo::Data&);

    WebProcess* m_process;
    HashSet<WebPage*> m_pageSet;
};

} // namespace WebKit

#endif // ENABLE(NETWORK_INFO)

#endif // WebNetworkInfoManager_h
