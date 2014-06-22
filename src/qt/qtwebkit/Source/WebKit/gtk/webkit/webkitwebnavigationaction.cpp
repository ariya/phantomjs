/*
 * Copyright (C) 2008 Collabora Ltd.
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
#include "webkitwebnavigationaction.h"

#include "FrameLoaderTypes.h"
#include "webkitenumtypes.h"
#include "webkitglobalsprivate.h"
#include <glib/gi18n-lib.h>
#include <string.h>
#include <wtf/Assertions.h>

static void webkit_web_navigation_action_set_target_frame(WebKitWebNavigationAction* navigationAction, const gchar* targetFrame);

/**
 * SECTION:webkitwebnavigationaction
 * @short_description: Object used to report details of navigation actions
 *
 * #WebKitWebNavigationAction is used in signals to provide details about
 * what led the navigation to happen. This includes, for instance, if the user
 * clicked a link to start that navigation, and what mouse button was used.
 */

struct _WebKitWebNavigationActionPrivate {
    WebKitWebNavigationReason reason;
    gchar* originalUri;
    gint button;
    gint modifier_state;
    gchar* targetFrame;
};

enum  {
    PROP_0,

    PROP_REASON,
    PROP_ORIGINAL_URI,
    PROP_BUTTON,
    PROP_MODIFIER_STATE,
    PROP_TARGET_FRAME
};

G_DEFINE_TYPE(WebKitWebNavigationAction, webkit_web_navigation_action, G_TYPE_OBJECT)


static void webkit_web_navigation_action_get_property(GObject* object, guint propertyId, GValue* value, GParamSpec* pspec)
{
    WebKitWebNavigationAction* navigationAction = WEBKIT_WEB_NAVIGATION_ACTION(object);

    switch(propertyId) {
    case PROP_REASON:
        g_value_set_enum(value, webkit_web_navigation_action_get_reason(navigationAction));
        break;
    case PROP_ORIGINAL_URI:
        g_value_set_string(value, webkit_web_navigation_action_get_original_uri(navigationAction));
        break;
    case PROP_BUTTON:
        g_value_set_int(value, webkit_web_navigation_action_get_button(navigationAction));
        break;
    case PROP_MODIFIER_STATE:
        g_value_set_int(value, webkit_web_navigation_action_get_modifier_state(navigationAction));
        break;
    case PROP_TARGET_FRAME:
        g_value_set_string(value, webkit_web_navigation_action_get_target_frame(navigationAction));
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, propertyId, pspec);
        break;
    }
}

static void webkit_web_navigation_action_set_property(GObject* object, guint propertyId, const GValue* value, GParamSpec* pspec)
{
    WebKitWebNavigationAction* navigationAction = WEBKIT_WEB_NAVIGATION_ACTION(object);
    WebKitWebNavigationActionPrivate* priv = navigationAction->priv;

    switch(propertyId) {
    case PROP_REASON:
        webkit_web_navigation_action_set_reason(navigationAction, (WebKitWebNavigationReason)g_value_get_enum(value));
        break;
    case PROP_ORIGINAL_URI:
        webkit_web_navigation_action_set_original_uri(navigationAction, g_value_get_string(value));
        break;
    case PROP_BUTTON:
        priv->button = g_value_get_int(value);
        break;
    case PROP_MODIFIER_STATE:
        priv->modifier_state = g_value_get_int(value);
        break;
    case PROP_TARGET_FRAME:
        webkit_web_navigation_action_set_target_frame(navigationAction, g_value_get_string(value));
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, propertyId, pspec);
        break;
    }
}

static void webkit_web_navigation_action_init(WebKitWebNavigationAction* navigationAction)
{
    navigationAction->priv = G_TYPE_INSTANCE_GET_PRIVATE(navigationAction, WEBKIT_TYPE_WEB_NAVIGATION_ACTION, WebKitWebNavigationActionPrivate);
}

static void webkit_web_navigation_action_finalize(GObject* obj)
{
    WebKitWebNavigationAction* navigationAction = WEBKIT_WEB_NAVIGATION_ACTION(obj);
    WebKitWebNavigationActionPrivate* priv = navigationAction->priv;

    g_free(priv->originalUri);
    g_free(priv->targetFrame);

    G_OBJECT_CLASS(webkit_web_navigation_action_parent_class)->finalize(obj);
}

