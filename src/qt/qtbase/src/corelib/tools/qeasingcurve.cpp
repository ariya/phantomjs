/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtCore module of the Qt Toolkit.
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

/*

| *property* | *Used for type* |
| period     | QEasingCurve::{In,Out,InOut,OutIn}Elastic |
| amplitude  | QEasingCurve::{In,Out,InOut,OutIn}Bounce, QEasingCurve::{In,Out,InOut,OutIn}Elastic |
| overshoot  | QEasingCurve::{In,Out,InOut,OutIn}Back   |

*/




/*!
    \class QEasingCurve
    \inmodule QtCore
    \since 4.6
    \ingroup animation
    \brief The QEasingCurve class provides easing curves for controlling animation.

    Easing curves describe a function that controls how the speed of the interpolation
    between 0 and 1 should be. Easing curves allow transitions from
    one value to another to appear more natural than a simple constant speed would allow.
    The QEasingCurve class is usually used in conjunction with the QVariantAnimation and
    QPropertyAnimation classes but can be used on its own. It is usually used to accelerate
    the interpolation from zero velocity (ease in) or decelerate to zero velocity (ease out).
    Ease in and ease out can also be combined in the same easing curve.

    To calculate the speed of the interpolation, the easing curve provides the function
    valueForProgress(), where the \a progress argument specifies the progress of the
    interpolation: 0 is the start value of the interpolation, 1 is the end value of the
    interpolation. The returned value is the effective progress of the interpolation.
    If the returned value is the same as the input value for all input values the easing
    curve is a linear curve. This is the default behaviour.

    For example,
    \code
    QEasingCurve easing(QEasingCurve::InOutQuad);

    for(qreal t = 0.0; t < 1.0; t+=0.1)
        qWarning() << "Effective progress" << t << " is
                   << easing.valueForProgress(t);
    \endcode
    will print the effective progress of the interpolation between 0 and 1.

    When using a QPropertyAnimation, the associated easing curve will be used to control the
    progress of the interpolation between startValue and endValue:
    \code
    QPropertyAnimation animation;
    animation.setStartValue(0);
    animation.setEndValue(1000);
    animation.setDuration(1000);
    animation.setEasingCurve(QEasingCurve::InOutQuad);
    \endcode

    The ability to set an amplitude, overshoot, or period depends on
    the QEasingCurve type. Amplitude access is available to curves
    that behave as springs such as elastic and bounce curves. Changing
    the amplitude changes the height of the curve. Period access is
    only available to elastic curves and setting a higher period slows
    the rate of bounce. Only curves that have "boomerang" behaviors
    such as the InBack, OutBack, InOutBack, and OutInBack have
    overshoot settings. These curves will interpolate beyond the end
    points and return to the end point, acting similar to a boomerang.

    The \l{Easing Curves Example} contains samples of QEasingCurve
    types and lets you change the curve settings.

 */

