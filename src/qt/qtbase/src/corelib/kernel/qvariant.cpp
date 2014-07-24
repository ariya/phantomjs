/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Copyright (C) 2013 Olivier Goffart <ogoffart@woboq.com>
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
#include "qregularexpression.h"
#include "qstring.h"
#include "qstringlist.h"
#include "qurl.h"
#include "qlocale.h"
#include "quuid.h"
#ifndef QT_BOOTSTRAPPED
#include "qabstractitemmodel.h"
#include "qjsonvalue.h"
#include "qjsonobject.h"
#include "qjsonarray.h"
#include "qjsondocument.h"
#endif
#include "private/qvariant_p.h"
#include "qmetatype_p.h"

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

namespace {
class HandlersManager
{
    static const QVariant::Handler *Handlers[QModulesPrivate::ModulesCount];
public:
    const QVariant::Handler *operator[] (const uint typeId) const
    {
        return Handlers[QModulesPrivate::moduleForType(typeId)];
    }

    void registerHandler(const QModulesPrivate::Names name, const QVariant::Handler *handler)
    {
        Handlers[name] = handler;
    }
};
}  // namespace

namespace {
struct CoreTypesFilter {
    template<typename T>
    struct Acceptor {
        static const bool IsAccepted = QModulesPrivate::QTypeModuleInfo<T>::IsCore && QtMetaTypePrivate::TypeDefinition<T>::IsAvailable;
    };
};
} // annonymous

namespace { // annonymous used to hide QVariant handlers

static void construct(QVariant::Private *x, const void *copy)
{
    QVariantConstructor<CoreTypesFilter> constructor(x, copy);
    QMetaTypeSwitcher::switcher<void>(constructor, x->type, 0);
}

static void clear(QVariant::Private *d)
{
    QVariantDestructor<CoreTypesFilter> cleaner(d);
    QMetaTypeSwitcher::switcher<void>(cleaner, d->type, 0);
}

static bool isNull(const QVariant::Private *d)
{
    QVariantIsNull<CoreTypesFilter> isNull(d);
    return QMetaTypeSwitcher::switcher<bool>(isNull, d->type, 0);
}

/*!
  \internal

  Compares \a a to \a b. The caller guarantees that \a a and \a b
  are of the same type.
 */
static bool compare(const QVariant::Private *a, const QVariant::Private *b)
{
    QVariantComparator<CoreTypesFilter> comparator(a, b);
    return QMetaTypeSwitcher::switcher<bool>(comparator, a->type, 0);
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
        return qlonglong(d->data.c);
    case QMetaType::SChar:
        return qlonglong(d->data.sc);
    case QMetaType::Short:
        return qlonglong(d->data.s);
    case QMetaType::Long:
        return qlonglong(d->data.l);
    case QMetaType::Float:
        return qRound64(d->data.f);
    case QVariant::Double:
        return qRound64(d->data.d);
#ifndef QT_BOOTSTRAPPED
    case QMetaType::QJsonValue:
        return v_cast<QJsonValue>(d)->toDouble();
#endif
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
        return d->data.uc;
    case QMetaType::UShort:
        return d->data.us;
    case QMetaType::ULong:
        return d->data.ul;
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
    case QMetaType::SChar:
    case QMetaType::Short:
    case QMetaType::Long:
    case QMetaType::Float:
    case QMetaType::LongLong:
    case QMetaType::QJsonValue:
        return qMetaTypeNumber(d);
    case QVariant::ULongLong:
    case QVariant::UInt:
    case QMetaType::UChar:
    case QMetaType::UShort:
    case QMetaType::ULong:

        return qlonglong(qMetaTypeUNumber(d));
    }

    if (QMetaType::typeFlags(d->type) & QMetaType::IsEnumeration) {
        switch (QMetaType::sizeOf(d->type)) {
        case 1:
            return d->is_shared ? *reinterpret_cast<signed char *>(d->data.shared->ptr) : d->data.sc;
        case 2:
            return d->is_shared ? *reinterpret_cast<qint16 *>(d->data.shared->ptr) : d->data.s;
        case 4:
            return d->is_shared ? *reinterpret_cast<qint32 *>(d->data.shared->ptr) : d->data.i;
        case 8:
            return d->is_shared ? *reinterpret_cast<qint64 *>(d->data.shared->ptr) : d->data.ll;
        }
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
    case QMetaType::SChar:
    case QMetaType::Short:
    case QMetaType::Long:
    case QMetaType::Float:
    case QMetaType::LongLong:
    case QMetaType::QJsonValue:
        return qulonglong(qMetaTypeNumber(d));
    case QVariant::ULongLong:
    case QVariant::UInt:
    case QMetaType::UChar:
    case QMetaType::UShort:
    case QMetaType::ULong:
        return qMetaTypeUNumber(d);
    }

