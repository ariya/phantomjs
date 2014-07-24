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

#include "qtimeline.h"

#include <private/qobject_p.h>
#include <QtCore/qcoreevent.h>
#include <QtCore/qmath.h>
#include <QtCore/qelapsedtimer.h>

QT_BEGIN_NAMESPACE

class QTimeLinePrivate : public QObjectPrivate
{
    Q_DECLARE_PUBLIC(QTimeLine)
public:
    inline QTimeLinePrivate()
        : easingCurve(QEasingCurve::InOutSine),
          startTime(0), duration(1000), startFrame(0), endFrame(0),
          updateInterval(1000 / 25),
          totalLoopCount(1), currentLoopCount(0), currentTime(0), timerId(0),
          direction(QTimeLine::Forward),
          state(QTimeLine::NotRunning)
    { }

    QElapsedTimer timer;
    QEasingCurve easingCurve;

    int startTime;
    int duration;
    int startFrame;
    int endFrame;
    int updateInterval;
    int totalLoopCount;
    int currentLoopCount;

    int currentTime;
    int timerId;

    QTimeLine::Direction direction;
    QTimeLine::State state;
    inline void setState(QTimeLine::State newState)
    {
        Q_Q(QTimeLine);
        if (newState != state)
            emit q->stateChanged(state = newState, QTimeLine::QPrivateSignal());
    }

    void setCurrentTime(int msecs);
};

/*!
    \internal
*/
void QTimeLinePrivate::setCurrentTime(int msecs)
{
    Q_Q(QTimeLine);

    qreal lastValue = q->currentValue();
    int lastFrame = q->currentFrame();

    // Determine if we are looping.
    int elapsed = (direction == QTimeLine::Backward) ? (-msecs +  duration) : msecs;
    int loopCount = elapsed / duration;

    bool looping = (loopCount != currentLoopCount);
#ifdef QTIMELINE_DEBUG
    qDebug() << "QTimeLinePrivate::setCurrentTime:" << msecs << duration << "with loopCount" << loopCount
             << "currentLoopCount" << currentLoopCount
             << "looping" << looping;
#endif
    if (looping)
        currentLoopCount = loopCount;

    // Normalize msecs to be between 0 and duration, inclusive.
    currentTime = elapsed % duration;
    if (direction == QTimeLine::Backward)
        currentTime = duration - currentTime;

    // Check if we have reached the end of loopcount.
    bool finished = false;
    if (totalLoopCount && currentLoopCount >= totalLoopCount) {
        finished = true;
        currentTime = (direction == QTimeLine::Backward) ? 0 : duration;
        currentLoopCount = totalLoopCount - 1;
    }

    int currentFrame = q->frameForTime(currentTime);
#ifdef QTIMELINE_DEBUG
    qDebug() << "QTimeLinePrivate::setCurrentTime: frameForTime" << currentTime << currentFrame;
#endif
    if (!qFuzzyCompare(lastValue, q->currentValue()))
        emit q->valueChanged(q->currentValue(), QTimeLine::QPrivateSignal());
    if (lastFrame != currentFrame) {
        const int transitionframe = (direction == QTimeLine::Forward ? endFrame : startFrame);
        if (looping && !finished && transitionframe != currentFrame) {
#ifdef QTIMELINE_DEBUG
            qDebug() << "QTimeLinePrivate::setCurrentTime: transitionframe";
#endif
            emit q->frameChanged(transitionframe, QTimeLine::QPrivateSignal());
        }
#ifdef QTIMELINE_DEBUG
        else {
            QByteArray reason;
            if (!looping)
                reason += " not looping";
            if (finished) {
                if (!reason.isEmpty())
                    reason += " and";
                reason += " finished";
            }
            if (transitionframe == currentFrame) {
                if (!reason.isEmpty())
                    reason += " and";
                reason += " transitionframe is equal to currentFrame: " + QByteArray::number(currentFrame);
            }
            qDebug("QTimeLinePrivate::setCurrentTime: not transitionframe because %s",  reason.constData());
        }
#endif
        emit q->frameChanged(currentFrame, QTimeLine::QPrivateSignal());
    }
    if (finished && state == QTimeLine::Running) {
        q->stop();
        emit q->finished(QTimeLine::QPrivateSignal());
    }
}

