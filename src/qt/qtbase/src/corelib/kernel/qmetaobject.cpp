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

#include "qmetaobject.h"
#include "qmetatype.h"
#include "qobject.h"
#include "qmetaobject_p.h"

#include <qcoreapplication.h>
#include <qcoreevent.h>
#include <qdatastream.h>
#include <qstringlist.h>
#include <qthread.h>
#include <qvariant.h>
#include <qhash.h>
#include <qdebug.h>
#include <qsemaphore.h>

#include "private/qobject_p.h"
#include "private/qmetaobject_p.h"

// for normalizeTypeInternal
#include "private/qmetaobject_moc_p.h"

#include <ctype.h>

QT_BEGIN_NAMESPACE

/*!
    \class QMetaObject
    \inmodule QtCore

    \brief The QMetaObject class contains meta-information about Qt
    objects.

    \ingroup objectmodel

    The Qt \l{Meta-Object System} in Qt is responsible for the
    signals and slots inter-object communication mechanism, runtime
    type information, and the Qt property system. A single
    QMetaObject instance is created for each QObject subclass that is
    used in an application, and this instance stores all the
    meta-information for the QObject subclass. This object is
    available as QObject::metaObject().

    This class is not normally required for application programming,
    but it is useful if you write meta-applications, such as scripting
    engines or GUI builders.

    The functions you are most likely to find useful are these:
    \list
    \li className() returns the name of a class.
    \li superClass() returns the superclass's meta-object.
    \li method() and methodCount() provide information
       about a class's meta-methods (signals, slots and other
       \l{Q_INVOKABLE}{invokable} member functions).
    \li enumerator() and enumeratorCount() and provide information about
       a class's enumerators.
    \li propertyCount() and property() provide information about a
       class's properties.
    \li constructor() and constructorCount() provide information
       about a class's meta-constructors.
    \endlist

    The index functions indexOfConstructor(), indexOfMethod(),
    indexOfEnumerator(), and indexOfProperty() map names of constructors,
    member functions, enumerators, or properties to indexes in the
    meta-object. For example, Qt uses indexOfMethod() internally when you
    connect a signal to a slot.

    Classes can also have a list of \e{name}--\e{value} pairs of
    additional class information, stored in QMetaClassInfo objects.
    The number of pairs is returned by classInfoCount(), single pairs
    are returned by classInfo(), and you can search for pairs with
    indexOfClassInfo().

    \sa QMetaClassInfo, QMetaEnum, QMetaMethod, QMetaProperty, QMetaType,
        {Meta-Object System}
*/

/*!
    \enum QMetaObject::Call

    \internal

    \value InvokeSlot
    \value EmitSignal
    \value ReadProperty
    \value WriteProperty
    \value ResetProperty
    \value QueryPropertyDesignable
    \value QueryPropertyScriptable
    \value QueryPropertyStored
    \value QueryPropertyEditable
    \value QueryPropertyUser
    \value CreateInstance
*/

/*!
    \enum QMetaMethod::Access

    This enum describes the access level of a method, following the conventions used in C++.

    \value Private
    \value Protected
    \value Public
*/

static inline const QMetaObjectPrivate *priv(const uint* data)
{ return reinterpret_cast<const QMetaObjectPrivate*>(data); }

static inline const QByteArray stringData(const QMetaObject *mo, int index)
{
    Q_ASSERT(priv(mo->d.data)->revision >= 7);
    const QByteArrayDataPtr data = { const_cast<QByteArrayData*>(&mo->d.stringdata[index]) };
    Q_ASSERT(data.ptr->ref.isStatic());
    Q_ASSERT(data.ptr->alloc == 0);
    Q_ASSERT(data.ptr->capacityReserved == 0);
    Q_ASSERT(data.ptr->size >= 0);
    return data;
}

static inline const char *rawStringData(const QMetaObject *mo, int index)
{
    return stringData(mo, index).data();
}

static inline int stringSize(const QMetaObject *mo, int index)
{
    return stringData(mo, index).size();
}

static inline QByteArray typeNameFromTypeInfo(const QMetaObject *mo, uint typeInfo)
{
    if (typeInfo & IsUnresolvedType) {
        return stringData(mo, typeInfo & TypeNameIndexMask);
    } else {
        // ### Use the QMetaType::typeName() that returns QByteArray
        const char *t = QMetaType::typeName(typeInfo);
        return QByteArray::fromRawData(t, qstrlen(t));
    }
}

static inline const char *rawTypeNameFromTypeInfo(const QMetaObject *mo, uint typeInfo)
{
    return typeNameFromTypeInfo(mo, typeInfo).constData();
}

static inline int typeFromTypeInfo(const QMetaObject *mo, uint typeInfo)
{
    if (!(typeInfo & IsUnresolvedType))
        return typeInfo;
    return QMetaType::type(stringData(mo, typeInfo & TypeNameIndexMask));
}

class QMetaMethodPrivate : public QMetaMethod
{
public:
    static const QMetaMethodPrivate *get(const QMetaMethod *q)
    { return static_cast<const QMetaMethodPrivate *>(q); }

    inline QByteArray signature() const;
    inline QByteArray name() const;
    inline int typesDataIndex() const;
    inline const char *rawReturnTypeName() const;
    inline int returnType() const;
    inline int parameterCount() const;
    inline int parametersDataIndex() const;
    inline uint parameterTypeInfo(int index) const;
    inline int parameterType(int index) const;
    inline void getParameterTypes(int *types) const;
    inline QList<QByteArray> parameterTypes() const;
    inline QList<QByteArray> parameterNames() const;
    inline QByteArray tag() const;
    inline int ownMethodIndex() const;

private:
    QMetaMethodPrivate();
};

/*!
    \since 4.5

    Constructs a new instance of this class. You can pass up to ten arguments
    (\a val0, \a val1, \a val2, \a val3, \a val4, \a val5, \a val6, \a val7,
    \a val8, and \a val9) to the constructor. Returns the new object, or 0 if
    no suitable constructor is available.

    Note that only constructors that are declared with the Q_INVOKABLE
    modifier are made available through the meta-object system.

    \sa Q_ARG(), constructor()
*/
QObject *QMetaObject::newInstance(QGenericArgument val0,
                                  QGenericArgument val1,
                                  QGenericArgument val2,
                                  QGenericArgument val3,
                                  QGenericArgument val4,
                                  QGenericArgument val5,
                                  QGenericArgument val6,
                                  QGenericArgument val7,
                                  QGenericArgument val8,
                                  QGenericArgument val9) const
{
    QByteArray constructorName = className();
    {
        int idx = constructorName.lastIndexOf(':');
        if (idx != -1)
            constructorName.remove(0, idx+1); // remove qualified part
    }
    QVarLengthArray<char, 512> sig;
    sig.append(constructorName.constData(), constructorName.length());
    sig.append('(');

    enum { MaximumParamCount = 10 };
    const char *typeNames[] = {val0.name(), val1.name(), val2.name(), val3.name(), val4.name(),
                               val5.name(), val6.name(), val7.name(), val8.name(), val9.name()};

    int paramCount;
    for (paramCount = 0; paramCount < MaximumParamCount; ++paramCount) {
        int len = qstrlen(typeNames[paramCount]);
        if (len <= 0)
            break;
        sig.append(typeNames[paramCount], len);
        sig.append(',');
    }
    if (paramCount == 0)
        sig.append(')'); // no parameters
    else
        sig[sig.size() - 1] = ')';
    sig.append('\0');

    int idx = indexOfConstructor(sig.constData());
    if (idx < 0) {
        QByteArray norm = QMetaObject::normalizedSignature(sig.constData());
        idx = indexOfConstructor(norm.constData());
    }
    if (idx < 0)
        return 0;

    QObject *returnValue = 0;
    void *param[] = {&returnValue, val0.data(), val1.data(), val2.data(), val3.data(), val4.data(),
                     val5.data(), val6.data(), val7.data(), val8.data(), val9.data()};

    if (static_metacall(CreateInstance, idx, param) >= 0)
        return 0;
    return returnValue;
}

/*!
    \internal
*/
int QMetaObject::static_metacall(Call cl, int idx, void **argv) const
{
    Q_ASSERT(priv(d.data)->revision >= 6);
    if (!d.static_metacall)
        return 0;
    d.static_metacall(0, cl, idx, argv);
    return -1;
}

/*!
    \internal
*/
int QMetaObject::metacall(QObject *object, Call cl, int idx, void **argv)
{
    if (object->d_ptr->metaObject)
        return object->d_ptr->metaObject->metaCall(object, cl, idx, argv);
    else
        return object->qt_metacall(cl, idx, argv);
}

/*!
    Returns the class name.

    \sa superClass()
*/
const char *QMetaObject::className() const
{
    return rawStringData(this, 0);
}

/*!
    \fn QMetaObject *QMetaObject::superClass() const

    Returns the meta-object of the superclass, or 0 if there is no
    such object.

    \sa className()
*/

/*!
    \internal

    Returns \a obj if object \a obj inherits from this
    meta-object; otherwise returns 0.
*/
QObject *QMetaObject::cast(QObject *obj) const
{
    if (obj) {
        const QMetaObject *m = obj->metaObject();
        do {
            if (m == this)
                return obj;
        } while ((m = m->d.superdata));
    }
    return 0;
}

/*!
    \internal

    Returns \a obj if object \a obj inherits from this
    meta-object; otherwise returns 0.
*/
const QObject *QMetaObject::cast(const QObject *obj) const
{
    if (obj) {
        const QMetaObject *m = obj->metaObject();
        do {
            if (m == this)
                return obj;
        } while ((m = m->d.superdata));
    }
    return 0;
}

#ifndef QT_NO_TRANSLATION
/*!
    \internal
*/
QString QMetaObject::tr(const char *s, const char *c, int n) const
{
    return QCoreApplication::translate(rawStringData(this, 0), s, c, n);
}
#endif // QT_NO_TRANSLATION

/*!
    Returns the method offset for this class; i.e. the index position
    of this class's first member function.

    The offset is the sum of all the methods in the class's
    superclasses (which is always positive since QObject has the
    deleteLater() slot and a destroyed() signal).

    \sa method(), methodCount(), indexOfMethod()
*/
int QMetaObject::methodOffset() const
{
    int offset = 0;
    const QMetaObject *m = d.superdata;
    while (m) {
        offset += priv(m->d.data)->methodCount;
        m = m->d.superdata;
    }
    return offset;
}


/*!
    Returns the enumerator offset for this class; i.e. the index
    position of this class's first enumerator.

    If the class has no superclasses with enumerators, the offset is
    0; otherwise the offset is the sum of all the enumerators in the
    class's superclasses.

    \sa enumerator(), enumeratorCount(), indexOfEnumerator()
*/
int QMetaObject::enumeratorOffset() const
{
    int offset = 0;
    const QMetaObject *m = d.superdata;
    while (m) {
        offset += priv(m->d.data)->enumeratorCount;
        m = m->d.superdata;
    }
    return offset;
}

/*!
    Returns the property offset for this class; i.e. the index
    position of this class's first property.

    The offset is the sum of all the properties in the class's
    superclasses (which is always positive since QObject has the
    name() property).

    \sa property(), propertyCount(), indexOfProperty()
*/
int QMetaObject::propertyOffset() const
{
    int offset = 0;
    const QMetaObject *m = d.superdata;
    while (m) {
        offset += priv(m->d.data)->propertyCount;
        m = m->d.superdata;
    }
    return offset;
}

/*!
    Returns the class information offset for this class; i.e. the
    index position of this class's first class information item.

    If the class has no superclasses with class information, the
    offset is 0; otherwise the offset is the sum of all the class
    information items in the class's superclasses.

    \sa classInfo(), classInfoCount(), indexOfClassInfo()
*/
int QMetaObject::classInfoOffset() const
{
    int offset = 0;
    const QMetaObject *m = d.superdata;
    while (m) {
        offset += priv(m->d.data)->classInfoCount;
        m = m->d.superdata;
    }
    return offset;
}

/*!
    \since 4.5

    Returns the number of constructors in this class.

    \sa constructor(), indexOfConstructor()
*/
int QMetaObject::constructorCount() const
{
    Q_ASSERT(priv(d.data)->revision >= 2);
    return priv(d.data)->constructorCount;
}

