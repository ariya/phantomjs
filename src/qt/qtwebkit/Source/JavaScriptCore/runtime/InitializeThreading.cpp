/*
 * Copyright (C) 2008 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer. 
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution. 
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission. 
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "InitializeThreading.h"

#include "ExecutableAllocator.h"
#include "Heap.h"
#include "HeapStatistics.h"
#include "Options.h"
#include "Identifier.h"
#include "JSDateMath.h"
#include "JSGlobalObject.h"
#include "JSLock.h"
#include "LLIntData.h"
#include "WriteBarrier.h"
#include <wtf/dtoa.h>
#include <wtf/Threading.h>
#include <wtf/dtoa/cached-powers.h>

using namespace WTF;

namespace JSC {

#if OS(DARWIN)
static pthread_once_t initializeThreadingKeyOnce = PTHREAD_ONCE_INIT;
#endif

static void initializeThreadingOnce()
{
    WTF::double_conversion::initialize();
    WTF::initializeThreading();
    GlobalJSLock::initialize();
    Options::initialize();
    if (Options::recordGCPauseTimes())
        HeapStatistics::initialize();
#if ENABLE(WRITE_BARRIER_PROFILING)
    WriteBarrierCounters::initialize();
#endif
#if ENABLE(ASSEMBLER)
    ExecutableAllocator::initializeAllocator();
#endif
    JSStack::initializeThreading();
#if ENABLE(LLINT)
    LLInt::initialize();
#endif
}

void initializeThreading()
{
#if OS(DARWIN)
    pthread_once(&initializeThreadingKeyOnce, initializeThreadingOnce);
#else
    static bool initializedThreading = false;
    if (!initializedThreading) {
        initializeThreadingOnce();
        initializedThreading = true;
    }
#endif
}

} // namespace JSC
