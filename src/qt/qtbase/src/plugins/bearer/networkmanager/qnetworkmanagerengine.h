/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the plugins of the Qt Toolkit.
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

#ifndef QNETWORKMANAGERENGINE_P_H
#define QNETWORKMANAGERENGINE_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of the QLibrary class.  This header file may change from
// version to version without notice, or even be removed.
//
// We mean it.
//

#include "../qbearerengine_impl.h"

#include "qnetworkmanagerservice.h"

#include "../linux_common/qofonoservice_linux_p.h"

#include <QMap>
#include <QVariant>

#ifndef QT_NO_BEARERMANAGEMENT
#ifndef QT_NO_DBUS

QT_BEGIN_NAMESPACE

class QNetworkManagerEngine : public QBearerEngineImpl
{
    Q_OBJECT

public:
    QNetworkManagerEngine(QObject *parent = 0);
    ~QNetworkManagerEngine();

    bool networkManagerAvailable() const;

    QString getInterfaceFromId(const QString &id);
    bool hasIdentifier(const QString &id);

    void connectToId(const QString &id);
    void disconnectFromId(const QString &id);

    Q_INVOKABLE void initialize();
    Q_INVOKABLE void requestUpdate();

    QNetworkSession::State sessionStateForId(const QString &id);

    quint64 bytesWritten(const QString &id);
    quint64 bytesReceived(const QString &id);
    quint64 startTime(const QString &id);

    QNetworkConfigurationManager::Capabilities capabilities() const;

    QNetworkSessionPrivate *createSessionBackend();

    QNetworkConfigurationPrivatePointer defaultConfiguration();

private Q_SLOTS:
    void interfacePropertiesChanged(const QMap<QString, QVariant> &properties);
    void activeConnectionPropertiesChanged(const QMap<QString, QVariant> &properties);

    void deviceAdded(const QDBusObjectPath &path);
    void deviceRemoved(const QDBusObjectPath &path);

    void newConnection(const QDBusObjectPath &path, QNetworkManagerSettings *settings = 0);
    void removeConnection(const QString &path);
    void updateConnection();
    void activationFinished(QDBusPendingCallWatcher *watcher);
    void deviceConnectionsChanged(const QStringList &activeConnectionsList);

    void newAccessPoint(const QString &path);
    void removeAccessPoint(const QString &path);
    void scanFinished();

    void wiredCarrierChanged(bool);

    void nmRegistered(const QString &serviceName = QString());
    void nmUnRegistered(const QString &serviceName = QString());

    void ofonoRegistered(const QString &serviceName = QString());
    void ofonoUnRegistered(const QString &serviceName = QString());

private:
    QNetworkConfigurationPrivate *parseConnection(const QString &settingsPath,
                                                  const QNmSettingsMap &map);
    QNetworkManagerSettingsConnection *connectionFromId(const QString &id) const;

    QNetworkManagerInterface *managerInterface;
    QNetworkManagerSettings *systemSettings;
    QHash<QString, QNetworkManagerInterfaceDeviceWired *> wiredDevices;
    QHash<QString, QNetworkManagerInterfaceDeviceWireless *> wirelessDevices;

    QHash<QString, QNetworkManagerConnectionActive *> activeConnectionsList;
    QList<QNetworkManagerSettingsConnection *> connections;
    QList<QNetworkManagerInterfaceAccessPoint *> accessPoints;
    QHash<QString, QNetworkManagerInterfaceDevice *> interfaceDevices;

    QMap<QString,QString> configuredAccessPoints; //ap, settings path
    QHash<QString,QString> connectionInterfaces; // ac, interface

    QOfonoManagerInterface *ofonoManager;
    QHash <QString, QOfonoDataConnectionManagerInterface *> ofonoContextManagers;
    QNetworkConfiguration::BearerType currentBearerType(const QString &id);
    QString contextName(const QString &path);

    bool isConnectionActive(const QString &settingsPath);
    QDBusServiceWatcher *ofonoWatcher;
    QDBusServiceWatcher *nmWatcher;

    bool isActiveContext(const QString &contextPath);
    bool nmAvailable;
    void setupConfigurations();
};

QT_END_NAMESPACE

#endif // QT_NO_DBUS
#endif // QT_NO_BEARERMANAGEMENT

#endif

