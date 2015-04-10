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

#include "qhttpnetworkreply_p.h"
#include "qhttpnetworkconnection_p.h"

#include <qbytearraymatcher.h>

#ifndef QT_NO_HTTP

#ifndef QT_NO_SSL
#    include <QtNetwork/qsslkey.h>
#    include <QtNetwork/qsslcipher.h>
#    include <QtNetwork/qsslconfiguration.h>
#endif

#ifndef QT_NO_COMPRESS
#include <zlib.h>
#endif

QT_BEGIN_NAMESPACE

QHttpNetworkReply::QHttpNetworkReply(const QUrl &url, QObject *parent)
    : QObject(*new QHttpNetworkReplyPrivate(url), parent)
{
}

QHttpNetworkReply::~QHttpNetworkReply()
{
    Q_D(QHttpNetworkReply);
    if (d->connection) {
        d->connection->d_func()->removeReply(this);
    }

#ifndef QT_NO_COMPRESS
    if (d->autoDecompress && d->isCompressed() && d->inflateStrm)
        inflateEnd(d->inflateStrm);
#endif
}

QUrl QHttpNetworkReply::url() const
{
    return d_func()->url;
}
void QHttpNetworkReply::setUrl(const QUrl &url)
{
    Q_D(QHttpNetworkReply);
    d->url = url;
}

qint64 QHttpNetworkReply::contentLength() const
{
    return d_func()->contentLength();
}

void QHttpNetworkReply::setContentLength(qint64 length)
{
    Q_D(QHttpNetworkReply);
    d->setContentLength(length);
}

QList<QPair<QByteArray, QByteArray> > QHttpNetworkReply::header() const
{
    return d_func()->fields;
}

QByteArray QHttpNetworkReply::headerField(const QByteArray &name, const QByteArray &defaultValue) const
{
    return d_func()->headerField(name, defaultValue);
}

void QHttpNetworkReply::setHeaderField(const QByteArray &name, const QByteArray &data)
{
    Q_D(QHttpNetworkReply);
    d->setHeaderField(name, data);
}

void QHttpNetworkReply::parseHeader(const QByteArray &header)
{
    Q_D(QHttpNetworkReply);
    d->parseHeader(header);
}

QHttpNetworkRequest QHttpNetworkReply::request() const
{
    return d_func()->request;
}

void QHttpNetworkReply::setRequest(const QHttpNetworkRequest &request)
{
    Q_D(QHttpNetworkReply);
    d->request = request;
    d->ssl = request.isSsl();
}

int QHttpNetworkReply::statusCode() const
{
    return d_func()->statusCode;
}

void QHttpNetworkReply::setStatusCode(int code)
{
    Q_D(QHttpNetworkReply);
    d->statusCode = code;
}

QString QHttpNetworkReply::errorString() const
{
    return d_func()->errorString;
}

QString QHttpNetworkReply::reasonPhrase() const
{
    return d_func()->reasonPhrase;
}

void QHttpNetworkReply::setErrorString(const QString &error)
{
    Q_D(QHttpNetworkReply);
    d->errorString = error;
}

int QHttpNetworkReply::majorVersion() const
{
    return d_func()->majorVersion;
}

int QHttpNetworkReply::minorVersion() const
{
    return d_func()->minorVersion;
}

qint64 QHttpNetworkReply::bytesAvailable() const
{
    Q_D(const QHttpNetworkReply);
    if (d->connection)
        return d->connection->d_func()->uncompressedBytesAvailable(*this);
    else
        return -1;
}

qint64 QHttpNetworkReply::bytesAvailableNextBlock() const
{
    Q_D(const QHttpNetworkReply);
    if (d->connection)
        return d->connection->d_func()->uncompressedBytesAvailableNextBlock(*this);
    else
        return -1;
}

bool QHttpNetworkReply::readAnyAvailable() const
{
    Q_D(const QHttpNetworkReply);
    return (d->responseData.bufferCount() > 0);
}

