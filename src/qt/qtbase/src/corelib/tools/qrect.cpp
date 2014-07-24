/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
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

#include "qrect.h"
#include "qdatastream.h"
#include "qdebug.h"
#include "qmath.h"

#include <math.h>

QT_BEGIN_NAMESPACE

/*!
    \class QRect
    \inmodule QtCore
    \ingroup painting
    \reentrant

    \brief The QRect class defines a rectangle in the plane using
    integer precision.

    A rectangle is normally expressed as an upper-left corner and a
    size.  The size (width and height) of a QRect is always equivalent
    to the mathematical rectangle that forms the basis for its
    rendering.

    A QRect can be constructed with a set of left, top, width and
    height integers, or from a QPoint and a QSize.  The following code
    creates two identical rectangles.

    \snippet code/src_corelib_tools_qrect.cpp 0

    There is a third constructor that creates a QRect using the
    top-left and bottom-right coordinates, but we recommend that you
    avoid using it. The rationale is that for historical reasons the
    values returned by the bottom() and right() functions deviate from
    the true bottom-right corner of the rectangle.

    The QRect class provides a collection of functions that return the
    various rectangle coordinates, and enable manipulation of
    these. QRect also provide functions to move the rectangle relative
    to the various coordinates. In addition there is a moveTo()
    function that moves the rectangle, leaving its top left corner at
    the given coordinates. Alternatively, the translate() function
    moves the rectangle the given offset relative to the current
    position, and the translated() function returns a translated copy
    of this rectangle.

    The size() function returns the rectange's dimensions as a
    QSize. The dimensions can also be retrieved separately using the
    width() and height() functions. To manipulate the dimensions use
    the setSize(), setWidth() or setHeight() functions. Alternatively,
    the size can be changed by applying either of the functions
    setting the rectangle coordinates, for example, setBottom() or
    setRight().

    The contains() function tells whether a given point is inside the
    rectangle or not, and the intersects() function returns \c true if
    this rectangle intersects with a given rectangle. The QRect class
    also provides the intersected() function which returns the
    intersection rectangle, and the united() function which returns the
    rectangle that encloses the given rectangle and this:

    \table
    \row
    \li \inlineimage qrect-intersect.png
    \li \inlineimage qrect-unite.png
    \row
    \li intersected()
    \li united()
    \endtable

    The isEmpty() function returns \c true if left() > right() or top() >
    bottom(). Note that an empty rectangle is not valid: The isValid()
    function returns \c true if left() <= right() \e and top() <=
    bottom(). A null rectangle (isNull() == true) on the other hand,
    has both width and height set to 0.

    Note that due to the way QRect and QRectF are defined, an
    empty QRect is defined in essentially the same way as QRectF.

    Finally, QRect objects can be streamed as well as compared.

    \tableofcontents

    \section1 Rendering

    When using an \l {QPainter::Antialiasing}{anti-aliased} painter,
    the boundary line of a QRect will be rendered symmetrically on
    both sides of the mathematical rectangle's boundary line. But when
    using an aliased painter (the default) other rules apply.

    Then, when rendering with a one pixel wide pen the QRect's boundary
    line will be rendered to the right and below the mathematical
    rectangle's boundary line.

    When rendering with a two pixels wide pen the boundary line will
    be split in the middle by the mathematical rectangle. This will be
    the case whenever the pen is set to an even number of pixels,
    while rendering with a pen with an odd number of pixels, the spare
    pixel will be rendered to the right and below the mathematical
    rectangle as in the one pixel case.

    \table
    \row
        \li \inlineimage qrect-diagram-zero.png
        \li \inlineimage qrect-diagram-one.png
    \row
        \li Logical representation
        \li One pixel wide pen
    \row
        \li \inlineimage qrect-diagram-two.png
        \li \inlineimage qrect-diagram-three.png
    \row
        \li Two pixel wide pen
        \li Three pixel wide pen
    \endtable

    \section1 Coordinates

    The QRect class provides a collection of functions that return the
    various rectangle coordinates, and enable manipulation of
    these. QRect also provide functions to move the rectangle relative
    to the various coordinates.

    For example the left(), setLeft() and moveLeft() functions as an
    example: left() returns the x-coordinate of the rectangle's left
    edge, setLeft() sets the left edge of the rectangle to the given x
    coordinate (it may change the width, but will never change the
    rectangle's right edge) and moveLeft() moves the entire rectangle
    horizontally, leaving the rectangle's left edge at the given x
    coordinate and its size unchanged.

    \image qrect-coordinates.png

    Note that for historical reasons the values returned by the
    bottom() and right() functions deviate from the true bottom-right
    corner of the rectangle: The right() function returns \e { left()
    + width() - 1} and the bottom() function returns \e {top() +
    height() - 1}. The same is the case for the point returned by the
    bottomRight() convenience function. In addition, the x and y
    coordinate of the topRight() and bottomLeft() functions,
    respectively, contain the same deviation from the true right and
    bottom edges.

    We recommend that you use x() + width() and y() + height() to find
    the true bottom-right corner, and avoid right() and
    bottom(). Another solution is to use QRectF: The QRectF class
    defines a rectangle in the plane using floating point accuracy for
    coordinates, and the QRectF::right() and QRectF::bottom()
    functions \e do return the right and bottom coordinates.

    It is also possible to add offsets to this rectangle's coordinates
    using the adjust() function, as well as retrieve a new rectangle
    based on adjustments of the original one using the adjusted()
    function. If either of the width and height is negative, use the
    normalized() function to retrieve a rectangle where the corners
    are swapped.

    In addition, QRect provides the getCoords() function which extracts
    the position of the rectangle's top-left and bottom-right corner,
    and the getRect() function which extracts the rectangle's top-left
    corner, width and height. Use the setCoords() and setRect()
    function to manipulate the rectangle's coordinates and dimensions
    in one go.

    \section1 Constraints

    QRect is limited to the minimum and maximum values for the \c int type.
    Operations on a QRect that could potentially result in values outside this
    range will result in undefined behavior.

    \sa QRectF, QRegion
*/

/*****************************************************************************
  QRect member functions
 *****************************************************************************/

/*!
    \fn QRect::QRect()

    Constructs a null rectangle.

    \sa isNull()
*/

/*!
    \fn QRect::QRect(const QPoint &topLeft, const QPoint &bottomRight)

    Constructs a rectangle with the given \a topLeft and \a bottomRight corners.

    \sa setTopLeft(), setBottomRight()
*/


/*!
    \fn QRect::QRect(const QPoint &topLeft, const QSize &size)

    Constructs a rectangle with the given \a topLeft corner and the
    given \a size.

    \sa setTopLeft(), setSize()
*/


/*!
    \fn QRect::QRect(int x, int y, int width, int height)

    Constructs a rectangle with (\a x, \a y) as its top-left corner
    and the given \a width and \a height.

    \sa setRect()
*/


/*!
    \fn bool QRect::isNull() const

    Returns \c true if the rectangle is a null rectangle, otherwise
    returns \c false.

    A null rectangle has both the width and the height set to 0 (i.e.,
    right() == left() - 1 and bottom() == top() - 1). A null rectangle
    is also empty, and hence is not valid.

    \sa isEmpty(), isValid()
*/

/*!
    \fn bool QRect::isEmpty() const

    Returns \c true if the rectangle is empty, otherwise returns \c false.

    An empty rectangle has a left() > right() or top() > bottom(). An
    empty rectangle is not valid (i.e., isEmpty() == !isValid()).

    Use the normalized() function to retrieve a rectangle where the
    corners are swapped.

    \sa isNull(), isValid(), normalized()
*/