/*!
    \enum QEasingCurve::Type

    The type of easing curve.

    \value Linear       \image qeasingcurve-linear.png
                        \caption Easing curve for a linear (t) function:
                        velocity is constant.
    \value InQuad       \image qeasingcurve-inquad.png
                        \caption Easing curve for a quadratic (t^2) function:
                        accelerating from zero velocity.
    \value OutQuad      \image qeasingcurve-outquad.png
                        \caption Easing curve for a quadratic (t^2) function:
                        decelerating to zero velocity.
    \value InOutQuad    \image qeasingcurve-inoutquad.png
                        \caption Easing curve for a quadratic (t^2) function:
                        acceleration until halfway, then deceleration.
    \value OutInQuad    \image qeasingcurve-outinquad.png
                        \caption Easing curve for a quadratic (t^2) function:
                        deceleration until halfway, then acceleration.
    \value InCubic      \image qeasingcurve-incubic.png
                        \caption Easing curve for a cubic (t^3) function:
                        accelerating from zero velocity.
    \value OutCubic     \image qeasingcurve-outcubic.png
                        \caption Easing curve for a cubic (t^3) function:
                        decelerating to zero velocity.
    \value InOutCubic   \image qeasingcurve-inoutcubic.png
                        \caption Easing curve for a cubic (t^3) function:
                        acceleration until halfway, then deceleration.
    \value OutInCubic   \image qeasingcurve-outincubic.png
                        \caption Easing curve for a cubic (t^3) function:
                        deceleration until halfway, then acceleration.
    \value InQuart      \image qeasingcurve-inquart.png
                        \caption Easing curve for a quartic (t^4) function:
                        accelerating from zero velocity.
    \value OutQuart     \image qeasingcurve-outquart.png
                        \caption
                        Easing curve for a quartic (t^4) function:
                        decelerating to zero velocity.
    \value InOutQuart   \image qeasingcurve-inoutquart.png
                        \caption
                        Easing curve for a quartic (t^4) function:
                        acceleration until halfway, then deceleration.
    \value OutInQuart   \image qeasingcurve-outinquart.png
                        \caption
                        Easing curve for a quartic (t^4) function:
                        deceleration until halfway, then acceleration.
    \value InQuint      \image qeasingcurve-inquint.png
                        \caption
                        Easing curve for a quintic (t^5) easing
                        in: accelerating from zero velocity.
    \value OutQuint     \image qeasingcurve-outquint.png
                        \caption
                        Easing curve for a quintic (t^5) function:
                        decelerating to zero velocity.
    \value InOutQuint   \image qeasingcurve-inoutquint.png
                        \caption
                        Easing curve for a quintic (t^5) function:
                        acceleration until halfway, then deceleration.
    \value OutInQuint   \image qeasingcurve-outinquint.png
                        \caption
                        Easing curve for a quintic (t^5) function:
                        deceleration until halfway, then acceleration.
    \value InSine       \image qeasingcurve-insine.png
                        \caption
                        Easing curve for a sinusoidal (sin(t)) function:
                        accelerating from zero velocity.
    \value OutSine      \image qeasingcurve-outsine.png
                        \caption
                        Easing curve for a sinusoidal (sin(t)) function:
                        decelerating from zero velocity.
    \value InOutSine    \image qeasingcurve-inoutsine.png
                        \caption
                        Easing curve for a sinusoidal (sin(t)) function:
                        acceleration until halfway, then deceleration.
    \value OutInSine    \image qeasingcurve-outinsine.png
                        \caption
                        Easing curve for a sinusoidal (sin(t)) function:
                        deceleration until halfway, then acceleration.
    \value InExpo       \image qeasingcurve-inexpo.png
                        \caption
                        Easing curve for an exponential (2^t) function:
                        accelerating from zero velocity.
    \value OutExpo      \image qeasingcurve-outexpo.png
                        \caption
                        Easing curve for an exponential (2^t) function:
                        decelerating from zero velocity.
    \value InOutExpo    \image qeasingcurve-inoutexpo.png
                        \caption
                        Easing curve for an exponential (2^t) function:
                        acceleration until halfway, then deceleration.
    \value OutInExpo    \image qeasingcurve-outinexpo.png
                        \caption
                        Easing curve for an exponential (2^t) function:
                        deceleration until halfway, then acceleration.
    \value InCirc       \image qeasingcurve-incirc.png
                        \caption
                        Easing curve for a circular (sqrt(1-t^2)) function:
                        accelerating from zero velocity.
    \value OutCirc      \image qeasingcurve-outcirc.png
                        \caption
                        Easing curve for a circular (sqrt(1-t^2)) function:
                        decelerating from zero velocity.
    \value InOutCirc    \image qeasingcurve-inoutcirc.png
                        \caption
                        Easing curve for a circular (sqrt(1-t^2)) function:
                        acceleration until halfway, then deceleration.
    \value OutInCirc    \image qeasingcurve-outincirc.png
                        \caption
                        Easing curve for a circular (sqrt(1-t^2)) function:
                        deceleration until halfway, then acceleration.
    \value InElastic    \image qeasingcurve-inelastic.png
                        \caption
                        Easing curve for an elastic
                        (exponentially decaying sine wave) function:
                        accelerating from zero velocity.  The peak amplitude
                        can be set with the \e amplitude parameter, and the
                        period of decay by the \e period parameter.
    \value OutElastic   \image qeasingcurve-outelastic.png
                        \caption
                        Easing curve for an elastic
                        (exponentially decaying sine wave) function:
                        decelerating from zero velocity.  The peak amplitude
                        can be set with the \e amplitude parameter, and the
                        period of decay by the \e period parameter.
    \value InOutElastic \image qeasingcurve-inoutelastic.png
                        \caption
                        Easing curve for an elastic
                        (exponentially decaying sine wave) function:
                        acceleration until halfway, then deceleration.
    \value OutInElastic \image qeasingcurve-outinelastic.png
                        \caption
                        Easing curve for an elastic
                        (exponentially decaying sine wave) function:
                        deceleration until halfway, then acceleration.
    \value InBack       \image qeasingcurve-inback.png
                        \caption
                        Easing curve for a back (overshooting
                        cubic function: (s+1)*t^3 - s*t^2) easing in:
                        accelerating from zero velocity.
    \value OutBack      \image qeasingcurve-outback.png
                        \caption
                        Easing curve for a back (overshooting
                        cubic function: (s+1)*t^3 - s*t^2) easing out:
                        decelerating to zero velocity.
    \value InOutBack    \image qeasingcurve-inoutback.png
                        \caption
                        Easing curve for a back (overshooting
                        cubic function: (s+1)*t^3 - s*t^2) easing in/out:
                        acceleration until halfway, then deceleration.
    \value OutInBack    \image qeasingcurve-outinback.png
                        \caption
                        Easing curve for a back (overshooting
                        cubic easing: (s+1)*t^3 - s*t^2) easing out/in:
                        deceleration until halfway, then acceleration.
    \value InBounce     \image qeasingcurve-inbounce.png
                        \caption
                        Easing curve for a bounce (exponentially
                        decaying parabolic bounce) function: accelerating
                        from zero velocity.
    \value OutBounce    \image qeasingcurve-outbounce.png
                        \caption
                        Easing curve for a bounce (exponentially
                        decaying parabolic bounce) function: decelerating
                        from zero velocity.
    \value InOutBounce  \image qeasingcurve-inoutbounce.png
                        \caption
                        Easing curve for a bounce (exponentially
                        decaying parabolic bounce) function easing in/out:
                        acceleration until halfway, then deceleration.
    \value OutInBounce  \image qeasingcurve-outinbounce.png
                        \caption
                        Easing curve for a bounce (exponentially
                        decaying parabolic bounce) function easing out/in:
                        deceleration until halfway, then acceleration.
    \omitvalue InCurve
    \omitvalue OutCurve
    \omitvalue SineCurve
    \omitvalue CosineCurve
    \value BezierSpline Allows defining a custom easing curve using a cubic bezier spline
                        \sa addCubicBezierSegment()
    \value TCBSpline    Allows defining a custom easing curve using a TCB spline
                        \sa addTCBSegment()
    \value Custom       This is returned if the user specified a custom curve type with
                        setCustomType(). Note that you cannot call setType() with this value,
                        but type() can return it.
    \omitvalue NCurveTypes
*/

/*!
    \typedef QEasingCurve::EasingFunction

    This is a typedef for a pointer to a function with the following
    signature:

    \snippet code/src_corelib_tools_qeasingcurve.cpp 0
*/

#include "qeasingcurve.h"
#include <cmath>

#ifndef QT_NO_DEBUG_STREAM
#include <QtCore/qdebug.h>
#include <QtCore/qstring.h>
#endif

#ifndef QT_NO_DATASTREAM
#include <QtCore/qdatastream.h>
#endif

#include <QtCore/qpoint.h>
#include <QtCore/qvector.h>

QT_BEGIN_NAMESPACE

static bool isConfigFunction(QEasingCurve::Type type)
{
    return (type >= QEasingCurve::InElastic
            && type <= QEasingCurve::OutInBounce) ||
            type == QEasingCurve::BezierSpline ||
            type == QEasingCurve::TCBSpline;
}

struct TCBPoint {
    QPointF _point;
    qreal _t;
    qreal _c;
    qreal _b;

    TCBPoint() {}
    TCBPoint(QPointF point, qreal t, qreal c, qreal b) : _point(point), _t(t), _c(c), _b(b) {}

    bool operator==(const TCBPoint &other) const
    {
        return _point == other._point &&
                qFuzzyCompare(_t, other._t) &&
                qFuzzyCompare(_c, other._c) &&
                qFuzzyCompare(_b, other._b);
    }
};


typedef QVector<TCBPoint> TCBPoints;

class QEasingCurveFunction
{
public:
    enum Type { In, Out, InOut, OutIn };

