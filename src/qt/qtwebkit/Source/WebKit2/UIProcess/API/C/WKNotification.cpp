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
#include "WKNotification.h"

#include "WKAPICast.h"
#include "WebNotification.h"
#include "WebSecurityOrigin.h"

using namespace WebKit;

WKTypeID WKNotificationGetTypeID()
{
    return toAPI(WebNotification::APIType);
}

WKStringRef WKNotificationCopyTitle(WKNotificationRef notification)
{
    return toCopiedAPI(toImpl(notification)->title());
}

WKStringRef WKNotificationCopyBody(WKNotificationRef notification)
{
    return toCopiedAPI(toImpl(notification)->body());
}

WKStringRef WKNotificationCopyIconURL(WKNotificationRef notification)
{
    return toCopiedAPI(toImpl(notification)->iconURL());
}

WKStringRef WKNotificationCopyTag(WKNotificationRef notification)
{
    return toCopiedAPI(toImpl(notification)->tag());
}

WKStringRef WKNotificationCopyLang(WKNotificationRef notification)
{
    return toCopiedAPI(toImpl(notification)->lang());
}

WKStringRef WKNotificationCopyDir(WKNotificationRef notification)
{
    return toCopiedAPI(toImpl(notification)->dir());
}

WKSecurityOriginRef WKNotificationGetSecurityOrigin(WKNotificationRef notification)
{
    return toAPI(toImpl(notification)->origin());
}

uint64_t WKNotificationGetID(WKNotificationRef notification)
{
    return toImpl(notification)->notificationID();
}
