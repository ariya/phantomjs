/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
 * Copyright (C) 2010 Google Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"

#if ENABLE(JAVASCRIPT_DEBUGGER)

#include "ScriptProfiler.h"

#include "Frame.h"
#include "GCController.h"
#include "JSDOMBinding.h"
#include "JSDOMWindow.h"
#include "Page.h"
#include "ScriptObject.h"
#include "ScriptState.h"
#include <profiler/LegacyProfiler.h>
#include <wtf/Forward.h>

namespace WebCore {

void ScriptProfiler::collectGarbage()
{
    gcController().garbageCollectSoon();
}

ScriptObject ScriptProfiler::objectByHeapObjectId(unsigned)
{
    return ScriptObject();
}

unsigned ScriptProfiler::getHeapObjectId(const ScriptValue&)
{
    return 0;
}

void ScriptProfiler::start(ScriptState* state, const String& title)
{
    JSC::LegacyProfiler::profiler()->startProfiling(state, title);
}

void ScriptProfiler::startForPage(Page* inspectedPage, const String& title)
{
    JSC::ExecState* scriptState = toJSDOMWindow(inspectedPage->mainFrame(), debuggerWorld())->globalExec();
    start(scriptState, title);
}

#if ENABLE(WORKERS)
void ScriptProfiler::startForWorkerGlobalScope(WorkerGlobalScope* context, const String& title)
{
    start(scriptStateFromWorkerGlobalScope(context), title);
}
#endif

PassRefPtr<ScriptProfile> ScriptProfiler::stop(ScriptState* state, const String& title)
{
    RefPtr<JSC::Profile> profile = JSC::LegacyProfiler::profiler()->stopProfiling(state, title);
    return ScriptProfile::create(profile);
}

PassRefPtr<ScriptProfile> ScriptProfiler::stopForPage(Page* inspectedPage, const String& title)
{
    JSC::ExecState* scriptState = toJSDOMWindow(inspectedPage->mainFrame(), debuggerWorld())->globalExec();
    return stop(scriptState, title);
}

#if ENABLE(WORKERS)
PassRefPtr<ScriptProfile> ScriptProfiler::stopForWorkerGlobalScope(WorkerGlobalScope* context, const String& title)
{
    return stop(scriptStateFromWorkerGlobalScope(context), title);
}
#endif

} // namespace WebCore

#endif // ENABLE(JAVASCRIPT_DEBUGGER)