    if (QMetaType::typeFlags(d->type) & QMetaType::IsEnumeration) {
        switch (QMetaType::sizeOf(d->type)) {
        case 1:
            return d->is_shared ? *reinterpret_cast<uchar *>(d->data.shared->ptr) : d->data.uc;
        case 2:
            return d->is_shared ? *reinterpret_cast<quint16 *>(d->data.shared->ptr) : d->data.us;
        case 4:
            return d->is_shared ? *reinterpret_cast<quint32 *>(d->data.shared->ptr) : d->data.u;
        case 8:
            return d->is_shared ? *reinterpret_cast<qint64 *>(d->data.shared->ptr) : d->data.ull;
        }
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
 Returns the internal data pointer from \a d.
 */

static const void *constData(const QVariant::Private &d)
{
    return d.is_shared ? d.data.shared->ptr : reinterpret_cast<const void *>(&d.data.c);
}

/*!
 \internal

 Converts \a d to type \a t, which is placed in \a result.
 */
static bool convert(const QVariant::Private *d, int t, void *result, bool *ok)
{
    Q_ASSERT(d->type != uint(t));
    Q_ASSERT(result);

    if (d->type >= QMetaType::User || t >= QMetaType::User) {
        const bool isOk = QMetaType::convert(constData(*d), d->type, result, t);
        if (ok)
            *ok = isOk;
        if (isOk)
            return true;
    }

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
        case QMetaType::SChar:
        case QMetaType::UChar:
            *str = QChar::fromLatin1(d->data.c);
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
            *str = QString::fromUtf8(v_cast<QByteArray>(d)->constData());
            break;
        case QVariant::StringList:
            if (v_cast<QStringList>(d)->count() == 1)
                *str = v_cast<QStringList>(d)->at(0);
            break;
#ifndef QT_BOOTSTRAPPED
        case QVariant::Url:
            *str = v_cast<QUrl>(d)->toString();
            break;
        case QMetaType::QJsonValue:
            *str = v_cast<QJsonValue>(d)->toString();
            break;
#endif
        case QVariant::Uuid:
            *str = v_cast<QUuid>(d)->toString();
            break;
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
        case QMetaType::SChar:
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
            *ba = v_cast<QString>(d)->toUtf8();
            break;
        case QVariant::Double:
            *ba = QByteArray::number(d->data.d, 'g', DBL_DIG);
            break;
        case QMetaType::Float:
            *ba = QByteArray::number(d->data.f, 'g', FLT_DIG);
            break;
        case QMetaType::Char:
        case QMetaType::SChar:
        case QMetaType::UChar:
            *ba = QByteArray(1, d->data.c);
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
    case QMetaType::SChar: {
        signed char s = qConvertToNumber(d, ok);
        *static_cast<signed char*>(result) = s;
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
        case QMetaType::SChar:
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
#ifndef QT_BOOTSTRAPPED
        case QMetaType::QJsonValue:
            *b = v_cast<QJsonValue>(d)->toBool();
            break;
#endif
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
        case QMetaType::SChar:
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
#ifndef QT_BOOTSTRAPPED
        case QMetaType::QJsonValue:
            *f = v_cast<QJsonValue>(d)->toDouble();
            break;
#endif
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
        case QMetaType::SChar:
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
#ifndef QT_BOOTSTRAPPED
        case QMetaType::QJsonValue:
            *f = v_cast<QJsonValue>(d)->toDouble();
            break;
#endif
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
    case QVariant::Uuid:
        switch (d->type) {
        case QVariant::String:
            *static_cast<QUuid *>(result) = QUuid(*v_cast<QString>(d));
            break;
        default:
            return false;
        }
        break;
    default:
        return false;
    }
    return true;
}

#if !defined(QT_NO_DEBUG_STREAM)
static void streamDebug(QDebug dbg, const QVariant &v)
{
    QVariant::Private *d = const_cast<QVariant::Private *>(&v.data_ptr());
    QVariantDebugStream<CoreTypesFilter> stream(dbg, d);
    QMetaTypeSwitcher::switcher<void>(stream, d->type, 0);
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
#if !defined(QT_NO_DEBUG_STREAM)
    streamDebug
#else
    0
#endif
};

static void dummyConstruct(QVariant::Private *, const void *) { Q_ASSERT_X(false, "QVariant", "Trying to construct an unknown type"); }
static void dummyClear(QVariant::Private *) { Q_ASSERT_X(false, "QVariant", "Trying to clear an unknown type"); }
static bool dummyIsNull(const QVariant::Private *d) { Q_ASSERT_X(false, "QVariant::isNull", "Trying to call isNull on an unknown type"); return d->is_null; }
static bool dummyCompare(const QVariant::Private *, const QVariant::Private *) { Q_ASSERT_X(false, "QVariant", "Trying to compare an unknown types"); return false; }
static bool dummyConvert(const QVariant::Private *, int, void *, bool *) { Q_ASSERT_X(false, "QVariant", "Trying to convert an unknown type"); return false; }
#if !defined(QT_NO_DEBUG_STREAM)
static void dummyStreamDebug(QDebug, const QVariant &) { Q_ASSERT_X(false, "QVariant", "Trying to convert an unknown type"); }
#endif
const QVariant::Handler qt_dummy_variant_handler = {
    dummyConstruct,
    dummyClear,
    dummyIsNull,
#ifndef QT_NO_DATASTREAM
    0,
    0,
#endif
    dummyCompare,
    dummyConvert,
    0,
#if !defined(QT_NO_DEBUG_STREAM)
    dummyStreamDebug
#else
    0
#endif
};

static void customConstruct(QVariant::Private *d, const void *copy)
{
    const QMetaType type(d->type);
    const uint size = type.sizeOf();
    if (!size) {
        qWarning("Trying to construct an instance of an invalid type, type id: %i", d->type);
        d->type = QVariant::Invalid;
        return;
    }

    // this logic should match with QVariantIntegrator::CanUseInternalSpace
    if (size <= sizeof(QVariant::Private::Data)
            && (type.flags() & (QMetaType::MovableType | QMetaType::IsEnumeration))) {
        type.construct(&d->data.ptr, copy);
        d->is_shared = false;
    } else {
        void *ptr = type.create(copy);
        d->is_shared = true;
        d->data.shared = new QVariant::PrivateShared(ptr);
    }
}

static void customClear(QVariant::Private *d)
{
    if (!d->is_shared) {
        QMetaType::destruct(d->type, &d->data.ptr);
    } else {
        QMetaType::destroy(d->type, d->data.shared->ptr);
        delete d->data.shared;
    }
}

static bool customIsNull(const QVariant::Private *d)
{
    return d->is_null;
}

static bool customCompare(const QVariant::Private *a, const QVariant::Private *b)
{
    const char *const typeName = QMetaType::typeName(a->type);
    if (Q_UNLIKELY(!typeName) && Q_LIKELY(!QMetaType::isRegistered(a->type)))
        qFatal("QVariant::compare: type %d unknown to QVariant.", a->type);

    const void *a_ptr = a->is_shared ? a->data.shared->ptr : &(a->data.ptr);
    const void *b_ptr = b->is_shared ? b->data.shared->ptr : &(b->data.ptr);

    uint typeNameLen = qstrlen(typeName);
    if (typeNameLen > 0 && typeName[typeNameLen - 1] == '*')
        return *static_cast<void *const *>(a_ptr) == *static_cast<void *const *>(b_ptr);

    if (a->is_null && b->is_null)
        return true;

    return !memcmp(a_ptr, b_ptr, QMetaType::sizeOf(a->type));
}

static bool customConvert(const QVariant::Private *d, int t, void *result, bool *ok)
{
    if (d->type >= QMetaType::User || t >= QMetaType::User) {
        if (QMetaType::convert(constData(*d), d->type, result, t)) {
            if (ok)
                *ok = true;
            return true;
        }
    }
    return convert(d, t, result, ok);
}

#if !defined(QT_NO_DEBUG_STREAM)
static void customStreamDebug(QDebug dbg, const QVariant &variant) {
#ifndef QT_BOOTSTRAPPED
    QMetaType::TypeFlags flags = QMetaType::typeFlags(variant.userType());
    if (flags & QMetaType::PointerToQObject)
        dbg.nospace() << variant.value<QObject*>();
#else
    Q_UNUSED(dbg);
    Q_UNUSED(variant);
#endif
}
#endif

const QVariant::Handler qt_custom_variant_handler = {
    customConstruct,
    customClear,
    customIsNull,
#ifndef QT_NO_DATASTREAM
    0,
    0,
#endif
    customCompare,
    customConvert,
    0,
#if !defined(QT_NO_DEBUG_STREAM)
    customStreamDebug
#else
    0
#endif
};

} // annonymous used to hide QVariant handlers

static HandlersManager handlerManager;
Q_STATIC_ASSERT_X(!QModulesPrivate::Core, "Initialization assumes that ModulesNames::Core is 0");
const QVariant::Handler *HandlersManager::Handlers[QModulesPrivate::ModulesCount]
                                        = { &qt_kernel_variant_handler, &qt_dummy_variant_handler,
                                            &qt_dummy_variant_handler, &qt_custom_variant_handler };

Q_CORE_EXPORT const QVariant::Handler *qcoreVariantHandler()
{
    return &qt_kernel_variant_handler;
}

Q_CORE_EXPORT void QVariantPrivate::registerHandler(const int /* Modules::Names */name, const QVariant::Handler *handler)
{
    handlerManager.registerHandler(static_cast<QModulesPrivate::Names>(name), handler);
}

/*!
    \class QVariant
    \inmodule QtCore
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

    \snippet code/src_corelib_kernel_qvariant.cpp 0

    You can even store QList<QVariant> and QMap<QString, QVariant>
    values in a variant, so you can easily construct arbitrarily
    complex data structures of arbitrary types. This is very powerful
    and versatile, but may prove less memory and speed efficient than
    storing specific types in standard data structures.

    QVariant also supports the notion of null values, where you can
    have a defined type with no value set. However, note that QVariant
    types can only be cast when they have had a value set.

    \snippet code/src_corelib_kernel_qvariant.cpp 1

    QVariant can be extended to support other types than those
    mentioned in the \l Type enum. See the \l QMetaType documentation
    for details.

    \section1 A Note on GUI Types

    Because QVariant is part of the Qt Core module, it cannot provide
    conversion functions to data types defined in Qt GUI, such as
    QColor, QImage, and QPixmap. In other words, there is no \c
    toColor() function. Instead, you can use the QVariant::value() or
    the qvariant_cast() template function. For example:

    \snippet code/src_corelib_kernel_qvariant.cpp 2

    The inverse conversion (e.g., from QColor to QVariant) is
    automatic for all data types supported by QVariant, including
    GUI-related types:

    \snippet code/src_corelib_kernel_qvariant.cpp 3

    \section1 Using canConvert() and convert() Consecutively

    When using canConvert() and convert() consecutively, it is possible for
    canConvert() to return true, but convert() to return false. This
    is typically because canConvert() only reports the general ability of
    QVariant to convert between types given suitable data; it is still
    possible to supply data which cannot actually be converted.

    For example, canConvert(Int) would return true when called on a variant
    containing a string because, in principle, QVariant is able to convert
    strings of numbers to integers.
    However, if the string contains non-numeric characters, it cannot be
    converted to an integer, and any attempt to convert it will fail.
    Hence, it is important to have both functions return true for a
    successful conversion.

    \sa QMetaType
*/

/*!
    \obsolete Use QMetaType::Type instead
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
    \value Uuid a QUuid
    \value ModelIndex a QModelIndex
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
    \value PointF  a QPointF
    \value Polygon a QPolygon
    \value PolygonF a QPolygonF
    \value Quaternion  a QQuaternion
    \value Rect  a QRect
    \value RectF  a QRectF
    \value RegExp  a QRegExp
    \value RegularExpression  a QRegularExpression
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

    \omitvalue LastGuiType
    \omitvalue LastCoreType
    \omitvalue LastType
*/

/*!
    \fn QVariant::QVariant(QVariant &&other)

    Move-constructs a QVariant instance, making it point at the same
    object that \a other was pointing to.

    \since 5.2
*/

/*!
    \fn QVariant &QVariant::operator=(QVariant &&other)

    Move-assigns \a other to this QVariant instance.

    \since 5.2
*/

/*!
    \fn QVariant::QVariant()

    Constructs an invalid variant.
*/


/*!
    \fn QVariant::QVariant(int typeId, const void *copy)

    Constructs variant of type \a typeId, and initializes with
    \a copy if \a copy is not 0.

    Note that you have to pass the address of the variable you want stored.

    Usually, you never have to use this constructor, use QVariant::fromValue()
    instead to construct variants from the pointer types represented by
    \c QMetaType::VoidStar, and \c QMetaType::QObjectStar.

    \sa QVariant::fromValue(), QMetaType::Type
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
    handlerManager[type]->construct(&d, copy);
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
    if ((d.is_shared && !d.data.shared->ref.deref()) || (!d.is_shared && d.type > Char))
        handlerManager[d.type]->clear(&d);
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
    } else if (p.d.type > Char) {
        handlerManager[d.type]->construct(&d, p.constData());
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
  \fn QVariant::QVariant(QLatin1String val)

    Constructs a new variant with a string value, \a val.
*/

/*!
  \fn QVariant::QVariant(const char *val)

    Constructs a new variant with a string value of \a val.
    The variant creates a deep copy of \a val into a QString assuming
    UTF-8 encoding on the input \a val.

    Note that \a val is converted to a QString for storing in the
    variant and QVariant::userType() will return QMetaType::QString for
    the variant.

    You can disable this operator by defining \c
    QT_NO_CAST_FROM_ASCII when you compile your applications.
*/

#ifndef QT_NO_CAST_FROM_ASCII
QVariant::QVariant(const char *val)
{
    QString s = QString::fromUtf8(val);
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
    \since 5.0
    \fn QVariant::QVariant(const QUuid &val)

    Constructs a new variant with an uuid value, \a val.
*/

/*!
    \since 5.0
    \fn QVariant::QVariant(const QModelIndex &val)

    Constructs a new variant with an modelIndex value, \a val.
*/

/*!
    \since 5.0
    \fn QVariant::QVariant(const QJsonValue &val)

    Constructs a new variant with a json value, \a val.
*/

/*!
    \since 5.0
    \fn QVariant::QVariant(const QJsonObject &val)

    Constructs a new variant with a json object value, \a val.
*/

/*!
    \since 5.0
    \fn QVariant::QVariant(const QJsonArray &val)

    Constructs a new variant with a json array value, \a val.
*/

/*!
    \since 5.0
    \fn QVariant::QVariant(const QJsonDocument &val)

    Constructs a new variant with a json document value, \a val.
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
  \fn QVariant::QVariant(QChar c)

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

/*!
  \fn QVariant::QVariant(const QRegularExpression &re)

  \since 5.0

  Constructs a new variant with the regular expression value \a re.
*/

QVariant::QVariant(Type type)
{ create(type, 0); }
QVariant::QVariant(int typeId, const void *copy)
{ create(typeId, copy); d.is_null = false; }

/*!
    \internal
    flags is true if it is a pointer type
 */
QVariant::QVariant(int typeId, const void *copy, uint flags)
{
    if (flags) { //type is a pointer type
        d.type = typeId;
        d.data.ptr = *reinterpret_cast<void *const*>(copy);
    } else {
        create(typeId, copy);
    }
    d.is_null = false;
}

QVariant::QVariant(int val)
    : d(Int)
{ d.data.i = val; }
QVariant::QVariant(uint val)
    : d(UInt)
{ d.data.u = val; }
QVariant::QVariant(qlonglong val)
    : d(LongLong)
{ d.data.ll = val; }
QVariant::QVariant(qulonglong val)
    : d(ULongLong)
{ d.data.ull = val; }
QVariant::QVariant(bool val)
    : d(Bool)
{ d.data.b = val; }
QVariant::QVariant(double val)
    : d(Double)
{ d.data.d = val; }
QVariant::QVariant(float val)
    : d(QMetaType::Float)
{ d.data.f = val; }

QVariant::QVariant(const QByteArray &val)
    : d(ByteArray)
{ v_construct<QByteArray>(&d, val); }
QVariant::QVariant(const QBitArray &val)
    : d(BitArray)
{ v_construct<QBitArray>(&d, val);  }
QVariant::QVariant(const QString &val)
    : d(String)
{ v_construct<QString>(&d, val);  }
QVariant::QVariant(QChar val)
    : d(Char)
{ v_construct<QChar>(&d, val);  }
QVariant::QVariant(QLatin1String val)
    : d(String)
{ v_construct<QString>(&d, val); }
QVariant::QVariant(const QStringList &val)
    : d(StringList)
{ v_construct<QStringList>(&d, val); }

QVariant::QVariant(const QDate &val)
    : d(Date)
{ v_construct<QDate>(&d, val); }
QVariant::QVariant(const QTime &val)
    : d(Time)
{ v_construct<QTime>(&d, val); }
QVariant::QVariant(const QDateTime &val)
    : d(DateTime)
{ v_construct<QDateTime>(&d, val); }
#ifndef QT_BOOTSTRAPPED
QVariant::QVariant(const QEasingCurve &val)
    : d(EasingCurve)
{ v_construct<QEasingCurve>(&d, val); }
#endif
QVariant::QVariant(const QList<QVariant> &list)
    : d(List)
{ v_construct<QVariantList>(&d, list); }
QVariant::QVariant(const QMap<QString, QVariant> &map)
    : d(Map)
{ v_construct<QVariantMap>(&d, map); }
QVariant::QVariant(const QHash<QString, QVariant> &hash)
    : d(Hash)
{ v_construct<QVariantHash>(&d, hash); }
#ifndef QT_NO_GEOM_VARIANT
QVariant::QVariant(const QPoint &pt)
    : d(Point)
{ v_construct<QPoint>(&d, pt); }
QVariant::QVariant(const QPointF &pt)
    : d(PointF)
{ v_construct<QPointF>(&d, pt); }
QVariant::QVariant(const QRectF &r)
    : d(RectF)
{ v_construct<QRectF>(&d, r); }
QVariant::QVariant(const QLineF &l)
    : d(LineF)
{ v_construct<QLineF>(&d, l); }
QVariant::QVariant(const QLine &l)
    : d(Line)
{ v_construct<QLine>(&d, l); }
QVariant::QVariant(const QRect &r)
    : d(Rect)
{ v_construct<QRect>(&d, r); }
QVariant::QVariant(const QSize &s)
    : d(Size)
{ v_construct<QSize>(&d, s); }
QVariant::QVariant(const QSizeF &s)
    : d(SizeF)
{ v_construct<QSizeF>(&d, s); }
#endif
#ifndef QT_BOOTSTRAPPED
QVariant::QVariant(const QUrl &u)
    : d(Url)
{ v_construct<QUrl>(&d, u); }
#endif
QVariant::QVariant(const QLocale &l)
    : d(Locale)
{ v_construct<QLocale>(&d, l); }
#ifndef QT_NO_REGEXP
QVariant::QVariant(const QRegExp &regExp)
    : d(RegExp)
{ v_construct<QRegExp>(&d, regExp); }
#endif // QT_NO_REGEXP
#ifndef QT_BOOTSTRAPPED
#ifndef QT_NO_REGULAREXPRESSION
QVariant::QVariant(const QRegularExpression &re)
    : d(RegularExpression)
{ v_construct<QRegularExpression>(&d, re); }
#endif
QVariant::QVariant(const QUuid &uuid)
    : d(Uuid)
{ v_construct<QUuid>(&d, uuid); }
QVariant::QVariant(const QModelIndex &modelIndex)
    : d(ModelIndex)
{ v_construct<QModelIndex>(&d, modelIndex); }
QVariant::QVariant(const QJsonValue &jsonValue)
    : d(QMetaType::QJsonValue)
{ v_construct<QJsonValue>(&d, jsonValue); }
QVariant::QVariant(const QJsonObject &jsonObject)
    : d(QMetaType::QJsonObject)
{ v_construct<QJsonObject>(&d, jsonObject); }
QVariant::QVariant(const QJsonArray &jsonArray)
    : d(QMetaType::QJsonArray)
{ v_construct<QJsonArray>(&d, jsonArray); }
QVariant::QVariant(const QJsonDocument &jsonDocument)
    : d(QMetaType::QJsonDocument)
{ v_construct<QJsonDocument>(&d, jsonDocument); }
#endif // QT_BOOTSTRAPPED

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
    } else if (variant.d.type > Char) {
        d.type = variant.d.type;
        handlerManager[d.type]->construct(&d, variant.constData());
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
    if (!d.is_shared || d.data.shared->ref.load() == 1)
        return;

    Private dd;
    dd.type = d.type;
    handlerManager[d.type]->construct(&dd, constData());
    if (!d.data.shared->ref.deref())
        handlerManager[d.type]->clear(&d);
    d.data.shared = dd.data.shared;
}

/*!
    \fn bool QVariant::isDetached() const

    \internal
*/

/*!
    Returns the name of the type stored in the variant. The returned
    strings describe the C++ datatype used to store the data: for
    example, "QFont", "QString", or "QVariantList". An Invalid
    variant returns 0.
*/
const char *QVariant::typeName() const
{
    return QMetaType::typeName(d.type);
}

/*!
    Convert this variant to type QMetaType::UnknownType and free up any resources
    used.
*/
void QVariant::clear()
{
    if ((d.is_shared && !d.data.shared->ref.deref()) || (!d.is_shared && d.type > Char))
        handlerManager[d.type]->clear(&d);
    d.type = Invalid;
    d.is_null = true;
    d.is_shared = false;
}

/*!
    Converts the int representation of the storage type, \a typeId, to
    its string representation.

    Returns a null pointer if the type is QMetaType::UnknownType or doesn't exist.
*/
const char *QVariant::typeToName(int typeId)
{
    return QMetaType::typeName(typeId);
}


/*!
    Converts the string representation of the storage type given in \a
    name, to its enum representation.

    If the string representation cannot be converted to any enum
    representation, the variant is set to \c Invalid.
*/
QVariant::Type QVariant::nameToType(const char *name)
{
    int metaType = QMetaType::type(name);
    return metaType <= int(UserType) ? QVariant::Type(metaType) : UserType;
}

#ifndef QT_NO_DATASTREAM
enum { MapFromThreeCount = 36 };
static const ushort mapIdFromQt3ToCurrent[MapFromThreeCount] =
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
    0, // ColorGroup
    QVariant::Icon,
    QVariant::Point,
    QVariant::Image,
    QVariant::Int,
    QVariant::UInt,
    QVariant::Bool,
    QVariant::Double,
    0, // Buggy ByteArray, QByteArray never had id == 20
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

