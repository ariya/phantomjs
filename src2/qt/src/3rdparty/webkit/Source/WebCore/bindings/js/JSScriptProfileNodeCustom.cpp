/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
 * Copyright (C) 2010 Google Inc. All rights reserved.
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

#include "JSScriptProfileNode.h"

#if ENABLE(JAVASCRIPT_DEBUGGER)
#include <profiler/ProfileNode.h>
#endif

#include <runtime/JSArray.h>

using namespace JSC;

namespace WebCore {

#if ENABLE(JAVASCRIPT_DEBUGGER)

JSValue JSScriptProfileNode::callUID(ExecState*) const
{
    JSValue result = jsNumber(impl()->callIdentifier().hash());
    return result;
}

typedef Vector<RefPtr<ProfileNode> > ProfileNodesList;

JSValue JSScriptProfileNode::children(ExecState* exec) const
{
    const ProfileNodesList& children = impl()->children();
    MarkedArgumentBuffer list;

    ProfileNodesList::const_iterator end = children.end();
    for (ProfileNodesList::const_iterator iter = children.begin(); iter != end; ++iter)
        list.append(toJS(exec, globalObject(), iter->get()));

    return constructArray(exec, globalObject(), list);
}

#endif

} // namespace WebCore
