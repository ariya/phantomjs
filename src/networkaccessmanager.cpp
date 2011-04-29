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

#include <QDebug>
#include <QNetworkRequest>
#include <QList>
#include <QNetworkReply>
#include <QDesktopServices>
#include <QNetworkDiskCache>

#include "networkaccessmanager.h"

// public:
NetworkAccessManager::NetworkAccessManager(QObject *parent, bool diskCacheEnabled, bool ignoreSslErrors)
    : QNetworkAccessManager(parent), m_networkDiskCache(0), m_ignoreSslErrors(ignoreSslErrors)
{
    if (diskCacheEnabled) {
        m_networkDiskCache = new QNetworkDiskCache();
        m_networkDiskCache->setCacheDirectory(QDesktopServices::storageLocation(QDesktopServices::CacheLocation));
        setCache(m_networkDiskCache);
    }
    connect(this, SIGNAL(finished(QNetworkReply*)), SLOT(handleFinished(QNetworkReply*)));
}

NetworkAccessManager::~NetworkAccessManager()
{
    if (m_networkDiskCache)
        delete m_networkDiskCache;
}

// protected:
QNetworkReply *NetworkAccessManager::createRequest(Operation op, const QNetworkRequest & req, QIODevice * outgoingData)
{
    switch(op) {
    case QNetworkAccessManager::HeadOperation: {
        qDebug() << "HTTP/1.1 HEAD Request";
        break;
    }
    case QNetworkAccessManager::GetOperation: {
        qDebug() << "HTTP/1.1 GET Request";
        break;
    }
    case QNetworkAccessManager::PutOperation: {
        qDebug() << "HTTP/1.1 PUT Request";
        break;
    }
    case QNetworkAccessManager::PostOperation: {
        qDebug() << "HTTP/1.1 POST Request";
        break;
    }
#if QT_VERSION >= QT_VERSION_CHECK(4, 6, 0)
    case QNetworkAccessManager::DeleteOperation: {
        qDebug() << "HTTP/1.1 DELETE Request";
        break;
    }
#endif
#if QT_VERSION >= QT_VERSION_CHECK(4, 7, 0)
    case QNetworkAccessManager::CustomOperation: {
        qDebug() << "HTTP/1.1 CUSTOM Request";
        break;
    }
#endif
    default: {
        qWarning() << "Unexpected HTTP Operation Type";
        break;
    }
    }
    qDebug() << "URL" << qPrintable(req.url().toString());

    // Pass duty to the superclass - Nothing special to do here (yet?)
    QNetworkReply *reply = QNetworkAccessManager::createRequest(op, req, outgoingData);
    if(m_ignoreSslErrors) {
        reply->ignoreSslErrors();
    }
    return reply;
}

// private slots:
void NetworkAccessManager::handleFinished(QNetworkReply *reply)
{
    qDebug() << "HTTP/1.1 Response";
    qDebug() << "URL" << qPrintable(reply->url().toString());
    QString code = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toString();
    if (!code.isEmpty()) {
        qDebug() << "Status code:" << qPrintable(code);
    }
#if QT_VERSION >= QT_VERSION_CHECK(4, 7, 0)
    QList<QNetworkReply::RawHeaderPair> headerPairs = reply->rawHeaderPairs();
    foreach ( QNetworkReply::RawHeaderPair pair, headerPairs ) {
        qDebug() << pair.first << "=" << pair.second;
    }
#endif
}
