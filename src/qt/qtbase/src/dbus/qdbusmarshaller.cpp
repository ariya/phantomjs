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
#include "qdbusmetatype_p.h"
#include "qdbusutil_p.h"

#ifndef QT_NO_DBUS

QT_BEGIN_NAMESPACE

static void qIterAppend(DBusMessageIter *it, QByteArray *ba, int type, const void *arg)
{
    if (ba)
        *ba += char(type);
    else
        q_dbus_message_iter_append_basic(it, type, arg);
}

QDBusMarshaller::~QDBusMarshaller()
{
    close();
}

inline QString QDBusMarshaller::currentSignature()
{
    if (message)
        return QString::fromUtf8(q_dbus_message_get_signature(message));
    return QString();
}

inline void QDBusMarshaller::append(uchar arg)
{
    qIterAppend(&iterator, ba, DBUS_TYPE_BYTE, &arg);
}

inline void QDBusMarshaller::append(bool arg)
{
    dbus_bool_t cast = arg;
    qIterAppend(&iterator, ba, DBUS_TYPE_BOOLEAN, &cast);
}

inline void QDBusMarshaller::append(short arg)
{
    qIterAppend(&iterator, ba, DBUS_TYPE_INT16, &arg);
}

inline void QDBusMarshaller::append(ushort arg)
{
    qIterAppend(&iterator, ba, DBUS_TYPE_UINT16, &arg);
}

inline void QDBusMarshaller::append(int arg)
{
    qIterAppend(&iterator, ba, DBUS_TYPE_INT32, &arg);
}

inline void QDBusMarshaller::append(uint arg)
{
    qIterAppend(&iterator, ba, DBUS_TYPE_UINT32, &arg);
}

inline void QDBusMarshaller::append(qlonglong arg)
{
    qIterAppend(&iterator, ba, DBUS_TYPE_INT64, &arg);
}

inline void QDBusMarshaller::append(qulonglong arg)
{
    qIterAppend(&iterator, ba, DBUS_TYPE_UINT64, &arg);
}

inline void QDBusMarshaller::append(double arg)
{
    qIterAppend(&iterator, ba, DBUS_TYPE_DOUBLE, &arg);
}

void QDBusMarshaller::append(const QString &arg)
{
    QByteArray data = arg.toUtf8();
    const char *cdata = data.constData();
    qIterAppend(&iterator, ba, DBUS_TYPE_STRING, &cdata);
}

inline void QDBusMarshaller::append(const QDBusObjectPath &arg)
{
    QByteArray data = arg.path().toUtf8();
    if (!ba && data.isEmpty()) {
        error(QLatin1String("Invalid object path passed in arguments"));
    } else {
        const char *cdata = data.constData();
        qIterAppend(&iterator, ba, DBUS_TYPE_OBJECT_PATH, &cdata);
    }
}

inline void QDBusMarshaller::append(const QDBusSignature &arg)
{
    QByteArray data = arg.signature().toUtf8();
    if (!ba && data.isEmpty()) {
        error(QLatin1String("Invalid signature passed in arguments"));
    } else {
        const char *cdata = data.constData();
        qIterAppend(&iterator, ba, DBUS_TYPE_SIGNATURE, &cdata);
    }
}

inline void QDBusMarshaller::append(const QDBusUnixFileDescriptor &arg)
{
    int fd = arg.fileDescriptor();
    if (!ba && fd == -1) {
        error(QLatin1String("Invalid file descriptor passed in arguments"));
    } else {
        qIterAppend(&iterator, ba, DBUS_TYPE_UNIX_FD, &fd);
    }
}

inline void QDBusMarshaller::append(const QByteArray &arg)
{
    if (ba) {
        *ba += DBUS_TYPE_ARRAY_AS_STRING DBUS_TYPE_BYTE_AS_STRING;
        return;
    }

    const char* cdata = arg.constData();
    DBusMessageIter subiterator;
    q_dbus_message_iter_open_container(&iterator, DBUS_TYPE_ARRAY, DBUS_TYPE_BYTE_AS_STRING,
                                     &subiterator);
    q_dbus_message_iter_append_fixed_array(&subiterator, DBUS_TYPE_BYTE, &cdata, arg.length());
    q_dbus_message_iter_close_container(&iterator, &subiterator);
}

