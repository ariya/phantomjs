/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
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

#ifndef QABSTRACTSTATE_H
#define QABSTRACTSTATE_H

#include <QtCore/qobject.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Core)

#ifndef QT_NO_STATEMACHINE

class QState;
class QStateMachine;

class QAbstractStatePrivate;
class Q_CORE_EXPORT QAbstractState : public QObject
{
    Q_OBJECT
public:
    ~QAbstractState();

    QState *parentState() const;
    QStateMachine *machine() const;

Q_SIGNALS:
#if !defined(Q_MOC_RUN) && !defined(qdoc)
private: // can only be emitted by QAbstractState
#endif
    void entered();
    void exited();

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

QT_END_HEADER

#endif
