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

#include "qdbusmetatype.h"
#include "qdbusmetatype_p.h"

#include <string.h>
#include "qdbus_symbols_p.h"

#include <qbytearray.h>
#include <qglobal.h>
#include <qreadwritelock.h>
#include <qvector.h>

#include "qdbusargument_p.h"
#include "qdbusutil_p.h"
#include "qdbusunixfiledescriptor.h"
#ifndef QT_BOOTSTRAPPED
#include "qdbusmessage.h"
#endif

#ifndef QT_NO_DBUS

#ifndef DBUS_TYPE_UNIX_FD
# define DBUS_TYPE_UNIX_FD int('h')
# define DBUS_TYPE_UNIX_FD_AS_STRING "h"
#endif

QT_BEGIN_NAMESPACE

class QDBusCustomTypeInfo
{
public:
    QDBusCustomTypeInfo() : signature(), marshall(0), demarshall(0)
    { }

    // Suggestion:
    // change 'signature' to char* and make QDBusCustomTypeInfo a Movable type
    QByteArray signature;
    QDBusMetaType::MarshallFunction marshall;
    QDBusMetaType::DemarshallFunction demarshall;
};

template<typename T>
inline static void registerHelper(T * = 0)
{
    void (*mf)(QDBusArgument &, const T *) = qDBusMarshallHelper<T>;
    void (*df)(const QDBusArgument &, T *) = qDBusDemarshallHelper<T>;
    QDBusMetaType::registerMarshallOperators(qMetaTypeId<T>(),
        reinterpret_cast<QDBusMetaType::MarshallFunction>(mf),
        reinterpret_cast<QDBusMetaType::DemarshallFunction>(df));
}

void QDBusMetaTypeId::init()
{
    static QBasicAtomicInt initialized = Q_BASIC_ATOMIC_INITIALIZER(false);

    // reentrancy is not a problem since everything else is locked on their own
    // set the guard variable at the end
    if (!initialized.load()) {
        // register our types with Qt Core (calling qMetaTypeId<T>() does this implicitly)
        (void)message();
        (void)argument();
        (void)variant();
        (void)objectpath();
        (void)signature();
        (void)error();
        (void)unixfd();

#ifndef QDBUS_NO_SPECIALTYPES
        // and register Qt Core's with us
        registerHelper<QDate>();
        registerHelper<QTime>();
        registerHelper<QDateTime>();
        registerHelper<QRect>();
        registerHelper<QRectF>();
        registerHelper<QSize>();
        registerHelper<QSizeF>();
        registerHelper<QPoint>();
        registerHelper<QPointF>();
        registerHelper<QLine>();
        registerHelper<QLineF>();
        registerHelper<QVariantList>();
        registerHelper<QVariantMap>();
        registerHelper<QVariantHash>();

        qDBusRegisterMetaType<QList<bool> >();
        qDBusRegisterMetaType<QList<short> >();
        qDBusRegisterMetaType<QList<ushort> >();
        qDBusRegisterMetaType<QList<int> >();
        qDBusRegisterMetaType<QList<uint> >();
        qDBusRegisterMetaType<QList<qlonglong> >();
        qDBusRegisterMetaType<QList<qulonglong> >();
        qDBusRegisterMetaType<QList<double> >();
        qDBusRegisterMetaType<QList<QDBusObjectPath> >();
        qDBusRegisterMetaType<QList<QDBusSignature> >();
        qDBusRegisterMetaType<QList<QDBusUnixFileDescriptor> >();
#endif

        initialized.store(true);
    }
}

Q_GLOBAL_STATIC(QVector<QDBusCustomTypeInfo>, customTypes)
Q_GLOBAL_STATIC(QReadWriteLock, customTypesLock)

/*!
    \class QDBusMetaType
    \inmodule QtDBus
    \brief Meta-type registration system for the Qt D-Bus module.
    \internal

    The QDBusMetaType class allows you to register class types for
    marshalling and demarshalling over D-Bus. D-Bus supports a very
    limited set of primitive types, but allows one to extend the type
    system by creating compound types, such as arrays (lists) and
    structs. In order to use them with Qt D-Bus, those types must be
    registered.

    See \l {qdbustypesystem.html}{Qt D-Bus Type System} for more
    information on the type system and how to register additional
    types.

    \sa {qdbustypesystem.html}{Qt D-Bus Type System},
    qDBusRegisterMetaType(), QMetaType, QVariant, QDBusArgument
*/