inline bool QDBusMarshaller::append(const QDBusVariant &arg)
{
    if (ba) {
        *ba += DBUS_TYPE_VARIANT_AS_STRING;
        return true;
    }

    const QVariant &value = arg.variant();
    int id = value.userType();
    if (id == QVariant::Invalid) {
        qWarning("QDBusMarshaller: cannot add a null QDBusVariant");
        error(QLatin1String("Variant containing QVariant::Invalid passed in arguments"));
        return false;
    }

    QByteArray tmpSignature;
    const char *signature = 0;
    if (id == QDBusMetaTypeId::argument()) {
        // take the signature from the QDBusArgument object we're marshalling
        tmpSignature =
            qvariant_cast<QDBusArgument>(value).currentSignature().toLatin1();
        signature = tmpSignature.constData();
    } else {
        // take the signatuer from the metatype we're marshalling
        signature = QDBusMetaType::typeToSignature(id);
    }
    if (!signature) {
        qWarning("QDBusMarshaller: type `%s' (%d) is not registered with D-BUS. "
                 "Use qDBusRegisterMetaType to register it",
                 QMetaType::typeName(id), id);
        error(QString::fromLatin1("Unregistered type %1 passed in arguments")
              .arg(QLatin1String(QMetaType::typeName(id))));
        return false;
    }

    QDBusMarshaller sub(capabilities);
    open(sub, DBUS_TYPE_VARIANT, signature);
    bool isOk = sub.appendVariantInternal(value);
    // don't call sub.close(): it auto-closes

    return isOk;
}

inline void QDBusMarshaller::append(const QStringList &arg)
{
    if (ba) {
        *ba += DBUS_TYPE_ARRAY_AS_STRING DBUS_TYPE_STRING_AS_STRING;
        return;
    }

    QDBusMarshaller sub(capabilities);
    open(sub, DBUS_TYPE_ARRAY, DBUS_TYPE_STRING_AS_STRING);
    QStringList::ConstIterator it = arg.constBegin();
    QStringList::ConstIterator end = arg.constEnd();
    for ( ; it != end; ++it)
        sub.append(*it);
    // don't call sub.close(): it auto-closes
}

inline QDBusMarshaller *QDBusMarshaller::beginStructure()
{
    return beginCommon(DBUS_TYPE_STRUCT, 0);
}

inline QDBusMarshaller *QDBusMarshaller::beginArray(int id)
{
    const char *signature = QDBusMetaType::typeToSignature( QVariant::Type(id) );
    if (!signature) {
        qWarning("QDBusMarshaller: type `%s' (%d) is not registered with D-BUS. "
                 "Use qDBusRegisterMetaType to register it",
                 QMetaType::typeName(id), id);
        error(QString::fromLatin1("Unregistered type %1 passed in arguments")
              .arg(QLatin1String(QMetaType::typeName(id))));
        return this;
    }

    return beginCommon(DBUS_TYPE_ARRAY, signature);
}

inline QDBusMarshaller *QDBusMarshaller::beginMap(int kid, int vid)
{
    const char *ksignature = QDBusMetaType::typeToSignature( QVariant::Type(kid) );
    if (!ksignature) {
        qWarning("QDBusMarshaller: type `%s' (%d) is not registered with D-BUS. "
                 "Use qDBusRegisterMetaType to register it",
                 QMetaType::typeName(kid), kid);
        error(QString::fromLatin1("Unregistered type %1 passed in arguments")
              .arg(QLatin1String(QMetaType::typeName(kid))));
        return this;
    }
    if (ksignature[1] != 0 || !QDBusUtil::isValidBasicType(*ksignature)) {
        qWarning("QDBusMarshaller: type '%s' (%d) cannot be used as the key type in a D-BUS map.",
                 QMetaType::typeName(kid), kid);
        error(QString::fromLatin1("Type %1 passed in arguments cannot be used as a key in a map")
              .arg(QLatin1String(QMetaType::typeName(kid))));
        return this;
    }

    const char *vsignature = QDBusMetaType::typeToSignature( QVariant::Type(vid) );
    if (!vsignature) {
        const char *typeName = QMetaType::typeName(vid);
        qWarning("QDBusMarshaller: type `%s' (%d) is not registered with D-BUS. "
                 "Use qDBusRegisterMetaType to register it",
                 typeName, vid);
        error(QString::fromLatin1("Unregistered type %1 passed in arguments")
              .arg(QLatin1String(typeName)));
        return this;
    }

    QByteArray signature;
    signature = DBUS_DICT_ENTRY_BEGIN_CHAR_AS_STRING;
    signature += ksignature;
    signature += vsignature;
    signature += DBUS_DICT_ENTRY_END_CHAR_AS_STRING;
    return beginCommon(DBUS_TYPE_ARRAY, signature);
}

inline QDBusMarshaller *QDBusMarshaller::beginMapEntry()
{
    return beginCommon(DBUS_TYPE_DICT_ENTRY, 0);
}

