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
#include "qtransform.h"

#include "qdatastream.h"
#include "qdebug.h"
#include "qmatrix.h"
#include "qregion.h"
#include "qpainterpath.h"
#include "qvariant.h"
#include <qmath.h>
#include <qnumeric.h>

#include <private/qbezier_p.h>

QT_BEGIN_NAMESPACE

#define Q_NEAR_CLIP (sizeof(qreal) == sizeof(double) ? 0.000001 : 0.0001)

#ifdef MAP
#  undef MAP
#endif
#define MAP(x, y, nx, ny) \
    do { \
        qreal FX_ = x; \
        qreal FY_ = y; \
        switch(t) {   \
        case TxNone:  \
            nx = FX_;   \
            ny = FY_;   \
            break;    \
        case TxTranslate:    \
            nx = FX_ + affine._dx;                \
            ny = FY_ + affine._dy;                \
            break;                              \
        case TxScale:                           \
            nx = affine._m11 * FX_ + affine._dx;  \
            ny = affine._m22 * FY_ + affine._dy;  \
            break;                              \
        case TxRotate:                          \
        case TxShear:                           \
        case TxProject:                                      \
            nx = affine._m11 * FX_ + affine._m21 * FY_ + affine._dx;        \
            ny = affine._m12 * FX_ + affine._m22 * FY_ + affine._dy;        \
            if (t == TxProject) {                                       \
                qreal w = (m_13 * FX_ + m_23 * FY_ + m_33);              \
                if (w < qreal(Q_NEAR_CLIP)) w = qreal(Q_NEAR_CLIP);     \
                w = 1./w;                                               \
                nx *= w;                                                \
                ny *= w;                                                \
            }                                                           \
        }                                                               \
    } while (0)

/*!
    \class QTransform
    \brief The QTransform class specifies 2D transformations of a coordinate system.
    \since 4.3
    \ingroup painting

    A transformation specifies how to translate, scale, shear, rotate
    or project the coordinate system, and is typically used when
    rendering graphics.

    QTransform differs from QMatrix in that it is a true 3x3 matrix,
    allowing perspective transformations. QTransform's toAffine()
    method allows casting QTransform to QMatrix. If a perspective
    transformation has been specified on the matrix, then the
    conversion will cause loss of data.

    QTransform is the recommended transformation class in Qt.

    A QTransform object can be built using the setMatrix(), scale(),
    rotate(), translate() and shear() functions.  Alternatively, it
    can be built by applying \l {QTransform#Basic Matrix
    Operations}{basic matrix operations}. The matrix can also be
    defined when constructed, and it can be reset to the identity
    matrix (the default) using the reset() function.

    The QTransform class supports mapping of graphic primitives: A given
    point, line, polygon, region, or painter path can be mapped to the
    coordinate system defined by \e this matrix using the map()
    function. In case of a rectangle, its coordinates can be
    transformed using the mapRect() function. A rectangle can also be
    transformed into a \e polygon (mapped to the coordinate system
    defined by \e this matrix), using the mapToPolygon() function.

    QTransform provides the isIdentity() function which returns true if
    the matrix is the identity matrix, and the isInvertible() function
    which returns true if the matrix is non-singular (i.e. AB = BA =
    I). The inverted() function returns an inverted copy of \e this
    matrix if it is invertible (otherwise it returns the identity
    matrix), and adjoint() returns the matrix's classical adjoint.
    In addition, QTransform provides the determinant() function which
    returns the matrix's determinant.

    Finally, the QTransform class supports matrix multiplication, addition
    and subtraction, and objects of the class can be streamed as well
    as compared.

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
    System} {coordinate system} documentation.

    QPainter has functions to translate, scale, shear and rotate the
    coordinate system without using a QTransform. For example:

    \table 100%
    \row
    \o \inlineimage qtransform-simpletransformation.png
    \o
    \snippet doc/src/snippets/transform/main.cpp 0
    \endtable

    Although these functions are very convenient, it can be more
    efficient to build a QTransform and call QPainter::setTransform() if you
    want to perform more than a single transform operation. For
    example:

    \table 100%
    \row
    \o \inlineimage qtransform-combinedtransformation.png
    \o
    \snippet doc/src/snippets/transform/main.cpp 1
    \endtable

    \section1 Basic Matrix Operations

    \image qtransform-representation.png

    A QTransform object contains a 3 x 3 matrix.  The \c m31 (\c dx) and
    \c m32 (\c dy) elements specify horizontal and vertical translation.
    The \c m11 and \c m22 elements specify horizontal and vertical scaling.
    The \c m21 and \c m12 elements specify horizontal and vertical \e shearing.
    And finally, the \c m13 and \c m23 elements specify horizontal and vertical
    projection, with \c m33 as an additional projection factor.

    QTransform transforms a point in the plane to another point using the
    following formulas:

    \snippet doc/src/snippets/code/src_gui_painting_qtransform.cpp 0

    The point \e (x, y) is the original point, and \e (x', y') is the
    transformed point. \e (x', y') can be transformed back to \e (x,
    y) by performing the same operation on the inverted() matrix.

    The various matrix elements can be set when constructing the
    matrix, or by using the setMatrix() function later on. They can also
    be manipulated using the translate(), rotate(), scale() and
    shear() convenience functions. The currently set values can be
    retrieved using the m11(), m12(), m13(), m21(), m22(), m23(),
    m31(), m32(), m33(), dx() and dy() functions.

    Translation is the simplest transformation. Setting \c dx and \c
    dy will move the coordinate system \c dx units along the X axis
    and \c dy units along the Y axis.  Scaling can be done by setting
    \c m11 and \c m22. For example, setting \c m11 to 2 and \c m22 to
    1.5 will double the height and increase the width by 50%.  The
    identity matrix has \c m11, \c m22, and \c m33 set to 1 (all others are set
    to 0) mapping a point to itself. Shearing is controlled by \c m12
    and \c m21. Setting these elements to values different from zero
    will twist the coordinate system. Rotation is achieved by
    setting both the shearing factors and the scaling factors. Perspective
    transformation is achieved by setting both the projection factors and
    the scaling factors.

    Here's the combined transformations example using basic matrix
    operations:

    \table 100%
    \row
    \o \inlineimage qtransform-combinedtransformation2.png
    \o
    \snippet doc/src/snippets/transform/main.cpp 2
    \endtable

    \sa QPainter, {Coordinate System}, {demos/affine}{Affine
    Transformations Demo}, {Transformations Example}
*/

/*!
    \enum QTransform::TransformationType

    \value TxNone
    \value TxTranslate
    \value TxScale
    \value TxRotate
    \value TxShear
    \value TxProject
*/

/*!
    \fn QTransform::QTransform(Qt::Initialization)
    \internal
*/

/*!
    Constructs an identity matrix.

    All elements are set to zero except \c m11 and \c m22 (specifying
    the scale) and \c m13 which are set to 1.

    \sa reset()
*/
QTransform::QTransform()
    : affine(true)
    , m_13(0), m_23(0), m_33(1)
    , m_type(TxNone)
    , m_dirty(TxNone)
{
}

/*!
    \fn QTransform::QTransform(qreal m11, qreal m12, qreal m13, qreal m21, qreal m22, qreal m23, qreal m31, qreal m32, qreal m33)

    Constructs a matrix with the elements, \a m11, \a m12, \a m13,
    \a m21, \a m22, \a m23, \a m31, \a m32, \a m33.

    \sa setMatrix()
*/
QTransform::QTransform(qreal h11, qreal h12, qreal h13,
                       qreal h21, qreal h22, qreal h23,
                       qreal h31, qreal h32, qreal h33)
    : affine(h11, h12, h21, h22, h31, h32, true)
    , m_13(h13), m_23(h23), m_33(h33)
    , m_type(TxNone)
    , m_dirty(TxProject)
{
}

/*!
    \fn QTransform::QTransform(qreal m11, qreal m12, qreal m21, qreal m22, qreal dx, qreal dy)

    Constructs a matrix with the elements, \a m11, \a m12, \a m21, \a m22, \a dx and \a dy.

    \sa setMatrix()
*/
QTransform::QTransform(qreal h11, qreal h12, qreal h21,
                       qreal h22, qreal dx, qreal dy)
    : affine(h11, h12, h21, h22, dx, dy, true)
    , m_13(0), m_23(0), m_33(1)
    , m_type(TxNone)
    , m_dirty(TxShear)
{
}

/*!
    \fn QTransform::QTransform(const QMatrix &matrix)

    Constructs a matrix that is a copy of the given \a matrix.
    Note that the \c m13, \c m23, and \c m33 elements are set to 0, 0,
    and 1 respectively.
 */
