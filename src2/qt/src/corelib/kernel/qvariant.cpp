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

#include "qvariant.h"
#include "qbitarray.h"
#include "qbytearray.h"
#include "qdatastream.h"
#include "qdebug.h"
#include "qmap.h"
#include "qdatetime.h"
#include "qeasingcurve.h"
#include "qlist.h"
#include "qstring.h"
#include "qstringlist.h"
#include "qurl.h"
#include "qlocale.h"
#include "private/qvariant_p.h"

#ifndef QT_NO_GEOM_VARIANT
#include "qsize.h"
#include "qpoint.h"
#include "qrect.h"
#include "qline.h"
#endif

#include <float.h>

QT_BEGIN_NAMESPACE

#ifndef DBL_DIG
#  define DBL_DIG 10
#endif
#ifndef FLT_DIG
#  define FLT_DIG 6
#endif

static void construct(QVariant::Private *x, const void *copy)
{
    x->is_shared = false;

    switch (x->type) {
    case QVariant::String:
        v_construct<QString>(x, copy);
        break;
    case QVariant::Char:
        v_construct<QChar>(x, copy);
        break;
    case QVariant::StringList:
        v_construct<QStringList>(x, copy);
        break;
    case QVariant::Map:
        v_construct<QVariantMap>(x, copy);
        break;
    case QVariant::Hash:
        v_construct<QVariantHash>(x, copy);
        break;
    case QVariant::List:
        v_construct<QVariantList>(x, copy);
        break;
    case QVariant::Date:
        v_construct<QDate>(x, copy);
        break;
    case QVariant::Time:
        v_construct<QTime>(x, copy);
        break;
    case QVariant::DateTime:
        v_construct<QDateTime>(x, copy);
        break;
    case QVariant::ByteArray:
        v_construct<QByteArray>(x, copy);
        break;
    case QVariant::BitArray:
        v_construct<QBitArray>(x, copy);
        break;
#ifndef QT_NO_GEOM_VARIANT
    case QVariant::Size:
        v_construct<QSize>(x, copy);
        break;
    case QVariant::SizeF:
        v_construct<QSizeF>(x, copy);
        break;
    case QVariant::Rect:
        v_construct<QRect>(x, copy);
        break;
    case QVariant::LineF:
        v_construct<QLineF>(x, copy);
        break;
    case QVariant::Line:
        v_construct<QLine>(x, copy);
        break;
    case QVariant::RectF:
        v_construct<QRectF>(x, copy);
        break;
    case QVariant::Point:
        v_construct<QPoint>(x, copy);
        break;
    case QVariant::PointF:
        v_construct<QPointF>(x, copy);
        break;
#endif
#ifndef QT_BOOTSTRAPPED
    case QVariant::Url:
        v_construct<QUrl>(x, copy);
        break;
#endif
    case QVariant::Locale:
        v_construct<QLocale>(x, copy);
        break;
#ifndef QT_NO_REGEXP
    case QVariant::RegExp:
        v_construct<QRegExp>(x, copy);
        break;
#endif
#ifndef QT_BOOTSTRAPPED
    case QVariant::EasingCurve:
        v_construct<QEasingCurve>(x, copy);
        break;
#endif
    case QVariant::Int:
        x->data.i = copy ? *static_cast<const int *>(copy) : 0;
        break;
    case QVariant::UInt:
        x->data.u = copy ? *static_cast<const uint *>(copy) : 0u;
        break;
    case QVariant::Bool:
        x->data.b = copy ? *static_cast<const bool *>(copy) : false;
        break;
    case QVariant::Double:
        x->data.d = copy ? *static_cast<const double*>(copy) : 0.0;
        break;
    case QMetaType::Float:
        x->data.f = copy ? *static_cast<const float*>(copy) : 0.0f;
        break;
    case QMetaType::QObjectStar:
        x->data.o = copy ? *static_cast<QObject *const*>(copy) : 0;
        break;
    case QVariant::LongLong:
        x->data.ll = copy ? *static_cast<const qlonglong *>(copy) : Q_INT64_C(0);
        break;
    case QVariant::ULongLong:
        x->data.ull = copy ? *static_cast<const qulonglong *>(copy) : Q_UINT64_C(0);
        break;
    case QVariant::Invalid:
    case QVariant::UserType:
        break;
    default:
        void *ptr = QMetaType::construct(x->type, copy);
        if (!ptr) {
            x->type = QVariant::Invalid;
        } else {
            x->is_shared = true;
            x->data.shared = new QVariant::PrivateShared(ptr);
        }
        break;
    }
    x->is_null = !copy;
}

static void clear(QVariant::Private *d)
{
    switch (d->type) {
    case QVariant::String:
        v_clear<QString>(d);
        break;
    case QVariant::Char:
        v_clear<QChar>(d);
        break;
    case QVariant::StringList:
        v_clear<QStringList>(d);
        break;
    case QVariant::Map:
        v_clear<QVariantMap>(d);
        break;
    case QVariant::Hash:
        v_clear<QVariantHash>(d);
        break;
    case QVariant::List:
        v_clear<QVariantList>(d);
        break;
    case QVariant::Date:
        v_clear<QDate>(d);
        break;
    case QVariant::Time:
        v_clear<QTime>(d);
        break;
    case QVariant::DateTime:
        v_clear<QDateTime>(d);
        break;
    case QVariant::ByteArray:
        v_clear<QByteArray>(d);
        break;
    case QVariant::BitArray:
        v_clear<QBitArray>(d);
        break;
#ifndef QT_NO_GEOM_VARIANT
    case QVariant::Point:
        v_clear<QPoint>(d);
        break;
    case QVariant::PointF:
        v_clear<QPointF>(d);
        break;
    case QVariant::Size:
        v_clear<QSize>(d);
        break;
    case QVariant::SizeF:
        v_clear<QSizeF>(d);
        break;
    case QVariant::Rect:
        v_clear<QRect>(d);
        break;
    case QVariant::LineF:
        v_clear<QLineF>(d);
        break;
    case QVariant::Line:
        v_clear<QLine>(d);
        break;
    case QVariant::RectF:
        v_clear<QRectF>(d);
        break;
#endif
#ifndef QT_BOOTSTRAPPED
    case QVariant::Url:
        v_clear<QUrl>(d);
        break;
#endif
    case QVariant::Locale:
        v_clear<QLocale>(d);
        break;
#ifndef QT_NO_REGEXP
    case QVariant::RegExp:
        v_clear<QRegExp>(d);
        break;
#endif
#ifndef QT_BOOTSTRAPPED
    case QVariant::EasingCurve:
        v_clear<QEasingCurve>(d);
        break;
#endif
    case QVariant::LongLong:
    case QVariant::ULongLong:
    case QVariant::Double:
    case QMetaType::Float:
    case QMetaType::QObjectStar:
        break;
    case QVariant::Invalid:
    case QVariant::UserType:
    case QVariant::Int:
    case QVariant::UInt:
    case QVariant::Bool:
        break;
    default:
        QMetaType::destroy(d->type, d->data.shared->ptr);
        delete d->data.shared;
        break;
    }

    d->type = QVariant::Invalid;
    d->is_null = true;
    d->is_shared = false;
}

static bool isNull(const QVariant::Private *d)
{
    switch(d->type) {
    case QVariant::String:
        return v_cast<QString>(d)->isNull();
    case QVariant::Char:
        return v_cast<QChar>(d)->isNull();
    case QVariant::Date:
        return v_cast<QDate>(d)->isNull();
    case QVariant::Time:
        return v_cast<QTime>(d)->isNull();
    case QVariant::DateTime:
        return v_cast<QDateTime>(d)->isNull();
    case QVariant::ByteArray:
        return v_cast<QByteArray>(d)->isNull();
    case QVariant::BitArray:
        return v_cast<QBitArray>(d)->isNull();
#ifndef QT_NO_GEOM_VARIANT
    case QVariant::Size:
        return v_cast<QSize>(d)->isNull();
    case QVariant::SizeF:
        return v_cast<QSizeF>(d)->isNull();
    case QVariant::Rect:
        return v_cast<QRect>(d)->isNull();
    case QVariant::Line:
        return v_cast<QLine>(d)->isNull();
    case QVariant::LineF:
        return v_cast<QLineF>(d)->isNull();
    case QVariant::RectF:
        return v_cast<QRectF>(d)->isNull();
    case QVariant::Point:
        return v_cast<QPoint>(d)->isNull();
    case QVariant::PointF:
        return v_cast<QPointF>(d)->isNull();
#endif
#ifndef QT_BOOTSTRAPPED
    case QVariant::EasingCurve:
    case QVariant::Url:
#endif
    case QVariant::Locale:
    case QVariant::RegExp:
    case QVariant::StringList:
    case QVariant::Map:
    case QVariant::Hash:
    case QVariant::List:
    case QVariant::Invalid:
    case QVariant::UserType:
    case QVariant::Int:
    case QVariant::UInt:
    case QVariant::LongLong:
    case QVariant::ULongLong:
    case QVariant::Bool:
    case QVariant::Double:
    case QMetaType::Float:
    case QMetaType::QObjectStar:
        break;
    }
    return d->is_null;
}

/*
  \internal
  \since 4.4

  We cannot use v_cast() for QMetaType's numeric types because they're smaller than QVariant::Private::Data,
  which in turns makes v_cast() believe the value is stored in d->data.c. But
  it's not, since we're a QMetaType type.
 */
template<typename T>
inline bool compareNumericMetaType(const QVariant::Private *const a, const QVariant::Private *const b)
{
    return *static_cast<const T *>(a->data.shared->ptr) == *static_cast<const T *>(b->data.shared->ptr);
}

/*!
  \internal

  Compares \a a to \a b. The caller guarantees that \a a and \a b
  are of the same type.
 */
