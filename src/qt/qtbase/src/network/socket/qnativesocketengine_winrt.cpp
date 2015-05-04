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

#include <qt_windows.h>

#include "qnativesocketengine_winrt_p.h"

#include <qcoreapplication.h>
#include <qabstracteventdispatcher.h>
#include <qsocketnotifier.h>
#include <qdatetime.h>
#include <qnetworkinterface.h>
#include <qelapsedtimer.h>
#include <qthread.h>
#include <qabstracteventdispatcher.h>
#include <qfunctions_winrt.h>

#include <private/qthread_p.h>
#include <private/qabstractsocket_p.h>

#ifndef QT_NO_SSL
#include <QSslSocket>
#endif

#include <wrl.h>
#include <windows.foundation.collections.h>
#include <windows.storage.streams.h>
#include <windows.networking.h>
#include <windows.networking.sockets.h>
#include <robuffer.h>

using namespace Microsoft::WRL;
using namespace Microsoft::WRL::Wrappers;
using namespace ABI::Windows::Foundation;
using namespace ABI::Windows::Foundation::Collections;
using namespace ABI::Windows::Storage::Streams;
using namespace ABI::Windows::Networking;
using namespace ABI::Windows::Networking::Connectivity;
using namespace ABI::Windows::Networking::Sockets;

typedef ITypedEventHandler<StreamSocketListener *, StreamSocketListenerConnectionReceivedEventArgs *> ClientConnectedHandler;
typedef ITypedEventHandler<DatagramSocket *, DatagramSocketMessageReceivedEventArgs *> DatagramReceivedHandler;
typedef IAsyncOperationWithProgressCompletedHandler<IBuffer *, UINT32> SocketReadCompletedHandler;
typedef IAsyncOperationWithProgressCompletedHandler<UINT32, UINT32> SocketWriteCompletedHandler;
typedef IAsyncOperationWithProgress<IBuffer *, UINT32> IAsyncBufferOperation;

QT_BEGIN_NAMESPACE

// Common constructs
#define Q_CHECK_VALID_SOCKETLAYER(function, returnValue) do { \
    if (!isValid()) { \
        qWarning(""#function" was called on an uninitialized socket device"); \
        return returnValue; \
    } } while (0)
#define Q_CHECK_INVALID_SOCKETLAYER(function, returnValue) do { \
    if (isValid()) { \
        qWarning(""#function" was called on an already initialized socket device"); \
        return returnValue; \
    } } while (0)
#define Q_CHECK_STATE(function, checkState, returnValue) do { \
    if (d->socketState != (checkState)) { \
        qWarning(""#function" was not called in "#checkState); \
        return (returnValue); \
    } } while (0)
#define Q_CHECK_NOT_STATE(function, checkState, returnValue) do { \
    if (d->socketState == (checkState)) { \
        qWarning(""#function" was called in "#checkState); \
        return (returnValue); \
    } } while (0)
#define Q_CHECK_STATES(function, state1, state2, returnValue) do { \
    if (d->socketState != (state1) && d->socketState != (state2)) { \
        qWarning(""#function" was called" \
                 " not in "#state1" or "#state2); \
        return (returnValue); \
    } } while (0)
#define Q_CHECK_TYPE(function, type, returnValue) do { \
    if (d->socketType != (type)) { \
        qWarning(#function" was called by a" \
                 " socket other than "#type""); \
        return (returnValue); \
    } } while (0)
#define Q_TR(a) QT_TRANSLATE_NOOP(QNativeSocketEngine, a)

typedef QHash<qintptr, IStreamSocket *> TcpSocketHash;

struct SocketHandler
{
    SocketHandler() : socketCount(0) {}
    qintptr socketCount;
    TcpSocketHash pendingTcpSockets;
};

Q_GLOBAL_STATIC(SocketHandler, gSocketHandler)

struct SocketGlobal
{
    SocketGlobal()
    {
        HRESULT hr;
        hr = GetActivationFactory(HString::MakeReference(RuntimeClass_Windows_Storage_Streams_Buffer).Get(),
                                  &bufferFactory);
        Q_ASSERT_SUCCEEDED(hr);
    }

    ComPtr<IBufferFactory> bufferFactory;
};
Q_GLOBAL_STATIC(SocketGlobal, g)

static inline QString qt_QStringFromHString(const HString &string)
{
    UINT32 length;
    PCWSTR rawString = string.GetRawBuffer(&length);
    return QString::fromWCharArray(rawString, length);
}

#define READ_BUFFER_SIZE 65536

template <typename T>
static AsyncStatus opStatus(const ComPtr<T> &op)
{
    ComPtr<IAsyncInfo> info;
    HRESULT hr = op.As(&info);
    if (FAILED(hr)) {
        qErrnoWarning(hr, "Failed to cast op to IAsyncInfo.");
        return Error;
    }
    AsyncStatus status;
    hr = info->get_Status(&status);
    if (FAILED(hr)) {
        qErrnoWarning(hr, "Failed to get AsyncStatus.");
        return Error;
    }
    return status;
}

QNativeSocketEngine::QNativeSocketEngine(QObject *parent)
    : QAbstractSocketEngine(*new QNativeSocketEnginePrivate(), parent)
{
#ifndef QT_NO_SSL
    Q_D(QNativeSocketEngine);
    if (parent)
        d->sslSocket = qobject_cast<QSslSocket *>(parent->parent());
#endif

    connect(this, SIGNAL(connectionReady()), SLOT(connectionNotification()), Qt::QueuedConnection);
    connect(this, SIGNAL(readReady()), SLOT(readNotification()), Qt::QueuedConnection);
    connect(this, SIGNAL(writeReady()), SLOT(writeNotification()), Qt::QueuedConnection);
}

QNativeSocketEngine::~QNativeSocketEngine()
{
    close();
}

