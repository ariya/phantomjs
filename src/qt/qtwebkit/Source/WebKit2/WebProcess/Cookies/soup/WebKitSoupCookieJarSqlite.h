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

#ifndef WebKitSoupCookieJarSqlite_h
#define WebKitSoupCookieJarSqlite_h

#include <libsoup/soup.h>
#include <wtf/text/WTFString.h>

G_BEGIN_DECLS

#define WEBKIT_TYPE_SOUP_COOKIE_JAR_SQLITE            (webkit_soup_cookie_jar_sqlite_get_type())
#define WEBKIT_SOUP_COOKIE_JAR_SQLITE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), WEBKIT_TYPE_SOUP_COOKIE_JAR_SQLITE, WebKitSoupCookieJarSqlite))
#define WEBKIT_IS_SOUP_COOKIE_JAR_SQLITE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), WEBKIT_TYPE_SOUP_COOKIE_JAR_SQLITE))
#define WEBKIT_SOUP_COOKIE_JAR_SQLITE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), WEBKIT_TYPE_SOUP_COOKIE_JAR_SQLITE, WebKitSoupCookieJarSqliteClass))
#define WEBKIT_IS_SOUP_COOKIE_JAR_SQLITE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((obj), WEBKIT_TYPE_SOUP_COOKIE_JAR_SQLITE))
#define WEBKIT_SOUP_COOKIE_JAR_SQLITE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), WEBKIT_TYPE_SOUP_COOKIE_JAR_SQLITE, WebKitSoupCookieJarSqliteClass))

typedef struct _WebKitSoupCookieJarSqlite WebKitSoupCookieJarSqlite;
typedef struct _WebKitSoupCookieJarSqliteClass WebKitSoupCookieJarSqliteClass;
typedef struct _WebKitSoupCookieJarSqlitePrivate WebKitSoupCookieJarSqlitePrivate;

struct _WebKitSoupCookieJarSqlite {
    SoupCookieJar parent;

    WebKitSoupCookieJarSqlitePrivate* priv;
};

struct _WebKitSoupCookieJarSqliteClass {
    SoupCookieJarClass parentClass;
};

GType webkit_soup_cookie_jar_sqlite_get_type();
SoupCookieJar* webkitSoupCookieJarSqliteNew(const String& databasePath);

G_END_DECLS

#endif // WebKitSoupCookieJarSqlite.h
