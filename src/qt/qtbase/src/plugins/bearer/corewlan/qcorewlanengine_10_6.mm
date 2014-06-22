/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the plugins of the Qt Toolkit.
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

#include <SystemConfiguration/SCNetworkConfiguration.h>

@interface QT_MANGLE_NAMESPACE(QNSListener) : NSObject
{
    NSNotificationCenter *notificationCenter;
    CWInterface *currentInterface;
    QCoreWlanEngine *engine;
    NSLock *locker;
}
- (void)notificationHandler:(NSNotification *)notification;
- (void)remove;
- (void)setEngine:(QCoreWlanEngine *)coreEngine;
- (QCoreWlanEngine *)engine;
- (void)dealloc;

@property (assign) QCoreWlanEngine* engine;

@end

@implementation QT_MANGLE_NAMESPACE(QNSListener)

- (id) init
{
    [locker lock];
    NSAutoreleasePool *autoreleasepool = [[NSAutoreleasePool alloc] init];
    notificationCenter = [NSNotificationCenter defaultCenter];
    currentInterface = [CWInterface interfaceWithName:nil];
    [notificationCenter addObserver:self selector:@selector(notificationHandler:) name:kCWPowerDidChangeNotification object:nil];
    [locker unlock];
    [autoreleasepool release];
    return self;
}

-(void)dealloc
{
    [super dealloc];
}

-(void)setEngine:(QCoreWlanEngine *)coreEngine
{
    [locker lock];
    if(!engine)
        engine = coreEngine;
    [locker unlock];
}

-(QCoreWlanEngine *)engine
{
    return engine;
}

-(void)remove
{
    [locker lock];
    [notificationCenter removeObserver:self];
    [locker unlock];
}

- (void)notificationHandler:(NSNotification *)notification
{
    Q_UNUSED(notification);
    engine->requestUpdate();
}
@end

static QT_MANGLE_NAMESPACE(QNSListener) *listener = 0;

QT_BEGIN_NAMESPACE

void networkChangeCallback(SCDynamicStoreRef/* store*/, CFArrayRef changedKeys, void *info)
{
    for ( long i = 0; i < CFArrayGetCount(changedKeys); i++) {

        QString changed =  QCFString::toQString((CFStringRef)CFArrayGetValueAtIndex(changedKeys, i));
        if( changed.contains("/Network/Global/IPv4")) {
            QCoreWlanEngine* wlanEngine = static_cast<QCoreWlanEngine*>(info);
            wlanEngine->requestUpdate();
        }
    }
    return;
}


QScanThread::QScanThread(QObject *parent)
    :QThread(parent)
{
}

QScanThread::~QScanThread()
{
}

void QScanThread::quit()
{
    wait();
}

