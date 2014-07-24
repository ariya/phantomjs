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

#include "qregion.h"
#include "qpainterpath.h"
#include "qpolygon.h"
#include "qbuffer.h"
#include "qdatastream.h"
#include "qvariant.h"
#include "qvarlengtharray.h"
#include "qimage.h"
#include "qbitmap.h"

#include <qdebug.h>

QT_BEGIN_NAMESPACE

/*!
    \class QRegion
    \brief The QRegion class specifies a clip region for a painter.

    \inmodule QtGui
    \ingroup painting
    \ingroup shared

    QRegion is used with QPainter::setClipRegion() to limit the paint
    area to what needs to be painted. There is also a QWidget::repaint()
    function that takes a QRegion parameter. QRegion is the best tool for
    minimizing the amount of screen area to be updated by a repaint.

    This class is not suitable for constructing shapes for rendering, especially
    as outlines. Use QPainterPath to create paths and shapes for use with
    QPainter.

    QRegion is an \l{implicitly shared} class.

    \section1 Creating and Using Regions

    A region can be created from a rectangle, an ellipse, a polygon or
    a bitmap. Complex regions may be created by combining simple
    regions using united(), intersected(), subtracted(), or xored() (exclusive
    or). You can move a region using translate().

    You can test whether a region isEmpty() or if it
    contains() a QPoint or QRect. The bounding rectangle can be found
    with boundingRect().

    The function rects() gives a decomposition of the region into
    rectangles.

    Example of using complex regions:
    \snippet code/src_gui_painting_qregion.cpp 0

    \section1 Additional License Information

    On Embedded Linux, Windows CE and X11 platforms, parts of this class rely on
    code obtained under the following licenses:

    \legalese
    Copyright (c) 1987  X Consortium

    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to deal
    in the Software without restriction, including without limitation the rights
    to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
    copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included in
    all copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
    X CONSORTIUM BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
    AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
    CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

    Except as contained in this notice, the name of the X Consortium shall not be
    used in advertising or otherwise to promote the sale, use or other dealings
    in this Software without prior written authorization from the X Consortium.
    \endlegalese

    \br

    \legalese
    Copyright 1987 by Digital Equipment Corporation, Maynard, Massachusetts.

                            All Rights Reserved

    Permission to use, copy, modify, and distribute this software and its
    documentation for any purpose and without fee is hereby granted,
    provided that the above copyright notice appear in all copies and that
    both that copyright notice and this permission notice appear in
    supporting documentation, and that the name of Digital not be
    used in advertising or publicity pertaining to distribution of the
    software without specific, written prior permission.

    DIGITAL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
    ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
    DIGITAL BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
    ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
    WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
    ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
    SOFTWARE.
    \endlegalese

    \sa QPainter::setClipRegion(), QPainter::setClipRect(), QPainterPath
*/


/*!
    \enum QRegion::RegionType

    Specifies the shape of the region to be created.

    \value Rectangle  the region covers the entire rectangle.
    \value Ellipse  the region is an ellipse inside the rectangle.
*/

/*!
    \fn void QRegion::translate(const QPoint &point)

    \overload

    Translates the region \a{point}\e{.x()} along the x axis and
    \a{point}\e{.y()} along the y axis, relative to the current
    position. Positive values move the region to the right and down.

    Translates to the given \a point.
*/

/*****************************************************************************
  QRegion member functions
 *****************************************************************************/

/*!
    \fn QRegion::QRegion()

    Constructs an empty region.

    \sa isEmpty()
*/

/*!
    \fn QRegion::QRegion(const QRect &r, RegionType t)
    \overload

    Create a region based on the rectange \a r with region type \a t.

    If the rectangle is invalid a null region will be created.

    \sa QRegion::RegionType
*/

/*!
    \fn QRegion::QRegion(const QPolygon &a, Qt::FillRule fillRule)

    Constructs a polygon region from the point array \a a with the fill rule
    specified by \a fillRule.

    If \a fillRule is \l{Qt::WindingFill}, the polygon region is defined
    using the winding algorithm; if it is \l{Qt::OddEvenFill}, the odd-even fill
    algorithm is used.

    \warning This constructor can be used to create complex regions that will
    slow down painting when used.
*/

/*!
    \fn QRegion::QRegion(const QRegion &r)

    Constructs a new region which is equal to region \a r.
*/

/*!
    \fn QRegion::QRegion(const QBitmap &bm)

    Constructs a region from the bitmap \a bm.

    The resulting region consists of the pixels in bitmap \a bm that
    are Qt::color1, as if each pixel was a 1 by 1 rectangle.

    This constructor may create complex regions that will slow down
    painting when used. Note that drawing masked pixmaps can be done
    much faster using QPixmap::setMask().
*/

/*!
    Constructs a rectangular or elliptic region.

    If \a t is \c Rectangle, the region is the filled rectangle (\a x,
    \a y, \a w, \a h). If \a t is \c Ellipse, the region is the filled
    ellipse with center at (\a x + \a w / 2, \a y + \a h / 2) and size
    (\a w ,\a h).
*/
QRegion::QRegion(int x, int y, int w, int h, RegionType t)
{
    QRegion tmp(QRect(x, y, w, h), t);
    tmp.d->ref.ref();
    d = tmp.d;
}

/*!
    \fn QRegion::~QRegion()
    \internal

    Destroys the region.
*/

void QRegion::detach()
{
    if (d->ref.load() != 1)
        *this = copy();
}

// duplicates in qregion_win.cpp and qregion_wce.cpp
#define QRGN_SETRECT          1                // region stream commands
#define QRGN_SETELLIPSE       2                //  (these are internal)
#define QRGN_SETPTARRAY_ALT   3
#define QRGN_SETPTARRAY_WIND  4
#define QRGN_TRANSLATE        5
#define QRGN_OR               6
#define QRGN_AND              7
#define QRGN_SUB              8
#define QRGN_XOR              9
#define QRGN_RECTS            10


#ifndef QT_NO_DATASTREAM

/*
    Executes region commands in the internal buffer and rebuilds the
    original region.

    We do this when we read a region from the data stream.

    If \a ver is non-0, uses the format version \a ver on reading the
    byte array.
*/
void QRegion::exec(const QByteArray &buffer, int ver, QDataStream::ByteOrder byteOrder)
{
    QByteArray copy = buffer;
    QDataStream s(&copy, QIODevice::ReadOnly);
    if (ver)
        s.setVersion(ver);
    s.setByteOrder(byteOrder);
    QRegion rgn;
#ifndef QT_NO_DEBUG
    int test_cnt = 0;
#endif
    while (!s.atEnd()) {
        qint32 id;
        if (s.version() == 1) {
            int id_int;
            s >> id_int;
            id = id_int;
        } else {
            s >> id;
        }
#ifndef QT_NO_DEBUG
        if (test_cnt > 0 && id != QRGN_TRANSLATE)
            qWarning("QRegion::exec: Internal error");
        test_cnt++;
#endif
        if (id == QRGN_SETRECT || id == QRGN_SETELLIPSE) {
            QRect r;
            s >> r;
            rgn = QRegion(r, id == QRGN_SETRECT ? Rectangle : Ellipse);
        } else if (id == QRGN_SETPTARRAY_ALT || id == QRGN_SETPTARRAY_WIND) {
            QPolygon a;
            s >> a;
            rgn = QRegion(a, id == QRGN_SETPTARRAY_WIND ? Qt::WindingFill : Qt::OddEvenFill);
        } else if (id == QRGN_TRANSLATE) {
            QPoint p;
            s >> p;
            rgn.translate(p.x(), p.y());
        } else if (id >= QRGN_OR && id <= QRGN_XOR) {
            QByteArray bop1, bop2;
            QRegion r1, r2;
            s >> bop1;
            r1.exec(bop1);
            s >> bop2;
            r2.exec(bop2);

            switch (id) {
                case QRGN_OR:
                    rgn = r1.united(r2);
                    break;
                case QRGN_AND:
                    rgn = r1.intersected(r2);
                    break;
                case QRGN_SUB:
                    rgn = r1.subtracted(r2);
                    break;
                case QRGN_XOR:
                    rgn = r1.xored(r2);
                    break;
            }
        } else if (id == QRGN_RECTS) {
            // (This is the only form used in Qt 2.0)
            quint32 n;
            s >> n;
            QRect r;
            for (int i=0; i<(int)n; i++) {
                s >> r;
                rgn = rgn.united(QRegion(r));
            }
        }
    }
    *this = rgn;
}


/*****************************************************************************
  QRegion stream functions
 *****************************************************************************/

/*!
    \fn QRegion &QRegion::operator=(const QRegion &r)

    Assigns \a r to this region and returns a reference to the region.
*/

/*!
    \fn QRegion &QRegion::operator=(QRegion &&other)

    Move-assigns \a other to this QRegion instance.

    \since 5.2
*/

/*!
    \fn void QRegion::swap(QRegion &other)
    \since 4.8

    Swaps region \a other with this region. This operation is very
    fast and never fails.
*/

/*!
    \relates QRegion

    Writes the region \a r to the stream \a s and returns a reference
    to the stream.

    \sa{Serializing Qt Data Types}{Format of the QDataStream operators}
*/

QDataStream &operator<<(QDataStream &s, const QRegion &r)
{
    QVector<QRect> a = r.rects();
    if (a.isEmpty()) {
        s << (quint32)0;
    } else {
        if (s.version() == 1) {
            int i;
            for (i = a.size() - 1; i > 0; --i) {
                s << (quint32)(12 + i * 24);
                s << (int)QRGN_OR;
            }
            for (i = 0; i < a.size(); ++i) {
                s << (quint32)(4+8) << (int)QRGN_SETRECT << a[i];
            }
        } else {
            s << (quint32)(4 + 4 + 16 * a.size()); // 16: storage size of QRect
            s << (qint32)QRGN_RECTS;
            s << a;
        }
    }
    return s;
}

/*!
    \relates QRegion

    Reads a region from the stream \a s into \a r and returns a
    reference to the stream.

    \sa{Serializing Qt Data Types}{Format of the QDataStream operators}
*/

QDataStream &operator>>(QDataStream &s, QRegion &r)
{
    QByteArray b;
    s >> b;
    r.exec(b, s.version(), s.byteOrder());
    return s;
}
#endif //QT_NO_DATASTREAM

#ifndef QT_NO_DEBUG_STREAM
QDebug operator<<(QDebug s, const QRegion &r)
{
    QVector<QRect> rects = r.rects();
    s.nospace() << "QRegion(size=" << rects.size() << "), "
                << "bounds = " << r.boundingRect() << '\n';
    for (int i=0; i<rects.size(); ++i)
        s << "- " << i << rects.at(i) << '\n';
    return s;
}
#endif


// These are not inline - they can be implemented better on some platforms
//  (eg. Windows at least provides 3-variable operations).  For now, simple.


/*!
    Applies the united() function to this region and \a r. \c r1|r2 is
    equivalent to \c r1.united(r2).

    \sa united(), operator+()
*/
const QRegion QRegion::operator|(const QRegion &r) const
    { return united(r); }

/*!
    Applies the united() function to this region and \a r. \c r1+r2 is
    equivalent to \c r1.united(r2).

    \sa united(), operator|()
*/
const QRegion QRegion::operator+(const QRegion &r) const
    { return united(r); }

/*!
   \overload
   \since 4.4
 */
const QRegion QRegion::operator+(const QRect &r) const
    { return united(r); }

/*!
    Applies the intersected() function to this region and \a r. \c r1&r2
    is equivalent to \c r1.intersected(r2).

    \sa intersected()
*/
const QRegion QRegion::operator&(const QRegion &r) const
    { return intersected(r); }

/*!
   \overload
   \since 4.4
 */
const QRegion QRegion::operator&(const QRect &r) const
{
    return intersected(r);
}

/*!
    Applies the subtracted() function to this region and \a r. \c r1-r2
    is equivalent to \c r1.subtracted(r2).

    \sa subtracted()
*/
const QRegion QRegion::operator-(const QRegion &r) const
    { return subtracted(r); }

/*!
    Applies the xored() function to this region and \a r. \c r1^r2 is
    equivalent to \c r1.xored(r2).

    \sa xored()
*/
const QRegion QRegion::operator^(const QRegion &r) const
    { return xored(r); }

/*!
    Applies the united() function to this region and \a r and assigns
    the result to this region. \c r1|=r2 is equivalent to \c
    {r1 = r1.united(r2)}.

    \sa united()
*/
QRegion& QRegion::operator|=(const QRegion &r)
    { return *this = *this | r; }

/*!
    \fn QRegion& QRegion::operator+=(const QRect &rect)

    Returns a region that is the union of this region with the specified \a rect.

    \sa united()
*/
/*!
    \fn QRegion& QRegion::operator+=(const QRegion &r)

    Applies the united() function to this region and \a r and assigns
    the result to this region. \c r1+=r2 is equivalent to \c
    {r1 = r1.united(r2)}.

    \sa intersected()
*/
#if !defined (Q_OS_UNIX) && !defined (Q_OS_WIN)
QRegion& QRegion::operator+=(const QRect &r)
{
    return operator+=(QRegion(r));
}
#endif

/*!
  \fn QRegion& QRegion::operator&=(const QRegion &r)

  Applies the intersected() function to this region and \a r and
  assigns the result to this region. \c r1&=r2 is equivalent to \c
  r1 = r1.intersected(r2).

  \sa intersected()
*/
QRegion& QRegion::operator&=(const QRegion &r)
    { return *this = *this & r; }

/*!
   \overload
   \since 4.4
 */
#if defined (Q_OS_UNIX) || defined (Q_OS_WIN)
QRegion& QRegion::operator&=(const QRect &r)
{
    return *this = *this & r;
}
#else
QRegion& QRegion::operator&=(const QRect &r)
{
    return *this &= (QRegion(r));
}
#endif

/*!
  \fn QRegion& QRegion::operator-=(const QRegion &r)

  Applies the subtracted() function to this region and \a r and
  assigns the result to this region. \c r1-=r2 is equivalent to \c
  {r1 = r1.subtracted(r2)}.

  \sa subtracted()
*/
QRegion& QRegion::operator-=(const QRegion &r)
    { return *this = *this - r; }

/*!
    Applies the xored() function to this region and \a r and
    assigns the result to this region. \c r1^=r2 is equivalent to \c
    {r1 = r1.xored(r2)}.

    \sa xored()
*/
QRegion& QRegion::operator^=(const QRegion &r)
    { return *this = *this ^ r; }

/*!
    \fn bool QRegion::operator!=(const QRegion &other) const

    Returns \c true if this region is different from the \a other region;
    otherwise returns \c false.
*/

/*!
   Returns the region as a QVariant
*/
QRegion::operator QVariant() const
{
    return QVariant(QVariant::Region, this);
}

/*!
    \fn bool QRegion::operator==(const QRegion &r) const

    Returns \c true if the region is equal to \a r; otherwise returns
    false.
*/

/*!
    \fn void QRegion::translate(int dx, int dy)

    Translates (moves) the region \a dx along the X axis and \a dy
    along the Y axis.
*/

/*!
    \fn QRegion QRegion::translated(const QPoint &p) const
    \overload
    \since 4.1

    Returns a copy of the regtion that is translated \a{p}\e{.x()}
    along the x axis and \a{p}\e{.y()} along the y axis, relative to
    the current position.  Positive values move the rectangle to the
    right and down.

    \sa translate()
*/

