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

#include "qdbusabstractinterface.h"
#include "qdbusabstractinterface_p.h"

#include <qthread.h>

#include "qdbusargument.h"
#include "qdbuspendingcall.h"
#include "qdbusmessage_p.h"
#include "qdbusmetaobject_p.h"
#include "qdbusmetatype_p.h"
#include "qdbusutil_p.h"

#include <qdebug.h>

#ifndef QT_NO_DBUS

QT_BEGIN_NAMESPACE

static QDBusError checkIfValid(const QString &service, const QString &path,
                               const QString &interface, bool isDynamic, bool isPeer)
{
    // We should be throwing exceptions here... oh well
    QDBusError error;

    // dynamic interfaces (QDBusInterface) can have empty interfaces, but not service and object paths
    // non-dynamic is the opposite: service and object paths can be empty, but not the interface
    if (!isDynamic) {
        // use assertion here because this should never happen, at all
        Q_ASSERT_X(!interface.isEmpty(), "QDBusAbstractInterface", "Interface name cannot be empty");
    }
    if (!QDBusUtil::checkBusName(service, (isDynamic && !isPeer) ? QDBusUtil::EmptyNotAllowed : QDBusUtil::EmptyAllowed, &error))
        return error;
    if (!QDBusUtil::checkObjectPath(path, isDynamic ? QDBusUtil::EmptyNotAllowed : QDBusUtil::EmptyAllowed, &error))
        return error;
    if (!QDBusUtil::checkInterfaceName(interface, QDBusUtil::EmptyAllowed, &error))
        return error;

    // no error
    return QDBusError();
}

QDBusAbstractInterfacePrivate::QDBusAbstractInterfacePrivate(const QString &serv,
                                                             const QString &p,
                                                             const QString &iface,
                                                             const QDBusConnection& con,
                                                             bool isDynamic)
    : connection(con), service(serv), path(p), interface(iface),
      lastError(checkIfValid(serv, p, iface, isDynamic, (connectionPrivate() &&
                                                         connectionPrivate()->mode == QDBusConnectionPrivate::PeerMode))),
      timeout(-1),
      isValid(!lastError.isValid())
{
    if (!isValid)
        return;

    if (!connection.isConnected()) {
        lastError = QDBusError(QDBusError::Disconnected,
                               QLatin1String("Not connected to D-Bus server"));
    } else if (!service.isEmpty()) {
        currentOwner = connectionPrivate()->getNameOwner(service); // verify the name owner
        if (currentOwner.isEmpty()) {
            lastError = connectionPrivate()->lastError;
        }
    }
}

bool QDBusAbstractInterfacePrivate::canMakeCalls() const
{
    // recheck only if we have a wildcard (i.e. empty) service or path
    // if any are empty, set the error message according to QDBusUtil
    if (service.isEmpty() && connectionPrivate()->mode != QDBusConnectionPrivate::PeerMode)
        return QDBusUtil::checkBusName(service, QDBusUtil::EmptyNotAllowed, &lastError);
    if (path.isEmpty())
        return QDBusUtil::checkObjectPath(path, QDBusUtil::EmptyNotAllowed, &lastError);
    return true;
}

