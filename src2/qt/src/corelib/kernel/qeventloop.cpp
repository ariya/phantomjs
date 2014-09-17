/****************************************************************************
**
** Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
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

#include "qeventloop.h"

#include "qabstracteventdispatcher.h"
#include "qcoreapplication.h"
#include "qelapsedtimer.h"

#include "qobject_p.h"
#include <private/qthread_p.h>

QT_BEGIN_NAMESPACE

class QEventLoopPrivate : public QObjectPrivate
{
    Q_DECLARE_PUBLIC(QEventLoop)
public:
    inline QEventLoopPrivate()
        : exit(true), inExec(false), returnCode(-1)
    { }
    bool exit, inExec;
    int returnCode;
};

/*!
    \class QEventLoop
    \brief The QEventLoop class provides a means of entering and leaving an event loop.

    At any time, you can create a QEventLoop object and call exec()
    on it to start a local event loop. From within the event loop,
    calling exit() will force exec() to return.

    \sa QAbstractEventDispatcher
*/

/*!
    \enum QEventLoop::ProcessEventsFlag

    This enum controls the types of events processed by the
    processEvents() functions.

    \value AllEvents All events. Note that
    \l{QEvent::DeferredDelete}{DeferredDelete} events are processed
    specially. See QObject::deleteLater() for more details.

    \value ExcludeUserInputEvents Do not process user input events,
    such as ButtonPress and KeyPress. Note that the events are not
    discarded; they will be delivered the next time processEvents() is
    called without the ExcludeUserInputEvents flag.

    \value ExcludeSocketNotifiers Do not process socket notifier
    events. Note that the events are not discarded; they will be
    delivered the next time processEvents() is called without the
    ExcludeSocketNotifiers flag.

    \value WaitForMoreEvents Wait for events if no pending events are
    available.

    \omitvalue X11ExcludeTimers
    \omitvalue ExcludeUserInput
    \omitvalue WaitForMore
    \omitvalue EventLoopExec
    \omitvalue DialogExec
    \value DeferredDeletion deprecated - do not use.

    \sa processEvents()
*/

/*!
    Constructs an event loop object with the given \a parent.
*/
QEventLoop::QEventLoop(QObject *parent)
    : QObject(*new QEventLoopPrivate, parent)
{
    Q_D(QEventLoop);
    if (!QCoreApplication::instance()) {
        qWarning("QEventLoop: Cannot be used without QApplication");
    } else if (!d->threadData->eventDispatcher) {
        QThreadPrivate::createEventDispatcher(d->threadData);
    }
}

/*!
    Destroys the event loop object.
*/
QEventLoop::~QEventLoop()
{ }


/*!
    Processes pending events that match \a flags until there are no
    more events to process. Returns true if pending events were handled;
    otherwise returns false.

    This function is especially useful if you have a long running
    operation and want to show its progress without allowing user
    input; i.e. by using the \l ExcludeUserInputEvents flag.

    This function is simply a wrapper for
    QAbstractEventDispatcher::processEvents(). See the documentation
    for that function for details.
*/
bool QEventLoop::processEvents(ProcessEventsFlags flags)
{
    Q_D(QEventLoop);
    if (!d->threadData->eventDispatcher)
        return false;
    if (flags & DeferredDeletion)
        QCoreApplication::sendPostedEvents(0, QEvent::DeferredDelete);
    return d->threadData->eventDispatcher->processEvents(flags);
}

