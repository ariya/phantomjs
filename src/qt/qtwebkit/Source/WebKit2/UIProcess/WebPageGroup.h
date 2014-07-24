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

#ifndef WebPageGroup_h
#define WebPageGroup_h

#include "APIObject.h"
#include "WebPageGroupData.h"
#include "WebPageProxy.h"
#include "WebProcessProxy.h"
#include <wtf/Forward.h>
#include <wtf/HashSet.h>

namespace WebKit {

class WebPreferences;
class WebPageProxy;

class WebPageGroup : public TypedAPIObject<APIObject::TypePageGroup> {
public:
    static PassRefPtr<WebPageGroup> create(const String& identifier = String(), bool visibleToInjectedBundle = true, bool visibleToHistoryClient = true);
    static WebPageGroup* get(uint64_t pageGroupID);

    virtual ~WebPageGroup();

    void addPage(WebPageProxy*);
    void removePage(WebPageProxy*);

    const String& identifier() const { return m_data.identifer; }
    uint64_t pageGroupID() const { return m_data.pageGroupID; }

    const WebPageGroupData& data() { return m_data; }

    void setPreferences(WebPreferences*);
    WebPreferences* preferences() const;
    void preferencesDidChange();
    
    void addUserStyleSheet(const String& source, const String& baseURL, ImmutableArray* whitelist, ImmutableArray* blacklist, WebCore::UserContentInjectedFrames, WebCore::UserStyleLevel);
    void addUserScript(const String& source, const String& baseURL, ImmutableArray* whitelist, ImmutableArray* blacklist, WebCore::UserContentInjectedFrames, WebCore::UserScriptInjectionTime);
    void removeAllUserStyleSheets();
    void removeAllUserScripts();
    void removeAllUserContent();

private:
    WebPageGroup(const String& identifier, bool visibleToInjectedBundle, bool visibleToHistoryClient);

    template<typename MessageType> void sendToAllProcessesInGroup(const MessageType&, uint64_t destinationID);

    WebPageGroupData m_data;
    mutable RefPtr<WebPreferences> m_preferences;
    HashSet<WebPageProxy*> m_pages;
};
    
template<typename MessageType> inline void WebPageGroup::sendToAllProcessesInGroup(const MessageType& message, uint64_t destinationID)
{
    HashSet<WebProcessProxy*> processesSeen;
    for (HashSet<WebPageProxy*>::const_iterator it = m_pages.begin(), end = m_pages.end(); it != end; ++it) {
        WebProcessProxy* webProcessProxy = (*it)->process();
        ASSERT(webProcessProxy);
        if (!processesSeen.add(webProcessProxy).isNewEntry)
            continue;
        if (webProcessProxy->canSendMessage())
            webProcessProxy->send(message, destinationID);
    }
}

} // namespace WebKit

#endif // WebPageGroup_h