/*!
    \fn bool QRect::isValid() const

    Returns \c true if the rectangle is valid, otherwise returns \c false.

    A valid rectangle has a left() < right() and top() <
    bottom(). Note that non-trivial operations like intersections are
    not defined for invalid rectangles. A valid rectangle is not empty
    (i.e., isValid() == !isEmpty()).

    \sa isNull(), isEmpty(), normalized()
*/


/*!
    Returns a normalized rectangle; i.e., a rectangle that has a
    non-negative width and height.

    If width() < 0 the function swaps the left and right corners, and
    it swaps the top and bottom corners if height() < 0.

    \sa isValid(), isEmpty()
*/

QRect QRect::normalized() const
{
    QRect r;
    if (x2 < x1 - 1) {                                // swap bad x values
        r.x1 = x2;
        r.x2 = x1;
    } else {
        r.x1 = x1;
        r.x2 = x2;
    }
    if (y2 < y1 - 1) {                                // swap bad y values
        r.y1 = y2;
        r.y2 = y1;
    } else {
        r.y1 = y1;
        r.y2 = y2;
    }
    return r;
}


/*!
    \fn int QRect::left() const

    Returns the x-coordinate of the rectangle's left edge. Equivalent
    to x().

    \sa setLeft(), topLeft(), bottomLeft()
*/

/*!
    \fn int QRect::top() const

    Returns the y-coordinate of the rectangle's top edge.
    Equivalent to y().

    \sa setTop(), topLeft(), topRight()
*/

/*!
    \fn int QRect::right() const

    Returns the x-coordinate of the rectangle's right edge.

    Note that for historical reasons this function returns left() +
    width() - 1; use x() + width() to retrieve the true x-coordinate.

    \sa setRight(), topRight(), bottomRight()
*/

/*!
    \fn int QRect::bottom() const

    Returns the y-coordinate of the rectangle's bottom edge.

    Note that for historical reasons this function returns top() +
    height() - 1; use y() + height() to retrieve the true y-coordinate.

    \sa setBottom(), bottomLeft(), bottomRight()
*/

/*!
    \fn int QRect::x() const

    Returns the x-coordinate of the rectangle's left edge. Equivalent to left().

    \sa setX(), y(), topLeft()
*/

/*!
    \fn int QRect::y() const

    Returns the y-coordinate of the rectangle's top edge. Equivalent to top().

    \sa setY(), x(), topLeft()
*/

/*!
    \fn void QRect::setLeft(int x)

    Sets the left edge of the rectangle to the given \a x
    coordinate. May change the width, but will never change the right
    edge of the rectangle.

    Equivalent to setX().

    \sa left(), moveLeft()
*/

/*!
    \fn void QRect::setTop(int y)

    Sets the top edge of the rectangle to the given \a y
    coordinate. May change the height, but will never change the
    bottom edge of the rectangle.

    Equivalent to setY().

    \sa top(), moveTop()
*/

/*!
    \fn void QRect::setRight(int x)

    Sets the right edge of the rectangle to the given \a x
    coordinate. May change the width, but will never change the left
    edge of the rectangle.

    \sa right(), moveRight()
*/

/*!
    \fn void QRect::setBottom(int y)

    Sets the bottom edge of the rectangle to the given \a y
    coordinate. May change the height, but will never change the top
    edge of the rectangle.

    \sa bottom(), moveBottom(),
*/

/*!
    \fn void QRect::setX(int x)

    Sets the left edge of the rectangle to the given \a x
    coordinate. May change the width, but will never change the right
    edge of the rectangle.

    Equivalent to setLeft().

    \sa x(), setY(), setTopLeft()
*/

/*!
    \fn void QRect::setY(int y)

    Sets the top edge of the rectangle to the given \a y
    coordinate. May change the height, but will never change the
    bottom edge of the rectangle.

    Equivalent to setTop().

    \sa y(), setX(), setTopLeft()
*/

/*!
    \fn void QRect::setTopLeft(const QPoint &position)

    Set the top-left corner of the rectangle to the given \a
    position. May change the size, but will never change the
    bottom-right corner of the rectangle.

    \sa topLeft(), moveTopLeft()
*/

/*!
    \fn void QRect::setBottomRight(const QPoint &position)

    Set the bottom-right corner of the rectangle to the given \a
    position. May change the size, but will never change the
    top-left corner of the rectangle.

    \sa bottomRight(), moveBottomRight()
*/

/*!
    \fn void QRect::setTopRight(const QPoint &position)

    Set the top-right corner of the rectangle to the given \a
    position. May change the size, but will never change the
    bottom-left corner of the rectangle.

    \sa topRight(), moveTopRight()
*/

/*!
    \fn void QRect::setBottomLeft(const QPoint &position)

    Set the bottom-left corner of the rectangle to the given \a
    position. May change the size, but will never change the
    top-right corner of the rectangle.

    \sa bottomLeft(), moveBottomLeft()
*/

/*!
    \fn QPoint QRect::topLeft() const

    Returns the position of the rectangle's top-left corner.

    \sa setTopLeft(), top(), left()
*/

/*!
    \fn QPoint QRect::bottomRight() const

    Returns the position of the rectangle's bottom-right corner.

    Note that for historical reasons this function returns
    QPoint(left() + width() -1, top() + height() - 1).

    \sa setBottomRight(), bottom(), right()
*/

/*!
    \fn QPoint QRect::topRight() const

    Returns the position of the rectangle's top-right corner.

    Note that for historical reasons this function returns
    QPoint(left() + width() -1, top()).

    \sa setTopRight(), top(), right()
*/

/*!
    \fn QPoint QRect::bottomLeft() const

    Returns the position of the rectangle's bottom-left corner. Note
    that for historical reasons this function returns QPoint(left(),
    top() + height() - 1).

    \sa setBottomLeft(), bottom(), left()
*/

/*!
    \fn QPoint QRect::center() const

    Returns the center point of the rectangle.

    \sa moveCenter()
*/


/*!
    \fn void QRect::getRect(int *x, int *y, int *width, int *height) const

    Extracts the position of the rectangle's top-left corner to *\a x
    and *\a y, and its dimensions to *\a width and *\a height.

    \sa setRect(), getCoords()
*/


/*!
    \fn void QRect::getCoords(int *x1, int *y1, int *x2, int *y2) const

    Extracts the position of the rectangle's top-left corner to *\a x1
    and *\a y1, and the position of the bottom-right corner to *\a x2
    and *\a y2.

    \sa setCoords(), getRect()
*/

/*!
    \fn void QRect::moveLeft(int x)

    Moves the rectangle horizontally, leaving the rectangle's left
    edge at the given \a x coordinate. The rectangle's size is
    unchanged.

    \sa left(), setLeft(), moveRight()
*/

/*!
    \fn void QRect::moveTop(int y)

    Moves the rectangle vertically, leaving the rectangle's top edge
    at the given \a y coordinate. The rectangle's size is unchanged.

    \sa top(), setTop(), moveBottom()
*/


/*!
    \fn void QRect::moveRight(int x)

    Moves the rectangle horizontally, leaving the rectangle's right
    edge at the given \a x coordinate. The rectangle's size is
    unchanged.

    \sa right(), setRight(), moveLeft()
*/


/*!
    \fn void QRect::moveBottom(int y)

    Moves the rectangle vertically, leaving the rectangle's bottom
    edge at the given \a y coordinate. The rectangle's size is
    unchanged.

    \sa bottom(), setBottom(), moveTop()
*/


/*!
    \fn void QRect::moveTopLeft(const QPoint &position)

    Moves the rectangle, leaving the top-left corner at the given \a
    position. The rectangle's size is unchanged.

    \sa setTopLeft(), moveTop(), moveLeft()
*/


