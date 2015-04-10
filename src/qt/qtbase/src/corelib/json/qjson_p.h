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

#ifndef QJSON_P_H
#define QJSON_P_H

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

#include <qjsonobject.h>
#include <qjsonvalue.h>
#include <qjsondocument.h>
#include <qjsonarray.h>
#include <qatomic.h>
#include <qstring.h>
#include <qendian.h>
#include <qnumeric.h>

#include <limits.h>
#include <limits>

QT_BEGIN_NAMESPACE

/*
  This defines a binary data structure for Json data. The data structure is optimised for fast reading
  and minimum allocations. The whole data structure can be mmap'ed and used directly.

  In most cases the binary structure is not as space efficient as a utf8 encoded text representation, but
  much faster to access.

  The size requirements are:

  String:
    Latin1 data: 2 bytes header + string.length()
    Full Unicode: 4 bytes header + 2*(string.length())

  Values: 4 bytes + size of data (size can be 0 for some data)
    bool: 0 bytes
    double: 8 bytes (0 if integer with less than 27bits)
    string: see above
    array: size of array
    object: size of object
  Array: 12 bytes + 4*length + size of Value data
  Object: 12 bytes + 8*length + size of Key Strings + size of Value data

  For an example such as

    {                                           // object: 12 + 5*8                   = 52
         "firstName": "John",                   // key 12, value 8                    = 20
         "lastName" : "Smith",                  // key 12, value 8                    = 20
         "age"      : 25,                       // key 8, value 0                     = 8
         "address"  :                           // key 12, object below               = 140
         {                                      // object: 12 + 4*8
             "streetAddress": "21 2nd Street",  // key 16, value 16
             "city"         : "New York",       // key 8, value 12
             "state"        : "NY",             // key 8, value 4
             "postalCode"   : "10021"           // key 12, value 8
         },                                     // object total: 128
         "phoneNumber":                         // key: 16, value array below         = 172
         [                                      // array: 12 + 2*4 + values below: 156
             {                                  // object 12 + 2*8
               "type"  : "home",                // key 8, value 8
               "number": "212 555-1234"         // key 8, value 16
             },                                 // object total: 68
             {                                  // object 12 + 2*8
               "type"  : "fax",                 // key 8, value 8
               "number": "646 555-4567"         // key 8, value 16
             }                                  // object total: 68
         ]                                      // array total: 156
    }                                           // great total:                         412 bytes

    The uncompressed text file used roughly 500 bytes, so in this case we end up using about
    the same space as the text representation.

    Other measurements have shown a slightly bigger binary size than a compact text
    representation where all possible whitespace was stripped out.
*/
namespace QJsonPrivate {

class Array;
class Object;
class Value;
class Entry;

template<typename T>
class q_littleendian
{
public:
    T val;

    q_littleendian &operator =(T i) { val = qToLittleEndian(i); return *this; }
    operator T() const { return qFromLittleEndian(val); }

    bool operator ==(T i) { return qFromLittleEndian(val) == i; }
    bool operator !=(T i) { return qFromLittleEndian(val) != i; }
    bool operator ==(q_littleendian<T> i) { return val == i.val; }
    bool operator !=(q_littleendian<T> i) { return val != i.val; }
    bool operator <(T i) { return qFromLittleEndian(val) < i; }
    bool operator >(T i) { return qFromLittleEndian(val) > i; }
    bool operator <=(T i) { return qFromLittleEndian(val) <= i; }
    bool operator >=(T i) { return qFromLittleEndian(val) >= i; }
    q_littleendian &operator +=(T i) {
        val = qToLittleEndian(qFromLittleEndian(val) + i);
        return *this;
    }
};

typedef q_littleendian<short> qle_short;
typedef q_littleendian<unsigned short> qle_ushort;
typedef q_littleendian<int> qle_int;
typedef q_littleendian<unsigned int> qle_uint;

template<int pos, int width>
class qle_bitfield
{
public:
    uint val;

    enum {
        mask = ((1u << width) - 1) << pos
    };

