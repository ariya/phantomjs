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

#ifndef QMETATYPE_P_H
#define QMETATYPE_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include "qmetatype.h"

QT_BEGIN_NAMESPACE

namespace QModulesPrivate {
enum Names { Core, Gui, Widgets, Unknown, ModulesCount /* ModulesCount has to be at the end */ };

static inline int moduleForType(const uint typeId)
{
    if (typeId <= QMetaType::LastCoreType)
        return Core;
    if (typeId >= QMetaType::FirstGuiType && typeId <= QMetaType::LastGuiType)
        return Gui;
    if (typeId >= QMetaType::FirstWidgetsType && typeId <= QMetaType::LastWidgetsType)
        return Widgets;
    return Unknown;
}

template <typename T>
class QTypeModuleInfo
{
public:
    enum Module {
        IsCore = !QTypeInfo<T>::isComplex, // Primitive types are in Core
        IsWidget = false,
        IsGui = false,
        IsUnknown = !IsCore
    };
};

#define QT_ASSIGN_TYPE_TO_MODULE(TYPE, MODULE) \
template<> \
class QTypeModuleInfo<TYPE > \
{ \
public: \
    enum Module { \
        IsCore = (((MODULE) == (QModulesPrivate::Core))), \
        IsWidget = (((MODULE) == (QModulesPrivate::Widgets))), \
        IsGui = (((MODULE) == (QModulesPrivate::Gui))), \
        IsUnknown = !(IsCore || IsWidget || IsGui) \
    }; \
    static inline int module() { return MODULE; } \
    Q_STATIC_ASSERT((IsUnknown && !(IsCore || IsWidget || IsGui)) \
                 || (IsCore && !(IsUnknown || IsWidget || IsGui)) \
                 || (IsWidget && !(IsUnknown || IsCore || IsGui)) \
                 || (IsGui && !(IsUnknown || IsCore || IsWidget))); \
};


#define QT_DECLARE_CORE_MODULE_TYPES_ITER(TypeName, TypeId, Name) \
    QT_ASSIGN_TYPE_TO_MODULE(Name, QModulesPrivate::Core);
#define QT_DECLARE_GUI_MODULE_TYPES_ITER(TypeName, TypeId, Name) \
    QT_ASSIGN_TYPE_TO_MODULE(Name, QModulesPrivate::Gui);
#define QT_DECLARE_WIDGETS_MODULE_TYPES_ITER(TypeName, TypeId, Name) \
    QT_ASSIGN_TYPE_TO_MODULE(Name, QModulesPrivate::Widgets);

QT_FOR_EACH_STATIC_CORE_CLASS(QT_DECLARE_CORE_MODULE_TYPES_ITER)
QT_FOR_EACH_STATIC_CORE_TEMPLATE(QT_DECLARE_CORE_MODULE_TYPES_ITER)
QT_FOR_EACH_STATIC_GUI_CLASS(QT_DECLARE_GUI_MODULE_TYPES_ITER)
QT_FOR_EACH_STATIC_WIDGETS_CLASS(QT_DECLARE_WIDGETS_MODULE_TYPES_ITER)
} // namespace QModulesPrivate

#undef QT_DECLARE_CORE_MODULE_TYPES_ITER
#undef QT_DECLARE_GUI_MODULE_TYPES_ITER
#undef QT_DECLARE_WIDGETS_MODULE_TYPES_ITER

class QMetaTypeInterface
{
public:
    QMetaType::Creator creator;
    QMetaType::Deleter deleter;
    QMetaType::SaveOperator saveOp;
    QMetaType::LoadOperator loadOp;
    QMetaType::Constructor constructor;
    QMetaType::Destructor destructor;
    int size;
    quint32 flags; // same as QMetaType::TypeFlags
    const QMetaObject *metaObject;
};

#ifndef QT_NO_DATASTREAM
#  define QT_METATYPE_INTERFACE_INIT_DATASTREAM_IMPL(Type) \
    /*saveOp*/(QtMetaTypePrivate::QMetaTypeFunctionHelper<Type, QtMetaTypePrivate::TypeDefinition<Type>::IsAvailable>::Save), \
    /*loadOp*/(QtMetaTypePrivate::QMetaTypeFunctionHelper<Type, QtMetaTypePrivate::TypeDefinition<Type>::IsAvailable>::Load),
#  define QT_METATYPE_INTERFACE_INIT_EMPTY_DATASTREAM_IMPL(Type) \
    /*saveOp*/ 0, \
    /*loadOp*/ 0,
#else
#  define QT_METATYPE_INTERFACE_INIT_EMPTY_DATASTREAM_IMPL(Type) \
    /*saveOp*/ 0, \
    /*loadOp*/ 0,
#  define QT_METATYPE_INTERFACE_INIT_DATASTREAM_IMPL(Type) \
    QT_METATYPE_INTERFACE_INIT_EMPTY_DATASTREAM_IMPL(Type)
#endif

#ifndef QT_BOOTSTRAPPED
#define METAOBJECT_DELEGATE(Type) (QtPrivate::MetaObjectForType<Type>::value())
#else
#define METAOBJECT_DELEGATE(Type) 0
#endif