QTransform::QTransform(const QMatrix &mtx)
    : affine(mtx._m11, mtx._m12, mtx._m21, mtx._m22, mtx._dx, mtx._dy, true),
      m_13(0), m_23(0), m_33(1)
    , m_type(TxNone)
    , m_dirty(TxShear)
{
}

/*!
    Returns the adjoint of this matrix.
*/
QTransform QTransform::adjoint() const
{
    qreal h11, h12, h13,
        h21, h22, h23,
        h31, h32, h33;
    h11 = affine._m22*m_33 - m_23*affine._dy;
    h21 = m_23*affine._dx - affine._m21*m_33;
    h31 = affine._m21*affine._dy - affine._m22*affine._dx;
    h12 = m_13*affine._dy - affine._m12*m_33;
    h22 = affine._m11*m_33 - m_13*affine._dx;
    h32 = affine._m12*affine._dx - affine._m11*affine._dy;
    h13 = affine._m12*m_23 - m_13*affine._m22;
    h23 = m_13*affine._m21 - affine._m11*m_23;
    h33 = affine._m11*affine._m22 - affine._m12*affine._m21;

    return QTransform(h11, h12, h13,
                      h21, h22, h23,
                      h31, h32, h33, true);
}

/*!
    Returns the transpose of this matrix.
*/
QTransform QTransform::transposed() const
{
    QTransform t(affine._m11, affine._m21, affine._dx,
                 affine._m12, affine._m22, affine._dy,
                 m_13, m_23, m_33, true);
    t.m_type = m_type;
    t.m_dirty = m_dirty;
    return t;
}

/*!
    Returns an inverted copy of this matrix.

    If the matrix is singular (not invertible), the returned matrix is
    the identity matrix. If \a invertible is valid (i.e. not 0), its
    value is set to true if the matrix is invertible, otherwise it is
    set to false.

    \sa isInvertible()
*/
QTransform QTransform::inverted(bool *invertible) const
{
    QTransform invert(true);
    bool inv = true;

    switch(inline_type()) {
    case TxNone:
        break;
    case TxTranslate:
        invert.affine._dx = -affine._dx;
        invert.affine._dy = -affine._dy;
        break;
    case TxScale:
        inv = !qFuzzyIsNull(affine._m11);
        inv &= !qFuzzyIsNull(affine._m22);
        if (inv) {
            invert.affine._m11 = 1. / affine._m11;
            invert.affine._m22 = 1. / affine._m22;
            invert.affine._dx = -affine._dx * invert.affine._m11;
            invert.affine._dy = -affine._dy * invert.affine._m22;
        }
        break;
    case TxRotate:
    case TxShear:
        invert.affine = affine.inverted(&inv);
        break;
    default:
        // general case
        qreal det = determinant();
        inv = !qFuzzyIsNull(det);
        if (inv)
            invert = adjoint() / det;
        break;
    }

    if (invertible)
        *invertible = inv;

    if (inv) {
        // inverting doesn't change the type
        invert.m_type = m_type;
        invert.m_dirty = m_dirty;
    }

    return invert;
}

/*!
    Moves the coordinate system \a dx along the x axis and \a dy along
    the y axis, and returns a reference to the matrix.

    \sa setMatrix()
*/
QTransform &QTransform::translate(qreal dx, qreal dy)
{
    if (dx == 0 && dy == 0)
        return *this;
#ifndef QT_NO_DEBUG
    if (qIsNaN(dx) | qIsNaN(dy)) {
        qWarning() << "QTransform::translate with NaN called";
        return *this;
    }
#endif

    switch(inline_type()) {
    case TxNone:
        affine._dx = dx;
        affine._dy = dy;
        break;
    case TxTranslate:
        affine._dx += dx;
        affine._dy += dy;
        break;
    case TxScale:
        affine._dx += dx*affine._m11;
        affine._dy += dy*affine._m22;
        break;
    case TxProject:
        m_33 += dx*m_13 + dy*m_23;
        // Fall through
    case TxShear:
    case TxRotate:
        affine._dx += dx*affine._m11 + dy*affine._m21;
        affine._dy += dy*affine._m22 + dx*affine._m12;
        break;
    }
    if (m_dirty < TxTranslate)
        m_dirty = TxTranslate;
    return *this;
}

/*!
    Creates a matrix which corresponds to a translation of \a dx along
    the x axis and \a dy along the y axis. This is the same as
    QTransform().translate(dx, dy) but slightly faster.

    \since 4.5
*/
QTransform QTransform::fromTranslate(qreal dx, qreal dy)
{
#ifndef QT_NO_DEBUG
    if (qIsNaN(dx) | qIsNaN(dy)) {
        qWarning() << "QTransform::fromTranslate with NaN called";
        return QTransform();
}
#endif
    QTransform transform(1, 0, 0, 0, 1, 0, dx, dy, 1, true);
    if (dx == 0 && dy == 0)
        transform.m_type = TxNone;
    else
        transform.m_type = TxTranslate;
    transform.m_dirty = TxNone;
    return transform;
}

/*!
    Scales the coordinate system by \a sx horizontally and \a sy
    vertically, and returns a reference to the matrix.

    \sa setMatrix()
*/
QTransform & QTransform::scale(qreal sx, qreal sy)
{
    if (sx == 1 && sy == 1)
        return *this;
#ifndef QT_NO_DEBUG
    if (qIsNaN(sx) | qIsNaN(sy)) {
        qWarning() << "QTransform::scale with NaN called";
        return *this;
    }
#endif

    switch(inline_type()) {
    case TxNone:
    case TxTranslate:
        affine._m11 = sx;
        affine._m22 = sy;
        break;
    case TxProject:
        m_13 *= sx;
        m_23 *= sy;
        // fall through
    case TxRotate:
    case TxShear:
        affine._m12 *= sx;
        affine._m21 *= sy;
        // fall through
    case TxScale:
        affine._m11 *= sx;
        affine._m22 *= sy;
        break;
    }
    if (m_dirty < TxScale)
        m_dirty = TxScale;
    return *this;
}

/*!
    Creates a matrix which corresponds to a scaling of
    \a sx horizontally and \a sy vertically.
    This is the same as QTransform().scale(sx, sy) but slightly faster.

    \since 4.5
*/
QTransform QTransform::fromScale(qreal sx, qreal sy)
{
#ifndef QT_NO_DEBUG
    if (qIsNaN(sx) | qIsNaN(sy)) {
        qWarning() << "QTransform::fromScale with NaN called";
        return QTransform();
}
#endif
    QTransform transform(sx, 0, 0, 0, sy, 0, 0, 0, 1, true);
    if (sx == 1. && sy == 1.)
        transform.m_type = TxNone;
    else
        transform.m_type = TxScale;
    transform.m_dirty = TxNone;
    return transform;
}

/*!
    Shears the coordinate system by \a sh horizontally and \a sv
    vertically, and returns a reference to the matrix.

    \sa setMatrix()
*/
QTransform & QTransform::shear(qreal sh, qreal sv)
{
    if (sh == 0 && sv == 0)
        return *this;
#ifndef QT_NO_DEBUG
    if (qIsNaN(sh) | qIsNaN(sv)) {
        qWarning() << "QTransform::shear with NaN called";
        return *this;
    }
#endif

    switch(inline_type()) {
    case TxNone:
    case TxTranslate:
        affine._m12 = sv;
        affine._m21 = sh;
        break;
    case TxScale:
        affine._m12 = sv*affine._m22;
        affine._m21 = sh*affine._m11;
        break;
    case TxProject: {
        qreal tm13 = sv*m_23;
        qreal tm23 = sh*m_13;
        m_13 += tm13;
        m_23 += tm23;
    }
        // fall through
    case TxRotate:
    case TxShear: {
        qreal tm11 = sv*affine._m21;
        qreal tm22 = sh*affine._m12;
        qreal tm12 = sv*affine._m22;
        qreal tm21 = sh*affine._m11;
        affine._m11 += tm11; affine._m12 += tm12;
        affine._m21 += tm21; affine._m22 += tm22;
        break;
    }
    }
    if (m_dirty < TxShear)
        m_dirty = TxShear;
    return *this;
}

const qreal deg2rad = qreal(0.017453292519943295769);        // pi/180
const qreal inv_dist_to_plane = 1. / 1024.;

