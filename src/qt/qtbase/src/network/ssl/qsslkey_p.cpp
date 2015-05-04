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


/*!
    \class QSslKey
    \brief The QSslKey class provides an interface for private and public keys.
    \since 4.3

    \reentrant
    \ingroup network
    \ingroup ssl
    \ingroup shared
    \inmodule QtNetwork

    QSslKey provides a simple API for managing keys.

    \sa QSslSocket, QSslCertificate, QSslCipher
*/

#include "qsslkey.h"
#include "qsslkey_p.h"
#include "qsslsocket.h"
#include "qsslsocket_p.h"

#include <QtCore/qatomic.h>
#include <QtCore/qbytearray.h>
#include <QtCore/qbytearraymatcher.h>
#include <QtCore/qiodevice.h>
#ifndef QT_NO_DEBUG_STREAM
#include <QtCore/qdebug.h>
#endif

QT_BEGIN_NAMESPACE

/*!
    \fn void QSslKeyPrivate::clear(bool deep)
    \internal
 */

/*!
    \fn void QSslKeyPrivate::decodePem(const QByteArray &pem, const QByteArray &passPhrase,
                               bool deepClear)
    \internal

    Allocates a new rsa or dsa struct and decodes \a pem into it
    according to the current algorithm and type.

    If \a deepClear is true, the rsa/dsa struct is freed if it is was
    already allocated, otherwise we "leak" memory (which is exactly
    what we want for copy construction).

    If \a passPhrase is non-empty, it will be used for decrypting
    \a pem.
*/

/*!
    Constructs a null key.

    \sa isNull()
*/
QSslKey::QSslKey()
    : d(new QSslKeyPrivate)
{
}

/*!
    \internal
*/
QByteArray QSslKeyPrivate::pemHeader() const
{
    if (type == QSsl::PublicKey)
        return QByteArrayLiteral("-----BEGIN PUBLIC KEY-----");
    else if (algorithm == QSsl::Rsa)
        return QByteArrayLiteral("-----BEGIN RSA PRIVATE KEY-----");
    return QByteArrayLiteral("-----BEGIN DSA PRIVATE KEY-----");
}

/*!
    \internal
*/
QByteArray QSslKeyPrivate::pemFooter() const
{
    if (type == QSsl::PublicKey)
        return QByteArrayLiteral("-----END PUBLIC KEY-----");
    else if (algorithm == QSsl::Rsa)
        return QByteArrayLiteral("-----END RSA PRIVATE KEY-----");
    return QByteArrayLiteral("-----END DSA PRIVATE KEY-----");
}

/*!
    \internal

    Returns a DER key formatted as PEM.
*/
QByteArray QSslKeyPrivate::pemFromDer(const QByteArray &der, const QMap<QByteArray, QByteArray> &headers) const
{
    QByteArray pem(der.toBase64());

    const int lineWidth = 64; // RFC 1421
    const int newLines = pem.size() / lineWidth;
    const bool rem = pem.size() % lineWidth;

    // ### optimize
    for (int i = 0; i < newLines; ++i)
        pem.insert((i + 1) * lineWidth + i, '\n');
    if (rem)
        pem.append('\n'); // ###

    QByteArray extra;
    if (!headers.isEmpty()) {
        QMap<QByteArray, QByteArray>::const_iterator it = headers.constEnd();
        do {
            it--;
                extra += it.key() + ": " + it.value() + '\n';
        } while (it != headers.constBegin());
        extra += '\n';
    }
    pem.prepend(pemHeader() + '\n' + extra);
    pem.append(pemFooter() + '\n');

    return pem;
}

