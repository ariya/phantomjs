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
#include "CustomElementRegistry.h"
#include "Element.h"
#include "Event.h"
#include "Frame.h"
#include "HTMLInputStream.h"
#include "HTMLNames.h"
#include "HTMLScriptRunnerHost.h"
#include "IgnoreDestructiveWriteCountIncrementer.h"
#include "MutationObserver.h"
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
    if (m_parserBlockingScript.cachedScript() && m_parserBlockingScript.watchingForLoad())
        stopWatchingForLoad(m_parserBlockingScript);

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
    if (!m_document)
        return false;
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
    ASSERT(!isExecutingScript());
    ASSERT(m_document->haveStylesheetsLoaded());
    ASSERT(isPendingScriptReady(m_parserBlockingScript));

    InsertionPointRecord insertionPointRecord(m_host->inputStream());
    executePendingScriptAndDispatchEvent(m_parserBlockingScript);
}

void HTMLScriptRunner::executePendingScriptAndDispatchEvent(PendingScript& pendingScript)
{
    bool errorOccurred = false;
    ScriptSourceCode sourceCode = sourceFromPendingScript(pendingScript, errorOccurred);

    // Stop watching loads before executeScript to prevent recursion if the script reloads itself.
    if (pendingScript.cachedScript() && pendingScript.watchingForLoad())
        stopWatchingForLoad(pendingScript);

    if (!isExecutingScript()) {
#if ENABLE(CUSTOM_ELEMENTS)
        CustomElementRegistry::deliverAllLifecycleCallbacks();
#endif
        MutationObserver::deliverAllMutations();
    }

    // Clear the pending script before possible rentrancy from executeScript()
    RefPtr<Element> element = pendingScript.releaseElementAndClear();
    if (ScriptElement* scriptElement = toScriptElementIfPossible(element.get())) {
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
    ASSERT(!isExecutingScript());
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
void HTMLScriptRunner::execute(PassRefPtr<Element> scriptElement, const TextPosition& scriptStartPosition)
{
    ASSERT(scriptElement);
    // FIXME: If scripting is disabled, always just return.

    bool hadPreloadScanner = m_host->hasPreloadScanner();

    // Try to execute the script given to us.
    runScript(scriptElement.get(), scriptStartPosition);

    if (hasParserBlockingScript()) {
        if (isExecutingScript())
            return; // Unwind to the outermost HTMLScriptRunner::execute before continuing parsing.
        // If preload scanner got created, it is missing the source after the current insertion point. Append it and scan.
        if (!hadPreloadScanner && m_host->hasPreloadScanner())
            m_host->appendCurrentInputStreamToPreloadScannerAndScan();
        executeParsingBlockingScripts();
    }
}

bool HTMLScriptRunner::hasParserBlockingScript() const
{
    return !!m_parserBlockingScript.element();
}

void HTMLScriptRunner::executeParsingBlockingScripts()
{
    while (hasParserBlockingScript() && isPendingScriptReady(m_parserBlockingScript))
        executeParsingBlockingScript();
}

void HTMLScriptRunner::executeScriptsWaitingForLoad(CachedResource* cachedScript)
{
    ASSERT(!isExecutingScript());
    ASSERT(hasParserBlockingScript());
    ASSERT_UNUSED(cachedScript, m_parserBlockingScript.cachedScript() == cachedScript);
    ASSERT(m_parserBlockingScript.cachedScript()->isLoaded());
    executeParsingBlockingScripts();
}

void HTMLScriptRunner::executeScriptsWaitingForStylesheets()
{
    ASSERT(m_document);
    // Callers should check hasScriptsWaitingForStylesheets() before calling
    // to prevent parser or script re-entry during </style> parsing.
    ASSERT(hasScriptsWaitingForStylesheets());
    ASSERT(!isExecutingScript());
    ASSERT(m_document->haveStylesheetsLoaded());
    executeParsingBlockingScripts();
}

bool HTMLScriptRunner::executeScriptsWaitingForParsing()
{
    while (!m_scriptsToExecuteAfterParsing.isEmpty()) {
        ASSERT(!isExecutingScript());
        ASSERT(!hasParserBlockingScript());
        ASSERT(m_scriptsToExecuteAfterParsing.first().cachedScript());
        if (!m_scriptsToExecuteAfterParsing.first().cachedScript()->isLoaded()) {
            watchForLoad(m_scriptsToExecuteAfterParsing.first());
            return false;
        }
        PendingScript first = m_scriptsToExecuteAfterParsing.takeFirst();
        executePendingScriptAndDispatchEvent(first);
        // FIXME: What is this m_document check for?
        if (!m_document)
            return false;
    }
    return true;
}

void HTMLScriptRunner::requestParsingBlockingScript(Element* element)
{
    if (!requestPendingScript(m_parserBlockingScript, element))
        return;

    ASSERT(m_parserBlockingScript.cachedScript());

    // We only care about a load callback if cachedScript is not already
    // in the cache. Callers will attempt to run the m_parserBlockingScript
    // if possible before returning control to the parser.
    if (!m_parserBlockingScript.cachedScript()->isLoaded())
        watchForLoad(m_parserBlockingScript);
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
    CachedScript* cachedScript = toScriptElementIfPossible(script)->cachedScript().get();
    if (!cachedScript) {
        notImplemented(); // Dispatch error event.
        return false;
    }
    pendingScript.setCachedScript(cachedScript);
    return true;
}

// This method is meant to match the HTML5 definition of "running a script"
// http://www.whatwg.org/specs/web-apps/current-work/multipage/scripting-1.html#running-a-script
void HTMLScriptRunner::runScript(Element* script, const TextPosition& scriptStartPosition)
{
    ASSERT(m_document);
    ASSERT(!hasParserBlockingScript());
    {
        ScriptElement* scriptElement = toScriptElementIfPossible(script);

        // This contains both and ASSERTION and a null check since we should not
        // be getting into the case of a null script element, but seem to be from
        // time to time. The assertion is left in to help find those cases and
        // is being tracked by <https://bugs.webkit.org/show_bug.cgi?id=60559>.
        ASSERT(scriptElement);
        if (!scriptElement)
            return;

        // FIXME: This may be too agressive as we always deliver mutations at
        // every script element, even if it's not ready to execute yet. There's
        // unfortuantely no obvious way to tell if prepareScript is going to
        // execute the script from out here.
        if (!isExecutingScript()) {
#if ENABLE(CUSTOM_ELEMENTS)
            CustomElementRegistry::deliverAllLifecycleCallbacks();
#endif
            MutationObserver::deliverAllMutations();
        }

        InsertionPointRecord insertionPointRecord(m_host->inputStream());
        NestingLevelIncrementer nestingLevelIncrementer(m_scriptNestingLevel);

        scriptElement->prepareScript(scriptStartPosition);

        if (!scriptElement->willBeParserExecuted())
            return;

        if (scriptElement->willExecuteWhenDocumentFinishedParsing())
            requestDeferredScript(script);
        else if (scriptElement->readyToBeParserExecuted()) {
            if (m_scriptNestingLevel == 1) {
                m_parserBlockingScript.setElement(script);
                m_parserBlockingScript.setStartingPosition(scriptStartPosition);
            } else {
                ScriptSourceCode sourceCode(script->textContent(), documentURLForScriptExecution(m_document), scriptStartPosition);
                scriptElement->executeScript(sourceCode);
            }
        } else
            requestParsingBlockingScript(script);
    }
}

}
