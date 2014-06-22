/*
 * Copyright (C) 2005, 2006, 2007, 2008 Apple Inc. All rights reserved.
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

#import "WebFrameInternal.h"

#import "DOMCSSStyleDeclarationInternal.h"
#import "DOMDocumentFragmentInternal.h"
#import "DOMDocumentInternal.h"
#import "DOMElementInternal.h"
#import "DOMHTMLElementInternal.h"
#import "DOMNodeInternal.h"
#import "DOMRangeInternal.h"
#import "WebArchiveInternal.h"
#import "WebChromeClient.h"
#import "WebDataSourceInternal.h"
#import "WebDocumentLoaderMac.h"
#import "WebDynamicScrollBarsView.h"
#import "WebElementDictionary.h"
#import "WebFrameLoaderClient.h"
#import "WebFrameViewInternal.h"
#import "WebHTMLView.h"
#import "WebHTMLViewInternal.h"
#import "WebKitStatisticsPrivate.h"
#import "WebKitVersionChecks.h"
#import "WebNSObjectExtras.h"
#import "WebNSURLExtras.h"
#import "WebScriptDebugger.h"
#import "WebScriptWorldInternal.h"
#import "WebViewInternal.h"
#import <JavaScriptCore/APICast.h>
#import <JavaScriptCore/JSContextInternal.h>
#import <WebCore/AXObjectCache.h>
#import <WebCore/AccessibilityObject.h>
#import <WebCore/AnimationController.h>
#import <WebCore/CSSStyleDeclaration.h>
#import <WebCore/CachedResourceLoader.h>
#import <WebCore/Chrome.h>
#import <WebCore/ColorMac.h>
#import <WebCore/DOMImplementation.h>
#import <WebCore/DatabaseManager.h>
#import <WebCore/DocumentFragment.h>
#import <WebCore/DocumentLoader.h>
#import <WebCore/DocumentMarkerController.h>
#import <WebCore/Editor.h>
#import <WebCore/EventHandler.h>
#import <WebCore/EventNames.h>
#import <WebCore/Frame.h>
#import <WebCore/FrameLoadRequest.h>
#import <WebCore/FrameLoader.h>
#import <WebCore/FrameLoaderStateMachine.h>
#import <WebCore/FrameTree.h>
#import <WebCore/GraphicsContext.h>
#import <WebCore/HTMLFrameOwnerElement.h>
#import <WebCore/HTMLNames.h>
#import <WebCore/HistoryItem.h>
#import <WebCore/HitTestResult.h>
#import <WebCore/JSNode.h>
#import <WebCore/LegacyWebArchive.h>
#import <WebCore/Page.h>
#import <WebCore/PlatformEventFactoryMac.h>
#import <WebCore/PluginData.h>
#import <WebCore/PrintContext.h>
#import <WebCore/RenderPart.h>
#import <WebCore/RenderView.h>
#import <WebCore/RuntimeApplicationChecks.h>
#import <WebCore/ScriptController.h>
#import <WebCore/ScriptValue.h>
#import <WebCore/SecurityOrigin.h>
#import <WebCore/SmartReplace.h>
#import <WebCore/StylePropertySet.h>
#import <WebCore/TextIterator.h>
#import <WebCore/ThreadCheck.h>
#import <WebCore/VisibleUnits.h>
#import <WebCore/htmlediting.h>
#import <WebCore/markup.h>
#import <WebKitSystemInterface.h>
#import <runtime/JSLock.h>
#import <runtime/JSObject.h>
#import <runtime/JSCJSValue.h>
#import <wtf/CurrentTime.h>

using namespace WebCore;
using namespace HTMLNames;

using JSC::JSGlobalObject;
using JSC::JSLock;

/*
Here is the current behavior matrix for four types of navigations:

Standard Nav:

 Restore form state:   YES
 Restore scroll and focus state:  YES
 Cache policy: NSURLRequestUseProtocolCachePolicy
 Add to back/forward list: YES
 
Back/Forward:

 Restore form state:   YES
 Restore scroll and focus state:  YES
 Cache policy: NSURLRequestReturnCacheDataElseLoad
 Add to back/forward list: NO

Reload (meaning only the reload button):

 Restore form state:   NO
 Restore scroll and focus state:  YES
 Cache policy: NSURLRequestReloadIgnoringCacheData
 Add to back/forward list: NO

Repeat load of the same URL (by any other means of navigation other than the reload button, including hitting return in the location field):

 Restore form state:   NO
 Restore scroll and focus state:  NO, reset to initial conditions
 Cache policy: NSURLRequestReloadIgnoringCacheData
 Add to back/forward list: NO
*/

NSString *WebPageCacheEntryDateKey = @"WebPageCacheEntryDateKey";
NSString *WebPageCacheDataSourceKey = @"WebPageCacheDataSourceKey";
NSString *WebPageCacheDocumentViewKey = @"WebPageCacheDocumentViewKey";

NSString *WebFrameMainDocumentError = @"WebFrameMainDocumentErrorKey";
NSString *WebFrameHasPlugins = @"WebFrameHasPluginsKey";
NSString *WebFrameHasUnloadListener = @"WebFrameHasUnloadListenerKey";
NSString *WebFrameUsesDatabases = @"WebFrameUsesDatabasesKey";
NSString *WebFrameUsesGeolocation = @"WebFrameUsesGeolocationKey";
NSString *WebFrameUsesApplicationCache = @"WebFrameUsesApplicationCacheKey";
NSString *WebFrameCanSuspendActiveDOMObjects = @"WebFrameCanSuspendActiveDOMObjectsKey";

// FIXME: Remove when this key becomes publicly defined
NSString *NSAccessibilityEnhancedUserInterfaceAttribute = @"AXEnhancedUserInterface";

@implementation WebFramePrivate

- (void)dealloc
{
    [webFrameView release];

    delete scriptDebugger;

    [super dealloc];
}

- (void)finalize
{
    delete scriptDebugger;

    [super finalize];
}

- (void)setWebFrameView:(WebFrameView *)v 
{ 
    [v retain];
    [webFrameView release];
    webFrameView = v;
}

@end

EditableLinkBehavior core(WebKitEditableLinkBehavior editableLinkBehavior)
{
    switch (editableLinkBehavior) {
        case WebKitEditableLinkDefaultBehavior:
            return EditableLinkDefaultBehavior;
        case WebKitEditableLinkAlwaysLive:
            return EditableLinkAlwaysLive;
        case WebKitEditableLinkOnlyLiveWithShiftKey:
            return EditableLinkOnlyLiveWithShiftKey;
        case WebKitEditableLinkLiveWhenNotFocused:
            return EditableLinkLiveWhenNotFocused;
        case WebKitEditableLinkNeverLive:
            return EditableLinkNeverLive;
    }
    ASSERT_NOT_REACHED();
    return EditableLinkDefaultBehavior;
}