void QDBusMarshaller::open(QDBusMarshaller &sub, int code, const char *signature)
{
    sub.parent = this;
    sub.ba = ba;
    sub.ok = true;
    sub.capabilities = capabilities;

    if (ba)
        switch (code) {
        case DBUS_TYPE_ARRAY:
            *ba += char(code);
            *ba += signature;
            // fall through

        case DBUS_TYPE_DICT_ENTRY:
            sub.closeCode = 0;
            break;

        case DBUS_TYPE_STRUCT:
            *ba += DBUS_STRUCT_BEGIN_CHAR;
            sub.closeCode = DBUS_STRUCT_END_CHAR;
            break;
        }
    else
        q_dbus_message_iter_open_container(&iterator, code, signature, &sub.iterator);
}

QDBusMarshaller *QDBusMarshaller::beginCommon(int code, const char *signature)
{
    QDBusMarshaller *d = new QDBusMarshaller(capabilities);
    open(*d, code, signature);
    return d;
}

inline QDBusMarshaller *QDBusMarshaller::endStructure()
{ return endCommon(); }

inline QDBusMarshaller *QDBusMarshaller::endArray()
{ return endCommon(); }

inline QDBusMarshaller *QDBusMarshaller::endMap()
{ return endCommon(); }

inline QDBusMarshaller *QDBusMarshaller::endMapEntry()
{ return endCommon(); }

QDBusMarshaller *QDBusMarshaller::endCommon()
{
    QDBusMarshaller *retval = parent;
    delete this;
    return retval;
}

void QDBusMarshaller::close()
{
    if (ba) {
        if (closeCode)
            *ba += closeCode;
    } else if (parent) {
        q_dbus_message_iter_close_container(&parent->iterator, &iterator);
    }
}

void QDBusMarshaller::error(const QString &msg)
{
    ok = false;
    if (parent)
        parent->error(msg);
    else
        errorString = msg;
}

bool QDBusMarshaller::appendVariantInternal(const QVariant &arg)
{
    int id = arg.userType();
    if (id == QVariant::Invalid) {
        qWarning("QDBusMarshaller: cannot add an invalid QVariant");
        error(QLatin1String("Variant containing QVariant::Invalid passed in arguments"));
        return false;
    }

    // intercept QDBusArgument parameters here
    if (id == QDBusMetaTypeId::argument()) {
        QDBusArgument dbusargument = qvariant_cast<QDBusArgument>(arg);
        QDBusArgumentPrivate *d = QDBusArgumentPrivate::d(dbusargument);
        if (!d->message)
            return false;       // can't append this one...

        QDBusDemarshaller demarshaller(capabilities);
        demarshaller.message = q_dbus_message_ref(d->message);

        if (d->direction == Demarshalling) {
            // it's demarshalling; just copy
            demarshaller.iterator = static_cast<QDBusDemarshaller *>(d)->iterator;
        } else {
            // it's marshalling; start over
            if (!q_dbus_message_iter_init(demarshaller.message, &demarshaller.iterator))
                return false;   // error!
        }

        return appendCrossMarshalling(&demarshaller);
    }

    const char *signature = QDBusMetaType::typeToSignature( QVariant::Type(id) );
    if (!signature) {
        qWarning("QDBusMarshaller: type `%s' (%d) is not registered with D-BUS. "
                 "Use qDBusRegisterMetaType to register it",
                 QMetaType::typeName(id), id);
        error(QString::fromLatin1("Unregistered type %1 passed in arguments")
              .arg(QLatin1String(QMetaType::typeName(id))));
        return false;
    }

    switch (*signature) {
#ifdef __OPTIMIZE__
    case DBUS_TYPE_BYTE:
    case DBUS_TYPE_INT16:
    case DBUS_TYPE_UINT16:
    case DBUS_TYPE_INT32:
    case DBUS_TYPE_UINT32:
    case DBUS_TYPE_INT64:
    case DBUS_TYPE_UINT64:
    case DBUS_TYPE_DOUBLE:
        qIterAppend(&iterator, ba, *signature, arg.constData());
        return true;
    case DBUS_TYPE_BOOLEAN:
        append( arg.toBool() );
        return true;
#else
    case DBUS_TYPE_BYTE:
        append( qvariant_cast<uchar>(arg) );
        return true;
    case DBUS_TYPE_BOOLEAN:
        append( arg.toBool() );
        return true;
    case DBUS_TYPE_INT16:
        append( qvariant_cast<short>(arg) );
        return true;
    case DBUS_TYPE_UINT16:
        append( qvariant_cast<ushort>(arg) );
        return true;
    case DBUS_TYPE_INT32:
        append( static_cast<dbus_int32_t>(arg.toInt()) );
        return true;
    case DBUS_TYPE_UINT32:
        append( static_cast<dbus_uint32_t>(arg.toUInt()) );
        return true;
    case DBUS_TYPE_INT64:
        append( arg.toLongLong() );
        return true;
    case DBUS_TYPE_UINT64:
        append( arg.toULongLong() );
        return true;
    case DBUS_TYPE_DOUBLE:
        append( arg.toDouble() );
        return true;
#endif

    case DBUS_TYPE_STRING:
        append( arg.toString() );
        return true;
    case DBUS_TYPE_OBJECT_PATH:
        append( qvariant_cast<QDBusObjectPath>(arg) );
        return true;
    case DBUS_TYPE_SIGNATURE:
        append( qvariant_cast<QDBusSignature>(arg) );
        return true;

    // compound types:
    case DBUS_TYPE_VARIANT:
        // nested QVariant
        return append( qvariant_cast<QDBusVariant>(arg) );

    case DBUS_TYPE_ARRAY:
        // could be many things
        // find out what kind of array it is
        switch (arg.type()) {
        case QVariant::StringList:
            append( arg.toStringList() );
            return true;

        case QVariant::ByteArray:
            append( arg.toByteArray() );
            return true;

        default:
            ;                   // fall through
        }
        // fall through

    case DBUS_TYPE_STRUCT:
    case DBUS_STRUCT_BEGIN_CHAR:
        return appendRegisteredType( arg );

    case DBUS_TYPE_DICT_ENTRY:
    case DBUS_DICT_ENTRY_BEGIN_CHAR:
        qFatal("QDBusMarshaller::appendVariantInternal got a DICT_ENTRY!");
        return false;

    case DBUS_TYPE_UNIX_FD:
        if (capabilities & QDBusConnection::UnixFileDescriptorPassing || ba) {
            append(qvariant_cast<QDBusUnixFileDescriptor>(arg));
            return true;
        }
        // fall through

    default:
        qWarning("QDBusMarshaller::appendVariantInternal: Found unknown D-BUS type '%s'",
                 signature);
        return false;
    }

    return true;
}

