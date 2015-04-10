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

#include "qplatformdefs.h"
#include "qstring.h"
#include "qvector.h"
#include "qlist.h"
#include "qthreadstorage.h"
#include "qdir.h"
#include "qdatetime.h"

#ifndef QT_NO_QOBJECT
#include <private/qthread_p.h>
#endif

#include <stdlib.h>
#include <limits.h>
#include <stdarg.h>
#include <string.h>

#ifndef QT_NO_EXCEPTIONS
#  include <string>
#  include <exception>
#endif

#if !defined(Q_OS_WINCE)
#  include <errno.h>
#  if defined(Q_CC_MSVC)
#    include <crtdbg.h>
#  endif
#endif

#if defined(Q_OS_VXWORKS) && defined(_WRS_KERNEL)
#  include <envLib.h>
#endif

#if defined(Q_OS_MACX)
#include <CoreServices/CoreServices.h>
#endif

#if defined(Q_OS_ANDROID) && !defined(Q_OS_ANDROID_NO_SDK)
#include <private/qjni_p.h>
#endif

QT_BEGIN_NAMESPACE

#if !QT_DEPRECATED_SINCE(5, 0)
// Make sure they're defined to be exported
Q_CORE_EXPORT void *qMemCopy(void *dest, const void *src, size_t n);
Q_CORE_EXPORT void *qMemSet(void *dest, int c, size_t n);
#endif

// Statically check assumptions about the environment we're running
// in. The idea here is to error or warn if otherwise implicit Qt
// assumptions are not fulfilled on new hardware or compilers
// (if this list becomes too long, consider factoring into a separate file)
Q_STATIC_ASSERT_X(sizeof(int) == 4, "Qt assumes that int is 32 bits");
Q_STATIC_ASSERT_X(UCHAR_MAX == 255, "Qt assumes that char is 8 bits");


/*!
    \class QFlag
    \inmodule QtCore
    \brief The QFlag class is a helper data type for QFlags.

    It is equivalent to a plain \c int, except with respect to
    function overloading and type conversions. You should never need
    to use this class in your applications.

    \sa QFlags
*/

/*!
    \fn QFlag::QFlag(int value)

    Constructs a QFlag object that stores the given \a value.
*/

/*!
    \fn QFlag::QFlag(uint value)
    \since Qt 5.3

    Constructs a QFlag object that stores the given \a value.
*/

/*!
    \fn QFlag::QFlag(short value)
    \since 5.3

    Constructs a QFlag object that stores the given \a value.
*/

/*!
    \fn QFlag::QFlag(ushort value)
    \since Qt 5.3

    Constructs a QFlag object that stores the given \a value.
*/

/*!
    \fn QFlag::operator int() const

    Returns the value stored by the QFlag object.
*/

/*!
    \fn QFlag::operator uint() const
    \since Qt 5.3

    Returns the value stored by the QFlag object.
*/

/*!
    \class QFlags
    \inmodule QtCore
    \brief The QFlags class provides a type-safe way of storing
    OR-combinations of enum values.


    \ingroup tools

    The QFlags<Enum> class is a template class, where Enum is an enum
    type. QFlags is used throughout Qt for storing combinations of
    enum values.

    The traditional C++ approach for storing OR-combinations of enum
    values is to use an \c int or \c uint variable. The inconvenience
    with this approach is that there's no type checking at all; any
    enum value can be OR'd with any other enum value and passed on to
    a function that takes an \c int or \c uint.

    Qt uses QFlags to provide type safety. For example, the
    Qt::Alignment type is simply a typedef for
    QFlags<Qt::AlignmentFlag>. QLabel::setAlignment() takes a
    Qt::Alignment parameter, which means that any combination of
    Qt::AlignmentFlag values, or 0, is legal:

    \snippet code/src_corelib_global_qglobal.cpp 0

    If you try to pass a value from another enum or just a plain
    integer other than 0, the compiler will report an error. If you
    need to cast integer values to flags in a untyped fashion, you can
    use the explicit QFlags constructor as cast operator.

    If you want to use QFlags for your own enum types, use
    the Q_DECLARE_FLAGS() and Q_DECLARE_OPERATORS_FOR_FLAGS().

    Example:

    \snippet code/src_corelib_global_qglobal.cpp 1

    You can then use the \c MyClass::Options type to store
    combinations of \c MyClass::Option values.

    \section1 Flags and the Meta-Object System

    The Q_DECLARE_FLAGS() macro does not expose the flags to the meta-object
    system, so they cannot be used by Qt Script or edited in Qt Designer.
    To make the flags available for these purposes, the Q_FLAGS() macro must
    be used:

    \snippet code/src_corelib_global_qglobal.cpp meta-object flags

    \section1 Naming Convention

    A sensible naming convention for enum types and associated QFlags
    types is to give a singular name to the enum type (e.g., \c
    Option) and a plural name to the QFlags type (e.g., \c Options).
    When a singular name is desired for the QFlags type (e.g., \c
    Alignment), you can use \c Flag as the suffix for the enum type
    (e.g., \c AlignmentFlag).

    \sa QFlag
*/

/*!
    \typedef QFlags::Int
    \since 5.0

    Typedef for the integer type used for storage as well as for
    implicit conversion. Either \c int or \c{unsigned int}, depending
    on whether the enum's underlying type is signed or unsigned.
*/

/*!
    \typedef QFlags::enum_type

    Typedef for the Enum template type.
*/

/*!
    \fn QFlags::QFlags(const QFlags &other)

    Constructs a copy of \a other.
*/

/*!
    \fn QFlags::QFlags(Enum flag)

    Constructs a QFlags object storing the given \a flag.
*/

/*!
    \fn QFlags::QFlags(Zero zero)

    Constructs a QFlags object with no flags set. \a zero must be a
    literal 0 value.
*/

/*!
    \fn QFlags::QFlags(QFlag value)

    Constructs a QFlags object initialized with the given integer \a
    value.

    The QFlag type is a helper type. By using it here instead of \c
    int, we effectively ensure that arbitrary enum values cannot be
    cast to a QFlags, whereas untyped enum values (i.e., \c int
    values) can.
*/

/*!
    \fn QFlags &QFlags::operator=(const QFlags &other)

    Assigns \a other to this object and returns a reference to this
    object.
*/

/*!
    \fn QFlags &QFlags::operator&=(int mask)

    Performs a bitwise AND operation with \a mask and stores the
    result in this QFlags object. Returns a reference to this object.

    \sa operator&(), operator|=(), operator^=()
*/

/*!
    \fn QFlags &QFlags::operator&=(uint mask)

    \overload
*/

/*!
    \fn QFlags &QFlags::operator&=(Enum mask)

    \overload
*/

/*!
    \fn QFlags &QFlags::operator|=(QFlags other)

    Performs a bitwise OR operation with \a other and stores the
    result in this QFlags object. Returns a reference to this object.

    \sa operator|(), operator&=(), operator^=()
*/

/*!
    \fn QFlags &QFlags::operator|=(Enum other)

    \overload
*/

/*!
    \fn QFlags &QFlags::operator^=(QFlags other)

    Performs a bitwise XOR operation with \a other and stores the
    result in this QFlags object. Returns a reference to this object.

    \sa operator^(), operator&=(), operator|=()
*/

/*!
    \fn QFlags &QFlags::operator^=(Enum other)

    \overload
*/

/*!
    \fn QFlags::operator Int() const

    Returns the value stored in the QFlags object as an integer.

    \sa Int
*/

/*!
    \fn QFlags QFlags::operator|(QFlags other) const

    Returns a QFlags object containing the result of the bitwise OR
    operation on this object and \a other.

    \sa operator|=(), operator^(), operator&(), operator~()
*/

/*!
    \fn QFlags QFlags::operator|(Enum other) const

    \overload
*/

/*!
    \fn QFlags QFlags::operator^(QFlags other) const

    Returns a QFlags object containing the result of the bitwise XOR
    operation on this object and \a other.

    \sa operator^=(), operator&(), operator|(), operator~()
*/

/*!
    \fn QFlags QFlags::operator^(Enum other) const

    \overload
*/

/*!
    \fn QFlags QFlags::operator&(int mask) const

    Returns a QFlags object containing the result of the bitwise AND
    operation on this object and \a mask.

    \sa operator&=(), operator|(), operator^(), operator~()
*/

/*!
    \fn QFlags QFlags::operator&(uint mask) const

    \overload
*/

/*!
    \fn QFlags QFlags::operator&(Enum mask) const

    \overload
*/

/*!
    \fn QFlags QFlags::operator~() const

    Returns a QFlags object that contains the bitwise negation of
    this object.

    \sa operator&(), operator|(), operator^()
*/

/*!
    \fn bool QFlags::operator!() const

    Returns \c true if no flag is set (i.e., if the value stored by the
    QFlags object is 0); otherwise returns \c false.
*/

/*!
    \fn bool QFlags::testFlag(Enum flag) const
    \since 4.2

    Returns \c true if the \a flag is set, otherwise \c false.
*/

/*!
  \macro Q_DISABLE_COPY(Class)
  \relates QObject

  Disables the use of copy constructors and assignment operators
  for the given \a Class.

  Instances of subclasses of QObject should not be thought of as
  values that can be copied or assigned, but as unique identities.
  This means that when you create your own subclass of QObject
  (director or indirect), you should \e not give it a copy constructor
  or an assignment operator.  However, it may not enough to simply
  omit them from your class, because, if you mistakenly write some code
  that requires a copy constructor or an assignment operator (it's easy
  to do), your compiler will thoughtfully create it for you. You must
  do more.

  The curious user will have seen that the Qt classes derived
  from QObject typically include this macro in a private section:

  \snippet code/src_corelib_global_qglobal.cpp 43

  It declares a copy constructor and an assignment operator in the
  private section, so that if you use them by mistake, the compiler
  will report an error.

  \snippet code/src_corelib_global_qglobal.cpp 44

  But even this might not catch absolutely every case. You might be
  tempted to do something like this:

  \snippet code/src_corelib_global_qglobal.cpp 45

  First of all, don't do that. Most compilers will generate code that
  uses the copy constructor, so the privacy violation error will be
  reported, but your C++ compiler is not required to generate code for
  this statement in a specific way. It could generate code using
  \e{neither} the copy constructor \e{nor} the assignment operator we
  made private. In that case, no error would be reported, but your
  application would probably crash when you called a member function
  of \c{w}.
*/

/*!
    \macro Q_DECLARE_FLAGS(Flags, Enum)
    \relates QFlags

    The Q_DECLARE_FLAGS() macro expands to

    \snippet code/src_corelib_global_qglobal.cpp 2

    \a Enum is the name of an existing enum type, whereas \a Flags is
    the name of the QFlags<\e{Enum}> typedef.

    See the QFlags documentation for details.

    \sa Q_DECLARE_OPERATORS_FOR_FLAGS()
*/

/*!
    \macro Q_DECLARE_OPERATORS_FOR_FLAGS(Flags)
    \relates QFlags

    The Q_DECLARE_OPERATORS_FOR_FLAGS() macro declares global \c
    operator|() functions for \a Flags, which is of type QFlags<T>.

    See the QFlags documentation for details.

    \sa Q_DECLARE_FLAGS()
*/

