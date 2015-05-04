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

#include <QtCore/qglobal.h>

#ifndef QFLAGS_H
#define QFLAGS_H

#include <QtCore/qtypeinfo.h>
#include <QtCore/qtypetraits.h>

#ifdef Q_COMPILER_INITIALIZER_LISTS
#include <initializer_list>
#endif

QT_BEGIN_NAMESPACE

class QFlag
{
    int i;
public:
    Q_DECL_CONSTEXPR inline QFlag(int ai) : i(ai) {}
    Q_DECL_CONSTEXPR inline operator int() const { return i; }

#if !defined(Q_CC_MSVC)
    // Microsoft Visual Studio has buggy behavior when it comes to
    // unsigned enums: even if the enum is unsigned, the enum tags are
    // always signed
#  if !defined(__LP64__) && !defined(Q_QDOC)
    Q_DECL_CONSTEXPR inline QFlag(long ai) : i(int(ai)) {}
    Q_DECL_CONSTEXPR inline QFlag(ulong ai) : i(int(long(ai))) {}
#  endif
    Q_DECL_CONSTEXPR inline QFlag(uint ai) : i(int(ai)) {}
    Q_DECL_CONSTEXPR inline QFlag(short ai) : i(int(ai)) {}
    Q_DECL_CONSTEXPR inline QFlag(ushort ai) : i(int(uint(ai))) {}
    Q_DECL_CONSTEXPR inline operator uint() const { return uint(i); }
#endif
};
Q_DECLARE_TYPEINFO(QFlag, Q_PRIMITIVE_TYPE);

class QIncompatibleFlag
{
    int i;
public:
    Q_DECL_CONSTEXPR inline explicit QIncompatibleFlag(int i);
    Q_DECL_CONSTEXPR inline operator int() const { return i; }
};
Q_DECLARE_TYPEINFO(QIncompatibleFlag, Q_PRIMITIVE_TYPE);

Q_DECL_CONSTEXPR inline QIncompatibleFlag::QIncompatibleFlag(int ai) : i(ai) {}


#ifndef Q_NO_TYPESAFE_FLAGS

template<typename Enum>
class QFlags
{
    Q_STATIC_ASSERT_X((sizeof(Enum) <= sizeof(int)),
                      "QFlags uses an int as storage, so an enum with underlying "
                      "long long will overflow.");
    struct Private;
    typedef int (Private::*Zero);
public:
#if defined(Q_CC_MSVC) || defined(Q_QDOC)
    // see above for MSVC
    // the definition below is too complex for qdoc
    typedef int Int;
#else
    typedef typename QtPrivate::if_<
            QtPrivate::is_unsigned<Enum>::value,
            unsigned int,
            signed int
        >::type Int;
#endif
    typedef Enum enum_type;
    // compiler-generated copy/move ctor/assignment operators are fine!
#ifdef Q_QDOC
    inline QFlags(const QFlags &other);
    inline QFlags &operator=(const QFlags &other);
#endif
    Q_DECL_CONSTEXPR inline QFlags(Enum f) : i(Int(f)) {}
    Q_DECL_CONSTEXPR inline QFlags(Zero = 0) : i(0) {}
    Q_DECL_CONSTEXPR inline QFlags(QFlag f) : i(f) {}

#ifdef Q_COMPILER_INITIALIZER_LISTS
    Q_DECL_CONSTEXPR inline QFlags(std::initializer_list<Enum> flags)
        : i(initializer_list_helper(flags.begin(), flags.end())) {}
#endif

    inline QFlags &operator&=(int mask) { i &= mask; return *this; }
    inline QFlags &operator&=(uint mask) { i &= mask; return *this; }
    inline QFlags &operator&=(Enum mask) { i &= Int(mask); return *this; }
    inline QFlags &operator|=(QFlags f) { i |= f.i; return *this; }
    inline QFlags &operator|=(Enum f) { i |= Int(f); return *this; }
    inline QFlags &operator^=(QFlags f) { i ^= f.i; return *this; }
    inline QFlags &operator^=(Enum f) { i ^= Int(f); return *this; }

    Q_DECL_CONSTEXPR  inline operator Int() const { return i; }

    Q_DECL_CONSTEXPR inline QFlags operator|(QFlags f) const { return QFlags(QFlag(i | f.i)); }
    Q_DECL_CONSTEXPR inline QFlags operator|(Enum f) const { return QFlags(QFlag(i | Int(f))); }
    Q_DECL_CONSTEXPR inline QFlags operator^(QFlags f) const { return QFlags(QFlag(i ^ f.i)); }
    Q_DECL_CONSTEXPR inline QFlags operator^(Enum f) const { return QFlags(QFlag(i ^ Int(f))); }
    Q_DECL_CONSTEXPR inline QFlags operator&(int mask) const { return QFlags(QFlag(i & mask)); }
    Q_DECL_CONSTEXPR inline QFlags operator&(uint mask) const { return QFlags(QFlag(i & mask)); }
    Q_DECL_CONSTEXPR inline QFlags operator&(Enum f) const { return QFlags(QFlag(i & Int(f))); }
    Q_DECL_CONSTEXPR inline QFlags operator~() const { return QFlags(QFlag(~i)); }

    Q_DECL_CONSTEXPR inline bool operator!() const { return !i; }

    Q_DECL_CONSTEXPR inline bool testFlag(Enum f) const { return (i & Int(f)) == Int(f) && (Int(f) != 0 || i == Int(f) ); }
private:
#ifdef Q_COMPILER_INITIALIZER_LISTS
    Q_DECL_CONSTEXPR static inline Int initializer_list_helper(typename std::initializer_list<Enum>::const_iterator it,
                                                               typename std::initializer_list<Enum>::const_iterator end)
    {
        return (it == end ? Int(0) : (Int(*it) | initializer_list_helper(it + 1, end)));
    }
#endif

    Int i;
};

#define Q_DECLARE_FLAGS(Flags, Enum)\
typedef QFlags<Enum> Flags;

#define Q_DECLARE_INCOMPATIBLE_FLAGS(Flags) \
Q_DECL_CONSTEXPR inline QIncompatibleFlag operator|(Flags::enum_type f1, int f2) \
{ return QIncompatibleFlag(int(f1) | f2); }

#define Q_DECLARE_OPERATORS_FOR_FLAGS(Flags) \
Q_DECL_CONSTEXPR inline QFlags<Flags::enum_type> operator|(Flags::enum_type f1, Flags::enum_type f2) \
{ return QFlags<Flags::enum_type>(f1) | f2; } \
Q_DECL_CONSTEXPR inline QFlags<Flags::enum_type> operator|(Flags::enum_type f1, QFlags<Flags::enum_type> f2) \
{ return f2 | f1; } Q_DECLARE_INCOMPATIBLE_FLAGS(Flags)


#else /* Q_NO_TYPESAFE_FLAGS */

#define Q_DECLARE_FLAGS(Flags, Enum)\
typedef uint Flags;
#define Q_DECLARE_OPERATORS_FOR_FLAGS(Flags)

#endif /* Q_NO_TYPESAFE_FLAGS */

QT_END_NAMESPACE

#endif // QFLAGS_H