bool QDBusMarshaller::appendRegisteredType(const QVariant &arg)
{
    ref.ref();                  // reference up
    QDBusArgument self(QDBusArgumentPrivate::create(this));
    return QDBusMetaType::marshall(self, arg.userType(), arg.constData());
}

bool QDBusMarshaller::appendCrossMarshalling(QDBusDemarshaller *demarshaller)
{
    int code = q_dbus_message_iter_get_arg_type(&demarshaller->iterator);
    if (QDBusUtil::isValidBasicType(code)) {
        // easy: just append
        // do exactly like the D-BUS docs suggest
        // (see apidocs for q_dbus_message_iter_get_basic)

        qlonglong value;
        q_dbus_message_iter_get_basic(&demarshaller->iterator, &value);
        q_dbus_message_iter_next(&demarshaller->iterator);
        q_dbus_message_iter_append_basic(&iterator, code, &value);
        return true;
    }

    if (code == DBUS_TYPE_ARRAY) {
        int element = q_dbus_message_iter_get_element_type(&demarshaller->iterator);
        if (QDBusUtil::isValidFixedType(element) && element != DBUS_TYPE_UNIX_FD) {
            // another optimization: fixed size arrays
            // code is exactly like QDBusDemarshaller::toByteArray
            DBusMessageIter sub;
            q_dbus_message_iter_recurse(&demarshaller->iterator, &sub);
            q_dbus_message_iter_next(&demarshaller->iterator);
            int len;
            void* data;
            q_dbus_message_iter_get_fixed_array(&sub,&data,&len);

            char signature[2] = { char(element), 0 };
            q_dbus_message_iter_open_container(&iterator, DBUS_TYPE_ARRAY, signature, &sub);
            q_dbus_message_iter_append_fixed_array(&sub, element, &data, len);
            q_dbus_message_iter_close_container(&iterator, &sub);

            return true;
        }
    }

    // We have to recurse
    QDBusDemarshaller *drecursed = demarshaller->beginCommon();

    QDBusMarshaller mrecursed(capabilities);  // create on the stack makes it autoclose
    QByteArray subSignature;
    const char *sig = 0;
    if (code == DBUS_TYPE_VARIANT || code == DBUS_TYPE_ARRAY) {
        subSignature = drecursed->currentSignature().toLatin1();
        if (!subSignature.isEmpty())
            sig = subSignature.constData();
    }
    open(mrecursed, code, sig);

    while (!drecursed->atEnd()) {
        if (!mrecursed.appendCrossMarshalling(drecursed)) {
            delete drecursed;
            return false;
        }
    }

    delete drecursed;
    return true;
}

QT_END_NAMESPACE

#endif // QT_NO_DBUS
