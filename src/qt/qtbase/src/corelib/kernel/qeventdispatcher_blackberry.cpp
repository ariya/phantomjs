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

#include "qeventdispatcher_blackberry_p.h"
#include "qsocketnotifier.h"
#include "qdebug.h"
#include "qelapsedtimer.h"
#include "private/qthread_p.h"

#include <bps/bps.h>
#include <bps/event.h>

//#define QEVENTDISPATCHERBLACKBERRY_DEBUG

#ifdef QEVENTDISPATCHERBLACKBERRY_DEBUG
#include <QThread>
#define qEventDispatcherDebug qDebug() << QThread::currentThread()
#else
#define qEventDispatcherDebug QT_NO_QDEBUG_MACRO()
#endif

class BpsChannelScopeSwitcher
{
public:
    BpsChannelScopeSwitcher(int scopeChannel) : innerChannel(scopeChannel)
    {
        outerChannel = bps_channel_get_active();
        if (outerChannel != innerChannel)
            bps_channel_set_active(innerChannel);
    }

    ~BpsChannelScopeSwitcher()
    {
        if (outerChannel != innerChannel)
            bps_channel_set_active(outerChannel);
    }

private:
    int innerChannel;
    int outerChannel;
};

class BBScopedLoopLevelCounter
{
    QEventDispatcherBlackberryPrivate *d;

public:
    inline BBScopedLoopLevelCounter(QEventDispatcherBlackberryPrivate *p)
        : d(p)
    { ++d->loop_level; }

    inline ~BBScopedLoopLevelCounter()
    { --d->loop_level; }
};

struct bpsIOHandlerData
{
    bpsIOHandlerData()
        : count(0), readfds(0), writefds(0), exceptfds(0)
    {
    }

    int count;
    fd_set *readfds;
    fd_set *writefds;
    fd_set *exceptfds;
};

static int bpsUnblockDomain = -1;

static int bpsIOHandler(int fd, int io_events, void *data)
{
    qEventDispatcherDebug << Q_FUNC_INFO;
    // decode callback payload
    bpsIOHandlerData *ioData = static_cast<bpsIOHandlerData*>(data);

    // check if first file is ready
    bool firstReady = (ioData->count == 0);

    // update ready state of file
    if (io_events & BPS_IO_INPUT) {
        qEventDispatcherDebug << fd << "ready for Read";
        FD_SET(fd, ioData->readfds);
        ioData->count++;
    }

    if (io_events & BPS_IO_OUTPUT) {
        qEventDispatcherDebug << fd << "ready for Write";
        FD_SET(fd, ioData->writefds);
        ioData->count++;
    }

    if (io_events & BPS_IO_EXCEPT) {
        qEventDispatcherDebug << fd << "ready for Exception";
        FD_SET(fd, ioData->exceptfds);
        ioData->count++;
    }

    // force bps_get_event() to return immediately by posting an event to ourselves;
    // but this only needs to happen once if multiple files become ready at the same time
    if (firstReady) {
        qEventDispatcherDebug << "Sending bpsIOReadyDomain event";
        // create unblock event
        bps_event_t *event;
        int result = bps_event_create(&event, bpsUnblockDomain, 0, NULL, NULL);
        if (Q_UNLIKELY(result != BPS_SUCCESS)) {
            qWarning("QEventDispatcherBlackberry: bps_event_create failed");
            return BPS_FAILURE;
        }

        // post unblock event to our thread; in this callback the bps channel is
        // guaranteed to be the same that was active when bps_add_fd was called
        result = bps_push_event(event);
        if (Q_UNLIKELY(result != BPS_SUCCESS)) {
            qWarning("QEventDispatcherBlackberry: bps_push_event failed");
            bps_event_destroy(event);
            return BPS_FAILURE;
        }
    }

    return BPS_SUCCESS;
}

QEventDispatcherBlackberryPrivate::QEventDispatcherBlackberryPrivate()
    : loop_level(0)
    , ioData(new bpsIOHandlerData)
{
    // prepare to use BPS
    int result = bps_initialize();
    if (Q_UNLIKELY(result != BPS_SUCCESS))
        qFatal("QEventDispatcherBlackberry: bps_initialize failed");

    bps_channel = bps_channel_get_active();

    if (bps_channel_create(&holding_channel, 0) != BPS_SUCCESS) {
        qWarning("QEventDispatcherBlackberry: bps_channel_create failed");
        holding_channel = -1;
    }

    // get domain for IO ready and wake up events - ignoring race condition here for now
    if (bpsUnblockDomain == -1) {
        bpsUnblockDomain = bps_register_domain();
        if (Q_UNLIKELY(bpsUnblockDomain == -1))
            qWarning("QEventDispatcherBlackberry: bps_register_domain failed");
    }
}

