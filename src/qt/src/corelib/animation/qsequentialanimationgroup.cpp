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
    \class QSequentialAnimationGroup
    \brief The QSequentialAnimationGroup class provides a sequential group of animations.
    \since 4.6
    \ingroup animation

    QSequentialAnimationGroup is a QAnimationGroup that runs its
    animations in sequence, i.e., it starts one animation after
    another has finished playing. The animations are played in the
    order they are added to the group (using
    \l{QAnimationGroup::}{addAnimation()} or
    \l{QAnimationGroup::}{insertAnimation()}). The animation group
    finishes when its last animation has finished.

    At each moment there is at most one animation that is active in
    the group; it is returned by currentAnimation(). An empty group
    has no current animation.

    A sequential animation group can be treated as any other
    animation, i.e., it can be started, stopped, and added to other
    groups. You can also call addPause() or insertPause() to add a
    pause to a sequential animation group.

    \code
        QSequentialAnimationGroup *group = new QSequentialAnimationGroup;

        group->addAnimation(anim1);
        group->addAnimation(anim2);

        group->start();
    \endcode

    In this example, \c anim1 and \c anim2 are two already set up
    \l{QPropertyAnimation}s.

    \sa QAnimationGroup, QAbstractAnimation, {The Animation Framework}
*/

#include "qsequentialanimationgroup.h"
#include "qsequentialanimationgroup_p.h"

#include "qpauseanimation.h"

#include <QtCore/qdebug.h>

#ifndef QT_NO_ANIMATION

QT_BEGIN_NAMESPACE

bool QSequentialAnimationGroupPrivate::atEnd() const
{
    // we try to detect if we're at the end of the group
    //this is true if the following conditions are true:
    // 1. we're in the last loop
    // 2. the direction is forward
    // 3. the current animation is the last one
    // 4. the current animation has reached its end
    const int animTotalCurrentTime = QAbstractAnimationPrivate::get(currentAnimation)->totalCurrentTime;
    return (currentLoop == loopCount - 1
        && direction == QAbstractAnimation::Forward
        && currentAnimation == animations.last()
        && animTotalCurrentTime == animationActualTotalDuration(currentAnimationIndex));
}

int QSequentialAnimationGroupPrivate::animationActualTotalDuration(int index) const
{
    QAbstractAnimation *anim = animations.at(index);
    int ret = anim->totalDuration();
    if (ret == -1 && actualDuration.size() > index)
        ret = actualDuration.at(index); //we can try the actual duration there
    return ret;
}

QSequentialAnimationGroupPrivate::AnimationIndex QSequentialAnimationGroupPrivate::indexForCurrentTime() const
{
    Q_ASSERT(!animations.isEmpty());

    AnimationIndex ret;
    int duration = 0;

    for (int i = 0; i < animations.size(); ++i) {
        duration = animationActualTotalDuration(i);

        // 'animation' is the current animation if one of these reasons is true:
        // 1. it's duration is undefined
        // 2. it ends after msecs
        // 3. it is the last animation (this can happen in case there is at least 1 uncontrolled animation)
        // 4. it ends exactly in msecs and the direction is backwards
        if (duration == -1 || currentTime < (ret.timeOffset + duration)
            || (currentTime == (ret.timeOffset + duration) && direction == QAbstractAnimation::Backward)) {
            ret.index = i;
            return ret;
        }

        // 'animation' has a non-null defined duration and is not the one at time 'msecs'.
        ret.timeOffset += duration;
    }

    // this can only happen when one of those conditions is true:
    // 1. the duration of the group is undefined and we passed its actual duration
    // 2. there are only 0-duration animations in the group
    ret.timeOffset -= duration;
    ret.index = animations.size() - 1;
    return ret;
}

void QSequentialAnimationGroupPrivate::restart()
{
    // restarting the group by making the first/last animation the current one
    if (direction == QAbstractAnimation::Forward) {
        lastLoop = 0;
        if (currentAnimationIndex == 0)
            activateCurrentAnimation();
        else
            setCurrentAnimation(0);
    } else { // direction == QAbstractAnimation::Backward
        lastLoop = loopCount - 1;
        int index = animations.size() - 1;
        if (currentAnimationIndex == index)
            activateCurrentAnimation();
        else
            setCurrentAnimation(index);
    }
}

