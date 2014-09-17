/****************************************************************************
**
** Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
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

/*!
    \class QAbstractAnimation
    \ingroup animation
    \brief The QAbstractAnimation class is the base of all animations.
    \since 4.6

    The class defines the functions for the functionality shared by
    all animations. By inheriting this class, you can create custom
    animations that plug into the rest of the animation framework.

    The progress of an animation is given by its current time
    (currentLoopTime()), which is measured in milliseconds from the start
    of the animation (0) to its end (duration()). The value is updated
    automatically while the animation is running. It can also be set
    directly with setCurrentTime().

    At any point an animation is in one of three states:
    \l{QAbstractAnimation::}{Running},
    \l{QAbstractAnimation::}{Stopped}, or
    \l{QAbstractAnimation::}{Paused}--as defined by the
    \l{QAbstractAnimation::}{State} enum. The current state can be
    changed by calling start(), stop(), pause(), or resume(). An
    animation will always reset its \l{currentTime()}{current time}
    when it is started. If paused, it will continue with the same
    current time when resumed. When an animation is stopped, it cannot
    be resumed, but will keep its current time (until started again).
    QAbstractAnimation will emit stateChanged() whenever its state
    changes.

    An animation can loop any number of times by setting the loopCount
    property. When an animation's current time reaches its duration(),
    it will reset the current time and keep running. A loop count of 1
    (the default value) means that the animation will run one time.
    Note that a duration of -1 means that the animation will run until
    stopped; the current time will increase indefinitely. When the
    current time equals duration() and the animation is in its
    final loop, the \l{QAbstractAnimation::}{Stopped} state is
    entered, and the finished() signal is emitted.

    QAbstractAnimation provides pure virtual functions used by
    subclasses to track the progress of the animation: duration() and
    updateCurrentTime(). The duration() function lets you report a
    duration for the animation (as discussed above). The animation
    framework calls updateCurrentTime() when current time has changed.
    By reimplementing this function, you can track the animation
    progress. Note that neither the interval between calls nor the
    number of calls to this function are defined; though, it will
    normally be 60 updates per second.

    By reimplementing updateState(), you can track the animation's
    state changes, which is particularly useful for animations that
    are not driven by time.

    \sa QVariantAnimation, QPropertyAnimation, QAnimationGroup, {The Animation Framework}
*/

/*!
    \enum QAbstractAnimation::DeletionPolicy

    \value KeepWhenStopped The animation will not be deleted when stopped.
    \value DeleteWhenStopped The animation will be automatically deleted when
    stopped.
*/

/*!
    \fn QAbstractAnimation::finished()

    QAbstractAnimation emits this signal after the animation has stopped and
    has reached the end.

    This signal is emitted after stateChanged().

    \sa stateChanged()
*/

/*!
    \fn QAbstractAnimation::stateChanged(QAbstractAnimation::State newState, QAbstractAnimation::State oldState)

    QAbstractAnimation emits this signal whenever the state of the animation has
    changed from \a oldState to \a newState. This signal is emitted after the virtual
    updateState() function is called.

    \sa updateState()
*/

/*!
    \fn QAbstractAnimation::currentLoopChanged(int currentLoop)

    QAbstractAnimation emits this signal whenever the current loop
    changes. \a currentLoop is the current loop.

    \sa currentLoop(), loopCount()
*/

/*!
    \fn QAbstractAnimation::directionChanged(QAbstractAnimation::Direction newDirection);

    QAbstractAnimation emits this signal whenever the direction has been
    changed. \a newDirection is the new direction.

    \sa direction
*/

#include "qabstractanimation.h"
#include "qanimationgroup.h"

#include <QtCore/qdebug.h>

#include "qabstractanimation_p.h"

#include <QtCore/qmath.h>
#include <QtCore/qthreadstorage.h>
#include <QtCore/qcoreevent.h>
#include <QtCore/qpointer.h>

#ifndef QT_NO_ANIMATION

#define DEFAULT_TIMER_INTERVAL 16
#define STARTSTOP_TIMER_DELAY 0

QT_BEGIN_NAMESPACE

#ifndef QT_NO_THREAD
Q_GLOBAL_STATIC(QThreadStorage<QUnifiedTimer *>, unifiedTimer)
#endif