static void webkit_web_navigation_action_class_init(WebKitWebNavigationActionClass* requestClass)
{
    GObjectClass* objectClass = G_OBJECT_CLASS(requestClass);

    objectClass->get_property = webkit_web_navigation_action_get_property;
    objectClass->set_property = webkit_web_navigation_action_set_property;
    objectClass->finalize = webkit_web_navigation_action_finalize;

    /**
     * WebKitWebNavigationAction:reason:
     *
     * The reason why this navigation is occuring.
     *
     * Since: 1.0.3
     */
    g_object_class_install_property(objectClass, PROP_REASON,
                                    g_param_spec_enum("reason",
                                                      _("Reason"),
                                                      _("The reason why this navigation is occurring"),
                                                      WEBKIT_TYPE_WEB_NAVIGATION_REASON,
                                                      WEBKIT_WEB_NAVIGATION_REASON_OTHER,
                                                      (GParamFlags)(WEBKIT_PARAM_READWRITE | G_PARAM_CONSTRUCT)));

    /**
     * WebKitWebNavigationAction:original-uri:
     *
     * The URI that was requested as the target for the navigation.
     *
     * Since: 1.0.3
     */
    g_object_class_install_property(objectClass, PROP_ORIGINAL_URI,
                                    g_param_spec_string("original-uri",
                                                        _("Original URI"),
                                                        _("The URI that was requested as the target for the navigation"),
                                                        "",
                                                        (GParamFlags)(WEBKIT_PARAM_READWRITE | G_PARAM_CONSTRUCT)));
    /**
     * WebKitWebNavigationAction:button:
     *
     * The GTK+ identifier for the mouse button used to click. Notice that GTK+ button values
     * are 1, 2 and 3 for left, middle and right buttons, so they are DOM button values +1. If the action was not
     * initiated by a mouse click the value will be -1.
     *
     * Since: 1.0.3
     */
    g_object_class_install_property(objectClass, PROP_BUTTON,
                                    g_param_spec_int("button",
                                                     _("Button"),
                                                     _("The button used to click"),
                                                     -1,
                                                     G_MAXINT,
                                                     -1,
                                                     (GParamFlags)(WEBKIT_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY)));

    /**
     * WebKitWebNavigationAction:modifier-state:
     *
     * The state of the modifier keys when the action was requested.
     *
     * Since: 1.0.3
     */
    g_object_class_install_property(objectClass, PROP_MODIFIER_STATE,
                                    g_param_spec_int("modifier-state",
                                                     _("Modifier state"),
                                                     _("A bitmask representing the state of the modifier keys"),
                                                     0,
                                                     G_MAXINT,
                                                     0,
                                                     (GParamFlags)(WEBKIT_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY)));

    /**
     * WebKitWebNavigationAction:target-frame:
     *
     * The target frame for the navigation.
     *
     * Since: 1.1.13
     */
    g_object_class_install_property(objectClass, PROP_TARGET_FRAME,
                                    g_param_spec_string("target-frame",
                                                        _("Target frame"),
                                                        _("The target frame for the navigation"),
                                                        NULL,
                                                        (GParamFlags)(WEBKIT_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY)));



    g_type_class_add_private(requestClass, sizeof(WebKitWebNavigationActionPrivate));
}

/**
 * webkit_web_navigation_action_get_reason:
 * @navigationAction: a #WebKitWebNavigationAction
 *
 * Returns the reason why WebKit is requesting a navigation.
 *
 * Return value: a #WebKitWebNavigationReason
 *
 * Since: 1.0.3
 */
WebKitWebNavigationReason webkit_web_navigation_action_get_reason(WebKitWebNavigationAction* navigationAction)
{
    g_return_val_if_fail(WEBKIT_IS_WEB_NAVIGATION_ACTION(navigationAction), WEBKIT_WEB_NAVIGATION_REASON_OTHER);

    return navigationAction->priv->reason;
}

/**
 * webkit_web_navigation_action_set_reason:
 * @navigationAction: a #WebKitWebNavigationAction
 * @reason: a #WebKitWebNavigationReason
 *
 * Sets the reason why WebKit is requesting a navigation.
 *
 * Since: 1.0.3
 */