static bool compare(const QVariant::Private *a, const QVariant::Private *b)
{
    switch(a->type) {
    case QVariant::List:
        return *v_cast<QVariantList>(a) == *v_cast<QVariantList>(b);
    case QVariant::Map: {
        const QVariantMap *m1 = v_cast<QVariantMap>(a);
        const QVariantMap *m2 = v_cast<QVariantMap>(b);
        if (m1->count() != m2->count())
            return false;
        QVariantMap::ConstIterator it = m1->constBegin();
        QVariantMap::ConstIterator it2 = m2->constBegin();
        while (it != m1->constEnd()) {
            if (*it != *it2 || it.key() != it2.key())
                return false;
            ++it;
            ++it2;
        }
        return true;
    }
    case QVariant::Hash:
        return *v_cast<QVariantHash>(a) == *v_cast<QVariantHash>(b);
    case QVariant::String:
        return *v_cast<QString>(a) == *v_cast<QString>(b);
    case QVariant::Char:
        return *v_cast<QChar>(a) == *v_cast<QChar>(b);
    case QVariant::StringList:
        return *v_cast<QStringList>(a) == *v_cast<QStringList>(b);
#ifndef QT_NO_GEOM_VARIANT
    case QVariant::Size:
        return *v_cast<QSize>(a) == *v_cast<QSize>(b);
    case QVariant::SizeF:
        return *v_cast<QSizeF>(a) == *v_cast<QSizeF>(b);
    case QVariant::Rect:
        return *v_cast<QRect>(a) == *v_cast<QRect>(b);
    case QVariant::Line:
        return *v_cast<QLine>(a) == *v_cast<QLine>(b);
    case QVariant::LineF:
        return *v_cast<QLineF>(a) == *v_cast<QLineF>(b);
    case QVariant::RectF:
        return *v_cast<QRectF>(a) == *v_cast<QRectF>(b);
    case QVariant::Point:
        return *v_cast<QPoint>(a) == *v_cast<QPoint>(b);
    case QVariant::PointF:
        return *v_cast<QPointF>(a) == *v_cast<QPointF>(b);
#endif
#ifndef QT_BOOTSTRAPPED
    case QVariant::Url:
        return *v_cast<QUrl>(a) == *v_cast<QUrl>(b);
#endif
    case QVariant::Locale:
        return *v_cast<QLocale>(a) == *v_cast<QLocale>(b);
#ifndef QT_NO_REGEXP
    case QVariant::RegExp:
        return *v_cast<QRegExp>(a) == *v_cast<QRegExp>(b);
#endif
    case QVariant::Int:
        return a->data.i == b->data.i;
    case QVariant::UInt:
        return a->data.u == b->data.u;
    case QVariant::LongLong:
        return a->data.ll == b->data.ll;
    case QVariant::ULongLong:
        return a->data.ull == b->data.ull;
    case QVariant::Bool:
        return a->data.b == b->data.b;
    case QVariant::Double:
        return a->data.d == b->data.d;
    case QMetaType::Float:
        return a->data.f == b->data.f;
    case QMetaType::QObjectStar:
        return a->data.o == b->data.o;
    case QVariant::Date:
        return *v_cast<QDate>(a) == *v_cast<QDate>(b);
    case QVariant::Time:
        return *v_cast<QTime>(a) == *v_cast<QTime>(b);
    case QVariant::DateTime:
        return *v_cast<QDateTime>(a) == *v_cast<QDateTime>(b);
#ifndef QT_BOOTSTRAPPED
    case QVariant::EasingCurve:
        return *v_cast<QEasingCurve>(a) == *v_cast<QEasingCurve>(b);
#endif
    case QVariant::ByteArray:
        return *v_cast<QByteArray>(a) == *v_cast<QByteArray>(b);
    case QVariant::BitArray:
        return *v_cast<QBitArray>(a) == *v_cast<QBitArray>(b);
    case QVariant::Invalid:
        return true;
    case QMetaType::Long:
        return compareNumericMetaType<long>(a, b);
    case QMetaType::ULong:
        return compareNumericMetaType<ulong>(a, b);
    case QMetaType::Short:
        return compareNumericMetaType<short>(a, b);
    case QMetaType::UShort:
        return compareNumericMetaType<ushort>(a, b);
    case QMetaType::UChar:
        return compareNumericMetaType<uchar>(a, b);
    case QMetaType::Char:
        return compareNumericMetaType<char>(a, b);
    default:
        break;
    }
    if (!QMetaType::isRegistered(a->type))
        qFatal("QVariant::compare: type %d unknown to QVariant.", a->type);

    const void *a_ptr = a->is_shared ? a->data.shared->ptr : &(a->data.ptr);
    const void *b_ptr = b->is_shared ? b->data.shared->ptr : &(b->data.ptr);

    /* The reason we cannot place this test in a case branch above for the types
     * QMetaType::VoidStar, QMetaType::QObjectStar and so forth, is that it wouldn't include
     * user defined pointer types. */
    const char *const typeName = QMetaType::typeName(a->type);
    uint typeNameLen = qstrlen(typeName);
    if (typeNameLen > 0 && typeName[typeNameLen - 1] == '*')
        return *static_cast<void *const *>(a_ptr) == *static_cast<void *const *>(b_ptr);

    if (a->is_null && b->is_null)
        return true;

    return a_ptr == b_ptr;
}

/*!
  \internal
 */
static qlonglong qMetaTypeNumber(const QVariant::Private *d)
{
    switch (d->type) {
    case QMetaType::Int:
        return d->data.i;
    case QMetaType::LongLong:
        return d->data.ll;
    case QMetaType::Char:
        return qlonglong(*static_cast<signed char *>(d->data.shared->ptr));
    case QMetaType::Short:
        return qlonglong(*static_cast<short *>(d->data.shared->ptr));
    case QMetaType::Long:
        return qlonglong(*static_cast<long *>(d->data.shared->ptr));
    case QMetaType::Float:
        return qRound64(d->data.f);
    case QVariant::Double:
        return qRound64(d->data.d);
    }
    Q_ASSERT(false);
    return 0;
}

static qulonglong qMetaTypeUNumber(const QVariant::Private *d)
{
    switch (d->type) {
    case QVariant::UInt:
        return d->data.u;
    case QVariant::ULongLong:
        return d->data.ull;
    case QMetaType::UChar:
        return qulonglong(*static_cast<unsigned char *>(d->data.shared->ptr));
    case QMetaType::UShort:
        return qulonglong(*static_cast<ushort *>(d->data.shared->ptr));
    case QMetaType::ULong:
        return qulonglong(*static_cast<ulong *>(d->data.shared->ptr));
    }
    Q_ASSERT(false);
    return 0;
}

static qlonglong qConvertToNumber(const QVariant::Private *d, bool *ok)
{
    *ok = true;

    switch (uint(d->type)) {
    case QVariant::String:
        return v_cast<QString>(d)->toLongLong(ok);
    case QVariant::Char:
        return v_cast<QChar>(d)->unicode();
    case QVariant::ByteArray:
        return v_cast<QByteArray>(d)->toLongLong(ok);
    case QVariant::Bool:
        return qlonglong(d->data.b);
    case QVariant::Double:
    case QVariant::Int:
    case QMetaType::Char:
    case QMetaType::Short:
    case QMetaType::Long:
    case QMetaType::Float:
    case QMetaType::LongLong:
        return qMetaTypeNumber(d);
    case QVariant::ULongLong:
    case QVariant::UInt:
    case QMetaType::UChar:
    case QMetaType::UShort:
    case QMetaType::ULong:
        return qlonglong(qMetaTypeUNumber(d));
    }

    *ok = false;
    return Q_INT64_C(0);
}

static qulonglong qConvertToUnsignedNumber(const QVariant::Private *d, bool *ok)
{
    *ok = true;

    switch (uint(d->type)) {
    case QVariant::String:
        return v_cast<QString>(d)->toULongLong(ok);
    case QVariant::Char:
        return v_cast<QChar>(d)->unicode();
    case QVariant::ByteArray:
        return v_cast<QByteArray>(d)->toULongLong(ok);
    case QVariant::Bool:
        return qulonglong(d->data.b);
    case QVariant::Double:
    case QVariant::Int:
    case QMetaType::Char:
    case QMetaType::Short:
    case QMetaType::Long:
    case QMetaType::Float:
    case QMetaType::LongLong:
        return qulonglong(qMetaTypeNumber(d));
    case QVariant::ULongLong:
    case QVariant::UInt:
    case QMetaType::UChar:
    case QMetaType::UShort:
    case QMetaType::ULong:
        return qMetaTypeUNumber(d);
    }

    *ok = false;
    return Q_UINT64_C(0);
}

template<typename TInput, typename LiteralWrapper>
inline bool qt_convertToBool(const QVariant::Private *const d)
{
    TInput str = v_cast<TInput>(d)->toLower();
    return !(str == LiteralWrapper("0") || str == LiteralWrapper("false") || str.isEmpty());
}

/*!
 \internal

 Converts \a d to type \a t, which is placed in \a result.
 */
