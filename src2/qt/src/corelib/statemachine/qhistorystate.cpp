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

#include "qhistorystate.h"

#ifndef QT_NO_STATEMACHINE

#include "qhistorystate_p.h"

QT_BEGIN_NAMESPACE

/*!
  \class QHistoryState

  \brief The QHistoryState class provides a means of returning to a previously active substate.

  \since 4.6
  \ingroup statemachine

  A history state is a pseudo-state that represents the child state that the
  parent state was in the last time the parent state was exited. A transition
  with a history state as its target is in fact a transition to one of the
  other child states of the parent state. QHistoryState is part of \l{The
  State Machine Framework}.

  Use the setDefaultState() function to set the state that should be entered
  if the parent state has never been entered.  Example:

  \code
  QStateMachine machine;

  QState *s1 = new QState();
  QState *s11 = new QState(s1);
  QState *s12 = new QState(s1);

  QHistoryState *s1h = new QHistoryState(s1);
  s1h->setDefaultState(s11);

  machine.addState(s1);

  QState *s2 = new QState();
  machine.addState(s2);

  QPushButton *button = new QPushButton();
  // Clicking the button will cause the state machine to enter the child state
  // that s1 was in the last time s1 was exited, or the history state's default
  // state if s1 has never been entered.
  s1->addTransition(button, SIGNAL(clicked()), s1h);
  \endcode

  By default a history state is shallow, meaning that it won't remember nested
  states. This can be configured through the historyType property.
*/

/*!
  \property QHistoryState::defaultState

  \brief the default state of this history state
*/

/*!
  \property QHistoryState::historyType

  \brief the type of history that this history state records

  The default value of this property is QHistoryState::ShallowHistory.
*/

/*!
  \enum QHistoryState::HistoryType

  This enum specifies the type of history that a QHistoryState records.

  \value ShallowHistory Only the immediate child states of the parent state
  are recorded. In this case a transition with the history state as its
  target will end up in the immediate child state that the parent was in the
  last time it was exited. This is the default.

  \value DeepHistory Nested states are recorded. In this case a transition
  with the history state as its target will end up in the most deeply nested
  descendant state the parent was in the last time it was exited.
*/

QHistoryStatePrivate::QHistoryStatePrivate()
    : QAbstractStatePrivate(HistoryState),
      defaultState(0), historyType(QHistoryState::ShallowHistory)
{
}

QHistoryStatePrivate *QHistoryStatePrivate::get(QHistoryState *q)
{
    return q->d_func();
}

/*!
  Constructs a new shallow history state with the given \a parent state.
*/
QHistoryState::QHistoryState(QState *parent)
    : QAbstractState(*new QHistoryStatePrivate, parent)
{
}
/*!
  Constructs a new history state of the given \a type, with the given \a
  parent state.
*/
QHistoryState::QHistoryState(HistoryType type, QState *parent)
    : QAbstractState(*new QHistoryStatePrivate, parent)
{
    Q_D(QHistoryState);
    d->historyType = type;
}

/*!
  Destroys this history state.
*/
QHistoryState::~QHistoryState()
{
}

/*!
  Returns this history state's default state.  The default state indicates the
  state to transition to if the parent state has never been entered before.
*/
QAbstractState *QHistoryState::defaultState() const
{
    Q_D(const QHistoryState);
    return d->defaultState;
}

/*!
  Sets this history state's default state to be the given \a state.
  \a state must be a sibling of this history state.

  Note that this function does not set \a state as the initial state
  of its parent.
*/
void QHistoryState::setDefaultState(QAbstractState *state)
{
    Q_D(QHistoryState);
    if (state && state->parentState() != parentState()) {
        qWarning("QHistoryState::setDefaultState: state %p does not belong "
                 "to this history state's group (%p)", state, parentState());
        return;
    }
    d->defaultState = state;
}

/*!
  Returns the type of history that this history state records.
*/
QHistoryState::HistoryType QHistoryState::historyType() const
{
    Q_D(const QHistoryState);
    return d->historyType;
}

/*!
  Sets the \a type of history that this history state records.
*/
void QHistoryState::setHistoryType(HistoryType type)
{
    Q_D(QHistoryState);
    d->historyType = type;
}

/*!
  \reimp
*/
void QHistoryState::onEntry(QEvent *event)
{
    Q_UNUSED(event);
}

/*!
  \reimp
*/
void QHistoryState::onExit(QEvent *event)
{
    Q_UNUSED(event);
}

/*!
  \reimp
*/
bool QHistoryState::event(QEvent *e)
{
    return QAbstractState::event(e);
}

QT_END_NAMESPACE

#endif //QT_NO_STATEMACHINE
