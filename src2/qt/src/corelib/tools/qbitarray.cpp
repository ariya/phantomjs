/****************************************************************************
**
** Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
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

#include "qbitarray.h"
#include <qdatastream.h>
#include <qdebug.h>
#include <string.h>

QT_BEGIN_NAMESPACE

/*!
    \class QBitArray
    \brief The QBitArray class provides an array of bits.

    \ingroup tools
    \ingroup shared
    \reentrant

    A QBitArray is an array that gives access to individual bits and
    provides operators (\link operator&() AND\endlink, \link
    operator|() OR\endlink, \link operator^() XOR\endlink, and \link
    operator~() NOT\endlink) that work on entire arrays of bits. It
    uses \l{implicit sharing} (copy-on-write) to reduce memory usage
    and to avoid the needless copying of data.

    The following code constructs a QBitArray containing 200 bits
    initialized to false (0):

    \snippet doc/src/snippets/code/src_corelib_tools_qbitarray.cpp 0

    To initialize the bits to true, either pass \c true as second
    argument to the constructor, or call fill() later on.

    QBitArray uses 0-based indexes, just like C++ arrays. To access
    the bit at a particular index position, you can use operator[]().
    On non-const bit arrays, operator[]() returns a reference to a
    bit that can be used on the left side of an assignment. For
    example:

    \snippet doc/src/snippets/code/src_corelib_tools_qbitarray.cpp 1

    For technical reasons, it is more efficient to use testBit() and
    setBit() to access bits in the array than operator[](). For
    example:

    \snippet doc/src/snippets/code/src_corelib_tools_qbitarray.cpp 2

    QBitArray supports \c{&} (\link operator&() AND\endlink), \c{|}
    (\link operator|() OR\endlink), \c{^} (\link operator^()
    XOR\endlink), \c{~} (\link operator~() NOT\endlink), as well as
    \c{&=}, \c{|=}, and \c{^=}. These operators work in the same way
    as the built-in C++ bitwise operators of the same name. For
    example:

    \snippet doc/src/snippets/code/src_corelib_tools_qbitarray.cpp 3

    For historical reasons, QBitArray distinguishes between a null
    bit array and an empty bit array. A \e null bit array is a bit
    array that is initialized using QBitArray's default constructor.
    An \e empty bit array is any bit array with size 0. A null bit
    array is always empty, but an empty bit array isn't necessarily
    null:

    \snippet doc/src/snippets/code/src_corelib_tools_qbitarray.cpp 4

    All functions except isNull() treat null bit arrays the same as
    empty bit arrays; for example, QBitArray() compares equal to
    QBitArray(0). We recommend that you always use isEmpty() and
    avoid isNull().

    \sa QByteArray, QVector
*/

/*! \fn QBitArray::QBitArray()

    Constructs an empty bit array.

    \sa isEmpty()
*/

/*!
    Constructs a bit array containing \a size bits. The bits are
    initialized with \a value, which defaults to false (0).
*/
QBitArray::QBitArray(int size, bool value)
{
    if (!size) {
        d.resize(0);
        return;
    }
    d.resize(1 + (size+7)/8);
    uchar* c = reinterpret_cast<uchar*>(d.data());
    memset(c, value ? 0xff : 0, d.size());
    *c = d.size()*8 - size;
    if (value && size && size % 8)
        *(c+1+size/8) &= (1 << (size%8)) - 1;
}

/*! \fn int QBitArray::size() const

    Returns the number of bits stored in the bit array.

    \sa resize()
*/

/*! \fn int QBitArray::count() const

    Same as size().
*/

