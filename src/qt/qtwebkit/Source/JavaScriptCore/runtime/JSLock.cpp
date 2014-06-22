/*
 * Copyright (C) 2005, 2008, 2012 Apple Inc. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the NU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA 
 *
 */

#include "config.h"
#include "JSLock.h"

#include "Heap.h"
#include "CallFrame.h"
#include "JSGlobalObject.h"
#include "JSObject.h"
#include "Operations.h"

#if USE(PTHREADS)
#include <pthread.h>
#endif

namespace JSC {

Mutex* GlobalJSLock::s_sharedInstanceLock = 0;

GlobalJSLock::GlobalJSLock()
{
    s_sharedInstanceLock->lock();
}

GlobalJSLock::~GlobalJSLock()
{
    s_sharedInstanceLock->unlock();
}

void GlobalJSLock::initialize()
{
    s_sharedInstanceLock = new Mutex();
}

JSLockHolder::JSLockHolder(ExecState* exec)
    : m_vm(&exec->vm())
{
    init();
}

JSLockHolder::JSLockHolder(VM* vm)
    : m_vm(vm)
{
    init();
}

JSLockHolder::JSLockHolder(VM& vm)
    : m_vm(&vm)
{
    init();
}

void JSLockHolder::init()
{
    m_vm->apiLock().lock();
}

JSLockHolder::~JSLockHolder()
{
    RefPtr<JSLock> apiLock(&m_vm->apiLock());
    m_vm.clear();
    apiLock->unlock();
}

JSLock::JSLock(VM* vm)
    : m_ownerThread(0)
    , m_lockCount(0)
    , m_lockDropDepth(0)
    , m_vm(vm)
{
    m_spinLock.Init();
}

JSLock::~JSLock()
{
}

void JSLock::willDestroyVM(VM* vm)
{
    ASSERT_UNUSED(vm, m_vm == vm);
    m_vm = 0;
}

void JSLock::lock()
{
    ThreadIdentifier currentThread = WTF::currentThread();
    {
        SpinLockHolder holder(&m_spinLock);
        if (m_ownerThread == currentThread && m_lockCount) {
            m_lockCount++;
            return;
        }
    }

    m_lock.lock();

    {
        SpinLockHolder holder(&m_spinLock);
        m_ownerThread = currentThread;
        ASSERT(!m_lockCount);
        m_lockCount = 1;
    }
}

void JSLock::unlock()
{
    ASSERT(currentThreadIsHoldingLock());

    SpinLockHolder holder(&m_spinLock);
    m_lockCount--;

    if (!m_lockCount)
        m_lock.unlock();
}

void JSLock::lock(ExecState* exec)
{
    exec->vm().apiLock().lock();
}

void JSLock::unlock(ExecState* exec)
{
    exec->vm().apiLock().unlock();
}

bool JSLock::currentThreadIsHoldingLock()
{
    return m_lockCount && m_ownerThread == WTF::currentThread();
}

// This is fairly nasty.  We allow multiple threads to run on the same
// context, and we do not require any locking semantics in doing so -
// clients of the API may simply use the context from multiple threads
// concurently, and assume this will work.  In order to make this work,
// We lock the context when a thread enters, and unlock it when it leaves.
// However we do not only unlock when the thread returns from its
// entry point (evaluate script or call function), we also unlock the
// context if the thread leaves JSC by making a call out to an external
// function through a callback.
//
// All threads using the context share the same JS stack (the JSStack).
// Whenever a thread calls into JSC it starts using the JSStack from the
// previous 'high water mark' - the maximum point the stack has ever grown to
// (returned by JSStack::end()).  So if a first thread calls out to a
// callback, and a second thread enters JSC, then also exits by calling out
// to a callback, we can be left with stackframes from both threads in the
// JSStack.  As such, a problem may occur should the first thread's
// callback complete first, and attempt to return to JSC.  Were we to allow
// this to happen, and were its stack to grow further, then it may potentially
// write over the second thread's call frames.
//
// To avoid JS stack corruption we enforce a policy of only ever allowing two
// threads to use a JS context concurrently, and only allowing the second of
// these threads to execute until it has completed and fully returned from its
// outermost call into JSC.  We enforce this policy using 'lockDropDepth'.  The
// first time a thread exits it will call DropAllLocks - which will do as expected
// and drop locks allowing another thread to enter.  Should another thread, or the
// same thread again, enter JSC (through evaluate script or call function), and exit
// again through a callback, then the locks will not be dropped when DropAllLocks
// is called (since lockDropDepth is non-zero).  Since this thread is still holding
// the locks, only it will be able to re-enter JSC (either be returning from the
// callback, or by re-entering through another call to evaulate script or call
// function).
//
// This policy is slightly more restricive than it needs to be for correctness -
// we could validly allow futher entries into JSC from other threads, we only
// need ensure that callbacks return in the reverse chronological order of the
// order in which they were made - though implementing the less restrictive policy
// would likely increase complexity and overhead.
//

// This function returns the number of locks that were dropped.
unsigned JSLock::dropAllLocks()
{
    if (m_lockDropDepth++)
        return 0;

    return dropAllLocksUnconditionally();
}

unsigned JSLock::dropAllLocksUnconditionally()
{
    unsigned lockCount = m_lockCount;
    for (unsigned i = 0; i < lockCount; i++)
        unlock();

    return lockCount;
}

void JSLock::grabAllLocks(unsigned lockCount)
{
    for (unsigned i = 0; i < lockCount; i++)
        lock();

    m_lockDropDepth--;
}

JSLock::DropAllLocks::DropAllLocks(ExecState* exec)
    : m_lockCount(0)
    , m_vm(&exec->vm())
{
    m_lockCount = m_vm->apiLock().dropAllLocks();
}

JSLock::DropAllLocks::DropAllLocks(VM* vm)
    : m_lockCount(0)
    , m_vm(vm)
{
    m_lockCount = m_vm->apiLock().dropAllLocks();
}

JSLock::DropAllLocks::~DropAllLocks()
{
    m_vm->apiLock().grabAllLocks(m_lockCount);
}

} // namespace JSC
