/*
 * Copyright (C) 2008 Gustavo Noronha Silva <gns@gnome.org>
 * Copyright (C) 2008 Holger Hans Peter Freyther
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
#include "webkitwebwindowfeatures.h"

#include "WindowFeatures.h"
#include "webkitglobalsprivate.h"
#include "webkitwebwindowfeaturesprivate.h"

/**
 * SECTION:webkitwebwindowfeatures
 * @short_description: Window properties of a #WebKitWebView
 * @see_also: #WebKitWebView::web-view-ready
 *
 * The content of a #WebKitWebView can request to change certain
 * properties of a #WebKitWebView. This can include the x, y position
 * of the window, the width and height but also if a toolbar,
 * scrollbar, statusbar, locationbar should be visible to the user,
 * the request to show the #WebKitWebView fullscreen.
 *
 * In the normal case one will use #webkit_web_view_get_window_features
 * to get the #WebKitWebWindowFeatures and then monitor the property
 * changes. Be aware that the #WebKitWebWindowFeatures might change
 * before #WebKitWebView::web-view-ready signal is emitted.
 * To be safe listen to the notify::window-features signal of the #WebKitWebView
 * and reconnect the signals whenever the #WebKitWebWindowFeatures of
 * a #WebKitWebView changes.
 *
 * <informalexample><programlisting>
 * /<!-- -->* Get the current WebKitWebWindowFeatures *<!-- -->/
 * WebKitWebWindowFeatures *features = webkit_web_view_get_window_features (my_webview);
 *
 * /<!-- -->* Connect to the property changes *<!-- -->/
 * g_signal_connect (G_OBJECT(features), "notify::menubar-visible", G_CALLBACK(make_menu_bar_visible), NULL);
 * g_signal_connect (G_OBJECT(features), "notify::statusbar-visible", G_CALLBACK(make_status_bar_visible), NULL);
 *
 * </programlisting></informalexample>
 */

enum {
    PROP_0,

    PROP_X,
    PROP_Y,
    PROP_WIDTH,
    PROP_HEIGHT,
    PROP_TOOLBAR_VISIBLE,
    PROP_STATUSBAR_VISIBLE,
    PROP_SCROLLBAR_VISIBLE,
    PROP_MENUBAR_VISIBLE,
    PROP_LOCATIONBAR_VISIBLE,
    PROP_FULLSCREEN,
};

G_DEFINE_TYPE(WebKitWebWindowFeatures, webkit_web_window_features, G_TYPE_OBJECT)

struct _WebKitWebWindowFeaturesPrivate {
    gint x;
    gint y;
    gint width;
    gint height;

    gboolean toolbar_visible;
    gboolean statusbar_visible;
    gboolean scrollbar_visible;
    gboolean menubar_visible;
    gboolean locationbar_visible;

    gboolean fullscreen;
};

#define WEBKIT_WEB_WINDOW_FEATURES_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE((obj), WEBKIT_TYPE_WEB_WINDOW_FEATURES, WebKitWebWindowFeaturesPrivate))

static void webkit_web_window_features_set_property(GObject* object, guint prop_id, const GValue* value, GParamSpec* pspec);

static void webkit_web_window_features_get_property(GObject* object, guint prop_id, GValue* value, GParamSpec* pspec);

