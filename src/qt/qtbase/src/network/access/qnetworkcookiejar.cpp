/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtNetwork module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qnetworkcookiejar.h"
#include "qnetworkcookiejar_p.h"

#include "QtNetwork/qnetworkcookie.h"
#include "QtCore/qurl.h"
#include "QtCore/qdatetime.h"
#include "private/qtldurl_p.h"

QT_BEGIN_NAMESPACE

/*!
    \class QNetworkCookieJar
    \since 4.4
    \inmodule QtNetwork

    \brief The QNetworkCookieJar class implements a simple jar of QNetworkCookie objects

    Cookies are small bits of information that stateless protocols
    like HTTP use to maintain some persistent information across
    requests.

    A cookie is set by a remote server when it replies to a request
    and it expects the same cookie to be sent back when further
    requests are sent.

    The cookie jar is the object that holds all cookies set in
    previous requests. Web browsers save their cookie jars to disk in
    order to conserve permanent cookies across invocations of the
    application.

    QNetworkCookieJar does not implement permanent storage: it only
    keeps the cookies in memory. Once the QNetworkCookieJar object is
    deleted, all cookies it held will be discarded as well. If you
    want to save the cookies, you should derive from this class and
    implement the saving to disk to your own storage format.

    This class implements only the basic security recommended by the
    cookie specifications and does not implement any cookie acceptance
    policy (it accepts all cookies set by any requests). In order to
    override those rules, you should reimplement the
    cookiesForUrl() and setCookiesFromUrl() virtual
    functions. They are called by QNetworkReply and
    QNetworkAccessManager when they detect new cookies and when they
    require cookies.

    \sa QNetworkCookie, QNetworkAccessManager, QNetworkReply,
    QNetworkRequest, QNetworkAccessManager::setCookieJar()
*/

/*!
    Creates a QNetworkCookieJar object and sets the parent object to
    be \a parent.

    The cookie jar is initialized to empty.
*/
QNetworkCookieJar::QNetworkCookieJar(QObject *parent)
    : QObject(*new QNetworkCookieJarPrivate, parent)
{
}

/*!
    Destroys this cookie jar object and discards all cookies stored in
    it. Cookies are not saved to disk in the QNetworkCookieJar default
    implementation.

    If you need to save the cookies to disk, you have to derive from
    QNetworkCookieJar and save the cookies to disk yourself.
*/
QNetworkCookieJar::~QNetworkCookieJar()
{
}

/*!
    Returns all cookies stored in this cookie jar. This function is
    suitable for derived classes to save cookies to disk, as well as
    to implement cookie expiration and other policies.

    \sa setAllCookies(), cookiesForUrl()
*/
QList<QNetworkCookie> QNetworkCookieJar::allCookies() const
{
    return d_func()->allCookies;
}

/*!
    Sets the internal list of cookies held by this cookie jar to be \a
    cookieList. This function is suitable for derived classes to
    implement loading cookies from permanent storage, or their own
    cookie acceptance policies by reimplementing
    setCookiesFromUrl().

    \sa allCookies(), setCookiesFromUrl()
*/
void QNetworkCookieJar::setAllCookies(const QList<QNetworkCookie> &cookieList)
{
    Q_D(QNetworkCookieJar);
    d->allCookies = cookieList;
}

static inline bool isParentPath(QString path, QString reference)
{
    if (path.startsWith(reference)) {
        //The cookie-path and the request-path are identical.
        if (path.length() == reference.length())
            return true;
        //The cookie-path is a prefix of the request-path, and the last
        //character of the cookie-path is %x2F ("/").
        if (reference.endsWith('/'))
            return true;
        //The cookie-path is a prefix of the request-path, and the first
        //character of the request-path that is not included in the cookie-
        //path is a %x2F ("/") character.
        if (path.at(reference.length()) == '/')
            return true;
    }
    return false;
}

static inline bool isParentDomain(QString domain, QString reference)
{
    if (!reference.startsWith(QLatin1Char('.')))
        return domain == reference;

    return domain.endsWith(reference) || domain == reference.mid(1);
}

/*!
    Adds the cookies in the list \a cookieList to this cookie
    jar. Before being inserted cookies are normalized.

    Returns \c true if one or more cookies are set for \a url,
    otherwise false.

    If a cookie already exists in the cookie jar, it will be
    overridden by those in \a cookieList.

    The default QNetworkCookieJar class implements only a very basic
    security policy (it makes sure that the cookies' domain and path
    match the reply's). To enhance the security policy with your own
    algorithms, override setCookiesFromUrl().

    Also, QNetworkCookieJar does not have a maximum cookie jar
    size. Reimplement this function to discard older cookies to create
    room for new ones.

    \sa cookiesForUrl(), QNetworkAccessManager::setCookieJar(), QNetworkCookie::normalize()
*/
bool QNetworkCookieJar::setCookiesFromUrl(const QList<QNetworkCookie> &cookieList,
                                          const QUrl &url)
{
    bool added = false;
    foreach (QNetworkCookie cookie, cookieList) {
        cookie.normalize(url);
        if (validateCookie(cookie, url) && insertCookie(cookie))
            added = true;
    }
    return added;
}

