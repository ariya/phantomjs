/*
 * Copyright (C) 2009, 2010, 2011, 2012 Research In Motion Limited. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifndef FrameLoaderClientBlackBerry_h
#define FrameLoaderClientBlackBerry_h

#include "CredentialTransformData.h"
#include "DocumentLoader.h"
#include "Frame.h"
#include "FrameLoaderClient.h"
#include "NotImplemented.h"
#include "Widget.h"

namespace BlackBerry {
namespace WebKit {
class WebPagePrivate;
}
}

namespace WebCore {

class FrameNetworkingContext;
class Geolocation;

class FrameLoaderClientBlackBerry : public FrameLoaderClient {
public:
    FrameLoaderClientBlackBerry();
    ~FrameLoaderClientBlackBerry();

    void setFrame(Frame* frame, BlackBerry::WebKit::WebPagePrivate* webPagePrivate) { m_frame = frame; m_webPagePrivate = webPagePrivate; }

    int playerId() const;
    bool cookiesEnabled() const;

    virtual void frameLoaderDestroyed();
    virtual bool hasWebView() const { return true; }
    virtual void makeRepresentation(DocumentLoader*) { notImplemented(); }
    virtual void forceLayout() { notImplemented(); }
    virtual void forceLayoutForNonHTML() { notImplemented(); }
    virtual void setCopiesOnScroll() { notImplemented(); }
    virtual void detachedFromParent2();
    virtual void detachedFromParent3() { notImplemented(); }
    virtual void assignIdentifierToInitialRequest(long unsigned, DocumentLoader*, const ResourceRequest&) { notImplemented(); }
    virtual void dispatchWillSendRequest(DocumentLoader*, long unsigned, ResourceRequest&, const ResourceResponse&);
    virtual bool shouldUseCredentialStorage(DocumentLoader*, long unsigned);
    virtual void dispatchDidReceiveAuthenticationChallenge(DocumentLoader*, long unsigned, const AuthenticationChallenge&) { notImplemented(); }
    virtual void dispatchDidCancelAuthenticationChallenge(DocumentLoader*, long unsigned, const AuthenticationChallenge&) { notImplemented(); }
    virtual void dispatchDidReceiveResponse(DocumentLoader*, long unsigned, const ResourceResponse&);
    virtual void dispatchDidReceiveContentLength(DocumentLoader*, long unsigned, int) { notImplemented(); }
    virtual void dispatchDidFinishLoading(DocumentLoader*, long unsigned) { notImplemented(); }
    virtual void dispatchDidFailLoading(DocumentLoader*, long unsigned, const ResourceError&) { notImplemented(); }
    virtual bool dispatchDidLoadResourceFromMemoryCache(DocumentLoader*, const ResourceRequest&, const ResourceResponse&, int) { notImplemented(); return false; }
    virtual void dispatchDidHandleOnloadEvents();
    virtual void dispatchDidReceiveServerRedirectForProvisionalLoad() { notImplemented(); }
    virtual void dispatchDidCancelClientRedirect();
    virtual void dispatchWillPerformClientRedirect(const KURL&, double, double);
    virtual void dispatchDidChangeLocationWithinPage();
    virtual void dispatchDidPushStateWithinPage();
    virtual void dispatchDidReplaceStateWithinPage();
    virtual void dispatchDidPopStateWithinPage();
    virtual void dispatchWillClose();
    virtual void dispatchDidReceiveIcon();
    virtual void dispatchDidStartProvisionalLoad();
    virtual void dispatchDidReceiveTitle(const StringWithDirection&);
    virtual void setTitle(const StringWithDirection& title, const KURL&);
    virtual void dispatchDidCommitLoad();
    virtual void dispatchDidFailProvisionalLoad(const ResourceError&);
    virtual void dispatchDidFailLoad(const ResourceError&);
    virtual void dispatchDidFinishDocumentLoad();
    virtual void dispatchDidFinishLoad();
    virtual void dispatchDidLayout(LayoutMilestones);
    virtual Frame* dispatchCreatePage(const NavigationAction&);
    virtual void dispatchShow() { notImplemented(); }

    virtual void dispatchDecidePolicyForResponse(FramePolicyFunction, const ResourceResponse&, const ResourceRequest&);
    virtual void dispatchDecidePolicyForNewWindowAction(FramePolicyFunction, const NavigationAction&, const ResourceRequest&, PassRefPtr<FormState>, const String& frameName);
    virtual void dispatchDecidePolicyForNavigationAction(FramePolicyFunction, const NavigationAction&, const ResourceRequest&, PassRefPtr<FormState>);
    virtual void cancelPolicyCheck();
    virtual void dispatchUnableToImplementPolicy(const ResourceError&) { notImplemented(); }
    virtual void dispatchWillSubmitForm(FramePolicyFunction, PassRefPtr<FormState>);
    virtual void revertToProvisionalState(DocumentLoader*) { notImplemented(); }
    virtual void setMainDocumentError(DocumentLoader*, const ResourceError&);
    virtual void postProgressStartedNotification();
    virtual void postProgressEstimateChangedNotification();
    virtual void postProgressFinishedNotification();
    virtual void setMainFrameDocumentReady(bool) { notImplemented(); }
    virtual void startDownload(const ResourceRequest&, const String& suggestedName = String());
    virtual void willChangeTitle(DocumentLoader*) { notImplemented(); }
    virtual void didChangeTitle(DocumentLoader*) { notImplemented(); }
    virtual void committedLoad(DocumentLoader*, const char*, int);
    virtual void finishedLoading(DocumentLoader*);
    virtual void updateGlobalHistory() { notImplemented(); }
    virtual void updateGlobalHistoryRedirectLinks() { notImplemented(); }
    virtual bool shouldGoToHistoryItem(HistoryItem*) const;
    virtual bool shouldStopLoadingForHistoryItem(HistoryItem*) const;
    virtual void dispatchWillUpdateApplicationCache(const ResourceRequest&);
    virtual void dispatchDidLoadFromApplicationCache(const ResourceRequest&);
    virtual void didDisplayInsecureContent() { notImplemented(); }
    virtual void didRunInsecureContent(SecurityOrigin*, const KURL&) { notImplemented(); }
    virtual ResourceError interruptedForPolicyChangeError(const ResourceRequest&) { notImplemented(); return ResourceError(emptyString(), 0, emptyString(), emptyString()); }
    virtual ResourceError cancelledError(const ResourceRequest&) { notImplemented(); return ResourceError(emptyString(), 0, emptyString(), emptyString()); }
    virtual ResourceError blockedError(const ResourceRequest&) { notImplemented(); return ResourceError(emptyString(), 0, emptyString(), emptyString()); }
    virtual ResourceError cannotShowURLError(const ResourceRequest&);
    virtual ResourceError interruptForPolicyChangeError(const ResourceRequest&) { notImplemented(); return ResourceError(emptyString(), 0, emptyString(), emptyString()); }
    virtual ResourceError cannotShowMIMETypeError(const ResourceResponse&) { notImplemented(); return ResourceError(emptyString(), 0, emptyString(), emptyString()); }
    virtual ResourceError fileDoesNotExistError(const ResourceResponse&) { notImplemented(); return ResourceError(emptyString(), 0, emptyString(), emptyString()); }
    virtual ResourceError pluginWillHandleLoadError(const ResourceResponse&) { notImplemented(); return ResourceError(emptyString(), 0, emptyString(), emptyString()); }
    virtual bool shouldFallBack(const ResourceError&) { notImplemented(); return false; }
    virtual bool canHandleRequest(const ResourceRequest&) const;
    virtual bool canShowMIMEType(const String&) const;
    virtual bool canShowMIMETypeAsHTML(const String&) const;
    virtual bool representationExistsForURLScheme(const String&) const { notImplemented(); return false; }
    virtual String generatedMIMETypeForURLScheme(const String&) const { notImplemented(); return String(); }
    virtual void frameLoadCompleted() { notImplemented(); }
    virtual void saveViewStateToItem(HistoryItem*);
    virtual void restoreViewState();
    virtual void provisionalLoadStarted();
    virtual void didFinishLoad() { notImplemented(); }
    virtual void prepareForDataSourceReplacement() { notImplemented(); }
    virtual PassRefPtr<DocumentLoader> createDocumentLoader(const ResourceRequest&, const SubstituteData&);
    virtual void setTitle(const String&, const KURL&) { notImplemented(); }
    virtual String userAgent(const KURL&);
    virtual void savePlatformDataToCachedFrame(CachedFrame*) { notImplemented(); }
    virtual void transitionToCommittedFromCachedFrame(CachedFrame*) { notImplemented(); }
    virtual void transitionToCommittedForNewPage();
    virtual bool canCachePage() const;
    virtual void didSaveToPageCache();
    virtual void didRestoreFromPageCache();
    virtual void dispatchDidBecomeFrameset(bool) { }
    virtual void convertMainResourceLoadToDownload(DocumentLoader*, const ResourceRequest&, const ResourceResponse&);
    virtual PassRefPtr<Frame> createFrame(const KURL&, const String&, HTMLFrameOwnerElement*, const String&, bool, int, int);
    virtual bool shouldAlwaysUsePluginDocument(const String&) const;
    virtual PassRefPtr<Widget> createPlugin(const IntSize&, HTMLPlugInElement*, const KURL&, const Vector<String>&, const Vector<String>&, const String&, bool);
    virtual void redirectDataToPlugin(Widget*);
    virtual PassRefPtr<Widget> createJavaAppletWidget(const IntSize&, HTMLAppletElement*, const KURL&, const Vector<String>&, const Vector<String>&) { notImplemented(); return 0; }

    virtual ObjectContentType objectContentType(const KURL&, const String& mimeType, bool shouldPreferPlugInsForImages);
    virtual String overrideMediaType() const { notImplemented(); return String(); }
    virtual void dispatchDidClearWindowObjectInWorld(DOMWrapperWorld*);
    virtual void documentElementAvailable() { notImplemented(); }
    virtual void didPerformFirstNavigation() const { notImplemented(); }
    virtual void registerForIconNotification(bool) { notImplemented(); }

    virtual bool shouldLoadIconExternally() { return false; }
    virtual void loadIconExternally(const String& originalPageUrl, const String& finalPageUrl, const String& iconUrl);

    virtual void didDetectXSS(const KURL&, bool) { }
    virtual void dispatchDidChangeIcons(IconType) { notImplemented(); };
    virtual void dispatchWillSendSubmitEvent(PassRefPtr<FormState>);

    virtual void willDeferLoading();
    virtual void didResumeLoading();

    virtual PassRefPtr<FrameNetworkingContext> createNetworkingContext();

    virtual PassRefPtr<SecurityOrigin> securityOriginForNewDocument(const KURL&);

    void readyToRender(bool pageIsVisuallyNonEmpty);

    void doPendingFragmentScroll();

    // Used to stop media files from loading because we don't need to have the entire file loaded by WebKit.
    void setCancelLoadOnNextData() { m_cancelLoadOnNextData = true; }
    bool shouldCancelLoadOnNextData() const { return m_cancelLoadOnNextData; }

    void suppressChildFrameCreation() { m_childFrameCreationSuppressed = true; }

private:
    void receivedData(DocumentLoader*, const char*, int, const String&);
    void didFinishOrFailLoading(const ResourceError&);
    bool isMainFrame() const;

    PolicyAction decidePolicyForExternalLoad(const ResourceRequest &, bool isFragmentScroll);
    void delayPolicyCheckUntilFragmentExists(const String& fragment, FramePolicyFunction);

    Frame* m_frame;
    ResourceError m_loadError;
    BlackBerry::WebKit::WebPagePrivate* m_webPagePrivate;

    Geolocation* m_geolocation;
    bool m_sentReadyToRender;

    FramePolicyFunction m_pendingFragmentScrollPolicyFunction;
    String m_pendingFragmentScroll;

    bool m_loadingErrorPage;
    bool m_clientRedirectIsPending;
    bool m_childFrameCreationSuppressed;

    // This set includes the original and final urls for server redirects.
    HashSet<KURL> m_historyNavigationSourceURLs;
    HashSet<KURL> m_redirectURLsToSkipDueToHistoryNavigation;

    // Plugin view to redirect data to.
    PluginView* m_pluginView;
    bool m_hasSentResponseToPlugin;

    // Used to stop media files from loading because we don't need to have the entire file loaded by WebKit.
    bool m_cancelLoadOnNextData;

    bool m_wasProvisionalLoadTriggeredByUserGesture;
#if ENABLE(BLACKBERRY_CREDENTIAL_PERSIST)
    CredentialTransformData m_formCredentials;
#endif
};

} // WebCore

#endif // FrameLoaderClientBlackBerry_h
