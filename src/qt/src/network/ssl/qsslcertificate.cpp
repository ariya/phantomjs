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


/*!
    \class QSslCertificate
    \brief The QSslCertificate class provides a convenient API for an X509 certificate.
    \since 4.3

    \reentrant
    \ingroup network
    \ingroup ssl
    \inmodule QtNetwork

    QSslCertificate stores an X509 certificate, and is commonly used
    to verify the identity and store information about the local host,
    a remotely connected peer, or a trusted third party Certificate
    Authority.

    There are many ways to construct a QSslCertificate. The most
    common way is to call QSslSocket::peerCertificate(), which returns
    a QSslCertificate object, or QSslSocket::peerCertificateChain(),
    which returns a list of them. You can also load certificates from
    a DER (binary) or PEM (Base64) encoded bundle, typically stored as
    one or more local files, or in a Qt Resource.

    You can call isNull() to check if your certificate is null. By
    default, QSslCertificate constructs a null certificate. To check
    if the certificate is valid, call isValid(). A null certificate is
    invalid, but an invalid certificate is not necessarily null. If
    you want to reset all contents in a certificate, call clear().

    After loading a certificate, you can find information about the
    certificate, its subject, and its issuer, by calling one of the
    many accessor functions, including version(), serialNumber(),
    issuerInfo() and subjectInfo(). You can call effectiveDate() and
    expiryDate() to check when the certificate starts being
    effective and when it expires.
    The publicKey() function returns the certificate
    subject's public key as a QSslKey. You can call issuerInfo() or
    subjectInfo() to get detailed information about the certificate
    issuer and its subject.

    Internally, QSslCertificate is stored as an X509 structure. You
    can access this handle by calling handle(), but the results are
    likely to not be portable.

    \sa QSslSocket, QSslKey, QSslCipher, QSslError
*/

/*!
    \enum QSslCertificate::SubjectInfo

    Describes keys that you can pass to QSslCertificate::issuerInfo() or
    QSslCertificate::subjectInfo() to get information about the certificate
    issuer or subject.

    \value Organization "O" The name of the organization.

    \value CommonName "CN" The common name; most often this is used to store
    the host name.

    \value LocalityName "L" The locality.

    \value OrganizationalUnitName "OU" The organizational unit name.

    \value CountryName "C" The country.

    \value StateOrProvinceName "ST" The state or province.
*/

#include "qsslsocket_openssl_symbols_p.h"
#include "qsslcertificate.h"
#include "qsslcertificate_p.h"
#include "qsslkey.h"
#include "qsslkey_p.h"

#include <QtCore/qatomic.h>
#include <QtCore/qdatetime.h>
#include <QtCore/qdebug.h>
#include <QtCore/qdir.h>
#include <QtCore/qdiriterator.h>
#include <QtCore/qfile.h>
#include <QtCore/qfileinfo.h>
#include <QtCore/qmap.h>
#include <QtCore/qmutex.h>
#include <QtCore/private/qmutexpool_p.h>
#include <QtCore/qstring.h>
#include <QtCore/qstringlist.h>

QT_BEGIN_NAMESPACE

// forward declaration
static QMap<QString, QString> _q_mapFromX509Name(X509_NAME *name);

/*!
    Constructs a QSslCertificate by reading \a format encoded data
    from \a device and using the first certificate found. You can
    later call isNull() to see if \a device contained a certificate,
    and if this certificate was loaded successfully.
*/
QSslCertificate::QSslCertificate(QIODevice *device, QSsl::EncodingFormat format)
    : d(new QSslCertificatePrivate)
{
    QSslSocketPrivate::ensureInitialized();
    if (device)
        d->init(device->readAll(), format);
}

/*!
    Constructs a QSslCertificate by parsing the \a format encoded
    \a data and using the first available certificate found. You can
    later call isNull() to see if \a data contained a certificate,
    and if this certificate was loaded successfully.
*/
QSslCertificate::QSslCertificate(const QByteArray &data, QSsl::EncodingFormat format)
    : d(new QSslCertificatePrivate)
{
    QSslSocketPrivate::ensureInitialized();
    d->init(data, format);
}