bool QNativeSocketEngine::initialize(QAbstractSocket::SocketType type, QAbstractSocket::NetworkLayerProtocol protocol)
{
    Q_D(QNativeSocketEngine);
    if (isValid())
        close();

    // Create the socket
    if (!d->createNewSocket(type, protocol))
        return false;

    d->socketType = type;
    d->socketProtocol = protocol;
    return true;
}

bool QNativeSocketEngine::initialize(qintptr socketDescriptor, QAbstractSocket::SocketState socketState)
{
    Q_D(QNativeSocketEngine);

    if (isValid())
        close();

    // Currently, only TCP sockets are initialized this way.
    d->socketDescriptor = qintptr(gSocketHandler->pendingTcpSockets.take(socketDescriptor));
    d->socketType = QAbstractSocket::TcpSocket;

    if (!d->socketDescriptor || !d->fetchConnectionParameters()) {
        d->setError(QAbstractSocket::UnsupportedSocketOperationError,
            d->InvalidSocketErrorString);
        d->socketDescriptor = -1;
        return false;
    }

    d->socketState = socketState;
    return true;
}

qintptr QNativeSocketEngine::socketDescriptor() const
{
    Q_D(const QNativeSocketEngine);
    return d->socketDescriptor;
}

bool QNativeSocketEngine::isValid() const
{
    Q_D(const QNativeSocketEngine);
    return d->socketDescriptor != -1;
}

bool QNativeSocketEngine::connectToHost(const QHostAddress &address, quint16 port)
{
    const QString addressString = address.toString();
    return connectToHostByName(addressString, port);
}

bool QNativeSocketEngine::connectToHostByName(const QString &name, quint16 port)
{
    Q_D(QNativeSocketEngine);
    HStringReference hostNameRef(reinterpret_cast<LPCWSTR>(name.utf16()));
    ComPtr<IHostNameFactory> hostNameFactory;
    GetActivationFactory(HString::MakeReference(RuntimeClass_Windows_Networking_HostName).Get(),
                         &hostNameFactory);
    ComPtr<IHostName> remoteHost;
    if (FAILED(hostNameFactory->CreateHostName(hostNameRef.Get(), &remoteHost))) {
        qWarning("QNativeSocketEnginePrivate::nativeConnect:: Could not create hostname");
        return false;
    }

    const QString portString = QString::number(port);
    HStringReference portReference(reinterpret_cast<LPCWSTR>(portString.utf16()));
    HRESULT hr = E_FAIL;
    if (d->socketType == QAbstractSocket::TcpSocket)
        hr = d->tcpSocket()->ConnectAsync(remoteHost.Get(), portReference.Get(), &d->connectOp);
    else if (d->socketType == QAbstractSocket::UdpSocket)
        hr = d->udpSocket()->ConnectAsync(remoteHost.Get(), portReference.Get(), &d->connectOp);
    if (FAILED(hr)) {
        qWarning("QNativeSocketEnginePrivate::nativeConnect:: Could not obtain connect action");
        return false;
    }
    d->socketState = QAbstractSocket::ConnectingState;
    hr = d->connectOp->put_Completed(Callback<IAsyncActionCompletedHandler>(
                                         d, &QNativeSocketEnginePrivate::handleConnectToHost).Get());
    Q_ASSERT_SUCCEEDED(hr);

    return d->socketState == QAbstractSocket::ConnectedState;
}

bool QNativeSocketEngine::bind(const QHostAddress &address, quint16 port)
{
    Q_D(QNativeSocketEngine);
    ComPtr<IHostName> hostAddress;
    if (address != QHostAddress::Any && address != QHostAddress::AnyIPv4 && address != QHostAddress::AnyIPv6) {
        ComPtr<IHostNameFactory> hostNameFactory;
        GetActivationFactory(HString::MakeReference(RuntimeClass_Windows_Networking_HostName).Get(),
                             &hostNameFactory);
        const QString addressString = address.toString();
        HStringReference addressRef(reinterpret_cast<LPCWSTR>(addressString.utf16()));
        hostNameFactory->CreateHostName(addressRef.Get(), &hostAddress);
    }

    HRESULT hr;
    QString portQString = port ? QString::number(port) : QString();
    HStringReference portString(reinterpret_cast<LPCWSTR>(portQString.utf16()));

    ComPtr<IAsyncAction> op;
    if (d->socketType == QAbstractSocket::TcpSocket) {
        if (!d->tcpListener
                && FAILED(RoActivateInstance(HString::MakeReference(RuntimeClass_Windows_Networking_Sockets_StreamSocketListener).Get(),
                                             &d->tcpListener))) {
            qWarning("Failed to create listener");
            return false;
        }

        EventRegistrationToken token;
        d->tcpListener->add_ConnectionReceived(Callback<ClientConnectedHandler>(d, &QNativeSocketEnginePrivate::handleClientConnection).Get(), &token);
        hr = d->tcpListener->BindEndpointAsync(hostAddress.Get(), portString.Get(), &op);
        if (FAILED(hr)) {
            qErrnoWarning(hr, "Unable to bind socket."); // ### Set error message
            return false;
        }
    } else if (d->socketType == QAbstractSocket::UdpSocket) {
        hr = d->udpSocket()->BindEndpointAsync(hostAddress.Get(), portString.Get(), &op);
        if (FAILED(hr)) {
            qErrnoWarning(hr, "Unable to bind socket."); // ### Set error message
            return false;
        }
        hr = op->put_Completed(Callback<IAsyncActionCompletedHandler>(d, &QNativeSocketEnginePrivate::handleBindCompleted).Get());
        if (FAILED(hr)) {
            qErrnoWarning(hr, "Unable to set bind callback.");
            return false;
        }
    }

    if (op) {
        while (opStatus(op) == Started)
            d->eventLoop.processEvents();

        AsyncStatus status = opStatus(op);
        if (status == Error || status == Canceled)
            return false;

        hr = op->GetResults();
        if (FAILED(hr)) {
            qErrnoWarning(hr, "Failed to bind socket");
            return false;
        }

        d->socketState = QAbstractSocket::BoundState;
        d->fetchConnectionParameters();
        return true;
    }

    return false;
}

