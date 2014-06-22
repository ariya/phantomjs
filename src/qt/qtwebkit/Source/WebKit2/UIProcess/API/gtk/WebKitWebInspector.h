/*
 * Copyright (C) 2012 Igalia S.L.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2,1 of the License, or (at your option) any later version.
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

#ifndef WebKitWebInspector_h
#define WebKitWebInspector_h

#include <glib-object.h>
#include <webkit2/WebKitDefines.h>
#include <webkit2/WebKitWebViewBase.h>

G_BEGIN_DECLS

#define WEBKIT_TYPE_WEB_INSPECTOR            (webkit_web_inspector_get_type())
#define WEBKIT_WEB_INSPECTOR(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj), WEBKIT_TYPE_WEB_INSPECTOR, WebKitWebInspector))
#define WEBKIT_IS_WEB_INSPECTOR(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj), WEBKIT_TYPE_WEB_INSPECTOR))
#define WEBKIT_WEB_INSPECTOR_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass),  WEBKIT_TYPE_WEB_INSPECTOR, WebKitWebInspectorClass))
#define WEBKIT_IS_WEB_INSPECTOR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass),  WEBKIT_TYPE_WEB_INSPECTOR))
#define WEBKIT_WEB_INSPECTOR_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj),  WEBKIT_TYPE_WEB_INSPECTOR, WebKitWebInspectorClass))

typedef struct _WebKitWebInspector        WebKitWebInspector;
typedef struct _WebKitWebInspectorClass   WebKitWebInspectorClass;
typedef struct _WebKitWebInspectorPrivate WebKitWebInspectorPrivate;

struct _WebKitWebInspector {
    GObject parent;

    WebKitWebInspectorPrivate *priv;
};

struct _WebKitWebInspectorClass {
    GObjectClass parent_class;

    void (*_webkit_reserved0) (void);
    void (*_webkit_reserved1) (void);
    void (*_webkit_reserved2) (void);
    void (*_webkit_reserved3) (void);
};

WEBKIT_API GType
webkit_web_inspector_get_type            (void);

WEBKIT_API WebKitWebViewBase *
webkit_web_inspector_get_web_view        (WebKitWebInspector *inspector);

WEBKIT_API const char *
webkit_web_inspector_get_inspected_uri   (WebKitWebInspector *inspector);

WEBKIT_API gboolean
webkit_web_inspector_is_attached         (WebKitWebInspector *inspector);

WEBKIT_API void
webkit_web_inspector_attach              (WebKitWebInspector *inspector);

WEBKIT_API void
webkit_web_inspector_detach              (WebKitWebInspector *inspector);

WEBKIT_API void
webkit_web_inspector_show                (WebKitWebInspector *inspector);

WEBKIT_API void
webkit_web_inspector_close               (WebKitWebInspector *inspector);

WEBKIT_API guint
webkit_web_inspector_get_attached_height (WebKitWebInspector *inspector);

G_END_DECLS

#endif
