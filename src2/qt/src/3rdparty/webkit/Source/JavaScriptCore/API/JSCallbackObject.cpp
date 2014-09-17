/*
 * Copyright (C) 2006 Apple Computer, Inc.  All rights reserved.
 * Copyright (C) 2007 Eric Seidel <eric@webkit.org>
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
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#include "config.h"
#include "JSCallbackObject.h"

#include "Heap.h"
#include <wtf/text/StringHash.h>

namespace JSC {

ASSERT_CLASS_FITS_IN_CELL(JSCallbackObject<JSObjectWithGlobalObject>);
ASSERT_CLASS_FITS_IN_CELL(JSCallbackObject<JSGlobalObject>);

// Define the two types of JSCallbackObjects we support.
template <> const ClassInfo JSCallbackObject<JSObjectWithGlobalObject>::s_info = { "CallbackObject", &JSObjectWithGlobalObject::s_info, 0, 0 };
template <> const ClassInfo JSCallbackObject<JSGlobalObject>::s_info = { "CallbackGlobalObject", &JSGlobalObject::s_info, 0, 0 };

void JSCallbackObjectData::finalize(Handle<Unknown> handle, void* context)
{
    JSClassRef jsClass = static_cast<JSClassRef>(context);
    JSObjectRef thisRef = toRef(asObject(handle.get()));
    
    for (; jsClass; jsClass = jsClass->parentClass)
        if (JSObjectFinalizeCallback finalize = jsClass->finalize)
            finalize(thisRef);
    HandleSlot slot = handle.slot();
    HandleHeap::heapFor(slot)->deallocate(slot);
}
    
} // namespace JSC
