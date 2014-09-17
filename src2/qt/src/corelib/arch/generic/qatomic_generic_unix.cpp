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

#if !defined(Q_OS_SYMBIAN) || (defined(Q_OS_SYMBIAN) && !defined(Q_CC_RVCT))

#include "qplatformdefs.h"

#include <QtCore/qatomic.h>

QT_BEGIN_NAMESPACE
static pthread_mutex_t qAtomicMutex = PTHREAD_MUTEX_INITIALIZER;

Q_CORE_EXPORT
bool QBasicAtomicInt_testAndSetOrdered(volatile int *_q_value, int expectedValue, int newValue)
{
    bool returnValue = false;
    pthread_mutex_lock(&qAtomicMutex);
    if (*_q_value == expectedValue) {
        *_q_value = newValue;
        returnValue = true;
    }
    pthread_mutex_unlock(&qAtomicMutex);
    return returnValue;
}

Q_CORE_EXPORT
int QBasicAtomicInt_fetchAndStoreOrdered(volatile int *_q_value, int newValue)
{
    int returnValue;
    pthread_mutex_lock(&qAtomicMutex);
    returnValue = *_q_value;
    *_q_value = newValue;
    pthread_mutex_unlock(&qAtomicMutex);
    return returnValue;
}

Q_CORE_EXPORT
int QBasicAtomicInt_fetchAndAddOrdered(volatile int *_q_value, int valueToAdd)
{
    int returnValue;
    pthread_mutex_lock(&qAtomicMutex);
    returnValue = *_q_value;
    *_q_value += valueToAdd;
    pthread_mutex_unlock(&qAtomicMutex);
    return returnValue;
}

Q_CORE_EXPORT
bool QBasicAtomicPointer_testAndSetOrdered(void * volatile *_q_value,
                                           void *expectedValue,
                                           void *newValue)
{
    bool returnValue = false;
    pthread_mutex_lock(&qAtomicMutex);
    if (*_q_value == expectedValue) {
        *_q_value = newValue;
        returnValue = true;
    }
    pthread_mutex_unlock(&qAtomicMutex);
    return returnValue;
}

Q_CORE_EXPORT
void *QBasicAtomicPointer_fetchAndStoreOrdered(void * volatile *_q_value, void *newValue)
{
    void *returnValue;
    pthread_mutex_lock(&qAtomicMutex);
    returnValue = *_q_value;
    *_q_value = newValue;
    pthread_mutex_unlock(&qAtomicMutex);
    return returnValue;
}

Q_CORE_EXPORT
void *QBasicAtomicPointer_fetchAndAddOrdered(void * volatile *_q_value, qptrdiff valueToAdd)
{
    void *returnValue;
    pthread_mutex_lock(&qAtomicMutex);
    returnValue = *_q_value;
    *_q_value = reinterpret_cast<char *>(returnValue) + valueToAdd;
    pthread_mutex_unlock(&qAtomicMutex);
    return returnValue;
}
QT_END_NAMESPACE
#endif //!defined(Q_OS_SYMBIAN) && !defined(Q_CC_RVCT)
