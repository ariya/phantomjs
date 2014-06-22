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

#ifndef WebKitPolicyDecision_h
#define WebKitPolicyDecision_h

#include <glib-object.h>
#include <webkit2/WebKitDefines.h>

G_BEGIN_DECLS

#define WEBKIT_TYPE_POLICY_DECISION            (webkit_policy_decision_get_type())
#define WEBKIT_POLICY_DECISION(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj), WEBKIT_TYPE_POLICY_DECISION, WebKitPolicyDecision))
#define WEBKIT_POLICY_DECISION_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass),  WEBKIT_TYPE_POLICY_DECISION, WebKitPolicyDecisionClass))
#define WEBKIT_IS_POLICY_DECISION(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj), WEBKIT_TYPE_POLICY_DECISION))
#define WEBKIT_IS_POLICY_DECISION_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass),  WEBKIT_TYPE_POLICY_DECISION))
#define WEBKIT_POLICY_DECISION_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj),  WEBKIT_TYPE_POLICY_DECISION, WebKitPolicyDecisionClass))

typedef struct _WebKitPolicyDecision        WebKitPolicyDecision;
typedef struct _WebKitPolicyDecisionClass   WebKitPolicyDecisionClass;
typedef struct _WebKitPolicyDecisionPrivate WebKitPolicyDecisionPrivate;

struct _WebKitPolicyDecision {
    GObject parent;

    /*< private >*/
    WebKitPolicyDecisionPrivate *priv;
};

struct _WebKitPolicyDecisionClass {
    GObjectClass parent_class;

    void (*_webkit_reserved0) (void);
    void (*_webkit_reserved1) (void);
    void (*_webkit_reserved2) (void);
    void (*_webkit_reserved3) (void);
};

WEBKIT_API GType
webkit_policy_decision_get_type (void);

WEBKIT_API void
webkit_policy_decision_use      (WebKitPolicyDecision *decision);

WEBKIT_API void
webkit_policy_decision_ignore   (WebKitPolicyDecision *decision);

WEBKIT_API void
webkit_policy_decision_download (WebKitPolicyDecision *decision);

G_END_DECLS

#endif
