/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the qmake spec of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia. For licensing terms and
** conditions see http://qt.digia.com/licensing. For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights. These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QPLATFORMDEFS_H
#define QPLATFORMDEFS_H

// Get Qt defines/settings

#include "qglobal.h"
#include "qfunctions_vxworks.h"

#define QT_USE_XOPEN_LFS_EXTENSIONS
#include "../../common/posix/qplatformdefs.h"

#undef QT_LSTAT
#undef QT_MKDIR
#undef QT_READ
#undef QT_WRITE
#undef QT_SOCKLEN_T
#undef QT_SOCKET_CONNECT

#define QT_LSTAT                QT_STAT
#define QT_MKDIR(dir, perm)     ::mkdir(dir)

#define QT_READ(fd, buf, len)   ::read(fd, (char*) buf, len)
#define QT_WRITE(fd, buf, len)  ::write(fd, (char*) buf, len)

// there IS a socklen_t in sys/socket.h (unsigned int),
// but sockLib.h uses int in all function declaration...
#define QT_SOCKLEN_T            int
#define QT_SOCKET_CONNECT(sd, to, tolen) \
                                ::connect(sd, (struct sockaddr *) to, tolen)

#define QT_SNPRINTF             ::snprintf
#define QT_VSNPRINTF            ::vsnprintf

#endif // QPLATFORMDEFS_H