/*!
    \since 4.1

    Returns a copy of the region that is translated \a dx along the
    x axis and \a dy along the y axis, relative to the current
    position. Positive values move the region to the right and
    down.

    \sa translate()
*/

QRegion
QRegion::translated(int dx, int dy) const
{
    QRegion ret(*this);
    ret.translate(dx, dy);
    return ret;
}


inline bool rect_intersects(const QRect &r1, const QRect &r2)
{
    return (r1.right() >= r2.left() && r1.left() <= r2.right() &&
            r1.bottom() >= r2.top() && r1.top() <= r2.bottom());
}

/*!
    \since 4.2

    Returns \c true if this region intersects with \a region, otherwise
    returns \c false.
*/
bool QRegion::intersects(const QRegion &region) const
{
    if (isEmpty() || region.isEmpty())
        return false;

    if (!rect_intersects(boundingRect(), region.boundingRect()))
        return false;
    if (rectCount() == 1 && region.rectCount() == 1)
        return true;

    const QVector<QRect> myRects = rects();
    const QVector<QRect> otherRects = region.rects();

    for (QVector<QRect>::const_iterator i1 = myRects.constBegin(); i1 < myRects.constEnd(); ++i1)
        for (QVector<QRect>::const_iterator i2 = otherRects.constBegin(); i2 < otherRects.constEnd(); ++i2)
            if (rect_intersects(*i1, *i2))
                return true;
    return false;
}

/*!
    \fn bool QRegion::intersects(const QRect &rect) const
    \since 4.2

    Returns \c true if this region intersects with \a rect, otherwise
    returns \c false.
*/


#if !defined (Q_OS_UNIX) && !defined (Q_OS_WIN)
/*!
    \overload
    \since 4.4
*/
QRegion QRegion::intersect(const QRect &r) const
{
    return intersect(QRegion(r));
}
#endif

/*!
    \fn int QRegion::rectCount() const
    \since 4.6

    Returns the number of rectangles that will be returned in rects().
*/

/*!
    \fn bool QRegion::isEmpty() const

    Returns \c true if the region is empty; otherwise returns \c false. An
    empty region is a region that contains no points.

    Example:
    \snippet code/src_gui_painting_qregion_unix.cpp 0
*/

/*!
    \fn bool QRegion::isNull() const
    \since 5.0

    Returns \c true if the region is empty; otherwise returns \c false. An
    empty region is a region that contains no points. This function is
    the same as isEmpty

    \sa isEmpty()
*/

/*!
    \fn bool QRegion::contains(const QPoint &p) const

    Returns \c true if the region contains the point \a p; otherwise
    returns \c false.
*/

/*!
    \fn bool QRegion::contains(const QRect &r) const
    \overload

    Returns \c true if the region overlaps the rectangle \a r; otherwise
    returns \c false.
*/

/*!
    \fn QRegion QRegion::unite(const QRegion &r) const
    \obsolete

    Use united(\a r) instead.
*/

/*!
    \fn QRegion QRegion::unite(const QRect &rect) const
    \since 4.4
    \obsolete

    Use united(\a rect) instead.
*/

/*!
    \fn QRegion QRegion::united(const QRect &rect) const
    \since 4.4

    Returns a region which is the union of this region and the given \a rect.

    \sa intersected(), subtracted(), xored()
*/

/*!
    \fn QRegion QRegion::united(const QRegion &r) const
    \since 4.2

    Returns a region which is the union of this region and \a r.

    \image runion.png Region Union

    The figure shows the union of two elliptical regions.

    \sa intersected(), subtracted(), xored()
*/

/*!
    \fn QRegion QRegion::intersect(const QRegion &r) const
    \obsolete

    Use intersected(\a r) instead.
*/

/*!
    \fn QRegion QRegion::intersect(const QRect &rect) const
    \since 4.4
    \obsolete

    Use intersected(\a rect) instead.
*/

/*!
    \fn QRegion QRegion::intersected(const QRect &rect) const
    \since 4.4

    Returns a region which is the intersection of this region and the given \a rect.

    \sa subtracted(), united(), xored()
*/

/*!
    \fn QRegion QRegion::intersected(const QRegion &r) const
    \since 4.2

    Returns a region which is the intersection of this region and \a r.

    \image rintersect.png Region Intersection

    The figure shows the intersection of two elliptical regions.

    \sa subtracted(), united(), xored()
*/

/*!
    \fn QRegion QRegion::subtract(const QRegion &r) const
    \obsolete

    Use subtracted(\a r) instead.
*/

/*!
    \fn QRegion QRegion::subtracted(const QRegion &r) const
    \since 4.2

    Returns a region which is \a r subtracted from this region.

    \image rsubtract.png Region Subtraction

    The figure shows the result when the ellipse on the right is
    subtracted from the ellipse on the left (\c {left - right}).

    \sa intersected(), united(), xored()
*/

/*!
    \fn QRegion QRegion::eor(const QRegion &r) const
    \obsolete

    Use xored(\a r) instead.
*/

/*!
    \fn QRegion QRegion::xored(const QRegion &r) const
    \since 4.2

    Returns a region which is the exclusive or (XOR) of this region
    and \a r.

    \image rxor.png Region XORed

    The figure shows the exclusive or of two elliptical regions.

    \sa intersected(), united(), subtracted()
*/

/*!
    \fn QRect QRegion::boundingRect() const

    Returns the bounding rectangle of this region. An empty region
    gives a rectangle that is QRect::isNull().
*/

/*!
    \fn QVector<QRect> QRegion::rects() const

    Returns an array of non-overlapping rectangles that make up the
    region.

    The union of all the rectangles is equal to the original region.
*/

/*!
    \fn void QRegion::setRects(const QRect *rects, int number)

    Sets the region using the array of rectangles specified by \a rects and
    \a number.
    The rectangles \e must be optimally Y-X sorted and follow these restrictions:

    \list
    \li The rectangles must not intersect.
    \li All rectangles with a given top coordinate must have the same height.
    \li No two rectangles may abut horizontally (they should be combined
       into a single wider rectangle in that case).
    \li The rectangles must be sorted in ascending order, with Y as the major
       sort key and X as the minor sort key.
    \endlist
    \omit
    Only some platforms have these restrictions (Qt for Embedded Linux, X11 and Mac OS X).
    \endomit
*/

namespace {

struct Segment
{
    Segment() {}
    Segment(const QPoint &p)
        : added(false)
        , point(p)
    {
    }

    int left() const
    {
        return qMin(point.x(), next->point.x());
    }

    int right() const
    {
        return qMax(point.x(), next->point.x());
    }

    bool overlaps(const Segment &other) const
    {
        return left() < other.right() && other.left() < right();
    }

    void connect(Segment &other)
    {
        next = &other;
        other.prev = this;

        horizontal = (point.y() == other.point.y());
    }

    void merge(Segment &other)
    {
        if (right() <= other.right()) {
            QPoint p = other.point;
            Segment *oprev = other.prev;

            other.point = point;
            other.prev = prev;
            prev->next = &other;

            point = p;
            prev = oprev;
            oprev->next = this;
        } else {
            Segment *onext = other.next;
            other.next = next;
            next->prev = &other;

            next = onext;
            next->prev = this;
        }
    }

    int horizontal : 1;
    int added : 1;

    QPoint point;
    Segment *prev;
    Segment *next;
};

void mergeSegments(Segment *a, int na, Segment *b, int nb)
{
    int i = 0;
    int j = 0;

    while (i != na && j != nb) {
        Segment &sa = a[i];
        Segment &sb = b[j];
        const int ra = sa.right();
        const int rb = sb.right();
        if (sa.overlaps(sb))
            sa.merge(sb);
        i += (rb >= ra);
        j += (ra >= rb);
    }
}

void addSegmentsToPath(Segment *segment, QPainterPath &path)
{
    Segment *current = segment;
    path.moveTo(current->point);

    current->added = true;

    Segment *last = current;
    current = current->next;
    while (current != segment) {
        if (current->horizontal != last->horizontal)
            path.lineTo(current->point);
        current->added = true;
        last = current;
        current = current->next;
    }
}

}

Q_GUI_EXPORT QPainterPath qt_regionToPath(const QRegion &region)
{
    QPainterPath result;
    if (region.rectCount() == 1) {
        result.addRect(region.boundingRect());
        return result;
    }

    const QVector<QRect> rects = region.rects();

    QVarLengthArray<Segment> segments;
    segments.resize(4 * rects.size());

    const QRect *rect = rects.constData();
    const QRect *end = rect + rects.size();

    int lastRowSegmentCount = 0;
    Segment *lastRowSegments = 0;

    int lastSegment = 0;
    int lastY = 0;
    while (rect != end) {
        const int y = rect[0].y();
        int count = 0;
        while (&rect[count] != end && rect[count].y() == y)
            ++count;

        for (int i = 0; i < count; ++i) {
            int offset = lastSegment + i;
            segments[offset] = Segment(rect[i].topLeft());
            segments[offset += count] = Segment(rect[i].topRight() + QPoint(1, 0));
            segments[offset += count] = Segment(rect[i].bottomRight() + QPoint(1, 1));
            segments[offset += count] = Segment(rect[i].bottomLeft() + QPoint(0, 1));

            offset = lastSegment + i;
            for (int j = 0; j < 4; ++j)
                segments[offset + j * count].connect(segments[offset + ((j + 1) % 4) * count]);
        }

        if (lastRowSegments && lastY == y)
            mergeSegments(lastRowSegments, lastRowSegmentCount, &segments[lastSegment], count);

        lastRowSegments = &segments[lastSegment + 2 * count];
        lastRowSegmentCount = count;
        lastSegment += 4 * count;
        lastY = y + rect[0].height();
        rect += count;
    }

    for (int i = 0; i < lastSegment; ++i) {
        Segment *segment = &segments[i];
        if (!segment->added)
            addSegmentsToPath(segment, result);
    }

    return result;
}

#if defined(Q_OS_UNIX) || defined(Q_OS_WIN)

//#define QT_REGION_DEBUG
/*
 *   clip region
 */

struct QRegionPrivate {
    int numRects;
    QVector<QRect> rects;
    QRect extents;
    QRect innerRect;
    int innerArea;

    inline QRegionPrivate() : numRects(0), innerArea(-1) {}
    inline QRegionPrivate(const QRect &r) {
        numRects = 1;
        extents = r;
        innerRect = r;
        innerArea = r.width() * r.height();
    }

    inline QRegionPrivate(const QRegionPrivate &r) {
        rects = r.rects;
        numRects = r.numRects;
        extents = r.extents;
        innerRect = r.innerRect;
        innerArea = r.innerArea;
    }

    inline QRegionPrivate &operator=(const QRegionPrivate &r) {
        rects = r.rects;
        numRects = r.numRects;
        extents = r.extents;
        innerRect = r.innerRect;
        innerArea = r.innerArea;
        return *this;
    }

    void intersect(const QRect &r);

    /*
     * Returns \c true if r is guaranteed to be fully contained in this region.
     * A false return value does not guarantee the opposite.
     */
    inline bool contains(const QRegionPrivate &r) const {
        return contains(r.extents);
    }

    inline bool contains(const QRect &r2) const {
        const QRect &r1 = innerRect;
        return r2.left() >= r1.left() && r2.right() <= r1.right()
            && r2.top() >= r1.top() && r2.bottom() <= r1.bottom();
    }

    /*
     * Returns \c true if this region is guaranteed to be fully contained in r.
     */
    inline bool within(const QRect &r1) const {
        const QRect &r2 = extents;
        return r2.left() >= r1.left() && r2.right() <= r1.right()
            && r2.top() >= r1.top() && r2.bottom() <= r1.bottom();
    }

    inline void updateInnerRect(const QRect &rect) {
        const int area = rect.width() * rect.height();
        if (area > innerArea) {
            innerArea = area;
            innerRect = rect;
        }
    }

    inline void vectorize() {
        if (numRects == 1) {
            if (!rects.size())
                rects.resize(1);
            rects[0] = extents;
        }
    }

    inline void append(const QRect *r);
    void append(const QRegionPrivate *r);
    void prepend(const QRect *r);
    void prepend(const QRegionPrivate *r);
    inline bool canAppend(const QRect *r) const;
    inline bool canAppend(const QRegionPrivate *r) const;
    inline bool canPrepend(const QRect *r) const;
    inline bool canPrepend(const QRegionPrivate *r) const;

    inline bool mergeFromRight(QRect *left, const QRect *right);
    inline bool mergeFromLeft(QRect *left, const QRect *right);
    inline bool mergeFromBelow(QRect *top, const QRect *bottom,
                               const QRect *nextToTop,
                               const QRect *nextToBottom);
    inline bool mergeFromAbove(QRect *bottom, const QRect *top,
                               const QRect *nextToBottom,
                               const QRect *nextToTop);

#ifdef QT_REGION_DEBUG
    void selfTest() const;
#endif
};

static inline bool isEmptyHelper(const QRegionPrivate *preg)
{
    return !preg || preg->numRects == 0;
}

static inline bool canMergeFromRight(const QRect *left, const QRect *right)
{
    return (right->top() == left->top()
            && right->bottom() == left->bottom()
            && right->left() <= (left->right() + 1));
}

static inline bool canMergeFromLeft(const QRect *right, const QRect *left)
{
    return canMergeFromRight(left, right);
}

bool QRegionPrivate::mergeFromRight(QRect *left, const QRect *right)
{
    if (canMergeFromRight(left, right)) {
        left->setRight(right->right());
        updateInnerRect(*left);
        return true;
    }
    return false;
}

bool QRegionPrivate::mergeFromLeft(QRect *right, const QRect *left)
{
    if (canMergeFromLeft(right, left)) {
        right->setLeft(left->left());
        updateInnerRect(*right);
        return true;
    }
    return false;
}

static inline bool canMergeFromBelow(const QRect *top, const QRect *bottom,
                                     const QRect *nextToTop,
                                     const QRect *nextToBottom)
{
    if (nextToTop && nextToTop->y() == top->y())
        return false;
    if (nextToBottom && nextToBottom->y() == bottom->y())
        return false;

    return ((top->bottom() >= (bottom->top() - 1))
            && top->left() == bottom->left()
            && top->right() == bottom->right());
}

bool QRegionPrivate::mergeFromBelow(QRect *top, const QRect *bottom,
                                    const QRect *nextToTop,
                                    const QRect *nextToBottom)
{
    if (canMergeFromBelow(top, bottom, nextToTop, nextToBottom)) {
        top->setBottom(bottom->bottom());
        updateInnerRect(*top);
        return true;
    }
    return false;
}

bool QRegionPrivate::mergeFromAbove(QRect *bottom, const QRect *top,
                                    const QRect *nextToBottom,
                                    const QRect *nextToTop)
{
    if (canMergeFromBelow(top, bottom, nextToTop, nextToBottom)) {
        bottom->setTop(top->top());
        updateInnerRect(*bottom);
        return true;
    }
    return false;
}

static inline QRect qt_rect_intersect_normalized(const QRect &r1,
                                                 const QRect &r2)
{
    QRect r;
    r.setLeft(qMax(r1.left(), r2.left()));
    r.setRight(qMin(r1.right(), r2.right()));
    r.setTop(qMax(r1.top(), r2.top()));
    r.setBottom(qMin(r1.bottom(), r2.bottom()));
    return r;
}

