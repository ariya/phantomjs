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

#include "qstate.h"

#ifndef QT_NO_STATEMACHINE

#include "qstate_p.h"
#include "qhistorystate.h"
#include "qhistorystate_p.h"
#include "qabstracttransition.h"
#include "qabstracttransition_p.h"
#include "qsignaltransition.h"
#include "qstatemachine.h"
#include "qstatemachine_p.h"

QT_BEGIN_NAMESPACE

/*!
  \class QState

  \brief The QState class provides a general-purpose state for QStateMachine.

  \since 4.6
  \ingroup statemachine

  QState objects can have child states, and can have transitions to other
  states. QState is part of \l{The State Machine Framework}.

  The addTransition() function adds a transition. The removeTransition()
  function removes a transition. The transitions() function returns the
  state's outgoing transitions.

  The assignProperty() function is used for defining property assignments that
  should be performed when a state is entered.

  Top-level states must be passed a QStateMachine object as their parent
  state, or added to a state machine using QStateMachine::addState().

  \section1 States with Child States

  The childMode property determines how child states are treated. For
  non-parallel state groups, the setInitialState() function must be called to
  set the initial state. The child states are mutually exclusive states, and
  the state machine needs to know which child state to enter when the parent
  state is the target of a transition.

  The state emits the QState::finished() signal when a final child state
  (QFinalState) is entered.

  The setErrorState() sets the state's error state. The error state is the
  state that the state machine will transition to if an error is detected when
  attempting to enter the state (e.g. because no initial state has been set).

*/

/*!
    \property QState::initialState

    \brief the initial state of this state (one of its child states)
*/

/*!
    \property QState::errorState

    \brief the error state of this state
*/

/*!
    \property QState::childMode

    \brief the child mode of this state

    The default value of this property is QState::ExclusiveStates.
*/

/*!
  \enum QState::ChildMode

  This enum specifies how a state's child states are treated.

  \value ExclusiveStates The child states are mutually exclusive and an
  initial state must be set by calling QState::setInitialState().

  \value ParallelStates The child states are parallel. When the parent state
  is entered, all its child states are entered in parallel.
*/

QStatePrivate::QStatePrivate()
    : QAbstractStatePrivate(StandardState),
      errorState(0), initialState(0), childMode(QState::ExclusiveStates),
      childStatesListNeedsRefresh(true), transitionsListNeedsRefresh(true)
{
}

QStatePrivate::~QStatePrivate()
{
}

void QStatePrivate::emitFinished()
{
    Q_Q(QState);
    emit q->finished();
}

void QStatePrivate::emitPropertiesAssigned()
{
    Q_Q(QState);
    emit q->propertiesAssigned();
}

/*!
  Constructs a new state with the given \a parent state.
*/
QState::QState(QState *parent)
    : QAbstractState(*new QStatePrivate, parent)
{
}

/*!
  Constructs a new state with the given \a childMode and the given \a parent
  state.
*/
QState::QState(ChildMode childMode, QState *parent)
    : QAbstractState(*new QStatePrivate, parent)
{
    Q_D(QState);
    d->childMode = childMode;
}

/*!
  \internal
*/
QState::QState(QStatePrivate &dd, QState *parent)
    : QAbstractState(dd, parent)
{
}

/*!
  Destroys this state.
*/
QState::~QState()
{
}

QList<QAbstractState*> QStatePrivate::childStates() const
{
    if (childStatesListNeedsRefresh) {
        childStatesList.clear();
        QList<QObject*>::const_iterator it;
        for (it = children.constBegin(); it != children.constEnd(); ++it) {
            QAbstractState *s = qobject_cast<QAbstractState*>(*it);
            if (!s || qobject_cast<QHistoryState*>(s))
                continue;
            childStatesList.append(s);
        }
        childStatesListNeedsRefresh = false;
    }
    return childStatesList;
}

QList<QHistoryState*> QStatePrivate::historyStates() const
{
    QList<QHistoryState*> result;
    QList<QObject*>::const_iterator it;
    for (it = children.constBegin(); it != children.constEnd(); ++it) {
        QHistoryState *h = qobject_cast<QHistoryState*>(*it);
        if (h)
            result.append(h);
    }
    return result;
}

