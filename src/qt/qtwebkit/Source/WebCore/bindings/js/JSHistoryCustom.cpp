/*
 * Copyright (C) 2008 Apple Inc. All rights reserved.
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
#include "JSHistory.h"

#include "Frame.h"
#include "History.h"
#include "SerializedScriptValue.h"
#include <runtime/JSFunction.h>

using namespace JSC;

namespace WebCore {

static JSValue nonCachingStaticBackFunctionGetter(ExecState* exec, JSValue, PropertyName propertyName)
{
    return JSFunction::create(exec, exec->lexicalGlobalObject(), 0, propertyName.publicName(), jsHistoryPrototypeFunctionBack);
}

static JSValue nonCachingStaticForwardFunctionGetter(ExecState* exec, JSValue, PropertyName propertyName)
{
    return JSFunction::create(exec, exec->lexicalGlobalObject(), 0, propertyName.publicName(), jsHistoryPrototypeFunctionForward);
}

static JSValue nonCachingStaticGoFunctionGetter(ExecState* exec, JSValue, PropertyName propertyName)
{
    return JSFunction::create(exec, exec->lexicalGlobalObject(), 1, propertyName.publicName(), jsHistoryPrototypeFunctionGo);
}

bool JSHistory::getOwnPropertySlotDelegate(ExecState* exec, PropertyName propertyName, PropertySlot& slot)
{
    // When accessing History cross-domain, functions are always the native built-in ones.
    // See JSDOMWindow::getOwnPropertySlotDelegate for additional details.

    // Our custom code is only needed to implement the Window cross-domain scheme, so if access is
    // allowed, return false so the normal lookup will take place.
    String message;
    if (shouldAllowAccessToFrame(exec, impl()->frame(), message))
        return false;

    // Check for the few functions that we allow, even when called cross-domain.
    const HashEntry* entry = JSHistoryPrototype::s_info.propHashTable(exec)->entry(exec, propertyName);
    if (entry) {
        // Allow access to back(), forward() and go() from any frame.
        if (entry->attributes() & JSC::Function) {
            if (entry->function() == jsHistoryPrototypeFunctionBack) {
                slot.setCustom(this, nonCachingStaticBackFunctionGetter);
                return true;
            } else if (entry->function() == jsHistoryPrototypeFunctionForward) {
                slot.setCustom(this, nonCachingStaticForwardFunctionGetter);
                return true;
            } else if (entry->function() == jsHistoryPrototypeFunctionGo) {
                slot.setCustom(this, nonCachingStaticGoFunctionGetter);
                return true;
            }
        }
    } else {
        // Allow access to toString() cross-domain, but always Object.toString.
        if (propertyName == exec->propertyNames().toString) {
            slot.setCustom(this, objectToStringFunctionGetter);
            return true;
        }
    }

    printErrorMessageForFrame(impl()->frame(), message);
    slot.setUndefined();
    return true;
}

bool JSHistory::getOwnPropertyDescriptorDelegate(ExecState* exec, PropertyName propertyName, PropertyDescriptor& descriptor)
{
    if (!impl()->frame()) {
        descriptor.setUndefined();
        return true;
    }

    // Throw out all cross domain access
    if (!shouldAllowAccessToFrame(exec, impl()->frame()))
        return true;

    // Check for the few functions that we allow, even when called cross-domain.
    const HashEntry* entry = JSHistoryPrototype::s_info.propHashTable(exec)->entry(exec, propertyName);
    if (entry) {
        PropertySlot slot;
        // Allow access to back(), forward() and go() from any frame.
        if (entry->attributes() & JSC::Function) {
            if (entry->function() == jsHistoryPrototypeFunctionBack) {
                slot.setCustom(this, nonCachingStaticBackFunctionGetter);
                descriptor.setDescriptor(slot.getValue(exec, propertyName), entry->attributes());
                return true;
            } else if (entry->function() == jsHistoryPrototypeFunctionForward) {
                slot.setCustom(this, nonCachingStaticForwardFunctionGetter);
                descriptor.setDescriptor(slot.getValue(exec, propertyName), entry->attributes());
                return true;
            } else if (entry->function() == jsHistoryPrototypeFunctionGo) {
                slot.setCustom(this, nonCachingStaticGoFunctionGetter);
                descriptor.setDescriptor(slot.getValue(exec, propertyName), entry->attributes());
                return true;
            }
        }
    } else {
        // Allow access to toString() cross-domain, but always Object.toString.
        if (propertyName == exec->propertyNames().toString) {
            PropertySlot slot;
            slot.setCustom(this, objectToStringFunctionGetter);
            descriptor.setDescriptor(slot.getValue(exec, propertyName), entry->attributes());
            return true;
        }
    }

    descriptor.setUndefined();
    return true;
}

bool JSHistory::putDelegate(ExecState* exec, PropertyName, JSValue, PutPropertySlot&)
{
    // Only allow putting by frames in the same origin.
    if (!shouldAllowAccessToFrame(exec, impl()->frame()))
        return true;
    return false;
}

bool JSHistory::deleteProperty(JSCell* cell, ExecState* exec, PropertyName propertyName)
{
    JSHistory* thisObject = jsCast<JSHistory*>(cell);
    // Only allow deleting by frames in the same origin.
    if (!shouldAllowAccessToFrame(exec, thisObject->impl()->frame()))
        return false;
    return Base::deleteProperty(thisObject, exec, propertyName);
}

bool JSHistory::deletePropertyByIndex(JSCell* cell, ExecState* exec, unsigned propertyName)
{
    JSHistory* thisObject = jsCast<JSHistory*>(cell);
    // Only allow deleting by frames in the same origin.
    if (!shouldAllowAccessToFrame(exec, thisObject->impl()->frame()))
        return false;
    return Base::deletePropertyByIndex(thisObject, exec, propertyName);
}

void JSHistory::getOwnPropertyNames(JSObject* object, ExecState* exec, PropertyNameArray& propertyNames, EnumerationMode mode)
{
    JSHistory* thisObject = jsCast<JSHistory*>(object);
    // Only allow the history object to enumerated by frames in the same origin.
    if (!shouldAllowAccessToFrame(exec, thisObject->impl()->frame()))
        return;
    Base::getOwnPropertyNames(thisObject, exec, propertyNames, mode);
}

JSValue JSHistory::state(ExecState *exec) const
{
    History* history = static_cast<History*>(impl());

    JSValue cachedValue = m_state.get();
    if (!cachedValue.isEmpty() && !history->stateChanged())
        return cachedValue;

    RefPtr<SerializedScriptValue> serialized = history->state();
    JSValue result = serialized ? serialized->deserialize(exec, globalObject(), 0) : jsNull();
    const_cast<JSHistory*>(this)->m_state.set(exec->vm(), this, result);
    return result;
}

JSValue JSHistory::pushState(ExecState* exec)
{
    RefPtr<SerializedScriptValue> historyState = SerializedScriptValue::create(exec, exec->argument(0), 0, 0);
    if (exec->hadException())
        return jsUndefined();

    String title = valueToStringWithUndefinedOrNullCheck(exec, exec->argument(1));
    if (exec->hadException())
        return jsUndefined();
        
    String url;
    if (exec->argumentCount() > 2) {
        url = valueToStringWithUndefinedOrNullCheck(exec, exec->argument(2));
        if (exec->hadException())
            return jsUndefined();
    }

    ExceptionCode ec = 0;
    impl()->stateObjectAdded(historyState.release(), title, url, History::StateObjectPush, ec);
    setDOMException(exec, ec);

    m_state.clear();

    return jsUndefined();
}

JSValue JSHistory::replaceState(ExecState* exec)
{
    RefPtr<SerializedScriptValue> historyState = SerializedScriptValue::create(exec, exec->argument(0), 0, 0);
    if (exec->hadException())
        return jsUndefined();

    String title = valueToStringWithUndefinedOrNullCheck(exec, exec->argument(1));
    if (exec->hadException())
        return jsUndefined();
        
    String url;
    if (exec->argumentCount() > 2) {
        url = valueToStringWithUndefinedOrNullCheck(exec, exec->argument(2));
        if (exec->hadException())
            return jsUndefined();
    }

    ExceptionCode ec = 0;
    impl()->stateObjectAdded(historyState.release(), title, url, History::StateObjectReplace, ec);
    setDOMException(exec, ec);

    m_state.clear();

    return jsUndefined();
}

} // namespace WebCore