void QRegionPrivate::intersect(const QRect &rect)
{
    Q_ASSERT(extents.intersects(rect));
    Q_ASSERT(numRects > 1);

#ifdef QT_REGION_DEBUG
    selfTest();
#endif

    const QRect r = rect.normalized();
    extents = QRect();
    innerRect = QRect();
    innerArea = -1;

    QRect *dest = rects.data();
    const QRect *src = dest;
    int n = numRects;
    numRects = 0;
    while (n--) {
        *dest = qt_rect_intersect_normalized(*src++, r);
        if (dest->isEmpty())
            continue;

        if (numRects == 0) {
            extents = *dest;
        } else {
            extents.setLeft(qMin(extents.left(), dest->left()));
            // hw: extents.top() will never change after initialization
            //extents.setTop(qMin(extents.top(), dest->top()));
            extents.setRight(qMax(extents.right(), dest->right()));
            extents.setBottom(qMax(extents.bottom(), dest->bottom()));

            const QRect *nextToLast = (numRects > 1 ? dest - 2 : 0);

            // mergeFromBelow inlined and optimized
            if (canMergeFromBelow(dest - 1, dest, nextToLast, 0)) {
                if (!n || src->y() != dest->y() || src->left() > r.right()) {
                    QRect *prev = dest - 1;
                    prev->setBottom(dest->bottom());
                    updateInnerRect(*prev);
                    continue;
                }
            }
        }
        updateInnerRect(*dest);
        ++dest;
        ++numRects;
    }
#ifdef QT_REGION_DEBUG
    selfTest();
#endif
}

void QRegionPrivate::append(const QRect *r)
{
    Q_ASSERT(!r->isEmpty());

    QRect *myLast = (numRects == 1 ? &extents : rects.data() + (numRects - 1));
    if (mergeFromRight(myLast, r)) {
        if (numRects > 1) {
            const QRect *nextToTop = (numRects > 2 ? myLast - 2 : 0);
            if (mergeFromBelow(myLast - 1, myLast, nextToTop, 0))
                --numRects;
        }
    } else if (mergeFromBelow(myLast, r, (numRects > 1 ? myLast - 1 : 0), 0)) {
        // nothing
    } else {
        vectorize();
        ++numRects;
        updateInnerRect(*r);
        if (rects.size() < numRects)
            rects.resize(numRects);
        rects[numRects - 1] = *r;
    }
    extents.setCoords(qMin(extents.left(), r->left()),
                      qMin(extents.top(), r->top()),
                      qMax(extents.right(), r->right()),
                      qMax(extents.bottom(), r->bottom()));

#ifdef QT_REGION_DEBUG
    selfTest();
#endif
}

void QRegionPrivate::append(const QRegionPrivate *r)
{
    Q_ASSERT(!isEmptyHelper(r));

    if (r->numRects == 1) {
        append(&r->extents);
        return;
    }

    vectorize();

    QRect *destRect = rects.data() + numRects;
    const QRect *srcRect = r->rects.constData();
    int numAppend = r->numRects;

    // try merging
    {
        const QRect *rFirst = srcRect;
        QRect *myLast = destRect - 1;
        const QRect *nextToLast = (numRects > 1 ? myLast - 1 : 0);
        if (mergeFromRight(myLast, rFirst)) {
            ++srcRect;
            --numAppend;
            const QRect *rNextToFirst = (numAppend > 1 ? rFirst + 2 : 0);
            if (mergeFromBelow(myLast, rFirst + 1, nextToLast, rNextToFirst)) {
                ++srcRect;
                --numAppend;
            }
            if (numRects  > 1) {
                nextToLast = (numRects > 2 ? myLast - 2 : 0);
                rNextToFirst = (numAppend > 0 ? srcRect : 0);
                if (mergeFromBelow(myLast - 1, myLast, nextToLast, rNextToFirst)) {
                    --destRect;
                    --numRects;
                }
            }
        } else if (mergeFromBelow(myLast, rFirst, nextToLast, rFirst + 1)) {
            ++srcRect;
            --numAppend;
        }
    }

    // append rectangles
    if (numAppend > 0) {
        const int newNumRects = numRects + numAppend;
        if (newNumRects > rects.size()) {
            rects.resize(newNumRects);
            destRect = rects.data() + numRects;
        }
        memcpy(destRect, srcRect, numAppend * sizeof(QRect));

        numRects = newNumRects;
    }

    // update inner rectangle
    if (innerArea < r->innerArea) {
        innerArea = r->innerArea;
        innerRect = r->innerRect;
    }

    // update extents
    destRect = &extents;
    srcRect = &r->extents;
    extents.setCoords(qMin(destRect->left(), srcRect->left()),
                      qMin(destRect->top(), srcRect->top()),
                      qMax(destRect->right(), srcRect->right()),
                      qMax(destRect->bottom(), srcRect->bottom()));

#ifdef QT_REGION_DEBUG
    selfTest();
#endif
}

void QRegionPrivate::prepend(const QRegionPrivate *r)
{
    Q_ASSERT(!isEmptyHelper(r));

    if (r->numRects == 1) {
        prepend(&r->extents);
        return;
    }

    vectorize();

    int numPrepend = r->numRects;
    int numSkip = 0;

    // try merging
    {
        QRect *myFirst = rects.data();
        const QRect *nextToFirst = (numRects > 1 ? myFirst + 1 : 0);
        const QRect *rLast = r->rects.constData() + r->numRects - 1;
        const QRect *rNextToLast = (r->numRects > 1 ? rLast - 1 : 0);
        if (mergeFromLeft(myFirst, rLast)) {
            --numPrepend;
            --rLast;
            rNextToLast = (numPrepend > 1 ? rLast - 1 : 0);
            if (mergeFromAbove(myFirst, rLast, nextToFirst, rNextToLast)) {
                --numPrepend;
                --rLast;
            }
            if (numRects  > 1) {
                nextToFirst = (numRects > 2? myFirst + 2 : 0);
                rNextToLast = (numPrepend > 0 ? rLast : 0);
                if (mergeFromAbove(myFirst + 1, myFirst, nextToFirst, rNextToLast)) {
                    --numRects;
                    ++numSkip;
                }
            }
        } else if (mergeFromAbove(myFirst, rLast, nextToFirst, rNextToLast)) {
            --numPrepend;
        }
    }

    if (numPrepend > 0) {
        const int newNumRects = numRects + numPrepend;
        if (newNumRects > rects.size())
            rects.resize(newNumRects);

        // move existing rectangles
        memmove(rects.data() + numPrepend, rects.constData() + numSkip,
                numRects * sizeof(QRect));

        // prepend new rectangles
        memcpy(rects.data(), r->rects.constData(), numPrepend * sizeof(QRect));

        numRects = newNumRects;
    }

    // update inner rectangle
    if (innerArea < r->innerArea) {
        innerArea = r->innerArea;
        innerRect = r->innerRect;
    }

    // update extents
    extents.setCoords(qMin(extents.left(), r->extents.left()),
                      qMin(extents.top(), r->extents.top()),
                      qMax(extents.right(), r->extents.right()),
                      qMax(extents.bottom(), r->extents.bottom()));

#ifdef QT_REGION_DEBUG
    selfTest();
#endif
}

void QRegionPrivate::prepend(const QRect *r)
{
    Q_ASSERT(!r->isEmpty());

    QRect *myFirst = (numRects == 1 ? &extents : rects.data());
    if (mergeFromLeft(myFirst, r)) {
        if (numRects > 1) {
            const QRect *nextToFirst = (numRects > 2 ? myFirst + 2 : 0);
            if (mergeFromAbove(myFirst + 1, myFirst, nextToFirst, 0)) {
                --numRects;
                memmove(rects.data(), rects.constData() + 1,
                        numRects * sizeof(QRect));
            }
        }
    } else if (mergeFromAbove(myFirst, r, (numRects > 1 ? myFirst + 1 : 0), 0)) {
        // nothing
    } else {
        vectorize();
        ++numRects;
        updateInnerRect(*r);
        rects.prepend(*r);
    }
    extents.setCoords(qMin(extents.left(), r->left()),
                      qMin(extents.top(), r->top()),
                      qMax(extents.right(), r->right()),
                      qMax(extents.bottom(), r->bottom()));

#ifdef QT_REGION_DEBUG
    selfTest();
#endif
}

bool QRegionPrivate::canAppend(const QRect *r) const
{
    Q_ASSERT(!r->isEmpty());

    const QRect *myLast = (numRects == 1) ? &extents : (rects.constData() + (numRects - 1));
    if (r->top() > myLast->bottom())
        return true;
    if (r->top() == myLast->top()
        && r->height() == myLast->height()
        && r->left() > myLast->right())
    {
        return true;
    }

    return false;
}

bool QRegionPrivate::canAppend(const QRegionPrivate *r) const
{
    return canAppend(r->numRects == 1 ? &r->extents : r->rects.constData());
}

bool QRegionPrivate::canPrepend(const QRect *r) const
{
    Q_ASSERT(!r->isEmpty());

    const QRect *myFirst = (numRects == 1) ? &extents : rects.constData();
    if (r->bottom() < myFirst->top()) // not overlapping
        return true;
    if (r->top() == myFirst->top()
        && r->height() == myFirst->height()
        && r->right() < myFirst->left())
    {
        return true;
    }

    return false;
}

bool QRegionPrivate::canPrepend(const QRegionPrivate *r) const
{
    return canPrepend(r->numRects == 1 ? &r->extents : r->rects.constData() + r->numRects - 1);
}

#ifdef QT_REGION_DEBUG
void QRegionPrivate::selfTest() const
{
    if (numRects == 0) {
        Q_ASSERT(extents.isEmpty());
        Q_ASSERT(innerRect.isEmpty());
        return;
    }

    Q_ASSERT(innerArea == (innerRect.width() * innerRect.height()));

    if (numRects == 1) {
        Q_ASSERT(innerRect == extents);
        Q_ASSERT(!innerRect.isEmpty());
        return;
    }

    for (int i = 0; i < numRects; ++i) {
        const QRect r = rects.at(i);
        if ((r.width() * r.height()) > innerArea)
            qDebug() << "selfTest(): innerRect" << innerRect << '<' << r;
    }

    QRect r = rects.first();
    for (int i = 1; i < numRects; ++i) {
        const QRect r2 = rects.at(i);
        Q_ASSERT(!r2.isEmpty());
        if (r2.y() == r.y()) {
            Q_ASSERT(r.bottom() == r2.bottom());
            Q_ASSERT(r.right() < (r2.left() + 1));
        } else {
            Q_ASSERT(r2.y() >= r.bottom());
        }
        r = r2;
    }
}
#endif // QT_REGION_DEBUG

static QRegionPrivate qrp;
QRegion::QRegionData QRegion::shared_empty = {Q_BASIC_ATOMIC_INITIALIZER(1), &qrp};

typedef void (*OverlapFunc)(QRegionPrivate &dest, const QRect *r1, const QRect *r1End,
                            const QRect *r2, const QRect *r2End, int y1, int y2);
typedef void (*NonOverlapFunc)(QRegionPrivate &dest, const QRect *r, const QRect *rEnd,
                               int y1, int y2);

static bool EqualRegion(const QRegionPrivate *r1, const QRegionPrivate *r2);
static void UnionRegion(const QRegionPrivate *reg1, const QRegionPrivate *reg2, QRegionPrivate &dest);
static void miRegionOp(QRegionPrivate &dest, const QRegionPrivate *reg1, const QRegionPrivate *reg2,
                       OverlapFunc overlapFunc, NonOverlapFunc nonOverlap1Func,
                       NonOverlapFunc nonOverlap2Func);

#define RectangleOut 0
#define RectangleIn 1
#define RectanglePart 2
#define EvenOddRule 0
#define WindingRule 1

// START OF region.h extract
/* $XConsortium: region.h,v 11.14 94/04/17 20:22:20 rws Exp $ */
/************************************************************************

Copyright (c) 1987  X Consortium

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
X CONSORTIUM BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of the X Consortium shall not be
used in advertising or otherwise to promote the sale, use or other dealings
in this Software without prior written authorization from the X Consortium.


Copyright 1987 by Digital Equipment Corporation, Maynard, Massachusetts.

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its
documentation for any purpose and without fee is hereby granted,
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in
supporting documentation, and that the name of Digital not be
used in advertising or publicity pertaining to distribution of the
software without specific, written prior permission.

DIGITAL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
DIGITAL BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
SOFTWARE.

************************************************************************/

#ifndef _XREGION_H
#define _XREGION_H

QT_BEGIN_INCLUDE_NAMESPACE
#include <limits.h>
QT_END_INCLUDE_NAMESPACE

/*  1 if two BOXes overlap.
 *  0 if two BOXes do not overlap.
 *  Remember, x2 and y2 are not in the region
 */
#define EXTENTCHECK(r1, r2) \
        ((r1)->right() >= (r2)->left() && \
         (r1)->left() <= (r2)->right() && \
         (r1)->bottom() >= (r2)->top() && \
         (r1)->top() <= (r2)->bottom())

/*
 *  update region extents
 */
#define EXTENTS(r,idRect){\
            if((r)->left() < (idRect)->extents.left())\
              (idRect)->extents.setLeft((r)->left());\
            if((r)->top() < (idRect)->extents.top())\
              (idRect)->extents.setTop((r)->top());\
            if((r)->right() > (idRect)->extents.right())\
              (idRect)->extents.setRight((r)->right());\
            if((r)->bottom() > (idRect)->extents.bottom())\
              (idRect)->extents.setBottom((r)->bottom());\
        }

/*
 *   Check to see if there is enough memory in the present region.
 */
#define MEMCHECK(dest, rect, firstrect){\
        if ((dest).numRects >= ((dest).rects.size()-1)){\
          firstrect.resize(firstrect.size() * 2); \
          (rect) = (firstrect).data() + (dest).numRects;\
        }\
      }


/*
 * number of points to buffer before sending them off
 * to scanlines(): Must be an even number
 */
#define NUMPTSTOBUFFER 200

/*
 * used to allocate buffers for points and link
 * the buffers together
 */
typedef struct _POINTBLOCK {
    char data[NUMPTSTOBUFFER * sizeof(QPoint)];
    QPoint *pts;
    struct _POINTBLOCK *next;
} POINTBLOCK;

#endif
// END OF region.h extract

// START OF Region.c extract
/* $XConsortium: Region.c /main/30 1996/10/22 14:21:24 kaleb $ */
/************************************************************************

Copyright (c) 1987, 1988  X Consortium

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
X CONSORTIUM BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of the X Consortium shall not be
used in advertising or otherwise to promote the sale, use or other dealings
in this Software without prior written authorization from the X Consortium.


Copyright 1987, 1988 by Digital Equipment Corporation, Maynard, Massachusetts.

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its
documentation for any purpose and without fee is hereby granted,
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in
supporting documentation, and that the name of Digital not be
used in advertising or publicity pertaining to distribution of the
software without specific, written prior permission.

DIGITAL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
DIGITAL BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
SOFTWARE.

************************************************************************/
/*
 * The functions in this file implement the Region abstraction, similar to one
 * used in the X11 sample server. A Region is simply an area, as the name
 * implies, and is implemented as a "y-x-banded" array of rectangles. To
 * explain: Each Region is made up of a certain number of rectangles sorted
 * by y coordinate first, and then by x coordinate.
 *
 * Furthermore, the rectangles are banded such that every rectangle with a
 * given upper-left y coordinate (y1) will have the same lower-right y
 * coordinate (y2) and vice versa. If a rectangle has scanlines in a band, it
 * will span the entire vertical distance of the band. This means that some
 * areas that could be merged into a taller rectangle will be represented as
 * several shorter rectangles to account for shorter rectangles to its left
 * or right but within its "vertical scope".
 *
 * An added constraint on the rectangles is that they must cover as much
 * horizontal area as possible. E.g. no two rectangles in a band are allowed
 * to touch.
 *
 * Whenever possible, bands will be merged together to cover a greater vertical
 * distance (and thus reduce the number of rectangles). Two bands can be merged
 * only if the bottom of one touches the top of the other and they have
 * rectangles in the same places (of the same width, of course). This maintains
 * the y-x-banding that's so nice to have...
 */
