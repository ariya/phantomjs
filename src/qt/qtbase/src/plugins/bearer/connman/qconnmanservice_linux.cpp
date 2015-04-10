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

#include <QObject>
#include <QList>
#include <QtDBus/QtDBus>
#include <QtDBus/QDBusConnection>
#include <QtDBus/QDBusError>
#include <QtDBus/QDBusInterface>
#include <QtDBus/QDBusMessage>
#include <QtDBus/QDBusReply>
#include <QtDBus/QDBusPendingCallWatcher>
#include <QtDBus/QDBusObjectPath>
#include <QtDBus/QDBusPendingCall>

#include "qconnmanservice_linux_p.h"

#ifndef QT_NO_BEARERMANAGEMENT
#ifndef QT_NO_DBUS

QT_BEGIN_NAMESPACE

QDBusArgument &operator<<(QDBusArgument &argument, const ConnmanMap &map)
{
    argument.beginStructure();
    argument << map.objectPath << map.propertyMap;
    argument.endStructure();
    return argument;
}

const QDBusArgument &operator>>(const QDBusArgument &argument, ConnmanMap &map)
{
    argument.beginStructure();
    argument >> map.objectPath >> map.propertyMap;
    argument.endStructure();
    return argument;
}

QConnmanManagerInterface::QConnmanManagerInterface( QObject *parent)
        : QDBusAbstractInterface(QStringLiteral(CONNMAN_SERVICE),
                                 QStringLiteral(CONNMAN_MANAGER_PATH),
                                 CONNMAN_MANAGER_INTERFACE,
                                 QDBusConnection::systemBus(), parent)
{
    qDBusRegisterMetaType<ConnmanMap>();
    qDBusRegisterMetaType<ConnmanMapList>();

    QList<QVariant> argumentList;
    QDBusPendingReply<QVariantMap> props_reply = asyncCallWithArgumentList(QLatin1String("GetProperties"), argumentList);
    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(props_reply, this);

    QObject::connect(watcher,SIGNAL(finished(QDBusPendingCallWatcher*)),
                     this, SLOT(propertiesReply(QDBusPendingCallWatcher*)));

    QDBusConnection::systemBus().connect(QStringLiteral(CONNMAN_SERVICE),
                           QStringLiteral(CONNMAN_MANAGER_PATH),
                           QStringLiteral(CONNMAN_SERVICE_INTERFACE),
                           QStringLiteral("PropertyChanged"),
                           this,SLOT(changedProperty(QString,QDBusVariant)));


    QDBusConnection::systemBus().connect(QStringLiteral(CONNMAN_SERVICE),
                           QStringLiteral(CONNMAN_MANAGER_PATH),
                           QStringLiteral(CONNMAN_SERVICE_INTERFACE),
                           QStringLiteral("TechnologyAdded"),
                           this,SLOT(technologyAdded(QDBusObjectPath,QVariantMap)));

    QDBusConnection::systemBus().connect(QStringLiteral(CONNMAN_SERVICE),
                           QStringLiteral(CONNMAN_MANAGER_PATH),
                           QStringLiteral(CONNMAN_SERVICE_INTERFACE),
                           QStringLiteral("TechnologyRemoved"),
                           this,SLOT(technologyRemoved(QDBusObjectPath)));

    QList<QVariant> argumentList2;
    QDBusPendingReply<ConnmanMapList> serv_reply = asyncCallWithArgumentList(QLatin1String("GetServices"), argumentList2);
    QDBusPendingCallWatcher *watcher2 = new QDBusPendingCallWatcher(serv_reply, this);

    QObject::connect(watcher2,SIGNAL(finished(QDBusPendingCallWatcher*)),
                this, SLOT(servicesReply(QDBusPendingCallWatcher*)));

}

QConnmanManagerInterface::~QConnmanManagerInterface()
{
}

void QConnmanManagerInterface::changedProperty(const QString &name, const QDBusVariant &value)
{
    propertiesCacheMap[name] = value.variant();
}

void QConnmanManagerInterface::propertiesReply(QDBusPendingCallWatcher *call)
{
    QDBusPendingReply<QVariantMap> props_reply = *call;

    if (props_reply.isError()) {
        qDebug() << props_reply.error().message();
    } else {
        propertiesCacheMap = props_reply.value();
    }
    call->deleteLater();
}

