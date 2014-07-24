/*
 * Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies)
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
 *
 */

#ifndef CookieJarQt_h
#define CookieJarQt_h

#include <QtCore/QObject>
#include <QtNetwork/QNetworkCookieJar>
#include <QtSql/QSqlDatabase>

#include <wtf/HashSet.h>
#include <wtf/text/WTFString.h>

namespace WebCore {

class SharedCookieJarQt : public QNetworkCookieJar {
    Q_OBJECT
public:
    static SharedCookieJarQt* shared();
    static SharedCookieJarQt* create(const String&);
    void destroy();

    void getHostnamesWithCookies(HashSet<String>&);
    bool deleteCookie(const QNetworkCookie&);
    void deleteCookiesForHostname(const String&);
    void deleteAllCookies();
    bool setCookiesFromUrl(const QList<QNetworkCookie>&, const QUrl&);
    void loadCookies();

private:
    SharedCookieJarQt(const String&);
    ~SharedCookieJarQt();
    void ensureDatabaseTable();

    QSqlDatabase m_database;
};

}

#endif
