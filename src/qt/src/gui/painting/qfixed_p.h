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

#ifndef QFIXED_P_H
#define QFIXED_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of other Qt classes.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include "QtCore/qdebug.h"
#include "QtCore/qpoint.h"
#include "QtCore/qsize.h"

QT_BEGIN_NAMESPACE

struct QFixed {
public:
    QFixed() : val(0) {}
    QFixed(int i) : val(i<<6) {}
    QFixed(long i) : val(i<<6) {}
    QFixed &operator=(int i) { val = (i<<6); return *this; }
    QFixed &operator=(long i) { val = (i<<6); return *this; }

    static QFixed fromReal(qreal r) { QFixed f; f.val = (int)(r*qreal(64)); return f; }
    static QFixed fromFixed(int fixed) { QFixed f; f.val = fixed; return f; }

    inline int value() const { return val; }
    inline void setValue(int value) { val = value; }

    inline int toInt() const { return (((val)+32) & -64)>>6; }
    inline qreal toReal() const { return ((qreal)val)/(qreal)64; }

    inline int truncate() const { return val>>6; }
    inline QFixed round() const { QFixed f; f.val = ((val)+32) & -64; return f; }
    inline QFixed floor() const { QFixed f; f.val = (val) & -64; return f; }
    inline QFixed ceil() const { QFixed f; f.val = (val+63) & -64; return f; }

    inline QFixed operator+(int i) const { QFixed f; f.val = (val + (i<<6)); return f; }
    inline QFixed operator+(uint i) const { QFixed f; f.val = (val + (i<<6)); return f; }
    inline QFixed operator+(const QFixed &other) const { QFixed f; f.val = (val + other.val); return f; }
    inline QFixed &operator+=(int i) { val += (i<<6); return *this; }
    inline QFixed &operator+=(uint i) { val += (i<<6); return *this; }
    inline QFixed &operator+=(const QFixed &other) { val += other.val; return *this; }
    inline QFixed operator-(int i) const { QFixed f; f.val = (val - (i<<6)); return f; }
    inline QFixed operator-(uint i) const { QFixed f; f.val = (val - (i<<6)); return f; }
    inline QFixed operator-(const QFixed &other) const { QFixed f; f.val = (val - other.val); return f; }
    inline QFixed &operator-=(int i) { val -= (i<<6); return *this; }
    inline QFixed &operator-=(uint i) { val -= (i<<6); return *this; }
    inline QFixed &operator-=(const QFixed &other) { val -= other.val; return *this; }
    inline QFixed operator-() const { QFixed f; f.val = -val; return f; }

    inline bool operator==(const QFixed &other) const { return val == other.val; }
    inline bool operator!=(const QFixed &other) const { return val != other.val; }
    inline bool operator<(const QFixed &other) const { return val < other.val; }
    inline bool operator>(const QFixed &other) const { return val > other.val; }
    inline bool operator<=(const QFixed &other) const { return val <= other.val; }
    inline bool operator>=(const QFixed &other) const { return val >= other.val; }
    inline bool operator!() const { return !val; }

    inline QFixed &operator/=(int x) { val /= x; return *this; }
    inline QFixed &operator/=(const QFixed &o) {
        if (o.val == 0) {
            val = 0x7FFFFFFFL;
        } else {
            bool neg = false;
            qint64 a = val;
            qint64 b = o.val;
            if (a < 0) { a = -a; neg = true; }
            if (b < 0) { b = -b; neg = !neg; }

            int res = (int)(((a << 6) + (b >> 1)) / b);

            val = (neg ? -res : res);
        }
        return *this;
    }
    inline QFixed operator/(int d) const { QFixed f; f.val = val/d; return f; }
    inline QFixed operator/(QFixed b) const { QFixed f = *this; return (f /= b); }
    inline QFixed operator>>(int d) const { QFixed f = *this; f.val >>= d; return f; }
    inline QFixed &operator*=(int i) { val *= i; return *this; }
    inline QFixed &operator*=(uint i) { val *= i; return *this; }
    inline QFixed &operator*=(const QFixed &o) {
        bool neg = false;
        qint64 a = val;
        qint64 b = o.val;
        if (a < 0) { a = -a; neg = true; }
        if (b < 0) { b = -b; neg = !neg; }

        int res = (int)((a * b + 0x20L) >> 6);
        val = neg ? -res : res;
        return *this;
    }
    inline QFixed operator*(int i) const { QFixed f = *this; return (f *= i); }
    inline QFixed operator*(uint i) const { QFixed f = *this; return (f *= i); }
    inline QFixed operator*(const QFixed &o) const { QFixed f = *this; return (f *= o); }

private:
    QFixed(qreal i) : val((int)(i*qreal(64))) {}
    QFixed &operator=(qreal i) { val = (int)(i*qreal(64)); return *this; }
    inline QFixed operator+(qreal i) const { QFixed f; f.val = (val + (int)(i*qreal(64))); return f; }
    inline QFixed &operator+=(qreal i) { val += (int)(i*64); return *this; }
    inline QFixed operator-(qreal i) const { QFixed f; f.val = (val - (int)(i*qreal(64))); return f; }
    inline QFixed &operator-=(qreal i) { val -= (int)(i*64); return *this; }
    inline QFixed &operator/=(qreal r) { val = (int)(val/r); return *this; }
    inline QFixed operator/(qreal d) const { QFixed f; f.val = (int)(val/d); return f; }
    inline QFixed &operator*=(qreal d) { val = (int) (val*d); return *this; }
    inline QFixed operator*(qreal d) const { QFixed f = *this; return (f *= d); }
    int val;
};
Q_DECLARE_TYPEINFO(QFixed, Q_PRIMITIVE_TYPE);

