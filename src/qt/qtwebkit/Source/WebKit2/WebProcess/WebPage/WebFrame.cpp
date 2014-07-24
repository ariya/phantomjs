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
#include "WebFrame.h"

#include "DownloadManager.h"
#include "InjectedBundleHitTestResult.h"
#include "InjectedBundleNodeHandle.h"
#include "InjectedBundleRangeHandle.h"
#include "InjectedBundleScriptWorld.h"
#include "PluginView.h"
#include "WKAPICast.h"
#include "WKBundleAPICast.h"
#include "WebChromeClient.h"
#include "WebPage.h"
#include "WebPageProxyMessages.h"
#include "WebProcess.h"
#include <JavaScriptCore/APICast.h>
#include <JavaScriptCore/JSContextRef.h>
#include <JavaScriptCore/JSLock.h>
#include <JavaScriptCore/JSValueRef.h>
#include <WebCore/ArchiveResource.h>
#include <WebCore/Chrome.h>
#include <WebCore/DocumentLoader.h>
#include <WebCore/EventHandler.h>
#include <WebCore/Frame.h>
#include <WebCore/FrameView.h>
#include <WebCore/HTMLFormElement.h>
#include <WebCore/HTMLFrameOwnerElement.h>
#include <WebCore/HTMLInputElement.h>
#include <WebCore/HTMLNames.h>
#include <WebCore/HTMLTextAreaElement.h>
#include <WebCore/JSCSSStyleDeclaration.h>
#include <WebCore/JSElement.h>
#include <WebCore/JSRange.h>
#include <WebCore/NetworkingContext.h>
#include <WebCore/NodeTraversal.h>
#include <WebCore/Page.h>
#include <WebCore/PluginDocument.h>
#include <WebCore/RenderTreeAsText.h>
#include <WebCore/ResourceBuffer.h>
#include <WebCore/ResourceLoader.h>
#include <WebCore/ScriptController.h>
#include <WebCore/SecurityOrigin.h>
#include <WebCore/TextIterator.h>
#include <WebCore/TextResourceDecoder.h>
#include <wtf/text/StringBuilder.h>

#if PLATFORM(MAC)
#include <WebCore/LegacyWebArchive.h>
#endif

#if ENABLE(NETWORK_PROCESS)
#include "NetworkConnectionToWebProcessMessages.h"
#include "NetworkProcessConnection.h"
#include "WebCoreArgumentCoders.h"
#endif

#ifndef NDEBUG
#include <wtf/RefCountedLeakCounter.h>
#endif

using namespace JSC;
using namespace WebCore;

