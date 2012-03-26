/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef LocalScope_h
#define LocalScope_h

#include "HandleStack.h"
#include "Local.h"

namespace JSC {
/*  
    A LocalScope is a temporary scope in which Locals are allocated. When a
    LocalScope goes out of scope, all the Locals created in it are destroyed.

    LocalScope is similar in concept to NSAutoreleasePool.
*/

class JSGlobalData;

class LocalScope {
public:
    explicit LocalScope(JSGlobalData&);
    ~LocalScope();
    
    template <typename T> Local<T> release(Local<T>); // Destroys all other locals in the scope.

private:
    HandleStack* m_handleStack;
    HandleStack::Frame m_lastFrame;
};

inline LocalScope::LocalScope(JSGlobalData& globalData)
    : m_handleStack(globalData.heap.handleStack())
{
    m_handleStack->enterScope(m_lastFrame);
}

inline LocalScope::~LocalScope()
{
    m_handleStack->leaveScope(m_lastFrame);
}

template <typename T> Local<T> LocalScope::release(Local<T> local)
{
    typename Local<T>::ExternalType ptr = local.get();

    m_handleStack->leaveScope(m_lastFrame);
    HandleSlot slot = m_handleStack->push();
    m_handleStack->enterScope(m_lastFrame);

    return Local<T>(slot, ptr);
}

}

#endif