/*!
    \headerfile <QtGlobal>
    \title Global Qt Declarations
    \ingroup funclists

    \brief The <QtGlobal> header file includes the fundamental global
    declarations. It is included by most other Qt header files.

    The global declarations include \l{types}, \l{functions} and
    \l{macros}.

    The type definitions are partly convenience definitions for basic
    types (some of which guarantee certain bit-sizes on all platforms
    supported by Qt), partly types related to Qt message handling. The
    functions are related to generating messages, Qt version handling
    and comparing and adjusting object values. And finally, some of
    the declared macros enable programmers to add compiler or platform
    specific code to their applications, while others are convenience
    macros for larger operations.

    \section1 Types

    The header file declares several type definitions that guarantee a
    specified bit-size on all platforms supported by Qt for various
    basic types, for example \l qint8 which is a signed char
    guaranteed to be 8-bit on all platforms supported by Qt. The
    header file also declares the \l qlonglong type definition for \c
    {long long int } (\c __int64 on Windows).

    Several convenience type definitions are declared: \l qreal for \c
    double, \l uchar for \c unsigned char, \l uint for \c unsigned
    int, \l ulong for \c unsigned long and \l ushort for \c unsigned
    short.

    Finally, the QtMsgType definition identifies the various messages
    that can be generated and sent to a Qt message handler;
    QtMessageHandler is a type definition for a pointer to a function with
    the signature
    \c {void myMessageHandler(QtMsgType, const QMessageLogContext &, const char *)}.
    QMessageLogContext class contains the line, file, and function the
    message was logged at. This information is created by the QMessageLogger
    class.

    \section1 Functions

    The <QtGlobal> header file contains several functions comparing
    and adjusting an object's value. These functions take a template
    type as argument: You can retrieve the absolute value of an object
    using the qAbs() function, and you can bound a given object's
    value by given minimum and maximum values using the qBound()
    function. You can retrieve the minimum and maximum of two given
    objects using qMin() and qMax() respectively. All these functions
    return a corresponding template type; the template types can be
    replaced by any other type.

    Example:

    \snippet code/src_corelib_global_qglobal.cpp 3

    <QtGlobal> also contains functions that generate messages from the
    given string argument: qCritical(), qDebug(), qFatal() and
    qWarning(). These functions call the message handler with the
    given message.

    Example:

    \snippet code/src_corelib_global_qglobal.cpp 4

    The remaining functions are qRound() and qRound64(), which both
    accept a \l qreal value as their argument returning the value
    rounded up to the nearest integer and 64-bit integer respectively,
    the qInstallMessageHandler() function which installs the given
    QtMessageHandler, and the qVersion() function which returns the
    version number of Qt at run-time as a string.

    \section1 Macros

    The <QtGlobal> header file provides a range of macros (Q_CC_*)
    that are defined if the application is compiled using the
    specified platforms. For example, the Q_CC_SUN macro is defined if
    the application is compiled using Forte Developer, or Sun Studio
    C++.  The header file also declares a range of macros (Q_OS_*)
    that are defined for the specified platforms. For example,
    Q_OS_UNIX which is defined for the Unix-based systems.

    The purpose of these macros is to enable programmers to add
    compiler or platform specific code to their application.

    The remaining macros are convenience macros for larger operations:
    The QT_TRANSLATE_NOOP() and QT_TR_NOOP() macros provide the
    possibility of marking text for dynamic translation,
    i.e. translation without changing the stored source text. The
    Q_ASSERT() and Q_ASSERT_X() enables warning messages of various
    level of refinement. The Q_FOREACH() and foreach() macros
    implement Qt's foreach loop.

    The Q_INT64_C() and Q_UINT64_C() macros wrap signed and unsigned
    64-bit integer literals in a platform-independent way. The
    Q_CHECK_PTR() macro prints a warning containing the source code's
    file name and line number, saying that the program ran out of
    memory, if the pointer is 0. The qPrintable() macro represent an
    easy way of printing text.

    Finally, the QT_POINTER_SIZE macro expands to the size of a
    pointer in bytes, and the QT_VERSION and QT_VERSION_STR macros
    expand to a numeric value or a string, respectively, specifying
    Qt's version number, i.e the version the application is compiled
    against.

    \sa <QtAlgorithms>, QSysInfo
*/

/*!
    \typedef qreal
    \relates <QtGlobal>

    Typedef for \c double unless Qt is configured with the
    \c{-qreal float} option.
*/

/*! \typedef uchar
    \relates <QtGlobal>

    Convenience typedef for \c{unsigned char}.
*/

/*! \typedef ushort
    \relates <QtGlobal>

    Convenience typedef for \c{unsigned short}.
*/

/*! \typedef uint
    \relates <QtGlobal>

    Convenience typedef for \c{unsigned int}.
*/

/*! \typedef ulong
    \relates <QtGlobal>

    Convenience typedef for \c{unsigned long}.
*/

/*! \typedef qint8
    \relates <QtGlobal>

    Typedef for \c{signed char}. This type is guaranteed to be 8-bit
    on all platforms supported by Qt.
*/

/*!
    \typedef quint8
    \relates <QtGlobal>

    Typedef for \c{unsigned char}. This type is guaranteed to
    be 8-bit on all platforms supported by Qt.
*/

/*! \typedef qint16
    \relates <QtGlobal>

    Typedef for \c{signed short}. This type is guaranteed to be
    16-bit on all platforms supported by Qt.
*/

/*!
    \typedef quint16
    \relates <QtGlobal>

    Typedef for \c{unsigned short}. This type is guaranteed to
    be 16-bit on all platforms supported by Qt.
*/

/*! \typedef qint32
    \relates <QtGlobal>

    Typedef for \c{signed int}. This type is guaranteed to be 32-bit
    on all platforms supported by Qt.
*/

/*!
    \typedef quint32
    \relates <QtGlobal>

    Typedef for \c{unsigned int}. This type is guaranteed to
    be 32-bit on all platforms supported by Qt.
*/

/*! \typedef qint64
    \relates <QtGlobal>

    Typedef for \c{long long int} (\c __int64 on Windows). This type
    is guaranteed to be 64-bit on all platforms supported by Qt.

    Literals of this type can be created using the Q_INT64_C() macro:

    \snippet code/src_corelib_global_qglobal.cpp 5

    \sa Q_INT64_C(), quint64, qlonglong
*/

/*!
    \typedef quint64
    \relates <QtGlobal>

    Typedef for \c{unsigned long long int} (\c{unsigned __int64} on
    Windows). This type is guaranteed to be 64-bit on all platforms
    supported by Qt.

    Literals of this type can be created using the Q_UINT64_C()
    macro:

    \snippet code/src_corelib_global_qglobal.cpp 6

    \sa Q_UINT64_C(), qint64, qulonglong
*/

/*!
    \typedef qintptr
    \relates <QtGlobal>

    Integral type for representing pointers in a signed integer (useful for
    hashing, etc.).

    Typedef for either qint32 or qint64. This type is guaranteed to
    be the same size as a pointer on all platforms supported by Qt. On
    a system with 32-bit pointers, qintptr is a typedef for qint32;
    on a system with 64-bit pointers, qintptr is a typedef for
    qint64.

    Note that qintptr is signed. Use quintptr for unsigned values.

    \sa qptrdiff, qint32, qint64
*/

/*!
    \typedef quintptr
    \relates <QtGlobal>

    Integral type for representing pointers in an unsigned integer (useful for
    hashing, etc.).

    Typedef for either quint32 or quint64. This type is guaranteed to
    be the same size as a pointer on all platforms supported by Qt. On
    a system with 32-bit pointers, quintptr is a typedef for quint32;
    on a system with 64-bit pointers, quintptr is a typedef for
    quint64.

    Note that quintptr is unsigned. Use qptrdiff for signed values.

    \sa qptrdiff, quint32, quint64
*/

/*!
    \typedef qptrdiff
    \relates <QtGlobal>

    Integral type for representing pointer differences.

    Typedef for either qint32 or qint64. This type is guaranteed to be
    the same size as a pointer on all platforms supported by Qt. On a
    system with 32-bit pointers, quintptr is a typedef for quint32; on
    a system with 64-bit pointers, quintptr is a typedef for quint64.

    Note that qptrdiff is signed. Use quintptr for unsigned values.

    \sa quintptr, qint32, qint64
*/

/*!
    \enum QtMsgType
    \relates <QtGlobal>

    This enum describes the messages that can be sent to a message
    handler (QtMessageHandler). You can use the enum to identify and
    associate the various message types with the appropriate
    actions.

    \value QtDebugMsg
           A message generated by the qDebug() function.
    \value QtWarningMsg
           A message generated by the qWarning() function.
    \value QtCriticalMsg
           A message generated by the qCritical() function.
    \value QtFatalMsg
           A message generated by the qFatal() function.
    \value QtSystemMsg


    \sa QtMessageHandler, qInstallMessageHandler()
*/

/*! \typedef QFunctionPointer
    \relates <QtGlobal>

    This is a typedef for \c{void (*)()}, a pointer to a function that takes
    no arguments and returns void.
*/

/*! \macro qint64 Q_INT64_C(literal)
    \relates <QtGlobal>

    Wraps the signed 64-bit integer \a literal in a
    platform-independent way.

    Example:

    \snippet code/src_corelib_global_qglobal.cpp 8

    \sa qint64, Q_UINT64_C()
*/

/*! \macro quint64 Q_UINT64_C(literal)
    \relates <QtGlobal>

    Wraps the unsigned 64-bit integer \a literal in a
    platform-independent way.

    Example:

    \snippet code/src_corelib_global_qglobal.cpp 9

    \sa quint64, Q_INT64_C()
*/

/*! \typedef qlonglong
    \relates <QtGlobal>

    Typedef for \c{long long int} (\c __int64 on Windows). This is
    the same as \l qint64.

    \sa qulonglong, qint64
*/

/*!
    \typedef qulonglong
    \relates <QtGlobal>

    Typedef for \c{unsigned long long int} (\c{unsigned __int64} on
    Windows). This is the same as \l quint64.

    \sa quint64, qlonglong
*/

/*! \fn T qAbs(const T &value)
    \relates <QtGlobal>

    Compares \a value to the 0 of type T and returns the absolute
    value. Thus if T is \e {double}, then \a value is compared to
    \e{(double) 0}.

    Example:

    \snippet code/src_corelib_global_qglobal.cpp 10
*/

/*! \fn int qRound(qreal value)
    \relates <QtGlobal>

    Rounds \a value to the nearest integer.

    Example:

    \snippet code/src_corelib_global_qglobal.cpp 11
*/

/*! \fn qint64 qRound64(qreal value)
    \relates <QtGlobal>

    Rounds \a value to the nearest 64-bit integer.

    Example:

    \snippet code/src_corelib_global_qglobal.cpp 12
*/

/*! \fn const T &qMin(const T &value1, const T &value2)
    \relates <QtGlobal>

    Returns the minimum of \a value1 and \a value2.

    Example:

    \snippet code/src_corelib_global_qglobal.cpp 13

    \sa qMax(), qBound()
*/

/*! \fn const T &qMax(const T &value1, const T &value2)
    \relates <QtGlobal>

    Returns the maximum of \a value1 and \a value2.

    Example:

    \snippet code/src_corelib_global_qglobal.cpp 14

    \sa qMin(), qBound()
*/

/*! \fn const T &qBound(const T &min, const T &value, const T &max)
    \relates <QtGlobal>

    Returns \a value bounded by \a min and \a max. This is equivalent
    to qMax(\a min, qMin(\a value, \a max)).

    Example:

    \snippet code/src_corelib_global_qglobal.cpp 15

    \sa qMin(), qMax()
*/

/*!
    \macro QT_VERSION_CHECK
    \relates <QtGlobal>

    Turns the major, minor and patch numbers of a version into an
    integer, 0xMMNNPP (MM = major, NN = minor, PP = patch). This can
    be compared with another similarly processed version id.

    Example:

    \snippet code/src_corelib_global_qglobal.cpp qt-version-check

    \sa QT_VERSION
*/

/*!
    \macro QT_VERSION
    \relates <QtGlobal>

    This macro expands a numeric value of the form 0xMMNNPP (MM =
    major, NN = minor, PP = patch) that specifies Qt's version
    number. For example, if you compile your application against Qt
    4.1.2, the QT_VERSION macro will expand to 0x040102.

    You can use QT_VERSION to use the latest Qt features where
    available.

    Example:

    \snippet code/src_corelib_global_qglobal.cpp 16

    \sa QT_VERSION_STR, qVersion()
*/

/*!
    \macro QT_VERSION_STR
    \relates <QtGlobal>

    This macro expands to a string that specifies Qt's version number
    (for example, "4.1.2"). This is the version against which the
    application is compiled.

    \sa qVersion(), QT_VERSION
*/

/*!
    \relates <QtGlobal>

    Returns the version number of Qt at run-time as a string (for
    example, "4.1.2"). This may be a different version than the
    version the application was compiled against.

    \sa QT_VERSION_STR
*/

const char *qVersion() Q_DECL_NOTHROW
{
    return QT_VERSION_STR;
}

bool qSharedBuild() Q_DECL_NOTHROW
{
#ifdef QT_SHARED
    return true;
#else
    return false;
#endif
}

/*****************************************************************************
  System detection routines
 *****************************************************************************/

/*!
    \class QSysInfo
    \inmodule QtCore
    \brief The QSysInfo class provides information about the system.

    \list
    \li \l WordSize specifies the size of a pointer for the platform
       on which the application is compiled.
    \li \l ByteOrder specifies whether the platform is big-endian or
       little-endian.
    \li \l WindowsVersion specifies the version of the Windows operating
       system on which the application is run (Windows only)
    \li \l MacintoshVersion specifies the version of the Macintosh
       operating system on which the application is run (Mac only).
    \endlist

    Some constants are defined only on certain platforms. You can use
    the preprocessor symbols Q_OS_WIN and Q_OS_OSX to test that
    the application is compiled under Windows or OS X.

    \sa QLibraryInfo
*/

/*!
    \enum QSysInfo::Sizes

    This enum provides platform-specific information about the sizes of data
    structures used by the underlying architecture.

    \value WordSize The size in bits of a pointer for the platform on which
           the application is compiled (32 or 64).
*/

/*!
    \variable QSysInfo::WindowsVersion
    \brief the version of the Windows operating system on which the
           application is run (Windows only)
*/

/*!
    \fn QSysInfo::WindowsVersion QSysInfo::windowsVersion()
    \since 4.4

    Returns the version of the Windows operating system on which the
    application is run (Windows only).
*/

/*!
    \variable QSysInfo::MacintoshVersion
    \brief the version of the Macintosh operating system on which
           the application is run (Mac only).
*/

/*!
    \fn QSysInfo::MacVersion QSysInfo::macVersion()

    Returns the version of Darwin (OS X or iOS) on which the application is run.
*/

/*!
    \enum QSysInfo::Endian

    \value BigEndian  Big-endian byte order (also called Network byte order)
    \value LittleEndian  Little-endian byte order
    \value ByteOrder  Equals BigEndian or LittleEndian, depending on
                      the platform's byte order.
*/

