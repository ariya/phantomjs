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
#include "WebKitResponsePolicyDecision.h"

#include "WebKitPolicyDecisionPrivate.h"
#include "WebKitPrivate.h"
#include "WebKitURIRequestPrivate.h"
#include "WebKitURIResponsePrivate.h"
#include "WebURLRequest.h"
#include "WebURLResponse.h"
#include <glib/gi18n-lib.h>
#include <wtf/gobject/GRefPtr.h>
#include <wtf/text/CString.h>

using namespace WebKit;

/**
 * SECTION: WebKitResponsePolicyDecision
 * @Short_description: A policy decision for resource responses
 * @Title: WebKitResponsePolicyDecision
 * @See_also: #WebKitPolicyDecision, #WebKitWebView
 *
 * WebKitResponsePolicyDecision represents a policy decision for a
 * resource response, whether from the network or the local system.
 * A very common usecase for these types of decision is deciding
 * whether or not to download a particular resource or to load it
 * normally.
 */

struct _WebKitResponsePolicyDecisionPrivate {
    GRefPtr<WebKitURIRequest> request;
    GRefPtr<WebKitURIResponse> response;
};

WEBKIT_DEFINE_TYPE(WebKitResponsePolicyDecision, webkit_response_policy_decision, WEBKIT_TYPE_POLICY_DECISION)

enum {
    PROP_0,
    PROP_REQUEST,
    PROP_RESPONSE,
};

static void webkitResponsePolicyDecisionGetProperty(GObject* object, guint propId, GValue* value, GParamSpec* paramSpec)
{
    WebKitResponsePolicyDecision* decision = WEBKIT_RESPONSE_POLICY_DECISION(object);
    switch (propId) {
    case PROP_REQUEST:
        g_value_set_object(value, webkit_response_policy_decision_get_request(decision));
        break;
    case PROP_RESPONSE:
        g_value_set_object(value, webkit_response_policy_decision_get_response(decision));
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, propId, paramSpec);
        break;
    }
}

static void webkit_response_policy_decision_class_init(WebKitResponsePolicyDecisionClass* decisionClass)
{
    GObjectClass* objectClass = G_OBJECT_CLASS(decisionClass);
    objectClass->get_property = webkitResponsePolicyDecisionGetProperty;

    /**
     * WebKitResponsePolicyDecision:request:
     *
     * This property contains the #WebKitURIRequest associated with this
     * policy decision.
     */
    g_object_class_install_property(objectClass,
                                    PROP_REQUEST,
                                    g_param_spec_object("request",
                                                      _("Response URI request"),
                                                      _("The URI request that is associated with this policy decision"),
                                                      WEBKIT_TYPE_URI_REQUEST,
                                                      WEBKIT_PARAM_READABLE));

    /**
     * WebKitResponsePolicyDecision:response:
     *
     * This property contains the #WebKitURIResponse associated with this
     * policy decision.
     */
    g_object_class_install_property(objectClass,
                                    PROP_REQUEST,
                                    g_param_spec_object("response",
                                                      _("URI response"),
                                                      _("The URI response that is associated with this policy decision"),
                                                      WEBKIT_TYPE_URI_REQUEST,
                                                      WEBKIT_PARAM_READABLE));

}

/**
 * webkit_response_policy_decision_get_request:
 * @decision: a #WebKitResponsePolicyDecision
 *
 * Gets the value of the #WebKitResponsePolicyDecision:request property.
 *
 * Returns: (transfer none): The URI request that is associated with this policy decision.
 */
WebKitURIRequest* webkit_response_policy_decision_get_request(WebKitResponsePolicyDecision* decision)
{
    g_return_val_if_fail(WEBKIT_IS_RESPONSE_POLICY_DECISION(decision), 0);
    return decision->priv->request.get();
}

/**
 * webkit_response_policy_decision_get_response:
 * @decision: a #WebKitResponsePolicyDecision
 *
 * Gets the value of the #WebKitResponsePolicyDecision:response property.
 *
 * Returns: (transfer none): The URI response that is associated with this policy decision.
 */
WebKitURIResponse* webkit_response_policy_decision_get_response(WebKitResponsePolicyDecision* decision)
{
    g_return_val_if_fail(WEBKIT_IS_RESPONSE_POLICY_DECISION(decision), 0);
    return decision->priv->response.get();
}

WebKitResponsePolicyDecision* webkitResponsePolicyDecisionCreate(WebURLRequest* request, WebURLResponse* response, WebFramePolicyListenerProxy* listener)
{
    WebKitResponsePolicyDecision* decision = WEBKIT_RESPONSE_POLICY_DECISION(g_object_new(WEBKIT_TYPE_RESPONSE_POLICY_DECISION, NULL));
    decision->priv->request = adoptGRef(webkitURIRequestCreateForResourceRequest(request->resourceRequest()));
    decision->priv->response = adoptGRef(webkitURIResponseCreateForResourceResponse(response->resourceResponse()));
    webkitPolicyDecisionSetListener(WEBKIT_POLICY_DECISION(decision), listener);
    return decision;
}
