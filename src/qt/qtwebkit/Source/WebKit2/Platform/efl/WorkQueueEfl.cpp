/*
    Copyright (C) 2012 Samsung Electronics

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

#include "config.h"
#include "WorkQueue.h"

#include <sys/timerfd.h>
#include <wtf/Assertions.h>
#include <wtf/CurrentTime.h>

static const int invalidSocketDescriptor = -1;
static const int threadMessageSize = 1;
static const char finishThreadMessage[] = "F";
static const char wakupThreadMessage[] = "W";

PassOwnPtr<WorkQueue::TimerWorkItem> WorkQueue::TimerWorkItem::create(Function<void()> function, double expireTime)
{
    if (expireTime < 0)
        return nullptr;

    return adoptPtr(new TimerWorkItem(function, expireTime));
}

WorkQueue::TimerWorkItem::TimerWorkItem(Function<void()> function, double expireTime)
    : m_function(function)
    , m_expireTime(expireTime)
{
}

void WorkQueue::platformInitialize(const char* name)
{
    int fds[2];
    if (pipe(fds))
        ASSERT_NOT_REACHED();

    m_readFromPipeDescriptor = fds[0];
    m_writeToPipeDescriptor = fds[1];
    FD_ZERO(&m_fileDescriptorSet);
    FD_SET(m_readFromPipeDescriptor, &m_fileDescriptorSet);
    m_maxFileDescriptor = m_readFromPipeDescriptor;

    m_socketDescriptor = invalidSocketDescriptor;

    m_threadLoop = true;
    createThread(reinterpret_cast<WTF::ThreadFunction>(&WorkQueue::workQueueThread), this, name);
}

void WorkQueue::platformInvalidate()
{
    sendMessageToThread(finishThreadMessage);
}

void WorkQueue::performWork()
{
    while (true) {
        Vector<Function<void()> > workItemQueue;

        {
            MutexLocker locker(m_workItemQueueLock);
            if (m_workItemQueue.isEmpty())
                return;

            m_workItemQueue.swap(workItemQueue);
        }

        for (size_t i = 0; i < workItemQueue.size(); ++i) {
            workItemQueue[i]();
            deref();
        }
    }
}

void WorkQueue::performFileDescriptorWork()
{
    fd_set readFileDescriptorSet = m_fileDescriptorSet;

    if (select(m_maxFileDescriptor + 1, &readFileDescriptorSet, 0, 0, getNextTimeOut()) >= 0) {
        if (FD_ISSET(m_readFromPipeDescriptor, &readFileDescriptorSet)) {
            char readBuf[threadMessageSize];
            if (read(m_readFromPipeDescriptor, readBuf, threadMessageSize) == -1)
                LOG_ERROR("Failed to read from WorkQueueThread pipe");
            if (!strncmp(readBuf, finishThreadMessage, threadMessageSize))
                m_threadLoop = false;
        }

        if (m_socketDescriptor != invalidSocketDescriptor && FD_ISSET(m_socketDescriptor, &readFileDescriptorSet))
            m_socketEventHandler();
    }
}

struct timeval* WorkQueue::getNextTimeOut()
{
    MutexLocker locker(m_timerWorkItemsLock);
    if (m_timerWorkItems.isEmpty())
        return 0;

    static struct timeval timeValue;
    timeValue.tv_sec = 0;
    timeValue.tv_usec = 0;
    double timeOut = m_timerWorkItems[0]->expireTime() - currentTime();
    if (timeOut > 0) {
        timeValue.tv_sec = static_cast<long>(timeOut);
        timeValue.tv_usec = static_cast<long>((timeOut - timeValue.tv_sec) * 1000000);
    }

    return &timeValue;
}

void WorkQueue::insertTimerWorkItem(PassOwnPtr<TimerWorkItem> item)
{
    if (!item)
        return;

    size_t position = 0;

    MutexLocker locker(m_timerWorkItemsLock);
    // m_timerWorkItems should be ordered by expire time.
    for (; position < m_timerWorkItems.size(); ++position)
        if (item->expireTime() < m_timerWorkItems[position]->expireTime())
            break;

    m_timerWorkItems.insert(position, item);
}

void WorkQueue::performTimerWork()
{
    Vector<OwnPtr<TimerWorkItem> > timerWorkItems;

    {
        // Protects m_timerWorkItems.
        MutexLocker locker(m_timerWorkItemsLock);
        if (m_timerWorkItems.isEmpty())
            return;

        // Copies all the timer work items in m_timerWorkItems to local vector.
        m_timerWorkItems.swap(timerWorkItems);
    }

    double current = currentTime();

    for (size_t i = 0; i < timerWorkItems.size(); ++i) {
        if (!timerWorkItems[i]->expired(current)) {
            // If a timer work item does not expired, keep it to the m_timerWorkItems.
            // m_timerWorkItems should be ordered by expire time.
            insertTimerWorkItem(timerWorkItems[i].release());
            continue;
        }

        // If a timer work item expired, dispatch the function of the work item.
        timerWorkItems[i]->dispatch();
        deref();
    }
}

void WorkQueue::sendMessageToThread(const char* message)
{
    MutexLocker locker(m_writeToPipeDescriptorLock);
    if (write(m_writeToPipeDescriptor, message, threadMessageSize) == -1)
        LOG_ERROR("Failed to wake up WorkQueue Thread");
}

void* WorkQueue::workQueueThread(WorkQueue* workQueue)
{
    while (workQueue->m_threadLoop) {
        workQueue->performWork();
        workQueue->performTimerWork();
        workQueue->performFileDescriptorWork();
    }

    close(workQueue->m_readFromPipeDescriptor);
    close(workQueue->m_writeToPipeDescriptor);

    return 0;
}

void WorkQueue::registerSocketEventHandler(int fileDescriptor, const Function<void()>& function)
{
    if (m_socketDescriptor != invalidSocketDescriptor)
        LOG_ERROR("%d is already registerd.", fileDescriptor);

    m_socketDescriptor = fileDescriptor;
    m_socketEventHandler = function;

    if (fileDescriptor > m_maxFileDescriptor)
        m_maxFileDescriptor = fileDescriptor;
    FD_SET(fileDescriptor, &m_fileDescriptorSet);
}

void WorkQueue::unregisterSocketEventHandler(int fileDescriptor)
{
    m_socketDescriptor = invalidSocketDescriptor;

    if (fileDescriptor == m_maxFileDescriptor)
        m_maxFileDescriptor = m_readFromPipeDescriptor;
    FD_CLR(fileDescriptor, &m_fileDescriptorSet);
}

void WorkQueue::dispatch(const Function<void()>& function)
{
    ref();

    {
        MutexLocker locker(m_workItemQueueLock);
        m_workItemQueue.append(function);
    }

    sendMessageToThread(wakupThreadMessage);
}

void WorkQueue::dispatchAfterDelay(const Function<void()>& function, double delay)
{
    if (delay < 0)
        return;

    OwnPtr<TimerWorkItem> timerWorkItem = TimerWorkItem::create(function, currentTime() + delay);
    if (!timerWorkItem)
        return;

    ref();
    insertTimerWorkItem(timerWorkItem.release());
    sendMessageToThread(wakupThreadMessage);
}
