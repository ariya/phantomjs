/*
 * Copyright (C) 2013 Apple Inc. All rights reserved.
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
#include "SuperRegion.h"

#include "Region.h"

namespace JSC {

const uint64_t SuperRegion::s_fixedHeapMemoryPoolSize = 4 * 1024 * static_cast<uint64_t>(MB);

SuperRegion::SuperRegion()
    : MetaAllocator(Region::s_regionSize, Region::s_regionSize)
    , m_reservationBase(0)
{
#if ENABLE(SUPER_REGION)
    // Over-allocate so that we can make sure that we're aligned to the size of Regions.
    m_reservation = PageReservation::reserve(s_fixedHeapMemoryPoolSize + Region::s_regionSize, OSAllocator::JSGCHeapPages);
    m_reservationBase = getAlignedBase(m_reservation);
    addFreshFreeSpace(m_reservationBase, s_fixedHeapMemoryPoolSize);
#else
    UNUSED_PARAM(m_reservation);
    UNUSED_PARAM(m_reservationBase);
#endif
}

SuperRegion::~SuperRegion()
{
#if ENABLE(SUPER_REGION)
    m_reservation.deallocate();
#endif
}

void* SuperRegion::getAlignedBase(PageReservation& reservation)
{
    for (char* current = static_cast<char*>(reservation.base()); current < static_cast<char*>(reservation.base()) + Region::s_regionSize; current += pageSize()) {
        if (!(reinterpret_cast<size_t>(current) & ~Region::s_regionMask))
            return current;
    }
    ASSERT_NOT_REACHED();
    return 0;
}

void* SuperRegion::allocateNewSpace(size_t&)
{
    return 0;
}

void SuperRegion::notifyNeedPage(void* page)
{
    m_reservation.commit(page, Region::s_regionSize);
}

void SuperRegion::notifyPageIsFree(void* page)
{
    m_reservation.decommit(page, Region::s_regionSize);
}

} // namespace JSC
