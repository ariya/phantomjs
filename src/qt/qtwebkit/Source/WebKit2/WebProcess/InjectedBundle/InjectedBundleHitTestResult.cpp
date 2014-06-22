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
#include "InjectedBundleHitTestResult.h"

#include "InjectedBundleNodeHandle.h"
#include "WebFrame.h"
#include "WebFrameLoaderClient.h"
#include <WebCore/Document.h>
#include <WebCore/Element.h>
#include <WebCore/Frame.h>
#include <WebCore/FrameLoader.h>
#include <WebCore/FrameView.h>
#include <WebCore/KURL.h>
#include <wtf/text/WTFString.h>

using namespace WebCore;

namespace WebKit {

PassRefPtr<InjectedBundleHitTestResult> InjectedBundleHitTestResult::create(const WebCore::HitTestResult& hitTestResult)
{
    return adoptRef(new InjectedBundleHitTestResult(hitTestResult));
}

PassRefPtr<InjectedBundleNodeHandle> InjectedBundleHitTestResult::nodeHandle() const
{
    return InjectedBundleNodeHandle::getOrCreate(m_hitTestResult.innerNonSharedNode());
}

WebFrame* InjectedBundleHitTestResult::frame() const
{
    Node* node = m_hitTestResult.innerNonSharedNode();
    if (!node)
        return 0;

    Document* document = node->document();
    if (!document)
        return 0;

    Frame* frame = document->frame();
    if (!frame)
        return 0;

    WebFrameLoaderClient* webFrameLoaderClient = toWebFrameLoaderClient(frame->loader()->client());
    return webFrameLoaderClient ? webFrameLoaderClient->webFrame() : 0;
}

WebFrame* InjectedBundleHitTestResult::targetFrame() const
{
    Frame* frame = m_hitTestResult.targetFrame();
    if (!frame)
        return 0;

    WebFrameLoaderClient* webFrameLoaderClient = toWebFrameLoaderClient(frame->loader()->client());
    return webFrameLoaderClient ? webFrameLoaderClient->webFrame() : 0;
}

String InjectedBundleHitTestResult::absoluteImageURL() const
{
    return m_hitTestResult.absoluteImageURL().string();
}

String InjectedBundleHitTestResult::absolutePDFURL() const
{
    return m_hitTestResult.absolutePDFURL().string();
}

String InjectedBundleHitTestResult::absoluteLinkURL() const
{
    return m_hitTestResult.absoluteLinkURL().string();
}

String InjectedBundleHitTestResult::absoluteMediaURL() const
{
    return m_hitTestResult.absoluteMediaURL().string();
}

bool InjectedBundleHitTestResult::mediaIsInFullscreen() const
{
    return m_hitTestResult.mediaIsInFullscreen();
}

bool InjectedBundleHitTestResult::mediaHasAudio() const
{
    return m_hitTestResult.mediaHasAudio();
}

BundleHitTestResultMediaType InjectedBundleHitTestResult::mediaType() const
{
#if !ENABLE(VIDEO)
    return BundleHitTestResultMediaTypeNone;
#else
    WebCore::Node* node = m_hitTestResult.innerNonSharedNode();
    if (!node->isElementNode())
        return BundleHitTestResultMediaTypeNone;
    
    if (!toElement(node)->isMediaElement())
        return BundleHitTestResultMediaTypeNone;
    
    return m_hitTestResult.mediaIsVideo() ? BundleHitTestResultMediaTypeVideo : BundleHitTestResultMediaTypeAudio;    
#endif
}

String InjectedBundleHitTestResult::linkLabel() const
{
    return m_hitTestResult.textContent();
}

String InjectedBundleHitTestResult::linkTitle() const
{
    return m_hitTestResult.titleDisplayString();
}

WebCore::IntRect InjectedBundleHitTestResult::imageRect() const
{
    WebCore::IntRect imageRect = m_hitTestResult.imageRect();
    if (imageRect.isEmpty())
        return imageRect;
        
    // The image rect in WebCore::HitTestResult is in frame coordinates, but we need it in WKView
    // coordinates since WebKit2 clients don't have enough context to do the conversion themselves.
    WebFrame* webFrame = frame();
    if (!webFrame)
        return imageRect;
    
    WebCore::Frame* coreFrame = webFrame->coreFrame();
    if (!coreFrame)
        return imageRect;
    
    WebCore::FrameView* view = coreFrame->view();
    if (!view)
        return imageRect;
    
    return view->contentsToRootView(imageRect);
}

bool InjectedBundleHitTestResult::isSelected() const
{
    return m_hitTestResult.isSelected();
}

} // WebKit
