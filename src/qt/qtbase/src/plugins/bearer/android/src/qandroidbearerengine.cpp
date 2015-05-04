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

#include "qandroidbearerengine.h"
#include "../../qnetworksession_impl.h"
#include "wrappers/androidconnectivitymanager.h"

#ifndef QT_NO_BEARERMANAGEMENT

QT_BEGIN_NAMESPACE

static QString networkConfType(const AndroidNetworkInfo &networkInfo)
{
    switch (networkInfo.getType()) {
    case AndroidNetworkInfo::Mobile:
        return QStringLiteral("Mobile");
    case AndroidNetworkInfo::Wifi:
        return QStringLiteral("WiFi");
    case AndroidNetworkInfo::Wimax:
        return QStringLiteral("WiMax");
    case AndroidNetworkInfo::Ethernet:
        return QStringLiteral("Ethernet");
    case AndroidNetworkInfo::Bluetooth:
        return QStringLiteral("Bluetooth");
    default:
        break;
    }

    return QString();
}

static inline bool isMobile(QNetworkConfiguration::BearerType type)
{
    if (type == QNetworkConfiguration::BearerWLAN
        || type == QNetworkConfiguration::BearerWiMAX
        || type == QNetworkConfiguration::BearerBluetooth
        || type == QNetworkConfiguration::BearerEthernet
        || type == QNetworkConfiguration::BearerUnknown) {
        return false;
    }

    return true;
}

static QNetworkConfiguration::BearerType getBearerType(const AndroidNetworkInfo &networkInfo)
{
    switch (networkInfo.getType()) {
    case AndroidNetworkInfo::Mobile:
    {
        switch (networkInfo.getSubtype()) {
        case AndroidNetworkInfo::Gprs:
        case AndroidNetworkInfo::Edge:
        case AndroidNetworkInfo::Iden:    // 2G
            return QNetworkConfiguration::Bearer2G;
        case AndroidNetworkInfo::Umts:    // BearerWCDMA (3 .5 .75 G)
        case AndroidNetworkInfo::Hsdpa:   // 3G (?) UMTS
        case AndroidNetworkInfo::Hsupa:   // 3G (?) UMTS
            return QNetworkConfiguration::BearerWCDMA;
        case AndroidNetworkInfo::Cdma:    // CDMA ISA95[AB]
        case AndroidNetworkInfo::Cdma1xRTT:   // BearerCDMA2000 (3G)
        case AndroidNetworkInfo::Ehrpd:   // CDMA Bridge thing?!?
            return QNetworkConfiguration::BearerCDMA2000;
        case AndroidNetworkInfo::Evdo0:   // BearerEVDO
        case AndroidNetworkInfo::EvdoA:   // BearerEVDO
        case AndroidNetworkInfo::EvdoB:   // BearerEVDO
            return QNetworkConfiguration::BearerEVDO;
        case AndroidNetworkInfo::Hspa:
        case AndroidNetworkInfo::Hspap:   // HSPA+
            return QNetworkConfiguration::BearerHSPA;
        case AndroidNetworkInfo::Lte:     // BearerLTE (4G)
            return QNetworkConfiguration::BearerLTE;
        default:
            break;
        }
    }
    case AndroidNetworkInfo::Wifi:
        return QNetworkConfiguration::BearerWLAN;
    case AndroidNetworkInfo::Wimax:
        return QNetworkConfiguration::BearerWiMAX;
    case AndroidNetworkInfo::Bluetooth:
    case AndroidNetworkInfo::MobileDun:
        return QNetworkConfiguration::BearerBluetooth;
    case AndroidNetworkInfo::Ethernet:
        return QNetworkConfiguration::BearerEthernet;
    case AndroidNetworkInfo::MobileMms:
    case AndroidNetworkInfo::MobileSupl:
    case AndroidNetworkInfo::MobileHipri:
    case AndroidNetworkInfo::Dummy:
    case AndroidNetworkInfo::UnknownType:
        break;
    }

    return QNetworkConfiguration::BearerUnknown;
}

QAndroidBearerEngine::QAndroidBearerEngine(QObject *parent)
    : QBearerEngineImpl(parent),
      m_connectivityManager(0)
{
}