    void operator =(uint t) {
        uint i = qFromLittleEndian(val);
        i &= ~mask;
        i |= t << pos;
        val = qToLittleEndian(i);
    }
    operator uint() const {
        uint t = qFromLittleEndian(val);
        t &= mask;
        t >>= pos;
        return t;
    }
    bool operator !() const {
        return !operator uint();
    }

    bool operator ==(uint t) { return uint(*this) == t; }
    bool operator !=(uint t) { return uint(*this) != t; }
    bool operator <(uint t) { return uint(*this) < t; }
    bool operator >(uint t) { return uint(*this) > t; }
    bool operator <=(uint t) { return uint(*this) <= t; }
    bool operator >=(uint t) { return uint(*this) >= t; }
    qle_bitfield &operator +=(uint i) {
        *this = (uint(*this) + i);
        return *this;
    }
    qle_bitfield &operator -=(uint i) {
        *this = (uint(*this) - i);
        return *this;
    }
};

template<int pos, int width>
class qle_signedbitfield
{
public:
    uint val;

    enum {
        mask = ((1u << width) - 1) << pos
    };

    void operator =(int t) {
        uint i = qFromLittleEndian(val);
        i &= ~mask;
        i |= t << pos;
        val = qToLittleEndian(i);
    }
    operator int() const {
        uint i = qFromLittleEndian(val);
        i <<= 32 - width - pos;
        int t = (int) i;
        t >>= pos;
        return t;
    }
    bool operator !() const {
        return !operator int();
    }

    bool operator ==(int t) { return int(*this) == t; }
    bool operator !=(int t) { return int(*this) != t; }
    bool operator <(int t) { return int(*this) < t; }
    bool operator >(int t) { return int(*this) > t; }
    bool operator <=(int t) { return int(*this) <= t; }
    bool operator >=(int t) { return int(*this) >= t; }
    qle_signedbitfield &operator +=(int i) {
        *this = (int(*this) + i);
        return *this;
    }
    qle_signedbitfield &operator -=(int i) {
        *this = (int(*this) - i);
        return *this;
    }
};

typedef qle_uint offset;

// round the size up to the next 4 byte boundary
inline int alignedSize(int size) { return (size + 3) & ~3; }

static inline bool useCompressed(const QString &s)
{
    if (s.length() >= 0x8000)
        return false;
    const ushort *uc = (const ushort *)s.constData();
    const ushort *e = uc + s.length();
    while (uc < e) {
        if (*uc > 0xff)
            return false;
        ++uc;
    }
    return true;
}

static inline int qStringSize(const QString &string, bool compress)
{
    int l = 2 + string.length();
    if (!compress)
        l *= 2;
    return alignedSize(l);
}

// returns INT_MAX if it can't compress it into 28 bits
static inline int compressedNumber(double d)
{
    // this relies on details of how ieee floats are represented
    const int exponent_off = 52;
    const quint64 fraction_mask = 0x000fffffffffffffull;
    const quint64 exponent_mask = 0x7ff0000000000000ull;

    quint64 val;
    memcpy (&val, &d, sizeof(double));
    int exp = (int)((val & exponent_mask) >> exponent_off) - 1023;
    if (exp < 0 || exp > 25)
        return INT_MAX;

    quint64 non_int = val & (fraction_mask >> exp);
    if (non_int)
        return INT_MAX;

    bool neg = (val >> 63);
    val &= fraction_mask;
    val |= ((quint64)1 << 52);
    int res = (int)(val >> (52 - exp));
    return neg ? -res : res;
}

class Latin1String;

class String
{
public:
    String(const char *data) { d = (Data *)data; }

    struct Data {
        qle_int length;
        qle_ushort utf16[1];
    };

    Data *d;

    inline String &operator=(const QString &str)
    {
        d->length = str.length();
#if Q_BYTE_ORDER == Q_BIG_ENDIAN
        const qle_ushort *uc = (const qle_ushort *)str.unicode();
        for (int i = 0; i < str.length(); ++i)
            d->utf16[i] = uc[i];
#else
        memcpy(d->utf16, str.unicode(), str.length()*sizeof(ushort));
#endif
        if (str.length() & 1)
            d->utf16[str.length()] = 0;
        return *this;
    }

