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

#include <QtCore/qatomic.h>

#include <limits.h>
#include <sched.h>

extern "C" {

int q_atomic_trylock_int(volatile int *addr);
int q_atomic_trylock_ptr(volatile void *addr);

Q_CORE_EXPORT int q_atomic_lock_int(volatile int *addr)
{
    int returnValue = q_atomic_trylock_int(addr);

    if (returnValue == INT_MIN) {
        do {
            // spin until we think we can succeed
            do {
                sched_yield();
                returnValue = *addr;
            } while (returnValue == INT_MIN);
            
            // try again
            returnValue = q_atomic_trylock_int(addr);
        } while (returnValue == INT_MIN);
    }

    return returnValue;
}

Q_CORE_EXPORT int q_atomic_lock_ptr(volatile void *addr)
{
    int returnValue = q_atomic_trylock_ptr(addr);

    if (returnValue == -1) {
        do {
            // spin until we think we can succeed
            do {
                sched_yield();
                returnValue = *reinterpret_cast<volatile int *>(addr);
            } while (returnValue == -1);

            // try again
            returnValue = q_atomic_trylock_ptr(addr);
        } while (returnValue == -1);
    }

    return returnValue;
}

} // extern "C"
