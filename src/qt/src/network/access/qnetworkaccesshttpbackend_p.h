/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtNetwork module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QNETWORKACCESSHTTPBACKEND_P_H
#define QNETWORKACCESSHTTPBACKEND_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of the Network Access API.  This header file may change from
// version to version without notice, or even be removed.
//
// We mean it.
//

#include "qhttpnetworkconnection_p.h"
#include "qnetworkaccessbackend_p.h"
#include "qnetworkrequest.h"
#include "qnetworkreply.h"
#include "qabstractsocket.h"

#include "QtCore/qpointer.h"
#include "QtCore/qdatetime.h"
#include "QtCore/qsharedpointer.h"
#include "qatomic.h"

#ifndef QT_NO_HTTP

QT_BEGIN_NAMESPACE

class QNetworkAccessCachedHttpConnection;

class QNetworkAccessHttpBackendIODevice;

class QNetworkAccessHttpBackend: public QNetworkAccessBackend
{
    Q_OBJECT
public:
    QNetworkAccessHttpBackend();
    virtual ~QNetworkAccessHttpBackend();

    virtual void open();
    virtual void closeDownstreamChannel();

    virtual void downstreamReadyWrite();
    virtual void setDownstreamLimited(bool b);

    virtual void copyFinished(QIODevice *);
#ifndef QT_NO_OPENSSL
    virtual void ignoreSslErrors();
    virtual void ignoreSslErrors(const QList<QSslError> &errors);

    virtual void fetchSslConfiguration(QSslConfiguration &configuration) const;
    virtual void setSslConfiguration(const QSslConfiguration &configuration);
#endif
    QNetworkCacheMetaData fetchCacheMetaData(const QNetworkCacheMetaData &metaData) const;

    // we return true since HTTP needs to send PUT/POST data again after having authenticated
    bool needsResetableUploadData() { return true; }

    bool canResume() const;
    void setResumeOffset(quint64 offset);

signals:
    // To HTTP thread:
    void startHttpRequest();
    void abortHttpRequest();

    void startHttpRequestSynchronously();

    void haveUploadData(QByteArray dataArray, bool dataAtEnd, qint64 dataSize);
private slots:
    // From HTTP thread:
    void replyDownloadData(QByteArray);
    void replyFinished();
    void replyDownloadMetaData(QList<QPair<QByteArray,QByteArray> >,int,QString,bool,QSharedPointer<char>,qint64);
    void replyDownloadProgressSlot(qint64,qint64);
    void httpAuthenticationRequired(const QHttpNetworkRequest &request, QAuthenticator *auth);
    void httpError(QNetworkReply::NetworkError error, const QString &errorString);
#ifndef QT_NO_OPENSSL
    void replySslErrors(const QList<QSslError> &, bool *, QList<QSslError> *);
    void replySslConfigurationChanged(const QSslConfiguration&);
#endif

    // From QNonContiguousByteDeviceThreadForwardImpl in HTTP thread:
    void resetUploadDataSlot(bool *r);
    void wantUploadDataSlot(qint64);
    void sentUploadDataSlot(qint64);

    bool sendCacheContents(const QNetworkCacheMetaData &metaData);

private:
    QHttpNetworkRequest httpRequest; // There is also a copy in the HTTP thread
    int statusCode;
    QString reasonPhrase;
    // Will be increased by HTTP thread:
    QSharedPointer<QAtomicInt> pendingDownloadDataEmissions;
    QSharedPointer<QAtomicInt> pendingDownloadProgressEmissions;
    bool loadingFromCache;
    QByteDataBuffer pendingDownloadData;
    bool usingZerocopyDownloadBuffer;

#ifndef QT_NO_OPENSSL
    QSslConfiguration *pendingSslConfiguration;
    bool pendingIgnoreAllSslErrors;
    QList<QSslError> pendingIgnoreSslErrorsList;
#endif

    quint64 resumeOffset;

    bool loadFromCacheIfAllowed(QHttpNetworkRequest &httpRequest);
    void invalidateCache();
    void postRequest();
    void readFromHttp();
    void checkForRedirect(const int statusCode);
};

class QNetworkAccessHttpBackendFactory : public QNetworkAccessBackendFactory
{
public:
    virtual QNetworkAccessBackend *create(QNetworkAccessManager::Operation op,
                                          const QNetworkRequest &request) const;
};

QT_END_NAMESPACE

#endif // QT_NO_HTTP

#endif
