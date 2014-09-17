/****************************************************************************
**
** Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtGui module of the Qt Toolkit.
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

#include "qkeyeventtransition.h"

#ifndef QT_NO_STATEMACHINE

#include "qbasickeyeventtransition_p.h"
#include <QtCore/qstatemachine.h>
#include <private/qeventtransition_p.h>

QT_BEGIN_NAMESPACE

/*!
  \class QKeyEventTransition

  \brief The QKeyEventTransition class provides a transition for key events.

  \since 4.6
  \ingroup statemachine

  QKeyEventTransition is part of \l{The State Machine Framework}.

  \sa QState::addTransition()
*/

/*!
    \property QKeyEventTransition::key

    \brief the key that this key event transition is associated with
*/

/*!
    \property QKeyEventTransition::modifierMask

    \brief the keyboard modifier mask that this key event transition checks for
*/

class QKeyEventTransitionPrivate : public QEventTransitionPrivate
{
    Q_DECLARE_PUBLIC(QKeyEventTransition)
public:
    QKeyEventTransitionPrivate() {}

    QBasicKeyEventTransition *transition;
};

/*!
  Constructs a new key event transition with the given \a sourceState.
*/
QKeyEventTransition::QKeyEventTransition(QState *sourceState)
    : QEventTransition(*new QKeyEventTransitionPrivate, sourceState)
{
    Q_D(QKeyEventTransition);
    d->transition = new QBasicKeyEventTransition();
}

/*!
  Constructs a new key event transition for events of the given \a type for
  the given \a object, with the given \a key and \a sourceState.
*/
QKeyEventTransition::QKeyEventTransition(QObject *object, QEvent::Type type,
                                         int key, QState *sourceState)
    : QEventTransition(*new QKeyEventTransitionPrivate, object, type, sourceState)
{
    Q_D(QKeyEventTransition);
    d->transition = new QBasicKeyEventTransition(type, key);
}

/*!
  Destroys this key event transition.
*/
QKeyEventTransition::~QKeyEventTransition()
{
    Q_D(QKeyEventTransition);
    delete d->transition;
}

/*!
  Returns the key that this key event transition checks for.
*/
int QKeyEventTransition::key() const
{
    Q_D(const QKeyEventTransition);
    return d->transition->key();
}

/*!
  Sets the key that this key event transition will check for.
*/
void QKeyEventTransition::setKey(int key)
{
    Q_D(QKeyEventTransition);
    d->transition->setKey(key);
}

/*!
  Returns the keyboard modifier mask that this key event transition checks
  for.
*/
Qt::KeyboardModifiers QKeyEventTransition::modifierMask() const
{
    Q_D(const QKeyEventTransition);
    return d->transition->modifierMask();
}

/*!
  Sets the keyboard modifier mask that this key event transition will
  check for to \a modifierMask.
*/
void QKeyEventTransition::setModifierMask(Qt::KeyboardModifiers modifierMask)
{
    Q_D(QKeyEventTransition);
    d->transition->setModifierMask(modifierMask);
}

/*!
  \reimp
*/
bool QKeyEventTransition::eventTest(QEvent *event)
{
    Q_D(const QKeyEventTransition);
    if (!QEventTransition::eventTest(event))
        return false;
    QStateMachine::WrappedEvent *we = static_cast<QStateMachine::WrappedEvent*>(event);
    d->transition->setEventType(we->event()->type());
    return QAbstractTransitionPrivate::get(d->transition)->callEventTest(we->event());
}

/*!
  \reimp
*/
void QKeyEventTransition::onTransition(QEvent *event)
{
    QEventTransition::onTransition(event);
}

QT_END_NAMESPACE

#endif //QT_NO_STATEMACHINE
