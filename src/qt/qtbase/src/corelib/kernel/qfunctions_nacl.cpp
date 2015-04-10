/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
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

#include "qfunctions_nacl.h"
#include <pthread.h>
#include <qglobal.h>

/*
    The purpose of this file is to stub out certain functions
    that are not provided by the Native Client SDK. This is
    done as an alterative to sprinkling the Qt sources with
    NACL ifdefs.

    There are two main classes of functions:

    - Functions that are called but can have no effect:
    For these we simply give an empty implementation

    - Functions that are referenced in the source code, but
    is not/must not be called at run-time:
    These we either leave undefined or implement with a
    qFatal.

    This is a work in progress.
*/

extern "C" {

void pthread_cleanup_push(void (*)(void *), void *)
{

}

void pthread_cleanup_pop(int)
{

}

int pthread_setcancelstate(int, int *)
{
    return 0;
}

int pthread_setcanceltype(int, int *)
{
    return 0;
}

void pthread_testcancel(void)
{

}


int pthread_cancel(pthread_t)
{
    return 0;
}

int pthread_attr_setinheritsched(pthread_attr_t *,int)
{
    return 0;
}


int pthread_attr_getinheritsched(const pthread_attr_t *, int *)
{
    return 0;
}

// event dispatcher, select
//struct fd_set;
//struct timeval;

int fcntl(int, int, ...)
{
    return 0;
}

int sigaction(int, const struct sigaction *, struct sigaction *)
{
    return 0;
}

int open(const char *, int, ...)
{
    return 0;
}

int open64(const char *, int, ...)
{
    return 0;
}

int access(const char *, int)
{
    return 0;
}

typedef long off64_t;
off64_t ftello64(void *)
{
    qFatal("ftello64 called");
    return 0;
}

off64_t lseek64(int, off_t, int)
{
    qFatal("lseek64 called");
    return 0;
}

} // Extern C

int select(int, fd_set *, fd_set *, fd_set *, struct timeval *)
{
    return 0;
}
