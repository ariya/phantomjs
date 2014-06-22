/*
 * Copyright (C) 2005-2012 Apple Inc. All rights reserved.
 * Copyright (C) 2006 David Smith (catfish.man@gmail.com)
 * Copyright (C) 2010 Igalia S.L
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

#import "WebViewInternal.h"
#import "WebViewData.h"

#import "DOMCSSStyleDeclarationInternal.h"
#import "DOMDocumentInternal.h"
#import "DOMNodeInternal.h"
#import "DOMRangeInternal.h"
#import "WebAlternativeTextClient.h"
#import "WebApplicationCache.h"
#import "WebBackForwardListInternal.h"
#import "WebBaseNetscapePluginView.h"
#import "WebCache.h"
#import "WebChromeClient.h"
#import "WebContextMenuClient.h"
#import "WebDOMOperationsPrivate.h"
#import "WebDataSourceInternal.h"
#import "WebDatabaseManagerPrivate.h"
#import "WebDefaultEditingDelegate.h"
#import "WebDefaultPolicyDelegate.h"
#import "WebDefaultUIDelegate.h"
#import "WebDelegateImplementationCaching.h"
#import "WebDeviceOrientationClient.h"
#import "WebDeviceOrientationProvider.h"
#import "WebDocument.h"
#import "WebDocumentInternal.h"
#import "WebDownload.h"
#import "WebDownloadInternal.h"
#import "WebDragClient.h"
#import "WebDynamicScrollBarsViewInternal.h"
#import "WebEditingDelegate.h"
#import "WebEditorClient.h"
#import "WebFormDelegatePrivate.h"
#import "WebFrameInternal.h"
#import "WebFrameNetworkingContext.h"
#import "WebFrameViewInternal.h"
#import "WebFullScreenController.h"
#import "WebGeolocationClient.h"
#import "WebGeolocationPositionInternal.h"
#import "WebHTMLRepresentation.h"
#import "WebHTMLViewInternal.h"
#import "WebHistoryItemInternal.h"
#import "WebIconDatabaseInternal.h"
#import "WebInspector.h"
#import "WebInspectorClient.h"
#import "WebKitErrors.h"
#import "WebKitFullScreenListener.h"
#import "WebKitLogging.h"
#import "WebKitNSStringExtras.h"
#import "WebKitStatisticsPrivate.h"
#import "WebKitSystemBits.h"
#import "WebKitVersionChecks.h"
#import "WebLocalizableStrings.h"
#import "WebNSDataExtras.h"
#import "WebNSDataExtrasPrivate.h"
#import "WebNSDictionaryExtras.h"
#import "WebNSEventExtras.h"
#import "WebNSObjectExtras.h"
#import "WebNSPasteboardExtras.h"
#import "WebNSPrintOperationExtras.h"
#import "WebNSURLExtras.h"
#import "WebNSURLRequestExtras.h"
#import "WebNSViewExtras.h"
#import "WebNodeHighlight.h"
#import "WebNotificationClient.h"
#import "WebPDFView.h"
#import "WebPanelAuthenticationHandler.h"
#import "WebPlatformStrategies.h"
#import "WebPluginDatabase.h"
#import "WebPolicyDelegate.h"
#import "WebPreferenceKeysPrivate.h"
#import "WebPreferencesPrivate.h"
#import "WebScriptDebugDelegate.h"
#import "WebScriptWorldInternal.h"
#import "WebStorageManagerInternal.h"
#import "WebSystemInterface.h"
#import "WebTextCompletionController.h"
#import "WebTextIterator.h"
#import "WebUIDelegate.h"
#import "WebUIDelegatePrivate.h"
#import <CoreFoundation/CFSet.h>
#import <Foundation/NSURLConnection.h>
#import <JavaScriptCore/APICast.h>
#import <JavaScriptCore/JSValueRef.h>
#import <WebCore/AlternativeTextUIController.h>
#import <WebCore/AnimationController.h>
#import <WebCore/ApplicationCacheStorage.h>
#import <WebCore/BackForwardListImpl.h>
#import <WebCore/MemoryCache.h>
#import <WebCore/Chrome.h>
#import <WebCore/ColorMac.h>
#import <WebCore/Cursor.h>
#import <WebCore/DatabaseManager.h>
#import <WebCore/Document.h>
#import <WebCore/DocumentLoader.h>
#import <WebCore/DragController.h>
#import <WebCore/DragData.h>
#import <WebCore/DragSession.h>
#import <WebCore/Editor.h>
#import <WebCore/EventHandler.h>
#import <WebCore/ExceptionHandlers.h>
#import <WebCore/FocusController.h>
#import <WebCore/Frame.h>
#import <WebCore/FrameLoader.h>
#import <WebCore/FrameSelection.h>
#import <WebCore/FrameTree.h>
#import <WebCore/FrameView.h>
#import <WebCore/GCController.h>
#import <WebCore/GeolocationController.h>
#import <WebCore/GeolocationError.h>
#import <WebCore/HTMLMediaElement.h>
#import <WebCore/HTMLNames.h>
#import <WebCore/HistoryController.h>
#import <WebCore/HistoryItem.h>
#import <WebCore/IconDatabase.h>
#import <WebCore/InitializeLogging.h>
#import <WebCore/JSCSSStyleDeclaration.h>
#import <WebCore/JSDocument.h>
#import <WebCore/JSElement.h>
#import <WebCore/JSNodeList.h>
#import <WebCore/JSNotification.h>
#import <WebCore/MemoryPressureHandler.h>
#import <WebCore/MIMETypeRegistry.h>
#import <WebCore/NodeList.h>
#import <WebCore/Notification.h>
#import <WebCore/NotificationController.h>
#import <WebCore/Page.h>
#import <WebCore/PageCache.h>
#import <WebCore/PageGroup.h>
#import <WebCore/PlatformEventFactoryMac.h>
#import <WebCore/ProgressTracker.h>
#import <WebCore/RenderView.h>
#import <WebCore/RenderWidget.h>
#import <WebCore/ResourceHandle.h>
#import <WebCore/ResourceLoadScheduler.h>
#import <WebCore/ResourceRequest.h>
#import <WebCore/RunLoop.h>
#import <WebCore/RuntimeApplicationChecks.h>
#import <WebCore/RuntimeEnabledFeatures.h>
#import <WebCore/SchemeRegistry.h>
#import <WebCore/ScriptController.h>
#import <WebCore/ScriptValue.h>
#import <WebCore/SecurityOrigin.h>
#import <WebCore/SecurityPolicy.h>
#import <WebCore/Settings.h>
#import <WebCore/StylePropertySet.h>
#import <WebCore/SystemVersionMac.h>
#import <WebCore/TextResourceDecoder.h>
#import <WebCore/ThreadCheck.h>
#import <WebCore/WebCoreObjCExtras.h>
#import <WebCore/WebCoreView.h>
#import <WebCore/WebVideoFullscreenController.h>
#import <WebCore/Widget.h>
#import <WebKit/DOM.h>
#import <WebKit/DOMExtensions.h>
#import <WebKit/DOMPrivate.h>
#import <WebKitSystemInterface.h>
#import <mach-o/dyld.h>
#import <objc/objc-auto.h>
#import <objc/runtime.h>
#import <runtime/ArrayPrototype.h>
#import <runtime/DateInstance.h>
#import <runtime/InitializeThreading.h>
#import <runtime/JSLock.h>
#import <runtime/JSCJSValue.h>
#import <wtf/Assertions.h>
#import <wtf/HashTraits.h>
#import <wtf/MainThread.h>
#import <wtf/RefCountedLeakCounter.h>
#import <wtf/RefPtr.h>
#import <wtf/StdLibExtras.h>

#if ENABLE(DASHBOARD_SUPPORT)
#import <WebKit/WebDashboardRegion.h>
#endif

#if USE(GLIB)
#import <glib.h>
#endif

@interface NSSpellChecker (WebNSSpellCheckerDetails)
- (void)_preflightChosenSpellServer;
@end

@interface NSView (WebNSViewDetails)
- (NSView *)_hitTest:(NSPoint *)aPoint dragTypes:(NSSet *)types;
- (void)_autoscrollForDraggingInfo:(id)dragInfo timeDelta:(NSTimeInterval)repeatDelta;
- (BOOL)_shouldAutoscrollForDraggingInfo:(id)dragInfo;
@end

@interface NSWindow (WebNSWindowDetails)
- (id)_oldFirstResponderBeforeBecoming;
- (void)_enableScreenUpdatesIfNeeded;
- (BOOL)_wrapsCarbonWindow;
- (BOOL)_hasKeyAppearance;
@end

using namespace WebCore;
using namespace JSC;

#if defined(__ppc__) || defined(__ppc64__)
#define PROCESSOR "PPC"
#elif defined(__i386__) || defined(__x86_64__)
#define PROCESSOR "Intel"
#else
#error Unknown architecture
#endif

#define FOR_EACH_RESPONDER_SELECTOR(macro) \
macro(alignCenter) \
macro(alignJustified) \
macro(alignLeft) \
macro(alignRight) \
macro(capitalizeWord) \
macro(centerSelectionInVisibleArea) \
macro(changeAttributes) \
macro(changeBaseWritingDirection) \
macro(changeBaseWritingDirectionToLTR) \
macro(changeBaseWritingDirectionToRTL) \
macro(changeColor) \
macro(changeDocumentBackgroundColor) \
macro(changeFont) \
macro(changeSpelling) \
macro(checkSpelling) \
macro(complete) \
macro(copy) \
macro(copyFont) \
macro(cut) \
macro(delete) \
macro(deleteBackward) \
macro(deleteBackwardByDecomposingPreviousCharacter) \
macro(deleteForward) \
macro(deleteToBeginningOfLine) \
macro(deleteToBeginningOfParagraph) \
macro(deleteToEndOfLine) \
macro(deleteToEndOfParagraph) \
macro(deleteToMark) \
macro(deleteWordBackward) \
macro(deleteWordForward) \
macro(ignoreSpelling) \
macro(indent) \
macro(insertBacktab) \
macro(insertLineBreak) \
macro(insertNewline) \
macro(insertNewlineIgnoringFieldEditor) \
macro(insertParagraphSeparator) \
macro(insertTab) \
macro(insertTabIgnoringFieldEditor) \
macro(lowercaseWord) \
macro(makeBaseWritingDirectionLeftToRight) \
macro(makeBaseWritingDirectionRightToLeft) \
macro(makeTextWritingDirectionLeftToRight) \
macro(makeTextWritingDirectionNatural) \
macro(makeTextWritingDirectionRightToLeft) \
macro(moveBackward) \
macro(moveBackwardAndModifySelection) \
macro(moveDown) \
macro(moveDownAndModifySelection) \
macro(moveForward) \
macro(moveForwardAndModifySelection) \
macro(moveLeft) \
macro(moveLeftAndModifySelection) \
macro(moveParagraphBackwardAndModifySelection) \
macro(moveParagraphForwardAndModifySelection) \
macro(moveRight) \
macro(moveRightAndModifySelection) \
macro(moveToBeginningOfDocument) \
macro(moveToBeginningOfDocumentAndModifySelection) \
macro(moveToBeginningOfLine) \
macro(moveToBeginningOfLineAndModifySelection) \
macro(moveToBeginningOfParagraph) \
macro(moveToBeginningOfParagraphAndModifySelection) \
macro(moveToBeginningOfSentence) \
macro(moveToBeginningOfSentenceAndModifySelection) \
macro(moveToEndOfDocument) \
macro(moveToEndOfDocumentAndModifySelection) \
macro(moveToEndOfLine) \
macro(moveToEndOfLineAndModifySelection) \
macro(moveToEndOfParagraph) \
macro(moveToEndOfParagraphAndModifySelection) \
macro(moveToEndOfSentence) \
macro(moveToEndOfSentenceAndModifySelection) \
macro(moveToLeftEndOfLine) \
macro(moveToLeftEndOfLineAndModifySelection) \
macro(moveToRightEndOfLine) \
macro(moveToRightEndOfLineAndModifySelection) \
macro(moveUp) \
macro(moveUpAndModifySelection) \
macro(moveWordBackward) \
macro(moveWordBackwardAndModifySelection) \
macro(moveWordForward) \
macro(moveWordForwardAndModifySelection) \
macro(moveWordLeft) \
macro(moveWordLeftAndModifySelection) \
macro(moveWordRight) \
macro(moveWordRightAndModifySelection) \
macro(orderFrontSubstitutionsPanel) \
macro(outdent) \
macro(overWrite) \
macro(pageDown) \
macro(pageDownAndModifySelection) \
macro(pageUp) \
macro(pageUpAndModifySelection) \
macro(paste) \
macro(pasteAsPlainText) \
macro(pasteAsRichText) \
macro(pasteFont) \
macro(performFindPanelAction) \
macro(scrollLineDown) \
macro(scrollLineUp) \
macro(scrollPageDown) \
macro(scrollPageUp) \
macro(scrollToBeginningOfDocument) \
macro(scrollToEndOfDocument) \
macro(selectAll) \
macro(selectLine) \
macro(selectParagraph) \
macro(selectSentence) \
macro(selectToMark) \
macro(selectWord) \
macro(setMark) \
macro(showGuessPanel) \
macro(startSpeaking) \
macro(stopSpeaking) \
macro(subscript) \
macro(superscript) \
macro(swapWithMark) \
macro(takeFindStringFromSelection) \
macro(toggleBaseWritingDirection) \
macro(transpose) \
macro(underline) \
macro(unscript) \
macro(uppercaseWord) \
macro(yank) \
macro(yankAndSelect) \

#define WebKitOriginalTopPrintingMarginKey @"WebKitOriginalTopMargin"
#define WebKitOriginalBottomPrintingMarginKey @"WebKitOriginalBottomMargin"

#define KeyboardUIModeDidChangeNotification @"com.apple.KeyboardUIModeDidChange"
#define AppleKeyboardUIMode CFSTR("AppleKeyboardUIMode")
#define UniversalAccessDomain CFSTR("com.apple.universalaccess")

static BOOL s_didSetCacheModel;
static WebCacheModel s_cacheModel = WebCacheModelDocumentViewer;

#ifndef NDEBUG
static const char webViewIsOpen[] = "At least one WebView is still open.";
#endif

@interface NSObject (WebValidateWithoutDelegate)
- (BOOL)validateUserInterfaceItemWithoutDelegate:(id <NSValidatedUserInterfaceItem>)item;
@end

@interface _WebSafeForwarder : NSObject
{
    id target; // Non-retained. Don't retain delegates.
    id defaultTarget;
}
- (id)initWithTarget:(id)target defaultTarget:(id)defaultTarget;
@end

FindOptions coreOptions(WebFindOptions options)
{
    return (options & WebFindOptionsCaseInsensitive ? CaseInsensitive : 0)
        | (options & WebFindOptionsAtWordStarts ? AtWordStarts : 0)
        | (options & WebFindOptionsTreatMedialCapitalAsWordStart ? TreatMedialCapitalAsWordStart : 0)
        | (options & WebFindOptionsBackwards ? Backwards : 0)
        | (options & WebFindOptionsWrapAround ? WrapAround : 0)
        | (options & WebFindOptionsStartInSelection ? StartInSelection : 0);
}

LayoutMilestones coreLayoutMilestones(WebLayoutMilestones milestones)
{
    return (milestones & WebDidFirstLayout ? DidFirstLayout : 0)
        | (milestones & WebDidFirstVisuallyNonEmptyLayout ? DidFirstVisuallyNonEmptyLayout : 0)
        | (milestones & WebDidHitRelevantRepaintedObjectsAreaThreshold ? DidHitRelevantRepaintedObjectsAreaThreshold : 0);
}

WebLayoutMilestones kitLayoutMilestones(LayoutMilestones milestones)
{
    return (milestones & DidFirstLayout ? WebDidFirstLayout : 0)
        | (milestones & DidFirstVisuallyNonEmptyLayout ? WebDidFirstVisuallyNonEmptyLayout : 0)
        | (milestones & DidHitRelevantRepaintedObjectsAreaThreshold ? WebDidHitRelevantRepaintedObjectsAreaThreshold : 0);
}

static PageVisibilityState core(WebPageVisibilityState visibilityState)
{
    switch (visibilityState) {
    case WebPageVisibilityStateVisible:
        return PageVisibilityStateVisible;
    case WebPageVisibilityStateHidden:
        return PageVisibilityStateHidden;
    case WebPageVisibilityStatePrerender:
        return PageVisibilityStatePrerender;
    case WebPageVisibilityStateUnloaded:
        return PageVisibilityStateUnloaded;
    }

    ASSERT_NOT_REACHED();
    return PageVisibilityStateVisible;
}

static WebPageVisibilityState kit(PageVisibilityState visibilityState)
{
    switch (visibilityState) {
    case PageVisibilityStateVisible:
        return WebPageVisibilityStateVisible;
    case PageVisibilityStateHidden:
        return WebPageVisibilityStateHidden;
    case PageVisibilityStatePrerender:
        return WebPageVisibilityStatePrerender;
    case PageVisibilityStateUnloaded:
        return WebPageVisibilityStateUnloaded;
    }

    ASSERT_NOT_REACHED();
    return WebPageVisibilityStateVisible;
}

@interface WebView (WebFileInternal)
- (float)_deviceScaleFactor;
- (BOOL)_isLoading;
- (WebFrameView *)_frameViewAtWindowPoint:(NSPoint)point;
- (WebFrame *)_focusedFrame;
+ (void)_preflightSpellChecker;
- (BOOL)_continuousCheckingAllowed;
- (NSResponder *)_responderForResponderOperations;
#if USE(GLIB)
- (void)_clearGlibLoopObserver;
#endif
@end

NSString *WebElementDOMNodeKey =            @"WebElementDOMNode";
NSString *WebElementFrameKey =              @"WebElementFrame";
NSString *WebElementImageKey =              @"WebElementImage";
NSString *WebElementImageAltStringKey =     @"WebElementImageAltString";
NSString *WebElementImageRectKey =          @"WebElementImageRect";
NSString *WebElementImageURLKey =           @"WebElementImageURL";
NSString *WebElementIsSelectedKey =         @"WebElementIsSelected";
NSString *WebElementLinkLabelKey =          @"WebElementLinkLabel";
NSString *WebElementLinkTargetFrameKey =    @"WebElementTargetFrame";
NSString *WebElementLinkTitleKey =          @"WebElementLinkTitle";
NSString *WebElementLinkURLKey =            @"WebElementLinkURL";
NSString *WebElementMediaURLKey =           @"WebElementMediaURL";
NSString *WebElementSpellingToolTipKey =    @"WebElementSpellingToolTip";
NSString *WebElementTitleKey =              @"WebElementTitle";
NSString *WebElementLinkIsLiveKey =         @"WebElementLinkIsLive";
NSString *WebElementIsInScrollBarKey =      @"WebElementIsInScrollBar";
NSString *WebElementIsContentEditableKey =  @"WebElementIsContentEditableKey";

NSString *WebViewProgressStartedNotification =          @"WebProgressStartedNotification";
NSString *WebViewProgressEstimateChangedNotification =  @"WebProgressEstimateChangedNotification";
NSString *WebViewProgressFinishedNotification =         @"WebProgressFinishedNotification";

NSString * const WebViewDidBeginEditingNotification =         @"WebViewDidBeginEditingNotification";
NSString * const WebViewDidChangeNotification =               @"WebViewDidChangeNotification";
NSString * const WebViewDidEndEditingNotification =           @"WebViewDidEndEditingNotification";
NSString * const WebViewDidChangeTypingStyleNotification =    @"WebViewDidChangeTypingStyleNotification";
NSString * const WebViewDidChangeSelectionNotification =      @"WebViewDidChangeSelectionNotification";

enum { WebViewVersion = 4 };

#define timedLayoutSize 4096

static NSMutableSet *schemesWithRepresentationsSet;

NSString *_WebCanGoBackKey =            @"canGoBack";
NSString *_WebCanGoForwardKey =         @"canGoForward";
NSString *_WebEstimatedProgressKey =    @"estimatedProgress";
NSString *_WebIsLoadingKey =            @"isLoading";
NSString *_WebMainFrameIconKey =        @"mainFrameIcon";
NSString *_WebMainFrameTitleKey =       @"mainFrameTitle";
NSString *_WebMainFrameURLKey =         @"mainFrameURL";
NSString *_WebMainFrameDocumentKey =    @"mainFrameDocument";

NSString *_WebViewDidStartAcceleratedCompositingNotification = @"_WebViewDidStartAcceleratedCompositing";

NSString *WebKitKerningAndLigaturesEnabledByDefaultDefaultsKey = @"WebKitKerningAndLigaturesEnabledByDefault";

@interface WebProgressItem : NSObject
{
@public
    long long bytesReceived;
    long long estimatedLength;
}
@end

@implementation WebProgressItem
@end

static BOOL continuousSpellCheckingEnabled;
static BOOL grammarCheckingEnabled;
static BOOL automaticQuoteSubstitutionEnabled;
static BOOL automaticLinkDetectionEnabled;
static BOOL automaticDashSubstitutionEnabled;
static BOOL automaticTextReplacementEnabled;
static BOOL automaticSpellingCorrectionEnabled;

@implementation WebView (AllWebViews)

static CFSetCallBacks NonRetainingSetCallbacks = {
    0,
    NULL,
    NULL,
    CFCopyDescription,
    CFEqual,
    CFHash
};

static CFMutableSetRef allWebViewsSet;

+ (void)_makeAllWebViewsPerformSelector:(SEL)selector
{
    if (!allWebViewsSet)
        return;

    [(NSMutableSet *)allWebViewsSet makeObjectsPerformSelector:selector];
}

- (void)_removeFromAllWebViewsSet
{
    if (allWebViewsSet)
        CFSetRemoveValue(allWebViewsSet, self);
}

- (void)_addToAllWebViewsSet
{
    if (!allWebViewsSet)
        allWebViewsSet = CFSetCreateMutable(NULL, 0, &NonRetainingSetCallbacks);

    CFSetSetValue(allWebViewsSet, self);
}

@end

@implementation WebView (WebPrivate)

static NSString *systemMarketingVersionForUserAgentString()
{
    // Use underscores instead of dots because when we first added the Mac OS X version to the user agent string
    // we were concerned about old DHTML libraries interpreting "4." as Netscape 4. That's no longer a concern for us
    // but we're sticking with the underscores for compatibility with the format used by older versions of Safari.
    return [systemMarketingVersion() stringByReplacingOccurrencesOfString:@"." withString:@"_"];
}

static NSString *createUserVisibleWebKitVersionString()
{
    // If the version is 4 digits long or longer, then the first digit represents
    // the version of the OS. Our user agent string should not include this first digit,
    // so strip it off and report the rest as the version. <rdar://problem/4997547>
    NSString *fullVersion = [[NSBundle bundleForClass:[WebView class]] objectForInfoDictionaryKey:(NSString *)kCFBundleVersionKey];
    NSRange nonDigitRange = [fullVersion rangeOfCharacterFromSet:[[NSCharacterSet decimalDigitCharacterSet] invertedSet]];
    if (nonDigitRange.location == NSNotFound && [fullVersion length] >= 4)
        return [[fullVersion substringFromIndex:1] copy];
    if (nonDigitRange.location != NSNotFound && nonDigitRange.location >= 4)
        return [[fullVersion substringFromIndex:1] copy];
    return [fullVersion copy];
}

+ (NSString *)_standardUserAgentWithApplicationName:(NSString *)applicationName
{
    // Note: Do *not* move the initialization of osVersion nor webKitVersion into the declaration.
    // Garbage collection won't correctly mark the global variable in that case <rdar://problem/5733674>.
    static NSString *osVersion;
    static NSString *webKitVersion;
    if (!osVersion)
        osVersion = [systemMarketingVersionForUserAgentString() retain];
    if (!webKitVersion)
        webKitVersion = createUserVisibleWebKitVersionString();
    if ([applicationName length])
        return [NSString stringWithFormat:@"Mozilla/5.0 (Macintosh; " PROCESSOR " Mac OS X %@) AppleWebKit/%@ (KHTML, like Gecko) %@", osVersion, webKitVersion, applicationName];
    return [NSString stringWithFormat:@"Mozilla/5.0 (Macintosh; " PROCESSOR " Mac OS X %@) AppleWebKit/%@ (KHTML, like Gecko)", osVersion, webKitVersion];
}

+ (void)_reportException:(JSValueRef)exception inContext:(JSContextRef)context
{
    if (!exception || !context)
        return;

    JSC::ExecState* execState = toJS(context);
    JSLockHolder lock(execState);

    // Make sure the context has a DOMWindow global object, otherwise this context didn't originate from a WebView.
    if (!toJSDOMWindow(execState->lexicalGlobalObject()))
        return;

    reportException(execState, toJS(execState, exception));
}

static void WebKitInitializeApplicationCachePathIfNecessary()
{
    static BOOL initialized = NO;
    if (initialized)
        return;

    NSString *appName = [[NSBundle mainBundle] bundleIdentifier];
    if (!appName)
        appName = [[NSProcessInfo processInfo] processName];

    ASSERT(appName);

    NSString* cacheDir = [NSString _webkit_localCacheDirectoryWithBundleIdentifier:appName];

    cacheStorage().setCacheDirectory(cacheDir);
    initialized = YES;
}

static bool shouldEnableLoadDeferring()
{
    return !applicationIsAdobeInstaller();
}

static bool shouldRestrictWindowFocus()
{
    return !applicationIsHRBlock();
}

- (void)_dispatchPendingLoadRequests
{
    resourceLoadScheduler()->servePendingRequests();
}

- (void)_registerDraggedTypes
{
    NSArray *editableTypes = [WebHTMLView _insertablePasteboardTypes];
    NSArray *URLTypes = [NSPasteboard _web_dragTypesForURL];
    NSMutableSet *types = [[NSMutableSet alloc] initWithArray:editableTypes];
    [types addObjectsFromArray:URLTypes];
    [self registerForDraggedTypes:[types allObjects]];
    [types release];
}

#if __MAC_OS_X_VERSION_MIN_REQUIRED == 1060
// This method should be removed once we no longer want to keep Safari 5.0.x working with nightly builds.
- (BOOL)_usesDocumentViews
{
    return true;
}
#endif

static bool needsOutlookQuirksScript()
{
    static bool isOutlookNeedingQuirksScript = !WebKitLinkedOnOrAfter(WEBKIT_FIRST_VERSION_WITH_HTML5_PARSER)
        && applicationIsMicrosoftOutlook();
    return isOutlookNeedingQuirksScript;
}

static NSString *leakOutlookQuirksUserScriptContents()
{
    NSString *scriptPath = [[NSBundle bundleForClass:[WebView class]] pathForResource:@"OutlookQuirksUserScript" ofType:@"js"];
    NSStringEncoding encoding;
    return [[NSString alloc] initWithContentsOfFile:scriptPath usedEncoding:&encoding error:0];
}

-(void)_injectOutlookQuirksScript
{
    static NSString *outlookQuirksScriptContents = leakOutlookQuirksUserScriptContents();
    core(self)->group().addUserScriptToWorld(core([WebScriptWorld world]),
        outlookQuirksScriptContents, KURL(), Vector<String>(), Vector<String>(), InjectAtDocumentEnd, InjectInAllFrames);
}

static bool shouldRespectPriorityInCSSAttributeSetters()
{
    static bool isIAdProducerNeedingAttributeSetterQuirk = !WebKitLinkedOnOrAfter(WEBKIT_FIRST_VERSION_WITH_CSS_ATTRIBUTE_SETTERS_IGNORING_PRIORITY)
        && [[[NSBundle mainBundle] bundleIdentifier] isEqualToString:@"com.apple.iAdProducer"];
    return isIAdProducerNeedingAttributeSetterQuirk;
}

static bool shouldUseLegacyBackgroundSizeShorthandBehavior()
{
    static bool shouldUseLegacyBackgroundSizeShorthandBehavior = applicationIsVersions()
        && !WebKitLinkedOnOrAfter(WEBKIT_FIRST_VERSION_WITHOUT_LEGACY_BACKGROUNDSIZE_SHORTHAND_BEHAVIOR);
    return shouldUseLegacyBackgroundSizeShorthandBehavior;
}

- (void)_commonInitializationWithFrameName:(NSString *)frameName groupName:(NSString *)groupName
{
    WebCoreThreadViolationCheckRoundTwo();

#ifndef NDEBUG
    WTF::RefCountedLeakCounter::suppressMessages(webViewIsOpen);
#endif
    
    WebPreferences *standardPreferences = [WebPreferences standardPreferences];
    [standardPreferences willAddToWebView];

    _private->preferences = [standardPreferences retain];
    _private->mainFrameDocumentReady = NO;
    _private->drawsBackground = YES;
    _private->backgroundColor = [[NSColor colorWithDeviceWhite:1 alpha:1] retain];
    _private->includesFlattenedCompositingLayersWhenDrawingToBitmap = YES;

    NSRect f = [self frame];
    WebFrameView *frameView = [[WebFrameView alloc] initWithFrame: NSMakeRect(0,0,f.size.width,f.size.height)];
    [frameView setAutoresizingMask:NSViewWidthSizable | NSViewHeightSizable];
    [self addSubview:frameView];
    [frameView release];

    static bool didOneTimeInitialization = false;
    if (!didOneTimeInitialization) {
#if !LOG_DISABLED
        WebKitInitializeLoggingChannelsIfNecessary();
        WebCore::initializeLoggingChannelsIfNecessary();
#endif // !LOG_DISABLED

        // Initialize our platform strategies first before invoking the rest
        // of the initialization code which may depend on the strategies.
        WebPlatformStrategies::initializeIfNecessary();

#if ENABLE(SQL_DATABASE)
        [WebDatabaseManager sharedWebDatabaseManager];
#endif

        WebKitInitializeStorageIfNecessary();
        WebKitInitializeApplicationCachePathIfNecessary();
        
        Settings::setDefaultMinDOMTimerInterval(0.004);
        
        Settings::setShouldRespectPriorityInCSSAttributeSetters(shouldRespectPriorityInCSSAttributeSetters());

        didOneTimeInitialization = true;
    }

    Page::PageClients pageClients;
    pageClients.chromeClient = new WebChromeClient(self);
    pageClients.contextMenuClient = new WebContextMenuClient(self);
    pageClients.editorClient = new WebEditorClient(self);
#if ENABLE(DRAG_SUPPORT)
    pageClients.dragClient = new WebDragClient(self);
#endif
    pageClients.inspectorClient = new WebInspectorClient(self);
    pageClients.alternativeTextClient = new WebAlternativeTextClient(self);
    _private->page = new Page(pageClients);
#if ENABLE(GEOLOCATION)
    WebCore::provideGeolocationTo(_private->page, new WebGeolocationClient(self));
#endif
#if ENABLE(NOTIFICATIONS) || ENABLE(LEGACY_NOTIFICATIONS)
    WebCore::provideNotification(_private->page, new WebNotificationClient(self));
#endif
#if ENABLE(DEVICE_ORIENTATION)
    WebCore::provideDeviceOrientationTo(_private->page, new WebDeviceOrientationClient(self));
#endif

    _private->page->setCanStartMedia([self window]);
    _private->page->settings()->setLocalStorageDatabasePath([[self preferences] _localStorageDatabasePath]);
    _private->page->settings()->setUseLegacyBackgroundSizeShorthandBehavior(shouldUseLegacyBackgroundSizeShorthandBehavior());

    if (needsOutlookQuirksScript()) {
        _private->page->settings()->setShouldInjectUserScriptsInInitialEmptyDocument(true);
        [self _injectOutlookQuirksScript];
    }

    if ([[NSUserDefaults standardUserDefaults] objectForKey:WebSmartInsertDeleteEnabled])
        [self setSmartInsertDeleteEnabled:[[NSUserDefaults standardUserDefaults] boolForKey:WebSmartInsertDeleteEnabled]];

    [WebFrame _createMainFrameWithPage:_private->page frameName:frameName frameView:frameView];

    NSRunLoop *runLoop = [NSRunLoop mainRunLoop];

    if (WebKitLinkedOnOrAfter(WEBKIT_FIRST_VERSION_WITH_LOADING_DURING_COMMON_RUNLOOP_MODES))
        [self scheduleInRunLoop:runLoop forMode:(NSString *)kCFRunLoopCommonModes];
    else
        [self scheduleInRunLoop:runLoop forMode:NSDefaultRunLoopMode];

    [self _addToAllWebViewsSet];
    [self setGroupName:groupName];
    
    // If there's already a next key view (e.g., from a nib), wire it up to our
    // contained frame view. In any case, wire our next key view up to the our
    // contained frame view. This works together with our becomeFirstResponder 
    // and setNextKeyView overrides.
    NSView *nextKeyView = [self nextKeyView];
    if (nextKeyView && nextKeyView != frameView)
        [frameView setNextKeyView:nextKeyView];
    [super setNextKeyView:frameView];

    if ([[self class] shouldIncludeInWebKitStatistics])
        ++WebViewCount;

    [self _registerDraggedTypes];

    [self _setVisibilityState:([self _isViewVisible] ? WebPageVisibilityStateVisible : WebPageVisibilityStateHidden) isInitialState:YES];

    WebPreferences *prefs = [self preferences];
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(_preferencesChangedNotification:)
                                                 name:WebPreferencesChangedInternalNotification object:prefs];

    [self _preferencesChanged:[self preferences]];
    [[self preferences] _postPreferencesChangedAPINotification];

    memoryPressureHandler().install();

    if (!WebKitLinkedOnOrAfter(WEBKIT_FIRST_VERSION_WITH_LOCAL_RESOURCE_SECURITY_RESTRICTION)) {
        // Originally, we allowed all local loads.
        SecurityPolicy::setLocalLoadPolicy(SecurityPolicy::AllowLocalLoadsForAll);
    } else if (!WebKitLinkedOnOrAfter(WEBKIT_FIRST_VERSION_WITH_MORE_STRICT_LOCAL_RESOURCE_SECURITY_RESTRICTION)) {
        // Later, we allowed local loads for local URLs and documents loaded
        // with substitute data.
        SecurityPolicy::setLocalLoadPolicy(SecurityPolicy::AllowLocalLoadsForLocalAndSubstituteData);
    }

    if (!WebKitLinkedOnOrAfter(WEBKIT_FIRST_VERSION_WITHOUT_CONTENT_SNIFFING_FOR_FILE_URLS))
        ResourceHandle::forceContentSniffing();

#if USE(GLIB)
    [self _scheduleGlibContextIterations];
#endif
}

- (id)_initWithFrame:(NSRect)f frameName:(NSString *)frameName groupName:(NSString *)groupName usesDocumentViews:(BOOL)usesDocumentViews
{
    // FIXME: Remove the usesDocumentViews parameter; it's only here for compatibility with WebKit nightly builds
    // running against Safari 5 on Leopard.
    ASSERT(usesDocumentViews);

    self = [super initWithFrame:f];
    if (!self)
        return nil;

#ifdef ENABLE_WEBKIT_UNSET_DYLD_FRAMEWORK_PATH
    // DYLD_FRAMEWORK_PATH is used so Safari will load the development version of WebKit, which
    // may not work with other WebKit applications.  Unsetting DYLD_FRAMEWORK_PATH removes the
    // need for Safari to unset it to prevent it from being passed to applications it launches.
    // Unsetting it when a WebView is first created is as good a place as any.
    // See <http://bugs.webkit.org/show_bug.cgi?id=4286> for more details.
    if (getenv("WEBKIT_UNSET_DYLD_FRAMEWORK_PATH")) {
        unsetenv("DYLD_FRAMEWORK_PATH");
        unsetenv("WEBKIT_UNSET_DYLD_FRAMEWORK_PATH");
    }
#endif

    _private = [[WebViewPrivate alloc] init];
    [self _commonInitializationWithFrameName:frameName groupName:groupName];
    [self setMaintainsBackForwardList: YES];
    _private->page->setDeviceScaleFactor([self _deviceScaleFactor]);
    return self;
}

- (void)_viewWillDrawInternal
{
    Frame* frame = [self _mainCoreFrame];
    if (frame && frame->view())
        frame->view()->updateLayoutAndStyleIfNeededRecursive();
}

+ (NSArray *)_supportedMIMETypes
{
    // Load the plug-in DB allowing plug-ins to install types.
    [WebPluginDatabase sharedDatabase];
    return [[WebFrameView _viewTypesAllowImageTypeOmission:NO] allKeys];
}

+ (NSArray *)_supportedFileExtensions
{
    NSMutableSet *extensions = [[NSMutableSet alloc] init];
    NSArray *MIMETypes = [self _supportedMIMETypes];
    NSEnumerator *enumerator = [MIMETypes objectEnumerator];
    NSString *MIMEType;
    while ((MIMEType = [enumerator nextObject]) != nil) {
        NSArray *extensionsForType = WKGetExtensionsForMIMEType(MIMEType);
        if (extensionsForType) {
            [extensions addObjectsFromArray:extensionsForType];
        }
    }
    NSArray *uniqueExtensions = [extensions allObjects];
    [extensions release];
    return uniqueExtensions;
}

static NSMutableSet *knownPluginMIMETypes()
{
    static NSMutableSet *mimeTypes = [[NSMutableSet alloc] init];
    
    return mimeTypes;
}

+ (void)_registerPluginMIMEType:(NSString *)MIMEType
{
    [WebView registerViewClass:[WebHTMLView class] representationClass:[WebHTMLRepresentation class] forMIMEType:MIMEType];
    [knownPluginMIMETypes() addObject:MIMEType];
}

+ (void)_unregisterPluginMIMEType:(NSString *)MIMEType
{
    [self _unregisterViewClassAndRepresentationClassForMIMEType:MIMEType];
    [knownPluginMIMETypes() removeObject:MIMEType];
}

+ (BOOL)_viewClass:(Class *)vClass andRepresentationClass:(Class *)rClass forMIMEType:(NSString *)MIMEType allowingPlugins:(BOOL)allowPlugins
{
    MIMEType = [MIMEType lowercaseString];
    Class viewClass = [[WebFrameView _viewTypesAllowImageTypeOmission:YES] _webkit_objectForMIMEType:MIMEType];
    Class repClass = [[WebDataSource _repTypesAllowImageTypeOmission:YES] _webkit_objectForMIMEType:MIMEType];

    if (!viewClass || !repClass || [[WebPDFView supportedMIMETypes] containsObject:MIMEType]) {
        // Our optimization to avoid loading the plug-in DB and image types for the HTML case failed.

        if (allowPlugins) {
            // Load the plug-in DB allowing plug-ins to install types.
            [WebPluginDatabase sharedDatabase];
        }

        // Load the image types and get the view class and rep class. This should be the fullest picture of all handled types.
        viewClass = [[WebFrameView _viewTypesAllowImageTypeOmission:NO] _webkit_objectForMIMEType:MIMEType];
        repClass = [[WebDataSource _repTypesAllowImageTypeOmission:NO] _webkit_objectForMIMEType:MIMEType];
    }
    
    if (viewClass && repClass) {
        if (viewClass == [WebHTMLView class] && repClass == [WebHTMLRepresentation class]) {
            // Special-case WebHTMLView for text types that shouldn't be shown.
            if ([[WebHTMLView unsupportedTextMIMETypes] containsObject:MIMEType])
                return NO;

            // If the MIME type is a known plug-in we might not want to load it.
            if (!allowPlugins && [knownPluginMIMETypes() containsObject:MIMEType]) {
                BOOL isSupportedByWebKit = [[WebHTMLView supportedNonImageMIMETypes] containsObject:MIMEType] ||
                    [[WebHTMLView supportedMIMETypes] containsObject:MIMEType];
                
                // If this is a known plug-in MIME type and WebKit can't show it natively, we don't want to show it.
                if (!isSupportedByWebKit)
                    return NO;
            }
        }
        if (vClass)
            *vClass = viewClass;
        if (rClass)
            *rClass = repClass;
        return YES;
    }
    
    return NO;
}

- (BOOL)_viewClass:(Class *)vClass andRepresentationClass:(Class *)rClass forMIMEType:(NSString *)MIMEType
{
    if ([[self class] _viewClass:vClass andRepresentationClass:rClass forMIMEType:MIMEType allowingPlugins:[_private->preferences arePlugInsEnabled]])
        return YES;

    if (_private->pluginDatabase) {
        WebBasePluginPackage *pluginPackage = [_private->pluginDatabase pluginForMIMEType:MIMEType];
        if (pluginPackage) {
            if (vClass)
                *vClass = [WebHTMLView class];
            if (rClass)
                *rClass = [WebHTMLRepresentation class];
            return YES;
        }
    }
    
    return NO;
}

+ (void)_setAlwaysUseATSU:(BOOL)f
{
    [self _setAlwaysUsesComplexTextCodePath:f];
}

+ (void)_setAlwaysUsesComplexTextCodePath:(BOOL)f
{
    Font::setCodePath(f ? Font::Complex : Font::Auto);
}

+ (void)_setAllowsRoundingHacks:(BOOL)allowsRoundingHacks
{
    TextRun::setAllowsRoundingHacks(allowsRoundingHacks);
}

+ (BOOL)_allowsRoundingHacks
{
    return TextRun::allowsRoundingHacks();
}

+ (BOOL)canCloseAllWebViews
{
    return DOMWindow::dispatchAllPendingBeforeUnloadEvents();
}

+ (void)closeAllWebViews
{
    DOMWindow::dispatchAllPendingUnloadEvents();

    // This will close the WebViews in a random order. Change this if close order is important.
    // Make a new set to avoid mutating the set we are enumerating.
    NSSet *webViewsToClose = [NSSet setWithSet:(NSSet *)allWebViewsSet]; 
    NSEnumerator *enumerator = [webViewsToClose objectEnumerator];
    while (WebView *webView = [enumerator nextObject])
        [webView close];
}

+ (BOOL)canShowFile:(NSString *)path
{
    return [[self class] canShowMIMEType:[WebView _MIMETypeForFile:path]];
}

+ (NSString *)suggestedFileExtensionForMIMEType:(NSString *)type
{
    return WKGetPreferredExtensionForMIMEType(type);
}

- (BOOL)_isClosed
{
    return !_private || _private->closed;
}

- (void)_closePluginDatabases
{
    pluginDatabaseClientCount--;

    // Close both sets of plug-in databases because plug-ins need an opportunity to clean up files, etc.

    // Unload the WebView local plug-in database. 
    if (_private->pluginDatabase) {
        [_private->pluginDatabase destroyAllPluginInstanceViews];
        [_private->pluginDatabase close];
        [_private->pluginDatabase release];
        _private->pluginDatabase = nil;
    }
    
    // Keep the global plug-in database active until the app terminates to avoid having to reload plug-in bundles.
    if (!pluginDatabaseClientCount && applicationIsTerminating)
        [WebPluginDatabase closeSharedDatabase];
}

- (void)_closeWithFastTeardown 
{
#ifndef NDEBUG
    WTF::RefCountedLeakCounter::suppressMessages("At least one WebView was closed with fast teardown.");
#endif

    [[NSDistributedNotificationCenter defaultCenter] removeObserver:self];
    [[NSNotificationCenter defaultCenter] removeObserver:self];

    [self _closePluginDatabases];
}

static bool fastDocumentTeardownEnabled()
{
#ifdef NDEBUG
    static bool enabled = ![[NSUserDefaults standardUserDefaults] boolForKey:WebKitEnableFullDocumentTeardownPreferenceKey];
#else
    static bool initialized = false;
    static bool enabled = false;
    if (!initialized) {
        // This allows debug builds to default to not have fast teardown, so leak checking still works.
        // But still allow the WebKitEnableFullDocumentTeardown default to override it if present.
        NSNumber *setting = [[NSUserDefaults standardUserDefaults] objectForKey:WebKitEnableFullDocumentTeardownPreferenceKey];
        if (setting)
            enabled = ![setting boolValue];
        initialized = true;
    }
#endif
    return enabled;
}

// _close is here only for backward compatibility; clients and subclasses should use
// public method -close instead.
- (void)_close
{
    if (!_private || _private->closed)
        return;

    _private->closed = YES;
    [self _removeFromAllWebViewsSet];

#ifndef NDEBUG
    WTF::RefCountedLeakCounter::cancelMessageSuppression(webViewIsOpen);
#endif

    // To quit the apps fast we skip document teardown, except plugins
    // need to be destroyed and unloaded.
    if (applicationIsTerminating && fastDocumentTeardownEnabled()) {
        [self _closeWithFastTeardown];
        return;
    }

#if ENABLE(VIDEO)
    [self _exitFullscreen];
#endif

    if (Frame* mainFrame = [self _mainCoreFrame])
        mainFrame->loader()->detachFromParent();

    [self setHostWindow:nil];

    [self setDownloadDelegate:nil];
    [self setEditingDelegate:nil];
    [self setFrameLoadDelegate:nil];
    [self setPolicyDelegate:nil];
    [self setResourceLoadDelegate:nil];
    [self setScriptDebugDelegate:nil];
    [self setUIDelegate:nil];

    [_private->inspector webViewClosed];

    // To avoid leaks, call removeDragCaret in case it wasn't called after moveDragCaretToPoint.
    [self removeDragCaret];

    // Deleteing the WebCore::Page will clear the page cache so we call destroy on 
    // all the plug-ins in the page cache to break any retain cycles.
    // See comment in HistoryItem::releaseAllPendingPageCaches() for more information.
    Page* page = _private->page;
    _private->page = 0;
    delete page;

    if (_private->hasSpellCheckerDocumentTag) {
        [[NSSpellChecker sharedSpellChecker] closeSpellDocumentWithTag:_private->spellCheckerDocumentTag];
        _private->hasSpellCheckerDocumentTag = NO;
    }

#if USE(ACCELERATED_COMPOSITING)
    if (_private->layerFlushController) {
        _private->layerFlushController->invalidate();
        _private->layerFlushController = nullptr;
    }
#endif
    
#if USE(GLIB)
    [self _clearGlibLoopObserver];
#endif

    [[self _notificationProvider] unregisterWebView:self];

    [[NSDistributedNotificationCenter defaultCenter] removeObserver:self];
    [[NSNotificationCenter defaultCenter] removeObserver:self];

    [WebPreferences _removeReferenceForIdentifier:[self preferencesIdentifier]];

    WebPreferences *preferences = _private->preferences;
    _private->preferences = nil;
    [preferences didRemoveFromWebView];
    [preferences release];

    [self _closePluginDatabases];

#ifndef NDEBUG
    // Need this to make leak messages accurate.
    if (applicationIsTerminating) {
        gcController().garbageCollectNow();
        [WebCache setDisabled:YES];
    }
#endif
}

// Indicates if the WebView is in the midst of a user gesture.
- (BOOL)_isProcessingUserGesture
{
    return ScriptController::processingUserGesture();
}

+ (NSString *)_MIMETypeForFile:(NSString *)path
{
    NSString *extension = [path pathExtension];
    NSString *MIMEType = nil;

    // Get the MIME type from the extension.
    if ([extension length] != 0) {
        MIMEType = WKGetMIMETypeForExtension(extension);
    }

    // If we can't get a known MIME type from the extension, sniff.
    if ([MIMEType length] == 0 || [MIMEType isEqualToString:@"application/octet-stream"]) {
        NSFileHandle *handle = [NSFileHandle fileHandleForReadingAtPath:path];
        NSData *data = [handle readDataOfLength:WEB_GUESS_MIME_TYPE_PEEK_LENGTH];
        [handle closeFile];
        if ([data length] != 0) {
            MIMEType = [data _webkit_guessedMIMEType];
        }
        if ([MIMEType length] == 0) {
            MIMEType = @"application/octet-stream";
        }
    }

    return MIMEType;
}

- (WebDownload *)_downloadURL:(NSURL *)URL
{
    ASSERT(URL);
    
    NSURLRequest *request = [[NSURLRequest alloc] initWithURL:URL];
    WebDownload *download = [WebDownload _downloadWithRequest:request
                                                     delegate:_private->downloadDelegate
                                                    directory:nil];
    [request release];
    
    return download;
}

- (WebView *)_openNewWindowWithRequest:(NSURLRequest *)request
{
    NSDictionary *features = [[NSDictionary alloc] init];
    WebView *newWindowWebView = [[self _UIDelegateForwarder] webView:self
                                            createWebViewWithRequest:nil
                                                      windowFeatures:features];
    [features release];
    if (!newWindowWebView)
        return nil;

    CallUIDelegate(newWindowWebView, @selector(webViewShow:));
    return newWindowWebView;
}

- (WebInspector *)inspector
{
    if (!_private->inspector)
        _private->inspector = [[WebInspector alloc] initWithWebView:self];
    return _private->inspector;
}

- (WebCore::Page*)page
{
    return _private->page;
}

- (NSMenu *)_menuForElement:(NSDictionary *)element defaultItems:(NSArray *)items
{
    NSArray *defaultMenuItems = [[WebDefaultUIDelegate sharedUIDelegate] webView:self contextMenuItemsForElement:element defaultMenuItems:items];
    NSArray *menuItems = defaultMenuItems;

    // CallUIDelegate returns nil if UIDelegate is nil or doesn't respond to the selector. So we need to check that here
    // to distinguish between using defaultMenuItems or the delegate really returning nil to say "no context menu".
    SEL selector = @selector(webView:contextMenuItemsForElement:defaultMenuItems:);
    if (_private->UIDelegate && [_private->UIDelegate respondsToSelector:selector]) {
        menuItems = CallUIDelegate(self, selector, element, defaultMenuItems);
        if (!menuItems)
            return nil;
    }

    unsigned count = [menuItems count];
    if (!count)
        return nil;

    NSMenu *menu = [[NSMenu alloc] init];
    for (unsigned i = 0; i < count; i++)
        [menu addItem:[menuItems objectAtIndex:i]];

    return [menu autorelease];
}

- (void)_mouseDidMoveOverElement:(NSDictionary *)dictionary modifierFlags:(NSUInteger)modifierFlags
{
    // We originally intended to call this delegate method sometimes with a nil dictionary, but due to
    // a bug dating back to WebKit 1.0 this delegate was never called with nil! Unfortunately we can't
    // start calling this with nil since it will break Adobe Help Viewer, and possibly other clients.
    if (!dictionary)
        return;
    CallUIDelegate(self, @selector(webView:mouseDidMoveOverElement:modifierFlags:), dictionary, modifierFlags);
}

- (void)_loadBackForwardListFromOtherView:(WebView *)otherView
{
    if (!_private->page)
        return;
    
    if (!otherView->_private->page)
        return;
    
    // It turns out the right combination of behavior is done with the back/forward load
    // type.  (See behavior matrix at the top of WebFramePrivate.)  So we copy all the items
    // in the back forward list, and go to the current one.

    BackForwardList* backForwardList = _private->page->backForwardList();
    ASSERT(!backForwardList->currentItem()); // destination list should be empty

    BackForwardList* otherBackForwardList = otherView->_private->page->backForwardList();
    if (!otherBackForwardList->currentItem())
        return; // empty back forward list, bail
    
    HistoryItem* newItemToGoTo = 0;

    int lastItemIndex = otherBackForwardList->forwardListCount();
    for (int i = -otherBackForwardList->backListCount(); i <= lastItemIndex; ++i) {
        if (i == 0) {
            // If this item is showing , save away its current scroll and form state,
            // since that might have changed since loading and it is normally not saved
            // until we leave that page.
            otherView->_private->page->mainFrame()->loader()->history()->saveDocumentAndScrollState();
        }
        RefPtr<HistoryItem> newItem = otherBackForwardList->itemAtIndex(i)->copy();
        if (i == 0) 
            newItemToGoTo = newItem.get();
        backForwardList->addItem(newItem.release());
    }
    
    ASSERT(newItemToGoTo);
    _private->page->goToItem(newItemToGoTo, FrameLoadTypeIndexedBackForward);
}

- (void)_setFormDelegate: (id<WebFormDelegate>)delegate
{
    _private->formDelegate = delegate;
}

- (id<WebFormDelegate>)_formDelegate
{
    return _private->formDelegate;
}

- (BOOL)_needsAdobeFrameReloadingQuirk
{
    static BOOL needsQuirk = WKAppVersionCheckLessThan(@"com.adobe.Acrobat", -1, 9.0)
        || WKAppVersionCheckLessThan(@"com.adobe.Acrobat.Pro", -1, 9.0)
        || WKAppVersionCheckLessThan(@"com.adobe.Reader", -1, 9.0)
        || WKAppVersionCheckLessThan(@"com.adobe.distiller", -1, 9.0)
        || WKAppVersionCheckLessThan(@"com.adobe.Contribute", -1, 4.2)
        || WKAppVersionCheckLessThan(@"com.adobe.dreamweaver-9.0", -1, 9.1)
        || WKAppVersionCheckLessThan(@"com.macromedia.fireworks", -1, 9.1)
        || WKAppVersionCheckLessThan(@"com.adobe.InCopy", -1, 5.1)
        || WKAppVersionCheckLessThan(@"com.adobe.InDesign", -1, 5.1)
        || WKAppVersionCheckLessThan(@"com.adobe.Soundbooth", -1, 2);

    return needsQuirk;
}

- (BOOL)_needsLinkElementTextCSSQuirk
{
    static BOOL needsQuirk = !WebKitLinkedOnOrAfter(WEBKIT_FIRST_VERSION_WITHOUT_LINK_ELEMENT_TEXT_CSS_QUIRK)
        && WKAppVersionCheckLessThan(@"com.e-frontier.shade10", -1, 10.6);
    return needsQuirk;
}

- (BOOL)_needsIsLoadingInAPISenseQuirk
{
    static BOOL needsQuirk = WKAppVersionCheckLessThan(@"com.apple.iAdProducer", -1, 2.1);
    
    return needsQuirk;
}

- (BOOL)_needsKeyboardEventDisambiguationQuirks
{
    static BOOL needsQuirks = !WebKitLinkedOnOrAfter(WEBKIT_FIRST_VERSION_WITH_IE_COMPATIBLE_KEYBOARD_EVENT_DISPATCH) && !applicationIsSafari();
    return needsQuirks;
}

- (BOOL)_needsFrameLoadDelegateRetainQuirk
{
    static BOOL needsQuirk = WKAppVersionCheckLessThan(@"com.equinux.iSale5", -1, 5.6);    
    return needsQuirk;
}

static bool needsDidFinishLoadOrderQuirk()
{
    static bool needsQuirk = !WebKitLinkedOnOrAfter(WEBKIT_FIRST_VERSION_WITH_CORRECT_DID_FINISH_LOAD_ORDER) && applicationIsAppleMail();
    return needsQuirk;
}

static bool needsSelfRetainWhileLoadingQuirk()
{
    static bool needsQuirk = applicationIsAperture();
    return needsQuirk;
}

- (BOOL)_needsPreHTML5ParserQuirks
{    
    // AOL Instant Messenger and Microsoft My Day contain markup incompatible
    // with the new HTML5 parser. If these applications were linked against a
    // version of WebKit prior to the introduction of the HTML5 parser, enable
    // parser quirks to maintain compatibility. For details, see 
    // <https://bugs.webkit.org/show_bug.cgi?id=46134> and
    // <https://bugs.webkit.org/show_bug.cgi?id=46334>.
    static bool isApplicationNeedingParserQuirks = !WebKitLinkedOnOrAfter(WEBKIT_FIRST_VERSION_WITH_HTML5_PARSER)
        && (applicationIsAOLInstantMessenger() || applicationIsMicrosoftMyDay());
    
    // Mail.app must continue to display HTML email that contains quirky markup.
    static bool isAppleMail = applicationIsAppleMail();

    return isApplicationNeedingParserQuirks
        || isAppleMail
#if ENABLE(DASHBOARD_SUPPORT)
        // Pre-HTML5 parser quirks are required to remain compatible with many
        // Dashboard widgets. See <rdar://problem/8175982>.
        || (_private->page && _private->page->settings()->usesDashboardBackwardCompatibilityMode())
#endif
        || [[self preferences] usePreHTML5ParserQuirks];
}

- (BOOL)_needsUnrestrictedGetMatchedCSSRules
{
    static bool needsUnrestrictedGetMatchedCSSRules = !WebKitLinkedOnOrAfter(WEBKIT_FIRST_VERSION_WITH_GET_MATCHED_CSS_RULES_RESTRICTIONS) && applicationIsSafari();
    return needsUnrestrictedGetMatchedCSSRules;
}

- (void)_preferencesChangedNotification:(NSNotification *)notification
{
    WebPreferences *preferences = (WebPreferences *)[notification object];
    [self _preferencesChanged:preferences];
}

- (void)_preferencesChanged:(WebPreferences *)preferences
{    
    ASSERT(preferences == [self preferences]);
    if (!_private->userAgentOverridden)
        _private->userAgent = String();

    // Cache this value so we don't have to read NSUserDefaults on each page load
    _private->useSiteSpecificSpoofing = [preferences _useSiteSpecificSpoofing];

    // Update corresponding WebCore Settings object.
    if (!_private->page)
        return;
    
    Settings* settings = _private->page->settings();
    
    settings->setCursiveFontFamily([preferences cursiveFontFamily]);
    settings->setDefaultFixedFontSize([preferences defaultFixedFontSize]);
    settings->setDefaultFontSize([preferences defaultFontSize]);
    settings->setDefaultTextEncodingName([preferences defaultTextEncodingName]);
    settings->setUsesEncodingDetector([preferences usesEncodingDetector]);
    settings->setFantasyFontFamily([preferences fantasyFontFamily]);
    settings->setFixedFontFamily([preferences fixedFontFamily]);
    settings->setScreenFontSubstitutionEnabled(
#if __MAC_OS_X_VERSION_MIN_REQUIRED >= 1090
        [[NSUserDefaults standardUserDefaults] boolForKey:@"NSFontDefaultScreenFontSubstitutionEnabled"] ||
#endif
        [preferences screenFontSubstitutionEnabled]
    );
    settings->setForceFTPDirectoryListings([preferences _forceFTPDirectoryListings]);
    settings->setFTPDirectoryTemplatePath([preferences _ftpDirectoryTemplatePath]);
    settings->setLocalStorageDatabasePath([preferences _localStorageDatabasePath]);
    settings->setJavaEnabled([preferences isJavaEnabled]);
    settings->setScriptEnabled([preferences isJavaScriptEnabled]);
    settings->setWebSecurityEnabled([preferences isWebSecurityEnabled]);
    settings->setAllowUniversalAccessFromFileURLs([preferences allowUniversalAccessFromFileURLs]);
    settings->setAllowFileAccessFromFileURLs([preferences allowFileAccessFromFileURLs]);
    settings->setJavaScriptCanOpenWindowsAutomatically([preferences javaScriptCanOpenWindowsAutomatically]);
    settings->setMinimumFontSize([preferences minimumFontSize]);
    settings->setMinimumLogicalFontSize([preferences minimumLogicalFontSize]);
    settings->setPictographFontFamily([preferences pictographFontFamily]);
    settings->setPluginsEnabled([preferences arePlugInsEnabled]);
#if ENABLE(SQL_DATABASE)
    DatabaseManager::manager().setIsAvailable([preferences databasesEnabled]);
#endif
    settings->setLocalStorageEnabled([preferences localStorageEnabled]);
    settings->setExperimentalNotificationsEnabled([preferences experimentalNotificationsEnabled]);

    bool privateBrowsingEnabled = [preferences privateBrowsingEnabled];
#if PLATFORM(MAC) || USE(CFNETWORK)
    if (privateBrowsingEnabled)
        WebFrameNetworkingContext::ensurePrivateBrowsingSession();
    else
        WebFrameNetworkingContext::destroyPrivateBrowsingSession();
#endif
    settings->setPrivateBrowsingEnabled(privateBrowsingEnabled);

    settings->setSansSerifFontFamily([preferences sansSerifFontFamily]);
    settings->setSerifFontFamily([preferences serifFontFamily]);
    settings->setStandardFontFamily([preferences standardFontFamily]);
    settings->setLoadsImagesAutomatically([preferences loadsImagesAutomatically]);
    settings->setLoadsSiteIconsIgnoringImageLoadingSetting([preferences loadsSiteIconsIgnoringImageLoadingPreference]);
    settings->setShouldPrintBackgrounds([preferences shouldPrintBackgrounds]);
    settings->setTextAreasAreResizable([preferences textAreasAreResizable]);
    settings->setShrinksStandaloneImagesToFit([preferences shrinksStandaloneImagesToFit]);
    settings->setEditableLinkBehavior(core([preferences editableLinkBehavior]));
    settings->setTextDirectionSubmenuInclusionBehavior(core([preferences textDirectionSubmenuInclusionBehavior]));
    settings->setDOMPasteAllowed([preferences isDOMPasteAllowed]);
    settings->setUsesPageCache([self usesPageCache]);
    settings->setPageCacheSupportsPlugins([preferences pageCacheSupportsPlugins]);
    settings->setBackForwardCacheExpirationInterval([preferences _backForwardCacheExpirationInterval]);
    settings->setShowsURLsInToolTips([preferences showsURLsInToolTips]);
    settings->setShowsToolTipOverTruncatedText([preferences showsToolTipOverTruncatedText]);
    settings->setDeveloperExtrasEnabled([preferences developerExtrasEnabled]);
    settings->setJavaScriptExperimentsEnabled([preferences javaScriptExperimentsEnabled]);
    settings->setAuthorAndUserStylesEnabled([preferences authorAndUserStylesEnabled]);
    settings->setApplicationChromeMode([preferences applicationChromeModeEnabled]);
    if ([preferences userStyleSheetEnabled]) {
        NSString* location = [[preferences userStyleSheetLocation] _web_originalDataAsString];
        if ([location isEqualToString:@"apple-dashboard://stylesheet"])
            location = @"file:///System/Library/PrivateFrameworks/DashboardClient.framework/Resources/widget.css";
        settings->setUserStyleSheetLocation([NSURL URLWithString:(location ? location : @"")]);
    } else
        settings->setUserStyleSheetLocation([NSURL URLWithString:@""]);
    settings->setNeedsAdobeFrameReloadingQuirk([self _needsAdobeFrameReloadingQuirk]);
    settings->setTreatsAnyTextCSSLinkAsStylesheet([self _needsLinkElementTextCSSQuirk]);
    settings->setNeedsKeyboardEventDisambiguationQuirks([self _needsKeyboardEventDisambiguationQuirks]);
    settings->setNeedsSiteSpecificQuirks(_private->useSiteSpecificSpoofing);
    settings->setWebArchiveDebugModeEnabled([preferences webArchiveDebugModeEnabled]);
    settings->setLocalFileContentSniffingEnabled([preferences localFileContentSniffingEnabled]);
    settings->setOfflineWebApplicationCacheEnabled([preferences offlineWebApplicationCacheEnabled]);
    settings->setJavaScriptCanAccessClipboard([preferences javaScriptCanAccessClipboard]);
    settings->setXSSAuditorEnabled([preferences isXSSAuditorEnabled]);
    settings->setEnforceCSSMIMETypeInNoQuirksMode(!WKAppVersionCheckLessThan(@"com.apple.iWeb", -1, 2.1));
    settings->setDNSPrefetchingEnabled([preferences isDNSPrefetchingEnabled]);
    
    // FIXME: Enabling accelerated compositing when WebGL is enabled causes tests to fail on Leopard which expect HW compositing to be disabled.
    // Until we fix that, I will comment out the test (CFM)
    settings->setAcceleratedCompositingEnabled([preferences acceleratedCompositingEnabled]);
    settings->setAcceleratedDrawingEnabled([preferences acceleratedDrawingEnabled]);
    settings->setCanvasUsesAcceleratedDrawing([preferences canvasUsesAcceleratedDrawing]);    
    settings->setShowDebugBorders([preferences showDebugBorders]);
    settings->setShowRepaintCounter([preferences showRepaintCounter]);
    settings->setWebGLEnabled([preferences webGLEnabled]);
    settings->setAccelerated2dCanvasEnabled([preferences accelerated2dCanvasEnabled]);
    settings->setLoadDeferringEnabled(shouldEnableLoadDeferring());
    settings->setWindowFocusRestricted(shouldRestrictWindowFocus());
    settings->setFrameFlatteningEnabled([preferences isFrameFlatteningEnabled]);
    settings->setSpatialNavigationEnabled([preferences isSpatialNavigationEnabled]);
    settings->setPaginateDuringLayoutEnabled([preferences paginateDuringLayoutEnabled]);
#if ENABLE(CSS_SHADERS)
    settings->setCSSCustomFilterEnabled([preferences cssCustomFilterEnabled]);
#endif
    RuntimeEnabledFeatures::setCSSRegionsEnabled([preferences cssRegionsEnabled]);
    RuntimeEnabledFeatures::setCSSCompositingEnabled([preferences cssCompositingEnabled]);
#if ENABLE(WEB_AUDIO)
    settings->setWebAudioEnabled([preferences webAudioEnabled]);
#endif
#if ENABLE(IFRAME_SEAMLESS)
    RuntimeEnabledFeatures::setSeamlessIFramesEnabled([preferences seamlessIFramesEnabled]);
#endif
    settings->setCSSGridLayoutEnabled([preferences cssGridLayoutEnabled]);
#if ENABLE(FULLSCREEN_API)
    settings->setFullScreenEnabled([preferences fullScreenEnabled]);
#endif
    settings->setAsynchronousSpellCheckingEnabled([preferences asynchronousSpellCheckingEnabled]);
    settings->setHyperlinkAuditingEnabled([preferences hyperlinkAuditingEnabled]);
    settings->setUsePreHTML5ParserQuirks([self _needsPreHTML5ParserQuirks]);
    settings->setCrossOriginCheckInGetMatchedCSSRulesDisabled([self _needsUnrestrictedGetMatchedCSSRules]);
    settings->setInteractiveFormValidationEnabled([self interactiveFormValidationEnabled]);
    settings->setValidationMessageTimerMagnification([self validationMessageTimerMagnification]);
#if USE(AVFOUNDATION)
    settings->setAVFoundationEnabled([preferences isAVFoundationEnabled]);
#endif
#if PLATFORM(MAC)
    settings->setQTKitEnabled([preferences isQTKitEnabled]);
#endif
    settings->setMediaPlaybackRequiresUserGesture([preferences mediaPlaybackRequiresUserGesture]);
    settings->setMediaPlaybackAllowsInline([preferences mediaPlaybackAllowsInline]);
    settings->setSuppressesIncrementalRendering([preferences suppressesIncrementalRendering]);
    settings->setRegionBasedColumnsEnabled([preferences regionBasedColumnsEnabled]);
    settings->setBackspaceKeyNavigationEnabled([preferences backspaceKeyNavigationEnabled]);
    settings->setWantsBalancedSetDefersLoadingBehavior([preferences wantsBalancedSetDefersLoadingBehavior]);
    settings->setMockScrollbarsEnabled([preferences mockScrollbarsEnabled]);

#if ENABLE(VIDEO_TRACK)
    settings->setShouldDisplaySubtitles([preferences shouldDisplaySubtitles]);
    settings->setShouldDisplayCaptions([preferences shouldDisplayCaptions]);
    settings->setShouldDisplayTextDescriptions([preferences shouldDisplayTextDescriptions]);
#endif

    settings->setShouldRespectImageOrientation([preferences shouldRespectImageOrientation]);
    settings->setNeedsIsLoadingInAPISenseQuirk([self _needsIsLoadingInAPISenseQuirk]);
    settings->setRequestAnimationFrameEnabled([preferences requestAnimationFrameEnabled]);
    settings->setNeedsDidFinishLoadOrderQuirk(needsDidFinishLoadOrderQuirk());
    settings->setDiagnosticLoggingEnabled([preferences diagnosticLoggingEnabled]);
    settings->setLowPowerVideoAudioBufferSizeEnabled([preferences lowPowerVideoAudioBufferSizeEnabled]);

    switch ([preferences storageBlockingPolicy]) {
    case WebAllowAllStorage:
        settings->setStorageBlockingPolicy(SecurityOrigin::AllowAllStorage);
        break;
    case WebBlockThirdPartyStorage:
        settings->setStorageBlockingPolicy(SecurityOrigin::BlockThirdPartyStorage);
        break;
    case WebBlockAllStorage:
        settings->setStorageBlockingPolicy(SecurityOrigin::BlockAllStorage);
        break;
    }

    settings->setPlugInSnapshottingEnabled([preferences plugInSnapshottingEnabled]);

#if ENABLE(HIDDEN_PAGE_DOM_TIMER_THROTTLING)
    settings->setHiddenPageDOMTimerThrottlingEnabled([preferences hiddenPageDOMTimerThrottlingEnabled]);
#endif
#if ENABLE(PAGE_VISIBILITY_API)
    settings->setHiddenPageCSSAnimationSuspensionEnabled([preferences hiddenPageCSSAnimationSuspensionEnabled]);
#endif

    // We have enabled this setting in WebKit2 for the sake of some ScrollingCoordinator work.
    // To avoid possible rendering differences, we should enable it for WebKit1 too.
    settings->setFixedPositionCreatesStackingContext(true);
    
    NSTimeInterval timeout = [preferences incrementalRenderingSuppressionTimeoutInSeconds];
    if (timeout > 0)
        settings->setIncrementalRenderingSuppressionTimeoutInSeconds(timeout);

    // Application Cache Preferences are stored on the global cache storage manager, not in Settings.
    [WebApplicationCache setDefaultOriginQuota:[preferences applicationCacheDefaultOriginQuota]];

    BOOL zoomsTextOnly = [preferences zoomsTextOnly];
    if (_private->zoomsTextOnly != zoomsTextOnly)
        [self _setZoomMultiplier:_private->zoomMultiplier isTextOnly:zoomsTextOnly];
}

static inline IMP getMethod(id o, SEL s)
{
    return [o respondsToSelector:s] ? [o methodForSelector:s] : 0;
}

- (void)_cacheResourceLoadDelegateImplementations
{
    WebResourceDelegateImplementationCache *cache = &_private->resourceLoadDelegateImplementations;
    id delegate = _private->resourceProgressDelegate;

    if (!delegate) {
        bzero(cache, sizeof(WebResourceDelegateImplementationCache));
        return;
    }

    cache->didCancelAuthenticationChallengeFunc = getMethod(delegate, @selector(webView:resource:didCancelAuthenticationChallenge:fromDataSource:));
    cache->didFailLoadingWithErrorFromDataSourceFunc = getMethod(delegate, @selector(webView:resource:didFailLoadingWithError:fromDataSource:));
    cache->didFinishLoadingFromDataSourceFunc = getMethod(delegate, @selector(webView:resource:didFinishLoadingFromDataSource:));
    cache->didLoadResourceFromMemoryCacheFunc = getMethod(delegate, @selector(webView:didLoadResourceFromMemoryCache:response:length:fromDataSource:));
    cache->didReceiveAuthenticationChallengeFunc = getMethod(delegate, @selector(webView:resource:didReceiveAuthenticationChallenge:fromDataSource:));
#if USE(PROTECTION_SPACE_AUTH_CALLBACK)
    cache->canAuthenticateAgainstProtectionSpaceFunc = getMethod(delegate, @selector(webView:resource:canAuthenticateAgainstProtectionSpace:forDataSource:));
#endif
    cache->didReceiveContentLengthFunc = getMethod(delegate, @selector(webView:resource:didReceiveContentLength:fromDataSource:));
    cache->didReceiveResponseFunc = getMethod(delegate, @selector(webView:resource:didReceiveResponse:fromDataSource:));
    cache->identifierForRequestFunc = getMethod(delegate, @selector(webView:identifierForInitialRequest:fromDataSource:));
    cache->plugInFailedWithErrorFunc = getMethod(delegate, @selector(webView:plugInFailedWithError:dataSource:));
    cache->willCacheResponseFunc = getMethod(delegate, @selector(webView:resource:willCacheResponse:fromDataSource:));
    cache->willSendRequestFunc = getMethod(delegate, @selector(webView:resource:willSendRequest:redirectResponse:fromDataSource:));
    cache->shouldUseCredentialStorageFunc = getMethod(delegate, @selector(webView:resource:shouldUseCredentialStorageForDataSource:));
    cache->shouldPaintBrokenImageForURLFunc = getMethod(delegate, @selector(webView:shouldPaintBrokenImageForURL:));
}

- (void)_cacheFrameLoadDelegateImplementations
{
    WebFrameLoadDelegateImplementationCache *cache = &_private->frameLoadDelegateImplementations;
    id delegate = _private->frameLoadDelegate;

    if (!delegate) {
        bzero(cache, sizeof(WebFrameLoadDelegateImplementationCache));
        return;
    }

    cache->didCancelClientRedirectForFrameFunc = getMethod(delegate, @selector(webView:didCancelClientRedirectForFrame:));
    cache->didChangeLocationWithinPageForFrameFunc = getMethod(delegate, @selector(webView:didChangeLocationWithinPageForFrame:));
    cache->didPushStateWithinPageForFrameFunc = getMethod(delegate, @selector(webView:didPushStateWithinPageForFrame:));
    cache->didReplaceStateWithinPageForFrameFunc = getMethod(delegate, @selector(webView:didReplaceStateWithinPageForFrame:));
    cache->didPopStateWithinPageForFrameFunc = getMethod(delegate, @selector(webView:didPopStateWithinPageForFrame:));
#if JSC_OBJC_API_ENABLED
    cache->didCreateJavaScriptContextForFrameFunc = getMethod(delegate, @selector(webView:didCreateJavaScriptContext:forFrame:));
#endif
    cache->didClearWindowObjectForFrameFunc = getMethod(delegate, @selector(webView:didClearWindowObject:forFrame:));
    cache->didClearWindowObjectForFrameInScriptWorldFunc = getMethod(delegate, @selector(webView:didClearWindowObjectForFrame:inScriptWorld:));
    cache->didClearInspectorWindowObjectForFrameFunc = getMethod(delegate, @selector(webView:didClearInspectorWindowObject:forFrame:));
    cache->didCommitLoadForFrameFunc = getMethod(delegate, @selector(webView:didCommitLoadForFrame:));
    cache->didFailLoadWithErrorForFrameFunc = getMethod(delegate, @selector(webView:didFailLoadWithError:forFrame:));
    cache->didFailProvisionalLoadWithErrorForFrameFunc = getMethod(delegate, @selector(webView:didFailProvisionalLoadWithError:forFrame:));
    cache->didFinishDocumentLoadForFrameFunc = getMethod(delegate, @selector(webView:didFinishDocumentLoadForFrame:));
    cache->didFinishLoadForFrameFunc = getMethod(delegate, @selector(webView:didFinishLoadForFrame:));
    cache->didFirstLayoutInFrameFunc = getMethod(delegate, @selector(webView:didFirstLayoutInFrame:));
    cache->didFirstVisuallyNonEmptyLayoutInFrameFunc = getMethod(delegate, @selector(webView:didFirstVisuallyNonEmptyLayoutInFrame:));
    cache->didLayoutFunc = getMethod(delegate, @selector(webView:didLayout:));
    cache->didHandleOnloadEventsForFrameFunc = getMethod(delegate, @selector(webView:didHandleOnloadEventsForFrame:));
    cache->didReceiveIconForFrameFunc = getMethod(delegate, @selector(webView:didReceiveIcon:forFrame:));
    cache->didReceiveServerRedirectForProvisionalLoadForFrameFunc = getMethod(delegate, @selector(webView:didReceiveServerRedirectForProvisionalLoadForFrame:));
    cache->didReceiveTitleForFrameFunc = getMethod(delegate, @selector(webView:didReceiveTitle:forFrame:));
    cache->didStartProvisionalLoadForFrameFunc = getMethod(delegate, @selector(webView:didStartProvisionalLoadForFrame:));
    cache->willCloseFrameFunc = getMethod(delegate, @selector(webView:willCloseFrame:));
    cache->willPerformClientRedirectToURLDelayFireDateForFrameFunc = getMethod(delegate, @selector(webView:willPerformClientRedirectToURL:delay:fireDate:forFrame:));
    cache->windowScriptObjectAvailableFunc = getMethod(delegate, @selector(webView:windowScriptObjectAvailable:));
    cache->didDisplayInsecureContentFunc = getMethod(delegate, @selector(webViewDidDisplayInsecureContent:));
    cache->didRunInsecureContentFunc = getMethod(delegate, @selector(webView:didRunInsecureContent:));
    cache->didDetectXSSFunc = getMethod(delegate, @selector(webView:didDetectXSS:));
    cache->didRemoveFrameFromHierarchyFunc = getMethod(delegate, @selector(webView:didRemoveFrameFromHierarchy:));

    // It would be nice to get rid of this code and transition all clients to using didLayout instead of
    // didFirstLayoutInFrame and didFirstVisuallyNonEmptyLayoutInFrame. In the meantime, this is required
    // for backwards compatibility.
    Page* page = core(self);
    if (page) {
        unsigned milestones = DidFirstLayout;
        if (cache->didFirstVisuallyNonEmptyLayoutInFrameFunc)
            milestones |= DidFirstVisuallyNonEmptyLayout;
        page->addLayoutMilestones(static_cast<LayoutMilestones>(milestones));
    }
}

- (void)_cacheScriptDebugDelegateImplementations
{
    WebScriptDebugDelegateImplementationCache *cache = &_private->scriptDebugDelegateImplementations;
    id delegate = _private->scriptDebugDelegate;

    if (!delegate) {
        bzero(cache, sizeof(WebScriptDebugDelegateImplementationCache));
        return;
    }

    cache->didParseSourceFunc = getMethod(delegate, @selector(webView:didParseSource:baseLineNumber:fromURL:sourceId:forWebFrame:));
    if (cache->didParseSourceFunc)
        cache->didParseSourceExpectsBaseLineNumber = YES;
    else {
        cache->didParseSourceExpectsBaseLineNumber = NO;
        cache->didParseSourceFunc = getMethod(delegate, @selector(webView:didParseSource:fromURL:sourceId:forWebFrame:));
    }

    cache->failedToParseSourceFunc = getMethod(delegate, @selector(webView:failedToParseSource:baseLineNumber:fromURL:withError:forWebFrame:));
    cache->didEnterCallFrameFunc = getMethod(delegate, @selector(webView:didEnterCallFrame:sourceId:line:forWebFrame:));
    cache->willExecuteStatementFunc = getMethod(delegate, @selector(webView:willExecuteStatement:sourceId:line:forWebFrame:));
    cache->willLeaveCallFrameFunc = getMethod(delegate, @selector(webView:willLeaveCallFrame:sourceId:line:forWebFrame:));

    cache->exceptionWasRaisedFunc = getMethod(delegate, @selector(webView:exceptionWasRaised:hasHandler:sourceId:line:forWebFrame:));
    if (cache->exceptionWasRaisedFunc)
        cache->exceptionWasRaisedExpectsHasHandlerFlag = YES;
    else {
        cache->exceptionWasRaisedExpectsHasHandlerFlag = NO;
        cache->exceptionWasRaisedFunc = getMethod(delegate, @selector(webView:exceptionWasRaised:sourceId:line:forWebFrame:));
    }
}

- (void)_cacheHistoryDelegateImplementations
{
    WebHistoryDelegateImplementationCache *cache = &_private->historyDelegateImplementations;
    id delegate = _private->historyDelegate;

    if (!delegate) {
        bzero(cache, sizeof(WebHistoryDelegateImplementationCache));
        return;
    }

    cache->navigatedFunc = getMethod(delegate, @selector(webView:didNavigateWithNavigationData:inFrame:));
    cache->clientRedirectFunc = getMethod(delegate, @selector(webView:didPerformClientRedirectFromURL:toURL:inFrame:));
    cache->serverRedirectFunc = getMethod(delegate, @selector(webView:didPerformServerRedirectFromURL:toURL:inFrame:));
    cache->deprecatedSetTitleFunc = getMethod(delegate, @selector(webView:updateHistoryTitle:forURL:));
    cache->setTitleFunc = getMethod(delegate, @selector(webView:updateHistoryTitle:forURL:inFrame:));
    cache->populateVisitedLinksFunc = getMethod(delegate, @selector(populateVisitedLinksForWebView:));
}

- (id)_policyDelegateForwarder
{
    if (!_private->policyDelegateForwarder)
        _private->policyDelegateForwarder = [[_WebSafeForwarder alloc] initWithTarget:_private->policyDelegate defaultTarget:[WebDefaultPolicyDelegate sharedPolicyDelegate]];
    return _private->policyDelegateForwarder;
}

- (id)_UIDelegateForwarder
{
    if (!_private->UIDelegateForwarder)
        _private->UIDelegateForwarder = [[_WebSafeForwarder alloc] initWithTarget:_private->UIDelegate defaultTarget:[WebDefaultUIDelegate sharedUIDelegate]];
    return _private->UIDelegateForwarder;
}

- (id)_editingDelegateForwarder
{
    // This can be called during window deallocation by QTMovieView in the QuickTime Cocoa Plug-in.
    // Not sure if that is a bug or not.
    if (!_private)
        return nil;

    if (!_private->editingDelegateForwarder)
        _private->editingDelegateForwarder = [[_WebSafeForwarder alloc] initWithTarget:_private->editingDelegate defaultTarget:[WebDefaultEditingDelegate sharedEditingDelegate]];
    return _private->editingDelegateForwarder;
}

- (void)_closeWindow
{
    [[self _UIDelegateForwarder] webViewClose:self];
}

+ (void)_unregisterViewClassAndRepresentationClassForMIMEType:(NSString *)MIMEType
{
    [[WebFrameView _viewTypesAllowImageTypeOmission:NO] removeObjectForKey:MIMEType];
    [[WebDataSource _repTypesAllowImageTypeOmission:NO] removeObjectForKey:MIMEType];
    
    // FIXME: We also need to maintain MIMEType registrations (which can be dynamically changed)
    // in the WebCore MIMEType registry.  For now we're doing this in a safe, limited manner
    // to fix <rdar://problem/5372989> - a future revamping of the entire system is neccesary for future robustness
    MIMETypeRegistry::getSupportedNonImageMIMETypes().remove(MIMEType);
}

+ (void)_registerViewClass:(Class)viewClass representationClass:(Class)representationClass forURLScheme:(NSString *)URLScheme
{
    NSString *MIMEType = [self _generatedMIMETypeForURLScheme:URLScheme];
    [self registerViewClass:viewClass representationClass:representationClass forMIMEType:MIMEType];

    // FIXME: We also need to maintain MIMEType registrations (which can be dynamically changed)
    // in the WebCore MIMEType registry.  For now we're doing this in a safe, limited manner
    // to fix <rdar://problem/5372989> - a future revamping of the entire system is neccesary for future robustness
    if ([viewClass class] == [WebHTMLView class])
        MIMETypeRegistry::getSupportedNonImageMIMETypes().add(MIMEType);
    
    // This is used to make _representationExistsForURLScheme faster.
    // Without this set, we'd have to create the MIME type each time.
    if (schemesWithRepresentationsSet == nil) {
        schemesWithRepresentationsSet = [[NSMutableSet alloc] init];
    }
    [schemesWithRepresentationsSet addObject:[[[URLScheme lowercaseString] copy] autorelease]];
}

+ (NSString *)_generatedMIMETypeForURLScheme:(NSString *)URLScheme
{
    return [@"x-apple-web-kit/" stringByAppendingString:[URLScheme lowercaseString]];
}

+ (BOOL)_representationExistsForURLScheme:(NSString *)URLScheme
{
    return [schemesWithRepresentationsSet containsObject:[URLScheme lowercaseString]];
}

+ (BOOL)_canHandleRequest:(NSURLRequest *)request forMainFrame:(BOOL)forMainFrame
{
    // FIXME: If <rdar://problem/5217309> gets fixed, this check can be removed.
    if (!request)
        return NO;

    if ([NSURLConnection canHandleRequest:request])
        return YES;

    NSString *scheme = [[request URL] scheme];

    // Representations for URL schemes work at the top level.
    if (forMainFrame && [self _representationExistsForURLScheme:scheme])
        return YES;

    if ([scheme _webkit_isCaseInsensitiveEqualToString:@"applewebdata"])
        return YES;

#if ENABLE(BLOB)
    if ([scheme _webkit_isCaseInsensitiveEqualToString:@"blob"])
        return YES;
#endif

    return NO;
}

+ (BOOL)_canHandleRequest:(NSURLRequest *)request
{
    return [self _canHandleRequest:request forMainFrame:YES];
}

+ (NSString *)_decodeData:(NSData *)data
{
    HTMLNames::init(); // this method is used for importing bookmarks at startup, so HTMLNames are likely to be uninitialized yet
    RefPtr<TextResourceDecoder> decoder = TextResourceDecoder::create("text/html"); // bookmark files are HTML
    String result = decoder->decode(static_cast<const char*>([data bytes]), [data length]);
    result.append(decoder->flush());
    return result;
}

- (void)_pushPerformingProgrammaticFocus
{
    _private->programmaticFocusCount++;
}

- (void)_popPerformingProgrammaticFocus
{
    _private->programmaticFocusCount--;
}

- (BOOL)_isPerformingProgrammaticFocus
{
    return _private->programmaticFocusCount != 0;
}

- (void)_didChangeValueForKey: (NSString *)key
{
    LOG (Bindings, "calling didChangeValueForKey: %@", key);
    [self didChangeValueForKey: key];
}

- (void)_willChangeValueForKey: (NSString *)key
{
    LOG (Bindings, "calling willChangeValueForKey: %@", key);
    [self willChangeValueForKey: key];
}

+ (BOOL)automaticallyNotifiesObserversForKey:(NSString *)key {
    static NSSet *manualNotifyKeys = nil;
    if (!manualNotifyKeys)
        manualNotifyKeys = [[NSSet alloc] initWithObjects:_WebMainFrameURLKey, _WebIsLoadingKey, _WebEstimatedProgressKey,
            _WebCanGoBackKey, _WebCanGoForwardKey, _WebMainFrameTitleKey, _WebMainFrameIconKey, _WebMainFrameDocumentKey,
            nil];
    if ([manualNotifyKeys containsObject:key])
        return NO;
    return YES;
}

- (NSArray *)_declaredKeys {
    static NSArray *declaredKeys = nil;
    if (!declaredKeys)
        declaredKeys = [[NSArray alloc] initWithObjects:_WebMainFrameURLKey, _WebIsLoadingKey, _WebEstimatedProgressKey,
            _WebCanGoBackKey, _WebCanGoForwardKey, _WebMainFrameTitleKey, _WebMainFrameIconKey, _WebMainFrameDocumentKey, nil];
    return declaredKeys;
}

- (void)setObservationInfo:(void *)info
{
    _private->observationInfo = info;
}

- (void *)observationInfo
{
    return _private->observationInfo;
}

- (void)_willChangeBackForwardKeys
{
    [self _willChangeValueForKey: _WebCanGoBackKey];
    [self _willChangeValueForKey: _WebCanGoForwardKey];
}

- (void)_didChangeBackForwardKeys
{
    [self _didChangeValueForKey: _WebCanGoBackKey];
    [self _didChangeValueForKey: _WebCanGoForwardKey];
}

- (void)_didStartProvisionalLoadForFrame:(WebFrame *)frame
{
    if (needsSelfRetainWhileLoadingQuirk())
        [self retain];

    [self _willChangeBackForwardKeys];
    if (frame == [self mainFrame]){
        // Force an observer update by sending a will/did.
        [self _willChangeValueForKey: _WebIsLoadingKey];
        [self _didChangeValueForKey: _WebIsLoadingKey];

        [self _willChangeValueForKey: _WebMainFrameURLKey];
    }

    [NSApp setWindowsNeedUpdate:YES];

#if ENABLE(FULLSCREEN_API)
    Document* document = core([frame DOMDocument]);
    if (Element* element = document ? document->webkitCurrentFullScreenElement() : 0) {
        SEL selector = @selector(webView:closeFullScreenWithListener:);
        if (_private->UIDelegate && [_private->UIDelegate respondsToSelector:selector]) {
            WebKitFullScreenListener *listener = [[WebKitFullScreenListener alloc] initWithElement:element];
            CallUIDelegate(self, selector, listener);
            [listener release];
        } else if (_private->newFullscreenController && [_private->newFullscreenController isFullScreen]) {
            [_private->newFullscreenController close];
        }
    }
#endif
}

- (void)_didCommitLoadForFrame:(WebFrame *)frame
{
    if (frame == [self mainFrame])
        [self _didChangeValueForKey: _WebMainFrameURLKey];
    [NSApp setWindowsNeedUpdate:YES];
}

- (void)_didFinishLoadForFrame:(WebFrame *)frame
{
    if (needsSelfRetainWhileLoadingQuirk())
        [self performSelector:@selector(release) withObject:nil afterDelay:0];
        
    [self _didChangeBackForwardKeys];
    if (frame == [self mainFrame]){
        // Force an observer update by sending a will/did.
        [self _willChangeValueForKey: _WebIsLoadingKey];
        [self _didChangeValueForKey: _WebIsLoadingKey];
    }
    [NSApp setWindowsNeedUpdate:YES];
}

- (void)_didFailLoadWithError:(NSError *)error forFrame:(WebFrame *)frame
{
    if (needsSelfRetainWhileLoadingQuirk())
        [self performSelector:@selector(release) withObject:nil afterDelay:0];

    [self _didChangeBackForwardKeys];
    if (frame == [self mainFrame]){
        // Force an observer update by sending a will/did.
        [self _willChangeValueForKey: _WebIsLoadingKey];
        [self _didChangeValueForKey: _WebIsLoadingKey];
    }
    [NSApp setWindowsNeedUpdate:YES];
}

- (void)_didFailProvisionalLoadWithError:(NSError *)error forFrame:(WebFrame *)frame
{
    if (needsSelfRetainWhileLoadingQuirk())
        [self performSelector:@selector(release) withObject:nil afterDelay:0];

    [self _didChangeBackForwardKeys];
    if (frame == [self mainFrame]){
        // Force an observer update by sending a will/did.
        [self _willChangeValueForKey: _WebIsLoadingKey];
        [self _didChangeValueForKey: _WebIsLoadingKey];
        
        [self _didChangeValueForKey: _WebMainFrameURLKey];
    }
    [NSApp setWindowsNeedUpdate:YES];
}

- (NSCachedURLResponse *)_cachedResponseForURL:(NSURL *)URL
{
    RetainPtr<NSMutableURLRequest *> request = adoptNS([[NSMutableURLRequest alloc] initWithURL:URL]);
    [request _web_setHTTPUserAgent:[self userAgentForURL:URL]];
    NSCachedURLResponse *cachedResponse;

    if (!_private->page)
        return nil;

    if (CFURLStorageSessionRef storageSession = _private->page->mainFrame()->loader()->networkingContext()->storageSession().platformSession())
        cachedResponse = WKCachedResponseForRequest(storageSession, request.get());
    else
        cachedResponse = [[NSURLCache sharedURLCache] cachedResponseForRequest:request.get()];

    return cachedResponse;
}

- (void)_writeImageForElement:(NSDictionary *)element withPasteboardTypes:(NSArray *)types toPasteboard:(NSPasteboard *)pasteboard
{
    NSURL *linkURL = [element objectForKey:WebElementLinkURLKey];
    DOMElement *domElement = [element objectForKey:WebElementDOMNodeKey];
    [pasteboard _web_writeImage:(NSImage *)(domElement ? nil : [element objectForKey:WebElementImageKey])
                        element:domElement
                            URL:linkURL ? linkURL : (NSURL *)[element objectForKey:WebElementImageURLKey]
                          title:[element objectForKey:WebElementImageAltStringKey] 
                        archive:[[element objectForKey:WebElementDOMNodeKey] webArchive]
                          types:types
                         source:nil];
}

- (void)_writeLinkElement:(NSDictionary *)element withPasteboardTypes:(NSArray *)types toPasteboard:(NSPasteboard *)pasteboard
{
    [pasteboard _web_writeURL:[element objectForKey:WebElementLinkURLKey]
                     andTitle:[element objectForKey:WebElementLinkLabelKey]
                        types:types];
}

#if ENABLE(DRAG_SUPPORT)
- (void)_setInitiatedDrag:(BOOL)initiatedDrag
{
    if (!_private->page)
        return;
    _private->page->dragController()->setDidInitiateDrag(initiatedDrag);
}
#endif

#if ENABLE(DASHBOARD_SUPPORT)

#define DASHBOARD_CONTROL_LABEL @"control"

- (void)_addControlRect:(NSRect)bounds clip:(NSRect)clip fromView:(NSView *)view toDashboardRegions:(NSMutableDictionary *)regions
{
    NSRect adjustedBounds = bounds;
    adjustedBounds.origin = [self convertPoint:bounds.origin fromView:view];
    adjustedBounds.origin.y = [self bounds].size.height - adjustedBounds.origin.y;
    adjustedBounds.size = bounds.size;

    NSRect adjustedClip;
    adjustedClip.origin = [self convertPoint:clip.origin fromView:view];
    adjustedClip.origin.y = [self bounds].size.height - adjustedClip.origin.y;
    adjustedClip.size = clip.size;

    WebDashboardRegion *region = [[WebDashboardRegion alloc] initWithRect:adjustedBounds 
        clip:adjustedClip type:WebDashboardRegionTypeScrollerRectangle];
    NSMutableArray *scrollerRegions = [regions objectForKey:DASHBOARD_CONTROL_LABEL];
    if (!scrollerRegions) {
        scrollerRegions = [[NSMutableArray alloc] init];
        [regions setObject:scrollerRegions forKey:DASHBOARD_CONTROL_LABEL];
        [scrollerRegions release];
    }
    [scrollerRegions addObject:region];
    [region release];
}

- (void)_addScrollerDashboardRegionsForFrameView:(FrameView*)frameView dashboardRegions:(NSMutableDictionary *)regions
{    
    NSView *documentView = [[kit(frameView->frame()) frameView] documentView];

    const HashSet<RefPtr<Widget> >* children = frameView->children();
    HashSet<RefPtr<Widget> >::const_iterator end = children->end();
    for (HashSet<RefPtr<Widget> >::const_iterator it = children->begin(); it != end; ++it) {
        Widget* widget = (*it).get();
        if (widget->isFrameView()) {
            [self _addScrollerDashboardRegionsForFrameView:toFrameView(widget) dashboardRegions:regions];
            continue;
        }

        if (!widget->isScrollbar())
            continue;

        // FIXME: This should really pass an appropriate clip, but our first try got it wrong, and
        // it's not common to need this to be correct in Dashboard widgets.
        NSRect bounds = widget->frameRect();
        [self _addControlRect:bounds clip:bounds fromView:documentView toDashboardRegions:regions];
    }
}

- (void)_addScrollerDashboardRegions:(NSMutableDictionary *)regions from:(NSArray *)views
{
    // Add scroller regions for NSScroller and WebCore scrollbars
    NSUInteger count = [views count];
    for (NSUInteger i = 0; i < count; i++) {
        NSView *view = [views objectAtIndex:i];
        
        if ([view isKindOfClass:[WebHTMLView class]]) {
            if (Frame* coreFrame = core([(WebHTMLView*)view _frame])) {
                if (FrameView* coreView = coreFrame->view())
                    [self _addScrollerDashboardRegionsForFrameView:coreView dashboardRegions:regions];
            }
        } else if ([view isKindOfClass:[NSScroller class]]) {
            // AppKit places absent scrollers at -100,-100
            if ([view frame].origin.y < 0)
                continue;
            [self _addControlRect:[view bounds] clip:[view visibleRect] fromView:view toDashboardRegions:regions];
        }
        [self _addScrollerDashboardRegions:regions from:[view subviews]];
    }
}

- (void)_addScrollerDashboardRegions:(NSMutableDictionary *)regions
{
    [self _addScrollerDashboardRegions:regions from:[self subviews]];
}

- (NSDictionary *)_dashboardRegions
{
    // Only return regions from main frame.
    Frame* mainFrame = [self _mainCoreFrame];
    if (!mainFrame)
        return nil;

    const Vector<AnnotatedRegionValue>& regions = mainFrame->document()->annotatedRegions();
    size_t size = regions.size();

    NSMutableDictionary *webRegions = [NSMutableDictionary dictionaryWithCapacity:size];
    for (size_t i = 0; i < size; i++) {
        const AnnotatedRegionValue& region = regions[i];

        if (region.type == StyleDashboardRegion::None)
            continue;

        NSString *label = region.label;
        WebDashboardRegionType type = WebDashboardRegionTypeNone;
        if (region.type == StyleDashboardRegion::Circle)
            type = WebDashboardRegionTypeCircle;
        else if (region.type == StyleDashboardRegion::Rectangle)
            type = WebDashboardRegionTypeRectangle;
        NSMutableArray *regionValues = [webRegions objectForKey:label];
        if (!regionValues) {
            regionValues = [[NSMutableArray alloc] initWithCapacity:1];
            [webRegions setObject:regionValues forKey:label];
            [regionValues release];
        }

        WebDashboardRegion *webRegion = [[WebDashboardRegion alloc] initWithRect:pixelSnappedIntRect(region.bounds) clip:pixelSnappedIntRect(region.clip) type:type];
        [regionValues addObject:webRegion];
        [webRegion release];
    }

    [self _addScrollerDashboardRegions:webRegions];

    return webRegions;
}

- (void)_setDashboardBehavior:(WebDashboardBehavior)behavior to:(BOOL)flag
{
    // FIXME: Remove this blanket assignment once Dashboard and Dashcode implement 
    // specific support for the backward compatibility mode flag.
    if (behavior == WebDashboardBehaviorAllowWheelScrolling && flag == NO && _private->page)
        _private->page->settings()->setUsesDashboardBackwardCompatibilityMode(true);
    
    switch (behavior) {
        case WebDashboardBehaviorAlwaysSendMouseEventsToAllWindows: {
            _private->dashboardBehaviorAlwaysSendMouseEventsToAllWindows = flag;
            break;
        }
        case WebDashboardBehaviorAlwaysSendActiveNullEventsToPlugIns: {
            _private->dashboardBehaviorAlwaysSendActiveNullEventsToPlugIns = flag;
            break;
        }
        case WebDashboardBehaviorAlwaysAcceptsFirstMouse: {
            _private->dashboardBehaviorAlwaysAcceptsFirstMouse = flag;
            break;
        }
        case WebDashboardBehaviorAllowWheelScrolling: {
            _private->dashboardBehaviorAllowWheelScrolling = flag;
            break;
        }
        case WebDashboardBehaviorUseBackwardCompatibilityMode: {
            if (_private->page)
                _private->page->settings()->setUsesDashboardBackwardCompatibilityMode(flag);
#if ENABLE(LEGACY_CSS_VENDOR_PREFIXES)
            RuntimeEnabledFeatures::setLegacyCSSVendorPrefixesEnabled(flag);
#endif
            break;
        }
    }

    // Pre-HTML5 parser quirks should be enabled if Dashboard is in backward
    // compatibility mode. See <rdar://problem/8175982>.
    if (_private->page)
        _private->page->settings()->setUsePreHTML5ParserQuirks([self _needsPreHTML5ParserQuirks]);
}

- (BOOL)_dashboardBehavior:(WebDashboardBehavior)behavior
{
    switch (behavior) {
        case WebDashboardBehaviorAlwaysSendMouseEventsToAllWindows: {
            return _private->dashboardBehaviorAlwaysSendMouseEventsToAllWindows;
        }
        case WebDashboardBehaviorAlwaysSendActiveNullEventsToPlugIns: {
            return _private->dashboardBehaviorAlwaysSendActiveNullEventsToPlugIns;
        }
        case WebDashboardBehaviorAlwaysAcceptsFirstMouse: {
            return _private->dashboardBehaviorAlwaysAcceptsFirstMouse;
        }
        case WebDashboardBehaviorAllowWheelScrolling: {
            return _private->dashboardBehaviorAllowWheelScrolling;
        }
        case WebDashboardBehaviorUseBackwardCompatibilityMode: {
            return _private->page && _private->page->settings()->usesDashboardBackwardCompatibilityMode();
        }
    }
    return NO;
}

#endif /* ENABLE(DASHBOARD_SUPPORT) */