QUnifiedTimer::QUnifiedTimer() :
    QObject(), defaultDriver(this), lastTick(0), timingInterval(DEFAULT_TIMER_INTERVAL),
    currentAnimationIdx(0), insideTick(false), consistentTiming(false), slowMode(false),
    slowdownFactor(5.0f), isPauseTimerActive(false), runningLeafAnimations(0)
{
    time.invalidate();
    driver = &defaultDriver;
}


QUnifiedTimer *QUnifiedTimer::instance(bool create)
{
    QUnifiedTimer *inst;
#ifndef QT_NO_THREAD
    if (create && !unifiedTimer()->hasLocalData()) {
        inst = new QUnifiedTimer;
        unifiedTimer()->setLocalData(inst);
    } else {
        inst = unifiedTimer()->localData();
    }
#else
    static QUnifiedTimer unifiedTimer;
    inst = &unifiedTimer;
#endif
    return inst;
}

QUnifiedTimer *QUnifiedTimer::instance()
{
    return instance(true);
}

void QUnifiedTimer::ensureTimerUpdate()
{
    QUnifiedTimer *inst = QUnifiedTimer::instance(false);
    if (inst && inst->isPauseTimerActive)
        inst->updateAnimationsTime();
}

void QUnifiedTimer::updateAnimationsTime()
{
    //setCurrentTime can get this called again while we're the for loop. At least with pauseAnimations
    if(insideTick)
        return;

    qint64 totalElapsed = time.elapsed();
    // ignore consistentTiming in case the pause timer is active
    int delta = (consistentTiming && !isPauseTimerActive) ?
                        timingInterval : totalElapsed - lastTick;
    if (slowMode) {
        if (slowdownFactor > 0)
            delta = qRound(delta / slowdownFactor);
        else
            delta = 0;
    }

    lastTick = totalElapsed;

    //we make sure we only call update time if the time has actually changed
    //it might happen in some cases that the time doesn't change because events are delayed
    //when the CPU load is high
    if (delta) {
        insideTick = true;
        for (currentAnimationIdx = 0; currentAnimationIdx < animations.count(); ++currentAnimationIdx) {
            QAbstractAnimation *animation = animations.at(currentAnimationIdx);
            int elapsed = QAbstractAnimationPrivate::get(animation)->totalCurrentTime
                          + (animation->direction() == QAbstractAnimation::Forward ? delta : -delta);
            animation->setCurrentTime(elapsed);
        }
        insideTick = false;
        currentAnimationIdx = 0;
    }
}

void QUnifiedTimer::updateAnimationTimer()
{
    QUnifiedTimer *inst = QUnifiedTimer::instance(false);
    if (inst)
        inst->restartAnimationTimer();
}

void QUnifiedTimer::restartAnimationTimer()
{
    if (runningLeafAnimations == 0 && !runningPauseAnimations.isEmpty()) {
        int closestTimeToFinish = closestPauseAnimationTimeToFinish();
        if (closestTimeToFinish < 0) {
            qDebug() << runningPauseAnimations;
            qDebug() << closestPauseAnimationTimeToFinish();
        }
        driver->stop();
        animationTimer.start(closestTimeToFinish, this);
        isPauseTimerActive = true;
    } else if (!driver->isRunning() || isPauseTimerActive) {
        driver->start();
        isPauseTimerActive = false;
    } else if (runningLeafAnimations == 0)
        driver->stop();
}

void QUnifiedTimer::setTimingInterval(int interval)
{
    timingInterval = interval;

    if (driver->isRunning() && !isPauseTimerActive) {
        //we changed the timing interval
        driver->stop();
        driver->start();
    }
}


void QUnifiedTimer::timerEvent(QTimerEvent *event)
{
    //in the case of consistent timing we make sure the orders in which events come is always the same
   //for that purpose we do as if the startstoptimer would always fire before the animation timer
    if ((consistentTiming && startStopAnimationTimer.isActive()) ||
        event->timerId() == startStopAnimationTimer.timerId()) {
        startStopAnimationTimer.stop();

        //we transfer the waiting animations into the "really running" state
        animations += animationsToStart;
        animationsToStart.clear();
        if (animations.isEmpty()) {
            animationTimer.stop();
            isPauseTimerActive = false;
            // invalidate the start reference time
            time.invalidate();
        } else {
            restartAnimationTimer();
            if (!time.isValid()) {
                lastTick = 0;
                time.start();
            }
        }
    }

    if (event->timerId() == animationTimer.timerId()) {
        // update current time on all top level animations
        updateAnimationsTime();
        restartAnimationTimer();
    }
}