/*!
    \fn void QRect::moveBottomRight(const QPoint &position)

    Moves the rectangle, leaving the bottom-right corner at the given
    \a position. The rectangle's size is unchanged.

    \sa setBottomRight(), moveRight(), moveBottom()
*/


/*!
    \fn void QRect::moveTopRight(const QPoint &position)

    Moves the rectangle, leaving the top-right corner at the given \a
    position. The rectangle's size is unchanged.

    \sa setTopRight(), moveTop(), moveRight()
*/


/*!
    \fn void QRect::moveBottomLeft(const QPoint &position)

    Moves the rectangle, leaving the bottom-left corner at the given
    \a position. The rectangle's size is unchanged.

    \sa setBottomLeft(), moveBottom(), moveLeft()
*/


/*!
    \fn void QRect::moveCenter(const QPoint &position)

    Moves the rectangle, leaving the center point at the given \a
    position. The rectangle's size is unchanged.

    \sa center()
*/

/*!
    \fn void QRect::moveTo(int x, int y)

    Moves the rectangle, leaving the top-left corner at the given
    position (\a x, \a y).  The rectangle's size is unchanged.

    \sa translate(), moveTopLeft()
*/

/*!
    \fn void QRect::moveTo(const QPoint &position)

    Moves the rectangle, leaving the top-left corner at the given \a
    position.
*/

/*!
    \fn void QRect::translate(int dx, int dy)

    Moves the rectangle \a dx along the x axis and \a dy along the y
    axis, relative to the current position. Positive values move the
    rectangle to the right and down.

    \sa moveTopLeft(), moveTo(), translated()
*/


/*!
    \fn void QRect::translate(const QPoint &offset)
    \overload

    Moves the rectangle \a{offset}.\l{QPoint::x()}{x()} along the x
    axis and \a{offset}.\l{QPoint::y()}{y()} along the y axis,
    relative to the current position.
*/


/*!
    \fn QRect QRect::translated(int dx, int dy) const

    Returns a copy of the rectangle that is translated \a dx along the
    x axis and \a dy along the y axis, relative to the current
    position. Positive values move the rectangle to the right and
    down.

    \sa translate()

*/


/*!
    \fn QRect QRect::translated(const QPoint &offset) const

    \overload

    Returns a copy of the rectangle that is translated
    \a{offset}.\l{QPoint::x()}{x()} along the x axis and
    \a{offset}.\l{QPoint::y()}{y()} along the y axis, relative to the
    current position.
*/


/*!
    \fn void QRect::setRect(int x, int y, int width, int height)

    Sets the coordinates of the rectangle's top-left corner to (\a{x},
    \a{y}), and its size to the given \a width and \a height.

    \sa getRect(), setCoords()
*/


/*!
    \fn void QRect::setCoords(int x1, int y1, int x2, int y2)

    Sets the coordinates of the rectangle's top-left corner to (\a x1,
    \a y1), and the coordinates of its bottom-right corner to (\a x2,
    \a y2).

    \sa getCoords(), setRect()
*/


/*! \fn QRect QRect::adjusted(int dx1, int dy1, int dx2, int dy2) const

    Returns a new rectangle with \a dx1, \a dy1, \a dx2 and \a dy2
    added respectively to the existing coordinates of this rectangle.

    \sa adjust()
*/

/*! \fn void QRect::adjust(int dx1, int dy1, int dx2, int dy2)

    Adds \a dx1, \a dy1, \a dx2 and \a dy2 respectively to the
    existing coordinates of the rectangle.

    \sa adjusted(), setRect()
*/

/*!
    \fn QSize QRect::size() const

    Returns the size of the rectangle.

    \sa setSize(), width(), height()
*/

/*!
    \fn int QRect::width() const

    Returns the width of the rectangle.

    \sa setWidth(), height(), size()
*/

/*!
    \fn int QRect::height() const

    Returns the height of the rectangle.

    \sa setHeight(), width(), size()
*/

/*!
    \fn void QRect::setWidth(int width)

    Sets the width of the rectangle to the given \a width. The right
    edge is changed, but not the left one.

    \sa width(), setSize()
*/


/*!
    \fn void QRect::setHeight(int height)

    Sets the height of the rectangle to the given \a height. The bottom
    edge is changed, but not the top one.

    \sa height(), setSize()
*/


/*!
    \fn void QRect::setSize(const QSize &size)

    Sets the size of the rectangle to the given \a size. The top-left
    corner is not moved.

    \sa size(), setWidth(), setHeight()
*/


/*!
    \fn bool QRect::contains(const QPoint &point, bool proper) const

    Returns \c true if the given \a point is inside or on the edge of
    the rectangle, otherwise returns \c false. If \a proper is true, this
    function only returns \c true if the given \a point is \e inside the
    rectangle (i.e., not on the edge).

    \sa intersects()
*/

bool QRect::contains(const QPoint &p, bool proper) const
{
    int l, r;
    if (x2 < x1 - 1) {
        l = x2;
        r = x1;
    } else {
        l = x1;
        r = x2;
    }
    if (proper) {
        if (p.x() <= l || p.x() >= r)
            return false;
    } else {
        if (p.x() < l || p.x() > r)
            return false;
    }
    int t, b;
    if (y2 < y1 - 1) {
        t = y2;
        b = y1;
    } else {
        t = y1;
        b = y2;
    }
    if (proper) {
        if (p.y() <= t || p.y() >= b)
            return false;
    } else {
        if (p.y() < t || p.y() > b)
            return false;
    }
    return true;
}


/*!
    \fn bool QRect::contains(int x, int y, bool proper) const
    \overload

    Returns \c true if the point (\a x, \a y) is inside or on the edge of
    the rectangle, otherwise returns \c false. If \a proper is true, this
    function only returns \c true if the point is entirely inside the
    rectangle(not on the edge).
*/

/*!
    \fn bool QRect::contains(int x, int y) const
    \overload

    Returns \c true if the point (\a x, \a y) is inside this rectangle,
    otherwise returns \c false.
*/

/*!
    \fn bool QRect::contains(const QRect &rectangle, bool proper) const
    \overload

    Returns \c true if the given \a rectangle is inside this rectangle.
    otherwise returns \c false. If \a proper is true, this function only
    returns \c true if the \a rectangle is entirely inside this
    rectangle (not on the edge).
*/

bool QRect::contains(const QRect &r, bool proper) const
{
    if (isNull() || r.isNull())
        return false;

    int l1 = x1;
    int r1 = x1;
    if (x2 - x1 + 1 < 0)
        l1 = x2;
    else
        r1 = x2;

    int l2 = r.x1;
    int r2 = r.x1;
    if (r.x2 - r.x1 + 1 < 0)
        l2 = r.x2;
    else
        r2 = r.x2;

    if (proper) {
        if (l2 <= l1 || r2 >= r1)
            return false;
    } else {
        if (l2 < l1 || r2 > r1)
            return false;
    }

    int t1 = y1;
    int b1 = y1;
    if (y2 - y1 + 1 < 0)
        t1 = y2;
    else
        b1 = y2;

    int t2 = r.y1;
    int b2 = r.y1;
    if (r.y2 - r.y1 + 1 < 0)
        t2 = r.y2;
    else
        b2 = r.y2;

    if (proper) {
        if (t2 <= t1 || b2 >= b1)
            return false;
    } else {
        if (t2 < t1 || b2 > b1)
            return false;
    }

    return true;
}

/*!
    \fn QRect& QRect::operator|=(const QRect &rectangle)

    Unites this rectangle with the given \a rectangle.

    \sa united(), operator|()
*/

/*!
    \fn QRect& QRect::operator&=(const QRect &rectangle)

    Intersects this rectangle with the given \a rectangle.

    \sa intersected(), operator&()
*/


