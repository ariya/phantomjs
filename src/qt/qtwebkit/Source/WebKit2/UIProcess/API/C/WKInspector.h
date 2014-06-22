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

#ifndef WKInspector_h
#define WKInspector_h

#include <WebKit2/WKBase.h>

#ifndef __cplusplus
#include <stdbool.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

WK_EXPORT WKTypeID WKInspectorGetTypeID();

WK_EXPORT WKPageRef WKInspectorGetPage(WKInspectorRef inspector);

WK_EXPORT bool WKInspectorIsConnected(WKInspectorRef inspector);
WK_EXPORT bool WKInspectorIsVisible(WKInspectorRef inspector);
WK_EXPORT bool WKInspectorIsFront(WKInspectorRef inspector);

WK_EXPORT void WKInspectorConnect(WKInspectorRef inspector);

WK_EXPORT void WKInspectorShow(WKInspectorRef inspector);
WK_EXPORT void WKInspectorHide(WKInspectorRef inspector);
WK_EXPORT void WKInspectorClose(WKInspectorRef inspector);

WK_EXPORT void WKInspectorShowConsole(WKInspectorRef inspector);
WK_EXPORT void WKInspectorShowResources(WKInspectorRef inspector);
WK_EXPORT void WKInspectorShowMainResourceForFrame(WKInspectorRef inspector, WKFrameRef frame);

WK_EXPORT bool WKInspectorIsAttached(WKInspectorRef inspector);
WK_EXPORT void WKInspectorAttach(WKInspectorRef inspector);
WK_EXPORT void WKInspectorDetach(WKInspectorRef inspector);

WK_EXPORT bool WKInspectorIsDebuggingJavaScript(WKInspectorRef inspector);
WK_EXPORT void WKInspectorToggleJavaScriptDebugging(WKInspectorRef inspector);

WK_EXPORT bool WKInspectorIsProfilingJavaScript(WKInspectorRef inspector);
WK_EXPORT void WKInspectorToggleJavaScriptProfiling(WKInspectorRef inspector);

WK_EXPORT bool WKInspectorIsProfilingPage(WKInspectorRef inspector);
WK_EXPORT void WKInspectorTogglePageProfiling(WKInspectorRef inspector);

#ifdef __cplusplus
}
#endif

#endif // WKInspector_h
