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

#include "qquaternion.h"
#include <QtCore/qmath.h>
#include <QtCore/qvariant.h>
#include <QtCore/qdebug.h>

QT_BEGIN_NAMESPACE

#ifndef QT_NO_QUATERNION

/*!
    \class QQuaternion
    \brief The QQuaternion class represents a quaternion consisting of a vector and scalar.
    \since 4.6
    \ingroup painting-3D

    Quaternions are used to represent rotations in 3D space, and
    consist of a 3D rotation axis specified by the x, y, and z
    coordinates, and a scalar representing the rotation angle.
*/

/*!
    \fn QQuaternion::QQuaternion()

    Constructs an identity quaternion, i.e. with coordinates (1, 0, 0, 0).
*/

/*!
    \fn QQuaternion::QQuaternion(qreal scalar, qreal xpos, qreal ypos, qreal zpos)

    Constructs a quaternion with the vector (\a xpos, \a ypos, \a zpos)
    and \a scalar.
*/

#ifndef QT_NO_VECTOR3D

/*!
    \fn QQuaternion::QQuaternion(qreal scalar, const QVector3D& vector)

    Constructs a quaternion vector from the specified \a vector and
    \a scalar.

    \sa vector(), scalar()
*/

/*!
    \fn QVector3D QQuaternion::vector() const

    Returns the vector component of this quaternion.

    \sa setVector(), scalar()
*/

/*!
    \fn void QQuaternion::setVector(const QVector3D& vector)

    Sets the vector component of this quaternion to \a vector.

    \sa vector(), setScalar()
*/

#endif

/*!
    \fn void QQuaternion::setVector(qreal x, qreal y, qreal z)

    Sets the vector component of this quaternion to (\a x, \a y, \a z).

    \sa vector(), setScalar()
*/

#ifndef QT_NO_VECTOR4D

/*!
    \fn QQuaternion::QQuaternion(const QVector4D& vector)

    Constructs a quaternion from the components of \a vector.
*/

/*!
    \fn QVector4D QQuaternion::toVector4D() const

    Returns this quaternion as a 4D vector.
*/

#endif

/*!
    \fn bool QQuaternion::isNull() const

    Returns true if the x, y, z, and scalar components of this
    quaternion are set to 0.0; otherwise returns false.
*/

/*!
    \fn bool QQuaternion::isIdentity() const

    Returns true if the x, y, and z components of this
    quaternion are set to 0.0, and the scalar component is set
    to 1.0; otherwise returns false.
*/

/*!
    \fn qreal QQuaternion::x() const

    Returns the x coordinate of this quaternion's vector.

    \sa setX(), y(), z(), scalar()
*/

/*!
    \fn qreal QQuaternion::y() const

    Returns the y coordinate of this quaternion's vector.

    \sa setY(), x(), z(), scalar()
*/

/*!
    \fn qreal QQuaternion::z() const

    Returns the z coordinate of this quaternion's vector.

    \sa setZ(), x(), y(), scalar()
*/

/*!
    \fn qreal QQuaternion::scalar() const

    Returns the scalar component of this quaternion.

    \sa setScalar(), x(), y(), z()
*/

/*!
    \fn void QQuaternion::setX(qreal x)

    Sets the x coordinate of this quaternion's vector to the given
    \a x coordinate.

    \sa x(), setY(), setZ(), setScalar()
*/

/*!
    \fn void QQuaternion::setY(qreal y)

    Sets the y coordinate of this quaternion's vector to the given
    \a y coordinate.

    \sa y(), setX(), setZ(), setScalar()
*/

/*!
    \fn void QQuaternion::setZ(qreal z)

    Sets the z coordinate of this quaternion's vector to the given
    \a z coordinate.

    \sa z(), setX(), setY(), setScalar()
*/

/*!
    \fn void QQuaternion::setScalar(qreal scalar)

    Sets the scalar component of this quaternion to \a scalar.

    \sa scalar(), setX(), setY(), setZ()
*/

/*!
    Returns the length of the quaternion.  This is also called the "norm".

    \sa lengthSquared(), normalized()
*/
qreal QQuaternion::length() const
{
    return qSqrt(xp * xp + yp * yp + zp * zp + wp * wp);
}