static void webkit_web_window_features_class_init(WebKitWebWindowFeaturesClass* klass)
{
    GObjectClass* gobject_class = G_OBJECT_CLASS(klass);
    gobject_class->set_property = webkit_web_window_features_set_property;
    gobject_class->get_property = webkit_web_window_features_get_property;

    GParamFlags flags = (GParamFlags)(WEBKIT_PARAM_READWRITE | G_PARAM_CONSTRUCT);

    webkitInit();

    /**
     * WebKitWebWindowFeatures:x:
     *
     * The starting x position of the window on the screen.
     *
     * Since: 1.0.3
     */
    g_object_class_install_property(gobject_class,
                                    PROP_X,
                                    g_param_spec_int(
                                    "x",
                                    "x",
                                    "The starting x position of the window on the screen.",
                                    -1,
                                    G_MAXINT,
                                    -1,
                                    flags));

    /**
     * WebKitWebWindowFeatures:y:
     *
     * The starting y position of the window on the screen.
     *
     * Since: 1.0.3
     */
    g_object_class_install_property(gobject_class,
                                    PROP_Y,
                                    g_param_spec_int(
                                    "y",
                                    "y",
                                    "The starting y position of the window on the screen.",
                                    -1,
                                    G_MAXINT,
                                    -1,
                                    flags));

    /**
     * WebKitWebWindowFeatures:width:
     *
     * The width of the window on the screen.
     *
     * Since: 1.0.3
     */
    g_object_class_install_property(gobject_class,
                                    PROP_WIDTH,
                                    g_param_spec_int(
                                    "width",
                                    "Width",
                                    "The width of the window on the screen.",
                                    -1,
                                    G_MAXINT,
                                    -1,
                                    flags));

    /**
     * WebKitWebWindowFeatures:height:
     *
     * The height of the window on the screen.
     *
     * Since: 1.0.3
     */
    g_object_class_install_property(gobject_class,
                                    PROP_HEIGHT,
                                    g_param_spec_int(
                                    "height",
                                    "Height",
                                    "The height of the window on the screen.",
                                    -1,
                                    G_MAXINT,
                                    -1,
                                    flags));

    /**
     * WebKitWebWindowFeatures:toolbar-visible:
     *
     * Controls whether the toolbar should be visible for the window.
     *
     * Since: 1.0.3
     */
    g_object_class_install_property(gobject_class,
                                    PROP_TOOLBAR_VISIBLE,
                                    g_param_spec_boolean(
                                    "toolbar-visible",
                                    "Toolbar Visible",
                                    "Controls whether the toolbar should be visible for the window.",
                                    TRUE,
                                    flags));

    /**
     * WebKitWebWindowFeatures:statusbar-visible:
     *
     * Controls whether the statusbar should be visible for the window.
     *
     * Since: 1.0.3
     */
    g_object_class_install_property(gobject_class,
                                    PROP_STATUSBAR_VISIBLE,
                                    g_param_spec_boolean(
                                    "statusbar-visible",
                                    "Statusbar Visible",
                                    "Controls whether the statusbar should be visible for the window.",
                                    TRUE,
                                    flags));

    /**
     * WebKitWebWindowFeatures:scrollbar-visible:
     *
     * Controls whether the scrollbars should be visible for the window.
     *
     * Since: 1.0.3
     */
    g_object_class_install_property(gobject_class,
                                    PROP_SCROLLBAR_VISIBLE,
                                    g_param_spec_boolean(
                                    "scrollbar-visible",
                                    "Scrollbar Visible",
                                    "Controls whether the scrollbars should be visible for the window.",
                                    TRUE,
                                    flags));

    /**
     * WebKitWebWindowFeatures:menubar-visible:
     *
     * Controls whether the menubar should be visible for the window.
     *
     * Since: 1.0.3
     */
    g_object_class_install_property(gobject_class,
                                    PROP_MENUBAR_VISIBLE,
                                    g_param_spec_boolean(
                                    "menubar-visible",
                                    "Menubar Visible",
                                    "Controls whether the menubar should be visible for the window.",
                                    TRUE,
                                    flags));

    /**
     * WebKitWebWindowFeatures:locationbar-visible:
     *
     * Controls whether the locationbar should be visible for the window.
     *
     * Since: 1.0.3
     */
    g_object_class_install_property(gobject_class,
                                    PROP_LOCATIONBAR_VISIBLE,
                                    g_param_spec_boolean(
                                    "locationbar-visible",
                                    "Locationbar Visible",
                                    "Controls whether the locationbar should be visible for the window.",
                                    TRUE,
                                    flags));

    /**
     * WebKitWebWindowFeatures:fullscreen:
     *
     * Controls whether window will be displayed fullscreen.
     *
     * Since: 1.0.3
     */
    g_object_class_install_property(gobject_class,
                                    PROP_FULLSCREEN,
                                    g_param_spec_boolean(
                                    "fullscreen",
                                    "Fullscreen",
                                    "Controls whether window will be displayed fullscreen.",
                                    FALSE,
                                    flags));


    g_type_class_add_private(klass, sizeof(WebKitWebWindowFeaturesPrivate));
}

static void webkit_web_window_features_init(WebKitWebWindowFeatures* web_window_features)
{
    web_window_features->priv = G_TYPE_INSTANCE_GET_PRIVATE(web_window_features, WEBKIT_TYPE_WEB_WINDOW_FEATURES, WebKitWebWindowFeaturesPrivate);
}

static void webkit_web_window_features_set_property(GObject* object, guint prop_id, const GValue* value, GParamSpec* pspec)
{
    WebKitWebWindowFeatures* web_window_features = WEBKIT_WEB_WINDOW_FEATURES(object);
    WebKitWebWindowFeaturesPrivate* priv = web_window_features->priv;

    switch(prop_id) {
    case PROP_X:
        priv->x = g_value_get_int(value);
        break;
    case PROP_Y:
        priv->y = g_value_get_int(value);
        break;
    case PROP_WIDTH:
        priv->width = g_value_get_int(value);
        break;
    case PROP_HEIGHT:
        priv->height = g_value_get_int(value);
        break;
    case PROP_TOOLBAR_VISIBLE:
        priv->toolbar_visible = g_value_get_boolean(value);
        break;
    case PROP_STATUSBAR_VISIBLE:
        priv->statusbar_visible = g_value_get_boolean(value);
        break;
    case PROP_SCROLLBAR_VISIBLE:
        priv->scrollbar_visible = g_value_get_boolean(value);
        break;
    case PROP_MENUBAR_VISIBLE:
        priv->menubar_visible = g_value_get_boolean(value);
        break;
    case PROP_LOCATIONBAR_VISIBLE:
        priv->locationbar_visible = g_value_get_boolean(value);
        break;
    case PROP_FULLSCREEN:
        priv->fullscreen = g_value_get_boolean(value);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        break;
    }
}

