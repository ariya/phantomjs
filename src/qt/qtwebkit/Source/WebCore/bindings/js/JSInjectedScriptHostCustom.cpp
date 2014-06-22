/*
 * Copyright (C) 2007, 2008 Apple Inc. All rights reserved.
 * Copyright (C) 2008 Matt Lilek <webkit@mattlilek.com>
 * Copyright (C) 2010-2011 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"

#if ENABLE(INSPECTOR)

#include "JSInjectedScriptHost.h"

#if ENABLE(SQL_DATABASE)
#include "Database.h"
#include "JSDatabase.h"
#endif
#include "ExceptionCode.h"
#include "InjectedScriptHost.h"
#include "InspectorDOMAgent.h"
#include "InspectorDebuggerAgent.h"
#include "InspectorValues.h"
#include "JSEventListener.h"
#include "JSFloat32Array.h"
#include "JSFloat64Array.h"
#include "JSHTMLAllCollection.h"
#include "JSHTMLCollection.h"
#include "JSInt16Array.h"
#include "JSInt32Array.h"
#include "JSInt8Array.h"
#include "JSNode.h"
#include "JSNodeList.h"
#include "JSStorage.h"
#include "JSUint16Array.h"
#include "JSUint32Array.h"
#include "JSUint8Array.h"
#include "ScriptValue.h"
#include "Storage.h"
#include <parser/SourceCode.h>
#include <runtime/DateInstance.h>
#include <runtime/Error.h>
#include <runtime/JSArray.h>
#include <runtime/JSFunction.h>
#include <runtime/JSLock.h>
#include <runtime/ObjectConstructor.h>
#include <runtime/RegExpObject.h>

using namespace JSC;

namespace WebCore {

Node* InjectedScriptHost::scriptValueAsNode(ScriptValue value)
{
    if (!value.isObject() || value.isNull())
        return 0;
    return toNode(value.jsValue());
}

ScriptValue InjectedScriptHost::nodeAsScriptValue(ScriptState* state, Node* node)
{
    if (!shouldAllowAccessToNode(state, node))
        return ScriptValue(state->vm(), jsNull());

    JSLockHolder lock(state);
    return ScriptValue(state->vm(), toJS(state, deprecatedGlobalObjectForPrototype(state), node));
}

JSValue JSInjectedScriptHost::inspectedObject(ExecState* exec)
{
    if (exec->argumentCount() < 1)
        return jsUndefined();

    InjectedScriptHost::InspectableObject* object = impl()->inspectedObject(exec->argument(0).toInt32(exec));
    if (!object)
        return jsUndefined();

    JSLockHolder lock(exec);
    ScriptValue scriptValue = object->get(exec);
    if (scriptValue.hasNoValue())
        return jsUndefined();

    return scriptValue.jsValue();
}

JSValue JSInjectedScriptHost::internalConstructorName(ExecState* exec)
{
    if (exec->argumentCount() < 1)
        return jsUndefined();

    JSObject* thisObject = exec->argument(0).toThisObject(exec);
    String result = thisObject->methodTable()->className(thisObject);
    return jsStringWithCache(exec, result);
}

JSValue JSInjectedScriptHost::isHTMLAllCollection(ExecState* exec)
{
    if (exec->argumentCount() < 1)
        return jsUndefined();

    JSValue value = exec->argument(0);
    return jsBoolean(value.inherits(&JSHTMLAllCollection::s_info));
}

JSValue JSInjectedScriptHost::type(ExecState* exec)
{
    if (exec->argumentCount() < 1)
        return jsUndefined();

    JSValue value = exec->argument(0);
    if (value.isString())
        return jsString(exec, String("string"));
    if (value.inherits(&JSArray::s_info))
        return jsString(exec, String("array"));
    if (value.isBoolean())
        return jsString(exec, String("boolean"));
    if (value.isNumber())
        return jsString(exec, String("number"));
    if (value.inherits(&DateInstance::s_info))
        return jsString(exec, String("date"));
    if (value.inherits(&RegExpObject::s_info))
        return jsString(exec, String("regexp"));
    if (value.inherits(&JSNode::s_info))
        return jsString(exec, String("node"));
    if (value.inherits(&JSNodeList::s_info))
        return jsString(exec, String("array"));
    if (value.inherits(&JSHTMLCollection::s_info))
        return jsString(exec, String("array"));
    if (value.inherits(&JSInt8Array::s_info) || value.inherits(&JSInt16Array::s_info) || value.inherits(&JSInt32Array::s_info))
        return jsString(exec, String("array"));
    if (value.inherits(&JSUint8Array::s_info) || value.inherits(&JSUint16Array::s_info) || value.inherits(&JSUint32Array::s_info))
        return jsString(exec, String("array"));
    if (value.inherits(&JSFloat32Array::s_info) || value.inherits(&JSFloat64Array::s_info))
        return jsString(exec, String("array"));
    return jsUndefined();
}

JSValue JSInjectedScriptHost::functionDetails(ExecState* exec)
{
    if (exec->argumentCount() < 1)
        return jsUndefined();
    JSValue value = exec->argument(0);
    if (!value.asCell()->inherits(&JSFunction::s_info))
        return jsUndefined();
    JSFunction* function = jsCast<JSFunction*>(value);

    const SourceCode* sourceCode = function->sourceCode();
    if (!sourceCode)
        return jsUndefined();
    int lineNumber = sourceCode->firstLine();
    if (lineNumber)
        lineNumber -= 1; // In the inspector protocol all positions are 0-based while in SourceCode they are 1-based
    String scriptId = String::number(sourceCode->provider()->asID());

    JSObject* location = constructEmptyObject(exec);
    location->putDirect(exec->vm(), Identifier(exec, "lineNumber"), jsNumber(lineNumber));
    location->putDirect(exec->vm(), Identifier(exec, "scriptId"), jsString(exec, scriptId));

    JSObject* result = constructEmptyObject(exec);
    result->putDirect(exec->vm(), Identifier(exec, "location"), location);
    String name = function->name(exec);
    if (!name.isEmpty())
        result->putDirect(exec->vm(), Identifier(exec, "name"), jsStringWithCache(exec, name));
    String displayName = function->displayName(exec);
    if (!displayName.isEmpty())
        result->putDirect(exec->vm(), Identifier(exec, "displayName"), jsStringWithCache(exec, displayName));
    // FIXME: provide function scope data in "scopesRaw" property when JSC supports it.
    //     https://bugs.webkit.org/show_bug.cgi?id=87192
    return result;
}

JSValue JSInjectedScriptHost::getInternalProperties(ExecState*)
{
    // FIXME: implement this. https://bugs.webkit.org/show_bug.cgi?id=94533
    return jsUndefined();
}

static JSArray* getJSListenerFunctions(ExecState* exec, Document* document, const EventListenerInfo& listenerInfo)
{
    JSArray* result = constructEmptyArray(exec, 0);
    size_t handlersCount = listenerInfo.eventListenerVector.size();
    for (size_t i = 0, outputIndex = 0; i < handlersCount; ++i) {
        const JSEventListener* jsListener = JSEventListener::cast(listenerInfo.eventListenerVector[i].listener.get());
        if (!jsListener) {
            ASSERT_NOT_REACHED();
            continue;
        }
        // Hide listeners from other contexts.
        if (jsListener->isolatedWorld() != currentWorld(exec))
            continue;
        JSObject* function = jsListener->jsFunction(document);
        if (!function)
            continue;
        JSObject* listenerEntry = constructEmptyObject(exec);
        listenerEntry->putDirect(exec->vm(), Identifier(exec, "listener"), function);
        listenerEntry->putDirect(exec->vm(), Identifier(exec, "useCapture"), jsBoolean(listenerInfo.eventListenerVector[i].useCapture));
        result->putDirectIndex(exec, outputIndex++, JSValue(listenerEntry));
    }
    return result;
}

JSValue JSInjectedScriptHost::getEventListeners(ExecState* exec)
{
    if (exec->argumentCount() < 1)
        return jsUndefined();
    JSValue value = exec->argument(0);
    if (!value.isObject() || value.isNull())
        return jsUndefined();
    Node* node = toNode(value);
    if (!node)
        return jsUndefined();
    // This can only happen for orphan DocumentType nodes.
    Document* document = node->document();
    if (!node->document())
        return jsUndefined();

    Vector<EventListenerInfo> listenersArray;
    impl()->getEventListenersImpl(node, listenersArray);

    JSObject* result = constructEmptyObject(exec);
    for (size_t i = 0; i < listenersArray.size(); ++i) {
        JSArray* listeners = getJSListenerFunctions(exec, document, listenersArray[i]);
        if (!listeners->length())
            continue;
        AtomicString eventType = listenersArray[i].eventType;
        result->putDirect(exec->vm(), Identifier(exec, eventType.impl()), JSValue(listeners));
    }

    return result;
}

JSValue JSInjectedScriptHost::inspect(ExecState* exec)
{
    if (exec->argumentCount() >= 2) {
        ScriptValue object(exec->vm(), exec->argument(0));
        ScriptValue hints(exec->vm(), exec->argument(1));
        impl()->inspectImpl(object.toInspectorValue(exec), hints.toInspectorValue(exec));
    }
    return jsUndefined();
}

JSValue JSInjectedScriptHost::databaseId(ExecState* exec)
{
    if (exec->argumentCount() < 1)
        return jsUndefined();
#if ENABLE(SQL_DATABASE)
    Database* database = toDatabase(exec->argument(0));
    if (database)
        return jsStringWithCache(exec, impl()->databaseIdImpl(database));
#endif
    return jsUndefined();
}

JSValue JSInjectedScriptHost::storageId(ExecState* exec)
{
    if (exec->argumentCount() < 1)
        return jsUndefined();
    Storage* storage = toStorage(exec->argument(0));
    if (storage)
        return jsStringWithCache(exec, impl()->storageIdImpl(storage));
    return jsUndefined();
}

JSValue JSInjectedScriptHost::evaluate(ExecState* exec) const
{
    JSGlobalObject* globalObject = exec->lexicalGlobalObject();
    return globalObject->evalFunction();
}

JSValue JSInjectedScriptHost::setFunctionVariableValue(JSC::ExecState* exec)
{
    // FIXME: implement this. https://bugs.webkit.org/show_bug.cgi?id=107830
    throwError(exec, createTypeError(exec, "Variable value mutation is not supported"));
    return jsUndefined();
}

} // namespace WebCore

#endif // ENABLE(INSPECTOR)
