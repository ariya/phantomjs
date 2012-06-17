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

#ifndef QORDEREDMUTEXLOCKER_P_H
#define QORDEREDMUTEXLOCKER_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

QT_BEGIN_NAMESPACE

#include <QtCore/qmutex.h>

/*
  Locks 2 mutexes in a defined order, avoiding a recursive lock if
  we're trying to lock the same mutex twice.
*/
class QOrderedMutexLocker
{
public:
    QOrderedMutexLocker(QMutex *m1, QMutex *m2)
        : mtx1((m1 == m2) ? m1 : (m1 < m2 ? m1 : m2)),
          mtx2((m1 == m2) ?  0 : (m1 < m2 ? m2 : m1)),
          locked(false)
    {
        relock();
    }
    ~QOrderedMutexLocker()
    {
        unlock();
    }

    void relock()
    {
        if (!locked) {
            if (mtx1) mtx1->lockInline();
            if (mtx2) mtx2->lockInline();
            locked = true;
        }
    }

    void unlock()
    {
        if (locked) {
            if (mtx1) mtx1->unlockInline();
            if (mtx2) mtx2->unlockInline();
            locked = false;
        }
    }

    static bool relock(QMutex *mtx1, QMutex *mtx2)
    {
        // mtx1 is already locked, mtx2 not... do we need to unlock and relock?
        if (mtx1 == mtx2)
            return false;
        if (mtx1 < mtx2) {
            mtx2->lockInline();
            return true;
        }
        if (!mtx2->tryLockInline()) {
            mtx1->unlock();
            mtx2->lock();
            mtx1->lock();
        }
        return true;
    }

private:
    QMutex *mtx1, *mtx2;
    bool locked;
};

QT_END_NAMESPACE

#endif