/*!
    Returns the number of methods in this class, including the number of
    methods provided by each base class. These include signals and slots
    as well as normal member functions.

    Use code like the following to obtain a QStringList containing the methods
    specific to a given class:

    \snippet code/src_corelib_kernel_qmetaobject.cpp methodCount

    \sa method(), methodOffset(), indexOfMethod()
*/
int QMetaObject::methodCount() const
{
    int n = priv(d.data)->methodCount;
    const QMetaObject *m = d.superdata;
    while (m) {
        n += priv(m->d.data)->methodCount;
        m = m->d.superdata;
    }
    return n;
}

/*!
    Returns the number of enumerators in this class.

    \sa enumerator(), enumeratorOffset(), indexOfEnumerator()
*/
int QMetaObject::enumeratorCount() const
{
    int n = priv(d.data)->enumeratorCount;
    const QMetaObject *m = d.superdata;
    while (m) {
        n += priv(m->d.data)->enumeratorCount;
        m = m->d.superdata;
    }
    return n;
}

/*!
    Returns the number of properties in this class, including the number of
    properties provided by each base class.

    Use code like the following to obtain a QStringList containing the properties
    specific to a given class:

    \snippet code/src_corelib_kernel_qmetaobject.cpp propertyCount

    \sa property(), propertyOffset(), indexOfProperty()
*/
int QMetaObject::propertyCount() const
{
    int n = priv(d.data)->propertyCount;
    const QMetaObject *m = d.superdata;
    while (m) {
        n += priv(m->d.data)->propertyCount;
        m = m->d.superdata;
    }
    return n;
}

/*!
    Returns the number of items of class information in this class.

    \sa classInfo(), classInfoOffset(), indexOfClassInfo()
*/
int QMetaObject::classInfoCount() const
{
    int n = priv(d.data)->classInfoCount;
    const QMetaObject *m = d.superdata;
    while (m) {
        n += priv(m->d.data)->classInfoCount;
        m = m->d.superdata;
    }
    return n;
}

// Returns \c true if the method defined by the given meta-object&handle
// matches the given name, argument count and argument types, otherwise
// returns \c false.
static bool methodMatch(const QMetaObject *m, int handle,
                        const QByteArray &name, int argc,
                        const QArgumentType *types)
{
    Q_ASSERT(priv(m->d.data)->revision >= 7);
    if (int(m->d.data[handle + 1]) != argc)
        return false;

    if (stringData(m, m->d.data[handle]) != name)
        return false;

    int paramsIndex = m->d.data[handle + 2] + 1;
    for (int i = 0; i < argc; ++i) {
        uint typeInfo = m->d.data[paramsIndex + i];
        if (types[i].type()) {
            if (types[i].type() != typeFromTypeInfo(m, typeInfo))
                return false;
        } else {
            if (types[i].name() != typeNameFromTypeInfo(m, typeInfo))
                return false;
        }
    }

    return true;
}

/**
* \internal
* helper function for indexOf{Method,Slot,Signal}, returns the relative index of the method within
* the baseObject
* \a MethodType might be MethodSignal or MethodSlot, or 0 to match everything.
*/
template<int MethodType>
static inline int indexOfMethodRelative(const QMetaObject **baseObject,
                                        const QByteArray &name, int argc,
                                        const QArgumentType *types)
{
    for (const QMetaObject *m = *baseObject; m; m = m->d.superdata) {
        Q_ASSERT(priv(m->d.data)->revision >= 7);
        int i = (MethodType == MethodSignal)
                 ? (priv(m->d.data)->signalCount - 1) : (priv(m->d.data)->methodCount - 1);
        const int end = (MethodType == MethodSlot)
                        ? (priv(m->d.data)->signalCount) : 0;

        for (; i >= end; --i) {
            int handle = priv(m->d.data)->methodData + 5*i;
            if (methodMatch(m, handle, name, argc, types)) {
                *baseObject = m;
                return i;
            }
        }
    }
    return -1;
}


/*!
    \since 4.5

    Finds \a constructor and returns its index; otherwise returns -1.

    Note that the \a constructor has to be in normalized form, as returned
    by normalizedSignature().

    \sa constructor(), constructorCount(), normalizedSignature()
*/
int QMetaObject::indexOfConstructor(const char *constructor) const
{
    Q_ASSERT(priv(d.data)->revision >= 7);
    QArgumentTypeArray types;
    QByteArray name = QMetaObjectPrivate::decodeMethodSignature(constructor, types);
    return QMetaObjectPrivate::indexOfConstructor(this, name, types.size(), types.constData());
}

/*!
    Finds \a method and returns its index; otherwise returns -1.

    Note that the \a method has to be in normalized form, as returned
    by normalizedSignature().

    \sa method(), methodCount(), methodOffset(), normalizedSignature()
*/
int QMetaObject::indexOfMethod(const char *method) const
{
    const QMetaObject *m = this;
    int i;
    Q_ASSERT(priv(m->d.data)->revision >= 7);
    QArgumentTypeArray types;
    QByteArray name = QMetaObjectPrivate::decodeMethodSignature(method, types);
    i = indexOfMethodRelative<0>(&m, name, types.size(), types.constData());
    if (i >= 0)
        i += m->methodOffset();
    return i;
}

// Parses a string of comma-separated types into QArgumentTypes.
// No normalization of the type names is performed.
static void argumentTypesFromString(const char *str, const char *end,
                                    QArgumentTypeArray &types)
{
    Q_ASSERT(str <= end);
    while (str != end) {
        if (!types.isEmpty())
            ++str; // Skip comma
        const char *begin = str;
        int level = 0;
        while (str != end && (level > 0 || *str != ',')) {
            if (*str == '<')
                ++level;
            else if (*str == '>')
                --level;
            ++str;
        }
        types += QArgumentType(QByteArray(begin, str - begin));
    }
}

// Given a method \a signature (e.g. "foo(int,double)"), this function
// populates the argument \a types array and returns the method name.
QByteArray QMetaObjectPrivate::decodeMethodSignature(
        const char *signature, QArgumentTypeArray &types)
{
    Q_ASSERT(signature != 0);
    const char *lparens = strchr(signature, '(');
    if (!lparens)
        return QByteArray();
    const char *rparens = strrchr(lparens + 1, ')');
    if (!rparens || *(rparens+1))
        return QByteArray();
    int nameLength = lparens - signature;
    argumentTypesFromString(lparens + 1, rparens, types);
    return QByteArray::fromRawData(signature, nameLength);
}

/*!
    Finds \a signal and returns its index; otherwise returns -1.

    This is the same as indexOfMethod(), except that it will return
    -1 if the method exists but isn't a signal.

    Note that the \a signal has to be in normalized form, as returned
    by normalizedSignature().

    \sa indexOfMethod(), normalizedSignature(), method(), methodCount(), methodOffset()
*/
int QMetaObject::indexOfSignal(const char *signal) const
{
    const QMetaObject *m = this;
    int i;
    Q_ASSERT(priv(m->d.data)->revision >= 7);
    QArgumentTypeArray types;
    QByteArray name = QMetaObjectPrivate::decodeMethodSignature(signal, types);
    i = QMetaObjectPrivate::indexOfSignalRelative(&m, name, types.size(), types.constData());
    if (i >= 0)
        i += m->methodOffset();
    return i;
}

/*!
    \internal
    Same as QMetaObject::indexOfSignal, but the result is the local offset to the base object.

    \a baseObject will be adjusted to the enclosing QMetaObject, or 0 if the signal is not found
*/
int QMetaObjectPrivate::indexOfSignalRelative(const QMetaObject **baseObject,
                                              const QByteArray &name, int argc,
                                              const QArgumentType *types)
{
    int i = indexOfMethodRelative<MethodSignal>(baseObject, name, argc, types);
#ifndef QT_NO_DEBUG
    const QMetaObject *m = *baseObject;
    if (i >= 0 && m && m->d.superdata) {
        int conflict = indexOfMethod(m->d.superdata, name, argc, types);
        if (conflict >= 0) {
            QMetaMethod conflictMethod = m->d.superdata->method(conflict);
            qWarning("QMetaObject::indexOfSignal: signal %s from %s redefined in %s",
                     conflictMethod.methodSignature().constData(),
                     rawStringData(m->d.superdata, 0), rawStringData(m, 0));
        }
     }
 #endif
     return i;
}

/*!
    Finds \a slot and returns its index; otherwise returns -1.

    This is the same as indexOfMethod(), except that it will return
    -1 if the method exists but isn't a slot.

    \sa indexOfMethod(), method(), methodCount(), methodOffset()
*/
int QMetaObject::indexOfSlot(const char *slot) const
{
    const QMetaObject *m = this;
    int i;
    Q_ASSERT(priv(m->d.data)->revision >= 7);
    QArgumentTypeArray types;
    QByteArray name = QMetaObjectPrivate::decodeMethodSignature(slot, types);
    i = QMetaObjectPrivate::indexOfSlotRelative(&m, name, types.size(), types.constData());
    if (i >= 0)
        i += m->methodOffset();
    return i;
}

// same as indexOfSignalRelative but for slots.
int QMetaObjectPrivate::indexOfSlotRelative(const QMetaObject **m,
                                            const QByteArray &name, int argc,
                                            const QArgumentType *types)
{
    return indexOfMethodRelative<MethodSlot>(m, name, argc, types);
}

int QMetaObjectPrivate::indexOfSignal(const QMetaObject *m, const QByteArray &name,
                                      int argc, const QArgumentType *types)
{
    int i = indexOfSignalRelative(&m, name, argc, types);
    if (i >= 0)
        i += m->methodOffset();
    return i;
}

int QMetaObjectPrivate::indexOfSlot(const QMetaObject *m, const QByteArray &name,
                                    int argc, const QArgumentType *types)
{
    int i = indexOfSlotRelative(&m, name, argc, types);
    if (i >= 0)
        i += m->methodOffset();
    return i;
}

int QMetaObjectPrivate::indexOfMethod(const QMetaObject *m, const QByteArray &name,
                                      int argc, const QArgumentType *types)
{
    int i = indexOfMethodRelative<0>(&m, name, argc, types);
    if (i >= 0)
        i += m->methodOffset();
    return i;
}

int QMetaObjectPrivate::indexOfConstructor(const QMetaObject *m, const QByteArray &name,
                                           int argc, const QArgumentType *types)
{
    for (int i = priv(m->d.data)->constructorCount-1; i >= 0; --i) {
        int handle = priv(m->d.data)->constructorData + 5*i;
        if (methodMatch(m, handle, name, argc, types))
            return i;
    }
    return -1;
}

/*!
    \internal
    \since 5.0

    Returns the signal offset for the class \a m; i.e., the index position
    of the class's first signal.

    Similar to QMetaObject::methodOffset(), but non-signal methods are
    excluded.
*/
int QMetaObjectPrivate::signalOffset(const QMetaObject *m)
{
    Q_ASSERT(m != 0);
    int offset = 0;
    for (m = m->d.superdata; m; m = m->d.superdata)
        offset += priv(m->d.data)->signalCount;
    return offset;
}

/*!
    \internal
    \since 5.0

    Returns the number of signals for the class \a m, including the signals
    for the base class.

    Similar to QMetaObject::methodCount(), but non-signal methods are
    excluded.
*/
int QMetaObjectPrivate::absoluteSignalCount(const QMetaObject *m)
{
    Q_ASSERT(m != 0);
    int n = priv(m->d.data)->signalCount;
    for (m = m->d.superdata; m; m = m->d.superdata)
        n += priv(m->d.data)->signalCount;
    return n;
}

/*!
    \internal
    \since 5.0

    Returns the index of the signal method \a m.

    Similar to QMetaMethod::methodIndex(), but non-signal methods are
    excluded.
*/
int QMetaObjectPrivate::signalIndex(const QMetaMethod &m)
{
    if (!m.mobj)
        return -1;
    return QMetaMethodPrivate::get(&m)->ownMethodIndex() + signalOffset(m.mobj);
}