/*!
    \internal

    Returns a PEM key formatted as DER.
*/
QByteArray QSslKeyPrivate::derFromPem(const QByteArray &pem, QMap<QByteArray, QByteArray> *headers) const
{
    const QByteArray header = pemHeader();
    const QByteArray footer = pemFooter();

    QByteArray der(pem);

    const int headerIndex = der.indexOf(header);
    const int footerIndex = der.indexOf(footer);
    if (headerIndex == -1 || footerIndex == -1)
        return QByteArray();

    der = der.mid(headerIndex + header.size(), footerIndex - (headerIndex + header.size()));

    if (der.contains("Proc-Type:")) {
        // taken from QHttpNetworkReplyPrivate::parseHeader
        const QByteArrayMatcher lf("\n");
        const QByteArrayMatcher colon(":");
        int i = 0;
        while (i < der.count()) {
            int j = colon.indexIn(der, i); // field-name
            if (j == -1)
                break;
            const QByteArray field = der.mid(i, j - i).trimmed();
            j++;
            // any number of LWS is allowed before and after the value
            QByteArray value;
            do {
                i = lf.indexIn(der, j);
                if (i == -1)
                    break;
                if (!value.isEmpty())
                    value += ' ';
                // check if we have CRLF or only LF
                bool hasCR = (i && der[i-1] == '\r');
                int length = i -(hasCR ? 1: 0) - j;
                value += der.mid(j, length).trimmed();
                j = ++i;
            } while (i < der.count() && (der.at(i) == ' ' || der.at(i) == '\t'));
            if (i == -1)
                break; // something is wrong

            headers->insert(field, value);
        }
        der = der.mid(i);
    }

    return QByteArray::fromBase64(der); // ignores newlines
}

/*!
    Constructs a QSslKey by decoding the string in the byte array
    \a encoded using a specified \a algorithm and \a encoding format.
    \a type specifies whether the key is public or private.

    If the key is encoded as PEM and encrypted, \a passPhrase is used
    to decrypt it.

    After construction, use isNull() to check if \a encoded contained
    a valid key.
*/
QSslKey::QSslKey(const QByteArray &encoded, QSsl::KeyAlgorithm algorithm,
                 QSsl::EncodingFormat encoding, QSsl::KeyType type, const QByteArray &passPhrase)
    : d(new QSslKeyPrivate)
{
    d->type = type;
    d->algorithm = algorithm;
    if (encoding == QSsl::Der)
        d->decodeDer(encoded);
    else
        d->decodePem(encoded, passPhrase);
}

/*!
    Constructs a QSslKey by reading and decoding data from a
    \a device using a specified \a algorithm and \a encoding format.
    \a type specifies whether the key is public or private.

    If the key is encoded as PEM and encrypted, \a passPhrase is used
    to decrypt it.

    After construction, use isNull() to check if \a device provided
    a valid key.
*/
QSslKey::QSslKey(QIODevice *device, QSsl::KeyAlgorithm algorithm, QSsl::EncodingFormat encoding,
                 QSsl::KeyType type, const QByteArray &passPhrase)
    : d(new QSslKeyPrivate)
{
    QByteArray encoded;
    if (device)
        encoded = device->readAll();
    d->type = type;
    d->algorithm = algorithm;
    if (encoding == QSsl::Der)
        d->decodeDer(encoded);
    else
        d->decodePem(encoded, passPhrase);
}

/*!
    \since 5.0
    Constructs a QSslKey from a valid native key \a handle.
    \a type specifies whether the key is public or private.

    QSslKey will take ownership for this key and you must not
    free the key using the native library. The algorithm used
    when creating a key from a handle will always be QSsl::Opaque.
*/
QSslKey::QSslKey(Qt::HANDLE handle, QSsl::KeyType type)
    : d(new QSslKeyPrivate)
{
#ifndef QT_NO_OPENSSL
    d->opaque = reinterpret_cast<EVP_PKEY *>(handle);
#else
    d->opaque = handle;
#endif
    d->algorithm = QSsl::Opaque;
    d->type = type;
    d->isNull = !d->opaque;
}

/*!
    Constructs an identical copy of \a other.
*/
QSslKey::QSslKey(const QSslKey &other) : d(other.d)
{
}

