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
#include "JSDOMWindowShell.h"

#include "Frame.h"
#include "JSDOMWindow.h"
#include "DOMWindow.h"
#include "ScriptController.h"
#include <runtime/JSObject.h>

using namespace JSC;

namespace WebCore {

ASSERT_CLASS_FITS_IN_CELL(JSDOMWindowShell);

const ClassInfo JSDOMWindowShell::s_info = { "JSDOMWindowShell", &Base::s_info, 0, 0 };

JSDOMWindowShell::JSDOMWindowShell(PassRefPtr<DOMWindow> window, DOMWrapperWorld* world)
    : Base(*world->globalData(), JSDOMWindowShell::createStructure(*world->globalData(), jsNull()))
    , m_world(world)
{
    ASSERT(inherits(&s_info));
    setWindow(window);
}

JSDOMWindowShell::~JSDOMWindowShell()
{
}

void JSDOMWindowShell::setWindow(PassRefPtr<DOMWindow> domWindow)
{
    // Explicitly protect the global object's prototype so it isn't collected
    // when we allocate the global object. (Once the global object is fully
    // constructed, it can mark its own prototype.)
    Structure* prototypeStructure = JSDOMWindowPrototype::createStructure(*JSDOMWindow::commonJSGlobalData(), jsNull());
    Strong<JSDOMWindowPrototype> prototype(*JSDOMWindow::commonJSGlobalData(), new JSDOMWindowPrototype(*JSDOMWindow::commonJSGlobalData(), 0, prototypeStructure));

    Structure* structure = JSDOMWindow::createStructure(*JSDOMWindow::commonJSGlobalData(), prototype.get());
    JSDOMWindow* jsDOMWindow = new (JSDOMWindow::commonJSGlobalData()) JSDOMWindow(*JSDOMWindow::commonJSGlobalData(), structure, domWindow, this);
    prototype->putAnonymousValue(*JSDOMWindow::commonJSGlobalData(), 0, jsDOMWindow);
    setWindow(*JSDOMWindow::commonJSGlobalData(), jsDOMWindow);
}

// ----
// JSObject methods
// ----

void JSDOMWindowShell::visitChildren(SlotVisitor& visitor)
{
    Base::visitChildren(visitor);
    if (m_window)
        visitor.append(&m_window);
}

UString JSDOMWindowShell::className() const
{
    return m_window->className();
}

bool JSDOMWindowShell::getOwnPropertySlot(ExecState* exec, const Identifier& propertyName, PropertySlot& slot)
{
    return m_window->getOwnPropertySlot(exec, propertyName, slot);
}

bool JSDOMWindowShell::getOwnPropertyDescriptor(ExecState* exec, const Identifier& propertyName, PropertyDescriptor& descriptor)
{
    return m_window->getOwnPropertyDescriptor(exec, propertyName, descriptor);
}

void JSDOMWindowShell::put(ExecState* exec, const Identifier& propertyName, JSValue value, PutPropertySlot& slot)
{
    m_window->put(exec, propertyName, value, slot);
}

void JSDOMWindowShell::putWithAttributes(ExecState* exec, const Identifier& propertyName, JSValue value, unsigned attributes)
{
    m_window->putWithAttributes(exec, propertyName, value, attributes);
}

bool JSDOMWindowShell::defineOwnProperty(JSC::ExecState* exec, const JSC::Identifier& propertyName, JSC::PropertyDescriptor& descriptor, bool shouldThrow)
{
    return m_window->defineOwnProperty(exec, propertyName, descriptor, shouldThrow);
}

bool JSDOMWindowShell::deleteProperty(ExecState* exec, const Identifier& propertyName)
{
    return m_window->deleteProperty(exec, propertyName);
}

void JSDOMWindowShell::getPropertyNames(ExecState* exec, PropertyNameArray& propertyNames, EnumerationMode mode)
{
    m_window->getPropertyNames(exec, propertyNames, mode);
}

void JSDOMWindowShell::getOwnPropertyNames(ExecState* exec, PropertyNameArray& propertyNames, EnumerationMode mode)
{
    m_window->getOwnPropertyNames(exec, propertyNames, mode);
}

void JSDOMWindowShell::defineGetter(ExecState* exec, const Identifier& propertyName, JSObject* getterFunction, unsigned attributes)
{
    m_window->defineGetter(exec, propertyName, getterFunction, attributes);
}

void JSDOMWindowShell::defineSetter(ExecState* exec, const Identifier& propertyName, JSObject* setterFunction, unsigned attributes)
{
    m_window->defineSetter(exec, propertyName, setterFunction, attributes);
}

JSValue JSDOMWindowShell::lookupGetter(ExecState* exec, const Identifier& propertyName)
{
    return m_window->lookupGetter(exec, propertyName);
}

JSValue JSDOMWindowShell::lookupSetter(ExecState* exec, const Identifier& propertyName)
{
    return m_window->lookupSetter(exec, propertyName);
}

JSObject* JSDOMWindowShell::unwrappedObject()
{
    return m_window.get();
}

// ----
// JSDOMWindow methods
// ----

DOMWindow* JSDOMWindowShell::impl() const
{
    return m_window->impl();
}

void* JSDOMWindowShell::operator new(size_t size)
{
    return JSDOMWindow::commonJSGlobalData()->heap.allocate(size);
}

// ----
// Conversion methods
// ----

JSValue toJS(ExecState* exec, Frame* frame)
{
    if (!frame)
        return jsNull();
    return frame->script()->windowShell(currentWorld(exec));
}

JSDOMWindowShell* toJSDOMWindowShell(Frame* frame, DOMWrapperWorld* isolatedWorld)
{
    if (!frame)
        return 0;
    return frame->script()->windowShell(isolatedWorld);
}

} // namespace WebCore