void QUnifiedTimer::registerAnimation(QAbstractAnimation *animation, bool isTopLevel)
{
    QUnifiedTimer *inst = instance(true); //we create the instance if needed
    inst->registerRunningAnimation(animation);
    if (isTopLevel) {
        Q_ASSERT(!QAbstractAnimationPrivate::get(animation)->hasRegisteredTimer);
        QAbstractAnimationPrivate::get(animation)->hasRegisteredTimer = true;
        inst->animationsToStart << animation;
        if (!inst->startStopAnimationTimer.isActive())
            inst->startStopAnimationTimer.start(STARTSTOP_TIMER_DELAY, inst);
    }
}

void QUnifiedTimer::unregisterAnimation(QAbstractAnimation *animation)
{
    QUnifiedTimer *inst = QUnifiedTimer::instance(false);
    if (inst) {
        //at this point the unified timer should have been created
        //but it might also have been already destroyed in case the application is shutting down

        inst->unregisterRunningAnimation(animation);

        if (!QAbstractAnimationPrivate::get(animation)->hasRegisteredTimer)
            return;

        int idx = inst->animations.indexOf(animation);
        if (idx != -1) {
            inst->animations.removeAt(idx);
            // this is needed if we unregister an animation while its running
            if (idx <= inst->currentAnimationIdx)
                --inst->currentAnimationIdx;

            if (inst->animations.isEmpty() && !inst->startStopAnimationTimer.isActive())
                inst->startStopAnimationTimer.start(STARTSTOP_TIMER_DELAY, inst);
        } else {
            inst->animationsToStart.removeOne(animation);
        }
    }
    QAbstractAnimationPrivate::get(animation)->hasRegisteredTimer = false;
}

void QUnifiedTimer::registerRunningAnimation(QAbstractAnimation *animation)
{
    if (QAbstractAnimationPrivate::get(animation)->isGroup)
        return;

    if (QAbstractAnimationPrivate::get(animation)->isPause) {
        runningPauseAnimations << animation;
    } else
        runningLeafAnimations++;
}

void QUnifiedTimer::unregisterRunningAnimation(QAbstractAnimation *animation)
{
    if (QAbstractAnimationPrivate::get(animation)->isGroup)
        return;

    if (QAbstractAnimationPrivate::get(animation)->isPause)
        runningPauseAnimations.removeOne(animation);
    else
        runningLeafAnimations--;
    Q_ASSERT(runningLeafAnimations >= 0);
}

int QUnifiedTimer::closestPauseAnimationTimeToFinish()
{
    int closestTimeToFinish = INT_MAX;
    for (int i = 0; i < runningPauseAnimations.size(); ++i) {
        QAbstractAnimation *animation = runningPauseAnimations.at(i);
        int timeToFinish;

        if (animation->direction() == QAbstractAnimation::Forward)
            timeToFinish = animation->duration() - animation->currentLoopTime();
        else
            timeToFinish = animation->currentLoopTime();

        if (timeToFinish < closestTimeToFinish)
            closestTimeToFinish = timeToFinish;
    }
    return closestTimeToFinish;
}

void QUnifiedTimer::installAnimationDriver(QAnimationDriver *d)
{
    if (driver) {

        if (driver->isRunning()) {
            qWarning("QUnifiedTimer: Cannot change animation driver while animations are running");
            return;
        }

        if (driver != &defaultDriver)
            delete driver;
    }

    driver = d;
}

/*!
   \class QAnimationDriver

   \brief The QAnimationDriver class is used to exchange the mechanism that drives animations.

   The default animation system is driven by a timer that fires at regular intervals.
   In some scenarios, it is better to drive the animation based on other synchronization
   mechanisms, such as the vertical refresh rate of the screen.

   \internal
 */

QAnimationDriver::QAnimationDriver(QObject *parent)
    : QObject(*(new QAnimationDriverPrivate), parent)
{
}

QAnimationDriver::QAnimationDriver(QAnimationDriverPrivate &dd, QObject *parent)
    : QObject(dd, parent)
{
}


/*!
    Advances the animation based on the current time. This function should
    be continuously called by the driver while the animation is running.

    \internal
 */
