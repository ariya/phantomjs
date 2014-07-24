/*
 * Copyright (C) 2012 Igalia S.L.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2,1 of the License, or (at your option) any later version.
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

#ifndef WebKitSecurityManager_h
#define WebKitSecurityManager_h

#include <glib-object.h>
#include <webkit2/WebKitDefines.h>

G_BEGIN_DECLS

#define WEBKIT_TYPE_SECURITY_MANAGER            (webkit_security_manager_get_type())
#define WEBKIT_SECURITY_MANAGER(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj), WEBKIT_TYPE_SECURITY_MANAGER, WebKitSecurityManager))
#define WEBKIT_IS_SECURITY_MANAGER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj), WEBKIT_TYPE_SECURITY_MANAGER))
#define WEBKIT_SECURITY_MANAGER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass),  WEBKIT_TYPE_SECURITY_MANAGER, WebKitSecurityManagerClass))
#define WEBKIT_IS_SECURITY_MANAGER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass),  WEBKIT_TYPE_SECURITY_MANAGER))
#define WEBKIT_SECURITY_MANAGER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj),  WEBKIT_TYPE_SECURITY_MANAGER, WebKitSecurityManagerClass))

typedef struct _WebKitSecurityManager        WebKitSecurityManager;
typedef struct _WebKitSecurityManagerClass   WebKitSecurityManagerClass;
typedef struct _WebKitSecurityManagerPrivate WebKitSecurityManagerPrivate;

struct _WebKitSecurityManager {
    GObject parent;

    WebKitSecurityManagerPrivate *priv;
};

struct _WebKitSecurityManagerClass {
    GObjectClass parent_class;

    void (*_webkit_reserved0) (void);
    void (*_webkit_reserved1) (void);
    void (*_webkit_reserved2) (void);
    void (*_webkit_reserved3) (void);
};

WEBKIT_API GType
webkit_security_manager_get_type                                (void);

WEBKIT_API void
webkit_security_manager_register_uri_scheme_as_local            (WebKitSecurityManager *security_manager,
                                                                 const gchar           *scheme);

WEBKIT_API gboolean
webkit_security_manager_uri_scheme_is_local                     (WebKitSecurityManager *security_manager,
                                                                 const gchar           *scheme);

WEBKIT_API void
webkit_security_manager_register_uri_scheme_as_no_access        (WebKitSecurityManager *security_manager,
                                                                 const gchar           *scheme);

WEBKIT_API gboolean
webkit_security_manager_uri_scheme_is_no_access                 (WebKitSecurityManager *security_manager,
                                                                 const gchar           *scheme);

WEBKIT_API void
webkit_security_manager_register_uri_scheme_as_display_isolated (WebKitSecurityManager *security_manager,
                                                                 const gchar           *scheme);

WEBKIT_API gboolean
webkit_security_manager_uri_scheme_is_display_isolated          (WebKitSecurityManager *security_manager,
                                                                 const gchar           *scheme);

WEBKIT_API void
webkit_security_manager_register_uri_scheme_as_secure           (WebKitSecurityManager *security_manager,
                                                                 const gchar           *scheme);

WEBKIT_API gboolean
webkit_security_manager_uri_scheme_is_secure                    (WebKitSecurityManager *security_manager,
                                                                 const gchar           *scheme);

WEBKIT_API void
webkit_security_manager_register_uri_scheme_as_cors_enabled     (WebKitSecurityManager *security_manager,
                                                                 const gchar           *scheme);

WEBKIT_API gboolean
webkit_security_manager_uri_scheme_is_cors_enabled              (WebKitSecurityManager *security_manager,
                                                                 const gchar           *scheme);

WEBKIT_API void
webkit_security_manager_register_uri_scheme_as_empty_document   (WebKitSecurityManager *security_manager,
                                                                 const gchar           *scheme);

WEBKIT_API gboolean
webkit_security_manager_uri_scheme_is_empty_document            (WebKitSecurityManager *security_manager,
                                                                 const gchar           *scheme);

G_END_DECLS

#endif
