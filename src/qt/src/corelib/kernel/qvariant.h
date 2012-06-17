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

#ifndef QVARIANT_H
#define QVARIANT_H

#include <QtCore/qatomic.h>
#include <QtCore/qbytearray.h>
#include <QtCore/qlist.h>
#include <QtCore/qmetatype.h>
#include <QtCore/qmap.h>
#include <QtCore/qhash.h>
#include <QtCore/qstring.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Core)

class QBitArray;
class QDataStream;
class QDate;
class QDateTime;
class QEasingCurve;
class QLine;
class QLineF;
class QLocale;
class QMatrix;
class QTransform;
class QStringList;
class QTime;
class QPoint;
class QPointF;
class QSize;
class QSizeF;
class QRect;
class QRectF;
#ifndef QT_NO_REGEXP
class QRegExp;
#endif
class QTextFormat;
class QTextLength;
class QUrl;
class QVariant;
class QVariantComparisonHelper;

template <typename T>
inline QVariant qVariantFromValue(const T &);

template<typename T>
inline T qvariant_cast(const QVariant &);

class Q_CORE_EXPORT QVariant
{
 public:
    enum Type {
        Invalid = 0,

        Bool = 1,
        Int = 2,
        UInt = 3,
        LongLong = 4,
        ULongLong = 5,
        Double = 6,
        Char = 7,
        Map = 8,
        List = 9,
        String = 10,
        StringList = 11,
        ByteArray = 12,
        BitArray = 13,
        Date = 14,
        Time = 15,
        DateTime = 16,
        Url = 17,
        Locale = 18,
        Rect = 19,
        RectF = 20,
        Size = 21,
        SizeF = 22,
        Line = 23,
        LineF = 24,
        Point = 25,
        PointF = 26,
        RegExp = 27,
        Hash = 28,
        EasingCurve = 29,
        LastCoreType = EasingCurve,

        // value 62 is internally reserved
#ifdef QT3_SUPPORT
        ColorGroup = 63,
#endif
        Font = 64,
        Pixmap = 65,
        Brush = 66,
        Color = 67,
        Palette = 68,
        Icon = 69,
        Image = 70,
        Polygon = 71,
        Region = 72,
        Bitmap = 73,
        Cursor = 74,
        SizePolicy = 75,
        KeySequence = 76,
        Pen = 77,
        TextLength = 78,
        TextFormat = 79,
        Matrix = 80,
        Transform = 81,
        Matrix4x4 = 82,
        Vector2D = 83,
        Vector3D = 84,
        Vector4D = 85,
        Quaternion = 86,
        LastGuiType = Quaternion,

        UserType = 127,
#ifdef QT3_SUPPORT
        IconSet = Icon,
        CString = ByteArray,
        PointArray = Polygon,
#endif
        LastType = 0xffffffff // need this so that gcc >= 3.4 allocates 32 bits for Type
    };

    inline QVariant();
    ~QVariant();
    QVariant(Type type);
    QVariant(int typeOrUserType, const void *copy);
    QVariant(int typeOrUserType, const void *copy, uint flags);
    QVariant(const QVariant &other);

#ifndef QT_NO_DATASTREAM
    QVariant(QDataStream &s);
#endif

    QVariant(int i);
    QVariant(uint ui);
    QVariant(qlonglong ll);
    QVariant(qulonglong ull);
    QVariant(bool b);
    QVariant(double d);
    QVariant(float f) { d.is_null = false; d.type = QMetaType::Float; d.data.f = f; }
#ifndef QT_NO_CAST_FROM_ASCII
    QT_ASCII_CAST_WARN_CONSTRUCTOR QVariant(const char *str);
#endif

