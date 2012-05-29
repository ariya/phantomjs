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

#ifndef QABSTRACTANIMATION_H
#define QABSTRACTANIMATION_H

#include <QtCore/qobject.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Core)

#ifndef QT_NO_ANIMATION

class QAnimationGroup;
class QSequentialAnimationGroup;
class QAnimationDriver;

class QAbstractAnimationPrivate;
class Q_CORE_EXPORT QAbstractAnimation : public QObject
{
    Q_OBJECT
    Q_ENUMS(State)
    Q_ENUMS(Direction)
    Q_PROPERTY(State state READ state NOTIFY stateChanged)
    Q_PROPERTY(int loopCount READ loopCount WRITE setLoopCount)
    Q_PROPERTY(int currentTime READ currentTime WRITE setCurrentTime)
    Q_PROPERTY(int currentLoop READ currentLoop NOTIFY currentLoopChanged)
    Q_PROPERTY(Direction direction READ direction WRITE setDirection NOTIFY directionChanged)
    Q_PROPERTY(int duration READ duration)

public:
    enum Direction {
        Forward,
        Backward
    };

    enum State {
        Stopped,
        Paused,
        Running
    };

    enum DeletionPolicy {
        KeepWhenStopped = 0,
        DeleteWhenStopped
    };

    QAbstractAnimation(QObject *parent = 0);
    virtual ~QAbstractAnimation();

    State state() const;

    QAnimationGroup *group() const;

    Direction direction() const;
    void setDirection(Direction direction);

    int currentTime() const;
    int currentLoopTime() const;

    int loopCount() const;
    void setLoopCount(int loopCount);
    int currentLoop() const;

    virtual int duration() const = 0;
    int totalDuration() const;

Q_SIGNALS:
    void finished();
    void stateChanged(QAbstractAnimation::State newState, QAbstractAnimation::State oldState);
    void currentLoopChanged(int currentLoop);
    void directionChanged(QAbstractAnimation::Direction);

public Q_SLOTS:
    void start(QAbstractAnimation::DeletionPolicy policy = KeepWhenStopped);
    void pause();
    void resume();
    void setPaused(bool);
    void stop();
    void setCurrentTime(int msecs);

protected:
    QAbstractAnimation(QAbstractAnimationPrivate &dd, QObject *parent = 0);
    bool event(QEvent *event);

    virtual void updateCurrentTime(int currentTime) = 0;
    virtual void updateState(QAbstractAnimation::State newState, QAbstractAnimation::State oldState);
    virtual void updateDirection(QAbstractAnimation::Direction direction);

private:
    Q_DISABLE_COPY(QAbstractAnimation)
    Q_DECLARE_PRIVATE(QAbstractAnimation)
};

class QAnimationDriverPrivate;
class Q_CORE_EXPORT QAnimationDriver : public QObject
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QAnimationDriver)

public:
    QAnimationDriver(QObject *parent = 0);

    void advance();
    void install();

    bool isRunning() const;

protected:
    virtual void started() {};
    virtual void stopped() {};

    QAnimationDriver(QAnimationDriverPrivate &dd, QObject *parent = 0);

private:
    friend class QUnifiedTimer;

    void start();
    void stop();
};




#endif //QT_NO_ANIMATION

QT_END_NAMESPACE

QT_END_HEADER

#endif // QABSTRACTANIMATION_H
