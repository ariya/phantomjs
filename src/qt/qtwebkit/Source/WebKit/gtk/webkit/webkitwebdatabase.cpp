/*
 * Copyright (C) 2009 Martin Robinson
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
#include "webkitwebdatabase.h"

#include "DatabaseDetails.h"
#include "DatabaseManager.h"
#include "FileSystem.h"
#include "GroupSettings.h"
#include "PageGroup.h"
#include "webkitglobalsprivate.h"
#include "webkitsecurityoriginprivate.h"
#include <glib/gi18n-lib.h>
#include <wtf/text/CString.h>

/**
 * SECTION:webkitwebdatabase
 * @short_description: A WebKit web application database
 *
 * #WebKitWebDatabase is a representation of a Web Database database. The
 * proposed Web Database standard introduces support for SQL databases that web
 * sites can create and access on a local computer through JavaScript.
 *
 * To get access to all databases defined by a security origin, use
 * #webkit_security_origin_get_databases. Each database has a canonical
 * name, as well as a user-friendly display name.
 *
 * WebKit uses SQLite to create and access the local SQL databases. The location
 * of a #WebKitWebDatabase can be accessed wth #webkit_web_database_get_filename.
 * You can configure the location of all databases with
 * #webkit_set_database_directory_path.
 *
 * For each database the web site can define an estimated size which can be
 * accessed with #webkit_web_database_get_expected_size. The current size of the
 * database in bytes is returned by #webkit_web_database_get_size.
 *
 * For more information refer to the Web Database specification proposal at
 * http://dev.w3.org/html5/webdatabase
 */

using namespace WebKit;

enum {
    PROP_0,

    PROP_SECURITY_ORIGIN,
    PROP_NAME,
    PROP_DISPLAY_NAME,
    PROP_EXPECTED_SIZE,
    PROP_SIZE,
    PROP_PATH
};

G_DEFINE_TYPE(WebKitWebDatabase, webkit_web_database, G_TYPE_OBJECT)

struct _WebKitWebDatabasePrivate {
    WebKitSecurityOrigin* origin;
    gchar* name;
    gchar* displayName;
    gchar* filename;
};

static CString gWebKitWebDatabasePath;
static guint64 webkit_default_database_quota = 5 * 1024 * 1024;

static void webkit_web_database_set_security_origin(WebKitWebDatabase* webDatabase, WebKitSecurityOrigin* security_origin);

static void webkit_web_database_set_name(WebKitWebDatabase* webDatabase, const gchar* name);

static void webkit_web_database_finalize(GObject* object)
{
    WebKitWebDatabase* webDatabase = WEBKIT_WEB_DATABASE(object);
    WebKitWebDatabasePrivate* priv = webDatabase->priv;

    g_free(priv->name);
    g_free(priv->displayName);
    g_free(priv->filename);

    G_OBJECT_CLASS(webkit_web_database_parent_class)->finalize(object);
}

static void webkit_web_database_dispose(GObject* object)
{
    WebKitWebDatabase* webDatabase = WEBKIT_WEB_DATABASE(object);
    WebKitWebDatabasePrivate* priv = webDatabase->priv;

    if (priv->origin) {
        g_object_unref(priv->origin);
        priv->origin = NULL;
    }

    G_OBJECT_CLASS(webkit_web_database_parent_class)->dispose(object);
}

static void webkit_web_database_set_property(GObject* object, guint propId, const GValue* value, GParamSpec* pspec)
{
    WebKitWebDatabase* webDatabase = WEBKIT_WEB_DATABASE(object);

    switch (propId) {
    case PROP_SECURITY_ORIGIN:
        webkit_web_database_set_security_origin(webDatabase, WEBKIT_SECURITY_ORIGIN(g_value_get_object(value)));
        break;
    case PROP_NAME:
        webkit_web_database_set_name(webDatabase, g_value_get_string(value));
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, propId, pspec);
        break;
    }
}

