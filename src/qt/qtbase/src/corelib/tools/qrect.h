/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia. For licensing terms and
** conditions see http://qt.digia.com/licensing. For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights. These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QRECT_H
#define QRECT_H

#include <QtCore/qmargins.h>
#include <QtCore/qsize.h>
#include <QtCore/qpoint.h>

#ifdef topLeft
#error qrect.h must be included before any header file that defines topLeft
#endif

QT_BEGIN_NAMESPACE

class Q_CORE_EXPORT QRect
{
public:
    Q_DECL_CONSTEXPR QRect() : x1(0), y1(0), x2(-1), y2(-1) {}
    Q_DECL_CONSTEXPR QRect(const QPoint &topleft, const QPoint &bottomright);
    Q_DECL_CONSTEXPR QRect(const QPoint &topleft, const QSize &size);
    Q_DECL_CONSTEXPR QRect(int left, int top, int width, int height);

    Q_DECL_CONSTEXPR inline bool isNull() const;
    Q_DECL_CONSTEXPR inline bool isEmpty() const;
    Q_DECL_CONSTEXPR inline bool isValid() const;

    Q_DECL_CONSTEXPR inline int left() const;
    Q_DECL_CONSTEXPR inline int top() const;
    Q_DECL_CONSTEXPR inline int right() const;
    Q_DECL_CONSTEXPR inline int bottom() const;
    QRect normalized() const Q_REQUIRED_RESULT;

    Q_DECL_CONSTEXPR inline int x() const;
    Q_DECL_CONSTEXPR inline int y() const;
    inline void setLeft(int pos);
    inline void setTop(int pos);
    inline void setRight(int pos);
    inline void setBottom(int pos);
    inline void setX(int x);
    inline void setY(int y);

    inline void setTopLeft(const QPoint &p);
    inline void setBottomRight(const QPoint &p);
    inline void setTopRight(const QPoint &p);
    inline void setBottomLeft(const QPoint &p);

    Q_DECL_CONSTEXPR inline QPoint topLeft() const;
    Q_DECL_CONSTEXPR inline QPoint bottomRight() const;
    Q_DECL_CONSTEXPR inline QPoint topRight() const;
    Q_DECL_CONSTEXPR inline QPoint bottomLeft() const;
    Q_DECL_CONSTEXPR inline QPoint center() const;

    inline void moveLeft(int pos);
    inline void moveTop(int pos);
    inline void moveRight(int pos);
    inline void moveBottom(int pos);
    inline void moveTopLeft(const QPoint &p);
    inline void moveBottomRight(const QPoint &p);
    inline void moveTopRight(const QPoint &p);
    inline void moveBottomLeft(const QPoint &p);
    inline void moveCenter(const QPoint &p);

    inline void translate(int dx, int dy);
    inline void translate(const QPoint &p);
    Q_DECL_CONSTEXPR inline QRect translated(int dx, int dy) const Q_REQUIRED_RESULT;
    Q_DECL_CONSTEXPR inline QRect translated(const QPoint &p) const Q_REQUIRED_RESULT;

    inline void moveTo(int x, int t);
    inline void moveTo(const QPoint &p);

    inline void setRect(int x, int y, int w, int h);
    inline void getRect(int *x, int *y, int *w, int *h) const;

    inline void setCoords(int x1, int y1, int x2, int y2);
    inline void getCoords(int *x1, int *y1, int *x2, int *y2) const;

    inline void adjust(int x1, int y1, int x2, int y2);
    Q_DECL_CONSTEXPR inline QRect adjusted(int x1, int y1, int x2, int y2) const Q_REQUIRED_RESULT;

    Q_DECL_CONSTEXPR inline QSize size() const;
    Q_DECL_CONSTEXPR inline int width() const;
    Q_DECL_CONSTEXPR inline int height() const;
    inline void setWidth(int w);
    inline void setHeight(int h);
    inline void setSize(const QSize &s);

    QRect operator|(const QRect &r) const;
    QRect operator&(const QRect &r) const;
    inline QRect& operator|=(const QRect &r);
    inline QRect& operator&=(const QRect &r);