QByteArray QHttpNetworkReply::readAny()
{
    Q_D(QHttpNetworkReply);
    if (d->responseData.bufferCount() == 0)
        return QByteArray();

    // we'll take the last buffer, so schedule another read from http
    if (d->downstreamLimited && d->responseData.bufferCount() == 1)
        d->connection->d_func()->readMoreLater(this);
    return d->responseData.read();
}

QByteArray QHttpNetworkReply::readAll()
{
    Q_D(QHttpNetworkReply);
    return d->responseData.readAll();
}

QByteArray QHttpNetworkReply::read(qint64 amount)
{
    Q_D(QHttpNetworkReply);
    return d->responseData.read(amount);
}


qint64 QHttpNetworkReply::sizeNextBlock()
{
    Q_D(QHttpNetworkReply);
    return d->responseData.sizeNextBlock();
}

void QHttpNetworkReply::setDownstreamLimited(bool dsl)
{
    Q_D(QHttpNetworkReply);
    d->downstreamLimited = dsl;
    d->connection->d_func()->readMoreLater(this);
}

void QHttpNetworkReply::setReadBufferSize(qint64 size)
{
    Q_D(QHttpNetworkReply);
    d->readBufferMaxSize = size;
}

bool QHttpNetworkReply::supportsUserProvidedDownloadBuffer()
{
    Q_D(QHttpNetworkReply);
    return (!d->isChunked() && !d->autoDecompress && d->bodyLength > 0 && d->statusCode == 200);
}

void QHttpNetworkReply::setUserProvidedDownloadBuffer(char* b)
{
    Q_D(QHttpNetworkReply);
    if (supportsUserProvidedDownloadBuffer())
        d->userProvidedDownloadBuffer = b;
}

char* QHttpNetworkReply::userProvidedDownloadBuffer()
{
    Q_D(QHttpNetworkReply);
    return d->userProvidedDownloadBuffer;
}

bool QHttpNetworkReply::isFinished() const
{
    return d_func()->state == QHttpNetworkReplyPrivate::AllDoneState;
}

bool QHttpNetworkReply::isPipeliningUsed() const
{
    return d_func()->pipeliningUsed;
}

bool QHttpNetworkReply::isSpdyUsed() const
{
    return d_func()->spdyUsed;
}

void QHttpNetworkReply::setSpdyWasUsed(bool spdy)
{
    d_func()->spdyUsed = spdy;
}

QHttpNetworkConnection* QHttpNetworkReply::connection()
{
    return d_func()->connection;
}


QHttpNetworkReplyPrivate::QHttpNetworkReplyPrivate(const QUrl &newUrl)
    : QHttpNetworkHeaderPrivate(newUrl)
    , state(NothingDoneState)
    , ssl(false)
    , statusCode(100),
      majorVersion(0), minorVersion(0), bodyLength(0), contentRead(0), totalProgress(0),
      chunkedTransferEncoding(false),
      connectionCloseEnabled(true),
      forceConnectionCloseEnabled(false),
      lastChunkRead(false),
      currentChunkSize(0), currentChunkRead(0), readBufferMaxSize(0),
      windowSizeDownload(65536), // 64K initial window size according to SPDY standard
      windowSizeUpload(65536), // 64K initial window size according to SPDY standard
      currentlyReceivedDataInWindow(0),
      currentlyUploadedDataInWindow(0),
      totallyUploadedData(0),
      connection(0),
      autoDecompress(false), responseData(), requestIsPrepared(false)
      ,pipeliningUsed(false), spdyUsed(false), downstreamLimited(false)
      ,userProvidedDownloadBuffer(0)
#ifndef QT_NO_COMPRESS
      ,inflateStrm(0)
#endif

{
    QString scheme = newUrl.scheme();
    if (scheme == QLatin1String("preconnect-http")
            || scheme == QLatin1String("preconnect-https"))
        // make sure we do not close the socket after preconnecting
        connectionCloseEnabled = false;
}

