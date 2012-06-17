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

//#define QSSLSOCKET_DEBUG

#include "qsslsocket_openssl_p.h"
#include "qsslsocket_openssl_symbols_p.h"
#include "qsslsocket.h"
#include "qsslcertificate_p.h"
#include "qsslcipher_p.h"

#include <QtCore/qdatetime.h>
#include <QtCore/qdebug.h>
#include <QtCore/qdir.h>
#include <QtCore/qdiriterator.h>
#include <QtCore/qelapsedtimer.h>
#include <QtCore/qfile.h>
#include <QtCore/qfileinfo.h>
#include <QtCore/qmutex.h>
#include <QtCore/qthread.h>
#include <QtCore/qurl.h>
#include <QtCore/qvarlengtharray.h>
#include <QLibrary> // for loading the security lib for the CA store

#if OPENSSL_VERSION_NUMBER >= 0x0090806fL && !defined(OPENSSL_NO_TLSEXT)
// Symbian does not seem to have the symbol for SNI defined
#ifndef SSL_CTRL_SET_TLSEXT_HOSTNAME
#define SSL_CTRL_SET_TLSEXT_HOSTNAME 55
#endif
#endif
QT_BEGIN_NAMESPACE

#if defined(Q_OS_MAC) && !defined(QT_NO_CORESERVICES)
#define kSecTrustSettingsDomainSystem 2 // so we do not need to include the header file
    PtrSecCertificateGetData QSslSocketPrivate::ptrSecCertificateGetData = 0;
    PtrSecTrustSettingsCopyCertificates QSslSocketPrivate::ptrSecTrustSettingsCopyCertificates = 0;
    PtrSecTrustCopyAnchorCertificates QSslSocketPrivate::ptrSecTrustCopyAnchorCertificates = 0;
#elif defined(Q_OS_WIN)
    PtrCertOpenSystemStoreW QSslSocketPrivate::ptrCertOpenSystemStoreW = 0;
    PtrCertFindCertificateInStore QSslSocketPrivate::ptrCertFindCertificateInStore = 0;
    PtrCertCloseStore QSslSocketPrivate::ptrCertCloseStore = 0;
#elif defined(Q_OS_SYMBIAN)
#include <e32base.h>
#include <e32std.h>
#include <e32debug.h>
#include <QtCore/private/qcore_symbian_p.h>
#endif

bool QSslSocketPrivate::s_libraryLoaded = false;
bool QSslSocketPrivate::s_loadedCiphersAndCerts = false;
bool QSslSocketPrivate::s_loadRootCertsOnDemand = false;

/* \internal

    From OpenSSL's thread(3) manual page:

    OpenSSL can safely be used in multi-threaded applications provided that at
    least two callback functions are set.

    locking_function(int mode, int n, const char *file, int line) is needed to
    perform locking on shared data structures.  (Note that OpenSSL uses a
    number of global data structures that will be implicitly shared
    when-whenever ever multiple threads use OpenSSL.)  Multi-threaded
    applications will crash at random if it is not set.  ...
    ...
    id_function(void) is a function that returns a thread ID. It is not
    needed on Windows nor on platforms where getpid() returns a different
    ID for each thread (most notably Linux)
*/
class QOpenSslLocks
{
public:
    inline QOpenSslLocks()
        : initLocker(QMutex::Recursive),
          locksLocker(QMutex::Recursive)
    {
        QMutexLocker locker(&locksLocker);
        int numLocks = q_CRYPTO_num_locks();
        locks = new QMutex *[numLocks];
        memset(locks, 0, numLocks * sizeof(QMutex *));
    }
    inline ~QOpenSslLocks()
    {
        QMutexLocker locker(&locksLocker);
        for (int i = 0; i < q_CRYPTO_num_locks(); ++i)
            delete locks[i];
        delete [] locks;

        QSslSocketPrivate::deinitialize();
    }
    inline QMutex *lock(int num)
    {
        QMutexLocker locker(&locksLocker);
        QMutex *tmp = locks[num];
        if (!tmp)
            tmp = locks[num] = new QMutex(QMutex::Recursive);
        return tmp;
    }

    QMutex *globalLock()
    {
        return &locksLocker;
    }

    QMutex *initLock()
    {
        return &initLocker;
    }

private:
    QMutex initLocker;
    QMutex locksLocker;
    QMutex **locks;
};
Q_GLOBAL_STATIC(QOpenSslLocks, openssl_locks)

extern "C" {
static void locking_function(int mode, int lockNumber, const char *, int)
{
    QMutex *mutex = openssl_locks()->lock(lockNumber);

    // Lock or unlock it
    if (mode & CRYPTO_LOCK)
        mutex->lock();
    else
        mutex->unlock();
}
static unsigned long id_function()
{
    return (quintptr)QThread::currentThreadId();
}
} // extern "C"

QSslSocketBackendPrivate::QSslSocketBackendPrivate()
    : ssl(0),
      ctx(0),
      pkey(0),
      readBio(0),
      writeBio(0),
      session(0)
{
    // Calls SSL_library_init().
    ensureInitialized();
}

QSslSocketBackendPrivate::~QSslSocketBackendPrivate()
{
    destroySslContext();
}

QSslCipher QSslSocketBackendPrivate::QSslCipher_from_SSL_CIPHER(SSL_CIPHER *cipher)
{
    QSslCipher ciph;

    char buf [256];
    QString descriptionOneLine = QString::fromLatin1(q_SSL_CIPHER_description(cipher, buf, sizeof(buf)));

    QStringList descriptionList = descriptionOneLine.split(QLatin1String(" "), QString::SkipEmptyParts);
    if (descriptionList.size() > 5) {
        // ### crude code.
        ciph.d->isNull = false;
        ciph.d->name = descriptionList.at(0);

        QString protoString = descriptionList.at(1);
        ciph.d->protocolString = protoString;
        ciph.d->protocol = QSsl::UnknownProtocol;
        if (protoString == QLatin1String("SSLv3"))
            ciph.d->protocol = QSsl::SslV3;
        else if (protoString == QLatin1String("SSLv2"))
            ciph.d->protocol = QSsl::SslV2;
        else if (protoString == QLatin1String("TLSv1"))
            ciph.d->protocol = QSsl::TlsV1;

        if (descriptionList.at(2).startsWith(QLatin1String("Kx=")))
            ciph.d->keyExchangeMethod = descriptionList.at(2).mid(3);
        if (descriptionList.at(3).startsWith(QLatin1String("Au=")))
            ciph.d->authenticationMethod = descriptionList.at(3).mid(3);
        if (descriptionList.at(4).startsWith(QLatin1String("Enc=")))
            ciph.d->encryptionMethod = descriptionList.at(4).mid(4);
        ciph.d->exportable = (descriptionList.size() > 6 && descriptionList.at(6) == QLatin1String("export"));

        ciph.d->bits = cipher->strength_bits;
        ciph.d->supportedBits = cipher->alg_bits;

    }
    return ciph;
}

// ### This list is shared between all threads, and protected by a
// mutex. Investigate using thread local storage instead.
struct QSslErrorList
{
    QMutex mutex;
    QList<QPair<int, int> > errors;
};
Q_GLOBAL_STATIC(QSslErrorList, _q_sslErrorList)
static int q_X509Callback(int ok, X509_STORE_CTX *ctx)
{
    if (!ok) {
        // Store the error and at which depth the error was detected.
        _q_sslErrorList()->errors << qMakePair<int, int>(ctx->error, ctx->error_depth);
    }
    // Always return OK to allow verification to continue. We're handle the
    // errors gracefully after collecting all errors, after verification has
    // completed.
    return 1;
}

