/*
 * Copyright (C) 2013 Igalia S.L.
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

#ifndef WebKitWebViewGroup_h
#define WebKitWebViewGroup_h

#include <glib-object.h>
#include <webkit2/WebKitDefines.h>
#include <webkit2/WebKitSettings.h>

G_BEGIN_DECLS

#define WEBKIT_TYPE_WEB_VIEW_GROUP            (webkit_web_view_group_get_type())
#define WEBKIT_WEB_VIEW_GROUP(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj), WEBKIT_TYPE_WEB_VIEW_GROUP, WebKitWebViewGroup))
#define WEBKIT_IS_WEB_VIEW_GROUP(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj), WEBKIT_TYPE_WEB_VIEW_GROUP))
#define WEBKIT_WEB_VIEW_GROUP_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass),  WEBKIT_TYPE_WEB_VIEW_GROUP, WebKitWebViewGroupClass))
#define WEBKIT_IS_WEB_VIEW_GROUP_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass),  WEBKIT_TYPE_WEB_VIEW_GROUP))
#define WEBKIT_WEB_VIEW_GROUP_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj),  WEBKIT_TYPE_WEB_VIEW_GROUP, WebKitWebViewGroupClass))

typedef struct _WebKitWebViewGroup        WebKitWebViewGroup;
typedef struct _WebKitWebViewGroupClass   WebKitWebViewGroupClass;
typedef struct _WebKitWebViewGroupPrivate WebKitWebViewGroupPrivate;

struct _WebKitWebViewGroup {
    GObject parent;

    WebKitWebViewGroupPrivate *priv;
};

struct _WebKitWebViewGroupClass {
    GObjectClass parent_class;

    void (*_webkit_reserved0) (void);
    void (*_webkit_reserved1) (void);
    void (*_webkit_reserved2) (void);
    void (*_webkit_reserved3) (void);
};

/**
 * WebKitInjectedContentFrames:
 * @WEBKIT_INJECTED_CONTENT_FRAMES_ALL: Content will be injected into all frames.
 * @WEBKIT_INJECTED_CONTENT_FRAMES_TOP_ONLY: Content will only be injected into the main frame.
 *
 * Enum values used for determining into which frames content is injected.
 */
typedef enum {
    WEBKIT_INJECTED_CONTENT_FRAMES_ALL,
    WEBKIT_INJECTED_CONTENT_FRAMES_TOP_ONLY,
} WebKitInjectedContentFrames;

WEBKIT_API GType
webkit_web_view_group_get_type                     (void);

WEBKIT_API WebKitWebViewGroup *
webkit_web_view_group_new                          (const gchar                 *name);

WEBKIT_API const gchar *
webkit_web_view_group_get_name                     (WebKitWebViewGroup          *group);

WEBKIT_API WebKitSettings *
webkit_web_view_group_get_settings                 (WebKitWebViewGroup          *group);

WEBKIT_API void
webkit_web_view_group_set_settings                 (WebKitWebViewGroup          *group,
                                                    WebKitSettings              *settings);

WEBKIT_API void
webkit_web_view_group_add_user_style_sheet         (WebKitWebViewGroup           *group,
                                                    const gchar                  *source,
                                                    const gchar                  *base_uri,
                                                    const gchar * const          *whitelist,
                                                    const gchar * const          *blacklist,
                                                    WebKitInjectedContentFrames   injected_frames);

WEBKIT_API void
webkit_web_view_group_remove_all_user_style_sheets (WebKitWebViewGroup          *group);

G_END_DECLS

#endif