QHttpNetworkReplyPrivate::~QHttpNetworkReplyPrivate()
{
#ifndef QT_NO_COMPRESS
      if (inflateStrm)
          delete inflateStrm;
#endif
}

void QHttpNetworkReplyPrivate::clearHttpLayerInformation()
{
    state = NothingDoneState;
    statusCode = 100;
    bodyLength = 0;
    contentRead = 0;
    totalProgress = 0;
    currentChunkSize = 0;
    currentChunkRead = 0;
    lastChunkRead = false;
    connectionCloseEnabled = true;
#ifndef QT_NO_COMPRESS
    if (autoDecompress && inflateStrm)
        inflateEnd(inflateStrm);
#endif
    fields.clear();
}

// TODO: Isn't everything HTTP layer related? We don't need to set connection and connectionChannel to 0 at all
void QHttpNetworkReplyPrivate::clear()
{
    connection = 0;
    connectionChannel = 0;
    autoDecompress = false;
    clearHttpLayerInformation();
}

// QHttpNetworkReplyPrivate
qint64 QHttpNetworkReplyPrivate::bytesAvailable() const
{
    return (state != ReadingDataState ? 0 : fragment.size());
}

bool QHttpNetworkReplyPrivate::isCompressed()
{
    QByteArray encoding = headerField("content-encoding");
    return qstricmp(encoding.constData(), "gzip") == 0 || qstricmp(encoding.constData(), "deflate") == 0;
}

void QHttpNetworkReplyPrivate::removeAutoDecompressHeader()
{
    // The header "Content-Encoding  = gzip" is retained.
    // Content-Length is removed since the actual one sent by the server is for compressed data
    QByteArray name("content-length");
    QList<QPair<QByteArray, QByteArray> >::Iterator it = fields.begin(),
                                                   end = fields.end();
    while (it != end) {
        if (qstricmp(name.constData(), it->first.constData()) == 0) {
            fields.erase(it);
            break;
        }
        ++it;
    }

}

bool QHttpNetworkReplyPrivate::findChallenge(bool forProxy, QByteArray &challenge) const
{
    challenge.clear();
    // find out the type of authentication protocol requested.
    QByteArray header = forProxy ? "proxy-authenticate" : "www-authenticate";
    // pick the best protocol (has to match parsing in QAuthenticatorPrivate)
    QList<QByteArray> challenges = headerFieldValues(header);
    for (int i = 0; i<challenges.size(); i++) {
        QByteArray line = challenges.at(i);
        // todo use qstrincmp
        if (!line.toLower().startsWith("negotiate"))
            challenge = line;
    }
    return !challenge.isEmpty();
}

QAuthenticatorPrivate::Method QHttpNetworkReplyPrivate::authenticationMethod(bool isProxy) const
{
    // The logic is same as the one used in void QAuthenticatorPrivate::parseHttpResponse()
    QAuthenticatorPrivate::Method method = QAuthenticatorPrivate::None;
    QByteArray header = isProxy ? "proxy-authenticate" : "www-authenticate";
    QList<QByteArray> challenges = headerFieldValues(header);
    for (int i = 0; i<challenges.size(); i++) {
        QByteArray line = challenges.at(i).trimmed().toLower();
        if (method < QAuthenticatorPrivate::Basic
            && line.startsWith("basic")) {
            method = QAuthenticatorPrivate::Basic;
        } else if (method < QAuthenticatorPrivate::Ntlm
            && line.startsWith("ntlm")) {
            method = QAuthenticatorPrivate::Ntlm;
        } else if (method < QAuthenticatorPrivate::DigestMd5
            && line.startsWith("digest")) {
            method = QAuthenticatorPrivate::DigestMd5;
        }
    }
    return method;
}