QAndroidBearerEngine::~QAndroidBearerEngine()
{
}

QString QAndroidBearerEngine::getInterfaceFromId(const QString &id)
{
    const QMutexLocker locker(&mutex);
    return m_configurationInterface.value(id);
}

bool QAndroidBearerEngine::hasIdentifier(const QString &id)
{
    const QMutexLocker locker(&mutex);
    return m_configurationInterface.contains(id);
}

void QAndroidBearerEngine::connectToId(const QString &id)
{
    Q_EMIT connectionError(id, OperationNotSupported);
}

void QAndroidBearerEngine::disconnectFromId(const QString &id)
{
    Q_EMIT connectionError(id, OperationNotSupported);
}

QNetworkSession::State QAndroidBearerEngine::sessionStateForId(const QString &id)
{
    const QMutexLocker locker(&mutex);
    QNetworkConfigurationPrivatePointer ptr = accessPointConfigurations.value(id);

    if ((!ptr || !ptr->isValid) || m_connectivityManager == 0)
        return QNetworkSession::Invalid;

    const QMutexLocker configLocker(&ptr->mutex);
    // Don't re-order...
    if ((ptr->state & QNetworkConfiguration::Active) == QNetworkConfiguration::Active) {
        return QNetworkSession::Connected;
    } else if ((ptr->state & QNetworkConfiguration::Discovered) == QNetworkConfiguration::Discovered) {
        return QNetworkSession::Disconnected;
    } else if ((ptr->state & QNetworkConfiguration::Defined) == QNetworkConfiguration::Defined) {
        return QNetworkSession::NotAvailable;
    } else if ((ptr->state & QNetworkConfiguration::Undefined) == QNetworkConfiguration::Undefined) {
        return QNetworkSession::NotAvailable;
    }

    return QNetworkSession::Invalid;
}

QNetworkConfigurationManager::Capabilities QAndroidBearerEngine::capabilities() const
{

    return AndroidTrafficStats::isTrafficStatsSupported()
            ? QNetworkConfigurationManager::ForcedRoaming
              | QNetworkConfigurationManager::DataStatistics
            : QNetworkConfigurationManager::ForcedRoaming;

}

QNetworkSessionPrivate *QAndroidBearerEngine::createSessionBackend()
{
    return new QNetworkSessionPrivateImpl();
}

QNetworkConfigurationPrivatePointer QAndroidBearerEngine::defaultConfiguration()
{
    return QNetworkConfigurationPrivatePointer();
}

bool QAndroidBearerEngine::requiresPolling() const
{
    return false;
}

quint64 QAndroidBearerEngine::bytesWritten(const QString &id)
{
    QMutexLocker lock(&mutex);
    QNetworkConfigurationPrivatePointer ptr = accessPointConfigurations.value(id);
    if (!ptr || !ptr->isValid)
        return 0;

    return isMobile(ptr->bearerType)
            ? AndroidTrafficStats::getMobileTxBytes()
            : AndroidTrafficStats::getTotalTxBytes() - AndroidTrafficStats::getMobileTxBytes();
}

quint64 QAndroidBearerEngine::bytesReceived(const QString &id)
{
    QMutexLocker lock(&mutex);
    QNetworkConfigurationPrivatePointer ptr = accessPointConfigurations.value(id);
    if (!ptr || !ptr->isValid)
        return 0;

    return isMobile(ptr->bearerType)
            ? AndroidTrafficStats::getMobileRxBytes()
            : AndroidTrafficStats::getTotalRxBytes() - AndroidTrafficStats::getMobileRxBytes();
}

quint64 QAndroidBearerEngine::startTime(const QString &id)
{
    Q_UNUSED(id);
    return Q_UINT64_C(0);
}

void QAndroidBearerEngine::initialize()
{
    if (m_connectivityManager != 0)
        return;

    m_connectivityManager = AndroidConnectivityManager::getInstance();
    if (m_connectivityManager == 0)
        return;

    updateConfigurations();

    connect(m_connectivityManager, &AndroidConnectivityManager::activeNetworkChanged,
            this, &QAndroidBearerEngine::updateConfigurations);

}

