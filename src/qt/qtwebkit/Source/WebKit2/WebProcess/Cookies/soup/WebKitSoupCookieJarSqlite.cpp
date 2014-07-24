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

#include "config.h"
#include "WebKitSoupCookieJarSqlite.h"

#include <WebCore/SQLiteDatabase.h>
#include <WebCore/SQLiteStatement.h>
#include <WebCore/SQLiteTransaction.h>
#include <libsoup/soup.h>
#include <wtf/CurrentTime.h>
#include <wtf/MathExtras.h>

using namespace WebCore;

struct _WebKitSoupCookieJarSqlitePrivate {
    String databasePath;
    SQLiteDatabase database;
    bool isLoading;
};

G_DEFINE_TYPE(WebKitSoupCookieJarSqlite, webkit_soup_cookie_jar_sqlite, SOUP_TYPE_COOKIE_JAR)

enum {
    ColumnID,
    ColumnName,
    ColumnValue,
    ColumnHost,
    ColumnPath,
    ColumnExpiry,
    ColumnLastAccess,
    ColumnSecure,
    ColumnHTTPOnly
};

static bool webkitSoupCookieJarSqliteOpenDatabase(WebKitSoupCookieJarSqlite* sqliteJar)
{
    WebKitSoupCookieJarSqlitePrivate* priv = sqliteJar->priv;
    if (priv->database.isOpen())
        return true;

    ASSERT(!priv->databasePath.isEmpty());
    if (!priv->database.open(priv->databasePath)) {
        g_warning("Can't open database %s", priv->databasePath.utf8().data());
        return false;
    }

    priv->database.setSynchronous(SQLiteDatabase::SyncOff);
    priv->database.executeCommand("PRAGMA secure_delete = 1;");

    return true;
}

static bool webkitSoupCookieJarSqliteCreateTable(WebKitSoupCookieJarSqlite* sqliteJar)
{
    WebKitSoupCookieJarSqlitePrivate* priv = sqliteJar->priv;
    if (priv->database.tableExists("moz_cookies"))
        return true;

    if (!priv->database.executeCommand("CREATE TABLE moz_cookies (id INTEGER PRIMARY KEY, name TEXT, value TEXT, host TEXT, path TEXT, expiry INTEGER, lastAccessed INTEGER, isSecure INTEGER, isHttpOnly INTEGER)")) {
        g_warning("Failed to create table moz_cookies: (%i) - %s", priv->database.lastError(), priv->database.lastErrorMsg());
        priv->database.close();

        return false;
    }

    return true;
}

static void webkitSoupCookieJarSqliteLoad(WebKitSoupCookieJarSqlite* sqliteJar)
{
    if (!webkitSoupCookieJarSqliteOpenDatabase(sqliteJar))
        return;
    if (!webkitSoupCookieJarSqliteCreateTable(sqliteJar))
        return;

    WebKitSoupCookieJarSqlitePrivate* priv = sqliteJar->priv;
    priv->isLoading = true;
    SQLiteStatement query(priv->database, "SELECT id, name, value, host, path, expiry, lastAccessed, isSecure, isHttpOnly FROM moz_cookies;");
    if (query.prepare() != SQLResultOk) {
        g_warning("Failed to prepare all cookies query");
        priv->isLoading = false;
        return;
    }

    SoupCookieJar* jar = SOUP_COOKIE_JAR(sqliteJar);
    time_t now = floorf(currentTime());
    int result;
    while ((result = query.step()) == SQLResultRow) {
        int expireTime = query.getColumnInt(ColumnExpiry);
        if (now >= expireTime)
            continue;

        SoupCookie* cookie = soup_cookie_new(query.getColumnText(ColumnName).utf8().data(), query.getColumnText(ColumnValue).utf8().data(),
                                             query.getColumnText(ColumnHost).utf8().data(), query.getColumnText(ColumnPath).utf8().data(),
                                             expireTime - now <= G_MAXINT ? expireTime - now : G_MAXINT);
        if (query.getColumnInt(ColumnSecure))
            soup_cookie_set_secure(cookie, TRUE);
        if (query.getColumnInt(ColumnHTTPOnly))
            soup_cookie_set_http_only(cookie, TRUE);

        soup_cookie_jar_add_cookie(jar, cookie);
    }

    if (result != SQLResultDone)
        g_warning("Error reading cookies from database");
    priv->isLoading = false;
}

