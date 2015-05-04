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

#ifndef QBITARRAY_H
#define QBITARRAY_H

#include <QtCore/qbytearray.h>

QT_BEGIN_NAMESPACE


class QBitRef;
class Q_CORE_EXPORT QBitArray
{
    friend Q_CORE_EXPORT QDataStream &operator<<(QDataStream &, const QBitArray &);
    friend Q_CORE_EXPORT QDataStream &operator>>(QDataStream &, QBitArray &);
    friend Q_CORE_EXPORT uint qHash(const QBitArray &key, uint seed) Q_DECL_NOTHROW;
    QByteArray d;

public:
    inline QBitArray() {}
    explicit QBitArray(int size, bool val = false);
    QBitArray(const QBitArray &other) : d(other.d) {}
    inline QBitArray &operator=(const QBitArray &other) { d = other.d; return *this; }
#ifdef Q_COMPILER_RVALUE_REFS
    inline QBitArray(QBitArray &&other) : d(std::move(other.d)) {}
    inline QBitArray &operator=(QBitArray &&other)
    { qSwap(d, other.d); return *this; }
#endif

    inline void swap(QBitArray &other) { qSwap(d, other.d); }

    inline int size() const { return (d.size() << 3) - *d.constData(); }
    inline int count() const { return (d.size() << 3) - *d.constData(); }
    int count(bool on) const;

    inline bool isEmpty() const { return d.isEmpty(); }
    inline bool isNull() const { return d.isNull(); }

    void resize(int size);

    inline void detach() { d.detach(); }
    inline bool isDetached() const { return d.isDetached(); }
    inline void clear() { d.clear(); }

    bool testBit(int i) const;
    void setBit(int i);
    void setBit(int i, bool val);
    void clearBit(int i);
    bool toggleBit(int i);

    bool at(int i) const;
    QBitRef operator[](int i);
    bool operator[](int i) const;
    QBitRef operator[](uint i);
    bool operator[](uint i) const;

    QBitArray& operator&=(const QBitArray &);
    QBitArray& operator|=(const QBitArray &);
    QBitArray& operator^=(const QBitArray &);
    QBitArray  operator~() const;

    inline bool operator==(const QBitArray& other) const { return d == other.d; }
    inline bool operator!=(const QBitArray& other) const { return d != other.d; }

    inline bool fill(bool val, int size = -1);
    void fill(bool val, int first, int last);

    inline void truncate(int pos) { if (pos < size()) resize(pos); }

public:
    typedef QByteArray::DataPtr DataPtr;
    inline DataPtr &data_ptr() { return d.data_ptr(); }
};

inline bool QBitArray::fill(bool aval, int asize)
{ *this = QBitArray((asize < 0 ? this->size() : asize), aval); return true; }

Q_CORE_EXPORT QBitArray operator&(const QBitArray &, const QBitArray &);
Q_CORE_EXPORT QBitArray operator|(const QBitArray &, const QBitArray &);
Q_CORE_EXPORT QBitArray operator^(const QBitArray &, const QBitArray &);

inline bool QBitArray::testBit(int i) const
{ Q_ASSERT(uint(i) < uint(size()));
 return (*(reinterpret_cast<const uchar*>(d.constData())+1+(i>>3)) & (1 << (i & 7))) != 0; }

inline void QBitArray::setBit(int i)
{ Q_ASSERT(uint(i) < uint(size()));
 *(reinterpret_cast<uchar*>(d.data())+1+(i>>3)) |= uchar(1 << (i & 7)); }

inline void QBitArray::clearBit(int i)
{ Q_ASSERT(uint(i) < uint(size()));
 *(reinterpret_cast<uchar*>(d.data())+1+(i>>3)) &= ~uchar(1 << (i & 7)); }

inline void QBitArray::setBit(int i, bool val)
{ if (val) setBit(i); else clearBit(i); }

inline bool QBitArray::toggleBit(int i)
{ Q_ASSERT(uint(i) < uint(size()));
 uchar b = uchar(1<<(i&7)); uchar* p = reinterpret_cast<uchar*>(d.data())+1+(i>>3);
 uchar c = uchar(*p&b); *p^=b; return c!=0; }

inline bool QBitArray::operator[](int i) const { return testBit(i); }
inline bool QBitArray::operator[](uint i) const { return testBit(i); }
inline bool QBitArray::at(int i) const { return testBit(i); }

class Q_CORE_EXPORT QBitRef
{
private:
    QBitArray& a;
    int i;
    inline QBitRef(QBitArray& array, int idx) : a(array), i(idx) {}
    friend class QBitArray;
public:
    inline operator bool() const { return a.testBit(i); }
    inline bool operator!() const { return !a.testBit(i); }
    QBitRef& operator=(const QBitRef& val) { a.setBit(i, val); return *this; }
    QBitRef& operator=(bool val) { a.setBit(i, val); return *this; }
};

inline QBitRef QBitArray::operator[](int i)
{ Q_ASSERT(i >= 0); return QBitRef(*this, i); }
inline QBitRef QBitArray::operator[](uint i)
{ return QBitRef(*this, i); }


#ifndef QT_NO_DATASTREAM
Q_CORE_EXPORT QDataStream &operator<<(QDataStream &, const QBitArray &);
Q_CORE_EXPORT QDataStream &operator>>(QDataStream &, QBitArray &);
#endif

#ifndef QT_NO_DEBUG_STREAM
Q_CORE_EXPORT QDebug operator<<(QDebug, const QBitArray &);
#endif

Q_DECLARE_SHARED(QBitArray)

QT_END_NAMESPACE

#endif // QBITARRAY_H
