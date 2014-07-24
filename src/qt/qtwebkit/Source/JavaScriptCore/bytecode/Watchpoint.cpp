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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#include "config.h"
#include "Watchpoint.h"

#include "LinkBuffer.h"
#include <wtf/PassRefPtr.h>

namespace JSC {

Watchpoint::~Watchpoint()
{
    if (isOnList())
        remove();
}

WatchpointSet::WatchpointSet(InitialWatchpointSetMode mode)
    : m_isWatched(mode == InitializedWatching)
    , m_isInvalidated(false)
{
}

WatchpointSet::~WatchpointSet()
{
    // Fire all watchpoints. This is necessary because it is possible, say with
    // structure watchpoints, for the watchpoint set owner to die while the
    // watchpoint owners are still live.
    fireAllWatchpoints();
}

void WatchpointSet::add(Watchpoint* watchpoint)
{
    if (!watchpoint)
        return;
    m_set.push(watchpoint);
    m_isWatched = true;
}

void WatchpointSet::notifyWriteSlow()
{
    ASSERT(m_isWatched);
    
    fireAllWatchpoints();
    m_isWatched = false;
    m_isInvalidated = true;
}

void WatchpointSet::fireAllWatchpoints()
{
    while (!m_set.isEmpty())
        m_set.begin()->fire();
}

void InlineWatchpointSet::add(Watchpoint* watchpoint)
{
    inflate()->add(watchpoint);
}

WatchpointSet* InlineWatchpointSet::inflateSlow()
{
    ASSERT(isThin());
    WatchpointSet* fat = adoptRef(new WatchpointSet(InitializedBlind)).leakRef();
    if (m_data & IsInvalidatedFlag)
        fat->m_isInvalidated = true;
    if (m_data & IsWatchedFlag)
        fat->m_isWatched = true;
    m_data = bitwise_cast<uintptr_t>(fat);
    return fat;
}

void InlineWatchpointSet::freeFat()
{
    ASSERT(isFat());
    fat()->deref();
}

} // namespace JSC

