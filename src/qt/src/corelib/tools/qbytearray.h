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

#ifndef QBYTEARRAY_H
#define QBYTEARRAY_H

#include <QtCore/qatomic.h>
#include <QtCore/qnamespace.h>

#include <string.h>
#include <stdarg.h>

#ifdef truncate
#error qbytearray.h must be included before any header file that defines truncate
#endif

#if defined(Q_CC_GNU) && (__GNUC__ == 4 && __GNUC_MINOR__ == 0)
//There is a bug in GCC 4.0 that tries to instantiate template of annonymous enum
#  ifdef QT_USE_FAST_OPERATOR_PLUS
#    undef QT_USE_FAST_OPERATOR_PLUS
#  endif
#  ifdef QT_USE_QSTRINGBUILDER
#    undef QT_USE_QSTRINGBUILDER
#  endif

#endif


QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Core)

/*****************************************************************************
  Safe and portable C string functions; extensions to standard string.h
 *****************************************************************************/

Q_CORE_EXPORT char *qstrdup(const char *);

inline uint qstrlen(const char *str)
{ return str ? uint(strlen(str)) : 0; }

inline uint qstrnlen(const char *str, uint maxlen)
{
    uint length = 0;
    if (str) {
        while (length < maxlen && *str++)
            length++;
    }
    return length;
}

Q_CORE_EXPORT char *qstrcpy(char *dst, const char *src);
Q_CORE_EXPORT char *qstrncpy(char *dst, const char *src, uint len);

Q_CORE_EXPORT int qstrcmp(const char *str1, const char *str2);
Q_CORE_EXPORT int qstrcmp(const QByteArray &str1, const QByteArray &str2);
Q_CORE_EXPORT int qstrcmp(const QByteArray &str1, const char *str2);
static inline int qstrcmp(const char *str1, const QByteArray &str2)
{ return -qstrcmp(str2, str1); }

inline int qstrncmp(const char *str1, const char *str2, uint len)
{
    return (str1 && str2) ? strncmp(str1, str2, len)
        : (str1 ? 1 : (str2 ? -1 : 0));
}
Q_CORE_EXPORT int qstricmp(const char *, const char *);
Q_CORE_EXPORT int qstrnicmp(const char *, const char *, uint len);

// implemented in qvsnprintf.cpp
Q_CORE_EXPORT int qvsnprintf(char *str, size_t n, const char *fmt, va_list ap);
Q_CORE_EXPORT int qsnprintf(char *str, size_t n, const char *fmt, ...);

#ifdef QT3_SUPPORT
inline QT3_SUPPORT void *qmemmove(void *dst, const void *src, uint len)
{ return memmove(dst, src, len); }
inline QT3_SUPPORT uint cstrlen(const char *str)
{ return uint(strlen(str)); }
inline QT3_SUPPORT char *cstrcpy(char *dst, const char *src)
{ return qstrcpy(dst,src); }
inline QT3_SUPPORT int cstrcmp(const char *str1, const char *str2)
{ return strcmp(str1,str2); }
inline QT3_SUPPORT int cstrncmp(const char *str1, const char *str2, uint len)
{ return strncmp(str1,str2,len); }
#endif

// qChecksum: Internet checksum

Q_CORE_EXPORT quint16 qChecksum(const char *s, uint len);

class QByteRef;
class QString;
class QDataStream;
template <typename T> class QList;

class Q_CORE_EXPORT QByteArray
{
private:
    struct Data {
        QBasicAtomicInt ref;
        int alloc, size;
        // ### Qt 5.0: We need to add the missing capacity bit
        // (like other tool classes have), to maintain the
        // reserved memory on resize.
        char *data;
        char array[1];
    };

public:
    inline QByteArray();
    QByteArray(const char *);
    QByteArray(const char *, int size);
    QByteArray(int size, char c);
    QByteArray(int size, Qt::Initialization);
    inline QByteArray(const QByteArray &);
    inline ~QByteArray();

    QByteArray &operator=(const QByteArray &);
    QByteArray &operator=(const char *str);
#ifdef Q_COMPILER_RVALUE_REFS
    inline QByteArray &operator=(QByteArray &&other)
    { qSwap(d, other.d); return *this; }
#endif

    inline void swap(QByteArray &other) { qSwap(d, other.d); }