+ (void)_setShouldUseFontSmoothing:(BOOL)f
{
    Font::setShouldUseSmoothing(f);
}

+ (BOOL)_shouldUseFontSmoothing
{
    return Font::shouldUseSmoothing();
}

+ (void)_setUsesTestModeFocusRingColor:(BOOL)f
{
    setUsesTestModeFocusRingColor(f);
}

+ (BOOL)_usesTestModeFocusRingColor
{
    return usesTestModeFocusRingColor();
}

- (void)setAlwaysShowVerticalScroller:(BOOL)flag
{
    WebDynamicScrollBarsView *scrollview = [[[self mainFrame] frameView] _scrollView];
    if (flag) {
        [scrollview setVerticalScrollingMode:ScrollbarAlwaysOn andLock:YES];
    } else {
        [scrollview setVerticalScrollingModeLocked:NO];
        [scrollview setVerticalScrollingMode:ScrollbarAuto andLock:NO];
    }
}

- (BOOL)alwaysShowVerticalScroller
{
    WebDynamicScrollBarsView *scrollview = [[[self mainFrame] frameView] _scrollView];
    return [scrollview verticalScrollingModeLocked] && [scrollview verticalScrollingMode] == ScrollbarAlwaysOn;
}

- (void)setAlwaysShowHorizontalScroller:(BOOL)flag
{
    WebDynamicScrollBarsView *scrollview = [[[self mainFrame] frameView] _scrollView];
    if (flag) {
        [scrollview setHorizontalScrollingMode:ScrollbarAlwaysOn andLock:YES];
    } else {
        [scrollview setHorizontalScrollingModeLocked:NO];
        [scrollview setHorizontalScrollingMode:ScrollbarAuto andLock:NO];
    }
}