    bool contains(const QRect &r, bool proper = false) const;
    bool contains(const QPoint &p, bool proper=false) const;
    inline bool contains(int x, int y) const;
    inline bool contains(int x, int y, bool proper) const;
    inline QRect united(const QRect &other) const Q_REQUIRED_RESULT;
    inline QRect intersected(const QRect &other) const Q_REQUIRED_RESULT;
    bool intersects(const QRect &r) const;

    Q_DECL_CONSTEXPR inline QRect marginsAdded(const QMargins &margins) const;
    Q_DECL_CONSTEXPR inline QRect marginsRemoved(const QMargins &margins) const;
    inline QRect &operator+=(const QMargins &margins);
    inline QRect &operator-=(const QMargins &margins);

#if QT_DEPRECATED_SINCE(5, 0)
    QT_DEPRECATED QRect unite(const QRect &r) const Q_REQUIRED_RESULT { return united(r); }
    QT_DEPRECATED QRect intersect(const QRect &r) const Q_REQUIRED_RESULT { return intersected(r); }
#endif

    friend Q_DECL_CONSTEXPR inline bool operator==(const QRect &, const QRect &);
    friend Q_DECL_CONSTEXPR inline bool operator!=(const QRect &, const QRect &);

private:
    int x1;
    int y1;
    int x2;
    int y2;
};
Q_DECLARE_TYPEINFO(QRect, Q_MOVABLE_TYPE);

Q_DECL_CONSTEXPR inline bool operator==(const QRect &, const QRect &);
Q_DECL_CONSTEXPR inline bool operator!=(const QRect &, const QRect &);


/*****************************************************************************
  QRect stream functions
 *****************************************************************************/
#ifndef QT_NO_DATASTREAM
Q_CORE_EXPORT QDataStream &operator<<(QDataStream &, const QRect &);
Q_CORE_EXPORT QDataStream &operator>>(QDataStream &, QRect &);
#endif

/*****************************************************************************
  QRect inline member functions
 *****************************************************************************/

Q_DECL_CONSTEXPR inline QRect::QRect(int aleft, int atop, int awidth, int aheight)
    : x1(aleft), y1(atop), x2(aleft + awidth - 1), y2(atop + aheight - 1) {}

Q_DECL_CONSTEXPR inline QRect::QRect(const QPoint &atopLeft, const QPoint &abottomRight)
    : x1(atopLeft.x()), y1(atopLeft.y()), x2(abottomRight.x()), y2(abottomRight.y()) {}

Q_DECL_CONSTEXPR inline QRect::QRect(const QPoint &atopLeft, const QSize &asize)
    : x1(atopLeft.x()), y1(atopLeft.y()), x2(atopLeft.x()+asize.width() - 1), y2(atopLeft.y()+asize.height() - 1) {}

Q_DECL_CONSTEXPR inline bool QRect::isNull() const
{ return x2 == x1 - 1 && y2 == y1 - 1; }

Q_DECL_CONSTEXPR inline bool QRect::isEmpty() const
{ return x1 > x2 || y1 > y2; }

Q_DECL_CONSTEXPR inline bool QRect::isValid() const
{ return x1 <= x2 && y1 <= y2; }

Q_DECL_CONSTEXPR inline int QRect::left() const
{ return x1; }

Q_DECL_CONSTEXPR inline int QRect::top() const
{ return y1; }

Q_DECL_CONSTEXPR inline int QRect::right() const
{ return x2; }

Q_DECL_CONSTEXPR inline int QRect::bottom() const
{ return y2; }

Q_DECL_CONSTEXPR inline int QRect::x() const
{ return x1; }

Q_DECL_CONSTEXPR inline int QRect::y() const
{ return y1; }

inline void QRect::setLeft(int pos)
{ x1 = pos; }

inline void QRect::setTop(int pos)
{ y1 = pos; }

inline void QRect::setRight(int pos)
{ x2 = pos; }

inline void QRect::setBottom(int pos)
{ y2 = pos; }

inline void QRect::setTopLeft(const QPoint &p)
{ x1 = p.x(); y1 = p.y(); }