/*!
    \fn QTransform &QTransform::rotate(qreal angle, Qt::Axis axis)

    Rotates the coordinate system counterclockwise by the given \a angle
    about the specified \a axis and returns a reference to the matrix.

    Note that if you apply a QTransform to a point defined in widget
    coordinates, the direction of the rotation will be clockwise
    because the y-axis points downwards.

    The angle is specified in degrees.

    \sa setMatrix()
*/
QTransform & QTransform::rotate(qreal a, Qt::Axis axis)
{
    if (a == 0)
        return *this;
#ifndef QT_NO_DEBUG
    if (qIsNaN(a)) {
        qWarning() << "QTransform::rotate with NaN called";
        return *this;
    }
#endif

    qreal sina = 0;
    qreal cosa = 0;
    if (a == 90. || a == -270.)
        sina = 1.;
    else if (a == 270. || a == -90.)
        sina = -1.;
    else if (a == 180.)
        cosa = -1.;
    else{
        qreal b = deg2rad*a;          // convert to radians
        sina = qSin(b);               // fast and convenient
        cosa = qCos(b);
    }

    if (axis == Qt::ZAxis) {
        switch(inline_type()) {
        case TxNone:
        case TxTranslate:
            affine._m11 = cosa;
            affine._m12 = sina;
            affine._m21 = -sina;
            affine._m22 = cosa;
            break;
        case TxScale: {
            qreal tm11 = cosa*affine._m11;
            qreal tm12 = sina*affine._m22;
            qreal tm21 = -sina*affine._m11;
            qreal tm22 = cosa*affine._m22;
            affine._m11 = tm11; affine._m12 = tm12;
            affine._m21 = tm21; affine._m22 = tm22;
            break;
        }
        case TxProject: {
            qreal tm13 = cosa*m_13 + sina*m_23;
            qreal tm23 = -sina*m_13 + cosa*m_23;
            m_13 = tm13;
            m_23 = tm23;
            // fall through
        }
        case TxRotate:
        case TxShear: {
            qreal tm11 = cosa*affine._m11 + sina*affine._m21;
            qreal tm12 = cosa*affine._m12 + sina*affine._m22;
            qreal tm21 = -sina*affine._m11 + cosa*affine._m21;
            qreal tm22 = -sina*affine._m12 + cosa*affine._m22;
            affine._m11 = tm11; affine._m12 = tm12;
            affine._m21 = tm21; affine._m22 = tm22;
            break;
        }
        }
        if (m_dirty < TxRotate)
            m_dirty = TxRotate;
    } else {
        QTransform result;
        if (axis == Qt::YAxis) {
            result.affine._m11 = cosa;
            result.m_13 = -sina * inv_dist_to_plane;
        } else {
            result.affine._m22 = cosa;
            result.m_23 = -sina * inv_dist_to_plane;
        }
        result.m_type = TxProject;
        *this = result * *this;
    }

    return *this;
}

/*!
    \fn QTransform & QTransform::rotateRadians(qreal angle, Qt::Axis axis)

    Rotates the coordinate system counterclockwise by the given \a angle
    about the specified \a axis and returns a reference to the matrix.

    Note that if you apply a QTransform to a point defined in widget
    coordinates, the direction of the rotation will be clockwise
    because the y-axis points downwards.

    The angle is specified in radians.

    \sa setMatrix()
*/
QTransform & QTransform::rotateRadians(qreal a, Qt::Axis axis)
{
#ifndef QT_NO_DEBUG
    if (qIsNaN(a)) {
        qWarning() << "QTransform::rotateRadians with NaN called";
        return *this;
    }
#endif
    qreal sina = qSin(a);
    qreal cosa = qCos(a);

    if (axis == Qt::ZAxis) {
        switch(inline_type()) {
        case TxNone:
        case TxTranslate:
            affine._m11 = cosa;
            affine._m12 = sina;
            affine._m21 = -sina;
            affine._m22 = cosa;
            break;
        case TxScale: {
            qreal tm11 = cosa*affine._m11;
            qreal tm12 = sina*affine._m22;
            qreal tm21 = -sina*affine._m11;
            qreal tm22 = cosa*affine._m22;
            affine._m11 = tm11; affine._m12 = tm12;
            affine._m21 = tm21; affine._m22 = tm22;
            break;
        }
        case TxProject: {
            qreal tm13 = cosa*m_13 + sina*m_23;
            qreal tm23 = -sina*m_13 + cosa*m_23;
            m_13 = tm13;
            m_23 = tm23;
            // fall through
        }
        case TxRotate:
        case TxShear: {
            qreal tm11 = cosa*affine._m11 + sina*affine._m21;
            qreal tm12 = cosa*affine._m12 + sina*affine._m22;
            qreal tm21 = -sina*affine._m11 + cosa*affine._m21;
            qreal tm22 = -sina*affine._m12 + cosa*affine._m22;
            affine._m11 = tm11; affine._m12 = tm12;
            affine._m21 = tm21; affine._m22 = tm22;
            break;
        }
        }
        if (m_dirty < TxRotate)
            m_dirty = TxRotate;
    } else {
        QTransform result;
        if (axis == Qt::YAxis) {
            result.affine._m11 = cosa;
            result.m_13 = -sina * inv_dist_to_plane;
        } else {
            result.affine._m22 = cosa;
            result.m_23 = -sina * inv_dist_to_plane;
        }
        result.m_type = TxProject;
        *this = result * *this;
    }
    return *this;
}

/*!
    \fn bool QTransform::operator==(const QTransform &matrix) const
    Returns true if this matrix is equal to the given \a matrix,
    otherwise returns false.
*/
bool QTransform::operator==(const QTransform &o) const
{
    return affine._m11 == o.affine._m11 &&
           affine._m12 == o.affine._m12 &&
           affine._m21 == o.affine._m21 &&
           affine._m22 == o.affine._m22 &&
           affine._dx == o.affine._dx &&
           affine._dy == o.affine._dy &&
           m_13 == o.m_13 &&
           m_23 == o.m_23 &&
           m_33 == o.m_33;
}

/*!
    \fn bool QTransform::operator!=(const QTransform &matrix) const
    Returns true if this matrix is not equal to the given \a matrix,
    otherwise returns false.
*/
bool QTransform::operator!=(const QTransform &o) const
{
    return !operator==(o);
}

/*!
    \fn QTransform & QTransform::operator*=(const QTransform &matrix)
    \overload

    Returns the result of multiplying this matrix by the given \a
    matrix.
*/
QTransform & QTransform::operator*=(const QTransform &o)
{
    const TransformationType otherType = o.inline_type();
    if (otherType == TxNone)
        return *this;

    const TransformationType thisType = inline_type();
    if (thisType == TxNone)
        return operator=(o);

    TransformationType t = qMax(thisType, otherType);
    switch(t) {
    case TxNone:
        break;
    case TxTranslate:
        affine._dx += o.affine._dx;
        affine._dy += o.affine._dy;
        break;
    case TxScale:
    {
        qreal m11 = affine._m11*o.affine._m11;
        qreal m22 = affine._m22*o.affine._m22;

        qreal m31 = affine._dx*o.affine._m11 + o.affine._dx;
        qreal m32 = affine._dy*o.affine._m22 + o.affine._dy;

        affine._m11 = m11;
        affine._m22 = m22;
        affine._dx = m31; affine._dy = m32;
        break;
    }
    case TxRotate:
    case TxShear:
    {
        qreal m11 = affine._m11*o.affine._m11 + affine._m12*o.affine._m21;
        qreal m12 = affine._m11*o.affine._m12 + affine._m12*o.affine._m22;

        qreal m21 = affine._m21*o.affine._m11 + affine._m22*o.affine._m21;
        qreal m22 = affine._m21*o.affine._m12 + affine._m22*o.affine._m22;

        qreal m31 = affine._dx*o.affine._m11 + affine._dy*o.affine._m21 + o.affine._dx;
        qreal m32 = affine._dx*o.affine._m12 + affine._dy*o.affine._m22 + o.affine._dy;

        affine._m11 = m11; affine._m12 = m12;
        affine._m21 = m21; affine._m22 = m22;
        affine._dx = m31; affine._dy = m32;
        break;
    }
    case TxProject:
    {
        qreal m11 = affine._m11*o.affine._m11 + affine._m12*o.affine._m21 + m_13*o.affine._dx;
        qreal m12 = affine._m11*o.affine._m12 + affine._m12*o.affine._m22 + m_13*o.affine._dy;
        qreal m13 = affine._m11*o.m_13 + affine._m12*o.m_23 + m_13*o.m_33;

        qreal m21 = affine._m21*o.affine._m11 + affine._m22*o.affine._m21 + m_23*o.affine._dx;
        qreal m22 = affine._m21*o.affine._m12 + affine._m22*o.affine._m22 + m_23*o.affine._dy;
        qreal m23 = affine._m21*o.m_13 + affine._m22*o.m_23 + m_23*o.m_33;

        qreal m31 = affine._dx*o.affine._m11 + affine._dy*o.affine._m21 + m_33*o.affine._dx;
        qreal m32 = affine._dx*o.affine._m12 + affine._dy*o.affine._m22 + m_33*o.affine._dy;
        qreal m33 = affine._dx*o.m_13 + affine._dy*o.m_23 + m_33*o.m_33;

        affine._m11 = m11; affine._m12 = m12; m_13 = m13;
        affine._m21 = m21; affine._m22 = m22; m_23 = m23;
        affine._dx = m31; affine._dy = m32; m_33 = m33;
    }
    }

    m_dirty = t;
    m_type = t;

    return *this;
}

