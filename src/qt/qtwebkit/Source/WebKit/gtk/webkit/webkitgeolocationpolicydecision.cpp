/*
 * Copyright (C) 2010 Arno Renevier
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
#include "webkitgeolocationpolicydecision.h"

#include "Geolocation.h"
#include "webkitgeolocationpolicydecisionprivate.h"
#include "webkitglobalsprivate.h"

/**
 * SECTION:webkitgeolocationpolicydecision
 * @short_description: Liaison between WebKit and the application regarding asynchronous geolocation policy decisions
 *
 * #WebKitGeolocationPolicyDecision objects are given to the application when
 * geolocation-policy-decision-requested signal is emitted. The application
 * uses it to tell the engine whether it wants to allow or deny geolocation for
 * a given frame.
 */

G_DEFINE_TYPE(WebKitGeolocationPolicyDecision, webkit_geolocation_policy_decision, G_TYPE_OBJECT);

struct _WebKitGeolocationPolicyDecisionPrivate {
    WebKitWebFrame* frame;
#if ENABLE(GEOLOCATION)
    WebCore::Geolocation* geolocation;
#endif
};

static void webkit_geolocation_policy_decision_class_init(WebKitGeolocationPolicyDecisionClass* decisionClass)
{
    g_type_class_add_private(decisionClass, sizeof(WebKitGeolocationPolicyDecisionPrivate));
}

static void webkit_geolocation_policy_decision_init(WebKitGeolocationPolicyDecision* decision)
{
    decision->priv = G_TYPE_INSTANCE_GET_PRIVATE(decision, WEBKIT_TYPE_GEOLOCATION_POLICY_DECISION, WebKitGeolocationPolicyDecisionPrivate);
}

#if ENABLE(GEOLOCATION)
WebKitGeolocationPolicyDecision* webkit_geolocation_policy_decision_new(WebKitWebFrame* frame, WebCore::Geolocation* geolocation)
{
    g_return_val_if_fail(frame, NULL);
    WebKitGeolocationPolicyDecision* decision = WEBKIT_GEOLOCATION_POLICY_DECISION(g_object_new(WEBKIT_TYPE_GEOLOCATION_POLICY_DECISION, NULL));
    WebKitGeolocationPolicyDecisionPrivate* priv = decision->priv;

    priv->frame = frame;
    priv->geolocation = geolocation;
    return decision;
}
#endif

/**
 * webkit_geolocation_policy_allow:
 * @decision: a #WebKitGeolocationPolicyDecision
 *
 * Will send the allow decision to the policy implementer.
 *
 * Since: 1.1.23
 */
void webkit_geolocation_policy_allow(WebKitGeolocationPolicyDecision* decision)
{
#if ENABLE(GEOLOCATION)
    g_return_if_fail(WEBKIT_IS_GEOLOCATION_POLICY_DECISION(decision));

    WebKitGeolocationPolicyDecisionPrivate* priv = decision->priv;
    priv->geolocation->setIsAllowed(TRUE);
#else
    WEBKIT_WARN_FEATURE_NOT_PRESENT("Geolocation")
#endif
}

/**
 * webkit_geolocation_policy_deny:
 * @decision: a #WebKitGeolocationPolicyDecision
 *
 * Will send the deny decision to the policy implementer.
 *
 * Since: 1.1.23
 */
void webkit_geolocation_policy_deny(WebKitGeolocationPolicyDecision* decision)
{
#if ENABLE(GEOLOCATION)
    g_return_if_fail(WEBKIT_IS_GEOLOCATION_POLICY_DECISION(decision));

    WebKitGeolocationPolicyDecisionPrivate* priv = decision->priv;
    priv->geolocation->setIsAllowed(FALSE);
#else
    WEBKIT_WARN_FEATURE_NOT_PRESENT("Geolocation")
#endif
}