/*!
    \internal
    \since 5.0

    Returns the signal for the given meta-object \a m at \a signal_index.

    It it different from QMetaObject::method(); the index should not include
    non-signal methods.
*/
QMetaMethod QMetaObjectPrivate::signal(const QMetaObject *m, int signal_index)
{
    QMetaMethod result;
    if (signal_index < 0)
        return result;
    Q_ASSERT(m != 0);
    int i = signal_index;
    i -= signalOffset(m);
    if (i < 0 && m->d.superdata)
        return signal(m->d.superdata, signal_index);

    if (i >= 0 && i < priv(m->d.data)->signalCount) {
        result.mobj = m;
        result.handle = priv(m->d.data)->methodData + 5*i;
    }
    return result;
}

/*!
    \internal

    Returns \c true if the \a signalTypes and \a methodTypes are
    compatible; otherwise returns \c false.
*/
bool QMetaObjectPrivate::checkConnectArgs(int signalArgc, const QArgumentType *signalTypes,
                                          int methodArgc, const QArgumentType *methodTypes)
{
    if (signalArgc < methodArgc)
        return false;
    for (int i = 0; i < methodArgc; ++i) {
        if (signalTypes[i] != methodTypes[i])
            return false;
    }
    return true;
}

/*!
    \internal

    Returns \c true if the \a signal and \a method arguments are
    compatible; otherwise returns \c false.
*/
bool QMetaObjectPrivate::checkConnectArgs(const QMetaMethodPrivate *signal,
                                          const QMetaMethodPrivate *method)
{
    if (signal->methodType() != QMetaMethod::Signal)
        return false;
    if (signal->parameterCount() < method->parameterCount())
        return false;
    const QMetaObject *smeta = signal->enclosingMetaObject();
    const QMetaObject *rmeta = method->enclosingMetaObject();
    for (int i = 0; i < method->parameterCount(); ++i) {
        uint sourceTypeInfo = signal->parameterTypeInfo(i);
        uint targetTypeInfo = method->parameterTypeInfo(i);
        if ((sourceTypeInfo & IsUnresolvedType)
            || (targetTypeInfo & IsUnresolvedType)) {
            QByteArray sourceName = typeNameFromTypeInfo(smeta, sourceTypeInfo);
            QByteArray targetName = typeNameFromTypeInfo(rmeta, targetTypeInfo);
            if (sourceName != targetName)
                return false;
        } else {
            int sourceType = typeFromTypeInfo(smeta, sourceTypeInfo);
            int targetType = typeFromTypeInfo(rmeta, targetTypeInfo);
            if (sourceType != targetType)
                return false;
        }
    }
    return true;
}

static const QMetaObject *QMetaObject_findMetaObject(const QMetaObject *self, const char *name)
{
    while (self) {
        if (strcmp(rawStringData(self, 0), name) == 0)
            return self;
        if (self->d.relatedMetaObjects) {
            Q_ASSERT(priv(self->d.data)->revision >= 2);
            const QMetaObject * const *e = self->d.relatedMetaObjects;
            if (e) {
                while (*e) {
                    if (const QMetaObject *m =QMetaObject_findMetaObject((*e), name))
                        return m;
                    ++e;
                }
            }
        }
        self = self->d.superdata;
    }
    return self;
}

/*!
    Finds enumerator \a name and returns its index; otherwise returns
    -1.

    \sa enumerator(), enumeratorCount(), enumeratorOffset()
*/
int QMetaObject::indexOfEnumerator(const char *name) const
{
    const QMetaObject *m = this;
    while (m) {
        const QMetaObjectPrivate *d = priv(m->d.data);
        for (int i = d->enumeratorCount - 1; i >= 0; --i) {
            const char *prop = rawStringData(m, m->d.data[d->enumeratorData + 4*i]);
            if (name[0] == prop[0] && strcmp(name + 1, prop + 1) == 0) {
                i += m->enumeratorOffset();
                return i;
            }
        }
        m = m->d.superdata;
    }
    return -1;
}

/*!
    Finds property \a name and returns its index; otherwise returns
    -1.

    \sa property(), propertyCount(), propertyOffset()
*/
int QMetaObject::indexOfProperty(const char *name) const
{
    const QMetaObject *m = this;
    while (m) {
        const QMetaObjectPrivate *d = priv(m->d.data);
        for (int i = d->propertyCount-1; i >= 0; --i) {
            const char *prop = rawStringData(m, m->d.data[d->propertyData + 3*i]);
            if (name[0] == prop[0] && strcmp(name + 1, prop + 1) == 0) {
                i += m->propertyOffset();
                return i;
            }
        }
        m = m->d.superdata;
    }

    Q_ASSERT(priv(this->d.data)->revision >= 3);
    if (priv(this->d.data)->flags & DynamicMetaObject) {
        QAbstractDynamicMetaObject *me =
            const_cast<QAbstractDynamicMetaObject *>(static_cast<const QAbstractDynamicMetaObject *>(this));

        return me->createProperty(name, 0);
    }

    return -1;
}

/*!
    Finds class information item \a name and returns its index;
    otherwise returns -1.

    \sa classInfo(), classInfoCount(), classInfoOffset()
*/
int QMetaObject::indexOfClassInfo(const char *name) const
{
    int i = -1;
    const QMetaObject *m = this;
    while (m && i < 0) {
        for (i = priv(m->d.data)->classInfoCount-1; i >= 0; --i)
            if (strcmp(name, rawStringData(m, m->d.data[priv(m->d.data)->classInfoData + 2*i])) == 0) {
                i += m->classInfoOffset();
                break;
            }
        m = m->d.superdata;
    }
    return i;
}

/*!
    \since 4.5

    Returns the meta-data for the constructor with the given \a index.

    \sa constructorCount(), newInstance()
*/
QMetaMethod QMetaObject::constructor(int index) const
{
    int i = index;
    QMetaMethod result;
    Q_ASSERT(priv(d.data)->revision >= 2);
    if (i >= 0 && i < priv(d.data)->constructorCount) {
        result.mobj = this;
        result.handle = priv(d.data)->constructorData + 5*i;
    }
    return result;
}

/*!
    Returns the meta-data for the method with the given \a index.

    \sa methodCount(), methodOffset(), indexOfMethod()
*/
QMetaMethod QMetaObject::method(int index) const
{
    int i = index;
    i -= methodOffset();
    if (i < 0 && d.superdata)
        return d.superdata->method(index);

    QMetaMethod result;
    if (i >= 0 && i < priv(d.data)->methodCount) {
        result.mobj = this;
        result.handle = priv(d.data)->methodData + 5*i;
    }
    return result;
}

/*!
    Returns the meta-data for the enumerator with the given \a index.

    \sa enumeratorCount(), enumeratorOffset(), indexOfEnumerator()
*/
QMetaEnum QMetaObject::enumerator(int index) const
{
    int i = index;
    i -= enumeratorOffset();
    if (i < 0 && d.superdata)
        return d.superdata->enumerator(index);

    QMetaEnum result;
    if (i >= 0 && i < priv(d.data)->enumeratorCount) {
        result.mobj = this;
        result.handle = priv(d.data)->enumeratorData + 4*i;
    }
    return result;
}

/*!
    Returns the meta-data for the property with the given \a index.
    If no such property exists, a null QMetaProperty is returned.

    \sa propertyCount(), propertyOffset(), indexOfProperty()
*/
QMetaProperty QMetaObject::property(int index) const
{
    int i = index;
    i -= propertyOffset();
    if (i < 0 && d.superdata)
        return d.superdata->property(index);

    QMetaProperty result;
    if (i >= 0 && i < priv(d.data)->propertyCount) {
        int handle = priv(d.data)->propertyData + 3*i;
        int flags = d.data[handle + 2];
        result.mobj = this;
        result.handle = handle;
        result.idx = i;

        if (flags & EnumOrFlag) {
            const char *type = rawTypeNameFromTypeInfo(this, d.data[handle + 1]);
            result.menum = enumerator(indexOfEnumerator(type));
            if (!result.menum.isValid()) {
                const char *enum_name = type;
                const char *scope_name = rawStringData(this, 0);
                char *scope_buffer = 0;

                const char *colon = strrchr(enum_name, ':');
                // ':' will always appear in pairs
                Q_ASSERT(colon <= enum_name || *(colon-1) == ':');
                if (colon > enum_name) {
                    int len = colon-enum_name-1;
                    scope_buffer = (char *)malloc(len+1);
                    memcpy(scope_buffer, enum_name, len);
                    scope_buffer[len] = '\0';
                    scope_name = scope_buffer;
                    enum_name = colon+1;
                }

                const QMetaObject *scope = 0;
                if (qstrcmp(scope_name, "Qt") == 0)
                    scope = &QObject::staticQtMetaObject;
                else
                    scope = QMetaObject_findMetaObject(this, scope_name);
                if (scope)
                    result.menum = scope->enumerator(scope->indexOfEnumerator(enum_name));
                if (scope_buffer)
                    free(scope_buffer);
            }
        }
    }
    return result;
}

/*!
    \since 4.2

    Returns the property that has the \c USER flag set to true.

    \sa QMetaProperty::isUser()
*/
QMetaProperty QMetaObject::userProperty() const
{
    const int propCount = propertyCount();
    for (int i = propCount - 1; i >= 0; --i) {
        const QMetaProperty prop = property(i);
        if (prop.isUser())
            return prop;
    }
    return QMetaProperty();
}

/*!
    Returns the meta-data for the item of class information with the
    given \a index.

    Example:

    \snippet code/src_corelib_kernel_qmetaobject.cpp 0

    \sa classInfoCount(), classInfoOffset(), indexOfClassInfo()
 */
QMetaClassInfo QMetaObject::classInfo(int index) const
{
    int i = index;
    i -= classInfoOffset();
    if (i < 0 && d.superdata)
        return d.superdata->classInfo(index);

    QMetaClassInfo result;
    if (i >= 0 && i < priv(d.data)->classInfoCount) {
        result.mobj = this;
        result.handle = priv(d.data)->classInfoData + 2*i;
    }
    return result;
}

/*!
    Returns \c true if the \a signal and \a method arguments are
    compatible; otherwise returns \c false.

    Both \a signal and \a method are expected to be normalized.

    \sa normalizedSignature()
*/
bool QMetaObject::checkConnectArgs(const char *signal, const char *method)
{
    const char *s1 = signal;
    const char *s2 = method;
    while (*s1++ != '(') { }                        // scan to first '('
    while (*s2++ != '(') { }
    if (*s2 == ')' || qstrcmp(s1,s2) == 0)        // method has no args or
        return true;                                //   exact match
    int s1len = qstrlen(s1);
    int s2len = qstrlen(s2);
    if (s2len < s1len && strncmp(s1,s2,s2len-1)==0 && s1[s2len-1]==',')
        return true;                                // method has less args
    return false;
}

/*!
    \since 5.0
    \overload

    Returns \c true if the \a signal and \a method arguments are
    compatible; otherwise returns \c false.
*/
bool QMetaObject::checkConnectArgs(const QMetaMethod &signal,
                                   const QMetaMethod &method)
{
    return QMetaObjectPrivate::checkConnectArgs(
            QMetaMethodPrivate::get(&signal),
            QMetaMethodPrivate::get(&method));
}

static void qRemoveWhitespace(const char *s, char *d)
{
    char last = 0;
    while (*s && is_space(*s))
        s++;
    while (*s) {
        while (*s && !is_space(*s))
            last = *d++ = *s++;
        while (*s && is_space(*s))
            s++;
        if (*s && ((is_ident_char(*s) && is_ident_char(last))
                   || ((*s == ':') && (last == '<')))) {
            last = *d++ = ' ';
        }
    }
    *d = '\0';
}

static char *qNormalizeType(char *d, int &templdepth, QByteArray &result)
{
    const char *t = d;
    while (*d && (templdepth
                   || (*d != ',' && *d != ')'))) {
        if (*d == '<')
            ++templdepth;
        if (*d == '>')
            --templdepth;
        ++d;
    }
    // "void" should only be removed if this is part of a signature that has
    // an explicit void argument; e.g., "void foo(void)" --> "void foo()"
    if (strncmp("void)", t, d - t + 1) != 0)
        result += normalizeTypeInternal(t, d);

    return d;
}


/*!
    \since 4.2

    Normalizes a \a type.

    See QMetaObject::normalizedSignature() for a description on how
    Qt normalizes.

    Example:

    \snippet code/src_corelib_kernel_qmetaobject.cpp 1

    \sa normalizedSignature()
 */