/*!
    \class QTimeLine
    \inmodule QtCore
    \brief The QTimeLine class provides a timeline for controlling animations.
    \since 4.2
    \ingroup animation

    It's most commonly used to animate a GUI control by calling a slot
    periodically. You can construct a timeline by passing its duration in
    milliseconds to QTimeLine's constructor. The timeline's duration describes
    for how long the animation will run. Then you set a suitable frame range
    by calling setFrameRange(). Finally connect the frameChanged() signal to a
    suitable slot in the widget you wish to animate (e.g., setValue() in
    QProgressBar). When you proceed to calling start(), QTimeLine will enter
    Running state, and start emitting frameChanged() at regular intervals,
    causing your widget's connected property's value to grow from the lower
    end to the upper and of your frame range, at a steady rate. You can
    specify the update interval by calling setUpdateInterval(). When done,
    QTimeLine enters NotRunning state, and emits finished().

    Example:

    \snippet code/src_corelib_tools_qtimeline.cpp 0

    By default the timeline runs once, from the beginning and towards the end,
    upon which you must call start() again to restart from the beginning. To
    make the timeline loop, you can call setLoopCount(), passing the number of
    times the timeline should run before finishing. The direction can also be
    changed, causing the timeline to run backward, by calling
    setDirection(). You can also pause and unpause the timeline while it's
    running by calling setPaused(). For interactive control, the
    setCurrentTime() function is provided, which sets the time position of the
    time line directly. Although most useful in NotRunning state, (e.g.,
    connected to a valueChanged() signal in a QSlider,) this function can be
    called at any time.

    The frame interface is useful for standard widgets, but QTimeLine can be
    used to control any type of animation. The heart of QTimeLine lies in the
    valueForTime() function, which generates a \e value between 0 and 1 for a
    given time. This value is typically used to describe the steps of an
    animation, where 0 is the first step of an animation, and 1 is the last
    step. When running, QTimeLine generates values between 0 and 1 by calling
    valueForTime() and emitting valueChanged(). By default, valueForTime()
    applies an interpolation algorithm to generate these value. You can choose
    from a set of predefined timeline algorithms by calling
    setCurveShape().

    Note that by default, QTimeLine uses the EaseInOut curve shape,
    which provides a value that grows slowly, then grows steadily, and
    finally grows slowly. For a custom timeline, you can reimplement
    valueForTime(), in which case QTimeLine's curveShape property is ignored.

    \sa QProgressBar, QProgressDialog
*/

/*!
    \enum QTimeLine::State

    This enum describes the state of the timeline.

    \value NotRunning The timeline is not running. This is the initial state
    of QTimeLine, and the state QTimeLine reenters when finished. The current
    time, frame and value remain unchanged until either setCurrentTime() is
    called, or the timeline is started by calling start().

    \value Paused The timeline is paused (i.e., temporarily
    suspended). Calling setPaused(false) will resume timeline activity.

    \value Running The timeline is running. While control is in the event
    loop, QTimeLine will update its current time at regular intervals,
    emitting valueChanged() and frameChanged() when appropriate.

    \sa state(), stateChanged()
*/

/*!
    \enum QTimeLine::Direction

    This enum describes the direction of the timeline when in \l Running state.

    \value Forward The current time of the timeline increases with time (i.e.,
    moves from 0 and towards the end / duration).

    \value Backward The current time of the timeline decreases with time (i.e.,
    moves from the end / duration and towards 0).

    \sa setDirection()
*/

/*!
    \enum QTimeLine::CurveShape

    This enum describes the default shape of QTimeLine's value curve. The
    default, shape is EaseInOutCurve. The curve defines the relation
    between the value and the timeline.

    \value EaseInCurve The value starts growing slowly, then increases in speed.
    \value EaseOutCurve The value starts growing steadily, then ends slowly.
    \value EaseInOutCurve The value starts growing slowly, then runs steadily, then grows slowly again.
    \value LinearCurve The value grows linearly (e.g., if the duration is 1000 ms,
           the value at time 500 ms is 0.5).
    \value SineCurve The value grows sinusoidally.
    \value CosineCurve The value grows cosinusoidally.

    \sa setCurveShape()
*/

/*!
    \fn QTimeLine::valueChanged(qreal value)

    QTimeLine emits this signal at regular intervals when in \l Running state,
    but only if the current value changes. \a value is the current value. \a value is
    a number between 0.0 and 1.0

    \sa QTimeLine::setDuration(), QTimeLine::valueForTime(), QTimeLine::updateInterval
*/

/*!
    \fn QTimeLine::frameChanged(int frame)

    QTimeLine emits this signal at regular intervals when in \l Running state,
    but only if the current frame changes. \a frame is the current frame number.

    \sa QTimeLine::setFrameRange(), QTimeLine::updateInterval
*/