void QAnimationDriver::advance()
{
    QUnifiedTimer *instance = QUnifiedTimer::instance();

    // update current time on all top level animations
    instance->updateAnimationsTime();
    instance->restartAnimationTimer();
}


/*!
    Installs this animation driver. The animation driver is thread local and
    will only apply for the thread its installed in.

    \internal
 */
void QAnimationDriver::install()
{
    QUnifiedTimer *timer = QUnifiedTimer::instance(true);
    timer->installAnimationDriver(this);
}

bool QAnimationDriver::isRunning() const
{
    return d_func()->running;
}


void QAnimationDriver::start()
{
    Q_D(QAnimationDriver);
    if (!d->running) {
        started();
        d->running = true;
    }
}


void QAnimationDriver::stop()
{
    Q_D(QAnimationDriver);
    if (d->running) {
        stopped();
        d->running = false;
    }
}

/*!
    \fn QAnimationDriver::started()

    This function is called by the animation framework to notify the driver
    that it should start running.

    \internal
 */

/*!
    \fn QAnimationDriver::stopped()

    This function is called by the animation framework to notify the driver
    that it should stop running.

    \internal
 */

/*!
   The default animation driver just spins the timer...
 */
QDefaultAnimationDriver::QDefaultAnimationDriver(QUnifiedTimer *timer)
    : QAnimationDriver(0), m_unified_timer(timer)
{
}

void QDefaultAnimationDriver::timerEvent(QTimerEvent *e)
{
    Q_ASSERT(e->timerId() == m_timer.timerId());
    Q_UNUSED(e); // if the assertions are disabled
    advance();
}

void QDefaultAnimationDriver::started()
{
    m_timer.start(m_unified_timer->timingInterval, this);
}

void QDefaultAnimationDriver::stopped()
{
    m_timer.stop();
}



void QAbstractAnimationPrivate::setState(QAbstractAnimation::State newState)
{
    Q_Q(QAbstractAnimation);
    if (state == newState)
        return;

    if (loopCount == 0)
        return;

    QAbstractAnimation::State oldState = state;
    int oldCurrentTime = currentTime;
    int oldCurrentLoop = currentLoop;
    QAbstractAnimation::Direction oldDirection = direction;

    // check if we should Rewind
    if ((newState == QAbstractAnimation::Paused || newState == QAbstractAnimation::Running)
        && oldState == QAbstractAnimation::Stopped) {
            //here we reset the time if needed
            //we don't call setCurrentTime because this might change the way the animation
            //behaves: changing the state or changing the current value
            totalCurrentTime = currentTime = (direction == QAbstractAnimation::Forward) ?
                0 : (loopCount == -1 ? q->duration() : q->totalDuration());
    }

    state = newState;
    QWeakPointer<QAbstractAnimation> guard(q);

    //(un)registration of the animation must always happen before calls to
    //virtual function (updateState) to ensure a correct state of the timer
    bool isTopLevel = !group || group->state() == QAbstractAnimation::Stopped;
    if (oldState == QAbstractAnimation::Running) {
        if (newState == QAbstractAnimation::Paused && hasRegisteredTimer)
            QUnifiedTimer::ensureTimerUpdate();
        //the animation, is not running any more
        QUnifiedTimer::unregisterAnimation(q);
    } else if (newState == QAbstractAnimation::Running) {
        QUnifiedTimer::registerAnimation(q, isTopLevel);
    }

    q->updateState(newState, oldState);
    if (!guard || newState != state) //this is to be safe if updateState changes the state
        return;

    // Notify state change
    emit q->stateChanged(newState, oldState);
    if (!guard || newState != state) //this is to be safe if updateState changes the state
        return;

    switch (state) {
    case QAbstractAnimation::Paused:
        break;
    case QAbstractAnimation::Running:
        {

            // this ensures that the value is updated now that the animation is running
            if (oldState == QAbstractAnimation::Stopped) {
                if (isTopLevel) {
                    // currentTime needs to be updated if pauseTimer is active
                    QUnifiedTimer::ensureTimerUpdate();
                    q->setCurrentTime(totalCurrentTime);
                }
            }
        }
        break;
    case QAbstractAnimation::Stopped:
        // Leave running state.
        int dura = q->duration();

        if (deleteWhenStopped)
            q->deleteLater();

        if (dura == -1 || loopCount < 0
            || (oldDirection == QAbstractAnimation::Forward && (oldCurrentTime * (oldCurrentLoop + 1)) == (dura * loopCount))
            || (oldDirection == QAbstractAnimation::Backward && oldCurrentTime == 0)) {
                emit q->finished();
        }
        break;
    }
}

