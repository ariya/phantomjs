/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
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

#ifndef QRECT_H
#define QRECT_H

#include <QtCore/qsize.h>
#include <QtCore/qpoint.h>

#ifdef topLeft
#error qrect.h must be included before any header file that defines topLeft
#endif

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Core)

class Q_CORE_EXPORT QRect
{
public:
    QRect() { x1 = y1 = 0; x2 = y2 = -1; }
    QRect(const QPoint &topleft, const QPoint &bottomright);
    QRect(const QPoint &topleft, const QSize &size);
    QRect(int left, int top, int width, int height);

    bool isNull() const;
    bool isEmpty() const;
    bool isValid() const;

    int left() const;
    int top() const;
    int right() const;
    int bottom() const;
    QRect normalized() const;

#ifdef QT3_SUPPORT
    QT3_SUPPORT int &rLeft() { return x1; }
    QT3_SUPPORT int &rTop() { return y1; }
    QT3_SUPPORT int &rRight() { return x2; }
    QT3_SUPPORT int &rBottom() { return y2; }

    QT3_SUPPORT QRect normalize() const { return normalized(); }
#endif

    int x() const;
    int y() const;
    void setLeft(int pos);
    void setTop(int pos);
    void setRight(int pos);
    void setBottom(int pos);
    void setX(int x);
    void setY(int y);

    void setTopLeft(const QPoint &p);
    void setBottomRight(const QPoint &p);
    void setTopRight(const QPoint &p);
    void setBottomLeft(const QPoint &p);

    QPoint topLeft() const;
    QPoint bottomRight() const;
    QPoint topRight() const;
    QPoint bottomLeft() const;
    QPoint center() const;

    void moveLeft(int pos);
    void moveTop(int pos);
    void moveRight(int pos);
    void moveBottom(int pos);
    void moveTopLeft(const QPoint &p);
    void moveBottomRight(const QPoint &p);
    void moveTopRight(const QPoint &p);
    void moveBottomLeft(const QPoint &p);
    void moveCenter(const QPoint &p);

    inline void translate(int dx, int dy);
    inline void translate(const QPoint &p);
    inline QRect translated(int dx, int dy) const;
    inline QRect translated(const QPoint &p) const;

    void moveTo(int x, int t);
    void moveTo(const QPoint &p);

#ifdef QT3_SUPPORT
    QT3_SUPPORT void moveBy(int dx, int dy) { translate(dx, dy); }
    QT3_SUPPORT void moveBy(const QPoint &p) { translate(p); }
#endif

    void setRect(int x, int y, int w, int h);
    inline void getRect(int *x, int *y, int *w, int *h) const;

    void setCoords(int x1, int y1, int x2, int y2);
#ifdef QT3_SUPPORT
    QT3_SUPPORT void addCoords(int x1, int y1, int x2, int y2);
#endif
    inline void getCoords(int *x1, int *y1, int *x2, int *y2) const;

    inline void adjust(int x1, int y1, int x2, int y2);
    inline QRect adjusted(int x1, int y1, int x2, int y2) const;

    QSize size() const;
    int width() const;
    int height() const;
    void setWidth(int w);
    void setHeight(int h);
    void setSize(const QSize &s);

    QRect operator|(const QRect &r) const;
    QRect operator&(const QRect &r) const;
    QRect& operator|=(const QRect &r);
    QRect& operator&=(const QRect &r);

    bool contains(const QPoint &p, bool proper=false) const;
    bool contains(int x, int y) const; // inline methods, _don't_ merge these
    bool contains(int x, int y, bool proper) const;
    bool contains(const QRect &r, bool proper = false) const;
    QRect unite(const QRect &r) const;  // ### Qt 5: make QT4_SUPPORT
    QRect united(const QRect &other) const;
    QRect intersect(const QRect &r) const;  // ### Qt 5: make QT4_SUPPORT
    QRect intersected(const QRect &other) const;
    bool intersects(const QRect &r) const;