bool QSslSocketBackendPrivate::initSslContext()
{
    Q_Q(QSslSocket);

    // Create and initialize SSL context. Accept SSLv2, SSLv3 and TLSv1.
    bool client = (mode == QSslSocket::SslClientMode);

    bool reinitialized = false;
init_context:
    switch (configuration.protocol) {
    case QSsl::SslV2:
#ifndef OPENSSL_NO_SSL2
        ctx = q_SSL_CTX_new(client ? q_SSLv2_client_method() : q_SSLv2_server_method());
#else
        ctx = 0; // SSL 2 not supported by the system, but chosen deliberately -> error
#endif
        break;
    case QSsl::SslV3:
        ctx = q_SSL_CTX_new(client ? q_SSLv3_client_method() : q_SSLv3_server_method());
        break;
    case QSsl::SecureProtocols: // SslV2 will be disabled below
    case QSsl::TlsV1SslV3: // SslV2 will be disabled below
    case QSsl::AnyProtocol:
    default:
        ctx = q_SSL_CTX_new(client ? q_SSLv23_client_method() : q_SSLv23_server_method());
        break;
    case QSsl::TlsV1:
        ctx = q_SSL_CTX_new(client ? q_TLSv1_client_method() : q_TLSv1_server_method());
        break;
    }
    if (!ctx) {
        // After stopping Flash 10 the SSL library looses its ciphers. Try re-adding them
        // by re-initializing the library.
        if (!reinitialized) {
            reinitialized = true;
            if (q_SSL_library_init() == 1)
                goto init_context;
        }

        // ### Bad error code
        q->setErrorString(QSslSocket::tr("Error creating SSL context (%1)").arg(getErrorsFromOpenSsl()));
        q->setSocketError(QAbstractSocket::UnknownSocketError);
        emit q->error(QAbstractSocket::UnknownSocketError);
        return false;
    }

    // Enable bug workarounds.
    long options;
    if (configuration.protocol == QSsl::TlsV1SslV3 || configuration.protocol == QSsl::SecureProtocols)
        options = SSL_OP_ALL|SSL_OP_NO_SSLv2;
    else
        options = SSL_OP_ALL;

    // This option is disabled by default, so we need to be able to clear it
    if (configuration.sslOptions & QSsl::SslOptionDisableEmptyFragments)
        options |= SSL_OP_DONT_INSERT_EMPTY_FRAGMENTS;
    else
        options &= ~SSL_OP_DONT_INSERT_EMPTY_FRAGMENTS;

#ifdef SSL_OP_ALLOW_UNSAFE_LEGACY_RENEGOTIATION
    // This option is disabled by default, so we need to be able to clear it
    if (configuration.sslOptions & QSsl::SslOptionDisableLegacyRenegotiation)
        options &= ~SSL_OP_ALLOW_UNSAFE_LEGACY_RENEGOTIATION;
    else
        options |= SSL_OP_ALLOW_UNSAFE_LEGACY_RENEGOTIATION;
#endif

#ifdef SSL_OP_NO_TICKET
    if (configuration.sslOptions & QSsl::SslOptionDisableSessionTickets)
        options |= SSL_OP_NO_TICKET;
#endif
#ifdef SSL_OP_NO_COMPRESSION
    if (configuration.sslOptions & QSsl::SslOptionDisableCompression)
        options |= SSL_OP_NO_COMPRESSION;
#endif

    q_SSL_CTX_set_options(ctx, options);

    // Initialize ciphers
    QByteArray cipherString;
    int first = true;
    QList<QSslCipher> ciphers = configuration.ciphers;
    if (ciphers.isEmpty())
        ciphers = defaultCiphers();
    foreach (const QSslCipher &cipher, ciphers) {
        if (first)
            first = false;
        else
            cipherString.append(':');
        cipherString.append(cipher.name().toLatin1());
    }

    if (!q_SSL_CTX_set_cipher_list(ctx, cipherString.data())) {
        // ### Bad error code
        q->setErrorString(QSslSocket::tr("Invalid or empty cipher list (%1)").arg(getErrorsFromOpenSsl()));
        q->setSocketError(QAbstractSocket::UnknownSocketError);
        emit q->error(QAbstractSocket::UnknownSocketError);
        return false;
    }

    // Add all our CAs to this store.
    QList<QSslCertificate> expiredCerts;
    foreach (const QSslCertificate &caCertificate, q->caCertificates()) {
        // add expired certs later, so that the
        // valid ones are used before the expired ones
        if (! caCertificate.isValid()) {
            expiredCerts.append(caCertificate);
        } else {
            q_X509_STORE_add_cert(ctx->cert_store, (X509 *)caCertificate.handle());
        }
    }

    bool addExpiredCerts = true;
#if defined(Q_OS_MAC) && (MAC_OS_X_VERSION_MAX_ALLOWED == MAC_OS_X_VERSION_10_5)
    //On Leopard SSL does not work if we add the expired certificates.
    if (QSysInfo::MacintoshVersion == QSysInfo::MV_10_5)
       addExpiredCerts = false;
#endif
    // now add the expired certs
    if (addExpiredCerts) {
        foreach (const QSslCertificate &caCertificate, expiredCerts) {
            q_X509_STORE_add_cert(ctx->cert_store, (X509 *)caCertificate.handle());
        }
    }

    if (s_loadRootCertsOnDemand && allowRootCertOnDemandLoading) {
        // tell OpenSSL the directories where to look up the root certs on demand
        QList<QByteArray> unixDirs = unixRootCertDirectories();
        for (int a = 0; a < unixDirs.count(); ++a)
            q_SSL_CTX_load_verify_locations(ctx, 0, unixDirs.at(a).constData());
    }

    // Register a custom callback to get all verification errors.
    X509_STORE_set_verify_cb_func(ctx->cert_store, q_X509Callback);

    if (!configuration.localCertificate.isNull()) {
        // Require a private key as well.
        if (configuration.privateKey.isNull()) {
            q->setErrorString(QSslSocket::tr("Cannot provide a certificate with no key, %1").arg(getErrorsFromOpenSsl()));
            emit q->error(QAbstractSocket::UnknownSocketError);
            return false;
        }

        // Load certificate
        if (!q_SSL_CTX_use_certificate(ctx, (X509 *)configuration.localCertificate.handle())) {
            q->setErrorString(QSslSocket::tr("Error loading local certificate, %1").arg(getErrorsFromOpenSsl()));
            emit q->error(QAbstractSocket::UnknownSocketError);
            return false;
        }

        // Load private key
        pkey = q_EVP_PKEY_new();
        // before we were using EVP_PKEY_assign_R* functions and did not use EVP_PKEY_free.
        // this lead to a memory leak. Now we use the *_set1_* functions which do not
        // take ownership of the RSA/DSA key instance because the QSslKey already has ownership.
        if (configuration.privateKey.algorithm() == QSsl::Rsa)
            q_EVP_PKEY_set1_RSA(pkey, (RSA *)configuration.privateKey.handle());
        else
            q_EVP_PKEY_set1_DSA(pkey, (DSA *)configuration.privateKey.handle());
        if (!q_SSL_CTX_use_PrivateKey(ctx, pkey)) {
            q->setErrorString(QSslSocket::tr("Error loading private key, %1").arg(getErrorsFromOpenSsl()));
            emit q->error(QAbstractSocket::UnknownSocketError);
            return false;
        }

        // Check if the certificate matches the private key.
        if (!q_SSL_CTX_check_private_key(ctx)) {
            q->setErrorString(QSslSocket::tr("Private key does not certify public key, %1").arg(getErrorsFromOpenSsl()));
            emit q->error(QAbstractSocket::UnknownSocketError);
            return false;
        }
    }

    // Initialize peer verification.
    if (configuration.peerVerifyMode == QSslSocket::VerifyNone) {
        q_SSL_CTX_set_verify(ctx, SSL_VERIFY_NONE, 0);
    } else {
        q_SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER, q_X509Callback);
    }

    // Set verification depth.
    if (configuration.peerVerifyDepth != 0)
        q_SSL_CTX_set_verify_depth(ctx, configuration.peerVerifyDepth);

    // Create and initialize SSL session
    if (!(ssl = q_SSL_new(ctx))) {
        // ### Bad error code
        q->setErrorString(QSslSocket::tr("Error creating SSL session, %1").arg(getErrorsFromOpenSsl()));
        q->setSocketError(QAbstractSocket::UnknownSocketError);
        emit q->error(QAbstractSocket::UnknownSocketError);
        return false;
    }

