/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the qmake spec of the Qt Toolkit.
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

#ifndef QPLATFORMDEFS_H
#define QPLATFORMDEFS_H

// Get Qt defines/settings
#include <qglobal.h>

// Set any POSIX/XOPEN defines at the top of this file to turn on specific APIs

#include <unistd.h>

// We are hot - unistd.h should have turned on the specific APIs we requested

#include <pthread.h>
#include <dirent.h>
#include <fcntl.h>
#include <grp.h>
#include <pwd.h>
#include <signal.h>

#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/ipc.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <netinet/in.h>
#ifndef QT_NO_IPV6IFNAME
#include <net/if.h>
#endif

// for htonl
#include <arpa/inet.h>

#define QT_USE_XOPEN_LFS_EXTENSIONS
#define QT_NO_READDIR64
#include "../posix/qplatformdefs.h"

#include <stdlib.h>

#define QT_SNPRINTF             ::snprintf
#define QT_VSNPRINTF            ::vsnprintf


#include <sys/neutrino.h>

#if !defined(_NTO_VERSION) || _NTO_VERSION < 650
// pre-6.5 versions of QNX doesn't have getpagesize()
inline int getpagesize()
{
    return ::sysconf(_SC_PAGESIZE);
}

// pre-6.5 versions of QNX doesn't have strtof()
inline float strtof(const char *b, char **e)
{
    return float(strtod(b, e));
}
#endif

#define QT_QWS_TEMP_DIR QString::fromLocal8Bit(qgetenv("TMPDIR").constData())

#endif // QPLATFORMDEFS_H
