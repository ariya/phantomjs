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

#include "qdbusargument_p.h"
#include "qdbusconnection.h"
#include <stdlib.h>

QT_BEGIN_NAMESPACE

template <typename T>
static inline T qIterGet(DBusMessageIter *it)
{
    // Use a union of expected and largest type q_dbus_message_iter_get_basic
    // will return to ensure reading the wrong basic type does not result in
    // stack overwrite
    union {
        // The value to be extracted
        T t;
        // Largest type that q_dbus_message_iter_get_basic will return
        // according to dbus_message_iter_get_basic API documentation
        dbus_uint64_t maxValue;
        // A pointer to ensure no stack overwrite in case there is a platform
        // where sizeof(void*) > sizeof(dbus_uint64_t)
        void* ptr;
    } value;

    // Initialize the value in case a narrower type is extracted to it.
    // Note that the result of extracting a narrower type in place of a wider
    // one and vice-versa will be platform-dependent.
    value.t = T();

    q_dbus_message_iter_get_basic(it, &value);
    q_dbus_message_iter_next(it);
    return value.t;
}

QDBusDemarshaller::~QDBusDemarshaller()
{
}

inline QString QDBusDemarshaller::currentSignature()
{
    char *sig = q_dbus_message_iter_get_signature(&iterator);
    QString retval = QString::fromUtf8(sig);
    q_dbus_free(sig);

    return retval;
}

inline uchar QDBusDemarshaller::toByte()
{
    return qIterGet<uchar>(&iterator);
}

inline bool QDBusDemarshaller::toBool()
{
    return bool(qIterGet<dbus_bool_t>(&iterator));
}

inline ushort QDBusDemarshaller::toUShort()
{
    return qIterGet<dbus_uint16_t>(&iterator);
}

inline short QDBusDemarshaller::toShort()
{
    return qIterGet<dbus_int16_t>(&iterator);
}

inline int QDBusDemarshaller::toInt()
{
    return qIterGet<dbus_int32_t>(&iterator);
}

inline uint QDBusDemarshaller::toUInt()
{
    return qIterGet<dbus_uint32_t>(&iterator);
}

inline qlonglong QDBusDemarshaller::toLongLong()
{
    return qIterGet<qlonglong>(&iterator);
}

inline qulonglong QDBusDemarshaller::toULongLong()
{
    return qIterGet<qulonglong>(&iterator);
}

inline double QDBusDemarshaller::toDouble()
{
    return qIterGet<double>(&iterator);
}

inline QString QDBusDemarshaller::toStringUnchecked()
{
    return QString::fromUtf8(qIterGet<char *>(&iterator));
}

inline QString QDBusDemarshaller::toString()
{
    if (isCurrentTypeStringLike())
        return toStringUnchecked();
    else
        return QString();
}

inline QDBusObjectPath QDBusDemarshaller::toObjectPathUnchecked()
 {
     return QDBusObjectPath(QString::fromUtf8(qIterGet<char *>(&iterator)));
 }

inline QDBusObjectPath QDBusDemarshaller::toObjectPath()
{
    if (isCurrentTypeStringLike())
        return toObjectPathUnchecked();
    else
        return QDBusObjectPath();
}

inline QDBusSignature QDBusDemarshaller::toSignatureUnchecked()
 {
     return QDBusSignature(QString::fromUtf8(qIterGet<char *>(&iterator)));
 }

inline QDBusSignature QDBusDemarshaller::toSignature()
{
    if (isCurrentTypeStringLike())
        return toSignatureUnchecked();
    else
        return QDBusSignature();
}

inline QDBusUnixFileDescriptor QDBusDemarshaller::toUnixFileDescriptor()
{
    QDBusUnixFileDescriptor fd;
    fd.giveFileDescriptor(qIterGet<dbus_int32_t>(&iterator));
    return fd;
}

inline QDBusVariant QDBusDemarshaller::toVariant()
{
    QDBusDemarshaller sub(capabilities);
    sub.message = q_dbus_message_ref(message);
    q_dbus_message_iter_recurse(&iterator, &sub.iterator);
    q_dbus_message_iter_next(&iterator);

    return QDBusVariant( sub.toVariantInternal() );
}

