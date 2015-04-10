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

#include "qmetaobjectbuilder_p.h"

#include "qobject_p.h"
#include "qmetaobject_p.h"

#include <stdlib.h>

QT_BEGIN_NAMESPACE

/*!
    \class QMetaObjectBuilder
    \inmodule QtCore
    \internal
    \brief The QMetaObjectBuilder class supports building QMetaObject objects at runtime.

*/

/*!
    \enum QMetaObjectBuilder::AddMember
    This enum defines which members of QMetaObject should be copied by QMetaObjectBuilder::addMetaObject()

    \value ClassName Add the class name.
    \value SuperClass Add the super class.
    \value Methods Add methods that aren't signals or slots.
    \value Signals Add signals.
    \value Slots Add slots.
    \value Constructors Add constructors.
    \value Properties Add properties.
    \value Enumerators Add enumerators.
    \value ClassInfos Add items of class information.
    \value RelatedMetaObjects Add related meta objects.
    \value StaticMetacall Add the static metacall function.
    \value PublicMethods Add public methods (ignored for signals).
    \value ProtectedMethods Add protected methods (ignored for signals).
    \value PrivateMethods All private methods (ignored for signals).
    \value AllMembers Add all members.
    \value AllPrimaryMembers Add everything except the class name, super class, and static metacall function.
*/

// copied from moc's generator.cpp
namespace QtPrivate {
Q_CORE_EXPORT bool isBuiltinType(const QByteArray &type)
{
    int id = QMetaType::type(type);
    if (!id && !type.isEmpty() && type != "void")
        return false;
    return (id < QMetaType::User);
}
} // namespace QtPrivate

// copied from qmetaobject.cpp
static inline Q_DECL_UNUSED const QMetaObjectPrivate *priv(const uint* data)
{ return reinterpret_cast<const QMetaObjectPrivate*>(data); }

class QMetaMethodBuilderPrivate
{
public:
    QMetaMethodBuilderPrivate
            (QMetaMethod::MethodType _methodType,
             const QByteArray& _signature,
             const QByteArray& _returnType = QByteArray("void"),
             QMetaMethod::Access _access = QMetaMethod::Public,
             int _revision = 0)
        : signature(QMetaObject::normalizedSignature(_signature.constData())),
          returnType(QMetaObject::normalizedType(_returnType)),
          attributes(((int)_access) | (((int)_methodType) << 2)),
          revision(_revision)
    {
        Q_ASSERT((_methodType == QMetaMethod::Constructor) == returnType.isNull());
    }

    QByteArray signature;
    QByteArray returnType;
    QList<QByteArray> parameterNames;
    QByteArray tag;
    int attributes;
    int revision;

    QMetaMethod::MethodType methodType() const
    {
        return (QMetaMethod::MethodType)((attributes & MethodTypeMask) >> 2);
    }

    QMetaMethod::Access access() const
    {
        return (QMetaMethod::Access)(attributes & AccessMask);
    }

    void setAccess(QMetaMethod::Access value)
    {
        attributes = ((attributes & ~AccessMask) | (int)value);
    }

    QList<QByteArray> parameterTypes() const
    {
        return QMetaObjectPrivate::parameterTypeNamesFromSignature(signature);
    }

    int parameterCount() const
    {
        return parameterTypes().size();
    }

    QByteArray name() const
    {
        return signature.left(qMax(signature.indexOf('('), 0));
    }
};

class QMetaPropertyBuilderPrivate
{
public:
    QMetaPropertyBuilderPrivate
            (const QByteArray& _name, const QByteArray& _type, int notifierIdx=-1,
             int _revision = 0)
        : name(_name),
          type(QMetaObject::normalizedType(_type.constData())),
          flags(Readable | Writable | Scriptable), notifySignal(-1),
          revision(_revision)
    {
        if (notifierIdx >= 0) {
            flags |= Notify;
            notifySignal = notifierIdx;
        }
    }

    QByteArray name;
    QByteArray type;
    int flags;
    int notifySignal;
    int revision;

    bool flag(int f) const
    {
        return ((flags & f) != 0);
    }

    void setFlag(int f, bool value)
    {
        if (value)
            flags |= f;
        else
            flags &= ~f;
    }
};

class QMetaEnumBuilderPrivate
{
public:
    QMetaEnumBuilderPrivate(const QByteArray& _name)
        : name(_name), isFlag(false)
    {
    }

    QByteArray name;
    bool isFlag;
    QList<QByteArray> keys;
    QList<int> values;
};

class QMetaObjectBuilderPrivate
{
public:
    QMetaObjectBuilderPrivate()
        : flags(0)
    {
        superClass = &QObject::staticMetaObject;
        staticMetacallFunction = 0;
    }

    bool hasRevisionedProperties() const;
    bool hasRevisionedMethods() const;

    QByteArray className;
    const QMetaObject *superClass;
    QMetaObjectBuilder::StaticMetacallFunction staticMetacallFunction;
    QList<QMetaMethodBuilderPrivate> methods;
    QList<QMetaMethodBuilderPrivate> constructors;
    QList<QMetaPropertyBuilderPrivate> properties;
    QList<QByteArray> classInfoNames;
    QList<QByteArray> classInfoValues;
    QList<QMetaEnumBuilderPrivate> enumerators;
    QList<const QMetaObject *> relatedMetaObjects;
    int flags;
};

bool QMetaObjectBuilderPrivate::hasRevisionedProperties() const
{
    for (int i = 0; i < properties.size(); ++i) {
        if (properties.at(i).revision)
            return true;
    }
    return false;
}

bool QMetaObjectBuilderPrivate::hasRevisionedMethods() const
{
    for (int i = 0; i < methods.size(); ++i) {
        if (methods.at(i).revision)
            return true;
    }
    return false;
}

/*!
    Constructs a new QMetaObjectBuilder.
*/
QMetaObjectBuilder::QMetaObjectBuilder()
{
    d = new QMetaObjectBuilderPrivate();
}

/*!
    Constructs a new QMetaObjectBuilder which is a copy of the
    meta object information in \a prototype.  Note: the super class
    contents for \a prototype are not copied, only the immediate
    class that is defined by \a prototype.

    The \a members parameter indicates which members of \a prototype
    should be added.  The default is AllMembers.

    \sa addMetaObject()
*/
QMetaObjectBuilder::QMetaObjectBuilder
    (const QMetaObject *prototype, QMetaObjectBuilder::AddMembers members)
{
    d = new QMetaObjectBuilderPrivate();
    addMetaObject(prototype, members);
}

/*!
    Destroys this meta object builder.
*/
QMetaObjectBuilder::~QMetaObjectBuilder()
{
    delete d;
}

/*!
    Returns the name of the class being constructed by this
    meta object builder.  The default value is an empty QByteArray.

    \sa setClassName(), superClass()
*/
QByteArray QMetaObjectBuilder::className() const
{
    return d->className;
}

/*!
    Sets the \a name of the class being constructed by this
    meta object builder.

    \sa className(), setSuperClass()
*/
void QMetaObjectBuilder::setClassName(const QByteArray& name)
{
    d->className = name;
}

/*!
    Returns the superclass meta object of the class being constructed
    by this meta object builder.  The default value is the meta object
    for QObject.

    \sa setSuperClass(), className()
*/
const QMetaObject *QMetaObjectBuilder::superClass() const
{
    return d->superClass;
}

/*!
    Sets the superclass meta object of the class being constructed
    by this meta object builder to \a meta.  The \a meta parameter
    must not be null.

    \sa superClass(), setClassName()
*/
void QMetaObjectBuilder::setSuperClass(const QMetaObject *meta)
{
    Q_ASSERT(meta);
    d->superClass = meta;
}

/*!
    Returns the flags of the class being constructed by this meta object
    builder.

    \sa setFlags()
*/
QMetaObjectBuilder::MetaObjectFlags QMetaObjectBuilder::flags() const
{
    return (QMetaObjectBuilder::MetaObjectFlags)d->flags;
}

/*!
    Sets the \a flags of the class being constructed by this meta object
    builder.

    \sa flags()
*/
void QMetaObjectBuilder::setFlags(MetaObjectFlags flags)
{
    d->flags = flags;
}

/*!
    Returns the number of methods in this class, excluding the number
    of methods in the base class.  These include signals and slots
    as well as normal member functions.

    \sa addMethod(), method(), removeMethod(), indexOfMethod()
*/
int QMetaObjectBuilder::methodCount() const
{
    return d->methods.size();
}

/*!
    Returns the number of constructors in this class.

    \sa addConstructor(), constructor(), removeConstructor(), indexOfConstructor()
*/
int QMetaObjectBuilder::constructorCount() const
{
    return d->constructors.size();
}

/*!
    Returns the number of properties in this class, excluding the number
    of properties in the base class.

    \sa addProperty(), property(), removeProperty(), indexOfProperty()
*/
int QMetaObjectBuilder::propertyCount() const
{
    return d->properties.size();
}

/*!
    Returns the number of enumerators in this class, excluding the
    number of enumerators in the base class.

    \sa addEnumerator(), enumerator(), removeEnumerator()
    \sa indexOfEnumerator()
*/
int QMetaObjectBuilder::enumeratorCount() const
{
    return d->enumerators.size();
}

/*!
    Returns the number of items of class information in this class,
    exclusing the number of items of class information in the base class.

    \sa addClassInfo(), classInfoName(), classInfoValue(), removeClassInfo()
    \sa indexOfClassInfo()
*/
int QMetaObjectBuilder::classInfoCount() const
{
    return d->classInfoNames.size();
}

/*!
    Returns the number of related meta objects that are associated
    with this class.

    Related meta objects are used when resolving the enumerated type
    associated with a property, where the enumerated type is in a
    different class from the property.

    \sa addRelatedMetaObject(), relatedMetaObject()
    \sa removeRelatedMetaObject()
*/
int QMetaObjectBuilder::relatedMetaObjectCount() const
{
    return d->relatedMetaObjects.size();
}

