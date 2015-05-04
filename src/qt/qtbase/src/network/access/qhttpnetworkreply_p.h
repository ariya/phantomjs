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

#ifndef QHTTPNETWORKREPLY_H
#define QHTTPNETWORKREPLY_H

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
#include <qplatformdefs.h>
#ifndef QT_NO_HTTP

#ifndef QT_NO_COMPRESS
struct z_stream_s;
#endif

#include <QtNetwork/qtcpsocket.h>
// it's safe to include these even if SSL support is not enabled
#include <QtNetwork/qsslsocket.h>
#include <QtNetwork/qsslerror.h>

#include <QtNetwork/qnetworkrequest.h>
#include <QtNetwork/qnetworkreply.h>
#include <qbuffer.h>

#include <private/qobject_p.h>
#include <private/qhttpnetworkheader_p.h>
#include <private/qhttpnetworkrequest_p.h>
#include <private/qauthenticator_p.h>
#include <private/qringbuffer_p.h>
#include <private/qbytedata_p.h>

QT_BEGIN_NAMESPACE

class QHttpNetworkConnection;
class QHttpNetworkConnectionChannel;
class QHttpNetworkRequest;
class QHttpNetworkConnectionPrivate;
class QHttpNetworkReplyPrivate;
class Q_AUTOTEST_EXPORT QHttpNetworkReply : public QObject, public QHttpNetworkHeader
{
    Q_OBJECT
public:

    explicit QHttpNetworkReply(const QUrl &url = QUrl(), QObject *parent = 0);
    virtual ~QHttpNetworkReply();

    QUrl url() const;
    void setUrl(const QUrl &url);

    int majorVersion() const;
    int minorVersion() const;

    qint64 contentLength() const;
    void setContentLength(qint64 length);

    QList<QPair<QByteArray, QByteArray> > header() const;
    QByteArray headerField(const QByteArray &name, const QByteArray &defaultValue = QByteArray()) const;
    void setHeaderField(const QByteArray &name, const QByteArray &data);
    void parseHeader(const QByteArray &header); // mainly for testing

    QHttpNetworkRequest request() const;
    void setRequest(const QHttpNetworkRequest &request);

    int statusCode() const;
    void setStatusCode(int code);

    QString errorString() const;
    void setErrorString(const QString &error);

    QString reasonPhrase() const;

    qint64 bytesAvailable() const;
    qint64 bytesAvailableNextBlock() const;
    bool readAnyAvailable() const;
    QByteArray readAny();
    QByteArray readAll();
    QByteArray read(qint64 amount);
    qint64 sizeNextBlock();
    void setDownstreamLimited(bool t);
    void setReadBufferSize(qint64 size);

    bool supportsUserProvidedDownloadBuffer();
    void setUserProvidedDownloadBuffer(char*);
    char* userProvidedDownloadBuffer();

    bool isFinished() const;

    bool isPipeliningUsed() const;
    bool isSpdyUsed() const;
    void setSpdyWasUsed(bool spdy);

    QHttpNetworkConnection* connection();

#ifndef QT_NO_SSL
    QSslConfiguration sslConfiguration() const;
    void setSslConfiguration(const QSslConfiguration &config);
    void ignoreSslErrors();
    void ignoreSslErrors(const QList<QSslError> &errors);

Q_SIGNALS:
    void encrypted();
    void sslErrors(const QList<QSslError> &errors);
#endif

Q_SIGNALS:
    void readyRead();
    void finished();
    void finishedWithError(QNetworkReply::NetworkError errorCode, const QString &detail = QString());
    void headerChanged();
    void dataReadProgress(qint64 done, qint64 total);
    void dataSendProgress(qint64 done, qint64 total);
    void cacheCredentials(const QHttpNetworkRequest &request, QAuthenticator *authenticator);
#ifndef QT_NO_NETWORKPROXY
    void proxyAuthenticationRequired(const QNetworkProxy &proxy, QAuthenticator *authenticator);
#endif
    void authenticationRequired(const QHttpNetworkRequest &request, QAuthenticator *authenticator);
private:
    Q_DECLARE_PRIVATE(QHttpNetworkReply)
    friend class QHttpSocketEngine;
    friend class QHttpNetworkConnection;
    friend class QHttpNetworkConnectionPrivate;
    friend class QHttpNetworkConnectionChannel;
    friend class QHttpProtocolHandler;
    friend class QSpdyProtocolHandler;
};


