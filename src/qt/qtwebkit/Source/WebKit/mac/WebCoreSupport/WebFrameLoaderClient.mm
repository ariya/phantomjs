/*
 * Copyright (C) 2006, 2007, 2008, 2009, 2011, 2012 Apple Inc. All rights reserved.
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

#import "WebFrameLoaderClient.h"

// Terrible hack; lets us get at the WebFrame private structure.
#define private public
#import "WebFrame.h"
#undef private

#import "DOMElementInternal.h"
#import "DOMHTMLFormElementInternal.h"
#import "WebBackForwardList.h"
#import "WebCachedFramePlatformData.h"
#import "WebChromeClient.h"
#import "WebDataSourceInternal.h"
#import "WebDelegateImplementationCaching.h"
#import "WebDocumentInternal.h"
#import "WebDocumentLoaderMac.h"
#import "WebDownloadInternal.h"
#import "WebDynamicScrollBarsViewInternal.h"
#import "WebElementDictionary.h"
#import "WebFormDelegate.h"
#import "WebFrameInternal.h"
#import "WebFrameLoadDelegate.h"
#import "WebFrameNetworkingContext.h"
#import "WebFrameViewInternal.h"
#import "WebHTMLRepresentationPrivate.h"
#import "WebHTMLViewInternal.h"
#import "WebHistoryInternal.h"
#import "WebHistoryItemInternal.h"
#import "WebIconDatabaseInternal.h"
#import "WebKitErrorsPrivate.h"
#import "WebKitLogging.h"
#import "WebKitNSStringExtras.h"
#import "WebNSURLExtras.h"
#import "WebNavigationData.h"
#import "WebNetscapePluginPackage.h"
#import "WebNetscapePluginView.h"
#import "WebPanelAuthenticationHandler.h"
#import "WebPluginController.h"
#import "WebPluginPackage.h"
#import "WebPluginViewFactoryPrivate.h"
#import "WebPolicyDelegate.h"
#import "WebPolicyDelegatePrivate.h"
#import "WebPreferences.h"
#import "WebResourceLoadDelegate.h"
#import "WebScriptWorldInternal.h"
#import "WebSecurityOriginInternal.h"
#import "WebUIDelegate.h"
#import "WebUIDelegatePrivate.h"
#import "WebViewInternal.h"
#import <JavaScriptCore/JSContextInternal.h>
#import <WebCore/AuthenticationCF.h>
#import <WebCore/AuthenticationMac.h>
#import <WebCore/BackForwardController.h>
#import <WebCore/BlockExceptions.h>
#import <WebCore/CachedFrame.h>
#import <WebCore/Chrome.h>
#import <WebCore/Document.h>
#import <WebCore/DocumentLoader.h>
#import <WebCore/EventHandler.h>
#import <WebCore/FocusController.h>
#import <WebCore/FormState.h>
#import <WebCore/Frame.h>
#import <WebCore/FrameLoader.h>
#import <WebCore/FrameLoaderStateMachine.h>
#import <WebCore/FrameLoaderTypes.h>
#import <WebCore/FrameTree.h>
#import <WebCore/FrameView.h>
#import <WebCore/HTMLAppletElement.h>
#import <WebCore/HTMLFormElement.h>
#import <WebCore/HTMLFrameElement.h>
#import <WebCore/HTMLFrameOwnerElement.h>
#import <WebCore/HTMLHeadElement.h>
#import <WebCore/HTMLNames.h>
#import <WebCore/HTMLParserIdioms.h>
#import <WebCore/HTMLPlugInElement.h>
#import <WebCore/HistoryController.h>
#import <WebCore/HistoryItem.h>
#import <WebCore/HitTestResult.h>
#import <WebCore/IconDatabase.h>
#import <WebCore/LoaderNSURLExtras.h>
#import <WebCore/MIMETypeRegistry.h>
#import <WebCore/MouseEvent.h>
#import <WebCore/Page.h>
#import <WebCore/PluginViewBase.h>
#import <WebCore/ProtectionSpace.h>
#import <WebCore/ResourceError.h>
#import <WebCore/ResourceHandle.h>
#import <WebCore/ResourceLoader.h>
#import <WebCore/ResourceRequest.h>
#import <WebCore/RunLoop.h>
#import <WebCore/ScriptController.h>
#import <WebCore/SharedBuffer.h>
#import <WebCore/WebCoreObjCExtras.h>
#import <WebCore/WebScriptObjectPrivate.h>
#import <WebCore/Widget.h>
#import <WebKit/DOMElement.h>
#import <WebKit/DOMHTMLFormElement.h>
#import <WebKitSystemInterface.h>
#import <runtime/InitializeThreading.h>
#import <wtf/MainThread.h>
#import <wtf/PassRefPtr.h>
#import <wtf/text/WTFString.h>

#if ENABLE(PLUGIN_PROXY_FOR_VIDEO)
#import <WebCore/HTMLMediaElement.h>
#endif

#if USE(PLUGIN_HOST_PROCESS) && ENABLE(NETSCAPE_PLUGIN_API)
#import "NetscapePluginHostManager.h"
#import "WebHostedNetscapePluginView.h"
#endif

using namespace WebCore;
using namespace HTMLNames;

// For backwards compatibility with older WebKit plug-ins.
NSString *WebPluginBaseURLKey = @"WebPluginBaseURL";
NSString *WebPluginAttributesKey = @"WebPluginAttributes";
NSString *WebPluginContainerKey = @"WebPluginContainer";

@interface WebFramePolicyListener : NSObject <WebPolicyDecisionListener, WebFormSubmissionListener> {
    RefPtr<Frame> _frame;
    FramePolicyFunction _policyFunction;
}

- (id)initWithFrame:(Frame*)frame policyFunction:(FramePolicyFunction)policyFunction;
- (void)invalidate;

@end

static inline WebDataSource *dataSource(DocumentLoader* loader)
{
    return loader ? static_cast<WebDocumentLoaderMac*>(loader)->dataSource() : nil;
}

// Quirk for the Apple Dictionary application.
//
// If a top level frame has a <script> element in its <head> for a script named MainPageJavaScript.js,
// then for that frame's document, ignore changes to the scrolling attribute of frames. That script
// has a bug in it where it sets the scrolling attribute on frames, and that erroneous scrolling
// attribute needs to be ignored to avoid showing extra scroll bars in the window.
// This quirk can be removed when Apple Dictionary is fixed (see <rdar://problem/6471058>).

static void applyAppleDictionaryApplicationQuirkNonInlinePart(WebFrameLoaderClient* client, const ResourceRequest& request)
{
    if (!request.url().isLocalFile())
        return;
    if (!request.url().string().endsWith("MainPageJavaScript.js"))
        return;
    Frame* frame = core(client->webFrame());
    if (!frame)
        return;
    if (frame->tree()->parent())
        return;
    Document* document = frame->document();
    if (!document)
        return;
    HTMLHeadElement* head = document->head();
    if (!head)
        return;
    for (Node* c = head->firstChild(); c; c = c->nextSibling()) {
        if (c->hasTagName(scriptTag) && toElement(c)->getAttribute(srcAttr) == "MainPageJavaScript.js") {
            document->setFrameElementsShouldIgnoreScrolling(true);
            return;
        }
    }
}

static inline void applyAppleDictionaryApplicationQuirk(WebFrameLoaderClient* client, const ResourceRequest& request)
{
    // Use a one-time-initialized global variable so we can quickly determine there's nothing to do in
    // all applications other than Apple Dictionary.
    static bool isAppleDictionary = [[[NSBundle mainBundle] bundleIdentifier] isEqualToString:@"com.apple.Dictionary"];
    if (isAppleDictionary)
        applyAppleDictionaryApplicationQuirkNonInlinePart(client, request);
}

WebFrameLoaderClient::WebFrameLoaderClient(WebFrame *webFrame)
    : m_webFrame(webFrame)
{
}

void WebFrameLoaderClient::frameLoaderDestroyed()
{
    [m_webFrame.get() _clearCoreFrame];
    delete this;
}

bool WebFrameLoaderClient::hasWebView() const
{
    return [m_webFrame.get() webView] != nil;
}

void WebFrameLoaderClient::makeRepresentation(DocumentLoader* loader)
{
    [dataSource(loader) _makeRepresentation];
}

bool WebFrameLoaderClient::hasHTMLView() const
{
    NSView <WebDocumentView> *view = [m_webFrame->_private->webFrameView documentView];
    return [view isKindOfClass:[WebHTMLView class]];
}

void WebFrameLoaderClient::forceLayout()
{
    NSView <WebDocumentView> *view = [m_webFrame->_private->webFrameView documentView];
    [view setNeedsLayout:YES];
    [view layout];
}

void WebFrameLoaderClient::forceLayoutForNonHTML()
{
    WebFrameView *thisView = m_webFrame->_private->webFrameView;
    NSView <WebDocumentView> *thisDocumentView = [thisView documentView];
    ASSERT(thisDocumentView != nil);
    
    // Tell the just loaded document to layout.  This may be necessary
    // for non-html content that needs a layout message.
    if (!([[m_webFrame.get() _dataSource] _isDocumentHTML])) {
        [thisDocumentView setNeedsLayout:YES];
        [thisDocumentView layout];
        [thisDocumentView setNeedsDisplay:YES];
    }
}

void WebFrameLoaderClient::setCopiesOnScroll()
{
    [[[m_webFrame->_private->webFrameView _scrollView] contentView] setCopiesOnScroll:YES];
}

void WebFrameLoaderClient::detachedFromParent2()
{
    //remove any NetScape plugins that are children of this frame because they are about to be detached
    WebView *webView = getWebView(m_webFrame.get());
    [webView removePluginInstanceViewsFor:(m_webFrame.get())];
    [m_webFrame->_private->webFrameView _setWebFrame:nil]; // needed for now to be compatible w/ old behavior

    WebFrameLoadDelegateImplementationCache* implementations = WebViewGetFrameLoadDelegateImplementations(webView);
    if (implementations->didRemoveFrameFromHierarchyFunc)
        CallFrameLoadDelegate(implementations->didRemoveFrameFromHierarchyFunc, webView, @selector(webView:didRemoveFrameFromHierarchy:), m_webFrame.get());
}

void WebFrameLoaderClient::detachedFromParent3()
{
    [m_webFrame->_private->webFrameView release];
    m_webFrame->_private->webFrameView = nil;
}

void WebFrameLoaderClient::convertMainResourceLoadToDownload(DocumentLoader* documentLoader, const ResourceRequest& request, const ResourceResponse& response)
{
    WebView *webView = getWebView(m_webFrame.get());

    if (!documentLoader->mainResourceLoader()) {
        // The resource has already been cached, start a new download.
        WebDownload *webDownload = [[WebDownload alloc] initWithRequest:request.nsURLRequest(UpdateHTTPBody) delegate:[webView downloadDelegate]];
        [webDownload autorelease];
        return;
    }

    ResourceHandle* handle = documentLoader->mainResourceLoader()->handle();

#if USE(CFNETWORK)
    ASSERT([WebDownload respondsToSelector:@selector(_downloadWithLoadingCFURLConnection:request:response:delegate:proxy:)]);
    CFURLConnectionRef connection = handle->connection();
    [WebDownload _downloadWithLoadingCFURLConnection:connection
                                                                     request:request.cfURLRequest(UpdateHTTPBody)
                                                                    response:response.cfURLResponse()
                                                                    delegate:[webView downloadDelegate]
                                                                       proxy:nil];

    // Release the connection since the NSURLDownload (actually CFURLDownload) will retain the connection and use it.
    handle->releaseConnectionForDownload();
    CFRelease(connection);
#else
    [WebDownload _downloadWithLoadingConnection:handle->connection()
                                                                request:request.nsURLRequest(UpdateHTTPBody)
                                                               response:response.nsURLResponse()
                                                               delegate:[webView downloadDelegate]
                                                                  proxy:nil];
#endif
}

bool WebFrameLoaderClient::dispatchDidLoadResourceFromMemoryCache(DocumentLoader* loader, const ResourceRequest& request, const ResourceResponse& response, int length)
{
    applyAppleDictionaryApplicationQuirk(this, request);

    WebView *webView = getWebView(m_webFrame.get());
    WebResourceDelegateImplementationCache* implementations = WebViewGetResourceLoadDelegateImplementations(webView);
    if (!implementations->didLoadResourceFromMemoryCacheFunc)
        return false;

    CallResourceLoadDelegate(implementations->didLoadResourceFromMemoryCacheFunc, webView, @selector(webView:didLoadResourceFromMemoryCache:response:length:fromDataSource:), request.nsURLRequest(UpdateHTTPBody), response.nsURLResponse(), length, dataSource(loader));
    return true;
}

void WebFrameLoaderClient::assignIdentifierToInitialRequest(unsigned long identifier, DocumentLoader* loader, const ResourceRequest& request)
{
    WebView *webView = getWebView(m_webFrame.get());
    WebResourceDelegateImplementationCache* implementations = WebViewGetResourceLoadDelegateImplementations(webView);

    id object = nil;
    BOOL shouldRelease = NO;
    if (implementations->identifierForRequestFunc)
        object = CallResourceLoadDelegate(implementations->identifierForRequestFunc, webView, @selector(webView:identifierForInitialRequest:fromDataSource:), request.nsURLRequest(UpdateHTTPBody), dataSource(loader));
    else {
        object = [[NSObject alloc] init];
        shouldRelease = YES;
    }

    [webView _addObject:object forIdentifier:identifier];

    if (shouldRelease)
        [object release];
}

void WebFrameLoaderClient::dispatchWillSendRequest(DocumentLoader* loader, unsigned long identifier, ResourceRequest& request, const ResourceResponse& redirectResponse)
{
    applyAppleDictionaryApplicationQuirk(this, request);

    WebView *webView = getWebView(m_webFrame.get());
    WebResourceDelegateImplementationCache* implementations = WebViewGetResourceLoadDelegateImplementations(webView);

    if (redirectResponse.isNull())
        static_cast<WebDocumentLoaderMac*>(loader)->increaseLoadCount(identifier);

    NSURLRequest *currentURLRequest = request.nsURLRequest(UpdateHTTPBody);
    NSURLRequest *newURLRequest = currentURLRequest;
    if (implementations->willSendRequestFunc)
        newURLRequest = (NSURLRequest *)CallResourceLoadDelegate(implementations->willSendRequestFunc, webView, @selector(webView:resource:willSendRequest:redirectResponse:fromDataSource:), [webView _objectForIdentifier:identifier], currentURLRequest, redirectResponse.nsURLResponse(), dataSource(loader));

    if (newURLRequest != currentURLRequest)
        request = newURLRequest;
}

bool WebFrameLoaderClient::shouldUseCredentialStorage(DocumentLoader* loader, unsigned long identifier)
{
    WebView *webView = getWebView(m_webFrame.get());
    WebResourceDelegateImplementationCache* implementations = WebViewGetResourceLoadDelegateImplementations(webView);

    if (implementations->shouldUseCredentialStorageFunc) {
        if (id resource = [webView _objectForIdentifier:identifier])
            return CallResourceLoadDelegateReturningBoolean(NO, implementations->shouldUseCredentialStorageFunc, webView, @selector(webView:resource:shouldUseCredentialStorageForDataSource:), resource, dataSource(loader));
    }

    return true;
}

void WebFrameLoaderClient::dispatchDidReceiveAuthenticationChallenge(DocumentLoader* loader, unsigned long identifier, const AuthenticationChallenge& challenge)
{
    WebView *webView = getWebView(m_webFrame.get());
    WebResourceDelegateImplementationCache* implementations = WebViewGetResourceLoadDelegateImplementations(webView);

    NSURLAuthenticationChallenge *webChallenge = mac(challenge);

    if (implementations->didReceiveAuthenticationChallengeFunc) {
        if (id resource = [webView _objectForIdentifier:identifier]) {
            CallResourceLoadDelegate(implementations->didReceiveAuthenticationChallengeFunc, webView, @selector(webView:resource:didReceiveAuthenticationChallenge:fromDataSource:), resource, webChallenge, dataSource(loader));
            return;
        }
    }

    NSWindow *window = [webView hostWindow] ? [webView hostWindow] : [webView window];
    [[WebPanelAuthenticationHandler sharedHandler] startAuthentication:webChallenge window:window];
}

#if USE(PROTECTION_SPACE_AUTH_CALLBACK)
bool WebFrameLoaderClient::canAuthenticateAgainstProtectionSpace(DocumentLoader* loader, unsigned long identifier, const ProtectionSpace& protectionSpace)
{
    WebView *webView = getWebView(m_webFrame.get());
    WebResourceDelegateImplementationCache* implementations = WebViewGetResourceLoadDelegateImplementations(webView);
    
    NSURLProtectionSpace *webProtectionSpace = mac(protectionSpace);
    
    if (implementations->canAuthenticateAgainstProtectionSpaceFunc) {
        if (id resource = [webView _objectForIdentifier:identifier]) {
            return CallResourceLoadDelegateReturningBoolean(NO, implementations->canAuthenticateAgainstProtectionSpaceFunc, webView, @selector(webView:resource:canAuthenticateAgainstProtectionSpace:forDataSource:), resource, webProtectionSpace, dataSource(loader));
        }
    }

    // If our resource load delegate doesn't handle the question, then only send authentication
    // challenges for pre-10.6 protection spaces.  This is the same as the default implementation
    // in CFNetwork.
    return (protectionSpace.authenticationScheme() < ProtectionSpaceAuthenticationSchemeClientCertificateRequested);
}
#endif

bool WebFrameLoaderClient::shouldPaintBrokenImage(const KURL& imageURL) const
{
    WebView *webView = getWebView(m_webFrame.get());
    WebResourceDelegateImplementationCache* implementations = WebViewGetResourceLoadDelegateImplementations(webView);

    if (implementations->shouldPaintBrokenImageForURLFunc) {
        NSURL* url = imageURL;
        return CallResourceLoadDelegateReturningBoolean(YES, implementations->shouldPaintBrokenImageForURLFunc, webView, @selector(webView:shouldPaintBrokenImageForURL:), url);
    }
    return true;
}

void WebFrameLoaderClient::dispatchDidCancelAuthenticationChallenge(DocumentLoader* loader, unsigned long identifier, const AuthenticationChallenge&challenge)
{
    WebView *webView = getWebView(m_webFrame.get());
    WebResourceDelegateImplementationCache* implementations = WebViewGetResourceLoadDelegateImplementations(webView);
    NSURLAuthenticationChallenge *webChallenge = mac(challenge);

    if (implementations->didCancelAuthenticationChallengeFunc) {
        if (id resource = [webView _objectForIdentifier:identifier]) {
            CallResourceLoadDelegate(implementations->didCancelAuthenticationChallengeFunc, webView, @selector(webView:resource:didCancelAuthenticationChallenge:fromDataSource:), resource, webChallenge, dataSource(loader));
            return;
        }
    }

    [(WebPanelAuthenticationHandler *)[WebPanelAuthenticationHandler sharedHandler] cancelAuthentication:webChallenge];
}

void WebFrameLoaderClient::dispatchDidReceiveResponse(DocumentLoader* loader, unsigned long identifier, const ResourceResponse& response)
{
    WebView *webView = getWebView(m_webFrame.get());
    WebResourceDelegateImplementationCache* implementations = WebViewGetResourceLoadDelegateImplementations(webView);
    if (implementations->didReceiveResponseFunc) {
        if (id resource = [webView _objectForIdentifier:identifier])
            CallResourceLoadDelegate(implementations->didReceiveResponseFunc, webView, @selector(webView:resource:didReceiveResponse:fromDataSource:), resource, response.nsURLResponse(), dataSource(loader));
    }
}

NSCachedURLResponse* WebFrameLoaderClient::willCacheResponse(DocumentLoader* loader, unsigned long identifier, NSCachedURLResponse* response) const
{
    WebView *webView = getWebView(m_webFrame.get());
    WebResourceDelegateImplementationCache* implementations = WebViewGetResourceLoadDelegateImplementations(webView);

    if (implementations->willCacheResponseFunc) {
        if (id resource = [webView _objectForIdentifier:identifier])
            return CallResourceLoadDelegate(implementations->willCacheResponseFunc, webView, @selector(webView:resource:willCacheResponse:fromDataSource:), resource, response, dataSource(loader));
    }

    return response;
}

void WebFrameLoaderClient::dispatchDidReceiveContentLength(DocumentLoader* loader, unsigned long identifier, int dataLength)
{
    WebView *webView = getWebView(m_webFrame.get());
    WebResourceDelegateImplementationCache* implementations = WebViewGetResourceLoadDelegateImplementations(webView);
    if (implementations->didReceiveContentLengthFunc) {
        if (id resource = [webView _objectForIdentifier:identifier])
            CallResourceLoadDelegate(implementations->didReceiveContentLengthFunc, webView, @selector(webView:resource:didReceiveContentLength:fromDataSource:), resource, (NSInteger)dataLength, dataSource(loader));
    }
}

void WebFrameLoaderClient::dispatchDidFinishLoading(DocumentLoader* loader, unsigned long identifier)
{
    WebView *webView = getWebView(m_webFrame.get());
    WebResourceDelegateImplementationCache* implementations = WebViewGetResourceLoadDelegateImplementations(webView);

    if (implementations->didFinishLoadingFromDataSourceFunc) {
        if (id resource = [webView _objectForIdentifier:identifier])
            CallResourceLoadDelegate(implementations->didFinishLoadingFromDataSourceFunc, webView, @selector(webView:resource:didFinishLoadingFromDataSource:), resource, dataSource(loader));
    }

    [webView _removeObjectForIdentifier:identifier];

    static_cast<WebDocumentLoaderMac*>(loader)->decreaseLoadCount(identifier);
}

void WebFrameLoaderClient::dispatchDidFailLoading(DocumentLoader* loader, unsigned long identifier, const ResourceError& error)
{
    WebView *webView = getWebView(m_webFrame.get());
    WebResourceDelegateImplementationCache* implementations = WebViewGetResourceLoadDelegateImplementations(webView);

    if (implementations->didFailLoadingWithErrorFromDataSourceFunc) {
        if (id resource = [webView _objectForIdentifier:identifier])
            CallResourceLoadDelegate(implementations->didFailLoadingWithErrorFromDataSourceFunc, webView, @selector(webView:resource:didFailLoadingWithError:fromDataSource:), resource, (NSError *)error, dataSource(loader));
    }

    [webView _removeObjectForIdentifier:identifier];

    static_cast<WebDocumentLoaderMac*>(loader)->decreaseLoadCount(identifier);
}

void WebFrameLoaderClient::dispatchDidHandleOnloadEvents()
{
    WebView *webView = getWebView(m_webFrame.get());
    WebFrameLoadDelegateImplementationCache* implementations = WebViewGetFrameLoadDelegateImplementations(webView);
    if (implementations->didHandleOnloadEventsForFrameFunc)
        CallFrameLoadDelegate(implementations->didHandleOnloadEventsForFrameFunc, webView, @selector(webView:didHandleOnloadEventsForFrame:), m_webFrame.get());
}

void WebFrameLoaderClient::dispatchDidReceiveServerRedirectForProvisionalLoad()
{
    m_webFrame->_private->provisionalURL = core(m_webFrame.get())->loader()->provisionalDocumentLoader()->url().string();

    WebView *webView = getWebView(m_webFrame.get());
    WebFrameLoadDelegateImplementationCache* implementations = WebViewGetFrameLoadDelegateImplementations(webView);
    if (implementations->didReceiveServerRedirectForProvisionalLoadForFrameFunc)
        CallFrameLoadDelegate(implementations->didReceiveServerRedirectForProvisionalLoadForFrameFunc, webView, @selector(webView:didReceiveServerRedirectForProvisionalLoadForFrame:), m_webFrame.get());
}

void WebFrameLoaderClient::dispatchDidCancelClientRedirect()
{
    WebView *webView = getWebView(m_webFrame.get());
    WebFrameLoadDelegateImplementationCache* implementations = WebViewGetFrameLoadDelegateImplementations(webView);
    if (implementations->didCancelClientRedirectForFrameFunc)
        CallFrameLoadDelegate(implementations->didCancelClientRedirectForFrameFunc, webView, @selector(webView:didCancelClientRedirectForFrame:), m_webFrame.get());
}

void WebFrameLoaderClient::dispatchWillPerformClientRedirect(const KURL& url, double delay, double fireDate)
{
    WebView *webView = getWebView(m_webFrame.get());
    WebFrameLoadDelegateImplementationCache* implementations = WebViewGetFrameLoadDelegateImplementations(webView);
    if (implementations->willPerformClientRedirectToURLDelayFireDateForFrameFunc) {
        NSURL *cocoaURL = url;
        CallFrameLoadDelegate(implementations->willPerformClientRedirectToURLDelayFireDateForFrameFunc, webView, @selector(webView:willPerformClientRedirectToURL:delay:fireDate:forFrame:), cocoaURL, delay, [NSDate dateWithTimeIntervalSince1970:fireDate], m_webFrame.get());
    }
}

void WebFrameLoaderClient::dispatchDidChangeLocationWithinPage()
{
    m_webFrame->_private->url = core(m_webFrame.get())->document()->url().string();

    WebView *webView = getWebView(m_webFrame.get());
    WebFrameLoadDelegateImplementationCache* implementations = WebViewGetFrameLoadDelegateImplementations(webView);
    if (implementations->didChangeLocationWithinPageForFrameFunc)
        CallFrameLoadDelegate(implementations->didChangeLocationWithinPageForFrameFunc, webView, @selector(webView:didChangeLocationWithinPageForFrame:), m_webFrame.get());
}

void WebFrameLoaderClient::dispatchDidPushStateWithinPage()
{
    m_webFrame->_private->url = core(m_webFrame.get())->document()->url().string();

    WebView *webView = getWebView(m_webFrame.get());
    WebFrameLoadDelegateImplementationCache* implementations = WebViewGetFrameLoadDelegateImplementations(webView);
    if (implementations->didPushStateWithinPageForFrameFunc)
        CallFrameLoadDelegate(implementations->didPushStateWithinPageForFrameFunc, webView, @selector(webView:didPushStateWithinPageForFrame:), m_webFrame.get());
}

void WebFrameLoaderClient::dispatchDidReplaceStateWithinPage()
{
    m_webFrame->_private->url = core(m_webFrame.get())->document()->url().string();

    WebView *webView = getWebView(m_webFrame.get());
    WebFrameLoadDelegateImplementationCache* implementations = WebViewGetFrameLoadDelegateImplementations(webView);
    if (implementations->didReplaceStateWithinPageForFrameFunc)
        CallFrameLoadDelegate(implementations->didReplaceStateWithinPageForFrameFunc, webView, @selector(webView:didReplaceStateWithinPageForFrame:), m_webFrame.get());
}

void WebFrameLoaderClient::dispatchDidPopStateWithinPage()
{
    m_webFrame->_private->url = core(m_webFrame.get())->document()->url().string();

    WebView *webView = getWebView(m_webFrame.get());
    WebFrameLoadDelegateImplementationCache* implementations = WebViewGetFrameLoadDelegateImplementations(webView);
    if (implementations->didPopStateWithinPageForFrameFunc)
        CallFrameLoadDelegate(implementations->didPopStateWithinPageForFrameFunc, webView, @selector(webView:didPopStateWithinPageForFrame:), m_webFrame.get());
}

void WebFrameLoaderClient::dispatchWillClose()
{
    WebView *webView = getWebView(m_webFrame.get());   
    WebFrameLoadDelegateImplementationCache* implementations = WebViewGetFrameLoadDelegateImplementations(webView);
    if (implementations->willCloseFrameFunc)
        CallFrameLoadDelegate(implementations->willCloseFrameFunc, webView, @selector(webView:willCloseFrame:), m_webFrame.get());
}

void WebFrameLoaderClient::dispatchDidReceiveIcon()
{
#if ENABLE(ICONDATABASE)
    WebView *webView = getWebView(m_webFrame.get());   
    ASSERT(m_webFrame == [webView mainFrame]);
    [webView _dispatchDidReceiveIconFromWebFrame:m_webFrame.get()];
#endif
}

void WebFrameLoaderClient::dispatchDidStartProvisionalLoad()
{
    ASSERT(!m_webFrame->_private->provisionalURL);
    m_webFrame->_private->provisionalURL = core(m_webFrame.get())->loader()->provisionalDocumentLoader()->url().string();

    WebView *webView = getWebView(m_webFrame.get());
    [webView _didStartProvisionalLoadForFrame:m_webFrame.get()];

    WebFrameLoadDelegateImplementationCache* implementations = WebViewGetFrameLoadDelegateImplementations(webView);
    if (implementations->didStartProvisionalLoadForFrameFunc)
        CallFrameLoadDelegate(implementations->didStartProvisionalLoadForFrameFunc, webView, @selector(webView:didStartProvisionalLoadForFrame:), m_webFrame.get());
}

void WebFrameLoaderClient::dispatchDidReceiveTitle(const StringWithDirection& title)
{
    WebView *webView = getWebView(m_webFrame.get());   
    WebFrameLoadDelegateImplementationCache* implementations = WebViewGetFrameLoadDelegateImplementations(webView);
    if (implementations->didReceiveTitleForFrameFunc)
        // FIXME: use direction of title.
        CallFrameLoadDelegate(implementations->didReceiveTitleForFrameFunc, webView, @selector(webView:didReceiveTitle:forFrame:), (NSString *)title.string(), m_webFrame.get());
}

void WebFrameLoaderClient::dispatchDidChangeIcons(WebCore::IconType)
{
     // FIXME: Implement this to allow container to update favicon.
}

void WebFrameLoaderClient::dispatchDidCommitLoad()
{
    // Tell the client we've committed this URL.
    ASSERT([m_webFrame->_private->webFrameView documentView] != nil);
    
    WebView *webView = getWebView(m_webFrame.get());   
    [webView _didCommitLoadForFrame:m_webFrame.get()];

    m_webFrame->_private->url = m_webFrame->_private->provisionalURL;
    m_webFrame->_private->provisionalURL = nullptr;

    WebFrameLoadDelegateImplementationCache* implementations = WebViewGetFrameLoadDelegateImplementations(webView);
    if (implementations->didCommitLoadForFrameFunc)
        CallFrameLoadDelegate(implementations->didCommitLoadForFrameFunc, webView, @selector(webView:didCommitLoadForFrame:), m_webFrame.get());
}

void WebFrameLoaderClient::dispatchDidFailProvisionalLoad(const ResourceError& error)
{
    m_webFrame->_private->provisionalURL = nullptr;

    WebView *webView = getWebView(m_webFrame.get());
    [webView _didFailProvisionalLoadWithError:error forFrame:m_webFrame.get()];

    WebFrameLoadDelegateImplementationCache* implementations = WebViewGetFrameLoadDelegateImplementations(webView);
    if (implementations->didFailProvisionalLoadWithErrorForFrameFunc)
        CallFrameLoadDelegate(implementations->didFailProvisionalLoadWithErrorForFrameFunc, webView, @selector(webView:didFailProvisionalLoadWithError:forFrame:), (NSError *)error, m_webFrame.get());

    [m_webFrame->_private->internalLoadDelegate webFrame:m_webFrame.get() didFinishLoadWithError:error];
}

void WebFrameLoaderClient::dispatchDidFailLoad(const ResourceError& error)
{
    ASSERT(!m_webFrame->_private->provisionalURL);

    WebView *webView = getWebView(m_webFrame.get());
    [webView _didFailLoadWithError:error forFrame:m_webFrame.get()];

    WebFrameLoadDelegateImplementationCache* implementations = WebViewGetFrameLoadDelegateImplementations(webView);
    if (implementations->didFailLoadWithErrorForFrameFunc)
        CallFrameLoadDelegate(implementations->didFailLoadWithErrorForFrameFunc, webView, @selector(webView:didFailLoadWithError:forFrame:), (NSError *)error, m_webFrame.get());

    [m_webFrame->_private->internalLoadDelegate webFrame:m_webFrame.get() didFinishLoadWithError:error];
}

void WebFrameLoaderClient::dispatchDidFinishDocumentLoad()
{
    WebView *webView = getWebView(m_webFrame.get());
    WebFrameLoadDelegateImplementationCache* implementations = WebViewGetFrameLoadDelegateImplementations(webView);
    if (implementations->didFinishDocumentLoadForFrameFunc)
        CallFrameLoadDelegate(implementations->didFinishDocumentLoadForFrameFunc, webView, @selector(webView:didFinishDocumentLoadForFrame:), m_webFrame.get());
}

void WebFrameLoaderClient::dispatchDidFinishLoad()
{
    ASSERT(!m_webFrame->_private->provisionalURL);

    WebView *webView = getWebView(m_webFrame.get());
    [webView _didFinishLoadForFrame:m_webFrame.get()];

    WebFrameLoadDelegateImplementationCache* implementations = WebViewGetFrameLoadDelegateImplementations(webView);
    if (implementations->didFinishLoadForFrameFunc)
        CallFrameLoadDelegate(implementations->didFinishLoadForFrameFunc, webView, @selector(webView:didFinishLoadForFrame:), m_webFrame.get());

    [m_webFrame->_private->internalLoadDelegate webFrame:m_webFrame.get() didFinishLoadWithError:nil];
}

void WebFrameLoaderClient::dispatchDidLayout(LayoutMilestones milestones)
{
    WebView *webView = getWebView(m_webFrame.get());
    WebFrameLoadDelegateImplementationCache* implementations = WebViewGetFrameLoadDelegateImplementations(webView);

    if (implementations->didLayoutFunc)
        CallFrameLoadDelegate(implementations->didLayoutFunc, webView, @selector(webView:didLayout:), kitLayoutMilestones(milestones));

    if (milestones & DidFirstLayout) {
        // FIXME: We should consider removing the old didFirstLayout API since this is doing double duty with the
        // new didLayout API.
        if (implementations->didFirstLayoutInFrameFunc)
            CallFrameLoadDelegate(implementations->didFirstLayoutInFrameFunc, webView, @selector(webView:didFirstLayoutInFrame:), m_webFrame.get());

        // See WebFrameLoaderClient::provisionalLoadStarted.
        WebDynamicScrollBarsView *scrollView = [m_webFrame->_private->webFrameView _scrollView];
        if ([getWebView(m_webFrame.get()) drawsBackground])
            [scrollView setDrawsBackground:YES];
#if __MAC_OS_X_VERSION_MIN_REQUIRED >= 1070
        [scrollView setVerticalScrollElasticity:NSScrollElasticityAutomatic];
        [scrollView setHorizontalScrollElasticity:NSScrollElasticityAutomatic];
#endif
    }

    if (milestones & DidFirstVisuallyNonEmptyLayout) {
        // FIXME: We should consider removing the old didFirstVisuallyNonEmptyLayoutForFrame API since this is doing
        // double duty with the new didLayout API.
        if (implementations->didFirstVisuallyNonEmptyLayoutInFrameFunc)
            CallFrameLoadDelegate(implementations->didFirstVisuallyNonEmptyLayoutInFrameFunc, webView, @selector(webView:didFirstVisuallyNonEmptyLayoutInFrame:), m_webFrame.get());
    }
}

Frame* WebFrameLoaderClient::dispatchCreatePage(const NavigationAction&)
{
    WebView *currentWebView = getWebView(m_webFrame.get());
    NSDictionary *features = [[NSDictionary alloc] init];
    WebView *newWebView = [[currentWebView _UIDelegateForwarder] webView:currentWebView 
                                                createWebViewWithRequest:nil
                                                          windowFeatures:features];
    [features release];
    
#if USE(PLUGIN_HOST_PROCESS) && ENABLE(NETSCAPE_PLUGIN_API)
    if (newWebView)
        WebKit::NetscapePluginHostManager::shared().didCreateWindow();
#endif
        
    return core([newWebView mainFrame]);
}

void WebFrameLoaderClient::dispatchShow()
{
    WebView *webView = getWebView(m_webFrame.get());
    [[webView _UIDelegateForwarder] webViewShow:webView];
}

void WebFrameLoaderClient::dispatchDecidePolicyForResponse(FramePolicyFunction function,
    const ResourceResponse& response, const ResourceRequest& request)
{
    WebView *webView = getWebView(m_webFrame.get());

    [[webView _policyDelegateForwarder] webView:webView
                        decidePolicyForMIMEType:response.mimeType()
                                        request:request.nsURLRequest(UpdateHTTPBody)
                                          frame:m_webFrame.get()
                               decisionListener:setUpPolicyListener(function).get()];
}

void WebFrameLoaderClient::dispatchDecidePolicyForNewWindowAction(FramePolicyFunction function,
    const NavigationAction& action, const ResourceRequest& request, PassRefPtr<FormState> formState, const String& frameName)
{
    WebView *webView = getWebView(m_webFrame.get());
    [[webView _policyDelegateForwarder] webView:webView
            decidePolicyForNewWindowAction:actionDictionary(action, formState)
                                   request:request.nsURLRequest(UpdateHTTPBody)
                              newFrameName:frameName
                          decisionListener:setUpPolicyListener(function).get()];
}

void WebFrameLoaderClient::dispatchDecidePolicyForNavigationAction(FramePolicyFunction function,
    const NavigationAction& action, const ResourceRequest& request, PassRefPtr<FormState> formState)
{
    WebView *webView = getWebView(m_webFrame.get());
    [[webView _policyDelegateForwarder] webView:webView
                decidePolicyForNavigationAction:actionDictionary(action, formState)
                                        request:request.nsURLRequest(UpdateHTTPBody)
                                          frame:m_webFrame.get()
                               decisionListener:setUpPolicyListener(function).get()];
}

void WebFrameLoaderClient::cancelPolicyCheck()
{
    [m_policyListener invalidate];
    m_policyListener = nullptr;
}

void WebFrameLoaderClient::dispatchUnableToImplementPolicy(const ResourceError& error)
{
    WebView *webView = getWebView(m_webFrame.get());
    [[webView _policyDelegateForwarder] webView:webView unableToImplementPolicyWithError:error frame:m_webFrame.get()];    
}

static NSDictionary *makeFormFieldValuesDictionary(FormState* formState)
{
    const StringPairVector& textFieldValues = formState->textFieldValues();
    size_t size = textFieldValues.size();
    NSMutableDictionary *dictionary = [[NSMutableDictionary alloc] initWithCapacity:size];
    for (size_t i = 0; i < size; ++i)
        [dictionary setObject:textFieldValues[i].second forKey:textFieldValues[i].first];

    return [dictionary autorelease];
}

void WebFrameLoaderClient::dispatchWillSendSubmitEvent(PassRefPtr<WebCore::FormState> formState)
{
    id <WebFormDelegate> formDelegate = [getWebView(m_webFrame.get()) _formDelegate];
    if (!formDelegate)
        return;

    DOMHTMLFormElement *formElement = kit(formState->form());
    NSDictionary *values = makeFormFieldValuesDictionary(formState.get());
    CallFormDelegate(getWebView(m_webFrame.get()), @selector(willSendSubmitEventToForm:inFrame:withValues:), formElement, m_webFrame.get(), values);
}

void WebFrameLoaderClient::dispatchWillSubmitForm(FramePolicyFunction function, PassRefPtr<FormState> formState)
{
    id <WebFormDelegate> formDelegate = [getWebView(m_webFrame.get()) _formDelegate];
    if (!formDelegate) {
        (core(m_webFrame.get())->loader()->policyChecker()->*function)(PolicyUse);
        return;
    }

    NSDictionary *values = makeFormFieldValuesDictionary(formState.get());
    CallFormDelegate(getWebView(m_webFrame.get()), @selector(frame:sourceFrame:willSubmitForm:withValues:submissionListener:), m_webFrame.get(), kit(formState->sourceDocument()->frame()), kit(formState->form()), values, setUpPolicyListener(function).get());
}

void WebFrameLoaderClient::revertToProvisionalState(DocumentLoader* loader)
{
    [dataSource(loader) _revertToProvisionalState];
}

void WebFrameLoaderClient::setMainDocumentError(DocumentLoader* loader, const ResourceError& error)
{
    [dataSource(loader) _setMainDocumentError:error];
}

void WebFrameLoaderClient::willChangeEstimatedProgress()
{
    [getWebView(m_webFrame.get()) _willChangeValueForKey:_WebEstimatedProgressKey];
}

void WebFrameLoaderClient::didChangeEstimatedProgress()
{
    [getWebView(m_webFrame.get()) _didChangeValueForKey:_WebEstimatedProgressKey];
}

void WebFrameLoaderClient::postProgressStartedNotification()
{
    [[NSNotificationCenter defaultCenter] postNotificationName:WebViewProgressStartedNotification object:getWebView(m_webFrame.get())];
}

void WebFrameLoaderClient::postProgressEstimateChangedNotification()
{
    [[NSNotificationCenter defaultCenter] postNotificationName:WebViewProgressEstimateChangedNotification object:getWebView(m_webFrame.get())];
}

void WebFrameLoaderClient::postProgressFinishedNotification()
{
    [[NSNotificationCenter defaultCenter] postNotificationName:WebViewProgressFinishedNotification object:getWebView(m_webFrame.get())];
}

void WebFrameLoaderClient::setMainFrameDocumentReady(bool ready)
{
    [getWebView(m_webFrame.get()) setMainFrameDocumentReady:ready];
}

void WebFrameLoaderClient::startDownload(const ResourceRequest& request, const String& /* suggestedName */)
{
    // FIXME: Should download full request.
    [getWebView(m_webFrame.get()) _downloadURL:request.url()];
}

