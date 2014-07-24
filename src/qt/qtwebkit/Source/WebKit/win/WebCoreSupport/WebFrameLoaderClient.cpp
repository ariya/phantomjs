/*
 * Copyright (C) 2006, 2007, 2008, 2009 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "WebFrameLoaderClient.h"

#include "CFDictionaryPropertyBag.h"
#include "COMPropertyBag.h"
#include "DOMHTMLClasses.h"
#include "EmbeddedWidget.h"
#include "MarshallingHelpers.h"
#include "NotImplemented.h"
#include "WebCachedFramePlatformData.h"
#include "WebChromeClient.h"
#include "WebDocumentLoader.h"
#include "WebError.h"
#include "WebFrame.h"
#include "WebHistory.h"
#include "WebHistoryItem.h"
#include "WebMutableURLRequest.h"
#include "WebNavigationData.h"
#include "WebNotificationCenter.h"
#include "WebSecurityOrigin.h"
#include "WebURLAuthenticationChallenge.h"
#include "WebURLResponse.h"
#include "WebView.h"
#include <WebCore/BackForwardController.h>
#include <WebCore/CachedFrame.h>
#include <WebCore/DocumentLoader.h>
#include <WebCore/Frame.h>
#include <WebCore/FrameLoader.h>
#include <WebCore/FrameTree.h>
#include <WebCore/FrameView.h>
#include <WebCore/HTMLAppletElement.h>
#include <WebCore/HTMLFrameElement.h>
#include <WebCore/HTMLFrameOwnerElement.h>
#include <WebCore/HTMLNames.h>
#include <WebCore/HTMLParserIdioms.h>
#include <WebCore/HTMLPlugInElement.h>
#include <WebCore/HistoryItem.h>
#include <WebCore/Page.h>
#include <WebCore/PluginPackage.h>
#include <WebCore/PluginView.h>
#include <WebCore/RenderPart.h>
#include <WebCore/ResourceHandle.h>
#include <WebCore/ResourceLoader.h>
#include <WebCore/Settings.h>

using namespace WebCore;
using namespace HTMLNames;

static WebDataSource* getWebDataSource(DocumentLoader* loader)
{
    return loader ? static_cast<WebDocumentLoader*>(loader)->dataSource() : 0;
}

WebFrameLoaderClient::WebFrameLoaderClient(WebFrame* webFrame)
    : m_webFrame(webFrame)
    , m_manualLoader(0) 
    , m_hasSentResponseToPlugin(false) 
{
    ASSERT_ARG(webFrame, webFrame);
}

WebFrameLoaderClient::~WebFrameLoaderClient()
{
}

bool WebFrameLoaderClient::hasWebView() const
{
    return m_webFrame->webView();
}

void WebFrameLoaderClient::forceLayout()
{
    Frame* frame = core(m_webFrame);
    if (!frame)
        return;

    if (frame->document() && frame->document()->inPageCache())
        return;

    FrameView* view = frame->view();
    if (!view)
        return;

    view->setNeedsLayout();
    view->forceLayout(true);
}

void WebFrameLoaderClient::assignIdentifierToInitialRequest(unsigned long identifier, DocumentLoader* loader, const ResourceRequest& request)
{
    WebView* webView = m_webFrame->webView();
    COMPtr<IWebResourceLoadDelegate> resourceLoadDelegate;
    if (FAILED(webView->resourceLoadDelegate(&resourceLoadDelegate)))
        return;

    COMPtr<WebMutableURLRequest> webURLRequest(AdoptCOM, WebMutableURLRequest::createInstance(request));
    resourceLoadDelegate->identifierForInitialRequest(webView, webURLRequest.get(), getWebDataSource(loader), identifier);
}

bool WebFrameLoaderClient::shouldUseCredentialStorage(DocumentLoader* loader, unsigned long identifier)
{
    WebView* webView = m_webFrame->webView();
    COMPtr<IWebResourceLoadDelegate> resourceLoadDelegate;
    if (FAILED(webView->resourceLoadDelegate(&resourceLoadDelegate)))
        return true;

    COMPtr<IWebResourceLoadDelegatePrivate> resourceLoadDelegatePrivate;
    if (FAILED(resourceLoadDelegate->QueryInterface(IID_IWebResourceLoadDelegatePrivate, reinterpret_cast<void**>(&resourceLoadDelegatePrivate))))
        return true;

    BOOL shouldUse;
    if (SUCCEEDED(resourceLoadDelegatePrivate->shouldUseCredentialStorage(webView, identifier, getWebDataSource(loader), &shouldUse)))
        return shouldUse;

    return true;
}

void WebFrameLoaderClient::dispatchDidReceiveAuthenticationChallenge(DocumentLoader* loader, unsigned long identifier, const AuthenticationChallenge& challenge)
{
    ASSERT(challenge.authenticationClient());

    WebView* webView = m_webFrame->webView();
    COMPtr<IWebResourceLoadDelegate> resourceLoadDelegate;
    if (SUCCEEDED(webView->resourceLoadDelegate(&resourceLoadDelegate))) {
        COMPtr<WebURLAuthenticationChallenge> webChallenge(AdoptCOM, WebURLAuthenticationChallenge::createInstance(challenge));
        if (SUCCEEDED(resourceLoadDelegate->didReceiveAuthenticationChallenge(webView, identifier, webChallenge.get(), getWebDataSource(loader))))
            return;
    }

    // If the ResourceLoadDelegate doesn't exist or fails to handle the call, we tell the ResourceHandle
    // to continue without credential - this is the best approximation of Mac behavior
    challenge.authenticationClient()->receivedRequestToContinueWithoutCredential(challenge);
}

void WebFrameLoaderClient::dispatchDidCancelAuthenticationChallenge(DocumentLoader* loader, unsigned long identifier, const AuthenticationChallenge& challenge)
{
    WebView* webView = m_webFrame->webView();
    COMPtr<IWebResourceLoadDelegate> resourceLoadDelegate;
    if (FAILED(webView->resourceLoadDelegate(&resourceLoadDelegate)))
        return;

    COMPtr<WebURLAuthenticationChallenge> webChallenge(AdoptCOM, WebURLAuthenticationChallenge::createInstance(challenge));
    resourceLoadDelegate->didCancelAuthenticationChallenge(webView, identifier, webChallenge.get(), getWebDataSource(loader));
}

void WebFrameLoaderClient::dispatchWillSendRequest(DocumentLoader* loader, unsigned long identifier, ResourceRequest& request, const ResourceResponse& redirectResponse)
{
    WebView* webView = m_webFrame->webView();
    COMPtr<IWebResourceLoadDelegate> resourceLoadDelegate;
    if (FAILED(webView->resourceLoadDelegate(&resourceLoadDelegate)))
        return;

    COMPtr<WebMutableURLRequest> webURLRequest(AdoptCOM, WebMutableURLRequest::createInstance(request));
    COMPtr<WebURLResponse> webURLRedirectResponse(AdoptCOM, WebURLResponse::createInstance(redirectResponse));

    COMPtr<IWebURLRequest> newWebURLRequest;
    if (FAILED(resourceLoadDelegate->willSendRequest(webView, identifier, webURLRequest.get(), webURLRedirectResponse.get(), getWebDataSource(loader), &newWebURLRequest)))
        return;

    if (webURLRequest == newWebURLRequest)
        return;

    if (!newWebURLRequest) {
        request = ResourceRequest();
        return;
    }

    COMPtr<WebMutableURLRequest> newWebURLRequestImpl(Query, newWebURLRequest);
    if (!newWebURLRequestImpl)
        return;

    request = newWebURLRequestImpl->resourceRequest();
}

void WebFrameLoaderClient::dispatchDidReceiveResponse(DocumentLoader* loader, unsigned long identifier, const ResourceResponse& response)
{
    WebView* webView = m_webFrame->webView();
    COMPtr<IWebResourceLoadDelegate> resourceLoadDelegate;
    if (FAILED(webView->resourceLoadDelegate(&resourceLoadDelegate)))
        return;

    COMPtr<WebURLResponse> webURLResponse(AdoptCOM, WebURLResponse::createInstance(response));
    resourceLoadDelegate->didReceiveResponse(webView, identifier, webURLResponse.get(), getWebDataSource(loader));
}

void WebFrameLoaderClient::dispatchDidReceiveContentLength(DocumentLoader* loader, unsigned long identifier, int length)
{
    WebView* webView = m_webFrame->webView();
    COMPtr<IWebResourceLoadDelegate> resourceLoadDelegate;
    if (FAILED(webView->resourceLoadDelegate(&resourceLoadDelegate)))
        return;

    resourceLoadDelegate->didReceiveContentLength(webView, identifier, length, getWebDataSource(loader));
}

void WebFrameLoaderClient::dispatchDidFinishLoading(DocumentLoader* loader, unsigned long identifier)
{
    WebView* webView = m_webFrame->webView();
    COMPtr<IWebResourceLoadDelegate> resourceLoadDelegate;
    if (FAILED(webView->resourceLoadDelegate(&resourceLoadDelegate)))
        return;

    resourceLoadDelegate->didFinishLoadingFromDataSource(webView, identifier, getWebDataSource(loader));
}

void WebFrameLoaderClient::dispatchDidFailLoading(DocumentLoader* loader, unsigned long identifier, const ResourceError& error)
{
    WebView* webView = m_webFrame->webView();
    COMPtr<IWebResourceLoadDelegate> resourceLoadDelegate;
    if (FAILED(webView->resourceLoadDelegate(&resourceLoadDelegate)))
        return;

    COMPtr<WebError> webError(AdoptCOM, WebError::createInstance(error));
    resourceLoadDelegate->didFailLoadingWithError(webView, identifier, webError.get(), getWebDataSource(loader));
}

bool WebFrameLoaderClient::shouldCacheResponse(DocumentLoader* loader, unsigned long identifier, const ResourceResponse& response, const unsigned char* data, const unsigned long long length)
{
    WebView* webView = m_webFrame->webView();
    COMPtr<IWebResourceLoadDelegate> resourceLoadDelegate;
    if (FAILED(webView->resourceLoadDelegate(&resourceLoadDelegate)))
        return true;

    COMPtr<IWebResourceLoadDelegatePrivate> resourceLoadDelegatePrivate(Query, resourceLoadDelegate);
    if (!resourceLoadDelegatePrivate)
        return true;

    COMPtr<IWebURLResponse> urlResponse(AdoptCOM, WebURLResponse::createInstance(response));
    BOOL shouldCache;
    if (SUCCEEDED(resourceLoadDelegatePrivate->shouldCacheResponse(webView, identifier, urlResponse.get(), data, length, getWebDataSource(loader), &shouldCache)))
        return shouldCache;

    return true;
}

void WebFrameLoaderClient::dispatchDidHandleOnloadEvents()
{
    WebView* webView = m_webFrame->webView();
    COMPtr<IWebFrameLoadDelegatePrivate> frameLoadDelegatePriv;
    if (SUCCEEDED(webView->frameLoadDelegatePrivate(&frameLoadDelegatePriv)) && frameLoadDelegatePriv)
        frameLoadDelegatePriv->didHandleOnloadEventsForFrame(webView, m_webFrame);
}

void WebFrameLoaderClient::dispatchDidReceiveServerRedirectForProvisionalLoad()
{
    WebView* webView = m_webFrame->webView();
    COMPtr<IWebFrameLoadDelegate> frameLoadDelegate;
    if (SUCCEEDED(webView->frameLoadDelegate(&frameLoadDelegate)))
        frameLoadDelegate->didReceiveServerRedirectForProvisionalLoadForFrame(webView, m_webFrame);
}

void WebFrameLoaderClient::dispatchDidCancelClientRedirect()
{
    WebView* webView = m_webFrame->webView();
    COMPtr<IWebFrameLoadDelegate> frameLoadDelegate;
    if (SUCCEEDED(webView->frameLoadDelegate(&frameLoadDelegate)))
        frameLoadDelegate->didCancelClientRedirectForFrame(webView, m_webFrame);
}

void WebFrameLoaderClient::dispatchWillPerformClientRedirect(const KURL& url, double delay, double fireDate)
{
    WebView* webView = m_webFrame->webView();
    COMPtr<IWebFrameLoadDelegate> frameLoadDelegate;
    if (SUCCEEDED(webView->frameLoadDelegate(&frameLoadDelegate)))
        frameLoadDelegate->willPerformClientRedirectToURL(webView, BString(url.string()), delay, MarshallingHelpers::CFAbsoluteTimeToDATE(fireDate), m_webFrame);
}

void WebFrameLoaderClient::dispatchDidChangeLocationWithinPage()
{
    WebView* webView = m_webFrame->webView();
    COMPtr<IWebFrameLoadDelegate> frameLoadDelegate;
    if (SUCCEEDED(webView->frameLoadDelegate(&frameLoadDelegate)))
        frameLoadDelegate->didChangeLocationWithinPageForFrame(webView, m_webFrame);
}

void WebFrameLoaderClient::dispatchDidPushStateWithinPage()
{
    WebView* webView = m_webFrame->webView();
    COMPtr<IWebFrameLoadDelegatePrivate> frameLoadDelegatePriv;
    if (FAILED(webView->frameLoadDelegatePrivate(&frameLoadDelegatePriv)) || !frameLoadDelegatePriv)
        return;

    COMPtr<IWebFrameLoadDelegatePrivate2> frameLoadDelegatePriv2(Query, frameLoadDelegatePriv);
    if (!frameLoadDelegatePriv2)
        return;

    frameLoadDelegatePriv2->didPushStateWithinPageForFrame(webView, m_webFrame);
}

void WebFrameLoaderClient::dispatchDidReplaceStateWithinPage()
{
    WebView* webView = m_webFrame->webView();
    COMPtr<IWebFrameLoadDelegatePrivate> frameLoadDelegatePriv;
    if (FAILED(webView->frameLoadDelegatePrivate(&frameLoadDelegatePriv)) || !frameLoadDelegatePriv)
        return;

    COMPtr<IWebFrameLoadDelegatePrivate2> frameLoadDelegatePriv2(Query, frameLoadDelegatePriv);
    if (!frameLoadDelegatePriv2)
        return;

    frameLoadDelegatePriv2->didReplaceStateWithinPageForFrame(webView, m_webFrame);
}

void WebFrameLoaderClient::dispatchDidPopStateWithinPage()
{
    WebView* webView = m_webFrame->webView();
    COMPtr<IWebFrameLoadDelegatePrivate> frameLoadDelegatePriv;
    if (FAILED(webView->frameLoadDelegatePrivate(&frameLoadDelegatePriv)) || !frameLoadDelegatePriv)
        return;

    COMPtr<IWebFrameLoadDelegatePrivate2> frameLoadDelegatePriv2(Query, frameLoadDelegatePriv);
    if (!frameLoadDelegatePriv2)
        return;

    frameLoadDelegatePriv2->didPopStateWithinPageForFrame(webView, m_webFrame);
}
void WebFrameLoaderClient::dispatchWillClose()
{
    WebView* webView = m_webFrame->webView();
    COMPtr<IWebFrameLoadDelegate> frameLoadDelegate;
    if (SUCCEEDED(webView->frameLoadDelegate(&frameLoadDelegate)))
        frameLoadDelegate->willCloseFrame(webView, m_webFrame);
}

void WebFrameLoaderClient::dispatchDidReceiveIcon()
{
    m_webFrame->webView()->dispatchDidReceiveIconFromWebFrame(m_webFrame);
}

void WebFrameLoaderClient::dispatchDidStartProvisionalLoad()
{
    WebView* webView = m_webFrame->webView();
    COMPtr<IWebFrameLoadDelegate> frameLoadDelegate;
    if (SUCCEEDED(webView->frameLoadDelegate(&frameLoadDelegate)))
        frameLoadDelegate->didStartProvisionalLoadForFrame(webView, m_webFrame);
}

void WebFrameLoaderClient::dispatchDidReceiveTitle(const StringWithDirection& title)
{
    WebView* webView = m_webFrame->webView();
    COMPtr<IWebFrameLoadDelegate> frameLoadDelegate;
    if (SUCCEEDED(webView->frameLoadDelegate(&frameLoadDelegate)))
        // FIXME: use direction of title.
        frameLoadDelegate->didReceiveTitle(webView, BString(title.string()), m_webFrame);
}

void WebFrameLoaderClient::dispatchDidChangeIcons(WebCore::IconType type)
{
    if (type != WebCore::Favicon)
        return;

    WebView* webView = m_webFrame->webView();
    COMPtr<IWebFrameLoadDelegatePrivate> frameLoadDelegatePriv;
    if (FAILED(webView->frameLoadDelegatePrivate(&frameLoadDelegatePriv)) || !frameLoadDelegatePriv)
        return;

    COMPtr<IWebFrameLoadDelegatePrivate2> frameLoadDelegatePriv2(Query, frameLoadDelegatePriv);
    if (!frameLoadDelegatePriv2)
        return;

    frameLoadDelegatePriv2->didChangeIcons(webView, m_webFrame);
}

void WebFrameLoaderClient::dispatchDidCommitLoad()
{
    WebView* webView = m_webFrame->webView();
    COMPtr<IWebFrameLoadDelegate> frameLoadDelegate;
    if (SUCCEEDED(webView->frameLoadDelegate(&frameLoadDelegate)))
        frameLoadDelegate->didCommitLoadForFrame(webView, m_webFrame);
}

void WebFrameLoaderClient::dispatchDidFinishDocumentLoad()
{
    WebView* webView = m_webFrame->webView();
    COMPtr<IWebFrameLoadDelegatePrivate> frameLoadDelegatePriv;
    if (SUCCEEDED(webView->frameLoadDelegatePrivate(&frameLoadDelegatePriv)) && frameLoadDelegatePriv)
        frameLoadDelegatePriv->didFinishDocumentLoadForFrame(webView, m_webFrame);
}

void WebFrameLoaderClient::dispatchDidFinishLoad()
{
    WebView* webView = m_webFrame->webView();
    COMPtr<IWebFrameLoadDelegate> frameLoadDelegate;
    if (SUCCEEDED(webView->frameLoadDelegate(&frameLoadDelegate)))
        frameLoadDelegate->didFinishLoadForFrame(webView, m_webFrame);
}

void WebFrameLoaderClient::dispatchDidLayout(LayoutMilestones milestones)
{
    WebView* webView = m_webFrame->webView();

    if (milestones & DidFirstLayout) {
        COMPtr<IWebFrameLoadDelegatePrivate> frameLoadDelegatePrivate;
        if (SUCCEEDED(webView->frameLoadDelegatePrivate(&frameLoadDelegatePrivate)) && frameLoadDelegatePrivate)
            frameLoadDelegatePrivate->didFirstLayoutInFrame(webView, m_webFrame);
    }

    if (milestones & DidFirstVisuallyNonEmptyLayout) {
        COMPtr<IWebFrameLoadDelegatePrivate> frameLoadDelegatePrivate;
        if (SUCCEEDED(webView->frameLoadDelegatePrivate(&frameLoadDelegatePrivate)) && frameLoadDelegatePrivate)
            frameLoadDelegatePrivate->didFirstVisuallyNonEmptyLayoutInFrame(webView, m_webFrame);
    }
}

Frame* WebFrameLoaderClient::dispatchCreatePage(const NavigationAction&)
{
    WebView* webView = m_webFrame->webView();

    COMPtr<IWebUIDelegate> ui;
    if (FAILED(webView->uiDelegate(&ui)))
        return 0;

    COMPtr<IWebView> newWebView;
    if (FAILED(ui->createWebViewWithRequest(webView, 0, &newWebView)))
        return 0;

    COMPtr<IWebFrame> mainFrame;
    if (FAILED(newWebView->mainFrame(&mainFrame)))
        return 0;

    COMPtr<WebFrame> mainFrameImpl(Query, mainFrame);
    return core(mainFrameImpl.get());
}

void WebFrameLoaderClient::dispatchShow()
{
    WebView* webView = m_webFrame->webView();
    COMPtr<IWebUIDelegate> ui;
    if (SUCCEEDED(webView->uiDelegate(&ui)))
        ui->webViewShow(webView);
}

void WebFrameLoaderClient::setMainDocumentError(DocumentLoader*, const ResourceError& error)
{
    if (!m_manualLoader)
        return;

    m_manualLoader->didFail(error);
    m_manualLoader = 0;
    m_hasSentResponseToPlugin = false;
}

void WebFrameLoaderClient::postProgressStartedNotification()
{
    static BSTR progressStartedName = SysAllocString(WebViewProgressStartedNotification);
    IWebNotificationCenter* notifyCenter = WebNotificationCenter::defaultCenterInternal();
    notifyCenter->postNotificationName(progressStartedName, static_cast<IWebView*>(m_webFrame->webView()), 0);
}

void WebFrameLoaderClient::postProgressEstimateChangedNotification()
{
    static BSTR progressEstimateChangedName = SysAllocString(WebViewProgressEstimateChangedNotification);
    IWebNotificationCenter* notifyCenter = WebNotificationCenter::defaultCenterInternal();
    notifyCenter->postNotificationName(progressEstimateChangedName, static_cast<IWebView*>(m_webFrame->webView()), 0);
}

void WebFrameLoaderClient::postProgressFinishedNotification()
{
    static BSTR progressFinishedName = SysAllocString(WebViewProgressFinishedNotification);
    IWebNotificationCenter* notifyCenter = WebNotificationCenter::defaultCenterInternal();
    notifyCenter->postNotificationName(progressFinishedName, static_cast<IWebView*>(m_webFrame->webView()), 0);
}

void WebFrameLoaderClient::committedLoad(DocumentLoader* loader, const char* data, int length)
{
    if (!m_manualLoader)
        loader->commitData(data, length);

    // If the document is a stand-alone media document, now is the right time to cancel the WebKit load.
    // FIXME: This code should be shared across all ports. <http://webkit.org/b/48762>.
    Frame* coreFrame = core(m_webFrame);
    if (coreFrame->document()->isMediaDocument())
        loader->cancelMainResourceLoad(pluginWillHandleLoadError(loader->response()));

    if (!m_manualLoader)
        return;

    if (!m_hasSentResponseToPlugin) {
        m_manualLoader->didReceiveResponse(loader->response());
        // didReceiveResponse sets up a new stream to the plug-in. on a full-page plug-in, a failure in
        // setting up this stream can cause the main document load to be cancelled, setting m_manualLoader
        // to null
        if (!m_manualLoader)
            return;
        m_hasSentResponseToPlugin = true;
    }
    m_manualLoader->didReceiveData(data, length);
}

void WebFrameLoaderClient::finishedLoading(DocumentLoader*)
{
    if (!m_manualLoader)
        return;

    m_manualLoader->didFinishLoading();
    m_manualLoader = 0;
    m_hasSentResponseToPlugin = false;
}

void WebFrameLoaderClient::updateGlobalHistory()
{
    DocumentLoader* loader = core(m_webFrame)->loader()->documentLoader();
    WebView* webView = m_webFrame->webView();
    COMPtr<IWebHistoryDelegate> historyDelegate;
    webView->historyDelegate(&historyDelegate);

    if (historyDelegate) {
        COMPtr<IWebURLResponse> urlResponse(AdoptCOM, WebURLResponse::createInstance(loader->response()));
        COMPtr<IWebURLRequest> urlRequest(AdoptCOM, WebMutableURLRequest::createInstance(loader->originalRequestCopy()));
        
        COMPtr<IWebNavigationData> navigationData(AdoptCOM, WebNavigationData::createInstance(
            loader->urlForHistory(), loader->title().string(), urlRequest.get(), urlResponse.get(), loader->substituteData().isValid(), loader->clientRedirectSourceForHistory()));

        historyDelegate->didNavigateWithNavigationData(webView, navigationData.get(), m_webFrame);
        return;
    }

    WebHistory* history = WebHistory::sharedHistory();
    if (!history)
        return;

    history->visitedURL(loader->urlForHistory(), loader->title().string(), loader->originalRequestCopy().httpMethod(), loader->urlForHistoryReflectsFailure(), !loader->clientRedirectSourceForHistory());
}

void WebFrameLoaderClient::updateGlobalHistoryRedirectLinks()
{
    WebView* webView = m_webFrame->webView();
    COMPtr<IWebHistoryDelegate> historyDelegate;
    webView->historyDelegate(&historyDelegate);

    WebHistory* history = WebHistory::sharedHistory();

    DocumentLoader* loader = core(m_webFrame)->loader()->documentLoader();
    ASSERT(loader->unreachableURL().isEmpty());

    if (!loader->clientRedirectSourceForHistory().isNull()) {
        if (historyDelegate) {
            BString sourceURL(loader->clientRedirectSourceForHistory());
            BString destURL(loader->clientRedirectDestinationForHistory());
            historyDelegate->didPerformClientRedirectFromURL(webView, sourceURL, destURL, m_webFrame);
        } else {
            if (history) {
                if (COMPtr<IWebHistoryItem> iWebHistoryItem = history->itemForURLString(loader->clientRedirectSourceForHistory())) {
                    COMPtr<WebHistoryItem> webHistoryItem(Query, iWebHistoryItem);
                    webHistoryItem->historyItem()->addRedirectURL(loader->clientRedirectDestinationForHistory());
                }
            }
        }
    }

    if (!loader->serverRedirectSourceForHistory().isNull()) {
        if (historyDelegate) {
            BString sourceURL(loader->serverRedirectSourceForHistory());
            BString destURL(loader->serverRedirectDestinationForHistory());
            historyDelegate->didPerformServerRedirectFromURL(webView, sourceURL, destURL, m_webFrame);
        } else {
            if (history) {
                if (COMPtr<IWebHistoryItem> iWebHistoryItem = history->itemForURLString(loader->serverRedirectSourceForHistory())) {
                    COMPtr<WebHistoryItem> webHistoryItem(Query, iWebHistoryItem);
                    webHistoryItem->historyItem()->addRedirectURL(loader->serverRedirectDestinationForHistory());
                }
            }
        }
    }
}

bool WebFrameLoaderClient::shouldGoToHistoryItem(HistoryItem*) const
{
    return true;
}

bool WebFrameLoaderClient::shouldStopLoadingForHistoryItem(HistoryItem*) const
{
    return true;
}

void WebFrameLoaderClient::updateGlobalHistoryItemForPage()
{
    HistoryItem* historyItem = 0;
    WebView* webView = m_webFrame->webView();

    if (Page* page = webView->page()) {
        if (!page->settings()->privateBrowsingEnabled())
            historyItem = page->backForward()->currentItem();
    }

    webView->setGlobalHistoryItem(historyItem);
}

void WebFrameLoaderClient::didDisplayInsecureContent()
{
    WebView* webView = m_webFrame->webView();
    COMPtr<IWebFrameLoadDelegatePrivate> frameLoadDelegatePriv;
    if (FAILED(webView->frameLoadDelegatePrivate(&frameLoadDelegatePriv)) || !frameLoadDelegatePriv)
        return;

    COMPtr<IWebFrameLoadDelegatePrivate2> frameLoadDelegatePriv2(Query, frameLoadDelegatePriv);
    if (!frameLoadDelegatePriv2)
        return;

    frameLoadDelegatePriv2->didDisplayInsecureContent(webView);
}

void WebFrameLoaderClient::didRunInsecureContent(SecurityOrigin* origin, const KURL& insecureURL)
{
    COMPtr<IWebSecurityOrigin> webSecurityOrigin = WebSecurityOrigin::createInstance(origin);

    WebView* webView = m_webFrame->webView();
    COMPtr<IWebFrameLoadDelegatePrivate> frameLoadDelegatePriv;
    if (FAILED(webView->frameLoadDelegatePrivate(&frameLoadDelegatePriv)) || !frameLoadDelegatePriv)
        return;

    COMPtr<IWebFrameLoadDelegatePrivate2> frameLoadDelegatePriv2(Query, frameLoadDelegatePriv);
    if (!frameLoadDelegatePriv2)
        return;

    frameLoadDelegatePriv2->didRunInsecureContent(webView, webSecurityOrigin.get());
}

void WebFrameLoaderClient::didDetectXSS(const KURL&, bool)
{
    // FIXME: propogate call into the private delegate.
}

PassRefPtr<DocumentLoader> WebFrameLoaderClient::createDocumentLoader(const ResourceRequest& request, const SubstituteData& substituteData)
{
    RefPtr<WebDocumentLoader> loader = WebDocumentLoader::create(request, substituteData);

    COMPtr<WebDataSource> dataSource(AdoptCOM, WebDataSource::createInstance(loader.get()));

    loader->setDataSource(dataSource.get());
    return loader.release();
}

void WebFrameLoaderClient::setTitle(const StringWithDirection& title, const KURL& url)
{
    WebView* webView = m_webFrame->webView();
    COMPtr<IWebHistoryDelegate> historyDelegate;
    webView->historyDelegate(&historyDelegate);
    if (historyDelegate) {
        BString titleBSTR(title.string());
        BString urlBSTR(url.string());
        historyDelegate->updateHistoryTitle(webView, titleBSTR, urlBSTR);
        return;
    }

    BOOL privateBrowsingEnabled = FALSE;
    COMPtr<IWebPreferences> preferences;
    if (SUCCEEDED(m_webFrame->webView()->preferences(&preferences)))
        preferences->privateBrowsingEnabled(&privateBrowsingEnabled);
    if (privateBrowsingEnabled)
        return;

    // update title in global history
    COMPtr<WebHistory> history = webHistory();
    if (!history)
        return;

    COMPtr<IWebHistoryItem> item;
    if (FAILED(history->itemForURL(BString(url.string()), &item)))
        return;

    COMPtr<IWebHistoryItemPrivate> itemPrivate(Query, item);
    if (!itemPrivate)
        return;

    itemPrivate->setTitle(BString(title.string()));
}

void WebFrameLoaderClient::savePlatformDataToCachedFrame(CachedFrame* cachedFrame)
{
#if USE(CFNETWORK)
    Frame* coreFrame = core(m_webFrame);
    if (!coreFrame)
        return;

    ASSERT(coreFrame->loader()->documentLoader() == cachedFrame->documentLoader());

    cachedFrame->setCachedFramePlatformData(adoptPtr(new WebCachedFramePlatformData(static_cast<IWebDataSource*>(getWebDataSource(coreFrame->loader()->documentLoader())))));
#else
    notImplemented();
#endif
}

void WebFrameLoaderClient::transitionToCommittedFromCachedFrame(CachedFrame*)
{
}

void WebFrameLoaderClient::transitionToCommittedForNewPage()
{
    WebView* view = m_webFrame->webView();

    RECT rect;
    view->frameRect(&rect);
    bool transparent = view->transparent();
    Color backgroundColor = transparent ? Color::transparent : Color::white;
    core(m_webFrame)->createView(IntRect(rect).size(), backgroundColor, transparent);
}

void WebFrameLoaderClient::didSaveToPageCache()
{
}

void WebFrameLoaderClient::didRestoreFromPageCache()
{
}

void WebFrameLoaderClient::dispatchDidBecomeFrameset(bool)
{
}

bool WebFrameLoaderClient::canCachePage() const
{
    return true;
}

PassRefPtr<Frame> WebFrameLoaderClient::createFrame(const KURL& url, const String& name, HTMLFrameOwnerElement* ownerElement,
                            const String& referrer, bool /*allowsScrolling*/, int /*marginWidth*/, int /*marginHeight*/)
{
    RefPtr<Frame> result = createFrame(url, name, ownerElement, referrer);
    if (!result)
        return 0;
    return result.release();
}

