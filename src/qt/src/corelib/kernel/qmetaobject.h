/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtCore module of the Qt Toolkit.
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

#ifndef QMETAOBJECT_H
#define QMETAOBJECT_H

#include <QtCore/qobjectdefs.h>
#include <QtCore/qvariant.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Core)

template <typename T> class QList;

class Q_CORE_EXPORT QMetaMethod
{
public:
    inline QMetaMethod() : mobj(0),handle(0) {}

    const char *signature() const;
    const char *typeName() const;
    QList<QByteArray> parameterTypes() const;
    QList<QByteArray> parameterNames() const;
    const char *tag() const;
    enum Access { Private, Protected, Public };
    Access access() const;
    enum MethodType { Method, Signal, Slot, Constructor };
    MethodType methodType() const;
    enum Attributes { Compatibility = 0x1, Cloned = 0x2, Scriptable = 0x4 };
    int attributes() const;
    int methodIndex() const;
    int revision() const;

    inline const QMetaObject *enclosingMetaObject() const { return mobj; }

    bool invoke(QObject *object,
                Qt::ConnectionType connectionType,
                QGenericReturnArgument returnValue,
                QGenericArgument val0 = QGenericArgument(0),
                QGenericArgument val1 = QGenericArgument(),
                QGenericArgument val2 = QGenericArgument(),
                QGenericArgument val3 = QGenericArgument(),
                QGenericArgument val4 = QGenericArgument(),
                QGenericArgument val5 = QGenericArgument(),
                QGenericArgument val6 = QGenericArgument(),
                QGenericArgument val7 = QGenericArgument(),
                QGenericArgument val8 = QGenericArgument(),
                QGenericArgument val9 = QGenericArgument()) const;
    inline bool invoke(QObject *object,
                       QGenericReturnArgument returnValue,
                       QGenericArgument val0 = QGenericArgument(0),
                       QGenericArgument val1 = QGenericArgument(),
                       QGenericArgument val2 = QGenericArgument(),
                       QGenericArgument val3 = QGenericArgument(),
                       QGenericArgument val4 = QGenericArgument(),
                       QGenericArgument val5 = QGenericArgument(),
                       QGenericArgument val6 = QGenericArgument(),
                       QGenericArgument val7 = QGenericArgument(),
                       QGenericArgument val8 = QGenericArgument(),
                       QGenericArgument val9 = QGenericArgument()) const
    {
        return invoke(object, Qt::AutoConnection, returnValue,
                      val0, val1, val2, val3, val4, val5, val6, val7, val8, val9);
    }
    inline bool invoke(QObject *object,
                       Qt::ConnectionType connectionType,
                       QGenericArgument val0 = QGenericArgument(0),
                       QGenericArgument val1 = QGenericArgument(),
                       QGenericArgument val2 = QGenericArgument(),
                       QGenericArgument val3 = QGenericArgument(),
                       QGenericArgument val4 = QGenericArgument(),
                       QGenericArgument val5 = QGenericArgument(),
                       QGenericArgument val6 = QGenericArgument(),
                       QGenericArgument val7 = QGenericArgument(),
                       QGenericArgument val8 = QGenericArgument(),
                       QGenericArgument val9 = QGenericArgument()) const
    {
        return invoke(object, connectionType, QGenericReturnArgument(),
                      val0, val1, val2, val3, val4, val5, val6, val7, val8, val9);
    }
    inline bool invoke(QObject *object,
                       QGenericArgument val0 = QGenericArgument(0),
                       QGenericArgument val1 = QGenericArgument(),
                       QGenericArgument val2 = QGenericArgument(),
                       QGenericArgument val3 = QGenericArgument(),
                       QGenericArgument val4 = QGenericArgument(),
                       QGenericArgument val5 = QGenericArgument(),
                       QGenericArgument val6 = QGenericArgument(),
                       QGenericArgument val7 = QGenericArgument(),
                       QGenericArgument val8 = QGenericArgument(),
                       QGenericArgument val9 = QGenericArgument()) const
    {
        return invoke(object, Qt::AutoConnection, QGenericReturnArgument(),
                      val0, val1, val2, val3, val4, val5, val6, val7, val8, val9);
    }

private:
    const QMetaObject *mobj;
    uint handle;
    friend struct QMetaObject;
    friend struct QMetaObjectPrivate;
    friend class QObject;
};
Q_DECLARE_TYPEINFO(QMetaMethod, Q_MOVABLE_TYPE);

class Q_CORE_EXPORT QMetaEnum
{
public:
    inline QMetaEnum() : mobj(0),handle(0) {}

    const char *name() const;
    bool isFlag() const;

    int keyCount() const;
    const char *key(int index) const;
    int value(int index) const;

    const char *scope() const;

    int keyToValue(const char *key) const;
    const char* valueToKey(int value) const;
    int keysToValue(const char * keys) const;
    QByteArray valueToKeys(int value) const;

    inline const QMetaObject *enclosingMetaObject() const { return mobj; }

    inline bool isValid() const { return name() != 0; }
private:
    const QMetaObject *mobj;
    uint handle;
    friend struct QMetaObject;
};
Q_DECLARE_TYPEINFO(QMetaEnum, Q_MOVABLE_TYPE);

class Q_CORE_EXPORT QMetaProperty
{
public:
    QMetaProperty();

    const char *name() const;
    const char *typeName() const;
    QVariant::Type type() const;
    int userType() const;
    int propertyIndex() const;

    bool isReadable() const;
    bool isWritable() const;
    bool isResettable() const;
    bool isDesignable(const QObject *obj = 0) const;
    bool isScriptable(const QObject *obj = 0) const;
    bool isStored(const QObject *obj = 0) const;
    bool isEditable(const QObject *obj = 0) const;
    bool isUser(const QObject *obj = 0) const;
    bool isConstant() const;
    bool isFinal() const;

    bool isFlagType() const;
    bool isEnumType() const;
    QMetaEnum enumerator() const;

    bool hasNotifySignal() const;
    QMetaMethod notifySignal() const;
    int notifySignalIndex() const;

    int revision() const;

    QVariant read(const QObject *obj) const;
    bool write(QObject *obj, const QVariant &value) const;
    bool reset(QObject *obj) const;

    bool hasStdCppSet() const;
    inline bool isValid() const { return isReadable(); }
    inline const QMetaObject *enclosingMetaObject() const { return mobj; }

private:
    const QMetaObject *mobj;
    uint handle;
    int idx;
    QMetaEnum menum;
    friend struct QMetaObject;
};

class Q_CORE_EXPORT QMetaClassInfo
{
public:
    inline QMetaClassInfo() : mobj(0),handle(0) {}
    const char *name() const;
    const char *value() const;
    inline const QMetaObject *enclosingMetaObject() const { return mobj; }
private:
    const QMetaObject *mobj;
    uint handle;
    friend struct QMetaObject;
};
Q_DECLARE_TYPEINFO(QMetaClassInfo, Q_MOVABLE_TYPE);

QT_END_NAMESPACE

QT_END_HEADER

#endif // QMETAOBJECT_H