QByteArray QMetaObject::normalizedType(const char *type)
{
    QByteArray result;

    if (!type || !*type)
        return result;

    QVarLengthArray<char> stackbuf(qstrlen(type) + 1);
    qRemoveWhitespace(type, stackbuf.data());
    int templdepth = 0;
    qNormalizeType(stackbuf.data(), templdepth, result);

    return result;
}

/*!
    Normalizes the signature of the given \a method.

    Qt uses normalized signatures to decide whether two given signals
    and slots are compatible. Normalization reduces whitespace to a
    minimum, moves 'const' to the front where appropriate, removes
    'const' from value types and replaces const references with
    values.

    \sa checkConnectArgs(), normalizedType()
 */
QByteArray QMetaObject::normalizedSignature(const char *method)
{
    QByteArray result;
    if (!method || !*method)
        return result;
    int len = int(strlen(method));
    QVarLengthArray<char> stackbuf(len + 1);
    char *d = stackbuf.data();
    qRemoveWhitespace(method, d);

    result.reserve(len);

    int argdepth = 0;
    int templdepth = 0;
    while (*d) {
        if (argdepth == 1) {
            d = qNormalizeType(d, templdepth, result);
            if (!*d) //most likely an invalid signature.
                break;
        }
        if (*d == '(')
            ++argdepth;
        if (*d == ')')
            --argdepth;
        result += *d++;
    }

    return result;
}

enum { MaximumParamCount = 11 }; // up to 10 arguments + 1 return value

/*!
    Invokes the \a member (a signal or a slot name) on the object \a
    obj. Returns \c true if the member could be invoked. Returns \c false
    if there is no such member or the parameters did not match.

    The invocation can be either synchronous or asynchronous,
    depending on \a type:

    \list
    \li If \a type is Qt::DirectConnection, the member will be invoked immediately.

    \li If \a type is Qt::QueuedConnection,
       a QEvent will be sent and the member is invoked as soon as the application
       enters the main event loop.

    \li If \a type is Qt::BlockingQueuedConnection, the method will be invoked in
       the same way as for Qt::QueuedConnection, except that the current thread
       will block until the event is delivered. Using this connection type to
       communicate between objects in the same thread will lead to deadlocks.

    \li If \a type is Qt::AutoConnection, the member is invoked
       synchronously if \a obj lives in the same thread as the
       caller; otherwise it will invoke the member asynchronously.
    \endlist

    The return value of the \a member function call is placed in \a
    ret. If the invocation is asynchronous, the return value cannot
    be evaluated. You can pass up to ten arguments (\a val0, \a val1,
    \a val2, \a val3, \a val4, \a val5, \a val6, \a val7, \a val8,
    and \a val9) to the \a member function.

    QGenericArgument and QGenericReturnArgument are internal
    helper classes. Because signals and slots can be dynamically
    invoked, you must enclose the arguments using the Q_ARG() and
    Q_RETURN_ARG() macros. Q_ARG() takes a type name and a
    const reference of that type; Q_RETURN_ARG() takes a type name
    and a non-const reference.

    You only need to pass the name of the signal or slot to this function,
    not the entire signature. For example, to asynchronously invoke
    the \l{QThread::quit()}{quit()} slot on a
    QThread, use the following code:

    \snippet code/src_corelib_kernel_qmetaobject.cpp 2

    With asynchronous method invocations, the parameters must be of
    types that are known to Qt's meta-object system, because Qt needs
    to copy the arguments to store them in an event behind the
    scenes. If you try to use a queued connection and get the error
    message

    \snippet code/src_corelib_kernel_qmetaobject.cpp 3

    call qRegisterMetaType() to register the data type before you
    call invokeMethod().

    To synchronously invoke the \c compute(QString, int, double) slot on
    some arbitrary object \c obj retrieve its return value:

    \snippet code/src_corelib_kernel_qmetaobject.cpp 4

    If the "compute" slot does not take exactly one QString, one int
    and one double in the specified order, the call will fail.

    \sa Q_ARG(), Q_RETURN_ARG(), qRegisterMetaType(), QMetaMethod::invoke()
*/
bool QMetaObject::invokeMethod(QObject *obj,
                               const char *member,
                               Qt::ConnectionType type,
                               QGenericReturnArgument ret,
                               QGenericArgument val0,
                               QGenericArgument val1,
                               QGenericArgument val2,
                               QGenericArgument val3,
                               QGenericArgument val4,
                               QGenericArgument val5,
                               QGenericArgument val6,
                               QGenericArgument val7,
                               QGenericArgument val8,
                               QGenericArgument val9)
{
    if (!obj)
        return false;

    QVarLengthArray<char, 512> sig;
    int len = qstrlen(member);
    if (len <= 0)
        return false;
    sig.append(member, len);
    sig.append('(');

    const char *typeNames[] = {ret.name(), val0.name(), val1.name(), val2.name(), val3.name(),
                               val4.name(), val5.name(), val6.name(), val7.name(), val8.name(),
                               val9.name()};

    int paramCount;
    for (paramCount = 1; paramCount < MaximumParamCount; ++paramCount) {
        len = qstrlen(typeNames[paramCount]);
        if (len <= 0)
            break;
        sig.append(typeNames[paramCount], len);
        sig.append(',');
    }
    if (paramCount == 1)
        sig.append(')'); // no parameters
    else
        sig[sig.size() - 1] = ')';
    sig.append('\0');

    const QMetaObject *meta = obj->metaObject();
    int idx = meta->indexOfMethod(sig.constData());
    if (idx < 0) {
        QByteArray norm = QMetaObject::normalizedSignature(sig.constData());
        idx = meta->indexOfMethod(norm.constData());
    }

    if (idx < 0 || idx >= meta->methodCount()) {
        qWarning("QMetaObject::invokeMethod: No such method %s::%s",
                 meta->className(), sig.constData());
        return false;
    }
    QMetaMethod method = meta->method(idx);
    return method.invoke(obj, type, ret,
                         val0, val1, val2, val3, val4, val5, val6, val7, val8, val9);
}

/*! \fn bool QMetaObject::invokeMethod(QObject *obj, const char *member,
                                       QGenericReturnArgument ret,
                                       QGenericArgument val0 = QGenericArgument(0),
                                       QGenericArgument val1 = QGenericArgument(),
                                       QGenericArgument val2 = QGenericArgument(),
                                       QGenericArgument val3 = QGenericArgument(),
                                       QGenericArgument val4 = QGenericArgument(),
                                       QGenericArgument val5 = QGenericArgument(),
                                       QGenericArgument val6 = QGenericArgument(),
                                       QGenericArgument val7 = QGenericArgument(),
                                       QGenericArgument val8 = QGenericArgument(),
                                       QGenericArgument val9 = QGenericArgument());
    \overload invokeMethod()

    This overload always invokes the member using the connection type Qt::AutoConnection.
*/

/*! \fn bool QMetaObject::invokeMethod(QObject *obj, const char *member,
                             Qt::ConnectionType type,
                             QGenericArgument val0 = QGenericArgument(0),
                             QGenericArgument val1 = QGenericArgument(),
                             QGenericArgument val2 = QGenericArgument(),
                             QGenericArgument val3 = QGenericArgument(),
                             QGenericArgument val4 = QGenericArgument(),
                             QGenericArgument val5 = QGenericArgument(),
                             QGenericArgument val6 = QGenericArgument(),
                             QGenericArgument val7 = QGenericArgument(),
                             QGenericArgument val8 = QGenericArgument(),
                             QGenericArgument val9 = QGenericArgument())

    \overload invokeMethod()

    This overload can be used if the return value of the member is of no interest.
*/

/*!
    \fn bool QMetaObject::invokeMethod(QObject *obj, const char *member,
                             QGenericArgument val0 = QGenericArgument(0),
                             QGenericArgument val1 = QGenericArgument(),
                             QGenericArgument val2 = QGenericArgument(),
                             QGenericArgument val3 = QGenericArgument(),
                             QGenericArgument val4 = QGenericArgument(),
                             QGenericArgument val5 = QGenericArgument(),
                             QGenericArgument val6 = QGenericArgument(),
                             QGenericArgument val7 = QGenericArgument(),
                             QGenericArgument val8 = QGenericArgument(),
                             QGenericArgument val9 = QGenericArgument())

    \overload invokeMethod()

    This overload invokes the member using the connection type Qt::AutoConnection and
    ignores return values.
*/

/*!
    \class QMetaMethod
    \inmodule QtCore

    \brief The QMetaMethod class provides meta-data about a member
    function.

    \ingroup objectmodel

    A QMetaMethod has a methodType(), a methodSignature(), a list of
    parameterTypes() and parameterNames(), a return typeName(), a
    tag(), and an access() specifier. You can use invoke() to invoke
    the method on an arbitrary QObject.

    \sa QMetaObject, QMetaEnum, QMetaProperty, {Qt's Property System}
*/

/*!
    \enum QMetaMethod::Attributes

    \internal

    \value Compatibility
    \value Cloned
    \value Scriptable
*/

/*!
    \fn bool QMetaMethod::isValid() const
    \since 5.0

    Returns \c true if this method is valid (can be introspected and
    invoked), otherwise returns \c false.
*/

/*! \fn bool operator==(const QMetaMethod &m1, const QMetaMethod &m2)
    \since 5.0
    \relates QMetaMethod
    \overload

    Returns \c true if method \a m1 is equal to method \a m2,
    otherwise returns \c false.
*/

/*! \fn bool operator!=(const QMetaMethod &m1, const QMetaMethod &m2)
    \since 5.0
    \relates QMetaMethod
    \overload

    Returns \c true if method \a m1 is not equal to method \a m2,
    otherwise returns \c false.
*/

/*!
    \fn const QMetaObject *QMetaMethod::enclosingMetaObject() const
    \internal
*/

/*!
    \enum QMetaMethod::MethodType

    \value Method  The function is a plain member function.
    \value Signal  The function is a signal.
    \value Slot    The function is a slot.
    \value Constructor The function is a constructor.
*/

/*!
    \fn QMetaMethod::QMetaMethod()
    \internal
*/

/*!
    \macro Q_METAMETHOD_INVOKE_MAX_ARGS
    \relates QMetaMethod

    Equals maximum number of arguments available for
    execution of the method via QMetaMethod::invoke()
 */

QByteArray QMetaMethodPrivate::signature() const
{
    Q_ASSERT(priv(mobj->d.data)->revision >= 7);
    QByteArray result;
    result.reserve(256);
    result += name();
    result += '(';
    QList<QByteArray> argTypes = parameterTypes();
    for (int i = 0; i < argTypes.size(); ++i) {
        if (i)
            result += ',';
        result += argTypes.at(i);
    }
    result += ')';
    return result;
}

QByteArray QMetaMethodPrivate::name() const
{
    Q_ASSERT(priv(mobj->d.data)->revision >= 7);
    return stringData(mobj, mobj->d.data[handle]);
}

int QMetaMethodPrivate::typesDataIndex() const
{
    Q_ASSERT(priv(mobj->d.data)->revision >= 7);
    return mobj->d.data[handle + 2];
}

const char *QMetaMethodPrivate::rawReturnTypeName() const
{
    Q_ASSERT(priv(mobj->d.data)->revision >= 7);
    uint typeInfo = mobj->d.data[typesDataIndex()];
    if (typeInfo & IsUnresolvedType)
        return rawStringData(mobj, typeInfo & TypeNameIndexMask);
    else
        return QMetaType::typeName(typeInfo);
}

int QMetaMethodPrivate::returnType() const
{
    return parameterType(-1);
}

int QMetaMethodPrivate::parameterCount() const
{
    Q_ASSERT(priv(mobj->d.data)->revision >= 7);
    return mobj->d.data[handle + 1];
}

int QMetaMethodPrivate::parametersDataIndex() const
{
    Q_ASSERT(priv(mobj->d.data)->revision >= 7);
    return typesDataIndex() + 1;
}

uint QMetaMethodPrivate::parameterTypeInfo(int index) const
{
    Q_ASSERT(priv(mobj->d.data)->revision >= 7);
    return mobj->d.data[parametersDataIndex() + index];
}

