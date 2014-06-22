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

#include "qvector2d.h"
#include "qvector3d.h"
#include "qvector4d.h"
#include <QtCore/qdatastream.h>
#include <QtCore/qdebug.h>
#include <QtCore/qvariant.h>
#include <QtCore/qmath.h>

QT_BEGIN_NAMESPACE

#ifndef QT_NO_VECTOR2D

/*!
    \class QVector2D
    \brief The QVector2D class represents a vector or vertex in 2D space.
    \since 4.6
    \ingroup painting
    \ingroup painting-3D
    \inmodule QtGui

    The QVector2D class can also be used to represent vertices in 2D space.
    We therefore do not need to provide a separate vertex class.

    \sa QVector3D, QVector4D, QQuaternion
*/

/*!
    \fn QVector2D::QVector2D()

    Constructs a null vector, i.e. with coordinates (0, 0).
*/

/*!
    \fn QVector2D::QVector2D(float xpos, float ypos)

    Constructs a vector with coordinates (\a xpos, \a ypos).
*/

/*!
    \fn QVector2D::QVector2D(const QPoint& point)

    Constructs a vector with x and y coordinates from a 2D \a point.
*/

/*!
    \fn QVector2D::QVector2D(const QPointF& point)

    Constructs a vector with x and y coordinates from a 2D \a point.
*/

#ifndef QT_NO_VECTOR3D

/*!
    Constructs a vector with x and y coordinates from a 3D \a vector.
    The z coordinate of \a vector is dropped.

    \sa toVector3D()
*/
QVector2D::QVector2D(const QVector3D& vector)
{
    xp = vector.xp;
    yp = vector.yp;
}

#endif

#ifndef QT_NO_VECTOR4D

/*!
    Constructs a vector with x and y coordinates from a 3D \a vector.
    The z and w coordinates of \a vector are dropped.

    \sa toVector4D()
*/
QVector2D::QVector2D(const QVector4D& vector)
{
    xp = vector.xp;
    yp = vector.yp;
}

#endif

/*!
    \fn bool QVector2D::isNull() const

    Returns \c true if the x and y coordinates are set to 0.0,
    otherwise returns \c false.
*/

/*!
    \fn float QVector2D::x() const

    Returns the x coordinate of this point.

    \sa setX(), y()
*/

/*!
    \fn float QVector2D::y() const

    Returns the y coordinate of this point.

    \sa setY(), x()
*/

/*!
    \fn void QVector2D::setX(float x)

    Sets the x coordinate of this point to the given \a x coordinate.

    \sa x(), setY()
*/

/*!
    \fn void QVector2D::setY(float y)

    Sets the y coordinate of this point to the given \a y coordinate.

    \sa y(), setX()
*/

/*! \fn float &QVector2D::operator[](int i)
    \since 5.2

    Returns the component of the vector at index position \a i
    as a modifiable reference.

    \a i must be a valid index position in the vector (i.e., 0 <= \a i
    < 2).
*/

/*! \fn float QVector2D::operator[](int i) const
    \since 5.2

    Returns the component of the vector at index position \a i.

    \a i must be a valid index position in the vector (i.e., 0 <= \a i
    < 2).
*/

/*!
    Returns the length of the vector from the origin.

    \sa lengthSquared(), normalized()
*/
float QVector2D::length() const
{
    // Need some extra precision if the length is very small.
    double len = double(xp) * double(xp) +
                 double(yp) * double(yp);
    return float(sqrt(len));
}

/*!
    Returns the squared length of the vector from the origin.
    This is equivalent to the dot product of the vector with itself.

    \sa length(), dotProduct()
*/
float QVector2D::lengthSquared() const
{
    return xp * xp + yp * yp;
}

