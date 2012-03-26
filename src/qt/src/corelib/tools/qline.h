/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef QLINE_H
#define QLINE_H

#include <QtCore/qpoint.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Core)

/*******************************************************************************
 * class QLine
 *******************************************************************************/

class Q_CORE_EXPORT QLine
{
public:
    inline QLine();
    inline QLine(const QPoint &pt1, const QPoint &pt2);
    inline QLine(int x1, int y1, int x2, int y2);

    inline bool isNull() const;

    inline QPoint p1() const;
    inline QPoint p2() const;

    inline int x1() const;
    inline int y1() const;

    inline int x2() const;
    inline int y2() const;

    inline int dx() const;
    inline int dy() const;

    inline void translate(const QPoint &p);
    inline void translate(int dx, int dy);

    inline QLine translated(const QPoint &p) const;
    inline QLine translated(int dx, int dy) const;

    inline void setP1(const QPoint &p1);
    inline void setP2(const QPoint &p2);
    inline void setPoints(const QPoint &p1, const QPoint &p2);
    inline void setLine(int x1, int y1, int x2, int y2);

    inline bool operator==(const QLine &d) const;
    inline bool operator!=(const QLine &d) const { return !(*this == d); }

private:
    QPoint pt1, pt2;
};
Q_DECLARE_TYPEINFO(QLine, Q_MOVABLE_TYPE);

/*******************************************************************************
 * class QLine inline members
 *******************************************************************************/

inline QLine::QLine() { }

inline QLine::QLine(const QPoint &pt1_, const QPoint &pt2_) : pt1(pt1_), pt2(pt2_) { }

inline QLine::QLine(int x1pos, int y1pos, int x2pos, int y2pos) : pt1(QPoint(x1pos, y1pos)), pt2(QPoint(x2pos, y2pos)) { }

inline bool QLine::isNull() const
{
    return pt1 == pt2;
}

inline int QLine::x1() const
{
    return pt1.x();
}

inline int QLine::y1() const
{
    return pt1.y();
}

inline int QLine::x2() const
{
    return pt2.x();
}

inline int QLine::y2() const
{
    return pt2.y();
}

inline QPoint QLine::p1() const
{
    return pt1;
}

inline QPoint QLine::p2() const
{
    return pt2;
}

inline int QLine::dx() const
{
    return pt2.x() - pt1.x();
}

inline int QLine::dy() const
{
    return pt2.y() - pt1.y();
}

inline void QLine::translate(const QPoint &point)
{
    pt1 += point;
    pt2 += point;
}

inline void QLine::translate(int adx, int ady)
{
    this->translate(QPoint(adx, ady));
}

inline QLine QLine::translated(const QPoint &p) const
{
    return QLine(pt1 + p, pt2 + p);
}

inline QLine QLine::translated(int adx, int ady) const
{
    return translated(QPoint(adx, ady));
}

inline void QLine::setP1(const QPoint &aP1)
{
    pt1 = aP1;
}

inline void QLine::setP2(const QPoint &aP2)
{
    pt2 = aP2;
}

inline void QLine::setPoints(const QPoint &aP1, const QPoint &aP2)
{
    pt1 = aP1;
    pt2 = aP2;
}

inline void QLine::setLine(int aX1, int aY1, int aX2, int aY2)
{
    pt1 = QPoint(aX1, aY1);
    pt2 = QPoint(aX2, aY2);
}

inline bool QLine::operator==(const QLine &d) const
{
    return pt1 == d.pt1 && pt2 == d.pt2;
}

#ifndef QT_NO_DEBUG_STREAM
Q_CORE_EXPORT QDebug operator<<(QDebug d, const QLine &p);
#endif

#ifndef QT_NO_DATASTREAM
Q_CORE_EXPORT QDataStream &operator<<(QDataStream &, const QLine &);
Q_CORE_EXPORT QDataStream &operator>>(QDataStream &, QLine &);
#endif

/*******************************************************************************
 * class QLineF
 *******************************************************************************/
class Q_CORE_EXPORT QLineF {
public:

    enum IntersectType { NoIntersection, BoundedIntersection, UnboundedIntersection };

    inline QLineF();
    inline QLineF(const QPointF &pt1, const QPointF &pt2);
    inline QLineF(qreal x1, qreal y1, qreal x2, qreal y2);
    inline QLineF(const QLine &line) : pt1(line.p1()), pt2(line.p2()) { }

    static QLineF fromPolar(qreal length, qreal angle);

    bool isNull() const;

    inline QPointF p1() const;
    inline QPointF p2() const;

    inline qreal x1() const;
    inline qreal y1() const;

    inline qreal x2() const;
    inline qreal y2() const;