bool QNativeSocketEngine::listen()
{
    Q_D(QNativeSocketEngine);
    Q_CHECK_VALID_SOCKETLAYER(QNativeSocketEngine::listen(), false);
    Q_CHECK_STATE(QNativeSocketEngine::listen(), QAbstractSocket::BoundState, false);
    Q_CHECK_TYPE(QNativeSocketEngine::listen(), QAbstractSocket::TcpSocket, false);

    if (d->tcpListener && d->socketDescriptor != -1) {
        d->socketState = QAbstractSocket::ListeningState;
        return true;
    }
    return false;
}

int QNativeSocketEngine::accept()
{
    Q_D(QNativeSocketEngine);
    Q_CHECK_VALID_SOCKETLAYER(QNativeSocketEngine::accept(), -1);
    Q_CHECK_STATE(QNativeSocketEngine::accept(), QAbstractSocket::ListeningState, -1);
    Q_CHECK_TYPE(QNativeSocketEngine::accept(), QAbstractSocket::TcpSocket, -1);

    if (d->socketDescriptor == -1 || d->pendingConnections.isEmpty())
        return -1;

    // Start processing incoming data
    if (d->socketType == QAbstractSocket::TcpSocket) {
        IStreamSocket *socket = d->pendingConnections.takeFirst();

        HRESULT hr;
        ComPtr<IBuffer> buffer;
        hr = g->bufferFactory->Create(READ_BUFFER_SIZE, &buffer);
        Q_ASSERT_SUCCEEDED(hr);

        ComPtr<IInputStream> stream;
        hr = socket->get_InputStream(&stream);
        Q_ASSERT_SUCCEEDED(hr);
        ComPtr<IAsyncBufferOperation> op;
        hr = stream->ReadAsync(buffer.Get(), READ_BUFFER_SIZE, InputStreamOptions_Partial, &op);
        if (FAILED(hr)) {
            qErrnoWarning(hr, "Faild to read from the socket buffer.");
            return -1;
        }
        hr = op->put_Completed(Callback<SocketReadCompletedHandler>(d, &QNativeSocketEnginePrivate::handleReadyRead).Get());
        if (FAILED(hr)) {
            qErrnoWarning(hr, "Failed to set socket read callback.");
            return -1;
        }
        d->currentConnections.append(socket);

        SocketHandler *handler = gSocketHandler();
        handler->pendingTcpSockets.insert(++handler->socketCount, socket);
        return handler->socketCount;
    }

    return -1;
}

void QNativeSocketEngine::close()
{
    Q_D(QNativeSocketEngine);

    if (d->connectOp) {
        ComPtr<IAsyncInfo> info;
        d->connectOp.As(&info);
        if (info) {
            info->Cancel();
            info->Close();
        }
    }

    if (d->socketDescriptor != -1) {
        ComPtr<IClosable> socket;
        if (d->socketType == QAbstractSocket::TcpSocket) {
            d->tcpSocket()->QueryInterface(IID_PPV_ARGS(&socket));
            d->tcpSocket()->Release();
        } else if (d->socketType == QAbstractSocket::UdpSocket) {
            d->udpSocket()->QueryInterface(IID_PPV_ARGS(&socket));
            d->udpSocket()->Release();
        }

        if (socket) {
            d->closingDown = true;
            socket->Close();
            d->socketDescriptor = -1;
        }
        d->socketDescriptor = -1;
    }
    d->socketState = QAbstractSocket::UnconnectedState;
    d->hasSetSocketError = false;
    d->localPort = 0;
    d->localAddress.clear();
    d->peerPort = 0;
    d->peerAddress.clear();
}

bool QNativeSocketEngine::joinMulticastGroup(const QHostAddress &groupAddress, const QNetworkInterface &iface)
{
    Q_UNUSED(groupAddress);
    Q_UNUSED(iface);
    Q_UNIMPLEMENTED();
    return false;
}

bool QNativeSocketEngine::leaveMulticastGroup(const QHostAddress &groupAddress, const QNetworkInterface &iface)
{
    Q_UNUSED(groupAddress);
    Q_UNUSED(iface);
    Q_UNIMPLEMENTED();
    return false;
}

QNetworkInterface QNativeSocketEngine::multicastInterface() const
{
    Q_UNIMPLEMENTED();
    return QNetworkInterface();
}

bool QNativeSocketEngine::setMulticastInterface(const QNetworkInterface &iface)
{
    Q_UNUSED(iface);
    Q_UNIMPLEMENTED();
    return false;
}

qint64 QNativeSocketEngine::bytesAvailable() const
{
    Q_D(const QNativeSocketEngine);
    if (d->socketType != QAbstractSocket::TcpSocket)
        return -1;

    return d->readBytes.size() - d->readBytes.pos();
}

qint64 QNativeSocketEngine::read(char *data, qint64 maxlen)
{
    Q_D(QNativeSocketEngine);
    if (d->socketType != QAbstractSocket::TcpSocket)
        return -1;

    QMutexLocker mutexLocker(&d->readMutex);
    return d->readBytes.read(data, maxlen);
}

