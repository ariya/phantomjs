/*
 *  Copyright (C) 2001 Peter Kelly (pmk@post.com)
 *  Copyright (C) 2003, 2004, 2005, 2006, 2007, 2008, 2009 Apple Inc. All Rights Reserved.
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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "config.h"
#include "JSEventListener.h"

#include "Event.h"
#include "Frame.h"
#include "JSEvent.h"
#include "JSEventTarget.h"
#include "JSMainThreadExecState.h"
#include "WorkerContext.h"
#include <runtime/JSLock.h>
#include <wtf/RefCountedLeakCounter.h>

using namespace JSC;

namespace WebCore {

JSEventListener::JSEventListener(JSObject* function, JSObject* wrapper, bool isAttribute, DOMWrapperWorld* isolatedWorld)
    : EventListener(JSEventListenerType)
    , m_wrapper(*isolatedWorld->globalData(), wrapper)
    , m_isAttribute(isAttribute)
    , m_isolatedWorld(isolatedWorld)
{
    if (wrapper)
        m_jsFunction.set(*m_isolatedWorld->globalData(), wrapper, function);
    else
        ASSERT(!function);

}

JSEventListener::~JSEventListener()
{
}

JSObject* JSEventListener::initializeJSFunction(ScriptExecutionContext*) const
{
    ASSERT_NOT_REACHED();
    return 0;
}

void JSEventListener::visitJSFunction(SlotVisitor& visitor)
{
    if (m_jsFunction)
        visitor.append(&m_jsFunction);
}

void JSEventListener::handleEvent(ScriptExecutionContext* scriptExecutionContext, Event* event)
{
    ASSERT(scriptExecutionContext);
    if (!scriptExecutionContext || scriptExecutionContext->isJSExecutionForbidden())
        return;

    JSLock lock(SilenceAssertionsOnly);

    JSObject* jsFunction = this->jsFunction(scriptExecutionContext);
    if (!jsFunction)
        return;

    JSDOMGlobalObject* globalObject = toJSDOMGlobalObject(scriptExecutionContext, m_isolatedWorld.get());
    if (!globalObject)
        return;

    if (scriptExecutionContext->isDocument()) {
        JSDOMWindow* window = static_cast<JSDOMWindow*>(globalObject);
        Frame* frame = window->impl()->frame();
        if (!frame)
            return;
        // The window must still be active in its frame. See <https://bugs.webkit.org/show_bug.cgi?id=21921>.
        // FIXME: A better fix for this may be to change DOMWindow::frame() to not return a frame the detached window used to be in.
        if (frame->domWindow() != window->impl())
            return;
        // FIXME: Is this check needed for other contexts?
        ScriptController* script = frame->script();
        if (!script->canExecuteScripts(AboutToExecuteScript) || script->isPaused())
            return;
    }

    ExecState* exec = globalObject->globalExec();
    JSValue handleEventFunction = jsFunction->get(exec, Identifier(exec, "handleEvent"));

    CallData callData;
    CallType callType = getCallData(handleEventFunction, callData);
    if (callType == CallTypeNone) {
        handleEventFunction = JSValue();
        callType = jsFunction->getCallData(callData);
    }

    if (callType != CallTypeNone) {
        ref();

        MarkedArgumentBuffer args;
        args.append(toJS(exec, globalObject, event));

        Event* savedEvent = globalObject->currentEvent();
        globalObject->setCurrentEvent(event);

        JSGlobalData& globalData = globalObject->globalData();
        DynamicGlobalObjectScope globalObjectScope(globalData, globalData.dynamicGlobalObject ? globalData.dynamicGlobalObject : globalObject);

        globalData.timeoutChecker.start();
        JSValue retval;
        if (handleEventFunction) {
            retval = scriptExecutionContext->isDocument()
                ? JSMainThreadExecState::call(exec, handleEventFunction, callType, callData, jsFunction, args)
                : JSC::call(exec, handleEventFunction, callType, callData, jsFunction, args);
        } else {
            JSValue currentTarget = toJS(exec, globalObject, event->currentTarget());
            retval = scriptExecutionContext->isDocument()
                ? JSMainThreadExecState::call(exec, jsFunction, callType, callData, currentTarget, args)
                : JSC::call(exec, jsFunction, callType, callData, currentTarget, args);
        }
        globalData.timeoutChecker.stop();

        globalObject->setCurrentEvent(savedEvent);

#if ENABLE(WORKERS)
        if (scriptExecutionContext->isWorkerContext()) {
            bool terminatorCausedException = (exec->hadException() && exec->exception().isObject() && asObject(exec->exception())->exceptionType() == Terminated);
            if (terminatorCausedException || globalData.terminator.shouldTerminate())
                static_cast<WorkerContext*>(scriptExecutionContext)->script()->forbidExecution();
        }
#endif

        if (exec->hadException()) {
            event->target()->uncaughtExceptionInEventHandler();
            reportCurrentException(exec);
        } else {
            if (!retval.isUndefinedOrNull() && event->storesResultAsString())
                event->storeResult(ustringToString(retval.toString(exec)));
            if (m_isAttribute) {
                bool retvalbool;
                if (retval.getBoolean(retvalbool) && !retvalbool)
                    event->preventDefault();
            }
        }

        deref();
    }
}

bool JSEventListener::virtualisAttribute() const
{
    return m_isAttribute;
}

bool JSEventListener::operator==(const EventListener& listener)
{
    if (const JSEventListener* jsEventListener = JSEventListener::cast(&listener))
        return m_jsFunction == jsEventListener->m_jsFunction && m_isAttribute == jsEventListener->m_isAttribute;
    return false;
}

} // namespace WebCore
