/*
 *  Copyright (C) 2000 Harri Porten (porten@kde.org)
 *  Copyright (C) 2006 Jon Shier (jshier@iastate.edu)
 *  Copyright (C) 2003, 2004, 2005, 2006, 2007, 2008, 2009 Apple Inc. All rights reseved.
 *  Copyright (C) 2006 Alexey Proskuryakov (ap@webkit.org)
 *  Copyright (C) 2009 Google Inc. All rights reseved.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301
 *  USA
 */

#include "config.h"
#include "ScheduledAction.h"

#include "ContentSecurityPolicy.h"
#include "DOMWindow.h"
#include "Document.h"
#include "Frame.h"
#include "FrameLoader.h"
#include "JSDOMBinding.h"
#include "JSDOMWindow.h"
#include "JSMainThreadExecState.h"
#include "ScriptController.h"
#include "ScriptExecutionContext.h"
#include "ScriptSourceCode.h"
#include "ScriptValue.h"
#include <runtime/JSLock.h>

#if ENABLE(WORKERS)
#include "JSWorkerContext.h"
#include "WorkerContext.h"
#include "WorkerThread.h"
#endif

using namespace JSC;

namespace WebCore {

PassOwnPtr<ScheduledAction> ScheduledAction::create(ExecState* exec, DOMWrapperWorld* isolatedWorld, ContentSecurityPolicy* policy)
{
    JSValue v = exec->argument(0);
    CallData callData;
    if (getCallData(v, callData) == CallTypeNone) {
        if (policy && !policy->allowEval())
            return nullptr;
        UString string = v.toString(exec);
        if (exec->hadException())
            return nullptr;
        return adoptPtr(new ScheduledAction(ustringToString(string), isolatedWorld));
    }

    return adoptPtr(new ScheduledAction(exec, v, isolatedWorld));
}

ScheduledAction::ScheduledAction(ExecState* exec, JSValue function, DOMWrapperWorld* isolatedWorld)
    : m_function(exec->globalData(), function)
    , m_isolatedWorld(isolatedWorld)
{
    // setTimeout(function, interval, arg0, arg1...).
    // Start at 2 to skip function and interval.
    for (size_t i = 2; i < exec->argumentCount(); ++i)
        m_args.append(Strong<JSC::Unknown>(exec->globalData(), exec->argument(i)));
}

void ScheduledAction::execute(ScriptExecutionContext* context)
{
    if (context->isDocument())
        execute(static_cast<Document*>(context));
#if ENABLE(WORKERS)
    else {
        ASSERT(context->isWorkerContext());
        execute(static_cast<WorkerContext*>(context));
    }
#else
    ASSERT(context->isDocument());
#endif
}

void ScheduledAction::executeFunctionInContext(JSGlobalObject* globalObject, JSValue thisValue, ScriptExecutionContext* context)
{
    ASSERT(m_function);
    JSLock lock(SilenceAssertionsOnly);

    CallData callData;
    CallType callType = getCallData(m_function.get(), callData);
    if (callType == CallTypeNone)
        return;

    ExecState* exec = globalObject->globalExec();

    MarkedArgumentBuffer args;
    size_t size = m_args.size();
    for (size_t i = 0; i < size; ++i)
        args.append(m_args[i].get());

    globalObject->globalData().timeoutChecker.start();
    if (context->isDocument())
        JSMainThreadExecState::call(exec, m_function.get(), callType, callData, thisValue, args);
    else
        JSC::call(exec, m_function.get(), callType, callData, thisValue, args);
    globalObject->globalData().timeoutChecker.stop();

    if (exec->hadException())
        reportCurrentException(exec);
}

void ScheduledAction::execute(Document* document)
{
    JSDOMWindow* window = toJSDOMWindow(document->frame(), m_isolatedWorld.get());
    if (!window)
        return;

    RefPtr<Frame> frame = window->impl()->frame();
    if (!frame || !frame->script()->canExecuteScripts(AboutToExecuteScript))
        return;

    frame->script()->setProcessingTimerCallback(true);

    if (m_function) {
        executeFunctionInContext(window, window->shell(), document);
        Document::updateStyleForAllDocuments();
    } else
        frame->script()->executeScriptInWorld(m_isolatedWorld.get(), m_code);

    frame->script()->setProcessingTimerCallback(false);
}

#if ENABLE(WORKERS)
void ScheduledAction::execute(WorkerContext* workerContext)
{
    // In a Worker, the execution should always happen on a worker thread.
    ASSERT(workerContext->thread()->threadID() == currentThread());

    WorkerScriptController* scriptController = workerContext->script();

    if (m_function) {
        JSWorkerContext* contextWrapper = scriptController->workerContextWrapper();
        executeFunctionInContext(contextWrapper, contextWrapper, workerContext);
    } else {
        ScriptSourceCode code(m_code, workerContext->url());
        scriptController->evaluate(code);
    }
}
#endif // ENABLE(WORKERS)

} // namespace WebCore