qint64 QHttpNetworkReplyPrivate::readStatus(QAbstractSocket *socket)
{
    if (fragment.isEmpty()) {
        // reserve bytes for the status line. This is better than always append() which reallocs the byte array
        fragment.reserve(32);
    }

    qint64 bytes = 0;
    char c;
    qint64 haveRead = 0;

    do {
        haveRead = socket->read(&c, 1);
        if (haveRead == -1)
            return -1; // unexpected EOF
        else if (haveRead == 0)
            break; // read more later
        else if (haveRead == 1 && fragment.size() == 0 && (c == 11 || c == '\n' || c == '\r' || c == ' ' || c == 31))
            continue; // Ignore all whitespace that was trailing froma previous request on that socket

        bytes++;

        // allow both CRLF & LF (only) line endings
        if (c == '\n') {
            // remove the CR at the end
            if (fragment.endsWith('\r')) {
                fragment.truncate(fragment.length()-1);
            }
            bool ok = parseStatus(fragment);
            state = ReadingHeaderState;
            fragment.clear();
            if (!ok) {
                return -1;
            }
            break;
        } else {
            fragment.append(c);
        }

        // is this a valid reply?
        if (fragment.length() >= 5 && !fragment.startsWith("HTTP/"))
        {
            fragment.clear();
            return -1;
        }
    } while (haveRead == 1);

    return bytes;
}

bool QHttpNetworkReplyPrivate::parseStatus(const QByteArray &status)
{
    // from RFC 2616:
    //        Status-Line = HTTP-Version SP Status-Code SP Reason-Phrase CRLF
    //        HTTP-Version   = "HTTP" "/" 1*DIGIT "." 1*DIGIT
    // that makes: 'HTTP/n.n xxx Message'
    // byte count:  0123456789012

    static const int minLength = 11;
    static const int dotPos = 6;
    static const int spacePos = 8;
    static const char httpMagic[] = "HTTP/";

    if (status.length() < minLength
        || !status.startsWith(httpMagic)
        || status.at(dotPos) != '.'
        || status.at(spacePos) != ' ') {
        // I don't know how to parse this status line
        return false;
    }

    // optimize for the valid case: defer checking until the end
    majorVersion = status.at(dotPos - 1) - '0';
    minorVersion = status.at(dotPos + 1) - '0';

    int i = spacePos;
    int j = status.indexOf(' ', i + 1); // j == -1 || at(j) == ' ' so j+1 == 0 && j+1 <= length()
    const QByteArray code = status.mid(i + 1, j - i - 1);

    bool ok;
    statusCode = code.toInt(&ok);
    reasonPhrase = QString::fromLatin1(status.constData() + j + 1);

    return ok && uint(majorVersion) <= 9 && uint(minorVersion) <= 9;
}