/*!
    \fn int qDBusRegisterMetaType()
    \relates QDBusArgument
    \threadsafe
    \since 4.2

    Registers \c{T} with the
    \l {qdbustypesystem.html}{Qt D-Bus Type System} and the Qt \l
    {QMetaType}{meta-type system}, if it's not already registered.

    To register a type, it must be declared as a meta-type with the
    Q_DECLARE_METATYPE() macro, and then registered as in the
    following example:

    \snippet code/src_qdbus_qdbusmetatype.cpp 0

    If \c{T} isn't a type derived from one of
    Qt's \l{container classes}, the \c{operator<<} and
    \c{operator>>} streaming operators between \c{T} and QDBusArgument
    must be already declared. See the \l {qdbustypesystem.html}{Qt D-Bus
    Type System} page for more information on how to declare such
    types.

    This function returns the Qt meta type id for the type (the same
    value that is returned from qRegisterMetaType()).

    \sa {qdbustypesystem.html}{Qt D-Bus Type System}, qRegisterMetaType(), QMetaType
*/

/*!
    \typedef QDBusMetaType::MarshallFunction
    \internal
*/

/*!
    \typedef QDBusMetaType::DemarshallFunction
    \internal
*/

/*!
    \internal
    Registers the marshalling and demarshalling functions for meta
    type \a id.
*/
void QDBusMetaType::registerMarshallOperators(int id, MarshallFunction mf,
                                              DemarshallFunction df)
{
    QByteArray var;
    QVector<QDBusCustomTypeInfo> *ct = customTypes();
    if (id < 0 || !mf || !df || !ct)
        return;                 // error!

    QWriteLocker locker(customTypesLock());
    if (id >= ct->size())
        ct->resize(id + 1);
    QDBusCustomTypeInfo &info = (*ct)[id];
    info.marshall = mf;
    info.demarshall = df;
}

/*!
    \internal
    Executes the marshalling of type \a id (whose data is contained in
    \a data) to the D-Bus marshalling argument \a arg. Returns \c true if
    the marshalling succeeded, or false if an error occurred.
*/
bool QDBusMetaType::marshall(QDBusArgument &arg, int id, const void *data)
{
    QDBusMetaTypeId::init();

    MarshallFunction mf;
    {
        QReadLocker locker(customTypesLock());
        QVector<QDBusCustomTypeInfo> *ct = customTypes();
        if (id >= ct->size())
            return false;       // non-existent

        const QDBusCustomTypeInfo &info = (*ct).at(id);
        if (!info.marshall) {
            mf = 0;             // make gcc happy
            return false;
        } else
            mf = info.marshall;
    }

    mf(arg, data);
    return true;
}

/*!
    \internal
    Executes the demarshalling of type \a id (whose data will be placed in
    \a data) from the D-Bus marshalling argument \a arg. Returns \c true if
    the demarshalling succeeded, or false if an error occurred.
*/
bool QDBusMetaType::demarshall(const QDBusArgument &arg, int id, void *data)
{
    QDBusMetaTypeId::init();

    DemarshallFunction df;
    {
        QReadLocker locker(customTypesLock());
        QVector<QDBusCustomTypeInfo> *ct = customTypes();
        if (id >= ct->size())
            return false;       // non-existent

        const QDBusCustomTypeInfo &info = (*ct).at(id);
        if (!info.demarshall) {
            df = 0;             // make gcc happy
            return false;
        } else
            df = info.demarshall;
    }
#ifndef QT_BOOTSTRAPPED
    QDBusArgument copy = arg;
    df(copy, data);
#else
    Q_UNUSED(arg);
    Q_UNUSED(data);
    Q_UNUSED(df);
#endif
    return true;
}