/*!
    \fn QRect QRect::operator|(const QRect &rectangle) const

    Returns the bounding rectangle of this rectangle and the given \a
    rectangle.

    \sa operator|=(), united()
*/

QRect QRect::operator|(const QRect &r) const
{
    if (isNull())
        return r;
    if (r.isNull())
        return *this;

    int l1 = x1;
    int r1 = x1;
    if (x2 - x1 + 1 < 0)
        l1 = x2;
    else
        r1 = x2;

    int l2 = r.x1;
    int r2 = r.x1;
    if (r.x2 - r.x1 + 1 < 0)
        l2 = r.x2;
    else
        r2 = r.x2;

    int t1 = y1;
    int b1 = y1;
    if (y2 - y1 + 1 < 0)
        t1 = y2;
    else
        b1 = y2;

    int t2 = r.y1;
    int b2 = r.y1;
    if (r.y2 - r.y1 + 1 < 0)
        t2 = r.y2;
    else
        b2 = r.y2;

    QRect tmp;
    tmp.x1 = qMin(l1, l2);
    tmp.x2 = qMax(r1, r2);
    tmp.y1 = qMin(t1, t2);
    tmp.y2 = qMax(b1, b2);
    return tmp;
}

/*!
    \fn QRect QRect::unite(const QRect &rectangle) const
    \obsolete

    Use united(\a rectangle) instead.
*/

/*!
    \fn QRect QRect::united(const QRect &rectangle) const
    \since 4.2

    Returns the bounding rectangle of this rectangle and the given \a rectangle.

    \image qrect-unite.png

    \sa intersected()
*/


/*!
    \fn QRect QRect::operator&(const QRect &rectangle) const

    Returns the intersection of this rectangle and the given \a
    rectangle. Returns an empty rectangle if there is no intersection.

    \sa operator&=(), intersected()
*/

QRect QRect::operator&(const QRect &r) const
{
    if (isNull() || r.isNull())
        return QRect();

    int l1 = x1;
    int r1 = x1;
    if (x2 - x1 + 1 < 0)
        l1 = x2;
    else
        r1 = x2;

    int l2 = r.x1;
    int r2 = r.x1;
    if (r.x2 - r.x1 + 1 < 0)
        l2 = r.x2;
    else
        r2 = r.x2;

    if (l1 > r2 || l2 > r1)
        return QRect();

    int t1 = y1;
    int b1 = y1;
    if (y2 - y1 + 1 < 0)
        t1 = y2;
    else
        b1 = y2;

    int t2 = r.y1;
    int b2 = r.y1;
    if (r.y2 - r.y1 + 1 < 0)
        t2 = r.y2;
    else
        b2 = r.y2;

    if (t1 > b2 || t2 > b1)
        return QRect();

    QRect tmp;
    tmp.x1 = qMax(l1, l2);
    tmp.x2 = qMin(r1, r2);
    tmp.y1 = qMax(t1, t2);
    tmp.y2 = qMin(b1, b2);
    return tmp;
}

/*!
    \fn QRect QRect::intersect(const QRect &rectangle) const
    \obsolete

    Use intersected(\a rectangle) instead.
*/

/*!
    \fn QRect QRect::intersected(const QRect &rectangle) const
    \since 4.2

    Returns the intersection of this rectangle and the given \a
    rectangle. Note that \c{r.intersected(s)} is equivalent to \c{r & s}.

    \image qrect-intersect.png

    \sa intersects(), united(), operator&=()
*/

/*!
    \fn bool QRect::intersects(const QRect &rectangle) const

    Returns \c true if this rectangle intersects with the given \a
    rectangle (i.e., there is at least one pixel that is within both
    rectangles), otherwise returns \c false.

    The intersection rectangle can be retrieved using the intersected()
    function.

    \sa contains()
*/

bool QRect::intersects(const QRect &r) const
{
    if (isNull() || r.isNull())
        return false;

    int l1 = x1;
    int r1 = x1;
    if (x2 - x1 + 1 < 0)
        l1 = x2;
    else
        r1 = x2;

    int l2 = r.x1;
    int r2 = r.x1;
    if (r.x2 - r.x1 + 1 < 0)
        l2 = r.x2;
    else
        r2 = r.x2;

    if (l1 > r2 || l2 > r1)
        return false;

    int t1 = y1;
    int b1 = y1;
    if (y2 - y1 + 1 < 0)
        t1 = y2;
    else
        b1 = y2;

    int t2 = r.y1;
    int b2 = r.y1;
    if (r.y2 - r.y1 + 1 < 0)
        t2 = r.y2;
    else
        b2 = r.y2;

    if (t1 > b2 || t2 > b1)
        return false;

    return true;
}

/*!
    \fn bool operator==(const QRect &r1, const QRect &r2)
    \relates QRect

    Returns \c true if the rectangles \a r1 and \a r2 are equal,
    otherwise returns \c false.
*/


/*!
    \fn bool operator!=(const QRect &r1, const QRect &r2)
    \relates QRect

    Returns \c true if the rectangles \a r1 and \a r2 are different, otherwise
    returns \c false.
*/

/*!
    \fn QRect operator+(const QRect &rectangle, const QMargins &margins)
    \relates QRect

    Returns the \a rectangle grown by the \a margins.

    \since 5.1
*/

/*!
    \fn QRect operator+(const QMargins &margins, const QRect &rectangle)
    \relates QRect
    \overload

    Returns the \a rectangle grown by the \a margins.

    \since 5.1
*/

/*!
    \fn QRect operator-(const QRect &lhs, const QMargins &rhs)
    \relates QRect

    Returns the \a lhs rectangle shrunken by the \a rhs margins.

    \since 5.3
*/

/*!
    \fn QRect QRect::marginsAdded(const QMargins &margins) const

    Returns a rectangle grown by the \a margins.

    \sa operator+=(), marginsRemoved(), operator-=()

    \since 5.1
*/

/*!
    \fn QRect QRect::operator+=(const QMargins &margins)

    Adds the \a margins to the rectangle, growing it.

    \sa marginsAdded(), marginsRemoved(), operator-=()

    \since 5.1
*/

/*!
    \fn QRect QRect::marginsRemoved(const QMargins &margins) const

    Removes the \a margins from the rectangle, shrinking it.

    \sa marginsAdded(), operator+=(), operator-=()

    \since 5.1
*/

/*!
    \fn QRect QRect::operator -=(const QMargins &margins)

    Returns a rectangle shrunk by the \a margins.

    \sa marginsRemoved(), operator+=(), marginsAdded()

    \since 5.1
*/


/*****************************************************************************
  QRect stream functions
 *****************************************************************************/
#ifndef QT_NO_DATASTREAM
/*!
    \fn QDataStream &operator<<(QDataStream &stream, const QRect &rectangle)
    \relates QRect

    Writes the given \a rectangle to the given \a stream, and returns
    a reference to the stream.

    \sa {Serializing Qt Data Types}
*/

QDataStream &operator<<(QDataStream &s, const QRect &r)
{
    if (s.version() == 1)
        s << (qint16)r.left() << (qint16)r.top()
          << (qint16)r.right() << (qint16)r.bottom();
    else
        s << (qint32)r.left() << (qint32)r.top()
          << (qint32)r.right() << (qint32)r.bottom();
    return s;
}

/*!
    \fn QDataStream &operator>>(QDataStream &stream, QRect &rectangle)
    \relates QRect

    Reads a rectangle from the given \a stream into the given \a
    rectangle, and returns a reference to the stream.

    \sa {Serializing Qt Data Types}
*/