/*!
    Constructs an identical copy of \a other.
*/
QSslCertificate::QSslCertificate(const QSslCertificate &other) : d(other.d)
{
}

/*!
    Destroys the QSslCertificate.
*/
QSslCertificate::~QSslCertificate()
{
}

/*!
    Copies the contents of \a other into this certificate, making the two
    certificates identical.
*/
QSslCertificate &QSslCertificate::operator=(const QSslCertificate &other)
{
    d = other.d;
    return *this;
}

/*!
    Returns true if this certificate is the same as \a other; otherwise
    returns false.
*/
bool QSslCertificate::operator==(const QSslCertificate &other) const
{
    if (d == other.d)
        return true;
    if (d->null && other.d->null)
        return true;
    if (d->x509 && other.d->x509)
        return q_X509_cmp(d->x509, other.d->x509) == 0;
    return false;
}

/*!
    \fn bool QSslCertificate::operator!=(const QSslCertificate &other) const

    Returns true if this certificate is not the same as \a other; otherwise
    returns false.
*/

/*!
    Returns true if this is a null certificate (i.e., a certificate
    with no contents); otherwise returns false.

    By default, QSslCertificate constructs a null certificate.

    \sa isValid(), clear()
*/
bool QSslCertificate::isNull() const
{
    return d->null;
}

/*!
    Returns true if this certificate is valid; otherwise returns
    false.

    Note: Currently, this function checks that the current
    data-time is within the date-time range during which the
    certificate is considered valid, and checks that the
    certificate is not in a blacklist of fraudulent certificates.

    \sa isNull()
*/
bool QSslCertificate::isValid() const
{
    const QDateTime currentTime = QDateTime::currentDateTime();
    return currentTime >= d->notValidBefore &&
            currentTime <= d->notValidAfter &&
            ! QSslCertificatePrivate::isBlacklisted(*this);
}

/*!
    Clears the contents of this certificate, making it a null
    certificate.

    \sa isNull()
*/
void QSslCertificate::clear()
{
    if (isNull())
        return;
    d = new QSslCertificatePrivate;
}

/*!
    Returns the certificate's version string.
*/
QByteArray QSslCertificate::version() const
{
    QMutexLocker lock(QMutexPool::globalInstanceGet(d.data()));
    if (d->versionString.isEmpty() && d->x509)
        d->versionString =
            QByteArray::number(qlonglong(q_ASN1_INTEGER_get(d->x509->cert_info->version)) + 1);

    return d->versionString;
}

/*!
    Returns the certificate's serial number string in decimal format.
    In case the serial number cannot be converted to decimal format
    (i.e. if it is bigger than 4294967295, which means it does not fit into 4 bytes),
    its hexadecimal version is returned.
*/
QByteArray QSslCertificate::serialNumber() const
{
    QMutexLocker lock(QMutexPool::globalInstanceGet(d.data()));
    if (d->serialNumberString.isEmpty() && d->x509) {
        ASN1_INTEGER *serialNumber = d->x509->cert_info->serialNumber;
        // if we cannot convert to a long, just output the hexadecimal number
        if (serialNumber->length > 4) {
            QByteArray hexString;
            hexString.reserve(serialNumber->length * 3);
            for (int a = 0; a < serialNumber->length; ++a) {
                hexString += QByteArray::number(serialNumber->data[a], 16).rightJustified(2, '0');
                hexString += ':';
            }
            hexString.chop(1);
            d->serialNumberString = hexString;
        } else {
            d->serialNumberString = QByteArray::number(qlonglong(q_ASN1_INTEGER_get(serialNumber)));
        }
    }
    return d->serialNumberString;
}

/*!
    Returns a cryptographic digest of this certificate. By default,
    an MD5 digest will be generated, but you can also specify a
    custom \a algorithm.
*/
QByteArray QSslCertificate::digest(QCryptographicHash::Algorithm algorithm) const
{
    return QCryptographicHash::hash(toDer(), algorithm);
}