void QScanThread::run()
{
    NSAutoreleasePool *autoreleasepool = [[NSAutoreleasePool alloc] init];
    QStringList found;
    mutex.lock();
    CWInterface *currentInterface = [CWInterface interfaceWithName: QCFString::toNSString(interfaceName)];
    mutex.unlock();

    if([currentInterface power]) {
        NSError *err = nil;
        NSDictionary *parametersDict =  [NSDictionary dictionaryWithObjectsAndKeys:
                                   [NSNumber numberWithBool:YES], kCWScanKeyMerge,
                                   [NSNumber numberWithInt:kCWScanTypeFast], kCWScanKeyScanType,
                                   [NSNumber numberWithInteger:100], kCWScanKeyRestTime, nil];

        NSArray* apArray = [currentInterface scanForNetworksWithParameters:parametersDict error:&err];
        CWNetwork *apNetwork;

        if (!err) {

            for(uint row=0; row < [apArray count]; row++ ) {
                apNetwork = [apArray objectAtIndex:row];

                const QString networkSsid = QCFString::toQString([apNetwork ssid]);
                const QString id = QString::number(qHash(QLatin1String("corewlan:") + networkSsid));
                found.append(id);

                QNetworkConfiguration::StateFlags state = QNetworkConfiguration::Undefined;
                bool known = isKnownSsid(networkSsid);
                if( [currentInterface.interfaceState intValue] == kCWInterfaceStateRunning) {
                    if( networkSsid == QCFString::toQString( [currentInterface ssid])) {
                        state = QNetworkConfiguration::Active;
                    }
                }
                if(state == QNetworkConfiguration::Undefined) {
                    if(known) {
                        state = QNetworkConfiguration::Discovered;
                    } else {
                        state = QNetworkConfiguration::Undefined;
                    }
                }
                QNetworkConfiguration::Purpose purpose = QNetworkConfiguration::UnknownPurpose;
                if([[apNetwork securityMode] intValue] == kCWSecurityModeOpen) {
                    purpose = QNetworkConfiguration::PublicPurpose;
                } else {
                    purpose = QNetworkConfiguration::PrivatePurpose;
                }

                found.append(foundNetwork(id, networkSsid, state, interfaceName, purpose));

            }
        }
    }
    // add known configurations that are not around.
    QMapIterator<QString, QMap<QString,QString> > i(userProfiles);
    while (i.hasNext()) {
        i.next();

        QString networkName = i.key();
        const QString id = QString::number(qHash(QLatin1String("corewlan:") + networkName));

        if(!found.contains(id)) {
            QString networkSsid = getSsidFromNetworkName(networkName);
            const QString ssidId = QString::number(qHash(QLatin1String("corewlan:") + networkSsid));
            QNetworkConfiguration::StateFlags state = QNetworkConfiguration::Undefined;
            QString interfaceName;
            QMapIterator<QString, QString> ij(i.value());
            while (ij.hasNext()) {
                ij.next();
                interfaceName = ij.value();
            }

            if( [currentInterface.interfaceState intValue] == kCWInterfaceStateRunning) {
                if( networkSsid == QCFString::toQString([currentInterface ssid])) {
                    state = QNetworkConfiguration::Active;
                }
            }
            if(state == QNetworkConfiguration::Undefined) {
                if( userProfiles.contains(networkName)
                    && found.contains(ssidId)) {
                    state = QNetworkConfiguration::Discovered;
                }
            }

            if(state == QNetworkConfiguration::Undefined) {
                state = QNetworkConfiguration::Defined;
            }

            found.append(foundNetwork(id, networkName, state, interfaceName, QNetworkConfiguration::UnknownPurpose));
        }
    }
    emit networksChanged();
    [autoreleasepool release];
}

QStringList QScanThread::foundNetwork(const QString &id, const QString &name, const QNetworkConfiguration::StateFlags state, const QString &interfaceName, const QNetworkConfiguration::Purpose purpose)
{
    QStringList found;
    QMutexLocker locker(&mutex);
        QNetworkConfigurationPrivate *ptr = new QNetworkConfigurationPrivate;

        ptr->name = name;
        ptr->isValid = true;
        ptr->id = id;
        ptr->state = state;
        ptr->type = QNetworkConfiguration::InternetAccessPoint;
        ptr->bearerType = QNetworkConfiguration::BearerWLAN;
        ptr->purpose = purpose;

        fetchedConfigurations.append( ptr);
        configurationInterface.insert(ptr->id, interfaceName);

        locker.unlock();
        locker.relock();
       found.append(id);
    return found;
}

QList<QNetworkConfigurationPrivate *> QScanThread::getConfigurations()
{
    QMutexLocker locker(&mutex);

    QList<QNetworkConfigurationPrivate *> foundConfigurations = fetchedConfigurations;
    fetchedConfigurations.clear();

    return foundConfigurations;
}