QDataStream &operator>>(QDataStream &s, QRect &r)
{
    if (s.version() == 1) {
        qint16 x1, y1, x2, y2;
        s >> x1; s >> y1; s >> x2; s >> y2;
        r.setCoords(x1, y1, x2, y2);
    }
    else {
        qint32 x1, y1, x2, y2;
        s >> x1; s >> y1; s >> x2; s >> y2;
        r.setCoords(x1, y1, x2, y2);
    }
    return s;
}

#endif // QT_NO_DATASTREAM


#ifndef QT_NO_DEBUG_STREAM
QDebug operator<<(QDebug dbg, const QRect &r) {
    dbg.nospace() << "QRect(" << r.x() << ',' << r.y() << ' '
                  << r.width() << 'x' << r.height() << ')';
    return dbg.space();
}
#endif

/*!
    \class QRectF
    \inmodule QtCore
    \ingroup painting
    \reentrant

    \brief The QRectF class defines a rectangle in the plane using floating
    point precision.

    A rectangle is normally expressed as an upper-left corner and a
    size.  The size (width and height) of a QRectF is always equivalent
    to the mathematical rectangle that forms the basis for its
    rendering.

    A QRectF can be constructed with a set of left, top, width and
    height integers, or from a QPoint and a QSize.  The following code
    creates two identical rectangles.

    \snippet code/src_corelib_tools_qrect.cpp 1

    There is also a third constructor creating a QRectF from a QRect,
    and a corresponding toRect() function that returns a QRect object
    based on the values of this rectangle (note that the coordinates
    in the returned rectangle are rounded to the nearest integer).

    The QRectF class provides a collection of functions that return
    the various rectangle coordinates, and enable manipulation of
    these. QRectF also provide functions to move the rectangle
    relative to the various coordinates. In addition there is a
    moveTo() function that moves the rectangle, leaving its top left
    corner at the given coordinates. Alternatively, the translate()
    function moves the rectangle the given offset relative to the
    current position, and the translated() function returns a
    translated copy of this rectangle.

    The size() function returns the rectange's dimensions as a
    QSize. The dimensions can also be retrieved separately using the
    width() and height() functions. To manipulate the dimensions use
    the setSize(), setWidth() or setHeight() functions. Alternatively,
    the size can be changed by applying either of the functions
    setting the rectangle coordinates, for example, setBottom() or
    setRight().

    The contains() function tells whether a given point is inside the
    rectangle or not, and the intersects() function returns \c true if
    this rectangle intersects with a given rectangle (otherwise
    false). The QRectF class also provides the intersected() function
    which returns the intersection rectangle, and the united() function
    which returns the rectangle that encloses the given rectangle and
    this:

    \table
    \row
    \li \inlineimage qrect-intersect.png
    \li \inlineimage qrect-unite.png
    \row
    \li intersected()
    \li united()
    \endtable

    The isEmpty() function returns \c true if the rectangle's width or
    height is less than, or equal to, 0. Note that an empty rectangle
    is not valid: The isValid() function returns \c true if both width
    and height is larger than 0. A null rectangle (isNull() == true)
    on the other hand, has both width and height set to 0.

    Note that due to the way QRect and QRectF are defined, an
    empty QRectF is defined in essentially the same way as QRect.

    Finally, QRectF objects can be streamed as well as compared.

    \tableofcontents

    \section1 Rendering

    When using an \l {QPainter::Antialiasing}{anti-aliased} painter,
    the boundary line of a QRectF will be rendered symmetrically on both
    sides of the mathematical rectangle's boundary line. But when
    using an aliased painter (the default) other rules apply.

    Then, when rendering with a one pixel wide pen the QRectF's boundary
    line will be rendered to the right and below the mathematical
    rectangle's boundary line.

    When rendering with a two pixels wide pen the boundary line will
    be split in the middle by the mathematical rectangle. This will be
    the case whenever the pen is set to an even number of pixels,
    while rendering with a pen with an odd number of pixels, the spare
    pixel will be rendered to the right and below the mathematical
    rectangle as in the one pixel case.

    \table
    \row
        \li \inlineimage qrect-diagram-zero.png
        \li \inlineimage qrectf-diagram-one.png
    \row
        \li Logical representation
        \li One pixel wide pen
    \row
        \li \inlineimage qrectf-diagram-two.png
        \li \inlineimage qrectf-diagram-three.png
    \row
        \li Two pixel wide pen
        \li Three pixel wide pen
    \endtable

    \section1 Coordinates

    The QRectF class provides a collection of functions that return
    the various rectangle coordinates, and enable manipulation of
    these. QRectF also provide functions to move the rectangle
    relative to the various coordinates.

    For example: the bottom(), setBottom() and moveBottom() functions:
    bottom() returns the y-coordinate of the rectangle's bottom edge,
    setBottom() sets the bottom edge of the rectangle to the given y
    coordinate (it may change the height, but will never change the
    rectangle's top edge) and moveBottom() moves the entire rectangle
    vertically, leaving the rectangle's bottom edge at the given y
    coordinate and its size unchanged.

    \image qrectf-coordinates.png

    It is also possible to add offsets to this rectangle's coordinates
    using the adjust() function, as well as retrieve a new rectangle
    based on adjustments of the original one using the adjusted()
    function. If either of the width and height is negative, use the
    normalized() function to retrieve a rectangle where the corners
    are swapped.

    In addition, QRectF provides the getCoords() function which extracts
    the position of the rectangle's top-left and bottom-right corner,
    and the getRect() function which extracts the rectangle's top-left
    corner, width and height. Use the setCoords() and setRect()
    function to manipulate the rectangle's coordinates and dimensions
    in one go.

    \sa QRect, QRegion
*/

/*****************************************************************************
  QRectF member functions
 *****************************************************************************/

/*!
    \fn QRectF::QRectF()

    Constructs a null rectangle.

    \sa isNull()
*/

/*!
    \fn QRectF::QRectF(const QPointF &topLeft, const QSizeF &size)

    Constructs a rectangle with the given \a topLeft corner and the given \a size.

    \sa setTopLeft(), setSize()
*/

/*!
    \fn QRectF::QRectF(const QPointF &topLeft, const QPointF &bottomRight)
    \since 4.3

    Constructs a rectangle with the given \a topLeft and \a bottomRight corners.

    \sa setTopLeft(), setBottomRight()
*/

/*!
    \fn QRectF::QRectF(qreal x, qreal y, qreal width, qreal height)

    Constructs a rectangle with (\a x, \a y) as its top-left corner
    and the given \a width and \a height.

    \sa setRect()
*/

/*!
    \fn QRectF::QRectF(const QRect &rectangle)

    Constructs a QRectF rectangle from the given QRect \a rectangle.

    \sa toRect()
*/

/*!
    \fn bool QRectF::isNull() const

    Returns \c true if the rectangle is a null rectangle, otherwise returns \c false.

    A null rectangle has both the width and the height set to 0. A
    null rectangle is also empty, and hence not valid.

    \sa isEmpty(), isValid()
*/

/*!
    \fn bool QRectF::isEmpty() const

    Returns \c true if the rectangle is empty, otherwise returns \c false.

    An empty rectangle has width() <= 0 or height() <= 0.  An empty
    rectangle is not valid (i.e., isEmpty() == !isValid()).

    Use the normalized() function to retrieve a rectangle where the
    corners are swapped.

    \sa isNull(), isValid(), normalized()
*/

/*!
    \fn bool QRectF::isValid() const

    Returns \c true if the rectangle is valid, otherwise returns \c false.

    A valid rectangle has a width() > 0 and height() > 0. Note that
    non-trivial operations like intersections are not defined for
    invalid rectangles. A valid rectangle is not empty (i.e., isValid()
    == !isEmpty()).

    \sa isNull(), isEmpty(), normalized()
*/


