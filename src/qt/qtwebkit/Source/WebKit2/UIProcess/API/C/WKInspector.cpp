/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "WKInspector.h"

#include "WKAPICast.h"
#include "WebInspectorProxy.h"

using namespace WebKit;

WKTypeID WKInspectorGetTypeID()
{
#if ENABLE(INSPECTOR)
    return toAPI(WebInspectorProxy::APIType);
#else
    return 0;
#endif
}

WKPageRef WKInspectorGetPage(WKInspectorRef inspectorRef)
{
#if ENABLE(INSPECTOR)
    return toAPI(toImpl(inspectorRef)->page());
#else
    UNUSED_PARAM(inspectorRef);
    return 0;
#endif
}

bool WKInspectorIsConnected(WKInspectorRef inspectorRef)
{
#if ENABLE(INSPECTOR)
    return toImpl(inspectorRef)->isConnected();
#else
    UNUSED_PARAM(inspectorRef);
    return false;
#endif
}

bool WKInspectorIsVisible(WKInspectorRef inspectorRef)
{
#if ENABLE(INSPECTOR)
    return toImpl(inspectorRef)->isVisible();
#else
    UNUSED_PARAM(inspectorRef);
    return false;
#endif
}

bool WKInspectorIsFront(WKInspectorRef inspectorRef)
{
#if ENABLE(INSPECTOR)
    return toImpl(inspectorRef)->isFront();
#else
    UNUSED_PARAM(inspectorRef);
    return false;
#endif
}

void WKInspectorConnect(WKInspectorRef inspectorRef)
{
#if ENABLE(INSPECTOR)
    toImpl(inspectorRef)->connect();
#else
    UNUSED_PARAM(inspectorRef);
#endif
}

void WKInspectorShow(WKInspectorRef inspectorRef)
{
#if ENABLE(INSPECTOR)
    toImpl(inspectorRef)->show();
#else
    UNUSED_PARAM(inspectorRef);
#endif
}

void WKInspectorHide(WKInspectorRef inspectorRef)
{
#if ENABLE(INSPECTOR)
    toImpl(inspectorRef)->hide();
#else
    UNUSED_PARAM(inspectorRef);
#endif
}

void WKInspectorClose(WKInspectorRef inspectorRef)
{
#if ENABLE(INSPECTOR)
    toImpl(inspectorRef)->close();
#else
    UNUSED_PARAM(inspectorRef);
#endif
}

void WKInspectorShowConsole(WKInspectorRef inspectorRef)
{
#if ENABLE(INSPECTOR)
    toImpl(inspectorRef)->showConsole();
#else
    UNUSED_PARAM(inspectorRef);
#endif
}

void WKInspectorShowResources(WKInspectorRef inspectorRef)
{
#if ENABLE(INSPECTOR)
    toImpl(inspectorRef)->showResources();
#else
    UNUSED_PARAM(inspectorRef);
#endif
}

void WKInspectorShowMainResourceForFrame(WKInspectorRef inspectorRef, WKFrameRef frameRef)
{
#if ENABLE(INSPECTOR)
    toImpl(inspectorRef)->showMainResourceForFrame(toImpl(frameRef));
#else
    UNUSED_PARAM(inspectorRef);
    UNUSED_PARAM(frameRef);
#endif
}

bool WKInspectorIsAttached(WKInspectorRef inspectorRef)
{
#if ENABLE(INSPECTOR)
    return toImpl(inspectorRef)->isAttached();
#else
    UNUSED_PARAM(inspectorRef);
    return false;
#endif
}

void WKInspectorAttach(WKInspectorRef inspectorRef)
{
#if ENABLE(INSPECTOR)
    toImpl(inspectorRef)->attach();
#else
    UNUSED_PARAM(inspectorRef);
#endif
}

void WKInspectorDetach(WKInspectorRef inspectorRef)
{
#if ENABLE(INSPECTOR)
    toImpl(inspectorRef)->detach();
#else
    UNUSED_PARAM(inspectorRef);
#endif
}

bool WKInspectorIsDebuggingJavaScript(WKInspectorRef inspectorRef)
{
#if ENABLE(INSPECTOR)
    return toImpl(inspectorRef)->isDebuggingJavaScript();
#else
    UNUSED_PARAM(inspectorRef);
    return false;
#endif
}

void WKInspectorToggleJavaScriptDebugging(WKInspectorRef inspectorRef)
{
#if ENABLE(INSPECTOR)
    toImpl(inspectorRef)->toggleJavaScriptDebugging();
#else
    UNUSED_PARAM(inspectorRef);
#endif
}

bool WKInspectorIsProfilingJavaScript(WKInspectorRef inspectorRef)
{
#if ENABLE(INSPECTOR)
    return toImpl(inspectorRef)->isProfilingJavaScript();
#else
    UNUSED_PARAM(inspectorRef);
    return false;
#endif
}

void WKInspectorToggleJavaScriptProfiling(WKInspectorRef inspectorRef)
{
#if ENABLE(INSPECTOR)
    toImpl(inspectorRef)->toggleJavaScriptProfiling();
#else
    UNUSED_PARAM(inspectorRef);
#endif
}

bool WKInspectorIsProfilingPage(WKInspectorRef inspectorRef)
{
#if ENABLE(INSPECTOR)
    return toImpl(inspectorRef)->isProfilingPage();
#else
    UNUSED_PARAM(inspectorRef);
    return false;
#endif
}

void WKInspectorTogglePageProfiling(WKInspectorRef inspectorRef)
{
#if ENABLE(INSPECTOR)
    toImpl(inspectorRef)->togglePageProfiling();
#else
    UNUSED_PARAM(inspectorRef);
#endif
}
