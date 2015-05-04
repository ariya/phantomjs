/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtNetwork module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia. For licensing terms and
** conditions see http://qt.digia.com/licensing. For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights. These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QNETWORKREPLYHTTPIMPL_P_H
#define QNETWORKREPLYHTTPIMPL_P_H

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

#include "qnetworkrequest.h"
#include "qnetworkreply.h"

#include "QtCore/qpointer.h"
#include "QtCore/qdatetime.h"
#include "QtCore/qsharedpointer.h"
#include "qatomic.h"

#include <QtNetwork/QNetworkCacheMetaData>
#include <private/qhttpnetworkrequest_p.h>
#include <private/qbytedata_p.h>
#include <private/qnetworkreply_p.h>
#include <QtNetwork/QNetworkProxy>
#include <QtNetwork/QNetworkSession>

#ifndef QT_NO_SSL
#include <QtNetwork/QSslConfiguration>
#endif

#ifndef QT_NO_HTTP

QT_BEGIN_NAMESPACE

class QIODevice;

class QNetworkReplyHttpImplPrivate;
class QNetworkReplyHttpImpl: public QNetworkReply
{
    Q_OBJECT
public:
    QNetworkReplyHttpImpl(QNetworkAccessManager* const, const QNetworkRequest&, QNetworkAccessManager::Operation&, QIODevice* outgoingData);
    virtual ~QNetworkReplyHttpImpl();

    void close();
    void abort();
    qint64 bytesAvailable() const;
    bool isSequential () const;
    qint64 size() const;
    qint64 readData(char*, qint64);
    void setReadBufferSize(qint64 size);
    bool canReadLine () const;

    Q_DECLARE_PRIVATE(QNetworkReplyHttpImpl)
    Q_PRIVATE_SLOT(d_func(), void _q_startOperation())
    Q_PRIVATE_SLOT(d_func(), void _q_cacheLoadReadyRead())
    Q_PRIVATE_SLOT(d_func(), void _q_bufferOutgoingData())
    Q_PRIVATE_SLOT(d_func(), void _q_bufferOutgoingDataFinished())
#ifndef QT_NO_BEARERMANAGEMENT
    Q_PRIVATE_SLOT(d_func(), void _q_networkSessionConnected())
    Q_PRIVATE_SLOT(d_func(), void _q_networkSessionFailed())
    Q_PRIVATE_SLOT(d_func(), void _q_networkSessionUsagePoliciesChanged(QNetworkSession::UsagePolicies))
#endif
    Q_PRIVATE_SLOT(d_func(), void _q_finished())
    Q_PRIVATE_SLOT(d_func(), void _q_error(QNetworkReply::NetworkError, const QString &))

    // From reply
    Q_PRIVATE_SLOT(d_func(), void replyDownloadData(QByteArray))
    Q_PRIVATE_SLOT(d_func(), void replyFinished())
    Q_PRIVATE_SLOT(d_func(), void replyDownloadMetaData(QList<QPair<QByteArray,QByteArray> >,
                                                        int, QString, bool, QSharedPointer<char>,
                                                        qint64, bool))
    Q_PRIVATE_SLOT(d_func(), void replyDownloadProgressSlot(qint64,qint64))
    Q_PRIVATE_SLOT(d_func(), void httpAuthenticationRequired(const QHttpNetworkRequest &, QAuthenticator *))
    Q_PRIVATE_SLOT(d_func(), void httpError(QNetworkReply::NetworkError, const QString &))
#ifndef QT_NO_SSL
    Q_PRIVATE_SLOT(d_func(), void replyEncrypted())
    Q_PRIVATE_SLOT(d_func(), void replySslErrors(const QList<QSslError> &, bool *, QList<QSslError> *))
    Q_PRIVATE_SLOT(d_func(), void replySslConfigurationChanged(const QSslConfiguration&))
#endif
#ifndef QT_NO_NETWORKPROXY
    Q_PRIVATE_SLOT(d_func(), void proxyAuthenticationRequired(const QNetworkProxy &proxy, QAuthenticator *auth))
#endif

    Q_PRIVATE_SLOT(d_func(), void resetUploadDataSlot(bool *r))
    Q_PRIVATE_SLOT(d_func(), void wantUploadDataSlot(qint64))
    Q_PRIVATE_SLOT(d_func(), void sentUploadDataSlot(qint64))
    Q_PRIVATE_SLOT(d_func(), void uploadByteDeviceReadyReadSlot())
    Q_PRIVATE_SLOT(d_func(), void emitReplyUploadProgress(qint64, qint64))
    Q_PRIVATE_SLOT(d_func(), void _q_cacheSaveDeviceAboutToClose())
    Q_PRIVATE_SLOT(d_func(), void _q_metaDataChanged())


#ifndef QT_NO_SSL
protected:
    void ignoreSslErrors();
    void ignoreSslErrorsImplementation(const QList<QSslError> &errors);
    void setSslConfigurationImplementation(const QSslConfiguration &configuration);
    void sslConfigurationImplementation(QSslConfiguration &configuration) const;
#endif

signals:
    // To HTTP thread:
    void startHttpRequest();
    void abortHttpRequest();
    void readBufferSizeChanged(qint64 size);
    void readBufferFreed(qint64 size);

    void startHttpRequestSynchronously();

