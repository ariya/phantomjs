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

#ifndef QSTRINGBUILDER_H
#define QSTRINGBUILDER_H

#include <QtCore/qstring.h>
#include <QtCore/qbytearray.h>

#if defined(Q_CC_GNU) && !defined(Q_CC_INTEL)
#  if __GNUC__ < 4 || (__GNUC__ == 4 && __GNUC_MINOR__ == 0)
#    include <QtCore/qmap.h>
#  endif
#endif

#include <string.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Core)

// ### Qt 5: merge with QLatin1String
class QLatin1Literal
{
public:
    int size() const { return m_size; }
    const char *data() const { return m_data; }

    template <int N>
    QLatin1Literal(const char (&str)[N])
        : m_size(N - 1), m_data(str) {}

private:
    const int m_size;
    const char * const m_data;
};

struct Q_CORE_EXPORT QAbstractConcatenable
{
protected:
    static void convertFromAscii(const char *a, int len, QChar *&out);
    static void convertToAscii(const QChar *a, int len, char *&out);
    static inline void convertFromAscii(char a, QChar *&out)
    {
#ifndef QT_NO_TEXTCODEC
        if (QString::codecForCStrings)
            *out++ = QChar::fromAscii(a);
        else
#endif
            *out++ = QLatin1Char(a);
    }

    static inline void convertToAscii(QChar a, char *&out)
    {
#ifndef QT_NO_TEXTCODEC
        if (QString::codecForCStrings)
            *out++ = a.toAscii(); //###
        else
#endif
            convertToLatin1(a, out);
    }

    static inline void convertToLatin1(QChar a, char *&out)
    {
        *out++ = a.unicode() > 0xff ? '?' : char(a.unicode());
    }
};

template <typename T> struct QConcatenable {};

template <typename A, typename B>
class QStringBuilder
{
public:
    QStringBuilder(const A &a_, const B &b_) : a(a_), b(b_) {}
private:
    friend class QByteArray;
    friend class QString;
    template <typename T> T convertTo() const
    {
        const uint len = QConcatenable< QStringBuilder<A, B> >::size(*this);
        T s(len, Qt::Uninitialized);

        typename T::iterator d = s.data();
        typename T::const_iterator const start = d;
        QConcatenable< QStringBuilder<A, B> >::appendTo(*this, d);

        if (!QConcatenable< QStringBuilder<A, B> >::ExactSize && int(len) != d - start) {
            // this resize is necessary since we allocate a bit too much
            // when dealing with variable sized 8-bit encodings
            s.resize(d - start);
        }
        return s;
    }

    typedef QConcatenable<QStringBuilder<A, B> > Concatenable;
    typedef typename Concatenable::ConvertTo ConvertTo;
public:
    operator ConvertTo() const { return convertTo<ConvertTo>(); }

    QByteArray toLatin1() const { return convertTo<QString>().toLatin1(); }
    int size() const { return Concatenable::size(*this); }

    const A &a;
    const B &b;
};

template <>
class QStringBuilder <QString, QString>
{
    public:
        QStringBuilder(const QString &a_, const QString &b_) : a(a_), b(b_) {}

        operator QString() const
        { QString r(a); r += b; return r; }
        QByteArray toLatin1() const { return QString(*this).toLatin1(); }

        const QString &a;
        const QString &b;
};

template <>
class QStringBuilder <QByteArray, QByteArray>
{
    public:
        QStringBuilder(const QByteArray &a_, const QByteArray &b_) : a(a_), b(b_) {}

        operator QByteArray() const
        { QByteArray r(a); r += b; return r; }

        const QByteArray &a;
        const QByteArray &b;
};


template <> struct QConcatenable<char> : private QAbstractConcatenable
{
    typedef char type;
    typedef QByteArray ConvertTo;
    enum { ExactSize = true };
    static int size(const char) { return 1; }
#ifndef QT_NO_CAST_FROM_ASCII
    static inline QT_ASCII_CAST_WARN void appendTo(const char c, QChar *&out)
    {
        QAbstractConcatenable::convertFromAscii(c, out);
    }
#endif
    static inline void appendTo(const char c, char *&out)
    { *out++ = c; }
};

template <> struct QConcatenable<QLatin1Char>
{
    typedef QLatin1Char type;
    typedef QString ConvertTo;
    enum { ExactSize = true };
    static int size(const QLatin1Char) { return 1; }
    static inline void appendTo(const QLatin1Char c, QChar *&out)
    { *out++ = c; }
    static inline void appendTo(const QLatin1Char c, char *&out)
    { *out++ = c.toLatin1(); }
};

