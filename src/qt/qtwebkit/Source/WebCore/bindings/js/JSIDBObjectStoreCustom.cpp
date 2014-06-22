/*
 * Copyright (C) 2012 Michael Pruett <michael@68k.org>
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

#if ENABLE(INDEXED_DATABASE)

#include "JSIDBObjectStore.h"

#include "IDBBindingUtilities.h"
#include "IDBKeyPath.h"
#include "IDBObjectStore.h"
#include "JSIDBIndex.h"
#include <runtime/Error.h>
#include <runtime/JSString.h>

using namespace JSC;

namespace WebCore {

JSValue JSIDBObjectStore::createIndex(ExecState* exec)
{
    ScriptExecutionContext* context = jsCast<JSDOMGlobalObject*>(exec->lexicalGlobalObject())->scriptExecutionContext();
    if (!context)
        return throwError(exec, createReferenceError(exec, "IDBObjectStore script execution context is unavailable"));

    if (exec->argumentCount() < 2)
        return throwError(exec, createNotEnoughArgumentsError(exec));

    String name = exec->argument(0).toString(exec)->value(exec);
    if (exec->hadException())
        return jsUndefined();

    IDBKeyPath keyPath = idbKeyPathFromValue(exec, exec->argument(1));
    if (exec->hadException())
        return jsUndefined();

    JSValue optionsValue = exec->argument(2);
    if (!optionsValue.isUndefinedOrNull() && !optionsValue.isObject())
        return throwTypeError(exec, "Not an object.");

    bool unique = false;
    bool multiEntry = false;
    if (!optionsValue.isUndefinedOrNull()) {
        unique = optionsValue.get(exec, Identifier(exec, "unique")).toBoolean(exec);
        if (exec->hadException())
            return jsUndefined();

        multiEntry = optionsValue.get(exec, Identifier(exec, "multiEntry")).toBoolean(exec);
        if (exec->hadException())
            return jsUndefined();
    }

    ExceptionCode ec = 0;
    JSValue result = toJS(exec, globalObject(), impl()->createIndex(context, name, keyPath, unique, multiEntry, ec).get());
    setDOMException(exec, ec);
    return result;
}

}

#endif