    inline int size() const;
    bool isEmpty() const;
    void resize(int size);

    QByteArray &fill(char c, int size = -1);

    int capacity() const;
    void reserve(int size);
    void squeeze();

#ifndef QT_NO_CAST_FROM_BYTEARRAY
    operator const char *() const;
    operator const void *() const;
#endif
    char *data();
    const char *data() const;
    inline const char *constData() const;
    inline void detach();
    bool isDetached() const;
    inline bool isSharedWith(const QByteArray &other) const { return d == other.d; }
    void clear();

#ifdef Q_COMPILER_MANGLES_RETURN_TYPE
    const char at(int i) const;
    const char operator[](int i) const;
    const char operator[](uint i) const;
#else
    char at(int i) const;
    char operator[](int i) const;
    char operator[](uint i) const;
#endif
    QByteRef operator[](int i);
    QByteRef operator[](uint i);

    int indexOf(char c, int from = 0) const;
    int indexOf(const char *c, int from = 0) const;
    int indexOf(const QByteArray &a, int from = 0) const;
    int lastIndexOf(char c, int from = -1) const;
    int lastIndexOf(const char *c, int from = -1) const;
    int lastIndexOf(const QByteArray &a, int from = -1) const;

    QBool contains(char c) const;
    QBool contains(const char *a) const;
    QBool contains(const QByteArray &a) const;
    int count(char c) const;
    int count(const char *a) const;
    int count(const QByteArray &a) const;

    QByteArray left(int len) const;
    QByteArray right(int len) const;
    QByteArray mid(int index, int len = -1) const;

    bool startsWith(const QByteArray &a) const;
    bool startsWith(char c) const;
    bool startsWith(const char *c) const;

    bool endsWith(const QByteArray &a) const;
    bool endsWith(char c) const;
    bool endsWith(const char *c) const;

    void truncate(int pos);
    void chop(int n);

    QByteArray toLower() const;
    QByteArray toUpper() const;

    QByteArray trimmed() const;
    QByteArray simplified() const;
    QByteArray leftJustified(int width, char fill = ' ', bool truncate = false) const;
    QByteArray rightJustified(int width, char fill = ' ', bool truncate = false) const;

#ifdef QT3_SUPPORT
    inline QT3_SUPPORT QByteArray leftJustify(uint width, char aFill = ' ', bool aTruncate = false) const
    { return leftJustified(int(width), aFill, aTruncate); }
    inline QT3_SUPPORT QByteArray rightJustify(uint width, char aFill = ' ', bool aTruncate = false) const
    { return rightJustified(int(width), aFill, aTruncate); }
#endif

    QByteArray &prepend(char c);
    QByteArray &prepend(const char *s);
    QByteArray &prepend(const char *s, int len);
    QByteArray &prepend(const QByteArray &a);
    QByteArray &append(char c);
    QByteArray &append(const char *s);
    QByteArray &append(const char *s, int len);
    QByteArray &append(const QByteArray &a);
    QByteArray &insert(int i, char c);
    QByteArray &insert(int i, const char *s);
    QByteArray &insert(int i, const char *s, int len);
    QByteArray &insert(int i, const QByteArray &a);
    QByteArray &remove(int index, int len);
    QByteArray &replace(int index, int len, const char *s);
    QByteArray &replace(int index, int len, const char *s, int alen);
    QByteArray &replace(int index, int len, const QByteArray &s);
    QByteArray &replace(char before, const char *after);
    QByteArray &replace(char before, const QByteArray &after);
    QByteArray &replace(const char *before, const char *after);
    QByteArray &replace(const char *before, int bsize, const char *after, int asize);
    QByteArray &replace(const QByteArray &before, const QByteArray &after);
    QByteArray &replace(const QByteArray &before, const char *after);
    QByteArray &replace(const char *before, const QByteArray &after);
    QByteArray &replace(char before, char after);
    QByteArray &operator+=(char c);
    QByteArray &operator+=(const char *s);
    QByteArray &operator+=(const QByteArray &a);

    QList<QByteArray> split(char sep) const;

