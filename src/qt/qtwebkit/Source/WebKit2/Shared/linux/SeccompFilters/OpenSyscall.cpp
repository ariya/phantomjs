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
#include "OpenSyscall.h"

#if ENABLE(SECCOMP_FILTERS)

#include "ArgumentCoders.h"
#include "SyscallPolicy.h"
#include <errno.h>
#include <fcntl.h>
#include <seccomp.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <wtf/text/WTFString.h>

namespace WebKit {

COMPILE_ASSERT(!O_RDONLY, O_RDONLY);
COMPILE_ASSERT(O_WRONLY == 1, O_WRONLY);
COMPILE_ASSERT(O_RDWR == 2, O_RDWR);

PassOwnPtr<Syscall> OpenSyscall::createFromOpenatContext(mcontext_t* context)
{
    OwnPtr<OpenSyscall> open = adoptPtr(new OpenSyscall(0));

    open->setFlags(context->gregs[REG_ARG2]);
    open->setMode(context->gregs[REG_ARG3]);
    open->setContext(context);

    int fd = context->gregs[REG_ARG0];
    char* path = reinterpret_cast<char*>(context->gregs[REG_ARG1]);

    if (path[0] == '/') {
        open->setPath(path);
        return open.release();
    }

    struct stat pathStat;
    if (fstat(fd, &pathStat) == -1) {
        context->gregs[REG_SYSCALL] = -errno;
        return nullptr;
    }

    if (!S_ISDIR(pathStat.st_mode)) {
        context->gregs[REG_SYSCALL] = -ENOTDIR;
        return nullptr;
    }

    char fdLinkPath[32];
    snprintf(fdLinkPath, sizeof(fdLinkPath), "/proc/self/fd/%d", fd);

    char fdPath[PATH_MAX];
    ssize_t size = readlink(fdLinkPath, fdPath, sizeof(fdPath) - 1);
    if (size == -1) {
        context->gregs[REG_SYSCALL] = -errno;
        return nullptr;
    }

    // The "+ 2" here stands for the '/' and null terminator.
    if (size + strlen(path) + 2 > PATH_MAX) {
        context->gregs[REG_SYSCALL] = -ENAMETOOLONG;
        return nullptr;
    }

    sprintf(&fdPath[size], "/%s", path);
    open->setPath(fdPath);

    return open.release();
}

PassOwnPtr<Syscall> OpenSyscall::createFromCreatContext(mcontext_t* context)
{
    OpenSyscall* open = new OpenSyscall(0);

    open->setPath(CString(reinterpret_cast<char*>(context->gregs[REG_ARG0])));
    open->setFlags(O_CREAT | O_WRONLY | O_TRUNC);
    open->setMode(context->gregs[REG_ARG1]);
    open->setContext(context);

    return adoptPtr(open);
}

OpenSyscall::OpenSyscall(mcontext_t* context)
    : Syscall(__NR_open, context)
    , m_flags(0)
    , m_mode(0)
{
    if (!context)
        return;

    m_path = CString(reinterpret_cast<char*>(context->gregs[REG_ARG0]));
    m_flags = context->gregs[REG_ARG1];
    m_mode = context->gregs[REG_ARG2];
}

void OpenSyscall::setResult(const SyscallResult* result)
{
    ASSERT(context() && result->type() == type());

    const OpenSyscallResult* openResult = static_cast<const OpenSyscallResult*>(result);

    if (openResult->fd() >= 0)
        context()->gregs[REG_SYSCALL] = dup(openResult->fd());
    else
        context()->gregs[REG_SYSCALL] = -openResult->errorNumber();
}

PassOwnPtr<SyscallResult> OpenSyscall::execute(const SyscallPolicy& policy)
{
    if (!strncmp("/proc/self/", m_path.data(), 11)) {
        String resolvedSelfPath = ASCIILiteral("/proc/") + String::number(getppid()) + &m_path.data()[10];
        m_path = resolvedSelfPath.utf8().data();
    }

    SyscallPolicy::Permission permission = SyscallPolicy::NotAllowed;
    if (m_flags & O_RDWR)
        permission = static_cast<SyscallPolicy::Permission>(permission | SyscallPolicy::ReadAndWrite);
    else if (m_flags & O_WRONLY)
        permission = static_cast<SyscallPolicy::Permission>(permission | SyscallPolicy::Write);
    else
        permission = static_cast<SyscallPolicy::Permission>(permission | SyscallPolicy::Read);

    // Create a file implies write permission on the directory.
    if (m_flags & O_CREAT || m_flags & O_EXCL)
        permission = static_cast<SyscallPolicy::Permission>(permission | SyscallPolicy::Write);

    if (!policy.hasPermissionForPath(m_path.data(), permission))
        return adoptPtr(new OpenSyscallResult(-1, EACCES));

    // Permission granted, execute the syscall. The syscall might still
    // fail because of hard permissions enforced by the filesystem and
    // things like if the entry does not exist.
    int fd = open(m_path.data(), m_flags, m_mode);
    int errorNumber = fd == -1 ? errno : 0;

    return adoptPtr(new OpenSyscallResult(fd, errorNumber));
}

void OpenSyscall::encode(CoreIPC::ArgumentEncoder& encoder) const
{
    encoder << type();
    encoder << m_path;
    encoder << m_flags;
    encoder << m_mode;
}

bool OpenSyscall::decode(CoreIPC::ArgumentDecoder* decoder)
{
    // m_type already decoded by the parent class.

    if (!decoder->decode(m_path))
        return false;
    if (!decoder->decode(m_flags))
        return false;

    return decoder->decode(m_mode);
}

OpenSyscallResult::OpenSyscallResult(int fd, int errorNumber)
    : SyscallResult(__NR_open)
    , m_fd(fd)
    , m_errorNumber(errorNumber)
{
}

OpenSyscallResult::~OpenSyscallResult()
{
    if (m_fd >= 0)
        close(m_fd);
}

void OpenSyscallResult::encode(CoreIPC::ArgumentEncoder& encoder) const
{
    encoder << type();

    if (m_fd >= 0) {
        CoreIPC::Attachment attachment(m_fd);
        encoder.addAttachment(attachment);
    }

    encoder << m_errorNumber;
}

bool OpenSyscallResult::decode(CoreIPC::ArgumentDecoder* decoder, int fd)
{
    if (fd >= 0)
        m_fd = fd;

    return decoder->decode(m_errorNumber);
}

} // namespace WebKit

#endif // ENABLE(SECCOMP_FILTERS)
