/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef QVECTOR3D_H
#define QVECTOR3D_H

#include <QtCore/qpoint.h>
#include <QtCore/qmetatype.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Gui)

class QMatrix4x4;
class QVector2D;
class QVector4D;

#ifndef QT_NO_VECTOR3D

class Q_GUI_EXPORT QVector3D
{
public:
    QVector3D();
    QVector3D(qreal xpos, qreal ypos, qreal zpos);
    explicit QVector3D(const QPoint& point);
    explicit QVector3D(const QPointF& point);
#ifndef QT_NO_VECTOR2D
    QVector3D(const QVector2D& vector);
    QVector3D(const QVector2D& vector, qreal zpos);
#endif
#ifndef QT_NO_VECTOR4D
    explicit QVector3D(const QVector4D& vector);
#endif

    bool isNull() const;

    qreal x() const;
    qreal y() const;
    qreal z() const;

    void setX(qreal x);
    void setY(qreal y);
    void setZ(qreal z);

    qreal length() const;
    qreal lengthSquared() const;

    QVector3D normalized() const;
    void normalize();

    QVector3D &operator+=(const QVector3D &vector);
    QVector3D &operator-=(const QVector3D &vector);
    QVector3D &operator*=(qreal factor);
    QVector3D &operator*=(const QVector3D& vector);
    QVector3D &operator/=(qreal divisor);

    static qreal dotProduct(const QVector3D& v1, const QVector3D& v2);
    static QVector3D crossProduct(const QVector3D& v1, const QVector3D& v2);
    static QVector3D normal(const QVector3D& v1, const QVector3D& v2);
    static QVector3D normal
        (const QVector3D& v1, const QVector3D& v2, const QVector3D& v3);

    qreal distanceToPlane(const QVector3D& plane, const QVector3D& normal) const;
    qreal distanceToPlane(const QVector3D& plane1, const QVector3D& plane2, const QVector3D& plane3) const;
    qreal distanceToLine(const QVector3D& point, const QVector3D& direction) const;

    friend inline bool operator==(const QVector3D &v1, const QVector3D &v2);
    friend inline bool operator!=(const QVector3D &v1, const QVector3D &v2);
    friend inline const QVector3D operator+(const QVector3D &v1, const QVector3D &v2);
    friend inline const QVector3D operator-(const QVector3D &v1, const QVector3D &v2);
    friend inline const QVector3D operator*(qreal factor, const QVector3D &vector);
    friend inline const QVector3D operator*(const QVector3D &vector, qreal factor);
    friend const QVector3D operator*(const QVector3D &v1, const QVector3D& v2);
    friend inline const QVector3D operator-(const QVector3D &vector);
    friend inline const QVector3D operator/(const QVector3D &vector, qreal divisor);

    friend inline bool qFuzzyCompare(const QVector3D& v1, const QVector3D& v2);

#ifndef QT_NO_VECTOR2D
    QVector2D toVector2D() const;
#endif
#ifndef QT_NO_VECTOR4D
    QVector4D toVector4D() const;
#endif

    QPoint toPoint() const;
    QPointF toPointF() const;

    operator QVariant() const;

private:
    float xp, yp, zp;

    QVector3D(float xpos, float ypos, float zpos, int dummy);

    friend class QVector2D;
    friend class QVector4D;
#ifndef QT_NO_MATRIX4X4
    friend QVector3D operator*(const QVector3D& vector, const QMatrix4x4& matrix);
    friend QVector3D operator*(const QMatrix4x4& matrix, const QVector3D& vector);
#endif
};

Q_DECLARE_TYPEINFO(QVector3D, Q_MOVABLE_TYPE);

inline QVector3D::QVector3D() : xp(0.0f), yp(0.0f), zp(0.0f) {}

inline QVector3D::QVector3D(qreal xpos, qreal ypos, qreal zpos) : xp(xpos), yp(ypos), zp(zpos) {}

inline QVector3D::QVector3D(float xpos, float ypos, float zpos, int) : xp(xpos), yp(ypos), zp(zpos) {}

inline QVector3D::QVector3D(const QPoint& point) : xp(point.x()), yp(point.y()), zp(0.0f) {}

inline QVector3D::QVector3D(const QPointF& point) : xp(point.x()), yp(point.y()), zp(0.0f) {}

inline bool QVector3D::isNull() const
{
    return qIsNull(xp) && qIsNull(yp) && qIsNull(zp);
}

inline qreal QVector3D::x() const { return qreal(xp); }
inline qreal QVector3D::y() const { return qreal(yp); }
inline qreal QVector3D::z() const { return qreal(zp); }

inline void QVector3D::setX(qreal aX) { xp = aX; }
inline void QVector3D::setY(qreal aY) { yp = aY; }
inline void QVector3D::setZ(qreal aZ) { zp = aZ; }

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

inline QVector3D &QVector3D::operator*=(qreal factor)
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

inline QVector3D &QVector3D::operator/=(qreal divisor)
{
    xp /= divisor;
    yp /= divisor;
    zp /= divisor;
    return *this;
}

inline bool operator==(const QVector3D &v1, const QVector3D &v2)
{
    return v1.xp == v2.xp && v1.yp == v2.yp && v1.zp == v2.zp;
}

inline bool operator!=(const QVector3D &v1, const QVector3D &v2)
{
    return v1.xp != v2.xp || v1.yp != v2.yp || v1.zp != v2.zp;
}

inline const QVector3D operator+(const QVector3D &v1, const QVector3D &v2)
{
    return QVector3D(v1.xp + v2.xp, v1.yp + v2.yp, v1.zp + v2.zp, 1);
}

inline const QVector3D operator-(const QVector3D &v1, const QVector3D &v2)
{
    return QVector3D(v1.xp - v2.xp, v1.yp - v2.yp, v1.zp - v2.zp, 1);
}

inline const QVector3D operator*(qreal factor, const QVector3D &vector)
{
    return QVector3D(vector.xp * factor, vector.yp * factor, vector.zp * factor, 1);
}

inline const QVector3D operator*(const QVector3D &vector, qreal factor)
{
    return QVector3D(vector.xp * factor, vector.yp * factor, vector.zp * factor, 1);
}

inline const QVector3D operator*(const QVector3D &v1, const QVector3D& v2)
{
    return QVector3D(v1.xp * v2.xp, v1.yp * v2.yp, v1.zp * v2.zp, 1);
}

inline const QVector3D operator-(const QVector3D &vector)
{
    return QVector3D(-vector.xp, -vector.yp, -vector.zp, 1);
}

inline const QVector3D operator/(const QVector3D &vector, qreal divisor)
{
    return QVector3D(vector.xp / divisor, vector.yp / divisor, vector.zp / divisor, 1);
}

inline bool qFuzzyCompare(const QVector3D& v1, const QVector3D& v2)
{
    return qFuzzyCompare(v1.xp, v2.xp) &&
           qFuzzyCompare(v1.yp, v2.yp) &&
           qFuzzyCompare(v1.zp, v2.zp);
}

inline QPoint QVector3D::toPoint() const
{
    return QPoint(qRound(xp), qRound(yp));
}

inline QPointF QVector3D::toPointF() const
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

QT_END_HEADER

#endif