void webkit_web_navigation_action_set_reason(WebKitWebNavigationAction* navigationAction, WebKitWebNavigationReason reason)
{
    g_return_if_fail(WEBKIT_IS_WEB_NAVIGATION_ACTION(navigationAction));

    if (navigationAction->priv->reason == reason)
        return;

    navigationAction->priv->reason = reason;
    g_object_notify(G_OBJECT(navigationAction), "reason");
}

/**
 * webkit_web_navigation_action_get_original_uri:
 * @navigationAction: a #WebKitWebNavigationAction
 *
 * Returns the URI that was originally requested. This may differ from the
 * navigation target, for instance because of a redirect.
 *
 * Return value: the originally requested URI
 *
 * Since: 1.0.3
 */
const gchar* webkit_web_navigation_action_get_original_uri(WebKitWebNavigationAction* navigationAction)
{
    g_return_val_if_fail(WEBKIT_IS_WEB_NAVIGATION_ACTION(navigationAction), NULL);

    return navigationAction->priv->originalUri;
}

/**
 * webkit_web_navigation_action_set_original_uri:
 * @navigationAction: a #WebKitWebNavigationAction
 * @originalUri: a URI
 *
 * Sets the URI that was originally requested. This may differ from the
 * navigation target, for instance because of a redirect.
 *
 * Since: 1.0.3
 */
void webkit_web_navigation_action_set_original_uri(WebKitWebNavigationAction* navigationAction, const gchar* originalUri)
{
    g_return_if_fail(WEBKIT_IS_WEB_NAVIGATION_ACTION(navigationAction));
    g_return_if_fail(originalUri);

    if (navigationAction->priv->originalUri &&
        (!strcmp(navigationAction->priv->originalUri, originalUri)))
        return;

    g_free(navigationAction->priv->originalUri);
    navigationAction->priv->originalUri = g_strdup(originalUri);
    g_object_notify(G_OBJECT(navigationAction), "original-uri");
}

/**
 * webkit_web_navigation_action_get_button:
 * @navigationAction: a #WebKitWebNavigationAction
 *
 * The GTK+ identifier for the mouse button used to click. Notice that GTK+ button values
 * are 1, 2 and 3 for left, middle and right buttons, so they are DOM button values +1. If the action was not
 * initiated by a mouse click the value will be -1.
 *
 * Return value: the mouse button used to click
 *
 * Since: 1.0.3
 */
gint webkit_web_navigation_action_get_button(WebKitWebNavigationAction* navigationAction)
{
    g_return_val_if_fail(WEBKIT_IS_WEB_NAVIGATION_ACTION(navigationAction), -1);

    return navigationAction->priv->button;
}

/**
 * webkit_web_navigation_action_get_modifier_state:
 * @navigationAction: a #WebKitWebNavigationAction
 *
 * Returns a bitmask with the the state of the modifier keys.
 *
 * Return value: a bitmask with the state of the modifier keys
 *
 * Since: 1.0.3
 */
gint webkit_web_navigation_action_get_modifier_state(WebKitWebNavigationAction* navigationAction)
{
    g_return_val_if_fail(WEBKIT_IS_WEB_NAVIGATION_ACTION(navigationAction), 0);

    return navigationAction->priv->modifier_state;
}

/**
 * webkit_web_navigation_action_get_target_frame:
 * @navigationAction: a #WebKitWebNavigationAction
 *
 * Returns the target frame of the action.
 *
 * Return value: the target frame of the action or NULL
 * if there is no target.
 *
 * Since: 1.1.13
 */
const gchar* webkit_web_navigation_action_get_target_frame(WebKitWebNavigationAction* navigationAction)
{
    g_return_val_if_fail(WEBKIT_IS_WEB_NAVIGATION_ACTION(navigationAction), NULL);

    return navigationAction->priv->targetFrame;
}

static void webkit_web_navigation_action_set_target_frame(WebKitWebNavigationAction* navigationAction, const gchar* targetFrame)
{
    if (!g_strcmp0(navigationAction->priv->targetFrame, targetFrame))
        return;

    g_free(navigationAction->priv->targetFrame);
    navigationAction->priv->targetFrame = g_strdup(targetFrame);
    g_object_notify(G_OBJECT(navigationAction), "target-frame");
}

namespace WebKit {

WebKitWebNavigationReason kit(WebCore::NavigationType type)
{
    return (WebKitWebNavigationReason)type;
}

WebCore::NavigationType core(WebKitWebNavigationReason type)
{
    return static_cast<WebCore::NavigationType>(type);
}

}