PassRefPtr<Frame> WebFrameLoaderClient::createFrame(const KURL& URL, const String& name, HTMLFrameOwnerElement* ownerElement, const String& referrer)
{
    Frame* coreFrame = core(m_webFrame);
    ASSERT(coreFrame);

    COMPtr<WebFrame> webFrame(AdoptCOM, WebFrame::createInstance());

    RefPtr<Frame> childFrame = webFrame->init(m_webFrame->webView(), coreFrame->page(), ownerElement);

    childFrame->tree()->setName(name);
    coreFrame->tree()->appendChild(childFrame);
    childFrame->init();

    coreFrame->loader()->loadURLIntoChildFrame(URL, referrer, childFrame.get());

    // The frame's onload handler may have removed it from the document.
    if (!childFrame->tree()->parent())
        return 0;

    return childFrame.release();
}

void WebFrameLoaderClient::dispatchDidFailToStartPlugin(const PluginView* pluginView) const
{
    WebView* webView = m_webFrame->webView();

    COMPtr<IWebResourceLoadDelegate> resourceLoadDelegate;
    if (FAILED(webView->resourceLoadDelegate(&resourceLoadDelegate)))
        return;

    RetainPtr<CFMutableDictionaryRef> userInfo = adoptCF(CFDictionaryCreateMutable(0, 0, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks));

    Frame* frame = core(m_webFrame);
    ASSERT(frame == pluginView->parentFrame());

    if (!pluginView->pluginsPage().isNull()) {
        KURL pluginPageURL = frame->document()->completeURL(stripLeadingAndTrailingHTMLSpaces(pluginView->pluginsPage()));
        if (pluginPageURL.protocolIsInHTTPFamily()) {
            static CFStringRef key = MarshallingHelpers::LPCOLESTRToCFStringRef(WebKitErrorPlugInPageURLStringKey);
            CFDictionarySetValue(userInfo.get(), key, pluginPageURL.string().createCFString().get());
        }
    }

    if (!pluginView->mimeType().isNull()) {
        static CFStringRef key = MarshallingHelpers::LPCOLESTRToCFStringRef(WebKitErrorMIMETypeKey);
        CFDictionarySetValue(userInfo.get(), key, pluginView->mimeType().createCFString().get());
    }

    if (pluginView->plugin()) {
        String pluginName = pluginView->plugin()->name();
        if (!pluginName.isNull()) {
            static CFStringRef key = MarshallingHelpers::LPCOLESTRToCFStringRef(WebKitErrorPlugInNameKey);
            CFDictionarySetValue(userInfo.get(), key, pluginName.createCFString().get());
        }
    }

    COMPtr<CFDictionaryPropertyBag> userInfoBag = CFDictionaryPropertyBag::createInstance();
    userInfoBag->setDictionary(userInfo.get());
 
    int errorCode = 0;
    switch (pluginView->status()) {
        case PluginStatusCanNotFindPlugin:
            errorCode = WebKitErrorCannotFindPlugIn;
            break;
        case PluginStatusCanNotLoadPlugin:
            errorCode = WebKitErrorCannotLoadPlugIn;
            break;
        default:
            ASSERT_NOT_REACHED();
    }

    ResourceError resourceError(String(WebKitErrorDomain), errorCode, pluginView->url().string(), String());
    COMPtr<IWebError> error(AdoptCOM, WebError::createInstance(resourceError, userInfoBag.get()));
     
    resourceLoadDelegate->plugInFailedWithError(webView, error.get(), getWebDataSource(frame->loader()->documentLoader()));
}