inline void QRect::setBottomRight(const QPoint &p)
{ x2 = p.x(); y2 = p.y(); }

inline void QRect::setTopRight(const QPoint &p)
{ x2 = p.x(); y1 = p.y(); }

inline void QRect::setBottomLeft(const QPoint &p)
{ x1 = p.x(); y2 = p.y(); }

inline void QRect::setX(int ax)
{ x1 = ax; }

inline void QRect::setY(int ay)
{ y1 = ay; }

Q_DECL_CONSTEXPR inline QPoint QRect::topLeft() const
{ return QPoint(x1, y1); }

Q_DECL_CONSTEXPR inline QPoint QRect::bottomRight() const
{ return QPoint(x2, y2); }

Q_DECL_CONSTEXPR inline QPoint QRect::topRight() const
{ return QPoint(x2, y1); }

Q_DECL_CONSTEXPR inline QPoint QRect::bottomLeft() const
{ return QPoint(x1, y2); }

Q_DECL_CONSTEXPR inline QPoint QRect::center() const
{ return QPoint((x1+x2)/2, (y1+y2)/2); }

Q_DECL_CONSTEXPR inline int QRect::width() const
{ return  x2 - x1 + 1; }

Q_DECL_CONSTEXPR inline int QRect::height() const
{ return  y2 - y1 + 1; }

Q_DECL_CONSTEXPR inline QSize QRect::size() const
{ return QSize(width(), height()); }

inline void QRect::translate(int dx, int dy)
{
    x1 += dx;
    y1 += dy;
    x2 += dx;
    y2 += dy;
}

inline void QRect::translate(const QPoint &p)
{
    x1 += p.x();
    y1 += p.y();
    x2 += p.x();
    y2 += p.y();
}

Q_DECL_CONSTEXPR inline QRect QRect::translated(int dx, int dy) const
{ return QRect(QPoint(x1 + dx, y1 + dy), QPoint(x2 + dx, y2 + dy)); }

Q_DECL_CONSTEXPR inline QRect QRect::translated(const QPoint &p) const
{ return QRect(QPoint(x1 + p.x(), y1 + p.y()), QPoint(x2 + p.x(), y2 + p.y())); }

inline void QRect::moveTo(int ax, int ay)
{
    x2 += ax - x1;
    y2 += ay - y1;
    x1 = ax;
    y1 = ay;
}

inline void QRect::moveTo(const QPoint &p)
{
    x2 += p.x() - x1;
    y2 += p.y() - y1;
    x1 = p.x();
    y1 = p.y();
}

inline void QRect::moveLeft(int pos)
{ x2 += (pos - x1); x1 = pos; }

inline void QRect::moveTop(int pos)
{ y2 += (pos - y1); y1 = pos; }

inline void QRect::moveRight(int pos)
{
    x1 += (pos - x2);
    x2 = pos;
}

inline void QRect::moveBottom(int pos)
{
    y1 += (pos - y2);
    y2 = pos;
}

inline void QRect::moveTopLeft(const QPoint &p)
{
    moveLeft(p.x());
    moveTop(p.y());
}

inline void QRect::moveBottomRight(const QPoint &p)
{
    moveRight(p.x());
    moveBottom(p.y());
}

inline void QRect::moveTopRight(const QPoint &p)
{
    moveRight(p.x());
    moveTop(p.y());
}

inline void QRect::moveBottomLeft(const QPoint &p)
{
    moveLeft(p.x());
    moveBottom(p.y());
}

inline void QRect::moveCenter(const QPoint &p)
{
    int w = x2 - x1;
    int h = y2 - y1;
    x1 = p.x() - w/2;
    y1 = p.y() - h/2;
    x2 = x1 + w;
    y2 = y1 + h;
}

inline void QRect::getRect(int *ax, int *ay, int *aw, int *ah) const
{
    *ax = x1;
    *ay = y1;
    *aw = x2 - x1 + 1;
    *ah = y2 - y1 + 1;
}

inline void QRect::setRect(int ax, int ay, int aw, int ah)
{
    x1 = ax;
    y1 = ay;
    x2 = (ax + aw - 1);
    y2 = (ay + ah - 1);
}

