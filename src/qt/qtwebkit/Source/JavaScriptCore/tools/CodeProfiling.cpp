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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#include "config.h"
#include "CodeProfiling.h"

#include "CodeProfile.h"
#include <wtf/MetaAllocator.h>

#if HAVE(SIGNAL_H)
#include <signal.h>
#endif

#if OS(LINUX)
#include <sys/time.h>
#endif

namespace JSC {

volatile CodeProfile* CodeProfiling::s_profileStack = 0;
CodeProfiling::Mode CodeProfiling::s_mode = CodeProfiling::Disabled;
WTF::MetaAllocatorTracker* CodeProfiling::s_tracker = 0;

#if COMPILER(CLANG)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmissing-noreturn"
#endif

#if (PLATFORM(MAC) && CPU(X86_64)) || (OS(LINUX) && CPU(X86) && !OS(ANDROID))
// Helper function to start & stop the timer.
// Presently we're using the wall-clock timer, since this seems to give the best results.
static void setProfileTimer(unsigned usec)
{
    itimerval timer;
    timer.it_value.tv_sec = 0;
    timer.it_value.tv_usec = usec;
    timer.it_interval.tv_sec = 0;
    timer.it_interval.tv_usec = usec;
    setitimer(ITIMER_REAL, &timer, 0);
}
#endif

#if COMPILER(CLANG)
#pragma clang diagnostic pop
#endif

#if PLATFORM(MAC) && CPU(X86_64)
static void profilingTimer(int, siginfo_t*, void* uap)
{
    mcontext_t context = static_cast<ucontext_t*>(uap)->uc_mcontext;
    CodeProfiling::sample(reinterpret_cast<void*>(context->__ss.__rip),
                          reinterpret_cast<void**>(context->__ss.__rbp));
}
#elif OS(LINUX) && CPU(X86) && !OS(ANDROID)
static void profilingTimer(int, siginfo_t*, void* uap)
{
    mcontext_t context = static_cast<ucontext_t*>(uap)->uc_mcontext;
    CodeProfiling::sample(reinterpret_cast<void*>(context.gregs[REG_EIP]),
                          reinterpret_cast<void**>(context.gregs[REG_EBP]));
}
#endif

// Callback triggered when the timer is fired.
void CodeProfiling::sample(void* pc, void** framePointer)
{
    CodeProfile* profileStack = const_cast<CodeProfile*>(s_profileStack);
    if (profileStack)
        profileStack->sample(pc, framePointer);
}

void CodeProfiling::notifyAllocator(WTF::MetaAllocator* allocator)
{
#if !OS(WINCE)
    // Check for JSC_CODE_PROFILING.
    const char* codeProfilingMode = getenv("JSC_CODE_PROFILING");
    if (!codeProfilingMode)
        return;

    // Check for a valid mode, currently "1", "2", or "3".
    if (!codeProfilingMode[0] || codeProfilingMode[1])
        return;
    switch (*codeProfilingMode) {
    case '1':
        s_mode = Enabled;
        break;
    case '2':
        s_mode = Verbose;
        break;
    case '3':
        s_mode = VeryVerbose;
        break;
    default:
        return;
    }

    ASSERT(enabled());
    ASSERT(!s_tracker);
    s_tracker = new WTF::MetaAllocatorTracker();
    allocator->trackAllocations(s_tracker);
#endif
}

void* CodeProfiling::getOwnerUIDForPC(void* address)
{
    if (!s_tracker)
        return 0;
    WTF::MetaAllocatorHandle* handle = s_tracker->find(address);
    if (!handle)
        return 0;
    return handle->ownerUID();
}

void CodeProfiling::begin(const SourceCode& source)
{
    // Push a new CodeProfile onto the stack for each script encountered.
    CodeProfile* profileStack = const_cast<CodeProfile*>(s_profileStack);
    bool alreadyProfiling = profileStack;
    s_profileStack = profileStack = new CodeProfile(source, profileStack);

    // Is the profiler already running - if so, the timer will already be set up.
    if (alreadyProfiling)
        return;

#if (PLATFORM(MAC) && CPU(X86_64)) || (OS(LINUX) && CPU(X86) && !OS(ANDROID))
    // Regsiter a signal handler & itimer.
    struct sigaction action;
    action.sa_sigaction = reinterpret_cast<void (*)(int, siginfo_t *, void *)>(profilingTimer);
    sigfillset(&action.sa_mask);
    action.sa_flags = SA_SIGINFO;
    sigaction(SIGALRM, &action, 0);
    setProfileTimer(100);
#endif
}

void CodeProfiling::end()
{
    // Pop the current profiler off the stack.
    CodeProfile* current = const_cast<CodeProfile*>(s_profileStack);
    ASSERT(current);
    s_profileStack = current->parent();

    // Is this the outermost script being profiled? - if not, just return.
    // We perform all output of profiles recursively from the outermost script,
    // to minimize profiling overhead from skewing results.
    if (s_profileStack)
        return;

#if (PLATFORM(MAC) && CPU(X86_64)) || (OS(LINUX) && CPU(X86) && !OS(ANDROID))
    // Stop profiling
    setProfileTimer(0);
#endif

    current->report();
    delete current;
}

}