/*!
    Adds a new public method to this class with the specified \a signature.
    Returns an object that can be used to adjust the other attributes
    of the method.  The \a signature will be normalized before it is
    added to the class.

    \sa method(), methodCount(), removeMethod(), indexOfMethod()
*/
QMetaMethodBuilder QMetaObjectBuilder::addMethod(const QByteArray& signature)
{
    int index = d->methods.size();
    d->methods.append(QMetaMethodBuilderPrivate(QMetaMethod::Method, signature));
    return QMetaMethodBuilder(this, index);
}

/*!
    Adds a new public method to this class with the specified
    \a signature and \a returnType.  Returns an object that can be
    used to adjust the other attributes of the method.  The \a signature
    and \a returnType will be normalized before they are added to
    the class.

    \sa method(), methodCount(), removeMethod(), indexOfMethod()
*/
QMetaMethodBuilder QMetaObjectBuilder::addMethod
    (const QByteArray& signature, const QByteArray& returnType)
{
    int index = d->methods.size();
    d->methods.append(QMetaMethodBuilderPrivate
        (QMetaMethod::Method, signature, returnType));
    return QMetaMethodBuilder(this, index);
}

/*!
    Adds a new public method to this class that has the same information as
    \a prototype.  This is used to clone the methods of an existing
    QMetaObject.  Returns an object that can be used to adjust the
    attributes of the method.

    This function will detect if \a prototype is an ordinary method,
    signal, slot, or constructor and act accordingly.

    \sa method(), methodCount(), removeMethod(), indexOfMethod()
*/
QMetaMethodBuilder QMetaObjectBuilder::addMethod(const QMetaMethod& prototype)
{
    QMetaMethodBuilder method;
    if (prototype.methodType() == QMetaMethod::Method)
        method = addMethod(prototype.methodSignature());
    else if (prototype.methodType() == QMetaMethod::Signal)
        method = addSignal(prototype.methodSignature());
    else if (prototype.methodType() == QMetaMethod::Slot)
        method = addSlot(prototype.methodSignature());
    else if (prototype.methodType() == QMetaMethod::Constructor)
        method = addConstructor(prototype.methodSignature());
    method.setReturnType(prototype.typeName());
    method.setParameterNames(prototype.parameterNames());
    method.setTag(prototype.tag());
    method.setAccess(prototype.access());
    method.setAttributes(prototype.attributes());
    method.setRevision(prototype.revision());
    return method;
}

/*!
    Adds a new public slot to this class with the specified \a signature.
    Returns an object that can be used to adjust the other attributes
    of the slot.  The \a signature will be normalized before it is
    added to the class.

    \sa addMethod(), addSignal(), indexOfSlot()
*/
QMetaMethodBuilder QMetaObjectBuilder::addSlot(const QByteArray& signature)
{
    int index = d->methods.size();
    d->methods.append(QMetaMethodBuilderPrivate(QMetaMethod::Slot, signature));
    return QMetaMethodBuilder(this, index);
}

/*!
    Adds a new signal to this class with the specified \a signature.
    Returns an object that can be used to adjust the other attributes
    of the signal.  The \a signature will be normalized before it is
    added to the class.

    \sa addMethod(), addSlot(), indexOfSignal()
*/
QMetaMethodBuilder QMetaObjectBuilder::addSignal(const QByteArray& signature)
{
    int index = d->methods.size();
    d->methods.append(QMetaMethodBuilderPrivate
        (QMetaMethod::Signal, signature, QByteArray("void"), QMetaMethod::Public));
    return QMetaMethodBuilder(this, index);
}

/*!
    Adds a new constructor to this class with the specified \a signature.
    Returns an object that can be used to adjust the other attributes
    of the constructor.  The \a signature will be normalized before it is
    added to the class.

    \sa constructor(), constructorCount(), removeConstructor()
    \sa indexOfConstructor()
*/
QMetaMethodBuilder QMetaObjectBuilder::addConstructor(const QByteArray& signature)
{
    int index = d->constructors.size();
    d->constructors.append(QMetaMethodBuilderPrivate(QMetaMethod::Constructor, signature,
                                                     /*returnType=*/QByteArray()));
    return QMetaMethodBuilder(this, -(index + 1));
}

/*!
    Adds a new constructor to this class that has the same information as
    \a prototype.  This is used to clone the constructors of an existing
    QMetaObject.  Returns an object that can be used to adjust the
    attributes of the constructor.

    This function requires that \a prototype be a constructor.

    \sa constructor(), constructorCount(), removeConstructor()
    \sa indexOfConstructor()
*/
QMetaMethodBuilder QMetaObjectBuilder::addConstructor(const QMetaMethod& prototype)
{
    Q_ASSERT(prototype.methodType() == QMetaMethod::Constructor);
    QMetaMethodBuilder ctor = addConstructor(prototype.methodSignature());
    ctor.setReturnType(prototype.typeName());
    ctor.setParameterNames(prototype.parameterNames());
    ctor.setTag(prototype.tag());
    ctor.setAccess(prototype.access());
    ctor.setAttributes(prototype.attributes());
    return ctor;
}

/*!
    Adds a new readable/writable property to this class with the
    specified \a name and \a type.  Returns an object that can be used
    to adjust the other attributes of the property.  The \a type will
    be normalized before it is added to the class. \a notifierId will
    be registered as the property's \e notify signal.

    \sa property(), propertyCount(), removeProperty(), indexOfProperty()
*/
QMetaPropertyBuilder QMetaObjectBuilder::addProperty
    (const QByteArray& name, const QByteArray& type, int notifierId)
{
    int index = d->properties.size();
    d->properties.append(QMetaPropertyBuilderPrivate(name, type, notifierId));
    return QMetaPropertyBuilder(this, index);
}

/*!
    Adds a new property to this class that has the same information as
    \a prototype.  This is used to clone the properties of an existing
    QMetaObject.  Returns an object that can be used to adjust the
    attributes of the property.

    \sa property(), propertyCount(), removeProperty(), indexOfProperty()
*/
QMetaPropertyBuilder QMetaObjectBuilder::addProperty(const QMetaProperty& prototype)
{
    QMetaPropertyBuilder property = addProperty(prototype.name(), prototype.typeName());
    property.setReadable(prototype.isReadable());
    property.setWritable(prototype.isWritable());
    property.setResettable(prototype.isResettable());
    property.setDesignable(prototype.isDesignable());
    property.setScriptable(prototype.isScriptable());
    property.setStored(prototype.isStored());
    property.setEditable(prototype.isEditable());
    property.setUser(prototype.isUser());
    property.setStdCppSet(prototype.hasStdCppSet());
    property.setEnumOrFlag(prototype.isEnumType());
    property.setConstant(prototype.isConstant());
    property.setFinal(prototype.isFinal());
    property.setRevision(prototype.revision());
    if (prototype.hasNotifySignal()) {
        // Find an existing method for the notify signal, or add a new one.
        QMetaMethod method = prototype.notifySignal();
        int index = indexOfMethod(method.methodSignature());
        if (index == -1)
            index = addMethod(method).index();
        d->properties[property._index].notifySignal = index;
        d->properties[property._index].setFlag(Notify, true);
    }
    return property;
}

/*!
    Adds a new enumerator to this class with the specified
    \a name.  Returns an object that can be used to adjust
    the other attributes of the enumerator.

    \sa enumerator(), enumeratorCount(), removeEnumerator()
    \sa indexOfEnumerator()
*/
QMetaEnumBuilder QMetaObjectBuilder::addEnumerator(const QByteArray& name)
{
    int index = d->enumerators.size();
    d->enumerators.append(QMetaEnumBuilderPrivate(name));
    return QMetaEnumBuilder(this, index);
}

/*!
    Adds a new enumerator to this class that has the same information as
    \a prototype.  This is used to clone the enumerators of an existing
    QMetaObject.  Returns an object that can be used to adjust the
    attributes of the enumerator.

    \sa enumerator(), enumeratorCount(), removeEnumerator()
    \sa indexOfEnumerator()
*/
QMetaEnumBuilder QMetaObjectBuilder::addEnumerator(const QMetaEnum& prototype)
{
    QMetaEnumBuilder en = addEnumerator(prototype.name());
    en.setIsFlag(prototype.isFlag());
    int count = prototype.keyCount();
    for (int index = 0; index < count; ++index)
        en.addKey(prototype.key(index), prototype.value(index));
    return en;
}

/*!
    Adds \a name and \a value as an item of class information to this class.
    Returns the index of the new item of class information.

    \sa classInfoCount(), classInfoName(), classInfoValue(), removeClassInfo()
    \sa indexOfClassInfo()
*/
int QMetaObjectBuilder::addClassInfo(const QByteArray& name, const QByteArray& value)
{
    int index = d->classInfoNames.size();
    d->classInfoNames += name;
    d->classInfoValues += value;
    return index;
}

/*!
    Adds \a meta to this class as a related meta object.  Returns
    the index of the new related meta object entry.

    Related meta objects are used when resolving the enumerated type
    associated with a property, where the enumerated type is in a
    different class from the property.

    \sa relatedMetaObjectCount(), relatedMetaObject()
    \sa removeRelatedMetaObject()
*/
int QMetaObjectBuilder::addRelatedMetaObject(const QMetaObject *meta)
{
    Q_ASSERT(meta);
    int index = d->relatedMetaObjects.size();
    d->relatedMetaObjects.append(meta);
    return index;
}