TextDirectionSubmenuInclusionBehavior core(WebTextDirectionSubmenuInclusionBehavior behavior)
{
    switch (behavior) {
        case WebTextDirectionSubmenuNeverIncluded:
            return TextDirectionSubmenuNeverIncluded;
        case WebTextDirectionSubmenuAutomaticallyIncluded:
            return TextDirectionSubmenuAutomaticallyIncluded;
        case WebTextDirectionSubmenuAlwaysIncluded:
            return TextDirectionSubmenuAlwaysIncluded;
    }
    ASSERT_NOT_REACHED();
    return TextDirectionSubmenuNeverIncluded;
}

@implementation WebFrame (WebInternal)

Frame* core(WebFrame *frame)
{
    return frame ? frame->_private->coreFrame : 0;
}

WebFrame *kit(Frame* frame)
{
    if (!frame)
        return nil;

    FrameLoaderClient* frameLoaderClient = frame->loader()->client();
    if (frameLoaderClient->isEmptyFrameLoaderClient())
        return nil;

    return static_cast<WebFrameLoaderClient*>(frameLoaderClient)->webFrame();
}

Page* core(WebView *webView)
{
    return [webView page];
}

WebView *kit(Page* page)
{
    if (!page)
        return nil;

    ChromeClient* chromeClient = page->chrome().client();
    if (chromeClient->isEmptyChromeClient())
        return nil;

    return static_cast<WebChromeClient*>(chromeClient)->webView();
}

WebView *getWebView(WebFrame *webFrame)
{
    Frame* coreFrame = core(webFrame);
    if (!coreFrame)
        return nil;
    return kit(coreFrame->page());
}

+ (PassRefPtr<Frame>)_createFrameWithPage:(Page*)page frameName:(const String&)name frameView:(WebFrameView *)frameView ownerElement:(HTMLFrameOwnerElement*)ownerElement
{
    WebView *webView = kit(page);

    WebFrame *frame = [[self alloc] _initWithWebFrameView:frameView webView:webView];
    RefPtr<Frame> coreFrame = Frame::create(page, ownerElement, new WebFrameLoaderClient(frame));
    [frame release];
    frame->_private->coreFrame = coreFrame.get();

    coreFrame->tree()->setName(name);
    if (ownerElement) {
        ASSERT(ownerElement->document()->frame());
        ownerElement->document()->frame()->tree()->appendChild(coreFrame.get());
    }

    coreFrame->init();

    [webView _setZoomMultiplier:[webView _realZoomMultiplier] isTextOnly:[webView _realZoomMultiplierIsTextOnly]];

    return coreFrame.release();
}

+ (void)_createMainFrameWithPage:(Page*)page frameName:(const String&)name frameView:(WebFrameView *)frameView
{
    [self _createFrameWithPage:page frameName:name frameView:frameView ownerElement:0];
}

+ (PassRefPtr<WebCore::Frame>)_createSubframeWithOwnerElement:(HTMLFrameOwnerElement*)ownerElement frameName:(const String&)name frameView:(WebFrameView *)frameView
{
    return [self _createFrameWithPage:ownerElement->document()->frame()->page() frameName:name frameView:frameView ownerElement:ownerElement];
}

- (BOOL)_isIncludedInWebKitStatistics
{
    return _private && _private->includedInWebKitStatistics;
}

- (void)_attachScriptDebugger
{
    ScriptController* scriptController = _private->coreFrame->script();

    // Calling ScriptController::globalObject() would create a window shell, and dispatch corresponding callbacks, which may be premature
    // if the script debugger is attached before a document is created.  These calls use the debuggerWorld(), we will need to pass a world
    // to be able to debug isolated worlds.
    if (!scriptController->existingWindowShell(debuggerWorld()))
        return;

    JSGlobalObject* globalObject = scriptController->globalObject(debuggerWorld());
    if (!globalObject)
        return;

    if (_private->scriptDebugger) {
        ASSERT(_private->scriptDebugger == globalObject->debugger());
        return;
    }

    _private->scriptDebugger = new WebScriptDebugger(globalObject);
}

- (void)_detachScriptDebugger
{
    if (!_private->scriptDebugger)
        return;

    delete _private->scriptDebugger;
    _private->scriptDebugger = 0;
}

- (id)_initWithWebFrameView:(WebFrameView *)fv webView:(WebView *)v
{
    self = [super init];
    if (!self)
        return nil;

    _private = [[WebFramePrivate alloc] init];

    // Set includedInWebKitStatistics before calling WebFrameView _setWebFrame, since
    // it calls WebFrame _isIncludedInWebKitStatistics.
    if ((_private->includedInWebKitStatistics = [[v class] shouldIncludeInWebKitStatistics]))
        ++WebFrameCount;

    if (fv) {
        [_private setWebFrameView:fv];
        [fv _setWebFrame:self];
    }

    _private->shouldCreateRenderers = YES;

    return self;
}

- (void)_clearCoreFrame
{
    _private->coreFrame = 0;
}

- (void)_updateBackgroundAndUpdatesWhileOffscreen
{
    WebView *webView = getWebView(self);
    BOOL drawsBackground = [webView drawsBackground];
    NSColor *backgroundColor = [webView backgroundColor];

    Frame* coreFrame = _private->coreFrame;
    for (Frame* frame = coreFrame; frame; frame = frame->tree()->traverseNext(coreFrame)) {
        // Don't call setDrawsBackground:YES here because it may be NO because of a load
        // in progress; WebFrameLoaderClient keeps it set to NO during the load process.
        WebFrame *webFrame = kit(frame);
        if (!drawsBackground)
            [[[webFrame frameView] _scrollView] setDrawsBackground:NO];
        [[[webFrame frameView] _scrollView] setBackgroundColor:backgroundColor];

        if (FrameView* view = frame->view()) {
            view->setTransparent(!drawsBackground);
            view->setBaseBackgroundColor(colorFromNSColor([backgroundColor colorUsingColorSpaceName:NSDeviceRGBColorSpace]));
            view->setShouldUpdateWhileOffscreen([webView shouldUpdateWhileOffscreen]);
        }
    }
}

- (void)_setInternalLoadDelegate:(id)internalLoadDelegate
{
    _private->internalLoadDelegate = internalLoadDelegate;
}

- (id)_internalLoadDelegate
{
    return _private->internalLoadDelegate;
}

- (void)_unmarkAllBadGrammar
{
    Frame* coreFrame = _private->coreFrame;
    for (Frame* frame = coreFrame; frame; frame = frame->tree()->traverseNext(coreFrame)) {
        if (Document* document = frame->document())
            document->markers()->removeMarkers(DocumentMarker::Grammar);
    }
}

- (void)_unmarkAllMisspellings
{
    Frame* coreFrame = _private->coreFrame;
    for (Frame* frame = coreFrame; frame; frame = frame->tree()->traverseNext(coreFrame)) {
        if (Document* document = frame->document())
            document->markers()->removeMarkers(DocumentMarker::Spelling);
    }
}

- (BOOL)_hasSelection
{
    id documentView = [_private->webFrameView documentView];    

    // optimization for common case to avoid creating potentially large selection string
    if ([documentView isKindOfClass:[WebHTMLView class]])
        if (Frame* coreFrame = _private->coreFrame)
            return coreFrame->selection()->isRange();

    if ([documentView conformsToProtocol:@protocol(WebDocumentText)])
        return [[documentView selectedString] length] > 0;
    
    return NO;
}

