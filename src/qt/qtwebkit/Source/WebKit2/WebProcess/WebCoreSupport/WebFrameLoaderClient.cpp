/*
 * Copyright (C) 2010, 2011, 2012 Apple Inc. All rights reserved.
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
#include "WebFrameLoaderClient.h"

#include "AuthenticationManager.h"
#include "DataReference.h"
#include "InjectedBundle.h"
#include "InjectedBundleBackForwardListItem.h"
#include "InjectedBundleDOMWindowExtension.h"
#include "InjectedBundleNavigationAction.h"
#include "InjectedBundleUserMessageCoders.h"
#include "PlatformCertificateInfo.h"
#include "PluginView.h"
#include "WebBackForwardListProxy.h"
#include "WebContextMessages.h"
#include "WebCoreArgumentCoders.h"
#include "WebErrors.h"
#include "WebEvent.h"
#include "WebFrame.h"
#include "WebFrameNetworkingContext.h"
#include "WebFullScreenManager.h"
#include "WebIconDatabaseMessages.h"
#include "WebNavigationDataStore.h"
#include "WebPage.h"
#include "WebPageProxyMessages.h"
#include "WebProcess.h"
#include "WebProcessProxyMessages.h"
#include <JavaScriptCore/APICast.h>
#include <JavaScriptCore/JSObject.h>
#include <WebCore/Chrome.h>
#include <WebCore/DOMWrapperWorld.h>
#include <WebCore/DocumentLoader.h>
#include <WebCore/FormState.h>
#include <WebCore/Frame.h>
#include <WebCore/FrameLoadRequest.h>
#include <WebCore/FrameLoader.h>
#include <WebCore/FrameView.h>
#include <WebCore/HTMLAppletElement.h>
#include <WebCore/HTMLFormElement.h>
#include <WebCore/HistoryController.h>
#include <WebCore/HistoryItem.h>
#include <WebCore/MIMETypeRegistry.h>
#include <WebCore/MouseEvent.h>
#include <WebCore/NotImplemented.h>
#include <WebCore/Page.h>
#include <WebCore/PluginData.h>
#include <WebCore/PluginDocument.h>
#include <WebCore/ProgressTracker.h>
#include <WebCore/ResourceBuffer.h>
#include <WebCore/ResourceError.h>
#include <WebCore/Settings.h>
#include <WebCore/UIEventWithKeyState.h>
#include <WebCore/Widget.h>
#include <WebCore/WindowFeatures.h>

using namespace WebCore;

namespace WebKit {

WebFrameLoaderClient::WebFrameLoaderClient(WebFrame* frame)
    : m_frame(frame)
    , m_hasSentResponseToPluginView(false)
    , m_didCompletePageTransitionAlready(false)
    , m_frameCameFromPageCache(false)
{
}

WebFrameLoaderClient::~WebFrameLoaderClient()
{
}
    
void WebFrameLoaderClient::frameLoaderDestroyed()
{
    m_frame->invalidate();

    // Balances explicit ref() in WebFrame::createMainFrame and WebFrame::createSubframe.
    m_frame->deref();
}

bool WebFrameLoaderClient::hasWebView() const
{
    return m_frame->page();
}

void WebFrameLoaderClient::makeRepresentation(DocumentLoader*)
{
    notImplemented();
}

void WebFrameLoaderClient::forceLayout()
{
    notImplemented();
}

void WebFrameLoaderClient::forceLayoutForNonHTML()
{
    notImplemented();
}

void WebFrameLoaderClient::setCopiesOnScroll()
{
    notImplemented();
}

void WebFrameLoaderClient::detachedFromParent2()
{
    WebPage* webPage = m_frame->page();
    if (!webPage)
        return;

    RefPtr<APIObject> userData;

    // Notify the bundle client.
    webPage->injectedBundleLoaderClient().didRemoveFrameFromHierarchy(webPage, m_frame, userData);

    // Notify the UIProcess.
    webPage->send(Messages::WebPageProxy::DidRemoveFrameFromHierarchy(m_frame->frameID(), InjectedBundleUserMessageEncoder(userData.get())));

}

void WebFrameLoaderClient::detachedFromParent3()
{
    notImplemented();
}

void WebFrameLoaderClient::assignIdentifierToInitialRequest(unsigned long identifier, DocumentLoader* loader, const ResourceRequest& request)
{
    WebPage* webPage = m_frame->page();
    if (!webPage)
        return;

    bool pageIsProvisionallyLoading = false;
    if (FrameLoader* frameLoader = loader->frameLoader())
        pageIsProvisionallyLoading = frameLoader->provisionalDocumentLoader() == loader;

    webPage->injectedBundleResourceLoadClient().didInitiateLoadForResource(webPage, m_frame, identifier, request, pageIsProvisionallyLoading);
}

void WebFrameLoaderClient::dispatchWillSendRequest(DocumentLoader*, unsigned long identifier, ResourceRequest& request, const ResourceResponse& redirectResponse)
{
    WebPage* webPage = m_frame->page();
    if (!webPage)
        return;

    webPage->injectedBundleResourceLoadClient().willSendRequestForFrame(webPage, m_frame, identifier, request, redirectResponse);
}

bool WebFrameLoaderClient::shouldUseCredentialStorage(DocumentLoader*, unsigned long identifier)
{
    WebPage* webPage = m_frame->page();
    if (!webPage)
        return true;

    return webPage->injectedBundleResourceLoadClient().shouldUseCredentialStorage(webPage, m_frame, identifier);
}

void WebFrameLoaderClient::dispatchDidReceiveAuthenticationChallenge(DocumentLoader*, unsigned long, const AuthenticationChallenge& challenge)
{
    // FIXME: Authentication is a per-resource concept, but we don't do per-resource handling in the UIProcess at the API level quite yet.
    // Once we do, we might need to make sure authentication fits with our solution.

    WebPage* webPage = m_frame->page();
    if (!webPage)
        return;

    WebProcess::shared().supplement<AuthenticationManager>()->didReceiveAuthenticationChallenge(m_frame, challenge);
}

void WebFrameLoaderClient::dispatchDidCancelAuthenticationChallenge(DocumentLoader*, unsigned long /*identifier*/, const AuthenticationChallenge&)    
{
    notImplemented();
}

#if USE(PROTECTION_SPACE_AUTH_CALLBACK)
bool WebFrameLoaderClient::canAuthenticateAgainstProtectionSpace(DocumentLoader*, unsigned long, const ProtectionSpace& protectionSpace)
{
    // FIXME: Authentication is a per-resource concept, but we don't do per-resource handling in the UIProcess at the API level quite yet.
    // Once we do, we might need to make sure authentication fits with our solution.
    
    WebPage* webPage = m_frame->page();
    if (!webPage)
        return false;
        
    bool canAuthenticate;
    if (!webPage->sendSync(Messages::WebPageProxy::CanAuthenticateAgainstProtectionSpaceInFrame(m_frame->frameID(), protectionSpace), Messages::WebPageProxy::CanAuthenticateAgainstProtectionSpaceInFrame::Reply(canAuthenticate)))
        return false;
    
    return canAuthenticate;
}
#endif

void WebFrameLoaderClient::dispatchDidReceiveResponse(DocumentLoader*, unsigned long identifier, const ResourceResponse& response)
{
    WebPage* webPage = m_frame->page();
    if (!webPage)
        return;

    webPage->injectedBundleResourceLoadClient().didReceiveResponseForResource(webPage, m_frame, identifier, response);
}