static bool webkitSoupCookieJarSqliteInsertCookie(WebKitSoupCookieJarSqlite* sqliteJar, SoupCookie* cookie)
{
    WebKitSoupCookieJarSqlitePrivate* priv = sqliteJar->priv;
    SQLiteStatement query(priv->database, "INSERT INTO moz_cookies VALUES(NULL, ?, ?, ?, ?, ?, NULL, ?, ?);");
    if (query.prepare() != SQLResultOk) {
        g_warning("Failed to prepare insert cookies query");
        return false;
    }

    query.bindText(1, String::fromUTF8(cookie->name));
    query.bindText(2, String::fromUTF8(cookie->value));
    query.bindText(3, String::fromUTF8(cookie->domain));
    query.bindText(4, String::fromUTF8(cookie->path));
    query.bindInt(5, static_cast<int64_t>(soup_date_to_time_t(cookie->expires)));
    query.bindInt(6, cookie->secure);
    query.bindInt(7, cookie->http_only);
    if (query.step() != SQLResultDone) {
        g_warning("Error adding cookie (name=%s, domain=%s) to database", cookie->name, cookie->name);
        return false;
    }

    return true;
}

static bool webkitSoupCookieJarSqliteDeleteCookie(WebKitSoupCookieJarSqlite* sqliteJar, SoupCookie* cookie)
{
    WebKitSoupCookieJarSqlitePrivate* priv = sqliteJar->priv;
    SQLiteStatement query(priv->database, "DELETE FROM moz_cookies WHERE name = (?) AND host = (?);");
    if (query.prepare() != SQLResultOk) {
        g_warning("Failed to prepare delete cookies query");
        return false;
    }

    query.bindText(1, String::fromUTF8(cookie->name));
    query.bindText(2, String::fromUTF8(cookie->domain));
    if (query.step() != SQLResultDone) {
        g_warning("Error deleting cookie (name=%s, domain=%s) from database", cookie->name, cookie->name);
        return false;
    }

    return true;
}

static void webkitSoupCookieJarSqliteChanged(SoupCookieJar* jar, SoupCookie* oldCookie, SoupCookie* newCookie)
{
    WebKitSoupCookieJarSqlite* sqliteJar = WEBKIT_SOUP_COOKIE_JAR_SQLITE(jar);
    if (sqliteJar->priv->isLoading)
        return;
    if (!webkitSoupCookieJarSqliteOpenDatabase(sqliteJar))
        return;
    if (!oldCookie && (!newCookie || !newCookie->expires))
        return;
    if (!webkitSoupCookieJarSqliteCreateTable(sqliteJar))
        return;

    SQLiteTransaction updateTransaction(sqliteJar->priv->database);
    updateTransaction.begin();

    if (oldCookie && !webkitSoupCookieJarSqliteDeleteCookie(sqliteJar, oldCookie))
        return;

    if (newCookie && newCookie->expires && !webkitSoupCookieJarSqliteInsertCookie(sqliteJar, newCookie))
        return;

    updateTransaction.commit();
}

static void webkitSoupCookieJarSqliteFinalize(GObject* object)
{
    WEBKIT_SOUP_COOKIE_JAR_SQLITE(object)->priv->~WebKitSoupCookieJarSqlitePrivate();
    G_OBJECT_CLASS(webkit_soup_cookie_jar_sqlite_parent_class)->finalize(object);
}

static void webkit_soup_cookie_jar_sqlite_init(WebKitSoupCookieJarSqlite* sqliteJar)
{
    WebKitSoupCookieJarSqlitePrivate* priv = G_TYPE_INSTANCE_GET_PRIVATE(sqliteJar, WEBKIT_TYPE_SOUP_COOKIE_JAR_SQLITE, WebKitSoupCookieJarSqlitePrivate);
    sqliteJar->priv = priv;
    new (priv) WebKitSoupCookieJarSqlitePrivate();
}

static void webkit_soup_cookie_jar_sqlite_class_init(WebKitSoupCookieJarSqliteClass* sqliteJarClass)
{
    SoupCookieJarClass* cookieJarClass = SOUP_COOKIE_JAR_CLASS(sqliteJarClass);
    cookieJarClass->changed = webkitSoupCookieJarSqliteChanged;

    GObjectClass* gObjectClass = G_OBJECT_CLASS(sqliteJarClass);
    gObjectClass->finalize = webkitSoupCookieJarSqliteFinalize;

    g_type_class_add_private(sqliteJarClass, sizeof(WebKitSoupCookieJarSqlitePrivate));
}

SoupCookieJar* webkitSoupCookieJarSqliteNew(const String& databasePath)
{
    ASSERT(!databasePath.isEmpty());
    WebKitSoupCookieJarSqlite* sqliteJar = WEBKIT_SOUP_COOKIE_JAR_SQLITE(g_object_new(WEBKIT_TYPE_SOUP_COOKIE_JAR_SQLITE, NULL));
    sqliteJar->priv->databasePath = databasePath;
    webkitSoupCookieJarSqliteLoad(sqliteJar);
    return SOUP_COOKIE_JAR(sqliteJar);
}
