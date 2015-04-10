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


#ifndef QSSLCERTIFICATE_H
#define QSSLCERTIFICATE_H

#ifdef verify
#undef verify
#endif

#include <QtCore/qnamespace.h>
#include <QtCore/qbytearray.h>
#include <QtCore/qcryptographichash.h>
#include <QtCore/qdatetime.h>
#include <QtCore/qregexp.h>
#include <QtCore/qsharedpointer.h>
#include <QtCore/qmap.h>
#include <QtNetwork/qssl.h>

#ifndef QT_NO_SSL

QT_BEGIN_NAMESPACE

class QDateTime;
class QIODevice;
class QSslError;
class QSslKey;
class QSslCertificateExtension;
class QStringList;

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
        StateOrProvinceName,
        DistinguishedNameQualifier,
        SerialNumber,
        EmailAddress
    };

    explicit QSslCertificate(QIODevice *device, QSsl::EncodingFormat format = QSsl::Pem);
    explicit QSslCertificate(const QByteArray &data = QByteArray(), QSsl::EncodingFormat format = QSsl::Pem);
    QSslCertificate(const QSslCertificate &other);
    ~QSslCertificate();
    QSslCertificate &operator=(const QSslCertificate &other);

    inline void swap(QSslCertificate &other)
    { qSwap(d, other.d); }

    bool operator==(const QSslCertificate &other) const;
    inline bool operator!=(const QSslCertificate &other) const { return !operator==(other); }

    bool isNull() const;
#if QT_DEPRECATED_SINCE(5,0)
    QT_DEPRECATED inline bool isValid() const {
        const QDateTime currentTime = QDateTime::currentDateTime();
        return currentTime >= effectiveDate() &&
               currentTime <= expiryDate() &&
               !isBlacklisted();
    }
#endif
    bool isBlacklisted() const;
    void clear();

    // Certificate info
    QByteArray version() const;
    QByteArray serialNumber() const;
    QByteArray digest(QCryptographicHash::Algorithm algorithm = QCryptographicHash::Md5) const;
    QStringList issuerInfo(SubjectInfo info) const;
    QStringList issuerInfo(const QByteArray &attribute) const;
    QStringList subjectInfo(SubjectInfo info) const;
    QStringList subjectInfo(const QByteArray &attribute) const;
    QList<QByteArray> subjectInfoAttributes() const;
    QList<QByteArray> issuerInfoAttributes() const;
#if QT_DEPRECATED_SINCE(5,0)
    QT_DEPRECATED inline QMultiMap<QSsl::AlternateNameEntryType, QString>
                  alternateSubjectNames() const { return subjectAlternativeNames(); }
#endif
    QMultiMap<QSsl::AlternativeNameEntryType, QString> subjectAlternativeNames() const;
    QDateTime effectiveDate() const;
    QDateTime expiryDate() const;
    QSslKey publicKey() const;
    QList<QSslCertificateExtension> extensions() const;

    QByteArray toPem() const;
    QByteArray toDer() const;
    QString toText() const;

    static QList<QSslCertificate> fromPath(
        const QString &path, QSsl::EncodingFormat format = QSsl::Pem,
        QRegExp::PatternSyntax syntax = QRegExp::FixedString);
    static QList<QSslCertificate> fromDevice(
        QIODevice *device, QSsl::EncodingFormat format = QSsl::Pem);
    static QList<QSslCertificate> fromData(
        const QByteArray &data, QSsl::EncodingFormat format = QSsl::Pem);

    static QList<QSslError> verify(QList<QSslCertificate> certificateChain, const QString &hostName = QString());

    Qt::HANDLE handle() const;

private:
    QExplicitlySharedDataPointer<QSslCertificatePrivate> d;
    friend class QSslCertificatePrivate;
    friend class QSslSocketBackendPrivate;
};
Q_DECLARE_SHARED(QSslCertificate)

#ifndef QT_NO_DEBUG_STREAM
class QDebug;
Q_NETWORK_EXPORT QDebug operator<<(QDebug debug, const QSslCertificate &certificate);
Q_NETWORK_EXPORT QDebug operator<<(QDebug debug, QSslCertificate::SubjectInfo info);
#endif

QT_END_NAMESPACE

Q_DECLARE_METATYPE(QSslCertificate)

#endif // QT_NO_SSL

#endif