void WebFrameLoaderClient::willChangeTitle(DocumentLoader* loader)
{
    // FIXME: Should do this only in main frame case, right?
    [getWebView(m_webFrame.get()) _willChangeValueForKey:_WebMainFrameTitleKey];
}

void WebFrameLoaderClient::didChangeTitle(DocumentLoader* loader)
{
    // FIXME: Should do this only in main frame case, right?
    [getWebView(m_webFrame.get()) _didChangeValueForKey:_WebMainFrameTitleKey];
}

void WebFrameLoaderClient::committedLoad(DocumentLoader* loader, const char* data, int length)
{
    NSData *nsData = [[NSData alloc] initWithBytesNoCopy:(void*)data length:length freeWhenDone:NO];
    [dataSource(loader) _receivedData:nsData];
    [nsData release];
}

void WebFrameLoaderClient::finishedLoading(DocumentLoader* loader)
{
    [dataSource(loader) _finishedLoading];
}

static inline NSString *nilOrNSString(const String& string)
{
    if (string.isNull())
        return nil;
    return string;
}

void WebFrameLoaderClient::updateGlobalHistory()
{
    WebView* view = getWebView(m_webFrame.get());
    DocumentLoader* loader = core(m_webFrame.get())->loader()->documentLoader();

    if ([view historyDelegate]) {
        WebHistoryDelegateImplementationCache* implementations = WebViewGetHistoryDelegateImplementations(view);
        if (implementations->navigatedFunc) {
            WebNavigationData *data = [[WebNavigationData alloc] initWithURLString:loader->url()
                                                                             title:nilOrNSString(loader->title().string())
                                                                   originalRequest:loader->originalRequestCopy().nsURLRequest(UpdateHTTPBody)
                                                                          response:loader->response().nsURLResponse()
                                                                 hasSubstituteData:loader->substituteData().isValid()
                                                              clientRedirectSource:loader->clientRedirectSourceForHistory()];

            CallHistoryDelegate(implementations->navigatedFunc, view, @selector(webView:didNavigateWithNavigationData:inFrame:), data, m_webFrame.get());
            [data release];
        }
    
        return;
    }

    [[WebHistory optionalSharedHistory] _visitedURL:loader->urlForHistory() 
                                          withTitle:loader->title().string()
                                             method:loader->originalRequestCopy().httpMethod()
                                         wasFailure:loader->urlForHistoryReflectsFailure()
                                 increaseVisitCount:!loader->clientRedirectSourceForHistory()]; // Do not increase visit count due to navigations that were not initiated by the user directly, avoiding growth from programmatic reloads.
}