/*!
    \fn QTransform QTransform::operator*(const QTransform &matrix) const
    Returns the result of multiplying this matrix by the given \a
    matrix.

    Note that matrix multiplication is not commutative, i.e. a*b !=
    b*a.
*/
QTransform QTransform::operator*(const QTransform &m) const
{
    const TransformationType otherType = m.inline_type();
    if (otherType == TxNone)
        return *this;

    const TransformationType thisType = inline_type();
    if (thisType == TxNone)
        return m;

    QTransform t(true);
    TransformationType type = qMax(thisType, otherType);
    switch(type) {
    case TxNone:
        break;
    case TxTranslate:
        t.affine._dx = affine._dx + m.affine._dx;
        t.affine._dy += affine._dy + m.affine._dy;
        break;
    case TxScale:
    {
        qreal m11 = affine._m11*m.affine._m11;
        qreal m22 = affine._m22*m.affine._m22;

        qreal m31 = affine._dx*m.affine._m11 + m.affine._dx;
        qreal m32 = affine._dy*m.affine._m22 + m.affine._dy;

        t.affine._m11 = m11;
        t.affine._m22 = m22;
        t.affine._dx = m31; t.affine._dy = m32;
        break;
    }
    case TxRotate:
    case TxShear:
    {
        qreal m11 = affine._m11*m.affine._m11 + affine._m12*m.affine._m21;
        qreal m12 = affine._m11*m.affine._m12 + affine._m12*m.affine._m22;

        qreal m21 = affine._m21*m.affine._m11 + affine._m22*m.affine._m21;
        qreal m22 = affine._m21*m.affine._m12 + affine._m22*m.affine._m22;

        qreal m31 = affine._dx*m.affine._m11 + affine._dy*m.affine._m21 + m.affine._dx;
        qreal m32 = affine._dx*m.affine._m12 + affine._dy*m.affine._m22 + m.affine._dy;

        t.affine._m11 = m11; t.affine._m12 = m12;
        t.affine._m21 = m21; t.affine._m22 = m22;
        t.affine._dx = m31; t.affine._dy = m32;
        break;
    }
    case TxProject:
    {
        qreal m11 = affine._m11*m.affine._m11 + affine._m12*m.affine._m21 + m_13*m.affine._dx;
        qreal m12 = affine._m11*m.affine._m12 + affine._m12*m.affine._m22 + m_13*m.affine._dy;
        qreal m13 = affine._m11*m.m_13 + affine._m12*m.m_23 + m_13*m.m_33;

        qreal m21 = affine._m21*m.affine._m11 + affine._m22*m.affine._m21 + m_23*m.affine._dx;
        qreal m22 = affine._m21*m.affine._m12 + affine._m22*m.affine._m22 + m_23*m.affine._dy;
        qreal m23 = affine._m21*m.m_13 + affine._m22*m.m_23 + m_23*m.m_33;

        qreal m31 = affine._dx*m.affine._m11 + affine._dy*m.affine._m21 + m_33*m.affine._dx;
        qreal m32 = affine._dx*m.affine._m12 + affine._dy*m.affine._m22 + m_33*m.affine._dy;
        qreal m33 = affine._dx*m.m_13 + affine._dy*m.m_23 + m_33*m.m_33;

        t.affine._m11 = m11; t.affine._m12 = m12; t.m_13 = m13;
        t.affine._m21 = m21; t.affine._m22 = m22; t.m_23 = m23;
        t.affine._dx = m31; t.affine._dy = m32; t.m_33 = m33;
    }
    }

    t.m_dirty = type;
    t.m_type = type;

    return t;
}

/*!
    \fn QTransform & QTransform::operator*=(qreal scalar)
    \overload

    Returns the result of performing an element-wise multiplication of this
    matrix with the given \a scalar.
*/

/*!
    \fn QTransform & QTransform::operator/=(qreal scalar)
    \overload

    Returns the result of performing an element-wise division of this
    matrix by the given \a scalar.
*/

/*!
    \fn QTransform & QTransform::operator+=(qreal scalar)
    \overload

    Returns the matrix obtained by adding the given \a scalar to each
    element of this matrix.
*/

/*!
    \fn QTransform & QTransform::operator-=(qreal scalar)
    \overload

    Returns the matrix obtained by subtracting the given \a scalar from each
    element of this matrix.
*/

/*!
    Assigns the given \a matrix's values to this matrix.
*/
QTransform & QTransform::operator=(const QTransform &matrix)
{
    affine._m11 = matrix.affine._m11;
    affine._m12 = matrix.affine._m12;
    affine._m21 = matrix.affine._m21;
    affine._m22 = matrix.affine._m22;
    affine._dx = matrix.affine._dx;
    affine._dy = matrix.affine._dy;
    m_13 = matrix.m_13;
    m_23 = matrix.m_23;
    m_33 = matrix.m_33;
    m_type = matrix.m_type;
    m_dirty = matrix.m_dirty;

    return *this;
}

/*!
    Resets the matrix to an identity matrix, i.e. all elements are set
    to zero, except \c m11 and \c m22 (specifying the scale) and \c m33
    which are set to 1.

    \sa QTransform(), isIdentity(), {QTransform#Basic Matrix
    Operations}{Basic Matrix Operations}
*/
void QTransform::reset()
{
    affine._m11 = affine._m22 = m_33 = 1.0;
    affine._m12 = m_13 = affine._m21 = m_23 = affine._dx = affine._dy = 0;
    m_type = TxNone;
    m_dirty = TxNone;
}

#ifndef QT_NO_DATASTREAM
/*!
    \fn QDataStream &operator<<(QDataStream &stream, const QTransform &matrix)
    \since 4.3
    \relates QTransform

    Writes the given \a matrix to the given \a stream and returns a
    reference to the stream.

    \sa {Serializing Qt Data Types}
*/
QDataStream & operator<<(QDataStream &s, const QTransform &m)
{
    s << double(m.m11())
      << double(m.m12())
      << double(m.m13())
      << double(m.m21())
      << double(m.m22())
      << double(m.m23())
      << double(m.m31())
      << double(m.m32())
      << double(m.m33());
    return s;
}

/*!
    \fn QDataStream &operator>>(QDataStream &stream, QTransform &matrix)
    \since 4.3
    \relates QTransform

    Reads the given \a matrix from the given \a stream and returns a
    reference to the stream.

    \sa {Serializing Qt Data Types}
*/
QDataStream & operator>>(QDataStream &s, QTransform &t)
{
     double m11, m12, m13,
         m21, m22, m23,
         m31, m32, m33;

     s >> m11;
     s >> m12;
     s >> m13;
     s >> m21;
     s >> m22;
     s >> m23;
     s >> m31;
     s >> m32;
     s >> m33;
     t.setMatrix(m11, m12, m13,
                 m21, m22, m23,
                 m31, m32, m33);
     return s;
}

#endif // QT_NO_DATASTREAM

#ifndef QT_NO_DEBUG_STREAM
QDebug operator<<(QDebug dbg, const QTransform &m)
{
    static const char *typeStr[] =
    {
        "TxNone",
        "TxTranslate",
        "TxScale",
        0,
        "TxRotate",
        0, 0, 0,
        "TxShear",
        0, 0, 0, 0, 0, 0, 0,
        "TxProject"
    };

    dbg.nospace() << "QTransform(type=" << typeStr[m.type()] << ','
                  << " 11=" << m.m11()
                  << " 12=" << m.m12()
                  << " 13=" << m.m13()
                  << " 21=" << m.m21()
                  << " 22=" << m.m22()
                  << " 23=" << m.m23()
                  << " 31=" << m.m31()
                  << " 32=" << m.m32()
                  << " 33=" << m.m33()
                  << ')';

    return dbg.space();
}
#endif