/*!
    Adds the contents of \a prototype to this meta object builder.
    This function is useful for cloning the contents of an existing QMetaObject.

    The \a members parameter indicates which members of \a prototype
    should be added.  The default is AllMembers.
*/
void QMetaObjectBuilder::addMetaObject
        (const QMetaObject *prototype, QMetaObjectBuilder::AddMembers members)
{
    Q_ASSERT(prototype);
    int index;

    if ((members & ClassName) != 0)
        d->className = prototype->className();

    if ((members & SuperClass) != 0)
        d->superClass = prototype->superClass();

    if ((members & (Methods | Signals | Slots)) != 0) {
        for (index = prototype->methodOffset(); index < prototype->methodCount(); ++index) {
            QMetaMethod method = prototype->method(index);
            if (method.methodType() != QMetaMethod::Signal) {
                if (method.access() == QMetaMethod::Public && (members & PublicMethods) == 0)
                    continue;
                if (method.access() == QMetaMethod::Private && (members & PrivateMethods) == 0)
                    continue;
                if (method.access() == QMetaMethod::Protected && (members & ProtectedMethods) == 0)
                    continue;
            }
            if (method.methodType() == QMetaMethod::Method && (members & Methods) != 0) {
                addMethod(method);
            } else if (method.methodType() == QMetaMethod::Signal &&
                       (members & Signals) != 0) {
                addMethod(method);
            } else if (method.methodType() == QMetaMethod::Slot &&
                       (members & Slots) != 0) {
                addMethod(method);
            }
        }
    }

    if ((members & Constructors) != 0) {
        for (index = 0; index < prototype->constructorCount(); ++index)
            addConstructor(prototype->constructor(index));
    }

    if ((members & Properties) != 0) {
        for (index = prototype->propertyOffset(); index < prototype->propertyCount(); ++index)
            addProperty(prototype->property(index));
    }

    if ((members & Enumerators) != 0) {
        for (index = prototype->enumeratorOffset(); index < prototype->enumeratorCount(); ++index)
            addEnumerator(prototype->enumerator(index));
    }

    if ((members & ClassInfos) != 0) {
        for (index = prototype->classInfoOffset(); index < prototype->classInfoCount(); ++index) {
            QMetaClassInfo ci = prototype->classInfo(index);
            addClassInfo(ci.name(), ci.value());
        }
    }

    if ((members & RelatedMetaObjects) != 0) {
        Q_ASSERT(priv(prototype->d.data)->revision >= 2);
        const QMetaObject * const *objects = prototype->d.relatedMetaObjects;
        if (objects) {
            while (*objects != 0) {
                addRelatedMetaObject(*objects);
                ++objects;
            }
        }
    }

    if ((members & StaticMetacall) != 0) {
        Q_ASSERT(priv(prototype->d.data)->revision >= 6);
        if (prototype->d.static_metacall)
            setStaticMetacallFunction(prototype->d.static_metacall);
    }
}

/*!
    Returns the method at \a index in this class.

    \sa methodCount(), addMethod(), removeMethod(), indexOfMethod()
*/
QMetaMethodBuilder QMetaObjectBuilder::method(int index) const
{
    if (index >= 0 && index < d->methods.size())
        return QMetaMethodBuilder(this, index);
    else
        return QMetaMethodBuilder();
}

/*!
    Returns the constructor at \a index in this class.

    \sa methodCount(), addMethod(), removeMethod(), indexOfConstructor()
*/
QMetaMethodBuilder QMetaObjectBuilder::constructor(int index) const
{
    if (index >= 0 && index < d->constructors.size())
        return QMetaMethodBuilder(this, -(index + 1));
    else
        return QMetaMethodBuilder();
}

/*!
    Returns the property at \a index in this class.

    \sa methodCount(), addMethod(), removeMethod(), indexOfProperty()
*/
QMetaPropertyBuilder QMetaObjectBuilder::property(int index) const
{
    if (index >= 0 && index < d->properties.size())
        return QMetaPropertyBuilder(this, index);
    else
        return QMetaPropertyBuilder();
}

/*!
    Returns the enumerator at \a index in this class.

    \sa enumeratorCount(), addEnumerator(), removeEnumerator()
    \sa indexOfEnumerator()
*/
QMetaEnumBuilder QMetaObjectBuilder::enumerator(int index) const
{
    if (index >= 0 && index < d->enumerators.size())
        return QMetaEnumBuilder(this, index);
    else
        return QMetaEnumBuilder();
}

/*!
    Returns the related meta object at \a index in this class.

    Related meta objects are used when resolving the enumerated type
    associated with a property, where the enumerated type is in a
    different class from the property.

    \sa relatedMetaObjectCount(), addRelatedMetaObject()
    \sa removeRelatedMetaObject()
*/
const QMetaObject *QMetaObjectBuilder::relatedMetaObject(int index) const
{
    if (index >= 0 && index < d->relatedMetaObjects.size())
        return d->relatedMetaObjects[index];
    else
        return 0;
}

/*!
    Returns the name of the item of class information at \a index
    in this class.

    \sa classInfoCount(), addClassInfo(), classInfoValue(), removeClassInfo()
    \sa indexOfClassInfo()
*/
QByteArray QMetaObjectBuilder::classInfoName(int index) const
{
    if (index >= 0 && index < d->classInfoNames.size())
        return d->classInfoNames[index];
    else
        return QByteArray();
}

/*!
    Returns the value of the item of class information at \a index
    in this class.

    \sa classInfoCount(), addClassInfo(), classInfoName(), removeClassInfo()
    \sa indexOfClassInfo()
*/
QByteArray QMetaObjectBuilder::classInfoValue(int index) const
{
    if (index >= 0 && index < d->classInfoValues.size())
        return d->classInfoValues[index];
    else
        return QByteArray();
}

/*!
    Removes the method at \a index from this class.  The indices of
    all following methods will be adjusted downwards by 1.  If the
    method is registered as a notify signal on a property, then the
    notify signal will be removed from the property.

    \sa methodCount(), addMethod(), method(), indexOfMethod()
*/
void QMetaObjectBuilder::removeMethod(int index)
{
    if (index >= 0 && index < d->methods.size()) {
        d->methods.removeAt(index);
        for (int prop = 0; prop < d->properties.size(); ++prop) {
            // Adjust the indices of property notify signal references.
            if (d->properties[prop].notifySignal == index) {
                d->properties[prop].notifySignal = -1;
                d->properties[prop].setFlag(Notify, false);
            } else if (d->properties[prop].notifySignal > index)
                (d->properties[prop].notifySignal)--;
        }
    }
}

/*!
    Removes the constructor at \a index from this class.  The indices of
    all following constructors will be adjusted downwards by 1.

    \sa constructorCount(), addConstructor(), constructor()
    \sa indexOfConstructor()
*/
void QMetaObjectBuilder::removeConstructor(int index)
{
    if (index >= 0 && index < d->constructors.size())
        d->constructors.removeAt(index);
}

/*!
    Removes the property at \a index from this class.  The indices of
    all following properties will be adjusted downwards by 1.

    \sa propertyCount(), addProperty(), property(), indexOfProperty()
*/
void QMetaObjectBuilder::removeProperty(int index)
{
    if (index >= 0 && index < d->properties.size())
        d->properties.removeAt(index);
}

/*!
    Removes the enumerator at \a index from this class.  The indices of
    all following enumerators will be adjusted downwards by 1.

    \sa enumertorCount(), addEnumerator(), enumerator()
    \sa indexOfEnumerator()
*/
void QMetaObjectBuilder::removeEnumerator(int index)
{
    if (index >= 0 && index < d->enumerators.size())
        d->enumerators.removeAt(index);
}

/*!
    Removes the item of class information at \a index from this class.
    The indices of all following items will be adjusted downwards by 1.

    \sa classInfoCount(), addClassInfo(), classInfoName(), classInfoValue()
    \sa indexOfClassInfo()
*/
void QMetaObjectBuilder::removeClassInfo(int index)
{
    if (index >= 0 && index < d->classInfoNames.size()) {
        d->classInfoNames.removeAt(index);
        d->classInfoValues.removeAt(index);
    }
}

/*!
    Removes the related meta object at \a index from this class.
    The indices of all following related meta objects will be adjusted
    downwards by 1.

    Related meta objects are used when resolving the enumerated type
    associated with a property, where the enumerated type is in a
    different class from the property.

    \sa relatedMetaObjectCount(), addRelatedMetaObject()
    \sa relatedMetaObject()
*/
void QMetaObjectBuilder::removeRelatedMetaObject(int index)
{
    if (index >= 0 && index < d->relatedMetaObjects.size())
        d->relatedMetaObjects.removeAt(index);
}

/*!
    Finds a method with the specified \a signature and returns its index;
    otherwise returns -1.  The \a signature will be normalized by this method.

    \sa method(), methodCount(), addMethod(), removeMethod()
*/
int QMetaObjectBuilder::indexOfMethod(const QByteArray& signature)
{
    QByteArray sig = QMetaObject::normalizedSignature(signature);
    for (int index = 0; index < d->methods.size(); ++index) {
        if (sig == d->methods[index].signature)
            return index;
    }
    return -1;
}

/*!
    Finds a signal with the specified \a signature and returns its index;
    otherwise returns -1.  The \a signature will be normalized by this method.

    \sa indexOfMethod(), indexOfSlot()
*/
int QMetaObjectBuilder::indexOfSignal(const QByteArray& signature)
{
    QByteArray sig = QMetaObject::normalizedSignature(signature);
    for (int index = 0; index < d->methods.size(); ++index) {
        if (sig == d->methods[index].signature &&
                d->methods[index].methodType() == QMetaMethod::Signal)
            return index;
    }
    return -1;
}

/*!
    Finds a slot with the specified \a signature and returns its index;
    otherwise returns -1.  The \a signature will be normalized by this method.

    \sa indexOfMethod(), indexOfSignal()
*/
int QMetaObjectBuilder::indexOfSlot(const QByteArray& signature)
{
    QByteArray sig = QMetaObject::normalizedSignature(signature);
    for (int index = 0; index < d->methods.size(); ++index) {
        if (sig == d->methods[index].signature &&
                d->methods[index].methodType() == QMetaMethod::Slot)
            return index;
    }
    return -1;
}

