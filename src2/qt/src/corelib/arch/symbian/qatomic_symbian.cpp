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
#include <QtCore/qatomic.h>

#ifdef SYMBIAN_E32_ATOMIC_API
#include <e32atomics.h>
#endif

#include <e32debug.h>

QT_BEGIN_NAMESPACE

// Heap and handle info printer.
// This way we can report on heap cells and handles that are really not owned by anything which still exists.
// This information can be used to detect whether memory leaks are happening, particularly if these numbers grow as the app is used more.
// This code is placed here as it happens to make it the very last static to be destroyed in a Qt app. The
// reason assumed is that this file appears before any other file declaring static data in the generated
// Symbian MMP file. This particular file was chosen as it is the earliest symbian specific file.
struct QSymbianPrintExitInfo
{
    QSymbianPrintExitInfo()
    {
        RThread().HandleCount(initProcessHandleCount, initThreadHandleCount);
        initCells = User::CountAllocCells();
    }
    ~QSymbianPrintExitInfo()
    {
        RProcess myProc;
        TFullName fullName = myProc.FileName();
        TInt cells = User::CountAllocCells();
        TInt processHandleCount=0;
        TInt threadHandleCount=0;
        RThread().HandleCount(processHandleCount, threadHandleCount);
        RDebug::Print(_L("%S exiting with %d allocated cells, %d handles"),
            &fullName,
            cells - initCells,
            (processHandleCount + threadHandleCount) - (initProcessHandleCount + initThreadHandleCount));
    }
    TInt initCells;
    TInt initProcessHandleCount;
    TInt initThreadHandleCount;
} symbian_printExitInfo;

Q_CORE_EXPORT bool QBasicAtomicInt::isReferenceCountingNative()
{
#if !defined(SYMBIAN_E32_ATOMIC_API) && defined(QT_HAVE_ARMV6)
    return true;
#else
    return false;
#endif
}

Q_CORE_EXPORT bool QBasicAtomicInt::isTestAndSetNative()
{
#if !defined(SYMBIAN_E32_ATOMIC_API) && defined(QT_HAVE_ARMV6)
    return true;
#else
    return false;
#endif
}

Q_CORE_EXPORT bool QBasicAtomicInt::isFetchAndStoreNative()
{
#if !defined(SYMBIAN_E32_ATOMIC_API) && defined(QT_HAVE_ARMV6)
    return true;
#else
    return false;
#endif
}

Q_CORE_EXPORT bool QBasicAtomicInt::isFetchAndAddNative()
{
#if !defined(SYMBIAN_E32_ATOMIC_API) && defined(QT_HAVE_ARMV6)
    return true;
#else
    return false;
#endif
}

Q_CORE_EXPORT bool QBasicAtomicPointer_isTestAndSetNative()
{
#if !defined(SYMBIAN_E32_ATOMIC_API) && defined(QT_HAVE_ARMV6)
    return true;
#else
    return false;
#endif
}

Q_CORE_EXPORT bool QBasicAtomicPointer_isFetchAndStoreNative()
{
#if !defined(SYMBIAN_E32_ATOMIC_API) && defined(QT_HAVE_ARMV6)
    return true;
#else
    return false;
#endif
}

Q_CORE_EXPORT bool QBasicAtomicPointer_isFetchAndAddNative()
{
#if !defined(SYMBIAN_E32_ATOMIC_API) && defined(QT_HAVE_ARMV6)
    return true;
#else
    return false;
#endif
}

#ifdef SYMBIAN_E32_ATOMIC_API
//Symbian's API is SMP-safe when using SMP kernel, and cheap when using uniprocessor kernel

//generate compiler error if casting assumptions are wrong (symbian64?)
__ASSERT_COMPILE(sizeof(int) == sizeof(TUint32));
__ASSERT_COMPILE(sizeof(void *) == sizeof(TUint32));