/*!
    \internal
    This manages advancing the execution of a group running forwards (time has gone forward),
    which is the same behaviour for rewinding the execution of a group running backwards
    (time has gone backward).
*/
void QSequentialAnimationGroupPrivate::advanceForwards(const AnimationIndex &newAnimationIndex)
{
    if (lastLoop < currentLoop) {
        // we need to fast forward to the end
        for (int i = currentAnimationIndex; i < animations.size(); ++i) {
            QAbstractAnimation *anim = animations.at(i);
            setCurrentAnimation(i, true);
            anim->setCurrentTime(animationActualTotalDuration(i));
        }
        // this will make sure the current animation is reset to the beginning
        if (animations.size() == 1)
            // we need to force activation because setCurrentAnimation will have no effect
            activateCurrentAnimation();
        else
            setCurrentAnimation(0, true);
    }

    // and now we need to fast forward from the current position to
    for (int i = currentAnimationIndex; i < newAnimationIndex.index; ++i) {     //### WRONG,
        QAbstractAnimation *anim = animations.at(i);
        setCurrentAnimation(i, true);
        anim->setCurrentTime(animationActualTotalDuration(i));
    }
    // setting the new current animation will happen later
}

/*!
    \internal
    This manages rewinding the execution of a group running forwards (time has gone forward),
    which is the same behaviour for advancing the execution of a group running backwards
    (time has gone backward).
*/
void QSequentialAnimationGroupPrivate::rewindForwards(const AnimationIndex &newAnimationIndex)
{
    if (lastLoop > currentLoop) {
        // we need to fast rewind to the beginning
        for (int i = currentAnimationIndex; i >= 0 ; --i) {
            QAbstractAnimation *anim = animations.at(i);
            setCurrentAnimation(i, true);
            anim->setCurrentTime(0);
        }
        // this will make sure the current animation is reset to the end
        if (animations.size() == 1)
            // we need to force activation because setCurrentAnimation will have no effect
            activateCurrentAnimation();
        else
            setCurrentAnimation(animations.count() - 1, true);
    }

    // and now we need to fast rewind from the current position to
    for (int i = currentAnimationIndex; i > newAnimationIndex.index; --i) {
        QAbstractAnimation *anim = animations.at(i);
        setCurrentAnimation(i, true);
        anim->setCurrentTime(0);
    }
    // setting the new current animation will happen later
}

/*!
    \fn QSequentialAnimationGroup::currentAnimationChanged(QAbstractAnimation *current)

    QSequentialAnimationGroup emits this signal when currentAnimation
    has been changed. \a current is the current animation.

    \sa currentAnimation()
*/


/*!
    Constructs a QSequentialAnimationGroup.
    \a parent is passed to QObject's constructor.
*/
QSequentialAnimationGroup::QSequentialAnimationGroup(QObject *parent)
    : QAnimationGroup(*new QSequentialAnimationGroupPrivate, parent)
{
}

/*!
    \internal
*/
QSequentialAnimationGroup::QSequentialAnimationGroup(QSequentialAnimationGroupPrivate &dd,
                                                     QObject *parent)
    : QAnimationGroup(dd, parent)
{
}

/*!
    Destroys the animation group. It will also destroy all its animations.
*/
QSequentialAnimationGroup::~QSequentialAnimationGroup()
{
}

/*!
    Adds a pause of \a msecs to this animation group.
    The pause is considered as a special type of animation, thus 
    \l{QAnimationGroup::animationCount()}{animationCount} will be 
    increased by one.

    \sa insertPause(), QAnimationGroup::addAnimation()
*/
QPauseAnimation *QSequentialAnimationGroup::addPause(int msecs)
{
    QPauseAnimation *pause = new QPauseAnimation(msecs);
    addAnimation(pause);
    return pause;
}

/*!
    Inserts a pause of \a msecs milliseconds at \a index in this animation
    group.

    \sa addPause(), QAnimationGroup::insertAnimation()
*/
QPauseAnimation *QSequentialAnimationGroup::insertPause(int index, int msecs)
{
    Q_D(const QSequentialAnimationGroup);

    if (index < 0 || index > d->animations.size()) {
        qWarning("QSequentialAnimationGroup::insertPause: index is out of bounds");
        return 0;
    }

    QPauseAnimation *pause = new QPauseAnimation(msecs);
    insertAnimation(index, pause);
    return pause;
}