static QString _q_SubjectInfoToString(QSslCertificate::SubjectInfo info)
{
    QString str;
    switch (info) {
    case QSslCertificate::Organization: str = QLatin1String("O"); break;
    case QSslCertificate::CommonName: str = QLatin1String("CN"); break;
    case QSslCertificate::LocalityName: str = QLatin1String("L"); break;
    case QSslCertificate::OrganizationalUnitName: str = QLatin1String("OU"); break;
    case QSslCertificate::CountryName: str = QLatin1String("C"); break;
    case QSslCertificate::StateOrProvinceName: str = QLatin1String("ST"); break;
    }
    return str;
}

/*!
  \fn QString QSslCertificate::issuerInfo(SubjectInfo subject) const
  
  Returns the issuer information for the \a subject from the
  certificate, or an empty string if there is no information for
  \a subject in the certificate.

  \sa subjectInfo()
*/
QString QSslCertificate::issuerInfo(SubjectInfo info) const
{
    QMutexLocker lock(QMutexPool::globalInstanceGet(d.data()));
    // lazy init
    if (d->issuerInfo.isEmpty() && d->x509)
        d->issuerInfo =
                _q_mapFromX509Name(q_X509_get_issuer_name(d->x509));

    return d->issuerInfo.value(_q_SubjectInfoToString(info));
}

/*!
  Returns the issuer information for \a tag from the certificate,
  or an empty string if there is no information for \a tag in the
  certificate.

  \sa subjectInfo()
*/
QString QSslCertificate::issuerInfo(const QByteArray &tag) const
{
    QMutexLocker lock(QMutexPool::globalInstanceGet(d.data()));
    // lazy init
    if (d->issuerInfo.isEmpty() && d->x509)
        d->issuerInfo =
                _q_mapFromX509Name(q_X509_get_issuer_name(d->x509));

    return d->issuerInfo.value(QString::fromLatin1(tag));
}

/*!

  \fn QString QSslCertificate::subjectInfo(SubjectInfo subject) const

  Returns the information for the \a subject, or an empty string if
  there is no information for \a subject in the certificate.

    \sa issuerInfo()
*/
QString QSslCertificate::subjectInfo(SubjectInfo info) const
{
    QMutexLocker lock(QMutexPool::globalInstanceGet(d.data()));
    // lazy init
    if (d->subjectInfo.isEmpty() && d->x509)
        d->subjectInfo =
                _q_mapFromX509Name(q_X509_get_subject_name(d->x509));

    return d->subjectInfo.value(_q_SubjectInfoToString(info));
}

/*!
    Returns the subject information for \a tag, or an empty string if
    there is no information for \a tag in the certificate.

    \sa issuerInfo()
*/
QString QSslCertificate::subjectInfo(const QByteArray &tag) const
{
    QMutexLocker lock(QMutexPool::globalInstanceGet(d.data()));
    // lazy init
    if (d->subjectInfo.isEmpty() && d->x509)
        d->subjectInfo =
                _q_mapFromX509Name(q_X509_get_subject_name(d->x509));

    return d->subjectInfo.value(QString::fromLatin1(tag));
}

