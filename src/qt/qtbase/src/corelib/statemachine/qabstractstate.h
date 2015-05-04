/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia. For licensing terms and
** conditions see http://qt.digia.com/licensing. For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights. These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QABSTRACTSTATE_H
#define QABSTRACTSTATE_H

#include <QtCore/qobject.h>

QT_BEGIN_NAMESPACE


#ifndef QT_NO_STATEMACHINE

class QState;
class QStateMachine;

class QAbstractStatePrivate;
class Q_CORE_EXPORT QAbstractState : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool active READ active NOTIFY activeChanged)
public:
    ~QAbstractState();

    QState *parentState() const;
    QStateMachine *machine() const;

    bool active() const;

Q_SIGNALS:
    void entered(
#if !defined(Q_QDOC)
      QPrivateSignal
#endif
    );
    void exited(
#if !defined(Q_QDOC)
      QPrivateSignal
#endif
    );
    void activeChanged(bool active);

protected:
    QAbstractState(QState *parent = 0);

    virtual void onEntry(QEvent *event) = 0;
    virtual void onExit(QEvent *event) = 0;

    bool event(QEvent *e);

protected:
    QAbstractState(QAbstractStatePrivate &dd, QState *parent);

private:
    Q_DISABLE_COPY(QAbstractState)
    Q_DECLARE_PRIVATE(QAbstractState)
};

#endif //QT_NO_STATEMACHINE

QT_END_NAMESPACE

#endif
