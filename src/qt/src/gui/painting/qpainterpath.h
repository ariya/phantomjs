/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
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

#ifndef QPAINTERPATH_H
#define QPAINTERPATH_H

#include <QtGui/qmatrix.h>
#include <QtCore/qglobal.h>
#include <QtCore/qrect.h>
#include <QtCore/qline.h>
#include <QtCore/qvector.h>
#include <QtCore/qscopedpointer.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Gui)

class QFont;
class QPainterPathPrivate;
struct QPainterPathPrivateDeleter;
class QPainterPathData;
class QPainterPathStrokerPrivate;
class QPolygonF;
class QRegion;
class QVectorPath;

class Q_GUI_EXPORT QPainterPath
{
public:
    enum ElementType {
        MoveToElement,
        LineToElement,
        CurveToElement,
        CurveToDataElement
    };

    class Element {
    public:
        qreal x;
        qreal y;
        ElementType type;

        bool isMoveTo() const { return type == MoveToElement; }
        bool isLineTo() const { return type == LineToElement; }
        bool isCurveTo() const { return type == CurveToElement; }

        operator QPointF () const { return QPointF(x, y); }

        bool operator==(const Element &e) const { return qFuzzyCompare(x, e.x)
            && qFuzzyCompare(y, e.y) && type == e.type; }
        inline bool operator!=(const Element &e) const { return !operator==(e); }
    };

    QPainterPath();
    explicit QPainterPath(const QPointF &startPoint);
    QPainterPath(const QPainterPath &other);
    QPainterPath &operator=(const QPainterPath &other);
#ifdef Q_COMPILER_RVALUE_REFS
    inline QPainterPath &operator=(QPainterPath &&other)
    { qSwap(d_ptr, other.d_ptr); return *this; }
#endif
    ~QPainterPath();
    inline void swap(QPainterPath &other) { d_ptr.swap(other.d_ptr); }

    void closeSubpath();

    void moveTo(const QPointF &p);
    inline void moveTo(qreal x, qreal y);

    void lineTo(const QPointF &p);
    inline void lineTo(qreal x, qreal y);

    void arcMoveTo(const QRectF &rect, qreal angle);
    inline void arcMoveTo(qreal x, qreal y, qreal w, qreal h, qreal angle);

    void arcTo(const QRectF &rect, qreal startAngle, qreal arcLength);
    inline void arcTo(qreal x, qreal y, qreal w, qreal h, qreal startAngle, qreal arcLength);

    void cubicTo(const QPointF &ctrlPt1, const QPointF &ctrlPt2, const QPointF &endPt);
    inline void cubicTo(qreal ctrlPt1x, qreal ctrlPt1y, qreal ctrlPt2x, qreal ctrlPt2y,
                        qreal endPtx, qreal endPty);
    void quadTo(const QPointF &ctrlPt, const QPointF &endPt);
    inline void quadTo(qreal ctrlPtx, qreal ctrlPty, qreal endPtx, qreal endPty);

    QPointF currentPosition() const;

    void addRect(const QRectF &rect);
    inline void addRect(qreal x, qreal y, qreal w, qreal h);
    void addEllipse(const QRectF &rect);
    inline void addEllipse(qreal x, qreal y, qreal w, qreal h);
    inline void addEllipse(const QPointF &center, qreal rx, qreal ry);
    void addPolygon(const QPolygonF &polygon);
    void addText(const QPointF &point, const QFont &f, const QString &text);
    inline void addText(qreal x, qreal y, const QFont &f, const QString &text);
    void addPath(const QPainterPath &path);
    void addRegion(const QRegion &region);

    void addRoundedRect(const QRectF &rect, qreal xRadius, qreal yRadius,
                        Qt::SizeMode mode = Qt::AbsoluteSize);
    inline void addRoundedRect(qreal x, qreal y, qreal w, qreal h,
                               qreal xRadius, qreal yRadius,
                               Qt::SizeMode mode = Qt::AbsoluteSize);

    void addRoundRect(const QRectF &rect, int xRnd, int yRnd);
    inline void addRoundRect(qreal x, qreal y, qreal w, qreal h,
                             int xRnd, int yRnd);
    inline void addRoundRect(const QRectF &rect, int roundness);
    inline void addRoundRect(qreal x, qreal y, qreal w, qreal h,
                             int roundness);

    void connectPath(const QPainterPath &path);

    bool contains(const QPointF &pt) const;
    bool contains(const QRectF &rect) const;
    bool intersects(const QRectF &rect) const;

    void translate(qreal dx, qreal dy);
    inline void translate(const QPointF &offset);

    QPainterPath translated(qreal dx, qreal dy) const;
    inline QPainterPath translated(const QPointF &offset) const;

    QRectF boundingRect() const;
    QRectF controlPointRect() const;

    Qt::FillRule fillRule() const;
    void setFillRule(Qt::FillRule fillRule);

