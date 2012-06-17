/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtGui module of the Qt Toolkit.
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

#include "qdatastream.h"
#include "qdebug.h"
#include "qmatrix.h"
#include "qregion.h"
#include "qpainterpath.h"
#include "qvariant.h"
#include <qmath.h>

#include <limits.h>

QT_BEGIN_NAMESPACE

/*!
    \class QMatrix
    \brief The QMatrix class specifies 2D transformations of a
    coordinate system.
    \obsolete

    \ingroup painting

    A matrix specifies how to translate, scale, shear or rotate the
    coordinate system, and is typically used when rendering graphics.
    QMatrix, in contrast to QTransform, does not allow perspective
    transformations. QTransform is the recommended transformation
    class in Qt.

    A QMatrix object can be built using the setMatrix(), scale(),
    rotate(), translate() and shear() functions.  Alternatively, it
    can be built by applying \l {QMatrix#Basic Matrix
    Operations}{basic matrix operations}. The matrix can also be
    defined when constructed, and it can be reset to the identity
    matrix (the default) using the reset() function.

    The QMatrix class supports mapping of graphic primitives: A given
    point, line, polygon, region, or painter path can be mapped to the
    coordinate system defined by \e this matrix using the map()
    function. In case of a rectangle, its coordinates can be
    transformed using the mapRect() function. A rectangle can also be
    transformed into a \e polygon (mapped to the coordinate system
    defined by \e this matrix), using the mapToPolygon() function.

    QMatrix provides the isIdentity() function which returns true if
    the matrix is the identity matrix, and the isInvertible() function
    which returns true if the matrix is non-singular (i.e. AB = BA =
    I). The inverted() function returns an inverted copy of \e this
    matrix if it is invertible (otherwise it returns the identity
    matrix). In addition, QMatrix provides the determinant() function
    returning the matrix's determinant.

    Finally, the QMatrix class supports matrix multiplication, and
    objects of the class can be streamed as well as compared.

    \tableofcontents

    \section1 Rendering Graphics

    When rendering graphics, the matrix defines the transformations
    but the actual transformation is performed by the drawing routines
    in QPainter.

    By default, QPainter operates on the associated device's own
    coordinate system.  The standard coordinate system of a
    QPaintDevice has its origin located at the top-left position. The
    \e x values increase to the right; \e y values increase
    downward. For a complete description, see the \l {Coordinate
    System}{coordinate system} documentation.

    QPainter has functions to translate, scale, shear and rotate the
    coordinate system without using a QMatrix. For example:

    \table 100%
    \row
    \o \inlineimage qmatrix-simpletransformation.png
    \o
    \snippet doc/src/snippets/matrix/matrix.cpp 0
    \endtable

    Although these functions are very convenient, it can be more
    efficient to build a QMatrix and call QPainter::setMatrix() if you
    want to perform more than a single transform operation. For
    example:

    \table 100%
    \row
    \o \inlineimage qmatrix-combinedtransformation.png
    \o
    \snippet doc/src/snippets/matrix/matrix.cpp 1
    \endtable

    \section1 Basic Matrix Operations

    \image qmatrix-representation.png

    A QMatrix object contains a 3 x 3 matrix.  The \c dx and \c dy
    elements specify horizontal and vertical translation. The \c m11
    and \c m22 elements specify horizontal and vertical scaling. And
    finally, the \c m21 and \c m12 elements specify horizontal and
    vertical \e shearing.

    QMatrix transforms a point in the plane to another point using the
    following formulas:

    \snippet doc/src/snippets/code/src_gui_painting_qmatrix.cpp 0

    The point \e (x, y) is the original point, and \e (x', y') is the
    transformed point. \e (x', y') can be transformed back to \e (x,
    y) by performing the same operation on the inverted() matrix.

    The various matrix elements can be set when constructing the
    matrix, or by using the setMatrix() function later on. They can also
    be manipulated using the translate(), rotate(), scale() and
    shear() convenience functions, The currently set values can be
    retrieved using the m11(), m12(), m21(), m22(), dx() and dy()
    functions.

    Translation is the simplest transformation. Setting \c dx and \c
    dy will move the coordinate system \c dx units along the X axis
    and \c dy units along the Y axis.  Scaling can be done by setting
    \c m11 and \c m22. For example, setting \c m11 to 2 and \c m22 to
    1.5 will double the height and increase the width by 50%.  The
    identity matrix has \c m11 and \c m22 set to 1 (all others are set
    to 0) mapping a point to itself. Shearing is controlled by \c m12
    and \c m21. Setting these elements to values different from zero
    will twist the coordinate system. Rotation is achieved by
    carefully setting both the shearing factors and the scaling
    factors.

    Here's the combined transformations example using basic matrix
    operations:

    \table 100%
    \row
    \o \inlineimage qmatrix-combinedtransformation.png
    \o
    \snippet doc/src/snippets/matrix/matrix.cpp 2
    \endtable

    \sa QPainter, QTransform, {Coordinate System}, 
    {demos/affine}{Affine Transformations Demo}, {Transformations Example}
*/