- (void)_clearSelection
{
    id documentView = [_private->webFrameView documentView];    
    if ([documentView conformsToProtocol:@protocol(WebDocumentText)])
        [documentView deselectAll];
}

#if !ASSERT_DISABLED
- (BOOL)_atMostOneFrameHasSelection
{
    // FIXME: 4186050 is one known case that makes this debug check fail.
    BOOL found = NO;
    Frame* coreFrame = _private->coreFrame;
    for (Frame* frame = coreFrame; frame; frame = frame->tree()->traverseNext(coreFrame))
        if ([kit(frame) _hasSelection]) {
            if (found)
                return NO;
            found = YES;
        }
    return YES;
}
#endif

- (WebFrame *)_findFrameWithSelection
{
    Frame* coreFrame = _private->coreFrame;
    for (Frame* frame = coreFrame; frame; frame = frame->tree()->traverseNext(coreFrame)) {
        WebFrame *webFrame = kit(frame);
        if ([webFrame _hasSelection])
            return webFrame;
    }
    return nil;
}

- (void)_clearSelectionInOtherFrames
{
    // We rely on WebDocumentSelection protocol implementors to call this method when they become first 
    // responder. It would be nicer to just notice first responder changes here instead, but there's no 
    // notification sent when the first responder changes in general (Radar 2573089).
    WebFrame *frameWithSelection = [[getWebView(self) mainFrame] _findFrameWithSelection];
    if (frameWithSelection != self)
        [frameWithSelection _clearSelection];

    // While we're in the general area of selection and frames, check that there is only one now.
    ASSERT([[getWebView(self) mainFrame] _atMostOneFrameHasSelection]);
}

static inline WebDataSource *dataSource(DocumentLoader* loader)
{
    return loader ? static_cast<WebDocumentLoaderMac*>(loader)->dataSource() : nil;
}

- (WebDataSource *)_dataSource
{
    return dataSource(_private->coreFrame->loader()->documentLoader());
}

- (NSString *)_stringWithDocumentTypeStringAndMarkupString:(NSString *)markupString
{
    return String(_private->coreFrame->documentTypeString() + String(markupString));
}

- (NSArray *)_nodesFromList:(Vector<Node*> *)nodesVector
{
    size_t size = nodesVector->size();
    NSMutableArray *nodes = [NSMutableArray arrayWithCapacity:size];
    for (size_t i = 0; i < size; ++i)
        [nodes addObject:kit((*nodesVector)[i])];
    return nodes;
}

- (NSString *)_markupStringFromRange:(DOMRange *)range nodes:(NSArray **)nodes
{
    // FIXME: This is always "for interchange". Is that right? See the previous method.
    Vector<Node*> nodeList;
    NSString *markupString = createMarkup(core(range), nodes ? &nodeList : 0, AnnotateForInterchange);
    if (nodes)
        *nodes = [self _nodesFromList:&nodeList];

    return [self _stringWithDocumentTypeStringAndMarkupString:markupString];
}

- (NSString *)_selectedString
{
    return _private->coreFrame->displayStringModifiedByEncoding(_private->coreFrame->editor().selectedText());
}

- (NSString *)_stringForRange:(DOMRange *)range
{
    return plainText(core(range), TextIteratorDefaultBehavior, true);
}

- (BOOL)_shouldFlattenCompositingLayers:(CGContextRef)context
{
    // -currentContextDrawingToScreen returns YES for bitmap contexts.
    BOOL isPrinting = ![NSGraphicsContext currentContextDrawingToScreen];
    if (isPrinting)
        return YES;

    if (!WKCGContextIsBitmapContext(context))
        return NO;

    // If we're drawing into a bitmap, we might be snapshotting, or drawing into a layer-backed view.
    id documentView = [_private->webFrameView documentView];
    if ([documentView isKindOfClass:[WebHTMLView class]] && [(WebHTMLView *)documentView _web_isDrawingIntoLayer])
        return NO;

    return [getWebView(self) _includesFlattenedCompositingLayersWhenDrawingToBitmap];
}

- (void)_drawRect:(NSRect)rect contentsOnly:(BOOL)contentsOnly
{
    ASSERT([[NSGraphicsContext currentContext] isFlipped]);

    CGContextRef ctx = static_cast<CGContextRef>([[NSGraphicsContext currentContext] graphicsPort]);
    GraphicsContext context(ctx);

    FrameView* view = _private->coreFrame->view();
    
    bool shouldFlatten = false;
    if (Frame* parentFrame = _private->coreFrame->tree()->parent()) {
        // For subframes, we need to inherit the paint behavior from our parent
        FrameView* parentView = parentFrame ? parentFrame->view() : 0;
        if (parentView)
            shouldFlatten = parentView->paintBehavior() & PaintBehaviorFlattenCompositingLayers;
    } else
        shouldFlatten = [self _shouldFlattenCompositingLayers:ctx];

    PaintBehavior oldBehavior = PaintBehaviorNormal;
    if (shouldFlatten) {
        oldBehavior = view->paintBehavior();
        view->setPaintBehavior(oldBehavior | PaintBehaviorFlattenCompositingLayers);
    }
    
    if (contentsOnly)
        view->paintContents(&context, enclosingIntRect(rect));
    else
        view->paint(&context, enclosingIntRect(rect));

    if (shouldFlatten)
        view->setPaintBehavior(oldBehavior);
}

- (BOOL)_getVisibleRect:(NSRect*)rect
{
    ASSERT_ARG(rect, rect);
    if (RenderPart* ownerRenderer = _private->coreFrame->ownerRenderer()) {
        if (ownerRenderer->needsLayout())
            return NO;
        *rect = ownerRenderer->pixelSnappedAbsoluteClippedOverflowRect();
        return YES;
    }

    return NO;
}

- (NSString *)_stringByEvaluatingJavaScriptFromString:(NSString *)string
{
    return [self _stringByEvaluatingJavaScriptFromString:string forceUserGesture:true];
}

- (NSString *)_stringByEvaluatingJavaScriptFromString:(NSString *)string forceUserGesture:(BOOL)forceUserGesture
{
    if (!string)
        return @"";

    ASSERT(_private->coreFrame->document());
    RetainPtr<WebFrame> protect(self); // Executing arbitrary JavaScript can destroy the frame.
    
    JSC::JSValue result = _private->coreFrame->script()->executeScript(string, forceUserGesture).jsValue();

    if (!_private->coreFrame) // In case the script removed our frame from the page.
        return @"";

    // This bizarre set of rules matches behavior from WebKit for Safari 2.0.
    // If you don't like it, use -[WebScriptObject evaluateWebScript:] or 
    // JSEvaluateScript instead, since they have less surprising semantics.
    if (!result || (!result.isBoolean() && !result.isString() && !result.isNumber()))
        return @"";

    JSC::ExecState* exec = _private->coreFrame->script()->globalObject(mainThreadNormalWorld())->globalExec();
    JSC::JSLockHolder lock(exec);
    return result.toWTFString(exec);
}

- (NSRect)_caretRectAtPosition:(const Position&)pos affinity:(NSSelectionAffinity)affinity
{
    VisiblePosition visiblePosition(pos, static_cast<EAffinity>(affinity));
    return visiblePosition.absoluteCaretBounds();
}

