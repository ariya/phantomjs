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

#include <QtCore/qtypetraits.h>

#ifndef QTYPEINFO_H
#define QTYPEINFO_H

QT_BEGIN_NAMESPACE

/*
   QTypeInfo     - type trait functionality
*/

/*
  The catch-all template.
*/

template <typename T>
class QTypeInfo
{
public:
    enum {
        isPointer = false,
        isIntegral = QtPrivate::is_integral<T>::value,
        isComplex = true,
        isStatic = true,
        isLarge = (sizeof(T)>sizeof(void*)),
        isDummy = false,
        sizeOf = sizeof(T)
    };
};

template<>
class QTypeInfo<void>
{
public:
    enum {
        isPointer = false,
        isIntegral = false,
        isComplex = false,
        isStatic = false,
        isLarge = false,
        isDummy = false,
        sizeOf = 0
    };
};

template <typename T>
class QTypeInfo<T*>
{
public:
    enum {
        isPointer = true,
        isIntegral = false,
        isComplex = false,
        isStatic = false,
        isLarge = false,
        isDummy = false,
        sizeOf = sizeof(T*)
    };
};

/*!
    \class QTypeInfoMerger
    \inmodule QtCore
    \internal

    \brief QTypeInfoMerger merges the QTypeInfo flags of T1, T2... and presents them
    as a QTypeInfo<T> would do.

    Let's assume that we have a simple set of structs:

    \snippet code/src_corelib_global_qglobal.cpp 50

    To create a proper QTypeInfo specialization for A struct, we have to check
    all sub-components; B, C and D, then take the lowest common denominator and call
    Q_DECLARE_TYPEINFO with the resulting flags. An easier and less fragile approach is to
    use QTypeInfoMerger, which does that automatically. So struct A would have
    the following QTypeInfo definition:

    \snippet code/src_corelib_global_qglobal.cpp 51
*/
template <class T, class T1, class T2 = T1, class T3 = T1, class T4 = T1>
class QTypeInfoMerger
{
public:
    enum {
        isComplex = QTypeInfo<T1>::isComplex || QTypeInfo<T2>::isComplex || QTypeInfo<T3>::isComplex || QTypeInfo<T4>::isComplex,
        isStatic = QTypeInfo<T1>::isStatic || QTypeInfo<T2>::isStatic || QTypeInfo<T3>::isStatic || QTypeInfo<T4>::isStatic,
        isLarge = sizeof(T) > sizeof(void*),
        isPointer = false,
        isIntegral = false,
        isDummy = false,
        sizeOf = sizeof(T)
    };
};

#define Q_DECLARE_MOVABLE_CONTAINER(CONTAINER) \
template <typename T> class CONTAINER; \
template <typename T> \
class QTypeInfo< CONTAINER<T> > \
{ \
public: \
    enum { \
        isPointer = false, \
        isIntegral = false, \
        isComplex = true, \
        isStatic = false, \
        isLarge = (sizeof(CONTAINER<T>) > sizeof(void*)), \
        isDummy = false, \
        sizeOf = sizeof(CONTAINER<T>) \
    }; \
};

Q_DECLARE_MOVABLE_CONTAINER(QList)
Q_DECLARE_MOVABLE_CONTAINER(QVector)
Q_DECLARE_MOVABLE_CONTAINER(QQueue)
Q_DECLARE_MOVABLE_CONTAINER(QStack)
Q_DECLARE_MOVABLE_CONTAINER(QLinkedList)
Q_DECLARE_MOVABLE_CONTAINER(QSet)

#undef Q_DECLARE_MOVABLE_CONTAINER

/*
   Specialize a specific type with:

     Q_DECLARE_TYPEINFO(type, flags);

   where 'type' is the name of the type to specialize and 'flags' is
   logically-OR'ed combination of the flags below.
*/
enum { /* TYPEINFO flags */
    Q_COMPLEX_TYPE = 0,
    Q_PRIMITIVE_TYPE = 0x1,
    Q_STATIC_TYPE = 0,
    Q_MOVABLE_TYPE = 0x2,
    Q_DUMMY_TYPE = 0x4
};

