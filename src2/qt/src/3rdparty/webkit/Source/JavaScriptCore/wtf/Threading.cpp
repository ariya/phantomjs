/*
 * Copyright (C) 2008, 2009 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer. 
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution. 
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "Threading.h"

#include <string.h>

namespace WTF {

struct NewThreadContext {
    WTF_MAKE_FAST_ALLOCATED;
public:
    NewThreadContext(ThreadFunction entryPoint, void* data, const char* name)
        : entryPoint(entryPoint)
        , data(data)
        , name(name)
    {
    }

    ThreadFunction entryPoint;
    void* data;
    const char* name;

    Mutex creationMutex;
};

static void* threadEntryPoint(void* contextData)
{
    NewThreadContext* context = reinterpret_cast<NewThreadContext*>(contextData);

    // Block until our creating thread has completed any extra setup work, including
    // establishing ThreadIdentifier.
    {
        MutexLocker locker(context->creationMutex);
    }

    initializeCurrentThreadInternal(context->name);

    // Grab the info that we need out of the context, then deallocate it.
    ThreadFunction entryPoint = context->entryPoint;
    void* data = context->data;
    delete context;

    return entryPoint(data);
}

ThreadIdentifier createThread(ThreadFunction entryPoint, void* data, const char* name)
{
    // Visual Studio has a 31-character limit on thread names. Longer names will
    // be truncated silently, but we'd like callers to know about the limit.
#if !LOG_DISABLED
    if (strlen(name) > 31)
        LOG_ERROR("Thread name \"%s\" is longer than 31 characters and will be truncated by Visual Studio", name);
#endif

    NewThreadContext* context = new NewThreadContext(entryPoint, data, name);

    // Prevent the thread body from executing until we've established the thread identifier.
    MutexLocker locker(context->creationMutex);

    return createThreadInternal(threadEntryPoint, context, name);
}

#if PLATFORM(MAC) || PLATFORM(WIN)

// This function is deprecated but needs to be kept around for backward
// compatibility. Use the 3-argument version of createThread above.

ThreadIdentifier createThread(ThreadFunction entryPoint, void* data);

ThreadIdentifier createThread(ThreadFunction entryPoint, void* data)
{
    return createThread(entryPoint, data, 0);
}
#endif

} // namespace WTF