    quint32 typeId;
    s >> typeId;
    if (s.version() < QDataStream::Qt_4_0) {
        if (typeId >= MapFromThreeCount)
            return;
        typeId = mapIdFromQt3ToCurrent[typeId];
    } else if (s.version() < QDataStream::Qt_5_0) {
        if (typeId == 127 /* QVariant::UserType */) {
            typeId = QMetaType::User;
        } else if (typeId >= 128 && typeId != QVariant::UserType) {
            // In Qt4 id == 128 was FirstExtCoreType. In Qt5 ExtCoreTypes set was merged to CoreTypes
            // by moving all ids down by 97.
            typeId -= 97;
        } else if (typeId == 75 /* QSizePolicy */) {
            typeId = QMetaType::QSizePolicy;
        } else if (typeId > 75 && typeId <= 86) {
            // and as a result these types received lower ids too
            // QKeySequence QPen QTextLength QTextFormat QMatrix QTransform QMatrix4x4 QVector2D QVector3D QVector4D QQuaternion
            typeId -=1;
        }
    }

    qint8 is_null = false;
    if (s.version() >= QDataStream::Qt_4_2)
        s >> is_null;
    if (typeId == QVariant::UserType) {
        QByteArray name;
        s >> name;
        typeId = QMetaType::type(name.constData());
        if (typeId == QMetaType::UnknownType) {
            s.setStatus(QDataStream::ReadCorruptData);
            return;
        }
    }
    create(typeId, 0);
    d.is_null = is_null;

