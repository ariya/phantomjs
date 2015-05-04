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

#ifndef QNETWORKINTERFACE_H
#define QNETWORKINTERFACE_H

#include <QtCore/qshareddata.h>
#include <QtCore/qscopedpointer.h>
#include <QtNetwork/qhostaddress.h>

#ifndef QT_NO_NETWORKINTERFACE

QT_BEGIN_NAMESPACE


template<typename T> class QList;

class QNetworkAddressEntryPrivate;
class Q_NETWORK_EXPORT QNetworkAddressEntry
{
public:
    QNetworkAddressEntry();
    QNetworkAddressEntry(const QNetworkAddressEntry &other);
    QNetworkAddressEntry &operator=(const QNetworkAddressEntry &other);
    ~QNetworkAddressEntry();

    void swap(QNetworkAddressEntry &other) { qSwap(d, other.d); }

    bool operator==(const QNetworkAddressEntry &other) const;
    inline bool operator!=(const QNetworkAddressEntry &other) const
    { return !(*this == other); }

    QHostAddress ip() const;
    void setIp(const QHostAddress &newIp);

    QHostAddress netmask() const;
    void setNetmask(const QHostAddress &newNetmask);
    int prefixLength() const;
    void setPrefixLength(int length);

    QHostAddress broadcast() const;
    void setBroadcast(const QHostAddress &newBroadcast);

private:
    QScopedPointer<QNetworkAddressEntryPrivate> d;
};

Q_DECLARE_SHARED(QNetworkAddressEntry)

class QNetworkInterfacePrivate;
class Q_NETWORK_EXPORT QNetworkInterface
{
public:
    enum InterfaceFlag {
        IsUp = 0x1,
        IsRunning = 0x2,
        CanBroadcast = 0x4,
        IsLoopBack = 0x8,
        IsPointToPoint = 0x10,
        CanMulticast = 0x20
    };
    Q_DECLARE_FLAGS(InterfaceFlags, InterfaceFlag)

    QNetworkInterface();
    QNetworkInterface(const QNetworkInterface &other);
    QNetworkInterface &operator=(const QNetworkInterface &other);
    ~QNetworkInterface();

    void swap(QNetworkInterface &other) { qSwap(d, other.d); }

    bool isValid() const;

    int index() const;
    QString name() const;
    QString humanReadableName() const;
    InterfaceFlags flags() const;
    QString hardwareAddress() const;
    QList<QNetworkAddressEntry> addressEntries() const;

    static QNetworkInterface interfaceFromName(const QString &name);
    static QNetworkInterface interfaceFromIndex(int index);
    static QList<QNetworkInterface> allInterfaces();
    static QList<QHostAddress> allAddresses();

private:
    friend class QNetworkInterfacePrivate;
    QSharedDataPointer<QNetworkInterfacePrivate> d;
};

Q_DECLARE_SHARED(QNetworkInterface)

Q_DECLARE_OPERATORS_FOR_FLAGS(QNetworkInterface::InterfaceFlags)

#ifndef QT_NO_DEBUG_STREAM
Q_NETWORK_EXPORT QDebug operator<<(QDebug debug, const QNetworkInterface &networkInterface);
#endif

QT_END_NAMESPACE

#endif // QT_NO_NETWORKINTERFACE

#endif