QDBusArgument::ElementType QDBusDemarshaller::currentType()
{
    switch (q_dbus_message_iter_get_arg_type(&iterator)) {
    case DBUS_TYPE_BYTE:
    case DBUS_TYPE_INT16:
    case DBUS_TYPE_UINT16:
    case DBUS_TYPE_INT32:
    case DBUS_TYPE_UINT32:
    case DBUS_TYPE_INT64:
    case DBUS_TYPE_UINT64:
    case DBUS_TYPE_BOOLEAN:
    case DBUS_TYPE_DOUBLE:
    case DBUS_TYPE_STRING:
    case DBUS_TYPE_OBJECT_PATH:
    case DBUS_TYPE_SIGNATURE:
        return QDBusArgument::BasicType;

    case DBUS_TYPE_VARIANT:
        return QDBusArgument::VariantType;

    case DBUS_TYPE_ARRAY:
        switch (q_dbus_message_iter_get_element_type(&iterator)) {
        case DBUS_TYPE_BYTE:
        case DBUS_TYPE_STRING:
            // QByteArray and QStringList
            return QDBusArgument::BasicType;
        case DBUS_TYPE_DICT_ENTRY:
            return QDBusArgument::MapType;
        default:
            return QDBusArgument::ArrayType;
        }

    case DBUS_TYPE_STRUCT:
        return QDBusArgument::StructureType;
    case DBUS_TYPE_DICT_ENTRY:
        return QDBusArgument::MapEntryType;

    case DBUS_TYPE_UNIX_FD:
        return capabilities & QDBusConnection::UnixFileDescriptorPassing ?
                    QDBusArgument::BasicType : QDBusArgument::UnknownType;

    case DBUS_TYPE_INVALID:
        return QDBusArgument::UnknownType;

//    default:
//        qWarning("QDBusDemarshaller: Found unknown D-Bus type %d '%c'",
//                 q_dbus_message_iter_get_arg_type(&iterator),
//                 q_dbus_message_iter_get_arg_type(&iterator));
    }
    return QDBusArgument::UnknownType;
}

QVariant QDBusDemarshaller::toVariantInternal()
{
    switch (q_dbus_message_iter_get_arg_type(&iterator)) {
    case DBUS_TYPE_BYTE:
        return QVariant::fromValue(toByte());
    case DBUS_TYPE_INT16:
        return QVariant::fromValue(toShort());
    case DBUS_TYPE_UINT16:
        return QVariant::fromValue(toUShort());
    case DBUS_TYPE_INT32:
        return toInt();
    case DBUS_TYPE_UINT32:
        return toUInt();
    case DBUS_TYPE_DOUBLE:
        return toDouble();
    case DBUS_TYPE_BOOLEAN:
        return toBool();
    case DBUS_TYPE_INT64:
        return toLongLong();
    case DBUS_TYPE_UINT64:
        return toULongLong();
    case DBUS_TYPE_STRING:
        return toStringUnchecked();
    case DBUS_TYPE_OBJECT_PATH:
        return QVariant::fromValue(toObjectPathUnchecked());
    case DBUS_TYPE_SIGNATURE:
        return QVariant::fromValue(toSignatureUnchecked());
    case DBUS_TYPE_VARIANT:
        return QVariant::fromValue(toVariant());

    case DBUS_TYPE_ARRAY:
        switch (q_dbus_message_iter_get_element_type(&iterator)) {
        case DBUS_TYPE_BYTE:
            // QByteArray
            return toByteArrayUnchecked();
        case DBUS_TYPE_STRING:
            return toStringListUnchecked();
        case DBUS_TYPE_DICT_ENTRY:
            return QVariant::fromValue(duplicate());

        default:
            return QVariant::fromValue(duplicate());
        }

    case DBUS_TYPE_STRUCT:
        return QVariant::fromValue(duplicate());

    case DBUS_TYPE_UNIX_FD:
        if (capabilities & QDBusConnection::UnixFileDescriptorPassing)
            return QVariant::fromValue(toUnixFileDescriptor());
        // fall through

    default:
//        qWarning("QDBusDemarshaller: Found unknown D-Bus type %d '%c'",
//                 q_dbus_message_iter_get_arg_type(&iterator),
//                 q_dbus_message_iter_get_arg_type(&iterator));
        char *ptr = 0;
        ptr += q_dbus_message_iter_get_arg_type(&iterator);
        q_dbus_message_iter_next(&iterator);

        // I hope you never dereference this pointer!
        return QVariant::fromValue<void *>(ptr);
    };
}