    QVariant(const QByteArray &bytearray);
    QVariant(const QBitArray &bitarray);
    QVariant(const QString &string);
    QVariant(const QLatin1String &string);
    QVariant(const QStringList &stringlist);
    QVariant(const QChar &qchar);
    QVariant(const QDate &date);
    QVariant(const QTime &time);
    QVariant(const QDateTime &datetime);
    QVariant(const QList<QVariant> &list);
    QVariant(const QMap<QString,QVariant> &map);
    QVariant(const QHash<QString,QVariant> &hash);
#ifndef QT_NO_GEOM_VARIANT
    QVariant(const QSize &size);
    QVariant(const QSizeF &size);
    QVariant(const QPoint &pt);
    QVariant(const QPointF &pt);
    QVariant(const QLine &line);
    QVariant(const QLineF &line);
    QVariant(const QRect &rect);
    QVariant(const QRectF &rect);
#endif
    QVariant(const QUrl &url);
    QVariant(const QLocale &locale);
#ifndef QT_NO_REGEXP
    QVariant(const QRegExp &regExp);
#endif
#ifndef QT_BOOTSTRAPPED
    QVariant(const QEasingCurve &easing);
#endif
    QVariant(Qt::GlobalColor color);

    QVariant& operator=(const QVariant &other);
#ifdef Q_COMPILER_RVALUE_REFS
    inline QVariant &operator=(QVariant &&other)
    { qSwap(d, other.d); return *this; }
#endif

    inline void swap(QVariant &other) { qSwap(d, other.d); }

    Type type() const;
    int userType() const;
    const char *typeName() const;

    bool canConvert(Type t) const;
    bool convert(Type t);

#ifdef QT3_SUPPORT
    inline QT3_SUPPORT bool canCast(Type t) const
    { return canConvert(t); }
    inline QT3_SUPPORT bool cast(Type t)
    { return convert(t); }
#endif

    inline bool isValid() const;
    bool isNull() const;

    void clear();

    void detach();
    inline bool isDetached() const;

    int toInt(bool *ok = 0) const;
    uint toUInt(bool *ok = 0) const;
    qlonglong toLongLong(bool *ok = 0) const;
    qulonglong toULongLong(bool *ok = 0) const;
    bool toBool() const;
    double toDouble(bool *ok = 0) const;
    float toFloat(bool *ok = 0) const;
    qreal toReal(bool *ok = 0) const;
    QByteArray toByteArray() const;
    QBitArray toBitArray() const;
    QString toString() const;
    QStringList toStringList() const;
    QChar toChar() const;
    QDate toDate() const;
    QTime toTime() const;
    QDateTime toDateTime() const;
    QList<QVariant> toList() const;
    QMap<QString, QVariant> toMap() const;
    QHash<QString, QVariant> toHash() const;

#ifndef QT_NO_GEOM_VARIANT
    QPoint toPoint() const;
    QPointF toPointF() const;
    QRect toRect() const;
    QSize toSize() const;
    QSizeF toSizeF() const;
    QLine toLine() const;
    QLineF toLineF() const;
    QRectF toRectF() const;
#endif
    QUrl toUrl() const;
    QLocale toLocale() const;
#ifndef QT_NO_REGEXP
    QRegExp toRegExp() const;
#endif
#ifndef QT_BOOTSTRAPPED
    QEasingCurve toEasingCurve() const;
#endif

#ifdef QT3_SUPPORT
    inline QT3_SUPPORT int &asInt();
    inline QT3_SUPPORT uint &asUInt();
    inline QT3_SUPPORT qlonglong &asLongLong();
    inline QT3_SUPPORT qulonglong &asULongLong();
    inline QT3_SUPPORT bool &asBool();
    inline QT3_SUPPORT double &asDouble();
    inline QT3_SUPPORT QByteArray &asByteArray();
    inline QT3_SUPPORT QBitArray &asBitArray();
    inline QT3_SUPPORT QString &asString();
    inline QT3_SUPPORT QStringList &asStringList();
    inline QT3_SUPPORT QDate &asDate();
    inline QT3_SUPPORT QTime &asTime();
    inline QT3_SUPPORT QDateTime &asDateTime();
    inline QT3_SUPPORT QList<QVariant> &asList();
    inline QT3_SUPPORT QMap<QString,QVariant> &asMap();
    inline QT3_SUPPORT QPoint &asPoint();
    inline QT3_SUPPORT QRect &asRect();
    inline QT3_SUPPORT QSize &asSize();
#endif //QT3_SUPPORT

#ifndef QT_NO_DATASTREAM
    void load(QDataStream &ds);
    void save(QDataStream &ds) const;
#endif
    static const char *typeToName(Type type);
    static Type nameToType(const char *name);

#ifdef QT3_SUPPORT
    inline QT3_SUPPORT_CONSTRUCTOR QVariant(bool val, int) { create(Bool, &val); }
    inline QT3_SUPPORT const QByteArray toCString() const { return toByteArray(); }
    inline QT3_SUPPORT QByteArray &asCString() { return *reinterpret_cast<QByteArray *>(castOrDetach(ByteArray)); }
#endif

