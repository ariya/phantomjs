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

#include "qdbusinterface.h"
#include "qdbusinterface_p.h"

#include "qdbus_symbols_p.h"
#include <QtCore/qpointer.h>
#include <QtCore/qstringlist.h>

#include "qdbusmetatype_p.h"
#include "qdbusconnection_p.h"

#ifndef QT_NO_DBUS

QT_BEGIN_NAMESPACE

static void copyArgument(void *to, int id, const QVariant &arg)
{
    if (id == arg.userType()) {
        switch (id) {
        case QVariant::Bool:
            *reinterpret_cast<bool *>(to) = arg.toBool();
            return;

        case QMetaType::UChar:
            *reinterpret_cast<uchar *>(to) = arg.value<uchar>();
            return;

        case QMetaType::Short:
            *reinterpret_cast<short *>(to) = arg.value<short>();
            return;

        case QMetaType::UShort:
            *reinterpret_cast<ushort *>(to) = arg.value<ushort>();
            return;

        case QVariant::Int:
            *reinterpret_cast<int *>(to) = arg.toInt();
            return;

        case QVariant::UInt:
            *reinterpret_cast<uint *>(to) = arg.toUInt();
            return;

        case QVariant::LongLong:
            *reinterpret_cast<qlonglong *>(to) = arg.toLongLong();
            return;

        case QVariant::ULongLong:
            *reinterpret_cast<qulonglong *>(to) = arg.toULongLong();
            return;

        case QVariant::Double:
            *reinterpret_cast<double *>(to) = arg.toDouble();
            return;

        case QVariant::String:
            *reinterpret_cast<QString *>(to) = arg.toString();
            return;

        case QVariant::ByteArray:
            *reinterpret_cast<QByteArray *>(to) = arg.toByteArray();
            return;

        case QVariant::StringList:
            *reinterpret_cast<QStringList *>(to) = arg.toStringList();
            return;
        }

        if (id == QDBusMetaTypeId::variant()) {
            *reinterpret_cast<QDBusVariant *>(to) = arg.value<QDBusVariant>();
            return;
        } else if (id == QDBusMetaTypeId::objectpath()) {
            *reinterpret_cast<QDBusObjectPath *>(to) = arg.value<QDBusObjectPath>();
            return;
        } else if (id == QDBusMetaTypeId::signature()) {
            *reinterpret_cast<QDBusSignature *>(to) = arg.value<QDBusSignature>();
            return;
        }

        // those above are the only types possible
        // the demarshaller code doesn't demarshall anything else
        qFatal("Found a decoded basic type in a D-Bus reply that shouldn't be there");
    }

    // if we got here, it's either an un-dermarshalled type or a mismatch
    if (arg.userType() != QDBusMetaTypeId::argument()) {
        // it's a mismatch
        //qWarning?
        return;
    }

    // is this type registered?
    const char *userSignature = QDBusMetaType::typeToSignature(id);
    if (!userSignature || !*userSignature) {
        // type not registered
        //qWarning?
        return;
    }

    // is it the same signature?
    QDBusArgument dbarg = arg.value<QDBusArgument>();
    if (dbarg.currentSignature() != QLatin1String(userSignature)) {
        // not the same signature, another mismatch
        //qWarning?
        return;
    }

    // we can demarshall
    QDBusMetaType::demarshall(dbarg, id, to);
}

QDBusInterfacePrivate::QDBusInterfacePrivate(const QString &serv, const QString &p,
                                             const QString &iface, const QDBusConnection &con)
    : QDBusAbstractInterfacePrivate(serv, p, iface, con, true), metaObject(0)
{
    // QDBusAbstractInterfacePrivate's constructor checked the parameters for us
    if (connection.isConnected()) {
        metaObject = connectionPrivate()->findMetaObject(service, path, interface, lastError);

        if (!metaObject) {
            // creation failed, somehow
            // most common causes are that the service doesn't exist or doesn't support introspection
            // those are not fatal errors, so we continue working

            if (!lastError.isValid())
                lastError = QDBusError(QDBusError::InternalError, QLatin1String("Unknown error"));
        }
    }
}

QDBusInterfacePrivate::~QDBusInterfacePrivate()
{
    if (metaObject && !metaObject->cached)
        delete metaObject;
}


