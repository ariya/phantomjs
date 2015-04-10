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

#ifndef WKDatabaseManager_h
#define WKDatabaseManager_h

#include <WebKit2/WKBase.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Value type: WKSecurityOriginRef */
WK_EXPORT WKStringRef WKDatabaseManagerGetOriginKey();

/* Value type: WKUInt64Ref */
WK_EXPORT WKStringRef WKDatabaseManagerGetOriginQuotaKey();

/* Value type: WKUInt64Ref */
WK_EXPORT WKStringRef WKDatabaseManagerGetOriginUsageKey();

/* Value type: WKArrayRef (array of WKDictionaryRef's with keys that include:
       - WKDatabaseManagerGetDatabaseNameKey()
       - WKDatabaseManagerGetDatabaseDisplayNameKey()
       - WKDatabaseManagerGetDatabaseExpectedUsageKey()
       - WKDatabaseManagerGetDatabaseCurrentUsageKey()
 */
WK_EXPORT WKStringRef WKDatabaseManagerGetDatabaseDetailsKey();

/* Value type: WKStringRef */
WK_EXPORT WKStringRef WKDatabaseManagerGetDatabaseDetailsNameKey();

/* Value type: WKStringRef */
WK_EXPORT WKStringRef WKDatabaseManagerGetDatabaseDetailsDisplayNameKey();

/* Value type: WKUInt64Ref */
WK_EXPORT WKStringRef WKDatabaseManagerGetDatabaseDetailsExpectedUsageKey();

/* Value type: WKUInt64Ref */
WK_EXPORT WKStringRef WKDatabaseManagerGetDatabaseDetailsCurrentUsageKey();


// Database Manager Client
typedef void (*WKDatabaseManagerDidModifyOriginCallback)(WKDatabaseManagerRef databaseManager, WKSecurityOriginRef origin, const void *clientInfo);
typedef void (*WKDatabaseManagerDidModifyDatabaseCallback)(WKDatabaseManagerRef databaseManager, WKSecurityOriginRef origin, WKStringRef databaseIdentifier, const void *clientInfo);

struct WKDatabaseManagerClient {
    int                                                                 version;
    const void *                                                        clientInfo;
    WKDatabaseManagerDidModifyOriginCallback                            didModifyOrigin;
    WKDatabaseManagerDidModifyDatabaseCallback                          didModifyDatabase;
};
typedef struct WKDatabaseManagerClient WKDatabaseManagerClient;

enum { kWKDatabaseManagerClientCurrentVersion = 0 };


WK_EXPORT WKTypeID WKDatabaseManagerGetTypeID();

WK_EXPORT void WKDatabaseManagerSetClient(WKDatabaseManagerRef databaseManager, const WKDatabaseManagerClient* client);

typedef void (*WKDatabaseManagerGetDatabasesByOriginFunction)(WKArrayRef, WKErrorRef, void*);
WK_EXPORT void WKDatabaseManagerGetDatabasesByOrigin(WKDatabaseManagerRef databaseManager, void* context, WKDatabaseManagerGetDatabasesByOriginFunction function);
#ifdef __BLOCKS__
typedef void (^WKDatabaseManagerGetDatabasesByOriginBlock)(WKArrayRef, WKErrorRef);
WK_EXPORT void WKDatabaseManagerGetDatabasesByOrigin_b(WKDatabaseManagerRef databaseManager, WKDatabaseManagerGetDatabasesByOriginBlock block);
#endif

typedef void (*WKDatabaseManagerGetDatabaseOriginsFunction)(WKArrayRef, WKErrorRef, void*);
WK_EXPORT void WKDatabaseManagerGetDatabaseOrigins(WKDatabaseManagerRef contextRef, void* context, WKDatabaseManagerGetDatabaseOriginsFunction function);
#ifdef __BLOCKS__
typedef void (^WKDatabaseManagerGetDatabaseOriginsBlock)(WKArrayRef, WKErrorRef);
WK_EXPORT void WKDatabaseManagerGetDatabaseOrigins_b(WKDatabaseManagerRef databaseManager, WKDatabaseManagerGetDatabaseOriginsBlock block);
#endif

WK_EXPORT void WKDatabaseManagerDeleteDatabasesWithNameForOrigin(WKDatabaseManagerRef databaseManager, WKStringRef databaseName, WKSecurityOriginRef origin);
WK_EXPORT void WKDatabaseManagerDeleteDatabasesForOrigin(WKDatabaseManagerRef databaseManager, WKSecurityOriginRef origin);
WK_EXPORT void WKDatabaseManagerDeleteAllDatabases(WKDatabaseManagerRef databaseManager);

WK_EXPORT void WKDatabaseManagerSetQuotaForOrigin(WKDatabaseManagerRef databaseManager, WKSecurityOriginRef origin, uint64_t quota);

#ifdef __cplusplus
}
#endif

#endif // WKDatabaseManager_h