    friend Q_CORE_EXPORT_INLINE bool operator==(const QRect &, const QRect &);
    friend Q_CORE_EXPORT_INLINE bool operator!=(const QRect &, const QRect &);

#ifdef QT3_SUPPORT
    inline QT3_SUPPORT void rect(int *x, int *y, int *w, int *h) const { getRect(x, y, w, h); }
    inline QT3_SUPPORT void coords(int *ax1, int *ay1, int *ax2, int *ay2) const
    { getCoords(ax1, ay1, ax2, ay2); }
#endif

private:
#if defined(Q_WS_X11)
    friend void qt_setCoords(QRect *r, int xp1, int yp1, int xp2, int yp2);
#endif
    // ### Qt 5;  remove the ifdef and just have the same order on all platforms.
#if defined(Q_OS_MAC)
    int y1;
    int x1;
    int y2;
    int x2;
#else
    int x1;
    int y1;
    int x2;
    int y2;
#endif

};
Q_DECLARE_TYPEINFO(QRect, Q_MOVABLE_TYPE);

Q_CORE_EXPORT_INLINE bool operator==(const QRect &, const QRect &);
Q_CORE_EXPORT_INLINE bool operator!=(const QRect &, const QRect &);


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

inline QRect::QRect(int aleft, int atop, int awidth, int aheight)
{
    x1 = aleft;
    y1 = atop;
    x2 = (aleft + awidth - 1);
    y2 = (atop + aheight - 1);
}

inline QRect::QRect(const QPoint &atopLeft, const QPoint &abottomRight)
{
    x1 = atopLeft.x();
    y1 = atopLeft.y();
    x2 = abottomRight.x();
    y2 = abottomRight.y();
}

inline QRect::QRect(const QPoint &atopLeft, const QSize &asize)
{
    x1 = atopLeft.x();
    y1 = atopLeft.y();
    x2 = (x1+asize.width() - 1);
    y2 = (y1+asize.height() - 1);
}

inline bool QRect::isNull() const
{ return x2 == x1 - 1 && y2 == y1 - 1; }

inline bool QRect::isEmpty() const
{ return x1 > x2 || y1 > y2; }

inline bool QRect::isValid() const
{ return x1 <= x2 && y1 <= y2; }

inline int QRect::left() const
{ return x1; }

inline int QRect::top() const
{ return y1; }

inline int QRect::right() const
{ return x2; }

inline int QRect::bottom() const
{ return y2; }

inline int QRect::x() const
{ return x1; }

inline int QRect::y() const
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

inline QPoint QRect::topLeft() const
{ return QPoint(x1, y1); }

inline QPoint QRect::bottomRight() const
{ return QPoint(x2, y2); }

inline QPoint QRect::topRight() const
{ return QPoint(x2, y1); }

inline QPoint QRect::bottomLeft() const
{ return QPoint(x1, y2); }

inline QPoint QRect::center() const
{ return QPoint((x1+x2)/2, (y1+y2)/2); }

inline int QRect::width() const
{ return  x2 - x1 + 1; }

inline int QRect::height() const
{ return  y2 - y1 + 1; }

inline QSize QRect::size() const
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

inline QRect QRect::translated(int dx, int dy) const
{ return QRect(QPoint(x1 + dx, y1 + dy), QPoint(x2 + dx, y2 + dy)); }

inline QRect QRect::translated(const QPoint &p) const
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

#ifdef QT3_SUPPORT
inline void QRect::addCoords(int dx1, int dy1, int dx2, int dy2)
{
    adjust(dx1, dy1, dx2, dy2);
}
#endif

inline QRect QRect::adjusted(int xp1, int yp1, int xp2, int yp2) const
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

inline QRect QRect::intersect(const QRect &r) const
{
    return *this & r;
}

inline QRect QRect::intersected(const QRect &other) const
{
    return intersect(other);
}

inline QRect QRect::unite(const QRect &r) const
{
    return *this | r;
}

inline QRect QRect::united(const QRect &r) const
{
     return unite(r);
}

inline bool operator==(const QRect &r1, const QRect &r2)
{
    return r1.x1==r2.x1 && r1.x2==r2.x2 && r1.y1==r2.y1 && r1.y2==r2.y2;
}

inline bool operator!=(const QRect &r1, const QRect &r2)
{
    return r1.x1!=r2.x1 || r1.x2!=r2.x2 || r1.y1!=r2.y1 || r1.y2!=r2.y2;
}