QList<QAbstractTransition*> QStatePrivate::transitions() const
{
    if (transitionsListNeedsRefresh) {
        transitionsList.clear();
        QList<QObject*>::const_iterator it;
        for (it = children.constBegin(); it != children.constEnd(); ++it) {
            QAbstractTransition *t = qobject_cast<QAbstractTransition*>(*it);
            if (t)
                transitionsList.append(t);
        }
        transitionsListNeedsRefresh = false;
    }
    return transitionsList;
}

#ifndef QT_NO_PROPERTIES

/*!
  Instructs this state to set the property with the given \a name of the given
  \a object to the given \a value when the state is entered.

  \sa propertiesAssigned()
*/
void QState::assignProperty(QObject *object, const char *name,
                            const QVariant &value)
{
    Q_D(QState);
    if (!object) {
        qWarning("QState::assignProperty: cannot assign property '%s' of null object", name);
        return;
    }
    for (int i = 0; i < d->propertyAssignments.size(); ++i) {
        QPropertyAssignment &assn = d->propertyAssignments[i];
        if ((assn.object == object) && (assn.propertyName == name)) {
            assn.value = value;
            return;
        }
    }
    d->propertyAssignments.append(QPropertyAssignment(object, name, value));
}

#endif // QT_NO_PROPERTIES

/*!
  Returns this state's error state. 

  \sa QStateMachine::error()
*/
QAbstractState *QState::errorState() const
{
    Q_D(const QState);
    return d->errorState;
}

/*!
  Sets this state's error state to be the given \a state. If the error state
  is not set, or if it is set to 0, the state will inherit its parent's error
  state recursively. If no error state is set for the state itself or any of 
  its ancestors, an error will cause the machine to stop executing and an error
  will be printed to the console.
*/
void QState::setErrorState(QAbstractState *state)
{
    Q_D(QState);
    if (state != 0 && qobject_cast<QStateMachine*>(state)) {
        qWarning("QStateMachine::setErrorState: root state cannot be error state");
        return;
    }
    if (state != 0 && (!state->machine() || ((state->machine() != machine()) && !qobject_cast<QStateMachine*>(this)))) {
        qWarning("QState::setErrorState: error state cannot belong "
                 "to a different state machine");
        return;
    }

    d->errorState = state;
}

/*!
  Adds the given \a transition. The transition has this state as the source.
  This state takes ownership of the transition. 
*/
void QState::addTransition(QAbstractTransition *transition)
{
    Q_D(QState);
    if (!transition) {
        qWarning("QState::addTransition: cannot add null transition");
        return ;
    }

    transition->setParent(this);
    const QList<QWeakPointer<QAbstractState> > &targets = QAbstractTransitionPrivate::get(transition)->targetStates;
    for (int i = 0; i < targets.size(); ++i) {
        QAbstractState *t = targets.at(i).data();
        if (!t) {
            qWarning("QState::addTransition: cannot add transition to null state");
            return ;
        }
        if ((QAbstractStatePrivate::get(t)->machine() != d->machine())
            && QAbstractStatePrivate::get(t)->machine() && d->machine()) {
            qWarning("QState::addTransition: cannot add transition "
                     "to a state in a different state machine");
            return ;
        }
    }
    if (machine() != 0 && machine()->configuration().contains(this))
        QStateMachinePrivate::get(machine())->registerTransitions(this);
}

/*!
  Adds a transition associated with the given \a signal of the given \a sender
  object, and returns the new QSignalTransition object. The transition has
  this state as the source, and the given \a target as the target state.
*/
QSignalTransition *QState::addTransition(QObject *sender, const char *signal,
                                         QAbstractState *target)
{
    if (!sender) {
        qWarning("QState::addTransition: sender cannot be null");
        return 0;
    }
    if (!signal) {
        qWarning("QState::addTransition: signal cannot be null");
        return 0;
    }
    if (!target) {
        qWarning("QState::addTransition: cannot add transition to null state");
        return 0;
    }
    int offset = (*signal == '0'+QSIGNAL_CODE) ? 1 : 0;
    const QMetaObject *meta = sender->metaObject();
    if (meta->indexOfSignal(signal+offset) == -1) {
        if (meta->indexOfSignal(QMetaObject::normalizedSignature(signal+offset)) == -1) {
            qWarning("QState::addTransition: no such signal %s::%s",
                     meta->className(), signal+offset);
            return 0;
        }
    }
    QSignalTransition *trans = new QSignalTransition(sender, signal);
    trans->setTargetState(target);
    addTransition(trans);
    return trans;
}

