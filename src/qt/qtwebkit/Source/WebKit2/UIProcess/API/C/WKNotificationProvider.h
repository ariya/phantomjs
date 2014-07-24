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

#ifndef WKNotificationProvider_h
#define WKNotificationProvider_h

#include <WebKit2/WKBase.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*WKNotificationProviderShowCallback)(WKPageRef page, WKNotificationRef notification, const void* clientInfo);
typedef void (*WKNotificationProviderCancelCallback)(WKNotificationRef notification, const void* clientInfo);
typedef void (*WKNotificationProviderDidDestroyNotificationCallback)(WKNotificationRef notification, const void* clientInfo);
typedef void (*WKNotificationProviderAddNotificationManagerCallback)(WKNotificationManagerRef manager, const void* clientInfo);
typedef void (*WKNotificationProviderRemoveNotificationManagerCallback)(WKNotificationManagerRef manager, const void* clientInfo);
typedef WKDictionaryRef (*WKNotificationProviderNotificationPermissionsCallback)(const void* clientInfo);
typedef void (*WKNotificationProviderClearNotificationsCallback)(WKArrayRef notificationIDs, const void* clientInfo);

struct WKNotificationProvider {
    int                                                                   version;
    const void*                                                           clientInfo;
    WKNotificationProviderShowCallback                                    show;
    WKNotificationProviderCancelCallback                                  cancel;
    WKNotificationProviderDidDestroyNotificationCallback                  didDestroyNotification;
    WKNotificationProviderAddNotificationManagerCallback                  addNotificationManager;
    WKNotificationProviderRemoveNotificationManagerCallback               removeNotificationManager;
    WKNotificationProviderNotificationPermissionsCallback                 notificationPermissions;
    WKNotificationProviderClearNotificationsCallback                      clearNotifications;
};
typedef struct WKNotificationProvider WKNotificationProvider;

enum { kWKNotificationProviderCurrentVersion = 0 };

#ifdef __cplusplus
}
#endif


#endif // WKNotificationProvider_h
