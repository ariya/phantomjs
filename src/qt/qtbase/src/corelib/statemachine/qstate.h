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

#ifndef QSTATE_H
#define QSTATE_H

#include <QtCore/qabstractstate.h>

#include <QtCore/qlist.h>

QT_BEGIN_NAMESPACE


#ifndef QT_NO_STATEMACHINE

class QAbstractTransition;
class QSignalTransition;

class QStatePrivate;
class Q_CORE_EXPORT QState : public QAbstractState
{
    Q_OBJECT
    Q_PROPERTY(QAbstractState* initialState READ initialState WRITE setInitialState NOTIFY initialStateChanged)
    Q_PROPERTY(QAbstractState* errorState READ errorState WRITE setErrorState NOTIFY errorStateChanged)
    Q_PROPERTY(ChildMode childMode READ childMode WRITE setChildMode NOTIFY childModeChanged)
    Q_ENUMS(ChildMode RestorePolicy)
public:
    enum ChildMode {
        ExclusiveStates,
        ParallelStates
    };

    enum RestorePolicy {
        DontRestoreProperties,
        RestoreProperties
    };

    QState(QState *parent = 0);
    QState(ChildMode childMode, QState *parent = 0);
    ~QState();

    QAbstractState *errorState() const;
    void setErrorState(QAbstractState *state);

    void addTransition(QAbstractTransition *transition);
    QSignalTransition *addTransition(const QObject *sender, const char *signal, QAbstractState *target);
    QAbstractTransition *addTransition(QAbstractState *target);
    void removeTransition(QAbstractTransition *transition);
    QList<QAbstractTransition*> transitions() const;

    QAbstractState *initialState() const;
    void setInitialState(QAbstractState *state);

    ChildMode childMode() const;
    void setChildMode(ChildMode mode);

#ifndef QT_NO_PROPERTIES
    void assignProperty(QObject *object, const char *name,
                        const QVariant &value);
#endif

Q_SIGNALS:
    void finished(
#if !defined(Q_QDOC)
      QPrivateSignal
#endif
    );
    void propertiesAssigned(
#if !defined(Q_QDOC)
      QPrivateSignal
#endif
    );
    void childModeChanged(
#if !defined(Q_QDOC)
      QPrivateSignal
#endif
    );
    void initialStateChanged(
#if !defined(Q_QDOC)
      QPrivateSignal
#endif
    );
    void errorStateChanged(
#if !defined(Q_QDOC)
      QPrivateSignal
#endif
    );

protected:
    void onEntry(QEvent *event);
    void onExit(QEvent *event);

    bool event(QEvent *e);

protected:
    QState(QStatePrivate &dd, QState *parent);

private:
    Q_DISABLE_COPY(QState)
    Q_DECLARE_PRIVATE(QState)
};

#endif //QT_NO_STATEMACHINE

QT_END_NAMESPACE

#endif
