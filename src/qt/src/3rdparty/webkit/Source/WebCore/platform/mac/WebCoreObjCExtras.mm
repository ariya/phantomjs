/*
 * Copyright (C) 2007 Apple Inc.  All rights reserved.
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

// This file intentionally calls objc_finalizeOnMainThread, which is deprecated.
// According to http://gcc.gnu.org/onlinedocs/gcc-4.2.1/gcc/Diagnostic-Pragmas.html#Diagnostic-Pragmas
// we need to place this directive before any data or functions are defined.
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

#include "config.h"
#include "WebCoreObjCExtras.h"

#include <objc/objc-auto.h>
#include <objc/objc-runtime.h>
#include <utility>
#include <wtf/Assertions.h>
#include <wtf/MainThread.h>
#include <wtf/Threading.h>
#include <wtf/UnusedParam.h>

void WebCoreObjCFinalizeOnMainThread(Class cls)
{
    // This method relies on threading being initialized by the caller, otherwise
    // WebCoreObjCScheduleDeallocateOnMainThread will crash.
#ifndef DONT_FINALIZE_ON_MAIN_THREAD
    objc_finalizeOnMainThread(cls);
#else
    UNUSED_PARAM(cls);
#endif
}


typedef std::pair<Class, id> ClassAndIdPair;

static void deallocCallback(void* context)
{
    ClassAndIdPair* pair = static_cast<ClassAndIdPair*>(context);
    
    Method method = class_getInstanceMethod(pair->first, @selector(dealloc));
    
    IMP imp = method_getImplementation(method);
    imp(pair->second, @selector(dealloc));
    
    delete pair;
}

bool WebCoreObjCScheduleDeallocateOnMainThread(Class cls, id object)
{
    ASSERT([object isKindOfClass:cls]);

    if (isMainThread())
        return false;
    
    ClassAndIdPair* pair = new ClassAndIdPair(cls, object);
    callOnMainThread(deallocCallback, pair);
    return true;
}

