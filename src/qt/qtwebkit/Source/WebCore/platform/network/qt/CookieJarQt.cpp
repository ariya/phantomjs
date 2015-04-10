/*
 * Copyright (C) 2006 George Staikos <staikos@kde.org>
 * Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies)
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "CookieJarQt.h"

#include "Cookie.h"
#include "KURL.h"
#include "NetworkingContext.h"
#include "PlatformCookieJar.h"
#include "ThirdPartyCookiesQt.h"
#include <QDateTime>
#include <QNetworkAccessManager>
#include <QNetworkCookie>
#include <QSqlQuery>
#include <QStringList>
#include <QVariant>
#include <wtf/text/WTFString.h>

namespace WebCore {

static SharedCookieJarQt* s_sharedCookieJarQt = 0;

void setCookiesFromDOM(const NetworkStorageSession& session, const KURL& firstParty, const KURL& url, const String& value)
{
    QNetworkCookieJar* jar = session.context() ? session.context()->networkAccessManager()->cookieJar() : SharedCookieJarQt::shared();
    if (!jar)
        return;

    QUrl urlForCookies(url);
    QUrl firstPartyUrl(firstParty);
    if (!thirdPartyCookiePolicyPermits(session.context(), urlForCookies, firstPartyUrl))
        return;

    QList<QNetworkCookie> cookies = QNetworkCookie::parseCookies(QString(value).toLatin1());
    QList<QNetworkCookie>::Iterator it = cookies.begin();
    while (it != cookies.end()) {
        if (it->isHttpOnly())
            it = cookies.erase(it);
        else
            ++it;
    }

    jar->setCookiesFromUrl(cookies, urlForCookies);
}

String cookiesForDOM(const NetworkStorageSession& session, const KURL& firstParty, const KURL& url)
{
    QNetworkCookieJar* jar = session.context() ? session.context()->networkAccessManager()->cookieJar() : SharedCookieJarQt::shared();
    if (!jar)
        return String();

    QUrl urlForCookies(url);
    QUrl firstPartyUrl(firstParty);
    if (!thirdPartyCookiePolicyPermits(session.context(), urlForCookies, firstPartyUrl))
        return String();

    QList<QNetworkCookie> cookies = jar->cookiesForUrl(urlForCookies);
    if (cookies.isEmpty())
        return String();

    QStringList resultCookies;
    foreach (const QNetworkCookie& networkCookie, cookies) {
        if (networkCookie.isHttpOnly())
            continue;
        resultCookies.append(QString::fromLatin1(networkCookie.toRawForm(QNetworkCookie::NameAndValueOnly).constData()));
    }

    return resultCookies.join(QLatin1String("; "));
}

String cookieRequestHeaderFieldValue(const NetworkStorageSession& session, const KURL& /*firstParty*/, const KURL& url)
{
    QNetworkCookieJar* jar = session.context() ? session.context()->networkAccessManager()->cookieJar() : SharedCookieJarQt::shared();
    if (!jar)
        return String();

    QList<QNetworkCookie> cookies = jar->cookiesForUrl(QUrl(url));
    if (cookies.isEmpty())
        return String();

    QStringList resultCookies;
    foreach (QNetworkCookie networkCookie, cookies)
        resultCookies.append(QString::fromLatin1(networkCookie.toRawForm(QNetworkCookie::NameAndValueOnly).constData()));

    return resultCookies.join(QLatin1String("; "));
}

bool cookiesEnabled(const NetworkStorageSession& session, const KURL& /*firstParty*/, const KURL& /*url*/)
{
    QNetworkCookieJar* jar = session.context() ? session.context()->networkAccessManager()->cookieJar() : SharedCookieJarQt::shared();
    return !!jar;
}

bool getRawCookies(const NetworkStorageSession& session, const KURL& /*firstParty*/, const KURL& /*url*/, Vector<Cookie>& rawCookies)
{
    // FIXME: Not yet implemented
    rawCookies.clear();
    return false; // return true when implemented
}

void deleteCookie(const NetworkStorageSession&, const KURL&, const String&)
{
    // FIXME: Not yet implemented
}

void getHostnamesWithCookies(const NetworkStorageSession& session, HashSet<String>& hostnames)
{
    ASSERT_UNUSED(session, !session.context()); // Not yet implemented for cookie jars other than the shared one.
    SharedCookieJarQt* jar = SharedCookieJarQt::shared();
    if (jar)
        jar->getHostnamesWithCookies(hostnames);
}

void deleteCookiesForHostname(const NetworkStorageSession& session, const String& hostname)
{
    ASSERT_UNUSED(session, !session.context()); // Not yet implemented for cookie jars other than the shared one.
    SharedCookieJarQt* jar = SharedCookieJarQt::shared();
    if (jar)
        jar->deleteCookiesForHostname(hostname);
}

void deleteAllCookies(const NetworkStorageSession& session)
{
    ASSERT_UNUSED(session, !session.context()); // Not yet implemented for cookie jars other than the shared one.
    SharedCookieJarQt* jar = SharedCookieJarQt::shared();
    if (jar)
        jar->deleteAllCookies();
}

SharedCookieJarQt* SharedCookieJarQt::shared()
{
    return s_sharedCookieJarQt;
}