qint64 QHttpNetworkReplyPrivate::readHeader(QAbstractSocket *socket)
{
    if (fragment.isEmpty()) {
        // according to http://dev.opera.com/articles/view/mama-http-headers/ the average size of the header
        // block is 381 bytes.
        // reserve bytes. This is better than always append() which reallocs the byte array.
        fragment.reserve(512);
    }

    qint64 bytes = 0;
    char c = 0;
    bool allHeaders = false;
    qint64 haveRead = 0;
    do {
        haveRead = socket->read(&c, 1);
        if (haveRead == 0) {
            // read more later
            break;
        } else if (haveRead == -1) {
            // connection broke down
            return -1;
        } else {
            fragment.append(c);
            bytes++;

            if (c == '\n') {
                // check for possible header endings. As per HTTP rfc,
                // the header endings will be marked by CRLFCRLF. But
                // we will allow CRLFCRLF, CRLFLF, LFLF
                if (fragment.endsWith("\r\n\r\n")
                    || fragment.endsWith("\r\n\n")
                    || fragment.endsWith("\n\n"))
                    allHeaders = true;

                // there is another case: We have no headers. Then the fragment equals just the line ending
                if ((fragment.length() == 2 && fragment.endsWith("\r\n"))
                    || (fragment.length() == 1 && fragment.endsWith("\n")))
                    allHeaders = true;
            }
        }
    } while (!allHeaders && haveRead > 0);

    // we received all headers now parse them
    if (allHeaders) {
        parseHeader(fragment);
        state = ReadingDataState;
        fragment.clear(); // next fragment
        bodyLength = contentLength(); // cache the length

        // cache isChunked() since it is called often
        chunkedTransferEncoding = headerField("transfer-encoding").toLower().contains("chunked");

        // cache isConnectionCloseEnabled since it is called often
        QByteArray connectionHeaderField = headerField("connection");
        // check for explicit indication of close or the implicit connection close of HTTP/1.0
        connectionCloseEnabled = (connectionHeaderField.toLower().contains("close") ||
            headerField("proxy-connection").toLower().contains("close")) ||
            (majorVersion == 1 && minorVersion == 0 &&
            (connectionHeaderField.isEmpty() && !headerField("proxy-connection").toLower().contains("keep-alive")));

#ifndef QT_NO_COMPRESS
        if (autoDecompress && isCompressed()) {
            // allocate inflate state
            if (!inflateStrm)
                inflateStrm = new z_stream;
            int ret = initializeInflateStream();
            if (ret != Z_OK)
                return -1;
        }
#endif

    }
    return bytes;
}

void QHttpNetworkReplyPrivate::parseHeader(const QByteArray &header)
{
    // see rfc2616, sec 4 for information about HTTP/1.1 headers.
    // allows relaxed parsing here, accepts both CRLF & LF line endings
    const QByteArrayMatcher lf("\n");
    const QByteArrayMatcher colon(":");
    int i = 0;
    while (i < header.count()) {
        int j = colon.indexIn(header, i); // field-name
        if (j == -1)
            break;
        const QByteArray field = header.mid(i, j - i).trimmed();
        j++;
        // any number of LWS is allowed before and after the value
        QByteArray value;
        do {
            i = lf.indexIn(header, j);
            if (i == -1)
                break;
            if (!value.isEmpty())
                value += ' ';
            // check if we have CRLF or only LF
            bool hasCR = (i && header[i-1] == '\r');
            int length = i -(hasCR ? 1: 0) - j;
            value += header.mid(j, length).trimmed();
            j = ++i;
        } while (i < header.count() && (header.at(i) == ' ' || header.at(i) == '\t'));
        if (i == -1)
            break; // something is wrong

        fields.append(qMakePair(field, value));
    }
}

bool QHttpNetworkReplyPrivate::isChunked()
{
    return chunkedTransferEncoding;
}

bool QHttpNetworkReplyPrivate::isConnectionCloseEnabled()
{
    return connectionCloseEnabled || forceConnectionCloseEnabled;
}

// note this function can only be used for non-chunked, non-compressed with
// known content length
qint64 QHttpNetworkReplyPrivate::readBodyVeryFast(QAbstractSocket *socket, char *b)
{
    // This first read is to flush the buffer inside the socket
    qint64 haveRead = 0;
    haveRead = socket->read(b, bodyLength - contentRead);
    if (haveRead == -1) {
        return -1;
    }
    contentRead += haveRead;

    if (contentRead == bodyLength) {
        state = AllDoneState;
    }

    return haveRead;
}

// note this function can only be used for non-chunked, non-compressed with
// known content length
qint64 QHttpNetworkReplyPrivate::readBodyFast(QAbstractSocket *socket, QByteDataBuffer *rb)
{

    qint64 toBeRead = qMin(socket->bytesAvailable(), bodyLength - contentRead);
    if (readBufferMaxSize)
        toBeRead = qMin(toBeRead, readBufferMaxSize);

    if (!toBeRead)
        return 0;

    QByteArray bd;
    bd.resize(toBeRead);
    qint64 haveRead = socket->read(bd.data(), toBeRead);
    if (haveRead == -1) {
        bd.clear();
        return 0; // ### error checking here;
    }
    bd.resize(haveRead);

    rb->append(bd);

    if (contentRead + haveRead == bodyLength) {
        state = AllDoneState;
    }

    contentRead += haveRead;
    return haveRead;
}


