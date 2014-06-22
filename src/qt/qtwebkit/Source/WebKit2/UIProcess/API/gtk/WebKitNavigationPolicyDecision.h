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

#if !defined(__WEBKIT2_H_INSIDE__) && !defined(WEBKIT2_COMPILATION)
#error "Only <webkit2/webkit2.h> can be included directly."
#endif

#ifndef WebKitNavigationPolicyDecision_h
#define WebKitNavigationPolicyDecision_h

#include <glib-object.h>
#include <webkit2/WebKitDefines.h>
#include <webkit2/WebKitPolicyDecision.h>
#include <webkit2/WebKitURIRequest.h>

G_BEGIN_DECLS

/**
 * WebKitNavigationType:
 * @WEBKIT_NAVIGATION_TYPE_LINK_CLICKED: The navigation was triggered by clicking a link.
 * @WEBKIT_NAVIGATION_TYPE_FORM_SUBMITTED: The navigation was triggered by submitting a form.
 * @WEBKIT_NAVIGATION_TYPE_BACK_FORWARD: The navigation was triggered by navigating forward or backward.
 * @WEBKIT_NAVIGATION_TYPE_RELOAD: The navigation was triggered by reloading.
 * @WEBKIT_NAVIGATION_TYPE_FORM_RESUBMITTED: The navigation was triggered by resubmitting a form.
 * @WEBKIT_NAVIGATION_TYPE_OTHER: The navigation was triggered by some other action.
 *
 * Enum values used to denote the various navigation types.
 */
typedef enum {
    WEBKIT_NAVIGATION_TYPE_LINK_CLICKED,
    WEBKIT_NAVIGATION_TYPE_FORM_SUBMITTED,
    WEBKIT_NAVIGATION_TYPE_BACK_FORWARD,
    WEBKIT_NAVIGATION_TYPE_RELOAD,
    WEBKIT_NAVIGATION_TYPE_FORM_RESUBMITTED,
    WEBKIT_NAVIGATION_TYPE_OTHER,
} WebKitNavigationType;

#define WEBKIT_TYPE_NAVIGATION_POLICY_DECISION            (webkit_navigation_policy_decision_get_type())
#define WEBKIT_NAVIGATION_POLICY_DECISION(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj), WEBKIT_TYPE_NAVIGATION_POLICY_DECISION, WebKitNavigationPolicyDecision))
#define WEBKIT_NAVIGATION_POLICY_DECISION_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass),  WEBKIT_TYPE_NAVIGATION_POLICY_DECISION, WebKitNavigationPolicyDecisionClass))
#define WEBKIT_IS_NAVIGATION_POLICY_DECISION(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj), WEBKIT_TYPE_NAVIGATION_POLICY_DECISION))
#define WEBKIT_IS_NAVIGATION_POLICY_DECISION_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass),  WEBKIT_TYPE_NAVIGATION_POLICY_DECISION))
#define WEBKIT_NAVIGATION_POLICY_DECISION_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj),  WEBKIT_TYPE_NAVIGATION_POLICY_DECISION, WebKitNavigationPolicyDecisionClass))

typedef struct _WebKitNavigationPolicyDecision        WebKitNavigationPolicyDecision;
typedef struct _WebKitNavigationPolicyDecisionClass   WebKitNavigationPolicyDecisionClass;
typedef struct _WebKitNavigationPolicyDecisionPrivate WebKitNavigationPolicyDecisionPrivate;

struct _WebKitNavigationPolicyDecision {
    WebKitPolicyDecision parent;

    /*< private >*/
    WebKitNavigationPolicyDecisionPrivate *priv;
};

struct _WebKitNavigationPolicyDecisionClass {
    WebKitPolicyDecisionClass parent_class;

    void (*_webkit_reserved0) (void);
    void (*_webkit_reserved1) (void);
    void (*_webkit_reserved2) (void);
    void (*_webkit_reserved3) (void);
};

WEBKIT_API GType
webkit_navigation_policy_decision_get_type            (void);

WEBKIT_API WebKitNavigationType
webkit_navigation_policy_decision_get_navigation_type (WebKitNavigationPolicyDecision *decision);
WEBKIT_API guint
webkit_navigation_policy_decision_get_mouse_button    (WebKitNavigationPolicyDecision *decision);
WEBKIT_API guint
webkit_navigation_policy_decision_get_modifiers       (WebKitNavigationPolicyDecision *decision);
WEBKIT_API WebKitURIRequest *
webkit_navigation_policy_decision_get_request         (WebKitNavigationPolicyDecision *decision);
WEBKIT_API const gchar *
webkit_navigation_policy_decision_get_frame_name      (WebKitNavigationPolicyDecision *decision);
G_END_DECLS

#endif