#if OPENSSL_VERSION_NUMBER >= 0x0090806fL && !defined(OPENSSL_NO_TLSEXT)
    if ((configuration.protocol == QSsl::TlsV1SslV3 ||
        configuration.protocol == QSsl::TlsV1 ||
        configuration.protocol == QSsl::SecureProtocols ||
        configuration.protocol == QSsl::AnyProtocol) &&
        client && q_SSLeay() >= 0x00090806fL) {
        // Set server hostname on TLS extension. RFC4366 section 3.1 requires it in ACE format.
        QString tlsHostName = verificationPeerName.isEmpty() ? q->peerName() : verificationPeerName;
        if (tlsHostName.isEmpty())
            tlsHostName = hostName;
        QByteArray ace = QUrl::toAce(tlsHostName);
        // only send the SNI header if the URL is valid and not an IP
        if (!ace.isEmpty()
            && !QHostAddress().setAddress(tlsHostName)
            && !(configuration.sslOptions & QSsl::SslOptionDisableServerNameIndication)) {
            if (!q_SSL_ctrl(ssl, SSL_CTRL_SET_TLSEXT_HOSTNAME, TLSEXT_NAMETYPE_host_name, ace.data()))
                qWarning("could not set SSL_CTRL_SET_TLSEXT_HOSTNAME, Server Name Indication disabled");
        }
    }
#endif

    // Clear the session.
    q_SSL_clear(ssl);
    errorList.clear();

    // Initialize memory BIOs for encryption and decryption.
    readBio = q_BIO_new(q_BIO_s_mem());
    writeBio = q_BIO_new(q_BIO_s_mem());
    if (!readBio || !writeBio) {
        // ### Bad error code
        q->setErrorString(QSslSocket::tr("Error creating SSL session: %1").arg(getErrorsFromOpenSsl()));
        q->setSocketError(QAbstractSocket::UnknownSocketError);
        emit q->error(QAbstractSocket::UnknownSocketError);
        return false;
    }

    // Assign the bios.
    q_SSL_set_bio(ssl, readBio, writeBio);

    if (mode == QSslSocket::SslClientMode)
        q_SSL_set_connect_state(ssl);
    else
        q_SSL_set_accept_state(ssl);

    return true;
}

void QSslSocketBackendPrivate::destroySslContext()
{
    if (ssl) {
        q_SSL_free(ssl);
        ssl = 0;
    }
    if (ctx) {
        q_SSL_CTX_free(ctx);
        ctx = 0;
    }
    if (pkey) {
        q_EVP_PKEY_free(pkey);
        pkey = 0;
    }
}

/*!
    \internal
*/
void QSslSocketPrivate::deinitialize()
{
    q_CRYPTO_set_id_callback(0);
    q_CRYPTO_set_locking_callback(0);
}

/*!
    \internal

    Does the minimum amount of initialization to determine whether SSL
    is supported or not.
*/

bool QSslSocketPrivate::supportsSsl()
{
    return ensureLibraryLoaded();
}

bool QSslSocketPrivate::ensureLibraryLoaded()
{
    if (!q_resolveOpenSslSymbols())
        return false;

    // Check if the library itself needs to be initialized.
    QMutexLocker locker(openssl_locks()->initLock());
    if (!s_libraryLoaded) {
        s_libraryLoaded = true;

        // Initialize OpenSSL.
        q_CRYPTO_set_id_callback(id_function);
        q_CRYPTO_set_locking_callback(locking_function);
        if (q_SSL_library_init() != 1)
            return false;
        q_SSL_load_error_strings();
        q_OpenSSL_add_all_algorithms();

        // Initialize OpenSSL's random seed.
        if (!q_RAND_status()) {
            struct {
                int msec;
                int sec;
                void *stack;
            } randomish;

            int attempts = 500;
            do {
                if (attempts < 500) {
#ifdef Q_OS_UNIX
                    struct timespec ts = {0, 33333333};
                    nanosleep(&ts, 0);
#else
                    Sleep(3);
#endif
                    randomish.msec = attempts;
                }
                randomish.stack = (void *)&randomish;
                randomish.msec = QTime::currentTime().msec();
                randomish.sec = QTime::currentTime().second();
                q_RAND_seed((const char *)&randomish, sizeof(randomish));
            } while (!q_RAND_status() && --attempts);
            if (!attempts)
                return false;
        }
    }
    return true;
}