qint64 QHttpNetworkReplyPrivate::readBody(QAbstractSocket *socket, QByteDataBuffer *out)
{
    qint64 bytes = 0;

#ifndef QT_NO_COMPRESS
    // for gzip we'll allocate a temporary one that we then decompress
    QByteDataBuffer *tempOutDataBuffer = (autoDecompress ? new QByteDataBuffer : out);
#else
    QByteDataBuffer *tempOutDataBuffer = out;
#endif


    if (isChunked()) {
        // chunked transfer encoding (rfc 2616, sec 3.6)
        bytes += readReplyBodyChunked(socket, tempOutDataBuffer);
    } else if (bodyLength > 0) {
        // we have a Content-Length
        bytes += readReplyBodyRaw(socket, tempOutDataBuffer, bodyLength - contentRead);
        if (contentRead + bytes == bodyLength)
            state = AllDoneState;
    } else {
        // no content length. just read what's possible
        bytes += readReplyBodyRaw(socket, tempOutDataBuffer, socket->bytesAvailable());
    }

#ifndef QT_NO_COMPRESS
    // This is true if there is compressed encoding and we're supposed to use it.
    if (autoDecompress) {
        qint64 uncompressRet = uncompressBodyData(tempOutDataBuffer, out);
        delete tempOutDataBuffer;
        if (uncompressRet < 0)
            return -1;
    }
#endif

    contentRead += bytes;
    return bytes;
}

#ifndef QT_NO_COMPRESS
int QHttpNetworkReplyPrivate::initializeInflateStream()
{
    inflateStrm->zalloc = Z_NULL;
    inflateStrm->zfree = Z_NULL;
    inflateStrm->opaque = Z_NULL;
    inflateStrm->avail_in = 0;
    inflateStrm->next_in = Z_NULL;
    // "windowBits can also be greater than 15 for optional gzip decoding.
    // Add 32 to windowBits to enable zlib and gzip decoding with automatic header detection"
    // http://www.zlib.net/manual.html
    int ret = inflateInit2(inflateStrm, MAX_WBITS+32);
    Q_ASSERT(ret == Z_OK);
    return ret;
}

qint64 QHttpNetworkReplyPrivate::uncompressBodyData(QByteDataBuffer *in, QByteDataBuffer *out)
{
    if (!inflateStrm) { // happens when called from the SPDY protocol handler
        inflateStrm = new z_stream;
        initializeInflateStream();
    }

    if (!inflateStrm)
        return -1;

    bool triedRawDeflate = false;
    for (int i = 0; i < in->bufferCount(); i++) {
        QByteArray &bIn = (*in)[i];

        inflateStrm->avail_in = bIn.size();
        inflateStrm->next_in = reinterpret_cast<Bytef*>(bIn.data());

        do {
            QByteArray bOut;
            // make a wild guess about the uncompressed size.
            bOut.reserve(inflateStrm->avail_in * 3 + 512);
            inflateStrm->avail_out = bOut.capacity();
            inflateStrm->next_out = reinterpret_cast<Bytef*>(bOut.data());

            int ret = inflate(inflateStrm, Z_NO_FLUSH);
            //All negative return codes are errors, in the context of HTTP compression, Z_NEED_DICT is also an error.
            // in the case where we get Z_DATA_ERROR this could be because we received raw deflate compressed data.
            if (ret == Z_DATA_ERROR && !triedRawDeflate) {
                inflateEnd(inflateStrm);
                triedRawDeflate = true;
                inflateStrm->zalloc = Z_NULL;
                inflateStrm->zfree = Z_NULL;
                inflateStrm->opaque = Z_NULL;
                inflateStrm->avail_in = 0;
                inflateStrm->next_in = Z_NULL;
                int ret = inflateInit2(inflateStrm, -MAX_WBITS);
                if (ret != Z_OK) {
                    return -1;
                } else {
                    inflateStrm->avail_in = bIn.size();
                    inflateStrm->next_in = reinterpret_cast<Bytef*>(bIn.data());
                    continue;
                }
            } else if (ret < 0 || ret == Z_NEED_DICT) {
                return -1;
            }
            bOut.resize(bOut.capacity() - inflateStrm->avail_out);
            out->append(bOut);
            if (ret == Z_STREAM_END)
                return out->byteAmount();
        } while (inflateStrm->avail_in > 0);
    }

    return out->byteAmount();
}
#endif