inline void QRect::getCoords(int *xp1, int *yp1, int *xp2, int *yp2) const
{
    *xp1 = x1;
    *yp1 = y1;
    *xp2 = x2;
    *yp2 = y2;
}

inline void QRect::setCoords(int xp1, int yp1, int xp2, int yp2)
{
    x1 = xp1;
    y1 = yp1;
    x2 = xp2;
    y2 = yp2;
}

Q_DECL_CONSTEXPR inline QRect QRect::adjusted(int xp1, int yp1, int xp2, int yp2) const
{ return QRect(QPoint(x1 + xp1, y1 + yp1), QPoint(x2 + xp2, y2 + yp2)); }

inline void QRect::adjust(int dx1, int dy1, int dx2, int dy2)
{
    x1 += dx1;
    y1 += dy1;
    x2 += dx2;
    y2 += dy2;
}

inline void QRect::setWidth(int w)
{ x2 = (x1 + w - 1); }

inline void QRect::setHeight(int h)
{ y2 = (y1 + h - 1); }

inline void QRect::setSize(const QSize &s)
{
    x2 = (s.width()  + x1 - 1);
    y2 = (s.height() + y1 - 1);
}

inline bool QRect::contains(int ax, int ay, bool aproper) const
{
    return contains(QPoint(ax, ay), aproper);
}

inline bool QRect::contains(int ax, int ay) const
{
    return contains(QPoint(ax, ay), false);
}

inline QRect& QRect::operator|=(const QRect &r)
{
    *this = *this | r;
    return *this;
}

inline QRect& QRect::operator&=(const QRect &r)
{
    *this = *this & r;
    return *this;
}

inline QRect QRect::intersected(const QRect &other) const
{
    return *this & other;
}

inline QRect QRect::united(const QRect &r) const
{
    return *this | r;
}

Q_DECL_CONSTEXPR inline bool operator==(const QRect &r1, const QRect &r2)
{
    return r1.x1==r2.x1 && r1.x2==r2.x2 && r1.y1==r2.y1 && r1.y2==r2.y2;
}

Q_DECL_CONSTEXPR inline bool operator!=(const QRect &r1, const QRect &r2)
{
    return r1.x1!=r2.x1 || r1.x2!=r2.x2 || r1.y1!=r2.y1 || r1.y2!=r2.y2;
}

Q_DECL_CONSTEXPR inline QRect operator+(const QRect &rectangle, const QMargins &margins)
{
    return QRect(QPoint(rectangle.left() - margins.left(), rectangle.top() - margins.top()),
                 QPoint(rectangle.right() + margins.right(), rectangle.bottom() + margins.bottom()));
}

Q_DECL_CONSTEXPR inline QRect operator+(const QMargins &margins, const QRect &rectangle)
{
    return QRect(QPoint(rectangle.left() - margins.left(), rectangle.top() - margins.top()),
                 QPoint(rectangle.right() + margins.right(), rectangle.bottom() + margins.bottom()));
}

Q_DECL_CONSTEXPR inline QRect operator-(const QRect &lhs, const QMargins &rhs)
{
    return QRect(QPoint(lhs.left() + rhs.left(), lhs.top() + rhs.top()),
                 QPoint(lhs.right() - rhs.right(), lhs.bottom() - rhs.bottom()));
}

Q_DECL_CONSTEXPR inline QRect QRect::marginsAdded(const QMargins &margins) const
{
    return QRect(QPoint(x1 - margins.left(), y1 - margins.top()),
                 QPoint(x2 + margins.right(), y2 + margins.bottom()));
}

Q_DECL_CONSTEXPR inline QRect QRect::marginsRemoved(const QMargins &margins) const
{
    return QRect(QPoint(x1 + margins.left(), y1 + margins.top()),
                 QPoint(x2 - margins.right(), y2 - margins.bottom()));
}

inline QRect &QRect::operator+=(const QMargins &margins)
{
    *this = marginsAdded(margins);
    return *this;
}

inline QRect &QRect::operator-=(const QMargins &margins)
{
    *this = marginsRemoved(margins);
    return *this;
}