Q_CORE_EXPORT
bool QBasicAtomicInt_testAndSetOrdered(volatile int *_q_value, int expectedValue, int newValue)
{
    return static_cast<bool>(__e32_atomic_cas_ord32(_q_value,
        reinterpret_cast<TUint32*>(&expectedValue), newValue));
}

Q_CORE_EXPORT
bool QBasicAtomicInt_testAndSetRelaxed(volatile int *_q_value, int expectedValue, int newValue)
{
    return static_cast<bool>(__e32_atomic_cas_rlx32(_q_value,
        reinterpret_cast<TUint32*>(&expectedValue), newValue));
}

Q_CORE_EXPORT
bool QBasicAtomicInt_testAndSetAcquire(volatile int *_q_value, int expectedValue, int newValue)
{
    return static_cast<bool>(__e32_atomic_cas_acq32(_q_value,
        reinterpret_cast<TUint32*>(&expectedValue), newValue));
}

Q_CORE_EXPORT
bool QBasicAtomicInt_testAndSetRelease(volatile int *_q_value, int expectedValue, int newValue)
{
    return static_cast<bool>(__e32_atomic_cas_rel32(_q_value,
        reinterpret_cast<TUint32*>(&expectedValue), newValue));
}

Q_CORE_EXPORT
int QBasicAtomicInt_fetchAndStoreOrdered(volatile int *_q_value, int newValue)
{
    return static_cast<int>(__e32_atomic_swp_ord32(_q_value, newValue));
}

Q_CORE_EXPORT
int QBasicAtomicInt_fetchAndStoreRelaxed(volatile int *_q_value, int newValue)
{
    return static_cast<int>(__e32_atomic_swp_rlx32(_q_value, newValue));
}

Q_CORE_EXPORT
int QBasicAtomicInt_fetchAndStoreAcquire(volatile int *_q_value, int newValue)
{
    return static_cast<int>(__e32_atomic_swp_acq32(_q_value, newValue));
}

Q_CORE_EXPORT
int QBasicAtomicInt_fetchAndStoreRelease(volatile int *_q_value, int newValue)
{
    return static_cast<int>(__e32_atomic_swp_rel32(_q_value, newValue));
}

Q_CORE_EXPORT
int QBasicAtomicInt_fetchAndAddOrdered(volatile int *_q_value, int valueToAdd)
{
    return static_cast<int>(__e32_atomic_add_ord32(_q_value, valueToAdd));
}

Q_CORE_EXPORT
int QBasicAtomicInt_fetchAndAddRelaxed(volatile int *_q_value, int valueToAdd)
{
    return static_cast<int>(__e32_atomic_add_rlx32(_q_value, valueToAdd));
}

Q_CORE_EXPORT
int QBasicAtomicInt_fetchAndAddAcquire(volatile int *_q_value, int valueToAdd)
{
    return static_cast<int>(__e32_atomic_add_acq32(_q_value, valueToAdd));
}

Q_CORE_EXPORT
int QBasicAtomicInt_fetchAndAddRelease(volatile int *_q_value, int valueToAdd)
{
    return static_cast<int>(__e32_atomic_add_rel32(_q_value, valueToAdd));
}

Q_CORE_EXPORT
bool QBasicAtomicPointer_testAndSetOrdered(void * volatile *_q_value,
                                           void *expectedValue,
                                           void *newValue)
{
    return static_cast<bool>(__e32_atomic_cas_ord_ptr(_q_value,
        &expectedValue,
        newValue));
}

Q_CORE_EXPORT
bool QBasicAtomicPointer_testAndSetRelaxed(void * volatile *_q_value,
                                           void *expectedValue,
                                           void *newValue)
{
    return static_cast<bool>(__e32_atomic_cas_rlx_ptr(_q_value,
        &expectedValue,
        newValue));
}