static bool convert(const QVariant::Private *d, QVariant::Type t, void *result, bool *ok)
{
    Q_ASSERT(d->type != uint(t));
    Q_ASSERT(result);

    bool dummy;
    if (!ok)
        ok = &dummy;

    switch (uint(t)) {
#ifndef QT_BOOTSTRAPPED
    case QVariant::Url:
        switch (d->type) {
        case QVariant::String:
            *static_cast<QUrl *>(result) = QUrl(*v_cast<QString>(d));
            break;
        default:
            return false;
        }
        break;
#endif
    case QVariant::String: {
        QString *str = static_cast<QString *>(result);
        switch (d->type) {
        case QVariant::Char:
            *str = QString(*v_cast<QChar>(d));
            break;
        case QMetaType::Char:
        case QMetaType::UChar:
            *str = QChar::fromAscii(*static_cast<char *>(d->data.shared->ptr));
            break;
        case QMetaType::Short:
        case QMetaType::Long:
        case QVariant::Int:
        case QVariant::LongLong:
            *str = QString::number(qMetaTypeNumber(d));
            break;
        case QVariant::UInt:
        case QVariant::ULongLong:
        case QMetaType::UShort:
        case QMetaType::ULong:
            *str = QString::number(qMetaTypeUNumber(d));
            break;
        case QMetaType::Float:
            *str = QString::number(d->data.f, 'g', FLT_DIG);
            break;
        case QVariant::Double:
            *str = QString::number(d->data.d, 'g', DBL_DIG);
            break;
#if !defined(QT_NO_DATESTRING)
        case QVariant::Date:
            *str = v_cast<QDate>(d)->toString(Qt::ISODate);
            break;
        case QVariant::Time:
            *str = v_cast<QTime>(d)->toString(Qt::ISODate);
            break;
        case QVariant::DateTime:
            *str = v_cast<QDateTime>(d)->toString(Qt::ISODate);
            break;
#endif
        case QVariant::Bool:
            *str = QLatin1String(d->data.b ? "true" : "false");
            break;
        case QVariant::ByteArray:
            *str = QString::fromAscii(v_cast<QByteArray>(d)->constData());
            break;
        case QVariant::StringList:
            if (v_cast<QStringList>(d)->count() == 1)
                *str = v_cast<QStringList>(d)->at(0);
            break;
#ifndef QT_BOOTSTRAPPED
        case QVariant::Url:
            *str = v_cast<QUrl>(d)->toString();
            break;
#endif
        default:
            return false;
        }
        break;
    }
    case QVariant::Char: {
        QChar *c = static_cast<QChar *>(result);
        switch (d->type) {
        case QVariant::Int:
        case QVariant::LongLong:
        case QMetaType::Char:
        case QMetaType::Short:
        case QMetaType::Long:
        case QMetaType::Float:
            *c = QChar(ushort(qMetaTypeNumber(d)));
            break;
        case QVariant::UInt:
        case QVariant::ULongLong:
        case QMetaType::UChar:
        case QMetaType::UShort:
        case QMetaType::ULong:
            *c = QChar(ushort(qMetaTypeUNumber(d)));
            break;
        default:
            return false;
        }
        break;
    }
#ifndef QT_NO_GEOM_VARIANT
    case QVariant::Size: {
        QSize *s = static_cast<QSize *>(result);
        switch (d->type) {
        case QVariant::SizeF:
            *s = v_cast<QSizeF>(d)->toSize();
            break;
        default:
            return false;
        }
        break;
    }

    case QVariant::SizeF: {
        QSizeF *s = static_cast<QSizeF *>(result);
        switch (d->type) {
        case QVariant::Size:
            *s = QSizeF(*(v_cast<QSize>(d)));
            break;
        default:
            return false;
        }
        break;
    }

    case QVariant::Line: {
        QLine *s = static_cast<QLine *>(result);
        switch (d->type) {
        case QVariant::LineF:
            *s = v_cast<QLineF>(d)->toLine();
            break;
        default:
            return false;
        }
        break;
    }

    case QVariant::LineF: {
        QLineF *s = static_cast<QLineF *>(result);
        switch (d->type) {
        case QVariant::Line:
            *s = QLineF(*(v_cast<QLine>(d)));
            break;
        default:
            return false;
        }
        break;
    }
#endif
    case QVariant::StringList:
        if (d->type == QVariant::List) {
            QStringList *slst = static_cast<QStringList *>(result);
            const QVariantList *list = v_cast<QVariantList >(d);
            for (int i = 0; i < list->size(); ++i)
                slst->append(list->at(i).toString());
        } else if (d->type == QVariant::String) {
            QStringList *slst = static_cast<QStringList *>(result);
            *slst = QStringList(*v_cast<QString>(d));
        } else {
            return false;
        }
        break;
    case QVariant::Date: {
        QDate *dt = static_cast<QDate *>(result);
        if (d->type == QVariant::DateTime)
            *dt = v_cast<QDateTime>(d)->date();
#ifndef QT_NO_DATESTRING
        else if (d->type == QVariant::String)
            *dt = QDate::fromString(*v_cast<QString>(d), Qt::ISODate);
#endif
        else
            return false;

        return dt->isValid();
    }
    case QVariant::Time: {
        QTime *t = static_cast<QTime *>(result);
        switch (d->type) {
        case QVariant::DateTime:
            *t = v_cast<QDateTime>(d)->time();
            break;
#ifndef QT_NO_DATESTRING
        case QVariant::String:
            *t = QTime::fromString(*v_cast<QString>(d), Qt::ISODate);
            break;
#endif
        default:
            return false;
        }
        return t->isValid();
    }
    case QVariant::DateTime: {
        QDateTime *dt = static_cast<QDateTime *>(result);
        switch (d->type) {
#ifndef QT_NO_DATESTRING
        case QVariant::String:
            *dt = QDateTime::fromString(*v_cast<QString>(d), Qt::ISODate);
            break;
#endif
        case QVariant::Date:
            *dt = QDateTime(*v_cast<QDate>(d));
            break;
        default:
            return false;
        }
        return dt->isValid();
    }
    case QVariant::ByteArray: {
        QByteArray *ba = static_cast<QByteArray *>(result);
        switch (d->type) {
        case QVariant::String:
            *ba = v_cast<QString>(d)->toAscii();
            break;
        case QVariant::Double:
            *ba = QByteArray::number(d->data.d, 'g', DBL_DIG);
            break;
        case QMetaType::Float:
            *ba = QByteArray::number(d->data.f, 'g', FLT_DIG);
            break;
        case QMetaType::Char:
        case QMetaType::UChar:
            *ba = QByteArray(1, *static_cast<char *>(d->data.shared->ptr));
            break;
        case QVariant::Int:
        case QVariant::LongLong:
        case QMetaType::Short:
        case QMetaType::Long:
            *ba = QByteArray::number(qMetaTypeNumber(d));
            break;
        case QVariant::UInt:
        case QVariant::ULongLong:
        case QMetaType::UShort:
        case QMetaType::ULong:
            *ba = QByteArray::number(qMetaTypeUNumber(d));
            break;
        case QVariant::Bool:
            *ba = QByteArray(d->data.b ? "true" : "false");
            break;
        default:
            return false;
        }
    }
    break;
    case QMetaType::Short:
        *static_cast<short *>(result) = short(qConvertToNumber(d, ok));
        return *ok;
    case QMetaType::Long:
        *static_cast<long *>(result) = long(qConvertToNumber(d, ok));
        return *ok;
    case QMetaType::UShort:
        *static_cast<ushort *>(result) = ushort(qConvertToUnsignedNumber(d, ok));
        return *ok;
    case QMetaType::ULong:
        *static_cast<ulong *>(result) = ulong(qConvertToUnsignedNumber(d, ok));
        return *ok;
    case QVariant::Int:
        *static_cast<int *>(result) = int(qConvertToNumber(d, ok));
        return *ok;
    case QVariant::UInt:
        *static_cast<uint *>(result) = uint(qConvertToUnsignedNumber(d, ok));
        return *ok;
    case QVariant::LongLong:
        *static_cast<qlonglong *>(result) = qConvertToNumber(d, ok);
        return *ok;
    case QVariant::ULongLong: {
        *static_cast<qulonglong *>(result) = qConvertToUnsignedNumber(d, ok);
        return *ok;
    }
    case QMetaType::UChar: {
        *static_cast<uchar *>(result) = qConvertToUnsignedNumber(d, ok);
        return *ok;
    }
    case QVariant::Bool: {
        bool *b = static_cast<bool *>(result);
        switch(d->type) {
        case QVariant::ByteArray:
            *b = qt_convertToBool<QByteArray, QByteArray>(d);
            break;
        case QVariant::String:
            *b = qt_convertToBool<QString, QLatin1String>(d);
            break;
        case QVariant::Char:
            *b = !v_cast<QChar>(d)->isNull();
            break;
        case QVariant::Double:
        case QVariant::Int:
        case QVariant::LongLong:
        case QMetaType::Char:
        case QMetaType::Short:
        case QMetaType::Long:
        case QMetaType::Float:
            *b = qMetaTypeNumber(d) != Q_INT64_C(0);
            break;
        case QVariant::UInt:
        case QVariant::ULongLong:
        case QMetaType::UChar:
        case QMetaType::UShort:
        case QMetaType::ULong:
            *b = qMetaTypeUNumber(d) != Q_UINT64_C(0);
            break;
        default:
            *b = false;
            return false;
        }
        break;
    }
    case QVariant::Double: {
        double *f = static_cast<double *>(result);
        switch (d->type) {
        case QVariant::String:
            *f = v_cast<QString>(d)->toDouble(ok);
            break;
        case QVariant::ByteArray:
            *f = v_cast<QByteArray>(d)->toDouble(ok);
            break;
        case QVariant::Bool:
            *f = double(d->data.b);
            break;
        case QMetaType::Float:
            *f = double(d->data.f);
            break;
        case QVariant::LongLong:
        case QVariant::Int:
        case QMetaType::Char:
        case QMetaType::Short:
        case QMetaType::Long:
            *f = double(qMetaTypeNumber(d));
            break;
        case QVariant::UInt:
        case QVariant::ULongLong:
        case QMetaType::UChar:
        case QMetaType::UShort:
        case QMetaType::ULong:
            *f = double(qMetaTypeUNumber(d));
            break;
        default:
            *f = 0.0;
            return false;
        }
        break;
    }
    case QMetaType::Float: {
        float *f = static_cast<float *>(result);
        switch (d->type) {
        case QVariant::String:
            *f = v_cast<QString>(d)->toFloat(ok);
            break;
        case QVariant::ByteArray:
            *f = v_cast<QByteArray>(d)->toFloat(ok);
            break;
        case QVariant::Bool:
            *f = float(d->data.b);
            break;
        case QVariant::Double:
            *f = float(d->data.d);
            break;
        case QVariant::LongLong:
        case QVariant::Int:
        case QMetaType::Char:
        case QMetaType::Short:
        case QMetaType::Long:
            *f = float(qMetaTypeNumber(d));
            break;
        case QVariant::UInt:
        case QVariant::ULongLong:
        case QMetaType::UChar:
        case QMetaType::UShort:
        case QMetaType::ULong:
            *f = float(qMetaTypeUNumber(d));
            break;
        default:
            *f = 0.0f;
            return false;
        }
        break;
    }
    case QVariant::List:
        if (d->type == QVariant::StringList) {
            QVariantList *lst = static_cast<QVariantList *>(result);
            const QStringList *slist = v_cast<QStringList>(d);
            for (int i = 0; i < slist->size(); ++i)
                lst->append(QVariant(slist->at(i)));
        } else if (qstrcmp(QMetaType::typeName(d->type), "QList<QVariant>") == 0) {
            *static_cast<QVariantList *>(result) =
                *static_cast<QList<QVariant> *>(d->data.shared->ptr);
        } else {
            return false;
        }
        break;
    case QVariant::Map:
        if (qstrcmp(QMetaType::typeName(d->type), "QMap<QString, QVariant>") == 0) {
            *static_cast<QVariantMap *>(result) =
                *static_cast<QMap<QString, QVariant> *>(d->data.shared->ptr);
        } else {
            return false;
        }
        break;
    case QVariant::Hash:
        if (qstrcmp(QMetaType::typeName(d->type), "QHash<QString, QVariant>") == 0) {
            *static_cast<QVariantHash *>(result) =
                *static_cast<QHash<QString, QVariant> *>(d->data.shared->ptr);
        } else {
            return false;
        }
        break;
#ifndef QT_NO_GEOM_VARIANT
    case QVariant::Rect:
        if (d->type == QVariant::RectF)
            *static_cast<QRect *>(result) = (v_cast<QRectF>(d))->toRect();
        else
            return false;
        break;
    case QVariant::RectF:
        if (d->type == QVariant::Rect)
            *static_cast<QRectF *>(result) = *v_cast<QRect>(d);
        else
            return false;
        break;
    case QVariant::PointF:
        if (d->type == QVariant::Point)
            *static_cast<QPointF *>(result) = *v_cast<QPoint>(d);
        else
            return false;
        break;
    case QVariant::Point:
        if (d->type == QVariant::PointF)
            *static_cast<QPoint *>(result) = (v_cast<QPointF>(d))->toPoint();
        else
            return false;
        break;
    case QMetaType::Char:
    {
        *static_cast<qint8 *>(result) = qint8(qConvertToNumber(d, ok));
        return *ok;
    }
#endif
    default:
        return false;
    }
    return true;
}

#if !defined(QT_NO_DEBUG_STREAM) && !defined(Q_BROKEN_DEBUG_STREAM)
static void streamDebug(QDebug dbg, const QVariant &v)
{
    switch (v.userType()) {
    case QVariant::Int:
        dbg.nospace() << v.toInt();
        break;
    case QVariant::UInt:
        dbg.nospace() << v.toUInt();
        break;
    case QVariant::LongLong:
        dbg.nospace() << v.toLongLong();
        break;
    case QVariant::ULongLong:
        dbg.nospace() << v.toULongLong();
        break;
    case QMetaType::Float:
        dbg.nospace() << v.toFloat();
        break;
    case QMetaType::QObjectStar:
        dbg.nospace() << qvariant_cast<QObject *>(v);
        break;
    case QVariant::Double:
        dbg.nospace() << v.toDouble();
        break;
    case QVariant::Bool:
        dbg.nospace() << v.toBool();
        break;
    case QVariant::String:
        dbg.nospace() << v.toString();
        break;
    case QVariant::Char:
        dbg.nospace() << v.toChar();
        break;
    case QVariant::StringList:
        dbg.nospace() << v.toStringList();
        break;
    case QVariant::Map:
        dbg.nospace() << v.toMap();
        break;
    case QVariant::Hash:
        dbg.nospace() << v.toHash();
        break;
    case QVariant::List:
        dbg.nospace() << v.toList();
        break;
    case QVariant::Date:
        dbg.nospace() << v.toDate();
        break;
    case QVariant::Time:
        dbg.nospace() << v.toTime();
        break;
    case QVariant::DateTime:
        dbg.nospace() << v.toDateTime();
        break;
#ifndef QT_BOOTSTRAPPED
    case QVariant::EasingCurve:
        dbg.nospace() << v.toEasingCurve();
        break;
#endif
    case QVariant::ByteArray:
        dbg.nospace() << v.toByteArray();
        break;
#ifndef QT_BOOTSTRAPPED
    case QVariant::Url:
        dbg.nospace() << v.toUrl();
        break;
#endif
#ifndef QT_NO_GEOM_VARIANT
    case QVariant::Point:
        dbg.nospace() << v.toPoint();
        break;
    case QVariant::PointF:
        dbg.nospace() << v.toPointF();
        break;
    case QVariant::Rect:
        dbg.nospace() << v.toRect();
        break;
    case QVariant::Size:
        dbg.nospace() << v.toSize();
        break;
    case QVariant::SizeF:
        dbg.nospace() << v.toSizeF();
        break;
    case QVariant::Line:
        dbg.nospace() << v.toLine();
        break;
    case QVariant::LineF:
        dbg.nospace() << v.toLineF();
        break;
    case QVariant::RectF:
        dbg.nospace() << v.toRectF();
        break;
#endif
    case QVariant::BitArray:
        //dbg.nospace() << v.toBitArray();
        break;
    default:
        break;
    }
}
#endif

const QVariant::Handler qt_kernel_variant_handler = {
    construct,
    clear,
    isNull,
#ifndef QT_NO_DATASTREAM
    0,
    0,
#endif
    compare,
    convert,
    0,
#if !defined(QT_NO_DEBUG_STREAM) && !defined(Q_BROKEN_DEBUG_STREAM)
    streamDebug
#else
    0
#endif
};

Q_CORE_EXPORT const QVariant::Handler *qcoreVariantHandler()
{
    return &qt_kernel_variant_handler;
}