    QEasingCurveFunction(QEasingCurveFunction::Type type = In, qreal period = 0.3, qreal amplitude = 1.0,
        qreal overshoot = 1.70158)
        : _t(type), _p(period), _a(amplitude), _o(overshoot)
    { }
    virtual ~QEasingCurveFunction() {}
    virtual qreal value(qreal t);
    virtual QEasingCurveFunction *copy() const;
    bool operator==(const QEasingCurveFunction &other) const;

    Type _t;
    qreal _p;
    qreal _a;
    qreal _o;
    QVector<QPointF> _bezierCurves;
    TCBPoints _tcbPoints;

};

qreal QEasingCurveFunction::value(qreal t)
{
    return t;
}

QEasingCurveFunction *QEasingCurveFunction::copy() const
{
    QEasingCurveFunction *rv = new QEasingCurveFunction(_t, _p, _a, _o);
    rv->_bezierCurves = _bezierCurves;
    rv->_tcbPoints = _tcbPoints;
    return rv;
}

bool QEasingCurveFunction::operator==(const QEasingCurveFunction &other) const
{
    return _t == other._t &&
           qFuzzyCompare(_p, other._p) &&
           qFuzzyCompare(_a, other._a) &&
           qFuzzyCompare(_o, other._o) &&
            _bezierCurves == other._bezierCurves &&
            _tcbPoints == other._tcbPoints;
}

QT_BEGIN_INCLUDE_NAMESPACE
#include "../../3rdparty/easing/easing.cpp"
QT_END_INCLUDE_NAMESPACE

class QEasingCurvePrivate
{
public:
    QEasingCurvePrivate()
        : type(QEasingCurve::Linear),
          config(0),
          func(&easeNone)
    { }
    QEasingCurvePrivate(const QEasingCurvePrivate &other)
        : type(other.type),
          config(other.config ? other.config->copy() : 0),
          func(other.func)
    { }
    ~QEasingCurvePrivate() { delete config; }
    void setType_helper(QEasingCurve::Type);

    QEasingCurve::Type type;
    QEasingCurveFunction *config;
    QEasingCurve::EasingFunction func;
};

struct BezierEase : public QEasingCurveFunction
{
    struct SingleCubicBezier {
        qreal p0x, p0y;
        qreal p1x, p1y;
        qreal p2x, p2y;
        qreal p3x, p3y;
    };

    QVector<SingleCubicBezier> _curves;
    QVector<qreal> _intervals;
    int _curveCount;
    bool _init;
    bool _valid;

    BezierEase()
        : QEasingCurveFunction(InOut), _curves(10), _intervals(10), _init(false), _valid(false)
    { }

    void init()
    {
        if (_bezierCurves.last() == QPointF(1.0, 1.0)) {
            _init = true;
            _curveCount = _bezierCurves.count() / 3;

            for (int i=0; i < _curveCount; i++) {
                _intervals[i] = _bezierCurves.at(i * 3 + 2).x();

                if (i == 0) {
                    _curves[0].p0x = 0.0;
                    _curves[0].p0y = 0.0;

                    _curves[0].p1x = _bezierCurves.at(0).x();
                    _curves[0].p1y = _bezierCurves.at(0).y();

                    _curves[0].p2x = _bezierCurves.at(1).x();
                    _curves[0].p2y = _bezierCurves.at(1).y();

                    _curves[0].p3x = _bezierCurves.at(2).x();
                    _curves[0].p3y = _bezierCurves.at(2).y();

                } else if (i == (_curveCount - 1)) {
                    _curves[i].p0x = _bezierCurves.at(_bezierCurves.count() - 4).x();
                    _curves[i].p0y = _bezierCurves.at(_bezierCurves.count() - 4).y();

                    _curves[i].p1x = _bezierCurves.at(_bezierCurves.count() - 3).x();
                    _curves[i].p1y = _bezierCurves.at(_bezierCurves.count() - 3).y();

                    _curves[i].p2x = _bezierCurves.at(_bezierCurves.count() - 2).x();
                    _curves[i].p2y = _bezierCurves.at(_bezierCurves.count() - 2).y();

                    _curves[i].p3x = _bezierCurves.at(_bezierCurves.count() - 1).x();
                    _curves[i].p3y = _bezierCurves.at(_bezierCurves.count() - 1).y();
                } else {
                    _curves[i].p0x = _bezierCurves.at(i * 3 - 1).x();
                    _curves[i].p0y = _bezierCurves.at(i * 3 - 1).y();

                    _curves[i].p1x = _bezierCurves.at(i * 3).x();
                    _curves[i].p1y = _bezierCurves.at(i * 3).y();

                    _curves[i].p2x = _bezierCurves.at(i * 3 + 1).x();
                    _curves[i].p2y = _bezierCurves.at(i * 3 + 1).y();

                    _curves[i].p3x = _bezierCurves.at(i * 3 + 2).x();
                    _curves[i].p3y = _bezierCurves.at(i * 3 + 2).y();
                }
            }
            _valid = true;
        } else {
            _valid = false;
        }
    }

    QEasingCurveFunction *copy() const
    {
        BezierEase *rv = new BezierEase();
        rv->_t = _t;
        rv->_p = _p;
        rv->_a = _a;
        rv->_o = _o;
        rv->_bezierCurves = _bezierCurves;
        rv->_tcbPoints = _tcbPoints;
        return rv;
    }

    void getBezierSegment(SingleCubicBezier * &singleCubicBezier, qreal x)
    {

        int currentSegment = 0;

        while (currentSegment < _curveCount) {
            if (x <= _intervals.data()[currentSegment])
                break;
            currentSegment++;
        }

        singleCubicBezier = &_curves.data()[currentSegment];
    }


    qreal static inline newtonIteration(const SingleCubicBezier &singleCubicBezier, qreal t, qreal x)
    {
        qreal currentXValue = evaluateForX(singleCubicBezier, t);

        const qreal newT = t - (currentXValue - x) / evaluateDerivateForX(singleCubicBezier, t);

        return newT;
    }

    qreal value(qreal x)
    {
        Q_ASSERT(_bezierCurves.count() % 3 == 0);

        if (_bezierCurves.isEmpty()) {
            return x;
        }

        if (!_init)
            init();

        if (!_valid) {
            qWarning("QEasingCurve: Invalid bezier curve");
            return x;
        }
        SingleCubicBezier *singleCubicBezier = 0;
        getBezierSegment(singleCubicBezier, x);

        return evaluateSegmentForY(*singleCubicBezier, findTForX(*singleCubicBezier, x));
    }