/*!
    Enters the main event loop and waits until exit() is called.
    Returns the value that was passed to exit().

    If \a flags are specified, only events of the types allowed by
    the \a flags will be processed.

    It is necessary to call this function to start event handling. The
    main event loop receives events from the window system and
    dispatches these to the application widgets.

    Generally speaking, no user interaction can take place before
    calling exec(). As a special case, modal widgets like QMessageBox
    can be used before calling exec(), because modal widgets
    use their own local event loop.

    To make your application perform idle processing (i.e. executing a
    special function whenever there are no pending events), use a
    QTimer with 0 timeout. More sophisticated idle processing schemes
    can be achieved using processEvents().

    \sa QApplication::quit(), exit(), processEvents()
*/
int QEventLoop::exec(ProcessEventsFlags flags)
{
    Q_D(QEventLoop);
    //we need to protect from race condition with QThread::exit
    QMutexLocker locker(&static_cast<QThreadPrivate *>(QObjectPrivate::get(d->threadData->thread))->mutex);
    if (d->threadData->quitNow)
        return -1;

    if (d->inExec) {
        qWarning("QEventLoop::exec: instance %p has already called exec()", this);
        return -1;
    }
    d->inExec = true;
    d->exit = false;
    ++d->threadData->loopLevel;
    d->threadData->eventLoops.push(this);
    locker.unlock();

    // remove posted quit events when entering a new event loop
    QCoreApplication *app = QCoreApplication::instance();
    if (app && app->thread() == thread())
        QCoreApplication::removePostedEvents(app, QEvent::Quit);

#if defined(QT_NO_EXCEPTIONS)
    while (!d->exit)
        processEvents(flags | WaitForMoreEvents | EventLoopExec);
#else
    try {
        while (!d->exit)
            processEvents(flags | WaitForMoreEvents | EventLoopExec);
    } catch (...) {
        qWarning("Qt has caught an exception thrown from an event handler. Throwing\n"
                 "exceptions from an event handler is not supported in Qt. You must\n"
                 "reimplement QApplication::notify() and catch all exceptions there.\n");

        // copied from below
        locker.relock();
        QEventLoop *eventLoop = d->threadData->eventLoops.pop();
        Q_ASSERT_X(eventLoop == this, "QEventLoop::exec()", "internal error");
        Q_UNUSED(eventLoop); // --release warning
        d->inExec = false;
        --d->threadData->loopLevel;

        throw;
    }
#endif

    // copied above
    locker.relock();
    QEventLoop *eventLoop = d->threadData->eventLoops.pop();
    Q_ASSERT_X(eventLoop == this, "QEventLoop::exec()", "internal error");
    Q_UNUSED(eventLoop); // --release warning
    d->inExec = false;
    --d->threadData->loopLevel;

    return d->returnCode;
}

/*!
    Process pending events that match \a flags for a maximum of \a
    maxTime milliseconds, or until there are no more events to
    process, whichever is shorter.
    This function is especially useful if you have a long running
    operation and want to show its progress without allowing user
    input, i.e. by using the \l ExcludeUserInputEvents flag.

    \bold{Notes:}
    \list
    \o This function does not process events continuously; it
       returns after all available events are processed.
    \o Specifying the \l WaitForMoreEvents flag makes no sense
       and will be ignored.
    \endlist
*/
void QEventLoop::processEvents(ProcessEventsFlags flags, int maxTime)
{
    Q_D(QEventLoop);
    if (!d->threadData->eventDispatcher)
        return;

    QElapsedTimer start;
    start.start();
    if (flags & DeferredDeletion)
        QCoreApplication::sendPostedEvents(0, QEvent::DeferredDelete);
    while (processEvents(flags & ~WaitForMoreEvents)) {
        if (start.elapsed() > maxTime)
            break;
        if (flags & DeferredDeletion)
            QCoreApplication::sendPostedEvents(0, QEvent::DeferredDelete);
    }
}

/*!
    Tells the event loop to exit with a return code.

    After this function has been called, the event loop returns from
    the call to exec(). The exec() function returns \a returnCode.

    By convention, a \a returnCode of 0 means success, and any non-zero
    value indicates an error.

    Note that unlike the C library function of the same name, this
    function \e does return to the caller -- it is event processing that
    stops.

    \sa QCoreApplication::quit(), quit(), exec()
*/
void QEventLoop::exit(int returnCode)
{
    Q_D(QEventLoop);
    if (!d->threadData->eventDispatcher)
        return;

    d->returnCode = returnCode;
    d->exit = true;
    d->threadData->eventDispatcher->interrupt();
}

/*!
    Returns true if the event loop is running; otherwise returns
    false. The event loop is considered running from the time when
    exec() is called until exit() is called.

    \sa exec() exit()
 */
bool QEventLoop::isRunning() const
{
    Q_D(const QEventLoop);
    return !d->exit;
}

/*!
    Wakes up the event loop.

    \sa QAbstractEventDispatcher::wakeUp()
*/
void QEventLoop::wakeUp()
{
    Q_D(QEventLoop);
    if (!d->threadData->eventDispatcher)
        return;
    d->threadData->eventDispatcher->wakeUp();
}

/*!
    Tells the event loop to exit normally.

    Same as exit(0).

    \sa QCoreApplication::quit(), exit()
*/
void QEventLoop::quit()
{ exit(0); }

QT_END_NAMESPACE
