/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the plugins of the Qt Toolkit.
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

#include <QtCore/qabstracteventdispatcher.h>
#include <QtCore/qhash.h>
#include <QtCore/qstack.h>
#include <QtGui/qwindowdefs.h>
#include <QtCore/private/qabstracteventdispatcher_p.h>
#include <QtCore/private/qtimerinfo_unix_p.h>
#include <QtPlatformSupport/private/qcfsocketnotifier_p.h>

#include <CoreFoundation/CoreFoundation.h>

QT_BEGIN_NAMESPACE

typedef struct _NSModalSession *NSModalSession;
typedef struct _QCocoaModalSessionInfo {
    QPointer<QWindow> window;
    NSModalSession session;
    void *nswindow;
} QCocoaModalSessionInfo;

class QCocoaEventDispatcherPrivate;
class QCocoaEventDispatcher : public QAbstractEventDispatcher
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QCocoaEventDispatcher)

public:
    QCocoaEventDispatcher(QAbstractEventDispatcherPrivate &priv, QObject *parent = 0);
    explicit QCocoaEventDispatcher(QObject *parent = 0);
    ~QCocoaEventDispatcher();

    bool processEvents(QEventLoop::ProcessEventsFlags flags);
    bool hasPendingEvents();

    void registerSocketNotifier(QSocketNotifier *notifier);
    void unregisterSocketNotifier(QSocketNotifier *notifier);

    void registerTimer(int timerId, int interval, Qt::TimerType timerType, QObject *object);
    bool unregisterTimer(int timerId);
    bool unregisterTimers(QObject *object);
    QList<TimerInfo> registeredTimers(QObject *object) const;

    int remainingTime(int timerId);

    void wakeUp();
    void interrupt();
    void flush();

    bool event(QEvent *);

    friend void qt_mac_maybeCancelWaitForMoreEventsForwarder(QAbstractEventDispatcher *eventDispatcher);
};

class QCocoaEventDispatcherPrivate : public QAbstractEventDispatcherPrivate
{
    Q_DECLARE_PUBLIC(QCocoaEventDispatcher)

public:
    QCocoaEventDispatcherPrivate();

    uint processEventsFlags;

    // timer handling
    QTimerInfoList timerInfoList;
    CFRunLoopTimerRef runLoopTimerRef;
    CFRunLoopSourceRef activateTimersSourceRef;
    void maybeStartCFRunLoopTimer();
    void maybeStopCFRunLoopTimer();
    static void runLoopTimerCallback(CFRunLoopTimerRef, void *info);
    static void activateTimersSourceCallback(void *info);

    // Set 'blockSendPostedEvents' to true if you _really_ need
    // to make sure that qt events are not posted while calling
    // low-level cocoa functions (like beginModalForWindow). And
    // use a QBoolBlocker to be safe:
    bool blockSendPostedEvents;
    // The following variables help organizing modal sessions:
    QStack<QCocoaModalSessionInfo> cocoaModalSessionStack;
    bool currentExecIsNSAppRun;
    bool nsAppRunCalledByQt;
    bool cleanupModalSessionsNeeded;
    uint processEventsCalled;
    NSModalSession currentModalSessionCached;
    NSModalSession currentModalSession();
    void updateChildrenWorksWhenModal();
    void temporarilyStopAllModalSessions();
    void beginModalSession(QWindow *widget);
    void endModalSession(QWindow *widget);
    void cleanupModalSessions();

    void cancelWaitForMoreEvents();
    void maybeCancelWaitForMoreEvents();
    void ensureNSAppInitialized();

    QCFSocketNotifier cfSocketNotifier;
    QList<void *> queuedUserInputEvents; // NSEvent *
    CFRunLoopSourceRef postedEventsSource;
    CFRunLoopObserverRef waitingObserver;
    CFRunLoopObserverRef firstTimeObserver;
    QAtomicInt serialNumber;
    int lastSerial;
    bool interrupt;

    static void postedEventsSourceCallback(void *info);
    static void waitingObserverCallback(CFRunLoopObserverRef observer,
                                        CFRunLoopActivity activity, void *info);
    static void firstLoopEntry(CFRunLoopObserverRef ref, CFRunLoopActivity activity, void *info);
    void processPostedEvents();
};

class QtCocoaInterruptDispatcher : public QObject
{
    static QtCocoaInterruptDispatcher *instance;
    bool cancelled;

    QtCocoaInterruptDispatcher();
    ~QtCocoaInterruptDispatcher();

    public:
    static void interruptLater();
    static void cancelInterruptLater();
};

QT_END_NAMESPACE

#endif // QEVENTDISPATCHER_MAC_P_H