/*!
    Finds a constructor with the specified \a signature and returns its index;
    otherwise returns -1.  The \a signature will be normalized by this method.

    \sa constructor(), constructorCount(), addConstructor(), removeConstructor()
*/
int QMetaObjectBuilder::indexOfConstructor(const QByteArray& signature)
{
    QByteArray sig = QMetaObject::normalizedSignature(signature);
    for (int index = 0; index < d->constructors.size(); ++index) {
        if (sig == d->constructors[index].signature)
            return index;
    }
    return -1;
}

/*!
    Finds a property with the specified \a name and returns its index;
    otherwise returns -1.

    \sa property(), propertyCount(), addProperty(), removeProperty()
*/
int QMetaObjectBuilder::indexOfProperty(const QByteArray& name)
{
    for (int index = 0; index < d->properties.size(); ++index) {
        if (name == d->properties[index].name)
            return index;
    }
    return -1;
}

/*!
    Finds an enumerator with the specified \a name and returns its index;
    otherwise returns -1.

    \sa enumertor(), enumeratorCount(), addEnumerator(), removeEnumerator()
*/
int QMetaObjectBuilder::indexOfEnumerator(const QByteArray& name)
{
    for (int index = 0; index < d->enumerators.size(); ++index) {
        if (name == d->enumerators[index].name)
            return index;
    }
    return -1;
}

/*!
    Finds an item of class information with the specified \a name and
    returns its index; otherwise returns -1.

    \sa classInfoName(), classInfoValue(), classInfoCount(), addClassInfo()
    \sa removeClassInfo()
*/
int QMetaObjectBuilder::indexOfClassInfo(const QByteArray& name)
{
    for (int index = 0; index < d->classInfoNames.size(); ++index) {
        if (name == d->classInfoNames[index])
            return index;
    }
    return -1;
}

// Align on a specific type boundary.
#define ALIGN(size,type)    \
    (size) = ((size) + sizeof(type) - 1) & ~(sizeof(type) - 1)

/*!
    \class QMetaStringTable
    \inmodule QtCore
    \internal
    \brief The QMetaStringTable class can generate a meta-object string table at runtime.
*/

QMetaStringTable::QMetaStringTable(const QByteArray &className)
    : m_index(0)
    , m_className(className)
{
    const int index = enter(m_className);
    Q_ASSERT(index == 0);
    Q_UNUSED(index);
}

// Enters the given value into the string table (if it hasn't already been
// entered). Returns the index of the string.
int QMetaStringTable::enter(const QByteArray &value)
{
    Entries::iterator it = m_entries.find(value);
    if (it != m_entries.end())
        return it.value();
    int pos = m_index;
    m_entries.insert(value, pos);
    ++m_index;
    return pos;
}

int QMetaStringTable::preferredAlignment()
{
    return Q_ALIGNOF(QByteArrayData);
}

// Returns the size (in bytes) required for serializing this string table.
int QMetaStringTable::blobSize() const
{
    int size = m_entries.size() * sizeof(QByteArrayData);
    Entries::const_iterator it;
    for (it = m_entries.constBegin(); it != m_entries.constEnd(); ++it)
        size += it.key().size() + 1;
    return size;
}

static void writeString(char *out, int i, const QByteArray &str,
                        const int offsetOfStringdataMember, int &stringdataOffset)
{
    int size = str.size();
    qptrdiff offset = offsetOfStringdataMember + stringdataOffset
            - i * sizeof(QByteArrayData);
    const QByteArrayData data =
        Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(size, offset);

    memcpy(out + i * sizeof(QByteArrayData), &data, sizeof(QByteArrayData));

    memcpy(out + offsetOfStringdataMember + stringdataOffset, str.constData(), size);
    out[offsetOfStringdataMember + stringdataOffset + size] = '\0';

    stringdataOffset += size + 1;
}

// Writes strings to string data struct.
// The struct consists of an array of QByteArrayData, followed by a char array
// containing the actual strings. This format must match the one produced by
// moc (see generator.cpp).
void QMetaStringTable::writeBlob(char *out) const
{
    Q_ASSERT(!(reinterpret_cast<quintptr>(out) & (preferredAlignment()-1)));

    int offsetOfStringdataMember = m_entries.size() * sizeof(QByteArrayData);
    int stringdataOffset = 0;

    // qt_metacast expects the first string in the string table to be the class name.
    writeString(out, /*index*/0, m_className, offsetOfStringdataMember, stringdataOffset);

    for (Entries::ConstIterator it = m_entries.constBegin(), end = m_entries.constEnd();
         it != end; ++it) {
        const int i = it.value();
        if (i == 0)
            continue;
        const QByteArray &str = it.key();

        writeString(out, i, str, offsetOfStringdataMember, stringdataOffset);
    }
}

// Returns the sum of all parameters (including return type) for the given
// \a methods. This is needed for calculating the size of the methods'
// parameter type/name meta-data.
static int aggregateParameterCount(const QList<QMetaMethodBuilderPrivate> &methods)
{
    int sum = 0;
    for (int i = 0; i < methods.size(); ++i)
        sum += methods.at(i).parameterCount() + 1; // +1 for return type
    return sum;
}

