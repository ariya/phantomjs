/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtCore module of the Qt Toolkit.
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

#ifndef QMETAOBJECTBUILDER_P_H
#define QMETAOBJECTBUILDER_P_H

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

#include <QtCore/qobject.h>
#include <QtCore/qmetaobject.h>
#include <QtCore/qdatastream.h>
#include <QtCore/qhash.h>
#include <QtCore/qmap.h>


QT_BEGIN_NAMESPACE

class QMetaObjectBuilderPrivate;
class QMetaMethodBuilder;
class QMetaMethodBuilderPrivate;
class QMetaPropertyBuilder;
class QMetaPropertyBuilderPrivate;
class QMetaEnumBuilder;
class QMetaEnumBuilderPrivate;

class Q_CORE_EXPORT QMetaObjectBuilder
{
public:
    enum AddMember
    {
        ClassName               = 0x00000001,
        SuperClass              = 0x00000002,
        Methods                 = 0x00000004,
        Signals                 = 0x00000008,
        Slots                   = 0x00000010,
        Constructors            = 0x00000020,
        Properties              = 0x00000040,
        Enumerators             = 0x00000080,
        ClassInfos              = 0x00000100,
        RelatedMetaObjects      = 0x00000200,
        StaticMetacall          = 0x00000400,
        PublicMethods           = 0x00000800,
        ProtectedMethods        = 0x00001000,
        PrivateMethods          = 0x00002000,
        AllMembers              = 0x7FFFFFFF,
        AllPrimaryMembers       = 0x7FFFFBFC
    };
    Q_DECLARE_FLAGS(AddMembers, AddMember)

    enum MetaObjectFlag {
        DynamicMetaObject = 0x01
    };
    Q_DECLARE_FLAGS(MetaObjectFlags, MetaObjectFlag)

    QMetaObjectBuilder();
    explicit QMetaObjectBuilder(const QMetaObject *prototype, QMetaObjectBuilder::AddMembers members = AllMembers);
    virtual ~QMetaObjectBuilder();

    QByteArray className() const;
    void setClassName(const QByteArray& name);

    const QMetaObject *superClass() const;
    void setSuperClass(const QMetaObject *meta);

    MetaObjectFlags flags() const;
    void setFlags(MetaObjectFlags);

    int methodCount() const;
    int constructorCount() const;
    int propertyCount() const;
    int enumeratorCount() const;
    int classInfoCount() const;
    int relatedMetaObjectCount() const;

    QMetaMethodBuilder addMethod(const QByteArray& signature);
    QMetaMethodBuilder addMethod(const QByteArray& signature, const QByteArray& returnType);
    QMetaMethodBuilder addMethod(const QMetaMethod& prototype);

    QMetaMethodBuilder addSlot(const QByteArray& signature);
    QMetaMethodBuilder addSignal(const QByteArray& signature);

    QMetaMethodBuilder addConstructor(const QByteArray& signature);
    QMetaMethodBuilder addConstructor(const QMetaMethod& prototype);

    QMetaPropertyBuilder addProperty(const QByteArray& name, const QByteArray& type, int notifierId=-1);
    QMetaPropertyBuilder addProperty(const QMetaProperty& prototype);

    QMetaEnumBuilder addEnumerator(const QByteArray& name);
    QMetaEnumBuilder addEnumerator(const QMetaEnum& prototype);

    int addClassInfo(const QByteArray& name, const QByteArray& value);

    int addRelatedMetaObject(const QMetaObject *meta);

    void addMetaObject(const QMetaObject *prototype, QMetaObjectBuilder::AddMembers members = AllMembers);

    QMetaMethodBuilder method(int index) const;
    QMetaMethodBuilder constructor(int index) const;
    QMetaPropertyBuilder property(int index) const;
    QMetaEnumBuilder enumerator(int index) const;
    const QMetaObject *relatedMetaObject(int index) const;

    QByteArray classInfoName(int index) const;
    QByteArray classInfoValue(int index) const;

    void removeMethod(int index);
    void removeConstructor(int index);
    void removeProperty(int index);
    void removeEnumerator(int index);
    void removeClassInfo(int index);
    void removeRelatedMetaObject(int index);

    int indexOfMethod(const QByteArray& signature);
    int indexOfSignal(const QByteArray& signature);
    int indexOfSlot(const QByteArray& signature);
    int indexOfConstructor(const QByteArray& signature);
    int indexOfProperty(const QByteArray& name);
    int indexOfEnumerator(const QByteArray& name);
    int indexOfClassInfo(const QByteArray& name);

    typedef void (*StaticMetacallFunction)(QObject *, QMetaObject::Call, int, void **);

    QMetaObjectBuilder::StaticMetacallFunction staticMetacallFunction() const;
    void setStaticMetacallFunction(QMetaObjectBuilder::StaticMetacallFunction value);