    inline bool operator ==(const QString &str) const {
        int slen = str.length();
        int l = d->length;
        if (slen != l)
            return false;
        const ushort *s = (const ushort *)str.constData();
        const qle_ushort *a = d->utf16;
        const ushort *b = s;
        while (l-- && *a == *b)
            a++,b++;
        return (l == -1);
    }
    inline bool operator !=(const QString &str) const {
        return !operator ==(str);
    }
    inline bool operator >=(const QString &str) const {
        // ###
        return toString() >= str;
    }

    inline bool operator<(const Latin1String &str) const;
    inline bool operator>=(const Latin1String &str) const { return !operator <(str); }
    inline bool operator ==(const Latin1String &str) const;

    inline bool operator ==(const String &str) const {
        if (d->length != str.d->length)
            return false;
        return !memcmp(d->utf16, str.d->utf16, d->length*sizeof(ushort));
    }
    inline bool operator<(const String &other) const;
    inline bool operator >=(const String &other) const { return other < *this; }

    inline QString toString() const {
#if Q_BYTE_ORDER == Q_LITTLE_ENDIAN
        return QString((QChar *)d->utf16, d->length);
#else
        int l = d->length;
        QString str(l, Qt::Uninitialized);
        QChar *ch = str.data();
        for (int i = 0; i < l; ++i)
            ch[i] = QChar(d->utf16[i]);
        return str;
#endif
    }

};

class Latin1String
{
public:
    Latin1String(const char *data) { d = (Data *)data; }

    struct Data {
        qle_short length;
        char latin1[1];
    };
    Data *d;

    inline Latin1String &operator=(const QString &str)
    {
        d->length = str.length();
        uchar *l = (uchar *)d->latin1;
        const ushort *uc = (const ushort *)str.unicode();
        for (int i = 0; i < str.length(); ++i)
            *l++ = uc[i];
        while ((quintptr)l & 0x3)
            *l++ = 0;
        return *this;
    }

    inline bool operator ==(const QString &str) const {
        return QLatin1String(d->latin1, d->length) == str;
    }
    inline bool operator !=(const QString &str) const {
        return !operator ==(str);
    }
    inline bool operator >=(const QString &str) const {
        return QLatin1String(d->latin1, d->length) >= str;
    }

    inline bool operator ==(const Latin1String &str) const {
        return d->length == str.d->length && !strcmp(d->latin1, str.d->latin1);
    }
    inline bool operator >=(const Latin1String &str) const {
        int l = qMin(d->length, str.d->length);
        int val = strncmp(d->latin1, str.d->latin1, l);
        if (!val)
            val = d->length - str.d->length;
        return val >= 0;
    }

    inline bool operator ==(const String &str) const {
        return (str == *this);
    }
    inline bool operator >=(const String &str) const {
        return (str < *this);
    }

    inline QString toString() const {
        return QString::fromLatin1(d->latin1, d->length);
    }
};

inline bool String::operator ==(const Latin1String &str) const
{
    if ((int)d->length != (int)str.d->length)
        return false;
    const qle_ushort *uc = d->utf16;
    const qle_ushort *e = uc + d->length;
    const uchar *c = (uchar *)str.d->latin1;

    while (uc < e) {
        if (*uc != *c)
            return false;
        ++uc;
        ++c;
    }
    return true;
}

inline bool String::operator <(const String &other) const
{
    int alen = d->length;
    int blen = other.d->length;
    int l = qMin(alen, blen);
    qle_ushort *a = d->utf16;
    qle_ushort *b = other.d->utf16;

    while (l-- && *a == *b)
        a++,b++;
    if (l==-1)
        return (alen < blen);
    return (ushort)*a - (ushort)*b;
}

inline bool String::operator<(const Latin1String &str) const
{
    const uchar *c = (uchar *) str.d->latin1;
    if (!c || *c == 0)
        return false;

    const qle_ushort *uc = d->utf16;
    const qle_ushort *e = uc + qMin((int)d->length, (int)str.d->length);

    while (uc < e) {
        if (*uc != *c)
            break;
        ++uc;
        ++c;
    }
    return (uc == e ? (int)d->length < (int)str.d->length : (ushort)*uc < *c);

}

static inline void copyString(char *dest, const QString &str, bool compress)
{
    if (compress) {
        Latin1String string(dest);
        string = str;
    } else {
        String string(dest);
        string = str;
    }
}


/*
 Base is the base class for both Object and Array. Both classe work more or less the same way.
 The class starts with a header (defined by the struct below), then followed by data (the data for
 values in the Array case and Entry's (see below) for objects.

 After the data a table follows (tableOffset points to it) containing Value objects for Arrays, and
 offsets from the beginning of the object to Entry's in the case of Object.

 Entry's in the Object's table are lexicographically sorted by key in the table(). This allows the usage
 of a binary search over the keys in an Object.
 */
class Base
{
public:
    qle_uint size;
    union {
        uint _dummy;
        qle_bitfield<0, 1> is_object;
        qle_bitfield<1, 31> length;
    };
    offset tableOffset;
    // content follows here