- (NSRect)_firstRectForDOMRange:(DOMRange *)range
{
   return _private->coreFrame->editor().firstRectForRange(core(range));
}

- (void)_scrollDOMRangeToVisible:(DOMRange *)range
{
    NSRect rangeRect = [self _firstRectForDOMRange:range];    
    Node *startNode = core([range startContainer]);
        
    if (startNode && startNode->renderer())
        startNode->renderer()->scrollRectToVisible(enclosingIntRect(rangeRect), ScrollAlignment::alignToEdgeIfNeeded, ScrollAlignment::alignToEdgeIfNeeded);
}

- (BOOL)_needsLayout
{
    return _private->coreFrame->view() ? _private->coreFrame->view()->needsLayout() : false;
}

- (DOMRange *)_rangeByAlteringCurrentSelection:(FrameSelection::EAlteration)alteration direction:(SelectionDirection)direction granularity:(TextGranularity)granularity
{
    if (_private->coreFrame->selection()->isNone())
        return nil;

    FrameSelection selection;
    selection.setSelection(_private->coreFrame->selection()->selection());
    selection.modify(alteration, direction, granularity);
    return kit(selection.toNormalizedRange().get());
}

- (TextGranularity)_selectionGranularity
{
    return _private->coreFrame->selection()->granularity();
}

- (NSRange)_convertToNSRange:(Range *)range
{
    if (!range)
        return NSMakeRange(NSNotFound, 0);

    size_t location;
    size_t length;
    if (!TextIterator::getLocationAndLengthFromRange(_private->coreFrame->selection()->rootEditableElementOrDocumentElement(), range, location, length))
        return NSMakeRange(NSNotFound, 0);

    return NSMakeRange(location, length);
}

- (PassRefPtr<Range>)_convertToDOMRange:(NSRange)nsrange
{
    if (nsrange.location > INT_MAX)
        return 0;
    if (nsrange.length > INT_MAX || nsrange.location + nsrange.length > INT_MAX)
        nsrange.length = INT_MAX - nsrange.location;

    // our critical assumption is that we are only called by input methods that
    // concentrate on a given area containing the selection
    // We have to do this because of text fields and textareas. The DOM for those is not
    // directly in the document DOM, so serialization is problematic. Our solution is
    // to use the root editable element of the selection start as the positional base.
    // That fits with AppKit's idea of an input context.
    return TextIterator::rangeFromLocationAndLength(_private->coreFrame->selection()->rootEditableElementOrDocumentElement(), nsrange.location, nsrange.length);
}

- (DOMRange *)_convertNSRangeToDOMRange:(NSRange)nsrange
{
    return kit([self _convertToDOMRange:nsrange].get());
}

- (NSRange)_convertDOMRangeToNSRange:(DOMRange *)range
{
    return [self _convertToNSRange:core(range)];
}

- (DOMRange *)_markDOMRange
{
    return kit(_private->coreFrame->editor().mark().toNormalizedRange().get());
}

// Given proposedRange, returns an extended range that includes adjacent whitespace that should
// be deleted along with the proposed range in order to preserve proper spacing and punctuation of
// the text surrounding the deletion.
- (DOMRange *)_smartDeleteRangeForProposedRange:(DOMRange *)proposedRange
{
    Node* startContainer = core([proposedRange startContainer]);
    Node* endContainer = core([proposedRange endContainer]);
    if (startContainer == nil || endContainer == nil)
        return nil;

    ASSERT(startContainer->document() == endContainer->document());
    
    _private->coreFrame->document()->updateLayoutIgnorePendingStylesheets();

    Position start = Position(startContainer, [proposedRange startOffset], Position::PositionIsOffsetInAnchor);
    Position end = Position(endContainer, [proposedRange endOffset], Position::PositionIsOffsetInAnchor);
    Position newStart = start.upstream().leadingWhitespacePosition(DOWNSTREAM, true);
    if (newStart.isNull())
        newStart = start;
    Position newEnd = end.downstream().trailingWhitespacePosition(DOWNSTREAM, true);
    if (newEnd.isNull())
        newEnd = end;

    newStart = newStart.parentAnchoredEquivalent();
    newEnd = newEnd.parentAnchoredEquivalent();

    RefPtr<Range> range = _private->coreFrame->document()->createRange();
    int exception = 0;
    range->setStart(newStart.containerNode(), newStart.offsetInContainerNode(), exception);
    range->setEnd(newStart.containerNode(), newStart.offsetInContainerNode(), exception);
    return kit(range.get());
}

- (DOMDocumentFragment *)_documentFragmentWithMarkupString:(NSString *)markupString baseURLString:(NSString *)baseURLString 
{
    if (!_private->coreFrame || !_private->coreFrame->document())
        return nil;

    return kit(createFragmentFromMarkup(_private->coreFrame->document(), markupString, baseURLString, DisallowScriptingContent).get());
}

- (DOMDocumentFragment *)_documentFragmentWithNodesAsParagraphs:(NSArray *)nodes
{
    if (!_private->coreFrame || !_private->coreFrame->document())
        return nil;
    
    NSEnumerator *nodeEnum = [nodes objectEnumerator];
    Vector<Node*> nodesVector;
    DOMNode *node;
    while ((node = [nodeEnum nextObject]))
        nodesVector.append(core(node));
    
    return kit(createFragmentFromNodes(_private->coreFrame->document(), nodesVector).get());
}

- (void)_replaceSelectionWithNode:(DOMNode *)node selectReplacement:(BOOL)selectReplacement smartReplace:(BOOL)smartReplace matchStyle:(BOOL)matchStyle
{
    DOMDocumentFragment *fragment = kit(_private->coreFrame->document()->createDocumentFragment().get());
    [fragment appendChild:node];
    [self _replaceSelectionWithFragment:fragment selectReplacement:selectReplacement smartReplace:smartReplace matchStyle:matchStyle];
}

- (void)_insertParagraphSeparatorInQuotedContent
{
    if (_private->coreFrame->selection()->isNone())
        return;

    _private->coreFrame->editor().insertParagraphSeparatorInQuotedContent();
}

- (VisiblePosition)_visiblePositionForPoint:(NSPoint)point
{
    // FIXME: Someone with access to Apple's sources could remove this needless wrapper call.
    return _private->coreFrame->visiblePositionForPoint(IntPoint(point));
}

- (DOMRange *)_characterRangeAtPoint:(NSPoint)point
{
    return kit(_private->coreFrame->rangeForPoint(IntPoint(point)).get());
}

- (DOMCSSStyleDeclaration *)_typingStyle
{
    if (!_private->coreFrame)
        return nil;
    RefPtr<MutableStylePropertySet> typingStyle = _private->coreFrame->selection()->copyTypingStyle();
    if (!typingStyle)
        return nil;
    return kit(typingStyle->ensureCSSStyleDeclaration());
}

- (void)_setTypingStyle:(DOMCSSStyleDeclaration *)style withUndoAction:(EditAction)undoAction
{
    if (!_private->coreFrame || !style)
        return;
    // FIXME: We shouldn't have to create a copy here.
    _private->coreFrame->editor().computeAndSetTypingStyle(core(style)->copyProperties().get(), undoAction);
}

