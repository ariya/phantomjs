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
#include "InjectedBundleNodeHandle.h"

#include "ShareableBitmap.h"
#include "WebFrame.h"
#include "WebFrameLoaderClient.h"
#include "WebImage.h"
#include <JavaScriptCore/APICast.h>
#include <WebCore/Document.h>
#include <WebCore/Frame.h>
#include <WebCore/FrameLoader.h>
#include <WebCore/FrameView.h>
#include <WebCore/GraphicsContext.h>
#include <WebCore/HTMLFrameElement.h>
#include <WebCore/HTMLIFrameElement.h>
#include <WebCore/HTMLInputElement.h>
#include <WebCore/HTMLNames.h>
#include <WebCore/HTMLTableCellElement.h>
#include <WebCore/HTMLTextAreaElement.h>
#include <WebCore/IntRect.h>
#include <WebCore/JSNode.h>
#include <WebCore/Node.h>
#include <WebCore/Page.h>
#include <WebCore/RenderObject.h>
#include <wtf/HashMap.h>
#include <wtf/text/WTFString.h>

using namespace WebCore;
using namespace HTMLNames;

namespace WebKit {

typedef HashMap<Node*, InjectedBundleNodeHandle*> DOMHandleCache;

static DOMHandleCache& domHandleCache()
{
    DEFINE_STATIC_LOCAL(DOMHandleCache, cache, ());
    return cache;
}

PassRefPtr<InjectedBundleNodeHandle> InjectedBundleNodeHandle::getOrCreate(JSContextRef, JSObjectRef object)
{
    Node* node = toNode(toJS(object));
    return getOrCreate(node);
}

PassRefPtr<InjectedBundleNodeHandle> InjectedBundleNodeHandle::getOrCreate(Node* node)
{
    if (!node)
        return 0;

    DOMHandleCache::AddResult result = domHandleCache().add(node, 0);
    if (!result.isNewEntry)
        return PassRefPtr<InjectedBundleNodeHandle>(result.iterator->value);

    RefPtr<InjectedBundleNodeHandle> nodeHandle = InjectedBundleNodeHandle::create(node);
    result.iterator->value = nodeHandle.get();
    return nodeHandle.release();
}

PassRefPtr<InjectedBundleNodeHandle> InjectedBundleNodeHandle::create(Node* node)
{
    return adoptRef(new InjectedBundleNodeHandle(node));
}

InjectedBundleNodeHandle::InjectedBundleNodeHandle(Node* node)
    : m_node(node)
{
}

InjectedBundleNodeHandle::~InjectedBundleNodeHandle()
{
    domHandleCache().remove(m_node.get());
}

Node* InjectedBundleNodeHandle::coreNode() const
{
    return m_node.get();
}

PassRefPtr<InjectedBundleNodeHandle> InjectedBundleNodeHandle::document()
{
    return getOrCreate(m_node->document());
}

// Additional DOM Operations
// Note: These should only be operations that are not exposed to JavaScript.

IntRect InjectedBundleNodeHandle::elementBounds() const
{
    if (!m_node->isElementNode())
        return IntRect();

    return toElement(m_node.get())->boundsInRootViewSpace();
}
    
IntRect InjectedBundleNodeHandle::renderRect(bool* isReplaced) const
{
    return m_node.get()->pixelSnappedRenderRect(isReplaced);
}

static PassRefPtr<WebImage> imageForRect(FrameView* frameView, const IntRect& rect, SnapshotOptions options)
{
    IntSize bitmapSize = rect.size();
    float scaleFactor = frameView->frame()->page()->deviceScaleFactor();
    bitmapSize.scale(scaleFactor);

    RefPtr<WebImage> snapshot = WebImage::create(bitmapSize, snapshotOptionsToImageOptions(options));
    if (!snapshot->bitmap())
        return 0;

    OwnPtr<GraphicsContext> graphicsContext = snapshot->bitmap()->createGraphicsContext();
    graphicsContext->clearRect(IntRect(IntPoint(), bitmapSize));
    graphicsContext->applyDeviceScaleFactor(scaleFactor);
    graphicsContext->translate(-rect.x(), -rect.y());

    FrameView::SelectionInSnaphot shouldPaintSelection = FrameView::IncludeSelection;
    if (options & SnapshotOptionsExcludeSelectionHighlighting)
        shouldPaintSelection = FrameView::ExcludeSelection;

    frameView->paintContentsForSnapshot(graphicsContext.get(), rect, shouldPaintSelection, FrameView::DocumentCoordinates);

    return snapshot.release();
}

PassRefPtr<WebImage> InjectedBundleNodeHandle::renderedImage(SnapshotOptions options)
{
    Document* document = m_node->document();
    if (!document)
        return 0;

    Frame* frame = document->frame();
    if (!frame)
        return 0;

    FrameView* frameView = frame->view();
    if (!frameView)
        return 0;

    document->updateLayout();

    RenderObject* renderer = m_node->renderer();
    if (!renderer)
        return 0;

    LayoutRect topLevelRect;
    IntRect paintingRect = pixelSnappedIntRect(renderer->paintingRootRect(topLevelRect));

    frameView->setNodeToDraw(m_node.get());
    RefPtr<WebImage> image = imageForRect(frameView, paintingRect, options);
    frameView->setNodeToDraw(0);

    return image.release();
}

void InjectedBundleNodeHandle::setHTMLInputElementValueForUser(const String& value)
{
    if (!isHTMLInputElement(m_node.get()))
        return;

    toHTMLInputElement(m_node.get())->setValueForUser(value);
}

bool InjectedBundleNodeHandle::isHTMLInputElementAutofilled() const
{
    if (!isHTMLInputElement(m_node.get()))
        return false;
    
    return toHTMLInputElement(m_node.get())->isAutofilled();
}

void InjectedBundleNodeHandle::setHTMLInputElementAutofilled(bool filled)
{
    if (!isHTMLInputElement(m_node.get()))
        return;

    toHTMLInputElement(m_node.get())->setAutofilled(filled);
}

bool InjectedBundleNodeHandle::htmlInputElementLastChangeWasUserEdit()
{
    if (!isHTMLInputElement(m_node.get()))
        return false;

    return toHTMLInputElement(m_node.get())->lastChangeWasUserEdit();
}

bool InjectedBundleNodeHandle::htmlTextAreaElementLastChangeWasUserEdit()
{
    if (!isHTMLTextAreaElement(m_node.get()))
        return false;

    return toHTMLTextAreaElement(m_node.get())->lastChangeWasUserEdit();
}

PassRefPtr<InjectedBundleNodeHandle> InjectedBundleNodeHandle::htmlTableCellElementCellAbove()
{
    if (!m_node->hasTagName(tdTag))
        return 0;

    return getOrCreate(static_cast<HTMLTableCellElement*>(m_node.get())->cellAbove());
}

PassRefPtr<WebFrame> InjectedBundleNodeHandle::documentFrame()
{
    if (!m_node->isDocumentNode())
        return 0;

    Frame* frame = static_cast<Document*>(m_node.get())->frame();
    if (!frame)
        return 0;

    WebFrameLoaderClient* webFrameLoaderClient = toWebFrameLoaderClient(frame->loader()->client());
    return webFrameLoaderClient ? webFrameLoaderClient->webFrame() : 0;
}

PassRefPtr<WebFrame> InjectedBundleNodeHandle::htmlFrameElementContentFrame()
{
    if (!m_node->hasTagName(frameTag))
        return 0;

    Frame* frame = static_cast<HTMLFrameElement*>(m_node.get())->contentFrame();
    if (!frame)
        return 0;

    WebFrameLoaderClient* webFrameLoaderClient = toWebFrameLoaderClient(frame->loader()->client());
    return webFrameLoaderClient ? webFrameLoaderClient->webFrame() : 0;
}

PassRefPtr<WebFrame> InjectedBundleNodeHandle::htmlIFrameElementContentFrame()
{
    if (!m_node->hasTagName(iframeTag))
        return 0;

    Frame* frame = toHTMLIFrameElement(m_node.get())->contentFrame();
    if (!frame)
        return 0;

    WebFrameLoaderClient* webFrameLoaderClient = toWebFrameLoaderClient(frame->loader()->client());
    return webFrameLoaderClient ? webFrameLoaderClient->webFrame() : 0;
}

} // namespace WebKit