    if (!isValid()) {
        if (s.version() < QDataStream::Qt_5_0) {
        // Since we wrote something, we should read something
            QString x;
            s >> x;
        }
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
    quint32 typeId = type();
    bool fakeUserType = false;
    if (s.version() < QDataStream::Qt_4_0) {
        int i;
        for (i = 0; i <= MapFromThreeCount - 1; ++i) {
            if (mapIdFromQt3ToCurrent[i] == typeId) {
                typeId = i;
                break;
            }
        }
        if (i >= MapFromThreeCount) {
            s << QVariant();
            return;
        }
    } else if (s.version() < QDataStream::Qt_5_0) {
        if (typeId == QMetaType::User) {
            typeId = 127; // QVariant::UserType had this value in Qt4
        } else if (typeId >= 128 - 97 && typeId <= LastCoreType) {
            // In Qt4 id == 128 was FirstExtCoreType. In Qt5 ExtCoreTypes set was merged to CoreTypes
            // by moving all ids down by 97.
            typeId += 97;
        } else if (typeId == QMetaType::QSizePolicy) {
            typeId = 75;
        } else if (typeId >= QMetaType::QKeySequence && typeId <= QMetaType::QQuaternion) {
            // and as a result these types received lower ids too
            typeId +=1;
        } else if (typeId == QMetaType::QPolygonF) {
            // This existed in Qt 4 only as a custom type
            typeId = 127;
            fakeUserType = true;
        }
    }
    s << typeId;
    if (s.version() >= QDataStream::Qt_4_2)
        s << qint8(d.is_null);
    if (d.type >= QVariant::UserType || fakeUserType) {
        s << QMetaType::typeName(userType());
    }

    if (!isValid()) {
        if (s.version() < QDataStream::Qt_5_0)
            s << QString();
        return;
    }

    if (!QMetaType::save(s, d.type, constData())) {
        qWarning("QVariant::save: unable to save type '%s' (type id: %d).\n", QMetaType::typeName(d.type), d.type);
        Q_ASSERT_X(false, "QVariant::save", "Invalid type to save");
    }
}

/*!
    \since 4.4

    Reads a variant \a p from the stream \a s.

    \sa{Serializing Qt Data Types}{Format of the QDataStream operators}
*/
QDataStream& operator>>(QDataStream &s, QVariant &p)
{
    p.load(s);
    return s;
}

/*!
    Writes a variant \a p to the stream \a s.

    \sa{Serializing Qt Data Types}{Format of the QDataStream operators}
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

    Returns \c true if the storage type of this variant is not
    QMetaType::UnknownType; otherwise returns \c false.
*/

template <typename T>
inline T qVariantToHelper(const QVariant::Private &d, const HandlersManager &handlerManager)
{
    const uint targetType = qMetaTypeId<T>();
    if (d.type == targetType)
        return *v_cast<T>(&d);

    T ret;
    if (d.type >= QMetaType::User || targetType >= QMetaType::User) {
        const void * const from = constData(d);
        if (QMetaType::convert(from, d.type, &ret, targetType))
            return ret;
    }

    handlerManager[d.type]->convert(&d, targetType, &ret, 0);
    return ret;
}

/*!
    \fn QStringList QVariant::toStringList() const

    Returns the variant as a QStringList if the variant has userType()
    \l QMetaType::QStringList, \l QMetaType::QString, or
    \l QMetaType::QVariantList of a type that can be converted to QString;
    otherwise returns an empty list.

    \sa canConvert(), convert()
*/
QStringList QVariant::toStringList() const
{
    return qVariantToHelper<QStringList>(d, handlerManager);
}

/*!
    Returns the variant as a QString if the variant has userType() \l
    QMetaType::QString, \l QMetaType::Bool, \l QMetaType::QByteArray,
    \l QMetaType::QChar, \l QMetaType::QDate, \l QMetaType::QDateTime,
    \l QMetaType::Double, \l QMetaType::Int, \l QMetaType::LongLong,
    \l QMetaType::QStringList, \l QMetaType::QTime, \l QMetaType::UInt, or
    \l QMetaType::ULongLong; otherwise returns an empty string.

    \sa canConvert(), convert()
*/
QString QVariant::toString() const
{
    return qVariantToHelper<QString>(d, handlerManager);
}

/*!
    Returns the variant as a QMap<QString, QVariant> if the variant
    has type() \l QMetaType::QVariantMap; otherwise returns an empty map.

    \sa canConvert(), convert()
*/
QVariantMap QVariant::toMap() const
{
    return qVariantToHelper<QVariantMap>(d, handlerManager);
}

/*!
    Returns the variant as a QHash<QString, QVariant> if the variant
    has type() \l QMetaType::QVariantHash; otherwise returns an empty map.

    \sa canConvert(), convert()
*/
QVariantHash QVariant::toHash() const
{
    return qVariantToHelper<QVariantHash>(d, handlerManager);
}

/*!
    \fn QDate QVariant::toDate() const

    Returns the variant as a QDate if the variant has userType()
    \l QMetaType::QDate, \l QMetaType::QDateTime, or \l QMetaType::QString;
    otherwise returns an invalid date.

    If the type() is \l QMetaType::QString, an invalid date will be returned if
    the string cannot be parsed as a Qt::ISODate format date.

    \sa canConvert(), convert()
*/
QDate QVariant::toDate() const
{
    return qVariantToHelper<QDate>(d, handlerManager);
}

/*!
    \fn QTime QVariant::toTime() const

    Returns the variant as a QTime if the variant has userType()
    \l QMetaType::QTime, \l QMetaType::QDateTime, or \l QMetaType::QString;
    otherwise returns an invalid time.

    If the type() is \l QMetaType::QString, an invalid time will be returned if
    the string cannot be parsed as a Qt::ISODate format time.

    \sa canConvert(), convert()
*/
QTime QVariant::toTime() const
{
    return qVariantToHelper<QTime>(d, handlerManager);
}

/*!
    \fn QDateTime QVariant::toDateTime() const

    Returns the variant as a QDateTime if the variant has userType()
    \l QMetaType::QDateTime, \l QMetaType::QDate, or \l QMetaType::QString;
    otherwise returns an invalid date/time.

    If the type() is \l QMetaType::QString, an invalid date/time will be
    returned if the string cannot be parsed as a Qt::ISODate format date/time.

    \sa canConvert(), convert()
*/
QDateTime QVariant::toDateTime() const
{
    return qVariantToHelper<QDateTime>(d, handlerManager);
}

/*!
    \since 4.7
    \fn QEasingCurve QVariant::toEasingCurve() const

    Returns the variant as a QEasingCurve if the variant has userType()
    \l QMetaType::QEasingCurve; otherwise returns a default easing curve.

    \sa canConvert(), convert()
*/
#ifndef QT_BOOTSTRAPPED
QEasingCurve QVariant::toEasingCurve() const
{
    return qVariantToHelper<QEasingCurve>(d, handlerManager);
}
#endif

/*!
    \fn QByteArray QVariant::toByteArray() const

    Returns the variant as a QByteArray if the variant has userType()
    \l QMetaType::QByteArray or \l QMetaType::QString (converted using
    QString::fromUtf8()); otherwise returns an empty byte array.

    \sa canConvert(), convert()
*/
QByteArray QVariant::toByteArray() const
{
    return qVariantToHelper<QByteArray>(d, handlerManager);
}

#ifndef QT_NO_GEOM_VARIANT
/*!
    \fn QPoint QVariant::toPoint() const

    Returns the variant as a QPoint if the variant has userType()
    \l QMetaType::QPointF or \l QMetaType::QPointF; otherwise returns a null
    QPoint.

    \sa canConvert(), convert()
*/
QPoint QVariant::toPoint() const
{
    return qVariantToHelper<QPoint>(d, handlerManager);
}

/*!
    \fn QRect QVariant::toRect() const

    Returns the variant as a QRect if the variant has userType()
    \l QMetaType::QRect; otherwise returns an invalid QRect.

    \sa canConvert(), convert()
*/
QRect QVariant::toRect() const
{
    return qVariantToHelper<QRect>(d, handlerManager);
}

/*!
    \fn QSize QVariant::toSize() const

    Returns the variant as a QSize if the variant has userType()
    \l QMetaType::QSize; otherwise returns an invalid QSize.

    \sa canConvert(), convert()
*/
QSize QVariant::toSize() const
{
    return qVariantToHelper<QSize>(d, handlerManager);
}

/*!
    \fn QSizeF QVariant::toSizeF() const

    Returns the variant as a QSizeF if the variant has userType() \l
    QMetaType::QSizeF; otherwise returns an invalid QSizeF.

    \sa canConvert(), convert()
*/
QSizeF QVariant::toSizeF() const
{
    return qVariantToHelper<QSizeF>(d, handlerManager);
}

/*!
    \fn QRectF QVariant::toRectF() const

    Returns the variant as a QRectF if the variant has userType()
    \l QMetaType::QRect or \l QMetaType::QRectF; otherwise returns an invalid
    QRectF.

    \sa canConvert(), convert()
*/
QRectF QVariant::toRectF() const
{
    return qVariantToHelper<QRectF>(d, handlerManager);
}

/*!
    \fn QLineF QVariant::toLineF() const

    Returns the variant as a QLineF if the variant has userType()
    \l QMetaType::QLineF; otherwise returns an invalid QLineF.

    \sa canConvert(), convert()
*/
QLineF QVariant::toLineF() const
{
    return qVariantToHelper<QLineF>(d, handlerManager);
}

/*!
    \fn QLine QVariant::toLine() const

    Returns the variant as a QLine if the variant has userType()
    \l QMetaType::QLine; otherwise returns an invalid QLine.

    \sa canConvert(), convert()
*/
QLine QVariant::toLine() const
{
    return qVariantToHelper<QLine>(d, handlerManager);
}

/*!
    \fn QPointF QVariant::toPointF() const

    Returns the variant as a QPointF if the variant has userType() \l
    QMetaType::QPoint or \l QMetaType::QPointF; otherwise returns a null
    QPointF.

    \sa canConvert(), convert()
*/
QPointF QVariant::toPointF() const
{
    return qVariantToHelper<QPointF>(d, handlerManager);
}

#endif // QT_NO_GEOM_VARIANT

#ifndef QT_BOOTSTRAPPED
/*!
    \fn QUrl QVariant::toUrl() const

    Returns the variant as a QUrl if the variant has userType()
    \l QMetaType::QUrl; otherwise returns an invalid QUrl.

    \sa canConvert(), convert()
*/
QUrl QVariant::toUrl() const
{
    return qVariantToHelper<QUrl>(d, handlerManager);
}
#endif

/*!
    \fn QLocale QVariant::toLocale() const

    Returns the variant as a QLocale if the variant has userType()
    \l QMetaType::QLocale; otherwise returns an invalid QLocale.

    \sa canConvert(), convert()
*/
QLocale QVariant::toLocale() const
{
    return qVariantToHelper<QLocale>(d, handlerManager);
}

/*!
    \fn QRegExp QVariant::toRegExp() const
    \since 4.1

    Returns the variant as a QRegExp if the variant has userType()
    \l QMetaType::QRegExp; otherwise returns an empty QRegExp.

    \sa canConvert(), convert()
*/
#ifndef QT_NO_REGEXP
QRegExp QVariant::toRegExp() const
{
    return qVariantToHelper<QRegExp>(d, handlerManager);
}
#endif

#ifndef QT_BOOTSTRAPPED
/*!
    \fn QRegularExpression QVariant::toRegularExpression() const
    \since 5.0

    Returns the variant as a QRegularExpression if the variant has userType() \l
    QRegularExpression; otherwise returns an empty QRegularExpression.

    \sa canConvert(), convert()
*/
#ifndef QT_NO_REGULAREXPRESSION
QRegularExpression QVariant::toRegularExpression() const
{
    return qVariantToHelper<QRegularExpression>(d, handlerManager);
}
#endif // QT_NO_REGULAREXPRESSION

/*!
    \since 5.0

    Returns the variant as a QUuid if the variant has userType() \l
    QUuid; otherwise returns a default constructed QUuid.

    \sa canConvert(), convert()
*/
QUuid QVariant::toUuid() const
{
    return qVariantToHelper<QUuid>(d, handlerManager);
}

/*!
    \since 5.0

    Returns the variant as a QModelIndex if the variant has userType() \l
    QModelIndex; otherwise returns a default constructed QModelIndex.

    \sa canConvert(), convert()
*/
QModelIndex QVariant::toModelIndex() const
{
    return qVariantToHelper<QModelIndex>(d, handlerManager);
}

/*!
    \since 5.0

    Returns the variant as a QJsonValue if the variant has userType() \l
    QJsonValue; otherwise returns a default constructed QJsonValue.

    \sa canConvert(), convert()
*/
QJsonValue QVariant::toJsonValue() const
{
    return qVariantToHelper<QJsonValue>(d, handlerManager);
}

/*!
    \since 5.0

    Returns the variant as a QJsonObject if the variant has userType() \l
    QJsonObject; otherwise returns a default constructed QJsonObject.

    \sa canConvert(), convert()
*/
QJsonObject QVariant::toJsonObject() const
{
    return qVariantToHelper<QJsonObject>(d, handlerManager);
}

/*!
    \since 5.0

    Returns the variant as a QJsonArray if the variant has userType() \l
    QJsonArray; otherwise returns a default constructed QJsonArray.

    \sa canConvert(), convert()
*/
QJsonArray QVariant::toJsonArray() const
{
    return qVariantToHelper<QJsonArray>(d, handlerManager);
}

/*!
    \since 5.0

    Returns the variant as a QJsonDocument if the variant has userType() \l
    QJsonDocument; otherwise returns a default constructed QJsonDocument.

    \sa canConvert(), convert()
*/
QJsonDocument QVariant::toJsonDocument() const
{
    return qVariantToHelper<QJsonDocument>(d, handlerManager);
}
#endif

/*!
    \fn QChar QVariant::toChar() const

    Returns the variant as a QChar if the variant has userType()
    \l QMetaType::QChar, \l QMetaType::Int, or \l QMetaType::UInt; otherwise
    returns an invalid QChar.

    \sa canConvert(), convert()
*/
QChar QVariant::toChar() const
{
    return qVariantToHelper<QChar>(d, handlerManager);
}

/*!
    Returns the variant as a QBitArray if the variant has userType()
    \l QMetaType::QBitArray; otherwise returns an empty bit array.

    \sa canConvert(), convert()
*/
QBitArray QVariant::toBitArray() const
{
    return qVariantToHelper<QBitArray>(d, handlerManager);
}

template <typename T>
inline T qNumVariantToHelper(const QVariant::Private &d,
                             const HandlersManager &handlerManager, bool *ok, const T& val)
{
    const uint t = qMetaTypeId<T>();
    if (ok)
        *ok = true;

    if (d.type == t)
        return val;

    T ret = 0;
    if ((d.type >= QMetaType::User || t >= QMetaType::User)
        && QMetaType::convert(&val, d.type, &ret, t)) {
        return ret;
    }

    if (!handlerManager[d.type]->convert(&d, t, &ret, ok) && ok)
        *ok = false;
    return ret;
}

/*!
    Returns the variant as an int if the variant has userType()
    \l QMetaType::Int, \l QMetaType::Bool, \l QMetaType::QByteArray,
    \l QMetaType::QChar, \l QMetaType::Double, \l QMetaType::LongLong,
    \l QMetaType::QString, \l QMetaType::UInt, or \l QMetaType::ULongLong;
    otherwise returns 0.

    If \a ok is non-null: \c{*}\a{ok} is set to true if the value could be
    converted to an int; otherwise \c{*}\a{ok} is set to false.

    \b{Warning:} If the value is convertible to a \l QMetaType::LongLong but is
    too large to be represented in an int, the resulting arithmetic overflow
    will not be reflected in \a ok. A simple workaround is to use
    QString::toInt().

    \sa canConvert(), convert()
*/
int QVariant::toInt(bool *ok) const
{
    return qNumVariantToHelper<int>(d, handlerManager, ok, d.data.i);
}

/*!
    Returns the variant as an unsigned int if the variant has userType()
    \l QMetaType::UInt, \l QMetaType::Bool, \l QMetaType::QByteArray,
    \l QMetaType::QChar, \l QMetaType::Double, \l QMetaType::Int,
    \l QMetaType::LongLong, \l QMetaType::QString, or \l QMetaType::ULongLong;
    otherwise returns 0.

    If \a ok is non-null: \c{*}\a{ok} is set to true if the value could be
    converted to an unsigned int; otherwise \c{*}\a{ok} is set to false.

    \b{Warning:} If the value is convertible to a \l QMetaType::ULongLong but is
    too large to be represented in an unsigned int, the resulting arithmetic
    overflow will not be reflected in \a ok. A simple workaround is to use
    QString::toUInt().

    \sa canConvert(), convert()
*/
uint QVariant::toUInt(bool *ok) const
{
    return qNumVariantToHelper<uint>(d, handlerManager, ok, d.data.u);
}

/*!
    Returns the variant as a long long int if the variant has userType()
    \l QMetaType::LongLong, \l QMetaType::Bool, \l QMetaType::QByteArray,
    \l QMetaType::QChar, \l QMetaType::Double, \l QMetaType::Int,
    \l QMetaType::QString, \l QMetaType::UInt, or \l QMetaType::ULongLong;
    otherwise returns 0.

    If \a ok is non-null: \c{*}\c{ok} is set to true if the value could be
    converted to an int; otherwise \c{*}\c{ok} is set to false.

    \sa canConvert(), convert()
*/
qlonglong QVariant::toLongLong(bool *ok) const
{
    return qNumVariantToHelper<qlonglong>(d, handlerManager, ok, d.data.ll);
}

/*!
    Returns the variant as as an unsigned long long int if the
    variant has type() \l QMetaType::ULongLong, \l QMetaType::Bool,
    \l QMetaType::QByteArray, \l QMetaType::QChar, \l QMetaType::Double,
    \l QMetaType::Int, \l QMetaType::LongLong, \l QMetaType::QString, or
    \l QMetaType::UInt; otherwise returns 0.

    If \a ok is non-null: \c{*}\a{ok} is set to true if the value could be
    converted to an int; otherwise \c{*}\a{ok} is set to false.

    \sa canConvert(), convert()
*/
qulonglong QVariant::toULongLong(bool *ok) const
{
    return qNumVariantToHelper<qulonglong>(d, handlerManager, ok, d.data.ull);
}

/*!
    Returns the variant as a bool if the variant has userType() Bool.

    Returns \c true if the variant has userType() \l QMetaType::Bool,
    \l QMetaType::QChar, \l QMetaType::Double, \l QMetaType::Int,
    \l QMetaType::LongLong, \l QMetaType::UInt, or \l QMetaType::ULongLong and
    the value is non-zero, or if the variant has type \l QMetaType::QString or
    \l QMetaType::QByteArray and its lower-case content is not one of the
    following: empty, "0" or "false"; otherwise returns \c false.

    \sa canConvert(), convert()
*/
bool QVariant::toBool() const
{
    if (d.type == Bool)
        return d.data.b;

    bool res = false;
    handlerManager[d.type]->convert(&d, Bool, &res, 0);

    return res;
}

/*!
    Returns the variant as a double if the variant has userType()
    \l QMetaType::Double, \l QMetaType::Float, \l QMetaType::Bool,
    \l QMetaType::QByteArray, \l QMetaType::Int, \l QMetaType::LongLong,
    \l QMetaType::QString, \l QMetaType::UInt, or \l QMetaType::ULongLong;
    otherwise returns 0.0.

    If \a ok is non-null: \c{*}\a{ok} is set to true if the value could be
    converted to a double; otherwise \c{*}\a{ok} is set to false.

    \sa canConvert(), convert()
*/
double QVariant::toDouble(bool *ok) const
{
    return qNumVariantToHelper<double>(d, handlerManager, ok, d.data.d);
}

/*!
    Returns the variant as a float if the variant has userType()
    \l QMetaType::Double, \l QMetaType::Float, \l QMetaType::Bool,
    \l QMetaType::QByteArray, \l QMetaType::Int, \l QMetaType::LongLong,
    \l QMetaType::QString, \l QMetaType::UInt, or \l QMetaType::ULongLong;
    otherwise returns 0.0.

    \since 4.6

    If \a ok is non-null: \c{*}\a{ok} is set to true if the value could be
    converted to a double; otherwise \c{*}\a{ok} is set to false.

    \sa canConvert(), convert()
*/
float QVariant::toFloat(bool *ok) const
{
    return qNumVariantToHelper<float>(d, handlerManager, ok, d.data.f);
}

/*!
    Returns the variant as a qreal if the variant has userType()
    \l QMetaType::Double, \l QMetaType::Float, \l QMetaType::Bool,
    \l QMetaType::QByteArray, \l QMetaType::Int, \l QMetaType::LongLong,
    \l QMetaType::QString, \l QMetaType::UInt, or \l QMetaType::ULongLong;
    otherwise returns 0.0.

    \since 4.6

    If \a ok is non-null: \c{*}\a{ok} is set to true if the value could be
    converted to a double; otherwise \c{*}\a{ok} is set to false.

    \sa canConvert(), convert()
*/
qreal QVariant::toReal(bool *ok) const
{
    return qNumVariantToHelper<qreal>(d, handlerManager, ok, d.data.real);
}

/*!
    Returns the variant as a QVariantList if the variant has userType()
    \l QMetaType::QVariantList or \l QMetaType::QStringList; otherwise returns
    an empty list.

    \sa canConvert(), convert()
*/
QVariantList QVariant::toList() const
{
    return qVariantToHelper<QVariantList>(d, handlerManager);
}


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
                | 1 << QVariant::Url        | 1 << QVariant::Uuid,

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

/*QEasingCurve*/  0,

/*QUuid*/         1 << QVariant::String
};