- (void)setProhibitsMainFrameScrolling:(BOOL)prohibits
{
    if (Frame* mainFrame = [self _mainCoreFrame])
        mainFrame->view()->setProhibitsScrolling(prohibits);
}

- (BOOL)alwaysShowHorizontalScroller
{
    WebDynamicScrollBarsView *scrollview = [[[self mainFrame] frameView] _scrollView];
    return [scrollview horizontalScrollingModeLocked] && [scrollview horizontalScrollingMode] == ScrollbarAlwaysOn;
}

- (void)_setInViewSourceMode:(BOOL)flag
{
    if (Frame* mainFrame = [self _mainCoreFrame])
        mainFrame->setInViewSourceMode(flag);
}

- (BOOL)_inViewSourceMode
{
    Frame* mainFrame = [self _mainCoreFrame];
    return mainFrame && mainFrame->inViewSourceMode();
}

- (void)_setUseFastImageScalingMode:(BOOL)flag
{
    if (_private->page && _private->page->inLowQualityImageInterpolationMode() != flag) {
        _private->page->setInLowQualityImageInterpolationMode(flag);
        [self setNeedsDisplay:YES];
    }
}

- (BOOL)_inFastImageScalingMode
{
    if (_private->page)
        return _private->page->inLowQualityImageInterpolationMode();
    return NO;
}