    inline bool isEmpty() const;

    QPainterPath toReversed() const;
    QList<QPolygonF> toSubpathPolygons(const QMatrix &matrix = QMatrix()) const;
    QList<QPolygonF> toFillPolygons(const QMatrix &matrix = QMatrix()) const;
    QPolygonF toFillPolygon(const QMatrix &matrix = QMatrix()) const;
    QList<QPolygonF> toSubpathPolygons(const QTransform &matrix) const;
    QList<QPolygonF> toFillPolygons(const QTransform &matrix) const;
    QPolygonF toFillPolygon(const QTransform &matrix) const;

    inline int elementCount() const;
    inline const QPainterPath::Element &elementAt(int i) const;
    inline void setElementPositionAt(int i, qreal x, qreal y);

    qreal   length() const;
    qreal   percentAtLength(qreal t) const;
    QPointF pointAtPercent(qreal t) const;
    qreal   angleAtPercent(qreal t) const;
    qreal   slopeAtPercent(qreal t) const;

    bool intersects(const QPainterPath &p) const;
    bool contains(const QPainterPath &p) const;
    QPainterPath united(const QPainterPath &r) const;
    QPainterPath intersected(const QPainterPath &r) const;
    QPainterPath subtracted(const QPainterPath &r) const;
    QPainterPath subtractedInverted(const QPainterPath &r) const;

    QPainterPath simplified() const;

    bool operator==(const QPainterPath &other) const;
    bool operator!=(const QPainterPath &other) const;

    QPainterPath operator&(const QPainterPath &other) const;
    QPainterPath operator|(const QPainterPath &other) const;
    QPainterPath operator+(const QPainterPath &other) const;
    QPainterPath operator-(const QPainterPath &other) const;
    QPainterPath &operator&=(const QPainterPath &other);
    QPainterPath &operator|=(const QPainterPath &other);
    QPainterPath &operator+=(const QPainterPath &other);
    QPainterPath &operator-=(const QPainterPath &other);

private:
    QScopedPointer<QPainterPathPrivate, QPainterPathPrivateDeleter> d_ptr;

    inline void ensureData() { if (!d_ptr) ensureData_helper(); }
    void ensureData_helper();
    inline void detach();
    void detach_helper();
    void setDirty(bool);
    void computeBoundingRect() const;
    void computeControlPointRect() const;

    QPainterPathData *d_func() const { return reinterpret_cast<QPainterPathData *>(d_ptr.data()); }

    friend class QPainterPathData;
    friend class QPainterPathStroker;
    friend class QPainterPathStrokerPrivate;
    friend class QMatrix;
    friend class QTransform;
    friend class QVectorPath;
    friend Q_GUI_EXPORT const QVectorPath &qtVectorPathForPath(const QPainterPath &);

#ifndef QT_NO_DATASTREAM
    friend Q_GUI_EXPORT QDataStream &operator<<(QDataStream &, const QPainterPath &);
    friend Q_GUI_EXPORT QDataStream &operator>>(QDataStream &, QPainterPath &);
#endif
};

class QPainterPathPrivate
{
public:
    friend class QPainterPath;
    friend class QPainterPathData;
    friend class QPainterPathStroker;
    friend class QPainterPathStrokerPrivate;
    friend class QMatrix;
    friend class QTransform;
    friend class QVectorPath;
    friend struct QPainterPathPrivateDeleter;
#ifndef QT_NO_DATASTREAM
    friend Q_GUI_EXPORT QDataStream &operator<<(QDataStream &, const QPainterPath &);
    friend Q_GUI_EXPORT QDataStream &operator>>(QDataStream &, QPainterPath &);
#endif
private:
    QAtomicInt ref;
    QVector<QPainterPath::Element> elements;
};

Q_DECLARE_TYPEINFO(QPainterPath::Element, Q_PRIMITIVE_TYPE);

#ifndef QT_NO_DATASTREAM
Q_GUI_EXPORT QDataStream &operator<<(QDataStream &, const QPainterPath &);
Q_GUI_EXPORT QDataStream &operator>>(QDataStream &, QPainterPath &);
#endif

class Q_GUI_EXPORT QPainterPathStroker
{
    Q_DECLARE_PRIVATE(QPainterPathStroker)
public:
    QPainterPathStroker();
    ~QPainterPathStroker();

    void setWidth(qreal width);
    qreal width() const;

    void setCapStyle(Qt::PenCapStyle style);
    Qt::PenCapStyle capStyle() const;

    void setJoinStyle(Qt::PenJoinStyle style);
    Qt::PenJoinStyle joinStyle() const;

    void setMiterLimit(qreal length);
    qreal miterLimit() const;

    void setCurveThreshold(qreal threshold);
    qreal curveThreshold() const;

    void setDashPattern(Qt::PenStyle);
    void setDashPattern(const QVector<qreal> &dashPattern);
    QVector<qreal> dashPattern() const;