qint64 QNativeSocketEngine::write(const char *data, qint64 len)
{
    Q_D(QNativeSocketEngine);
    if (!isValid())
        return -1;

    HRESULT hr = E_FAIL;
    ComPtr<IOutputStream> stream;
    if (d->socketType == QAbstractSocket::TcpSocket)
        hr = d->tcpSocket()->get_OutputStream(&stream);
    else if (d->socketType == QAbstractSocket::UdpSocket)
        hr = d->udpSocket()->get_OutputStream(&stream);
    if (FAILED(hr)) {
        qErrnoWarning(hr, "Failed to get output stream to socket.");
        return -1;
    }

    ComPtr<IBuffer> buffer;
    hr = g->bufferFactory->Create(len, &buffer);
    Q_ASSERT_SUCCEEDED(hr);
    hr = buffer->put_Length(len);
    Q_ASSERT_SUCCEEDED(hr);
    ComPtr<Windows::Storage::Streams::IBufferByteAccess> byteArrayAccess;
    hr = buffer.As(&byteArrayAccess);
    Q_ASSERT_SUCCEEDED(hr);
    byte *bytes;
    hr = byteArrayAccess->Buffer(&bytes);
    Q_ASSERT_SUCCEEDED(hr);
    memcpy(bytes, data, len);
    ComPtr<IAsyncOperationWithProgress<UINT32, UINT32>> op;
    hr = stream->WriteAsync(buffer.Get(), &op);
    RETURN_IF_FAILED("Failed to write to stream", return -1);

    UINT32 bytesWritten;
    hr = QWinRTFunctions::await(op, &bytesWritten);
    if (FAILED(hr)) {
        d->setError(QAbstractSocket::SocketAccessError, QNativeSocketEnginePrivate::AccessErrorString);
        return -1;
    }

    if (bytesWritten && d->notifyOnWrite)
        emit writeReady();

    return bytesWritten;
}

qint64 QNativeSocketEngine::readDatagram(char *data, qint64 maxlen, QHostAddress *addr, quint16 *port)
{
    Q_D(QNativeSocketEngine);
    if (d->socketType != QAbstractSocket::UdpSocket || d->pendingDatagrams.isEmpty())
        return -1;

    WinRtDatagram datagram = d->pendingDatagrams.takeFirst();
    if (addr)
        *addr = datagram.address;

    if (port)
        *port = datagram.port;

    QByteArray readOrigin;
    // Do not read the whole datagram. Put the rest of it back into the "queue"
    if (maxlen < datagram.data.length()) {
        QByteArray readOrigin = datagram.data.left(maxlen);
        datagram.data = datagram.data.remove(0, maxlen);
        d->pendingDatagrams.prepend(datagram);
    } else {
        readOrigin = datagram.data;
    }
    strcpy(data, readOrigin);
    return readOrigin.length();
}

qint64 QNativeSocketEngine::writeDatagram(const char *data, qint64 len, const QHostAddress &addr, quint16 port)
{
    Q_D(QNativeSocketEngine);
    if (d->socketType != QAbstractSocket::UdpSocket)
        return -1;

    ComPtr<IHostName> remoteHost;
    ComPtr<IHostNameFactory> hostNameFactory;
    if (FAILED(GetActivationFactory(HString::MakeReference(RuntimeClass_Windows_Networking_HostName).Get(),
                                    &hostNameFactory))) {
        qWarning("QNativeSocketEnginePrivate::nativeSendDatagram: could not obtain hostname factory");
        return -1;
    }
    const QString addressString = addr.toString();
    HStringReference hostNameRef(reinterpret_cast<LPCWSTR>(addressString.utf16()));
    hostNameFactory->CreateHostName(hostNameRef.Get(), &remoteHost);

    ComPtr<IAsyncOperation<IOutputStream *>> streamOperation;
    ComPtr<IOutputStream> stream;
    const QString portString = QString::number(port);
    HStringReference portRef(reinterpret_cast<LPCWSTR>(portString.utf16()));
    if (FAILED(d->udpSocket()->GetOutputStreamAsync(remoteHost.Get(), portRef.Get(), &streamOperation)))
        return -1;
    HRESULT hr;
    while (hr = streamOperation->GetResults(&stream) == E_ILLEGAL_METHOD_CALL)
        QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
    ComPtr<IDataWriterFactory> dataWriterFactory;
    GetActivationFactory(HString::MakeReference(RuntimeClass_Windows_Storage_Streams_DataWriter).Get(), &dataWriterFactory);
    ComPtr<IDataWriter> writer;
    dataWriterFactory->CreateDataWriter(stream.Get(), &writer);
    writer->WriteBytes(len, (unsigned char *)data);
    return len;
}

bool QNativeSocketEngine::hasPendingDatagrams() const
{
    Q_D(const QNativeSocketEngine);
    return d->pendingDatagrams.length() > 0;
}

qint64 QNativeSocketEngine::pendingDatagramSize() const
{
    Q_D(const QNativeSocketEngine);
    if (d->pendingDatagrams.isEmpty())
        return -1;

    return d->pendingDatagrams.at(0).data.length();
}

qint64 QNativeSocketEngine::bytesToWrite() const
{
    return 0;
}

qint64 QNativeSocketEngine::receiveBufferSize() const
{
    Q_D(const QNativeSocketEngine);
    return d->option(QAbstractSocketEngine::ReceiveBufferSocketOption);
}

void QNativeSocketEngine::setReceiveBufferSize(qint64 bufferSize)
{
    Q_D(QNativeSocketEngine);
    d->setOption(QAbstractSocketEngine::ReceiveBufferSocketOption, bufferSize);
}

qint64 QNativeSocketEngine::sendBufferSize() const
{
    Q_D(const QNativeSocketEngine);
    return d->option(QAbstractSocketEngine::SendBufferSocketOption);
}

void QNativeSocketEngine::setSendBufferSize(qint64 bufferSize)
{
    Q_D(QNativeSocketEngine);
    d->setOption(QAbstractSocketEngine::SendBufferSocketOption, bufferSize);
}

int QNativeSocketEngine::option(QAbstractSocketEngine::SocketOption option) const
{
    Q_D(const QNativeSocketEngine);
    return d->option(option);
}

bool QNativeSocketEngine::setOption(QAbstractSocketEngine::SocketOption option, int value)
{
    Q_D(QNativeSocketEngine);
    return d->setOption(option, value);
}