#ifndef QT_NO_DEBUG_STREAM
Q_CORE_EXPORT QDebug operator<<(QDebug, const QRect &);
#endif


class Q_CORE_EXPORT QRectF
{
public:
    Q_DECL_CONSTEXPR QRectF() : xp(0.), yp(0.), w(0.), h(0.) {}
    Q_DECL_CONSTEXPR QRectF(const QPointF &topleft, const QSizeF &size);
    Q_DECL_CONSTEXPR QRectF(const QPointF &topleft, const QPointF &bottomRight);
    Q_DECL_CONSTEXPR QRectF(qreal left, qreal top, qreal width, qreal height);
    Q_DECL_CONSTEXPR QRectF(const QRect &rect);

    Q_DECL_CONSTEXPR inline bool isNull() const;
    Q_DECL_CONSTEXPR inline bool isEmpty() const;
    Q_DECL_CONSTEXPR inline bool isValid() const;
    QRectF normalized() const Q_REQUIRED_RESULT;

    Q_DECL_CONSTEXPR inline qreal left() const { return xp; }
    Q_DECL_CONSTEXPR inline qreal top() const { return yp; }
    Q_DECL_CONSTEXPR inline qreal right() const { return xp + w; }
    Q_DECL_CONSTEXPR inline qreal bottom() const { return yp + h; }

    Q_DECL_CONSTEXPR inline qreal x() const;
    Q_DECL_CONSTEXPR inline qreal y() const;
    inline void setLeft(qreal pos);
    inline void setTop(qreal pos);
    inline void setRight(qreal pos);
    inline void setBottom(qreal pos);
    inline void setX(qreal pos) { setLeft(pos); }
    inline void setY(qreal pos) { setTop(pos); }

    Q_DECL_CONSTEXPR inline QPointF topLeft() const { return QPointF(xp, yp); }
    Q_DECL_CONSTEXPR inline QPointF bottomRight() const { return QPointF(xp+w, yp+h); }
    Q_DECL_CONSTEXPR inline QPointF topRight() const { return QPointF(xp+w, yp); }
    Q_DECL_CONSTEXPR inline QPointF bottomLeft() const { return QPointF(xp, yp+h); }
    Q_DECL_CONSTEXPR inline QPointF center() const;

    inline void setTopLeft(const QPointF &p);
    inline void setBottomRight(const QPointF &p);
    inline void setTopRight(const QPointF &p);
    inline void setBottomLeft(const QPointF &p);

    inline void moveLeft(qreal pos);
    inline void moveTop(qreal pos);
    inline void moveRight(qreal pos);
    inline void moveBottom(qreal pos);
    inline void moveTopLeft(const QPointF &p);
    inline void moveBottomRight(const QPointF &p);
    inline void moveTopRight(const QPointF &p);
    inline void moveBottomLeft(const QPointF &p);
    inline void moveCenter(const QPointF &p);

    inline void translate(qreal dx, qreal dy);
    inline void translate(const QPointF &p);

    Q_DECL_CONSTEXPR inline QRectF translated(qreal dx, qreal dy) const Q_REQUIRED_RESULT;
    Q_DECL_CONSTEXPR inline QRectF translated(const QPointF &p) const Q_REQUIRED_RESULT;

    inline void moveTo(qreal x, qreal y);
    inline void moveTo(const QPointF &p);

    inline void setRect(qreal x, qreal y, qreal w, qreal h);
    inline void getRect(qreal *x, qreal *y, qreal *w, qreal *h) const;

    inline void setCoords(qreal x1, qreal y1, qreal x2, qreal y2);
    inline void getCoords(qreal *x1, qreal *y1, qreal *x2, qreal *y2) const;

    inline void adjust(qreal x1, qreal y1, qreal x2, qreal y2);
    Q_DECL_CONSTEXPR inline QRectF adjusted(qreal x1, qreal y1, qreal x2, qreal y2) const Q_REQUIRED_RESULT;

