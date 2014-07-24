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

#ifndef QVECTOR4D_H
#define QVECTOR4D_H

#include <QtCore/qpoint.h>
#include <QtCore/qmetatype.h>

QT_BEGIN_NAMESPACE


class QMatrix4x4;
class QVector2D;
class QVector3D;

#ifndef QT_NO_VECTOR4D

class Q_GUI_EXPORT QVector4D
{
public:
    Q_DECL_CONSTEXPR QVector4D();
    Q_DECL_CONSTEXPR QVector4D(float xpos, float ypos, float zpos, float wpos);
    Q_DECL_CONSTEXPR explicit QVector4D(const QPoint& point);
    Q_DECL_CONSTEXPR explicit QVector4D(const QPointF& point);
#ifndef QT_NO_VECTOR2D
    QVector4D(const QVector2D& vector);
    QVector4D(const QVector2D& vector, float zpos, float wpos);
#endif
#ifndef QT_NO_VECTOR3D
    QVector4D(const QVector3D& vector);
    QVector4D(const QVector3D& vector, float wpos);
#endif

    bool isNull() const;

    Q_DECL_CONSTEXPR float x() const;
    Q_DECL_CONSTEXPR float y() const;
    Q_DECL_CONSTEXPR float z() const;
    Q_DECL_CONSTEXPR float w() const;

    void setX(float x);
    void setY(float y);
    void setZ(float z);
    void setW(float w);

    float &operator[](int i);
    float operator[](int i) const;

    float length() const;
    float lengthSquared() const; //In Qt 6 convert to inline and constexpr

    QVector4D normalized() const;
    void normalize();

    QVector4D &operator+=(const QVector4D &vector);
    QVector4D &operator-=(const QVector4D &vector);
    QVector4D &operator*=(float factor);
    QVector4D &operator*=(const QVector4D &vector);
    QVector4D &operator/=(float divisor);

    static float dotProduct(const QVector4D& v1, const QVector4D& v2); //In Qt 6 convert to inline and constexpr

    Q_DECL_CONSTEXPR friend inline bool operator==(const QVector4D &v1, const QVector4D &v2);
    Q_DECL_CONSTEXPR friend inline bool operator!=(const QVector4D &v1, const QVector4D &v2);
    Q_DECL_CONSTEXPR friend inline const QVector4D operator+(const QVector4D &v1, const QVector4D &v2);
    Q_DECL_CONSTEXPR friend inline const QVector4D operator-(const QVector4D &v1, const QVector4D &v2);
    Q_DECL_CONSTEXPR friend inline const QVector4D operator*(float factor, const QVector4D &vector);
    Q_DECL_CONSTEXPR friend inline const QVector4D operator*(const QVector4D &vector, float factor);
    Q_DECL_CONSTEXPR friend inline const QVector4D operator*(const QVector4D &v1, const QVector4D& v2);
    Q_DECL_CONSTEXPR friend inline const QVector4D operator-(const QVector4D &vector);
    Q_DECL_CONSTEXPR friend inline const QVector4D operator/(const QVector4D &vector, float divisor);

    Q_DECL_CONSTEXPR friend inline bool qFuzzyCompare(const QVector4D& v1, const QVector4D& v2);

#ifndef QT_NO_VECTOR2D
    QVector2D toVector2D() const;
    QVector2D toVector2DAffine() const;
#endif
#ifndef QT_NO_VECTOR3D
    QVector3D toVector3D() const;
    QVector3D toVector3DAffine() const;
#endif

    Q_DECL_CONSTEXPR QPoint toPoint() const;
    Q_DECL_CONSTEXPR QPointF toPointF() const;

    operator QVariant() const;

private:
    float xp, yp, zp, wp;

    friend class QVector2D;
    friend class QVector3D;
#ifndef QT_NO_MATRIX4X4
    friend QVector4D operator*(const QVector4D& vector, const QMatrix4x4& matrix);
    friend QVector4D operator*(const QMatrix4x4& matrix, const QVector4D& vector);
#endif
};

Q_DECLARE_TYPEINFO(QVector4D, Q_MOVABLE_TYPE);

Q_DECL_CONSTEXPR inline QVector4D::QVector4D() : xp(0.0f), yp(0.0f), zp(0.0f), wp(0.0f) {}

Q_DECL_CONSTEXPR inline QVector4D::QVector4D(float xpos, float ypos, float zpos, float wpos) : xp(xpos), yp(ypos), zp(zpos), wp(wpos) {}

Q_DECL_CONSTEXPR inline QVector4D::QVector4D(const QPoint& point) : xp(point.x()), yp(point.y()), zp(0.0f), wp(0.0f) {}

Q_DECL_CONSTEXPR inline QVector4D::QVector4D(const QPointF& point) : xp(point.x()), yp(point.y()), zp(0.0f), wp(0.0f) {}

inline bool QVector4D::isNull() const
{
    return qIsNull(xp) && qIsNull(yp) && qIsNull(zp) && qIsNull(wp);
}

