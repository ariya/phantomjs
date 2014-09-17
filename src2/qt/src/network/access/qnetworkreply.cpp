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

#include "qnetworkreply.h"
#include "qnetworkreply_p.h"
#include <QtNetwork/qsslconfiguration.h>

QT_BEGIN_NAMESPACE

QNetworkReplyPrivate::QNetworkReplyPrivate()
    : readBufferMaxSize(0),
      operation(QNetworkAccessManager::UnknownOperation),
      errorCode(QNetworkReply::NoError)
    , isFinished(false)
{
    // set the default attribute values
    attributes.insert(QNetworkRequest::ConnectionEncryptedAttribute, false);
}


/*!
    \class QNetworkReply
    \since 4.4
    \brief The QNetworkReply class contains the data and headers for a request
    sent with QNetworkAccessManager

    \reentrant
    \ingroup network
    \inmodule QtNetwork

    The QNetworkReply class contains the data and meta data related to
    a request posted with QNetworkAccessManager. Like QNetworkRequest,
    it contains a URL and headers (both in parsed and raw form), some
    information about the reply's state and the contents of the reply
    itself.

    QNetworkReply is a sequential-access QIODevice, which means that
    once data is read from the object, it no longer kept by the
    device. It is therefore the application's responsibility to keep
    this data if it needs to. Whenever more data is received from the
    network and processed, the readyRead() signal is emitted.

    The downloadProgress() signal is also emitted when data is
    received, but the number of bytes contained in it may not
    represent the actual bytes received, if any transformation is done
    to the contents (for example, decompressing and removing the
    protocol overhead).

    Even though QNetworkReply is a QIODevice connected to the contents
    of the reply, it also emits the uploadProgress() signal, which
    indicates the progress of the upload for operations that have such
    content.

    \note Do not delete the object in the slot connected to the
    error() or finished() signal. Use deleteLater().

    \sa QNetworkRequest, QNetworkAccessManager
*/

/*!
    \enum QNetworkReply::NetworkError

    Indicates all possible error conditions found during the
    processing of the request.

    \value NoError              no error condition.
    \note When the HTTP protocol returns a redirect no error will be
    reported.  You can check if there is a redirect with the
    QNetworkRequest::RedirectionTargetAttribute attribute.

    \value ConnectionRefusedError  the remote server refused the
    connection (the server is not accepting requests)

    \value RemoteHostClosedError   the remote server closed the
    connection prematurely, before the entire reply was received and
    processed

    \value HostNotFoundError       the remote host name was not found
    (invalid hostname)

    \value TimeoutError            the connection to the remote server
    timed out

    \value OperationCanceledError  the operation was canceled via calls
    to abort() or close() before it was finished.

    \value SslHandshakeFailedError the SSL/TLS handshake failed and the
    encrypted channel could not be established. The sslErrors() signal
    should have been emitted.

    \value TemporaryNetworkFailureError the connection was broken due
    to disconnection from the network, however the system has initiated
    roaming to another access point. The request should be resubmitted
    and will be processed as soon as the connection is re-established.

    \value ProxyConnectionRefusedError the connection to the proxy
    server was refused (the proxy server is not accepting requests)

    \value ProxyConnectionClosedError  the proxy server closed the
    connection prematurely, before the entire reply was received and
    processed

    \value ProxyNotFoundError          the proxy host name was not
    found (invalid proxy hostname)

    \value ProxyTimeoutError           the connection to the proxy
    timed out or the proxy did not reply in time to the request sent

    \value ProxyAuthenticationRequiredError the proxy requires
    authentication in order to honour the request but did not accept
    any credentials offered (if any)

    \value ContentAccessDenied          the access to the remote
    content was denied (similar to HTTP error 401)

    \value ContentOperationNotPermittedError the operation requested
    on the remote content is not permitted

    \value ContentNotFoundError         the remote content was not
    found at the server (similar to HTTP error 404)

    \value AuthenticationRequiredError  the remote server requires
    authentication to serve the content but the credentials provided
    were not accepted (if any)

    \value ContentReSendError          the request needed to be sent
    again, but this failed for example because the upload data
    could not be read a second time.

    \value ProtocolUnknownError         the Network Access API cannot
    honor the request because the protocol is not known

    \value ProtocolInvalidOperationError the requested operation is
    invalid for this protocol

    \value UnknownNetworkError          an unknown network-related
    error was detected

    \value UnknownProxyError            an unknown proxy-related error
    was detected

    \value UnknownContentError          an unknown error related to
    the remote content was detected

    \value ProtocolFailure              a breakdown in protocol was
    detected (parsing error, invalid or unexpected responses, etc.)

    \sa error()
*/