void QConnmanManagerInterface::servicesReply(QDBusPendingCallWatcher *call)
{
    QDBusPendingReply<ConnmanMapList> serv_reply = *call;

    if (serv_reply.isError()) {
        qDebug() << serv_reply.error().message();
    } else {
        servicesList.clear(); //connman list changes order
        ConnmanMap connmanobj;
        Q_FOREACH (connmanobj, serv_reply.value()) {
            servicesList << connmanobj.objectPath.path();
        }
        Q_EMIT servicesReady(servicesList);
    }
    call->deleteLater();
}

void QConnmanManagerInterface::connectNotify(const QMetaMethod &signal)
{
    static const QMetaMethod propertyChangedSignal = QMetaMethod::fromSignal(&QConnmanManagerInterface::propertyChanged);
    if (signal == propertyChangedSignal) {
        if (!connection().connect(QStringLiteral(CONNMAN_SERVICE),
                               QStringLiteral(CONNMAN_MANAGER_PATH),
                               QStringLiteral(CONNMAN_MANAGER_INTERFACE),
                               QStringLiteral("PropertyChanged"),
                                   this,SIGNAL(propertyChanged(QString,QDBusVariant)))) {
            qWarning() << "PropertyChanged not connected";
        }
    }

    static const QMetaMethod servicesChangedSignal = QMetaMethod::fromSignal(&QConnmanManagerInterface::servicesChanged);
    if (signal == servicesChangedSignal) {
        if (!connection().connect(QStringLiteral(CONNMAN_SERVICE),
                               QStringLiteral(CONNMAN_MANAGER_PATH),
                               QStringLiteral(CONNMAN_MANAGER_INTERFACE),
                               QStringLiteral("ServicesChanged"),
                               this,SLOT(onServicesChanged(ConnmanMapList, QList<QDBusObjectPath>)))) {
            qWarning() << "servicesChanged not connected";
        }
    }
}

void QConnmanManagerInterface::onServicesChanged(const ConnmanMapList &changed, const QList<QDBusObjectPath> &removed)
{
    ConnmanMap connmanobj;
    servicesList.clear(); //connman list changes order
    Q_FOREACH (connmanobj, changed) {
        const QString svcPath(connmanobj.objectPath.path());
            servicesList << svcPath;
    }

   Q_EMIT servicesChanged(changed, removed);
}

QVariant QConnmanManagerInterface::getProperty(const QString &property)
{
    QVariant var;
    var = propertiesCacheMap.value(property);
    return var;
}

QVariantMap QConnmanManagerInterface::getProperties()
{
    if (propertiesCacheMap.isEmpty()) {
        QDBusPendingReply<QVariantMap> reply = call(QLatin1String("GetProperties"));
        reply.waitForFinished();
        if (!reply.isError()) {
            propertiesCacheMap = reply.value();
        }
    }
    return propertiesCacheMap;
}

QString QConnmanManagerInterface::getState()
{
    return getProperty(QStringLiteral("State")).toString();
}

bool QConnmanManagerInterface::getOfflineMode()
{
    QVariant var = getProperty(QStringLiteral("OfflineMode"));
    return qdbus_cast<bool>(var);
}

QStringList QConnmanManagerInterface::getTechnologies()
{
    if (technologiesMap.isEmpty()) {
        QDBusPendingReply<ConnmanMapList> reply = call(QLatin1String("GetTechnologies"));
        reply.waitForFinished();
        if (!reply.isError()) {
            Q_FOREACH (ConnmanMap map, reply.value()) {
                if (!technologiesMap.contains(map.objectPath.path())) {
                    technologyAdded(map.objectPath, map.propertyMap);
                }
            }
        }
    }
    return technologiesMap.keys();
}

QStringList QConnmanManagerInterface::getServices()
{
    if (servicesList.isEmpty()) {
        QDBusPendingReply<ConnmanMapList> reply = call(QLatin1String("GetServices"));
        reply.waitForFinished();
        if (!reply.isError()) {
            Q_FOREACH (ConnmanMap map, reply.value()) {
                servicesList << map.objectPath.path();
            }
        }
    }
    return servicesList;
}