int QMetaMethodPrivate::parameterType(int index) const
{
    Q_ASSERT(priv(mobj->d.data)->revision >= 7);
    return typeFromTypeInfo(mobj, parameterTypeInfo(index));
}

void QMetaMethodPrivate::getParameterTypes(int *types) const
{
    Q_ASSERT(priv(mobj->d.data)->revision >= 7);
    int dataIndex = parametersDataIndex();
    int argc = parameterCount();
    for (int i = 0; i < argc; ++i) {
        int id = typeFromTypeInfo(mobj, mobj->d.data[dataIndex++]);
        *(types++) = id;
    }
}

QList<QByteArray> QMetaMethodPrivate::parameterTypes() const
{
    Q_ASSERT(priv(mobj->d.data)->revision >= 7);
    int argc = parameterCount();
    QList<QByteArray> list;
    list.reserve(argc);
    int paramsIndex = parametersDataIndex();
    for (int i = 0; i < argc; ++i)
        list += typeNameFromTypeInfo(mobj, mobj->d.data[paramsIndex + i]);
    return list;
}

QList<QByteArray> QMetaMethodPrivate::parameterNames() const
{
    Q_ASSERT(priv(mobj->d.data)->revision >= 7);
    int argc = parameterCount();
    QList<QByteArray> list;
    list.reserve(argc);
    int namesIndex = parametersDataIndex() + argc;
    for (int i = 0; i < argc; ++i)
        list += stringData(mobj, mobj->d.data[namesIndex + i]);
    return list;
}

QByteArray QMetaMethodPrivate::tag() const
{
    Q_ASSERT(priv(mobj->d.data)->revision >= 7);
    return stringData(mobj, mobj->d.data[handle + 3]);
}

int QMetaMethodPrivate::ownMethodIndex() const
{
    // recompute the methodIndex by reversing the arithmetic in QMetaObject::property()
    return (handle - priv(mobj->d.data)->methodData) / 5;
}

/*!
    \since 5.0

    Returns the signature of this method (e.g.,
    \c{setValue(double)}).

    \sa parameterTypes(), parameterNames()
*/
QByteArray QMetaMethod::methodSignature() const
{
    if (!mobj)
        return QByteArray();
    return QMetaMethodPrivate::get(this)->signature();
}

/*!
    \since 5.0

    Returns the name of this method.

    \sa methodSignature(), parameterCount()
*/
QByteArray QMetaMethod::name() const
{
    if (!mobj)
        return QByteArray();
    return QMetaMethodPrivate::get(this)->name();
}

/*!
    \since 5.0

    Returns the return type of this method.

    The return value is one of the types that are registered
    with QMetaType, or QMetaType::UnknownType if the type is not registered.

    \sa parameterType(), QMetaType, typeName()
*/
int QMetaMethod::returnType() const
 {
     if (!mobj)
         return QMetaType::UnknownType;
    return QMetaMethodPrivate::get(this)->returnType();
}

/*!
    \since 5.0

    Returns the number of parameters of this method.

    \sa parameterType(), parameterNames()
*/
int QMetaMethod::parameterCount() const
{
    if (!mobj)
        return 0;
    return QMetaMethodPrivate::get(this)->parameterCount();
}

/*!
    \since 5.0

    Returns the type of the parameter at the given \a index.

    The return value is one of the types that are registered
    with QMetaType, or QMetaType::UnknownType if the type is not registered.

    \sa parameterCount(), returnType(), QMetaType
*/
int QMetaMethod::parameterType(int index) const
{
    if (!mobj || index < 0)
        return QMetaType::UnknownType;
    if (index >= QMetaMethodPrivate::get(this)->parameterCount())
        return QMetaType::UnknownType;

    int type = QMetaMethodPrivate::get(this)->parameterType(index);
    if (type != QMetaType::UnknownType)
        return type;

    void *argv[] = { &type, &index };
    mobj->static_metacall(QMetaObject::RegisterMethodArgumentMetaType, QMetaMethodPrivate::get(this)->ownMethodIndex(), argv);
    if (type != -1)
        return type;
    return QMetaType::UnknownType;
}

/*!
    \since 5.0
    \internal

    Gets the parameter \a types of this method. The storage
    for \a types must be able to hold parameterCount() items.

    \sa parameterCount(), returnType(), parameterType()
*/
void QMetaMethod::getParameterTypes(int *types) const
{
    if (!mobj)
        return;
    QMetaMethodPrivate::get(this)->getParameterTypes(types);
}

/*!
    Returns a list of parameter types.

    \sa parameterNames(), methodSignature()
*/
QList<QByteArray> QMetaMethod::parameterTypes() const
{
    if (!mobj)
        return QList<QByteArray>();
    return QMetaMethodPrivate::get(this)->parameterTypes();
}

/*!
    Returns a list of parameter names.

    \sa parameterTypes(), methodSignature()
*/
QList<QByteArray> QMetaMethod::parameterNames() const
{
    QList<QByteArray> list;
    if (!mobj)
        return list;
    return QMetaMethodPrivate::get(this)->parameterNames();
}


/*!
    Returns the return type name of this method.

    \sa returnType(), QMetaType::type()
*/
const char *QMetaMethod::typeName() const
{
    if (!mobj)
        return 0;
    return QMetaMethodPrivate::get(this)->rawReturnTypeName();
}

/*!
    Returns the tag associated with this method.

    Tags are special macros recognized by \c moc that make it
    possible to add extra information about a method.

    Tag information can be added in the following
    way in the function declaration:

    \code
        #ifndef Q_MOC_RUN
        // define the tag text
        #  define THISISTESTTAG
        #endif
        ...
        private slots:
            THISISTESTTAG void testFunc();
    \endcode

    and the information can be accessed by using:

    \code
        MainWindow win;
        win.show();

        int functionIndex = win.metaObject()->indexOfSlot("testFunc()");
        QMetaMethod mm = metaObject()->method(functionIndex);
        qDebug() << mm.tag(); // prints THISISTESTTAG
    \endcode

    For the moment, \c moc will extract and record all tags, but it will not
    handle any of them specially.

    \note Since Qt 5.0, \c moc expands preprocessor macros, so it is necessary
    to surround the definition with \c #ifndef \c Q_MOC_RUN, as shown in the
    example above. This was not required in Qt 4. The code as shown above works
    with Qt 4 too.
*/
const char *QMetaMethod::tag() const
{
    if (!mobj)
        return 0;
    return QMetaMethodPrivate::get(this)->tag().constData();
}


/*!
    \internal
 */
int QMetaMethod::attributes() const
{
    if (!mobj)
        return false;
    return ((mobj->d.data[handle + 4])>>4);
}

/*!
  \since 4.6

  Returns this method's index.
*/
int QMetaMethod::methodIndex() const
{
    if (!mobj)
        return -1;
    return QMetaMethodPrivate::get(this)->ownMethodIndex() + mobj->methodOffset();
}

// This method has been around for a while, but the documentation was marked \internal until 5.1
/*!
    \since 5.1
    Returns the method revision if one was
    specified by Q_REVISION, otherwise returns 0.
 */
int QMetaMethod::revision() const
{
    if (!mobj)
        return 0;
    if ((QMetaMethod::Access)(mobj->d.data[handle + 4] & MethodRevisioned)) {
        int offset = priv(mobj->d.data)->methodData
                     + priv(mobj->d.data)->methodCount * 5
                     + QMetaMethodPrivate::get(this)->ownMethodIndex();
        return mobj->d.data[offset];
    }
    return 0;
}

/*!
    Returns the access specification of this method (private,
    protected, or public).

    Signals are always protected, meaning that you can only emit them
    from the class or from a subclass.

    \sa methodType()
*/
QMetaMethod::Access QMetaMethod::access() const
{
    if (!mobj)
        return Private;
    return (QMetaMethod::Access)(mobj->d.data[handle + 4] & AccessMask);
}

/*!
    Returns the type of this method (signal, slot, or method).

    \sa access()
*/
QMetaMethod::MethodType QMetaMethod::methodType() const
{
    if (!mobj)
        return QMetaMethod::Method;
    return (QMetaMethod::MethodType)((mobj->d.data[handle + 4] & MethodTypeMask)>>2);
}

/*!
    \fn QMetaMethod QMetaMethod::fromSignal(PointerToMemberFunction signal)
    \since 5.0

    Returns the meta-method that corresponds to the given \a signal, or an
    invalid QMetaMethod if \a signal is not a signal of the class.

    Example:

    \snippet code/src_corelib_kernel_qmetaobject.cpp 9
*/

/*!
    \internal

    Implementation of the fromSignal() function.

    \a metaObject is the class's meta-object
    \a signal is a pointer to a pointer to a member signal of the class
*/
QMetaMethod QMetaMethod::fromSignalImpl(const QMetaObject *metaObject, void **signal)
{
    int i = -1;
    void *args[] = { &i, signal };
    QMetaMethod result;
    for (const QMetaObject *m = metaObject; m; m = m->d.superdata) {
        m->static_metacall(QMetaObject::IndexOfMethod, 0, args);
        if (i >= 0) {
            result.mobj = m;
            result.handle = priv(m->d.data)->methodData + 5*i;
            break;
        }
    }
    return result;
}