    QByteArray repeated(int times) const;

#ifndef QT_NO_CAST_TO_ASCII
    QT_ASCII_CAST_WARN QByteArray &append(const QString &s);
    QT_ASCII_CAST_WARN QByteArray &insert(int i, const QString &s);
    QT_ASCII_CAST_WARN QByteArray &replace(const QString &before, const char *after);
    QT_ASCII_CAST_WARN QByteArray &replace(char c, const QString &after);
    QT_ASCII_CAST_WARN QByteArray &replace(const QString &before, const QByteArray &after);

    QT_ASCII_CAST_WARN QByteArray &operator+=(const QString &s);
    QT_ASCII_CAST_WARN int indexOf(const QString &s, int from = 0) const;
    QT_ASCII_CAST_WARN int lastIndexOf(const QString &s, int from = -1) const;
#endif
#ifndef QT_NO_CAST_FROM_ASCII
    inline QT_ASCII_CAST_WARN bool operator==(const QString &s2) const;
    inline QT_ASCII_CAST_WARN bool operator!=(const QString &s2) const;
    inline QT_ASCII_CAST_WARN bool operator<(const QString &s2) const;
    inline QT_ASCII_CAST_WARN bool operator>(const QString &s2) const;
    inline QT_ASCII_CAST_WARN bool operator<=(const QString &s2) const;
    inline QT_ASCII_CAST_WARN bool operator>=(const QString &s2) const;
#endif

    short toShort(bool *ok = 0, int base = 10) const;
    ushort toUShort(bool *ok = 0, int base = 10) const;
    int toInt(bool *ok = 0, int base = 10) const;
    uint toUInt(bool *ok = 0, int base = 10) const;
    long toLong(bool *ok = 0, int base = 10) const;
    ulong toULong(bool *ok = 0, int base = 10) const;
    qlonglong toLongLong(bool *ok = 0, int base = 10) const;
    qulonglong toULongLong(bool *ok = 0, int base = 10) const;
    float toFloat(bool *ok = 0) const;
    double toDouble(bool *ok = 0) const;
    QByteArray toBase64() const;
    QByteArray toHex() const;
    QByteArray toPercentEncoding(const QByteArray &exclude = QByteArray(),
                                 const QByteArray &include = QByteArray(),
                                 char percent = '%') const;

    QByteArray &setNum(short, int base = 10);
    QByteArray &setNum(ushort, int base = 10);
    QByteArray &setNum(int, int base = 10);
    QByteArray &setNum(uint, int base = 10);
    QByteArray &setNum(qlonglong, int base = 10);
    QByteArray &setNum(qulonglong, int base = 10);
    QByteArray &setNum(float, char f = 'g', int prec = 6);
    QByteArray &setNum(double, char f = 'g', int prec = 6);
    QByteArray &setRawData(const char *a, uint n); // ### Qt 5: use an int

    static QByteArray number(int, int base = 10);
    static QByteArray number(uint, int base = 10);
    static QByteArray number(qlonglong, int base = 10);
    static QByteArray number(qulonglong, int base = 10);
    static QByteArray number(double, char f = 'g', int prec = 6);
    static QByteArray fromRawData(const char *, int size);
    static QByteArray fromBase64(const QByteArray &base64);
    static QByteArray fromHex(const QByteArray &hexEncoded);
    static QByteArray fromPercentEncoding(const QByteArray &pctEncoded, char percent = '%');


    typedef char *iterator;
    typedef const char *const_iterator;
    typedef iterator Iterator;
    typedef const_iterator ConstIterator;
    iterator begin();
    const_iterator begin() const;
    const_iterator constBegin() const;
    iterator end();
    const_iterator end() const;
    const_iterator constEnd() const;

    // stl compatibility
    typedef const char & const_reference;
    typedef char & reference;
    typedef char value_type;
    void push_back(char c);
    void push_back(const char *c);
    void push_back(const QByteArray &a);
    void push_front(char c);
    void push_front(const char *c);
    void push_front(const QByteArray &a);

    inline int count() const { return d->size; }
    int length() const { return d->size; }
    bool isNull() const;

