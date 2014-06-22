/*
    Copyright (C) 2009 Nokia Corporation and/or its subsidiary(-ies)

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/
// Functions and macros that really need to be in QTestLib

#if 0
#pragma qt_no_master_include
#endif

#include <QEventLoop>
#include <QSignalSpy>
#include <QTimer>

#if !defined(TESTS_SOURCE_DIR)
#define TESTS_SOURCE_DIR ""
#endif

/**
 * Starts an event loop that runs until the given signal is received.
 * Optionally the event loop
 * can return earlier on a timeout.
 *
 * \return \p true if the requested signal was received
 *         \p false on timeout
 */
static inline bool waitForSignal(QObject* obj, const char* signal, int timeout = 10000)
{
    QEventLoop loop;
    QObject::connect(obj, signal, &loop, SLOT(quit()));
    QTimer timer;
    QSignalSpy timeoutSpy(&timer, SIGNAL(timeout()));
    if (timeout > 0) {
        QObject::connect(&timer, SIGNAL(timeout()), &loop, SLOT(quit()));
        timer.setSingleShot(true);
        timer.start(timeout);
    }
    loop.exec();
    return timeoutSpy.isEmpty();
}

/**
 * Just like QSignalSpy but facilitates sync and async
 * signal emission. For example if you want to verify that
 * page->foo() emitted a signal, it could be that the
 * implementation decides to emit the signal asynchronously
 * - in which case we want to spin a local event loop until
 * emission - or that the call to foo() emits it right away.
 */
class SignalBarrier : private QSignalSpy
{
public:
    SignalBarrier(const QObject* obj, const char* aSignal)
        : QSignalSpy(obj, aSignal)
    { }

    bool ensureSignalEmitted()
    {
        bool result = count() > 0;
        if (!result)
            result = wait();
        clear();
        return result;
    }
};

#define W_QSKIP(a, b) QSKIP(a)
