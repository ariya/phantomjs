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

#ifndef QABSTRACTSOCKET_P_H
#define QABSTRACTSOCKET_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of the QAbstractSocket class.  This header file may change from
// version to version without notice, or even be removed.
//
// We mean it.
//

#include "QtNetwork/qabstractsocket.h"
#include "QtCore/qbytearray.h"
#include "QtCore/qlist.h"
#include "QtCore/qtimer.h"
#include "private/qringbuffer_p.h"
#include "private/qiodevice_p.h"
#include "private/qabstractsocketengine_p.h"
#include "qnetworkproxy.h"

QT_BEGIN_NAMESPACE

class QHostInfo;

class QAbstractSocketPrivate : public QIODevicePrivate, public QAbstractSocketEngineReceiver
{
    Q_DECLARE_PUBLIC(QAbstractSocket)
public:
    QAbstractSocketPrivate();
    virtual ~QAbstractSocketPrivate();

    // from QAbstractSocketEngineReceiver
    inline void readNotification() { canReadNotification(); }
    inline void writeNotification() { canWriteNotification(); }
    inline void exceptionNotification() {}
    inline void closeNotification() { canCloseNotification(); }
    void connectionNotification();
#ifndef QT_NO_NETWORKPROXY
    inline void proxyAuthenticationRequired(const QNetworkProxy &proxy, QAuthenticator *authenticator) {
        Q_Q(QAbstractSocket);
        q->proxyAuthenticationRequired(proxy, authenticator);
    }
#endif

    bool canReadNotification();
    bool canWriteNotification();
    void canCloseNotification();

    // slots
    void _q_connectToNextAddress();
    void _q_startConnecting(const QHostInfo &hostInfo);
    void _q_testConnection();
    void _q_abortConnectionAttempt();
    void _q_forceDisconnect();

    bool readSocketNotifierCalled;
    bool readSocketNotifierState;
    bool readSocketNotifierStateSet;

    bool emittedReadyRead;
    bool emittedBytesWritten;

    bool abortCalled;
    bool closeCalled;
    bool pendingClose;

    QAbstractSocket::PauseModes pauseMode;

    QString hostName;
    quint16 port;
    QHostAddress host;
    QList<QHostAddress> addresses;

    quint16 localPort;
    quint16 peerPort;
    QHostAddress localAddress;
    QHostAddress peerAddress;
    QString peerName;

    QAbstractSocketEngine *socketEngine;
    qintptr cachedSocketDescriptor;

#ifndef QT_NO_NETWORKPROXY
    QNetworkProxy proxy;
    QNetworkProxy proxyInUse;
    void resolveProxy(const QString &hostName, quint16 port);
#else
    inline void resolveProxy(const QString &, quint16) { }
#endif
    inline void resolveProxy(quint16 port) { resolveProxy(QString(), port); }

    void resetSocketLayer();
    bool flush();

    bool initSocketLayer(QAbstractSocket::NetworkLayerProtocol protocol);
    void startConnectingByName(const QString &host);
    void fetchConnectionParameters();
    void setupSocketNotifiers();
    bool readFromSocket();

    qint64 readBufferMaxSize;
    QRingBuffer writeBuffer;

    bool isBuffered;
    int blockingTimeout;

    QTimer *connectTimer;
    QTimer *disconnectTimer;
    int connectTimeElapsed;

    int hostLookupId;

    QAbstractSocket::SocketType socketType;
    QAbstractSocket::SocketState state;

    QAbstractSocket::SocketError socketError;

    QAbstractSocket::NetworkLayerProtocol preferredNetworkLayerProtocol;

    bool prePauseReadSocketNotifierState;
    bool prePauseWriteSocketNotifierState;
    bool prePauseExceptionSocketNotifierState;
    static void pauseSocketNotifiers(QAbstractSocket*);
    static void resumeSocketNotifiers(QAbstractSocket*);
    static QAbstractSocketEngine* getSocketEngine(QAbstractSocket*);
};

QT_END_NAMESPACE

#endif // QABSTRACTSOCKET_P_H