// Build a QMetaObject in "buf" based on the information in "d".
// If "buf" is null, then return the number of bytes needed to
// build the QMetaObject.  Returns -1 if the metaobject if
// relocatable is set, but the metaobject contains relatedMetaObjects.
static int buildMetaObject(QMetaObjectBuilderPrivate *d, char *buf,
                           int expectedSize, bool relocatable)
{
    Q_UNUSED(expectedSize); // Avoid warning in release mode
    int size = 0;
    int dataIndex;
    int paramsIndex;
    int enumIndex;
    int index;
    bool hasRevisionedMethods = d->hasRevisionedMethods();
    bool hasRevisionedProperties = d->hasRevisionedProperties();
    bool hasNotifySignals = false;

    if (relocatable &&
        (d->relatedMetaObjects.size() > 0 || d->staticMetacallFunction))
        return -1;

    // Create the main QMetaObject structure at the start of the buffer.
    QMetaObject *meta = reinterpret_cast<QMetaObject *>(buf);
    size += sizeof(QMetaObject);
    ALIGN(size, int);
    if (buf) {
        if (!relocatable) meta->d.superdata = d->superClass;
        meta->d.relatedMetaObjects = 0;
        meta->d.extradata = 0;
        meta->d.static_metacall = d->staticMetacallFunction;
    }

    // Populate the QMetaObjectPrivate structure.
    QMetaObjectPrivate *pmeta
        = reinterpret_cast<QMetaObjectPrivate *>(buf + size);
    int pmetaSize = size;
    dataIndex = MetaObjectPrivateFieldCount;
    for (index = 0; index < d->properties.size(); ++index) {
        if (d->properties[index].notifySignal != -1) {
            hasNotifySignals = true;
            break;
        }
    }
    int methodParametersDataSize =
            ((aggregateParameterCount(d->methods)
             + aggregateParameterCount(d->constructors)) * 2) // types and parameter names
            - d->methods.size()       // return "parameters" don't have names
            - d->constructors.size(); // "this" parameters don't have names
    if (buf) {
        Q_STATIC_ASSERT_X(QMetaObjectPrivate::OutputRevision == 7, "QMetaObjectBuilder should generate the same version as moc");
        pmeta->revision = QMetaObjectPrivate::OutputRevision;
        pmeta->flags = d->flags;
        pmeta->className = 0;   // Class name is always the first string.
        //pmeta->signalCount is handled in the "output method loop" as an optimization.

        pmeta->classInfoCount = d->classInfoNames.size();
        pmeta->classInfoData = dataIndex;
        dataIndex += 2 * d->classInfoNames.size();

        pmeta->methodCount = d->methods.size();
        pmeta->methodData = dataIndex;
        dataIndex += 5 * d->methods.size();
        if (hasRevisionedMethods)
            dataIndex += d->methods.size();
        paramsIndex = dataIndex;
        dataIndex += methodParametersDataSize;

        pmeta->propertyCount = d->properties.size();
        pmeta->propertyData = dataIndex;
        dataIndex += 3 * d->properties.size();
        if (hasNotifySignals)
            dataIndex += d->properties.size();
        if (hasRevisionedProperties)
            dataIndex += d->properties.size();

        pmeta->enumeratorCount = d->enumerators.size();
        pmeta->enumeratorData = dataIndex;
        dataIndex += 4 * d->enumerators.size();

        pmeta->constructorCount = d->constructors.size();
        pmeta->constructorData = dataIndex;
        dataIndex += 5 * d->constructors.size();
    } else {
        dataIndex += 2 * d->classInfoNames.size();
        dataIndex += 5 * d->methods.size();
        if (hasRevisionedMethods)
            dataIndex += d->methods.size();
        paramsIndex = dataIndex;
        dataIndex += methodParametersDataSize;
        dataIndex += 3 * d->properties.size();
        if (hasNotifySignals)
            dataIndex += d->properties.size();
        if (hasRevisionedProperties)
            dataIndex += d->properties.size();
        dataIndex += 4 * d->enumerators.size();
        dataIndex += 5 * d->constructors.size();
    }

    // Allocate space for the enumerator key names and values.
    enumIndex = dataIndex;
    for (index = 0; index < d->enumerators.size(); ++index) {
        QMetaEnumBuilderPrivate *enumerator = &(d->enumerators[index]);
        dataIndex += 2 * enumerator->keys.size();
    }

    // Zero terminator at the end of the data offset table.
    ++dataIndex;

    // Find the start of the data and string tables.
    int *data = reinterpret_cast<int *>(pmeta);
    size += dataIndex * sizeof(int);
    ALIGN(size, void *);
    char *str = reinterpret_cast<char *>(buf + size);
    if (buf) {
        if (relocatable) {
            meta->d.stringdata = reinterpret_cast<const QByteArrayData *>((quintptr)size);
            meta->d.data = reinterpret_cast<uint *>((quintptr)pmetaSize);
        } else {
            meta->d.stringdata = reinterpret_cast<const QByteArrayData *>(str);
            meta->d.data = reinterpret_cast<uint *>(data);
        }
    }

    // Reset the current data position to just past the QMetaObjectPrivate.
    dataIndex = MetaObjectPrivateFieldCount;

    QMetaStringTable strings(d->className);

    // Output the class infos,
    Q_ASSERT(!buf || dataIndex == pmeta->classInfoData);
    for (index = 0; index < d->classInfoNames.size(); ++index) {
        int name = strings.enter(d->classInfoNames[index]);
        int value = strings.enter(d->classInfoValues[index]);
        if (buf) {
            data[dataIndex] = name;
            data[dataIndex + 1] = value;
        }
        dataIndex += 2;
    }

    // Output the methods in the class.
    Q_ASSERT(!buf || dataIndex == pmeta->methodData);
    for (index = 0; index < d->methods.size(); ++index) {
        QMetaMethodBuilderPrivate *method = &(d->methods[index]);
        int name = strings.enter(method->name());
        int argc = method->parameterCount();
        int tag = strings.enter(method->tag);
        int attrs = method->attributes;
        if (buf) {
            data[dataIndex]     = name;
            data[dataIndex + 1] = argc;
            data[dataIndex + 2] = paramsIndex;
            data[dataIndex + 3] = tag;
            data[dataIndex + 4] = attrs;
            if (method->methodType() == QMetaMethod::Signal)
                pmeta->signalCount++;
        }
        dataIndex += 5;
        paramsIndex += 1 + argc * 2;
    }
    if (hasRevisionedMethods) {
        for (index = 0; index < d->methods.size(); ++index) {
            QMetaMethodBuilderPrivate *method = &(d->methods[index]);
            if (buf)
                data[dataIndex] = method->revision;
            ++dataIndex;
        }
    }

    // Output the method parameters in the class.
    Q_ASSERT(!buf || dataIndex == pmeta->methodData + d->methods.size() * 5
             + (hasRevisionedMethods ? d->methods.size() : 0));
    for (int x = 0; x < 2; ++x) {
        QList<QMetaMethodBuilderPrivate> &methods = (x == 0) ? d->methods : d->constructors;
        for (index = 0; index < methods.size(); ++index) {
            QMetaMethodBuilderPrivate *method = &(methods[index]);
            QList<QByteArray> paramTypeNames = method->parameterTypes();
            int paramCount = paramTypeNames.size();
            for (int i = -1; i < paramCount; ++i) {
                const QByteArray &typeName = (i < 0) ? method->returnType : paramTypeNames.at(i);
                int typeInfo;
                if (QtPrivate::isBuiltinType(typeName))
                    typeInfo = QMetaType::type(typeName);
                else
                    typeInfo = IsUnresolvedType | strings.enter(typeName);
                if (buf)
                    data[dataIndex] = typeInfo;
                ++dataIndex;
            }

            QList<QByteArray> paramNames = method->parameterNames;
            while (paramNames.size() < paramCount)
                paramNames.append(QByteArray());
            for (int i = 0; i < paramCount; ++i) {
                int stringIndex = strings.enter(paramNames.at(i));
                if (buf)
                    data[dataIndex] = stringIndex;
                ++dataIndex;
            }
        }
    }

    // Output the properties in the class.
    Q_ASSERT(!buf || dataIndex == pmeta->propertyData);
    for (index = 0; index < d->properties.size(); ++index) {
        QMetaPropertyBuilderPrivate *prop = &(d->properties[index]);
        int name = strings.enter(prop->name);

        int typeInfo;
        if (QtPrivate::isBuiltinType(prop->type))
            typeInfo = QMetaType::type(prop->type);
        else
            typeInfo = IsUnresolvedType | strings.enter(prop->type);

        int flags = prop->flags;

        if (!QtPrivate::isBuiltinType(prop->type))
            flags |= EnumOrFlag;

        if (buf) {
            data[dataIndex]     = name;
            data[dataIndex + 1] = typeInfo;
            data[dataIndex + 2] = flags;
        }
        dataIndex += 3;
    }
    if (hasNotifySignals) {
        for (index = 0; index < d->properties.size(); ++index) {
            QMetaPropertyBuilderPrivate *prop = &(d->properties[index]);
            if (buf) {
                if (prop->notifySignal != -1)
                    data[dataIndex] = prop->notifySignal;
                else
                    data[dataIndex] = 0;
            }
            ++dataIndex;
        }
    }
    if (hasRevisionedProperties) {
        for (index = 0; index < d->properties.size(); ++index) {
            QMetaPropertyBuilderPrivate *prop = &(d->properties[index]);
            if (buf)
                data[dataIndex] = prop->revision;
            ++dataIndex;
        }
    }

    // Output the enumerators in the class.
    Q_ASSERT(!buf || dataIndex == pmeta->enumeratorData);
    for (index = 0; index < d->enumerators.size(); ++index) {
        QMetaEnumBuilderPrivate *enumerator = &(d->enumerators[index]);
        int name = strings.enter(enumerator->name);
        int isFlag = (int)(enumerator->isFlag);
        int count = enumerator->keys.size();
        int enumOffset = enumIndex;
        if (buf) {
            data[dataIndex]     = name;
            data[dataIndex + 1] = isFlag;
            data[dataIndex + 2] = count;
            data[dataIndex + 3] = enumOffset;
        }
        for (int key = 0; key < count; ++key) {
            int keyIndex = strings.enter(enumerator->keys[key]);
            if (buf) {
                data[enumOffset++] = keyIndex;
                data[enumOffset++] = enumerator->values[key];
            }
        }
        dataIndex += 4;
        enumIndex += 2 * count;
    }

    // Output the constructors in the class.
    Q_ASSERT(!buf || dataIndex == pmeta->constructorData);
    for (index = 0; index < d->constructors.size(); ++index) {
        QMetaMethodBuilderPrivate *method = &(d->constructors[index]);
        int name = strings.enter(method->name());
        int argc = method->parameterCount();
        int tag = strings.enter(method->tag);
        int attrs = method->attributes;
        if (buf) {
            data[dataIndex]     = name;
            data[dataIndex + 1] = argc;
            data[dataIndex + 2] = paramsIndex;
            data[dataIndex + 3] = tag;
            data[dataIndex + 4] = attrs;
        }
        dataIndex += 5;
        paramsIndex += 1 + argc * 2;
    }

    size += strings.blobSize();

    if (buf)
        strings.writeBlob(str);

    // Output the zero terminator in the data array.
    if (buf)
        data[enumIndex] = 0;

    // Create the relatedMetaObjects block if we need one.
    if (d->relatedMetaObjects.size() > 0) {
        ALIGN(size, QMetaObject *);
        const QMetaObject **objects =
            reinterpret_cast<const QMetaObject **>(buf + size);
        if (buf) {
            meta->d.relatedMetaObjects = objects;
            for (index = 0; index < d->relatedMetaObjects.size(); ++index)
                objects[index] = d->relatedMetaObjects[index];
            objects[index] = 0;
        }
        size += sizeof(QMetaObject *) * (d->relatedMetaObjects.size() + 1);
    }

    // Align the final size and return it.
    ALIGN(size, void *);
    Q_ASSERT(!buf || size == expectedSize);
    return size;
}

/*!
    Converts this meta object builder into a concrete QMetaObject.
    The return value should be deallocated using free() once it
    is no longer needed.

    The returned meta object is a snapshot of the state of the
    QMetaObjectBuilder.  Any further modifications to the QMetaObjectBuilder
    will not be reflected in previous meta objects returned by
    this method.
*/
QMetaObject *QMetaObjectBuilder::toMetaObject() const
{
    int size = buildMetaObject(d, 0, 0, false);
    char *buf = reinterpret_cast<char *>(malloc(size));
    memset(buf, 0, size);
    buildMetaObject(d, buf, size, false);
    return reinterpret_cast<QMetaObject *>(buf);
}

/*
    \internal

    Converts this meta object builder into relocatable data.  This data can
    be stored, copied and later passed to fromRelocatableData() to create a
    concrete QMetaObject.

    The data is specific to the architecture on which it was created, but is not
    specific to the process that created it.  Not all meta object builder's can
    be converted to data in this way.  If \a ok is provided, it will be set to
    true if the conversion succeeds, and false otherwise.  If a
    staticMetacallFunction() or any relatedMetaObject()'s are specified the
    conversion to relocatable data will fail.
*/
QByteArray QMetaObjectBuilder::toRelocatableData(bool *ok) const
{
    int size = buildMetaObject(d, 0, 0, true);
    if (size == -1) {
        if (ok) *ok = false;
        return QByteArray();
    }

    QByteArray data;
    data.resize(size);
    char *buf = data.data();
    memset(buf, 0, size);
    buildMetaObject(d, buf, size, true);
    if (ok) *ok = true;
    return data;
}

/*
    \internal

    Sets the \a data returned from toRelocatableData() onto a concrete
    QMetaObject instance, \a output.  As the meta object's super class is not
    saved in the relocatable data, it must be passed as \a superClass.
*/
void QMetaObjectBuilder::fromRelocatableData(QMetaObject *output,
                                             const QMetaObject *superclass,
                                             const QByteArray &data)
{
    if (!output)
        return;

    const char *buf = data.constData();
    const QMetaObject *dataMo = reinterpret_cast<const QMetaObject *>(buf);

    quintptr stringdataOffset = (quintptr)dataMo->d.stringdata;
    quintptr dataOffset = (quintptr)dataMo->d.data;

    output->d.superdata = superclass;
    output->d.stringdata = reinterpret_cast<const QByteArrayData *>(buf + stringdataOffset);
    output->d.data = reinterpret_cast<const uint *>(buf + dataOffset);
    output->d.extradata = 0;
    output->d.relatedMetaObjects = 0;
    output->d.static_metacall = 0;
}

