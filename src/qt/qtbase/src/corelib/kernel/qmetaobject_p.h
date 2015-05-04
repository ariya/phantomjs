/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia. For licensing terms and
** conditions see http://qt.digia.com/licensing. For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights. These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QMETAOBJECT_P_H
#define QMETAOBJECT_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of moc.  This header file may change from version to version without notice,
// or even be removed.
//
// We mean it.
//

#include <QtCore/qglobal.h>
#include <QtCore/qobjectdefs.h>
#ifndef QT_NO_QOBJECT
#include <private/qobject_p.h> // For QObjectPrivate::Connection
#endif
#include <QtCore/qvarlengtharray.h>

QT_BEGIN_NAMESPACE

enum PropertyFlags  {
    Invalid = 0x00000000,
    Readable = 0x00000001,
    Writable = 0x00000002,
    Resettable = 0x00000004,
    EnumOrFlag = 0x00000008,
    StdCppSet = 0x00000100,
//     Override = 0x00000200,
    Constant = 0x00000400,
    Final = 0x00000800,
    Designable = 0x00001000,
    ResolveDesignable = 0x00002000,
    Scriptable = 0x00004000,
    ResolveScriptable = 0x00008000,
    Stored = 0x00010000,
    ResolveStored = 0x00020000,
    Editable = 0x00040000,
    ResolveEditable = 0x00080000,
    User = 0x00100000,
    ResolveUser = 0x00200000,
    Notify = 0x00400000,
    Revisioned = 0x00800000
};

enum MethodFlags  {
    AccessPrivate = 0x00,
    AccessProtected = 0x01,
    AccessPublic = 0x02,
    AccessMask = 0x03, //mask

    MethodMethod = 0x00,
    MethodSignal = 0x04,
    MethodSlot = 0x08,
    MethodConstructor = 0x0c,
    MethodTypeMask = 0x0c,

    MethodCompatibility = 0x10,
    MethodCloned = 0x20,
    MethodScriptable = 0x40,
    MethodRevisioned = 0x80
};

enum MetaObjectFlags {
    DynamicMetaObject = 0x01,
    RequiresVariantMetaObject = 0x02
};

enum MetaDataFlags {
    IsUnresolvedType = 0x80000000,
    TypeNameIndexMask = 0x7FFFFFFF
};

extern int qMetaTypeTypeInternal(const char *);

class QArgumentType
{
public:
    QArgumentType(int type)
        : _type(type)
    {}
    QArgumentType(const QByteArray &name)
        : _type(qMetaTypeTypeInternal(name.constData())), _name(name)
    {}
    QArgumentType()
        : _type(0)
    {}
    int type() const
    { return _type; }
    QByteArray name() const
    {
        if (_type && _name.isEmpty())
            const_cast<QArgumentType *>(this)->_name = QMetaType::typeName(_type);
        return _name;
    }
    bool operator==(const QArgumentType &other) const
    {
        if (_type)
            return _type == other._type;
        else if (other._type)
            return false;
        else
            return _name == other._name;
    }
    bool operator!=(const QArgumentType &other) const
    {
        if (_type)
            return _type != other._type;
        else if (other._type)
            return true;
        else
            return _name != other._name;
    }

private:
    int _type;
    QByteArray _name;
};

typedef QVarLengthArray<QArgumentType, 10> QArgumentTypeArray;

class QMetaMethodPrivate;
class QMutex;

struct QMetaObjectPrivate
{
    enum { OutputRevision = 7 }; // Used by moc, qmetaobjectbuilder and qdbus

    int revision;
    int className;
    int classInfoCount, classInfoData;
    int methodCount, methodData;
    int propertyCount, propertyData;
    int enumeratorCount, enumeratorData;
    int constructorCount, constructorData; //since revision 2
    int flags; //since revision 3
    int signalCount; //since revision 4
    // revision 5 introduces changes in normalized signatures, no new members
    // revision 6 added qt_static_metacall as a member of each Q_OBJECT and inside QMetaObject itself
    // revision 7 is Qt 5

    static inline const QMetaObjectPrivate *get(const QMetaObject *metaobject)
    { return reinterpret_cast<const QMetaObjectPrivate*>(metaobject->d.data); }

    static int originalClone(const QMetaObject *obj, int local_method_index);

    static QByteArray decodeMethodSignature(const char *signature,
                                            QArgumentTypeArray &types);
    static int indexOfSignalRelative(const QMetaObject **baseObject,
                                     const QByteArray &name, int argc,
                                     const QArgumentType *types);
    static int indexOfSlotRelative(const QMetaObject **m,
                                   const QByteArray &name, int argc,
                                   const QArgumentType *types);
    static int indexOfSignal(const QMetaObject *m, const QByteArray &name,
                             int argc, const QArgumentType *types);
    static int indexOfSlot(const QMetaObject *m, const QByteArray &name,
                           int argc, const QArgumentType *types);
    static int indexOfMethod(const QMetaObject *m, const QByteArray &name,
                             int argc, const QArgumentType *types);
    static int indexOfConstructor(const QMetaObject *m, const QByteArray &name,
                                  int argc, const QArgumentType *types);
    Q_CORE_EXPORT static QMetaMethod signal(const QMetaObject *m, int signal_index);
    Q_CORE_EXPORT static int signalOffset(const QMetaObject *m);
    Q_CORE_EXPORT static int absoluteSignalCount(const QMetaObject *m);
    Q_CORE_EXPORT static int signalIndex(const QMetaMethod &m);
    static bool checkConnectArgs(int signalArgc, const QArgumentType *signalTypes,
                                 int methodArgc, const QArgumentType *methodTypes);
    static bool checkConnectArgs(const QMetaMethodPrivate *signal,
                                 const QMetaMethodPrivate *method);

    static QList<QByteArray> parameterTypeNamesFromSignature(const char *signature);

#ifndef QT_NO_QOBJECT
    //defined in qobject.cpp
    enum DisconnectType { DisconnectAll, DisconnectOne };
    static void memberIndexes(const QObject *obj, const QMetaMethod &member,
                              int *signalIndex, int *methodIndex);
    static QObjectPrivate::Connection *connect(const QObject *sender, int signal_index,
                        const QMetaObject *smeta,
                        const QObject *receiver, int method_index_relative,
                        const QMetaObject *rmeta = 0,
                        int type = 0, int *types = 0);
    static bool disconnect(const QObject *sender, int signal_index,
                           const QMetaObject *smeta,
                           const QObject *receiver, int method_index, void **slot,
                           DisconnectType = DisconnectAll);
    static inline bool disconnectHelper(QObjectPrivate::Connection *c,
                                        const QObject *receiver, int method_index, void **slot,
                                        QMutex *senderMutex, DisconnectType = DisconnectAll);
#endif
};

// For meta-object generators

enum { MetaObjectPrivateFieldCount = sizeof(QMetaObjectPrivate) / sizeof(int) };

#ifndef UTILS_H
// mirrored in moc's utils.h
static inline bool is_ident_char(char s)
{
    return ((s >= 'a' && s <= 'z')
            || (s >= 'A' && s <= 'Z')
            || (s >= '0' && s <= '9')
            || s == '_'
       );
}

static inline bool is_space(char s)
{
    return (s == ' ' || s == '\t');
}
#endif

/*
    This function is shared with moc.cpp. The implementation lives in qmetaobject_moc_p.h, which
    should be included where needed. The declaration here is not used to avoid warnings from
    the compiler about unused functions.

static QByteArray normalizeTypeInternal(const char *t, const char *e, bool fixScope = false, bool adjustConst = true);
*/

QT_END_NAMESPACE

#endif

