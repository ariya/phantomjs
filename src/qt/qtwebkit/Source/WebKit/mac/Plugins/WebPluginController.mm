/*
 * Copyright (C) 2005, 2006 Apple Computer, Inc.  All rights reserved.
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


#import "WebPluginController.h"

#import "DOMNodeInternal.h"
#import "WebDataSourceInternal.h"
#import "WebFrameInternal.h"
#import "WebFrameView.h"
#import "WebHTMLViewPrivate.h"
#import "WebKitErrorsPrivate.h"
#import "WebKitLogging.h"
#import "WebNSObjectExtras.h"
#import "WebNSURLExtras.h"
#import "WebNSViewExtras.h"
#import "WebPlugin.h"
#import "WebPluginContainer.h"
#import "WebPluginContainerCheck.h"
#import "WebPluginPackage.h"
#import "WebPluginPrivate.h"
#import "WebPluginViewFactory.h"
#import "WebUIDelegate.h"
#import "WebViewInternal.h"
#import <Foundation/NSURLRequest.h>
#import <WebCore/DocumentLoader.h>
#import <WebCore/Frame.h>
#import <WebCore/FrameLoadRequest.h>
#import <WebCore/FrameLoader.h>
#import <WebCore/HTMLMediaElement.h>
#import <WebCore/HTMLNames.h>
#import <WebCore/MediaPlayerProxy.h>
#import <WebCore/ResourceRequest.h>
#import <WebCore/ScriptController.h>
#import <WebCore/WebCoreURLResponse.h>
#import <objc/runtime.h>
#import <runtime/JSLock.h>
#import <wtf/text/WTFString.h>

using namespace WebCore;
using namespace HTMLNames;

@interface NSView (PluginSecrets)
- (void)setContainingWindow:(NSWindow *)w;
@end

// For compatibility only.
@interface NSObject (OldPluginAPI)
+ (NSView *)pluginViewWithArguments:(NSDictionary *)arguments;
@end

@interface NSView (OldPluginAPI)
- (void)pluginInitialize;
- (void)pluginStart;
- (void)pluginStop;
- (void)pluginDestroy;
@end

static bool isKindOfClass(id, NSString* className);
static void installFlip4MacPlugInWorkaroundIfNecessary();


static NSMutableSet *pluginViews = nil;

@implementation WebPluginController

+ (NSView *)plugInViewWithArguments:(NSDictionary *)arguments fromPluginPackage:(WebPluginPackage *)pluginPackage
{
    [pluginPackage load];
    Class viewFactory = [pluginPackage viewFactory];
    
    NSView *view = nil;

    if ([viewFactory respondsToSelector:@selector(plugInViewWithArguments:)]) {
        JSC::JSLock::DropAllLocks dropAllLocks(JSDOMWindowBase::commonVM());
        view = [viewFactory plugInViewWithArguments:arguments];
    } else if ([viewFactory respondsToSelector:@selector(pluginViewWithArguments:)]) {
        JSC::JSLock::DropAllLocks dropAllLocks(JSDOMWindowBase::commonVM());
        view = [viewFactory pluginViewWithArguments:arguments];
    }
    
    if (view == nil) {
        return nil;
    }
    
    if (pluginViews == nil) {
        pluginViews = [[NSMutableSet alloc] init];
    }
    [pluginViews addObject:view];
    
    return view;
}

+ (BOOL)isPlugInView:(NSView *)view
{
    return [pluginViews containsObject:view];
}

- (id)initWithDocumentView:(NSView *)view
{
    self = [super init];
    if (!self)
        return nil;
    _documentView = view;
    _views = [[NSMutableArray alloc] init];
    _checksInProgress = (NSMutableSet *)CFMakeCollectable(CFSetCreateMutable(NULL, 0, NULL));
    return self;
}

- (void)setDataSource:(WebDataSource *)dataSource
{
    _dataSource = dataSource;    
}

- (void)dealloc
{
    [_views release];
    [_checksInProgress release];
#if ENABLE(PLUGIN_PROXY_FOR_VIDEO)
    [_viewsNotInDocument release];
#endif
    [super dealloc];
}

- (void)stopOnePlugin:(NSView *)view
{
    if ([view respondsToSelector:@selector(webPlugInStop)]) {
        JSC::JSLock::DropAllLocks dropAllLocks(JSDOMWindowBase::commonVM());
        [view webPlugInStop];
    } else if ([view respondsToSelector:@selector(pluginStop)]) {
        JSC::JSLock::DropAllLocks dropAllLocks(JSDOMWindowBase::commonVM());
        [view pluginStop];
    }
}

- (void)destroyOnePlugin:(NSView *)view
{
    if ([view respondsToSelector:@selector(webPlugInDestroy)]) {
        JSC::JSLock::DropAllLocks dropAllLocks(JSDOMWindowBase::commonVM());
        [view webPlugInDestroy];
    } else if ([view respondsToSelector:@selector(pluginDestroy)]) {
        JSC::JSLock::DropAllLocks dropAllLocks(JSDOMWindowBase::commonVM());
        [view pluginDestroy];
    }
}

- (void)startAllPlugins
{
    if (_started)
        return;
    
    if ([_views count] > 0)
        LOG(Plugins, "starting WebKit plugins : %@", [_views description]);
    
    int count = [_views count];
    for (int i = 0; i < count; i++) {
        id aView = [_views objectAtIndex:i];
        if ([aView respondsToSelector:@selector(webPlugInStart)]) {
            JSC::JSLock::DropAllLocks dropAllLocks(JSDOMWindowBase::commonVM());
            [aView webPlugInStart];
        } else if ([aView respondsToSelector:@selector(pluginStart)]) {
            JSC::JSLock::DropAllLocks dropAllLocks(JSDOMWindowBase::commonVM());
            [aView pluginStart];
        }
    }
    _started = YES;
}

- (void)stopAllPlugins
{
    if (!_started)
        return;

    if ([_views count] > 0) {
        LOG(Plugins, "stopping WebKit plugins: %@", [_views description]);
    }
    
    int viewsCount = [_views count];
    for (int i = 0; i < viewsCount; i++)
        [self stopOnePlugin:[_views objectAtIndex:i]];

#if ENABLE(PLUGIN_PROXY_FOR_VIDEO)
    int viewsNotInDocumentCount = [_viewsNotInDocument count];
    for (int i = 0; i < viewsNotInDocumentCount; i++)
        [self stopOnePlugin:[_viewsNotInDocument objectAtIndex:i]];
#endif

    _started = NO;
}

#if ENABLE(PLUGIN_PROXY_FOR_VIDEO)
- (void)pluginViewCreated:(NSView *)view
{
    if (!_viewsNotInDocument)
        _viewsNotInDocument= [[NSMutableArray alloc] init];
    if (![_viewsNotInDocument containsObject:view])
        [_viewsNotInDocument addObject:view];
}

+ (void)pluginViewHidden:(NSView *)view
{
    [pluginViews removeObject:view];
}
#endif

- (void)addPlugin:(NSView *)view
{
    if (!_documentView) {
        LOG_ERROR("can't add a plug-in to a defunct WebPluginController");
        return;
    }
    
    if (![_views containsObject:view]) {
        [_views addObject:view];
        [[_documentView _webView] addPluginInstanceView:view];

#if ENABLE(PLUGIN_PROXY_FOR_VIDEO)
        if ([_viewsNotInDocument containsObject:view])
            [_viewsNotInDocument removeObject:view];
#endif

        BOOL oldDefersCallbacks = [[self webView] defersCallbacks];
        if (!oldDefersCallbacks)
            [[self webView] setDefersCallbacks:YES];

        if (isKindOfClass(view, @"WmvPlugin"))
            installFlip4MacPlugInWorkaroundIfNecessary();

        LOG(Plugins, "initializing plug-in %@", view);
        if ([view respondsToSelector:@selector(webPlugInInitialize)]) {
            JSC::JSLock::DropAllLocks dropAllLocks(JSDOMWindowBase::commonVM());
            [view webPlugInInitialize];
        } else if ([view respondsToSelector:@selector(pluginInitialize)]) {
            JSC::JSLock::DropAllLocks dropAllLocks(JSDOMWindowBase::commonVM());
            [view pluginInitialize];
        }

        if (!oldDefersCallbacks)
            [[self webView] setDefersCallbacks:NO];
        
        if (_started) {
            LOG(Plugins, "starting plug-in %@", view);
            if ([view respondsToSelector:@selector(webPlugInStart)]) {
                JSC::JSLock::DropAllLocks dropAllLocks(JSDOMWindowBase::commonVM());
                [view webPlugInStart];
            } else if ([view respondsToSelector:@selector(pluginStart)]) {
                JSC::JSLock::DropAllLocks dropAllLocks(JSDOMWindowBase::commonVM());
                [view pluginStart];
            }
            
            if ([view respondsToSelector:@selector(setContainingWindow:)]) {
                JSC::JSLock::DropAllLocks dropAllLocks(JSDOMWindowBase::commonVM());
                [view setContainingWindow:[_documentView window]];
            }
        }
    }
}

- (void)destroyPlugin:(NSView *)view
{
#if ENABLE(PLUGIN_PROXY_FOR_VIDEO)
    if ([_views containsObject:view] || [_viewsNotInDocument containsObject:view]) {
#else
    if ([_views containsObject:view]) {
#endif
        if (_started)
            [self stopOnePlugin:view];
        [self destroyOnePlugin:view];
        
#if ENABLE(NETSCAPE_PLUGIN_API)
        if (Frame* frame = core([self webFrame]))
            frame->script()->cleanupScriptObjectsForPlugin(self);
#endif
        
        [pluginViews removeObject:view];
        [[_documentView _webView] removePluginInstanceView:view];
        [_views removeObject:view];
#if ENABLE(PLUGIN_PROXY_FOR_VIDEO)
        [_viewsNotInDocument removeObject:view];
#endif
    }
}

- (void)_webPluginContainerCancelCheckIfAllowedToLoadRequest:(id)checkIdentifier
{
    [checkIdentifier cancel];
    [_checksInProgress removeObject:checkIdentifier];
}

static void cancelOutstandingCheck(const void *item, void *context)
{
    [(id)item cancel];
}

- (void)_cancelOutstandingChecks
{
    if (_checksInProgress) {
        CFSetApplyFunction((CFSetRef)_checksInProgress, cancelOutstandingCheck, NULL);
        [_checksInProgress release];
        _checksInProgress = nil;
    }
}

- (void)destroyAllPlugins
{    
    [self stopAllPlugins];

    if ([_views count] > 0) {
        LOG(Plugins, "destroying WebKit plugins: %@", [_views description]);
    }

    [self _cancelOutstandingChecks];
    
    int viewsCount = [_views count];
    for (int i = 0; i < viewsCount; i++) {
        id aView = [_views objectAtIndex:i];
        [self destroyOnePlugin:aView];
        
#if ENABLE(NETSCAPE_PLUGIN_API)
        if (Frame* frame = core([self webFrame]))
            frame->script()->cleanupScriptObjectsForPlugin(self);
#endif
        
        [pluginViews removeObject:aView];
        [[_documentView _webView] removePluginInstanceView:aView];
    }

#if ENABLE(PLUGIN_PROXY_FOR_VIDEO)
    int viewsNotInDocumentCount = [_viewsNotInDocument count];
    for (int i = 0; i < viewsNotInDocumentCount; i++)
        [self destroyOnePlugin:[_viewsNotInDocument objectAtIndex:i]];
#endif

    [_views makeObjectsPerformSelector:@selector(removeFromSuperviewWithoutNeedingDisplay)];
    [_views release];
    _views = nil;

    _documentView = nil;
}

- (id)_webPluginContainerCheckIfAllowedToLoadRequest:(NSURLRequest *)request inFrame:(NSString *)target resultObject:(id)obj selector:(SEL)selector
{
    WebPluginContainerCheck *check = [WebPluginContainerCheck checkWithRequest:request target:target resultObject:obj selector:selector controller:self contextInfo:nil];
    [_checksInProgress addObject:check];
    [check start];

    return check;
}

- (void)webPlugInContainerLoadRequest:(NSURLRequest *)request inFrame:(NSString *)target
{
    if (!request) {
        LOG_ERROR("nil URL passed");
        return;
    }
    if (!_documentView) {
        LOG_ERROR("could not load URL %@ because plug-in has already been destroyed", request);
        return;
    }
    WebFrame *frame = [_dataSource webFrame];
    if (!frame) {
        LOG_ERROR("could not load URL %@ because plug-in has already been stopped", request);
        return;
    }
    if (!target) {
        target = @"_top";
    }
    NSString *JSString = [[request URL] _webkit_scriptIfJavaScriptURL];
    if (JSString) {
        if ([frame findFrameNamed:target] != frame) {
            LOG_ERROR("JavaScript requests can only be made on the frame that contains the plug-in");
            return;
        }
        [frame _stringByEvaluatingJavaScriptFromString:JSString];
    } else {
        if (!request) {
            LOG_ERROR("could not load URL %@", [request URL]);
            return;
        }
        FrameLoadRequest frameRequest(core(frame), request);
        frameRequest.setFrameName(target);
        frameRequest.setShouldCheckNewWindowPolicy(true);
        core(frame)->loader()->load(frameRequest);
    }
}

- (void)webPlugInContainerShowStatus:(NSString *)message
{
    if (!message)
        message = @"";

    WebView *v = [_dataSource _webView];
    [[v _UIDelegateForwarder] webView:v setStatusText:message];
}

// For compatibility only.
- (void)showStatus:(NSString *)message
{
    [self webPlugInContainerShowStatus:message];
}

- (NSColor *)webPlugInContainerSelectionColor
{
    bool primary = true;
    if (Frame* frame = core([self webFrame]))
        primary = frame->selection()->isFocusedAndActive();
    return primary ? [NSColor selectedTextBackgroundColor] : [NSColor secondarySelectedControlColor];
}

// For compatibility only.
- (NSColor *)selectionColor
{
    return [self webPlugInContainerSelectionColor];
}

- (WebFrame *)webFrame
{
    return [_dataSource webFrame];
}

- (WebView *)webView
{
    return [[self webFrame] webView];
}

- (NSString *)URLPolicyCheckReferrer
{
    NSURL *responseURL = [[[[self webFrame] _dataSource] response] URL];
    ASSERT(responseURL);
    return [responseURL _web_originalDataAsString];
}

- (void)pluginView:(NSView *)pluginView receivedResponse:(NSURLResponse *)response
{    
    if ([pluginView respondsToSelector:@selector(webPlugInMainResourceDidReceiveResponse:)])
        [pluginView webPlugInMainResourceDidReceiveResponse:response];
    else {
        // Cancel the load since this plug-in does its own loading.
        // FIXME: See <rdar://problem/4258008> for a problem with this.
        NSError *error = [[NSError alloc] _initWithPluginErrorCode:WebKitErrorPlugInWillHandleLoad
                                                        contentURL:[response URL]
                                                     pluginPageURL:nil
                                                        pluginName:nil // FIXME: Get this from somewhere
                                                          MIMEType:[response MIMEType]];
        [_dataSource _documentLoader]->cancelMainResourceLoad(error);
        [error release];
    }        
}

- (void)pluginView:(NSView *)pluginView receivedData:(NSData *)data
{
    if ([pluginView respondsToSelector:@selector(webPlugInMainResourceDidReceiveData:)])
        [pluginView webPlugInMainResourceDidReceiveData:data];
}

- (void)pluginView:(NSView *)pluginView receivedError:(NSError *)error
{
    if ([pluginView respondsToSelector:@selector(webPlugInMainResourceDidFailWithError:)])
        [pluginView webPlugInMainResourceDidFailWithError:error];
}

- (void)pluginViewFinishedLoading:(NSView *)pluginView
{
    if ([pluginView respondsToSelector:@selector(webPlugInMainResourceDidFinishLoading)])
        [pluginView webPlugInMainResourceDidFinishLoading];
}

#if ENABLE(PLUGIN_PROXY_FOR_VIDEO)
static WebCore::HTMLMediaElement* mediaProxyClient(DOMElement* element)
{
    if (!element) {
        LOG_ERROR("nil element passed");
        return nil;
    }

    Element* node = core(element);
    if (!node || (!node->hasTagName(HTMLNames::videoTag) && !isHTMLAudioElement(node))) {
        LOG_ERROR("invalid media element passed");
        return nil;
    }

    return static_cast<WebCore::HTMLMediaElement*>(node);
}

- (void)_webPluginContainerSetMediaPlayerProxy:(WebMediaPlayerProxy *)proxy forElement:(DOMElement *)element
{
    WebCore::HTMLMediaElement* client = mediaProxyClient(element);
    if (client)
        client->setMediaPlayerProxy(proxy);
}

- (void)_webPluginContainerPostMediaPlayerNotification:(int)notification forElement:(DOMElement *)element
{
    WebCore::HTMLMediaElement* client = mediaProxyClient(element);
    if (client)
        client->deliverNotification((MediaPlayerProxyNotificationType)notification);
}
#endif

@end

static bool isKindOfClass(id object, NSString *className)
{
    Class cls = NSClassFromString(className);

    if (!cls)
        return false;

    return [object isKindOfClass:cls];
}


// Existing versions of the Flip4Mac WebKit plug-in have an object lifetime bug related to an NSAlert that is
// used to notify the user about updates to the plug-in. This bug can result in Safari crashing if the page
// containing the plug-in navigates while the alert is displayed (<rdar://problem/7313430>).
//
// The gist of the bug is thus: Flip4Mac sets an instance of the TSUpdateCheck class as the modal delegate of the
// NSAlert instance. This TSUpdateCheck instance itself has a delegate. The delegate is set to the WmvPlugin
// instance which is the NSView subclass that is exposed to WebKit as the plug-in view. Since this relationship
// is that of delegates the TSUpdateCheck does not retain the WmvPlugin. This leads to a bug if the WmvPlugin
// instance is destroyed before the TSUpdateCheck instance as the TSUpdateCheck instance will be left with a
// pointer to a stale object. This will happen if a page containing the Flip4Mac plug-in triggers a navigation
// while the update sheet is visible as the WmvPlugin instance is removed from the view hierarchy and there are
// no other references to keep the object alive.
//
// We work around this bug by patching the following two messages:
//
// 1) -[NSAlert beginSheetModalForWindow:modalDelegate:didEndSelector:contextInfo:]
// 2) -[TSUpdateCheck alertDidEnd:returnCode:contextInfo:]
//
// Our override of 1) detects whether it is Flip4Mac's update sheet triggering the alert by checking whether the
// modal delegate is an instance of TSUpdateCheck. If it is, it retains the modal delegate's delegate.
//
// Our override of 2) then autoreleases the delegate, balancing the retain we added in 1).
//
// These two overrides have the effect of ensuring that the WmvPlugin instance will always outlive the TSUpdateCheck
// instance, preventing the TSUpdateCheck instance from accessing a stale delegate pointer and crashing the application.


typedef void (*beginSheetModalForWindowIMP)(id, SEL, NSWindow *, id, SEL, void*);
static beginSheetModalForWindowIMP original_NSAlert_beginSheetModalForWindow_modalDelegate_didEndSelector_contextInfo_;

typedef void (*alertDidEndIMP)(id, SEL, NSAlert *, NSInteger, void*);
static alertDidEndIMP original_TSUpdateCheck_alertDidEnd_returnCode_contextInfo_;

static void WebKit_TSUpdateCheck_alertDidEnd_returnCode_contextInfo_(id object, SEL selector, NSAlert *alert, NSInteger returnCode, void* contextInfo)
{
    [[object delegate] autorelease];

    original_TSUpdateCheck_alertDidEnd_returnCode_contextInfo_(object, selector, alert, returnCode, contextInfo);
}

static void WebKit_NSAlert_beginSheetModalForWindow_modalDelegate_didEndSelector_contextInfo_(id object, SEL selector, NSWindow *window, id modalDelegate, SEL didEndSelector, void* contextInfo)
{
    if (isKindOfClass(modalDelegate, @"TSUpdateCheck"))
        [[modalDelegate delegate] retain];

    original_NSAlert_beginSheetModalForWindow_modalDelegate_didEndSelector_contextInfo_(object, selector, window, modalDelegate, didEndSelector, contextInfo);
}

static void installFlip4MacPlugInWorkaroundIfNecessary()
{
    static bool hasInstalledFlip4MacPlugInWorkaround;
    if (!hasInstalledFlip4MacPlugInWorkaround) {
        Class TSUpdateCheck = objc_lookUpClass("TSUpdateCheck");
        if (!TSUpdateCheck)
            return;

        Method methodToPatch = class_getInstanceMethod(TSUpdateCheck, @selector(alertDidEnd:returnCode:contextInfo:));
        if (!methodToPatch)
            return;

        IMP originalMethod = method_setImplementation(methodToPatch, reinterpret_cast<IMP>(WebKit_TSUpdateCheck_alertDidEnd_returnCode_contextInfo_));
        original_TSUpdateCheck_alertDidEnd_returnCode_contextInfo_ = reinterpret_cast<alertDidEndIMP>(originalMethod);

        methodToPatch = class_getInstanceMethod(objc_getRequiredClass("NSAlert"), @selector(beginSheetModalForWindow:modalDelegate:didEndSelector:contextInfo:));
        originalMethod = method_setImplementation(methodToPatch, reinterpret_cast<IMP>(WebKit_NSAlert_beginSheetModalForWindow_modalDelegate_didEndSelector_contextInfo_));
        original_NSAlert_beginSheetModalForWindow_modalDelegate_didEndSelector_contextInfo_ = reinterpret_cast<beginSheetModalForWindowIMP>(originalMethod);

        hasInstalledFlip4MacPlugInWorkaround = true;
    }
}