void QSslSocketPrivate::ensureCiphersAndCertsLoaded()
{
    QMutexLocker locker(openssl_locks()->initLock());
    if (s_loadedCiphersAndCerts)
        return;
    s_loadedCiphersAndCerts = true;

    resetDefaultCiphers();

    //load symbols needed to receive certificates from system store
#if defined(Q_OS_MAC) && !defined(QT_NO_CORESERVICES)
    QLibrary securityLib("/System/Library/Frameworks/Security.framework/Versions/Current/Security");
    if (securityLib.load()) {
        ptrSecCertificateGetData = (PtrSecCertificateGetData) securityLib.resolve("SecCertificateGetData");
        if (!ptrSecCertificateGetData)
            qWarning("could not resolve symbols in security library"); // should never happen

        ptrSecTrustSettingsCopyCertificates = (PtrSecTrustSettingsCopyCertificates) securityLib.resolve("SecTrustSettingsCopyCertificates");
        if (!ptrSecTrustSettingsCopyCertificates) { // method was introduced in Leopard, use legacy method if it's not there
            ptrSecTrustCopyAnchorCertificates = (PtrSecTrustCopyAnchorCertificates) securityLib.resolve("SecTrustCopyAnchorCertificates");
            if (!ptrSecTrustCopyAnchorCertificates)
                qWarning("could not resolve symbols in security library"); // should never happen
        }
    } else {
        qWarning("could not load security library");
    }
#elif defined(Q_OS_WIN)
    HINSTANCE hLib = LoadLibraryW(L"Crypt32");
    if (hLib) {
#if defined(Q_OS_WINCE)
        ptrCertOpenSystemStoreW = (PtrCertOpenSystemStoreW)GetProcAddress(hLib, L"CertOpenStore");
        ptrCertFindCertificateInStore = (PtrCertFindCertificateInStore)GetProcAddress(hLib, L"CertFindCertificateInStore");
        ptrCertCloseStore = (PtrCertCloseStore)GetProcAddress(hLib, L"CertCloseStore");
#else
        ptrCertOpenSystemStoreW = (PtrCertOpenSystemStoreW)GetProcAddress(hLib, "CertOpenSystemStoreW");
        ptrCertFindCertificateInStore = (PtrCertFindCertificateInStore)GetProcAddress(hLib, "CertFindCertificateInStore");
        ptrCertCloseStore = (PtrCertCloseStore)GetProcAddress(hLib, "CertCloseStore");
#endif
        if (!ptrCertOpenSystemStoreW || !ptrCertFindCertificateInStore || !ptrCertCloseStore)
            qWarning("could not resolve symbols in crypt32 library"); // should never happen
    } else {
        qWarning("could not load crypt32 library"); // should never happen
    }
#elif defined(Q_OS_UNIX) && !defined(Q_OS_SYMBIAN) && !defined(Q_OS_MAC)
    // check whether we can enable on-demand root-cert loading (i.e. check whether the sym links are there)
    QList<QByteArray> dirs = unixRootCertDirectories();
    QStringList symLinkFilter;
    symLinkFilter << QLatin1String("[0-9a-f][0-9a-f][0-9a-f][0-9a-f][0-9a-f][0-9a-f][0-9a-f][0-9a-f].[0-9]");
    for (int a = 0; a < dirs.count(); ++a) {
        QDirIterator iterator(QLatin1String(dirs.at(a)), symLinkFilter, QDir::Files);
        if (iterator.hasNext()) {
            s_loadRootCertsOnDemand = true;
            break;
        }
    }
#endif
    // if on-demand loading was not enabled, load the certs now
    if (!s_loadRootCertsOnDemand)
        setDefaultCaCertificates(systemCaCertificates());
}

/*!
    \internal

    Declared static in QSslSocketPrivate, makes sure the SSL libraries have
    been initialized.
*/

void QSslSocketPrivate::ensureInitialized()
{
    if (!supportsSsl())
        return;

    ensureCiphersAndCertsLoaded();
}

/*!
    \internal

    Declared static in QSslSocketPrivate, backend-dependent loading of
    application-wide global ciphers.
*/
void QSslSocketPrivate::resetDefaultCiphers()
{
    SSL_CTX *myCtx = q_SSL_CTX_new(q_SSLv23_client_method());
    SSL *mySsl = q_SSL_new(myCtx);

    QList<QSslCipher> ciphers;

    STACK_OF(SSL_CIPHER) *supportedCiphers = q_SSL_get_ciphers(mySsl);
    for (int i = 0; i < q_sk_SSL_CIPHER_num(supportedCiphers); ++i) {
        if (SSL_CIPHER *cipher = q_sk_SSL_CIPHER_value(supportedCiphers, i)) {
            if (cipher->valid) {
                QSslCipher ciph = QSslSocketBackendPrivate::QSslCipher_from_SSL_CIPHER(cipher);
                if (!ciph.isNull()) {
                    if (!ciph.name().toLower().startsWith(QLatin1String("adh")))
                        ciphers << ciph;
                }
            }
        }
    }

    q_SSL_CTX_free(myCtx);
    q_SSL_free(mySsl);

    setDefaultSupportedCiphers(ciphers);
    setDefaultCiphers(ciphers);
}

#if defined(Q_OS_SYMBIAN)

CSymbianCertificateRetriever::CSymbianCertificateRetriever() : CActive(CActive::EPriorityStandard),
    iCertificatePtr(0,0,0), iSequenceError(KErrNone)
{
}

CSymbianCertificateRetriever::~CSymbianCertificateRetriever()
{
    iThread.Close();
}

CSymbianCertificateRetriever* CSymbianCertificateRetriever::NewL()
{
    CSymbianCertificateRetriever* self = new (ELeave) CSymbianCertificateRetriever();
    CleanupStack::PushL(self);
    self->ConstructL();
    CleanupStack::Pop();
    return self;
}

int CSymbianCertificateRetriever::GetCertificates(QList<QByteArray> &certificates)
{
    iCertificates = &certificates;

    TRequestStatus status;
    iThread.Logon(status);
    iThread.Resume();
    User::WaitForRequest(status);
    if (iThread.ExitType() == EExitKill)
        return KErrDied;
    else
        return status.Int();    // Logon() completes with the thread's exit value
}

void CSymbianCertificateRetriever::doThreadEntryL()
{
    CActiveScheduler* activeScheduler = new (ELeave) CActiveScheduler;
    CleanupStack::PushL(activeScheduler);
    CActiveScheduler::Install(activeScheduler);

    CActiveScheduler::Add(this);

    // These aren't deleted in the destructor so leaving the to CS is ok
    iCertStore = CUnifiedCertStore::NewLC(qt_s60GetRFs(), EFalse);
    iCertFilter = CCertAttributeFilter::NewLC();

    // only interested in CA certs
    iCertFilter->SetOwnerType(ECACertificate);
    // only interested in X.509 format (we don't support WAP formats)
    iCertFilter->SetFormat(EX509Certificate);

    // Kick off the sequence by initializing the cert store
    iState = Initializing;
    iCertStore->Initialize(iStatus);
    SetActive();

    CActiveScheduler::Start();

    // Sequence complete, clean up

    // These MUST be cleaned up before the installed CActiveScheduler is destroyed and can't be left to the
    // destructor of CSymbianCertificateRetriever. Otherwise the destructor of CActiveScheduler will get
    // stuck.
    iCertInfos.Close();
    CleanupStack::PopAndDestroy(3);     // activeScheduler, iCertStore, iCertFilter
}


TInt CSymbianCertificateRetriever::ThreadEntryPoint(TAny* aParams)
{
    User::SetCritical(User::EProcessCritical);
    CTrapCleanup* cleanupStack = CTrapCleanup::New();

    CSymbianCertificateRetriever* self = (CSymbianCertificateRetriever*) aParams;
    TRAPD(err, self->doThreadEntryL());
    delete cleanupStack;

    // doThreadEntryL() can leave only before the retrieval sequence is started
    if (err)
        return err;
    else
        return self->iSequenceError;    // return any error that occurred during the retrieval
}

void CSymbianCertificateRetriever::ConstructL()
{
    TInt err;
    int i=0;
    QString name(QLatin1String("CertWorkerThread-%1"));
    //recently closed thread names remain in use for a while until all handles have been closed
    //including users of RUndertaker
    do {
        err = iThread.Create(qt_QString2TPtrC(name.arg(i++)),
            CSymbianCertificateRetriever::ThreadEntryPoint, 16384, NULL, this);
    } while (err == KErrAlreadyExists);
    User::LeaveIfError(err);
}

void CSymbianCertificateRetriever::DoCancel()
{
    switch(iState) {
    case Initializing:
        iCertStore->CancelInitialize();
        break;
    case Listing:
        iCertStore->CancelList();
        break;
    case RetrievingCertificates:
        iCertStore->CancelGetCert();
        break;
    }
}