    qreal static inline evaluateSegmentForY(const SingleCubicBezier &singleCubicBezier, qreal t)
    {
        const qreal p0 = singleCubicBezier.p0y;
        const qreal p1 = singleCubicBezier.p1y;
        const qreal p2 = singleCubicBezier.p2y;
        const qreal p3 = singleCubicBezier.p3y;

        const qreal s = 1 - t;

        const qreal s_squared = s*s;
        const qreal t_squared = t*t;

        const qreal s_cubic = s_squared * s;
        const qreal t_cubic = t_squared * t;

        return s_cubic * p0 + 3 * s_squared * t * p1 + 3 * s * t_squared * p2 + t_cubic * p3;
    }

    qreal static inline evaluateForX(const SingleCubicBezier &singleCubicBezier, qreal t)
    {
        const qreal p0 = singleCubicBezier.p0x;
        const qreal p1 = singleCubicBezier.p1x;
        const qreal p2 = singleCubicBezier.p2x;
        const qreal p3 = singleCubicBezier.p3x;

        const qreal s = 1 - t;

        const qreal s_squared = s*s;
        const qreal t_squared = t*t;

        const qreal s_cubic = s_squared * s;
        const qreal t_cubic = t_squared * t;

        return s_cubic * p0 + 3 * s_squared * t * p1 + 3 * s * t_squared * p2 + t_cubic * p3;
    }

    qreal static inline evaluateDerivateForX(const SingleCubicBezier &singleCubicBezier, qreal t)
    {
        const qreal p0 = singleCubicBezier.p0x;
        const qreal p1 = singleCubicBezier.p1x;
        const qreal p2 = singleCubicBezier.p2x;
        const qreal p3 = singleCubicBezier.p3x;

        const qreal t_squared = t*t;

        return -3*p0 + 3*p1 + 6*p0*t - 12*p1*t + 6*p2*t + 3*p3*t_squared - 3*p0*t_squared + 9*p1*t_squared - 9*p2*t_squared;
    }

    qreal static inline _cbrt(qreal d)
    {
        qreal sign = 1;
        if (d < 0)
            sign = -1;
        d = d * sign;

        qreal t = _fast_cbrt(d);

        //one step of Halley's Method to get a better approximation
        const qreal t_cubic = t * t * t;
        const qreal f = t_cubic + t_cubic + d;
        if (f != qreal(0.0))
            t = t * (t_cubic + d + d) / f;

        //another step
        /*qreal t_i = t;
         t_i_cubic = pow(t_i, 3);
         t = t_i * (t_i_cubic + d + d) / (t_i_cubic + t_i_cubic + d);*/

        return t * sign;
    }

    float static inline _fast_cbrt(float x)
    {
        union {
            float f;
            quint32 i;
        } ux;

        const unsigned int B1 = 709921077;

        ux.f = x;
        ux.i = (ux.i / 3 + B1);

        return ux.f;
    }

    double static inline _fast_cbrt(double d)
    {
        union {
            double d;
            quint32 pt[2];
        } ut, ux;

        const unsigned int B1 = 715094163;

#if Q_BYTE_ORDER == Q_LITTLE_ENDIAN
        const int h0 = 1;
#else
        const int h0 = 0;
#endif
        ut.d = 0.0;
        ux.d = d;

        quint32 hx = ux.pt[h0]; //high word of d
        ut.pt[h0] = hx / 3 + B1;

        return ut.d;
    }

    qreal static inline _acos(qreal x)
    {
        return sqrt(1-x)*(1.5707963267948966192313216916398f + x*(-0.213300989f + x*(0.077980478f + x*-0.02164095f)));
    }

    qreal static inline _cos(qreal x) //super fast _cos
    {
        const qreal pi_times2 = 2 * M_PI;
        const qreal pi_neg = -1 * M_PI;
        const qreal pi_by2 = M_PI / 2.0;

        x += pi_by2; //the polynom is for sin

        if (x < pi_neg)
            x += pi_times2;
        else if (x > M_PI)
            x -= pi_times2;

        const qreal a = 0.405284735;
        const qreal b = 1.27323954;

        const qreal x_squared = x * x;

        if (x < 0) {
            qreal cos = b * x + a * x_squared;

            if (cos < 0)
                return 0.225 * (cos * -1 * cos - cos) + cos;
            return 0.225 * (cos * cos - cos) + cos;
        } //else

        qreal cos = b * x - a * x_squared;

        if (cos < 0)
            return 0.225 * (cos * 1 *-cos - cos) + cos;
        return 0.225 * (cos * cos - cos) + cos;
    }

    bool static inline inRange(qreal f)
    {
        return (f >= -0.01 && f <= 1.01);
    }

    void static inline cosacos(qreal x, qreal &s1, qreal &s2, qreal &s3 )
    {
        //This function has no proper algebraic representation in real numbers.
        //We use approximations instead

        const qreal x_squared = x * x;
        const qreal x_plus_one_sqrt = sqrt(1.0 + x);
        const qreal one_minus_x_sqrt = sqrt(1.0 - x);

        //cos(acos(x) / 3)
        //s1 = _cos(_acos(x) / 3);
        s1 = 0.463614 - 0.0347815 * x + 0.00218245 * x_squared +  0.402421 * x_plus_one_sqrt;

        //cos(acos((x) -  M_PI) / 3)
        //s3 = _cos((_acos(x) - M_PI) / 3);
        s3 = 0.463614 + 0.402421 * one_minus_x_sqrt + 0.0347815 * x + 0.00218245 * x_squared;

        //cos((acos(x) +  M_PI) / 3)
        //s2 = _cos((_acos(x) + M_PI) / 3);
        s2 = -0.401644 * one_minus_x_sqrt - 0.0686804  * x + 0.401644 * x_plus_one_sqrt;
    }