/*!
  Returns the list of alternative subject names for this
  certificate. The alternate subject names typically contain host
  names, optionally with wildcards, that are valid for this
  certificate.
  
  These names are tested against the connected peer's host name, if
  either the subject information for \l CommonName doesn't define a
  valid host name, or the subject info name doesn't match the peer's
  host name.
  
  \sa subjectInfo()
*/
QMultiMap<QSsl::AlternateNameEntryType, QString> QSslCertificate::alternateSubjectNames() const
{
    QMultiMap<QSsl::AlternateNameEntryType, QString> result;

    if (!d->x509)
        return result;

    STACK_OF(GENERAL_NAME) *altNames = (STACK_OF(GENERAL_NAME)*)q_X509_get_ext_d2i(d->x509, NID_subject_alt_name, 0, 0);

    if (altNames) {
        for (int i = 0; i < q_sk_GENERAL_NAME_num(altNames); ++i) {
            const GENERAL_NAME *genName = q_sk_GENERAL_NAME_value(altNames, i);
            if (genName->type != GEN_DNS && genName->type != GEN_EMAIL)
                continue;

            int len = q_ASN1_STRING_length(genName->d.ia5);
            if (len < 0 || len >= 8192) {
                // broken name
                continue;
            }

            const char *altNameStr = reinterpret_cast<const char *>(q_ASN1_STRING_data(genName->d.ia5));
            const QString altName = QString::fromLatin1(altNameStr, len);
            if (genName->type == GEN_DNS)
                result.insert(QSsl::DnsEntry, altName);
            else if (genName->type == GEN_EMAIL)
                result.insert(QSsl::EmailEntry, altName);
        }
        q_sk_pop_free((STACK*)altNames, reinterpret_cast<void(*)(void*)>(q_sk_free));
    }

    return result;
}

/*!
  Returns the date-time that the certificate becomes valid, or an
  empty QDateTime if this is a null certificate.

  \sa expiryDate()
*/
QDateTime QSslCertificate::effectiveDate() const
{
    return d->notValidBefore;
}

/*!
  Returns the date-time that the certificate expires, or an empty
  QDateTime if this is a null certificate.

    \sa effectiveDate()
*/
QDateTime QSslCertificate::expiryDate() const
{
    return d->notValidAfter;
}

/*!
    Returns a pointer to the native certificate handle, if there is
    one, or a null pointer otherwise.

    You can use this handle, together with the native API, to access
    extended information about the certificate.

    \warning Use of this function has a high probability of being
    non-portable, and its return value may vary from platform to
    platform or change from minor release to minor release.
*/
Qt::HANDLE QSslCertificate::handle() const
{
    return Qt::HANDLE(d->x509);
}

/*!
    Returns the certificate subject's public key.
*/
QSslKey QSslCertificate::publicKey() const
{
    if (!d->x509)
        return QSslKey();

    QSslKey key;

    key.d->type = QSsl::PublicKey;
    X509_PUBKEY *xkey = d->x509->cert_info->key;
    EVP_PKEY *pkey = q_X509_PUBKEY_get(xkey);
    Q_ASSERT(pkey);

    if (q_EVP_PKEY_type(pkey->type) == EVP_PKEY_RSA) {
        key.d->rsa = q_EVP_PKEY_get1_RSA(pkey);
        key.d->algorithm = QSsl::Rsa;
        key.d->isNull = false;
    } else if (q_EVP_PKEY_type(pkey->type) == EVP_PKEY_DSA) {
        key.d->dsa = q_EVP_PKEY_get1_DSA(pkey);
        key.d->algorithm = QSsl::Dsa;
        key.d->isNull = false;
    } else if (q_EVP_PKEY_type(pkey->type) == EVP_PKEY_DH) {
        // DH unsupported
    } else {
        // error?
    }

    q_EVP_PKEY_free(pkey);
    return key;
}

/*!
    Returns this certificate converted to a PEM (Base64) encoded
    representation.
*/
QByteArray QSslCertificate::toPem() const
{
    if (!d->x509)
        return QByteArray();
    return d->QByteArray_from_X509(d->x509, QSsl::Pem);
}

/*!
    Returns this certificate converted to a DER (binary) encoded
    representation.
*/
QByteArray QSslCertificate::toDer() const
{
    if (!d->x509)
        return QByteArray();
    return d->QByteArray_from_X509(d->x509, QSsl::Der);
}

