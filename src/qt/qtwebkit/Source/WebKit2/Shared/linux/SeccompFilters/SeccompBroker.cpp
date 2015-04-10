/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
 * Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies)
 * Copyright (C) 2011 Igalia S.L.
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
#include "SeccompBroker.h"

#if ENABLE(SECCOMP_FILTERS)

#include "ArgumentCoders.h"
#include "Syscall.h"
#include <errno.h>
#include <fcntl.h>
#include <seccomp.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#ifndef SYS_SECCOMP
#define SYS_SECCOMP 1
#endif

static const size_t messageMaxSize = 4096;
static const char onlineCPUCountPath[] = "/sys/devices/system/cpu/online";

namespace WebKit {

class SeccompBrokerClient {
public:
    static SeccompBrokerClient& shared(int socket = -1);
    ~SeccompBrokerClient();

    void dispatch(Syscall*) const;

    bool handleIfOpeningOnlineCPUCount(mcontext_t*) const;

private:
    SeccompBrokerClient(int socket);

    int m_socket;
    int m_onlineCPUCountFd;

    mutable Mutex m_socketLock;
};

static ssize_t sendMessage(int socket, void* data, size_t size, int fd = -1)
{
    ASSERT(size <= messageMaxSize);

    struct msghdr message;
    memset(&message, 0, sizeof(message));

    struct iovec iov;
    memset(&iov, 0, sizeof(iov));
    iov.iov_base = data;
    iov.iov_len = size;

    message.msg_iov = &iov;
    message.msg_iovlen = 1;

    char control[CMSG_SPACE(sizeof(fd))];
    if (fd >= 0) {
        message.msg_control = control;
        message.msg_controllen = sizeof(control);
        memset(message.msg_control, 0, message.msg_controllen);

        struct cmsghdr* cmsg = CMSG_FIRSTHDR(&message);
        cmsg->cmsg_level = SOL_SOCKET;
        cmsg->cmsg_type = SCM_RIGHTS;
        cmsg->cmsg_len = CMSG_LEN(sizeof(fd));

        memmove(CMSG_DATA(cmsg), &fd, sizeof(fd));
    }

    return sendmsg(socket, &message, 0);
}

static ssize_t receiveMessage(int socket, void* data, size_t size, int* fd = 0)
{
    struct msghdr message;
    memset(&message, 0, sizeof(message));

    struct iovec iov;
    memset(&iov, 0, sizeof(iov));
    iov.iov_base = data;
    iov.iov_len = size;

    message.msg_iov = &iov;
    message.msg_iovlen = 1;

    char control[CMSG_SPACE(sizeof(fd))];
    message.msg_control = control;
    message.msg_controllen = sizeof(control);
    memset(message.msg_control, 0, message.msg_controllen);

    ssize_t receivedBytes = recvmsg(socket, &message, 0);

    if (fd && receivedBytes > 0) {
        struct cmsghdr* cmsg = CMSG_FIRSTHDR(&message);
        if (cmsg && cmsg->cmsg_level == SOL_SOCKET && cmsg->cmsg_type == SCM_RIGHTS)
            memcpy(fd, CMSG_DATA(cmsg), sizeof(*fd));
        else
            *fd = -1;
    }

    return receivedBytes >= 0 ? receivedBytes : -errno;
}

static void SIGSYSHandler(int signal, siginfo_t* info, void* data)
{
    if (signal != SIGSYS || info->si_code != SYS_SECCOMP)
        CRASH();

    ucontext_t* ucontext = static_cast<ucontext_t*>(data);
    if (!ucontext)
        CRASH();

    SeccompBrokerClient* client = &SeccompBrokerClient::shared();

    if (client->handleIfOpeningOnlineCPUCount(&ucontext->uc_mcontext))
        return;

    // createFromContext might return a nullptr if it is able to resolve the
    // syscall locally without sending it to the broker process. In this case,
    // we just return. Examples of locally resolved syscalls are the ones
    // with cached resources and invalid arguments.
    OwnPtr<Syscall> syscall = Syscall::createFromContext(ucontext);
    if (!syscall)
        return;

    client->dispatch(syscall.get());
}

static void registerSIGSYSHandler()
{
    struct sigaction action;
    memset(&action, 0, sizeof(action));
    action.sa_sigaction = &SIGSYSHandler;
    action.sa_flags = SA_SIGINFO | SA_NODEFER;

    if (sigaction(SIGSYS, &action, 0) < 0)
        CRASH();

    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGSYS);

    if (sigprocmask(SIG_UNBLOCK, &mask, 0) < 0)
        CRASH();
}

SeccompBrokerClient& SeccompBrokerClient::shared(int socket)
{
    DEFINE_STATIC_LOCAL(SeccompBrokerClient, brokerClient, (socket));

    return brokerClient;
}

SeccompBrokerClient::SeccompBrokerClient(int socket)
    : m_socket(socket)
    , m_onlineCPUCountFd(open(onlineCPUCountPath, O_RDONLY))
{
    ASSERT(m_socket >= 0 && m_onlineCPUCountFd >= 0);
}

SeccompBrokerClient::~SeccompBrokerClient()
{
    close(m_socket);
    close(m_onlineCPUCountFd);
}