    qreal static inline singleRealSolutionForCubic(qreal a, qreal b, qreal c)
    {
        //returns the real solutiuon in [0..1]
        //We use the Cardano formula

        //substituiton: x = z - a/3
        // z^3+pz+q=0

        if (c < 0.000001 && c > -0.000001)
            return 0;

        const qreal a_by3 = a / 3.0;

        const qreal a_cubic = a * a * a;

        const qreal p = b - a * a_by3;
        const qreal q = 2.0 * a_cubic / 27.0 - a * b / 3.0 + c;

        const qreal q_squared = q * q;
        const qreal p_cubic = p * p * p;
        const qreal D = 0.25 * q_squared + p_cubic / 27.0;

        if (D >= 0) {
            const qreal D_sqrt = sqrt(D);
            qreal u = _cbrt( -q * 0.5 + D_sqrt);
            qreal v = _cbrt( -q * 0.5 - D_sqrt);
            qreal z1 = u + v;

            qreal t1 = z1 - a_by3;

            if (inRange(t1))
                return t1;
            qreal z2 = -1 *u;
            qreal t2 = z2 - a_by3;
            return t2;
        }

        //casus irreducibilis
        const qreal p_minus_sqrt = sqrt(-p);

        //const qreal f = sqrt(4.0 / 3.0 * -p);
        const qreal f = sqrt(4.0 / 3.0) * p_minus_sqrt;

        //const qreal sqrtP = sqrt(27.0 / -p_cubic);
        const qreal sqrtP = -3.0*sqrt(3.0) / (p_minus_sqrt * p);


        const qreal g = -q * 0.5 * sqrtP;

        qreal s1;
        qreal s2;
        qreal s3;

        cosacos(g, s1, s2, s3);

        qreal z1 = -1* f * s2;
        qreal t1 = z1 - a_by3;
        if (inRange(t1))
            return t1;

        qreal z2 = f * s1;
        qreal t2 = z2 - a_by3;
        if (inRange(t2))
            return t2;

        qreal z3 = -1 * f * s3;
        qreal t3 = z3 - a_by3;
        return t3;
    }

     qreal static inline findTForX(const SingleCubicBezier &singleCubicBezier, qreal x)
     {
         const qreal p0 = singleCubicBezier.p0x;
         const qreal p1 = singleCubicBezier.p1x;
         const qreal p2 = singleCubicBezier.p2x;
         const qreal p3 = singleCubicBezier.p3x;

         const qreal factorT3 = p3 - p0 + 3 * p1 - 3 * p2;
         const qreal factorT2 = 3 * p0 - 6 * p1 + 3 * p2;
         const qreal factorT1 = -3 * p0 + 3 * p1;
         const qreal factorT0 = p0 - x;

         const qreal a = factorT2 / factorT3;
         const qreal b = factorT1 / factorT3;
         const qreal c = factorT0 / factorT3;

         return singleRealSolutionForCubic(a, b, c);

         //one new iteration to increase numeric stability
         //return newtonIteration(singleCubicBezier, t, x);
     }
};

struct TCBEase : public BezierEase
{
    qreal value(qreal x)
    {
        Q_ASSERT(_bezierCurves.count() % 3 == 0);

        if (_bezierCurves.isEmpty()) {
            qWarning("QEasingCurve: Invalid tcb curve");
            return x;
        }

        return BezierEase::value(x);
    }

};

struct ElasticEase : public QEasingCurveFunction
{
    ElasticEase(Type type)
        : QEasingCurveFunction(type, qreal(0.3), qreal(1.0))
    { }

    QEasingCurveFunction *copy() const
    {
        ElasticEase *rv = new ElasticEase(_t);
        rv->_p = _p;
        rv->_a = _a;
        rv->_bezierCurves = _bezierCurves;
        rv->_tcbPoints = _tcbPoints;
        return rv;
    }

    qreal value(qreal t)
    {
        qreal p = (_p < 0) ? qreal(0.3) : _p;
        qreal a = (_a < 0) ? qreal(1.0) : _a;
        switch(_t) {
        case In:
            return easeInElastic(t, a, p);
        case Out:
            return easeOutElastic(t, a, p);
        case InOut:
            return easeInOutElastic(t, a, p);
        case OutIn:
            return easeOutInElastic(t, a, p);
        default:
            return t;
        }
    }
};

struct BounceEase : public QEasingCurveFunction
{
    BounceEase(Type type)
        : QEasingCurveFunction(type, qreal(0.3), qreal(1.0))
    { }

    QEasingCurveFunction *copy() const
    {
        BounceEase *rv = new BounceEase(_t);
        rv->_a = _a;
        rv->_bezierCurves = _bezierCurves;
        rv->_tcbPoints = _tcbPoints;
        return rv;
    }

    qreal value(qreal t)
    {
        qreal a = (_a < 0) ? qreal(1.0) : _a;
        switch(_t) {
        case In:
            return easeInBounce(t, a);
        case Out:
            return easeOutBounce(t, a);
        case InOut:
            return easeInOutBounce(t, a);
        case OutIn:
            return easeOutInBounce(t, a);
        default:
            return t;
        }
    }
};

struct BackEase : public QEasingCurveFunction
{
    BackEase(Type type)
        : QEasingCurveFunction(type, qreal(0.3), qreal(1.0), qreal(1.70158))
    { }

    QEasingCurveFunction *copy() const
    {
        BackEase *rv = new BackEase(_t);
        rv->_o = _o;
        rv->_bezierCurves = _bezierCurves;
        rv->_tcbPoints = _tcbPoints;
        return rv;
    }

    qreal value(qreal t)
    {
        qreal o = (_o < 0) ? qreal(1.70158) : _o;
        switch(_t) {
        case In:
            return easeInBack(t, o);
        case Out:
            return easeOutBack(t, o);
        case InOut:
            return easeInOutBack(t, o);
        case OutIn:
            return easeOutInBack(t, o);
        default:
            return t;
        }
    }
};