/*!
    \fn void QNetworkReply::sslErrors(const QList<QSslError> &errors)

    This signal is emitted if the SSL/TLS session encountered errors
    during the set up, including certificate verification errors. The
    \a errors parameter contains the list of errors.

    To indicate that the errors are not fatal and that the connection
    should proceed, the ignoreSslErrors() function should be called
    from the slot connected to this signal. If it is not called, the
    SSL session will be torn down before any data is exchanged
    (including the URL).

    This signal can be used to display an error message to the user
    indicating that security may be compromised and display the
    SSL settings (see sslConfiguration() to obtain it). If the user
    decides to proceed after analyzing the remote certificate, the
    slot should call ignoreSslErrors().

    \sa QSslSocket::sslErrors(), QNetworkAccessManager::sslErrors(),
    sslConfiguration(), ignoreSslErrors()
*/

/*!
    \fn void QNetworkReply::metaDataChanged()

    \omit FIXME: Update name? \endomit

    This signal is emitted whenever the metadata in this reply
    changes. metadata is any information that is not the content
    (data) itself, including the network headers. In the majority of
    cases, the metadata will be known fully by the time the first
    byte of data is received. However, it is possible to receive
    updates of headers or other metadata during the processing of the
    data.

    \sa header(), rawHeaderList(), rawHeader(), hasRawHeader()
*/

/*!
    \fn void QNetworkReply::finished()

    This signal is emitted when the reply has finished
    processing. After this signal is emitted, there will be no more
    updates to the reply's data or metadata.

    Unless close() has been called, the reply will be still be opened
    for reading, so the data can be retrieved by calls to read() or
    readAll(). In particular, if no calls to read() were made as a
    result of readyRead(), a call to readAll() will retrieve the full
    contents in a QByteArray.

    This signal is emitted in tandem with
    QNetworkAccessManager::finished() where that signal's reply
    parameter is this object.

    \note Do not delete the object in the slot connected to this
    signal. Use deleteLater().

    You can also use isFinished() to check if a QNetworkReply
    has finished even before you receive the finished() signal.

    \sa QNetworkAccessManager::finished(), isFinished()
*/

/*!
    \fn void QNetworkReply::error(QNetworkReply::NetworkError code)

    This signal is emitted when the reply detects an error in
    processing. The finished() signal will probably follow, indicating
    that the connection is over.

    The \a code parameter contains the code of the error that was
    detected. Call errorString() to obtain a textual representation of
    the error condition.

    \note Do not delete the object in the slot connected to this
    signal. Use deleteLater().

    \sa error(), errorString()
*/

/*!
    \fn void QNetworkReply::uploadProgress(qint64 bytesSent, qint64 bytesTotal)

    This signal is emitted to indicate the progress of the upload part
    of this network request, if there's any. If there's no upload
    associated with this request, this signal will not be emitted.

    The \a bytesSent
    parameter indicates the number of bytes uploaded, while \a
    bytesTotal indicates the total number of bytes to be uploaded. If
    the number of bytes to be uploaded could not be determined, \a
    bytesTotal will be -1.

    The upload is finished when \a bytesSent is equal to \a
    bytesTotal. At that time, \a bytesTotal will not be -1.

    \sa downloadProgress()
*/

/*!
    \fn void QNetworkReply::downloadProgress(qint64 bytesReceived, qint64 bytesTotal)

    This signal is emitted to indicate the progress of the download
    part of this network request, if there's any. If there's no
    download associated with this request, this signal will be emitted
    once with 0 as the value of both \a bytesReceived and \a
    bytesTotal.

    The \a bytesReceived parameter indicates the number of bytes
    received, while \a bytesTotal indicates the total number of bytes
    expected to be downloaded. If the number of bytes to be downloaded
    is not known, \a bytesTotal will be -1.

    The download is finished when \a bytesReceived is equal to \a
    bytesTotal. At that time, \a bytesTotal will not be -1.

    Note that the values of both \a bytesReceived and \a bytesTotal
    may be different from size(), the total number of bytes
    obtained through read() or readAll(), or the value of the
    header(ContentLengthHeader). The reason for that is that there may
    be protocol overhead or the data may be compressed during the
    download.

    \sa uploadProgress(), bytesAvailable()
*/

