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
#include "WebKitNavigationPolicyDecision.h"

#include "WebEvent.h"
#include "WebKitEnumTypes.h"
#include "WebKitPolicyDecisionPrivate.h"
#include "WebKitURIRequestPrivate.h"
#include "WebURLRequest.h"
#include <glib/gi18n-lib.h>
#include <wtf/gobject/GRefPtr.h>
#include <wtf/text/CString.h>

using namespace WebKit;
using namespace WebCore;

/**
 * SECTION: WebKitNavigationPolicyDecision
 * @Short_description: A policy decision for navigation actions
 * @Title: WebKitNavigationPolicyDecision
 * @See_also: #WebKitPolicyDecision, #WebKitWebView
 *
 * WebKitNavigationPolicyDecision represents a policy decision for events associated with
 * navigations. If the value of #WebKitNavigationPolicyDecision:mouse-button is not 0, then
 * the navigation was triggered by a mouse event.
 */

struct _WebKitNavigationPolicyDecisionPrivate {
    WebKitNavigationType navigationType;
    unsigned modifiers;
    unsigned mouseButton;
    GRefPtr<WebKitURIRequest> request;
    CString frameName;
};

WEBKIT_DEFINE_TYPE(WebKitNavigationPolicyDecision, webkit_navigation_policy_decision, WEBKIT_TYPE_POLICY_DECISION)

enum {
    PROP_0,
    PROP_NAVIGATION_TYPE,
    PROP_MOUSE_BUTTON,
    PROP_MODIFIERS,
    PROP_REQUEST,
    PROP_FRAME_NAME,
};

static void webkitNavigationPolicyDecisionGetProperty(GObject* object, guint propId, GValue* value, GParamSpec* paramSpec)
{
    WebKitNavigationPolicyDecision* decision = WEBKIT_NAVIGATION_POLICY_DECISION(object);
    switch (propId) {
    case PROP_NAVIGATION_TYPE:
        g_value_set_enum(value, webkit_navigation_policy_decision_get_navigation_type(decision));
        break;
    case PROP_MOUSE_BUTTON:
        g_value_set_enum(value, webkit_navigation_policy_decision_get_mouse_button(decision));
        break;
    case PROP_MODIFIERS:
        g_value_set_uint(value, webkit_navigation_policy_decision_get_modifiers(decision));
        break;
    case PROP_REQUEST:
        g_value_set_object(value, webkit_navigation_policy_decision_get_request(decision));
        break;
    case PROP_FRAME_NAME:
        g_value_set_string(value, webkit_navigation_policy_decision_get_frame_name(decision));
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, propId, paramSpec);
        break;
    }
}

static void webkit_navigation_policy_decision_class_init(WebKitNavigationPolicyDecisionClass* decisionClass)
{
    GObjectClass* objectClass = G_OBJECT_CLASS(decisionClass);
    objectClass->get_property = webkitNavigationPolicyDecisionGetProperty;

    /**
     * WebKitNavigationPolicyDecision:navigation-type:
     *
     * The type of navigation that triggered this policy decision. This is
     * useful for enacting different policies depending on what type of user
     * action caused the navigation.
     */
    g_object_class_install_property(objectClass,
                                    PROP_NAVIGATION_TYPE,
                                    g_param_spec_enum("navigation-type",
                                                      _("Navigation type"),
                                                      _("The type of navigation triggering this decision"),
                                                      WEBKIT_TYPE_NAVIGATION_TYPE,
                                                      WEBKIT_NAVIGATION_TYPE_LINK_CLICKED,
                                                      WEBKIT_PARAM_READABLE));

    /**
     * WebKitNavigationPolicyDecision:mouse-button:
     *
     * If the navigation associated with this policy decision was originally
     * triggered by a mouse event, this property contains non-zero button number
     * of the button triggering that event. The button numbers match those from GDK.
     * If the navigation was not triggered by a mouse event, the value of this
     * property will be 0.
     */
    g_object_class_install_property(objectClass,
                                    PROP_MOUSE_BUTTON,
                                    g_param_spec_uint("mouse-button",
                                                      _("Mouse button"),
                                                      _("The mouse button used if this decision was triggered by a mouse event"),
                                                      0, G_MAXUINT, 0,
                                                      WEBKIT_PARAM_READABLE));

    /**
     * WebKitNavigationPolicyDecision:modifiers:
     *
     * If the navigation associated with this policy decision was originally
     * triggered by a mouse event, this property contains a bitmask of various
     * #GdkModifierType values describing the modifiers used for that click.
     * If the navigation was not triggered by a mouse event or no modifiers
     * were active, the value of this property will be zero.
     */
    g_object_class_install_property(objectClass,
                                    PROP_MODIFIERS,
                                    g_param_spec_uint("modifiers",
                                                      _("Mouse event modifiers"),
                                                      _("The modifiers active if this decision was triggered by a mouse event"),
                                                      0, G_MAXUINT, 0,
                                                      WEBKIT_PARAM_READABLE));

    /**
     * WebKitNavigationPolicyDecision:request:
     *
     * This property contains the #WebKitURIRequest associated with this
     * navigation.
     */
    g_object_class_install_property(objectClass,
                                    PROP_REQUEST,
                                    g_param_spec_object("request",
                                                      _("Navigation URI request"),
                                                      _("The URI request that is associated with this navigation"),
                                                      WEBKIT_TYPE_URI_REQUEST,
                                                      WEBKIT_PARAM_READABLE));

    /**
     * WebKitNavigationPolicyDecision:frame-name:
     *
     * If this navigation request targets a new frame, this property contains
     * the name of that frame. For example if the decision was triggered by clicking a
     * link with a target attribute equal to "_blank", this property will contain the
     * value of that attribute. In all other cases, this value will be %NULL.
     */
    g_object_class_install_property(objectClass,
                                    PROP_FRAME_NAME,
                                    g_param_spec_string("frame-name",
                                                      _("Frame name"),
                                                      _("The name of the new frame this navigation action targets"),
                                                      0,
                                                      WEBKIT_PARAM_READABLE));
}