    Q_DECL_CONSTEXPR inline QSizeF size() const;
    Q_DECL_CONSTEXPR inline qreal width() const;
    Q_DECL_CONSTEXPR inline qreal height() const;
    inline void setWidth(qreal w);
    inline void setHeight(qreal h);
    inline void setSize(const QSizeF &s);

    QRectF operator|(const QRectF &r) const;
    QRectF operator&(const QRectF &r) const;
    inline QRectF& operator|=(const QRectF &r);
    inline QRectF& operator&=(const QRectF &r);

    bool contains(const QRectF &r) const;
    bool contains(const QPointF &p) const;
    inline bool contains(qreal x, qreal y) const;
    inline QRectF united(const QRectF &other) const Q_REQUIRED_RESULT;
    inline QRectF intersected(const QRectF &other) const Q_REQUIRED_RESULT;
    bool intersects(const QRectF &r) const;

    Q_DECL_CONSTEXPR inline QRectF marginsAdded(const QMarginsF &margins) const;
    Q_DECL_CONSTEXPR inline QRectF marginsRemoved(const QMarginsF &margins) const;
    inline QRectF &operator+=(const QMarginsF &margins);
    inline QRectF &operator-=(const QMarginsF &margins);

#if QT_DEPRECATED_SINCE(5, 0)
    QT_DEPRECATED QRectF unite(const QRectF &r) const Q_REQUIRED_RESULT { return united(r); }
    QT_DEPRECATED QRectF intersect(const QRectF &r) const Q_REQUIRED_RESULT { return intersected(r); }
#endif

    friend Q_DECL_CONSTEXPR inline bool operator==(const QRectF &, const QRectF &);
    friend Q_DECL_CONSTEXPR inline bool operator!=(const QRectF &, const QRectF &);

    Q_DECL_CONSTEXPR inline QRect toRect() const Q_REQUIRED_RESULT;
    QRect toAlignedRect() const Q_REQUIRED_RESULT;

private:
    qreal xp;
    qreal yp;
    qreal w;
    qreal h;
};
Q_DECLARE_TYPEINFO(QRectF, Q_MOVABLE_TYPE);

Q_DECL_CONSTEXPR inline bool operator==(const QRectF &, const QRectF &);
Q_DECL_CONSTEXPR inline bool operator!=(const QRectF &, const QRectF &);


/*****************************************************************************
  QRectF stream functions
 *****************************************************************************/
#ifndef QT_NO_DATASTREAM
Q_CORE_EXPORT QDataStream &operator<<(QDataStream &, const QRectF &);
Q_CORE_EXPORT QDataStream &operator>>(QDataStream &, QRectF &);
#endif

/*****************************************************************************
  QRectF inline member functions
 *****************************************************************************/

Q_DECL_CONSTEXPR inline QRectF::QRectF(qreal aleft, qreal atop, qreal awidth, qreal aheight)
    : xp(aleft), yp(atop), w(awidth), h(aheight)
{
}

Q_DECL_CONSTEXPR inline QRectF::QRectF(const QPointF &atopLeft, const QSizeF &asize)
    : xp(atopLeft.x()), yp(atopLeft.y()), w(asize.width()), h(asize.height())
{
}


Q_DECL_CONSTEXPR inline QRectF::QRectF(const QPointF &atopLeft, const QPointF &abottomRight)
    : xp(atopLeft.x()), yp(atopLeft.y()), w(abottomRight.x() - atopLeft.x()), h(abottomRight.y() - atopLeft.y())
{
}

Q_DECL_CONSTEXPR inline QRectF::QRectF(const QRect &r)
    : xp(r.x()), yp(r.y()), w(r.width()), h(r.height())
{
}

Q_DECL_CONSTEXPR inline bool QRectF::isNull() const
{ return w == 0. && h == 0.; }

Q_DECL_CONSTEXPR inline bool QRectF::isEmpty() const
{ return w <= 0. || h <= 0.; }

Q_DECL_CONSTEXPR inline bool QRectF::isValid() const
{ return w > 0. && h > 0.; }

Q_DECL_CONSTEXPR inline qreal QRectF::x() const
{ return xp; }

Q_DECL_CONSTEXPR inline qreal QRectF::y() const
{ return yp; }

