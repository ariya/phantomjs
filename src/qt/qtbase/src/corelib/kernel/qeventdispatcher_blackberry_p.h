/****************************************************************************
**
** Copyright (C) 2012 - 2013 BlackBerry Limited. All rights reserved.
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

#ifndef QEVENTDISPATCHER_BLACKBERRY_P_H
#define QEVENTDISPATCHER_BLACKBERRY_P_H

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

#include "private/qeventdispatcher_unix_p.h"

QT_BEGIN_NAMESPACE

class QEventDispatcherBlackberryPrivate;

class Q_CORE_EXPORT QEventDispatcherBlackberry : public QEventDispatcherUNIX
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QEventDispatcherBlackberry)

public:
    explicit QEventDispatcherBlackberry(QObject *parent = 0);
    ~QEventDispatcherBlackberry();

    void registerSocketNotifier(QSocketNotifier *notifier);
    void unregisterSocketNotifier(QSocketNotifier *notifier);

    void wakeUp();

protected:
    QEventDispatcherBlackberry(QEventDispatcherBlackberryPrivate &dd, QObject *parent = 0);

    int select(int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds,
               timespec *timeout);
    int ioEvents(int fd);
};

struct bpsIOHandlerData;

class Q_CORE_EXPORT QEventDispatcherBlackberryPrivate : public QEventDispatcherUNIXPrivate
{
    Q_DECLARE_PUBLIC(QEventDispatcherBlackberry)

public:
    QEventDispatcherBlackberryPrivate();
    ~QEventDispatcherBlackberryPrivate();

    int initThreadWakeUp();
    int processThreadWakeUp(int nsel);

    int bps_channel;
    int holding_channel;
    int loop_level;
    QScopedPointer<bpsIOHandlerData> ioData;
};

QT_END_NAMESPACE

#endif // QEVENTDISPATCHER_BLACKBERRY_P_H
