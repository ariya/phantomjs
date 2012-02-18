/*
 * Copyright (C) 2010 Nokia Inc. All rights reserved.
 * Copyright (C) 2009 Google Inc.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "SocketStreamHandle.h"

#include "KURL.h"
#include "Logging.h"
#include "NotImplemented.h"
#include "SocketStreamHandleClient.h"
#include "SocketStreamHandlePrivate.h"

namespace WebCore {

SocketStreamHandlePrivate::SocketStreamHandlePrivate(SocketStreamHandle* streamHandle, const KURL& url) : QObject()
{
    m_streamHandle = streamHandle;
    m_socket = 0;
    bool isSecure = url.protocolIs("wss");

    if (isSecure) {
#ifndef QT_NO_OPENSSL
        m_socket = new QSslSocket(this);
#endif
    } else
        m_socket = new QTcpSocket(this);

    if (!m_socket)
        return;

    connect(m_socket, SIGNAL(connected()), this, SLOT(socketConnected()));
    connect(m_socket, SIGNAL(readyRead()), this, SLOT(socketReadyRead()));
    connect(m_socket, SIGNAL(disconnected()), this, SLOT(socketClosed()));
    connect(m_socket, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(socketError(QAbstractSocket::SocketError)));
    if (isSecure)
        connect(m_socket, SIGNAL(sslErrors(const QList<QSslError>&)), this, SLOT(socketSslErrors(const QList<QSslError>&)));

    unsigned int port = url.hasPort() ? url.port() : (isSecure ? 443 : 80);

    QString host = url.host();
    if (isSecure) {
#ifndef QT_NO_OPENSSL
        static_cast<QSslSocket*>(m_socket)->connectToHostEncrypted(host, port);
#endif
    } else
        m_socket->connectToHost(host, port);
}

SocketStreamHandlePrivate::~SocketStreamHandlePrivate()
{
    Q_ASSERT(!(m_socket && m_socket->state() == QAbstractSocket::ConnectedState));
}

void SocketStreamHandlePrivate::socketConnected()
{
    if (m_streamHandle && m_streamHandle->client()) {
        m_streamHandle->m_state = SocketStreamHandleBase::Open;
        m_streamHandle->client()->didOpen(m_streamHandle);
    }
}

void SocketStreamHandlePrivate::socketReadyRead()
{
    if (m_streamHandle && m_streamHandle->client()) {
        QByteArray data = m_socket->read(m_socket->bytesAvailable());
        m_streamHandle->client()->didReceiveData(m_streamHandle, data.constData(), data.size());
    }
}

int SocketStreamHandlePrivate::send(const char* data, int len)
{
    if (!m_socket || m_socket->state() != QAbstractSocket::ConnectedState)
        return 0;
    quint64 sentSize = m_socket->write(data, len);
    QMetaObject::invokeMethod(this, "socketSentData", Qt::QueuedConnection);
    return sentSize;
}

void SocketStreamHandlePrivate::close()
{
    if (m_socket && m_socket->state() == QAbstractSocket::ConnectedState)
        m_socket->close();
}

void SocketStreamHandlePrivate::socketSentData()
{
    if (m_streamHandle)
        m_streamHandle->sendPendingData();
}

void SocketStreamHandlePrivate::socketClosed()
{
    QMetaObject::invokeMethod(this, "socketClosedCallback", Qt::QueuedConnection);
}

void SocketStreamHandlePrivate::socketError(QAbstractSocket::SocketError error)
{
    QMetaObject::invokeMethod(this, "socketErrorCallback", Qt::QueuedConnection, Q_ARG(int, error));
}

void SocketStreamHandlePrivate::socketClosedCallback()
{
    if (m_streamHandle && m_streamHandle->client()) {
        SocketStreamHandle* streamHandle = m_streamHandle;
        m_streamHandle = 0;
        // This following call deletes _this_. Nothing should be after it.
        streamHandle->client()->didClose(streamHandle);
    }
}

void SocketStreamHandlePrivate::socketErrorCallback(int error)
{
    // FIXME - in the future, we might not want to treat all errors as fatal.
    if (m_streamHandle && m_streamHandle->client()) {
        SocketStreamHandle* streamHandle = m_streamHandle;
        m_streamHandle = 0;
        // This following call deletes _this_. Nothing should be after it.
        streamHandle->client()->didClose(streamHandle);
    }
}

#ifndef QT_NO_OPENSSL
void SocketStreamHandlePrivate::socketSslErrors(const QList<QSslError>& error)
{
    QMetaObject::invokeMethod(this, "socketErrorCallback", Qt::QueuedConnection, Q_ARG(int, error[0].error()));
}
#endif

SocketStreamHandle::SocketStreamHandle(const KURL& url, SocketStreamHandleClient* client)
    : SocketStreamHandleBase(url, client)
{
    LOG(Network, "SocketStreamHandle %p new client %p", this, m_client);
    m_p = new SocketStreamHandlePrivate(this, url);
}

SocketStreamHandle::~SocketStreamHandle()
{
    LOG(Network, "SocketStreamHandle %p delete", this);
    setClient(0);
    delete m_p;
}

int SocketStreamHandle::platformSend(const char* data, int len)
{
    LOG(Network, "SocketStreamHandle %p platformSend", this);
    return m_p->send(data, len);
}

void SocketStreamHandle::platformClose()
{
    LOG(Network, "SocketStreamHandle %p platformClose", this);
    m_p->close();
}

void SocketStreamHandle::didReceiveAuthenticationChallenge(const AuthenticationChallenge&)
{
    notImplemented();
}

void SocketStreamHandle::receivedCredential(const AuthenticationChallenge&, const Credential&)
{
    notImplemented();
}

void SocketStreamHandle::receivedRequestToContinueWithoutCredential(const AuthenticationChallenge&)
{
    notImplemented();
}

void SocketStreamHandle::receivedCancellation(const AuthenticationChallenge&)
{
    notImplemented();
}

} // namespace WebCore

#include "moc_SocketStreamHandlePrivate.cpp"