/*!
    Returns a normalized rectangle; i.e., a rectangle that has a
    non-negative width and height.

    If width() < 0 the function swaps the left and right corners, and
    it swaps the top and bottom corners if height() < 0.

    \sa isValid(), isEmpty()
*/

QRectF QRectF::normalized() const
{
    QRectF r = *this;
    if (r.w < 0) {
        r.xp += r.w;
        r.w = -r.w;
    }
    if (r.h < 0) {
        r.yp += r.h;
        r.h = -r.h;
    }
    return r;
}

/*!
    \fn qreal QRectF::x() const

    Returns the x-coordinate of the rectangle's left edge. Equivalent
    to left().


    \sa setX(), y(), topLeft()
*/

/*!
    \fn qreal QRectF::y() const

    Returns the y-coordinate of the rectangle's top edge. Equivalent
    to top().

    \sa setY(), x(), topLeft()
*/


/*!
    \fn void QRectF::setLeft(qreal x)

    Sets the left edge of the rectangle to the given \a x
    coordinate. May change the width, but will never change the right
    edge of the rectangle.

    Equivalent to setX().

    \sa left(), moveLeft()
*/

/*!
    \fn void QRectF::setTop(qreal y)

    Sets the top edge of the rectangle to the given \a y coordinate. May
    change the height, but will never change the bottom edge of the
    rectangle.

    Equivalent to setY().

    \sa top(), moveTop()
*/

/*!
    \fn void QRectF::setRight(qreal x)

    Sets the right edge of the rectangle to the given \a x
    coordinate. May change the width, but will never change the left
    edge of the rectangle.

    \sa right(), moveRight()
*/

/*!
    \fn void QRectF::setBottom(qreal y)

    Sets the bottom edge of the rectangle to the given \a y
    coordinate. May change the height, but will never change the top
    edge of the rectangle.

    \sa bottom(), moveBottom()
*/

/*!
    \fn void QRectF::setX(qreal x)

    Sets the left edge of the rectangle to the given \a x
    coordinate. May change the width, but will never change the right
    edge of the rectangle.

    Equivalent to setLeft().

    \sa x(), setY(), setTopLeft()
*/

/*!
    \fn void QRectF::setY(qreal y)

    Sets the top edge of the rectangle to the given \a y
    coordinate. May change the height, but will never change the
    bottom edge of the rectangle.

    Equivalent to setTop().

    \sa y(), setX(), setTopLeft()
*/

/*!
    \fn void QRectF::setTopLeft(const QPointF &position)

    Set the top-left corner of the rectangle to the given \a
    position. May change the size, but will never change the
    bottom-right corner of the rectangle.

    \sa topLeft(), moveTopLeft()
*/

/*!
    \fn void QRectF::setBottomRight(const QPointF &position)

    Set the bottom-right corner of the rectangle to the given \a
    position. May change the size, but will never change the
    top-left corner of the rectangle.

    \sa bottomRight(), moveBottomRight()
*/

/*!
    \fn void QRectF::setTopRight(const QPointF &position)

    Set the top-right corner of the rectangle to the given \a
    position. May change the size, but will never change the
    bottom-left corner of the rectangle.

    \sa topRight(), moveTopRight()
*/

/*!
    \fn void QRectF::setBottomLeft(const QPointF &position)

    Set the bottom-left corner of the rectangle to the given \a
    position. May change the size, but will never change the
    top-right corner of the rectangle.

    \sa bottomLeft(), moveBottomLeft()
*/

/*!
    \fn QPointF QRectF::center() const

    Returns the center point of the rectangle.

    \sa moveCenter()
*/


/*!
    \fn void QRectF::getRect(qreal *x, qreal *y, qreal *width, qreal *height) const

    Extracts the position of the rectangle's top-left corner to *\a x and
    *\a y, and its dimensions to *\a width and *\a height.

    \sa setRect(), getCoords()
*/


/*!
    \fn void QRectF::getCoords(qreal *x1, qreal *y1, qreal *x2, qreal *y2) const

    Extracts the position of the rectangle's top-left corner to *\a x1
    and *\a y1, and the position of the bottom-right corner to *\a x2 and
    *\a y2.

    \sa setCoords(), getRect()
*/

/*!
    \fn void QRectF::moveLeft(qreal x)

    Moves the rectangle horizontally, leaving the rectangle's left
    edge at the given \a x coordinate. The rectangle's size is
    unchanged.

    \sa left(), setLeft(), moveRight()
*/

/*!
    \fn void QRectF::moveTop(qreal y)

    Moves the rectangle vertically, leaving the rectangle's top line
    at the given \a y coordinate. The rectangle's size is unchanged.

    \sa top(), setTop(), moveBottom()
*/


/*!
    \fn void QRectF::moveRight(qreal x)

    Moves the rectangle horizontally, leaving the rectangle's right
    edge at the given \a x coordinate. The rectangle's size is
    unchanged.

    \sa right(), setRight(), moveLeft()
*/


/*!
    \fn void QRectF::moveBottom(qreal y)

    Moves the rectangle vertically, leaving the rectangle's bottom
    edge at the given \a y coordinate. The rectangle's size is
    unchanged.

    \sa bottom(), setBottom(), moveTop()
*/


/*!
    \fn void QRectF::moveTopLeft(const QPointF &position)

    Moves the rectangle, leaving the top-left corner at the given \a
    position. The rectangle's size is unchanged.

    \sa setTopLeft(), moveTop(), moveLeft()
*/


/*!
    \fn void QRectF::moveBottomRight(const QPointF &position)

    Moves the rectangle, leaving the bottom-right corner at the given
    \a position. The rectangle's size is unchanged.

    \sa setBottomRight(), moveBottom(), moveRight()
*/


/*!
    \fn void QRectF::moveTopRight(const QPointF &position)

    Moves the rectangle, leaving the top-right corner at the given
    \a position. The rectangle's size is unchanged.

    \sa setTopRight(), moveTop(), moveRight()
*/


/*!
    \fn void QRectF::moveBottomLeft(const QPointF &position)

    Moves the rectangle, leaving the bottom-left corner at the given
    \a position. The rectangle's size is unchanged.

    \sa setBottomLeft(), moveBottom(), moveLeft()
*/


/*!
    \fn void QRectF::moveTo(qreal x, qreal y)

    Moves the rectangle, leaving the top-left corner at the given
    position (\a x, \a y). The rectangle's size is unchanged.

    \sa translate(), moveTopLeft()
*/

/*!
    \fn void QRectF::moveTo(const QPointF &position)
    \overload

    Moves the rectangle, leaving the top-left corner at the given \a
    position.
*/

/*!
    \fn void QRectF::translate(qreal dx, qreal dy)

    Moves the rectangle \a dx along the x-axis and \a dy along the y-axis,
    relative to the current position. Positive values move the rectangle to the
    right and downwards.

    \sa moveTopLeft(), moveTo(), translated()
*/


/*!
    \fn void QRectF::translate(const QPointF &offset)
    \overload

    Moves the rectangle \a{offset}.\l{QPointF::x()}{x()} along the x
    axis and \a{offset}.\l{QPointF::y()}{y()} along the y axis,
    relative to the current position.
*/


/*!
    \fn QRectF QRectF::translated(qreal dx, qreal dy) const

    Returns a copy of the rectangle that is translated \a dx along the
    x axis and \a dy along the y axis, relative to the current
    position. Positive values move the rectangle to the right and
    down.

    \sa translate()
*/


/*!
    \fn QRectF QRectF::translated(const QPointF &offset) const
    \overload

    Returns a copy of the rectangle that is translated
    \a{offset}.\l{QPointF::x()}{x()} along the x axis and
    \a{offset}.\l{QPointF::y()}{y()} along the y axis, relative to the
    current position.
*/