void QAndroidBearerEngine::requestUpdate()
{
    updateConfigurations();
}

void QAndroidBearerEngine::updateConfigurations()
{
#ifndef QT_NO_NETWORKINTERFACE
    if (m_connectivityManager == 0)
        return;

    {
        QMutexLocker locker(&mutex);
        QStringList oldKeys = accessPointConfigurations.keys();

        QList<QNetworkInterface> interfaces = QNetworkInterface::allInterfaces();
        if (interfaces.isEmpty())
            interfaces = QNetworkInterface::allInterfaces();

        // Create a configuration for each of the main types (WiFi, Mobile, Bluetooth, WiMax, Ethernet)
        foreach (const AndroidNetworkInfo &netInfo, m_connectivityManager->getAllNetworkInfo()) {

            if (!netInfo.isValid())
                continue;

            const QString name = networkConfType(netInfo);
            if (name.isEmpty())
                continue;

            QNetworkConfiguration::BearerType bearerType = getBearerType(netInfo);

            QString interfaceName;
            QNetworkConfiguration::StateFlag state = QNetworkConfiguration::Defined;
            if (netInfo.isAvailable()) {
                if (netInfo.isConnected()) {
                    // Attempt to map an interface to this configuration
                    while (!interfaces.isEmpty()) {
                        QNetworkInterface interface = interfaces.takeFirst();
                        // ignore loopback interface
                        if (!interface.isValid())
                            continue;

                        if (interface.flags() & QNetworkInterface::IsLoopBack)
                            continue;
                        // There is no way to get the interface from the NetworkInfo, so
                        // look for an active interface...
                        if (interface.flags() & QNetworkInterface::IsRunning
                                && !interface.addressEntries().isEmpty()) {
                            state = QNetworkConfiguration::Active;
                            interfaceName = interface.name();
                            break;
                        }
                    }
                }
            }

            const QString key = QString(QLatin1String("android:%1:%2")).arg(name).arg(interfaceName);
            const QString id = QString::number(qHash(key));
            m_configurationInterface[id] = interfaceName;

            oldKeys.removeAll(id);
            if (accessPointConfigurations.contains(id)) {
                QNetworkConfigurationPrivatePointer ptr = accessPointConfigurations.value(id);
                bool changed = false;
                {
                    const QMutexLocker confLocker(&ptr->mutex);

                    if (!ptr->isValid) {
                        ptr->isValid = true;
                        changed = true;
                    }

                    // Don't reset the bearer type to 'Unknown'
                    if (ptr->bearerType != QNetworkConfiguration::BearerUnknown
                            && ptr->bearerType != bearerType) {
                        ptr->bearerType = bearerType;
                        changed = true;
                    }

                    if (ptr->name != name) {
                        ptr->name = name;
                        changed = true;
                    }

                    if (ptr->id != id) {
                        ptr->id = id;
                        changed = true;
                    }

                    if (ptr->state != state) {
                        ptr->state = state;
                        changed = true;
                    }
                } // Unlock configuration

                if (changed) {
                    locker.unlock();
                    Q_EMIT configurationChanged(ptr);
                    locker.relock();
                }
            } else {
                QNetworkConfigurationPrivatePointer ptr(new QNetworkConfigurationPrivate);
                ptr->name = name;
                ptr->isValid = true;
                ptr->id = id;
                ptr->state = state;
                ptr->type = QNetworkConfiguration::InternetAccessPoint;
                ptr->bearerType = bearerType;
                accessPointConfigurations.insert(id, ptr);
                locker.unlock();
                Q_EMIT configurationAdded(ptr);
                locker.relock();
            }
        }

        while (!oldKeys.isEmpty()) {
            QNetworkConfigurationPrivatePointer ptr = accessPointConfigurations.take(oldKeys.takeFirst());
            m_configurationInterface.remove(ptr->id);
            locker.unlock();
            Q_EMIT configurationRemoved(ptr);
            locker.relock();
        }

    } // Unlock engine

#endif // QT_NO_NETWORKINTERFACE

    Q_EMIT updateCompleted();
}

QT_END_NAMESPACE

#endif // QT_NO_BEARERMANAGEMENT