template <> struct QConcatenable<QChar> : private QAbstractConcatenable
{
    typedef QChar type;
    typedef QString ConvertTo;
    enum { ExactSize = true };
    static int size(const QChar) { return 1; }
    static inline void appendTo(const QChar c, QChar *&out)
    { *out++ = c; }
#ifndef QT_NO_CAST_TO_ASCII
    static inline QT_ASCII_CAST_WARN void appendTo(const QChar c, char *&out)
    { convertToAscii(c, out); }
#endif
};

template <> struct QConcatenable<QCharRef> : private QAbstractConcatenable
{
    typedef QCharRef type;
    typedef QString ConvertTo;
    enum { ExactSize = true };
    static int size(const QCharRef &) { return 1; }
    static inline void appendTo(const QCharRef &c, QChar *&out)
    { *out++ = QChar(c); }
#ifndef QT_NO_CAST_TO_ASCII
    static inline QT_ASCII_CAST_WARN void appendTo(const QCharRef &c, char *&out)
    { convertToAscii(c, out); }
#endif
};

template <> struct QConcatenable<QLatin1String>
{
    typedef QLatin1String type;
    typedef QString ConvertTo;
    enum { ExactSize = true };
    static int size(const QLatin1String &a) { return qstrlen(a.latin1()); }
    static inline void appendTo(const QLatin1String &a, QChar *&out)
    {
        for (const char *s = a.latin1(); *s; )
            *out++ = QLatin1Char(*s++);
    }
    static inline void appendTo(const QLatin1String &a, char *&out)
    {
        for (const char *s = a.latin1(); *s; )
            *out++ = *s++;
    }
};

template <> struct QConcatenable<QLatin1Literal>
{
    typedef QLatin1Literal type;
    typedef QString ConvertTo;
    enum { ExactSize = true };
    static int size(const QLatin1Literal &a) { return a.size(); }
    static inline void appendTo(const QLatin1Literal &a, QChar *&out)
    {
        for (const char *s = a.data(); *s; )
            *out++ = QLatin1Char(*s++);
    }
    static inline void appendTo(const QLatin1Literal &a, char *&out)
    {
        for (const char *s = a.data(); *s; )
            *out++ = *s++;
    }
};

template <> struct QConcatenable<QString> : private QAbstractConcatenable
{
    typedef QString type;
    typedef QString ConvertTo;
    enum { ExactSize = true };
    static int size(const QString &a) { return a.size(); }
    static inline void appendTo(const QString &a, QChar *&out)
    {
        const int n = a.size();
        memcpy(out, reinterpret_cast<const char*>(a.constData()), sizeof(QChar) * n);
        out += n;
    }
#ifndef QT_NO_CAST_TO_ASCII
    static inline QT_ASCII_CAST_WARN void appendTo(const QString &a, char *&out)
    { convertToAscii(a.constData(), a.length(), out); }
#endif
};

template <> struct QConcatenable<QStringRef> : private QAbstractConcatenable
{
    typedef QStringRef type;
    typedef QString ConvertTo;
    enum { ExactSize = true };
    static int size(const QStringRef &a) { return a.size(); }
    static inline void appendTo(const QStringRef &a, QChar *&out)
    {
        const int n = a.size();
        memcpy(out, reinterpret_cast<const char*>(a.constData()), sizeof(QChar) * n);
        out += n;
    }
#ifndef QT_NO_CAST_TO_ASCII
    static inline QT_ASCII_CAST_WARN void appendTo(const QStringRef &a, char *&out)
    { convertToAscii(a.constData(), a.length(), out); }
#endif

};

template <int N> struct QConcatenable<char[N]> : private QAbstractConcatenable
{
    typedef char type[N];
    typedef QByteArray ConvertTo;
    enum { ExactSize = false };
    static int size(const char[N]) { return N - 1; }
#ifndef QT_NO_CAST_FROM_ASCII
    static inline void QT_ASCII_CAST_WARN appendTo(const char a[N], QChar *&out)
    {
        QAbstractConcatenable::convertFromAscii(a, N, out);
    }
#endif
    static inline void appendTo(const char a[N], char *&out)
    {
        while (*a)
            *out++ = *a++;
    }
};

template <int N> struct QConcatenable<const char[N]> : private QAbstractConcatenable
{
    typedef const char type[N];
    typedef QByteArray ConvertTo;
    enum { ExactSize = false };
    static int size(const char[N]) { return N - 1; }
#ifndef QT_NO_CAST_FROM_ASCII
    static inline void QT_ASCII_CAST_WARN appendTo(const char a[N], QChar *&out)
    {
        QAbstractConcatenable::convertFromAscii(a, N, out);
    }
#endif
    static inline void appendTo(const char a[N], char *&out)
    {
        while (*a)
            *out++ = *a++;
    }
};