TInt CSymbianCertificateRetriever::RunError(TInt aError)
{
    // If something goes wrong in the sequence, abort the sequence
    iSequenceError = aError;    // this gets reported to the client in the TRequestStatus
    CActiveScheduler::Stop();
    return KErrNone;
}

void CSymbianCertificateRetriever::GetCertificateL()
{
    if (iCurrentCertIndex < iCertInfos.Count()) {
        CCTCertInfo* certInfo = iCertInfos[iCurrentCertIndex++];
        iCertificateData = QByteArray();
        QT_TRYCATCH_LEAVING(iCertificateData.resize(certInfo->Size()));
        iCertificatePtr.Set((TUint8*)iCertificateData.data(), 0, iCertificateData.size());
#ifdef QSSLSOCKET_DEBUG
        qDebug() << "getting " << qt_TDesC2QString(certInfo->Label()) << " size=" << certInfo->Size();
        qDebug() << "format=" << certInfo->CertificateFormat();
        qDebug() << "ownertype=" << certInfo->CertificateOwnerType();
        qDebug() << "type=" << hex << certInfo->Type().iUid;
#endif
        iCertStore->Retrieve(*certInfo, iCertificatePtr, iStatus);
        iState = RetrievingCertificates;
        SetActive();
    } else {
        //reached end of list
        CActiveScheduler::Stop();
    }
}

void CSymbianCertificateRetriever::RunL()
{
#ifdef QSSLSOCKET_DEBUG
    qDebug() << "CSymbianCertificateRetriever::RunL status " << iStatus.Int() << " count " << iCertInfos.Count() << " index " << iCurrentCertIndex;
#endif
    switch (iState) {
    case Initializing:
        User::LeaveIfError(iStatus.Int()); // initialise fail means pointless to continue
        iState = Listing;
        iCertStore->List(iCertInfos, *iCertFilter, iStatus);
        SetActive();
        break;

    case Listing:
        User::LeaveIfError(iStatus.Int()); // listing fail means pointless to continue
        iCurrentCertIndex = 0;
        GetCertificateL();
        break;

    case RetrievingCertificates:
        if (iStatus.Int() == KErrNone)
            iCertificates->append(iCertificateData);
        else
            qWarning() << "CSymbianCertificateRetriever: failed to retrieve a certificate, error " << iStatus.Int();
        GetCertificateL();
        break;
    }
}
#endif // defined(Q_OS_SYMBIAN)

QList<QSslCertificate> QSslSocketPrivate::systemCaCertificates()
{
    ensureInitialized();
#ifdef QSSLSOCKET_DEBUG
    QElapsedTimer timer;
    timer.start();
#endif
    QList<QSslCertificate> systemCerts;
#if defined(Q_OS_MAC) && !defined(QT_NO_CORESERVICES)
    CFArrayRef cfCerts;
    OSStatus status = 1;

    OSStatus SecCertificateGetData (
       SecCertificateRef certificate,
       CSSM_DATA_PTR data
    );

    if (ptrSecCertificateGetData) {
        if (ptrSecTrustSettingsCopyCertificates)
            status = ptrSecTrustSettingsCopyCertificates(kSecTrustSettingsDomainSystem, &cfCerts);
        else if (ptrSecTrustCopyAnchorCertificates)
            status = ptrSecTrustCopyAnchorCertificates(&cfCerts);
        if (!status) {
            CFIndex size = CFArrayGetCount(cfCerts);
            for (CFIndex i = 0; i < size; ++i) {
                SecCertificateRef cfCert = (SecCertificateRef)CFArrayGetValueAtIndex(cfCerts, i);
                CSSM_DATA data;
                CSSM_DATA_PTR dataPtr = &data;
                if (ptrSecCertificateGetData(cfCert, dataPtr)) {
                    qWarning("error retrieving a CA certificate from the system store");
                } else {
                    int len = data.Length;
                    char *rawData = reinterpret_cast<char *>(data.Data);
                    QByteArray rawCert(rawData, len);
                    systemCerts.append(QSslCertificate::fromData(rawCert, QSsl::Der));
                }
            }
            CFRelease(cfCerts);
        }
        else {
           // no detailed error handling here
           qWarning("could not retrieve system CA certificates");
        }
    }
#elif defined(Q_OS_WIN)
    if (ptrCertOpenSystemStoreW && ptrCertFindCertificateInStore && ptrCertCloseStore) {
        HCERTSTORE hSystemStore;
#if defined(Q_OS_WINCE)
        hSystemStore = ptrCertOpenSystemStoreW(CERT_STORE_PROV_SYSTEM_W,
                                               0,
                                               0,
                                               CERT_STORE_NO_CRYPT_RELEASE_FLAG|CERT_SYSTEM_STORE_CURRENT_USER,
                                               L"ROOT");
#else
        hSystemStore = ptrCertOpenSystemStoreW(0, L"ROOT");
#endif
        if(hSystemStore) {
            PCCERT_CONTEXT pc = NULL;
            while(1) {
                pc = ptrCertFindCertificateInStore( hSystemStore, X509_ASN_ENCODING, 0, CERT_FIND_ANY, NULL, pc);
                if(!pc)
                    break;
                QByteArray der((const char *)(pc->pbCertEncoded), static_cast<int>(pc->cbCertEncoded));
                QSslCertificate cert(der, QSsl::Der);
                systemCerts.append(cert);
            }
            ptrCertCloseStore(hSystemStore, 0);
        }
    }
#elif defined(Q_OS_UNIX) && !defined(Q_OS_SYMBIAN)
    QSet<QString> certFiles;
    QList<QByteArray> directories = unixRootCertDirectories();
    QDir currentDir;
    QStringList nameFilters;
    nameFilters << QLatin1String("*.pem") << QLatin1String("*.crt");
    currentDir.setNameFilters(nameFilters);
    for (int a = 0; a < directories.count(); a++) {
        currentDir.setPath(QLatin1String(directories.at(a)));
        QDirIterator it(currentDir);
        while(it.hasNext()) {
            it.next();
            // use canonical path here to not load the same certificate twice if symlinked
            certFiles.insert(it.fileInfo().canonicalFilePath());
        }
    }
    QSetIterator<QString> it(certFiles);
    while(it.hasNext()) {
        systemCerts.append(QSslCertificate::fromPath(it.next()));
    }
    systemCerts.append(QSslCertificate::fromPath(QLatin1String("/etc/pki/tls/certs/ca-bundle.crt"), QSsl::Pem)); // Fedora, Mandriva
    systemCerts.append(QSslCertificate::fromPath(QLatin1String("/usr/local/share/certs/ca-root-nss.crt"), QSsl::Pem)); // FreeBSD's ca_root_nss

#elif defined(Q_OS_SYMBIAN)
    QList<QByteArray> certs;
    QScopedPointer<CSymbianCertificateRetriever> retriever(CSymbianCertificateRetriever::NewL());

    retriever->GetCertificates(certs);
    foreach (const QByteArray &encodedCert, certs) {
        QSslCertificate cert(encodedCert, QSsl::Der);
        if (!cert.isNull()) {
#ifdef QSSLSOCKET_DEBUG
            qDebug() << "imported certificate: " << cert.issuerInfo(QSslCertificate::CommonName);
#endif
            systemCerts.append(cert);
        }
    }
#endif
#ifdef QSSLSOCKET_DEBUG
    qDebug() << "systemCaCertificates retrieval time " << timer.elapsed() << "ms";
    qDebug() << "imported " << systemCerts.count() << " certificates";
#endif

    return systemCerts;
}