    // compatibility
#ifdef QT3_SUPPORT
    QT3_SUPPORT_CONSTRUCTOR QByteArray(int size);
    inline QT3_SUPPORT QByteArray& duplicate(const QByteArray& a) { *this = a; return *this; }
    inline QT3_SUPPORT QByteArray& duplicate(const char *a, uint n)
    { *this = QByteArray(a, n); return *this; }
    inline QT3_SUPPORT void resetRawData(const char *, uint) { clear(); }
    inline QT3_SUPPORT QByteArray lower() const { return toLower(); }
    inline QT3_SUPPORT QByteArray upper() const { return toUpper(); }
    inline QT3_SUPPORT QByteArray stripWhiteSpace() const { return trimmed(); }
    inline QT3_SUPPORT QByteArray simplifyWhiteSpace() const { return simplified(); }
    inline QT3_SUPPORT int find(char c, int from = 0) const { return indexOf(c, from); }
    inline QT3_SUPPORT int find(const char *c, int from = 0) const { return indexOf(c, from); }
    inline QT3_SUPPORT int find(const QByteArray &ba, int from = 0) const { return indexOf(ba, from); }
    inline QT3_SUPPORT int findRev(char c, int from = -1) const { return lastIndexOf(c, from); }
    inline QT3_SUPPORT int findRev(const char *c, int from = -1) const { return lastIndexOf(c, from); }
    inline QT3_SUPPORT int findRev(const QByteArray &ba, int from = -1) const { return lastIndexOf(ba, from); }
#ifndef QT_NO_CAST_TO_ASCII
    QT3_SUPPORT int find(const QString &s, int from = 0) const;
    QT3_SUPPORT int findRev(const QString &s, int from = -1) const;
#endif
#endif

private:
    operator QNoImplicitBoolCast() const;
    static Data shared_null;
    static Data shared_empty;
    Data *d;
    QByteArray(Data *dd, int /*dummy*/, int /*dummy*/) : d(dd) {}
    void realloc(int alloc);
    void expand(int i);
    QByteArray nulTerminated() const;

    friend class QByteRef;
    friend class QString;
    friend Q_CORE_EXPORT QByteArray qUncompress(const uchar *data, int nbytes);
public:
    typedef Data * DataPtr;
    inline DataPtr &data_ptr() { return d; }
};

inline QByteArray::QByteArray(): d(&shared_null) { d->ref.ref(); }
inline QByteArray::~QByteArray() { if (!d->ref.deref()) qFree(d); }
inline int QByteArray::size() const
{ return d->size; }

#ifdef Q_COMPILER_MANGLES_RETURN_TYPE
inline const char QByteArray::at(int i) const
{ Q_ASSERT(i >= 0 && i < size()); return d->data[i]; }
inline const char QByteArray::operator[](int i) const
{ Q_ASSERT(i >= 0 && i < size()); return d->data[i]; }
inline const char QByteArray::operator[](uint i) const
{ Q_ASSERT(i < uint(size())); return d->data[i]; }
#else
inline char QByteArray::at(int i) const
{ Q_ASSERT(uint(i) < uint(size())); return d->data[i]; }
inline char QByteArray::operator[](int i) const
{ Q_ASSERT(uint(i) < uint(size())); return d->data[i]; }
inline char QByteArray::operator[](uint i) const
{ Q_ASSERT(i < uint(size())); return d->data[i]; }
#endif

inline bool QByteArray::isEmpty() const
{ return d->size == 0; }
#ifndef QT_NO_CAST_FROM_BYTEARRAY
inline QByteArray::operator const char *() const
{ return d->data; }
inline QByteArray::operator const void *() const
{ return d->data; }
#endif
inline char *QByteArray::data()
{ detach(); return d->data; }
inline const char *QByteArray::data() const
{ return d->data; }
inline const char *QByteArray::constData() const
{ return d->data; }
inline void QByteArray::detach()
{ if (d->ref != 1 || d->data != d->array) realloc(d->size); }
inline bool QByteArray::isDetached() const
{ return d->ref == 1; }
inline QByteArray::QByteArray(const QByteArray &a) : d(a.d)
{ d->ref.ref(); }
#ifdef QT3_SUPPORT
inline QByteArray::QByteArray(int aSize) : d(&shared_null)
{ d->ref.ref(); if (aSize > 0) fill('\0', aSize); }
#endif

inline int QByteArray::capacity() const
{ return d->alloc; }

inline void QByteArray::reserve(int asize)
{ if (d->ref != 1 || asize > d->alloc) realloc(asize); }

inline void QByteArray::squeeze()
{ if (d->size < d->alloc) realloc(d->size); }