#if ENABLE(DRAG_SUPPORT)
- (void)_dragSourceEndedAt:(NSPoint)windowLoc operation:(NSDragOperation)operation
{
    if (!_private->coreFrame)
        return;
    FrameView* view = _private->coreFrame->view();
    if (!view)
        return;
    // FIXME: These are fake modifier keys here, but they should be real ones instead.
    PlatformMouseEvent event(IntPoint(windowLoc), globalPoint(windowLoc, [view->platformWidget() window]),
        LeftButton, PlatformEvent::MouseMoved, 0, false, false, false, false, currentTime());
    _private->coreFrame->eventHandler()->dragSourceEndedAt(event, (DragOperation)operation);
}
#endif

- (BOOL)_canProvideDocumentSource
{
    Frame* frame = _private->coreFrame;
    String mimeType = frame->document()->loader()->writer()->mimeType();
    PluginData* pluginData = frame->page() ? frame->page()->pluginData() : 0;

    if (WebCore::DOMImplementation::isTextMIMEType(mimeType)
        || Image::supportsType(mimeType)
        || (pluginData && pluginData->supportsMimeType(mimeType, PluginData::AllPlugins) && frame->loader()->subframeLoader()->allowPlugins(NotAboutToInstantiatePlugin))
        || (pluginData && pluginData->supportsMimeType(mimeType, PluginData::OnlyApplicationPlugins)))
        return NO;

    return YES;
}

- (BOOL)_canSaveAsWebArchive
{
    // Currently, all documents that we can view source for
    // (HTML and XML documents) can also be saved as web archives
    return [self _canProvideDocumentSource];
}

- (void)_commitData:(NSData *)data
{
    // FIXME: This really should be a setting.
    Document* document = _private->coreFrame->document();
    document->setShouldCreateRenderers(_private->shouldCreateRenderers);

    _private->coreFrame->loader()->documentLoader()->commitData((const char *)[data bytes], [data length]);
}

@end

@implementation WebFrame (WebPrivate)

// FIXME: This exists only as a convenience for Safari, consider moving there.
- (BOOL)_isDescendantOfFrame:(WebFrame *)ancestor
{
    Frame* coreFrame = _private->coreFrame;
    return coreFrame && coreFrame->tree()->isDescendantOf(core(ancestor));
}

- (void)_setShouldCreateRenderers:(BOOL)shouldCreateRenderers
{
    _private->shouldCreateRenderers = shouldCreateRenderers;
}

- (NSColor *)_bodyBackgroundColor
{
    Document* document = _private->coreFrame->document();
    if (!document)
        return nil;
    HTMLElement* body = document->body();
    if (!body)
        return nil;
    RenderObject* bodyRenderer = body->renderer();
    if (!bodyRenderer)
        return nil;
    Color color = bodyRenderer->style()->visitedDependentColor(CSSPropertyBackgroundColor);
    if (!color.isValid())
        return nil;
    return nsColor(color);
}

- (BOOL)_isFrameSet
{
    Document* document = _private->coreFrame->document();
    return document && document->isFrameSet();
}

- (BOOL)_firstLayoutDone
{
    return _private->coreFrame->loader()->stateMachine()->firstLayoutDone();
}

- (BOOL)_isVisuallyNonEmpty
{
    if (FrameView* view = _private->coreFrame->view())
        return view->isVisuallyNonEmpty();
    return NO;
}

- (WebFrameLoadType)_loadType
{
    return (WebFrameLoadType)_private->coreFrame->loader()->loadType();
}

- (NSRange)_selectedNSRange
{
    return [self _convertToNSRange:_private->coreFrame->selection()->toNormalizedRange().get()];
}

- (void)_selectNSRange:(NSRange)range
{
    RefPtr<Range> domRange = [self _convertToDOMRange:range];
    if (domRange)
        _private->coreFrame->selection()->setSelection(VisibleSelection(domRange.get(), SEL_DEFAULT_AFFINITY));
}

- (BOOL)_isDisplayingStandaloneImage
{
    Document* document = _private->coreFrame->document();
    return document && document->isImageDocument();
}

- (unsigned)_pendingFrameUnloadEventCount
{
    return _private->coreFrame->document()->domWindow()->pendingUnloadEventListeners();
}

#if ENABLE(NETSCAPE_PLUGIN_API)
- (void)_recursive_resumeNullEventsForAllNetscapePlugins
{
    Frame* coreFrame = core(self);
    for (Frame* frame = coreFrame; frame; frame = frame->tree()->traverseNext(coreFrame)) {
        NSView <WebDocumentView> *documentView = [[kit(frame) frameView] documentView];
        if ([documentView isKindOfClass:[WebHTMLView class]])
            [(WebHTMLView *)documentView _resumeNullEventsForAllNetscapePlugins];
    }
}

- (void)_recursive_pauseNullEventsForAllNetscapePlugins
{
    Frame* coreFrame = core(self);
    for (Frame* frame = coreFrame; frame; frame = frame->tree()->traverseNext(coreFrame)) {
        NSView <WebDocumentView> *documentView = [[kit(frame) frameView] documentView];
        if ([documentView isKindOfClass:[WebHTMLView class]])
            [(WebHTMLView *)documentView _pauseNullEventsForAllNetscapePlugins];
    }
}
#endif

- (void)_replaceSelectionWithFragment:(DOMDocumentFragment *)fragment selectReplacement:(BOOL)selectReplacement smartReplace:(BOOL)smartReplace matchStyle:(BOOL)matchStyle
{
    if (_private->coreFrame->selection()->isNone() || !fragment)
        return;
    _private->coreFrame->editor().replaceSelectionWithFragment(core(fragment), selectReplacement, smartReplace, matchStyle);
}

- (void)_replaceSelectionWithText:(NSString *)text selectReplacement:(BOOL)selectReplacement smartReplace:(BOOL)smartReplace
{   
    DOMDocumentFragment* fragment = kit(createFragmentFromText(_private->coreFrame->selection()->toNormalizedRange().get(), text).get());
    [self _replaceSelectionWithFragment:fragment selectReplacement:selectReplacement smartReplace:smartReplace matchStyle:YES];
}

- (void)_replaceSelectionWithMarkupString:(NSString *)markupString baseURLString:(NSString *)baseURLString selectReplacement:(BOOL)selectReplacement smartReplace:(BOOL)smartReplace
{
    DOMDocumentFragment *fragment = [self _documentFragmentWithMarkupString:markupString baseURLString:baseURLString];
    [self _replaceSelectionWithFragment:fragment selectReplacement:selectReplacement smartReplace:smartReplace matchStyle:NO];
}

