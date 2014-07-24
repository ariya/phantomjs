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

#ifndef WKBundleFrame_h
#define WKBundleFrame_h

#include <JavaScriptCore/JavaScript.h>
#include <WebKit2/WKBase.h>
#include <WebKit2/WKFrame.h>
#include <WebKit2/WKGeometry.h>

#ifdef __cplusplus
extern "C" {
#endif

WK_EXPORT WKTypeID WKBundleFrameGetTypeID();

WK_EXPORT bool WKBundleFrameIsMainFrame(WKBundleFrameRef frame);
WK_EXPORT WKBundleFrameRef WKBundleFrameGetParentFrame(WKBundleFrameRef frame);
WK_EXPORT WKArrayRef WKBundleFrameCopyChildFrames(WKBundleFrameRef frame);

WK_EXPORT WKStringRef WKBundleFrameCopyName(WKBundleFrameRef frame);
WK_EXPORT WKURLRef WKBundleFrameCopyURL(WKBundleFrameRef frame);
WK_EXPORT WKURLRef WKBundleFrameCopyProvisionalURL(WKBundleFrameRef frame);
WK_EXPORT WKSecurityOriginRef WKBundleFrameCopySecurityOrigin(WKBundleFrameRef frame);

WK_EXPORT WKFrameLoadState WKBundleFrameGetFrameLoadState(WKBundleFrameRef frame); 

WK_EXPORT JSGlobalContextRef WKBundleFrameGetJavaScriptContext(WKBundleFrameRef frame);
WK_EXPORT JSGlobalContextRef WKBundleFrameGetJavaScriptContextForWorld(WKBundleFrameRef frame, WKBundleScriptWorldRef world);

WK_EXPORT WKBundleFrameRef WKBundleFrameForJavaScriptContext(JSContextRef context);

WK_EXPORT JSValueRef WKBundleFrameGetJavaScriptWrapperForNodeForWorld(WKBundleFrameRef frame, WKBundleNodeHandleRef nodeHandle, WKBundleScriptWorldRef world);
WK_EXPORT JSValueRef WKBundleFrameGetJavaScriptWrapperForRangeForWorld(WKBundleFrameRef frame, WKBundleRangeHandleRef rangeHandle, WKBundleScriptWorldRef world);

WK_EXPORT WKBundlePageRef WKBundleFrameGetPage(WKBundleFrameRef frame);

WK_EXPORT bool WKBundleFrameAllowsFollowingLink(WKBundleFrameRef frame, WKURLRef url);

WK_EXPORT WKRect WKBundleFrameGetContentBounds(WKBundleFrameRef frame);
WK_EXPORT WKRect WKBundleFrameGetVisibleContentBounds(WKBundleFrameRef frame);
WK_EXPORT WKRect WKBundleFrameGetVisibleContentBoundsExcludingScrollbars(WKBundleFrameRef frame);
WK_EXPORT WKSize WKBundleFrameGetScrollOffset(WKBundleFrameRef frame);

WK_EXPORT bool WKBundleFrameHasHorizontalScrollbar(WKBundleFrameRef frame);
WK_EXPORT bool WKBundleFrameHasVerticalScrollbar(WKBundleFrameRef frame);

WK_EXPORT bool WKBundleFrameGetDocumentBackgroundColor(WKBundleFrameRef frame, double* red, double* green, double* blue, double* alpha);

WK_EXPORT WKStringRef WKBundleFrameCopySuggestedFilenameForResourceWithURL(WKBundleFrameRef frame, WKURLRef url);
WK_EXPORT WKStringRef WKBundleFrameCopyMIMETypeForResourceWithURL(WKBundleFrameRef frame, WKURLRef url);

WK_EXPORT WKDataRef WKBundleFrameCopyWebArchive(WKBundleFrameRef frame);

typedef bool (*WKBundleFrameFrameFilterCallback)(WKBundleFrameRef frame, WKBundleFrameRef subframe, void* context);
WK_EXPORT WKDataRef WKBundleFrameCopyWebArchiveFilteringSubframes(WKBundleFrameRef frame, WKBundleFrameFrameFilterCallback frameFilterCallback, void* context);

#ifdef __cplusplus
}
#endif

#endif /* WKBundleFrame_h */