    QMetaObject *toMetaObject() const;
    QByteArray toRelocatableData(bool * = 0) const;
    static void fromRelocatableData(QMetaObject *, const QMetaObject *, const QByteArray &);

#ifndef QT_NO_DATASTREAM
    void serialize(QDataStream& stream) const;
    void deserialize
        (QDataStream& stream,
         const QMap<QByteArray, const QMetaObject *>& references);
#endif

private:
    Q_DISABLE_COPY(QMetaObjectBuilder)

    QMetaObjectBuilderPrivate *d;

    friend class QMetaMethodBuilder;
    friend class QMetaPropertyBuilder;
    friend class QMetaEnumBuilder;
};

class Q_CORE_EXPORT QMetaMethodBuilder
{
public:
    QMetaMethodBuilder() : _mobj(0), _index(0) {}

    int index() const;

    QMetaMethod::MethodType methodType() const;
    QByteArray signature() const;

    QByteArray returnType() const;
    void setReturnType(const QByteArray& value);

    QList<QByteArray> parameterTypes() const;
    QList<QByteArray> parameterNames() const;
    void setParameterNames(const QList<QByteArray>& value);

    QByteArray tag() const;
    void setTag(const QByteArray& value);

    QMetaMethod::Access access() const;
    void setAccess(QMetaMethod::Access value);

    int attributes() const;
    void setAttributes(int value);

    int revision() const;
    void setRevision(int revision);

private:
    const QMetaObjectBuilder *_mobj;
    int _index;

    friend class QMetaObjectBuilder;
    friend class QMetaPropertyBuilder;

    QMetaMethodBuilder(const QMetaObjectBuilder *mobj, int index)
        : _mobj(mobj), _index(index) {}

    QMetaMethodBuilderPrivate *d_func() const;
};

class Q_CORE_EXPORT QMetaPropertyBuilder
{
public:
    QMetaPropertyBuilder() : _mobj(0), _index(0) {}

    int index() const { return _index; }

    QByteArray name() const;
    QByteArray type() const;

    bool hasNotifySignal() const;
    QMetaMethodBuilder notifySignal() const;
    void setNotifySignal(const QMetaMethodBuilder& value);
    void removeNotifySignal();

    bool isReadable() const;
    bool isWritable() const;
    bool isResettable() const;
    bool isDesignable() const;
    bool isScriptable() const;
    bool isStored() const;
    bool isEditable() const;
    bool isUser() const;
    bool hasStdCppSet() const;
    bool isEnumOrFlag() const;
    bool isConstant() const;
    bool isFinal() const;

    void setReadable(bool value);
    void setWritable(bool value);
    void setResettable(bool value);
    void setDesignable(bool value);
    void setScriptable(bool value);
    void setStored(bool value);
    void setEditable(bool value);
    void setUser(bool value);
    void setStdCppSet(bool value);
    void setEnumOrFlag(bool value);
    void setConstant(bool value);
    void setFinal(bool value);

    int revision() const;
    void setRevision(int revision);

private:
    const QMetaObjectBuilder *_mobj;
    int _index;

    friend class QMetaObjectBuilder;

    QMetaPropertyBuilder(const QMetaObjectBuilder *mobj, int index)
        : _mobj(mobj), _index(index) {}

    QMetaPropertyBuilderPrivate *d_func() const;
};

class Q_CORE_EXPORT QMetaEnumBuilder
{
public:
    QMetaEnumBuilder() : _mobj(0), _index(0) {}

    int index() const { return _index; }

    QByteArray name() const;

    bool isFlag() const;
    void setIsFlag(bool value);

    int keyCount() const;
    QByteArray key(int index) const;
    int value(int index) const;

    int addKey(const QByteArray& name, int value);
    void removeKey(int index);

private:
    const QMetaObjectBuilder *_mobj;
    int _index;

    friend class QMetaObjectBuilder;

    QMetaEnumBuilder(const QMetaObjectBuilder *mobj, int index)
        : _mobj(mobj), _index(index) {}

    QMetaEnumBuilderPrivate *d_func() const;
};

class Q_CORE_EXPORT QMetaStringTable
{
public:
    explicit QMetaStringTable(const QByteArray &className);

    int enter(const QByteArray &value);

    static int preferredAlignment();
    int blobSize() const;
    void writeBlob(char *out) const;

private:
    typedef QHash<QByteArray, int> Entries; // string --> index mapping
    Entries m_entries;
    int m_index;
    QByteArray m_className;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QMetaObjectBuilder::AddMembers)
Q_DECLARE_OPERATORS_FOR_FLAGS(QMetaObjectBuilder::MetaObjectFlags)

QT_END_NAMESPACE

#endif
