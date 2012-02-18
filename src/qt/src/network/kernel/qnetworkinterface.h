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

#ifndef QNETWORKINTERFACE_H
#define QNETWORKINTERFACE_H

#include <QtCore/qshareddata.h>
#include <QtCore/qscopedpointer.h>
#include <QtNetwork/qhostaddress.h>

#ifndef QT_NO_NETWORKINTERFACE

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Network)

template<typename T> class QList;

class QNetworkAddressEntryPrivate;
class Q_NETWORK_EXPORT QNetworkAddressEntry
{
public:
    QNetworkAddressEntry();
    QNetworkAddressEntry(const QNetworkAddressEntry &other);
    QNetworkAddressEntry &operator=(const QNetworkAddressEntry &other);
    ~QNetworkAddressEntry();
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

Q_DECLARE_OPERATORS_FOR_FLAGS(QNetworkInterface::InterfaceFlags)

#ifndef QT_NO_DEBUG_STREAM
Q_NETWORK_EXPORT QDebug operator<<(QDebug debug, const QNetworkInterface &networkInterface);
#endif

QT_END_NAMESPACE

QT_END_HEADER

#endif // QT_NO_NETWORKINTERFACE

#endif