SharedCookieJarQt* SharedCookieJarQt::create(const String& cookieStorageDirectory)
{
    if (!s_sharedCookieJarQt)
        s_sharedCookieJarQt = new SharedCookieJarQt(cookieStorageDirectory);

    return s_sharedCookieJarQt;
}

void SharedCookieJarQt::destroy()
{
    delete s_sharedCookieJarQt;
    s_sharedCookieJarQt = 0;
}

void SharedCookieJarQt::getHostnamesWithCookies(HashSet<String>& hostnames)
{
    QList<QNetworkCookie> cookies = allCookies();
    foreach (const QNetworkCookie& networkCookie, cookies)
        hostnames.add(networkCookie.domain());
}

bool SharedCookieJarQt::deleteCookie(const QNetworkCookie& cookie)
{
    if (!QNetworkCookieJar::deleteCookie(cookie))
        return false;

    if (!m_database.isOpen())
        return false;

    QSqlQuery sqlQuery(m_database);
    sqlQuery.prepare(QLatin1String("DELETE FROM cookies WHERE cookieId=:cookieIdvalue"));
    sqlQuery.bindValue(QLatin1String(":cookieIdvalue"), cookie.domain().append(QLatin1String(cookie.name())));
    sqlQuery.exec();

    return true;
}

void SharedCookieJarQt::deleteCookiesForHostname(const String& hostname)
{
    if (!m_database.isOpen())
        return;

    QList<QNetworkCookie> cookies = allCookies();
    QList<QNetworkCookie>::Iterator it = cookies.begin();
    QList<QNetworkCookie>::Iterator end = cookies.end();
    QSqlQuery sqlQuery(m_database);
    sqlQuery.prepare(QLatin1String("DELETE FROM cookies WHERE cookieId=:cookieIdvalue"));
    while (it != end) {
        if (it->domain() == QString(hostname)) {
            sqlQuery.bindValue(QLatin1String(":cookieIdvalue"), it->domain().append(QLatin1String(it->name())));
            sqlQuery.exec();
            it = cookies.erase(it);
        } else
            it++;
    }
    setAllCookies(cookies);
}

void SharedCookieJarQt::deleteAllCookies()
{
    if (!m_database.isOpen())
        return;

    QSqlQuery sqlQuery(m_database);
    sqlQuery.prepare(QLatin1String("DELETE FROM cookies"));
    sqlQuery.exec();
    setAllCookies(QList<QNetworkCookie>());
}

SharedCookieJarQt::SharedCookieJarQt(const String& cookieStorageDirectory)
{
    m_database = QSqlDatabase::addDatabase(QLatin1String("QSQLITE"));
    const QString cookieStoragePath = cookieStorageDirectory;
    const QString dataBaseName = cookieStoragePath + QLatin1String("/cookies.db");
    m_database.setDatabaseName(dataBaseName);
    ensureDatabaseTable();
    loadCookies();
}

SharedCookieJarQt::~SharedCookieJarQt()
{
    m_database.close();
}

bool SharedCookieJarQt::setCookiesFromUrl(const QList<QNetworkCookie>& cookieList, const QUrl& url)
{
    if (!QNetworkCookieJar::setCookiesFromUrl(cookieList, url))
        return false;

    if (!m_database.isOpen())
        return false;

    QSqlQuery sqlQuery(m_database);
    sqlQuery.prepare(QLatin1String("INSERT OR REPLACE INTO cookies (cookieId, cookie) VALUES (:cookieIdvalue, :cookievalue)"));
    QVariantList cookiesIds;
    QVariantList cookiesValues;
    foreach (const QNetworkCookie &cookie, cookiesForUrl(url)) {
        if (cookie.isSessionCookie())
            continue;
        cookiesIds.append(cookie.domain().append(QLatin1String(cookie.name())));
        cookiesValues.append(cookie.toRawForm());
    }
    sqlQuery.bindValue(QLatin1String(":cookieIdvalue"), cookiesIds);
    sqlQuery.bindValue(QLatin1String(":cookievalue"), cookiesValues);
    sqlQuery.execBatch();
    return true;
}

void SharedCookieJarQt::ensureDatabaseTable()
{
    if (!m_database.open()) {
        qWarning("Can't open cookie database");
        return;
    }
    m_database.exec(QLatin1String("PRAGMA synchronous=OFF"));

    QSqlQuery sqlQuery(m_database);
    sqlQuery.prepare(QLatin1String("CREATE TABLE IF NOT EXISTS cookies (cookieId VARCHAR PRIMARY KEY, cookie BLOB);"));
    sqlQuery.exec();
}

void SharedCookieJarQt::loadCookies()
{
    if (!m_database.isOpen())
        return;

    QList<QNetworkCookie> cookies;
    QSqlQuery sqlQuery(m_database);
    sqlQuery.prepare(QLatin1String("SELECT cookie FROM cookies"));
    sqlQuery.exec();
    while (sqlQuery.next())
        cookies.append(QNetworkCookie::parseCookies(sqlQuery.value(0).toByteArray()));
    setAllCookies(cookies);
}

#include "moc_CookieJarQt.cpp"

}

// vim: ts=4 sw=4 et
