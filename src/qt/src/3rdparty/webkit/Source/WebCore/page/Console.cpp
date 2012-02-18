/*
 * Copyright (C) 2007 Apple Inc. All rights reserved.
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
#include "Console.h"

#include "Chrome.h"
#include "ChromeClient.h"
#include "Frame.h"
#include "FrameLoader.h"
#include "FrameTree.h"
#include "InspectorConsoleInstrumentation.h"
#include "InspectorController.h"
#include "MemoryInfo.h"
#include "Page.h"
#include "PageGroup.h"
#include "PlatformString.h"
#include "ScriptArguments.h"
#include "ScriptCallStack.h"
#include "ScriptProfile.h"
#include "ScriptProfiler.h"
#include "ScriptValue.h"
#include <stdio.h>
#include <wtf/UnusedParam.h>
#include <wtf/text/CString.h>

namespace WebCore {

Console::Console(Frame* frame)
    : m_frame(frame)
{
}

Frame* Console::frame() const
{
    return m_frame;
}

void Console::disconnectFrame()
{
    if (m_memory)
        m_memory = 0;
    m_frame = 0;
}

static void printSourceURLAndLine(const String& sourceURL, unsigned lineNumber)
{
    if (!sourceURL.isEmpty()) {
        if (lineNumber > 0)
            printf("%s:%d: ", sourceURL.utf8().data(), lineNumber);
        else
            printf("%s: ", sourceURL.utf8().data());
    }
}

static void printMessageSourceAndLevelPrefix(MessageSource source, MessageLevel level)
{
    const char* sourceString;
    switch (source) {
    case HTMLMessageSource:
        sourceString = "HTML";
        break;
    case XMLMessageSource:
        sourceString = "XML";
        break;
    case JSMessageSource:
        sourceString = "JS";
        break;
    case CSSMessageSource:
        sourceString = "CSS";
        break;
    case OtherMessageSource:
        sourceString = "OTHER";
        break;
    default:
        ASSERT_NOT_REACHED();
        sourceString = "UNKNOWN";
        break;
    }

    const char* levelString;
    switch (level) {
    case TipMessageLevel:
        levelString = "TIP";
        break;
    case LogMessageLevel:
        levelString = "LOG";
        break;
    case WarningMessageLevel:
        levelString = "WARN";
        break;
    case ErrorMessageLevel:
        levelString = "ERROR";
        break;
    case DebugMessageLevel:
        levelString = "DEBUG";
        break;
    default:
        ASSERT_NOT_REACHED();
        levelString = "UNKNOWN";
        break;
    }

    printf("%s %s:", sourceString, levelString);
}

void Console::addMessage(MessageSource source, MessageType type, MessageLevel level, const String& message, unsigned lineNumber, const String& sourceURL)
{
    addMessage(source, type, level, message, lineNumber, sourceURL, 0);
}

void Console::addMessage(MessageSource source, MessageType type, MessageLevel level, const String& message, unsigned lineNumber, const String& sourceURL, PassRefPtr<ScriptCallStack> callStack)
{
    Page* page = this->page();
    if (!page)
        return;

    page->chrome()->client()->addMessageToConsole(source, type, level, message, lineNumber, sourceURL);

    if (callStack)
        InspectorInstrumentation::addMessageToConsole(page, source, type, level, message, 0, callStack);
    else
        InspectorInstrumentation::addMessageToConsole(page, source, type, level, message, lineNumber, sourceURL);

    if (!Console::shouldPrintExceptions())
        return;

    printSourceURLAndLine(sourceURL, lineNumber);
    printMessageSourceAndLevelPrefix(source, level);

    printf(" %s\n", message.utf8().data());
}

void Console::addMessage(MessageType type, MessageLevel level, PassRefPtr<ScriptArguments> prpArguments,  PassRefPtr<ScriptCallStack> prpCallStack, bool acceptNoArguments)
{
    RefPtr<ScriptArguments> arguments = prpArguments;
    RefPtr<ScriptCallStack> callStack = prpCallStack;

    Page* page = this->page();
    if (!page)
        return;

    const ScriptCallFrame& lastCaller = callStack->at(0);

    if (!acceptNoArguments && !arguments->argumentCount())
        return;

    if (Console::shouldPrintExceptions()) {
        printSourceURLAndLine(lastCaller.sourceURL(), 0);
        printMessageSourceAndLevelPrefix(JSMessageSource, level);

        for (unsigned i = 0; i < arguments->argumentCount(); ++i) {
            String argAsString;
            if (arguments->argumentAt(i).getString(arguments->globalState(), argAsString))
                printf(" %s", argAsString.utf8().data());
        }
        printf("\n");
    }

    String message;
    if (arguments->getFirstArgumentAsString(message))
        page->chrome()->client()->addMessageToConsole(JSMessageSource, type, level, message, lastCaller.lineNumber(), lastCaller.sourceURL());

    InspectorInstrumentation::addMessageToConsole(page, JSMessageSource, type, level, message, arguments, callStack);
}

void Console::debug(PassRefPtr<ScriptArguments> arguments, PassRefPtr<ScriptCallStack> callStack)
{
    // In Firebug, console.debug has the same behavior as console.log. So we'll do the same.
    log(arguments, callStack);
}

void Console::error(PassRefPtr<ScriptArguments> arguments, PassRefPtr<ScriptCallStack> callStack)
{
    addMessage(LogMessageType, ErrorMessageLevel, arguments, callStack);
}

void Console::info(PassRefPtr<ScriptArguments> arguments, PassRefPtr<ScriptCallStack> callStack)
{
    log(arguments, callStack);
}

void Console::log(PassRefPtr<ScriptArguments> arguments, PassRefPtr<ScriptCallStack> callStack)
{
    addMessage(LogMessageType, LogMessageLevel, arguments, callStack);
}

void Console::dir(PassRefPtr<ScriptArguments> arguments, PassRefPtr<ScriptCallStack> callStack)
{
    addMessage(ObjectMessageType, LogMessageLevel, arguments, callStack);
}

void Console::dirxml(PassRefPtr<ScriptArguments> arguments, PassRefPtr<ScriptCallStack> callStack)
{
    // The standard behavior of our console.log will print the DOM tree for nodes.
    log(arguments, callStack);
}

void Console::trace(PassRefPtr<ScriptArguments> arguments, PassRefPtr<ScriptCallStack> prpCallStack)
{
    RefPtr<ScriptCallStack> callStack = prpCallStack;
    addMessage(TraceMessageType, LogMessageLevel, arguments, callStack, true);

    if (!shouldPrintExceptions())
        return;

    printf("Stack Trace\n");
    for (unsigned i = 0; i < callStack->size(); ++i) {
        String functionName = String(callStack->at(i).functionName());
        printf("\t%s\n", functionName.utf8().data());
    }
}

void Console::assertCondition(bool condition, PassRefPtr<ScriptArguments> arguments, PassRefPtr<ScriptCallStack> callStack)
{
    if (condition)
        return;

    addMessage(AssertMessageType, ErrorMessageLevel, arguments, callStack, true);
}

void Console::count(PassRefPtr<ScriptArguments> arguments, PassRefPtr<ScriptCallStack> callStack)
{
    InspectorInstrumentation::consoleCount(page(), arguments, callStack);
}

void Console::markTimeline(PassRefPtr<ScriptArguments> arguments, PassRefPtr<ScriptCallStack>)
{
    InspectorInstrumentation::consoleMarkTimeline(page(), arguments);
}

#if ENABLE(JAVASCRIPT_DEBUGGER)

void Console::profile(const String& title, ScriptState* state, PassRefPtr<ScriptCallStack> callStack)
{
    Page* page = this->page();
    if (!page)
        return;

    // FIXME: log a console message when profiling is disabled.
    if (!InspectorInstrumentation::profilerEnabled(page))
        return;

    String resolvedTitle = title;
    if (title.isNull()) // no title so give it the next user initiated profile title.
        resolvedTitle = InspectorInstrumentation::getCurrentUserInitiatedProfileName(page, true);

    ScriptProfiler::start(state, resolvedTitle);

    const ScriptCallFrame& lastCaller = callStack->at(0);
    InspectorInstrumentation::addStartProfilingMessageToConsole(page, resolvedTitle, lastCaller.lineNumber(), lastCaller.sourceURL());
}

void Console::profileEnd(const String& title, ScriptState* state, PassRefPtr<ScriptCallStack> callStack)
{
    Page* page = this->page();
    if (!page)
        return;

    if (!InspectorInstrumentation::profilerEnabled(page))
        return;

    RefPtr<ScriptProfile> profile = ScriptProfiler::stop(state, title);
    if (!profile)
        return;

    m_profiles.append(profile);
    InspectorInstrumentation::addProfile(page, profile, callStack);
}

#endif

void Console::time(const String& title)
{
    InspectorInstrumentation::startConsoleTiming(page(), title);
}

void Console::timeEnd(const String& title, PassRefPtr<ScriptArguments>, PassRefPtr<ScriptCallStack> callStack)
{
    InspectorInstrumentation::stopConsoleTiming(page(), title, callStack);
}

void Console::group(PassRefPtr<ScriptArguments> arguments, PassRefPtr<ScriptCallStack> callStack)
{
    InspectorInstrumentation::addMessageToConsole(page(), JSMessageSource, StartGroupMessageType, LogMessageLevel, String(), arguments, callStack);
}

void Console::groupCollapsed(PassRefPtr<ScriptArguments> arguments, PassRefPtr<ScriptCallStack> callStack)
{
    InspectorInstrumentation::addMessageToConsole(page(), JSMessageSource, StartGroupCollapsedMessageType, LogMessageLevel, String(), arguments, callStack);
}

void Console::groupEnd()
{
    InspectorInstrumentation::addMessageToConsole(page(), JSMessageSource, EndGroupMessageType, LogMessageLevel, String(), 0, String());
}

bool Console::shouldCaptureFullStackTrace() const
{
#if ENABLE(INSPECTOR)
    Page* page = this->page();
    if (!page)
        return false;

    return page->inspectorController()->hasFrontend();
#else
    return false;
#endif
}

void Console::warn(PassRefPtr<ScriptArguments> arguments, PassRefPtr<ScriptCallStack> callStack)
{
    addMessage(LogMessageType, WarningMessageLevel, arguments, callStack);
}

MemoryInfo* Console::memory() const
{
    m_memory = MemoryInfo::create(m_frame);
    return m_memory.get();
}

static bool printExceptions = false;

bool Console::shouldPrintExceptions()
{
    return printExceptions;
}

void Console::setShouldPrintExceptions(bool print)
{
    printExceptions = print;
}

Page* Console::page() const
{
    if (!m_frame)
        return 0;
    return m_frame->page();
}

} // namespace WebCore
