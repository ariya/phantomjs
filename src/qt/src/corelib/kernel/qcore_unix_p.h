/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QCORE_UNIX_P_H
#define QCORE_UNIX_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of Qt code on Unix. This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include "qplatformdefs.h"

#ifndef Q_OS_UNIX
# error "qcore_unix_p.h included on a non-Unix system"
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <sys/wait.h>
#include <errno.h>
#include <fcntl.h>

#if defined(Q_OS_VXWORKS)
#  include <ioLib.h>
#endif

struct sockaddr;

#if defined(Q_OS_LINUX) && defined(O_CLOEXEC)
# define QT_UNIX_SUPPORTS_THREADSAFE_CLOEXEC 1
QT_BEGIN_NAMESPACE
namespace QtLibcSupplement {
    inline int accept4(int, sockaddr *, QT_SOCKLEN_T *, int)
    { errno = ENOSYS; return -1; }
    inline int dup3(int, int, int)
    { errno = ENOSYS; return -1; }
    inline int pipe2(int [], int )
    { errno = ENOSYS; return -1; }
}
QT_END_NAMESPACE
using namespace QT_PREPEND_NAMESPACE(QtLibcSupplement);

#else
# define QT_UNIX_SUPPORTS_THREADSAFE_CLOEXEC 0
#endif

#define EINTR_LOOP(var, cmd)                    \
    do {                                        \
        var = cmd;                              \
    } while (var == -1 && errno == EINTR)

QT_BEGIN_NAMESPACE

// Internal operator functions for timevals
inline timeval &normalizedTimeval(timeval &t)
{
    while (t.tv_usec > 1000000l) {
        ++t.tv_sec;
        t.tv_usec -= 1000000l;
    }
    while (t.tv_usec < 0l) {
        --t.tv_sec;
        t.tv_usec += 1000000l;
    }
    return t;
}
inline bool operator<(const timeval &t1, const timeval &t2)
{ return t1.tv_sec < t2.tv_sec || (t1.tv_sec == t2.tv_sec && t1.tv_usec < t2.tv_usec); }
inline bool operator==(const timeval &t1, const timeval &t2)
{ return t1.tv_sec == t2.tv_sec && t1.tv_usec == t2.tv_usec; }
inline timeval &operator+=(timeval &t1, const timeval &t2)
{
    t1.tv_sec += t2.tv_sec;
    t1.tv_usec += t2.tv_usec;
    return normalizedTimeval(t1);
}
inline timeval operator+(const timeval &t1, const timeval &t2)
{
    timeval tmp;
    tmp.tv_sec = t1.tv_sec + t2.tv_sec;
    tmp.tv_usec = t1.tv_usec + t2.tv_usec;
    return normalizedTimeval(tmp);
}
inline timeval operator-(const timeval &t1, const timeval &t2)
{
    timeval tmp;
    tmp.tv_sec = t1.tv_sec - (t2.tv_sec - 1);
    tmp.tv_usec = t1.tv_usec - (t2.tv_usec + 1000000);
    return normalizedTimeval(tmp);
}
inline timeval operator*(const timeval &t1, int mul)
{
    timeval tmp;
    tmp.tv_sec = t1.tv_sec * mul;
    tmp.tv_usec = t1.tv_usec * mul;
    return normalizedTimeval(tmp);
}

// don't call QT_OPEN or ::open
// call qt_safe_open
static inline int qt_safe_open(const char *pathname, int flags, mode_t mode = 0777)
{
#ifdef O_CLOEXEC
    flags |= O_CLOEXEC;
#endif
    register int fd;
    EINTR_LOOP(fd, QT_OPEN(pathname, flags, mode));

    // unknown flags are ignored, so we have no way of verifying if
    // O_CLOEXEC was accepted
    if (fd != -1)
        ::fcntl(fd, F_SETFD, FD_CLOEXEC);
    return fd;
}
#undef QT_OPEN
#define QT_OPEN         qt_safe_open

#ifndef Q_OS_VXWORKS // no POSIX pipes in VxWorks
// don't call ::pipe
// call qt_safe_pipe
static inline int qt_safe_pipe(int pipefd[2], int flags = 0)
{
#ifdef O_CLOEXEC
    Q_ASSERT((flags & ~(O_CLOEXEC | O_NONBLOCK)) == 0);
#else
    Q_ASSERT((flags & ~O_NONBLOCK) == 0);
#endif

    register int ret;
#if QT_UNIX_SUPPORTS_THREADSAFE_CLOEXEC && defined(O_CLOEXEC)
    // use pipe2
    flags |= O_CLOEXEC;
    ret = ::pipe2(pipefd, flags); // pipe2 is Linux-specific and is documented not to return EINTR
    if (ret == 0 || errno != ENOSYS)
        return ret;
#endif

    ret = ::pipe(pipefd);
    if (ret == -1)
        return -1;

    ::fcntl(pipefd[0], F_SETFD, FD_CLOEXEC);
    ::fcntl(pipefd[1], F_SETFD, FD_CLOEXEC);

    // set non-block too?
    if (flags & O_NONBLOCK) {
        ::fcntl(pipefd[0], F_SETFL, ::fcntl(pipefd[0], F_GETFL) | O_NONBLOCK);
        ::fcntl(pipefd[1], F_SETFL, ::fcntl(pipefd[1], F_GETFL) | O_NONBLOCK);
    }

    return 0;
}