/*!
    \property QSequentialAnimationGroup::currentAnimation
    Returns the animation in the current time.

    \sa currentAnimationChanged()
*/
QAbstractAnimation *QSequentialAnimationGroup::currentAnimation() const
{
    Q_D(const QSequentialAnimationGroup);
    return d->currentAnimation;
}

/*!
    \reimp
*/
int QSequentialAnimationGroup::duration() const
{
    Q_D(const QSequentialAnimationGroup);
    int ret = 0;

    for (int i = 0; i < d->animations.size(); ++i) {
        QAbstractAnimation *animation = d->animations.at(i);
        const int currentDuration = animation->totalDuration();
        if (currentDuration == -1)
            return -1; // Undetermined length

        ret += currentDuration;
    }

    return ret;
}

/*!
    \reimp
*/
void QSequentialAnimationGroup::updateCurrentTime(int currentTime)
{
    Q_D(QSequentialAnimationGroup);
    if (!d->currentAnimation)
        return;

    const QSequentialAnimationGroupPrivate::AnimationIndex newAnimationIndex = d->indexForCurrentTime();

    // remove unneeded animations from actualDuration list
    while (newAnimationIndex.index < d->actualDuration.size())
        d->actualDuration.removeLast();

    // newAnimationIndex.index is the new current animation
    if (d->lastLoop < d->currentLoop
        || (d->lastLoop == d->currentLoop && d->currentAnimationIndex < newAnimationIndex.index)) {
            // advancing with forward direction is the same as rewinding with backwards direction
            d->advanceForwards(newAnimationIndex);
    } else if (d->lastLoop > d->currentLoop
        || (d->lastLoop == d->currentLoop && d->currentAnimationIndex > newAnimationIndex.index)) {
            // rewinding with forward direction is the same as advancing with backwards direction
            d->rewindForwards(newAnimationIndex);
    }

    d->setCurrentAnimation(newAnimationIndex.index);

    const int newCurrentTime = currentTime - newAnimationIndex.timeOffset;

    if (d->currentAnimation) {
        d->currentAnimation->setCurrentTime(newCurrentTime);
        if (d->atEnd()) {
            //we make sure that we don't exceed the duration here
            d->currentTime += QAbstractAnimationPrivate::get(d->currentAnimation)->totalCurrentTime - newCurrentTime;
            stop();
        }
    } else {
        //the only case where currentAnimation could be null
        //is when all animations have been removed
        Q_ASSERT(d->animations.isEmpty());
        d->currentTime = 0;
        stop();
    }

    d->lastLoop = d->currentLoop;
}

/*!
    \reimp
*/
void QSequentialAnimationGroup::updateState(QAbstractAnimation::State newState,
                                            QAbstractAnimation::State oldState)
{
    Q_D(QSequentialAnimationGroup);
    QAnimationGroup::updateState(newState, oldState);

    if (!d->currentAnimation)
        return;

    switch (newState) {
    case Stopped:
        d->currentAnimation->stop();
        break;
    case Paused:
        if (oldState == d->currentAnimation->state()
            && oldState == QSequentialAnimationGroup::Running) {
                d->currentAnimation->pause();
            }
        else
            d->restart();
        break;
    case Running:
        if (oldState == d->currentAnimation->state()
            && oldState == QSequentialAnimationGroup::Paused)
            d->currentAnimation->start();
        else
            d->restart();
        break;
    }
}

/*!
    \reimp
*/
void QSequentialAnimationGroup::updateDirection(QAbstractAnimation::Direction direction)
{
    Q_D(QSequentialAnimationGroup);
    // we need to update the direction of the current animation
    if (state() != Stopped && d->currentAnimation)
        d->currentAnimation->setDirection(direction);
}

/*!
    \reimp
*/
bool QSequentialAnimationGroup::event(QEvent *event)
{
    return QAnimationGroup::event(event);
}

void QSequentialAnimationGroupPrivate::setCurrentAnimation(int index, bool intermediate)
{
    Q_Q(QSequentialAnimationGroup);

    index = qMin(index, animations.count() - 1);

    if (index == -1) {
        Q_ASSERT(animations.isEmpty());
        currentAnimationIndex = -1;
        currentAnimation = 0;
        return;
    }

    // need these two checks below because this func can be called after the current animation
    // has been removed
    if (index == currentAnimationIndex && animations.at(index) == currentAnimation)
        return;

    // stop the old current animation
    if (currentAnimation)
        currentAnimation->stop();

    currentAnimation = animations.at(index);
    currentAnimationIndex = index;

    emit q->currentAnimationChanged(currentAnimation);

    activateCurrentAnimation(intermediate);
}

