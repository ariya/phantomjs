/*
 * Copyright (c) 2011 Google Inc. All rights reserved.
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
#include "PageScriptDebugServer.h"

#if ENABLE(JAVASCRIPT_DEBUGGER)

#include "Frame.h"
#include "FrameView.h"
#include "JSDOMWindowCustom.h"
#include "Page.h"
#include "PageGroup.h"
#include "PluginView.h"
#include "ScriptDebugListener.h"
#include "Widget.h"
#include <runtime/JSLock.h>
#include <wtf/MainThread.h>
#include <wtf/OwnPtr.h>
#include <wtf/PassOwnPtr.h>
#include <wtf/StdLibExtras.h>

using namespace JSC;

namespace WebCore {

static Page* toPage(JSGlobalObject* globalObject)
{
    ASSERT_ARG(globalObject, globalObject);

    JSDOMWindow* window = asJSDOMWindow(globalObject);
    Frame* frame = window->impl()->frame();
    return frame ? frame->page() : 0;
}

PageScriptDebugServer& PageScriptDebugServer::shared()
{
    DEFINE_STATIC_LOCAL(PageScriptDebugServer, server, ());
    return server;
}

PageScriptDebugServer::PageScriptDebugServer()
    : ScriptDebugServer()
{
}

PageScriptDebugServer::~PageScriptDebugServer()
{
    deleteAllValues(m_pageListenersMap);
}

void PageScriptDebugServer::addListener(ScriptDebugListener* listener, Page* page)
{
    ASSERT_ARG(listener, listener);
    ASSERT_ARG(page, page);

    pair<PageListenersMap::iterator, bool> result = m_pageListenersMap.add(page, 0);
    if (result.second)
        result.first->second = new ListenerSet;

    ListenerSet* listeners = result.first->second;
    listeners->add(listener);

    recompileAllJSFunctionsSoon();
    page->setDebugger(this);
}

void PageScriptDebugServer::removeListener(ScriptDebugListener* listener, Page* page)
{
    ASSERT_ARG(listener, listener);
    ASSERT_ARG(page, page);

    PageListenersMap::iterator it = m_pageListenersMap.find(page);
    if (it == m_pageListenersMap.end())
        return;

    ListenerSet* listeners = it->second;
    listeners->remove(listener);
    if (listeners->isEmpty()) {
        m_pageListenersMap.remove(it);
        delete listeners;
        didRemoveLastListener(page);
    }
}

void PageScriptDebugServer::recompileAllJSFunctions(Timer<ScriptDebugServer>*)
{
    JSLock lock(SilenceAssertionsOnly);
    // If JavaScript stack is not empty postpone recompilation.
    if (JSDOMWindow::commonJSGlobalData()->dynamicGlobalObject)
        recompileAllJSFunctionsSoon();
    else
        Debugger::recompileAllJSFunctions(JSDOMWindow::commonJSGlobalData());
}

ScriptDebugServer::ListenerSet* PageScriptDebugServer::getListenersForGlobalObject(JSGlobalObject* globalObject)
{
    Page* page = toPage(globalObject);
    if (!page)
        return 0;
    return m_pageListenersMap.get(page);
}

void PageScriptDebugServer::didPause(JSC::JSGlobalObject* globalObject)
{
    Page* page = toPage(globalObject);
    m_pausedPage = page;
    setJavaScriptPaused(page->group(), true);
}

void PageScriptDebugServer::didContinue(JSC::JSGlobalObject* globalObject)
{
    Page* page = toPage(globalObject);
    m_pausedPage = 0;
    setJavaScriptPaused(page->group(), false);
}

void PageScriptDebugServer::didRemoveLastListener(Page* page)
{
    if (m_pausedPage == page)
        m_doneProcessingDebuggerEvents = true;

    recompileAllJSFunctionsSoon();
    page->setDebugger(0);
}

void PageScriptDebugServer::setJavaScriptPaused(const PageGroup& pageGroup, bool paused)
{
    setMainThreadCallbacksPaused(paused);

    const HashSet<Page*>& pages = pageGroup.pages();

    HashSet<Page*>::const_iterator end = pages.end();
    for (HashSet<Page*>::const_iterator it = pages.begin(); it != end; ++it)
        setJavaScriptPaused(*it, paused);
}

void PageScriptDebugServer::setJavaScriptPaused(Page* page, bool paused)
{
    ASSERT_ARG(page, page);

    page->setDefersLoading(paused);

    for (Frame* frame = page->mainFrame(); frame; frame = frame->tree()->traverseNext())
        setJavaScriptPaused(frame, paused);
}

void PageScriptDebugServer::setJavaScriptPaused(Frame* frame, bool paused)
{
    ASSERT_ARG(frame, frame);

    if (!frame->script()->canExecuteScripts(NotAboutToExecuteScript))
        return;

    frame->script()->setPaused(paused);

    Document* document = frame->document();
    if (paused) {
        document->suspendScriptedAnimationControllerCallbacks();
        document->suspendActiveDOMObjects(ActiveDOMObject::JavaScriptDebuggerPaused);
    } else {
        document->resumeActiveDOMObjects();
        document->resumeScriptedAnimationControllerCallbacks();
    }

    setJavaScriptPaused(frame->view(), paused);
}

void PageScriptDebugServer::setJavaScriptPaused(FrameView* view, bool paused)
{
    if (!view)
        return;

    const HashSet<RefPtr<Widget> >* children = view->children();
    ASSERT(children);

    HashSet<RefPtr<Widget> >::const_iterator end = children->end();
    for (HashSet<RefPtr<Widget> >::const_iterator it = children->begin(); it != end; ++it) {
        Widget* widget = (*it).get();
        if (!widget->isPluginView())
            continue;
        static_cast<PluginView*>(widget)->setJavaScriptPaused(paused);
    }
}

} // namespace WebCore

#endif // ENABLE(JAVASCRIPT_DEBUGGER)