const QVariant::Handler *QVariant::handler = &qt_kernel_variant_handler;

/*!
    \class QVariant
    \brief The QVariant class acts like a union for the most common Qt data types.

    \ingroup objectmodel
    \ingroup shared


    Because C++ forbids unions from including types that have
    non-default constructors or destructors, most interesting Qt
    classes cannot be used in unions. Without QVariant, this would be
    a problem for QObject::property() and for database work, etc.

    A QVariant object holds a single value of a single type() at a
    time. (Some type()s are multi-valued, for example a string list.)
    You can find out what type, T, the variant holds, convert it to a
    different type using convert(), get its value using one of the
    toT() functions (e.g., toSize()) and check whether the type can
    be converted to a particular type using canConvert().

    The methods named toT() (e.g., toInt(), toString()) are const. If
    you ask for the stored type, they return a copy of the stored
    object. If you ask for a type that can be generated from the
    stored type, toT() copies and converts and leaves the object
    itself unchanged. If you ask for a type that cannot be generated
    from the stored type, the result depends on the type; see the
    function documentation for details.

    Here is some example code to demonstrate the use of QVariant:

    \snippet doc/src/snippets/code/src_corelib_kernel_qvariant.cpp 0

    You can even store QList<QVariant> and QMap<QString, QVariant>
    values in a variant, so you can easily construct arbitrarily
    complex data structures of arbitrary types. This is very powerful
    and versatile, but may prove less memory and speed efficient than
    storing specific types in standard data structures.

    QVariant also supports the notion of null values, where you can
    have a defined type with no value set. However, note that QVariant
    types can only be cast when they have had a value set.

    \snippet doc/src/snippets/code/src_corelib_kernel_qvariant.cpp 1

    QVariant can be extended to support other types than those
    mentioned in the \l Type enum. See the \l QMetaType documentation
    for details.

    \section1 A Note on GUI Types

    Because QVariant is part of the QtCore library, it cannot provide
    conversion functions to data types defined in QtGui, such as
    QColor, QImage, and QPixmap. In other words, there is no \c
    toColor() function. Instead, you can use the QVariant::value() or
    the qvariant_cast() template function. For example:

    \snippet doc/src/snippets/code/src_corelib_kernel_qvariant.cpp 2

    The inverse conversion (e.g., from QColor to QVariant) is
    automatic for all data types supported by QVariant, including
    GUI-related types:

    \snippet doc/src/snippets/code/src_corelib_kernel_qvariant.cpp 3

    \section1 Using canConvert() and convert() Consecutively

    When using canConvert() and convert() consecutively, it is possible for
    canConvert() to return true, but convert() to return false. This
    is typically because canConvert() only reports the general ability of
    QVariant to convert between types given suitable data; it is still
    possible to supply data which cannot actually be converted.

    For example, canConvert() would return true when called on a variant
    containing a string because, in principle, QVariant is able to convert
    strings of numbers to integers.
    However, if the string contains non-numeric characters, it cannot be
    converted to an integer, and any attempt to convert it will fail.
    Hence, it is important to have both functions return true for a
    successful conversion.

    \sa QMetaType
*/

/*!
    \enum QVariant::Type

    This enum type defines the types of variable that a QVariant can
    contain.

    \value Invalid  no type
    \value BitArray  a QBitArray
    \value Bitmap  a QBitmap
    \value Bool  a bool
    \value Brush  a QBrush
    \value ByteArray  a QByteArray
    \value Char  a QChar
    \value Color  a QColor
    \value Cursor  a QCursor
    \value Date  a QDate
    \value DateTime  a QDateTime
    \value Double  a double
    \value EasingCurve a QEasingCurve
    \value Font  a QFont
    \value Hash a QVariantHash
    \value Icon  a QIcon
    \value Image  a QImage
    \value Int  an int
    \value KeySequence  a QKeySequence
    \value Line  a QLine
    \value LineF  a QLineF
    \value List  a QVariantList
    \value Locale  a QLocale
    \value LongLong a \l qlonglong
    \value Map  a QVariantMap
    \value Matrix  a QMatrix
    \value Transform  a QTransform
    \value Matrix4x4  a QMatrix4x4
    \value Palette  a QPalette
    \value Pen  a QPen
    \value Pixmap  a QPixmap
    \value Point  a QPoint
    \value PointArray  a QPointArray
    \value PointF  a QPointF
    \value Polygon a QPolygon
    \value Quaternion  a QQuaternion
    \value Rect  a QRect
    \value RectF  a QRectF
    \value RegExp  a QRegExp
    \value Region  a QRegion
    \value Size  a QSize
    \value SizeF  a QSizeF
    \value SizePolicy  a QSizePolicy
    \value String  a QString
    \value StringList  a QStringList
    \value TextFormat  a QTextFormat
    \value TextLength  a QTextLength
    \value Time  a QTime
    \value UInt  a \l uint
    \value ULongLong a \l qulonglong
    \value Url  a QUrl
    \value Vector2D  a QVector2D
    \value Vector3D  a QVector3D
    \value Vector4D  a QVector4D

    \value UserType Base value for user-defined types.

    \omitvalue CString
    \omitvalue ColorGroup
    \omitvalue IconSet
    \omitvalue LastGuiType
    \omitvalue LastCoreType
    \omitvalue LastType
*/

/*!
    \fn QVariant::QVariant()

    Constructs an invalid variant.
*/


/*!
    \fn QVariant::QVariant(int typeOrUserType, const void *copy)

    Constructs variant of type \a typeOrUserType, and initializes with
    \a copy if \a copy is not 0.

    Note that you have to pass the address of the variable you want stored.

    Usually, you never have to use this constructor, use QVariant::fromValue()
    instead to construct variants from the pointer types represented by
    \c QMetaType::VoidStar, \c QMetaType::QObjectStar and
    \c QMetaType::QWidgetStar.

    \sa QVariant::fromValue(), Type
*/

/*!
    \fn QVariant::QVariant(Type type)

    Constructs a null variant of type \a type.
*/



/*!
    \fn QVariant::create(int type, const void *copy)

    \internal

    Constructs a variant private of type \a type, and initializes with \a copy if
    \a copy is not 0.
*/

void QVariant::create(int type, const void *copy)
{
    d.type = type;
    handler->construct(&d, copy);
}

/*!
    \fn QVariant::~QVariant()

    Destroys the QVariant and the contained object.

    Note that subclasses that reimplement clear() should reimplement
    the destructor to call clear(). This destructor calls clear(), but
    because it is the destructor, QVariant::clear() is called rather
    than a subclass's clear().
*/

QVariant::~QVariant()
{
    if ((d.is_shared && !d.data.shared->ref.deref()) || (!d.is_shared && d.type > Char && d.type < UserType))
        handler->clear(&d);
}

/*!
  \fn QVariant::QVariant(const QVariant &p)

    Constructs a copy of the variant, \a p, passed as the argument to
    this constructor.
*/

QVariant::QVariant(const QVariant &p)
    : d(p.d)
{
    if (d.is_shared) {
        d.data.shared->ref.ref();
    } else if (p.d.type > Char && p.d.type < QVariant::UserType) {
        handler->construct(&d, p.constData());
        d.is_null = p.d.is_null;
    }
}

#ifndef QT_NO_DATASTREAM
/*!
    Reads the variant from the data stream, \a s.
*/
QVariant::QVariant(QDataStream &s)
{
    d.is_null = true;
    s >> *this;
}
#endif //QT_NO_DATASTREAM

/*!
  \fn QVariant::QVariant(const QString &val)

    Constructs a new variant with a string value, \a val.
*/

/*!
  \fn QVariant::QVariant(const QLatin1String &val)

    Constructs a new variant with a string value, \a val.
*/

/*!
  \fn QVariant::QVariant(const char *val)

    Constructs a new variant with a string value of \a val.
    The variant creates a deep copy of \a val, using the encoding
    set by QTextCodec::setCodecForCStrings().

    Note that \a val is converted to a QString for storing in the
    variant and QVariant::type() will return QMetaType::QString for
    the variant.

    You can disable this operator by defining \c
    QT_NO_CAST_FROM_ASCII when you compile your applications.

    \sa QTextCodec::setCodecForCStrings()
*/

#ifndef QT_NO_CAST_FROM_ASCII
QVariant::QVariant(const char *val)
{
    QString s = QString::fromAscii(val);
    create(String, &s);
}
#endif

/*!
  \fn QVariant::QVariant(const QStringList &val)

    Constructs a new variant with a string list value, \a val.
*/

/*!
  \fn QVariant::QVariant(const QMap<QString, QVariant> &val)

    Constructs a new variant with a map of QVariants, \a val.
*/

/*!
  \fn QVariant::QVariant(const QHash<QString, QVariant> &val)

    Constructs a new variant with a hash of QVariants, \a val.
*/

/*!
  \fn QVariant::QVariant(const QDate &val)

    Constructs a new variant with a date value, \a val.
*/

/*!
  \fn QVariant::QVariant(const QTime &val)

    Constructs a new variant with a time value, \a val.
*/

/*!
  \fn QVariant::QVariant(const QDateTime &val)

    Constructs a new variant with a date/time value, \a val.
*/

/*!
    \since 4.7
  \fn QVariant::QVariant(const QEasingCurve &val)

    Constructs a new variant with an easing curve value, \a val.
*/

/*!
  \fn QVariant::QVariant(const QByteArray &val)

    Constructs a new variant with a bytearray value, \a val.
*/

/*!
  \fn QVariant::QVariant(const QBitArray &val)

    Constructs a new variant with a bitarray value, \a val.
*/

/*!
  \fn QVariant::QVariant(const QPoint &val)

  Constructs a new variant with a point value of \a val.
 */

/*!
  \fn QVariant::QVariant(const QPointF &val)

  Constructs a new variant with a point value of \a val.
 */

/*!
  \fn QVariant::QVariant(const QRectF &val)

  Constructs a new variant with a rect value of \a val.
 */

/*!
  \fn QVariant::QVariant(const QLineF &val)

  Constructs a new variant with a line value of \a val.
 */

/*!
  \fn QVariant::QVariant(const QLine &val)

  Constructs a new variant with a line value of \a val.
 */

/*!
  \fn QVariant::QVariant(const QRect &val)

  Constructs a new variant with a rect value of \a val.
 */

/*!
  \fn QVariant::QVariant(const QSize &val)

  Constructs a new variant with a size value of \a val.
 */

/*!
  \fn QVariant::QVariant(const QSizeF &val)

  Constructs a new variant with a size value of \a val.
 */

/*!
  \fn QVariant::QVariant(const QUrl &val)

  Constructs a new variant with a url value of \a val.
 */

/*!
  \fn QVariant::QVariant(int val)

    Constructs a new variant with an integer value, \a val.
*/

/*!
  \fn QVariant::QVariant(uint val)

    Constructs a new variant with an unsigned integer value, \a val.
*/

/*!
  \fn QVariant::QVariant(qlonglong val)

    Constructs a new variant with a long long integer value, \a val.
*/

/*!
  \fn QVariant::QVariant(qulonglong val)

    Constructs a new variant with an unsigned long long integer value, \a val.
*/


/*!
  \fn QVariant::QVariant(bool val)

    Constructs a new variant with a boolean value, \a val.
*/

/*!
  \fn QVariant::QVariant(double val)

    Constructs a new variant with a floating point value, \a val.
*/

/*!
  \fn QVariant::QVariant(float val)

    Constructs a new variant with a floating point value, \a val.
    \since 4.6
*/

/*!
    \fn QVariant::QVariant(const QList<QVariant> &val)

    Constructs a new variant with a list value, \a val.
*/

/*!
  \fn QVariant::QVariant(const QChar &c)

  Constructs a new variant with a char value, \a c.
*/

/*!
  \fn QVariant::QVariant(const QLocale &l)

  Constructs a new variant with a locale value, \a l.
*/

