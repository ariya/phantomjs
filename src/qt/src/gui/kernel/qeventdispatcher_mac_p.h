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

/****************************************************************************
**
** Copyright (c) 2007-2008, Apple, Inc.
**
** All rights reserved.
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are met:
**
**   * Redistributions of source code must retain the above copyright notice,
**     this list of conditions and the following disclaimer.
**
**   * Redistributions in binary form must reproduce the above copyright notice,
**     this list of conditions and the following disclaimer in the documentation
**     and/or other materials provided with the distribution.
**
**   * Neither the name of Apple, Inc. nor the names of its contributors
**     may be used to endorse or promote products derived from this software
**     without specific prior written permission.
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
** CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
** EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
** PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
** PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
** LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
** NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
** SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**
****************************************************************************/

#ifndef QEVENTDISPATCHER_MAC_P_H
#define QEVENTDISPATCHER_MAC_P_H

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

#include <QtGui/qwindowdefs.h>
#include <QtCore/qhash.h>
#include <QtCore/qstack.h>
#include "private/qabstracteventdispatcher_p.h"
#include "private/qt_mac_p.h"

QT_BEGIN_NAMESPACE

#ifdef QT_MAC_USE_COCOA
typedef struct _NSModalSession *NSModalSession;
typedef struct _QCocoaModalSessionInfo {
    QPointer<QWidget> widget;
    NSModalSession session;
    void *nswindow;
} QCocoaModalSessionInfo;
#endif

class QEventDispatcherMacPrivate;

class QEventDispatcherMac : public QAbstractEventDispatcher
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QEventDispatcherMac)

public:
    explicit QEventDispatcherMac(QObject *parent = 0);
    ~QEventDispatcherMac();

    bool processEvents(QEventLoop::ProcessEventsFlags flags);
    bool hasPendingEvents();

    void registerSocketNotifier(QSocketNotifier *notifier);
    void unregisterSocketNotifier(QSocketNotifier *notifier);

    void registerTimer(int timerId, int interval, QObject *object);
    bool unregisterTimer(int timerId);
    bool unregisterTimers(QObject *object);
    QList<TimerInfo> registeredTimers(QObject *object) const;

    void wakeUp();
    void flush();
    void interrupt();

private:
    friend void qt_mac_select_timer_callbk(__EventLoopTimer*, void*);
    friend class QApplicationPrivate;
};

struct MacTimerInfo {
    int id;
    int interval;
    QObject *obj;
    bool pending;
    CFRunLoopTimerRef runLoopTimer;
    bool operator==(const MacTimerInfo &other)
    {
        return (id == other.id);
    }
};
typedef QHash<int, MacTimerInfo *> MacTimerHash;

struct MacSocketInfo {
    MacSocketInfo() : socket(0), runloop(0), readNotifier(0), writeNotifier(0) {}
    CFSocketRef socket;
    CFRunLoopSourceRef runloop;
    QObject *readNotifier;
    QObject *writeNotifier;
};
typedef QHash<int, MacSocketInfo *> MacSocketHash;

class QEventDispatcherMacPrivate : public QAbstractEventDispatcherPrivate
{
    Q_DECLARE_PUBLIC(QEventDispatcherMac)

public:
    QEventDispatcherMacPrivate();

    static MacTimerHash macTimerHash;
    // Set 'blockSendPostedEvents' to true if you _really_ need
    // to make sure that qt events are not posted while calling
    // low-level cocoa functions (like beginModalForWindow). And
    // use a QBoolBlocker to be safe:
    static bool blockSendPostedEvents;
#ifdef QT_MAC_USE_COCOA
    // The following variables help organizing modal sessions:
    static QStack<QCocoaModalSessionInfo> cocoaModalSessionStack;
    static bool currentExecIsNSAppRun;
    static bool nsAppRunCalledByQt;
    static bool cleanupModalSessionsNeeded;
    static NSModalSession currentModalSessionCached;
    static NSModalSession currentModalSession();
    static void updateChildrenWorksWhenModal();
    static void temporarilyStopAllModalSessions();
    static void beginModalSession(QWidget *widget);
    static void endModalSession(QWidget *widget);
    static void cancelWaitForMoreEvents();
    static void cleanupModalSessions();
    static void ensureNSAppInitialized();
#endif

    MacSocketHash macSockets;
    QList<void *> queuedUserInputEvents; // List of EventRef in Carbon, and NSEvent * in Cocoa
    CFRunLoopSourceRef postedEventsSource;
    CFRunLoopObserverRef waitingObserver;
    CFRunLoopObserverRef firstTimeObserver;
    QAtomicInt serialNumber;
    int lastSerial;
    static bool interrupt;
private:
    static Boolean postedEventSourceEqualCallback(const void *info1, const void *info2);
    static void postedEventsSourcePerformCallback(void *info);
    static void activateTimer(CFRunLoopTimerRef, void *info);
    static void waitingObserverCallback(CFRunLoopObserverRef observer,
                                        CFRunLoopActivity activity, void *info);
    static void firstLoopEntry(CFRunLoopObserverRef ref, CFRunLoopActivity activity, void *info);
};

#ifdef QT_MAC_USE_COCOA
class QtMacInterruptDispatcherHelp : public QObject
{
    static QtMacInterruptDispatcherHelp *instance;
    bool cancelled;

    QtMacInterruptDispatcherHelp();
    ~QtMacInterruptDispatcherHelp();

    public:
    static void interruptLater();
    static void cancelInterruptLater();
};
#endif

QT_END_NAMESPACE

#endif // QEVENTDISPATCHER_MAC_P_H