void QScanThread::getUserConfigurations()
{
    QMutexLocker locker(&mutex);

    NSAutoreleasePool *autoreleasepool = [[NSAutoreleasePool alloc] init];
    userProfiles.clear();

    NSArray *wifiInterfaces = [CWInterface supportedInterfaces];
    for(uint row=0; row < [wifiInterfaces count]; row++ ) {

        CWInterface *wifiInterface = [CWInterface interfaceWithName: [wifiInterfaces objectAtIndex:row]];
        if ( ![wifiInterface power] )
            continue;

        NSString *nsInterfaceName = [wifiInterface name];
// add user configured system networks
        SCDynamicStoreRef dynRef = SCDynamicStoreCreate(kCFAllocatorSystemDefault, (CFStringRef)@"Qt corewlan", nil, nil);
        NSDictionary * airportPlist = (NSDictionary *)SCDynamicStoreCopyValue(dynRef, (CFStringRef)[NSString stringWithFormat:@"Setup:/Network/Interface/%@/AirPort", nsInterfaceName]);
        CFRelease(dynRef);
        if(airportPlist != nil) {
            NSDictionary *prefNetDict = [airportPlist objectForKey:@"PreferredNetworks"];

            NSArray *thisSsidarray = [prefNetDict valueForKey:@"SSID_STR"];
            for(NSString *ssidkey in thisSsidarray) {
                QString thisSsid = QCFString::toQString(ssidkey);
                if(!userProfiles.contains(thisSsid)) {
                    QMap <QString,QString> map;
                    map.insert(thisSsid, QCFString::toQString(nsInterfaceName));
                    userProfiles.insert(thisSsid, map);
                }
            }
            CFRelease(airportPlist);
        }

        // 802.1X user profiles
        QString userProfilePath = QDir::homePath() + "/Library/Preferences/com.apple.eap.profiles.plist";
        NSDictionary* eapDict = [[[NSDictionary alloc] initWithContentsOfFile: QCFString::toNSString(userProfilePath)] autorelease];
        if(eapDict != nil) {
            NSString *profileStr= @"Profiles";
            NSString *nameStr = @"UserDefinedName";
            NSString *networkSsidStr = @"Wireless Network";
            for (id profileKey in eapDict) {
                if ([profileStr isEqualToString:profileKey]) {
                    NSDictionary *itemDict = [eapDict objectForKey:profileKey];
                    for (id itemKey in itemDict) {

                        NSInteger dictSize = [itemKey count];
                        id objects[dictSize];
                        id keys[dictSize];

                        [itemKey getObjects:objects andKeys:keys];
                        QString networkName;
                        QString ssid;
                        for(int i = 0; i < dictSize; i++) {
                            if([nameStr isEqualToString:keys[i]]) {
                                networkName = QCFString::toQString(objects[i]);
                            }
                            if([networkSsidStr isEqualToString:keys[i]]) {
                                ssid = QCFString::toQString(objects[i]);
                            }
                            if(!userProfiles.contains(networkName)
                                && !ssid.isEmpty()) {
                                QMap<QString,QString> map;
                                map.insert(ssid, QCFString::toQString(nsInterfaceName));
                                userProfiles.insert(networkName, map);
                            }
                        }
                    }
                }
            }
        }
    }
    [autoreleasepool release];
}

QString QScanThread::getSsidFromNetworkName(const QString &name)
{
    QMutexLocker locker(&mutex);

    QMapIterator<QString, QMap<QString,QString> > i(userProfiles);
    while (i.hasNext()) {
        i.next();
        QMap<QString,QString> map = i.value();
        QMapIterator<QString, QString> ij(i.value());
         while (ij.hasNext()) {
             ij.next();
             const QString networkNameHash = QString::number(qHash(QLatin1String("corewlan:") +i.key()));
             if(name == i.key() || name == networkNameHash) {
                 return ij.key();
             }
        }
    }
    return QString();
}

QString QScanThread::getNetworkNameFromSsid(const QString &ssid)
{
    QMutexLocker locker(&mutex);

    QMapIterator<QString, QMap<QString,QString> > i(userProfiles);
    while (i.hasNext()) {
        i.next();
        QMap<QString,QString> map = i.value();
        QMapIterator<QString, QString> ij(i.value());
         while (ij.hasNext()) {
             ij.next();
             if(ij.key() == ssid) {
                 return i.key();
             }
         }
    }
    return QString();
}

bool QScanThread::isKnownSsid(const QString &ssid)
{
    QMutexLocker locker(&mutex);

    QMapIterator<QString, QMap<QString,QString> > i(userProfiles);
    while (i.hasNext()) {
        i.next();
        QMap<QString,QString> map = i.value();
        if(map.keys().contains(ssid)) {
            return true;
        }
    }
    return false;
}


QCoreWlanEngine::QCoreWlanEngine(QObject *parent)
:   QBearerEngineImpl(parent), scanThread(0)
{
    scanThread = new QScanThread(this);
    connect(scanThread, SIGNAL(networksChanged()),
            this, SLOT(networksChanged()));
}

QCoreWlanEngine::~QCoreWlanEngine()
{
    while (!foundConfigurations.isEmpty())
        delete foundConfigurations.takeFirst();
    [listener remove];
    [listener release];
}