void WebFrameLoaderClient::dispatchDidReceiveContentLength(DocumentLoader*, unsigned long identifier, int dataLength)
{
    WebPage* webPage = m_frame->page();
    if (!webPage)
        return;

    webPage->injectedBundleResourceLoadClient().didReceiveContentLengthForResource(webPage, m_frame, identifier, dataLength);
}

void WebFrameLoaderClient::dispatchDidFinishLoading(DocumentLoader*, unsigned long identifier)
{
    WebPage* webPage = m_frame->page();
    if (!webPage)
        return;

    webPage->injectedBundleResourceLoadClient().didFinishLoadForResource(webPage, m_frame, identifier);
}

void WebFrameLoaderClient::dispatchDidFailLoading(DocumentLoader*, unsigned long identifier, const ResourceError& error)
{
    WebPage* webPage = m_frame->page();
    if (!webPage)
        return;

    webPage->injectedBundleResourceLoadClient().didFailLoadForResource(webPage, m_frame, identifier, error);
}

bool WebFrameLoaderClient::dispatchDidLoadResourceFromMemoryCache(DocumentLoader*, const ResourceRequest&, const ResourceResponse&, int /*length*/)
{
    notImplemented();
    return false;
}

void WebFrameLoaderClient::dispatchDidHandleOnloadEvents()
{
    WebPage* webPage = m_frame->page();
    if (!webPage)
        return;

    // Notify the bundle client.
    webPage->injectedBundleLoaderClient().didHandleOnloadEventsForFrame(webPage, m_frame);
}

void WebFrameLoaderClient::dispatchDidReceiveServerRedirectForProvisionalLoad()
{
    WebPage* webPage = m_frame->page();
    if (!webPage)
        return;

    DocumentLoader* provisionalLoader = m_frame->coreFrame()->loader()->provisionalDocumentLoader();
    const String& url = provisionalLoader->url().string();
    RefPtr<APIObject> userData;

    // Notify the bundle client.
    webPage->injectedBundleLoaderClient().didReceiveServerRedirectForProvisionalLoadForFrame(webPage, m_frame, userData);

    // Notify the UIProcess.
    webPage->send(Messages::WebPageProxy::DidReceiveServerRedirectForProvisionalLoadForFrame(m_frame->frameID(), url, InjectedBundleUserMessageEncoder(userData.get())));
}

void WebFrameLoaderClient::dispatchDidCancelClientRedirect()
{
    WebPage* webPage = m_frame->page();
    if (!webPage)
        return;

    // Notify the bundle client.
    webPage->injectedBundleLoaderClient().didCancelClientRedirectForFrame(webPage, m_frame);
}

void WebFrameLoaderClient::dispatchWillPerformClientRedirect(const KURL& url, double interval, double fireDate)
{
    WebPage* webPage = m_frame->page();
    if (!webPage)
        return;

    // Notify the bundle client.
    webPage->injectedBundleLoaderClient().willPerformClientRedirectForFrame(webPage, m_frame, url.string(), interval, fireDate);
}

void WebFrameLoaderClient::dispatchDidChangeLocationWithinPage()
{
    WebPage* webPage = m_frame->page();
    if (!webPage)
        return;

    RefPtr<APIObject> userData;

    // Notify the bundle client.
    webPage->injectedBundleLoaderClient().didSameDocumentNavigationForFrame(webPage, m_frame, SameDocumentNavigationAnchorNavigation, userData);

    // Notify the UIProcess.
    webPage->send(Messages::WebPageProxy::DidSameDocumentNavigationForFrame(m_frame->frameID(), SameDocumentNavigationAnchorNavigation, m_frame->coreFrame()->document()->url().string(), InjectedBundleUserMessageEncoder(userData.get())));
}

void WebFrameLoaderClient::dispatchDidPushStateWithinPage()
{
    WebPage* webPage = m_frame->page();
    if (!webPage)
        return;

    RefPtr<APIObject> userData;

    // Notify the bundle client.
    webPage->injectedBundleLoaderClient().didSameDocumentNavigationForFrame(webPage, m_frame, SameDocumentNavigationSessionStatePush, userData);

    // Notify the UIProcess.
    webPage->send(Messages::WebPageProxy::DidSameDocumentNavigationForFrame(m_frame->frameID(), SameDocumentNavigationSessionStatePush, m_frame->coreFrame()->document()->url().string(), InjectedBundleUserMessageEncoder(userData.get())));
}

void WebFrameLoaderClient::dispatchDidReplaceStateWithinPage()
{
    WebPage* webPage = m_frame->page();
    if (!webPage)
        return;

    RefPtr<APIObject> userData;

    // Notify the bundle client.
    webPage->injectedBundleLoaderClient().didSameDocumentNavigationForFrame(webPage, m_frame, SameDocumentNavigationSessionStateReplace, userData);

    // Notify the UIProcess.
    webPage->send(Messages::WebPageProxy::DidSameDocumentNavigationForFrame(m_frame->frameID(), SameDocumentNavigationSessionStateReplace, m_frame->coreFrame()->document()->url().string(), InjectedBundleUserMessageEncoder(userData.get())));
}

void WebFrameLoaderClient::dispatchDidPopStateWithinPage()
{
    WebPage* webPage = m_frame->page();
    if (!webPage)
        return;

    RefPtr<APIObject> userData;

    // Notify the bundle client.
    webPage->injectedBundleLoaderClient().didSameDocumentNavigationForFrame(webPage, m_frame, SameDocumentNavigationSessionStatePop, userData);

    // Notify the UIProcess.
    webPage->send(Messages::WebPageProxy::DidSameDocumentNavigationForFrame(m_frame->frameID(), SameDocumentNavigationSessionStatePop, m_frame->coreFrame()->document()->url().string(), InjectedBundleUserMessageEncoder(userData.get())));
}

void WebFrameLoaderClient::dispatchWillClose()
{
    notImplemented();
}

void WebFrameLoaderClient::dispatchDidReceiveIcon()
{
    WebProcess::shared().parentProcessConnection()->send(Messages::WebIconDatabase::DidReceiveIconForPageURL(m_frame->url()), 0);
}

void WebFrameLoaderClient::dispatchDidStartProvisionalLoad()
{
    WebPage* webPage = m_frame->page();
    if (!webPage)
        return;

#if ENABLE(FULLSCREEN_API)
    Element* documentElement = m_frame->coreFrame()->document()->documentElement();
    if (documentElement && documentElement->containsFullScreenElement())
        webPage->fullScreenManager()->exitFullScreenForElement(webPage->fullScreenManager()->element());
#endif

    webPage->findController().hideFindUI();
    webPage->sandboxExtensionTracker().didStartProvisionalLoad(m_frame);

    DocumentLoader* provisionalLoader = m_frame->coreFrame()->loader()->provisionalDocumentLoader();
    const String& url = provisionalLoader->url().string();
    RefPtr<APIObject> userData;

    // Notify the bundle client.
    webPage->injectedBundleLoaderClient().didStartProvisionalLoadForFrame(webPage, m_frame, userData);

    String unreachableURL = provisionalLoader->unreachableURL().string();

    // Notify the UIProcess.
    webPage->send(Messages::WebPageProxy::DidStartProvisionalLoadForFrame(m_frame->frameID(), url, unreachableURL, InjectedBundleUserMessageEncoder(userData.get())));
}

void WebFrameLoaderClient::dispatchDidReceiveTitle(const StringWithDirection& title)
{
    WebPage* webPage = m_frame->page();
    if (!webPage)
        return;

    RefPtr<APIObject> userData;

    // Notify the bundle client.
    // FIXME: use direction of title.
    webPage->injectedBundleLoaderClient().didReceiveTitleForFrame(webPage, title.string(), m_frame, userData);

    // Notify the UIProcess.
    webPage->send(Messages::WebPageProxy::DidReceiveTitleForFrame(m_frame->frameID(), title.string(), InjectedBundleUserMessageEncoder(userData.get())));
}