- (BOOL)_cookieEnabled
{
    if (_private->page)
        return _private->page->settings()->cookieEnabled();
    return YES;
}

- (void)_setCookieEnabled:(BOOL)enable
{
    if (_private->page)
        _private->page->settings()->setCookieEnabled(enable);
}

- (void)_setAdditionalWebPlugInPaths:(NSArray *)newPaths
{
    if (!_private->pluginDatabase)
        _private->pluginDatabase = [[WebPluginDatabase alloc] init];
        
    [_private->pluginDatabase setPlugInPaths:newPaths];
    [_private->pluginDatabase refresh];
}

- (void)_attachScriptDebuggerToAllFrames
{
    for (Frame* frame = [self _mainCoreFrame]; frame; frame = frame->tree()->traverseNext())
        [kit(frame) _attachScriptDebugger];
}

- (void)_detachScriptDebuggerFromAllFrames
{
    for (Frame* frame = [self _mainCoreFrame]; frame; frame = frame->tree()->traverseNext())
        [kit(frame) _detachScriptDebugger];
}

- (void)setBackgroundColor:(NSColor *)backgroundColor
{
    if ([_private->backgroundColor isEqual:backgroundColor])
        return;

    id old = _private->backgroundColor;
    _private->backgroundColor = [backgroundColor retain];
    [old release];

    [[self mainFrame] _updateBackgroundAndUpdatesWhileOffscreen];
}

- (NSColor *)backgroundColor
{
    return _private->backgroundColor;
}

- (BOOL)defersCallbacks
{
    if (!_private->page)
        return NO;
    return _private->page->defersLoading();
}

- (void)setDefersCallbacks:(BOOL)defer
{
    if (!_private->page)
        return;
    return _private->page->setDefersLoading(defer);
}

// For backwards compatibility with the WebBackForwardList API, we honor both
// a per-WebView and a per-preferences setting for whether to use the page cache.

- (BOOL)usesPageCache
{
    return _private->usesPageCache && [[self preferences] usesPageCache];
}

- (void)setUsesPageCache:(BOOL)usesPageCache
{
    _private->usesPageCache = usesPageCache;

    // Update our own settings and post the public notification only
    [self _preferencesChanged:[self preferences]];
    [[self preferences] _postPreferencesChangedAPINotification];
}

- (WebHistoryItem *)_globalHistoryItem
{
    if (!_private)
        return nil;

    return kit(_private->_globalHistoryItem.get());
}

- (void)_setGlobalHistoryItem:(HistoryItem*)historyItem
{
    _private->_globalHistoryItem = historyItem;
}

- (WebTextIterator *)textIteratorForRect:(NSRect)rect
{
    IntPoint rectStart(rect.origin.x, rect.origin.y);
    IntPoint rectEnd(rect.origin.x + rect.size.width, rect.origin.y + rect.size.height);
    
    Frame* coreFrame = [self _mainCoreFrame];
    if (!coreFrame)
        return nil;
    
    VisibleSelection selectionInsideRect(coreFrame->visiblePositionForPoint(rectStart), coreFrame->visiblePositionForPoint(rectEnd));
    
    return [[[WebTextIterator alloc] initWithRange:kit(selectionInsideRect.toNormalizedRange().get())] autorelease];
}

- (void)handleAuthenticationForResource:(id)identifier challenge:(NSURLAuthenticationChallenge *)challenge fromDataSource:(WebDataSource *)dataSource 
{
    NSWindow *window = [self hostWindow] ? [self hostWindow] : [self window]; 
    [[WebPanelAuthenticationHandler sharedHandler] startAuthentication:challenge window:window]; 
} 

- (void)_clearUndoRedoOperations
{
    if (!_private->page)
        return;
    _private->page->clearUndoRedoOperations();
}

- (void)_executeCoreCommandByName:(NSString *)name value:(NSString *)value
{
    Frame* coreFrame = [self _mainCoreFrame];
    if (!coreFrame)
        return;
    coreFrame->editor().command(name).execute(value);
}

- (void)_setCustomHTMLTokenizerTimeDelay:(double)timeDelay
{
    if (!_private->page)
        return;
    return _private->page->setCustomHTMLTokenizerTimeDelay(timeDelay);
}

- (void)_setCustomHTMLTokenizerChunkSize:(int)chunkSize
{
    if (!_private->page)
        return;
    return _private->page->setCustomHTMLTokenizerChunkSize(chunkSize);
}

- (void)_clearMainFrameName
{
    _private->page->mainFrame()->tree()->clearName();
}

- (void)setSelectTrailingWhitespaceEnabled:(BOOL)flag
{
    if (_private->page->settings()->selectTrailingWhitespaceEnabled() != flag) {
        _private->page->settings()->setSelectTrailingWhitespaceEnabled(flag);
        [self setSmartInsertDeleteEnabled:!flag];
    }
}

- (BOOL)isSelectTrailingWhitespaceEnabled
{
    return _private->page->settings()->selectTrailingWhitespaceEnabled();
}

- (void)setMemoryCacheDelegateCallsEnabled:(BOOL)enabled
{
    _private->page->setMemoryCacheClientCallsEnabled(enabled);
}

- (BOOL)areMemoryCacheDelegateCallsEnabled
{
    return _private->page->areMemoryCacheClientCallsEnabled();
}

+ (NSCursor *)_pointingHandCursor
{
    return handCursor().platformCursor();
}

- (BOOL)_postsAcceleratedCompositingNotifications
{
#if USE(ACCELERATED_COMPOSITING)
    return _private->postsAcceleratedCompositingNotifications;
#else
    return NO;
#endif

}
- (void)_setPostsAcceleratedCompositingNotifications:(BOOL)flag
{
#if USE(ACCELERATED_COMPOSITING)
    _private->postsAcceleratedCompositingNotifications = flag;
#endif
}

- (BOOL)_isUsingAcceleratedCompositing
{
#if USE(ACCELERATED_COMPOSITING)
    Frame* coreFrame = [self _mainCoreFrame];
    for (Frame* frame = coreFrame; frame; frame = frame->tree()->traverseNext(coreFrame)) {
        NSView *documentView = [[kit(frame) frameView] documentView];
        if ([documentView isKindOfClass:[WebHTMLView class]] && [(WebHTMLView *)documentView _isUsingAcceleratedCompositing])
            return YES;
    }
#endif
    return NO;
}

- (void)_setBaseCTM:(CGAffineTransform)transform forContext:(CGContextRef)context
{
    WKSetBaseCTM(context, transform);
}

- (BOOL)interactiveFormValidationEnabled
{
    return _private->interactiveFormValidationEnabled;
}

- (void)setInteractiveFormValidationEnabled:(BOOL)enabled
{
    _private->interactiveFormValidationEnabled = enabled;
}

- (int)validationMessageTimerMagnification
{
    return _private->validationMessageTimerMagnification;
}

- (void)setValidationMessageTimerMagnification:(int)newValue
{
    _private->validationMessageTimerMagnification = newValue;
}

- (BOOL)_isSoftwareRenderable
{
#if USE(ACCELERATED_COMPOSITING)
    Frame* coreFrame = [self _mainCoreFrame];
    for (Frame* frame = coreFrame; frame; frame = frame->tree()->traverseNext(coreFrame)) {
        if (FrameView* view = frame->view()) {
            if (!view->isSoftwareRenderable())
                return NO;
        }
    }
#endif
    return YES;
}

- (void)_setIncludesFlattenedCompositingLayersWhenDrawingToBitmap:(BOOL)flag
{
    _private->includesFlattenedCompositingLayersWhenDrawingToBitmap = flag;
}

- (BOOL)_includesFlattenedCompositingLayersWhenDrawingToBitmap
{
    return _private->includesFlattenedCompositingLayersWhenDrawingToBitmap;
}

- (void)setTracksRepaints:(BOOL)flag
{
    Frame* coreFrame = [self _mainCoreFrame];
    if (FrameView* view = coreFrame->view())
        view->setTracksRepaints(flag);
}

- (BOOL)isTrackingRepaints
{
    Frame* coreFrame = [self _mainCoreFrame];
    if (FrameView* view = coreFrame->view())
        return view->isTrackingRepaints();
    
    return NO;
}

- (void)resetTrackedRepaints
{
    Frame* coreFrame = [self _mainCoreFrame];
    if (FrameView* view = coreFrame->view())
        view->resetTrackedRepaints();
}

- (NSArray*)trackedRepaintRects
{
    Frame* coreFrame = [self _mainCoreFrame];
    FrameView* view = coreFrame->view();
    if (!view || !view->isTrackingRepaints())
        return nil;

    const Vector<IntRect>& repaintRects = view->trackedRepaintRects();
    NSMutableArray* rectsArray = [[NSMutableArray alloc] initWithCapacity:repaintRects.size()];
    
    for (unsigned i = 0; i < repaintRects.size(); ++i)
        [rectsArray addObject:[NSValue valueWithRect:pixelSnappedIntRect(repaintRects[i])]];

    return [rectsArray autorelease];
}

- (NSPasteboard *)_insertionPasteboard
{
    return _private ? _private->insertionPasteboard : nil;
}

+ (void)_addOriginAccessWhitelistEntryWithSourceOrigin:(NSString *)sourceOrigin destinationProtocol:(NSString *)destinationProtocol destinationHost:(NSString *)destinationHost allowDestinationSubdomains:(BOOL)allowDestinationSubdomains
{
    SecurityPolicy::addOriginAccessWhitelistEntry(*SecurityOrigin::createFromString(sourceOrigin), destinationProtocol, destinationHost, allowDestinationSubdomains);
}

+ (void)_removeOriginAccessWhitelistEntryWithSourceOrigin:(NSString *)sourceOrigin destinationProtocol:(NSString *)destinationProtocol destinationHost:(NSString *)destinationHost allowDestinationSubdomains:(BOOL)allowDestinationSubdomains
{
    SecurityPolicy::removeOriginAccessWhitelistEntry(*SecurityOrigin::createFromString(sourceOrigin), destinationProtocol, destinationHost, allowDestinationSubdomains);
}

+ (void)_resetOriginAccessWhitelists
{
    SecurityPolicy::resetOriginAccessWhitelists();
}

- (BOOL)_isViewVisible
{
    if (![self window])
        return false;

    if (![[self window] isVisible])
        return false;

    if ([self isHiddenOrHasHiddenAncestor])
        return false;

    return true;
}

- (void)_updateVisibilityState
{
    if (_private && _private->page)
        [self _setVisibilityState:([self _isViewVisible] ? WebPageVisibilityStateVisible : WebPageVisibilityStateHidden) isInitialState:NO];
}

- (void)_updateActiveState
{
    if (_private && _private->page)
        _private->page->focusController()->setActive([[self window] _hasKeyAppearance]);
}

static Vector<String> toStringVector(NSArray* patterns)
{
    Vector<String> patternsVector;

    NSUInteger count = [patterns count];
    if (!count)
        return patternsVector;

    for (NSUInteger i = 0; i < count; ++i) {
        id entry = [patterns objectAtIndex:i];
        if ([entry isKindOfClass:[NSString class]])
            patternsVector.append(String((NSString *)entry));
    }
    return patternsVector;
}

+ (void)_addUserScriptToGroup:(NSString *)groupName world:(WebScriptWorld *)world source:(NSString *)source url:(NSURL *)url
                    whitelist:(NSArray *)whitelist blacklist:(NSArray *)blacklist
                injectionTime:(WebUserScriptInjectionTime)injectionTime
{
    [WebView _addUserScriptToGroup:groupName world:world source:source url:url whitelist:whitelist blacklist:blacklist injectionTime:injectionTime injectedFrames:WebInjectInAllFrames];
}

+ (void)_addUserScriptToGroup:(NSString *)groupName world:(WebScriptWorld *)world source:(NSString *)source url:(NSURL *)url
                    whitelist:(NSArray *)whitelist blacklist:(NSArray *)blacklist
                injectionTime:(WebUserScriptInjectionTime)injectionTime
               injectedFrames:(WebUserContentInjectedFrames)injectedFrames
{
    String group(groupName);
    if (group.isEmpty())
        return;
    
    PageGroup* pageGroup = PageGroup::pageGroup(group);
    if (!pageGroup)
        return;
    
    pageGroup->addUserScriptToWorld(core(world), source, url, toStringVector(whitelist), toStringVector(blacklist), 
                                    injectionTime == WebInjectAtDocumentStart ? InjectAtDocumentStart : InjectAtDocumentEnd,
                                    injectedFrames == WebInjectInAllFrames ? InjectInAllFrames : InjectInTopFrameOnly);
}

+ (void)_addUserStyleSheetToGroup:(NSString *)groupName world:(WebScriptWorld *)world source:(NSString *)source url:(NSURL *)url
                        whitelist:(NSArray *)whitelist blacklist:(NSArray *)blacklist
{
    [WebView _addUserStyleSheetToGroup:groupName world:world source:source url:url whitelist:whitelist blacklist:blacklist injectedFrames:WebInjectInAllFrames];
}

+ (void)_addUserStyleSheetToGroup:(NSString *)groupName world:(WebScriptWorld *)world source:(NSString *)source url:(NSURL *)url
                        whitelist:(NSArray *)whitelist blacklist:(NSArray *)blacklist
                   injectedFrames:(WebUserContentInjectedFrames)injectedFrames
{
    String group(groupName);
    if (group.isEmpty())
        return;
    
    PageGroup* pageGroup = PageGroup::pageGroup(group);
    if (!pageGroup)
        return;

    pageGroup->addUserStyleSheetToWorld(core(world), source, url, toStringVector(whitelist), toStringVector(blacklist), injectedFrames == WebInjectInAllFrames ? InjectInAllFrames : InjectInTopFrameOnly);
}

+ (void)_removeUserScriptFromGroup:(NSString *)groupName world:(WebScriptWorld *)world url:(NSURL *)url
{
    String group(groupName);
    if (group.isEmpty())
        return;
    
    PageGroup* pageGroup = PageGroup::pageGroup(group);
    if (!pageGroup)
        return;

    pageGroup->removeUserScriptFromWorld(core(world), url);
}

+ (void)_removeUserStyleSheetFromGroup:(NSString *)groupName world:(WebScriptWorld *)world url:(NSURL *)url
{
    String group(groupName);
    if (group.isEmpty())
        return;
    
    PageGroup* pageGroup = PageGroup::pageGroup(group);
    if (!pageGroup)
        return;

    pageGroup->removeUserStyleSheetFromWorld(core(world), url);
}

+ (void)_removeUserScriptsFromGroup:(NSString *)groupName world:(WebScriptWorld *)world
{
    String group(groupName);
    if (group.isEmpty())
        return;
    
    PageGroup* pageGroup = PageGroup::pageGroup(group);
    if (!pageGroup)
        return;

    pageGroup->removeUserScriptsFromWorld(core(world));
}

+ (void)_removeUserStyleSheetsFromGroup:(NSString *)groupName world:(WebScriptWorld *)world
{
    String group(groupName);
    if (group.isEmpty())
        return;
    
    PageGroup* pageGroup = PageGroup::pageGroup(group);
    if (!pageGroup)
        return;

    pageGroup->removeUserStyleSheetsFromWorld(core(world));
}

+ (void)_removeAllUserContentFromGroup:(NSString *)groupName
{
    String group(groupName);
    if (group.isEmpty())
        return;
    
    PageGroup* pageGroup = PageGroup::pageGroup(group);
    if (!pageGroup)
        return;
    
    pageGroup->removeAllUserContent();
}

- (BOOL)cssAnimationsSuspended
{
    // should ask the page!
    Frame* frame = core([self mainFrame]);
    if (frame)
        return frame->animation()->isSuspended();

    return false;
}

- (void)setCSSAnimationsSuspended:(BOOL)suspended
{
    Frame* frame = core([self mainFrame]);
    if (suspended == frame->animation()->isSuspended())
        return;
        
    if (suspended)
        frame->animation()->suspendAnimations();
    else
        frame->animation()->resumeAnimations();
}

+ (void)_setDomainRelaxationForbidden:(BOOL)forbidden forURLScheme:(NSString *)scheme
{
    SchemeRegistry::setDomainRelaxationForbiddenForURLScheme(forbidden, scheme);
}

+ (void)_registerURLSchemeAsSecure:(NSString *)scheme
{
    SchemeRegistry::registerURLSchemeAsSecure(scheme);
}

+ (void)_registerURLSchemeAsAllowingLocalStorageAccessInPrivateBrowsing:(NSString *)scheme
{
    SchemeRegistry::registerURLSchemeAsAllowingLocalStorageAccessInPrivateBrowsing(scheme);
}

+ (void)_registerURLSchemeAsAllowingDatabaseAccessInPrivateBrowsing:(NSString *)scheme
{
    SchemeRegistry::registerURLSchemeAsAllowingDatabaseAccessInPrivateBrowsing(scheme);
}

- (void)_scaleWebView:(float)scale atOrigin:(NSPoint)origin
{
    _private->page->setPageScaleFactor(scale, IntPoint(origin));
}

- (float)_viewScaleFactor
{
    return _private->page->pageScaleFactor();
}

- (void)_setUseFixedLayout:(BOOL)fixed
{
    Frame* coreFrame = [self _mainCoreFrame];
    if (!coreFrame)
        return;

    FrameView* view = coreFrame->view();
    if (!view)
        return;

    view->setUseFixedLayout(fixed);
    if (!fixed)
        view->setFixedLayoutSize(IntSize());
}

- (void)_setFixedLayoutSize:(NSSize)size
{
    Frame* coreFrame = [self _mainCoreFrame];
    if (!coreFrame)
        return;
    
    FrameView* view = coreFrame->view();
    if (!view)
        return;
    
    view->setFixedLayoutSize(IntSize(size));
    view->forceLayout();
}

- (BOOL)_useFixedLayout
{
    Frame* coreFrame = [self _mainCoreFrame];
    if (!coreFrame)
        return NO;
    
    FrameView* view = coreFrame->view();
    if (!view)
        return NO;
    
    return view->useFixedLayout();
}

- (NSSize)_fixedLayoutSize
{
    Frame* coreFrame = [self _mainCoreFrame];
    if (!coreFrame)
        return IntSize();
    
    FrameView* view = coreFrame->view();
    if (!view)
        return IntSize();

    return view->fixedLayoutSize();
}

- (void)_setPaginationMode:(WebPaginationMode)paginationMode
{
    Page* page = core(self);
    if (!page)
        return;

    Pagination pagination = page->pagination();
    switch (paginationMode) {
    case WebPaginationModeUnpaginated:
        pagination.mode = Pagination::Unpaginated;
        break;
    case WebPaginationModeLeftToRight:
        pagination.mode = Pagination::LeftToRightPaginated;
        break;
    case WebPaginationModeRightToLeft:
        pagination.mode = Pagination::RightToLeftPaginated;
        break;
    case WebPaginationModeTopToBottom:
        pagination.mode = Pagination::TopToBottomPaginated;
        break;
    case WebPaginationModeBottomToTop:
        pagination.mode = Pagination::BottomToTopPaginated;
        break;
    default:
        return;
    }

    page->setPagination(pagination);
}

- (WebPaginationMode)_paginationMode
{
    Page* page = core(self);
    if (!page)
        return WebPaginationModeUnpaginated;

    switch (page->pagination().mode) {
    case Pagination::Unpaginated:
        return WebPaginationModeUnpaginated;
    case Pagination::LeftToRightPaginated:
        return WebPaginationModeLeftToRight;
    case Pagination::RightToLeftPaginated:
        return WebPaginationModeRightToLeft;
    case Pagination::TopToBottomPaginated:
        return WebPaginationModeTopToBottom;
    case Pagination::BottomToTopPaginated:
        return WebPaginationModeBottomToTop;
    }

    ASSERT_NOT_REACHED();
    return WebPaginationModeUnpaginated;
}

- (void)_listenForLayoutMilestones:(WebLayoutMilestones)layoutMilestones
{
    Page* page = core(self);
    if (!page)
        return;

    page->addLayoutMilestones(coreLayoutMilestones(layoutMilestones));
}

- (WebLayoutMilestones)_layoutMilestones
{
    Page* page = core(self);
    if (!page)
        return 0;

    return kitLayoutMilestones(page->requestedLayoutMilestones());
}

- (WebPageVisibilityState)_visibilityState
{
#if ENABLE(PAGE_VISIBILITY_API) || ENABLE(HIDDEN_PAGE_DOM_TIMER_THROTTLING)
    if (_private->page)
        return kit(_private->page->visibilityState());
#endif
    return WebPageVisibilityStateVisible;
}

- (void)_setVisibilityState:(WebPageVisibilityState)visibilityState isInitialState:(BOOL)isInitialState
{
#if ENABLE(PAGE_VISIBILITY_API) || ENABLE(HIDDEN_PAGE_DOM_TIMER_THROTTLING)
    if (_private->page)
        _private->page->setVisibilityState(core(visibilityState), isInitialState);
#endif
}

- (void)_setPaginationBehavesLikeColumns:(BOOL)behavesLikeColumns
{
    Page* page = core(self);
    if (!page)
        return;

    Pagination pagination = page->pagination();
    pagination.behavesLikeColumns = behavesLikeColumns;

    page->setPagination(pagination);
}

- (BOOL)_paginationBehavesLikeColumns
{
    Page* page = core(self);
    if (!page)
        return NO;

    return page->pagination().behavesLikeColumns;
}

- (void)_setPageLength:(CGFloat)pageLength
{
    Page* page = core(self);
    if (!page)
        return;

    Pagination pagination = page->pagination();
    pagination.pageLength = pageLength;

    page->setPagination(pagination);
}

- (CGFloat)_pageLength
{
    Page* page = core(self);
    if (!page)
        return 1;

    return page->pagination().pageLength;
}

- (void)_setGapBetweenPages:(CGFloat)pageGap
{
    Page* page = core(self);
    if (!page)
        return;

    Pagination pagination = page->pagination();
    pagination.gap = pageGap;
    page->setPagination(pagination);
}

- (CGFloat)_gapBetweenPages
{
    Page* page = core(self);
    if (!page)
        return 0;

    return page->pagination().gap;
}

- (NSUInteger)_pageCount
{
    Page* page = core(self);
    if (!page)
        return 0;

    return page->pageCount();
}

- (CGFloat)_backingScaleFactor
{
    return [self _deviceScaleFactor];
}

- (void)_setCustomBackingScaleFactor:(CGFloat)customScaleFactor
{
    float oldScaleFactor = [self _deviceScaleFactor];

    _private->customDeviceScaleFactor = customScaleFactor;

    if (oldScaleFactor != [self _deviceScaleFactor])
        _private->page->setDeviceScaleFactor([self _deviceScaleFactor]);
}

- (NSUInteger)markAllMatchesForText:(NSString *)string caseSensitive:(BOOL)caseFlag highlight:(BOOL)highlight limit:(NSUInteger)limit
{
    return [self countMatchesForText:string options:(caseFlag ? 0 : WebFindOptionsCaseInsensitive) highlight:highlight limit:limit markMatches:YES];
}

- (NSUInteger)countMatchesForText:(NSString *)string caseSensitive:(BOOL)caseFlag highlight:(BOOL)highlight limit:(NSUInteger)limit markMatches:(BOOL)markMatches
{
    return [self countMatchesForText:string options:(caseFlag ? 0 : WebFindOptionsCaseInsensitive) highlight:highlight limit:limit markMatches:markMatches];
}

- (BOOL)searchFor:(NSString *)string direction:(BOOL)forward caseSensitive:(BOOL)caseFlag wrap:(BOOL)wrapFlag startInSelection:(BOOL)startInSelection
{
    return [self findString:string options:((forward ? 0 : WebFindOptionsBackwards) | (caseFlag ? 0 : WebFindOptionsCaseInsensitive) | (wrapFlag ? WebFindOptionsWrapAround : 0) | (startInSelection ? WebFindOptionsStartInSelection : 0))];
}

+ (void)_setLoadResourcesSerially:(BOOL)serialize 
{
    WebPlatformStrategies::initializeIfNecessary();
    resourceLoadScheduler()->setSerialLoadingEnabled(serialize);
}

+ (BOOL)_HTTPPipeliningEnabled
{
    return ResourceRequest::httpPipeliningEnabled();
}

+ (void)_setHTTPPipeliningEnabled:(BOOL)enabled
{
    ResourceRequest::setHTTPPipeliningEnabled(enabled);
}

- (void)_setSourceApplicationAuditData:(NSData *)sourceApplicationAuditData
{
    if (_private->sourceApplicationAuditData == sourceApplicationAuditData)
        return;

    _private->sourceApplicationAuditData = adoptNS([sourceApplicationAuditData copy]);
}

- (NSData *)_sourceApplicationAuditData
{
    return _private->sourceApplicationAuditData.get();
}

@end

@implementation _WebSafeForwarder

// Used to send messages to delegates that implement informal protocols.

- (id)initWithTarget:(id)t defaultTarget:(id)dt
{
    self = [super init];
    if (!self)
        return nil;
    target = t; // Non retained.
    defaultTarget = dt;
    return self;
}

- (void)forwardInvocation:(NSInvocation *)invocation
{
    if ([target respondsToSelector:[invocation selector]]) {
        @try {
            [invocation invokeWithTarget:target];
        } @catch(id exception) {
            ReportDiscardedDelegateException([invocation selector], exception);
        }
        return;
    }

    if ([defaultTarget respondsToSelector:[invocation selector]])
        [invocation invokeWithTarget:defaultTarget];

    // Do nothing quietly if method not implemented.
}

- (NSMethodSignature *)methodSignatureForSelector:(SEL)aSelector
{
    return [defaultTarget methodSignatureForSelector:aSelector];
}

@end

@implementation WebView

+ (void)initialize
{
    static BOOL initialized = NO;
    if (initialized)
        return;
    initialized = YES;

    InitWebCoreSystemInterface();
    JSC::initializeThreading();
    WTF::initializeMainThreadToProcessMainThread();
    WebCore::RunLoop::initializeMainRunLoop();

    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(_applicationWillTerminate) name:NSApplicationWillTerminateNotification object:NSApp];
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(_cacheModelChangedNotification:) name:WebPreferencesCacheModelChangedInternalNotification object:nil];
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(_preferencesRemovedNotification:) name:WebPreferencesRemovedNotification object:nil];

    NSUserDefaults *defaults = [NSUserDefaults standardUserDefaults];

#if __MAC_OS_X_VERSION_MIN_REQUIRED >= 1090
    [defaults registerDefaults:[NSDictionary dictionaryWithObject:[NSNumber numberWithBool:YES] forKey:WebKitKerningAndLigaturesEnabledByDefaultDefaultsKey]];
#endif

    continuousSpellCheckingEnabled = [defaults boolForKey:WebContinuousSpellCheckingEnabled];
    grammarCheckingEnabled = [defaults boolForKey:WebGrammarCheckingEnabled];

    Font::setDefaultTypesettingFeatures([defaults boolForKey:WebKitKerningAndLigaturesEnabledByDefaultDefaultsKey] ? Kerning | Ligatures : 0);

    automaticQuoteSubstitutionEnabled = [self _shouldAutomaticQuoteSubstitutionBeEnabled];
    automaticLinkDetectionEnabled = [defaults boolForKey:WebAutomaticLinkDetectionEnabled];
    automaticDashSubstitutionEnabled = [self _shouldAutomaticDashSubstitutionBeEnabled];
    automaticTextReplacementEnabled = [self _shouldAutomaticTextReplacementBeEnabled];
    automaticSpellingCorrectionEnabled = [self _shouldAutomaticSpellingCorrectionBeEnabled];

    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(_didChangeAutomaticTextReplacementEnabled:)
        name:NSSpellCheckerDidChangeAutomaticTextReplacementNotification object:nil];
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(_didChangeAutomaticSpellingCorrectionEnabled:)
        name:NSSpellCheckerDidChangeAutomaticSpellingCorrectionNotification object:nil];