/*!
    \enum QSysInfo::WinVersion

    This enum provides symbolic names for the various versions of the
    Windows operating system. On Windows, the
    QSysInfo::WindowsVersion variable gives the version of the system
    on which the application is run.

    MS-DOS-based versions:

    \value WV_32s   Windows 3.1 with Win 32s
    \value WV_95    Windows 95
    \value WV_98    Windows 98
    \value WV_Me    Windows Me

    NT-based versions (note that each operating system version is only represented once rather than each Windows edition):

    \value WV_NT    Windows NT (operating system version 4.0)
    \value WV_2000  Windows 2000 (operating system version 5.0)
    \value WV_XP    Windows XP (operating system version 5.1)
    \value WV_2003  Windows Server 2003, Windows Server 2003 R2, Windows Home Server, Windows XP Professional x64 Edition (operating system version 5.2)
    \value WV_VISTA Windows Vista, Windows Server 2008 (operating system version 6.0)
    \value WV_WINDOWS7 Windows 7, Windows Server 2008 R2 (operating system version 6.1)
    \value WV_WINDOWS8 Windows 8 (operating system version 6.2)
    \value WV_WINDOWS8_1 Windows 8.1 (operating system version 6.3), introduced in Qt 5.2

    Alternatively, you may use the following macros which correspond directly to the Windows operating system version number:

    \value WV_4_0   Operating system version 4.0, corresponds to Windows NT
    \value WV_5_0   Operating system version 5.0, corresponds to Windows 2000
    \value WV_5_1   Operating system version 5.1, corresponds to Windows XP
    \value WV_5_2   Operating system version 5.2, corresponds to Windows Server 2003, Windows Server 2003 R2, Windows Home Server, and Windows XP Professional x64 Edition
    \value WV_6_0   Operating system version 6.0, corresponds to Windows Vista and Windows Server 2008
    \value WV_6_1   Operating system version 6.1, corresponds to Windows 7 and Windows Server 2008 R2
    \value WV_6_2   Operating system version 6.2, corresponds to Windows 8
    \value WV_6_3   Operating system version 6.3, corresponds to Windows 8.1, introduced in Qt 5.2

    CE-based versions:

    \value WV_CE    Windows CE
    \value WV_CENET Windows CE .NET
    \value WV_CE_5  Windows CE 5.x
    \value WV_CE_6  Windows CE 6.x

    The following masks can be used for testing whether a Windows
    version is MS-DOS-based, NT-based, or CE-based:

    \value WV_DOS_based MS-DOS-based version of Windows
    \value WV_NT_based  NT-based version of Windows
    \value WV_CE_based  CE-based version of Windows

    \sa MacVersion
*/

/*!
    \enum QSysInfo::MacVersion

    This enum provides symbolic names for the various versions of the
    Darwin operating system, covering both OS X and iOS. The
    QSysInfo::MacintoshVersion variable gives the version of the
    system on which the application is run.

    \value MV_9        Mac OS 9 (unsupported)
    \value MV_10_0     Mac OS X 10.0 (unsupported)
    \value MV_10_1     Mac OS X 10.1 (unsupported)
    \value MV_10_2     Mac OS X 10.2 (unsupported)
    \value MV_10_3     Mac OS X 10.3 (unsupported)
    \value MV_10_4     Mac OS X 10.4 (unsupported)
    \value MV_10_5     Mac OS X 10.5 (unsupported)
    \value MV_10_6     Mac OS X 10.6
    \value MV_10_7     OS X 10.7
    \value MV_10_8     OS X 10.8
    \value MV_10_9     OS X 10.9
    \value MV_Unknown  An unknown and currently unsupported platform

    \value MV_CHEETAH  Apple codename for MV_10_0
    \value MV_PUMA     Apple codename for MV_10_1
    \value MV_JAGUAR   Apple codename for MV_10_2
    \value MV_PANTHER  Apple codename for MV_10_3
    \value MV_TIGER    Apple codename for MV_10_4
    \value MV_LEOPARD  Apple codename for MV_10_5
    \value MV_SNOWLEOPARD  Apple codename for MV_10_6
    \value MV_LION     Apple codename for MV_10_7
    \value MV_MOUNTAINLION Apple codename for MV_10_8
    \value MV_MAVERICKS    Apple codename for MV_10_9

    \value MV_IOS      iOS (any)
    \value MV_IOS_4_3  iOS 4.3
    \value MV_IOS_5_0  iOS 5.0
    \value MV_IOS_5_1  iOS 5.1
    \value MV_IOS_6_0  iOS 6.0
    \value MV_IOS_6_1  iOS 6.1
    \value MV_IOS_7_0  iOS 7.0
    \value MV_IOS_7_1  iOS 7.1

    \sa WinVersion
*/

/*!
    \macro Q_OS_DARWIN
    \relates <QtGlobal>

    Defined on Darwin-based operating systems such as OS X and iOS,
    including any open source version(s) of Darwin.
*/

/*!
    \macro Q_OS_MAC
    \relates <QtGlobal>

    Defined on Darwin-based operating systems distributed by Apple, which
    currently includes OS X and iOS, but not the open source version.
 */

/*!
    \macro Q_OS_OSX
    \relates <QtGlobal>

    Defined on OS X.
 */

/*!
    \macro Q_OS_IOS
    \relates <QtGlobal>

    Defined on iOS.
 */

/*!
    \macro Q_OS_WIN
    \relates <QtGlobal>

    Defined on all supported versions of Windows. That is, if
    \l Q_OS_WIN32, \l Q_OS_WIN64 or \l Q_OS_WINCE is defined.
*/

/*!
    \macro Q_OS_WIN32
    \relates <QtGlobal>

    Defined on 32-bit and 64-bit versions of Windows (not on Windows CE).
*/

/*!
    \macro Q_OS_WIN64
    \relates <QtGlobal>

    Defined on 64-bit versions of Windows.
*/

/*!
    \macro Q_OS_WINCE
    \relates <QtGlobal>

    Defined on Windows CE.
*/

/*!
    \macro Q_OS_WINRT
    \relates <QtGlobal>

    Defined for Windows Runtime (Windows Store apps) on Windows 8, Windows RT,
    and Windows Phone 8.
*/

/*!
    \macro Q_OS_WINPHONE
    \relates <QtGlobal>

    Defined on Windows Phone 8.
*/

/*!
    \macro Q_OS_CYGWIN
    \relates <QtGlobal>

    Defined on Cygwin.
*/

/*!
    \macro Q_OS_SOLARIS
    \relates <QtGlobal>

    Defined on Sun Solaris.
*/

/*!
    \macro Q_OS_HPUX
    \relates <QtGlobal>

    Defined on HP-UX.
*/

/*!
    \macro Q_OS_ULTRIX
    \relates <QtGlobal>

    Defined on DEC Ultrix.
*/

/*!
    \macro Q_OS_LINUX
    \relates <QtGlobal>

    Defined on Linux.
*/

/*!
    \macro Q_OS_ANDROID
    \relates <QtGlobal>

    Defined on Android.
*/

/*!
    \macro Q_OS_FREEBSD
    \relates <QtGlobal>

    Defined on FreeBSD.
*/

/*!
    \macro Q_OS_NETBSD
    \relates <QtGlobal>

    Defined on NetBSD.
*/

/*!
    \macro Q_OS_OPENBSD
    \relates <QtGlobal>

    Defined on OpenBSD.
*/

/*!
    \macro Q_OS_BSDI
    \relates <QtGlobal>

    Defined on BSD/OS.
*/

/*!
    \macro Q_OS_IRIX
    \relates <QtGlobal>

    Defined on SGI Irix.
*/

/*!
    \macro Q_OS_OSF
    \relates <QtGlobal>

    Defined on HP Tru64 UNIX.
*/

/*!
    \macro Q_OS_SCO
    \relates <QtGlobal>

    Defined on SCO OpenServer 5.
*/

/*!
    \macro Q_OS_UNIXWARE
    \relates <QtGlobal>

    Defined on UnixWare 7, Open UNIX 8.
*/

/*!
    \macro Q_OS_AIX
    \relates <QtGlobal>

    Defined on AIX.
*/

/*!
    \macro Q_OS_HURD
    \relates <QtGlobal>

    Defined on GNU Hurd.
*/

/*!
    \macro Q_OS_DGUX
    \relates <QtGlobal>

    Defined on DG/UX.
*/

/*!
    \macro Q_OS_RELIANT
    \relates <QtGlobal>

    Defined on Reliant UNIX.
*/

/*!
    \macro Q_OS_DYNIX
    \relates <QtGlobal>

    Defined on DYNIX/ptx.
*/

/*!
    \macro Q_OS_QNX
    \relates <QtGlobal>

    Defined on QNX Neutrino.
*/

/*!
    \macro Q_OS_LYNX
    \relates <QtGlobal>

    Defined on LynxOS.
*/

/*!
    \macro Q_OS_BSD4
    \relates <QtGlobal>

    Defined on Any BSD 4.4 system.
*/

/*!
    \macro Q_OS_UNIX
    \relates <QtGlobal>

    Defined on Any UNIX BSD/SYSV system.
*/

/*!
    \macro Q_CC_SYM
    \relates <QtGlobal>

    Defined if the application is compiled using Digital Mars C/C++
    (used to be Symantec C++).
*/

/*!
    \macro Q_CC_MSVC
    \relates <QtGlobal>

    Defined if the application is compiled using Microsoft Visual
    C/C++, Intel C++ for Windows.
*/

/*!
    \macro Q_CC_BOR
    \relates <QtGlobal>

    Defined if the application is compiled using Borland/Turbo C++.
*/

/*!
    \macro Q_CC_WAT
    \relates <QtGlobal>

    Defined if the application is compiled using Watcom C++.
*/

/*!
    \macro Q_CC_GNU
    \relates <QtGlobal>

    Defined if the application is compiled using GNU C++.
*/

/*!
    \macro Q_CC_COMEAU
    \relates <QtGlobal>

    Defined if the application is compiled using Comeau C++.
*/

/*!
    \macro Q_CC_EDG
    \relates <QtGlobal>

    Defined if the application is compiled using Edison Design Group
    C++.
*/

/*!
    \macro Q_CC_OC
    \relates <QtGlobal>

    Defined if the application is compiled using CenterLine C++.
*/

/*!
    \macro Q_CC_SUN
    \relates <QtGlobal>

    Defined if the application is compiled using Forte Developer, or
    Sun Studio C++.
*/

/*!
    \macro Q_CC_MIPS
    \relates <QtGlobal>

    Defined if the application is compiled using MIPSpro C++.
*/

/*!
    \macro Q_CC_DEC
    \relates <QtGlobal>

    Defined if the application is compiled using DEC C++.
*/

/*!
    \macro Q_CC_HPACC
    \relates <QtGlobal>

    Defined if the application is compiled using HP aC++.
*/

/*!
    \macro Q_CC_USLC
    \relates <QtGlobal>

    Defined if the application is compiled using SCO OUDK and UDK.
*/

/*!
    \macro Q_CC_CDS
    \relates <QtGlobal>

    Defined if the application is compiled using Reliant C++.
*/

/*!
    \macro Q_CC_KAI
    \relates <QtGlobal>

    Defined if the application is compiled using KAI C++.
*/

/*!
    \macro Q_CC_INTEL
    \relates <QtGlobal>

    Defined if the application is compiled using Intel C++ for Linux,
    Intel C++ for Windows.
*/

/*!
    \macro Q_CC_HIGHC
    \relates <QtGlobal>

    Defined if the application is compiled using MetaWare High C/C++.
*/

/*!
    \macro Q_CC_PGI
    \relates <QtGlobal>

    Defined if the application is compiled using Portland Group C++.
*/

/*!
    \macro Q_CC_GHS
    \relates <QtGlobal>

    Defined if the application is compiled using Green Hills
    Optimizing C++ Compilers.
*/

/*!
    \macro Q_PROCESSOR_ALPHA
    \relates <QtGlobal>

    Defined if the application is compiled for Alpha processors.
*/

/*!
    \macro Q_PROCESSOR_ARM
    \relates <QtGlobal>

    Defined if the application is compiled for ARM processors. Qt currently
    supports three optional ARM revisions: \l Q_PROCESSOR_ARM_V5, \l
    Q_PROCESSOR_ARM_V6, and \l Q_PROCESSOR_ARM_V7.
*/
/*!
    \macro Q_PROCESSOR_ARM_V5
    \relates <QtGlobal>

    Defined if the application is compiled for ARMv5 processors. The \l
    Q_PROCESSOR_ARM macro is also defined when Q_PROCESSOR_ARM_V5 is defined.
*/
/*!
    \macro Q_PROCESSOR_ARM_V6
    \relates <QtGlobal>

    Defined if the application is compiled for ARMv6 processors. The \l
    Q_PROCESSOR_ARM and \l Q_PROCESSOR_ARM_V5 macros are also defined when
    Q_PROCESSOR_ARM_V6 is defined.
*/
/*!
    \macro Q_PROCESSOR_ARM_V7
    \relates <QtGlobal>

    Defined if the application is compiled for ARMv7 processors. The \l
    Q_PROCESSOR_ARM, \l Q_PROCESSOR_ARM_V5, and \l Q_PROCESSOR_ARM_V6 macros
    are also defined when Q_PROCESSOR_ARM_V7 is defined.
*/

