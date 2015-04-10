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

#include "config.h"
#include "WebNotificationManagerProxy.h"

#include "ImmutableArray.h"
#include "ImmutableDictionary.h"
#include "WebContext.h"
#include "WebNotification.h"
#include "WebNotificationManagerMessages.h"
#include "WebPageProxy.h"
#include "WebProcessProxy.h"
#include "WebSecurityOrigin.h"

using namespace std;
using namespace WTF;
using namespace WebCore;

namespace WebKit {

static uint64_t generateGlobalNotificationID()
{
    static uint64_t uniqueGlobalNotificationID = 1;
    return uniqueGlobalNotificationID++;
}

const char* WebNotificationManagerProxy::supplementName()
{
    return "WebNotificationManagerProxy";
}

PassRefPtr<WebNotificationManagerProxy> WebNotificationManagerProxy::create(WebContext* context)
{
    return adoptRef(new WebNotificationManagerProxy(context));
}

WebNotificationManagerProxy::WebNotificationManagerProxy(WebContext* context)
    : WebContextSupplement(context)
{
}

void WebNotificationManagerProxy::initializeProvider(const WKNotificationProvider *provider)
{
    m_provider.initialize(provider);
    m_provider.addNotificationManager(this);
}

// WebContextSupplement

void WebNotificationManagerProxy::contextDestroyed()
{
    m_provider.removeNotificationManager(this);
}

void WebNotificationManagerProxy::refWebContextSupplement()
{
    APIObject::ref();
}

void WebNotificationManagerProxy::derefWebContextSupplement()
{
    APIObject::deref();
}

void WebNotificationManagerProxy::populateCopyOfNotificationPermissions(HashMap<String, bool>& permissions)
{
    RefPtr<ImmutableDictionary> knownPermissions = m_provider.notificationPermissions();

    if (!knownPermissions)
        return;

    permissions.clear();
    RefPtr<ImmutableArray> knownOrigins = knownPermissions->keys();
    for (size_t i = 0; i < knownOrigins->size(); ++i) {
        WebString* origin = knownOrigins->at<WebString>(i);
        permissions.set(origin->string(), knownPermissions->get<WebBoolean>(origin->string())->value());
    }
}

void WebNotificationManagerProxy::show(WebPageProxy* webPage, const String& title, const String& body, const String& iconURL, const String& tag, const String& lang, const String& dir, const String& originString, uint64_t pageNotificationID)
{
    uint64_t globalNotificationID = generateGlobalNotificationID();
    RefPtr<WebNotification> notification = WebNotification::create(title, body, iconURL, tag, lang, dir, originString, globalNotificationID);
    pair<uint64_t, uint64_t> notificationIDPair = make_pair(webPage->pageID(), pageNotificationID);
    m_globalNotificationMap.set(globalNotificationID, notificationIDPair);
    m_notifications.set(notificationIDPair, make_pair(globalNotificationID, notification));
    m_provider.show(webPage, notification.get());
}

void WebNotificationManagerProxy::cancel(WebPageProxy* webPage, uint64_t pageNotificationID)
{
    if (WebNotification* notification = m_notifications.get(make_pair(webPage->pageID(), pageNotificationID)).second.get())
        m_provider.cancel(notification);
}
    
void WebNotificationManagerProxy::didDestroyNotification(WebPageProxy* webPage, uint64_t pageNotificationID)
{
    pair<uint64_t, RefPtr<WebNotification> > globalIDNotificationPair = m_notifications.take(make_pair(webPage->pageID(), pageNotificationID));
    if (uint64_t globalNotificationID = globalIDNotificationPair.first) {
        WebNotification* notification = globalIDNotificationPair.second.get();
        m_globalNotificationMap.remove(globalNotificationID);
        m_provider.didDestroyNotification(notification);
    }
}

static bool pageIDsMatch(uint64_t pageID, uint64_t, uint64_t desiredPageID, const Vector<uint64_t>&)
{
    return pageID == desiredPageID;
}

static bool pageAndNotificationIDsMatch(uint64_t pageID, uint64_t pageNotificationID, uint64_t desiredPageID, const Vector<uint64_t>& desiredPageNotificationIDs)
{
    return pageID == desiredPageID && desiredPageNotificationIDs.contains(pageNotificationID);
}

void WebNotificationManagerProxy::clearNotifications(WebPageProxy* webPage)
{
    clearNotifications(webPage, Vector<uint64_t>(), pageIDsMatch);
}

void WebNotificationManagerProxy::clearNotifications(WebPageProxy* webPage, const Vector<uint64_t>& pageNotificationIDs)
{
    clearNotifications(webPage, pageNotificationIDs, pageAndNotificationIDsMatch);
}

void WebNotificationManagerProxy::clearNotifications(WebPageProxy* webPage, const Vector<uint64_t>& pageNotificationIDs, NotificationFilterFunction filterFunction)
{
    uint64_t targetPageID = webPage->pageID();

    Vector<uint64_t> globalNotificationIDs;
    globalNotificationIDs.reserveCapacity(m_globalNotificationMap.size());

    {
        HashMap<pair<uint64_t, uint64_t>, pair<uint64_t, RefPtr<WebNotification> > >::iterator it  = m_notifications.begin();
        HashMap<pair<uint64_t, uint64_t>, pair<uint64_t, RefPtr<WebNotification> > >::iterator end  = m_notifications.end();
        for (; it != end; ++it) {
            uint64_t webPageID = it->key.first;
            uint64_t pageNotificationID = it->key.second;
            if (!filterFunction(webPageID, pageNotificationID, targetPageID, pageNotificationIDs))
                continue;

            uint64_t globalNotificationID = it->value.first;
            globalNotificationIDs.append(globalNotificationID);
        }
    }


    {
        Vector<uint64_t>::iterator it = globalNotificationIDs.begin();
        Vector<uint64_t>::iterator end = globalNotificationIDs.end();
        for (; it != end; ++it) {
            pair<uint64_t, uint64_t> pageNotification = m_globalNotificationMap.take(*it);
            m_notifications.remove(pageNotification);
        }
    }

    m_provider.clearNotifications(globalNotificationIDs);
}

void WebNotificationManagerProxy::providerDidShowNotification(uint64_t globalNotificationID)
{
    HashMap<uint64_t, pair<uint64_t, uint64_t> >::iterator it = m_globalNotificationMap.find(globalNotificationID);
    if (it == m_globalNotificationMap.end())
        return;

    uint64_t webPageID = it->value.first;
    WebPageProxy* webPage = WebProcessProxy::webPage(webPageID);
    if (!webPage)
        return;

    uint64_t pageNotificationID = it->value.second;
    webPage->process()->send(Messages::WebNotificationManager::DidShowNotification(pageNotificationID), 0);
}

void WebNotificationManagerProxy::providerDidClickNotification(uint64_t globalNotificationID)
{
    HashMap<uint64_t, pair<uint64_t, uint64_t> >::iterator it = m_globalNotificationMap.find(globalNotificationID);
    if (it == m_globalNotificationMap.end())
        return;
    
    uint64_t webPageID = it->value.first;
    WebPageProxy* webPage = WebProcessProxy::webPage(webPageID);
    if (!webPage)
        return;

    uint64_t pageNotificationID = it->value.second;
    webPage->process()->send(Messages::WebNotificationManager::DidClickNotification(pageNotificationID), 0);
}


void WebNotificationManagerProxy::providerDidCloseNotifications(ImmutableArray* globalNotificationIDs)
{
    HashMap<WebPageProxy*, Vector<uint64_t> > pageNotificationIDs;
    
    size_t size = globalNotificationIDs->size();
    for (size_t i = 0; i < size; ++i) {
        HashMap<uint64_t, pair<uint64_t, uint64_t> >::iterator it = m_globalNotificationMap.find(globalNotificationIDs->at<WebUInt64>(i)->value());
        if (it == m_globalNotificationMap.end())
            continue;

        if (WebPageProxy* webPage = WebProcessProxy::webPage(it->value.first)) {
            HashMap<WebPageProxy*, Vector<uint64_t> >::iterator pageIt = pageNotificationIDs.find(webPage);
            if (pageIt == pageNotificationIDs.end()) {
                Vector<uint64_t> newVector;
                newVector.reserveInitialCapacity(size);
                pageIt = pageNotificationIDs.add(webPage, newVector).iterator;
            }

            uint64_t pageNotificationID = it->value.second;
            pageIt->value.append(pageNotificationID);
        }

        m_notifications.remove(it->value);
        m_globalNotificationMap.remove(it);
    }

    for (HashMap<WebPageProxy*, Vector<uint64_t> >::iterator it = pageNotificationIDs.begin(), end = pageNotificationIDs.end(); it != end; ++it)
        it->key->process()->send(Messages::WebNotificationManager::DidCloseNotifications(it->value), 0);
}

void WebNotificationManagerProxy::providerDidUpdateNotificationPolicy(const WebSecurityOrigin* origin, bool allowed)
{
    if (!context())
        return;

    context()->sendToAllProcesses(Messages::WebNotificationManager::DidUpdateNotificationDecision(origin->toString(), allowed));
}

void WebNotificationManagerProxy::providerDidRemoveNotificationPolicies(ImmutableArray* origins)
{
    if (!context())
        return;

    size_t size = origins->size();
    if (!size)
        return;
    
    Vector<String> originStrings;
    originStrings.reserveInitialCapacity(size);
    
    for (size_t i = 0; i < size; ++i)
        originStrings.append(origins->at<WebSecurityOrigin>(i)->toString());
    
    context()->sendToAllProcesses(Messages::WebNotificationManager::DidRemoveNotificationDecisions(originStrings));
}

} // namespace WebKit
