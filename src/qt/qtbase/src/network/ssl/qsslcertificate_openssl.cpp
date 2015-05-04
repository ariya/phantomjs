/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
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

#include "qssl_p.h"
#include "qsslsocket_openssl_symbols_p.h"
#include "qsslcertificate_p.h"
#include "qsslkey_p.h"
#include "qsslcertificateextension_p.h"

#include <QtCore/private/qmutexpool_p.h>

QT_BEGIN_NAMESPACE

// forward declaration
static QMap<QByteArray, QString> _q_mapFromX509Name(X509_NAME *name);

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

uint qHash(const QSslCertificate &key, uint seed) Q_DECL_NOTHROW
{
    if (X509 * const x509 = key.d->x509) {
        (void)q_X509_cmp(x509, x509); // populate x509->sha1_hash
                                      // (if someone knows a better way...)
        return qHashBits(x509->sha1_hash, SHA_DIGEST_LENGTH, seed);
    } else {
        return seed;
    }
}

bool QSslCertificate::isNull() const
{
    return d->null;
}

bool QSslCertificate::isSelfSigned() const
{
    if (!d->x509)
        return false;

    return (q_X509_check_issued(d->x509, d->x509) == X509_V_OK);
}

QByteArray QSslCertificate::version() const
{
    QMutexLocker lock(QMutexPool::globalInstanceGet(d.data()));
    if (d->versionString.isEmpty() && d->x509)
        d->versionString =
            QByteArray::number(qlonglong(q_ASN1_INTEGER_get(d->x509->cert_info->version)) + 1);

    return d->versionString;
}

QByteArray QSslCertificate::serialNumber() const
{
    QMutexLocker lock(QMutexPool::globalInstanceGet(d.data()));
    if (d->serialNumberString.isEmpty() && d->x509) {
        ASN1_INTEGER *serialNumber = d->x509->cert_info->serialNumber;
        QByteArray hexString;
        hexString.reserve(serialNumber->length * 3);
        for (int a = 0; a < serialNumber->length; ++a) {
            hexString += QByteArray::number(serialNumber->data[a], 16).rightJustified(2, '0');
            hexString += ':';
        }
        hexString.chop(1);
        d->serialNumberString = hexString;
    }
    return d->serialNumberString;
}

QStringList QSslCertificate::issuerInfo(SubjectInfo info) const
{
    QMutexLocker lock(QMutexPool::globalInstanceGet(d.data()));
    // lazy init
    if (d->issuerInfo.isEmpty() && d->x509)
        d->issuerInfo =
                _q_mapFromX509Name(q_X509_get_issuer_name(d->x509));

    return d->issuerInfo.values(d->subjectInfoToString(info));
}

QStringList QSslCertificate::issuerInfo(const QByteArray &attribute) const
{
    QMutexLocker lock(QMutexPool::globalInstanceGet(d.data()));
    // lazy init
    if (d->issuerInfo.isEmpty() && d->x509)
        d->issuerInfo =
                _q_mapFromX509Name(q_X509_get_issuer_name(d->x509));

    return d->issuerInfo.values(attribute);
}

QStringList QSslCertificate::subjectInfo(SubjectInfo info) const
{
    QMutexLocker lock(QMutexPool::globalInstanceGet(d.data()));
    // lazy init
    if (d->subjectInfo.isEmpty() && d->x509)
        d->subjectInfo =
                _q_mapFromX509Name(q_X509_get_subject_name(d->x509));

    return d->subjectInfo.values(d->subjectInfoToString(info));
}

QStringList QSslCertificate::subjectInfo(const QByteArray &attribute) const
{
    QMutexLocker lock(QMutexPool::globalInstanceGet(d.data()));
    // lazy init
    if (d->subjectInfo.isEmpty() && d->x509)
        d->subjectInfo =
                _q_mapFromX509Name(q_X509_get_subject_name(d->x509));

    return d->subjectInfo.values(attribute);
}