/*!
    Returns the squared length of the quaternion.

    \sa length()
*/
qreal QQuaternion::lengthSquared() const
{
    return xp * xp + yp * yp + zp * zp + wp * wp;
}

/*!
    Returns the normalized unit form of this quaternion.

    If this quaternion is null, then a null quaternion is returned.
    If the length of the quaternion is very close to 1, then the quaternion
    will be returned as-is.  Otherwise the normalized form of the
    quaternion of length 1 will be returned.

    \sa length(), normalize()
*/
QQuaternion QQuaternion::normalized() const
{
    // Need some extra precision if the length is very small.
    double len = double(xp) * double(xp) +
                 double(yp) * double(yp) +
                 double(zp) * double(zp) +
                 double(wp) * double(wp);
    if (qFuzzyIsNull(len - 1.0f))
        return *this;
    else if (!qFuzzyIsNull(len))
        return *this / qSqrt(len);
    else
        return QQuaternion(0.0f, 0.0f, 0.0f, 0.0f);
}

/*!
    Normalizes the currect quaternion in place.  Nothing happens if this
    is a null quaternion or the length of the quaternion is very close to 1.

    \sa length(), normalized()
*/
void QQuaternion::normalize()
{
    // Need some extra precision if the length is very small.
    double len = double(xp) * double(xp) +
                 double(yp) * double(yp) +
                 double(zp) * double(zp) +
                 double(wp) * double(wp);
    if (qFuzzyIsNull(len - 1.0f) || qFuzzyIsNull(len))
        return;

    len = qSqrt(len);

    xp /= len;
    yp /= len;
    zp /= len;
    wp /= len;
}

/*!
    \fn QQuaternion QQuaternion::conjugate() const

    Returns the conjugate of this quaternion, which is
    (-x, -y, -z, scalar).
*/

/*!
    Rotates \a vector with this quaternion to produce a new vector
    in 3D space.  The following code:

    \code
    QVector3D result = q.rotatedVector(vector);
    \endcode

    is equivalent to the following:

    \code
    QVector3D result = (q * QQuaternion(0, vector) * q.conjugate()).vector();
    \endcode
*/
QVector3D QQuaternion::rotatedVector(const QVector3D& vector) const
{
    return (*this * QQuaternion(0, vector) * conjugate()).vector();
}

/*!
    \fn QQuaternion &QQuaternion::operator+=(const QQuaternion &quaternion)

    Adds the given \a quaternion to this quaternion and returns a reference to
    this quaternion.

    \sa operator-=()
*/

/*!
    \fn QQuaternion &QQuaternion::operator-=(const QQuaternion &quaternion)

    Subtracts the given \a quaternion from this quaternion and returns a
    reference to this quaternion.

    \sa operator+=()
*/

/*!
    \fn QQuaternion &QQuaternion::operator*=(qreal factor)

    Multiplies this quaternion's components by the given \a factor, and
    returns a reference to this quaternion.

    \sa operator/=()
*/

/*!
    \fn QQuaternion &QQuaternion::operator*=(const QQuaternion &quaternion)

    Multiplies this quaternion by \a quaternion and returns a reference
    to this quaternion.
*/

/*!
    \fn QQuaternion &QQuaternion::operator/=(qreal divisor)

    Divides this quaternion's components by the given \a divisor, and
    returns a reference to this quaternion.

    \sa operator*=()
*/

#ifndef QT_NO_VECTOR3D

/*!
    Creates a normalized quaternion that corresponds to rotating through
    \a angle degrees about the specified 3D \a axis.
*/
QQuaternion QQuaternion::fromAxisAndAngle(const QVector3D& axis, qreal angle)
{
    // Algorithm from:
    // http://www.j3d.org/matrix_faq/matrfaq_latest.html#Q56
    // We normalize the result just in case the values are close
    // to zero, as suggested in the above FAQ.
    qreal a = (angle / 2.0f) * M_PI / 180.0f;
    qreal s = qSin(a);
    qreal c = qCos(a);
    QVector3D ax = axis.normalized();
    return QQuaternion(c, ax.x() * s, ax.y() * s, ax.z() * s).normalized();
}