/*!
  \fn QVariant::QVariant(const QRegExp &regExp)

  Constructs a new variant with the regexp value \a regExp.
*/

/*! \since 4.2
  \fn QVariant::QVariant(Qt::GlobalColor color)

  Constructs a new variant of type QVariant::Color and initializes
  it with \a color.

  This is a convenience constructor that allows \c{QVariant(Qt::blue);}
  to create a valid QVariant storing a QColor.

  Note: This constructor will assert if the application does not link
  to the Qt GUI library.
 */

QVariant::QVariant(Type type)
{ create(type, 0); }
QVariant::QVariant(int typeOrUserType, const void *copy)
{ create(typeOrUserType, copy); d.is_null = false; }

/*! \internal
    flags is true if it is a pointer type
 */
QVariant::QVariant(int typeOrUserType, const void *copy, uint flags)
{
    if (flags) { //type is a pointer type
        d.type = typeOrUserType;
        d.data.ptr = *reinterpret_cast<void *const*>(copy);
        d.is_null = false;
    } else {
        create(typeOrUserType, copy);
        d.is_null = false;
    }
}

QVariant::QVariant(int val)
{ d.is_null = false; d.type = Int; d.data.i = val; }
QVariant::QVariant(uint val)
{ d.is_null = false; d.type = UInt; d.data.u = val; }
QVariant::QVariant(qlonglong val)
{ d.is_null = false; d.type = LongLong; d.data.ll = val; }
QVariant::QVariant(qulonglong val)
{ d.is_null = false; d.type = ULongLong; d.data.ull = val; }
QVariant::QVariant(bool val)
{ d.is_null = false; d.type = Bool; d.data.b = val; }
QVariant::QVariant(double val)
{ d.is_null = false; d.type = Double; d.data.d = val; }

QVariant::QVariant(const QByteArray &val)
{ d.is_null = false; d.type = ByteArray; v_construct<QByteArray>(&d, val); }
QVariant::QVariant(const QBitArray &val)
{ d.is_null = false; d.type = BitArray; v_construct<QBitArray>(&d, val);  }
QVariant::QVariant(const QString &val)
{ d.is_null = false; d.type = String; v_construct<QString>(&d, val);  }
QVariant::QVariant(const QChar &val)
{ d.is_null = false; d.type = Char; v_construct<QChar>(&d, val);  }
QVariant::QVariant(const QLatin1String &val)
{ QString str(val); d.is_null = false; d.type = String; v_construct<QString>(&d, str); }
QVariant::QVariant(const QStringList &val)
{ d.is_null = false; d.type = StringList; v_construct<QStringList>(&d, val); }

QVariant::QVariant(const QDate &val)
{ d.is_null = false; d.type = Date; v_construct<QDate>(&d, val); }
QVariant::QVariant(const QTime &val)
{ d.is_null = false; d.type = Time; v_construct<QTime>(&d, val); }
QVariant::QVariant(const QDateTime &val)
{ d.is_null = false; d.type = DateTime; v_construct<QDateTime>(&d, val); }
#ifndef QT_BOOTSTRAPPED
QVariant::QVariant(const QEasingCurve &val)
{ d.is_null = false; d.type = EasingCurve; v_construct<QEasingCurve>(&d, val); }
#endif
QVariant::QVariant(const QList<QVariant> &list)
{ d.is_null = false; d.type = List; v_construct<QVariantList>(&d, list); }
QVariant::QVariant(const QMap<QString, QVariant> &map)
{ d.is_null = false; d.type = Map; v_construct<QVariantMap>(&d, map); }
QVariant::QVariant(const QHash<QString, QVariant> &hash)
{ d.is_null = false; d.type = Hash; v_construct<QVariantHash>(&d, hash); }
#ifndef QT_NO_GEOM_VARIANT
QVariant::QVariant(const QPoint &pt) { d.is_null = false; d.type = Point; v_construct<QPoint>(&d, pt); }
QVariant::QVariant(const QPointF &pt) { d.is_null = false; d.type = PointF; v_construct<QPointF>(&d, pt); }
QVariant::QVariant(const QRectF &r) { d.is_null = false; d.type = RectF; v_construct<QRectF>(&d, r); }
QVariant::QVariant(const QLineF &l) { d.is_null = false; d.type = LineF; v_construct<QLineF>(&d, l); }
QVariant::QVariant(const QLine &l) { d.is_null = false; d.type = Line; v_construct<QLine>(&d, l); }
QVariant::QVariant(const QRect &r) { d.is_null = false; d.type = Rect; v_construct<QRect>(&d, r); }
QVariant::QVariant(const QSize &s) { d.is_null = false; d.type = Size; v_construct<QSize>(&d, s); }
QVariant::QVariant(const QSizeF &s) { d.is_null = false; d.type = SizeF; v_construct<QSizeF>(&d, s); }
#endif
#ifndef QT_BOOTSTRAPPED
QVariant::QVariant(const QUrl &u) { d.is_null = false; d.type = Url; v_construct<QUrl>(&d, u); }
#endif
QVariant::QVariant(const QLocale &l) { d.is_null = false; d.type = Locale; v_construct<QLocale>(&d, l); }
#ifndef QT_NO_REGEXP
QVariant::QVariant(const QRegExp &regExp) { d.is_null = false; d.type = RegExp; v_construct<QRegExp>(&d, regExp); }
#endif
QVariant::QVariant(Qt::GlobalColor color) { create(62, &color); }

/*!
    Returns the storage type of the value stored in the variant.
    Although this function is declared as returning QVariant::Type,
    the return value should be interpreted as QMetaType::Type. In
    particular, QVariant::UserType is returned here only if the value
    is equal or greater than QMetaType::User.

    Note that return values in the ranges QVariant::Char through
    QVariant::RegExp and QVariant::Font through QVariant::Transform
    correspond to the values in the ranges QMetaType::QChar through
    QMetaType::QRegExp and QMetaType::QFont through QMetaType::QQuaternion.

    Pay particular attention when working with char and QChar
    variants.  Note that there is no QVariant constructor specifically
    for type char, but there is one for QChar. For a variant of type
    QChar, this function returns QVariant::Char, which is the same as
    QMetaType::QChar, but for a variant of type \c char, this function
    returns QMetaType::Char, which is \e not the same as
    QVariant::Char.

    Also note that the types \c void*, \c long, \c short, \c unsigned
    \c long, \c unsigned \c short, \c unsigned \c char, \c float, \c
    QObject*, and \c QWidget* are represented in QMetaType::Type but
    not in QVariant::Type, and they can be returned by this function.
    However, they are considered to be user defined types when tested
    against QVariant::Type.

    To test whether an instance of QVariant contains a data type that
    is compatible with the data type you are interested in, use
    canConvert().
*/

QVariant::Type QVariant::type() const
{
    return d.type >= QMetaType::User ? UserType : static_cast<Type>(d.type);
}

/*!
    Returns the storage type of the value stored in the variant. For
    non-user types, this is the same as type().

    \sa type()
*/

int QVariant::userType() const
{
    return d.type;
}

/*!
    Assigns the value of the variant \a variant to this variant.
*/
QVariant& QVariant::operator=(const QVariant &variant)
{
    if (this == &variant)
        return *this;

    clear();
    if (variant.d.is_shared) {
        variant.d.data.shared->ref.ref();
        d = variant.d;
    } else if (variant.d.type > Char && variant.d.type < UserType) {
        d.type = variant.d.type;
        handler->construct(&d, variant.constData());
        d.is_null = variant.d.is_null;
    } else {
        d = variant.d;
    }

    return *this;
}

/*!
    \fn void QVariant::swap(QVariant &other)
    \since 4.8

    Swaps variant \a other with this variant. This operation is very
    fast and never fails.
*/

/*!
    \fn void QVariant::detach()

    \internal
*/

void QVariant::detach()
{
    if (!d.is_shared || d.data.shared->ref == 1)
        return;

    Private dd;
    dd.type = d.type;
    handler->construct(&dd, constData());
    if (!d.data.shared->ref.deref())
        handler->clear(&d);
    d.data.shared = dd.data.shared;
}

/*!
    \fn bool QVariant::isDetached() const

    \internal
*/

// ### Qt 5: change typeName()(and froends= to return a QString. Suggestion from Harald.
/*!
    Returns the name of the type stored in the variant. The returned
    strings describe the C++ datatype used to store the data: for
    example, "QFont", "QString", or "QVariantList". An Invalid
    variant returns 0.
*/
const char *QVariant::typeName() const
{
    return typeToName(Type(d.type));
}

/*!
    Convert this variant to type Invalid and free up any resources
    used.
*/
void QVariant::clear()
{
    if ((d.is_shared && !d.data.shared->ref.deref()) || (!d.is_shared && d.type < UserType && d.type > Char))
        handler->clear(&d);
    d.type = Invalid;
    d.is_null = true;
    d.is_shared = false;
}

/*!
    Converts the enum representation of the storage type, \a typ, to
    its string representation.

    Returns a null pointer if the type is QVariant::Invalid or doesn't exist.
*/
const char *QVariant::typeToName(Type typ)
{
    if (typ == Invalid)
        return 0;
    if (typ == UserType)
        return "UserType";

    return QMetaType::typeName(typ);
}


/*!
    Converts the string representation of the storage type given in \a
    name, to its enum representation.

    If the string representation cannot be converted to any enum
    representation, the variant is set to \c Invalid.
*/
QVariant::Type QVariant::nameToType(const char *name)
{
    if (!name || !*name)
        return Invalid;
    if (strcmp(name, "Q3CString") == 0)
        return ByteArray;
    if (strcmp(name, "Q_LLONG") == 0)
        return LongLong;
    if (strcmp(name, "Q_ULLONG") == 0)
        return ULongLong;
    if (strcmp(name, "QIconSet") == 0)
        return Icon;
    if (strcmp(name, "UserType") == 0)
        return UserType;

    int metaType = QMetaType::type(name);
    return metaType <= int(LastGuiType) ? QVariant::Type(metaType) : UserType;
}

#ifndef QT_NO_DATASTREAM
enum { MapFromThreeCount = 36 };
static const ushort map_from_three[MapFromThreeCount] =
{
    QVariant::Invalid,
    QVariant::Map,
    QVariant::List,
    QVariant::String,
    QVariant::StringList,
    QVariant::Font,
    QVariant::Pixmap,
    QVariant::Brush,
    QVariant::Rect,
    QVariant::Size,
    QVariant::Color,
    QVariant::Palette,
    63, // ColorGroup
    QVariant::Icon,
    QVariant::Point,
    QVariant::Image,
    QVariant::Int,
    QVariant::UInt,
    QVariant::Bool,
    QVariant::Double,
    QVariant::ByteArray,
    QVariant::Polygon,
    QVariant::Region,
    QVariant::Bitmap,
    QVariant::Cursor,
    QVariant::SizePolicy,
    QVariant::Date,
    QVariant::Time,
    QVariant::DateTime,
    QVariant::ByteArray,
    QVariant::BitArray,
    QVariant::KeySequence,
    QVariant::Pen,
    QVariant::LongLong,
    QVariant::ULongLong,
    QVariant::EasingCurve
};

/*!
    Internal function for loading a variant from stream \a s. Use the
    stream operators instead.

    \internal
*/
void QVariant::load(QDataStream &s)
{
    clear();

    quint32 u;
    s >> u;
    if (s.version() < QDataStream::Qt_4_0) {
        if (u >= MapFromThreeCount)
            return;
        u = map_from_three[u];
    }
    qint8 is_null = false;
    if (s.version() >= QDataStream::Qt_4_2)
        s >> is_null;
    if (u == QVariant::UserType) {
        QByteArray name;
        s >> name;
        u = QMetaType::type(name);
        if (!u) {
            s.setStatus(QDataStream::ReadCorruptData);
            return;
        }
    }
    create(static_cast<int>(u), 0);
    d.is_null = is_null;

    if (!isValid()) {
        // Since we wrote something, we should read something
        QString x;
        s >> x;
        d.is_null = true;
        return;
    }

    // const cast is safe since we operate on a newly constructed variant
    if (!QMetaType::load(s, d.type, const_cast<void *>(constData()))) {
        s.setStatus(QDataStream::ReadCorruptData);
        qWarning("QVariant::load: unable to load type %d.", d.type);
    }
}