    void *data();
    const void *constData() const;
    inline const void *data() const { return constData(); }

    template<typename T>
    inline void setValue(const T &value);

    template<typename T>
    inline T value() const
    { return qvariant_cast<T>(*this); }

    template<typename T>
    static inline QVariant fromValue(const T &value)
    { return qVariantFromValue(value); }

    template<typename T>
    bool canConvert() const
    { return canConvert(Type(qMetaTypeId<T>())); }

 public:
#ifndef qdoc
    struct PrivateShared
    {
        inline PrivateShared(void *v) : ptr(v), ref(1) { }
        void *ptr;
        QAtomicInt ref;
    };
    struct Private
    {
        inline Private(): type(Invalid), is_shared(false), is_null(true) { data.ptr = 0; }
        inline Private(const Private &other)
            : data(other.data), type(other.type),
              is_shared(other.is_shared), is_null(other.is_null)
        {}
        union Data
        {
            char c;
            int i;
            uint u;
            bool b;
            double d;
            float f;
            qreal real;
            qlonglong ll;
            qulonglong ull;
            QObject *o;
            void *ptr;
            PrivateShared *shared;
        } data;
        uint type : 30;
        uint is_shared : 1;
        uint is_null : 1;
    };
 public:
    typedef void (*f_construct)(Private *, const void *);
    typedef void (*f_clear)(Private *);
    typedef bool (*f_null)(const Private *);
#ifndef QT_NO_DATASTREAM
    typedef void (*f_load)(Private *, QDataStream &);
    typedef void (*f_save)(const Private *, QDataStream &);
#endif
    typedef bool (*f_compare)(const Private *, const Private *);
    typedef bool (*f_convert)(const QVariant::Private *d, Type t, void *, bool *);
    typedef bool (*f_canConvert)(const QVariant::Private *d, Type t);
    typedef void (*f_debugStream)(QDebug, const QVariant &);
    struct Handler {
        f_construct construct;
        f_clear clear;
        f_null isNull;
#ifndef QT_NO_DATASTREAM
        f_load load;
        f_save save;
#endif
        f_compare compare;
        f_convert convert;
        f_canConvert canConvert;
        f_debugStream debugStream;
    };
#endif

    inline bool operator==(const QVariant &v) const
    { return cmp(v); }
    inline bool operator!=(const QVariant &v) const
    { return !cmp(v); }

protected:
    friend inline bool qvariant_cast_helper(const QVariant &, QVariant::Type, void *);
    friend int qRegisterGuiVariant();
    friend int qUnregisterGuiVariant();
    friend inline bool operator==(const QVariant &, const QVariantComparisonHelper &);
#ifndef QT_NO_DEBUG_STREAM
    friend Q_CORE_EXPORT QDebug operator<<(QDebug, const QVariant &);
#endif
    Private d;

    static const Handler *handler;