// some defines to inline some code
#define MAPDOUBLE(x, y, nx, ny) \
{ \
    qreal fx = x; \
    qreal fy = y; \
    nx = _m11*fx + _m21*fy + _dx; \
    ny = _m12*fx + _m22*fy + _dy; \
}

#define MAPINT(x, y, nx, ny) \
{ \
    qreal fx = x; \
    qreal fy = y; \
    nx = qRound(_m11*fx + _m21*fy + _dx); \
    ny = qRound(_m12*fx + _m22*fy + _dy); \
}

/*****************************************************************************
  QMatrix member functions
 *****************************************************************************/
/*!
    \fn QMatrix::QMatrix(Qt::Initialization)
    \internal
*/

/*!
    Constructs an identity matrix.

    All elements are set to zero except \c m11 and \c m22 (specifying
    the scale), which are set to 1.

    \sa reset()
*/

QMatrix::QMatrix()
    : _m11(1.)
    , _m12(0.)
    , _m21(0.)
    , _m22(1.)
    , _dx(0.)
    , _dy(0.)
{
}

/*!
    Constructs a matrix with the elements, \a m11, \a m12, \a m21, \a
    m22, \a dx and \a dy.

    \sa setMatrix()
*/

QMatrix::QMatrix(qreal m11, qreal m12, qreal m21, qreal m22, qreal dx, qreal dy)
    : _m11(m11)
    , _m12(m12)
    , _m21(m21)
    , _m22(m22)
    , _dx(dx)
    , _dy(dy)
{
}


/*!
     Constructs a matrix that is a copy of the given \a matrix.
 */
QMatrix::QMatrix(const QMatrix &matrix)
    : _m11(matrix._m11)
    , _m12(matrix._m12)
    , _m21(matrix._m21)
    , _m22(matrix._m22)
    , _dx(matrix._dx)
    , _dy(matrix._dy)
{
}

/*!
    Sets the matrix elements to the specified values, \a m11, \a m12,
    \a m21, \a m22, \a dx and \a dy.

    Note that this function replaces the previous values. QMatrix
    provide the translate(), rotate(), scale() and shear() convenience
    functions to manipulate the various matrix elements based on the
    currently defined coordinate system.

    \sa QMatrix()
*/

void QMatrix::setMatrix(qreal m11, qreal m12, qreal m21, qreal m22, qreal dx, qreal dy)
{
    _m11 = m11;
    _m12 = m12;
    _m21 = m21;
    _m22 = m22;
    _dx  = dx;
    _dy  = dy;
}


/*!
    \fn qreal QMatrix::m11() const

    Returns the horizontal scaling factor.

    \sa scale(), {QMatrix#Basic Matrix Operations}{Basic Matrix
    Operations}
*/

/*!
    \fn qreal QMatrix::m12() const

    Returns the vertical shearing factor.

    \sa shear(), {QMatrix#Basic Matrix Operations}{Basic Matrix
    Operations}
*/

/*!
    \fn qreal QMatrix::m21() const

    Returns the horizontal shearing factor.

    \sa shear(), {QMatrix#Basic Matrix Operations}{Basic Matrix
    Operations}
*/

/*!
    \fn qreal QMatrix::m22() const

    Returns the vertical scaling factor.

    \sa scale(), {QMatrix#Basic Matrix Operations}{Basic Matrix
    Operations}
*/

/*!
    \fn qreal QMatrix::dx() const

    Returns the horizontal translation factor.

    \sa translate(), {QMatrix#Basic Matrix Operations}{Basic Matrix
    Operations}
*/

/*!
    \fn qreal QMatrix::dy() const

    Returns the vertical translation factor.

    \sa translate(), {QMatrix#Basic Matrix Operations}{Basic Matrix
    Operations}
*/