/*!
    \fn QTimeLine::stateChanged(QTimeLine::State newState)

    This signal is emitted whenever QTimeLine's state changes. The new state
    is \a newState.
*/

/*!
    \fn QTimeLine::finished()

    This signal is emitted when QTimeLine finishes (i.e., reaches the end of
    its time line), and does not loop.
*/

/*!
    Constructs a timeline with a duration of \a duration milliseconds. \a
    parent is passed to QObject's constructor. The default duration is 1000
    milliseconds.
 */
QTimeLine::QTimeLine(int duration, QObject *parent)
    : QObject(*new QTimeLinePrivate, parent)
{
    setDuration(duration);
}

/*!
    Destroys the timeline.
 */
QTimeLine::~QTimeLine()
{
    Q_D(QTimeLine);

    if (d->state == Running)
        stop();
}

/*!
    Returns the state of the timeline.

    \sa start(), setPaused(), stop()
*/
QTimeLine::State QTimeLine::state() const
{
    Q_D(const QTimeLine);
    return d->state;
}

/*!
    \property QTimeLine::loopCount
    \brief the number of times the timeline should loop before it's finished.

    A loop count of of 0 means that the timeline will loop forever.

    By default, this property contains a value of 1.
*/
int QTimeLine::loopCount() const
{
    Q_D(const QTimeLine);
    return d->totalLoopCount;
}
void QTimeLine::setLoopCount(int count)
{
    Q_D(QTimeLine);
    d->totalLoopCount = count;
}

/*!
    \property QTimeLine::direction
    \brief the direction of the timeline when QTimeLine is in \l Running
    state.

    This direction indicates whether the time moves from 0 towards the
    timeline duration, or from the value of the duration and towards 0 after
    start() has been called.

    By default, this property is set to \l Forward.
*/
QTimeLine::Direction QTimeLine::direction() const
{
    Q_D(const QTimeLine);
    return d->direction;
}
void QTimeLine::setDirection(Direction direction)
{
    Q_D(QTimeLine);
    d->direction = direction;
    d->startTime = d->currentTime;
    d->timer.start();
}

/*!
    \property QTimeLine::duration
    \brief the total duration of the timeline in milliseconds.

    By default, this value is 1000 (i.e., 1 second), but you can change this
    by either passing a duration to QTimeLine's constructor, or by calling
    setDuration(). The duration must be larger than 0.

    \note Changing the duration does not cause the current time to be reset
    to zero or the new duration. You also need to call setCurrentTime() with
    the desired value.
*/
int QTimeLine::duration() const
{
    Q_D(const QTimeLine);
    return d->duration;
}
void QTimeLine::setDuration(int duration)
{
    Q_D(QTimeLine);
    if (duration <= 0) {
        qWarning("QTimeLine::setDuration: cannot set duration <= 0");
        return;
    }
    d->duration = duration;
}

/*!
    Returns the start frame, which is the frame corresponding to the start of
    the timeline (i.e., the frame for which the current value is 0).

    \sa setStartFrame(), setFrameRange()
*/
int QTimeLine::startFrame() const
{
    Q_D(const QTimeLine);
    return d->startFrame;
}

/*!
    Sets the start frame, which is the frame corresponding to the start of the
    timeline (i.e., the frame for which the current value is 0), to \a frame.

    \sa startFrame(), endFrame(), setFrameRange()
*/
void QTimeLine::setStartFrame(int frame)
{
    Q_D(QTimeLine);
    d->startFrame = frame;
}

/*!
    Returns the end frame, which is the frame corresponding to the end of the
    timeline (i.e., the frame for which the current value is 1).

    \sa setEndFrame(), setFrameRange()
*/
int QTimeLine::endFrame() const
{
    Q_D(const QTimeLine);
    return d->endFrame;
}

/*!
    Sets the end frame, which is the frame corresponding to the end of the
    timeline (i.e., the frame for which the current value is 1), to \a frame.

    \sa endFrame(), startFrame(), setFrameRange()
*/
void QTimeLine::setEndFrame(int frame)
{
    Q_D(QTimeLine);
    d->endFrame = frame;
}