void QSslSocketBackendPrivate::startClientEncryption()
{
    if (!initSslContext()) {
        // ### report error: internal OpenSSL failure
        return;
    }

    // Start connecting. This will place outgoing data in the BIO, so we
    // follow up with calling transmit().
    startHandshake();
    transmit();
}

void QSslSocketBackendPrivate::startServerEncryption()
{
    if (!initSslContext()) {
        // ### report error: internal OpenSSL failure
        return;
    }

    // Start connecting. This will place outgoing data in the BIO, so we
    // follow up with calling transmit().
    startHandshake();
    transmit();
}

/*!
    \internal

    Transmits encrypted data between the BIOs and the socket.
*/
void QSslSocketBackendPrivate::transmit()
{
    Q_Q(QSslSocket);

    // If we don't have any SSL context, don't bother transmitting.
    if (!ssl)
        return;

    bool transmitting;
    do {
        transmitting = false;

        // If the connection is secure, we can transfer data from the write
        // buffer (in plain text) to the write BIO through SSL_write.
        if (connectionEncrypted && !writeBuffer.isEmpty()) {
            qint64 totalBytesWritten = 0;
            int nextDataBlockSize;
            while ((nextDataBlockSize = writeBuffer.nextDataBlockSize()) > 0) {
                int writtenBytes = q_SSL_write(ssl, writeBuffer.readPointer(), nextDataBlockSize);
                if (writtenBytes <= 0) {
                    // ### Better error handling.
                    q->setErrorString(QSslSocket::tr("Unable to write data: %1").arg(getErrorsFromOpenSsl()));
                    q->setSocketError(QAbstractSocket::UnknownSocketError);
                    emit q->error(QAbstractSocket::UnknownSocketError);
                    return;
                }
#ifdef QSSLSOCKET_DEBUG
                qDebug() << "QSslSocketBackendPrivate::transmit: encrypted" << writtenBytes << "bytes";
#endif
                writeBuffer.free(writtenBytes);
                totalBytesWritten += writtenBytes;

                if (writtenBytes < nextDataBlockSize) {
                    // break out of the writing loop and try again after we had read
                    transmitting = true;
                    break;
                }
            }

            if (totalBytesWritten > 0) {
                // Don't emit bytesWritten() recursively.
                if (!emittedBytesWritten) {
                    emittedBytesWritten = true;
                    emit q->bytesWritten(totalBytesWritten);
                    emittedBytesWritten = false;
                }
            }
        }

        // Check if we've got any data to be written to the socket.
        QVarLengthArray<char, 4096> data;
        int pendingBytes;
        while (plainSocket->isValid() && (pendingBytes = q_BIO_pending(writeBio)) > 0) {
            // Read encrypted data from the write BIO into a buffer.
            data.resize(pendingBytes);
            int encryptedBytesRead = q_BIO_read(writeBio, data.data(), pendingBytes);

            // Write encrypted data from the buffer to the socket.
            qint64 actualWritten = plainSocket->write(data.constData(), encryptedBytesRead);
#ifdef QSSLSOCKET_DEBUG
            qDebug() << "QSslSocketBackendPrivate::transmit: wrote" << encryptedBytesRead << "encrypted bytes to the socket" << actualWritten << "actual.";
#endif
            if (actualWritten < 0) {
                //plain socket write fails if it was in the pending close state.
                q->setErrorString(plainSocket->errorString());
                q->setSocketError(plainSocket->error());
                emit q->error(plainSocket->error());
                return;
            }
            transmitting = true;
        }

        // Check if we've got any data to be read from the socket.
        if (!connectionEncrypted || !readBufferMaxSize || readBuffer.size() < readBufferMaxSize)
            while ((pendingBytes = plainSocket->bytesAvailable()) > 0) {
                // Read encrypted data from the socket into a buffer.
                data.resize(pendingBytes);
                // just peek() here because q_BIO_write could write less data than expected
                int encryptedBytesRead = plainSocket->peek(data.data(), pendingBytes);
#ifdef QSSLSOCKET_DEBUG
                qDebug() << "QSslSocketBackendPrivate::transmit: read" << encryptedBytesRead << "encrypted bytes from the socket";
#endif
                // Write encrypted data from the buffer into the read BIO.
                int writtenToBio = q_BIO_write(readBio, data.constData(), encryptedBytesRead);

                // do the actual read() here and throw away the results.
                if (writtenToBio > 0) {
                    // ### TODO: make this cheaper by not making it memcpy. E.g. make it work with data=0x0 or make it work with seek
                    plainSocket->read(data.data(), writtenToBio);
                } else {
                    // ### Better error handling.
                    q->setErrorString(QSslSocket::tr("Unable to decrypt data: %1").arg(getErrorsFromOpenSsl()));
                    q->setSocketError(QAbstractSocket::UnknownSocketError);
                    emit q->error(QAbstractSocket::UnknownSocketError);
                    return;
                }

                transmitting = true;
            }

        // If the connection isn't secured yet, this is the time to retry the
        // connect / accept.
        if (!connectionEncrypted) {
#ifdef QSSLSOCKET_DEBUG
            qDebug() << "QSslSocketBackendPrivate::transmit: testing encryption";
#endif
            if (startHandshake()) {
#ifdef QSSLSOCKET_DEBUG
                qDebug() << "QSslSocketBackendPrivate::transmit: encryption established";
#endif
                connectionEncrypted = true;
                transmitting = true;
            } else if (plainSocket->state() != QAbstractSocket::ConnectedState) {
#ifdef QSSLSOCKET_DEBUG
                qDebug() << "QSslSocketBackendPrivate::transmit: connection lost";
#endif
                break;
            } else {
#ifdef QSSLSOCKET_DEBUG
                qDebug() << "QSslSocketBackendPrivate::transmit: encryption not done yet";
#endif
            }
        }

        // If the request is small and the remote host closes the transmission
        // after sending, there's a chance that startHandshake() will already
        // have triggered a shutdown.
        if (!ssl)
            continue;

        // We always read everything from the SSL decryption buffers, even if
        // we have a readBufferMaxSize. There's no point in leaving data there
        // just so that readBuffer.size() == readBufferMaxSize.
        int readBytes = 0;
        data.resize(4096);
        ::memset(data.data(), 0, data.size());
        do {
            // Don't use SSL_pending(). It's very unreliable.
            if ((readBytes = q_SSL_read(ssl, data.data(), data.size())) > 0) {
#ifdef QSSLSOCKET_DEBUG
                qDebug() << "QSslSocketBackendPrivate::transmit: decrypted" << readBytes << "bytes";
#endif
                char *ptr = readBuffer.reserve(readBytes);
                ::memcpy(ptr, data.data(), readBytes);

                if (readyReadEmittedPointer)
                    *readyReadEmittedPointer = true;
                emit q->readyRead();
                transmitting = true;
                continue;
            }

            // Error.
            switch (q_SSL_get_error(ssl, readBytes)) {
            case SSL_ERROR_WANT_READ:
            case SSL_ERROR_WANT_WRITE:
                // Out of data.
                break;
            case SSL_ERROR_ZERO_RETURN:
                // The remote host closed the connection.
#ifdef QSSLSOCKET_DEBUG
                qDebug() << "QSslSocketBackendPrivate::transmit: remote disconnect";
#endif
                plainSocket->disconnectFromHost();
                break;
            case SSL_ERROR_SYSCALL: // some IO error
            case SSL_ERROR_SSL: // error in the SSL library
                // we do not know exactly what the error is, nor whether we can recover from it,
                // so just return to prevent an endless loop in the outer "while" statement
                q->setErrorString(QSslSocket::tr("Error while reading: %1").arg(getErrorsFromOpenSsl()));
                q->setSocketError(QAbstractSocket::UnknownSocketError);
                emit q->error(QAbstractSocket::UnknownSocketError);
                return;
            default:
                // SSL_ERROR_WANT_CONNECT, SSL_ERROR_WANT_ACCEPT: can only happen with a
                // BIO_s_connect() or BIO_s_accept(), which we do not call.
                // SSL_ERROR_WANT_X509_LOOKUP: can only happen with a
                // SSL_CTX_set_client_cert_cb(), which we do not call.
                // So this default case should never be triggered.
                q->setErrorString(QSslSocket::tr("Error while reading: %1").arg(getErrorsFromOpenSsl()));
                q->setSocketError(QAbstractSocket::UnknownSocketError);
                emit q->error(QAbstractSocket::UnknownSocketError);
                break;
            }
        } while (ssl && readBytes > 0);
    } while (ssl && ctx && transmitting);
}