/**
 * webkit_navigation_policy_decision_get_navigation_type:
 * @decision: a #WebKitNavigationPolicyDecision
 *
 * Gets the value of the #WebKitNavigationPolicyDecision:navigation-type property.
 *
 * Returns: The type of navigation triggering this policy decision.
 */
WebKitNavigationType webkit_navigation_policy_decision_get_navigation_type(WebKitNavigationPolicyDecision* decision)
{
    g_return_val_if_fail(WEBKIT_IS_NAVIGATION_POLICY_DECISION(decision), WEBKIT_NAVIGATION_TYPE_OTHER);
    return decision->priv->navigationType;
}

/**
 * webkit_navigation_policy_decision_get_mouse_button:
 * @decision: a #WebKitNavigationPolicyDecision
 *
 * Gets the value of the #WebKitNavigationPolicyDecision:mouse-button property.
 *
 * Returns: The mouse button used if this decision was triggered by a mouse event or 0 otherwise
 */
guint webkit_navigation_policy_decision_get_mouse_button(WebKitNavigationPolicyDecision* decision)
{
    g_return_val_if_fail(WEBKIT_IS_NAVIGATION_POLICY_DECISION(decision), 0);
    return decision->priv->mouseButton;
}

/**
 * webkit_navigation_policy_decision_get_modifiers:
 * @decision: a #WebKitNavigationPolicyDecision
 *
 * Gets the value of the #WebKitNavigationPolicyDecision:modifiers property.
 *
 * Returns: The modifiers active if this decision was triggered by a mouse event
 */
unsigned webkit_navigation_policy_decision_get_modifiers(WebKitNavigationPolicyDecision* decision)
{
    g_return_val_if_fail(WEBKIT_IS_NAVIGATION_POLICY_DECISION(decision), 0);
    return decision->priv->modifiers;
}

/**
 * webkit_navigation_policy_decision_get_request:
 * @decision: a #WebKitNavigationPolicyDecision
 *
 * Gets the value of the #WebKitNavigationPolicyDecision:request property.
 *
 * Returns: (transfer none): The URI request that is associated with this navigation
 */
WebKitURIRequest* webkit_navigation_policy_decision_get_request(WebKitNavigationPolicyDecision* decision)
{
    g_return_val_if_fail(WEBKIT_IS_NAVIGATION_POLICY_DECISION(decision), 0);
    return decision->priv->request.get();
}

/**
 * webkit_navigation_policy_decision_get_frame_name:
 * @decision: a #WebKitNavigationPolicyDecision
 *
 * Gets the value of the #WebKitNavigationPolicyDecision:frame-name property.
 *
 * Returns: The name of the new frame this navigation action targets or %NULL
 */
const char* webkit_navigation_policy_decision_get_frame_name(WebKitNavigationPolicyDecision* decision)
{
    g_return_val_if_fail(WEBKIT_IS_NAVIGATION_POLICY_DECISION(decision), 0);
    return decision->priv->frameName.data();
}

COMPILE_ASSERT_MATCHING_ENUM(WEBKIT_NAVIGATION_TYPE_LINK_CLICKED, NavigationTypeLinkClicked);
COMPILE_ASSERT_MATCHING_ENUM(WEBKIT_NAVIGATION_TYPE_FORM_SUBMITTED, NavigationTypeFormSubmitted);
COMPILE_ASSERT_MATCHING_ENUM(WEBKIT_NAVIGATION_TYPE_BACK_FORWARD, NavigationTypeBackForward);
COMPILE_ASSERT_MATCHING_ENUM(WEBKIT_NAVIGATION_TYPE_RELOAD, NavigationTypeReload);
COMPILE_ASSERT_MATCHING_ENUM(WEBKIT_NAVIGATION_TYPE_FORM_RESUBMITTED, NavigationTypeFormResubmitted);
COMPILE_ASSERT_MATCHING_ENUM(WEBKIT_NAVIGATION_TYPE_OTHER, NavigationTypeOther);

WebKitNavigationPolicyDecision* webkitNavigationPolicyDecisionCreate(WebKitNavigationType navigationType, unsigned mouseButton, unsigned modifiers, WebURLRequest* request, const char* frameName, WebFramePolicyListenerProxy* listener)
{
    WebKitNavigationPolicyDecision* decision = WEBKIT_NAVIGATION_POLICY_DECISION(g_object_new(WEBKIT_TYPE_NAVIGATION_POLICY_DECISION, NULL));
    decision->priv->navigationType = navigationType;
    decision->priv->mouseButton = mouseButton;
    decision->priv->modifiers = modifiers;
    decision->priv->request = adoptGRef(webkitURIRequestCreateForResourceRequest(request->resourceRequest()));
    decision->priv->frameName = frameName;
    webkitPolicyDecisionSetListener(WEBKIT_POLICY_DECISION(decision), listener);
    return decision;
}