/*!
    Internal function for saving a variant to the stream \a s. Use the
    stream operators instead.

    \internal
*/
void QVariant::save(QDataStream &s) const
{
    quint32 tp = type();
    if (s.version() < QDataStream::Qt_4_0) {
        int i;
        for (i = MapFromThreeCount - 1; i >= 0; i--) {
            if (map_from_three[i] == tp) {
                tp = i;
                break;
            }
        }
        if (i == -1) {
            s << QVariant();
            return;
        }
    }
    s << tp;
    if (s.version() >= QDataStream::Qt_4_2)
        s << qint8(d.is_null);
    if (tp == QVariant::UserType) {
        s << QMetaType::typeName(userType());
    }

    if (!isValid()) {
        s << QString();
        return;
    }

    if (!QMetaType::save(s, d.type, constData())) {
        Q_ASSERT_X(false, "QVariant::save", "Invalid type to save");
        qWarning("QVariant::save: unable to save type %d.", d.type);
    }
}

/*!
    \since 4.4

    Reads a variant \a p from the stream \a s.

    \sa \link datastreamformat.html Format of the QDataStream
    operators \endlink
*/
QDataStream& operator>>(QDataStream &s, QVariant &p)
{
    p.load(s);
    return s;
}

/*!
    Writes a variant \a p to the stream \a s.

    \sa \link datastreamformat.html Format of the QDataStream
    operators \endlink
*/
QDataStream& operator<<(QDataStream &s, const QVariant &p)
{
    p.save(s);
    return s;
}

/*!
    Reads a variant type \a p in enum representation from the stream \a s.
*/
QDataStream& operator>>(QDataStream &s, QVariant::Type &p)
{
    quint32 u;
    s >> u;
    p = (QVariant::Type)u;

    return s;
}

/*!
    Writes a variant type \a p to the stream \a s.
*/
QDataStream& operator<<(QDataStream &s, const QVariant::Type p)
{
    s << static_cast<quint32>(p);

    return s;
}

#endif //QT_NO_DATASTREAM

/*!
    \fn bool QVariant::isValid() const

    Returns true if the storage type of this variant is not
    QVariant::Invalid; otherwise returns false.
*/

template <typename T>
inline T qVariantToHelper(const QVariant::Private &d, QVariant::Type t,
                          const QVariant::Handler *handler, T * = 0)
{
    if (d.type == t)
        return *v_cast<T>(&d);

    T ret;
    handler->convert(&d, t, &ret, 0);
    return ret;
}

/*!
    \fn QStringList QVariant::toStringList() const

    Returns the variant as a QStringList if the variant has type()
    StringList, \l String, or \l List of a type that can be converted
    to QString; otherwise returns an empty list.

    \sa canConvert(), convert()
*/
QStringList QVariant::toStringList() const
{
    return qVariantToHelper<QStringList>(d, StringList, handler);
}

/*!
    Returns the variant as a QString if the variant has type() \l
    String, \l Bool, \l ByteArray, \l Char, \l Date, \l DateTime, \l
    Double, \l Int, \l LongLong, \l StringList, \l Time, \l UInt, or
    \l ULongLong; otherwise returns an empty string.

    \sa canConvert(), convert()
*/
QString QVariant::toString() const
{
    return qVariantToHelper<QString>(d, String, handler);
}

/*!
    Returns the variant as a QMap<QString, QVariant> if the variant
    has type() \l Map; otherwise returns an empty map.

    \sa canConvert(), convert()
*/
QVariantMap QVariant::toMap() const
{
    return qVariantToHelper<QVariantMap>(d, Map, handler);
}

/*!
    Returns the variant as a QHash<QString, QVariant> if the variant
    has type() \l Hash; otherwise returns an empty map.

    \sa canConvert(), convert()
*/
QVariantHash QVariant::toHash() const
{
    return qVariantToHelper<QVariantHash>(d, Hash, handler);
}

/*!
    \fn QDate QVariant::toDate() const

    Returns the variant as a QDate if the variant has type() \l Date,
    \l DateTime, or \l String; otherwise returns an invalid date.

    If the type() is \l String, an invalid date will be returned if the
    string cannot be parsed as a Qt::ISODate format date.

    \sa canConvert(), convert()
*/
QDate QVariant::toDate() const
{
    return qVariantToHelper<QDate>(d, Date, handler);
}

/*!
    \fn QTime QVariant::toTime() const

    Returns the variant as a QTime if the variant has type() \l Time,
    \l DateTime, or \l String; otherwise returns an invalid time.

    If the type() is \l String, an invalid time will be returned if
    the string cannot be parsed as a Qt::ISODate format time.

    \sa canConvert(), convert()
*/
QTime QVariant::toTime() const
{
    return qVariantToHelper<QTime>(d, Time, handler);
}

/*!
    \fn QDateTime QVariant::toDateTime() const

    Returns the variant as a QDateTime if the variant has type() \l
    DateTime, \l Date, or \l String; otherwise returns an invalid
    date/time.

    If the type() is \l String, an invalid date/time will be returned
    if the string cannot be parsed as a Qt::ISODate format date/time.

    \sa canConvert(), convert()
*/
QDateTime QVariant::toDateTime() const
{
    return qVariantToHelper<QDateTime>(d, DateTime, handler);
}

/*!
    \since 4.7
    \fn QEasingCurve QVariant::toEasingCurve() const

    Returns the variant as a QEasingCurve if the variant has type() \l
    EasingCurve; otherwise returns a default easing curve.

    \sa canConvert(), convert()
*/
#ifndef QT_BOOTSTRAPPED
QEasingCurve QVariant::toEasingCurve() const
{
    return qVariantToHelper<QEasingCurve>(d, EasingCurve, handler);
}
#endif

/*!
    \fn QByteArray QVariant::toByteArray() const

    Returns the variant as a QByteArray if the variant has type() \l
    ByteArray or \l String (converted using QString::fromAscii());
    otherwise returns an empty byte array.

    \sa canConvert(), convert()
*/
QByteArray QVariant::toByteArray() const
{
    return qVariantToHelper<QByteArray>(d, ByteArray, handler);
}

#ifndef QT_NO_GEOM_VARIANT
/*!
    \fn QPoint QVariant::toPoint() const

    Returns the variant as a QPoint if the variant has type()
    \l Point or \l PointF; otherwise returns a null QPoint.

    \sa canConvert(), convert()
*/
QPoint QVariant::toPoint() const
{
    return qVariantToHelper<QPoint>(d, Point, handler);
}

/*!
    \fn QRect QVariant::toRect() const

    Returns the variant as a QRect if the variant has type() \l Rect;
    otherwise returns an invalid QRect.

    \sa canConvert(), convert()
*/
QRect QVariant::toRect() const
{
    return qVariantToHelper<QRect>(d, Rect, handler);
}

/*!
    \fn QSize QVariant::toSize() const

    Returns the variant as a QSize if the variant has type() \l Size;
    otherwise returns an invalid QSize.

    \sa canConvert(), convert()
*/
QSize QVariant::toSize() const
{
    return qVariantToHelper<QSize>(d, Size, handler);
}

/*!
    \fn QSizeF QVariant::toSizeF() const

    Returns the variant as a QSizeF if the variant has type() \l
    SizeF; otherwise returns an invalid QSizeF.

    \sa canConvert(), convert()
*/
QSizeF QVariant::toSizeF() const
{
    return qVariantToHelper<QSizeF>(d, SizeF, handler);
}

/*!
    \fn QRectF QVariant::toRectF() const

    Returns the variant as a QRectF if the variant has type() \l Rect
    or \l RectF; otherwise returns an invalid QRectF.

    \sa canConvert(), convert()
*/
QRectF QVariant::toRectF() const
{
    return qVariantToHelper<QRectF>(d, RectF, handler);
}

/*!
    \fn QLineF QVariant::toLineF() const

    Returns the variant as a QLineF if the variant has type() \l
    LineF; otherwise returns an invalid QLineF.

    \sa canConvert(), convert()
*/
QLineF QVariant::toLineF() const
{
    return qVariantToHelper<QLineF>(d, LineF, handler);
}

/*!
    \fn QLine QVariant::toLine() const

    Returns the variant as a QLine if the variant has type() \l Line;
    otherwise returns an invalid QLine.

    \sa canConvert(), convert()
*/
QLine QVariant::toLine() const
{
    return qVariantToHelper<QLine>(d, Line, handler);
}

/*!
    \fn QPointF QVariant::toPointF() const

    Returns the variant as a QPointF if the variant has type() \l
    Point or \l PointF; otherwise returns a null QPointF.

    \sa canConvert(), convert()
*/
QPointF QVariant::toPointF() const
{
    return qVariantToHelper<QPointF>(d, PointF, handler);
}

#endif // QT_NO_GEOM_VARIANT

#ifndef QT_BOOTSTRAPPED
/*!
    \fn QUrl QVariant::toUrl() const

    Returns the variant as a QUrl if the variant has type()
    \l Url; otherwise returns an invalid QUrl.

    \sa canConvert(), convert()
*/
QUrl QVariant::toUrl() const
{
    return qVariantToHelper<QUrl>(d, Url, handler);
}
#endif

/*!
    \fn QLocale QVariant::toLocale() const

    Returns the variant as a QLocale if the variant has type()
    \l Locale; otherwise returns an invalid QLocale.

    \sa canConvert(), convert()
*/
QLocale QVariant::toLocale() const
{
    return qVariantToHelper<QLocale>(d, Locale, handler);
}

/*!
    \fn QRegExp QVariant::toRegExp() const
    \since 4.1

    Returns the variant as a QRegExp if the variant has type() \l
    RegExp; otherwise returns an empty QRegExp.

    \sa canConvert(), convert()
*/
#ifndef QT_NO_REGEXP
QRegExp QVariant::toRegExp() const
{
    return qVariantToHelper<QRegExp>(d, RegExp, handler);
}
#endif

/*!
    \fn QChar QVariant::toChar() const

    Returns the variant as a QChar if the variant has type() \l Char,
    \l Int, or \l UInt; otherwise returns an invalid QChar.

    \sa canConvert(), convert()
*/
QChar QVariant::toChar() const
{
    return qVariantToHelper<QChar>(d, Char, handler);
}

/*!
    Returns the variant as a QBitArray if the variant has type()
    \l BitArray; otherwise returns an empty bit array.

    \sa canConvert(), convert()
*/
QBitArray QVariant::toBitArray() const
{
    return qVariantToHelper<QBitArray>(d, BitArray, handler);
}

template <typename T>
inline T qNumVariantToHelper(const QVariant::Private &d,
                             const QVariant::Handler *handler, bool *ok, const T& val)
{
    uint t = qMetaTypeId<T>();
    if (ok)
        *ok = true;
    if (d.type == t)
        return val;

    T ret;
    if (!handler->convert(&d, QVariant::Type(t), &ret, ok) && ok)
        *ok = false;
    return ret;
}

/*!
    Returns the variant as an int if the variant has type() \l Int,
    \l Bool, \l ByteArray, \l Char, \l Double, \l LongLong, \l
    String, \l UInt, or \l ULongLong; otherwise returns 0.

    If \a ok is non-null: \c{*}\a{ok} is set to true if the value could be
    converted to an int; otherwise \c{*}\a{ok} is set to false.

    \bold{Warning:} If the value is convertible to a \l LongLong but is too
    large to be represented in an int, the resulting arithmetic overflow will
    not be reflected in \a ok. A simple workaround is to use QString::toInt().
    Fixing this bug has been postponed to Qt 5 in order to avoid breaking existing code.

    \sa canConvert(), convert()
*/
int QVariant::toInt(bool *ok) const
{
    return qNumVariantToHelper<int>(d, handler, ok, d.data.i);
}

