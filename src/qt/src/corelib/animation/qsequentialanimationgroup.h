/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef QSEQUENTIALANIMATIONGROUP_H
#define QSEQUENTIALANIMATIONGROUP_H

#include <QtCore/qanimationgroup.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Core)

#ifndef QT_NO_ANIMATION

class QPauseAnimation;
class QSequentialAnimationGroupPrivate;

class Q_CORE_EXPORT QSequentialAnimationGroup : public QAnimationGroup
{
    Q_OBJECT
    Q_PROPERTY(QAbstractAnimation* currentAnimation READ currentAnimation NOTIFY currentAnimationChanged)

public:
    QSequentialAnimationGroup(QObject *parent = 0);
    ~QSequentialAnimationGroup();

    QPauseAnimation *addPause(int msecs);
    QPauseAnimation *insertPause(int index, int msecs);

    QAbstractAnimation *currentAnimation() const;
    int duration() const;

Q_SIGNALS:
    void currentAnimationChanged(QAbstractAnimation *current);

protected:
    QSequentialAnimationGroup(QSequentialAnimationGroupPrivate &dd, QObject *parent);
    bool event(QEvent *event);

    void updateCurrentTime(int);
    void updateState(QAbstractAnimation::State newState, QAbstractAnimation::State oldState);
    void updateDirection(QAbstractAnimation::Direction direction);

private:
    Q_DISABLE_COPY(QSequentialAnimationGroup)
    Q_DECLARE_PRIVATE(QSequentialAnimationGroup)
    Q_PRIVATE_SLOT(d_func(), void _q_uncontrolledAnimationFinished())
};

#endif //QT_NO_ANIMATION

QT_END_NAMESPACE

QT_END_HEADER

#endif //QSEQUENTIALANIMATIONGROUP_H