/*!
    \typedef QMetaObjectBuilder::StaticMetacallFunction

    Typedef for static metacall functions.  The three parameters are
    the call type value, the constructor index, and the
    array of parameters.
*/

/*!
    Returns the static metacall function to use to construct objects
    of this class.  The default value is null.

    \sa setStaticMetacallFunction()
*/
QMetaObjectBuilder::StaticMetacallFunction QMetaObjectBuilder::staticMetacallFunction() const
{
    return d->staticMetacallFunction;
}

/*!
    Sets the static metacall function to use to construct objects
    of this class to \a value.  The default value is null.

    \sa staticMetacallFunction()
*/
void QMetaObjectBuilder::setStaticMetacallFunction
        (QMetaObjectBuilder::StaticMetacallFunction value)
{
    d->staticMetacallFunction = value;
}

#ifndef QT_NO_DATASTREAM

/*!
    Serializes the contents of the meta object builder onto \a stream.

    \sa deserialize()
*/
void QMetaObjectBuilder::serialize(QDataStream& stream) const
{
    int index;

    // Write the class and super class names.
    stream << d->className;
    if (d->superClass)
        stream << QByteArray(d->superClass->className());
    else
        stream << QByteArray();

    // Write the counts for each type of class member.
    stream << d->classInfoNames.size();
    stream << d->methods.size();
    stream << d->properties.size();
    stream << d->enumerators.size();
    stream << d->constructors.size();
    stream << d->relatedMetaObjects.size();

    // Write the items of class information.
    for (index = 0; index < d->classInfoNames.size(); ++index) {
        stream << d->classInfoNames[index];
        stream << d->classInfoValues[index];
    }

    // Write the methods.
    for (index = 0; index < d->methods.size(); ++index) {
        const QMetaMethodBuilderPrivate *method = &(d->methods[index]);
        stream << method->signature;
        stream << method->returnType;
        stream << method->parameterNames;
        stream << method->tag;
        stream << method->attributes;
        if (method->revision)
            stream << method->revision;
    }

    // Write the properties.
    for (index = 0; index < d->properties.size(); ++index) {
        const QMetaPropertyBuilderPrivate *property = &(d->properties[index]);
        stream << property->name;
        stream << property->type;
        stream << property->flags;
        stream << property->notifySignal;
        if (property->revision)
            stream << property->revision;
    }

    // Write the enumerators.
    for (index = 0; index < d->enumerators.size(); ++index) {
        const QMetaEnumBuilderPrivate *enumerator = &(d->enumerators[index]);
        stream << enumerator->name;
        stream << enumerator->isFlag;
        stream << enumerator->keys;
        stream << enumerator->values;
    }

    // Write the constructors.
    for (index = 0; index < d->constructors.size(); ++index) {
        const QMetaMethodBuilderPrivate *method = &(d->constructors[index]);
        stream << method->signature;
        stream << method->returnType;
        stream << method->parameterNames;
        stream << method->tag;
        stream << method->attributes;
    }

    // Write the related meta objects.
    for (index = 0; index < d->relatedMetaObjects.size(); ++index) {
        const QMetaObject *meta = d->relatedMetaObjects[index];
        stream << QByteArray(meta->className());
    }

    // Add an extra empty QByteArray for additional data in future versions.
    // This should help maintain backwards compatibility, allowing older
    // versions to read newer data.
    stream << QByteArray();
}

// Resolve a class name using the name reference map.
static const QMetaObject *resolveClassName
        (const QMap<QByteArray, const QMetaObject *>& references,
         const QByteArray& name)
{
    if (name == QByteArray("QObject"))
        return &QObject::staticMetaObject;
    else
        return references.value(name, 0);
}

/*!
    Deserializes a meta object builder from \a stream into
    this meta object builder.

    The \a references parameter specifies a mapping from class names
    to QMetaObject instances for resolving the super class name and
    related meta objects in the object that is deserialized.
    The meta object for QObject is implicitly added to \a references
    and does not need to be supplied.

    The QDataStream::status() value on \a stream will be set to
    QDataStream::ReadCorruptData if the input data is corrupt.
    The status will be set to QDataStream::ReadPastEnd if the
    input was exhausted before the full meta object was read.

    \sa serialize()
*/
void QMetaObjectBuilder::deserialize
        (QDataStream& stream,
         const QMap<QByteArray, const QMetaObject *>& references)
{
    QByteArray name;
    const QMetaObject *cl;
    int index;

    // Clear all members in the builder to their default states.
    d->className.clear();
    d->superClass = &QObject::staticMetaObject;
    d->classInfoNames.clear();
    d->classInfoValues.clear();
    d->methods.clear();
    d->properties.clear();
    d->enumerators.clear();
    d->constructors.clear();
    d->relatedMetaObjects.clear();
    d->staticMetacallFunction = 0;

    // Read the class and super class names.
    stream >> d->className;
    stream >> name;
    if (name.isEmpty()) {
        d->superClass = 0;
    } else if ((cl = resolveClassName(references, name)) != 0) {
        d->superClass = cl;
    } else {
        stream.setStatus(QDataStream::ReadCorruptData);
        return;
    }

    // Read the counts for each type of class member.
    int classInfoCount, methodCount, propertyCount;
    int enumeratorCount, constructorCount, relatedMetaObjectCount;
    stream >> classInfoCount;
    stream >> methodCount;
    stream >> propertyCount;
    stream >> enumeratorCount;
    stream >> constructorCount;
    stream >> relatedMetaObjectCount;
    if (classInfoCount < 0 || methodCount < 0 ||
        propertyCount < 0 || enumeratorCount < 0 ||
        constructorCount < 0 || relatedMetaObjectCount < 0) {
        stream.setStatus(QDataStream::ReadCorruptData);
        return;
    }

    // Read the items of class information.
    for (index = 0; index < classInfoCount; ++index) {
        if (stream.status() != QDataStream::Ok)
            return;
        QByteArray value;
        stream >> name;
        stream >> value;
        addClassInfo(name, value);
    }

    // Read the member methods.
    for (index = 0; index < methodCount; ++index) {
        if (stream.status() != QDataStream::Ok)
            return;
        stream >> name;
        addMethod(name);
        QMetaMethodBuilderPrivate *method = &(d->methods[index]);
        stream >> method->returnType;
        stream >> method->parameterNames;
        stream >> method->tag;
        stream >> method->attributes;
        if (method->attributes & MethodRevisioned)
            stream >> method->revision;
        if (method->methodType() == QMetaMethod::Constructor) {
            // Cannot add a constructor in this set of methods.
            stream.setStatus(QDataStream::ReadCorruptData);
            return;
        }
    }

    // Read the properties.
    for (index = 0; index < propertyCount; ++index) {
        if (stream.status() != QDataStream::Ok)
            return;
        QByteArray type;
        stream >> name;
        stream >> type;
        addProperty(name, type);
        QMetaPropertyBuilderPrivate *property = &(d->properties[index]);
        stream >> property->flags;
        stream >> property->notifySignal;
        if (property->notifySignal < -1 ||
            property->notifySignal >= d->methods.size()) {
            // Notify signal method index is out of range.
            stream.setStatus(QDataStream::ReadCorruptData);
            return;
        }
        if (property->notifySignal >= 0 &&
            d->methods[property->notifySignal].methodType() != QMetaMethod::Signal) {
            // Notify signal method index does not refer to a signal.
            stream.setStatus(QDataStream::ReadCorruptData);
            return;
        }
        if (property->flags & Revisioned)
            stream >> property->revision;
    }

    // Read the enumerators.
    for (index = 0; index < enumeratorCount; ++index) {
        if (stream.status() != QDataStream::Ok)
            return;
        stream >> name;
        addEnumerator(name);
        QMetaEnumBuilderPrivate *enumerator = &(d->enumerators[index]);
        stream >> enumerator->isFlag;
        stream >> enumerator->keys;
        stream >> enumerator->values;
        if (enumerator->keys.size() != enumerator->values.size()) {
            // Mismatch between number of keys and number of values.
            stream.setStatus(QDataStream::ReadCorruptData);
            return;
        }
    }

    // Read the constructor methods.
    for (index = 0; index < constructorCount; ++index) {
        if (stream.status() != QDataStream::Ok)
            return;
        stream >> name;
        addConstructor(name);
        QMetaMethodBuilderPrivate *method = &(d->constructors[index]);
        stream >> method->returnType;
        stream >> method->parameterNames;
        stream >> method->tag;
        stream >> method->attributes;
        if (method->methodType() != QMetaMethod::Constructor) {
            // The type must be Constructor.
            stream.setStatus(QDataStream::ReadCorruptData);
            return;
        }
    }

    // Read the related meta objects.
    for (index = 0; index < relatedMetaObjectCount; ++index) {
        if (stream.status() != QDataStream::Ok)
            return;
        stream >> name;
        cl = resolveClassName(references, name);
        if (!cl) {
            stream.setStatus(QDataStream::ReadCorruptData);
            return;
        }
        addRelatedMetaObject(cl);
    }

    // Read the extra data block, which is reserved for future use.
    stream >> name;
}

#endif // !QT_NO_DATASTREAM

/*!
    \class QMetaMethodBuilder
    \inmodule QtCore
    \internal
    \brief The QMetaMethodBuilder class enables modifications to a method definition on a meta object builder.
*/

QMetaMethodBuilderPrivate *QMetaMethodBuilder::d_func() const
{
    // Positive indices indicate methods, negative indices indicate constructors.
    if (_mobj && _index >= 0 && _index < _mobj->d->methods.size())
        return &(_mobj->d->methods[_index]);
    else if (_mobj && -_index >= 1 && -_index <= _mobj->d->constructors.size())
        return &(_mobj->d->constructors[(-_index) - 1]);
    else
        return 0;
}

