/*
 * Copyright (C) 2005, 2006, 2007 Apple Inc. All rights reserved.
 *           (C) 2007 Graham Dennis (graham.dennis@gmail.com)
 *           (C) 2007 Eric Seidel <eric@webkit.org>
 *           (C) 2012 Patrick Ganstere <paroga@paroga.com>
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
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission. 
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
#include "JavaScriptThreading.h"

#include <JavaScriptCore/JavaScriptCore.h>
#include <stdlib.h>
#include <wtf/Assertions.h>
#include <wtf/HashSet.h>
#include <wtf/Vector.h>

static const size_t javaScriptThreadsCount = 4;
static bool javaScriptThreadsShouldTerminate;
static JSContextGroupRef javaScriptThreadsGroup;

static Mutex& javaScriptThreadsMutex()
{
    DEFINE_STATIC_LOCAL(Mutex, staticMutex, ());
    return staticMutex;
}

typedef HashSet<ThreadIdentifier> ThreadSet;
static ThreadSet& javaScriptThreads()
{
    DEFINE_STATIC_LOCAL(ThreadSet, staticJavaScriptThreads, ());
    ASSERT(!javaScriptThreadsMutex().tryLock());
    return staticJavaScriptThreads;
}

// This function exercises JSC in a loop until javaScriptThreadsShouldTerminate
// becomes true or it probabilistically decides to spawn a replacement thread and exit.
void runJavaScriptThread(void*)
{
    static const char* const script =
        "var array = [];"
        "for (var i = 0; i < 1024; i++) {"
        "    array.push(String(i));"
        "}";

    JSGlobalContextRef ctx;
    {
        MutexLocker locker(javaScriptThreadsMutex());
        ctx = JSGlobalContextCreateInGroup(javaScriptThreadsGroup, 0);
    }

    JSStringRef scriptRef;
    {
        MutexLocker locker(javaScriptThreadsMutex());
        scriptRef = JSStringCreateWithUTF8CString(script);
    }

    while (true) {
        {
            MutexLocker locker(javaScriptThreadsMutex());
            JSValueRef exception = 0;
            JSEvaluateScript(ctx, scriptRef, 0, 0, 1, &exception);
            ASSERT(!exception);
        }

        {
            MutexLocker locker(javaScriptThreadsMutex());
            const size_t valuesCount = 1024;
            JSValueRef values[valuesCount];
            for (size_t i = 0; i < valuesCount; ++i)
                values[i] = JSObjectMake(ctx, 0, 0);
        }

        {
            MutexLocker locker(javaScriptThreadsMutex());
            if (javaScriptThreadsShouldTerminate)
                break;
        }

        // Respawn probabilistically.
        if (rand() % 5)
            continue;

        MutexLocker locker(javaScriptThreadsMutex());
        ThreadIdentifier thread = currentThread();
        detachThread(thread);
        javaScriptThreads().remove(thread);
        javaScriptThreads().add(createThread(&runJavaScriptThread, 0, 0));
        break;
    }

    MutexLocker locker(javaScriptThreadsMutex());
    JSStringRelease(scriptRef);
    JSGarbageCollect(ctx);
    JSGlobalContextRelease(ctx);
}

void startJavaScriptThreads()
{
    javaScriptThreadsGroup = JSContextGroupCreate();

    MutexLocker locker(javaScriptThreadsMutex());

    for (size_t i = 0; i < javaScriptThreadsCount; ++i)
        javaScriptThreads().add(createThread(&runJavaScriptThread, 0, 0));
}

void stopJavaScriptThreads()
{
    {
        MutexLocker locker(javaScriptThreadsMutex());
        javaScriptThreadsShouldTerminate = true;
    }

    Vector<ThreadIdentifier, javaScriptThreadsCount> threads;
    {
        MutexLocker locker(javaScriptThreadsMutex());
        copyToVector(javaScriptThreads(), threads);
        ASSERT(threads.size() == javaScriptThreadsCount);
    }

    for (size_t i = 0; i < javaScriptThreadsCount; ++i)
        waitForThreadCompletion(threads[i]);

    javaScriptThreads().clear();

    JSContextGroupRelease(javaScriptThreadsGroup);
}
