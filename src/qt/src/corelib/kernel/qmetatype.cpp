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

#include "qmetatype.h"
#include "qobjectdefs.h"
#include "qdatetime.h"
#include "qbytearray.h"
#include "qreadwritelock.h"
#include "qstring.h"
#include "qstringlist.h"
#include "qvector.h"
#include "qlocale.h"
#include "qeasingcurve.h"

#ifdef QT_BOOTSTRAPPED
# ifndef QT_NO_GEOM_VARIANT
#  define QT_NO_GEOM_VARIANT
# endif
#else
#  include "qbitarray.h"
#  include "qurl.h"
#  include "qvariant.h"
#endif

#ifndef QT_NO_GEOM_VARIANT
# include "qsize.h"
# include "qpoint.h"
# include "qrect.h"
# include "qline.h"
#endif

QT_BEGIN_NAMESPACE

#define NS(x) QT_PREPEND_NAMESPACE(x)

/*!
    \macro Q_DECLARE_METATYPE(Type)
    \relates QMetaType

    This macro makes the type \a Type known to QMetaType as long as it
    provides a public default constructor, a public copy constructor and
    a public destructor.
    It is needed to use the type \a Type as a custom type in QVariant.

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

    \snippet doc/src/snippets/code/src_corelib_kernel_qmetatype.cpp 0

    If \c MyStruct is in a namespace, the Q_DECLARE_METATYPE() macro
    has to be outside the namespace:

    \snippet doc/src/snippets/code/src_corelib_kernel_qmetatype.cpp 1

    Since \c{MyStruct} is now known to QMetaType, it can be used in QVariant:

    \snippet doc/src/snippets/code/src_corelib_kernel_qmetatype.cpp 2

    \sa qRegisterMetaType()
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
    \value UChar \c{unsigned char}
    \value Float \c float
    \value QObjectStar QObject *
    \value QWidgetStar QWidget *
    \value QVariant QVariant

    \value QColorGroup QColorGroup
    \value QCursor QCursor
    \value QDate QDate
    \value QSize QSize
    \value QTime QTime
    \value QVariantList QVariantList
    \value QPolygon QPolygon
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

    \value User  Base value for user types

    \omitvalue FirstCoreExtType
    \omitvalue FirstGuiType
    \omitvalue LastCoreExtType
    \omitvalue LastCoreType
    \omitvalue LastGuiType
    \omitvalue QReal

    Additional types can be registered using Q_DECLARE_METATYPE().

    \sa type(), typeName()
*/

/*!
    \class QMetaType
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

    \snippet doc/src/snippets/code/src_corelib_kernel_qmetatype.cpp 3

    If we want the stream operators \c operator<<() and \c
    operator>>() to work on QVariant objects that store custom types,
    the custom type must provide \c operator<<() and \c operator>>()
    operators.

    \sa Q_DECLARE_METATYPE(), QVariant::setValue(), QVariant::value(), QVariant::fromValue()
*/

#define QT_ADD_STATIC_METATYPE(STR, TP) \
    { STR, sizeof(STR) - 1, TP }

