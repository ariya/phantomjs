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

#ifndef webkitgeolocationpolicydecision_h
#define webkitgeolocationpolicydecision_h

#include <glib-object.h>
#include <webkit/webkitdefines.h>

G_BEGIN_DECLS

#define WEBKIT_TYPE_GEOLOCATION_POLICY_DECISION            (webkit_geolocation_policy_decision_get_type())
#define WEBKIT_GEOLOCATION_POLICY_DECISION(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj), WEBKIT_TYPE_GEOLOCATION_POLICY_DECISION, WebKitGeolocationPolicyDecision))
#define WEBKIT_GEOLOCATION_POLICY_DECISION_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass),  WEBKIT_TYPE_GEOLOCATION_POLICY_DECISION, WebKitGeolocationPolicyDecisionClass))
#define WEBKIT_IS_GEOLOCATION_POLICY_DECISION(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj), WEBKIT_TYPE_GEOLOCATION_POLICY_DECISION))
#define WEBKIT_IS_GEOLOCATION_POLICY_DECISION_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass),  WEBKIT_TYPE_GEOLOCATION_POLICY_DECISION))
#define WEBKIT_GEOLOCATION_POLICY_DECISION_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj),  WEBKIT_TYPE_GEOLOCATION_POLICY_DECISION, WebKitGeolocationPolicyDecisionClass))

typedef struct _WebKitGeolocationPolicyDecisionPrivate WebKitGeolocationPolicyDecisionPrivate;
struct _WebKitGeolocationPolicyDecision {
    GObject parent_instance;

    /*< private >*/
    WebKitGeolocationPolicyDecisionPrivate* priv;
};

struct _WebKitGeolocationPolicyDecisionClass {
    GObjectClass parent_class;

    /* Padding for future expansion */
    void (*_webkit_reserved0) (void);
    void (*_webkit_reserved1) (void);
    void (*_webkit_reserved2) (void);
    void (*_webkit_reserved3) (void);
};

WEBKIT_API GType
webkit_geolocation_policy_decision_get_type (void);

WEBKIT_API void
webkit_geolocation_policy_allow (WebKitGeolocationPolicyDecision* decision);

WEBKIT_API void
webkit_geolocation_policy_deny (WebKitGeolocationPolicyDecision* decision);

G_END_DECLS

#endif