static void webkit_web_window_features_get_property(GObject* object, guint prop_id, GValue* value, GParamSpec* pspec)
{
    WebKitWebWindowFeatures* web_window_features = WEBKIT_WEB_WINDOW_FEATURES(object);
    WebKitWebWindowFeaturesPrivate* priv = web_window_features->priv;

    switch (prop_id) {
    case PROP_X:
        g_value_set_int(value, priv->x);
        break;
    case PROP_Y:
        g_value_set_int(value, priv->y);
        break;
    case PROP_WIDTH:
        g_value_set_int(value, priv->width);
        break;
    case PROP_HEIGHT:
        g_value_set_int(value, priv->height);
        break;
    case PROP_TOOLBAR_VISIBLE:
        g_value_set_boolean(value, priv->toolbar_visible);
        break;
    case PROP_STATUSBAR_VISIBLE:
        g_value_set_boolean(value, priv->statusbar_visible);
        break;
    case PROP_SCROLLBAR_VISIBLE:
        g_value_set_boolean(value, priv->scrollbar_visible);
        break;
    case PROP_MENUBAR_VISIBLE:
        g_value_set_boolean(value, priv->menubar_visible);
        break;
    case PROP_LOCATIONBAR_VISIBLE:
        g_value_set_boolean(value, priv->locationbar_visible);
        break;
    case PROP_FULLSCREEN:
        g_value_set_boolean(value, priv->fullscreen);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        break;
    }
}

/**
 * webkit_web_window_features_new:
 *
 * Creates a new #WebKitWebWindowFeatures instance with default values. It must
 * be manually attached to a WebView.
 *
 * Returns: a new #WebKitWebWindowFeatures instance
 *
 * Since: 1.0.3
 */
WebKitWebWindowFeatures* webkit_web_window_features_new()
{
    return WEBKIT_WEB_WINDOW_FEATURES(g_object_new(WEBKIT_TYPE_WEB_WINDOW_FEATURES, NULL));
}

/**
 * webkit_web_window_features_equal:
 * @features1: a #WebKitWebWindowFeatures instance
 * @features2: another #WebKitWebWindowFeatures instance
 *
 * Decides if a #WebKitWebWindowFeatures instance equals another, as
 * in has the same values.
 *
 * Returns: %TRUE if the instances have the same values, %FALSE
 * otherwise
 *
 * Since: 1.0.3
 */
gboolean webkit_web_window_features_equal(WebKitWebWindowFeatures* features1, WebKitWebWindowFeatures* features2)
{
    if (features1 == features2)
        return TRUE;
    if (!features1 || !features2)
        return FALSE;

    WebKitWebWindowFeaturesPrivate* priv1 = features1->priv;
    WebKitWebWindowFeaturesPrivate* priv2 = features2->priv;

    if ((priv1->x == priv2->x)
        && (priv1->y == priv2->y)
        && (priv1->width == priv2->width)
        && (priv1->height == priv2->height)
        && (priv1->toolbar_visible == priv2->toolbar_visible)
        && (priv1->statusbar_visible == priv2->statusbar_visible)
        && (priv1->scrollbar_visible == priv2->scrollbar_visible)
        && (priv1->menubar_visible == priv2->menubar_visible)
        && (priv1->locationbar_visible == priv2->locationbar_visible)
        && (priv1->fullscreen == priv2->fullscreen))
        return TRUE;
    return FALSE;
}

namespace WebKit {

WebKitWebWindowFeatures* kitNew(const WebCore::WindowFeatures& features)
{
    WebKitWebWindowFeatures *webWindowFeatures = webkit_web_window_features_new();

    if(features.xSet)
        g_object_set(webWindowFeatures, "x", static_cast<int>(features.x), NULL);

    if(features.ySet)
        g_object_set(webWindowFeatures, "y", static_cast<int>(features.y), NULL);

    if(features.widthSet)
        g_object_set(webWindowFeatures, "width", static_cast<int>(features.width), NULL);

    if(features.heightSet)
        g_object_set(webWindowFeatures, "height", static_cast<int>(features.height), NULL);

    g_object_set(webWindowFeatures,
                 "toolbar-visible", features.toolBarVisible,
                 "statusbar-visible", features.statusBarVisible,
                 "scrollbar-visible", features.scrollbarsVisible,
                 "menubar-visible", features.menuBarVisible,
                 "locationbar-visible", features.locationBarVisible,
                 "fullscreen", features.fullscreen,
                 NULL);

    return webWindowFeatures;
}

}
