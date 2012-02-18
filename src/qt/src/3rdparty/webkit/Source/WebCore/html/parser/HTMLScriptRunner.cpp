/*
 * Copyright (C) 2010 Google, Inc. All Rights Reserved.
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
#include "HTMLScriptRunner.h"

#include "Attribute.h"
#include "CachedScript.h"
#include "CachedResourceLoader.h"
#include "Element.h"
#include "Event.h"
#include "Frame.h"
#include "HTMLInputStream.h"
#include "HTMLNames.h"
#include "HTMLScriptRunnerHost.h"
#include "IgnoreDestructiveWriteCountIncrementer.h"
#include "NestingLevelIncrementer.h"
#include "NotImplemented.h"
#include "ScriptElement.h"
#include "ScriptSourceCode.h"

namespace WebCore {

using namespace HTMLNames;

HTMLScriptRunner::HTMLScriptRunner(Document* document, HTMLScriptRunnerHost* host)
    : m_document(document)
    , m_host(host)
    , m_scriptNestingLevel(0)
    , m_hasScriptsWaitingForStylesheets(false)
{
    ASSERT(m_host);
}

HTMLScriptRunner::~HTMLScriptRunner()
{
    // FIXME: Should we be passed a "done loading/parsing" callback sooner than destruction?
    if (m_parsingBlockingScript.cachedScript() && m_parsingBlockingScript.watchingForLoad())
        stopWatchingForLoad(m_parsingBlockingScript);

    while (!m_scriptsToExecuteAfterParsing.isEmpty()) {
        PendingScript pendingScript = m_scriptsToExecuteAfterParsing.takeFirst();
        if (pendingScript.cachedScript() && pendingScript.watchingForLoad())
            stopWatchingForLoad(pendingScript);
    }
}

void HTMLScriptRunner::detach()
{
    m_document = 0;
}

static KURL documentURLForScriptExecution(Document* document)
{
    if (!document || !document->frame())
        return KURL();

    // Use the URL of the currently active document for this frame.
    return document->frame()->document()->url();
}

inline PassRefPtr<Event> createScriptLoadEvent()
{
    return Event::create(eventNames().loadEvent, false, false);
}

ScriptSourceCode HTMLScriptRunner::sourceFromPendingScript(const PendingScript& script, bool& errorOccurred) const
{
    if (script.cachedScript()) {
        errorOccurred = script.cachedScript()->errorOccurred();
        ASSERT(script.cachedScript()->isLoaded());
        return ScriptSourceCode(script.cachedScript());
    }
    errorOccurred = false;
    return ScriptSourceCode(script.element()->textContent(), documentURLForScriptExecution(m_document), script.startingPosition());
}

bool HTMLScriptRunner::isPendingScriptReady(const PendingScript& script)
{
    m_hasScriptsWaitingForStylesheets = !m_document->haveStylesheetsLoaded();
    if (m_hasScriptsWaitingForStylesheets)
        return false;
    if (script.cachedScript() && !script.cachedScript()->isLoaded())
        return false;
    return true;
}

void HTMLScriptRunner::executeParsingBlockingScript()
{
    ASSERT(m_document);
    ASSERT(!m_scriptNestingLevel);
    ASSERT(m_document->haveStylesheetsLoaded());
    ASSERT(isPendingScriptReady(m_parsingBlockingScript));

    InsertionPointRecord insertionPointRecord(m_host->inputStream());
    executePendingScriptAndDispatchEvent(m_parsingBlockingScript);
}

void HTMLScriptRunner::executePendingScriptAndDispatchEvent(PendingScript& pendingScript)
{
    bool errorOccurred = false;
    ScriptSourceCode sourceCode = sourceFromPendingScript(pendingScript, errorOccurred);

    // Stop watching loads before executeScript to prevent recursion if the script reloads itself.
    if (pendingScript.cachedScript() && pendingScript.watchingForLoad())
        stopWatchingForLoad(pendingScript);

    // Clear the pending script before possible rentrancy from executeScript()
    RefPtr<Element> element = pendingScript.releaseElementAndClear();
    if (ScriptElement* scriptElement = toScriptElement(element.get())) {
        NestingLevelIncrementer nestingLevelIncrementer(m_scriptNestingLevel);
        IgnoreDestructiveWriteCountIncrementer ignoreDestructiveWriteCountIncrementer(m_document);
        if (errorOccurred)
            scriptElement->dispatchErrorEvent();
        else {
            ASSERT(isExecutingScript());
            scriptElement->executeScript(sourceCode);
            element->dispatchEvent(createScriptLoadEvent());
        }
    }
    ASSERT(!m_scriptNestingLevel);
}

void HTMLScriptRunner::watchForLoad(PendingScript& pendingScript)
{
    ASSERT(!pendingScript.watchingForLoad());
    m_host->watchForLoad(pendingScript.cachedScript());
    pendingScript.setWatchingForLoad(true);
}

void HTMLScriptRunner::stopWatchingForLoad(PendingScript& pendingScript)
{
    ASSERT(pendingScript.watchingForLoad());
    m_host->stopWatchingForLoad(pendingScript.cachedScript());
    pendingScript.setWatchingForLoad(false);
}

// This function should match 10.2.5.11 "An end tag whose tag name is 'script'"
// Script handling lives outside the tree builder to keep the each class simple.
bool HTMLScriptRunner::execute(PassRefPtr<Element> scriptElement, const TextPosition1& scriptStartPosition)
{
    ASSERT(scriptElement);
    // FIXME: If scripting is disabled, always just return true;

    bool hadPreloadScanner = m_host->hasPreloadScanner();

    // Try to execute the script given to us.
    runScript(scriptElement.get(), scriptStartPosition);

    if (haveParsingBlockingScript()) {
        if (m_scriptNestingLevel)
            return false; // Block the parser.  Unwind to the outermost HTMLScriptRunner::execute before continuing parsing.
        // If preload scanner got created, it is missing the source after the current insertion point. Append it and scan.
        if (!hadPreloadScanner && m_host->hasPreloadScanner())
            m_host->appendCurrentInputStreamToPreloadScannerAndScan();
        if (!executeParsingBlockingScripts())
            return false; // We still have a parsing blocking script, block the parser.
    }
    return true; // Scripts executed as expected, continue parsing.
}

bool HTMLScriptRunner::haveParsingBlockingScript() const
{
    return !!m_parsingBlockingScript.element();
}

bool HTMLScriptRunner::executeParsingBlockingScripts()
{
    while (haveParsingBlockingScript()) {
        // We only really need to check once.
        if (!isPendingScriptReady(m_parsingBlockingScript))
            return false;
        executeParsingBlockingScript();
    }
    return true;
}

bool HTMLScriptRunner::executeScriptsWaitingForLoad(CachedResource* cachedScript)
{
    ASSERT(!m_scriptNestingLevel);
    ASSERT(haveParsingBlockingScript());
    ASSERT_UNUSED(cachedScript, m_parsingBlockingScript.cachedScript() == cachedScript);
    ASSERT(m_parsingBlockingScript.cachedScript()->isLoaded());
    return executeParsingBlockingScripts();
}

bool HTMLScriptRunner::executeScriptsWaitingForStylesheets()
{
    ASSERT(m_document);
    // Callers should check hasScriptsWaitingForStylesheets() before calling
    // to prevent parser or script re-entry during </style> parsing.
    ASSERT(hasScriptsWaitingForStylesheets());
    ASSERT(!m_scriptNestingLevel);
    ASSERT(m_document->haveStylesheetsLoaded());
    return executeParsingBlockingScripts();
}

bool HTMLScriptRunner::executeScriptsWaitingForParsing()
{
    while (!m_scriptsToExecuteAfterParsing.isEmpty()) {
        ASSERT(!m_scriptNestingLevel);
        ASSERT(!haveParsingBlockingScript());
        ASSERT(m_scriptsToExecuteAfterParsing.first().cachedScript());
        if (!m_scriptsToExecuteAfterParsing.first().cachedScript()->isLoaded()) {
            watchForLoad(m_scriptsToExecuteAfterParsing.first());
            return false;
        }
        PendingScript first = m_scriptsToExecuteAfterParsing.takeFirst();
        executePendingScriptAndDispatchEvent(first);
        if (!m_document)
            return false;
    }
    return true;
}

void HTMLScriptRunner::requestParsingBlockingScript(Element* element)
{
    if (!requestPendingScript(m_parsingBlockingScript, element))
        return;

    ASSERT(m_parsingBlockingScript.cachedScript());

    // We only care about a load callback if cachedScript is not already
    // in the cache.  Callers will attempt to run the m_parsingBlockingScript
    // if possible before returning control to the parser.
    if (!m_parsingBlockingScript.cachedScript()->isLoaded())
        watchForLoad(m_parsingBlockingScript);
}

void HTMLScriptRunner::requestDeferredScript(Element* element)
{
    PendingScript pendingScript;
    if (!requestPendingScript(pendingScript, element))
        return;

    ASSERT(pendingScript.cachedScript());
    m_scriptsToExecuteAfterParsing.append(pendingScript);
}

bool HTMLScriptRunner::requestPendingScript(PendingScript& pendingScript, Element* script) const
{
    ASSERT(!pendingScript.element());
    pendingScript.setElement(script);
    // This should correctly return 0 for empty or invalid srcValues.
    CachedScript* cachedScript = toScriptElement(script)->cachedScript().get();
    if (!cachedScript) {
        notImplemented(); // Dispatch error event.
        return false;
    }
    pendingScript.setCachedScript(cachedScript);
    return true;
}

// This method is meant to match the HTML5 definition of "running a script"
// http://www.whatwg.org/specs/web-apps/current-work/multipage/scripting-1.html#running-a-script
void HTMLScriptRunner::runScript(Element* script, const TextPosition1& scriptStartPosition)
{
    ASSERT(m_document);
    ASSERT(!haveParsingBlockingScript());
    {
        InsertionPointRecord insertionPointRecord(m_host->inputStream());
        NestingLevelIncrementer nestingLevelIncrementer(m_scriptNestingLevel);

        ScriptElement* scriptElement = toScriptElement(script);

        // This contains both and ASSERTION and a null check since we should not
        // be getting into the case of a null script element, but seem to be from
        // time to time. The assertion is left in to help find those cases and
        // is being tracked by <https://bugs.webkit.org/show_bug.cgi?id=60559>.
        ASSERT(scriptElement);
        if (!scriptElement)
            return;

        scriptElement->prepareScript(scriptStartPosition);

        if (!scriptElement->willBeParserExecuted())
            return;

        if (scriptElement->willExecuteWhenDocumentFinishedParsing())
            requestDeferredScript(script);
        else if (scriptElement->readyToBeParserExecuted()) {
            if (m_scriptNestingLevel == 1) {
                m_parsingBlockingScript.setElement(script);
                m_parsingBlockingScript.setStartingPosition(scriptStartPosition);
            } else {
                ScriptSourceCode sourceCode(script->textContent(), documentURLForScriptExecution(m_document), scriptStartPosition);
                scriptElement->executeScript(sourceCode);
            }
        } else
            requestParsingBlockingScript(script);
    }
}

}