    void create(int type, const void *copy);
#ifdef QT3_SUPPORT
    void *castOrDetach(Type t);
#endif
    bool cmp(const QVariant &other) const;

private:
    // force compile error, prevent QVariant(bool) to be called
    inline QVariant(void *) { Q_ASSERT(false); }
#ifdef QT_NO_CAST_FROM_ASCII
    // force compile error when implicit conversion is not wanted
    inline QVariant(const char *) { Q_ASSERT(false); }
#endif
#ifndef QT3_SUPPORT
    // force compile error, prevent QVariant(QVariant::Type, int) to be called
    inline QVariant(bool, int) { Q_ASSERT(false); }
#endif
public:
    typedef Private DataPtr;
    inline DataPtr &data_ptr() { return d; }
};

typedef QList<QVariant> QVariantList;
typedef QMap<QString, QVariant> QVariantMap;
typedef QHash<QString, QVariant> QVariantHash;

inline bool qvariant_cast_helper(const QVariant &v, QVariant::Type tp, void *ptr)
{ return QVariant::handler->convert(&v.d, tp, ptr, 0); }

template <typename T>
inline QVariant qVariantFromValue(const T &t)
{
    return QVariant(qMetaTypeId<T>(reinterpret_cast<T *>(0)), &t, QTypeInfo<T>::isPointer);
}

template <>
inline QVariant qVariantFromValue(const QVariant &t) { return t; }

template <typename T>
inline void qVariantSetValue(QVariant &v, const T &t)
{
    //if possible we reuse the current QVariant private
    const uint type = qMetaTypeId<T>(reinterpret_cast<T *>(0));
    QVariant::Private &d = v.data_ptr();
    if (v.isDetached() && (type == d.type || (type <= uint(QVariant::Char) && d.type <= uint(QVariant::Char)))) {
        d.type = type;
        d.is_null = false;
        T *old = reinterpret_cast<T*>(d.is_shared ? d.data.shared->ptr : &d.data.ptr);
        if (QTypeInfo<T>::isComplex)
            old->~T();
        new (old) T(t); //call the copy constructor
    } else {
        v = QVariant(type, &t, QTypeInfo<T>::isPointer);
    }
}

template <>
inline void qVariantSetValue<QVariant>(QVariant &v, const QVariant &t)
{
    v = t;
}


inline QVariant::QVariant() {}
inline bool QVariant::isValid() const { return d.type != Invalid; }

#ifdef QT3_SUPPORT
inline int &QVariant::asInt()
{ return *reinterpret_cast<int *>(castOrDetach(Int)); }
inline uint &QVariant::asUInt()
{ return *reinterpret_cast<uint *>(castOrDetach(UInt)); }
inline qlonglong &QVariant::asLongLong()
{ return *reinterpret_cast<qlonglong *>(castOrDetach(LongLong)); }
inline qulonglong &QVariant::asULongLong()
{ return *reinterpret_cast<qulonglong *>(castOrDetach(ULongLong)); }
inline bool &QVariant::asBool()
{ return *reinterpret_cast<bool *>(castOrDetach(Bool)); }
inline double &QVariant::asDouble()
{ return *reinterpret_cast<double *>(castOrDetach(Double)); }
inline QByteArray& QVariant::asByteArray()
{ return *reinterpret_cast<QByteArray *>(castOrDetach(ByteArray)); }
inline QBitArray& QVariant::asBitArray()
{ return *reinterpret_cast<QBitArray *>(castOrDetach(BitArray)); }
inline QString& QVariant::asString()
{ return *reinterpret_cast<QString *>(castOrDetach(String)); }
inline QStringList& QVariant::asStringList()
{ return *reinterpret_cast<QStringList *>(castOrDetach(StringList)); }
inline QDate& QVariant::asDate()
{ return *reinterpret_cast<QDate *>(castOrDetach(Date)); }
inline QTime& QVariant::asTime()
{ return *reinterpret_cast<QTime *>(castOrDetach(Time)); }
inline QDateTime& QVariant::asDateTime()
{ return *reinterpret_cast<QDateTime *>(castOrDetach(DateTime)); }
inline QList<QVariant>& QVariant::asList()
{ return *reinterpret_cast<QList<QVariant> *>(castOrDetach(List)); }
inline QMap<QString, QVariant>& QVariant::asMap()
{ return *reinterpret_cast<QMap<QString, QVariant> *>(castOrDetach(Map)); }
inline QPoint &QVariant::asPoint()
{ return *reinterpret_cast<QPoint *>(castOrDetach(Point)); }
inline QRect &QVariant::asRect()
{ return *reinterpret_cast<QRect *>(castOrDetach(Rect)); }
inline QSize &QVariant::asSize()
{ return *reinterpret_cast<QSize *>(castOrDetach(Size)); }
#endif //QT3_SUPPORT