inline void QRectF::setLeft(qreal pos) { qreal diff = pos - xp; xp += diff; w -= diff; }

inline void QRectF::setRight(qreal pos) { w = pos - xp; }

inline void QRectF::setTop(qreal pos) { qreal diff = pos - yp; yp += diff; h -= diff; }

inline void QRectF::setBottom(qreal pos) { h = pos - yp; }

inline void QRectF::setTopLeft(const QPointF &p) { setLeft(p.x()); setTop(p.y()); }

inline void QRectF::setTopRight(const QPointF &p) { setRight(p.x()); setTop(p.y()); }

inline void QRectF::setBottomLeft(const QPointF &p) { setLeft(p.x()); setBottom(p.y()); }

inline void QRectF::setBottomRight(const QPointF &p) { setRight(p.x()); setBottom(p.y()); }

Q_DECL_CONSTEXPR inline QPointF QRectF::center() const
{ return QPointF(xp + w/2, yp + h/2); }

inline void QRectF::moveLeft(qreal pos) { xp = pos; }

inline void QRectF::moveTop(qreal pos) { yp = pos; }

inline void QRectF::moveRight(qreal pos) { xp = pos - w; }

inline void QRectF::moveBottom(qreal pos) { yp = pos - h; }

inline void QRectF::moveTopLeft(const QPointF &p) { moveLeft(p.x()); moveTop(p.y()); }

inline void QRectF::moveTopRight(const QPointF &p) { moveRight(p.x()); moveTop(p.y()); }

inline void QRectF::moveBottomLeft(const QPointF &p) { moveLeft(p.x()); moveBottom(p.y()); }

inline void QRectF::moveBottomRight(const QPointF &p) { moveRight(p.x()); moveBottom(p.y()); }

inline void QRectF::moveCenter(const QPointF &p) { xp = p.x() - w/2; yp = p.y() - h/2; }

Q_DECL_CONSTEXPR inline qreal QRectF::width() const
{ return w; }

Q_DECL_CONSTEXPR inline qreal QRectF::height() const
{ return h; }

Q_DECL_CONSTEXPR inline QSizeF QRectF::size() const
{ return QSizeF(w, h); }

inline void QRectF::translate(qreal dx, qreal dy)
{
    xp += dx;
    yp += dy;
}

inline void QRectF::translate(const QPointF &p)
{
    xp += p.x();
    yp += p.y();
}

inline void QRectF::moveTo(qreal ax, qreal ay)
{
    xp = ax;
    yp = ay;
}

inline void QRectF::moveTo(const QPointF &p)
{
    xp = p.x();
    yp = p.y();
}

Q_DECL_CONSTEXPR inline QRectF QRectF::translated(qreal dx, qreal dy) const
{ return QRectF(xp + dx, yp + dy, w, h); }

Q_DECL_CONSTEXPR inline QRectF QRectF::translated(const QPointF &p) const
{ return QRectF(xp + p.x(), yp + p.y(), w, h); }

inline void QRectF::getRect(qreal *ax, qreal *ay, qreal *aaw, qreal *aah) const
{
    *ax = this->xp;
    *ay = this->yp;
    *aaw = this->w;
    *aah = this->h;
}

inline void QRectF::setRect(qreal ax, qreal ay, qreal aaw, qreal aah)
{
    this->xp = ax;
    this->yp = ay;
    this->w = aaw;
    this->h = aah;
}

inline void QRectF::getCoords(qreal *xp1, qreal *yp1, qreal *xp2, qreal *yp2) const
{
    *xp1 = xp;
    *yp1 = yp;
    *xp2 = xp + w;
    *yp2 = yp + h;
}

inline void QRectF::setCoords(qreal xp1, qreal yp1, qreal xp2, qreal yp2)
{
    xp = xp1;
    yp = yp1;
    w = xp2 - xp1;
    h = yp2 - yp1;
}

inline void QRectF::adjust(qreal xp1, qreal yp1, qreal xp2, qreal yp2)
{ xp += xp1; yp += yp1; w += xp2 - xp1; h += yp2 - yp1; }

