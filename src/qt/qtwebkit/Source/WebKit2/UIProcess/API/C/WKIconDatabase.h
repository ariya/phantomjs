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

#ifndef WKIconDatabase_h
#define WKIconDatabase_h

#include <WebKit2/WKBase.h>

#ifdef __cplusplus
extern "C" {
#endif

// IconDatabase Client.
typedef void (*WKIconDatabaseDidChangeIconForPageURLCallback)(WKIconDatabaseRef iconDatabase, WKURLRef pageURL, const void* clientInfo);
typedef void (*WKIconDatabaseDidRemoveAllIconsCallback)(WKIconDatabaseRef iconDatabase, const void* clientInfo);
typedef void (*WKIconDatabaseIconDataReadyForPageURLCallback)(WKIconDatabaseRef iconDatabase, WKURLRef pageURL, const void* clientInfo);

struct WKIconDatabaseClient {
    int                                                                 version;
    const void *                                                        clientInfo;

    // Version 0
    WKIconDatabaseDidChangeIconForPageURLCallback                       didChangeIconForPageURL;
    WKIconDatabaseDidRemoveAllIconsCallback                             didRemoveAllIcons;

    // Version 1
    WKIconDatabaseIconDataReadyForPageURLCallback                       iconDataReadyForPageURL;
};
typedef struct WKIconDatabaseClient WKIconDatabaseClient;

enum { kWKIconDatabaseClientCurrentVersion = 1 };

WK_EXPORT WKTypeID WKIconDatabaseGetTypeID();

WK_EXPORT void WKIconDatabaseSetIconDatabaseClient(WKIconDatabaseRef iconDatabase, const WKIconDatabaseClient* client);

WK_EXPORT void WKIconDatabaseRetainIconForURL(WKIconDatabaseRef iconDatabase, WKURLRef pageURL);
WK_EXPORT void WKIconDatabaseReleaseIconForURL(WKIconDatabaseRef iconDatabase, WKURLRef pageURL);
WK_EXPORT void WKIconDatabaseEnableDatabaseCleanup(WKIconDatabaseRef iconDatabase);

WK_EXPORT void WKIconDatabaseRemoveAllIcons(WKIconDatabaseRef iconDatabase);
WK_EXPORT void WKIconDatabaseCheckIntegrityBeforeOpening(WKIconDatabaseRef iconDatabase);

WK_EXPORT void WKIconDatabaseClose(WKIconDatabaseRef iconDatabase);

#ifdef __cplusplus
}
#endif

#endif /* WKIconDatabase_h */
