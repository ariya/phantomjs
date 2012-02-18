/*
 * Copyright (C) 2010 Google Inc. All rights reserved.
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

#if ENABLE(FILE_SYSTEM)

#include "JSDirectoryEntry.h"

#include "ExceptionCode.h"
#include "JSDOMBinding.h"
#include "JSEntryCallback.h"
#include "JSErrorCallback.h"
#include "JSWebKitFlags.h"
#include <wtf/Assertions.h>

using namespace JSC;

namespace WebCore {

JSValue JSDirectoryEntry::getFile(ExecState* exec)
{
    DirectoryEntry* imp = static_cast<DirectoryEntry*>(impl());
    const String& path = valueToStringWithUndefinedOrNullCheck(exec, exec->argument(0));
    if (exec->hadException())
        return jsUndefined();

    int argsCount = exec->argumentCount();
    if (argsCount <= 1) {
        imp->getFile(path);
        return jsUndefined();
    }

    RefPtr<WebKitFlags> flags;
    if (!exec->argument(1).isNull() && !exec->argument(1).isUndefined() && exec->argument(1).isObject() && !exec->argument(1).inherits(&JSWebKitFlags::s_info)) {
        JSObject* object = exec->argument(1).getObject();
        flags = WebKitFlags::create();
        JSValue jsCreate = object->get(exec, Identifier(exec, "create"));
        flags->setCreate(jsCreate.toBoolean(exec));
        JSValue jsExclusive = object->get(exec, Identifier(exec, "exclusive"));
        flags->setExclusive(jsExclusive.toBoolean(exec));
    } else
        flags = toWebKitFlags(exec->argument(1));
    if (exec->hadException())
        return jsUndefined();
    RefPtr<EntryCallback> successCallback;
    if (exec->argumentCount() > 2 && !exec->argument(2).isNull() && !exec->argument(2).isUndefined()) {
        if (!exec->argument(2).isObject()) {
            setDOMException(exec, TYPE_MISMATCH_ERR);
            return jsUndefined();
        }
        successCallback = JSEntryCallback::create(asObject(exec->argument(2)), globalObject());
    }
    RefPtr<ErrorCallback> errorCallback;
    if (exec->argumentCount() > 3 && !exec->argument(3).isNull() && !exec->argument(3).isUndefined()) {
        if (!exec->argument(3).isObject()) {
            setDOMException(exec, TYPE_MISMATCH_ERR);
            return jsUndefined();
        }
        errorCallback = JSErrorCallback::create(asObject(exec->argument(3)), globalObject());
    }

    imp->getFile(path, flags, successCallback, errorCallback);
    return jsUndefined();
}

JSValue JSDirectoryEntry::getDirectory(ExecState* exec)
{
    DirectoryEntry* imp = static_cast<DirectoryEntry*>(impl());
    const String& path = valueToStringWithUndefinedOrNullCheck(exec, exec->argument(0));
    if (exec->hadException())
        return jsUndefined();

    int argsCount = exec->argumentCount();
    if (argsCount <= 1) {
        imp->getDirectory(path);
        return jsUndefined();
    }

    RefPtr<WebKitFlags> flags;
    if (!exec->argument(1).isNull() && !exec->argument(1).isUndefined() && exec->argument(1).isObject() && !exec->argument(1).inherits(&JSWebKitFlags::s_info)) {
        JSObject* object = exec->argument(1).getObject();
        flags = WebKitFlags::create();
        JSValue jsCreate = object->get(exec, Identifier(exec, "create"));
        flags->setCreate(jsCreate.toBoolean(exec));
        JSValue jsExclusive = object->get(exec, Identifier(exec, "exclusive"));
        flags->setExclusive(jsExclusive.toBoolean(exec));
    } else
        flags = toWebKitFlags(exec->argument(1));
    if (exec->hadException())
        return jsUndefined();
    RefPtr<EntryCallback> successCallback;
    if (exec->argumentCount() > 2 && !exec->argument(2).isNull() && !exec->argument(2).isUndefined()) {
        if (!exec->argument(2).isObject()) {
            setDOMException(exec, TYPE_MISMATCH_ERR);
            return jsUndefined();
        }
        successCallback = JSEntryCallback::create(asObject(exec->argument(2)), globalObject());
    }
    RefPtr<ErrorCallback> errorCallback;
    if (exec->argumentCount() > 3 && !exec->argument(3).isNull() && !exec->argument(3).isUndefined()) {
        if (!exec->argument(3).isObject()) {
            setDOMException(exec, TYPE_MISMATCH_ERR);
            return jsUndefined();
        }
        errorCallback = JSErrorCallback::create(asObject(exec->argument(3)), globalObject());
    }

    imp->getDirectory(path, flags, successCallback, errorCallback);
    return jsUndefined();
}

} // namespace WebCore

#endif // ENABLE(FILE_SYSTEM)
