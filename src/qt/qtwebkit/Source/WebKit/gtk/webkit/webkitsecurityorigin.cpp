/*
 * Copyright (C) 2009 Martin Robinson, Jan Michael C. Alonzo
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
#include "webkitsecurityorigin.h"

#include "DatabaseManager.h"
#include "webkitglobalsprivate.h"
#include "webkitsecurityoriginprivate.h"
#include <glib/gi18n-lib.h>
#include <wtf/text/CString.h>
#include <wtf/text/WTFString.h>

/**
 * SECTION:webkitsecurityorigin
 * @short_description: A security boundary for web sites
 *
 * #WebKitSecurityOrigin is a representation of a security domain defined
 * by web sites. An origin consists of a host name, a protocol, and a port
 * number. Web sites with the same security origin can access each other's
 * resources for client-side scripting or database access.
 *
 * Use #webkit_web_frame_get_security_origin to get the security origin of a
 * #WebKitWebFrame.
 *
 * Database quotas and usages are also defined per security origin. The
 * cumulative disk usage of an origin's databases may be retrieved with
 * #webkit_security_origin_get_web_database_usage. An origin's quota can be
 * adjusted with #webkit_security_origin_set_web_database_quota.
 */

using namespace WebKit;

enum {
    PROP_0,

    PROP_PROTOCOL,
    PROP_HOST,
    PROP_PORT,
    PROP_DATABASE_USAGE,
    PROP_DATABASE_QUOTA
};

G_DEFINE_TYPE(WebKitSecurityOrigin, webkit_security_origin, G_TYPE_OBJECT)

static void webkit_security_origin_finalize(GObject* object)
{
    WebKitSecurityOrigin* securityOrigin = WEBKIT_SECURITY_ORIGIN(object);
    WebKitSecurityOriginPrivate* priv = securityOrigin->priv;

    g_free(priv->protocol);
    g_free(priv->host);

    G_OBJECT_CLASS(webkit_security_origin_parent_class)->finalize(object);
}

static void webkit_security_origin_dispose(GObject* object)
{
    WebKitSecurityOrigin* securityOrigin = WEBKIT_SECURITY_ORIGIN(object);
    WebKitSecurityOriginPrivate* priv = securityOrigin->priv;

    if (!priv->disposed) {
        priv->coreOrigin->deref();
        g_hash_table_destroy(priv->webDatabases);
        priv->disposed = true;
    }

    G_OBJECT_CLASS(webkit_security_origin_parent_class)->dispose(object);
}

static void webkit_security_origin_set_property(GObject* object, guint propId, const GValue* value, GParamSpec* pspec)
{
    WebKitSecurityOrigin* securityOrigin = WEBKIT_SECURITY_ORIGIN(object);

    switch (propId) {
    case PROP_DATABASE_QUOTA:
        webkit_security_origin_set_web_database_quota(securityOrigin, g_value_get_uint64(value));
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, propId, pspec);
        break;
    }
}

static void webkit_security_origin_get_property(GObject* object, guint propId, GValue* value, GParamSpec* pspec)
{
    WebKitSecurityOrigin* securityOrigin = WEBKIT_SECURITY_ORIGIN(object);

    switch (propId) {
    case PROP_PROTOCOL:
        g_value_set_string(value, webkit_security_origin_get_protocol(securityOrigin));
        break;
    case PROP_HOST:
        g_value_set_string(value, webkit_security_origin_get_host(securityOrigin));
        break;
    case PROP_PORT:
        g_value_set_uint(value, webkit_security_origin_get_port(securityOrigin));
        break;
    case PROP_DATABASE_USAGE:
        g_value_set_uint64(value, webkit_security_origin_get_web_database_usage(securityOrigin));
        break;
    case PROP_DATABASE_QUOTA:
        g_value_set_uint64(value, webkit_security_origin_get_web_database_quota(securityOrigin));
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, propId, pspec);
        break;
    }
}

static GHashTable* webkit_security_origins()
{
    static GHashTable* securityOrigins = g_hash_table_new_full(g_direct_hash, g_direct_equal, NULL, g_object_unref);
    return securityOrigins;
}