#endif

/*!
    Creates a normalized quaternion that corresponds to rotating through
    \a angle degrees about the 3D axis (\a x, \a y, \a z).
*/
QQuaternion QQuaternion::fromAxisAndAngle
        (qreal x, qreal y, qreal z, qreal angle)
{
    qreal length = qSqrt(x * x + y * y + z * z);
    if (!qFuzzyIsNull(length - 1.0f) && !qFuzzyIsNull(length)) {
        x /= length;
        y /= length;
        z /= length;
    }
    qreal a = (angle / 2.0f) * M_PI / 180.0f;
    qreal s = qSin(a);
    qreal c = qCos(a);
    return QQuaternion(c, x * s, y * s, z * s).normalized();
}

/*!
    \fn bool operator==(const QQuaternion &q1, const QQuaternion &q2)
    \relates QQuaternion

    Returns true if \a q1 is equal to \a q2; otherwise returns false.
    This operator uses an exact floating-point comparison.
*/

/*!
    \fn bool operator!=(const QQuaternion &q1, const QQuaternion &q2)
    \relates QQuaternion

    Returns true if \a q1 is not equal to \a q2; otherwise returns false.
    This operator uses an exact floating-point comparison.
*/

/*!
    \fn const QQuaternion operator+(const QQuaternion &q1, const QQuaternion &q2)
    \relates QQuaternion

    Returns a QQuaternion object that is the sum of the given quaternions,
    \a q1 and \a q2; each component is added separately.

    \sa QQuaternion::operator+=()
*/

/*!
    \fn const QQuaternion operator-(const QQuaternion &q1, const QQuaternion &q2)
    \relates QQuaternion

    Returns a QQuaternion object that is formed by subtracting
    \a q2 from \a q1; each component is subtracted separately.

    \sa QQuaternion::operator-=()
*/

/*!
    \fn const QQuaternion operator*(qreal factor, const QQuaternion &quaternion)
    \relates QQuaternion

    Returns a copy of the given \a quaternion,  multiplied by the
    given \a factor.

    \sa QQuaternion::operator*=()
*/

/*!
    \fn const QQuaternion operator*(const QQuaternion &quaternion, qreal factor)
    \relates QQuaternion

    Returns a copy of the given \a quaternion,  multiplied by the
    given \a factor.

    \sa QQuaternion::operator*=()
*/

/*!
    \fn const QQuaternion operator*(const QQuaternion &q1, const QQuaternion& q2)
    \relates QQuaternion

    Multiplies \a q1 and \a q2 using quaternion multiplication.
    The result corresponds to applying both of the rotations specified
    by \a q1 and \a q2.

    \sa QQuaternion::operator*=()
*/

/*!
    \fn const QQuaternion operator-(const QQuaternion &quaternion)
    \relates QQuaternion
    \overload

    Returns a QQuaternion object that is formed by changing the sign of
    all three components of the given \a quaternion.

    Equivalent to \c {QQuaternion(0,0,0,0) - quaternion}.
*/

/*!
    \fn const QQuaternion operator/(const QQuaternion &quaternion, qreal divisor)
    \relates QQuaternion

    Returns the QQuaternion object formed by dividing all components of
    the given \a quaternion by the given \a divisor.

    \sa QQuaternion::operator/=()
*/

/*!
    \fn bool qFuzzyCompare(const QQuaternion& q1, const QQuaternion& q2)
    \relates QQuaternion

    Returns true if \a q1 and \a q2 are equal, allowing for a small
    fuzziness factor for floating-point comparisons; false otherwise.
*/