bool QDBusAbstractInterfacePrivate::property(const QMetaProperty &mp, void *returnValuePtr) const
{
    if (!isValid || !canMakeCalls())   // can't make calls
        return false;

    const int type = mp.userType();
    // is this metatype registered?
    const char *expectedSignature = "";
    if (int(mp.type()) != QMetaType::QVariant) {
        expectedSignature = QDBusMetaType::typeToSignature(type);
        if (expectedSignature == 0) {
            qWarning("QDBusAbstractInterface: type %s must be registered with Qt D-Bus before it can be "
                     "used to read property %s.%s",
                     mp.typeName(), qPrintable(interface), mp.name());
            lastError = QDBusError(QDBusError::Failed,
                                   QString::fromLatin1("Unregistered type %1 cannot be handled")
                                   .arg(QLatin1String(mp.typeName())));
            return false;
        }
    }

    // try to read this property
    QDBusMessage msg = QDBusMessage::createMethodCall(service, path,
                                                      QLatin1String(DBUS_INTERFACE_PROPERTIES),
                                                      QLatin1String("Get"));
    QDBusMessagePrivate::setParametersValidated(msg, true);
    msg << interface << QString::fromUtf8(mp.name());
    QDBusMessage reply = connection.call(msg, QDBus::Block, timeout);

    if (reply.type() != QDBusMessage::ReplyMessage) {
        lastError = QDBusError(reply);
        return false;
    }
    if (reply.signature() != QLatin1String("v")) {
        QString errmsg = QLatin1String("Invalid signature `%1' in return from call to "
                                       DBUS_INTERFACE_PROPERTIES);
        lastError = QDBusError(QDBusError::InvalidSignature, errmsg.arg(reply.signature()));
        return false;
    }

    QByteArray foundSignature;
    const char *foundType = 0;
    QVariant value = qvariant_cast<QDBusVariant>(reply.arguments().at(0)).variant();

    if (value.userType() == type || type == QMetaType::QVariant
        || (expectedSignature[0] == 'v' && expectedSignature[1] == '\0')) {
        // simple match
        if (type == QMetaType::QVariant) {
            *reinterpret_cast<QVariant*>(returnValuePtr) = value;
        } else {
            QMetaType::destruct(type, returnValuePtr);
            QMetaType::construct(type, returnValuePtr, value.constData());
        }
        return true;
    }

    if (value.userType() == qMetaTypeId<QDBusArgument>()) {
        QDBusArgument arg = qvariant_cast<QDBusArgument>(value);

        foundType = "user type";
        foundSignature = arg.currentSignature().toLatin1();
        if (foundSignature == expectedSignature) {
            // signatures match, we can demarshall
            return QDBusMetaType::demarshall(arg, type, returnValuePtr);
        }
    } else {
        foundType = value.typeName();
        foundSignature = QDBusMetaType::typeToSignature(value.userType());
    }

    // there was an error...
    QString errmsg = QLatin1String("Unexpected `%1' (%2) when retrieving property `%3.%4' "
                                   "(expected type `%5' (%6))");
    lastError = QDBusError(QDBusError::InvalidSignature,
                           errmsg.arg(QString::fromLatin1(foundType),
                                      QString::fromLatin1(foundSignature),
                                      interface,
                                      QString::fromUtf8(mp.name()),
                                      QString::fromLatin1(mp.typeName()),
                                      QString::fromLatin1(expectedSignature)));
    return false;
}

bool QDBusAbstractInterfacePrivate::setProperty(const QMetaProperty &mp, const QVariant &value)
{
    if (!isValid || !canMakeCalls())    // can't make calls
        return false;

    // send the value
    QDBusMessage msg = QDBusMessage::createMethodCall(service, path,
                                                QLatin1String(DBUS_INTERFACE_PROPERTIES),
                                                QLatin1String("Set"));
    QDBusMessagePrivate::setParametersValidated(msg, true);
    msg << interface << QString::fromUtf8(mp.name()) << QVariant::fromValue(QDBusVariant(value));
    QDBusMessage reply = connection.call(msg, QDBus::Block, timeout);

    if (reply.type() != QDBusMessage::ReplyMessage) {
        lastError = QDBusError(reply);
        return false;
    }
    return true;
}

void QDBusAbstractInterfacePrivate::_q_serviceOwnerChanged(const QString &name,
                                                           const QString &oldOwner,
                                                           const QString &newOwner)
{
    Q_UNUSED(oldOwner);
    //qDebug() << "QDBusAbstractInterfacePrivate serviceOwnerChanged" << name << oldOwner << newOwner;
    if (name == service) {
        currentOwner = newOwner;
    }
}

