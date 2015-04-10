/*
 * Copyright (C) 2010, 2011 Research In Motion Limited. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "config.h"
#include "CacheClientBlackBerry.h"

#include "MemoryCache.h"
#include <BlackBerryPlatformMisc.h>
#include <BlackBerryPlatformSettings.h>

namespace WebCore {

CacheClientBlackBerry* CacheClientBlackBerry::get()
{
    static CacheClientBlackBerry s_cacheClient;
    return &s_cacheClient;
}

CacheClientBlackBerry::CacheClientBlackBerry()
{
}

void CacheClientBlackBerry::initialize()
{
#if ENABLE(BLACKBERRY_DEBUG_MEMORY)
    bool isDisabled = true;
#else
    bool isDisabled = false;
#endif
    memoryCache()->setDisabled(isDisabled);
    if (!isDisabled) {
        // We have to set a non-zero interval to schedule cache pruning after a CachedImage becoming dead.
        memoryCache()->setDeadDecodedDataDeletionInterval(0.01);
        updateCacheCapacity();
    }
}

void CacheClientBlackBerry::updateCacheCapacity()
{
#if ENABLE(BLACKBERRY_DEBUG_MEMORY)
    // We're debugging memory usage. So keep it disabled.
#else
    unsigned cacheTotalCapacity = 64 * 1024 * 1024;
    unsigned cacheMinDeadCapacity = cacheTotalCapacity / 4;
    unsigned cacheMaxDeadCapacity = cacheTotalCapacity / 2;

    memoryCache()->setCapacities(cacheMinDeadCapacity, cacheMaxDeadCapacity, cacheTotalCapacity);
#endif
}

} // WebCore