/* Note: these MUST be in the order of the enums */
static const struct { const char * typeName; int typeNameLength; int type; } types[] = {

    /* All Core types */
    QT_ADD_STATIC_METATYPE("void", QMetaType::Void),
    QT_ADD_STATIC_METATYPE("bool", QMetaType::Bool),
    QT_ADD_STATIC_METATYPE("int", QMetaType::Int),
    QT_ADD_STATIC_METATYPE("uint", QMetaType::UInt),
    QT_ADD_STATIC_METATYPE("qlonglong", QMetaType::LongLong),
    QT_ADD_STATIC_METATYPE("qulonglong", QMetaType::ULongLong),
    QT_ADD_STATIC_METATYPE("double", QMetaType::Double),
    QT_ADD_STATIC_METATYPE("QChar", QMetaType::QChar),
    QT_ADD_STATIC_METATYPE("QVariantMap", QMetaType::QVariantMap),
    QT_ADD_STATIC_METATYPE("QVariantList", QMetaType::QVariantList),
    QT_ADD_STATIC_METATYPE("QString", QMetaType::QString),
    QT_ADD_STATIC_METATYPE("QStringList", QMetaType::QStringList),
    QT_ADD_STATIC_METATYPE("QByteArray", QMetaType::QByteArray),
    QT_ADD_STATIC_METATYPE("QBitArray", QMetaType::QBitArray),
    QT_ADD_STATIC_METATYPE("QDate", QMetaType::QDate),
    QT_ADD_STATIC_METATYPE("QTime", QMetaType::QTime),
    QT_ADD_STATIC_METATYPE("QDateTime", QMetaType::QDateTime),
    QT_ADD_STATIC_METATYPE("QUrl", QMetaType::QUrl),
    QT_ADD_STATIC_METATYPE("QLocale", QMetaType::QLocale),
    QT_ADD_STATIC_METATYPE("QRect", QMetaType::QRect),
    QT_ADD_STATIC_METATYPE("QRectF", QMetaType::QRectF),
    QT_ADD_STATIC_METATYPE("QSize", QMetaType::QSize),
    QT_ADD_STATIC_METATYPE("QSizeF", QMetaType::QSizeF),
    QT_ADD_STATIC_METATYPE("QLine", QMetaType::QLine),
    QT_ADD_STATIC_METATYPE("QLineF", QMetaType::QLineF),
    QT_ADD_STATIC_METATYPE("QPoint", QMetaType::QPoint),
    QT_ADD_STATIC_METATYPE("QPointF", QMetaType::QPointF),
    QT_ADD_STATIC_METATYPE("QRegExp", QMetaType::QRegExp),
    QT_ADD_STATIC_METATYPE("QVariantHash", QMetaType::QVariantHash),
    QT_ADD_STATIC_METATYPE("QEasingCurve", QMetaType::QEasingCurve),

    /* All GUI types */
    QT_ADD_STATIC_METATYPE("QColorGroup", 63),
    QT_ADD_STATIC_METATYPE("QFont", QMetaType::QFont),
    QT_ADD_STATIC_METATYPE("QPixmap", QMetaType::QPixmap),
    QT_ADD_STATIC_METATYPE("QBrush", QMetaType::QBrush),
    QT_ADD_STATIC_METATYPE("QColor", QMetaType::QColor),
    QT_ADD_STATIC_METATYPE("QPalette", QMetaType::QPalette),
    QT_ADD_STATIC_METATYPE("QIcon", QMetaType::QIcon),
    QT_ADD_STATIC_METATYPE("QImage", QMetaType::QImage),
    QT_ADD_STATIC_METATYPE("QPolygon", QMetaType::QPolygon),
    QT_ADD_STATIC_METATYPE("QRegion", QMetaType::QRegion),
    QT_ADD_STATIC_METATYPE("QBitmap", QMetaType::QBitmap),
    QT_ADD_STATIC_METATYPE("QCursor", QMetaType::QCursor),
    QT_ADD_STATIC_METATYPE("QSizePolicy", QMetaType::QSizePolicy),
    QT_ADD_STATIC_METATYPE("QKeySequence", QMetaType::QKeySequence),
    QT_ADD_STATIC_METATYPE("QPen", QMetaType::QPen),
    QT_ADD_STATIC_METATYPE("QTextLength", QMetaType::QTextLength),
    QT_ADD_STATIC_METATYPE("QTextFormat", QMetaType::QTextFormat),
    QT_ADD_STATIC_METATYPE("QMatrix", QMetaType::QMatrix),
    QT_ADD_STATIC_METATYPE("QTransform", QMetaType::QTransform),
    QT_ADD_STATIC_METATYPE("QMatrix4x4", QMetaType::QMatrix4x4),
    QT_ADD_STATIC_METATYPE("QVector2D", QMetaType::QVector2D),
    QT_ADD_STATIC_METATYPE("QVector3D", QMetaType::QVector3D),
    QT_ADD_STATIC_METATYPE("QVector4D", QMetaType::QVector4D),
    QT_ADD_STATIC_METATYPE("QQuaternion", QMetaType::QQuaternion),

    /* All Metatype builtins */
    QT_ADD_STATIC_METATYPE("void*", QMetaType::VoidStar),
    QT_ADD_STATIC_METATYPE("long", QMetaType::Long),
    QT_ADD_STATIC_METATYPE("short", QMetaType::Short),
    QT_ADD_STATIC_METATYPE("char", QMetaType::Char),
    QT_ADD_STATIC_METATYPE("ulong", QMetaType::ULong),
    QT_ADD_STATIC_METATYPE("ushort", QMetaType::UShort),
    QT_ADD_STATIC_METATYPE("uchar", QMetaType::UChar),
    QT_ADD_STATIC_METATYPE("float", QMetaType::Float),
    QT_ADD_STATIC_METATYPE("QObject*", QMetaType::QObjectStar),
    QT_ADD_STATIC_METATYPE("QWidget*", QMetaType::QWidgetStar),
    QT_ADD_STATIC_METATYPE("QVariant", QMetaType::QVariant),

    /* Type aliases - order doesn't matter */
    QT_ADD_STATIC_METATYPE("unsigned long", QMetaType::ULong),
    QT_ADD_STATIC_METATYPE("unsigned int", QMetaType::UInt),
    QT_ADD_STATIC_METATYPE("unsigned short", QMetaType::UShort),
    QT_ADD_STATIC_METATYPE("unsigned char", QMetaType::UChar),
    QT_ADD_STATIC_METATYPE("long long", QMetaType::LongLong),
    QT_ADD_STATIC_METATYPE("unsigned long long", QMetaType::ULongLong),
    QT_ADD_STATIC_METATYPE("qint8", QMetaType::Char),
    QT_ADD_STATIC_METATYPE("signed char", QMetaType::Char),
    QT_ADD_STATIC_METATYPE("quint8", QMetaType::UChar),
    QT_ADD_STATIC_METATYPE("qint16", QMetaType::Short),
    QT_ADD_STATIC_METATYPE("quint16", QMetaType::UShort),
    QT_ADD_STATIC_METATYPE("qint32", QMetaType::Int),
    QT_ADD_STATIC_METATYPE("quint32", QMetaType::UInt),
    QT_ADD_STATIC_METATYPE("qint64", QMetaType::LongLong),
    QT_ADD_STATIC_METATYPE("quint64", QMetaType::ULongLong),
    QT_ADD_STATIC_METATYPE("QList<QVariant>", QMetaType::QVariantList),
    QT_ADD_STATIC_METATYPE("QMap<QString,QVariant>", QMetaType::QVariantMap),
    QT_ADD_STATIC_METATYPE("QHash<QString,QVariant>", QMetaType::QVariantHash),
    // let QMetaTypeId2 figure out the type at compile time
    QT_ADD_STATIC_METATYPE("qreal", QMetaTypeId2<qreal>::MetaType),

    {0, 0, QMetaType::Void}
};

