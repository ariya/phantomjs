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

#ifndef QVECTOR3D_H
#define QVECTOR3D_H

#include <QtCore/qpoint.h>
#include <QtCore/qmetatype.h>

QT_BEGIN_NAMESPACE


class QMatrix4x4;
class QVector2D;
class QVector4D;

#ifndef QT_NO_VECTOR3D

class Q_GUI_EXPORT QVector3D
{
public:
    Q_DECL_CONSTEXPR QVector3D();
    Q_DECL_CONSTEXPR QVector3D(float xpos, float ypos, float zpos) : xp(xpos), yp(ypos), zp(zpos) {}

    Q_DECL_CONSTEXPR explicit QVector3D(const QPoint& point);
    Q_DECL_CONSTEXPR explicit QVector3D(const QPointF& point);
#ifndef QT_NO_VECTOR2D
    QVector3D(const QVector2D& vector);
    QVector3D(const QVector2D& vector, float zpos);
#endif
#ifndef QT_NO_VECTOR4D
    explicit QVector3D(const QVector4D& vector);
#endif

    bool isNull() const;

    Q_DECL_CONSTEXPR float x() const;
    Q_DECL_CONSTEXPR float y() const;
    Q_DECL_CONSTEXPR float z() const;

    void setX(float x);
    void setY(float y);
    void setZ(float z);

    float &operator[](int i);
    float operator[](int i) const;

    float length() const;
    float lengthSquared() const;

    QVector3D normalized() const;
    void normalize();

    QVector3D &operator+=(const QVector3D &vector);
    QVector3D &operator-=(const QVector3D &vector);
    QVector3D &operator*=(float factor);
    QVector3D &operator*=(const QVector3D& vector);
    QVector3D &operator/=(float divisor);

    static float dotProduct(const QVector3D& v1, const QVector3D& v2); //In Qt 6 convert to inline and constexpr
    static QVector3D crossProduct(const QVector3D& v1, const QVector3D& v2); //in Qt 6 convert to inline and constexpr

    static QVector3D normal(const QVector3D& v1, const QVector3D& v2);
    static QVector3D normal
        (const QVector3D& v1, const QVector3D& v2, const QVector3D& v3);

    float distanceToPoint(const QVector3D& point) const;
    float distanceToPlane(const QVector3D& plane, const QVector3D& normal) const;
    float distanceToPlane(const QVector3D& plane1, const QVector3D& plane2, const QVector3D& plane3) const;
    float distanceToLine(const QVector3D& point, const QVector3D& direction) const;

    Q_DECL_CONSTEXPR friend inline bool operator==(const QVector3D &v1, const QVector3D &v2);
    Q_DECL_CONSTEXPR friend inline bool operator!=(const QVector3D &v1, const QVector3D &v2);
    Q_DECL_CONSTEXPR friend inline const QVector3D operator+(const QVector3D &v1, const QVector3D &v2);
    Q_DECL_CONSTEXPR friend inline const QVector3D operator-(const QVector3D &v1, const QVector3D &v2);
    Q_DECL_CONSTEXPR friend inline const QVector3D operator*(float factor, const QVector3D &vector);
    Q_DECL_CONSTEXPR friend inline const QVector3D operator*(const QVector3D &vector, float factor);
    Q_DECL_CONSTEXPR friend const QVector3D operator*(const QVector3D &v1, const QVector3D& v2);
    Q_DECL_CONSTEXPR friend inline const QVector3D operator-(const QVector3D &vector);
    Q_DECL_CONSTEXPR friend inline const QVector3D operator/(const QVector3D &vector, float divisor);

    Q_DECL_CONSTEXPR friend inline bool qFuzzyCompare(const QVector3D& v1, const QVector3D& v2);

#ifndef QT_NO_VECTOR2D
    QVector2D toVector2D() const;
#endif
#ifndef QT_NO_VECTOR4D
    QVector4D toVector4D() const;
#endif

    Q_DECL_CONSTEXPR QPoint toPoint() const;
    Q_DECL_CONSTEXPR QPointF toPointF() const;

    operator QVariant() const;

private:
    float xp, yp, zp;

    friend class QVector2D;
    friend class QVector4D;
#ifndef QT_NO_MATRIX4X4
    friend QVector3D operator*(const QVector3D& vector, const QMatrix4x4& matrix);
    friend QVector3D operator*(const QMatrix4x4& matrix, const QVector3D& vector);
#endif
};

Q_DECLARE_TYPEINFO(QVector3D, Q_MOVABLE_TYPE);

Q_DECL_CONSTEXPR inline QVector3D::QVector3D() : xp(0.0f), yp(0.0f), zp(0.0f) {}

Q_DECL_CONSTEXPR inline QVector3D::QVector3D(const QPoint& point) : xp(point.x()), yp(point.y()), zp(0.0f) {}

