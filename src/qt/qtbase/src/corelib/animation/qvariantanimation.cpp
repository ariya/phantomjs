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

#include "qvariantanimation.h"
#include "qvariantanimation_p.h"

#include <QtCore/qrect.h>
#include <QtCore/qline.h>
#include <QtCore/qmutex.h>

#include <algorithm>

#ifndef QT_NO_ANIMATION

QT_BEGIN_NAMESPACE

/*!
    \class QVariantAnimation
    \inmodule QtCore
    \ingroup animation
    \brief The QVariantAnimation class provides an abstract base class for animations.
    \since 4.6

    This class is part of \l{The Animation Framework}. It serves as a
    base class for property and item animations, with functions for
    shared functionality.

    QVariantAnimation cannot be used directly as it is an abstract
    class; it has a pure virtual method called updateCurrentValue().
    The class performs interpolation over
    \l{QVariant}s, but leaves using the interpolated values to its
    subclasses. Currently, Qt provides QPropertyAnimation, which
    animates Qt \l{Qt's Property System}{properties}. See the
    QPropertyAnimation class description if you wish to animate such
    properties.

    You can then set start and end values for the property by calling
    setStartValue() and setEndValue(), and finally call start() to
    start the animation. QVariantAnimation will interpolate the
    property of the target object and emit valueChanged(). To react to
    a change in the current value you have to reimplement the
    updateCurrentValue() virtual function.

    It is also possible to set values at specified steps situated
    between the start and end value. The interpolation will then
    touch these points at the specified steps. Note that the start and
    end values are defined as the key values at 0.0 and 1.0.

    There are two ways to affect how QVariantAnimation interpolates
    the values. You can set an easing curve by calling
    setEasingCurve(), and configure the duration by calling
    setDuration(). You can change how the QVariants are interpolated
    by creating a subclass of QVariantAnimation, and reimplementing
    the virtual interpolated() function.

    Subclassing QVariantAnimation can be an alternative if you have
    \l{QVariant}s that you do not wish to declare as Qt properties.
    Note, however, that you in most cases will be better off declaring
    your QVariant as a property.

    Not all QVariant types are supported. Below is a list of currently
    supported QVariant types:

    \list
        \li \l{QMetaType::}{Int}
        \li \l{QMetaType::}{UInt}
        \li \l{QMetaType::}{Double}
        \li \l{QMetaType::}{Float}
        \li \l{QMetaType::}{QLine}
        \li \l{QMetaType::}{QLineF}
        \li \l{QMetaType::}{QPoint}
        \li \l{QMetaType::}{QPointF}
        \li \l{QMetaType::}{QSize}
        \li \l{QMetaType::}{QSizeF}
        \li \l{QMetaType::}{QRect}
        \li \l{QMetaType::}{QRectF}
        \li \l{QMetaType::}{QColor}
    \endlist

    If you need to interpolate other variant types, including custom
    types, you have to implement interpolation for these yourself.
    To do this, you can register an interpolator function for a given
    type. This function takes 3 parameters: the start value, the end value
    and the current progress.

    Example:
    \code
        QVariant myColorInterpolator(const QColor &start, const QColor &end, qreal progress)
        {
            ...
            return QColor(...);
        }
        ...
        qRegisterAnimationInterpolator<QColor>(myColorInterpolator);
    \endcode

    Another option is to reimplement interpolated(), which returns
    interpolation values for the value being interpolated.

    \omit We need some snippets around here. \endomit

    \sa QPropertyAnimation, QAbstractAnimation, {The Animation Framework}
*/

/*!
    \fn void QVariantAnimation::valueChanged(const QVariant &value)

    QVariantAnimation emits this signal whenever the current \a value changes.

    \sa currentValue, startValue, endValue
*/

/*!
    This virtual function is called every time the animation's current
    value changes. The \a value argument is the new current value.

    The base class implementation does nothing.

    \sa currentValue
*/
void QVariantAnimation::updateCurrentValue(const QVariant &) {}

static bool animationValueLessThan(const QVariantAnimation::KeyValue &p1, const QVariantAnimation::KeyValue &p2)
{
    return p1.first < p2.first;
}

static QVariant defaultInterpolator(const void *, const void *, qreal)
{
    return QVariant();
}