QDBusAbstractInterfaceBase::QDBusAbstractInterfaceBase(QDBusAbstractInterfacePrivate &d, QObject *parent)
    : QObject(d, parent)
{
}

int QDBusAbstractInterfaceBase::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    int saved_id = _id;
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;

    if (_c == QMetaObject::ReadProperty || _c == QMetaObject::WriteProperty) {
        QMetaProperty mp = metaObject()->property(saved_id);
        int &status = *reinterpret_cast<int *>(_a[2]);

        if (_c == QMetaObject::WriteProperty) {
            QVariant value;
            if (mp.userType() == qMetaTypeId<QDBusVariant>())
                value = reinterpret_cast<const QDBusVariant*>(_a[0])->variant();
            else
                value = QVariant(mp.userType(), _a[0]);
            status = d_func()->setProperty(mp, value) ? 1 : 0;
        } else {
            bool readStatus = d_func()->property(mp, _a[0]);
            // Caller supports QVariant returns? Then we can also report errors
            // by storing an invalid variant.
            if (!readStatus && _a[1]) {
                status = 0;
                reinterpret_cast<QVariant*>(_a[1])->clear();
            }
        }
        _id = -1;
    }
    return _id;
}

/*!
    \class QDBusAbstractInterface
    \inmodule QtDBus
    \since 4.2

    \brief The QDBusAbstractInterface class is the base class for all D-Bus interfaces in the Qt D-Bus binding, allowing access to remote interfaces

    Generated-code classes also derive from QDBusAbstractInterface,
    all methods described here are also valid for generated-code
    classes. In addition to those described here, generated-code
    classes provide member functions for the remote methods, which
    allow for compile-time checking of the correct parameters and
    return values, as well as property type-matching and signal
    parameter-matching.

    \sa {qdbusxml2cpp.html}{The QDBus compiler}, QDBusInterface
*/

/*!
    \internal
    This is the constructor called from QDBusInterface::QDBusInterface.
*/
QDBusAbstractInterface::QDBusAbstractInterface(QDBusAbstractInterfacePrivate &d, QObject *parent)
    : QDBusAbstractInterfaceBase(d, parent)
{
    // keep track of the service owner
    if (d.isValid &&
        d.connection.isConnected()
        && !d.service.isEmpty()
        && !d.service.startsWith(QLatin1Char(':'))
        && d.connectionPrivate()->mode != QDBusConnectionPrivate::PeerMode)
        d_func()->connection.connect(QLatin1String(DBUS_SERVICE_DBUS), // service
                                     QString(), // path
                                     QLatin1String(DBUS_INTERFACE_DBUS), // interface
                                     QLatin1String("NameOwnerChanged"),
                                     QStringList() << d.service,
                                     QString(), // signature
                                     this, SLOT(_q_serviceOwnerChanged(QString,QString,QString)));
}

/*!
    \internal
    This is the constructor called from static classes derived from
    QDBusAbstractInterface (i.e., those generated by dbusxml2cpp).
*/
QDBusAbstractInterface::QDBusAbstractInterface(const QString &service, const QString &path,
                                               const char *interface, const QDBusConnection &con,
                                               QObject *parent)
    : QDBusAbstractInterfaceBase(*new QDBusAbstractInterfacePrivate(service, path, QString::fromLatin1(interface),
                                                 con, false), parent)
{
    // keep track of the service owner
    if (d_func()->isValid &&
        d_func()->connection.isConnected()
        && !service.isEmpty()
        && !service.startsWith(QLatin1Char(':'))
        && d_func()->connectionPrivate()->mode != QDBusConnectionPrivate::PeerMode)
        d_func()->connection.connect(QLatin1String(DBUS_SERVICE_DBUS), // service
                                     QString(), // path
                                     QLatin1String(DBUS_INTERFACE_DBUS), // interface
                                     QLatin1String("NameOwnerChanged"),
                                     QStringList() << service,
                                     QString(), //signature
                                     this, SLOT(_q_serviceOwnerChanged(QString,QString,QString)));
}