void QCoreWlanEngine::initialize()
{
    QMutexLocker locker(&mutex);
    NSAutoreleasePool *autoreleasepool = [[NSAutoreleasePool alloc] init];

    if([[CWInterface supportedInterfaces] count] > 0 && !listener) {
        listener = [[QT_MANGLE_NAMESPACE(QNSListener) alloc] init];
        listener.engine = this;
        hasWifi = true;
    } else {
        hasWifi = false;
    }
    storeSession = NULL;

    startNetworkChangeLoop();
    [autoreleasepool release];
}


QString QCoreWlanEngine::getInterfaceFromId(const QString &id)
{
    QMutexLocker locker(&mutex);

    return scanThread->configurationInterface.value(id);
}

bool QCoreWlanEngine::hasIdentifier(const QString &id)
{
    QMutexLocker locker(&mutex);

    return scanThread->configurationInterface.contains(id);
}

void QCoreWlanEngine::connectToId(const QString &id)
{
    QMutexLocker locker(&mutex);
    NSAutoreleasePool *autoreleasepool = [[NSAutoreleasePool alloc] init];
    QString interfaceString = getInterfaceFromId(id);

    CWInterface *wifiInterface =
        [CWInterface interfaceWithName: QCFString::toNSString(interfaceString)];

    if ([wifiInterface power]) {
        NSError *err = nil;
        NSMutableDictionary *params = [NSMutableDictionary dictionaryWithCapacity:0];

        QString wantedSsid;

        QNetworkConfigurationPrivatePointer ptr = accessPointConfigurations.value(id);

        const QString idHash = QString::number(qHash(QLatin1String("corewlan:") + ptr->name));
        const QString idHash2 = QString::number(qHash(QLatin1String("corewlan:") + scanThread->getNetworkNameFromSsid(ptr->name)));

        bool using8021X = false;
        if (idHash2 != id) {
            NSArray *array = [CW8021XProfile allUser8021XProfiles];

            for (NSUInteger i = 0; i < [array count]; ++i) {
                const QString networkNameHashCheck = QString::number(qHash(QLatin1String("corewlan:") + QCFString::toQString([[array objectAtIndex:i] userDefinedName])));

                const QString ssidHash = QString::number(qHash(QLatin1String("corewlan:") + QCFString::toQString([[array objectAtIndex:i] ssid])));

                if (id == networkNameHashCheck || id == ssidHash) {
                    const QString thisName = scanThread->getSsidFromNetworkName(id);
                    if (thisName.isEmpty())
                        wantedSsid = id;
                    else
                        wantedSsid = thisName;

                    [params setValue: [array objectAtIndex:i] forKey:kCWAssocKey8021XProfile];
                    using8021X = true;
                    break;
                }
            }
        }

        if (!using8021X) {
            QString wantedNetwork;
            QMapIterator<QString, QMap<QString,QString> > i(scanThread->userProfiles);
            while (i.hasNext()) {
                i.next();
                wantedNetwork = i.key();
                const QString networkNameHash = QString::number(qHash(QLatin1String("corewlan:") + wantedNetwork));
                if (id == networkNameHash) {
                    wantedSsid =  scanThread->getSsidFromNetworkName(wantedNetwork);
                    break;
                }
            }
        }
        NSDictionary *scanParameters = [NSDictionary dictionaryWithObjectsAndKeys:
                                        [NSNumber numberWithBool:YES], kCWScanKeyMerge,
                                        [NSNumber numberWithInt:kCWScanTypeFast], kCWScanKeyScanType,
                                        [NSNumber numberWithInteger:100], kCWScanKeyRestTime,
                                        QCFString::toNSString(wantedSsid), kCWScanKeySSID,
                                        nil];

        NSArray *scanArray = [wifiInterface scanForNetworksWithParameters:scanParameters error:&err];

        if(!err) {
            for(uint row=0; row < [scanArray count]; row++ ) {
                CWNetwork *apNetwork = [scanArray objectAtIndex:row];

                if(wantedSsid == QCFString::toQString([apNetwork ssid])) {

                    if(!using8021X) {
                        SecKeychainAttribute attributes[3];

                        NSString *account = [apNetwork ssid];
                        NSString *keyKind = @"AirPort network password";
                        NSString *keyName = account;

                        attributes[0].tag = kSecAccountItemAttr;
                        attributes[0].data = (void *)[account UTF8String];
                        attributes[0].length = [account length];

                        attributes[1].tag = kSecDescriptionItemAttr;
                        attributes[1].data = (void *)[keyKind UTF8String];
                        attributes[1].length = [keyKind length];

                        attributes[2].tag = kSecLabelItemAttr;
                        attributes[2].data = (void *)[keyName UTF8String];
                        attributes[2].length = [keyName length];

                        SecKeychainAttributeList attributeList = {3,attributes};

                        SecKeychainSearchRef searchRef;
                        SecKeychainSearchCreateFromAttributes(NULL, kSecGenericPasswordItemClass, &attributeList, &searchRef);

                        NSString *password = @"";
                        SecKeychainItemRef searchItem;

                        if (SecKeychainSearchCopyNext(searchRef, &searchItem) == noErr) {
                            UInt32 realPasswordLength;
                            SecKeychainAttribute attributesW[8];
                            attributesW[0].tag = kSecAccountItemAttr;
                            SecKeychainAttributeList listW = {1,attributesW};
                            char *realPassword;
                            OSStatus status = SecKeychainItemCopyContent(searchItem, NULL, &listW, &realPasswordLength,(void **)&realPassword);

                            if (status == noErr) {
                                if (realPassword != NULL) {

                                    QByteArray pBuf;
                                    pBuf.resize(realPasswordLength);
                                    pBuf.prepend(realPassword);
                                    pBuf.insert(realPasswordLength,'\0');

                                    password = [NSString stringWithUTF8String:pBuf];
                                }
                                SecKeychainItemFreeContent(&listW, realPassword);
                            }

                            CFRelease(searchItem);
                        } else {
                            qDebug() << "SecKeychainSearchCopyNext error";
                        }
                        [params setValue: password forKey: kCWAssocKeyPassphrase];
                    } // end using8021X


                    bool result = [wifiInterface associateToNetwork: apNetwork parameters:[NSDictionary dictionaryWithDictionary:params] error:&err];

                    if(!err) {
                        if(!result) {
                            emit connectionError(id, ConnectError);
                        } else {
                            return;
                        }
                    } else {
                        qDebug() <<"associate ERROR"<<  QCFString::toQString([err localizedDescription ]);
                    }
                }
            } //end scan network
        } else {
            qDebug() <<"scan ERROR"<<  QCFString::toQString([err localizedDescription ]);
        }
        emit connectionError(id, InterfaceLookupError);
    }

    locker.unlock();
    emit connectionError(id, InterfaceLookupError);
    [autoreleasepool release];
}