static QSslError _q_OpenSSL_to_QSslError(int errorCode, const QSslCertificate &cert)
{
    QSslError error;
    switch (errorCode) {
    case X509_V_OK:
        // X509_V_OK is also reported if the peer had no certificate.
        break;
    case X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT:
        error = QSslError(QSslError::UnableToGetIssuerCertificate, cert); break;
    case X509_V_ERR_UNABLE_TO_DECRYPT_CERT_SIGNATURE:
        error = QSslError(QSslError::UnableToDecryptCertificateSignature, cert); break;
    case X509_V_ERR_UNABLE_TO_DECODE_ISSUER_PUBLIC_KEY:
        error = QSslError(QSslError::UnableToDecodeIssuerPublicKey, cert); break;
    case X509_V_ERR_CERT_SIGNATURE_FAILURE:
        error = QSslError(QSslError::CertificateSignatureFailed, cert); break;
    case X509_V_ERR_CERT_NOT_YET_VALID:
        error = QSslError(QSslError::CertificateNotYetValid, cert); break;
    case X509_V_ERR_CERT_HAS_EXPIRED:
        error = QSslError(QSslError::CertificateExpired, cert); break;
    case X509_V_ERR_ERROR_IN_CERT_NOT_BEFORE_FIELD:
        error = QSslError(QSslError::InvalidNotBeforeField, cert); break;
    case X509_V_ERR_ERROR_IN_CERT_NOT_AFTER_FIELD:
        error = QSslError(QSslError::InvalidNotAfterField, cert); break;
    case X509_V_ERR_DEPTH_ZERO_SELF_SIGNED_CERT:
        error = QSslError(QSslError::SelfSignedCertificate, cert); break;
    case X509_V_ERR_SELF_SIGNED_CERT_IN_CHAIN:
        error = QSslError(QSslError::SelfSignedCertificateInChain, cert); break;
    case X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT_LOCALLY:
        error = QSslError(QSslError::UnableToGetLocalIssuerCertificate, cert); break;
    case X509_V_ERR_UNABLE_TO_VERIFY_LEAF_SIGNATURE:
        error = QSslError(QSslError::UnableToVerifyFirstCertificate, cert); break;
    case X509_V_ERR_CERT_REVOKED:
        error = QSslError(QSslError::CertificateRevoked, cert); break;
    case X509_V_ERR_INVALID_CA:
        error = QSslError(QSslError::InvalidCaCertificate, cert); break;
    case X509_V_ERR_PATH_LENGTH_EXCEEDED:
        error = QSslError(QSslError::PathLengthExceeded, cert); break;
    case X509_V_ERR_INVALID_PURPOSE:
        error = QSslError(QSslError::InvalidPurpose, cert); break;
    case X509_V_ERR_CERT_UNTRUSTED:
        error = QSslError(QSslError::CertificateUntrusted, cert); break;
    case X509_V_ERR_CERT_REJECTED:
        error = QSslError(QSslError::CertificateRejected, cert); break;
    default:
        error = QSslError(QSslError::UnspecifiedError, cert); break;
    }
    return error;
}