/*!
    Releases this object's resources.
*/
QDBusAbstractInterface::~QDBusAbstractInterface()
{
}

/*!
    Returns \c true if this is a valid reference to a remote object. It returns \c false if
    there was an error during the creation of this interface (for instance, if the remote
    application does not exist).

    Note: when dealing with remote objects, it is not always possible to determine if it
    exists when creating a QDBusInterface.
*/
bool QDBusAbstractInterface::isValid() const
{
    Q_D(const QDBusAbstractInterface);
    /* We don't retrieve the owner name for peer connections */
    if (d->connectionPrivate() && d->connectionPrivate()->mode == QDBusConnectionPrivate::PeerMode) {
        return d->isValid;
    } else {
        return !d->currentOwner.isEmpty();
    }
}

/*!
    Returns the connection this interface is assocated with.
*/
QDBusConnection QDBusAbstractInterface::connection() const
{
    return d_func()->connection;
}

/*!
    Returns the name of the service this interface is associated with.
*/
QString QDBusAbstractInterface::service() const
{
    return d_func()->service;
}

/*!
    Returns the object path that this interface is associated with.
*/
QString QDBusAbstractInterface::path() const
{
    return d_func()->path;
}

/*!
    Returns the name of this interface.
*/
QString QDBusAbstractInterface::interface() const
{
    return d_func()->interface;
}

/*!
    Returns the error the last operation produced, or an invalid error if the last operation did not
    produce an error.
*/
QDBusError QDBusAbstractInterface::lastError() const
{
    return d_func()->lastError;
}

/*!
    Sets the timeout in milliseconds for all future DBus calls to \a timeout.
    -1 means the default DBus timeout (usually 25 seconds).

    \since 4.8
*/
void QDBusAbstractInterface::setTimeout(int timeout)
{
    d_func()->timeout = timeout;
}

/*!
    Returns the current value of the timeout in milliseconds.
    -1 means the default DBus timeout (usually 25 seconds).

    \since 4.8
*/
int QDBusAbstractInterface::timeout() const
{
    return d_func()->timeout;
}

/*!
    Places a call to the remote method specified by \a method on this interface, using \a args as
    arguments. This function returns the message that was received as a reply, which can be a normal
    QDBusMessage::ReplyMessage (indicating success) or QDBusMessage::ErrorMessage (if the call
    failed). The \a mode parameter specifies how this call should be placed.

    If the call succeeds, lastError() will be cleared; otherwise, it will contain the error this
    call produced.

    Normally, you should place calls using call().

    \warning If you use \c UseEventLoop, your code must be prepared to deal with any reentrancy:
             other method calls and signals may be delivered before this function returns, as well
             as other Qt queued signals and events.

    \threadsafe
*/
QDBusMessage QDBusAbstractInterface::callWithArgumentList(QDBus::CallMode mode,
                                                          const QString& method,
                                                          const QList<QVariant>& args)
{
    Q_D(QDBusAbstractInterface);

    if (!d->isValid || !d->canMakeCalls())
        return QDBusMessage::createError(d->lastError);

    QString m = method;
    // split out the signature from the method
    int pos = method.indexOf(QLatin1Char('.'));
    if (pos != -1)
        m.truncate(pos);

    if (mode == QDBus::AutoDetect) {
        // determine if this a sync or async call
        mode = QDBus::Block;
        const QMetaObject *mo = metaObject();
        QByteArray match = m.toLatin1();

        for (int i = staticMetaObject.methodCount(); i < mo->methodCount(); ++i) {
            QMetaMethod mm = mo->method(i);
            if (mm.name() == match) {
                // found a method with the same name as what we're looking for
                // hopefully, nobody is overloading asynchronous and synchronous methods with
                // the same name

                QList<QByteArray> tags = QByteArray(mm.tag()).split(' ');
                if (tags.contains("Q_NOREPLY"))
                    mode = QDBus::NoBlock;

                break;
            }
        }
    }

//    qDebug() << "QDBusAbstractInterface" << "Service" << service() << "Path:" << path();
    QDBusMessage msg = QDBusMessage::createMethodCall(service(), path(), interface(), m);
    QDBusMessagePrivate::setParametersValidated(msg, true);
    msg.setArguments(args);

    QDBusMessage reply = d->connection.call(msg, mode, d->timeout);
    if (thread() == QThread::currentThread())
        d->lastError = QDBusError(reply);       // will clear if reply isn't an error

    // ensure that there is at least one element
    if (reply.arguments().isEmpty())
        reply << QVariant();

    return reply;
}

