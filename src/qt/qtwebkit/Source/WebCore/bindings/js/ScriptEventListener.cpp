/*
 * Copyright (C) 2009 Google Inc. All rights reserved.
 * Copyright (C) 2011 Research In Motion Limited. All rights reserved.
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
#include "ScriptEventListener.h"

#include "Attribute.h"
#include "Document.h"
#include "EventListener.h"
#include "JSNode.h"
#include "Frame.h"
#include "ScriptController.h"
#include <runtime/Executable.h>
#include <runtime/JSFunction.h>
#include <runtime/JSLock.h>

using namespace JSC;

namespace WebCore {

static const String& eventParameterName(bool isSVGEvent)
{
    DEFINE_STATIC_LOCAL(const String, eventString, (ASCIILiteral("event")));
    DEFINE_STATIC_LOCAL(const String, evtString, (ASCIILiteral("evt")));
    return isSVGEvent ? evtString : eventString;
}

PassRefPtr<JSLazyEventListener> createAttributeEventListener(Node* node, const QualifiedName& name, const AtomicString& value)
{
    ASSERT(node);
    if (value.isNull())
        return 0;

    TextPosition position = TextPosition::minimumPosition();
    String sourceURL;
    
    // FIXME: We should be able to provide accurate source information for frameless documents, too (e.g. for importing nodes from XMLHttpRequest.responseXML).
    if (Frame* frame = node->document()->frame()) {
        ScriptController* scriptController = frame->script();
        if (!scriptController->canExecuteScripts(AboutToExecuteScript))
            return 0;

        position = scriptController->eventHandlerPosition();
        sourceURL = node->document()->url().string();
    }

    return JSLazyEventListener::create(name.localName().string(), eventParameterName(node->isSVGElement()), value, node, sourceURL, position, 0, mainThreadNormalWorld());
}

PassRefPtr<JSLazyEventListener> createAttributeEventListener(Frame* frame, const QualifiedName& name, const AtomicString& value)
{
    if (!frame)
        return 0;

    if (value.isNull())
        return 0;

    ScriptController* scriptController = frame->script();
    if (!scriptController->canExecuteScripts(AboutToExecuteScript))
        return 0;

    TextPosition position = scriptController->eventHandlerPosition();
    String sourceURL = frame->document()->url().string();
    JSObject* wrapper = toJSDOMWindow(frame, mainThreadNormalWorld());
    return JSLazyEventListener::create(name.localName().string(), eventParameterName(frame->document()->isSVGDocument()), value, 0, sourceURL, position, wrapper, mainThreadNormalWorld());
}

String eventListenerHandlerBody(Document* document, EventListener* eventListener)
{
    const JSEventListener* jsListener = JSEventListener::cast(eventListener);
    ASSERT(jsListener);
    if (!jsListener)
        return "";
    JSLockHolder lock(jsListener->isolatedWorld()->vm());
    JSC::JSObject* jsFunction = jsListener->jsFunction(document);
    if (!jsFunction)
        return "";
    ScriptState* scriptState = scriptStateFromNode(jsListener->isolatedWorld(), document);
    return jsFunction->toString(scriptState)->value(scriptState);
}

ScriptValue eventListenerHandler(Document* document, EventListener* eventListener)
{
    const JSEventListener* jsListener = JSEventListener::cast(eventListener);
    ASSERT(jsListener);
    if (!jsListener)
        return ScriptValue();
    JSLockHolder lock(jsListener->isolatedWorld()->vm());
    JSC::JSObject* jsFunction = jsListener->jsFunction(document);
    if (!jsFunction)
        return ScriptValue();
    return ScriptValue(*jsListener->isolatedWorld()->vm(), jsFunction);
}

ScriptState* eventListenerHandlerScriptState(Frame* frame, EventListener* eventListener)
{
    const JSEventListener* jsListener = JSEventListener::cast(eventListener);
    ASSERT(jsListener);
    if (!jsListener)
        return 0;
    if (!frame->script()->canExecuteScripts(NotAboutToExecuteScript))
        return 0;
    DOMWrapperWorld* world = jsListener->isolatedWorld();
    return frame->script()->globalObject(world)->globalExec();
}

bool eventListenerHandlerLocation(Document* document, EventListener* eventListener, String& sourceName, String& scriptId, int& lineNumber)
{
    const JSEventListener* jsListener = JSEventListener::cast(eventListener);
    ASSERT(jsListener);
    if (!jsListener)
        return false;
    JSLockHolder lock(jsListener->isolatedWorld()->vm());
    JSC::JSObject* jsObject = jsListener->jsFunction(document);
    if (!jsObject)
        return false;
    JSC::JSFunction* jsFunction = jsDynamicCast<JSFunction*>(jsObject);
    if (!jsFunction || jsFunction->isHostFunction())
        return false;
    JSC::FunctionExecutable* funcExecutable = jsFunction->jsExecutable();
    if (!funcExecutable)
        return false;
    lineNumber = funcExecutable->lineNo() - 1;
    intptr_t funcSourceId = funcExecutable->sourceID();
    scriptId = funcSourceId == SourceProvider::nullID ? "" : String::number(funcSourceId);
    sourceName = funcExecutable->sourceURL();
    return true;
}

} // namespace WebCore