/*!
    \fn QPoint operator*(const QPoint &point, const QTransform &matrix)
    \relates QTransform

    This is the same as \a{matrix}.map(\a{point}).

    \sa QTransform::map()
*/
QPoint QTransform::map(const QPoint &p) const
{
    qreal fx = p.x();
    qreal fy = p.y();

    qreal x = 0, y = 0;

    TransformationType t = inline_type();
    switch(t) {
    case TxNone:
        x = fx;
        y = fy;
        break;
    case TxTranslate:
        x = fx + affine._dx;
        y = fy + affine._dy;
        break;
    case TxScale:
        x = affine._m11 * fx + affine._dx;
        y = affine._m22 * fy + affine._dy;
        break;
    case TxRotate:
    case TxShear:
    case TxProject:
        x = affine._m11 * fx + affine._m21 * fy + affine._dx;
        y = affine._m12 * fx + affine._m22 * fy + affine._dy;
        if (t == TxProject) {
            qreal w = 1./(m_13 * fx + m_23 * fy + m_33);
            x *= w;
            y *= w;
        }
    }
    return QPoint(qRound(x), qRound(y));
}


/*!
    \fn QPointF operator*(const QPointF &point, const QTransform &matrix)
    \relates QTransform

    Same as \a{matrix}.map(\a{point}).

    \sa QTransform::map()
*/

/*!
    \overload

    Creates and returns a QPointF object that is a copy of the given point,
    \a p, mapped into the coordinate system defined by this matrix.
*/
QPointF QTransform::map(const QPointF &p) const
{
    qreal fx = p.x();
    qreal fy = p.y();

    qreal x = 0, y = 0;

    TransformationType t = inline_type();
    switch(t) {
    case TxNone:
        x = fx;
        y = fy;
        break;
    case TxTranslate:
        x = fx + affine._dx;
        y = fy + affine._dy;
        break;
    case TxScale:
        x = affine._m11 * fx + affine._dx;
        y = affine._m22 * fy + affine._dy;
        break;
    case TxRotate:
    case TxShear:
    case TxProject:
        x = affine._m11 * fx + affine._m21 * fy + affine._dx;
        y = affine._m12 * fx + affine._m22 * fy + affine._dy;
        if (t == TxProject) {
            qreal w = 1./(m_13 * fx + m_23 * fy + m_33);
            x *= w;
            y *= w;
        }
    }
    return QPointF(x, y);
}

/*!
    \fn QPoint QTransform::map(const QPoint &point) const
    \overload

    Creates and returns a QPoint object that is a copy of the given \a
    point, mapped into the coordinate system defined by this
    matrix. Note that the transformed coordinates are rounded to the
    nearest integer.
*/

/*!
    \fn QLineF operator*(const QLineF &line, const QTransform &matrix)
    \relates QTransform

    This is the same as \a{matrix}.map(\a{line}).

    \sa QTransform::map()
*/

/*!
    \fn QLine operator*(const QLine &line, const QTransform &matrix)
    \relates QTransform

    This is the same as \a{matrix}.map(\a{line}).

    \sa QTransform::map()
*/

/*!
    \overload

    Creates and returns a QLineF object that is a copy of the given line,
    \a l, mapped into the coordinate system defined by this matrix.
*/
QLine QTransform::map(const QLine &l) const
{
    qreal fx1 = l.x1();
    qreal fy1 = l.y1();
    qreal fx2 = l.x2();
    qreal fy2 = l.y2();

    qreal x1 = 0, y1 = 0, x2 = 0, y2 = 0;

    TransformationType t = inline_type();
    switch(t) {
    case TxNone:
        x1 = fx1;
        y1 = fy1;
        x2 = fx2;
        y2 = fy2;
        break;
    case TxTranslate:
        x1 = fx1 + affine._dx;
        y1 = fy1 + affine._dy;
        x2 = fx2 + affine._dx;
        y2 = fy2 + affine._dy;
        break;
    case TxScale:
        x1 = affine._m11 * fx1 + affine._dx;
        y1 = affine._m22 * fy1 + affine._dy;
        x2 = affine._m11 * fx2 + affine._dx;
        y2 = affine._m22 * fy2 + affine._dy;
        break;
    case TxRotate:
    case TxShear:
    case TxProject:
        x1 = affine._m11 * fx1 + affine._m21 * fy1 + affine._dx;
        y1 = affine._m12 * fx1 + affine._m22 * fy1 + affine._dy;
        x2 = affine._m11 * fx2 + affine._m21 * fy2 + affine._dx;
        y2 = affine._m12 * fx2 + affine._m22 * fy2 + affine._dy;
        if (t == TxProject) {
            qreal w = 1./(m_13 * fx1 + m_23 * fy1 + m_33);
            x1 *= w;
            y1 *= w;
            w = 1./(m_13 * fx2 + m_23 * fy2 + m_33);
            x2 *= w;
            y2 *= w;
        }
    }
    return QLine(qRound(x1), qRound(y1), qRound(x2), qRound(y2));
}

/*!
    \overload

    \fn QLineF QTransform::map(const QLineF &line) const

    Creates and returns a QLine object that is a copy of the given \a
    line, mapped into the coordinate system defined by this matrix.
    Note that the transformed coordinates are rounded to the nearest
    integer.
*/

QLineF QTransform::map(const QLineF &l) const
{
    qreal fx1 = l.x1();
    qreal fy1 = l.y1();
    qreal fx2 = l.x2();
    qreal fy2 = l.y2();

    qreal x1 = 0, y1 = 0, x2 = 0, y2 = 0;

    TransformationType t = inline_type();
    switch(t) {
    case TxNone:
        x1 = fx1;
        y1 = fy1;
        x2 = fx2;
        y2 = fy2;
        break;
    case TxTranslate:
        x1 = fx1 + affine._dx;
        y1 = fy1 + affine._dy;
        x2 = fx2 + affine._dx;
        y2 = fy2 + affine._dy;
        break;
    case TxScale:
        x1 = affine._m11 * fx1 + affine._dx;
        y1 = affine._m22 * fy1 + affine._dy;
        x2 = affine._m11 * fx2 + affine._dx;
        y2 = affine._m22 * fy2 + affine._dy;
        break;
    case TxRotate:
    case TxShear:
    case TxProject:
        x1 = affine._m11 * fx1 + affine._m21 * fy1 + affine._dx;
        y1 = affine._m12 * fx1 + affine._m22 * fy1 + affine._dy;
        x2 = affine._m11 * fx2 + affine._m21 * fy2 + affine._dx;
        y2 = affine._m12 * fx2 + affine._m22 * fy2 + affine._dy;
        if (t == TxProject) {
            qreal w = 1./(m_13 * fx1 + m_23 * fy1 + m_33);
            x1 *= w;
            y1 *= w;
            w = 1./(m_13 * fx2 + m_23 * fy2 + m_33);
            x2 *= w;
            y2 *= w;
        }
    }
    return QLineF(x1, y1, x2, y2);
}

static QPolygonF mapProjective(const QTransform &transform, const QPolygonF &poly)
{
    if (poly.size() == 0)
        return poly;

    if (poly.size() == 1)
        return QPolygonF() << transform.map(poly.at(0));

    QPainterPath path;
    path.addPolygon(poly);

    path = transform.map(path);

    QPolygonF result;
    for (int i = 0; i < path.elementCount(); ++i)
        result << path.elementAt(i);
    return result;
}


/*!
    \fn QPolygonF operator *(const QPolygonF &polygon, const QTransform &matrix)
    \since 4.3
    \relates QTransform

    This is the same as \a{matrix}.map(\a{polygon}).

    \sa QTransform::map()
*/

/*!
    \fn QPolygon operator*(const QPolygon &polygon, const QTransform &matrix)
    \relates QTransform

    This is the same as \a{matrix}.map(\a{polygon}).

    \sa QTransform::map()
*/

/*!
    \fn QPolygonF QTransform::map(const QPolygonF &polygon) const
    \overload

    Creates and returns a QPolygonF object that is a copy of the given
    \a polygon, mapped into the coordinate system defined by this
    matrix.
*/
QPolygonF QTransform::map(const QPolygonF &a) const
{
    TransformationType t = inline_type();
    if (t <= TxTranslate)
        return a.translated(affine._dx, affine._dy);

    if (t >= QTransform::TxProject)
        return mapProjective(*this, a);

    int size = a.size();
    int i;
    QPolygonF p(size);
    const QPointF *da = a.constData();
    QPointF *dp = p.data();

    for(i = 0; i < size; ++i) {
        MAP(da[i].xp, da[i].yp, dp[i].xp, dp[i].yp);
    }
    return p;
}