/*!
    Searches all files in the \a path for certificates encoded in the
    specified \a format and returns them in a list. \e must be a file or a
    pattern matching one or more files, as specified by \a syntax.

    Example:
    
    \snippet doc/src/snippets/code/src_network_ssl_qsslcertificate.cpp 0

    \sa fromData()
*/
QList<QSslCertificate> QSslCertificate::fromPath(const QString &path,
                                                 QSsl::EncodingFormat format,
                                                 QRegExp::PatternSyntax syntax)
{
    // $, (,), *, +, ., ?, [, ,], ^, {, | and }.
    int pos = -1;
    if (syntax == QRegExp::Wildcard)
        pos = path.indexOf(QRegExp(QLatin1String("[^\\][\\*\\?\\[\\]]")));
    else if (syntax != QRegExp::FixedString)
        pos = path.indexOf(QRegExp(QLatin1String("[^\\][\\$\\(\\)\\*\\+\\.\\?\\[\\]\\^\\{\\}\\|]")));
    QString pathPrefix = path.left(pos); // == path if pos < 0
    if (pos != -1)
        pathPrefix = pathPrefix.left(pathPrefix.lastIndexOf(QLatin1Char('/')));

    // Special case - if the prefix ends up being nothing, use "." instead and
    // chop off the first two characters from the glob'ed paths.
    int startIndex = 0;
    if (pathPrefix.trimmed().isEmpty()) {
        if(path.startsWith(QLatin1Char('/'))) {
            pathPrefix = path.left(path.indexOf(QRegExp(QLatin1String("[\\*\\?\\[]"))));
            pathPrefix = path.left(path.lastIndexOf(QLatin1Char('/')));
        } else {
            startIndex = 2;
            pathPrefix = QLatin1String(".");
        }
    }

    // The path is a file.
    if (pos == -1 && QFileInfo(pathPrefix).isFile()) {
        QFile file(pathPrefix);
        if (file.open(QIODevice::ReadOnly | QIODevice::Text))
            return QSslCertificate::fromData(file.readAll(),format);
        return QList<QSslCertificate>();
    }

    // The path can be a file or directory.
    QList<QSslCertificate> certs;
    QRegExp pattern(path, Qt::CaseSensitive, syntax);
    QDirIterator it(pathPrefix, QDir::Files, QDirIterator::FollowSymlinks | QDirIterator::Subdirectories);
    while (it.hasNext()) {
        QString filePath = startIndex == 0 ? it.next() : it.next().mid(startIndex);
        if (!pattern.exactMatch(filePath))
            continue;

        QFile file(filePath);
        if (file.open(QIODevice::ReadOnly | QIODevice::Text))
            certs += QSslCertificate::fromData(file.readAll(),format);
    }
    return certs;
}

/*!
    Searches for and parses all certificates in \a device that are
    encoded in the specified \a format and returns them in a list of
    certificates.

    \sa fromData()
*/
QList<QSslCertificate> QSslCertificate::fromDevice(QIODevice *device, QSsl::EncodingFormat format)
{
    if (!device) {
        qWarning("QSslCertificate::fromDevice: cannot read from a null device");
        return QList<QSslCertificate>();
    }
    return fromData(device->readAll(), format);
}

/*!
    Searches for and parses all certificates in \a data that are
    encoded in the specified \a format and returns them in a list of
    certificates.

    \sa fromDevice()
*/
QList<QSslCertificate> QSslCertificate::fromData(const QByteArray &data, QSsl::EncodingFormat format)
{
    return (format == QSsl::Pem)
        ? QSslCertificatePrivate::certificatesFromPem(data)
        : QSslCertificatePrivate::certificatesFromDer(data);
}

void QSslCertificatePrivate::init(const QByteArray &data, QSsl::EncodingFormat format)
{
    if (!data.isEmpty()) {
        QList<QSslCertificate> certs = (format == QSsl::Pem)
            ? certificatesFromPem(data, 1)
            : certificatesFromDer(data, 1);
        if (!certs.isEmpty()) {
            *this = *certs.first().d;
            if (x509)
                x509 = q_X509_dup(x509);
        }
    }
}

#define BEGINCERTSTRING "-----BEGIN CERTIFICATE-----"
#define ENDCERTSTRING "-----END CERTIFICATE-----"