void QSequentialAnimationGroupPrivate::activateCurrentAnimation(bool intermediate)
{
    if (!currentAnimation || state == QSequentialAnimationGroup::Stopped)
        return;

    currentAnimation->stop();

    // we ensure the direction is consistent with the group's direction
    currentAnimation->setDirection(direction);

    // connects to the finish signal of uncontrolled animations
    if (currentAnimation->totalDuration() == -1)
        connectUncontrolledAnimation(currentAnimation);

    currentAnimation->start();
    if (!intermediate && state == QSequentialAnimationGroup::Paused)
        currentAnimation->pause();
}

void QSequentialAnimationGroupPrivate::_q_uncontrolledAnimationFinished()
{
    Q_Q(QSequentialAnimationGroup);
    Q_ASSERT(qobject_cast<QAbstractAnimation *>(q->sender()) == currentAnimation);

    // we trust the duration returned by the animation
    while (actualDuration.size() < (currentAnimationIndex + 1))
        actualDuration.append(-1);
    actualDuration[currentAnimationIndex] = currentAnimation->currentTime();

    disconnectUncontrolledAnimation(currentAnimation);

    if ((direction == QAbstractAnimation::Forward && currentAnimation == animations.last())
        || (direction == QAbstractAnimation::Backward && currentAnimationIndex == 0)) {
        // we don't handle looping of a group with undefined duration
        q->stop();
    } else if (direction == QAbstractAnimation::Forward) {
        // set the current animation to be the next one
        setCurrentAnimation(currentAnimationIndex + 1);
    } else {
        // set the current animation to be the previous one
        setCurrentAnimation(currentAnimationIndex - 1);
    }
}

/*!
    \internal
    This method is called whenever an animation is added to
    the group at index \a index.
    Note: We only support insertion after the current animation
*/
void QSequentialAnimationGroupPrivate::animationInsertedAt(int index)
{
    if (currentAnimation == 0)
        setCurrentAnimation(0); // initialize the current animation

    if (currentAnimationIndex == index
        && currentAnimation->currentTime() == 0 && currentAnimation->currentLoop() == 0) {
            //in this case we simply insert an animation before the current one has actually started
            setCurrentAnimation(index);
    }

    //we update currentAnimationIndex in case it has changed (the animation pointer is still valid)
    currentAnimationIndex = animations.indexOf(currentAnimation);

    if (index < currentAnimationIndex || currentLoop != 0) {
        qWarning("QSequentialGroup::insertAnimation only supports to add animations after the current one.");
        return; //we're not affected because it is added after the current one
    }
}

/*!
    \internal
    This method is called whenever an animation is removed from
    the group at index \a index. The animation is no more listed when this
    method is called.
*/
void QSequentialAnimationGroupPrivate::animationRemoved(int index, QAbstractAnimation *anim)
{
    Q_Q(QSequentialAnimationGroup);
    QAnimationGroupPrivate::animationRemoved(index, anim);

    Q_ASSERT(currentAnimation); // currentAnimation should always be set

    if (actualDuration.size() > index)
        actualDuration.removeAt(index);

    const int currentIndex = animations.indexOf(currentAnimation);
    if (currentIndex == -1) {
        //we're removing the current animation

        disconnectUncontrolledAnimation(currentAnimation);

        if (index < animations.count())
            setCurrentAnimation(index); //let's try to take the next one
        else if (index > 0)
            setCurrentAnimation(index - 1);
        else// case all animations were removed
            setCurrentAnimation(-1);
    } else if (currentAnimationIndex > index) {
        currentAnimationIndex--;
    }

    // duration of the previous animations up to the current animation
    currentTime = 0;
    for (int i = 0; i < currentAnimationIndex; ++i) {
        const int current = animationActualTotalDuration(i);
        currentTime += current;
    }

    if (currentIndex != -1) {
        //the current animation is not the one being removed
        //so we add its current time to the current time of this group
        currentTime += QAbstractAnimationPrivate::get(currentAnimation)->totalCurrentTime;
    }

    //let's also update the total current time
    totalCurrentTime = currentTime + loopCount * q->duration();
}

QT_END_NAMESPACE

#include "moc_qsequentialanimationgroup.cpp"

#endif //QT_NO_ANIMATION
