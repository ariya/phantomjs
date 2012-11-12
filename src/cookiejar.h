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

#ifndef COOKIEJAR_H
#define COOKIEJAR_H

#include <QSettings>
#include <QNetworkCookieJar>
#include <QVariantList>
#include <QVariantMap>

class CookieJar: public QNetworkCookieJar
{
    Q_OBJECT

private:
    CookieJar(QString cookiesFile, QObject *parent = NULL);

public:
    static CookieJar *instance(QString cookiesFile = QString());
    virtual ~CookieJar();

    bool setCookiesFromUrl(const QList<QNetworkCookie> &cookieList, const QUrl & url);
    QList<QNetworkCookie> cookiesForUrl (const QUrl & url) const;

    bool addCookie(const QNetworkCookie &cookie, const QString &url = QString());
    bool addCookieFromMap(const QVariantMap &cookie, const QString &url = QString());
    bool addCookies(const QList<QNetworkCookie> &cookiesList, const QString &url = QString());
    bool addCookiesFromMap(const QVariantList &cookiesList, const QString &url = QString());

    QList<QNetworkCookie> cookies(const QString &url = QString()) const;
    QVariantList cookiesToMap(const QString &url = QString()) const;

    QNetworkCookie cookie(const QString &name, const QString &url = QString()) const;
    QVariantMap cookieToMap(const QString &name, const QString &url = QString()) const;

    bool deleteCookie(const QString &name, const QString &url = QString());
    bool deleteCookies(const QString &url = QString());
    void clearCookies();

    void enable();
    void disable();
    bool isEnabled() const;

private slots:
    bool purgeExpiredCookies();
    bool purgeSessionCookies();
    void save();
    void load();

private:
    bool contains(const QNetworkCookie &cookie) const;

private:
    QSettings *m_cookieStorage;
    bool m_enabled;
};

#endif // COOKIEJAR_H
