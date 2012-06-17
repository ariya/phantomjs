/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
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

#include "qabstracttransition.h"

#ifndef QT_NO_STATEMACHINE

#include "qabstracttransition_p.h"
#include "qabstractstate.h"
#include "qstate.h"
#include "qstatemachine.h"

QT_BEGIN_NAMESPACE

/*!
  \class QAbstractTransition

  \brief The QAbstractTransition class is the base class of transitions between QAbstractState objects.

  \since 4.6
  \ingroup statemachine

  The QAbstractTransition class is the abstract base class of transitions
  between states (QAbstractState objects) of a
  QStateMachine. QAbstractTransition is part of \l{The State Machine
  Framework}.

  The sourceState() function returns the source of the transition. The
  targetStates() function returns the targets of the transition. The machine()
  function returns the state machine that the transition is part of.

  The triggered() signal is emitted when the transition has been triggered.

  Transitions can cause animations to be played. Use the addAnimation()
  function to add an animation to the transition.

  \section1 Subclassing

  The eventTest() function is called by the state machine to determine whether
  an event should trigger the transition. In your reimplementation you
  typically check the event type and cast the event object to the proper type,
  and check that one or more properties of the event meet your criteria.

  The onTransition() function is called when the transition is triggered;
  reimplement this function to perform custom processing for the transition.
*/

/*!
    \property QAbstractTransition::sourceState

    \brief the source state (parent) of this transition
*/

/*!
    \property QAbstractTransition::targetState

    \brief the target state of this transition

    If a transition has no target state, the transition may still be
    triggered, but this will not cause the state machine's configuration to
    change (i.e. the current state will not be exited and re-entered).
*/

/*!
    \property QAbstractTransition::targetStates

    \brief the target states of this transition

    If multiple states are specified, all must be descendants of the same
    parallel group state.
*/

QAbstractTransitionPrivate::QAbstractTransitionPrivate()
{
}

QAbstractTransitionPrivate *QAbstractTransitionPrivate::get(QAbstractTransition *q)
{
    return q->d_func();
}

QStateMachine *QAbstractTransitionPrivate::machine() const
{
    QState *source = sourceState();
    if (!source)
        return 0;
    return source->machine();
}

bool QAbstractTransitionPrivate::callEventTest(QEvent *e)
{
    Q_Q(QAbstractTransition);
    return q->eventTest(e);
}

void QAbstractTransitionPrivate::callOnTransition(QEvent *e)
{
    Q_Q(QAbstractTransition);
    q->onTransition(e);
}

QState *QAbstractTransitionPrivate::sourceState() const
{
    return qobject_cast<QState*>(parent);
}

void QAbstractTransitionPrivate::emitTriggered()
{
    Q_Q(QAbstractTransition);
    emit q->triggered();
}

/*!
  Constructs a new QAbstractTransition object with the given \a sourceState.
*/
QAbstractTransition::QAbstractTransition(QState *sourceState)
    : QObject(*new QAbstractTransitionPrivate, sourceState)
{
}

/*!
  \internal
*/
QAbstractTransition::QAbstractTransition(QAbstractTransitionPrivate &dd,
                                         QState *parent)
    : QObject(dd, parent)
{
}

/*!
  Destroys this transition.
*/
QAbstractTransition::~QAbstractTransition()
{
}

/*!
  Returns the source state of this transition, or 0 if this transition has no
  source state.
*/
QState *QAbstractTransition::sourceState() const
{
    Q_D(const QAbstractTransition);
    return d->sourceState();
}

/*!
  Returns the target state of this transition, or 0 if the transition has no
  target.
*/
QAbstractState *QAbstractTransition::targetState() const
{
    Q_D(const QAbstractTransition);
    if (d->targetStates.isEmpty())
        return 0;
    return d->targetStates.first().data();
}

/*!
  Sets the \a target state of this transition.
*/
void QAbstractTransition::setTargetState(QAbstractState* target)
{
    Q_D(QAbstractTransition);
    if (!target)
        d->targetStates.clear();
    else
        setTargetStates(QList<QAbstractState*>() << target);
}

/*!
  Returns the target states of this transition, or an empty list if this
  transition has no target states.
*/
QList<QAbstractState*> QAbstractTransition::targetStates() const
{
    Q_D(const QAbstractTransition);
    QList<QAbstractState*> result;
    for (int i = 0; i < d->targetStates.size(); ++i) {
        QAbstractState *target = d->targetStates.at(i).data();
        if (target)
            result.append(target);
    }
    return result;
}

/*!
  Sets the target states of this transition to be the given \a targets.
*/
void QAbstractTransition::setTargetStates(const QList<QAbstractState*> &targets)
{
    Q_D(QAbstractTransition);

    for (int i = 0; i < targets.size(); ++i) {
        QAbstractState *target = targets.at(i);
        if (!target) {
            qWarning("QAbstractTransition::setTargetStates: target state(s) cannot be null");
            return;
        }
    }

    d->targetStates.clear();
    for (int i = 0; i < targets.size(); ++i)
        d->targetStates.append(targets.at(i));
}

/*!
  Returns the state machine that this transition is part of, or 0 if the
  transition is not part of a state machine.
*/
QStateMachine *QAbstractTransition::machine() const
{
    Q_D(const QAbstractTransition);
    return d->machine();
}

#ifndef QT_NO_ANIMATION

/*!
  Adds the given \a animation to this transition.
  The transition does not take ownership of the animation.

  \sa removeAnimation(), animations()
*/
void QAbstractTransition::addAnimation(QAbstractAnimation *animation)
{
    Q_D(QAbstractTransition);
    if (!animation) {
        qWarning("QAbstractTransition::addAnimation: cannot add null animation");
        return;
    }
    d->animations.append(animation);
}

/*!
  Removes the given \a animation from this transition.

  \sa addAnimation()
*/
void QAbstractTransition::removeAnimation(QAbstractAnimation *animation)
{
    Q_D(QAbstractTransition);
    if (!animation) {
        qWarning("QAbstractTransition::removeAnimation: cannot remove null animation");
        return;
    }
    d->animations.removeOne(animation);
}

/*!
  Returns the list of animations associated with this transition, or an empty
  list if it has no animations.

  \sa addAnimation()
*/
QList<QAbstractAnimation*> QAbstractTransition::animations() const
{
    Q_D(const QAbstractTransition);
    return d->animations;
}

#endif

/*!
  \fn QAbstractTransition::eventTest(QEvent *event)

  This function is called to determine whether the given \a event should cause
  this transition to trigger. Reimplement this function and return true if the
  event should trigger the transition, otherwise return false.
*/

/*!
  \fn QAbstractTransition::onTransition(QEvent *event)

  This function is called when the transition is triggered. The given \a event
  is what caused the transition to trigger. Reimplement this function to
  perform custom processing when the transition is triggered.
*/

/*!
  \fn QAbstractTransition::triggered()

  This signal is emitted when the transition has been triggered (after
  onTransition() has been called).
*/

/*!
  \reimp
*/
bool QAbstractTransition::event(QEvent *e)
{
    return QObject::event(e);
}

QT_END_NAMESPACE

#endif //QT_NO_STATEMACHINE