/*!
    \fn QDBusMetaType::signatureToType(const char *signature)
    \internal

    Returns the Qt meta type id for the given D-Bus signature for exactly one full type, given
    by \a signature.

    Note: this function only handles the basic D-Bus types.

    \sa QDBusUtil::isValidSingleSignature(), typeToSignature(),
        QVariant::type(), QVariant::userType()
*/
int QDBusMetaType::signatureToType(const char *signature)
{
    if (!signature)
        return QMetaType::UnknownType;

    QDBusMetaTypeId::init();
    switch (signature[0])
    {
    case DBUS_TYPE_BOOLEAN:
        return QVariant::Bool;

    case DBUS_TYPE_BYTE:
        return QMetaType::UChar;

    case DBUS_TYPE_INT16:
        return QMetaType::Short;

    case DBUS_TYPE_UINT16:
        return QMetaType::UShort;

    case DBUS_TYPE_INT32:
        return QVariant::Int;

    case DBUS_TYPE_UINT32:
        return QVariant::UInt;

    case DBUS_TYPE_INT64:
        return QVariant::LongLong;

    case DBUS_TYPE_UINT64:
        return QVariant::ULongLong;

    case DBUS_TYPE_DOUBLE:
        return QVariant::Double;

    case DBUS_TYPE_STRING:
        return QVariant::String;

    case DBUS_TYPE_OBJECT_PATH:
        return QDBusMetaTypeId::objectpath();

    case DBUS_TYPE_SIGNATURE:
        return QDBusMetaTypeId::signature();

    case DBUS_TYPE_UNIX_FD:
        return QDBusMetaTypeId::unixfd();

    case DBUS_TYPE_VARIANT:
        return QDBusMetaTypeId::variant();

    case DBUS_TYPE_ARRAY:       // special case
        switch (signature[1]) {
        case DBUS_TYPE_BYTE:
            return QVariant::ByteArray;

        case DBUS_TYPE_STRING:
            return QVariant::StringList;

        case DBUS_TYPE_VARIANT:
            return QVariant::List;

        case DBUS_TYPE_OBJECT_PATH:
            return qMetaTypeId<QList<QDBusObjectPath> >();

        case DBUS_TYPE_SIGNATURE:
            return qMetaTypeId<QList<QDBusSignature> >();

        }
        // fall through
    default:
        return QMetaType::UnknownType;
    }
}

/*!
    \fn QDBusMetaType::typeToSignature(int type)
    \internal

    Returns the D-Bus signature equivalent to the supplied meta type id \a type.

    More types can be registered with the qDBusRegisterMetaType() function.

    \sa QDBusUtil::isValidSingleSignature(), signatureToType(),
        QVariant::type(), QVariant::userType()
*/
const char *QDBusMetaType::typeToSignature(int type)
{
    // check if it's a static type
    switch (type)
    {
    case QMetaType::UChar:
        return DBUS_TYPE_BYTE_AS_STRING;

    case QVariant::Bool:
        return DBUS_TYPE_BOOLEAN_AS_STRING;

    case QMetaType::Short:
        return DBUS_TYPE_INT16_AS_STRING;

    case QMetaType::UShort:
        return DBUS_TYPE_UINT16_AS_STRING;

    case QVariant::Int:
        return DBUS_TYPE_INT32_AS_STRING;

    case QVariant::UInt:
        return DBUS_TYPE_UINT32_AS_STRING;

    case QVariant::LongLong:
        return DBUS_TYPE_INT64_AS_STRING;

    case QVariant::ULongLong:
        return DBUS_TYPE_UINT64_AS_STRING;

    case QVariant::Double:
        return DBUS_TYPE_DOUBLE_AS_STRING;

    case QVariant::String:
        return DBUS_TYPE_STRING_AS_STRING;

    case QVariant::StringList:
        return DBUS_TYPE_ARRAY_AS_STRING
            DBUS_TYPE_STRING_AS_STRING; // as

    case QVariant::ByteArray:
        return DBUS_TYPE_ARRAY_AS_STRING
            DBUS_TYPE_BYTE_AS_STRING; // ay
    }

    QDBusMetaTypeId::init();
    if (type == QDBusMetaTypeId::variant())
        return DBUS_TYPE_VARIANT_AS_STRING;
    else if (type == QDBusMetaTypeId::objectpath())
        return DBUS_TYPE_OBJECT_PATH_AS_STRING;
    else if (type == QDBusMetaTypeId::signature())
        return DBUS_TYPE_SIGNATURE_AS_STRING;
    else if (type == QDBusMetaTypeId::unixfd())
        return DBUS_TYPE_UNIX_FD_AS_STRING;

    // try the database
    QVector<QDBusCustomTypeInfo> *ct = customTypes();
    {
        QReadLocker locker(customTypesLock());
        if (type >= ct->size())
            return 0;           // type not registered with us

        const QDBusCustomTypeInfo &info = (*ct).at(type);

        if (!info.signature.isNull())
            return info.signature;

        if (!info.marshall)
            return 0;           // type not registered with us
    }

    // call to user code to construct the signature type
    QDBusCustomTypeInfo *info;
    {
        // createSignature will never return a null QByteArray
        // if there was an error, it'll return ""
        QByteArray signature = QDBusArgumentPrivate::createSignature(type);

        // re-acquire lock
        QWriteLocker locker(customTypesLock());
        info = &(*ct)[type];
        info->signature = signature;
    }
    return info->signature;
}

QT_END_NAMESPACE

#endif // QT_NO_DBUS