/*!
    Sets the timeline's frame counter to start at \a startFrame, and end and
    \a endFrame. For each time value, QTimeLine will find the corresponding
    frame when you call currentFrame() or frameForTime() by interpolating,
    using the return value of valueForTime().

    When in Running state, QTimeLine also emits the frameChanged() signal when
    the frame changes.

    \sa startFrame(), endFrame(), start(), currentFrame()
*/
void QTimeLine::setFrameRange(int startFrame, int endFrame)
{
    Q_D(QTimeLine);
    d->startFrame = startFrame;
    d->endFrame = endFrame;
}

/*!
    \property QTimeLine::updateInterval
    \brief the time in milliseconds between each time QTimeLine updates its
    current time.

    When updating the current time, QTimeLine will emit valueChanged() if the
    current value changed, and frameChanged() if the frame changed.

    By default, the interval is 40 ms, which corresponds to a rate of 25
    updates per second.
*/
int QTimeLine::updateInterval() const
{
    Q_D(const QTimeLine);
    return d->updateInterval;
}
void QTimeLine::setUpdateInterval(int interval)
{
    Q_D(QTimeLine);
    d->updateInterval = interval;
}

/*!
    \property QTimeLine::curveShape
    \brief the shape of the timeline curve.

    The curve shape describes the relation between the time and value for the
    base implementation of valueForTime().

    If you have reimplemented valueForTime(), this value is ignored.

    By default, this property is set to \l EaseInOutCurve.

    \sa valueForTime()
*/
QTimeLine::CurveShape QTimeLine::curveShape() const
{
    Q_D(const QTimeLine);
    switch (d->easingCurve.type()) {
    default:
    case QEasingCurve::InOutSine:
        return EaseInOutCurve;
    case QEasingCurve::InCurve:
        return EaseInCurve;
    case QEasingCurve::OutCurve:
        return EaseOutCurve;
    case QEasingCurve::Linear:
        return LinearCurve;
    case QEasingCurve::SineCurve:
        return SineCurve;
    case QEasingCurve::CosineCurve:
        return CosineCurve;
    }
    return EaseInOutCurve;
}

void QTimeLine::setCurveShape(CurveShape shape)
{
    switch (shape) {
    default:
    case EaseInOutCurve:
        setEasingCurve(QEasingCurve(QEasingCurve::InOutSine));
        break;
    case EaseInCurve:
        setEasingCurve(QEasingCurve(QEasingCurve::InCurve));
        break;
    case EaseOutCurve:
        setEasingCurve(QEasingCurve(QEasingCurve::OutCurve));
        break;
    case LinearCurve:
        setEasingCurve(QEasingCurve(QEasingCurve::Linear));
        break;
    case SineCurve:
        setEasingCurve(QEasingCurve(QEasingCurve::SineCurve));
        break;
    case CosineCurve:
        setEasingCurve(QEasingCurve(QEasingCurve::CosineCurve));
        break;
    }
}

/*!
    \property QTimeLine::easingCurve

    \since 4.6

    Specifies the easing curve that the timeline will use.
    If both easing curve and curveShape are set, the last set property will
    override the previous one. (If valueForTime() is reimplemented it will
    override both)
*/

QEasingCurve QTimeLine::easingCurve() const
{
    Q_D(const QTimeLine);
    return d->easingCurve;
}

void QTimeLine::setEasingCurve(const QEasingCurve& curve)
{
    Q_D(QTimeLine);
    d->easingCurve = curve;
}

/*!
    \property QTimeLine::currentTime
    \brief the current time of the time line.

    When QTimeLine is in Running state, this value is updated continuously as
    a function of the duration and direction of the timeline. Otherwise, it is
    value that was current when stop() was called last, or the value set by
    setCurrentTime().

    By default, this property contains a value of 0.
*/
int QTimeLine::currentTime() const
{
    Q_D(const QTimeLine);
    return d->currentTime;
}
void QTimeLine::setCurrentTime(int msec)
{
    Q_D(QTimeLine);
    d->startTime = 0;
    d->currentLoopCount = 0;
    d->timer.restart();
    d->setCurrentTime(msec);
}

/*!
    Returns the frame corresponding to the current time.

    \sa currentTime(), frameForTime(), setFrameRange()
*/
int QTimeLine::currentFrame() const
{
    Q_D(const QTimeLine);
    return frameForTime(d->currentTime);
}

/*!
    Returns the value corresponding to the current time.

    \sa valueForTime(), currentFrame()
*/
qreal QTimeLine::currentValue() const
{
    Q_D(const QTimeLine);
    return valueForTime(d->currentTime);
}