/*!
    \fn QPolygon QTransform::map(const QPolygon &polygon) const
    \overload

    Creates and returns a QPolygon object that is a copy of the given
    \a polygon, mapped into the coordinate system defined by this
    matrix. Note that the transformed coordinates are rounded to the
    nearest integer.
*/
QPolygon QTransform::map(const QPolygon &a) const
{
    TransformationType t = inline_type();
    if (t <= TxTranslate)
        return a.translated(qRound(affine._dx), qRound(affine._dy));

    if (t >= QTransform::TxProject)
        return mapProjective(*this, QPolygonF(a)).toPolygon();

    int size = a.size();
    int i;
    QPolygon p(size);
    const QPoint *da = a.constData();
    QPoint *dp = p.data();

    for(i = 0; i < size; ++i) {
        qreal nx = 0, ny = 0;
        MAP(da[i].xp, da[i].yp, nx, ny);
        dp[i].xp = qRound(nx);
        dp[i].yp = qRound(ny);
    }
    return p;
}

/*!
    \fn QRegion operator*(const QRegion &region, const QTransform &matrix)
    \relates QTransform

    This is the same as \a{matrix}.map(\a{region}).

    \sa QTransform::map()
*/

extern Q_AUTOTEST_EXPORT QPainterPath qt_regionToPath(const QRegion &region);

/*!
    \fn QRegion QTransform::map(const QRegion &region) const
    \overload

    Creates and returns a QRegion object that is a copy of the given
    \a region, mapped into the coordinate system defined by this matrix.

    Calling this method can be rather expensive if rotations or
    shearing are used.
*/
QRegion QTransform::map(const QRegion &r) const
{
    TransformationType t = inline_type();
    if (t == TxNone)
        return r;

    if (t == TxTranslate) {
        QRegion copy(r);
        copy.translate(qRound(affine._dx), qRound(affine._dy));
        return copy;
    }

    if (t == TxScale && r.rectCount() == 1)
        return QRegion(mapRect(r.boundingRect()));

    QPainterPath p = map(qt_regionToPath(r));
    return p.toFillPolygon(QTransform()).toPolygon();
}

struct QHomogeneousCoordinate
{
    qreal x;
    qreal y;
    qreal w;

    QHomogeneousCoordinate() {}
    QHomogeneousCoordinate(qreal x_, qreal y_, qreal w_) : x(x_), y(y_), w(w_) {}

    const QPointF toPoint() const {
        qreal iw = 1. / w;
        return QPointF(x * iw, y * iw);
    }
};

static inline QHomogeneousCoordinate mapHomogeneous(const QTransform &transform, const QPointF &p)
{
    QHomogeneousCoordinate c;
    c.x = transform.m11() * p.x() + transform.m21() * p.y() + transform.m31();
    c.y = transform.m12() * p.x() + transform.m22() * p.y() + transform.m32();
    c.w = transform.m13() * p.x() + transform.m23() * p.y() + transform.m33();
    return c;
}

static inline bool lineTo_clipped(QPainterPath &path, const QTransform &transform, const QPointF &a, const QPointF &b,
                                  bool needsMoveTo, bool needsLineTo = true)
{
    QHomogeneousCoordinate ha = mapHomogeneous(transform, a);
    QHomogeneousCoordinate hb = mapHomogeneous(transform, b);

    if (ha.w < Q_NEAR_CLIP && hb.w < Q_NEAR_CLIP)
        return false;

    if (hb.w < Q_NEAR_CLIP) {
        const qreal t = (Q_NEAR_CLIP - hb.w) / (ha.w - hb.w);

        hb.x += (ha.x - hb.x) * t;
        hb.y += (ha.y - hb.y) * t;
        hb.w = qreal(Q_NEAR_CLIP);
    } else if (ha.w < Q_NEAR_CLIP) {
        const qreal t = (Q_NEAR_CLIP - ha.w) / (hb.w - ha.w);

        ha.x += (hb.x - ha.x) * t;
        ha.y += (hb.y - ha.y) * t;
        ha.w = qreal(Q_NEAR_CLIP);

        const QPointF p = ha.toPoint();
        if (needsMoveTo) {
            path.moveTo(p);
            needsMoveTo = false;
        } else {
            path.lineTo(p);
        }
    }

    if (needsMoveTo)
        path.moveTo(ha.toPoint());

    if (needsLineTo)
        path.lineTo(hb.toPoint());

    return true;
}
Q_GUI_EXPORT bool qt_scaleForTransform(const QTransform &transform, qreal *scale);

static inline bool cubicTo_clipped(QPainterPath &path, const QTransform &transform, const QPointF &a, const QPointF &b, const QPointF &c, const QPointF &d, bool needsMoveTo)
{
    // Convert projective xformed curves to line
    // segments so they can be transformed more accurately

    qreal scale;
    qt_scaleForTransform(transform, &scale);

    qreal curveThreshold = scale == 0 ? qreal(0.25) : (qreal(0.25) / scale);

    QPolygonF segment = QBezier::fromPoints(a, b, c, d).toPolygon(curveThreshold);

    for (int i = 0; i < segment.size() - 1; ++i)
        if (lineTo_clipped(path, transform, segment.at(i), segment.at(i+1), needsMoveTo))
            needsMoveTo = false;

    return !needsMoveTo;
}

static QPainterPath mapProjective(const QTransform &transform, const QPainterPath &path)
{
    QPainterPath result;

    QPointF last;
    QPointF lastMoveTo;
    bool needsMoveTo = true;
    for (int i = 0; i < path.elementCount(); ++i) {
        switch (path.elementAt(i).type) {
        case QPainterPath::MoveToElement:
            if (i > 0 && lastMoveTo != last)
                lineTo_clipped(result, transform, last, lastMoveTo, needsMoveTo);

            lastMoveTo = path.elementAt(i);
            last = path.elementAt(i);
            needsMoveTo = true;
            break;
        case QPainterPath::LineToElement:
            if (lineTo_clipped(result, transform, last, path.elementAt(i), needsMoveTo))
                needsMoveTo = false;
            last = path.elementAt(i);
            break;
        case QPainterPath::CurveToElement:
            if (cubicTo_clipped(result, transform, last, path.elementAt(i), path.elementAt(i+1), path.elementAt(i+2), needsMoveTo))
                needsMoveTo = false;
            i += 2;
            last = path.elementAt(i);
            break;
        default:
            Q_ASSERT(false);
        }
    }

    if (path.elementCount() > 0 && lastMoveTo != last)
        lineTo_clipped(result, transform, last, lastMoveTo, needsMoveTo, false);

    result.setFillRule(path.fillRule());
    return result;
}

/*!
    \fn QPainterPath operator *(const QPainterPath &path, const QTransform &matrix)
    \since 4.3
    \relates QTransform

    This is the same as \a{matrix}.map(\a{path}).

    \sa QTransform::map()
*/

/*!
    \overload

    Creates and returns a QPainterPath object that is a copy of the
    given \a path, mapped into the coordinate system defined by this
    matrix.
*/
QPainterPath QTransform::map(const QPainterPath &path) const
{
    TransformationType t = inline_type();
    if (t == TxNone || path.elementCount() == 0)
        return path;

    if (t >= TxProject)
        return mapProjective(*this, path);

    QPainterPath copy = path;

    if (t == TxTranslate) {
        copy.translate(affine._dx, affine._dy);
    } else {
        copy.detach();
        // Full xform
        for (int i=0; i<path.elementCount(); ++i) {
            QPainterPath::Element &e = copy.d_ptr->elements[i];
            MAP(e.x, e.y, e.x, e.y);
        }
    }

    return copy;
}

/*!
    \fn QPolygon QTransform::mapToPolygon(const QRect &rectangle) const

    Creates and returns a QPolygon representation of the given \a
    rectangle, mapped into the coordinate system defined by this
    matrix.

    The rectangle's coordinates are transformed using the following
    formulas:

    \snippet doc/src/snippets/code/src_gui_painting_qtransform.cpp 1

    Polygons and rectangles behave slightly differently when
    transformed (due to integer rounding), so
    \c{matrix.map(QPolygon(rectangle))} is not always the same as
    \c{matrix.mapToPolygon(rectangle)}.

    \sa mapRect(), {QTransform#Basic Matrix Operations}{Basic Matrix
    Operations}
*/
QPolygon QTransform::mapToPolygon(const QRect &rect) const
{
    TransformationType t = inline_type();

    QPolygon a(4);
    qreal x[4] = { 0, 0, 0, 0 }, y[4] = { 0, 0, 0, 0 };
    if (t <= TxScale) {
        x[0] = affine._m11*rect.x() + affine._dx;
        y[0] = affine._m22*rect.y() + affine._dy;
        qreal w = affine._m11*rect.width();
        qreal h = affine._m22*rect.height();
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
        MAP(rect.x(), rect.y(), x[0], y[0]);
        MAP(right, rect.y(), x[1], y[1]);
        MAP(right, bottom, x[2], y[2]);
        MAP(rect.x(), bottom, x[3], y[3]);
    }

    // all coordinates are correctly, tranform to a pointarray
    // (rounding to the next integer)
    a.setPoints(4, qRound(x[0]), qRound(y[0]),
                qRound(x[1]), qRound(y[1]),
                qRound(x[2]), qRound(y[2]),
                qRound(x[3]), qRound(y[3]));
    return a;
}

