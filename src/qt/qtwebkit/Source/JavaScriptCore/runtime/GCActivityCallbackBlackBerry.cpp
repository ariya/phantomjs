/*
 * Copyright (C) 2012 Research In Motion Limited. All rights reserved.
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
#include "GCActivityCallback.h"

#include "Heap.h"
#include "VM.h"
#include <BlackBerryPlatformMemory.h>

namespace JSC {

DefaultGCActivityCallback::DefaultGCActivityCallback(Heap* heap)
    : GCActivityCallback(heap->vm())
{
}

void DefaultGCActivityCallback::doWork()
{
    JSLockHolder lock(m_vm);
    m_vm->heap.collect(Heap::DoNotSweep);
}

void DefaultGCActivityCallback::didAllocate(size_t)
{
    if (m_timer.started())
        return;

    // Try using ~5% CPU time.
    m_timer.start(m_vm->heap.lastGCLength() * 20);
}

void DefaultGCActivityCallback::willCollect()
{
    cancel();
}

void DefaultGCActivityCallback::cancel()
{
    m_timer.stop();
}

}