/*!
    \fn void QNetworkReply::abort()

    Aborts the operation immediately and close down any network
    connections still open. Uploads still in progress are also
    aborted.

    \sa close()
*/

/*!
    Creates a QNetworkReply object with parent \a parent.

    You cannot directly instantiate QNetworkReply objects. Use
    QNetworkAccessManager functions to do that.
*/
QNetworkReply::QNetworkReply(QObject *parent)
    : QIODevice(*new QNetworkReplyPrivate, parent)
{
}

/*!
    \internal
*/
QNetworkReply::QNetworkReply(QNetworkReplyPrivate &dd, QObject *parent)
    : QIODevice(dd, parent)
{
}

/*!
    Disposes of this reply and frees any resources associated with
    it. If any network connections are still open, they will be
    closed.

    \sa abort(), close()
*/
QNetworkReply::~QNetworkReply()
{
}

/*!
    Closes this device for reading. Unread data is discarded, but the
    network resources are not discarded until they are finished. In
    particular, if any upload is in progress, it will continue until
    it is done.

    The finished() signal is emitted when all operations are over and
    the network resources are freed.

    \sa abort(), finished()
*/
void QNetworkReply::close()
{
    QIODevice::close();
}

/*!
    \internal
*/
bool QNetworkReply::isSequential() const
{
    return true;
}

/*!
    Returns the size of the read buffer, in bytes.

    \sa setReadBufferSize()
*/
qint64 QNetworkReply::readBufferSize() const
{
    return d_func()->readBufferMaxSize;
}

/*!
    Sets the size of the read buffer to be \a size bytes. The read
    buffer is the buffer that holds data that is being downloaded off
    the network, before it is read with QIODevice::read(). Setting the
    buffer size to 0 will make the buffer unlimited in size.

    QNetworkReply will try to stop reading from the network once this
    buffer is full (i.e., bytesAvailable() returns \a size or more),
    thus causing the download to throttle down as well. If the buffer
    is not limited in size, QNetworkReply will try to download as fast
    as possible from the network.

    Unlike QAbstractSocket::setReadBufferSize(), QNetworkReply cannot
    guarantee precision in the read buffer size. That is,
    bytesAvailable() can return more than \a size.

    \sa readBufferSize()
*/
void QNetworkReply::setReadBufferSize(qint64 size)
{
    Q_D(QNetworkReply);
    d->readBufferMaxSize = size;
}

/*!
    Returns the QNetworkAccessManager that was used to create this
    QNetworkReply object. Initially, it is also the parent object.
*/
QNetworkAccessManager *QNetworkReply::manager() const
{
    return d_func()->manager;
}

/*!
    Returns the request that was posted for this reply. In special,
    note that the URL for the request may be different than that of
    the reply.

    \sa QNetworkRequest::url(), url(), setRequest()
*/
QNetworkRequest QNetworkReply::request() const
{
    return d_func()->request;
}

/*!
    Returns the operation that was posted for this reply.

    \sa setOperation()
*/
QNetworkAccessManager::Operation QNetworkReply::operation() const
{
    return d_func()->operation;
}

/*!
    Returns the error that was found during the processing of this
    request. If no error was found, returns NoError.

    \sa setError()
*/
QNetworkReply::NetworkError QNetworkReply::error() const
{
    return d_func()->errorCode;
}

/*!
    \since 4.6

    Returns true when the reply has finished or was aborted.

    \sa isRunning()
*/
bool QNetworkReply::isFinished() const
{
    return d_func()->isFinished;
}

/*!
    \since 4.6

    Returns true when the request is still processing and the
    reply has not finished or was aborted yet.

    \sa isFinished()
*/
bool QNetworkReply::isRunning() const
{
    return !isFinished();
}

/*!
    Returns the URL of the content downloaded or uploaded. Note that
    the URL may be different from that of the original request.

    \sa request(), setUrl(), QNetworkRequest::url()
*/
QUrl QNetworkReply::url() const
{
    return d_func()->url;
}

