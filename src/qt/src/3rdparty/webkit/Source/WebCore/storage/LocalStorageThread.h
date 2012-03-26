/*
 * Copyright (C) 2008 Apple Inc. All Rights Reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef LocalStorageThread_h
#define LocalStorageThread_h

#if ENABLE(DOM_STORAGE)

#include <wtf/HashSet.h>
#include <wtf/MessageQueue.h>
#include <wtf/PassOwnPtr.h>
#include <wtf/PassRefPtr.h>
#include <wtf/Threading.h>

namespace WebCore {

    class StorageAreaSync;
    class LocalStorageTask;

    // FIXME: Rename this class to StorageThread
    class LocalStorageThread {
        WTF_MAKE_NONCOPYABLE(LocalStorageThread); WTF_MAKE_FAST_ALLOCATED;
    public:
        static PassOwnPtr<LocalStorageThread> create();
        ~LocalStorageThread();

        bool start();
        void terminate();
        void scheduleTask(PassOwnPtr<LocalStorageTask>);

        // Background thread part of the terminate procedure.
        void performTerminate();

    private:
        LocalStorageThread();

        // Called on background thread.
        static void* threadEntryPointCallback(void*);
        void* threadEntryPoint();

        ThreadIdentifier m_threadID;
        MessageQueue<LocalStorageTask> m_queue;
    };

} // namespace WebCore

#endif // ENABLE(DOM_STORAGE)

#endif // LocalStorageThread_h