void QConnmanManagerInterface::requestScan(const QString &type)
{
    Q_FOREACH (QConnmanTechnologyInterface *tech, technologiesMap) {
        if (tech->type() == type) {
            tech->scan();
        }
    }
}

void QConnmanManagerInterface::technologyAdded(const QDBusObjectPath &path, const QVariantMap &)
{
    if (!technologiesList.contains(path.path())) {
        technologiesList << path.path();
        QConnmanTechnologyInterface *tech;
        tech = new QConnmanTechnologyInterface(path.path(),this);
        technologiesMap.insert(path.path(),tech);
        connect(tech,SIGNAL(scanFinished()),this,SIGNAL(scanFinished()));
    }
}

void QConnmanManagerInterface::technologyRemoved(const QDBusObjectPath &path)
{
    if (technologiesList.contains(path.path())) {
        technologiesList.removeOne(path.path());
        QConnmanTechnologyInterface * tech = technologiesMap.take(path.path());
        delete tech;
    }
}

QConnmanServiceInterface::QConnmanServiceInterface(const QString &dbusPathName,QObject *parent)
    : QDBusAbstractInterface(QStringLiteral(CONNMAN_SERVICE),
                             dbusPathName,
                             CONNMAN_SERVICE_INTERFACE,
                             QDBusConnection::systemBus(), parent)
{
    QList<QVariant> argumentList;
    QDBusPendingReply<QVariantMap> props_reply = asyncCallWithArgumentList(QLatin1String("GetProperties"), argumentList);

    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(props_reply, this);

    QObject::connect(watcher,SIGNAL(finished(QDBusPendingCallWatcher*)),
            this, SLOT(propertiesReply(QDBusPendingCallWatcher*)));

    QDBusConnection::systemBus().connect(QStringLiteral(CONNMAN_SERVICE),
                           path(),
                           QStringLiteral(CONNMAN_SERVICE_INTERFACE),
                           QStringLiteral("PropertyChanged"),
                           this,SLOT(changedProperty(QString,QDBusVariant)));
}

QConnmanServiceInterface::~QConnmanServiceInterface()
{
}

QVariantMap QConnmanServiceInterface::getProperties()
{
    if (propertiesCacheMap.isEmpty()) {
        QDBusPendingReply<QVariantMap> reply = call(QLatin1String("GetProperties"));
        reply.waitForFinished();
        if (!reply.isError()) {
            propertiesCacheMap = reply.value();
            Q_EMIT propertiesReady();
        }
    }
    return propertiesCacheMap;
}

void QConnmanServiceInterface::propertiesReply(QDBusPendingCallWatcher *call)
{
    QDBusPendingReply<QVariantMap> props_reply = *call;
    if (props_reply.isError()) {
        qDebug() << props_reply.error().message();
        return;
    }
    propertiesCacheMap = props_reply.value();
    Q_EMIT propertiesReady();
}

void QConnmanServiceInterface::connectNotify(const QMetaMethod &signal)
{
    static const QMetaMethod propertyChangedSignal = QMetaMethod::fromSignal(&QConnmanServiceInterface::propertyChanged);
    if (signal == propertyChangedSignal) {
        QDBusConnection::systemBus().connect(QStringLiteral(CONNMAN_SERVICE),
                               path(),
                               QStringLiteral(CONNMAN_SERVICE_INTERFACE),
                               QStringLiteral("PropertyChanged"),
                               this,SIGNAL(propertyChanged(QString,QDBusVariant)));
    }
}

void QConnmanServiceInterface::changedProperty(const QString &name, const QDBusVariant &value)
{
    propertiesCacheMap[name] = value.variant();
    if (name == QStringLiteral("State"))
        Q_EMIT stateChanged(value.variant().toString());
}

QVariant QConnmanServiceInterface::getProperty(const QString &property)
{
    QVariant var;
    QVariantMap map = getProperties();
    var = map.value(property);
    return var;
}

void QConnmanServiceInterface::connect()
{
    asyncCall(QLatin1String("Connect"));
}

void QConnmanServiceInterface::disconnect()
{
    asyncCall(QLatin1String("Disconnect"));
}

void QConnmanServiceInterface::remove()
{
    asyncCall(QLatin1String("Remove"));
}

// properties
QString QConnmanServiceInterface::state()
{
    QVariant var = getProperty(QStringLiteral("State"));
    return qdbus_cast<QString>(var);
}

