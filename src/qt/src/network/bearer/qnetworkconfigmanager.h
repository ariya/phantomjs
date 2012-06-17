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

#ifndef QNETWORKCONFIGURATIONMANAGER_H
#define QNETWORKCONFIGURATIONMANAGER_H

#ifdef QT_MOBILITY_BEARER
# include "qmobilityglobal.h"
#endif

#include <QtCore/qobject.h>
#include <QtNetwork/qnetworkconfiguration.h>

#ifndef QT_NO_BEARERMANAGEMENT

QT_BEGIN_HEADER

#ifndef QT_MOBILITY_BEARER
QT_BEGIN_NAMESPACE
#define QNetworkConfigurationManagerExport Q_NETWORK_EXPORT
QT_MODULE(Network)
#else
QTM_BEGIN_NAMESPACE
#define QNetworkConfigurationManagerExport Q_BEARER_EXPORT
#endif

class QNetworkConfigurationManagerPrivate;
class QNetworkConfigurationManagerExport QNetworkConfigurationManager : public QObject
{
    Q_OBJECT

public:
    enum Capability {
         CanStartAndStopInterfaces  = 0x00000001,
         DirectConnectionRouting = 0x00000002,
         SystemSessionSupport = 0x00000004,
         ApplicationLevelRoaming = 0x00000008,
         ForcedRoaming = 0x00000010,
         DataStatistics = 0x00000020,
         NetworkSessionRequired = 0x00000040
    };

    Q_DECLARE_FLAGS(Capabilities, Capability)

    explicit QNetworkConfigurationManager(QObject *parent = 0);
    virtual ~QNetworkConfigurationManager();

    QNetworkConfigurationManager::Capabilities capabilities() const;

    QNetworkConfiguration defaultConfiguration() const;
    QList<QNetworkConfiguration> allConfigurations(QNetworkConfiguration::StateFlags flags = 0) const;
    QNetworkConfiguration configurationFromIdentifier(const QString &identifier) const;

    bool isOnline() const;

public Q_SLOTS:
    void updateConfigurations();

Q_SIGNALS:
    void configurationAdded(const QNetworkConfiguration &config);
    void configurationRemoved(const QNetworkConfiguration &config);
    void configurationChanged(const QNetworkConfiguration &config);
    void onlineStateChanged(bool isOnline);
    void updateCompleted();
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QNetworkConfigurationManager::Capabilities)

#ifndef QT_MOBILITY_BEARER
QT_END_NAMESPACE
#else
QTM_END_NAMESPACE
#endif

QT_END_HEADER

#endif // QT_NO_BEARERMANAGEMENT

#endif // QNETWORKCONFIGURATIONMANAGER_H