template<> Q_INLINE_TEMPLATE QRect _q_interpolate(const QRect &f, const QRect &t, qreal progress)
{
    QRect ret;
    ret.setCoords(_q_interpolate(f.left(), t.left(), progress),
                  _q_interpolate(f.top(), t.top(), progress),
                  _q_interpolate(f.right(), t.right(), progress),
                  _q_interpolate(f.bottom(), t.bottom(), progress));
    return ret;
}

template<> Q_INLINE_TEMPLATE QRectF _q_interpolate(const QRectF &f, const QRectF &t, qreal progress)
{
    qreal x1, y1, w1, h1;
    f.getRect(&x1, &y1, &w1, &h1);
    qreal x2, y2, w2, h2;
    t.getRect(&x2, &y2, &w2, &h2);
    return QRectF(_q_interpolate(x1, x2, progress), _q_interpolate(y1, y2, progress),
                  _q_interpolate(w1, w2, progress), _q_interpolate(h1, h2, progress));
}

template<> Q_INLINE_TEMPLATE QLine _q_interpolate(const QLine &f, const QLine &t, qreal progress)
{
    return QLine( _q_interpolate(f.p1(), t.p1(), progress), _q_interpolate(f.p2(), t.p2(), progress));
}

template<> Q_INLINE_TEMPLATE QLineF _q_interpolate(const QLineF &f, const QLineF &t, qreal progress)
{
    return QLineF( _q_interpolate(f.p1(), t.p1(), progress), _q_interpolate(f.p2(), t.p2(), progress));
}

QVariantAnimationPrivate::QVariantAnimationPrivate() : duration(250), interpolator(&defaultInterpolator)
{ }

void QVariantAnimationPrivate::convertValues(int t)
{
    //this ensures that all the keyValues are of type t
    for (int i = 0; i < keyValues.count(); ++i) {
        QVariantAnimation::KeyValue &pair = keyValues[i];
        pair.second.convert(static_cast<QVariant::Type>(t));
    }
    //we also need update to the current interval if needed
    currentInterval.start.second.convert(static_cast<QVariant::Type>(t));
    currentInterval.end.second.convert(static_cast<QVariant::Type>(t));

    //... and the interpolator
    updateInterpolator();
}

void QVariantAnimationPrivate::updateInterpolator()
{
    int type = currentInterval.start.second.userType();
    if (type == currentInterval.end.second.userType())
        interpolator = getInterpolator(type);
    else
        interpolator = 0;

    //we make sure that the interpolator is always set to something
    if (!interpolator)
        interpolator = &defaultInterpolator;
}

/*!
    \internal
    The goal of this function is to update the currentInterval member. As a consequence, we also
    need to update the currentValue.
    Set \a force to true to always recalculate the interval.
*/
void QVariantAnimationPrivate::recalculateCurrentInterval(bool force/*=false*/)
{
    // can't interpolate if we don't have at least 2 values
    if ((keyValues.count() + (defaultStartEndValue.isValid() ? 1 : 0)) < 2)
        return;

    const qreal endProgress = (direction == QAbstractAnimation::Forward) ? qreal(1) : qreal(0);
    const qreal progress = easing.valueForProgress(((duration == 0) ? endProgress : qreal(currentTime) / qreal(duration)));

    //0 and 1 are still the boundaries
    if (force || (currentInterval.start.first > 0 && progress < currentInterval.start.first)
        || (currentInterval.end.first < 1 && progress > currentInterval.end.first)) {
        //let's update currentInterval
        QVariantAnimation::KeyValues::const_iterator it = std::lower_bound(keyValues.constBegin(),
                                                                           keyValues.constEnd(),
                                                                           qMakePair(progress, QVariant()),
                                                                           animationValueLessThan);
        if (it == keyValues.constBegin()) {
            //the item pointed to by it is the start element in the range
            if (it->first == 0 && keyValues.count() > 1) {
                currentInterval.start = *it;
                currentInterval.end = *(it+1);
            } else {
                currentInterval.start = qMakePair(qreal(0), defaultStartEndValue);
                currentInterval.end = *it;
            }
        } else if (it == keyValues.constEnd()) {
            --it; //position the iterator on the last item
            if (it->first == 1 && keyValues.count() > 1) {
                //we have an end value (item with progress = 1)
                currentInterval.start = *(it-1);
                currentInterval.end = *it;
            } else {
                //we use the default end value here
                currentInterval.start = *it;
                currentInterval.end = qMakePair(qreal(1), defaultStartEndValue);
            }
        } else {
            currentInterval.start = *(it-1);
            currentInterval.end = *it;
        }

        // update all the values of the currentInterval
        updateInterpolator();
    }
    setCurrentValueForProgress(progress);
}