/* $XFree86: xc/lib/X11/Region.c,v 1.1.1.2.2.2 1998/10/04 15:22:50 hohndel Exp $ */

static void UnionRectWithRegion(const QRect *rect, const QRegionPrivate *source,
                                QRegionPrivate &dest)
{
    if (rect->isEmpty())
        return;

    Q_ASSERT(EqualRegion(source, &dest));

    if (dest.numRects == 0) {
        dest = QRegionPrivate(*rect);
    } else if (dest.canAppend(rect)) {
        dest.append(rect);
    } else {
        QRegionPrivate p(*rect);
        UnionRegion(&p, source, dest);
    }
}

/*-
 *-----------------------------------------------------------------------
 * miSetExtents --
 *      Reset the extents and innerRect of a region to what they should be.
 *      Called by miSubtract and miIntersect b/c they can't figure it out
 *      along the way or do so easily, as miUnion can.
 *
 * Results:
 *      None.
 *
 * Side Effects:
 *      The region's 'extents' and 'innerRect' structure is overwritten.
 *
 *-----------------------------------------------------------------------
 */
static void miSetExtents(QRegionPrivate &dest)
{
    const QRect *pBox,
                         *pBoxEnd;
    QRect *pExtents;

    dest.innerRect.setCoords(0, 0, -1, -1);
    dest.innerArea = -1;
    if (dest.numRects == 0) {
        dest.extents.setCoords(0, 0, -1, -1);
        return;
    }

    pExtents = &dest.extents;
    if (dest.rects.isEmpty())
        pBox = &dest.extents;
    else
        pBox = dest.rects.constData();
    pBoxEnd = pBox + dest.numRects - 1;

    /*
     * Since pBox is the first rectangle in the region, it must have the
     * smallest y1 and since pBoxEnd is the last rectangle in the region,
     * it must have the largest y2, because of banding. Initialize x1 and
     * x2 from  pBox and pBoxEnd, resp., as good things to initialize them
     * to...
     */
    pExtents->setLeft(pBox->left());
    pExtents->setTop(pBox->top());
    pExtents->setRight(pBoxEnd->right());
    pExtents->setBottom(pBoxEnd->bottom());

    Q_ASSERT(pExtents->top() <= pExtents->bottom());
    while (pBox <= pBoxEnd) {
        if (pBox->left() < pExtents->left())
            pExtents->setLeft(pBox->left());
        if (pBox->right() > pExtents->right())
            pExtents->setRight(pBox->right());
        dest.updateInnerRect(*pBox);
        ++pBox;
    }
    Q_ASSERT(pExtents->left() <= pExtents->right());
}

/* TranslateRegion(pRegion, x, y)
   translates in place
   added by raymond
*/

static void OffsetRegion(QRegionPrivate &region, int x, int y)
{
    if (region.rects.size()) {
        QRect *pbox = region.rects.data();
        int nbox = region.numRects;

        while (nbox--) {
            pbox->translate(x, y);
            ++pbox;
        }
    }
    region.extents.translate(x, y);
    region.innerRect.translate(x, y);
}

/*======================================================================
 *          Region Intersection
 *====================================================================*/
/*-
 *-----------------------------------------------------------------------
 * miIntersectO --
 *      Handle an overlapping band for miIntersect.
 *
 * Results:
 *      None.
 *
 * Side Effects:
 *      Rectangles may be added to the region.
 *
 *-----------------------------------------------------------------------
 */
static void miIntersectO(QRegionPrivate &dest, const QRect *r1, const QRect *r1End,
                         const QRect *r2, const QRect *r2End, int y1, int y2)
{
    int x1;
    int x2;
    QRect *pNextRect;

    pNextRect = dest.rects.data() + dest.numRects;

    while (r1 != r1End && r2 != r2End) {
        x1 = qMax(r1->left(), r2->left());
        x2 = qMin(r1->right(), r2->right());

        /*
         * If there's any overlap between the two rectangles, add that
         * overlap to the new region.
         * There's no need to check for subsumption because the only way
         * such a need could arise is if some region has two rectangles
         * right next to each other. Since that should never happen...
         */
        if (x1 <= x2) {
            Q_ASSERT(y1 <= y2);
            MEMCHECK(dest, pNextRect, dest.rects)
            pNextRect->setCoords(x1, y1, x2, y2);
            ++dest.numRects;
            ++pNextRect;
        }

        /*
         * Need to advance the pointers. Shift the one that extends
         * to the right the least, since the other still has a chance to
         * overlap with that region's next rectangle, if you see what I mean.
         */
        if (r1->right() < r2->right()) {
            ++r1;
        } else if (r2->right() < r1->right()) {
            ++r2;
        } else {
            ++r1;
            ++r2;
        }
    }
}

/*======================================================================
 *          Generic Region Operator
 *====================================================================*/

/*-
 *-----------------------------------------------------------------------
 * miCoalesce --
 *      Attempt to merge the boxes in the current band with those in the
 *      previous one. Used only by miRegionOp.
 *
 * Results:
 *      The new index for the previous band.
 *
 * Side Effects:
 *      If coalescing takes place:
 *          - rectangles in the previous band will have their y2 fields
 *            altered.
 *          - dest.numRects will be decreased.
 *
 *-----------------------------------------------------------------------
 */
static int miCoalesce(QRegionPrivate &dest, int prevStart, int curStart)
{
    QRect *pPrevBox;   /* Current box in previous band */
    QRect *pCurBox;    /* Current box in current band */
    QRect *pRegEnd;    /* End of region */
    int curNumRects;    /* Number of rectangles in current band */
    int prevNumRects;   /* Number of rectangles in previous band */
    int bandY1;         /* Y1 coordinate for current band */
    QRect *rData = dest.rects.data();

    pRegEnd = rData + dest.numRects;

    pPrevBox = rData + prevStart;
    prevNumRects = curStart - prevStart;

    /*
     * Figure out how many rectangles are in the current band. Have to do
     * this because multiple bands could have been added in miRegionOp
     * at the end when one region has been exhausted.
     */
    pCurBox = rData + curStart;
    bandY1 = pCurBox->top();
    for (curNumRects = 0; pCurBox != pRegEnd && pCurBox->top() == bandY1; ++curNumRects) {
        ++pCurBox;
    }

    if (pCurBox != pRegEnd) {
        /*
         * If more than one band was added, we have to find the start
         * of the last band added so the next coalescing job can start
         * at the right place... (given when multiple bands are added,
         * this may be pointless -- see above).
         */
        --pRegEnd;
        while ((pRegEnd - 1)->top() == pRegEnd->top())
            --pRegEnd;
        curStart = pRegEnd - rData;
        pRegEnd = rData + dest.numRects;
    }

    if (curNumRects == prevNumRects && curNumRects != 0) {
        pCurBox -= curNumRects;
        /*
         * The bands may only be coalesced if the bottom of the previous
         * matches the top scanline of the current.
         */
        if (pPrevBox->bottom() == pCurBox->top() - 1) {
            /*
             * Make sure the bands have boxes in the same places. This
             * assumes that boxes have been added in such a way that they
             * cover the most area possible. I.e. two boxes in a band must
             * have some horizontal space between them.
             */
            do {
                if (pPrevBox->left() != pCurBox->left() || pPrevBox->right() != pCurBox->right()) {
                     // The bands don't line up so they can't be coalesced.
                    return curStart;
                }
                ++pPrevBox;
                ++pCurBox;
                --prevNumRects;
            } while (prevNumRects != 0);

            dest.numRects -= curNumRects;
            pCurBox -= curNumRects;
            pPrevBox -= curNumRects;

            /*
             * The bands may be merged, so set the bottom y of each box
             * in the previous band to that of the corresponding box in
             * the current band.
             */
            do {
                pPrevBox->setBottom(pCurBox->bottom());
                dest.updateInnerRect(*pPrevBox);
                ++pPrevBox;
                ++pCurBox;
                curNumRects -= 1;
            } while (curNumRects != 0);

            /*
             * If only one band was added to the region, we have to backup
             * curStart to the start of the previous band.
             *
             * If more than one band was added to the region, copy the
             * other bands down. The assumption here is that the other bands
             * came from the same region as the current one and no further
             * coalescing can be done on them since it's all been done
             * already... curStart is already in the right place.
             */
            if (pCurBox == pRegEnd) {
                curStart = prevStart;
            } else {
                do {
                    *pPrevBox++ = *pCurBox++;
                    dest.updateInnerRect(*pPrevBox);
                } while (pCurBox != pRegEnd);
            }
        }
    }
    return curStart;
}

/*-
 *-----------------------------------------------------------------------
 * miRegionOp --
 *      Apply an operation to two regions. Called by miUnion, miInverse,
 *      miSubtract, miIntersect...
 *
 * Results:
 *      None.
 *
 * Side Effects:
 *      The new region is overwritten.
 *
 * Notes:
 *      The idea behind this function is to view the two regions as sets.
 *      Together they cover a rectangle of area that this function divides
 *      into horizontal bands where points are covered only by one region
 *      or by both. For the first case, the nonOverlapFunc is called with
 *      each the band and the band's upper and lower extents. For the
 *      second, the overlapFunc is called to process the entire band. It
 *      is responsible for clipping the rectangles in the band, though
 *      this function provides the boundaries.
 *      At the end of each band, the new region is coalesced, if possible,
 *      to reduce the number of rectangles in the region.
 *
 *-----------------------------------------------------------------------
 */
static void miRegionOp(QRegionPrivate &dest,
                       const QRegionPrivate *reg1, const QRegionPrivate *reg2,
                       OverlapFunc overlapFunc, NonOverlapFunc nonOverlap1Func,
                       NonOverlapFunc nonOverlap2Func)
{
    const QRect *r1;         // Pointer into first region
    const QRect *r2;         // Pointer into 2d region
    const QRect *r1End;               // End of 1st region
    const QRect *r2End;               // End of 2d region
    int ybot;          // Bottom of intersection
    int ytop;          // Top of intersection
    int prevBand;               // Index of start of previous band in dest
    int curBand;                // Index of start of current band in dest
    const QRect *r1BandEnd;  // End of current band in r1
    const QRect *r2BandEnd;  // End of current band in r2
    int top;                    // Top of non-overlapping band
    int bot;                    // Bottom of non-overlapping band

    /*
     * Initialization:
     *  set r1, r2, r1End and r2End appropriately, preserve the important
     * parts of the destination region until the end in case it's one of
     * the two source regions, then mark the "new" region empty, allocating
     * another array of rectangles for it to use.
     */
    if (reg1->numRects == 1)
        r1 = &reg1->extents;
    else
        r1 = reg1->rects.constData();
    if (reg2->numRects == 1)
        r2 = &reg2->extents;
    else
        r2 = reg2->rects.constData();

    r1End = r1 + reg1->numRects;
    r2End = r2 + reg2->numRects;

    dest.vectorize();

    QVector<QRect> oldRects = dest.rects;

    dest.numRects = 0;

    /*
     * Allocate a reasonable number of rectangles for the new region. The idea
     * is to allocate enough so the individual functions don't need to
     * reallocate and copy the array, which is time consuming, yet we don't
     * have to worry about using too much memory. I hope to be able to
     * nuke the realloc() at the end of this function eventually.
     */
    dest.rects.resize(qMax(reg1->numRects,reg2->numRects) * 2);

    /*
     * Initialize ybot and ytop.
     * In the upcoming loop, ybot and ytop serve different functions depending
     * on whether the band being handled is an overlapping or non-overlapping
     * band.
     *  In the case of a non-overlapping band (only one of the regions
     * has points in the band), ybot is the bottom of the most recent
     * intersection and thus clips the top of the rectangles in that band.
     * ytop is the top of the next intersection between the two regions and
     * serves to clip the bottom of the rectangles in the current band.
     *  For an overlapping band (where the two regions intersect), ytop clips
     * the top of the rectangles of both regions and ybot clips the bottoms.
     */
    if (reg1->extents.top() < reg2->extents.top())
        ybot = reg1->extents.top() - 1;
    else
        ybot = reg2->extents.top() - 1;

    /*
     * prevBand serves to mark the start of the previous band so rectangles
     * can be coalesced into larger rectangles. qv. miCoalesce, above.
     * In the beginning, there is no previous band, so prevBand == curBand
     * (curBand is set later on, of course, but the first band will always
     * start at index 0). prevBand and curBand must be indices because of
     * the possible expansion, and resultant moving, of the new region's
     * array of rectangles.
     */
    prevBand = 0;

    do {
        curBand = dest.numRects;

        /*
         * This algorithm proceeds one source-band (as opposed to a
         * destination band, which is determined by where the two regions
         * intersect) at a time. r1BandEnd and r2BandEnd serve to mark the
         * rectangle after the last one in the current band for their
         * respective regions.
         */
        r1BandEnd = r1;
        while (r1BandEnd != r1End && r1BandEnd->top() == r1->top())
            ++r1BandEnd;

        r2BandEnd = r2;
        while (r2BandEnd != r2End && r2BandEnd->top() == r2->top())
            ++r2BandEnd;

        /*
         * First handle the band that doesn't intersect, if any.
         *
         * Note that attention is restricted to one band in the
         * non-intersecting region at once, so if a region has n
         * bands between the current position and the next place it overlaps
         * the other, this entire loop will be passed through n times.
         */
        if (r1->top() < r2->top()) {
            top = qMax(r1->top(), ybot + 1);
            bot = qMin(r1->bottom(), r2->top() - 1);

            if (nonOverlap1Func != 0 && bot >= top)
                (*nonOverlap1Func)(dest, r1, r1BandEnd, top, bot);
            ytop = r2->top();
        } else if (r2->top() < r1->top()) {
            top = qMax(r2->top(), ybot + 1);
            bot = qMin(r2->bottom(), r1->top() - 1);

            if (nonOverlap2Func != 0 && bot >= top)
                (*nonOverlap2Func)(dest, r2, r2BandEnd, top, bot);
            ytop = r1->top();
        } else {
            ytop = r1->top();
        }

        /*
         * If any rectangles got added to the region, try and coalesce them
         * with rectangles from the previous band. Note we could just do
         * this test in miCoalesce, but some machines incur a not
         * inconsiderable cost for function calls, so...
         */
        if (dest.numRects != curBand)
            prevBand = miCoalesce(dest, prevBand, curBand);

        /*
         * Now see if we've hit an intersecting band. The two bands only
         * intersect if ybot >= ytop
         */
        ybot = qMin(r1->bottom(), r2->bottom());
        curBand = dest.numRects;
        if (ybot >= ytop)
            (*overlapFunc)(dest, r1, r1BandEnd, r2, r2BandEnd, ytop, ybot);

        if (dest.numRects != curBand)
            prevBand = miCoalesce(dest, prevBand, curBand);

        /*
         * If we've finished with a band (y2 == ybot) we skip forward
         * in the region to the next band.
         */
        if (r1->bottom() == ybot)
            r1 = r1BandEnd;
        if (r2->bottom() == ybot)
            r2 = r2BandEnd;
    } while (r1 != r1End && r2 != r2End);

    /*
     * Deal with whichever region still has rectangles left.
     */
    curBand = dest.numRects;
    if (r1 != r1End) {
        if (nonOverlap1Func != 0) {
            do {
                r1BandEnd = r1;
                while (r1BandEnd < r1End && r1BandEnd->top() == r1->top())
                    ++r1BandEnd;
                (*nonOverlap1Func)(dest, r1, r1BandEnd, qMax(r1->top(), ybot + 1), r1->bottom());
                r1 = r1BandEnd;
            } while (r1 != r1End);
        }
    } else if ((r2 != r2End) && (nonOverlap2Func != 0)) {
        do {
            r2BandEnd = r2;
            while (r2BandEnd < r2End && r2BandEnd->top() == r2->top())
                 ++r2BandEnd;
            (*nonOverlap2Func)(dest, r2, r2BandEnd, qMax(r2->top(), ybot + 1), r2->bottom());
            r2 = r2BandEnd;
        } while (r2 != r2End);
    }

    if (dest.numRects != curBand)
        (void)miCoalesce(dest, prevBand, curBand);

    /*
     * A bit of cleanup. To keep regions from growing without bound,
     * we shrink the array of rectangles to match the new number of
     * rectangles in the region.
     *
     * Only do this stuff if the number of rectangles allocated is more than
     * twice the number of rectangles in the region (a simple optimization).
     */
    if (qMax(4, dest.numRects) < (dest.rects.size() >> 1))
        dest.rects.resize(dest.numRects);
}