struct QMetaTypeGuiHelper
{
    QMetaType::Constructor constr;
    QMetaType::Destructor destr;
#ifndef QT_NO_DATASTREAM
    QMetaType::SaveOperator saveOp;
    QMetaType::LoadOperator loadOp;
#endif
};
Q_CORE_EXPORT const QMetaTypeGuiHelper *qMetaTypeGuiHelper = 0;

class QCustomTypeInfo
{
public:
    QCustomTypeInfo() : typeName(), constr(0), destr(0)
#ifndef QT_NO_DATASTREAM
    , saveOp(0), loadOp(0)
#endif
    {}

    QByteArray typeName;
    QMetaType::Constructor constr;
    QMetaType::Destructor destr;
#ifndef QT_NO_DATASTREAM
    QMetaType::SaveOperator saveOp;
    QMetaType::LoadOperator loadOp;
#endif
    int alias;
};

Q_DECLARE_TYPEINFO(QCustomTypeInfo, Q_MOVABLE_TYPE);
Q_GLOBAL_STATIC(QVector<QCustomTypeInfo>, customTypes)
Q_GLOBAL_STATIC(QReadWriteLock, customTypesLock)

#ifndef QT_NO_DATASTREAM
/*! \internal
*/
void QMetaType::registerStreamOperators(const char *typeName, SaveOperator saveOp,
                                        LoadOperator loadOp)
{
    int idx = type(typeName);
    if (!idx)
        return;
    registerStreamOperators(idx, saveOp, loadOp);
}

/*! \internal
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
    Returns the type name associated with the given \a type, or 0 if no
    matching type was found. The returned pointer must not be deleted.

    \sa type(), isRegistered(), Type
*/
const char *QMetaType::typeName(int type)
{
    enum { GuiTypeCount = LastGuiType - FirstGuiType };

    if (type >= 0 && type <= LastCoreType) {
        return types[type].typeName;
    } else if (type >= FirstGuiType && type <= LastGuiType) {
        return types[type - FirstGuiType + LastCoreType + 1].typeName;
    } else if (type >= FirstCoreExtType && type <= LastCoreExtType) {
        return types[type - FirstCoreExtType + GuiTypeCount + LastCoreType + 2].typeName;
    } else if (type >= User) {
        const QVector<QCustomTypeInfo> * const ct = customTypes();
        QReadLocker locker(customTypesLock());
        return ct && ct->count() > type - User && !ct->at(type - User).typeName.isEmpty()
                ? ct->at(type - User).typeName.constData()
                : static_cast<const char *>(0);
    }

    return 0;
}

