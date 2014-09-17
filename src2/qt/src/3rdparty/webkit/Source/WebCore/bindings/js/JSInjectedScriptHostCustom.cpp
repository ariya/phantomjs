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
#include "JSInjectedScriptHost.h"

#if ENABLE(INSPECTOR)

#if ENABLE(DATABASE)
#include "Database.h"
#include "JSDatabase.h"
#endif
#include "ExceptionCode.h"
#include "InjectedScriptHost.h"
#include "InspectorDebuggerAgent.h"
#include "InspectorValues.h"
#include "JSHTMLAllCollection.h"
#include "JSHTMLCollection.h"
#include "JSNode.h"
#include "JSNodeList.h"
#include "ScriptValue.h"
#if ENABLE(DOM_STORAGE)
#include "Storage.h"
#include "JSStorage.h"
#endif
#include <runtime/DateInstance.h>
#include <runtime/JSArray.h>
#include <runtime/JSLock.h>
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
    JSLock lock(SilenceAssertionsOnly);
    return ScriptValue(state->globalData(), toJS(state, deprecatedGlobalObjectForPrototype(state), node));
}

JSValue JSInjectedScriptHost::inspectedNode(ExecState* exec)
{
    if (exec->argumentCount() < 1)
        return jsUndefined();

    Node* node = impl()->inspectedNode(exec->argument(0).toInt32(exec));
    if (!node)
        return jsUndefined();

    JSLock lock(SilenceAssertionsOnly);
    return toJS(exec, globalObject(), node);
}

JSValue JSInjectedScriptHost::internalConstructorName(ExecState* exec)
{
    if (exec->argumentCount() < 1)
        return jsUndefined();

    UString result = exec->argument(0).toThisObject(exec)->className();
    return jsString(exec, result);
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
    return jsUndefined();
}

JSValue JSInjectedScriptHost::inspect(ExecState* exec)
{
    if (exec->argumentCount() >= 2) {
        ScriptValue object(exec->globalData(), exec->argument(0));
        ScriptValue hints(exec->globalData(), exec->argument(1));
        impl()->inspectImpl(object.toInspectorValue(exec), hints.toInspectorValue(exec));
    }
    return jsUndefined();
}

JSValue JSInjectedScriptHost::databaseId(ExecState* exec)
{
    if (exec->argumentCount() < 1)
        return jsUndefined();
#if ENABLE(DATABASE)
    Database* database = toDatabase(exec->argument(0));
    if (database)
        return jsNumber(impl()->databaseIdImpl(database));
#endif
    return jsUndefined();
}

JSValue JSInjectedScriptHost::storageId(ExecState* exec)
{
    if (exec->argumentCount() < 1)
        return jsUndefined();
#if ENABLE(DOM_STORAGE)
    Storage* storage = toStorage(exec->argument(0));
    if (storage)
        return jsNumber(impl()->storageIdImpl(storage));
#endif
    return jsUndefined();
}

} // namespace WebCore

#endif // ENABLE(INSPECTOR)
