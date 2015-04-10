/*
 * Copyright (C) 2007, 2009 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer. 
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution. 
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission. 
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "WorkQueue.h"

#if PLATFORM(QT)
#include "WorkQueueItemQt.h"
#else
#include "WorkQueueItem.h"
#endif
#include <wtf/Assertions.h>

static const unsigned queueLength = 1024;

static WorkQueueItem* theQueue[queueLength];
static unsigned startOfQueue;
static unsigned endOfQueue;

WorkQueue* WorkQueue::shared()
{
    static WorkQueue* sharedInstance = new WorkQueue;
    return sharedInstance;
}

WorkQueue::WorkQueue()
    : m_frozen(false)
{
}

void WorkQueue::queue(WorkQueueItem* item)
{
    ASSERT(endOfQueue < queueLength);
    ASSERT(endOfQueue >= startOfQueue);

    if (m_frozen) {
        delete item;
        return;
    }

    theQueue[endOfQueue++] = item;
}

WorkQueueItem* WorkQueue::dequeue()
{
    ASSERT(endOfQueue >= startOfQueue);

    if (startOfQueue == endOfQueue)
        return 0;

    return theQueue[startOfQueue++];
}

unsigned WorkQueue::count()
{
    return endOfQueue - startOfQueue;
}

void WorkQueue::clear()
{
    for (unsigned i = startOfQueue; i < endOfQueue; ++i) {
        delete theQueue[i];
        theQueue[i] = 0;
    }

    startOfQueue = 0;
    endOfQueue = 0;
}

bool WorkQueue::processWork()
{
    bool startedLoad = false;

    while (!startedLoad && count()) {
        WorkQueueItem* item = dequeue();
        ASSERT(item);
        startedLoad = item->invoke();
        delete item;
    }

    // If we're done and we didn't start a load, then we're really done, so return true.
    return !startedLoad;
}