void WebFrameLoaderClient::updateGlobalHistoryRedirectLinks()
{
    WebView* view = getWebView(m_webFrame.get());
    WebHistoryDelegateImplementationCache* implementations = [view historyDelegate] ? WebViewGetHistoryDelegateImplementations(view) : 0;
    
    DocumentLoader* loader = core(m_webFrame.get())->loader()->documentLoader();
    ASSERT(loader->unreachableURL().isEmpty());

    if (!loader->clientRedirectSourceForHistory().isNull()) {
        if (implementations) {
            if (implementations->clientRedirectFunc) {
                CallHistoryDelegate(implementations->clientRedirectFunc, view, @selector(webView:didPerformClientRedirectFromURL:toURL:inFrame:), 
                    m_webFrame->_private->url.get(), loader->clientRedirectDestinationForHistory(), m_webFrame.get());
            }
        } else if (WebHistoryItem *item = [[WebHistory optionalSharedHistory] _itemForURLString:loader->clientRedirectSourceForHistory()])
            core(item)->addRedirectURL(loader->clientRedirectDestinationForHistory());
    }

    if (!loader->serverRedirectSourceForHistory().isNull()) {
        if (implementations) {
            if (implementations->serverRedirectFunc) {
                CallHistoryDelegate(implementations->serverRedirectFunc, view, @selector(webView:didPerformServerRedirectFromURL:toURL:inFrame:), 
                    loader->serverRedirectSourceForHistory(), loader->serverRedirectDestinationForHistory(), m_webFrame.get());
            }
        } else if (WebHistoryItem *item = [[WebHistory optionalSharedHistory] _itemForURLString:loader->serverRedirectSourceForHistory()])
            core(item)->addRedirectURL(loader->serverRedirectDestinationForHistory());
    }
}

