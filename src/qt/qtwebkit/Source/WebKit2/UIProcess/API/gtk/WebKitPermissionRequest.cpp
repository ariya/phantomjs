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
#include "WebKitPermissionRequest.h"

/**
 * SECTION: WebKitPermissionRequest
 * @Short_description: A permission request
 * @Title: WebKitPermissionRequest
 * @See_also: #WebKitWebView
 *
 * There are situations where an embedder would need to ask the user
 * for permission to do certain types of operations, such as switching
 * to fullscreen mode or reporting the user's location through the
 * standard Geolocation API. In those cases, WebKit will emit a
 * #WebKitWebView::permission-request signal with a
 * #WebKitPermissionRequest object attached to it.
 */

typedef WebKitPermissionRequestIface WebKitPermissionRequestInterface;
G_DEFINE_INTERFACE(WebKitPermissionRequest, webkit_permission_request, G_TYPE_OBJECT)

static void webkit_permission_request_default_init(WebKitPermissionRequestIface*)
{
}

/**
 * webkit_permission_request_allow:
 * @request: a #WebKitPermissionRequest
 *
 * Allow the action which triggered this request.
 */
void webkit_permission_request_allow(WebKitPermissionRequest* request)
{
    g_return_if_fail(WEBKIT_IS_PERMISSION_REQUEST(request));

    WebKitPermissionRequestIface* iface = WEBKIT_PERMISSION_REQUEST_GET_IFACE(request);
    if (iface->allow)
        iface->allow(request);
}

/**
 * webkit_permission_request_deny:
 * @request: a #WebKitPermissionRequest
 *
 * Deny the action which triggered this request.
 */
void webkit_permission_request_deny(WebKitPermissionRequest* request)
{
    g_return_if_fail(WEBKIT_IS_PERMISSION_REQUEST(request));

    WebKitPermissionRequestIface* iface = WEBKIT_PERMISSION_REQUEST_GET_IFACE(request);
    if (iface->deny)
        iface->deny(request);
}