/*!
    \since 4.5
    Places a call to the remote method specified by \a method on this
    interface, using \a args as arguments. This function returns a
    QDBusPendingCall object that can be used to track the status of the
    reply and access its contents once it has arrived.

    Normally, you should place calls using asyncCall().

    \threadsafe
*/
QDBusPendingCall QDBusAbstractInterface::asyncCallWithArgumentList(const QString& method,
                                                                   const QList<QVariant>& args)
{
    Q_D(QDBusAbstractInterface);

    if (!d->isValid || !d->canMakeCalls())
        return QDBusPendingCall::fromError(d->lastError);

    QDBusMessage msg = QDBusMessage::createMethodCall(service(), path(), interface(), method);
    QDBusMessagePrivate::setParametersValidated(msg, true);
    msg.setArguments(args);
    return d->connection.asyncCall(msg, d->timeout);
}

/*!
    Places a call to the remote method specified by \a method
    on this interface, using \a args as arguments. This function
    returns immediately after queueing the call. The reply from
    the remote function is delivered to the \a returnMethod on
    object \a receiver. If an error occurs, the \a errorMethod
    on object \a receiver is called instead.

    This function returns \c true if the queueing succeeds. It does
    not indicate that the executed call succeeded. If it fails,
    the \a errorMethod is called. If the queueing failed, this
    function returns \c false and no slot will be called.

    The \a returnMethod must have as its parameters the types returned
    by the function call. Optionally, it may have a QDBusMessage
    parameter as its last or only parameter.  The \a errorMethod must
    have a QDBusError as its only parameter.

    \since 4.3
    \sa QDBusError, QDBusMessage
 */
bool QDBusAbstractInterface::callWithCallback(const QString &method,
                                              const QList<QVariant> &args,
                                              QObject *receiver,
                                              const char *returnMethod,
                                              const char *errorMethod)
{
    Q_D(QDBusAbstractInterface);

    if (!d->isValid || !d->canMakeCalls())
        return false;

    QDBusMessage msg = QDBusMessage::createMethodCall(service(),
                                                      path(),
                                                      interface(),
                                                      method);
    QDBusMessagePrivate::setParametersValidated(msg, true);
    msg.setArguments(args);

    d->lastError = QDBusError();
    return d->connection.callWithCallback(msg,
                                          receiver,
                                          returnMethod,
                                          errorMethod,
                                          d->timeout);
}

/*!
    \overload

    This function is deprecated. Please use the overloaded version.

    Places a call to the remote method specified by \a method
    on this interface, using \a args as arguments. This function
    returns immediately after queueing the call. The reply from
    the remote function or any errors emitted by it are delivered
    to the \a slot slot on object \a receiver.

    This function returns \c true if the queueing succeeded: it does
    not indicate that the call succeeded. If it failed, the slot
    will be called with an error message. lastError() will not be
    set under those circumstances.

    \sa QDBusError, QDBusMessage
*/
bool QDBusAbstractInterface::callWithCallback(const QString &method,
                                              const QList<QVariant> &args,
                                              QObject *receiver,
                                              const char *slot)
{
    return callWithCallback(method, args, receiver, slot, 0);
}

