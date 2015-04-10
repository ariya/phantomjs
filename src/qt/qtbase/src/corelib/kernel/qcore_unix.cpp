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

#include "qcore_unix_p.h"
#include "qelapsedtimer.h"

#ifdef Q_OS_NACL
#elif !defined (Q_OS_VXWORKS)
# if !defined(Q_OS_HPUX) || defined(__ia64)
#  include <sys/select.h>
# endif
#  include <sys/time.h>
#else
#  include <selectLib.h>
#endif

#include <stdlib.h>

#ifdef Q_OS_MAC
#include <mach/mach_time.h>
#endif

#ifdef Q_OS_BLACKBERRY
#include <qsocketnotifier.h>
#endif // Q_OS_BLACKBERRY

QT_BEGIN_NAMESPACE

static inline bool time_update(struct timespec *tv, const struct timespec &start,
                               const struct timespec &timeout)
{
    // clock source is (hopefully) monotonic, so we can recalculate how much timeout is left;
    // if it isn't monotonic, we'll simply hope that it hasn't jumped, because we have no alternative
    struct timespec now = qt_gettime();
    *tv = timeout + start - now;
    return tv->tv_sec >= 0;
}

int qt_safe_select(int nfds, fd_set *fdread, fd_set *fdwrite, fd_set *fdexcept,
                   const struct timespec *orig_timeout)
{
    if (!orig_timeout) {
        // no timeout -> block forever
        int ret;
        EINTR_LOOP(ret, select(nfds, fdread, fdwrite, fdexcept, 0));
        return ret;
    }

    timespec start = qt_gettime();
    timespec timeout = *orig_timeout;

    // loop and recalculate the timeout as needed
    int ret;
    forever {
#ifndef Q_OS_QNX
        ret = ::pselect(nfds, fdread, fdwrite, fdexcept, &timeout, 0);
#else
        timeval timeoutVal;
        timeoutVal.tv_sec = timeout.tv_sec;
        timeoutVal.tv_usec = timeout.tv_nsec / 1000;
        ret = ::select(nfds, fdread, fdwrite, fdexcept, &timeoutVal);
#endif
        if (ret != -1 || errno != EINTR)
            return ret;

        // recalculate the timeout
        if (!time_update(&timeout, start, *orig_timeout)) {
            // timeout during update
            // or clock reset, fake timeout error
            return 0;
        }
    }
}

int qt_select_msecs(int nfds, fd_set *fdread, fd_set *fdwrite, int timeout)
{
    if (timeout < 0)
        return qt_safe_select(nfds, fdread, fdwrite, 0, 0);

    struct timespec tv;
    tv.tv_sec = timeout / 1000;
    tv.tv_nsec = (timeout % 1000) * 1000 * 1000;
    return qt_safe_select(nfds, fdread, fdwrite, 0, &tv);
}

#ifdef Q_OS_BLACKBERRY
// The BlackBerry event dispatcher uses bps_get_event. Unfortunately, already registered
// socket notifiers are disabled by a call to select. This is to rearm the standard streams.
int bb_select(QList<QSocketNotifier *> socketNotifiers, int nfds, fd_set *fdread, fd_set *fdwrite,
              int timeout)
{
    QList<bool> socketNotifiersEnabled;
    socketNotifiersEnabled.reserve(socketNotifiers.count());
    for (int a = 0; a < socketNotifiers.count(); ++a) {
        if (socketNotifiers.at(a) && socketNotifiers.at(a)->isEnabled()) {
            socketNotifiersEnabled.append(true);
            socketNotifiers.at(a)->setEnabled(false);
        } else {
            socketNotifiersEnabled.append(false);
        }
    }

    const int ret = qt_select_msecs(nfds, fdread, fdwrite, timeout);

    for (int a = 0; a < socketNotifiers.count(); ++a) {
        if (socketNotifiersEnabled.at(a) == true)
            socketNotifiers.at(a)->setEnabled(true);
    }

    return ret;
}
#endif // Q_OS_BLACKBERRY

QT_END_NAMESPACE
