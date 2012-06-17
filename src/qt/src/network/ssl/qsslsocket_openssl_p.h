/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtNetwork module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
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
#endif

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
    SSL_CTX *ctx;
    EVP_PKEY *pkey;
    BIO *readBio;
    BIO *writeBio;
    SSL_SESSION *session;
    X509_STORE *certificateStore;
    X509_STORE_CTX *certificateStoreCtx;
    QList<QPair<int, int> > errorList;

    // Platform specific functions
    void startClientEncryption();
    void startServerEncryption();
    void transmit();
    bool startHandshake();
    void disconnectFromHost();
    void disconnected();
    QSslCipher sessionCipher() const;

    static QSslCipher QSslCipher_from_SSL_CIPHER(SSL_CIPHER *cipher);
    static QList<QSslCertificate> STACKOFX509_to_QSslCertificates(STACK_OF(X509) *x509);
    Q_AUTOTEST_EXPORT static bool isMatchingHostname(const QString &cn, const QString &hostname);
    static QString getErrorsFromOpenSsl();
};

#if defined(Q_OS_SYMBIAN)

#include <QByteArray>
#include <e32base.h>
#include <f32file.h>
#include <unifiedcertstore.h>     // link against certstore.lib
#include <ccertattributefilter.h> // link against ctframework.lib

// The purpose of this class is to wrap the asynchronous API of Symbian certificate store to one
// synchronizable call. The user of this class needs to provide a TRequestStatus object which can
// be used with User::WaitForRequest() unlike with the calls of the certificate store API.
// A thread is used instead of a CActiveSchedulerWait scheme, because that would make the call
// asynchronous (other events might be processed during the call even though the call would be seemingly
// synchronous).

class CSymbianCertificateRetriever : public CActive
{
public:
    static CSymbianCertificateRetriever* NewL();
    ~CSymbianCertificateRetriever();

    int GetCertificates(QList<QByteArray> &aCertificates);

private:
    void ConstructL();
    CSymbianCertificateRetriever();
    static TInt ThreadEntryPoint(TAny* aParams);
    void doThreadEntryL();
    void GetCertificateL();
    void DoCancel();
    void RunL();
    TInt RunError(TInt aError);

private:
    enum {
        Initializing,
        Listing,
        RetrievingCertificates
    } iState;

    RThread iThread;
    CUnifiedCertStore* iCertStore;
    RMPointerArray<CCTCertInfo> iCertInfos;
    CCertAttributeFilter* iCertFilter;
    TInt iCurrentCertIndex;
    QByteArray iCertificateData;
    TPtr8 iCertificatePtr;
    QList<QByteArray>* iCertificates;
    TInt iSequenceError;
};


#endif


QT_END_NAMESPACE

#endif