Q_DECL_CONSTEXPR inline QRectF QRectF::adjusted(qreal xp1, qreal yp1, qreal xp2, qreal yp2) const
{ return QRectF(xp + xp1, yp + yp1, w + xp2 - xp1, h + yp2 - yp1); }

inline void QRectF::setWidth(qreal aw)
{ this->w = aw; }

inline void QRectF::setHeight(qreal ah)
{ this->h = ah; }

inline void QRectF::setSize(const QSizeF &s)
{
    w = s.width();
    h = s.height();
}

inline bool QRectF::contains(qreal ax, qreal ay) const
{
    return contains(QPointF(ax, ay));
}

inline QRectF& QRectF::operator|=(const QRectF &r)
{
    *this = *this | r;
    return *this;
}

inline QRectF& QRectF::operator&=(const QRectF &r)
{
    *this = *this & r;
    return *this;
}

inline QRectF QRectF::intersected(const QRectF &r) const
{
    return *this & r;
}

inline QRectF QRectF::united(const QRectF &r) const
{
    return *this | r;
}

Q_DECL_CONSTEXPR inline bool operator==(const QRectF &r1, const QRectF &r2)
{
    return qFuzzyCompare(r1.xp, r2.xp) && qFuzzyCompare(r1.yp, r2.yp)
           && qFuzzyCompare(r1.w, r2.w) && qFuzzyCompare(r1.h, r2.h);
}

Q_DECL_CONSTEXPR inline bool operator!=(const QRectF &r1, const QRectF &r2)
{
    return !qFuzzyCompare(r1.xp, r2.xp) || !qFuzzyCompare(r1.yp, r2.yp)
           || !qFuzzyCompare(r1.w, r2.w) || !qFuzzyCompare(r1.h, r2.h);
}

Q_DECL_CONSTEXPR inline QRect QRectF::toRect() const
{
    return QRect(qRound(xp), qRound(yp), qRound(w), qRound(h));
}

Q_DECL_CONSTEXPR inline QRectF operator+(const QRectF &lhs, const QMarginsF &rhs)
{
    return QRectF(QPointF(lhs.left() - rhs.left(), lhs.top() - rhs.top()),
                  QSizeF(lhs.width() + rhs.left() + rhs.right(), lhs.height() + rhs.top() + rhs.bottom()));
}

Q_DECL_CONSTEXPR inline QRectF operator+(const QMarginsF &lhs, const QRectF &rhs)
{
    return QRectF(QPointF(rhs.left() - lhs.left(), rhs.top() - lhs.top()),
                  QSizeF(rhs.width() + lhs.left() + lhs.right(), rhs.height() + lhs.top() + lhs.bottom()));
}

Q_DECL_CONSTEXPR inline QRectF operator-(const QRectF &lhs, const QMarginsF &rhs)
{
    return QRectF(QPointF(lhs.left() + rhs.left(), lhs.top() + rhs.top()),
                  QSizeF(lhs.width() - rhs.left() - rhs.right(), lhs.height() - rhs.top() - rhs.bottom()));
}

Q_DECL_CONSTEXPR inline QRectF QRectF::marginsAdded(const QMarginsF &margins) const
{
    return QRectF(QPointF(xp - margins.left(), yp - margins.top()),
                  QSizeF(w + margins.left() + margins.right(), h + margins.top() + margins.bottom()));
}

Q_DECL_CONSTEXPR inline QRectF QRectF::marginsRemoved(const QMarginsF &margins) const
{
    return QRectF(QPointF(xp + margins.left(), yp + margins.top()),
                  QSizeF(w - margins.left() - margins.right(), h - margins.top() - margins.bottom()));
}

inline QRectF &QRectF::operator+=(const QMarginsF &margins)
{
    *this = marginsAdded(margins);
    return *this;
}

inline QRectF &QRectF::operator-=(const QMarginsF &margins)
{
    *this = marginsRemoved(margins);
    return *this;
}

#ifndef QT_NO_DEBUG_STREAM
Q_CORE_EXPORT QDebug operator<<(QDebug, const QRectF &);
#endif

QT_END_NAMESPACE

#endif // QRECT_H