/*!
    \macro Q_PROCESSOR_AVR32
    \relates <QtGlobal>

    Defined if the application is compiled for AVR32 processors.
*/

/*!
    \macro Q_PROCESSOR_BLACKFIN
    \relates <QtGlobal>

    Defined if the application is compiled for Blackfin processors.
*/

/*!
    \macro Q_PROCESSOR_IA64
    \relates <QtGlobal>

    Defined if the application is compiled for IA-64 processors. This includes
    all Itanium and Itanium 2 processors.
*/

/*!
    \macro Q_PROCESSOR_MIPS
    \relates <QtGlobal>

    Defined if the application is compiled for MIPS processors. Qt currently
    supports seven MIPS revisions: \l Q_PROCESSOR_MIPS_I, \l
    Q_PROCESSOR_MIPS_II, \l Q_PROCESSOR_MIPS_III, \l Q_PROCESSOR_MIPS_IV, \l
    Q_PROCESSOR_MIPS_V, \l Q_PROCESSOR_MIPS_32, and \l Q_PROCESSOR_MIPS_64.
*/
/*!
    \macro Q_PROCESSOR_MIPS_I
    \relates <QtGlobal>

    Defined if the application is compiled for MIPS-I processors. The \l
    Q_PROCESSOR_MIPS macro is also defined when Q_PROCESSOR_MIPS_I is defined.
*/
/*!
    \macro Q_PROCESSOR_MIPS_II
    \relates <QtGlobal>

    Defined if the application is compiled for MIPS-II processors. The \l
    Q_PROCESSOR_MIPS and \l Q_PROCESSOR_MIPS_I macros are also defined when
    Q_PROCESSOR_MIPS_II is defined.
*/
/*!
    \macro Q_PROCESSOR_MIPS_32
    \relates <QtGlobal>

    Defined if the application is compiled for MIPS32 processors. The \l
    Q_PROCESSOR_MIPS, \l Q_PROCESSOR_MIPS_I, and \l Q_PROCESSOR_MIPS_II macros
    are also defined when Q_PROCESSOR_MIPS_32 is defined.
*/
/*!
    \macro Q_PROCESSOR_MIPS_III
    \relates <QtGlobal>

    Defined if the application is compiled for MIPS-III processors. The \l
    Q_PROCESSOR_MIPS, \l Q_PROCESSOR_MIPS_I, and \l Q_PROCESSOR_MIPS_II macros
    are also defined when Q_PROCESSOR_MIPS_III is defined.
*/
/*!
    \macro Q_PROCESSOR_MIPS_IV
    \relates <QtGlobal>

    Defined if the application is compiled for MIPS-IV processors. The \l
    Q_PROCESSOR_MIPS, \l Q_PROCESSOR_MIPS_I, \l Q_PROCESSOR_MIPS_II, and \l
    Q_PROCESSOR_MIPS_III macros are also defined when Q_PROCESSOR_MIPS_IV is
    defined.
*/
/*!
    \macro Q_PROCESSOR_MIPS_V
    \relates <QtGlobal>

    Defined if the application is compiled for MIPS-V processors. The \l
    Q_PROCESSOR_MIPS, \l Q_PROCESSOR_MIPS_I, \l Q_PROCESSOR_MIPS_II, \l
    Q_PROCESSOR_MIPS_III, and \l Q_PROCESSOR_MIPS_IV macros are also defined
    when Q_PROCESSOR_MIPS_V is defined.
*/
/*!
    \macro Q_PROCESSOR_MIPS_64
    \relates <QtGlobal>

    Defined if the application is compiled for MIPS64 processors. The \l
    Q_PROCESSOR_MIPS, \l Q_PROCESSOR_MIPS_I, \l Q_PROCESSOR_MIPS_II, \l
    Q_PROCESSOR_MIPS_III, \l Q_PROCESSOR_MIPS_IV, and \l Q_PROCESSOR_MIPS_V
    macros are also defined when Q_PROCESSOR_MIPS_64 is defined.
*/

/*!
    \macro Q_PROCESSOR_POWER
    \relates <QtGlobal>

    Defined if the application is compiled for POWER processors. Qt currently
    supports two Power variants: \l Q_PROCESSOR_POWER_32 and \l
    Q_PROCESSOR_POWER_64.
*/
/*!
    \macro Q_PROCESSOR_POWER_32
    \relates <QtGlobal>

    Defined if the application is compiled for 32-bit Power processors. The \l
    Q_PROCESSOR_POWER macro is also defined when Q_PROCESSOR_POWER_32 is
    defined.
*/
/*!
    \macro Q_PROCESSOR_POWER_64
    \relates <QtGlobal>

    Defined if the application is compiled for 64-bit Power processors. The \l
    Q_PROCESSOR_POWER macro is also defined when Q_PROCESSOR_POWER_64 is
    defined.
*/

/*!
    \macro Q_PROCESSOR_S390
    \relates <QtGlobal>

    Defined if the application is compiled for S/390 processors. Qt supports
    one optional variant of S/390: Q_PROCESSOR_S390_X.
*/
/*!
    \macro Q_PROCESSOR_S390_X
    \relates <QtGlobal>

    Defined if the application is compiled for S/390x processors. The \l
    Q_PROCESSOR_S390 macro is also defined when Q_PROCESSOR_S390_X is defined.
*/

/*!
    \macro Q_PROCESSOR_SH
    \relates <QtGlobal>

    Defined if the application is compiled for SuperH processors. Qt currently
    supports one SuperH revision: \l Q_PROCESSOR_SH_4A.
*/
/*!
    \macro Q_PROCESSOR_SH_4A
    \relates <QtGlobal>

    Defined if the application is compiled for SuperH 4A processors. The \l
    Q_PROCESSOR_SH macro is also defined when Q_PROCESSOR_SH_4A is defined.
*/

/*!
    \macro Q_PROCESSOR_SPARC
    \relates <QtGlobal>

    Defined if the application is compiled for SPARC processors. Qt currently
    supports one optional SPARC revision: \l Q_PROCESSOR_SPARC_V9.
*/
/*!
    \macro Q_PROCESSOR_SPARC_V9
    \relates <QtGlobal>

    Defined if the application is compiled for SPARC V9 processors. The \l
    Q_PROCESSOR_SPARC macro is also defined when Q_PROCESSOR_SPARC_V9 is
    defined.
*/

/*!
    \macro Q_PROCESSOR_X86
    \relates <QtGlobal>

    Defined if the application is compiled for x86 processors. Qt currently
    supports two x86 variants: \l Q_PROCESSOR_X86_32 and \l Q_PROCESSOR_X86_64.
*/
/*!
    \macro Q_PROCESSOR_X86_32
    \relates <QtGlobal>

    Defined if the application is compiled for 32-bit x86 processors. This
    includes all i386, i486, i586, and i686 processors. The \l Q_PROCESSOR_X86
    macro is also defined when Q_PROCESSOR_X86_32 is defined.
*/
/*!
    \macro Q_PROCESSOR_X86_64
    \relates <QtGlobal>

    Defined if the application is compiled for 64-bit x86 processors. This
    includes all AMD64, Intel 64, and other x86_64/x64 processors. The \l
    Q_PROCESSOR_X86 macro is also defined when Q_PROCESSOR_X86_64 is defined.
*/

/*!
  \macro QT_DISABLE_DEPRECATED_BEFORE
  \relates <QtGlobal>

  This macro can be defined in the project file to disable functions deprecated in
  a specified version of Qt or any earlier version. The default version number is 5.0,
  meaning that functions deprecated in or before Qt 5.0 will not be included.

  Examples:
  When using a future release of Qt 5, set QT_DISABLE_DEPRECATED_BEFORE=0x050100 to
  disable functions deprecated in Qt 5.1 and earlier. In any release, set
  QT_DISABLE_DEPRECATED_BEFORE=0x000000 to enable any functions, including the ones
  deprecated in Qt 5.0
 */

#if defined(QT_BUILD_QMAKE)
// needed to bootstrap qmake
static const unsigned int qt_one = 1;
const int QSysInfo::ByteOrder = ((*((unsigned char *) &qt_one) == 0) ? BigEndian : LittleEndian);
#endif

#if defined(Q_OS_MAC)

QT_BEGIN_INCLUDE_NAMESPACE
#include "private/qcore_mac_p.h"
#include "qnamespace.h"
QT_END_INCLUDE_NAMESPACE

#if defined(Q_OS_OSX)

Q_CORE_EXPORT OSErr qt_mac_create_fsref(const QString &file, FSRef *fsref)
{
    return FSPathMakeRef(reinterpret_cast<const UInt8 *>(file.toUtf8().constData()), fsref, 0);
}

Q_CORE_EXPORT void qt_mac_to_pascal_string(QString s, Str255 str, TextEncoding encoding=0, int len=-1)
{
    Q_UNUSED(encoding);
    Q_UNUSED(len);
    CFStringGetPascalString(QCFString(s), str, 256, CFStringGetSystemEncoding());
}

Q_CORE_EXPORT QString qt_mac_from_pascal_string(const Str255 pstr) {
    return QCFString(CFStringCreateWithPascalString(0, pstr, CFStringGetSystemEncoding()));
}
#endif // defined(Q_OS_OSX)

QSysInfo::MacVersion QSysInfo::macVersion()
{
#if defined(Q_OS_OSX)
    SInt32 gestalt_version;
    if (Gestalt(gestaltSystemVersion, &gestalt_version) == noErr) {
        return QSysInfo::MacVersion(((gestalt_version & 0x00F0) >> 4) + 2);
    }
#elif defined(Q_OS_IOS)
    return qt_ios_version(); // qtcore_mac_objc.mm
#endif
    return QSysInfo::MV_Unknown;
}
const QSysInfo::MacVersion QSysInfo::MacintoshVersion = QSysInfo::macVersion();

#elif defined(Q_OS_WIN) || defined(Q_OS_CYGWIN) || defined(Q_OS_WINCE) || defined(Q_OS_WINRT)

QT_BEGIN_INCLUDE_NAMESPACE
#include "qt_windows.h"
QT_END_INCLUDE_NAMESPACE

#ifndef Q_OS_WINRT
static inline OSVERSIONINFO winOsVersion()
{
    OSVERSIONINFO result = { sizeof(OSVERSIONINFO), 0, 0, 0, 0, {'\0'}};
    // GetVersionEx() has been deprecated in Windows 8.1 and will return
    // only Windows 8 from that version on.
#  if defined(_MSC_VER) && _MSC_VER >= 1800
#    pragma warning( push )
#    pragma warning( disable : 4996 )
#  endif
    GetVersionEx(&result);
#  if defined(_MSC_VER) && _MSC_VER >= 1800
#    pragma warning( pop )
#  endif
#  ifndef Q_OS_WINCE
    if (result.dwMajorVersion == 6 && result.dwMinorVersion == 2) {
        // This could be Windows 8.1 or higher. Note that as of Windows 9,
        // the major version needs to be checked as well.
        DWORDLONG conditionMask = 0;
        VER_SET_CONDITION(conditionMask, VER_MAJORVERSION, VER_GREATER_EQUAL);
        VER_SET_CONDITION(conditionMask, VER_MINORVERSION, VER_GREATER_EQUAL);
        VER_SET_CONDITION(conditionMask, VER_PLATFORMID, VER_EQUAL);
        OSVERSIONINFOEX checkVersion = { sizeof(OSVERSIONINFOEX), result.dwMajorVersion, result.dwMinorVersion,
                                         result.dwBuildNumber, result.dwPlatformId, {'\0'}, 0, 0, 0, 0, 0 };
        for ( ; VerifyVersionInfo(&checkVersion, VER_MAJORVERSION | VER_MINORVERSION | VER_PLATFORMID, conditionMask); ++checkVersion.dwMinorVersion)
            result.dwMinorVersion = checkVersion.dwMinorVersion;
    }
#  endif // !Q_OS_WINCE
    return result;
}
#endif // !Q_OS_WINRT

