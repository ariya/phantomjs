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
#include "WebKitPolicyDecision.h"

#include "WebFramePolicyListenerProxy.h"
#include "WebKitPolicyDecisionPrivate.h"

using namespace WebKit;

/**
 * SECTION: WebKitPolicyDecision
 * @Short_description: A pending policy decision
 * @Title: WebKitPolicyDecision
 * @See_also: #WebKitWebView
 *
 * Often WebKit allows the client to decide the policy for certain
 * operations. For instance, a client may want to open a link in a new
 * tab, block a navigation entirely, query the user or trigger a download
 * instead of a navigation. In these cases WebKit will fire the
 * #WebKitWebView::decide-policy signal with a #WebKitPolicyDecision
 * object. If the signal handler does nothing, WebKit will act as if
 * webkit_policy_decision_use() was called as soon as signal handling
 * completes. To make a policy decision asynchronously, simply increment
 * the reference count of the #WebKitPolicyDecision object.
 */

struct _WebKitPolicyDecisionPrivate {
    RefPtr<WebFramePolicyListenerProxy> listener;
    bool madePolicyDecision;
};

WEBKIT_DEFINE_ABSTRACT_TYPE(WebKitPolicyDecision, webkit_policy_decision, G_TYPE_OBJECT)

static void webkitPolicyDecisionDispose(GObject* object)
{
    webkit_policy_decision_use(WEBKIT_POLICY_DECISION(object));
    G_OBJECT_CLASS(webkit_policy_decision_parent_class)->dispose(object);
}

void webkitPolicyDecisionSetListener(WebKitPolicyDecision* decision, WebFramePolicyListenerProxy* listener)
{
     decision->priv->listener = listener;
}

static void webkit_policy_decision_class_init(WebKitPolicyDecisionClass* decisionClass)
{
    GObjectClass* objectClass = G_OBJECT_CLASS(decisionClass);
    objectClass->dispose = webkitPolicyDecisionDispose;
}

/**
 * webkit_policy_decision_use:
 * @decision: a #WebKitPolicyDecision
 *
 * Accept the action which triggerd this decision.
 */
void webkit_policy_decision_use(WebKitPolicyDecision* decision)
{
    g_return_if_fail(WEBKIT_IS_POLICY_DECISION(decision));

    if (decision->priv->madePolicyDecision)
        return;

    decision->priv->listener->use();
    decision->priv->madePolicyDecision = true;
}

/**
 * webkit_policy_decision_ignore:
 * @decision: a #WebKitPolicyDecision
 *
 * Ignore the action which triggerd this decision. For instance, for a
 * #WebKitResponsePolicyDecision, this would cancel the request.
 */
void webkit_policy_decision_ignore(WebKitPolicyDecision* decision)
{
    g_return_if_fail(WEBKIT_IS_POLICY_DECISION(decision));

    if (decision->priv->madePolicyDecision)
        return;

    decision->priv->listener->ignore();
    decision->priv->madePolicyDecision = true;
}

/**
 * webkit_policy_decision_download:
 * @decision: a #WebKitPolicyDecision
 *
 * Spawn a download from this decision.
 */
void webkit_policy_decision_download(WebKitPolicyDecision* decision)
{
    g_return_if_fail(WEBKIT_IS_POLICY_DECISION(decision));

    if (decision->priv->madePolicyDecision)
        return;

    decision->priv->listener->download();
    decision->priv->madePolicyDecision = true;
}
