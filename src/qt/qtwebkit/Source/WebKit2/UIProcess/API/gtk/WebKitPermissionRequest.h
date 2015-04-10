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

#if !defined(__WEBKIT2_H_INSIDE__) && !defined(WEBKIT2_COMPILATION)
#error "Only <webkit2/webkit2.h> can be included directly."
#endif

#ifndef WebKitPermissionRequest_h
#define WebKitPermissionRequest_h

#include <glib-object.h>
#include <webkit2/WebKitDefines.h>

G_BEGIN_DECLS

#define WEBKIT_TYPE_PERMISSION_REQUEST           (webkit_permission_request_get_type())
#define WEBKIT_PERMISSION_REQUEST(obj)           (G_TYPE_CHECK_INSTANCE_CAST((obj), WEBKIT_TYPE_PERMISSION_REQUEST, WebKitPermissionRequest))
#define WEBKIT_IS_PERMISSION_REQUEST(obj)        (G_TYPE_CHECK_INSTANCE_TYPE((obj), WEBKIT_TYPE_PERMISSION_REQUEST))
#define WEBKIT_PERMISSION_REQUEST_GET_IFACE(obj) (G_TYPE_INSTANCE_GET_INTERFACE((obj), WEBKIT_TYPE_PERMISSION_REQUEST, WebKitPermissionRequestIface))

typedef struct _WebKitPermissionRequest WebKitPermissionRequest;
typedef struct _WebKitPermissionRequestIface WebKitPermissionRequestIface;

struct _WebKitPermissionRequestIface {
    GTypeInterface parent_interface;

    void (* allow) (WebKitPermissionRequest *request);
    void (* deny)  (WebKitPermissionRequest *request);
};

WEBKIT_API GType
webkit_permission_request_get_type (void);

WEBKIT_API void
webkit_permission_request_allow    (WebKitPermissionRequest *request);

WEBKIT_API void
webkit_permission_request_deny     (WebKitPermissionRequest *request);

G_END_DECLS

#endif