#if __MAC_OS_X_VERSION_MIN_REQUIRED >= 1090
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(_didChangeAutomaticQuoteSubstitutionEnabled:)
        name:NSSpellCheckerDidChangeAutomaticQuoteSubstitutionNotification object:nil];
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(_didChangeAutomaticDashSubstitutionEnabled:)
        name:NSSpellCheckerDidChangeAutomaticDashSubstitutionNotification object:nil];
#endif
}

+ (BOOL)_shouldAutomaticTextReplacementBeEnabled
{
    NSUserDefaults *defaults = [NSUserDefaults standardUserDefaults];
    if (![defaults objectForKey:WebAutomaticTextReplacementEnabled])
        return [NSSpellChecker isAutomaticTextReplacementEnabled];
    return [defaults boolForKey:WebAutomaticTextReplacementEnabled];
}

+ (void)_didChangeAutomaticTextReplacementEnabled:(NSNotification *)notification
{
    automaticTextReplacementEnabled = [self _shouldAutomaticTextReplacementBeEnabled];
    [[NSSpellChecker sharedSpellChecker] updatePanels];
}

+ (BOOL)_shouldAutomaticSpellingCorrectionBeEnabled
{
    NSUserDefaults *defaults = [NSUserDefaults standardUserDefaults];
    if (![defaults objectForKey:WebAutomaticSpellingCorrectionEnabled])
        return [NSSpellChecker isAutomaticTextReplacementEnabled];
    return [defaults boolForKey:WebAutomaticSpellingCorrectionEnabled];
}

+ (void)_didChangeAutomaticSpellingCorrectionEnabled:(NSNotification *)notification
{
    automaticSpellingCorrectionEnabled = [self _shouldAutomaticSpellingCorrectionBeEnabled];
    [[NSSpellChecker sharedSpellChecker] updatePanels];
}

+ (BOOL)_shouldAutomaticQuoteSubstitutionBeEnabled
{
    NSUserDefaults *defaults = [NSUserDefaults standardUserDefaults];
#if __MAC_OS_X_VERSION_MIN_REQUIRED >= 1090
    if (![defaults objectForKey:WebAutomaticQuoteSubstitutionEnabled])
        return [NSSpellChecker isAutomaticQuoteSubstitutionEnabled];
#endif
    return [defaults boolForKey:WebAutomaticQuoteSubstitutionEnabled];
}

+ (BOOL)_shouldAutomaticDashSubstitutionBeEnabled
{
    NSUserDefaults *defaults = [NSUserDefaults standardUserDefaults];
#if __MAC_OS_X_VERSION_MIN_REQUIRED >= 1090
    if (![defaults objectForKey:WebAutomaticDashSubstitutionEnabled])
        return [NSSpellChecker isAutomaticDashSubstitutionEnabled];
#endif
    return [defaults boolForKey:WebAutomaticDashSubstitutionEnabled];
}

#if __MAC_OS_X_VERSION_MIN_REQUIRED >= 1090
+ (void)_didChangeAutomaticQuoteSubstitutionEnabled:(NSNotification *)notification
{
    automaticQuoteSubstitutionEnabled = [self _shouldAutomaticQuoteSubstitutionBeEnabled];
    [[NSSpellChecker sharedSpellChecker] updatePanels];
}

+ (void)_didChangeAutomaticDashSubstitutionEnabled:(NSNotification *)notification
{
    automaticDashSubstitutionEnabled = [self _shouldAutomaticDashSubstitutionBeEnabled];
    [[NSSpellChecker sharedSpellChecker] updatePanels];
}
#endif

+ (void)_applicationWillTerminate
{   
    applicationIsTerminating = YES;

    if (fastDocumentTeardownEnabled())
        [self closeAllWebViews];

    if (!pluginDatabaseClientCount)
        [WebPluginDatabase closeSharedDatabase];

    PageGroup::closeLocalStorage();
}

+ (BOOL)_canShowMIMEType:(NSString *)MIMEType allowingPlugins:(BOOL)allowPlugins
{
    return [self _viewClass:nil andRepresentationClass:nil forMIMEType:MIMEType allowingPlugins:allowPlugins];
}

+ (BOOL)canShowMIMEType:(NSString *)MIMEType
{
    return [self _canShowMIMEType:MIMEType allowingPlugins:YES];
}

- (BOOL)_canShowMIMEType:(NSString *)MIMEType
{
    return [[self class] _canShowMIMEType:MIMEType allowingPlugins:[_private->preferences arePlugInsEnabled]];
}

- (WebBasePluginPackage *)_pluginForMIMEType:(NSString *)MIMEType
{
    if (![_private->preferences arePlugInsEnabled])
        return nil;

    WebBasePluginPackage *pluginPackage = [[WebPluginDatabase sharedDatabase] pluginForMIMEType:MIMEType];
    if (pluginPackage)
        return pluginPackage;
    
    if (_private->pluginDatabase)
        return [_private->pluginDatabase pluginForMIMEType:MIMEType];
    
    return nil;
}

#if ENABLE(PLUGIN_PROXY_FOR_VIDEO)
- (WebBasePluginPackage *)_videoProxyPluginForMIMEType:(NSString *)MIMEType
{
    WebBasePluginPackage *pluginPackage = [[WebPluginDatabase sharedDatabase] pluginForMIMEType:MIMEType];
    if (pluginPackage)
        return pluginPackage;

    if (_private->pluginDatabase)
        return [_private->pluginDatabase pluginForMIMEType:MIMEType];

    return nil;
}
#endif

- (WebBasePluginPackage *)_pluginForExtension:(NSString *)extension
{
    if (![_private->preferences arePlugInsEnabled])
        return nil;

    WebBasePluginPackage *pluginPackage = [[WebPluginDatabase sharedDatabase] pluginForExtension:extension];
    if (pluginPackage)
        return pluginPackage;
    
    if (_private->pluginDatabase)
        return [_private->pluginDatabase pluginForExtension:extension];
    
    return nil;
}

- (void)addPluginInstanceView:(NSView *)view
{
    if (!_private->pluginDatabase)
        _private->pluginDatabase = [[WebPluginDatabase alloc] init];
    [_private->pluginDatabase addPluginInstanceView:view];
}

- (void)removePluginInstanceView:(NSView *)view
{
    if (_private->pluginDatabase)
        [_private->pluginDatabase removePluginInstanceView:view];    
}

- (void)removePluginInstanceViewsFor:(WebFrame*)webFrame 
{
    if (_private->pluginDatabase)
        [_private->pluginDatabase removePluginInstanceViewsFor:webFrame];    
}

- (BOOL)_isMIMETypeRegisteredAsPlugin:(NSString *)MIMEType
{
    if (![_private->preferences arePlugInsEnabled])
        return NO;

    if ([[WebPluginDatabase sharedDatabase] isMIMETypeRegistered:MIMEType])
        return YES;
        
    if (_private->pluginDatabase && [_private->pluginDatabase isMIMETypeRegistered:MIMEType])
        return YES;
    
    return NO;
}

+ (BOOL)canShowMIMETypeAsHTML:(NSString *)MIMEType
{
    return [WebFrameView _canShowMIMETypeAsHTML:MIMEType];
}

+ (NSArray *)MIMETypesShownAsHTML
{
    NSMutableDictionary *viewTypes = [WebFrameView _viewTypesAllowImageTypeOmission:YES];
    NSEnumerator *enumerator = [viewTypes keyEnumerator];
    id key;
    NSMutableArray *array = [[[NSMutableArray alloc] init] autorelease];
    
    while ((key = [enumerator nextObject])) {
        if ([viewTypes objectForKey:key] == [WebHTMLView class])
            [array addObject:key];
    }
    
    return array;
}

+ (void)setMIMETypesShownAsHTML:(NSArray *)MIMETypes
{
    NSDictionary *viewTypes = [[WebFrameView _viewTypesAllowImageTypeOmission:YES] copy];
    NSEnumerator *enumerator = [viewTypes keyEnumerator];
    id key;
    while ((key = [enumerator nextObject])) {
        if ([viewTypes objectForKey:key] == [WebHTMLView class])
            [WebView _unregisterViewClassAndRepresentationClassForMIMEType:key];
    }
    
    int i, count = [MIMETypes count];
    for (i = 0; i < count; i++) {
        [WebView registerViewClass:[WebHTMLView class] 
                representationClass:[WebHTMLRepresentation class] 
                forMIMEType:[MIMETypes objectAtIndex:i]];
    }
    [viewTypes release];
}

+ (NSURL *)URLFromPasteboard:(NSPasteboard *)pasteboard
{
    return [pasteboard _web_bestURL];
}

+ (NSString *)URLTitleFromPasteboard:(NSPasteboard *)pasteboard
{
    return [pasteboard stringForType:WebURLNamePboardType];
}

+ (void)registerURLSchemeAsLocal:(NSString *)protocol
{
    SchemeRegistry::registerURLSchemeAsLocal(protocol);
}

- (id)_initWithArguments:(NSDictionary *) arguments
{
    NSCoder *decoder = [arguments objectForKey:@"decoder"];
    if (decoder) {
        self = [self initWithCoder:decoder];
    } else {
        ASSERT([arguments objectForKey:@"frame"]);
        NSValue *frameValue = [arguments objectForKey:@"frame"];
        NSRect frame = (frameValue ? [frameValue rectValue] : NSZeroRect);
        NSString *frameName = [arguments objectForKey:@"frameName"];
        NSString *groupName = [arguments objectForKey:@"groupName"];
        self = [self initWithFrame:frame frameName:frameName groupName:groupName];
    }

    return self;
}

static bool clientNeedsWebViewInitThreadWorkaround()
{
    if (WebKitLinkedOnOrAfter(WEBKIT_FIRST_VERSION_WITHOUT_WEBVIEW_INIT_THREAD_WORKAROUND))
        return false;

    NSString *bundleIdentifier = [[NSBundle mainBundle] bundleIdentifier];

    // Installer.
    if ([bundleIdentifier _webkit_isCaseInsensitiveEqualToString:@"com.apple.installer"])
        return true;

    // Automator.
    if ([bundleIdentifier _webkit_isCaseInsensitiveEqualToString:@"com.apple.Automator"])
        return true;

    // Automator Runner.
    if ([bundleIdentifier _webkit_isCaseInsensitiveEqualToString:@"com.apple.AutomatorRunner"])
        return true;

    // Automator workflows.
    if ([bundleIdentifier _webkit_hasCaseInsensitivePrefix:@"com.apple.Automator."])
        return true;

    return false;
}

static bool needsWebViewInitThreadWorkaround()
{
    static bool isOldClient = clientNeedsWebViewInitThreadWorkaround();
    return isOldClient && !pthread_main_np();
}

- (id)initWithFrame:(NSRect)f
{
    return [self initWithFrame:f frameName:nil groupName:nil];
}

- (id)initWithFrame:(NSRect)f frameName:(NSString *)frameName groupName:(NSString *)groupName
{
    if (needsWebViewInitThreadWorkaround())
        return [[self _webkit_invokeOnMainThread] initWithFrame:f frameName:frameName groupName:groupName];

    WebCoreThreadViolationCheckRoundTwo();
    return [self _initWithFrame:f frameName:frameName groupName:groupName usesDocumentViews:YES];
}

- (id)initWithCoder:(NSCoder *)decoder
{
    if (needsWebViewInitThreadWorkaround())
        return [[self _webkit_invokeOnMainThread] initWithCoder:decoder];

    WebCoreThreadViolationCheckRoundTwo();
    WebView *result = nil;

    @try {
        NSString *frameName;
        NSString *groupName;
        WebPreferences *preferences;
        BOOL useBackForwardList = NO;
        BOOL allowsUndo = YES;
        
        result = [super initWithCoder:decoder];
        result->_private = [[WebViewPrivate alloc] init];

        // We don't want any of the archived subviews. The subviews will always
        // be created in _commonInitializationFrameName:groupName:.
        [[result subviews] makeObjectsPerformSelector:@selector(removeFromSuperview)];

        if ([decoder allowsKeyedCoding]) {
            frameName = [decoder decodeObjectForKey:@"FrameName"];
            groupName = [decoder decodeObjectForKey:@"GroupName"];
            preferences = [decoder decodeObjectForKey:@"Preferences"];
            useBackForwardList = [decoder decodeBoolForKey:@"UseBackForwardList"];
            if ([decoder containsValueForKey:@"AllowsUndo"])
                allowsUndo = [decoder decodeBoolForKey:@"AllowsUndo"];
        } else {
            int version;
            [decoder decodeValueOfObjCType:@encode(int) at:&version];
            frameName = [decoder decodeObject];
            groupName = [decoder decodeObject];
            preferences = [decoder decodeObject];
            if (version > 1)
                [decoder decodeValuesOfObjCTypes:"c", &useBackForwardList];
            // The allowsUndo field is no longer written out in encodeWithCoder, but since there are
            // version 3 NIBs that have this field encoded, we still need to read it in.
            if (version == 3)
                [decoder decodeValuesOfObjCTypes:"c", &allowsUndo];
        }

        if (![frameName isKindOfClass:[NSString class]])
            frameName = nil;
        if (![groupName isKindOfClass:[NSString class]])
            groupName = nil;
        if (![preferences isKindOfClass:[WebPreferences class]])
            preferences = nil;

        LOG(Encoding, "FrameName = %@, GroupName = %@, useBackForwardList = %d\n", frameName, groupName, (int)useBackForwardList);
        [result _commonInitializationWithFrameName:frameName groupName:groupName];
        static_cast<BackForwardListImpl*>([result page]->backForwardList())->setEnabled(useBackForwardList);
        result->_private->allowsUndo = allowsUndo;
        if (preferences)
            [result setPreferences:preferences];
    } @catch (NSException *localException) {
        result = nil;
        [self release];
    }

    return result;
}

- (void)encodeWithCoder:(NSCoder *)encoder
{
    // Set asside the subviews before we archive. We don't want to archive any subviews.
    // The subviews will always be created in _commonInitializationFrameName:groupName:.
    id originalSubviews = _subviews;
    _subviews = nil;

    [super encodeWithCoder:encoder];

    // Restore the subviews we set aside.
    _subviews = originalSubviews;

    BOOL useBackForwardList = _private->page && static_cast<BackForwardListImpl*>(_private->page->backForwardList())->enabled();
    if ([encoder allowsKeyedCoding]) {
        [encoder encodeObject:[[self mainFrame] name] forKey:@"FrameName"];
        [encoder encodeObject:[self groupName] forKey:@"GroupName"];
        [encoder encodeObject:[self preferences] forKey:@"Preferences"];
        [encoder encodeBool:useBackForwardList forKey:@"UseBackForwardList"];
        [encoder encodeBool:_private->allowsUndo forKey:@"AllowsUndo"];
    } else {
        int version = WebViewVersion;
        [encoder encodeValueOfObjCType:@encode(int) at:&version];
        [encoder encodeObject:[[self mainFrame] name]];
        [encoder encodeObject:[self groupName]];
        [encoder encodeObject:[self preferences]];
        [encoder encodeValuesOfObjCTypes:"c", &useBackForwardList];
        // DO NOT encode any new fields here, doing so will break older WebKit releases.
    }

    LOG(Encoding, "FrameName = %@, GroupName = %@, useBackForwardList = %d\n", [[self mainFrame] name], [self groupName], (int)useBackForwardList);
}

- (void)dealloc
{
    if (WebCoreObjCScheduleDeallocateOnMainThread([WebView class], self))
        return;

    // call close to ensure we tear-down completely
    // this maintains our old behavior for existing applications
    [self close];

    if ([[self class] shouldIncludeInWebKitStatistics])
        --WebViewCount;

    if ([self _needsFrameLoadDelegateRetainQuirk])
        [_private->frameLoadDelegate release];
        
    [_private release];
    // [super dealloc] can end up dispatching against _private (3466082)
    _private = nil;

    [super dealloc];
}

- (void)finalize
{
    ASSERT(_private->closed);

    --WebViewCount;

    [super finalize];
}

- (void)close
{
    // _close existed first, and some clients might be calling or overriding it, so call through.
    [self _close];
}

- (void)setShouldCloseWithWindow:(BOOL)close
{
    _private->shouldCloseWithWindow = close;
}

- (BOOL)shouldCloseWithWindow
{
    return _private->shouldCloseWithWindow;
}

// FIXME: Use AppKit constants for these when they are available.
static NSString * const windowDidChangeBackingPropertiesNotification = @"NSWindowDidChangeBackingPropertiesNotification";
static NSString * const backingPropertyOldScaleFactorKey = @"NSBackingPropertyOldScaleFactorKey"; 

- (void)addWindowObserversForWindow:(NSWindow *)window
{
    if (window) {
        [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(_windowWillOrderOnScreen:)
            name:WKWindowWillOrderOnScreenNotification() object:window];
        [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(_windowWillOrderOffScreen:)
            name:WKWindowWillOrderOffScreenNotification() object:window];
        [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(_windowDidChangeBackingProperties:)
            name:windowDidChangeBackingPropertiesNotification object:window];
        [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(_windowDidChangeScreen:)
            name:NSWindowDidChangeScreenNotification object:window];
        [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(_windowVisibilityChanged:) 
            name:NSWindowDidMiniaturizeNotification object:window];
        [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(_windowVisibilityChanged:)
            name:NSWindowDidDeminiaturizeNotification object:window];
        [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(_windowVisibilityChanged:) 
            name:@"NSWindowDidOrderOffScreenNotification" object:window];
        [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(_windowVisibilityChanged:) 
            name:@"_NSWindowDidBecomeVisible" object:window];
    }
}

- (void)removeWindowObservers
{
    NSWindow *window = [self window];
    if (window) {
        [[NSNotificationCenter defaultCenter] removeObserver:self
            name:WKWindowWillOrderOnScreenNotification() object:window];
        [[NSNotificationCenter defaultCenter] removeObserver:self
            name:WKWindowWillOrderOffScreenNotification() object:window];
        [[NSNotificationCenter defaultCenter] removeObserver:self
            name:windowDidChangeBackingPropertiesNotification object:window];
        [[NSNotificationCenter defaultCenter] removeObserver:self
            name:NSWindowDidChangeScreenNotification object:window];
        [[NSNotificationCenter defaultCenter] removeObserver:self
            name:NSWindowDidMiniaturizeNotification object:window];
        [[NSNotificationCenter defaultCenter] removeObserver:self
            name:NSWindowDidDeminiaturizeNotification object:window];
        [[NSNotificationCenter defaultCenter] removeObserver:self
            name:@"NSWindowDidOrderOffScreenNotification" object:window];
        [[NSNotificationCenter defaultCenter] removeObserver:self
            name:@"_NSWindowDidBecomeVisible" object:window];
    }
}

- (void)viewWillMoveToWindow:(NSWindow *)window
{
    // Don't do anything if the WebView isn't initialized.
    // This happens when decoding a WebView in a nib.
    // FIXME: What sets up the observer of NSWindowWillCloseNotification in this case?
    if (!_private || _private->closed)
        return;
    
    if ([self window] && [self window] != [self hostWindow])
        [[NSNotificationCenter defaultCenter] removeObserver:self name:NSWindowWillCloseNotification object:[self window]];

    if (window) {
        [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(_windowWillClose:) name:NSWindowWillCloseNotification object:window];
         
        // Ensure that we will receive the events that WebHTMLView (at least) needs.
        // The following are expensive enough that we don't want to call them over
        // and over, so do them when we move into a window.
        [window setAcceptsMouseMovedEvents:YES];
        WKSetNSWindowShouldPostEventNotifications(window, YES);
    } else {
        _private->page->setCanStartMedia(false);
        _private->page->willMoveOffscreen();
        _private->page->setIsInWindow(false);
    }
        
    if (window != [self window]) {
        [self removeWindowObservers];
        [self addWindowObserversForWindow:window];
    }
}

- (void)viewDidMoveToWindow
{
    // Don't do anything if we aren't initialized.  This happens
    // when decoding a WebView.  When WebViews are decoded their subviews
    // are created by initWithCoder: and so won't be normally
    // initialized.  The stub views are discarded by WebView.
    if (!_private || _private->closed)
        return;

    if ([self window]) {
        _private->page->setCanStartMedia(true);
        _private->page->didMoveOnscreen();
        _private->page->setIsInWindow(true);
    }
    
    _private->page->setDeviceScaleFactor([self _deviceScaleFactor]);

    [self _updateActiveState];
    [self _updateVisibilityState];
}

- (void)doWindowDidChangeScreen
{
    if (_private && _private->page)
        _private->page->chrome().windowScreenDidChange((PlatformDisplayID)[[[[[self window] screen] deviceDescription] objectForKey:@"NSScreenNumber"] intValue]);
}

- (void)_windowChangedKeyState
{
    [self _updateActiveState];
}

- (void)_windowWillOrderOnScreen:(NSNotification *)notification
{
    if (![self shouldUpdateWhileOffscreen])
        [self setNeedsDisplay:YES];

    // Send a change screen to make sure the initial displayID is set
    [self doWindowDidChangeScreen];

    if (_private && _private->page) {
        _private->page->resumeScriptedAnimations();
        _private->page->focusController()->setContainingWindowIsVisible(true);
    }
}

- (void)_windowDidChangeScreen:(NSNotification *)notification
{
    [self doWindowDidChangeScreen];
}

- (void)_windowWillOrderOffScreen:(NSNotification *)notification
{
    if (_private && _private->page) {
        _private->page->suspendScriptedAnimations();
        _private->page->focusController()->setContainingWindowIsVisible(false);
    }
}

- (void)_windowVisibilityChanged:(NSNotification *)notification
{
    [self _updateVisibilityState];
}

- (void)_windowWillClose:(NSNotification *)notification
{
    if ([self shouldCloseWithWindow] && ([self window] == [self hostWindow] || ([self window] && ![self hostWindow]) || (![self window] && [self hostWindow])))
        [self close];
}

- (void)_windowDidChangeBackingProperties:(NSNotification *)notification
{
    CGFloat oldBackingScaleFactor = [[notification.userInfo objectForKey:backingPropertyOldScaleFactorKey] doubleValue]; 
    CGFloat newBackingScaleFactor = [self _deviceScaleFactor];
    if (oldBackingScaleFactor == newBackingScaleFactor) 
        return; 

    _private->page->setDeviceScaleFactor(newBackingScaleFactor);
}

- (void)setPreferences:(WebPreferences *)prefs
{
    if (!prefs)
        prefs = [WebPreferences standardPreferences];

    if (_private->preferences == prefs)
        return;

    [prefs willAddToWebView];

    WebPreferences *oldPrefs = _private->preferences;

    [[NSNotificationCenter defaultCenter] removeObserver:self name:WebPreferencesChangedInternalNotification object:[self preferences]];
    [WebPreferences _removeReferenceForIdentifier:[oldPrefs identifier]];

    _private->preferences = [prefs retain];

    // After registering for the notification, post it so the WebCore settings update.
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(_preferencesChangedNotification:)
        name:WebPreferencesChangedInternalNotification object:[self preferences]];
    [self _preferencesChanged:[self preferences]];
    [[self preferences] _postPreferencesChangedAPINotification];

    [oldPrefs didRemoveFromWebView];
    [oldPrefs release];
}

- (WebPreferences *)preferences
{
    return _private->preferences;
}

- (void)setPreferencesIdentifier:(NSString *)anIdentifier
{
    if (!_private->closed && ![anIdentifier isEqual:[[self preferences] identifier]]) {
        WebPreferences *prefs = [[WebPreferences alloc] initWithIdentifier:anIdentifier];
        [self setPreferences:prefs];
        [prefs release];
    }
}

- (NSString *)preferencesIdentifier
{
    return [[self preferences] identifier];
}


- (void)setUIDelegate:delegate
{
    _private->UIDelegate = delegate;
    [_private->UIDelegateForwarder release];
    _private->UIDelegateForwarder = nil;
}

- (id)UIDelegate
{
    return _private->UIDelegate;
}

- (void)setResourceLoadDelegate: delegate
{
    _private->resourceProgressDelegate = delegate;
    [self _cacheResourceLoadDelegateImplementations];
}

- (id)resourceLoadDelegate
{
    return _private->resourceProgressDelegate;
}

- (void)setDownloadDelegate: delegate
{
    _private->downloadDelegate = delegate;
}


- (id)downloadDelegate
{
    return _private->downloadDelegate;
}

- (void)setPolicyDelegate:delegate
{
    _private->policyDelegate = delegate;
    [_private->policyDelegateForwarder release];
    _private->policyDelegateForwarder = nil;
}

- (id)policyDelegate
{
    return _private->policyDelegate;
}

- (void)setFrameLoadDelegate:delegate
{
    // <rdar://problem/6950660> - Due to some subtle WebKit changes - presumably to delegate callback behavior - we've
    // unconvered a latent bug in at least one WebKit app where the delegate wasn't properly retained by the app and
    // was dealloc'ed before being cleared.
    // This is an effort to keep such apps working for now.
    if ([self _needsFrameLoadDelegateRetainQuirk]) {
        [delegate retain];
        [_private->frameLoadDelegate release];
    }
    
    _private->frameLoadDelegate = delegate;
    [self _cacheFrameLoadDelegateImplementations];

#if ENABLE(ICONDATABASE)
    // If this delegate wants callbacks for icons, fire up the icon database.
    if (_private->frameLoadDelegateImplementations.didReceiveIconForFrameFunc)
        [WebIconDatabase sharedIconDatabase];
#endif
}

- (id)frameLoadDelegate
{
    return _private->frameLoadDelegate;
}

- (WebFrame *)mainFrame
{
    // This can be called in initialization, before _private has been set up (3465613)
    if (!_private || !_private->page)
        return nil;
    return kit(_private->page->mainFrame());
}

- (WebFrame *)selectedFrame
{
    // If the first responder is a view in our tree, we get the frame containing the first responder.
    // This is faster than searching the frame hierarchy, and will give us a result even in the case
    // where the focused frame doesn't actually contain a selection.
    WebFrame *focusedFrame = [self _focusedFrame];
    if (focusedFrame)
        return focusedFrame;
    
    // If the first responder is outside of our view tree, we search for a frame containing a selection.
    // There should be at most only one of these.
    return [[self mainFrame] _findFrameWithSelection];
}

- (WebBackForwardList *)backForwardList
{
    if (!_private->page)
        return nil;
    BackForwardListImpl* list = static_cast<BackForwardListImpl*>(_private->page->backForwardList());
    if (!list->enabled())
        return nil;
    return kit(list);
}

- (void)setMaintainsBackForwardList:(BOOL)flag
{
    if (!_private->page)
        return;
    static_cast<BackForwardListImpl*>(_private->page->backForwardList())->setEnabled(flag);
}

- (BOOL)goBack
{
    if (!_private->page)
        return NO;
    
    return _private->page->goBack();
}

- (BOOL)goForward
{
    if (!_private->page)
        return NO;

    return _private->page->goForward();
}

- (BOOL)goToBackForwardItem:(WebHistoryItem *)item
{
    if (!_private->page)
        return NO;

    _private->page->goToItem(core(item), FrameLoadTypeIndexedBackForward);
    return YES;
}

- (void)setTextSizeMultiplier:(float)m
{
    [self _setZoomMultiplier:m isTextOnly:![[NSUserDefaults standardUserDefaults] boolForKey:WebKitDebugFullPageZoomPreferenceKey]];
}

- (float)textSizeMultiplier
{
    return [self _realZoomMultiplierIsTextOnly] ? _private->zoomMultiplier : 1.0f;
}

- (void)_setZoomMultiplier:(float)multiplier isTextOnly:(BOOL)isTextOnly
{
    // NOTE: This has no visible effect when viewing a PDF (see <rdar://problem/4737380>)
    _private->zoomMultiplier = multiplier;
    _private->zoomsTextOnly = isTextOnly;

    // FIXME: It might be nice to rework this code so that _private->zoomMultiplier doesn't exist
    // and instead the zoom factors stored in Frame are used.
    Frame* coreFrame = [self _mainCoreFrame];
    if (coreFrame) {
        if (_private->zoomsTextOnly)
            coreFrame->setPageAndTextZoomFactors(1, multiplier);
        else
            coreFrame->setPageAndTextZoomFactors(multiplier, 1);
    }
}

- (float)_zoomMultiplier:(BOOL)isTextOnly
{
    if (isTextOnly != [self _realZoomMultiplierIsTextOnly])
        return 1.0f;
    return _private->zoomMultiplier;
}

- (float)_realZoomMultiplier
{
    return _private->zoomMultiplier;
}

- (BOOL)_realZoomMultiplierIsTextOnly
{
    if (!_private->page)
        return NO;
    
    return _private->zoomsTextOnly;
}

#define MinimumZoomMultiplier       0.5f
#define MaximumZoomMultiplier       3.0f
#define ZoomMultiplierRatio         1.2f

- (BOOL)_canZoomOut:(BOOL)isTextOnly
{
    id docView = [[[self mainFrame] frameView] documentView];
    if ([docView conformsToProtocol:@protocol(_WebDocumentZooming)]) {
        id <_WebDocumentZooming> zoomingDocView = (id <_WebDocumentZooming>)docView;
        return [zoomingDocView _canZoomOut];
    }
    return [self _zoomMultiplier:isTextOnly] / ZoomMultiplierRatio > MinimumZoomMultiplier;
}


- (BOOL)_canZoomIn:(BOOL)isTextOnly
{
    id docView = [[[self mainFrame] frameView] documentView];
    if ([docView conformsToProtocol:@protocol(_WebDocumentZooming)]) {
        id <_WebDocumentZooming> zoomingDocView = (id <_WebDocumentZooming>)docView;
        return [zoomingDocView _canZoomIn];
    }
    return [self _zoomMultiplier:isTextOnly] * ZoomMultiplierRatio < MaximumZoomMultiplier;
}

- (IBAction)_zoomOut:(id)sender isTextOnly:(BOOL)isTextOnly
{
    id docView = [[[self mainFrame] frameView] documentView];
    if ([docView conformsToProtocol:@protocol(_WebDocumentZooming)]) {
        id <_WebDocumentZooming> zoomingDocView = (id <_WebDocumentZooming>)docView;
        return [zoomingDocView _zoomOut:sender];
    }
    float newScale = [self _zoomMultiplier:isTextOnly] / ZoomMultiplierRatio;
    if (newScale > MinimumZoomMultiplier)
        [self _setZoomMultiplier:newScale isTextOnly:isTextOnly];
}

- (IBAction)_zoomIn:(id)sender isTextOnly:(BOOL)isTextOnly
{
    id docView = [[[self mainFrame] frameView] documentView];
    if ([docView conformsToProtocol:@protocol(_WebDocumentZooming)]) {
        id <_WebDocumentZooming> zoomingDocView = (id <_WebDocumentZooming>)docView;
        return [zoomingDocView _zoomIn:sender];
    }
    float newScale = [self _zoomMultiplier:isTextOnly] * ZoomMultiplierRatio;
    if (newScale < MaximumZoomMultiplier)
        [self _setZoomMultiplier:newScale isTextOnly:isTextOnly];
}

- (BOOL)_canResetZoom:(BOOL)isTextOnly
{
    id docView = [[[self mainFrame] frameView] documentView];
    if ([docView conformsToProtocol:@protocol(_WebDocumentZooming)]) {
        id <_WebDocumentZooming> zoomingDocView = (id <_WebDocumentZooming>)docView;
        return [zoomingDocView _canResetZoom];
    }
    return [self _zoomMultiplier:isTextOnly] != 1.0f;
}

- (IBAction)_resetZoom:(id)sender isTextOnly:(BOOL)isTextOnly
{
    id docView = [[[self mainFrame] frameView] documentView];
    if ([docView conformsToProtocol:@protocol(_WebDocumentZooming)]) {
        id <_WebDocumentZooming> zoomingDocView = (id <_WebDocumentZooming>)docView;
        return [zoomingDocView _resetZoom:sender];
    }
    if ([self _zoomMultiplier:isTextOnly] != 1.0f)
        [self _setZoomMultiplier:1.0f isTextOnly:isTextOnly];
}

- (void)setApplicationNameForUserAgent:(NSString *)applicationName
{
    NSString *name = [applicationName copy];
    [_private->applicationNameForUserAgent release];
    _private->applicationNameForUserAgent = name;
    if (!_private->userAgentOverridden)
        _private->userAgent = String();
}

- (NSString *)applicationNameForUserAgent
{
    return [[_private->applicationNameForUserAgent retain] autorelease];
}

- (void)setCustomUserAgent:(NSString *)userAgentString
{
    _private->userAgent = userAgentString;
    _private->userAgentOverridden = userAgentString != nil;
}

- (NSString *)customUserAgent
{
    if (!_private->userAgentOverridden)
        return nil;
    return _private->userAgent;
}

- (void)setMediaStyle:(NSString *)mediaStyle
{
    if (_private->mediaStyle != mediaStyle) {
        [_private->mediaStyle release];
        _private->mediaStyle = [mediaStyle copy];
    }
}

- (NSString *)mediaStyle
{
    return _private->mediaStyle;
}

