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

//#define QSSLSOCKET_DEBUG
//#define QT_DECRYPT_SSL_TRAFFIC

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

QT_BEGIN_NAMESPACE

#if defined(Q_OS_MACX)
#define kSecTrustSettingsDomainSystem 2 // so we do not need to include the header file
    PtrSecCertificateCopyData QSslSocketPrivate::ptrSecCertificateCopyData = 0;
    PtrSecTrustSettingsCopyCertificates QSslSocketPrivate::ptrSecTrustSettingsCopyCertificates = 0;
    PtrSecTrustCopyAnchorCertificates QSslSocketPrivate::ptrSecTrustCopyAnchorCertificates = 0;
#elif defined(Q_OS_WIN)
    PtrCertOpenSystemStoreW QSslSocketPrivate::ptrCertOpenSystemStoreW = 0;
    PtrCertFindCertificateInStore QSslSocketPrivate::ptrCertFindCertificateInStore = 0;
    PtrCertCloseStore QSslSocketPrivate::ptrCertCloseStore = 0;
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
    whenever multiple threads use OpenSSL.)  Multi-threaded
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

QString QSslSocketBackendPrivate::getErrorsFromOpenSsl()
{
    QString errorString;
    unsigned long errNum;
    while ((errNum = q_ERR_get_error())) {
        if (! errorString.isEmpty())
            errorString.append(QLatin1String(", "));
        const char *error = q_ERR_error_string(errNum, NULL);
        errorString.append(QString::fromLatin1(error)); // error is ascii according to man ERR_error_string
    }
    return errorString;
}

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
            ciph.d->protocol = QSsl::TlsV1_0;
        else if (protoString == QLatin1String("TLSv1.1"))
            ciph.d->protocol = QSsl::TlsV1_1;
        else if (protoString == QLatin1String("TLSv1.2"))
            ciph.d->protocol = QSsl::TlsV1_2;

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

int q_X509Callback(int ok, X509_STORE_CTX *ctx)
{
    if (!ok) {
        // Store the error and at which depth the error was detected.
        _q_sslErrorList()->errors << qMakePair<int, int>(q_X509_STORE_CTX_get_error(ctx), q_X509_STORE_CTX_get_error_depth(ctx));
#ifdef QSSLSOCKET_DEBUG
        qDebug() << "verification error: dumping bad certificate";
        qDebug() << QSslCertificatePrivate::QSslCertificate_from_X509(q_X509_STORE_CTX_get_current_cert(ctx)).toPem();
        qDebug() << "dumping chain";
        foreach (QSslCertificate cert, QSslSocketBackendPrivate::STACKOFX509_to_QSslCertificates(q_X509_STORE_CTX_get_chain(ctx))) {
            QString certFormat(QStringLiteral("O=%1 CN=%2 L=%3 OU=%4 C=%5 ST=%6"));
            qDebug() << "Issuer:" << "O=" << cert.issuerInfo(QSslCertificate::Organization)
                << "CN=" << cert.issuerInfo(QSslCertificate::CommonName)
                << "L=" << cert.issuerInfo(QSslCertificate::LocalityName)
                << "OU=" << cert.issuerInfo(QSslCertificate::OrganizationalUnitName)
                << "C=" << cert.issuerInfo(QSslCertificate::CountryName)
                << "ST=" << cert.issuerInfo(QSslCertificate::StateOrProvinceName);
            qDebug() << "Subject:" << "O=" << cert.subjectInfo(QSslCertificate::Organization)
                << "CN=" << cert.subjectInfo(QSslCertificate::CommonName)
                << "L=" << cert.subjectInfo(QSslCertificate::LocalityName)
                << "OU=" << cert.subjectInfo(QSslCertificate::OrganizationalUnitName)
                << "C=" << cert.subjectInfo(QSslCertificate::CountryName)
                << "ST=" << cert.subjectInfo(QSslCertificate::StateOrProvinceName);
            qDebug() << "Valid:" << cert.effectiveDate() << "-" << cert.expiryDate();
        }
#endif
    }
    // Always return OK to allow verification to continue. We're handle the
    // errors gracefully after collecting all errors, after verification has
    // completed.
    return 1;
}