void QVariantAnimationPrivate::setCurrentValueForProgress(const qreal progress)
{
    Q_Q(QVariantAnimation);

    const qreal startProgress = currentInterval.start.first;
    const qreal endProgress = currentInterval.end.first;
    const qreal localProgress = (progress - startProgress) / (endProgress - startProgress);

    QVariant ret = q->interpolated(currentInterval.start.second,
                                   currentInterval.end.second,
                                   localProgress);
    qSwap(currentValue, ret);
    q->updateCurrentValue(currentValue);
    static QBasicAtomicInt changedSignalIndex = Q_BASIC_ATOMIC_INITIALIZER(0);
    if (!changedSignalIndex.load()) {
        //we keep the mask so that we emit valueChanged only when needed (for performance reasons)
        changedSignalIndex.testAndSetRelaxed(0, signalIndex("valueChanged(QVariant)"));
    }
    if (isSignalConnected(changedSignalIndex.load()) && currentValue != ret) {
        //the value has changed
        emit q->valueChanged(currentValue);
    }
}

QVariant QVariantAnimationPrivate::valueAt(qreal step) const
{
    QVariantAnimation::KeyValues::const_iterator result =
        std::lower_bound(keyValues.constBegin(), keyValues.constEnd(), qMakePair(step, QVariant()), animationValueLessThan);
    if (result != keyValues.constEnd() && !animationValueLessThan(qMakePair(step, QVariant()), *result))
        return result->second;

    return QVariant();
}

void QVariantAnimationPrivate::setValueAt(qreal step, const QVariant &value)
{
    if (step < qreal(0.0) || step > qreal(1.0)) {
        qWarning("QVariantAnimation::setValueAt: invalid step = %f", step);
        return;
    }

    QVariantAnimation::KeyValue pair(step, value);

    QVariantAnimation::KeyValues::iterator result = std::lower_bound(keyValues.begin(), keyValues.end(), pair, animationValueLessThan);
    if (result == keyValues.end() || result->first != step) {
        keyValues.insert(result, pair);
    } else {
        if (value.isValid())
            result->second = value; // replaces the previous value
        else
            keyValues.erase(result); // removes the previous value
    }

    recalculateCurrentInterval(/*force=*/true);
}

void QVariantAnimationPrivate::setDefaultStartEndValue(const QVariant &value)
{
    defaultStartEndValue = value;
    recalculateCurrentInterval(/*force=*/true);
}

/*!
    Construct a QVariantAnimation object. \a parent is passed to QAbstractAnimation's
    constructor.
*/
QVariantAnimation::QVariantAnimation(QObject *parent) : QAbstractAnimation(*new QVariantAnimationPrivate, parent)
{
}

/*!
    \internal
*/
QVariantAnimation::QVariantAnimation(QVariantAnimationPrivate &dd, QObject *parent) : QAbstractAnimation(dd, parent)
{
}

/*!
    Destroys the animation.
*/
QVariantAnimation::~QVariantAnimation()
{
}

/*!
    \property QVariantAnimation::easingCurve
    \brief the easing curve of the animation

    This property defines the easing curve of the animation. By
    default, a linear easing curve is used, resulting in linear
    interpolation. Other curves are provided, for instance,
    QEasingCurve::InCirc, which provides a circular entry curve.
    Another example is QEasingCurve::InOutElastic, which provides an
    elastic effect on the values of the interpolated variant.

    QVariantAnimation will use the QEasingCurve::valueForProgress() to
    transform the "normalized progress" (currentTime / totalDuration)
    of the animation into the effective progress actually
    used by the animation. It is this effective progress that will be
    the progress when interpolated() is called. Also, the steps in the
    keyValues are referring to this effective progress.

    The easing curve is used with the interpolator, the interpolated()
    virtual function, the animation's duration, and iterationCount, to
    control how the current value changes as the animation progresses.
*/
QEasingCurve QVariantAnimation::easingCurve() const
{
    Q_D(const QVariantAnimation);
    return d->easing;
}

void QVariantAnimation::setEasingCurve(const QEasingCurve &easing)
{
    Q_D(QVariantAnimation);
    d->easing = easing;
    d->recalculateCurrentInterval();
}

typedef QVector<QVariantAnimation::Interpolator> QInterpolatorVector;
Q_GLOBAL_STATIC(QInterpolatorVector, registeredInterpolators)
static QBasicMutex registeredInterpolatorsMutex;