bool WebFrameLoaderClient::shouldGoToHistoryItem(HistoryItem* item) const
{
    WebView* view = getWebView(m_webFrame.get());
    WebHistoryItem *webItem = kit(item);
    
    return [[view _policyDelegateForwarder] webView:view shouldGoToHistoryItem:webItem];
}

bool WebFrameLoaderClient::shouldStopLoadingForHistoryItem(HistoryItem* item) const
{
    return true;
}

void WebFrameLoaderClient::updateGlobalHistoryItemForPage()
{
    HistoryItem* historyItem = 0;

    if (Page* page = core(m_webFrame.get())->page()) {
        if (!page->settings()->privateBrowsingEnabled())
            historyItem = page->backForward()->currentItem();
    }

    WebView *webView = getWebView(m_webFrame.get());
    [webView _setGlobalHistoryItem:historyItem];
}

void WebFrameLoaderClient::didDisplayInsecureContent()
{
    WebView *webView = getWebView(m_webFrame.get());   
    WebFrameLoadDelegateImplementationCache* implementations = WebViewGetFrameLoadDelegateImplementations(webView);
    if (implementations->didDisplayInsecureContentFunc)
        CallFrameLoadDelegate(implementations->didDisplayInsecureContentFunc, webView, @selector(webViewDidDisplayInsecureContent:));
}