/*!
    Returns the variant as an unsigned int if the variant has type()
    \l UInt,  \l Bool, \l ByteArray, \l Char, \l Double, \l Int, \l
    LongLong, \l String, or \l ULongLong; otherwise returns 0.

    If \a ok is non-null: \c{*}\a{ok} is set to true if the value could be
    converted to an unsigned int; otherwise \c{*}\a{ok} is set to false.

    \bold{Warning:} If the value is convertible to a \l ULongLong but is too
    large to be represented in an unsigned int, the resulting arithmetic overflow will
    not be reflected in \a ok. A simple workaround is to use QString::toUInt().
    Fixing this bug has been postponed to Qt 5 in order to avoid breaking existing code.

    \sa canConvert(), convert()
*/
uint QVariant::toUInt(bool *ok) const
{
    return qNumVariantToHelper<uint>(d, handler, ok, d.data.u);
}

/*!
    Returns the variant as a long long int if the variant has type()
    \l LongLong, \l Bool, \l ByteArray, \l Char, \l Double, \l Int,
    \l String, \l UInt, or \l ULongLong; otherwise returns 0.

    If \a ok is non-null: \c{*}\c{ok} is set to true if the value could be
    converted to an int; otherwise \c{*}\c{ok} is set to false.

    \sa canConvert(), convert()
*/
qlonglong QVariant::toLongLong(bool *ok) const
{
    return qNumVariantToHelper<qlonglong>(d, handler, ok, d.data.ll);
}

/*!
    Returns the variant as as an unsigned long long int if the
    variant has type() \l ULongLong, \l Bool, \l ByteArray, \l Char,
    \l Double, \l Int, \l LongLong, \l String, or \l UInt; otherwise
    returns 0.

    If \a ok is non-null: \c{*}\a{ok} is set to true if the value could be
    converted to an int; otherwise \c{*}\a{ok} is set to false.

    \sa canConvert(), convert()
*/
qulonglong QVariant::toULongLong(bool *ok) const
{
    return qNumVariantToHelper<qulonglong>(d, handler, ok, d.data.ull);
}

/*!
    Returns the variant as a bool if the variant has type() Bool.

    Returns true if the variant has type() \l Bool, \l Char, \l Double,
    \l Int, \l LongLong, \l UInt, or \l ULongLong and the value is
    non-zero, or if the variant has type \l String or \l ByteArray and
    its lower-case content is not empty, "0" or "false"; otherwise
    returns false.

    \sa canConvert(), convert()
*/
bool QVariant::toBool() const
{
    if (d.type == Bool)
        return d.data.b;

    bool res = false;
    handler->convert(&d, Bool, &res, 0);

    return res;
}

/*!
    Returns the variant as a double if the variant has type() \l
    Double, \l QMetaType::Float, \l Bool, \l ByteArray, \l Int, \l LongLong, \l String, \l
    UInt, or \l ULongLong; otherwise returns 0.0.

    If \a ok is non-null: \c{*}\a{ok} is set to true if the value could be
    converted to a double; otherwise \c{*}\a{ok} is set to false.

    \sa canConvert(), convert()
*/
double QVariant::toDouble(bool *ok) const
{
    return qNumVariantToHelper<double>(d, handler, ok, d.data.d);
}

/*!
    Returns the variant as a float if the variant has type() \l
    Double, \l QMetaType::Float, \l Bool, \l ByteArray, \l Int, \l LongLong, \l String, \l
    UInt, or \l ULongLong; otherwise returns 0.0.

    \since 4.6

    If \a ok is non-null: \c{*}\a{ok} is set to true if the value could be
    converted to a double; otherwise \c{*}\a{ok} is set to false.

    \sa canConvert(), convert()
*/
float QVariant::toFloat(bool *ok) const
{
    return qNumVariantToHelper<float>(d, handler, ok, d.data.f);
}

/*!
    Returns the variant as a qreal if the variant has type() \l
    Double, \l QMetaType::Float, \l Bool, \l ByteArray, \l Int, \l LongLong, \l String, \l
    UInt, or \l ULongLong; otherwise returns 0.0.

    \since 4.6

    If \a ok is non-null: \c{*}\a{ok} is set to true if the value could be
    converted to a double; otherwise \c{*}\a{ok} is set to false.

    \sa canConvert(), convert()
*/
qreal QVariant::toReal(bool *ok) const
{
    return qNumVariantToHelper<qreal>(d, handler, ok, d.data.real);
}

/*!
    Returns the variant as a QVariantList if the variant has type()
    \l List or \l StringList; otherwise returns an empty list.

    \sa canConvert(), convert()
*/
QVariantList QVariant::toList() const
{
    return qVariantToHelper<QVariantList>(d, List, handler);
}

/*! \fn QVariant::canCast(Type t) const
    Use canConvert() instead.
*/

/*! \fn QVariant::cast(Type t)
    Use convert() instead.
*/


static const quint32 qCanConvertMatrix[QVariant::LastCoreType + 1] =
{
/*Invalid*/     0,

/*Bool*/          1 << QVariant::Double     | 1 << QVariant::Int        | 1 << QVariant::UInt
                | 1 << QVariant::LongLong   | 1 << QVariant::ULongLong  | 1 << QVariant::ByteArray
                | 1 << QVariant::String     | 1 << QVariant::Char,

/*Int*/           1 << QVariant::UInt       | 1 << QVariant::String     | 1 << QVariant::Double
                | 1 << QVariant::Bool       | 1 << QVariant::LongLong   | 1 << QVariant::ULongLong
                | 1 << QVariant::Char       | 1 << QVariant::ByteArray,

/*UInt*/          1 << QVariant::Int        | 1 << QVariant::String     | 1 << QVariant::Double
                | 1 << QVariant::Bool       | 1 << QVariant::LongLong   | 1 << QVariant::ULongLong
                | 1 << QVariant::Char       | 1 << QVariant::ByteArray,

/*LLong*/         1 << QVariant::Int        | 1 << QVariant::String     | 1 << QVariant::Double
                | 1 << QVariant::Bool       | 1 << QVariant::UInt       | 1 << QVariant::ULongLong
                | 1 << QVariant::Char       | 1 << QVariant::ByteArray,

/*ULlong*/        1 << QVariant::Int        | 1 << QVariant::String     | 1 << QVariant::Double
                | 1 << QVariant::Bool       | 1 << QVariant::UInt       | 1 << QVariant::LongLong
                | 1 << QVariant::Char       | 1 << QVariant::ByteArray,

/*double*/        1 << QVariant::Int        | 1 << QVariant::String     | 1 << QVariant::ULongLong
                | 1 << QVariant::Bool       | 1 << QVariant::UInt       | 1 << QVariant::LongLong
                | 1 << QVariant::ByteArray,

/*QChar*/         1 << QVariant::Int        | 1 << QVariant::UInt       | 1 << QVariant::LongLong
                | 1 << QVariant::ULongLong,

/*QMap*/          0,

/*QList*/         1 << QVariant::StringList,

/*QString*/       1 << QVariant::StringList | 1 << QVariant::ByteArray  | 1 << QVariant::Int
                | 1 << QVariant::UInt       | 1 << QVariant::Bool       | 1 << QVariant::Double
                | 1 << QVariant::Date       | 1 << QVariant::Time       | 1 << QVariant::DateTime
                | 1 << QVariant::LongLong   | 1 << QVariant::ULongLong  | 1 << QVariant::Char
                | 1 << QVariant::Url,

/*QStringList*/   1 << QVariant::List       | 1 << QVariant::String,

/*QByteArray*/    1 << QVariant::String     | 1 << QVariant::Int        | 1 << QVariant::UInt | 1 << QVariant::Bool
                | 1 << QVariant::Double     | 1 << QVariant::LongLong   | 1 << QVariant::ULongLong,

/*QBitArray*/     0,

/*QDate*/         1 << QVariant::String     | 1 << QVariant::DateTime,

/*QTime*/         1 << QVariant::String     | 1 << QVariant::DateTime,

/*QDateTime*/     1 << QVariant::String     | 1 << QVariant::Date,

/*QUrl*/          1 << QVariant::String,

/*QLocale*/       0,

/*QRect*/         1 << QVariant::RectF,

/*QRectF*/        1 << QVariant::Rect,

/*QSize*/         1 << QVariant::SizeF,

/*QSizeF*/        1 << QVariant::Size,

/*QLine*/         1 << QVariant::LineF,

/*QLineF*/        1 << QVariant::Line,

/*QPoint*/        1 << QVariant::PointF,

/*QPointF*/       1 << QVariant::Point,

/*QRegExp*/       0,

/*QHash*/         0,

/*QEasingCurve*/  0
};

/*!
    Returns true if the variant's type can be cast to the requested
    type, \a t. Such casting is done automatically when calling the
    toInt(), toBool(), ... methods.

    The following casts are done automatically:

    \table
    \header \o Type \o Automatically Cast To
    \row \o \l Bool \o \l Char, \l Double, \l Int, \l LongLong, \l String, \l UInt, \l ULongLong
    \row \o \l ByteArray \o \l Double, \l Int, \l LongLong, \l String, \l UInt, \l ULongLong
    \row \o \l Char \o \l Bool, \l Int, \l UInt, \l LongLong, \l ULongLong
    \row \o \l Color \o \l String
    \row \o \l Date \o \l DateTime, \l String
    \row \o \l DateTime \o \l Date, \l String, \l Time
    \row \o \l Double \o \l Bool, \l Int, \l LongLong, \l String, \l UInt, \l ULongLong
    \row \o \l Font \o \l String
    \row \o \l Int \o \l Bool, \l Char, \l Double, \l LongLong, \l String, \l UInt, \l ULongLong
    \row \o \l KeySequence \o \l Int, \l String
    \row \o \l List \o \l StringList (if the list's items can be converted to strings)
    \row \o \l LongLong \o \l Bool, \l ByteArray, \l Char, \l Double, \l Int, \l String, \l UInt, \l ULongLong
    \row \o \l Point \o PointF
    \row \o \l Rect \o RectF
    \row \o \l String \o \l Bool, \l ByteArray, \l Char, \l Color, \l Date, \l DateTime, \l Double,
                         \l Font, \l Int, \l KeySequence, \l LongLong, \l StringList, \l Time, \l UInt,
                         \l ULongLong
    \row \o \l StringList \o \l List, \l String (if the list contains exactly one item)
    \row \o \l Time \o \l String
    \row \o \l UInt \o \l Bool, \l Char, \l Double, \l Int, \l LongLong, \l String, \l ULongLong
    \row \o \l ULongLong \o \l Bool, \l Char, \l Double, \l Int, \l LongLong, \l String, \l UInt
    \endtable

    \sa convert()
*/
bool QVariant::canConvert(Type t) const
{
    //we can treat floats as double
    //the reason for not doing it the "proper" way is that QMetaType::Float's value is 135,
    //which can't be handled by qCanConvertMatrix
    //In addition QVariant::Type doesn't have a Float value, so we're using QMetaType::Float
    const uint currentType = ((d.type == QMetaType::Float) ? QVariant::Double : d.type);
    if (uint(t) == uint(QMetaType::Float)) t = QVariant::Double;

    if (currentType == uint(t))
        return true;

    if (currentType > QVariant::LastCoreType || t > QVariant::LastCoreType) {
        switch (uint(t)) {
        case QVariant::Int:
            return currentType == QVariant::KeySequence
                   || currentType == QMetaType::ULong
                   || currentType == QMetaType::Long
                   || currentType == QMetaType::UShort
                   || currentType == QMetaType::UChar
                   || currentType == QMetaType::Char
                   || currentType == QMetaType::Short;
        case QVariant::Image:
            return currentType == QVariant::Pixmap || currentType == QVariant::Bitmap;
        case QVariant::Pixmap:
            return currentType == QVariant::Image || currentType == QVariant::Bitmap
                              || currentType == QVariant::Brush;
        case QVariant::Bitmap:
            return currentType == QVariant::Pixmap || currentType == QVariant::Image;
        case QVariant::ByteArray:
            return currentType == QVariant::Color;
        case QVariant::String:
            return currentType == QVariant::KeySequence || currentType == QVariant::Font
                              || currentType == QVariant::Color;
        case QVariant::KeySequence:
            return currentType == QVariant::String || currentType == QVariant::Int;
        case QVariant::Font:
            return currentType == QVariant::String;
        case QVariant::Color:
            return currentType == QVariant::String || currentType == QVariant::ByteArray
                              || currentType == QVariant::Brush;
        case QVariant::Brush:
            return currentType == QVariant::Color || currentType == QVariant::Pixmap;
        case QMetaType::Long:
        case QMetaType::Char:
        case QMetaType::UChar:
        case QMetaType::ULong:
        case QMetaType::Short:
        case QMetaType::UShort:
            return qCanConvertMatrix[QVariant::Int] & (1 << currentType) || currentType == QVariant::Int;
        default:
            return false;
        }
    }

    if(t == String && currentType == StringList)
        return v_cast<QStringList>(&d)->count() == 1;
    else
        return qCanConvertMatrix[t] & (1 << currentType);
}

