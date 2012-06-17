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

#include "qfinalstate.h"

#ifndef QT_NO_STATEMACHINE

#include "qabstractstate_p.h"

QT_BEGIN_NAMESPACE

/*!
  \class QFinalState

  \brief The QFinalState class provides a final state.

  \since 4.6
  \ingroup statemachine

  A final state is used to communicate that (part of) a QStateMachine has
  finished its work. When a final top-level state is entered, the state
  machine's \l{QStateMachine::finished()}{finished}() signal is emitted. In
  general, when a final substate (a child of a QState) is entered, the parent
  state's \l{QState::finished()}{finished}() signal is emitted.  QFinalState
  is part of \l{The State Machine Framework}.

  To use a final state, you create a QFinalState object and add a transition
  to it from another state. Example:

  \code
  QPushButton button;

  QStateMachine machine;
  QState *s1 = new QState();
  QFinalState *s2 = new QFinalState();
  s1->addTransition(&button, SIGNAL(clicked()), s2);
  machine.addState(s1);
  machine.addState(s2);

  QObject::connect(&machine, SIGNAL(finished()), QApplication::instance(), SLOT(quit()));
  machine.setInitialState(s1);
  machine.start();
  \endcode

  \sa QState::finished()
*/

class QFinalStatePrivate : public QAbstractStatePrivate
{
    Q_DECLARE_PUBLIC(QFinalState)

public:
    QFinalStatePrivate();
};

QFinalStatePrivate::QFinalStatePrivate()
    : QAbstractStatePrivate(FinalState)
{
}

/*!
  Constructs a new QFinalState object with the given \a parent state.
*/
QFinalState::QFinalState(QState *parent)
    : QAbstractState(*new QFinalStatePrivate, parent)
{
}

/*!
  Destroys this final state.
*/
QFinalState::~QFinalState()
{
}

/*!
  \reimp
*/
void QFinalState::onEntry(QEvent *event)
{
    Q_UNUSED(event);
}

/*!
  \reimp
*/
void QFinalState::onExit(QEvent *event)
{
    Q_UNUSED(event);
}

/*!
  \reimp
*/
bool QFinalState::event(QEvent *e)
{
    return QAbstractState::event(e);
}

QT_END_NAMESPACE

#endif //QT_NO_STATEMACHINE