/*!
    Maps the given coordinates \a x and \a y into the coordinate
    system defined by this matrix. The resulting values are put in *\a
    tx and *\a ty, respectively.

    The coordinates are transformed using the following formulas:

    \snippet doc/src/snippets/code/src_gui_painting_qmatrix.cpp 1

    The point (x, y) is the original point, and (x', y') is the
    transformed point.

    \sa {QMatrix#Basic Matrix Operations}{Basic Matrix Operations}
*/

void QMatrix::map(qreal x, qreal y, qreal *tx, qreal *ty) const
{
    MAPDOUBLE(x, y, *tx, *ty);
}



/*!
    \overload

    Maps the given coordinates \a x and \a y into the coordinate
    system defined by this matrix. The resulting values are put in *\a
    tx and *\a ty, respectively. Note that the transformed coordinates
    are rounded to the nearest integer.
*/

void QMatrix::map(int x, int y, int *tx, int *ty) const
{
    MAPINT(x, y, *tx, *ty);
}

QRect QMatrix::mapRect(const QRect &rect) const
{
    QRect result;
    if (_m12 == 0.0F && _m21 == 0.0F) {
        int x = qRound(_m11*rect.x() + _dx);
        int y = qRound(_m22*rect.y() + _dy);
        int w = qRound(_m11*rect.width());
        int h = qRound(_m22*rect.height());
        if (w < 0) {
            w = -w;
            x -= w;
        }
        if (h < 0) {
            h = -h;
            y -= h;
        }
        result = QRect(x, y, w, h);
    } else {
        // see mapToPolygon for explanations of the algorithm.
        qreal x0, y0;
        qreal x, y;
        MAPDOUBLE(rect.left(), rect.top(), x0, y0);
        qreal xmin = x0;
        qreal ymin = y0;
        qreal xmax = x0;
        qreal ymax = y0;
        MAPDOUBLE(rect.right() + 1, rect.top(), x, y);
        xmin = qMin(xmin, x);
        ymin = qMin(ymin, y);
        xmax = qMax(xmax, x);
        ymax = qMax(ymax, y);
        MAPDOUBLE(rect.right() + 1, rect.bottom() + 1, x, y);
        xmin = qMin(xmin, x);
        ymin = qMin(ymin, y);
        xmax = qMax(xmax, x);
        ymax = qMax(ymax, y);
        MAPDOUBLE(rect.left(), rect.bottom() + 1, x, y);
        xmin = qMin(xmin, x);
        ymin = qMin(ymin, y);
        xmax = qMax(xmax, x);
        ymax = qMax(ymax, y);
        result = QRect(qRound(xmin), qRound(ymin), qRound(xmax)-qRound(xmin), qRound(ymax)-qRound(ymin));
    }
    return result;
}

/*!
    \fn QRectF QMatrix::mapRect(const QRectF &rectangle) const

    Creates and returns a QRectF object that is a copy of the given \a
    rectangle, mapped into the coordinate system defined by this
    matrix.

    The rectangle's coordinates are transformed using the following
    formulas:

    \snippet doc/src/snippets/code/src_gui_painting_qmatrix.cpp 2

    If rotation or shearing has been specified, this function returns
    the \e bounding rectangle. To retrieve the exact region the given
    \a rectangle maps to, use the mapToPolygon() function instead.

    \sa mapToPolygon(), {QMatrix#Basic Matrix Operations}{Basic Matrix
    Operations}
*/
QRectF QMatrix::mapRect(const QRectF &rect) const
{
    QRectF result;
    if (_m12 == 0.0F && _m21 == 0.0F) {
        qreal x = _m11*rect.x() + _dx;
        qreal y = _m22*rect.y() + _dy;
        qreal w = _m11*rect.width();
        qreal h = _m22*rect.height();
        if (w < 0) {
            w = -w;
            x -= w;
        }
        if (h < 0) {
            h = -h;
            y -= h;
        }
        result = QRectF(x, y, w, h);
    } else {
        qreal x0, y0;
        qreal x, y;
        MAPDOUBLE(rect.x(), rect.y(), x0, y0);
        qreal xmin = x0;
        qreal ymin = y0;
        qreal xmax = x0;
        qreal ymax = y0;
        MAPDOUBLE(rect.x() + rect.width(), rect.y(), x, y);
        xmin = qMin(xmin, x);
        ymin = qMin(ymin, y);
        xmax = qMax(xmax, x);
        ymax = qMax(ymax, y);
        MAPDOUBLE(rect.x() + rect.width(), rect.y() + rect.height(), x, y);
        xmin = qMin(xmin, x);
        ymin = qMin(ymin, y);
        xmax = qMax(xmax, x);
        ymax = qMax(ymax, y);
        MAPDOUBLE(rect.x(), rect.y() + rect.height(), x, y);
        xmin = qMin(xmin, x);
        ymin = qMin(ymin, y);
        xmax = qMax(xmax, x);
        ymax = qMax(ymax, y);
        result = QRectF(xmin, ymin, xmax-xmin, ymax - ymin);
    }
    return result;
}

