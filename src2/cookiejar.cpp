/*
  This file is part of the PhantomJS project from Ofi Labs.

  Copyright (C) 2011 Ariya Hidayat <ariya.hidayat@gmail.com>
  Copyright (C) 2012 Ivan De Marino <ivan.de.marino@gmail.com>

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of the <organization> nor the
      names of its contributors may be used to endorse or promote products
      derived from this software without specific prior written permission.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
  ARE DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
  DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
  (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
  ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
  THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "phantom.h"
#include "config.h"
#include "cookiejar.h"

#include <QDateTime>
#include <QSettings>
#include <QTimer>

#define COOKIE_JAR_VERSION      1

// Operators needed for Cookie Serialization
QT_BEGIN_NAMESPACE
QDataStream &operator<<(QDataStream &stream, const QList<QNetworkCookie> &list)
{
    stream << COOKIE_JAR_VERSION;
    stream << quint32(list.size());
    for (int i = 0; i < list.size(); ++i)
        stream << list.at(i).toRawForm();
    return stream;
}

QDataStream &operator>>(QDataStream &stream, QList<QNetworkCookie> &list)
{
    list.clear();

    quint32 version;
    stream >> version;

    if (version != COOKIE_JAR_VERSION)
        return stream;

    quint32 count;
    stream >> count;
    for(quint32 i = 0; i < count; ++i)
    {
        QByteArray value;
        stream >> value;
        QList<QNetworkCookie> newCookies = QNetworkCookie::parseCookies(value);
        if (newCookies.count() == 0 && value.length() != 0) {
            qWarning() << "CookieJar: Unable to parse saved cookie:" << value;
        }
        for (int j = 0; j < newCookies.count(); ++j)
            list.append(newCookies.at(j));
        if (stream.atEnd())
            break;
    }
    return stream;
}
QT_END_NAMESPACE

// private:
CookieJar::CookieJar(QString cookiesFile, QObject *parent)
    : QNetworkCookieJar(parent)
    , m_cookieStorage(new QSettings(cookiesFile, QSettings::IniFormat, this))
    , m_enabled(true)
{
    load();
}

// public:
CookieJar *CookieJar::instance(QString cookiesFile)
{
    static CookieJar *singleton = NULL;
    if (!singleton) {
        if (cookiesFile.isEmpty()) {
            qDebug() << "CookieJar - Created but will not store cookies (use option '--cookies-file=<filename>' to enable persisten cookie storage)";
        } else {
            qDebug() << "CookieJar - Created and will store cookies in:" << cookiesFile;
        }
        // Create singleton and assign ownershipt to the Phantom singleton object
        // NOTE: First time this is done is when we set "once and for all" the Cookies' File
        singleton = new CookieJar(cookiesFile, Phantom::instance());
    }
    return singleton;
}

CookieJar::~CookieJar()
{
    // On destruction, before saving, clear all the session cookies
    purgeSessionCookies();
    save();
}

bool CookieJar::setCookiesFromUrl(const QList<QNetworkCookie> & cookieList, const QUrl &url)
{
    // Update cookies in memory
    if (isEnabled()) {
        QNetworkCookieJar::setCookiesFromUrl(cookieList, url);
        save();
    }
    // No changes occurred
    return false;
}

QList<QNetworkCookie> CookieJar::cookiesForUrl(const QUrl &url) const
{
    if (isEnabled()) {
        return QNetworkCookieJar::cookiesForUrl(url);
    }
    // The CookieJar is disabled: don't return any cookie
    return QList<QNetworkCookie>();
}

bool CookieJar::addCookie(const QNetworkCookie &cookie, const QString &url)
{
    if (isEnabled() && (!url.isEmpty() || !cookie.domain().isEmpty())) {
        // Save a single cookie
        setCookiesFromUrl(
            QList<QNetworkCookie>() << cookie, //< unfortunately, "setCookiesFromUrl" requires a list
            !url.isEmpty() ?
                url :           //< use given URL
                QString(        //< mock-up a URL
                    (cookie.isSecure() ? "https://" : "http://") +                              //< URL protocol
                    QString(cookie.domain().startsWith('.') ? "www" : "") + cookie.domain() +   //< URL domain
                    (cookie.path().isEmpty() ? "/" : cookie.path())));                          //< URL path

        // Return "true" if the cookie was really set
        if (contains(cookie)) {
            return true;
        }
        qDebug() << "CookieJar - Rejected Cookie" << cookie.toRawForm();
        return false;
    }
    return false;
}

bool CookieJar::addCookieFromMap(const QVariantMap &cookie, const QString &url)
{
    QNetworkCookie newCookie;

    // The cookie must have a non-empty "name" and a "value"
    if (!cookie["name"].isNull() && !cookie["name"].toString().isEmpty() && !cookie["value"].isNull()) {
        // Name & Value
        newCookie.setName(cookie["name"].toByteArray());
        newCookie.setValue(cookie["value"].toByteArray());

        // Domain, if provided
        if (!cookie["domain"].isNull() && !cookie["domain"].toString().isEmpty()) {
            newCookie.setDomain(cookie["domain"].toString());
        }

        // Path, if provided
        if (!cookie["path"].isNull() || !cookie["path"].toString().isEmpty()) {
            newCookie.setPath(cookie["path"].toString());
        }

        // HttpOnly, false by default
        newCookie.setHttpOnly(cookie["httponly"].isNull() ? false : cookie["httponly"].toBool());
        // Secure, false by default
        newCookie.setSecure(cookie["secure"].isNull() ? false : cookie["secure"].toBool());

        // Expiration Date, if provided, giving priority to "expires" over "expiry"
        QVariant expiresVar;
        if (!cookie["expires"].isNull()) {
            expiresVar = cookie["expires"];
        } else if (!cookie["expiry"].isNull()) {
            expiresVar = cookie["expiry"];
        }

        if (expiresVar.isValid()) {
            QDateTime expirationDate;
            if (expiresVar.type() == QVariant::String) {
                // Set cookie expire date via "classic" string format
                QString datetime = expiresVar.toString().replace(" GMT", "");
                expirationDate = QDateTime::fromString(datetime, "ddd, dd MMM yyyy hh:mm:ss");
            } else if (expiresVar.type() == QVariant::Double){
                // Set cookie expire date via "number of seconds since epoch"
                // NOTE: Every JS number is a Double.
                // @see http://www.ecma-international.org/publications/files/ECMA-ST/Ecma-262.pdf
                expirationDate = QDateTime::fromMSecsSinceEpoch(expiresVar.toLongLong());
            }

            if (expirationDate.isValid()) {
                newCookie.setExpirationDate(expirationDate);
            }
        }

        return addCookie(newCookie, url);
    }
    return false;
}

bool CookieJar::addCookies(const QList<QNetworkCookie> &cookiesList, const QString &url)
{
    bool added = false;
    for (int i = cookiesList.length() -1; i >=0; --i) {
        if (addCookie(cookiesList.at(i), url)) {
            // change it to "true" if at least 1 cookie was set
            added = true;
        }
    }
    return added;
}

bool CookieJar::addCookiesFromMap(const QVariantList &cookiesList, const QString &url)
{
    bool added = false;
    for (int i = cookiesList.length() -1; i >= 0; --i) {
        if (addCookieFromMap(cookiesList.at(i).toMap(), url)) {
            // change it to "true" if at least 1 cookie was set
            added = true;
        }
    }
    return added;
}

QList<QNetworkCookie> CookieJar::cookies(const QString &url) const
{
    if (url.isEmpty()) {
        // No url provided: return all the cookies in this CookieJar
        return allCookies();
    } else {
        // Return ONLY the cookies that match this URL
        return cookiesForUrl(url);
    }
}

QVariantList CookieJar::cookiesToMap(const QString &url) const
{
    QVariantList result;
    QNetworkCookie c;
    QVariantMap cookie;

    QList<QNetworkCookie> cookiesList = cookies(url);
    for (int i = cookiesList.length() -1; i >= 0; --i) {
        c = cookiesList.at(i);

        cookie["domain"] = QVariant(c.domain());
        cookie["name"] = QVariant(QString(c.name()));
        cookie["value"] = QVariant(QString(c.value()));
        cookie["path"] = (c.path().isNull() || c.path().isEmpty()) ? QVariant("/") : QVariant(c.path());
        cookie["httponly"] = QVariant(c.isHttpOnly());
        cookie["secure"] = QVariant(c.isSecure());
        if (c.expirationDate().isValid()) {
            cookie["expires"] = QVariant(QString(c.expirationDate().toString("ddd, dd MMM yyyy hh:mm:ss")).append(" GMT"));
            cookie["expiry"] = QVariant(c.expirationDate().toMSecsSinceEpoch() / 1000);
        }

        result.append(cookie);
    }

    return result;
}

QNetworkCookie CookieJar::cookie(const QString &name, const QString &url) const
{
    QList<QNetworkCookie> cookiesList = cookies(url);
    for (int i = cookiesList.length() -1; i >= 0; --i) {
        if (cookiesList.at(i).name() == name) {
            return cookiesList.at(i);
        }
    }
    return QNetworkCookie();
}

QVariantMap CookieJar::cookieToMap(const QString &name, const QString &url) const
{
    QVariantMap cookie;

    QVariantList cookiesList = cookiesToMap(url);
    for (int i = cookiesList.length() -1; i >= 0; --i) {
        cookie = cookiesList.at(i).toMap();
        if (cookie["name"].toString() == name) {
            return cookie;
        }
    }
    return QVariantMap();
}

bool CookieJar::deleteCookie(const QString &name, const QString &url)
{
    bool deleted = false;
    if (isEnabled()) {

        // NOTE: This code has been written in an "extended form" to make it
        // easy to understand. Surely this could be "shrinked", but it
        // would probably look uglier.

        QList<QNetworkCookie> cookiesListAll;

        if (url.isEmpty()) {
            if (name.isEmpty()) {           //< Neither "name" or "url" provided
                // This method has been used wrong:
                // "redirecting" to the right method for the job
                clearCookies();
            } else {                        //< Only "name" provided
                // Delete all cookies with the given name from the CookieJar
                cookiesListAll = allCookies();
                for (int i = cookiesListAll.length() -1; i >= 0; --i) {
                    if (cookiesListAll.at(i).name() == name) {
                        // Remove this cookie
                        qDebug() << "CookieJar - Deleted" << cookiesListAll.at(i).toRawForm();
                        cookiesListAll.removeAt(i);
                        deleted = true;
                    }
                }
            }
        } else {
            // Delete cookie(s) from the ones visible to the given "url".
            // Use the "name" to delete only the right one, otherwise all of them.
            QList<QNetworkCookie> cookiesListUrl = cookies(url);
            cookiesListAll = allCookies();
            for (int i = cookiesListAll.length() -1; i >= 0; --i) {
                if (cookiesListUrl.contains(cookiesListAll.at(i)) &&            //< if it part of the set of cookies visible at URL
                    (cookiesListAll.at(i).name() == name || name.isEmpty())) {  //< and if the name matches, or no name provided
                    // Remove this cookie
                    qDebug() << "CookieJar - Deleted" << cookiesListAll.at(i).toRawForm();
                    cookiesListAll.removeAt(i);
                    deleted = true;

                    if (!name.isEmpty()) {
                        // Only one cookie was supposed to be deleted: we are done here!
                        break;
                    }
                }
            }
        }

        // Put back the remaining cookies
        setAllCookies(cookiesListAll);
    }
    return deleted;
}

bool CookieJar::deleteCookies(const QString &url)
{
    if (isEnabled()) {
        if (url.isEmpty()) {
            // No URL provided: delete ALL the cookies in the CookieJar
            clearCookies();
            return true;
        }

        // No cookie name provided: delete all the cookies visible by this URL
        qDebug() << "Delete all cookies for URL:" << url;
        return deleteCookie("", url);
    }
    return false;
}

void CookieJar::clearCookies()
{
    if (isEnabled()) {
        setAllCookies(QList<QNetworkCookie>());
    }
}

void CookieJar::enable()
{
    m_enabled = true;
}

void CookieJar::disable()
{
    m_enabled = false;
}

bool CookieJar::isEnabled() const
{
    return m_enabled;
}

// private:
bool CookieJar::purgeExpiredCookies()
{
    QList<QNetworkCookie> cookiesList = allCookies();

    // If empty, there is nothing to purge
    if (cookiesList.isEmpty()) {
        return false;
    }

    // Check if any cookie has expired
    int prePurgeCookiesCount = cookiesList.count();
    QDateTime now = QDateTime::currentDateTime();
    for (int i = cookiesList.count() - 1; i >= 0; --i) {
        if (!cookiesList.at(i).isSessionCookie() && cookiesList.at(i).expirationDate() < now) {
            qDebug() << "CookieJar - Purged (expired)" << cookiesList.at(i).toRawForm();
            cookiesList.removeAt(i);
        }
    }

    // Set cookies and returns "true" if at least 1 cookie expired and has been removed
    if (prePurgeCookiesCount != cookiesList.count()) {
        setAllCookies(cookiesList);
        return true;
    }
    return false;
}

bool CookieJar::purgeSessionCookies()
{
    QList<QNetworkCookie> cookiesList = allCookies();

    // If empty, there is nothing to purge
    if (cookiesList.isEmpty()) {
        return false;
    }

    // Check if any cookie has expired
    int prePurgeCookiesCount = cookiesList.count();
    for (int i = cookiesList.count() - 1; i >= 0; --i) {
        if (cookiesList.at(i).isSessionCookie() || !cookiesList.at(i).expirationDate().isValid() || cookiesList.at(i).expirationDate().isNull()) {
            qDebug() << "CookieJar - Purged (session)" << cookiesList.at(i).toRawForm();
            cookiesList.removeAt(i);
        }
    }

    // Set cookies and returns "true" if at least 1 session cookie was found and removed
    if (prePurgeCookiesCount != cookiesList.count()) {
        setAllCookies(cookiesList);
        return true;
    }
    return false;
}

void CookieJar::save()
{
    if (isEnabled()) {
        // Get rid of all the Cookies that have expired
        purgeExpiredCookies();

#ifndef QT_NO_DEBUG_OUTPUT
        foreach (QNetworkCookie cookie, allCookies()) {
            qDebug() << "CookieJar - Saved" << cookie.toRawForm();
        }
#endif

        // Store cookies
        m_cookieStorage->setValue(QLatin1String("cookies"), QVariant::fromValue<QList<QNetworkCookie> >(allCookies()));
    }
}

void CookieJar::load()
{
    if (isEnabled()) {
        // Register a "StreamOperator" for this Meta Type, so we can easily serialize/deserialize the cookies
        qRegisterMetaTypeStreamOperators<QList<QNetworkCookie> >("QList<QNetworkCookie>");

        // Load all the cookies
        setAllCookies(qvariant_cast<QList<QNetworkCookie> >(m_cookieStorage->value(QLatin1String("cookies"))));

        // If any cookie has expired since last execution, purge and save before going any further
        if (purgeExpiredCookies()) {
            save();
        }

#ifndef QT_NO_DEBUG_OUTPUT
        foreach (QNetworkCookie cookie, allCookies()) {
            qDebug() << "CookieJar - Loaded" << cookie.toRawForm();
        }
#endif
    }
}

bool CookieJar::contains(const QNetworkCookie &cookie) const
{
    QList<QNetworkCookie> cookiesList = allCookies();
    for (int i = cookiesList.length() -1; i >= 0; --i) {
        if (cookie.name() == cookiesList.at(i).name() &&
            cookie.value() == cookiesList.at(i).value() &&
            (cookie.domain().isEmpty() || cookiesList.at(i).domain().prepend('.').endsWith(cookie.domain())) &&
            (cookie.path().isEmpty() || cookiesList.at(i).path() == cookie.path()) &&
            cookie.isSecure() == cookiesList.at(i).isSecure() &&
            cookie.isHttpOnly() == cookiesList.at(i).isHttpOnly() &&
            cookie.expirationDate().toMSecsSinceEpoch() == cookiesList.at(i).expirationDate().toMSecsSinceEpoch()
            ) {
            return true;
        }
    }

    return false;
}