/*!
    \fn void QRectF::setRect(qreal x, qreal y, qreal width, qreal height)

    Sets the coordinates of the rectangle's top-left corner to (\a x,
    \a y), and its size to the given \a width and \a height.

    \sa getRect(), setCoords()
*/


/*!
    \fn void QRectF::setCoords(qreal x1, qreal y1, qreal x2, qreal y2)

    Sets the coordinates of the rectangle's top-left corner to (\a x1,
    \a y1), and the coordinates of its bottom-right corner to (\a x2,
    \a y2).

    \sa getCoords(), setRect()
*/

/*!
    \fn QRectF QRectF::adjusted(qreal dx1, qreal dy1, qreal dx2, qreal dy2) const

    Returns a new rectangle with \a dx1, \a dy1, \a dx2 and \a dy2
    added respectively to the existing coordinates of this rectangle.

    \sa adjust()
*/

/*! \fn void QRectF::adjust(qreal dx1, qreal dy1, qreal dx2, qreal dy2)

    Adds \a dx1, \a dy1, \a dx2 and \a dy2 respectively to the
    existing coordinates of the rectangle.

    \sa adjusted(), setRect()
*/
/*!
    \fn QSizeF QRectF::size() const

    Returns the size of the rectangle.

    \sa setSize(), width(), height()
*/

/*!
    \fn qreal QRectF::width() const

    Returns the width of the rectangle.

    \sa setWidth(), height(), size()
*/

/*!
    \fn qreal QRectF::height() const

    Returns the height of the rectangle.

    \sa setHeight(), width(), size()
*/

/*!
    \fn void QRectF::setWidth(qreal width)

    Sets the width of the rectangle to the given \a width. The right
    edge is changed, but not the left one.

    \sa width(), setSize()
*/


/*!
    \fn void QRectF::setHeight(qreal height)

    Sets the height of the rectangle to the given \a height. The bottom
    edge is changed, but not the top one.

    \sa height(), setSize()
*/


/*!
    \fn void QRectF::setSize(const QSizeF &size)

    Sets the size of the rectangle to the given \a size. The top-left
    corner is not moved.

    \sa size(), setWidth(), setHeight()
*/


/*!
    \fn bool QRectF::contains(const QPointF &point) const

    Returns \c true if the given \a point is inside or on the edge of the
    rectangle; otherwise returns \c false.

    \sa intersects()
*/

bool QRectF::contains(const QPointF &p) const
{
    qreal l = xp;
    qreal r = xp;
    if (w < 0)
        l += w;
    else
        r += w;
    if (l == r) // null rect
        return false;

    if (p.x() < l || p.x() > r)
        return false;

    qreal t = yp;
    qreal b = yp;
    if (h < 0)
        t += h;
    else
        b += h;
    if (t == b) // null rect
        return false;

    if (p.y() < t || p.y() > b)
        return false;

    return true;
}


/*!
    \fn bool QRectF::contains(qreal x, qreal y) const
    \overload

    Returns \c true if the point (\a x, \a y) is inside or on the edge of
    the rectangle; otherwise returns \c false.
*/

/*!
    \fn bool QRectF::contains(const QRectF &rectangle) const
    \overload

    Returns \c true if the given \a rectangle is inside this rectangle;
    otherwise returns \c false.
*/

bool QRectF::contains(const QRectF &r) const
{
    qreal l1 = xp;
    qreal r1 = xp;
    if (w < 0)
        l1 += w;
    else
        r1 += w;
    if (l1 == r1) // null rect
        return false;

    qreal l2 = r.xp;
    qreal r2 = r.xp;
    if (r.w < 0)
        l2 += r.w;
    else
        r2 += r.w;
    if (l2 == r2) // null rect
        return false;

    if (l2 < l1 || r2 > r1)
        return false;

    qreal t1 = yp;
    qreal b1 = yp;
    if (h < 0)
        t1 += h;
    else
        b1 += h;
    if (t1 == b1) // null rect
        return false;

    qreal t2 = r.yp;
    qreal b2 = r.yp;
    if (r.h < 0)
        t2 += r.h;
    else
        b2 += r.h;
    if (t2 == b2) // null rect
        return false;

    if (t2 < t1 || b2 > b1)
        return false;

    return true;
}

/*!
    \fn qreal QRectF::left() const

    Returns the x-coordinate of the rectangle's left edge. Equivalent
    to x().

    \sa setLeft(), topLeft(), bottomLeft()
*/

/*!
    \fn qreal QRectF::top() const

    Returns the y-coordinate of the rectangle's top edge. Equivalent
    to y().

    \sa setTop(), topLeft(), topRight()
*/

/*!
    \fn qreal QRectF::right() const

    Returns the x-coordinate of the rectangle's right edge.

    \sa setRight(), topRight(), bottomRight()
*/

/*!
    \fn qreal QRectF::bottom() const

    Returns the y-coordinate of the rectangle's bottom edge.

    \sa setBottom(), bottomLeft(), bottomRight()
*/

/*!
    \fn QPointF QRectF::topLeft() const

    Returns the position of the rectangle's top-left corner.

    \sa setTopLeft(), top(), left()
*/

/*!
    \fn QPointF QRectF::bottomRight() const

    Returns the position of the rectangle's  bottom-right corner.

    \sa setBottomRight(), bottom(), right()
*/

/*!
    \fn QPointF QRectF::topRight() const

    Returns the position of the rectangle's top-right corner.

    \sa setTopRight(), top(), right()
*/

/*!
    \fn QPointF QRectF::bottomLeft() const

    Returns the position of the rectangle's  bottom-left corner.

    \sa setBottomLeft(), bottom(), left()
*/

/*!
    \fn QRectF& QRectF::operator|=(const QRectF &rectangle)

    Unites this rectangle with the given \a rectangle.

    \sa united(), operator|()
*/

/*!
    \fn QRectF& QRectF::operator&=(const QRectF &rectangle)

    Intersects this rectangle with the given \a rectangle.

    \sa intersected(), operator|=()
*/


/*!
    \fn QRectF QRectF::operator|(const QRectF &rectangle) const

    Returns the bounding rectangle of this rectangle and the given \a rectangle.

    \sa united(), operator|=()
*/

QRectF QRectF::operator|(const QRectF &r) const
{
    if (isNull())
        return r;
    if (r.isNull())
        return *this;

    qreal left = xp;
    qreal right = xp;
    if (w < 0)
        left += w;
    else
        right += w;

    if (r.w < 0) {
        left = qMin(left, r.xp + r.w);
        right = qMax(right, r.xp);
    } else {
        left = qMin(left, r.xp);
        right = qMax(right, r.xp + r.w);
    }

    qreal top = yp;
    qreal bottom = yp;
    if (h < 0)
        top += h;
    else
        bottom += h;

    if (r.h < 0) {
        top = qMin(top, r.yp + r.h);
        bottom = qMax(bottom, r.yp);
    } else {
        top = qMin(top, r.yp);
        bottom = qMax(bottom, r.yp + r.h);
    }

    return QRectF(left, top, right - left, bottom - top);
}

/*!
    \fn QRectF QRectF::unite(const QRectF &rectangle) const
    \obsolete

    Use united(\a rectangle) instead.
*/

/*!
    \fn QRectF QRectF::united(const QRectF &rectangle) const
    \since 4.2

    Returns the bounding rectangle of this rectangle and the given \a
    rectangle.

    \image qrect-unite.png

    \sa intersected()
*/


/*!
    \fn QRectF QRectF::operator &(const QRectF &rectangle) const

    Returns the intersection of this rectangle and the given \a
    rectangle. Returns an empty rectangle if there is no intersection.

    \sa operator&=(), intersected()
*/

