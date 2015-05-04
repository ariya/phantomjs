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

#ifndef QVECTOR2D_H
#define QVECTOR2D_H

#include <QtCore/qpoint.h>
#include <QtCore/qmetatype.h>

QT_BEGIN_NAMESPACE


class QVector3D;
class QVector4D;
class QVariant;

#ifndef QT_NO_VECTOR2D

class Q_GUI_EXPORT QVector2D
{
public:
    Q_DECL_CONSTEXPR QVector2D();
    Q_DECL_CONSTEXPR QVector2D(float xpos, float ypos);
    Q_DECL_CONSTEXPR explicit QVector2D(const QPoint& point);
    Q_DECL_CONSTEXPR explicit QVector2D(const QPointF& point);
#ifndef QT_NO_VECTOR3D
    explicit QVector2D(const QVector3D& vector);
#endif
#ifndef QT_NO_VECTOR4D
    explicit QVector2D(const QVector4D& vector);
#endif

    bool isNull() const;

    Q_DECL_CONSTEXPR float x() const;
    Q_DECL_CONSTEXPR float y() const;

    void setX(float x);
    void setY(float y);

    float &operator[](int i);
    float operator[](int i) const;

    float length() const;
    float lengthSquared() const; //In Qt 6 convert to inline and constexpr

    QVector2D normalized() const;
    void normalize();

    float distanceToPoint(const QVector2D &point) const;
    float distanceToLine(const QVector2D& point, const QVector2D& direction) const;

    QVector2D &operator+=(const QVector2D &vector);
    QVector2D &operator-=(const QVector2D &vector);
    QVector2D &operator*=(float factor);
    QVector2D &operator*=(const QVector2D &vector);
    QVector2D &operator/=(float divisor);

    static float dotProduct(const QVector2D& v1, const QVector2D& v2); //In Qt 6 convert to inline and constexpr

    Q_DECL_CONSTEXPR friend inline bool operator==(const QVector2D &v1, const QVector2D &v2);
    Q_DECL_CONSTEXPR friend inline bool operator!=(const QVector2D &v1, const QVector2D &v2);
    Q_DECL_CONSTEXPR friend inline const QVector2D operator+(const QVector2D &v1, const QVector2D &v2);
    Q_DECL_CONSTEXPR friend inline const QVector2D operator-(const QVector2D &v1, const QVector2D &v2);
    Q_DECL_CONSTEXPR friend inline const QVector2D operator*(float factor, const QVector2D &vector);
    Q_DECL_CONSTEXPR friend inline const QVector2D operator*(const QVector2D &vector, float factor);
    Q_DECL_CONSTEXPR friend inline const QVector2D operator*(const QVector2D &v1, const QVector2D &v2);
    Q_DECL_CONSTEXPR friend inline const QVector2D operator-(const QVector2D &vector);
    Q_DECL_CONSTEXPR friend inline const QVector2D operator/(const QVector2D &vector, float divisor);

    Q_DECL_CONSTEXPR friend inline bool qFuzzyCompare(const QVector2D& v1, const QVector2D& v2);

#ifndef QT_NO_VECTOR3D
    QVector3D toVector3D() const;
#endif
#ifndef QT_NO_VECTOR4D
    QVector4D toVector4D() const;
#endif

    Q_DECL_CONSTEXPR QPoint toPoint() const;
    Q_DECL_CONSTEXPR QPointF toPointF() const;

    operator QVariant() const;

private:
    float xp, yp;

    friend class QVector3D;
    friend class QVector4D;
};

Q_DECLARE_TYPEINFO(QVector2D, Q_MOVABLE_TYPE);

Q_DECL_CONSTEXPR inline QVector2D::QVector2D() : xp(0.0f), yp(0.0f) {}

Q_DECL_CONSTEXPR inline QVector2D::QVector2D(float xpos, float ypos) : xp(xpos), yp(ypos) {}

Q_DECL_CONSTEXPR inline QVector2D::QVector2D(const QPoint& point) : xp(point.x()), yp(point.y()) {}

Q_DECL_CONSTEXPR inline QVector2D::QVector2D(const QPointF& point) : xp(point.x()), yp(point.y()) {}