long QSslSocketBackendPrivate::setupOpenSslOptions(QSsl::SslProtocol protocol, QSsl::SslOptions sslOptions)
{
    long options;
    if (protocol == QSsl::TlsV1SslV3 || protocol == QSsl::SecureProtocols)
        options = SSL_OP_ALL|SSL_OP_NO_SSLv2;
    else
        options = SSL_OP_ALL;

    // This option is disabled by default, so we need to be able to clear it
    if (sslOptions & QSsl::SslOptionDisableEmptyFragments)
        options |= SSL_OP_DONT_INSERT_EMPTY_FRAGMENTS;
    else
        options &= ~SSL_OP_DONT_INSERT_EMPTY_FRAGMENTS;

#ifdef SSL_OP_ALLOW_UNSAFE_LEGACY_RENEGOTIATION
    // This option is disabled by default, so we need to be able to clear it
    if (sslOptions & QSsl::SslOptionDisableLegacyRenegotiation)
        options &= ~SSL_OP_ALLOW_UNSAFE_LEGACY_RENEGOTIATION;
    else
        options |= SSL_OP_ALLOW_UNSAFE_LEGACY_RENEGOTIATION;
#endif

#ifdef SSL_OP_NO_TICKET
    if (sslOptions & QSsl::SslOptionDisableSessionTickets)
        options |= SSL_OP_NO_TICKET;
#endif
#ifdef SSL_OP_NO_COMPRESSION
    if (sslOptions & QSsl::SslOptionDisableCompression)
        options |= SSL_OP_NO_COMPRESSION;
#endif

    return options;
}