void WebFrameLoaderClient::didRunInsecureContent(SecurityOrigin* origin, const KURL& insecureURL)
{
    WebView *webView = getWebView(m_webFrame.get());   
    WebFrameLoadDelegateImplementationCache* implementations = WebViewGetFrameLoadDelegateImplementations(webView);
    if (implementations->didRunInsecureContentFunc) {
        RetainPtr<WebSecurityOrigin> webSecurityOrigin = adoptNS([[WebSecurityOrigin alloc] _initWithWebCoreSecurityOrigin:origin]);
        CallFrameLoadDelegate(implementations->didRunInsecureContentFunc, webView, @selector(webView:didRunInsecureContent:), webSecurityOrigin.get());
    }
}

void WebFrameLoaderClient::didDetectXSS(const KURL& insecureURL, bool didBlockEntirePage)
{
    WebView *webView = getWebView(m_webFrame.get());   
    WebFrameLoadDelegateImplementationCache* implementations = WebViewGetFrameLoadDelegateImplementations(webView);
    if (implementations->didDetectXSSFunc) {
        // FIXME: must pass didBlockEntirePage if we want to do more on mac than just pass tests.
        NSURL* insecureNSURL = insecureURL;
        CallFrameLoadDelegate(implementations->didDetectXSSFunc, webView, @selector(webView:didDetectXSS:), insecureNSURL);
    }
}

ResourceError WebFrameLoaderClient::cancelledError(const ResourceRequest& request)
{
    return [NSError _webKitErrorWithDomain:NSURLErrorDomain code:NSURLErrorCancelled URL:request.url()];
}
    
ResourceError WebFrameLoaderClient::blockedError(const ResourceRequest& request)
{
    return [NSError _webKitErrorWithDomain:WebKitErrorDomain code:WebKitErrorCannotUseRestrictedPort URL:request.url()];
}

ResourceError WebFrameLoaderClient::cannotShowURLError(const ResourceRequest& request)
{
    return [NSError _webKitErrorWithDomain:WebKitErrorDomain code:WebKitErrorCannotShowURL URL:request.url()];
}

ResourceError WebFrameLoaderClient::interruptedForPolicyChangeError(const ResourceRequest& request)
{
    return [NSError _webKitErrorWithDomain:WebKitErrorDomain code:WebKitErrorFrameLoadInterruptedByPolicyChange URL:request.url()];
}

ResourceError WebFrameLoaderClient::cannotShowMIMETypeError(const ResourceResponse& response)
{
    return [NSError _webKitErrorWithDomain:NSURLErrorDomain code:WebKitErrorCannotShowMIMEType URL:response.url()];
}

ResourceError WebFrameLoaderClient::fileDoesNotExistError(const ResourceResponse& response)
{
    return [NSError _webKitErrorWithDomain:NSURLErrorDomain code:NSURLErrorFileDoesNotExist URL:response.url()];    
}

ResourceError WebFrameLoaderClient::pluginWillHandleLoadError(const ResourceResponse& response)
{
    NSError *error = [[NSError alloc] _initWithPluginErrorCode:WebKitErrorPlugInWillHandleLoad
                                                    contentURL:response.url()
                                                 pluginPageURL:nil
                                                    pluginName:nil
                                                      MIMEType:response.mimeType()];
    return [error autorelease];
}

bool WebFrameLoaderClient::shouldFallBack(const ResourceError& error)
{
    // FIXME: Needs to check domain.
    // FIXME: WebKitErrorPlugInWillHandleLoad is a workaround for the cancel we do to prevent
    // loading plugin content twice.  See <rdar://problem/4258008>
    return error.errorCode() != NSURLErrorCancelled && error.errorCode() != WebKitErrorPlugInWillHandleLoad;
}

bool WebFrameLoaderClient::canHandleRequest(const ResourceRequest& request) const
{
    Frame* frame = core(m_webFrame.get());
    Page* page = frame->page();
    BOOL forMainFrame = page && page->mainFrame() == frame;
    return [WebView _canHandleRequest:request.nsURLRequest(UpdateHTTPBody) forMainFrame:forMainFrame];
}

bool WebFrameLoaderClient::canShowMIMEType(const String& MIMEType) const
{
    return [getWebView(m_webFrame.get()) _canShowMIMEType:MIMEType];
}

bool WebFrameLoaderClient::canShowMIMETypeAsHTML(const String& MIMEType) const
{
    return [WebView canShowMIMETypeAsHTML:MIMEType];
}

bool WebFrameLoaderClient::representationExistsForURLScheme(const String& URLScheme) const
{
    return [WebView _representationExistsForURLScheme:URLScheme];
}

String WebFrameLoaderClient::generatedMIMETypeForURLScheme(const String& URLScheme) const
{
    return [WebView _generatedMIMETypeForURLScheme:URLScheme];
}

void WebFrameLoaderClient::frameLoadCompleted()
{
    // Note: Can be called multiple times.

    // See WebFrameLoaderClient::provisionalLoadStarted.
    if ([getWebView(m_webFrame.get()) drawsBackground])
        [[m_webFrame->_private->webFrameView _scrollView] setDrawsBackground:YES];
}

void WebFrameLoaderClient::saveViewStateToItem(HistoryItem* item)
{
    if (!item)
        return;
    
    NSView <WebDocumentView> *docView = [m_webFrame->_private->webFrameView documentView];

    // we might already be detached when this is called from detachFromParent, in which
    // case we don't want to override real data earlier gathered with (0,0)
    if ([docView superview] && [docView conformsToProtocol:@protocol(_WebDocumentViewState)])
        item->setViewState([(id <_WebDocumentViewState>)docView viewState]);
}

void WebFrameLoaderClient::restoreViewState()
{
    HistoryItem* currentItem = core(m_webFrame.get())->loader()->history()->currentItem();
    ASSERT(currentItem);

    // FIXME: As the ASSERT attests, it seems we should always have a currentItem here.
    // One counterexample is <rdar://problem/4917290>
    // For now, to cover this issue in release builds, there is no technical harm to returning
    // early and from a user standpoint - as in the above radar - the previous page load failed 
    // so there *is* no scroll state to restore!
    if (!currentItem)
        return;
    
    NSView <WebDocumentView> *docView = [m_webFrame->_private->webFrameView documentView];
    if ([docView conformsToProtocol:@protocol(_WebDocumentViewState)]) {        
        id state = currentItem->viewState();
        if (state) {
            [(id <_WebDocumentViewState>)docView setViewState:state];
        }
    }
}

void WebFrameLoaderClient::provisionalLoadStarted()
{    
    // Tell the scroll view not to draw a background so we can leave the contents of
    // the old page showing during the beginning of the loading process.

    // This will stay set to NO until:
    //    1) The load gets far enough along: WebFrameLoader::frameLoadCompleted.
    //    2) The window is resized: -[WebFrameView setFrameSize:].
    // or 3) The view is moved out of the window: -[WebFrameView viewDidMoveToWindow].
    // Please keep the comments in these four functions in agreement with each other.

    WebDynamicScrollBarsView *scrollView = [m_webFrame->_private->webFrameView _scrollView];
    [scrollView setDrawsBackground:NO];
#if __MAC_OS_X_VERSION_MIN_REQUIRED >= 1070
    [scrollView setVerticalScrollElasticity:NSScrollElasticityNone];
    [scrollView setHorizontalScrollElasticity:NSScrollElasticityNone];
#endif
}

void WebFrameLoaderClient::didFinishLoad()
{
    [m_webFrame->_private->internalLoadDelegate webFrame:m_webFrame.get() didFinishLoadWithError:nil];    
}

void WebFrameLoaderClient::prepareForDataSourceReplacement()
{
    if (![m_webFrame.get() _dataSource]) {
        ASSERT(!core(m_webFrame.get())->tree()->childCount());
        return;
    }
    
    // Make sure that any work that is triggered by resigning first reponder can get done.
    // The main example where this came up is the textDidEndEditing that is sent to the
    // FormsDelegate (3223413). We need to do this before _detachChildren, since that will
    // remove the views as a side-effect of freeing the frame, at which point we can't
    // post the FormDelegate messages.
    //
    // Note that this can also take FirstResponder away from a child of our frameView that
    // is not in a child frame's view.  This is OK because we are in the process
    // of loading new content, which will blow away all editors in this top frame, and if
    // a non-editor is firstReponder it will not be affected by endEditingFor:.
    // Potentially one day someone could write a DocView whose editors were not all
    // replaced by loading new content, but that does not apply currently.
    NSView *frameView = m_webFrame->_private->webFrameView;
    NSWindow *window = [frameView window];
    NSResponder *firstResp = [window firstResponder];
    if ([firstResp isKindOfClass:[NSView class]] && [(NSView *)firstResp isDescendantOf:frameView])
        [window endEditingFor:firstResp];
}

PassRefPtr<DocumentLoader> WebFrameLoaderClient::createDocumentLoader(const ResourceRequest& request, const SubstituteData& substituteData)
{
    RefPtr<WebDocumentLoaderMac> loader = WebDocumentLoaderMac::create(request, substituteData);

    WebDataSource *dataSource = [[WebDataSource alloc] _initWithDocumentLoader:loader.get()];
    loader->setDataSource(dataSource, getWebView(m_webFrame.get()));
    [dataSource release];

    return loader.release();
}

void WebFrameLoaderClient::setTitle(const StringWithDirection& title, const KURL& url)
{
    WebView* view = getWebView(m_webFrame.get());
    
    if ([view historyDelegate]) {
        WebHistoryDelegateImplementationCache* implementations = WebViewGetHistoryDelegateImplementations(view);
        // FIXME: use direction of title.
        if (implementations->setTitleFunc)
            CallHistoryDelegate(implementations->setTitleFunc, view, @selector(webView:updateHistoryTitle:forURL:inFrame:), (NSString *)title.string(), (NSString *)url, m_webFrame.get());
        else if (implementations->deprecatedSetTitleFunc)
            CallHistoryDelegate(implementations->deprecatedSetTitleFunc, view, @selector(webView:updateHistoryTitle:forURL:), (NSString *)title.string(), (NSString *)url);

        return;
    }
    
    NSURL* nsURL = url;
    nsURL = [nsURL _webkit_canonicalize];
    if(!nsURL)
        return;
    NSString *titleNSString = title.string();
       
    [[[WebHistory optionalSharedHistory] itemForURL:nsURL] setTitle:titleNSString];
}