/*!
    \fn QRect QMatrix::mapRect(const QRect &rectangle) const
    \overload

    Creates and returns a QRect object that is a copy of the given \a
    rectangle, mapped into the coordinate system defined by this
    matrix. Note that the transformed coordinates are rounded to the
    nearest integer.
*/


/*!
    \fn QPoint operator*(const QPoint &point, const QMatrix &matrix)
    \relates QMatrix

    This is the same as \a{matrix}.map(\a{point}).

    \sa QMatrix::map()
*/

QPoint QMatrix::map(const QPoint &p) const
{
    qreal fx = p.x();
    qreal fy = p.y();
    return QPoint(qRound(_m11*fx + _m21*fy + _dx),
                   qRound(_m12*fx + _m22*fy + _dy));
}

/*!
    \fn QPointF operator*(const QPointF &point, const QMatrix &matrix)
    \relates QMatrix

    Same as \a{matrix}.map(\a{point}).

    \sa QMatrix::map()
*/

/*!
    \overload

    Creates and returns a QPointF object that is a copy of the given
    \a point, mapped into the coordinate system defined by this
    matrix.
*/
QPointF QMatrix::map(const QPointF &point) const
{
    qreal fx = point.x();
    qreal fy = point.y();
    return QPointF(_m11*fx + _m21*fy + _dx, _m12*fx + _m22*fy + _dy);
}

/*!
    \fn QPoint QMatrix::map(const QPoint &point) const
    \overload

    Creates and returns a QPoint object that is a copy of the given \a
    point, mapped into the coordinate system defined by this
    matrix. Note that the transformed coordinates are rounded to the
    nearest integer.
*/

/*!
    \fn QLineF operator*(const QLineF &line, const QMatrix &matrix)
    \relates QMatrix

    This is the same as \a{matrix}.map(\a{line}).

    \sa QMatrix::map()
*/

/*!
    \fn QLine operator*(const QLine &line, const QMatrix &matrix)
    \relates QMatrix

    This is the same as \a{matrix}.map(\a{line}).

    \sa QMatrix::map()
*/

/*!
    \overload

    Creates and returns a QLineF object that is a copy of the given \a
    line, mapped into the coordinate system defined by this matrix.
*/
QLineF QMatrix::map(const QLineF &line) const
{
    return QLineF(map(line.p1()), map(line.p2()));
}

/*!
    \overload

    Creates and returns a QLine object that is a copy of the given \a
    line, mapped into the coordinate system defined by this matrix.
    Note that the transformed coordinates are rounded to the nearest
    integer.
*/
QLine QMatrix::map(const QLine &line) const
{
    return QLine(map(line.p1()), map(line.p2()));
}

/*!
    \fn QPolygonF operator *(const QPolygonF &polygon, const QMatrix &matrix)
    \relates QMatrix

    This is the same as \a{matrix}.map(\a{polygon}).

    \sa QMatrix::map()
*/

/*!
    \fn QPolygon operator*(const QPolygon &polygon, const QMatrix &matrix)
    \relates QMatrix

    This is the same as \a{matrix}.map(\a{polygon}).

    \sa QMatrix::map()
*/

QPolygon QMatrix::map(const QPolygon &a) const
{
    int size = a.size();
    int i;
    QPolygon p(size);
    const QPoint *da = a.constData();
    QPoint *dp = p.data();
    for(i = 0; i < size; i++) {
        MAPINT(da[i].x(), da[i].y(), dp[i].rx(), dp[i].ry());
    }
    return p;
}

/*!
    \fn QPolygonF QMatrix::map(const QPolygonF &polygon) const
    \overload

    Creates and returns a QPolygonF object that is a copy of the given
    \a polygon, mapped into the coordinate system defined by this
    matrix.
*/
QPolygonF QMatrix::map(const QPolygonF &a) const
{
    int size = a.size();
    int i;
    QPolygonF p(size);
    const QPointF *da = a.constData();
    QPointF *dp = p.data();
    for(i = 0; i < size; i++) {
        MAPDOUBLE(da[i].xp, da[i].yp, dp[i].xp, dp[i].yp);
    }
    return p;
}

