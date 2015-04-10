/*
 * Copyright (C) 2013 Intel Corporation. All rights reserved.
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

#include "config.h"
#include "SigprocmaskSyscall.h"

#if ENABLE(SECCOMP_FILTERS)

#include <signal.h>
#include <string.h>

namespace WebKit {

PassOwnPtr<Syscall> SigprocmaskSyscall::createFromContext(ucontext_t* ucontext)
{
    // This syscall is never proxied to the broker process and resolved locally.
    // What we do here is silently remove SIGSYS from the signal set so no
    // thread will ever be able to block it.
    ASSERT(ucontext);

    mcontext_t mcontext = ucontext->uc_mcontext;
    int how = mcontext.gregs[REG_ARG0];
    sigset_t* set = reinterpret_cast<sigset_t*>(mcontext.gregs[REG_ARG1]);
    sigset_t* oldSet = reinterpret_cast<sigset_t*>(mcontext.gregs[REG_ARG2]);

    if (oldSet)
        memcpy(oldSet, &ucontext->uc_sigmask, sizeof(sigset_t));

    if (how == SIG_SETMASK)
        memcpy(&ucontext->uc_sigmask, set, sizeof(sigset_t));
    else
        sigorset(&ucontext->uc_sigmask, set, &ucontext->uc_sigmask);

    sigdelset(&ucontext->uc_sigmask, SIGSYS);
    mcontext.gregs[REG_SYSCALL] = 0;

    return nullptr;
}

} // namespace WebKit

#endif // ENABLE(SECCOMP_FILTERS)