#ifndef QT_BOOTSTRAPPED
/*!
    Returns \c true if from inherits to.
*/
static bool canConvertMetaObject(const QMetaObject *from, const QMetaObject *to)
{
    if (from && to == &QObject::staticMetaObject)
        return true;

    while (from) {
        if (from == to)
            return true;
        from = from->superClass();
    }

    return false;
}
#endif

static bool canConvertMetaObject(int fromId, int toId, QObject *fromObject)
{
#ifndef QT_BOOTSTRAPPED
    QMetaType toType(toId);
    if ((QMetaType::typeFlags(fromId) & QMetaType::PointerToQObject) && (toType.flags() & QMetaType::PointerToQObject)) {
        if (!fromObject)
            return true;
        return canConvertMetaObject(fromObject->metaObject(), toType.metaObject());
    }
#else
    Q_UNUSED(fromId);
    Q_UNUSED(toId);
    Q_UNUSED(fromObject);
#endif
    return false;
}


/*!
    Returns \c true if the variant's type can be cast to the requested
    type, \a targetTypeId. Such casting is done automatically when calling the
    toInt(), toBool(), ... methods.

    The following casts are done automatically:

    \table
    \header \li Type \li Automatically Cast To
    \row \li \l QMetaType::Bool \li \l QMetaType::QChar, \l QMetaType::Double,
        \l QMetaType::Int, \l QMetaType::LongLong, \l QMetaType::QString,
        \l QMetaType::UInt, \l QMetaType::ULongLong
    \row \li \l QMetaType::QByteArray \li \l QMetaType::Double,
        \l QMetaType::Int, \l QMetaType::LongLong, \l QMetaType::QString,
        \l QMetaType::UInt, \l QMetaType::ULongLong
    \row \li \l QMetaType::QChar \li \l QMetaType::Bool, \l QMetaType::Int,
        \l QMetaType::UInt, \l QMetaType::LongLong, \l QMetaType::ULongLong
    \row \li \l QMetaType::QColor \li \l QMetaType::QString
    \row \li \l QMetaType::QDate \li \l QMetaType::QDateTime,
        \l QMetaType::QString
    \row \li \l QMetaType::QDateTime \li \l QMetaType::QDate,
        \l QMetaType::QString, \l QMetaType::QTime
    \row \li \l QMetaType::Double \li \l QMetaType::Bool, \l QMetaType::Int,
        \l QMetaType::LongLong, \l QMetaType::QString, \l QMetaType::UInt,
        \l QMetaType::ULongLong
    \row \li \l QMetaType::QFont \li \l QMetaType::QString
    \row \li \l QMetaType::Int \li \l QMetaType::Bool, \l QMetaType::QChar,
        \l QMetaType::Double, \l QMetaType::LongLong, \l QMetaType::QString,
        \l QMetaType::UInt, \l QMetaType::ULongLong
    \row \li \l QMetaType::QKeySequence \li \l QMetaType::Int,
        \l QMetaType::QString
    \row \li \l QMetaType::QVariantList \li \l QMetaType::QStringList (if the
        list's items can be converted to QStrings)
    \row \li \l QMetaType::LongLong \li \l QMetaType::Bool,
        \l QMetaType::QByteArray, \l QMetaType::QChar, \l QMetaType::Double,
        \l QMetaType::Int, \l QMetaType::QString, \l QMetaType::UInt,
        \l QMetaType::ULongLong
    \row \li \l QMetaType::QPoint \li QMetaType::QPointF
    \row \li \l QMetaType::QRect \li QMetaType::QRectF
    \row \li \l QMetaType::QString \li \l QMetaType::Bool,
        \l QMetaType::QByteArray, \l QMetaType::QChar, \l QMetaType::QColor,
        \l QMetaType::QDate, \l QMetaType::QDateTime, \l QMetaType::Double,
        \l QMetaType::QFont, \l QMetaType::Int, \l QMetaType::QKeySequence,
        \l QMetaType::LongLong, \l QMetaType::QStringList, \l QMetaType::QTime,
        \l QMetaType::UInt, \l QMetaType::ULongLong
    \row \li \l QMetaType::QStringList \li \l QMetaType::QVariantList,
        \l QMetaType::QString (if the list contains exactly one item)
    \row \li \l QMetaType::QTime \li \l QMetaType::QString
    \row \li \l QMetaType::UInt \li \l QMetaType::Bool, \l QMetaType::QChar,
        \l QMetaType::Double, \l QMetaType::Int, \l QMetaType::LongLong,
        \l QMetaType::QString, \l QMetaType::ULongLong
    \row \li \l QMetaType::ULongLong \li \l QMetaType::Bool,
        \l QMetaType::QChar, \l QMetaType::Double, \l QMetaType::Int,
        \l QMetaType::LongLong, \l QMetaType::QString, \l QMetaType::UInt
    \endtable

    A QVariant containing a pointer to a type derived from QObject will also return true for this
    function if a qobject_cast to the type described by \a targetTypeId would succeed. Note that
    this only works for QObject subclasses which use the Q_OBJECT macro.

    A QVariant containing a sequential container will also return true for this
    function if the \a targetTypeId is QVariantList. It is possible to iterate over
    the contents of the container without extracting it as a (copied) QVariantList:

    \snippet code/src_corelib_kernel_qvariant.cpp 9

    This requires that the value_type of the container is itself a metatype.

    Similarly, a QVariant containing a sequential container will also return true for this
    function the \a targetTypeId is QVariantHash or QVariantMap. It is possible to iterate over
    the contents of the container without extracting it as a (copied) QVariantHash or QVariantMap:

    \snippet code/src_corelib_kernel_qvariant.cpp 10

    \sa convert(), QSequentialIterable, Q_DECLARE_SEQUENTIAL_CONTAINER_METATYPE(), QAssociativeIterable,
        Q_DECLARE_ASSOCIATIVE_CONTAINER_METATYPE()
*/
bool QVariant::canConvert(int targetTypeId) const
{
    if (targetTypeId == QMetaType::QVariantList
            && (d.type == QMetaType::QVariantList
              || d.type == QMetaType::QStringList
              || QMetaType::hasRegisteredConverterFunction(d.type,
                    qMetaTypeId<QtMetaTypePrivate::QSequentialIterableImpl>()))) {
        return true;
    }

    if ((targetTypeId == QMetaType::QVariantHash || targetTypeId == QMetaType::QVariantMap)
            && (d.type == QMetaType::QVariantMap
              || d.type == QMetaType::QVariantHash
              || QMetaType::hasRegisteredConverterFunction(d.type,
                    qMetaTypeId<QtMetaTypePrivate::QAssociativeIterableImpl>()))) {
        return true;
    }

    if (targetTypeId == qMetaTypeId<QPair<QVariant, QVariant> >() &&
              QMetaType::hasRegisteredConverterFunction(d.type,
                    qMetaTypeId<QtMetaTypePrivate::QPairVariantInterfaceImpl>())) {
        return true;
    }

    if ((d.type >= QMetaType::User || targetTypeId >= QMetaType::User)
        && QMetaType::hasRegisteredConverterFunction(d.type, targetTypeId)) {
        return true;
    }

    // TODO Reimplement this function, currently it works but it is a historical mess.
    uint currentType = ((d.type == QMetaType::Float) ? QVariant::Double : d.type);
    if (currentType == QMetaType::SChar || currentType == QMetaType::Char)
        currentType = QMetaType::UInt;
    if (targetTypeId == QMetaType::SChar || currentType == QMetaType::Char)
        targetTypeId = QMetaType::UInt;
    if (uint(targetTypeId) == uint(QMetaType::Float)) targetTypeId = QVariant::Double;


    if (currentType == uint(targetTypeId))
        return true;

    if (targetTypeId < 0)
        return false;
    if (targetTypeId >= QMetaType::User) {
        if (QMetaType::typeFlags(targetTypeId) & QMetaType::IsEnumeration) {
            targetTypeId = QMetaType::Int;
        } else {
            return canConvertMetaObject(currentType, targetTypeId, d.data.o);
        }
    }

    if (currentType == QMetaType::QJsonValue) {
        switch (targetTypeId) {
        case QMetaType::QString:
        case QMetaType::Bool:
        case QMetaType::Int:
        case QMetaType::UInt:
        case QMetaType::Double:
        case QMetaType::Float:
        case QMetaType::ULong:
        case QMetaType::Long:
        case QMetaType::LongLong:
        case QMetaType::ULongLong:
        case QMetaType::UShort:
        case QMetaType::UChar:
        case QMetaType::Char:
        case QMetaType::SChar:
        case QMetaType::Short:
            return true;
        default:
            return false;
        }
    }

    // FIXME It should be LastCoreType intead of Uuid
    if (currentType > int(QMetaType::QUuid) || targetTypeId > int(QMetaType::QUuid)) {
        switch (uint(targetTypeId)) {
        case QVariant::Int:
            if (currentType == QVariant::KeySequence)
                return true;
            // fall through
        case QVariant::UInt:
        case QVariant::LongLong:
        case QVariant::ULongLong:
               return currentType == QMetaType::ULong
                   || currentType == QMetaType::Long
                   || currentType == QMetaType::UShort
                   || currentType == QMetaType::UChar
                   || currentType == QMetaType::Char
                   || currentType == QMetaType::SChar
                   || currentType == QMetaType::Short
                   || QMetaType::typeFlags(currentType) & QMetaType::IsEnumeration;
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
        case QMetaType::SChar:
        case QMetaType::UChar:
        case QMetaType::ULong:
        case QMetaType::Short:
        case QMetaType::UShort:
            return qCanConvertMatrix[QVariant::Int] & (1 << currentType)
                || currentType == QVariant::Int
                || QMetaType::typeFlags(currentType) & QMetaType::IsEnumeration;
        case QMetaType::QObjectStar:
            return canConvertMetaObject(currentType, targetTypeId, d.data.o);
        default:
            return false;
        }
    }

    if (targetTypeId == String && currentType == StringList)
        return v_cast<QStringList>(&d)->count() == 1;
    return qCanConvertMatrix[targetTypeId] & (1 << currentType);
}

