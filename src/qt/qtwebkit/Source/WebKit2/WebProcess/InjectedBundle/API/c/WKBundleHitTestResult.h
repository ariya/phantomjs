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

#ifndef WKBundleHitTestResult_h
#define WKBundleHitTestResult_h

#include <WebKit2/WKBase.h>
#include <WebKit2/WKGeometry.h>

#ifdef __cplusplus
extern "C" {
#endif

enum {
    kWKBundleHitTestResultMediaTypeNone,
    kWKBundleHitTestResultMediaTypeAudio,
    kWKBundleHitTestResultMediaTypeVideo
};
typedef uint32_t WKBundleHitTestResultMediaType;

WK_EXPORT WKTypeID WKBundleHitTestResultGetTypeID();

WK_EXPORT WKBundleNodeHandleRef WKBundleHitTestResultCopyNodeHandle(WKBundleHitTestResultRef hitTestResult);

WK_EXPORT WKBundleFrameRef WKBundleHitTestResultGetFrame(WKBundleHitTestResultRef hitTestResult);
WK_EXPORT WKBundleFrameRef WKBundleHitTestResultGetTargetFrame(WKBundleHitTestResultRef hitTestResult);

WK_EXPORT WKURLRef WKBundleHitTestResultCopyAbsoluteImageURL(WKBundleHitTestResultRef hitTestResult);
WK_EXPORT WKURLRef WKBundleHitTestResultCopyAbsolutePDFURL(WKBundleHitTestResultRef hitTestResult);
WK_EXPORT WKURLRef WKBundleHitTestResultCopyAbsoluteLinkURL(WKBundleHitTestResultRef hitTestResult);
WK_EXPORT WKURLRef WKBundleHitTestResultCopyAbsoluteMediaURL(WKBundleHitTestResultRef hitTestResult);
WK_EXPORT bool WKBundleHitTestResultMediaIsInFullscreen(WKBundleHitTestResultRef hitTestResult);
WK_EXPORT bool WKBundleHitTestResultMediaHasAudio(WKBundleHitTestResultRef hitTestResult);
WK_EXPORT WKBundleHitTestResultMediaType WKBundleHitTestResultGetMediaType(WKBundleHitTestResultRef hitTestResult);

WK_EXPORT WKRect WKBundleHitTestResultGetImageRect(WKBundleHitTestResultRef hitTestResult);
WK_EXPORT bool WKBundleHitTestResultGetIsSelected(WKBundleHitTestResultRef hitTestResult);

WK_EXPORT WKStringRef WKBundleHitTestResultCopyLinkLabel(WKBundleHitTestResultRef hitTestResult);
WK_EXPORT WKStringRef WKBundleHitTestResultCopyLinkTitle(WKBundleHitTestResultRef hitTestResult);

#ifdef __cplusplus
}
#endif

#endif /* WKBundleHitTestResult_h */