QSysInfo::WinVersion QSysInfo::windowsVersion()
{
#ifndef VER_PLATFORM_WIN32s
#define VER_PLATFORM_WIN32s            0
#endif
#ifndef VER_PLATFORM_WIN32_WINDOWS
#define VER_PLATFORM_WIN32_WINDOWS  1
#endif
#ifndef VER_PLATFORM_WIN32_NT
#define VER_PLATFORM_WIN32_NT            2
#endif
#ifndef VER_PLATFORM_WIN32_CE
#define VER_PLATFORM_WIN32_CE            3
#endif

    static QSysInfo::WinVersion winver;
    if (winver)
        return winver;
#ifdef Q_OS_WINRT
    winver = QSysInfo::WV_WINDOWS8;
#else
    winver = QSysInfo::WV_NT;
    const OSVERSIONINFO osver = winOsVersion();
#ifdef Q_OS_WINCE
    DWORD qt_cever = 0;
    qt_cever = osver.dwMajorVersion * 100;
    qt_cever += osver.dwMinorVersion * 10;
#endif
    switch (osver.dwPlatformId) {
    case VER_PLATFORM_WIN32s:
        winver = QSysInfo::WV_32s;
        break;
    case VER_PLATFORM_WIN32_WINDOWS:
        // We treat Windows Me (minor 90) the same as Windows 98
        if (osver.dwMinorVersion == 90)
            winver = QSysInfo::WV_Me;
        else if (osver.dwMinorVersion == 10)
            winver = QSysInfo::WV_98;
        else
            winver = QSysInfo::WV_95;
        break;
#ifdef Q_OS_WINCE
    case VER_PLATFORM_WIN32_CE:
        if (qt_cever >= 600)
            winver = QSysInfo::WV_CE_6;
        if (qt_cever >= 500)
            winver = QSysInfo::WV_CE_5;
        else if (qt_cever >= 400)
            winver = QSysInfo::WV_CENET;
        else
            winver = QSysInfo::WV_CE;
        break;
#endif
    default: // VER_PLATFORM_WIN32_NT
        if (osver.dwMajorVersion < 5) {
            winver = QSysInfo::WV_NT;
        } else if (osver.dwMajorVersion == 5 && osver.dwMinorVersion == 0) {
            winver = QSysInfo::WV_2000;
        } else if (osver.dwMajorVersion == 5 && osver.dwMinorVersion == 1) {
            winver = QSysInfo::WV_XP;
        } else if (osver.dwMajorVersion == 5 && osver.dwMinorVersion == 2) {
            winver = QSysInfo::WV_2003;
        } else if (osver.dwMajorVersion == 6 && osver.dwMinorVersion == 0) {
            winver = QSysInfo::WV_VISTA;
        } else if (osver.dwMajorVersion == 6 && osver.dwMinorVersion == 1) {
            winver = QSysInfo::WV_WINDOWS7;
        } else if (osver.dwMajorVersion == 6 && osver.dwMinorVersion == 2) {
            winver = QSysInfo::WV_WINDOWS8;
        } else if (osver.dwMajorVersion == 6 && osver.dwMinorVersion == 3) {
            winver = QSysInfo::WV_WINDOWS8_1;
        } else {
            qWarning("Qt: Untested Windows version %d.%d detected!",
                     int(osver.dwMajorVersion), int(osver.dwMinorVersion));
            winver = QSysInfo::WV_NT_based;
        }
    }

#ifdef QT_DEBUG
    {
        QByteArray override = qgetenv("QT_WINVER_OVERRIDE");
        if (override.isEmpty())
            return winver;

        if (override == "Me")
            winver = QSysInfo::WV_Me;
        if (override == "95")
            winver = QSysInfo::WV_95;
        else if (override == "98")
            winver = QSysInfo::WV_98;
        else if (override == "NT")
            winver = QSysInfo::WV_NT;
        else if (override == "2000")
            winver = QSysInfo::WV_2000;
        else if (override == "2003")
            winver = QSysInfo::WV_2003;
        else if (override == "XP")
            winver = QSysInfo::WV_XP;
        else if (override == "VISTA")
            winver = QSysInfo::WV_VISTA;
        else if (override == "WINDOWS7")
            winver = QSysInfo::WV_WINDOWS7;
        else if (override == "WINDOWS8")
            winver = QSysInfo::WV_WINDOWS8;
    }
#endif
#endif // !Q_OS_WINRT

    return winver;
}

const QSysInfo::WinVersion QSysInfo::WindowsVersion = QSysInfo::windowsVersion();

#endif

/*!
    \macro void Q_ASSERT(bool test)
    \relates <QtGlobal>

    Prints a warning message containing the source code file name and
    line number if \a test is \c false.

    Q_ASSERT() is useful for testing pre- and post-conditions
    during development. It does nothing if \c QT_NO_DEBUG was defined
    during compilation.

    Example:

    \snippet code/src_corelib_global_qglobal.cpp 17

    If \c b is zero, the Q_ASSERT statement will output the following
    message using the qFatal() function:

    \snippet code/src_corelib_global_qglobal.cpp 18

    \sa Q_ASSERT_X(), qFatal(), {Debugging Techniques}
*/

/*!
    \macro void Q_ASSERT_X(bool test, const char *where, const char *what)
    \relates <QtGlobal>

    Prints the message \a what together with the location \a where,
    the source file name and line number if \a test is \c false.

    Q_ASSERT_X is useful for testing pre- and post-conditions during
    development. It does nothing if \c QT_NO_DEBUG was defined during
    compilation.

    Example:

    \snippet code/src_corelib_global_qglobal.cpp 19

    If \c b is zero, the Q_ASSERT_X statement will output the following
    message using the qFatal() function:

    \snippet code/src_corelib_global_qglobal.cpp 20

    \sa Q_ASSERT(), qFatal(), {Debugging Techniques}
*/

/*!
    \macro void Q_ASSUME(bool expr)
    \relates <QtGlobal>
    \since 5.0

    Causes the compiler to assume that \a expr is \c true. This macro is useful
    for improving code generation, by providing the compiler with hints about
    conditions that it would not otherwise know about. However, there is no
    guarantee that the compiler will actually use those hints.

    This macro could be considered a "lighter" version of \l{Q_ASSERT()}. While
    Q_ASSERT will abort the program's execution if the condition is \c false,
    Q_ASSUME will tell the compiler not to generate code for those conditions.
    Therefore, it is important that the assumptions always hold, otherwise
    undefined behaviour may occur.

    If \a expr is a constantly \c false condition, Q_ASSUME will tell the compiler
    that the current code execution cannot be reached. That is, Q_ASSUME(false)
    is equivalent to Q_UNREACHABLE().

    In debug builds the condition is enforced by an assert to facilitate debugging.

    \note Q_LIKELY() tells the compiler that the expression is likely, but not
    the only possibility. Q_ASSUME tells the compiler that it is the only
    possibility.

    \sa Q_ASSERT(), Q_UNREACHABLE(), Q_LIKELY()
*/

/*!
    \macro void Q_UNREACHABLE()
    \relates <QtGlobal>
    \since 5.0

    Tells the compiler that the current point cannot be reached by any
    execution, so it may optimize any code paths leading here as dead code, as
    well as code continuing from here.

    This macro is useful to mark impossible conditions. For example, given the
    following enum:

    \snippet code/src_corelib_global_qglobal.cpp qunreachable-enum

    One can write a switch table like so:

    \snippet code/src_corelib_global_qglobal.cpp qunreachable-switch

    The advantage of inserting Q_UNREACHABLE() at that point is that the
    compiler is told not to generate code for a shape variable containing that
    value. If the macro is missing, the compiler will still generate the
    necessary comparisons for that value. If the case label were removed, some
    compilers could produce a warning that some enum values were not checked.

    By using this macro in impossible conditions, code coverage may be improved
    as dead code paths may be eliminated.

    In debug builds the condition is enforced by an assert to facilitate debugging.

    \sa Q_ASSERT(), Q_ASSUME(), qFatal()
*/

/*!
    \macro void Q_CHECK_PTR(void *pointer)
    \relates <QtGlobal>

    If \a pointer is 0, prints a warning message containing the source
    code's file name and line number, saying that the program ran out
    of memory.

    Q_CHECK_PTR does nothing if \c QT_NO_DEBUG was defined during
    compilation.

    Example:

    \snippet code/src_corelib_global_qglobal.cpp 21

    \sa qWarning(), {Debugging Techniques}
*/

/*!
    \fn T *q_check_ptr(T *pointer)
    \relates <QtGlobal>

    Uses Q_CHECK_PTR on \a pointer, then returns \a pointer.

    This can be used as an inline version of Q_CHECK_PTR.
*/

/*!
    \macro const char* Q_FUNC_INFO()
    \relates <QtGlobal>

    Expands to a string that describe the function the macro resides in. How this string looks
    more specifically is compiler dependent. With GNU GCC it is typically the function signature,
    while with other compilers it might be the line and column number.

    Q_FUNC_INFO can be conveniently used with qDebug(). For example, this function:

    \snippet code/src_corelib_global_qglobal.cpp 22

    when instantiated with the integer type, will with the GCC compiler produce:

    \tt{const TInputType& myMin(const TInputType&, const TInputType&) [with TInputType = int] was called with value1: 3 value2: 4}

    If this macro is used outside a function, the behavior is undefined.
 */

/*
  The Q_CHECK_PTR macro calls this function if an allocation check
  fails.
*/
void qt_check_pointer(const char *n, int l)
{
    qFatal("In file %s, line %d: Out of memory", n, l);
}

/*
   \internal
   Allows you to throw an exception without including <new>
   Called internally from Q_CHECK_PTR on certain OS combinations
*/
void qBadAlloc()
{
    QT_THROW(std::bad_alloc());
}

#ifndef QT_NO_EXCEPTIONS
/*
   \internal
   Allows you to call std::terminate() without including <exception>.
   Called internally from QT_TERMINATE_ON_EXCEPTION
*/
Q_NORETURN void qTerminate() Q_DECL_NOTHROW
{
    std::terminate();
}
#endif

/*
  The Q_ASSERT macro calls this function when the test fails.
*/
void qt_assert(const char *assertion, const char *file, int line) Q_DECL_NOTHROW
{
    qFatal("ASSERT: \"%s\" in file %s, line %d", assertion, file, line);
}

/*
  The Q_ASSERT_X macro calls this function when the test fails.
*/
void qt_assert_x(const char *where, const char *what, const char *file, int line) Q_DECL_NOTHROW
{
    qFatal("ASSERT failure in %s: \"%s\", file %s, line %d", where, what, file, line);
}


/*
    Dijkstra's bisection algorithm to find the square root of an integer.
    Deliberately not exported as part of the Qt API, but used in both
    qsimplerichtext.cpp and qgfxraster_qws.cpp
*/
Q_CORE_EXPORT unsigned int qt_int_sqrt(unsigned int n)
{
    // n must be in the range 0...UINT_MAX/2-1
    if (n >= (UINT_MAX>>2)) {
        unsigned int r = 2 * qt_int_sqrt(n / 4);
        unsigned int r2 = r + 1;
        return (n >= r2 * r2) ? r2 : r;
    }
    uint h, p= 0, q= 1, r= n;
    while (q <= n)
        q <<= 2;
    while (q != 1) {
        q >>= 2;
        h= p + q;
        p >>= 1;
        if (r >= h) {
            p += q;
            r -= h;
        }
    }
    return p;
}

void *qMemCopy(void *dest, const void *src, size_t n) { return memcpy(dest, src, n); }
void *qMemSet(void *dest, int c, size_t n) { return memset(dest, c, n); }

#if !defined(Q_OS_WIN) && !defined(QT_NO_THREAD) && !defined(Q_OS_INTEGRITY) && !defined(Q_OS_QNX) && \
    defined(_POSIX_THREAD_SAFE_FUNCTIONS) && _POSIX_VERSION >= 200112L
namespace {
    // There are two incompatible versions of strerror_r:
    // a) the XSI/POSIX.1 version, which returns an int,
    //    indicating success or not
    // b) the GNU version, which returns a char*, which may or may not
    //    be the beginning of the buffer we used
    // The GNU libc manpage for strerror_r says you should use the XSI
    // version in portable code. However, it's impossible to do that if
    // _GNU_SOURCE is defined so we use C++ overloading to decide what to do
    // depending on the return type
    static inline Q_DECL_UNUSED QString fromstrerror_helper(int, const QByteArray &buf)
    {
        return QString::fromLocal8Bit(buf);
    }
    static inline Q_DECL_UNUSED QString fromstrerror_helper(const char *str, const QByteArray &)
    {
        return QString::fromLocal8Bit(str);
    }
}
#endif

QString qt_error_string(int errorCode)
{
    const char *s = 0;
    QString ret;
    if (errorCode == -1) {
#if defined(Q_OS_WIN)
        errorCode = GetLastError();
#else
        errorCode = errno;
#endif
    }
    switch (errorCode) {
    case 0:
        break;
    case EACCES:
        s = QT_TRANSLATE_NOOP("QIODevice", "Permission denied");
        break;
    case EMFILE:
        s = QT_TRANSLATE_NOOP("QIODevice", "Too many open files");
        break;
    case ENOENT:
        s = QT_TRANSLATE_NOOP("QIODevice", "No such file or directory");
        break;
    case ENOSPC:
        s = QT_TRANSLATE_NOOP("QIODevice", "No space left on device");
        break;
    default: {
#if defined(Q_OS_WIN)
        // Retrieve the system error message for the last-error code.
#  ifndef Q_OS_WINRT
        wchar_t *string = 0;
        FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_SYSTEM,
                      NULL,
                      errorCode,
                      MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                      (LPWSTR)&string,
                      0,
                      NULL);
        ret = QString::fromWCharArray(string);
        LocalFree((HLOCAL)string);
#  else // !Q_OS_WINRT
        __declspec(thread) static wchar_t errorString[4096];
        FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                      NULL,
                      errorCode,
                      MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                      errorString,
                      ARRAYSIZE(errorString),
                      NULL);
        ret = QString::fromWCharArray(errorString);
#  endif // Q_OS_WINRT

        if (ret.isEmpty() && errorCode == ERROR_MOD_NOT_FOUND)
            ret = QString::fromLatin1("The specified module could not be found.");
#elif !defined(QT_NO_THREAD) && defined(_POSIX_THREAD_SAFE_FUNCTIONS) && _POSIX_VERSION >= 200112L && !defined(Q_OS_INTEGRITY) && !defined(Q_OS_QNX)
        QByteArray buf(1024, '\0');
        ret = fromstrerror_helper(strerror_r(errorCode, buf.data(), buf.size()), buf);
#else
        ret = QString::fromLocal8Bit(strerror(errorCode));
#endif
    break; }
    }
    if (s)
        // ######## this breaks moc build currently
