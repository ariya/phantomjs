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
#include "Syscall.h"

#if ENABLE(SECCOMP_FILTERS)

#include "ArgumentCoders.h"
#include "OpenSyscall.h"
#include "SigactionSyscall.h"
#include "SigprocmaskSyscall.h"
#include <seccomp.h>

namespace WebKit {

PassOwnPtr<Syscall> Syscall::createFromContext(ucontext_t* ucontext)
{
    mcontext_t* mcontext = &ucontext->uc_mcontext;

    switch (mcontext->gregs[REG_SYSCALL]) {
    case __NR_open:
        return adoptPtr(new OpenSyscall(mcontext));
    case __NR_openat:
        return OpenSyscall::createFromOpenatContext(mcontext);
    case __NR_creat:
        return OpenSyscall::createFromCreatContext(mcontext);
    case __NR_sigprocmask:
    case __NR_rt_sigprocmask:
        return SigprocmaskSyscall::createFromContext(ucontext);
    case __NR_sigaction:
    case __NR_rt_sigaction:
        return SigactionSyscall::createFromContext(mcontext);
    default:
        CRASH();
    }

    return nullptr;
}

PassOwnPtr<Syscall> Syscall::createFromDecoder(CoreIPC::ArgumentDecoder* decoder)
{
    int type;
    if (!decoder->decode(type))
        return nullptr;

    OwnPtr<Syscall> syscall;
    if (type == __NR_open)
        syscall = adoptPtr(new OpenSyscall(0));

    if (!syscall->decode(decoder))
        return nullptr;

    return syscall.release();
}

Syscall::Syscall(int type, mcontext_t* context)
    : m_type(type)
    , m_context(context)
{
}

PassOwnPtr<SyscallResult> SyscallResult::createFromDecoder(CoreIPC::ArgumentDecoder* decoder, int fd)
{
    int type;
    if (!decoder->decode(type))
        return nullptr;

    OwnPtr<SyscallResult> result;
    if (type == __NR_open)
        result = adoptPtr(new OpenSyscallResult(-1, 0));

    if (!result->decode(decoder, fd))
        return nullptr;

    return result.release();
}

SyscallResult::SyscallResult(int type)
    : m_type(type)
{
}

} // namespace WebKit

#endif // ENABLE(SECCOMP_FILTERS)
