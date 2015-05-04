/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtGui module of the Qt Toolkit.
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

#ifndef QQUATERNION_H
#define QQUATERNION_H

#include <QtGui/qvector3d.h>
#include <QtGui/qvector4d.h>

QT_BEGIN_NAMESPACE


#ifndef QT_NO_QUATERNION

class QMatrix4x4;
class QVariant;

class Q_GUI_EXPORT QQuaternion
{
public:
    QQuaternion();
    QQuaternion(float scalar, float xpos, float ypos, float zpos);
#ifndef QT_NO_VECTOR3D
    QQuaternion(float scalar, const QVector3D& vector);
#endif
#ifndef QT_NO_VECTOR4D
    explicit QQuaternion(const QVector4D& vector);
#endif

    bool isNull() const;
    bool isIdentity() const;

#ifndef QT_NO_VECTOR3D
    QVector3D vector() const;
    void setVector(const QVector3D& vector);
#endif
    void setVector(float x, float y, float z);

    float x() const;
    float y() const;
    float z() const;
    float scalar() const;

    void setX(float x);
    void setY(float y);
    void setZ(float z);
    void setScalar(float scalar);

    float length() const;
    float lengthSquared() const;

    QQuaternion normalized() const;
    void normalize();

    QQuaternion conjugate() const;

    QVector3D rotatedVector(const QVector3D& vector) const;

    QQuaternion &operator+=(const QQuaternion &quaternion);
    QQuaternion &operator-=(const QQuaternion &quaternion);
    QQuaternion &operator*=(float factor);
    QQuaternion &operator*=(const QQuaternion &quaternion);
    QQuaternion &operator/=(float divisor);

    friend inline bool operator==(const QQuaternion &q1, const QQuaternion &q2);
    friend inline bool operator!=(const QQuaternion &q1, const QQuaternion &q2);
    friend inline const QQuaternion operator+(const QQuaternion &q1, const QQuaternion &q2);
    friend inline const QQuaternion operator-(const QQuaternion &q1, const QQuaternion &q2);
    friend inline const QQuaternion operator*(float factor, const QQuaternion &quaternion);
    friend inline const QQuaternion operator*(const QQuaternion &quaternion, float factor);
    friend inline const QQuaternion operator*(const QQuaternion &q1, const QQuaternion& q2);
    friend inline const QQuaternion operator-(const QQuaternion &quaternion);
    friend inline const QQuaternion operator/(const QQuaternion &quaternion, float divisor);

    friend inline bool qFuzzyCompare(const QQuaternion& q1, const QQuaternion& q2);

#ifndef QT_NO_VECTOR4D
    QVector4D toVector4D() const;
#endif

    operator QVariant() const;

#ifndef QT_NO_VECTOR3D
    static QQuaternion fromAxisAndAngle(const QVector3D& axis, float angle);
#endif
    static QQuaternion fromAxisAndAngle
            (float x, float y, float z, float angle);

    static QQuaternion slerp
        (const QQuaternion& q1, const QQuaternion& q2, float t);
    static QQuaternion nlerp
        (const QQuaternion& q1, const QQuaternion& q2, float t);

private:
    float wp, xp, yp, zp;
};

Q_DECLARE_TYPEINFO(QQuaternion, Q_MOVABLE_TYPE);

inline QQuaternion::QQuaternion() : wp(1.0f), xp(0.0f), yp(0.0f), zp(0.0f) {}

inline QQuaternion::QQuaternion(float aScalar, float xpos, float ypos, float zpos) : wp(aScalar), xp(xpos), yp(ypos), zp(zpos) {}


inline bool QQuaternion::isNull() const
{
    return qIsNull(xp) && qIsNull(yp) && qIsNull(zp) && qIsNull(wp);
}

inline bool QQuaternion::isIdentity() const
{
    return qIsNull(xp) && qIsNull(yp) && qIsNull(zp) && wp == 1.0f;
}

inline float QQuaternion::x() const { return xp; }
inline float QQuaternion::y() const { return yp; }
inline float QQuaternion::z() const { return zp; }
inline float QQuaternion::scalar() const { return wp; }

inline void QQuaternion::setX(float aX) { xp = aX; }
inline void QQuaternion::setY(float aY) { yp = aY; }
inline void QQuaternion::setZ(float aZ) { zp = aZ; }
inline void QQuaternion::setScalar(float aScalar) { wp = aScalar; }

inline QQuaternion QQuaternion::conjugate() const
{
    return QQuaternion(wp, -xp, -yp, -zp);
}

inline QQuaternion &QQuaternion::operator+=(const QQuaternion &quaternion)
{
    xp += quaternion.xp;
    yp += quaternion.yp;
    zp += quaternion.zp;
    wp += quaternion.wp;
    return *this;
}

inline QQuaternion &QQuaternion::operator-=(const QQuaternion &quaternion)
{
    xp -= quaternion.xp;
    yp -= quaternion.yp;
    zp -= quaternion.zp;
    wp -= quaternion.wp;
    return *this;
}