/*!
    Creates a transformation matrix, \a trans, that maps a unit square
    to a four-sided polygon, \a quad. Returns true if the transformation
    is constructed or false if such a transformation does not exist.

    \sa quadToSquare(), quadToQuad()
*/
bool QTransform::squareToQuad(const QPolygonF &quad, QTransform &trans)
{
    if (quad.count() != 4)
        return false;

    qreal dx0 = quad[0].x();
    qreal dx1 = quad[1].x();
    qreal dx2 = quad[2].x();
    qreal dx3 = quad[3].x();

    qreal dy0 = quad[0].y();
    qreal dy1 = quad[1].y();
    qreal dy2 = quad[2].y();
    qreal dy3 = quad[3].y();

    double ax  = dx0 - dx1 + dx2 - dx3;
    double ay  = dy0 - dy1 + dy2 - dy3;

    if (!ax && !ay) { //afine transform
        trans.setMatrix(dx1 - dx0, dy1 - dy0,  0,
                        dx2 - dx1, dy2 - dy1,  0,
                        dx0,       dy0,  1);
    } else {
        double ax1 = dx1 - dx2;
        double ax2 = dx3 - dx2;
        double ay1 = dy1 - dy2;
        double ay2 = dy3 - dy2;

        /*determinants */
        double gtop    =  ax  * ay2 - ax2 * ay;
        double htop    =  ax1 * ay  - ax  * ay1;
        double bottom  =  ax1 * ay2 - ax2 * ay1;

        double a, b, c, d, e, f, g, h;  /*i is always 1*/

        if (!bottom)
            return false;

        g = gtop/bottom;
        h = htop/bottom;

        a = dx1 - dx0 + g * dx1;
        b = dx3 - dx0 + h * dx3;
        c = dx0;
        d = dy1 - dy0 + g * dy1;
        e = dy3 - dy0 + h * dy3;
        f = dy0;

        trans.setMatrix(a, d, g,
                        b, e, h,
                        c, f, 1.0);
    }

    return true;
}

/*!
    \fn bool QTransform::quadToSquare(const QPolygonF &quad, QTransform &trans)

    Creates a transformation matrix, \a trans, that maps a four-sided polygon,
    \a quad, to a unit square. Returns true if the transformation is constructed
    or false if such a transformation does not exist.

    \sa squareToQuad(), quadToQuad()
*/
bool QTransform::quadToSquare(const QPolygonF &quad, QTransform &trans)
{
    if (!squareToQuad(quad, trans))
        return false;

    bool invertible = false;
    trans = trans.inverted(&invertible);

    return invertible;
}

/*!
    Creates a transformation matrix, \a trans, that maps a four-sided
    polygon, \a one, to another four-sided polygon, \a two.
    Returns true if the transformation is possible; otherwise returns
    false.

    This is a convenience method combining quadToSquare() and
    squareToQuad() methods. It allows the input quad to be
    transformed into any other quad.

    \sa squareToQuad(), quadToSquare()
*/
bool QTransform::quadToQuad(const QPolygonF &one,
                            const QPolygonF &two,
                            QTransform &trans)
{
    QTransform stq;
    if (!quadToSquare(one, trans))
        return false;
    if (!squareToQuad(two, stq))
        return false;
    trans *= stq;
    //qDebug()<<"Final = "<<trans;
    return true;
}

/*!
    Sets the matrix elements to the specified values, \a m11,
    \a m12, \a m13 \a m21, \a m22, \a m23 \a m31, \a m32 and
    \a m33. Note that this function replaces the previous values.
    QTransform provides the translate(), rotate(), scale() and shear()
    convenience functions to manipulate the various matrix elements
    based on the currently defined coordinate system.

    \sa QTransform()
*/

void QTransform::setMatrix(qreal m11, qreal m12, qreal m13,
                           qreal m21, qreal m22, qreal m23,
                           qreal m31, qreal m32, qreal m33)
{
    affine._m11 = m11; affine._m12 = m12; m_13 = m13;
    affine._m21 = m21; affine._m22 = m22; m_23 = m23;
    affine._dx = m31; affine._dy = m32; m_33 = m33;
    m_type = TxNone;
    m_dirty = TxProject;
}

static inline bool needsPerspectiveClipping(const QRectF &rect, const QTransform &transform)
{
    const qreal wx = qMin(transform.m13() * rect.left(), transform.m13() * rect.right());
    const qreal wy = qMin(transform.m23() * rect.top(), transform.m23() * rect.bottom());

    return wx + wy + transform.m33() < Q_NEAR_CLIP;
}

QRect QTransform::mapRect(const QRect &rect) const
{
    TransformationType t = inline_type();
    if (t <= TxTranslate)
        return rect.translated(qRound(affine._dx), qRound(affine._dy));

    if (t <= TxScale) {
        int x = qRound(affine._m11*rect.x() + affine._dx);
        int y = qRound(affine._m22*rect.y() + affine._dy);
        int w = qRound(affine._m11*rect.width());
        int h = qRound(affine._m22*rect.height());
        if (w < 0) {
            w = -w;
            x -= w;
        }
        if (h < 0) {
            h = -h;
            y -= h;
        }
        return QRect(x, y, w, h);
    } else if (t < TxProject || !needsPerspectiveClipping(rect, *this)) {
        // see mapToPolygon for explanations of the algorithm.
        qreal x = 0, y = 0;
        MAP(rect.left(), rect.top(), x, y);
        qreal xmin = x;
        qreal ymin = y;
        qreal xmax = x;
        qreal ymax = y;
        MAP(rect.right() + 1, rect.top(), x, y);
        xmin = qMin(xmin, x);
        ymin = qMin(ymin, y);
        xmax = qMax(xmax, x);
        ymax = qMax(ymax, y);
        MAP(rect.right() + 1, rect.bottom() + 1, x, y);
        xmin = qMin(xmin, x);
        ymin = qMin(ymin, y);
        xmax = qMax(xmax, x);
        ymax = qMax(ymax, y);
        MAP(rect.left(), rect.bottom() + 1, x, y);
        xmin = qMin(xmin, x);
        ymin = qMin(ymin, y);
        xmax = qMax(xmax, x);
        ymax = qMax(ymax, y);
        return QRect(qRound(xmin), qRound(ymin), qRound(xmax)-qRound(xmin), qRound(ymax)-qRound(ymin));
    } else {
        QPainterPath path;
        path.addRect(rect);
        return map(path).boundingRect().toRect();
    }
}

/*!
    \fn QRectF QTransform::mapRect(const QRectF &rectangle) const

    Creates and returns a QRectF object that is a copy of the given \a
    rectangle, mapped into the coordinate system defined by this
    matrix.

    The rectangle's coordinates are transformed using the following
    formulas:

    \snippet doc/src/snippets/code/src_gui_painting_qtransform.cpp 2

    If rotation or shearing has been specified, this function returns
    the \e bounding rectangle. To retrieve the exact region the given
    \a rectangle maps to, use the mapToPolygon() function instead.

    \sa mapToPolygon(), {QTransform#Basic Matrix Operations}{Basic Matrix
    Operations}
*/
QRectF QTransform::mapRect(const QRectF &rect) const
{
    TransformationType t = inline_type();
    if (t <= TxTranslate)
        return rect.translated(affine._dx, affine._dy);

    if (t <= TxScale) {
        qreal x = affine._m11*rect.x() + affine._dx;
        qreal y = affine._m22*rect.y() + affine._dy;
        qreal w = affine._m11*rect.width();
        qreal h = affine._m22*rect.height();
        if (w < 0) {
            w = -w;
            x -= w;
        }
        if (h < 0) {
            h = -h;
            y -= h;
        }
        return QRectF(x, y, w, h);
    } else if (t < TxProject || !needsPerspectiveClipping(rect, *this)) {
        qreal x = 0, y = 0;
        MAP(rect.x(), rect.y(), x, y);
        qreal xmin = x;
        qreal ymin = y;
        qreal xmax = x;
        qreal ymax = y;
        MAP(rect.x() + rect.width(), rect.y(), x, y);
        xmin = qMin(xmin, x);
        ymin = qMin(ymin, y);
        xmax = qMax(xmax, x);
        ymax = qMax(ymax, y);
        MAP(rect.x() + rect.width(), rect.y() + rect.height(), x, y);
        xmin = qMin(xmin, x);
        ymin = qMin(ymin, y);
        xmax = qMax(xmax, x);
        ymax = qMax(ymax, y);
        MAP(rect.x(), rect.y() + rect.height(), x, y);
        xmin = qMin(xmin, x);
        ymin = qMin(ymin, y);
        xmax = qMax(xmax, x);
        ymax = qMax(ymax, y);
        return QRectF(xmin, ymin, xmax-xmin, ymax - ymin);
    } else {
        QPainterPath path;
        path.addRect(rect);
        return map(path).boundingRect();
    }
}