Q_CORE_EXPORT
bool QBasicAtomicPointer_testAndSetAcquire(void * volatile *_q_value,
                                           void *expectedValue,
                                           void *newValue)
{
    return static_cast<bool>(__e32_atomic_cas_acq_ptr(_q_value,
        &expectedValue,
        newValue));
}

Q_CORE_EXPORT
bool QBasicAtomicPointer_testAndSetRelease(void * volatile *_q_value,
                                           void *expectedValue,
                                           void *newValue)
{
    return static_cast<bool>(__e32_atomic_cas_rel_ptr(_q_value,
        &expectedValue,
        newValue));
}

Q_CORE_EXPORT
void *QBasicAtomicPointer_fetchAndStoreOrdered(void * volatile *_q_value, void *newValue)
{
    return __e32_atomic_swp_ord_ptr(_q_value, newValue);
}

Q_CORE_EXPORT
void *QBasicAtomicPointer_fetchAndStoreRelaxed(void * volatile *_q_value, void *newValue)
{
    return __e32_atomic_swp_rlx_ptr(_q_value, newValue);
}

Q_CORE_EXPORT
void *QBasicAtomicPointer_fetchAndStoreAcquire(void * volatile *_q_value, void *newValue)
{
    return __e32_atomic_swp_acq_ptr(_q_value, newValue);
}

Q_CORE_EXPORT
void *QBasicAtomicPointer_fetchAndStoreRelease(void * volatile *_q_value, void *newValue)
{
    return __e32_atomic_swp_rel_ptr(_q_value, newValue);
}

Q_CORE_EXPORT
void *QBasicAtomicPointer_fetchAndAddOrdered(void * volatile *_q_value, qptrdiff valueToAdd)
{
    return __e32_atomic_add_ord_ptr(_q_value, valueToAdd);
}

Q_CORE_EXPORT
void *QBasicAtomicPointer_fetchAndAddRelaxed(void * volatile *_q_value, qptrdiff valueToAdd)
{
    return __e32_atomic_add_rlx_ptr(_q_value, valueToAdd);
}

Q_CORE_EXPORT
void *QBasicAtomicPointer_fetchAndAddAcquire(void * volatile *_q_value, qptrdiff valueToAdd)
{
    return __e32_atomic_add_acq_ptr(_q_value, valueToAdd);
}

Q_CORE_EXPORT
void *QBasicAtomicPointer_fetchAndAddRelease(void * volatile *_q_value, qptrdiff valueToAdd)
{
    return __e32_atomic_add_rel_ptr(_q_value, valueToAdd);
}

#else
//Symbian kernels 9.4 and earlier don't expose a suitable API

//For ARMv6, the generic atomics are machine coded
#ifndef QT_HAVE_ARMV6

class QCriticalSection
{
public:
        QCriticalSection()  { fastlock.CreateLocal(); }
        ~QCriticalSection() { fastlock.Close(); }
        void lock()         { fastlock.Wait(); }
        void unlock()       { fastlock.Signal(); }

private:
        RFastLock fastlock;
};

QCriticalSection qAtomicCriticalSection;

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

Q_CORE_EXPORT
bool QBasicAtomicInt_testAndSetRelaxed(volatile int *_q_value, int expectedValue, int newValue)
{
    return QBasicAtomicInt_testAndSetOrdered(_q_value, expectedValue, newValue);
}

Q_CORE_EXPORT
bool QBasicAtomicInt_testAndSetAcquire(volatile int *_q_value, int expectedValue, int newValue)
{
    return QBasicAtomicInt_testAndSetOrdered(_q_value, expectedValue, newValue);
}

Q_CORE_EXPORT
bool QBasicAtomicInt_testAndSetRelease(volatile int *_q_value, int expectedValue, int newValue)
{
    return QBasicAtomicInt_testAndSetOrdered(_q_value, expectedValue, newValue);
}

Q_CORE_EXPORT
int QBasicAtomicInt_fetchAndStoreRelaxed(volatile int *_q_value, int newValue)
{
    return QBasicAtomicInt_fetchAndStoreOrdered(_q_value, newValue);
}