bool QNativeSocketEngine::waitForRead(int msecs, bool *timedOut)
{
    Q_D(QNativeSocketEngine);
    Q_CHECK_VALID_SOCKETLAYER(QNativeSocketEngine::waitForRead(), false);
    Q_CHECK_NOT_STATE(QNativeSocketEngine::waitForRead(),
                      QAbstractSocket::UnconnectedState, false);

    if (timedOut)
        *timedOut = false;

    QElapsedTimer timer;
    timer.start();
    while (msecs > timer.elapsed()) {
        // Servers with active connections are ready for reading
        if (!d->currentConnections.isEmpty())
            return true;

        // If we are a client, we are ready to read if our buffer has data
        QMutexLocker locker(&d->readMutex);
        if (!d->readBytes.atEnd())
            return true;

        // Nothing to do, wait for more events
        d->eventLoop.processEvents();
    }

    d->setError(QAbstractSocket::SocketTimeoutError,
                QNativeSocketEnginePrivate::TimeOutErrorString);

    if (timedOut)
        *timedOut = true;
    return false;
}

bool QNativeSocketEngine::waitForWrite(int msecs, bool *timedOut)
{
    Q_UNUSED(msecs);
    Q_UNUSED(timedOut);
    return false;
}

bool QNativeSocketEngine::waitForReadOrWrite(bool *readyToRead, bool *readyToWrite, bool checkRead, bool checkWrite, int msecs, bool *timedOut)
{
    Q_UNUSED(readyToRead);
    Q_UNUSED(readyToWrite);
    Q_UNUSED(checkRead);
    Q_UNUSED(checkWrite);
    Q_UNUSED(msecs);
    Q_UNUSED(timedOut);
    return false;
}

bool QNativeSocketEngine::isReadNotificationEnabled() const
{
    Q_D(const QNativeSocketEngine);
    return d->notifyOnRead;
}

void QNativeSocketEngine::setReadNotificationEnabled(bool enable)
{
    Q_D(QNativeSocketEngine);
    d->notifyOnRead = enable;
}

bool QNativeSocketEngine::isWriteNotificationEnabled() const
{
    Q_D(const QNativeSocketEngine);
    return d->notifyOnWrite;
}

void QNativeSocketEngine::setWriteNotificationEnabled(bool enable)
{
    Q_D(QNativeSocketEngine);
    d->notifyOnWrite = enable;
    if (enable && d->socketState == QAbstractSocket::ConnectedState) {
        if (bytesToWrite())
            return; // will be emitted as a result of bytes written
        writeNotification();
        d->notifyOnWrite = false;
    }
}

bool QNativeSocketEngine::isExceptionNotificationEnabled() const
{
    Q_D(const QNativeSocketEngine);
    return d->notifyOnException;
}

void QNativeSocketEngine::setExceptionNotificationEnabled(bool enable)
{
    Q_D(QNativeSocketEngine);
    d->notifyOnException = enable;
}

void QNativeSocketEngine::establishRead()
{
    Q_D(QNativeSocketEngine);

    HRESULT hr;
    ComPtr<IInputStream> stream;
    hr = d->tcpSocket()->get_InputStream(&stream);
    RETURN_VOID_IF_FAILED("Failed to get socket input stream");

    ComPtr<IBuffer> buffer;
    hr = g->bufferFactory->Create(READ_BUFFER_SIZE, &buffer);
    Q_ASSERT_SUCCEEDED(hr);

    ComPtr<IAsyncBufferOperation> op;
    hr = stream->ReadAsync(buffer.Get(), READ_BUFFER_SIZE, InputStreamOptions_Partial, &op);
    RETURN_VOID_IF_FAILED("Failed to initiate socket read");
    hr = op->put_Completed(Callback<SocketReadCompletedHandler>(d, &QNativeSocketEnginePrivate::handleReadyRead).Get());
    Q_ASSERT_SUCCEEDED(hr);
}

bool QNativeSocketEnginePrivate::createNewSocket(QAbstractSocket::SocketType socketType, QAbstractSocket::NetworkLayerProtocol &socketProtocol)
{
    Q_UNUSED(socketProtocol);
    switch (socketType) {
    case QAbstractSocket::TcpSocket: {
        ComPtr<IStreamSocket> socket;
        HRESULT hr = RoActivateInstance(HString::MakeReference(RuntimeClass_Windows_Networking_Sockets_StreamSocket).Get(), &socket);
        if (FAILED(hr)) {
            qWarning("Failed to create StreamSocket instance");
            return false;
        }
        socketDescriptor = qintptr(socket.Detach());
        return true;
    }
    case QAbstractSocket::UdpSocket: {
        ComPtr<IDatagramSocket> socket;
        HRESULT hr = RoActivateInstance(HString::MakeReference(RuntimeClass_Windows_Networking_Sockets_DatagramSocket).Get(), &socket);
        if (FAILED(hr)) {
            qWarning("Failed to create stream socket");
            return false;
        }
        EventRegistrationToken token;
        socketDescriptor = qintptr(socket.Detach());
        udpSocket()->add_MessageReceived(Callback<DatagramReceivedHandler>(this, &QNativeSocketEnginePrivate::handleNewDatagram).Get(), &token);
        return true;
    }
    default:
        qWarning("Invalid socket type");
        return false;
    }
    return false;
}

QNativeSocketEnginePrivate::QNativeSocketEnginePrivate()
    : QAbstractSocketEnginePrivate()
    , notifyOnRead(true)
    , notifyOnWrite(true)
    , notifyOnException(false)
    , closingDown(false)
    , socketDescriptor(-1)
    , sslSocket(Q_NULLPTR)
{
}

QNativeSocketEnginePrivate::~QNativeSocketEnginePrivate()
{
}

