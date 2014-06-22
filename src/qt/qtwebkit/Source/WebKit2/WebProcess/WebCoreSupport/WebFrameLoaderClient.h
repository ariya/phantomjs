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

#ifndef WebFrameLoaderClient_h
#define WebFrameLoaderClient_h

#include <WebCore/FrameLoaderClient.h>

namespace WebKit {

class PluginView;
class WebFrame;
    
class WebFrameLoaderClient : public WebCore::FrameLoaderClient {
public:
    WebFrameLoaderClient(WebFrame*);
    ~WebFrameLoaderClient();

    WebFrame* webFrame() const { return m_frame; }

private:
    virtual void frameLoaderDestroyed() OVERRIDE;

    virtual bool hasHTMLView() const OVERRIDE { return true; }
    virtual bool hasWebView() const OVERRIDE;
    
    virtual void makeRepresentation(WebCore::DocumentLoader*) OVERRIDE;
    virtual void forceLayout() OVERRIDE;
    virtual void forceLayoutForNonHTML() OVERRIDE;
    
    virtual void setCopiesOnScroll() OVERRIDE;
    
    virtual void detachedFromParent2() OVERRIDE;
    virtual void detachedFromParent3() OVERRIDE;
    
    virtual void assignIdentifierToInitialRequest(unsigned long identifier, WebCore::DocumentLoader*, const WebCore::ResourceRequest&) OVERRIDE;
    
    virtual void dispatchWillSendRequest(WebCore::DocumentLoader*, unsigned long identifier, WebCore::ResourceRequest&, const WebCore::ResourceResponse& redirectResponse) OVERRIDE;
    virtual bool shouldUseCredentialStorage(WebCore::DocumentLoader*, unsigned long identifier) OVERRIDE;
    virtual void dispatchDidReceiveAuthenticationChallenge(WebCore::DocumentLoader*, unsigned long identifier, const WebCore::AuthenticationChallenge&) OVERRIDE;
    virtual void dispatchDidCancelAuthenticationChallenge(WebCore::DocumentLoader*, unsigned long identifier, const WebCore::AuthenticationChallenge&) OVERRIDE;
#if USE(PROTECTION_SPACE_AUTH_CALLBACK)
    virtual bool canAuthenticateAgainstProtectionSpace(WebCore::DocumentLoader*, unsigned long identifier, const WebCore::ProtectionSpace&) OVERRIDE;
#endif
    virtual void dispatchDidReceiveResponse(WebCore::DocumentLoader*, unsigned long identifier, const WebCore::ResourceResponse&) OVERRIDE;
    virtual void dispatchDidReceiveContentLength(WebCore::DocumentLoader*, unsigned long identifier, int dataLength) OVERRIDE;
    virtual void dispatchDidFinishLoading(WebCore::DocumentLoader*, unsigned long identifier) OVERRIDE;
    virtual void dispatchDidFailLoading(WebCore::DocumentLoader*, unsigned long identifier, const WebCore::ResourceError&) OVERRIDE;
    virtual bool dispatchDidLoadResourceFromMemoryCache(WebCore::DocumentLoader*, const WebCore::ResourceRequest&, const WebCore::ResourceResponse&, int length) OVERRIDE;
    
    virtual void dispatchDidHandleOnloadEvents() OVERRIDE;
    virtual void dispatchDidReceiveServerRedirectForProvisionalLoad() OVERRIDE;
    virtual void dispatchDidCancelClientRedirect() OVERRIDE;
    virtual void dispatchWillPerformClientRedirect(const WebCore::KURL&, double interval, double fireDate) OVERRIDE;
    virtual void dispatchDidChangeLocationWithinPage() OVERRIDE;
    virtual void dispatchDidPushStateWithinPage() OVERRIDE;
    virtual void dispatchDidReplaceStateWithinPage() OVERRIDE;
    virtual void dispatchDidPopStateWithinPage() OVERRIDE;
    virtual void dispatchWillClose() OVERRIDE;
    virtual void dispatchDidReceiveIcon() OVERRIDE;
    virtual void dispatchDidStartProvisionalLoad() OVERRIDE;
    virtual void dispatchDidReceiveTitle(const WebCore::StringWithDirection&) OVERRIDE;
    virtual void dispatchDidChangeIcons(WebCore::IconType) OVERRIDE;
    virtual void dispatchDidCommitLoad() OVERRIDE;
    virtual void dispatchDidFailProvisionalLoad(const WebCore::ResourceError&) OVERRIDE;
    virtual void dispatchDidFailLoad(const WebCore::ResourceError&) OVERRIDE;
    virtual void dispatchDidFinishDocumentLoad() OVERRIDE;
    virtual void dispatchDidFinishLoad() OVERRIDE;

    virtual void dispatchDidLayout(WebCore::LayoutMilestones) OVERRIDE;
    virtual void dispatchDidLayout() OVERRIDE;

    virtual WebCore::Frame* dispatchCreatePage(const WebCore::NavigationAction&) OVERRIDE;
    virtual void dispatchShow() OVERRIDE;
    
