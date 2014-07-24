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
#include "WKFrame.h"

#include "WKAPICast.h"
#include "WebFrameProxy.h"

#ifdef __BLOCKS__
#include <Block.h>
#endif

using namespace WebKit;

WKTypeID WKFrameGetTypeID()
{
    return toAPI(WebFrameProxy::APIType);
}

bool WKFrameIsMainFrame(WKFrameRef frameRef)
{
    return toImpl(frameRef)->isMainFrame();
}

WKFrameLoadState WKFrameGetFrameLoadState(WKFrameRef frameRef)
{
    WebFrameProxy* frame = toImpl(frameRef);
    switch (frame->loadState()) {
        case WebFrameProxy::LoadStateProvisional:
            return kWKFrameLoadStateProvisional;
        case WebFrameProxy::LoadStateCommitted:
            return kWKFrameLoadStateCommitted;
        case WebFrameProxy::LoadStateFinished:
            return kWKFrameLoadStateFinished;
    }
    
    ASSERT_NOT_REACHED();
    return kWKFrameLoadStateFinished;
}

WKURLRef WKFrameCopyProvisionalURL(WKFrameRef frameRef)
{
    return toCopiedURLAPI(toImpl(frameRef)->provisionalURL());
}

WKURLRef WKFrameCopyURL(WKFrameRef frameRef)
{
    return toCopiedURLAPI(toImpl(frameRef)->url());
}

WKURLRef WKFrameCopyUnreachableURL(WKFrameRef frameRef)
{
    return toCopiedURLAPI(toImpl(frameRef)->unreachableURL());
}

void WKFrameStopLoading(WKFrameRef frameRef)
{
    toImpl(frameRef)->stopLoading();
}

WKStringRef WKFrameCopyMIMEType(WKFrameRef frameRef)
{
    return toCopiedAPI(toImpl(frameRef)->mimeType());
}

WKStringRef WKFrameCopyTitle(WKFrameRef frameRef)
{
    return toCopiedAPI(toImpl(frameRef)->title());
}

WKPageRef WKFrameGetPage(WKFrameRef frameRef)
{
    return toAPI(toImpl(frameRef)->page());
}

WKCertificateInfoRef WKFrameGetCertificateInfo(WKFrameRef frameRef)
{
    return toAPI(toImpl(frameRef)->certificateInfo());
}

bool WKFrameCanProvideSource(WKFrameRef frameRef)
{
    return toImpl(frameRef)->canProvideSource();
}

bool WKFrameCanShowMIMEType(WKFrameRef frameRef, WKStringRef mimeTypeRef)
{
    return toImpl(frameRef)->canShowMIMEType(toWTFString(mimeTypeRef));
}

bool WKFrameIsDisplayingStandaloneImageDocument(WKFrameRef frameRef)
{
    return toImpl(frameRef)->isDisplayingStandaloneImageDocument();
}

bool WKFrameIsDisplayingMarkupDocument(WKFrameRef frameRef)
{
    return toImpl(frameRef)->isDisplayingMarkupDocument();
}

bool WKFrameIsFrameSet(WKFrameRef frameRef)
{
    return toImpl(frameRef)->isFrameSet();
}

void WKFrameGetMainResourceData(WKFrameRef frameRef, WKFrameGetResourceDataFunction callback, void* context)
{
    toImpl(frameRef)->getMainResourceData(DataCallback::create(context, callback));
}

void WKFrameGetResourceData(WKFrameRef frameRef, WKURLRef resourceURL, WKFrameGetResourceDataFunction callback, void* context)
{
    toImpl(frameRef)->getResourceData(toImpl(resourceURL), DataCallback::create(context, callback));
}

#ifdef __BLOCKS__
static void callGetResourceDataBlockAndDispose(WKDataRef data, WKErrorRef error, void* context)
{
    WKFrameGetResourceDataBlock block = (WKFrameGetResourceDataBlock)context;
    block(data, error);
    Block_release(block);
}

void WKFrameGetMainResourceData_b(WKFrameRef frameRef, WKFrameGetResourceDataBlock block)
{
    WKFrameGetMainResourceData(frameRef, callGetResourceDataBlockAndDispose, Block_copy(block));
}

void WKFrameGetResourceData_b(WKFrameRef frameRef, WKURLRef resourceURL, WKFrameGetResourceDataBlock block)
{
    WKFrameGetResourceData(frameRef, resourceURL, callGetResourceDataBlockAndDispose, Block_copy(block));
}
#endif

void WKFrameGetWebArchive(WKFrameRef frameRef, WKFrameGetWebArchiveFunction callback, void* context)
{
    toImpl(frameRef)->getWebArchive(DataCallback::create(context, callback));
}

#ifdef __BLOCKS__
static void callGetWebArchiveBlockAndDispose(WKDataRef archiveData, WKErrorRef error, void* context)
{
    WKFrameGetWebArchiveBlock block = (WKFrameGetWebArchiveBlock)context;
    block(archiveData, error);
    Block_release(block);
}

void WKFrameGetWebArchive_b(WKFrameRef frameRef, WKFrameGetWebArchiveBlock block)
{
    WKFrameGetWebArchive(frameRef, callGetWebArchiveBlockAndDispose, Block_copy(block));
}
#endif


// NOTE: These are deprecated and should be removed. They currently do nothing.

WKArrayRef WKFrameCopyChildFrames(WKFrameRef)
{
    return 0;
}

WKFrameRef WKFrameGetParentFrame(WKFrameRef)
{
    return 0;
}
