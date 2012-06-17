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

#ifndef QEVENTTRANSITION_H
#define QEVENTTRANSITION_H

#include <QtCore/qabstracttransition.h>
#include <QtCore/qcoreevent.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Core)

#ifndef QT_NO_STATEMACHINE

class QEventTransitionPrivate;
class Q_CORE_EXPORT QEventTransition : public QAbstractTransition
{
    Q_OBJECT
    Q_PROPERTY(QObject* eventSource READ eventSource WRITE setEventSource)
    Q_PROPERTY(QEvent::Type eventType READ eventType WRITE setEventType)
public:
    QEventTransition(QState *sourceState = 0);
    QEventTransition(QObject *object, QEvent::Type type, QState *sourceState = 0);
    ~QEventTransition();

    QObject *eventSource() const;
    void setEventSource(QObject *object);

    QEvent::Type eventType() const;
    void setEventType(QEvent::Type type);

protected:
    bool eventTest(QEvent *event);
    void onTransition(QEvent *event);

    bool event(QEvent *e);

protected:
    QEventTransition(QEventTransitionPrivate &dd, QState *parent);
    QEventTransition(QEventTransitionPrivate &dd, QObject *object,
                     QEvent::Type type, QState *parent);

private:
    Q_DISABLE_COPY(QEventTransition)
    Q_DECLARE_PRIVATE(QEventTransition)
};

#endif //QT_NO_STATEMACHINE

QT_END_NAMESPACE

QT_END_HEADER

#endif