/*!
    Returns the cookies to be added to when a request is sent to
    \a url. This function is called by the default
    QNetworkAccessManager::createRequest(), which adds the
    cookies returned by this function to the request being sent.

    If more than one cookie with the same name is found, but with
    differing paths, the one with longer path is returned before the
    one with shorter path. In other words, this function returns
    cookies sorted decreasingly by path length.

    The default QNetworkCookieJar class implements only a very basic
    security policy (it makes sure that the cookies' domain and path
    match the reply's). To enhance the security policy with your own
    algorithms, override cookiesForUrl().

    \sa setCookiesFromUrl(), QNetworkAccessManager::setCookieJar()
*/
QList<QNetworkCookie> QNetworkCookieJar::cookiesForUrl(const QUrl &url) const
{
//     \b Warning! This is only a dumb implementation!
//     It does NOT follow all of the recommendations from
//     http://wp.netscape.com/newsref/std/cookie_spec.html
//     It does not implement a very good cross-domain verification yet.

    Q_D(const QNetworkCookieJar);
    QDateTime now = QDateTime::currentDateTime();
    QList<QNetworkCookie> result;
    bool isEncrypted = url.scheme().toLower() == QLatin1String("https");

    // scan our cookies for something that matches
    QList<QNetworkCookie>::ConstIterator it = d->allCookies.constBegin(),
                                        end = d->allCookies.constEnd();
    for ( ; it != end; ++it) {
        if (!isParentDomain(url.host(), it->domain()))
            continue;
        if (!isParentPath(url.path(), it->path()))
            continue;
        if (!(*it).isSessionCookie() && (*it).expirationDate() < now)
            continue;
        if ((*it).isSecure() && !isEncrypted)
            continue;

        // insert this cookie into result, sorted by path
        QList<QNetworkCookie>::Iterator insertIt = result.begin();
        while (insertIt != result.end()) {
            if (insertIt->path().length() < it->path().length()) {
                // insert here
                insertIt = result.insert(insertIt, *it);
                break;
            } else {
                ++insertIt;
            }
        }

        // this is the shortest path yet, just append
        if (insertIt == result.end())
            result += *it;
    }

    return result;
}

/*!
    \since 5.0
    Adds \a cookie to this cookie jar.

    Returns \c true if \a cookie was added, false otherwise.

    If a cookie with the same identifier already exists in the
    cookie jar, it will be overridden.
*/
bool QNetworkCookieJar::insertCookie(const QNetworkCookie &cookie)
{
    Q_D(QNetworkCookieJar);
    QDateTime now = QDateTime::currentDateTime();
    bool isDeletion = !cookie.isSessionCookie() &&
                      cookie.expirationDate() < now;

    deleteCookie(cookie);

    if (!isDeletion) {
        d->allCookies += cookie;
        return true;
    }
    return false;
}

/*!
    \since 5.0
    If a cookie with the same identifier as \a cookie exists in this cookie jar
    it will be updated. This function uses insertCookie().

    Returns \c true if \a cookie was updated, false if no cookie in the jar matches
    the identifier of \a cookie.

    \sa QNetworkCookie::hasSameIdentifier()
*/
bool QNetworkCookieJar::updateCookie(const QNetworkCookie &cookie)
{
    if (deleteCookie(cookie))
        return insertCookie(cookie);
    return false;
}

/*!
    \since 5.0
    Deletes from cookie jar the cookie found to have the same identifier as \a cookie.

    Returns \c true if a cookie was deleted, false otherwise.

    \sa QNetworkCookie::hasSameIdentifier()
*/
bool QNetworkCookieJar::deleteCookie(const QNetworkCookie &cookie)
{
    Q_D(QNetworkCookieJar);
    QList<QNetworkCookie>::Iterator it;
    for (it = d->allCookies.begin(); it != d->allCookies.end(); it++)
        if (it->hasSameIdentifier(cookie)) {
            d->allCookies.erase(it);
            return true;
        }
    return false;
}

/*!
    \since 5.0
    Returns \c true if the domain and path of \a cookie are valid, false otherwise.
    The \a url parameter is used to determine if the domain specified in the cookie
    is allowed.
*/
bool QNetworkCookieJar::validateCookie(const QNetworkCookie &cookie, const QUrl &url) const
{
    QString domain = cookie.domain();
    if (!(isParentDomain(domain, url.host()) || isParentDomain(url.host(), domain)))
        return false; // not accepted

    // the check for effective TLDs makes the "embedded dot" rule from RFC 2109 section 4.3.2
    // redundant; the "leading dot" rule has been relaxed anyway, see QNetworkCookie::normalize()
    // we remove the leading dot for this check if it's present
    if (qIsEffectiveTLD(domain.startsWith('.') ? domain.remove(0, 1) : domain))
        return false; // not accepted

    return true;
}

QT_END_NAMESPACE
