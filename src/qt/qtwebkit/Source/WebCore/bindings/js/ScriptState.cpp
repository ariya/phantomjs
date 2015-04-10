/*
 * Copyright (C) 2009, 2011 Google Inc. All rights reserved.
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
#include "ScriptState.h"

#include "Frame.h"
#include "JSDOMWindowBase.h"
#include "Node.h"
#include "Page.h"
#include "ScriptController.h"
#include "WorkerGlobalScope.h"
#include "WorkerScriptController.h"
#include <heap/StrongInlines.h>
#include <interpreter/CallFrame.h>
#include <runtime/JSGlobalObject.h>

#if ENABLE(WORKERS)
#include "JSWorkerGlobalScope.h"
#endif

namespace WebCore {

DOMWindow* domWindowFromScriptState(ScriptState* scriptState)
{
    JSC::JSGlobalObject* globalObject = scriptState->lexicalGlobalObject();
    if (!globalObject->inherits(&JSDOMWindowBase::s_info))
        return 0;
    return JSC::jsCast<JSDOMWindowBase*>(globalObject)->impl();
}

ScriptExecutionContext* scriptExecutionContextFromScriptState(ScriptState* scriptState)
{
    JSC::JSGlobalObject* globalObject = scriptState->lexicalGlobalObject();
    if (!globalObject->inherits(&JSDOMGlobalObject::s_info))
        return 0;
    return JSC::jsCast<JSDOMGlobalObject*>(globalObject)->scriptExecutionContext();
}

ScriptState* mainWorldScriptState(Frame* frame)
{
    if (!frame)
        return 0;
    JSDOMWindowShell* shell = frame->script()->windowShell(mainThreadNormalWorld());
    return shell->window()->globalExec();
}

ScriptState* scriptStateFromNode(DOMWrapperWorld* world, Node* node)
{
    if (!node)
        return 0;
    Document* document = node->document();
    if (!document)
        return 0;
    Frame* frame = document->frame();
    if (!frame)
        return 0;
    if (!frame->script()->canExecuteScripts(NotAboutToExecuteScript))
        return 0;
    return frame->script()->globalObject(world)->globalExec();
}

ScriptState* scriptStateFromPage(DOMWrapperWorld* world, Page* page)
{
    return page->mainFrame()->script()->globalObject(world)->globalExec();
}

#if ENABLE(WORKERS)
ScriptState* scriptStateFromWorkerGlobalScope(WorkerGlobalScope* workerGlobalScope)
{
    return workerGlobalScope->script()->workerGlobalScopeWrapper()->globalExec();
}
#endif

}