template<typename T>
inline void QVariant::setValue(const T &avalue)
{ qVariantSetValue(*this, avalue); }

#ifndef QT_NO_DATASTREAM
Q_CORE_EXPORT QDataStream& operator>> (QDataStream& s, QVariant& p);
Q_CORE_EXPORT QDataStream& operator<< (QDataStream& s, const QVariant& p);
Q_CORE_EXPORT QDataStream& operator>> (QDataStream& s, QVariant::Type& p);
Q_CORE_EXPORT QDataStream& operator<< (QDataStream& s, const QVariant::Type p);
#endif

inline bool QVariant::isDetached() const
{ return !d.is_shared || d.data.shared->ref == 1; }


#ifdef qdoc
    inline bool operator==(const QVariant &v1, const QVariant &v2);
    inline bool operator!=(const QVariant &v1, const QVariant &v2);
#else

/* Helper class to add one more level of indirection to prevent
   implicit casts.
*/
class QVariantComparisonHelper
{
public:
    inline QVariantComparisonHelper(const QVariant &var)
        : v(&var) {}
private:
    friend inline bool operator==(const QVariant &, const QVariantComparisonHelper &);
    const QVariant *v;
};

inline bool operator==(const QVariant &v1, const QVariantComparisonHelper &v2)
{
    return v1.cmp(*v2.v);
}

inline bool operator!=(const QVariant &v1, const QVariantComparisonHelper &v2)
{
    return !operator==(v1, v2);
}
#endif

#ifndef QT_MOC
template<typename T> inline T qvariant_cast(const QVariant &v)
{
    const int vid = qMetaTypeId<T>(static_cast<T *>(0));
    if (vid == v.userType())
        return *reinterpret_cast<const T *>(v.constData());
    if (vid < int(QMetaType::User)) {
        T t;
        if (qvariant_cast_helper(v, QVariant::Type(vid), &t))
            return t;
    }
    return T();
}

template<> inline QVariant qvariant_cast<QVariant>(const QVariant &v)
{
    if (v.userType() == QMetaType::QVariant)
        return *reinterpret_cast<const QVariant *>(v.constData());
    return v;
}

#ifdef QT_DEPRECATED
template<typename T>
inline QT_DEPRECATED T qVariantValue(const QVariant &variant)
{ return qvariant_cast<T>(variant); }

template<typename T>
inline QT_DEPRECATED bool qVariantCanConvert(const QVariant &variant)
{ return variant.template canConvert<T>(); }
#endif

#endif
Q_DECLARE_SHARED(QVariant)
Q_DECLARE_TYPEINFO(QVariant, Q_MOVABLE_TYPE);

#ifndef QT_NO_DEBUG_STREAM
Q_CORE_EXPORT QDebug operator<<(QDebug, const QVariant &);
Q_CORE_EXPORT QDebug operator<<(QDebug, const QVariant::Type);
#endif

QT_END_NAMESPACE

Q_DECLARE_BUILTIN_METATYPE(QVariantList, QVariantList)
Q_DECLARE_BUILTIN_METATYPE(QVariantMap, QVariantMap)
Q_DECLARE_BUILTIN_METATYPE(QVariantHash, QVariantHash)

QT_END_HEADER

#endif // QVARIANT_H