bool QDBusDemarshaller::isCurrentTypeStringLike()
{
    const int type = q_dbus_message_iter_get_arg_type(&iterator);
    switch (type) {
    case DBUS_TYPE_STRING:  //FALLTHROUGH
    case DBUS_TYPE_OBJECT_PATH:  //FALLTHROUGH
    case DBUS_TYPE_SIGNATURE:
        return true;
    default:
        return false;
    }
}

QStringList QDBusDemarshaller::toStringListUnchecked()
{
    QStringList list;

    QDBusDemarshaller sub(capabilities);
    q_dbus_message_iter_recurse(&iterator, &sub.iterator);
    q_dbus_message_iter_next(&iterator);
    while (!sub.atEnd())
        list.append(sub.toStringUnchecked());

    return list;
}

QStringList QDBusDemarshaller::toStringList()
{
    if (q_dbus_message_iter_get_arg_type(&iterator) == DBUS_TYPE_ARRAY
            && q_dbus_message_iter_get_element_type(&iterator) == DBUS_TYPE_STRING)
        return toStringListUnchecked();
    else
        return QStringList();
}

QByteArray QDBusDemarshaller::toByteArrayUnchecked()
{
    DBusMessageIter sub;
    q_dbus_message_iter_recurse(&iterator, &sub);
    q_dbus_message_iter_next(&iterator);
    int len;
    char* data;
    q_dbus_message_iter_get_fixed_array(&sub,&data,&len);
    return QByteArray(data,len);
}

QByteArray QDBusDemarshaller::toByteArray()
{
    if (q_dbus_message_iter_get_arg_type(&iterator) == DBUS_TYPE_ARRAY
            && q_dbus_message_iter_get_element_type(&iterator) == DBUS_TYPE_BYTE) {
        return toByteArrayUnchecked();
    }
    return QByteArray();
}

bool QDBusDemarshaller::atEnd()
{
    // dbus_message_iter_has_next is broken if the list has one single element
    return q_dbus_message_iter_get_arg_type(&iterator) == DBUS_TYPE_INVALID;
}

inline QDBusDemarshaller *QDBusDemarshaller::beginStructure()
{
    return beginCommon();
}

inline QDBusDemarshaller *QDBusDemarshaller::beginArray()
{
    return beginCommon();
}

inline QDBusDemarshaller *QDBusDemarshaller::beginMap()
{
    return beginCommon();
}

inline QDBusDemarshaller *QDBusDemarshaller::beginMapEntry()
{
    return beginCommon();
}

QDBusDemarshaller *QDBusDemarshaller::beginCommon()
{
    QDBusDemarshaller *d = new QDBusDemarshaller(capabilities);
    d->parent = this;
    d->message = q_dbus_message_ref(message);

    // recurse
    q_dbus_message_iter_recurse(&iterator, &d->iterator);
    q_dbus_message_iter_next(&iterator);
    return d;
}

inline QDBusDemarshaller *QDBusDemarshaller::endStructure()
{
    return endCommon();
}

inline QDBusDemarshaller *QDBusDemarshaller::endArray()
{
    return endCommon();
}

inline QDBusDemarshaller *QDBusDemarshaller::endMap()
{
    return endCommon();
}

inline QDBusDemarshaller *QDBusDemarshaller::endMapEntry()
{
    return endCommon();
}

QDBusDemarshaller *QDBusDemarshaller::endCommon()
{
    QDBusDemarshaller *retval = parent;
    delete this;
    return retval;
}

QDBusArgument QDBusDemarshaller::duplicate()
{
    QDBusDemarshaller *d = new QDBusDemarshaller(capabilities);
    d->iterator = iterator;
    d->message = q_dbus_message_ref(message);

    q_dbus_message_iter_next(&iterator);
    return QDBusArgumentPrivate::create(d);
}

QT_END_NAMESPACE