static void webkit_security_origin_class_init(WebKitSecurityOriginClass* klass)
{
    GObjectClass* gobjectClass = G_OBJECT_CLASS(klass);
    gobjectClass->dispose = webkit_security_origin_dispose;
    gobjectClass->finalize = webkit_security_origin_finalize;
    gobjectClass->set_property = webkit_security_origin_set_property;
    gobjectClass->get_property = webkit_security_origin_get_property;

     /**
      * WebKitSecurityOrigin:protocol:
      *
      * The protocol of the security origin.
      *
      * Since: 1.1.14
      */
     g_object_class_install_property(gobjectClass, PROP_PROTOCOL,
                                     g_param_spec_string("protocol",
                                                         _("Protocol"),
                                                         _("The protocol of the security origin"),
                                                         NULL,
                                                         WEBKIT_PARAM_READABLE));

     /**
      * WebKitSecurityOrigin:host:
      *
      * The host of the security origin.
      *
      * Since: 1.1.14
      */
     g_object_class_install_property(gobjectClass, PROP_HOST,
                                     g_param_spec_string("host",
                                                         _("Host"),
                                                         _("The host of the security origin"),
                                                         NULL,
                                                         WEBKIT_PARAM_READABLE));

     /**
      * WebKitSecurityOrigin:port:
      *
      * The port of the security origin.
      *
      * Since: 1.1.14
      */
     g_object_class_install_property(gobjectClass, PROP_PORT,
                                     g_param_spec_uint("port",
                                                       _("Port"),
                                                       _("The port of the security origin"),
                                                       0, G_MAXUSHORT, 0,
                                                       WEBKIT_PARAM_READABLE));

      /**
      * WebKitSecurityOrigin:web-database-usage:
      *
      * The cumulative size of all web databases in the security origin in bytes.
      *
      * Since: 1.1.14
      */
      g_object_class_install_property(gobjectClass, PROP_DATABASE_USAGE,
                                      g_param_spec_uint64("web-database-usage",
                                                          _("Web Database Usage"),
                                                          _("The cumulative size of all web databases in the security origin"),
                                                          0, G_MAXUINT64, 0,
                                                          WEBKIT_PARAM_READABLE));
      /**
      * WebKitSecurityOrigin:web-database-quota:
      *
      * The web database qouta of the security origin in bytes.
      *
      * Since: 1.1.14
      */
      g_object_class_install_property(gobjectClass, PROP_DATABASE_QUOTA,
                                      g_param_spec_uint64("web-database-quota",
                                                          _("Web Database Quota"),
                                                          _("The web database quota of the security origin in bytes"),
                                                          0, G_MAXUINT64, 0,
                                                          WEBKIT_PARAM_READWRITE));

    g_type_class_add_private(klass, sizeof(WebKitSecurityOriginPrivate));
}

static void webkit_security_origin_init(WebKitSecurityOrigin* securityOrigin)
{
    WebKitSecurityOriginPrivate* priv = G_TYPE_INSTANCE_GET_PRIVATE(securityOrigin, WEBKIT_TYPE_SECURITY_ORIGIN, WebKitSecurityOriginPrivate);
    priv->webDatabases = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_object_unref);
    securityOrigin->priv = priv;
}

/**
 * webkit_security_origin_get_protocol:
 * @securityOrigin: a #WebKitSecurityOrigin
 *
 * Returns the protocol for the security origin.
 *
 * Returns: the protocol for the security origin
 *
 * Since: 1.1.14
 **/
const gchar* webkit_security_origin_get_protocol(WebKitSecurityOrigin* securityOrigin)
{
    g_return_val_if_fail(WEBKIT_IS_SECURITY_ORIGIN(securityOrigin), NULL);

    WebKitSecurityOriginPrivate* priv = securityOrigin->priv;
    WTF::String protocol =  priv->coreOrigin->protocol();

    if (!priv->protocol)
        priv->protocol = g_strdup(protocol.utf8().data());

    return priv->protocol;
}

/**
 * webkit_security_origin_get_host:
 * @securityOrigin: a #WebKitSecurityOrigin
 *
 * Returns the hostname for the security origin.
 *
 * Returns: the hostname for the security origin
 *
 * Since: 1.1.14
 **/
const gchar* webkit_security_origin_get_host(WebKitSecurityOrigin* securityOrigin)
{
    g_return_val_if_fail(WEBKIT_IS_SECURITY_ORIGIN(securityOrigin), NULL);

    WebKitSecurityOriginPrivate* priv = securityOrigin->priv;
    WTF::String host =  priv->coreOrigin->host();

    if (!priv->host)
        priv->host = g_strdup(host.utf8().data());

    return priv->host;
}

/**
 * webkit_security_origin_get_port:
 * @securityOrigin: a #WebKitSecurityOrigin
 *
 * Returns the port for the security origin.
 *
 * Returns: the port for the security origin
 *
 * Since: 1.1.14
 **/
guint webkit_security_origin_get_port(WebKitSecurityOrigin* securityOrigin)
{
    g_return_val_if_fail(WEBKIT_IS_SECURITY_ORIGIN(securityOrigin), 0);

    WebCore::SecurityOrigin* coreOrigin = core(securityOrigin);
    return coreOrigin->port();
}

/**
 * webkit_security_origin_get_web_database_usage:
 * @securityOrigin: a #WebKitSecurityOrigin
 *
 * Returns the cumulative size of all Web Database database's in the origin
 * in bytes.
 *
 * Returns: the cumulative size of all databases
 *
 * Since: 1.1.14
 **/