/*!
    Casts the variant to the requested type, \a targetTypeId. If the cast cannot be
    done, the variant is cleared. Returns \c true if the current type of
    the variant was successfully cast; otherwise returns \c false.

    A QVariant containing a pointer to a type derived from QObject will also convert
    and return true for this function if a qobject_cast to the type described
    by \a targetTypeId would succeed. Note that this only works for QObject subclasses
    which use the Q_OBJECT macro.

    \warning For historical reasons, converting a null QVariant results
    in a null value of the desired type (e.g., an empty string for
    QString) and a result of false.

    \sa canConvert(), clear()
*/

bool QVariant::convert(int targetTypeId)
{
    if (d.type == uint(targetTypeId))
        return true;

    QVariant oldValue = *this;

    clear();
    if (!oldValue.canConvert(targetTypeId))
        return false;

    create(targetTypeId, 0);
    if (oldValue.isNull())
        return false;

    if ((QMetaType::typeFlags(oldValue.userType()) & QMetaType::PointerToQObject) && (QMetaType::typeFlags(targetTypeId) & QMetaType::PointerToQObject)) {
        create(targetTypeId, &oldValue.d.data.o);
        return true;
    }

    bool isOk = true;
    int converterType = std::max(oldValue.userType(), targetTypeId);
    if (!handlerManager[converterType]->convert(&oldValue.d, targetTypeId, data(), &isOk))
        isOk = false;
    d.is_null = !isOk;
    return isOk;
}

/*!
  \fn bool QVariant::convert(const int type, void *ptr) const
  \internal
  Created for qvariant_cast() usage
*/
bool QVariant::convert(const int type, void *ptr) const
{
    return handlerManager[type]->convert(&d, type, ptr, 0);
}


/*!
    \fn bool operator==(const QVariant &v1, const QVariant &v2)

    \relates QVariant

    Returns \c true if \a v1 and \a v2 are equal; otherwise returns \c false.

    If \a v1 and \a v2 have the same \l{QVariant::}{type()}, the
    type's equality operator is used for comparison. If not, it is
    attempted to \l{QVariant::}{convert()} \a v2 to the same type as
    \a v1. See \l{QVariant::}{canConvert()} for a list of possible
    conversions.

    The result of the function is not affected by the result of QVariant::isNull,
    which means that two values can be equal even if one of them is null and
    another is not.

    \warning To make this function work with a custom type registered with
    qRegisterMetaType(), its comparison operator must be registered using
    QMetaType::registerComparators().
*/
/*!
    \fn bool operator!=(const QVariant &v1, const QVariant &v2)

    \relates QVariant

    Returns \c false if \a v1 and \a v2 are equal; otherwise returns \c true.

    \warning To make this function work with a custom type registered with
    qRegisterMetaType(), its comparison operator must be registered using
    QMetaType::registerComparators().
*/

/*! \fn bool QVariant::operator==(const QVariant &v) const

    Compares this QVariant with \a v and returns \c true if they are
    equal; otherwise returns \c false.

    QVariant uses the equality operator of the type() it contains to
    check for equality. QVariant will try to convert() \a v if its
    type is not the same as this variant's type. See canConvert() for
    a list of possible conversions.

    \warning To make this function work with a custom type registered with
    qRegisterMetaType(), its comparison operator must be registered using
    QMetaType::registerComparators().
*/

/*!
    \fn bool QVariant::operator!=(const QVariant &v) const

    Compares this QVariant with \a v and returns \c true if they are not
    equal; otherwise returns \c false.

    \warning To make this function work with a custom type registered with
    qRegisterMetaType(), its comparison operator must be registered using
    QMetaType::registerComparators().
*/

/*!
    \fn bool QVariant::operator<(const QVariant &v) const

    Compares this QVariant with \a v and returns \c true if this is less than \a v.

    \note Comparability might not be availabe for the type stored in this QVariant
    or in \a v.

    \warning To make this function work with a custom type registered with
    qRegisterMetaType(), its comparison operator must be registered using
    QMetaType::registerComparators().
*/

/*!
    \fn bool QVariant::operator<=(const QVariant &v) const

    Compares this QVariant with \a v and returns \c true if this is less or equal than \a v.

    \note Comparability might not be available for the type stored in this QVariant
    or in \a v.

    \warning To make this function work with a custom type registered with
    qRegisterMetaType(), its comparison operator must be registered using
    QMetaType::registerComparators().
*/

/*!
    \fn bool QVariant::operator>(const QVariant &v) const

    Compares this QVariant with \a v and returns \c true if this is larger than \a v.

    \note Comparability might not be available for the type stored in this QVariant
    or in \a v.

    \warning To make this function work with a custom type registered with
    qRegisterMetaType(), its comparison operator must be registered using
    QMetaType::registerComparators().
*/

/*!
    \fn bool QVariant::operator>=(const QVariant &v) const

    Compares this QVariant with \a v and returns \c true if this is larger or equal than \a v.

    \note Comparability might not be available for the type stored in this QVariant
    or in \a v.

    \warning To make this function work with a custom type registered with
    qRegisterMetaType(), its comparison operator must be registered using
    QMetaType::registerComparators().
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

/*!
    \internal
 */
bool QVariant::cmp(const QVariant &v) const
{
    QVariant v1 = *this;
    QVariant v2 = v;
    if (d.type != v2.d.type) {
        if (qIsNumericType(d.type) && qIsNumericType(v.d.type)) {
            if (qIsFloatingPoint(d.type) || qIsFloatingPoint(v.d.type))
                return qFuzzyCompare(toReal(), v.toReal());
            else
                return toLongLong() == v.toLongLong();
        }
        if (!v2.canConvert(v1.d.type) || !v2.convert(v1.d.type))
            return false;
    }
    if (v1.d.type >= QMetaType::User) {
        int result;
        if (QMetaType::compare(QT_PREPEND_NAMESPACE(constData(v1.d)), QT_PREPEND_NAMESPACE(constData(v2.d)), v1.d.type, &result))
            return result == 0;
    }
    return handlerManager[v1.d.type]->compare(&v1.d, &v2.d);
}

/*!
    \internal
 */
