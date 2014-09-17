/****************************************************************************
**
** Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
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

#include <QtCore/qglobal.h>
#include <QtCore/qhash.h>

QT_BEGIN_NAMESPACE

QT_USE_NAMESPACE

#define UNLOCKED    {-1,-1,-1,-1}
#define UNLOCKED2      UNLOCKED,UNLOCKED
#define UNLOCKED4     UNLOCKED2,UNLOCKED2
#define UNLOCKED8     UNLOCKED4,UNLOCKED4
#define UNLOCKED16    UNLOCKED8,UNLOCKED8
#define UNLOCKED32   UNLOCKED16,UNLOCKED16
#define UNLOCKED64   UNLOCKED32,UNLOCKED32
#define UNLOCKED128  UNLOCKED64,UNLOCKED64
#define UNLOCKED256 UNLOCKED128,UNLOCKED128

// use a 4k page for locks
static int locks[256][4] = { UNLOCKED256 };

int *getLock(volatile void *addr)
{ return locks[qHash(const_cast<void *>(addr)) % 256]; }

static int *align16(int *lock)
{
    ulong off = (((ulong) lock) % 16);
    return off ? (int *)(ulong(lock) + 16 - off) : lock;
}

extern "C" {

    int q_ldcw(volatile int *addr);

    void q_atomic_lock(int *lock)
    {
        // ldcw requires a 16-byte aligned address
        volatile int *x = align16(lock);
        while (q_ldcw(x) == 0)
	    ;
    }

    void q_atomic_unlock(int *lock)
    { lock[0] = lock[1] = lock[2] = lock[3] = -1; }
}


QT_END_NAMESPACE
