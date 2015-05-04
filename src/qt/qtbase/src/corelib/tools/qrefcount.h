/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtCore module of the Qt Toolkit.
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

#ifndef QREFCOUNT_H
#define QREFCOUNT_H

#include <QtCore/qatomic.h>

QT_BEGIN_NAMESPACE


namespace QtPrivate
{

class RefCount
{
public:
    inline bool ref() Q_DECL_NOTHROW {
        int count = atomic.load();
#if QT_SUPPORTS(UNSHARABLE_CONTAINERS)
        if (count == 0) // !isSharable
            return false;
#endif
        if (count != -1) // !isStatic
            atomic.ref();
        return true;
    }

    inline bool deref() Q_DECL_NOTHROW {
        int count = atomic.load();
#if QT_SUPPORTS(UNSHARABLE_CONTAINERS)
        if (count == 0) // !isSharable
            return false;
#endif
        if (count == -1) // isStatic
            return true;
        return atomic.deref();
    }

#if QT_SUPPORTS(UNSHARABLE_CONTAINERS)
    bool setSharable(bool sharable) Q_DECL_NOTHROW
    {
        Q_ASSERT(!isShared());
        if (sharable)
            return atomic.testAndSetRelaxed(0, 1);
        else
            return atomic.testAndSetRelaxed(1, 0);
    }

    bool isSharable() const Q_DECL_NOTHROW
    {
        // Sharable === Shared ownership.
        return atomic.load() != 0;
    }
#endif

    bool isStatic() const Q_DECL_NOTHROW
    {
        // Persistent object, never deleted
        return atomic.load() == -1;
    }

    bool isShared() const Q_DECL_NOTHROW
    {
        int count = atomic.load();
        return (count != 1) && (count != 0);
    }

    void initializeOwned() Q_DECL_NOTHROW { atomic.store(1); }
    void initializeUnsharable() Q_DECL_NOTHROW { atomic.store(0); }

    QBasicAtomicInt atomic;
};

}

#define Q_REFCOUNT_INITIALIZE_STATIC { Q_BASIC_ATOMIC_INITIALIZER(-1) }

QT_END_NAMESPACE

#endif