/*!
    \fn QRect QTransform::mapRect(const QRect &rectangle) const
    \overload

    Creates and returns a QRect object that is a copy of the given \a
    rectangle, mapped into the coordinate system defined by this
    matrix. Note that the transformed coordinates are rounded to the
    nearest integer.
*/

/*!
    Maps the given coordinates \a x and \a y into the coordinate
    system defined by this matrix. The resulting values are put in *\a
    tx and *\a ty, respectively.

    The coordinates are transformed using the following formulas:

    \snippet doc/src/snippets/code/src_gui_painting_qtransform.cpp 3

    The point (x, y) is the original point, and (x', y') is the
    transformed point.

    \sa {QTransform#Basic Matrix Operations}{Basic Matrix Operations}
*/
void QTransform::map(qreal x, qreal y, qreal *tx, qreal *ty) const
{
    TransformationType t = inline_type();
    MAP(x, y, *tx, *ty);
}

/*!
    \overload

    Maps the given coordinates \a x and \a y into the coordinate
    system defined by this matrix. The resulting values are put in *\a
    tx and *\a ty, respectively. Note that the transformed coordinates
    are rounded to the nearest integer.
*/
void QTransform::map(int x, int y, int *tx, int *ty) const
{
    TransformationType t = inline_type();
    qreal fx = 0, fy = 0;
    MAP(x, y, fx, fy);
    *tx = qRound(fx);
    *ty = qRound(fy);
}

/*!
  Returns the QTransform as an affine matrix.

  \warning If a perspective transformation has been specified,
  then the conversion will cause loss of data.
*/
const QMatrix &QTransform::toAffine() const
{
    return affine;
}

/*!
  Returns the transformation type of this matrix.

  The transformation type is the highest enumeration value
  capturing all of the matrix's transformations. For example,
  if the matrix both scales and shears, the type would be \c TxShear,
  because \c TxShear has a higher enumeration value than \c TxScale.

  Knowing the transformation type of a matrix is useful for optimization:
  you can often handle specific types more optimally than handling
  the generic case.
  */
QTransform::TransformationType QTransform::type() const
{
    if(m_dirty == TxNone || m_dirty < m_type)
        return static_cast<TransformationType>(m_type);

    switch (static_cast<TransformationType>(m_dirty)) {
    case TxProject:
        if (!qFuzzyIsNull(m_13) || !qFuzzyIsNull(m_23) || !qFuzzyIsNull(m_33 - 1)) {
             m_type = TxProject;
             break;
         }
    case TxShear:
    case TxRotate:
        if (!qFuzzyIsNull(affine._m12) || !qFuzzyIsNull(affine._m21)) {
            const qreal dot = affine._m11 * affine._m12 + affine._m21 * affine._m22;
            if (qFuzzyIsNull(dot))
                m_type = TxRotate;
            else
                m_type = TxShear;
            break;
        }
    case TxScale:
        if (!qFuzzyIsNull(affine._m11 - 1) || !qFuzzyIsNull(affine._m22 - 1)) {
            m_type = TxScale;
            break;
        }
    case TxTranslate:
        if (!qFuzzyIsNull(affine._dx) || !qFuzzyIsNull(affine._dy)) {
            m_type = TxTranslate;
            break;
        }
    case TxNone:
        m_type = TxNone;
        break;
    }

    m_dirty = TxNone;
    return static_cast<TransformationType>(m_type);
}

/*!

    Returns the transform as a QVariant.
*/
QTransform::operator QVariant() const
{
    return QVariant(QVariant::Transform, this);
}


/*!
    \fn bool QTransform::isInvertible() const

    Returns true if the matrix is invertible, otherwise returns false.

    \sa inverted()
*/

/*!
    \fn qreal QTransform::det() const
    \obsolete

    Returns the matrix's determinant. Use determinant() instead.
*/


/*!
    \fn qreal QTransform::m11() const

    Returns the horizontal scaling factor.

    \sa scale(), {QTransform#Basic Matrix Operations}{Basic Matrix
    Operations}
*/

/*!
    \fn qreal QTransform::m12() const

    Returns the vertical shearing factor.

    \sa shear(), {QTransform#Basic Matrix Operations}{Basic Matrix
    Operations}
*/

/*!
    \fn qreal QTransform::m21() const

    Returns the horizontal shearing factor.

    \sa shear(), {QTransform#Basic Matrix Operations}{Basic Matrix
    Operations}
*/

/*!
    \fn qreal QTransform::m22() const

    Returns the vertical scaling factor.

    \sa scale(), {QTransform#Basic Matrix Operations}{Basic Matrix
    Operations}
*/

/*!
    \fn qreal QTransform::dx() const

    Returns the horizontal translation factor.

    \sa m31(), translate(), {QTransform#Basic Matrix Operations}{Basic Matrix
    Operations}
*/

/*!
    \fn qreal QTransform::dy() const

    Returns the vertical translation factor.

    \sa translate(), {QTransform#Basic Matrix Operations}{Basic Matrix
    Operations}
*/


/*!
    \fn qreal QTransform::m13() const

    Returns the horizontal projection factor.

    \sa translate(), {QTransform#Basic Matrix Operations}{Basic Matrix
    Operations}
*/


/*!
    \fn qreal QTransform::m23() const

    Returns the vertical projection factor.

    \sa translate(), {QTransform#Basic Matrix Operations}{Basic Matrix
    Operations}
*/

/*!
    \fn qreal QTransform::m31() const

    Returns the horizontal translation factor.

    \sa dx(), translate(), {QTransform#Basic Matrix Operations}{Basic Matrix
    Operations}
*/

/*!
    \fn qreal QTransform::m32() const

    Returns the vertical translation factor.

    \sa dy(), translate(), {QTransform#Basic Matrix Operations}{Basic Matrix
    Operations}
*/

/*!
    \fn qreal QTransform::m33() const

    Returns the division factor.

    \sa translate(), {QTransform#Basic Matrix Operations}{Basic Matrix
    Operations}
*/

/*!
    \fn qreal QTransform::determinant() const

    Returns the matrix's determinant.
*/

/*!
    \fn bool QTransform::isIdentity() const

    Returns true if the matrix is the identity matrix, otherwise
    returns false.

    \sa reset()
*/

/*!
    \fn bool QTransform::isAffine() const

    Returns true if the matrix represent an affine transformation,
    otherwise returns false.
*/

/*!
    \fn bool QTransform::isScaling() const

    Returns true if the matrix represents a scaling
    transformation, otherwise returns false.

    \sa reset()
*/

/*!
    \fn bool QTransform::isRotating() const

    Returns true if the matrix represents some kind of a
    rotating transformation, otherwise returns false.

    \sa reset()
*/

/*!
    \fn bool QTransform::isTranslating() const

    Returns true if the matrix represents a translating
    transformation, otherwise returns false.

    \sa reset()
*/

/*!
    \fn bool qFuzzyCompare(const QTransform& t1, const QTransform& t2)

    \relates QTransform
    \since 4.6

    Returns true if \a t1 and \a t2 are equal, allowing for a small
    fuzziness factor for floating-point comparisons; false otherwise.
*/


// returns true if the transform is uniformly scaling
// (same scale in x and y direction)
// scale is set to the max of x and y scaling factors
Q_GUI_EXPORT
bool qt_scaleForTransform(const QTransform &transform, qreal *scale)
{
    const QTransform::TransformationType type = transform.type();
    if (type <= QTransform::TxTranslate) {
        if (scale)
            *scale = 1;
        return true;
    } else if (type == QTransform::TxScale) {
        const qreal xScale = qAbs(transform.m11());
        const qreal yScale = qAbs(transform.m22());
        if (scale)
            *scale = qMax(xScale, yScale);
        return qFuzzyCompare(xScale, yScale);
    }

    const qreal xScale = transform.m11() * transform.m11()
                         + transform.m21() * transform.m21();
    const qreal yScale = transform.m12() * transform.m12()
                         + transform.m22() * transform.m22();
    if (scale)
        *scale = qSqrt(qMax(xScale, yScale));
    return type == QTransform::TxRotate && qFuzzyCompare(xScale, yScale);
}

QT_END_NAMESPACE
