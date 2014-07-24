/*
 * Copyright (C) 2008 Gustavo Noronha Silva
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

#ifndef webkitwebinspector_h
#define webkitwebinspector_h

#include <glib-object.h>

#include <webkit/webkitdefines.h>
#include <webkitdom/webkitdom.h>

G_BEGIN_DECLS

#define WEBKIT_TYPE_WEB_INSPECTOR            (webkit_web_inspector_get_type())
#define WEBKIT_WEB_INSPECTOR(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj), WEBKIT_TYPE_WEB_INSPECTOR, WebKitWebInspector))
#define WEBKIT_WEB_INSPECTOR_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass),  WEBKIT_TYPE_WEB_INSPECTOR, WebKitWebInspectorClass))
#define WEBKIT_IS_WEB_INSPECTOR(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj), WEBKIT_TYPE_WEB_INSPECTOR))
#define WEBKIT_IS_WEB_INSPECTOR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass),  WEBKIT_TYPE_WEB_INSPECTOR))
#define WEBKIT_WEB_INSPECTOR_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj),  WEBKIT_TYPE_WEB_INSPECTOR, WebKitWebInspectorClass))

typedef struct _WebKitWebInspectorPrivate WebKitWebInspectorPrivate;

struct _WebKitWebInspector {
    GObject parent_instance;

    WebKitWebInspectorPrivate* priv;
};

struct _WebKitWebInspectorClass {
    GObjectClass parent_class;

    /* Padding for future expansion */
    void (*_webkit_reserved1) (void);
    void (*_webkit_reserved2) (void);
    void (*_webkit_reserved3) (void);
    void (*_webkit_reserved4) (void);
};

WEBKIT_API GType
webkit_web_inspector_get_type (void);

WEBKIT_API WebKitWebView*
webkit_web_inspector_get_web_view        (WebKitWebInspector *web_inspector);

WEBKIT_API const gchar*
webkit_web_inspector_get_inspected_uri   (WebKitWebInspector *web_inspector);

WEBKIT_API void
webkit_web_inspector_inspect_node        (WebKitWebInspector *web_inspector,
                                          WebKitDOMNode      *node);

WEBKIT_API void
webkit_web_inspector_inspect_coordinates (WebKitWebInspector *web_inspector,
                                          gdouble            x,
                                          gdouble            y);

WEBKIT_API void
webkit_web_inspector_show                (WebKitWebInspector *web_inspector);

WEBKIT_API void
webkit_web_inspector_close               (WebKitWebInspector *web_inspector);
G_END_DECLS

#endif /* webkitwebinspector_h */
