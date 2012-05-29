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
#include "JSLazyEventListener.h"

#include "ContentSecurityPolicy.h"
#include "Frame.h"
#include "JSNode.h"
#include <runtime/FunctionConstructor.h>
#include <runtime/JSFunction.h>
#include <runtime/JSLock.h>
#include <wtf/RefCountedLeakCounter.h>

using namespace JSC;

namespace WebCore {

#ifndef NDEBUG
static WTF::RefCountedLeakCounter eventListenerCounter("JSLazyEventListener");
#endif

JSLazyEventListener::JSLazyEventListener(const String& functionName, const String& eventParameterName, const String& code, Node* node, const String& sourceURL, int lineNumber, JSObject* wrapper, DOMWrapperWorld* isolatedWorld)
    : JSEventListener(0, wrapper, true, isolatedWorld)
    , m_functionName(functionName)
    , m_eventParameterName(eventParameterName)
    , m_code(code)
    , m_sourceURL(sourceURL)
    , m_lineNumber(lineNumber)
    , m_originalNode(node)
{
    // We don't retain the original node because we assume it
    // will stay alive as long as this handler object is around
    // and we need to avoid a reference cycle. If JS transfers
    // this handler to another node, initializeJSFunction will
    // be called and then originalNode is no longer needed.

    // A JSLazyEventListener can be created with a line number of zero when it is created with
    // a setAttribute call from JavaScript, so make the line number 1 in that case.
    if (m_lineNumber == 0)
        m_lineNumber = 1;

#ifndef NDEBUG
    eventListenerCounter.increment();
#endif
}

JSLazyEventListener::~JSLazyEventListener()
{
#ifndef NDEBUG
    eventListenerCounter.decrement();
#endif
}

JSObject* JSLazyEventListener::initializeJSFunction(ScriptExecutionContext* executionContext) const
{
    ASSERT(executionContext);
    ASSERT(executionContext->isDocument());
    if (!executionContext)
        return 0;

    Document* document = static_cast<Document*>(executionContext);

    if (!document->frame())
        return 0;

    if (!document->contentSecurityPolicy()->allowInlineEventHandlers())
        return 0;

    ScriptController* script = document->frame()->script();
    if (!script->canExecuteScripts(AboutToExecuteScript) || script->isPaused())
        return 0;

    JSDOMGlobalObject* globalObject = toJSDOMGlobalObject(executionContext, isolatedWorld());
    if (!globalObject)
        return 0;

    ExecState* exec = globalObject->globalExec();

    MarkedArgumentBuffer args;
    args.append(jsNontrivialString(exec, stringToUString(m_eventParameterName)));
    args.append(jsString(exec, m_code));

    JSObject* jsFunction = constructFunction(exec, exec->lexicalGlobalObject(), args, Identifier(exec, stringToUString(m_functionName)), stringToUString(m_sourceURL), m_lineNumber); // FIXME: is globalExec ok?
    if (exec->hadException()) {
        exec->clearException();
        return 0;
    }

    JSFunction* listenerAsFunction = static_cast<JSFunction*>(jsFunction);
    if (m_originalNode) {
        if (!wrapper()) {
            // Ensure that 'node' has a JavaScript wrapper to mark the event listener we're creating.
            JSLock lock(SilenceAssertionsOnly);
            // FIXME: Should pass the global object associated with the node
            setWrapper(exec->globalData(), asObject(toJS(exec, globalObject, m_originalNode)));
        }

        // Add the event's home element to the scope
        // (and the document, and the form - see JSHTMLElement::eventHandlerScope)
        listenerAsFunction->setScope(exec->globalData(), static_cast<JSNode*>(wrapper())->pushEventHandlerScope(exec, listenerAsFunction->scope()));
    }

    // Since we only parse once, there's no need to keep data used for parsing around anymore.
    m_functionName = String();
    m_code = String();
    m_eventParameterName = String();
    m_sourceURL = String();
    return jsFunction;
}

} // namespace WebCore