static void webkit_web_database_get_property(GObject* object, guint propId, GValue* value, GParamSpec* pspec)
{
    WebKitWebDatabase* webDatabase = WEBKIT_WEB_DATABASE(object);
    WebKitWebDatabasePrivate* priv = webDatabase->priv;

    switch (propId) {
    case PROP_SECURITY_ORIGIN:
        g_value_set_object(value, priv->origin);
        break;
    case PROP_NAME:
        g_value_set_string(value, webkit_web_database_get_name(webDatabase));
        break;
    case PROP_DISPLAY_NAME:
        g_value_set_string(value, webkit_web_database_get_display_name(webDatabase));
        break;
    case PROP_EXPECTED_SIZE:
        g_value_set_uint64(value, webkit_web_database_get_expected_size(webDatabase));
        break;
    case PROP_SIZE:
        g_value_set_uint64(value, webkit_web_database_get_size(webDatabase));
        break;
    case PROP_PATH:
        g_value_set_string(value, webkit_web_database_get_filename(webDatabase));
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, propId, pspec);
        break;
    }
}

static void webkit_web_database_class_init(WebKitWebDatabaseClass* klass)
{
    GObjectClass* gobjectClass = G_OBJECT_CLASS(klass);
    gobjectClass->dispose = webkit_web_database_dispose;
    gobjectClass->finalize = webkit_web_database_finalize;
    gobjectClass->set_property = webkit_web_database_set_property;
    gobjectClass->get_property = webkit_web_database_get_property;

     /**
      * WebKitWebDatabase:security-origin:
      *
      * The security origin of the database.
      *
      * Since: 1.1.14
      */
     g_object_class_install_property(gobjectClass, PROP_SECURITY_ORIGIN,
                                     g_param_spec_object("security-origin",
                                                         _("Security Origin"),
                                                         _("The security origin of the database"),
                                                         WEBKIT_TYPE_SECURITY_ORIGIN,
                                                         (GParamFlags) (G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY)));

     /**
      * WebKitWebDatabase:name:
      *
      * The name of the Web Database database.
      *
      * Since: 1.1.14
      */
     g_object_class_install_property(gobjectClass, PROP_NAME,
                                     g_param_spec_string("name",
                                                         _("Name"),
                                                         _("The name of the Web Database database"),
                                                         NULL,
                                                         (GParamFlags) (G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY)));

     /**
      * WebKitWebDatabase:display-name:
      *
      * The display name of the Web Database database.
      *
      * Since: 1.1.14
      */
     g_object_class_install_property(gobjectClass, PROP_DISPLAY_NAME,
                                     g_param_spec_string("display-name",
                                                         _("Display Name"),
                                                         _("The display name of the Web Storage database"),
                                                         NULL,
                                                         WEBKIT_PARAM_READABLE));

     /**
     * WebKitWebDatabase:expected-size:
     *
     * The expected size of the database in bytes as defined by the web author.
     *
     * Since: 1.1.14
     */
     g_object_class_install_property(gobjectClass, PROP_EXPECTED_SIZE,
                                     g_param_spec_uint64("expected-size",
                                                         _("Expected Size"),
                                                         _("The expected size of the Web Database database"),
                                                         0, G_MAXUINT64, 0,
                                                         WEBKIT_PARAM_READABLE));
     /**
     * WebKitWebDatabase:size:
     *
     * The current size of the database in bytes.
     *
     * Since: 1.1.14
     */
     g_object_class_install_property(gobjectClass, PROP_SIZE,
                                     g_param_spec_uint64("size",
                                                         _("Size"),
                                                         _("The current size of the Web Database database"),
                                                         0, G_MAXUINT64, 0,
                                                         WEBKIT_PARAM_READABLE));
     /**
      * WebKitWebDatabase:filename:
      *
      * The absolute filename of the Web Database database.
      *
      * Since: 1.1.14
      */
     g_object_class_install_property(gobjectClass, PROP_PATH,
                                     g_param_spec_string("filename",
                                                         _("Filename"),
                                                         _("The absolute filename of the Web Storage database"),
                                                         NULL,
                                                         WEBKIT_PARAM_READABLE));

    g_type_class_add_private(klass, sizeof(WebKitWebDatabasePrivate));
}

static void webkit_web_database_init(WebKitWebDatabase* webDatabase)
{
    webDatabase->priv = G_TYPE_INSTANCE_GET_PRIVATE(webDatabase, WEBKIT_TYPE_WEB_DATABASE, WebKitWebDatabasePrivate);
}