/*! \internal
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

/*! \internal
    Similar to QMetaType::type(), but only looks in the custom set of
    types, and doesn't lock the mutex.
*/
static int qMetaTypeCustomType_unlocked(const char *typeName, int length)
{
    const QVector<QCustomTypeInfo> * const ct = customTypes();
    if (!ct)
        return 0;

    for (int v = 0; v < ct->count(); ++v) {
        const QCustomTypeInfo &customInfo = ct->at(v);
        if ((length == customInfo.typeName.size())
            && !strcmp(typeName, customInfo.typeName.constData())) {
            if (customInfo.alias >= 0)
                return customInfo.alias;
            return v + QMetaType::User;
        }
    }
    return 0;
}

/*! \internal

    Registers a user type for marshalling, with \a typeName, a \a
    destructor, and a \a constructor. Returns the type's handle,
    or -1 if the type could not be registered.
 */
int QMetaType::registerType(const char *typeName, Destructor destructor,
                            Constructor constructor)
{
    QVector<QCustomTypeInfo> *ct = customTypes();
    if (!ct || !typeName || !destructor || !constructor)
        return -1;

#ifdef QT_NO_QOBJECT
    NS(QByteArray) normalizedTypeName = typeName;
#else
    NS(QByteArray) normalizedTypeName = QMetaObject::normalizedType(typeName);
#endif

    int idx = qMetaTypeStaticType(normalizedTypeName.constData(),
                                  normalizedTypeName.size());

    if (!idx) {
        QWriteLocker locker(customTypesLock());
        idx = qMetaTypeCustomType_unlocked(normalizedTypeName.constData(),
                                           normalizedTypeName.size());
        if (!idx) {
            QCustomTypeInfo inf;
            inf.typeName = normalizedTypeName;
            inf.constr = constructor;
            inf.destr = destructor;
            inf.alias = -1;
            idx = ct->size() + User;
            ct->append(inf);
        }
    }
    return idx;
}

/*! \internal
    \since 4.7

    Registers a user type for marshalling, as an alias of another type (typedef)
*/
int QMetaType::registerTypedef(const char* typeName, int aliasId)
{
    QVector<QCustomTypeInfo> *ct = customTypes();
    if (!ct || !typeName)
        return -1;

#ifdef QT_NO_QOBJECT
    NS(QByteArray) normalizedTypeName = typeName;
#else
    NS(QByteArray) normalizedTypeName = QMetaObject::normalizedType(typeName);
#endif

    int idx = qMetaTypeStaticType(normalizedTypeName.constData(),
                                  normalizedTypeName.size());

    if (idx) {
        Q_ASSERT(idx == aliasId);
        return idx;
    }

    QWriteLocker locker(customTypesLock());
    idx = qMetaTypeCustomType_unlocked(normalizedTypeName.constData(),
                                           normalizedTypeName.size());

    if (idx)
        return idx;

    QCustomTypeInfo inf;
    inf.typeName = normalizedTypeName;
    inf.alias = aliasId;
    inf.constr = 0;
    inf.destr = 0;
    ct->append(inf);
    return aliasId;
}

/*!
    \since 4.4

    Unregisters a user type, with \a typeName.

    \sa type(), typeName()
 */
void QMetaType::unregisterType(const char *typeName)
{
    QVector<QCustomTypeInfo> *ct = customTypes();
    if (!ct || !typeName)
        return;

#ifdef QT_NO_QOBJECT
    NS(QByteArray) normalizedTypeName = typeName;
#else
    NS(QByteArray) normalizedTypeName = QMetaObject::normalizedType(typeName);
#endif
    QWriteLocker locker(customTypesLock());
    for (int v = 0; v < ct->count(); ++v) {
        if (ct->at(v).typeName == typeName) {
            QCustomTypeInfo &inf = (*ct)[v];
            inf.typeName.clear();
            inf.constr = 0;
            inf.destr = 0;
            inf.alias = -1;
        }
    }
}

/*!
    Returns true if the datatype with ID \a type is registered;
    otherwise returns false.

    \sa type(), typeName(), Type
*/
bool QMetaType::isRegistered(int type)
{
    if (type >= 0 && type < User) {
        // predefined type
        return true;
    }
    QReadLocker locker(customTypesLock());
    const QVector<QCustomTypeInfo> * const ct = customTypes();
    return ((type >= User) && (ct && ct->count() > type - User) && !ct->at(type - User).typeName.isEmpty());
}

/*!
    Returns a handle to the type called \a typeName, or 0 if there is
    no such type.

    \sa isRegistered(), typeName(), Type
*/
int QMetaType::type(const char *typeName)
{
    int length = qstrlen(typeName);
    if (!length)
        return 0;
    int type = qMetaTypeStaticType(typeName, length);
    if (!type) {
        QReadLocker locker(customTypesLock());
        type = qMetaTypeCustomType_unlocked(typeName, length);
#ifndef QT_NO_QOBJECT
        if (!type) {
            const NS(QByteArray) normalizedTypeName = QMetaObject::normalizedType(typeName);
            type = qMetaTypeStaticType(normalizedTypeName.constData(),
                                       normalizedTypeName.size());
            if (!type) {
                type = qMetaTypeCustomType_unlocked(normalizedTypeName.constData(),
                                                    normalizedTypeName.size());
            }
        }
#endif
    }
    return type;
}

