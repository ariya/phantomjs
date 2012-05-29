/*
 * Copyright (C) 2008 Apple Inc. All Rights Reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"

#include "JSConsole.h"

#include "Console.h"
#include "JSScriptProfile.h"
#include "ScriptCallStack.h"
#include "ScriptCallStackFactory.h"
#include "ScriptProfile.h"
#include <runtime/JSArray.h>
#include <wtf/OwnPtr.h>

using namespace JSC;

namespace WebCore {

#if ENABLE(JAVASCRIPT_DEBUGGER)

typedef Vector<RefPtr<ScriptProfile> > ProfilesArray;

JSValue JSConsole::profiles(ExecState* exec) const
{
    const ProfilesArray& profiles = impl()->profiles();
    MarkedArgumentBuffer list;

    ProfilesArray::const_iterator end = profiles.end();
    for (ProfilesArray::const_iterator iter = profiles.begin(); iter != end; ++iter)
        list.append(toJS(exec, globalObject(), iter->get()));

    return constructArray(exec, globalObject(), list);
}

JSValue JSConsole::profile(ExecState* exec)
{
    RefPtr<ScriptCallStack> callStack(createScriptCallStack(exec, 1));
    const String& title = valueToStringWithUndefinedOrNullCheck(exec, exec->argument(0));
    if (exec->hadException())
        return jsUndefined();

    impl()->profile(title, exec, callStack);
    return jsUndefined();
}

JSValue JSConsole::profileEnd(ExecState* exec)
{
    RefPtr<ScriptCallStack> callStack(createScriptCallStack(exec, 1));
    const String& title = valueToStringWithUndefinedOrNullCheck(exec, exec->argument(0));
    if (exec->hadException())
        return jsUndefined();

    impl()->profileEnd(title, exec, callStack);
    return jsUndefined();
}

#endif

} // namespace WebCore