#define Q_DECLARE_TYPEINFO_BODY(TYPE, FLAGS) \
class QTypeInfo<TYPE > \
{ \
public: \
    enum { \
        isComplex = (((FLAGS) & Q_PRIMITIVE_TYPE) == 0), \
        isStatic = (((FLAGS) & (Q_MOVABLE_TYPE | Q_PRIMITIVE_TYPE)) == 0), \
        isLarge = (sizeof(TYPE)>sizeof(void*)), \
        isPointer = false, \
        isIntegral = QtPrivate::is_integral< TYPE >::value, \
        isDummy = (((FLAGS) & Q_DUMMY_TYPE) != 0), \
        sizeOf = sizeof(TYPE) \
    }; \
    static inline const char *name() { return #TYPE; } \
}

#define Q_DECLARE_TYPEINFO(TYPE, FLAGS) \
template<> \
Q_DECLARE_TYPEINFO_BODY(TYPE, FLAGS)

/* Specialize QTypeInfo for QFlags<T> */
template<typename T> class QFlags;
template<typename T>
Q_DECLARE_TYPEINFO_BODY(QFlags<T>, Q_PRIMITIVE_TYPE);

/*
   Specialize a shared type with:

     Q_DECLARE_SHARED(type)

   where 'type' is the name of the type to specialize.  NOTE: shared
   types must define a member-swap, and be defined in the same
   namespace as Qt for this to work.
*/
#define Q_DECLARE_SHARED_STL(TYPE) \
QT_END_NAMESPACE \
namespace std { \
    template<> inline void swap< QT_PREPEND_NAMESPACE(TYPE) >(QT_PREPEND_NAMESPACE(TYPE) &value1, QT_PREPEND_NAMESPACE(TYPE) &value2) \
    { value1.swap(value2); } \
} \
QT_BEGIN_NAMESPACE

#define Q_DECLARE_SHARED(TYPE)                                          \
Q_DECLARE_TYPEINFO(TYPE, Q_MOVABLE_TYPE); \
template <> inline void qSwap<TYPE>(TYPE &value1, TYPE &value2) \
{ value1.swap(value2); } \
Q_DECLARE_SHARED_STL(TYPE)

/*
   QTypeInfo primitive specializations
*/
Q_DECLARE_TYPEINFO(bool, Q_PRIMITIVE_TYPE);
Q_DECLARE_TYPEINFO(char, Q_PRIMITIVE_TYPE);
Q_DECLARE_TYPEINFO(signed char, Q_PRIMITIVE_TYPE);
Q_DECLARE_TYPEINFO(uchar, Q_PRIMITIVE_TYPE);
Q_DECLARE_TYPEINFO(short, Q_PRIMITIVE_TYPE);
Q_DECLARE_TYPEINFO(ushort, Q_PRIMITIVE_TYPE);
Q_DECLARE_TYPEINFO(int, Q_PRIMITIVE_TYPE);
Q_DECLARE_TYPEINFO(uint, Q_PRIMITIVE_TYPE);
Q_DECLARE_TYPEINFO(long, Q_PRIMITIVE_TYPE);
Q_DECLARE_TYPEINFO(ulong, Q_PRIMITIVE_TYPE);
Q_DECLARE_TYPEINFO(qint64, Q_PRIMITIVE_TYPE);
Q_DECLARE_TYPEINFO(quint64, Q_PRIMITIVE_TYPE);
Q_DECLARE_TYPEINFO(float, Q_PRIMITIVE_TYPE);
Q_DECLARE_TYPEINFO(double, Q_PRIMITIVE_TYPE);
#ifndef Q_OS_DARWIN
Q_DECLARE_TYPEINFO(long double, Q_PRIMITIVE_TYPE);
#endif

#if QT_VERSION >= QT_VERSION_CHECK(6,0,0)
// We can't do it now because it would break BC on QList<char32_t>
Q_DECLARE_TYPEINFO(char16_t, Q_PRIMITIVE_TYPE);
Q_DECLARE_TYPEINFO(char32_t, Q_PRIMITIVE_TYPE);
#  if !defined(Q_CC_MSVC) || defined(_NATIVE_WCHAR_T_DEFINED)
Q_DECLARE_TYPEINFO(wchar_t, Q_PRIMITIVE_TYPE);
#  endif
#endif // Qt 6

QT_END_NAMESPACE
#endif // QTYPEINFO_H