// Internal use only
static void webkit_web_database_set_security_origin(WebKitWebDatabase *webDatabase, WebKitSecurityOrigin *securityOrigin)
{
    g_return_if_fail(WEBKIT_IS_WEB_DATABASE(webDatabase));
    g_return_if_fail(WEBKIT_IS_SECURITY_ORIGIN(securityOrigin));

    WebKitWebDatabasePrivate* priv = webDatabase->priv;

    if (priv->origin)
        g_object_unref(priv->origin);

    g_object_ref(securityOrigin);
    priv->origin = securityOrigin;
}

static void webkit_web_database_set_name(WebKitWebDatabase* webDatabase, const gchar* name)
{
    g_return_if_fail(WEBKIT_IS_WEB_DATABASE(webDatabase));

    WebKitWebDatabasePrivate* priv = webDatabase->priv;
    g_free(priv->name);
    priv->name = g_strdup(name);
}

/**
 * webkit_web_database_get_security_origin:
 * @webDatabase: a #WebKitWebDatabase
 *
 * Returns the security origin of the #WebKitWebDatabase.
 *
 * Returns: (transfer none): the security origin of the database
 *
 * Since: 1.1.14
 **/
WebKitSecurityOrigin* webkit_web_database_get_security_origin(WebKitWebDatabase* webDatabase)
{
    g_return_val_if_fail(WEBKIT_IS_WEB_DATABASE(webDatabase), NULL);
    WebKitWebDatabasePrivate* priv = webDatabase->priv;

    return priv->origin;
}

/**
 * webkit_web_database_get_name:
 * @webDatabase: a #WebKitWebDatabase
 *
 * Returns the canonical name of the #WebKitWebDatabase.
 *
 * Returns: the name of the database
 *
 * Since: 1.1.14
 **/
const gchar* webkit_web_database_get_name(WebKitWebDatabase* webDatabase)
{
    g_return_val_if_fail(WEBKIT_IS_WEB_DATABASE(webDatabase), NULL);
    WebKitWebDatabasePrivate* priv = webDatabase->priv;

    return priv->name;
}

/**
 * webkit_web_database_get_display_name:
 * @webDatabase: a #WebKitWebDatabase
 *
 * Returns the name of the #WebKitWebDatabase as seen by the user.
 *
 * Returns: the name of the database as seen by the user.
 *
 * Since: 1.1.14
 **/
const gchar* webkit_web_database_get_display_name(WebKitWebDatabase* webDatabase)
{
    g_return_val_if_fail(WEBKIT_IS_WEB_DATABASE(webDatabase), NULL);

#if ENABLE(SQL_DATABASE)
    WebKitWebDatabasePrivate* priv = webDatabase->priv;
    WebCore::DatabaseDetails details = WebCore::DatabaseManager::manager().detailsForNameAndOrigin(priv->name, core(priv->origin));
    WTF::String displayName =  details.displayName();

    if (displayName.isEmpty())
        return "";

    g_free(priv->displayName);
    priv->displayName = g_strdup(displayName.utf8().data());
    return priv->displayName;
#else
    return "";
#endif
}

/**
 * webkit_web_database_get_expected_size:
 * @webDatabase: a #WebKitWebDatabase
 *
 * Returns the expected size of the #WebKitWebDatabase in bytes as defined by the
 * web author. The Web Database standard allows web authors to specify an expected
 * size of the database to optimize the user experience.
 *
 * Returns: the expected size of the database in bytes
 *
 * Since: 1.1.14
 **/
guint64 webkit_web_database_get_expected_size(WebKitWebDatabase* webDatabase)
{
    g_return_val_if_fail(WEBKIT_IS_WEB_DATABASE(webDatabase), 0);

#if ENABLE(SQL_DATABASE)
    WebKitWebDatabasePrivate* priv = webDatabase->priv;
    WebCore::DatabaseDetails details = WebCore::DatabaseManager::manager().detailsForNameAndOrigin(priv->name, core(priv->origin));
    return details.expectedUsage();
#else
    return 0;
#endif
}

/**
 * webkit_web_database_get_size:
 * @webDatabase: a #WebKitWebDatabase
 *
 * Returns the actual size of the #WebKitWebDatabase space on disk in bytes.
 *
 * Returns: the actual size of the database in bytes
 *
 * Since: 1.1.14
 **/
