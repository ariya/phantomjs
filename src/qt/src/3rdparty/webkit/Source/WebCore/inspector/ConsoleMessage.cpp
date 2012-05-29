/*
 * Copyright (C) 2007, 2008 Apple Inc. All rights reserved.
 * Copyright (C) 2008 Matt Lilek <webkit@mattlilek.com>
 * Copyright (C) 2009, 2010 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "ConsoleMessage.h"

#if ENABLE(INSPECTOR)

#include "Console.h"
#include "InjectedScript.h"
#include "InjectedScriptManager.h"
#include "InspectorFrontend.h"
#include "InspectorValues.h"
#include "ScriptArguments.h"
#include "ScriptCallStack.h"
#include "ScriptValue.h"

namespace WebCore {

ConsoleMessage::ConsoleMessage(MessageSource s, MessageType t, MessageLevel l, const String& m, unsigned li, const String& u)
    : m_source(s)
    , m_type(t)
    , m_level(l)
    , m_message(m)
    , m_line(li)
    , m_url(u)
    , m_repeatCount(1)
    , m_requestId(0)
{
}

ConsoleMessage::ConsoleMessage(MessageSource s, MessageType t, MessageLevel l, const String& m, PassRefPtr<ScriptArguments> arguments, PassRefPtr<ScriptCallStack> callStack)
    : m_source(s)
    , m_type(t)
    , m_level(l)
    , m_message(m)
    , m_arguments(arguments)
    , m_callStack(callStack)
    , m_line(0)
    , m_url()
    , m_repeatCount(1)
    , m_requestId(0)
{
}

ConsoleMessage::ConsoleMessage(MessageSource s, MessageType t, MessageLevel l, const String& m, const String& responseUrl, unsigned long identifier)
    : m_source(s)
    , m_type(t)
    , m_level(l)
    , m_message(m)
    , m_line(0)
    , m_url(responseUrl)
    , m_repeatCount(1)
    , m_requestId(identifier)
{
}

ConsoleMessage::~ConsoleMessage()
{
}

// Keep in sync with inspector/front-end/ConsoleView.js
static String messageSourceValue(MessageSource source)
{
    switch (source) {
    case HTMLMessageSource: return "html";
    case XMLMessageSource: return "xml";
    case JSMessageSource: return "javascript";
    case CSSMessageSource: return "css";
    case OtherMessageSource: return "other";
    }
    return "other";
}

static String messageTypeValue(MessageType type)
{
    switch (type) {
    case LogMessageType: return "log";
    case ObjectMessageType: return "other";
    case TraceMessageType: return "trace";
    case StartGroupMessageType: return "startGroup";
    case StartGroupCollapsedMessageType: return "startGroupCollapsed";
    case EndGroupMessageType: return "endGroup";
    case AssertMessageType: return "assert";
    case UncaughtExceptionMessageType: return "uncaughtException";
    case NetworkErrorMessageType: return "networkError";
    }
    return "other";
}

static String messageLevelValue(MessageLevel level)
{
    switch (level) {
    case TipMessageLevel: return "tip";
    case LogMessageLevel: return "log";
    case WarningMessageLevel: return "warning";
    case ErrorMessageLevel: return "error";
    case DebugMessageLevel: return "debug";
    }
    return "log";
}

void ConsoleMessage::addToFrontend(InspectorFrontend::Console* frontend, InjectedScriptManager* injectedScriptManager)
{
    RefPtr<InspectorObject> jsonObj = InspectorObject::create();
    jsonObj->setString("source", messageSourceValue(m_source));
    jsonObj->setString("type", messageTypeValue(m_type));
    jsonObj->setString("level", messageLevelValue(m_level));
    jsonObj->setNumber("line", static_cast<int>(m_line));
    jsonObj->setString("url", m_url);
    jsonObj->setNumber("repeatCount", static_cast<int>(m_repeatCount));
    jsonObj->setString("text", m_message);
    if (m_type == NetworkErrorMessageType) 
        jsonObj->setNumber("networkIdentifier", m_requestId);
    if (m_arguments && m_arguments->argumentCount()) {
        InjectedScript injectedScript = injectedScriptManager->injectedScriptFor(m_arguments->globalState());
        if (!injectedScript.hasNoValue()) {
            RefPtr<InspectorArray> jsonArgs = InspectorArray::create();
            for (unsigned i = 0; i < m_arguments->argumentCount(); ++i) {
                RefPtr<InspectorValue> inspectorValue = injectedScript.wrapObject(m_arguments->argumentAt(i), "console");
                if (!inspectorValue) {
                    ASSERT_NOT_REACHED();
                    return;
                }
                jsonArgs->pushValue(inspectorValue);
            }
            jsonObj->setArray("parameters", jsonArgs);
        }
    }
    if (m_callStack)
        jsonObj->setArray("stackTrace", m_callStack->buildInspectorArray());
    frontend->messageAdded(jsonObj);
}

void ConsoleMessage::updateRepeatCountInConsole(InspectorFrontend::Console* frontend)
{
    frontend->messageRepeatCountUpdated(m_repeatCount);
}

bool ConsoleMessage::isEqual(ConsoleMessage* msg) const
{
    if (m_arguments) {
        if (!m_arguments->isEqual(msg->m_arguments.get()))
            return false;
    } else if (msg->m_arguments)
        return false;

    if (m_callStack) {
        if (!m_callStack->isEqual(msg->m_callStack.get()))
            return false;
    } else if (msg->m_callStack)
        return false;

    return msg->m_source == m_source
        && msg->m_type == m_type
        && msg->m_level == m_level
        && msg->m_message == m_message
        && msg->m_line == m_line
        && msg->m_url == m_url
        && msg->m_requestId == m_requestId;
}

} // namespace WebCore

#endif // ENABLE(INSPECTOR)
