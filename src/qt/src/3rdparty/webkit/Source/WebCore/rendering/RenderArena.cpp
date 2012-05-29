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

#include <stdlib.h>
#include <string.h>
#include <wtf/Assertions.h>

#define ROUNDUP(x, y) ((((x)+((y)-1))/(y))*(y))

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
{
    // Initialize the arena pool
    INIT_ARENA_POOL(&m_pool, "RenderArena", arenaSize);

    // Zero out the recyclers array
    memset(m_recyclers, 0, sizeof(m_recyclers));
}

RenderArena::~RenderArena()
{
    FinishArenaPool(&m_pool);
}

void* RenderArena::allocate(size_t size)
{
#ifndef NDEBUG
    // Use standard malloc so that memory debugging tools work.
    ASSERT(this);
    void* block = ::malloc(debugHeaderSize + size);
    RenderArenaDebugHeader* header = static_cast<RenderArenaDebugHeader*>(block);
    header->arena = this;
    header->size = size;
    header->signature = signature;
    return static_cast<char*>(block) + debugHeaderSize;
#else
    void* result = 0;

    // Ensure we have correct alignment for pointers.  Important for Tru64
    size = ROUNDUP(size, sizeof(void*));

    // Check recyclers first
    if (size < gMaxRecycledSize) {
        const int index = size >> 2;

        result = m_recyclers[index];
        if (result) {
            // Need to move to the next object
            void* next = *((void**)result);
            m_recyclers[index] = next;
        }
    }

    if (!result) {
        // Allocate a new chunk from the arena
        ARENA_ALLOCATE(result, &m_pool, size);
    }

    return result;
#endif
}

void RenderArena::free(size_t size, void* ptr)
{
#ifndef NDEBUG
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

    // See if it's a size that we recycle
    if (size < gMaxRecycledSize) {
        const int index = size >> 2;
        void* currentTop = m_recyclers[index];
        m_recyclers[index] = ptr;
        *((void**)ptr) = currentTop;
    }
#endif
}

} // namespace WebCore