void WebFrameLoaderClient::dispatchDidChangeIcons(WebCore::IconType)
{
    notImplemented();
}

void WebFrameLoaderClient::dispatchDidCommitLoad()
{
    WebPage* webPage = m_frame->page();
    if (!webPage)
        return;

    const ResourceResponse& response = m_frame->coreFrame()->loader()->documentLoader()->response();
    RefPtr<APIObject> userData;

    // Notify the bundle client.
    webPage->injectedBundleLoaderClient().didCommitLoadForFrame(webPage, m_frame, userData);

    webPage->sandboxExtensionTracker().didCommitProvisionalLoad(m_frame);

    // Notify the UIProcess.

    webPage->send(Messages::WebPageProxy::DidCommitLoadForFrame(m_frame->frameID(), response.mimeType(), m_frame->coreFrame()->loader()->loadType(), PlatformCertificateInfo(response), InjectedBundleUserMessageEncoder(userData.get())));

    webPage->didCommitLoad(m_frame);
}

void WebFrameLoaderClient::dispatchDidFailProvisionalLoad(const ResourceError& error)
{
    WebPage* webPage = m_frame->page();
    if (!webPage)
        return;

    RefPtr<APIObject> userData;

    // Notify the bundle client.
    webPage->injectedBundleLoaderClient().didFailProvisionalLoadWithErrorForFrame(webPage, m_frame, error, userData);

    webPage->sandboxExtensionTracker().didFailProvisionalLoad(m_frame);

    // Notify the UIProcess.
    webPage->send(Messages::WebPageProxy::DidFailProvisionalLoadForFrame(m_frame->frameID(), error, InjectedBundleUserMessageEncoder(userData.get())));
    
    // If we have a load listener, notify it.
    if (WebFrame::LoadListener* loadListener = m_frame->loadListener())
        loadListener->didFailLoad(m_frame, error.isCancellation());
}

void WebFrameLoaderClient::dispatchDidFailLoad(const ResourceError& error)
{
    WebPage* webPage = m_frame->page();
    if (!webPage)
        return;

    RefPtr<APIObject> userData;

    // Notify the bundle client.
    webPage->injectedBundleLoaderClient().didFailLoadWithErrorForFrame(webPage, m_frame, error, userData);

    // Notify the UIProcess.
    webPage->send(Messages::WebPageProxy::DidFailLoadForFrame(m_frame->frameID(), error, InjectedBundleUserMessageEncoder(userData.get())));

    // If we have a load listener, notify it.
    if (WebFrame::LoadListener* loadListener = m_frame->loadListener())
        loadListener->didFailLoad(m_frame, error.isCancellation());
}

void WebFrameLoaderClient::dispatchDidFinishDocumentLoad()
{
    WebPage* webPage = m_frame->page();
    if (!webPage)
        return;

    RefPtr<APIObject> userData;

    // Notify the bundle client.
    webPage->injectedBundleLoaderClient().didFinishDocumentLoadForFrame(webPage, m_frame, userData);

    // Notify the UIProcess.
    webPage->send(Messages::WebPageProxy::DidFinishDocumentLoadForFrame(m_frame->frameID(), InjectedBundleUserMessageEncoder(userData.get())));
}

void WebFrameLoaderClient::dispatchDidFinishLoad()
{
    WebPage* webPage = m_frame->page();
    if (!webPage)
        return;

    RefPtr<APIObject> userData;

    // Notify the bundle client.
    webPage->injectedBundleLoaderClient().didFinishLoadForFrame(webPage, m_frame, userData);

    // Notify the UIProcess.
    webPage->send(Messages::WebPageProxy::DidFinishLoadForFrame(m_frame->frameID(), InjectedBundleUserMessageEncoder(userData.get())));

    // If we have a load listener, notify it.
    if (WebFrame::LoadListener* loadListener = m_frame->loadListener())
        loadListener->didFinishLoad(m_frame);

    webPage->didFinishLoad(m_frame);
}

void WebFrameLoaderClient::forcePageTransitionIfNeeded()
{
    if (m_didCompletePageTransitionAlready)
        return;

    WebPage* webPage = m_frame->page();
    if (!webPage)
        return;

    webPage->didCompletePageTransition();
    m_didCompletePageTransitionAlready = true;
}

void WebFrameLoaderClient::dispatchDidLayout(LayoutMilestones milestones)
{
    WebPage* webPage = m_frame->page();
    if (!webPage)
        return;

    RefPtr<APIObject> userData;

    webPage->injectedBundleLoaderClient().didLayout(webPage, milestones, userData);
    webPage->send(Messages::WebPageProxy::DidLayout(milestones, InjectedBundleUserMessageEncoder(userData.get())));

    if (milestones & DidFirstLayout) {
        // FIXME: We should consider removing the old didFirstLayout API since this is doing double duty with the
        // new didLayout API.
        webPage->injectedBundleLoaderClient().didFirstLayoutForFrame(webPage, m_frame, userData);
        webPage->send(Messages::WebPageProxy::DidFirstLayoutForFrame(m_frame->frameID(), InjectedBundleUserMessageEncoder(userData.get())));

        if (m_frame == m_frame->page()->mainWebFrame()) {
            if (!webPage->corePage()->settings()->suppressesIncrementalRendering() && !m_didCompletePageTransitionAlready) {
                webPage->didCompletePageTransition();
                m_didCompletePageTransitionAlready = true;
            }
        }
    
#if USE(TILED_BACKING_STORE)
        // Make sure viewport properties are dispatched on the main frame by the time the first layout happens.
        ASSERT(!webPage->useFixedLayout() || m_frame != m_frame->page()->mainWebFrame() || m_frame->coreFrame()->document()->didDispatchViewportPropertiesChanged());
#endif
    }

    if (milestones & DidFirstVisuallyNonEmptyLayout) {
        // FIXME: We should consider removing the old didFirstVisuallyNonEmptyLayoutForFrame API since this is doing
        // double duty with the new didLayout API.
        webPage->injectedBundleLoaderClient().didFirstVisuallyNonEmptyLayoutForFrame(webPage, m_frame, userData);
        webPage->send(Messages::WebPageProxy::DidFirstVisuallyNonEmptyLayoutForFrame(m_frame->frameID(), InjectedBundleUserMessageEncoder(userData.get())));
    }

    if (milestones & DidHitRelevantRepaintedObjectsAreaThreshold) {
        // FIXME: This can go away when we remove didNewFirstVisuallyNonEmptyLayout.
        webPage->injectedBundleLoaderClient().didNewFirstVisuallyNonEmptyLayout(webPage, userData);
        webPage->send(Messages::WebPageProxy::DidNewFirstVisuallyNonEmptyLayout(InjectedBundleUserMessageEncoder(userData.get())));
    }
}

void WebFrameLoaderClient::dispatchDidLayout()
{
    WebPage* webPage = m_frame->page();
    if (!webPage)
        return;

    // Notify the bundle client.
    webPage->injectedBundleLoaderClient().didLayoutForFrame(webPage, m_frame);

    webPage->recomputeShortCircuitHorizontalWheelEventsState();

    // NOTE: Unlike the other layout notifications, this does not notify the
    // the UIProcess for every call.

    if (m_frame == m_frame->page()->mainWebFrame()) {
        // FIXME: Remove at the soonest possible time.
        webPage->send(Messages::WebPageProxy::SetRenderTreeSize(webPage->renderTreeSize()));
        webPage->mainFrameDidLayout();
    }
}

