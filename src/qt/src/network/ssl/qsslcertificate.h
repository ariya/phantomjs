/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
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


#ifndef QSSLCERTIFICATE_H
#define QSSLCERTIFICATE_H

#include <QtCore/qnamespace.h>
#include <QtCore/qbytearray.h>
#include <QtCore/qcryptographichash.h>
#include <QtCore/qregexp.h>
#include <QtCore/qsharedpointer.h>
#include <QtNetwork/qssl.h>

typedef struct x509_st X509; // ### check if this works

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Network)

#ifndef QT_NO_OPENSSL

class QDateTime;
class QIODevice;
class QSslKey;
class QStringList;
template <typename T, typename U> class QMultiMap;

class QSslCertificatePrivate;
class Q_NETWORK_EXPORT QSslCertificate
{
public:
    enum SubjectInfo {
        Organization,
        CommonName,
        LocalityName,
        OrganizationalUnitName,
        CountryName,
        StateOrProvinceName
    };

    QSslCertificate(QIODevice *device, QSsl::EncodingFormat format = QSsl::Pem);
    QSslCertificate( // ### s/encoded/data (to be consistent with signature in .cpp file) ?
        const QByteArray &encoded = QByteArray(), QSsl::EncodingFormat format = QSsl::Pem);
    QSslCertificate(const QSslCertificate &other);
    ~QSslCertificate();
    QSslCertificate &operator=(const QSslCertificate &other);
    bool operator==(const QSslCertificate &other) const;
    inline bool operator!=(const QSslCertificate &other) const { return !operator==(other); }

    bool isNull() const;
    bool isValid() const;
    void clear();

    // Certificate info
    QByteArray version() const;
    QByteArray serialNumber() const;
    QByteArray digest(QCryptographicHash::Algorithm algorithm = QCryptographicHash::Md5) const;
    QString issuerInfo(SubjectInfo info) const;
    QString issuerInfo(const QByteArray &tag) const;
    QString subjectInfo(SubjectInfo info) const;
    QString subjectInfo(const QByteArray &tag) const;
    QMultiMap<QSsl::AlternateNameEntryType, QString> alternateSubjectNames() const;
    QDateTime effectiveDate() const;
    QDateTime expiryDate() const;
    QSslKey publicKey() const;

    QByteArray toPem() const;
    QByteArray toDer() const;

    static QList<QSslCertificate> fromPath(
        const QString &path, QSsl::EncodingFormat format = QSsl::Pem,
        QRegExp::PatternSyntax syntax = QRegExp::FixedString);
    static QList<QSslCertificate> fromDevice(
        QIODevice *device, QSsl::EncodingFormat format = QSsl::Pem);
    static QList<QSslCertificate> fromData(
        const QByteArray &data, QSsl::EncodingFormat format = QSsl::Pem);

    Qt::HANDLE handle() const;

private:
    QExplicitlySharedDataPointer<QSslCertificatePrivate> d;
    friend class QSslCertificatePrivate;
    friend class QSslSocketBackendPrivate;
};

#ifndef QT_NO_DEBUG_STREAM
class QDebug;
Q_NETWORK_EXPORT QDebug operator<<(QDebug debug, const QSslCertificate &certificate);
Q_NETWORK_EXPORT QDebug operator<<(QDebug debug, QSslCertificate::SubjectInfo info);
#endif

#endif // QT_NO_OPENSSL

QT_END_NAMESPACE

QT_END_HEADER

#endif