/*!
    \fn QPolygon QMatrix::map(const QPolygon &polygon) const
    \overload

    Creates and returns a QPolygon object that is a copy of the given
    \a polygon, mapped into the coordinate system defined by this
    matrix. Note that the transformed coordinates are rounded to the
    nearest integer.
*/

/*!
    \fn QRegion operator*(const QRegion &region, const QMatrix &matrix)
    \relates QMatrix

    This is the same as \a{matrix}.map(\a{region}).

    \sa QMatrix::map()
*/

extern Q_AUTOTEST_EXPORT QPainterPath qt_regionToPath(const QRegion &region);

/*!
    \fn QRegion QMatrix::map(const QRegion &region) const
    \overload

    Creates and returns a QRegion object that is a copy of the given
    \a region, mapped into the coordinate system defined by this matrix.

    Calling this method can be rather expensive if rotations or
    shearing are used.
*/
QRegion QMatrix::map(const QRegion &r) const
{
    if (_m11 == 1.0 && _m22 == 1.0 && _m12 == 0.0 && _m21 == 0.0) { // translate or identity
        if (_dx == 0.0 && _dy == 0.0) // Identity
            return r;
        QRegion copy(r);
        copy.translate(qRound(_dx), qRound(_dy));
        return copy;
    }

    QPainterPath p = map(qt_regionToPath(r));
    return p.toFillPolygon().toPolygon();
}

/*!
    \fn QPainterPath operator *(const QPainterPath &path, const QMatrix &matrix)
    \relates QMatrix

    This is the same as \a{matrix}.map(\a{path}).

    \sa QMatrix::map()
*/

/*!
    \overload

    Creates and returns a QPainterPath object that is a copy of the
    given \a path, mapped into the coordinate system defined by this
    matrix.
*/
QPainterPath QMatrix::map(const QPainterPath &path) const
{
    if (path.isEmpty())
        return QPainterPath();

    QPainterPath copy = path;

    // Translate or identity
    if (_m11 == 1.0 && _m22 == 1.0 && _m12 == 0.0 && _m21 == 0.0) {

        // Translate
        if (_dx != 0.0 || _dy != 0.0) {
            copy.detach();
            for (int i=0; i<path.elementCount(); ++i) {
                QPainterPath::Element &e = copy.d_ptr->elements[i];
                e.x += _dx;
                e.y += _dy;
            }
        }

    // Full xform
    } else {
        copy.detach();
        for (int i=0; i<path.elementCount(); ++i) {
            QPainterPath::Element &e = copy.d_ptr->elements[i];
            qreal fx = e.x, fy = e.y;
            e.x = _m11*fx + _m21*fy + _dx;
            e.y =  _m12*fx + _m22*fy + _dy;
        }
    }

    return copy;
}