void SeccompBrokerClient::dispatch(Syscall* syscall) const
{
    OwnPtr<CoreIPC::ArgumentEncoder> encoder = CoreIPC::ArgumentEncoder::create();
    encoder->encode(*syscall);

    char buffer[messageMaxSize];
    ssize_t receivedBytes = 0;
    int fd = -1;

    m_socketLock.lock();

    if (sendMessage(m_socket, encoder->buffer(), encoder->bufferSize()) < 0)
        CRASH();

    while (true) {
        receivedBytes = receiveMessage(m_socket, &buffer, sizeof(buffer), &fd);
        if (receivedBytes > 0)
            break;

        if (receivedBytes != -EINTR)
            CRASH();
    }

    m_socketLock.unlock();

    OwnPtr<CoreIPC::ArgumentDecoder> decoder = CoreIPC::ArgumentDecoder::create((const uint8_t*) buffer, receivedBytes);
    OwnPtr<SyscallResult> result = SyscallResult::createFromDecoder(decoder.get(), fd);
    if (!result)
        CRASH();

    syscall->setResult(result.get());
}

bool SeccompBrokerClient::handleIfOpeningOnlineCPUCount(mcontext_t* context) const
{
    if (context->gregs[REG_SYSCALL] != __NR_open)
        return false;

    const char *path = reinterpret_cast<char*>(context->gregs[REG_ARG0]);
    if (strcmp(onlineCPUCountPath, path))
        return false;

    // Malloc will eventually check the number of online CPUs (i.e being
    // scheduled) present on the system by opening a special file. If it does
    // that in the middle of the SIGSYS signal handler, it might trigger a
    // recursive attempt of proxying the open() syscall to the broker.
    // Because of that, we cache this resource.
    context->gregs[REG_SYSCALL] = dup(m_onlineCPUCountFd);

    return true;
}

void SeccompBroker::launchProcess(SeccompFilters* filters, const SyscallPolicy& policy)
{
    static bool initialized = false;
    if (initialized)
        return;

    // The sigprocmask filters bellow are needed to trap sigprocmask()
    // so we can prevent the running processes from blocking SIGSYS.
    filters->addRule("sigprocmask", SeccompFilters::Trap,
        0, SeccompFilters::Equal, SIG_BLOCK,
        1, SeccompFilters::NotEqual, 0);
    filters->addRule("sigprocmask", SeccompFilters::Trap,
        0, SeccompFilters::Equal, SIG_SETMASK,
        1, SeccompFilters::NotEqual, 0);
    filters->addRule("rt_sigprocmask", SeccompFilters::Trap,
        0, SeccompFilters::Equal, SIG_BLOCK,
        1, SeccompFilters::NotEqual, 0);
    filters->addRule("rt_sigprocmask", SeccompFilters::Trap,
        0, SeccompFilters::Equal, SIG_SETMASK,
        1, SeccompFilters::NotEqual, 0);

    // The sigaction filters bellow are needed to trap sigaction()
    // so we can prevent the running processes from handling SIGSYS.
    filters->addRule("sigaction", SeccompFilters::Trap,
        0, SeccompFilters::Equal, SIGSYS);
    filters->addRule("rt_sigaction", SeccompFilters::Trap,
        0, SeccompFilters::Equal, SIGSYS);

    SeccompBroker seccompBroker;
    seccompBroker.setSyscallPolicy(policy);
    seccompBroker.initialize();

    initialized = true;
}

void SeccompBroker::initialize()
{
    int sockets[2];
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, sockets) < 0)
        CRASH();

    pid_t pid = fork();
    if (pid) { // Sandboxed process.
        close(sockets[1]);
        SeccompBrokerClient::shared(sockets[0]);
        registerSIGSYSHandler();
    } else { // Broker.
        // TODO: The broker should setup seccomp filters
        // for itself and block everything else other than
        // the minimal set of syscalls needed to execute the
        // syscalls it is suppose to proxy.
        close(sockets[0]);
        runLoop(sockets[1]);
    }
}

NO_RETURN void SeccompBroker::runLoop(int socket)
{
#ifndef NDEBUG
    int i = STDERR_FILENO + 1;
#else
    int i = 0;
#endif
    // Close all inherited file descriptors other
    // than the socket to the sandboxed process.
    for (; i < FD_SETSIZE; ++i)
        if (i != socket)
            close(i);

    while (true) {
        char buffer[messageMaxSize];
        ssize_t receivedBytes = receiveMessage(socket, &buffer, sizeof(buffer));
        if (receivedBytes == -EINTR)
            continue;

        if (receivedBytes <= 0)
            exit(receivedBytes ? EXIT_FAILURE : EXIT_SUCCESS);

        OwnPtr<CoreIPC::ArgumentDecoder> decoder = CoreIPC::ArgumentDecoder::create((const uint8_t*) buffer, receivedBytes);
        OwnPtr<Syscall> syscall = Syscall::createFromDecoder(decoder.get());
        if (!syscall)
            exit(EXIT_FAILURE);

        OwnPtr<SyscallResult> result = syscall->execute(m_policy);
        if (!result)
            exit(EXIT_FAILURE);

        OwnPtr<CoreIPC::ArgumentEncoder> encoder = CoreIPC::ArgumentEncoder::create();
        encoder->encode(*result);

        Vector<CoreIPC::Attachment> attachments = encoder->releaseAttachments();
        int fd = attachments.size() == 1 ? attachments[0].fileDescriptor() : -1;

        // The client is down, the broker should go away.
        if (sendMessage(socket, encoder->buffer(), encoder->bufferSize(), fd) < 0)
            exit(EXIT_SUCCESS);
    }
}

} // namespace WebKit

#endif // ENABLE(SECCOMP_FILTERS)