void QNativeSocketEnginePrivate::setError(QAbstractSocket::SocketError error, ErrorString errorString) const
{
    if (hasSetSocketError) {
        // Only set socket errors once for one engine; expect the
        // socket to recreate its engine after an error. Note: There's
        // one exception: SocketError(11) bypasses this as it's purely
        // a temporary internal error condition.
        // Another exception is the way the waitFor*() functions set
        // an error when a timeout occurs. After the call to setError()
        // they reset the hasSetSocketError to false
        return;
    }
    if (error != QAbstractSocket::SocketError(11))
        hasSetSocketError = true;

    socketError = error;

    switch (errorString) {
    case NonBlockingInitFailedErrorString:
        socketErrorString = QNativeSocketEngine::tr("Unable to initialize non-blocking socket");
        break;
    case BroadcastingInitFailedErrorString:
        socketErrorString = QNativeSocketEngine::tr("Unable to initialize broadcast socket");
        break;
    // should not happen anymore
    case NoIpV6ErrorString:
        socketErrorString = QNativeSocketEngine::tr("Attempt to use IPv6 socket on a platform with no IPv6 support");
        break;
    case RemoteHostClosedErrorString:
        socketErrorString = QNativeSocketEngine::tr("The remote host closed the connection");
        break;
    case TimeOutErrorString:
        socketErrorString = QNativeSocketEngine::tr("Network operation timed out");
        break;
    case ResourceErrorString:
        socketErrorString = QNativeSocketEngine::tr("Out of resources");
        break;
    case OperationUnsupportedErrorString:
        socketErrorString = QNativeSocketEngine::tr("Unsupported socket operation");
        break;
    case ProtocolUnsupportedErrorString:
        socketErrorString = QNativeSocketEngine::tr("Protocol type not supported");
        break;
    case InvalidSocketErrorString:
        socketErrorString = QNativeSocketEngine::tr("Invalid socket descriptor");
        break;
    case HostUnreachableErrorString:
        socketErrorString = QNativeSocketEngine::tr("Host unreachable");
        break;
    case NetworkUnreachableErrorString:
        socketErrorString = QNativeSocketEngine::tr("Network unreachable");
        break;
    case AccessErrorString:
        socketErrorString = QNativeSocketEngine::tr("Permission denied");
        break;
    case ConnectionTimeOutErrorString:
        socketErrorString = QNativeSocketEngine::tr("Connection timed out");
        break;
    case ConnectionRefusedErrorString:
        socketErrorString = QNativeSocketEngine::tr("Connection refused");
        break;
    case AddressInuseErrorString:
        socketErrorString = QNativeSocketEngine::tr("The bound address is already in use");
        break;
    case AddressNotAvailableErrorString:
        socketErrorString = QNativeSocketEngine::tr("The address is not available");
        break;
    case AddressProtectedErrorString:
        socketErrorString = QNativeSocketEngine::tr("The address is protected");
        break;
    case DatagramTooLargeErrorString:
        socketErrorString = QNativeSocketEngine::tr("Datagram was too large to send");
        break;
    case SendDatagramErrorString:
        socketErrorString = QNativeSocketEngine::tr("Unable to send a message");
        break;
    case ReceiveDatagramErrorString:
        socketErrorString = QNativeSocketEngine::tr("Unable to receive a message");
        break;
    case WriteErrorString:
        socketErrorString = QNativeSocketEngine::tr("Unable to write");
        break;
    case ReadErrorString:
        socketErrorString = QNativeSocketEngine::tr("Network error");
        break;
    case PortInuseErrorString:
        socketErrorString = QNativeSocketEngine::tr("Another socket is already listening on the same port");
        break;
    case NotSocketErrorString:
        socketErrorString = QNativeSocketEngine::tr("Operation on non-socket");
        break;
    case InvalidProxyTypeString:
        socketErrorString = QNativeSocketEngine::tr("The proxy type is invalid for this operation");
        break;
    case TemporaryErrorString:
        socketErrorString = QNativeSocketEngine::tr("Temporary error");
        break;
    case UnknownSocketErrorString:
        socketErrorString = QNativeSocketEngine::tr("Unknown error");
        break;
    }
}

int QNativeSocketEnginePrivate::option(QAbstractSocketEngine::SocketOption opt) const
{
    ComPtr<IStreamSocketControl> control;
    if (socketType == QAbstractSocket::TcpSocket) {
        if (FAILED(tcpSocket()->get_Control(&control))) {
            qWarning("QNativeSocketEnginePrivate::option: Could not obtain socket control");
            return -1;
        }
    }
    switch (opt) {
    case QAbstractSocketEngine::NonBlockingSocketOption:
    case QAbstractSocketEngine::BroadcastSocketOption:
    case QAbstractSocketEngine::ReceiveOutOfBandData:
        return 1;
    case QAbstractSocketEngine::SendBufferSocketOption:
        if (socketType == QAbstractSocket::UdpSocket)
            return -1;

        UINT32 bufferSize;
        if (FAILED(control->get_OutboundBufferSizeInBytes(&bufferSize))) {
            qWarning("Could not obtain OutboundBufferSizeInBytes information vom socket control");
            return -1;
        }
        return bufferSize;
    case QAbstractSocketEngine::LowDelayOption:
        if (socketType == QAbstractSocket::UdpSocket)
            return -1;

        boolean noDelay;
        if (FAILED(control->get_NoDelay(&noDelay))) {
            qWarning("Could not obtain NoDelay information from socket control");
            return -1;
        }
        return noDelay;
    case QAbstractSocketEngine::KeepAliveOption:
        if (socketType == QAbstractSocket::UdpSocket)
            return -1;

        boolean keepAlive;
        if (FAILED(control->get_KeepAlive(&keepAlive))) {
            qWarning("Could not obtain KeepAlive information from socket control");
            return -1;
        }
        return keepAlive;
    case QAbstractSocketEngine::ReceiveBufferSocketOption:
    case QAbstractSocketEngine::AddressReusable:
    case QAbstractSocketEngine::BindExclusively:
    case QAbstractSocketEngine::MulticastTtlOption:
    case QAbstractSocketEngine::MulticastLoopbackOption:
    case QAbstractSocketEngine::TypeOfServiceOption:
    default:
        return -1;
    }
    return -1;
}