guint64 webkit_web_database_get_size(WebKitWebDatabase* webDatabase)
{
    g_return_val_if_fail(WEBKIT_IS_WEB_DATABASE(webDatabase), 0);

#if ENABLE(SQL_DATABASE)
    WebKitWebDatabasePrivate* priv = webDatabase->priv;
    WebCore::DatabaseDetails details = WebCore::DatabaseManager::manager().detailsForNameAndOrigin(priv->name, core(priv->origin));
    return details.currentUsage();
#else
    return 0;
#endif
}

/**
 * webkit_web_database_get_filename:
 * @webDatabase: a #WebKitWebDatabase
 *
 * Returns the absolute filename to the #WebKitWebDatabase file on disk.
 *
 * Returns: the absolute filename of the database
 *
 * Since: 1.1.14
 **/
const gchar* webkit_web_database_get_filename(WebKitWebDatabase* webDatabase)
{
    g_return_val_if_fail(WEBKIT_IS_WEB_DATABASE(webDatabase), NULL);

#if ENABLE(SQL_DATABASE)
    WebKitWebDatabasePrivate* priv = webDatabase->priv;
    WTF::String coreName = WTF::String::fromUTF8(priv->name);
    WTF::String corePath = WebCore::DatabaseManager::manager().fullPathForDatabase(core(priv->origin), coreName);

    if (corePath.isEmpty())
        return"";

    g_free(priv->filename);
    priv->filename = g_strdup(corePath.utf8().data());
    return priv->filename;

#else
    return "";
#endif
}

/**
 * webkit_web_database_remove:
 * @webDatabase: a #WebKitWebDatabase
 *
 * Removes the #WebKitWebDatabase from its security origin and destroys all data
 * stored in the database.
 *
 * Since: 1.1.14
 **/
void webkit_web_database_remove(WebKitWebDatabase* webDatabase)
{
    g_return_if_fail(WEBKIT_IS_WEB_DATABASE(webDatabase));

#if ENABLE(SQL_DATABASE)
    WebKitWebDatabasePrivate* priv = webDatabase->priv;
    WebCore::DatabaseManager::manager().deleteDatabase(core(priv->origin), priv->name);
#endif
}

/**
 * webkit_remove_all_web_databases:
 *
 * Removes all web databases from the current database directory path.
 *
 * Since: 1.1.14
 **/
void webkit_remove_all_web_databases()
{
#if ENABLE(SQL_DATABASE)
    WebCore::DatabaseManager::manager().deleteAllDatabases();
#endif
}

/**
 * webkit_get_web_database_directory_path:
 *
 * Returns the current path to the directory WebKit will write Web
 * Database and Indexed Database databases. By default this path will
 * be in the user data directory.
 *
 * Returns: the current database directory path in the filesystem encoding
 *
 * Since: 1.1.14
 **/
const gchar* webkit_get_web_database_directory_path()
{
    return gWebKitWebDatabasePath.data();
}

/**
 * webkit_set_web_database_directory_path:
 * @path: the new database directory path in the filesystem encoding
 *
 * Sets the current path to the directory WebKit will write Web
 * Database and Indexed Database databases.
 *
 * Since: 1.1.14
 **/
void webkit_set_web_database_directory_path(const gchar* path)
{
    gWebKitWebDatabasePath = path;

    String pathString = WebCore::filenameToString(path);
#if ENABLE(SQL_DATABASE)
    WebCore::DatabaseManager::manager().setDatabaseDirectoryPath(pathString);
#endif

#if ENABLE(INDEXED_DATABASE)
    WebCore::PageGroup::pageGroup(webkitPageGroupName())->groupSettings()->setIndexedDBDatabasePath(pathString);
#endif

}

/**
 * webkit_get_default_web_database_quota:
 *
 * Returns the default quota for Web Database databases. By default
 * this value is 5MB.
 *
 * Returns: the current default database quota in bytes
 *
 * Since: 1.1.14
 **/
guint64 webkit_get_default_web_database_quota()
{
    return webkit_default_database_quota;
}

/**
 * webkit_set_default_web_database_quota:
 * @defaultQuota: the new default database quota
 *
 * Sets the default quota for Web Database databases.
 *
 * Since: 1.1.14
 **/
void webkit_set_default_web_database_quota(guint64 defaultQuota)
{
    webkit_default_database_quota = defaultQuota;
}