QList<QByteArray> QSslCertificate::subjectInfoAttributes() const
{
    QMutexLocker lock(QMutexPool::globalInstanceGet(d.data()));
    // lazy init
    if (d->subjectInfo.isEmpty() && d->x509)
        d->subjectInfo =
                _q_mapFromX509Name(q_X509_get_subject_name(d->x509));

    return d->subjectInfo.uniqueKeys();
}

QList<QByteArray> QSslCertificate::issuerInfoAttributes() const
{
    QMutexLocker lock(QMutexPool::globalInstanceGet(d.data()));
    // lazy init
    if (d->issuerInfo.isEmpty() && d->x509)
        d->issuerInfo =
                _q_mapFromX509Name(q_X509_get_issuer_name(d->x509));

    return d->issuerInfo.uniqueKeys();
}

QMultiMap<QSsl::AlternativeNameEntryType, QString> QSslCertificate::subjectAlternativeNames() const
{
    QMultiMap<QSsl::AlternativeNameEntryType, QString> result;

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

QDateTime QSslCertificate::effectiveDate() const
{
    return d->notValidBefore;
}

QDateTime QSslCertificate::expiryDate() const
{
    return d->notValidAfter;
}

Qt::HANDLE QSslCertificate::handle() const
{
    return Qt::HANDLE(d->x509);
}

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

/*
 * Convert unknown extensions to a QVariant.
 */
static QVariant x509UnknownExtensionToValue(X509_EXTENSION *ext)
{
    // Get the extension specific method object if available
    // we cast away the const-ness here because some versions of openssl
    // don't use const for the parameters in the functions pointers stored
    // in the object.
    X509V3_EXT_METHOD *meth = const_cast<X509V3_EXT_METHOD *>(q_X509V3_EXT_get(ext));
    if (!meth) {
        ASN1_OCTET_STRING *value = q_X509_EXTENSION_get_data(ext);
        QByteArray result( reinterpret_cast<const char *>(q_ASN1_STRING_data(value)),
                           q_ASN1_STRING_length(value));
        return result;
    }

    //const unsigned char *data = ext->value->data;
    void *ext_internal = q_X509V3_EXT_d2i(ext);

    // If this extension can be converted
    if (meth->i2v && ext_internal) {
        STACK_OF(CONF_VALUE) *val = meth->i2v(meth, ext_internal, 0);

        QVariantMap map;
        QVariantList list;
        bool isMap = false;

        for (int j = 0; j < q_SKM_sk_num(CONF_VALUE, val); j++) {
            CONF_VALUE *nval = q_SKM_sk_value(CONF_VALUE, val, j);
            if (nval->name && nval->value) {
                isMap = true;
                map[QString::fromUtf8(nval->name)] = QString::fromUtf8(nval->value);
            } else if (nval->name) {
                list << QString::fromUtf8(nval->name);
            } else if (nval->value) {
                list << QString::fromUtf8(nval->value);
            }
        }

        if (isMap)
            return map;
        else
            return list;
    } else if (meth->i2s && ext_internal) {
        //qCDebug(lcSsl) << meth->i2s(meth, ext_internal);
        QVariant result(QString::fromUtf8(meth->i2s(meth, ext_internal)));
        return result;
    } else if (meth->i2r && ext_internal) {
        QByteArray result;

        BIO *bio = q_BIO_new(q_BIO_s_mem());
        if (!bio)
            return result;

        meth->i2r(meth, ext_internal, bio, 0);

        char *bio_buffer;
        long bio_size = q_BIO_get_mem_data(bio, &bio_buffer);
        result = QByteArray(bio_buffer, bio_size);

        q_BIO_free(bio);
        return result;
    }

    return QVariant();
}

/*
 * Convert extensions to a variant. The naming of the keys of the map are
 * taken from RFC 5280, however we decided the capitalisation in the RFC
 * was too silly for the real world.
 */
static QVariant x509ExtensionToValue(X509_EXTENSION *ext)
{
    ASN1_OBJECT *obj = q_X509_EXTENSION_get_object(ext);
    int nid = q_OBJ_obj2nid(obj);

    switch (nid) {
    case NID_basic_constraints:
        {
            BASIC_CONSTRAINTS *basic = reinterpret_cast<BASIC_CONSTRAINTS *>(q_X509V3_EXT_d2i(ext));

            QVariantMap result;
            result[QLatin1String("ca")] = basic->ca ? true : false;
            if (basic->pathlen)
                result[QLatin1String("pathLenConstraint")] = (qlonglong)q_ASN1_INTEGER_get(basic->pathlen);

            q_BASIC_CONSTRAINTS_free(basic);
            return result;
        }
        break;
    case NID_info_access:
        {
            AUTHORITY_INFO_ACCESS *info = reinterpret_cast<AUTHORITY_INFO_ACCESS *>(q_X509V3_EXT_d2i(ext));

            QVariantMap result;
            for (int i=0; i < q_SKM_sk_num(ACCESS_DESCRIPTION, info); i++) {
                ACCESS_DESCRIPTION *ad = q_SKM_sk_value(ACCESS_DESCRIPTION, info, i);

                GENERAL_NAME *name = ad->location;
                if (name->type == GEN_URI) {
                    int len = q_ASN1_STRING_length(name->d.uniformResourceIdentifier);
                    if (len < 0 || len >= 8192) {
                        // broken name
                        continue;
                    }

                    const char *uriStr = reinterpret_cast<const char *>(q_ASN1_STRING_data(name->d.uniformResourceIdentifier));
                    const QString uri = QString::fromUtf8(uriStr, len);

                    result[QString::fromUtf8(QSslCertificatePrivate::asn1ObjectName(ad->method))] = uri;
                } else {
                    qCWarning(lcSsl) << "Strange location type" << name->type;
                }
            }

#if OPENSSL_VERSION_NUMBER >= 0x10000000L
            q_sk_pop_free((_STACK*)info, reinterpret_cast<void(*)(void*)>(q_sk_free));
#else
            q_sk_pop_free((STACK*)info, reinterpret_cast<void(*)(void*)>(q_sk_free));
#endif
            return result;
        }
        break;
    case NID_subject_key_identifier:
        {
            void *ext_internal = q_X509V3_EXT_d2i(ext);

            // we cast away the const-ness here because some versions of openssl
            // don't use const for the parameters in the functions pointers stored
            // in the object.
            X509V3_EXT_METHOD *meth = const_cast<X509V3_EXT_METHOD *>(q_X509V3_EXT_get(ext));

            return QVariant(QString::fromUtf8(meth->i2s(meth, ext_internal)));
        }
        break;
    case NID_authority_key_identifier:
        {
            AUTHORITY_KEYID *auth_key = reinterpret_cast<AUTHORITY_KEYID *>(q_X509V3_EXT_d2i(ext));

            QVariantMap result;

            // keyid
            if (auth_key->keyid) {
                QByteArray keyid(reinterpret_cast<const char *>(auth_key->keyid->data),
                                 auth_key->keyid->length);
                result[QLatin1String("keyid")] = keyid.toHex();
            }

            // issuer
            // TODO: GENERAL_NAMES

            // serial
            if (auth_key->serial)
                result[QLatin1String("serial")] = (qlonglong)q_ASN1_INTEGER_get(auth_key->serial);

            q_AUTHORITY_KEYID_free(auth_key);
            return result;
        }
        break;
    }

    return QVariant();
}

QSslCertificateExtension QSslCertificatePrivate::convertExtension(X509_EXTENSION *ext)
{
    QSslCertificateExtension result;

    ASN1_OBJECT *obj = q_X509_EXTENSION_get_object(ext);
    QByteArray oid = QSslCertificatePrivate::asn1ObjectId(obj);
    QByteArray name = QSslCertificatePrivate::asn1ObjectName(obj);

    result.d->oid = QString::fromUtf8(oid);
    result.d->name = QString::fromUtf8(name);

    bool critical = q_X509_EXTENSION_get_critical(ext);
    result.d->critical = critical;

    // Lets see if we have custom support for this one
    QVariant extensionValue = x509ExtensionToValue(ext);
    if (extensionValue.isValid()) {
        result.d->value = extensionValue;
        result.d->supported = true;

        return result;
    }

    extensionValue = x509UnknownExtensionToValue(ext);
    if (extensionValue.isValid()) {
        result.d->value = extensionValue;
        result.d->supported = false;
        return result;
    }

    return result;
}

QList<QSslCertificateExtension> QSslCertificate::extensions() const
{
    QList<QSslCertificateExtension> result;

    if (!d->x509)
        return result;

    int count = q_X509_get_ext_count(d->x509);

    for (int i=0; i < count; i++) {
        X509_EXTENSION *ext = q_X509_get_ext(d->x509, i);
        result << QSslCertificatePrivate::convertExtension(ext);
    }

    return result;
}

QByteArray QSslCertificate::toPem() const
{
    if (!d->x509)
        return QByteArray();
    return d->QByteArray_from_X509(d->x509, QSsl::Pem);
}

QByteArray QSslCertificate::toDer() const
{
    if (!d->x509)
        return QByteArray();
    return d->QByteArray_from_X509(d->x509, QSsl::Der);
}

QString QSslCertificate::toText() const
{
    if (!d->x509)
        return QString();
    return d->text_from_X509(d->x509);
}

#define BEGINCERTSTRING "-----BEGIN CERTIFICATE-----"
#define ENDCERTSTRING "-----END CERTIFICATE-----"

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

// ### refactor against QSsl::pemFromDer() etc. (to avoid redundant implementations)
QByteArray QSslCertificatePrivate::QByteArray_from_X509(X509 *x509, QSsl::EncodingFormat format)
{
    if (!x509) {
        qCWarning(lcSsl, "QSslSocketBackendPrivate::X509_to_QByteArray: null X509");
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

QString QSslCertificatePrivate::text_from_X509(X509 *x509)
{
    if (!x509) {
        qCWarning(lcSsl, "QSslSocketBackendPrivate::text_from_X509: null X509");
        return QString();
    }

    QByteArray result;
    BIO *bio = q_BIO_new(q_BIO_s_mem());
    if (!bio)
        return QString();

    q_X509_print(bio, x509);

    QVarLengthArray<char, 16384> data;
    int count = q_BIO_read(bio, data.data(), 16384);
    if ( count > 0 ) {
        result = QByteArray( data.data(), count );
    }

    q_BIO_free(bio);

    return QString::fromLatin1(result);
}

QByteArray QSslCertificatePrivate::asn1ObjectId(ASN1_OBJECT *object)
{
    char buf[80]; // The openssl docs a buffer length of 80 should be more than enough
    q_OBJ_obj2txt(buf, sizeof(buf), object, 1); // the 1 says always use the oid not the long name

    return QByteArray(buf);
}


QByteArray QSslCertificatePrivate::asn1ObjectName(ASN1_OBJECT *object)
{
    int nid = q_OBJ_obj2nid(object);
    if (nid != NID_undef)
        return QByteArray(q_OBJ_nid2sn(nid));

    return asn1ObjectId(object);
}

static QMap<QByteArray, QString> _q_mapFromX509Name(X509_NAME *name)
{
    QMap<QByteArray, QString> info;
    for (int i = 0; i < q_X509_NAME_entry_count(name); ++i) {
        X509_NAME_ENTRY *e = q_X509_NAME_get_entry(name, i);

        QByteArray name = QSslCertificatePrivate::asn1ObjectName(q_X509_NAME_ENTRY_get_object(e));
        unsigned char *data = 0;
        int size = q_ASN1_STRING_to_UTF8(&data, q_X509_NAME_ENTRY_get_data(e));
        info.insertMulti(name, QString::fromUtf8((char*)data, size));
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

    while (size > 0 && (count == -1 || certificates.size() < count)) {
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

QT_END_NAMESPACE
