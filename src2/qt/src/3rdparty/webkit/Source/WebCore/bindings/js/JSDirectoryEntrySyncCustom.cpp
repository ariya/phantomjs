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

#include "JSDirectoryEntrySync.h"

#include "JSDOMBinding.h"
#include "JSEntryCallback.h"
#include "JSErrorCallback.h"
#include "JSFileEntrySync.h"
#include "JSWebKitFlags.h"
#include <wtf/Assertions.h>

using namespace JSC;

namespace WebCore {

static PassRefPtr<WebKitFlags> getFlags(ExecState* exec, const JSValue& argument)
{
    if (argument.isNull() || argument.isUndefined() || !argument.isObject())
        return 0;
    if (argument.inherits(&JSWebKitFlags::s_info))
        return toFlags(argument);

    RefPtr<WebKitFlags> flags;
    JSObject* object = argument.getObject();
    flags = WebKitFlags::create();
    JSValue jsCreate = object->get(exec, Identifier(exec, "create"));
    flags->setCreate(jsCreate.toBoolean(exec));
    JSValue jsExclusive = object->get(exec, Identifier(exec, "exclusive"));
    flags->setExclusive(jsExclusive.toBoolean(exec));
    return flags;
}

JSValue JSDirectoryEntrySync::getFile(ExecState* exec)
{
    DirectoryEntrySync* imp = static_cast<DirectoryEntrySync*>(impl());
    const String& path = valueToStringWithUndefinedOrNullCheck(exec, exec->argument(0));
    if (exec->hadException())
        return jsUndefined();

    RefPtr<WebKitFlags> flags = getFlags(exec, exec->argument(1));
    if (exec->hadException())
        return jsUndefined();

    ExceptionCode ec = 0;
    JSC::JSValue result = toJS(exec, this->globalObject(), WTF::getPtr(imp->getFile(path, flags, ec)));
    setDOMException(exec, ec);
    return result;
}

JSValue JSDirectoryEntrySync::getDirectory(ExecState* exec)
{
    DirectoryEntrySync* imp = static_cast<DirectoryEntrySync*>(impl());
    const String& path = valueToStringWithUndefinedOrNullCheck(exec, exec->argument(0));
    if (exec->hadException())
        return jsUndefined();

    RefPtr<WebKitFlags> flags = getFlags(exec, exec->argument(1));
    if (exec->hadException())
        return jsUndefined();

    ExceptionCode ec = 0;
    JSC::JSValue result = toJS(exec, this->globalObject(), WTF::getPtr(imp->getDirectory(path, flags, ec)));
    setDOMException(exec, ec);
    return result;
}

} // namespace WebCore

#endif // ENABLE(FILE_SYSTEM)