Frame* WebFrameLoaderClient::dispatchCreatePage(const NavigationAction& navigationAction)
{
    WebPage* webPage = m_frame->page();
    if (!webPage)
        return 0;

    // Just call through to the chrome client.
    Page* newPage = webPage->corePage()->chrome().createWindow(m_frame->coreFrame(), FrameLoadRequest(m_frame->coreFrame()->document()->securityOrigin()), WindowFeatures(), navigationAction);
    if (!newPage)
        return 0;
    
    return newPage->mainFrame();
}

void WebFrameLoaderClient::dispatchShow()
{
    WebPage* webPage = m_frame->page();
    if (!webPage)
        return;

    webPage->show();
}

void WebFrameLoaderClient::dispatchDecidePolicyForResponse(FramePolicyFunction function, const ResourceResponse& response, const ResourceRequest& request)
{
    WebPage* webPage = m_frame->page();
    if (!webPage)
        return;

    if (!request.url().string()) {
        (m_frame->coreFrame()->loader()->policyChecker()->*function)(PolicyUse);
        return;
    }

    RefPtr<APIObject> userData;

    // Notify the bundle client.
    WKBundlePagePolicyAction policy = webPage->injectedBundlePolicyClient().decidePolicyForResponse(webPage, m_frame, response, request, userData);
    if (policy == WKBundlePagePolicyActionUse) {
        (m_frame->coreFrame()->loader()->policyChecker()->*function)(PolicyUse);
        return;
    }

    uint64_t listenerID = m_frame->setUpPolicyListener(function);
    bool receivedPolicyAction;
    uint64_t policyAction;
    uint64_t downloadID;

    // Notify the UIProcess.
    if (!webPage->sendSync(Messages::WebPageProxy::DecidePolicyForResponseSync(m_frame->frameID(), response, request, listenerID, InjectedBundleUserMessageEncoder(userData.get())), Messages::WebPageProxy::DecidePolicyForResponseSync::Reply(receivedPolicyAction, policyAction, downloadID)))
        return;

    // We call this synchronously because CFNetwork can only convert a loading connection to a download from its didReceiveResponse callback.
    if (receivedPolicyAction)
        m_frame->didReceivePolicyDecision(listenerID, static_cast<PolicyAction>(policyAction), downloadID);
}

void WebFrameLoaderClient::dispatchDecidePolicyForNewWindowAction(FramePolicyFunction function, const NavigationAction& navigationAction, const ResourceRequest& request, PassRefPtr<FormState> formState, const String& frameName)
{
    WebPage* webPage = m_frame->page();
    if (!webPage)
        return;

    RefPtr<APIObject> userData;

    RefPtr<InjectedBundleNavigationAction> action = InjectedBundleNavigationAction::create(m_frame, navigationAction, formState);

    // Notify the bundle client.
    WKBundlePagePolicyAction policy = webPage->injectedBundlePolicyClient().decidePolicyForNewWindowAction(webPage, m_frame, action.get(), request, frameName, userData);
    if (policy == WKBundlePagePolicyActionUse) {
        (m_frame->coreFrame()->loader()->policyChecker()->*function)(PolicyUse);
        return;
    }


    uint64_t listenerID = m_frame->setUpPolicyListener(function);

    // Notify the UIProcess.
    webPage->send(Messages::WebPageProxy::DecidePolicyForNewWindowAction(m_frame->frameID(), action->navigationType(), action->modifiers(), action->mouseButton(), request, frameName, listenerID, InjectedBundleUserMessageEncoder(userData.get())));
}

void WebFrameLoaderClient::dispatchDecidePolicyForNavigationAction(FramePolicyFunction function, const NavigationAction& navigationAction, const ResourceRequest& request, PassRefPtr<FormState> formState)
{
    WebPage* webPage = m_frame->page();
    if (!webPage)
        return;

    // Always ignore requests with empty URLs. 
    if (request.isEmpty()) { 
        (m_frame->coreFrame()->loader()->policyChecker()->*function)(PolicyIgnore); 
        return; 
    }

    RefPtr<APIObject> userData;

    RefPtr<InjectedBundleNavigationAction> action = InjectedBundleNavigationAction::create(m_frame, navigationAction, formState);

    // Notify the bundle client.
    WKBundlePagePolicyAction policy = webPage->injectedBundlePolicyClient().decidePolicyForNavigationAction(webPage, m_frame, action.get(), request, userData);
    if (policy == WKBundlePagePolicyActionUse) {
        (m_frame->coreFrame()->loader()->policyChecker()->*function)(PolicyUse);
        return;
    }
    
    uint64_t listenerID = m_frame->setUpPolicyListener(function);
    bool receivedPolicyAction;
    uint64_t policyAction;
    uint64_t downloadID;

    // Notify the UIProcess.
    if (!webPage->sendSync(Messages::WebPageProxy::DecidePolicyForNavigationAction(m_frame->frameID(), action->navigationType(), action->modifiers(), action->mouseButton(), request, listenerID, InjectedBundleUserMessageEncoder(userData.get())), Messages::WebPageProxy::DecidePolicyForNavigationAction::Reply(receivedPolicyAction, policyAction, downloadID)))
        return;

    // We call this synchronously because WebCore cannot gracefully handle a frame load without a synchronous navigation policy reply.
    if (receivedPolicyAction)
        m_frame->didReceivePolicyDecision(listenerID, static_cast<PolicyAction>(policyAction), downloadID);
}

void WebFrameLoaderClient::cancelPolicyCheck()
{
    m_frame->invalidatePolicyListener();
}

void WebFrameLoaderClient::dispatchUnableToImplementPolicy(const ResourceError& error)
{
    WebPage* webPage = m_frame->page();
    if (!webPage)
        return;

    RefPtr<APIObject> userData;

    // Notify the bundle client.
    webPage->injectedBundlePolicyClient().unableToImplementPolicy(webPage, m_frame, error, userData);

    // Notify the UIProcess.
    webPage->send(Messages::WebPageProxy::UnableToImplementPolicy(m_frame->frameID(), error, InjectedBundleUserMessageEncoder(userData.get())));
}

void WebFrameLoaderClient::dispatchWillSendSubmitEvent(PassRefPtr<FormState> prpFormState)
{
    WebPage* webPage = m_frame->page();
    if (!webPage)
        return;

    RefPtr<FormState> formState = prpFormState;
    HTMLFormElement* form = formState->form();

    WebFrameLoaderClient* webFrameLoaderClient = toWebFrameLoaderClient(formState->sourceDocument()->frame()->loader()->client());
    WebFrame* sourceFrame = webFrameLoaderClient ? webFrameLoaderClient->webFrame() : 0;
    ASSERT(sourceFrame);

    webPage->injectedBundleFormClient().willSendSubmitEvent(webPage, form, m_frame, sourceFrame, formState->textFieldValues());
}

void WebFrameLoaderClient::dispatchWillSubmitForm(FramePolicyFunction function, PassRefPtr<FormState> prpFormState)
{
    WebPage* webPage = m_frame->page();
    if (!webPage)
        return;

    // FIXME: Pass more of the form state.
    RefPtr<FormState> formState = prpFormState;
    
    HTMLFormElement* form = formState->form();

    WebFrameLoaderClient* webFrameLoaderClient = toWebFrameLoaderClient(formState->sourceDocument()->frame()->loader()->client());
    WebFrame* sourceFrame = webFrameLoaderClient ? webFrameLoaderClient->webFrame() : 0;
    ASSERT(sourceFrame);

    const Vector<std::pair<String, String> >& values = formState->textFieldValues();

    RefPtr<APIObject> userData;
    webPage->injectedBundleFormClient().willSubmitForm(webPage, form, m_frame, sourceFrame, values, userData);


    uint64_t listenerID = m_frame->setUpPolicyListener(function);

    webPage->send(Messages::WebPageProxy::WillSubmitForm(m_frame->frameID(), sourceFrame->frameID(), values, listenerID, InjectedBundleUserMessageEncoder(userData.get())));
}