qint64 QHttpNetworkReplyPrivate::readReplyBodyRaw(QAbstractSocket *socket, QByteDataBuffer *out, qint64 size)
{
    // FIXME get rid of this function and just use readBodyFast and give it socket->bytesAvailable()
    qint64 bytes = 0;
    Q_ASSERT(socket);
    Q_ASSERT(out);

    int toBeRead = qMin<qint64>(128*1024, qMin<qint64>(size, socket->bytesAvailable()));

    if (readBufferMaxSize)
        toBeRead = qMin<qint64>(toBeRead, readBufferMaxSize);

    while (toBeRead > 0) {
        QByteArray byteData;
        byteData.resize(toBeRead);
        qint64 haveRead = socket->read(byteData.data(), byteData.size());
        if (haveRead <= 0) {
            // ### error checking here
            byteData.clear();
            return bytes;
        }

        byteData.resize(haveRead);
        out->append(byteData);
        bytes += haveRead;
        size -= haveRead;

        toBeRead = qMin<qint64>(128*1024, qMin<qint64>(size, socket->bytesAvailable()));
    }
    return bytes;

}

qint64 QHttpNetworkReplyPrivate::readReplyBodyChunked(QAbstractSocket *socket, QByteDataBuffer *out)
{
    qint64 bytes = 0;
    while (socket->bytesAvailable()) {

        if (readBufferMaxSize && (bytes > readBufferMaxSize))
            break;

        if (!lastChunkRead && currentChunkRead >= currentChunkSize) {
            // For the first chunk and when we're done with a chunk
            currentChunkSize = 0;
            currentChunkRead = 0;
            if (bytes) {
                // After a chunk
                char crlf[2];
                // read the "\r\n" after the chunk
                qint64 haveRead = socket->read(crlf, 2);
                // FIXME: This code is slightly broken and not optimal. What if the 2 bytes are not available yet?!
                // For nice reasons (the toLong in getChunkSize accepting \n at the beginning
                // it right now still works, but we should definitely fix this.

                if (haveRead != 2)
                    return bytes; // FIXME
                bytes += haveRead;
            }
            // Note that chunk size gets stored in currentChunkSize, what is returned is the bytes read
            bytes += getChunkSize(socket, &currentChunkSize);
            if (currentChunkSize == -1)
                break;
        }
        // if the chunk size is 0, end of the stream
        if (currentChunkSize == 0 || lastChunkRead) {
            lastChunkRead = true;
            // try to read the "\r\n" after the chunk
            char crlf[2];
            qint64 haveRead = socket->read(crlf, 2);
            if (haveRead > 0)
                bytes += haveRead;

            if ((haveRead == 2 && crlf[0] == '\r' && crlf[1] == '\n') || (haveRead == 1 && crlf[0] == '\n'))
                state = AllDoneState;
            else if (haveRead == 1 && crlf[0] == '\r')
                break; // Still waiting for the last \n
            else if (haveRead > 0) {
                // If we read something else then CRLF, we need to close the channel.
                forceConnectionCloseEnabled = true;
                state = AllDoneState;
            }
            break;
        }

        // otherwise, try to begin reading this chunk / to read what is missing for this chunk
        qint64 haveRead = readReplyBodyRaw (socket, out, currentChunkSize - currentChunkRead);
        currentChunkRead += haveRead;
        bytes += haveRead;

        // ### error checking here

    }
    return bytes;
}

