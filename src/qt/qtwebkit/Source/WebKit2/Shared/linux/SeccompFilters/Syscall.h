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

#ifndef Syscall_h
#define Syscall_h

#if ENABLE(SECCOMP_FILTERS)

#if CPU(X86_64)
#define REG_SYSCALL REG_RAX
#define REG_ARG0    REG_RDI
#define REG_ARG1    REG_RSI
#define REG_ARG2    REG_RDX
#define REG_ARG3    REG_R10
#elif CPU(X86)
#define REG_SYSCALL REG_EAX
#define REG_ARG0    REG_EBX
#define REG_ARG1    REG_ECX
#define REG_ARG2    REG_EDX
#define REG_ARG3    REG_ESI
#else
#error "CPU not supported."
#endif

#include <signal.h>
#include <sys/types.h>
#include <wtf/Noncopyable.h>
#include <wtf/PassOwnPtr.h>

namespace CoreIPC {
class ArgumentDecoder;
class ArgumentEncoder;
}

namespace WebKit {

class SyscallResult;
class SyscallPolicy;

class Syscall {
    WTF_MAKE_NONCOPYABLE(Syscall);

public:
    virtual ~Syscall() { }

    static PassOwnPtr<Syscall> createFromContext(ucontext_t*);
    static PassOwnPtr<Syscall> createFromDecoder(CoreIPC::ArgumentDecoder*);

    int type() const { return m_type; }

    void setContext(mcontext_t* context) { m_context = context; }
    mcontext_t* context() const { return m_context; }

    virtual void setResult(const SyscallResult*) = 0;
    virtual PassOwnPtr<SyscallResult> execute(const SyscallPolicy&) = 0;
    virtual void encode(CoreIPC::ArgumentEncoder&) const = 0;
    virtual bool decode(CoreIPC::ArgumentDecoder*) = 0;

protected:
    Syscall(int type, mcontext_t*);

private:
    int m_type;
    mcontext_t* m_context;
};

class SyscallResult {
    WTF_MAKE_NONCOPYABLE(SyscallResult);

public:
    virtual ~SyscallResult() { }

    static PassOwnPtr<SyscallResult> createFromDecoder(CoreIPC::ArgumentDecoder*, int fd);

    int type() const { return m_type; }

    virtual void encode(CoreIPC::ArgumentEncoder&) const = 0;
    virtual bool decode(CoreIPC::ArgumentDecoder*, int fd=-1) = 0;

protected:
    SyscallResult(int type);

private:
    int m_type;
};

} // namespace WebKit

#endif // ENABLE(SECCOMP_FILTERS)

#endif // Syscall_h