void QCoreWlanEngine::disconnectFromId(const QString &id)
{
    QMutexLocker locker(&mutex);

    QString interfaceString = getInterfaceFromId(id);
    NSAutoreleasePool *autoreleasepool = [[NSAutoreleasePool alloc] init];

    CWInterface *wifiInterface =
        [CWInterface interfaceWithName: QCFString::toNSString(interfaceString)];

    [wifiInterface disassociate];
    if ([[wifiInterface interfaceState]intValue] != kCWInterfaceStateInactive) {
        locker.unlock();
        emit connectionError(id, DisconnectionError);
        locker.relock();
    }
    [autoreleasepool release];
}

void QCoreWlanEngine::requestUpdate()
{
    scanThread->getUserConfigurations();
    doRequestUpdate();
}

void QCoreWlanEngine::doRequestUpdate()
{
    QMutexLocker locker(&mutex);

    NSAutoreleasePool *autoreleasepool = [[NSAutoreleasePool alloc] init];

    NSArray *wifiInterfaces = [CWInterface supportedInterfaces];
    for (uint row = 0; row < [wifiInterfaces count]; ++row) {
            scanThread->interfaceName = QCFString::toQString([wifiInterfaces objectAtIndex:row]);
            scanThread->start();
    }
    locker.unlock();
    if ([wifiInterfaces count] == 0)
        networksChanged();
    [autoreleasepool release];
}

bool QCoreWlanEngine::isWifiReady(const QString &wifiDeviceName)
{
    QMutexLocker locker(&mutex);
    bool haswifi = false;
    if(hasWifi) {
        NSAutoreleasePool *autoreleasepool = [[NSAutoreleasePool alloc] init];
        CWInterface *defaultInterface = [CWInterface interfaceWithName: QCFString::toNSString(wifiDeviceName)];
        if([defaultInterface power]) {
            haswifi = true;
        }
        [autoreleasepool release];
    }
    return haswifi;
}