qint64 QHttpNetworkReplyPrivate::getChunkSize(QAbstractSocket *socket, qint64 *chunkSize)
{
    qint64 bytes = 0;
    char crlf[2];
    *chunkSize = -1;

    int bytesAvailable = socket->bytesAvailable();
    // FIXME rewrite to permanent loop without bytesAvailable
    while (bytesAvailable > bytes) {
        qint64 sniffedBytes = socket->peek(crlf, 2);
        int fragmentSize = fragment.size();

        // check the next two bytes for a "\r\n", skip blank lines
        if ((fragmentSize && sniffedBytes == 2 && crlf[0] == '\r' && crlf[1] == '\n')
           ||(fragmentSize > 1 && fragment.endsWith('\r')  && crlf[0] == '\n'))
        {
            bytes += socket->read(crlf, 1);     // read the \r or \n
            if (crlf[0] == '\r')
                bytes += socket->read(crlf, 1); // read the \n
            bool ok = false;
            // ignore the chunk-extension
            fragment = fragment.mid(0, fragment.indexOf(';')).trimmed();
            *chunkSize = fragment.toLong(&ok, 16);
            fragment.clear();
            break; // size done
        } else {
            // read the fragment to the buffer
            char c = 0;
            qint64 haveRead = socket->read(&c, 1);
            if (haveRead < 0) {
                return -1; // FIXME
            }
            bytes += haveRead;
            fragment.append(c);
        }
    }

    return bytes;
}

bool QHttpNetworkReplyPrivate::shouldEmitSignals()
{
    // for 401 & 407 don't emit the data signals. Content along with these
    // responses are sent only if the authentication fails.
    return (statusCode != 401 && statusCode != 407);
}

bool QHttpNetworkReplyPrivate::expectContent()
{
    // check whether we can expect content after the headers (rfc 2616, sec4.4)
    if ((statusCode >= 100 && statusCode < 200)
        || statusCode == 204 || statusCode == 304)
        return false;
    if (request.operation() == QHttpNetworkRequest::Head)
        return false; // no body expected for HEAD request
    qint64 expectedContentLength = contentLength();
    if (expectedContentLength == 0)
        return false;
    if (expectedContentLength == -1 && bodyLength == 0) {
        // The content-length header was stripped, but its value was 0.
        // This would be the case for an explicitly zero-length compressed response.
        return false;
    }
    return true;
}

void QHttpNetworkReplyPrivate::eraseData()
{
    compressedData.clear();
    responseData.clear();
}


// SSL support below
#ifndef QT_NO_SSL

QSslConfiguration QHttpNetworkReply::sslConfiguration() const
{
    Q_D(const QHttpNetworkReply);

    if (!d->connectionChannel)
        return QSslConfiguration();

    QSslSocket *sslSocket = qobject_cast<QSslSocket*>(d->connectionChannel->socket);
    if (!sslSocket)
        return QSslConfiguration();

    return sslSocket->sslConfiguration();
}

void QHttpNetworkReply::setSslConfiguration(const QSslConfiguration &config)
{
    Q_D(QHttpNetworkReply);
    if (d->connection)
        d->connection->setSslConfiguration(config);
}

void QHttpNetworkReply::ignoreSslErrors()
{
    Q_D(QHttpNetworkReply);
    if (d->connection)
        d->connection->ignoreSslErrors();
}

void QHttpNetworkReply::ignoreSslErrors(const QList<QSslError> &errors)
{
    Q_D(QHttpNetworkReply);
    if (d->connection)
        d->connection->ignoreSslErrors(errors);
}


#endif //QT_NO_SSL


QT_END_NAMESPACE

#endif // QT_NO_HTTP