Q_DECL_CONSTEXPR inline QVector3D::QVector3D(const QPointF& point) : xp(point.x()), yp(point.y()), zp(0.0f) {}

inline bool QVector3D::isNull() const
{
    return qIsNull(xp) && qIsNull(yp) && qIsNull(zp);
}

Q_DECL_CONSTEXPR inline float QVector3D::x() const { return xp; }
Q_DECL_CONSTEXPR inline float QVector3D::y() const { return yp; }
Q_DECL_CONSTEXPR inline float QVector3D::z() const { return zp; }

inline void QVector3D::setX(float aX) { xp = aX; }
inline void QVector3D::setY(float aY) { yp = aY; }
inline void QVector3D::setZ(float aZ) { zp = aZ; }

inline float &QVector3D::operator[](int i)
{
    Q_ASSERT(uint(i) < 3u);
    return *(&xp + i);
}

inline float QVector3D::operator[](int i) const
{
    Q_ASSERT(uint(i) < 3u);
    return *(&xp + i);
}

inline QVector3D &QVector3D::operator+=(const QVector3D &vector)
{
    xp += vector.xp;
    yp += vector.yp;
    zp += vector.zp;
    return *this;
}

inline QVector3D &QVector3D::operator-=(const QVector3D &vector)
{
    xp -= vector.xp;
    yp -= vector.yp;
    zp -= vector.zp;
    return *this;
}

inline QVector3D &QVector3D::operator*=(float factor)
{
    xp *= factor;
    yp *= factor;
    zp *= factor;
    return *this;
}

inline QVector3D &QVector3D::operator*=(const QVector3D& vector)
{
    xp *= vector.xp;
    yp *= vector.yp;
    zp *= vector.zp;
    return *this;
}

inline QVector3D &QVector3D::operator/=(float divisor)
{
    xp /= divisor;
    yp /= divisor;
    zp /= divisor;
    return *this;
}

Q_DECL_CONSTEXPR inline bool operator==(const QVector3D &v1, const QVector3D &v2)
{
    return v1.xp == v2.xp && v1.yp == v2.yp && v1.zp == v2.zp;
}

Q_DECL_CONSTEXPR inline bool operator!=(const QVector3D &v1, const QVector3D &v2)
{
    return v1.xp != v2.xp || v1.yp != v2.yp || v1.zp != v2.zp;
}

Q_DECL_CONSTEXPR inline const QVector3D operator+(const QVector3D &v1, const QVector3D &v2)
{
    return QVector3D(v1.xp + v2.xp, v1.yp + v2.yp, v1.zp + v2.zp);
}

Q_DECL_CONSTEXPR inline const QVector3D operator-(const QVector3D &v1, const QVector3D &v2)
{
    return QVector3D(v1.xp - v2.xp, v1.yp - v2.yp, v1.zp - v2.zp);
}

Q_DECL_CONSTEXPR inline const QVector3D operator*(float factor, const QVector3D &vector)
{
    return QVector3D(vector.xp * factor, vector.yp * factor, vector.zp * factor);
}

Q_DECL_CONSTEXPR inline const QVector3D operator*(const QVector3D &vector, float factor)
{
    return QVector3D(vector.xp * factor, vector.yp * factor, vector.zp * factor);
}

Q_DECL_CONSTEXPR inline const QVector3D operator*(const QVector3D &v1, const QVector3D& v2)
{
    return QVector3D(v1.xp * v2.xp, v1.yp * v2.yp, v1.zp * v2.zp);
}

Q_DECL_CONSTEXPR inline const QVector3D operator-(const QVector3D &vector)
{
    return QVector3D(-vector.xp, -vector.yp, -vector.zp);
}

Q_DECL_CONSTEXPR inline const QVector3D operator/(const QVector3D &vector, float divisor)
{
    return QVector3D(vector.xp / divisor, vector.yp / divisor, vector.zp / divisor);
}

Q_DECL_CONSTEXPR inline bool qFuzzyCompare(const QVector3D& v1, const QVector3D& v2)
{
    return qFuzzyCompare(v1.xp, v2.xp) &&
           qFuzzyCompare(v1.yp, v2.yp) &&
           qFuzzyCompare(v1.zp, v2.zp);
}

Q_DECL_CONSTEXPR inline QPoint QVector3D::toPoint() const
{
    return QPoint(qRound(xp), qRound(yp));
}

Q_DECL_CONSTEXPR inline QPointF QVector3D::toPointF() const
{
    return QPointF(qreal(xp), qreal(yp));
}

#ifndef QT_NO_DEBUG_STREAM
Q_GUI_EXPORT QDebug operator<<(QDebug dbg, const QVector3D &vector);
#endif

#ifndef QT_NO_DATASTREAM
Q_GUI_EXPORT QDataStream &operator<<(QDataStream &, const QVector3D &);
Q_GUI_EXPORT QDataStream &operator>>(QDataStream &, QVector3D &);
#endif

#endif

QT_END_NAMESPACE

#endif