PassRefPtr<Widget> WebFrameLoaderClient::createPlugin(const IntSize& pluginSize, HTMLPlugInElement* element, const KURL& url, const Vector<String>& paramNames, const Vector<String>& paramValues, const String& mimeType, bool loadManually)
{
    WebView* webView = m_webFrame->webView();

    COMPtr<IWebUIDelegate> ui;
    if (SUCCEEDED(webView->uiDelegate(&ui)) && ui) {
        COMPtr<IWebUIDelegatePrivate> uiPrivate(Query, ui);

        if (uiPrivate) {
            // Assemble the view arguments in a property bag.
            HashMap<String, String> viewArguments;
            for (unsigned i = 0; i < paramNames.size(); i++) 
                viewArguments.set(paramNames[i], paramValues[i]);
            COMPtr<IPropertyBag> viewArgumentsBag(AdoptCOM, COMPropertyBag<String>::adopt(viewArguments));
            COMPtr<IDOMElement> containingElement(AdoptCOM, DOMElement::createInstance(element));

            HashMap<String, COMVariant> arguments;

            arguments.set(WebEmbeddedViewAttributesKey, viewArgumentsBag);
            arguments.set(WebEmbeddedViewBaseURLKey, url.string());
            arguments.set(WebEmbeddedViewContainingElementKey, containingElement);
            arguments.set(WebEmbeddedViewMIMETypeKey, mimeType);

            COMPtr<IPropertyBag> argumentsBag(AdoptCOM, COMPropertyBag<COMVariant>::adopt(arguments));

            COMPtr<IWebEmbeddedView> view;
            HRESULT result = uiPrivate->embeddedViewWithArguments(webView, m_webFrame, argumentsBag.get(), &view);
            if (SUCCEEDED(result)) {
                HWND parentWindow;
                HRESULT hr = webView->viewWindow((OLE_HANDLE*)&parentWindow);
                ASSERT(SUCCEEDED(hr));

                return EmbeddedWidget::create(view.get(), element, parentWindow, pluginSize);
            }
        }
    }

    Frame* frame = core(m_webFrame);
    RefPtr<PluginView> pluginView = PluginView::create(frame, pluginSize, element, url, paramNames, paramValues, mimeType, loadManually);

    if (pluginView->status() == PluginStatusLoadedSuccessfully)
        return pluginView;

    dispatchDidFailToStartPlugin(pluginView.get());

    return 0;
}

void WebFrameLoaderClient::redirectDataToPlugin(Widget* pluginWidget)
{
    // Ideally, this function shouldn't be necessary, see <rdar://problem/4852889>
    if (!pluginWidget || pluginWidget->isPluginView())
        m_manualLoader = toPluginView(pluginWidget);
    else 
        m_manualLoader = static_cast<EmbeddedWidget*>(pluginWidget);
}

WebHistory* WebFrameLoaderClient::webHistory() const
{
    if (m_webFrame != m_webFrame->webView()->topLevelFrame())
        return 0;

    return WebHistory::sharedHistory();
}

bool WebFrameLoaderClient::shouldAlwaysUsePluginDocument(const String& mimeType) const
{
    WebView* webView = m_webFrame->webView();
    if (!webView)
        return false;

    return webView->shouldUseEmbeddedView(mimeType);
}