#endif // Q_OS_VXWORKS

// don't call dup or fcntl(F_DUPFD)
static inline int qt_safe_dup(int oldfd, int atleast = 0, int flags = FD_CLOEXEC)
{
    Q_ASSERT(flags == FD_CLOEXEC || flags == 0);

    register int ret;
#ifdef F_DUPFD_CLOEXEC
    // use this fcntl
    if (flags & FD_CLOEXEC) {
        ret = ::fcntl(oldfd, F_DUPFD_CLOEXEC, atleast);
        if (ret != -1 || errno != EINVAL)
            return ret;
    }
#endif

    // use F_DUPFD
    ret = ::fcntl(oldfd, F_DUPFD, atleast);

    if (flags && ret != -1)
        ::fcntl(ret, F_SETFD, flags);
    return ret;
}

// don't call dup2
// call qt_safe_dup2
static inline int qt_safe_dup2(int oldfd, int newfd, int flags = FD_CLOEXEC)
{
    Q_ASSERT(flags == FD_CLOEXEC || flags == 0);

    register int ret;
#if QT_UNIX_SUPPORTS_THREADSAFE_CLOEXEC && defined(O_CLOEXEC)
    // use dup3
    if (flags & FD_CLOEXEC) {
        EINTR_LOOP(ret, ::dup3(oldfd, newfd, O_CLOEXEC));
        if (ret == 0 || errno != ENOSYS)
            return ret;
    }
#endif
    EINTR_LOOP(ret, ::dup2(oldfd, newfd));
    if (ret == -1)
        return -1;

    if (flags)
        ::fcntl(newfd, F_SETFD, flags);
    return 0;
}

static inline qint64 qt_safe_read(int fd, void *data, qint64 maxlen)
{
    qint64 ret = 0;
    EINTR_LOOP(ret, QT_READ(fd, data, maxlen));
    return ret;
}
#undef QT_READ
#define QT_READ qt_safe_read

static inline qint64 qt_safe_write(int fd, const void *data, qint64 len)
{
    qint64 ret = 0;
    EINTR_LOOP(ret, QT_WRITE(fd, data, len));
    return ret;
}
#undef QT_WRITE
#define QT_WRITE qt_safe_write

static inline int qt_safe_close(int fd)
{
    register int ret;
    EINTR_LOOP(ret, QT_CLOSE(fd));
    return ret;
}
#undef QT_CLOSE
#define QT_CLOSE qt_safe_close

// - Open C does not (yet?) implement these on Symbian OS
// - VxWorks doesn't have processes
#if !defined(Q_OS_SYMBIAN) && !defined(Q_OS_VXWORKS)
static inline int qt_safe_execve(const char *filename, char *const argv[],
                                 char *const envp[])
{
    register int ret;
    EINTR_LOOP(ret, ::execve(filename, argv, envp));
    return ret;
}

static inline int qt_safe_execv(const char *path, char *const argv[])
{
    register int ret;
    EINTR_LOOP(ret, ::execv(path, argv));
    return ret;
}

static inline int qt_safe_execvp(const char *file, char *const argv[])
{
    register int ret;
    EINTR_LOOP(ret, ::execvp(file, argv));
    return ret;
}
#endif

#ifndef Q_OS_VXWORKS // no processes on VxWorks
static inline pid_t qt_safe_waitpid(pid_t pid, int *status, int options)
{
    register int ret;
    EINTR_LOOP(ret, ::waitpid(pid, status, options));
    return ret;
}

#endif // Q_OS_VXWORKS

#if !defined(_POSIX_MONOTONIC_CLOCK)
#  define _POSIX_MONOTONIC_CLOCK -1
#endif

timeval qt_gettime(); // in qelapsedtimer_mac.cpp or qtimestamp_unix.cpp

Q_CORE_EXPORT int qt_safe_select(int nfds, fd_set *fdread, fd_set *fdwrite, fd_set *fdexcept,
                                 const struct timeval *tv);

// according to X/OPEN we have to define semun ourselves
// we use prefix as on some systems sem.h will have it
struct semid_ds;
union qt_semun {
    int val;                    /* value for SETVAL */
    struct semid_ds *buf;       /* buffer for IPC_STAT, IPC_SET */
    unsigned short *array;      /* array for GETALL, SETALL */
};

QT_END_NAMESPACE

#endif