// ### refactor against QSsl::pemFromDer() etc. (to avoid redundant implementations)
QByteArray QSslCertificatePrivate::QByteArray_from_X509(X509 *x509, QSsl::EncodingFormat format)
{
    if (!x509) {
        qWarning("QSslSocketBackendPrivate::X509_to_QByteArray: null X509");
        return QByteArray();
    }

    // Use i2d_X509 to convert the X509 to an array.
    int length = q_i2d_X509(x509, 0);
    QByteArray array;
    array.resize(length);
    char *data = array.data();
    char **dataP = &data;
    unsigned char **dataPu = (unsigned char **)dataP;
    if (q_i2d_X509(x509, dataPu) < 0)
        return QByteArray();

    if (format == QSsl::Der)
        return array;

    // Convert to Base64 - wrap at 64 characters.
    array = array.toBase64();
    QByteArray tmp;
    for (int i = 0; i <= array.size() - 64; i += 64) {
        tmp += QByteArray::fromRawData(array.data() + i, 64);
        tmp += '\n';
    }
    if (int remainder = array.size() % 64) {
        tmp += QByteArray::fromRawData(array.data() + array.size() - remainder, remainder);
        tmp += '\n';
    }

    return BEGINCERTSTRING "\n" + tmp + ENDCERTSTRING "\n";
}

static QMap<QString, QString> _q_mapFromX509Name(X509_NAME *name)
{
    QMap<QString, QString> info;
    for (int i = 0; i < q_X509_NAME_entry_count(name); ++i) {
        X509_NAME_ENTRY *e = q_X509_NAME_get_entry(name, i);
        const char *obj = q_OBJ_nid2sn(q_OBJ_obj2nid(q_X509_NAME_ENTRY_get_object(e)));
        unsigned char *data = 0;
        int size = q_ASN1_STRING_to_UTF8(&data, q_X509_NAME_ENTRY_get_data(e));
        info[QString::fromUtf8(obj)] = QString::fromUtf8((char*)data, size);
        q_CRYPTO_free(data);
    }
    return info;
}

QSslCertificate QSslCertificatePrivate::QSslCertificate_from_X509(X509 *x509)
{
    QSslCertificate certificate;
    if (!x509 || !QSslSocket::supportsSsl())
        return certificate;

    ASN1_TIME *nbef = q_X509_get_notBefore(x509);
    ASN1_TIME *naft = q_X509_get_notAfter(x509);
    certificate.d->notValidBefore = q_getTimeFromASN1(nbef);
    certificate.d->notValidAfter = q_getTimeFromASN1(naft);
    certificate.d->null = false;
    certificate.d->x509 = q_X509_dup(x509);

    return certificate;
}

static bool matchLineFeed(const QByteArray &pem, int *offset)
{
    char ch = 0;

    // ignore extra whitespace at the end of the line
    while (*offset < pem.size() && (ch = pem.at(*offset)) == ' ')
        ++*offset;

    if (ch == '\n') {
        *offset += 1;
        return true;
    }
    if (ch == '\r' && pem.size() > (*offset + 1) && pem.at(*offset + 1) == '\n') {
        *offset += 2;
        return true;
    }
    return false;
}

QList<QSslCertificate> QSslCertificatePrivate::certificatesFromPem(const QByteArray &pem, int count)
{
    QList<QSslCertificate> certificates;
    QSslSocketPrivate::ensureInitialized();

    int offset = 0;
    while (count == -1 || certificates.size() < count) {
        int startPos = pem.indexOf(BEGINCERTSTRING, offset);
        if (startPos == -1)
            break;
        startPos += sizeof(BEGINCERTSTRING) - 1;
        if (!matchLineFeed(pem, &startPos))
            break;

        int endPos = pem.indexOf(ENDCERTSTRING, startPos);
        if (endPos == -1)
            break;

        offset = endPos + sizeof(ENDCERTSTRING) - 1;
        if (offset < pem.size() && !matchLineFeed(pem, &offset))
            break;

        QByteArray decoded = QByteArray::fromBase64(
            QByteArray::fromRawData(pem.data() + startPos, endPos - startPos));
#if OPENSSL_VERSION_NUMBER >= 0x00908000L
        const unsigned char *data = (const unsigned char *)decoded.data();
#else
        unsigned char *data = (unsigned char *)decoded.data();
#endif

        if (X509 *x509 = q_d2i_X509(0, &data, decoded.size())) {
            certificates << QSslCertificate_from_X509(x509);
            q_X509_free(x509);
        }
    }

    return certificates;
}