/*!
    Constructs the QAbstractAnimation base class, and passes \a parent to
    QObject's constructor.

    \sa QVariantAnimation, QAnimationGroup
*/
QAbstractAnimation::QAbstractAnimation(QObject *parent)
    : QObject(*new QAbstractAnimationPrivate, 0)
{
    // Allow auto-add on reparent
    setParent(parent);
}

/*!
    \internal
*/
QAbstractAnimation::QAbstractAnimation(QAbstractAnimationPrivate &dd, QObject *parent)
    : QObject(dd, 0)
{
    // Allow auto-add on reparent
   setParent(parent);
}

/*!
    Stops the animation if it's running, then destroys the
    QAbstractAnimation. If the animation is part of a QAnimationGroup, it is
    automatically removed before it's destroyed.
*/
QAbstractAnimation::~QAbstractAnimation()
{
    Q_D(QAbstractAnimation);
    //we can't call stop here. Otherwise we get pure virtual calls
    if (d->state != Stopped) {
        QAbstractAnimation::State oldState = d->state;
        d->state = Stopped;
        emit stateChanged(oldState, d->state);
        if (oldState == QAbstractAnimation::Running)
            QUnifiedTimer::unregisterAnimation(this);
    }
}

/*!
    \property QAbstractAnimation::state
    \brief state of the animation.

    This property describes the current state of the animation. When the
    animation state changes, QAbstractAnimation emits the stateChanged()
    signal.
*/
QAbstractAnimation::State QAbstractAnimation::state() const
{
    Q_D(const QAbstractAnimation);
    return d->state;
}

/*!
    If this animation is part of a QAnimationGroup, this function returns a
    pointer to the group; otherwise, it returns 0.

    \sa QAnimationGroup::addAnimation()
*/
QAnimationGroup *QAbstractAnimation::group() const
{
    Q_D(const QAbstractAnimation);
    return d->group;
}

/*!
    \enum QAbstractAnimation::State

    This enum describes the state of the animation.

    \value Stopped The animation is not running. This is the initial state
    of QAbstractAnimation, and the state QAbstractAnimation reenters when finished. The current
    time remain unchanged until either setCurrentTime() is
    called, or the animation is started by calling start().

    \value Paused The animation is paused (i.e., temporarily
    suspended). Calling resume() will resume animation activity.

    \value Running The animation is running. While control is in the event
    loop, QAbstractAnimation will update its current time at regular intervals,
    calling updateCurrentTime() when appropriate.

    \sa state(), stateChanged()
*/

/*!
    \enum QAbstractAnimation::Direction

    This enum describes the direction of the animation when in \l Running state.

    \value Forward The current time of the animation increases with time (i.e.,
    moves from 0 and towards the end / duration).

    \value Backward The current time of the animation decreases with time (i.e.,
    moves from the end / duration and towards 0).

    \sa direction
*/

/*!
    \property QAbstractAnimation::direction
    \brief the direction of the animation when it is in \l Running
    state.

    This direction indicates whether the time moves from 0 towards the
    animation duration, or from the value of the duration and towards 0 after
    start() has been called.

    By default, this property is set to \l Forward.
*/
QAbstractAnimation::Direction QAbstractAnimation::direction() const
{
    Q_D(const QAbstractAnimation);
    return d->direction;
}
void QAbstractAnimation::setDirection(Direction direction)
{
    Q_D(QAbstractAnimation);
    if (d->direction == direction)
        return;

    if (state() == Stopped) {
        if (direction == Backward) {
            d->currentTime = duration();
            d->currentLoop = d->loopCount - 1;
        } else {
            d->currentTime = 0;
            d->currentLoop = 0;
        }
    }

    // the commands order below is important: first we need to setCurrentTime with the old direction,
    // then update the direction on this and all children and finally restart the pauseTimer if needed
    if (d->hasRegisteredTimer)
        QUnifiedTimer::ensureTimerUpdate();

    d->direction = direction;
    updateDirection(direction);

    if (d->hasRegisteredTimer)
        // needed to update the timer interval in case of a pause animation
        QUnifiedTimer::updateAnimationTimer();

    emit directionChanged(direction);
}