//         ret = QCoreApplication::translate("QIODevice", s);
        ret = QString::fromLatin1(s);
    return ret.trimmed();
}

// getenv is declared as deprecated in VS2005. This function
// makes use of the new secure getenv function.
/*!
    \relates <QtGlobal>

    Returns the value of the environment variable with name \a
    varName. To get the variable string, use QByteArray::constData().

    \note qgetenv() was introduced because getenv() from the standard
    C library was deprecated in VC2005 (and later versions). qgetenv()
    uses the new replacement function in VC, and calls the standard C
    library's implementation on all other platforms.

    \sa qputenv(), qEnvironmentVariableIsSet(), qEnvironmentVariableIsEmpty()
*/
QByteArray qgetenv(const char *varName)
{
#if defined(_MSC_VER) && _MSC_VER >= 1400
    size_t requiredSize = 0;
    QByteArray buffer;
    getenv_s(&requiredSize, 0, 0, varName);
    if (requiredSize == 0)
        return buffer;
    buffer.resize(int(requiredSize));
    getenv_s(&requiredSize, buffer.data(), requiredSize, varName);
    // requiredSize includes the terminating null, which we don't want.
    Q_ASSERT(buffer.endsWith('\0'));
    buffer.chop(1);
    return buffer;
#else
    return QByteArray(::getenv(varName));
#endif
}

/*!
    \relates <QtGlobal>
    \since 5.1

    Returns whether the environment variable \a varName is empty.

    Equivalent to
    \code
    qgetenv(varName).isEmpty()
    \endcode
    except that it's potentially much faster, and can't throw exceptions.

    \sa qgetenv(), qEnvironmentVariableIsSet()
*/
bool qEnvironmentVariableIsEmpty(const char *varName) Q_DECL_NOEXCEPT
{
#if defined(_MSC_VER) && _MSC_VER >= 1400
    // we provide a buffer that can only hold the empty string, so
    // when the env.var isn't empty, we'll get an ERANGE error (buffer
    // too small):
    size_t dummy;
    char buffer = '\0';
    return getenv_s(&dummy, &buffer, 1, varName) != ERANGE;
#else
    const char * const value = ::getenv(varName);
    return !value || !*value;
#endif
}

/*!
    \relates <QtGlobal>
    \since 5.1

    Returns whether the environment variable \a varName is set.

    Equivalent to
    \code
    !qgetenv(varName).isNull()
    \endcode
    except that it's potentially much faster, and can't throw exceptions.

    \sa qgetenv(), qEnvironmentVariableIsEmpty()
*/
bool qEnvironmentVariableIsSet(const char *varName) Q_DECL_NOEXCEPT
{
#if defined(_MSC_VER) && _MSC_VER >= 1400
    size_t requiredSize = 0;
    (void)getenv_s(&requiredSize, 0, 0, varName);
    return requiredSize != 0;
#else
    return ::getenv(varName) != 0;
#endif
}

/*!
    \relates <QtGlobal>

    This function sets the \a value of the environment variable named
    \a varName. It will create the variable if it does not exist. It
    returns 0 if the variable could not be set.

    Calling qputenv with an empty value removes the environment variable on
    Windows, and makes it set (but empty) on Unix. Prefer using qunsetenv()
    for fully portable behavior.

    \note qputenv() was introduced because putenv() from the standard
    C library was deprecated in VC2005 (and later versions). qputenv()
    uses the replacement function in VC, and calls the standard C
    library's implementation on all other platforms.

    \sa qgetenv()
*/
bool qputenv(const char *varName, const QByteArray& value)
{
#if defined(_MSC_VER) && _MSC_VER >= 1400
    return _putenv_s(varName, value.constData()) == 0;
#elif defined(_POSIX_VERSION) && (_POSIX_VERSION-0) >= 200112L
    // POSIX.1-2001 has setenv
    return setenv(varName, value.constData(), true) == 0;
#else
    QByteArray buffer(varName);
    buffer += '=';
    buffer += value;
    char* envVar = qstrdup(buffer.constData());
    int result = putenv(envVar);
    if (result != 0) // error. we have to delete the string.
        delete[] envVar;
    return result == 0;
#endif
}

/*!
    \relates <QtGlobal>

    This function deletes the variable \a varName from the environment.

    Returns \c true on success.

    \since 5.1

    \sa qputenv(), qgetenv()
*/
bool qunsetenv(const char *varName)
{
#if defined(_MSC_VER) && _MSC_VER >= 1400
    return _putenv_s(varName, "") == 0;
#elif (defined(_POSIX_VERSION) && (_POSIX_VERSION-0) >= 200112L) || defined(Q_OS_BSD4)
    // POSIX.1-2001 and BSD have unsetenv
    return unsetenv(varName) == 0;
#elif defined(Q_CC_MINGW)
    // On mingw, putenv("var=") removes "var" from the environment
    QByteArray buffer(varName);
    buffer += '=';
    return putenv(buffer.constData()) == 0;
#else
    // Fallback to putenv("var=") which will insert an empty var into the
    // environment and leak it
    QByteArray buffer(varName);
    buffer += '=';
    char *envVar = qstrdup(buffer.constData());
    return putenv(envVar) == 0;
#endif
}

#if defined(Q_OS_UNIX) && !defined(QT_NO_THREAD) && defined(_POSIX_THREAD_SAFE_FUNCTIONS) && (_POSIX_THREAD_SAFE_FUNCTIONS - 0 > 0)

#  if defined(Q_OS_INTEGRITY) && defined(__GHS_VERSION_NUMBER) && (__GHS_VERSION_NUMBER < 500)
// older versions of INTEGRITY used a long instead of a uint for the seed.
typedef long SeedStorageType;
#  else
typedef uint SeedStorageType;
#  endif

typedef QThreadStorage<SeedStorageType *> SeedStorage;
Q_GLOBAL_STATIC(SeedStorage, randTLS)  // Thread Local Storage for seed value

#elif defined(Q_OS_ANDROID) && !defined(Q_OS_ANDROID_NO_SDK)
typedef QThreadStorage<QJNIObjectPrivate> AndroidRandomStorage;
Q_GLOBAL_STATIC(AndroidRandomStorage, randomTLS)
#endif

/*!
    \relates <QtGlobal>
    \since 4.2

    Thread-safe version of the standard C++ \c srand() function.

    Sets the argument \a seed to be used to generate a new random number sequence of
    pseudo random integers to be returned by qrand().

    The sequence of random numbers generated is deterministic per thread. For example,
    if two threads call qsrand(1) and subsequently calls qrand(), the threads will get
    the same random number sequence.

    \sa qrand()
*/
void qsrand(uint seed)
{
#if defined(Q_OS_UNIX) && !defined(QT_NO_THREAD) && defined(_POSIX_THREAD_SAFE_FUNCTIONS) && (_POSIX_THREAD_SAFE_FUNCTIONS - 0 > 0)
    SeedStorage *seedStorage = randTLS();
    if (seedStorage) {
        SeedStorageType *pseed = seedStorage->localData();
        if (!pseed)
            seedStorage->setLocalData(pseed = new SeedStorageType);
        *pseed = seed;
    } else {
        //global static seed storage should always exist,
        //except after being deleted by QGlobalStaticDeleter.
        //But since it still can be called from destructor of another
        //global static object, fallback to srand(seed)
        srand(seed);
    }
#elif defined(Q_OS_ANDROID) && !defined(Q_OS_ANDROID_NO_SDK)
    if (randomTLS->hasLocalData()) {
        randomTLS->localData().callMethod<void>("setSeed", "(J)V", jlong(seed));
        return;
    }

    QJNIObjectPrivate random("java/util/Random",
                             "(J)V",
                             jlong(seed));
    if (!random.isValid()) {
        srand(seed);
        return;
    }

    randomTLS->setLocalData(random);
#else
    // On Windows srand() and rand() already use Thread-Local-Storage
    // to store the seed between calls
    // this is also valid for QT_NO_THREAD
    srand(seed);
#endif
}

/*!
    \relates <QtGlobal>
    \since 4.2

    Thread-safe version of the standard C++ \c rand() function.

    Returns a value between 0 and \c RAND_MAX (defined in \c <cstdlib> and
    \c <stdlib.h>), the next number in the current sequence of pseudo-random
    integers.

    Use \c qsrand() to initialize the pseudo-random number generator with
    a seed value.

    \sa qsrand()
*/
int qrand()
{
#if defined(Q_OS_UNIX) && !defined(QT_NO_THREAD) && defined(_POSIX_THREAD_SAFE_FUNCTIONS) && (_POSIX_THREAD_SAFE_FUNCTIONS - 0 > 0)
    SeedStorage *seedStorage = randTLS();
    if (seedStorage) {
        SeedStorageType *pseed = seedStorage->localData();
        if (!pseed) {
            seedStorage->setLocalData(pseed = new SeedStorageType);
            *pseed = 1;
        }
        return rand_r(pseed);
    } else {
        //global static seed storage should always exist,
        //except after being deleted by QGlobalStaticDeleter.
        //But since it still can be called from destructor of another
        //global static object, fallback to rand()
        return rand();
    }
#elif defined(Q_OS_ANDROID) && !defined(Q_OS_ANDROID_NO_SDK)
    AndroidRandomStorage *randomStorage = randomTLS();
    if (!randomStorage)
        return rand();

    if (randomStorage->hasLocalData()) {
        return randomStorage->localData().callMethod<jint>("nextInt",
                                                           "(I)I",
                                                           RAND_MAX);
    }

    QJNIObjectPrivate random("java/util/Random",
                             "(J)V",
                             jlong(1));

    if (!random.isValid())
        return rand();

    randomStorage->setLocalData(random);
    return random.callMethod<jint>("nextInt", "(I)I", RAND_MAX);
#else
    // On Windows srand() and rand() already use Thread-Local-Storage
    // to store the seed between calls
    // this is also valid for QT_NO_THREAD
    return rand();
#endif
}

/*!
    \macro forever
    \relates <QtGlobal>

    This macro is provided for convenience for writing infinite
    loops.

    Example:

    \snippet code/src_corelib_global_qglobal.cpp 31

    It is equivalent to \c{for (;;)}.

    If you're worried about namespace pollution, you can disable this
    macro by adding the following line to your \c .pro file:

    \snippet code/src_corelib_global_qglobal.cpp 32

    \sa Q_FOREVER
*/

/*!
    \macro Q_FOREVER
    \relates <QtGlobal>

    Same as \l{forever}.

    This macro is available even when \c no_keywords is specified
    using the \c .pro file's \c CONFIG variable.

    \sa foreach()
*/

/*!
    \macro foreach(variable, container)
    \relates <QtGlobal>

    This macro is used to implement Qt's \c foreach loop. The \a
    variable parameter is a variable name or variable definition; the
    \a container parameter is a Qt container whose value type
    corresponds to the type of the variable. See \l{The foreach
    Keyword} for details.

    If you're worried about namespace pollution, you can disable this
    macro by adding the following line to your \c .pro file:

    \snippet code/src_corelib_global_qglobal.cpp 33

    \sa Q_FOREACH()
*/

/*!
    \macro Q_FOREACH(variable, container)
    \relates <QtGlobal>

    Same as foreach(\a variable, \a container).

    This macro is available even when \c no_keywords is specified
    using the \c .pro file's \c CONFIG variable.

    \sa foreach()
*/

/*!
    \macro QT_TR_NOOP(sourceText)
    \relates <QtGlobal>

    Marks the string literal \a sourceText for dynamic translation in
    the current context (class), i.e the stored \a sourceText will not
    be altered.

    The macro expands to \a sourceText.

    Example:

    \snippet code/src_corelib_global_qglobal.cpp 34

    The macro QT_TR_NOOP_UTF8() is identical except that it tells lupdate
    that the source string is encoded in UTF-8. Corresponding variants
    exist in the QT_TRANSLATE_NOOP() family of macros, too.

    \sa QT_TRANSLATE_NOOP(), {Internationalization with Qt}
*/

/*!
    \macro QT_TRANSLATE_NOOP(context, sourceText)
    \relates <QtGlobal>

    Marks the string literal \a sourceText for dynamic translation in
    the given \a context; i.e, the stored \a sourceText will not be
    altered. The \a context is typically a class and also needs to
    be specified as string literal.

    The macro expands to \a sourceText.

    Example:

    \snippet code/src_corelib_global_qglobal.cpp 35

    \sa QT_TR_NOOP(), QT_TRANSLATE_NOOP3(), {Internationalization with Qt}
*/

/*!
    \macro QT_TRANSLATE_NOOP3(context, sourceText, comment)
    \relates <QtGlobal>
    \since 4.4

    Marks the string literal \a sourceText for dynamic translation in the
    given \a context and with \a comment, i.e the stored \a sourceText will
    not be altered. The \a context is typically a class and also needs to
    be specified as string literal. The string literal \a comment
    will be available for translators using e.g. Qt Linguist.

    The macro expands to anonymous struct of the two string
    literals passed as \a sourceText and \a comment.

    Example:

    \snippet code/src_corelib_global_qglobal.cpp 36

    \sa QT_TR_NOOP(), QT_TRANSLATE_NOOP(), {Internationalization with Qt}
*/