bool QSslSocketBackendPrivate::initSslContext()
{
    Q_Q(QSslSocket);

    // If no external context was set (e.g. bei QHttpNetworkConnection) we will create a default context
    if (!sslContextPointer) {
        // create a deep copy of our configuration
        QSslConfigurationPrivate *configurationCopy = new QSslConfigurationPrivate(configuration);
        configurationCopy->ref.store(0);              // the QSslConfiguration constructor refs up
        sslContextPointer = QSharedPointer<QSslContext>(
                    QSslContext::fromConfiguration(mode, configurationCopy, allowRootCertOnDemandLoading));
    }

    if (sslContextPointer->error() != QSslError::NoError) {
        q->setErrorString(sslContextPointer->errorString());
        q->setSocketError(QAbstractSocket::SslInvalidUserDataError);
        emit q->error(QAbstractSocket::SslInvalidUserDataError);
        sslContextPointer.clear(); // deletes the QSslContext
        return false;
    }

    // Create and initialize SSL session
    if (!(ssl = sslContextPointer->createSsl())) {
        // ### Bad error code
        q->setErrorString(QSslSocket::tr("Error creating SSL session, %1").arg(getErrorsFromOpenSsl()));
        q->setSocketError(QAbstractSocket::SslInternalError);
        emit q->error(QAbstractSocket::SslInternalError);
        return false;
    }

#if OPENSSL_VERSION_NUMBER >= 0x0090806fL && !defined(OPENSSL_NO_TLSEXT)
    if ((configuration.protocol == QSsl::TlsV1SslV3 ||
        configuration.protocol == QSsl::TlsV1_0 ||
        configuration.protocol == QSsl::TlsV1_1 ||
        configuration.protocol == QSsl::TlsV1_2 ||
        configuration.protocol == QSsl::SecureProtocols ||
        configuration.protocol == QSsl::AnyProtocol) &&
        mode == QSslSocket::SslClientMode && q_SSLeay() >= 0x00090806fL) {
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
    errorList.clear();

    // Initialize memory BIOs for encryption and decryption.
    readBio = q_BIO_new(q_BIO_s_mem());
    writeBio = q_BIO_new(q_BIO_s_mem());
    if (!readBio || !writeBio) {
        q->setErrorString(QSslSocket::tr("Error creating SSL session: %1").arg(getErrorsFromOpenSsl()));
        q->setSocketError(QAbstractSocket::SslInternalError);
        emit q->error(QAbstractSocket::SslInternalError);
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
    sslContextPointer.clear();
}

/*!
    \internal
*/
void QSslSocketPrivate::deinitialize()
{
    q_CRYPTO_set_id_callback(0);
    q_CRYPTO_set_locking_callback(0);
    q_ERR_free_strings();
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

#ifndef QT_NO_LIBRARY
    //load symbols needed to receive certificates from system store
#if defined(Q_OS_MACX)
    QLibrary securityLib("/System/Library/Frameworks/Security.framework/Versions/Current/Security");
    if (securityLib.load()) {
        ptrSecCertificateCopyData = (PtrSecCertificateCopyData) securityLib.resolve("SecCertificateCopyData");
        if (!ptrSecCertificateCopyData)
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
#elif defined(Q_OS_QNX)
    s_loadRootCertsOnDemand = true;
#elif defined(Q_OS_UNIX) && !defined(Q_OS_MAC)
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
#endif //QT_NO_LIBRARY
    // if on-demand loading was not enabled, load the certs now
    if (!s_loadRootCertsOnDemand)
        setDefaultCaCertificates(systemCaCertificates());
#ifdef Q_OS_WIN
    //Enabled for fetching additional root certs from windows update on windows 6+
    //This flag is set false by setDefaultCaCertificates() indicating the app uses
    //its own cert bundle rather than the system one.
    //Same logic that disables the unix on demand cert loading.
    //Unlike unix, we do preload the certificates from the cert store.
    if ((QSysInfo::windowsVersion() & QSysInfo::WV_NT_based) >= QSysInfo::WV_6_0)
        s_loadRootCertsOnDemand = true;
#endif
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

long QSslSocketPrivate::sslLibraryVersionNumber()
{
    if (!supportsSsl())
        return 0;

    return q_SSLeay();
}

QString QSslSocketPrivate::sslLibraryVersionString()
{
    if (!supportsSsl())
        return QString();

    const char *versionString = q_SSLeay_version(SSLEAY_VERSION);
    if (!versionString)
        return QString();

    return QString::fromLatin1(versionString);
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
    QList<QSslCipher> defaultCiphers;

    STACK_OF(SSL_CIPHER) *supportedCiphers = q_SSL_get_ciphers(mySsl);
    for (int i = 0; i < q_sk_SSL_CIPHER_num(supportedCiphers); ++i) {
        if (SSL_CIPHER *cipher = q_sk_SSL_CIPHER_value(supportedCiphers, i)) {
            if (cipher->valid) {
                QSslCipher ciph = QSslSocketBackendPrivate::QSslCipher_from_SSL_CIPHER(cipher);
                if (!ciph.isNull()) {
                    // Unconditionally exclude ADH ciphers since they offer no MITM protection
                    if (!ciph.name().toLower().startsWith(QLatin1String("adh")))
                        ciphers << ciph;
                    if (ciph.usedBits() >= 128)
                        defaultCiphers << ciph;
                }
            }
        }
    }

    q_SSL_CTX_free(myCtx);
    q_SSL_free(mySsl);

    setDefaultSupportedCiphers(ciphers);
    setDefaultCiphers(defaultCiphers);
}

QList<QSslCertificate> QSslSocketPrivate::systemCaCertificates()
{
    ensureInitialized();
#ifdef QSSLSOCKET_DEBUG
    QElapsedTimer timer;
    timer.start();
#endif
    QList<QSslCertificate> systemCerts;
#if defined(Q_OS_MACX)
    CFArrayRef cfCerts;
    OSStatus status = 1;

    CFDataRef SecCertificateCopyData (
       SecCertificateRef certificate
    );

    if (ptrSecCertificateCopyData) {
        if (ptrSecTrustSettingsCopyCertificates)
            status = ptrSecTrustSettingsCopyCertificates(kSecTrustSettingsDomainSystem, &cfCerts);
        else if (ptrSecTrustCopyAnchorCertificates)
            status = ptrSecTrustCopyAnchorCertificates(&cfCerts);
        if (!status) {
            CFIndex size = CFArrayGetCount(cfCerts);
            for (CFIndex i = 0; i < size; ++i) {
                SecCertificateRef cfCert = (SecCertificateRef)CFArrayGetValueAtIndex(cfCerts, i);
                CFDataRef data;

                data = ptrSecCertificateCopyData(cfCert);

                if (data == NULL) {
                    qWarning("error retrieving a CA certificate from the system store");
                } else {
                    QByteArray rawCert = QByteArray::fromRawData((const char *)CFDataGetBytePtr(data), CFDataGetLength(data));
                    systemCerts.append(QSslCertificate::fromData(rawCert, QSsl::Der));
                    CFRelease(data);
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
#elif defined(Q_OS_UNIX)
    QSet<QString> certFiles;
    QDir currentDir;
    QStringList nameFilters;
    QList<QByteArray> directories;
    QSsl::EncodingFormat platformEncodingFormat;
# ifndef Q_OS_ANDROID
    directories = unixRootCertDirectories();
    nameFilters << QLatin1String("*.pem") << QLatin1String("*.crt");
    platformEncodingFormat = QSsl::Pem;
# else
    // Q_OS_ANDROID
    QByteArray ministroPath = qgetenv("MINISTRO_SSL_CERTS_PATH"); // Set by Ministro
    directories << ministroPath;
    nameFilters << QLatin1String("*.der");
    platformEncodingFormat = QSsl::Der;
#  ifndef Q_OS_ANDROID_NO_SDK
    if (ministroPath.isEmpty()) {
        QList<QByteArray> certificateData = fetchSslCertificateData();
        for (int i = 0; i < certificateData.size(); ++i) {
            systemCerts.append(QSslCertificate::fromData(certificateData.at(i), QSsl::Der));
        }
    } else
#   endif //Q_OS_ANDROID_NO_SDK
# endif //Q_OS_ANDROID
    {
        currentDir.setNameFilters(nameFilters);
        for (int a = 0; a < directories.count(); a++) {
            currentDir.setPath(QLatin1String(directories.at(a)));
            QDirIterator it(currentDir);
            while (it.hasNext()) {
                it.next();
                // use canonical path here to not load the same certificate twice if symlinked
                certFiles.insert(it.fileInfo().canonicalFilePath());
            }
        }
        QSetIterator<QString> it(certFiles);
        while (it.hasNext())
            systemCerts.append(QSslCertificate::fromPath(it.next(), platformEncodingFormat));
# ifndef Q_OS_ANDROID
        systemCerts.append(QSslCertificate::fromPath(QLatin1String("/etc/pki/tls/certs/ca-bundle.crt"), QSsl::Pem)); // Fedora, Mandriva
        systemCerts.append(QSslCertificate::fromPath(QLatin1String("/usr/local/share/certs/ca-root-nss.crt"), QSsl::Pem)); // FreeBSD's ca_root_nss
# endif
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
    Q_Q(QSslSocket);
    if (!initSslContext()) {
        q->setErrorString(QSslSocket::tr("Unable to init SSL Context: %1").arg(getErrorsFromOpenSsl()));
        q->setSocketError(QAbstractSocket::SslInternalError);
        emit q->error(QAbstractSocket::SslInternalError);
        return;
    }

    // Start connecting. This will place outgoing data in the BIO, so we
    // follow up with calling transmit().
    startHandshake();
    transmit();
}

void QSslSocketBackendPrivate::startServerEncryption()
{
    Q_Q(QSslSocket);
    if (!initSslContext()) {
        q->setErrorString(QSslSocket::tr("Unable to init SSL Context: %1").arg(getErrorsFromOpenSsl()));
        q->setSocketError(QAbstractSocket::SslInternalError);
        emit q->error(QAbstractSocket::SslInternalError);
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
                    int error = q_SSL_get_error(ssl, writtenBytes);
                    //write can result in a want_write_error - not an error - continue transmitting
                    if (error == SSL_ERROR_WANT_WRITE) {
                        transmitting = true;
                        break;
                    } else if (error == SSL_ERROR_WANT_READ) {
                        //write can result in a want_read error, possibly due to renegotiation - not an error - stop transmitting
                        transmitting = false;
                        break;
                    } else {
                        // ### Better error handling.
                        q->setErrorString(QSslSocket::tr("Unable to write data: %1").arg(getErrorsFromOpenSsl()));
                        q->setSocketError(QAbstractSocket::SslInternalError);
                        emit q->error(QAbstractSocket::SslInternalError);
                        return;
                    }
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
        if (!connectionEncrypted || !readBufferMaxSize || buffer.size() < readBufferMaxSize)
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
                    q->setSocketError(QAbstractSocket::SslInternalError);
                    emit q->error(QAbstractSocket::SslInternalError);
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
            } else if (paused) {
                // just wait until the user continues
                return;
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
                char *ptr = buffer.reserve(readBytes);
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
                shutdown = true; // the other side shut down, make sure we do not send shutdown ourselves
                q->setErrorString(QSslSocket::tr("The TLS/SSL connection has been closed"));
                q->setSocketError(QAbstractSocket::RemoteHostClosedError);
                emit q->error(QAbstractSocket::RemoteHostClosedError);
                return;
            case SSL_ERROR_SYSCALL: // some IO error
            case SSL_ERROR_SSL: // error in the SSL library
                // we do not know exactly what the error is, nor whether we can recover from it,
                // so just return to prevent an endless loop in the outer "while" statement
                q->setErrorString(QSslSocket::tr("Error while reading: %1").arg(getErrorsFromOpenSsl()));
                q->setSocketError(QAbstractSocket::SslInternalError);
                emit q->error(QAbstractSocket::SslInternalError);
                return;
            default:
                // SSL_ERROR_WANT_CONNECT, SSL_ERROR_WANT_ACCEPT: can only happen with a
                // BIO_s_connect() or BIO_s_accept(), which we do not call.
                // SSL_ERROR_WANT_X509_LOOKUP: can only happen with a
                // SSL_CTX_set_client_cert_cb(), which we do not call.
                // So this default case should never be triggered.
                q->setErrorString(QSslSocket::tr("Error while reading: %1").arg(getErrorsFromOpenSsl()));
                q->setSocketError(QAbstractSocket::SslInternalError);
                emit q->error(QAbstractSocket::SslInternalError);
                break;
            }
        } while (ssl && readBytes > 0);
    } while (ssl && transmitting);
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

            if (!isMatchingHostname(configuration.peerCertificate, peerName)) {
                // No matches in common names or alternate names.
                QSslError error(QSslError::HostNameMismatch, configuration.peerCertificate);
                errors << error;
                emit q->peerVerifyError(error);
                if (q->state() != QAbstractSocket::ConnectedState)
                    return false;
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

#ifdef Q_OS_WIN
        //Skip this if not using system CAs, or if the SSL errors are configured in advance to be ignorable
        if (doVerifyPeer
            && s_loadRootCertsOnDemand
            && allowRootCertOnDemandLoading
            && !verifyErrorsHaveBeenIgnored()) {
            //Windows desktop versions starting from vista ship with minimal set of roots
            //and download on demand from the windows update server CA roots that are
            //trusted by MS.
            //However, this is only transparent if using WinINET - we have to trigger it
            //ourselves.
            QSslCertificate certToFetch;
            bool fetchCertificate = true;
            for (int i=0; i< sslErrors.count(); i++) {
                switch (sslErrors.at(i).error()) {
                case QSslError::UnableToGetLocalIssuerCertificate: // site presented intermediate cert, but root is unknown
                case QSslError::SelfSignedCertificateInChain: // site presented a complete chain, but root is unknown
                    certToFetch = sslErrors.at(i).certificate();
                    break;
                case QSslError::SelfSignedCertificate:
                case QSslError::CertificateBlacklisted:
                    //With these errors, we know it will be untrusted so save time by not asking windows
                    fetchCertificate = false;
                    break;
                default:
#ifdef QSSLSOCKET_DEBUG
                    qDebug() << sslErrors.at(i).errorString();
#endif
                    break;
                }
            }
            if (fetchCertificate && !certToFetch.isNull()) {
                fetchCaRootForCert(certToFetch);
                return false;
            }
        }
#endif
        if (!checkSslErrors())
            return false;
    } else {
        sslErrors.clear();
    }

    continueHandshake();
    return true;
}

bool QSslSocketBackendPrivate::checkSslErrors()
{
    Q_Q(QSslSocket);
    if (sslErrors.isEmpty())
        return true;

    emit q->sslErrors(sslErrors);

    bool doVerifyPeer = configuration.peerVerifyMode == QSslSocket::VerifyPeer
                        || (configuration.peerVerifyMode == QSslSocket::AutoVerifyPeer
                            && mode == QSslSocket::SslClientMode);
    bool doEmitSslError = !verifyErrorsHaveBeenIgnored();
    // check whether we need to emit an SSL handshake error
    if (doVerifyPeer && doEmitSslError) {
        if (q->pauseMode() & QAbstractSocket::PauseOnSslErrors) {
            pauseSocketNotifiers(q);
            paused = true;
        } else {
            q->setErrorString(sslErrors.first().errorString());
            q->setSocketError(QAbstractSocket::SslHandshakeFailedError);
            emit q->error(QAbstractSocket::SslHandshakeFailedError);
            plainSocket->disconnectFromHost();
        }
        return false;
    }
    return true;
}

#ifdef Q_OS_WIN

void QSslSocketBackendPrivate::fetchCaRootForCert(const QSslCertificate &cert)
{
    Q_Q(QSslSocket);
    //The root certificate is downloaded from windows update, which blocks for 15 seconds in the worst case
    //so the request is done in a worker thread.
    QWindowsCaRootFetcher *fetcher = new QWindowsCaRootFetcher(cert, mode);
    QObject::connect(fetcher, SIGNAL(finished(QSslCertificate,QSslCertificate)), q, SLOT(_q_caRootLoaded(QSslCertificate,QSslCertificate)), Qt::QueuedConnection);
    QMetaObject::invokeMethod(fetcher, "start", Qt::QueuedConnection);
    pauseSocketNotifiers(q);
    paused = true;
}

//This is the callback from QWindowsCaRootFetcher, trustedRoot will be invalid (default constructed) if it failed.
void QSslSocketBackendPrivate::_q_caRootLoaded(QSslCertificate cert, QSslCertificate trustedRoot)
{
    Q_Q(QSslSocket);
    if (!trustedRoot.isNull() && !trustedRoot.isBlacklisted()) {
        if (s_loadRootCertsOnDemand) {
            //Add the new root cert to default cert list for use by future sockets
            QSslSocket::addDefaultCaCertificate(trustedRoot);
        }
        //Add the new root cert to this socket for future connections
        q->addCaCertificate(trustedRoot);
        //Remove the broken chain ssl errors (as chain is verified by windows)
        for (int i=sslErrors.count() - 1; i >= 0; --i) {
            if (sslErrors.at(i).certificate() == cert) {
                switch (sslErrors.at(i).error()) {
                case QSslError::UnableToGetLocalIssuerCertificate:
                case QSslError::CertificateUntrusted:
                case QSslError::UnableToVerifyFirstCertificate:
                case QSslError::SelfSignedCertificateInChain:
                    // error can be ignored if OS says the chain is trusted
                    sslErrors.removeAt(i);
                    break;
                default:
                    // error cannot be ignored
                    break;
                }
            }
        }
    }
    // Continue with remaining errors
    if (plainSocket)
        plainSocket->resume();
    paused = false;
    if (checkSslErrors())
        continueHandshake();
}

class QWindowsCaRootFetcherThread : public QThread
{
public:
    QWindowsCaRootFetcherThread()
    {
        qRegisterMetaType<QSslCertificate>();
        setObjectName(QStringLiteral("QWindowsCaRootFetcher"));
        start();
    }
    ~QWindowsCaRootFetcherThread()
    {
        quit();
        wait(15500); // worst case, a running request can block for 15 seconds
    }
};

Q_GLOBAL_STATIC(QWindowsCaRootFetcherThread, windowsCaRootFetcherThread);

QWindowsCaRootFetcher::QWindowsCaRootFetcher(const QSslCertificate &certificate, QSslSocket::SslMode sslMode)
    : cert(certificate), mode(sslMode)
{
    moveToThread(windowsCaRootFetcherThread());
}

QWindowsCaRootFetcher::~QWindowsCaRootFetcher()
{
}

void QWindowsCaRootFetcher::start()
{
    QByteArray der = cert.toDer();
    PCCERT_CONTEXT wincert = CertCreateCertificateContext(X509_ASN_ENCODING, (const BYTE *)der.constData(), der.length());
    if (!wincert) {
#ifdef QSSLSOCKET_DEBUG
        qDebug("QWindowsCaRootFetcher failed to convert certificate to windows form");
#endif
        emit finished(cert, QSslCertificate());
        deleteLater();
        return;
    }

    CERT_CHAIN_PARA parameters;
    memset(&parameters, 0, sizeof(parameters));
    parameters.cbSize = sizeof(parameters);
    // set key usage constraint
    parameters.RequestedUsage.dwType = USAGE_MATCH_TYPE_AND;
    parameters.RequestedUsage.Usage.cUsageIdentifier = 1;
    LPSTR oid = (LPSTR)(mode == QSslSocket::SslClientMode ? szOID_PKIX_KP_SERVER_AUTH : szOID_PKIX_KP_CLIENT_AUTH);
    parameters.RequestedUsage.Usage.rgpszUsageIdentifier = &oid;

#ifdef QSSLSOCKET_DEBUG
    QElapsedTimer stopwatch;
    stopwatch.start();
#endif
    PCCERT_CHAIN_CONTEXT chain;
    BOOL result = CertGetCertificateChain(
        0, //default engine
        wincert,
        0, //current date/time
        0, //default store
        &parameters,
        0, //default dwFlags
        0, //reserved
        &chain);
#ifdef QSSLSOCKET_DEBUG
    qDebug() << "QWindowsCaRootFetcher" << stopwatch.elapsed() << "ms to get chain";
#endif

    QSslCertificate trustedRoot;
    if (result) {
#ifdef QSSLSOCKET_DEBUG
        qDebug() << "QWindowsCaRootFetcher - examining windows chains";
        if (chain->TrustStatus.dwErrorStatus == CERT_TRUST_NO_ERROR)
            qDebug() << " - TRUSTED";
        else
            qDebug() << " - NOT TRUSTED" << chain->TrustStatus.dwErrorStatus;
        if (chain->TrustStatus.dwInfoStatus & CERT_TRUST_IS_SELF_SIGNED)
            qDebug() << " - SELF SIGNED";
        qDebug() << "QSslSocketBackendPrivate::fetchCaRootForCert - dumping simple chains";
        for (unsigned int i = 0; i < chain->cChain; i++) {
            if (chain->rgpChain[i]->TrustStatus.dwErrorStatus == CERT_TRUST_NO_ERROR)
                qDebug() << " - TRUSTED SIMPLE CHAIN" << i;
            else
                qDebug() << " - UNTRUSTED SIMPLE CHAIN" << i << "reason:" << chain->rgpChain[i]->TrustStatus.dwErrorStatus;
            for (unsigned int j = 0; j < chain->rgpChain[i]->cElement; j++) {
                QSslCertificate foundCert(QByteArray((const char *)chain->rgpChain[i]->rgpElement[j]->pCertContext->pbCertEncoded
                    , chain->rgpChain[i]->rgpElement[j]->pCertContext->cbCertEncoded), QSsl::Der);
                qDebug() << "   - " << foundCert;
            }
        }
        qDebug() << " - and" << chain->cLowerQualityChainContext << "low quality chains"; //expect 0, we haven't asked for them
#endif

        //based on http://msdn.microsoft.com/en-us/library/windows/desktop/aa377182%28v=vs.85%29.aspx
        //about the final chain rgpChain[cChain-1] which must begin with a trusted root to be valid
        if (chain->TrustStatus.dwErrorStatus == CERT_TRUST_NO_ERROR
            && chain->cChain > 0) {
            const PCERT_SIMPLE_CHAIN finalChain = chain->rgpChain[chain->cChain - 1];
            // http://msdn.microsoft.com/en-us/library/windows/desktop/aa377544%28v=vs.85%29.aspx
            // rgpElement[0] is the end certificate chain element. rgpElement[cElement-1] is the self-signed "root" certificate element.
            if (finalChain->TrustStatus.dwErrorStatus == CERT_TRUST_NO_ERROR
                && finalChain->cElement > 0) {
                    trustedRoot = QSslCertificate(QByteArray((const char *)finalChain->rgpElement[finalChain->cElement - 1]->pCertContext->pbCertEncoded
                        , finalChain->rgpElement[finalChain->cElement - 1]->pCertContext->cbCertEncoded), QSsl::Der);
            }
        }
        CertFreeCertificateChain(chain);
    }
    CertFreeCertificateContext(wincert);

    emit finished(cert, trustedRoot);
    deleteLater();
}
#endif

void QSslSocketBackendPrivate::disconnectFromHost()
{
    if (ssl) {
        if (!shutdown) {
            q_SSL_shutdown(ssl);
            shutdown = true;
            transmit();
        }
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
    if (!ssl)
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

void QSslSocketBackendPrivate::continueHandshake()
{
    Q_Q(QSslSocket);
    // if we have a max read buffer size, reset the plain socket's to match
    if (readBufferMaxSize)
        plainSocket->setReadBufferSize(readBufferMaxSize);

#if OPENSSL_VERSION_NUMBER >= 0x0090806fL && !defined(OPENSSL_NO_TLSEXT)
    if (q_SSL_ctrl((ssl), SSL_CTRL_GET_SESSION_REUSED, 0, NULL))
        configuration.peerSessionShared = true;
#endif

#ifdef QT_DECRYPT_SSL_TRAFFIC
    if (ssl->session && ssl->s3) {
        const char *mk = reinterpret_cast<const char *>(ssl->session->master_key);
        QByteArray masterKey(mk, ssl->session->master_key_length);
        const char *random = reinterpret_cast<const char *>(ssl->s3->client_random);
        QByteArray clientRandom(random, SSL3_RANDOM_SIZE);

        // different format, needed for e.g. older Wireshark versions:
//        const char *sid = reinterpret_cast<const char *>(ssl->session->session_id);
//        QByteArray sessionID(sid, ssl->session->session_id_length);
//        QByteArray debugLineRSA("RSA Session-ID:");
//        debugLineRSA.append(sessionID.toHex().toUpper());
//        debugLineRSA.append(" Master-Key:");
//        debugLineRSA.append(masterKey.toHex().toUpper());
//        debugLineRSA.append("\n");

        QByteArray debugLineClientRandom("CLIENT_RANDOM ");
        debugLineClientRandom.append(clientRandom.toHex().toUpper());
        debugLineClientRandom.append(" ");
        debugLineClientRandom.append(masterKey.toHex().toUpper());
        debugLineClientRandom.append("\n");

        QString sslKeyFile = QDir::tempPath() + QLatin1String("/qt-ssl-keys");
        QFile file(sslKeyFile);
        if (!file.open(QIODevice::Append))
            qWarning() << "could not open file" << sslKeyFile << "for appending";
        if (!file.write(debugLineClientRandom))
            qWarning() << "could not write to file" << sslKeyFile;
        file.close();
    } else {
        qWarning("could not decrypt SSL traffic");
    }
#endif

    // Cache this SSL session inside the QSslContext
    if (!(configuration.sslOptions & QSsl::SslOptionDisableSessionSharing)) {
        if (!sslContextPointer->cacheSession(ssl)) {
            sslContextPointer.clear(); // we could not cache the session
        } else {
            // Cache the session for permanent usage as well
            if (!(configuration.sslOptions & QSsl::SslOptionDisableSessionPersistence)) {
                if (!sslContextPointer->sessionASN1().isEmpty())
                    configuration.sslSession = sslContextPointer->sessionASN1();
                configuration.sslSessionTicketLifeTimeHint = sslContextPointer->sessionTicketLifeTimeHint();
            }
        }
    }

#if OPENSSL_VERSION_NUMBER >= 0x1000100fL && !defined(OPENSSL_NO_TLSEXT) && !defined(OPENSSL_NO_NEXTPROTONEG)
    const unsigned char *proto = 0;
    unsigned int proto_len = 0;
    q_SSL_get0_next_proto_negotiated(ssl, &proto, &proto_len);
    if (proto_len)
        configuration.nextNegotiatedProtocol = QByteArray(reinterpret_cast<const char *>(proto), proto_len);
    else
        configuration.nextNegotiatedProtocol.clear();
    configuration.nextProtocolNegotiationStatus = sslContextPointer->npnContext().status;
#endif // OPENSSL_VERSION_NUMBER >= 0x1000100fL ...

    connectionEncrypted = true;
    emit q->encrypted();
    if (autoStartHandshake && pendingClose) {
        pendingClose = false;
        q->disconnectFromHost();
    }
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

bool QSslSocketBackendPrivate::isMatchingHostname(const QSslCertificate &cert, const QString &peerName)
{
    QStringList commonNameList = cert.subjectInfo(QSslCertificate::CommonName);

    foreach (const QString &commonName, commonNameList) {
        if (isMatchingHostname(commonName.toLower(), peerName.toLower())) {
            return true;
        }
    }

    foreach (const QString &altName, cert.subjectAlternativeNames().values(QSsl::DnsEntry)) {
        if (isMatchingHostname(altName.toLower(), peerName.toLower())) {
            return true;
        }
    }

    return false;
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

QList<QSslError> QSslSocketBackendPrivate::verify(QList<QSslCertificate> certificateChain, const QString &hostName)
{
    QList<QSslError> errors;
    if (certificateChain.count() <= 0) {
        errors << QSslError(QSslError::UnspecifiedError);
        return errors;
    }

    // Setup the store with the default CA certificates
    X509_STORE *certStore = q_X509_STORE_new();
    if (!certStore) {
        qWarning() << "Unable to create certificate store";
        errors << QSslError(QSslError::UnspecifiedError);
        return errors;
    }

    if (s_loadRootCertsOnDemand) {
        setDefaultCaCertificates(defaultCaCertificates() + systemCaCertificates());
    }

    foreach (const QSslCertificate &caCertificate, QSslSocket::defaultCaCertificates()) {
        // From https://www.openssl.org/docs/ssl/SSL_CTX_load_verify_locations.html:
        //
        // If several CA certificates matching the name, key identifier, and
        // serial number condition are available, only the first one will be
        // examined. This may lead to unexpected results if the same CA
        // certificate is available with different expiration dates. If a
        // ``certificate expired'' verification error occurs, no other
        // certificate will be searched. Make sure to not have expired
        // certificates mixed with valid ones.
        //
        // See also: QSslContext::fromConfiguration()
        if (caCertificate.expiryDate() >= QDateTime::currentDateTime()) {
            q_X509_STORE_add_cert(certStore, reinterpret_cast<X509 *>(caCertificate.handle()));
        }
    }

    QMutexLocker sslErrorListMutexLocker(&_q_sslErrorList()->mutex);

    // Register a custom callback to get all verification errors.
    X509_STORE_set_verify_cb_func(certStore, q_X509Callback);

    // Build the chain of intermediate certificates
    STACK_OF(X509) *intermediates = 0;
    if (certificateChain.length() > 1) {
        intermediates = (STACK_OF(X509) *) q_sk_new_null();

        if (!intermediates) {
            q_X509_STORE_free(certStore);
            errors << QSslError(QSslError::UnspecifiedError);
            return errors;
        }

        bool first = true;
        foreach (const QSslCertificate &cert, certificateChain) {
            if (first) {
                first = false;
                continue;
            }
#if OPENSSL_VERSION_NUMBER >= 0x10000000L
            q_sk_push( (_STACK *)intermediates, reinterpret_cast<X509 *>(cert.handle()));
#else
            q_sk_push( (STACK *)intermediates, reinterpret_cast<char *>(cert.handle()));
#endif
        }
    }

    X509_STORE_CTX *storeContext = q_X509_STORE_CTX_new();
    if (!storeContext) {
        q_X509_STORE_free(certStore);
        errors << QSslError(QSslError::UnspecifiedError);
        return errors;
    }

    if (!q_X509_STORE_CTX_init(storeContext, certStore, reinterpret_cast<X509 *>(certificateChain[0].handle()), intermediates)) {
        q_X509_STORE_CTX_free(storeContext);
        q_X509_STORE_free(certStore);
        errors << QSslError(QSslError::UnspecifiedError);
        return errors;
    }

    // Now we can actually perform the verification of the chain we have built.
    // We ignore the result of this function since we process errors via the
    // callback.
    (void) q_X509_verify_cert(storeContext);

    q_X509_STORE_CTX_free(storeContext);
#if OPENSSL_VERSION_NUMBER >= 0x10000000L
    q_sk_free( (_STACK *) intermediates);
#else
    q_sk_free( (STACK *) intermediates);
#endif

    // Now process the errors
    const QList<QPair<int, int> > errorList = _q_sslErrorList()->errors;
    _q_sslErrorList()->errors.clear();

    sslErrorListMutexLocker.unlock();

    // Translate the errors
    if (QSslCertificatePrivate::isBlacklisted(certificateChain[0])) {
        QSslError error(QSslError::CertificateBlacklisted, certificateChain[0]);
        errors << error;
    }

    // Check the certificate name against the hostname if one was specified
    if ((!hostName.isEmpty()) && (!isMatchingHostname(certificateChain[0], hostName))) {
        // No matches in common names or alternate names.
        QSslError error(QSslError::HostNameMismatch, certificateChain[0]);
        errors << error;
    }

    // Translate errors from the error list into QSslErrors.
    for (int i = 0; i < errorList.size(); ++i) {
        const QPair<int, int> &errorAndDepth = errorList.at(i);
        int err = errorAndDepth.first;
        int depth = errorAndDepth.second;
        errors << _q_OpenSSL_to_QSslError(err, certificateChain.value(depth));
    }

    q_X509_STORE_free(certStore);

    return errors;
}

QT_END_NAMESPACE
