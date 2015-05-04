/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtGui module of the Qt Toolkit.
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

#ifndef QCFSOCKETNOTIFIER_P_H
#define QCFSOCKETNOTIFIER_P_H

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

#include <CoreFoundation/CoreFoundation.h>

QT_BEGIN_NAMESPACE

struct MacSocketInfo {
    MacSocketInfo() : socket(0), runloop(0), readNotifier(0), writeNotifier(0) {}
    CFSocketRef socket;
    CFRunLoopSourceRef runloop;
    QObject *readNotifier;
    QObject *writeNotifier;
};
typedef QHash<int, MacSocketInfo *> MacSocketHash;

typedef void (*MaybeCancelWaitForMoreEventsFn)(QAbstractEventDispatcher *hostEventDispacher);

// The CoreFoundationSocketNotifier class implements socket notifiers support using
// CFSocket for event dispatchers running on top of the Core Foundation run loop system.
// (currently Mac and iOS)
//
// The principal functions are registerSocketNotifier() and unregisterSocketNotifier().
//
// setHostEventDispatcher() should be called at startup.
// removeSocketNotifiers() should be called at shutdown.
//
class QCFSocketNotifier
{
public:
    QCFSocketNotifier();
    ~QCFSocketNotifier();
    void setHostEventDispatcher(QAbstractEventDispatcher *hostEventDispacher);
    void setMaybeCancelWaitForMoreEventsCallback(MaybeCancelWaitForMoreEventsFn callBack);
    void registerSocketNotifier(QSocketNotifier *notifier);
    void unregisterSocketNotifier(QSocketNotifier *notifier);
    void removeSocketNotifiers();

    MacSocketHash macSockets;
    QAbstractEventDispatcher *eventDispatcher;
    MaybeCancelWaitForMoreEventsFn maybeCancelWaitForMoreEvents;
};

QT_END_NAMESPACE

#endif
