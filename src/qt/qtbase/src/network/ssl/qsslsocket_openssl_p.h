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

/****************************************************************************
**
** In addition, as a special exception, the copyright holders listed above give
** permission to link the code of its release of Qt with the OpenSSL project's
** "OpenSSL" library (or modified versions of the "OpenSSL" library that use the
** same license as the original version), and distribute the linked executables.
**
** You must comply with the GNU General Public License version 2 in all
** respects for all of the code used other than the "OpenSSL" code.  If you
** modify this file, you may extend this exception to your version of the file,
** but you are not obligated to do so.  If you do not wish to do so, delete
** this exception statement from your version of this file.
**
****************************************************************************/

#ifndef QSSLSOCKET_OPENSSL_P_H
#define QSSLSOCKET_OPENSSL_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of the QLibrary class.  This header file may change from
// version to version without notice, or even be removed.
//
// We mean it.
//

#include "qsslsocket_p.h"

#ifdef Q_OS_WIN
#include <qt_windows.h>
#if defined(OCSP_RESPONSE)
#undef OCSP_RESPONSE
#endif
#if defined(X509_NAME)
#undef X509_NAME
#endif
#endif // Q_OS_WIN

#include <openssl/asn1.h>
#include <openssl/bio.h>
#include <openssl/bn.h>
#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/pkcs12.h>
#include <openssl/pkcs7.h>
#include <openssl/rand.h>
#include <openssl/ssl.h>
#include <openssl/stack.h>
#include <openssl/x509.h>
#include <openssl/x509v3.h>
#include <openssl/x509_vfy.h>
#include <openssl/dsa.h>
#include <openssl/rsa.h>
#include <openssl/crypto.h>
#if OPENSSL_VERSION_NUMBER >= 0x0090806fL && !defined(OPENSSL_NO_TLSEXT)
#include <openssl/tls1.h>
#endif

#if OPENSSL_VERSION_NUMBER >= 0x10000000L
typedef _STACK STACK;
#endif

QT_BEGIN_NAMESPACE

class QSslSocketBackendPrivate : public QSslSocketPrivate
{
    Q_DECLARE_PUBLIC(QSslSocket)
public:
    QSslSocketBackendPrivate();
    virtual ~QSslSocketBackendPrivate();

    // SSL context
    bool initSslContext();
    void destroySslContext();
    SSL *ssl;
    BIO *readBio;
    BIO *writeBio;
    SSL_SESSION *session;
    QList<QPair<int, int> > errorList;

    // Platform specific functions
    void startClientEncryption();
    void startServerEncryption();
    void transmit();
    bool startHandshake();
    void disconnectFromHost();
    void disconnected();
    QSslCipher sessionCipher() const;
    QSsl::SslProtocol sessionProtocol() const;
    void continueHandshake();
    bool checkSslErrors();
#ifdef Q_OS_WIN
    void fetchCaRootForCert(const QSslCertificate &cert);
    void _q_caRootLoaded(QSslCertificate,QSslCertificate);
#endif

    Q_AUTOTEST_EXPORT static long setupOpenSslOptions(QSsl::SslProtocol protocol, QSsl::SslOptions sslOptions);
    static QSslCipher QSslCipher_from_SSL_CIPHER(SSL_CIPHER *cipher);
    static QList<QSslCertificate> STACKOFX509_to_QSslCertificates(STACK_OF(X509) *x509);
    static QList<QSslError> verify(QList<QSslCertificate> certificateChain, const QString &hostName);
    static QString getErrorsFromOpenSsl();
    static bool importPkcs12(QIODevice *device,
                             QSslKey *key, QSslCertificate *cert,
                             QList<QSslCertificate> *caCertificates,
                             const QByteArray &passPhrase);
};

#ifdef Q_OS_WIN
class QWindowsCaRootFetcher : public QObject
{
    Q_OBJECT;
public:
    QWindowsCaRootFetcher(const QSslCertificate &certificate, QSslSocket::SslMode sslMode);
    ~QWindowsCaRootFetcher();
public slots:
    void start();
signals:
    void finished(QSslCertificate brokenChain, QSslCertificate caroot);
private:
    QSslCertificate cert;
    QSslSocket::SslMode mode;
};
#endif

QT_END_NAMESPACE

#endif
