/*
 * Copyright (C) 2010 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"

#include <wtf/Assertions.h>
#include <wtf/CrossThreadRefCounted.h>
#include <wtf/OwnPtr.h>
#include <wtf/PassRefPtr.h>
#include <wtf/RefCounted.h>
#include <wtf/RefPtr.h>
#include <wtf/Vector.h>

namespace WTF {

#ifndef NDEBUG
struct StructWithIntAndTwoBools { int a; bool b; bool c; };
static const size_t refCountedExtraDebugSize = sizeof(StructWithIntAndTwoBools) - sizeof(int);
#else
static const size_t refCountedExtraDebugSize = 0;
#endif

COMPILE_ASSERT(sizeof(OwnPtr<int>) == sizeof(int*), OwnPtr_should_stay_small);
COMPILE_ASSERT(sizeof(PassRefPtr<RefCounted<int> >) == sizeof(int*), PassRefPtr_should_stay_small);
COMPILE_ASSERT(sizeof(RefCounted<int>) == sizeof(int) + refCountedExtraDebugSize, RefCounted_should_stay_small);
COMPILE_ASSERT(sizeof(RefCountedCustomAllocated<int>) == sizeof(int) + refCountedExtraDebugSize, RefCountedCustomAllocated_should_stay_small);
COMPILE_ASSERT(sizeof(RefPtr<RefCounted<int> >) == sizeof(int*), RefPtr_should_stay_small);
COMPILE_ASSERT(sizeof(Vector<int>) == 3 * sizeof(int*), Vector_should_stay_small);

}