    virtual void dispatchDecidePolicyForResponse(WebCore::FramePolicyFunction, const WebCore::ResourceResponse&, const WebCore::ResourceRequest&) OVERRIDE;
    virtual void dispatchDecidePolicyForNewWindowAction(WebCore::FramePolicyFunction, const WebCore::NavigationAction&, const WebCore::ResourceRequest&, PassRefPtr<WebCore::FormState>, const String& frameName) OVERRIDE;
    virtual void dispatchDecidePolicyForNavigationAction(WebCore::FramePolicyFunction, const WebCore::NavigationAction&, const WebCore::ResourceRequest&, PassRefPtr<WebCore::FormState>) OVERRIDE;
    virtual void cancelPolicyCheck() OVERRIDE;
    
    virtual void dispatchUnableToImplementPolicy(const WebCore::ResourceError&) OVERRIDE;
    
    virtual void dispatchWillSendSubmitEvent(PassRefPtr<WebCore::FormState>) OVERRIDE;
    virtual void dispatchWillSubmitForm(WebCore::FramePolicyFunction, PassRefPtr<WebCore::FormState>) OVERRIDE;
    
    virtual void revertToProvisionalState(WebCore::DocumentLoader*) OVERRIDE;
    virtual void setMainDocumentError(WebCore::DocumentLoader*, const WebCore::ResourceError&) OVERRIDE;
    
    // Maybe these should go into a ProgressTrackerClient some day
    virtual void willChangeEstimatedProgress() OVERRIDE;
    virtual void didChangeEstimatedProgress() OVERRIDE;
    virtual void postProgressStartedNotification() OVERRIDE;
    virtual void postProgressEstimateChangedNotification() OVERRIDE;
    virtual void postProgressFinishedNotification() OVERRIDE;
    
    virtual void setMainFrameDocumentReady(bool) OVERRIDE;
    
    virtual void startDownload(const WebCore::ResourceRequest&, const String& suggestedName = String()) OVERRIDE;
    
    virtual void willChangeTitle(WebCore::DocumentLoader*) OVERRIDE;
    virtual void didChangeTitle(WebCore::DocumentLoader*) OVERRIDE;
    
    virtual void committedLoad(WebCore::DocumentLoader*, const char*, int) OVERRIDE;
    virtual void finishedLoading(WebCore::DocumentLoader*) OVERRIDE;
    
    virtual void updateGlobalHistory() OVERRIDE;
    virtual void updateGlobalHistoryRedirectLinks() OVERRIDE;
    
    virtual bool shouldGoToHistoryItem(WebCore::HistoryItem*) const OVERRIDE;
    virtual bool shouldStopLoadingForHistoryItem(WebCore::HistoryItem*) const OVERRIDE;

    virtual void didDisplayInsecureContent() OVERRIDE;
    virtual void didRunInsecureContent(WebCore::SecurityOrigin*, const WebCore::KURL&) OVERRIDE;
    virtual void didDetectXSS(const WebCore::KURL&, bool didBlockEntirePage) OVERRIDE;

    virtual WebCore::ResourceError cancelledError(const WebCore::ResourceRequest&) OVERRIDE;
    virtual WebCore::ResourceError blockedError(const WebCore::ResourceRequest&) OVERRIDE;
    virtual WebCore::ResourceError cannotShowURLError(const WebCore::ResourceRequest&) OVERRIDE;
    virtual WebCore::ResourceError interruptedForPolicyChangeError(const WebCore::ResourceRequest&) OVERRIDE;
    
    virtual WebCore::ResourceError cannotShowMIMETypeError(const WebCore::ResourceResponse&) OVERRIDE;
    virtual WebCore::ResourceError fileDoesNotExistError(const WebCore::ResourceResponse&) OVERRIDE;
    virtual WebCore::ResourceError pluginWillHandleLoadError(const WebCore::ResourceResponse&) OVERRIDE;
    
    virtual bool shouldFallBack(const WebCore::ResourceError&) OVERRIDE;
    
    virtual bool canHandleRequest(const WebCore::ResourceRequest&) const OVERRIDE;
    virtual bool canShowMIMEType(const String& MIMEType) const OVERRIDE;
    virtual bool canShowMIMETypeAsHTML(const String& MIMEType) const OVERRIDE;
    virtual bool representationExistsForURLScheme(const String& URLScheme) const OVERRIDE;
    virtual String generatedMIMETypeForURLScheme(const String& URLScheme) const OVERRIDE;
    
    virtual void frameLoadCompleted() OVERRIDE;
    virtual void saveViewStateToItem(WebCore::HistoryItem*) OVERRIDE;
    virtual void restoreViewState() OVERRIDE;
    virtual void provisionalLoadStarted() OVERRIDE;
    virtual void didFinishLoad() OVERRIDE;
    virtual void prepareForDataSourceReplacement() OVERRIDE;
    
    virtual PassRefPtr<WebCore::DocumentLoader> createDocumentLoader(const WebCore::ResourceRequest&, const WebCore::SubstituteData&);
    virtual void setTitle(const WebCore::StringWithDirection&, const WebCore::KURL&) OVERRIDE;
    