static QEasingCurve::EasingFunction curveToFunc(QEasingCurve::Type curve)
{
    switch(curve) {
    case QEasingCurve::Linear:
        return &easeNone;
    case QEasingCurve::InQuad:
        return &easeInQuad;
    case QEasingCurve::OutQuad:
        return &easeOutQuad;
    case QEasingCurve::InOutQuad:
        return &easeInOutQuad;
    case QEasingCurve::OutInQuad:
        return &easeOutInQuad;
    case QEasingCurve::InCubic:
        return &easeInCubic;
    case QEasingCurve::OutCubic:
        return &easeOutCubic;
    case QEasingCurve::InOutCubic:
        return &easeInOutCubic;
    case QEasingCurve::OutInCubic:
        return &easeOutInCubic;
    case QEasingCurve::InQuart:
        return &easeInQuart;
    case QEasingCurve::OutQuart:
        return &easeOutQuart;
    case QEasingCurve::InOutQuart:
        return &easeInOutQuart;
    case QEasingCurve::OutInQuart:
        return &easeOutInQuart;
    case QEasingCurve::InQuint:
        return &easeInQuint;
    case QEasingCurve::OutQuint:
        return &easeOutQuint;
    case QEasingCurve::InOutQuint:
        return &easeInOutQuint;
    case QEasingCurve::OutInQuint:
        return &easeOutInQuint;
    case QEasingCurve::InSine:
        return &easeInSine;
    case QEasingCurve::OutSine:
        return &easeOutSine;
    case QEasingCurve::InOutSine:
        return &easeInOutSine;
    case QEasingCurve::OutInSine:
        return &easeOutInSine;
    case QEasingCurve::InExpo:
        return &easeInExpo;
    case QEasingCurve::OutExpo:
        return &easeOutExpo;
    case QEasingCurve::InOutExpo:
        return &easeInOutExpo;
    case QEasingCurve::OutInExpo:
        return &easeOutInExpo;
    case QEasingCurve::InCirc:
        return &easeInCirc;
    case QEasingCurve::OutCirc:
        return &easeOutCirc;
    case QEasingCurve::InOutCirc:
        return &easeInOutCirc;
    case QEasingCurve::OutInCirc:
        return &easeOutInCirc;
    // Internal for, compatibility with QTimeLine only ??
    case QEasingCurve::InCurve:
        return &easeInCurve;
    case QEasingCurve::OutCurve:
        return &easeOutCurve;
    case QEasingCurve::SineCurve:
        return &easeSineCurve;
    case QEasingCurve::CosineCurve:
        return &easeCosineCurve;
    default:
        return 0;
    };
}

static QEasingCurveFunction *curveToFunctionObject(QEasingCurve::Type type)
{
    QEasingCurveFunction *curveFunc = 0;
    switch(type) {
    case QEasingCurve::InElastic:
        curveFunc = new ElasticEase(ElasticEase::In);
        break;
    case QEasingCurve::OutElastic:
        curveFunc = new ElasticEase(ElasticEase::Out);
        break;
    case QEasingCurve::InOutElastic:
        curveFunc = new ElasticEase(ElasticEase::InOut);
        break;
    case QEasingCurve::OutInElastic:
        curveFunc = new ElasticEase(ElasticEase::OutIn);
        break;
    case QEasingCurve::OutBounce:
        curveFunc = new BounceEase(BounceEase::Out);
        break;
    case QEasingCurve::InBounce:
        curveFunc = new BounceEase(BounceEase::In);
        break;
    case QEasingCurve::OutInBounce:
        curveFunc = new BounceEase(BounceEase::OutIn);
        break;
    case QEasingCurve::InOutBounce:
        curveFunc = new BounceEase(BounceEase::InOut);
        break;
    case QEasingCurve::InBack:
        curveFunc = new BackEase(BackEase::In);
        break;
    case QEasingCurve::OutBack:
        curveFunc = new BackEase(BackEase::Out);
        break;
    case QEasingCurve::InOutBack:
        curveFunc = new BackEase(BackEase::InOut);
        break;
    case QEasingCurve::OutInBack:
        curveFunc = new BackEase(BackEase::OutIn);
        break;
    case QEasingCurve::BezierSpline:
        curveFunc = new BezierEase();
        break;
    case QEasingCurve::TCBSpline:
        curveFunc = new TCBEase();
        break;
    default:
        curveFunc = new QEasingCurveFunction(QEasingCurveFunction::In, qreal(0.3), qreal(1.0), qreal(1.70158));
    }

    return curveFunc;
}

/*!
    \fn QEasingCurve::QEasingCurve(QEasingCurve &&other)

    Move-constructs a QEasingCurve instance, making it point at the same
    object that \a other was pointing to.

    \since 5.2
*/

/*!
    Constructs an easing curve of the given \a type.
 */
QEasingCurve::QEasingCurve(Type type)
    : d_ptr(new QEasingCurvePrivate)
{
    setType(type);
}

/*!
    Construct a copy of \a other.
 */
QEasingCurve::QEasingCurve(const QEasingCurve &other)
    : d_ptr(new QEasingCurvePrivate(*other.d_ptr))
{
    // ### non-atomic, requires malloc on shallow copy
}

/*!
    Destructor.
 */

QEasingCurve::~QEasingCurve()
{
    delete d_ptr;
}

/*!
    \fn QEasingCurve &QEasingCurve::operator=(const QEasingCurve &other)
    Copy \a other.
 */

/*!
    \fn QEasingCurve &QEasingCurve::operator=(QEasingCurve &&other)

    Move-assigns \a other to this QEasingCurve instance.

    \since 5.2
*/

/*!
    \fn void QEasingCurve::swap(QEasingCurve &other)
    \since 5.0

    Swaps curve \a other with this curve. This operation is very
    fast and never fails.
*/

/*!
    Compare this easing curve with \a other and returns \c true if they are
    equal. It will also compare the properties of a curve.
 */
bool QEasingCurve::operator==(const QEasingCurve &other) const
{
    bool res = d_ptr->func == other.d_ptr->func
            && d_ptr->type == other.d_ptr->type;
    if (res) {
        if (d_ptr->config && other.d_ptr->config) {
        // catch the config content
            res = d_ptr->config->operator==(*(other.d_ptr->config));

        } else if (d_ptr->config || other.d_ptr->config) {
        // one one has a config object, which could contain default values
            res = qFuzzyCompare(amplitude(), other.amplitude()) &&
                  qFuzzyCompare(period(), other.period()) &&
                  qFuzzyCompare(overshoot(), other.overshoot());
        }
    }
    return res;
}

/*!
    \fn bool QEasingCurve::operator!=(const QEasingCurve &other) const
    Compare this easing curve with \a other and returns \c true if they are not equal.
    It will also compare the properties of a curve.

    \sa operator==()
*/

/*!
    Returns the amplitude. This is not applicable for all curve types.
    It is only applicable for bounce and elastic curves (curves of type()
    QEasingCurve::InBounce, QEasingCurve::OutBounce, QEasingCurve::InOutBounce,
    QEasingCurve::OutInBounce, QEasingCurve::InElastic, QEasingCurve::OutElastic,
    QEasingCurve::InOutElastic or QEasingCurve::OutInElastic).
 */
