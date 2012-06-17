/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the FOO module of the Qt Toolkit.
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

/**
 * Some notes about the code:
 *
 * ** It is assumed that the system proxies are for url based requests
 *  ie. HTTP/HTTPS based.
 * ** It is assumed that proxies don't use authentication.
 * ** It is assumed that there is no exceptions to proxy use (Symbian side
 *  does have the field for it but it is not user modifiable by default).
 * ** There is no checking for protocol name.
 */

#include <QtNetwork/qnetworkproxy.h>

#ifndef QT_NO_NETWORKPROXY

#include <metadatabase.h> // CMDBSession
#include <commsdattypeinfov1_1.h> // CCDIAPRecord, CCDProxiesRecord
#include <commsdattypesv1_1.h> // KCDTIdIAPRecord, KCDTIdProxiesRecord
#include <QtNetwork/QNetworkConfigurationManager>
#include <QtNetwork/QNetworkConfiguration>
#include <QFlags>
#include <QtCore/private/qcore_symbian_p.h>

using namespace CommsDat;

QT_BEGIN_NAMESPACE

class SymbianIapId
{
public:
    SymbianIapId() : valid(false), id(0) {}
    ~SymbianIapId() {}
    void setIapId(TUint32 iapId) { valid = true; id = iapId; }
    bool isValid() { return valid; }
    TUint32 iapId() { return id; }
    static SymbianIapId fromConfiguration(const QNetworkConfiguration& config)
    {
        SymbianIapId iapId;
        // Note: the following code assumes that the identifier is in format
        // I_xxxx where xxxx is the identifier of IAP. This is meant as a
        // temporary solution until there is a support for returning
        // implementation specific identifier.
        const int generalPartLength = 2;
        QString idString(config.identifier().mid(generalPartLength));
        bool success;
        uint id = idString.toUInt(&success);
        if (success)
            iapId.setIapId(id);
        else
            qWarning() << "Failed to convert identifier to access point identifier: "
                << config.identifier();
        return iapId;
    }

private:
    bool valid;
    TUint32 id;
};

class SymbianProxyQuery
{
public:
    static QNetworkConfiguration findCurrentConfiguration(QNetworkConfigurationManager& configurationManager);
    static QNetworkConfiguration findCurrentConfigurationFromServiceNetwork(const QNetworkConfiguration& serviceNetwork);
    static SymbianIapId getIapId(QNetworkConfigurationManager &configurationManager, const QNetworkProxyQuery &query);
    static CCDIAPRecord *getIapRecordLC(TUint32 aIAPId, CMDBSession &aDb);
    static CMDBRecordSet<CCDProxiesRecord> *prepareQueryLC(TUint32 serviceId, TDesC& serviceType);
    static QList<QNetworkProxy> proxyQueryL(TUint32 aIAPId, const QNetworkProxyQuery &query);
};

QNetworkConfiguration SymbianProxyQuery::findCurrentConfigurationFromServiceNetwork(const QNetworkConfiguration& serviceNetwork)
{
    // Note: This code assumes that the only unambigious way to
    // find current proxy config is if there is only one access point
    // or if the found access point is immediately usable.
    QList<QNetworkConfiguration> childConfigurations = serviceNetwork.children();
    if (childConfigurations.isEmpty()) {
        qWarning("QNetworkProxyFactory::systemProxyForQuery called with empty service network");
        return QNetworkConfiguration();
    } else if (childConfigurations.count() == 1) {
        //if only one IAP in the service network, it's always going to be used.
        return childConfigurations.at(0);
    } else {
        //use highest priority active config, if available
        for (int index = 0; index < childConfigurations.count(); index++) {
            QNetworkConfiguration childConfig = childConfigurations.at(index);
            if (childConfig.isValid() && childConfig.state() == QNetworkConfiguration::Active)
                return childConfig;
        }
        //otherwise use highest priority discovered config (that's the one which will be activated if start were called now)
        for (int index = 0; index < childConfigurations.count(); index++) {
            QNetworkConfiguration childConfig = childConfigurations.at(index);
            if (childConfig.isValid() && childConfig.state() == QNetworkConfiguration::Discovered)
                return childConfig;
        }
        //otherwise the highest priority defined (most likely to be activated if all were available when start is called)
        qWarning("QNetworkProxyFactory::systemProxyForQuery called with service network, but none of its IAPs are available");
        return childConfigurations.at(0);
    }
}