/*!
    \fn QRegion QMatrix::mapToRegion(const QRect &rectangle) const

    Returns the transformed rectangle \a rectangle as a QRegion
    object. A rectangle which has been rotated or sheared may result
    in a non-rectangular region being returned.

    Use the mapToPolygon() or map() function instead.
*/
#ifdef QT3_SUPPORT
QRegion QMatrix::mapToRegion(const QRect &rect) const
{
    QRegion result;
    if (isIdentity()) {
        result = rect;
    } else if (m12() == 0.0F && m21() == 0.0F) {
        int x = qRound(m11()*rect.x() + dx());
        int y = qRound(m22()*rect.y() + dy());
        int w = qRound(m11()*rect.width());
        int h = qRound(m22()*rect.height());
        if (w < 0) {
            w = -w;
            x -= w - 1;
        }
        if (h < 0) {
            h = -h;
            y -= h - 1;
        }
        result = QRect(x, y, w, h);
    } else {
        result = QRegion(mapToPolygon(rect));
    }
    return result;

}
#endif
/*!
    \fn QPolygon QMatrix::mapToPolygon(const QRect &rectangle) const

    Creates and returns a QPolygon representation of the given \a
    rectangle, mapped into the coordinate system defined by this
    matrix.

    The rectangle's coordinates are transformed using the following
    formulas:

    \snippet doc/src/snippets/code/src_gui_painting_qmatrix.cpp 3

    Polygons and rectangles behave slightly differently when
    transformed (due to integer rounding), so
    \c{matrix.map(QPolygon(rectangle))} is not always the same as
    \c{matrix.mapToPolygon(rectangle)}.

    \sa mapRect(), {QMatrix#Basic Matrix Operations}{Basic Matrix
    Operations}
*/
QPolygon QMatrix::mapToPolygon(const QRect &rect) const
{
    QPolygon a(4);
    qreal x[4], y[4];
    if (_m12 == 0.0F && _m21 == 0.0F) {
        x[0] = _m11*rect.x() + _dx;
        y[0] = _m22*rect.y() + _dy;
        qreal w = _m11*rect.width();
        qreal h = _m22*rect.height();
        if (w < 0) {
            w = -w;
            x[0] -= w;
        }
        if (h < 0) {
            h = -h;
            y[0] -= h;
        }
        x[1] = x[0]+w;
        x[2] = x[1];
        x[3] = x[0];
        y[1] = y[0];
        y[2] = y[0]+h;
        y[3] = y[2];
    } else {
        qreal right = rect.x() + rect.width();
        qreal bottom = rect.y() + rect.height();
        MAPDOUBLE(rect.x(), rect.y(), x[0], y[0]);
        MAPDOUBLE(right, rect.y(), x[1], y[1]);
        MAPDOUBLE(right, bottom, x[2], y[2]);
        MAPDOUBLE(rect.x(), bottom, x[3], y[3]);
    }
#if 0
    int i;
    for(i = 0; i< 4; i++)
        qDebug("coords(%d) = (%f/%f) (%d/%d)", i, x[i], y[i], qRound(x[i]), qRound(y[i]));
    qDebug("width=%f, height=%f", qSqrt((x[1]-x[0])*(x[1]-x[0]) + (y[1]-y[0])*(y[1]-y[0])),
            qSqrt((x[0]-x[3])*(x[0]-x[3]) + (y[0]-y[3])*(y[0]-y[3])));
#endif
    // all coordinates are correctly, tranform to a pointarray
    // (rounding to the next integer)
    a.setPoints(4, qRound(x[0]), qRound(y[0]),
                 qRound(x[1]), qRound(y[1]),
                 qRound(x[2]), qRound(y[2]),
                 qRound(x[3]), qRound(y[3]));
    return a;
}

/*!
    Resets the matrix to an identity matrix, i.e. all elements are set
    to zero, except \c m11 and \c m22 (specifying the scale) which are
    set to 1.

    \sa QMatrix(), isIdentity(), {QMatrix#Basic Matrix
    Operations}{Basic Matrix Operations}
*/

void QMatrix::reset()
{
    _m11 = _m22 = 1.0;
    _m12 = _m21 = _dx = _dy = 0.0;
}

/*!
    \fn bool QMatrix::isIdentity() const

    Returns true if the matrix is the identity matrix, otherwise
    returns false.

    \sa reset()
*/

/*!
    Moves the coordinate system \a dx along the x axis and \a dy along
    the y axis, and returns a reference to the matrix.

    \sa setMatrix()
*/

QMatrix &QMatrix::translate(qreal dx, qreal dy)
{
    _dx += dx*_m11 + dy*_m21;
    _dy += dy*_m22 + dx*_m12;
    return *this;
}

/*!
    \fn QMatrix &QMatrix::scale(qreal sx, qreal sy)

    Scales the coordinate system by \a sx horizontally and \a sy
    vertically, and returns a reference to the matrix.

    \sa setMatrix()
*/

QMatrix &QMatrix::scale(qreal sx, qreal sy)
{
    _m11 *= sx;
    _m12 *= sx;
    _m21 *= sy;
    _m22 *= sy;
    return *this;
}

/*!
    Shears the coordinate system by \a sh horizontally and \a sv
    vertically, and returns a reference to the matrix.

    \sa setMatrix()
*/

QMatrix &QMatrix::shear(qreal sh, qreal sv)
{
    qreal tm11 = sv*_m21;
    qreal tm12 = sv*_m22;
    qreal tm21 = sh*_m11;
    qreal tm22 = sh*_m12;
    _m11 += tm11;
    _m12 += tm12;
    _m21 += tm21;
    _m22 += tm22;
    return *this;
}

const qreal deg2rad = qreal(0.017453292519943295769);        // pi/180