/*!
    Returns the normalized unit vector form of this vector.

    If this vector is null, then a null vector is returned.  If the length
    of the vector is very close to 1, then the vector will be returned as-is.
    Otherwise the normalized form of the vector of length 1 will be returned.

    \sa length(), normalize()
*/
QVector2D QVector2D::normalized() const
{
    // Need some extra precision if the length is very small.
    double len = double(xp) * double(xp) +
                 double(yp) * double(yp);
    if (qFuzzyIsNull(len - 1.0f)) {
        return *this;
    } else if (!qFuzzyIsNull(len)) {
        double sqrtLen = sqrt(len);
        return QVector2D(float(double(xp) / sqrtLen), float(double(yp) / sqrtLen));
    } else {
        return QVector2D();
    }
}

/*!
    Normalizes the currect vector in place.  Nothing happens if this
    vector is a null vector or the length of the vector is very close to 1.

    \sa length(), normalized()
*/
void QVector2D::normalize()
{
    // Need some extra precision if the length is very small.
    double len = double(xp) * double(xp) +
                 double(yp) * double(yp);
    if (qFuzzyIsNull(len - 1.0f) || qFuzzyIsNull(len))
        return;

    len = sqrt(len);

    xp = float(double(xp) / len);
    yp = float(double(yp) / len);
}

/*!
    \since 5.1

    Returns the distance from this vertex to a point defined by
    the vertex \a point.

    \sa distanceToLine()
*/
float QVector2D::distanceToPoint(const QVector2D& point) const
{
    return (*this - point).length();
}

/*!
    \since 5.1

    Returns the distance that this vertex is from a line defined
    by \a point and the unit vector \a direction.

    If \a direction is a null vector, then it does not define a line.
    In that case, the distance from \a point to this vertex is returned.

    \sa distanceToPoint()
*/
float QVector2D::distanceToLine
        (const QVector2D& point, const QVector2D& direction) const
{
    if (direction.isNull())
        return (*this - point).length();
    QVector2D p = point + dotProduct(*this - point, direction) * direction;
    return (*this - p).length();
}

/*!
    \fn QVector2D &QVector2D::operator+=(const QVector2D &vector)

    Adds the given \a vector to this vector and returns a reference to
    this vector.

    \sa operator-=()
*/

/*!
    \fn QVector2D &QVector2D::operator-=(const QVector2D &vector)

    Subtracts the given \a vector from this vector and returns a reference to
    this vector.

    \sa operator+=()
*/

/*!
    \fn QVector2D &QVector2D::operator*=(float factor)

    Multiplies this vector's coordinates by the given \a factor, and
    returns a reference to this vector.

    \sa operator/=()
*/

/*!
    \fn QVector2D &QVector2D::operator*=(const QVector2D &vector)

    Multiplies the components of this vector by the corresponding
    components in \a vector.
*/

/*!
    \fn QVector2D &QVector2D::operator/=(float divisor)

    Divides this vector's coordinates by the given \a divisor, and
    returns a reference to this vector.

    \sa operator*=()
*/

/*!
    Returns the dot product of \a v1 and \a v2.
*/
float QVector2D::dotProduct(const QVector2D& v1, const QVector2D& v2)
{
    return v1.xp * v2.xp + v1.yp * v2.yp;
}

/*!
    \fn bool operator==(const QVector2D &v1, const QVector2D &v2)
    \relates QVector2D

    Returns \c true if \a v1 is equal to \a v2; otherwise returns \c false.
    This operator uses an exact floating-point comparison.
*/

/*!
    \fn bool operator!=(const QVector2D &v1, const QVector2D &v2)
    \relates QVector2D

    Returns \c true if \a v1 is not equal to \a v2; otherwise returns \c false.
    This operator uses an exact floating-point comparison.
*/

/*!
    \fn const QVector2D operator+(const QVector2D &v1, const QVector2D &v2)
    \relates QVector2D

    Returns a QVector2D object that is the sum of the given vectors, \a v1
    and \a v2; each component is added separately.

    \sa QVector2D::operator+=()
*/

/*!
    \fn const QVector2D operator-(const QVector2D &v1, const QVector2D &v2)
    \relates QVector2D

    Returns a QVector2D object that is formed by subtracting \a v2 from \a v1;
    each component is subtracted separately.

    \sa QVector2D::operator-=()
*/