/*!
    If \a on is true, this function returns the number of
    1-bits stored in the bit array; otherwise the number
    of 0-bits is returned.
*/
int QBitArray::count(bool on) const
{
    int numBits = 0;
    int len = size();
#if 0
    for (int i = 0; i < len; ++i)
        numBits += testBit(i);
#else
    // See http://graphics.stanford.edu/~seander/bithacks.html#CountBitsSetParallel
    const quint8 *bits = reinterpret_cast<const quint8 *>(d.data()) + 1;
    while (len >= 32) {
        quint32 v = quint32(bits[0]) | (quint32(bits[1]) << 8) | (quint32(bits[2]) << 16) | (quint32(bits[3]) << 24);
        quint32 c = ((v & 0xfff) * Q_UINT64_C(0x1001001001001) & Q_UINT64_C(0x84210842108421)) % 0x1f;
        c += (((v & 0xfff000) >> 12) * Q_UINT64_C(0x1001001001001) & Q_UINT64_C(0x84210842108421)) % 0x1f;
        c += ((v >> 24) * Q_UINT64_C(0x1001001001001) & Q_UINT64_C(0x84210842108421)) % 0x1f;
        len -= 32;
        bits += 4;
        numBits += int(c);
    }
    while (len >= 24) {
        quint32 v = quint32(bits[0]) | (quint32(bits[1]) << 8) | (quint32(bits[2]) << 16);
        quint32 c =  ((v & 0xfff) * Q_UINT64_C(0x1001001001001) & Q_UINT64_C(0x84210842108421)) % 0x1f;
        c += (((v & 0xfff000) >> 12) * Q_UINT64_C(0x1001001001001) & Q_UINT64_C(0x84210842108421)) % 0x1f;    
        len -= 24;
        bits += 3;
        numBits += int(c);
    }
    while (len >= 0) {
        if (bits[len / 8] & (1 << ((len - 1) & 7)))
            ++numBits;
        --len;
    }
#endif
    return on ? numBits : size() - numBits;
}

/*!
    Resizes the bit array to \a size bits.

    If \a size is greater than the current size, the bit array is
    extended to make it \a size bits with the extra bits added to the
    end. The new bits are initialized to false (0).

    If \a size is less than the current size, bits are removed from
    the end.

    \sa size()
*/
void QBitArray::resize(int size)
{
    if (!size) {
        d.resize(0);
    } else {
        int s = d.size();
        d.resize(1 + (size+7)/8);
        uchar* c = reinterpret_cast<uchar*>(d.data());
        if (size > (s << 3))
            memset(c + s, 0, d.size() - s);
        else if ( size % 8)
            *(c+1+size/8) &= (1 << (size%8)) - 1;
        *c = d.size()*8 - size;
    }
}

/*! \fn bool QBitArray::isEmpty() const

    Returns true if this bit array has size 0; otherwise returns
    false.

    \sa size()
*/

/*! \fn bool QBitArray::isNull() const

    Returns true if this bit array is null; otherwise returns false.

    Example:
    \snippet doc/src/snippets/code/src_corelib_tools_qbitarray.cpp 5

    Qt makes a distinction between null bit arrays and empty bit
    arrays for historical reasons. For most applications, what
    matters is whether or not a bit array contains any data,
    and this can be determined using isEmpty().

    \sa isEmpty()
*/

/*! \fn bool QBitArray::fill(bool value, int size = -1)

    Sets every bit in the bit array to \a value, returning true if successful;
    otherwise returns false. If \a size is different from -1 (the default),
    the bit array is resized to \a size beforehand.

    Example:
    \snippet doc/src/snippets/code/src_corelib_tools_qbitarray.cpp 6

    \sa resize()
*/

/*!
    \overload

    Sets bits at index positions \a begin up to and excluding \a end
    to \a value.

    \a begin and \a end must be a valid index position in the bit
    array (i.e., 0 <= \a begin <= size() and 0 <= \a end <= size()).
*/

void QBitArray::fill(bool value, int begin, int end)
{
    while (begin < end && begin & 0x7)
        setBit(begin++, value);
    int len = end - begin;
    if (len <= 0)
        return;
    int s = len & ~0x7;
    uchar *c = reinterpret_cast<uchar*>(d.data());
    memset(c + (begin >> 3) + 1, value ? 0xff : 0, s >> 3);
    begin += s;
    while (begin < end)
        setBit(begin++, value);
}

/*! \fn bool QBitArray::isDetached() const

    \internal
*/

