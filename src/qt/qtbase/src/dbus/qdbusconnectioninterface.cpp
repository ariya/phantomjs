/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtDBus module of the Qt Toolkit.
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

#include "qdbusconnectioninterface.h"

#include <QtCore/QByteArray>
#include <QtCore/QList>
#include <QtCore/QMap>
#include <QtCore/QMetaMethod>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QVariant>
#include <QtCore/QDebug>

#include "qdbus_symbols_p.h"          // for the DBUS_* constants

#ifndef QT_NO_DBUS

QT_BEGIN_NAMESPACE

/*
 * Implementation of interface class QDBusConnectionInterface
 */

/*!
    \class QDBusConnectionInterface
    \inmodule QtDBus
    \since 4.2

    \brief The QDBusConnectionInterface class provides access to the D-Bus bus daemon service.

    The D-Bus bus server daemon provides one special interface \c
    org.freedesktop.DBus that allows clients to access certain
    properties of the bus, such as the current list of clients
    connected. The QDBusConnectionInterface class provides access to that
    interface.

    The most common uses of this class are to register and unregister
    service names on the bus using the registerService() and
    unregisterService() functions, query about existing names using
    the isServiceRegistered(), registeredServiceNames() and
    serviceOwner() functions, and to receive notification that a
    client has registered or de-registered through the
    serviceRegistered(), serviceUnregistered() and serviceOwnerChanged()
    signals.
*/

/*!
    \enum QDBusConnectionInterface::ServiceQueueOptions

    Flags for determining how a service registration should behave, in
    case the service name is already registered.

    \value DontQueueService     If an application requests a name that
                                is already owned, no queueing will be
                                performed. The registeredService()
                                call will simply fail.
                                This is the default.

    \value QueueService         Attempts to register the requested
                                service, but do not try to replace it
                                if another application already has it
                                registered. Instead, simply put this
                                application in queue, until it is
                                given up. The serviceRegistered()
                                signal will be emitted when that
                                happens.

    \value ReplaceExistingService If another application already has
                                the service name registered, attempt
                                to replace it.

    \sa ServiceReplacementOptions
*/

/*!
    \enum QDBusConnectionInterface::ServiceReplacementOptions

    Flags for determining if the D-Bus server should allow another
    application to replace a name that this application has registered
    with the ReplaceExistingService option.

    The possible values are:

    \value DontAllowReplacement Do not allow another application to
                                replace us. The service must be
                                explicitly unregistered with
                                unregisterService() for another
                                application to acquire it.
                                This is the default.

    \value AllowReplacement     Allow other applications to replace us
                                with the ReplaceExistingService option
                                to registerService() without
                                intervention. If that happens, the
                                serviceUnregistered() signal will be
                                emitted.

    \sa ServiceQueueOptions
*/

/*!
    \enum QDBusConnectionInterface::RegisterServiceReply

    The possible return values from registerService():

    \value ServiceNotRegistered The call failed and the service name was not registered.
    \value ServiceRegistered    The caller is now the owner of the service name.
    \value ServiceQueued        The caller specified the QueueService flag and the
                                service was already registered, so we are in queue.

    The serviceRegistered() signal will be emitted when the service is
    acquired by this application.
*/

/*!
    \internal
*/
const char *QDBusConnectionInterface::staticInterfaceName()
{ return "org.freedesktop.DBus"; }

/*!
    \internal
*/
QDBusConnectionInterface::QDBusConnectionInterface(const QDBusConnection &connection,
                                                   QObject *parent)
    : QDBusAbstractInterface(QLatin1String(DBUS_SERVICE_DBUS),
                             QLatin1String(DBUS_PATH_DBUS),
                             DBUS_INTERFACE_DBUS, connection, parent)
{
    connect(this, SIGNAL(NameAcquired(QString)), this, SIGNAL(serviceRegistered(QString)));
    connect(this, SIGNAL(NameLost(QString)), this, SIGNAL(serviceUnregistered(QString)));
    connect(this, SIGNAL(NameOwnerChanged(QString,QString,QString)),
            this, SIGNAL(serviceOwnerChanged(QString,QString,QString)));
}

/*!
    \internal
*/
QDBusConnectionInterface::~QDBusConnectionInterface()
{
}

/*!
    Returns the unique connection name of the primary owner of the
    name \a name. If the requested name doesn't have an owner, returns
    a \c org.freedesktop.DBus.Error.NameHasNoOwner error.
*/
QDBusReply<QString> QDBusConnectionInterface::serviceOwner(const QString &name) const
{
    return internalConstCall(QDBus::AutoDetect, QLatin1String("GetNameOwner"), QList<QVariant>() << name);
}