QNetworkConfiguration SymbianProxyQuery::findCurrentConfiguration(QNetworkConfigurationManager& configurationManager)
{
    QList<TUint32> openConfigurations = QSymbianSocketManager::instance().activeConnections();
    QList<QNetworkConfiguration> activeConfigurations = configurationManager.allConfigurations(
        QNetworkConfiguration::Active);
    for (int i = 0; i < activeConfigurations.count(); i++) {
        // get first configuration which was opened by this process
        if (openConfigurations.contains(SymbianIapId::fromConfiguration(activeConfigurations.at(i)).iapId()))
            return activeConfigurations.at(i);
    }
    if (activeConfigurations.count() > 0) {
        // get first active configuration opened by any process
        return activeConfigurations.at(0);
    } else {
        // No active configurations, try default one
        QNetworkConfiguration defaultConfiguration = configurationManager.defaultConfiguration();
        if (defaultConfiguration.isValid()) {
            switch (defaultConfiguration.type()) {
            case QNetworkConfiguration::InternetAccessPoint:
                return defaultConfiguration;
            case QNetworkConfiguration::ServiceNetwork:
                return findCurrentConfigurationFromServiceNetwork(defaultConfiguration);
            case QNetworkConfiguration::UserChoice:
                qWarning("QNetworkProxyFactory::systemProxyForQuery called with user choice configuration, which is not valid");
                break;
            }
        }
    }
    return QNetworkConfiguration();
}

SymbianIapId SymbianProxyQuery::getIapId(QNetworkConfigurationManager& configurationManager, const QNetworkProxyQuery &query)
{
    SymbianIapId iapId;

    QNetworkConfiguration currentConfig = query.networkConfiguration();
    if (!currentConfig.isValid()) {
        //If config is not specified, then try to find out an active or default one
        currentConfig = findCurrentConfiguration(configurationManager);
    }
    if (currentConfig.isValid() && currentConfig.type() == QNetworkConfiguration::ServiceNetwork) {
        //convert service network to the real IAP.
        currentConfig = findCurrentConfigurationFromServiceNetwork(currentConfig);
    }
    if (currentConfig.isValid() && currentConfig.type() == QNetworkConfiguration::InternetAccessPoint) {
        // Note: the following code assumes that the identifier is in format
        // I_xxxx where xxxx is the identifier of IAP. This is meant as a
        // temporary solution until there is a support for returning
        // implementation specific identifier.
        const int generalPartLength = 2;
        const int identifierNumberLength = currentConfig.identifier().length() - generalPartLength;
        QString idString(currentConfig.identifier().right(identifierNumberLength));
        bool success;
        uint id = idString.toUInt(&success);
        if (success)
            iapId.setIapId(id);
        else
            qWarning() << "Failed to convert identifier to access point identifier: "
                << currentConfig.identifier();
    }

    return iapId;
}

CCDIAPRecord *SymbianProxyQuery::getIapRecordLC(TUint32 aIAPId, CMDBSession &aDb)
{
    CCDIAPRecord *iap = static_cast<CCDIAPRecord*> (CCDRecordBase::RecordFactoryL(KCDTIdIAPRecord));
    CleanupStack::PushL(iap);
    iap->SetRecordId(aIAPId);
    iap->LoadL(aDb);
    return iap;
}