    void haveUploadData(QByteArray dataArray, bool dataAtEnd, qint64 dataSize);
};

class QNetworkReplyHttpImplPrivate: public QNetworkReplyPrivate
{
public:

    static QHttpNetworkRequest::Priority convert(const QNetworkRequest::Priority& prio);

    QNetworkReplyHttpImplPrivate();
    ~QNetworkReplyHttpImplPrivate();

    bool start();
    void _q_startOperation();

    void _q_cacheLoadReadyRead();

    void _q_bufferOutgoingData();
    void _q_bufferOutgoingDataFinished();

    void _q_cacheSaveDeviceAboutToClose();

#ifndef QT_NO_BEARERMANAGEMENT
    void _q_networkSessionConnected();
    void _q_networkSessionFailed();
    void _q_networkSessionUsagePoliciesChanged(QNetworkSession::UsagePolicies);
#endif
    void _q_finished();

    void finished();
    void error(QNetworkReply::NetworkError code, const QString &errorString);
    void _q_error(QNetworkReply::NetworkError code, const QString &errorString);
    void _q_metaDataChanged();

    void checkForRedirect(const int statusCode);

    // incoming from user
    QNetworkAccessManager *manager;
    QNetworkAccessManagerPrivate *managerPrivate;
    QHttpNetworkRequest httpRequest; // There is also a copy in the HTTP thread
    bool synchronous;

    State state;

    // from http thread
    int statusCode;
    QString reasonPhrase;

    // upload
    QNonContiguousByteDevice* createUploadByteDevice();
    QSharedPointer<QNonContiguousByteDevice> uploadByteDevice;
    bool uploadDeviceChoking; // if we couldn't readPointer() any data at the moment
    QIODevice *outgoingData;
    QSharedPointer<QRingBuffer> outgoingDataBuffer;
    void emitReplyUploadProgress(qint64 bytesSent, qint64 bytesTotal); // dup?
    qint64 bytesUploaded;


    // cache
    void createCache();
    void completeCacheSave();
    void setCachingEnabled(bool enable);
    bool isCachingEnabled() const;
    bool isCachingAllowed() const;
    void initCacheSaveDevice();
    QIODevice *cacheLoadDevice;
    bool loadingFromCache;

    QIODevice *cacheSaveDevice;
    bool cacheEnabled; // is this for saving?


    QUrl urlForLastAuthentication;
#ifndef QT_NO_NETWORKPROXY
    QNetworkProxy lastProxyAuthentication;
#endif


    bool migrateBackend();
    bool canResume() const;
    void setResumeOffset(quint64 offset);
    quint64 resumeOffset;
    qint64 preMigrationDownloaded;

    // Used for normal downloading. For "zero copy" the downloadZerocopyBuffer is used
    QByteDataBuffer downloadMultiBuffer;
    QByteDataBuffer pendingDownloadData; // For signal compression
    qint64 bytesDownloaded;

    // only used when the "zero copy" style is used. Else downloadMultiBuffer is used.
    // Please note that the whole "zero copy" download buffer API is private right now. Do not use it.
    qint64 downloadBufferReadPosition;
    qint64 downloadBufferCurrentSize;
    QSharedPointer<char> downloadBufferPointer;
    char* downloadZerocopyBuffer;

    // Will be increased by HTTP thread:
    QSharedPointer<QAtomicInt> pendingDownloadDataEmissions;
    QSharedPointer<QAtomicInt> pendingDownloadProgressEmissions;


#ifndef QT_NO_SSL
    QSslConfiguration sslConfiguration;
    bool pendingIgnoreAllSslErrors;
    QList<QSslError> pendingIgnoreSslErrorsList;
#endif


    bool loadFromCacheIfAllowed(QHttpNetworkRequest &httpRequest);
    void invalidateCache();
    bool sendCacheContents(const QNetworkCacheMetaData &metaData);
    QNetworkCacheMetaData fetchCacheMetaData(const QNetworkCacheMetaData &metaData) const;


    void postRequest();



public:
    // From HTTP thread:
    void replyDownloadData(QByteArray);
    void replyFinished();
    void replyDownloadMetaData(QList<QPair<QByteArray,QByteArray> >, int, QString, bool,
                               QSharedPointer<char>, qint64, bool);
    void replyDownloadProgressSlot(qint64,qint64);
    void httpAuthenticationRequired(const QHttpNetworkRequest &request, QAuthenticator *auth);
    void httpError(QNetworkReply::NetworkError error, const QString &errorString);
#ifndef QT_NO_SSL
    void replyEncrypted();
    void replySslErrors(const QList<QSslError> &, bool *, QList<QSslError> *);
    void replySslConfigurationChanged(const QSslConfiguration&);
#endif
#ifndef QT_NO_NETWORKPROXY
    void proxyAuthenticationRequired(const QNetworkProxy &proxy, QAuthenticator *auth);
#endif

    // From QNonContiguousByteDeviceThreadForwardImpl in HTTP thread:
    void resetUploadDataSlot(bool *r);
    void wantUploadDataSlot(qint64);
    void sentUploadDataSlot(qint64);

    // From user's QNonContiguousByteDevice
    void uploadByteDeviceReadyReadSlot();

    Q_DECLARE_PUBLIC(QNetworkReplyHttpImpl)
};

QT_END_NAMESPACE

#endif // QT_NO_HTTP

#endif