/*!
    \fn QString qtTrId(const char *id, int n = -1)
    \relates <QtGlobal>
    \reentrant
    \since 4.6

    \brief The qtTrId function finds and returns a translated string.

    Returns a translated string identified by \a id.
    If no matching string is found, the id itself is returned. This
    should not happen under normal conditions.

    If \a n >= 0, all occurrences of \c %n in the resulting string
    are replaced with a decimal representation of \a n. In addition,
    depending on \a n's value, the translation text may vary.

    Meta data and comments can be passed as documented for QObject::tr().
    In addition, it is possible to supply a source string template like that:

    \tt{//% <C string>}

    or

    \tt{\\begincomment% <C string> \\endcomment}

    Example:

    \snippet code/src_corelib_global_qglobal.cpp qttrid

    Creating QM files suitable for use with this function requires passing
    the \c -idbased option to the \c lrelease tool.

    \warning This method is reentrant only if all translators are
    installed \e before calling this method. Installing or removing
    translators while performing translations is not supported. Doing
    so will probably result in crashes or other undesirable behavior.

    \sa QObject::tr(), QCoreApplication::translate(), {Internationalization with Qt}
*/

/*!
    \macro QT_TRID_NOOP(id)
    \relates <QtGlobal>
    \since 4.6

    \brief The QT_TRID_NOOP macro marks an id for dynamic translation.

    The only purpose of this macro is to provide an anchor for attaching
    meta data like to qtTrId().

    The macro expands to \a id.

    Example:

    \snippet code/src_corelib_global_qglobal.cpp qttrid_noop

    \sa qtTrId(), {Internationalization with Qt}
*/

/*!
    \macro Q_LIKELY(expr)
    \relates <QtGlobal>
    \since 4.8

    \brief Hints to the compiler that the enclosed condition, \a expr, is
    likely to evaluate to \c true.

    Use of this macro can help the compiler to optimize the code.

    Example:

    \snippet code/src_corelib_global_qglobal.cpp qlikely

    \sa Q_UNLIKELY()
*/

/*!
    \macro Q_UNLIKELY(expr)
    \relates <QtGlobal>
    \since 4.8

    \brief Hints to the compiler that the enclosed condition, \a expr, is
    likely to evaluate to \c false.

    Use of this macro can help the compiler to optimize the code.

    Example:

    \snippet code/src_corelib_global_qglobal.cpp qunlikely

    \sa Q_LIKELY()
*/

/*!
    \macro QT_POINTER_SIZE
    \relates <QtGlobal>

    Expands to the size of a pointer in bytes (4 or 8). This is
    equivalent to \c sizeof(void *) but can be used in a preprocessor
    directive.
*/

/*!
    \macro QABS(n)
    \relates <QtGlobal>
    \obsolete

    Use qAbs(\a n) instead.

    \sa QMIN(), QMAX()
*/

/*!
    \macro QMIN(x, y)
    \relates <QtGlobal>
    \obsolete

    Use qMin(\a x, \a y) instead.

    \sa QMAX(), QABS()
*/

/*!
    \macro QMAX(x, y)
    \relates <QtGlobal>
    \obsolete

    Use qMax(\a x, \a y) instead.

    \sa QMIN(), QABS()
*/

/*!
    \macro const char *qPrintable(const QString &str)
    \relates <QtGlobal>

    Returns \a str as a \c{const char *}. This is equivalent to
    \a{str}.toLocal8Bit().constData().

    The char pointer will be invalid after the statement in which
    qPrintable() is used. This is because the array returned by
    QString::toLocal8Bit() will fall out of scope.

    Example:

    \snippet code/src_corelib_global_qglobal.cpp 37

    \note qDebug(), qWarning(), qCritical(), qFatal() expect %s
    arguments to be UTF-8 encoded, while qPrintable() converts to
    local 8-bit encoding. Therefore using qPrintable for logging
    strings is only safe if the argument contains only ASCII
    characters.

    \sa qDebug(), qWarning(), qCritical(), qFatal()
*/

/*!
    \macro Q_DECLARE_TYPEINFO(Type, Flags)
    \relates <QtGlobal>

    You can use this macro to specify information about a custom type
    \a Type. With accurate type information, Qt's \l{Container Classes}
    {generic containers} can choose appropriate storage methods and
    algorithms.

    \a Flags can be one of the following:

    \list
    \li \c Q_PRIMITIVE_TYPE specifies that \a Type is a POD (plain old
       data) type with no constructor or destructor, or else a type where
       every bit pattern is a valid object and memcpy() creates a valid
       independent copy of the object.
    \li \c Q_MOVABLE_TYPE specifies that \a Type has a constructor
       and/or a destructor but can be moved in memory using \c
       memcpy().
    \li \c Q_COMPLEX_TYPE (the default) specifies that \a Type has
       constructors and/or a destructor and that it may not be moved
       in memory.
    \endlist

    Example of a "primitive" type:

    \snippet code/src_corelib_global_qglobal.cpp 38

    An example of a non-POD "primitive" type is QUuid: Even though
    QUuid has constructors (and therefore isn't POD), every bit
    pattern still represents a valid object, and memcpy() can be used
    to create a valid independent copy of a QUuid object.

    Example of a movable type:

    \snippet code/src_corelib_global_qglobal.cpp 39
*/

/*!
    \macro Q_UNUSED(name)
    \relates <QtGlobal>

    Indicates to the compiler that the parameter with the specified
    \a name is not used in the body of a function. This can be used to
    suppress compiler warnings while allowing functions to be defined
    with meaningful parameter names in their signatures.
*/

struct QInternal_CallBackTable {
    QVector<QList<qInternalCallback> > callbacks;
};

Q_GLOBAL_STATIC(QInternal_CallBackTable, global_callback_table)

bool QInternal::registerCallback(Callback cb, qInternalCallback callback)
{
    if (cb >= 0 && cb < QInternal::LastCallback) {
        QInternal_CallBackTable *cbt = global_callback_table();
        cbt->callbacks.resize(cb + 1);
        cbt->callbacks[cb].append(callback);
        return true;
    }
    return false;
}

bool QInternal::unregisterCallback(Callback cb, qInternalCallback callback)
{
    if (cb >= 0 && cb < QInternal::LastCallback) {
        QInternal_CallBackTable *cbt = global_callback_table();
        return (bool) cbt->callbacks[cb].removeAll(callback);
    }
    return false;
}

bool QInternal::activateCallbacks(Callback cb, void **parameters)
{
    Q_ASSERT_X(cb >= 0, "QInternal::activateCallback()", "Callback id must be a valid id");

    QInternal_CallBackTable *cbt = global_callback_table();
    if (cbt && cb < cbt->callbacks.size()) {
        QList<qInternalCallback> callbacks = cbt->callbacks[cb];
        bool ret = false;
        for (int i=0; i<callbacks.size(); ++i)
            ret |= (callbacks.at(i))(parameters);
        return ret;
    }
    return false;
}

/*!
    \macro Q_BYTE_ORDER
    \relates <QtGlobal>

    This macro can be used to determine the byte order your system
    uses for storing data in memory. i.e., whether your system is
    little-endian or big-endian. It is set by Qt to one of the macros
    Q_LITTLE_ENDIAN or Q_BIG_ENDIAN. You normally won't need to worry
    about endian-ness, but you might, for example if you need to know
    which byte of an integer or UTF-16 character is stored in the
    lowest address. Endian-ness is important in networking, where
    computers with different values for Q_BYTE_ORDER must pass data
    back and forth.

    Use this macro as in the following examples.

    \snippet code/src_corelib_global_qglobal.cpp 40

    \sa Q_BIG_ENDIAN, Q_LITTLE_ENDIAN
*/

/*!
    \macro Q_LITTLE_ENDIAN
    \relates <QtGlobal>

    This macro represents a value you can compare to the macro
    Q_BYTE_ORDER to determine the endian-ness of your system.  In a
    little-endian system, the least significant byte is stored at the
    lowest address. The other bytes follow in increasing order of
    significance.

    \snippet code/src_corelib_global_qglobal.cpp 41

    \sa Q_BYTE_ORDER, Q_BIG_ENDIAN
*/

/*!
    \macro Q_BIG_ENDIAN
    \relates <QtGlobal>

    This macro represents a value you can compare to the macro
    Q_BYTE_ORDER to determine the endian-ness of your system.  In a
    big-endian system, the most significant byte is stored at the
    lowest address. The other bytes follow in decreasing order of
    significance.

    \snippet code/src_corelib_global_qglobal.cpp 42

    \sa Q_BYTE_ORDER, Q_LITTLE_ENDIAN
*/

/*!
    \macro Q_GLOBAL_STATIC(type, name)
    \internal

    Declares a global static variable with the given \a type and \a name.

    Use this macro to instantiate an object in a thread-safe way, creating
    a global pointer that can be used to refer to it.

    \warning This macro is subject to a race condition that can cause the object
    to be constructed twice. However, if this occurs, the second instance will
    be immediately deleted.

    See also
    \l{http://www.aristeia.com/publications.html}{"C++ and the perils of Double-Checked Locking"}
    by Scott Meyers and Andrei Alexandrescu.
*/

/*!
    \macro Q_GLOBAL_STATIC_WITH_ARGS(type, name, arguments)
    \internal

    Declares a global static variable with the specified \a type and \a name.

    Use this macro to instantiate an object using the \a arguments specified
    in a thread-safe way, creating a global pointer that can be used to refer
    to it.

    \warning This macro is subject to a race condition that can cause the object
    to be constructed twice. However, if this occurs, the second instance will
    be immediately deleted.

    See also
    \l{http://www.aristeia.com/publications.html}{"C++ and the perils of Double-Checked Locking"}
    by Scott Meyers and Andrei Alexandrescu.
*/

/*!
    \macro QT_NAMESPACE
    \internal

    If this macro is defined to \c ns all Qt classes are put in a namespace
    called \c ns. Also, moc will output code putting metaobjects etc.
    into namespace \c ns.

    \sa QT_BEGIN_NAMESPACE, QT_END_NAMESPACE,
    QT_PREPEND_NAMESPACE, QT_USE_NAMESPACE,
    QT_BEGIN_INCLUDE_NAMESPACE, QT_END_INCLUDE_NAMESPACE,
    QT_BEGIN_MOC_NAMESPACE, QT_END_MOC_NAMESPACE,
*/

/*!
    \macro QT_PREPEND_NAMESPACE(identifier)
    \internal

    This macro qualifies \a identifier with the full namespace.
    It expands to \c{::QT_NAMESPACE::identifier} if \c QT_NAMESPACE is defined
    and only \a identifier otherwise.

    \sa QT_NAMESPACE
*/

/*!
    \macro QT_USE_NAMESPACE
    \internal

    This macro expands to using QT_NAMESPACE if QT_NAMESPACE is defined
    and nothing otherwise.

    \sa QT_NAMESPACE
*/

/*!
    \macro QT_BEGIN_NAMESPACE
    \internal

    This macro expands to

    \snippet code/src_corelib_global_qglobal.cpp begin namespace macro

    if \c QT_NAMESPACE is defined and nothing otherwise. If should always
    appear in the file-level scope and be followed by \c QT_END_NAMESPACE
    at the same logical level with respect to preprocessor conditionals
    in the same file.

    As a rule of thumb, \c QT_BEGIN_NAMESPACE should appear in all Qt header
    and Qt source files after the last \c{#include} line and before the first
    declaration.

    If that rule can't be followed because, e.g., \c{#include} lines and
    declarations are wildly mixed, place \c QT_BEGIN_NAMESPACE before
    the first declaration and wrap the \c{#include} lines in
    \c QT_BEGIN_INCLUDE_NAMESPACE and \c QT_END_INCLUDE_NAMESPACE.

    When using the \c QT_NAMESPACE feature in user code
    (e.g., when building plugins statically linked to Qt) where
    the user code is not intended to go into the \c QT_NAMESPACE
    namespace, all forward declarations of Qt classes need to
    be wrapped in \c QT_BEGIN_NAMESPACE and \c QT_END_NAMESPACE.
    After that, a \c QT_USE_NAMESPACE should follow.
    No further changes should be needed.

    \sa QT_NAMESPACE
*/

/*!
    \macro QT_END_NAMESPACE
    \internal

    This macro expands to

    \snippet code/src_corelib_global_qglobal.cpp end namespace macro

    if \c QT_NAMESPACE is defined and nothing otherwise. It is used to cancel
    the effect of \c QT_BEGIN_NAMESPACE.

    If a source file ends with a \c{#include} directive that includes a moc file,
    \c QT_END_NAMESPACE should be placed before that \c{#include}.

    \sa QT_NAMESPACE
*/

/*!
    \macro QT_BEGIN_INCLUDE_NAMESPACE
    \internal

    This macro is equivalent to \c QT_END_NAMESPACE.
    It only serves as syntactic sugar and is intended
    to be used before #include lines within a
    \c QT_BEGIN_NAMESPACE ... \c QT_END_NAMESPACE block.

    \sa QT_NAMESPACE
*/

