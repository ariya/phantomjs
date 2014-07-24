/*
 * Copyright (C) 2011 Apple Inc. All rights reserved.
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
#include "WebNotificationProvider.h"

#include "ImmutableDictionary.h"
#include "MutableArray.h"
#include "WKAPICast.h"
#include "WebNotification.h"
#include "WebNotificationManagerProxy.h"
#include "WebNumber.h"
#include "WebSecurityOrigin.h"

namespace WebKit {

void WebNotificationProvider::show(WebPageProxy* page, WebNotification* notification)
{
    if (!m_client.show)
        return;
    
    m_client.show(toAPI(page), toAPI(notification), m_client.clientInfo);
}

void WebNotificationProvider::cancel(WebNotification* notification)
{
    if (!m_client.cancel)
        return;
    
    m_client.cancel(toAPI(notification), m_client.clientInfo);
}

void WebNotificationProvider::didDestroyNotification(WebNotification* notification)
{
    if (!m_client.didDestroyNotification)
        return;
    
    m_client.didDestroyNotification(toAPI(notification), m_client.clientInfo);
}

void WebNotificationProvider::clearNotifications(const Vector<uint64_t>& notificationIDs)
{
    if (!m_client.clearNotifications)
        return;

    RefPtr<MutableArray> arrayIDs = MutableArray::create();
    size_t count = notificationIDs.size();
    arrayIDs->reserveCapacity(count);
    for (size_t i = 0; i < count; ++i)
        arrayIDs->append(WebUInt64::create(notificationIDs[i]).leakRef());

    m_client.clearNotifications(toAPI(arrayIDs.get()), m_client.clientInfo);
}

void WebNotificationProvider::addNotificationManager(WebNotificationManagerProxy* manager)
{
    if (!m_client.addNotificationManager)
        return;
    
    m_client.addNotificationManager(toAPI(manager), m_client.clientInfo);
}

void WebNotificationProvider::removeNotificationManager(WebNotificationManagerProxy* manager)
{
    if (!m_client.removeNotificationManager)
        return;
    
    m_client.removeNotificationManager(toAPI(manager), m_client.clientInfo);
}

PassRefPtr<ImmutableDictionary> WebNotificationProvider::notificationPermissions()
{
    if (!m_client.notificationPermissions)
        return ImmutableDictionary::create();

    return adoptRef(toImpl(m_client.notificationPermissions(m_client.clientInfo)));
}

} // namespace WebKit
