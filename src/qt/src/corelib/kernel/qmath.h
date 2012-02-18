/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtCore module of the Qt Toolkit.
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

#ifndef QMATH_H
#define QMATH_H

#include <math.h>

#include <QtCore/qglobal.h>

#ifdef Q_OS_SYMBIAN
#    include <e32math.h>
#endif

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Core)

#define QT_SINE_TABLE_SIZE 256

extern Q_CORE_EXPORT const qreal qt_sine_table[QT_SINE_TABLE_SIZE];

inline int qCeil(qreal v)
{
#ifdef QT_USE_MATH_H_FLOATS
    if (sizeof(qreal) == sizeof(float))
        return int(ceilf(float(v)));
    else
#endif
        return int(ceil(v));
}

inline int qFloor(qreal v)
{
#ifdef QT_USE_MATH_H_FLOATS
    if (sizeof(qreal) == sizeof(float))
        return int(floorf(float(v)));
    else
#endif
        return int(floor(v));
}

inline qreal qFabs(qreal v)
{
#ifdef QT_USE_MATH_H_FLOATS
    if(sizeof(qreal) == sizeof(float))
        return fabsf(float(v));
    else
#endif
        return fabs(v);
}

inline qreal qSin(qreal v)
{
#ifdef Q_OS_SYMBIAN
    TReal sin_v;
    Math::Sin(sin_v, static_cast<TReal>(v));
    return static_cast<qreal>(sin_v);
#else
#    ifdef QT_USE_MATH_H_FLOATS
        if (sizeof(qreal) == sizeof(float))
            return sinf(float(v));
        else
#    endif
            return sin(v);
#endif
}

inline qreal qCos(qreal v)
{
#ifdef Q_OS_SYMBIAN
    TReal cos_v;
    Math::Cos(cos_v, static_cast<TReal>(v));
    return static_cast<qreal>(cos_v);
#else
#    ifdef QT_USE_MATH_H_FLOATS
        if (sizeof(qreal) == sizeof(float))
            return cosf(float(v));
        else
#    endif
            return cos(v);
#endif
}

inline qreal qTan(qreal v)
{
#ifdef Q_OS_SYMBIAN
    TReal tan_v;
    Math::Tan(tan_v, static_cast<TReal>(v));
    return static_cast<qreal>(tan_v);
#else
#    ifdef QT_USE_MATH_H_FLOATS
        if (sizeof(qreal) == sizeof(float))
            return tanf(float(v));
        else
#    endif
            return tan(v);
#endif
}

inline qreal qAcos(qreal v)
{
#ifdef Q_OS_SYMBIAN
    TReal acos_v;
    Math::ACos(acos_v, static_cast<TReal>(v));
    return static_cast<qreal>(acos_v);
#else
#    ifdef QT_USE_MATH_H_FLOATS
        if (sizeof(qreal) == sizeof(float))
            return acosf(float(v));
        else
#    endif
           return acos(v);
#endif
}

inline qreal qAsin(qreal v)
{
#ifdef Q_OS_SYMBIAN
    TReal asin_v;
    Math::ASin(asin_v, static_cast<TReal>(v));
    return static_cast<qreal>(asin_v);
#else
#    ifdef QT_USE_MATH_H_FLOATS
        if (sizeof(qreal) == sizeof(float))
            return asinf(float(v));
        else
#    endif
            return asin(v);
#endif
}

inline qreal qAtan(qreal v)
{
#ifdef Q_OS_SYMBIAN
    TReal atan_v;
    Math::ATan(atan_v, static_cast<TReal>(v));
    return static_cast<qreal>(atan_v);
#else
#    ifdef QT_USE_MATH_H_FLOATS
        if(sizeof(qreal) == sizeof(float))
            return atanf(float(v));
        else
#    endif
            return atan(v);
#endif
}

inline qreal qAtan2(qreal x, qreal y)
{
#ifdef Q_OS_SYMBIAN
    TReal atan2_v;
    Math::ATan(atan2_v, static_cast<TReal>(x), static_cast<TReal>(y));
    return static_cast<qreal>(atan2_v);
#else
#    ifdef QT_USE_MATH_H_FLOATS
        if(sizeof(qreal) == sizeof(float))
            return atan2f(float(x), float(y));
        else
#    endif
            return atan2(x, y);
#endif
}

inline qreal qSqrt(qreal v)
{
#ifdef Q_OS_SYMBIAN
    TReal sqrt_v;
    Math::Sqrt(sqrt_v, static_cast<TReal>(v));
    return static_cast<qreal>(sqrt_v);
#else
#    ifdef QT_USE_MATH_H_FLOATS
        if (sizeof(qreal) == sizeof(float))
            return sqrtf(float(v));
        else
#    endif
            return sqrt(v);
#endif
}

inline qreal qLn(qreal v)
{
#ifdef QT_USE_MATH_H_FLOATS
    if (sizeof(qreal) == sizeof(float))
        return logf(float(v));
    else
#endif
        return log(v);
}

inline qreal qExp(qreal v)
{
#ifdef Q_OS_SYMBIAN
    TReal exp_v;
    Math::Exp(exp_v, static_cast<TReal>(v));
    return static_cast<qreal>(exp_v);
#else
    // only one signature
    // exists, exp(double)
    return exp(v);
#endif
}

inline qreal qPow(qreal x, qreal y)
{
#ifdef Q_OS_SYMBIAN
    TReal pow_v;
    Math::Pow(pow_v, static_cast<TReal>(x), static_cast<TReal>(y));
    return static_cast<qreal>(pow_v);
#else
#    ifdef QT_USE_MATH_H_FLOATS
        if (sizeof(qreal) == sizeof(float))
            return powf(float(x), float(y));
        else
#    endif
            return pow(x, y);
#endif
}

#ifndef M_PI
#define M_PI (3.14159265358979323846)
#endif

inline qreal qFastSin(qreal x)
{
    int si = int(x * (0.5 * QT_SINE_TABLE_SIZE / M_PI)); // Would be more accurate with qRound, but slower.
    qreal d = x - si * (2.0 * M_PI / QT_SINE_TABLE_SIZE);
    int ci = si + QT_SINE_TABLE_SIZE / 4;
    si &= QT_SINE_TABLE_SIZE - 1;
    ci &= QT_SINE_TABLE_SIZE - 1;
    return qt_sine_table[si] + (qt_sine_table[ci] - 0.5 * qt_sine_table[si] * d) * d;
}

inline qreal qFastCos(qreal x)
{
    int ci = int(x * (0.5 * QT_SINE_TABLE_SIZE / M_PI)); // Would be more accurate with qRound, but slower.
    qreal d = x - ci * (2.0 * M_PI / QT_SINE_TABLE_SIZE);
    int si = ci + QT_SINE_TABLE_SIZE / 4;
    si &= QT_SINE_TABLE_SIZE - 1;
    ci &= QT_SINE_TABLE_SIZE - 1;
    return qt_sine_table[si] - (qt_sine_table[ci] + 0.5 * qt_sine_table[si] * d) * d;
}

QT_END_NAMESPACE

QT_END_HEADER

#endif // QMATH_H