/*======================================================================
 *          Region Union
 *====================================================================*/

/*-
 *-----------------------------------------------------------------------
 * miUnionNonO --
 *      Handle a non-overlapping band for the union operation. Just
 *      Adds the rectangles into the region. Doesn't have to check for
 *      subsumption or anything.
 *
 * Results:
 *      None.
 *
 * Side Effects:
 *      dest.numRects is incremented and the final rectangles overwritten
 *      with the rectangles we're passed.
 *
 *-----------------------------------------------------------------------
 */

static void miUnionNonO(QRegionPrivate &dest, const QRect *r, const QRect *rEnd,
                        int y1, int y2)
{
    QRect *pNextRect;

    pNextRect = dest.rects.data() + dest.numRects;

    Q_ASSERT(y1 <= y2);

    while (r != rEnd) {
        Q_ASSERT(r->left() <= r->right());
        MEMCHECK(dest, pNextRect, dest.rects)
        pNextRect->setCoords(r->left(), y1, r->right(), y2);
        dest.numRects++;
        ++pNextRect;
        ++r;
    }
}


/*-
 *-----------------------------------------------------------------------
 * miUnionO --
 *      Handle an overlapping band for the union operation. Picks the
 *      left-most rectangle each time and merges it into the region.
 *
 * Results:
 *      None.
 *
 * Side Effects:
 *      Rectangles are overwritten in dest.rects and dest.numRects will
 *      be changed.
 *
 *-----------------------------------------------------------------------
 */

static void miUnionO(QRegionPrivate &dest, const QRect *r1, const QRect *r1End,
                     const QRect *r2, const QRect *r2End, int y1, int y2)
{
    QRect *pNextRect;

    pNextRect = dest.rects.data() + dest.numRects;

#define MERGERECT(r)             \
    if ((dest.numRects != 0) &&  \
        (pNextRect[-1].top() == y1) &&  \
        (pNextRect[-1].bottom() == y2) &&  \
        (pNextRect[-1].right() >= r->left()-1)) { \
        if (pNextRect[-1].right() < r->right()) { \
            pNextRect[-1].setRight(r->right());  \
            dest.updateInnerRect(pNextRect[-1]); \
            Q_ASSERT(pNextRect[-1].left() <= pNextRect[-1].right()); \
        }  \
    } else { \
        MEMCHECK(dest, pNextRect, dest.rects)  \
        pNextRect->setCoords(r->left(), y1, r->right(), y2); \
        dest.updateInnerRect(*pNextRect); \
        dest.numRects++;  \
        pNextRect++;  \
    }  \
    r++;

    Q_ASSERT(y1 <= y2);
    while (r1 != r1End && r2 != r2End) {
        if (r1->left() < r2->left()) {
            MERGERECT(r1)
        } else {
            MERGERECT(r2)
        }
    }

    if (r1 != r1End) {
        do {
            MERGERECT(r1)
        } while (r1 != r1End);
    } else {
        while (r2 != r2End) {
            MERGERECT(r2)
        }
    }
}

static void UnionRegion(const QRegionPrivate *reg1, const QRegionPrivate *reg2, QRegionPrivate &dest)
{
    Q_ASSERT(!isEmptyHelper(reg1) && !isEmptyHelper(reg2));
    Q_ASSERT(!reg1->contains(*reg2));
    Q_ASSERT(!reg2->contains(*reg1));
    Q_ASSERT(!EqualRegion(reg1, reg2));
    Q_ASSERT(!reg1->canAppend(reg2));
    Q_ASSERT(!reg2->canAppend(reg1));

    if (reg1->innerArea > reg2->innerArea) {
        dest.innerArea = reg1->innerArea;
        dest.innerRect = reg1->innerRect;
    } else {
        dest.innerArea = reg2->innerArea;
        dest.innerRect = reg2->innerRect;
    }
    miRegionOp(dest, reg1, reg2, miUnionO, miUnionNonO, miUnionNonO);

    dest.extents.setCoords(qMin(reg1->extents.left(), reg2->extents.left()),
                           qMin(reg1->extents.top(), reg2->extents.top()),
                           qMax(reg1->extents.right(), reg2->extents.right()),
                           qMax(reg1->extents.bottom(), reg2->extents.bottom()));
}

/*======================================================================
 *        Region Subtraction
 *====================================================================*/

/*-
 *-----------------------------------------------------------------------
 * miSubtractNonO --
 *      Deal with non-overlapping band for subtraction. Any parts from
 *      region 2 we discard. Anything from region 1 we add to the region.
 *
 * Results:
 *      None.
 *
 * Side Effects:
 *      dest may be affected.
 *
 *-----------------------------------------------------------------------
 */

static void miSubtractNonO1(QRegionPrivate &dest, const QRect *r,
                            const QRect *rEnd, int y1, int y2)
{
    QRect *pNextRect;

    pNextRect = dest.rects.data() + dest.numRects;

    Q_ASSERT(y1<=y2);

    while (r != rEnd) {
        Q_ASSERT(r->left() <= r->right());
        MEMCHECK(dest, pNextRect, dest.rects)
        pNextRect->setCoords(r->left(), y1, r->right(), y2);
        ++dest.numRects;
        ++pNextRect;
        ++r;
    }
}

/*-
 *-----------------------------------------------------------------------
 * miSubtractO --
 *      Overlapping band subtraction. x1 is the left-most point not yet
 *      checked.
 *
 * Results:
 *      None.
 *
 * Side Effects:
 *      dest may have rectangles added to it.
 *
 *-----------------------------------------------------------------------
 */

static void miSubtractO(QRegionPrivate &dest, const QRect *r1, const QRect *r1End,
                        const QRect *r2, const QRect *r2End, int y1, int y2)
{
    QRect *pNextRect;
    int x1;

    x1 = r1->left();

    Q_ASSERT(y1 <= y2);
    pNextRect = dest.rects.data() + dest.numRects;

    while (r1 != r1End && r2 != r2End) {
        if (r2->right() < x1) {
            /*
             * Subtrahend missed the boat: go to next subtrahend.
             */
            ++r2;
        } else if (r2->left() <= x1) {
            /*
             * Subtrahend precedes minuend: nuke left edge of minuend.
             */
            x1 = r2->right() + 1;
            if (x1 > r1->right()) {
                /*
                 * Minuend completely covered: advance to next minuend and
                 * reset left fence to edge of new minuend.
                 */
                ++r1;
                if (r1 != r1End)
                    x1 = r1->left();
            } else {
                // Subtrahend now used up since it doesn't extend beyond minuend
                ++r2;
            }
        } else if (r2->left() <= r1->right()) {
            /*
             * Left part of subtrahend covers part of minuend: add uncovered
             * part of minuend to region and skip to next subtrahend.
             */
            Q_ASSERT(x1 < r2->left());
            MEMCHECK(dest, pNextRect, dest.rects)
            pNextRect->setCoords(x1, y1, r2->left() - 1, y2);
            ++dest.numRects;
            ++pNextRect;

            x1 = r2->right() + 1;
            if (x1 > r1->right()) {
                /*
                 * Minuend used up: advance to new...
                 */
                ++r1;
                if (r1 != r1End)
                    x1 = r1->left();
            } else {
                // Subtrahend used up
                ++r2;
            }
        } else {
            /*
             * Minuend used up: add any remaining piece before advancing.
             */
            if (r1->right() >= x1) {
                MEMCHECK(dest, pNextRect, dest.rects)
                pNextRect->setCoords(x1, y1, r1->right(), y2);
                ++dest.numRects;
                ++pNextRect;
            }
            ++r1;
            if (r1 != r1End)
                x1 = r1->left();
        }
    }

    /*
     * Add remaining minuend rectangles to region.
     */
    while (r1 != r1End) {
        Q_ASSERT(x1 <= r1->right());
        MEMCHECK(dest, pNextRect, dest.rects)
        pNextRect->setCoords(x1, y1, r1->right(), y2);
        ++dest.numRects;
        ++pNextRect;

        ++r1;
        if (r1 != r1End)
            x1 = r1->left();
    }
}

/*-
 *-----------------------------------------------------------------------
 * miSubtract --
 *      Subtract regS from regM and leave the result in regD.
 *      S stands for subtrahend, M for minuend and D for difference.
 *
 * Side Effects:
 *      regD is overwritten.
 *
 *-----------------------------------------------------------------------
 */

static void SubtractRegion(QRegionPrivate *regM, QRegionPrivate *regS,
                           QRegionPrivate &dest)
{
    Q_ASSERT(!isEmptyHelper(regM));
    Q_ASSERT(!isEmptyHelper(regS));
    Q_ASSERT(EXTENTCHECK(&regM->extents, &regS->extents));
    Q_ASSERT(!regS->contains(*regM));
    Q_ASSERT(!EqualRegion(regM, regS));

    miRegionOp(dest, regM, regS, miSubtractO, miSubtractNonO1, 0);

    /*
     * Can't alter dest's extents before we call miRegionOp because
     * it might be one of the source regions and miRegionOp depends
     * on the extents of those regions being the unaltered. Besides, this
     * way there's no checking against rectangles that will be nuked
     * due to coalescing, so we have to examine fewer rectangles.
     */
    miSetExtents(dest);
}

static void XorRegion(QRegionPrivate *sra, QRegionPrivate *srb, QRegionPrivate &dest)
{
    Q_ASSERT(!isEmptyHelper(sra) && !isEmptyHelper(srb));
    Q_ASSERT(EXTENTCHECK(&sra->extents, &srb->extents));
    Q_ASSERT(!EqualRegion(sra, srb));

    QRegionPrivate tra, trb;

    if (!srb->contains(*sra))
        SubtractRegion(sra, srb, tra);
    if (!sra->contains(*srb))
        SubtractRegion(srb, sra, trb);

    Q_ASSERT(isEmptyHelper(&trb) || !tra.contains(trb));
    Q_ASSERT(isEmptyHelper(&tra) || !trb.contains(tra));

    if (isEmptyHelper(&tra)) {
        dest = trb;
    } else if (isEmptyHelper(&trb)) {
        dest = tra;
    } else if (tra.canAppend(&trb)) {
        dest = tra;
        dest.append(&trb);
    } else if (trb.canAppend(&tra)) {
        dest = trb;
        dest.append(&tra);
    } else {
        UnionRegion(&tra, &trb, dest);
    }
}

/*
 *      Check to see if two regions are equal
 */
static bool EqualRegion(const QRegionPrivate *r1, const QRegionPrivate *r2)
{
    if (r1->numRects != r2->numRects) {
        return false;
    } else if (r1->numRects == 0) {
        return true;
    } else if (r1->extents != r2->extents) {
        return false;
    } else if (r1->numRects == 1 && r2->numRects == 1) {
        return true; // equality tested in previous if-statement
    } else {
        const QRect *rr1 = (r1->numRects == 1) ? &r1->extents : r1->rects.constData();
        const QRect *rr2 = (r2->numRects == 1) ? &r2->extents : r2->rects.constData();
        for (int i = 0; i < r1->numRects; ++i, ++rr1, ++rr2) {
            if (*rr1 != *rr2)
                return false;
        }
    }

    return true;
}

static bool PointInRegion(QRegionPrivate *pRegion, int x, int y)
{
    int i;

    if (isEmptyHelper(pRegion))
        return false;
    if (!pRegion->extents.contains(x, y))
        return false;
    if (pRegion->numRects == 1)
        return pRegion->extents.contains(x, y);
    if (pRegion->innerRect.contains(x, y))
        return true;
    for (i = 0; i < pRegion->numRects; ++i) {
        if (pRegion->rects[i].contains(x, y))
            return true;
    }
    return false;
}

static bool RectInRegion(QRegionPrivate *region, int rx, int ry, uint rwidth, uint rheight)
{
    const QRect *pbox;
    const QRect *pboxEnd;
    QRect rect(rx, ry, rwidth, rheight);
    QRect *prect = &rect;
    int partIn, partOut;

    if (!region || region->numRects == 0 || !EXTENTCHECK(&region->extents, prect))
        return RectangleOut;

    partOut = false;
    partIn = false;

    /* can stop when both partOut and partIn are true, or we reach prect->y2 */
    pbox = (region->numRects == 1) ? &region->extents : region->rects.constData();
    pboxEnd = pbox + region->numRects;
    for (; pbox < pboxEnd; ++pbox) {
        if (pbox->bottom() < ry)
           continue;

        if (pbox->top() > ry) {
           partOut = true;
           if (partIn || pbox->top() > prect->bottom())
              break;
           ry = pbox->top();
        }

        if (pbox->right() < rx)
           continue;            /* not far enough over yet */

        if (pbox->left() > rx) {
           partOut = true;      /* missed part of rectangle to left */
           if (partIn)
              break;
        }

        if (pbox->left() <= prect->right()) {
            partIn = true;      /* definitely overlap */
            if (partOut)
               break;
        }

        if (pbox->right() >= prect->right()) {
           ry = pbox->bottom() + 1;     /* finished with this band */
           if (ry > prect->bottom())
              break;
           rx = prect->left();  /* reset x out to left again */
        } else {
            /*
             * Because boxes in a band are maximal width, if the first box
             * to overlap the rectangle doesn't completely cover it in that
             * band, the rectangle must be partially out, since some of it
             * will be uncovered in that band. partIn will have been set true
             * by now...
             */
            break;
        }
    }
    return partIn ? ((ry <= prect->bottom()) ? RectanglePart : RectangleIn) : RectangleOut;
}
// END OF Region.c extract
// START OF poly.h extract
/* $XConsortium: poly.h,v 1.4 94/04/17 20:22:19 rws Exp $ */
/************************************************************************

Copyright (c) 1987  X Consortium

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
X CONSORTIUM BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of the X Consortium shall not be
used in advertising or otherwise to promote the sale, use or other dealings
in this Software without prior written authorization from the X Consortium.


Copyright 1987 by Digital Equipment Corporation, Maynard, Massachusetts.

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its
documentation for any purpose and without fee is hereby granted,
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in
supporting documentation, and that the name of Digital not be
used in advertising or publicity pertaining to distribution of the
software without specific, written prior permission.

DIGITAL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
DIGITAL BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
SOFTWARE.

************************************************************************/