/*!
    \internal
    Catch signal connections.
*/
void QDBusAbstractInterface::connectNotify(const QMetaMethod &signal)
{
    // someone connecting to one of our signals
    Q_D(QDBusAbstractInterface);
    if (!d->isValid)
        return;

    // we end up recursing here, so optimize away
    static const QMetaMethod destroyedSignal = QMetaMethod::fromSignal(&QDBusAbstractInterface::destroyed);
    if (signal == destroyedSignal)
        return;

    QDBusConnectionPrivate *conn = d->connectionPrivate();
    if (conn) {
        conn->connectRelay(d->service, d->path, d->interface,
                           this, signal);
    }
}

/*!
    \internal
    Catch signal disconnections.
*/
void QDBusAbstractInterface::disconnectNotify(const QMetaMethod &signal)
{
    // someone disconnecting from one of our signals
    Q_D(QDBusAbstractInterface);
    if (!d->isValid)
        return;

    QDBusConnectionPrivate *conn = d->connectionPrivate();
    if (conn && signal.isValid() && !isSignalConnected(signal))
        return conn->disconnectRelay(d->service, d->path, d->interface,
                                     this, signal);
    if (!conn)
        return;

    // wildcard disconnecting, we need to figure out which of our signals are
    // no longer connected to anything
    const QMetaObject *mo = metaObject();
    int midx = QObject::staticMetaObject.methodCount();
    const int end = mo->methodCount();
    for ( ; midx < end; ++midx) {
        QMetaMethod mm = mo->method(midx);
        if (mm.methodType() == QMetaMethod::Signal && !isSignalConnected(mm))
            conn->disconnectRelay(d->service, d->path, d->interface, this, mm);
    }
}

/*!
    \internal
    Get the value of the property \a propname.
*/
QVariant QDBusAbstractInterface::internalPropGet(const char *propname) const
{
    // assume this property exists and is readable
    // we're only called from generated code anyways

    return property(propname);
}

/*!
    \internal
    Set the value of the property \a propname to \a value.
*/
void QDBusAbstractInterface::internalPropSet(const char *propname, const QVariant &value)
{
    setProperty(propname, value);
}

/*!
    Calls the method \a method on this interface and passes the parameters to this function to the
    method.

    The parameters to \c call are passed on to the remote function via D-Bus as input
    arguments. Output arguments are returned in the QDBusMessage reply. If the reply is an error
    reply, lastError() will also be set to the contents of the error message.

    This function can be used with up to 8 parameters, passed in arguments \a arg1, \a arg2,
    \a arg3, \a arg4, \a arg5, \a arg6, \a arg7 and \a arg8. If you need more than 8
    parameters or if you have a variable number of parameters to be passed, use
    callWithArgumentList().

    It can be used the following way:

    \snippet code/src_qdbus_qdbusabstractinterface.cpp 0

    This example illustrates function calling with 0, 1 and 2 parameters and illustrates different
    parameter types passed in each (the first call to \c "ProcessWorkUnicode" will contain one
    Unicode string, the second call to \c "ProcessWork" will contain one string and one byte array).
*/
QDBusMessage QDBusAbstractInterface::call(const QString &method, const QVariant &arg1,
                                          const QVariant &arg2,
                                          const QVariant &arg3,
                                          const QVariant &arg4,
                                          const QVariant &arg5,
                                          const QVariant &arg6,
                                          const QVariant &arg7,
                                          const QVariant &arg8)
{
    return call(QDBus::AutoDetect, method, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8);
}

