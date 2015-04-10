/*
 * Copyright (C) 2012 Apple Inc. All rights reserved.
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

#ifndef WKRenderObject_h
#define WKRenderObject_h

#include <WebKit2/WKBase.h>
#include <WebKit2/WKGeometry.h>

#ifdef __cplusplus
extern "C" {
#endif

WK_EXPORT WKTypeID WKRenderObjectGetTypeID();

WK_EXPORT WKStringRef WKRenderObjectCopyName(WKRenderObjectRef renderObject);

WK_EXPORT WKStringRef WKRenderObjectCopyElementTagName(WKRenderObjectRef renderObject);
WK_EXPORT WKStringRef WKRenderObjectCopyElementID(WKRenderObjectRef renderObject);
WK_EXPORT WKArrayRef WKRenderObjectGetElementClassNames(WKRenderObjectRef renderObject);

WK_EXPORT WKPoint WKRenderObjectGetAbsolutePosition(WKRenderObjectRef renderObject);
WK_EXPORT WKRect WKRenderObjectGetFrameRect(WKRenderObjectRef renderObject);
WK_EXPORT WKArrayRef WKRenderObjectGetChildren(WKRenderObjectRef renderObject);

#ifdef __cplusplus
}
#endif

#endif // WKRenderObject_h