    inline bool isObject() const { return is_object; }
    inline bool isArray() const { return !isObject(); }

    inline offset *table() const { return (offset *) (((char *) this) + tableOffset); }

    int reserveSpace(uint dataSize, int posInTable, uint numItems, bool replace);
    void removeItems(int pos, int numItems);
};

class Object : public Base
{
public:
    Entry *entryAt(int i) const {
        return reinterpret_cast<Entry *>(((char *)this) + table()[i]);
    }
    int indexOf(const QString &key, bool *exists);

    bool isValid() const;
};


class Array : public Base
{
public:
    inline Value at(int i) const;
    inline Value &operator [](int i);

    bool isValid() const;
};


class Value
{
public:
    enum {
        MaxSize = (1<<27) - 1
    };
    union {
        uint _dummy;
        qle_bitfield<0, 3> type;
        qle_bitfield<3, 1> latinOrIntValue;
        qle_bitfield<4, 1> latinKey;
        qle_bitfield<5, 27> value;
        qle_signedbitfield<5, 27> int_value;
    };

    inline char *data(const Base *b) const { return ((char *)b) + value; }
    int usedStorage(const Base *b) const;

    bool toBoolean() const;
    double toDouble(const Base *b) const;
    QString toString(const Base *b) const;
    String asString(const Base *b) const;
    Latin1String asLatin1String(const Base *b) const;
    Base *base(const Base *b) const;

    bool isValid(const Base *b) const;

    static int requiredStorage(QJsonValue &v, bool *compressed);
    static uint valueToStore(const QJsonValue &v, uint offset);
    static void copyData(const QJsonValue &v, char *dest, bool compressed);
};

inline Value Array::at(int i) const
{
    return *(Value *) (table() + i);
}

inline Value &Array::operator [](int i)
{
    return *(Value *) (table() + i);
}



class Entry {
public:
    Value value;
    // key
    // value data follows key

    int size() const {
        int s = sizeof(Entry);
        if (value.latinKey)
            s += sizeof(ushort) + qFromLittleEndian(*(ushort *) ((const char *)this + sizeof(Entry)));
        else
            s += sizeof(uint) + qFromLittleEndian(*(int *) ((const char *)this + sizeof(Entry)));
        return alignedSize(s);
    }

    int usedStorage(Base *b) const {
        return size() + value.usedStorage(b);
    }

    String shallowKey() const
    {
        Q_ASSERT(!value.latinKey);
        return String((const char *)this + sizeof(Entry));
    }
    Latin1String shallowLatin1Key() const
    {
        Q_ASSERT(value.latinKey);
        return Latin1String((const char *)this + sizeof(Entry));
    }
    QString key() const
    {
        if (value.latinKey) {
            return shallowLatin1Key().toString();
        }
        return shallowKey().toString();
    }

    bool operator ==(const QString &key) const;
    inline bool operator !=(const QString &key) const { return !operator ==(key); }
    inline bool operator >=(const QString &key) const;

