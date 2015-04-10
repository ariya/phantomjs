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
#include "WKBundleHitTestResult.h"

#include "InjectedBundleHitTestResult.h"
#include "InjectedBundleNodeHandle.h"
#include "WKAPICast.h"
#include "WKBundleAPICast.h"

using namespace WebKit;

WKTypeID WKBundleHitTestResultGetTypeID()
{
    return toAPI(InjectedBundleHitTestResult::APIType);
}

WKBundleNodeHandleRef WKBundleHitTestResultCopyNodeHandle(WKBundleHitTestResultRef hitTestResultRef)
{
    RefPtr<InjectedBundleNodeHandle> nodeHandle = toImpl(hitTestResultRef)->nodeHandle();
    return toAPI(nodeHandle.release().leakRef());
}

WKBundleFrameRef WKBundleHitTestResultGetFrame(WKBundleHitTestResultRef hitTestResultRef)
{
    return toAPI(toImpl(hitTestResultRef)->frame());
}

WKBundleFrameRef WKBundleHitTestResultGetTargetFrame(WKBundleHitTestResultRef hitTestResultRef)
{
    return toAPI(toImpl(hitTestResultRef)->targetFrame());
}

WKURLRef WKBundleHitTestResultCopyAbsoluteImageURL(WKBundleHitTestResultRef hitTestResultRef)
{
    return toCopiedURLAPI(toImpl(hitTestResultRef)->absoluteImageURL());
}

WKURLRef WKBundleHitTestResultCopyAbsolutePDFURL(WKBundleHitTestResultRef hitTestResultRef)
{
    return toCopiedURLAPI(toImpl(hitTestResultRef)->absolutePDFURL());
}

WKURLRef WKBundleHitTestResultCopyAbsoluteLinkURL(WKBundleHitTestResultRef hitTestResultRef)
{
    return toCopiedURLAPI(toImpl(hitTestResultRef)->absoluteLinkURL());
}

WKURLRef WKBundleHitTestResultCopyAbsoluteMediaURL(WKBundleHitTestResultRef hitTestResultRef)
{
    return toCopiedURLAPI(toImpl(hitTestResultRef)->absoluteMediaURL());
}

bool WKBundleHitTestResultMediaIsInFullscreen(WKBundleHitTestResultRef hitTestResultRef)
{
    return toImpl(hitTestResultRef)->mediaIsInFullscreen();
}

bool WKBundleHitTestResultMediaHasAudio(WKBundleHitTestResultRef hitTestResultRef)
{
    return toImpl(hitTestResultRef)->mediaHasAudio();
}

WKBundleHitTestResultMediaType WKBundleHitTestResultGetMediaType(WKBundleHitTestResultRef hitTestResultRef)
{
    return toAPI(toImpl(hitTestResultRef)->mediaType());
}

WKRect WKBundleHitTestResultGetImageRect(WKBundleHitTestResultRef hitTestResultRef)
{
    return toAPI(toImpl(hitTestResultRef)->imageRect());
}

bool WKBundleHitTestResultGetIsSelected(WKBundleHitTestResultRef hitTestResultRef)
{
    return toImpl(hitTestResultRef)->isSelected();
}

WKStringRef WKBundleHitTestResultCopyLinkLabel(WKBundleHitTestResultRef hitTestResultRef)
{
    return toCopiedAPI(toImpl(hitTestResultRef)->linkLabel());
}

WKStringRef WKBundleHitTestResultCopyLinkTitle(WKBundleHitTestResultRef hitTestResultRef)
{
    return toCopiedAPI(toImpl(hitTestResultRef)->linkTitle());
}
