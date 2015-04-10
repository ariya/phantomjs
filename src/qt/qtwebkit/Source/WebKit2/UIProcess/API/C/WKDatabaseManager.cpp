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
#include "WKDatabaseManager.h"

#include "WebDatabaseManagerProxy.h"
#include "WKAPICast.h"

#ifdef __BLOCKS__
#include <Block.h>
#endif

using namespace WebKit;

WKTypeID WKDatabaseManagerGetTypeID()
{
#if ENABLE(SQL_DATABASE)
    return toAPI(WebDatabaseManagerProxy::APIType);
#else
    return 0;
#endif
}

WKStringRef WKDatabaseManagerGetOriginKey()
{
#if ENABLE(SQL_DATABASE)
    static WebString* key = WebString::create(WebDatabaseManagerProxy::originKey()).leakRef();
    return toAPI(key);
#else
    return 0;
#endif
}

WKStringRef WKDatabaseManagerGetOriginQuotaKey()
{
#if ENABLE(SQL_DATABASE)
    static WebString* key = WebString::create(WebDatabaseManagerProxy::originQuotaKey()).leakRef();
    return toAPI(key);
#else
    return 0;
#endif
}

WKStringRef WKDatabaseManagerGetOriginUsageKey()
{
#if ENABLE(SQL_DATABASE)
    static WebString* key = WebString::create(WebDatabaseManagerProxy::originUsageKey()).leakRef();
    return toAPI(key);
#else
    return 0;
#endif
}

WKStringRef WKDatabaseManagerGetDatabaseDetailsKey()
{
#if ENABLE(SQL_DATABASE)
    static WebString* key = WebString::create(WebDatabaseManagerProxy::databaseDetailsKey()).leakRef();
    return toAPI(key);
#else
    return 0;
#endif
}

WKStringRef WKDatabaseManagerGetDatabaseDetailsNameKey()
{
#if ENABLE(SQL_DATABASE)
    static WebString* key = WebString::create(WebDatabaseManagerProxy::databaseDetailsNameKey()).leakRef();
    return toAPI(key);
#else
    return 0;
#endif
}

WKStringRef WKDatabaseManagerGetDatabaseDetailsDisplayNameKey()
{
#if ENABLE(SQL_DATABASE)
    static WebString* key = WebString::create(WebDatabaseManagerProxy::databaseDetailsDisplayNameKey()).leakRef();
    return toAPI(key);
#else
    return 0;
#endif
}

WKStringRef WKDatabaseManagerGetDatabaseDetailsExpectedUsageKey()
{
#if ENABLE(SQL_DATABASE)
    static WebString* key = WebString::create(WebDatabaseManagerProxy::databaseDetailsExpectedUsageKey()).leakRef();
    return toAPI(key);
#else
    return 0;
#endif
}

WKStringRef WKDatabaseManagerGetDatabaseDetailsCurrentUsageKey()
{
#if ENABLE(SQL_DATABASE)
    static WebString* key = WebString::create(WebDatabaseManagerProxy::databaseDetailsCurrentUsageKey()).leakRef();
    return toAPI(key);
#else
    return 0;
#endif
}

void WKDatabaseManagerSetClient(WKDatabaseManagerRef databaseManagerRef, const WKDatabaseManagerClient* wkClient)
{
#if ENABLE(SQL_DATABASE)
    if (wkClient && wkClient->version)
        return;
    toImpl(databaseManagerRef)->initializeClient(wkClient);
#endif
}

void WKDatabaseManagerGetDatabasesByOrigin(WKDatabaseManagerRef databaseManagerRef, void* context, WKDatabaseManagerGetDatabasesByOriginFunction callback)
{
#if ENABLE(SQL_DATABASE)
    toImpl(databaseManagerRef)->getDatabasesByOrigin(ArrayCallback::create(context, callback));
#endif
}

#ifdef __BLOCKS__
static void callGetDatabasesByOriginBlockAndDispose(WKArrayRef resultValue, WKErrorRef errorRef, void* context)
{
#if ENABLE(SQL_DATABASE)
    WKDatabaseManagerGetDatabasesByOriginBlock block = (WKDatabaseManagerGetDatabasesByOriginBlock)context;
    block(resultValue, errorRef);
    Block_release(block);
#endif
}

void WKDatabaseManagerGetDatabasesByOrigin_b(WKDatabaseManagerRef databaseManagerRef, WKDatabaseManagerGetDatabasesByOriginBlock block)
{
#if ENABLE(SQL_DATABASE)
    WKDatabaseManagerGetDatabasesByOrigin(databaseManagerRef, Block_copy(block), callGetDatabasesByOriginBlockAndDispose);
#endif
}
#endif // __BLOCKS__

void WKDatabaseManagerGetDatabaseOrigins(WKDatabaseManagerRef databaseManagerRef, void* context, WKDatabaseManagerGetDatabaseOriginsFunction callback)
{
#if ENABLE(SQL_DATABASE)
    toImpl(databaseManagerRef)->getDatabaseOrigins(ArrayCallback::create(context, callback));
#endif
}

#ifdef __BLOCKS__
static void callGetDatabaseOriginsBlockBlockAndDispose(WKArrayRef resultValue, WKErrorRef errorRef, void* context)
{
#if ENABLE(SQL_DATABASE)
    WKDatabaseManagerGetDatabaseOriginsBlock block = (WKDatabaseManagerGetDatabaseOriginsBlock)context;
    block(resultValue, errorRef);
    Block_release(block);
#endif
}

void WKDatabaseManagerGetDatabaseOrigins_b(WKDatabaseManagerRef databaseManagerRef, WKDatabaseManagerGetDatabaseOriginsBlock block)
{
#if ENABLE(SQL_DATABASE)
    WKDatabaseManagerGetDatabaseOrigins(databaseManagerRef, Block_copy(block), callGetDatabaseOriginsBlockBlockAndDispose);
#endif
}
#endif // __BLOCKS__

void WKDatabaseManagerDeleteDatabasesWithNameForOrigin(WKDatabaseManagerRef databaseManagerRef, WKStringRef databaseNameRef, WKSecurityOriginRef originRef)
{
#if ENABLE(SQL_DATABASE)
    toImpl(databaseManagerRef)->deleteDatabaseWithNameForOrigin(toWTFString(databaseNameRef), toImpl(originRef));
#endif
}

void WKDatabaseManagerDeleteDatabasesForOrigin(WKDatabaseManagerRef databaseManagerRef, WKSecurityOriginRef originRef)
{
#if ENABLE(SQL_DATABASE)
    toImpl(databaseManagerRef)->deleteDatabasesForOrigin(toImpl(originRef));
#endif
}

void WKDatabaseManagerDeleteAllDatabases(WKDatabaseManagerRef databaseManagerRef)
{
#if ENABLE(SQL_DATABASE)
    toImpl(databaseManagerRef)->deleteAllDatabases();
#endif
}

void WKDatabaseManagerSetQuotaForOrigin(WKDatabaseManagerRef databaseManagerRef, WKSecurityOriginRef originRef, uint64_t quota)
{
#if ENABLE(SQL_DATABASE)
    toImpl(databaseManagerRef)->setQuotaForOrigin(toImpl(originRef), quota);
#endif
}