inline QQuaternion &QQuaternion::operator*=(float factor)
{
    xp *= factor;
    yp *= factor;
    zp *= factor;
    wp *= factor;
    return *this;
}

inline const QQuaternion operator*(const QQuaternion &q1, const QQuaternion& q2)
{
    float ww = (q1.zp + q1.xp) * (q2.xp + q2.yp);
    float yy = (q1.wp - q1.yp) * (q2.wp + q2.zp);
    float zz = (q1.wp + q1.yp) * (q2.wp - q2.zp);
    float xx = ww + yy + zz;
    float qq = 0.5 * (xx + (q1.zp - q1.xp) * (q2.xp - q2.yp));

    float w = qq - ww + (q1.zp - q1.yp) * (q2.yp - q2.zp);
    float x = qq - xx + (q1.xp + q1.wp) * (q2.xp + q2.wp);
    float y = qq - yy + (q1.wp - q1.xp) * (q2.yp + q2.zp);
    float z = qq - zz + (q1.zp + q1.yp) * (q2.wp - q2.xp);

    return QQuaternion(w, x, y, z);
}

inline QQuaternion &QQuaternion::operator*=(const QQuaternion &quaternion)
{
    *this = *this * quaternion;
    return *this;
}

inline QQuaternion &QQuaternion::operator/=(float divisor)
{
    xp /= divisor;
    yp /= divisor;
    zp /= divisor;
    wp /= divisor;
    return *this;
}

inline bool operator==(const QQuaternion &q1, const QQuaternion &q2)
{
    return q1.xp == q2.xp && q1.yp == q2.yp && q1.zp == q2.zp && q1.wp == q2.wp;
}

inline bool operator!=(const QQuaternion &q1, const QQuaternion &q2)
{
    return q1.xp != q2.xp || q1.yp != q2.yp || q1.zp != q2.zp || q1.wp != q2.wp;
}

inline const QQuaternion operator+(const QQuaternion &q1, const QQuaternion &q2)
{
    return QQuaternion(q1.wp + q2.wp, q1.xp + q2.xp, q1.yp + q2.yp, q1.zp + q2.zp);
}

inline const QQuaternion operator-(const QQuaternion &q1, const QQuaternion &q2)
{
    return QQuaternion(q1.wp - q2.wp, q1.xp - q2.xp, q1.yp - q2.yp, q1.zp - q2.zp);
}

inline const QQuaternion operator*(float factor, const QQuaternion &quaternion)
{
    return QQuaternion(quaternion.wp * factor, quaternion.xp * factor, quaternion.yp * factor, quaternion.zp * factor);
}

inline const QQuaternion operator*(const QQuaternion &quaternion, float factor)
{
    return QQuaternion(quaternion.wp * factor, quaternion.xp * factor, quaternion.yp * factor, quaternion.zp * factor);
}

inline const QQuaternion operator-(const QQuaternion &quaternion)
{
    return QQuaternion(-quaternion.wp, -quaternion.xp, -quaternion.yp, -quaternion.zp);
}

inline const QQuaternion operator/(const QQuaternion &quaternion, float divisor)
{
    return QQuaternion(quaternion.wp / divisor, quaternion.xp / divisor, quaternion.yp / divisor, quaternion.zp / divisor);
}

inline bool qFuzzyCompare(const QQuaternion& q1, const QQuaternion& q2)
{
    return qFuzzyCompare(q1.xp, q2.xp) &&
           qFuzzyCompare(q1.yp, q2.yp) &&
           qFuzzyCompare(q1.zp, q2.zp) &&
           qFuzzyCompare(q1.wp, q2.wp);
}

#ifndef QT_NO_VECTOR3D

inline QQuaternion::QQuaternion(float aScalar, const QVector3D& aVector)
    : wp(aScalar), xp(aVector.x()), yp(aVector.y()), zp(aVector.z()) {}

inline void QQuaternion::setVector(const QVector3D& aVector)
{
    xp = aVector.x();
    yp = aVector.y();
    zp = aVector.z();
}

inline QVector3D QQuaternion::vector() const
{
    return QVector3D(xp, yp, zp);
}

#endif

inline void QQuaternion::setVector(float aX, float aY, float aZ)
{
    xp = aX;
    yp = aY;
    zp = aZ;
}

#ifndef QT_NO_VECTOR4D

inline QQuaternion::QQuaternion(const QVector4D& aVector)
    : wp(aVector.w()), xp(aVector.x()), yp(aVector.y()), zp(aVector.z()) {}

inline QVector4D QQuaternion::toVector4D() const
{
    return QVector4D(xp, yp, zp, wp);
}

#endif

#ifndef QT_NO_DEBUG_STREAM
Q_GUI_EXPORT QDebug operator<<(QDebug dbg, const QQuaternion &q);
#endif

#ifndef QT_NO_DATASTREAM
Q_GUI_EXPORT QDataStream &operator<<(QDataStream &, const QQuaternion &);
Q_GUI_EXPORT QDataStream &operator>>(QDataStream &, QQuaternion &);
#endif

#endif

QT_END_NAMESPACE

#endif