void WebFrameLoaderClient::savePlatformDataToCachedFrame(CachedFrame* cachedFrame)
{
    OwnPtr<WebCachedFramePlatformData> webPlatformData = adoptPtr(new WebCachedFramePlatformData([m_webFrame->_private->webFrameView documentView]));
    cachedFrame->setCachedFramePlatformData(webPlatformData.release());
}

void WebFrameLoaderClient::transitionToCommittedFromCachedFrame(CachedFrame* cachedFrame)
{
    WebCachedFramePlatformData* platformData = reinterpret_cast<WebCachedFramePlatformData*>(cachedFrame->cachedFramePlatformData());
    NSView <WebDocumentView> *cachedView = platformData->webDocumentView();
    ASSERT(cachedView != nil);
    ASSERT(cachedFrame->documentLoader());
    [cachedView setDataSource:dataSource(cachedFrame->documentLoader())];
    
    // clean up webkit plugin instances before WebHTMLView gets freed.
    WebView *webView = getWebView(m_webFrame.get());
    [webView removePluginInstanceViewsFor:(m_webFrame.get())];
    
    [m_webFrame->_private->webFrameView _setDocumentView:cachedView];
}

void WebFrameLoaderClient::transitionToCommittedForNewPage()
{
    WebView *webView = getWebView(m_webFrame.get());
    WebDataSource *dataSource = [m_webFrame.get() _dataSource];

    bool willProduceHTMLView = [m_webFrame->_private->webFrameView _viewClassForMIMEType:[dataSource _responseMIMEType]] == [WebHTMLView class];
    bool canSkipCreation = core(m_webFrame.get())->loader()->stateMachine()->committingFirstRealLoad() && willProduceHTMLView;
    if (canSkipCreation) {
        [[m_webFrame->_private->webFrameView documentView] setDataSource:dataSource];
        return;
    }

    // Don't suppress scrollbars before the view creation if we're making the view for a non-HTML view.
    if (!willProduceHTMLView)
        [[m_webFrame->_private->webFrameView _scrollView] setScrollBarsSuppressed:NO repaintOnUnsuppress:NO];
    
    // clean up webkit plugin instances before WebHTMLView gets freed.
    [webView removePluginInstanceViewsFor:(m_webFrame.get())];
    
    NSView <WebDocumentView> *documentView = [m_webFrame->_private->webFrameView _makeDocumentViewForDataSource:dataSource];
    if (!documentView)
        return;

    // FIXME: Could we skip some of this work for a top-level view that is not a WebHTMLView?

    // If we own the view, delete the old one - otherwise the render m_frame will take care of deleting the view.
    Frame* coreFrame = core(m_webFrame.get());
    Page* page = coreFrame->page();
    bool isMainFrame = coreFrame == page->mainFrame();
    if (isMainFrame && coreFrame->view())
        coreFrame->view()->setParentVisible(false);
    coreFrame->setView(0);
    RefPtr<FrameView> coreView = FrameView::create(coreFrame);
    coreFrame->setView(coreView);

    [m_webFrame.get() _updateBackgroundAndUpdatesWhileOffscreen];
    [m_webFrame->_private->webFrameView _install];

    if (isMainFrame)
        coreView->setParentVisible(true);

    // Call setDataSource on the document view after it has been placed in the view hierarchy.
    // This what we for the top-level view, so should do this for views in subframes as well.
    [documentView setDataSource:dataSource];

    // The following is a no-op for WebHTMLRepresentation, but for custom document types
    // like the ones that Safari uses for bookmarks it is the only way the DocumentLoader
    // will get the proper title.
    if (DocumentLoader* documentLoader = [dataSource _documentLoader])
        documentLoader->setTitle(StringWithDirection([dataSource pageTitle], LTR));

    if (HTMLFrameOwnerElement* owner = coreFrame->ownerElement())
        coreFrame->view()->setCanHaveScrollbars(owner->scrollingMode() != ScrollbarAlwaysOff);
        
    // If the document view implicitly became first responder, make sure to set the focused frame properly.
    if ([[documentView window] firstResponder] == documentView) {
        page->focusController()->setFocusedFrame(coreFrame);
        page->focusController()->setFocused(true);
    }
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

RetainPtr<WebFramePolicyListener> WebFrameLoaderClient::setUpPolicyListener(FramePolicyFunction function)
{
    // FIXME: <rdar://5634381> We need to support multiple active policy listeners.
    [m_policyListener invalidate];

    m_policyListener = adoptNS([[WebFramePolicyListener alloc] initWithFrame:core(m_webFrame.get()) policyFunction:function]);

    return m_policyListener;
}

String WebFrameLoaderClient::userAgent(const KURL& url)
{
    WebView *webView = getWebView(m_webFrame.get());
    ASSERT(webView);

    // We should never get here with nil for the WebView unless there is a bug somewhere else.
    // But if we do, it's better to return the empty string than just crashing on the spot.
    // Most other call sites are tolerant of nil because of Objective-C behavior, but this one
    // is not because the return value of _userAgentForURL is a const KURL&.
    if (!webView)
        return String("");

    return [webView _userAgentString];
}

static const MouseEvent* findMouseEvent(const Event* event)
{
    for (const Event* e = event; e; e = e->underlyingEvent())
        if (e->isMouseEvent())
            return static_cast<const MouseEvent*>(e);
    return 0;
}

NSDictionary *WebFrameLoaderClient::actionDictionary(const NavigationAction& action, PassRefPtr<FormState> formState) const
{
    unsigned modifierFlags = 0;
    const Event* event = action.event();
    if (const UIEventWithKeyState* keyStateEvent = findEventWithKeyState(const_cast<Event*>(event))) {
        if (keyStateEvent->ctrlKey())
            modifierFlags |= NSControlKeyMask;
        if (keyStateEvent->altKey())
            modifierFlags |= NSAlternateKeyMask;
        if (keyStateEvent->shiftKey())
            modifierFlags |= NSShiftKeyMask;
        if (keyStateEvent->metaKey())
            modifierFlags |= NSCommandKeyMask;
    }

    NSURL *originalURL = action.url();

    NSMutableDictionary *result = [NSMutableDictionary dictionaryWithObjectsAndKeys:
        [NSNumber numberWithInt:action.type()], WebActionNavigationTypeKey,
        [NSNumber numberWithInt:modifierFlags], WebActionModifierFlagsKey,
        originalURL, WebActionOriginalURLKey,
        nil];

    if (const MouseEvent* mouseEvent = findMouseEvent(event)) {
        WebElementDictionary *element = [[WebElementDictionary alloc]
            initWithHitTestResult:core(m_webFrame.get())->eventHandler()->hitTestResultAtPoint(mouseEvent->absoluteLocation())];
        [result setObject:element forKey:WebActionElementKey];
        [element release];

        [result setObject:[NSNumber numberWithInt:mouseEvent->button()] forKey:WebActionButtonKey];
    }

    if (formState) {
        ASSERT(formState->form());
        [result setObject:kit(formState->form()) forKey:WebActionFormKey];
    }

    return result;
}

bool WebFrameLoaderClient::canCachePage() const
{
    // We can only cache HTML pages right now
    return [[[m_webFrame.get() _dataSource] representation] isKindOfClass:[WebHTMLRepresentation class]];
}

PassRefPtr<Frame> WebFrameLoaderClient::createFrame(const KURL& url, const String& name, HTMLFrameOwnerElement* ownerElement,
    const String& referrer, bool allowsScrolling, int marginWidth, int marginHeight)
{
    BEGIN_BLOCK_OBJC_EXCEPTIONS;
    
    ASSERT(m_webFrame);
    
    WebFrameView *childView = [[WebFrameView alloc] init];
    
    RefPtr<Frame> result = [WebFrame _createSubframeWithOwnerElement:ownerElement frameName:name frameView:childView];
    [childView release];

    WebFrame *newFrame = kit(result.get());

    if ([newFrame _dataSource])
        [[newFrame _dataSource] _documentLoader]->setOverrideEncoding([[m_webFrame.get() _dataSource] _documentLoader]->overrideEncoding());  

    // The creation of the frame may have run arbitrary JavaScript that removed it from the page already.
    if (!result->page())
        return 0;
 
    core(m_webFrame.get())->loader()->loadURLIntoChildFrame(url, referrer, result.get());

    // The frame's onload handler may have removed it from the document.
    if (!result->tree()->parent())
        return 0;

    return result.release();

    END_BLOCK_OBJC_EXCEPTIONS;

    return 0;
}

ObjectContentType WebFrameLoaderClient::objectContentType(const KURL& url, const String& mimeType, bool shouldPreferPlugInsForImages)
{
    BEGIN_BLOCK_OBJC_EXCEPTIONS;

    String type = mimeType;

    if (type.isEmpty()) {
        // Try to guess the MIME type based off the extension.
        NSURL *URL = url;
        NSString *extension = [[URL path] pathExtension];
        if ([extension length] > 0) {
            type = WKGetMIMETypeForExtension(extension);
            if (type.isEmpty()) {
                // If no MIME type is specified, use a plug-in if we have one that can handle the extension.
                if (WebBasePluginPackage *package = [getWebView(m_webFrame.get()) _pluginForExtension:extension]) {
                    if ([package isKindOfClass:[WebPluginPackage class]]) 
                        return ObjectContentOtherPlugin;
#if ENABLE(NETSCAPE_PLUGIN_API)
                    else {
                        ASSERT([package isKindOfClass:[WebNetscapePluginPackage class]]);
                        return ObjectContentNetscapePlugin;
                    }
#endif
                }
            }
        }
    }

    if (type.isEmpty())
        return ObjectContentFrame; // Go ahead and hope that we can display the content.

    WebBasePluginPackage *package = [getWebView(m_webFrame.get()) _pluginForMIMEType:type];
    ObjectContentType plugInType = ObjectContentNone;
    if (package) {
#if ENABLE(NETSCAPE_PLUGIN_API)
        if ([package isKindOfClass:[WebNetscapePluginPackage class]])
            plugInType = ObjectContentNetscapePlugin;
        else
#endif
        {
            ASSERT([package isKindOfClass:[WebPluginPackage class]]);
            plugInType = ObjectContentOtherPlugin;
        }
    }
    
    if (MIMETypeRegistry::isSupportedImageMIMEType(type))
        return shouldPreferPlugInsForImages && plugInType != ObjectContentNone ? plugInType : ObjectContentImage;

    if (plugInType != ObjectContentNone)
        return plugInType;

    if ([m_webFrame->_private->webFrameView _viewClassForMIMEType:type])
        return ObjectContentFrame;
    
    return ObjectContentNone;

    END_BLOCK_OBJC_EXCEPTIONS;

    return ObjectContentNone;
}

static NSMutableArray* kit(const Vector<String>& vector)
{
    unsigned len = vector.size();
    NSMutableArray* array = [NSMutableArray arrayWithCapacity:len];
    for (unsigned x = 0; x < len; x++)
        [array addObject:vector[x]];
    return array;
}

static String parameterValue(const Vector<String>& paramNames, const Vector<String>& paramValues, const String& name)
{
    size_t size = paramNames.size();
    ASSERT(size == paramValues.size());
    for (size_t i = 0; i < size; ++i) {
        if (equalIgnoringCase(paramNames[i], name))
            return paramValues[i];
    }
    return String();
}

static NSView *pluginView(WebFrame *frame, WebPluginPackage *pluginPackage,
    NSArray *attributeNames, NSArray *attributeValues, NSURL *baseURL,
    DOMElement *element, BOOL loadManually)
{
    WebHTMLView *docView = (WebHTMLView *)[[frame frameView] documentView];
    ASSERT([docView isKindOfClass:[WebHTMLView class]]);
        
    WebPluginController *pluginController = [docView _pluginController];
    
    // Store attributes in a dictionary so they can be passed to WebPlugins.
    NSMutableDictionary *attributes = [[NSMutableDictionary alloc] initWithObjects:attributeValues forKeys:attributeNames];
    
    [pluginPackage load];
    Class viewFactory = [pluginPackage viewFactory];
    
    NSView *view = nil;
    NSDictionary *arguments = nil;
    
    if ([viewFactory respondsToSelector:@selector(plugInViewWithArguments:)]) {
        arguments = [NSDictionary dictionaryWithObjectsAndKeys:
            baseURL, WebPlugInBaseURLKey,
            attributes, WebPlugInAttributesKey,
            pluginController, WebPlugInContainerKey,
            [NSNumber numberWithInt:loadManually ? WebPlugInModeFull : WebPlugInModeEmbed], WebPlugInModeKey,
            [NSNumber numberWithBool:!loadManually], WebPlugInShouldLoadMainResourceKey,
            element, WebPlugInContainingElementKey,
            nil];
        LOG(Plugins, "arguments:\n%@", arguments);
    } else if ([viewFactory respondsToSelector:@selector(pluginViewWithArguments:)]) {
        arguments = [NSDictionary dictionaryWithObjectsAndKeys:
            baseURL, WebPluginBaseURLKey,
            attributes, WebPluginAttributesKey,
            pluginController, WebPluginContainerKey,
            element, WebPlugInContainingElementKey,
            nil];
        LOG(Plugins, "arguments:\n%@", arguments);
    }

    view = [WebPluginController plugInViewWithArguments:arguments fromPluginPackage:pluginPackage];
    [attributes release];
    return view;
}

class PluginWidget : public PluginViewBase {
public:
    PluginWidget(NSView *view = 0)
        : PluginViewBase(view)
    {
    }
    
private:
    virtual void invalidateRect(const IntRect& rect)
    {
        [platformWidget() setNeedsDisplayInRect:rect];
    }
};

#if ENABLE(NETSCAPE_PLUGIN_API)

class NetscapePluginWidget : public PluginWidget {
public:
    NetscapePluginWidget(WebBaseNetscapePluginView *view)
        : PluginWidget(view)
    {
    }
    
#if USE(ACCELERATED_COMPOSITING)
    virtual PlatformLayer* platformLayer() const
    {
        return [(WebBaseNetscapePluginView *)platformWidget() pluginLayer];
    }
#endif

    virtual bool getFormValue(String& value)
    {
        NSString* nsValue = 0;
        if ([(WebBaseNetscapePluginView *)platformWidget() getFormValue:&nsValue]) {
            if (!nsValue)
                return false;
            value = String(nsValue);
            [nsValue release];
            return true;
        }
        return false;
    }

    virtual void handleEvent(Event* event)
    {
        Frame* frame = Frame::frameForWidget(this);
        if (!frame)
            return;
        
        NSEvent* currentNSEvent = frame->eventHandler()->currentNSEvent();
        if (event->type() == eventNames().mousemoveEvent)
            [(WebBaseNetscapePluginView *)platformWidget() handleMouseMoved:currentNSEvent];
        else if (event->type() == eventNames().mouseoverEvent)
            [(WebBaseNetscapePluginView *)platformWidget() handleMouseEntered:currentNSEvent];
        else if (event->type() == eventNames().mouseoutEvent)
            [(WebBaseNetscapePluginView *)platformWidget() handleMouseExited:currentNSEvent];
        else if (event->type() == eventNames().contextmenuEvent)
            event->setDefaultHandled(); // We don't know if the plug-in has handled mousedown event by displaying a context menu, so we never want WebKit to show a default one.
    }

    virtual void clipRectChanged()
    {
        // Changing the clip rect doesn't affect the view hierarchy, so the plugin must be told about the change directly.
        [(WebBaseNetscapePluginView *)platformWidget() updateAndSetWindow];
    }

private:
    virtual void notifyWidget(WidgetNotification notification)
    {
        switch (notification) {
        case WillPaintFlattened: {
            BEGIN_BLOCK_OBJC_EXCEPTIONS;
            [(WebBaseNetscapePluginView *)platformWidget() cacheSnapshot];
            END_BLOCK_OBJC_EXCEPTIONS;
            break;
        }
        case DidPaintFlattened: {
            BEGIN_BLOCK_OBJC_EXCEPTIONS;
            [(WebBaseNetscapePluginView *)platformWidget() clearCachedSnapshot];
            END_BLOCK_OBJC_EXCEPTIONS;
            break;
        }
        }
    }
};

#if USE(PLUGIN_HOST_PROCESS)
#define NETSCAPE_PLUGIN_VIEW WebHostedNetscapePluginView
#else
#define NETSCAPE_PLUGIN_VIEW WebNetscapePluginView
#endif

#endif // ENABLE(NETSCAPE_PLUGIN_API)

PassRefPtr<Widget> WebFrameLoaderClient::createPlugin(const IntSize& size, HTMLPlugInElement* element, const KURL& url,
    const Vector<String>& paramNames, const Vector<String>& paramValues, const String& mimeType, bool loadManually)
{
    BEGIN_BLOCK_OBJC_EXCEPTIONS;

    ASSERT(paramNames.size() == paramValues.size());

    int errorCode = 0;

    WebView *webView = getWebView(m_webFrame.get());
    SEL selector = @selector(webView:plugInViewWithArguments:);

    Document* document = core(m_webFrame.get())->document();
    NSURL *baseURL = document->baseURL();
    NSURL *pluginURL = url;
    
    // <rdar://problem/8366089>: AppleConnect has a bug where it does not
    // understand the parameter names specified in the <object> element that
    // embeds its plug-in. This site-specific hack works around the issue by
    // converting the parameter names to lowercase before passing them to the
    // plug-in.
    Frame* frame = core(m_webFrame.get());
    NSMutableArray *attributeKeys = kit(paramNames);
    if (frame && frame->settings()->needsSiteSpecificQuirks() && equalIgnoringCase(mimeType, "application/x-snkp")) {
        for (NSUInteger i = 0; i < [attributeKeys count]; ++i)
            [attributeKeys replaceObjectAtIndex:i withObject:[[attributeKeys objectAtIndex:i] lowercaseString]];
    }
    
    if ([[webView UIDelegate] respondsToSelector:selector]) {
        NSMutableDictionary *attributes = [[NSMutableDictionary alloc] initWithObjects:kit(paramValues) forKeys:attributeKeys];
        NSDictionary *arguments = [[NSDictionary alloc] initWithObjectsAndKeys:
            attributes, WebPlugInAttributesKey,
            [NSNumber numberWithInt:loadManually ? WebPlugInModeFull : WebPlugInModeEmbed], WebPlugInModeKey,
            [NSNumber numberWithBool:!loadManually], WebPlugInShouldLoadMainResourceKey,
            kit(element), WebPlugInContainingElementKey,
            // FIXME: We should be passing base URL, see <https://bugs.webkit.org/show_bug.cgi?id=35215>.
            pluginURL, WebPlugInBaseURLKey, // pluginURL might be nil, so add it last
            nil];

        NSView *view = CallUIDelegate(webView, selector, arguments);

        [attributes release];
        [arguments release];

        if (view)
            return adoptRef(new PluginWidget(view));
    }

    NSString *MIMEType;
    WebBasePluginPackage *pluginPackage;
    if (mimeType.isEmpty()) {
        MIMEType = nil;
        pluginPackage = nil;
    } else {
        MIMEType = mimeType;
        pluginPackage = [webView _pluginForMIMEType:mimeType];
    }
    
    NSString *extension = [[pluginURL path] pathExtension];
    if (!pluginPackage && [extension length] && ![MIMEType length]) {
        pluginPackage = [webView _pluginForExtension:extension];
        if (pluginPackage) {
            NSString *newMIMEType = [pluginPackage MIMETypeForExtension:extension];
            if ([newMIMEType length] != 0)
                MIMEType = newMIMEType;
        }
    }

    NSView *view = nil;

    if (pluginPackage) {
        if (WKShouldBlockPlugin([pluginPackage bundleIdentifier], [pluginPackage bundleVersion])) {
            errorCode = WebKitErrorBlockedPlugInVersion;
            if (element->renderer()->isEmbeddedObject())
                toRenderEmbeddedObject(element->renderer())->setPluginUnavailabilityReason(RenderEmbeddedObject::InsecurePluginVersion);
        } else {
            if ([pluginPackage isKindOfClass:[WebPluginPackage class]])
                view = pluginView(m_webFrame.get(), (WebPluginPackage *)pluginPackage, attributeKeys, kit(paramValues), baseURL, kit(element), loadManually);

#if ENABLE(NETSCAPE_PLUGIN_API)
            else if ([pluginPackage isKindOfClass:[WebNetscapePluginPackage class]]) {
                WebBaseNetscapePluginView *pluginView = [[[NETSCAPE_PLUGIN_VIEW alloc]
                    initWithFrame:NSMakeRect(0, 0, size.width(), size.height())
                    pluginPackage:(WebNetscapePluginPackage *)pluginPackage
                    URL:pluginURL
                    baseURL:baseURL
                    MIMEType:MIMEType
                    attributeKeys:attributeKeys
                    attributeValues:kit(paramValues)
                    loadManually:loadManually
                    element:element] autorelease];

                return adoptRef(new NetscapePluginWidget(pluginView));
            }
#endif
        }
    } else
        errorCode = WebKitErrorCannotFindPlugIn;

    if (!errorCode && !view)
        errorCode = WebKitErrorCannotLoadPlugIn;
    
    if (errorCode && m_webFrame) {
        WebResourceDelegateImplementationCache* implementations = WebViewGetResourceLoadDelegateImplementations(webView);
        if (implementations->plugInFailedWithErrorFunc) {
            KURL pluginPageURL = document->completeURL(stripLeadingAndTrailingHTMLSpaces(parameterValue(paramNames, paramValues, "pluginspage")));
            if (!pluginPageURL.protocolIsInHTTPFamily())
                pluginPageURL = KURL();
            NSString *pluginName = pluginPackage ? (NSString *)[pluginPackage pluginInfo].name : nil;

            NSError *error = [[NSError alloc] _initWithPluginErrorCode:errorCode
                                                            contentURL:pluginURL pluginPageURL:pluginPageURL pluginName:pluginName MIMEType:MIMEType];
            CallResourceLoadDelegate(implementations->plugInFailedWithErrorFunc, [m_webFrame.get() webView],
                                     @selector(webView:plugInFailedWithError:dataSource:), error, [m_webFrame.get() _dataSource]);
            [error release];
        }

        return 0;
    }
    
    ASSERT(view);
    return adoptRef(new PluginWidget(view));

    END_BLOCK_OBJC_EXCEPTIONS;

    return 0;
}

void WebFrameLoaderClient::recreatePlugin(Widget*)
{
}

void WebFrameLoaderClient::redirectDataToPlugin(Widget* pluginWidget)
{
    if (!pluginWidget)
        return;

    BEGIN_BLOCK_OBJC_EXCEPTIONS;

    WebHTMLRepresentation *representation = (WebHTMLRepresentation *)[[m_webFrame.get() _dataSource] representation];

    NSView *pluginView = pluginWidget->platformWidget();

#if ENABLE(NETSCAPE_PLUGIN_API)
    if ([pluginView isKindOfClass:[NETSCAPE_PLUGIN_VIEW class]])
        [representation _redirectDataToManualLoader:(NETSCAPE_PLUGIN_VIEW *)pluginView forPluginView:pluginView];
    else {
#else
    {
#endif
        WebHTMLView *documentView = (WebHTMLView *)[[m_webFrame.get() frameView] documentView];
        ASSERT([documentView isKindOfClass:[WebHTMLView class]]);
        [representation _redirectDataToManualLoader:[documentView _pluginController] forPluginView:pluginView];
    }

    END_BLOCK_OBJC_EXCEPTIONS;
}
    
PassRefPtr<Widget> WebFrameLoaderClient::createJavaAppletWidget(const IntSize& size, HTMLAppletElement* element, const KURL& baseURL, 
    const Vector<String>& paramNames, const Vector<String>& paramValues)
{
    BEGIN_BLOCK_OBJC_EXCEPTIONS;

    NSView *view = nil;

    NSString *MIMEType = @"application/x-java-applet";
    
    WebView *webView = getWebView(m_webFrame.get());

    WebBasePluginPackage *pluginPackage = [webView _pluginForMIMEType:MIMEType];

    int errorCode = WebKitErrorJavaUnavailable;

    if (pluginPackage) {
        if (WKShouldBlockPlugin([pluginPackage bundleIdentifier], [pluginPackage bundleVersion])) {
            errorCode = WebKitErrorBlockedPlugInVersion;
            if (element->renderer()->isEmbeddedObject())
                toRenderEmbeddedObject(element->renderer())->setPluginUnavailabilityReason(RenderEmbeddedObject::InsecurePluginVersion);
        } else {
    #if ENABLE(NETSCAPE_PLUGIN_API)
            if ([pluginPackage isKindOfClass:[WebNetscapePluginPackage class]]) {
                view = [[[NETSCAPE_PLUGIN_VIEW alloc] initWithFrame:NSMakeRect(0, 0, size.width(), size.height())
                    pluginPackage:(WebNetscapePluginPackage *)pluginPackage
                    URL:nil
                    baseURL:baseURL
                    MIMEType:MIMEType
                    attributeKeys:kit(paramNames)
                    attributeValues:kit(paramValues)
                    loadManually:NO
                    element:element] autorelease];
                if (view)
                    return adoptRef(new NetscapePluginWidget(static_cast<WebBaseNetscapePluginView *>(view)));
            }
    #endif
        }
    }

    if (!view) {
        WebResourceDelegateImplementationCache* implementations = WebViewGetResourceLoadDelegateImplementations(getWebView(m_webFrame.get()));
        if (implementations->plugInFailedWithErrorFunc) {
            NSString *pluginName = pluginPackage ? (NSString *)[pluginPackage pluginInfo].name : nil;
            NSError *error = [[NSError alloc] _initWithPluginErrorCode:errorCode contentURL:nil pluginPageURL:nil pluginName:pluginName MIMEType:MIMEType];
            CallResourceLoadDelegate(implementations->plugInFailedWithErrorFunc, [m_webFrame.get() webView],
                                     @selector(webView:plugInFailedWithError:dataSource:), error, [m_webFrame.get() _dataSource]);
            [error release];
        }
    }

    END_BLOCK_OBJC_EXCEPTIONS;

    return 0;
}

#if ENABLE(PLUGIN_PROXY_FOR_VIDEO)
PassRefPtr<Widget> WebFrameLoaderClient::createMediaPlayerProxyPlugin(const IntSize& size, HTMLMediaElement* element, const KURL& url,
    const Vector<String>& paramNames, const Vector<String>& paramValues, const String& mimeType)
{
    BEGIN_BLOCK_OBJC_EXCEPTIONS;

    ASSERT(paramNames.size() == paramValues.size());
    ASSERT(mimeType);

    int errorCode = 0;
    WebView *webView = getWebView(m_webFrame.get());
    NSURL *URL = url;

    SEL selector = @selector(webView:plugInViewWithArguments:);

    if ([[webView UIDelegate] respondsToSelector:selector]) {
        NSMutableDictionary *attributes = [[NSMutableDictionary alloc] initWithObjects:kit(paramValues) forKeys:kit(paramNames)];
        NSDictionary *arguments = [[NSDictionary alloc] initWithObjectsAndKeys:
            attributes, WebPlugInAttributesKey,
            [NSNumber numberWithInt:WebPlugInModeEmbed], WebPlugInModeKey,
            [NSNumber numberWithBool:YES], WebPlugInShouldLoadMainResourceKey,
            kit(element), WebPlugInContainingElementKey,
            URL, WebPlugInBaseURLKey, // URL might be nil, so add it last
            nil];

        NSView *view = CallUIDelegate(webView, selector, arguments);

        [attributes release];
        [arguments release];

        if (view)
            return adoptRef(new PluginWidget(view));
    }

    WebBasePluginPackage *pluginPackage = [webView _videoProxyPluginForMIMEType:mimeType];
    Document* document = core(m_webFrame.get())->document();
    NSURL *baseURL = document->baseURL();
    NSView *view = nil;

    if (pluginPackage) {
        if ([pluginPackage isKindOfClass:[WebPluginPackage class]])
            view = pluginView(m_webFrame.get(), (WebPluginPackage *)pluginPackage, kit(paramNames), kit(paramValues), baseURL, kit(element), false);
    } else
        errorCode = WebKitErrorCannotFindPlugIn;

    if (!errorCode && !view)
        errorCode = WebKitErrorCannotLoadPlugIn;

    if (errorCode) {
        NSError *error = [[NSError alloc] _initWithPluginErrorCode:errorCode
            contentURL:URL pluginPageURL:nil pluginName:[pluginPackage pluginInfo].name MIMEType:mimeType];
        WebNullPluginView *nullView = [[[WebNullPluginView alloc] initWithFrame:NSMakeRect(0, 0, size.width(), size.height())
            error:error DOMElement:kit(element)] autorelease];
        view = nullView;
        [error release];
    }
    
    ASSERT(view);
    return adoptRef(new PluginWidget(view));

    END_BLOCK_OBJC_EXCEPTIONS;

    return 0;
}

void WebFrameLoaderClient::hideMediaPlayerProxyPlugin(Widget* widget)
{
    [WebPluginController pluginViewHidden:widget->platformWidget()];
}

void WebFrameLoaderClient::showMediaPlayerProxyPlugin(Widget* widget)
{
    [WebPluginController addPlugInView:widget->platformWidget()];
}

#endif  // ENABLE(PLUGIN_PROXY_FOR_VIDEO)

String WebFrameLoaderClient::overrideMediaType() const
{
    NSString* overrideType = [getWebView(m_webFrame.get()) mediaStyle];
    if (overrideType)
        return overrideType;
    return String();
}

void WebFrameLoaderClient::documentElementAvailable() {
}

void WebFrameLoaderClient::dispatchDidClearWindowObjectInWorld(DOMWrapperWorld* world)
{
    WebView *webView = getWebView(m_webFrame.get());
    WebFrameLoadDelegateImplementationCache* implementations = WebViewGetFrameLoadDelegateImplementations(webView);

    if (implementations->didClearWindowObjectForFrameInScriptWorldFunc) {
        CallFrameLoadDelegate(implementations->didClearWindowObjectForFrameInScriptWorldFunc,
            webView, @selector(webView:didClearWindowObjectForFrame:inScriptWorld:), m_webFrame.get(), [WebScriptWorld findOrCreateWorld:world]);
        return;
    }

    if (world != mainThreadNormalWorld())
        return;

    Frame *frame = core(m_webFrame.get());
    ScriptController *script = frame->script();

#if JSC_OBJC_API_ENABLED
    if (implementations->didCreateJavaScriptContextForFrameFunc) {
        CallFrameLoadDelegate(implementations->didCreateJavaScriptContextForFrameFunc, webView, @selector(webView:didCreateJavaScriptContext:forFrame:),
            script->javaScriptContext(), m_webFrame.get());
    } else if (implementations->didClearWindowObjectForFrameFunc) {
#else
    if (implementations->didClearWindowObjectForFrameFunc) {
#endif
        CallFrameLoadDelegate(implementations->didClearWindowObjectForFrameFunc, webView, @selector(webView:didClearWindowObject:forFrame:),
            script->windowScriptObject(), m_webFrame.get());
    } else if (implementations->windowScriptObjectAvailableFunc) {
        CallFrameLoadDelegate(implementations->windowScriptObjectAvailableFunc, webView, @selector(webView:windowScriptObjectAvailable:),
            script->windowScriptObject());
    }

    if ([webView scriptDebugDelegate]) {
        [m_webFrame.get() _detachScriptDebugger];
        [m_webFrame.get() _attachScriptDebugger];
    }
}

void WebFrameLoaderClient::registerForIconNotification(bool listen)
{
#if ENABLE(ICONDATABASE)
    [[m_webFrame.get() webView] _registerForIconNotification:listen];
#endif
}

void WebFrameLoaderClient::didPerformFirstNavigation() const
{
    WebPreferences *preferences = [[m_webFrame.get() webView] preferences];
    if ([preferences automaticallyDetectsCacheModel] && [preferences cacheModel] < WebCacheModelDocumentBrowser)
        [preferences setCacheModel:WebCacheModelDocumentBrowser];
}

PassRefPtr<FrameNetworkingContext> WebFrameLoaderClient::createNetworkingContext()
{
    return WebFrameNetworkingContext::create(core(m_webFrame.get()));
}

@implementation WebFramePolicyListener

+ (void)initialize
{
    JSC::initializeThreading();
    WTF::initializeMainThreadToProcessMainThread();
    WebCore::RunLoop::initializeMainRunLoop();
    WebCoreObjCFinalizeOnMainThread(self);
}

- (id)initWithFrame:(Frame*)frame policyFunction:(FramePolicyFunction)policyFunction
{
    self = [self init];
    if (!self)
        return nil;

    _frame = frame;
    _policyFunction = policyFunction;

    return self;
}

- (void)invalidate
{
    _frame = nullptr;
}

- (void)dealloc
{
    if (WebCoreObjCScheduleDeallocateOnMainThread([WebFramePolicyListener class], self))
        return;

    [super dealloc];
}

- (void)receivedPolicyDecision:(PolicyAction)action
{
    RefPtr<Frame> frame = _frame.release();
    if (!frame)
        return;

    FramePolicyFunction policyFunction = _policyFunction;
    _policyFunction = nullptr;

    ASSERT(policyFunction);
    (frame->loader()->policyChecker()->*policyFunction)(action);
}

- (void)ignore
{
    [self receivedPolicyDecision:PolicyIgnore];
}

- (void)download
{
    [self receivedPolicyDecision:PolicyDownload];
}

- (void)use
{
    [self receivedPolicyDecision:PolicyUse];
}

- (void)continue
{
    [self receivedPolicyDecision:PolicyUse];
}

@end
