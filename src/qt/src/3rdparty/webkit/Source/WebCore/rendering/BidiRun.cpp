/**
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 * Copyright (C) 2003, 2004, 2005, 2006, 2010 Apple Inc. All rights reserved.
 * Copyright (C) 2006 Andrew Wellington (proton@wiretapped.net)
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 */

#include "config.h"
#include "BidiRun.h"
#include "InlineBox.h"
#include "RenderArena.h"
#include <wtf/RefCountedLeakCounter.h>

using namespace WTF;

namespace WebCore {

#ifndef NDEBUG
static RefCountedLeakCounter bidiRunCounter("BidiRun");

static bool inBidiRunDestroy;
#endif

void BidiRun::destroy()
{
#ifndef NDEBUG
    inBidiRunDestroy = true;
#endif
    RenderArena* renderArena = m_object->renderArena();
    delete this;
#ifndef NDEBUG
    inBidiRunDestroy = false;
#endif

    // Recover the size left there for us by operator delete and free the memory.
    renderArena->free(*reinterpret_cast<size_t*>(this), this);
}

void* BidiRun::operator new(size_t sz, RenderArena* renderArena) throw()
{
#ifndef NDEBUG
    bidiRunCounter.increment();
#endif
    return renderArena->allocate(sz);
}

void BidiRun::operator delete(void* ptr, size_t sz)
{
#ifndef NDEBUG
    bidiRunCounter.decrement();
#endif
    ASSERT(inBidiRunDestroy);

    // Stash size where destroy() can find it.
    *(size_t*)ptr = sz;
}

}
