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

#include "config.h"
#include "WebPageGroup.h"

#include "WebPageGroupProxyMessages.h"
#include "WebPageProxy.h"
#include "WebPreferences.h"
#include <wtf/HashMap.h>
#include <wtf/text/StringConcatenate.h>

namespace WebKit {

static uint64_t generatePageGroupID()
{
    static uint64_t uniquePageGroupID = 1;
    return uniquePageGroupID++;
}

typedef HashMap<uint64_t, WebPageGroup*> WebPageGroupMap;

static WebPageGroupMap& webPageGroupMap()
{
    DEFINE_STATIC_LOCAL(WebPageGroupMap, map, ());
    return map;
}

PassRefPtr<WebPageGroup> WebPageGroup::create(const String& identifier, bool visibleToInjectedBundle, bool visibleToHistoryClient)
{
    RefPtr<WebPageGroup> pageGroup = adoptRef(new WebPageGroup(identifier, visibleToInjectedBundle, visibleToHistoryClient));

    webPageGroupMap().set(pageGroup->pageGroupID(), pageGroup.get());

    return pageGroup.release();
}

WebPageGroup* WebPageGroup::get(uint64_t pageGroupID)
{
    return webPageGroupMap().get(pageGroupID);
}

WebPageGroup::WebPageGroup(const String& identifier, bool visibleToInjectedBundle, bool visibleToHistoryClient)
{
    m_data.pageGroupID = generatePageGroupID();

    if (!identifier.isNull())
        m_data.identifer = identifier;
    else
        m_data.identifer = m_data.identifer = makeString("__uniquePageGroupID-", String::number(m_data.pageGroupID));

    m_data.visibleToInjectedBundle = visibleToInjectedBundle;
    m_data.visibleToHistoryClient = visibleToHistoryClient;
}

WebPageGroup::~WebPageGroup()
{
    if (m_preferences)
        m_preferences->removePageGroup(this);
    webPageGroupMap().remove(pageGroupID());
}

void WebPageGroup::addPage(WebPageProxy* page)
{
    m_pages.add(page);
}

void WebPageGroup::removePage(WebPageProxy* page)
{
    m_pages.remove(page);
}

void WebPageGroup::setPreferences(WebPreferences* preferences)
{
    if (preferences == m_preferences)
        return;

    if (!m_preferences) {
        m_preferences = preferences;
        m_preferences->addPageGroup(this);
    } else {
        m_preferences->removePageGroup(this);
        m_preferences = preferences;
        m_preferences->addPageGroup(this);

        preferencesDidChange();
    }
}

WebPreferences* WebPageGroup::preferences() const
{
    if (!m_preferences) {
        if (!m_data.identifer.isNull())
            m_preferences = WebPreferences::create(m_data.identifer);
        else
            m_preferences = WebPreferences::create();
        m_preferences->addPageGroup(const_cast<WebPageGroup*>(this));
    }
    return m_preferences.get();
}

void WebPageGroup::preferencesDidChange()
{
    for (HashSet<WebPageProxy*>::iterator it = m_pages.begin(), end = m_pages.end(); it != end; ++it) {
        WebPageProxy* page = *it;
        page->preferencesDidChange();
    }
}

static Vector<String> toStringVector(ImmutableArray* array)
{
    Vector<String> patternVector;
    if (!array)
        return patternVector;

    size_t size = array->size();
    if (!size)
        return patternVector;
    
    patternVector.reserveInitialCapacity(size);
    for (size_t i = 0; i < size; ++i) {
        WebString* webString = array->at<WebString>(i);
        ASSERT(webString);
        patternVector.uncheckedAppend(webString->string());
    }
    
    return patternVector;
}

void WebPageGroup::addUserStyleSheet(const String& source, const String& baseURL, ImmutableArray* whitelist, ImmutableArray* blacklist, WebCore::UserContentInjectedFrames injectedFrames, WebCore::UserStyleLevel level)
{
    if (source.isEmpty())
        return;

    WebCore::UserStyleSheet userStyleSheet = WebCore::UserStyleSheet(source, (baseURL.isEmpty() ? WebCore::blankURL() : WebCore::KURL(WebCore::KURL(), baseURL)), toStringVector(whitelist), toStringVector(blacklist), injectedFrames, level);

    m_data.userStyleSheets.append(userStyleSheet);
    sendToAllProcessesInGroup(Messages::WebPageGroupProxy::AddUserStyleSheet(userStyleSheet), m_data.pageGroupID);
}

void WebPageGroup::addUserScript(const String& source, const String& baseURL, ImmutableArray* whitelist, ImmutableArray* blacklist, WebCore::UserContentInjectedFrames injectedFrames, WebCore::UserScriptInjectionTime injectionTime)
{
    if (source.isEmpty())
        return;

    WebCore::UserScript userScript = WebCore::UserScript(source, (baseURL.isEmpty() ? WebCore::blankURL() : WebCore::KURL(WebCore::KURL(), baseURL)), toStringVector(whitelist), toStringVector(blacklist), injectionTime, injectedFrames);

    m_data.userScripts.append(userScript);
    sendToAllProcessesInGroup(Messages::WebPageGroupProxy::AddUserScript(userScript), m_data.pageGroupID);
}

void WebPageGroup::removeAllUserStyleSheets()
{
    m_data.userStyleSheets.clear();
    sendToAllProcessesInGroup(Messages::WebPageGroupProxy::RemoveAllUserStyleSheets(), m_data.pageGroupID);
}

void WebPageGroup::removeAllUserScripts()
{
    m_data.userScripts.clear();
    sendToAllProcessesInGroup(Messages::WebPageGroupProxy::RemoveAllUserScripts(), m_data.pageGroupID);
}

void WebPageGroup::removeAllUserContent()
{
    m_data.userStyleSheets.clear();
    m_data.userScripts.clear();
    sendToAllProcessesInGroup(Messages::WebPageGroupProxy::RemoveAllUserContent(), m_data.pageGroupID);
}

} // namespace WebKit