/*!
    \fn QMetaMethodBuilder::QMetaMethodBuilder()
    \internal
*/

/*!
    Returns the index of this method within its QMetaObjectBuilder.
*/
int QMetaMethodBuilder::index() const
{
    if (_index >= 0)
        return _index;          // Method, signal, or slot
    else
        return (-_index) - 1;   // Constructor
}

/*!
    Returns the type of this method (signal, slot, method, or constructor).
*/
QMetaMethod::MethodType QMetaMethodBuilder::methodType() const
{
    QMetaMethodBuilderPrivate *d = d_func();
    if (d)
        return d->methodType();
    else
        return QMetaMethod::Method;
}

/*!
    Returns the signature of this method.

    \sa parameterNames(), returnType()
*/
QByteArray QMetaMethodBuilder::signature() const
{
    QMetaMethodBuilderPrivate *d = d_func();
    if (d)
        return d->signature;
    else
        return QByteArray();
}

/*!
    Returns the return type for this method; empty if the method's
    return type is \c{void}.

    \sa setReturnType(), signature()
*/
QByteArray QMetaMethodBuilder::returnType() const
{
    QMetaMethodBuilderPrivate *d = d_func();
    if (d)
        return d->returnType;
    else
        return QByteArray();
}

/*!
    Sets the return type for this method to \a value.  If \a value
    is empty, then the method's return type is \c{void}.  The \a value
    will be normalized before it is added to the method.

    \sa returnType(), parameterTypes(), signature()
*/
void QMetaMethodBuilder::setReturnType(const QByteArray& value)
{
    QMetaMethodBuilderPrivate *d = d_func();
    if (d)
        d->returnType = QMetaObject::normalizedType(value);
}

/*!
    Returns the list of parameter types for this method.

    \sa returnType(), parameterNames()
*/
QList<QByteArray> QMetaMethodBuilder::parameterTypes() const
{
    QMetaMethodBuilderPrivate *d = d_func();
    if (d)
        return d->parameterTypes();
    else
        return QList<QByteArray>();
}

/*!
    Returns the list of parameter names for this method.

    \sa setParameterNames()
*/
QList<QByteArray> QMetaMethodBuilder::parameterNames() const
{
    QMetaMethodBuilderPrivate *d = d_func();
    if (d)
        return d->parameterNames;
    else
        return QList<QByteArray>();
}

/*!
    Sets the list of parameter names for this method to \a value.

    \sa parameterNames()
*/
void QMetaMethodBuilder::setParameterNames(const QList<QByteArray>& value)
{
    QMetaMethodBuilderPrivate *d = d_func();
    if (d)
        d->parameterNames = value;
}

/*!
    Returns the tag associated with this method.

    \sa setTag()
*/
QByteArray QMetaMethodBuilder::tag() const
{
    QMetaMethodBuilderPrivate *d = d_func();
    if (d)
        return d->tag;
    else
        return QByteArray();
}

/*!
    Sets the tag associated with this method to \a value.

    \sa setTag()
*/
void QMetaMethodBuilder::setTag(const QByteArray& value)
{
    QMetaMethodBuilderPrivate *d = d_func();
    if (d)
        d->tag = value;
}

/*!
    Returns the access specification of this method (private, protected,
    or public).  The default value is QMetaMethod::Public for methods,
    slots, signals and constructors.

    \sa setAccess()
*/
QMetaMethod::Access QMetaMethodBuilder::access() const
{
    QMetaMethodBuilderPrivate *d = d_func();
    if (d)
        return d->access();
    else
        return QMetaMethod::Public;
}

/*!
    Sets the access specification of this method (private, protected,
    or public) to \a value.  If the method is a signal, this function
    will be ignored.

    \sa access()
*/
void QMetaMethodBuilder::setAccess(QMetaMethod::Access value)
{
    QMetaMethodBuilderPrivate *d = d_func();
    if (d && d->methodType() != QMetaMethod::Signal)
        d->setAccess(value);
}

/*!
    Returns the additional attributes for this method.

    \sa setAttributes()
*/
int QMetaMethodBuilder::attributes() const
{
    QMetaMethodBuilderPrivate *d = d_func();
    if (d)
        return (d->attributes >> 4);
    else
        return 0;
}

/*!
    Sets the additional attributes for this method to \a value.

    \sa attributes()
*/
void QMetaMethodBuilder::setAttributes(int value)
{
    QMetaMethodBuilderPrivate *d = d_func();
    if (d)
        d->attributes = ((d->attributes & 0x0f) | (value << 4));
}

/*!
    Returns the revision of this method.

    \sa setRevision()
*/
int QMetaMethodBuilder::revision() const
{
    QMetaMethodBuilderPrivate *d = d_func();
    if (d)
        return d->revision;
    return 0;

}

/*!
    Sets the \a revision of this method.

    \sa revision()
*/
void QMetaMethodBuilder::setRevision(int revision)
{
    QMetaMethodBuilderPrivate *d = d_func();
    if (d) {
        d->revision = revision;
        if (revision)
            d->attributes |= MethodRevisioned;
        else
            d->attributes &= ~MethodRevisioned;
    }
}

/*!
    \class QMetaPropertyBuilder
    \inmodule QtCore
    \internal
    \brief The QMetaPropertyBuilder class enables modifications to a property definition on a meta object builder.
*/

QMetaPropertyBuilderPrivate *QMetaPropertyBuilder::d_func() const
{
    if (_mobj && _index >= 0 && _index < _mobj->d->properties.size())
        return &(_mobj->d->properties[_index]);
    else
        return 0;
}

/*!
    \fn QMetaPropertyBuilder::QMetaPropertyBuilder()
    \internal
*/

/*!
    \fn int QMetaPropertyBuilder::index() const

    Returns the index of this property within its QMetaObjectBuilder.
*/

/*!
    Returns the name associated with this property.

    \sa type()
*/
QByteArray QMetaPropertyBuilder::name() const
{
    QMetaPropertyBuilderPrivate *d = d_func();
    if (d)
        return d->name;
    else
        return QByteArray();
}

/*!
    Returns the type associated with this property.

    \sa name()
*/
QByteArray QMetaPropertyBuilder::type() const
{
    QMetaPropertyBuilderPrivate *d = d_func();
    if (d)
        return d->type;
    else
        return QByteArray();
}

/*!
    Returns \c true if this property has a notify signal; false otherwise.

    \sa notifySignal(), setNotifySignal(), removeNotifySignal()
*/
bool QMetaPropertyBuilder::hasNotifySignal() const
{
    QMetaPropertyBuilderPrivate *d = d_func();
    if (d)
        return d->flag(Notify);
    else
        return false;
}

/*!
    Returns the notify signal associated with this property.

    \sa hasNotifySignal(), setNotifySignal(), removeNotifySignal()
*/
QMetaMethodBuilder QMetaPropertyBuilder::notifySignal() const
{
    QMetaPropertyBuilderPrivate *d = d_func();
    if (d && d->notifySignal >= 0)
        return QMetaMethodBuilder(_mobj, d->notifySignal);
    else
        return QMetaMethodBuilder();
}

/*!
    Sets the notify signal associated with this property to \a value.

    \sa hasNotifySignal(), notifySignal(), removeNotifySignal()
*/
void QMetaPropertyBuilder::setNotifySignal(const QMetaMethodBuilder& value)
{
    QMetaPropertyBuilderPrivate *d = d_func();
    if (d) {
        if (value._mobj) {
            d->notifySignal = value._index;
            d->setFlag(Notify, true);
        } else {
            d->notifySignal = -1;
            d->setFlag(Notify, false);
        }
    }
}

/*!
    Removes the notify signal from this property.

    \sa hasNotifySignal(), notifySignal(), setNotifySignal()
*/
void QMetaPropertyBuilder::removeNotifySignal()
{
    QMetaPropertyBuilderPrivate *d = d_func();
    if (d) {
        d->notifySignal = -1;
        d->setFlag(Notify, false);
    }
}

/*!
    Returns \c true if this property is readable; otherwise returns \c false.
    The default value is true.

    \sa setReadable(), isWritable()
*/
bool QMetaPropertyBuilder::isReadable() const
{
    QMetaPropertyBuilderPrivate *d = d_func();
    if (d)
        return d->flag(Readable);
    else
        return false;
}

/*!
    Returns \c true if this property is writable; otherwise returns \c false.
    The default value is true.

    \sa setWritable(), isReadable()
*/
bool QMetaPropertyBuilder::isWritable() const
{
    QMetaPropertyBuilderPrivate *d = d_func();
    if (d)
        return d->flag(Writable);
    else
        return false;
}

/*!
    Returns \c true if this property can be reset to a default value; otherwise
    returns \c false.  The default value is false.

    \sa setResettable()
*/
bool QMetaPropertyBuilder::isResettable() const
{
    QMetaPropertyBuilderPrivate *d = d_func();
    if (d)
        return d->flag(Resettable);
    else
        return false;
}

/*!
    Returns \c true if this property is designable; otherwise returns \c false.
    This default value is false.

    \sa setDesignable(), isScriptable(), isStored()
*/
bool QMetaPropertyBuilder::isDesignable() const
{
    QMetaPropertyBuilderPrivate *d = d_func();
    if (d)
        return d->flag(Designable);
    else
        return false;
}

/*!
    Returns \c true if the property is scriptable; otherwise returns \c false.
    This default value is true.

    \sa setScriptable(), isDesignable(), isStored()
*/
bool QMetaPropertyBuilder::isScriptable() const
{
    QMetaPropertyBuilderPrivate *d = d_func();
    if (d)
        return d->flag(Scriptable);
    else
        return false;
}

/*!
    Returns \c true if the property is stored; otherwise returns \c false.
    This default value is false.

    \sa setStored(), isDesignable(), isScriptable()
*/
bool QMetaPropertyBuilder::isStored() const
{
    QMetaPropertyBuilderPrivate *d = d_func();
    if (d)
        return d->flag(Stored);
    else
        return false;
}