/*!
    Casts the variant to the requested type, \a t. If the cast cannot be
    done, the variant is cleared. Returns true if the current type of
    the variant was successfully cast; otherwise returns false.

    \warning For historical reasons, converting a null QVariant results
    in a null value of the desired type (e.g., an empty string for
    QString) and a result of false.

    \sa canConvert(), clear()
*/

bool QVariant::convert(Type t)
{
    if (d.type == uint(t))
        return true;

    QVariant oldValue = *this;

    clear();
    if (!oldValue.canConvert(t))
        return false;

    create(t, 0);
    if (oldValue.isNull())
        return false;

    bool isOk = true;
    if (!handler->convert(&oldValue.d, t, data(), &isOk))
        isOk = false;
    d.is_null = !isOk;
    return isOk;
}

/*!
    \fn bool operator==(const QVariant &v1, const QVariant &v2)

    \relates QVariant

    Returns true if \a v1 and \a v2 are equal; otherwise returns false.

    \warning This function doesn't support custom types registered
    with qRegisterMetaType().
*/
/*!
    \fn bool operator!=(const QVariant &v1, const QVariant &v2)

    \relates QVariant

    Returns false if \a v1 and \a v2 are equal; otherwise returns true.

    \warning This function doesn't support custom types registered
    with qRegisterMetaType().
*/

/*! \fn bool QVariant::operator==(const QVariant &v) const

    Compares this QVariant with \a v and returns true if they are
    equal; otherwise returns false.

    In the case of custom types, their equalness operators are not called.
    Instead the values' addresses are compared.
*/

/*!
    \fn bool QVariant::operator!=(const QVariant &v) const

    Compares this QVariant with \a v and returns true if they are not
    equal; otherwise returns false.

    \warning This function doesn't support custom types registered
    with qRegisterMetaType().
*/

static bool qIsNumericType(uint tp)
{
    return (tp >= QVariant::Bool && tp <= QVariant::Double)
           || (tp >= QMetaType::Long && tp <= QMetaType::Float);
}

static bool qIsFloatingPoint(uint tp)
{
    return tp == QVariant::Double || tp == QMetaType::Float;
}

/*! \internal
 */
bool QVariant::cmp(const QVariant &v) const
{
    QVariant v2 = v;
    if (d.type != v2.d.type) {
        if (qIsNumericType(d.type) && qIsNumericType(v.d.type)) {
            if (qIsFloatingPoint(d.type) || qIsFloatingPoint(v.d.type))
                return qFuzzyCompare(toReal(), v.toReal());
            else
                return toLongLong() == v.toLongLong();
        }
        if (!v2.canConvert(Type(d.type)) || !v2.convert(Type(d.type)))
            return false;
    }
    return handler->compare(&d, &v2.d);
}

/*! \internal
 */

const void *QVariant::constData() const
{
    return d.is_shared ? d.data.shared->ptr : reinterpret_cast<const void *>(&d.data.ptr);
}

/*!
    \fn const void* QVariant::data() const

    \internal
*/

/*! \internal */
void* QVariant::data()
{
    detach();
    return const_cast<void *>(constData());
}


#ifdef QT3_SUPPORT
/*! \internal
 */
void *QVariant::castOrDetach(Type t)
{
    if (d.type != uint(t)) {
        if (!convert(t))
            create(t, 0);
    } else {
        detach();
    }
    return data();
}
#endif

/*!
  Returns true if this is a NULL variant, false otherwise.
*/
bool QVariant::isNull() const
{
    return handler->isNull(&d);
}

#ifndef QT_NO_DEBUG_STREAM
QDebug operator<<(QDebug dbg, const QVariant &v)
{
#ifndef Q_BROKEN_DEBUG_STREAM
    dbg.nospace() << "QVariant(" << v.typeName() << ", ";
    QVariant::handler->debugStream(dbg, v);
    dbg.nospace() << ')';
    return dbg.space();
#else
    qWarning("This compiler doesn't support streaming QVariant to QDebug");
    return dbg;
    Q_UNUSED(v);
#endif
}

QDebug operator<<(QDebug dbg, const QVariant::Type p)
{
#ifndef Q_BROKEN_DEBUG_STREAM
    dbg.nospace() << "QVariant::" << QVariant::typeToName(p);
    return dbg.space();
#else
    qWarning("This compiler doesn't support streaming QVariant::Type to QDebug");
    return dbg;
    Q_UNUSED(p);
#endif
}
#endif

/*!
    \fn int &QVariant::asInt()

    Use toInt() instead.
*/

/*!
    \fn uint &QVariant::asUInt()

    Use toUInt() instead.
*/

/*!
    \fn qlonglong &QVariant::asLongLong()

    Use toLongLong() instead.
*/

/*!
    \fn qulonglong &QVariant::asULongLong()

    Use toULongLong() instead.
*/

/*!
    \fn bool &QVariant::asBool()

    Use toBool() instead.
*/

/*!
    \fn double &QVariant::asDouble()

    Use toDouble() instead.
*/

/*!
    \fn QByteArray &QVariant::asByteArray()

    Use toByteArray() instead.
*/

/*!
    \fn QBitArray &QVariant::asBitArray()

    Use toBitArray() instead.
*/

/*!
    \fn QString &QVariant::asString()

    Use toString() instead.
*/

/*!
    \fn QStringList &QVariant::asStringList()

    Use toStringList() instead.
*/

/*!
    \fn QDate &QVariant::asDate()

    Use toDate() instead.
*/

/*!
    \fn QTime &QVariant::asTime()

    Use toTime() instead.
*/

/*!
    \fn QDateTime &QVariant::asDateTime()

    Use toDateTime() instead.
*/

/*!
    \fn QList<QVariant> &QVariant::asList()

    Use toList() instead.
*/

/*!
    \fn QMap<QString, QVariant> &QVariant::asMap()

    Use toMap() instead.
*/

/*!
    \fn QVariant::QVariant(bool b, int dummy)

    Use the QVariant(bool) constructor instead.

*/

/*!
    \fn const QByteArray QVariant::toCString() const

    Use toByteArray() instead.
*/

/*!
    \fn QByteArray &QVariant::asCString()

    Use toByteArray() instead.
*/

/*!
    \fn QPoint &QVariant::asPoint()

    Use toPoint() instead.
 */

/*!
    \fn QRect &QVariant::asRect()

    Use toRect() instead.
 */

/*!
    \fn QSize &QVariant::asSize()

    Use toSize() instead.
 */

/*! \fn void QVariant::setValue(const T &value)

    Stores a copy of \a value. If \c{T} is a type that QVariant
    doesn't support, QMetaType is used to store the value. A compile
    error will occur if QMetaType doesn't handle the type.

    Example:

    \snippet doc/src/snippets/code/src_corelib_kernel_qvariant.cpp 4

    \sa value(), fromValue(), canConvert()
 */

/*! \fn T QVariant::value() const

    Returns the stored value converted to the template type \c{T}.
    Call canConvert() to find out whether a type can be converted.
    If the value cannot be converted, \l{default-constructed value}
    will be returned.

    If the type \c{T} is supported by QVariant, this function behaves
    exactly as toString(), toInt() etc.

    Example:

    \snippet doc/src/snippets/code/src_corelib_kernel_qvariant.cpp 5

    \sa setValue(), fromValue(), canConvert()
*/

/*! \fn bool QVariant::canConvert() const

    Returns true if the variant can be converted to the template type \c{T},
    otherwise false.

    Example:

    \snippet doc/src/snippets/code/src_corelib_kernel_qvariant.cpp 6

    \sa convert()
*/

/*! \fn static QVariant QVariant::fromValue(const T &value)

    Returns a QVariant containing a copy of \a value. Behaves
    exactly like setValue() otherwise.

    Example:

    \snippet doc/src/snippets/code/src_corelib_kernel_qvariant.cpp 7

    \note If you are working with custom types, you should use
    the Q_DECLARE_METATYPE() macro to register your custom type.

    \sa setValue(), value()
*/

/*!
    \fn QVariant qVariantFromValue(const T &value)
    \relates QVariant
    \obsolete

    Returns a variant containing a copy of the given \a value
    with template type \c{T}.

    This function is equivalent to QVariant::fromValue(\a value).

    \note This function was provided as a workaround for MSVC 6
    which did not support member template functions. It is advised
    to use the other form in new code.

    For example, a QObject pointer can be stored in a variant with the
    following code:

    \snippet doc/src/snippets/code/src_corelib_kernel_qvariant.cpp 8

    \sa QVariant::fromValue()
*/

/*! \fn void qVariantSetValue(QVariant &variant, const T &value)
    \relates QVariant
    \obsolete

    Sets the contents of the given \a variant to a copy of the
    \a value with the specified template type \c{T}.

    This function is equivalent to QVariant::setValue(\a value).

    \note This function was provided as a workaround for MSVC 6
    which did not support member template functions. It is advised
    to use the other form in new code.

    \sa QVariant::setValue()
*/

/*!
    \fn T qvariant_cast(const QVariant &value)
    \relates QVariant

    Returns the given \a value converted to the template type \c{T}.

    This function is equivalent to QVariant::value().

    \sa QVariant::value()
*/

/*! \fn T qVariantValue(const QVariant &value)
    \relates QVariant
    \obsolete

    Returns the given \a value converted to the template type \c{T}.

    This function is equivalent to
    \l{QVariant::value()}{QVariant::value}<T>(\a value).

    \note This function was provided as a workaround for MSVC 6
    which did not support member template functions. It is advised
    to use the other form in new code.

    \sa QVariant::value(), qvariant_cast()
*/

/*! \fn bool qVariantCanConvert(const QVariant &value)
    \relates QVariant
    \obsolete

    Returns true if the given \a value can be converted to the
    template type specified; otherwise returns false.

    This function is equivalent to QVariant::canConvert(\a value).

    \note This function was provided as a workaround for MSVC 6
    which did not support member template functions. It is advised
    to use the other form in new code.

    \sa QVariant::canConvert()
*/

/*!
    \typedef QVariantList
    \relates QVariant

    Synonym for QList<QVariant>.
*/

/*!
    \typedef QVariantMap
    \relates QVariant

    Synonym for QMap<QString, QVariant>.
*/

/*!
    \typedef QVariantHash
    \relates QVariant
    \since 4.5

    Synonym for QHash<QString, QVariant>.
*/

/*!
    \typedef QVariant::DataPtr
    \internal
*/

/*!
    \fn DataPtr &QVariant::data_ptr()
    \internal
*/

QT_END_NAMESPACE