QList<QSslCertificate> QSslCertificatePrivate::certificatesFromDer(const QByteArray &der, int count)
{
    QList<QSslCertificate> certificates;
    QSslSocketPrivate::ensureInitialized();


#if OPENSSL_VERSION_NUMBER >= 0x00908000L
        const unsigned char *data = (const unsigned char *)der.data();
#else
        unsigned char *data = (unsigned char *)der.data();
#endif
    int size = der.size();

    while (count == -1 || certificates.size() < count) {
        if (X509 *x509 = q_d2i_X509(0, &data, size)) {
            certificates << QSslCertificate_from_X509(x509);
            q_X509_free(x509);
        } else {
            break;
        }
        size -= ((char *)data - der.data());
    }

    return certificates;
}

// These certificates are known to be fraudulent and were created during the comodo
// compromise. See http://www.comodo.com/Comodo-Fraud-Incident-2011-03-23.html
static const char *certificate_blacklist[] = {
    "04:7e:cb:e9:fc:a5:5f:7b:d0:9e:ae:36:e1:0c:ae:1e", "mail.google.com", // Comodo
    "f5:c8:6a:f3:61:62:f1:3a:64:f5:4f:6d:c9:58:7c:06", "www.google.com", // Comodo
    "d7:55:8f:da:f5:f1:10:5b:b2:13:28:2b:70:77:29:a3", "login.yahoo.com", // Comodo
    "39:2a:43:4f:0e:07:df:1f:8a:a3:05:de:34:e0:c2:29", "login.yahoo.com", // Comodo
    "3e:75:ce:d4:6b:69:30:21:21:88:30:ae:86:a8:2a:71", "login.yahoo.com", // Comodo
    "e9:02:8b:95:78:e4:15:dc:1a:71:0a:2b:88:15:44:47", "login.skype.com", // Comodo
    "92:39:d5:34:8f:40:d1:69:5a:74:54:70:e1:f2:3f:43", "addons.mozilla.org", // Comodo
    "b0:b7:13:3e:d0:96:f9:b5:6f:ae:91:c8:74:bd:3a:c0", "login.live.com", // Comodo
    "d8:f3:5f:4e:b7:87:2b:2d:ab:06:92:e3:15:38:2f:b0", "global trustee", // Comodo

    "05:e2:e6:a4:cd:09:ea:54:d6:65:b0:75:fe:22:a2:56", "*.google.com", // leaf certificate issued by DigiNotar
    "0c:76:da:9c:91:0c:4e:2c:9e:fe:15:d0:58:93:3c:4c", "DigiNotar Root CA", // DigiNotar root
    "f1:4a:13:f4:87:2b:56:dc:39:df:84:ca:7a:a1:06:49", "DigiNotar Services CA", // DigiNotar intermediate signed by DigiNotar Root
    "36:16:71:55:43:42:1b:9d:e6:cb:a3:64:41:df:24:38", "DigiNotar Services 1024 CA", // DigiNotar intermediate signed by DigiNotar Root
    "0a:82:bd:1e:14:4e:88:14:d7:5b:1a:55:27:be:bf:3e", "DigiNotar Root CA G2", // other DigiNotar Root CA
    "a4:b6:ce:e3:2e:d3:35:46:26:3c:b3:55:3a:a8:92:21", "CertiID Enterprise Certificate Authority", // DigiNotar intermediate signed by "DigiNotar Root CA G2"
    "5b:d5:60:9c:64:17:68:cf:21:0e:35:fd:fb:05:ad:41", "DigiNotar Qualified CA", // DigiNotar intermediate signed by DigiNotar Root

    "1184640176",                                      "DigiNotar Services 1024 CA", // DigiNotar intermediate cross-signed by Entrust
    "120000525",                                       "DigiNotar Cyber CA", // DigiNotar intermediate cross-signed by CyberTrust
    "120000505",                                       "DigiNotar Cyber CA", // DigiNotar intermediate cross-signed by CyberTrust
    "120000515",                                       "DigiNotar Cyber CA", // DigiNotar intermediate cross-signed by CyberTrust
    "20015536",                                        "DigiNotar PKIoverheid CA Overheid en Bedrijven", // DigiNotar intermediate cross-signed by the Dutch government
    "20001983",                                        "DigiNotar PKIoverheid CA Organisatie - G2", // DigiNotar intermediate cross-signed by the Dutch government
    "d6:d0:29:77:f1:49:fd:1a:83:f2:b9:ea:94:8c:5c:b4", "DigiNotar Extended Validation CA", // DigiNotar intermediate signed by DigiNotar EV Root
    "1e:7d:7a:53:3d:45:30:41:96:40:0f:71:48:1f:45:04", "DigiNotar Public CA 2025", // DigiNotar intermediate
//    "(has not been seen in the wild so far)", "DigiNotar Public CA - G2", // DigiNotar intermediate
//    "(has not been seen in the wild so far)", "Koninklijke Notariele Beroepsorganisatie CA", // compromised during DigiNotar breach
//    "(has not been seen in the wild so far)", "Stichting TTP Infos CA," // compromised during DigiNotar breach
    "1184640175", "DigiNotar Root CA", // DigiNotar intermediate cross-signed by Entrust
    "1184644297", "DigiNotar Root CA", // DigiNotar intermediate cross-signed by Entrust

    "120001705", "Digisign Server ID (Enrich)", // (Malaysian) Digicert Sdn. Bhd. cross-signed by Verizon CyberTrust
    "1276011370", "Digisign Server ID - (Enrich)", // (Malaysian) Digicert Sdn. Bhd. cross-signed by Entrust
    0
};