inline bool QVector2D::isNull() const
{
    return qIsNull(xp) && qIsNull(yp);
}

Q_DECL_CONSTEXPR inline float QVector2D::x() const { return xp; }
Q_DECL_CONSTEXPR inline float QVector2D::y() const { return yp; }

inline void QVector2D::setX(float aX) { xp = aX; }
inline void QVector2D::setY(float aY) { yp = aY; }

inline float &QVector2D::operator[](int i)
{
    Q_ASSERT(uint(i) < 2u);
    return *(&xp + i);
}

inline float QVector2D::operator[](int i) const
{
    Q_ASSERT(uint(i) < 2u);
    return *(&xp + i);
}

inline QVector2D &QVector2D::operator+=(const QVector2D &vector)
{
    xp += vector.xp;
    yp += vector.yp;
    return *this;
}

inline QVector2D &QVector2D::operator-=(const QVector2D &vector)
{
    xp -= vector.xp;
    yp -= vector.yp;
    return *this;
}

inline QVector2D &QVector2D::operator*=(float factor)
{
    xp *= factor;
    yp *= factor;
    return *this;
}

inline QVector2D &QVector2D::operator*=(const QVector2D &vector)
{
    xp *= vector.xp;
    yp *= vector.yp;
    return *this;
}

inline QVector2D &QVector2D::operator/=(float divisor)
{
    xp /= divisor;
    yp /= divisor;
    return *this;
}

Q_DECL_CONSTEXPR inline bool operator==(const QVector2D &v1, const QVector2D &v2)
{
    return v1.xp == v2.xp && v1.yp == v2.yp;
}

Q_DECL_CONSTEXPR inline bool operator!=(const QVector2D &v1, const QVector2D &v2)
{
    return v1.xp != v2.xp || v1.yp != v2.yp;
}

Q_DECL_CONSTEXPR inline const QVector2D operator+(const QVector2D &v1, const QVector2D &v2)
{
    return QVector2D(v1.xp + v2.xp, v1.yp + v2.yp);
}

Q_DECL_CONSTEXPR inline const QVector2D operator-(const QVector2D &v1, const QVector2D &v2)
{
    return QVector2D(v1.xp - v2.xp, v1.yp - v2.yp);
}

Q_DECL_CONSTEXPR inline const QVector2D operator*(float factor, const QVector2D &vector)
{
    return QVector2D(vector.xp * factor, vector.yp * factor);
}

Q_DECL_CONSTEXPR inline const QVector2D operator*(const QVector2D &vector, float factor)
{
    return QVector2D(vector.xp * factor, vector.yp * factor);
}

Q_DECL_CONSTEXPR inline const QVector2D operator*(const QVector2D &v1, const QVector2D &v2)
{
    return QVector2D(v1.xp * v2.xp, v1.yp * v2.yp);
}

Q_DECL_CONSTEXPR inline const QVector2D operator-(const QVector2D &vector)
{
    return QVector2D(-vector.xp, -vector.yp);
}

Q_DECL_CONSTEXPR inline const QVector2D operator/(const QVector2D &vector, float divisor)
{
    return QVector2D(vector.xp / divisor, vector.yp / divisor);
}

Q_DECL_CONSTEXPR inline bool qFuzzyCompare(const QVector2D& v1, const QVector2D& v2)
{
    return qFuzzyCompare(v1.xp, v2.xp) && qFuzzyCompare(v1.yp, v2.yp);
}

Q_DECL_CONSTEXPR inline QPoint QVector2D::toPoint() const
{
    return QPoint(qRound(xp), qRound(yp));
}

Q_DECL_CONSTEXPR inline QPointF QVector2D::toPointF() const
{
    return QPointF(qreal(xp), qreal(yp));
}

#ifndef QT_NO_DEBUG_STREAM
Q_GUI_EXPORT QDebug operator<<(QDebug dbg, const QVector2D &vector);
#endif

#ifndef QT_NO_DATASTREAM
Q_GUI_EXPORT QDataStream &operator<<(QDataStream &, const QVector2D &);
Q_GUI_EXPORT QDataStream &operator>>(QDataStream &, QVector2D &);
#endif

#endif

QT_END_NAMESPACE

#endif