/*!
    \property QAbstractAnimation::duration
    \brief the duration of the animation.

    If the duration is -1, it means that the duration is undefined.
    In this case, loopCount is ignored.
*/

/*!
    \property QAbstractAnimation::loopCount
    \brief the loop count of the animation

    This property describes the loop count of the animation as an integer.
    By default this value is 1, indicating that the animation
    should run once only, and then stop. By changing it you can let the
    animation loop several times. With a value of 0, the animation will not
    run at all, and with a value of -1, the animation will loop forever
    until stopped.
    It is not supported to have loop on an animation that has an undefined
    duration. It will only run once.
*/
int QAbstractAnimation::loopCount() const
{
    Q_D(const QAbstractAnimation);
    return d->loopCount;
}
void QAbstractAnimation::setLoopCount(int loopCount)
{
    Q_D(QAbstractAnimation);
    d->loopCount = loopCount;
}

/*!
    \property QAbstractAnimation::currentLoop
    \brief the current loop of the animation

    This property describes the current loop of the animation. By default,
    the animation's loop count is 1, and so the current loop will
    always be 0. If the loop count is 2 and the animation runs past its
    duration, it will automatically rewind and restart at current time 0, and
    current loop 1, and so on.

    When the current loop changes, QAbstractAnimation emits the
    currentLoopChanged() signal.
*/
int QAbstractAnimation::currentLoop() const
{
    Q_D(const QAbstractAnimation);
    return d->currentLoop;
}

/*!
    \fn virtual int QAbstractAnimation::duration() const = 0

    This pure virtual function returns the duration of the animation, and
    defines for how long QAbstractAnimation should update the current
    time. This duration is local, and does not include the loop count.

    A return value of -1 indicates that the animation has no defined duration;
    the animation should run forever until stopped. This is useful for
    animations that are not time driven, or where you cannot easily predict
    its duration (e.g., event driven audio playback in a game).

    If the animation is a parallel QAnimationGroup, the duration will be the longest
    duration of all its animations. If the animation is a sequential QAnimationGroup,
    the duration will be the sum of the duration of all its animations.
    \sa loopCount
*/

/*!
    Returns the total and effective duration of the animation, including the
    loop count.

    \sa duration(), currentTime
*/
int QAbstractAnimation::totalDuration() const
{
    int dura = duration();
    if (dura <= 0)
        return dura;
    int loopcount = loopCount();
    if (loopcount < 0)
        return -1;
    return dura * loopcount;
}

/*!
    Returns the current time inside the current loop. It can go from 0 to duration().

    \sa duration(), currentTime
*/

int QAbstractAnimation::currentLoopTime() const
{
    Q_D(const QAbstractAnimation);
    return d->currentTime;
}

/*!
    \property QAbstractAnimation::currentTime
    \brief the current time and progress of the animation

    This property describes the animation's current time. You can change the
    current time by calling setCurrentTime, or you can call start() and let
    the animation run, setting the current time automatically as the animation
    progresses.

    The animation's current time starts at 0, and ends at totalDuration().

    \sa loopCount, currentLoopTime()
 */
int QAbstractAnimation::currentTime() const
{
    Q_D(const QAbstractAnimation);
    return d->totalCurrentTime;
}
void QAbstractAnimation::setCurrentTime(int msecs)
{
    Q_D(QAbstractAnimation);
    msecs = qMax(msecs, 0);

    // Calculate new time and loop.
    int dura = duration();
    int totalDura = dura <= 0 ? dura : ((d->loopCount < 0) ? -1 : dura * d->loopCount);
    if (totalDura != -1)
        msecs = qMin(totalDura, msecs);
    d->totalCurrentTime = msecs;

    // Update new values.
    int oldLoop = d->currentLoop;
    d->currentLoop = ((dura <= 0) ? 0 : (msecs / dura));
    if (d->currentLoop == d->loopCount) {
        //we're at the end
        d->currentTime = qMax(0, dura);
        d->currentLoop = qMax(0, d->loopCount - 1);
    } else {
        if (d->direction == Forward) {
            d->currentTime = (dura <= 0) ? msecs : (msecs % dura);
        } else {
            d->currentTime = (dura <= 0) ? msecs : ((msecs - 1) % dura) + 1;
            if (d->currentTime == dura)
                --d->currentLoop;
        }
    }

    updateCurrentTime(d->currentTime);
    if (d->currentLoop != oldLoop)
        emit currentLoopChanged(d->currentLoop);

    // All animations are responsible for stopping the animation when their
    // own end state is reached; in this case the animation is time driven,
    // and has reached the end.
    if ((d->direction == Forward && d->totalCurrentTime == totalDura)
        || (d->direction == Backward && d->totalCurrentTime == 0)) {
        stop();
    }
}

