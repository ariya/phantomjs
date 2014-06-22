/*
 * Copyright (C) 2012 Igalia S.L.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include "config.h"
#include "WebKitRequestManagerClient.h"

#include "WebKitURISchemeRequestPrivate.h"
#include "WebKitWebContextPrivate.h"
#include <wtf/gobject/GRefPtr.h>

using namespace WebKit;

static void didReceiveURIRequest(WKSoupRequestManagerRef soupRequestManagerRef, WKURLRef urlRef, WKPageRef initiatingPageRef, uint64_t requestID, const void* clientInfo)
{
    WebKitWebContext* webContext = WEBKIT_WEB_CONTEXT(clientInfo);
    GRefPtr<WebKitURISchemeRequest> request = adoptGRef(webkitURISchemeRequestCreate(webContext, toImpl(soupRequestManagerRef), toImpl(urlRef), toImpl(initiatingPageRef), requestID));
    webkitWebContextReceivedURIRequest(webContext, request.get());
}

static void didFailToLoadURIRequest(WKSoupRequestManagerRef, uint64_t requestID, const void* clientInfo)
{
    webkitWebContextDidFailToLoadURIRequest(WEBKIT_WEB_CONTEXT(clientInfo), requestID);
}

void attachRequestManagerClientToContext(WebKitWebContext* webContext)
{
    WKSoupRequestManagerClient wkRequestManagerClient = {
        kWKSoupRequestManagerClientCurrentVersion,
        webContext, // clientInfo
        didReceiveURIRequest,
        didFailToLoadURIRequest
    };
    WKSoupRequestManagerSetClient(toAPI(webkitWebContextGetRequestManager(webContext)), &wkRequestManagerClient);
}