/*!
    \overload

    Calls the method \a method on this interface and passes the
    parameters to this function to the method. If \a mode is \c
    NoWaitForReply, then this function will return immediately after
    placing the call, without waiting for a reply from the remote
    method. Otherwise, \a mode indicates whether this function should
    activate the Qt Event Loop while waiting for the reply to arrive.

    This function can be used with up to 8 parameters, passed in arguments \a arg1, \a arg2,
    \a arg3, \a arg4, \a arg5, \a arg6, \a arg7 and \a arg8. If you need more than 8
    parameters or if you have a variable number of parameters to be passed, use
    callWithArgumentList().

    If this function reenters the Qt event loop in order to wait for the
    reply, it will exclude user input. During the wait, it may deliver
    signals and other method calls to your application. Therefore, it
    must be prepared to handle a reentrancy whenever a call is placed
    with call().
*/
QDBusMessage QDBusAbstractInterface::call(QDBus::CallMode mode, const QString &method,
                                          const QVariant &arg1,
                                          const QVariant &arg2,
                                          const QVariant &arg3,
                                          const QVariant &arg4,
                                          const QVariant &arg5,
                                          const QVariant &arg6,
                                          const QVariant &arg7,
                                          const QVariant &arg8)
{
    QList<QVariant> argList;
    int count = 0 + arg1.isValid() + arg2.isValid() + arg3.isValid() + arg4.isValid() +
                arg5.isValid() + arg6.isValid() + arg7.isValid() + arg8.isValid();

    switch (count) {
    case 8:
        argList.prepend(arg8);
    case 7:
        argList.prepend(arg7);
    case 6:
        argList.prepend(arg6);
    case 5:
        argList.prepend(arg5);
    case 4:
        argList.prepend(arg4);
    case 3:
        argList.prepend(arg3);
    case 2:
        argList.prepend(arg2);
    case 1:
        argList.prepend(arg1);
    }

    return callWithArgumentList(mode, method, argList);
}


/*!
    \since 4.5
    Calls the method \a method on this interface and passes the parameters to this function to the
    method.

    The parameters to \c call are passed on to the remote function via D-Bus as input
    arguments. The returned QDBusPendingCall object can be used to find out information about
    the reply.

    This function can be used with up to 8 parameters, passed in arguments \a arg1, \a arg2,
    \a arg3, \a arg4, \a arg5, \a arg6, \a arg7 and \a arg8. If you need more than 8
    parameters or if you have a variable number of parameters to be passed, use
    asyncCallWithArgumentList().

    It can be used the following way:

    \snippet code/src_qdbus_qdbusabstractinterface.cpp 1

    This example illustrates function calling with 0, 1 and 2 parameters and illustrates different
    parameter types passed in each (the first call to \c "ProcessWorkUnicode" will contain one
    Unicode string, the second call to \c "ProcessWork" will contain one string and one byte array).
*/
QDBusPendingCall QDBusAbstractInterface::asyncCall(const QString &method, const QVariant &arg1,
                                                   const QVariant &arg2,
                                                   const QVariant &arg3,
                                                   const QVariant &arg4,
                                                   const QVariant &arg5,
                                                   const QVariant &arg6,
                                                   const QVariant &arg7,
                                                   const QVariant &arg8)
{
    QList<QVariant> argList;
    int count = 0 + arg1.isValid() + arg2.isValid() + arg3.isValid() + arg4.isValid() +
                arg5.isValid() + arg6.isValid() + arg7.isValid() + arg8.isValid();

    switch (count) {
    case 8:
        argList.prepend(arg8);
    case 7:
        argList.prepend(arg7);
    case 6:
        argList.prepend(arg6);
    case 5:
        argList.prepend(arg5);
    case 4:
        argList.prepend(arg4);
    case 3:
        argList.prepend(arg3);
    case 2:
        argList.prepend(arg2);
    case 1:
        argList.prepend(arg1);
    }

    return asyncCallWithArgumentList(method, argList);
}

/*!
    \internal
*/
QDBusMessage QDBusAbstractInterface::internalConstCall(QDBus::CallMode mode,
                                                       const QString &method,
                                                       const QList<QVariant> &args) const
{
    // ### move the code here, and make the other functions call this
    return const_cast<QDBusAbstractInterface*>(this)->callWithArgumentList(mode, method, args);
}

QT_END_NAMESPACE

#endif // QT_NO_DBUS

#include "moc_qdbusabstractinterface.cpp"