#ifndef QT_NO_DATASTREAM
/*!
    Writes the object pointed to by \a data with the ID \a type to
    the given \a stream. Returns true if the object is saved
    successfully; otherwise returns false.

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
    case QMetaType::Void:
    case QMetaType::VoidStar:
    case QMetaType::QObjectStar:
    case QMetaType::QWidgetStar:
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
    case QMetaType::QEasingCurve:
        stream << *static_cast<const NS(QEasingCurve)*>(data);
        break;
#endif
#ifdef QT3_SUPPORT
    case QMetaType::QColorGroup:
#endif
    case QMetaType::QFont:
    case QMetaType::QPixmap:
    case QMetaType::QBrush:
    case QMetaType::QColor:
    case QMetaType::QPalette:
    case QMetaType::QIcon:
    case QMetaType::QImage:
    case QMetaType::QPolygon:
    case QMetaType::QRegion:
    case QMetaType::QBitmap:
    case QMetaType::QCursor:
    case QMetaType::QSizePolicy:
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
        if (!qMetaTypeGuiHelper)
            return false;
        qMetaTypeGuiHelper[type - FirstGuiType].saveOp(stream, data);
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
    stream into \a data. Returns true if the object is loaded
    successfully; otherwise returns false.

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
    case QMetaType::Void:
    case QMetaType::VoidStar:
    case QMetaType::QObjectStar:
    case QMetaType::QWidgetStar:
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
    case QMetaType::QEasingCurve:
        stream >> *static_cast< NS(QEasingCurve)*>(data);
        break;
#endif
#ifdef QT3_SUPPORT
    case QMetaType::QColorGroup:
#endif
    case QMetaType::QFont:
    case QMetaType::QPixmap:
    case QMetaType::QBrush:
    case QMetaType::QColor:
    case QMetaType::QPalette:
    case QMetaType::QIcon:
    case QMetaType::QImage:
    case QMetaType::QPolygon:
    case QMetaType::QRegion:
    case QMetaType::QBitmap:
    case QMetaType::QCursor:
    case QMetaType::QSizePolicy:
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
        if (!qMetaTypeGuiHelper)
            return false;
        qMetaTypeGuiHelper[type - FirstGuiType].loadOp(stream, data);
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

/*!
    Returns a copy of \a copy, assuming it is of type \a type. If \a
    copy is zero, creates a default type.

    \sa destroy(), isRegistered(), Type
*/
void *QMetaType::construct(int type, const void *copy)
{
    if (copy) {
        switch(type) {
        case QMetaType::VoidStar:
        case QMetaType::QObjectStar:
        case QMetaType::QWidgetStar:
            return new void *(*static_cast<void* const *>(copy));
        case QMetaType::Long:
            return new long(*static_cast<const long*>(copy));
        case QMetaType::Int:
            return new int(*static_cast<const int*>(copy));
        case QMetaType::Short:
            return new short(*static_cast<const short*>(copy));
        case QMetaType::Char:
            return new char(*static_cast<const char*>(copy));
        case QMetaType::ULong:
            return new ulong(*static_cast<const ulong*>(copy));
        case QMetaType::UInt:
            return new uint(*static_cast<const uint*>(copy));
        case QMetaType::LongLong:
            return new qlonglong(*static_cast<const qlonglong*>(copy));
        case QMetaType::ULongLong:
            return new qulonglong(*static_cast<const qulonglong*>(copy));
        case QMetaType::UShort:
            return new ushort(*static_cast<const ushort*>(copy));
        case QMetaType::UChar:
            return new uchar(*static_cast<const uchar*>(copy));
        case QMetaType::Bool:
            return new bool(*static_cast<const bool*>(copy));
        case QMetaType::Float:
            return new float(*static_cast<const float*>(copy));
        case QMetaType::Double:
            return new double(*static_cast<const double*>(copy));
        case QMetaType::QChar:
            return new NS(QChar)(*static_cast<const NS(QChar)*>(copy));
#ifndef QT_BOOTSTRAPPED
        case QMetaType::QVariantMap:
            return new NS(QVariantMap)(*static_cast<const NS(QVariantMap)*>(copy));
        case QMetaType::QVariantHash:
            return new NS(QVariantHash)(*static_cast<const NS(QVariantHash)*>(copy));
        case QMetaType::QVariantList:
            return new NS(QVariantList)(*static_cast<const NS(QVariantList)*>(copy));
        case QMetaType::QVariant:
            return new NS(QVariant)(*static_cast<const NS(QVariant)*>(copy));
#endif
        case QMetaType::QByteArray:
            return new NS(QByteArray)(*static_cast<const NS(QByteArray)*>(copy));
        case QMetaType::QString:
            return new NS(QString)(*static_cast<const NS(QString)*>(copy));
        case QMetaType::QStringList:
            return new NS(QStringList)(*static_cast<const NS(QStringList)*>(copy));
#ifndef QT_BOOTSTRAPPED
        case QMetaType::QBitArray:
            return new NS(QBitArray)(*static_cast<const NS(QBitArray)*>(copy));
#endif
        case QMetaType::QDate:
            return new NS(QDate)(*static_cast<const NS(QDate)*>(copy));
        case QMetaType::QTime:
            return new NS(QTime)(*static_cast<const NS(QTime)*>(copy));
        case QMetaType::QDateTime:
            return new NS(QDateTime)(*static_cast<const NS(QDateTime)*>(copy));
#ifndef QT_BOOTSTRAPPED
        case QMetaType::QUrl:
            return new NS(QUrl)(*static_cast<const NS(QUrl)*>(copy));
#endif
        case QMetaType::QLocale:
            return new NS(QLocale)(*static_cast<const NS(QLocale)*>(copy));
#ifndef QT_NO_GEOM_VARIANT
        case QMetaType::QRect:
            return new NS(QRect)(*static_cast<const NS(QRect)*>(copy));
        case QMetaType::QRectF:
            return new NS(QRectF)(*static_cast<const NS(QRectF)*>(copy));
        case QMetaType::QSize:
            return new NS(QSize)(*static_cast<const NS(QSize)*>(copy));
        case QMetaType::QSizeF:
            return new NS(QSizeF)(*static_cast<const NS(QSizeF)*>(copy));
        case QMetaType::QLine:
            return new NS(QLine)(*static_cast<const NS(QLine)*>(copy));
        case QMetaType::QLineF:
            return new NS(QLineF)(*static_cast<const NS(QLineF)*>(copy));
        case QMetaType::QPoint:
            return new NS(QPoint)(*static_cast<const NS(QPoint)*>(copy));
        case QMetaType::QPointF:
            return new NS(QPointF)(*static_cast<const NS(QPointF)*>(copy));
#endif
#ifndef QT_NO_REGEXP
        case QMetaType::QRegExp:
            return new NS(QRegExp)(*static_cast<const NS(QRegExp)*>(copy));
#endif
#ifndef QT_BOOTSTRAPPED
        case QMetaType::QEasingCurve:
            return new NS(QEasingCurve)(*static_cast<const NS(QEasingCurve)*>(copy));
#endif
        case QMetaType::Void:
            return 0;
        default:
            ;
        }
    } else {
        switch(type) {
        case QMetaType::VoidStar:
        case QMetaType::QObjectStar:
        case QMetaType::QWidgetStar:
            return new void *;
        case QMetaType::Long:
            return new long;
        case QMetaType::Int:
            return new int;
        case QMetaType::Short:
            return new short;
        case QMetaType::Char:
            return new char;
        case QMetaType::ULong:
            return new ulong;
        case QMetaType::UInt:
            return new uint;
        case QMetaType::LongLong:
            return new qlonglong;
        case QMetaType::ULongLong:
            return new qulonglong;
        case QMetaType::UShort:
            return new ushort;
        case QMetaType::UChar:
            return new uchar;
        case QMetaType::Bool:
            return new bool;
        case QMetaType::Float:
            return new float;
        case QMetaType::Double:
            return new double;
        case QMetaType::QChar:
            return new NS(QChar);
#ifndef QT_BOOTSTRAPPED
        case QMetaType::QVariantMap:
            return new NS(QVariantMap);
        case QMetaType::QVariantHash:
            return new NS(QVariantHash);
        case QMetaType::QVariantList:
            return new NS(QVariantList);
        case QMetaType::QVariant:
            return new NS(QVariant);
#endif
        case QMetaType::QByteArray:
            return new NS(QByteArray);
        case QMetaType::QString:
            return new NS(QString);
        case QMetaType::QStringList:
            return new NS(QStringList);
#ifndef QT_BOOTSTRAPPED
        case QMetaType::QBitArray:
            return new NS(QBitArray);
#endif
        case QMetaType::QDate:
            return new NS(QDate);
        case QMetaType::QTime:
            return new NS(QTime);
        case QMetaType::QDateTime:
            return new NS(QDateTime);
#ifndef QT_BOOTSTRAPPED
        case QMetaType::QUrl:
            return new NS(QUrl);
#endif
        case QMetaType::QLocale:
            return new NS(QLocale);
#ifndef QT_NO_GEOM_VARIANT
        case QMetaType::QRect:
            return new NS(QRect);
        case QMetaType::QRectF:
            return new NS(QRectF);
        case QMetaType::QSize:
            return new NS(QSize);
        case QMetaType::QSizeF:
            return new NS(QSizeF);
        case QMetaType::QLine:
            return new NS(QLine);
        case QMetaType::QLineF:
            return new NS(QLineF);
        case QMetaType::QPoint:
            return new NS(QPoint);
        case QMetaType::QPointF:
            return new NS(QPointF);
#endif
#ifndef QT_NO_REGEXP
        case QMetaType::QRegExp:
            return new NS(QRegExp);
#endif
#ifndef QT_BOOTSTRAPPED
        case QMetaType::QEasingCurve:
            return new NS(QEasingCurve);
#endif
        case QMetaType::Void:
            return 0;
        default:
            ;
        }
    }

    Constructor constr = 0;
    if (type >= FirstGuiType && type <= LastGuiType) {
        if (!qMetaTypeGuiHelper)
            return 0;
        constr = qMetaTypeGuiHelper[type - FirstGuiType].constr;
    } else {
        const QVector<QCustomTypeInfo> * const ct = customTypes();
        QReadLocker locker(customTypesLock());
        if (type < User || !ct || ct->count() <= type - User)
            return 0;
        if (ct->at(type - User).typeName.isEmpty())
            return 0;
        constr = ct->at(type - User).constr;
    }

    return constr(copy);
}