/*!
    Returns the value of the known header \a header, if that header
    was sent by the remote server. If the header was not sent, returns
    an invalid QVariant.

    \sa rawHeader(), setHeader(), QNetworkRequest::header()
*/
QVariant QNetworkReply::header(QNetworkRequest::KnownHeaders header) const
{
    return d_func()->cookedHeaders.value(header);
}

/*!
    Returns true if the raw header of name \a headerName was sent by
    the remote server

    \sa rawHeader()
*/
bool QNetworkReply::hasRawHeader(const QByteArray &headerName) const
{
    Q_D(const QNetworkReply);
    return d->findRawHeader(headerName) != d->rawHeaders.constEnd();
}

/*!
    Returns the raw contents of the header \a headerName as sent by
    the remote server. If there is no such header, returns an empty
    byte array, which may be indistinguishable from an empty
    header. Use hasRawHeader() to verify if the server sent such
    header field.

    \sa setRawHeader(), hasRawHeader(), header()
*/
QByteArray QNetworkReply::rawHeader(const QByteArray &headerName) const
{
    Q_D(const QNetworkReply);
    QNetworkHeadersPrivate::RawHeadersList::ConstIterator it =
        d->findRawHeader(headerName);
    if (it != d->rawHeaders.constEnd())
        return it->second;
    return QByteArray();
}

/*! \typedef QNetworkReply::RawHeaderPair

  RawHeaderPair is a QPair<QByteArray, QByteArray> where the first
  QByteArray is the header name and the second is the header.
 */

/*!
  Returns a list of raw header pairs.
 */
const QList<QNetworkReply::RawHeaderPair>& QNetworkReply::rawHeaderPairs() const
{
    Q_D(const QNetworkReply);
    return d->rawHeaders;
}

/*!
    Returns a list of headers fields that were sent by the remote
    server, in the order that they were sent. Duplicate headers are
    merged together and take place of the latter duplicate.
*/
QList<QByteArray> QNetworkReply::rawHeaderList() const
{
    return d_func()->rawHeadersKeys();
}

/*!
    Returns the attribute associated with the code \a code. If the
    attribute has not been set, it returns an invalid QVariant (type QVariant::Null).

    You can expect the default values listed in
    QNetworkRequest::Attribute to be applied to the values returned by
    this function.

    \sa setAttribute(), QNetworkRequest::Attribute
*/
QVariant QNetworkReply::attribute(QNetworkRequest::Attribute code) const
{
    return d_func()->attributes.value(code);
}

#ifndef QT_NO_OPENSSL
/*!
    Returns the SSL configuration and state associated with this
    reply, if SSL was used. It will contain the remote server's
    certificate, its certificate chain leading to the Certificate
    Authority as well as the encryption ciphers in use.

    The peer's certificate and its certificate chain will be known by
    the time sslErrors() is emitted, if it's emitted.
*/
QSslConfiguration QNetworkReply::sslConfiguration() const
{
    QSslConfiguration config;

    // determine if we support this extension
    int id = metaObject()->indexOfMethod("sslConfigurationImplementation()");
    if (id != -1) {
        void *arr[] = { &config, 0 };
        const_cast<QNetworkReply *>(this)->qt_metacall(QMetaObject::InvokeMetaMethod, id, arr);
    }
    return config;
}

/*!
    Sets the SSL configuration for the network connection associated
    with this request, if possible, to be that of \a config.
*/
void QNetworkReply::setSslConfiguration(const QSslConfiguration &config)
{
    if (config.isNull())
        return;

    int id = metaObject()->indexOfMethod("setSslConfigurationImplementation(QSslConfiguration)");
    if (id != -1) {
        QSslConfiguration copy(config);
        void *arr[] = { 0, &copy };
        qt_metacall(QMetaObject::InvokeMetaMethod, id, arr);
    }
}

/*!
    \overload
    \since 4.6

    If this function is called, the SSL errors given in \a errors
    will be ignored.

    Note that you can set the expected certificate in the SSL error:
    If, for instance, you want to issue a request to a server that uses
    a self-signed certificate, consider the following snippet:

    \snippet doc/src/snippets/code/src_network_access_qnetworkreply.cpp 0

    Multiple calls to this function will replace the list of errors that
    were passed in previous calls.
    You can clear the list of errors you want to ignore by calling this
    function with an empty list.

    \sa sslConfiguration(), sslErrors(), QSslSocket::ignoreSslErrors()
*/
void QNetworkReply::ignoreSslErrors(const QList<QSslError> &errors)
{
    // do this cryptic trick, because we could not add a virtual method to this class later on
    // since that breaks binary compatibility
    int id = metaObject()->indexOfMethod("ignoreSslErrorsImplementation(QList<QSslError>)");
    if (id != -1) {
        QList<QSslError> copy(errors);
        void *arr[] = { 0, &copy };
        qt_metacall(QMetaObject::InvokeMetaMethod, id, arr);
    }
}
#endif

