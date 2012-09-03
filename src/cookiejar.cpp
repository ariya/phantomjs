/*
  This file is part of the PhantomJS project from Ofi Labs.

  Copyright (C) 2011 Ariya Hidayat <ariya.hidayat@gmail.com>

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

#include "config.h"
#include "cookiejar.h"

#include <QDateTime>
#include <QSettings>
#include <QStringList>

CookieJar::CookieJar(QString cookiesFile)
        : QNetworkCookieJar()
{
    m_cookiesFile = cookiesFile;
}

bool CookieJar::setCookiesFromUrl(const QList<QNetworkCookie> & cookieList, const QUrl & url)
{
    QSettings settings(m_cookiesFile, QSettings::IniFormat);

    settings.beginGroup(url.host());
    
    for (QList<QNetworkCookie>::const_iterator i = cookieList.begin(); i != cookieList.end(); i++) {
        settings.setValue((*i).name(), QString((*i).value()));
    }

    settings.sync();

    // updating cookies from the server.
    QNetworkCookieJar::setCookiesFromUrl(cookieList, url);

    return true;
}

QList<QNetworkCookie> CookieJar::cookiesForUrl(const QUrl & url) const
{
    QSettings settings(m_cookiesFile, QSettings::IniFormat);
    QList<QNetworkCookie> cookieList;

    settings.beginGroup(url.host());

    QStringList keys = settings.childKeys();
    
    for (QStringList::iterator i = keys.begin(); i != keys.end(); i++) {
        cookieList.push_back(QNetworkCookie((*i).toLocal8Bit(), settings.value(*i).toByteArray()));
    }
    
    // sending cookies to the server.
    QList<QNetworkCookie> allCookies = QNetworkCookieJar::cookiesForUrl(url);
    for (QList<QNetworkCookie>::const_iterator i = allCookies.begin(); i != allCookies.end(); i++) {
        cookieList.push_back((*i));
    }

    return cookieList;
}

void CookieJar::setCookies(const QVariantList &cookies)
{
    QList<QNetworkCookie> newCookies;
    for (int i = 0; i < cookies.size(); ++i) {
        QNetworkCookie nc;
        QVariantMap cookie = cookies.at(i).toMap();

        //
        // The field of domain and cookie name/value MUST be set, otherwise skip it.
        //
        if (cookie["domain"].isNull() || cookie["domain"].toString().isEmpty()
            || cookie["name"].isNull() || cookie["name"].toString().isEmpty()
            || cookie["value"].isNull()
        ) {
            continue;
        } else {
            nc.setDomain(cookie["domain"].toString());
            nc.setName(cookie["name"].toByteArray());
            nc.setValue(cookie["value"].toByteArray());
        }

        if (cookie["path"].isNull() || cookie["path"].toString().isEmpty()) {
            nc.setPath("/");
        } else {
            nc.setPath(cookie["path"].toString());
        }

        if (cookie["httponly"].isNull()) {
            nc.setHttpOnly(false);
        } else {
            nc.setHttpOnly(cookie["httponly"].toBool());
        }

        if (cookie["secure"].isNull()) {
            nc.setSecure(false);
        } else {
            nc.setSecure(cookie["secure"].toBool());
        }

        if (!cookie["expires"].isNull()) {
            QString datetime = cookie["expires"].toString().replace(" GMT", "");
            QDateTime expires = QDateTime::fromString(datetime, "ddd, dd MMM yyyy hh:mm:ss");
            if (expires.isValid()) {
                nc.setExpirationDate(expires);
            }
        }

        newCookies.append(nc);
    }

    this->setAllCookies(newCookies);
}

QVariantList CookieJar::cookies() const
{
    QVariantList returnCookies;
    QList<QNetworkCookie> allCookies = this->allCookies();
    for (QList<QNetworkCookie>::const_iterator i = allCookies.begin(); i != allCookies.end(); i++) {
        QVariantMap cookie;

        cookie["domain"] = QVariant((*i).domain());
        cookie["name"] = QVariant(QString((*i).name()));
        cookie["value"] = QVariant(QString((*i).value()));

        if ((*i).path().isNull() || (*i).path().isEmpty()) {
            cookie["path"] = QVariant("/");
        } else {
            cookie["path"] = QVariant((*i).path());
        }

        if ((*i).isHttpOnly()) {
            cookie["httponly"] = QVariant(true);
        } else {
            cookie["httponly"] = QVariant(false);
        }

        if ((*i).isSecure()) {
            cookie["secure"] = QVariant(true);
        } else {
            cookie["secure"] = QVariant(false);
        }

        if ((*i).expirationDate().isValid()) {
            cookie["expires"] = QVariant(QString((*i).expirationDate().toString("ddd, dd MMM yyyy hh:mm:ss")).append(" GMT"));
        }

        returnCookies.append(cookie);
    }

    return returnCookies;
}