/*!
    Invokes this method on the object \a object. Returns \c true if the member could be invoked.
    Returns \c false if there is no such member or the parameters did not match.

    The invocation can be either synchronous or asynchronous, depending on the
    \a connectionType:

    \list
    \li If \a connectionType is Qt::DirectConnection, the member will be invoked immediately.

    \li If \a connectionType is Qt::QueuedConnection,
       a QEvent will be posted and the member is invoked as soon as the application
       enters the main event loop.

    \li If \a connectionType is Qt::AutoConnection, the member is invoked
       synchronously if \a object lives in the same thread as the
       caller; otherwise it will invoke the member asynchronously.
    \endlist

    The return value of this method call is placed in \a
    returnValue. If the invocation is asynchronous, the return value cannot
    be evaluated. You can pass up to ten arguments (\a val0, \a val1,
    \a val2, \a val3, \a val4, \a val5, \a val6, \a val7, \a val8,
    and \a val9) to this method call.

    QGenericArgument and QGenericReturnArgument are internal
    helper classes. Because signals and slots can be dynamically
    invoked, you must enclose the arguments using the Q_ARG() and
    Q_RETURN_ARG() macros. Q_ARG() takes a type name and a
    const reference of that type; Q_RETURN_ARG() takes a type name
    and a non-const reference.

    To asynchronously invoke the
    \l{QPushButton::animateClick()}{animateClick()} slot on a
    QPushButton:

    \snippet code/src_corelib_kernel_qmetaobject.cpp 6

    With asynchronous method invocations, the parameters must be of
    types that are known to Qt's meta-object system, because Qt needs
    to copy the arguments to store them in an event behind the
    scenes. If you try to use a queued connection and get the error
    message

    \snippet code/src_corelib_kernel_qmetaobject.cpp 7

    call qRegisterMetaType() to register the data type before you
    call QMetaMethod::invoke().

    To synchronously invoke the \c compute(QString, int, double) slot on
    some arbitrary object \c obj retrieve its return value:

    \snippet code/src_corelib_kernel_qmetaobject.cpp 8

    QMetaObject::normalizedSignature() is used here to ensure that the format
    of the signature is what invoke() expects.  E.g. extra whitespace is
    removed.

    If the "compute" slot does not take exactly one QString, one int
    and one double in the specified order, the call will fail.

    \warning this method will not test the validity of the arguments: \a object
    must be an instance of the class of the QMetaObject of which this QMetaMethod
    has been constructed with.  The arguments must have the same type as the ones
    expected by the method, else, the behaviour is undefined.

    \sa Q_ARG(), Q_RETURN_ARG(), qRegisterMetaType(), QMetaObject::invokeMethod()
*/
bool QMetaMethod::invoke(QObject *object,
                         Qt::ConnectionType connectionType,
                         QGenericReturnArgument returnValue,
                         QGenericArgument val0,
                         QGenericArgument val1,
                         QGenericArgument val2,
                         QGenericArgument val3,
                         QGenericArgument val4,
                         QGenericArgument val5,
                         QGenericArgument val6,
                         QGenericArgument val7,
                         QGenericArgument val8,
                         QGenericArgument val9) const
{
    if (!object || !mobj)
        return false;

    Q_ASSERT(mobj->cast(object));

    // check return type
    if (returnValue.data()) {
        const char *retType = typeName();
        if (qstrcmp(returnValue.name(), retType) != 0) {
            // normalize the return value as well
            QByteArray normalized = QMetaObject::normalizedType(returnValue.name());
            if (qstrcmp(normalized.constData(), retType) != 0) {
                // String comparison failed, try compare the metatype.
                int t = returnType();
                if (t == QMetaType::UnknownType || t != QMetaType::type(normalized))
                    return false;
            }
        }
    }

    // check argument count (we don't allow invoking a method if given too few arguments)
    const char *typeNames[] = {
        returnValue.name(),
        val0.name(),
        val1.name(),
        val2.name(),
        val3.name(),
        val4.name(),
        val5.name(),
        val6.name(),
        val7.name(),
        val8.name(),
        val9.name()
    };
    int paramCount;
    for (paramCount = 1; paramCount < MaximumParamCount; ++paramCount) {
        if (qstrlen(typeNames[paramCount]) <= 0)
            break;
    }
    if (paramCount <= QMetaMethodPrivate::get(this)->parameterCount())
        return false;

    // check connection type
    QThread *currentThread = QThread::currentThread();
    QThread *objectThread = object->thread();
    if (connectionType == Qt::AutoConnection) {
        connectionType = currentThread == objectThread
                         ? Qt::DirectConnection
                         : Qt::QueuedConnection;
    }

#ifdef QT_NO_THREAD
    if (connectionType == Qt::BlockingQueuedConnection) {
        connectionType = Qt::DirectConnection;
    }
#endif

    // invoke!
    void *param[] = {
        returnValue.data(),
        val0.data(),
        val1.data(),
        val2.data(),
        val3.data(),
        val4.data(),
        val5.data(),
        val6.data(),
        val7.data(),
        val8.data(),
        val9.data()
    };
    int idx_relative = QMetaMethodPrivate::get(this)->ownMethodIndex();
    int idx_offset =  mobj->methodOffset();
    Q_ASSERT(QMetaObjectPrivate::get(mobj)->revision >= 6);
    QObjectPrivate::StaticMetaCallFunction callFunction = mobj->d.static_metacall;

    if (connectionType == Qt::DirectConnection) {
        if (callFunction) {
            callFunction(object, QMetaObject::InvokeMetaMethod, idx_relative, param);
            return true;
        } else {
            return QMetaObject::metacall(object, QMetaObject::InvokeMetaMethod, idx_relative + idx_offset, param) < 0;
        }
    } else if (connectionType == Qt::QueuedConnection) {
        if (returnValue.data()) {
            qWarning("QMetaMethod::invoke: Unable to invoke methods with return values in "
                     "queued connections");
            return false;
        }

        int nargs = 1; // include return type
        void **args = (void **) malloc(paramCount * sizeof(void *));
        Q_CHECK_PTR(args);
        int *types = (int *) malloc(paramCount * sizeof(int));
        Q_CHECK_PTR(types);
        types[0] = 0; // return type
        args[0] = 0;

        for (int i = 1; i < paramCount; ++i) {
            types[i] = QMetaType::type(typeNames[i]);
            if (types[i] != QMetaType::UnknownType) {
                args[i] = QMetaType::create(types[i], param[i]);
                ++nargs;
            } else if (param[i]) {
                // Try to register the type and try again before reporting an error.
                void *argv[] = { &types[i], &i };
                QMetaObject::metacall(object, QMetaObject::RegisterMethodArgumentMetaType,
                                      idx_relative + idx_offset, argv);
                if (types[i] == -1) {
                    qWarning("QMetaMethod::invoke: Unable to handle unregistered datatype '%s'",
                            typeNames[i]);
                    for (int x = 1; x < i; ++x) {
                        if (types[x] && args[x])
                            QMetaType::destroy(types[x], args[x]);
                    }
                    free(types);
                    free(args);
                    return false;
                }
            }
        }

        QCoreApplication::postEvent(object, new QMetaCallEvent(idx_offset, idx_relative, callFunction,
                                                        0, -1, nargs, types, args));
    } else { // blocking queued connection
#ifndef QT_NO_THREAD
        if (currentThread == objectThread) {
            qWarning("QMetaMethod::invoke: Dead lock detected in "
                        "BlockingQueuedConnection: Receiver is %s(%p)",
                        mobj->className(), object);
        }

        QSemaphore semaphore;
        QCoreApplication::postEvent(object, new QMetaCallEvent(idx_offset, idx_relative, callFunction,
                                                        0, -1, 0, 0, param, &semaphore));
        semaphore.acquire();
#endif // QT_NO_THREAD
    }
    return true;
}

/*! \fn bool QMetaMethod::invoke(QObject *object,
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
    \overload invoke()

    This overload always invokes this method using the connection type Qt::AutoConnection.
*/

/*! \fn bool QMetaMethod::invoke(QObject *object,
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

    \overload invoke()

    This overload can be used if the return value of the member is of no interest.
*/

/*!
    \fn bool QMetaMethod::invoke(QObject *object,
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

    \overload invoke()

    This overload invokes this method using the
    connection type Qt::AutoConnection and ignores return values.
*/

/*!
    \class QMetaEnum
    \inmodule QtCore
    \brief The QMetaEnum class provides meta-data about an enumerator.

    \ingroup objectmodel

    Use name() for the enumerator's name. The enumerator's keys (names
    of each enumerated item) are returned by key(); use keyCount() to find
    the number of keys. isFlag() returns whether the enumerator is
    meant to be used as a flag, meaning that its values can be combined
    using the OR operator.

    The conversion functions keyToValue(), valueToKey(), keysToValue(),
    and valueToKeys() allow conversion between the integer
    representation of an enumeration or set value and its literal
    representation. The scope() function returns the class scope this
    enumerator was declared in.

    \sa QMetaObject, QMetaMethod, QMetaProperty
*/

/*!
    \fn bool QMetaEnum::isValid() const

    Returns \c true if this enum is valid (has a name); otherwise returns
    false.

    \sa name()
*/

/*!
    \fn const QMetaObject *QMetaEnum::enclosingMetaObject() const
    \internal
*/


/*!
    \fn QMetaEnum::QMetaEnum()
    \internal
*/

/*!
    Returns the name of the enumerator (without the scope).

    For example, the Qt::AlignmentFlag enumeration has \c
    AlignmentFlag as the name and \l Qt as the scope.

    \sa isValid(), scope()
*/
const char *QMetaEnum::name() const
{
    if (!mobj)
        return 0;
    return rawStringData(mobj, mobj->d.data[handle]);
}

/*!
    Returns the number of keys.

    \sa key()
*/
int QMetaEnum::keyCount() const
{
    if (!mobj)
        return 0;
    return mobj->d.data[handle + 2];
}


/*!
    Returns the key with the given \a index, or 0 if no such key exists.

    \sa keyCount(), value(), valueToKey()
*/
const char *QMetaEnum::key(int index) const
{
    if (!mobj)
        return 0;
    int count = mobj->d.data[handle + 2];
    int data = mobj->d.data[handle + 3];
    if (index >= 0  && index < count)
        return rawStringData(mobj, mobj->d.data[data + 2*index]);
    return 0;
}

/*!
    Returns the value with the given \a index; or returns -1 if there
    is no such value.

    \sa keyCount(), key(), keyToValue()
*/
int QMetaEnum::value(int index) const
{
    if (!mobj)
        return 0;
    int count = mobj->d.data[handle + 2];
    int data = mobj->d.data[handle + 3];
    if (index >= 0  && index < count)
        return mobj->d.data[data + 2*index + 1];
    return -1;
}


/*!
    Returns \c true if this enumerator is used as a flag; otherwise returns
    false.

    When used as flags, enumerators can be combined using the OR
    operator.

    \sa keysToValue(), valueToKeys()
*/
bool QMetaEnum::isFlag() const
{
    return mobj && mobj->d.data[handle + 1];
}


/*!
    Returns the scope this enumerator was declared in.

    For example, the Qt::AlignmentFlag enumeration has \c Qt as
    the scope and \c AlignmentFlag as the name.

    \sa name()
*/
const char *QMetaEnum::scope() const
{
    return mobj?rawStringData(mobj, 0) : 0;
}

/*!
    Returns the integer value of the given enumeration \a key, or -1
    if \a key is not defined.

    If \a key is not defined, *\a{ok} is set to false; otherwise
    *\a{ok} is set to true.

    For flag types, use keysToValue().

    \sa valueToKey(), isFlag(), keysToValue()
*/
int QMetaEnum::keyToValue(const char *key, bool *ok) const
{
    if (ok != 0)
        *ok = false;
    if (!mobj || !key)
        return -1;
    uint scope = 0;
    const char *qualified_key = key;
    const char *s = key + qstrlen(key);
    while (s > key && *s != ':')
        --s;
    if (s > key && *(s-1)==':') {
        scope = s - key - 1;
        key += scope + 2;
    }
    int count = mobj->d.data[handle + 2];
    int data = mobj->d.data[handle + 3];
    for (int i = 0; i < count; ++i) {
        if ((!scope || (stringSize(mobj, 0) == int(scope) && strncmp(qualified_key, rawStringData(mobj, 0), scope) == 0))
             && strcmp(key, rawStringData(mobj, mobj->d.data[data + 2*i])) == 0) {
            if (ok != 0)
                *ok = true;
            return mobj->d.data[data + 2*i + 1];
        }
    }
    return -1;
}

/*!
    Returns the string that is used as the name of the given
    enumeration \a value, or 0 if \a value is not defined.

    For flag types, use valueToKeys().

    \sa isFlag(), valueToKeys()
*/
const char* QMetaEnum::valueToKey(int value) const
{
    if (!mobj)
        return 0;
    int count = mobj->d.data[handle + 2];
    int data = mobj->d.data[handle + 3];
    for (int i = 0; i < count; ++i)
        if (value == (int)mobj->d.data[data + 2*i + 1])
            return rawStringData(mobj, mobj->d.data[data + 2*i]);
    return 0;
}

/*!
    Returns the value derived from combining together the values of
    the \a keys using the OR operator, or -1 if \a keys is not
    defined. Note that the strings in \a keys must be '|'-separated.

    If \a keys is not defined, *\a{ok} is set to false; otherwise
    *\a{ok} is set to true.

    \sa isFlag(), valueToKey(), valueToKeys()
*/
int QMetaEnum::keysToValue(const char *keys, bool *ok) const
{
    if (ok != 0)
        *ok = false;
    if (!mobj || !keys)
        return -1;
    if (ok != 0)
        *ok = true;
    QStringList l = QString::fromLatin1(keys).split(QLatin1Char('|'));
    if (l.isEmpty())
        return 0;
    //#### TODO write proper code, do not use QStringList
    int value = 0;
    int count = mobj->d.data[handle + 2];
    int data = mobj->d.data[handle + 3];
    for (int li = 0; li < l.size(); ++li) {
        QString trimmed = l.at(li).trimmed();
        QByteArray qualified_key = trimmed.toLatin1();
        const char *key = qualified_key.constData();
        uint scope = 0;
        const char *s = key + qstrlen(key);
        while (s > key && *s != ':')
            --s;
        if (s > key && *(s-1)==':') {
            scope = s - key - 1;
            key += scope + 2;
        }
        int i;
        for (i = count-1; i >= 0; --i)
            if ((!scope || (stringSize(mobj, 0) == int(scope) && strncmp(qualified_key.constData(), rawStringData(mobj, 0), scope) == 0))
                 && strcmp(key, rawStringData(mobj, mobj->d.data[data + 2*i])) == 0) {
                value |= mobj->d.data[data + 2*i + 1];
                break;
            }
        if (i < 0) {
            if (ok != 0)
                *ok = false;
            value |= -1;
        }
    }
    return value;
}

