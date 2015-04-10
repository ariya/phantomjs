/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
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

#ifndef QSTATEMACHINE_P_H
#define QSTATEMACHINE_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include "private/qstate_p.h"

#include <QtCore/qcoreevent.h>
#include <QtCore/qhash.h>
#include <QtCore/qlist.h>
#include <QtCore/qmutex.h>
#include <QtCore/qpair.h>
#include <QtCore/qpointer.h>
#include <QtCore/qset.h>
#include <QtCore/qvector.h>
#include <private/qfreelist_p.h>

QT_BEGIN_NAMESPACE

class QEvent;
#ifndef QT_NO_STATEMACHINE_EVENTFILTER
class QEventTransition;
#endif
class QSignalEventGenerator;
class QSignalTransition;
class QAbstractState;
class QAbstractTransition;
class QFinalState;
class QHistoryState;
class QState;

#ifndef QT_NO_ANIMATION
class QAbstractAnimation;
#endif

class QStateMachine;
class Q_CORE_EXPORT QStateMachinePrivate : public QStatePrivate
{
    Q_DECLARE_PUBLIC(QStateMachine)
public:
    enum State {
        NotRunning,
        Starting,
        Running
    };
    enum EventProcessingMode {
        DirectProcessing,
        QueuedProcessing
    };
    enum StopProcessingReason {
        EventQueueEmpty,
        Finished,
        Stopped
    };

    QStateMachinePrivate();
    ~QStateMachinePrivate();

    static QStateMachinePrivate *get(QStateMachine *q);

    QState *findLCA(const QList<QAbstractState*> &states) const;

    static bool stateEntryLessThan(QAbstractState *s1, QAbstractState *s2);
    static bool stateExitLessThan(QAbstractState *s1, QAbstractState *s2);

    QAbstractState *findErrorState(QAbstractState *context);
    void setError(QStateMachine::Error error, QAbstractState *currentContext);

    // private slots
    void _q_start();
    void _q_process();
#ifndef QT_NO_ANIMATION
    void _q_animationFinished();
#endif
    void _q_startDelayedEventTimer(int id, int delay);
    void _q_killDelayedEventTimer(int id, int timerId);

    QState *rootState() const;

    void clearHistory();
    QAbstractTransition *createInitialTransition() const;

    void microstep(QEvent *event, const QList<QAbstractTransition*> &transitionList);
    bool isPreempted(const QAbstractState *s, const QSet<QAbstractTransition*> &transitions) const;
    QSet<QAbstractTransition*> selectTransitions(QEvent *event) const;
    void exitStates(QEvent *event, const QList<QAbstractState *> &statesToExit_sorted,
                    const QHash<QAbstractState*, QList<QPropertyAssignment> > &assignmentsForEnteredStates);
    QList<QAbstractState*> computeStatesToExit(const QList<QAbstractTransition*> &enabledTransitions);
    void executeTransitionContent(QEvent *event, const QList<QAbstractTransition*> &transitionList);
    void enterStates(QEvent *event, const QList<QAbstractState*> &exitedStates_sorted,
                     const QList<QAbstractState*> &statesToEnter_sorted,
                     const QSet<QAbstractState*> &statesForDefaultEntry,
                     QHash<QAbstractState *, QList<QPropertyAssignment> > &propertyAssignmentsForState
#ifndef QT_NO_ANIMATION
                     , const QList<QAbstractAnimation*> &selectedAnimations
#endif
                     );
    QList<QAbstractState*> computeStatesToEnter(const QList<QAbstractTransition*> &enabledTransitions,
                                                QSet<QAbstractState*> &statesForDefaultEntry);
    void addStatesToEnter(QAbstractState *s, QState *root,
                          QSet<QAbstractState*> &statesToEnter,
                          QSet<QAbstractState*> &statesForDefaultEntry);
    void addAncestorStatesToEnter(QAbstractState *s, QState *root,
                                  QSet<QAbstractState*> &statesToEnter,
                                  QSet<QAbstractState*> &statesForDefaultEntry);

    static QState *toStandardState(QAbstractState *state);
    static const QState *toStandardState(const QAbstractState *state);
    static QFinalState *toFinalState(QAbstractState *state);
    static QHistoryState *toHistoryState(QAbstractState *state);

    bool isInFinalState(QAbstractState *s) const;
    static bool isFinal(const QAbstractState *s);
    static bool isParallel(const QAbstractState *s);
    bool isCompound(const QAbstractState *s) const;
    bool isAtomic(const QAbstractState *s) const;
    static bool isDescendantOf(const QAbstractState *s, const QAbstractState *other);
    static QList<QState*> properAncestors(const QAbstractState *s, const QState *upperBound);

    void goToState(QAbstractState *targetState);