namespace {

// ### Make public?
class UnconditionalTransition : public QAbstractTransition
{
public:
    UnconditionalTransition(QAbstractState *target)
        : QAbstractTransition()
    { setTargetState(target); }
protected:
    void onTransition(QEvent *) {}
    bool eventTest(QEvent *) { return true; }
};

} // namespace

/*!
  Adds an unconditional transition from this state to the given \a target
  state, and returns then new transition object.
*/
QAbstractTransition *QState::addTransition(QAbstractState *target)
{
    if (!target) {
        qWarning("QState::addTransition: cannot add transition to null state");
        return 0;
    }
    UnconditionalTransition *trans = new UnconditionalTransition(target);
    addTransition(trans);
    return trans;
}

/*!
  Removes the given \a transition from this state.  The state releases
  ownership of the transition.

  \sa addTransition()
*/
void QState::removeTransition(QAbstractTransition *transition)
{
    Q_D(QState);
    if (!transition) {
        qWarning("QState::removeTransition: cannot remove null transition");
        return;
    }
    if (transition->sourceState() != this) {
        qWarning("QState::removeTransition: transition %p's source state (%p)"
                 " is different from this state (%p)",
                 transition, transition->sourceState(), this);
        return;
    }
    QStateMachinePrivate *mach = QStateMachinePrivate::get(d->machine());
    if (mach)
        mach->unregisterTransition(transition);
    transition->setParent(0);
}

/*!
  \since 4.7

  Returns this state's outgoing transitions (i.e. transitions where
  this state is the \l{QAbstractTransition::sourceState()}{source
  state}), or an empty list if this state has no outgoing transitions.

  \sa addTransition()
*/
QList<QAbstractTransition*> QState::transitions() const
{
    Q_D(const QState);
    return d->transitions();
}

/*!
  \reimp
*/
void QState::onEntry(QEvent *event)
{
    Q_UNUSED(event);
}

/*!
  \reimp
*/
void QState::onExit(QEvent *event)
{
    Q_UNUSED(event);
}

/*!
  Returns this state's initial state, or 0 if the state has no initial state.
*/
QAbstractState *QState::initialState() const
{
    Q_D(const QState);
    return d->initialState;
}

/*!
  Sets this state's initial state to be the given \a state.
  \a state has to be a child of this state.
*/
void QState::setInitialState(QAbstractState *state)
{
    Q_D(QState);
    if (d->childMode == QState::ParallelStates) {
        qWarning("QState::setInitialState: ignoring attempt to set initial state "
                 "of parallel state group %p", this);
        return;
    }
    if (state && (state->parentState() != this)) {
        qWarning("QState::setInitialState: state %p is not a child of this state (%p)",
                 state, this);
        return;
    }
    d->initialState = state;
}

/*!
  Returns the child mode of this state.
*/
QState::ChildMode QState::childMode() const
{
    Q_D(const QState);
    return d->childMode;
}

/*!
  Sets the child \a mode of this state.
*/
void QState::setChildMode(ChildMode mode)
{
    Q_D(QState);
    d->childMode = mode;
}

/*!
  \reimp
*/
bool QState::event(QEvent *e)
{
    Q_D(QState);
    if ((e->type() == QEvent::ChildAdded) || (e->type() == QEvent::ChildRemoved)) {
        d->childStatesListNeedsRefresh = true;
        d->transitionsListNeedsRefresh = true;
    }
    return QAbstractState::event(e);
}

/*!
  \fn QState::finished()

  This signal is emitted when a final child state of this state is entered.

  \sa QFinalState
*/

/*!
  \fn QState::propertiesAssigned()

  This signal is emitted when all properties have been assigned their final value. If the state
  assigns a value to one or more properties for which an animation exists (either set on the
  transition or as a default animation on the state machine), then the signal will not be emitted
  until all such animations have finished playing.

  If there are no relevant animations, or no property assignments defined for the state, then
  the signal will be emitted immediately before the state is entered.

  \sa QState::assignProperty(), QAbstractTransition::addAnimation()
*/

QT_END_NAMESPACE

#endif //QT_NO_STATEMACHINE