/*!
  \property QDBusConnectionInterface::registeredServiceNames
  \brief holds the registered service names

  Lists all names currently registered on the bus.
*/
QDBusReply<QStringList> QDBusConnectionInterface::registeredServiceNames() const
{
    return internalConstCall(QDBus::AutoDetect, QLatin1String("ListNames"));
}

/*!
    Returns \c true if the service name \a serviceName has is currently
    registered.
*/
QDBusReply<bool> QDBusConnectionInterface::isServiceRegistered(const QString &serviceName) const
{
    return internalConstCall(QDBus::AutoDetect, QLatin1String("NameHasOwner"),
                             QList<QVariant>() << serviceName);
}

/*!
    Returns the Unix Process ID (PID) for the process currently
    holding the bus service \a serviceName.
*/
QDBusReply<uint> QDBusConnectionInterface::servicePid(const QString &serviceName) const
{
    return internalConstCall(QDBus::AutoDetect, QLatin1String("GetConnectionUnixProcessID"),
                             QList<QVariant>() << serviceName);
}

/*!
    Returns the Unix User ID (UID) for the process currently holding
    the bus service \a serviceName.
*/
QDBusReply<uint> QDBusConnectionInterface::serviceUid(const QString &serviceName) const
{
    return internalConstCall(QDBus::AutoDetect, QLatin1String("GetConnectionUnixUser"),
                             QList<QVariant>() << serviceName);
}

/*!
    Requests that the bus start the service given by the name \a name.
*/
QDBusReply<void> QDBusConnectionInterface::startService(const QString &name)
{
    return call(QLatin1String("StartServiceByName"), name, uint(0));
}

/*!
    Requests to register the service name \a serviceName on the
    bus. The \a qoption flag specifies how the D-Bus server should behave
    if \a serviceName is already registered. The \a roption flag
    specifies if the server should allow another application to
    replace our registered name.

    If the service registration succeeds, the serviceRegistered()
    signal will be emitted. If we are placed in queue, the signal will
    be emitted when we obtain the name. If \a roption is
    AllowReplacement, the serviceUnregistered() signal will be emitted
    if another application replaces this one.

    \sa unregisterService()
*/
QDBusReply<QDBusConnectionInterface::RegisterServiceReply>
QDBusConnectionInterface::registerService(const QString &serviceName,
                                          ServiceQueueOptions qoption,
                                          ServiceReplacementOptions roption)
{
    // reconstruct the low-level flags
    uint flags = 0;
    switch (qoption) {
    case DontQueueService:
        flags = DBUS_NAME_FLAG_DO_NOT_QUEUE;
        break;
    case QueueService:
        flags = 0;
        break;
    case ReplaceExistingService:
        flags = DBUS_NAME_FLAG_DO_NOT_QUEUE | DBUS_NAME_FLAG_REPLACE_EXISTING;
        break;
    }

    switch (roption) {
    case DontAllowReplacement:
        break;
    case AllowReplacement:
        flags |= DBUS_NAME_FLAG_ALLOW_REPLACEMENT;
        break;
    }

    QDBusMessage reply = call(QLatin1String("RequestName"), serviceName, flags);
//    qDebug() << "QDBusConnectionInterface::registerService" << serviceName << "Reply:" << reply;

    // convert the low-level flags to something that we can use
    if (reply.type() == QDBusMessage::ReplyMessage) {
        uint code = 0;

        switch (reply.arguments().at(0).toUInt()) {
        case DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER:
        case DBUS_REQUEST_NAME_REPLY_ALREADY_OWNER:
            code = uint(ServiceRegistered);
            break;

        case DBUS_REQUEST_NAME_REPLY_EXISTS:
            code = uint(ServiceNotRegistered);
            break;

        case DBUS_REQUEST_NAME_REPLY_IN_QUEUE:
            code = uint(ServiceQueued);
            break;
        }

        reply.setArguments(QVariantList() << code);
    }

    return reply;
}

/*!
    Releases the claim on the bus service name \a serviceName, that
    had been previously registered with registerService(). If this
    application had ownership of the name, it will be released for
    other applications to claim. If it only had the name queued, it
    gives up its position in the queue.
*/
QDBusReply<bool>
QDBusConnectionInterface::unregisterService(const QString &serviceName)
{
    QDBusMessage reply = call(QLatin1String("ReleaseName"), serviceName);
    if (reply.type() == QDBusMessage::ReplyMessage) {
        bool success = reply.arguments().at(0).toUInt() == DBUS_RELEASE_NAME_REPLY_RELEASED;
        reply.setArguments(QVariantList() << success);
    }
    return reply;
}