/*!
    \fn void qRegisterAnimationInterpolator(QVariant (*func)(const T &from, const T &to, qreal progress))
    \relates QVariantAnimation
    \threadsafe

    Registers a custom interpolator \a func for the template type \c{T}.
    The interpolator has to be registered before the animation is constructed.
    To unregister (and use the default interpolator) set \a func to 0.
 */

/*!
    \internal
    \typedef QVariantAnimation::Interpolator

    This is a typedef for a pointer to a function with the following
    signature:
    \code
    QVariant myInterpolator(const QVariant &from, const QVariant &to, qreal progress);
    \endcode

*/

/*!
 * \internal
 * Registers a custom interpolator \a func for the specific \a interpolationType.
 * The interpolator has to be registered before the animation is constructed.
 * To unregister (and use the default interpolator) set \a func to 0.
 */
void QVariantAnimation::registerInterpolator(QVariantAnimation::Interpolator func, int interpolationType)
{
    // will override any existing interpolators
    QInterpolatorVector *interpolators = registeredInterpolators();
    // When built on solaris with GCC, the destructors can be called
    // in such an order that we get here with interpolators == NULL,
    // to continue causes the app to crash on exit with a SEGV
    if (interpolators) {
        QMutexLocker locker(&registeredInterpolatorsMutex);
        if (int(interpolationType) >= interpolators->count())
            interpolators->resize(int(interpolationType) + 1);
        interpolators->replace(interpolationType, func);
    }
}


template<typename T> static inline QVariantAnimation::Interpolator castToInterpolator(QVariant (*func)(const T &from, const T &to, qreal progress))
{
     return reinterpret_cast<QVariantAnimation::Interpolator>(func);
}

QVariantAnimation::Interpolator QVariantAnimationPrivate::getInterpolator(int interpolationType)
{
    {
        QInterpolatorVector *interpolators = registeredInterpolators();
        QMutexLocker locker(&registeredInterpolatorsMutex);
        QVariantAnimation::Interpolator ret = 0;
        if (interpolationType < interpolators->count()) {
            ret = interpolators->at(interpolationType);
            if (ret) return ret;
        }
    }

    switch(interpolationType)
    {
    case QMetaType::Int:
        return castToInterpolator(_q_interpolateVariant<int>);
    case QMetaType::UInt:
        return castToInterpolator(_q_interpolateVariant<uint>);
    case QMetaType::Double:
        return castToInterpolator(_q_interpolateVariant<double>);
    case QMetaType::Float:
        return castToInterpolator(_q_interpolateVariant<float>);
    case QMetaType::QLine:
        return castToInterpolator(_q_interpolateVariant<QLine>);
    case QMetaType::QLineF:
        return castToInterpolator(_q_interpolateVariant<QLineF>);
    case QMetaType::QPoint:
        return castToInterpolator(_q_interpolateVariant<QPoint>);
    case QMetaType::QPointF:
        return castToInterpolator(_q_interpolateVariant<QPointF>);
    case QMetaType::QSize:
        return castToInterpolator(_q_interpolateVariant<QSize>);
    case QMetaType::QSizeF:
        return castToInterpolator(_q_interpolateVariant<QSizeF>);
    case QMetaType::QRect:
        return castToInterpolator(_q_interpolateVariant<QRect>);
    case QMetaType::QRectF:
        return castToInterpolator(_q_interpolateVariant<QRectF>);
    default:
        return 0; //this type is not handled
    }
}

/*!
    \property QVariantAnimation::duration
    \brief the duration of the animation

    This property describes the duration in milliseconds of the
    animation. The default duration is 250 milliseconds.

    \sa QAbstractAnimation::duration()
 */
int QVariantAnimation::duration() const
{
    Q_D(const QVariantAnimation);
    return d->duration;
}

void QVariantAnimation::setDuration(int msecs)
{
    Q_D(QVariantAnimation);
    if (msecs < 0) {
        qWarning("QVariantAnimation::setDuration: cannot set a negative duration");
        return;
    }
    if (d->duration == msecs)
        return;
    d->duration = msecs;
    d->recalculateCurrentInterval();
}

/*!
    \property QVariantAnimation::startValue
    \brief the optional start value of the animation

    This property describes the optional start value of the animation. If
    omitted, or if a null QVariant is assigned as the start value, the
    animation will use the current position of the end when the animation
    is started.

    \sa endValue
*/
QVariant QVariantAnimation::startValue() const
{
    return keyValueAt(0);
}