    virtual String userAgent(const WebCore::KURL&) OVERRIDE;
    
    virtual void savePlatformDataToCachedFrame(WebCore::CachedFrame*) OVERRIDE;
    virtual void transitionToCommittedFromCachedFrame(WebCore::CachedFrame*) OVERRIDE;
    virtual void transitionToCommittedForNewPage() OVERRIDE;

    virtual void didSaveToPageCache() OVERRIDE;
    virtual void didRestoreFromPageCache() OVERRIDE;

    virtual void dispatchDidBecomeFrameset(bool) OVERRIDE;

    virtual bool canCachePage() const OVERRIDE { return true; }
    virtual void convertMainResourceLoadToDownload(WebCore::DocumentLoader*, const WebCore::ResourceRequest&, const WebCore::ResourceResponse&) OVERRIDE;
    
    virtual PassRefPtr<WebCore::Frame> createFrame(const WebCore::KURL& url, const String& name, WebCore::HTMLFrameOwnerElement* ownerElement,
                                          const String& referrer, bool allowsScrolling, int marginWidth, int marginHeight) OVERRIDE;
    
    virtual PassRefPtr<WebCore::Widget> createPlugin(const WebCore::IntSize&, WebCore::HTMLPlugInElement*, const WebCore::KURL&, const Vector<String>&, const Vector<String>&, const String&, bool loadManually) OVERRIDE;
    virtual void recreatePlugin(WebCore::Widget*) OVERRIDE;
    virtual void redirectDataToPlugin(WebCore::Widget* pluginWidget) OVERRIDE;
    
    virtual PassRefPtr<WebCore::Widget> createJavaAppletWidget(const WebCore::IntSize&, WebCore::HTMLAppletElement*, const WebCore::KURL& baseURL, const Vector<String>& paramNames, const Vector<String>& paramValues) OVERRIDE;
    
#if ENABLE(PLUGIN_PROXY_FOR_VIDEO)
    virtual PassRefPtr<WebCore::Widget> createMediaPlayerProxyPlugin(const WebCore::IntSize&, WebCore::HTMLMediaElement*, const WebCore::KURL&, const Vector<String>&, const Vector<String>&, const String&) OVERRIDE;
    virtual void hideMediaPlayerProxyPlugin(WebCore::Widget*) OVERRIDE;
    virtual void showMediaPlayerProxyPlugin(WebCore::Widget*) OVERRIDE;
#endif

    virtual WebCore::ObjectContentType objectContentType(const WebCore::KURL&, const String& mimeType, bool shouldPreferPlugInsForImages) OVERRIDE;
    virtual String overrideMediaType() const OVERRIDE;

    virtual void dispatchDidClearWindowObjectInWorld(WebCore::DOMWrapperWorld*) OVERRIDE;
    
    virtual void dispatchGlobalObjectAvailable(WebCore::DOMWrapperWorld*) OVERRIDE;
    virtual void dispatchWillDisconnectDOMWindowExtensionFromGlobalObject(WebCore::DOMWindowExtension*) OVERRIDE;
    virtual void dispatchDidReconnectDOMWindowExtensionToGlobalObject(WebCore::DOMWindowExtension*) OVERRIDE;
    virtual void dispatchWillDestroyGlobalObjectForDOMWindowExtension(WebCore::DOMWindowExtension*) OVERRIDE;

    virtual void documentElementAvailable() OVERRIDE;
    virtual void didPerformFirstNavigation() const OVERRIDE; // "Navigation" here means a transition from one page to another that ends up in the back/forward list.
    
    virtual void registerForIconNotification(bool listen = true) OVERRIDE;
    
#if PLATFORM(MAC)
    virtual RemoteAXObjectRef accessibilityRemoteObject() OVERRIDE;
    
    virtual NSCachedURLResponse* willCacheResponse(WebCore::DocumentLoader*, unsigned long identifier, NSCachedURLResponse*) const OVERRIDE;
#endif

    virtual bool shouldAlwaysUsePluginDocument(const String& /*mimeType*/) const OVERRIDE;

    virtual void didChangeScrollOffset() OVERRIDE;

    virtual bool allowScript(bool enabledPerSettings) OVERRIDE;

    virtual bool shouldForceUniversalAccessFromLocalURL(const WebCore::KURL&) OVERRIDE;

    virtual PassRefPtr<WebCore::FrameNetworkingContext> createNetworkingContext() OVERRIDE;

    virtual void forcePageTransitionIfNeeded() OVERRIDE;

    WebFrame* m_frame;
    RefPtr<PluginView> m_pluginView;
    bool m_hasSentResponseToPluginView;
    bool m_didCompletePageTransitionAlready;
    bool m_frameCameFromPageCache;
};

// As long as EmptyFrameLoaderClient exists in WebCore, this can return 0.
inline WebFrameLoaderClient* toWebFrameLoaderClient(WebCore::FrameLoaderClient* client)
{
    return client->isEmptyFrameLoaderClient() ? 0 : static_cast<WebFrameLoaderClient*>(client);
}

} // namespace WebKit

#endif // WebFrameLoaderClient_h