/*!
    Destroys the \a data, assuming it is of the \a type given.

    \sa construct(), isRegistered(), Type
*/
void QMetaType::destroy(int type, void *data)
{
    if (!data)
        return;
    switch(type) {
    case QMetaType::VoidStar:
    case QMetaType::QObjectStar:
    case QMetaType::QWidgetStar:
        delete static_cast<void**>(data);
        break;
    case QMetaType::Long:
        delete static_cast<long*>(data);
        break;
    case QMetaType::Int:
        delete static_cast<int*>(data);
        break;
    case QMetaType::Short:
        delete static_cast<short*>(data);
        break;
    case QMetaType::Char:
        delete static_cast<char*>(data);
        break;
    case QMetaType::ULong:
        delete static_cast<ulong*>(data);
        break;
    case QMetaType::LongLong:
        delete static_cast<qlonglong*>(data);
        break;
    case QMetaType::ULongLong:
        delete static_cast<qulonglong*>(data);
        break;
    case QMetaType::UInt:
        delete static_cast<uint*>(data);
        break;
    case QMetaType::UShort:
        delete static_cast<ushort*>(data);
        break;
    case QMetaType::UChar:
        delete static_cast<uchar*>(data);
        break;
    case QMetaType::Bool:
        delete static_cast<bool*>(data);
        break;
    case QMetaType::Float:
        delete static_cast<float*>(data);
        break;
    case QMetaType::Double:
        delete static_cast<double*>(data);
        break;
    case QMetaType::QChar:
        delete static_cast< NS(QChar)* >(data);
        break;
#ifndef QT_BOOTSTRAPPED
    case QMetaType::QVariantMap:
        delete static_cast< NS(QVariantMap)* >(data);
        break;
    case QMetaType::QVariantHash:
        delete static_cast< NS(QVariantHash)* >(data);
        break;
    case QMetaType::QVariantList:
        delete static_cast< NS(QVariantList)* >(data);
        break;
    case QMetaType::QVariant:
        delete static_cast< NS(QVariant)* >(data);
        break;
#endif
    case QMetaType::QByteArray:
        delete static_cast< NS(QByteArray)* >(data);
        break;
    case QMetaType::QString:
        delete static_cast< NS(QString)* >(data);
        break;
    case QMetaType::QStringList:
        delete static_cast< NS(QStringList)* >(data);
        break;
#ifndef QT_BOOTSTRAPPED
    case QMetaType::QBitArray:
        delete static_cast< NS(QBitArray)* >(data);
        break;
#endif
    case QMetaType::QDate:
        delete static_cast< NS(QDate)* >(data);
        break;
    case QMetaType::QTime:
        delete static_cast< NS(QTime)* >(data);
        break;
    case QMetaType::QDateTime:
        delete static_cast< NS(QDateTime)* >(data);
        break;
#ifndef QT_BOOTSTRAPPED
    case QMetaType::QUrl:
        delete static_cast< NS(QUrl)* >(data);
#endif
        break;
    case QMetaType::QLocale:
        delete static_cast< NS(QLocale)* >(data);
        break;
#ifndef QT_NO_GEOM_VARIANT
    case QMetaType::QRect:
        delete static_cast< NS(QRect)* >(data);
        break;
    case QMetaType::QRectF:
        delete static_cast< NS(QRectF)* >(data);
        break;
    case QMetaType::QSize:
        delete static_cast< NS(QSize)* >(data);
        break;
    case QMetaType::QSizeF:
        delete static_cast< NS(QSizeF)* >(data);
        break;
    case QMetaType::QLine:
        delete static_cast< NS(QLine)* >(data);
        break;
    case QMetaType::QLineF:
        delete static_cast< NS(QLineF)* >(data);
        break;
    case QMetaType::QPoint:
        delete static_cast< NS(QPoint)* >(data);
        break;
    case QMetaType::QPointF:
        delete static_cast< NS(QPointF)* >(data);
        break;
#endif
#ifndef QT_NO_REGEXP
    case QMetaType::QRegExp:
        delete static_cast< NS(QRegExp)* >(data);
        break;
#endif
#ifndef QT_BOOTSTRAPPED
    case QMetaType::QEasingCurve:
        delete static_cast< NS(QEasingCurve)* >(data);
        break;
#endif
    case QMetaType::Void:
        break;
    default: {
        const QVector<QCustomTypeInfo> * const ct = customTypes();
        Destructor destr = 0;
        if (type >= FirstGuiType && type <= LastGuiType) {
            Q_ASSERT(qMetaTypeGuiHelper);

            if (!qMetaTypeGuiHelper)
                return;
            destr = qMetaTypeGuiHelper[type - FirstGuiType].destr;
        } else {
            QReadLocker locker(customTypesLock());
            if (type < User || !ct || ct->count() <= type - User)
                break;
            if (ct->at(type - User).typeName.isEmpty())
                break;
            destr = ct->at(type - User).destr;
        }
        destr(data);
        break; }
    }
}

