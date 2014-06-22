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
#include "NotificationPermissionRequestManagerProxy.h"

#include "NotificationPermissionRequest.h"
#include "WebPageMessages.h"
#include "WebPageProxy.h"
#include "WebProcessProxy.h"

namespace WebKit {

NotificationPermissionRequestManagerProxy::NotificationPermissionRequestManagerProxy(WebPageProxy* page)
    : m_page(page)
{
}

void NotificationPermissionRequestManagerProxy::invalidateRequests()
{
    PendingRequestMap::const_iterator it = m_pendingRequests.begin();
    PendingRequestMap::const_iterator end = m_pendingRequests.end();
    for (; it != end; ++it)
        it->value->invalidate();
    
    m_pendingRequests.clear();
}

PassRefPtr<NotificationPermissionRequest> NotificationPermissionRequestManagerProxy::createRequest(uint64_t notificationID)
{
    RefPtr<NotificationPermissionRequest> request = NotificationPermissionRequest::create(this, notificationID);
    m_pendingRequests.add(notificationID, request.get());
    return request.release();
}

void NotificationPermissionRequestManagerProxy::didReceiveNotificationPermissionDecision(uint64_t notificationID, bool allow)
{
    if (!m_page->isValid())
        return;
    
    RefPtr<NotificationPermissionRequest> request = m_pendingRequests.take(notificationID);
    if (!request)
        return;
    
    m_page->process()->send(Messages::WebPage::DidReceiveNotificationPermissionDecision(notificationID, allow), m_page->pageID());
}

} // namespace WebKit