// Determines whether whitespace needs to be added around aString to preserve proper spacing and
// punctuation when it's inserted into the receiver's text over charRange. Returns by reference
// in beforeString and afterString any whitespace that should be added, unless either or both are
// nil. Both are returned as nil if aString is nil or if smart insertion and deletion are disabled.
- (void)_smartInsertForString:(NSString *)pasteString replacingRange:(DOMRange *)rangeToReplace beforeString:(NSString **)beforeString afterString:(NSString **)afterString
{
    // give back nil pointers in case of early returns
    if (beforeString)
        *beforeString = nil;
    if (afterString)
        *afterString = nil;
        
    // inspect destination
    Node *startContainer = core([rangeToReplace startContainer]);
    Node *endContainer = core([rangeToReplace endContainer]);

    Position startPos(startContainer, [rangeToReplace startOffset], Position::PositionIsOffsetInAnchor);
    Position endPos(endContainer, [rangeToReplace endOffset], Position::PositionIsOffsetInAnchor);

    VisiblePosition startVisiblePos = VisiblePosition(startPos, VP_DEFAULT_AFFINITY);
    VisiblePosition endVisiblePos = VisiblePosition(endPos, VP_DEFAULT_AFFINITY);
    
    // this check also ensures startContainer, startPos, endContainer, and endPos are non-null
    if (startVisiblePos.isNull() || endVisiblePos.isNull())
        return;

    bool addLeadingSpace = startPos.leadingWhitespacePosition(VP_DEFAULT_AFFINITY, true).isNull() && !isStartOfParagraph(startVisiblePos);
    if (addLeadingSpace)
        if (UChar previousChar = startVisiblePos.previous().characterAfter())
            addLeadingSpace = !isCharacterSmartReplaceExempt(previousChar, true);
    
    bool addTrailingSpace = endPos.trailingWhitespacePosition(VP_DEFAULT_AFFINITY, true).isNull() && !isEndOfParagraph(endVisiblePos);
    if (addTrailingSpace)
        if (UChar thisChar = endVisiblePos.characterAfter())
            addTrailingSpace = !isCharacterSmartReplaceExempt(thisChar, false);
    
    // inspect source
    bool hasWhitespaceAtStart = false;
    bool hasWhitespaceAtEnd = false;
    unsigned pasteLength = [pasteString length];
    if (pasteLength > 0) {
        NSCharacterSet *whiteSet = [NSCharacterSet whitespaceAndNewlineCharacterSet];
        
        if ([whiteSet characterIsMember:[pasteString characterAtIndex:0]]) {
            hasWhitespaceAtStart = YES;
        }
        if ([whiteSet characterIsMember:[pasteString characterAtIndex:(pasteLength - 1)]]) {
            hasWhitespaceAtEnd = YES;
        }
    }
    
    // issue the verdict
    if (beforeString && addLeadingSpace && !hasWhitespaceAtStart)
        *beforeString = @" ";
    if (afterString && addTrailingSpace && !hasWhitespaceAtEnd)
        *afterString = @" ";
}

- (NSMutableDictionary *)_cacheabilityDictionary
{
    NSMutableDictionary *result = [NSMutableDictionary dictionary];
    
    FrameLoader* frameLoader = _private->coreFrame->loader();
    DocumentLoader* documentLoader = frameLoader->documentLoader();
    if (documentLoader && !documentLoader->mainDocumentError().isNull())
        [result setObject:(NSError *)documentLoader->mainDocumentError() forKey:WebFrameMainDocumentError];
        
    if (frameLoader->subframeLoader()->containsPlugins())
        [result setObject:[NSNumber numberWithBool:YES] forKey:WebFrameHasPlugins];
    
    if (DOMWindow* domWindow = _private->coreFrame->document()->domWindow()) {
        if (domWindow->hasEventListeners(eventNames().unloadEvent))
            [result setObject:[NSNumber numberWithBool:YES] forKey:WebFrameHasUnloadListener];
        if (domWindow->optionalApplicationCache())
            [result setObject:[NSNumber numberWithBool:YES] forKey:WebFrameUsesApplicationCache];
    }
    
    if (Document* document = _private->coreFrame->document()) {
#if ENABLE(SQL_DATABASE)
        if (DatabaseManager::manager().hasOpenDatabases(document))
            [result setObject:[NSNumber numberWithBool:YES] forKey:WebFrameUsesDatabases];
#endif
        if (!document->canSuspendActiveDOMObjects())
            [result setObject:[NSNumber numberWithBool:YES] forKey:WebFrameCanSuspendActiveDOMObjects];
    }
    
    return result;
}

- (BOOL)_allowsFollowingLink:(NSURL *)URL
{
    if (!_private->coreFrame)
        return YES;
    return _private->coreFrame->document()->securityOrigin()->canDisplay(URL);
}

- (NSString *)_stringByEvaluatingJavaScriptFromString:(NSString *)string withGlobalObject:(JSObjectRef)globalObjectRef inScriptWorld:(WebScriptWorld *)world
{
    if (!string)
        return @"";

    // Start off with some guess at a frame and a global object, we'll try to do better...!
    JSDOMWindow* anyWorldGlobalObject = _private->coreFrame->script()->globalObject(mainThreadNormalWorld());

    // The global object is probably a shell object? - if so, we know how to use this!
    JSC::JSObject* globalObjectObj = toJS(globalObjectRef);
    if (!strcmp(globalObjectObj->classInfo()->className, "JSDOMWindowShell"))
        anyWorldGlobalObject = static_cast<JSDOMWindowShell*>(globalObjectObj)->window();

    // Get the frame frome the global object we've settled on.
    Frame* frame = anyWorldGlobalObject->impl()->frame();
    ASSERT(frame->document());
    RetainPtr<WebFrame> webFrame(kit(frame)); // Running arbitrary JavaScript can destroy the frame.

    JSC::JSValue result = frame->script()->executeScriptInWorld(core(world), string, true).jsValue();

    if (!webFrame->_private->coreFrame) // In case the script removed our frame from the page.
        return @"";

    // This bizarre set of rules matches behavior from WebKit for Safari 2.0.
    // If you don't like it, use -[WebScriptObject evaluateWebScript:] or 
    // JSEvaluateScript instead, since they have less surprising semantics.
    if (!result || (!result.isBoolean() && !result.isString() && !result.isNumber()))
        return @"";

    JSC::ExecState* exec = anyWorldGlobalObject->globalExec();
    JSC::JSLockHolder lock(exec);
    return result.toWTFString(exec);
}

- (JSGlobalContextRef)_globalContextForScriptWorld:(WebScriptWorld *)world
{
    Frame* coreFrame = _private->coreFrame;
    if (!coreFrame)
        return 0;
    DOMWrapperWorld* coreWorld = core(world);
    if (!coreWorld)
        return 0;
    return toGlobalRef(coreFrame->script()->globalObject(coreWorld)->globalExec());
}

#if JSC_OBJC_API_ENABLED
- (JSContext *)_javaScriptContextForScriptWorld:(WebScriptWorld *)world
{
    JSGlobalContextRef globalContextRef = [self _globalContextForScriptWorld:world];
    if (!globalContextRef)
        return 0;
    return [JSContext contextWithJSGlobalContextRef:globalContextRef];
}
#endif

- (void)setAllowsScrollersToOverlapContent:(BOOL)flag
{
    ASSERT([[[self frameView] _scrollView] isKindOfClass:[WebDynamicScrollBarsView class]]);
    [(WebDynamicScrollBarsView *)[[self frameView] _scrollView] setAllowsScrollersToOverlapContent:flag];
}

