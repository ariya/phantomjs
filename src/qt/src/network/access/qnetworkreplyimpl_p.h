/****************************************************************************
**
** Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
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

#ifndef QNETWORKREPLYIMPL_P_H
#define QNETWORKREPLYIMPL_P_H

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

#include "qnetworkreply.h"
#include "qnetworkreply_p.h"
#include "qnetworkaccessmanager.h"
#include "qnetworkproxy.h"
#include "QtCore/qmap.h"
#include "QtCore/qqueue.h"
#include "QtCore/qbuffer.h"
#include "private/qringbuffer_p.h"
#include "private/qbytedata_p.h"
#include <QSharedPointer>

QT_BEGIN_NAMESPACE

class QAbstractNetworkCache;
class QNetworkAccessBackend;

class QNetworkReplyImplPrivate;
class QNetworkReplyImpl: public QNetworkReply
{
    Q_OBJECT
public:
    QNetworkReplyImpl(QObject *parent = 0);
    ~QNetworkReplyImpl();
    virtual void abort();

    // reimplemented from QNetworkReply / QIODevice
    virtual void close();
    virtual qint64 bytesAvailable() const;
    virtual void setReadBufferSize(qint64 size);
    virtual bool canReadLine () const;

    virtual qint64 readData(char *data, qint64 maxlen);
    virtual bool event(QEvent *);

#ifndef QT_NO_OPENSSL
    Q_INVOKABLE QSslConfiguration sslConfigurationImplementation() const;
    Q_INVOKABLE void setSslConfigurationImplementation(const QSslConfiguration &configuration);
    virtual void ignoreSslErrors();
    Q_INVOKABLE virtual void ignoreSslErrorsImplementation(const QList<QSslError> &errors);
#endif

    Q_DECLARE_PRIVATE(QNetworkReplyImpl)
    Q_PRIVATE_SLOT(d_func(), void _q_startOperation())
    Q_PRIVATE_SLOT(d_func(), void _q_copyReadyRead())
    Q_PRIVATE_SLOT(d_func(), void _q_copyReadChannelFinished())
    Q_PRIVATE_SLOT(d_func(), void _q_bufferOutgoingData())
    Q_PRIVATE_SLOT(d_func(), void _q_bufferOutgoingDataFinished())
#ifndef QT_NO_BEARERMANAGEMENT
    Q_PRIVATE_SLOT(d_func(), void _q_networkSessionConnected())
    Q_PRIVATE_SLOT(d_func(), void _q_networkSessionFailed())
#endif
    Q_PRIVATE_SLOT(d_func(), void _q_cacheDestroyed())
};

class QNetworkReplyImplPrivate: public QNetworkReplyPrivate
{
public:
    enum InternalNotifications {
        NotifyDownstreamReadyWrite,
        NotifyCloseDownstreamChannel,
        NotifyCopyFinished
    };

    enum State {
        Idle,               // The reply is idle.
        Buffering,          // The reply is buffering outgoing data.
        Working,            // The reply is uploading/downloading data.
        Finished,           // The reply has finished.
        Aborted,            // The reply has been aborted.
        WaitingForSession,  // The reply is waiting for the session to open before connecting.
        Reconnecting        // The reply will reconnect to once roaming has completed.
    };

    typedef QQueue<InternalNotifications> NotificationQueue;

    QNetworkReplyImplPrivate();

    void _q_startOperation();
    void _q_sourceReadyRead();
    void _q_sourceReadChannelFinished();
    void _q_copyReadyRead();
    void _q_copyReadChannelFinished();
    void _q_bufferOutgoingData();
    void _q_bufferOutgoingDataFinished();
#ifndef QT_NO_BEARERMANAGEMENT
    void _q_networkSessionConnected();
    void _q_networkSessionFailed();
#endif
    void _q_cacheDestroyed();

    void setup(QNetworkAccessManager::Operation op, const QNetworkRequest &request,
               QIODevice *outgoingData);

    void pauseNotificationHandling();
    void resumeNotificationHandling();
    void backendNotify(InternalNotifications notification);
    void handleNotifications();
    void createCache();
    void completeCacheSave();

    // callbacks from the backend (through the manager):
    void setCachingEnabled(bool enable);
    bool isCachingEnabled() const;
    void consume(qint64 count);
    void emitUploadProgress(qint64 bytesSent, qint64 bytesTotal);
    qint64 nextDownstreamBlockSize() const;

    void initCacheSaveDevice();
    void appendDownstreamDataSignalEmissions();
    void appendDownstreamData(QByteDataBuffer &data);
    void appendDownstreamData(QIODevice *data);
    void appendDownstreamData(const QByteArray &data);

    void setDownloadBuffer(QSharedPointer<char> sp, qint64 size);
    char* getDownloadBuffer(qint64 size);
    void appendDownstreamDataDownloadBuffer(qint64, qint64);

    void finished();
    void error(QNetworkReply::NetworkError code, const QString &errorString);
    void metaDataChanged();
    void redirectionRequested(const QUrl &target);
    void sslErrors(const QList<QSslError> &errors);

    QNetworkAccessBackend *backend;
    QIODevice *outgoingData;
    QSharedPointer<QRingBuffer> outgoingDataBuffer;
    QIODevice *copyDevice;
    QAbstractNetworkCache *networkCache() const;

    bool migrateBackend();

    bool cacheEnabled;
    QIODevice *cacheSaveDevice;

    NotificationQueue pendingNotifications;
    bool notificationHandlingPaused;

    QUrl urlForLastAuthentication;
#ifndef QT_NO_NETWORKPROXY
    QNetworkProxy lastProxyAuthentication;
    QList<QNetworkProxy> proxyList;
#endif

    // Used for normal downloading. For "zero copy" the downloadBuffer is used
    QByteDataBuffer readBuffer;
    qint64 bytesDownloaded;
    qint64 lastBytesDownloaded;
    qint64 bytesUploaded;
    qint64 preMigrationDownloaded;

    QString httpReasonPhrase;
    int httpStatusCode;

    State state;

    // only used when the "zero copy" style is used. Else readBuffer is used.
    // Please note that the whole "zero copy" download buffer API is private right now. Do not use it.
    qint64 downloadBufferReadPosition;
    qint64 downloadBufferCurrentSize;
    qint64 downloadBufferMaximumSize;
    QSharedPointer<char> downloadBufferPointer;
    char* downloadBuffer;

    Q_DECLARE_PUBLIC(QNetworkReplyImpl)
};

#ifndef QT_NO_BEARERMANAGEMENT
class QDisabledNetworkReply : public QNetworkReply
{
    Q_OBJECT

public:
    QDisabledNetworkReply(QObject *parent, const QNetworkRequest &req,
                          QNetworkAccessManager::Operation op);
    ~QDisabledNetworkReply();

    void abort() { }
protected:
    qint64 readData(char *, qint64) { return -1; }
};
#endif

QT_END_NAMESPACE

#endif
