/*
 * Copyright (C) 2012 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "BlockAllocator.h"

#include "CopiedBlock.h"
#include "CopyWorkList.h"
#include "MarkedBlock.h"
#include "WeakBlock.h"
#include <wtf/CurrentTime.h>

namespace JSC {

BlockAllocator::BlockAllocator()
    : m_superRegion()
    , m_copiedRegionSet(CopiedBlock::blockSize)
    , m_markedRegionSet(MarkedBlock::blockSize)
    , m_fourKBBlockRegionSet(WeakBlock::blockSize)
    , m_workListRegionSet(CopyWorkListSegment::blockSize)
    , m_numberOfEmptyRegions(0)
    , m_isCurrentlyAllocating(false)
    , m_blockFreeingThreadShouldQuit(false)
    , m_blockFreeingThread(createThread(blockFreeingThreadStartFunc, this, "JavaScriptCore::BlockFree"))
{
    RELEASE_ASSERT(m_blockFreeingThread);
    m_regionLock.Init();
}

BlockAllocator::~BlockAllocator()
{
    releaseFreeRegions();
    {
        MutexLocker locker(m_emptyRegionConditionLock);
        m_blockFreeingThreadShouldQuit = true;
        m_emptyRegionCondition.broadcast();
    }
    waitForThreadCompletion(m_blockFreeingThread);
    ASSERT(allRegionSetsAreEmpty());
    ASSERT(m_emptyRegions.isEmpty());
}

bool BlockAllocator::allRegionSetsAreEmpty() const
{
    return m_copiedRegionSet.isEmpty()
        && m_markedRegionSet.isEmpty()
        && m_fourKBBlockRegionSet.isEmpty()
        && m_workListRegionSet.isEmpty();
}

void BlockAllocator::releaseFreeRegions()
{
    while (true) {
        Region* region;
        {
            SpinLockHolder locker(&m_regionLock);
            if (!m_numberOfEmptyRegions)
                region = 0;
            else {
                region = m_emptyRegions.removeHead();
                RELEASE_ASSERT(region);
                m_numberOfEmptyRegions--;
            }
        }
        
        if (!region)
            break;

        region->destroy();
    }
}

void BlockAllocator::waitForRelativeTimeWhileHoldingLock(double relative)
{
    if (m_blockFreeingThreadShouldQuit)
        return;

    m_emptyRegionCondition.timedWait(m_emptyRegionConditionLock, currentTime() + relative);
}

void BlockAllocator::waitForRelativeTime(double relative)
{
    // If this returns early, that's fine, so long as it doesn't do it too
    // frequently. It would only be a bug if this function failed to return
    // when it was asked to do so.
    
    MutexLocker locker(m_emptyRegionConditionLock);
    waitForRelativeTimeWhileHoldingLock(relative);
}

void BlockAllocator::blockFreeingThreadStartFunc(void* blockAllocator)
{
    static_cast<BlockAllocator*>(blockAllocator)->blockFreeingThreadMain();
}

void BlockAllocator::blockFreeingThreadMain()
{
    size_t currentNumberOfEmptyRegions;
    while (!m_blockFreeingThreadShouldQuit) {
        // Generally wait for one second before scavenging free blocks. This
        // may return early, particularly when we're being asked to quit.
        waitForRelativeTime(1.0);
        if (m_blockFreeingThreadShouldQuit)
            break;
        
        if (m_isCurrentlyAllocating) {
            m_isCurrentlyAllocating = false;
            continue;
        }

        // Sleep until there is actually work to do rather than waking up every second to check.
        {
            MutexLocker locker(m_emptyRegionConditionLock);
            SpinLockHolder regionLocker(&m_regionLock);
            while (!m_numberOfEmptyRegions && !m_blockFreeingThreadShouldQuit) {
                m_regionLock.Unlock();
                m_emptyRegionCondition.wait(m_emptyRegionConditionLock);
                m_regionLock.Lock();
            }
            currentNumberOfEmptyRegions = m_numberOfEmptyRegions;
        }
        
        size_t desiredNumberOfEmptyRegions = currentNumberOfEmptyRegions / 2;
        
        while (!m_blockFreeingThreadShouldQuit) {
            Region* region;
            {
                SpinLockHolder locker(&m_regionLock);
                if (m_numberOfEmptyRegions <= desiredNumberOfEmptyRegions)
                    region = 0;
                else {
                    region = m_emptyRegions.removeHead();
                    RELEASE_ASSERT(region);
                    m_numberOfEmptyRegions--;
                }
            }
            
            if (!region)
                break;
            
            region->destroy();
        }
    }
}

} // namespace JSC