/*
 *     This file contains a few macros to help track
 *     the edge of a filled object.  The object is assumed
 *     to be filled in scanline order, and thus the
 *     algorithm used is an extension of Bresenham's line
 *     drawing algorithm which assumes that y is always the
 *     major axis.
 *     Since these pieces of code are the same for any filled shape,
 *     it is more convenient to gather the library in one
 *     place, but since these pieces of code are also in
 *     the inner loops of output primitives, procedure call
 *     overhead is out of the question.
 *     See the author for a derivation if needed.
 */


/*
 *  In scan converting polygons, we want to choose those pixels
 *  which are inside the polygon.  Thus, we add .5 to the starting
 *  x coordinate for both left and right edges.  Now we choose the
 *  first pixel which is inside the pgon for the left edge and the
 *  first pixel which is outside the pgon for the right edge.
 *  Draw the left pixel, but not the right.
 *
 *  How to add .5 to the starting x coordinate:
 *      If the edge is moving to the right, then subtract dy from the
 *  error term from the general form of the algorithm.
 *      If the edge is moving to the left, then add dy to the error term.
 *
 *  The reason for the difference between edges moving to the left
 *  and edges moving to the right is simple:  If an edge is moving
 *  to the right, then we want the algorithm to flip immediately.
 *  If it is moving to the left, then we don't want it to flip until
 *  we traverse an entire pixel.
 */
#define BRESINITPGON(dy, x1, x2, xStart, d, m, m1, incr1, incr2) { \
    int dx;      /* local storage */ \
\
    /* \
     *  if the edge is horizontal, then it is ignored \
     *  and assumed not to be processed.  Otherwise, do this stuff. \
     */ \
    if ((dy) != 0) { \
        xStart = (x1); \
        dx = (x2) - xStart; \
        if (dx < 0) { \
            m = dx / (dy); \
            m1 = m - 1; \
            incr1 = -2 * dx + 2 * (dy) * m1; \
            incr2 = -2 * dx + 2 * (dy) * m; \
            d = 2 * m * (dy) - 2 * dx - 2 * (dy); \
        } else { \
            m = dx / (dy); \
            m1 = m + 1; \
            incr1 = 2 * dx - 2 * (dy) * m1; \
            incr2 = 2 * dx - 2 * (dy) * m; \
            d = -2 * m * (dy) + 2 * dx; \
        } \
    } \
}

#define BRESINCRPGON(d, minval, m, m1, incr1, incr2) { \
    if (m1 > 0) { \
        if (d > 0) { \
            minval += m1; \
            d += incr1; \
        } \
        else { \
            minval += m; \
            d += incr2; \
        } \
    } else {\
        if (d >= 0) { \
            minval += m1; \
            d += incr1; \
        } \
        else { \
            minval += m; \
            d += incr2; \
        } \
    } \
}


/*
 *     This structure contains all of the information needed
 *     to run the bresenham algorithm.
 *     The variables may be hardcoded into the declarations
 *     instead of using this structure to make use of
 *     register declarations.
 */
typedef struct {
    int minor_axis;     /* minor axis        */
    int d;              /* decision variable */
    int m, m1;          /* slope and slope+1 */
    int incr1, incr2;   /* error increments */
} BRESINFO;


#define BRESINITPGONSTRUCT(dmaj, min1, min2, bres) \
        BRESINITPGON(dmaj, min1, min2, bres.minor_axis, bres.d, \
                     bres.m, bres.m1, bres.incr1, bres.incr2)

#define BRESINCRPGONSTRUCT(bres) \
        BRESINCRPGON(bres.d, bres.minor_axis, bres.m, bres.m1, bres.incr1, bres.incr2)



/*
 *     These are the data structures needed to scan
 *     convert regions.  Two different scan conversion
 *     methods are available -- the even-odd method, and
 *     the winding number method.
 *     The even-odd rule states that a point is inside
 *     the polygon if a ray drawn from that point in any
 *     direction will pass through an odd number of
 *     path segments.
 *     By the winding number rule, a point is decided
 *     to be inside the polygon if a ray drawn from that
 *     point in any direction passes through a different
 *     number of clockwise and counter-clockwise path
 *     segments.
 *
 *     These data structures are adapted somewhat from
 *     the algorithm in (Foley/Van Dam) for scan converting
 *     polygons.
 *     The basic algorithm is to start at the top (smallest y)
 *     of the polygon, stepping down to the bottom of
 *     the polygon by incrementing the y coordinate.  We
 *     keep a list of edges which the current scanline crosses,
 *     sorted by x.  This list is called the Active Edge Table (AET)
 *     As we change the y-coordinate, we update each entry in
 *     in the active edge table to reflect the edges new xcoord.
 *     This list must be sorted at each scanline in case
 *     two edges intersect.
 *     We also keep a data structure known as the Edge Table (ET),
 *     which keeps track of all the edges which the current
 *     scanline has not yet reached.  The ET is basically a
 *     list of ScanLineList structures containing a list of
 *     edges which are entered at a given scanline.  There is one
 *     ScanLineList per scanline at which an edge is entered.
 *     When we enter a new edge, we move it from the ET to the AET.
 *
 *     From the AET, we can implement the even-odd rule as in
 *     (Foley/Van Dam).
 *     The winding number rule is a little trickier.  We also
 *     keep the EdgeTableEntries in the AET linked by the
 *     nextWETE (winding EdgeTableEntry) link.  This allows
 *     the edges to be linked just as before for updating
 *     purposes, but only uses the edges linked by the nextWETE
 *     link as edges representing spans of the polygon to
 *     drawn (as with the even-odd rule).
 */

/*
 * for the winding number rule
 */
#define CLOCKWISE          1
#define COUNTERCLOCKWISE  -1

typedef struct _EdgeTableEntry {
     int ymax;             /* ycoord at which we exit this edge. */
     BRESINFO bres;        /* Bresenham info to run the edge     */
     struct _EdgeTableEntry *next;       /* next in the list     */
     struct _EdgeTableEntry *back;       /* for insertion sort   */
     struct _EdgeTableEntry *nextWETE;   /* for winding num rule */
     int ClockWise;        /* flag for winding number rule       */
} EdgeTableEntry;


typedef struct _ScanLineList{
     int scanline;              /* the scanline represented */
     EdgeTableEntry *edgelist;  /* header node              */
     struct _ScanLineList *next;  /* next in the list       */
} ScanLineList;


typedef struct {
     int ymax;                 /* ymax for the polygon     */
     int ymin;                 /* ymin for the polygon     */
     ScanLineList scanlines;   /* header node              */
} EdgeTable;


/*
 * Here is a struct to help with storage allocation
 * so we can allocate a big chunk at a time, and then take
 * pieces from this heap when we need to.
 */
#define SLLSPERBLOCK 25

typedef struct _ScanLineListBlock {
     ScanLineList SLLs[SLLSPERBLOCK];
     struct _ScanLineListBlock *next;
} ScanLineListBlock;



/*
 *
 *     a few macros for the inner loops of the fill code where
 *     performance considerations don't allow a procedure call.
 *
 *     Evaluate the given edge at the given scanline.
 *     If the edge has expired, then we leave it and fix up
 *     the active edge table; otherwise, we increment the
 *     x value to be ready for the next scanline.
 *     The winding number rule is in effect, so we must notify
 *     the caller when the edge has been removed so he
 *     can reorder the Winding Active Edge Table.
 */
#define EVALUATEEDGEWINDING(pAET, pPrevAET, y, fixWAET) { \
   if (pAET->ymax == y) {          /* leaving this edge */ \
      pPrevAET->next = pAET->next; \
      pAET = pPrevAET->next; \
      fixWAET = 1; \
      if (pAET) \
         pAET->back = pPrevAET; \
   } \
   else { \
      BRESINCRPGONSTRUCT(pAET->bres) \
      pPrevAET = pAET; \
      pAET = pAET->next; \
   } \
}


/*
 *     Evaluate the given edge at the given scanline.
 *     If the edge has expired, then we leave it and fix up
 *     the active edge table; otherwise, we increment the
 *     x value to be ready for the next scanline.
 *     The even-odd rule is in effect.
 */
#define EVALUATEEDGEEVENODD(pAET, pPrevAET, y) { \
   if (pAET->ymax == y) {          /* leaving this edge */ \
      pPrevAET->next = pAET->next; \
      pAET = pPrevAET->next; \
      if (pAET) \
         pAET->back = pPrevAET; \
   } \
   else { \
      BRESINCRPGONSTRUCT(pAET->bres) \
      pPrevAET = pAET; \
      pAET = pAET->next; \
   } \
}
// END OF poly.h extract
// START OF PolyReg.c extract
/* $XConsortium: PolyReg.c,v 11.23 94/11/17 21:59:37 converse Exp $ */
/************************************************************************

Copyright (c) 1987  X Consortium

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
X CONSORTIUM BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of the X Consortium shall not be
used in advertising or otherwise to promote the sale, use or other dealings
in this Software without prior written authorization from the X Consortium.


Copyright 1987 by Digital Equipment Corporation, Maynard, Massachusetts.

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its
documentation for any purpose and without fee is hereby granted,
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in
supporting documentation, and that the name of Digital not be
used in advertising or publicity pertaining to distribution of the
software without specific, written prior permission.

DIGITAL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
DIGITAL BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
SOFTWARE.

************************************************************************/
/* $XFree86: xc/lib/X11/PolyReg.c,v 1.1.1.2.8.2 1998/10/04 15:22:49 hohndel Exp $ */

#define LARGE_COORDINATE INT_MAX
#define SMALL_COORDINATE INT_MIN

/*
 *     InsertEdgeInET
 *
 *     Insert the given edge into the edge table.
 *     First we must find the correct bucket in the
 *     Edge table, then find the right slot in the
 *     bucket.  Finally, we can insert it.
 *
 */
static void InsertEdgeInET(EdgeTable *ET, EdgeTableEntry *ETE, int scanline,
                           ScanLineListBlock **SLLBlock, int *iSLLBlock)
{
    EdgeTableEntry *start, *prev;
    ScanLineList *pSLL, *pPrevSLL;
    ScanLineListBlock *tmpSLLBlock;

    /*
     * find the right bucket to put the edge into
     */
    pPrevSLL = &ET->scanlines;
    pSLL = pPrevSLL->next;
    while (pSLL && (pSLL->scanline < scanline)) {
        pPrevSLL = pSLL;
        pSLL = pSLL->next;
    }

    /*
     * reassign pSLL (pointer to ScanLineList) if necessary
     */
    if ((!pSLL) || (pSLL->scanline > scanline)) {
        if (*iSLLBlock > SLLSPERBLOCK-1)
        {
            tmpSLLBlock =
                  (ScanLineListBlock *)malloc(sizeof(ScanLineListBlock));
            Q_CHECK_PTR(tmpSLLBlock);
            (*SLLBlock)->next = tmpSLLBlock;
            tmpSLLBlock->next = (ScanLineListBlock *)NULL;
            *SLLBlock = tmpSLLBlock;
            *iSLLBlock = 0;
        }
        pSLL = &((*SLLBlock)->SLLs[(*iSLLBlock)++]);

        pSLL->next = pPrevSLL->next;
        pSLL->edgelist = (EdgeTableEntry *)NULL;
        pPrevSLL->next = pSLL;
    }
    pSLL->scanline = scanline;

    /*
     * now insert the edge in the right bucket
     */
    prev = 0;
    start = pSLL->edgelist;
    while (start && (start->bres.minor_axis < ETE->bres.minor_axis)) {
        prev = start;
        start = start->next;
    }
    ETE->next = start;

    if (prev)
        prev->next = ETE;
    else
        pSLL->edgelist = ETE;
}

/*
 *     CreateEdgeTable
 *
 *     This routine creates the edge table for
 *     scan converting polygons.
 *     The Edge Table (ET) looks like:
 *
 *    EdgeTable
 *     --------
 *    |  ymax  |        ScanLineLists
 *    |scanline|-->------------>-------------->...
 *     --------   |scanline|   |scanline|
 *                |edgelist|   |edgelist|
 *                ---------    ---------
 *                    |             |
 *                    |             |
 *                    V             V
 *              list of ETEs   list of ETEs
 *
 *     where ETE is an EdgeTableEntry data structure,
 *     and there is one ScanLineList per scanline at
 *     which an edge is initially entered.
 *
 */

static void CreateETandAET(int count, const QPoint *pts,
                           EdgeTable *ET, EdgeTableEntry *AET, EdgeTableEntry *pETEs,
                           ScanLineListBlock *pSLLBlock)
{
    const QPoint *top,
                          *bottom,
                          *PrevPt,
                          *CurrPt;
    int iSLLBlock = 0;
    int dy;

    if (count < 2)
        return;

    /*
     *  initialize the Active Edge Table
     */
    AET->next = 0;
    AET->back = 0;
    AET->nextWETE = 0;
    AET->bres.minor_axis = SMALL_COORDINATE;

    /*
     *  initialize the Edge Table.
     */
    ET->scanlines.next = 0;
    ET->ymax = SMALL_COORDINATE;
    ET->ymin = LARGE_COORDINATE;
    pSLLBlock->next = 0;

    PrevPt = &pts[count - 1];

    /*
     *  for each vertex in the array of points.
     *  In this loop we are dealing with two vertices at
     *  a time -- these make up one edge of the polygon.
     */
    while (count--) {
        CurrPt = pts++;

        /*
         *  find out which point is above and which is below.
         */
        if (PrevPt->y() > CurrPt->y()) {
            bottom = PrevPt;
            top = CurrPt;
            pETEs->ClockWise = 0;
        } else {
            bottom = CurrPt;
            top = PrevPt;
            pETEs->ClockWise = 1;
        }

        /*
         * don't add horizontal edges to the Edge table.
         */
        if (bottom->y() != top->y()) {
            pETEs->ymax = bottom->y() - 1;  /* -1 so we don't get last scanline */

            /*
             *  initialize integer edge algorithm
             */
            dy = bottom->y() - top->y();
            BRESINITPGONSTRUCT(dy, top->x(), bottom->x(), pETEs->bres)

            InsertEdgeInET(ET, pETEs, top->y(), &pSLLBlock, &iSLLBlock);

            if (PrevPt->y() > ET->ymax)
                ET->ymax = PrevPt->y();
            if (PrevPt->y() < ET->ymin)
                ET->ymin = PrevPt->y();
            ++pETEs;
        }

        PrevPt = CurrPt;
    }
}

/*
 *     loadAET
 *
 *     This routine moves EdgeTableEntries from the
 *     EdgeTable into the Active Edge Table,
 *     leaving them sorted by smaller x coordinate.
 *
 */

static void loadAET(EdgeTableEntry *AET, EdgeTableEntry *ETEs)
{
    EdgeTableEntry *pPrevAET;
    EdgeTableEntry *tmp;

    pPrevAET = AET;
    AET = AET->next;
    while (ETEs) {
        while (AET && AET->bres.minor_axis < ETEs->bres.minor_axis) {
            pPrevAET = AET;
            AET = AET->next;
        }
        tmp = ETEs->next;
        ETEs->next = AET;
        if (AET)
            AET->back = ETEs;
        ETEs->back = pPrevAET;
        pPrevAET->next = ETEs;
        pPrevAET = ETEs;

        ETEs = tmp;
    }
}