#ifndef QT_NO_DEBUG_STREAM
Q_CORE_EXPORT QDebug operator<<(QDebug, const QRect &);
#endif


class Q_CORE_EXPORT QRectF
{
public:
    QRectF() { xp = yp = 0.; w = h = 0.; }
    QRectF(const QPointF &topleft, const QSizeF &size);
    QRectF(const QPointF &topleft, const QPointF &bottomRight);
    QRectF(qreal left, qreal top, qreal width, qreal height);
    QRectF(const QRect &rect);

    bool isNull() const;
    bool isEmpty() const;
    bool isValid() const;
    QRectF normalized() const;

    inline qreal left() const { return xp; }
    inline qreal top() const { return yp; }
    inline qreal right() const { return xp + w; }
    inline qreal bottom() const { return yp + h; }

    inline qreal x() const;
    inline qreal y() const;
    inline void setLeft(qreal pos);
    inline void setTop(qreal pos);
    inline void setRight(qreal pos);
    inline void setBottom(qreal pos);
    inline void setX(qreal pos) { setLeft(pos); }
    inline void setY(qreal pos) { setTop(pos); }

    inline QPointF topLeft() const { return QPointF(xp, yp); }
    inline QPointF bottomRight() const { return QPointF(xp+w, yp+h); }
    inline QPointF topRight() const { return QPointF(xp+w, yp); }
    inline QPointF bottomLeft() const { return QPointF(xp, yp+h); }
    inline QPointF center() const;

    void setTopLeft(const QPointF &p);
    void setBottomRight(const QPointF &p);
    void setTopRight(const QPointF &p);
    void setBottomLeft(const QPointF &p);

    void moveLeft(qreal pos);
    void moveTop(qreal pos);
    void moveRight(qreal pos);
    void moveBottom(qreal pos);
    void moveTopLeft(const QPointF &p);
    void moveBottomRight(const QPointF &p);
    void moveTopRight(const QPointF &p);
    void moveBottomLeft(const QPointF &p);
    void moveCenter(const QPointF &p);

    void translate(qreal dx, qreal dy);
    void translate(const QPointF &p);

    QRectF translated(qreal dx, qreal dy) const;
    QRectF translated(const QPointF &p) const;

    void moveTo(qreal x, qreal t);
    void moveTo(const QPointF &p);

    void setRect(qreal x, qreal y, qreal w, qreal h);
    void getRect(qreal *x, qreal *y, qreal *w, qreal *h) const;

    void setCoords(qreal x1, qreal y1, qreal x2, qreal y2);
    void getCoords(qreal *x1, qreal *y1, qreal *x2, qreal *y2) const;

    inline void adjust(qreal x1, qreal y1, qreal x2, qreal y2);
    inline QRectF adjusted(qreal x1, qreal y1, qreal x2, qreal y2) const;

    QSizeF size() const;
    qreal width() const;
    qreal height() const;
    void setWidth(qreal w);
    void setHeight(qreal h);
    void setSize(const QSizeF &s);

    QRectF operator|(const QRectF &r) const;
    QRectF operator&(const QRectF &r) const;
    QRectF& operator|=(const QRectF &r);
    QRectF& operator&=(const QRectF &r);

    bool contains(const QPointF &p) const;
    bool contains(qreal x, qreal y) const;
    bool contains(const QRectF &r) const;
    QRectF unite(const QRectF &r) const;  // ### Qt 5: make QT4_SUPPORT
    QRectF united(const QRectF &other) const;
    QRectF intersect(const QRectF &r) const;  // ### Qt 5: make QT4_SUPPORT
    QRectF intersected(const QRectF &other) const;
    bool intersects(const QRectF &r) const;

    friend Q_CORE_EXPORT_INLINE bool operator==(const QRectF &, const QRectF &);
    friend Q_CORE_EXPORT_INLINE bool operator!=(const QRectF &, const QRectF &);

    QRect toRect() const;
    QRect toAlignedRect() const;

private:
    qreal xp;
    qreal yp;
    qreal w;
    qreal h;
};
Q_DECLARE_TYPEINFO(QRectF, Q_MOVABLE_TYPE);