guint64 webkit_security_origin_get_web_database_usage(WebKitSecurityOrigin* securityOrigin)
{
    g_return_val_if_fail(WEBKIT_IS_SECURITY_ORIGIN(securityOrigin), 0);

#if ENABLE(SQL_DATABASE)
    WebCore::SecurityOrigin* coreOrigin = core(securityOrigin);
    return WebCore::DatabaseManager::manager().usageForOrigin(coreOrigin);
#else
    return 0;
#endif
}

/**
 * webkit_security_origin_get_web_database_quota:
 * @securityOrigin: a #WebKitSecurityOrigin
 *
 * Returns the quota for Web Database storage of the security origin
 * in bytes.
 *
 * Returns: the Web Database quota
 *
 * Since: 1.1.14
 **/
guint64 webkit_security_origin_get_web_database_quota(WebKitSecurityOrigin* securityOrigin)
{
    g_return_val_if_fail(WEBKIT_IS_SECURITY_ORIGIN(securityOrigin), 0);

#if ENABLE(SQL_DATABASE)
    WebCore::SecurityOrigin* coreOrigin = core(securityOrigin);
    return WebCore::DatabaseManager::manager().quotaForOrigin(coreOrigin);
#else
    return 0;
#endif
}

/**
 * webkit_security_origin_set_web_database_quota:
 * @securityOrigin: a #WebKitSecurityOrigin
 * @quota: a new Web Database quota in bytes
 *
 * Adjust the quota for Web Database storage of the security origin
 *
 * Since: 1.1.14
 **/
void webkit_security_origin_set_web_database_quota(WebKitSecurityOrigin* securityOrigin, guint64 quota)
{
    g_return_if_fail(WEBKIT_IS_SECURITY_ORIGIN(securityOrigin));

#if ENABLE(SQL_DATABASE)
    WebCore::SecurityOrigin* coreOrigin = core(securityOrigin);
    WebCore::DatabaseManager::manager().setQuota(coreOrigin, quota);
#endif
}

/**
 * webkit_security_origin_get_all_web_databases:
 * @securityOrigin: a #WebKitSecurityOrigin
 *
 * Returns a list of all Web Databases in the security origin.
 *
 * Returns: (transfer container) (element-type WebKitWebDatabase): a
 * #GList of databases in the security origin.
 *
 * Since: 1.1.14
 **/
GList* webkit_security_origin_get_all_web_databases(WebKitSecurityOrigin* securityOrigin)
{
    g_return_val_if_fail(WEBKIT_IS_SECURITY_ORIGIN(securityOrigin), NULL);
    GList* databases = NULL;

#if ENABLE(SQL_DATABASE)
    WebCore::SecurityOrigin* coreOrigin = core(securityOrigin);
    Vector<WTF::String> databaseNames;

    if (!WebCore::DatabaseManager::manager().databaseNamesForOrigin(coreOrigin, databaseNames))
        return NULL;

    for (unsigned i = 0; i < databaseNames.size(); ++i) {
        WebKitWebDatabase* database = webkit_security_origin_get_web_database(securityOrigin, databaseNames[i].utf8().data());
        databases = g_list_append(databases, database);
    }
#endif

    return databases;
}

WebKitWebDatabase* webkit_security_origin_get_web_database(WebKitSecurityOrigin* securityOrigin, const gchar* databaseName)
{
    g_return_val_if_fail(WEBKIT_IS_SECURITY_ORIGIN(securityOrigin), NULL);

    WebKitSecurityOriginPrivate* priv = securityOrigin->priv;
    GHashTable* databaseHash = priv->webDatabases;
    WebKitWebDatabase* database = (WebKitWebDatabase*) g_hash_table_lookup(databaseHash, databaseName);

    if (!database) {
        database =  WEBKIT_WEB_DATABASE(g_object_new(WEBKIT_TYPE_WEB_DATABASE,
                                       "security-origin", securityOrigin,
                                       "name", databaseName,
                                        NULL));
        g_hash_table_insert(databaseHash, g_strdup(databaseName), database);
    }

    return database;
}

namespace WebKit {

WebCore::SecurityOrigin* core(WebKitSecurityOrigin* securityOrigin)
{
    ASSERT(securityOrigin);

    return securityOrigin->priv->coreOrigin.get();
}

WebKitSecurityOrigin* kit(WebCore::SecurityOrigin* coreOrigin)
{
    ASSERT(coreOrigin);

    GHashTable* table = webkit_security_origins();
    WebKitSecurityOrigin* origin = (WebKitSecurityOrigin*) g_hash_table_lookup(table, coreOrigin);

    if (!origin) {
        origin = WEBKIT_SECURITY_ORIGIN(g_object_new(WEBKIT_TYPE_SECURITY_ORIGIN, NULL));
        origin->priv->coreOrigin = coreOrigin;
        g_hash_table_insert(table, coreOrigin, origin);
    }

    return origin;
}

}