/*!
    \internal
*/
void QDBusConnectionInterface::connectNotify(const QMetaMethod &signal)
{
    // translate the signal names to what we really want
    // this avoids setting hooks for signals that don't exist on the bus
    static const QMetaMethod serviceRegisteredSignal = QMetaMethod::fromSignal(&QDBusConnectionInterface::serviceRegistered);
    static const QMetaMethod serviceUnregisteredSignal = QMetaMethod::fromSignal(&QDBusConnectionInterface::serviceUnregistered);
    static const QMetaMethod serviceOwnerChangedSignal = QMetaMethod::fromSignal(&QDBusConnectionInterface::serviceOwnerChanged);
    static const QMetaMethod NameAcquiredSignal = QMetaMethod::fromSignal(&QDBusConnectionInterface::NameAcquired);
    static const QMetaMethod NameLostSignal = QMetaMethod::fromSignal(&QDBusConnectionInterface::NameLost);
    static const QMetaMethod NameOwnerChangedSignal = QMetaMethod::fromSignal(&QDBusConnectionInterface::NameOwnerChanged);
    if (signal == serviceRegisteredSignal)
        QDBusAbstractInterface::connectNotify(NameAcquiredSignal);

    else if (signal == serviceUnregisteredSignal)
        QDBusAbstractInterface::connectNotify(NameLostSignal);

    else if (signal == serviceOwnerChangedSignal) {
        static bool warningPrinted = false;
        if (!warningPrinted) {
            qWarning("Connecting to deprecated signal QDBusConnectionInterface::serviceOwnerChanged(QString,QString,QString)");
            warningPrinted = true;
        }
        QDBusAbstractInterface::connectNotify(NameOwnerChangedSignal);
    }
}

/*!
    \internal
*/
void QDBusConnectionInterface::disconnectNotify(const QMetaMethod &signal)
{
    // translate the signal names to what we really want
    // this avoids setting hooks for signals that don't exist on the bus
    static const QMetaMethod serviceRegisteredSignal = QMetaMethod::fromSignal(&QDBusConnectionInterface::serviceRegistered);
    static const QMetaMethod serviceUnregisteredSignal = QMetaMethod::fromSignal(&QDBusConnectionInterface::serviceUnregistered);
    static const QMetaMethod serviceOwnerChangedSignal = QMetaMethod::fromSignal(&QDBusConnectionInterface::serviceOwnerChanged);
    static const QMetaMethod NameAcquiredSignal = QMetaMethod::fromSignal(&QDBusConnectionInterface::NameAcquired);
    static const QMetaMethod NameLostSignal = QMetaMethod::fromSignal(&QDBusConnectionInterface::NameLost);
    static const QMetaMethod NameOwnerChangedSignal = QMetaMethod::fromSignal(&QDBusConnectionInterface::NameOwnerChanged);
    if (signal == serviceRegisteredSignal)
        QDBusAbstractInterface::disconnectNotify(NameAcquiredSignal);

    else if (signal == serviceUnregisteredSignal)
        QDBusAbstractInterface::disconnectNotify(NameLostSignal);

    else if (signal == serviceOwnerChangedSignal)
        QDBusAbstractInterface::disconnectNotify(NameOwnerChangedSignal);
}

// signals
/*!
    \fn QDBusConnectionInterface::serviceRegistered(const QString &serviceName)

    This signal is emitted by the D-Bus server when the bus service
    name (unique connection name or well-known service name) given by
    \a serviceName is acquired by this application.

    Acquisition happens after this application has requested a name using
    registerService().
*/

/*!
    \fn QDBusConnectionInterface::serviceUnregistered(const QString &serviceName)

    This signal is emitted by the D-Bus server when this application
    loses ownership of the bus service name given by \a serviceName.
*/

/*!
    \fn QDBusConnectionInterface::serviceOwnerChanged(const QString &name, const QString &oldOwner, const QString &newOwner)
    \deprecated

    Use QDBusServiceWatcher instead.

    This signal is emitted by the D-Bus server whenever a service
    ownership change happens in the bus, including apparition and
    disparition of names.

    This signal means the application \a oldOwner lost ownership of
    bus name \a name to application \a newOwner. If \a oldOwner is an
    empty string, it means the name \a name has just been created; if
    \a newOwner is empty, the name \a name has no current owner and is
    no longer available.

    \note connecting to this signal will make the application listen for and
    receive every single service ownership change on the bus. Depending on
    how many services are running, this make the application be activated to
    receive more signals than it needs. To avoid this problem, use the
    QDBusServiceWatcher class, which can listen for specific changes.
*/

/*!
  \fn void QDBusConnectionInterface::callWithCallbackFailed(const QDBusError &error, const QDBusMessage &call)

  This signal is emitted when there is an error during a
  QDBusConnection::callWithCallback(). \a error specifies the error.
  \a call is the message that couldn't be delivered.

  \sa QDBusConnection::callWithCallback()
 */

QT_END_NAMESPACE

#endif // QT_NO_DBUS
