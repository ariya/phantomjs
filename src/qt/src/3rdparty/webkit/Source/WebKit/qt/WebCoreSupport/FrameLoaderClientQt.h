/*
 * Copyright (C) 2006 Zack Rusin <zack@kde.org>
 * Copyright (C) 2006, 2011 Apple Inc. All rights reserved.
 * Copyright (C) 2008 Nokia Corporation and/or its subsidiary(-ies)
 * Copyright (C) 2008 Collabora Ltd. All rights reserved.
 *
 * All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef FrameLoaderClientQt_h
#define FrameLoaderClientQt_h


#include "Frame.h"
#include "FrameLoader.h"
#include "FrameLoaderClient.h"
#include "KURL.h"
#include <wtf/OwnPtr.h>
#include "PluginView.h"
#include "RefCounted.h"
#include "ResourceError.h"
#include "ResourceResponse.h"
#include <QUrl>
#include <qobject.h>
#include <wtf/Forward.h>

QT_BEGIN_NAMESPACE
class QNetworkReply;
QT_END_NAMESPACE

class QWebFrame;

namespace WebCore {

class AuthenticationChallenge;
class DocumentLoader;
class Element;
class FormState;
class NavigationAction;
class FrameNetworkingContext;
class ResourceLoader;

struct LoadErrorResetToken;

class FrameLoaderClientQt : public QObject, public FrameLoaderClient {
    Q_OBJECT

    friend class ::QWebFrame;
    void callPolicyFunction(FramePolicyFunction function, PolicyAction action);
    bool callErrorPageExtension(const ResourceError&);

signals:
    void loadProgress(int d);
    void titleChanged(const QString& title);
    void unsupportedContent(QNetworkReply*);

public:
    FrameLoaderClientQt();
    ~FrameLoaderClientQt();
    virtual void frameLoaderDestroyed();

    void setFrame(QWebFrame* webFrame, Frame* frame);

    virtual bool hasWebView() const; // mainly for assertions

    virtual void makeRepresentation(DocumentLoader*);
    virtual void forceLayout();
    virtual void forceLayoutForNonHTML();

    virtual void setCopiesOnScroll();

    virtual void detachedFromParent2();
    virtual void detachedFromParent3();

    virtual void assignIdentifierToInitialRequest(unsigned long identifier, WebCore::DocumentLoader*, const WebCore::ResourceRequest&);

    virtual void dispatchWillSendRequest(WebCore::DocumentLoader*, unsigned long, WebCore::ResourceRequest&, const WebCore::ResourceResponse&);
    virtual bool shouldUseCredentialStorage(DocumentLoader*, unsigned long identifier);
    virtual void dispatchDidReceiveAuthenticationChallenge(DocumentLoader*, unsigned long identifier, const AuthenticationChallenge&);
    virtual void dispatchDidCancelAuthenticationChallenge(DocumentLoader*, unsigned long identifier, const AuthenticationChallenge&);
    virtual void dispatchDidReceiveResponse(WebCore::DocumentLoader*, unsigned long, const WebCore::ResourceResponse&);
    virtual void dispatchDidReceiveContentLength(WebCore::DocumentLoader*, unsigned long, int);
    virtual void dispatchDidFinishLoading(WebCore::DocumentLoader*, unsigned long);
    virtual void dispatchDidFailLoading(WebCore::DocumentLoader*, unsigned long, const WebCore::ResourceError&);
    virtual bool dispatchDidLoadResourceFromMemoryCache(WebCore::DocumentLoader*, const WebCore::ResourceRequest&, const WebCore::ResourceResponse&, int);

    virtual void dispatchDidHandleOnloadEvents();
    virtual void dispatchDidReceiveServerRedirectForProvisionalLoad();
    virtual void dispatchDidCancelClientRedirect();
    virtual void dispatchWillPerformClientRedirect(const KURL&, double interval, double fireDate);
    virtual void dispatchDidChangeLocationWithinPage();
    virtual void dispatchDidPushStateWithinPage();
    virtual void dispatchDidReplaceStateWithinPage();
    virtual void dispatchDidPopStateWithinPage();
    virtual void dispatchWillClose();
    virtual void dispatchDidReceiveIcon();
    virtual void dispatchDidStartProvisionalLoad();
    virtual void dispatchDidReceiveTitle(const StringWithDirection&);
    virtual void dispatchDidChangeIcons(WebCore::IconType);
    virtual void dispatchDidCommitLoad();
    virtual void dispatchDidFailProvisionalLoad(const ResourceError&);
    virtual void dispatchDidFailLoad(const WebCore::ResourceError&);
    virtual void dispatchDidFinishDocumentLoad();
    virtual void dispatchDidFinishLoad();
    virtual void dispatchDidFirstLayout();
    virtual void dispatchDidFirstVisuallyNonEmptyLayout();

    virtual WebCore::Frame* dispatchCreatePage(const WebCore::NavigationAction&);
    virtual void dispatchShow();

    virtual void dispatchDecidePolicyForResponse(FramePolicyFunction function, const WebCore::ResourceResponse&, const WebCore::ResourceRequest&);
    virtual void dispatchDecidePolicyForNewWindowAction(FramePolicyFunction function, const WebCore::NavigationAction&, const WebCore::ResourceRequest&, PassRefPtr<FormState>, const WTF::String&);
    virtual void dispatchDecidePolicyForNavigationAction(FramePolicyFunction function, const WebCore::NavigationAction&, const WebCore::ResourceRequest&, PassRefPtr<FormState>);
    virtual void cancelPolicyCheck();

    virtual void dispatchUnableToImplementPolicy(const WebCore::ResourceError&);

    virtual void dispatchWillSendSubmitEvent(HTMLFormElement*) { }
    virtual void dispatchWillSubmitForm(FramePolicyFunction, PassRefPtr<FormState>);

    virtual void dispatchDidLoadMainResource(DocumentLoader*);
    virtual void revertToProvisionalState(DocumentLoader*);
    virtual void setMainDocumentError(DocumentLoader*, const ResourceError&);

    virtual void postProgressStartedNotification();
    virtual void postProgressEstimateChangedNotification();
    virtual void postProgressFinishedNotification();

    virtual void setMainFrameDocumentReady(bool);

    virtual void startDownload(const WebCore::ResourceRequest&);

    virtual void willChangeTitle(DocumentLoader*);
    virtual void didChangeTitle(DocumentLoader*);

    virtual void committedLoad(WebCore::DocumentLoader*, const char*, int);
    virtual void finishedLoading(DocumentLoader*);

    virtual void updateGlobalHistory();
    virtual void updateGlobalHistoryRedirectLinks();
    virtual bool shouldGoToHistoryItem(HistoryItem*) const;
    virtual bool shouldStopLoadingForHistoryItem(HistoryItem*) const;
    virtual void dispatchDidAddBackForwardItem(HistoryItem*) const;
    virtual void dispatchDidRemoveBackForwardItem(HistoryItem*) const;
    virtual void dispatchDidChangeBackForwardIndex() const;
    virtual void didDisplayInsecureContent();
    virtual void didRunInsecureContent(SecurityOrigin*, const KURL&);

    virtual ResourceError cancelledError(const ResourceRequest&);
    virtual ResourceError blockedError(const ResourceRequest&);
    virtual ResourceError cannotShowURLError(const ResourceRequest&);
    virtual ResourceError interruptForPolicyChangeError(const ResourceRequest&);

    virtual ResourceError cannotShowMIMETypeError(const ResourceResponse&);
    virtual ResourceError fileDoesNotExistError(const ResourceResponse&);
    virtual ResourceError pluginWillHandleLoadError(const ResourceResponse&);

    virtual bool shouldFallBack(const ResourceError&);

    virtual bool canHandleRequest(const WebCore::ResourceRequest&) const;
    virtual bool canShowMIMEType(const String& MIMEType) const;
    virtual bool canShowMIMETypeAsHTML(const String& MIMEType) const;
    virtual bool representationExistsForURLScheme(const String& URLScheme) const;
    virtual String generatedMIMETypeForURLScheme(const String& URLScheme) const;

    virtual void frameLoadCompleted();
    virtual void saveViewStateToItem(WebCore::HistoryItem*);
    virtual void restoreViewState();
    virtual void provisionalLoadStarted();
    virtual void didFinishLoad();
    virtual void prepareForDataSourceReplacement();

    virtual WTF::PassRefPtr<WebCore::DocumentLoader> createDocumentLoader(const WebCore::ResourceRequest&, const WebCore::SubstituteData&);
    virtual void setTitle(const StringWithDirection&, const KURL&);

    virtual String userAgent(const WebCore::KURL&);

    virtual void savePlatformDataToCachedFrame(WebCore::CachedFrame*);
    virtual void transitionToCommittedFromCachedFrame(WebCore::CachedFrame*);
    virtual void transitionToCommittedForNewPage();

    virtual void didSaveToPageCache();
    virtual void didRestoreFromPageCache();

    virtual void dispatchDidBecomeFrameset(bool);

    virtual bool canCachePage() const;
    virtual void download(WebCore::ResourceHandle*, const WebCore::ResourceRequest&, const WebCore::ResourceRequest&, const WebCore::ResourceResponse&);

    virtual PassRefPtr<Frame> createFrame(const KURL& url, const String& name, HTMLFrameOwnerElement* ownerElement,
                               const String& referrer, bool allowsScrolling, int marginWidth, int marginHeight);
    virtual void didTransferChildFrameToNewDocument(WebCore::Page*);
    virtual void transferLoadingResourceFromPage(unsigned long, WebCore::DocumentLoader*, const WebCore::ResourceRequest&, WebCore::Page*);
    virtual PassRefPtr<Widget> createPlugin(const IntSize&, HTMLPlugInElement*, const KURL&, const Vector<String>&, const Vector<String>&, const String&, bool);
    virtual void redirectDataToPlugin(Widget* pluginWidget);

    virtual PassRefPtr<Widget> createJavaAppletWidget(const IntSize&, HTMLAppletElement*, const KURL& baseURL, const Vector<String>& paramNames, const Vector<String>& paramValues);

    virtual ObjectContentType objectContentType(const KURL&, const String& mimeTypeIn, bool shouldPreferPlugInsForImages);
    virtual String overrideMediaType() const;

    virtual void dispatchDidClearWindowObjectInWorld(DOMWrapperWorld*);
    virtual void documentElementAvailable();
    virtual void didPerformFirstNavigation() const;

#if USE(V8)
    // A frame's V8 context was created or destroyed.
    virtual void didCreateScriptContextForFrame();
    virtual void didDestroyScriptContextForFrame();

    // A context untied to a frame was created (through evaluateInIsolatedWorld).
    // This context is not tied to the lifetime of its frame, and is destroyed
    // in garbage collection.
    virtual void didCreateIsolatedScriptContext();

    // Returns true if we should allow the given V8 extension to be added to
    // the script context at the currently loading page and given extension group.
    virtual bool allowScriptExtension(const String& extensionName, int extensionGroup) { return false; }
#endif

    virtual void registerForIconNotification(bool);

    QString chooseFile(const QString& oldFile);

    virtual PassRefPtr<FrameNetworkingContext> createNetworkingContext();

    const KURL& lastRequestedUrl() const { return m_lastRequestedUrl; }

    static bool dumpFrameLoaderCallbacks;
    static bool dumpProgressFinishedCallback;
    static bool dumpUserGestureInFrameLoaderCallbacks;
    static bool dumpResourceLoadCallbacks;
    static bool dumpResourceResponseMIMETypes;
    static QString dumpResourceLoadCallbacksPath;
    static bool sendRequestReturnsNullOnRedirect;
    static bool sendRequestReturnsNull;
    static QStringList sendRequestClearHeaders;
    static bool policyDelegateEnabled;
    static bool policyDelegatePermissive;
    static bool deferMainResourceDataLoad;
    static bool dumpHistoryCallbacks;
    static QMap<QString, QString> URLsToRedirect;

private slots:
    void onIconLoadedForPageURL(const QString&);

private:
    void emitLoadStarted();
    void emitLoadFinished(bool ok);

    Frame *m_frame;
    QWebFrame *m_webFrame;
    ResourceResponse m_response;

    // Plugin view to redirect data to
    WebCore::PluginView* m_pluginView;
    bool m_hasSentResponseToPlugin;

    // True if makeRepresentation was called.  We don't actually have a concept
    // of a "representation", but we need to know when we're expected to have one.
    // See finishedLoading().
    bool m_hasRepresentation;

    KURL m_lastRequestedUrl;
    bool m_isOriginatingLoad;
};

}

#endif