/*!
    Starts the animation. The \a policy argument says whether or not the
    animation should be deleted when it's done. When the animation starts, the
    stateChanged() signal is emitted, and state() returns Running. When control
    reaches the event loop, the animation will run by itself, periodically
    calling updateCurrentTime() as the animation progresses.

    If the animation is currently stopped or has already reached the end,
    calling start() will rewind the animation and start again from the beginning.
    When the animation reaches the end, the animation will either stop, or
    if the loop level is more than 1, it will rewind and continue from the beginning.

    If the animation is already running, this function does nothing.

    \sa stop(), state()
*/
void QAbstractAnimation::start(DeletionPolicy policy)
{
    Q_D(QAbstractAnimation);
    if (d->state == Running)
        return;
    d->deleteWhenStopped = policy;
    d->setState(Running);
}

/*!
    Stops the animation. When the animation is stopped, it emits the stateChanged()
    signal, and state() returns Stopped. The current time is not changed.

    If the animation stops by itself after reaching the end (i.e.,
    currentLoopTime() == duration() and currentLoop() > loopCount() - 1), the
    finished() signal is emitted.

    \sa start(), state()
 */
void QAbstractAnimation::stop()
{
    Q_D(QAbstractAnimation);

    if (d->state == Stopped)
        return;

    d->setState(Stopped);
}

/*!
    Pauses the animation. When the animation is paused, state() returns Paused.
    The value of currentTime will remain unchanged until resume() or start()
    is called. If you want to continue from the current time, call resume().

    \sa start(), state(), resume()
 */
void QAbstractAnimation::pause()
{
    Q_D(QAbstractAnimation);
    if (d->state == Stopped) {
        qWarning("QAbstractAnimation::pause: Cannot pause a stopped animation");
        return;
    }

    d->setState(Paused);
}

/*!
    Resumes the animation after it was paused. When the animation is resumed,
    it emits the resumed() and stateChanged() signals. The currenttime is not
    changed.

    \sa start(), pause(), state()
 */
void QAbstractAnimation::resume()
{
    Q_D(QAbstractAnimation);
    if (d->state != Paused) {
        qWarning("QAbstractAnimation::resume: "
                 "Cannot resume an animation that is not paused");
        return;
    }

    d->setState(Running);
}

/*!
    If \a paused is true, the animation is paused.
    If \a paused is false, the animation is resumed.

    \sa state(), pause(), resume()
*/
void QAbstractAnimation::setPaused(bool paused)
{
    if (paused)
        pause();
    else
        resume();
}


/*!
    \reimp
*/
bool QAbstractAnimation::event(QEvent *event)
{
    return QObject::event(event);
}

/*!
    \fn virtual void QAbstractAnimation::updateCurrentTime(int currentTime) = 0;

    This pure virtual function is called every time the animation's
    \a currentTime changes.

    \sa updateState()
*/

/*!
    This virtual function is called by QAbstractAnimation when the state
    of the animation is changed from \a oldState to \a newState.

    \sa start(), stop(), pause(), resume()
*/
void QAbstractAnimation::updateState(QAbstractAnimation::State newState,
                                     QAbstractAnimation::State oldState)
{
    Q_UNUSED(oldState);
    Q_UNUSED(newState);
}

/*!
    This virtual function is called by QAbstractAnimation when the direction
    of the animation is changed. The \a direction argument is the new direction.

    \sa setDirection(), direction()
*/
void QAbstractAnimation::updateDirection(QAbstractAnimation::Direction direction)
{
    Q_UNUSED(direction);
}


QT_END_NAMESPACE

#include "moc_qabstractanimation.cpp"

#endif //QT_NO_ANIMATION
