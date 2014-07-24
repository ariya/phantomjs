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
#include "WebKitFormClient.h"

#include "WebKitFormSubmissionRequestPrivate.h"
#include "WebKitPrivate.h"
#include "WebKitWebViewBasePrivate.h"
#include "WebKitWebViewPrivate.h"
#include <wtf/gobject/GRefPtr.h>

using namespace WebKit;

static void willSubmitForm(WKPageRef page, WKFrameRef frame, WKFrameRef sourceFrame, WKDictionaryRef values, WKTypeRef userData, WKFormSubmissionListenerRef listener, const void* clientInfo)
{
    GRefPtr<WebKitFormSubmissionRequest> request = adoptGRef(webkitFormSubmissionRequestCreate(toImpl(values), toImpl(listener)));
    webkitWebViewSubmitFormRequest(WEBKIT_WEB_VIEW(clientInfo), request.get());
}

void attachFormClientToView(WebKitWebView* webView)
{
    WKPageFormClient wkFormClient = {
        kWKPageFormClientCurrentVersion,
        webView, // clientInfo
        willSubmitForm
    };
    WKPageRef wkPage = toAPI(webkitWebViewBaseGetPage(WEBKIT_WEB_VIEW_BASE(webView)));
    WKPageSetPageFormClient(wkPage, &wkFormClient);
}