    bool operator ==(const Entry &other) const;
    bool operator >=(const Entry &other) const;
};

inline bool Entry::operator >=(const QString &key) const
{
    if (value.latinKey)
        return (shallowLatin1Key() >= key);
    else
        return (shallowKey() >= key);
}

inline bool operator <(const QString &key, const Entry &e)
{ return e >= key; }


class Header {
public:
    qle_uint tag; // 'qbjs'
    qle_uint version; // 1
    Base *root() { return (Base *)(this + 1); }
};


inline bool Value::toBoolean() const
{
    Q_ASSERT(type == QJsonValue::Bool);
    return value != 0;
}

inline double Value::toDouble(const Base *b) const
{
    Q_ASSERT(type == QJsonValue::Double);
    if (latinOrIntValue)
        return int_value;

    quint64 i = qFromLittleEndian<quint64>((const uchar *)b + value);
    double d;
    memcpy(&d, &i, sizeof(double));
    return d;
}

inline String Value::asString(const Base *b) const
{
    Q_ASSERT(type == QJsonValue::String && !latinOrIntValue);
    return String(data(b));
}

inline Latin1String Value::asLatin1String(const Base *b) const
{
    Q_ASSERT(type == QJsonValue::String && latinOrIntValue);
    return Latin1String(data(b));
}

inline QString Value::toString(const Base *b) const
{
    if (latinOrIntValue)
        return asLatin1String(b).toString();
    else
        return asString(b).toString();
}

inline Base *Value::base(const Base *b) const
{
    Q_ASSERT(type == QJsonValue::Array || type == QJsonValue::Object);
    return reinterpret_cast<Base *>(data(b));
}

class Data {
public:
    enum Validation {
        Unchecked,
        Validated,
        Invalid
    };

    QAtomicInt ref;
    int alloc;
    union {
        char *rawData;
        Header *header;
    };
    uint compactionCounter : 31;
    uint ownsData : 1;

    inline Data(char *raw, int a)
        : alloc(a), rawData(raw), compactionCounter(0), ownsData(true)
    {
    }
    inline Data(int reserved, QJsonValue::Type valueType)
        : rawData(0), compactionCounter(0), ownsData(true)
    {
        Q_ASSERT(valueType == QJsonValue::Array || valueType == QJsonValue::Object);

        alloc = sizeof(Header) + sizeof(Base) + reserved + sizeof(offset);
        header = (Header *)malloc(alloc);
        Q_CHECK_PTR(header);
        header->tag = QJsonDocument::BinaryFormatTag;
        header->version = 1;
        Base *b = header->root();
        b->size = sizeof(Base);
        b->is_object = (valueType == QJsonValue::Object);
        b->tableOffset = sizeof(Base);
        b->length = 0;
    }
    inline ~Data()
    { if (ownsData) free(rawData); }

    uint offsetOf(const void *ptr) const { return (uint)(((char *)ptr - rawData)); }

    QJsonObject toObject(Object *o) const
    {
        return QJsonObject(const_cast<Data *>(this), o);
    }

    QJsonArray toArray(Array *a) const
    {
        return QJsonArray(const_cast<Data *>(this), a);
    }

    Data *clone(Base *b, int reserve = 0)
    {
        int size = sizeof(Header) + b->size;
        if (b == header->root() && ref.load() == 1 && alloc >= size + reserve)
            return this;

        if (reserve) {
            if (reserve < 128)
                reserve = 128;
            size = qMax(size + reserve, size *2);
        }
        char *raw = (char *)malloc(size);
        Q_CHECK_PTR(raw);
        memcpy(raw + sizeof(Header), b, b->size);
        Header *h = (Header *)raw;
        h->tag = QJsonDocument::BinaryFormatTag;
        h->version = 1;
        Data *d = new Data(raw, size);
        d->compactionCounter = (b == header->root()) ? compactionCounter : 0;
        return d;
    }

    void compact();
    bool valid() const;

private:
    Q_DISABLE_COPY(Data)
};

}

QT_END_NAMESPACE

#endif // QJSON_P_H
