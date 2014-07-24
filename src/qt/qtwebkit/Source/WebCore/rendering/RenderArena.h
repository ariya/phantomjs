/*
 * Copyright (C) 2003 Apple Computer, Inc.
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

#ifndef RenderArena_h
#define RenderArena_h

#include "Arena.h"
#include <wtf/FastAllocBase.h>
#include <wtf/Noncopyable.h>
#include <wtf/PassRefPtr.h>
#include <wtf/RefCounted.h>

namespace WebCore {

static const size_t gMaxRecycledSize = 1024;

class RenderArena : public RefCounted<RenderArena> {
public:
    static PassRefPtr<RenderArena> create() { return adoptRef(new RenderArena); }
    ~RenderArena();

    // Memory management functions
    void* allocate(size_t);
    void free(size_t, void*);

    size_t totalRenderArenaSize() const { return m_totalSize; }
    size_t totalRenderArenaAllocatedBytes() const { return m_totalAllocated; }

private:
    RenderArena(unsigned arenaSize = 8192);
    
    // Underlying arena pool
    ArenaPool m_pool;

    // The mask used to secure the recycled freelist pointers.
    uintptr_t m_mask;
    // The recycler array is sparse with the indices being multiples of the
    // rounding size, sizeof(void*), i.e., 0, 4, 8, 12, 16, 20, ... on 32-bit.
    static const size_t kRecyclerShift = (sizeof(void*) == 8) ? 3 : 2;
    void* m_recyclers[gMaxRecycledSize >> kRecyclerShift];

    size_t m_totalSize;
    size_t m_totalAllocated;
};

} // namespace WebCore

#endif // RenderArena_h