QEventDispatcherBlackberryPrivate::~QEventDispatcherBlackberryPrivate()
{
    if ((holding_channel != -1) &&
        (bps_channel_destroy(holding_channel) != BPS_SUCCESS)) {
        qWarning("QEventDispatcherBlackberry: bps_channel_destroy failed");
    }

    // we're done using BPS
    bps_shutdown();
}

int QEventDispatcherBlackberryPrivate::initThreadWakeUp()
{
    return -1;  // no fd's used
}

int QEventDispatcherBlackberryPrivate::processThreadWakeUp(int nsel)
{
    Q_UNUSED(nsel);
    return wakeUps.fetchAndStoreRelaxed(0);
}

/////////////////////////////////////////////////////////////////////////////

QEventDispatcherBlackberry::QEventDispatcherBlackberry(QObject *parent)
    : QEventDispatcherUNIX(*new QEventDispatcherBlackberryPrivate, parent)
{
}

QEventDispatcherBlackberry::QEventDispatcherBlackberry(QEventDispatcherBlackberryPrivate &dd, QObject *parent)
    : QEventDispatcherUNIX(dd, parent)
{
}

QEventDispatcherBlackberry::~QEventDispatcherBlackberry()
{
}

void QEventDispatcherBlackberry::registerSocketNotifier(QSocketNotifier *notifier)
{
    Q_ASSERT(notifier);
    Q_D(QEventDispatcherBlackberry);

    int sockfd = notifier->socket();
    int type = notifier->type();

    qEventDispatcherDebug << Q_FUNC_INFO << "fd =" << sockfd;

    if (Q_UNLIKELY(sockfd >= FD_SETSIZE)) {
        qWarning() << "QEventDispatcherBlackberry: cannot register QSocketNotifier (fd too high)"
                   << sockfd;
        return;
    }

    // Register the fd with bps
    BpsChannelScopeSwitcher channelSwitcher(d->bps_channel);
    int io_events = ioEvents(sockfd);
    if (io_events)
        bps_remove_fd(sockfd);

    switch (type) {
    case QSocketNotifier::Read:
        qEventDispatcherDebug << "Registering" << sockfd << "for Reads";
        io_events |= BPS_IO_INPUT;
        break;
    case QSocketNotifier::Write:
        qEventDispatcherDebug << "Registering" << sockfd << "for Writes";
        io_events |= BPS_IO_OUTPUT;
        break;
    case QSocketNotifier::Exception:
    default:
        qEventDispatcherDebug << "Registering" << sockfd << "for Exceptions";
        io_events |= BPS_IO_EXCEPT;
        break;
    }

    const int result = bps_add_fd(sockfd, io_events, &bpsIOHandler, d->ioData.data());
    if (Q_UNLIKELY(result != BPS_SUCCESS))
        qWarning() << "QEventDispatcherBlackberry: bps_add_fd failed";

    // Call the base Unix implementation. Needed to allow select() to be called correctly
    QEventDispatcherUNIX::registerSocketNotifier(notifier);
}

void QEventDispatcherBlackberry::unregisterSocketNotifier(QSocketNotifier *notifier)
{
    Q_D(QEventDispatcherBlackberry);

    int sockfd = notifier->socket();

    qEventDispatcherDebug << Q_FUNC_INFO << "fd =" << sockfd;

    if (Q_UNLIKELY(sockfd >= FD_SETSIZE)) {
        qWarning() << "QEventDispatcherBlackberry: cannot unregister QSocketNotifier" << sockfd;
        return;
    }

    // Allow the base Unix implementation to unregister the fd too (before call to ioEvents()!)
    QEventDispatcherUNIX::unregisterSocketNotifier(notifier);

    // Unregister the fd with bps
    BpsChannelScopeSwitcher channelSwitcher(d->bps_channel);
    int result = bps_remove_fd(sockfd);
    if (Q_UNLIKELY(result != BPS_SUCCESS))
        qWarning() << "QEventDispatcherBlackberry: bps_remove_fd failed" << sockfd;

    const int io_events = ioEvents(sockfd);
    // if other socket notifier is watching sockfd, readd it
    if (io_events) {
        result = bps_add_fd(sockfd, io_events, &bpsIOHandler, d->ioData.data());
        if (Q_UNLIKELY(result != BPS_SUCCESS))
            qWarning("QEventDispatcherBlackberry: bps_add_fd error");
    }
}