void QVariantAnimation::setStartValue(const QVariant &value)
{
    setKeyValueAt(0, value);
}

/*!
    \property QVariantAnimation::endValue
    \brief the end value of the animation

    This property describes the end value of the animation.

    \sa startValue
 */
QVariant QVariantAnimation::endValue() const
{
    return keyValueAt(1);
}

void QVariantAnimation::setEndValue(const QVariant &value)
{
    setKeyValueAt(1, value);
}


/*!
    Returns the key frame value for the given \a step. The given \a step
    must be in the range 0 to 1. If there is no KeyValue for \a step,
    it returns an invalid QVariant.

    \sa keyValues(), setKeyValueAt()
*/
QVariant QVariantAnimation::keyValueAt(qreal step) const
{
    return d_func()->valueAt(step);
}

/*!
    \typedef QVariantAnimation::KeyValue

    This is a typedef for QPair<qreal, QVariant>.
*/
/*!
    \typedef QVariantAnimation::KeyValues

    This is a typedef for QVector<KeyValue>
*/

/*!
    Creates a key frame at the given \a step with the given \a value.
    The given \a step must be in the range 0 to 1.

    \sa setKeyValues(), keyValueAt()
*/
void QVariantAnimation::setKeyValueAt(qreal step, const QVariant &value)
{
    d_func()->setValueAt(step, value);
}

/*!
    Returns the key frames of this animation.

    \sa keyValueAt(), setKeyValues()
*/
QVariantAnimation::KeyValues QVariantAnimation::keyValues() const
{
    return d_func()->keyValues;
}

/*!
    Replaces the current set of key frames with the given \a keyValues.
    the step of the key frames must be in the range 0 to 1.

    \sa keyValues(), keyValueAt()
*/
void QVariantAnimation::setKeyValues(const KeyValues &keyValues)
{
    Q_D(QVariantAnimation);
    d->keyValues = keyValues;
    std::sort(d->keyValues.begin(), d->keyValues.end(), animationValueLessThan);
    d->recalculateCurrentInterval(/*force=*/true);
}

/*!
    \property QVariantAnimation::currentValue
    \brief the current value of the animation.

    This property describes the current value; an interpolated value
    between the \l{startValue}{start value} and the \l{endValue}{end
    value}, using the current time for progress. The value itself is
    obtained from interpolated(), which is called repeatedly as the
    animation is running.

    QVariantAnimation calls the virtual updateCurrentValue() function
    when the current value changes. This is particularly useful for
    subclasses that need to track updates. For example,
    QPropertyAnimation uses this function to animate Qt \l{Qt's
    Property System}{properties}.

    \sa startValue, endValue
*/
QVariant QVariantAnimation::currentValue() const
{
    Q_D(const QVariantAnimation);
    if (!d->currentValue.isValid())
        const_cast<QVariantAnimationPrivate*>(d)->recalculateCurrentInterval();
    return d->currentValue;
}

/*!
    \reimp
 */
bool QVariantAnimation::event(QEvent *event)
{
    return QAbstractAnimation::event(event);
}

/*!
    \reimp
*/
void QVariantAnimation::updateState(QAbstractAnimation::State newState,
                                    QAbstractAnimation::State oldState)
{
    Q_UNUSED(oldState);
    Q_UNUSED(newState);
}

/*!

    This virtual function returns the linear interpolation between
    variants \a from and \a to, at \a progress, usually a value
    between 0 and 1. You can reimplement this function in a subclass
    of QVariantAnimation to provide your own interpolation algorithm.

    Note that in order for the interpolation to work with a
    QEasingCurve that return a value smaller than 0 or larger than 1
    (such as QEasingCurve::InBack) you should make sure that it can
    extrapolate. If the semantic of the datatype does not allow
    extrapolation this function should handle that gracefully.

    You should call the QVariantAnimation implementation of this
    function if you want your class to handle the types already
    supported by Qt (see class QVariantAnimation description for a
    list of supported types).

    \sa QEasingCurve
 */
QVariant QVariantAnimation::interpolated(const QVariant &from, const QVariant &to, qreal progress) const
{
    return d_func()->interpolator(from.constData(), to.constData(), progress);
}

/*!
    \reimp
 */
void QVariantAnimation::updateCurrentTime(int)
{
    d_func()->recalculateCurrentInterval();
}

QT_END_NAMESPACE

#include "moc_qvariantanimation.cpp"

#endif //QT_NO_ANIMATION