QNetworkSession::State QCoreWlanEngine::sessionStateForId(const QString &id)
{
    QMutexLocker locker(&mutex);
    QNetworkConfigurationPrivatePointer ptr = accessPointConfigurations.value(id);

    if (!ptr)
        return QNetworkSession::Invalid;

    if (!ptr->isValid) {
        return QNetworkSession::Invalid;
    } else if ((ptr->state & QNetworkConfiguration::Active) == QNetworkConfiguration::Active) {
        return QNetworkSession::Connected;
    } else if ((ptr->state & QNetworkConfiguration::Discovered) ==
                QNetworkConfiguration::Discovered) {
        return QNetworkSession::Disconnected;
    } else if ((ptr->state & QNetworkConfiguration::Defined) == QNetworkConfiguration::Defined) {
        return QNetworkSession::NotAvailable;
    } else if ((ptr->state & QNetworkConfiguration::Undefined) ==
                QNetworkConfiguration::Undefined) {
        return QNetworkSession::NotAvailable;
    }

    return QNetworkSession::Invalid;
}

QNetworkConfigurationManager::Capabilities QCoreWlanEngine::capabilities() const
{
    return QNetworkConfigurationManager::ForcedRoaming;
}

void QCoreWlanEngine::startNetworkChangeLoop()
{

    SCDynamicStoreContext dynStoreContext = { 0, this/*(void *)storeSession*/, NULL, NULL, NULL };
    storeSession = SCDynamicStoreCreate(NULL,
                                 CFSTR("networkChangeCallback"),
                                 networkChangeCallback,
                                 &dynStoreContext);
    if (!storeSession ) {
        qWarning() << "could not open dynamic store: error:" << SCErrorString(SCError());
        return;
    }

    CFMutableArrayRef notificationKeys;
    notificationKeys = CFArrayCreateMutable(NULL, 0, &kCFTypeArrayCallBacks);
    CFMutableArrayRef patternsArray;
    patternsArray = CFArrayCreateMutable(NULL, 0, &kCFTypeArrayCallBacks);

    CFStringRef storeKey;
    storeKey = SCDynamicStoreKeyCreateNetworkGlobalEntity(NULL,
                                                     kSCDynamicStoreDomainState,
                                                     kSCEntNetIPv4);
    CFArrayAppendValue(notificationKeys, storeKey);
    CFRelease(storeKey);

    storeKey = SCDynamicStoreKeyCreateNetworkServiceEntity(NULL,
                                                      kSCDynamicStoreDomainState,
                                                      kSCCompAnyRegex,
                                                      kSCEntNetIPv4);
    CFArrayAppendValue(patternsArray, storeKey);
    CFRelease(storeKey);

    if (!SCDynamicStoreSetNotificationKeys(storeSession , notificationKeys, patternsArray)) {
        qWarning() << "register notification error:"<< SCErrorString(SCError());
        CFRelease(storeSession );
        CFRelease(notificationKeys);
        CFRelease(patternsArray);
        return;
    }
    CFRelease(notificationKeys);
    CFRelease(patternsArray);

    runloopSource = SCDynamicStoreCreateRunLoopSource(NULL, storeSession , 0);
    if (!runloopSource) {
        qWarning() << "runloop source error:"<< SCErrorString(SCError());
        CFRelease(storeSession );
        return;
    }

    CFRunLoopAddSource(CFRunLoopGetCurrent(), runloopSource, kCFRunLoopDefaultMode);
    return;
}

QNetworkSessionPrivate *QCoreWlanEngine::createSessionBackend()
{
    return new QNetworkSessionPrivateImpl;
}

QNetworkConfigurationPrivatePointer QCoreWlanEngine::defaultConfiguration()
{
    return QNetworkConfigurationPrivatePointer();
}

bool QCoreWlanEngine::requiresPolling() const
{
    return true;
}