- (BOOL)supportsTextEncoding
{
    id documentView = [[[self mainFrame] frameView] documentView];
    return [documentView conformsToProtocol:@protocol(WebDocumentText)]
        && [documentView supportsTextEncoding];
}

- (void)setCustomTextEncodingName:(NSString *)encoding
{
    NSString *oldEncoding = [self customTextEncodingName];
    if (encoding == oldEncoding || [encoding isEqualToString:oldEncoding])
        return;
    if (Frame* mainFrame = [self _mainCoreFrame])
        mainFrame->loader()->reloadWithOverrideEncoding(encoding);
}

- (NSString *)_mainFrameOverrideEncoding
{
    WebDataSource *dataSource = [[self mainFrame] provisionalDataSource];
    if (dataSource == nil)
        dataSource = [[self mainFrame] _dataSource];
    if (dataSource == nil)
        return nil;
    return nsStringNilIfEmpty([dataSource _documentLoader]->overrideEncoding());
}

- (NSString *)customTextEncodingName
{
    return [self _mainFrameOverrideEncoding];
}

- (NSString *)stringByEvaluatingJavaScriptFromString:(NSString *)script
{
    // Return statements are only valid in a function but some applications pass in scripts
    // prefixed with return (<rdar://problems/5103720&4616860>) since older WebKit versions
    // silently ignored the return. If the application is linked against an earlier version
    // of WebKit we will strip the return so the script wont fail.
    if (!WebKitLinkedOnOrAfter(WEBKIT_FIRST_VERSION_WITHOUT_JAVASCRIPT_RETURN_QUIRK)) {
        NSRange returnStringRange = [script rangeOfString:@"return "];
        if (returnStringRange.length && !returnStringRange.location)
            script = [script substringFromIndex:returnStringRange.location + returnStringRange.length];
    }

    NSString *result = [[self mainFrame] _stringByEvaluatingJavaScriptFromString:script];
    // The only way stringByEvaluatingJavaScriptFromString can return nil is if the frame was removed by the script
    // Since there's no way to get rid of the main frame, result will never ever be nil here.
    ASSERT(result);

    return result;
}

- (WebScriptObject *)windowScriptObject
{
    Frame* coreFrame = [self _mainCoreFrame];
    if (!coreFrame)
        return nil;
    return coreFrame->script()->windowScriptObject();
}

- (String)_userAgentString
{
    if (_private->userAgent.isNull())
        _private->userAgent = [[self class] _standardUserAgentWithApplicationName:_private->applicationNameForUserAgent];

    return _private->userAgent;
}

// Get the appropriate user-agent string for a particular URL.
- (NSString *)userAgentForURL:(NSURL *)url
{
    return [self _userAgentString];
}

- (void)setHostWindow:(NSWindow *)hostWindow
{
    if (_private->closed && hostWindow)
        return;
    if (hostWindow == _private->hostWindow)
        return;

    Frame* coreFrame = [self _mainCoreFrame];
    for (Frame* frame = coreFrame; frame; frame = frame->tree()->traverseNext(coreFrame))
        [[[kit(frame) frameView] documentView] viewWillMoveToHostWindow:hostWindow];
    if (_private->hostWindow && [self window] != _private->hostWindow)
        [[NSNotificationCenter defaultCenter] removeObserver:self name:NSWindowWillCloseNotification object:_private->hostWindow];
    if (hostWindow)
        [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(_windowWillClose:) name:NSWindowWillCloseNotification object:hostWindow];
    [_private->hostWindow release];
    _private->hostWindow = [hostWindow retain];
    for (Frame* frame = coreFrame; frame; frame = frame->tree()->traverseNext(coreFrame))
        [[[kit(frame) frameView] documentView] viewDidMoveToHostWindow];
    _private->page->setDeviceScaleFactor([self _deviceScaleFactor]);
}

- (NSWindow *)hostWindow
{
    // -[WebView hostWindow] can sometimes be called from the WebView's [super dealloc] method
    // so we check here to make sure it's not null.
    if (!_private)
        return nil;
    
    return _private->hostWindow;
}

- (NSView <WebDocumentView> *)documentViewAtWindowPoint:(NSPoint)point
{
    return [[self _frameViewAtWindowPoint:point] documentView];
}

- (NSDictionary *)_elementAtWindowPoint:(NSPoint)windowPoint
{
    WebFrameView *frameView = [self _frameViewAtWindowPoint:windowPoint];
    if (!frameView)
        return nil;
    NSView <WebDocumentView> *documentView = [frameView documentView];
    if ([documentView conformsToProtocol:@protocol(WebDocumentElement)]) {
        NSPoint point = [documentView convertPoint:windowPoint fromView:nil];
        return [(NSView <WebDocumentElement> *)documentView elementAtPoint:point];
    }
    return [NSDictionary dictionaryWithObject:[frameView webFrame] forKey:WebElementFrameKey];
}

- (NSDictionary *)elementAtPoint:(NSPoint)point
{
    return [self _elementAtWindowPoint:[self convertPoint:point toView:nil]];
}

#if ENABLE(DRAG_SUPPORT)
// The following 2 internal NSView methods are called on the drag destination to make scrolling while dragging work.
// Scrolling while dragging will only work if the drag destination is in a scroll view. The WebView is the drag destination. 
// When dragging to a WebView, the document subview should scroll, but it doesn't because it is not the drag destination. 
// Forward these calls to the document subview to make its scroll view scroll.
- (void)_autoscrollForDraggingInfo:(id)draggingInfo timeDelta:(NSTimeInterval)repeatDelta
{
    NSView <WebDocumentView> *documentView = [self documentViewAtWindowPoint:[draggingInfo draggingLocation]];
    [documentView _autoscrollForDraggingInfo:draggingInfo timeDelta:repeatDelta];
}

- (BOOL)_shouldAutoscrollForDraggingInfo:(id)draggingInfo
{
    NSView <WebDocumentView> *documentView = [self documentViewAtWindowPoint:[draggingInfo draggingLocation]];
    return [documentView _shouldAutoscrollForDraggingInfo:draggingInfo];
}

- (DragApplicationFlags)applicationFlags:(id <NSDraggingInfo>)draggingInfo
{
    uint32_t flags = 0;
    if ([NSApp modalWindow])
        flags = DragApplicationIsModal;
    if ([[self window] attachedSheet])
        flags |= DragApplicationHasAttachedSheet;
    if ([draggingInfo draggingSource] == self)
        flags |= DragApplicationIsSource;
    if ([[NSApp currentEvent] modifierFlags] & NSAlternateKeyMask)
        flags |= DragApplicationIsCopyKeyDown;
    return static_cast<DragApplicationFlags>(flags);
}

- (NSDragOperation)draggingEntered:(id <NSDraggingInfo>)draggingInfo
{
    IntPoint client([draggingInfo draggingLocation]);
    IntPoint global(globalPoint([draggingInfo draggingLocation], [self window]));
    DragData dragData(draggingInfo, client, global, static_cast<DragOperation>([draggingInfo draggingSourceOperationMask]), [self applicationFlags:draggingInfo]);
    return core(self)->dragController()->dragEntered(&dragData).operation;
}

- (NSDragOperation)draggingUpdated:(id <NSDraggingInfo>)draggingInfo
{
    Page* page = core(self);
    if (!page)
        return NSDragOperationNone;

    IntPoint client([draggingInfo draggingLocation]);
    IntPoint global(globalPoint([draggingInfo draggingLocation], [self window]));
    DragData dragData(draggingInfo, client, global, static_cast<DragOperation>([draggingInfo draggingSourceOperationMask]), [self applicationFlags:draggingInfo]);
    return page->dragController()->dragUpdated(&dragData).operation;
}

- (void)draggingExited:(id <NSDraggingInfo>)draggingInfo
{
    Page* page = core(self);
    if (!page)
        return;

    IntPoint client([draggingInfo draggingLocation]);
    IntPoint global(globalPoint([draggingInfo draggingLocation], [self window]));
    DragData dragData(draggingInfo, client, global, static_cast<DragOperation>([draggingInfo draggingSourceOperationMask]), [self applicationFlags:draggingInfo]);
    page->dragController()->dragExited(&dragData);
}

- (BOOL)prepareForDragOperation:(id <NSDraggingInfo>)draggingInfo
{
    return YES;
}

- (BOOL)performDragOperation:(id <NSDraggingInfo>)draggingInfo
{
    IntPoint client([draggingInfo draggingLocation]);
    IntPoint global(globalPoint([draggingInfo draggingLocation], [self window]));
    DragData dragData(draggingInfo, client, global, static_cast<DragOperation>([draggingInfo draggingSourceOperationMask]), [self applicationFlags:draggingInfo]);
    return core(self)->dragController()->performDrag(&dragData);
}

- (NSView *)_hitTest:(NSPoint *)point dragTypes:(NSSet *)types
{
    NSView *hitView = [super _hitTest:point dragTypes:types];
    if (!hitView && [[self superview] mouse:*point inRect:[self frame]])
        return self;
    return hitView;
}
#endif

- (BOOL)acceptsFirstResponder
{
    return [[[self mainFrame] frameView] acceptsFirstResponder];
}

- (BOOL)becomeFirstResponder
{
    if (_private->becomingFirstResponder) {
        // Fix for unrepro infinite recursion reported in Radar 4448181. If we hit this assert on
        // a debug build, we should figure out what causes the problem and do a better fix.
        ASSERT_NOT_REACHED();
        return NO;
    }
    
    // This works together with setNextKeyView to splice the WebView into
    // the key loop similar to the way NSScrollView does this. Note that
    // WebFrameView has very similar code.
    NSWindow *window = [self window];
    WebFrameView *mainFrameView = [[self mainFrame] frameView];

    NSResponder *previousFirstResponder = [[self window] _oldFirstResponderBeforeBecoming];
    BOOL fromOutside = ![previousFirstResponder isKindOfClass:[NSView class]] || (![(NSView *)previousFirstResponder isDescendantOf:self] && previousFirstResponder != self);

    if ([window keyViewSelectionDirection] == NSSelectingPrevious) {
        NSView *previousValidKeyView = [self previousValidKeyView];
        if (previousValidKeyView != self && previousValidKeyView != mainFrameView) {
            _private->becomingFirstResponder = YES;
            _private->becomingFirstResponderFromOutside = fromOutside;
            [window makeFirstResponder:previousValidKeyView];
            _private->becomingFirstResponderFromOutside = NO;
            _private->becomingFirstResponder = NO;
            return YES;
        }
        return NO;
    }

    if ([mainFrameView acceptsFirstResponder]) {
        _private->becomingFirstResponder = YES;
        _private->becomingFirstResponderFromOutside = fromOutside;
        [window makeFirstResponder:mainFrameView];
        _private->becomingFirstResponderFromOutside = NO;
        _private->becomingFirstResponder = NO;
        return YES;
    } 

    return NO;
}

- (NSView *)_webcore_effectiveFirstResponder
{
    if (WebFrameView *frameView = [[self mainFrame] frameView])
        return [frameView _webcore_effectiveFirstResponder];

    return [super _webcore_effectiveFirstResponder];
}

- (void)setNextKeyView:(NSView *)view
{
    // This works together with becomeFirstResponder to splice the WebView into
    // the key loop similar to the way NSScrollView does this. Note that
    // WebFrameView has similar code.
    if (WebFrameView *mainFrameView = [[self mainFrame] frameView]) {
        [mainFrameView setNextKeyView:view];
        return;
    }

    [super setNextKeyView:view];
}

static WebFrame *incrementFrame(WebFrame *frame, WebFindOptions options = 0)
{
    Frame* coreFrame = core(frame);
    return kit((options & WebFindOptionsBackwards)
        ? coreFrame->tree()->traversePreviousWithWrap(options & WebFindOptionsWrapAround)
        : coreFrame->tree()->traverseNextWithWrap(options & WebFindOptionsWrapAround));
}

- (BOOL)searchFor:(NSString *)string direction:(BOOL)forward caseSensitive:(BOOL)caseFlag wrap:(BOOL)wrapFlag
{
    return [self searchFor:string direction:forward caseSensitive:caseFlag wrap:wrapFlag startInSelection:NO];
}

+ (void)registerViewClass:(Class)viewClass representationClass:(Class)representationClass forMIMEType:(NSString *)MIMEType
{
    [[WebFrameView _viewTypesAllowImageTypeOmission:YES] setObject:viewClass forKey:MIMEType];
    [[WebDataSource _repTypesAllowImageTypeOmission:YES] setObject:representationClass forKey:MIMEType];
    
    // FIXME: We also need to maintain MIMEType registrations (which can be dynamically changed)
    // in the WebCore MIMEType registry.  For now we're doing this in a safe, limited manner
    // to fix <rdar://problem/5372989> - a future revamping of the entire system is neccesary for future robustness
    if ([viewClass class] == [WebHTMLView class])
        MIMETypeRegistry::getSupportedNonImageMIMETypes().add(MIMEType);
}

- (void)setGroupName:(NSString *)groupName
{
    if (!_private->page)
        return;
    _private->page->setGroupName(groupName);
}

- (NSString *)groupName
{
    if (!_private->page)
        return nil;
    return _private->page->groupName();
}

- (double)estimatedProgress
{
    if (!_private->page)
        return 0.0;
    return _private->page->progress()->estimatedProgress();
}

- (NSArray *)pasteboardTypesForSelection
{
    NSView <WebDocumentView> *documentView = [[[self _selectedOrMainFrame] frameView] documentView];
    if ([documentView conformsToProtocol:@protocol(WebDocumentSelection)]) {
        return [(NSView <WebDocumentSelection> *)documentView pasteboardTypesForSelection];
    }
    return [NSArray array];
}

- (void)writeSelectionWithPasteboardTypes:(NSArray *)types toPasteboard:(NSPasteboard *)pasteboard
{
    WebFrame *frame = [self _selectedOrMainFrame];
    if (frame && [frame _hasSelection]) {
        NSView <WebDocumentView> *documentView = [[frame frameView] documentView];
        if ([documentView conformsToProtocol:@protocol(WebDocumentSelection)])
            [(NSView <WebDocumentSelection> *)documentView writeSelectionWithPasteboardTypes:types toPasteboard:pasteboard];
    }
}

- (NSArray *)pasteboardTypesForElement:(NSDictionary *)element
{
    if ([element objectForKey:WebElementImageURLKey] != nil) {
        return [NSPasteboard _web_writableTypesForImageIncludingArchive:([element objectForKey:WebElementDOMNodeKey] != nil)];
    } else if ([element objectForKey:WebElementLinkURLKey] != nil) {
        return [NSPasteboard _web_writableTypesForURL];
    } else if ([[element objectForKey:WebElementIsSelectedKey] boolValue]) {
        return [self pasteboardTypesForSelection];
    }
    return [NSArray array];
}

- (void)writeElement:(NSDictionary *)element withPasteboardTypes:(NSArray *)types toPasteboard:(NSPasteboard *)pasteboard
{
    if ([element objectForKey:WebElementImageURLKey] != nil) {
        [self _writeImageForElement:element withPasteboardTypes:types toPasteboard:pasteboard];
    } else if ([element objectForKey:WebElementLinkURLKey] != nil) {
        [self _writeLinkElement:element withPasteboardTypes:types toPasteboard:pasteboard];
    } else if ([[element objectForKey:WebElementIsSelectedKey] boolValue]) {
        [self writeSelectionWithPasteboardTypes:types toPasteboard:pasteboard];
    }
}

- (void)moveDragCaretToPoint:(NSPoint)point
{
#if ENABLE(DRAG_SUPPORT)
    if (Page* page = core(self))
        page->dragController()->placeDragCaret(IntPoint([self convertPoint:point toView:nil]));
#endif
}

- (void)removeDragCaret
{
#if ENABLE(DRAG_SUPPORT)
    if (Page* page = core(self))
        page->dragController()->dragEnded();
#endif
}

- (void)setMainFrameURL:(NSString *)URLString
{
    NSURL *url;
    if ([URLString hasPrefix:@"/"])
        url = [NSURL fileURLWithPath:URLString];
    else
        url = [NSURL _web_URLWithDataAsString:URLString];

    [[self mainFrame] loadRequest:[NSURLRequest requestWithURL:url]];
}

- (NSString *)mainFrameURL
{
    WebDataSource *ds;
    ds = [[self mainFrame] provisionalDataSource];
    if (!ds)
        ds = [[self mainFrame] _dataSource];
    return [[[ds request] URL] _web_originalDataAsString];
}

- (BOOL)isLoading
{
    LOG (Bindings, "isLoading = %d", (int)[self _isLoading]);
    return [self _isLoading];
}

- (NSString *)mainFrameTitle
{
    NSString *mainFrameTitle = [[[self mainFrame] _dataSource] pageTitle];
    return (mainFrameTitle != nil) ? mainFrameTitle : (NSString *)@"";
}

- (NSImage *)mainFrameIcon
{
    return [[WebIconDatabase sharedIconDatabase] iconForURL:[[[[self mainFrame] _dataSource] _URL] _web_originalDataAsString] withSize:WebIconSmallSize];
}

- (DOMDocument *)mainFrameDocument
{
    // only return the actual value if the state we're in gives NSTreeController
    // enough time to release its observers on the old model
    if (_private->mainFrameDocumentReady)
        return [[self mainFrame] DOMDocument];
    return nil;
}

- (void)setDrawsBackground:(BOOL)drawsBackground
{
    if (_private->drawsBackground == drawsBackground)
        return;
    _private->drawsBackground = drawsBackground;
    [[self mainFrame] _updateBackgroundAndUpdatesWhileOffscreen];
}

- (BOOL)drawsBackground
{
    // This method can be called beneath -[NSView dealloc] after we have cleared _private,
    // indirectly via -[WebFrameView viewDidMoveToWindow].
    return !_private || _private->drawsBackground;
}

- (void)setShouldUpdateWhileOffscreen:(BOOL)updateWhileOffscreen
{
    if (_private->shouldUpdateWhileOffscreen == updateWhileOffscreen)
        return;
    _private->shouldUpdateWhileOffscreen = updateWhileOffscreen;
    [[self mainFrame] _updateBackgroundAndUpdatesWhileOffscreen];
}

- (BOOL)shouldUpdateWhileOffscreen
{
    return _private->shouldUpdateWhileOffscreen;
}

- (void)setCurrentNodeHighlight:(WebNodeHighlight *)nodeHighlight
{
    id old = _private->currentNodeHighlight;
    _private->currentNodeHighlight = [nodeHighlight retain];
    [old release];
}

- (WebNodeHighlight *)currentNodeHighlight
{
    return _private->currentNodeHighlight;
}

- (NSView *)previousValidKeyView
{
    NSView *result = [super previousValidKeyView];

    // Work around AppKit bug 6905484. If the result is a view that's inside this one, it's
    // possible it is the wrong answer, because the fact that it's a descendant causes the
    // code that implements key view redirection to fail; this means we won't redirect to
    // the toolbar, for example, when we hit the edge of a window. Since the bug is specific
    // to cases where the receiver of previousValidKeyView is an ancestor of the last valid
    // key view in the loop, we can sidestep it by walking along previous key views until
    // we find one that is not a superview, then using that to call previousValidKeyView.

    if (![result isDescendantOf:self])
        return result;

    // Use a visited set so we don't loop indefinitely when walking crazy key loops.
    // AppKit uses such sets internally and we want our loop to be as robust as its loops.
    RetainPtr<CFMutableSetRef> visitedViews = adoptCF(CFSetCreateMutable(0, 0, 0));
    CFSetAddValue(visitedViews.get(), result);

    NSView *previousView = self;
    do {
        CFSetAddValue(visitedViews.get(), previousView);
        previousView = [previousView previousKeyView];
        if (!previousView || CFSetGetValue(visitedViews.get(), previousView))
            return result;
    } while ([result isDescendantOf:previousView]);
    return [previousView previousValidKeyView];
}

@end

@implementation WebView (WebIBActions)

- (IBAction)takeStringURLFrom: sender
{
    NSString *URLString = [sender stringValue];
    
    [[self mainFrame] loadRequest: [NSURLRequest requestWithURL: [NSURL _web_URLWithDataAsString: URLString]]];
}

- (BOOL)canGoBack
{
    if (!_private->page || _private->page->defersLoading())
        return NO;

    return !!_private->page->backForwardList()->backItem();
}

- (BOOL)canGoForward
{
    if (!_private->page || _private->page->defersLoading())
        return NO;

    return !!_private->page->backForwardList()->forwardItem();
}

- (IBAction)goBack:(id)sender
{
    [self goBack];
}

- (IBAction)goForward:(id)sender
{
    [self goForward];
}

- (IBAction)stopLoading:(id)sender
{
    [[self mainFrame] stopLoading];
}

- (IBAction)reload:(id)sender
{
    [[self mainFrame] reload];
}

- (IBAction)reloadFromOrigin:(id)sender
{
    [[self mainFrame] reloadFromOrigin];
}

// FIXME: This code should move into WebCore so that it is not duplicated in each WebKit.
// (This includes canMakeTextSmaller/Larger, makeTextSmaller/Larger, and canMakeTextStandardSize/makeTextStandardSize)
- (BOOL)canMakeTextSmaller
{
    return [self _canZoomOut:![[NSUserDefaults standardUserDefaults] boolForKey:WebKitDebugFullPageZoomPreferenceKey]];
}

- (IBAction)makeTextSmaller:(id)sender
{
    return [self _zoomOut:sender isTextOnly:![[NSUserDefaults standardUserDefaults] boolForKey:WebKitDebugFullPageZoomPreferenceKey]];
}

- (BOOL)canMakeTextLarger
{
    return [self _canZoomIn:![[NSUserDefaults standardUserDefaults] boolForKey:WebKitDebugFullPageZoomPreferenceKey]];
}

- (IBAction)makeTextLarger:(id)sender
{
    return [self _zoomIn:sender isTextOnly:![[NSUserDefaults standardUserDefaults] boolForKey:WebKitDebugFullPageZoomPreferenceKey]];
}

- (BOOL)canMakeTextStandardSize
{
    return [self _canResetZoom:![[NSUserDefaults standardUserDefaults] boolForKey:WebKitDebugFullPageZoomPreferenceKey]];
}

- (IBAction)makeTextStandardSize:(id)sender
{
   return [self _resetZoom:sender isTextOnly:![[NSUserDefaults standardUserDefaults] boolForKey:WebKitDebugFullPageZoomPreferenceKey]];
}

- (IBAction)toggleSmartInsertDelete:(id)sender
{
    [self setSmartInsertDeleteEnabled:![self smartInsertDeleteEnabled]];
}

- (IBAction)toggleContinuousSpellChecking:(id)sender
{
    [self setContinuousSpellCheckingEnabled:![self isContinuousSpellCheckingEnabled]];
}

- (BOOL)_responderValidateUserInterfaceItem:(id <NSValidatedUserInterfaceItem>)item
{
    id responder = [self _responderForResponderOperations];
    if (responder != self && [responder respondsToSelector:[item action]]) {
        if ([responder respondsToSelector:@selector(validateUserInterfaceItemWithoutDelegate:)])
            return [responder validateUserInterfaceItemWithoutDelegate:item];
        if ([responder respondsToSelector:@selector(validateUserInterfaceItem:)])
            return [responder validateUserInterfaceItem:item];
        return YES;
    }
    return NO;
}

#define VALIDATE(name) \
    else if (action == @selector(name:)) { return [self _responderValidateUserInterfaceItem:item]; }

- (BOOL)validateUserInterfaceItemWithoutDelegate:(id <NSValidatedUserInterfaceItem>)item
{
    SEL action = [item action];

    if (action == @selector(goBack:)) {
        return [self canGoBack];
    } else if (action == @selector(goForward:)) {
        return [self canGoForward];
    } else if (action == @selector(makeTextLarger:)) {
        return [self canMakeTextLarger];
    } else if (action == @selector(makeTextSmaller:)) {
        return [self canMakeTextSmaller];
    } else if (action == @selector(makeTextStandardSize:)) {
        return [self canMakeTextStandardSize];
    } else if (action == @selector(reload:)) {
        return [[self mainFrame] _dataSource] != nil;
    } else if (action == @selector(stopLoading:)) {
        return [self _isLoading];
    } else if (action == @selector(toggleContinuousSpellChecking:)) {
        BOOL checkMark = NO;
        BOOL retVal = NO;
        if ([self _continuousCheckingAllowed]) {
            checkMark = [self isContinuousSpellCheckingEnabled];
            retVal = YES;
        }
        if ([(NSObject *)item isKindOfClass:[NSMenuItem class]]) {
            NSMenuItem *menuItem = (NSMenuItem *)item;
            [menuItem setState:checkMark ? NSOnState : NSOffState];
        }
        return retVal;
    } else if (action == @selector(toggleSmartInsertDelete:)) {
        BOOL checkMark = [self smartInsertDeleteEnabled];
        if ([(NSObject *)item isKindOfClass:[NSMenuItem class]]) {
            NSMenuItem *menuItem = (NSMenuItem *)item;
            [menuItem setState:checkMark ? NSOnState : NSOffState];
        }
        return YES;
    } else if (action == @selector(toggleGrammarChecking:)) {
        BOOL checkMark = [self isGrammarCheckingEnabled];
        if ([(NSObject *)item isKindOfClass:[NSMenuItem class]]) {
            NSMenuItem *menuItem = (NSMenuItem *)item;
            [menuItem setState:checkMark ? NSOnState : NSOffState];
        }
        return YES;
    } else if (action == @selector(toggleAutomaticQuoteSubstitution:)) {
        BOOL checkMark = [self isAutomaticQuoteSubstitutionEnabled];
        if ([(NSObject *)item isKindOfClass:[NSMenuItem class]]) {
            NSMenuItem *menuItem = (NSMenuItem *)item;
            [menuItem setState:checkMark ? NSOnState : NSOffState];
        }
        return YES;
    } else if (action == @selector(toggleAutomaticLinkDetection:)) {
        BOOL checkMark = [self isAutomaticLinkDetectionEnabled];
        if ([(NSObject *)item isKindOfClass:[NSMenuItem class]]) {
            NSMenuItem *menuItem = (NSMenuItem *)item;
            [menuItem setState:checkMark ? NSOnState : NSOffState];
        }
        return YES;
    } else if (action == @selector(toggleAutomaticDashSubstitution:)) {
        BOOL checkMark = [self isAutomaticDashSubstitutionEnabled];
        if ([(NSObject *)item isKindOfClass:[NSMenuItem class]]) {
            NSMenuItem *menuItem = (NSMenuItem *)item;
            [menuItem setState:checkMark ? NSOnState : NSOffState];
        }
        return YES;
    } else if (action == @selector(toggleAutomaticTextReplacement:)) {
        BOOL checkMark = [self isAutomaticTextReplacementEnabled];
        if ([(NSObject *)item isKindOfClass:[NSMenuItem class]]) {
            NSMenuItem *menuItem = (NSMenuItem *)item;
            [menuItem setState:checkMark ? NSOnState : NSOffState];
        }
        return YES;
    } else if (action == @selector(toggleAutomaticSpellingCorrection:)) {
        BOOL checkMark = [self isAutomaticSpellingCorrectionEnabled];
        if ([(NSObject *)item isKindOfClass:[NSMenuItem class]]) {
            NSMenuItem *menuItem = (NSMenuItem *)item;
            [menuItem setState:checkMark ? NSOnState : NSOffState];
        }
        return YES;
    }
    FOR_EACH_RESPONDER_SELECTOR(VALIDATE)

    return YES;
}

- (BOOL)validateUserInterfaceItem:(id <NSValidatedUserInterfaceItem>)item
{
    BOOL result = [self validateUserInterfaceItemWithoutDelegate:item];
    return CallUIDelegateReturningBoolean(result, self, @selector(webView:validateUserInterfaceItem:defaultValidation:), item, result);
}

@end

@implementation WebView (WebPendingPublic)

- (void)scheduleInRunLoop:(NSRunLoop *)runLoop forMode:(NSString *)mode
{
#if USE(CFNETWORK)
    CFRunLoopRef schedulePairRunLoop = [runLoop getCFRunLoop];
#else
    NSRunLoop *schedulePairRunLoop = runLoop;
#endif
    if (runLoop && mode)
        core(self)->addSchedulePair(SchedulePair::create(schedulePairRunLoop, (CFStringRef)mode));
}

- (void)unscheduleFromRunLoop:(NSRunLoop *)runLoop forMode:(NSString *)mode
{
#if USE(CFNETWORK)
    CFRunLoopRef schedulePairRunLoop = [runLoop getCFRunLoop];
#else
    NSRunLoop *schedulePairRunLoop = runLoop;
#endif
    if (runLoop && mode)
        core(self)->removeSchedulePair(SchedulePair::create(schedulePairRunLoop, (CFStringRef)mode));
}

static BOOL findString(NSView <WebDocumentSearching> *searchView, NSString *string, WebFindOptions options)
{
    if ([searchView conformsToProtocol:@protocol(WebDocumentOptionsSearching)])
        return [(NSView <WebDocumentOptionsSearching> *)searchView _findString:string options:options];
    if ([searchView conformsToProtocol:@protocol(WebDocumentIncrementalSearching)])
        return [(NSView <WebDocumentIncrementalSearching> *)searchView searchFor:string direction:!(options & WebFindOptionsBackwards) caseSensitive:!(options & WebFindOptionsCaseInsensitive) wrap:!!(options & WebFindOptionsWrapAround) startInSelection:!!(options & WebFindOptionsStartInSelection)];
    return [searchView searchFor:string direction:!(options & WebFindOptionsBackwards) caseSensitive:!(options & WebFindOptionsCaseInsensitive) wrap:!!(options & WebFindOptionsWrapAround)];
}

- (BOOL)findString:(NSString *)string options:(WebFindOptions)options
{
    if (_private->closed)
        return NO;
    
    // Get the frame holding the selection, or start with the main frame
    WebFrame *startFrame = [self _selectedOrMainFrame];
    
    // Search the first frame, then all the other frames, in order
    NSView <WebDocumentSearching> *startSearchView = nil;
    WebFrame *frame = startFrame;
    do {
        WebFrame *nextFrame = incrementFrame(frame, options);
        
        BOOL onlyOneFrame = (frame == nextFrame);
        ASSERT(!onlyOneFrame || frame == startFrame);
        
        id <WebDocumentView> view = [[frame frameView] documentView];
        if ([view conformsToProtocol:@protocol(WebDocumentSearching)]) {
            NSView <WebDocumentSearching> *searchView = (NSView <WebDocumentSearching> *)view;
            
            if (frame == startFrame)
                startSearchView = searchView;
            
            // In some cases we have to search some content twice; see comment later in this method.
            // We can avoid ever doing this in the common one-frame case by passing the wrap option through 
            // here, and then bailing out before we get to the code that would search again in the
            // same content.
            WebFindOptions optionsForThisPass = onlyOneFrame ? options : (options & ~WebFindOptionsWrapAround);

            if (findString(searchView, string, optionsForThisPass)) {
                if (frame != startFrame)
                    [startFrame _clearSelection];
                [[self window] makeFirstResponder:searchView];
                return YES;
            }
            
            if (onlyOneFrame)
                return NO;
        }
        frame = nextFrame;
    } while (frame && frame != startFrame);
    
    // If there are multiple frames and WebFindOptionsWrapAround is set and we've visited each one without finding a result, we still need to search in the 
    // first-searched frame up to the selection. However, the API doesn't provide a way to search only up to a particular point. The only 
    // way to make sure the entire frame is searched is to pass WebFindOptionsWrapAround. When there are no matches, this will search
    // some content that we already searched on the first pass. In the worst case, we could search the entire contents of this frame twice.
    // To fix this, we'd need to add a mechanism to specify a range in which to search.
    if ((options & WebFindOptionsWrapAround) && startSearchView) {
        if (findString(startSearchView, string, options)) {
            [[self window] makeFirstResponder:startSearchView];
            return YES;
        }
    }
    return NO;
}

- (DOMRange *)DOMRangeOfString:(NSString *)string relativeTo:(DOMRange *)previousRange options:(WebFindOptions)options
{
    if (!_private->page)
        return nil;

    return kit(_private->page->rangeOfString(string, core(previousRange), coreOptions(options)).get());
}

#if __MAC_OS_X_VERSION_MIN_REQUIRED <= 1070
// FIXME: Remove once WebKit no longer needs to support versions of Safari that call this.
- (void)setHoverFeedbackSuspended:(BOOL)newValue
{
}

// FIXME: Remove once WebKit no longer needs to support versions of Safari that call this.
- (BOOL)isHoverFeedbackSuspended
{
    return NO;
}
#endif

