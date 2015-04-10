/*
 * Copyright (C) 2003 Apple Computer, Inc.
 * Copyright (C) Research In Motion Limited 2010. All rights reserved.
 *
 * Portions are Copyright (C) 1998 Netscape Communications Corporation.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * Alternatively, the contents of this file may be used under the terms
 * of either the Mozilla Public License Version 1.1, found at
 * http://www.mozilla.org/MPL/ (the "MPL") or the GNU General Public
 * License Version 2.0, found at http://www.fsf.org/copyleft/gpl.html
 * (the "GPL"), in which case the provisions of the MPL or the GPL are
 * applicable instead of those above.  If you wish to allow use of your
 * version of this file only under the terms of one of those two
 * licenses (the MPL or the GPL) and not to allow others to use your
 * version of this file under the LGPL, indicate your decision by
 * deletingthe provisions above and replace them with the notice and
 * other provisions required by the MPL or the GPL, as the case may be.
 * If you do not delete the provisions above, a recipient may use your
 * version of this file under any of the LGPL, the MPL or the GPL.
 */

#include "config.h"
#include "RenderArena.h"

#include <limits>
#include <stdlib.h>
#include <string.h>
#include <wtf/Assertions.h>
#include <wtf/CryptographicallyRandomNumber.h>

#define ROUNDUP(x, y) ((((x)+((y)-1))/(y))*(y))

#ifdef NDEBUG
static void* MaskPtr(void* p, uintptr_t mask)
{
    return reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(p) ^ mask);
}
#endif

namespace WebCore {

#ifndef NDEBUG

const int signature = 0xDBA00AEA;
const int signatureDead = 0xDBA00AED;

typedef struct {
    RenderArena* arena;
    size_t size;
    int signature;
} RenderArenaDebugHeader;

static const size_t debugHeaderSize = ARENA_ALIGN(sizeof(RenderArenaDebugHeader));

#endif

RenderArena::RenderArena(unsigned arenaSize)
    : m_totalSize(0)
    , m_totalAllocated(0)
{
    ASSERT(arenaSize > sizeof(Arena) + ARENA_ALIGN_MASK);
    // The underlying Arena class allocates some metadata on top of our
    // requested size. Factor this in so that we can get perfect power-of-two
    // allocation sizes passed to the underlying malloc() call.
    arenaSize -= (sizeof(Arena) + ARENA_ALIGN_MASK);
    // Initialize the arena pool
    INIT_ARENA_POOL(&m_pool, "RenderArena", arenaSize);

    // Zero out the recyclers array
    memset(m_recyclers, 0, sizeof(m_recyclers));

    // Mask freelist pointers to detect corruption and stop freelist spraying.
    // We use an arbitray function and rely on ASLR to randomize it.
    // The first value in RenderObject (or any class) is a vtable pointer, which
    // always overlaps with the next pointer. This change guarantees that the
    // masked vtable/next pointer will never point to valid memory. So, we
    // should immediately crash on the first invalid vtable access for a stale
    // RenderObject pointer.
    // See http://download.crowdstrike.com/papers/hes-exploiting-a-coalmine.pdf.
    WTF::cryptographicallyRandomValues(&m_mask, sizeof(m_mask));
    m_mask |= (static_cast<uintptr_t>(3) << (std::numeric_limits<uintptr_t>::digits - 2)) | 1;
}

RenderArena::~RenderArena()
{
    FinishArenaPool(&m_pool);
}

void* RenderArena::allocate(size_t size)
{
    ASSERT(size <= gMaxRecycledSize - 32);
    m_totalSize += size;

#ifdef ADDRESS_SANITIZER
    return ::malloc(size);
#elif !defined(NDEBUG)
    // Use standard malloc so that memory debugging tools work.
    ASSERT(this);
    void* block = ::malloc(debugHeaderSize + size);
    RenderArenaDebugHeader* header = static_cast<RenderArenaDebugHeader*>(block);
    header->arena = this;
    header->size = size;
    header->signature = signature;
    return static_cast<char*>(block) + debugHeaderSize;
#else
    // Ensure we have correct alignment for pointers.  Important for Tru64
    size = ROUNDUP(size, sizeof(void*));

    const size_t index = size >> kRecyclerShift;

    void* result = m_recyclers[index];
    if (result) {
        // Need to move to the next object
        void* next = MaskPtr(*((void**)result), m_mask);
        m_recyclers[index] = next;
    }

    if (!result) {
        // Allocate a new chunk from the arena
        unsigned bytesAllocated = 0;
        ARENA_ALLOCATE(result, &m_pool, size, &bytesAllocated);
        m_totalAllocated += bytesAllocated;
    }

    return result;
#endif
}

void RenderArena::free(size_t size, void* ptr)
{
    ASSERT(size <= gMaxRecycledSize - 32);
    m_totalSize -= size;

#ifdef ADDRESS_SANITIZER
    ::free(ptr);
#elif !defined(NDEBUG)
    // Use standard free so that memory debugging tools work.
    void* block = static_cast<char*>(ptr) - debugHeaderSize;
    RenderArenaDebugHeader* header = static_cast<RenderArenaDebugHeader*>(block);
    ASSERT(header->signature == signature);
    ASSERT_UNUSED(size, header->size == size);
    ASSERT(header->arena == this);
    header->signature = signatureDead;
    ::free(block);
#else
    // Ensure we have correct alignment for pointers.  Important for Tru64
    size = ROUNDUP(size, sizeof(void*));

    const size_t index = size >> kRecyclerShift;
    void* currentTop = m_recyclers[index];
    m_recyclers[index] = ptr;
    *((void**)ptr) = MaskPtr(currentTop, m_mask);
#endif
}

} // namespace WebCore
