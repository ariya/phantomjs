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

#ifndef QNETWORKCONFIGURATION_H
#define QNETWORKCONFIGURATION_H

#ifndef QT_MOBILITY_BEARER
# include <QtCore/qglobal.h>
#else
# include "qmobilityglobal.h"
#endif

#include <QtCore/qshareddata.h>
#include <QtCore/qstring.h>
#include <QtCore/qlist.h>

#if defined(Q_OS_WIN) && defined(interface)
#undef interface
#endif

QT_BEGIN_HEADER

#ifndef QT_MOBILITY_BEARER
QT_BEGIN_NAMESPACE
QT_MODULE(Network)
#define QNetworkConfigurationExport Q_NETWORK_EXPORT
#else
QTM_BEGIN_NAMESPACE
#define QNetworkConfigurationExport Q_BEARER_EXPORT
#endif

class QNetworkConfigurationPrivate;
class QNetworkConfigurationExport QNetworkConfiguration
{
public:
    QNetworkConfiguration();
    QNetworkConfiguration(const QNetworkConfiguration& other);
    QNetworkConfiguration &operator=(const QNetworkConfiguration &other);
    ~QNetworkConfiguration();

    bool operator==(const QNetworkConfiguration &other) const;
    inline bool operator!=(const QNetworkConfiguration &other) const
    { return !operator==(other); }

    enum Type {
        InternetAccessPoint = 0,
        ServiceNetwork,
        UserChoice,
        Invalid
    };

    enum Purpose {
        UnknownPurpose = 0,
        PublicPurpose,
        PrivatePurpose,
        ServiceSpecificPurpose
    };

    enum StateFlag {
        Undefined        = 0x0000001,
        Defined          = 0x0000002,
        Discovered       = 0x0000006,
        Active           = 0x000000e
    };
    Q_DECLARE_FLAGS(StateFlags, StateFlag)

#ifndef QT_MOBILITY_BEARER
    enum BearerType {
        BearerUnknown,
        BearerEthernet,
        BearerWLAN,
        Bearer2G,
        BearerCDMA2000,
        BearerWCDMA,
        BearerHSPA,
        BearerBluetooth,
        BearerWiMAX
    };
#endif

    StateFlags state() const;
    Type type() const;
    Purpose purpose() const;

#ifndef QT_MOBILITY_BEARER
#ifdef QT_DEPRECATED
    // Required to maintain source compatibility with Qt Mobility.
    QT_DEPRECATED inline QString bearerName() const { return bearerTypeName(); }
#endif
    BearerType bearerType() const;
    QString bearerTypeName() const;
#else
    QString bearerName() const;
#endif

    QString identifier() const;
    bool isRoamingAvailable() const;
    QList<QNetworkConfiguration> children() const;

    QString name() const;
    bool isValid() const;

private:
    friend class QNetworkConfigurationPrivate;
    friend class QNetworkConfigurationManager;
    friend class QNetworkConfigurationManagerPrivate;
    friend class QNetworkSessionPrivate;
    QExplicitlySharedDataPointer<QNetworkConfigurationPrivate> d;
};

#ifndef QT_MOBILITY_BEARER
QT_END_NAMESPACE
#else
QTM_END_NAMESPACE
#endif

QT_END_HEADER

#endif // QNETWORKCONFIGURATION_H
