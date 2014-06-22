// Copyright (c) 2005, 2006, Google Inc.
// Copyright (c) 2010, Patrick Gansterer <paroga@paroga.com>
// All rights reserved.
// 
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
// 
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

// ---
// Author: Sanjay Ghemawat <opensource@google.com>

#ifndef TCMALLOC_INTERNAL_SPINLOCK_H__
#define TCMALLOC_INTERNAL_SPINLOCK_H__

#include <wtf/Atomics.h>
#if OS(UNIX)
#include <sched.h>
#endif

#if ENABLE(COMPARE_AND_SWAP)

static void TCMalloc_SlowLock(unsigned* lockword);

// The following is a struct so that it can be initialized at compile time
struct TCMalloc_SpinLock {
    void Lock() {
      if (!WTF::weakCompareAndSwap(&lockword_, 0, 1))
        TCMalloc_SlowLock(&lockword_);
      WTF::memoryBarrierAfterLock();
    }

    void Unlock() {
      WTF::memoryBarrierBeforeUnlock();
      lockword_ = 0;
    }

    // Report if we think the lock can be held by this thread.
    // When the lock is truly held by the invoking thread
    // we will always return true.
    // Indended to be used as CHECK(lock.IsHeld());
    bool IsHeld() const {
        return lockword_ != 0;
    }

    void Init() { lockword_ = 0; }
    void Finalize() { }

    unsigned lockword_;
};

#define SPINLOCK_INITIALIZER { 0 }

static void TCMalloc_SlowLock(unsigned* lockword) {
  do {
#if OS(WINDOWS)
    Sleep(0);
#else
    sched_yield();
#endif
  } while (!WTF::weakCompareAndSwap(lockword, 0, 1));
}

#else

#include <pthread.h>

// Portable version
struct TCMalloc_SpinLock {
  pthread_mutex_t private_lock_;

  inline void Init() {
    if (pthread_mutex_init(&private_lock_, NULL) != 0) CRASH();
  }
  inline void Finalize() {
    if (pthread_mutex_destroy(&private_lock_) != 0) CRASH();
  }
  inline void Lock() {
    if (pthread_mutex_lock(&private_lock_) != 0) CRASH();
  }
  inline void Unlock() {
    if (pthread_mutex_unlock(&private_lock_) != 0) CRASH();
  }
  bool IsHeld() {
    if (pthread_mutex_trylock(&private_lock_))
      return true;

    Unlock();
    return false;
  }
};

#define SPINLOCK_INITIALIZER { PTHREAD_MUTEX_INITIALIZER }

#endif

// Corresponding locker object that arranges to acquire a spinlock for
// the duration of a C++ scope.
class TCMalloc_SpinLockHolder {
 private:
  TCMalloc_SpinLock* lock_;
 public:
  inline explicit TCMalloc_SpinLockHolder(TCMalloc_SpinLock* l)
    : lock_(l) { l->Lock(); }
  inline ~TCMalloc_SpinLockHolder() { lock_->Unlock(); }
};

// Short-hands for convenient use by tcmalloc.cc
typedef TCMalloc_SpinLock SpinLock;
typedef TCMalloc_SpinLockHolder SpinLockHolder;

#endif  // TCMALLOC_INTERNAL_SPINLOCK_H__