/*!
    \fn QMatrix &QMatrix::rotate(qreal degrees)

    Rotates the coordinate system the given \a degrees
    counterclockwise.

    Note that if you apply a QMatrix to a point defined in widget
    coordinates, the direction of the rotation will be clockwise
    because the y-axis points downwards.

    Returns a reference to the matrix.

    \sa setMatrix()
*/

QMatrix &QMatrix::rotate(qreal a)
{
    qreal sina = 0;
    qreal cosa = 0;
    if (a == 90. || a == -270.)
        sina = 1.;
    else if (a == 270. || a == -90.)
        sina = -1.;
    else if (a == 180.)
        cosa = -1.;
    else{
        qreal b = deg2rad*a;                        // convert to radians
        sina = qSin(b);               // fast and convenient
        cosa = qCos(b);
    }
    qreal tm11 = cosa*_m11 + sina*_m21;
    qreal tm12 = cosa*_m12 + sina*_m22;
    qreal tm21 = -sina*_m11 + cosa*_m21;
    qreal tm22 = -sina*_m12 + cosa*_m22;
    _m11 = tm11; _m12 = tm12;
    _m21 = tm21; _m22 = tm22;
    return *this;
}

/*!
    \fn bool QMatrix::isInvertible() const

    Returns true if the matrix is invertible, otherwise returns false.

    \sa inverted()
*/

/*!
    \obsolete
    \fn qreal QMatrix::det() const

    Returns the matrix's determinant.

    \sa determinant()
*/

/*!
    \since 4.6
    \fn qreal QMatrix::determinant() const

    Returns the matrix's determinant.
*/

/*!
    \fn QMatrix QMatrix::invert(bool *invertible) const

    Returns an inverted copy of this matrix.

    Use the inverted() function instead.
*/

/*!
    Returns an inverted copy of this matrix.

    If the matrix is singular (not invertible), the returned matrix is
    the identity matrix. If \a invertible is valid (i.e. not 0), its
    value is set to true if the matrix is invertible, otherwise it is
    set to false.

    \sa isInvertible()
*/

QMatrix QMatrix::inverted(bool *invertible) const
{
    qreal dtr = determinant();
    if (dtr == 0.0) {
        if (invertible)
            *invertible = false;                // singular matrix
        return QMatrix(true);
    }
    else {                                        // invertible matrix
        if (invertible)
            *invertible = true;
        qreal dinv = 1.0/dtr;
        return QMatrix((_m22*dinv),        (-_m12*dinv),
                       (-_m21*dinv), (_m11*dinv),
                       ((_m21*_dy - _m22*_dx)*dinv),
                       ((_m12*_dx - _m11*_dy)*dinv),
                       true);
    }
}


/*!
    \fn bool QMatrix::operator==(const QMatrix &matrix) const

    Returns true if this matrix is equal to the given \a matrix,
    otherwise returns false.
*/

bool QMatrix::operator==(const QMatrix &m) const
{
    return _m11 == m._m11 &&
           _m12 == m._m12 &&
           _m21 == m._m21 &&
           _m22 == m._m22 &&
           _dx == m._dx &&
           _dy == m._dy;
}

/*!
    \fn bool QMatrix::operator!=(const QMatrix &matrix) const

    Returns true if this matrix is not equal to the given \a matrix,
    otherwise returns false.
*/

bool QMatrix::operator!=(const QMatrix &m) const
{
    return _m11 != m._m11 ||
           _m12 != m._m12 ||
           _m21 != m._m21 ||
           _m22 != m._m22 ||
           _dx != m._dx ||
           _dy != m._dy;
}

/*!
    \fn QMatrix &QMatrix::operator *=(const QMatrix &matrix)
    \overload

    Returns the result of multiplying this matrix by the given \a
    matrix.
*/

QMatrix &QMatrix::operator *=(const QMatrix &m)
{
    qreal tm11 = _m11*m._m11 + _m12*m._m21;
    qreal tm12 = _m11*m._m12 + _m12*m._m22;
    qreal tm21 = _m21*m._m11 + _m22*m._m21;
    qreal tm22 = _m21*m._m12 + _m22*m._m22;

    qreal tdx  = _dx*m._m11  + _dy*m._m21 + m._dx;
    qreal tdy =  _dx*m._m12  + _dy*m._m22 + m._dy;

    _m11 = tm11; _m12 = tm12;
    _m21 = tm21; _m22 = tm22;
    _dx = tdx; _dy = tdy;
    return *this;
}