/*
 *     computeWAET
 *
 *     This routine links the AET by the
 *     nextWETE (winding EdgeTableEntry) link for
 *     use by the winding number rule.  The final
 *     Active Edge Table (AET) might look something
 *     like:
 *
 *     AET
 *     ----------  ---------   ---------
 *     |ymax    |  |ymax    |  |ymax    |
 *     | ...    |  |...     |  |...     |
 *     |next    |->|next    |->|next    |->...
 *     |nextWETE|  |nextWETE|  |nextWETE|
 *     ---------   ---------   ^--------
 *         |                   |       |
 *         V------------------->       V---> ...
 *
 */
static void computeWAET(EdgeTableEntry *AET)
{
    EdgeTableEntry *pWETE;
    int inside = 1;
    int isInside = 0;

    AET->nextWETE = 0;
    pWETE = AET;
    AET = AET->next;
    while (AET) {
        if (AET->ClockWise)
            ++isInside;
        else
            --isInside;

        if ((!inside && !isInside) || (inside && isInside)) {
            pWETE->nextWETE = AET;
            pWETE = AET;
            inside = !inside;
        }
        AET = AET->next;
    }
    pWETE->nextWETE = 0;
}

/*
 *     InsertionSort
 *
 *     Just a simple insertion sort using
 *     pointers and back pointers to sort the Active
 *     Edge Table.
 *
 */

static int InsertionSort(EdgeTableEntry *AET)
{
    EdgeTableEntry *pETEchase;
    EdgeTableEntry *pETEinsert;
    EdgeTableEntry *pETEchaseBackTMP;
    int changed = 0;

    AET = AET->next;
    while (AET) {
        pETEinsert = AET;
        pETEchase = AET;
        while (pETEchase->back->bres.minor_axis > AET->bres.minor_axis)
            pETEchase = pETEchase->back;

        AET = AET->next;
        if (pETEchase != pETEinsert) {
            pETEchaseBackTMP = pETEchase->back;
            pETEinsert->back->next = AET;
            if (AET)
                AET->back = pETEinsert->back;
            pETEinsert->next = pETEchase;
            pETEchase->back->next = pETEinsert;
            pETEchase->back = pETEinsert;
            pETEinsert->back = pETEchaseBackTMP;
            changed = 1;
        }
    }
    return changed;
}

/*
 *     Clean up our act.
 */
static void FreeStorage(ScanLineListBlock *pSLLBlock)
{
    ScanLineListBlock *tmpSLLBlock;

    while (pSLLBlock) {
        tmpSLLBlock = pSLLBlock->next;
        free(pSLLBlock);
        pSLLBlock = tmpSLLBlock;
    }
}

struct QRegionSpan {
    QRegionSpan() {}
    QRegionSpan(int x1_, int x2_) : x1(x1_), x2(x2_) {}

    int x1;
    int x2;
    int width() const { return x2 - x1; }
};

Q_DECLARE_TYPEINFO(QRegionSpan, Q_PRIMITIVE_TYPE);

static inline void flushRow(const QRegionSpan *spans, int y, int numSpans, QRegionPrivate *reg, int *lastRow, int *extendTo, bool *needsExtend)
{
    QRect *regRects = reg->rects.data() + *lastRow;
    bool canExtend = reg->rects.size() - *lastRow == numSpans
        && !(*needsExtend && *extendTo + 1 != y)
        && (*needsExtend || regRects[0].y() + regRects[0].height() == y);

    for (int i = 0; i < numSpans && canExtend; ++i) {
        if (regRects[i].x() != spans[i].x1 || regRects[i].right() != spans[i].x2 - 1)
            canExtend = false;
    }

    if (canExtend) {
        *extendTo = y;
        *needsExtend = true;
    } else {
        if (*needsExtend) {
            for (int i = 0; i < reg->rects.size() - *lastRow; ++i)
                regRects[i].setBottom(*extendTo);
        }

        *lastRow = reg->rects.size();
        reg->rects.reserve(*lastRow + numSpans);
        for (int i = 0; i < numSpans; ++i)
            reg->rects << QRect(spans[i].x1, y, spans[i].width(), 1);

        if (spans[0].x1 < reg->extents.left())
            reg->extents.setLeft(spans[0].x1);

        if (spans[numSpans-1].x2 - 1 > reg->extents.right())
            reg->extents.setRight(spans[numSpans-1].x2 - 1);

        *needsExtend = false;
    }
}

/*
 *     Create an array of rectangles from a list of points.
 *     If indeed these things (POINTS, RECTS) are the same,
 *     then this proc is still needed, because it allocates
 *     storage for the array, which was allocated on the
 *     stack by the calling procedure.
 *
 */
static void PtsToRegion(int numFullPtBlocks, int iCurPtBlock,
                       POINTBLOCK *FirstPtBlock, QRegionPrivate *reg)
{
    int lastRow = 0;
    int extendTo = 0;
    bool needsExtend = false;
    QVarLengthArray<QRegionSpan> row;
    int rowSize = 0;

    reg->extents.setLeft(INT_MAX);
    reg->extents.setRight(INT_MIN);
    reg->innerArea = -1;

    POINTBLOCK *CurPtBlock = FirstPtBlock;
    for (; numFullPtBlocks >= 0; --numFullPtBlocks) {
        /* the loop uses 2 points per iteration */
        int i = NUMPTSTOBUFFER >> 1;
        if (!numFullPtBlocks)
            i = iCurPtBlock >> 1;
        if(i) {
            row.resize(qMax(row.size(), rowSize + i));
            for (QPoint *pts = CurPtBlock->pts; i--; pts += 2) {
                const int width = pts[1].x() - pts[0].x();
                if (width) {
                    if (rowSize && row[rowSize-1].x2 == pts[0].x())
                        row[rowSize-1].x2 = pts[1].x();
                    else
                        row[rowSize++] = QRegionSpan(pts[0].x(), pts[1].x());
                }

                if (rowSize) {
                    QPoint *next = i ? &pts[2] : (numFullPtBlocks ? CurPtBlock->next->pts : 0);

                    if (!next || next->y() != pts[0].y()) {
                        flushRow(row.data(), pts[0].y(), rowSize, reg, &lastRow, &extendTo, &needsExtend);
                        rowSize = 0;
                    }
                }
            }
        }
        CurPtBlock = CurPtBlock->next;
    }

    if (needsExtend) {
        for (int i = lastRow; i < reg->rects.size(); ++i)
            reg->rects[i].setBottom(extendTo);
    }

    reg->numRects = reg->rects.size();

    if (reg->numRects) {
        reg->extents.setTop(reg->rects[0].top());
        reg->extents.setBottom(reg->rects[lastRow].bottom());

        for (int i = 0; i < reg->rects.size(); ++i)
            reg->updateInnerRect(reg->rects[i]);
    } else {
        reg->extents.setCoords(0, 0, 0, 0);
    }
}

/*
 *     polytoregion
 *
 *     Scan converts a polygon by returning a run-length
 *     encoding of the resultant bitmap -- the run-length
 *     encoding is in the form of an array of rectangles.
 *
 *     Can return 0 in case of errors.
 */
static QRegionPrivate *PolygonRegion(const QPoint *Pts, int Count, int rule)
    //Point     *Pts;                /* the pts                 */
    //int       Count;                 /* number of pts           */
    //int       rule;                        /* winding rule */
{
    QRegionPrivate *region;
    EdgeTableEntry *pAET;   /* Active Edge Table       */
    int y;                  /* current scanline        */
    int iPts = 0;           /* number of pts in buffer */
    EdgeTableEntry *pWETE;  /* Winding Edge Table Entry*/
    ScanLineList *pSLL;     /* current scanLineList    */
    QPoint *pts;             /* output buffer           */
    EdgeTableEntry *pPrevAET;        /* ptr to previous AET     */
    EdgeTable ET;                    /* header node for ET      */
    EdgeTableEntry *AET;             /* header node for AET     */
    EdgeTableEntry *pETEs;           /* EdgeTableEntries pool   */
    ScanLineListBlock SLLBlock;      /* header for scanlinelist */
    int fixWAET = false;
    POINTBLOCK FirstPtBlock, *curPtBlock; /* PtBlock buffers    */
    FirstPtBlock.pts = reinterpret_cast<QPoint *>(FirstPtBlock.data);
    POINTBLOCK *tmpPtBlock;
    int numFullPtBlocks = 0;

    if (!(region = new QRegionPrivate))
        return 0;

        /* special case a rectangle */
    if (((Count == 4) ||
         ((Count == 5) && (Pts[4].x() == Pts[0].x()) && (Pts[4].y() == Pts[0].y())))
         && (((Pts[0].y() == Pts[1].y()) && (Pts[1].x() == Pts[2].x()) && (Pts[2].y() == Pts[3].y())
               && (Pts[3].x() == Pts[0].x())) || ((Pts[0].x() == Pts[1].x())
               && (Pts[1].y() == Pts[2].y()) && (Pts[2].x() == Pts[3].x())
               && (Pts[3].y() == Pts[0].y())))) {
        int x = qMin(Pts[0].x(), Pts[2].x());
        region->extents.setLeft(x);
        int y = qMin(Pts[0].y(), Pts[2].y());
        region->extents.setTop(y);
        region->extents.setWidth(qMax(Pts[0].x(), Pts[2].x()) - x);
        region->extents.setHeight(qMax(Pts[0].y(), Pts[2].y()) - y);
        if ((region->extents.left() <= region->extents.right()) &&
            (region->extents.top() <= region->extents.bottom())) {
            region->numRects = 1;
            region->innerRect = region->extents;
            region->innerArea = region->innerRect.width() * region->innerRect.height();
        }
        return region;
    }

    if (!(pETEs = static_cast<EdgeTableEntry *>(malloc(sizeof(EdgeTableEntry) * Count))))
        return 0;

    region->vectorize();

    AET = new EdgeTableEntry;
    pts = FirstPtBlock.pts;
    CreateETandAET(Count, Pts, &ET, AET, pETEs, &SLLBlock);

    pSLL = ET.scanlines.next;
    curPtBlock = &FirstPtBlock;

    // sanity check that the region won't become too big...
    if (ET.ymax - ET.ymin > 100000) {
        // clean up region ptr
#ifndef QT_NO_DEBUG
        qWarning("QRegion: creating region from big polygon failed...!");
#endif
        delete AET;
        delete region;
        return 0;
    }


    QT_TRY {
        if (rule == EvenOddRule) {
            /*
             *  for each scanline
             */
            for (y = ET.ymin; y < ET.ymax; ++y) {

                /*
                 *  Add a new edge to the active edge table when we
                 *  get to the next edge.
                 */
                if (pSLL && y == pSLL->scanline) {
                    loadAET(AET, pSLL->edgelist);
                    pSLL = pSLL->next;
                }
                pPrevAET = AET;
                pAET = AET->next;

                /*
                 *  for each active edge
                 */
                while (pAET) {
                    pts->setX(pAET->bres.minor_axis);
                    pts->setY(y);
                    ++pts;
                    ++iPts;

                    /*
                     *  send out the buffer
                     */
                    if (iPts == NUMPTSTOBUFFER) {
                        tmpPtBlock = (POINTBLOCK *)malloc(sizeof(POINTBLOCK));
                        Q_CHECK_PTR(tmpPtBlock);
                        tmpPtBlock->pts = reinterpret_cast<QPoint *>(tmpPtBlock->data);
                        curPtBlock->next = tmpPtBlock;
                        curPtBlock = tmpPtBlock;
                        pts = curPtBlock->pts;
                        ++numFullPtBlocks;
                        iPts = 0;
                    }
                    EVALUATEEDGEEVENODD(pAET, pPrevAET, y)
                }
                InsertionSort(AET);
            }
        } else {
            /*
             *  for each scanline
             */
            for (y = ET.ymin; y < ET.ymax; ++y) {
                /*
                 *  Add a new edge to the active edge table when we
                 *  get to the next edge.
                 */
                if (pSLL && y == pSLL->scanline) {
                    loadAET(AET, pSLL->edgelist);
                    computeWAET(AET);
                    pSLL = pSLL->next;
                }
                pPrevAET = AET;
                pAET = AET->next;
                pWETE = pAET;

                /*
                 *  for each active edge
                 */
                while (pAET) {
                    /*
                     *  add to the buffer only those edges that
                     *  are in the Winding active edge table.
                     */
                    if (pWETE == pAET) {
                        pts->setX(pAET->bres.minor_axis);
                        pts->setY(y);
                        ++pts;
                        ++iPts;

                        /*
                         *  send out the buffer
                         */
                        if (iPts == NUMPTSTOBUFFER) {
                            tmpPtBlock = static_cast<POINTBLOCK *>(malloc(sizeof(POINTBLOCK)));
                            tmpPtBlock->pts = reinterpret_cast<QPoint *>(tmpPtBlock->data);
                            curPtBlock->next = tmpPtBlock;
                            curPtBlock = tmpPtBlock;
                            pts = curPtBlock->pts;
                            ++numFullPtBlocks;
                            iPts = 0;
                        }
                        pWETE = pWETE->nextWETE;
                    }
                    EVALUATEEDGEWINDING(pAET, pPrevAET, y, fixWAET)
                }

                /*
                 *  recompute the winding active edge table if
                 *  we just resorted or have exited an edge.
                 */
                if (InsertionSort(AET) || fixWAET) {
                    computeWAET(AET);
                    fixWAET = false;
                }
            }
        }
    } QT_CATCH(...) {
        FreeStorage(SLLBlock.next);
        PtsToRegion(numFullPtBlocks, iPts, &FirstPtBlock, region);
        for (curPtBlock = FirstPtBlock.next; --numFullPtBlocks >= 0;) {
            tmpPtBlock = curPtBlock->next;
            free(curPtBlock);
            curPtBlock = tmpPtBlock;
        }
        free(pETEs);
        return 0; // this function returns 0 in case of an error
    }

    FreeStorage(SLLBlock.next);
    PtsToRegion(numFullPtBlocks, iPts, &FirstPtBlock, region);
    for (curPtBlock = FirstPtBlock.next; --numFullPtBlocks >= 0;) {
        tmpPtBlock = curPtBlock->next;
        free(curPtBlock);
        curPtBlock = tmpPtBlock;
    }
    delete AET;
    free(pETEs);
    return region;
}
// END OF PolyReg.c extract

QRegionPrivate *qt_bitmapToRegion(const QBitmap& bitmap)
{
    QImage image = bitmap.toImage();

    QRegionPrivate *region = new QRegionPrivate;

    QRect xr;

#define AddSpan \
        { \
            xr.setCoords(prev1, y, x-1, y); \
            UnionRectWithRegion(&xr, region, *region); \
        }

    const uchar zero = 0;
    bool little = image.format() == QImage::Format_MonoLSB;

    int x,
        y;
    for (y = 0; y < image.height(); ++y) {
        uchar *line = image.scanLine(y);
        int w = image.width();
        uchar all = zero;
        int prev1 = -1;
        for (x = 0; x < w;) {
            uchar byte = line[x / 8];
            if (x > w - 8 || byte!=all) {
                if (little) {
                    for (int b = 8; b > 0 && x < w; --b) {
                        if (!(byte & 0x01) == !all) {
                            // More of the same
                        } else {
                            // A change.
                            if (all!=zero) {
                                AddSpan
                                all = zero;
                            } else {
                                prev1 = x;
                                all = ~zero;
                            }
                        }
                        byte >>= 1;
                        ++x;
                    }
                } else {
                    for (int b = 8; b > 0 && x < w; --b) {
                        if (!(byte & 0x80) == !all) {
                            // More of the same
                        } else {
                            // A change.
                            if (all != zero) {
                                AddSpan
                                all = zero;
                            } else {
                                prev1 = x;
                                all = ~zero;
                            }
                        }
                        byte <<= 1;
                        ++x;
                    }
                }
            } else {
                x += 8;
            }
        }
        if (all != zero) {
            AddSpan
        }
    }
#undef AddSpan

    return region;
}