    void setDashOffset(qreal offset);
    qreal dashOffset() const;

    QPainterPath createStroke(const QPainterPath &path) const;

private:
    Q_DISABLE_COPY(QPainterPathStroker)

    friend class QX11PaintEngine;

    QScopedPointer<QPainterPathStrokerPrivate> d_ptr;
};

inline void QPainterPath::moveTo(qreal x, qreal y)
{
    moveTo(QPointF(x, y));
}

inline void QPainterPath::lineTo(qreal x, qreal y)
{
    lineTo(QPointF(x, y));
}

inline void QPainterPath::arcTo(qreal x, qreal y, qreal w, qreal h, qreal startAngle, qreal arcLength)
{
    arcTo(QRectF(x, y, w, h), startAngle, arcLength);
}

inline void QPainterPath::arcMoveTo(qreal x, qreal y, qreal w, qreal h, qreal angle)
{
    arcMoveTo(QRectF(x, y, w, h), angle);
}

inline void QPainterPath::cubicTo(qreal ctrlPt1x, qreal ctrlPt1y, qreal ctrlPt2x, qreal ctrlPt2y,
                                   qreal endPtx, qreal endPty)
{
    cubicTo(QPointF(ctrlPt1x, ctrlPt1y), QPointF(ctrlPt2x, ctrlPt2y),
            QPointF(endPtx, endPty));
}

inline void QPainterPath::quadTo(qreal ctrlPtx, qreal ctrlPty, qreal endPtx, qreal endPty)
{
    quadTo(QPointF(ctrlPtx, ctrlPty), QPointF(endPtx, endPty));
}

inline void QPainterPath::addEllipse(qreal x, qreal y, qreal w, qreal h)
{
    addEllipse(QRectF(x, y, w, h));
}

inline void QPainterPath::addEllipse(const QPointF &center, qreal rx, qreal ry)
{
    addEllipse(QRectF(center.x() - rx, center.y() - ry, 2 * rx, 2 * ry));
}

inline void QPainterPath::addRect(qreal x, qreal y, qreal w, qreal h)
{
    addRect(QRectF(x, y, w, h));
}

inline void QPainterPath::addRoundedRect(qreal x, qreal y, qreal w, qreal h,
                                         qreal xRadius, qreal yRadius,
                                         Qt::SizeMode mode)
{
    addRoundedRect(QRectF(x, y, w, h), xRadius, yRadius, mode);
}

inline void QPainterPath::addRoundRect(qreal x, qreal y, qreal w, qreal h,
                                       int xRnd, int yRnd)
{
    addRoundRect(QRectF(x, y, w, h), xRnd, yRnd);
}

inline void QPainterPath::addRoundRect(const QRectF &rect,
                                       int roundness)
{
    int xRnd = roundness;
    int yRnd = roundness;
    if (rect.width() > rect.height())
        xRnd = int(roundness * rect.height()/rect.width());
    else
        yRnd = int(roundness * rect.width()/rect.height());
    addRoundRect(rect, xRnd, yRnd);
}

inline void QPainterPath::addRoundRect(qreal x, qreal y, qreal w, qreal h,
                                       int roundness)
{
    addRoundRect(QRectF(x, y, w, h), roundness);
}

inline void QPainterPath::addText(qreal x, qreal y, const QFont &f, const QString &text)
{
    addText(QPointF(x, y), f, text);
}

inline void QPainterPath::translate(const QPointF &offset)
{ translate(offset.x(), offset.y()); }

inline QPainterPath QPainterPath::translated(const QPointF &offset) const
{ return translated(offset.x(), offset.y()); }

inline bool QPainterPath::isEmpty() const
{
    return !d_ptr || (d_ptr->elements.size() == 1 && d_ptr->elements.first().type == MoveToElement);
}

inline int QPainterPath::elementCount() const
{
    return d_ptr ? d_ptr->elements.size() : 0;
}

inline const QPainterPath::Element &QPainterPath::elementAt(int i) const
{
    Q_ASSERT(d_ptr);
    Q_ASSERT(i >= 0 && i < elementCount());
    return d_ptr->elements.at(i);
}

inline void QPainterPath::setElementPositionAt(int i, qreal x, qreal y)
{
    Q_ASSERT(d_ptr);
    Q_ASSERT(i >= 0 && i < elementCount());
    detach();
    QPainterPath::Element &e = d_ptr->elements[i];
    e.x = x;
    e.y = y;
}


inline void QPainterPath::detach()
{
    if (d_ptr->ref != 1)
        detach_helper();
    setDirty(true);
}

#ifndef QT_NO_DEBUG_STREAM
Q_GUI_EXPORT QDebug operator<<(QDebug, const QPainterPath &);
#endif

QT_END_NAMESPACE

QT_END_HEADER

#endif // QPAINTERPATH_H