void WebFrameLoaderClient::revertToProvisionalState(DocumentLoader*)
{
    notImplemented();
}

void WebFrameLoaderClient::setMainDocumentError(DocumentLoader*, const ResourceError& error)
{
    if (!m_pluginView)
        return;
    
    m_pluginView->manualLoadDidFail(error);
    m_pluginView = 0;
    m_hasSentResponseToPluginView = false;
}

void WebFrameLoaderClient::willChangeEstimatedProgress()
{
    notImplemented();
}

void WebFrameLoaderClient::didChangeEstimatedProgress()
{
    notImplemented();
}

void WebFrameLoaderClient::postProgressStartedNotification()
{
    if (WebPage* webPage = m_frame->page()) {
        if (m_frame->isMainFrame())
            webPage->send(Messages::WebPageProxy::DidStartProgress());
    }
}

void WebFrameLoaderClient::postProgressEstimateChangedNotification()
{
    if (WebPage* webPage = m_frame->page()) {
        if (m_frame->isMainFrame()) {
            double progress = webPage->corePage()->progress()->estimatedProgress();
            webPage->send(Messages::WebPageProxy::DidChangeProgress(progress));
        }
    }
}

void WebFrameLoaderClient::postProgressFinishedNotification()
{
    if (WebPage* webPage = m_frame->page()) {
        if (m_frame->isMainFrame()) {
            // Notify the bundle client.
            webPage->injectedBundleLoaderClient().didFinishProgress(webPage);

            webPage->send(Messages::WebPageProxy::DidFinishProgress());
        }
    }
}

void WebFrameLoaderClient::setMainFrameDocumentReady(bool)
{
    notImplemented();
}

void WebFrameLoaderClient::startDownload(const ResourceRequest& request, const String& /* suggestedName */)
{
    m_frame->startDownload(request);
}

void WebFrameLoaderClient::willChangeTitle(DocumentLoader*)
{
    notImplemented();
}

void WebFrameLoaderClient::didChangeTitle(DocumentLoader*)
{
    notImplemented();
}

void WebFrameLoaderClient::committedLoad(DocumentLoader* loader, const char* data, int length)
{
    if (!m_pluginView)
        loader->commitData(data, length);

    // If the document is a stand-alone media document, now is the right time to cancel the WebKit load.
    // FIXME: This code should be shared across all ports. <http://webkit.org/b/48762>.
    if (m_frame->coreFrame()->document()->isMediaDocument())
        loader->cancelMainResourceLoad(pluginWillHandleLoadError(loader->response()));

    // Calling commitData did not create the plug-in view.
    if (!m_pluginView)
        return;

    if (!m_hasSentResponseToPluginView) {
        m_pluginView->manualLoadDidReceiveResponse(loader->response());
        // manualLoadDidReceiveResponse sets up a new stream to the plug-in. on a full-page plug-in, a failure in
        // setting up this stream can cause the main document load to be cancelled, setting m_pluginView
        // to null
        if (!m_pluginView)
            return;
        m_hasSentResponseToPluginView = true;
    }
    m_pluginView->manualLoadDidReceiveData(data, length);
}

void WebFrameLoaderClient::finishedLoading(DocumentLoader* loader)
{
    if (!m_pluginView)
        return;

    // If we just received an empty response without any data, we won't have sent a response to the plug-in view.
    // Make sure to do this before calling manualLoadDidFinishLoading.
    if (!m_hasSentResponseToPluginView) {
        m_pluginView->manualLoadDidReceiveResponse(loader->response());

        // Protect against the above call nulling out the plug-in (by trying to cancel the load for example).
        if (!m_pluginView)
            return;
    }

    m_pluginView->manualLoadDidFinishLoading();
    m_pluginView = 0;
    m_hasSentResponseToPluginView = false;
}

void WebFrameLoaderClient::updateGlobalHistory()
{
    WebPage* webPage = m_frame->page();
    if (!webPage || !webPage->pageGroup()->isVisibleToHistoryClient())
        return;

    DocumentLoader* loader = m_frame->coreFrame()->loader()->documentLoader();

    WebNavigationDataStore data;
    data.url = loader->url().string();
    // FIXME: use direction of title.
    data.title = loader->title().string();
    data.originalRequest = loader->originalRequestCopy();

    WebProcess::shared().parentProcessConnection()->send(Messages::WebProcessProxy::DidNavigateWithNavigationData(webPage->pageID(), data, m_frame->frameID()), 0);
}

void WebFrameLoaderClient::updateGlobalHistoryRedirectLinks()
{
    WebPage* webPage = m_frame->page();
    if (!webPage || !webPage->pageGroup()->isVisibleToHistoryClient())
        return;

    DocumentLoader* loader = m_frame->coreFrame()->loader()->documentLoader();
    ASSERT(loader->unreachableURL().isEmpty());

    // Client redirect
    if (!loader->clientRedirectSourceForHistory().isNull()) {
        WebProcess::shared().parentProcessConnection()->send(Messages::WebProcessProxy::DidPerformClientRedirect(webPage->pageID(),
            loader->clientRedirectSourceForHistory(), loader->clientRedirectDestinationForHistory(), m_frame->frameID()), 0);
    }

    // Server redirect
    if (!loader->serverRedirectSourceForHistory().isNull()) {
        WebProcess::shared().parentProcessConnection()->send(Messages::WebProcessProxy::DidPerformServerRedirect(webPage->pageID(),
            loader->serverRedirectSourceForHistory(), loader->serverRedirectDestinationForHistory(), m_frame->frameID()), 0);
    }
}

bool WebFrameLoaderClient::shouldGoToHistoryItem(HistoryItem* item) const
{
    WebPage* webPage = m_frame->page();
    if (!webPage)
        return false;
    
    uint64_t itemID = WebBackForwardListProxy::idForItem(item);
    if (!itemID) {
        // We should never be considering navigating to an item that is not actually in the back/forward list.
        ASSERT_NOT_REACHED();
        return false;
    }

    RefPtr<InjectedBundleBackForwardListItem> bundleItem = InjectedBundleBackForwardListItem::create(item);
    RefPtr<APIObject> userData;

    // Ask the bundle client first
    bool shouldGoToBackForwardListItem = webPage->injectedBundleLoaderClient().shouldGoToBackForwardListItem(webPage, bundleItem.get(), userData);
    if (!shouldGoToBackForwardListItem)
        return false;
    
    if (webPage->willGoToBackForwardItemCallbackEnabled()) {
        webPage->send(Messages::WebPageProxy::WillGoToBackForwardListItem(itemID, InjectedBundleUserMessageEncoder(userData.get())));
        return true;
    }
    
    if (!webPage->sendSync(Messages::WebPageProxy::ShouldGoToBackForwardListItem(itemID), Messages::WebPageProxy::ShouldGoToBackForwardListItem::Reply(shouldGoToBackForwardListItem)))
        return false;
    
    return shouldGoToBackForwardListItem;
}

bool WebFrameLoaderClient::shouldStopLoadingForHistoryItem(HistoryItem*) const
{
    return true;
}