    inline qreal dx() const;
    inline qreal dy() const;

    qreal length() const;
    void setLength(qreal len);

    qreal angle() const;
    void setAngle(qreal angle);

    qreal angleTo(const QLineF &l) const;

    QLineF unitVector() const;
    QLineF normalVector() const;

    // ### Qt 5: rename intersects() or intersection() and rename IntersectType IntersectionType
    IntersectType intersect(const QLineF &l, QPointF *intersectionPoint) const;

    qreal angle(const QLineF &l) const;

    QPointF pointAt(qreal t) const;
    inline void translate(const QPointF &p);
    inline void translate(qreal dx, qreal dy);

    inline QLineF translated(const QPointF &p) const;
    inline QLineF translated(qreal dx, qreal dy) const;

    inline void setP1(const QPointF &p1);
    inline void setP2(const QPointF &p2);
    inline void setPoints(const QPointF &p1, const QPointF &p2);
    inline void setLine(qreal x1, qreal y1, qreal x2, qreal y2);

    inline bool operator==(const QLineF &d) const;
    inline bool operator!=(const QLineF &d) const { return !(*this == d); }

    QLine toLine() const;

private:
    QPointF pt1, pt2;
};
Q_DECLARE_TYPEINFO(QLineF, Q_MOVABLE_TYPE);

/*******************************************************************************
 * class QLineF inline members
 *******************************************************************************/

inline QLineF::QLineF()
{
}

inline QLineF::QLineF(const QPointF &apt1, const QPointF &apt2)
    : pt1(apt1), pt2(apt2)
{
}

inline QLineF::QLineF(qreal x1pos, qreal y1pos, qreal x2pos, qreal y2pos)
    : pt1(x1pos, y1pos), pt2(x2pos, y2pos)
{
}

inline qreal QLineF::x1() const
{
    return pt1.x();
}

inline qreal QLineF::y1() const
{
    return pt1.y();
}

inline qreal QLineF::x2() const
{
    return pt2.x();
}

inline qreal QLineF::y2() const
{
    return pt2.y();
}

inline QPointF QLineF::p1() const
{
    return pt1;
}

inline QPointF QLineF::p2() const
{
    return pt2;
}

inline qreal QLineF::dx() const
{
    return pt2.x() - pt1.x();
}

inline qreal QLineF::dy() const
{
    return pt2.y() - pt1.y();
}

inline QLineF QLineF::normalVector() const
{
    return QLineF(p1(), p1() + QPointF(dy(), -dx()));
}

inline void QLineF::translate(const QPointF &point)
{
    pt1 += point;
    pt2 += point;
}

inline void QLineF::translate(qreal adx, qreal ady)
{
    this->translate(QPointF(adx, ady));
}

inline QLineF QLineF::translated(const QPointF &p) const
{
    return QLineF(pt1 + p, pt2 + p);
}

inline QLineF QLineF::translated(qreal adx, qreal ady) const
{
    return translated(QPointF(adx, ady));
}

inline void QLineF::setLength(qreal len)
{
    if (isNull())
        return;
    QLineF v = unitVector();
    pt2 = QPointF(pt1.x() + v.dx() * len, pt1.y() + v.dy() * len);
}

inline QPointF QLineF::pointAt(qreal t) const
{
    qreal vx = pt2.x() - pt1.x();
    qreal vy = pt2.y() - pt1.y();
    return QPointF(pt1.x() + vx * t, pt1.y() + vy * t);
}

inline QLine QLineF::toLine() const
{
    return QLine(pt1.toPoint(), pt2.toPoint());
}


inline void QLineF::setP1(const QPointF &aP1)
{
    pt1 = aP1;
}

inline void QLineF::setP2(const QPointF &aP2)
{
    pt2 = aP2;
}

inline void QLineF::setPoints(const QPointF &aP1, const QPointF &aP2)
{
    pt1 = aP1;
    pt2 = aP2;
}

inline void QLineF::setLine(qreal aX1, qreal aY1, qreal aX2, qreal aY2)
{
    pt1 = QPointF(aX1, aY1);
    pt2 = QPointF(aX2, aY2);
}


inline bool QLineF::operator==(const QLineF &d) const
{
    return pt1 == d.pt1 && pt2 == d.pt2;
}



#ifndef QT_NO_DEBUG_STREAM
Q_CORE_EXPORT QDebug operator<<(QDebug d, const QLineF &p);
#endif

#ifndef QT_NO_DATASTREAM
Q_CORE_EXPORT QDataStream &operator<<(QDataStream &, const QLineF &);
Q_CORE_EXPORT QDataStream &operator>>(QDataStream &, QLineF &);
#endif

QT_END_NAMESPACE

QT_END_HEADER

#endif // QLINE_H
