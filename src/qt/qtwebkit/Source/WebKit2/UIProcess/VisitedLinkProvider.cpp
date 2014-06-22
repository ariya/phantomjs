/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
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
#include "VisitedLinkProvider.h"

#include "SharedMemory.h"
#include "VisitedLinkTable.h"
#include "WebContext.h"
#include "WebProcessMessages.h"

using namespace WebCore;

namespace WebKit {

static const int VisitedLinkTableMaxLoad = 2;

VisitedLinkProvider::VisitedLinkProvider(WebContext* context)
    : m_context(context)
    , m_visitedLinksPopulated(false)
    , m_keyCount(0)
    , m_tableSize(0)
    , m_pendingVisitedLinksTimer(RunLoop::main(), this, &VisitedLinkProvider::pendingVisitedLinksTimerFired)
{
}

void VisitedLinkProvider::processDidFinishLaunching(WebProcessProxy* process)
{
    m_processesWithoutVisitedLinkState.add(process);

    if (m_keyCount)
        m_pendingVisitedLinksTimer.startOneShot(0);

    if (m_visitedLinksPopulated)
        return;

    m_context->populateVisitedLinks();

    m_visitedLinksPopulated = true;
}

void VisitedLinkProvider::addVisitedLink(LinkHash linkHash)
{
    m_pendingVisitedLinks.add(linkHash);

    if (!m_pendingVisitedLinksTimer.isActive())
        m_pendingVisitedLinksTimer.startOneShot(0);
}

void VisitedLinkProvider::processDidClose(WebProcessProxy* process)
{
    m_processesWithVisitedLinkState.remove(process);
    m_processesWithoutVisitedLinkState.remove(process);
}

static unsigned nextPowerOf2(unsigned v)
{
    // Taken from http://www.cs.utk.edu/~vose/c-stuff/bithacks.html
    // Devised by Sean Anderson, Sepember 14, 2001
    
    v--;
    v |= v >> 1;
    v |= v >> 2;
    v |= v >> 4;
    v |= v >> 8;
    v |= v >> 16;
    v++;
    
    return v;
}

static unsigned tableSizeForKeyCount(unsigned keyCount)
{
    // We want the table to be at least half empty.
    unsigned tableSize = nextPowerOf2(keyCount * VisitedLinkTableMaxLoad);

    // Ensure that the table size is at least the size of a page.
    size_t minimumTableSize = SharedMemory::systemPageSize() / sizeof(LinkHash);
    if (tableSize < minimumTableSize)
        return minimumTableSize;
    
    return tableSize;
}

void VisitedLinkProvider::pendingVisitedLinksTimerFired()
{
    Vector<WebCore::LinkHash> pendingVisitedLinks;
    copyToVector(m_pendingVisitedLinks, pendingVisitedLinks);
    m_pendingVisitedLinks.clear();

    unsigned currentTableSize = m_tableSize;

    // Upper bound on needed size - some of the links may be duplicates, in which case we could have done with less.
    unsigned newTableSize = tableSizeForKeyCount(m_keyCount + pendingVisitedLinks.size());

    // Never decrease table size when adding to it, to avoid unneeded churn.
    newTableSize = std::max(currentTableSize, newTableSize);

    // Links that were added.
    Vector<WebCore::LinkHash> addedVisitedLinks;

    // VisitedLinkTable remains internally consistent when adding, so it's OK to modify it in place
    // even if a web process is accessing it at the same time.
    if (currentTableSize != newTableSize) {
        RefPtr<SharedMemory> newTableMemory = SharedMemory::create(newTableSize * sizeof(LinkHash));

        // We failed to create the shared memory.
        if (!newTableMemory) {
            LOG_ERROR("Could not allocate shared memory for visited link table");
            return;
        }

        memset(newTableMemory->data(), 0, newTableMemory->size());

        RefPtr<SharedMemory> currentTableMemory = m_table.sharedMemory();

        m_table.setSharedMemory(newTableMemory);
        m_tableSize = newTableSize;

        if (currentTableMemory) {
            ASSERT(currentTableMemory->size() == currentTableSize * sizeof(LinkHash));

            // Go through the current hash table and re-add all entries to the new hash table.
            const LinkHash* currentLinkHashes = static_cast<const LinkHash*>(currentTableMemory->data());
            for (unsigned i = 0; i < currentTableSize; ++i) {
                LinkHash linkHash = currentLinkHashes[i];
                
                if (!linkHash)
                    continue;

                // It should always be possible to add the link hash to a new table.
                if (!m_table.addLinkHash(linkHash))
                    ASSERT_NOT_REACHED();
            }
        }
    }

    for (size_t i = 0; i < pendingVisitedLinks.size(); ++i) {
        if (m_table.addLinkHash(pendingVisitedLinks[i]))
            addedVisitedLinks.append(pendingVisitedLinks[i]);
    }

    m_keyCount += pendingVisitedLinks.size();


    for (HashSet<WebProcessProxy*>::iterator iter = m_processesWithVisitedLinkState.begin(); iter != m_processesWithVisitedLinkState.end(); ++iter) {
        WebProcessProxy* process = *iter;
        if (currentTableSize != newTableSize) {
            // In the rare case of needing to resize the table, we'll bypass the VisitedLinkStateChanged optimization,
            // and unconditionally use AllVisitedLinkStateChanged for the process.
            m_processesWithoutVisitedLinkState.add(process);
            continue;
        }

        if (addedVisitedLinks.size() <= 20)
            process->send(Messages::WebProcess::VisitedLinkStateChanged(addedVisitedLinks), 0);
        else
            process->send(Messages::WebProcess::AllVisitedLinkStateChanged(), 0);
    }

    for (HashSet<WebProcessProxy*>::iterator iter = m_processesWithoutVisitedLinkState.begin(); iter != m_processesWithoutVisitedLinkState.end(); ++iter) {
        WebProcessProxy* process = *iter;

        SharedMemory::Handle handle;
        if (!m_table.sharedMemory()->createHandle(handle, SharedMemory::ReadOnly))
            return;

        process->send(Messages::WebProcess::SetVisitedLinkTable(handle), 0);
        process->send(Messages::WebProcess::AllVisitedLinkStateChanged(), 0);

        m_processesWithVisitedLinkState.add(process);
    }
    
    m_processesWithoutVisitedLinkState.clear();
}

} // namespace WebKit