qreal QEasingCurve::amplitude() const
{
    return d_ptr->config ? d_ptr->config->_a : qreal(1.0);
}

/*!
    Sets the amplitude to \a amplitude.

    This will set the amplitude of the bounce or the amplitude of the
    elastic "spring" effect. The higher the number, the higher the amplitude.
    \sa amplitude()
*/
void QEasingCurve::setAmplitude(qreal amplitude)
{
    if (!d_ptr->config)
        d_ptr->config = curveToFunctionObject(d_ptr->type);
    d_ptr->config->_a = amplitude;
}

/*!
    Returns the period. This is not applicable for all curve types.
    It is only applicable if type() is QEasingCurve::InElastic, QEasingCurve::OutElastic,
    QEasingCurve::InOutElastic or QEasingCurve::OutInElastic.
 */
qreal QEasingCurve::period() const
{
    return d_ptr->config ? d_ptr->config->_p : qreal(0.3);
}

/*!
    Sets the period to \a period.
    Setting a small period value will give a high frequency of the curve. A
    large period will give it a small frequency.

    \sa period()
*/
void QEasingCurve::setPeriod(qreal period)
{
    if (!d_ptr->config)
        d_ptr->config = curveToFunctionObject(d_ptr->type);
    d_ptr->config->_p = period;
}

/*!
    Returns the overshoot. This is not applicable for all curve types.
    It is only applicable if type() is QEasingCurve::InBack, QEasingCurve::OutBack,
    QEasingCurve::InOutBack or QEasingCurve::OutInBack.
 */
qreal QEasingCurve::overshoot() const
{
    return d_ptr->config ? d_ptr->config->_o : qreal(1.70158) ;
}

/*!
    Sets the overshoot to \a overshoot.

    0 produces no overshoot, and the default value of 1.70158 produces an overshoot of 10 percent.

    \sa overshoot()
*/
void QEasingCurve::setOvershoot(qreal overshoot)
{
    if (!d_ptr->config)
        d_ptr->config = curveToFunctionObject(d_ptr->type);
    d_ptr->config->_o = overshoot;
}

/*!
    Adds a segment of a cubic bezier spline to define a custom easing curve.
    It is only applicable if type() is QEasingCurve::BezierSpline.
    Note that the spline implicitly starts at (0.0, 0.0) and has to end at (1.0, 1.0) to
    be a valid easing curve.
    \a c1 and \a c2 are the control points used for drawing the curve.
    \a endPoint is the endpoint of the curve.
 */
void QEasingCurve::addCubicBezierSegment(const QPointF & c1, const QPointF & c2, const QPointF & endPoint)
{
    if (!d_ptr->config)
        d_ptr->config = curveToFunctionObject(d_ptr->type);
    d_ptr->config->_bezierCurves << c1 << c2 << endPoint;
}

QVector<QPointF> static inline tcbToBezier(const TCBPoints &tcbPoints)
{
    const int count = tcbPoints.count();
    QVector<QPointF> bezierPoints;

    for (int i = 1; i < count; i++) {
        const qreal t_0 = tcbPoints.at(i - 1)._t;
        const qreal c_0 = tcbPoints.at(i - 1)._c;
        qreal b_0 = -1;

        qreal const t_1 = tcbPoints.at(i)._t;
        qreal const c_1 = tcbPoints.at(i)._c;
        qreal b_1 = 1;

        QPointF c_minusOne;                                   //P1 last segment - not available for the first point
        const QPointF c0(tcbPoints.at(i - 1)._point);         //P0 Hermite/TBC
        const QPointF c3(tcbPoints.at(i)._point);             //P1 Hermite/TBC
        QPointF c4;                                           //P0 next segment - not available for the last point

        if (i > 1) { //first point no left tangent
            c_minusOne = tcbPoints.at(i - 2)._point;
            b_0 = tcbPoints.at(i - 1)._b;
        }

        if (i < (count - 1)) { //last point no right tangent
            c4 = tcbPoints.at(i + 1)._point;
            b_1 = tcbPoints.at(i)._b;
        }

        const qreal dx_0 = 0.5 * (1-t_0) *  ((1 + b_0) * (1 + c_0) * (c0.x() - c_minusOne.x()) + (1- b_0) * (1 - c_0) * (c3.x() - c0.x()));
        const qreal dy_0 = 0.5 * (1-t_0) *  ((1 + b_0) * (1 + c_0) * (c0.y() - c_minusOne.y()) + (1- b_0) * (1 - c_0) * (c3.y() - c0.y()));

        const qreal dx_1 =  0.5 * (1-t_1) * ((1 + b_1) * (1 - c_1) * (c3.x() - c0.x()) + (1 - b_1) * (1 + c_1) * (c4.x() - c3.x()));
        const qreal dy_1 =  0.5 * (1-t_1) * ((1 + b_1) * (1 - c_1) * (c3.y() - c0.y()) + (1 - b_1) * (1 + c_1) * (c4.y() - c3.y()));

        const QPointF d_0 = QPointF(dx_0, dy_0);
        const QPointF d_1 = QPointF(dx_1, dy_1);

        QPointF c1 = (3 * c0 + d_0) / 3;
        QPointF c2 = (3 * c3 - d_1) / 3;
        bezierPoints << c1 << c2 << c3;
    }
    return bezierPoints;
}

/*!
    Adds a segment of a TCB bezier spline to define a custom easing curve.
    It is only applicable if type() is QEasingCurve::TCBSpline.
    The spline has to start explitly at (0.0, 0.0) and has to end at (1.0, 1.0) to
    be a valid easing curve.
    The tension \a t changes the length of the tangent vector.
    The continuity \a c changes the sharpness in change between the tangents.
    The bias \a b changes the direction of the tangent vector.
    \a nextPoint is the sample position.
    All three parameters are valid between -1 and 1 and define the
    tangent of the control point.
    If all three parameters are 0 the resulting spline is a Catmull-Rom spline.
    The begin and endpoint always have a bias of -1 and 1, since the outer tangent is not defined.
 */
void QEasingCurve::addTCBSegment(const QPointF &nextPoint, qreal t, qreal c, qreal b)
{
    if (!d_ptr->config)
        d_ptr->config = curveToFunctionObject(d_ptr->type);

    d_ptr->config->_tcbPoints.append(TCBPoint(nextPoint, t, c ,b));

    if (nextPoint == QPointF(1.0, 1.0)) {
        d_ptr->config->_bezierCurves = tcbToBezier(d_ptr->config->_tcbPoints);
        d_ptr->config->_tcbPoints.clear();
    }

}

