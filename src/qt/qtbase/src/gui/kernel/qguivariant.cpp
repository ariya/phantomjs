/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtGui module of the Qt Toolkit.
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

// Gui types
#include "qbitmap.h"
#include "qbrush.h"
#include "qcolor.h"
#include "qcursor.h"
#include "qfont.h"
#include "qimage.h"
#include "qkeysequence.h"
#include "qtransform.h"
#include "qmatrix.h"
#include "qpalette.h"
#include "qpen.h"
#include "qpixmap.h"
#include "qpolygon.h"
#include "qregion.h"
#include "qtextformat.h"
#include "qmatrix4x4.h"
#include "qvector2d.h"
#include "qvector3d.h"
#include "qvector4d.h"
#include "qquaternion.h"
#include "qicon.h"

// Core types
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
#include "quuid.h"

#ifndef QT_NO_GEOM_VARIANT
#include "qsize.h"
#include "qpoint.h"
#include "qrect.h"
#include "qline.h"
#endif

#include <float.h>

#include "private/qvariant_p.h"
#include <private/qmetatype_p.h>

QT_BEGIN_NAMESPACE

Q_CORE_EXPORT const QVariant::Handler *qcoreVariantHandler();

namespace {
struct GuiTypesFilter {
    template<typename T>
    struct Acceptor {
        static const bool IsAccepted = QModulesPrivate::QTypeModuleInfo<T>::IsGui && QtMetaTypePrivate::TypeDefinition<T>::IsAvailable;
    };
};

static void construct(QVariant::Private *x, const void *copy)
{
    const int type = x->type;
    QVariantConstructor<GuiTypesFilter> constructor(x, copy);
    QMetaTypeSwitcher::switcher<void>(constructor, type, 0);
}

static void clear(QVariant::Private *d)
{
    QVariantDestructor<GuiTypesFilter> destructor(d);
    QMetaTypeSwitcher::switcher<void>(destructor, d->type, 0);
}

// This class is a hack that customizes access to QPolygon and QPolygonF
template<class Filter>
class QGuiVariantIsNull : public QVariantIsNull<Filter> {
    typedef QVariantIsNull<Filter> Base;
public:
    QGuiVariantIsNull(const QVariant::Private *d)
        : QVariantIsNull<Filter>(d)
    {}
    template<typename T>
    bool delegate(const T *p) { return Base::delegate(p); }
    bool delegate(const QPolygon*) { return v_cast<QPolygon>(Base::m_d)->isEmpty(); }
    bool delegate(const QPolygonF*) { return v_cast<QPolygonF>(Base::m_d)->isEmpty(); }
    bool delegate(const void *p) { return Base::delegate(p); }
};
static bool isNull(const QVariant::Private *d)
{
    QGuiVariantIsNull<GuiTypesFilter> isNull(d);
    return QMetaTypeSwitcher::switcher<bool>(isNull, d->type, 0);
}

// This class is a hack that customizes access to QPixmap, QBitmap, QCursor and QIcon
template<class Filter>
class QGuiVariantComparator : public QVariantComparator<Filter> {
    typedef QVariantComparator<Filter> Base;
public:
    QGuiVariantComparator(const QVariant::Private *a, const QVariant::Private *b)
        : QVariantComparator<Filter>(a, b)
    {}
    template<typename T>
    bool delegate(const T *p)
    {
        return Base::delegate(p);
    }
    bool delegate(const QPixmap*)
    {
        return v_cast<QPixmap>(Base::m_a)->cacheKey() == v_cast<QPixmap>(Base::m_b)->cacheKey();
    }
    bool delegate(const QBitmap*)
    {
        return v_cast<QBitmap>(Base::m_a)->cacheKey() == v_cast<QBitmap>(Base::m_b)->cacheKey();
    }
#ifndef QT_NO_CURSOR
    bool delegate(const QCursor*)
    {
        return v_cast<QCursor>(Base::m_a)->shape() == v_cast<QCursor>(Base::m_b)->shape();
    }
#endif
#ifndef QT_NO_ICON
    bool delegate(const QIcon *)
    {
        return v_cast<QIcon>(Base::m_a)->cacheKey() == v_cast<QIcon>(Base::m_b)->cacheKey();
    }
#endif
    bool delegate(const void *p) { return Base::delegate(p); }
};

static bool compare(const QVariant::Private *a, const QVariant::Private *b)
{
    QGuiVariantComparator<GuiTypesFilter> comparator(a, b);
    return QMetaTypeSwitcher::switcher<bool>(comparator, a->type, 0);
}

static bool convert(const QVariant::Private *d, int t,
                 void *result, bool *ok)
{
    switch (t) {
    case QVariant::ByteArray:
        if (d->type == QVariant::Color) {
            *static_cast<QByteArray *>(result) = v_cast<QColor>(d)->name().toLatin1();
            return true;
        }
        break;
    case QVariant::String: {
        QString *str = static_cast<QString *>(result);
        switch (d->type) {
#ifndef QT_NO_SHORTCUT
        case QVariant::KeySequence:
            *str = (*v_cast<QKeySequence>(d)).toString(QKeySequence::NativeText);
            return true;
#endif
        case QVariant::Font:
            *str = v_cast<QFont>(d)->toString();
            return true;
        case QVariant::Color:
            *str = v_cast<QColor>(d)->name();
            return true;
        default:
            break;
        }
        break;
    }
    case QVariant::Pixmap:
        if (d->type == QVariant::Image) {
            *static_cast<QPixmap *>(result) = QPixmap::fromImage(*v_cast<QImage>(d));
            return true;
        } else if (d->type == QVariant::Bitmap) {
            *static_cast<QPixmap *>(result) = *v_cast<QBitmap>(d);
            return true;
        } else if (d->type == QVariant::Brush) {
            if (v_cast<QBrush>(d)->style() == Qt::TexturePattern) {
                *static_cast<QPixmap *>(result) = v_cast<QBrush>(d)->texture();
                return true;
            }
        }
        break;
    case QVariant::Image:
        if (d->type == QVariant::Pixmap) {
            *static_cast<QImage *>(result) = v_cast<QPixmap>(d)->toImage();
            return true;
        } else if (d->type == QVariant::Bitmap) {
            *static_cast<QImage *>(result) = v_cast<QBitmap>(d)->toImage();
            return true;
        }
        break;
    case QVariant::Bitmap:
        if (d->type == QVariant::Pixmap) {
            *static_cast<QBitmap *>(result) = *v_cast<QPixmap>(d);
            return true;
        } else if (d->type == QVariant::Image) {
            *static_cast<QBitmap *>(result) = QBitmap::fromImage(*v_cast<QImage>(d));
            return true;
        }
        break;
#ifndef QT_NO_SHORTCUT
    case QVariant::Int:
        if (d->type == QVariant::KeySequence) {
            const QKeySequence &seq = *v_cast<QKeySequence>(d);
            *static_cast<int *>(result) = seq.isEmpty() ? 0 : seq[0];
            return true;
        }
        break;
#endif
    case QVariant::Font:
        if (d->type == QVariant::String) {
            QFont *f = static_cast<QFont *>(result);
            f->fromString(*v_cast<QString>(d));
            return true;
        }
        break;
    case QVariant::Color:
        if (d->type == QVariant::String) {
            static_cast<QColor *>(result)->setNamedColor(*v_cast<QString>(d));
            return static_cast<QColor *>(result)->isValid();
        } else if (d->type == QVariant::ByteArray) {
            static_cast<QColor *>(result)->setNamedColor(QString::fromLatin1(
                                *v_cast<QByteArray>(d)));
            return true;
        } else if (d->type == QVariant::Brush) {
            if (v_cast<QBrush>(d)->style() == Qt::SolidPattern) {
                *static_cast<QColor *>(result) = v_cast<QBrush>(d)->color();
                return true;
            }
        }
        break;
    case QVariant::Brush:
        if (d->type == QVariant::Color) {
            *static_cast<QBrush *>(result) = QBrush(*v_cast<QColor>(d));
            return true;
        } else if (d->type == QVariant::Pixmap) {
            *static_cast<QBrush *>(result) = QBrush(*v_cast<QPixmap>(d));
            return true;
        }
        break;
#ifndef QT_NO_SHORTCUT
    case QVariant::KeySequence: {
        QKeySequence *seq = static_cast<QKeySequence *>(result);
        switch (d->type) {
        case QVariant::String:
            *seq = QKeySequence(*v_cast<QString>(d));
            return true;
        case QVariant::Int:
            *seq = QKeySequence(d->data.i);
            return true;
        default:
            break;
        }
    }
#endif
#ifndef QT_NO_ICON
    case QVariant::Icon: {
        if (ok)
            *ok = false;
        return false;
    }
#endif
    default:
        break;
    }
    return qcoreVariantHandler()->convert(d, t, result, ok);
}

#if !defined(QT_NO_DEBUG_STREAM)
static void streamDebug(QDebug dbg, const QVariant &v)
{
    QVariant::Private *d = const_cast<QVariant::Private *>(&v.data_ptr());
    QVariantDebugStream<GuiTypesFilter> stream(dbg, d);
    QMetaTypeSwitcher::switcher<void>(stream, d->type, 0);
}
#endif

const QVariant::Handler qt_gui_variant_handler = {
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

#define QT_IMPL_METATYPEINTERFACE_GUI_TYPES(MetaTypeName, MetaTypeId, RealName) \
    QT_METATYPE_INTERFACE_INIT(RealName),

static const QMetaTypeInterface qVariantGuiHelper[] = {
    QT_FOR_EACH_STATIC_GUI_CLASS(QT_IMPL_METATYPEINTERFACE_GUI_TYPES)
};

#undef QT_IMPL_METATYPEINTERFACE_GUI_TYPES
} // namespace used to hide QVariant handler

extern Q_CORE_EXPORT const QMetaTypeInterface *qMetaTypeGuiHelper;

void qRegisterGuiVariant()
{
    QVariantPrivate::registerHandler(QModulesPrivate::Gui, &qt_gui_variant_handler);
    qMetaTypeGuiHelper = qVariantGuiHelper;
}
Q_CONSTRUCTOR_FUNCTION(qRegisterGuiVariant)

QT_END_NAMESPACE