template <> struct QConcatenable<const char *> : private QAbstractConcatenable
{
    typedef char const *type;
    typedef QByteArray ConvertTo;
    enum { ExactSize = false };
    static int size(const char *a) { return qstrlen(a); }
#ifndef QT_NO_CAST_FROM_ASCII
    static inline void QT_ASCII_CAST_WARN appendTo(const char *a, QChar *&out)
    { QAbstractConcatenable::convertFromAscii(a, -1, out); }
#endif
    static inline void appendTo(const char *a, char *&out)
    {
        if (!a)
            return;
        while (*a)
            *out++ = *a++;
    }
};

template <> struct QConcatenable<QByteArray> : private QAbstractConcatenable
{
    typedef QByteArray type;
    typedef QByteArray ConvertTo;
    enum { ExactSize = false };
    static int size(const QByteArray &ba) { return ba.size(); }
#ifndef QT_NO_CAST_FROM_ASCII
    static inline void appendTo(const QByteArray &ba, QChar *&out)
    {
        // adding 1 because convertFromAscii expects the size including the null-termination
        QAbstractConcatenable::convertFromAscii(ba.constData(), ba.size() + 1, out);
    }
#endif
    static inline void appendTo(const QByteArray &ba, char *&out)
    {
        const char *a = ba.constData();
        const char * const end = ba.end();
        while (a != end)
            *out++ = *a++;
    }
};

namespace QtStringBuilder {
    template <typename A, typename B> struct ConvertToTypeHelper
    { typedef A ConvertTo; };
    template <typename T> struct ConvertToTypeHelper<T, QString>
    { typedef QString ConvertTo; };
}

template <typename A, typename B>
struct QConcatenable< QStringBuilder<A, B> >
{
    typedef QStringBuilder<A, B> type;
    typedef typename QtStringBuilder::ConvertToTypeHelper<typename QConcatenable<A>::ConvertTo, typename QConcatenable<B>::ConvertTo>::ConvertTo ConvertTo;
    enum { ExactSize = QConcatenable<A>::ExactSize && QConcatenable<B>::ExactSize };
    static int size(const type &p)
    {
        return QConcatenable<A>::size(p.a) + QConcatenable<B>::size(p.b);
    }
    template<typename T> static inline void appendTo(const type &p, T *&out)
    {
        QConcatenable<A>::appendTo(p.a, out);
        QConcatenable<B>::appendTo(p.b, out);
    }
};

template <typename A, typename B>
QStringBuilder<typename QConcatenable<A>::type, typename QConcatenable<B>::type>
operator%(const A &a, const B &b)
{
   return QStringBuilder<typename QConcatenable<A>::type, typename QConcatenable<B>::type>(a, b);
}

// QT_USE_FAST_OPERATOR_PLUS was introduced in 4.7, QT_USE_QSTRINGBUILDER is to be used from 4.8 onwards
// QT_USE_FAST_OPERATOR_PLUS does not remove the normal operator+ for QByteArray
#if defined(QT_USE_FAST_OPERATOR_PLUS) || defined(QT_USE_QSTRINGBUILDER)
template <typename A, typename B>
QStringBuilder<typename QConcatenable<A>::type, typename QConcatenable<B>::type>
operator+(const A &a, const B &b)
{
   return QStringBuilder<typename QConcatenable<A>::type, typename QConcatenable<B>::type>(a, b);
}
#endif

template <typename A, typename B>
QByteArray &operator+=(QByteArray &a, const QStringBuilder<A, B> &b)
{
#ifndef QT_NO_CAST_TO_ASCII
    if (sizeof(typename QConcatenable< QStringBuilder<A, B> >::ConvertTo::value_type) == sizeof(QChar)) {
        //it is not save to optimize as in utf8 it is not possible to compute the size
        return a += QString(b);
    }
#endif
    int len = a.size() + QConcatenable< QStringBuilder<A, B> >::size(b);
    a.reserve(len);
    char *it = a.data() + a.size();
    QConcatenable< QStringBuilder<A, B> >::appendTo(b, it);
    a.resize(len); //we need to resize after the appendTo for the case str+=foo+str
    return a;
}

template <typename A, typename B>
QString &operator+=(QString &a, const QStringBuilder<A, B> &b)
{
    int len = a.size() + QConcatenable< QStringBuilder<A, B> >::size(b);
    a.reserve(len);
    QChar *it = a.data() + a.size();
    QConcatenable< QStringBuilder<A, B> >::appendTo(b, it);
    a.resize(it - a.constData()); //may be smaller than len if there was conversion from utf8
    return a;
}


QT_END_NAMESPACE

QT_END_HEADER

#endif // QSTRINGBUILDER_H
