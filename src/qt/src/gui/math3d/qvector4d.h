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

#ifndef QVECTOR4D_H
#define QVECTOR4D_H

#include <QtCore/qpoint.h>
#include <QtCore/qmetatype.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Gui)

class QMatrix4x4;
class QVector2D;
class QVector3D;

#ifndef QT_NO_VECTOR4D

class Q_GUI_EXPORT QVector4D
{
public:
    QVector4D();
    QVector4D(qreal xpos, qreal ypos, qreal zpos, qreal wpos);
    explicit QVector4D(const QPoint& point);
    explicit QVector4D(const QPointF& point);
#ifndef QT_NO_VECTOR2D
    QVector4D(const QVector2D& vector);
    QVector4D(const QVector2D& vector, qreal zpos, qreal wpos);
#endif
#ifndef QT_NO_VECTOR3D
    QVector4D(const QVector3D& vector);
    QVector4D(const QVector3D& vector, qreal wpos);
#endif

    bool isNull() const;

    qreal x() const;
    qreal y() const;
    qreal z() const;
    qreal w() const;

    void setX(qreal x);
    void setY(qreal y);
    void setZ(qreal z);
    void setW(qreal w);

    qreal length() const;
    qreal lengthSquared() const;

    QVector4D normalized() const;
    void normalize();

    QVector4D &operator+=(const QVector4D &vector);
    QVector4D &operator-=(const QVector4D &vector);
    QVector4D &operator*=(qreal factor);
    QVector4D &operator*=(const QVector4D &vector);
    QVector4D &operator/=(qreal divisor);

    static qreal dotProduct(const QVector4D& v1, const QVector4D& v2);

    friend inline bool operator==(const QVector4D &v1, const QVector4D &v2);
    friend inline bool operator!=(const QVector4D &v1, const QVector4D &v2);
    friend inline const QVector4D operator+(const QVector4D &v1, const QVector4D &v2);
    friend inline const QVector4D operator-(const QVector4D &v1, const QVector4D &v2);
    friend inline const QVector4D operator*(qreal factor, const QVector4D &vector);
    friend inline const QVector4D operator*(const QVector4D &vector, qreal factor);
    friend inline const QVector4D operator*(const QVector4D &v1, const QVector4D& v2);
    friend inline const QVector4D operator-(const QVector4D &vector);
    friend inline const QVector4D operator/(const QVector4D &vector, qreal divisor);

    friend inline bool qFuzzyCompare(const QVector4D& v1, const QVector4D& v2);

#ifndef QT_NO_VECTOR2D
    QVector2D toVector2D() const;
    QVector2D toVector2DAffine() const;
#endif
#ifndef QT_NO_VECTOR3D
    QVector3D toVector3D() const;
    QVector3D toVector3DAffine() const;
#endif

    QPoint toPoint() const;
    QPointF toPointF() const;

    operator QVariant() const;

private:
    float xp, yp, zp, wp;

    QVector4D(float xpos, float ypos, float zpos, float wpos, int dummy);

    friend class QVector2D;
    friend class QVector3D;
#ifndef QT_NO_MATRIX4X4
    friend QVector4D operator*(const QVector4D& vector, const QMatrix4x4& matrix);
    friend QVector4D operator*(const QMatrix4x4& matrix, const QVector4D& vector);
#endif
};

Q_DECLARE_TYPEINFO(QVector4D, Q_MOVABLE_TYPE);

inline QVector4D::QVector4D() : xp(0.0f), yp(0.0f), zp(0.0f), wp(0.0f) {}

inline QVector4D::QVector4D(qreal xpos, qreal ypos, qreal zpos, qreal wpos) : xp(xpos), yp(ypos), zp(zpos), wp(wpos) {}

inline QVector4D::QVector4D(float xpos, float ypos, float zpos, float wpos, int) : xp(xpos), yp(ypos), zp(zpos), wp(wpos) {}

inline QVector4D::QVector4D(const QPoint& point) : xp(point.x()), yp(point.y()), zp(0.0f), wp(0.0f) {}

inline QVector4D::QVector4D(const QPointF& point) : xp(point.x()), yp(point.y()), zp(0.0f), wp(0.0f) {}

inline bool QVector4D::isNull() const
{
    return qIsNull(xp) && qIsNull(yp) && qIsNull(zp) && qIsNull(wp);
}

inline qreal QVector4D::x() const { return qreal(xp); }
inline qreal QVector4D::y() const { return qreal(yp); }
inline qreal QVector4D::z() const { return qreal(zp); }
inline qreal QVector4D::w() const { return qreal(wp); }

inline void QVector4D::setX(qreal aX) { xp = aX; }
inline void QVector4D::setY(qreal aY) { yp = aY; }
inline void QVector4D::setZ(qreal aZ) { zp = aZ; }
inline void QVector4D::setW(qreal aW) { wp = aW; }

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

inline QVector4D &QVector4D::operator*=(qreal factor)
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

inline QVector4D &QVector4D::operator/=(qreal divisor)
{
    xp /= divisor;
    yp /= divisor;
    zp /= divisor;
    wp /= divisor;
    return *this;
}

inline bool operator==(const QVector4D &v1, const QVector4D &v2)
{
    return v1.xp == v2.xp && v1.yp == v2.yp && v1.zp == v2.zp && v1.wp == v2.wp;
}

inline bool operator!=(const QVector4D &v1, const QVector4D &v2)
{
    return v1.xp != v2.xp || v1.yp != v2.yp || v1.zp != v2.zp || v1.wp != v2.wp;
}

inline const QVector4D operator+(const QVector4D &v1, const QVector4D &v2)
{
    return QVector4D(v1.xp + v2.xp, v1.yp + v2.yp, v1.zp + v2.zp, v1.wp + v2.wp, 1);
}

inline const QVector4D operator-(const QVector4D &v1, const QVector4D &v2)
{
    return QVector4D(v1.xp - v2.xp, v1.yp - v2.yp, v1.zp - v2.zp, v1.wp - v2.wp, 1);
}

inline const QVector4D operator*(qreal factor, const QVector4D &vector)
{
    return QVector4D(vector.xp * factor, vector.yp * factor, vector.zp * factor, vector.wp * factor, 1);
}

inline const QVector4D operator*(const QVector4D &vector, qreal factor)
{
    return QVector4D(vector.xp * factor, vector.yp * factor, vector.zp * factor, vector.wp * factor, 1);
}

inline const QVector4D operator*(const QVector4D &v1, const QVector4D& v2)
{
    return QVector4D(v1.xp * v2.xp, v1.yp * v2.yp, v1.zp * v2.zp, v1.wp * v2.wp, 1);
}

inline const QVector4D operator-(const QVector4D &vector)
{
    return QVector4D(-vector.xp, -vector.yp, -vector.zp, -vector.wp, 1);
}

inline const QVector4D operator/(const QVector4D &vector, qreal divisor)
{
    return QVector4D(vector.xp / divisor, vector.yp / divisor, vector.zp / divisor, vector.wp / divisor, 1);
}

inline bool qFuzzyCompare(const QVector4D& v1, const QVector4D& v2)
{
    return qFuzzyCompare(v1.xp, v2.xp) &&
           qFuzzyCompare(v1.yp, v2.yp) &&
           qFuzzyCompare(v1.zp, v2.zp) &&
           qFuzzyCompare(v1.wp, v2.wp);
}

inline QPoint QVector4D::toPoint() const
{
    return QPoint(qRound(xp), qRound(yp));
}

inline QPointF QVector4D::toPointF() const
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

QT_END_HEADER

#endif
