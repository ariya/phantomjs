/*
 * Copyright (C) 2008 Christian Dywan <christian@imendio.com>
 * Copyright (C) 2009 Jan Michael Alonzo <jmalonzo@gmail.com
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

#ifndef webkitwebsettings_h
#define webkitwebsettings_h

#include <glib-object.h>

#include <webkit/webkitdefines.h>

G_BEGIN_DECLS

#define WEBKIT_TYPE_WEB_SETTINGS            (webkit_web_settings_get_type())
#define WEBKIT_WEB_SETTINGS(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj), WEBKIT_TYPE_WEB_SETTINGS, WebKitWebSettings))
#define WEBKIT_WEB_SETTINGS_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass),  WEBKIT_TYPE_WEB_SETTINGS, WebKitWebSettingsClass))
#define WEBKIT_IS_WEB_SETTINGS(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj), WEBKIT_TYPE_WEB_SETTINGS))
#define WEBKIT_IS_WEB_SETTINGS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass),  WEBKIT_TYPE_WEB_SETTINGS))
#define WEBKIT_WEB_SETTINGS_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj),  WEBKIT_TYPE_WEB_SETTINGS, WebKitWebSettingsClass))

/**
 * WebKitEditingBehavior:
 * @WEBKIT_EDITING_BEHAVIOR_MAC: Editing behavior mimicking OS X user interfaces.
 * @WEBKIT_EDITING_BEHAVIOR_WINDOWS: Editing behavior mimicking Windows user interfaces.
 * @WEBKIT_EDITING_BEHAVIOR_UNIX: Editing behavior mimicking free desktop user interfaces.
 *
 * Enum values used for determining the editing behavior of editable elements.
 *
 **/
typedef enum {
    WEBKIT_EDITING_BEHAVIOR_MAC,
    WEBKIT_EDITING_BEHAVIOR_WINDOWS,
    WEBKIT_EDITING_BEHAVIOR_UNIX
} WebKitEditingBehavior;

typedef struct _WebKitWebSettingsPrivate WebKitWebSettingsPrivate;

struct _WebKitWebSettings {
    GObject parent_instance;

    /*< private >*/
    WebKitWebSettingsPrivate *priv;
};

struct _WebKitWebSettingsClass {
    GObjectClass parent_class;

    /* Padding for future expansion */
    void (*_webkit_reserved1) (void);
    void (*_webkit_reserved2) (void);
    void (*_webkit_reserved3) (void);
    void (*_webkit_reserved4) (void);
};

WEBKIT_API GType
webkit_web_settings_get_type          (void);

WEBKIT_API WebKitWebSettings *
webkit_web_settings_new               (void);

WEBKIT_API WebKitWebSettings *
webkit_web_settings_copy              (WebKitWebSettings *web_settings);

WEBKIT_API const gchar *
webkit_web_settings_get_user_agent    (WebKitWebSettings *web_settings);

G_END_DECLS

#endif /* webkitwebsettings_h */
