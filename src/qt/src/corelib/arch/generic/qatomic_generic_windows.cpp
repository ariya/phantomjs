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

#include "qplatformdefs.h"

#include <QtCore/qatomic.h>

QT_BEGIN_NAMESPACE

class QCriticalSection
{
public:
        QCriticalSection()  { InitializeCriticalSection(&section); }
        ~QCriticalSection() { DeleteCriticalSection(&section); }
        void lock()         { EnterCriticalSection(&section); }
        void unlock()       { LeaveCriticalSection(&section); }

private:
        CRITICAL_SECTION section;
};

static QCriticalSection qAtomicCriticalSection;

Q_CORE_EXPORT
bool QBasicAtomicInt_testAndSetOrdered(volatile int *_q_value, int expectedValue, int newValue)
{
    bool returnValue = false;
    qAtomicCriticalSection.lock();
    if (*_q_value == expectedValue) {
        *_q_value = newValue;
        returnValue = true;
    }
        qAtomicCriticalSection.unlock();
    return returnValue;
}

Q_CORE_EXPORT
int QBasicAtomicInt_fetchAndStoreOrdered(volatile int *_q_value, int newValue)
{
    int returnValue;
        qAtomicCriticalSection.lock();
    returnValue = *_q_value;
    *_q_value = newValue;
        qAtomicCriticalSection.unlock();
    return returnValue;
}

Q_CORE_EXPORT
int QBasicAtomicInt_fetchAndAddOrdered(volatile int *_q_value, int valueToAdd)
{
    int returnValue;
        qAtomicCriticalSection.lock();
    returnValue = *_q_value;
    *_q_value += valueToAdd;
        qAtomicCriticalSection.unlock();
    return returnValue;
}

Q_CORE_EXPORT
bool QBasicAtomicPointer_testAndSetOrdered(void * volatile *_q_value,
                                           void *expectedValue,
                                           void *newValue)
{
    bool returnValue = false;
        qAtomicCriticalSection.lock();
    if (*_q_value == expectedValue) {
        *_q_value = newValue;
        returnValue = true;
    }
        qAtomicCriticalSection.unlock();
    return returnValue;
}

Q_CORE_EXPORT
void *QBasicAtomicPointer_fetchAndStoreOrdered(void * volatile *_q_value, void *newValue)
{
    void *returnValue;
        qAtomicCriticalSection.lock();
    returnValue = *_q_value;
    *_q_value = newValue;
        qAtomicCriticalSection.unlock();
    return returnValue;
}

Q_CORE_EXPORT
void *QBasicAtomicPointer_fetchAndAddOrdered(void * volatile *_q_value, qptrdiff valueToAdd)
{
    void *returnValue;
        qAtomicCriticalSection.lock();
    returnValue = *_q_value;
    *_q_value = reinterpret_cast<char *>(returnValue) + valueToAdd;
        qAtomicCriticalSection.unlock();
    return returnValue;
}

QT_END_NAMESPACE