int QVariant::compare(const QVariant &v) const
{
    if (cmp(v))
        return 0;
    QVariant v1 = *this;
    QVariant v2 = v;
    if (v1.d.type != v2.d.type) {
        // if both types differ, try to convert
        if (v2.canConvert(v1.d.type)) {
            QVariant temp = v2;
            if (temp.convert(v1.d.type))
                v2 = temp;
        }
        if (v1.d.type != v2.d.type && v1.canConvert(v2.d.type)) {
            QVariant temp = v1;
            if (temp.convert(v2.d.type))
                v1 = temp;
        }
        if (v1.d.type != v2.d.type) {
            // if conversion fails, default to toString
            return v1.toString().compare(v2.toString(), Qt::CaseInsensitive);
        }
    }
    if (v1.d.type >= QMetaType::User) {
        int result;
        if (QMetaType::compare(QT_PREPEND_NAMESPACE(constData(d)), QT_PREPEND_NAMESPACE(constData(v2.d)), d.type, &result))
            return result;
    }
    if (qIsNumericType(v1.d.type)) {
        if (qIsFloatingPoint(v1.d.type))
            return v1.toReal() < v2.toReal() ? -1 : 1;
        else
            return v1.toLongLong() < v2.toLongLong() ? -1 : 1;
    }
    switch (v1.d.type) {
    case QVariant::Date:
        return v1.toDate() < v2.toDate() ? -1 : 1;
    case QVariant::Time:
        return v1.toTime() < v2.toTime() ? -1 : 1;
    case QVariant::DateTime:
        return v1.toDateTime() < v2.toDateTime() ? -1 : 1;
    }
    return v1.toString().compare(v2.toString(), Qt::CaseInsensitive);
}

/*!
    \internal
 */

const void *QVariant::constData() const
{
    return d.is_shared ? d.data.shared->ptr : reinterpret_cast<const void *>(&d.data.ptr);
}

/*!
    \fn const void* QVariant::data() const

    \internal
*/

/*!
    \internal
*/
void* QVariant::data()
{
    detach();
    return const_cast<void *>(constData());
}


/*!
    Returns \c true if this is a null variant, false otherwise. A variant is
    considered null if it contains a default constructed value or a built-in
    type instance that has an isNull method, in which case the result
    would be the same as calling isNull on the wrapped object.

    \warning The result of the function doesn't affect == operator, which means
    that two values can be equal even if one of them is null and another is not.
*/
bool QVariant::isNull() const
{
    return handlerManager[d.type]->isNull(&d);
}

#ifndef QT_NO_DEBUG_STREAM
QDebug operator<<(QDebug dbg, const QVariant &v)
{
    const uint typeId = v.d.type;
    dbg.nospace() << "QVariant(";
    if (typeId != QMetaType::UnknownType) {
        dbg.nospace() << QMetaType::typeName(typeId) << ", ";
        bool userStream = false;
        bool canConvertToString = false;
        if (typeId >= QMetaType::User) {
            userStream = QMetaType::debugStream(dbg, constData(v.d), typeId);
            canConvertToString = v.canConvert<QString>();
        }
        if (!userStream && canConvertToString)
            dbg << v.toString();
        else if (!userStream)
            handlerManager[typeId]->debugStream(dbg, v);
    } else {
        dbg.nospace() << "Invalid";
    }
    dbg.nospace() << ')';
    return dbg.space();
}

QDebug operator<<(QDebug dbg, const QVariant::Type p)
{
    dbg.nospace() << "QVariant::"
                  << (int(p) != int(QMetaType::UnknownType)
                     ? QMetaType::typeName(p)
                     : "Invalid");
    return dbg.space();
}
#endif


/*! \fn void QVariant::setValue(const T &value)

    Stores a copy of \a value. If \c{T} is a type that QVariant
    doesn't support, QMetaType is used to store the value. A compile
    error will occur if QMetaType doesn't handle the type.

    Example:

    \snippet code/src_corelib_kernel_qvariant.cpp 4

    \sa value(), fromValue(), canConvert()
 */

/*! \fn T QVariant::value() const

    Returns the stored value converted to the template type \c{T}.
    Call canConvert() to find out whether a type can be converted.
    If the value cannot be converted, a \l{default-constructed value}
    will be returned.

    If the type \c{T} is supported by QVariant, this function behaves
    exactly as toString(), toInt() etc.

    Example:

    \snippet code/src_corelib_kernel_qvariant.cpp 5

    If the QVariant contains a pointer to a type derived from QObject then
    \c{T} may be any QObject type. If the pointer stored in the QVariant can be
    qobject_cast to T, then that result is returned. Otherwise a null pointer is
    returned. Note that this only works for QObject subclasses which use the
    Q_OBJECT macro.

    If the QVariant contains a sequential container and \c{T} is QVariantList, the
    elements of the container will be converted into QVariants and returned as a QVariantList.

    \snippet code/src_corelib_kernel_qvariant.cpp 9

    \sa setValue(), fromValue(), canConvert(), Q_DECLARE_SEQUENTIAL_CONTAINER_METATYPE()
*/

/*! \fn bool QVariant::canConvert() const

    Returns \c true if the variant can be converted to the template type \c{T},
    otherwise false.

    Example:

    \snippet code/src_corelib_kernel_qvariant.cpp 6

    A QVariant containing a pointer to a type derived from QObject will also return true for this
    function if a qobject_cast to the template type \c{T} would succeed. Note that this only works
    for QObject subclasses which use the Q_OBJECT macro.

    \sa convert()
*/

/*! \fn static QVariant QVariant::fromValue(const T &value)

    Returns a QVariant containing a copy of \a value. Behaves
    exactly like setValue() otherwise.

    Example:

    \snippet code/src_corelib_kernel_qvariant.cpp 7

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

    \snippet code/src_corelib_kernel_qvariant.cpp 8

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

    Returns \c true if the given \a value can be converted to the
    template type specified; otherwise returns \c false.

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

/*!
    \fn const DataPtr &QVariant::data_ptr() const
    \internal
*/

/*!
    \class QSequentialIterable
    \since 5.2
    \inmodule QtCore
    \brief The QSequentialIterable class is an iterable interface for a container in a QVariant.

    This class allows several methods of accessing the elements of a container held within
    a QVariant. An instance of QSequentialIterable can be extracted from a QVariant if it can
    be converted to a QVariantList.

    \snippet code/src_corelib_kernel_qvariant.cpp 9

    The container itself is not copied before iterating over it.

    \sa QVariant
*/

/*!
    \internal
*/
QSequentialIterable::QSequentialIterable(QtMetaTypePrivate::QSequentialIterableImpl impl)
  : m_impl(impl)
{
}

QSequentialIterable::const_iterator::const_iterator(const QSequentialIterable &iter, QAtomicInt *ref_)
  : m_impl(iter.m_impl), ref(ref_)
{
    ref->ref();
}

QSequentialIterable::const_iterator::const_iterator(const QtMetaTypePrivate::QSequentialIterableImpl &impl, QAtomicInt *ref_)
  : m_impl(impl), ref(ref_)
{
    ref->ref();
}

void QSequentialIterable::const_iterator::begin()
{
    m_impl.moveToBegin();
}

void QSequentialIterable::const_iterator::end()
{
    m_impl.moveToEnd();
}

/*! \fn QSequentialIterable::const_iterator QSequentialIterable::begin() const

    Returns a QSequentialIterable::const_iterator for the beginning of the container. This
    can be used in stl-style iteration.

    \sa end()
*/
QSequentialIterable::const_iterator QSequentialIterable::begin() const
{
    const_iterator it(*this, new QAtomicInt(0));
    it.begin();
    return it;
}

/*!
    Returns a QSequentialIterable::const_iterator for the end of the container. This
    can be used in stl-style iteration.

    \sa begin()
*/
QSequentialIterable::const_iterator QSequentialIterable::end() const
{
    const_iterator it(*this, new QAtomicInt(0));
    it.end();
    return it;
}

/*!
    Returns the element at position \a idx in the container.
*/
QVariant QSequentialIterable::at(int idx) const
{
    const QtMetaTypePrivate::VariantData d = m_impl.at(idx);
    if (d.metaTypeId == qMetaTypeId<QVariant>())
        return *reinterpret_cast<const QVariant*>(d.data);
    return QVariant(d.metaTypeId, d.data, d.flags);
}

/*!
    Returns the number of elements in the container.
*/
int QSequentialIterable::size() const
{
    return m_impl.size();
}

/*!
    Returns whether it is possible to iterate over the container in reverse. This
    corresponds to the std::bidirectional_iterator_tag iterator trait of the
    const_iterator of the container.
*/
bool QSequentialIterable::canReverseIterate() const
{
    return m_impl._iteratorCapabilities & QtMetaTypePrivate::BiDirectionalCapability;
}

/*!
    \class QSequentialIterable::const_iterator
    \since 5.2
    \inmodule QtCore
    \brief The QSequentialIterable::const_iterator allows iteration over a container in a QVariant.

    A QSequentialIterable::const_iterator can only be created by a QSequentialIterable instance,
    and can be used in a way similar to other stl-style iterators.

    \snippet code/src_corelib_kernel_qvariant.cpp 9

    \sa QSequentialIterable
*/


/*!
    Destroys the QSequentialIterable::const_iterator.
*/
QSequentialIterable::const_iterator::~const_iterator() {
    if (!ref->deref()) {
        m_impl.destroyIter();
        delete ref;
    }
}

/*!
    Creates a copy of \a other.
*/
QSequentialIterable::const_iterator::const_iterator(const const_iterator &other)
  : m_impl(other.m_impl), ref(other.ref)
{
    ref->ref();
}

/*!
    Assigns \a other to this.
*/
QSequentialIterable::const_iterator&
QSequentialIterable::const_iterator::operator=(const const_iterator &other)
{
    if (!m_impl.equal(other.m_impl)) {
        m_impl = other.m_impl;
        ref = other.ref;
    }
    ref->ref();
    return *this;
}

/*!
    Returns the current item, converted to a QVariant.
*/
const QVariant QSequentialIterable::const_iterator::operator*() const
{
    const QtMetaTypePrivate::VariantData d = m_impl.getCurrent();
    if (d.metaTypeId == qMetaTypeId<QVariant>())
        return *reinterpret_cast<const QVariant*>(d.data);
    return QVariant(d.metaTypeId, d.data, d.flags);
}

/*!
    Returns \c true if \a other points to the same item as this
    iterator; otherwise returns \c false.

    \sa operator!=()
*/
bool QSequentialIterable::const_iterator::operator==(const const_iterator &other) const
{
    return m_impl.equal(other.m_impl);
}

/*!
    Returns \c true if \a other points to a different item than this
    iterator; otherwise returns \c false.

    \sa operator==()
*/
bool QSequentialIterable::const_iterator::operator!=(const const_iterator &other) const
{
    return !m_impl.equal(other.m_impl);
}

/*!
    The prefix ++ operator (\c{++it}) advances the iterator to the
    next item in the container and returns an iterator to the new current
    item.

    Calling this function on QSequentialIterable::end() leads to undefined results.

    \sa operator--()
*/
QSequentialIterable::const_iterator &QSequentialIterable::const_iterator::operator++()
{
    m_impl.advance(1);
    return *this;
}

/*!
    \overload

    The postfix ++ operator (\c{it++}) advances the iterator to the
    next item in the container and returns an iterator to the previously
    current item.
*/
QSequentialIterable::const_iterator QSequentialIterable::const_iterator::operator++(int)
{
    QtMetaTypePrivate::QSequentialIterableImpl impl;
    impl.copy(m_impl);
    m_impl.advance(1);
    return const_iterator(impl, new QAtomicInt(0));
}

/*!
    The prefix -- operator (\c{--it}) makes the preceding item
    current and returns an iterator to the new current item.

    Calling this function on QSequentialIterable::begin() leads to undefined results.

    If the container in the QVariant does not support bi-directional iteration, calling this function
    leads to undefined results.

    \sa operator++(), canReverseIterate()
*/
QSequentialIterable::const_iterator &QSequentialIterable::const_iterator::operator--()
{
    m_impl.advance(-1);
    return *this;
}