/*!
    Returns a byte array of '|'-separated keys that represents the
    given \a value.

    \sa isFlag(), valueToKey(), keysToValue()
*/
QByteArray QMetaEnum::valueToKeys(int value) const
{
    QByteArray keys;
    if (!mobj)
        return keys;
    int count = mobj->d.data[handle + 2];
    int data = mobj->d.data[handle + 3];
    int v = value;
    for(int i = 0; i < count; i++) {
        int k = mobj->d.data[data + 2*i + 1];
        if ((k != 0 && (v & k) == k ) ||  (k == value))  {
            v = v & ~k;
            if (!keys.isEmpty())
                keys += '|';
            keys += stringData(mobj, mobj->d.data[data + 2*i]);
        }
    }
    return keys;
}

static QByteArray qualifiedName(const QMetaEnum &e)
{
    return QByteArray(e.scope()) + "::" + e.name();
}

/*!
    \class QMetaProperty
    \inmodule QtCore
    \brief The QMetaProperty class provides meta-data about a property.

    \ingroup objectmodel

    Property meta-data is obtained from an object's meta-object. See
    QMetaObject::property() and QMetaObject::propertyCount() for
    details.

    \section1 Property Meta-Data

    A property has a name() and a type(), as well as various
    attributes that specify its behavior: isReadable(), isWritable(),
    isDesignable(), isScriptable(), revision(), and isStored().

    If the property is an enumeration, isEnumType() returns \c true; if the
    property is an enumeration that is also a flag (i.e. its values
    can be combined using the OR operator), isEnumType() and
    isFlagType() both return true. The enumerator for these types is
    available from enumerator().

    The property's values are set and retrieved with read(), write(),
    and reset(); they can also be changed through QObject's set and get
    functions. See QObject::setProperty() and QObject::property() for
    details.

    \section1 Copying and Assignment

    QMetaProperty objects can be copied by value. However, each copy will
    refer to the same underlying property meta-data.

    \sa QMetaObject, QMetaEnum, QMetaMethod, {Qt's Property System}
*/

/*!
    \fn bool QMetaProperty::isValid() const

    Returns \c true if this property is valid (readable); otherwise
    returns \c false.

    \sa isReadable()
*/

/*!
    \fn const QMetaObject *QMetaProperty::enclosingMetaObject() const
    \internal
*/

/*!
    \internal
*/
QMetaProperty::QMetaProperty()
    : mobj(0), handle(0), idx(0)
{
}


/*!
    Returns this property's name.

    \sa type(), typeName()
*/
const char *QMetaProperty::name() const
{
    if (!mobj)
        return 0;
    int handle = priv(mobj->d.data)->propertyData + 3*idx;
    return rawStringData(mobj, mobj->d.data[handle]);
}

/*!
    Returns the name of this property's type.

    \sa type(), name()
*/
const char *QMetaProperty::typeName() const
{
    if (!mobj)
        return 0;
    int handle = priv(mobj->d.data)->propertyData + 3*idx;
    return rawTypeNameFromTypeInfo(mobj, mobj->d.data[handle + 1]);
}

/*!
    Returns this property's type. The return value is one
    of the values of the QVariant::Type enumeration.

    \sa userType(), typeName(), name()
*/
QVariant::Type QMetaProperty::type() const
{
    if (!mobj)
        return QVariant::Invalid;
    int handle = priv(mobj->d.data)->propertyData + 3*idx;

    Q_ASSERT(priv(mobj->d.data)->revision >= 7);
    uint type = typeFromTypeInfo(mobj, mobj->d.data[handle + 1]);
    if (type >= QMetaType::User)
        return QVariant::UserType;
    if (type != QMetaType::UnknownType)
        return QVariant::Type(type);
    if (isEnumType()) {
        int enumMetaTypeId = QMetaType::type(qualifiedName(menum));
        if (enumMetaTypeId == QMetaType::UnknownType)
            return QVariant::Int;
    }
#ifdef QT_COORD_TYPE
    // qreal metatype must be resolved at runtime.
    if (strcmp(typeName(), "qreal") == 0)
        return QVariant::Type(qMetaTypeId<qreal>());
#endif

    return QVariant::UserType;
}

/*!
    \since 4.2

    Returns this property's user type. The return value is one
    of the values that are registered with QMetaType, or QMetaType::UnknownType if
    the type is not registered.

    \sa type(), QMetaType, typeName()
 */
int QMetaProperty::userType() const
{
    if (!mobj)
        return QMetaType::UnknownType;
    Q_ASSERT(priv(mobj->d.data)->revision >= 7);
    int handle = priv(mobj->d.data)->propertyData + 3*idx;
    int type = typeFromTypeInfo(mobj, mobj->d.data[handle + 1]);
    if (type != QMetaType::UnknownType)
        return type;
    if (isEnumType()) {
        type = QMetaType::type(qualifiedName(menum));
        if (type == QMetaType::UnknownType) {
            void *argv[] = { &type };
            mobj->static_metacall(QMetaObject::RegisterPropertyMetaType, idx, argv);
            if (type == -1 || type == QMetaType::UnknownType)
                return QVariant::Int; // Match behavior of QMetaType::type()
        }
        return type;
    }
    type = QMetaType::type(typeName());
    if (type != QMetaType::UnknownType)
        return type;
    void *argv[] = { &type };
    mobj->static_metacall(QMetaObject::RegisterPropertyMetaType, idx, argv);
    if (type != -1)
        return type;
    return QMetaType::UnknownType;
}

/*!
  \since 4.6

  Returns this property's index.
*/
int QMetaProperty::propertyIndex() const
{
    if (!mobj)
        return -1;
    return idx + mobj->propertyOffset();
}

/*!
    Returns \c true if the property's type is an enumeration value that
    is used as a flag; otherwise returns \c false.

    Flags can be combined using the OR operator. A flag type is
    implicitly also an enum type.

    \sa isEnumType(), enumerator(), QMetaEnum::isFlag()
*/

bool QMetaProperty::isFlagType() const
{
    return isEnumType() && menum.isFlag();
}

/*!
    Returns \c true if the property's type is an enumeration value;
    otherwise returns \c false.

    \sa enumerator(), isFlagType()
*/
bool QMetaProperty::isEnumType() const
{
    if (!mobj)
        return false;
    int handle = priv(mobj->d.data)->propertyData + 3*idx;
    int flags = mobj->d.data[handle + 2];
    return (flags & EnumOrFlag) && menum.name();
}

/*!
    \internal

    Returns \c true if the property has a C++ setter function that
    follows Qt's standard "name" / "setName" pattern. Designer and uic
    query hasStdCppSet() in order to avoid expensive
    QObject::setProperty() calls. All properties in Qt [should] follow
    this pattern.
*/
bool QMetaProperty::hasStdCppSet() const
{
    if (!mobj)
        return false;
    int handle = priv(mobj->d.data)->propertyData + 3*idx;
    int flags = mobj->d.data[handle + 2];
    return (flags & StdCppSet);
}

/*!
    Returns the enumerator if this property's type is an enumerator
    type; otherwise the returned value is undefined.

    \sa isEnumType(), isFlagType()
*/
QMetaEnum QMetaProperty::enumerator() const
{
    return menum;
}

/*!
    Reads the property's value from the given \a object. Returns the value
    if it was able to read it; otherwise returns an invalid variant.

    \sa write(), reset(), isReadable()
*/
QVariant QMetaProperty::read(const QObject *object) const
{
    if (!object || !mobj)
        return QVariant();

    uint t = QVariant::Int;
    if (isEnumType()) {
        /*
          try to create a QVariant that can be converted to this enum
          type (only works if the enum has already been registered
          with QMetaType)
        */
        int enumMetaTypeId = QMetaType::type(qualifiedName(menum));
        if (enumMetaTypeId != QMetaType::UnknownType)
            t = enumMetaTypeId;
    } else {
        int handle = priv(mobj->d.data)->propertyData + 3*idx;
        const char *typeName = 0;
        Q_ASSERT(priv(mobj->d.data)->revision >= 7);
        uint typeInfo = mobj->d.data[handle + 1];
        if (!(typeInfo & IsUnresolvedType))
            t = typeInfo;
        else {
            typeName = rawStringData(mobj, typeInfo & TypeNameIndexMask);
            t = QMetaType::type(typeName);
        }
        if (t == QMetaType::UnknownType) {
            // Try to register the type and try again before reporting an error.
            int registerResult = -1;
            void *argv[] = { &registerResult };
            QMetaObject::metacall(const_cast<QObject*>(object), QMetaObject::RegisterPropertyMetaType,
                                  idx + mobj->propertyOffset(), argv);
            if (registerResult == -1) {
                qWarning("QMetaProperty::read: Unable to handle unregistered datatype '%s' for property '%s::%s'", typeName, mobj->className(), name());
                return QVariant();
            }
            t = registerResult;
        }
    }

    // the status variable is changed by qt_metacall to indicate what it did
    // this feature is currently only used by Qt D-Bus and should not be depended
    // upon. Don't change it without looking into QDBusAbstractInterface first
    // -1 (unchanged): normal qt_metacall, result stored in argv[0]
    // changed: result stored directly in value
    int status = -1;
    QVariant value;
    void *argv[] = { 0, &value, &status };
    if (t == QMetaType::QVariant) {
        argv[0] = &value;
    } else {
        value = QVariant(t, (void*)0);
        argv[0] = value.data();
    }
    QMetaObject::metacall(const_cast<QObject*>(object), QMetaObject::ReadProperty,
                          idx + mobj->propertyOffset(), argv);

    if (status != -1)
        return value;
    if (t != QMetaType::QVariant && argv[0] != value.data())
        // pointer or reference
        return QVariant((QVariant::Type)t, argv[0]);
    return value;
}

/*!
    Writes \a value as the property's value to the given \a object. Returns
    true if the write succeeded; otherwise returns \c false.

    \sa read(), reset(), isWritable()
*/
bool QMetaProperty::write(QObject *object, const QVariant &value) const
{
    if (!object || !isWritable())
        return false;

    QVariant v = value;
    uint t = QVariant::Invalid;
    if (isEnumType()) {
        if (v.type() == QVariant::String) {
            bool ok;
            if (isFlagType())
                v = QVariant(menum.keysToValue(value.toByteArray(), &ok));
            else
                v = QVariant(menum.keyToValue(value.toByteArray(), &ok));
            if (!ok)
                return false;
        } else if (v.type() != QVariant::Int && v.type() != QVariant::UInt) {
            int enumMetaTypeId = QMetaType::type(qualifiedName(menum));
            if ((enumMetaTypeId == QMetaType::UnknownType) || (v.userType() != enumMetaTypeId) || !v.constData())
                return false;
            v = QVariant(*reinterpret_cast<const int *>(v.constData()));
        }
        v.convert(QVariant::Int);
    } else {
        int handle = priv(mobj->d.data)->propertyData + 3*idx;
        const char *typeName = 0;
        Q_ASSERT(priv(mobj->d.data)->revision >= 7);
        uint typeInfo = mobj->d.data[handle + 1];
        if (!(typeInfo & IsUnresolvedType))
            t = typeInfo;
        else {
            typeName = rawStringData(mobj, typeInfo & TypeNameIndexMask);
            t = QMetaType::type(typeName);
        }
        if (t == QMetaType::UnknownType) {
            Q_ASSERT(typeName != 0);
            const char *vtypeName = value.typeName();
            if (vtypeName && strcmp(typeName, vtypeName) == 0)
                t = value.userType();
            else
                t = QVariant::nameToType(typeName);
        }
        if (t == QVariant::Invalid)
            return false;
        if (t != QMetaType::QVariant && t != (uint)value.userType() && (t < QMetaType::User && !v.convert((QVariant::Type)t)))
            return false;
    }

    // the status variable is changed by qt_metacall to indicate what it did
    // this feature is currently only used by Qt D-Bus and should not be depended
    // upon. Don't change it without looking into QDBusAbstractInterface first
    // -1 (unchanged): normal qt_metacall, result stored in argv[0]
    // changed: result stored directly in value, return the value of status
    int status = -1;
    // the flags variable is used by the declarative module to implement
    // interception of property writes.
    int flags = 0;
    void *argv[] = { 0, &v, &status, &flags };
    if (t == QMetaType::QVariant)
        argv[0] = &v;
    else
        argv[0] = v.data();
    QMetaObject::metacall(object, QMetaObject::WriteProperty, idx + mobj->propertyOffset(), argv);
    return status;
}

