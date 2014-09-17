/*
 * Copyright (c) 2008, Google Inc. All rights reserved.
 * Copyright (C) 2008 Apple Inc. All Rights Reserved.
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
#include "ScriptCachedFrameData.h"

#include "Frame.h"
#include "GCController.h"
#include "Page.h"
#include "PageGroup.h"
#include <runtime/JSLock.h>
#include "ScriptController.h"

using namespace JSC;

namespace WebCore {

ScriptCachedFrameData::ScriptCachedFrameData(Frame* frame)
    : m_domWindow(0)
{
    JSLock lock(SilenceAssertionsOnly);

    ScriptController* scriptController = frame->script();
    ScriptController::ShellMap& windowShells = scriptController->m_windowShells;

    ScriptController::ShellMap::iterator windowShellsEnd = windowShells.end();
    for (ScriptController::ShellMap::iterator iter = windowShells.begin(); iter != windowShellsEnd; ++iter) {
        JSDOMWindow* window = iter->second->window();
        m_windows.add(iter->first.get(), Strong<JSDOMWindow>(window->globalData(), window));
        m_domWindow = window->impl();
    }

    scriptController->attachDebugger(0);
}

DOMWindow* ScriptCachedFrameData::domWindow() const
{
    return m_domWindow;
}

ScriptCachedFrameData::~ScriptCachedFrameData()
{
    clear();
}

void ScriptCachedFrameData::restore(Frame* frame)
{
    JSLock lock(SilenceAssertionsOnly);

    ScriptController* scriptController = frame->script();
    ScriptController::ShellMap& windowShells = scriptController->m_windowShells;

    ScriptController::ShellMap::iterator windowShellsEnd = windowShells.end();
    for (ScriptController::ShellMap::iterator iter = windowShells.begin(); iter != windowShellsEnd; ++iter) {
        DOMWrapperWorld* world = iter->first.get();
        JSDOMWindowShell* windowShell = iter->second.get();

        if (JSDOMWindow* window = m_windows.get(world).get())
            windowShell->setWindow(window->globalData(), window);
        else {
            windowShell->setWindow(frame->domWindow());

            if (Page* page = frame->page()) {
                scriptController->attachDebugger(windowShell, page->debugger());
                windowShell->window()->setProfileGroup(page->group().identifier());
            }
        }
    }
}

void ScriptCachedFrameData::clear()
{
    if (m_windows.isEmpty())
        return;

    JSLock lock(SilenceAssertionsOnly);
    m_windows.clear();
    gcController().garbageCollectSoon();
}

} // namespace WebCore