/*!
    \overload

    The postfix -- operator (\c{it--}) makes the preceding item
    current and returns an iterator to the previously current item.

    If the container in the QVariant does not support bi-directional iteration, calling this function
    leads to undefined results.

    \sa canReverseIterate()
*/
QSequentialIterable::const_iterator QSequentialIterable::const_iterator::operator--(int)
{
    QtMetaTypePrivate::QSequentialIterableImpl impl;
    impl.copy(m_impl);
    m_impl.advance(-1);
    return const_iterator(impl, new QAtomicInt(0));
}

/*!
    Advances the iterator by \a j items.

    \sa operator-=(), operator+()
*/
QSequentialIterable::const_iterator &QSequentialIterable::const_iterator::operator+=(int j)
{
    m_impl.advance(j);
    return *this;
}

/*!
    Makes the iterator go back by \a j items.

    If the container in the QVariant does not support bi-directional iteration, calling this function
    leads to undefined results.

    \sa operator+=(), operator-(), canReverseIterate()
*/
QSequentialIterable::const_iterator &QSequentialIterable::const_iterator::operator-=(int j)
{
    m_impl.advance(-j);
    return *this;
}

/*!
    Returns an iterator to the item at \a j positions forward from
    this iterator.

    \sa operator-(), operator+=()
*/
QSequentialIterable::const_iterator QSequentialIterable::const_iterator::operator+(int j) const
{
    QtMetaTypePrivate::QSequentialIterableImpl impl;
    impl.copy(m_impl);
    impl.advance(j);
    return const_iterator(impl, new QAtomicInt(0));
}

/*!
    Returns an iterator to the item at \a j positions backward from
    this iterator.

    If the container in the QVariant does not support bi-directional iteration, calling this function
    leads to undefined results.

    \sa operator+(), operator-=(), canReverseIterate()
*/
QSequentialIterable::const_iterator QSequentialIterable::const_iterator::operator-(int j) const
{
    QtMetaTypePrivate::QSequentialIterableImpl impl;
    impl.copy(m_impl);
    impl.advance(-j);
    return const_iterator(impl, new QAtomicInt(0));
}

/*!
    \class QAssociativeIterable
    \since 5.2
    \inmodule QtCore
    \brief The QAssociativeIterable class is an iterable interface for an associative container in a QVariant.

    This class allows several methods of accessing the elements of an associative container held within
    a QVariant. An instance of QAssociativeIterable can be extracted from a QVariant if it can
    be converted to a QVariantHash or QVariantMap.

    \snippet code/src_corelib_kernel_qvariant.cpp 10

    The container itself is not copied before iterating over it.

    \sa QVariant
*/

/*!
    \internal
*/
QAssociativeIterable::QAssociativeIterable(QtMetaTypePrivate::QAssociativeIterableImpl impl)
  : m_impl(impl)
{
}

QAssociativeIterable::const_iterator::const_iterator(const QAssociativeIterable &iter, QAtomicInt *ref_)
  : m_impl(iter.m_impl), ref(ref_)
{
    ref->ref();
}

QAssociativeIterable::const_iterator::const_iterator(const QtMetaTypePrivate::QAssociativeIterableImpl &impl, QAtomicInt *ref_)
  : m_impl(impl), ref(ref_)
{
    ref->ref();
}

void QAssociativeIterable::const_iterator::begin()
{
    m_impl.begin();
}

void QAssociativeIterable::const_iterator::end()
{
    m_impl.end();
}

/*!
    Returns a QAssociativeIterable::const_iterator for the beginning of the container. This
    can be used in stl-style iteration.

    \sa end()
*/
QAssociativeIterable::const_iterator QAssociativeIterable::begin() const
{
    const_iterator it(*this, new QAtomicInt(0));
    it.begin();
    return it;
}

/*!
    Returns a QAssociativeIterable::const_iterator for the end of the container. This
    can be used in stl-style iteration.

    \sa begin()
*/
QAssociativeIterable::const_iterator QAssociativeIterable::end() const
{
    const_iterator it(*this, new QAtomicInt(0));
    it.end();
    return it;
}

/*!
    Returns the value for the given \a key in the container, if the types are convertible.
*/
QVariant QAssociativeIterable::value(const QVariant &key) const
{
    QVariant key_ = key;
    if (!key_.canConvert(m_impl._metaType_id_key))
        return QVariant();
    if (!key_.convert(m_impl._metaType_id_key))
        return QVariant();
    const QtMetaTypePrivate::VariantData dkey(key_.userType(), key_.constData(), 0 /*key.flags()*/);
    QtMetaTypePrivate::QAssociativeIterableImpl impl = m_impl;
    impl.find(dkey);
    QtMetaTypePrivate::QAssociativeIterableImpl endIt = m_impl;
    endIt.end();
    if (impl.equal(endIt))
        return QVariant();
    const QtMetaTypePrivate::VariantData d = impl.getCurrentValue();
    QVariant v(d.metaTypeId, d.data, d.flags);
    if (d.metaTypeId == qMetaTypeId<QVariant>())
        return *reinterpret_cast<const QVariant*>(d.data);
    return v;
}

/*!
    Returns the number of elements in the container.
*/
int QAssociativeIterable::size() const
{
    return m_impl.size();
}

/*!
    \class QAssociativeIterable::const_iterator
    \since 5.2
    \inmodule QtCore
    \brief The QAssociativeIterable::const_iterator allows iteration over a container in a QVariant.

    A QAssociativeIterable::const_iterator can only be created by a QAssociativeIterable instance,
    and can be used in a way similar to other stl-style iterators.

    \snippet code/src_corelib_kernel_qvariant.cpp 10

    \sa QAssociativeIterable
*/


/*!
    Destroys the QAssociativeIterable::const_iterator.
*/
QAssociativeIterable::const_iterator::~const_iterator()
{
    if (!ref->deref()) {
        m_impl.destroyIter();
        delete ref;
    }
}

/*!
    Creates a copy of \a other.
*/
QAssociativeIterable::const_iterator::const_iterator(const const_iterator &other)
  : m_impl(other.m_impl), ref(other.ref)
{
    ref->ref();
}

/*!
    Assigns \a other to this.
*/
QAssociativeIterable::const_iterator&
QAssociativeIterable::const_iterator::operator=(const const_iterator &other)
{
    if (!m_impl.equal(other.m_impl)) {
        m_impl = other.m_impl;
        ref = other.ref;
    }
    ref->ref();
    return *this;
}

/*!
    Returns the current value, converted to a QVariant.
*/
const QVariant QAssociativeIterable::const_iterator::operator*() const
{
    const QtMetaTypePrivate::VariantData d = m_impl.getCurrentValue();
    QVariant v(d.metaTypeId, d.data, d.flags);
    if (d.metaTypeId == qMetaTypeId<QVariant>())
        return *reinterpret_cast<const QVariant*>(d.data);
    return v;
}

/*!
    Returns the current key, converted to a QVariant.
*/
const QVariant QAssociativeIterable::const_iterator::key() const
{
    const QtMetaTypePrivate::VariantData d = m_impl.getCurrentKey();
    QVariant v(d.metaTypeId, d.data, d.flags);
    if (d.metaTypeId == qMetaTypeId<QVariant>())
        return *reinterpret_cast<const QVariant*>(d.data);
    return v;
}

/*!
    Returns the current value, converted to a QVariant.
*/
const QVariant QAssociativeIterable::const_iterator::value() const
{
    const QtMetaTypePrivate::VariantData d = m_impl.getCurrentValue();
    QVariant v(d.metaTypeId, d.data, d.flags);
    if (d.metaTypeId == qMetaTypeId<QVariant>())
        return *reinterpret_cast<const QVariant*>(d.data);
    return v;
}

/*!
    Returns \c true if \a other points to the same item as this
    iterator; otherwise returns \c false.

    \sa operator!=()
*/
bool QAssociativeIterable::const_iterator::operator==(const const_iterator &other) const
{
    return m_impl.equal(other.m_impl);
}

/*!
    Returns \c true if \a other points to a different item than this
    iterator; otherwise returns \c false.

    \sa operator==()
*/
bool QAssociativeIterable::const_iterator::operator!=(const const_iterator &other) const
{
    return !m_impl.equal(other.m_impl);
}

/*!
    The prefix ++ operator (\c{++it}) advances the iterator to the
    next item in the container and returns an iterator to the new current
    item.

    Calling this function on QAssociativeIterable::end() leads to undefined results.

    \sa operator--()
*/
QAssociativeIterable::const_iterator &QAssociativeIterable::const_iterator::operator++()
{
    m_impl.advance(1);
    return *this;
}

/*!
    \overload

    The postfix ++ operator (\c{it++}) advances the iterator to the
    next item in the container and returns an iterator to the previously
    current item.
*/
QAssociativeIterable::const_iterator QAssociativeIterable::const_iterator::operator++(int)
{
    QtMetaTypePrivate::QAssociativeIterableImpl impl;
    impl.copy(m_impl);
    m_impl.advance(1);
    return const_iterator(impl, new QAtomicInt(0));
}

/*!
    The prefix -- operator (\c{--it}) makes the preceding item
    current and returns an iterator to the new current item.

    Calling this function on QAssociativeIterable::begin() leads to undefined results.

    \sa operator++()
*/
QAssociativeIterable::const_iterator &QAssociativeIterable::const_iterator::operator--()
{
    m_impl.advance(-1);
    return *this;
}

/*!
    \overload

    The postfix -- operator (\c{it--}) makes the preceding item
    current and returns an iterator to the previously current item.
*/
QAssociativeIterable::const_iterator QAssociativeIterable::const_iterator::operator--(int)
{
    QtMetaTypePrivate::QAssociativeIterableImpl impl;
    impl.copy(m_impl);
    m_impl.advance(-1);
    return const_iterator(impl, new QAtomicInt(0));
}

/*!
    Advances the iterator by \a j items.

    \sa operator-=(), operator+()
*/
QAssociativeIterable::const_iterator &QAssociativeIterable::const_iterator::operator+=(int j)
{
    m_impl.advance(j);
    return *this;
}

/*!
    Makes the iterator go back by \a j items.

    \sa operator+=(), operator-()
*/
QAssociativeIterable::const_iterator &QAssociativeIterable::const_iterator::operator-=(int j)
{
    m_impl.advance(-j);
    return *this;
}

/*!
    Returns an iterator to the item at \a j positions forward from
    this iterator.

    \sa operator-(), operator+=()
*/
QAssociativeIterable::const_iterator QAssociativeIterable::const_iterator::operator+(int j) const
{
    QtMetaTypePrivate::QAssociativeIterableImpl impl;
    impl.copy(m_impl);
    impl.advance(j);
    return const_iterator(impl, new QAtomicInt(0));
}

/*!
    Returns an iterator to the item at \a j positions backward from
    this iterator.

    \sa operator+(), operator-=()
*/
QAssociativeIterable::const_iterator QAssociativeIterable::const_iterator::operator-(int j) const
{
    QtMetaTypePrivate::QAssociativeIterableImpl impl;
    impl.copy(m_impl);
    impl.advance(-j);
    return const_iterator(impl, new QAtomicInt(0));
}

QT_END_NAMESPACE
