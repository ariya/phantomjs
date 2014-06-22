/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
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

#ifndef WebPageGroupProxy_h
#define WebPageGroupProxy_h

#include "APIObject.h"
#include "WebPageGroupData.h"
#include <wtf/PassRefPtr.h>

namespace CoreIPC {
class Connection;
class MessageDecoder;
}

namespace WebCore {
class PageGroup;
}

namespace WebKit {

class WebPageGroupProxy : public TypedAPIObject<APIObject::TypeBundlePageGroup> {
public:
    static PassRefPtr<WebPageGroupProxy> create(const WebPageGroupData&);
    virtual ~WebPageGroupProxy();

    const String& identifier() const { return m_data.identifer; }
    uint64_t pageGroupID() const { return m_data.pageGroupID; }
    bool isVisibleToInjectedBundle() const { return m_data.visibleToInjectedBundle; }
    bool isVisibleToHistoryClient() const { return m_data.visibleToHistoryClient; }
    WebCore::PageGroup* corePageGroup() const { return m_pageGroup; }

    void didReceiveMessage(CoreIPC::Connection*, CoreIPC::MessageDecoder&);

private:
    WebPageGroupProxy(const WebPageGroupData&);

    void addUserStyleSheet(const WebCore::UserStyleSheet&);
    void addUserScript(const WebCore::UserScript&);
    void removeAllUserStyleSheets();
    void removeAllUserScripts();
    void removeAllUserContent();

    WebPageGroupData m_data;
    WebCore::PageGroup* m_pageGroup;
};

} // namespace WebKit

#endif // WebPageGroupProxy_h