bool QNativeSocketEnginePrivate::setOption(QAbstractSocketEngine::SocketOption opt, int v)
{
    ComPtr<IStreamSocketControl> control;
    if (socketType == QAbstractSocket::TcpSocket) {
        if (FAILED(tcpSocket()->get_Control(&control))) {
            qWarning("QNativeSocketEnginePrivate::setOption: Could not obtain socket control");
            return false;
        }
    }
    switch (opt) {
    case QAbstractSocketEngine::NonBlockingSocketOption:
    case QAbstractSocketEngine::BroadcastSocketOption:
    case QAbstractSocketEngine::ReceiveOutOfBandData:
        return v != 0;
    case QAbstractSocketEngine::SendBufferSocketOption:
        if (socketType == QAbstractSocket::UdpSocket)
            return false;

        if (FAILED(control->put_OutboundBufferSizeInBytes(v))) {
            qWarning("Could not set OutboundBufferSizeInBytes");
            return false;
        }
        return true;
    case QAbstractSocketEngine::LowDelayOption: {
        if (socketType == QAbstractSocket::UdpSocket)
            return false;

        boolean noDelay = v;
        if (FAILED(control->put_NoDelay(noDelay))) {
            qWarning("Could not obtain NoDelay information from socket control");
            return false;
        }
        return true;
    }
    case QAbstractSocketEngine::KeepAliveOption: {
        if (socketType == QAbstractSocket::UdpSocket
                || socketState != QAbstractSocket::UnconnectedState)
            return false;

        boolean keepAlive = v;
        if (FAILED(control->put_KeepAlive(keepAlive))) {
            qWarning("Could not set KeepAlive value");
            return false;
        }
        return true;
    }
    case QAbstractSocketEngine::ReceiveBufferSocketOption:
    case QAbstractSocketEngine::AddressReusable:
    case QAbstractSocketEngine::BindExclusively:
    case QAbstractSocketEngine::MulticastTtlOption:
    case QAbstractSocketEngine::MulticastLoopbackOption:
    case QAbstractSocketEngine::TypeOfServiceOption:
    default:
        return false;
    }
    return false;
}

bool QNativeSocketEnginePrivate::fetchConnectionParameters()
{
    localPort = 0;
    localAddress.clear();
    peerPort = 0;
    peerAddress.clear();

    if (socketType == QAbstractSocket::TcpSocket) {
        ComPtr<IHostName> hostName;
        HString tmpHString;
        ComPtr<IStreamSocketInformation> info;
        if (FAILED(tcpSocket()->get_Information(&info))) {
            qWarning("QNativeSocketEnginePrivate::fetchConnectionParameters: Could not obtain socket info");
            return false;
        }
        info->get_LocalAddress(&hostName);
        if (hostName) {
            hostName->get_CanonicalName(tmpHString.GetAddressOf());
            localAddress.setAddress(qt_QStringFromHString(tmpHString));
            info->get_LocalPort(tmpHString.GetAddressOf());
            localPort = qt_QStringFromHString(tmpHString).toInt();
        }
        if (!localPort && tcpListener) {
            ComPtr<IStreamSocketListenerInformation> listenerInfo = 0;
            tcpListener->get_Information(&listenerInfo);
            listenerInfo->get_LocalPort(tmpHString.GetAddressOf());
            localPort = qt_QStringFromHString(tmpHString).toInt();
            localAddress == QHostAddress::Any;
        }
        info->get_RemoteAddress(&hostName);
        if (hostName) {
            hostName->get_CanonicalName(tmpHString.GetAddressOf());
            peerAddress.setAddress(qt_QStringFromHString(tmpHString));
            info->get_RemotePort(tmpHString.GetAddressOf());
            peerPort = qt_QStringFromHString(tmpHString).toInt();
        }
    } else if (socketType == QAbstractSocket::UdpSocket) {
        ComPtr<IHostName> hostName;
        HString tmpHString;
        ComPtr<IDatagramSocketInformation> info;
        if (FAILED(udpSocket()->get_Information(&info))) {
            qWarning("QNativeSocketEnginePrivate::fetchConnectionParameters: Could not obtain socket information");
            return false;
        }
        info->get_LocalAddress(&hostName);
        if (hostName) {
            hostName->get_CanonicalName(tmpHString.GetAddressOf());
            localAddress.setAddress(qt_QStringFromHString(tmpHString));
            info->get_LocalPort(tmpHString.GetAddressOf());
            localPort = qt_QStringFromHString(tmpHString).toInt();
        }

        info->get_RemoteAddress(&hostName);
        if (hostName) {
            hostName->get_CanonicalName(tmpHString.GetAddressOf());
            peerAddress.setAddress(qt_QStringFromHString(tmpHString));
            info->get_RemotePort(tmpHString.GetAddressOf());
            peerPort = qt_QStringFromHString(tmpHString).toInt();
        }
    }
    return true;
}

HRESULT QNativeSocketEnginePrivate::handleBindCompleted(IAsyncAction *, AsyncStatus)
{
    return S_OK;
}

HRESULT QNativeSocketEnginePrivate::handleClientConnection(IStreamSocketListener *listener, IStreamSocketListenerConnectionReceivedEventArgs *args)
{
    Q_Q(QNativeSocketEngine);
    Q_UNUSED(listener)
    IStreamSocket *socket;
    args->get_Socket(&socket);
    pendingConnections.append(socket);
    emit q->connectionReady();
    emit q->readReady();
    return S_OK;
}