Q_DECL_CONSTEXPR inline float QVector4D::x() const { return xp; }
Q_DECL_CONSTEXPR inline float QVector4D::y() const { return yp; }
Q_DECL_CONSTEXPR inline float QVector4D::z() const { return zp; }
Q_DECL_CONSTEXPR inline float QVector4D::w() const { return wp; }

inline void QVector4D::setX(float aX) { xp = aX; }
inline void QVector4D::setY(float aY) { yp = aY; }
inline void QVector4D::setZ(float aZ) { zp = aZ; }
inline void QVector4D::setW(float aW) { wp = aW; }

inline float &QVector4D::operator[](int i)
{
    Q_ASSERT(uint(i) < 4u);
    return *(&xp + i);
}

inline float QVector4D::operator[](int i) const
{
    Q_ASSERT(uint(i) < 4u);
    return *(&xp + i);
}

inline QVector4D &QVector4D::operator+=(const QVector4D &vector)
{
    xp += vector.xp;
    yp += vector.yp;
    zp += vector.zp;
    wp += vector.wp;
    return *this;
}

inline QVector4D &QVector4D::operator-=(const QVector4D &vector)
{
    xp -= vector.xp;
    yp -= vector.yp;
    zp -= vector.zp;
    wp -= vector.wp;
    return *this;
}

inline QVector4D &QVector4D::operator*=(float factor)
{
    xp *= factor;
    yp *= factor;
    zp *= factor;
    wp *= factor;
    return *this;
}

inline QVector4D &QVector4D::operator*=(const QVector4D &vector)
{
    xp *= vector.xp;
    yp *= vector.yp;
    zp *= vector.zp;
    wp *= vector.wp;
    return *this;
}

inline QVector4D &QVector4D::operator/=(float divisor)
{
    xp /= divisor;
    yp /= divisor;
    zp /= divisor;
    wp /= divisor;
    return *this;
}

Q_DECL_CONSTEXPR inline bool operator==(const QVector4D &v1, const QVector4D &v2)
{
    return v1.xp == v2.xp && v1.yp == v2.yp && v1.zp == v2.zp && v1.wp == v2.wp;
}

Q_DECL_CONSTEXPR inline bool operator!=(const QVector4D &v1, const QVector4D &v2)
{
    return v1.xp != v2.xp || v1.yp != v2.yp || v1.zp != v2.zp || v1.wp != v2.wp;
}

Q_DECL_CONSTEXPR inline const QVector4D operator+(const QVector4D &v1, const QVector4D &v2)
{
    return QVector4D(v1.xp + v2.xp, v1.yp + v2.yp, v1.zp + v2.zp, v1.wp + v2.wp);
}

Q_DECL_CONSTEXPR inline const QVector4D operator-(const QVector4D &v1, const QVector4D &v2)
{
    return QVector4D(v1.xp - v2.xp, v1.yp - v2.yp, v1.zp - v2.zp, v1.wp - v2.wp);
}

Q_DECL_CONSTEXPR inline const QVector4D operator*(float factor, const QVector4D &vector)
{
    return QVector4D(vector.xp * factor, vector.yp * factor, vector.zp * factor, vector.wp * factor);
}

Q_DECL_CONSTEXPR inline const QVector4D operator*(const QVector4D &vector, float factor)
{
    return QVector4D(vector.xp * factor, vector.yp * factor, vector.zp * factor, vector.wp * factor);
}

Q_DECL_CONSTEXPR inline const QVector4D operator*(const QVector4D &v1, const QVector4D& v2)
{
    return QVector4D(v1.xp * v2.xp, v1.yp * v2.yp, v1.zp * v2.zp, v1.wp * v2.wp);
}

Q_DECL_CONSTEXPR inline const QVector4D operator-(const QVector4D &vector)
{
    return QVector4D(-vector.xp, -vector.yp, -vector.zp, -vector.wp);
}

Q_DECL_CONSTEXPR inline const QVector4D operator/(const QVector4D &vector, float divisor)
{
    return QVector4D(vector.xp / divisor, vector.yp / divisor, vector.zp / divisor, vector.wp / divisor);
}

Q_DECL_CONSTEXPR inline bool qFuzzyCompare(const QVector4D& v1, const QVector4D& v2)
{
    return qFuzzyCompare(v1.xp, v2.xp) &&
           qFuzzyCompare(v1.yp, v2.yp) &&
           qFuzzyCompare(v1.zp, v2.zp) &&
           qFuzzyCompare(v1.wp, v2.wp);
}

Q_DECL_CONSTEXPR inline QPoint QVector4D::toPoint() const
{
    return QPoint(qRound(xp), qRound(yp));
}

Q_DECL_CONSTEXPR inline QPointF QVector4D::toPointF() const
{
    return QPointF(qreal(xp), qreal(yp));
}

#ifndef QT_NO_DEBUG_STREAM
Q_GUI_EXPORT QDebug operator<<(QDebug dbg, const QVector4D &vector);
#endif

#ifndef QT_NO_DATASTREAM
Q_GUI_EXPORT QDataStream &operator<<(QDataStream &, const QVector4D &);
Q_GUI_EXPORT QDataStream &operator>>(QDataStream &, QVector4D &);
#endif

#endif

QT_END_NAMESPACE

#endif