QString QConnmanServiceInterface::lastError()
{
    QVariant var = getProperty(QStringLiteral("Error"));
    return qdbus_cast<QString>(var);
}

QString QConnmanServiceInterface::name()
{
    QVariant var = getProperty(QStringLiteral("Name"));
    return qdbus_cast<QString>(var);
}

QString QConnmanServiceInterface::type()
{
    QVariant var = getProperty(QStringLiteral("Type"));
    return qdbus_cast<QString>(var);
}

QString QConnmanServiceInterface::security()
{
    QVariant var = getProperty(QStringLiteral("Security"));
    return qdbus_cast<QString>(var);
}

bool QConnmanServiceInterface::favorite()
{
    QVariant var = getProperty(QStringLiteral("Favorite"));
    return qdbus_cast<bool>(var);
}

bool QConnmanServiceInterface::autoConnect()
{
    QVariant var = getProperty(QStringLiteral("AutoConnect"));
    return qdbus_cast<bool>(var);
}

bool QConnmanServiceInterface::roaming()
{
    QVariant var = getProperty(QStringLiteral("Roaming"));
    return qdbus_cast<bool>(var);
}

QVariantMap QConnmanServiceInterface::ethernet()
{
    QVariant var = getProperty(QStringLiteral("Ethernet"));
    return qdbus_cast<QVariantMap >(var);
}

QString QConnmanServiceInterface::serviceInterface()
{
    QVariantMap map = ethernet();
    return map.value(QStringLiteral("Interface")).toString();
}

bool QConnmanServiceInterface::isOfflineMode()
{
    QVariant var = getProperty(QStringLiteral("OfflineMode"));
    return qdbus_cast<bool>(var);
}

QStringList QConnmanServiceInterface::services()
{
    QVariant var = getProperty(QStringLiteral("Services"));
    return qdbus_cast<QStringList>(var);
}

//////////////////////////
QConnmanTechnologyInterface::QConnmanTechnologyInterface(const QString &dbusPathName,QObject *parent)
    : QDBusAbstractInterface(QStringLiteral(CONNMAN_SERVICE),
                             dbusPathName,
                             CONNMAN_TECHNOLOGY_INTERFACE,
                             QDBusConnection::systemBus(), parent)
{
}

QConnmanTechnologyInterface::~QConnmanTechnologyInterface()
{
}

void QConnmanTechnologyInterface::connectNotify(const QMetaMethod &signal)
{
    static const QMetaMethod propertyChangedSignal = QMetaMethod::fromSignal(&QConnmanTechnologyInterface::propertyChanged);
    if (signal == propertyChangedSignal) {
        QDBusConnection::systemBus().connect(QStringLiteral(CONNMAN_SERVICE),
                               path(),
                               QStringLiteral(CONNMAN_TECHNOLOGY_INTERFACE),
                               QStringLiteral("PropertyChanged"),
                               this,SIGNAL(propertyChanged(QString,QDBusVariant)));
    }
}

QVariantMap QConnmanTechnologyInterface::properties()
{
    if (propertiesMap.isEmpty()) {
        QDBusPendingReply<QVariantMap> reply = call(QLatin1String("GetProperties"));
        reply.waitForFinished();
        propertiesMap = reply.value();
    }
    return propertiesMap;
}

QVariant QConnmanTechnologyInterface::getProperty(const QString &property)
{
    QVariant var;
    QVariantMap map = properties();
    var = map.value(property);
    return var;
}

QString QConnmanTechnologyInterface::type()
{
    QVariant var = getProperty(QStringLiteral("Type"));
    return qdbus_cast<QString>(var);
}

void QConnmanTechnologyInterface::scan()
{
    QDBusPendingReply<> reply = asyncCall(QLatin1String("Scan"));
    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(reply, this);
    connect(watcher, SIGNAL(finished(QDBusPendingCallWatcher*)),
            this, SLOT(scanReply(QDBusPendingCallWatcher*)));
}

void QConnmanTechnologyInterface::scanReply(QDBusPendingCallWatcher *call)
{
    Q_EMIT scanFinished();
    call->deleteLater();
}

QT_END_NAMESPACE

#endif // QT_NO_DBUS
#endif // QT_NO_BEARERMANAGEMENT

