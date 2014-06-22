/*
 * Copyright (C) 2006, 2007, 2008, 2009, 2011 Apple Inc. All rights reserved.
 * Copyright (C) 2008, 2009 Torch Mobile Inc. All rights reserved. (http://www.torchmobile.com/)
 * Copyright (C) Research In Motion Limited 2009. All rights reserved.
 * Copyright (C) 2011 Google Inc. All rights reserved.
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

#ifndef FrameLoader_h
#define FrameLoader_h

#include "CachePolicy.h"
#include "FrameLoaderStateMachine.h"
#include "FrameLoaderTypes.h"
#include "IconURL.h"
#include "LayoutMilestones.h"
#include "MixedContentChecker.h"
#include "ResourceHandleTypes.h"
#include "ResourceLoadNotifier.h"
#include "SecurityContext.h"
#include "SubframeLoader.h"
#include "Timer.h"
#include <wtf/Forward.h>
#include <wtf/HashSet.h>

namespace WebCore {

class Archive;
class CachedFrameBase;
class CachedPage;
class CachedResource;
class Chrome;
class DOMWrapperWorld;
class DocumentLoader;
class Event;
class FormState;
class FormSubmission;
class FrameLoaderClient;
class FrameNetworkingContext;
class HistoryController;
class HistoryItem;
class IconController;
class NavigationAction;
class NetworkingContext;
class Page;
class PageActivityAssertionToken;
class PolicyChecker;
class ResourceError;
class ResourceRequest;
class ResourceResponse;
class SecurityOrigin;
class SerializedScriptValue;
class StringWithDirection;
class SubstituteData;

struct FrameLoadRequest;
struct WindowFeatures;

bool isBackForwardLoadType(FrameLoadType);

class FrameLoader {
    WTF_MAKE_NONCOPYABLE(FrameLoader);
public:
    FrameLoader(Frame*, FrameLoaderClient*);
    ~FrameLoader();

    void init();

    Frame* frame() const { return m_frame; }

    PolicyChecker* policyChecker() const { return m_policyChecker.get(); }
    HistoryController* history() const { return m_history.get(); }
    ResourceLoadNotifier* notifier() const { return &m_notifer; }
    SubframeLoader* subframeLoader() const { return &m_subframeLoader; }
    IconController* icon() const { return m_icon.get(); }
    MixedContentChecker* mixedContentChecker() const { return &m_mixedContentChecker; }

    void prepareForHistoryNavigation();
    void setupForReplace();

    // FIXME: These are all functions which start loads. We have too many.
    void loadURLIntoChildFrame(const KURL&, const String& referer, Frame*);
    void loadFrameRequest(const FrameLoadRequest&, bool lockHistory, bool lockBackForwardList,  // Called by submitForm, calls loadPostRequest and loadURL.
        PassRefPtr<Event>, PassRefPtr<FormState>, ShouldSendReferrer);

    void load(const FrameLoadRequest&);

#if ENABLE(WEB_ARCHIVE) || ENABLE(MHTML)
    void loadArchive(PassRefPtr<Archive>);
#endif
    unsigned long loadResourceSynchronously(const ResourceRequest&, StoredCredentials, ClientCredentialPolicy, ResourceError&, ResourceResponse&, Vector<char>& data);

    void changeLocation(SecurityOrigin*, const KURL&, const String& referrer, bool lockHistory = true, bool lockBackForwardList = true, bool refresh = false);
    void urlSelected(const KURL&, const String& target, PassRefPtr<Event>, bool lockHistory, bool lockBackForwardList, ShouldSendReferrer);
    void submitForm(PassRefPtr<FormSubmission>);

    void reload(bool endToEndReload = false);
    void reloadWithOverrideEncoding(const String& overrideEncoding);
    void reloadWithOverrideURL(const KURL& overrideUrl, bool endToEndReload = false);

    void open(CachedFrameBase&);
    void loadItem(HistoryItem*, FrameLoadType);
    HistoryItem* requestedHistoryItem() const { return m_requestedHistoryItem.get(); }

    void retryAfterFailedCacheOnlyMainResourceLoad();

    static void reportLocalLoadFailed(Frame*, const String& url);

    // FIXME: These are all functions which stop loads. We have too many.
    void stopAllLoaders(ClearProvisionalItemPolicy = ShouldClearProvisionalItem);
    void stopForUserCancel(bool deferCheckLoadComplete = false);
    void stop();
    void stopLoading(UnloadEventPolicy);
    bool closeURL();
    void cancelAndClear();
    // FIXME: clear() is trying to do too many things. We should break it down into smaller functions (ideally with fewer raw Boolean parameters).
    void clear(Document* newDocument, bool clearWindowProperties = true, bool clearScriptObjects = true, bool clearFrameView = true);

    bool isLoading() const;
    bool frameHasLoaded() const;

    int numPendingOrLoadingRequests(bool recurse) const;
    String referrer() const;
    String outgoingReferrer() const;
    String outgoingOrigin() const;

    DocumentLoader* activeDocumentLoader() const;
    DocumentLoader* documentLoader() const { return m_documentLoader.get(); }
    DocumentLoader* policyDocumentLoader() const { return m_policyDocumentLoader.get(); }
    DocumentLoader* provisionalDocumentLoader() const { return m_provisionalDocumentLoader.get(); }
    FrameState state() const { return m_state; }
    static double timeOfLastCompletedLoad();

    const ResourceRequest& originalRequest() const;
    const ResourceRequest& initialRequest() const;
    void receivedMainResourceError(const ResourceError&);

    bool willLoadMediaElementURL(KURL&);

    void handleFallbackContent();

    ResourceError cancelledError(const ResourceRequest&) const;

    bool isHostedByObjectElement() const;
    bool isLoadingMainFrame() const;

    bool isReplacing() const;
    void setReplacing();
    bool subframeIsLoading() const;
    void willChangeTitle(DocumentLoader*);
    void didChangeTitle(DocumentLoader*);
    void didChangeIcons(IconType);

    bool shouldTreatURLAsSrcdocDocument(const KURL&) const;

    FrameLoadType loadType() const;

    CachePolicy subresourceCachePolicy() const;

    void didLayout(LayoutMilestones);
    void didFirstLayout();

    void loadedResourceFromMemoryCache(CachedResource*, ResourceRequest& newRequest);
    void tellClientAboutPastMemoryCacheLoads();

    void checkLoadComplete();
    void detachFromParent();
    void detachViewsAndDocumentLoader();

    void addExtraFieldsToSubresourceRequest(ResourceRequest&);
    void addExtraFieldsToMainResourceRequest(ResourceRequest&);
    
    static void addHTTPOriginIfNeeded(ResourceRequest&, const String& origin);

    FrameLoaderClient* client() const { return m_client; }

    void setDefersLoading(bool);

    void didExplicitOpen();

    // Callbacks from DocumentWriter
    void didBeginDocument(bool dispatchWindowObjectAvailable);

    void receivedFirstData();

    void handledOnloadEvents();
    String userAgent(const KURL&) const;

    void dispatchDidClearWindowObjectInWorld(DOMWrapperWorld*);
    void dispatchDidClearWindowObjectsInAllWorlds();
    void dispatchDocumentElementAvailable();

    // The following sandbox flags will be forced, regardless of changes to
    // the sandbox attribute of any parent frames.
    void forceSandboxFlags(SandboxFlags flags) { m_forcedSandboxFlags |= flags; }
    SandboxFlags effectiveSandboxFlags() const;

    bool checkIfFormActionAllowedByCSP(const KURL&) const;

    Frame* opener();
    void setOpener(Frame*);

    void resetMultipleFormSubmissionProtection();

    void checkCallImplicitClose();

    void frameDetached();

    void setOutgoingReferrer(const KURL&);

    void loadDone();
    void finishedParsing();
    void checkCompleted();

    void checkDidPerformFirstNavigation();

    bool isComplete() const;

    void setTitle(const StringWithDirection&);

    void commitProvisionalLoad();

    FrameLoaderStateMachine* stateMachine() const { return &m_stateMachine; }

    Frame* findFrameForNavigation(const AtomicString& name, Document* activeDocument = 0);

    void applyUserAgent(ResourceRequest&);

    bool shouldInterruptLoadForXFrameOptions(const String&, const KURL&, unsigned long requestIdentifier);

    void completed();
    bool allAncestorsAreComplete() const; // including this
    void clientRedirected(const KURL&, double delay, double fireDate, bool lockBackForwardList);
    void clientRedirectCancelledOrFinished(bool cancelWithLoadInProgress);

    // FIXME: This is public because this asynchronous callback from the FrameLoaderClient
    // uses the policy machinery (and therefore is called via the PolicyChecker).  Once we
    // introduce a proper callback type for this function, we should make it private again.
    void continueLoadAfterWillSubmitForm();

    void setOriginalURLForDownloadRequest(ResourceRequest&);

    bool suppressOpenerInNewFrame() const { return m_suppressOpenerInNewFrame; }

    static ObjectContentType defaultObjectContentType(const KURL&, const String& mimeType, bool shouldPreferPlugInsForImages);

    bool quickRedirectComing() const { return m_quickRedirectComing; }

    bool shouldClose();
    
    void started();

    enum PageDismissalType {
        NoDismissal = 0,
        BeforeUnloadDismissal = 1,
        PageHideDismissal = 2,
        UnloadDismissal = 3
    };
    PageDismissalType pageDismissalEventBeingDispatched() const { return m_pageDismissalEventBeingDispatched; }

    NetworkingContext* networkingContext() const;

    void loadProgressingStatusChanged();

    const KURL& previousURL() const { return m_previousURL; }

    void forcePageTransitionIfNeeded();

private:
    enum FormSubmissionCacheLoadPolicy {
        MayAttemptCacheOnlyLoadForFormSubmissionItem,
        MayNotAttemptCacheOnlyLoadForFormSubmissionItem
    };

    bool allChildrenAreComplete() const; // immediate children, not all descendants

    void checkTimerFired(Timer<FrameLoader>*);
    
    void loadSameDocumentItem(HistoryItem*);
    void loadDifferentDocumentItem(HistoryItem*, FrameLoadType, FormSubmissionCacheLoadPolicy);
    
    void loadProvisionalItemFromCachedPage();

    void updateFirstPartyForCookies();
    void setFirstPartyForCookies(const KURL&);
    
    void addExtraFieldsToRequest(ResourceRequest&, FrameLoadType, bool isMainResource);

    void clearProvisionalLoad();
    void transitionToCommitted(PassRefPtr<CachedPage>);
    void frameLoadCompleted();

    SubstituteData defaultSubstituteDataForURL(const KURL&);

    static void callContinueLoadAfterNavigationPolicy(void*, const ResourceRequest&, PassRefPtr<FormState>, bool shouldContinue);
    static void callContinueLoadAfterNewWindowPolicy(void*, const ResourceRequest&, PassRefPtr<FormState>, const String& frameName, const NavigationAction&, bool shouldContinue);
    static void callContinueFragmentScrollAfterNavigationPolicy(void*, const ResourceRequest&, PassRefPtr<FormState>, bool shouldContinue);
    
    bool handleBeforeUnloadEvent(Chrome&, FrameLoader* frameLoaderBeingNavigated);

    void continueLoadAfterNavigationPolicy(const ResourceRequest&, PassRefPtr<FormState>, bool shouldContinue);
    void continueLoadAfterNewWindowPolicy(const ResourceRequest&, PassRefPtr<FormState>, const String& frameName, const NavigationAction&, bool shouldContinue);
    void continueFragmentScrollAfterNavigationPolicy(const ResourceRequest&, bool shouldContinue);

    bool shouldPerformFragmentNavigation(bool isFormSubmission, const String& httpMethod, FrameLoadType, const KURL&);
    void scrollToFragmentWithParentBoundary(const KURL&);

    void checkLoadCompleteForThisFrame();

    void setDocumentLoader(DocumentLoader*);
    void setPolicyDocumentLoader(DocumentLoader*);
    void setProvisionalDocumentLoader(DocumentLoader*);

    void setState(FrameState);

    void closeOldDataSources();
    void prepareForCachedPageRestore();

    bool shouldReloadToHandleUnreachableURL(DocumentLoader*);

    void dispatchDidCommitLoad();

    void urlSelected(const FrameLoadRequest&, PassRefPtr<Event>, bool lockHistory, bool lockBackForwardList, ShouldSendReferrer, ShouldReplaceDocumentIfJavaScriptURL);

    void loadWithDocumentLoader(DocumentLoader*, FrameLoadType, PassRefPtr<FormState>); // Calls continueLoadAfterNavigationPolicy
    void load(DocumentLoader*);                                                         // Calls loadWithDocumentLoader   

    void loadWithNavigationAction(const ResourceRequest&, const NavigationAction&,      // Calls loadWithDocumentLoader
        bool lockHistory, FrameLoadType, PassRefPtr<FormState>);

    void loadPostRequest(const ResourceRequest&, const String& referrer,                // Called by loadFrameRequest, calls loadWithNavigationAction
        const String& frameName, bool lockHistory, FrameLoadType, PassRefPtr<Event>, PassRefPtr<FormState>);
    void loadURL(const KURL&, const String& referrer, const String& frameName,          // Called by loadFrameRequest, calls loadWithNavigationAction or dispatches to navigation policy delegate
        bool lockHistory, FrameLoadType, PassRefPtr<Event>, PassRefPtr<FormState>);                                                         

    void reloadWithRequest(const ResourceRequest&, bool endToEndReload);

    bool shouldReload(const KURL& currentURL, const KURL& destinationURL);

    void requestFromDelegate(ResourceRequest&, unsigned long& identifier, ResourceError&);

    void detachChildren();
    void closeAndRemoveChild(Frame*);

    void loadInSameDocument(const KURL&, PassRefPtr<SerializedScriptValue> stateObject, bool isNewNavigation);

    void prepareForLoadStart();
    void provisionalLoadStarted();

    void willTransitionToCommitted();
    bool didOpenURL();

    void scheduleCheckCompleted();
    void scheduleCheckLoadComplete();
    void startCheckCompleteTimer();

    bool shouldTreatURLAsSameAsCurrent(const KURL&) const;

    void dispatchGlobalObjectAvailableInAllWorlds();

    Frame* m_frame;
    FrameLoaderClient* m_client;

    // FIXME: These should be OwnPtr<T> to reduce build times and simplify
    // header dependencies unless performance testing proves otherwise.
    // Some of these could be lazily created for memory savings on devices.
    OwnPtr<PolicyChecker> m_policyChecker;
    OwnPtr<HistoryController> m_history;
    mutable ResourceLoadNotifier m_notifer;
    mutable SubframeLoader m_subframeLoader;
    mutable FrameLoaderStateMachine m_stateMachine;
    OwnPtr<IconController> m_icon;
    mutable MixedContentChecker m_mixedContentChecker;

    class FrameProgressTracker;
    OwnPtr<FrameProgressTracker> m_progressTracker;

    FrameState m_state;
    FrameLoadType m_loadType;

    // Document loaders for the three phases of frame loading. Note that while 
    // a new request is being loaded, the old document loader may still be referenced.
    // E.g. while a new request is in the "policy" state, the old document loader may
    // be consulted in particular as it makes sense to imply certain settings on the new loader.
    RefPtr<DocumentLoader> m_documentLoader;
    RefPtr<DocumentLoader> m_provisionalDocumentLoader;
    RefPtr<DocumentLoader> m_policyDocumentLoader;

    bool m_delegateIsHandlingProvisionalLoadError;

    bool m_quickRedirectComing;
    bool m_sentRedirectNotification;
    bool m_inStopAllLoaders;

    String m_outgoingReferrer;

    bool m_isExecutingJavaScriptFormAction;

    bool m_didCallImplicitClose;
    bool m_wasUnloadEventEmitted;
    PageDismissalType m_pageDismissalEventBeingDispatched;
    bool m_isComplete;

    RefPtr<SerializedScriptValue> m_pendingStateObject;

    bool m_needsClear;

    KURL m_submittedFormURL;

    Timer<FrameLoader> m_checkTimer;
    bool m_shouldCallCheckCompleted;
    bool m_shouldCallCheckLoadComplete;

    Frame* m_opener;
    HashSet<Frame*> m_openedFrames;

    bool m_didPerformFirstNavigation;
    bool m_loadingFromCachedPage;
    bool m_suppressOpenerInNewFrame;
    
    bool m_currentNavigationHasShownBeforeUnloadConfirmPanel;

    SandboxFlags m_forcedSandboxFlags;

    RefPtr<FrameNetworkingContext> m_networkingContext;

    KURL m_previousURL;
    RefPtr<HistoryItem> m_requestedHistoryItem;
    OwnPtr<PageActivityAssertionToken> m_activityAssertion;
};

// This function is called by createWindow() in JSDOMWindowBase.cpp, for example, for
// modal dialog creation.  The lookupFrame is for looking up the frame name in case
// the frame name references a frame different from the openerFrame, e.g. when it is
// "_self" or "_parent".
//
// FIXME: Consider making this function part of an appropriate class (not FrameLoader)
// and moving it to a more appropriate location.
PassRefPtr<Frame> createWindow(Frame* openerFrame, Frame* lookupFrame, const FrameLoadRequest&, const WindowFeatures&, bool& created);

} // namespace WebCore

#endif // FrameLoader_h
