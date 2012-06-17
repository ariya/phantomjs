/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtGui module of the Qt Toolkit.
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

#include "qbasickeyeventtransition_p.h"

#ifndef QT_NO_STATEMACHINE

#include <QtGui/qevent.h>
#include <qdebug.h>
#include <private/qabstracttransition_p.h>

QT_BEGIN_NAMESPACE

/*!
  \internal
  \class QBasicKeyEventTransition
  \since 4.6
  \ingroup statemachine

  \brief The QBasicKeyEventTransition class provides a transition for Qt key events.
*/

class QBasicKeyEventTransitionPrivate : public QAbstractTransitionPrivate
{
    Q_DECLARE_PUBLIC(QBasicKeyEventTransition)
public:
    QBasicKeyEventTransitionPrivate();

    static QBasicKeyEventTransitionPrivate *get(QBasicKeyEventTransition *q);

    QEvent::Type eventType;
    int key;
    Qt::KeyboardModifiers modifierMask;
};

QBasicKeyEventTransitionPrivate::QBasicKeyEventTransitionPrivate()
{
    eventType = QEvent::None;
    key = 0;
    modifierMask = Qt::NoModifier;
}

QBasicKeyEventTransitionPrivate *QBasicKeyEventTransitionPrivate::get(QBasicKeyEventTransition *q)
{
    return q->d_func();
}

/*!
  Constructs a new key event transition with the given \a sourceState.
*/
QBasicKeyEventTransition::QBasicKeyEventTransition(QState *sourceState)
    : QAbstractTransition(*new QBasicKeyEventTransitionPrivate, sourceState)
{
}

/*!
  Constructs a new event transition for events of the given \a type for the
  given \a key, with the given \a sourceState.
*/
QBasicKeyEventTransition::QBasicKeyEventTransition(QEvent::Type type, int key,
                                                   QState *sourceState)
    : QAbstractTransition(*new QBasicKeyEventTransitionPrivate, sourceState)
{
    Q_D(QBasicKeyEventTransition);
    d->eventType = type;
    d->key = key;
}

/*!
  Constructs a new event transition for events of the given \a type for the
  given \a key, with the given \a modifierMask and \a sourceState.
*/
QBasicKeyEventTransition::QBasicKeyEventTransition(QEvent::Type type, int key,
                                                   Qt::KeyboardModifiers modifierMask,
                                                   QState *sourceState)
    : QAbstractTransition(*new QBasicKeyEventTransitionPrivate, sourceState)
{
    Q_D(QBasicKeyEventTransition);
    d->eventType = type;
    d->key = key;
    d->modifierMask = modifierMask;
}

/*!
  Destroys this event transition.
*/
QBasicKeyEventTransition::~QBasicKeyEventTransition()
{
}

/*!
  Returns the event type that this key event transition is associated with.
*/
QEvent::Type QBasicKeyEventTransition::eventType() const
{
    Q_D(const QBasicKeyEventTransition);
    return d->eventType;
}

/*!
  Sets the event \a type that this key event transition is associated with.
*/
void QBasicKeyEventTransition::setEventType(QEvent::Type type)
{
    Q_D(QBasicKeyEventTransition);
    d->eventType = type;
}

/*!
  Returns the key that this key event transition checks for.
*/
int QBasicKeyEventTransition::key() const
{
    Q_D(const QBasicKeyEventTransition);
    return d->key;
}

/*!
  Sets the key that this key event transition will check for.
*/
void QBasicKeyEventTransition::setKey(int key)
{
    Q_D(QBasicKeyEventTransition);
    d->key = key;
}

/*!
  Returns the keyboard modifier mask that this key event transition checks
  for.
*/
Qt::KeyboardModifiers QBasicKeyEventTransition::modifierMask() const
{
    Q_D(const QBasicKeyEventTransition);
    return d->modifierMask;
}

/*!
  Sets the keyboard modifier mask that this key event transition will check
  for.
*/
void QBasicKeyEventTransition::setModifierMask(Qt::KeyboardModifiers modifierMask)
{
    Q_D(QBasicKeyEventTransition);
    d->modifierMask = modifierMask;
}

/*!
  \reimp
*/
bool QBasicKeyEventTransition::eventTest(QEvent *event)
{
    Q_D(const QBasicKeyEventTransition);
    if (event->type() == d->eventType) {
        QKeyEvent *ke = static_cast<QKeyEvent*>(event);
        return (ke->key() == d->key)
            && ((ke->modifiers() & d->modifierMask) == d->modifierMask);
    }
    return false;
}

/*!
  \reimp
*/
void QBasicKeyEventTransition::onTransition(QEvent *)
{
}

QT_END_NAMESPACE

#endif //QT_NO_STATEMACHINE
