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

#ifndef WeakSetInlines_h
#define WeakSetInlines_h

#include "MarkedBlock.h"

namespace JSC {

inline WeakImpl* WeakSet::allocate(JSValue jsValue, WeakHandleOwner* weakHandleOwner, void* context)
{
    WeakSet& weakSet = MarkedBlock::blockFor(jsValue.asCell())->weakSet();
    WeakBlock::FreeCell* allocator = weakSet.m_allocator;
    if (UNLIKELY(!allocator))
        allocator = weakSet.findAllocator();
    weakSet.m_allocator = allocator->next;

    WeakImpl* weakImpl = WeakBlock::asWeakImpl(allocator);
    return new (NotNull, weakImpl) WeakImpl(jsValue, weakHandleOwner, context);
}

inline void WeakBlock::finalize(WeakImpl* weakImpl)
{
    ASSERT(weakImpl->state() == WeakImpl::Dead);
    weakImpl->setState(WeakImpl::Finalized);
    WeakHandleOwner* weakHandleOwner = weakImpl->weakHandleOwner();
    if (!weakHandleOwner)
        return;
    weakHandleOwner->finalize(Handle<Unknown>::wrapSlot(&const_cast<JSValue&>(weakImpl->jsValue())), weakImpl->context());
}

} // namespace JSC

#endif // WeakSetInlines_h