QRectF QRectF::operator&(const QRectF &r) const
{
    qreal l1 = xp;
    qreal r1 = xp;
    if (w < 0)
        l1 += w;
    else
        r1 += w;
    if (l1 == r1) // null rect
        return QRectF();

    qreal l2 = r.xp;
    qreal r2 = r.xp;
    if (r.w < 0)
        l2 += r.w;
    else
        r2 += r.w;
    if (l2 == r2) // null rect
        return QRectF();

    if (l1 >= r2 || l2 >= r1)
        return QRectF();

    qreal t1 = yp;
    qreal b1 = yp;
    if (h < 0)
        t1 += h;
    else
        b1 += h;
    if (t1 == b1) // null rect
        return QRectF();

    qreal t2 = r.yp;
    qreal b2 = r.yp;
    if (r.h < 0)
        t2 += r.h;
    else
        b2 += r.h;
    if (t2 == b2) // null rect
        return QRectF();

    if (t1 >= b2 || t2 >= b1)
        return QRectF();

    QRectF tmp;
    tmp.xp = qMax(l1, l2);
    tmp.yp = qMax(t1, t2);
    tmp.w = qMin(r1, r2) - tmp.xp;
    tmp.h = qMin(b1, b2) - tmp.yp;
    return tmp;
}

/*!
    \fn QRectF QRectF::intersect(const QRectF &rectangle) const
    \obsolete

    Use intersected(\a rectangle) instead.
*/

/*!
    \fn QRectF QRectF::intersected(const QRectF &rectangle) const
    \since 4.2

    Returns the intersection of this rectangle and the given \a
    rectangle. Note that \c {r.intersected(s)} is equivalent to \c
    {r & s}.

    \image qrect-intersect.png

    \sa intersects(), united(), operator&=()
*/

/*!
    \fn bool QRectF::intersects(const QRectF &rectangle) const

    Returns \c true if this rectangle intersects with the given \a
    rectangle (i.e. there is a non-empty area of overlap between
    them), otherwise returns \c false.

    The intersection rectangle can be retrieved using the intersected()
    function.

    \sa contains()
*/

bool QRectF::intersects(const QRectF &r) const
{
    qreal l1 = xp;
    qreal r1 = xp;
    if (w < 0)
        l1 += w;
    else
        r1 += w;
    if (l1 == r1) // null rect
        return false;

    qreal l2 = r.xp;
    qreal r2 = r.xp;
    if (r.w < 0)
        l2 += r.w;
    else
        r2 += r.w;
    if (l2 == r2) // null rect
        return false;

    if (l1 >= r2 || l2 >= r1)
        return false;

    qreal t1 = yp;
    qreal b1 = yp;
    if (h < 0)
        t1 += h;
    else
        b1 += h;
    if (t1 == b1) // null rect
        return false;

    qreal t2 = r.yp;
    qreal b2 = r.yp;
    if (r.h < 0)
        t2 += r.h;
    else
        b2 += r.h;
    if (t2 == b2) // null rect
        return false;

    if (t1 >= b2 || t2 >= b1)
        return false;

    return true;
}

/*!
    \fn QRect QRectF::toRect() const

    Returns a QRect based on the values of this rectangle.  Note that the
    coordinates in the returned rectangle are rounded to the nearest integer.

    \sa QRectF(), toAlignedRect()
*/

/*!
    \fn QRect QRectF::toAlignedRect() const
    \since 4.3

    Returns a QRect based on the values of this rectangle that is the
    smallest possible integer rectangle that completely contains this
    rectangle.

    \sa toRect()
*/

QRect QRectF::toAlignedRect() const
{
    int xmin = int(qFloor(xp));
    int xmax = int(qCeil(xp + w));
    int ymin = int(qFloor(yp));
    int ymax = int(qCeil(yp + h));
    return QRect(xmin, ymin, xmax - xmin, ymax - ymin);
}

/*!
    \fn void QRectF::moveCenter(const QPointF &position)

    Moves the rectangle, leaving the center point at the given \a
    position. The rectangle's size is unchanged.

    \sa center()
*/

/*!
    \fn bool operator==(const QRectF &r1, const QRectF &r2)
    \relates QRectF

    Returns \c true if the rectangles \a r1 and \a r2 are equal,
    otherwise returns \c false.
*/


/*!
    \fn bool operator!=(const QRectF &r1, const QRectF &r2)
    \relates QRectF

    Returns \c true if the rectangles \a r1 and \a r2 are different, otherwise
    returns \c false.
*/

/*!
    \fn QRectF operator+(const QRectF &lhs, const QMarginsF &rhs)
    \relates QRectF
    \since 5.3

    Returns the \a lhs rectangle grown by the \a rhs margins.
*/

/*!
    \fn QRectF operator-(const QRectF &lhs, const QMarginsF &rhs)
    \relates QRectF
    \since 5.3

    Returns the \a lhs rectangle grown by the \a rhs margins.
*/

/*!
    \fn QRectF operator+(const QMarginsF &lhs, const QRectF &rhs)
    \relates QRectF
    \overload
    \since 5.3

    Returns the \a lhs rectangle grown by the \a rhs margins.
*/

/*!
    \fn QRectF QRectF::marginsAdded(const QMarginsF &margins) const
    \relates QRectF
    \since 5.3

    Returns a rectangle grown by the \a margins.

    \sa operator+=(), marginsRemoved(), operator-=()
*/

/*!
    \fn QRectF QRectF::marginsRemoved(const QMarginsF &margins) const
    \relates QRectF
    \since 5.3

    Removes the \a margins from the rectangle, shrinking it.

    \sa marginsAdded(), operator+=(), operator-=()
*/

/*!
    \fn QRectF QRectF::operator+=(const QMarginsF &margins)
    \relates QRectF
    \since 5.3

    Adds the \a margins to the rectangle, growing it.

    \sa marginsAdded(), marginsRemoved(), operator-=()
*/

/*!
    \fn QRectF QRectF::operator-=(const QMarginsF &margins)
    \relates QRectF
    \since 5.3

    Returns a rectangle shrunk by the \a margins.

    \sa marginsRemoved(), operator+=(), marginsAdded()
*/

/*****************************************************************************
  QRectF stream functions
 *****************************************************************************/
#ifndef QT_NO_DATASTREAM
/*!
    \fn QDataStream &operator<<(QDataStream &stream, const QRectF &rectangle)

    \relates QRectF

    Writes the \a rectangle to the \a stream, and returns a reference to the
    stream.

    \sa {Serializing Qt Data Types}
*/

QDataStream &operator<<(QDataStream &s, const QRectF &r)
{
    s << double(r.x()) << double(r.y()) << double(r.width()) << double(r.height());
    return s;
}

/*!
    \fn QDataStream &operator>>(QDataStream &stream, QRectF &rectangle)

    \relates QRectF

    Reads a \a rectangle from the \a stream, and returns a reference to the
    stream.

    \sa {Serializing Qt Data Types}
*/

QDataStream &operator>>(QDataStream &s, QRectF &r)
{
    double x, y, w, h;
    s >> x;
    s >> y;
    s >> w;
    s >> h;
    r.setRect(qreal(x), qreal(y), qreal(w), qreal(h));
    return s;
}

#endif // QT_NO_DATASTREAM


#ifndef QT_NO_DEBUG_STREAM
QDebug operator<<(QDebug dbg, const QRectF &r) {
    dbg.nospace() << "QRectF(" << r.x() << ',' << r.y() << ' '
                  << r.width() << 'x' << r.height() << ')';
    return dbg.space();
}
#endif

QT_END_NAMESPACE