/*! \fn void QBitArray::detach()

    \internal
*/

/*! \fn void QBitArray::clear()

    Clears the contents of the bit array and makes it empty.

    \sa resize(), isEmpty()
*/

/*! \fn void QBitArray::truncate(int pos)

    Truncates the bit array at index position \a pos.

    If \a pos is beyond the end of the array, nothing happens.

    \sa resize()
*/

/*! \fn bool QBitArray::toggleBit(int i)

    Inverts the value of the bit at index position \a i, returning the
    previous value of that bit as either true (if it was set) or false (if
    it was unset).

    If the previous value was 0, the new value will be 1. If the
    previous value was 1, the new value will be 0.

    \a i must be a valid index position in the bit array (i.e., 0 <=
    \a i < size()).

    \sa setBit(), clearBit()
*/

/*! \fn bool QBitArray::testBit(int i) const

    Returns true if the bit at index position \a i is 1; otherwise
    returns false.

    \a i must be a valid index position in the bit array (i.e., 0 <=
    \a i < size()).

    \sa setBit(), clearBit()
*/

/*! \fn bool QBitArray::setBit(int i)

    Sets the bit at index position \a i to 1.

    \a i must be a valid index position in the bit array (i.e., 0 <=
    \a i < size()).

    \sa clearBit(), toggleBit()
*/

/*! \fn void QBitArray::setBit(int i, bool value)

    \overload

    Sets the bit at index position \a i to \a value.
*/

/*! \fn void QBitArray::clearBit(int i)

    Sets the bit at index position \a i to 0.

    \a i must be a valid index position in the bit array (i.e., 0 <=
    \a i < size()).

    \sa setBit(), toggleBit()
*/

/*! \fn bool QBitArray::at(int i) const

    Returns the value of the bit at index position \a i.

    \a i must be a valid index position in the bit array (i.e., 0 <=
    \a i < size()).

    \sa operator[]()
*/

/*! \fn QBitRef QBitArray::operator[](int i)

    Returns the bit at index position \a i as a modifiable reference.

    \a i must be a valid index position in the bit array (i.e., 0 <=
    \a i < size()).

    Example:
    \snippet doc/src/snippets/code/src_corelib_tools_qbitarray.cpp 7

    The return value is of type QBitRef, a helper class for QBitArray.
    When you get an object of type QBitRef, you can assign to
    it, and the assignment will apply to the bit in the QBitArray
    from which you got the reference.

    The functions testBit(), setBit(), and clearBit() are slightly
    faster.

    \sa at(), testBit(), setBit(), clearBit()
*/

/*! \fn bool QBitArray::operator[](int i) const

    \overload
*/

/*! \fn bool QBitArray::operator[](uint i)

    \overload
*/

/*! \fn bool QBitArray::operator[](uint i) const

    \overload
*/

/*! \fn QBitArray::QBitArray(const QBitArray &other)

    Constructs a copy of \a other.

    This operation takes \l{constant time}, because QBitArray is
    \l{implicitly shared}. This makes returning a QBitArray from a
    function very fast. If a shared instance is modified, it will be
    copied (copy-on-write), and that takes \l{linear time}.

    \sa operator=()
*/

/*! \fn QBitArray &QBitArray::operator=(const QBitArray &other)

    Assigns \a other to this bit array and returns a reference to
    this bit array.
*/

/*! \fn void QBitArray::swap(QBitArray &other)
    \since 4.8

    Swaps bit array \a other with this bit array. This operation is very
    fast and never fails.
*/

/*! \fn bool QBitArray::operator==(const QBitArray &other) const

    Returns true if \a other is equal to this bit array; otherwise
    returns false.

    \sa operator!=()
*/

/*! \fn bool QBitArray::operator!=(const QBitArray &other) const

    Returns true if \a other is not equal to this bit array;
    otherwise returns false.

    \sa operator==()
*/