/*!
    Returns \c true if the property is editable; otherwise returns \c false.
    This default value is false.

    \sa setEditable(), isDesignable(), isScriptable(), isStored()
*/
bool QMetaPropertyBuilder::isEditable() const
{
    QMetaPropertyBuilderPrivate *d = d_func();
    if (d)
        return d->flag(Editable);
    else
        return false;
}

/*!
    Returns \c true if this property is designated as the \c USER
    property, i.e., the one that the user can edit or that is
    significant in some other way.  Otherwise it returns
    false.  This default value is false.

    \sa setUser(), isDesignable(), isScriptable()
*/
bool QMetaPropertyBuilder::isUser() const
{
    QMetaPropertyBuilderPrivate *d = d_func();
    if (d)
        return d->flag(User);
    else
        return false;
}

/*!
    Returns \c true if the property has a C++ setter function that
    follows Qt's standard "name" / "setName" pattern. Designer and uic
    query hasStdCppSet() in order to avoid expensive
    QObject::setProperty() calls. All properties in Qt [should] follow
    this pattern.  The default value is false.

    \sa setStdCppSet()
*/
bool QMetaPropertyBuilder::hasStdCppSet() const
{
    QMetaPropertyBuilderPrivate *d = d_func();
    if (d)
        return d->flag(StdCppSet);
    else
        return false;
}

/*!
    Returns \c true if the property is an enumerator or flag type;
    otherwise returns \c false.  This default value is false.

    \sa setEnumOrFlag()
*/
bool QMetaPropertyBuilder::isEnumOrFlag() const
{
    QMetaPropertyBuilderPrivate *d = d_func();
    if (d)
        return d->flag(EnumOrFlag);
    else
        return false;
}

/*!
    Returns \c true if the property is constant; otherwise returns \c false.
    The default value is false.
*/
bool QMetaPropertyBuilder::isConstant() const
{
    QMetaPropertyBuilderPrivate *d = d_func();
    if (d)
        return d->flag(Constant);
    else
        return false;
}

/*!
    Returns \c true if the property is final; otherwise returns \c false.
    The default value is false.
*/
bool QMetaPropertyBuilder::isFinal() const
{
    QMetaPropertyBuilderPrivate *d = d_func();
    if (d)
        return d->flag(Final);
    else
        return false;
}

/*!
    Sets this property to readable if \a value is true.

    \sa isReadable(), setWritable()
*/
void QMetaPropertyBuilder::setReadable(bool value)
{
    QMetaPropertyBuilderPrivate *d = d_func();
    if (d)
        d->setFlag(Readable, value);
}

/*!
    Sets this property to writable if \a value is true.

    \sa isWritable(), setReadable()
*/
void QMetaPropertyBuilder::setWritable(bool value)
{
    QMetaPropertyBuilderPrivate *d = d_func();
    if (d)
        d->setFlag(Writable, value);
}

/*!
    Sets this property to resettable if \a value is true.

    \sa isResettable()
*/
void QMetaPropertyBuilder::setResettable(bool value)
{
    QMetaPropertyBuilderPrivate *d = d_func();
    if (d)
        d->setFlag(Resettable, value);
}

/*!
    Sets this property to designable if \a value is true.

    \sa isDesignable(), setScriptable(), setStored()
*/
void QMetaPropertyBuilder::setDesignable(bool value)
{
    QMetaPropertyBuilderPrivate *d = d_func();
    if (d)
        d->setFlag(Designable, value);
}

/*!
    Sets this property to scriptable if \a value is true.

    \sa isScriptable(), setDesignable(), setStored()
*/
void QMetaPropertyBuilder::setScriptable(bool value)
{
    QMetaPropertyBuilderPrivate *d = d_func();
    if (d)
        d->setFlag(Scriptable, value);
}

/*!
    Sets this property to storable if \a value is true.

    \sa isStored(), setDesignable(), setScriptable()
*/
void QMetaPropertyBuilder::setStored(bool value)
{
    QMetaPropertyBuilderPrivate *d = d_func();
    if (d)
        d->setFlag(Stored, value);
}

/*!
    Sets this property to editable if \a value is true.

    \sa isEditable(), setDesignable(), setScriptable(), setStored()
*/
void QMetaPropertyBuilder::setEditable(bool value)
{
    QMetaPropertyBuilderPrivate *d = d_func();
    if (d)
        d->setFlag(Editable, value);
}

/*!
    Sets the \c USER flag on this property to \a value.

    \sa isUser(), setDesignable(), setScriptable()
*/
void QMetaPropertyBuilder::setUser(bool value)
{
    QMetaPropertyBuilderPrivate *d = d_func();
    if (d)
        d->setFlag(User, value);
}

/*!
    Sets the C++ setter flag on this property to \a value, which is
    true if the property has a C++ setter function that follows Qt's
    standard "name" / "setName" pattern.

    \sa hasStdCppSet()
*/
void QMetaPropertyBuilder::setStdCppSet(bool value)
{
    QMetaPropertyBuilderPrivate *d = d_func();
    if (d)
        d->setFlag(StdCppSet, value);
}

/*!
    Sets this property to be of an enumerator or flag type if
    \a value is true.

    \sa isEnumOrFlag()
*/
void QMetaPropertyBuilder::setEnumOrFlag(bool value)
{
    QMetaPropertyBuilderPrivate *d = d_func();
    if (d)
        d->setFlag(EnumOrFlag, value);
}

/*!
    Sets the \c CONSTANT flag on this property to \a value.

    \sa isConstant()
*/
void QMetaPropertyBuilder::setConstant(bool value)
{
    QMetaPropertyBuilderPrivate *d = d_func();
    if (d)
        d->setFlag(Constant, value);
}

/*!
    Sets the \c FINAL flag on this property to \a value.

    \sa isFinal()
*/
void QMetaPropertyBuilder::setFinal(bool value)
{
    QMetaPropertyBuilderPrivate *d = d_func();
    if (d)
        d->setFlag(Final, value);
}

/*!
    Returns the revision of this property.

    \sa setRevision()
*/
int QMetaPropertyBuilder::revision() const
{
    QMetaPropertyBuilderPrivate *d = d_func();
    if (d)
        return d->revision;
    return 0;

}

/*!
    Sets the \a revision of this property.

    \sa revision()
*/
void QMetaPropertyBuilder::setRevision(int revision)
{
    QMetaPropertyBuilderPrivate *d = d_func();
    if (d) {
        d->revision = revision;
        d->setFlag(Revisioned, revision != 0);
    }
}


/*!
    \class QMetaEnumBuilder
    \inmodule QtCore
    \internal
    \brief The QMetaEnumBuilder class enables modifications to an enumerator definition on a meta object builder.
*/

QMetaEnumBuilderPrivate *QMetaEnumBuilder::d_func() const
{
    if (_mobj && _index >= 0 && _index < _mobj->d->enumerators.size())
        return &(_mobj->d->enumerators[_index]);
    else
        return 0;
}

/*!
    \fn QMetaEnumBuilder::QMetaEnumBuilder()
    \internal
*/

/*!
    \fn int QMetaEnumBuilder::index() const

    Returns the index of this enumerator within its QMetaObjectBuilder.
*/

/*!
    Returns the name of the enumerator (without the scope).
*/
QByteArray QMetaEnumBuilder::name() const
{
    QMetaEnumBuilderPrivate *d = d_func();
    if (d)
        return d->name;
    else
        return QByteArray();
}

/*!
    Returns \c true if this enumerator is used as a flag; otherwise returns
    false.

    \sa setIsFlag()
*/
bool QMetaEnumBuilder::isFlag() const
{
    QMetaEnumBuilderPrivate *d = d_func();
    if (d)
        return d->isFlag;
    else
        return false;
}

/*!
    Sets this enumerator to be used as a flag if \a value is true.

    \sa isFlag()
*/
void QMetaEnumBuilder::setIsFlag(bool value)
{
    QMetaEnumBuilderPrivate *d = d_func();
    if (d)
        d->isFlag = value;
}

/*!
    Returns the number of keys.

    \sa key(), addKey()
*/
int QMetaEnumBuilder::keyCount() const
{
    QMetaEnumBuilderPrivate *d = d_func();
    if (d)
        return d->keys.size();
    else
        return 0;
}

/*!
    Returns the key with the given \a index, or an empty QByteArray
    if no such key exists.

    \sa keyCount(), addKey(), value()
*/
QByteArray QMetaEnumBuilder::key(int index) const
{
    QMetaEnumBuilderPrivate *d = d_func();
    if (d && index >= 0 && index < d->keys.size())
        return d->keys[index];
    else
        return QByteArray();
}

/*!
    Returns the value with the given \a index; or returns -1 if there
    is no such value.

    \sa keyCount(), addKey(), key()
*/
int QMetaEnumBuilder::value(int index) const
{
    QMetaEnumBuilderPrivate *d = d_func();
    if (d && index >= 0 && index < d->keys.size())
        return d->values[index];
    else
        return -1;
}

/*!
    Adds a new key called \a name to this enumerator, associated
    with \a value.  Returns the index of the new key.

    \sa keyCount(), key(), value(), removeKey()
*/
int QMetaEnumBuilder::addKey(const QByteArray& name, int value)
{
    QMetaEnumBuilderPrivate *d = d_func();
    if (d) {
        int index = d->keys.size();
        d->keys += name;
        d->values += value;
        return index;
    } else {
        return -1;
    }
}

/*!
    Removes the key at \a index from this enumerator.

    \sa addKey()
*/
void QMetaEnumBuilder::removeKey(int index)
{
    QMetaEnumBuilderPrivate *d = d_func();
    if (d && index >= 0 && index < d->keys.size()) {
        d->keys.removeAt(index);
        d->values.removeAt(index);
    }
}

QT_END_NAMESPACE