/*!
    Destroys the QSslKey object.
*/
QSslKey::~QSslKey()
{
}

/*!
    Copies the contents of \a other into this key, making the two keys
    identical.

    Returns a reference to this QSslKey.
*/
QSslKey &QSslKey::operator=(const QSslKey &other)
{
    d = other.d;
    return *this;
}

/*!
    \fn void QSslKey::swap(QSslKey &other)
    \since 5.0

    Swaps this ssl key with \a other. This function is very fast and
    never fails.
*/

/*!
    Returns \c true if this is a null key; otherwise false.

    \sa clear()
*/
bool QSslKey::isNull() const
{
    return d->isNull;
}

/*!
    Clears the contents of this key, making it a null key.

    \sa isNull()
*/
void QSslKey::clear()
{
    d = new QSslKeyPrivate;
}

/*!
    Returns the length of the key in bits, or -1 if the key is null.
*/
int QSslKey::length() const
{
    return d->length();
}

/*!
    Returns the type of the key (i.e., PublicKey or PrivateKey).
*/
QSsl::KeyType QSslKey::type() const
{
    return d->type;
}

/*!
    Returns the key algorithm.
*/
QSsl::KeyAlgorithm QSslKey::algorithm() const
{
    return d->algorithm;
}

/*!
  Returns the key in DER encoding.

  The \a passPhrase argument should be omitted as DER cannot be
  encrypted. It will be removed in a future version of Qt.
*/
QByteArray QSslKey::toDer(const QByteArray &passPhrase) const
{
    if (d->isNull || d->algorithm == QSsl::Opaque)
        return QByteArray();

    // Encrypted DER is nonsense, see QTBUG-41038.
    if (d->type == QSsl::PrivateKey && !passPhrase.isEmpty())
        return QByteArray();

#ifndef QT_NO_OPENSSL
    QMap<QByteArray, QByteArray> headers;
    return d->derFromPem(toPem(passPhrase), &headers);
#else
    return d->derData;
#endif
}

/*!
  Returns the key in PEM encoding. The result is encrypted with
  \a passPhrase if the key is a private key and \a passPhrase is
  non-empty.
*/
QByteArray QSslKey::toPem(const QByteArray &passPhrase) const
{
    return d->toPem(passPhrase);
}

/*!
    Returns a pointer to the native key handle, if it is available;
    otherwise a null pointer is returned.

    You can use this handle together with the native API to access
    extended information about the key.

    \warning Use of this function has a high probability of being
    non-portable, and its return value may vary across platforms, and
    between minor Qt releases.
*/
Qt::HANDLE QSslKey::handle() const
{
    return d->handle();
}

/*!
    Returns \c true if this key is equal to \a other; otherwise returns \c false.
*/
bool QSslKey::operator==(const QSslKey &other) const
{
    if (isNull())
        return other.isNull();
    if (other.isNull())
        return isNull();
    if (algorithm() != other.algorithm())
        return false;
    if (type() != other.type())
        return false;
    if (length() != other.length())
        return false;
    if (algorithm() == QSsl::Opaque)
        return handle() == other.handle();
    return toDer() == other.toDer();
}

/*! \fn bool QSslKey::operator!=(const QSslKey &other) const

  Returns \c true if this key is not equal to key \a other; otherwise
  returns \c false.
*/

#ifndef QT_NO_DEBUG_STREAM
class QDebug;
QDebug operator<<(QDebug debug, const QSslKey &key)
{
    debug << "QSslKey("
          << (key.type() == QSsl::PublicKey ? "PublicKey" : "PrivateKey")
          << ", " << (key.algorithm() == QSsl::Opaque ? "OPAQUE" :
                      (key.algorithm() == QSsl::Rsa ? "RSA" : "DSA"))
          << ", " << key.length()
          << ')';
    return debug;
}
#endif

QT_END_NAMESPACE