/*!
    Performs the AND operation between all bits in this bit array and
    \a other. Assigns the result to this bit array, and returns a
    reference to it.

    The result has the length of the longest of the two bit arrays,
    with any missing bits (if one array is shorter than the other)
    taken to be 0.

    Example:
    \snippet doc/src/snippets/code/src_corelib_tools_qbitarray.cpp 8

    \sa operator&(), operator|=(), operator^=(), operator~()
*/

QBitArray &QBitArray::operator&=(const QBitArray &other)
{
    resize(qMax(size(), other.size()));
    uchar *a1 = reinterpret_cast<uchar*>(d.data()) + 1;
    const uchar *a2 = reinterpret_cast<const uchar*>(other.d.constData()) + 1;
    int n = other.d.size() -1 ; 
    int p = d.size() - 1 - n;
    while (n-- > 0)
        *a1++ &= *a2++;
    while (p-- > 0)
        *a1++ = 0;
    return *this;
}

/*!
    Performs the OR operation between all bits in this bit array and
    \a other. Assigns the result to this bit array, and returns a
    reference to it.

    The result has the length of the longest of the two bit arrays,
    with any missing bits (if one array is shorter than the other)
    taken to be 0.

    Example:
    \snippet doc/src/snippets/code/src_corelib_tools_qbitarray.cpp 9

    \sa operator|(), operator&=(), operator^=(), operator~()
*/

QBitArray &QBitArray::operator|=(const QBitArray &other)
{
    resize(qMax(size(), other.size()));
    uchar *a1 = reinterpret_cast<uchar*>(d.data()) + 1;
    const uchar *a2 = reinterpret_cast<const uchar *>(other.d.constData()) + 1;
    int n = other.d.size() - 1;   
    while (n-- > 0)
        *a1++ |= *a2++;
    return *this;
}

/*!
    Performs the XOR operation between all bits in this bit array and
    \a other. Assigns the result to this bit array, and returns a
    reference to it.

    The result has the length of the longest of the two bit arrays,
    with any missing bits (if one array is shorter than the other)
    taken to be 0.

    Example:
    \snippet doc/src/snippets/code/src_corelib_tools_qbitarray.cpp 10

    \sa operator^(), operator&=(), operator|=(), operator~()
*/

QBitArray &QBitArray::operator^=(const QBitArray &other)
{
    resize(qMax(size(), other.size()));
    uchar *a1 = reinterpret_cast<uchar*>(d.data()) + 1;
    const uchar *a2 = reinterpret_cast<const uchar *>(other.d.constData()) + 1;
    int n = other.d.size() - 1;
    while (n-- > 0)
        *a1++ ^= *a2++;
    return *this;
}

/*!
    Returns a bit array that contains the inverted bits of this bit
    array.

    Example:
    \snippet doc/src/snippets/code/src_corelib_tools_qbitarray.cpp 11

    \sa operator&(), operator|(), operator^()
*/

QBitArray QBitArray::operator~() const
{
    int sz = size();
    QBitArray a(sz);
    const uchar *a1 = reinterpret_cast<const uchar *>(d.constData()) + 1;
    uchar *a2 = reinterpret_cast<uchar*>(a.d.data()) + 1;
    int n = d.size() - 1;

    while (n-- > 0)
        *a2++ = ~*a1++;

    if (sz && sz%8)
        *(a2-1) &= (1 << (sz%8)) - 1;
    return a;
}

/*!
    \relates QBitArray

    Returns a bit array that is the AND of the bit arrays \a a1 and \a
    a2.

    The result has the length of the longest of the two bit arrays,
    with any missing bits (if one array is shorter than the other)
    taken to be 0.

    Example:
    \snippet doc/src/snippets/code/src_corelib_tools_qbitarray.cpp 12

    \sa QBitArray::operator&=(), operator|(), operator^()
*/

QBitArray operator&(const QBitArray &a1, const QBitArray &a2)
{
    QBitArray tmp = a1;
    tmp &= a2;
    return tmp;
}

