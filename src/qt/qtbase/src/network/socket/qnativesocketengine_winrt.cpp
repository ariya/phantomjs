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

#include <private/qeventdispatcher_winrt_p.h>

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

QString qt_QStringFromHSTRING(HSTRING string)
{
    UINT32 length;
    PCWSTR rawString = WindowsGetStringRawBuffer(string, &length);
    return QString::fromWCharArray(rawString, length);
}

#define READ_BUFFER_SIZE 8192

class ByteArrayBuffer : public Microsoft::WRL::RuntimeClass<RuntimeClassFlags<WinRtClassicComMix>,
        IBuffer, Windows::Storage::Streams::IBufferByteAccess>
{
public:
    ByteArrayBuffer(int size) : m_bytes(size, Qt::Uninitialized), m_length(0)
    {
    }

    ByteArrayBuffer(const char *data, int size) : m_bytes(data, size), m_length(size)
    {
    }

    HRESULT __stdcall Buffer(byte **value)
    {
        *value = reinterpret_cast<byte *>(m_bytes.data());
        return S_OK;
    }

    HRESULT __stdcall get_Capacity(UINT32 *value)
    {
        *value = m_bytes.size();
        return S_OK;
    }

    HRESULT __stdcall get_Length(UINT32 *value)
    {
        *value = m_length;
        return S_OK;
    }

    HRESULT __stdcall put_Length(UINT32 value)
    {
        Q_ASSERT(value <= UINT32(m_bytes.size()));
        m_length = value;
        return S_OK;
    }

    ComPtr<IInputStream> inputStream() const
    {
        return m_stream;
    }

    void setInputStream(ComPtr<IInputStream> stream)
    {
        m_stream = stream;
    }

private:
    QByteArray m_bytes;
    UINT32 m_length;
    ComPtr<IInputStream> m_stream;
};

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

    d->socketDescriptor = socketDescriptor;

    // Currently, only TCP sockets are initialized this way.
    SocketHandler *handler = gSocketHandler();
    d->tcp = handler->pendingTcpSockets.take(socketDescriptor);
    d->socketType = QAbstractSocket::TcpSocket;

    if (!d->tcp || !d->fetchConnectionParameters())
        return false;

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

    ComPtr<IAsyncAction> op;
    const QString portString = QString::number(port);
    HStringReference portReference(reinterpret_cast<LPCWSTR>(portString.utf16()));
    HRESULT hr = E_FAIL;
    if (d->socketType == QAbstractSocket::TcpSocket)
        hr = d->tcp->ConnectAsync(remoteHost.Get(), portReference.Get(), &op);
    else if (d->socketType == QAbstractSocket::UdpSocket)
        hr = d->udp->ConnectAsync(remoteHost.Get(), portReference.Get(), &op);
    if (FAILED(hr)) {
        qWarning("QNativeSocketEnginePrivate::nativeConnect:: Could not obtain connect action");
        return false;
    }

    hr = op->put_Completed(Callback<IAsyncActionCompletedHandler>(
                               d, &QNativeSocketEnginePrivate::handleConnectToHost).Get());
    if (FAILED(hr)) {
        qErrnoWarning(hr, "Unable to set host connection callback.");
        return false;
    }
    d->socketState = QAbstractSocket::ConnectingState;
    while (opStatus(op) == Started)
        d->eventLoop.processEvents();

    AsyncStatus status = opStatus(op);
    if (status == Error || status == Canceled)
        return false;

    if (hr == 0x8007274c) { // A connection attempt failed because the connected party did not properly respond after a period of time, or established connection failed because connected host has failed to respond.
        d->setError(QAbstractSocket::NetworkError, d->ConnectionTimeOutErrorString);
        d->socketState = QAbstractSocket::UnconnectedState;
        return false;
    }
    if (hr == 0x8007274d) { // No connection could be made because the target machine actively refused it.
        d->setError(QAbstractSocket::ConnectionRefusedError, d->ConnectionRefusedErrorString);
        d->socketState = QAbstractSocket::UnconnectedState;
        return false;
    }
    if (FAILED(hr)) {
        d->setError(QAbstractSocket::UnknownSocketError, d->UnknownSocketErrorString);
        d->socketState = QAbstractSocket::UnconnectedState;
        return false;
    }

    if (d->socketType == QAbstractSocket::TcpSocket) {
        IInputStream *stream;
        hr = d->tcp->get_InputStream(&stream);
        if (FAILED(hr))
            return false;
        ByteArrayBuffer *buffer = static_cast<ByteArrayBuffer *>(d->readBuffer.Get());
        buffer->setInputStream(stream);
        ComPtr<IAsyncBufferOperation> op;
        hr = stream->ReadAsync(buffer, READ_BUFFER_SIZE, InputStreamOptions_Partial, &op);
        if (FAILED(hr))
            return false;
        hr = op->put_Completed(Callback<SocketReadCompletedHandler>(d, &QNativeSocketEnginePrivate::handleReadyRead).Get());
        if (FAILED(hr)) {
            qErrnoWarning(hr, "Failed to set socket read callback.");
            return false;
        }
    }
    d->socketState = QAbstractSocket::ConnectedState;
    return true;
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
        hr = d->udp->BindEndpointAsync(hostAddress.Get(), portString.Get(), &op);
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

        IInputStream *stream;
        socket->get_InputStream(&stream);
        // TODO: delete buffer and stream on socket close
        ByteArrayBuffer *buffer = static_cast<ByteArrayBuffer *>(d->readBuffer.Get());
        buffer->setInputStream(stream);
        ComPtr<IAsyncBufferOperation> op;
        HRESULT hr = stream->ReadAsync(buffer, READ_BUFFER_SIZE, InputStreamOptions_Partial, &op);
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
    if (d->socketDescriptor != -1) {
        IClosable *socket = 0;
        if (d->socketType == QAbstractSocket::TcpSocket)
            d->tcp->QueryInterface(IID_PPV_ARGS(&socket));
        else if (d->socketType == QAbstractSocket::UdpSocket)
            d->udp->QueryInterface(IID_PPV_ARGS(&socket));

        if (socket) {
            d->closingDown = true;
            socket->Close();
            socket->Release();
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
    HRESULT hr = E_FAIL;
    ComPtr<IOutputStream> stream;
    if (d->socketType == QAbstractSocket::TcpSocket)
        hr = d->tcp->get_OutputStream(&stream);
    else if (d->socketType == QAbstractSocket::UdpSocket)
        hr = d->udp->get_OutputStream(&stream);
    if (FAILED(hr)) {
        qErrnoWarning(hr, "Failed to get output stream to socket.");
        return -1;
    }

    ComPtr<ByteArrayBuffer> buffer = Make<ByteArrayBuffer>(data, len);
    ComPtr<IAsyncOperationWithProgress<UINT32, UINT32>> op;
    hr = stream->WriteAsync(buffer.Get(), &op);
    if (FAILED(hr)) {
        qErrnoWarning(hr, "Failed to write to socket.");
        return -1;
    }
    hr = op->put_Completed(Callback<IAsyncOperationWithProgressCompletedHandler<UINT32, UINT32>>(
                               d, &QNativeSocketEnginePrivate::handleWriteCompleted).Get());
    if (FAILED(hr)) {
        qErrnoWarning(hr, "Failed to set socket write callback.");
        return -1;
    }

    while (opStatus(op) == Started)
        d->eventLoop.processEvents();

    AsyncStatus status = opStatus(op);
    if (status == Error || status == Canceled)
        return -1;

    UINT32 bytesWritten;
    hr = op->GetResults(&bytesWritten);
    if (FAILED(hr)) {
        qErrnoWarning(hr, "Failed to get written socket length.");
        return -1;
    }

    if (bytesWritten && d->notifyOnWrite)
        emit writeReady();

    return bytesWritten;
}

qint64 QNativeSocketEngine::readDatagram(char *data, qint64 maxlen, QHostAddress *addr, quint16 *port)
{
    Q_D(QNativeSocketEngine);
    if (d->socketType != QAbstractSocket::UdpSocket)
        return -1;

    QHostAddress returnAddress;
    quint16 returnPort;

    for (int i = 0; i < d->pendingDatagrams.size(); ++i) {
        IDatagramSocketMessageReceivedEventArgs *arg = d->pendingDatagrams.at(i);
        ComPtr<IHostName> remoteHost;
        HSTRING remoteHostString;
        HSTRING remotePort;
        arg->get_RemoteAddress(&remoteHost);
        arg->get_RemotePort(&remotePort);
        remoteHost->get_CanonicalName(&remoteHostString);
        returnAddress.setAddress(qt_QStringFromHSTRING(remoteHostString));
        returnPort = qt_QStringFromHSTRING(remotePort).toInt();
        ComPtr<IDataReader> reader;
        arg->GetDataReader(&reader);
        if (!reader)
            continue;

        BYTE buffer[1024];
        reader->ReadBytes(maxlen, buffer);
        *addr = returnAddress;
        *port = returnPort;
        arg = d->pendingDatagrams.takeFirst();

        // TODO: fill data
        Q_UNUSED(data);
        arg->Release();
        delete arg;
        --i;
        return maxlen;
    }

    return -1;
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
    if (FAILED(d->udp->GetOutputStreamAsync(remoteHost.Get(), portRef.Get(), &streamOperation)))
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
    qint64 ret = 0;
    foreach (IDatagramSocketMessageReceivedEventArgs *arg, d->pendingDatagrams) {
        ComPtr<IDataReader> reader;
        UINT32 unconsumedBufferLength;
        arg->GetDataReader(&reader);
        if (!reader)
            return -1;
        reader->get_UnconsumedBufferLength(&unconsumedBufferLength);
        ret += unconsumedBufferLength;
    }
    return ret;
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

bool QNativeSocketEnginePrivate::createNewSocket(QAbstractSocket::SocketType socketType, QAbstractSocket::NetworkLayerProtocol &socketProtocol)
{
    Q_UNUSED(socketProtocol);
    SocketHandler *handler = gSocketHandler();
    switch (socketType) {
    case QAbstractSocket::TcpSocket: {
        if (FAILED(RoActivateInstance(HString::MakeReference(RuntimeClass_Windows_Networking_Sockets_StreamSocket).Get(),
                                      reinterpret_cast<IInspectable **>(&tcp)))) {
            qWarning("Failed to create StreamSocket instance");
            return false;
        }
        socketDescriptor = ++handler->socketCount;
        return true;
    }
    case QAbstractSocket::UdpSocket: {
        if (FAILED(RoActivateInstance(HString::MakeReference(RuntimeClass_Windows_Networking_Sockets_DatagramSocket).Get(),
                                      reinterpret_cast<IInspectable **>(&udp)))) {
            qWarning("Failed to create stream socket");
            return false;
        }
        EventRegistrationToken token;
        udp->add_MessageReceived(Callback<DatagramReceivedHandler>(this, &QNativeSocketEnginePrivate::handleNewDatagram).Get(), &token);
        socketDescriptor = ++handler->socketCount;
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
{
    ComPtr<ByteArrayBuffer> buffer = Make<ByteArrayBuffer>(READ_BUFFER_SIZE);
    readBuffer = buffer;
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
        if (FAILED(tcp->get_Control(&control))) {
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
        if (FAILED(tcp->get_Control(&control))) {
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
        if (socketType == QAbstractSocket::UdpSocket)
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
        HSTRING tmpHString;
        ComPtr<IStreamSocketInformation> info;
        if (FAILED(tcp->get_Information(&info))) {
            qWarning("QNativeSocketEnginePrivate::fetchConnectionParameters: Could not obtain socket info");
            return false;
        }
        info->get_LocalAddress(&hostName);
        if (hostName) {
            hostName->get_CanonicalName(&tmpHString);
            localAddress.setAddress(qt_QStringFromHSTRING(tmpHString));
            info->get_LocalPort(&tmpHString);
            localPort = qt_QStringFromHSTRING(tmpHString).toInt();
        }
        if (!localPort && tcpListener) {
            ComPtr<IStreamSocketListenerInformation> listenerInfo = 0;
            tcpListener->get_Information(&listenerInfo);
            listenerInfo->get_LocalPort(&tmpHString);
            localPort = qt_QStringFromHSTRING(tmpHString).toInt();
            localAddress == QHostAddress::Any;
        }
        info->get_RemoteAddress(&hostName);
        if (hostName) {
            hostName->get_CanonicalName(&tmpHString);
            peerAddress.setAddress(qt_QStringFromHSTRING(tmpHString));
            info->get_RemotePort(&tmpHString);
            peerPort = qt_QStringFromHSTRING(tmpHString).toInt();
        }
    } else if (socketType == QAbstractSocket::UdpSocket) {
        ComPtr<IHostName> hostName;
        HSTRING tmpHString;
        ComPtr<IDatagramSocketInformation> info;
        if (FAILED(udp->get_Information(&info))) {
            qWarning("QNativeSocketEnginePrivate::fetchConnectionParameters: Could not obtain socket information");
            return false;
        }
        info->get_LocalAddress(&hostName);
        if (hostName) {
            hostName->get_CanonicalName(&tmpHString);
            localAddress.setAddress(qt_QStringFromHSTRING(tmpHString));
            info->get_LocalPort(&tmpHString);
            localPort = qt_QStringFromHSTRING(tmpHString).toInt();
        }

        info->get_RemoteAddress(&hostName);
        if (hostName) {
            hostName->get_CanonicalName(&tmpHString);
            peerAddress.setAddress(qt_QStringFromHSTRING(tmpHString));
            info->get_RemotePort(&tmpHString);
            peerPort = qt_QStringFromHSTRING(tmpHString).toInt();
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
    Q_ASSERT(tcpListener.Get() == listener);
    IStreamSocket *socket;
    args->get_Socket(&socket);
    pendingConnections.append(socket);
    emit q->connectionReady();
    emit q->readReady();
    return S_OK;
}

HRESULT QNativeSocketEnginePrivate::handleConnectToHost(ABI::Windows::Foundation::IAsyncAction *, ABI::Windows::Foundation::AsyncStatus)
{
    return S_OK;
}

HRESULT QNativeSocketEnginePrivate::handleReadyRead(IAsyncBufferOperation *asyncInfo, AsyncStatus status)
{
    Q_Q(QNativeSocketEngine);
    if (wasDeleted || isDeletingChildren)
        return S_OK;

    if (status == Error || status == Canceled)
        return S_OK;

    ByteArrayBuffer *buffer = 0;
    HRESULT hr = asyncInfo->GetResults((IBuffer **)&buffer);
    if (FAILED(hr)) {
        qErrnoWarning(hr, "Failed to get ready read results.");
        return S_OK;
    }
    UINT32 len;
    buffer->get_Length(&len);
    if (!len) {
        if (q->isReadNotificationEnabled())
            emit q->readReady();
        return S_OK;
    }

    byte *data;
    buffer->Buffer(&data);

    readMutex.lock();
    if (readBytes.atEnd()) // Everything has been read; the buffer is safe to reset
        readBytes.close();
    if (!readBytes.isOpen())
        readBytes.open(QBuffer::ReadWrite|QBuffer::Truncate);
    qint64 readPos = readBytes.pos();
    readBytes.seek(readBytes.size());
    Q_ASSERT(readBytes.atEnd());
    readBytes.write(reinterpret_cast<const char*>(data), qint64(len));
    readBytes.seek(readPos);
    readMutex.unlock();

    if (q->isReadNotificationEnabled())
        emit q->readReady();

    ComPtr<IAsyncBufferOperation> op;
    hr = buffer->inputStream()->ReadAsync(buffer, READ_BUFFER_SIZE, InputStreamOptions_Partial, &op);
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

HRESULT QNativeSocketEnginePrivate::handleWriteCompleted(IAsyncOperationWithProgress<UINT32, UINT32> *op, AsyncStatus status)
{
    if (status == Error) {
        ComPtr<IAsyncInfo> info;
        HRESULT hr = op->QueryInterface(IID_PPV_ARGS(&info));
        if (FAILED(hr)) {
            qErrnoWarning(hr, "Failed to cast operation.");
            return S_OK;
        }
        HRESULT errorCode;
        hr = info->get_ErrorCode(&errorCode);
        if (FAILED(hr)) {
            qErrnoWarning(hr, "Failed to get error code.");
            return S_OK;
        }
        qErrnoWarning(errorCode, "A socket error occurred.");
        return S_OK;
    }

    return S_OK;
}

HRESULT QNativeSocketEnginePrivate::handleNewDatagram(IDatagramSocket *socket, IDatagramSocketMessageReceivedEventArgs *args)
{
    Q_Q(QNativeSocketEngine);
    Q_ASSERT(udp == socket);
    pendingDatagrams.append(args);
    emit q->readReady();

    return S_OK;
}

QT_END_NAMESPACE