    void registerTransitions(QAbstractState *state);
    void maybeRegisterTransition(QAbstractTransition *transition);
    void registerTransition(QAbstractTransition *transition);
    void maybeRegisterSignalTransition(QSignalTransition *transition);
    void registerSignalTransition(QSignalTransition *transition);
    void unregisterSignalTransition(QSignalTransition *transition);
    void registerMultiThreadedSignalTransitions();
#ifndef QT_NO_STATEMACHINE_EVENTFILTER
    void maybeRegisterEventTransition(QEventTransition *transition);
    void registerEventTransition(QEventTransition *transition);
    void unregisterEventTransition(QEventTransition *transition);
    void handleFilteredEvent(QObject *watched, QEvent *event);
#endif
    void unregisterTransition(QAbstractTransition *transition);
    void unregisterAllTransitions();
    void handleTransitionSignal(QObject *sender, int signalIndex,
                                void **args);

    void postInternalEvent(QEvent *e);
    void postExternalEvent(QEvent *e);
    QEvent *dequeueInternalEvent();
    QEvent *dequeueExternalEvent();
    bool isInternalEventQueueEmpty();
    bool isExternalEventQueueEmpty();
    void processEvents(EventProcessingMode processingMode);
    void cancelAllDelayedEvents();

#ifndef QT_NO_PROPERTIES
    typedef QPair<QPointer<QObject>, QByteArray> RestorableId;
    QHash<QAbstractState*, QHash<RestorableId, QVariant> > registeredRestorablesForState;
    bool hasRestorable(QAbstractState *state, QObject *object, const QByteArray &propertyName) const;
    QVariant savedValueForRestorable(const QList<QAbstractState*> &exitedStates_sorted,
                                     QObject *object, const QByteArray &propertyName) const;
    void registerRestorable(QAbstractState *state, QObject *object, const QByteArray &propertyName,
                            const QVariant &value);
    void unregisterRestorables(const QList<QAbstractState*> &states, QObject *object,
                               const QByteArray &propertyName);
    QList<QPropertyAssignment> restorablesToPropertyList(const QHash<RestorableId, QVariant> &restorables) const;
    QHash<RestorableId, QVariant> computePendingRestorables(const QList<QAbstractState*> &statesToExit_sorted) const;
    QHash<QAbstractState*, QList<QPropertyAssignment> > computePropertyAssignments(
            const QList<QAbstractState*> &statesToEnter_sorted,
            QHash<RestorableId, QVariant> &pendingRestorables) const;
#endif

    State state;
    bool processing;
    bool processingScheduled;
    bool stop;
    StopProcessingReason stopProcessingReason;
    QSet<QAbstractState*> configuration;
    QList<QEvent*> internalEventQueue;
    QList<QEvent*> externalEventQueue;
    QMutex internalEventMutex;
    QMutex externalEventMutex;

    QStateMachine::Error error;
    QState::RestorePolicy globalRestorePolicy;

    QString errorString;
    QSet<QAbstractState *> pendingErrorStates;
    QSet<QAbstractState *> pendingErrorStatesForDefaultEntry;

#ifndef QT_NO_ANIMATION
    bool animated;

    QPair<QList<QAbstractAnimation*>, QList<QAbstractAnimation*> >
        initializeAnimation(QAbstractAnimation *abstractAnimation,
                            const QPropertyAssignment &prop);

    QHash<QAbstractState*, QList<QAbstractAnimation*> > animationsForState;
    QHash<QAbstractAnimation*, QPropertyAssignment> propertyForAnimation;
    QHash<QAbstractAnimation*, QAbstractState*> stateForAnimation;
    QSet<QAbstractAnimation*> resetAnimationEndValues;

    QList<QAbstractAnimation *> defaultAnimations;
    QMultiHash<QAbstractState *, QAbstractAnimation *> defaultAnimationsForSource;
    QMultiHash<QAbstractState *, QAbstractAnimation *> defaultAnimationsForTarget;

    QList<QAbstractAnimation *> selectAnimations(const QList<QAbstractTransition *> &transitionList) const;
    void terminateActiveAnimations(QAbstractState *state,
            const QHash<QAbstractState*, QList<QPropertyAssignment> > &assignmentsForEnteredStates);
    void initializeAnimations(QAbstractState *state, const QList<QAbstractAnimation*> &selectedAnimations,
                              const QList<QAbstractState *> &exitedStates_sorted,
                              QHash<QAbstractState *, QList<QPropertyAssignment> > &assignmentsForEnteredStates);
#endif // QT_NO_ANIMATION

    QSignalEventGenerator *signalEventGenerator;

    QHash<const QObject*, QVector<int> > connections;
    QMutex connectionsMutex;
#ifndef QT_NO_STATEMACHINE_EVENTFILTER
    QHash<QObject*, QHash<QEvent::Type, int> > qobjectEvents;
#endif
    QFreeList<void> delayedEventIdFreeList;
    struct DelayedEvent {
        QEvent *event;
        int timerId;
        DelayedEvent(QEvent *e, int tid)
            : event(e), timerId(tid) {}
        DelayedEvent()
            : event(0), timerId(0) {}
    };
    QHash<int, DelayedEvent> delayedEvents;
    QHash<int, int> timerIdToDelayedEventId;
    QMutex delayedEventsMutex;

    typedef QEvent* (*f_cloneEvent)(QEvent*);
    struct Handler {
        f_cloneEvent cloneEvent;
    };

    static const Handler *handler;
};

Q_CORE_EXPORT const QStateMachinePrivate::Handler *qcoreStateMachineHandler();

QT_END_NAMESPACE

#endif