/*!
    \relates QBitArray

    Returns a bit array that is the OR of the bit arrays \a a1 and \a
    a2.

    The result has the length of the longest of the two bit arrays,
    with any missing bits (if one array is shorter than the other)
    taken to be 0.

    Example:
    \snippet doc/src/snippets/code/src_corelib_tools_qbitarray.cpp 13

    \sa QBitArray::operator|=(), operator&(), operator^()
*/

QBitArray operator|(const QBitArray &a1, const QBitArray &a2)
{
    QBitArray tmp = a1;
    tmp |= a2;
    return tmp;
}

/*!
    \relates QBitArray

    Returns a bit array that is the XOR of the bit arrays \a a1 and \a
    a2.

    The result has the length of the longest of the two bit arrays,
    with any missing bits (if one array is shorter than the other)
    taken to be 0.

    Example:
    \snippet doc/src/snippets/code/src_corelib_tools_qbitarray.cpp 14

    \sa QBitArray::operator^=(), operator&(), operator|()
*/

QBitArray operator^(const QBitArray &a1, const QBitArray &a2)
{
    QBitArray tmp = a1;
    tmp ^= a2;
    return tmp;
}

/*!
    \class QBitRef
    \reentrant
    \brief The QBitRef class is an internal class, used with QBitArray.

    \internal

    The QBitRef is required by the indexing [] operator on bit arrays.
    It is not for use in any other context.
*/

/*! \fn QBitRef::QBitRef (QBitArray& a, int i)

    Constructs a reference to element \a i in the QBitArray \a a.
    This is what QBitArray::operator[] constructs its return value
    with.
*/

/*! \fn QBitRef::operator bool() const

    Returns the value referenced by the QBitRef.
*/

/*! \fn bool QBitRef::operator!() const

    \internal
*/

/*! \fn QBitRef& QBitRef::operator= (const QBitRef& v)

    Sets the value referenced by the QBitRef to that referenced by
    QBitRef \a v.
*/

/*! \fn QBitRef& QBitRef::operator= (bool v)
    \overload

    Sets the value referenced by the QBitRef to \a v.
*/


/*****************************************************************************
  QBitArray stream functions
 *****************************************************************************/

#ifndef QT_NO_DATASTREAM
/*!
    \relates QBitArray

    Writes bit array \a ba to stream \a out.

    \sa \link datastreamformat.html Format of the QDataStream operators \endlink
*/

QDataStream &operator<<(QDataStream &out, const QBitArray &ba)
{
    quint32 len = ba.size();
    out << len;
    if (len > 0)
        out.writeRawData(ba.d.constData() + 1, ba.d.size() - 1);
    return out;
}

/*!
    \relates QBitArray

    Reads a bit array into \a ba from stream \a in.

    \sa \link datastreamformat.html Format of the QDataStream operators \endlink
*/

QDataStream &operator>>(QDataStream &in, QBitArray &ba)
{
    ba.clear();
    quint32 len;
    in >> len;
    if (len == 0) {
	ba.clear();
	return in;
    }

    const quint32 Step = 8 * 1024 * 1024;
    quint32 totalBytes = (len + 7) / 8;
    quint32 allocated = 0;

    while (allocated < totalBytes) {
        int blockSize = qMin(Step, totalBytes - allocated);
        ba.d.resize(allocated + blockSize + 1);
        if (in.readRawData(ba.d.data() + 1 + allocated, blockSize) != blockSize) {
            ba.clear();
            in.setStatus(QDataStream::ReadPastEnd);
            return in;
        }
        allocated += blockSize;
    }

    int paddingMask = ~((0x1 << (len & 0x7)) - 1);
    if (paddingMask != ~0x0 && (ba.d.constData()[ba.d.size() - 1] & paddingMask)) {
        ba.clear();
        in.setStatus(QDataStream::ReadCorruptData);
        return in;
    }

    *ba.d.data() = ba.d.size() * 8 - len;
    return in;
}
#endif // QT_NO_DATASTREAM

/*!
    \fn DataPtr &QBitArray::data_ptr()
    \internal
*/

/*!
    \typedef QBitArray::DataPtr
    \internal
*/

QT_END_NAMESPACE