void WebFrameLoaderClient::didDisplayInsecureContent()
{
    WebPage* webPage = m_frame->page();
    if (!webPage)
        return;

    RefPtr<APIObject> userData;

    webPage->injectedBundleLoaderClient().didDisplayInsecureContentForFrame(webPage, m_frame, userData);

    webPage->send(Messages::WebPageProxy::DidDisplayInsecureContentForFrame(m_frame->frameID(), InjectedBundleUserMessageEncoder(userData.get())));
}

void WebFrameLoaderClient::didRunInsecureContent(SecurityOrigin*, const KURL&)
{
    WebPage* webPage = m_frame->page();
    if (!webPage)
        return;

    RefPtr<APIObject> userData;

    webPage->injectedBundleLoaderClient().didRunInsecureContentForFrame(webPage, m_frame, userData);

    webPage->send(Messages::WebPageProxy::DidRunInsecureContentForFrame(m_frame->frameID(), InjectedBundleUserMessageEncoder(userData.get())));
}

void WebFrameLoaderClient::didDetectXSS(const KURL&, bool)
{
    WebPage* webPage = m_frame->page();
    if (!webPage)
        return;

    RefPtr<APIObject> userData;

    webPage->injectedBundleLoaderClient().didDetectXSSForFrame(webPage, m_frame, userData);

    webPage->send(Messages::WebPageProxy::DidDetectXSSForFrame(m_frame->frameID(), InjectedBundleUserMessageEncoder(userData.get())));
}

ResourceError WebFrameLoaderClient::cancelledError(const ResourceRequest& request)
{
    return WebKit::cancelledError(request);
}

ResourceError WebFrameLoaderClient::blockedError(const ResourceRequest& request)
{
    return WebKit::blockedError(request);
}

ResourceError WebFrameLoaderClient::cannotShowURLError(const ResourceRequest& request)
{
    return WebKit::cannotShowURLError(request);
}

ResourceError WebFrameLoaderClient::interruptedForPolicyChangeError(const ResourceRequest& request)
{
    return WebKit::interruptedForPolicyChangeError(request);
}

ResourceError WebFrameLoaderClient::cannotShowMIMETypeError(const ResourceResponse& response)
{
    return WebKit::cannotShowMIMETypeError(response);
}

ResourceError WebFrameLoaderClient::fileDoesNotExistError(const ResourceResponse& response)
{
    return WebKit::fileDoesNotExistError(response);
}

ResourceError WebFrameLoaderClient::pluginWillHandleLoadError(const ResourceResponse& response)
{
    return WebKit::pluginWillHandleLoadError(response);
}

bool WebFrameLoaderClient::shouldFallBack(const ResourceError& error)
{
    DEFINE_STATIC_LOCAL(const ResourceError, cancelledError, (this->cancelledError(ResourceRequest())));
    DEFINE_STATIC_LOCAL(const ResourceError, pluginWillHandleLoadError, (this->pluginWillHandleLoadError(ResourceResponse())));

    if (error.errorCode() == cancelledError.errorCode() && error.domain() == cancelledError.domain())
        return false;

    if (error.errorCode() == pluginWillHandleLoadError.errorCode() && error.domain() == pluginWillHandleLoadError.domain())
        return false;

#if PLATFORM(QT)
    DEFINE_STATIC_LOCAL(const ResourceError, errorInterruptedForPolicyChange, (this->interruptedForPolicyChangeError(ResourceRequest())));

    if (error.errorCode() == errorInterruptedForPolicyChange.errorCode() && error.domain() == errorInterruptedForPolicyChange.domain())
        return false;
#endif

    return true;
}

bool WebFrameLoaderClient::canHandleRequest(const ResourceRequest&) const
{
    notImplemented();
    return true;
}

bool WebFrameLoaderClient::canShowMIMEType(const String& /*MIMEType*/) const
{
    notImplemented();
    return true;
}

bool WebFrameLoaderClient::canShowMIMETypeAsHTML(const String& /*MIMEType*/) const
{
    return true;
}

bool WebFrameLoaderClient::representationExistsForURLScheme(const String& /*URLScheme*/) const
{
    notImplemented();
    return false;
}

String WebFrameLoaderClient::generatedMIMETypeForURLScheme(const String& /*URLScheme*/) const
{
    notImplemented();
    return String();
}

void WebFrameLoaderClient::frameLoadCompleted()
{
    // Note: Can be called multiple times.
    WebPage* webPage = m_frame->page();
    if (!webPage)
        return;

    if (m_frame == m_frame->page()->mainWebFrame() && !m_didCompletePageTransitionAlready) {
        webPage->didCompletePageTransition();
        m_didCompletePageTransitionAlready = true;
    }
}

void WebFrameLoaderClient::saveViewStateToItem(HistoryItem*)
{
    notImplemented();
}

void WebFrameLoaderClient::restoreViewState()
{
    // Inform the UI process of the scale factor.
    double scaleFactor = m_frame->coreFrame()->loader()->history()->currentItem()->pageScaleFactor();

    // A scale factor of 0 means the history item has the default scale factor, thus we do not need to update it.
    if (scaleFactor)
        m_frame->page()->send(Messages::WebPageProxy::PageScaleFactorDidChange(scaleFactor));

    // FIXME: This should not be necessary. WebCore should be correctly invalidating
    // the view on restores from the back/forward cache.
    if (m_frame == m_frame->page()->mainWebFrame())
        m_frame->page()->drawingArea()->setNeedsDisplay();
}

void WebFrameLoaderClient::provisionalLoadStarted()
{
    WebPage* webPage = m_frame->page();
    if (!webPage)
        return;

    if (m_frame == m_frame->page()->mainWebFrame()) {
        webPage->didStartPageTransition();
        m_didCompletePageTransitionAlready = false;
    }
}

void WebFrameLoaderClient::didFinishLoad()
{
    // If we have a load listener, notify it.
    if (WebFrame::LoadListener* loadListener = m_frame->loadListener())
        loadListener->didFinishLoad(m_frame);
}

void WebFrameLoaderClient::prepareForDataSourceReplacement()
{
    notImplemented();
}

PassRefPtr<DocumentLoader> WebFrameLoaderClient::createDocumentLoader(const ResourceRequest& request, const SubstituteData& data)
{
    return DocumentLoader::create(request, data);
}

void WebFrameLoaderClient::setTitle(const StringWithDirection& title, const KURL& url)
{
    WebPage* webPage = m_frame->page();
    if (!webPage || !webPage->pageGroup()->isVisibleToHistoryClient())
        return;

    // FIXME: use direction of title.
    WebProcess::shared().parentProcessConnection()->send(Messages::WebProcessProxy::DidUpdateHistoryTitle(webPage->pageID(),
        title.string(), url.string(), m_frame->frameID()), 0);
}

String WebFrameLoaderClient::userAgent(const KURL&)
{
    WebPage* webPage = m_frame->page();
    if (!webPage)
        return String();

    return webPage->userAgent();
}

void WebFrameLoaderClient::savePlatformDataToCachedFrame(CachedFrame*)
{
}

void WebFrameLoaderClient::transitionToCommittedFromCachedFrame(CachedFrame*)
{
    m_frameCameFromPageCache = true;
}