class QHttpNetworkReplyPrivate : public QObjectPrivate, public QHttpNetworkHeaderPrivate
{
public:
    QHttpNetworkReplyPrivate(const QUrl &newUrl = QUrl());
    ~QHttpNetworkReplyPrivate();
    qint64 readStatus(QAbstractSocket *socket);
    bool parseStatus(const QByteArray &status);
    qint64 readHeader(QAbstractSocket *socket);
    void parseHeader(const QByteArray &header);
    qint64 readBody(QAbstractSocket *socket, QByteDataBuffer *out);
    qint64 readBodyVeryFast(QAbstractSocket *socket, char *b);
    qint64 readBodyFast(QAbstractSocket *socket, QByteDataBuffer *rb);
    bool findChallenge(bool forProxy, QByteArray &challenge) const;
    QAuthenticatorPrivate::Method authenticationMethod(bool isProxy) const;
    void clear();
    void clearHttpLayerInformation();

    qint64 readReplyBodyRaw(QAbstractSocket *in, QByteDataBuffer *out, qint64 size);
    qint64 readReplyBodyChunked(QAbstractSocket *in, QByteDataBuffer *out);
    qint64 getChunkSize(QAbstractSocket *in, qint64 *chunkSize);

    bool shouldEmitSignals();
    bool expectContent();
    void eraseData();

    qint64 bytesAvailable() const;
    bool isChunked();
    bool isConnectionCloseEnabled();

    bool isCompressed();
    void removeAutoDecompressHeader();

    enum ReplyState {
        NothingDoneState,
        ReadingStatusState,
        ReadingHeaderState,
        ReadingDataState,
        AllDoneState,
        SPDYSYNSent,
        SPDYUploading,
        SPDYHalfClosed,
        SPDYClosed
    } state;

    QHttpNetworkRequest request;
    bool ssl;
    int statusCode;
    int majorVersion;
    int minorVersion;
    QString errorString;
    QString reasonPhrase;
    qint64 bodyLength;
    qint64 contentRead;
    qint64 totalProgress;
    QByteArray fragment; // used for header, status, chunk header etc, not for reply data
    bool chunkedTransferEncoding;
    bool connectionCloseEnabled;
    bool forceConnectionCloseEnabled;
    bool lastChunkRead;
    qint64 currentChunkSize;
    qint64 currentChunkRead;
    qint64 readBufferMaxSize;
    qint32 windowSizeDownload; // only for SPDY
    qint32 windowSizeUpload; // only for SPDY
    qint32 currentlyReceivedDataInWindow; // only for SPDY
    qint32 currentlyUploadedDataInWindow; // only for SPDY
    qint64 totallyUploadedData; // only for SPDY
    QPointer<QHttpNetworkConnection> connection;
    QPointer<QHttpNetworkConnectionChannel> connectionChannel;

    bool autoDecompress;

    QByteDataBuffer responseData; // uncompressed body
    QByteArray compressedData; // compressed body (temporary)
    bool requestIsPrepared;

    bool pipeliningUsed;
    bool spdyUsed;
    bool downstreamLimited;

    char* userProvidedDownloadBuffer;

#ifndef QT_NO_COMPRESS
    z_stream_s *inflateStrm;
    int initializeInflateStream();
    qint64 uncompressBodyData(QByteDataBuffer *in, QByteDataBuffer *out);
#endif
};




QT_END_NAMESPACE

#endif // QT_NO_HTTP


#endif // QHTTPNETWORKREPLY_H