CMDBRecordSet<CCDProxiesRecord> *SymbianProxyQuery::prepareQueryLC(TUint32 serviceId, TDesC& serviceType)
{
    // Create a recordset of type CCDProxiesRecord
    // for priming search.
    // This will ultimately contain record(s)
    // matching the priming record attributes
    CMDBRecordSet<CCDProxiesRecord> *proxyRecords = new (ELeave) CMDBRecordSet<CCDProxiesRecord> (
        KCDTIdProxiesRecord);
    CleanupStack::PushL(proxyRecords);

    CCDProxiesRecord *primingProxyRecord =
        static_cast<CCDProxiesRecord *> (CCDRecordBase::RecordFactoryL(KCDTIdProxiesRecord));
    CleanupStack::PushL(primingProxyRecord);

    primingProxyRecord->iServiceType.SetMaxLengthL(serviceType.Length());
    primingProxyRecord->iServiceType = serviceType;
    primingProxyRecord->iService = serviceId;
    primingProxyRecord->iUseProxyServer = ETrue;

    proxyRecords->iRecords.AppendL(primingProxyRecord);
    // Ownership of primingProxyRecord is transferred to
    // proxyRecords, just remove it from the CleanupStack
    CleanupStack::Pop(primingProxyRecord);
    return proxyRecords;
}

QList<QNetworkProxy> SymbianProxyQuery::proxyQueryL(TUint32 aIAPId, const QNetworkProxyQuery &query)
{
    QList<QNetworkProxy> foundProxies;
    if (query.queryType() != QNetworkProxyQuery::UrlRequest) {
        return foundProxies;
    }

    CMDBSession *iDb = CMDBSession::NewLC(KCDVersion1_1);
    CCDIAPRecord *iap = getIapRecordLC(aIAPId, *iDb);

    // Read service table id and service type
    // from the IAP record found
    TUint32 serviceId = iap->iService;
    RBuf serviceType;
    serviceType.CreateL(iap->iServiceType);
    CleanupStack::PopAndDestroy(iap);
    CleanupClosePushL(serviceType);

    CMDBRecordSet<CCDProxiesRecord> *proxyRecords = prepareQueryLC(serviceId, serviceType);

    // Now to find a proxy table matching our criteria
    if (proxyRecords->FindL(*iDb)) {
        TInt count = proxyRecords->iRecords.Count();
        for(TInt index = 0; index < count; index++) {
            CCDProxiesRecord *proxyRecord = static_cast<CCDProxiesRecord *> (proxyRecords->iRecords[index]);
            RBuf serverName;
            serverName.CreateL(proxyRecord->iServerName);
            CleanupClosePushL(serverName);
            if (serverName.Length() == 0)
                User::Leave(KErrNotFound);
            QString serverNameQt((const QChar*)serverName.Ptr(), serverName.Length());
            CleanupStack::Pop(); // serverName
            TUint32 port = proxyRecord->iPortNumber;

            //Symbian config doesn't include proxy type, assume http unless the port matches assigned port of another type
            //Mobile operators use a wide variety of port numbers for http proxies.
            QNetworkProxy::ProxyType proxyType = QNetworkProxy::HttpProxy;
            if (port == 1080) //IANA assigned port for SOCKS
                proxyType = QNetworkProxy::Socks5Proxy;
            QNetworkProxy proxy(proxyType, serverNameQt, port);
            foundProxies.append(proxy);
        }
    }

    CleanupStack::PopAndDestroy(proxyRecords);
    CleanupStack::Pop(); // serviceType
    CleanupStack::PopAndDestroy(iDb);

    return foundProxies;
}

QList<QNetworkProxy> QNetworkProxyFactory::systemProxyForQuery(const QNetworkProxyQuery &query)
{
    QList<QNetworkProxy> proxies;
    SymbianIapId iapId;
    TInt error;
    QNetworkConfigurationManager manager;
    iapId = SymbianProxyQuery::getIapId(manager, query);
    if (iapId.isValid()) {
        TRAP(error, proxies = SymbianProxyQuery::proxyQueryL(iapId.iapId(), query))
        if (error != KErrNone) {
            qWarning() << "Error while retrieving proxies: '" << error << '"';
            proxies.clear();
        }
    }
    proxies << QNetworkProxy::NoProxy;

    return proxies;
}

QT_END_NAMESPACE

#endif