QRegion::QRegion()
    : d(&shared_empty)
{
    d->ref.ref();
}

QRegion::QRegion(const QRect &r, RegionType t)
{
    if (r.isEmpty()) {
        d = &shared_empty;
        d->ref.ref();
    } else {
        d = new QRegionData;
        d->ref.store(1);
        if (t == Rectangle) {
            d->qt_rgn = new QRegionPrivate(r);
        } else if (t == Ellipse) {
            QPainterPath path;
            path.addEllipse(r.x(), r.y(), r.width(), r.height());
            QPolygon a = path.toSubpathPolygons().at(0).toPolygon();
            d->qt_rgn = PolygonRegion(a.constData(), a.size(), EvenOddRule);
        }
    }
}

QRegion::QRegion(const QPolygon &a, Qt::FillRule fillRule)
{
    if (a.count() > 2) {
        QRegionPrivate *qt_rgn = PolygonRegion(a.constData(), a.size(),
                                               fillRule == Qt::WindingFill ? WindingRule : EvenOddRule);
        if (qt_rgn) {
            d =  new QRegionData;
            d->ref.store(1);
            d->qt_rgn = qt_rgn;
        } else {
            d = &shared_empty;
            d->ref.ref();
        }
    } else {
        d = &shared_empty;
        d->ref.ref();
    }
}

QRegion::QRegion(const QRegion &r)
{
    d = r.d;
    d->ref.ref();
}


QRegion::QRegion(const QBitmap &bm)
{
    if (bm.isNull()) {
        d = &shared_empty;
        d->ref.ref();
    } else {
        d = new QRegionData;
        d->ref.store(1);
        d->qt_rgn = qt_bitmapToRegion(bm);
    }
}

void QRegion::cleanUp(QRegion::QRegionData *x)
{
    delete x->qt_rgn;
    delete x;
}

QRegion::~QRegion()
{
    if (!d->ref.deref())
        cleanUp(d);
}


QRegion &QRegion::operator=(const QRegion &r)
{
    r.d->ref.ref();
    if (!d->ref.deref())
        cleanUp(d);
    d = r.d;
    return *this;
}


/*!
    \internal
*/
QRegion QRegion::copy() const
{
    QRegion r;
    QScopedPointer<QRegionData> x(new QRegionData);
    x->ref.store(1);
    if (d->qt_rgn)
        x->qt_rgn = new QRegionPrivate(*d->qt_rgn);
    else
        x->qt_rgn = new QRegionPrivate;
    if (!r.d->ref.deref())
        cleanUp(r.d);
    r.d = x.take();
    return r;
}

bool QRegion::isEmpty() const
{
    return d == &shared_empty || d->qt_rgn->numRects == 0;
}

bool QRegion::isNull() const
{
    return d == &shared_empty || d->qt_rgn->numRects == 0;
}

bool QRegion::contains(const QPoint &p) const
{
    return PointInRegion(d->qt_rgn, p.x(), p.y());
}

bool QRegion::contains(const QRect &r) const
{
    return RectInRegion(d->qt_rgn, r.left(), r.top(), r.width(), r.height()) != RectangleOut;
}



void QRegion::translate(int dx, int dy)
{
    if ((dx == 0 && dy == 0) || isEmptyHelper(d->qt_rgn))
        return;

    detach();
    OffsetRegion(*d->qt_rgn, dx, dy);
}

QRegion QRegion::united(const QRegion &r) const
{
    if (isEmptyHelper(d->qt_rgn))
        return r;
    if (isEmptyHelper(r.d->qt_rgn))
        return *this;
    if (d == r.d)
        return *this;

    if (d->qt_rgn->contains(*r.d->qt_rgn)) {
        return *this;
    } else if (r.d->qt_rgn->contains(*d->qt_rgn)) {
        return r;
    } else if (d->qt_rgn->canAppend(r.d->qt_rgn)) {
        QRegion result(*this);
        result.detach();
        result.d->qt_rgn->append(r.d->qt_rgn);
        return result;
    } else if (d->qt_rgn->canPrepend(r.d->qt_rgn)) {
        QRegion result(*this);
        result.detach();
        result.d->qt_rgn->prepend(r.d->qt_rgn);
        return result;
    } else if (EqualRegion(d->qt_rgn, r.d->qt_rgn)) {
        return *this;
    } else {
        QRegion result;
        result.detach();
        UnionRegion(d->qt_rgn, r.d->qt_rgn, *result.d->qt_rgn);
        return result;
    }
}

QRegion& QRegion::operator+=(const QRegion &r)
{
    if (isEmptyHelper(d->qt_rgn))
        return *this = r;
    if (isEmptyHelper(r.d->qt_rgn))
        return *this;
    if (d == r.d)
        return *this;

    if (d->qt_rgn->contains(*r.d->qt_rgn)) {
        return *this;
    } else if (r.d->qt_rgn->contains(*d->qt_rgn)) {
        return *this = r;
    } else if (d->qt_rgn->canAppend(r.d->qt_rgn)) {
        detach();
        d->qt_rgn->append(r.d->qt_rgn);
        return *this;
    } else if (d->qt_rgn->canPrepend(r.d->qt_rgn)) {
        detach();
        d->qt_rgn->prepend(r.d->qt_rgn);
        return *this;
    } else if (EqualRegion(d->qt_rgn, r.d->qt_rgn)) {
        return *this;
    } else {
        detach();
        UnionRegion(d->qt_rgn, r.d->qt_rgn, *d->qt_rgn);
        return *this;
    }
}

QRegion QRegion::united(const QRect &r) const
{
    if (isEmptyHelper(d->qt_rgn))
        return r;
    if (r.isEmpty())
        return *this;

    if (d->qt_rgn->contains(r)) {
        return *this;
    } else if (d->qt_rgn->within(r)) {
        return r;
    } else if (d->qt_rgn->numRects == 1 && d->qt_rgn->extents == r) {
        return *this;
    } else if (d->qt_rgn->canAppend(&r)) {
        QRegion result(*this);
        result.detach();
        result.d->qt_rgn->append(&r);
        return result;
    } else if (d->qt_rgn->canPrepend(&r)) {
        QRegion result(*this);
        result.detach();
        result.d->qt_rgn->prepend(&r);
        return result;
    } else {
        QRegion result;
        result.detach();
        QRegionPrivate rp(r);
        UnionRegion(d->qt_rgn, &rp, *result.d->qt_rgn);
        return result;
    }
}

QRegion& QRegion::operator+=(const QRect &r)
{
    if (isEmptyHelper(d->qt_rgn))
        return *this = r;
    if (r.isEmpty())
        return *this;

    if (d->qt_rgn->contains(r)) {
        return *this;
    } else if (d->qt_rgn->within(r)) {
        return *this = r;
    } else if (d->qt_rgn->canAppend(&r)) {
        detach();
        d->qt_rgn->append(&r);
        return *this;
    } else if (d->qt_rgn->canPrepend(&r)) {
        detach();
        d->qt_rgn->prepend(&r);
        return *this;
    } else if (d->qt_rgn->numRects == 1 && d->qt_rgn->extents == r) {
        return *this;
    } else {
        detach();
        QRegionPrivate p(r);
        UnionRegion(d->qt_rgn, &p, *d->qt_rgn);
        return *this;
    }
}

QRegion QRegion::intersected(const QRegion &r) const
{
    if (isEmptyHelper(d->qt_rgn) || isEmptyHelper(r.d->qt_rgn)
        || !EXTENTCHECK(&d->qt_rgn->extents, &r.d->qt_rgn->extents))
        return QRegion();

    /* this is fully contained in r */
    if (r.d->qt_rgn->contains(*d->qt_rgn))
        return *this;

    /* r is fully contained in this */
    if (d->qt_rgn->contains(*r.d->qt_rgn))
        return r;

    if (r.d->qt_rgn->numRects == 1 && d->qt_rgn->numRects == 1) {
        const QRect rect = qt_rect_intersect_normalized(r.d->qt_rgn->extents,
                                                        d->qt_rgn->extents);
        return QRegion(rect);
    } else if (r.d->qt_rgn->numRects == 1) {
        QRegion result(*this);
        result.detach();
        result.d->qt_rgn->intersect(r.d->qt_rgn->extents);
        return result;
    } else if (d->qt_rgn->numRects == 1) {
        QRegion result(r);
        result.detach();
        result.d->qt_rgn->intersect(d->qt_rgn->extents);
        return result;
    }

    QRegion result;
    result.detach();
    miRegionOp(*result.d->qt_rgn, d->qt_rgn, r.d->qt_rgn, miIntersectO, 0, 0);

    /*
     * Can't alter dest's extents before we call miRegionOp because
     * it might be one of the source regions and miRegionOp depends
     * on the extents of those regions being the same. Besides, this
     * way there's no checking against rectangles that will be nuked
     * due to coalescing, so we have to examine fewer rectangles.
     */
    miSetExtents(*result.d->qt_rgn);
    return result;
}

QRegion QRegion::intersected(const QRect &r) const
{
    if (isEmptyHelper(d->qt_rgn) || r.isEmpty()
        || !EXTENTCHECK(&d->qt_rgn->extents, &r))
        return QRegion();

    /* this is fully contained in r */
    if (d->qt_rgn->within(r))
        return *this;

    /* r is fully contained in this */
    if (d->qt_rgn->contains(r))
        return r;

    if (d->qt_rgn->numRects == 1) {
        const QRect rect = qt_rect_intersect_normalized(d->qt_rgn->extents,
                                                        r.normalized());
        return QRegion(rect);
    }

    QRegion result(*this);
    result.detach();
    result.d->qt_rgn->intersect(r);
    return result;
}

QRegion QRegion::subtracted(const QRegion &r) const
{
    if (isEmptyHelper(d->qt_rgn) || isEmptyHelper(r.d->qt_rgn))
        return *this;
    if (r.d->qt_rgn->contains(*d->qt_rgn))
        return QRegion();
    if (!EXTENTCHECK(&d->qt_rgn->extents, &r.d->qt_rgn->extents))
        return *this;
    if (d == r.d || EqualRegion(d->qt_rgn, r.d->qt_rgn))
        return QRegion();

#ifdef QT_REGION_DEBUG
    d->qt_rgn->selfTest();
    r.d->qt_rgn->selfTest();
#endif

    QRegion result;
    result.detach();
    SubtractRegion(d->qt_rgn, r.d->qt_rgn, *result.d->qt_rgn);
#ifdef QT_REGION_DEBUG
    result.d->qt_rgn->selfTest();
#endif
    return result;
}

QRegion QRegion::xored(const QRegion &r) const
{
    if (isEmptyHelper(d->qt_rgn)) {
        return r;
    } else if (isEmptyHelper(r.d->qt_rgn)) {
        return *this;
    } else if (!EXTENTCHECK(&d->qt_rgn->extents, &r.d->qt_rgn->extents)) {
        return (*this + r);
    } else if (d == r.d || EqualRegion(d->qt_rgn, r.d->qt_rgn)) {
        return QRegion();
    } else {
        QRegion result;
        result.detach();
        XorRegion(d->qt_rgn, r.d->qt_rgn, *result.d->qt_rgn);
        return result;
    }
}

QRect QRegion::boundingRect() const
{
    if (isEmpty())
        return QRect();
    return d->qt_rgn->extents;
}

/*! \internal
    Returns \c true if \a rect is guaranteed to be fully contained in \a region.
    A false return value does not guarantee the opposite.
*/
Q_GUI_EXPORT
bool qt_region_strictContains(const QRegion &region, const QRect &rect)
{
    if (isEmptyHelper(region.d->qt_rgn) || !rect.isValid())
        return false;

#if 0 // TEST_INNERRECT
    static bool guard = false;
    if (guard)
        return false;
    guard = true;
    QRegion inner = region.d->qt_rgn->innerRect;
    Q_ASSERT((inner - region).isEmpty());
    guard = false;

    int maxArea = 0;
    for (int i = 0; i < region.d->qt_rgn->numRects; ++i) {
        const QRect r = region.d->qt_rgn->rects.at(i);
        if (r.width() * r.height() > maxArea)
            maxArea = r.width() * r.height();
    }

    if (maxArea > region.d->qt_rgn->innerArea) {
        qDebug() << "not largest rectangle" << region << region.d->qt_rgn->innerRect;
    }
    Q_ASSERT(maxArea <= region.d->qt_rgn->innerArea);
#endif

    const QRect r1 = region.d->qt_rgn->innerRect;
    return (rect.left() >= r1.left() && rect.right() <= r1.right()
            && rect.top() >= r1.top() && rect.bottom() <= r1.bottom());
}

QVector<QRect> QRegion::rects() const
{
    if (d->qt_rgn) {
        d->qt_rgn->vectorize();
        d->qt_rgn->rects.reserve(d->qt_rgn->numRects);
        d->qt_rgn->rects.resize(d->qt_rgn->numRects);
        return d->qt_rgn->rects;
    } else {
        return QVector<QRect>();
    }
}

void QRegion::setRects(const QRect *rects, int num)
{
    *this = QRegion();
    if (!rects || num == 0 || (num == 1 && rects->isEmpty()))
        return;

    detach();

    d->qt_rgn->numRects = num;
    if (num == 1) {
        d->qt_rgn->extents = *rects;
        d->qt_rgn->innerRect = *rects;
    } else {
        d->qt_rgn->rects.resize(num);

        int left = INT_MAX,
            right = INT_MIN,
            top = INT_MAX,
            bottom = INT_MIN;
        for (int i = 0; i < num; ++i) {
            const QRect &rect = rects[i];
            d->qt_rgn->rects[i] = rect;
            left = qMin(rect.left(), left);
            right = qMax(rect.right(), right);
            top = qMin(rect.top(), top);
            bottom = qMax(rect.bottom(), bottom);
            d->qt_rgn->updateInnerRect(rect);
        }
        d->qt_rgn->extents = QRect(QPoint(left, top), QPoint(right, bottom));
    }
}

int QRegion::rectCount() const
{
    return (d->qt_rgn ? d->qt_rgn->numRects : 0);
}


bool QRegion::operator==(const QRegion &r) const
{
    if (!d->qt_rgn)
        return r.isEmpty();
    if (!r.d->qt_rgn)
        return isEmpty();

    if (d == r.d)
        return true;
    else
        return EqualRegion(d->qt_rgn, r.d->qt_rgn);
}

bool QRegion::intersects(const QRect &rect) const
{
    if (isEmptyHelper(d->qt_rgn) || rect.isNull())
        return false;

    const QRect r = rect.normalized();
    if (!rect_intersects(d->qt_rgn->extents, r))
        return false;
    if (d->qt_rgn->numRects == 1)
        return true;

    const QVector<QRect> myRects = rects();
    for (QVector<QRect>::const_iterator it = myRects.constBegin(); it < myRects.constEnd(); ++it)
        if (rect_intersects(r, *it))
            return true;
    return false;
}


#endif
QT_END_NAMESPACE
