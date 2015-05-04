/****************************************************************************
**
** Copyright (C) 2012 Jeremy Lainé <jeremy.laine@m4x.org>
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

#ifndef QDNSLOOKUP_H
#define QDNSLOOKUP_H

#include <QtCore/qlist.h>
#include <QtCore/qobject.h>
#include <QtCore/qshareddata.h>
#include <QtCore/qsharedpointer.h>
#include <QtCore/qstring.h>

QT_BEGIN_NAMESPACE

class QHostAddress;
class QDnsLookupPrivate;
class QDnsDomainNameRecordPrivate;
class QDnsHostAddressRecordPrivate;
class QDnsMailExchangeRecordPrivate;
class QDnsServiceRecordPrivate;
class QDnsTextRecordPrivate;

class Q_NETWORK_EXPORT QDnsDomainNameRecord
{
public:
    QDnsDomainNameRecord();
    QDnsDomainNameRecord(const QDnsDomainNameRecord &other);
    ~QDnsDomainNameRecord();

    void swap(QDnsDomainNameRecord &other) { qSwap(d, other.d); }

    QString name() const;
    quint32 timeToLive() const;
    QString value() const;

    QDnsDomainNameRecord &operator=(const QDnsDomainNameRecord &other);

private:
    QSharedDataPointer<QDnsDomainNameRecordPrivate> d;
    friend class QDnsLookupRunnable;
};

Q_DECLARE_SHARED(QDnsDomainNameRecord)

class Q_NETWORK_EXPORT QDnsHostAddressRecord
{
public:
    QDnsHostAddressRecord();
    QDnsHostAddressRecord(const QDnsHostAddressRecord &other);
    ~QDnsHostAddressRecord();

    void swap(QDnsHostAddressRecord &other) { qSwap(d, other.d); }

    QString name() const;
    quint32 timeToLive() const;
    QHostAddress value() const;

    QDnsHostAddressRecord &operator=(const QDnsHostAddressRecord &other);

private:
    QSharedDataPointer<QDnsHostAddressRecordPrivate> d;
    friend class QDnsLookupRunnable;
};

Q_DECLARE_SHARED(QDnsHostAddressRecord)

class Q_NETWORK_EXPORT QDnsMailExchangeRecord
{
public:
    QDnsMailExchangeRecord();
    QDnsMailExchangeRecord(const QDnsMailExchangeRecord &other);
    ~QDnsMailExchangeRecord();

    void swap(QDnsMailExchangeRecord &other) { qSwap(d, other.d); }

    QString exchange() const;
    QString name() const;
    quint16 preference() const;
    quint32 timeToLive() const;

    QDnsMailExchangeRecord &operator=(const QDnsMailExchangeRecord &other);

private:
    QSharedDataPointer<QDnsMailExchangeRecordPrivate> d;
    friend class QDnsLookupRunnable;
};

Q_DECLARE_SHARED(QDnsMailExchangeRecord)

class Q_NETWORK_EXPORT QDnsServiceRecord
{
public:
    QDnsServiceRecord();
    QDnsServiceRecord(const QDnsServiceRecord &other);
    ~QDnsServiceRecord();

    void swap(QDnsServiceRecord &other) { qSwap(d, other.d); }

    QString name() const;
    quint16 port() const;
    quint16 priority() const;
    QString target() const;
    quint32 timeToLive() const;
    quint16 weight() const;

    QDnsServiceRecord &operator=(const QDnsServiceRecord &other);

private:
    QSharedDataPointer<QDnsServiceRecordPrivate> d;
    friend class QDnsLookupRunnable;
};

Q_DECLARE_SHARED(QDnsServiceRecord)

class Q_NETWORK_EXPORT QDnsTextRecord
{
public:
    QDnsTextRecord();
    QDnsTextRecord(const QDnsTextRecord &other);
    ~QDnsTextRecord();

    void swap(QDnsTextRecord &other) { qSwap(d, other.d); }

    QString name() const;
    quint32 timeToLive() const;
    QList<QByteArray> values() const;

    QDnsTextRecord &operator=(const QDnsTextRecord &other);

private:
    QSharedDataPointer<QDnsTextRecordPrivate> d;
    friend class QDnsLookupRunnable;
};

Q_DECLARE_SHARED(QDnsTextRecord)

class Q_NETWORK_EXPORT QDnsLookup : public QObject
{
    Q_OBJECT
    Q_ENUMS(Error Type)
    Q_PROPERTY(Error error READ error NOTIFY finished)
    Q_PROPERTY(QString errorString READ errorString NOTIFY finished)
    Q_PROPERTY(QString name READ name WRITE setName NOTIFY nameChanged)
    Q_PROPERTY(Type type READ type WRITE setType NOTIFY typeChanged)
    Q_PROPERTY(QHostAddress nameserver READ nameserver WRITE setNameserver NOTIFY nameserverChanged)

public:
    enum Error
    {
        NoError = 0,
        ResolverError,
        OperationCancelledError,
        InvalidRequestError,
        InvalidReplyError,
        ServerFailureError,
        ServerRefusedError,
        NotFoundError
    };

    enum Type
    {
        A = 1,
        AAAA = 28,
        ANY = 255,
        CNAME = 5,
        MX = 15,
        NS = 2,
        PTR = 12,
        SRV = 33,
        TXT = 16
    };

    explicit QDnsLookup(QObject *parent = 0);
    QDnsLookup(Type type, const QString &name, QObject *parent = 0);
    QDnsLookup(Type type, const QString &name, const QHostAddress &nameserver, QObject *parent = 0);
    ~QDnsLookup();

    Error error() const;
    QString errorString() const;
    bool isFinished() const;

    QString name() const;
    void setName(const QString &name);

    Type type() const;
    void setType(QDnsLookup::Type);

    QHostAddress nameserver() const;
    void setNameserver(const QHostAddress &nameserver);

    QList<QDnsDomainNameRecord> canonicalNameRecords() const;
    QList<QDnsHostAddressRecord> hostAddressRecords() const;
    QList<QDnsMailExchangeRecord> mailExchangeRecords() const;
    QList<QDnsDomainNameRecord> nameServerRecords() const;
    QList<QDnsDomainNameRecord> pointerRecords() const;
    QList<QDnsServiceRecord> serviceRecords() const;
    QList<QDnsTextRecord> textRecords() const;


public Q_SLOTS:
    void abort();
    void lookup();

Q_SIGNALS:
    void finished();
    void nameChanged(const QString &name);
    void typeChanged(Type type);
    void nameserverChanged(const QHostAddress &nameserver);

private:
    Q_DECLARE_PRIVATE(QDnsLookup)
    Q_PRIVATE_SLOT(d_func(), void _q_lookupFinished(const QDnsLookupReply &reply))
};

QT_END_NAMESPACE

#endif // QDNSLOOKUP_H