/*!
    \class QDBusInterface
    \inmodule QtDBus
    \since 4.2

    \brief The QDBusInterface class is a proxy for interfaces on remote objects.

    QDBusInterface is a generic accessor class that is used to place calls to remote objects,
    connect to signals exported by remote objects and get/set the value of remote properties. This
    class is useful for dynamic access to remote objects: that is, when you do not have a generated
    code that represents the remote interface.

    Calls are usually placed by using the call() function, which constructs the message, sends it
    over the bus, waits for the reply and decodes the reply. Signals are connected to by using the
    normal QObject::connect() function. Finally, properties are accessed using the
    QObject::property() and QObject::setProperty() functions.

    The following code snippet demonstrates how to perform a
    mathematical operation of \tt{"2 + 2"} in a remote application
    called \c com.example.Calculator, accessed via the session bus.

    \snippet code/src_qdbus_qdbusinterface.cpp 0

    \sa {Qt D-Bus XML compiler (qdbusxml2cpp)}
*/

/*!
    Creates a dynamic QDBusInterface object associated with the
    interface \a interface on object at path \a path on service \a
    service, using the given \a connection. If \a interface is an
    empty string, the object created will refer to the merging of all
    interfaces found in that object.

    \a parent is passed to the base class constructor.

    If the remote service \a service is not present or if an error
    occurs trying to obtain the description of the remote interface
    \a interface, the object created will not be valid (see
    isValid()).
*/
QDBusInterface::QDBusInterface(const QString &service, const QString &path, const QString &interface,
                               const QDBusConnection &connection, QObject *parent)
    : QDBusAbstractInterface(*new QDBusInterfacePrivate(service, path, interface, connection),
                             parent)
{
}

/*!
    Destroy the object interface and frees up any resource used.
*/
QDBusInterface::~QDBusInterface()
{
    // resources are freed in QDBusInterfacePrivate::~QDBusInterfacePrivate()
}

/*!
    \internal
    Overrides QObject::metaObject to return our own copy.
*/
const QMetaObject *QDBusInterface::metaObject() const
{
    return d_func()->metaObject ? d_func()->metaObject : &QDBusAbstractInterface::staticMetaObject;
}

/*!
    \internal
    Override QObject::qt_metacast to catch the interface name too.
*/
void *QDBusInterface::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, "QDBusInterface"))
        return static_cast<void*>(const_cast<QDBusInterface*>(this));
    if (d_func()->interface.toLatin1() == _clname)
        return static_cast<void*>(const_cast<QDBusInterface*>(this));
    return QDBusAbstractInterface::qt_metacast(_clname);
}

/*!
    \internal
    Dispatch the call through the private.
*/
int QDBusInterface::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QDBusAbstractInterface::qt_metacall(_c, _id, _a);
    if (_id < 0 || !d_func()->isValid || !d_func()->metaObject)
        return _id;
    return d_func()->metacall(_c, _id, _a);
}

int QDBusInterfacePrivate::metacall(QMetaObject::Call c, int id, void **argv)
{
    Q_Q(QDBusInterface);

    if (c == QMetaObject::InvokeMetaMethod) {
        int offset = metaObject->methodOffset();
        QMetaMethod mm = metaObject->method(id + offset);

        if (mm.methodType() == QMetaMethod::Signal) {
            // signal relay from D-Bus world to Qt world
            QMetaObject::activate(q, metaObject, id, argv);

        } else if (mm.methodType() == QMetaMethod::Slot || mm.methodType() == QMetaMethod::Method) {
            // method call relay from Qt world to D-Bus world
            // get D-Bus equivalent signature
            QString methodName = QString::fromLatin1(mm.name());
            const int *inputTypes = metaObject->inputTypesForMethod(id);
            int inputTypesCount = *inputTypes;

            // we will assume that the input arguments were passed correctly
            QVariantList args;
            int i = 1;
            for ( ; i <= inputTypesCount; ++i)
                args << QVariant(inputTypes[i], argv[i]);

            // make the call
            QDBusMessage reply = q->callWithArgumentList(QDBus::Block, methodName, args);

            if (reply.type() == QDBusMessage::ReplyMessage) {
                // attempt to demarshall the return values
                args = reply.arguments();
                QVariantList::ConstIterator it = args.constBegin();
                const int *outputTypes = metaObject->outputTypesForMethod(id);
                int outputTypesCount = *outputTypes++;

                if (mm.returnType() != QMetaType::UnknownType && mm.returnType() != QMetaType::Void) {
                    // this method has a return type
                    if (argv[0] && it != args.constEnd())
                        copyArgument(argv[0], *outputTypes++, *it);

                    // skip this argument even if we didn't copy it
                    --outputTypesCount;
                    ++it;
                }

                for (int j = 0; j < outputTypesCount && it != args.constEnd(); ++i, ++j, ++it) {
                    copyArgument(argv[i], outputTypes[j], *it);
                }
            }

            // done
            lastError = QDBusError(reply);
            return -1;
        }
    }
    return id;
}

QT_END_NAMESPACE

#endif // QT_NO_DBUS