/*!
    If this function is called, SSL errors related to network
    connection will be ignored, including certificate validation
    errors.

    Note that calling this function without restraint may pose a
    security risk for your application. Use it with care.

    This function can be called from the slot connected to the
    sslErrors() signal, which indicates which errors were
    found.

    \sa sslConfiguration(), sslErrors(), QSslSocket::ignoreSslErrors()
*/
void QNetworkReply::ignoreSslErrors()
{
}

/*!
    \internal
*/
qint64 QNetworkReply::writeData(const char *, qint64)
{
    return -1;                  // you can't write
}

/*!
    Sets the associated operation for this object to be \a
    operation. This value will be returned by operation().

    Note: the operation should be set when this object is created and
    not changed again.

    \sa operation(), setRequest()
*/
void QNetworkReply::setOperation(QNetworkAccessManager::Operation operation)
{
    Q_D(QNetworkReply);
    d->operation = operation;
}

/*!
    Sets the associated request for this object to be \a request. This
    value will be returned by request().

    Note: the request should be set when this object is created and
    not changed again.

    \sa request(), setOperation()
*/
void QNetworkReply::setRequest(const QNetworkRequest &request)
{
    Q_D(QNetworkReply);
    d->request = request;
}

/*!
    Sets the error condition to be \a errorCode. The human-readable
    message is set with \a errorString.

    Calling setError() does not emit the error(QNetworkReply::NetworkError)
    signal.

    \sa error(), errorString()
*/
void QNetworkReply::setError(NetworkError errorCode, const QString &errorString)
{
    Q_D(QNetworkReply);
    d->errorCode = errorCode;
    setErrorString(errorString); // in QIODevice
}

/*!
    \since 4.8
    Sets the reply as \a finished.

    After having this set the replies data must not change.

    \sa isFinished()
*/
void QNetworkReply::setFinished(bool finished)
{
    Q_D(QNetworkReply);
    d->isFinished = finished;
}


/*!
    Sets the URL being processed to be \a url. Normally, the URL
    matches that of the request that was posted, but for a variety of
    reasons it can be different (for example, a file path being made
    absolute or canonical).

    \sa url(), request(), QNetworkRequest::url()
*/
void QNetworkReply::setUrl(const QUrl &url)
{
    Q_D(QNetworkReply);
    d->url = url;
}

/*!
    Sets the known header \a header to be of value \a value. The
    corresponding raw form of the header will be set as well.

    \sa header(), setRawHeader(), QNetworkRequest::setHeader()
*/
void QNetworkReply::setHeader(QNetworkRequest::KnownHeaders header, const QVariant &value)
{
    Q_D(QNetworkReply);
    d->setCookedHeader(header, value);
}

/*!
    Sets the raw header \a headerName to be of value \a value. If \a
    headerName was previously set, it is overridden. Multiple HTTP
    headers of the same name are functionally equivalent to one single
    header with the values concatenated, separated by commas.

    If \a headerName matches a known header, the value \a value will
    be parsed and the corresponding parsed form will also be set.

    \sa rawHeader(), header(), setHeader(), QNetworkRequest::setRawHeader()
*/
void QNetworkReply::setRawHeader(const QByteArray &headerName, const QByteArray &value)
{
    Q_D(QNetworkReply);
    d->setRawHeader(headerName, value);
}

/*!
    Sets the attribute \a code to have value \a value. If \a code was
    previously set, it will be overridden. If \a value is an invalid
    QVariant, the attribute will be unset.

    \sa attribute(), QNetworkRequest::setAttribute()
*/
void QNetworkReply::setAttribute(QNetworkRequest::Attribute code, const QVariant &value)
{
    Q_D(QNetworkReply);
    if (value.isValid())
        d->attributes.insert(code, value);
    else
        d->attributes.remove(code);
}

QT_END_NAMESPACE
