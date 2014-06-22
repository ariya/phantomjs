/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the qmake spec of the Qt Toolkit.
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

#ifndef QPLATFORMDEFS_H
#define QPLATFORMDEFS_H

// Get Qt defines/settings

#include "qglobal.h"

// Set any POSIX/XOPEN defines at the top of this file to turn on specific APIs

#include <unistd.h>


// We are hot - unistd.h should have turned on the specific APIs we requested


#include <pthread.h>
#include <dirent.h>
#include <fcntl.h>
#include <grp.h>
#include <pwd.h>
#include <signal.h>
#include <dlfcn.h>

#include <sys/param.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/ipc.h>
#include <sys/time.h>
#include <sys/shm.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <netinet/in.h>
#ifndef QT_NO_IPV6IFNAME
#include <net/if.h>
#endif

#include "../common/posix/qplatformdefs.h"

#undef QT_OPEN_LARGEFILE
#define QT_OPEN_LARGEFILE       0

// QT_SOCKLEN_T
// OpenBSD 2.2 - 2.4            int
// OpenBSD 2.5 - 2.8            socklen_t

#define QT_SNPRINTF             ::snprintf
#define QT_VSNPRINTF            ::vsnprintf

// 1003.1c-1995 says on page 38 (2.9.3, paragraph 3) that if _POSIX_THREADS
// is defined, then _POSIX_THREAD_SAFE_FUNCTIONS must also be defined.
// However this looks like a well-known typo (reversed dependency).
//
// On the other hand _POSIX_THREAD_SAFE_FUNCTIONS should be defined only
// if the Thread-Safe Functions option is implemented. OpenBSD does not
// support all of the required _r() interfaces, especially getpwuid_r(),
// which means it should not define _POSIX_THREAD_SAFE_FUNCTIONS.
//
// Since OpenBSD does define _POSIX_THREAD_SAFE_FUNCTIONS, we have to
// undefine it behind its back.
#ifdef _POSIX_THREAD_SAFE_FUNCTIONS
#undef _POSIX_THREAD_SAFE_FUNCTIONS
#endif

// Older OpenBSD versions may still use the a.out format instead of ELF.
#ifndef __ELF__
#define QT_AOUT_UNDERSCORE
#endif

#endif // QPLATFORMDEFS_H
