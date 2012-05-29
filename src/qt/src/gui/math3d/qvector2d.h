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

#ifndef QVECTOR2D_H
#define QVECTOR2D_H

#include <QtCore/qpoint.h>
#include <QtCore/qmetatype.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Gui)

class QVector3D;
class QVector4D;
class QVariant;

#ifndef QT_NO_VECTOR2D

class Q_GUI_EXPORT QVector2D
{
public:
    QVector2D();
    QVector2D(qreal xpos, qreal ypos);
    explicit QVector2D(const QPoint& point);
    explicit QVector2D(const QPointF& point);
#ifndef QT_NO_VECTOR3D
    explicit QVector2D(const QVector3D& vector);
#endif
#ifndef QT_NO_VECTOR4D
    explicit QVector2D(const QVector4D& vector);
#endif

    bool isNull() const;

    qreal x() const;
    qreal y() const;

    void setX(qreal x);
    void setY(qreal y);

    qreal length() const;
    qreal lengthSquared() const;

    QVector2D normalized() const;
    void normalize();

    QVector2D &operator+=(const QVector2D &vector);
    QVector2D &operator-=(const QVector2D &vector);
    QVector2D &operator*=(qreal factor);
    QVector2D &operator*=(const QVector2D &vector);
    QVector2D &operator/=(qreal divisor);

    static qreal dotProduct(const QVector2D& v1, const QVector2D& v2);

    friend inline bool operator==(const QVector2D &v1, const QVector2D &v2);
    friend inline bool operator!=(const QVector2D &v1, const QVector2D &v2);
    friend inline const QVector2D operator+(const QVector2D &v1, const QVector2D &v2);
    friend inline const QVector2D operator-(const QVector2D &v1, const QVector2D &v2);
    friend inline const QVector2D operator*(qreal factor, const QVector2D &vector);
    friend inline const QVector2D operator*(const QVector2D &vector, qreal factor);
    friend inline const QVector2D operator*(const QVector2D &v1, const QVector2D &v2);
    friend inline const QVector2D operator-(const QVector2D &vector);
    friend inline const QVector2D operator/(const QVector2D &vector, qreal divisor);

    friend inline bool qFuzzyCompare(const QVector2D& v1, const QVector2D& v2);

#ifndef QT_NO_VECTOR3D
    QVector3D toVector3D() const;
#endif
#ifndef QT_NO_VECTOR4D
    QVector4D toVector4D() const;
#endif

    QPoint toPoint() const;
    QPointF toPointF() const;

    operator QVariant() const;

private:
    float xp, yp;

    QVector2D(float xpos, float ypos, int dummy);

    friend class QVector3D;
    friend class QVector4D;
};

Q_DECLARE_TYPEINFO(QVector2D, Q_MOVABLE_TYPE);

inline QVector2D::QVector2D() : xp(0.0f), yp(0.0f) {}

inline QVector2D::QVector2D(float xpos, float ypos, int) : xp(xpos), yp(ypos) {}

inline QVector2D::QVector2D(qreal xpos, qreal ypos) : xp(xpos), yp(ypos) {}

inline QVector2D::QVector2D(const QPoint& point) : xp(point.x()), yp(point.y()) {}

inline QVector2D::QVector2D(const QPointF& point) : xp(point.x()), yp(point.y()) {}

inline bool QVector2D::isNull() const
{
    return qIsNull(xp) && qIsNull(yp);
}

inline qreal QVector2D::x() const { return qreal(xp); }
inline qreal QVector2D::y() const { return qreal(yp); }

inline void QVector2D::setX(qreal aX) { xp = aX; }
inline void QVector2D::setY(qreal aY) { yp = aY; }

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

inline QVector2D &QVector2D::operator*=(qreal factor)
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

inline QVector2D &QVector2D::operator/=(qreal divisor)
{
    xp /= divisor;
    yp /= divisor;
    return *this;
}

inline bool operator==(const QVector2D &v1, const QVector2D &v2)
{
    return v1.xp == v2.xp && v1.yp == v2.yp;
}

inline bool operator!=(const QVector2D &v1, const QVector2D &v2)
{
    return v1.xp != v2.xp || v1.yp != v2.yp;
}

inline const QVector2D operator+(const QVector2D &v1, const QVector2D &v2)
{
    return QVector2D(v1.xp + v2.xp, v1.yp + v2.yp, 1);
}

inline const QVector2D operator-(const QVector2D &v1, const QVector2D &v2)
{
    return QVector2D(v1.xp - v2.xp, v1.yp - v2.yp, 1);
}

inline const QVector2D operator*(qreal factor, const QVector2D &vector)
{
    return QVector2D(vector.xp * factor, vector.yp * factor, 1);
}

inline const QVector2D operator*(const QVector2D &vector, qreal factor)
{
    return QVector2D(vector.xp * factor, vector.yp * factor, 1);
}

inline const QVector2D operator*(const QVector2D &v1, const QVector2D &v2)
{
    return QVector2D(v1.xp * v2.xp, v1.yp * v2.yp, 1);
}

inline const QVector2D operator-(const QVector2D &vector)
{
    return QVector2D(-vector.xp, -vector.yp, 1);
}

inline const QVector2D operator/(const QVector2D &vector, qreal divisor)
{
    return QVector2D(vector.xp / divisor, vector.yp / divisor, 1);
}

inline bool qFuzzyCompare(const QVector2D& v1, const QVector2D& v2)
{
    return qFuzzyCompare(v1.xp, v2.xp) && qFuzzyCompare(v1.yp, v2.yp);
}

inline QPoint QVector2D::toPoint() const
{
    return QPoint(qRound(xp), qRound(yp));
}

inline QPointF QVector2D::toPointF() const
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

QT_END_HEADER

#endif