- (void)setAlwaysHideHorizontalScroller:(BOOL)flag
{
    ASSERT([[[self frameView] _scrollView] isKindOfClass:[WebDynamicScrollBarsView class]]);
    [(WebDynamicScrollBarsView *)[[self frameView] _scrollView] setAlwaysHideHorizontalScroller:flag];
}
- (void)setAlwaysHideVerticalScroller:(BOOL)flag
{
    ASSERT([[[self frameView] _scrollView] isKindOfClass:[WebDynamicScrollBarsView class]]);
    [(WebDynamicScrollBarsView *)[[self frameView] _scrollView] setAlwaysHideVerticalScroller:flag];
}

- (void)setAccessibleName:(NSString *)name
{
#if HAVE(ACCESSIBILITY)
    if (!AXObjectCache::accessibilityEnabled())
        return;
    
    if (!_private->coreFrame || !_private->coreFrame->document())
        return;
    
    AccessibilityObject* rootObject = _private->coreFrame->document()->axObjectCache()->rootObject();
    if (rootObject) {
        String strName(name);
        rootObject->setAccessibleName(strName);
    }
#endif
}

- (NSString*)_layerTreeAsText
{
    Frame* coreFrame = _private->coreFrame;
    if (!coreFrame)
        return @"";

    return coreFrame->layerTreeAsText();
}

- (id)accessibilityRoot
{
#if HAVE(ACCESSIBILITY)
    if (!AXObjectCache::accessibilityEnabled()) {
        AXObjectCache::enableAccessibility();
        AXObjectCache::setEnhancedUserInterfaceAccessibility([[NSApp accessibilityAttributeValue:NSAccessibilityEnhancedUserInterfaceAttribute] boolValue]);
    }
    
    if (!_private->coreFrame || !_private->coreFrame->document())
        return nil;
    
    AccessibilityObject* rootObject = _private->coreFrame->document()->axObjectCache()->rootObjectForFrame(_private->coreFrame);
    if (!rootObject)
        return nil;
    
    // The root object will be a WebCore scroll view object. In WK1, scroll views are handled
    // by the system and the root object should be the web area (instead of the scroll view).
    if (rootObject->isAttachment() && rootObject->firstChild())
        return rootObject->firstChild()->wrapper();
    
    return rootObject->wrapper();
#else
    return nil;
#endif
}

- (void)_clearOpener
{
    Frame* coreFrame = _private->coreFrame;
    if (coreFrame)
        coreFrame->loader()->setOpener(0);
}

// Used by pagination code called from AppKit when a standalone web page is printed.
- (NSArray *)_computePageRectsWithPrintScaleFactor:(float)printScaleFactor pageSize:(NSSize)pageSize
{
    if (printScaleFactor <= 0) {
        LOG_ERROR("printScaleFactor has bad value %.2f", printScaleFactor);
        return [NSArray array];
    }

    if (!_private->coreFrame)
        return [NSArray array];
    if (!_private->coreFrame->document())
        return [NSArray array];
    if (!_private->coreFrame->view())
        return [NSArray array];
    if (!_private->coreFrame->view()->documentView())
        return [NSArray array];

    RenderView* root = toRenderView(_private->coreFrame->document()->renderer());
    if (!root)
        return [NSArray array];

    const LayoutRect& documentRect = root->documentRect();
    float printWidth = root->style()->isHorizontalWritingMode() ? static_cast<float>(documentRect.width()) / printScaleFactor : pageSize.width;
    float printHeight = root->style()->isHorizontalWritingMode() ? pageSize.height : static_cast<float>(documentRect.height()) / printScaleFactor;

    PrintContext printContext(_private->coreFrame);
    printContext.computePageRectsWithPageSize(FloatSize(printWidth, printHeight), true);
    const Vector<IntRect>& pageRects = printContext.pageRects();

    size_t size = pageRects.size();
    NSMutableArray *pages = [NSMutableArray arrayWithCapacity:size];
    for (size_t i = 0; i < size; ++i)
        [pages addObject:[NSValue valueWithRect:NSRect(pageRects[i])]];
    return pages;
}

- (JSValueRef)jsWrapperForNode:(DOMNode *)node inScriptWorld:(WebScriptWorld *)world
{
    Frame* coreFrame = _private->coreFrame;
    if (!coreFrame)
        return 0;

    JSDOMWindow* globalObject = coreFrame->script()->globalObject(core(world));
    JSC::ExecState* exec = globalObject->globalExec();

    JSC::JSLockHolder lock(exec);
    return toRef(exec, toJS(exec, globalObject, core(node)));
}

- (NSDictionary *)elementAtPoint:(NSPoint)point
{
    Frame* coreFrame = _private->coreFrame;
    if (!coreFrame)
        return nil;
    return [[[WebElementDictionary alloc] initWithHitTestResult:coreFrame->eventHandler()->hitTestResultAtPoint(IntPoint(point), HitTestRequest::ReadOnly | HitTestRequest::Active | HitTestRequest::IgnoreClipping | HitTestRequest::DisallowShadowContent)] autorelease];
}

- (NSURL *)_unreachableURL
{
    return [[self _dataSource] unreachableURL];
}

@end

@implementation WebFrame

- (id)init
{
    return nil;
}

// Should be deprecated.
- (id)initWithName:(NSString *)name webFrameView:(WebFrameView *)view webView:(WebView *)webView
{
    return nil;
}

- (void)dealloc
{
    if (_private && _private->includedInWebKitStatistics)
        --WebFrameCount;

    [_private release];

    [super dealloc];
}

- (void)finalize
{
    if (_private && _private->includedInWebKitStatistics)
        --WebFrameCount;

    [super finalize];
}

- (NSString *)name
{
    Frame* coreFrame = _private->coreFrame;
    if (!coreFrame)
        return nil;
    return coreFrame->tree()->uniqueName();
}

- (WebFrameView *)frameView
{
    return _private->webFrameView;
}

- (WebView *)webView
{
    return getWebView(self);
}

static bool needsMicrosoftMessengerDOMDocumentWorkaround()
{
    static bool needsWorkaround = applicationIsMicrosoftMessenger() && [[[NSBundle mainBundle] objectForInfoDictionaryKey:(NSString *)kCFBundleVersionKey] compare:@"7.1" options:NSNumericSearch] == NSOrderedAscending;
    return needsWorkaround;
}

- (DOMDocument *)DOMDocument
{
    if (needsMicrosoftMessengerDOMDocumentWorkaround() && !pthread_main_np())
        return nil;

    Frame* coreFrame = _private->coreFrame;
    if (!coreFrame)
        return nil;
    
    // FIXME: <rdar://problem/5145841> When loading a custom view/representation 
    // into a web frame, the old document can still be around. This makes sure that
    // we'll return nil in those cases.
    if (![[self _dataSource] _isDocumentHTML]) 
        return nil; 

    Document* document = coreFrame->document();
    
    // According to the documentation, we should return nil if the frame doesn't have a document.
    // While full-frame images and plugins do have an underlying HTML document, we return nil here to be
    // backwards compatible.
    if (document && (document->isPluginDocument() || document->isImageDocument()))
        return nil;
    
    return kit(coreFrame->document());
}