/*!
    \fn QMatrix QMatrix::operator *(const QMatrix &matrix) const

    Returns the result of multiplying this matrix by the given \a
    matrix.

    Note that matrix multiplication is not commutative, i.e. a*b !=
    b*a.
*/

QMatrix QMatrix::operator *(const QMatrix &m) const
{
    qreal tm11 = _m11*m._m11 + _m12*m._m21;
    qreal tm12 = _m11*m._m12 + _m12*m._m22;
    qreal tm21 = _m21*m._m11 + _m22*m._m21;
    qreal tm22 = _m21*m._m12 + _m22*m._m22;

    qreal tdx  = _dx*m._m11  + _dy*m._m21 + m._dx;
    qreal tdy =  _dx*m._m12  + _dy*m._m22 + m._dy;
    return QMatrix(tm11, tm12, tm21, tm22, tdx, tdy, true);
}

/*!
    Assigns the given \a matrix's values to this matrix.
*/
QMatrix &QMatrix::operator=(const QMatrix &matrix)
{
    _m11 = matrix._m11;
    _m12 = matrix._m12;
    _m21 = matrix._m21;
    _m22 = matrix._m22;
    _dx  = matrix._dx;
    _dy  = matrix._dy;
    return *this;
}

/*!
    \since 4.2

    Returns the matrix as a QVariant.
*/
QMatrix::operator QVariant() const
{
    return QVariant(QVariant::Matrix, this);
}

Q_GUI_EXPORT QPainterPath operator *(const QPainterPath &p, const QMatrix &m)
{
    return m.map(p);
}


/*****************************************************************************
  QMatrix stream functions
 *****************************************************************************/
#ifndef QT_NO_DATASTREAM
/*!
    \fn QDataStream &operator<<(QDataStream &stream, const QMatrix &matrix)
    \relates QMatrix

    Writes the given \a matrix to the given \a stream and returns a
    reference to the stream.

    \sa {Serializing Qt Data Types}
*/

QDataStream &operator<<(QDataStream &s, const QMatrix &m)
{
    if (s.version() == 1) {
        s << (float)m.m11() << (float)m.m12() << (float)m.m21()
          << (float)m.m22() << (float)m.dx()  << (float)m.dy();
    } else {
        s << double(m.m11())
          << double(m.m12())
          << double(m.m21())
          << double(m.m22())
          << double(m.dx())
          << double(m.dy());
    }
    return s;
}

/*!
    \fn QDataStream &operator>>(QDataStream &stream, QMatrix &matrix)
    \relates QMatrix

    Reads the given \a matrix from the given \a stream and returns a
    reference to the stream.

    \sa {Serializing Qt Data Types}
*/

QDataStream &operator>>(QDataStream &s, QMatrix &m)
{
    if (s.version() == 1) {
        float m11, m12, m21, m22, dx, dy;
        s >> m11;  s >> m12;  s >> m21;  s >> m22;
        s >> dx;   s >> dy;
        m.setMatrix(m11, m12, m21, m22, dx, dy);
    }
    else {
        double m11, m12, m21, m22, dx, dy;
        s >> m11;
        s >> m12;
        s >> m21;
        s >> m22;
        s >> dx;
        s >> dy;
        m.setMatrix(m11, m12, m21, m22, dx, dy);
    }
    return s;
}
#endif // QT_NO_DATASTREAM

#ifndef QT_NO_DEBUG_STREAM
QDebug operator<<(QDebug dbg, const QMatrix &m)
{
    dbg.nospace() << "QMatrix("
                  << "11=" << m.m11()
                  << " 12=" << m.m12()
                  << " 21=" << m.m21()
                  << " 22=" << m.m22()
                  << " dx=" << m.dx()
                  << " dy=" << m.dy()
                  << ')';
    return dbg.space();
}
#endif

/*!
    \fn QRect QMatrix::map(const QRect &rect) const
    \compat

    Creates and returns a QRect object that is a copy of the given
    rectangle, mapped into the coordinate system defined by this
    matrix.

    Use the mapRect() function instead.
*/


/*!
    \fn bool qFuzzyCompare(const QMatrix& m1, const QMatrix& m2)

    \relates QMatrix
    \since 4.6

    \brief The qFuzzyCompare function is for comparing two matrices
    using a fuzziness factor.
    
    Returns true if \a m1 and \a m2 are equal, allowing for a small
    fuzziness factor for floating-point comparisons; false otherwise.
*/

QT_END_NAMESPACE