void QCoreWlanEngine::networksChanged()
{
    QMutexLocker locker(&mutex);

    QStringList previous = accessPointConfigurations.keys();

    QList<QNetworkConfigurationPrivate *> foundConfigurations = scanThread->getConfigurations();
    while (!foundConfigurations.isEmpty()) {
        QNetworkConfigurationPrivate *cpPriv = foundConfigurations.takeFirst();

        previous.removeAll(cpPriv->id);

        if (accessPointConfigurations.contains(cpPriv->id)) {
            QNetworkConfigurationPrivatePointer ptr = accessPointConfigurations.value(cpPriv->id);

            bool changed = false;

            ptr->mutex.lock();

            if (ptr->isValid != cpPriv->isValid) {
                ptr->isValid = cpPriv->isValid;
                changed = true;
            }

            if (ptr->name != cpPriv->name) {
                ptr->name = cpPriv->name;
                changed = true;
            }

            if (ptr->bearerType != cpPriv->bearerType) {
                ptr->bearerType = cpPriv->bearerType;
                changed = true;
            }

            if (ptr->state != cpPriv->state) {
                ptr->state = cpPriv->state;
                changed = true;
            }

            ptr->mutex.unlock();

            if (changed) {
                locker.unlock();
                emit configurationChanged(ptr);
                locker.relock();
            }

            delete cpPriv;
        } else {
            QNetworkConfigurationPrivatePointer ptr(cpPriv);

            accessPointConfigurations.insert(ptr->id, ptr);

            locker.unlock();
            emit configurationAdded(ptr);
            locker.relock();
        }
    }

    while (!previous.isEmpty()) {
        QNetworkConfigurationPrivatePointer ptr =
            accessPointConfigurations.take(previous.takeFirst());

        locker.unlock();
        emit configurationRemoved(ptr);
        locker.relock();
    }

    locker.unlock();
    emit updateCompleted();

}

quint64 QCoreWlanEngine::bytesWritten(const QString &id)
{
    QMutexLocker locker(&mutex);
    const QString interfaceStr = getInterfaceFromId(id);
    return getBytes(interfaceStr,false);
}

quint64 QCoreWlanEngine::bytesReceived(const QString &id)
{
    QMutexLocker locker(&mutex);
    const QString interfaceStr = getInterfaceFromId(id);
    return getBytes(interfaceStr,true);
}

quint64 QCoreWlanEngine::startTime(const QString &identifier)
{
    QMutexLocker locker(&mutex);
    NSAutoreleasePool *autoreleasepool = [[NSAutoreleasePool alloc] init];
    quint64 timestamp = 0;

    NSString *filePath = @"/Library/Preferences/SystemConfiguration/com.apple.airport.preferences.plist";
    NSDictionary* plistDict = [[[NSDictionary alloc] initWithContentsOfFile:filePath] autorelease];
    if(plistDict == nil)
        return timestamp;
    NSString *input = @"KnownNetworks";
    NSString *timeStampStr = @"_timeStamp";

    NSString *ssidStr = @"SSID_STR";

    for (id key in plistDict) {
        if ([input isEqualToString:key]) {

            NSDictionary *knownNetworksDict = [plistDict objectForKey:key];
            if(knownNetworksDict == nil)
                return timestamp;
            for (id networkKey in knownNetworksDict) {
                bool isFound = false;
                NSDictionary *itemDict = [knownNetworksDict objectForKey:networkKey];
                if(itemDict == nil)
                    return timestamp;
                NSInteger dictSize = [itemDict count];
                id objects[dictSize];
                id keys[dictSize];

                [itemDict getObjects:objects andKeys:keys];
                bool ok = false;
                for(int i = 0; i < dictSize; i++) {
                    if([ssidStr isEqualToString:keys[i]]) {
                        const QString ident = QString::number(qHash(QLatin1String("corewlan:") + QCFString::toQString(objects[i])));
                        if(ident == identifier) {
                            ok = true;
                        }
                    }
                    if(ok && [timeStampStr isEqualToString:keys[i]]) {
                        timestamp = (quint64)[objects[i] timeIntervalSince1970];
                        isFound = true;
                        break;
                    }
                }
                if(isFound)
                    break;
            }
        }
    }
    [autoreleasepool release];
    return timestamp;
}

quint64 QCoreWlanEngine::getBytes(const QString &interfaceName, bool b)
{
    struct ifaddrs *ifAddressList, *ifAddress;
    struct if_data *if_data;

    quint64 bytes = 0;
    ifAddressList = nil;
    if(getifaddrs(&ifAddressList) == 0) {
        for(ifAddress = ifAddressList; ifAddress; ifAddress = ifAddress->ifa_next) {
            if(interfaceName == ifAddress->ifa_name) {
                if_data = (struct if_data*)ifAddress->ifa_data;
                if(b) {
                    bytes = if_data->ifi_ibytes;
                    break;
                } else {
                    bytes = if_data->ifi_obytes;
                    break;
                }
            }
        }
        freeifaddrs(ifAddressList);
    }
    return bytes;
}

QT_END_NAMESPACE