bool QSslSocketBackendPrivate::startHandshake()
{
    Q_Q(QSslSocket);

    // Check if the connection has been established. Get all errors from the
    // verification stage.
    _q_sslErrorList()->mutex.lock();
    _q_sslErrorList()->errors.clear();
    int result = (mode == QSslSocket::SslClientMode) ? q_SSL_connect(ssl) : q_SSL_accept(ssl);

    const QList<QPair<int, int> > &lastErrors = _q_sslErrorList()->errors;
    for (int i = 0; i < lastErrors.size(); ++i) {
        const QPair<int, int> &currentError = lastErrors.at(i);
        // Initialize the peer certificate chain in order to find which certificate caused this error
        if (configuration.peerCertificateChain.isEmpty())
            configuration.peerCertificateChain = STACKOFX509_to_QSslCertificates(q_SSL_get_peer_cert_chain(ssl));
        emit q->peerVerifyError(_q_OpenSSL_to_QSslError(currentError.first,
                                configuration.peerCertificateChain.value(currentError.second)));
        if (q->state() != QAbstractSocket::ConnectedState)
            break;
    }

    errorList << lastErrors;
    _q_sslErrorList()->mutex.unlock();

    // Connection aborted during handshake phase.
    if (q->state() != QAbstractSocket::ConnectedState)
        return false;

    // Check if we're encrypted or not.
    if (result <= 0) {
        switch (q_SSL_get_error(ssl, result)) {
        case SSL_ERROR_WANT_READ:
        case SSL_ERROR_WANT_WRITE:
            // The handshake is not yet complete.
            break;
        default:
            q->setErrorString(QSslSocket::tr("Error during SSL handshake: %1").arg(getErrorsFromOpenSsl()));
            q->setSocketError(QAbstractSocket::SslHandshakeFailedError);
#ifdef QSSLSOCKET_DEBUG
            qDebug() << "QSslSocketBackendPrivate::startHandshake: error!" << q->errorString();
#endif
            emit q->error(QAbstractSocket::SslHandshakeFailedError);
            q->abort();
        }
        return false;
    }

    // Store the peer certificate and chain. For clients, the peer certificate
    // chain includes the peer certificate; for servers, it doesn't. Both the
    // peer certificate and the chain may be empty if the peer didn't present
    // any certificate.
    if (configuration.peerCertificateChain.isEmpty())
        configuration.peerCertificateChain = STACKOFX509_to_QSslCertificates(q_SSL_get_peer_cert_chain(ssl));
    X509 *x509 = q_SSL_get_peer_certificate(ssl);
    configuration.peerCertificate = QSslCertificatePrivate::QSslCertificate_from_X509(x509);
    q_X509_free(x509);

    // Start translating errors.
    QList<QSslError> errors;

    // check the whole chain for blacklisting (including root, as we check for subjectInfo and issuer)
    foreach (const QSslCertificate &cert, configuration.peerCertificateChain) {
        if (QSslCertificatePrivate::isBlacklisted(cert)) {
            QSslError error(QSslError::CertificateBlacklisted, cert);
            errors << error;
            emit q->peerVerifyError(error);
            if (q->state() != QAbstractSocket::ConnectedState)
                return false;
        }
    }

    bool doVerifyPeer = configuration.peerVerifyMode == QSslSocket::VerifyPeer
                        || (configuration.peerVerifyMode == QSslSocket::AutoVerifyPeer
                            && mode == QSslSocket::SslClientMode);

    // Check the peer certificate itself. First try the subject's common name
    // (CN) as a wildcard, then try all alternate subject name DNS entries the
    // same way.
    if (!configuration.peerCertificate.isNull()) {
        // but only if we're a client connecting to a server
        // if we're the server, don't check CN
        if (mode == QSslSocket::SslClientMode) {
            QString peerName = (verificationPeerName.isEmpty () ? q->peerName() : verificationPeerName);
            QString commonName = configuration.peerCertificate.subjectInfo(QSslCertificate::CommonName);

            if (!isMatchingHostname(commonName.toLower(), peerName.toLower())) {
                bool matched = false;
                foreach (const QString &altName, configuration.peerCertificate
                         .alternateSubjectNames().values(QSsl::DnsEntry)) {
                    if (isMatchingHostname(altName.toLower(), peerName.toLower())) {
                        matched = true;
                        break;
                    }
                }

                if (!matched) {
                    // No matches in common names or alternate names.
                    QSslError error(QSslError::HostNameMismatch, configuration.peerCertificate);
                    errors << error;
                    emit q->peerVerifyError(error);
                    if (q->state() != QAbstractSocket::ConnectedState)
                        return false;
                }
            }
        }
    } else {
        // No peer certificate presented. Report as error if the socket
        // expected one.
        if (doVerifyPeer) {
            QSslError error(QSslError::NoPeerCertificate);
            errors << error;
            emit q->peerVerifyError(error);
            if (q->state() != QAbstractSocket::ConnectedState)
                return false;
        }
    }

    // Translate errors from the error list into QSslErrors.
    for (int i = 0; i < errorList.size(); ++i) {
        const QPair<int, int> &errorAndDepth = errorList.at(i);
        int err = errorAndDepth.first;
        int depth = errorAndDepth.second;
        errors << _q_OpenSSL_to_QSslError(err, configuration.peerCertificateChain.value(depth));
    }

    if (!errors.isEmpty()) {
        sslErrors = errors;
        emit q->sslErrors(errors);

        bool doEmitSslError;
        if (!ignoreErrorsList.empty()) {
            // check whether the errors we got are all in the list of expected errors
            // (applies only if the method QSslSocket::ignoreSslErrors(const QList<QSslError> &errors)
            // was called)
            doEmitSslError = false;
            for (int a = 0; a < errors.count(); a++) {
                if (!ignoreErrorsList.contains(errors.at(a))) {
                    doEmitSslError = true;
                    break;
                }
            }
        } else {
            // if QSslSocket::ignoreSslErrors(const QList<QSslError> &errors) was not called and
            // we get an SSL error, emit a signal unless we ignored all errors (by calling
            // QSslSocket::ignoreSslErrors() )
            doEmitSslError = !ignoreAllSslErrors;
        }
        // check whether we need to emit an SSL handshake error
        if (doVerifyPeer && doEmitSslError) {
            q->setErrorString(sslErrors.first().errorString());
            q->setSocketError(QAbstractSocket::SslHandshakeFailedError);
            emit q->error(QAbstractSocket::SslHandshakeFailedError);
            plainSocket->disconnectFromHost();
            return false;
        }
    } else {
        sslErrors.clear();
    }

    // if we have a max read buffer size, reset the plain socket's to 32k
    if (readBufferMaxSize)
        plainSocket->setReadBufferSize(32768);

    connectionEncrypted = true;
    emit q->encrypted();
    if (autoStartHandshake && pendingClose) {
        pendingClose = false;
        q->disconnectFromHost();
    }
    return true;
}

void QSslSocketBackendPrivate::disconnectFromHost()
{
    if (ssl) {
        q_SSL_shutdown(ssl);
        transmit();
    }
    plainSocket->disconnectFromHost();
}

void QSslSocketBackendPrivate::disconnected()
{
    if (plainSocket->bytesAvailable() <= 0)
        destroySslContext();
    //if there is still buffered data in the plain socket, don't destroy the ssl context yet.
    //it will be destroyed when the socket is deleted.
}

QSslCipher QSslSocketBackendPrivate::sessionCipher() const
{
    if (!ssl || !ctx)
        return QSslCipher();
#if OPENSSL_VERSION_NUMBER >= 0x10000000L
    // FIXME This is fairly evil, but needed to keep source level compatibility
    // with the OpenSSL 0.9.x implementation at maximum -- some other functions
    // don't take a const SSL_CIPHER* when they should
    SSL_CIPHER *sessionCipher = const_cast<SSL_CIPHER *>(q_SSL_get_current_cipher(ssl));
#else
    SSL_CIPHER *sessionCipher = q_SSL_get_current_cipher(ssl);
#endif
    return sessionCipher ? QSslCipher_from_SSL_CIPHER(sessionCipher) : QSslCipher();
}

QList<QSslCertificate> QSslSocketBackendPrivate::STACKOFX509_to_QSslCertificates(STACK_OF(X509) *x509)
{
    ensureInitialized();
    QList<QSslCertificate> certificates;
    for (int i = 0; i < q_sk_X509_num(x509); ++i) {
        if (X509 *entry = q_sk_X509_value(x509, i))
            certificates << QSslCertificatePrivate::QSslCertificate_from_X509(entry);
    }
    return certificates;
}

QString QSslSocketBackendPrivate::getErrorsFromOpenSsl()
{
    QString errorString;
    unsigned long errNum;
    while((errNum = q_ERR_get_error())) {
        if (! errorString.isEmpty())
            errorString.append(QLatin1String(", "));
        const char *error = q_ERR_error_string(errNum, NULL);
        errorString.append(QString::fromAscii(error)); // error is ascii according to man ERR_error_string
    }
    return errorString;
}

bool QSslSocketBackendPrivate::isMatchingHostname(const QString &cn, const QString &hostname)
{
    int wildcard = cn.indexOf(QLatin1Char('*'));

    // Check this is a wildcard cert, if not then just compare the strings
    if (wildcard < 0)
        return cn == hostname;

    int firstCnDot = cn.indexOf(QLatin1Char('.'));
    int secondCnDot = cn.indexOf(QLatin1Char('.'), firstCnDot+1);

    // Check at least 3 components
    if ((-1 == secondCnDot) || (secondCnDot+1 >= cn.length()))
        return false;

    // Check * is last character of 1st component (ie. there's a following .)
    if (wildcard+1 != firstCnDot)
        return false;

    // Check only one star
    if (cn.lastIndexOf(QLatin1Char('*')) != wildcard)
        return false;

    // Check characters preceding * (if any) match
    if (wildcard && (hostname.leftRef(wildcard) != cn.leftRef(wildcard)))
        return false;

    // Check characters following first . match
    if (hostname.midRef(hostname.indexOf(QLatin1Char('.'))) != cn.midRef(firstCnDot))
        return false;

    // Check if the hostname is an IP address, if so then wildcards are not allowed
    QHostAddress addr(hostname);
    if (!addr.isNull())
        return false;

    // Ok, I guess this was a wildcard CN and the hostname matches.
    return true;
}

QT_END_NAMESPACE
