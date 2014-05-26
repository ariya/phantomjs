/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QFUNCTIONS_VXWORKS_H
#define QFUNCTIONS_VXWORKS_H
#ifdef Q_OS_VXWORKS

#include <unistd.h>
#include <pthread.h>
#include <dirent.h>
#include <signal.h>
#include <string.h>
#include <strings.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#if defined(_WRS_KERNEL)
#include <sys/times.h>
#else
#include <sys/time.h>
#endif
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <netinet/in.h>
#ifndef QT_NO_IPV6IFNAME
#include <net/if.h>
#endif

// VxWorks has public header mbuf.h which defines following variables for DKM.
// Let's undef those to because they overlap with Qt variable names-
// File mbuf.h is included in headers <netinet/in.h> <net/if.h>, so make sure
// that those are included before undef's.
#if defined(mbuf)
#  undef mbuf
#endif
#if defined(m_data)
#  undef m_data
#endif
#if defined(m_type)
#  undef m_type
#endif
#if defined(m_next)
#  undef m_next
#endif
#if defined(m_len)
#  undef m_len
#endif
#if defined(m_flags)
#  undef m_flags
#endif
#if defined(m_hdr)
#  undef m_hdr
#endif
#if defined(m_ext)
#  undef m_ext
#endif
#if defined(m_act)
#  undef m_act
#endif
#if defined(m_nextpkt)
#  undef m_nextpkt
#endif
#if defined(m_pkthdr)
#  undef m_pkthdr
#endif

QT_BEGIN_HEADER
QT_BEGIN_NAMESPACE

#ifdef QT_BUILD_CORE_LIB
QT_MODULE(Core)
#endif

QT_END_NAMESPACE
QT_END_HEADER


#ifndef RTLD_LOCAL
#define RTLD_LOCAL  0
#endif

#ifndef NSIG
#define NSIG _NSIGS
#endif

#ifdef __cplusplus
extern "C" {
#endif

// isascii is missing (sometimes!!)
#ifndef isascii
inline int isascii(int c)  { return (c & 0x7f); }
#endif

// no lfind() - used by the TIF image format
void *lfind(const void* key, const void* base, size_t* elements, size_t size,
            int (*compare)(const void*, const void*));

// no rand_r(), but rand()
// NOTE: this implementation is wrong for multi threaded applications,
// but there is no way to get it right on VxWorks (in kernel mode)
int rand_r(unsigned int * /*seedp*/);

// no usleep() support
int usleep(unsigned int);

#if defined(VXWORKS_DKM) || defined(VXWORKS_RTP)
int gettimeofday(struct timeval *, void *);
#else
// gettimeofday() is declared, but is missing from the library.
// It IS however defined in the Curtis-Wright X11 libraries, so
// we have to make the symbol 'weak'
int gettimeofday(struct timeval *tv, void /*struct timezone*/ *) __attribute__((weak));
#endif

// getpagesize() not available
int getpagesize();

// symlinks are not supported (lstat is now just a call to stat - see qplatformdefs.h)
int symlink(const char *, const char *);
ssize_t readlink(const char *, char *, size_t);

// there's no truncate(), but ftruncate() support...
int truncate(const char *path, off_t length);

// VxWorks doesn't know about passwd & friends.
// in order to avoid patching the unix fs path everywhere
// we introduce some dummy functions that simulate a single
// 'root' user on the system.

uid_t getuid();
gid_t getgid();
uid_t geteuid();

struct passwd {
    char   *pw_name;       /* user name */
    char   *pw_passwd;     /* user password */
    uid_t   pw_uid;        /* user ID */
    gid_t   pw_gid;        /* group ID */
    char   *pw_gecos;      /* real name */
    char   *pw_dir;        /* home directory */
    char   *pw_shell;      /* shell program */
};

struct group {
    char   *gr_name;       /* group name */
    char   *gr_passwd;     /* group password */
    gid_t   gr_gid;        /* group ID */
    char  **gr_mem;        /* group members */
};

struct passwd *getpwuid(uid_t uid);
struct group *getgrgid(gid_t gid);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // Q_OS_VXWORKS
#endif // QFUNCTIONS_VXWORKS_H
