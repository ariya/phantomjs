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
#include "WebKitGeolocationPermissionRequest.h"

#include "GeolocationPermissionRequestProxy.h"
#include "WebKitGeolocationPermissionRequestPrivate.h"
#include "WebKitPermissionRequest.h"

using namespace WebKit;

/**
 * SECTION: WebKitGeolocationPermissionRequest
 * @Short_description: A permission request for sharing user's location
 * @Title: WebKitGeolocationPermissionRequest
 * @See_also: #WebKitPermissionRequest, #WebKitWebView
 *
 * WebKitGeolocationPermissionRequest represents a request for
 * permission to decide whether WebKit should provide the user's
 * location to a website when requested throught the Geolocation API.
 */

static void webkit_permission_request_interface_init(WebKitPermissionRequestIface*);

struct _WebKitGeolocationPermissionRequestPrivate {
    RefPtr<GeolocationPermissionRequestProxy> request;
    bool madeDecision;
};

WEBKIT_DEFINE_TYPE_WITH_CODE(
    WebKitGeolocationPermissionRequest, webkit_geolocation_permission_request, G_TYPE_OBJECT,
    G_IMPLEMENT_INTERFACE(WEBKIT_TYPE_PERMISSION_REQUEST, webkit_permission_request_interface_init))

static void webkitGeolocationPermissionRequestAllow(WebKitPermissionRequest* request)
{
    ASSERT(WEBKIT_IS_GEOLOCATION_PERMISSION_REQUEST(request));

    WebKitGeolocationPermissionRequestPrivate* priv = WEBKIT_GEOLOCATION_PERMISSION_REQUEST(request)->priv;

    // Only one decision at a time.
    if (priv->madeDecision)
        return;

    priv->request->allow();
    priv->madeDecision = true;
}

static void webkitGeolocationPermissionRequestDeny(WebKitPermissionRequest* request)
{
    ASSERT(WEBKIT_IS_GEOLOCATION_PERMISSION_REQUEST(request));

    WebKitGeolocationPermissionRequestPrivate* priv = WEBKIT_GEOLOCATION_PERMISSION_REQUEST(request)->priv;

    // Only one decision at a time.
    if (priv->madeDecision)
        return;

    priv->request->deny();
    priv->madeDecision = true;
}

static void webkit_permission_request_interface_init(WebKitPermissionRequestIface* iface)
{
    iface->allow = webkitGeolocationPermissionRequestAllow;
    iface->deny = webkitGeolocationPermissionRequestDeny;
}

static void webkitGeolocationPermissionRequestDispose(GObject* object)
{
    // Default behaviour when no decision has been made is denying the request.
    webkitGeolocationPermissionRequestDeny(WEBKIT_PERMISSION_REQUEST(object));
    G_OBJECT_CLASS(webkit_geolocation_permission_request_parent_class)->dispose(object);
}

static void webkit_geolocation_permission_request_class_init(WebKitGeolocationPermissionRequestClass* klass)
{
    GObjectClass* objectClass = G_OBJECT_CLASS(klass);
    objectClass->dispose = webkitGeolocationPermissionRequestDispose;
}

WebKitGeolocationPermissionRequest* webkitGeolocationPermissionRequestCreate(GeolocationPermissionRequestProxy* request)
{
    WebKitGeolocationPermissionRequest* geolocationPermissionRequest = WEBKIT_GEOLOCATION_PERMISSION_REQUEST(g_object_new(WEBKIT_TYPE_GEOLOCATION_PERMISSION_REQUEST, NULL));
    geolocationPermissionRequest->priv->request = request;
    return geolocationPermissionRequest;
}