namespace WebKit {

DEFINE_DEBUG_ONLY_GLOBAL(WTF::RefCountedLeakCounter, webFrameCounter, ("WebFrame"));

static uint64_t generateFrameID()
{
    static uint64_t uniqueFrameID = 1;
    return uniqueFrameID++;
}

static uint64_t generateListenerID()
{
    static uint64_t uniqueListenerID = 1;
    return uniqueListenerID++;
}

PassRefPtr<WebFrame> WebFrame::createMainFrame(WebPage* page)
{
    RefPtr<WebFrame> frame = create();

    page->send(Messages::WebPageProxy::DidCreateMainFrame(frame->frameID()));

    frame->init(page, String(), 0);

    return frame.release();
}

PassRefPtr<WebFrame> WebFrame::createSubframe(WebPage* page, const String& frameName, HTMLFrameOwnerElement* ownerElement)
{
    RefPtr<WebFrame> frame = create();

    page->send(Messages::WebPageProxy::DidCreateSubframe(frame->frameID()));

    frame->init(page, frameName, ownerElement);

    return frame.release();
}

PassRefPtr<WebFrame> WebFrame::create()
{
    RefPtr<WebFrame> frame = adoptRef(new WebFrame);

    // Add explict ref() that will be balanced in WebFrameLoaderClient::frameLoaderDestroyed().
    frame->ref();

    return frame.release();
}

WebFrame::WebFrame()
    : m_coreFrame(0)
    , m_policyListenerID(0)
    , m_policyFunction(0)
    , m_policyDownloadID(0)
    , m_frameLoaderClient(this)
    , m_loadListener(0)
    , m_frameID(generateFrameID())
{
    WebProcess::shared().addWebFrame(m_frameID, this);

#ifndef NDEBUG
    webFrameCounter.increment();
#endif
}

WebFrame::~WebFrame()
{
    ASSERT(!m_coreFrame);

#ifndef NDEBUG
    webFrameCounter.decrement();
#endif
}

void WebFrame::init(WebPage* page, const String& frameName, HTMLFrameOwnerElement* ownerElement)
{
    RefPtr<Frame> frame = Frame::create(page->corePage(), ownerElement, &m_frameLoaderClient);
    m_coreFrame = frame.get();

    frame->tree()->setName(frameName);

    if (ownerElement) {
        ASSERT(ownerElement->document()->frame());
        ownerElement->document()->frame()->tree()->appendChild(frame);
    }

    frame->init();
}

WebPage* WebFrame::page() const
{ 
    if (!m_coreFrame)
        return 0;
    
    if (Page* page = m_coreFrame->page())
        return WebPage::fromCorePage(page);

    return 0;
}

void WebFrame::invalidate()
{
    WebProcess::shared().removeWebFrame(m_frameID);
    m_coreFrame = 0;
}

uint64_t WebFrame::setUpPolicyListener(WebCore::FramePolicyFunction policyFunction)
{
    // FIXME: <rdar://5634381> We need to support multiple active policy listeners.

    invalidatePolicyListener();

    m_policyListenerID = generateListenerID();
    m_policyFunction = policyFunction;
    return m_policyListenerID;
}

void WebFrame::invalidatePolicyListener()
{
    if (!m_policyListenerID)
        return;

    m_policyDownloadID = 0;
    m_policyListenerID = 0;
    m_policyFunction = 0;
}

void WebFrame::didReceivePolicyDecision(uint64_t listenerID, PolicyAction action, uint64_t downloadID)
{
    if (!m_coreFrame)
        return;

    if (!m_policyListenerID)
        return;

    if (listenerID != m_policyListenerID)
        return;

    ASSERT(m_policyFunction);

    FramePolicyFunction function = m_policyFunction;

    invalidatePolicyListener();

    m_policyDownloadID = downloadID;

    (m_coreFrame->loader()->policyChecker()->*function)(action);
}

void WebFrame::startDownload(const WebCore::ResourceRequest& request)
{
    ASSERT(m_policyDownloadID);

    uint64_t policyDownloadID = m_policyDownloadID;
    m_policyDownloadID = 0;

#if ENABLE(NETWORK_PROCESS)
    if (WebProcess::shared().usesNetworkProcess()) {
        bool privateBrowsingEnabled = m_coreFrame->loader()->networkingContext()->storageSession().isPrivateBrowsingSession();
        WebProcess::shared().networkConnection()->connection()->send(Messages::NetworkConnectionToWebProcess::StartDownload(privateBrowsingEnabled, policyDownloadID, request), 0);
        return;
    }
#endif

    WebProcess::shared().downloadManager().startDownload(policyDownloadID, request);
}

void WebFrame::convertMainResourceLoadToDownload(DocumentLoader* documentLoader, const ResourceRequest& request, const ResourceResponse& response)
{
    ASSERT(m_policyDownloadID);

    uint64_t policyDownloadID = m_policyDownloadID;
    m_policyDownloadID = 0;

    ResourceLoader* mainResourceLoader = documentLoader->mainResourceLoader();

#if ENABLE(NETWORK_PROCESS)
    if (WebProcess::shared().usesNetworkProcess()) {
        // Use 0 to indicate that there is no main resource loader.
        // This can happen if the main resource is in the WebCore memory cache.
        uint64_t mainResourceLoadIdentifier;
        if (mainResourceLoader)
            mainResourceLoadIdentifier = mainResourceLoader->identifier();
        else
            mainResourceLoadIdentifier = 0;

        WebProcess::shared().networkConnection()->connection()->send(Messages::NetworkConnectionToWebProcess::ConvertMainResourceLoadToDownload(mainResourceLoadIdentifier, policyDownloadID, request, response), 0);
        return;
    }
#endif

    if (!mainResourceLoader) {
        // The main resource has already been loaded. Start a new download instead.
        WebProcess::shared().downloadManager().startDownload(policyDownloadID, request);
        return;
    }

    WebProcess::shared().downloadManager().convertHandleToDownload(policyDownloadID, documentLoader->mainResourceLoader()->handle(), request, response);
}

String WebFrame::source() const 
{
    if (!m_coreFrame)
        return String();
    Document* document = m_coreFrame->document();
    if (!document)
        return String();
    TextResourceDecoder* decoder = document->decoder();
    if (!decoder)
        return String();
    DocumentLoader* documentLoader = m_coreFrame->loader()->activeDocumentLoader();
    if (!documentLoader)
        return String();
    RefPtr<ResourceBuffer> mainResourceData = documentLoader->mainResourceData();
    if (!mainResourceData)
        return String();
    return decoder->encoding().decode(mainResourceData->data(), mainResourceData->size());
}

String WebFrame::contentsAsString() const 
{
    if (!m_coreFrame)
        return String();

    if (isFrameSet()) {
        StringBuilder builder;
        for (Frame* child = m_coreFrame->tree()->firstChild(); child; child = child->tree()->nextSibling()) {
            if (!builder.isEmpty())
                builder.append(' ');

            WebFrameLoaderClient* webFrameLoaderClient = toWebFrameLoaderClient(child->loader()->client());
            WebFrame* webFrame = webFrameLoaderClient ? webFrameLoaderClient->webFrame() : 0;
            ASSERT(webFrame);

            builder.append(webFrame->contentsAsString());
        }
        // FIXME: It may make sense to use toStringPreserveCapacity() here.
        return builder.toString();
    }

    Document* document = m_coreFrame->document();
    if (!document)
        return String();

    RefPtr<Element> documentElement = document->documentElement();
    if (!documentElement)
        return String();

    RefPtr<Range> range = document->createRange();

    ExceptionCode ec = 0;
    range->selectNode(documentElement.get(), ec);
    if (ec)
        return String();

    return plainText(range.get());
}

String WebFrame::selectionAsString() const 
{
    if (!m_coreFrame)
        return String();

    return m_coreFrame->displayStringModifiedByEncoding(m_coreFrame->editor().selectedText());
}

IntSize WebFrame::size() const
{
    if (!m_coreFrame)
        return IntSize();

    FrameView* frameView = m_coreFrame->view();
    if (!frameView)
        return IntSize();

    return frameView->contentsSize();
}

bool WebFrame::isFrameSet() const
{
    if (!m_coreFrame)
        return false;

    Document* document = m_coreFrame->document();
    if (!document)
        return false;
    return document->isFrameSet();
}

bool WebFrame::isMainFrame() const
{
    if (WebPage* p = page())
        return p->mainWebFrame() == this;

    return false;
}

String WebFrame::name() const
{
    if (!m_coreFrame)
        return String();

    return m_coreFrame->tree()->uniqueName();
}

String WebFrame::url() const
{
    if (!m_coreFrame)
        return String();

    DocumentLoader* documentLoader = m_coreFrame->loader()->documentLoader();
    if (!documentLoader)
        return String();

    return documentLoader->url().string();
}

String WebFrame::innerText() const
{
    if (!m_coreFrame)
        return String();

    if (!m_coreFrame->document()->documentElement())
        return String();

    return m_coreFrame->document()->documentElement()->innerText();
}

WebFrame* WebFrame::parentFrame() const
{
    if (!m_coreFrame || !m_coreFrame->ownerElement() || !m_coreFrame->ownerElement()->document())
        return 0;

    WebFrameLoaderClient* webFrameLoaderClient = toWebFrameLoaderClient(m_coreFrame->ownerElement()->document()->frame()->loader()->client());
    return webFrameLoaderClient ? webFrameLoaderClient->webFrame() : 0;
}

PassRefPtr<ImmutableArray> WebFrame::childFrames()
{
    if (!m_coreFrame)
        return ImmutableArray::create();

    size_t size = m_coreFrame->tree()->childCount();
    if (!size)
        return ImmutableArray::create();

    Vector<RefPtr<APIObject> > vector;
    vector.reserveInitialCapacity(size);

    for (Frame* child = m_coreFrame->tree()->firstChild(); child; child = child->tree()->nextSibling()) {
        WebFrameLoaderClient* webFrameLoaderClient = toWebFrameLoaderClient(child->loader()->client());
        WebFrame* webFrame = webFrameLoaderClient ? webFrameLoaderClient->webFrame() : 0;
        ASSERT(webFrame);
        vector.uncheckedAppend(webFrame);
    }

    return ImmutableArray::adopt(vector);
}

String WebFrame::layerTreeAsText() const
{
    if (!m_coreFrame)
        return "";

    return m_coreFrame->layerTreeAsText(0);
}

unsigned WebFrame::pendingUnloadCount() const
{
    if (!m_coreFrame)
        return 0;

    return m_coreFrame->document()->domWindow()->pendingUnloadEventListeners();
}

bool WebFrame::allowsFollowingLink(const WebCore::KURL& url) const
{
    if (!m_coreFrame)
        return true;
        
    return m_coreFrame->document()->securityOrigin()->canDisplay(url);
}

JSGlobalContextRef WebFrame::jsContext()
{
    return toGlobalRef(m_coreFrame->script()->globalObject(mainThreadNormalWorld())->globalExec());
}

JSGlobalContextRef WebFrame::jsContextForWorld(InjectedBundleScriptWorld* world)
{
    return toGlobalRef(m_coreFrame->script()->globalObject(world->coreWorld())->globalExec());
}

bool WebFrame::handlesPageScaleGesture() const
{
    if (!m_coreFrame->document()->isPluginDocument())
        return 0;

    PluginDocument* pluginDocument = static_cast<PluginDocument*>(m_coreFrame->document());
    PluginView* pluginView = static_cast<PluginView*>(pluginDocument->pluginWidget());
    return pluginView && pluginView->handlesPageScaleFactor();
}

IntRect WebFrame::contentBounds() const
{    
    if (!m_coreFrame)
        return IntRect();
    
    FrameView* view = m_coreFrame->view();
    if (!view)
        return IntRect();
    
    return IntRect(0, 0, view->contentsWidth(), view->contentsHeight());
}

IntRect WebFrame::visibleContentBounds() const
{
    if (!m_coreFrame)
        return IntRect();
    
    FrameView* view = m_coreFrame->view();
    if (!view)
        return IntRect();
    
    IntRect contentRect = view->visibleContentRect(ScrollableArea::IncludeScrollbars);
    return IntRect(0, 0, contentRect.width(), contentRect.height());
}

IntRect WebFrame::visibleContentBoundsExcludingScrollbars() const
{
    if (!m_coreFrame)
        return IntRect();
    
    FrameView* view = m_coreFrame->view();
    if (!view)
        return IntRect();
    
    IntRect contentRect = view->visibleContentRect();
    return IntRect(0, 0, contentRect.width(), contentRect.height());
}

IntSize WebFrame::scrollOffset() const
{
    if (!m_coreFrame)
        return IntSize();
    
    FrameView* view = m_coreFrame->view();
    if (!view)
        return IntSize();

    return view->scrollOffset();
}

bool WebFrame::hasHorizontalScrollbar() const
{
    if (!m_coreFrame)
        return false;

    FrameView* view = m_coreFrame->view();
    if (!view)
        return false;

    return view->horizontalScrollbar();
}

bool WebFrame::hasVerticalScrollbar() const
{
    if (!m_coreFrame)
        return false;

    FrameView* view = m_coreFrame->view();
    if (!view)
        return false;

    return view->verticalScrollbar();
}

PassRefPtr<InjectedBundleHitTestResult> WebFrame::hitTest(const IntPoint point) const
{
    if (!m_coreFrame)
        return 0;

    return InjectedBundleHitTestResult::create(m_coreFrame->eventHandler()->hitTestResultAtPoint(point, HitTestRequest::ReadOnly | HitTestRequest::Active | HitTestRequest::IgnoreClipping | HitTestRequest::DisallowShadowContent));
}

bool WebFrame::getDocumentBackgroundColor(double* red, double* green, double* blue, double* alpha)
{
    if (!m_coreFrame)
        return false;

    FrameView* view = m_coreFrame->view();
    if (!view)
        return false;

    Color bgColor = view->documentBackgroundColor();
    if (!bgColor.isValid())
        return false;

    bgColor.getRGBA(*red, *green, *blue, *alpha);
    return true;
}

bool WebFrame::containsAnyFormElements() const
{
    if (!m_coreFrame)
        return false;
    
    Document* document = m_coreFrame->document();
    if (!document)
        return false;

    for (Node* node = document->documentElement(); node; node = NodeTraversal::next(node)) {
        if (!node->isElementNode())
            continue;
        if (isHTMLFormElement(node))
            return true;
    }
    return false;
}

bool WebFrame::containsAnyFormControls() const
{
    if (!m_coreFrame)
        return false;
    
    Document* document = m_coreFrame->document();
    if (!document)
        return false;

    for (Node* node = document->documentElement(); node; node = NodeTraversal::next(node)) {
        if (!node->isElementNode())
            continue;
        if (isHTMLInputElement(node) || toElement(node)->hasTagName(HTMLNames::selectTag) || isHTMLTextAreaElement(node))
            return true;
    }
    return false;
}

void WebFrame::stopLoading()
{
    if (!m_coreFrame)
        return;

    m_coreFrame->loader()->stopForUserCancel();
}

WebFrame* WebFrame::frameForContext(JSContextRef context)
{
    JSObjectRef globalObjectRef = JSContextGetGlobalObject(context);
    JSC::JSObject* globalObjectObj = toJS(globalObjectRef);
    if (strcmp(globalObjectObj->classInfo()->className, "JSDOMWindowShell") != 0)
        return 0;

    Frame* coreFrame = static_cast<JSDOMWindowShell*>(globalObjectObj)->window()->impl()->frame();

    WebFrameLoaderClient* webFrameLoaderClient = toWebFrameLoaderClient(coreFrame->loader()->client());
    return webFrameLoaderClient ? webFrameLoaderClient->webFrame() : 0;
}

JSValueRef WebFrame::jsWrapperForWorld(InjectedBundleNodeHandle* nodeHandle, InjectedBundleScriptWorld* world)
{
    if (!m_coreFrame)
        return 0;

    JSDOMWindow* globalObject = m_coreFrame->script()->globalObject(world->coreWorld());
    ExecState* exec = globalObject->globalExec();

    JSLockHolder lock(exec);
    return toRef(exec, toJS(exec, globalObject, nodeHandle->coreNode()));
}

JSValueRef WebFrame::jsWrapperForWorld(InjectedBundleRangeHandle* rangeHandle, InjectedBundleScriptWorld* world)
{
    if (!m_coreFrame)
        return 0;

    JSDOMWindow* globalObject = m_coreFrame->script()->globalObject(world->coreWorld());
    ExecState* exec = globalObject->globalExec();

    JSLockHolder lock(exec);
    return toRef(exec, toJS(exec, globalObject, rangeHandle->coreRange()));
}

String WebFrame::counterValue(JSObjectRef element)
{
    if (!toJS(element)->inherits(&JSElement::s_info))
        return String();

    return counterValueForElement(static_cast<JSElement*>(toJS(element))->impl());
}

String WebFrame::provisionalURL() const
{
    if (!m_coreFrame)
        return String();

    return m_coreFrame->loader()->provisionalDocumentLoader()->url().string();
}

String WebFrame::suggestedFilenameForResourceWithURL(const KURL& url) const
{
    if (!m_coreFrame)
        return String();

    DocumentLoader* loader = m_coreFrame->loader()->documentLoader();
    if (!loader)
        return String();

    // First, try the main resource.
    if (loader->url() == url)
        return loader->response().suggestedFilename();

    // Next, try subresources.
    RefPtr<ArchiveResource> resource = loader->subresource(url);
    if (resource)
        return resource->response().suggestedFilename();

    return page()->cachedSuggestedFilenameForURL(url);
}

String WebFrame::mimeTypeForResourceWithURL(const KURL& url) const
{
    if (!m_coreFrame)
        return String();

    DocumentLoader* loader = m_coreFrame->loader()->documentLoader();
    if (!loader)
        return String();

    // First, try the main resource.
    if (loader->url() == url)
        return loader->response().mimeType();

    // Next, try subresources.
    RefPtr<ArchiveResource> resource = loader->subresource(url);
    if (resource)
        return resource->mimeType();

    return page()->cachedResponseMIMETypeForURL(url);
}

void WebFrame::setTextDirection(const String& direction)
{
    if (!m_coreFrame)
        return;

    if (direction == "auto")
        m_coreFrame->editor().setBaseWritingDirection(NaturalWritingDirection);
    else if (direction == "ltr")
        m_coreFrame->editor().setBaseWritingDirection(LeftToRightWritingDirection);
    else if (direction == "rtl")
        m_coreFrame->editor().setBaseWritingDirection(RightToLeftWritingDirection);
}

#if PLATFORM(MAC)

class WebFrameFilter : public FrameFilter {
public:
    WebFrameFilter(WebFrame*, WebFrame::FrameFilterFunction, void* context);
        
private:
    virtual bool shouldIncludeSubframe(Frame*) const OVERRIDE;

    WebFrame* m_topLevelWebFrame;
    WebFrame::FrameFilterFunction m_callback;
    void* m_context;
};

WebFrameFilter::WebFrameFilter(WebFrame* topLevelWebFrame, WebFrame::FrameFilterFunction callback, void* context)
    : m_topLevelWebFrame(topLevelWebFrame)
    , m_callback(callback)
    , m_context(context)
{
}

bool WebFrameFilter::shouldIncludeSubframe(Frame* frame) const
{
    if (!m_callback)
        return true;

    WebFrameLoaderClient* webFrameLoaderClient = toWebFrameLoaderClient(frame->loader()->client());
    WebFrame* webFrame = webFrameLoaderClient ? webFrameLoaderClient->webFrame() : 0;
    ASSERT(webFrame);

    return m_callback(toAPI(m_topLevelWebFrame), toAPI(webFrame), m_context);
}

RetainPtr<CFDataRef> WebFrame::webArchiveData(FrameFilterFunction callback, void* context)
{
    WebFrameFilter filter(this, callback, context);

    if (RefPtr<LegacyWebArchive> archive = LegacyWebArchive::create(coreFrame()->document(), &filter))
        return archive->rawDataRepresentation();
    
    return 0;
}
#endif
    
} // namespace WebKit