/*!
    Returns the frame corresponding to the time \a msec. This value is
    calculated using a linear interpolation of the start and end frame, based
    on the value returned by valueForTime().

    \sa valueForTime(), setFrameRange()
*/
int QTimeLine::frameForTime(int msec) const
{
    Q_D(const QTimeLine);
    if (d->direction == Forward)
        return d->startFrame + int((d->endFrame - d->startFrame) * valueForTime(msec));
    return d->startFrame + qCeil((d->endFrame - d->startFrame) * valueForTime(msec));
}

/*!
    Returns the timeline value for the time \a msec. The returned value, which
    varies depending on the curve shape, is always between 0 and 1. If \a msec
    is 0, the default implementation always returns 0.

    Reimplement this function to provide a custom curve shape for your
    timeline.

    \sa CurveShape, frameForTime()
*/
qreal QTimeLine::valueForTime(int msec) const
{
    Q_D(const QTimeLine);
    msec = qMin(qMax(msec, 0), d->duration);

    qreal value = msec / qreal(d->duration);
    return d->easingCurve.valueForProgress(value);
}

/*!
    Starts the timeline. QTimeLine will enter Running state, and once it
    enters the event loop, it will update its current time, frame and value at
    regular intervals. The default interval is 40 ms (i.e., 25 times per
    second). You can change the update interval by calling
    setUpdateInterval().

    The timeline will start from position 0, or the end if going backward.
    If you want to resume a stopped timeline without restarting, you can call
    resume() instead.

    \sa resume(), updateInterval(), frameChanged(), valueChanged()
*/
void QTimeLine::start()
{
    Q_D(QTimeLine);
    if (d->timerId) {
        qWarning("QTimeLine::start: already running");
        return;
    }
    int curTime = 0;
    if (d->direction == Backward)
        curTime = d->duration;
    d->timerId = startTimer(d->updateInterval);
    d->startTime = curTime;
    d->currentLoopCount = 0;
    d->timer.start();
    d->setState(Running);
    d->setCurrentTime(curTime);
}

/*!
    Resumes the timeline from the current time. QTimeLine will reenter Running
    state, and once it enters the event loop, it will update its current time,
    frame and value at regular intervals.

    In contrast to start(), this function does not restart the timeline before
    it resumes.

    \sa start(), updateInterval(), frameChanged(), valueChanged()
*/
void QTimeLine::resume()
{
    Q_D(QTimeLine);
    if (d->timerId) {
        qWarning("QTimeLine::resume: already running");
        return;
    }
    d->timerId = startTimer(d->updateInterval);
    d->startTime = d->currentTime;
    d->timer.start();
    d->setState(Running);
}

/*!
    Stops the timeline, causing QTimeLine to enter NotRunning state.

    \sa start()
*/
void QTimeLine::stop()
{
    Q_D(QTimeLine);
    if (d->timerId)
        killTimer(d->timerId);
    d->setState(NotRunning);
    d->timerId = 0;
}

/*!
    If \a paused is true, the timeline is paused, causing QTimeLine to enter
    Paused state. No updates will be signaled until either start() or
    setPaused(false) is called. If \a paused is false, the timeline is resumed
    and continues where it left.

    \sa state(), start()
*/
void QTimeLine::setPaused(bool paused)
{
    Q_D(QTimeLine);
    if (d->state == NotRunning) {
        qWarning("QTimeLine::setPaused: Not running");
        return;
    }
    if (paused && d->state != Paused) {
        d->startTime = d->currentTime;
        killTimer(d->timerId);
        d->timerId = 0;
        d->setState(Paused);
    } else if (!paused && d->state == Paused) {
        // Same as resume()
        d->timerId = startTimer(d->updateInterval);
        d->startTime = d->currentTime;
        d->timer.start();
        d->setState(Running);
    }
}

/*!
    Toggles the direction of the timeline. If the direction was Forward, it
    becomes Backward, and vice verca.

    \sa setDirection()
*/
void QTimeLine::toggleDirection()
{
    Q_D(QTimeLine);
    setDirection(d->direction == Forward ? Backward : Forward);
}

/*!
    \reimp
*/
void QTimeLine::timerEvent(QTimerEvent *event)
{
    Q_D(QTimeLine);
    if (event->timerId() != d->timerId) {
        event->ignore();
        return;
    }
    event->accept();

    if (d->direction == Forward) {
        d->setCurrentTime(d->startTime + d->timer.elapsed());
    } else {
        d->setCurrentTime(d->startTime - d->timer.elapsed());
    }
}

QT_END_NAMESPACE