Q_CORE_EXPORT
int QBasicAtomicInt_fetchAndStoreAcquire(volatile int *_q_value, int newValue)
{
    return QBasicAtomicInt_fetchAndStoreOrdered(_q_value, newValue);
}

Q_CORE_EXPORT
int QBasicAtomicInt_fetchAndStoreRelease(volatile int *_q_value, int newValue)
{
    return QBasicAtomicInt_fetchAndStoreOrdered(_q_value, newValue);
}

Q_CORE_EXPORT
int QBasicAtomicInt_fetchAndAddRelaxed(volatile int *_q_value, int valueToAdd)
{
    return QBasicAtomicInt_fetchAndAddOrdered(_q_value, valueToAdd);
}

Q_CORE_EXPORT
int QBasicAtomicInt_fetchAndAddAcquire(volatile int *_q_value, int valueToAdd)
{
    return QBasicAtomicInt_fetchAndAddOrdered(_q_value, valueToAdd);
}

Q_CORE_EXPORT
int QBasicAtomicInt_fetchAndAddRelease(volatile int *_q_value, int valueToAdd)
{
    return QBasicAtomicInt_fetchAndAddOrdered(_q_value, valueToAdd);
}

Q_CORE_EXPORT
bool QBasicAtomicPointer_testAndSetRelaxed(void * volatile *_q_value,
                                           void *expectedValue,
                                           void *newValue)
{
    return QBasicAtomicPointer_testAndSetOrdered(_q_value, expectedValue, newValue);
}

Q_CORE_EXPORT
bool QBasicAtomicPointer_testAndSetAcquire(void * volatile *_q_value,
                                           void *expectedValue,
                                           void *newValue)
{
    return QBasicAtomicPointer_testAndSetOrdered(_q_value, expectedValue, newValue);
}

Q_CORE_EXPORT
bool QBasicAtomicPointer_testAndSetRelease(void * volatile *_q_value,
                                           void *expectedValue,
                                           void *newValue)
{
    return QBasicAtomicPointer_testAndSetOrdered(_q_value, expectedValue, newValue);
}

Q_CORE_EXPORT
void *QBasicAtomicPointer_fetchAndStoreRelaxed(void * volatile *_q_value, void *newValue)
{
    return QBasicAtomicPointer_fetchAndStoreOrdered(_q_value, newValue);
}

Q_CORE_EXPORT
void *QBasicAtomicPointer_fetchAndStoreAcquire(void * volatile *_q_value, void *newValue)
{
    return QBasicAtomicPointer_fetchAndStoreOrdered(_q_value, newValue);
}

Q_CORE_EXPORT
void *QBasicAtomicPointer_fetchAndStoreRelease(void * volatile *_q_value, void *newValue)
{
    return QBasicAtomicPointer_fetchAndStoreOrdered(_q_value, newValue);
}

Q_CORE_EXPORT
void *QBasicAtomicPointer_fetchAndAddRelaxed(void * volatile *_q_value, qptrdiff valueToAdd)
{
    return QBasicAtomicPointer_fetchAndAddOrdered(_q_value, valueToAdd);
}

Q_CORE_EXPORT
void *QBasicAtomicPointer_fetchAndAddAcquire(void * volatile *_q_value, qptrdiff valueToAdd)
{
    return QBasicAtomicPointer_fetchAndAddOrdered(_q_value, valueToAdd);
}

Q_CORE_EXPORT
void *QBasicAtomicPointer_fetchAndAddRelease(void * volatile *_q_value, qptrdiff valueToAdd)
{
    return QBasicAtomicPointer_fetchAndAddOrdered(_q_value, valueToAdd);
}

#endif // QT_HAVE_ARMV6
#endif // SYMBIAN_E32_ATOMIC_API

QT_END_NAMESPACE
