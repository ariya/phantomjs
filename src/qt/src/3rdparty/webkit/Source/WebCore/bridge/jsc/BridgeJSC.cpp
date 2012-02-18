/*
 * Copyright (C) 2003, 2006, 2008 Apple Inc. All rights reserved.
 * Copyright 2010, The Android Open Source Project
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
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "BridgeJSC.h"

#include "JSDOMWindowBase.h"
#include "runtime_object.h"
#include "runtime_root.h"
#include "runtime/JSLock.h"
#include "runtime/ObjectPrototype.h"


#if PLATFORM(QT)
#include "qt_instance.h"
#endif

namespace JSC {

namespace Bindings {

Array::Array(PassRefPtr<RootObject> rootObject)
    : m_rootObject(rootObject)
{
    ASSERT(m_rootObject);
}

Array::~Array()
{
}

Instance::Instance(PassRefPtr<RootObject> rootObject)
    : m_rootObject(rootObject)
    , m_runtimeObject(*WebCore::JSDOMWindowBase::commonJSGlobalData())
{
    ASSERT(m_rootObject);
}

Instance::~Instance()
{
    ASSERT(!m_runtimeObject);
}

static KJSDidExecuteFunctionPtr s_didExecuteFunction;

void Instance::setDidExecuteFunction(KJSDidExecuteFunctionPtr func)
{
    s_didExecuteFunction = func;
}

KJSDidExecuteFunctionPtr Instance::didExecuteFunction()
{
    return s_didExecuteFunction;
}

void Instance::begin()
{
    virtualBegin();
}

void Instance::end()
{
    virtualEnd();
}

JSObject* Instance::createRuntimeObject(ExecState* exec)
{
    ASSERT(m_rootObject);
    ASSERT(m_rootObject->isValid());
    if (RuntimeObject* existingObject = m_runtimeObject.get())
        return existingObject;

    JSLock lock(SilenceAssertionsOnly);
    RuntimeObject* newObject = newRuntimeObject(exec);
    m_runtimeObject.set(exec->globalData(), newObject, 0);
    m_rootObject->addRuntimeObject(exec->globalData(), newObject);
    return newObject;
}

RuntimeObject* Instance::newRuntimeObject(ExecState* exec)
{
    JSLock lock(SilenceAssertionsOnly);

    // FIXME: deprecatedGetDOMStructure uses the prototype off of the wrong global object
    // We need to pass in the right global object for "i".
    return new (exec) RuntimeObject(exec, exec->lexicalGlobalObject(), WebCore::deprecatedGetDOMStructure<RuntimeObject>(exec), this);
}

void Instance::willInvalidateRuntimeObject()
{
    m_runtimeObject.clear();
}

RootObject* Instance::rootObject() const
{
    return m_rootObject && m_rootObject->isValid() ? m_rootObject.get() : 0;
}

} // namespace Bindings

} // namespace JSC