/*!
    Resets the property for the given \a object with a reset method.
    Returns \c true if the reset worked; otherwise returns \c false.

    Reset methods are optional; only a few properties support them.

    \sa read(), write()
*/
bool QMetaProperty::reset(QObject *object) const
{
    if (!object || !mobj || !isResettable())
        return false;
    void *argv[] = { 0 };
    QMetaObject::metacall(object, QMetaObject::ResetProperty, idx + mobj->propertyOffset(), argv);
    return true;
}

/*!
    Returns \c true if this property can be reset to a default value; otherwise
    returns \c false.

    \sa reset()
*/
bool QMetaProperty::isResettable() const
{
    if (!mobj)
        return false;
    int flags = mobj->d.data[handle + 2];
    return flags & Resettable;
}

/*!
    Returns \c true if this property is readable; otherwise returns \c false.

    \sa isWritable(), read(), isValid()
 */
bool QMetaProperty::isReadable() const
{
    if (!mobj)
        return false;
    int flags = mobj->d.data[handle + 2];
    return flags & Readable;
}

/*!
    Returns \c true if this property has a corresponding change notify signal;
    otherwise returns \c false.

    \sa notifySignal()
 */
bool QMetaProperty::hasNotifySignal() const
{
    if (!mobj)
        return false;
    int flags = mobj->d.data[handle + 2];
    return flags & Notify;
}

/*!
    \since 4.5

    Returns the QMetaMethod instance of the property change notifying signal if
    one was specified, otherwise returns an invalid QMetaMethod.

    \sa hasNotifySignal()
 */
QMetaMethod QMetaProperty::notifySignal() const
{
    int id = notifySignalIndex();
    if (id != -1)
        return mobj->method(id);
    else
        return QMetaMethod();
}

/*!
    \since 4.6

    Returns the index of the property change notifying signal if one was
    specified, otherwise returns -1.

    \sa hasNotifySignal()
 */
int QMetaProperty::notifySignalIndex() const
{
    if (hasNotifySignal()) {
        int offset = priv(mobj->d.data)->propertyData +
                     priv(mobj->d.data)->propertyCount * 3 + idx;
        return mobj->d.data[offset] + mobj->methodOffset();
    } else {
        return -1;
    }
}

// This method has been around for a while, but the documentation was marked \internal until 5.1
/*!
    \since 5.1

    Returns the property revision if one was
    specified by REVISION, otherwise returns 0.
 */
int QMetaProperty::revision() const
{
    if (!mobj)
        return 0;
    int flags = mobj->d.data[handle + 2];
    if (flags & Revisioned) {
        int offset = priv(mobj->d.data)->propertyData +
                     priv(mobj->d.data)->propertyCount * 3 + idx;
        // Revision data is placed after NOTIFY data, if present.
        // Iterate through properties to discover whether we have NOTIFY signals.
        for (int i = 0; i < priv(mobj->d.data)->propertyCount; ++i) {
            int handle = priv(mobj->d.data)->propertyData + 3*i;
            if (mobj->d.data[handle + 2] & Notify) {
                offset += priv(mobj->d.data)->propertyCount;
                break;
            }
        }
        return mobj->d.data[offset];
    } else {
        return 0;
    }
}

/*!
    Returns \c true if this property is writable; otherwise returns
    false.

    \sa isReadable(), write()
 */
bool QMetaProperty::isWritable() const
{
    if (!mobj)
        return false;
    int flags = mobj->d.data[handle + 2];
    return flags & Writable;
}


/*!
    Returns \c true if this property is designable for the given \a object;
    otherwise returns \c false.

    If no \a object is given, the function returns \c false if the
    \c{Q_PROPERTY()}'s \c DESIGNABLE attribute is false; otherwise
    returns \c true (if the attribute is true or is a function or expression).

    \sa isScriptable(), isStored()
*/
bool QMetaProperty::isDesignable(const QObject *object) const
{
    if (!mobj)
        return false;
    int flags = mobj->d.data[handle + 2];
    bool b = flags & Designable;
    if (object) {
        void *argv[] = { &b };
        QMetaObject::metacall(const_cast<QObject*>(object), QMetaObject::QueryPropertyDesignable,
                              idx + mobj->propertyOffset(), argv);
    }
    return b;


}

/*!
    Returns \c true if the property is scriptable for the given \a object;
    otherwise returns \c false.

    If no \a object is given, the function returns \c false if the
    \c{Q_PROPERTY()}'s \c SCRIPTABLE attribute is false; otherwise returns
    true (if the attribute is true or is a function or expression).

    \sa isDesignable(), isStored()
*/
bool QMetaProperty::isScriptable(const QObject *object) const
{
    if (!mobj)
        return false;
    int flags = mobj->d.data[handle + 2];
    bool b = flags & Scriptable;
    if (object) {
        void *argv[] = { &b };
        QMetaObject::metacall(const_cast<QObject*>(object), QMetaObject::QueryPropertyScriptable,
                              idx + mobj->propertyOffset(), argv);
    }
    return b;
}

/*!
    Returns \c true if the property is stored for \a object; otherwise returns
    false.

    If no \a object is given, the function returns \c false if the
    \c{Q_PROPERTY()}'s \c STORED attribute is false; otherwise returns
    true (if the attribute is true or is a function or expression).

    \sa isDesignable(), isScriptable()
*/
bool QMetaProperty::isStored(const QObject *object) const
{
    if (!mobj)
        return false;
    int flags = mobj->d.data[handle + 2];
    bool b = flags & Stored;
    if (object) {
        void *argv[] = { &b };
        QMetaObject::metacall(const_cast<QObject*>(object), QMetaObject::QueryPropertyStored,
                              idx + mobj->propertyOffset(), argv);
    }
    return b;
}

/*!
    Returns \c true if this property is designated as the \c USER
    property, i.e., the one that the user can edit for \a object or
    that is significant in some other way.  Otherwise it returns
    false. e.g., the \c text property is the \c USER editable property
    of a QLineEdit.

    If \a object is null, the function returns \c false if the \c
    {Q_PROPERTY()}'s \c USER attribute is false. Otherwise it returns
    true.

    \sa QMetaObject::userProperty(), isDesignable(), isScriptable()
*/
bool QMetaProperty::isUser(const QObject *object) const
{
    if (!mobj)
        return false;
    int flags = mobj->d.data[handle + 2];
    bool b = flags & User;
    if (object) {
        void *argv[] = { &b };
        QMetaObject::metacall(const_cast<QObject*>(object), QMetaObject::QueryPropertyUser,
                              idx + mobj->propertyOffset(), argv);
    }
    return b;
}

/*!
    \since 4.6
    Returns \c true if the property is constant; otherwise returns \c false.

    A property is constant if the \c{Q_PROPERTY()}'s \c CONSTANT attribute
    is set.
*/
bool QMetaProperty::isConstant() const
{
    if (!mobj)
        return false;
    int flags = mobj->d.data[handle + 2];
    return flags & Constant;
}

/*!
    \since 4.6
    Returns \c true if the property is final; otherwise returns \c false.

    A property is final if the \c{Q_PROPERTY()}'s \c FINAL attribute
    is set.
*/
bool QMetaProperty::isFinal() const
{
    if (!mobj)
        return false;
    int flags = mobj->d.data[handle + 2];
    return flags & Final;
}

/*!
    \obsolete

    Returns \c true if the property is editable for the given \a object;
    otherwise returns \c false.

    If no \a object is given, the function returns \c false if the
    \c{Q_PROPERTY()}'s \c EDITABLE attribute is false; otherwise returns
    true (if the attribute is true or is a function or expression).

    \sa isDesignable(), isScriptable(), isStored()
*/
bool QMetaProperty::isEditable(const QObject *object) const
{
    if (!mobj)
        return false;
    int flags = mobj->d.data[handle + 2];
    bool b = flags & Editable;
    if (object) {
        void *argv[] = { &b };
        QMetaObject::metacall(const_cast<QObject*>(object), QMetaObject::QueryPropertyEditable,
                              idx + mobj->propertyOffset(), argv);
    }
    return b;
}

/*!
    \class QMetaClassInfo
    \inmodule QtCore

    \brief The QMetaClassInfo class provides additional information
    about a class.

    \ingroup objectmodel

    Class information items are simple \e{name}--\e{value} pairs that
    are specified using Q_CLASSINFO() in the source code. The
    information can be retrieved using name() and value(). For example:

    \snippet code/src_corelib_kernel_qmetaobject.cpp 5

    This mechanism is free for you to use in your Qt applications. Qt
    doesn't use it for any of its classes.

    \sa QMetaObject
*/


/*!
    \fn QMetaClassInfo::QMetaClassInfo()
    \internal
*/

/*!
    \fn const QMetaObject *QMetaClassInfo::enclosingMetaObject() const
    \internal
*/

/*!
    Returns the name of this item.

    \sa value()
*/
const char *QMetaClassInfo::name() const
{
    if (!mobj)
        return 0;
    return rawStringData(mobj, mobj->d.data[handle]);
}

/*!
    Returns the value of this item.

    \sa name()
*/
const char* QMetaClassInfo::value() const
{
    if (!mobj)
        return 0;
    return rawStringData(mobj, mobj->d.data[handle + 1]);
}

/*!
    \macro QGenericArgument Q_ARG(Type, const Type &value)
    \relates QMetaObject

    This macro takes a \a Type and a \a value of that type and
    returns a \l QGenericArgument object that can be passed to
    QMetaObject::invokeMethod().

    \sa Q_RETURN_ARG()
*/

/*!
    \macro QGenericReturnArgument Q_RETURN_ARG(Type, Type &value)
    \relates QMetaObject

    This macro takes a \a Type and a non-const reference to a \a
    value of that type and returns a QGenericReturnArgument object
    that can be passed to QMetaObject::invokeMethod().

    \sa Q_ARG()
*/

/*!
    \class QGenericArgument
    \inmodule QtCore

    \brief The QGenericArgument class is an internal helper class for
    marshalling arguments.

    This class should never be used directly. Please use the \l Q_ARG()
    macro instead.

    \sa Q_ARG(), QMetaObject::invokeMethod(), QGenericReturnArgument
*/

/*!
    \fn QGenericArgument::QGenericArgument(const char *name, const void *data)

    Constructs a QGenericArgument object with the given \a name and \a data.
*/

/*!
    \fn QGenericArgument::data () const

    Returns the data set in the constructor.
*/

/*!
    \fn QGenericArgument::name () const

    Returns the name set in the constructor.
*/

/*!
    \class QGenericReturnArgument
    \inmodule QtCore

    \brief The QGenericReturnArgument class is an internal helper class for
    marshalling arguments.

    This class should never be used directly. Please use the
    Q_RETURN_ARG() macro instead.

    \sa Q_RETURN_ARG(), QMetaObject::invokeMethod(), QGenericArgument
*/

/*!
    \fn QGenericReturnArgument::QGenericReturnArgument(const char *name, void *data)

    Constructs a QGenericReturnArgument object with the given \a name
    and \a data.
*/

/*!
    \internal
    If the local_method_index is a cloned method, return the index of the original.

    Example: if the index of "destroyed()" is passed, the index of "destroyed(QObject*)" is returned
 */
int QMetaObjectPrivate::originalClone(const QMetaObject *mobj, int local_method_index)
{
    Q_ASSERT(local_method_index < get(mobj)->methodCount);
    int handle = get(mobj)->methodData + 5 * local_method_index;
    while (mobj->d.data[handle + 4] & MethodCloned) {
        Q_ASSERT(local_method_index > 0);
        handle -= 5;
        local_method_index--;
    }
    return local_method_index;
}

/*!
    \internal

    Returns the parameter type names extracted from the given \a signature.
*/
QList<QByteArray> QMetaObjectPrivate::parameterTypeNamesFromSignature(const char *signature)
{
    QList<QByteArray> list;
    while (*signature && *signature != '(')
        ++signature;
    while (*signature && *signature != ')' && *++signature != ')') {
        const char *begin = signature;
        int level = 0;
        while (*signature && (level > 0 || *signature != ',') && *signature != ')') {
            if (*signature == '<')
                ++level;
            else if (*signature == '>')
                --level;
            ++signature;
        }
        list += QByteArray(begin, signature - begin);
    }
    return list;
}

QT_END_NAMESPACE