void WebFrameLoaderClient::transitionToCommittedForNewPage()
{
    WebPage* webPage = m_frame->page();

    Color backgroundColor = webPage->drawsTransparentBackground() ? Color::transparent : Color::white;
    bool isMainFrame = webPage->mainWebFrame() == m_frame;
    bool isTransparent = !webPage->drawsBackground();
    bool shouldUseFixedLayout = isMainFrame && webPage->useFixedLayout();
    bool shouldDisableScrolling = isMainFrame && !webPage->mainFrameIsScrollable();
    bool shouldHideScrollbars = shouldUseFixedLayout || shouldDisableScrolling;
    IntRect currentFixedVisibleContentRect = m_frame->coreFrame()->view() ? m_frame->coreFrame()->view()->fixedVisibleContentRect() : IntRect();

    m_frameCameFromPageCache = false;

    m_frame->coreFrame()->createView(webPage->size(), backgroundColor, isTransparent,
        IntSize(), currentFixedVisibleContentRect, shouldUseFixedLayout,
        ScrollbarAuto, /* lock */ shouldHideScrollbars, ScrollbarAuto, /* lock */ shouldHideScrollbars);

    if (int minimumLayoutWidth = webPage->minimumLayoutSize().width()) {
        int minimumLayoutHeight = std::max(webPage->minimumLayoutSize().height(), 1);
        int maximumSize = std::numeric_limits<int>::max();
        m_frame->coreFrame()->view()->enableAutoSizeMode(true, IntSize(minimumLayoutWidth, minimumLayoutHeight), IntSize(maximumSize, maximumSize));
    }

    m_frame->coreFrame()->view()->setProhibitsScrolling(shouldDisableScrolling);
    m_frame->coreFrame()->view()->setVisualUpdatesAllowedByClient(!webPage->shouldExtendIncrementalRenderingSuppression());
    
    if (webPage->scrollPinningBehavior() != DoNotPin)
        m_frame->coreFrame()->view()->setScrollPinningBehavior(webPage->scrollPinningBehavior());

#if USE(TILED_BACKING_STORE)
    if (shouldUseFixedLayout) {
        m_frame->coreFrame()->view()->setDelegatesScrolling(shouldUseFixedLayout);
        m_frame->coreFrame()->view()->setPaintsEntireContents(shouldUseFixedLayout);
        return;
    }
#endif
}

void WebFrameLoaderClient::didSaveToPageCache()
{
    WebPage* webPage = m_frame->page();
    if (!webPage)
        return;

    webPage->send(Messages::WebPageProxy::DidSaveToPageCache());
}

void WebFrameLoaderClient::didRestoreFromPageCache()
{
}

void WebFrameLoaderClient::dispatchDidBecomeFrameset(bool value)
{
    WebPage* webPage = m_frame->page();
    if (!webPage)
        return;

    webPage->send(Messages::WebPageProxy::FrameDidBecomeFrameSet(m_frame->frameID(), value));
}

void WebFrameLoaderClient::convertMainResourceLoadToDownload(DocumentLoader *documentLoader, const ResourceRequest& request, const ResourceResponse& response)
{
    m_frame->convertMainResourceLoadToDownload(documentLoader, request, response);
}

PassRefPtr<Frame> WebFrameLoaderClient::createFrame(const KURL& url, const String& name, HTMLFrameOwnerElement* ownerElement,
                                                    const String& referrer, bool /*allowsScrolling*/, int /*marginWidth*/, int /*marginHeight*/)
{
    WebPage* webPage = m_frame->page();

    RefPtr<WebFrame> subframe = WebFrame::createSubframe(webPage, name, ownerElement);

    Frame* coreSubframe = subframe->coreFrame();
    if (!coreSubframe)
        return 0;

    // The creation of the frame may have run arbitrary JavaScript that removed it from the page already.
    if (!coreSubframe->page())
        return 0;

    m_frame->coreFrame()->loader()->loadURLIntoChildFrame(url, referrer, coreSubframe);

    // The frame's onload handler may have removed it from the document.
    if (!subframe->coreFrame())
        return 0;
    ASSERT(subframe->coreFrame() == coreSubframe);
    if (!coreSubframe->tree()->parent())
        return 0;

    return coreSubframe;
}

PassRefPtr<Widget> WebFrameLoaderClient::createPlugin(const IntSize&, HTMLPlugInElement* pluginElement, const KURL& url, const Vector<String>& paramNames, const Vector<String>& paramValues, const String& mimeType, bool loadManually)
{
    ASSERT(paramNames.size() == paramValues.size());
    ASSERT(m_frame->page());

    Plugin::Parameters parameters;
    parameters.url = url;
    parameters.names = paramNames;
    parameters.values = paramValues;
    parameters.mimeType = mimeType;
    parameters.isFullFramePlugin = loadManually;
    parameters.shouldUseManualLoader = parameters.isFullFramePlugin && !m_frameCameFromPageCache;
#if PLATFORM(MAC)
    parameters.layerHostingMode = m_frame->page()->layerHostingMode();
#endif

#if PLUGIN_ARCHITECTURE(X11)
    // FIXME: This should really be X11-specific plug-in quirks.
    if (equalIgnoringCase(mimeType, "application/x-shockwave-flash")) {
        // Currently we don't support transparency and windowed mode.
        // Inject wmode=opaque to make Flash work in these conditions.
        size_t wmodeIndex = parameters.names.find("wmode");
        if (wmodeIndex == notFound) {
            parameters.names.append("wmode");
            parameters.values.append("opaque");
        } else if (equalIgnoringCase(parameters.values[wmodeIndex], "window"))
            parameters.values[wmodeIndex] = "opaque";
    } else if (equalIgnoringCase(mimeType, "application/x-webkit-test-netscape")) {
        parameters.names.append("windowedPlugin");
        parameters.values.append("false");
    }
#endif

#if ENABLE(NETSCAPE_PLUGIN_API)
    RefPtr<Plugin> plugin = m_frame->page()->createPlugin(m_frame, pluginElement, parameters, parameters.mimeType);
    if (!plugin)
        return 0;

    return PluginView::create(pluginElement, plugin.release(), parameters);
#else
    return 0;
#endif
}

void WebFrameLoaderClient::recreatePlugin(Widget* widget)
{
#if ENABLE(NETSCAPE_PLUGIN_API)
    ASSERT(widget && widget->isPluginViewBase());
    ASSERT(m_frame->page());

    PluginView* pluginView = static_cast<PluginView*>(widget);
    String newMIMEType;
    RefPtr<Plugin> plugin = m_frame->page()->createPlugin(m_frame, pluginView->pluginElement(), pluginView->initialParameters(), newMIMEType);
    pluginView->recreateAndInitialize(plugin.release());
#endif
}

void WebFrameLoaderClient::redirectDataToPlugin(Widget* pluginWidget)
{
    if (pluginWidget)
        m_pluginView = static_cast<PluginView*>(pluginWidget);
}

PassRefPtr<Widget> WebFrameLoaderClient::createJavaAppletWidget(const IntSize& pluginSize, HTMLAppletElement* appletElement, const KURL&, const Vector<String>& paramNames, const Vector<String>& paramValues)
{
    RefPtr<Widget> plugin = createPlugin(pluginSize, appletElement, KURL(), paramNames, paramValues, appletElement->serviceType(), false);
    if (!plugin) {
        if (WebPage* webPage = m_frame->page()) {
            String frameURLString = m_frame->coreFrame()->loader()->documentLoader()->responseURL().string();
            String pageURLString = webPage->corePage()->mainFrame()->loader()->documentLoader()->responseURL().string();
            webPage->send(Messages::WebPageProxy::DidFailToInitializePlugin(appletElement->serviceType(), frameURLString, pageURLString));
        }
    }
    return plugin.release();
}

#if ENABLE(PLUGIN_PROXY_FOR_VIDEO)
PassRefPtr<Widget> WebFrameLoaderClient::createMediaPlayerProxyPlugin(const IntSize&, HTMLMediaElement*, const KURL&, const Vector<String>&, const Vector<String>&, const String&)
{
    notImplemented();
    return 0;
}

