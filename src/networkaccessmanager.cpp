/*
  This file is part of the PhantomJS project from Ofi Labs.

  Copyright (C) 2011 Ariya Hidayat <ariya.hidayat@gmail.com>
  Copyright (C) 2011 Ivan De Marino <ivan.de.marino@gmail.com>

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

#include <QAuthenticator>
#include <QDateTime>
#include <QDesktopServices>
#include <QNetworkDiskCache>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QSslSocket>

#include "phantom.h"
#include "config.h"
#include "cookiejar.h"
#include "networkaccessmanager.h"

static const char *toString(QNetworkAccessManager::Operation op)
{
    const char *str = 0;
    switch (op) {
    case QNetworkAccessManager::HeadOperation:
        str = "HEAD";
        break;
    case QNetworkAccessManager::GetOperation:
        str = "GET";
        break;
    case QNetworkAccessManager::PutOperation:
        str = "PUT";
        break;
    case QNetworkAccessManager::PostOperation:
        str = "POST";
        break;
    case QNetworkAccessManager::DeleteOperation:
        str = "DELETE";
        break;
    default:
        str = "?";
        break;
    }
    return str;
}

// public:
NetworkAccessManager::NetworkAccessManager(QObject *parent, const Config *config)
    : QNetworkAccessManager(parent)
    , m_ignoreSslErrors(config->ignoreSslErrors())
    , m_idCounter(0)
    , m_networkDiskCache(0)
{
    setCookieJar(CookieJar::instance());

    if (config->diskCacheEnabled()) {
        m_networkDiskCache = new QNetworkDiskCache(this);
        m_networkDiskCache->setCacheDirectory(QDesktopServices::storageLocation(QDesktopServices::CacheLocation));
        if (config->maxDiskCacheSize() >= 0)
            m_networkDiskCache->setMaximumCacheSize(config->maxDiskCacheSize() * 1024);
        setCache(m_networkDiskCache);
    }

    connect(this, SIGNAL(authenticationRequired(QNetworkReply*,QAuthenticator*)), SLOT(provideAuthentication(QNetworkReply*,QAuthenticator*)));
    connect(this, SIGNAL(finished(QNetworkReply*)), SLOT(handleFinished(QNetworkReply*)));
}

void NetworkAccessManager::setUserName(const QString &userName)
{
    m_userName = userName;
}

void NetworkAccessManager::setPassword(const QString &password)
{
    m_password = password;
}

void NetworkAccessManager::setCustomHeaders(const QVariantMap &headers)
{
    m_customHeaders = headers;
}

QVariantMap NetworkAccessManager::customHeaders() const
{
    return m_customHeaders;
}

void NetworkAccessManager::setCookieJar(QNetworkCookieJar *cookieJar)
{
    QNetworkAccessManager::setCookieJar(cookieJar);
    // Remove NetworkAccessManager's ownership of this CookieJar and
    // pass it to the PhantomJS Singleton object.
    // CookieJar is a SINGLETON, shouldn't be deleted when
    // the NetworkAccessManager is deleted but only when we shutdown.
    cookieJar->setParent(Phantom::instance());
}

// protected:
QNetworkReply *NetworkAccessManager::createRequest(Operation op, const QNetworkRequest & request, QIODevice * outgoingData)
{
    QNetworkRequest req(request);

    if (!QSslSocket::supportsSsl()) {
        if (req.url().scheme().toLower() == QLatin1String("https"))
            qWarning() << "Request using https scheme without SSL support";
    }

    // Get the URL string before calling the superclass. Seems to work around
    // segfaults in Qt 4.8: https://gist.github.com/1430393
    QByteArray url = req.url().toEncoded();

    // http://code.google.com/p/phantomjs/issues/detail?id=337
    if (op == QNetworkAccessManager::PostOperation) {
        QString contentType = req.header(QNetworkRequest::ContentTypeHeader).toString();
        if (contentType.isEmpty()) {
            req.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
        }
    }

    // set custom HTTP headers
    QVariantMap::const_iterator i = m_customHeaders.begin();
    while (i != m_customHeaders.end()) {
        req.setRawHeader(i.key().toAscii(), i.value().toByteArray());
        ++i;
    }

    // Pass duty to the superclass - Nothing special to do here (yet?)
    QNetworkReply *reply = QNetworkAccessManager::createRequest(op, req, outgoingData);
    if(m_ignoreSslErrors) {
        reply->ignoreSslErrors();
    }

    QVariantList headers;
    foreach (QByteArray headerName, req.rawHeaderList()) {
        QVariantMap header;
        header["name"] = QString::fromUtf8(headerName);
        header["value"] = QString::fromUtf8(req.rawHeader(headerName));
        headers += header;
    }

    m_idCounter++;
    m_ids[reply] = m_idCounter;

    QVariantMap data;
    data["id"] = m_idCounter;
    data["url"] = url.data();
    data["method"] = toString(op);
    data["headers"] = headers;
    data["time"] = QDateTime::currentDateTime();

    connect(reply, SIGNAL(readyRead()), this, SLOT(handleStarted()));

    emit resourceRequested(data);
    return reply;
}

void NetworkAccessManager::handleStarted()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply)
        return;
    if (m_started.contains(reply))
        return;

    m_started += reply;

    QVariantList headers;
    foreach (QByteArray headerName, reply->rawHeaderList()) {
        QVariantMap header;
        header["name"] = QString::fromUtf8(headerName);
        header["value"] = QString::fromUtf8(reply->rawHeader(headerName));
        headers += header;
    }

    QVariantMap data;
    data["stage"] = "start";
    data["id"] = m_ids.value(reply);
    data["url"] = reply->url().toEncoded().data();
    data["status"] = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute);
    data["statusText"] = reply->attribute(QNetworkRequest::HttpReasonPhraseAttribute);
    data["contentType"] = reply->header(QNetworkRequest::ContentTypeHeader);
    data["bodySize"] = reply->size();
    data["redirectURL"] = reply->header(QNetworkRequest::LocationHeader);
    data["headers"] = headers;
    data["time"] = QDateTime::currentDateTime();

    emit resourceReceived(data);
}

void NetworkAccessManager::handleFinished(QNetworkReply *reply)
{
    QVariantList headers;
    foreach (QByteArray headerName, reply->rawHeaderList()) {
        QVariantMap header;
        header["name"] = QString::fromUtf8(headerName);
        header["value"] = QString::fromUtf8(reply->rawHeader(headerName));
        headers += header;
    }

    QVariantMap data;
    data["stage"] = "end";
    data["id"] = m_ids.value(reply);
    data["url"] = reply->url().toEncoded().data();
    data["status"] = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute);
    data["statusText"] = reply->attribute(QNetworkRequest::HttpReasonPhraseAttribute);
    data["contentType"] = reply->header(QNetworkRequest::ContentTypeHeader);
    data["redirectURL"] = reply->header(QNetworkRequest::LocationHeader);
    data["headers"] = headers;
    data["time"] = QDateTime::currentDateTime();

    m_ids.remove(reply);
    m_started.remove(reply);

    emit resourceReceived(data);
}

void NetworkAccessManager::provideAuthentication(QNetworkReply *reply, QAuthenticator *authenticator)
{
    Q_UNUSED(reply);
    authenticator->setUser(m_userName);
    authenticator->setPassword(m_password);
}
