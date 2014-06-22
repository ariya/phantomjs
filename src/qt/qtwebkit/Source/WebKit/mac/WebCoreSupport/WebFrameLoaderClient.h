/*
 * Copyright (C) 2006, 2007, 2008, 2011, 2012 Apple Inc. All rights reserved.
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

#import <WebCore/FrameLoaderClient.h>
#import <WebCore/Timer.h>
#import <wtf/Forward.h>
#import <wtf/HashMap.h>
#import <wtf/RetainPtr.h>

@class WebDownload;
@class WebFrame;
@class WebFramePolicyListener;
@class WebHistoryItem;
@class WebResource;

namespace WebCore {
    class AuthenticationChallenge;
    class CachedFrame;
    class HistoryItem;
    class ProtectionSpace;
    class ResourceLoader;
    class ResourceRequest;
}

typedef HashMap<RefPtr<WebCore::ResourceLoader>, RetainPtr<WebResource> > ResourceMap;

class WebFrameLoaderClient : public WebCore::FrameLoaderClient {
public:
    WebFrameLoaderClient(WebFrame*);

    WebFrame* webFrame() const { return m_webFrame.get(); }

private:
    virtual void frameLoaderDestroyed() OVERRIDE;
    virtual bool hasWebView() const OVERRIDE; // mainly for assertions

    virtual void makeRepresentation(WebCore::DocumentLoader*) OVERRIDE;
    virtual bool hasHTMLView() const OVERRIDE;
    virtual void forceLayout() OVERRIDE;
    virtual void forceLayoutForNonHTML() OVERRIDE;

    virtual void setCopiesOnScroll() OVERRIDE;

    virtual void detachedFromParent2() OVERRIDE;
    virtual void detachedFromParent3() OVERRIDE;

    virtual void convertMainResourceLoadToDownload(WebCore::DocumentLoader*, const WebCore::ResourceRequest&, const WebCore::ResourceResponse&) OVERRIDE;

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

    virtual NSCachedURLResponse* willCacheResponse(WebCore::DocumentLoader*, unsigned long identifier, NSCachedURLResponse*) const OVERRIDE;

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

    virtual WebCore::Frame* dispatchCreatePage(const WebCore::NavigationAction&) OVERRIDE;
    virtual void dispatchShow() OVERRIDE;

    virtual void dispatchDecidePolicyForResponse(WebCore::FramePolicyFunction, 
        const WebCore::ResourceResponse&, const WebCore::ResourceRequest&);
    virtual void dispatchDecidePolicyForNewWindowAction(WebCore::FramePolicyFunction,
        const WebCore::NavigationAction&, const WebCore::ResourceRequest&, PassRefPtr<WebCore::FormState>, const WTF::String& frameName) OVERRIDE;
    virtual void dispatchDecidePolicyForNavigationAction(WebCore::FramePolicyFunction,
        const WebCore::NavigationAction&, const WebCore::ResourceRequest&, PassRefPtr<WebCore::FormState>) OVERRIDE;
    virtual void cancelPolicyCheck() OVERRIDE;

    virtual void dispatchUnableToImplementPolicy(const WebCore::ResourceError&) OVERRIDE;

    virtual void dispatchWillSendSubmitEvent(PassRefPtr<WebCore::FormState>) OVERRIDE;
    virtual void dispatchWillSubmitForm(WebCore::FramePolicyFunction, PassRefPtr<WebCore::FormState>) OVERRIDE;

    virtual void revertToProvisionalState(WebCore::DocumentLoader*) OVERRIDE;
    virtual void setMainDocumentError(WebCore::DocumentLoader*, const WebCore::ResourceError&) OVERRIDE;
    virtual bool dispatchDidLoadResourceFromMemoryCache(WebCore::DocumentLoader*, const WebCore::ResourceRequest&, const WebCore::ResourceResponse&, int length) OVERRIDE;

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
    virtual void updateGlobalHistoryItemForPage() OVERRIDE;

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

    virtual WTF::String userAgent(const WebCore::KURL&) OVERRIDE;
    
    virtual void savePlatformDataToCachedFrame(WebCore::CachedFrame*) OVERRIDE;
    virtual void transitionToCommittedFromCachedFrame(WebCore::CachedFrame*) OVERRIDE;
    virtual void transitionToCommittedForNewPage() OVERRIDE;

    virtual void didSaveToPageCache() OVERRIDE;
    virtual void didRestoreFromPageCache() OVERRIDE;

    virtual void dispatchDidBecomeFrameset(bool) OVERRIDE;

    virtual bool canHandleRequest(const WebCore::ResourceRequest&) const OVERRIDE;
    virtual bool canShowMIMEType(const WTF::String& MIMEType) const OVERRIDE;
    virtual bool canShowMIMETypeAsHTML(const WTF::String& MIMEType) const OVERRIDE;
    virtual bool representationExistsForURLScheme(const WTF::String& URLScheme) const OVERRIDE;
    virtual WTF::String generatedMIMETypeForURLScheme(const WTF::String& URLScheme) const OVERRIDE;

    virtual void frameLoadCompleted() OVERRIDE;
    virtual void saveViewStateToItem(WebCore::HistoryItem*) OVERRIDE;
    virtual void restoreViewState() OVERRIDE;
    virtual void provisionalLoadStarted() OVERRIDE;
    virtual void didFinishLoad() OVERRIDE;
    virtual void prepareForDataSourceReplacement() OVERRIDE;
    virtual PassRefPtr<WebCore::DocumentLoader> createDocumentLoader(const WebCore::ResourceRequest&, const WebCore::SubstituteData&) OVERRIDE;

    virtual void setTitle(const WebCore::StringWithDirection&, const WebCore::KURL&) OVERRIDE;

    virtual PassRefPtr<WebCore::Frame> createFrame(const WebCore::KURL& url, const WTF::String& name, WebCore::HTMLFrameOwnerElement*,
                                        const WTF::String& referrer, bool allowsScrolling, int marginWidth, int marginHeight) OVERRIDE;
    virtual PassRefPtr<WebCore::Widget> createPlugin(const WebCore::IntSize&, WebCore::HTMLPlugInElement*, const WebCore::KURL&, const Vector<WTF::String>&,
                                          const Vector<WTF::String>&, const WTF::String&, bool) OVERRIDE;
    virtual void recreatePlugin(WebCore::Widget*) OVERRIDE;
    virtual void redirectDataToPlugin(WebCore::Widget* pluginWidget) OVERRIDE;
    
    virtual PassRefPtr<WebCore::Widget> createJavaAppletWidget(const WebCore::IntSize&, WebCore::HTMLAppletElement*, const WebCore::KURL& baseURL,
                                                    const Vector<WTF::String>& paramNames, const Vector<WTF::String>& paramValues) OVERRIDE;
    
#if ENABLE(PLUGIN_PROXY_FOR_VIDEO)
    virtual PassRefPtr<WebCore::Widget> createMediaPlayerProxyPlugin(const WebCore::IntSize&, WebCore::HTMLMediaElement*, const WebCore::KURL&, 
                                        const Vector<WTF::String>&, const Vector<WTF::String>&, const WTF::String&) OVERRIDE;
    virtual void hideMediaPlayerProxyPlugin(WebCore::Widget*) OVERRIDE;
    virtual void showMediaPlayerProxyPlugin(WebCore::Widget*) OVERRIDE;
#endif
    
    virtual WebCore::ObjectContentType objectContentType(const WebCore::KURL&, const WTF::String& mimeType, bool shouldPreferPlugInsForImages) OVERRIDE;
    virtual WTF::String overrideMediaType() const OVERRIDE;
    
    virtual void dispatchDidClearWindowObjectInWorld(WebCore::DOMWrapperWorld*) OVERRIDE;
    virtual void documentElementAvailable() OVERRIDE;
    virtual void didPerformFirstNavigation() const OVERRIDE;

    virtual void registerForIconNotification(bool listen) OVERRIDE;

#if PLATFORM(MAC)
    virtual RemoteAXObjectRef accessibilityRemoteObject() OVERRIDE { return 0; }
#endif
    
    RetainPtr<WebFramePolicyListener> setUpPolicyListener(WebCore::FramePolicyFunction);

    NSDictionary *actionDictionary(const WebCore::NavigationAction&, PassRefPtr<WebCore::FormState>) const;
    
    virtual bool canCachePage() const;

    virtual PassRefPtr<WebCore::FrameNetworkingContext> createNetworkingContext();

    virtual bool shouldPaintBrokenImage(const WebCore::KURL&) const;

    RetainPtr<WebFrame> m_webFrame;

    RetainPtr<WebFramePolicyListener> m_policyListener;
};