/*!
    \fn const QVector2D operator*(float factor, const QVector2D &vector)
    \relates QVector2D

    Returns a copy of the given \a vector,  multiplied by the given \a factor.

    \sa QVector2D::operator*=()
*/

/*!
    \fn const QVector2D operator*(const QVector2D &vector, float factor)
    \relates QVector2D

    Returns a copy of the given \a vector,  multiplied by the given \a factor.

    \sa QVector2D::operator*=()
*/

/*!
    \fn const QVector2D operator*(const QVector2D &v1, const QVector2D &v2)
    \relates QVector2D

    Multiplies the components of \a v1 by the corresponding
    components in \a v2.
*/

/*!
    \fn const QVector2D operator-(const QVector2D &vector)
    \relates QVector2D
    \overload

    Returns a QVector2D object that is formed by changing the sign of
    the components of the given \a vector.

    Equivalent to \c {QVector2D(0,0) - vector}.
*/

/*!
    \fn const QVector2D operator/(const QVector2D &vector, float divisor)
    \relates QVector2D

    Returns the QVector2D object formed by dividing all three components of
    the given \a vector by the given \a divisor.

    \sa QVector2D::operator/=()
*/

/*!
    \fn bool qFuzzyCompare(const QVector2D& v1, const QVector2D& v2)
    \relates QVector2D

    Returns \c true if \a v1 and \a v2 are equal, allowing for a small
    fuzziness factor for floating-point comparisons; false otherwise.
*/

#ifndef QT_NO_VECTOR3D

/*!
    Returns the 3D form of this 2D vector, with the z coordinate set to zero.

    \sa toVector4D(), toPoint()
*/
QVector3D QVector2D::toVector3D() const
{
    return QVector3D(xp, yp, 0.0f);
}

#endif

#ifndef QT_NO_VECTOR4D

/*!
    Returns the 4D form of this 2D vector, with the z and w coordinates set to zero.

    \sa toVector3D(), toPoint()
*/
QVector4D QVector2D::toVector4D() const
{
    return QVector4D(xp, yp, 0.0f, 0.0f);
}

#endif

/*!
    \fn QPoint QVector2D::toPoint() const

    Returns the QPoint form of this 2D vector.

    \sa toPointF(), toVector3D()
*/

/*!
    \fn QPointF QVector2D::toPointF() const

    Returns the QPointF form of this 2D vector.

    \sa toPoint(), toVector3D()
*/

/*!
    Returns the 2D vector as a QVariant.
*/
QVector2D::operator QVariant() const
{
    return QVariant(QVariant::Vector2D, this);
}

#ifndef QT_NO_DEBUG_STREAM

QDebug operator<<(QDebug dbg, const QVector2D &vector)
{
    dbg.nospace() << "QVector2D(" << vector.x() << ", " << vector.y() << ')';
    return dbg.space();
}

#endif

#ifndef QT_NO_DATASTREAM

/*!
    \fn QDataStream &operator<<(QDataStream &stream, const QVector2D &vector)
    \relates QVector2D

    Writes the given \a vector to the given \a stream and returns a
    reference to the stream.

    \sa {Serializing Qt Data Types}
*/

QDataStream &operator<<(QDataStream &stream, const QVector2D &vector)
{
    stream << vector.x() << vector.y();
    return stream;
}

/*!
    \fn QDataStream &operator>>(QDataStream &stream, QVector2D &vector)
    \relates QVector2D

    Reads a 2D vector from the given \a stream into the given \a vector
    and returns a reference to the stream.

    \sa {Serializing Qt Data Types}
*/

QDataStream &operator>>(QDataStream &stream, QVector2D &vector)
{
    float x, y;
    stream >> x;
    stream >> y;
    vector.setX(x);
    vector.setY(y);
    return stream;
}

#endif // QT_NO_DATASTREAM

#endif // QT_NO_VECTOR2D

QT_END_NAMESPACE