/*!
    \fn QList<QPointF> QEasingCurve::cubicBezierSpline() const
    \obsolete Use toCubicSpline() instead.
 */

/*!
    \since 5.0

    Returns the cubicBezierSpline that defines a custom easing curve.
    If the easing curve does not have a custom bezier easing curve the list
    is empty.
*/
QVector<QPointF> QEasingCurve::toCubicSpline() const
{
    return d_ptr->config ? d_ptr->config->_bezierCurves : QVector<QPointF>();
}

/*!
    Returns the type of the easing curve.
*/
QEasingCurve::Type QEasingCurve::type() const
{
    return d_ptr->type;
}

void QEasingCurvePrivate::setType_helper(QEasingCurve::Type newType)
{
    qreal amp = -1.0;
    qreal period = -1.0;
    qreal overshoot = -1.0;
    QVector<QPointF> bezierCurves;
    QVector<TCBPoint> tcbPoints;

    if (config) {
        amp = config->_a;
        period = config->_p;
        overshoot = config->_o;
        bezierCurves = config->_bezierCurves;
        tcbPoints = config->_tcbPoints;

        delete config;
        config = 0;
    }

    if (isConfigFunction(newType) || (amp != -1.0) || (period != -1.0) || (overshoot != -1.0) ||
        !bezierCurves.isEmpty()) {
        config = curveToFunctionObject(newType);
        if (amp != -1.0)
            config->_a = amp;
        if (period != -1.0)
            config->_p = period;
        if (overshoot != -1.0)
            config->_o = overshoot;
        config->_bezierCurves = bezierCurves;
        config->_tcbPoints = tcbPoints;
        func = 0;
    } else if (newType != QEasingCurve::Custom) {
        func = curveToFunc(newType);
    }
    Q_ASSERT((func == 0) == (config != 0));
    type = newType;
}

/*!
    Sets the type of the easing curve to \a type.
*/
void QEasingCurve::setType(Type type)
{
    if (d_ptr->type == type)
        return;
    if (type < Linear || type >= NCurveTypes - 1) {
        qWarning("QEasingCurve: Invalid curve type %d", type);
        return;
    }

    d_ptr->setType_helper(type);
}

/*!
    Sets a custom easing curve that is defined by the user in the function \a func.
    The signature of the function is qreal myEasingFunction(qreal progress),
    where \e progress and the return value are considered to be normalized between 0 and 1.
    (In some cases the return value can be outside that range)
    After calling this function type() will return QEasingCurve::Custom.
    \a func cannot be zero.

    \sa customType()
    \sa valueForProgress()
*/
void QEasingCurve::setCustomType(EasingFunction func)
{
    if (!func) {
        qWarning("Function pointer must not be null");
        return;
    }
    d_ptr->func = func;
    d_ptr->setType_helper(Custom);
}

/*!
    Returns the function pointer to the custom easing curve.
    If type() does not return QEasingCurve::Custom, this function
    will return 0.
*/
QEasingCurve::EasingFunction QEasingCurve::customType() const
{
    return d_ptr->type == Custom ? d_ptr->func : 0;
}

/*!
    Return the effective progress for the easing curve at \a progress.
    Whereas \a progress must be between 0 and 1, the returned effective progress
    can be outside those bounds. For example, QEasingCurve::InBack will
    return negative values in the beginning of the function.
 */
qreal QEasingCurve::valueForProgress(qreal progress) const
{
    progress = qBound<qreal>(0, progress, 1);
    if (d_ptr->func)
        return d_ptr->func(progress);
    else if (d_ptr->config)
        return d_ptr->config->value(progress);
    else
        return progress;
}

#ifndef QT_NO_DEBUG_STREAM
QDebug operator<<(QDebug debug, const QEasingCurve &item)
{
    debug << "type:" << item.d_ptr->type
          << "func:" << item.d_ptr->func;
    if (item.d_ptr->config) {
        debug << QString::fromLatin1("period:%1").arg(item.d_ptr->config->_p, 0, 'f', 20)
              << QString::fromLatin1("amp:%1").arg(item.d_ptr->config->_a, 0, 'f', 20)
              << QString::fromLatin1("overshoot:%1").arg(item.d_ptr->config->_o, 0, 'f', 20);
    }
    return debug;
}
#endif // QT_NO_DEBUG_STREAM

#ifndef QT_NO_DATASTREAM
/*!
    \fn QDataStream &operator<<(QDataStream &stream, const QEasingCurve &easing)
    \relates QEasingCurve

    Writes the given \a easing curve to the given \a stream and returns a
    reference to the stream.

    \sa {Serializing Qt Data Types}
*/

QDataStream &operator<<(QDataStream &stream, const QEasingCurve &easing)
{
    stream << quint8(easing.d_ptr->type);
    stream << quint64(quintptr(easing.d_ptr->func));

    bool hasConfig = easing.d_ptr->config;
    stream << hasConfig;
    if (hasConfig) {
        stream << easing.d_ptr->config->_p;
        stream << easing.d_ptr->config->_a;
        stream << easing.d_ptr->config->_o;
    }
    return stream;
}

/*!
    \fn QDataStream &operator>>(QDataStream &stream, QEasingCurve &easing)
    \relates QEasingCurve

    Reads an easing curve from the given \a stream into the given \a
    easing curve and returns a reference to the stream.

    \sa {Serializing Qt Data Types}
*/

QDataStream &operator>>(QDataStream &stream, QEasingCurve &easing)
{
    QEasingCurve::Type type;
    quint8 int_type;
    stream >> int_type;
    type = static_cast<QEasingCurve::Type>(int_type);
    easing.setType(type);

    quint64 ptr_func;
    stream >> ptr_func;
    easing.d_ptr->func = QEasingCurve::EasingFunction(quintptr(ptr_func));

    bool hasConfig;
    stream >> hasConfig;
    if (hasConfig) {
        QEasingCurveFunction *config = curveToFunctionObject(type);
        stream >> config->_p;
        stream >> config->_a;
        stream >> config->_o;
        easing.d_ptr->config = config;
    }
    return stream;
}
#endif // QT_NO_DATASTREAM

QT_END_NAMESPACE
