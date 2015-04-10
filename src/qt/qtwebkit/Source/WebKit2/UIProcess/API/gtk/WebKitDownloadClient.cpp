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
#include "WebKitDownloadClient.h"

#include "WebContext.h"
#include "WebKitDownloadPrivate.h"
#include "WebKitURIResponsePrivate.h"
#include "WebKitWebContextPrivate.h"
#include "WebURLResponse.h"
#include <WebKit2/WKString.h>
#include <wtf/gobject/GRefPtr.h>
#include <wtf/text/CString.h>

using namespace WebCore;
using namespace WebKit;

static void didStart(WKContextRef, WKDownloadRef wkDownload, const void* clientInfo)
{
    GRefPtr<WebKitDownload> download = webkitWebContextGetOrCreateDownload(toImpl(wkDownload));
    webkitWebContextDownloadStarted(WEBKIT_WEB_CONTEXT(clientInfo), download.get());
}

static void didReceiveResponse(WKContextRef, WKDownloadRef wkDownload, WKURLResponseRef wkResponse, const void* clientInfo)
{
    GRefPtr<WebKitDownload> download = webkitWebContextGetOrCreateDownload(toImpl(wkDownload));
    if (webkitDownloadIsCancelled(download.get()))
        return;

    GRefPtr<WebKitURIResponse> response = adoptGRef(webkitURIResponseCreateForResourceResponse(toImpl(wkResponse)->resourceResponse()));
    webkitDownloadSetResponse(download.get(), response.get());
}

static void didReceiveData(WKContextRef, WKDownloadRef wkDownload, uint64_t length, const void* clientInfo)
{
    GRefPtr<WebKitDownload> download = webkitWebContextGetOrCreateDownload(toImpl(wkDownload));
    webkitDownloadNotifyProgress(download.get(), length);
}

static WKStringRef decideDestinationWithSuggestedFilename(WKContextRef, WKDownloadRef wkDownload, WKStringRef filename, bool* allowOverwrite, const void* clientInfo)
{
    GRefPtr<WebKitDownload> download = webkitWebContextGetOrCreateDownload(toImpl(wkDownload));
    CString destinationURI = webkitDownloadDecideDestinationWithSuggestedFilename(download.get(),
                                                                                  toImpl(filename)->string().utf8());
    return WKStringCreateWithUTF8CString(destinationURI.data());
}

static void didCreateDestination(WKContextRef, WKDownloadRef wkDownload, WKStringRef path, const void* clientInfo)
{
    GRefPtr<WebKitDownload> download = webkitWebContextGetOrCreateDownload(toImpl(wkDownload));
    webkitDownloadDestinationCreated(download.get(), toImpl(path)->string().utf8());
}

static void didFail(WKContextRef, WKDownloadRef wkDownload, WKErrorRef error, const void *clientInfo)
{
    GRefPtr<WebKitDownload> download = webkitWebContextGetOrCreateDownload(toImpl(wkDownload));
    if (webkitDownloadIsCancelled(download.get())) {
        // Cancellation takes precedence over other errors.
        webkitDownloadCancelled(download.get());
    } else
        webkitDownloadFailed(download.get(), toImpl(error)->platformError());
    webkitWebContextRemoveDownload(toImpl(wkDownload));
}

static void didCancel(WKContextRef, WKDownloadRef wkDownload, const void *clientInfo)
{
    GRefPtr<WebKitDownload> download = webkitWebContextGetOrCreateDownload(toImpl(wkDownload));
    webkitDownloadCancelled(download.get());
    webkitWebContextRemoveDownload(toImpl(wkDownload));
}

static void didFinish(WKContextRef wkContext, WKDownloadRef wkDownload, const void *clientInfo)
{
    GRefPtr<WebKitDownload> download = webkitWebContextGetOrCreateDownload(toImpl(wkDownload));
    webkitDownloadFinished(download.get());
    webkitWebContextRemoveDownload(toImpl(wkDownload));
}

void attachDownloadClientToContext(WebKitWebContext* webContext)
{
    WKContextDownloadClient wkDownloadClient = {
        kWKContextDownloadClientCurrentVersion,
        webContext, // ClientInfo
        didStart,
        0, // didReceiveAuthenticationChallenge
        didReceiveResponse,
        didReceiveData,
        0, // shouldDecodeSourceDataOfMIMEType
        decideDestinationWithSuggestedFilename,
        didCreateDestination,
        didFinish,
        didFail,
        didCancel,
        0, // processDidCrash
    };
    WKContextSetDownloadClient(toAPI(webkitWebContextGetContext(webContext)), &wkDownloadClient);
}

