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

#include "qabstractsocketengine_p.h"

#ifndef Q_OS_WINRT
#include "qnativesocketengine_p.h"
#else
#include "qnativesocketengine_winrt_p.h"
#endif

#include "qmutex.h"
#include "qnetworkproxy.h"

QT_BEGIN_NAMESPACE

class QSocketEngineHandlerList : public QList<QSocketEngineHandler*>
{
public:
    QMutex mutex;
};

Q_GLOBAL_STATIC(QSocketEngineHandlerList, socketHandlers)

QSocketEngineHandler::QSocketEngineHandler()
{
    if (!socketHandlers())
        return;
    QMutexLocker locker(&socketHandlers()->mutex);
    socketHandlers()->prepend(this);
}

QSocketEngineHandler::~QSocketEngineHandler()
{
    if (!socketHandlers())
        return;
    QMutexLocker locker(&socketHandlers()->mutex);
    socketHandlers()->removeAll(this);
}

QAbstractSocketEnginePrivate::QAbstractSocketEnginePrivate()
    : socketError(QAbstractSocket::UnknownSocketError)
    , hasSetSocketError(false)
    , socketErrorString(QLatin1String(QT_TRANSLATE_NOOP(QSocketLayer, "Unknown error")))
    , socketState(QAbstractSocket::UnconnectedState)
    , socketType(QAbstractSocket::UnknownSocketType)
    , socketProtocol(QAbstractSocket::UnknownNetworkLayerProtocol)
    , localPort(0)
    , peerPort(0)
    , receiver(0)
{
}

QAbstractSocketEngine::QAbstractSocketEngine(QObject *parent)
    : QObject(*new QAbstractSocketEnginePrivate(), parent)
{
}

QAbstractSocketEngine::QAbstractSocketEngine(QAbstractSocketEnginePrivate &dd, QObject* parent)
    : QObject(dd, parent)
{
}

QAbstractSocketEngine *QAbstractSocketEngine::createSocketEngine(QAbstractSocket::SocketType socketType, const QNetworkProxy &proxy, QObject *parent)
{
#ifndef QT_NO_NETWORKPROXY
    // proxy type must have been resolved by now
    if (proxy.type() == QNetworkProxy::DefaultProxy)
        return 0;
#endif

    QMutexLocker locker(&socketHandlers()->mutex);
    for (int i = 0; i < socketHandlers()->size(); i++) {
        if (QAbstractSocketEngine *ret = socketHandlers()->at(i)->createSocketEngine(socketType, proxy, parent))
            return ret;
    }

#ifndef QT_NO_NETWORKPROXY
    // only NoProxy can have reached here
    if (proxy.type() != QNetworkProxy::NoProxy)
        return 0;
#endif

    return new QNativeSocketEngine(parent);
}

QAbstractSocketEngine *QAbstractSocketEngine::createSocketEngine(qintptr socketDescripter, QObject *parent)
{
    QMutexLocker locker(&socketHandlers()->mutex);
    for (int i = 0; i < socketHandlers()->size(); i++) {
        if (QAbstractSocketEngine *ret = socketHandlers()->at(i)->createSocketEngine(socketDescripter, parent))
            return ret;
    }
    return new QNativeSocketEngine(parent);
}

QAbstractSocket::SocketError QAbstractSocketEngine::error() const
{
    return d_func()->socketError;
}

QString QAbstractSocketEngine::errorString() const
{
    return d_func()->socketErrorString;
}

void QAbstractSocketEngine::setError(QAbstractSocket::SocketError error, const QString &errorString) const
{
    Q_D(const QAbstractSocketEngine);
    d->socketError = error;
    d->socketErrorString = errorString;
}

void QAbstractSocketEngine::setReceiver(QAbstractSocketEngineReceiver *receiver)
{
    d_func()->receiver = receiver;
}

void QAbstractSocketEngine::readNotification()
{
    if (QAbstractSocketEngineReceiver *receiver = d_func()->receiver)
        receiver->readNotification();
}

void QAbstractSocketEngine::writeNotification()
{
    if (QAbstractSocketEngineReceiver *receiver = d_func()->receiver)
        receiver->writeNotification();
}

void QAbstractSocketEngine::exceptionNotification()
{
    if (QAbstractSocketEngineReceiver *receiver = d_func()->receiver)
        receiver->exceptionNotification();
}

void QAbstractSocketEngine::closeNotification()
{
    if (QAbstractSocketEngineReceiver *receiver = d_func()->receiver)
        receiver->closeNotification();
}

void QAbstractSocketEngine::connectionNotification()
{
    if (QAbstractSocketEngineReceiver *receiver = d_func()->receiver)
        receiver->connectionNotification();
}

#ifndef QT_NO_NETWORKPROXY
void QAbstractSocketEngine::proxyAuthenticationRequired(const QNetworkProxy &proxy, QAuthenticator *authenticator)
{
    if (QAbstractSocketEngineReceiver *receiver = d_func()->receiver)
        receiver->proxyAuthenticationRequired(proxy, authenticator);
}
#endif


QAbstractSocket::SocketState QAbstractSocketEngine::state() const
{
    return d_func()->socketState;
}

void QAbstractSocketEngine::setState(QAbstractSocket::SocketState state)
{
    d_func()->socketState = state;
}

QAbstractSocket::SocketType QAbstractSocketEngine::socketType() const
{
    return d_func()->socketType;
}

void QAbstractSocketEngine::setSocketType(QAbstractSocket::SocketType socketType)
{
    d_func()->socketType = socketType;
}

QAbstractSocket::NetworkLayerProtocol QAbstractSocketEngine::protocol() const
{
    return d_func()->socketProtocol;
}

void QAbstractSocketEngine::setProtocol(QAbstractSocket::NetworkLayerProtocol protocol)
{
    d_func()->socketProtocol = protocol;
}

QHostAddress QAbstractSocketEngine::localAddress() const
{
    return d_func()->localAddress;
}

void QAbstractSocketEngine::setLocalAddress(const QHostAddress &address)
{
    d_func()->localAddress = address;
}

quint16 QAbstractSocketEngine::localPort() const
{
    return d_func()->localPort;
}

void QAbstractSocketEngine::setLocalPort(quint16 port)
{
    d_func()->localPort = port;
}

QHostAddress QAbstractSocketEngine::peerAddress() const
{
    return d_func()->peerAddress;
}

void QAbstractSocketEngine::setPeerAddress(const QHostAddress &address)
{
   d_func()->peerAddress = address;
}

quint16 QAbstractSocketEngine::peerPort() const
{
    return d_func()->peerPort;
}

void QAbstractSocketEngine::setPeerPort(quint16 port)
{
    d_func()->peerPort = port;
}

QT_END_NAMESPACE