void WebFrameLoaderClient::hideMediaPlayerProxyPlugin(Widget*)
{
    notImplemented();
}

void WebFrameLoaderClient::showMediaPlayerProxyPlugin(Widget*)
{
    notImplemented();
}
#endif

static bool pluginSupportsExtension(PluginData* pluginData, const String& extension)
{
    ASSERT(extension.lower() == extension);

    for (size_t i = 0; i < pluginData->mimes().size(); ++i) {
        const MimeClassInfo& mimeClassInfo = pluginData->mimes()[i];

        if (mimeClassInfo.extensions.contains(extension))
            return true;
    }
    return false;
}

ObjectContentType WebFrameLoaderClient::objectContentType(const KURL& url, const String& mimeTypeIn, bool shouldPreferPlugInsForImages)
{
    // FIXME: This should be merged with WebCore::FrameLoader::defaultObjectContentType when the plugin code
    // is consolidated.

    String mimeType = mimeTypeIn;
    if (mimeType.isEmpty()) {
        String extension = url.path().substring(url.path().reverseFind('.') + 1).lower();

        // Try to guess the MIME type from the extension.
        mimeType = MIMETypeRegistry::getMIMETypeForExtension(extension);

        if (mimeType.isEmpty()) {
            // Check if there's a plug-in around that can handle the extension.
            if (WebPage* webPage = m_frame->page()) {
                if (PluginData* pluginData = webPage->corePage()->pluginData()) {
                    if (pluginSupportsExtension(pluginData, extension))
                        return ObjectContentNetscapePlugin;
                }
            }
        }
    }

    if (mimeType.isEmpty())
        return ObjectContentFrame;

    bool plugInSupportsMIMEType = false;
    if (WebPage* webPage = m_frame->page()) {
        if (PluginData* pluginData = webPage->corePage()->pluginData()) {
            if (pluginData->supportsMimeType(mimeType, PluginData::AllPlugins) && webFrame()->coreFrame()->loader()->subframeLoader()->allowPlugins(NotAboutToInstantiatePlugin))
                plugInSupportsMIMEType = true;
            else if (pluginData->supportsMimeType(mimeType, PluginData::OnlyApplicationPlugins))
                plugInSupportsMIMEType = true;
        }
    }
    
    if (MIMETypeRegistry::isSupportedImageMIMEType(mimeType))
        return shouldPreferPlugInsForImages && plugInSupportsMIMEType ? ObjectContentNetscapePlugin : ObjectContentImage;

    if (plugInSupportsMIMEType)
        return ObjectContentNetscapePlugin;

    if (MIMETypeRegistry::isSupportedNonImageMIMEType(mimeType))
        return ObjectContentFrame;

    return ObjectContentNone;
}

String WebFrameLoaderClient::overrideMediaType() const
{
    notImplemented();
    return String();
}

void WebFrameLoaderClient::dispatchDidClearWindowObjectInWorld(DOMWrapperWorld* world)
{
    WebPage* webPage = m_frame->page();
    if (!webPage)
        return;

    webPage->injectedBundleLoaderClient().didClearWindowObjectForFrame(webPage, m_frame, world);

#if HAVE(ACCESSIBILITY) && (PLATFORM(GTK) || PLATFORM(EFL))
    // Ensure the accessibility hierarchy is updated.
    webPage->updateAccessibilityTree();
#endif
}


void WebFrameLoaderClient::dispatchGlobalObjectAvailable(DOMWrapperWorld* world)
{
    WebPage* webPage = m_frame->page();
    if (!webPage)
        return;
    
    webPage->injectedBundleLoaderClient().globalObjectIsAvailableForFrame(webPage, m_frame, world);
}

void WebFrameLoaderClient::dispatchWillDisconnectDOMWindowExtensionFromGlobalObject(WebCore::DOMWindowExtension* extension)
{
    WebPage* webPage = m_frame->page();
    if (!webPage)
        return;
        
    webPage->injectedBundleLoaderClient().willDisconnectDOMWindowExtensionFromGlobalObject(webPage, extension);
}

void WebFrameLoaderClient::dispatchDidReconnectDOMWindowExtensionToGlobalObject(WebCore::DOMWindowExtension* extension)
{
    WebPage* webPage = m_frame->page();
    if (!webPage)
        return;
        
    webPage->injectedBundleLoaderClient().didReconnectDOMWindowExtensionToGlobalObject(webPage, extension);
}

void WebFrameLoaderClient::dispatchWillDestroyGlobalObjectForDOMWindowExtension(WebCore::DOMWindowExtension* extension)
{
    WebPage* webPage = m_frame->page();
    if (!webPage)
        return;
        
    webPage->injectedBundleLoaderClient().willDestroyGlobalObjectForDOMWindowExtension(webPage, extension);
}

void WebFrameLoaderClient::documentElementAvailable()
{
    notImplemented();
}

void WebFrameLoaderClient::didPerformFirstNavigation() const
{
    notImplemented();
}

void WebFrameLoaderClient::registerForIconNotification(bool /*listen*/)
{
    notImplemented();
}

#if PLATFORM(MAC)
    
RemoteAXObjectRef WebFrameLoaderClient::accessibilityRemoteObject() 
{
    WebPage* webPage = m_frame->page();
    if (!webPage)
        return 0;
    
    return webPage->accessibilityRemoteObject();
}
    
NSCachedURLResponse* WebFrameLoaderClient::willCacheResponse(DocumentLoader*, unsigned long identifier, NSCachedURLResponse* response) const
{
    WebPage* webPage = m_frame->page();
    if (!webPage)
        return response;

    return webPage->injectedBundleResourceLoadClient().shouldCacheResponse(webPage, m_frame, identifier) ? response : nil;
}

#endif // PLATFORM(MAC)

bool WebFrameLoaderClient::shouldAlwaysUsePluginDocument(const String& /*mimeType*/) const
{
    notImplemented();
    return false;
}

void WebFrameLoaderClient::didChangeScrollOffset()
{
    WebPage* webPage = m_frame->page();
    if (!webPage)
        return;

    webPage->drawingArea()->didChangeScrollOffsetForAnyFrame();

    if (!m_frame->isMainFrame())
        return;

    // If this is called when tearing down a FrameView, the WebCore::Frame's
    // current FrameView will be null.
    if (!m_frame->coreFrame()->view())
        return;

    webPage->didChangeScrollOffsetForMainFrame();
}

bool WebFrameLoaderClient::allowScript(bool enabledPerSettings)
{
    if (!enabledPerSettings)
        return false;

    Frame* coreFrame = m_frame->coreFrame();

    if (coreFrame->document()->isPluginDocument()) {
        PluginDocument* pluginDocument = static_cast<PluginDocument*>(coreFrame->document());

        if (pluginDocument->pluginWidget() && pluginDocument->pluginWidget()->isPluginView()) {
            PluginView* pluginView = static_cast<PluginView*>(pluginDocument->pluginWidget());

            if (!pluginView->shouldAllowScripting())
                return false;
        }
    }

    return true;
}

bool WebFrameLoaderClient::shouldForceUniversalAccessFromLocalURL(const WebCore::KURL& url)
{
    WebPage* webPage = m_frame->page();
    if (!webPage)
        return false;

    return webPage->injectedBundleLoaderClient().shouldForceUniversalAccessFromLocalURL(webPage, url.string());
}

PassRefPtr<FrameNetworkingContext> WebFrameLoaderClient::createNetworkingContext()
{
    RefPtr<WebFrameNetworkingContext> context = WebFrameNetworkingContext::create(m_frame);
    return context.release();
}

} // namespace WebKit