HRESULT QNativeSocketEnginePrivate::handleConnectToHost(IAsyncAction *action, AsyncStatus)
{
    Q_Q(QNativeSocketEngine);

    HRESULT hr = action->GetResults();
    if (wasDeleted || !connectOp) // Protect against a late callback
        return S_OK;

    connectOp.Reset();
    switch (hr) {
    case 0x8007274c: // A connection attempt failed because the connected party did not properly respond after a period of time, or established connection failed because connected host has failed to respond.
        setError(QAbstractSocket::NetworkError, ConnectionTimeOutErrorString);
        socketState = QAbstractSocket::UnconnectedState;
        return S_OK;
    case 0x80072751: // A socket operation was attempted to an unreachable host.
        setError(QAbstractSocket::HostNotFoundError, HostUnreachableErrorString);
        socketState = QAbstractSocket::UnconnectedState;
        return S_OK;
    case 0x8007274d: // No connection could be made because the target machine actively refused it.
        setError(QAbstractSocket::ConnectionRefusedError, ConnectionRefusedErrorString);
        socketState = QAbstractSocket::UnconnectedState;
        return S_OK;
    default:
        if (FAILED(hr)) {
            setError(QAbstractSocket::UnknownSocketError, UnknownSocketErrorString);
            socketState = QAbstractSocket::UnconnectedState;
            return S_OK;
        }
        break;
    }

    socketState = QAbstractSocket::ConnectedState;
    emit q->connectionReady();

    // Delay the reader so that the SSL socket can upgrade
    if (sslSocket)
        q->connect(sslSocket, SIGNAL(encrypted()), SLOT(establishRead()));
    else
        q->establishRead();

    return S_OK;
}

HRESULT QNativeSocketEnginePrivate::handleReadyRead(IAsyncBufferOperation *asyncInfo, AsyncStatus status)
{
    Q_Q(QNativeSocketEngine);
    if (wasDeleted || isDeletingChildren)
        return S_OK;

    if (status == Error || status == Canceled)
        return S_OK;

    ComPtr<IBuffer> buffer;
    HRESULT hr = asyncInfo->GetResults(&buffer);
    RETURN_OK_IF_FAILED("Failed to get read results buffer");

    UINT32 bufferLength;
    hr = buffer->get_Length(&bufferLength);
    Q_ASSERT_SUCCEEDED(hr);
    if (!bufferLength) {
        if (q->isReadNotificationEnabled())
            emit q->readReady();
        return S_OK;
    }

    ComPtr<Windows::Storage::Streams::IBufferByteAccess> byteArrayAccess;
    hr = buffer.As(&byteArrayAccess);
    Q_ASSERT_SUCCEEDED(hr);
    byte *data;
    hr = byteArrayAccess->Buffer(&data);
    Q_ASSERT_SUCCEEDED(hr);

    readMutex.lock();
    if (readBytes.atEnd()) // Everything has been read; the buffer is safe to reset
        readBytes.close();
    if (!readBytes.isOpen())
        readBytes.open(QBuffer::ReadWrite|QBuffer::Truncate);
    qint64 readPos = readBytes.pos();
    readBytes.seek(readBytes.size());
    Q_ASSERT(readBytes.atEnd());
    readBytes.write(reinterpret_cast<const char*>(data), qint64(bufferLength));
    readBytes.seek(readPos);
    readMutex.unlock();

    if (q->isReadNotificationEnabled())
        emit q->readReady();

    ComPtr<IInputStream> stream;
    hr = tcpSocket()->get_InputStream(&stream);
    Q_ASSERT_SUCCEEDED(hr);

    // Reuse the stream buffer
    hr = buffer->get_Capacity(&bufferLength);
    Q_ASSERT_SUCCEEDED(hr);
    hr = buffer->put_Length(0);
    Q_ASSERT_SUCCEEDED(hr);

    ComPtr<IAsyncBufferOperation> op;
    hr = stream->ReadAsync(buffer.Get(), bufferLength, InputStreamOptions_Partial, &op);
    if (FAILED(hr)) {
        qErrnoWarning(hr, "Could not read into socket stream buffer.");
        return S_OK;
    }
    hr = op->put_Completed(Callback<SocketReadCompletedHandler>(this, &QNativeSocketEnginePrivate::handleReadyRead).Get());
    if (FAILED(hr)) {
        qErrnoWarning(hr, "Failed to set socket read callback.");
        return S_OK;
    }
    return S_OK;
}

HRESULT QNativeSocketEnginePrivate::handleNewDatagram(IDatagramSocket *socket, IDatagramSocketMessageReceivedEventArgs *args)
{
    Q_Q(QNativeSocketEngine);
    Q_UNUSED(socket);

    WinRtDatagram datagram;
    QHostAddress returnAddress;
    ComPtr<IHostName> remoteHost;
    HRESULT hr = args->get_RemoteAddress(&remoteHost);
    RETURN_OK_IF_FAILED("Could not obtain remote host");
    HString remoteHostString;
    remoteHost->get_CanonicalName(remoteHostString.GetAddressOf());
    RETURN_OK_IF_FAILED("Could not obtain remote host's canonical name");
    returnAddress.setAddress(qt_QStringFromHString(remoteHostString));
    datagram.address = returnAddress;
    HString remotePort;
    hr = args->get_RemotePort(remotePort.GetAddressOf());
    RETURN_OK_IF_FAILED("Could not obtain remote port");
    datagram.port = qt_QStringFromHString(remotePort).toInt();

    ComPtr<IDataReader> reader;
    hr = args->GetDataReader(&reader);
    RETURN_OK_IF_FAILED("Could not obtain data reader");
    quint32 length;
    hr = reader->get_UnconsumedBufferLength(&length);
    RETURN_OK_IF_FAILED("Could not obtain unconsumed buffer length");
    datagram.data.resize(length);
    hr = reader->ReadBytes(length, reinterpret_cast<BYTE *>(datagram.data.data()));
    RETURN_OK_IF_FAILED("Could not read datagram");
    pendingDatagrams.append(datagram);
    emit q->readReady();

    return S_OK;
}

QT_END_NAMESPACE
