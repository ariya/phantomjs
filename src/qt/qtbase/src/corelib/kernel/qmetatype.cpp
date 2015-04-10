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

#include "qmetatype.h"
#include "qmetatype_p.h"
#include "qobjectdefs.h"
#include "qdatetime.h"
#include "qbytearray.h"
#include "qreadwritelock.h"
#include "qstring.h"
#include "qstringlist.h"
#include "qvector.h"
#include "qlocale.h"
#include "qeasingcurve.h"
#include "quuid.h"
#include "qvariant.h"
#include "qmetatypeswitcher_p.h"

#ifndef QT_BOOTSTRAPPED
#  include "qbitarray.h"
#  include "qurl.h"
#  include "qvariant.h"
#  include "qabstractitemmodel.h"
#  include "qregularexpression.h"
#  include "qjsonvalue.h"
#  include "qjsonobject.h"
#  include "qjsonarray.h"
#  include "qjsondocument.h"
#endif

#ifndef QT_NO_GEOM_VARIANT
# include "qsize.h"
# include "qpoint.h"
# include "qrect.h"
# include "qline.h"
#endif

QT_BEGIN_NAMESPACE

#define NS(x) QT_PREPEND_NAMESPACE(x)


namespace {
struct DefinedTypesFilter {
    template<typename T>
    struct Acceptor {
        static const bool IsAccepted = QtMetaTypePrivate::TypeDefinition<T>::IsAvailable && QModulesPrivate::QTypeModuleInfo<T>::IsCore;
    };
};
} // namespace

/*!
    \macro Q_DECLARE_OPAQUE_POINTER(PointerType)
    \relates QMetaType
    \since 5.0

    This macro enables pointers to forward-declared types (\a PointerType)
    to be registered with QMetaType using either Q_DECLARE_METATYPE()
    or qRegisterMetaType().

    \sa Q_DECLARE_METATYPE(), qRegisterMetaType()
*/

/*!
    \macro Q_DECLARE_METATYPE(Type)
    \relates QMetaType

    This macro makes the type \a Type known to QMetaType as long as it
    provides a public default constructor, a public copy constructor and
    a public destructor.
    It is needed to use the type \a Type as a custom type in QVariant.

    This macro requires that \a Type is a fully defined type at the point where
    it is used. For pointer types, it also requires that the pointed to type is
    fully defined. Use in conjunction with Q_DECLARE_OPAQUE_POINTER() to
    register pointers to forward declared types.

    Ideally, this macro should be placed below the declaration of
    the class or struct. If that is not possible, it can be put in
    a private header file which has to be included every time that
    type is used in a QVariant.

    Adding a Q_DECLARE_METATYPE() makes the type known to all template
    based functions, including QVariant. Note that if you intend to
    use the type in \e queued signal and slot connections or in
    QObject's property system, you also have to call
    qRegisterMetaType() since the names are resolved at runtime.

    This example shows a typical use case of Q_DECLARE_METATYPE():

    \snippet code/src_corelib_kernel_qmetatype.cpp 0

    If \c MyStruct is in a namespace, the Q_DECLARE_METATYPE() macro
    has to be outside the namespace:

    \snippet code/src_corelib_kernel_qmetatype.cpp 1

    Since \c{MyStruct} is now known to QMetaType, it can be used in QVariant:

    \snippet code/src_corelib_kernel_qmetatype.cpp 2

    \sa qRegisterMetaType()
*/

/*!
    \macro Q_DECLARE_SEQUENTIAL_CONTAINER_METATYPE(Container)
    \relates QMetaType

    This macro makes the container \a Container known to QMetaType as a sequential
    container. This makes it possible to put an instance of Container<T> into
    a QVariant, if T itself is known to QMetaType.

    Note that all of the Qt sequential containers already have built-in
    support, and it is not necessary to use this macro with them. The
    std::vector and std::list containers also have built-in support.

    This example shows a typical use of Q_DECLARE_SEQUENTIAL_CONTAINER_METATYPE():

    \snippet code/src_corelib_kernel_qmetatype.cpp 10
*/

/*!
    \macro Q_DECLARE_ASSOCIATIVE_CONTAINER_METATYPE(Container)
    \relates QMetaType

    This macro makes the container \a Container known to QMetaType as an associative
    container. This makes it possible to put an instance of Container<T, U> into
    a QVariant, if T and U are themselves known to QMetaType.

    Note that all of the Qt associative containers already have built-in
    support, and it is not necessary to use this macro with them. The
    std::map container also has built-in support.

    This example shows a typical use of Q_DECLARE_ASSOCIATIVE_CONTAINER_METATYPE():

    \snippet code/src_corelib_kernel_qmetatype.cpp 11
*/

/*!
    \macro Q_DECLARE_SMART_POINTER_METATYPE(SmartPointer)
    \relates QMetaType

    This macro makes the smart pointer \a SmartPointer known to QMetaType as a
    smart pointer. This makes it possible to put an instance of SmartPointer<T> into
    a QVariant, if T is a type which inherits QObject.

    Note that the QWeakPointer, QSharedPointer and QPointer already have built-in
    support, and it is not necessary to use this macro with them.

    This example shows a typical use of Q_DECLARE_SMART_POINTER_METATYPE():

    \snippet code/src_corelib_kernel_qmetatype.cpp 13
*/

/*!
    \enum QMetaType::Type

    These are the built-in types supported by QMetaType:

    \value Void \c void
    \value Bool \c bool
    \value Int \c int
    \value UInt \c{unsigned int}
    \value Double \c double
    \value QChar QChar
    \value QString QString
    \value QByteArray QByteArray

    \value VoidStar \c{void *}
    \value Long \c{long}
    \value LongLong LongLong
    \value Short \c{short}
    \value Char \c{char}
    \value ULong \c{unsigned long}
    \value ULongLong ULongLong
    \value UShort \c{unsigned short}
    \value SChar \c{signed char}
    \value UChar \c{unsigned char}
    \value Float \c float
    \value QObjectStar QObject *
    \value QVariant QVariant

    \value QCursor QCursor
    \value QDate QDate
    \value QSize QSize
    \value QTime QTime
    \value QVariantList QVariantList
    \value QPolygon QPolygon
    \value QPolygonF QPolygonF
    \value QColor QColor
    \value QSizeF QSizeF
    \value QRectF QRectF
    \value QLine QLine
    \value QTextLength QTextLength
    \value QStringList QStringList
    \value QVariantMap QVariantMap
    \value QVariantHash QVariantHash
    \value QIcon QIcon
    \value QPen QPen
    \value QLineF QLineF
    \value QTextFormat QTextFormat
    \value QRect QRect
    \value QPoint QPoint
    \value QUrl QUrl
    \value QRegExp QRegExp
    \value QRegularExpression QRegularExpression
    \value QDateTime QDateTime
    \value QPointF QPointF
    \value QPalette QPalette
    \value QFont QFont
    \value QBrush QBrush
    \value QRegion QRegion
    \value QBitArray QBitArray
    \value QImage QImage
    \value QKeySequence QKeySequence
    \value QSizePolicy QSizePolicy
    \value QPixmap QPixmap
    \value QLocale QLocale
    \value QBitmap QBitmap
    \value QMatrix QMatrix
    \value QTransform QTransform
    \value QMatrix4x4 QMatrix4x4
    \value QVector2D QVector2D
    \value QVector3D QVector3D
    \value QVector4D QVector4D
    \value QQuaternion QQuaternion
    \value QEasingCurve QEasingCurve
    \value QJsonValue QJsonValue
    \value QJsonObject QJsonObject
    \value QJsonArray QJsonArray
    \value QJsonDocument QJsonDocument
    \value QModelIndex QModelIndex
    \value QUuid QUuid

    \value User  Base value for user types
    \value UnknownType This is an invalid type id. It is returned from QMetaType for types that are not registered

    Additional types can be registered using Q_DECLARE_METATYPE().

    \sa type(), typeName()
*/

/*!
    \enum QMetaType::TypeFlag

    The enum describes attributes of a type supported by QMetaType.

    \value NeedsConstruction This type has non-trivial constructors. If the flag is not set instances can be safely initialized with memset to 0.
    \value NeedsDestruction This type has a non-trivial destructor. If the flag is not set calls to the destructor are not necessary before discarding objects.
    \value MovableType An instance of a type having this attribute can be safely moved by memcpy.
    \omitvalue SharedPointerToQObject
    \omitvalue IsEnumeration
    \omitvalue PointerToQObject
    \omitvalue WeakPointerToQObject
    \omitvalue TrackingPointerToQObject
    \omitvalue WasDeclaredAsMetaType
*/

/*!
    \class QMetaType
    \inmodule QtCore
    \brief The QMetaType class manages named types in the meta-object system.

    \ingroup objectmodel
    \threadsafe

    The class is used as a helper to marshall types in QVariant and
    in queued signals and slots connections. It associates a type
    name to a type so that it can be created and destructed
    dynamically at run-time. Declare new types with Q_DECLARE_METATYPE()
    to make them available to QVariant and other template-based functions.
    Call qRegisterMetaType() to make type available to non-template based
    functions, such as the queued signal and slot connections.

    Any class or struct that has a public default
    constructor, a public copy constructor, and a public destructor
    can be registered.

    The following code allocates and destructs an instance of
    \c{MyClass}:

    \snippet code/src_corelib_kernel_qmetatype.cpp 3

    If we want the stream operators \c operator<<() and \c
    operator>>() to work on QVariant objects that store custom types,
    the custom type must provide \c operator<<() and \c operator>>()
    operators.

    \sa Q_DECLARE_METATYPE(), QVariant::setValue(), QVariant::value(), QVariant::fromValue()
*/

/*!
    \fn bool QMetaType::isValid() const
    \since 5.0

    Returns \c true if this QMetaType object contains valid
    information about a type, false otherwise.
*/

/*!
    \fn bool QMetaType::isRegistered() const
    \since 5.0

    Returns \c true if this QMetaType object contains valid
    information about a type, false otherwise.
*/