class Q_CORE_EXPORT QByteRef {
    QByteArray &a;
    int i;
    inline QByteRef(QByteArray &array, int idx)
        : a(array),i(idx) {}
    friend class QByteArray;
public:
#ifdef Q_COMPILER_MANGLES_RETURN_TYPE
    inline operator const char() const
        { return i < a.d->size ? a.d->data[i] : char(0); }
#else
    inline operator char() const
        { return i < a.d->size ? a.d->data[i] : char(0); }
#endif
    inline QByteRef &operator=(char c)
        { if (i >= a.d->size) a.expand(i); else a.detach();
          a.d->data[i] = c;  return *this; }
    inline QByteRef &operator=(const QByteRef &c)
        { if (i >= a.d->size) a.expand(i); else a.detach();
          a.d->data[i] = c.a.d->data[c.i];  return *this; }
    inline bool operator==(char c) const
    { return a.d->data[i] == c; }
    inline bool operator!=(char c) const
    { return a.d->data[i] != c; }
    inline bool operator>(char c) const
    { return a.d->data[i] > c; }
    inline bool operator>=(char c) const
    { return a.d->data[i] >= c; }
    inline bool operator<(char c) const
    { return a.d->data[i] < c; }
    inline bool operator<=(char c) const
    { return a.d->data[i] <= c; }
};

inline QByteRef QByteArray::operator[](int i)
{ Q_ASSERT(i >= 0); return QByteRef(*this, i); }
inline QByteRef QByteArray::operator[](uint i)
{ return QByteRef(*this, i); }
inline QByteArray::iterator QByteArray::begin()
{ detach(); return d->data; }
inline QByteArray::const_iterator QByteArray::begin() const
{ return d->data; }
inline QByteArray::const_iterator QByteArray::constBegin() const
{ return d->data; }
inline QByteArray::iterator QByteArray::end()
{ detach(); return d->data + d->size; }
inline QByteArray::const_iterator QByteArray::end() const
{ return d->data + d->size; }
inline QByteArray::const_iterator QByteArray::constEnd() const
{ return d->data + d->size; }
inline QByteArray &QByteArray::operator+=(char c)
{ return append(c); }
inline QByteArray &QByteArray::operator+=(const char *s)
{ return append(s); }
inline QByteArray &QByteArray::operator+=(const QByteArray &a)
{ return append(a); }
inline void QByteArray::push_back(char c)
{ append(c); }
inline void QByteArray::push_back(const char *c)
{ append(c); }
inline void QByteArray::push_back(const QByteArray &a)
{ append(a); }
inline void QByteArray::push_front(char c)
{ prepend(c); }
inline void QByteArray::push_front(const char *c)
{ prepend(c); }
inline void QByteArray::push_front(const QByteArray &a)
{ prepend(a); }
inline QBool QByteArray::contains(const QByteArray &a) const
{ return QBool(indexOf(a) != -1); }
inline QBool QByteArray::contains(char c) const
{ return QBool(indexOf(c) != -1); }
inline bool operator==(const QByteArray &a1, const QByteArray &a2)
{ return (a1.size() == a2.size()) && (memcmp(a1.constData(), a2.constData(), a1.size())==0); }
inline bool operator==(const QByteArray &a1, const char *a2)
{ return a2 ? qstrcmp(a1,a2) == 0 : a1.isEmpty(); }
inline bool operator==(const char *a1, const QByteArray &a2)
{ return a1 ? qstrcmp(a1,a2) == 0 : a2.isEmpty(); }
inline bool operator!=(const QByteArray &a1, const QByteArray &a2)
{ return !(a1==a2); }
inline bool operator!=(const QByteArray &a1, const char *a2)
{ return a2 ? qstrcmp(a1,a2) != 0 : !a1.isEmpty(); }
inline bool operator!=(const char *a1, const QByteArray &a2)
{ return a1 ? qstrcmp(a1,a2) != 0 : !a2.isEmpty(); }
inline bool operator<(const QByteArray &a1, const QByteArray &a2)
{ return qstrcmp(a1, a2) < 0; }
 inline bool operator<(const QByteArray &a1, const char *a2)
{ return qstrcmp(a1, a2) < 0; }
inline bool operator<(const char *a1, const QByteArray &a2)
{ return qstrcmp(a1, a2) < 0; }
inline bool operator<=(const QByteArray &a1, const QByteArray &a2)
{ return qstrcmp(a1, a2) <= 0; }
inline bool operator<=(const QByteArray &a1, const char *a2)
{ return qstrcmp(a1, a2) <= 0; }
inline bool operator<=(const char *a1, const QByteArray &a2)
{ return qstrcmp(a1, a2) <= 0; }
inline bool operator>(const QByteArray &a1, const QByteArray &a2)
{ return qstrcmp(a1, a2) > 0; }
inline bool operator>(const QByteArray &a1, const char *a2)
{ return qstrcmp(a1, a2) > 0; }
inline bool operator>(const char *a1, const QByteArray &a2)
{ return qstrcmp(a1, a2) > 0; }
inline bool operator>=(const QByteArray &a1, const QByteArray &a2)
{ return qstrcmp(a1, a2) >= 0; }
inline bool operator>=(const QByteArray &a1, const char *a2)
{ return qstrcmp(a1, a2) >= 0; }
inline bool operator>=(const char *a1, const QByteArray &a2)
{ return qstrcmp(a1, a2) >= 0; }
#if !defined(QT_USE_QSTRINGBUILDER)
inline const QByteArray operator+(const QByteArray &a1, const QByteArray &a2)
{ return QByteArray(a1) += a2; }
inline const QByteArray operator+(const QByteArray &a1, const char *a2)
{ return QByteArray(a1) += a2; }
inline const QByteArray operator+(const QByteArray &a1, char a2)
{ return QByteArray(a1) += a2; }
inline const QByteArray operator+(const char *a1, const QByteArray &a2)
{ return QByteArray(a1) += a2; }
inline const QByteArray operator+(char a1, const QByteArray &a2)
{ return QByteArray(&a1, 1) += a2; }
#endif // QT_USE_QSTRINGBUILDER
inline QBool QByteArray::contains(const char *c) const
{ return QBool(indexOf(c) != -1); }
inline QByteArray &QByteArray::replace(char before, const char *c)
{ return replace(&before, 1, c, qstrlen(c)); }
inline QByteArray &QByteArray::replace(const QByteArray &before, const char *c)
{ return replace(before.constData(), before.size(), c, qstrlen(c)); }
inline QByteArray &QByteArray::replace(const char *before, const char *after)
{ return replace(before, qstrlen(before), after, qstrlen(after)); }