/*!
    \macro QT_END_INCLUDE_NAMESPACE
    \internal

    This macro is equivalent to \c QT_BEGIN_NAMESPACE.
    It only serves as syntactic sugar and is intended
    to be used after #include lines within a
    \c QT_BEGIN_NAMESPACE ... \c QT_END_NAMESPACE block.

    \sa QT_NAMESPACE
*/

/*!
    \macro QT_BEGIN_MOC_NAMESPACE
    \internal

    This macro is output by moc at the beginning of
    moc files. It is equivalent to \c QT_USE_NAMESPACE.

    \sa QT_NAMESPACE
*/

/*!
    \macro QT_END_MOC_NAMESPACE
    \internal

    This macro is output by moc at the beginning of
    moc files. It expands to nothing.

    \sa QT_NAMESPACE
*/

/*!
 \fn bool qFuzzyCompare(double p1, double p2)
 \relates <QtGlobal>
 \since 4.4
 \threadsafe

 Compares the floating point value \a p1 and \a p2 and
 returns \c true if they are considered equal, otherwise \c false.

 Note that comparing values where either \a p1 or \a p2 is 0.0 will not work.
 The solution to this is to compare against values greater than or equal to 1.0.

 \snippet code/src_corelib_global_qglobal.cpp 46

 The two numbers are compared in a relative way, where the
 exactness is stronger the smaller the numbers are.
 */

/*!
 \fn bool qFuzzyCompare(float p1, float p2)
 \relates <QtGlobal>
 \since 4.4
 \threadsafe

 Compares the floating point value \a p1 and \a p2 and
 returns \c true if they are considered equal, otherwise \c false.

 The two numbers are compared in a relative way, where the
 exactness is stronger the smaller the numbers are.
 */

/*!
    \macro QT_REQUIRE_VERSION(int argc, char **argv, const char *version)
    \relates <QtGlobal>

    This macro can be used to ensure that the application is run
    against a recent enough version of Qt. This is especially useful
    if your application depends on a specific bug fix introduced in a
    bug-fix release (e.g., 4.0.2).

    The \a argc and \a argv parameters are the \c main() function's
    \c argc and \c argv parameters. The \a version parameter is a
    string literal that specifies which version of Qt the application
    requires (e.g., "4.0.2").

    Example:

    \snippet code/src_gui_dialogs_qmessagebox.cpp 4
*/

/*!
    \macro Q_DECL_EXPORT
    \relates <QtGlobal>

    This macro marks a symbol for shared library export (see
     \l{sharedlibrary.html}{Creating Shared Libraries}).

    \sa Q_DECL_IMPORT
*/

/*!
    \macro Q_DECL_IMPORT
    \relates <QtGlobal>

    This macro declares a symbol to be an import from a shared library (see
    \l{sharedlibrary.html}{Creating Shared Libraries}).

    \sa Q_DECL_EXPORT
*/

/*!
    \macro Q_DECL_CONSTEXPR
    \relates <QtGlobal>

    This macro can be used to declare variable that should be constructed at compile-time,
    or an inline function that can be computed at compile-time.

    It expands to "constexpr" if your compiler supports that C++11 keyword, or to nothing
    otherwise.
*/

/*!
    \macro qDebug(const char *message, ...)
    \relates <QtGlobal>

    Calls the message handler with the debug message \a message. If no
    message handler has been installed, the message is printed to
    stderr. Under Windows, the message is sent to the console, if it is a
    console application; otherwise, it is sent to the debugger. On Blackberry the
    message is sent to slogger2. This function does nothing if \c QT_NO_DEBUG_OUTPUT
    was defined during compilation.

    If you pass the function a format string and a list of arguments,
    it works in similar way to the C printf() function. The format
    should be a Latin-1 string.

    Example:

    \snippet code/src_corelib_global_qglobal.cpp 24

    If you include \c <QtDebug>, a more convenient syntax is also
    available:

    \snippet code/src_corelib_global_qglobal.cpp 25

    With this syntax, the function returns a QDebug object that is
    configured to use the QtDebugMsg message type. It automatically
    puts a single space between each item, and outputs a newline at
    the end. It supports many C++ and Qt types.

    To suppress the output at run-time, install your own message handler
    with qInstallMessageHandler().

    \sa qWarning(), qCritical(), qFatal(), qInstallMessageHandler(),
        {Debugging Techniques}
*/

/*!
    \macro qWarning(const char *message, ...)
    \relates <QtGlobal>

    Calls the message handler with the warning message \a message. If no
    message handler has been installed, the message is printed to
    stderr. Under Windows, the message is sent to the debugger.
    On Blackberry the message is sent to slogger2. This
    function does nothing if \c QT_NO_WARNING_OUTPUT was defined
    during compilation; it exits if the environment variable \c
    QT_FATAL_WARNINGS is not empty.

    This function takes a format string and a list of arguments,
    similar to the C printf() function. The format should be a Latin-1
    string.

    Example:
    \snippet code/src_corelib_global_qglobal.cpp 26

    If you include <QtDebug>, a more convenient syntax is
    also available:

    \snippet code/src_corelib_global_qglobal.cpp 27

    This syntax inserts a space between each item, and
    appends a newline at the end.

    To suppress the output at runtime, install your own message handler
    with qInstallMessageHandler().

    \sa qDebug(), qCritical(), qFatal(), qInstallMessageHandler(),
        {Debugging Techniques}
*/

/*!
    \macro qCritical(const char *message, ...)
    \relates <QtGlobal>

    Calls the message handler with the critical message \a message. If no
    message handler has been installed, the message is printed to
    stderr. Under Windows, the message is sent to the debugger.
    On Blackberry the message is sent to slogger2.

    It exits if the environment variable QT_FATAL_CRITICALS is not empty.

    This function takes a format string and a list of arguments,
    similar to the C printf() function. The format should be a Latin-1
    string.

    Example:
    \snippet code/src_corelib_global_qglobal.cpp 28

    If you include <QtDebug>, a more convenient syntax is
    also available:

    \snippet code/src_corelib_global_qglobal.cpp 29

    A space is inserted between the items, and a newline is
    appended at the end.

    To suppress the output at runtime, install your own message handler
    with qInstallMessageHandler().

    \sa qDebug(), qWarning(), qFatal(), qInstallMessageHandler(),
        {Debugging Techniques}
*/

/*!
    \macro qFatal(const char *message, ...)
    \relates <QtGlobal>

    Calls the message handler with the fatal message \a message. If no
    message handler has been installed, the message is printed to
    stderr. Under Windows, the message is sent to the debugger.
    On Blackberry the message is sent to slogger2.

    If you are using the \b{default message handler} this function will
    abort on Unix systems to create a core dump. On Windows, for debug builds,
    this function will report a _CRT_ERROR enabling you to connect a debugger
    to the application.

    This function takes a format string and a list of arguments,
    similar to the C printf() function.

    Example:
    \snippet code/src_corelib_global_qglobal.cpp 30

    To suppress the output at runtime, install your own message handler
    with qInstallMessageHandler().

    \sa qDebug(), qCritical(), qWarning(), qInstallMessageHandler(),
        {Debugging Techniques}
*/

/*!
    \macro qMove(x)
    \relates <QtGlobal>

    It expands to "std::move" if your compiler supports that C++11 function, or to nothing
    otherwise.

    qMove takes an rvalue reference to its parameter \a x, and converts it to an xvalue.
*/

/*!
    \macro Q_DECL_NOTHROW
    \relates <QtGlobal>
    \since 5.0

    This macro marks a function as never throwing, under no
    circumstances. If the function does nevertheless throw, the
    behaviour is undefined.

    The macro expands to either "throw()", if that has some benefit on
    the compiler, or to C++11 noexcept, if available, or to nothing
    otherwise.

    If you need C++11 noexcept semantics, don't use this macro, use
    Q_DECL_NOEXCEPT/Q_DECL_NOEXCEPT_EXPR instead.

    \sa Q_DECL_NOEXCEPT, Q_DECL_NOEXCEPT_EXPR()
*/

/*!
    \macro QT_TERMINATE_ON_EXCEPTION(expr)
    \relates <QtGlobal>
    \internal

    In general, use of the Q_DECL_NOEXCEPT macro is preferred over
    Q_DECL_NOTHROW, because it exhibits well-defined behavior and
    supports the more powerful Q_DECL_NOEXCEPT_EXPR variant. However,
    use of Q_DECL_NOTHROW has the advantage that Windows builds
    benefit on a wide range or compiler versions that do not yet
    support the C++11 noexcept feature.

    It may therefore be beneficial to use Q_DECL_NOTHROW and emulate
    the C++11 behavior manually with an embedded try/catch.

    Qt provides the QT_TERMINATE_ON_EXCEPTION(expr) macro for this
    purpose. It either expands to \c expr (if Qt is compiled without
    exception support or the compiler supports C++11 noexcept
    semantics) or to
    \code
    try { expr; } catch(...) { qTerminate(); }
    \endcode
    otherwise.

    Since this macro expands to just \c expr if the compiler supports
    C++11 noexcept, expecting the compiler to take over responsibility
    of calling std::terminate() in that case, it should not be used
    outside Q_DECL_NOTHROW functions.

    \sa Q_DECL_NOEXCEPT, Q_DECL_NOTHROW, qTerminate()
*/

/*!
    \macro Q_DECL_NOEXCEPT
    \relates <QtGlobal>
    \since 5.0

    This macro marks a function as never throwing. If the function
    does nevertheless throw, the behaviour is defined:
    std::terminate() is called.

    The macro expands to C++11 noexcept, if available, or to nothing
    otherwise.

    If you need the operator version of C++11 noexcept, use
    Q_DECL_NOEXCEPT_EXPR(x).

    If you don't need C++11 noexcept semantics, e.g. because your
    function can't possibly throw, don't use this macro, use
    Q_DECL_NOTHROW instead.

    \sa Q_DECL_NOTHROW, Q_DECL_NOEXCEPT_EXPR()
*/

/*!
    \macro Q_DECL_NOEXCEPT_EXPR(x)
    \relates <QtGlobal>
    \since 5.0

    This macro marks a function as non-throwing if \a x is \c true. If
    the function does nevertheless throw, the behaviour is defined:
    std::terminate() is called.

    The macro expands to C++11 noexcept(x), if available, or to
    nothing otherwise.

    If you need the always-true version of C++11 noexcept, use
    Q_DECL_NOEXCEPT.

    If you don't need C++11 noexcept semantics, e.g. because your
    function can't possibly throw, don't use this macro, use
    Q_DECL_NOTHROW instead.

    \sa Q_DECL_NOTHROW, Q_DECL_NOEXCEPT
*/

/*!
    \macro Q_DECL_OVERRIDE
    \since 5.0
    \relates <QtGlobal>

    This macro can be used to declare an overriding virtual
    function. Use of this markup will allow the compiler to generate
    an error if the overriding virtual function does not in fact
    override anything.

    It expands to "override" if your compiler supports that C++11
    contextual keyword, or to nothing otherwise.

    The macro goes at the end of the function, usually after the
    \c{const}, if any:
    \code
    // generate error if this doesn't actually override anything:
    virtual void MyWidget::paintEvent(QPaintEvent*) Q_DECL_OVERRIDE;
    \endcode

    \sa Q_DECL_FINAL
*/

/*!
    \macro Q_DECL_FINAL
    \since 5.0
    \relates <QtGlobal>

    This macro can be used to declare an overriding virtual or a class
    as "final", with Java semantics. Further-derived classes can then
    no longer override this virtual function, or inherit from this
    class, respectively.

    It expands to "final" if your compiler supports that C++11
    contextual keyword, or something non-standard if your compiler
    supports something close enough to the C++11 semantics, or to
    nothing otherwise.

    The macro goes at the end of the function, usually after the
    \c{const}, if any:
    \code
    // more-derived classes no longer permitted to override this:
    virtual void MyWidget::paintEvent(QPaintEvent*) Q_DECL_FINAL;
    \endcode

    For classes, it goes in front of the \c{:} in the class
    definition, if any:
    \code
    class QRect Q_DECL_FINAL { // cannot be derived from
        // ...
    };
    \endcode

    \sa Q_DECL_OVERRIDE
*/

/*!
    \macro Q_FORWARD_DECLARE_OBJC_CLASS(classname)
    \since 5.2
    \relates <QtGlobal>

    Forward-declares an Objective-C \a classname in a manner such that it can be
    compiled as either Objective-C or C++.

    This is primarily intended for use in header files that may be included by
    both Objective-C and C++ source files.
*/

/*!
    \macro Q_FORWARD_DECLARE_CF_TYPE(type)
    \since 5.2
    \relates <QtGlobal>

    Forward-declares a Core Foundation \a type. This includes the actual
    type and the ref type. For example, Q_FORWARD_DECLARE_CF_TYPE(CFString)
    declares __CFString and CFStringRef.
*/

/*!
    \macro Q_FORWARD_DECLARE_MUTABLE_CF_TYPE(type)
    \since 5.2
    \relates <QtGlobal>

    Forward-declares a mutable Core Foundation \a type. This includes the actual
    type and the ref type. For example, Q_FORWARD_DECLARE_CF_TYPE(CFString)
    declares __CFMutableString and CFMutableStringRef.
*/

QT_END_NAMESPACE
