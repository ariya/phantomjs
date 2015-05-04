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

#ifndef QTIMER_H
#define QTIMER_H

#include <QtCore/qglobal.h>

#ifndef QT_NO_QOBJECT

#include <QtCore/qbasictimer.h> // conceptual inheritance
#include <QtCore/qobject.h>

QT_BEGIN_NAMESPACE


class Q_CORE_EXPORT QTimer : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool singleShot READ isSingleShot WRITE setSingleShot)
    Q_PROPERTY(int interval READ interval WRITE setInterval)
    Q_PROPERTY(int remainingTime READ remainingTime)
    Q_PROPERTY(Qt::TimerType timerType READ timerType WRITE setTimerType)
    Q_PROPERTY(bool active READ isActive)
public:
    explicit QTimer(QObject *parent = 0);
    ~QTimer();

    inline bool isActive() const { return id >= 0; }
    int timerId() const { return id; }

    void setInterval(int msec);
    int interval() const { return inter; }

    int remainingTime() const;

    void setTimerType(Qt::TimerType atype) { this->type = atype; }
    Qt::TimerType timerType() const { return Qt::TimerType(type); }

    inline void setSingleShot(bool singleShot);
    inline bool isSingleShot() const { return single; }

    static void singleShot(int msec, const QObject *receiver, const char *member);
    static void singleShot(int msec, Qt::TimerType timerType, const QObject *receiver, const char *member);

#ifdef Q_QDOC
    static void singleShot(int msec, const QObject *receiver, PointerToMemberFunction method);
    static void singleShot(int msec, Qt::TimerType timerType, const QObject *receiver, PointerToMemberFunction method);
    static void singleShot(int msec, Functor functor);
    static void singleShot(int msec, Qt::TimerType timerType, Functor functor);
    static void singleShot(int msec, const QObject *context, Functor functor);
    static void singleShot(int msec, Qt::TimerType timerType, const QObject *context, Functor functor);
#else
    // singleShot to a QObject slot
    template <typename Func1>
    static inline void singleShot(int msec, const typename QtPrivate::FunctionPointer<Func1>::Object *receiver, Func1 slot)
    {
        singleShot(msec, msec >= 2000 ? Qt::CoarseTimer : Qt::PreciseTimer, receiver, slot);
    }
    template <typename Func1>
    static inline void singleShot(int msec, Qt::TimerType timerType, const typename QtPrivate::FunctionPointer<Func1>::Object *receiver,
                                  Func1 slot)
    {
        typedef QtPrivate::FunctionPointer<Func1> SlotType;

        //compilation error if the slot has arguments.
        Q_STATIC_ASSERT_X(int(SlotType::ArgumentCount) == 0,
                          "The slot must not have any arguments.");

        singleShotImpl(msec, timerType, receiver,
                       new QtPrivate::QSlotObject<Func1, typename SlotType::Arguments, void>(slot));
    }
    // singleShot to a functor or function pointer (without context)
    template <typename Func1>
    static inline void singleShot(int msec, Func1 slot)
    {
        singleShot(msec, msec >= 2000 ? Qt::CoarseTimer : Qt::PreciseTimer, Q_NULLPTR, slot);
    }
    template <typename Func1>
    static inline void singleShot(int msec, Qt::TimerType timerType, Func1 slot)
    {
        singleShot(msec, timerType, Q_NULLPTR, slot);
    }
    // singleShot to a functor or function pointer (with context)
    template <typename Func1>
    static inline typename QtPrivate::QEnableIf<!QtPrivate::FunctionPointer<Func1>::IsPointerToMemberFunction &&
                                                !QtPrivate::is_same<const char*, Func1>::value, void>::Type
            singleShot(int msec, QObject *context, Func1 slot)
    {
        singleShot(msec, msec >= 2000 ? Qt::CoarseTimer : Qt::PreciseTimer, context, slot);
    }
    template <typename Func1>
    static inline typename QtPrivate::QEnableIf<!QtPrivate::FunctionPointer<Func1>::IsPointerToMemberFunction &&
                                                !QtPrivate::is_same<const char*, Func1>::value, void>::Type
            singleShot(int msec, Qt::TimerType timerType, QObject *context, Func1 slot)
    {
        //compilation error if the slot has arguments.
        typedef QtPrivate::FunctionPointer<Func1> SlotType;
        Q_STATIC_ASSERT_X(int(SlotType::ArgumentCount) <= 0,  "The slot must not have any arguments.");

        singleShotImpl(msec, timerType, context,
                       new QtPrivate::QFunctorSlotObject<Func1, 0,
                            typename QtPrivate::List_Left<void, 0>::Value, void>(slot));
    }
#endif

public Q_SLOTS:
    void start(int msec);

    void start();
    void stop();

Q_SIGNALS:
    void timeout(
#if !defined(Q_QDOC)
      QPrivateSignal
#endif
    );

protected:
    void timerEvent(QTimerEvent *);

private:
    Q_DISABLE_COPY(QTimer)

    inline int startTimer(int){ return -1;}
    inline void killTimer(int){}

    static void singleShotImpl(int msec, Qt::TimerType timerType,
                               const QObject *receiver, QtPrivate::QSlotObjectBase *slotObj);

    int id, inter, del;
    uint single : 1;
    uint nulltimer : 1;
    uint type : 2;
    // reserved : 28
};

inline void QTimer::setSingleShot(bool asingleShot) { single = asingleShot; }

QT_END_NAMESPACE

#endif // QT_NO_QOBJECT

#endif // QTIMER_H