/*!
    \fn bool QMetaType::sizeOf() const
    \since 5.0

    Returns the size of the type in bytes (i.e. sizeof(T),
    where T is the actual type for which this QMetaType instance
    was constructed for).

    This function is typically used together with construct()
    to perform low-level management of the memory used by a type.

    \sa QMetaType::construct(), QMetaType::sizeOf()
*/

/*!
    \fn TypeFlags QMetaType::flags() const
    \since 5.0

    Returns flags of the type for which this QMetaType instance was constructed.

    \sa QMetaType::TypeFlags, QMetaType::typeFlags()
*/

/*!
    \fn const QMetaObject *QMetaType::metaObject() const
    \since 5.0
    \internal
*/

/*!
    \fn void *QMetaType::create(const void *copy = 0) const
    \since 5.0

    Returns a copy of \a copy, assuming it is of the type that this
    QMetaType instance was created for. If \a copy is null, creates
    a default constructed instance.

    \sa QMetaType::destroy()
*/

/*!
    \fn void QMetaType::destroy(void *data) const
    \since 5.0

    Destroys the \a data, assuming it is of the type that this
    QMetaType instance was created for.

    \sa QMetaType::create()
*/

/*!
    \fn void *QMetaType::construct(void *where, const void *copy = 0) const
    \since 5.0

    Constructs a value of the type that this QMetaType instance
    was constructed for in the existing memory addressed by \a where,
    that is a copy of \a copy, and returns \a where. If \a copy is
    zero, the value is default constructed.

    This is a low-level function for explicitly managing the memory
    used to store the type. Consider calling create() if you don't
    need this level of control (that is, use "new" rather than
    "placement new").

    You must ensure that \a where points to a location where the new
    value can be stored and that \a where is suitably aligned.
    The type's size can be queried by calling sizeOf().

    The rule of thumb for alignment is that a type is aligned to its
    natural boundary, which is the smallest power of 2 that is bigger
    than the type, unless that alignment is larger than the maximum
    useful alignment for the platform. For practical purposes,
    alignment larger than 2 * sizeof(void*) is only necessary for
    special hardware instructions (e.g., aligned SSE loads and stores
    on x86).
*/

/*!
    \fn void QMetaType::destruct(void *data) const
    \since 5.0

    Destructs the value, located at \a data, assuming that it is
    of the type for which this QMetaType instance was constructed for.

    Unlike destroy(), this function only invokes the type's
    destructor, it doesn't invoke the delete operator.
    \sa QMetaType::construct()
*/

/*!
    \fn QMetaType::~QMetaType()

    Destructs this object.
*/