- (void)setMainFrameDocumentReady:(BOOL)mainFrameDocumentReady
{
    // by setting this to NO, calls to mainFrameDocument are forced to return nil
    // setting this to YES lets it return the actual DOMDocument value
    // we use this to tell NSTreeController to reset its observers and clear its state
    if (_private->mainFrameDocumentReady == mainFrameDocumentReady)
        return;
    [self _willChangeValueForKey:_WebMainFrameDocumentKey];
    _private->mainFrameDocumentReady = mainFrameDocumentReady;
    [self _didChangeValueForKey:_WebMainFrameDocumentKey];
    // this will cause observers to call mainFrameDocument where this flag will be checked
}

- (void)setTabKeyCyclesThroughElements:(BOOL)cyclesElements
{
    _private->tabKeyCyclesThroughElementsChanged = YES;
    if (_private->page)
        _private->page->setTabKeyCyclesThroughElements(cyclesElements);
}

- (BOOL)tabKeyCyclesThroughElements
{
    return _private->page && _private->page->tabKeyCyclesThroughElements();
}

- (void)setScriptDebugDelegate:(id)delegate
{
    _private->scriptDebugDelegate = delegate;
    [self _cacheScriptDebugDelegateImplementations];

    if (delegate)
        [self _attachScriptDebuggerToAllFrames];
    else
        [self _detachScriptDebuggerFromAllFrames];
}

- (id)scriptDebugDelegate
{
    return _private->scriptDebugDelegate;
}
  
- (void)setHistoryDelegate:(id)delegate
{
    _private->historyDelegate = delegate;
    [self _cacheHistoryDelegateImplementations];
}

- (id)historyDelegate
{
    return _private->historyDelegate;
}

- (BOOL)shouldClose
{
    Frame* coreFrame = [self _mainCoreFrame];
    if (!coreFrame)
        return YES;
    return coreFrame->loader()->shouldClose();
}

static NSAppleEventDescriptor* aeDescFromJSValue(ExecState* exec, JSC::JSValue jsValue)
{
    NSAppleEventDescriptor* aeDesc = 0;
    if (jsValue.isBoolean())
        return [NSAppleEventDescriptor descriptorWithBoolean:jsValue.asBoolean()];
    if (jsValue.isString())
        return [NSAppleEventDescriptor descriptorWithString:jsValue.getString(exec)];
    if (jsValue.isNumber()) {
        double value = jsValue.asNumber();
        int intValue = value;
        if (value == intValue)
            return [NSAppleEventDescriptor descriptorWithDescriptorType:typeSInt32 bytes:&intValue length:sizeof(intValue)];
        return [NSAppleEventDescriptor descriptorWithDescriptorType:typeIEEE64BitFloatingPoint bytes:&value length:sizeof(value)];
    }
    if (jsValue.isObject()) {
        JSObject* object = jsValue.getObject();
        if (object->inherits(&DateInstance::s_info)) {
            DateInstance* date = static_cast<DateInstance*>(object);
            double ms = date->internalNumber();
            if (!std::isnan(ms)) {
                CFAbsoluteTime utcSeconds = ms / 1000 - kCFAbsoluteTimeIntervalSince1970;
                LongDateTime ldt;
                if (noErr == UCConvertCFAbsoluteTimeToLongDateTime(utcSeconds, &ldt))
                    return [NSAppleEventDescriptor descriptorWithDescriptorType:typeLongDateTime bytes:&ldt length:sizeof(ldt)];
            }
        }
        else if (object->inherits(&JSArray::s_info)) {
            DEFINE_STATIC_LOCAL(HashSet<JSObject*>, visitedElems, ());
            if (!visitedElems.contains(object)) {
                visitedElems.add(object);
                
                JSArray* array = static_cast<JSArray*>(object);
                aeDesc = [NSAppleEventDescriptor listDescriptor];
                unsigned numItems = array->length();
                for (unsigned i = 0; i < numItems; ++i)
                    [aeDesc insertDescriptor:aeDescFromJSValue(exec, array->get(exec, i)) atIndex:0];
                
                visitedElems.remove(object);
                return aeDesc;
            }
        }
        JSC::JSValue primitive = object->toPrimitive(exec);
        if (exec->hadException()) {
            exec->clearException();
            return [NSAppleEventDescriptor nullDescriptor];
        }
        return aeDescFromJSValue(exec, primitive);
    }
    if (jsValue.isUndefined())
        return [NSAppleEventDescriptor descriptorWithTypeCode:cMissingValue];
    ASSERT(jsValue.isNull());
    return [NSAppleEventDescriptor nullDescriptor];
}

- (NSAppleEventDescriptor *)aeDescByEvaluatingJavaScriptFromString:(NSString *)script
{
    Frame* coreFrame = [self _mainCoreFrame];
    if (!coreFrame)
        return nil;
    if (!coreFrame->document())
        return nil;
    JSC::JSValue result = coreFrame->script()->executeScript(script, true).jsValue();
    if (!result) // FIXME: pass errors
        return 0;
    JSLockHolder lock(coreFrame->script()->globalObject(mainThreadNormalWorld())->globalExec());
    return aeDescFromJSValue(coreFrame->script()->globalObject(mainThreadNormalWorld())->globalExec(), result);
}

- (BOOL)canMarkAllTextMatches
{
    if (_private->closed)
        return NO;

    WebFrame *frame = [self mainFrame];
    do {
        id <WebDocumentView> view = [[frame frameView] documentView];
        if (view && ![view conformsToProtocol:@protocol(WebMultipleTextMatches)])
            return NO;
        
        frame = incrementFrame(frame);
    } while (frame);
    
    return YES;
}

- (NSUInteger)countMatchesForText:(NSString *)string options:(WebFindOptions)options highlight:(BOOL)highlight limit:(NSUInteger)limit markMatches:(BOOL)markMatches
{
    return [self countMatchesForText:string inDOMRange:nil options:options highlight:highlight limit:limit markMatches:markMatches];
}

- (NSUInteger)countMatchesForText:(NSString *)string inDOMRange:(DOMRange *)range options:(WebFindOptions)options highlight:(BOOL)highlight limit:(NSUInteger)limit markMatches:(BOOL)markMatches
{
    if (_private->closed)
        return 0;

    WebFrame *frame = [self mainFrame];
    unsigned matchCount = 0;
    do {
        id <WebDocumentView> view = [[frame frameView] documentView];
        if ([view conformsToProtocol:@protocol(WebMultipleTextMatches)]) {
            if (markMatches)
                [(NSView <WebMultipleTextMatches>*)view setMarkedTextMatchesAreHighlighted:highlight];
        
            ASSERT(limit == 0 || matchCount < limit);
            matchCount += [(NSView <WebMultipleTextMatches>*)view countMatchesForText:string inDOMRange:range options:options limit:(limit == 0 ? 0 : limit - matchCount) markMatches:markMatches];

            // Stop looking if we've reached the limit. A limit of 0 means no limit.
            if (limit > 0 && matchCount >= limit)
                break;
        }
        
        frame = incrementFrame(frame);
    } while (frame);
    
    return matchCount;
}

- (void)unmarkAllTextMatches
{
    if (_private->closed)
        return;

    WebFrame *frame = [self mainFrame];
    do {
        id <WebDocumentView> view = [[frame frameView] documentView];
        if ([view conformsToProtocol:@protocol(WebMultipleTextMatches)])
            [(NSView <WebMultipleTextMatches>*)view unmarkAllTextMatches];
        
        frame = incrementFrame(frame);
    } while (frame);
}

- (NSArray *)rectsForTextMatches
{
    if (_private->closed)
        return [NSArray array];

    NSMutableArray *result = [NSMutableArray array];
    WebFrame *frame = [self mainFrame];
    do {
        id <WebDocumentView> view = [[frame frameView] documentView];
        if ([view conformsToProtocol:@protocol(WebMultipleTextMatches)]) {
            NSView <WebMultipleTextMatches> *documentView = (NSView <WebMultipleTextMatches> *)view;
            NSRect documentViewVisibleRect = [documentView visibleRect];
            NSArray *originalRects = [documentView rectsForTextMatches];
            unsigned rectCount = [originalRects count];
            unsigned rectIndex;
            NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
            for (rectIndex = 0; rectIndex < rectCount; ++rectIndex) {
                NSRect r = [[originalRects objectAtIndex:rectIndex] rectValue];
                // Clip rect to document view's visible rect so rect is confined to subframe
                r = NSIntersectionRect(r, documentViewVisibleRect);
                if (NSIsEmptyRect(r))
                    continue;
                
                // Convert rect to our coordinate system
                r = [documentView convertRect:r toView:self];
                [result addObject:[NSValue valueWithRect:r]];
                if (rectIndex % 10 == 0) {
                    [pool drain];
                    pool = [[NSAutoreleasePool alloc] init];
                }
            }
            [pool drain];
        }
        
        frame = incrementFrame(frame);
    } while (frame);
    
    return result;
}

- (void)scrollDOMRangeToVisible:(DOMRange *)range
{
    [[[[range startContainer] ownerDocument] webFrame] _scrollDOMRangeToVisible:range];
}

- (BOOL)allowsUndo
{
    return _private->allowsUndo;
}

- (void)setAllowsUndo:(BOOL)flag
{
    _private->allowsUndo = flag;
}

- (void)setPageSizeMultiplier:(float)m
{
    [self _setZoomMultiplier:m isTextOnly:NO];
}

- (float)pageSizeMultiplier
{
    return ![self _realZoomMultiplierIsTextOnly] ? _private->zoomMultiplier : 1.0f;
}

- (BOOL)canZoomPageIn
{
    return [self _canZoomIn:NO];
}

- (IBAction)zoomPageIn:(id)sender
{
    return [self _zoomIn:sender isTextOnly:NO];
}

- (BOOL)canZoomPageOut
{
    return [self _canZoomOut:NO];
}

- (IBAction)zoomPageOut:(id)sender
{
    return [self _zoomOut:sender isTextOnly:NO];
}

- (BOOL)canResetPageZoom
{
    return [self _canResetZoom:NO];
}

- (IBAction)resetPageZoom:(id)sender
{
    return [self _resetZoom:sender isTextOnly:NO];
}

- (void)setMediaVolume:(float)volume
{
    if (_private->page)
        _private->page->setMediaVolume(volume);
}

- (float)mediaVolume
{
    if (!_private->page)
        return 0;

    return _private->page->mediaVolume();
}

- (void)addVisitedLinks:(NSArray *)visitedLinks
{
    PageGroup& group = core(self)->group();
    
    NSEnumerator *enumerator = [visitedLinks objectEnumerator];
    while (NSString *url = [enumerator nextObject]) {
        size_t length = [url length];
        const UChar* characters = CFStringGetCharactersPtr(reinterpret_cast<CFStringRef>(url));
        if (characters)
            group.addVisitedLink(characters, length);
        else {
            Vector<UChar, 512> buffer(length);
            [url getCharacters:buffer.data()];
            group.addVisitedLink(buffer.data(), length);
        }
    }
}

@end

@implementation WebView (WebViewPrintingPrivate)

- (float)_headerHeight
{
    return CallUIDelegateReturningFloat(self, @selector(webViewHeaderHeight:));
}

- (float)_footerHeight
{
    return CallUIDelegateReturningFloat(self, @selector(webViewFooterHeight:));
}

- (void)_drawHeaderInRect:(NSRect)rect
{
#ifdef DEBUG_HEADER_AND_FOOTER
    NSGraphicsContext *currentContext = [NSGraphicsContext currentContext];
    [currentContext saveGraphicsState];
    [[NSColor yellowColor] set];
    NSRectFill(rect);
    [currentContext restoreGraphicsState];
#endif

    SEL selector = @selector(webView:drawHeaderInRect:);
    if (![_private->UIDelegate respondsToSelector:selector])
        return;

    NSGraphicsContext *currentContext = [NSGraphicsContext currentContext];
    [currentContext saveGraphicsState];

    NSRectClip(rect);
    CallUIDelegate(self, selector, rect);

    [currentContext restoreGraphicsState];
}

- (void)_drawFooterInRect:(NSRect)rect
{
#ifdef DEBUG_HEADER_AND_FOOTER
    NSGraphicsContext *currentContext = [NSGraphicsContext currentContext];
    [currentContext saveGraphicsState];
    [[NSColor cyanColor] set];
    NSRectFill(rect);
    [currentContext restoreGraphicsState];
#endif
    
    SEL selector = @selector(webView:drawFooterInRect:);
    if (![_private->UIDelegate respondsToSelector:selector])
        return;

    NSGraphicsContext *currentContext = [NSGraphicsContext currentContext];
    [currentContext saveGraphicsState];

    NSRectClip(rect);
    CallUIDelegate(self, selector, rect);

    [currentContext restoreGraphicsState];
}

- (void)_adjustPrintingMarginsForHeaderAndFooter
{
    NSPrintOperation *op = [NSPrintOperation currentOperation];
    NSPrintInfo *info = [op printInfo];
    NSMutableDictionary *infoDictionary = [info dictionary];
    
    // We need to modify the top and bottom margins in the NSPrintInfo to account for the space needed by the
    // header and footer. Because this method can be called more than once on the same NSPrintInfo (see 5038087),
    // we stash away the unmodified top and bottom margins the first time this method is called, and we read from
    // those stashed-away values on subsequent calls.
    float originalTopMargin;
    float originalBottomMargin;
    NSNumber *originalTopMarginNumber = [infoDictionary objectForKey:WebKitOriginalTopPrintingMarginKey];
    if (!originalTopMarginNumber) {
        ASSERT(![infoDictionary objectForKey:WebKitOriginalBottomPrintingMarginKey]);
        originalTopMargin = [info topMargin];
        originalBottomMargin = [info bottomMargin];
        [infoDictionary setObject:[NSNumber numberWithFloat:originalTopMargin] forKey:WebKitOriginalTopPrintingMarginKey];
        [infoDictionary setObject:[NSNumber numberWithFloat:originalBottomMargin] forKey:WebKitOriginalBottomPrintingMarginKey];
    } else {
        ASSERT([originalTopMarginNumber isKindOfClass:[NSNumber class]]);
        ASSERT([[infoDictionary objectForKey:WebKitOriginalBottomPrintingMarginKey] isKindOfClass:[NSNumber class]]);
        originalTopMargin = [originalTopMarginNumber floatValue];
        originalBottomMargin = [[infoDictionary objectForKey:WebKitOriginalBottomPrintingMarginKey] floatValue];
    }
    
    float scale = [op _web_pageSetupScaleFactor];
    [info setTopMargin:originalTopMargin + [self _headerHeight] * scale];
    [info setBottomMargin:originalBottomMargin + [self _footerHeight] * scale];
}

- (void)_drawHeaderAndFooter
{
    // The header and footer rect height scales with the page, but the width is always
    // all the way across the printed page (inset by printing margins).
    NSPrintOperation *op = [NSPrintOperation currentOperation];
    float scale = [op _web_pageSetupScaleFactor];
    NSPrintInfo *printInfo = [op printInfo];
    NSSize paperSize = [printInfo paperSize];
    float headerFooterLeft = [printInfo leftMargin]/scale;
    float headerFooterWidth = (paperSize.width - ([printInfo leftMargin] + [printInfo rightMargin]))/scale;
    NSRect footerRect = NSMakeRect(headerFooterLeft, [printInfo bottomMargin]/scale - [self _footerHeight] , 
                                   headerFooterWidth, [self _footerHeight]);
    NSRect headerRect = NSMakeRect(headerFooterLeft, (paperSize.height - [printInfo topMargin])/scale, 
                                   headerFooterWidth, [self _headerHeight]);
    
    [self _drawHeaderInRect:headerRect];
    [self _drawFooterInRect:footerRect];
}
@end

@implementation WebView (WebDebugBinding)

- (void)addObserver:(NSObject *)anObserver forKeyPath:(NSString *)keyPath options:(NSKeyValueObservingOptions)options context:(void *)context
{
    LOG (Bindings, "addObserver:%p forKeyPath:%@ options:%x context:%p", anObserver, keyPath, options, context);
    [super addObserver:anObserver forKeyPath:keyPath options:options context:context];
}

- (void)removeObserver:(NSObject *)anObserver forKeyPath:(NSString *)keyPath
{
    LOG (Bindings, "removeObserver:%p forKeyPath:%@", anObserver, keyPath);
    [super removeObserver:anObserver forKeyPath:keyPath];
}

@end

//==========================================================================================
// Editing

@implementation WebView (WebViewCSS)

- (DOMCSSStyleDeclaration *)computedStyleForElement:(DOMElement *)element pseudoElement:(NSString *)pseudoElement
{
    // FIXME: is this the best level for this conversion?
    if (pseudoElement == nil)
        pseudoElement = @"";

    return [[element ownerDocument] getComputedStyle:element pseudoElement:pseudoElement];
}

@end

@implementation WebView (WebViewEditing)

- (DOMRange *)editableDOMRangeForPoint:(NSPoint)point
{
    Page* page = core(self);
    if (!page)
        return nil;
    return kit(page->mainFrame()->editor().rangeForPoint(IntPoint([self convertPoint:point toView:nil])).get());
}

- (BOOL)_shouldChangeSelectedDOMRange:(DOMRange *)currentRange toDOMRange:(DOMRange *)proposedRange affinity:(NSSelectionAffinity)selectionAffinity stillSelecting:(BOOL)flag
{
    // FIXME: This quirk is needed due to <rdar://problem/4985321> - We can phase it out once Aperture can adopt the new behavior on their end
    if (!WebKitLinkedOnOrAfter(WEBKIT_FIRST_VERSION_WITHOUT_APERTURE_QUIRK) && [[[NSBundle mainBundle] bundleIdentifier] isEqualToString:@"com.apple.Aperture"])
        return YES;
    return [[self _editingDelegateForwarder] webView:self shouldChangeSelectedDOMRange:currentRange toDOMRange:proposedRange affinity:selectionAffinity stillSelecting:flag];
}

- (BOOL)maintainsInactiveSelection
{
    return NO;
}

- (void)setSelectedDOMRange:(DOMRange *)range affinity:(NSSelectionAffinity)selectionAffinity
{
    Frame* coreFrame = core([self _selectedOrMainFrame]);
    if (!coreFrame)
        return;

    if (range == nil)
        coreFrame->selection()->clear();
    else {
        // Derive the frame to use from the range passed in.
        // Using _selectedOrMainFrame could give us a different document than
        // the one the range uses.
        coreFrame = core([range startContainer])->document()->frame();
        if (!coreFrame)
            return;

        coreFrame->selection()->setSelectedRange(core(range), core(selectionAffinity), true);
    }
}

- (DOMRange *)selectedDOMRange
{
    Frame* coreFrame = core([self _selectedOrMainFrame]);
    if (!coreFrame)
        return nil;
    return kit(coreFrame->selection()->toNormalizedRange().get());
}

- (NSSelectionAffinity)selectionAffinity
{
    Frame* coreFrame = core([self _selectedOrMainFrame]);
    if (!coreFrame)
        return NSSelectionAffinityDownstream;
    return kit(coreFrame->selection()->affinity());
}

- (void)setEditable:(BOOL)flag
{
    if ([self isEditable] != flag && _private->page) {
        _private->page->setEditable(flag);
        if (!_private->tabKeyCyclesThroughElementsChanged)
            _private->page->setTabKeyCyclesThroughElements(!flag);
        Frame* mainFrame = [self _mainCoreFrame];
        if (mainFrame) {
            if (flag) {
                mainFrame->editor().applyEditingStyleToBodyElement();
                // If the WebView is made editable and the selection is empty, set it to something.
                if (![self selectedDOMRange])
                    mainFrame->selection()->setSelectionFromNone();
            }
        }
    }
}

- (BOOL)isEditable
{
    return _private->page && _private->page->isEditable();
}

- (void)setTypingStyle:(DOMCSSStyleDeclaration *)style
{
    // We don't know enough at thls level to pass in a relevant WebUndoAction; we'd have to
    // change the API to allow this.
    [[self _selectedOrMainFrame] _setTypingStyle:style withUndoAction:EditActionUnspecified];
}

- (DOMCSSStyleDeclaration *)typingStyle
{
    return [[self _selectedOrMainFrame] _typingStyle];
}

- (void)setSmartInsertDeleteEnabled:(BOOL)flag
{
    if (_private->page->settings()->smartInsertDeleteEnabled() != flag) {
        _private->page->settings()->setSmartInsertDeleteEnabled(flag);
        [[NSUserDefaults standardUserDefaults] setBool:_private->page->settings()->smartInsertDeleteEnabled() forKey:WebSmartInsertDeleteEnabled];
        [self setSelectTrailingWhitespaceEnabled:!flag];
    }
}

- (BOOL)smartInsertDeleteEnabled
{
    return _private->page->settings()->smartInsertDeleteEnabled();
}

- (void)setContinuousSpellCheckingEnabled:(BOOL)flag
{
    if (continuousSpellCheckingEnabled != flag) {
        continuousSpellCheckingEnabled = flag;
        [[NSUserDefaults standardUserDefaults] setBool:continuousSpellCheckingEnabled forKey:WebContinuousSpellCheckingEnabled];
    }
    
    if ([self isContinuousSpellCheckingEnabled]) {
        [[self class] _preflightSpellChecker];
    } else {
        [[self mainFrame] _unmarkAllMisspellings];
    }
}

- (BOOL)isContinuousSpellCheckingEnabled
{
    return (continuousSpellCheckingEnabled && [self _continuousCheckingAllowed]);
}

- (NSInteger)spellCheckerDocumentTag
{
    if (!_private->hasSpellCheckerDocumentTag) {
        _private->spellCheckerDocumentTag = [NSSpellChecker uniqueSpellDocumentTag];
        _private->hasSpellCheckerDocumentTag = YES;
    }
    return _private->spellCheckerDocumentTag;
}

- (NSUndoManager *)undoManager
{
    if (!_private->allowsUndo)
        return nil;

    NSUndoManager *undoManager = [[self _editingDelegateForwarder] undoManagerForWebView:self];
    if (undoManager)
        return undoManager;

    return [super undoManager];
}

- (void)registerForEditingDelegateNotification:(NSString *)name selector:(SEL)selector
{
    NSNotificationCenter *defaultCenter = [NSNotificationCenter defaultCenter];
    if ([_private->editingDelegate respondsToSelector:selector])
        [defaultCenter addObserver:_private->editingDelegate selector:selector name:name object:self];
}

- (void)setEditingDelegate:(id)delegate
{
    if (_private->editingDelegate == delegate)
        return;

    NSNotificationCenter *defaultCenter = [NSNotificationCenter defaultCenter];

    // remove notifications from current delegate
    [defaultCenter removeObserver:_private->editingDelegate name:WebViewDidBeginEditingNotification object:self];
    [defaultCenter removeObserver:_private->editingDelegate name:WebViewDidChangeNotification object:self];
    [defaultCenter removeObserver:_private->editingDelegate name:WebViewDidEndEditingNotification object:self];
    [defaultCenter removeObserver:_private->editingDelegate name:WebViewDidChangeTypingStyleNotification object:self];
    [defaultCenter removeObserver:_private->editingDelegate name:WebViewDidChangeSelectionNotification object:self];
    
    _private->editingDelegate = delegate;
    [_private->editingDelegateForwarder release];
    _private->editingDelegateForwarder = nil;
    
    // add notifications for new delegate
    [self registerForEditingDelegateNotification:WebViewDidBeginEditingNotification selector:@selector(webViewDidBeginEditing:)];
    [self registerForEditingDelegateNotification:WebViewDidChangeNotification selector:@selector(webViewDidChange:)];
    [self registerForEditingDelegateNotification:WebViewDidEndEditingNotification selector:@selector(webViewDidEndEditing:)];
    [self registerForEditingDelegateNotification:WebViewDidChangeTypingStyleNotification selector:@selector(webViewDidChangeTypingStyle:)];
    [self registerForEditingDelegateNotification:WebViewDidChangeSelectionNotification selector:@selector(webViewDidChangeSelection:)];
}

- (id)editingDelegate
{
    return _private->editingDelegate;
}

- (DOMCSSStyleDeclaration *)styleDeclarationWithText:(NSString *)text
{
    // FIXME: Should this really be attached to the document with the current selection?
    DOMCSSStyleDeclaration *decl = [[[self _selectedOrMainFrame] DOMDocument] createCSSStyleDeclaration];
    [decl setCssText:text];
    return decl;
}

@end

@implementation WebView (WebViewGrammarChecking)

// FIXME: This method should be merged into WebViewEditing when we're not in API freeze
- (BOOL)isGrammarCheckingEnabled
{
    return grammarCheckingEnabled;
}

// FIXME: This method should be merged into WebViewEditing when we're not in API freeze
- (void)setGrammarCheckingEnabled:(BOOL)flag
{
    if (grammarCheckingEnabled == flag)
        return;
    
    grammarCheckingEnabled = flag;
    [[NSUserDefaults standardUserDefaults] setBool:grammarCheckingEnabled forKey:WebGrammarCheckingEnabled];    
    [[NSSpellChecker sharedSpellChecker] updatePanels];

    // We call _preflightSpellChecker when turning continuous spell checking on, but we don't need to do that here
    // because grammar checking only occurs on code paths that already preflight spell checking appropriately.
    
    if (![self isGrammarCheckingEnabled])
        [[self mainFrame] _unmarkAllBadGrammar];
}

// FIXME: This method should be merged into WebIBActions when we're not in API freeze
- (void)toggleGrammarChecking:(id)sender
{
    [self setGrammarCheckingEnabled:![self isGrammarCheckingEnabled]];
}

@end

@implementation WebView (WebViewTextChecking)

- (BOOL)isAutomaticQuoteSubstitutionEnabled
{
    return automaticQuoteSubstitutionEnabled;
}

- (BOOL)isAutomaticLinkDetectionEnabled
{
    return automaticLinkDetectionEnabled;
}

- (BOOL)isAutomaticDashSubstitutionEnabled
{
    return automaticDashSubstitutionEnabled;
}

- (BOOL)isAutomaticTextReplacementEnabled
{
    return automaticTextReplacementEnabled;
}

- (BOOL)isAutomaticSpellingCorrectionEnabled
{
    return automaticSpellingCorrectionEnabled;
}

- (void)setAutomaticQuoteSubstitutionEnabled:(BOOL)flag
{
    if (automaticQuoteSubstitutionEnabled == flag)
        return;
    automaticQuoteSubstitutionEnabled = flag;
    [[NSUserDefaults standardUserDefaults] setBool:automaticQuoteSubstitutionEnabled forKey:WebAutomaticQuoteSubstitutionEnabled];    
    [[NSSpellChecker sharedSpellChecker] updatePanels];
}

- (void)toggleAutomaticQuoteSubstitution:(id)sender
{
    [self setAutomaticQuoteSubstitutionEnabled:![self isAutomaticQuoteSubstitutionEnabled]];
}

- (void)setAutomaticLinkDetectionEnabled:(BOOL)flag
{
    if (automaticLinkDetectionEnabled == flag)
        return;
    automaticLinkDetectionEnabled = flag;
    [[NSUserDefaults standardUserDefaults] setBool:automaticLinkDetectionEnabled forKey:WebAutomaticLinkDetectionEnabled];    
    [[NSSpellChecker sharedSpellChecker] updatePanels];
}

- (void)toggleAutomaticLinkDetection:(id)sender
{
    [self setAutomaticLinkDetectionEnabled:![self isAutomaticLinkDetectionEnabled]];
}

- (void)setAutomaticDashSubstitutionEnabled:(BOOL)flag
{
    if (automaticDashSubstitutionEnabled == flag)
        return;
    automaticDashSubstitutionEnabled = flag;
    [[NSUserDefaults standardUserDefaults] setBool:automaticDashSubstitutionEnabled forKey:WebAutomaticDashSubstitutionEnabled];    
    [[NSSpellChecker sharedSpellChecker] updatePanels];
}

- (void)toggleAutomaticDashSubstitution:(id)sender
{
    [self setAutomaticDashSubstitutionEnabled:![self isAutomaticDashSubstitutionEnabled]];
}

- (void)setAutomaticTextReplacementEnabled:(BOOL)flag
{
    if (automaticTextReplacementEnabled == flag)
        return;
    automaticTextReplacementEnabled = flag;
    [[NSUserDefaults standardUserDefaults] setBool:automaticTextReplacementEnabled forKey:WebAutomaticTextReplacementEnabled];    
    [[NSSpellChecker sharedSpellChecker] updatePanels];
}

- (void)toggleAutomaticTextReplacement:(id)sender
{
    [self setAutomaticTextReplacementEnabled:![self isAutomaticTextReplacementEnabled]];
}

- (void)setAutomaticSpellingCorrectionEnabled:(BOOL)flag
{
    if (automaticSpellingCorrectionEnabled == flag)
        return;
    automaticSpellingCorrectionEnabled = flag;
    [[NSUserDefaults standardUserDefaults] setBool:automaticSpellingCorrectionEnabled forKey:WebAutomaticSpellingCorrectionEnabled];    
    [[NSSpellChecker sharedSpellChecker] updatePanels];
}

- (void)toggleAutomaticSpellingCorrection:(id)sender
{
    [self setAutomaticSpellingCorrectionEnabled:![self isAutomaticSpellingCorrectionEnabled]];
}

@end

@implementation WebView (WebViewUndoableEditing)

- (void)replaceSelectionWithNode:(DOMNode *)node
{
    [[self _selectedOrMainFrame] _replaceSelectionWithNode:node selectReplacement:YES smartReplace:NO matchStyle:NO];
}    

- (void)replaceSelectionWithText:(NSString *)text
{
    [[self _selectedOrMainFrame] _replaceSelectionWithText:text selectReplacement:YES smartReplace:NO];
}

- (void)replaceSelectionWithMarkupString:(NSString *)markupString
{
    [[self _selectedOrMainFrame] _replaceSelectionWithMarkupString:markupString baseURLString:nil selectReplacement:YES smartReplace:NO];
}

- (void)replaceSelectionWithArchive:(WebArchive *)archive
{
    [[[self _selectedOrMainFrame] _dataSource] _replaceSelectionWithArchive:archive selectReplacement:YES];
}

- (void)deleteSelection
{
    WebFrame *webFrame = [self _selectedOrMainFrame];
    Frame* coreFrame = core(webFrame);
    if (coreFrame)
        coreFrame->editor().deleteSelectionWithSmartDelete([(WebHTMLView *)[[webFrame frameView] documentView] _canSmartCopyOrDelete]);
}
    
- (void)applyStyle:(DOMCSSStyleDeclaration *)style
{
    // We don't know enough at thls level to pass in a relevant WebUndoAction; we'd have to
    // change the API to allow this.
    WebFrame *webFrame = [self _selectedOrMainFrame];
    Frame* coreFrame = core(webFrame);
    // FIXME: We shouldn't have to make a copy here.
    if (coreFrame)
        coreFrame->editor().applyStyle(core(style)->copyProperties().get());
}

@end

@implementation WebView (WebViewEditingActions)

- (void)_performResponderOperation:(SEL)selector with:(id)parameter
{
    static BOOL reentered = NO;
    if (reentered) {
        [[self nextResponder] tryToPerform:selector with:parameter];
        return;
    }

    // There are two possibilities here.
    //
    // One is that WebView has been called in its role as part of the responder chain.
    // In that case, it's fine to call the first responder and end up calling down the
    // responder chain again. Later we will return here with reentered = YES and continue
    // past the WebView.
    //
    // The other is that we are being called directly, in which case we want to pass the
    // selector down to the view inside us that can handle it, and continue down the
    // responder chain as usual.

    // Pass this selector down to the first responder.
    NSResponder *responder = [self _responderForResponderOperations];
    reentered = YES;
    [responder tryToPerform:selector with:parameter];
    reentered = NO;
}

#define FORWARD(name) \
    - (void)name:(id)sender { [self _performResponderOperation:_cmd with:sender]; }

FOR_EACH_RESPONDER_SELECTOR(FORWARD)

- (void)insertText:(NSString *)text
{
    [self _performResponderOperation:_cmd with:text];
}

- (NSDictionary *)typingAttributes
{
    Frame* coreFrame = core([self _selectedOrMainFrame]);
    if (coreFrame)
        return coreFrame->editor().fontAttributesForSelectionStart();
    
    return nil;
}


@end

@implementation WebView (WebViewEditingInMail)

- (void)_insertNewlineInQuotedContent
{
    [[self _selectedOrMainFrame] _insertParagraphSeparatorInQuotedContent];
}

- (void)_replaceSelectionWithNode:(DOMNode *)node matchStyle:(BOOL)matchStyle
{
    [[self _selectedOrMainFrame] _replaceSelectionWithNode:node selectReplacement:YES smartReplace:NO matchStyle:matchStyle];
}

- (BOOL)_selectionIsCaret
{
    Frame* coreFrame = core([self _selectedOrMainFrame]);
    if (!coreFrame)
        return NO;
    return coreFrame->selection()->isCaret();
}

- (BOOL)_selectionIsAll
{
    Frame* coreFrame = core([self _selectedOrMainFrame]);
    if (!coreFrame)
        return NO;
    return coreFrame->selection()->isAll(CanCrossEditingBoundary);
}