- (DOMHTMLElement *)frameElement
{
    Frame* coreFrame = _private->coreFrame;
    if (!coreFrame)
        return nil;
    return kit(coreFrame->ownerElement());
}

- (WebDataSource *)provisionalDataSource
{
    Frame* coreFrame = _private->coreFrame;
    return coreFrame ? dataSource(coreFrame->loader()->provisionalDocumentLoader()) : nil;
}

- (WebDataSource *)dataSource
{
    Frame* coreFrame = _private->coreFrame;
    return coreFrame && coreFrame->loader()->frameHasLoaded() ? [self _dataSource] : nil;
}

- (void)loadRequest:(NSURLRequest *)request
{
    Frame* coreFrame = _private->coreFrame;
    if (!coreFrame)
        return;

    ResourceRequest resourceRequest(request);
    
    // Some users of WebKit API incorrectly use "file path as URL" style requests which are invalid.
    // By re-writing those URLs here we technically break the -[WebDataSource initialRequest] API
    // but that is necessary to implement this quirk only at the API boundary.
    // Note that other users of WebKit API use nil requests or requests with nil URLs or empty URLs, so we
    // only implement this workaround when the request had a non-nil or non-empty URL.
    if (!resourceRequest.url().isValid() && !resourceRequest.url().isEmpty())
        resourceRequest.setURL([NSURL URLWithString:[@"file:" stringByAppendingString:[[request URL] absoluteString]]]);

    coreFrame->loader()->load(FrameLoadRequest(coreFrame, resourceRequest));
}

static NSURL *createUniqueWebDataURL()
{
    CFUUIDRef UUIDRef = CFUUIDCreate(kCFAllocatorDefault);
    NSString *UUIDString = (NSString *)CFUUIDCreateString(kCFAllocatorDefault, UUIDRef);
    CFRelease(UUIDRef);
    NSURL *URL = [NSURL URLWithString:[NSString stringWithFormat:@"applewebdata://%@", UUIDString]];
    CFRelease(UUIDString);
    return URL;
}

- (void)_loadData:(NSData *)data MIMEType:(NSString *)MIMEType textEncodingName:(NSString *)encodingName baseURL:(NSURL *)baseURL unreachableURL:(NSURL *)unreachableURL
{
    if (!pthread_main_np())
        return [[self _webkit_invokeOnMainThread] _loadData:data MIMEType:MIMEType textEncodingName:encodingName baseURL:baseURL unreachableURL:unreachableURL];
    
    KURL responseURL;
    if (!baseURL) {
        baseURL = blankURL();
        responseURL = createUniqueWebDataURL();
    }
    
    ResourceRequest request([baseURL absoluteURL]);

    // hack because Mail checks for this property to detect data / archive loads
    [NSURLProtocol setProperty:@"" forKey:@"WebDataRequest" inRequest:(NSMutableURLRequest *)request.nsURLRequest(UpdateHTTPBody)];

    SubstituteData substituteData(WebCore::SharedBuffer::wrapNSData(data), MIMEType, encodingName, [unreachableURL absoluteURL], responseURL);

    _private->coreFrame->loader()->load(FrameLoadRequest(_private->coreFrame, request, substituteData));
}


- (void)loadData:(NSData *)data MIMEType:(NSString *)MIMEType textEncodingName:(NSString *)encodingName baseURL:(NSURL *)baseURL
{
    WebCoreThreadViolationCheckRoundTwo();
    
    if (!MIMEType)
        MIMEType = @"text/html";
    [self _loadData:data MIMEType:MIMEType textEncodingName:encodingName baseURL:[baseURL _webkit_URLFromURLOrSchemelessFileURL] unreachableURL:nil];
}

- (void)_loadHTMLString:(NSString *)string baseURL:(NSURL *)baseURL unreachableURL:(NSURL *)unreachableURL
{
    NSData *data = [string dataUsingEncoding:NSUTF8StringEncoding];
    [self _loadData:data MIMEType:@"text/html" textEncodingName:@"UTF-8" baseURL:baseURL unreachableURL:unreachableURL];
}

- (void)loadHTMLString:(NSString *)string baseURL:(NSURL *)baseURL
{
    WebCoreThreadViolationCheckRoundTwo();

    [self _loadHTMLString:string baseURL:[baseURL _webkit_URLFromURLOrSchemelessFileURL] unreachableURL:nil];
}

- (void)loadAlternateHTMLString:(NSString *)string baseURL:(NSURL *)baseURL forUnreachableURL:(NSURL *)unreachableURL
{
    WebCoreThreadViolationCheckRoundTwo();

    [self _loadHTMLString:string baseURL:[baseURL _webkit_URLFromURLOrSchemelessFileURL] unreachableURL:[unreachableURL _webkit_URLFromURLOrSchemelessFileURL]];
}

- (void)loadArchive:(WebArchive *)archive
{
    if (LegacyWebArchive* coreArchive = [archive _coreLegacyWebArchive])
        _private->coreFrame->loader()->loadArchive(coreArchive);
}

- (void)stopLoading
{
    if (!_private->coreFrame)
        return;
    _private->coreFrame->loader()->stopForUserCancel();
}

- (void)reload
{
    if (!WebKitLinkedOnOrAfter(WEBKIT_FIRST_VERSION_WITH_RELOAD_FROM_ORIGIN) && applicationIsSafari())
        _private->coreFrame->loader()->reload(GetCurrentKeyModifiers() & shiftKey);
    else
        _private->coreFrame->loader()->reload(false);
}

- (void)reloadFromOrigin
{
    _private->coreFrame->loader()->reload(true);
}

- (WebFrame *)findFrameNamed:(NSString *)name
{
    Frame* coreFrame = _private->coreFrame;
    if (!coreFrame)
        return nil;
    return kit(coreFrame->tree()->find(name));
}

- (WebFrame *)parentFrame
{
    Frame* coreFrame = _private->coreFrame;
    if (!coreFrame)
        return nil;
    return [[kit(coreFrame->tree()->parent()) retain] autorelease];
}

- (NSArray *)childFrames
{
    Frame* coreFrame = _private->coreFrame;
    if (!coreFrame)
        return [NSArray array];
    NSMutableArray *children = [NSMutableArray arrayWithCapacity:coreFrame->tree()->childCount()];
    for (Frame* child = coreFrame->tree()->firstChild(); child; child = child->tree()->nextSibling())
        [children addObject:kit(child)];
    return children;
}

- (WebScriptObject *)windowObject
{
    Frame* coreFrame = _private->coreFrame;
    if (!coreFrame)
        return 0;
    return coreFrame->script()->windowScriptObject();
}

- (JSGlobalContextRef)globalContext
{
    Frame* coreFrame = _private->coreFrame;
    if (!coreFrame)
        return 0;
    return toGlobalRef(coreFrame->script()->globalObject(mainThreadNormalWorld())->globalExec());
}

#if JSC_OBJC_API_ENABLED
- (JSContext *)javaScriptContext
{
    Frame* coreFrame = _private->coreFrame;
    if (!coreFrame)
        return 0;
    return coreFrame->script()->javaScriptContext();
}
#endif

@end