#define QT_METATYPE_INTERFACE_INIT_IMPL(Type, DATASTREAM_DELEGATE) \
{ \
    /*creator*/(QtMetaTypePrivate::QMetaTypeFunctionHelper<Type, QtMetaTypePrivate::TypeDefinition<Type>::IsAvailable>::Create), \
    /*deleter*/(QtMetaTypePrivate::QMetaTypeFunctionHelper<Type, QtMetaTypePrivate::TypeDefinition<Type>::IsAvailable>::Delete), \
    DATASTREAM_DELEGATE(Type) \
    /*constructor*/(QtMetaTypePrivate::QMetaTypeFunctionHelper<Type, QtMetaTypePrivate::TypeDefinition<Type>::IsAvailable>::Construct), \
    /*destructor*/(QtMetaTypePrivate::QMetaTypeFunctionHelper<Type, QtMetaTypePrivate::TypeDefinition<Type>::IsAvailable>::Destruct), \
    /*size*/(QTypeInfo<Type>::sizeOf), \
    /*flags*/QtPrivate::QMetaTypeTypeFlags<Type>::Flags, \
    /*metaObject*/METAOBJECT_DELEGATE(Type) \
}


/* These  QT_METATYPE_INTERFACE_INIT* macros are used to initialize QMetaTypeInterface instance.

 - QT_METATYPE_INTERFACE_INIT(Type) -> It takes Type argument and creates all necessary wrapper functions for the Type,
   it detects if QT_NO_DATASTREAM was defined. Probably it is the macro that you want to use.

 - QT_METATYPE_INTERFACE_INIT_EMPTY() -> It initializes an empty QMetaTypeInterface instance.

 - QT_METATYPE_INTERFACE_INIT_NO_DATASTREAM(Type) -> Temporary workaround for missing auto-detection of data stream
   operators. It creates same instance as QT_METATYPE_INTERFACE_INIT(Type) but with null stream operators callbacks.
 */
#define QT_METATYPE_INTERFACE_INIT(Type) QT_METATYPE_INTERFACE_INIT_IMPL(Type, QT_METATYPE_INTERFACE_INIT_DATASTREAM_IMPL)
#define QT_METATYPE_INTERFACE_INIT_NO_DATASTREAM(Type) QT_METATYPE_INTERFACE_INIT_IMPL(Type, QT_METATYPE_INTERFACE_INIT_EMPTY_DATASTREAM_IMPL)
#define QT_METATYPE_INTERFACE_INIT_EMPTY() \
{ \
    /*creator*/ 0, \
    /*deleter*/ 0, \
    QT_METATYPE_INTERFACE_INIT_EMPTY_DATASTREAM_IMPL(void) \
    /*constructor*/ 0, \
    /*destructor*/ 0, \
    /*size*/ 0, \
    /*flags*/ 0, \
    /*metaObject*/ 0 \
}

namespace QtMetaTypePrivate {
template<typename T>
struct TypeDefinition {
    static const bool IsAvailable = true;
};

// Ignore these types, as incomplete
#ifdef QT_BOOTSTRAPPED
template<> struct TypeDefinition<QBitArray> { static const bool IsAvailable = false; };
template<> struct TypeDefinition<QEasingCurve> { static const bool IsAvailable = false; };
template<> struct TypeDefinition<QJsonArray> { static const bool IsAvailable = false; };
template<> struct TypeDefinition<QJsonDocument> { static const bool IsAvailable = false; };
template<> struct TypeDefinition<QJsonObject> { static const bool IsAvailable = false; };
template<> struct TypeDefinition<QJsonValue> { static const bool IsAvailable = false; };
template<> struct TypeDefinition<QModelIndex> { static const bool IsAvailable = false; };
template<> struct TypeDefinition<QUrl> { static const bool IsAvailable = false; };
#endif
#ifdef QT_NO_GEOM_VARIANT
template<> struct TypeDefinition<QRect> { static const bool IsAvailable = false; };
template<> struct TypeDefinition<QRectF> { static const bool IsAvailable = false; };
template<> struct TypeDefinition<QSize> { static const bool IsAvailable = false; };
template<> struct TypeDefinition<QSizeF> { static const bool IsAvailable = false; };
template<> struct TypeDefinition<QLine> { static const bool IsAvailable = false; };
template<> struct TypeDefinition<QLineF> { static const bool IsAvailable = false; };
template<> struct TypeDefinition<QPoint> { static const bool IsAvailable = false; };
template<> struct TypeDefinition<QPointF> { static const bool IsAvailable = false; };
#endif
#ifdef QT_NO_REGEXP
template<> struct TypeDefinition<QRegExp> { static const bool IsAvailable = false; };
#endif
#if defined(QT_BOOTSTRAPPED) || defined(QT_NO_REGULAREXPRESSION)
template<> struct TypeDefinition<QRegularExpression> { static const bool IsAvailable = false; };
#endif
#ifdef QT_NO_SHORTCUT
template<> struct TypeDefinition<QKeySequence> { static const bool IsAvailable = false; };
#endif
#ifdef QT_NO_CURSOR
template<> struct TypeDefinition<QCursor> { static const bool IsAvailable = false; };
#endif
#ifdef QT_NO_MATRIX4X4
template<> struct TypeDefinition<QMatrix4x4> { static const bool IsAvailable = false; };
#endif
#ifdef QT_NO_VECTOR2D
template<> struct TypeDefinition<QVector2D> { static const bool IsAvailable = false; };
#endif
#ifdef QT_NO_VECTOR3D
template<> struct TypeDefinition<QVector3D> { static const bool IsAvailable = false; };
#endif
#ifdef QT_NO_VECTOR4D
template<> struct TypeDefinition<QVector4D> { static const bool IsAvailable = false; };
#endif
#ifdef QT_NO_QUATERNION
template<> struct TypeDefinition<QQuaternion> { static const bool IsAvailable = false; };
#endif
#ifdef QT_NO_ICON
template<> struct TypeDefinition<QIcon> { static const bool IsAvailable = false; };
#endif
} //namespace QtMetaTypePrivate

QT_END_NAMESPACE

#endif // QMETATYPE_P_H