bool QSslCertificatePrivate::isBlacklisted(const QSslCertificate &certificate)
{
    for (int a = 0; certificate_blacklist[a] != 0; a++) {
        QString blacklistedCommonName = QString::fromUtf8(certificate_blacklist[(a+1)]);
        if (certificate.serialNumber() == certificate_blacklist[a++] &&
            (certificate.subjectInfo(QSslCertificate::CommonName) == blacklistedCommonName ||
             certificate.issuerInfo(QSslCertificate::CommonName) == blacklistedCommonName))
            return true;
    }
    return false;
}

#ifndef QT_NO_DEBUG_STREAM
QDebug operator<<(QDebug debug, const QSslCertificate &certificate)
{
    debug << "QSslCertificate("
          << certificate.version()
          << ',' << certificate.serialNumber()
          << ',' << certificate.digest().toBase64()
          << ',' << certificate.issuerInfo(QSslCertificate::Organization)
          << ',' << certificate.subjectInfo(QSslCertificate::Organization)
          << ',' << certificate.alternateSubjectNames()
#ifndef QT_NO_TEXTSTREAM
          << ',' << certificate.effectiveDate()
          << ',' << certificate.expiryDate()
#endif
          << ')';
    return debug;
}
QDebug operator<<(QDebug debug, QSslCertificate::SubjectInfo info)
{
    switch (info) {
    case QSslCertificate::Organization: debug << "Organization"; break;
    case QSslCertificate::CommonName: debug << "CommonName"; break;
    case QSslCertificate::CountryName: debug << "CountryName"; break;
    case QSslCertificate::LocalityName: debug << "LocalityName"; break;
    case QSslCertificate::OrganizationalUnitName: debug << "OrganizationalUnitName"; break;
    case QSslCertificate::StateOrProvinceName: debug << "StateOrProvinceName"; break;
    }
    return debug;
}
#endif

QT_END_NAMESPACE
