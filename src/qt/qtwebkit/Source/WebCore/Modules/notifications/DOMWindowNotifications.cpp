/*
 * Copyright (C) 2006, 2007, 2008, 2010 Apple Inc. All rights reserved.
 * Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies)
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
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "DOMWindowNotifications.h"

#if ENABLE(NOTIFICATIONS) || ENABLE(LEGACY_NOTIFICATIONS)

#include "DOMWindow.h"
#include "Document.h"
#include "NotificationCenter.h"
#include "NotificationController.h"
#include "Page.h"

namespace WebCore {

DOMWindowNotifications::DOMWindowNotifications(DOMWindow* window)
    : DOMWindowProperty(window->frame())
    , m_window(window)
{
}

DOMWindowNotifications::~DOMWindowNotifications()
{
}

const char* DOMWindowNotifications::supplementName()
{
    return "DOMWindowNotifications";
}

DOMWindowNotifications* DOMWindowNotifications::from(DOMWindow* window)
{
    DOMWindowNotifications* supplement = static_cast<DOMWindowNotifications*>(Supplement<DOMWindow>::from(window, supplementName()));
    if (!supplement) {
        supplement = new DOMWindowNotifications(window);
        Supplement<DOMWindow>::provideTo(window, supplementName(), adoptPtr(supplement));
    }
    return supplement;
}

NotificationCenter* DOMWindowNotifications::webkitNotifications(DOMWindow* window)
{
    return DOMWindowNotifications::from(window)->webkitNotifications();
}

void DOMWindowNotifications::disconnectFrameForPageCache()
{
    m_suspendedNotificationCenter = m_notificationCenter.release();
    DOMWindowProperty::disconnectFrameForPageCache();
}

void DOMWindowNotifications::reconnectFrameFromPageCache(Frame* frame)
{
    DOMWindowProperty::reconnectFrameFromPageCache(frame);
    m_notificationCenter = m_suspendedNotificationCenter.release();
}

void DOMWindowNotifications::willDestroyGlobalObjectInCachedFrame()
{
    m_suspendedNotificationCenter = nullptr;
    DOMWindowProperty::willDestroyGlobalObjectInCachedFrame();
}

void DOMWindowNotifications::willDestroyGlobalObjectInFrame()
{
    m_notificationCenter = nullptr;
    DOMWindowProperty::willDestroyGlobalObjectInFrame();
}

void DOMWindowNotifications::willDetachGlobalObjectFromFrame()
{
    m_notificationCenter = nullptr;
    DOMWindowProperty::willDetachGlobalObjectFromFrame();
}

NotificationCenter* DOMWindowNotifications::webkitNotifications()
{
    if (!m_window->isCurrentlyDisplayedInFrame())
        return 0;

    if (m_notificationCenter)
        return m_notificationCenter.get();

    Document* document = m_window->document();
    if (!document)
        return 0;
    
    Page* page = document->page();
    if (!page)
        return 0;

    NotificationClient* provider = NotificationController::clientFrom(page);
    if (provider) 
        m_notificationCenter = NotificationCenter::create(document, provider);    

    return m_notificationCenter.get();
}

} // namespace WebCore

#endif // ENABLE(NOTIFICATIONS) || ENABLE(LEGACY_NOTIFICATIONS)