static inline int timespecToMillisecs(const timespec &tv)
{
    return (tv.tv_sec * 1000) + (tv.tv_nsec / 1000000);
}

static inline void destroyHeldBpsEvent(int holding_channel)
{
    // Switch to the holding channel and use bps_get_event() to trigger its destruction.  We
    // don't care about the return value from this call to bps_get_event().
    BpsChannelScopeSwitcher holdingChannelSwitcher(holding_channel);
    bps_event_t *held_event = 0;
    (void)bps_get_event(&held_event, 0);
 }

int QEventDispatcherBlackberry::select(int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds,
                                       timespec *timeout)
{
    Q_UNUSED(nfds);
    Q_D(QEventDispatcherBlackberry);
    const BBScopedLoopLevelCounter bbLoopCounter(d);

    BpsChannelScopeSwitcher channelSwitcher(d->bps_channel);

    // prepare file sets for bps callback
    d->ioData->count = 0;
    d->ioData->readfds = readfds;
    d->ioData->writefds = writefds;
    d->ioData->exceptfds = exceptfds;

    // reset all file sets
    if (readfds)
        FD_ZERO(readfds);

    if (writefds)
        FD_ZERO(writefds);

    if (exceptfds)
        FD_ZERO(exceptfds);

    bps_event_t *event = 0;
    unsigned int eventCount = 0;

    // If an event handler called through filterEvent() starts a nested event loop by creating a
    // new QEventLoop, we will recursively enter this function again.  However, each time
    // bps_get_event() is called, it destroys the last event it handed out before returning the
    // next event.  We don't want it to destroy the event that triggered the nested event loop,
    // since there may still be more handlers that need to get that event, once the nested event
    // loop is done and control returns to the outer event loop.
    //
    // So we move an event to a holding channel, which takes ownership of the event.  Putting
    // the event on our own channel allows us to manage when it is destroyed, keeping it alive
    // until we know we are done with it.  Each recursive call of this function needs to have
    // it's own holding channel, since a channel is a queue, not a stack.
    //
    // However, a recursive call into this function happens very rarely compared to the many
    // times this function is called.  We don't want to create a holding channel for each time
    // this function is called, only when it is called recursively.  Thus we have the instance
    // variable d->holding_channel to use in the common case.  We keep track of recursive calls
    // with d->loop_level.  If we are in a recursive call, then we create a new holding channel
    // for this run.
    int holding_channel = d->holding_channel;
    if ((d->loop_level > 1) &&
        Q_UNLIKELY(bps_channel_create(&holding_channel, 0) != BPS_SUCCESS)) {
        qWarning("QEventDispatcherBlackberry: bps_channel_create failed");
        holding_channel = -1;
    }

    // Convert timeout to milliseconds
    int timeoutTotal = -1;
    if (timeout)
        timeoutTotal = timespecToMillisecs(*timeout);
    int timeoutLeft = timeoutTotal;
    timespec startTime = qt_gettime();

    // This loop exists such that we can drain the bps event queue of all native events
    // more efficiently than if we were to return control to Qt after each event. This
    // is important for handling touch events which can come in rapidly.
    forever {
        // Only emit the awake() and aboutToBlock() signals in the second iteration. For the
        // first iteration, the UNIX event dispatcher will have taken care of that already.
        // Also native events are actually processed one loop iteration after they were
        // retrieved with bps_get_event().

        // Filtering the native event should happen between the awake() and aboutToBlock()
        // signal emissions. The calls awake() - filterNativeEvent() - aboutToBlock() -
        // bps_get_event() need not to be interrupted by a break or return statement.
        if (eventCount > 0) {
            if (event) {
                emit awake();
                filterNativeEvent(QByteArrayLiteral("bps_event_t"), static_cast<void*>(event), 0);
                emit aboutToBlock();

                if (Q_LIKELY(holding_channel != -1)) {
                    // We are now done with this BPS event.  Destroy it.
                    destroyHeldBpsEvent(holding_channel);
                }
            }

            // Update the timeout
            // Clock source is monotonic, so we can recalculate how much timeout is left
            if (timeoutTotal != -1) {
                timespec t2 = qt_gettime();
                timeoutLeft = timeoutTotal
                              - (timespecToMillisecs(t2) - timespecToMillisecs(startTime));
                if (timeoutLeft < 0)
                    timeoutLeft = 0;
            }

            timespec tnext;
            if (d->timerList.timerWait(tnext)) {
                int timeoutNext = timespecToMillisecs(tnext);
                if (timeoutNext < timeoutLeft || timeoutTotal == -1) {
                    timeoutTotal = timeoutLeft = timeoutNext;
                    startTime = qt_gettime();
                }
            }
        }

        event = 0;
        {   // We need to increase loop level in this scope,
            // because bps_get_event can also invoke callbacks
            QScopedLoopLevelCounter loopLevelCounter(d->threadData);

            // Wait for event or file to be ready
            const int result = bps_get_event(&event, timeoutLeft);
            if (Q_UNLIKELY(result != BPS_SUCCESS))
                qWarning("QEventDispatcherBlackberry: bps_get_event failed");
        }

        if (!event)    // In case of !event, we break out of the loop to let Qt process the timers
            break;     // (since timeout has expired) and socket notifiers that are now ready.

        if (bps_event_get_domain(event) == bpsUnblockDomain) {
            timeoutTotal = 0;   // in order to immediately drain the event queue of native events
            event = 0;          // (especially touch move events) we don't break out here
        } else {
            // Move the event to our holding channel so we can manage when it is destroyed.
            if (Q_LIKELY(holding_channel != 1) &&
                Q_UNLIKELY(bps_channel_push_event(holding_channel, event) != BPS_SUCCESS)) {
                qWarning("QEventDispatcherBlackberry: bps_channel_push_event failed");
            }
        }

        ++eventCount;

        // Make sure we are not trapped in this loop due to continuous native events
        // also we cannot recalculate the timeout without a monotonic clock as the time may have changed
        const unsigned int maximumEventCount = 12;
        if (Q_UNLIKELY((eventCount > maximumEventCount && timeoutLeft == 0)
                       || !QElapsedTimer::isMonotonic())) {
            if (event) {
                filterNativeEvent(QByteArrayLiteral("bps_event_t"), static_cast<void*>(event), 0);

                if (Q_LIKELY(holding_channel != -1)) {
                    // We are now done with this BPS event.  Destroy it.
                    destroyHeldBpsEvent(holding_channel);
                }
            }
            break;
        }
    }

    // If this was a recursive call into this function, a new holding channel was created for
    // this run, so destroy it now.
    if ((holding_channel != d->holding_channel) &&
        Q_LIKELY(holding_channel != -1) &&
        Q_UNLIKELY(bps_channel_destroy(holding_channel) != BPS_SUCCESS)) {
        qWarning("QEventDispatcherBlackberry: bps_channel_destroy failed");
    }

    // the number of bits set in the file sets
    return d->ioData->count;
}

void QEventDispatcherBlackberry::wakeUp()
{
    Q_D(QEventDispatcherBlackberry);
    if (d->wakeUps.testAndSetAcquire(0, 1)) {
        bps_event_t *event;
        if (Q_LIKELY(bps_event_create(&event, bpsUnblockDomain, 0, 0, 0) == BPS_SUCCESS)) {
            if (Q_LIKELY(bps_channel_push_event(d->bps_channel, event) == BPS_SUCCESS))
                return;
            else
                bps_event_destroy(event);
        }
        qWarning("QEventDispatcherBlackberry: wakeUp failed");
    }
}

int QEventDispatcherBlackberry::ioEvents(int fd)
{
    int io_events = 0;

    Q_D(QEventDispatcherBlackberry);

    if (FD_ISSET(fd, &d->sn_vec[0].enabled_fds))
        io_events |= BPS_IO_INPUT;

    if (FD_ISSET(fd, &d->sn_vec[1].enabled_fds))
        io_events |= BPS_IO_OUTPUT;

    if (FD_ISSET(fd, &d->sn_vec[2].enabled_fds))
        io_events |= BPS_IO_EXCEPT;

    return io_events;
}

QT_END_NAMESPACE