/*!
    \fn int qRegisterMetaType(const char *typeName)
    \relates QMetaType
    \threadsafe

    Registers the type name \a typeName for the type \c{T}. Returns
    the internal ID used by QMetaType. Any class or struct that has a
    public default constructor, a public copy constructor and a public
    destructor can be registered.

    After a type has been registered, you can create and destroy
    objects of that type dynamically at run-time.

    This example registers the class \c{MyClass}:

    \snippet doc/src/snippets/code/src_corelib_kernel_qmetatype.cpp 4

    This function is useful to register typedefs so they can be used
    by QMetaProperty, or in QueuedConnections

    \snippet doc/src/snippets/code/src_corelib_kernel_qmetatype.cpp 9

    \sa qRegisterMetaTypeStreamOperators(), QMetaType::isRegistered(),
        Q_DECLARE_METATYPE()
*/

/*!
    \fn int qRegisterMetaTypeStreamOperators(const char *typeName)
    \relates QMetaType
    \threadsafe

    Registers the stream operators for the type \c{T} called \a
    typeName.

    Afterward, the type can be streamed using QMetaType::load() and
    QMetaType::save(). These functions are used when streaming a
    QVariant.

    \snippet doc/src/snippets/code/src_corelib_kernel_qmetatype.cpp 5

    The stream operators should have the following signatures:

    \snippet doc/src/snippets/code/src_corelib_kernel_qmetatype.cpp 6

    \sa qRegisterMetaType(), QMetaType::isRegistered(), Q_DECLARE_METATYPE()
*/

/*! \typedef QMetaType::Destructor
    \internal
*/
/*! \typedef QMetaType::Constructor
    \internal
*/
/*! \typedef QMetaType::SaveOperator
    \internal
*/
/*! \typedef QMetaType::LoadOperator
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

    \snippet doc/src/snippets/code/src_corelib_kernel_qmetatype.cpp 7

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

/*! \fn int qMetaTypeId()
    \relates QMetaType
    \threadsafe
    \since 4.1

    Returns the meta type id of type \c T at compile time. If the
    type was not declared with Q_DECLARE_METATYPE(), compilation will
    fail.

    Typical usage:

    \snippet doc/src/snippets/code/src_corelib_kernel_qmetatype.cpp 8

    QMetaType::type() returns the same ID as qMetaTypeId(), but does
    a lookup at runtime based on the name of the type.
    QMetaType::type() is a bit slower, but compilation succeeds if a
    type is not registered.

    \sa Q_DECLARE_METATYPE(), QMetaType::type()
*/

QT_END_NAMESPACE