#define QFIXED_MAX (INT_MAX/256)

inline int qRound(const QFixed &f) { return f.toInt(); }
inline int qFloor(const QFixed &f) { return f.floor().truncate(); }

inline QFixed operator*(int i, const QFixed &d) { return d*i; }
inline QFixed operator+(int i, const QFixed &d) { return d+i; }
inline QFixed operator-(int i, const QFixed &d) { return -(d-i); }
inline QFixed operator*(uint i, const QFixed &d) { return d*i; }
inline QFixed operator+(uint i, const QFixed &d) { return d+i; }
inline QFixed operator-(uint i, const QFixed &d) { return -(d-i); }
// inline QFixed operator*(qreal d, const QFixed &d2) { return d2*d; }

inline bool operator==(const QFixed &f, int i) { return f.value() == (i<<6); }
inline bool operator==(int i, const QFixed &f) { return f.value() == (i<<6); }
inline bool operator!=(const QFixed &f, int i) { return f.value() != (i<<6); }
inline bool operator!=(int i, const QFixed &f) { return f.value() != (i<<6); }
inline bool operator<=(const QFixed &f, int i) { return f.value() <= (i<<6); }
inline bool operator<=(int i, const QFixed &f) { return (i<<6) <= f.value(); }
inline bool operator>=(const QFixed &f, int i) { return f.value() >= (i<<6); }
inline bool operator>=(int i, const QFixed &f) { return (i<<6) >= f.value(); }
inline bool operator<(const QFixed &f, int i) { return f.value() < (i<<6); }
inline bool operator<(int i, const QFixed &f) { return (i<<6) < f.value(); }
inline bool operator>(const QFixed &f, int i) { return f.value() > (i<<6); }
inline bool operator>(int i, const QFixed &f) { return (i<<6) > f.value(); }

#ifndef QT_NO_DEBUG_STREAM
inline QDebug &operator<<(QDebug &dbg, const QFixed &f)
{ return dbg << f.toReal(); }
#endif

struct QFixedPoint {
    QFixed x;
    QFixed y;
    inline QFixedPoint() {}
    inline QFixedPoint(const QFixed &_x, const QFixed &_y) : x(_x), y(_y) {}
    QPointF toPointF() const { return QPointF(x.toReal(), y.toReal()); }
    static QFixedPoint fromPointF(const QPointF &p) {
        return QFixedPoint(QFixed::fromReal(p.x()), QFixed::fromReal(p.y()));
    }
};
Q_DECLARE_TYPEINFO(QFixedPoint, Q_PRIMITIVE_TYPE);

inline QFixedPoint operator-(const QFixedPoint &p1, const QFixedPoint &p2)
{ return QFixedPoint(p1.x - p2.x, p1.y - p2.y); }
inline QFixedPoint operator+(const QFixedPoint &p1, const QFixedPoint &p2)
{ return QFixedPoint(p1.x + p2.x, p1.y + p2.y); }

struct QFixedSize {
    QFixed width;
    QFixed height;
    QSizeF toSizeF() const { return QSizeF(width.toReal(), height.toReal()); }
    static QFixedSize fromSizeF(const QSizeF &s) {
        QFixedSize size;
        size.width = QFixed::fromReal(s.width());
        size.height = QFixed::fromReal(s.height());
        return size;
    }
};
Q_DECLARE_TYPEINFO(QFixedSize, Q_PRIMITIVE_TYPE);

QT_END_NAMESPACE

#endif // QTEXTENGINE_P_H