/*!
    Interpolates along the shortest spherical path between the
    rotational positions \a q1 and \a q2.  The value \a t should
    be between 0 and 1, indicating the spherical distance to travel
    between \a q1 and \a q2.

    If \a t is less than or equal to 0, then \a q1 will be returned.
    If \a t is greater than or equal to 1, then \a q2 will be returned.

    \sa nlerp()
*/
QQuaternion QQuaternion::slerp
    (const QQuaternion& q1, const QQuaternion& q2, qreal t)
{
    // Handle the easy cases first.
    if (t <= 0.0f)
        return q1;
    else if (t >= 1.0f)
        return q2;

    // Determine the angle between the two quaternions.
    QQuaternion q2b;
    qreal dot;
    dot = q1.xp * q2.xp + q1.yp * q2.yp + q1.zp * q2.zp + q1.wp * q2.wp;
    if (dot >= 0.0f) {
        q2b = q2;
    } else {
        q2b = -q2;
        dot = -dot;
    }

    // Get the scale factors.  If they are too small,
    // then revert to simple linear interpolation.
    qreal factor1 = 1.0f - t;
    qreal factor2 = t;
    if ((1.0f - dot) > 0.0000001) {
        qreal angle = qreal(qAcos(dot));
        qreal sinOfAngle = qreal(qSin(angle));
        if (sinOfAngle > 0.0000001) {
            factor1 = qreal(qSin((1.0f - t) * angle)) / sinOfAngle;
            factor2 = qreal(qSin(t * angle)) / sinOfAngle;
        }
    }

    // Construct the result quaternion.
    return q1 * factor1 + q2b * factor2;
}

/*!
    Interpolates along the shortest linear path between the rotational
    positions \a q1 and \a q2.  The value \a t should be between 0 and 1,
    indicating the distance to travel between \a q1 and \a q2.
    The result will be normalized().

    If \a t is less than or equal to 0, then \a q1 will be returned.
    If \a t is greater than or equal to 1, then \a q2 will be returned.

    The nlerp() function is typically faster than slerp() and will
    give approximate results to spherical interpolation that are
    good enough for some applications.

    \sa slerp()
*/
QQuaternion QQuaternion::nlerp
    (const QQuaternion& q1, const QQuaternion& q2, qreal t)
{
    // Handle the easy cases first.
    if (t <= 0.0f)
        return q1;
    else if (t >= 1.0f)
        return q2;

    // Determine the angle between the two quaternions.
    QQuaternion q2b;
    qreal dot;
    dot = q1.xp * q2.xp + q1.yp * q2.yp + q1.zp * q2.zp + q1.wp * q2.wp;
    if (dot >= 0.0f)
        q2b = q2;
    else
        q2b = -q2;

    // Perform the linear interpolation.
    return (q1 * (1.0f - t) + q2b * t).normalized();
}

/*!
    Returns the quaternion as a QVariant.
*/
QQuaternion::operator QVariant() const
{
    return QVariant(QVariant::Quaternion, this);
}

#ifndef QT_NO_DEBUG_STREAM

QDebug operator<<(QDebug dbg, const QQuaternion &q)
{
    dbg.nospace() << "QQuaternion(scalar:" << q.scalar()
        << ", vector:(" << q.x() << ", "
        << q.y() << ", " << q.z() << "))";
    return dbg.space();
}

#endif

#ifndef QT_NO_DATASTREAM

/*!
    \fn QDataStream &operator<<(QDataStream &stream, const QQuaternion &quaternion)
    \relates QQuaternion

    Writes the given \a quaternion to the given \a stream and returns a
    reference to the stream.

    \sa {Serializing Qt Data Types}
*/

QDataStream &operator<<(QDataStream &stream, const QQuaternion &quaternion)
{
    stream << double(quaternion.scalar()) << double(quaternion.x())
           << double(quaternion.y()) << double(quaternion.z());
    return stream;
}

/*!
    \fn QDataStream &operator>>(QDataStream &stream, QQuaternion &quaternion)
    \relates QQuaternion

    Reads a quaternion from the given \a stream into the given \a quaternion
    and returns a reference to the stream.

    \sa {Serializing Qt Data Types}
*/

QDataStream &operator>>(QDataStream &stream, QQuaternion &quaternion)
{
    double scalar, x, y, z;
    stream >> scalar;
    stream >> x;
    stream >> y;
    stream >> z;
    quaternion.setScalar(qreal(scalar));
    quaternion.setX(qreal(x));
    quaternion.setY(qreal(y));
    quaternion.setZ(qreal(z));
    return stream;
}

#endif // QT_NO_DATASTREAM

#endif

QT_END_NAMESPACE