inline QByteArray &QByteArray::setNum(short n, int base)
{ return setNum(qlonglong(n), base); }
inline QByteArray &QByteArray::setNum(ushort n, int base)
{ return setNum(qulonglong(n), base); }
inline QByteArray &QByteArray::setNum(int n, int base)
{ return setNum(qlonglong(n), base); }
inline QByteArray &QByteArray::setNum(uint n, int base)
{ return setNum(qulonglong(n), base); }
inline QByteArray &QByteArray::setNum(float n, char f, int prec)
{ return setNum(double(n),f,prec); }


#if !defined(QT_NO_DATASTREAM) || (defined(QT_BOOTSTRAPPED) && !defined(QT_BUILD_QMAKE))
Q_CORE_EXPORT QDataStream &operator<<(QDataStream &, const QByteArray &);
Q_CORE_EXPORT QDataStream &operator>>(QDataStream &, QByteArray &);
#endif

#ifndef QT_NO_COMPRESS
Q_CORE_EXPORT QByteArray qCompress(const uchar* data, int nbytes, int compressionLevel = -1);
Q_CORE_EXPORT QByteArray qUncompress(const uchar* data, int nbytes);
inline QByteArray qCompress(const QByteArray& data, int compressionLevel = -1)
{ return qCompress(reinterpret_cast<const uchar *>(data.constData()), data.size(), compressionLevel); }
inline QByteArray qUncompress(const QByteArray& data)
{ return qUncompress(reinterpret_cast<const uchar*>(data.constData()), data.size()); }
#endif

Q_DECLARE_TYPEINFO(QByteArray, Q_MOVABLE_TYPE);
Q_DECLARE_SHARED(QByteArray)

QT_END_NAMESPACE

QT_END_HEADER

#ifdef QT_USE_QSTRINGBUILDER
#include <QtCore/qstring.h>
#endif

#endif // QBYTEARRAY_H