Q_CORE_EXPORT_INLINE bool operator==(const QRectF &, const QRectF &);
Q_CORE_EXPORT_INLINE bool operator!=(const QRectF &, const QRectF &);


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

inline QRectF::QRectF(qreal aleft, qreal atop, qreal awidth, qreal aheight)
    : xp(aleft), yp(atop), w(awidth), h(aheight)
{
}

inline QRectF::QRectF(const QPointF &atopLeft, const QSizeF &asize)
{
    xp = atopLeft.x();
    yp = atopLeft.y();
    w = asize.width();
    h = asize.height();
}

inline QRectF::QRectF(const QPointF &atopLeft, const QPointF &abottomRight)
{
    xp = atopLeft.x();
    yp = atopLeft.y();
    w = abottomRight.x() - xp;
    h = abottomRight.y() - yp;
}

inline QRectF::QRectF(const QRect &r)
    : xp(r.x()), yp(r.y()), w(r.width()), h(r.height())
{
}

inline bool QRectF::isNull() const
{ return w == 0. && h == 0.; }

inline bool QRectF::isEmpty() const
{ return w <= 0. || h <= 0.; }

inline bool QRectF::isValid() const
{ return w > 0. && h > 0.; }

inline qreal QRectF::x() const
{ return xp; }

inline qreal QRectF::y() const
{ return yp; }

inline void QRectF::setLeft(qreal pos) { qreal diff = pos - xp; xp += diff; w -= diff; }

inline void QRectF::setRight(qreal pos) { w = pos - xp; }

inline void QRectF::setTop(qreal pos) { qreal diff = pos - yp; yp += diff; h -= diff; }

inline void QRectF::setBottom(qreal pos) { h = pos - yp; }

inline void QRectF::setTopLeft(const QPointF &p) { setLeft(p.x()); setTop(p.y()); }

inline void QRectF::setTopRight(const QPointF &p) { setRight(p.x()); setTop(p.y()); }

inline void QRectF::setBottomLeft(const QPointF &p) { setLeft(p.x()); setBottom(p.y()); }

inline void QRectF::setBottomRight(const QPointF &p) { setRight(p.x()); setBottom(p.y()); }

inline QPointF QRectF::center() const
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

inline qreal QRectF::width() const
{ return w; }

inline qreal QRectF::height() const
{ return h; }

inline QSizeF QRectF::size() const
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

inline QRectF QRectF::translated(qreal dx, qreal dy) const
{ return QRectF(xp + dx, yp + dy, w, h); }

inline QRectF QRectF::translated(const QPointF &p) const
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

inline QRectF QRectF::adjusted(qreal xp1, qreal yp1, qreal xp2, qreal yp2) const
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

inline QRectF QRectF::intersect(const QRectF &r) const
{
    return *this & r;
}

inline QRectF QRectF::intersected(const QRectF &r) const
{
    return intersect(r);
}

inline QRectF QRectF::unite(const QRectF &r) const
{
    return *this | r;
}

inline QRectF QRectF::united(const QRectF &r) const
{
    return unite(r);
}

inline bool operator==(const QRectF &r1, const QRectF &r2)
{
    return qFuzzyCompare(r1.xp, r2.xp) && qFuzzyCompare(r1.yp, r2.yp)
           && qFuzzyCompare(r1.w, r2.w) && qFuzzyCompare(r1.h, r2.h);
}

inline bool operator!=(const QRectF &r1, const QRectF &r2)
{
    return !qFuzzyCompare(r1.xp, r2.xp) || !qFuzzyCompare(r1.yp, r2.yp)
           || !qFuzzyCompare(r1.w, r2.w) || !qFuzzyCompare(r1.h, r2.h);
}

inline QRect QRectF::toRect() const
{
    return QRect(qRound(xp), qRound(yp), qRound(w), qRound(h));
}

#ifndef QT_NO_DEBUG_STREAM
Q_CORE_EXPORT QDebug operator<<(QDebug, const QRectF &);
#endif

QT_END_NAMESPACE

QT_END_HEADER

#endif // QRECT_H