#define QT_ADD_STATIC_METATYPE(MetaTypeName, MetaTypeId, RealName) \
    { #RealName, sizeof(#RealName) - 1, MetaTypeId },

#define QT_ADD_STATIC_METATYPE_ALIASES_ITER(MetaTypeName, MetaTypeId, AliasingName, RealNameStr) \
    { RealNameStr, sizeof(RealNameStr) - 1, QMetaType::MetaTypeName },

#define QT_ADD_STATIC_METATYPE_HACKS_ITER(MetaTypeName, TypeId, Name) \
    QT_ADD_STATIC_METATYPE(MetaTypeName, MetaTypeName, Name)

static const struct { const char * typeName; int typeNameLength; int type; } types[] = {
    QT_FOR_EACH_STATIC_TYPE(QT_ADD_STATIC_METATYPE)
    QT_FOR_EACH_STATIC_ALIAS_TYPE(QT_ADD_STATIC_METATYPE_ALIASES_ITER)
    QT_FOR_EACH_STATIC_HACKS_TYPE(QT_ADD_STATIC_METATYPE_HACKS_ITER)
    {0, 0, QMetaType::UnknownType}
};

Q_CORE_EXPORT const QMetaTypeInterface *qMetaTypeGuiHelper = 0;
Q_CORE_EXPORT const QMetaTypeInterface *qMetaTypeWidgetsHelper = 0;
Q_CORE_EXPORT const QMetaObject *qMetaObjectWidgetsHelper = 0;

class QCustomTypeInfo : public QMetaTypeInterface
{
public:
    QCustomTypeInfo()
        : alias(-1)
    {
        QMetaTypeInterface empty = QT_METATYPE_INTERFACE_INIT(void);
        *static_cast<QMetaTypeInterface*>(this) = empty;
    }
    QByteArray typeName;
    int alias;
};

template<typename T, typename Key>
class QMetaTypeFunctionRegistry
{
public:
    ~QMetaTypeFunctionRegistry()
    {
        const QWriteLocker locker(&lock);
        map.clear();
    }

    bool contains(Key k) const
    {
        const QReadLocker locker(&lock);
        return map.contains(k);
    }

    bool insertIfNotContains(Key k, const T *f)
    {
        const QWriteLocker locker(&lock);
        const T* &fun = map[k];
        if (fun != 0)
            return false;
        fun = f;
        return true;
    }

    const T *function(Key k) const
    {
        const QReadLocker locker(&lock);
        return map.value(k, 0);
    }

    void remove(int from, int to)
    {
        const Key k(from, to);
        const QWriteLocker locker(&lock);
        map.remove(k);
    }
private:
    mutable QReadWriteLock lock;
    QHash<Key, const T *> map;
};

typedef QMetaTypeFunctionRegistry<QtPrivate::AbstractConverterFunction,QPair<int,int> >
QMetaTypeConverterRegistry;
typedef QMetaTypeFunctionRegistry<QtPrivate::AbstractComparatorFunction,int>
QMetaTypeComparatorRegistry;
typedef QMetaTypeFunctionRegistry<QtPrivate::AbstractDebugStreamFunction,int>
QMetaTypeDebugStreamRegistry;

namespace
{
union CheckThatItIsPod
{   // This should break if QMetaTypeInterface is not a POD type
    QMetaTypeInterface iface;
};
}

Q_DECLARE_TYPEINFO(QCustomTypeInfo, Q_MOVABLE_TYPE);
Q_GLOBAL_STATIC(QVector<QCustomTypeInfo>, customTypes)
Q_GLOBAL_STATIC(QReadWriteLock, customTypesLock)
Q_GLOBAL_STATIC(QMetaTypeConverterRegistry, customTypesConversionRegistry)
Q_GLOBAL_STATIC(QMetaTypeComparatorRegistry, customTypesComparatorRegistry)
Q_GLOBAL_STATIC(QMetaTypeDebugStreamRegistry, customTypesDebugStreamRegistry)

/*!
    \fn bool QMetaType::registerConverter()
    \since 5.2
    Registers the possibility of an implicit conversion from type From to type To in the meta
    type system. Returns \c true if the registration succeeded, otherwise false.
*/

/*!
    \fn bool QMetaType::registerConverter(MemberFunction function)
    \since 5.2
    \overload
    Registers a method \a function like To From::function() const as converter from type From
    to type To in the meta type system. Returns \c true if the registration succeeded, otherwise false.
*/

/*!
    \fn bool QMetaType::registerConverter(MemberFunctionOk function)
    \since 5.2
    \overload
    Registers a method \a function like To From::function(bool *ok) const as converter from type From
    to type To in the meta type system. Returns \c true if the registration succeeded, otherwise false.
*/

/*!
    \fn bool QMetaType::registerConverter(UnaryFunction function)
    \since 5.2
    \overload
    Registers a unary function object \a function as converter from type From
    to type To in the meta type system. Returns \c true if the registration succeeded, otherwise false.
*/

/*!
    \fn bool QMetaType::registerComparators()
    \since 5.2
    Registers comparison operetarors for the user-registered type T. This requires T to have
    both an operator== and an operator<.
    Returns \c true if the registration succeeded, otherwise false.
*/

#ifndef QT_NO_DEBUG_STREAM
/*!
    \fn bool QMetaType::registerDebugStreamOperator()
    Registers the debug stream operator for the user-registered type T. This requires T to have
    an operator<<(QDebug dbg, T).
    Returns \c true if the registration succeeded, otherwise false.
*/
#endif

/*!
    Registers function \a f as converter function from type id \a from to \a to.
    If there's already a conversion registered, this does nothing but deleting \a f.
    Returns \c true if the registration succeeded, otherwise false.
    \since 5.2
    \internal
*/
bool QMetaType::registerConverterFunction(const QtPrivate::AbstractConverterFunction *f, int from, int to)
{
    if (!customTypesConversionRegistry()->insertIfNotContains(qMakePair(from, to), f)) {
        qWarning("Type conversion already registered from type %s to type %s",
                 QMetaType::typeName(from), QMetaType::typeName(to));
        return false;
    }
    return true;
}

/*!
    \internal

    Invoked automatically when a converter function object is destroyed.
 */
void QMetaType::unregisterConverterFunction(int from, int to)
{
    if (customTypesConversionRegistry.isDestroyed())
        return;
    customTypesConversionRegistry()->remove(from, to);
}

bool QMetaType::registerComparatorFunction(const QtPrivate::AbstractComparatorFunction *f, int type)
{
    if (!customTypesComparatorRegistry()->insertIfNotContains(type, f)) {
        qWarning("Comparators already registered for type %s", QMetaType::typeName(type));
        return false;
    }
    return true;
}

/*!
    \fn bool QMetaType::hasRegisteredComparators()
    Returns \c true, if the meta type system has registered comparators for type T.
    \since 5.2
 */

/*!
    Returns \c true, if the meta type system has registered comparators for type id \a typeId.
    \since 5.2
 */
bool QMetaType::hasRegisteredComparators(int typeId)
{
    return customTypesComparatorRegistry()->contains(typeId);
}

#ifndef QT_NO_DEBUG_STREAM
bool QMetaType::registerDebugStreamOperatorFunction(const QtPrivate::AbstractDebugStreamFunction *f,
                                                    int type)
{
    if (!customTypesDebugStreamRegistry()->insertIfNotContains(type, f)) {
        qWarning("Debug stream operator already registered for type %s", QMetaType::typeName(type));
        return false;
    }
    return true;
}

/*!
    \fn bool QMetaType::hasRegisteredDebugStreamOperator()
    Returns \c true, if the meta type system has a registered debug stream operator for type T.
    \since 5.2
 */

/*!
    Returns \c true, if the meta type system has a registered debug stream operator for type
    id \a typeId.
    \since 5.2
*/
bool QMetaType::hasRegisteredDebugStreamOperator(int typeId)
{
    return customTypesDebugStreamRegistry()->contains(typeId);
}
#endif

/*!
    Converts the object at \a from from \a fromTypeId to the preallocated space at \a to
    typed \a toTypeId. Returns \c true, if the conversion succeeded, otherwise false.
    \since 5.2
*/
bool QMetaType::convert(const void *from, int fromTypeId, void *to, int toTypeId)
{
    const QtPrivate::AbstractConverterFunction * const f =
        customTypesConversionRegistry()->function(qMakePair(fromTypeId, toTypeId));
    return f && f->convert(f, from, to);
}

/*!
    Compares the objects at \a lhs and \a rhs. Both objects need to be of type \a typeId.
    \a result is set to less than, equal to or greater than zero, if \a lhs is less than, equal to
    or greater than \a rhs. Returns \c true, if the comparison succeeded, otherwiess false.
    \since 5.2
*/
bool QMetaType::compare(const void *lhs, const void *rhs, int typeId, int* result)
{
    const QtPrivate::AbstractComparatorFunction * const f =
        customTypesComparatorRegistry()->function(typeId);
    if (!f)
        return false;
    if (f->equals(f, lhs, rhs))
        *result = 0;
    else
        *result = f->lessThan(f, lhs, rhs) ? -1 : 1;
    return true;
}

/*!
    Streams the object at \a rhs of type \a typeId to the debug stream \a dbg. Returns \c true
    on success, otherwise false.
    \since 5.2
*/
bool QMetaType::debugStream(QDebug& dbg, const void *rhs, int typeId)
{
    const QtPrivate::AbstractDebugStreamFunction * const f = customTypesDebugStreamRegistry()->function(typeId);
    if (!f)
        return false;
    f->stream(f, dbg, rhs);
    return true;
}

/*!
    \fn bool QMetaType::hasRegisteredConverterFunction()
    Returns \c true, if the meta type system has a registered conversion from type From to type To.
    \since 5.2
    \overload
    */

/*!
    Returns \c true, if the meta type system has a registered conversion from meta type id \a fromTypeId
    to \a toTypeId
    \since 5.2
*/
bool QMetaType::hasRegisteredConverterFunction(int fromTypeId, int toTypeId)
{
    return customTypesConversionRegistry()->contains(qMakePair(fromTypeId, toTypeId));
}

#ifndef QT_NO_DATASTREAM
/*!
    \internal
*/
void QMetaType::registerStreamOperators(const char *typeName, SaveOperator saveOp,
                                        LoadOperator loadOp)
{
    registerStreamOperators(type(typeName), saveOp, loadOp);
}

/*!
    \internal
*/
void QMetaType::registerStreamOperators(int idx, SaveOperator saveOp,
                                        LoadOperator loadOp)
{
    if (idx < User)
        return; //builtin types should not be registered;
    QVector<QCustomTypeInfo> *ct = customTypes();
    if (!ct)
        return;
    QWriteLocker locker(customTypesLock());
    QCustomTypeInfo &inf = (*ct)[idx - User];
    inf.saveOp = saveOp;
    inf.loadOp = loadOp;
}
#endif // QT_NO_DATASTREAM

/*!
    Returns the type name associated with the given \a typeId, or 0 if no
    matching type was found. The returned pointer must not be deleted.

    \sa type(), isRegistered(), Type
*/
const char *QMetaType::typeName(int typeId)
{
    const uint type = typeId;
    // In theory it can be filled during compilation time, but for some reason template code
    // that is able to do it causes GCC 4.6 to generate additional 3K of executable code. Probably
    // it is not worth of it.
    static const char *namesCache[QMetaType::HighestInternalId + 1];

    const char *result;
    if (type <= QMetaType::HighestInternalId && ((result = namesCache[type])))
        return result;

#define QT_METATYPE_TYPEID_TYPENAME_CONVERTER(MetaTypeName, TypeId, RealName) \
        case QMetaType::MetaTypeName: result = #RealName; break;

    switch (QMetaType::Type(type)) {
    QT_FOR_EACH_STATIC_TYPE(QT_METATYPE_TYPEID_TYPENAME_CONVERTER)

    default: {
        if (Q_UNLIKELY(type < QMetaType::User)) {
            return 0; // It can happen when someone cast int to QVariant::Type, we should not crash...
        } else {
            const QVector<QCustomTypeInfo> * const ct = customTypes();
            QReadLocker locker(customTypesLock());
            return ct && uint(ct->count()) > type - QMetaType::User && !ct->at(type - QMetaType::User).typeName.isEmpty()
                    ? ct->at(type - QMetaType::User).typeName.constData()
                    : 0;
        }
    }
    }
#undef QT_METATYPE_TYPEID_TYPENAME_CONVERTER

    Q_ASSERT(type <= QMetaType::HighestInternalId);
    namesCache[type] = result;
    return result;
}

/*!
    \internal
    Similar to QMetaType::type(), but only looks in the static set of types.
*/
static inline int qMetaTypeStaticType(const char *typeName, int length)
{
    int i = 0;
    while (types[i].typeName && ((length != types[i].typeNameLength)
                                 || strcmp(typeName, types[i].typeName))) {
        ++i;
    }
    return types[i].type;
}

/*!
    \internal
    Similar to QMetaType::type(), but only looks in the custom set of
    types, and doesn't lock the mutex.
*/
static int qMetaTypeCustomType_unlocked(const char *typeName, int length)
{
    const QVector<QCustomTypeInfo> * const ct = customTypes();
    if (!ct)
        return QMetaType::UnknownType;

    for (int v = 0; v < ct->count(); ++v) {
        const QCustomTypeInfo &customInfo = ct->at(v);
        if ((length == customInfo.typeName.size())
            && !strcmp(typeName, customInfo.typeName.constData())) {
            if (customInfo.alias >= 0)
                return customInfo.alias;
            return v + QMetaType::User;
        }
    }
    return QMetaType::UnknownType;
}

/*!
    \internal

    This function is needed until existing code outside of qtbase
    has been changed to call the new version of registerType().
 */
int QMetaType::registerType(const char *typeName, Deleter deleter,
                            Creator creator)
{
    return registerType(typeName, deleter, creator,
                        QtMetaTypePrivate::QMetaTypeFunctionHelper<void>::Destruct,
                        QtMetaTypePrivate::QMetaTypeFunctionHelper<void>::Construct, 0, TypeFlags(), 0);
}

/*!
    \internal
    \since 5.0

    Registers a user type for marshalling, with \a typeName, a \a
    deleter, a \a creator, a \a destructor, a \a constructor, and
    a \a size. Returns the type's handle, or -1 if the type could
    not be registered.
 */
int QMetaType::registerType(const char *typeName, Deleter deleter,
                            Creator creator,
                            Destructor destructor,
                            Constructor constructor,
                            int size, TypeFlags flags, const QMetaObject *metaObject)
{
#ifdef QT_NO_QOBJECT
    NS(QByteArray) normalizedTypeName = typeName;
#else
    NS(QByteArray) normalizedTypeName = QMetaObject::normalizedType(typeName);
#endif

    return registerNormalizedType(normalizedTypeName, deleter, creator, destructor, constructor, size, flags, metaObject);
}


/*!
    \internal
    \since 5.0

    Registers a user type for marshalling, with \a normalizedTypeName, a \a
    deleter, a \a creator, a \a destructor, a \a constructor, and
    a \a size. Returns the type's handle, or -1 if the type could
    not be registered.  Note that normalizedTypeName is not checked for
    conformance with Qt's normalized format, so it must already
    conform.
 */
int QMetaType::registerNormalizedType(const NS(QByteArray) &normalizedTypeName, Deleter deleter,
                            Creator creator,
                            Destructor destructor,
                            Constructor constructor,
                            int size, TypeFlags flags, const QMetaObject *metaObject)
{
    QVector<QCustomTypeInfo> *ct = customTypes();
    if (!ct || normalizedTypeName.isEmpty() || !deleter || !creator || !destructor || !constructor)
        return -1;

    int idx = qMetaTypeStaticType(normalizedTypeName.constData(),
                                  normalizedTypeName.size());

    int previousSize = 0;
    int previousFlags = 0;
    if (idx == UnknownType) {
        QWriteLocker locker(customTypesLock());
        idx = qMetaTypeCustomType_unlocked(normalizedTypeName.constData(),
                                           normalizedTypeName.size());
        if (idx == UnknownType) {
            QCustomTypeInfo inf;
            inf.typeName = normalizedTypeName;
            inf.creator = creator;
            inf.deleter = deleter;
#ifndef QT_NO_DATASTREAM
            inf.loadOp = 0;
            inf.saveOp = 0;
#endif
            inf.alias = -1;
            inf.constructor = constructor;
            inf.destructor = destructor;
            inf.size = size;
            inf.flags = flags;
            inf.metaObject = metaObject;
            idx = ct->size() + User;
            ct->append(inf);
            return idx;
        }

        if (idx >= User) {
            previousSize = ct->at(idx - User).size;
            previousFlags = ct->at(idx - User).flags;
        }
    }

    if (idx < User) {
        previousSize = QMetaType::sizeOf(idx);
        previousFlags = QMetaType::typeFlags(idx);
    }

    if (previousSize != size) {
        qFatal("QMetaType::registerType: Binary compatibility break "
            "-- Size mismatch for type '%s' [%i]. Previously registered "
            "size %i, now registering size %i.",
            normalizedTypeName.constData(), idx, previousSize, size);
    }

    // Ignore WasDeclaredAsMetaType inconsitency, to many users were hitting the problem
    previousFlags |= WasDeclaredAsMetaType;
    flags |= WasDeclaredAsMetaType;

    if (previousFlags != flags) {
        const int maskForTypeInfo = NeedsConstruction | NeedsDestruction | MovableType;
        const char *msg = "QMetaType::registerType: Binary compatibility break. "
                "\nType flags for type '%s' [%i] don't match. Previously "
                "registered TypeFlags(0x%x), now registering TypeFlags(0x%x). "
                "This is an ODR break, which means that your application depends on a C++ undefined behavior."
                "\nHint: %s";
        QT_PREPEND_NAMESPACE(QByteArray) hint;
        if ((previousFlags & maskForTypeInfo) != (flags & maskForTypeInfo)) {
            hint += "\nIt seems that the type was registered at least twice in a different translation units, "
                    "but Q_DECLARE_TYPEINFO is not visible from all the translations unit or different flags were used."
                    "Remember that Q_DECLARE_TYPEINFO should be declared before QMetaType registration, "
                    "preferably it should be placed just after the type declaration and before Q_DECLARE_METATYPE";
        }
        qFatal(msg, normalizedTypeName.constData(), idx, previousFlags, int(flags), hint.constData());
    }

    return idx;
}

/*!
    \internal
    \since 4.7

    Registers a user type for marshalling, as an alias of another type (typedef)
*/
int QMetaType::registerTypedef(const char* typeName, int aliasId)
{
#ifdef QT_NO_QOBJECT
    NS(QByteArray) normalizedTypeName = typeName;
#else
    NS(QByteArray) normalizedTypeName = QMetaObject::normalizedType(typeName);
#endif

    return registerNormalizedTypedef(normalizedTypeName, aliasId);
}

/*!
    \internal
    \since 5.0

    Registers a user type for marshalling, as an alias of another type (typedef).
    Note that normalizedTypeName is not checked for conformance with Qt's normalized format,
    so it must already conform.
*/
int QMetaType::registerNormalizedTypedef(const NS(QByteArray) &normalizedTypeName, int aliasId)
{
    QVector<QCustomTypeInfo> *ct = customTypes();
    if (!ct || normalizedTypeName.isEmpty())
        return -1;

    int idx = qMetaTypeStaticType(normalizedTypeName.constData(),
                                  normalizedTypeName.size());

    if (idx == UnknownType) {
        QWriteLocker locker(customTypesLock());
        idx = qMetaTypeCustomType_unlocked(normalizedTypeName.constData(),
                                               normalizedTypeName.size());

        if (idx == UnknownType) {
            QCustomTypeInfo inf;
            inf.typeName = normalizedTypeName;
            inf.alias = aliasId;
            inf.creator = 0;
            inf.deleter = 0;
            ct->append(inf);
            return aliasId;
        }
    }

    if (idx != aliasId) {
        qWarning("QMetaType::registerTypedef: "
                 "-- Type name '%s' previously registered as typedef of '%s' [%i], "
                 "now registering as typedef of '%s' [%i].",
                 normalizedTypeName.constData(), QMetaType::typeName(idx), idx,
                 QMetaType::typeName(aliasId), aliasId);
    }
    return idx;
}

/*!
    Returns \c true if the datatype with ID \a type is registered;
    otherwise returns \c false.

    \sa type(), typeName(), Type
*/
bool QMetaType::isRegistered(int type)
{
    // predefined type
    if ((type >= FirstCoreType && type <= LastCoreType)
        || (type >= FirstGuiType && type <= LastGuiType)
        || (type >= FirstWidgetsType && type <= LastWidgetsType)) {
        return true;
    }

    QReadLocker locker(customTypesLock());
    const QVector<QCustomTypeInfo> * const ct = customTypes();
    return ((type >= User) && (ct && ct->count() > type - User) && !ct->at(type - User).typeName.isEmpty());
}

/*!
    \fn int qMetaTypeTypeImpl(const char *typeName)
    \internal

    Implementation of QMetaType::type().
*/
template <bool tryNormalizedType>
static inline int qMetaTypeTypeImpl(const char *typeName)
{
    int length = qstrlen(typeName);
    if (!length)
        return QMetaType::UnknownType;
    int type = qMetaTypeStaticType(typeName, length);
    if (type == QMetaType::UnknownType) {
        QReadLocker locker(customTypesLock());
        type = qMetaTypeCustomType_unlocked(typeName, length);
#ifndef QT_NO_QOBJECT
        if ((type == QMetaType::UnknownType) && tryNormalizedType) {
            const NS(QByteArray) normalizedTypeName = QMetaObject::normalizedType(typeName);
            type = qMetaTypeStaticType(normalizedTypeName.constData(),
                                       normalizedTypeName.size());
            if (type == QMetaType::UnknownType) {
                type = qMetaTypeCustomType_unlocked(normalizedTypeName.constData(),
                                                    normalizedTypeName.size());
            }
        }
#endif
    }
    return type;
}

/*!
    Returns a handle to the type called \a typeName, or QMetaType::UnknownType if there is
    no such type.

    \sa isRegistered(), typeName(), Type
*/
int QMetaType::type(const char *typeName)
{
    return qMetaTypeTypeImpl</*tryNormalizedType=*/true>(typeName);
}

/*!
    \a internal

    Similar to QMetaType::type(); the only difference is that this function
    doesn't attempt to normalize the type name (i.e., the lookup will fail
    for type names in non-normalized form).
*/
int qMetaTypeTypeInternal(const char *typeName)
{
    return qMetaTypeTypeImpl</*tryNormalizedType=*/false>(typeName);
}

#ifndef QT_NO_DATASTREAM
/*!
    Writes the object pointed to by \a data with the ID \a type to
    the given \a stream. Returns \c true if the object is saved
    successfully; otherwise returns \c false.

    The type must have been registered with qRegisterMetaType() and
    qRegisterMetaTypeStreamOperators() beforehand.

    Normally, you should not need to call this function directly.
    Instead, use QVariant's \c operator<<(), which relies on save()
    to stream custom types.

    \sa load(), qRegisterMetaTypeStreamOperators()
*/
bool QMetaType::save(QDataStream &stream, int type, const void *data)
{
    if (!data || !isRegistered(type))
        return false;

    switch(type) {
    case QMetaType::UnknownType:
    case QMetaType::Void:
    case QMetaType::VoidStar:
    case QMetaType::QObjectStar:
    case QMetaType::QModelIndex:
    case QMetaType::QJsonValue:
    case QMetaType::QJsonObject:
    case QMetaType::QJsonArray:
    case QMetaType::QJsonDocument:
        return false;
    case QMetaType::Long:
        stream << qlonglong(*static_cast<const long *>(data));
        break;
    case QMetaType::Int:
        stream << *static_cast<const int *>(data);
        break;
    case QMetaType::Short:
        stream << *static_cast<const short *>(data);
        break;
    case QMetaType::Char:
        // force a char to be signed
        stream << *static_cast<const signed char *>(data);
        break;
    case QMetaType::ULong:
        stream << qulonglong(*static_cast<const ulong *>(data));
        break;
    case QMetaType::UInt:
        stream << *static_cast<const uint *>(data);
        break;
    case QMetaType::LongLong:
        stream << *static_cast<const qlonglong *>(data);
        break;
    case QMetaType::ULongLong:
        stream << *static_cast<const qulonglong *>(data);
        break;
    case QMetaType::UShort:
        stream << *static_cast<const ushort *>(data);
        break;
    case QMetaType::SChar:
        stream << *static_cast<const signed char *>(data);
        break;
    case QMetaType::UChar:
        stream << *static_cast<const uchar *>(data);
        break;
    case QMetaType::Bool:
        stream << qint8(*static_cast<const bool *>(data));
        break;
    case QMetaType::Float:
        stream << *static_cast<const float *>(data);
        break;
    case QMetaType::Double:
        stream << *static_cast<const double *>(data);
        break;
    case QMetaType::QChar:
        stream << *static_cast<const NS(QChar) *>(data);
        break;
#ifndef QT_BOOTSTRAPPED
    case QMetaType::QVariantMap:
        stream << *static_cast<const NS(QVariantMap)*>(data);
        break;
    case QMetaType::QVariantHash:
        stream << *static_cast<const NS(QVariantHash)*>(data);
        break;
    case QMetaType::QVariantList:
        stream << *static_cast<const NS(QVariantList)*>(data);
        break;
    case QMetaType::QVariant:
        stream << *static_cast<const NS(QVariant)*>(data);
        break;
#endif
    case QMetaType::QByteArray:
        stream << *static_cast<const NS(QByteArray)*>(data);
        break;
    case QMetaType::QString:
        stream << *static_cast<const NS(QString)*>(data);
        break;
    case QMetaType::QStringList:
        stream << *static_cast<const NS(QStringList)*>(data);
        break;
#ifndef QT_BOOTSTRAPPED
    case QMetaType::QBitArray:
        stream << *static_cast<const NS(QBitArray)*>(data);
        break;
#endif
    case QMetaType::QDate:
        stream << *static_cast<const NS(QDate)*>(data);
        break;
    case QMetaType::QTime:
        stream << *static_cast<const NS(QTime)*>(data);
        break;
    case QMetaType::QDateTime:
        stream << *static_cast<const NS(QDateTime)*>(data);
        break;
#ifndef QT_BOOTSTRAPPED
    case QMetaType::QUrl:
        stream << *static_cast<const NS(QUrl)*>(data);
        break;
#endif
    case QMetaType::QLocale:
        stream << *static_cast<const NS(QLocale)*>(data);
        break;
#ifndef QT_NO_GEOM_VARIANT
    case QMetaType::QRect:
        stream << *static_cast<const NS(QRect)*>(data);
        break;
    case QMetaType::QRectF:
        stream << *static_cast<const NS(QRectF)*>(data);
        break;
    case QMetaType::QSize:
        stream << *static_cast<const NS(QSize)*>(data);
        break;
    case QMetaType::QSizeF:
        stream << *static_cast<const NS(QSizeF)*>(data);
        break;
    case QMetaType::QLine:
        stream << *static_cast<const NS(QLine)*>(data);
        break;
    case QMetaType::QLineF:
        stream << *static_cast<const NS(QLineF)*>(data);
        break;
    case QMetaType::QPoint:
        stream << *static_cast<const NS(QPoint)*>(data);
        break;
    case QMetaType::QPointF:
        stream << *static_cast<const NS(QPointF)*>(data);
        break;
#endif
#ifndef QT_NO_REGEXP
    case QMetaType::QRegExp:
        stream << *static_cast<const NS(QRegExp)*>(data);
        break;
#endif
#ifndef QT_BOOTSTRAPPED
#ifndef QT_NO_REGULAREXPRESSION
    case QMetaType::QRegularExpression:
        stream << *static_cast<const NS(QRegularExpression)*>(data);
        break;
#endif // QT_NO_REGULAREXPRESSION
    case QMetaType::QEasingCurve:
        stream << *static_cast<const NS(QEasingCurve)*>(data);
        break;
#endif // QT_BOOTSTRAPPED
    case QMetaType::QFont:
    case QMetaType::QPixmap:
    case QMetaType::QBrush:
    case QMetaType::QColor:
    case QMetaType::QPalette:
    case QMetaType::QImage:
    case QMetaType::QPolygon:
    case QMetaType::QPolygonF:
    case QMetaType::QRegion:
    case QMetaType::QBitmap:
    case QMetaType::QCursor:
    case QMetaType::QKeySequence:
    case QMetaType::QPen:
    case QMetaType::QTextLength:
    case QMetaType::QTextFormat:
    case QMetaType::QMatrix:
    case QMetaType::QTransform:
    case QMetaType::QMatrix4x4:
    case QMetaType::QVector2D:
    case QMetaType::QVector3D:
    case QMetaType::QVector4D:
    case QMetaType::QQuaternion:
    case QMetaType::QIcon:
        if (!qMetaTypeGuiHelper)
            return false;
        qMetaTypeGuiHelper[type - FirstGuiType].saveOp(stream, data);
        break;
    case QMetaType::QSizePolicy:
        if (!qMetaTypeWidgetsHelper)
            return false;
        qMetaTypeWidgetsHelper[type - FirstWidgetsType].saveOp(stream, data);
        break;
    case QMetaType::QUuid:
        stream << *static_cast<const NS(QUuid)*>(data);
        break;
    default: {
        const QVector<QCustomTypeInfo> * const ct = customTypes();
        if (!ct)
            return false;

        SaveOperator saveOp = 0;
        {
            QReadLocker locker(customTypesLock());
            saveOp = ct->at(type - User).saveOp;
        }

        if (!saveOp)
            return false;
        saveOp(stream, data);
        break; }
    }

    return true;
}

/*!
    Reads the object of the specified \a type from the given \a
    stream into \a data. Returns \c true if the object is loaded
    successfully; otherwise returns \c false.

    The type must have been registered with qRegisterMetaType() and
    qRegisterMetaTypeStreamOperators() beforehand.

    Normally, you should not need to call this function directly.
    Instead, use QVariant's \c operator>>(), which relies on load()
    to stream custom types.

    \sa save(), qRegisterMetaTypeStreamOperators()
*/
bool QMetaType::load(QDataStream &stream, int type, void *data)
{
    if (!data || !isRegistered(type))
        return false;

    switch(type) {
    case QMetaType::UnknownType:
    case QMetaType::Void:
    case QMetaType::VoidStar:
    case QMetaType::QObjectStar:
    case QMetaType::QModelIndex:
    case QMetaType::QJsonValue:
    case QMetaType::QJsonObject:
    case QMetaType::QJsonArray:
    case QMetaType::QJsonDocument:
        return false;
    case QMetaType::Long: {
        qlonglong l;
        stream >> l;
        *static_cast<long *>(data) = long(l);
        break; }
    case QMetaType::Int:
        stream >> *static_cast<int *>(data);
        break;
    case QMetaType::Short:
        stream >> *static_cast<short *>(data);
        break;
    case QMetaType::Char:
        // force a char to be signed
        stream >> *static_cast<signed char *>(data);
        break;
    case QMetaType::ULong: {
        qulonglong ul;
        stream >> ul;
        *static_cast<ulong *>(data) = ulong(ul);
        break; }
    case QMetaType::UInt:
        stream >> *static_cast<uint *>(data);
        break;
    case QMetaType::LongLong:
        stream >> *static_cast<qlonglong *>(data);
        break;
    case QMetaType::ULongLong:
        stream >> *static_cast<qulonglong *>(data);
        break;
    case QMetaType::UShort:
        stream >> *static_cast<ushort *>(data);
        break;
    case QMetaType::SChar:
        stream >> *static_cast<signed char *>(data);
        break;
    case QMetaType::UChar:
        stream >> *static_cast<uchar *>(data);
        break;
    case QMetaType::Bool: {
        qint8 b;
        stream >> b;
        *static_cast<bool *>(data) = b;
        break; }
    case QMetaType::Float:
        stream >> *static_cast<float *>(data);
        break;
    case QMetaType::Double:
        stream >> *static_cast<double *>(data);
        break;
    case QMetaType::QChar:
        stream >> *static_cast< NS(QChar)*>(data);
        break;
#ifndef QT_BOOTSTRAPPED
    case QMetaType::QVariantMap:
        stream >> *static_cast< NS(QVariantMap)*>(data);
        break;
    case QMetaType::QVariantHash:
        stream >> *static_cast< NS(QVariantHash)*>(data);
        break;
    case QMetaType::QVariantList:
        stream >> *static_cast< NS(QVariantList)*>(data);
        break;
    case QMetaType::QVariant:
        stream >> *static_cast< NS(QVariant)*>(data);
        break;
#endif
    case QMetaType::QByteArray:
        stream >> *static_cast< NS(QByteArray)*>(data);
        break;
    case QMetaType::QString:
        stream >> *static_cast< NS(QString)*>(data);
        break;
    case QMetaType::QStringList:
        stream >> *static_cast< NS(QStringList)*>(data);
        break;
#ifndef QT_BOOTSTRAPPED
    case QMetaType::QBitArray:
        stream >> *static_cast< NS(QBitArray)*>(data);
        break;
#endif
    case QMetaType::QDate:
        stream >> *static_cast< NS(QDate)*>(data);
        break;
    case QMetaType::QTime:
        stream >> *static_cast< NS(QTime)*>(data);
        break;
    case QMetaType::QDateTime:
        stream >> *static_cast< NS(QDateTime)*>(data);
        break;
#ifndef QT_BOOTSTRAPPED
    case QMetaType::QUrl:
        stream >> *static_cast< NS(QUrl)*>(data);
        break;
#endif
    case QMetaType::QLocale:
        stream >> *static_cast< NS(QLocale)*>(data);
        break;
#ifndef QT_NO_GEOM_VARIANT
    case QMetaType::QRect:
        stream >> *static_cast< NS(QRect)*>(data);
        break;
    case QMetaType::QRectF:
        stream >> *static_cast< NS(QRectF)*>(data);
        break;
    case QMetaType::QSize:
        stream >> *static_cast< NS(QSize)*>(data);
        break;
    case QMetaType::QSizeF:
        stream >> *static_cast< NS(QSizeF)*>(data);
        break;
    case QMetaType::QLine:
        stream >> *static_cast< NS(QLine)*>(data);
        break;
    case QMetaType::QLineF:
        stream >> *static_cast< NS(QLineF)*>(data);
        break;
    case QMetaType::QPoint:
        stream >> *static_cast< NS(QPoint)*>(data);
        break;
    case QMetaType::QPointF:
        stream >> *static_cast< NS(QPointF)*>(data);
        break;
#endif
#ifndef QT_NO_REGEXP
    case QMetaType::QRegExp:
        stream >> *static_cast< NS(QRegExp)*>(data);
        break;
#endif
#ifndef QT_BOOTSTRAPPED
#ifndef QT_NO_REGULAREXPRESSION
    case QMetaType::QRegularExpression:
        stream >> *static_cast< NS(QRegularExpression)*>(data);
        break;
#endif // QT_NO_REGULAREXPRESSION
    case QMetaType::QEasingCurve:
        stream >> *static_cast< NS(QEasingCurve)*>(data);
        break;
#endif // QT_BOOTSTRAPPED
    case QMetaType::QFont:
    case QMetaType::QPixmap:
    case QMetaType::QBrush:
    case QMetaType::QColor:
    case QMetaType::QPalette:
    case QMetaType::QImage:
    case QMetaType::QPolygon:
    case QMetaType::QPolygonF:
    case QMetaType::QRegion:
    case QMetaType::QBitmap:
    case QMetaType::QCursor:
    case QMetaType::QKeySequence:
    case QMetaType::QPen:
    case QMetaType::QTextLength:
    case QMetaType::QTextFormat:
    case QMetaType::QMatrix:
    case QMetaType::QTransform:
    case QMetaType::QMatrix4x4:
    case QMetaType::QVector2D:
    case QMetaType::QVector3D:
    case QMetaType::QVector4D:
    case QMetaType::QQuaternion:
    case QMetaType::QIcon:
        if (!qMetaTypeGuiHelper)
            return false;
        qMetaTypeGuiHelper[type - FirstGuiType].loadOp(stream, data);
        break;
    case QMetaType::QSizePolicy:
        if (!qMetaTypeWidgetsHelper)
            return false;
        qMetaTypeWidgetsHelper[type - FirstWidgetsType].loadOp(stream, data);
        break;
    case QMetaType::QUuid:
        stream >> *static_cast< NS(QUuid)*>(data);
        break;
    default: {
        const QVector<QCustomTypeInfo> * const ct = customTypes();
        if (!ct)
            return false;

        LoadOperator loadOp = 0;
        {
            QReadLocker locker(customTypesLock());
            loadOp = ct->at(type - User).loadOp;
        }

        if (!loadOp)
            return false;
        loadOp(stream, data);
        break; }
    }
    return true;
}
#endif // QT_NO_DATASTREAM
namespace {
class TypeCreator {
    template<typename T, bool IsAcceptedType = DefinedTypesFilter::Acceptor<T>::IsAccepted>
    struct CreatorImpl {
        static void *Create(const int /* type */, const void *copy)
        {
            // Using QMetaTypeFunctionHelper<T>::Create adds function call cost, even if it is a template (gcc).
            // This "copy" check is moved out from the switcher by compiler (at least by gcc)
            return copy ? new T(*static_cast<const T*>(copy)) : new T();
        }
    };
    template<typename T>
    struct CreatorImpl<T, /* IsAcceptedType = */ false> {
        static void *Create(const int type, const void *copy)
        {
            if (QModulesPrivate::QTypeModuleInfo<T>::IsGui) {
                if (Q_LIKELY(qMetaTypeGuiHelper))
                    return qMetaTypeGuiHelper[type - QMetaType::FirstGuiType].creator(copy);
            }
            if (QModulesPrivate::QTypeModuleInfo<T>::IsWidget) {
                if (Q_LIKELY(qMetaTypeWidgetsHelper))
                    return qMetaTypeWidgetsHelper[type - QMetaType::FirstWidgetsType].creator(copy);
            }
            // This point can be reached only for known types that definition is not available, for example
            // in bootstrap mode. We have no other choice then ignore it.
            return 0;
        }
    };
public:
    TypeCreator(const int type)
        : m_type(type)
    {}

    template<typename T>
    void *delegate(const T *copy) { return CreatorImpl<T>::Create(m_type, copy); }
    void *delegate(const void*) { return 0; }
    void *delegate(const QMetaTypeSwitcher::UnknownType *) { return 0; }
    void *delegate(const QMetaTypeSwitcher::NotBuiltinType *copy)
    {
        QMetaType::Creator creator;
        const QVector<QCustomTypeInfo> * const ct = customTypes();
        {
            QReadLocker locker(customTypesLock());
            if (Q_UNLIKELY(m_type < QMetaType::User || !ct || ct->count() <= m_type - QMetaType::User))
                return 0;
            creator = ct->at(m_type - QMetaType::User).creator;
        }
        Q_ASSERT_X(creator, "void *QMetaType::create(int type, const void *copy)", "The type was not properly registered");
        return creator(copy);
    }
private:
    const int m_type;
};
} // namespace

/*!
    Returns a copy of \a copy, assuming it is of type \a type. If \a
    copy is zero, creates a default constructed instance.

    \sa destroy(), isRegistered(), Type
*/
void *QMetaType::create(int type, const void *copy)
{
    TypeCreator typeCreator(type);
    return QMetaTypeSwitcher::switcher<void*>(typeCreator, type, copy);
}

namespace {
class TypeDestroyer {
    template<typename T, bool IsAcceptedType = DefinedTypesFilter::Acceptor<T>::IsAccepted>
    struct DestroyerImpl {
        static void Destroy(const int /* type */, void *where) { QtMetaTypePrivate::QMetaTypeFunctionHelper<T>::Delete(where); }
    };
    template<typename T>
    struct DestroyerImpl<T, /* IsAcceptedType = */ false> {
        static void Destroy(const int type, void *where)
        {
            if (QModulesPrivate::QTypeModuleInfo<T>::IsGui) {
                if (Q_LIKELY(qMetaTypeGuiHelper))
                    qMetaTypeGuiHelper[type - QMetaType::FirstGuiType].deleter(where);
                return;
            }
            if (QModulesPrivate::QTypeModuleInfo<T>::IsWidget) {
                if (Q_LIKELY(qMetaTypeWidgetsHelper))
                    qMetaTypeWidgetsHelper[type - QMetaType::FirstWidgetsType].deleter(where);
                return;
            }
            // This point can be reached only for known types that definition is not available, for example
            // in bootstrap mode. We have no other choice then ignore it.
        }
    };
public:
    TypeDestroyer(const int type)
        : m_type(type)
    {}

    template<typename T>
    void delegate(const T *where) { DestroyerImpl<T>::Destroy(m_type, const_cast<T*>(where)); }
    void delegate(const void *) {}
    void delegate(const QMetaTypeSwitcher::UnknownType*) {}
    void delegate(const QMetaTypeSwitcher::NotBuiltinType *where) { customTypeDestroyer(m_type, (void*)where); }

private:
    static void customTypeDestroyer(const int type, void *where)
    {
        QMetaType::Destructor deleter;
        const QVector<QCustomTypeInfo> * const ct = customTypes();
        {
            QReadLocker locker(customTypesLock());
            if (Q_UNLIKELY(type < QMetaType::User || !ct || ct->count() <= type - QMetaType::User))
                return;
            deleter = ct->at(type - QMetaType::User).deleter;
        }
        Q_ASSERT_X(deleter, "void QMetaType::destroy(int type, void *data)", "The type was not properly registered");
        deleter(where);
    }

    const int m_type;
};
} // namespace


/*!
    Destroys the \a data, assuming it is of the \a type given.

    \sa create(), isRegistered(), Type
*/
void QMetaType::destroy(int type, void *data)
{
    TypeDestroyer deleter(type);
    QMetaTypeSwitcher::switcher<void>(deleter, type, data);
}

namespace {
class TypeConstructor {
    template<typename T, bool IsAcceptedType = DefinedTypesFilter::Acceptor<T>::IsAccepted>
    struct ConstructorImpl {
        static void *Construct(const int /*type*/, void *where, const void *copy) { return QtMetaTypePrivate::QMetaTypeFunctionHelper<T>::Construct(where, copy); }
    };
    template<typename T>
    struct ConstructorImpl<T, /* IsAcceptedType = */ false> {
        static void *Construct(const int type, void *where, const void *copy)
        {
            if (QModulesPrivate::QTypeModuleInfo<T>::IsGui)
                return Q_LIKELY(qMetaTypeGuiHelper) ? qMetaTypeGuiHelper[type - QMetaType::FirstGuiType].constructor(where, copy) : 0;

            if (QModulesPrivate::QTypeModuleInfo<T>::IsWidget)
                return Q_LIKELY(qMetaTypeWidgetsHelper) ? qMetaTypeWidgetsHelper[type - QMetaType::FirstWidgetsType].constructor(where, copy) : 0;

            // This point can be reached only for known types that definition is not available, for example
            // in bootstrap mode. We have no other choice then ignore it.
            return 0;
        }
    };
public:
    TypeConstructor(const int type, void *where)
        : m_type(type)
        , m_where(where)
    {}

    template<typename T>
    void *delegate(const T *copy) { return ConstructorImpl<T>::Construct(m_type, m_where, copy); }
    void *delegate(const void *) { return m_where; }
    void *delegate(const QMetaTypeSwitcher::UnknownType*) { return m_where; }
    void *delegate(const QMetaTypeSwitcher::NotBuiltinType *copy) { return customTypeConstructor(m_type, m_where, copy); }

private:
    static void *customTypeConstructor(const int type, void *where, const void *copy)
    {
        QMetaType::Constructor ctor;
        const QVector<QCustomTypeInfo> * const ct = customTypes();
        {
            QReadLocker locker(customTypesLock());
            if (Q_UNLIKELY(type < QMetaType::User || !ct || ct->count() <= type - QMetaType::User))
                return 0;
            ctor = ct->at(type - QMetaType::User).constructor;
        }
        Q_ASSERT_X(ctor, "void *QMetaType::construct(int type, void *where, const void *copy)", "The type was not properly registered");
        return ctor(where, copy);
    }

    const int m_type;
    void *m_where;
};
} // namespace

/*!
    \since 5.0

    Constructs a value of the given \a type in the existing memory
    addressed by \a where, that is a copy of \a copy, and returns
    \a where. If \a copy is zero, the value is default constructed.

    This is a low-level function for explicitly managing the memory
    used to store the type. Consider calling create() if you don't
    need this level of control (that is, use "new" rather than
    "placement new").

    You must ensure that \a where points to a location that can store
    a value of type \a type, and that \a where is suitably aligned.
    The type's size can be queried by calling sizeOf().

    The rule of thumb for alignment is that a type is aligned to its
    natural boundary, which is the smallest power of 2 that is bigger
    than the type, unless that alignment is larger than the maximum
    useful alignment for the platform. For practical purposes,
    alignment larger than 2 * sizeof(void*) is only necessary for
    special hardware instructions (e.g., aligned SSE loads and stores
    on x86).

    \sa destruct(), sizeOf()
*/
void *QMetaType::construct(int type, void *where, const void *copy)
{
    if (!where)
        return 0;
    TypeConstructor constructor(type, where);
    return QMetaTypeSwitcher::switcher<void*>(constructor, type, copy);
}


namespace {
class TypeDestructor {
    template<typename T, bool IsAcceptedType = DefinedTypesFilter::Acceptor<T>::IsAccepted>
    struct DestructorImpl {
        static void Destruct(const int /* type */, void *where) { QtMetaTypePrivate::QMetaTypeFunctionHelper<T>::Destruct(where); }
    };
    template<typename T>
    struct DestructorImpl<T, /* IsAcceptedType = */ false> {
        static void Destruct(const int type, void *where)
        {
            if (QModulesPrivate::QTypeModuleInfo<T>::IsGui) {
                if (Q_LIKELY(qMetaTypeGuiHelper))
                    qMetaTypeGuiHelper[type - QMetaType::FirstGuiType].destructor(where);
                return;
            }
            if (QModulesPrivate::QTypeModuleInfo<T>::IsWidget) {
                if (Q_LIKELY(qMetaTypeWidgetsHelper))
                    qMetaTypeWidgetsHelper[type - QMetaType::FirstWidgetsType].destructor(where);
                return;
            }
            // This point can be reached only for known types that definition is not available, for example
            // in bootstrap mode. We have no other choice then ignore it.
        }
    };
public:
    TypeDestructor(const int type)
        : m_type(type)
    {}

    template<typename T>
    void delegate(const T *where) { DestructorImpl<T>::Destruct(m_type, const_cast<T*>(where)); }
    void delegate(const void *) {}
    void delegate(const QMetaTypeSwitcher::UnknownType*) {}
    void delegate(const QMetaTypeSwitcher::NotBuiltinType *where) { customTypeDestructor(m_type, (void*)where); }

private:
    static void customTypeDestructor(const int type, void *where)
    {
        QMetaType::Destructor dtor;
        const QVector<QCustomTypeInfo> * const ct = customTypes();
        {
            QReadLocker locker(customTypesLock());
            if (Q_UNLIKELY(type < QMetaType::User || !ct || ct->count() <= type - QMetaType::User))
                return;
            dtor = ct->at(type - QMetaType::User).destructor;
        }
        Q_ASSERT_X(dtor, "void QMetaType::destruct(int type, void *where)", "The type was not properly registered");
        dtor(where);
    }

    const int m_type;
};
} // namespace

/*!
    \since 5.0

    Destructs the value of the given \a type, located at \a where.

    Unlike destroy(), this function only invokes the type's
    destructor, it doesn't invoke the delete operator.

    \sa construct()
*/
void QMetaType::destruct(int type, void *where)
{
    if (!where)
        return;
    TypeDestructor destructor(type);
    QMetaTypeSwitcher::switcher<void>(destructor, type, where);
}


namespace {
class SizeOf {
    template<typename T, bool IsAcceptedType = DefinedTypesFilter::Acceptor<T>::IsAccepted>
    struct SizeOfImpl {
        static int Size(const int) { return QTypeInfo<T>::sizeOf; }
    };
    template<typename T>
    struct SizeOfImpl<T, /* IsAcceptedType = */ false> {
        static int Size(const int type)
        {
            if (QModulesPrivate::QTypeModuleInfo<T>::IsGui)
                return Q_LIKELY(qMetaTypeGuiHelper) ? qMetaTypeGuiHelper[type - QMetaType::FirstGuiType].size : 0;

            if (QModulesPrivate::QTypeModuleInfo<T>::IsWidget)
                return Q_LIKELY(qMetaTypeWidgetsHelper) ? qMetaTypeWidgetsHelper[type - QMetaType::FirstWidgetsType].size : 0;

            // This point can be reached only for known types that definition is not available, for example
            // in bootstrap mode. We have no other choice then ignore it.
            return 0;
        }
    };

public:
    SizeOf(int type)
        : m_type(type)
    {}

    template<typename T>
    int delegate(const T*) { return SizeOfImpl<T>::Size(m_type); }
    int delegate(const QMetaTypeSwitcher::UnknownType*) { return 0; }
    int delegate(const QMetaTypeSwitcher::NotBuiltinType*) { return customTypeSizeOf(m_type); }
private:
    static int customTypeSizeOf(const int type)
    {
        const QVector<QCustomTypeInfo> * const ct = customTypes();
        QReadLocker locker(customTypesLock());
        if (Q_UNLIKELY(type < QMetaType::User || !ct || ct->count() <= type - QMetaType::User))
            return 0;
        return ct->at(type - QMetaType::User).size;
    }

    const int m_type;
};
} // namespace

/*!
    \since 5.0

    Returns the size of the given \a type in bytes (i.e. sizeof(T),
    where T is the actual type identified by the \a type argument).

    This function is typically used together with construct()
    to perform low-level management of the memory used by a type.

    \sa construct()
*/
int QMetaType::sizeOf(int type)
{
    SizeOf sizeOf(type);
    return QMetaTypeSwitcher::switcher<int>(sizeOf, type, 0);
}

namespace {
class Flags
{
    template<typename T, bool IsAcceptedType = DefinedTypesFilter::Acceptor<T>::IsAccepted>
    struct FlagsImpl
    {
        static quint32 Flags(const int /* type */)
        {
            return QtPrivate::QMetaTypeTypeFlags<T>::Flags;
        }
    };
    template<typename T>
    struct FlagsImpl<T, /* IsAcceptedType = */ false>
    {
        static quint32 Flags(const int type)
        {
            if (QModulesPrivate::QTypeModuleInfo<T>::IsGui)
                return Q_LIKELY(qMetaTypeGuiHelper) ? qMetaTypeGuiHelper[type - QMetaType::FirstGuiType].flags : 0;

            if (QModulesPrivate::QTypeModuleInfo<T>::IsWidget)
                return Q_LIKELY(qMetaTypeWidgetsHelper) ? qMetaTypeWidgetsHelper[type - QMetaType::FirstWidgetsType].flags : 0;

            // This point can be reached only for known types that definition is not available, for example
            // in bootstrap mode. We have no other choice then ignore it.
            return 0;
        }
    };
public:
    Flags(const int type)
        : m_type(type)
    {}
    template<typename T>
    quint32 delegate(const T*) { return FlagsImpl<T>::Flags(m_type); }
    quint32 delegate(const void*) { return 0; }
    quint32 delegate(const QMetaTypeSwitcher::UnknownType*) { return 0; }
    quint32 delegate(const QMetaTypeSwitcher::NotBuiltinType*) { return customTypeFlags(m_type); }
private:
    const int m_type;
    static quint32 customTypeFlags(const int type)
    {
        const QVector<QCustomTypeInfo> * const ct = customTypes();
        if (Q_UNLIKELY(!ct || type < QMetaType::User))
            return 0;
        QReadLocker locker(customTypesLock());
        if (Q_UNLIKELY(ct->count() <= type - QMetaType::User))
            return 0;
        return ct->at(type - QMetaType::User).flags;
    }
};
}  // namespace

/*!
    \since 5.0

    Returns flags of the given \a type.

    \sa QMetaType::TypeFlags
*/
QMetaType::TypeFlags QMetaType::typeFlags(int type)
{
    Flags flags(type);
    return static_cast<QMetaType::TypeFlags>(QMetaTypeSwitcher::switcher<quint32>(flags, type, 0));
}

#ifndef QT_BOOTSTRAPPED
namespace {
class MetaObject
{
public:
    MetaObject(const int type)
        : m_type(type)
    {}
    template<typename T>
    const QMetaObject *delegate(const T*) { return QtPrivate::MetaObjectForType<T>::value(); }
    const QMetaObject *delegate(const void*) { return 0; }
    const QMetaObject *delegate(const QMetaTypeSwitcher::UnknownType*) { return 0; }
    const QMetaObject *delegate(const QMetaTypeSwitcher::NotBuiltinType*) { return customMetaObject(m_type); }
private:
    const int m_type;
    static const QMetaObject *customMetaObject(const int type)
    {
        const QVector<QCustomTypeInfo> * const ct = customTypes();
        if (Q_UNLIKELY(!ct || type < QMetaType::User))
            return 0;
        QReadLocker locker(customTypesLock());
        if (Q_UNLIKELY(ct->count() <= type - QMetaType::User))
            return 0;
        return ct->at(type - QMetaType::User).metaObject;
    }
};
}  // namespace
#endif

/*!
    \since 5.0

    Returns QMetaObject of a given \a type, if the \a type is a pointer to type derived from QObject.
*/
const QMetaObject *QMetaType::metaObjectForType(int type)
{
#ifndef QT_BOOTSTRAPPED
    MetaObject mo(type);
    return QMetaTypeSwitcher::switcher<const QMetaObject*>(mo, type, 0);
#else
    Q_UNUSED(type);
    return 0;
#endif
}

/*!
    \fn int qRegisterMetaType(const char *typeName)
    \relates QMetaType
    \threadsafe

    Registers the type name \a typeName for the type \c{T}. Returns
    the internal ID used by QMetaType. Any class or struct that has a
    public default constructor, a public copy constructor and a public
    destructor can be registered.

    This function requires that \c{T} is a fully defined type at the point
    where the function is called. For pointer types, it also requires that the
    pointed to type is fully defined. Use Q_DECLARE_OPAQUE_POINTER() to be able
    to register pointers to forward declared types.

    After a type has been registered, you can create and destroy
    objects of that type dynamically at run-time.

    This example registers the class \c{MyClass}:

    \snippet code/src_corelib_kernel_qmetatype.cpp 4

    This function is useful to register typedefs so they can be used
    by QMetaProperty, or in QueuedConnections

    \snippet code/src_corelib_kernel_qmetatype.cpp 9

    \warning This function is useful only for registering an alias (typedef)
    for every other use case Q_DECLARE_METATYPE and qMetaTypeId() should be used instead.

    \sa qRegisterMetaTypeStreamOperators(), QMetaType::isRegistered(),
        Q_DECLARE_METATYPE()
*/

/*!
    \fn void qRegisterMetaTypeStreamOperators(const char *typeName)
    \relates QMetaType
    \threadsafe

    Registers the stream operators for the type \c{T} called \a
    typeName.

    Afterward, the type can be streamed using QMetaType::load() and
    QMetaType::save(). These functions are used when streaming a
    QVariant.

    \snippet code/src_corelib_kernel_qmetatype.cpp 5

    The stream operators should have the following signatures:

    \snippet code/src_corelib_kernel_qmetatype.cpp 6

    \sa qRegisterMetaType(), QMetaType::isRegistered(), Q_DECLARE_METATYPE()
*/

/*! \typedef QMetaType::Deleter
    \internal
*/
/*! \typedef QMetaType::Creator
    \internal
*/
/*! \typedef QMetaType::SaveOperator
    \internal
*/
/*! \typedef QMetaType::LoadOperator
    \internal
*/
/*! \typedef QMetaType::Destructor
    \internal
*/
/*! \typedef QMetaType::Constructor
    \internal
*/

/*!
    \fn int qRegisterMetaType()
    \relates QMetaType
    \threadsafe
    \since 4.2

    Call this function to register the type \c T. \c T must be declared with
    Q_DECLARE_METATYPE(). Returns the meta type Id.

    Example:

    \snippet code/src_corelib_kernel_qmetatype.cpp 7

    This function requires that \c{T} is a fully defined type at the point
    where the function is called. For pointer types, it also requires that the
    pointed to type is fully defined. Use Q_DECLARE_OPAQUE_POINTER() to be able
    to register pointers to forward declared types.

    After a type has been registered, you can create and destroy
    objects of that type dynamically at run-time.

    To use the type \c T in QVariant, using Q_DECLARE_METATYPE() is
    sufficient. To use the type \c T in queued signal and slot connections,
    \c{qRegisterMetaType<T>()} must be called before the first connection
    is established.

    Also, to use type \c T with the QObject::property() API,
    \c{qRegisterMetaType<T>()} must be called before it is used, typically
    in the constructor of the class that uses \c T, or in the \c{main()}
    function.

    \sa Q_DECLARE_METATYPE()
 */

/*!
    \fn int qMetaTypeId()
    \relates QMetaType
    \threadsafe
    \since 4.1

    Returns the meta type id of type \c T at compile time. If the
    type was not declared with Q_DECLARE_METATYPE(), compilation will
    fail.

    Typical usage:

    \snippet code/src_corelib_kernel_qmetatype.cpp 8

    QMetaType::type() returns the same ID as qMetaTypeId(), but does
    a lookup at runtime based on the name of the type.
    QMetaType::type() is a bit slower, but compilation succeeds if a
    type is not registered.

    \sa Q_DECLARE_METATYPE(), QMetaType::type()
*/

namespace {
class TypeInfo {
    template<typename T, bool IsAcceptedType = DefinedTypesFilter::Acceptor<T>::IsAccepted>
    struct TypeInfoImpl
    {
        TypeInfoImpl(const uint /* type */, QMetaTypeInterface &info)
        {
            QMetaTypeInterface tmp = QT_METATYPE_INTERFACE_INIT_NO_DATASTREAM(T);
            info = tmp;
        }
    };

    template<typename T>
    struct TypeInfoImpl<T, /* IsAcceptedType = */ false>
    {
        TypeInfoImpl(const uint type, QMetaTypeInterface &info)
        {
            if (QModulesPrivate::QTypeModuleInfo<T>::IsGui) {
                if (Q_LIKELY(qMetaTypeGuiHelper))
                    info = qMetaTypeGuiHelper[type - QMetaType::FirstGuiType];
                return;
            }
            if (QModulesPrivate::QTypeModuleInfo<T>::IsWidget) {
                if (Q_LIKELY(qMetaTypeWidgetsHelper))
                    info = qMetaTypeWidgetsHelper[type - QMetaType::FirstWidgetsType];
                return;
            }
        }
    };
public:
    QMetaTypeInterface info;
    TypeInfo(const uint type)
        : m_type(type)
    {
        QMetaTypeInterface tmp = QT_METATYPE_INTERFACE_INIT_EMPTY();
        info = tmp;
    }
    template<typename T>
    void delegate(const T*) { TypeInfoImpl<T>(m_type, info); }
    void delegate(const QMetaTypeSwitcher::UnknownType*) {}
    void delegate(const QMetaTypeSwitcher::NotBuiltinType*) { customTypeInfo(m_type); }
private:
    void customTypeInfo(const uint type)
    {
        const QVector<QCustomTypeInfo> * const ct = customTypes();
        if (Q_UNLIKELY(!ct))
            return;
        QReadLocker locker(customTypesLock());
        if (Q_LIKELY(uint(ct->count()) > type - QMetaType::User))
            info = ct->at(type - QMetaType::User);
    }

    const uint m_type;
};
} // namespace

/*!
    \fn QMetaType QMetaType::typeInfo(const int type)
    \internal
*/
QMetaType QMetaType::typeInfo(const int type)
{
    TypeInfo typeInfo(type);
    QMetaTypeSwitcher::switcher<void>(typeInfo, type, 0);
    return typeInfo.info.creator ? QMetaType(QMetaType::NoExtensionFlags
                                 , static_cast<const QMetaTypeInterface *>(0) // typeInfo::info is a temporary variable, we can't return address of it.
                                 , typeInfo.info.creator
                                 , typeInfo.info.deleter
                                 , typeInfo.info.saveOp
                                 , typeInfo.info.loadOp
                                 , typeInfo.info.constructor
                                 , typeInfo.info.destructor
                                 , typeInfo.info.size
                                 , typeInfo.info.flags
                                 , type
                                 , typeInfo.info.metaObject)
                : QMetaType(UnknownType);
}

/*!
     \fn QMetaType::QMetaType(const int typeId)
     \since 5.0

     Constructs a QMetaType object that contains all information about type \a typeId.
*/
QMetaType::QMetaType(const int typeId)
    : m_typeId(typeId)
{
    if (Q_UNLIKELY(typeId == UnknownType)) {
        // Constructs invalid QMetaType instance.
        m_extensionFlags = 0xffffffff;
        Q_ASSERT(!isValid());
    } else {
        // TODO it can be better.
        *this = QMetaType::typeInfo(typeId);
        if (m_typeId == UnknownType)
            m_extensionFlags = 0xffffffff;
        else if (m_typeId == QMetaType::Void)
            m_extensionFlags = CreateEx | DestroyEx | ConstructEx | DestructEx;
    }
}

/*!
     \fn QMetaType::QMetaType(const QMetaType &other)
     \since 5.0

     Copy constructs a QMetaType object.
*/
QMetaType::QMetaType(const QMetaType &other)
    : m_creator(other.m_creator)
    , m_deleter(other.m_deleter)
    , m_saveOp(other.m_saveOp)
    , m_loadOp(other.m_loadOp)
    , m_constructor(other.m_constructor)
    , m_destructor(other.m_destructor)
    , m_extension(other.m_extension) // space reserved for future use
    , m_size(other.m_size)
    , m_typeFlags(other.m_typeFlags)
    , m_extensionFlags(other.m_extensionFlags)
    , m_typeId(other.m_typeId)
    , m_metaObject(other.m_metaObject)
{}

QMetaType &QMetaType::operator =(const QMetaType &other)
{
    m_creator = other.m_creator;
    m_deleter = other.m_deleter;
    m_saveOp = other.m_saveOp;
    m_loadOp = other.m_loadOp;
    m_constructor = other.m_constructor;
    m_destructor = other.m_destructor;
    m_size = other.m_size;
    m_typeFlags = other.m_typeFlags;
    m_extensionFlags = other.m_extensionFlags;
    m_extension = other.m_extension; // space reserved for future use
    m_typeId = other.m_typeId;
    m_metaObject = other.m_metaObject;
    return *this;
}

/*!
    \fn void QMetaType::ctor(const QMetaTypeInterface *info)
    \internal

    Method used for future binary compatible extensions.  The function may be
    called from within QMetaType's constructor to force a library call from
    inlined code.
*/
void QMetaType::ctor(const QMetaTypeInterface *info)
{
    // Special case for Void type, the type is valid but not constructible.
    // In future we may consider to remove this assert and extend this function to initialize
    // differently m_extensionFlags for different types. Currently it is not needed.
    Q_ASSERT(m_typeId == QMetaType::Void);
    Q_UNUSED(info);
    m_extensionFlags = CreateEx | DestroyEx | ConstructEx | DestructEx;
}

/*!
    \fn void QMetaType::dtor()
    \internal

    Method used for future binary compatible extensions.  The function may be
    called from within QMetaType's destructor to force a library call from
    inlined code.
*/
void QMetaType::dtor()
{}

/*!
    \fn void *QMetaType::createExtended(const void *copy) const
    \internal

    Method used for future binary compatible extensions. The function may be called
    during QMetaType::create to force library call from inlined code.
*/
void *QMetaType::createExtended(const void *copy) const
{
    Q_UNUSED(copy);
    return 0;
}

/*!
    \fn void QMetaType::destroyExtended(void *data) const
    \internal

    Method used for future binary compatible extensions. The function may be called
    during QMetaType::destroy to force library call from inlined code.
*/
void QMetaType::destroyExtended(void *data) const
{
    Q_UNUSED(data);
}

/*!
    \fn void *QMetaType::constructExtended(void *where, const void *copy) const
    \internal

    Method used for future binary compatible extensions. The function may be called
    during QMetaType::construct to force library call from inlined code.
*/
void *QMetaType::constructExtended(void *where, const void *copy) const
{
    Q_UNUSED(where);
    Q_UNUSED(copy);
    return 0;
}

/*!
    \fn void QMetaType::destructExtended(void *data) const
    \internal

    Method used for future binary compatible extensions. The function may be called
    during QMetaType::destruct to force library call from inlined code.
*/
void QMetaType::destructExtended(void *data) const
{
    Q_UNUSED(data);
}

/*!
    \fn uint QMetaType::sizeExtended() const
    \internal

    Method used for future binary compatible extensions. The function may be
    called from within QMetaType::size to force a library call from
    inlined code.
*/
uint QMetaType::sizeExtended() const
{
    return 0;
}

/*!
    \fn QMetaType::TypeFlags QMetaType::flagsExtended() const
    \internal

    Method used for future binary compatible extensions.  The function may be
    called from within QMetaType::flags to force a library call from
    inlined code.
*/
QMetaType::TypeFlags QMetaType::flagsExtended() const
{
    return 0;
}

/*!
    \brief QMetaType::metaObjectExtended
    \internal

    Method used for future binary compatible extensions. The function may be
    called from within QMetaType::metaObject to force a library call from
    inlined code.
*/
const QMetaObject *QMetaType::metaObjectExtended() const
{
    return 0;
}


namespace QtPrivate
{
const QMetaObject *metaObjectForQWidget()
{
    if (!qMetaTypeWidgetsHelper)
        return 0;
    return qMetaObjectWidgetsHelper;
}
}

QT_END_NAMESPACE