- (void)_simplifyMarkup:(DOMNode *)startNode endNode:(DOMNode *)endNode
{
    Frame* coreFrame = core([self mainFrame]);
    if (!coreFrame || !startNode)
        return;
    Node* coreStartNode= core(startNode);
    if (coreStartNode->document() != coreFrame->document())
        return;
    return coreFrame->editor().simplifyMarkup(coreStartNode, core(endNode));    
}

@end

static WebFrameView *containingFrameView(NSView *view)
{
    while (view && ![view isKindOfClass:[WebFrameView class]])
        view = [view superview];
    return (WebFrameView *)view;    
}

@implementation WebView (WebFileInternal)

- (float)_deviceScaleFactor
{
    if (_private->customDeviceScaleFactor != 0)
        return _private->customDeviceScaleFactor;

    NSWindow *window = [self window];
    NSWindow *hostWindow = [self hostWindow];
#if __MAC_OS_X_VERSION_MIN_REQUIRED >= 1070
    if (window)
        return [window backingScaleFactor];
    if (hostWindow)
        return [hostWindow backingScaleFactor];
    return [[NSScreen mainScreen] backingScaleFactor];
#else
    if (window)
        return [window userSpaceScaleFactor];
    if (hostWindow)
        return [hostWindow userSpaceScaleFactor];
    return [[NSScreen mainScreen] userSpaceScaleFactor];
#endif
}

static inline uint64_t roundUpToPowerOf2(uint64_t num)
{
    return powf(2.0, ceilf(log2f(num)));
}

+ (void)_setCacheModel:(WebCacheModel)cacheModel
{
    if (s_didSetCacheModel && cacheModel == s_cacheModel)
        return;

    NSString *nsurlCacheDirectory = (NSString *)WebCFAutorelease(WKCopyFoundationCacheDirectory());
    if (!nsurlCacheDirectory)
        nsurlCacheDirectory = NSHomeDirectory();

    static uint64_t memSize = roundUpToPowerOf2(WebMemorySize() / 1024 / 1024);
    unsigned long long diskFreeSize = WebVolumeFreeSize(nsurlCacheDirectory) / 1024 / 1000;
    NSURLCache *nsurlCache = [NSURLCache sharedURLCache];

    unsigned cacheTotalCapacity = 0;
    unsigned cacheMinDeadCapacity = 0;
    unsigned cacheMaxDeadCapacity = 0;
    double deadDecodedDataDeletionInterval = 0;

    unsigned pageCacheCapacity = 0;

    NSUInteger nsurlCacheMemoryCapacity = 0;
    NSUInteger nsurlCacheDiskCapacity = 0;

    switch (cacheModel) {
    case WebCacheModelDocumentViewer: {
        // Page cache capacity (in pages)
        pageCacheCapacity = 0;

        // Object cache capacities (in bytes)
        if (memSize >= 4096)
            cacheTotalCapacity = 128 * 1024 * 1024;
        else if (memSize >= 2048)
            cacheTotalCapacity = 96 * 1024 * 1024;
        else if (memSize >= 1024)
            cacheTotalCapacity = 32 * 1024 * 1024;
        else if (memSize >= 512)
            cacheTotalCapacity = 16 * 1024 * 1024;

        cacheMinDeadCapacity = 0;
        cacheMaxDeadCapacity = 0;

        // Foundation memory cache capacity (in bytes)
        nsurlCacheMemoryCapacity = 0;

        // Foundation disk cache capacity (in bytes)
        nsurlCacheDiskCapacity = [nsurlCache diskCapacity];

        break;
    }
    case WebCacheModelDocumentBrowser: {
        // Page cache capacity (in pages)
        if (memSize >= 1024)
            pageCacheCapacity = 3;
        else if (memSize >= 512)
            pageCacheCapacity = 2;
        else if (memSize >= 256)
            pageCacheCapacity = 1;
        else
            pageCacheCapacity = 0;

        // Object cache capacities (in bytes)
        if (memSize >= 4096)
            cacheTotalCapacity = 128 * 1024 * 1024;
        else if (memSize >= 2048)
            cacheTotalCapacity = 96 * 1024 * 1024;
        else if (memSize >= 1024)
            cacheTotalCapacity = 32 * 1024 * 1024;
        else if (memSize >= 512)
            cacheTotalCapacity = 16 * 1024 * 1024;

        cacheMinDeadCapacity = cacheTotalCapacity / 8;
        cacheMaxDeadCapacity = cacheTotalCapacity / 4;

        // Foundation memory cache capacity (in bytes)
        if (memSize >= 2048)
            nsurlCacheMemoryCapacity = 4 * 1024 * 1024;
        else if (memSize >= 1024)
            nsurlCacheMemoryCapacity = 2 * 1024 * 1024;
        else if (memSize >= 512)
            nsurlCacheMemoryCapacity = 1 * 1024 * 1024;
        else
            nsurlCacheMemoryCapacity =      512 * 1024; 

        // Foundation disk cache capacity (in bytes)
        if (diskFreeSize >= 16384)
            nsurlCacheDiskCapacity = 50 * 1024 * 1024;
        else if (diskFreeSize >= 8192)
            nsurlCacheDiskCapacity = 40 * 1024 * 1024;
        else if (diskFreeSize >= 4096)
            nsurlCacheDiskCapacity = 30 * 1024 * 1024;
        else
            nsurlCacheDiskCapacity = 20 * 1024 * 1024;

        break;
    }
    case WebCacheModelPrimaryWebBrowser: {
        // Page cache capacity (in pages)
        // (Research indicates that value / page drops substantially after 3 pages.)
        if (memSize >= 2048)
            pageCacheCapacity = 5;
        else if (memSize >= 1024)
            pageCacheCapacity = 4;
        else if (memSize >= 512)
            pageCacheCapacity = 3;
        else if (memSize >= 256)
            pageCacheCapacity = 2;
        else
            pageCacheCapacity = 1;

        // Object cache capacities (in bytes)
        // (Testing indicates that value / MB depends heavily on content and
        // browsing pattern. Even growth above 128MB can have substantial 
        // value / MB for some content / browsing patterns.)
        if (memSize >= 4096)
            cacheTotalCapacity = 192 * 1024 * 1024;
        else if (memSize >= 2048)
            cacheTotalCapacity = 128 * 1024 * 1024;
        else if (memSize >= 1024)
            cacheTotalCapacity = 64 * 1024 * 1024;
        else if (memSize >= 512)
            cacheTotalCapacity = 32 * 1024 * 1024;

        cacheMinDeadCapacity = cacheTotalCapacity / 4;
        cacheMaxDeadCapacity = cacheTotalCapacity / 2;

        // This code is here to avoid a PLT regression. We can remove it if we
        // can prove that the overall system gain would justify the regression.
        cacheMaxDeadCapacity = max(24u, cacheMaxDeadCapacity);

        deadDecodedDataDeletionInterval = 60;

        // Foundation memory cache capacity (in bytes)
        // (These values are small because WebCore does most caching itself.)
        if (memSize >= 1024)
            nsurlCacheMemoryCapacity = 4 * 1024 * 1024;
        else if (memSize >= 512)
            nsurlCacheMemoryCapacity = 2 * 1024 * 1024;
        else if (memSize >= 256)
            nsurlCacheMemoryCapacity = 1 * 1024 * 1024;
        else
            nsurlCacheMemoryCapacity =      512 * 1024; 

        // Foundation disk cache capacity (in bytes)
        if (diskFreeSize >= 16384)
            nsurlCacheDiskCapacity = 175 * 1024 * 1024;
        else if (diskFreeSize >= 8192)
            nsurlCacheDiskCapacity = 150 * 1024 * 1024;
        else if (diskFreeSize >= 4096)
            nsurlCacheDiskCapacity = 125 * 1024 * 1024;
        else if (diskFreeSize >= 2048)
            nsurlCacheDiskCapacity = 100 * 1024 * 1024;
        else if (diskFreeSize >= 1024)
            nsurlCacheDiskCapacity = 75 * 1024 * 1024;
        else
            nsurlCacheDiskCapacity = 50 * 1024 * 1024;

        break;
    }
    default:
        ASSERT_NOT_REACHED();
    };


    // Don't shrink a big disk cache, since that would cause churn.
    nsurlCacheDiskCapacity = max(nsurlCacheDiskCapacity, [nsurlCache diskCapacity]);

    memoryCache()->setCapacities(cacheMinDeadCapacity, cacheMaxDeadCapacity, cacheTotalCapacity);
    memoryCache()->setDeadDecodedDataDeletionInterval(deadDecodedDataDeletionInterval);
    pageCache()->setCapacity(pageCacheCapacity);
    [nsurlCache setMemoryCapacity:nsurlCacheMemoryCapacity];
    [nsurlCache setDiskCapacity:nsurlCacheDiskCapacity];
    
    s_cacheModel = cacheModel;
    s_didSetCacheModel = YES;
}

+ (WebCacheModel)_cacheModel
{
    return s_cacheModel;
}

+ (WebCacheModel)_didSetCacheModel
{
    return s_didSetCacheModel;
}

+ (WebCacheModel)_maxCacheModelInAnyInstance
{
    WebCacheModel cacheModel = WebCacheModelDocumentViewer;
    NSEnumerator *enumerator = [(NSMutableSet *)allWebViewsSet objectEnumerator];
    while (WebPreferences *preferences = [[enumerator nextObject] preferences])
        cacheModel = max(cacheModel, [preferences cacheModel]);
    return cacheModel;
}

+ (void)_cacheModelChangedNotification:(NSNotification *)notification
{
    WebPreferences *preferences = (WebPreferences *)[notification object];
    ASSERT([preferences isKindOfClass:[WebPreferences class]]);

    WebCacheModel cacheModel = [preferences cacheModel];
    if (![self _didSetCacheModel] || cacheModel > [self _cacheModel])
        [self _setCacheModel:cacheModel];
    else if (cacheModel < [self _cacheModel])
        [self _setCacheModel:max([[WebPreferences standardPreferences] cacheModel], [self _maxCacheModelInAnyInstance])];
}

+ (void)_preferencesRemovedNotification:(NSNotification *)notification
{
    WebPreferences *preferences = (WebPreferences *)[notification object];
    ASSERT([preferences isKindOfClass:[WebPreferences class]]);

    if ([preferences cacheModel] == [self _cacheModel])
        [self _setCacheModel:max([[WebPreferences standardPreferences] cacheModel], [self _maxCacheModelInAnyInstance])];
}

- (WebFrame *)_focusedFrame
{
    NSResponder *resp = [[self window] firstResponder];
    if (resp && [resp isKindOfClass:[NSView class]] && [(NSView *)resp isDescendantOf:[[self mainFrame] frameView]]) {
        WebFrameView *frameView = containingFrameView((NSView *)resp);
        ASSERT(frameView != nil);
        return [frameView webFrame];
    }
    
    return nil;
}

- (BOOL)_isLoading
{
    WebFrame *mainFrame = [self mainFrame];
    return [[mainFrame _dataSource] isLoading]
        || [[mainFrame provisionalDataSource] isLoading];
}

- (WebFrameView *)_frameViewAtWindowPoint:(NSPoint)point
{
    if (_private->closed)
        return nil;
    NSView *view = [self hitTest:[[self superview] convertPoint:point fromView:nil]];
    if (![view isDescendantOf:[[self mainFrame] frameView]])
        return nil;
    WebFrameView *frameView = containingFrameView(view);
    ASSERT(frameView);
    return frameView;
}

+ (void)_preflightSpellCheckerNow:(id)sender
{
    [[NSSpellChecker sharedSpellChecker] _preflightChosenSpellServer];
}

+ (void)_preflightSpellChecker
{
    // As AppKit does, we wish to delay tickling the shared spellchecker into existence on application launch.
    if ([NSSpellChecker sharedSpellCheckerExists]) {
        [self _preflightSpellCheckerNow:self];
    } else {
        [self performSelector:@selector(_preflightSpellCheckerNow:) withObject:self afterDelay:2.0];
    }
}

- (BOOL)_continuousCheckingAllowed
{
    static BOOL allowContinuousSpellChecking = YES;
    static BOOL readAllowContinuousSpellCheckingDefault = NO;
    if (!readAllowContinuousSpellCheckingDefault) {
        if ([[NSUserDefaults standardUserDefaults] objectForKey:@"NSAllowContinuousSpellChecking"]) {
            allowContinuousSpellChecking = [[NSUserDefaults standardUserDefaults] boolForKey:@"NSAllowContinuousSpellChecking"];
        }
        readAllowContinuousSpellCheckingDefault = YES;
    }
    return allowContinuousSpellChecking;
}

- (NSResponder *)_responderForResponderOperations
{
    NSResponder *responder = [[self window] firstResponder];
    WebFrameView *mainFrameView = [[self mainFrame] frameView];
    
    // If the current responder is outside of the webview, use our main frameView or its
    // document view. We also do this for subviews of self that are siblings of the main
    // frameView since clients might insert non-webview-related views there (see 4552713).
    if (responder != self && ![mainFrameView _web_firstResponderIsSelfOrDescendantView]) {
        responder = [mainFrameView documentView];
        if (!responder)
            responder = mainFrameView;
    }
    return responder;
}

- (void)_openFrameInNewWindowFromMenu:(NSMenuItem *)sender
{
    ASSERT_ARG(sender, [sender isKindOfClass:[NSMenuItem class]]);

    NSDictionary *element = [sender representedObject];
    ASSERT([element isKindOfClass:[NSDictionary class]]);

    WebDataSource *dataSource = [(WebFrame *)[element objectForKey:WebElementFrameKey] dataSource];
    NSURLRequest *request = [[dataSource request] copy];
    ASSERT(request);
    
    [self _openNewWindowWithRequest:request];
    [request release];
}

- (void)_searchWithGoogleFromMenu:(id)sender
{
    id documentView = [[[self selectedFrame] frameView] documentView];
    if (![documentView conformsToProtocol:@protocol(WebDocumentText)]) {
        return;
    }
    
    NSString *selectedString = [(id <WebDocumentText>)documentView selectedString];
    if ([selectedString length] == 0) {
        return;
    }
    
    NSPasteboard *pasteboard = [NSPasteboard pasteboardWithUniqueName];
    [pasteboard declareTypes:[NSArray arrayWithObject:NSStringPboardType] owner:nil];
    NSMutableString *s = [selectedString mutableCopy];
    const unichar nonBreakingSpaceCharacter = 0xA0;
    NSString *nonBreakingSpaceString = [NSString stringWithCharacters:&nonBreakingSpaceCharacter length:1];
    [s replaceOccurrencesOfString:nonBreakingSpaceString withString:@" " options:0 range:NSMakeRange(0, [s length])];
    [pasteboard setString:s forType:NSStringPboardType];
    [s release];
    
    // FIXME: seems fragile to use the service by name, but this is what AppKit does
    NSPerformService(@"Search With Google", pasteboard);
}

- (void)_searchWithSpotlightFromMenu:(id)sender
{
    id documentView = [[[self selectedFrame] frameView] documentView];
    if (![documentView conformsToProtocol:@protocol(WebDocumentText)])
        return;

    NSString *selectedString = [(id <WebDocumentText>)documentView selectedString];
    if (![selectedString length])
        return;

    [[NSWorkspace sharedWorkspace] showSearchResultsForQueryString:selectedString];
}

#if USE(GLIB)
- (void)_clearGlibLoopObserver
{
    if (!_private->glibRunLoopObserver)
        return;

    CFRunLoopObserverInvalidate(_private->glibRunLoopObserver);
    CFRelease(_private->glibRunLoopObserver);
    _private->glibRunLoopObserver = 0;
}
#endif
@end

@implementation WebView (WebViewInternal)

+ (BOOL)shouldIncludeInWebKitStatistics
{
    return NO;
}

- (BOOL)_becomingFirstResponderFromOutside
{
    return _private->becomingFirstResponderFromOutside;
}

#if ENABLE(ICONDATABASE)
- (void)_receivedIconChangedNotification:(NSNotification *)notification
{
    // Get the URL for this notification
    NSDictionary *userInfo = [notification userInfo];
    ASSERT([userInfo isKindOfClass:[NSDictionary class]]);
    NSString *urlString = [userInfo objectForKey:WebIconNotificationUserInfoURLKey];
    ASSERT([urlString isKindOfClass:[NSString class]]);
    
    // If that URL matches the current main frame, dispatch the delegate call, which will also unregister
    // us for this notification
    if ([[self mainFrameURL] isEqualTo:urlString])
        [self _dispatchDidReceiveIconFromWebFrame:[self mainFrame]];
}

- (void)_registerForIconNotification:(BOOL)listen
{
    if (listen)
        [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(_receivedIconChangedNotification:) name:WebIconDatabaseDidAddIconNotification object:nil];        
    else
        [[NSNotificationCenter defaultCenter] removeObserver:self name:WebIconDatabaseDidAddIconNotification object:nil];
}

- (void)_dispatchDidReceiveIconFromWebFrame:(WebFrame *)webFrame
{
    // FIXME: This willChangeValueForKey call is too late, because the icon has already changed by now.
    [self _willChangeValueForKey:_WebMainFrameIconKey];
    
    // Since we definitely have an icon and are about to send out the delegate call for that, this WebView doesn't need to listen for the general
    // notification any longer
    [self _registerForIconNotification:NO];

    WebFrameLoadDelegateImplementationCache* cache = &_private->frameLoadDelegateImplementations;
    if (cache->didReceiveIconForFrameFunc) {
        Image* image = iconDatabase().synchronousIconForPageURL(core(webFrame)->document()->url().string(), IntSize(16, 16));
        if (NSImage *icon = webGetNSImage(image, NSMakeSize(16, 16)))
            CallFrameLoadDelegate(cache->didReceiveIconForFrameFunc, self, @selector(webView:didReceiveIcon:forFrame:), icon, webFrame);
    }

    [self _didChangeValueForKey:_WebMainFrameIconKey];
}
#endif // ENABLE(ICONDATABASE)

- (void)_addObject:(id)object forIdentifier:(unsigned long)identifier
{
    ASSERT(!_private->identifierMap.contains(identifier));

    // If the identifier map is initially empty it means we're starting a load
    // of something. The semantic is that the web view should be around as long 
    // as something is loading. Because of that we retain the web view.
    if (_private->identifierMap.isEmpty())
        CFRetain(self);
    
    _private->identifierMap.set(identifier, object);
}

- (id)_objectForIdentifier:(unsigned long)identifier
{
    return _private->identifierMap.get(identifier).get();
}

- (void)_removeObjectForIdentifier:(unsigned long)identifier
{
    ASSERT(_private->identifierMap.contains(identifier));
    _private->identifierMap.remove(identifier);
    
    // If the identifier map is now empty it means we're no longer loading anything
    // and we should release the web view. Autorelease rather than release in order to
    // avoid re-entering this method beneath -dealloc with the same identifier. <rdar://problem/10523721>
    if (_private->identifierMap.isEmpty())
        WebCFAutorelease(self);
}

- (void)_retrieveKeyboardUIModeFromPreferences:(NSNotification *)notification
{
    CFPreferencesAppSynchronize(UniversalAccessDomain);

    Boolean keyExistsAndHasValidFormat;
    int mode = CFPreferencesGetAppIntegerValue(AppleKeyboardUIMode, UniversalAccessDomain, &keyExistsAndHasValidFormat);
    
    // The keyboard access mode is reported by two bits:
    // Bit 0 is set if feature is on
    // Bit 1 is set if full keyboard access works for any control, not just text boxes and lists
    _private->_keyboardUIMode = (mode & 0x2) ? KeyboardAccessFull : KeyboardAccessDefault;
    
    // check for tabbing to links
    if ([_private->preferences tabsToLinks])
        _private->_keyboardUIMode = (KeyboardUIMode)(_private->_keyboardUIMode | KeyboardAccessTabsToLinks);
}

- (KeyboardUIMode)_keyboardUIMode
{
    if (!_private->_keyboardUIModeAccessed) {
        _private->_keyboardUIModeAccessed = YES;

        [self _retrieveKeyboardUIModeFromPreferences:nil];
        
        [[NSDistributedNotificationCenter defaultCenter] 
            addObserver:self selector:@selector(_retrieveKeyboardUIModeFromPreferences:) 
            name:KeyboardUIModeDidChangeNotification object:nil];

        [[NSNotificationCenter defaultCenter] 
            addObserver:self selector:@selector(_retrieveKeyboardUIModeFromPreferences:) 
            name:WebPreferencesChangedInternalNotification object:nil];
    }
    return _private->_keyboardUIMode;
}

- (void)_setInsertionPasteboard:(NSPasteboard *)pasteboard
{
    _private->insertionPasteboard = pasteboard;
}

- (Frame*)_mainCoreFrame
{
    return (_private && _private->page) ? _private->page->mainFrame() : 0;
}

- (WebFrame *)_selectedOrMainFrame
{
    WebFrame *result = [self selectedFrame];
    if (result == nil)
        result = [self mainFrame];
    return result;
}

#if USE(ACCELERATED_COMPOSITING)

- (BOOL)_needsOneShotDrawingSynchronization
{
    return _private->needsOneShotDrawingSynchronization;
}

- (void)_setNeedsOneShotDrawingSynchronization:(BOOL)needsSynchronization
{
    _private->needsOneShotDrawingSynchronization = needsSynchronization;
}

- (BOOL)_flushCompositingChanges
{
    Frame* frame = [self _mainCoreFrame];
    if (frame && frame->view())
        return frame->view()->flushCompositingStateIncludingSubframes();

    return YES;
}

/*
    The order of events with compositing updates is this:
    
   Start of runloop                                        End of runloop
        |                                                       |
      --|-------------------------------------------------------|--
           ^         ^                                        ^
           |         |                                        |
    NSWindow update, |                                     CA commit
     NSView drawing  |                                  
        flush        |                                  
                layerSyncRunLoopObserverCallBack

    To avoid flashing, we have to ensure that compositing changes (rendered via
    the CoreAnimation rendering display link) appear on screen at the same time
    as content painted into the window via the normal WebCore rendering path.

    CoreAnimation will commit any layer changes at the end of the runloop via
    its "CA commit" observer. Those changes can then appear onscreen at any time
    when the display link fires, which can result in unsynchronized rendering.
    
    To fix this, the GraphicsLayerCA code in WebCore does not change the CA
    layer tree during style changes and layout; it stores up all changes and
    commits them via flushCompositingState(). There are then two situations in
    which we can call flushCompositingState():
    
    1. When painting. FrameView::paintContents() makes a call to flushCompositingState().
    
    2. When style changes/layout have made changes to the layer tree which do not
       result in painting. In this case we need a run loop observer to do a
       flushCompositingState() at an appropriate time. The observer will keep firing
       until the time is right (essentially when there are no more pending layouts).
    
*/
bool LayerFlushController::flushLayers()
{
    NSWindow *window = [m_webView window];
    
    // An NSWindow may not display in the next runloop cycle after dirtying due to delayed window display logic,
    // in which case this observer can fire first. So if the window is due for a display, don't commit
    // layer changes, otherwise they'll show on screen before the view drawing.
    bool viewsNeedDisplay;
#ifndef __LP64__
    if (window && [window _wrapsCarbonWindow])
        viewsNeedDisplay = HIViewGetNeedsDisplay(HIViewGetRoot(static_cast<WindowRef>([window windowRef])));
    else
#endif
        viewsNeedDisplay = [window viewsNeedDisplay];

    if (viewsNeedDisplay)
        return false;

    if ([m_webView _flushCompositingChanges]) {
        // AppKit may have disabled screen updates, thinking an upcoming window flush will re-enable them.
        // In case setNeedsDisplayInRect() has prevented the window from needing to be flushed, re-enable screen
        // updates here.
        if (![window isFlushWindowDisabled])
            [window _enableScreenUpdatesIfNeeded];

        return true;
    }

    // Since the WebView does not need display, -viewWillDraw will not be called. Perform pending layout now,
    // so that the layers draw with up-to-date layout. 
    [m_webView _viewWillDrawInternal];

    return false;
}

- (void)_scheduleCompositingLayerFlush
{
    if (!_private->layerFlushController)
        _private->layerFlushController = LayerFlushController::create(self);
    _private->layerFlushController->scheduleLayerFlush();
}

#endif

#if ENABLE(VIDEO)

- (void)_enterFullscreenForNode:(WebCore::Node*)node
{
    ASSERT(node->hasTagName(WebCore::HTMLNames::videoTag));
    HTMLMediaElement* videoElement = toHTMLMediaElement(node);

    if (_private->fullscreenController) {
        if ([_private->fullscreenController mediaElement] == videoElement) {
            // The backend may just warn us that the underlaying plaftormMovie()
            // has changed. Just force an update.
            [_private->fullscreenController setMediaElement:videoElement];
            return; // No more to do.
        }

        // First exit Fullscreen for the old mediaElement.
        [_private->fullscreenController mediaElement]->exitFullscreen();
        // This previous call has to trigger _exitFullscreen,
        // which has to clear _private->fullscreenController.
        ASSERT(!_private->fullscreenController);
    }
    if (!_private->fullscreenController) {
        _private->fullscreenController = [[WebVideoFullscreenController alloc] init];
        [_private->fullscreenController setMediaElement:videoElement];
        [_private->fullscreenController enterFullscreen:[[self window] screen]];        
    }
    else
        [_private->fullscreenController setMediaElement:videoElement];
}

- (void)_exitFullscreen
{
    if (!_private->fullscreenController)
        return;
    [_private->fullscreenController exitFullscreen];
    [_private->fullscreenController release];
    _private->fullscreenController = nil;
}

#endif

#if ENABLE(FULLSCREEN_API)
- (BOOL)_supportsFullScreenForElement:(const WebCore::Element*)element withKeyboard:(BOOL)withKeyboard
{
    if (![[WebPreferences standardPreferences] fullScreenEnabled])
        return NO;

    return !withKeyboard;
}

- (void)_enterFullScreenForElement:(WebCore::Element*)element
{
    if (!_private->newFullscreenController)
        _private->newFullscreenController = [[WebFullScreenController alloc] init];

    [_private->newFullscreenController setElement:element];
    [_private->newFullscreenController setWebView:self];
    [_private->newFullscreenController enterFullScreen:[[self window] screen]];        
}

- (void)_exitFullScreenForElement:(WebCore::Element*)element
{
    if (!_private->newFullscreenController)
        return;
    [_private->newFullscreenController exitFullScreen];
}
#endif

#if USE(GLIB)

static void glibContextIterationCallback(CFRunLoopObserverRef, CFRunLoopActivity, void*)
{
    g_main_context_iteration(0, FALSE);
}

- (void)_scheduleGlibContextIterations
{
    if (_private->glibRunLoopObserver)
        return;

    NSRunLoop* myRunLoop = [NSRunLoop currentRunLoop];

    // Create a run loop observer and attach it to the run loop.
    CFRunLoopObserverContext context = {0, self, 0, 0, 0};
    _private->glibRunLoopObserver = CFRunLoopObserverCreate(kCFAllocatorDefault, kCFRunLoopBeforeWaiting, YES, 0, &glibContextIterationCallback, &context);

    if (_private->glibRunLoopObserver) {
        CFRunLoopRef cfLoop = [myRunLoop getCFRunLoop];
        CFRunLoopAddObserver(cfLoop, _private->glibRunLoopObserver, kCFRunLoopDefaultMode);
    }

}
#endif

#if USE(AUTOCORRECTION_PANEL)
- (void)handleAcceptedAlternativeText:(NSString*)text
{
    WebFrame *webFrame = [self _selectedOrMainFrame];
    Frame* coreFrame = core(webFrame);
    if (coreFrame)
        coreFrame->editor().handleAlternativeTextUIResult(text);
}
#endif

#if USE(DICTATION_ALTERNATIVES)
- (void)_getWebCoreDictationAlternatives:(Vector<DictationAlternative>&)alternatives fromTextAlternatives:(const Vector<TextAlternativeWithRange>&)alternativesWithRange
{
    for (size_t i = 0; i < alternativesWithRange.size(); ++i) {
        const TextAlternativeWithRange& alternativeWithRange = alternativesWithRange[i];
        uint64_t dictationContext = _private->m_alternativeTextUIController->addAlternatives(alternativeWithRange.alternatives);
        if (dictationContext)
            alternatives.append(DictationAlternative(alternativeWithRange.range.location, alternativeWithRange.range.length, dictationContext));
    }
}

- (void)_showDictationAlternativeUI:(const WebCore::FloatRect&)boundingBoxOfDictatedText forDictationContext:(uint64_t)dictationContext
{
    _private->m_alternativeTextUIController->showAlternatives(self, [self _convertRectFromRootView:boundingBoxOfDictatedText], dictationContext, ^(NSString* acceptedAlternative) {
        [self handleAcceptedAlternativeText:acceptedAlternative];
    });
}

- (void)_removeDictationAlternatives:(uint64_t)dictationContext
{
    _private->m_alternativeTextUIController->removeAlternatives(dictationContext);
}

- (Vector<String>)_dictationAlternatives:(uint64_t)dictationContext
{
    return _private->m_alternativeTextUIController->alternativesForContext(dictationContext);
}
#endif

- (NSPoint)_convertPointFromRootView:(NSPoint)point
{
    return NSMakePoint(point.x, [self bounds].size.height - point.y);
}

- (NSRect)_convertRectFromRootView:(NSRect)rect
{
    return NSMakeRect(rect.origin.x, [self bounds].size.height - rect.origin.y - rect.size.height, rect.size.width, rect.size.height);
}

@end

@implementation WebView (WebViewDeviceOrientation)

- (void)_setDeviceOrientationProvider:(id<WebDeviceOrientationProvider>)deviceOrientationProvider
{
    if (_private)
        _private->m_deviceOrientationProvider = deviceOrientationProvider;
}

- (id<WebDeviceOrientationProvider>)_deviceOrientationProvider
{
    if (_private)
        return _private->m_deviceOrientationProvider;
    return nil;
}

@end

@implementation WebView (WebViewGeolocation)

- (void)_setGeolocationProvider:(id<WebGeolocationProvider>)geolocationProvider
{
    if (_private)
        _private->_geolocationProvider = geolocationProvider;
}

- (id<WebGeolocationProvider>)_geolocationProvider
{
    if (_private)
        return _private->_geolocationProvider;
    return nil;
}

- (void)_geolocationDidChangePosition:(WebGeolocationPosition *)position
{
#if ENABLE(GEOLOCATION)
    if (_private && _private->page)
        WebCore::GeolocationController::from(_private->page)->positionChanged(core(position));
#endif // ENABLE(GEOLOCATION)
}

- (void)_geolocationDidFailWithMessage:(NSString *)errorMessage
{
#if ENABLE(GEOLOCATION)
    if (_private && _private->page) {
        RefPtr<GeolocationError> geolocatioError = GeolocationError::create(GeolocationError::PositionUnavailable, errorMessage);
        WebCore::GeolocationController::from(_private->page)->errorOccurred(geolocatioError.get());
    }
#endif // ENABLE(GEOLOCATION)
}

@end

@implementation WebView (WebViewNotification)
- (void)_setNotificationProvider:(id<WebNotificationProvider>)notificationProvider
{
    if (_private && !_private->_notificationProvider) {
        _private->_notificationProvider = notificationProvider;
        [_private->_notificationProvider registerWebView:self];
    }
}

- (id<WebNotificationProvider>)_notificationProvider
{
    if (_private)
        return _private->_notificationProvider;
    return nil;
}

- (void)_notificationDidShow:(uint64_t)notificationID
{
    [[self _notificationProvider] webView:self didShowNotification:notificationID];
}

- (void)_notificationDidClick:(uint64_t)notificationID
{
    [[self _notificationProvider] webView:self didClickNotification:notificationID];
}

- (void)_notificationsDidClose:(NSArray *)notificationIDs
{
    [[self _notificationProvider] webView:self didCloseNotifications:notificationIDs];
}

- (uint64_t)_notificationIDForTesting:(JSValueRef)jsNotification
{
#if ENABLE(NOTIFICATIONS) || ENABLE(LEGACY_NOTIFICATIONS)
    JSContextRef context = [[self mainFrame] globalContext];
    WebCore::Notification* notification = toNotification(toJS(toJS(context), jsNotification));
    return static_cast<WebNotificationClient*>(NotificationController::clientFrom(_private->page))->notificationIDForTesting(notification);
#else
    return 0;
#endif
}
@end

@implementation WebView (WebViewFullScreen)

- (NSView*)fullScreenPlaceholderView
{
#if ENABLE(FULLSCREEN_API)
    if (_private->newFullscreenController && [_private->newFullscreenController isFullScreen])
        return [_private->newFullscreenController webViewPlaceholder];
#endif
    return nil;
}

@end

void WebInstallMemoryPressureHandler(void)
{
    memoryPressureHandler().install();
}
