/*
 * Copyright (C) 2005, 2006, 2007, 2008, 2009, 2013 Apple Inc. All rights reserved.
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

#import "WebPDFView.h"

#import "DOMNodeInternal.h"
#import "DOMRangeInternal.h"
#import "WebDataSourceInternal.h"
#import "WebDelegateImplementationCaching.h"
#import "WebDocumentInternal.h"
#import "WebDocumentPrivate.h"
#import "WebFrame.h"
#import "WebFrameInternal.h"
#import "WebFrameView.h"
#import "WebLocalizableStringsInternal.h"
#import "WebNSArrayExtras.h"
#import "WebNSPasteboardExtras.h"
#import "WebNSViewExtras.h"
#import "WebPDFRepresentation.h"
#import "WebPreferencesPrivate.h"
#import "WebUIDelegate.h"
#import "WebUIDelegatePrivate.h"
#import "WebView.h"
#import "WebViewInternal.h"
#import <PDFKit/PDFKit.h>
#import <WebCore/Clipboard.h>
#import <WebCore/EventNames.h>
#import <WebCore/FormState.h>
#import <WebCore/Frame.h>
#import <WebCore/FrameLoadRequest.h>
#import <WebCore/FrameLoader.h>
#import <WebCore/HTMLFormElement.h>
#import <WebCore/HTMLFrameOwnerElement.h>
#import <WebCore/KURL.h>
#import <WebCore/KeyboardEvent.h>
#import <WebCore/MouseEvent.h>
#import <WebCore/PlatformEventFactoryMac.h>
#import <WebCore/RuntimeApplicationChecks.h>
#import <WebCore/WebNSAttributedStringExtras.h>
#import <wtf/Assertions.h>
#import <wtf/CurrentTime.h>

using namespace WebCore;

// Redeclarations of PDFKit notifications. We can't use the API since we use a weak link to the framework.
#define _webkit_PDFViewDisplayModeChangedNotification @"PDFViewDisplayModeChanged"
#define _webkit_PDFViewScaleChangedNotification @"PDFViewScaleChanged"
#define _webkit_PDFViewPageChangedNotification @"PDFViewChangedPage"

@interface PDFDocument (PDFKitSecretsIKnow)
- (NSPrintOperation *)getPrintOperationForPrintInfo:(NSPrintInfo *)printInfo autoRotate:(BOOL)doRotate;
@end

extern "C" NSString *_NSPathForSystemFramework(NSString *framework);

@interface WebPDFView (FileInternal)
+ (Class)_PDFPreviewViewClass;
+ (Class)_PDFViewClass;
- (BOOL)_anyPDFTagsFoundInMenu:(NSMenu *)menu;
- (void)_applyPDFDefaults;
- (BOOL)_canLookUpInDictionary;
- (NSClipView *)_clipViewForPDFDocumentView;
- (NSEvent *)_fakeKeyEventWithFunctionKey:(unichar)functionKey;
- (NSMutableArray *)_menuItemsFromPDFKitForEvent:(NSEvent *)theEvent;
- (PDFSelection *)_nextMatchFor:(NSString *)string direction:(BOOL)forward caseSensitive:(BOOL)caseFlag wrap:(BOOL)wrapFlag fromSelection:(PDFSelection *)initialSelection startInSelection:(BOOL)startInSelection;
- (void)_openWithFinder:(id)sender;
- (NSString *)_path;
- (void)_PDFDocumentViewMightHaveScrolled:(NSNotification *)notification;
- (BOOL)_pointIsInSelection:(NSPoint)point;
- (NSAttributedString *)_scaledAttributedString:(NSAttributedString *)unscaledAttributedString;
- (void)_setTextMatches:(NSArray *)array;
- (NSString *)_temporaryPDFDirectoryPath;
- (void)_trackFirstResponder;
- (void)_updatePreferencesSoon;
- (NSSet *)_visiblePDFPages;
@end;

// WebPDFPrefUpdatingProxy is a class that forwards everything it gets to a target and updates the PDF viewing prefs
// after each of those messages.  We use it as a way to hook all the places that the PDF viewing attrs change.
@interface WebPDFPrefUpdatingProxy : NSProxy {
    WebPDFView *view;
}
- (id)initWithView:(WebPDFView *)view;
@end

// MARK: C UTILITY FUNCTIONS

static void _applicationInfoForMIMEType(NSString *type, NSString **name, NSImage **image)
{
    NSURL *appURL = nil;
    
    OSStatus error = LSCopyApplicationForMIMEType((CFStringRef)type, kLSRolesAll, (CFURLRef *)&appURL);
    if (error != noErr)
        return;
    
    NSString *appPath = [appURL path];
    CFRelease (appURL);
    
    *image = [[NSWorkspace sharedWorkspace] iconForFile:appPath];  
    [*image setSize:NSMakeSize(16.f,16.f)];  
    
    NSString *appName = [[NSFileManager defaultManager] displayNameAtPath:appPath];
    *name = appName;
}

// FIXME 4182876: We can eliminate this function in favor if -isEqual: if [PDFSelection isEqual:] is overridden
// to compare contents.
static BOOL _PDFSelectionsAreEqual(PDFSelection *selectionA, PDFSelection *selectionB)
{
    NSArray *aPages = [selectionA pages];
    NSArray *bPages = [selectionB pages];
    
    if (![aPages isEqual:bPages])
        return NO;
    
    int count = [aPages count];
    int i;
    for (i = 0; i < count; ++i) {
        NSRect aBounds = [selectionA boundsForPage:[aPages objectAtIndex:i]];
        NSRect bBounds = [selectionB boundsForPage:[bPages objectAtIndex:i]];
        if (!NSEqualRects(aBounds, bBounds)) {
            return NO;
        }
    }
    
    return YES;
}

@implementation WebPDFView

// MARK: WebPDFView API

+ (NSBundle *)PDFKitBundle
{
    static NSBundle *PDFKitBundle = nil;
    if (PDFKitBundle == nil) {
        NSString *PDFKitPath = [_NSPathForSystemFramework(@"Quartz.framework") stringByAppendingString:@"/Frameworks/PDFKit.framework"];
        if (PDFKitPath == nil) {
            LOG_ERROR("Couldn't find PDFKit.framework");
            return nil;
        }
        PDFKitBundle = [NSBundle bundleWithPath:PDFKitPath];
        if (![PDFKitBundle load]) {
            LOG_ERROR("Couldn't load PDFKit.framework");
        }
    }
    return PDFKitBundle;
}

+ (NSArray *)supportedMIMETypes
{
    return [WebPDFRepresentation supportedMIMETypes];
}

- (void)setPDFDocument:(PDFDocument *)doc
{
    // Both setDocument: and _applyPDFDefaults will trigger scale and mode-changed notifications.
    // Those aren't reflecting user actions, so we need to ignore them.
    _ignoreScaleAndDisplayModeAndPageNotifications = YES;
    [PDFSubview setDocument:doc];
    [self _applyPDFDefaults];
    _ignoreScaleAndDisplayModeAndPageNotifications = NO;
}

- (PDFDocument *)PDFDocument
{
    return [PDFSubview document];
}

// MARK: NSObject OVERRIDES

- (void)dealloc
{
    [dataSource release];
    [previewView release];
    [PDFSubview setDelegate:nil];
    [PDFSubview release];
    [path release];
    [PDFSubviewProxy release];
    [textMatches release];
    [super dealloc];
}

// MARK: NSResponder OVERRIDES

- (void)centerSelectionInVisibleArea:(id)sender
{
    [PDFSubview scrollSelectionToVisible:nil];
}

- (void)scrollPageDown:(id)sender
{
    // PDFView doesn't support this responder method directly, so we pass it a fake key event
    [PDFSubview keyDown:[self _fakeKeyEventWithFunctionKey:NSPageDownFunctionKey]];
}

- (void)scrollPageUp:(id)sender
{
    // PDFView doesn't support this responder method directly, so we pass it a fake key event
    [PDFSubview keyDown:[self _fakeKeyEventWithFunctionKey:NSPageUpFunctionKey]];
}

- (void)scrollLineDown:(id)sender
{
    // PDFView doesn't support this responder method directly, so we pass it a fake key event
    [PDFSubview keyDown:[self _fakeKeyEventWithFunctionKey:NSDownArrowFunctionKey]];
}

- (void)scrollLineUp:(id)sender
{
    // PDFView doesn't support this responder method directly, so we pass it a fake key event
    [PDFSubview keyDown:[self _fakeKeyEventWithFunctionKey:NSUpArrowFunctionKey]];
}

- (void)scrollToBeginningOfDocument:(id)sender
{
    // PDFView doesn't support this responder method directly, so we pass it a fake key event
    [PDFSubview keyDown:[self _fakeKeyEventWithFunctionKey:NSHomeFunctionKey]];
}

- (void)scrollToEndOfDocument:(id)sender
{
    // PDFView doesn't support this responder method directly, so we pass it a fake key event
    [PDFSubview keyDown:[self _fakeKeyEventWithFunctionKey:NSEndFunctionKey]];
}

// jumpToSelection is the old name for what AppKit now calls centerSelectionInVisibleArea. Safari
// was using the old jumpToSelection selector in its menu. Newer versions of Safari will us the
// selector centerSelectionInVisibleArea. We'll leave this old selector in place for two reasons:
// (1) compatibility between older Safari and newer WebKit; (2) other WebKit-based applications
// might be using the jumpToSelection: selector, and we don't want to break them.
- (void)jumpToSelection:(id)sender
{
    [self centerSelectionInVisibleArea:nil];
}

// MARK: NSView OVERRIDES

- (BOOL)acceptsFirstResponder {
    return YES;
}

- (BOOL)becomeFirstResponder
{
    // This works together with setNextKeyView to splice our PDFSubview into
    // the key loop similar to the way NSScrollView does this.
    NSWindow *window = [self window];
    id newFirstResponder = nil;
    
    if ([window keyViewSelectionDirection] == NSSelectingPrevious) {
        NSView *previousValidKeyView = [self previousValidKeyView];
        if ((previousValidKeyView != self) && (previousValidKeyView != PDFSubview))
            newFirstResponder = previousValidKeyView;
    } else {
        NSView *PDFDocumentView = [PDFSubview documentView];
        if ([PDFDocumentView acceptsFirstResponder])
            newFirstResponder = PDFDocumentView;
    }
    
    if (!newFirstResponder)
        return NO;
    
    if (![window makeFirstResponder:newFirstResponder])
        return NO;
    
    [[dataSource webFrame] _clearSelectionInOtherFrames];
    
    return YES;
}

- (NSView *)hitTest:(NSPoint)point
{
    // Override hitTest so we can override menuForEvent.
    NSEvent *event = [NSApp currentEvent];
    NSEventType type = [event type];
    if (type == NSRightMouseDown || (type == NSLeftMouseDown && ([event modifierFlags] & NSControlKeyMask)))
        return self;

    return [super hitTest:point];
}

- (id)initWithFrame:(NSRect)frame
{
    self = [super initWithFrame:frame];
    if (self) {
        [self setAutoresizingMask:NSViewWidthSizable | NSViewHeightSizable];
        
        Class previewViewClass = [[self class] _PDFPreviewViewClass];
        
        // We might not have found a previewViewClass, but if we did find it
        // then we should be able to create an instance.
        if (previewViewClass) {
            previewView = [[previewViewClass alloc] initWithFrame:frame];
            ASSERT(previewView);
        }
        
        NSView *topLevelPDFKitView = nil;
        if (previewView) {
            // We'll retain the PDFSubview here so that it is equally retained in all
            // code paths. That way we don't need to worry about conditionally releasing
            // it later.
            PDFSubview = [[previewView performSelector:@selector(pdfView)] retain];
            topLevelPDFKitView = previewView;
        } else {
            PDFSubview = [[[[self class] _PDFViewClass] alloc] initWithFrame:frame];
            topLevelPDFKitView = PDFSubview;
        }
        
        ASSERT(PDFSubview);
        
        [topLevelPDFKitView setAutoresizingMask:NSViewWidthSizable | NSViewHeightSizable];
        [self addSubview:topLevelPDFKitView];
        
        [PDFSubview setDelegate:self];
        written = NO;
        // Messaging this proxy is the same as messaging PDFSubview, with the side effect that the
        // PDF viewing defaults are updated afterwards
        PDFSubviewProxy = (PDFView *)[[WebPDFPrefUpdatingProxy alloc] initWithView:self];
    }
    
    return self;
}

- (NSMenu *)menuForEvent:(NSEvent *)theEvent
{
    // Start with the menu items supplied by PDFKit, with WebKit tags applied
    NSMutableArray *items = [self _menuItemsFromPDFKitForEvent:theEvent];
    
    // Add in an "Open with <default PDF viewer>" item
    NSString *appName = nil;
    NSImage *appIcon = nil;
    
    _applicationInfoForMIMEType([dataSource _responseMIMEType], &appName, &appIcon);
    if (!appName)
        appName = UI_STRING_INTERNAL("Finder", "Default application name for Open With context menu");
    
    // To match the PDFKit style, we'll add Open with Preview even when there's no document yet to view, and
    // disable it using validateUserInterfaceItem.
    NSString *title = [NSString stringWithFormat:UI_STRING_INTERNAL("Open with %@", "context menu item for PDF"), appName];
    NSMenuItem *item = [[NSMenuItem alloc] initWithTitle:title action:@selector(_openWithFinder:) keyEquivalent:@""];
    [item setTag:WebMenuItemTagOpenWithDefaultApplication];
    if (appIcon)
        [item setImage:appIcon];
    [items insertObject:item atIndex:0];
    [item release];
    
    [items insertObject:[NSMenuItem separatorItem] atIndex:1];
    
    // pass the items off to the WebKit context menu mechanism
    WebView *webView = [[dataSource webFrame] webView];
    ASSERT(webView);
    NSMenu *menu = [webView _menuForElement:[self elementAtPoint:[self convertPoint:[theEvent locationInWindow] fromView:nil]] defaultItems:items];
    
    // The delegate has now had the opportunity to add items to the standard PDF-related items, or to
    // remove or modify some of the PDF-related items. In 10.4, the PDF context menu did not go through 
    // the standard WebKit delegate path, and so the standard PDF-related items always appeared. For
    // clients that create their own context menu by hand-picking specific items from the default list, such as
    // Safari, none of the PDF-related items will appear until the client is rewritten to explicitly
    // include these items. For backwards compatibility of tip-of-tree WebKit with the 10.4 version of Safari
    // (the configuration that people building open source WebKit use), we'll use the entire set of PDFKit-supplied
    // menu items. This backward-compatibility hack won't work with any non-Safari clients, but this seems OK since
    // (1) the symptom is fairly minor, and (2) we suspect that non-Safari clients are probably using the entire
    // set of default items, rather than manually choosing from them. We can remove this code entirely when we
    // ship a version of Safari that includes the fix for radar 3796579.
    if (![self _anyPDFTagsFoundInMenu:menu] && applicationIsSafari()) {
        [menu addItem:[NSMenuItem separatorItem]];
        NSEnumerator *e = [items objectEnumerator];
        NSMenuItem *menuItem;
        while ((menuItem = [e nextObject]) != nil) {
            // copy menuItem since a given menuItem can be in only one menu at a time, and we don't
            // want to mess with the menu returned from PDFKit.
            NSMenuItem *menuItemCopy = [menuItem copy];
            [menu addItem:menuItemCopy];
            [menuItemCopy release];
        }
    }
    
    return menu;
}

- (void)setNextKeyView:(NSView *)aView
{
    // This works together with becomeFirstResponder to splice PDFSubview into
    // the key loop similar to the way NSScrollView and NSClipView do this.
    NSView *documentView = [PDFSubview documentView];
    if (documentView) {
        [documentView setNextKeyView:aView];
        
        // We need to make the documentView be the next view in the keyview loop.
        // It would seem more sensible to do this in our init method, but it turns out
        // that [NSClipView setDocumentView] won't call this method if our next key view
        // is already set, so we wait until we're called before adding this connection.
        // We'll also clear it when we're called with nil, so this could go through the
        // same code path more than once successfully.
        [super setNextKeyView: aView ? documentView : nil];
    } else
        [super setNextKeyView:aView];
}

- (void)viewDidMoveToWindow
{
    // FIXME 2573089: we can observe a notification for first responder changes
    // instead of the very frequent NSWindowDidUpdateNotification if/when 2573089 is addressed.
    NSWindow *newWindow = [self window];
    if (!newWindow)
        return;
    
    [self _trackFirstResponder];
    NSNotificationCenter *notificationCenter = [NSNotificationCenter defaultCenter];
    [notificationCenter addObserver:self
                           selector:@selector(_trackFirstResponder) 
                               name:NSWindowDidUpdateNotification
                             object:newWindow];
    
    [notificationCenter addObserver:self
                           selector:@selector(_scaleOrDisplayModeOrPageChanged:) 
                               name:_webkit_PDFViewScaleChangedNotification
                             object:PDFSubview];
    
    [notificationCenter addObserver:self
                           selector:@selector(_scaleOrDisplayModeOrPageChanged:) 
                               name:_webkit_PDFViewDisplayModeChangedNotification
                             object:PDFSubview];
    
    [notificationCenter addObserver:self
                           selector:@selector(_scaleOrDisplayModeOrPageChanged:) 
                               name:_webkit_PDFViewPageChangedNotification
                             object:PDFSubview];
    
    [notificationCenter addObserver:self 
                           selector:@selector(_PDFDocumentViewMightHaveScrolled:)
                               name:NSViewBoundsDidChangeNotification 
                             object:[self _clipViewForPDFDocumentView]];
}

- (void)viewWillMoveToWindow:(NSWindow *)window
{
    // FIXME 2573089: we can observe a notification for changes to the first responder
    // instead of the very frequent NSWindowDidUpdateNotification if/when 2573089 is addressed.
    NSWindow *oldWindow = [self window];
    if (!oldWindow)
        return;
    
    NSNotificationCenter *notificationCenter = [NSNotificationCenter defaultCenter];
    [notificationCenter removeObserver:self
                                  name:NSWindowDidUpdateNotification
                                object:oldWindow];
    [notificationCenter removeObserver:self
                                  name:_webkit_PDFViewScaleChangedNotification
                                object:PDFSubview];
    [notificationCenter removeObserver:self
                                  name:_webkit_PDFViewDisplayModeChangedNotification
                                object:PDFSubview];
    [notificationCenter removeObserver:self
                                  name:_webkit_PDFViewPageChangedNotification
                                object:PDFSubview];
    
    [notificationCenter removeObserver:self
                                  name:NSViewBoundsDidChangeNotification 
                                object:[self _clipViewForPDFDocumentView]];
    
    firstResponderIsPDFDocumentView = NO;
}

// MARK: NSUserInterfaceValidations PROTOCOL IMPLEMENTATION

- (BOOL)validateUserInterfaceItemWithoutDelegate:(id <NSValidatedUserInterfaceItem>)item
{
    SEL action = [item action];    
    if (action == @selector(takeFindStringFromSelection:) || action == @selector(centerSelectionInVisibleArea:) || action == @selector(jumpToSelection:))
        return [PDFSubview currentSelection] != nil;
    
    if (action == @selector(_openWithFinder:))
        return [PDFSubview document] != nil;
    
    if (action == @selector(_lookUpInDictionaryFromMenu:))
        return [self _canLookUpInDictionary];

    return YES;
}

- (BOOL)validateUserInterfaceItem:(id <NSValidatedUserInterfaceItem>)item
{
    // This can be called during teardown when _webView is nil. Return NO when this happens, because CallUIDelegateReturningBoolean
    // assumes the WebVIew is non-nil.
    if (![self _webView])
        return NO;
    BOOL result = [self validateUserInterfaceItemWithoutDelegate:item];
    return CallUIDelegateReturningBoolean(result, [self _webView], @selector(webView:validateUserInterfaceItem:defaultValidation:), item, result);
}

// MARK: INTERFACE BUILDER ACTIONS FOR SAFARI

// Surprisingly enough, this isn't defined in any superclass, though it is defined in assorted AppKit classes since
// it's a standard menu item IBAction.
- (IBAction)copy:(id)sender
{
    [PDFSubview copy:sender];
}

// This used to be a standard IBAction (for Use Selection For Find), but AppKit now uses performFindPanelAction:
// with a menu item tag for this purpose.
- (IBAction)takeFindStringFromSelection:(id)sender
{
    [NSPasteboard _web_setFindPasteboardString:[[PDFSubview currentSelection] string] withOwner:self];
}

// MARK: WebFrameView UNDECLARED "DELEGATE METHODS"

// This is tested in -[WebFrameView canPrintHeadersAndFooters], but isn't declared anywhere (yuck)
- (BOOL)canPrintHeadersAndFooters
{
    return NO;
}

// This is tested in -[WebFrameView printOperationWithPrintInfo:], but isn't declared anywhere (yuck)
- (NSPrintOperation *)printOperationWithPrintInfo:(NSPrintInfo *)printInfo
{
    return [[PDFSubview document] getPrintOperationForPrintInfo:printInfo autoRotate:YES];
}

// MARK: WebDocumentView PROTOCOL IMPLEMENTATION

- (void)setDataSource:(WebDataSource *)ds
{
    if (dataSource == ds)
        return;

    dataSource = [ds retain];
    
    // FIXME: There must be some better place to put this. There is no comment in ChangeLog
    // explaining why it's in this method.
    [self setFrame:[[self superview] frame]];
}

- (void)dataSourceUpdated:(WebDataSource *)dataSource
{
}

- (void)setNeedsLayout:(BOOL)flag
{
}

- (void)layout
{
}

- (void)viewWillMoveToHostWindow:(NSWindow *)hostWindow
{
}

- (void)viewDidMoveToHostWindow
{
}

// MARK: WebDocumentElement PROTOCOL IMPLEMENTATION

- (NSDictionary *)elementAtPoint:(NSPoint)point
{
    WebFrame *frame = [dataSource webFrame];
    ASSERT(frame);
    
    return [NSDictionary dictionaryWithObjectsAndKeys:
        frame, WebElementFrameKey, 
        [NSNumber numberWithBool:[self _pointIsInSelection:point]], WebElementIsSelectedKey,
        nil];
}

- (NSDictionary *)elementAtPoint:(NSPoint)point allowShadowContent:(BOOL)allow
{
    return [self elementAtPoint:point];
}

// MARK: WebDocumentSearching PROTOCOL IMPLEMENTATION

- (BOOL)searchFor:(NSString *)string direction:(BOOL)forward caseSensitive:(BOOL)caseFlag wrap:(BOOL)wrapFlag
{
    return [self searchFor:string direction:forward caseSensitive:caseFlag wrap:wrapFlag startInSelection:NO];
}

// MARK: WebDocumentIncrementalSearching PROTOCOL IMPLEMENTATION

- (BOOL)searchFor:(NSString *)string direction:(BOOL)forward caseSensitive:(BOOL)caseFlag wrap:(BOOL)wrapFlag startInSelection:(BOOL)startInSelection
{
    PDFSelection *selection = [self _nextMatchFor:string direction:forward caseSensitive:caseFlag wrap:wrapFlag fromSelection:[PDFSubview currentSelection] startInSelection:startInSelection];
    if (!selection)
        return NO;

    [PDFSubview setCurrentSelection:selection];
    [PDFSubview scrollSelectionToVisible:nil];
    return YES;
}

// MARK: WebMultipleTextMatches PROTOCOL IMPLEMENTATION

- (void)setMarkedTextMatchesAreHighlighted:(BOOL)newValue
{
    // This method is part of the WebMultipleTextMatches algorithm, but this class doesn't support
    // highlighting text matches inline.
#ifndef NDEBUG
    if (newValue)
        LOG_ERROR("[WebPDFView setMarkedTextMatchesAreHighlighted:] called with YES, which isn't supported");
#endif
}

- (BOOL)markedTextMatchesAreHighlighted
{
    return NO;
}

static BOOL isFrameInRange(WebFrame *frame, DOMRange *range)
{
    BOOL inRange = NO;
    for (HTMLFrameOwnerElement* ownerElement = core(frame)->ownerElement(); ownerElement; ownerElement = ownerElement->document()->frame()->ownerElement()) {
        if (ownerElement->document() == core(range)->ownerDocument()) {
            inRange = [range intersectsNode:kit(ownerElement)];
            break;
        }
    }
    return inRange;
}

- (NSUInteger)countMatchesForText:(NSString *)string inDOMRange:(DOMRange *)range options:(WebFindOptions)options limit:(NSUInteger)limit markMatches:(BOOL)markMatches
{
    if (range && !isFrameInRange([dataSource webFrame], range))
        return 0;

    PDFSelection *previousMatch = nil;
    NSMutableArray *matches = [[NSMutableArray alloc] initWithCapacity:limit];
    
    for (;;) {
        PDFSelection *nextMatch = [self _nextMatchFor:string direction:YES caseSensitive:!(options & WebFindOptionsCaseInsensitive) wrap:NO fromSelection:previousMatch startInSelection:NO];
        if (!nextMatch)
            break;
        
        [matches addObject:nextMatch];
        previousMatch = nextMatch;

        if ([matches count] >= limit)
            break;
    }
    
    [self _setTextMatches:matches];
    [matches release];
    
    return [matches count];
}

- (void)unmarkAllTextMatches
{
    [self _setTextMatches:nil];
}

- (NSArray *)rectsForTextMatches
{
    NSMutableArray *result = [NSMutableArray arrayWithCapacity:[textMatches count]];
    NSSet *visiblePages = [self _visiblePDFPages];
    NSEnumerator *matchEnumerator = [textMatches objectEnumerator];
    PDFSelection *match;
    
    while ((match = [matchEnumerator nextObject]) != nil) {
        NSEnumerator *pages = [[match pages] objectEnumerator];
        PDFPage *page;
        while ((page = [pages nextObject]) != nil) {
            
            // Skip pages that aren't visible (needed for non-continuous modes, see 5362989)
            if (![visiblePages containsObject:page])
                continue;
            
            NSRect selectionOnPageInPDFViewCoordinates = [PDFSubview convertRect:[match boundsForPage:page] fromPage:page];
            [result addObject:[NSValue valueWithRect:selectionOnPageInPDFViewCoordinates]];
        }
    }

    return result;
}

// MARK: WebDocumentText PROTOCOL IMPLEMENTATION

- (BOOL)supportsTextEncoding
{
    return NO;
}

- (NSString *)string
{
    return [[PDFSubview document] string];
}

- (NSAttributedString *)attributedString
{
    // changing the selection is a hack, but the only way to get an attr string is via PDFSelection
    
    // must copy this selection object because we change the selection which seems to release it
    PDFSelection *savedSelection = [[PDFSubview currentSelection] copy];
    [PDFSubview selectAll:nil];
    NSAttributedString *result = [[PDFSubview currentSelection] attributedString];
    if (savedSelection) {
        [PDFSubview setCurrentSelection:savedSelection];
        [savedSelection release];
    } else {
        // FIXME: behavior of setCurrentSelection:nil is not documented - check 4182934 for progress
        // Otherwise, we could collapse this code with the case above.
        [PDFSubview clearSelection];
    }
    
    result = [self _scaledAttributedString:result];
    
    return result;
}

- (NSString *)selectedString
{
    return [[PDFSubview currentSelection] string];
}

- (NSAttributedString *)selectedAttributedString
{
    return [self _scaledAttributedString:[[PDFSubview currentSelection] attributedString]];
}

- (void)selectAll
{
    [PDFSubview selectAll:nil];
}

- (void)deselectAll
{
    [PDFSubview clearSelection];
}

// MARK: WebDocumentViewState PROTOCOL IMPLEMENTATION

// Even though to WebKit we are the "docView", in reality a PDFView contains its own scrollview and docView.
// And it even turns out there is another PDFKit view between the docView and its enclosing ScrollView, so
// we have to be sure to do our calculations based on that view, immediately inside the ClipView.  We try
// to make as few assumptions about the PDFKit view hierarchy as possible.

- (NSPoint)scrollPoint
{
    NSView *realDocView = [PDFSubview documentView];
    NSClipView *clipView = [[realDocView enclosingScrollView] contentView];
    return [clipView bounds].origin;
}

- (void)setScrollPoint:(NSPoint)p
{
    WebFrame *frame = [dataSource webFrame];
    //FIXME:  We only restore scroll state in the non-frames case because otherwise we get a crash due to
    // PDFKit calling display from within its drawRect:. See bugzilla 4164.
    if (![frame parentFrame]) {
        NSView *realDocView = [PDFSubview documentView];
        [(NSView *)[[realDocView enclosingScrollView] documentView] scrollPoint:p];
    }
}

- (id)viewState
{
    NSMutableArray *state = [NSMutableArray arrayWithCapacity:4];
    PDFDisplayMode mode = [PDFSubview displayMode];
    [state addObject:[NSNumber numberWithInt:mode]];
    if (mode == kPDFDisplaySinglePage || mode == kPDFDisplayTwoUp) {
        unsigned int pageIndex = [[PDFSubview document] indexForPage:[PDFSubview currentPage]];
        [state addObject:[NSNumber numberWithUnsignedInt:pageIndex]];
    }  // else in continuous modes, scroll position gets us to the right page
    BOOL autoScaleFlag = [PDFSubview autoScales];
    [state addObject:[NSNumber numberWithBool:autoScaleFlag]];
    if (!autoScaleFlag)
        [state addObject:[NSNumber numberWithFloat:[PDFSubview scaleFactor]]];

    return state;
}

- (void)setViewState:(id)statePList
{
    ASSERT([statePList isKindOfClass:[NSArray class]]);
    NSArray *state = statePList;
    int i = 0;
    PDFDisplayMode mode = [[state objectAtIndex:i++] intValue];
    [PDFSubview setDisplayMode:mode];
    if (mode == kPDFDisplaySinglePage || mode == kPDFDisplayTwoUp) {
        unsigned int pageIndex = [[state objectAtIndex:i++] unsignedIntValue];
        [PDFSubview goToPage:[[PDFSubview document] pageAtIndex:pageIndex]];
    }  // else in continuous modes, scroll position gets us to the right page
    BOOL autoScaleFlag = [[state objectAtIndex:i++] boolValue];
    [PDFSubview setAutoScales:autoScaleFlag];
    if (!autoScaleFlag)
        [PDFSubview setScaleFactor:[[state objectAtIndex:i++] floatValue]];
}

// MARK: _WebDocumentTextSizing PROTOCOL IMPLEMENTATION

- (IBAction)_zoomOut:(id)sender
{
    [PDFSubviewProxy zoomOut:sender];
}

- (IBAction)_zoomIn:(id)sender
{
    [PDFSubviewProxy zoomIn:sender];
}

- (IBAction)_resetZoom:(id)sender
{
    [PDFSubviewProxy setScaleFactor:1.0f];
}

- (BOOL)_canZoomOut
{
    return [PDFSubview canZoomOut];
}

- (BOOL)_canZoomIn
{
    return [PDFSubview canZoomIn];
}

- (BOOL)_canResetZoom
{
    return [PDFSubview scaleFactor] != 1.0;
}

// MARK: WebDocumentSelection PROTOCOL IMPLEMENTATION

- (NSRect)selectionRect
{
    NSRect result = NSZeroRect;
    PDFSelection *selection = [PDFSubview currentSelection];
    NSEnumerator *pages = [[selection pages] objectEnumerator];
    PDFPage *page;
    while ((page = [pages nextObject]) != nil) {
        NSRect selectionOnPageInPDFViewCoordinates = [PDFSubview convertRect:[selection boundsForPage:page] fromPage:page];
        if (NSIsEmptyRect(result))
            result = selectionOnPageInPDFViewCoordinates;
        else
            result = NSUnionRect(result, selectionOnPageInPDFViewCoordinates);
    }
    
    // Convert result to be in documentView (selectionView) coordinates
    result = [PDFSubview convertRect:result toView:[PDFSubview documentView]];
    
    return result;
}

- (NSArray *)selectionTextRects
{
    // FIXME: We'd need new PDFKit API/SPI to get multiple text rects for selections that intersect more than one line
    return [NSArray arrayWithObject:[NSValue valueWithRect:[self selectionRect]]];
}

- (NSView *)selectionView
{
    return [PDFSubview documentView];
}

- (NSImage *)selectionImageForcingBlackText:(BOOL)forceBlackText
{
    // Convert the selection to an attributed string, and draw that.
    // FIXME 4621154: this doesn't handle italics (and maybe other styles)
    // FIXME 4604366: this doesn't handle text at non-actual size
    NSMutableAttributedString *attributedString = [[self selectedAttributedString] mutableCopy];
    NSRange wholeStringRange = NSMakeRange(0, [attributedString length]);
    
    // Modify the styles in the attributed string to draw black text, no background, and no underline. We draw 
    // no underline because it would look ugly.
    [attributedString beginEditing];
    [attributedString removeAttribute:NSBackgroundColorAttributeName range:wholeStringRange];
    [attributedString removeAttribute:NSUnderlineStyleAttributeName range:wholeStringRange];
    if (forceBlackText)
        [attributedString addAttribute:NSForegroundColorAttributeName value:[NSColor colorWithDeviceWhite:0.0f alpha:1.0f] range:wholeStringRange];
    [attributedString endEditing];
    
    NSImage* selectionImage = [[[NSImage alloc] initWithSize:[self selectionRect].size] autorelease];
    
    [selectionImage lockFocus];
    [attributedString drawAtPoint:NSZeroPoint];
    [selectionImage unlockFocus];
    
    [attributedString release];

    return selectionImage;
}

- (NSRect)selectionImageRect
{
    // FIXME: deal with clipping?
    return [self selectionRect];
}

- (NSArray *)pasteboardTypesForSelection
{
    return [NSArray arrayWithObjects:NSRTFDPboardType, NSRTFPboardType, NSStringPboardType, nil];
}

- (void)writeSelectionWithPasteboardTypes:(NSArray *)types toPasteboard:(NSPasteboard *)pasteboard
{
    NSAttributedString *attributedString = [self selectedAttributedString];
    
    if ([types containsObject:NSRTFDPboardType]) {
        NSData *RTFDData = [attributedString RTFDFromRange:NSMakeRange(0, [attributedString length]) documentAttributes:nil];
        [pasteboard setData:RTFDData forType:NSRTFDPboardType];
    }        
    
    if ([types containsObject:NSRTFPboardType]) {
        if ([attributedString containsAttachments])
            attributedString = attributedStringByStrippingAttachmentCharacters(attributedString);

        NSData *RTFData = [attributedString RTFFromRange:NSMakeRange(0, [attributedString length]) documentAttributes:nil];
        [pasteboard setData:RTFData forType:NSRTFPboardType];
    }
    
    if ([types containsObject:NSStringPboardType])
        [pasteboard setString:[self selectedString] forType:NSStringPboardType];
}

// MARK: PDFView DELEGATE METHODS

- (void)PDFViewWillClickOnLink:(PDFView *)sender withURL:(NSURL *)URL
{
    if (!URL)
        return;

    NSWindow *window = [sender window];
    NSEvent *nsEvent = [window currentEvent];
    const int noButton = -1;
    int button = noButton;
    RefPtr<Event> event;
    switch ([nsEvent type]) {
        case NSLeftMouseUp:
            button = 0;
            break;
        case NSRightMouseUp:
            button = 1;
            break;
        case NSOtherMouseUp:
            button = [nsEvent buttonNumber];
            break;
        case NSKeyDown: {
            PlatformKeyboardEvent pe = PlatformEventFactory::createPlatformKeyboardEvent(nsEvent);
            pe.disambiguateKeyDownEvent(PlatformEvent::RawKeyDown);
            event = KeyboardEvent::create(pe, 0);
        }
        default:
            break;
    }
    if (button != noButton) {
        event = MouseEvent::create(eventNames().clickEvent, true, true, currentTime(), 0, [nsEvent clickCount], 0, 0, 0, 0,
            [nsEvent modifierFlags] & NSControlKeyMask,
            [nsEvent modifierFlags] & NSAlternateKeyMask,
            [nsEvent modifierFlags] & NSShiftKeyMask,
            [nsEvent modifierFlags] & NSCommandKeyMask,
            button, 0, 0, true);
    }

    // Call to the frame loader because this is where our security checks are made.
    Frame* frame = core([dataSource webFrame]);
    frame->loader()->loadFrameRequest(FrameLoadRequest(frame->document()->securityOrigin(), ResourceRequest(URL)), false, false, event.get(), 0, MaybeSendReferrer);
}

- (void)PDFViewOpenPDFInNativeApplication:(PDFView *)sender
{
    // Delegate method sent when the user requests opening the PDF file in the system's default app
    [self _openWithFinder:sender];
}

- (void)PDFViewPerformPrint:(PDFView *)sender
{
    CallUIDelegate([self _webView], @selector(webView:printFrameView:), [[dataSource webFrame] frameView]);
}

- (void)PDFViewSavePDFToDownloadFolder:(PDFView *)sender
{
    // We don't want to write the file until we have a document to write (see 5267607).
    if (![PDFSubview document]) {
        NSBeep();
        return;
    }

    // Delegate method sent when the user requests downloading the PDF file to disk. We pass NO for
    // showingPanel: so that the PDF file is saved to the standard location without user intervention.
    CallUIDelegate([self _webView], @selector(webView:saveFrameView:showingPanel:), [[dataSource webFrame] frameView], NO);
}

@end

@implementation WebPDFView (FileInternal)

+ (Class)_PDFPreviewViewClass
{
    static Class PDFPreviewViewClass = nil;
    static BOOL checkedForPDFPreviewViewClass = NO;
    
    if (!checkedForPDFPreviewViewClass) {
        checkedForPDFPreviewViewClass = YES;
        PDFPreviewViewClass = [[WebPDFView PDFKitBundle] classNamed:@"PDFPreviewView"];
    }
    
    // This class might not be available; callers need to deal with a nil return here.
    return PDFPreviewViewClass;
}

+ (Class)_PDFViewClass
{
    static Class PDFViewClass = nil;
    if (PDFViewClass == nil) {
        PDFViewClass = [[WebPDFView PDFKitBundle] classNamed:@"PDFView"];
        if (!PDFViewClass)
            LOG_ERROR("Couldn't find PDFView class in PDFKit.framework");
    }
    return PDFViewClass;
}

- (BOOL)_anyPDFTagsFoundInMenu:(NSMenu *)menu
{
    NSEnumerator *e = [[menu itemArray] objectEnumerator];
    NSMenuItem *item;
    while ((item = [e nextObject]) != nil) {
        switch ([item tag]) {
            case WebMenuItemTagOpenWithDefaultApplication:
            case WebMenuItemPDFActualSize:
            case WebMenuItemPDFZoomIn:
            case WebMenuItemPDFZoomOut:
            case WebMenuItemPDFAutoSize:
            case WebMenuItemPDFSinglePage:
            case WebMenuItemPDFSinglePageScrolling:
            case WebMenuItemPDFFacingPages:
            case WebMenuItemPDFFacingPagesScrolling:
            case WebMenuItemPDFContinuous:
            case WebMenuItemPDFNextPage:
            case WebMenuItemPDFPreviousPage:
                return YES;
        }
    }
    return NO;
}

- (void)_applyPDFDefaults
{
    // Set up default viewing params
    WebPreferences *prefs = [[dataSource _webView] preferences];
    float scaleFactor = [prefs PDFScaleFactor];
    if (scaleFactor == 0)
        [PDFSubview setAutoScales:YES];
    else {
        [PDFSubview setAutoScales:NO];
        [PDFSubview setScaleFactor:scaleFactor];
    }
    [PDFSubview setDisplayMode:[prefs PDFDisplayMode]];
}

- (BOOL)_canLookUpInDictionary
{
    return [PDFSubview respondsToSelector:@selector(_searchInDictionary:)];
}

- (NSClipView *)_clipViewForPDFDocumentView
{
    NSClipView *clipView = (NSClipView *)[[PDFSubview documentView] _web_superviewOfClass:[NSClipView class]];
    ASSERT(clipView);
    return clipView;
}

- (NSEvent *)_fakeKeyEventWithFunctionKey:(unichar)functionKey
{
    // FIXME 4400480: when PDFView implements the standard scrolling selectors that this
    // method is used to mimic, we can eliminate this method and call them directly.
    NSString *keyAsString = [NSString stringWithCharacters:&functionKey length:1];
    return [NSEvent keyEventWithType:NSKeyDown
                            location:NSZeroPoint
                       modifierFlags:0
                           timestamp:0
                        windowNumber:0
                             context:nil
                          characters:keyAsString
         charactersIgnoringModifiers:keyAsString
                           isARepeat:NO
                             keyCode:0];
}

- (void)_lookUpInDictionaryFromMenu:(id)sender
{
    // This method is used by WebKit's context menu item. Here we map to the method that
    // PDFView uses. Since the PDFView method isn't API, and isn't available on all versions
    // of PDFKit, we use performSelector after a respondsToSelector check, rather than calling it directly.
    if ([self _canLookUpInDictionary])
        [PDFSubview performSelector:@selector(_searchInDictionary:) withObject:sender];
}

- (NSMutableArray *)_menuItemsFromPDFKitForEvent:(NSEvent *)theEvent
{
    NSMutableArray *copiedItems = [NSMutableArray array];
    NSDictionary *actionsToTags = [[NSDictionary alloc] initWithObjectsAndKeys:
        [NSNumber numberWithInt:WebMenuItemPDFActualSize], NSStringFromSelector(@selector(_setActualSize:)),
        [NSNumber numberWithInt:WebMenuItemPDFZoomIn], NSStringFromSelector(@selector(zoomIn:)),
        [NSNumber numberWithInt:WebMenuItemPDFZoomOut], NSStringFromSelector(@selector(zoomOut:)),
        [NSNumber numberWithInt:WebMenuItemPDFAutoSize], NSStringFromSelector(@selector(_setAutoSize:)),
        [NSNumber numberWithInt:WebMenuItemPDFSinglePage], NSStringFromSelector(@selector(_setSinglePage:)),
        [NSNumber numberWithInt:WebMenuItemPDFSinglePageScrolling], NSStringFromSelector(@selector(_setSinglePageScrolling:)),
        [NSNumber numberWithInt:WebMenuItemPDFFacingPages], NSStringFromSelector(@selector(_setDoublePage:)),
        [NSNumber numberWithInt:WebMenuItemPDFFacingPagesScrolling], NSStringFromSelector(@selector(_setDoublePageScrolling:)),
        [NSNumber numberWithInt:WebMenuItemPDFContinuous], NSStringFromSelector(@selector(_toggleContinuous:)),
        [NSNumber numberWithInt:WebMenuItemPDFNextPage], NSStringFromSelector(@selector(goToNextPage:)),
        [NSNumber numberWithInt:WebMenuItemPDFPreviousPage], NSStringFromSelector(@selector(goToPreviousPage:)),
        nil];
    
    // Leave these menu items out, since WebKit inserts equivalent ones. Note that we leave out PDFKit's "Look Up in Dictionary"
    // item here because WebKit already includes an item with the same title and purpose. We map WebKit's to PDFKit's 
    // "Look Up in Dictionary" via the implementation of -[WebPDFView _lookUpInDictionaryFromMenu:].
    NSSet *unwantedActions = [[NSSet alloc] initWithObjects:
                              NSStringFromSelector(@selector(_searchInSpotlight:)),
                              NSStringFromSelector(@selector(_searchInGoogle:)),
                              NSStringFromSelector(@selector(_searchInDictionary:)),
                              NSStringFromSelector(@selector(copy:)),
                              nil];
    
    NSEnumerator *e = [[[PDFSubview menuForEvent:theEvent] itemArray] objectEnumerator];
    NSMenuItem *item;
    while ((item = [e nextObject]) != nil) {
        
        NSString *actionString = NSStringFromSelector([item action]);
        
        if ([unwantedActions containsObject:actionString])
            continue;
        
        // Copy items since a menu item can be in only one menu at a time, and we don't
        // want to modify the original menu supplied by PDFKit.
        NSMenuItem *itemCopy = [item copy];
        [copiedItems addObject:itemCopy];
        [itemCopy release];
        
        // Include all of PDFKit's separators for now. At the end we'll remove any ones that were made
        // useless by removing PDFKit's menu items.
        if ([itemCopy isSeparatorItem])
            continue;

        NSNumber *tagNumber = [actionsToTags objectForKey:actionString];
        
        int tag;
        if (tagNumber != nil)
            tag = [tagNumber intValue];
        else {
            // This should happen only if PDFKit updates behind WebKit's back. It's non-ideal because clients that only include tags
            // that they recognize (like Safari) won't get these PDFKit additions until WebKit is updated to match.
            tag = WebMenuItemTagOther;
            LOG_ERROR("no WebKit menu item tag found for PDF context menu item action \"%@\", using WebMenuItemTagOther", actionString);
        }
        
        if ([itemCopy tag] == 0) {
            [itemCopy setTag:tag];
            if ([itemCopy target] == PDFSubview) {
                // Note that updating the defaults is cheap because it catches redundant settings, so installing
                // the proxy for actions that don't impact the defaults is OK
                [itemCopy setTarget:PDFSubviewProxy];
            }
        } else
            LOG_ERROR("PDF context menu item %@ came with tag %d, so no WebKit tag was applied. This could mean that the item doesn't appear in clients such as Safari.", [itemCopy title], [itemCopy tag]);
    }
    
    [actionsToTags release];
    [unwantedActions release];
    
    // Since we might have removed elements supplied by PDFKit, and we want to minimize our hardwired
    // knowledge of the order and arrangement of PDFKit's menu items, we need to remove any bogus
    // separators that were left behind.
    [copiedItems _webkit_removeUselessMenuItemSeparators];
    
    return copiedItems;
}

- (PDFSelection *)_nextMatchFor:(NSString *)string direction:(BOOL)forward caseSensitive:(BOOL)caseFlag wrap:(BOOL)wrapFlag fromSelection:(PDFSelection *)initialSelection startInSelection:(BOOL)startInSelection
{
    if (![string length])
        return nil;
    
    int options = 0;
    if (!forward)
        options |= NSBackwardsSearch;
    
    if (!caseFlag)
        options |= NSCaseInsensitiveSearch;
    
    PDFDocument *document = [PDFSubview document];
    
    PDFSelection *selectionForInitialSearch = [initialSelection copy];
    if (startInSelection) {
        // Initially we want to include the selected text in the search. PDFDocument's API always searches from just
        // past the passed-in selection, so we need to pass a selection that's modified appropriately. 
        // FIXME 4182863: Ideally we'd use a zero-length selection at the edge of the current selection, but zero-length
        // selections don't work in PDFDocument. So instead we make a one-length selection just before or after the
        // current selection, which works for our purposes even when the current selection is at an edge of the
        // document.
        int initialSelectionLength = [[initialSelection string] length];
        if (forward) {
            [selectionForInitialSearch extendSelectionAtStart:1];
            [selectionForInitialSearch extendSelectionAtEnd:-initialSelectionLength];
        } else {
            [selectionForInitialSearch extendSelectionAtEnd:1];
            [selectionForInitialSearch extendSelectionAtStart:-initialSelectionLength];
        }
    }
    PDFSelection *foundSelection = [document findString:string fromSelection:selectionForInitialSearch withOptions:options];
    [selectionForInitialSearch release];

    // If we first searched in the selection, and we found the selection, search again from just past the selection
    if (startInSelection && _PDFSelectionsAreEqual(foundSelection, initialSelection))
        foundSelection = [document findString:string fromSelection:initialSelection withOptions:options];
    
    if (!foundSelection && wrapFlag)
        foundSelection = [document findString:string fromSelection:nil withOptions:options];
    
    return foundSelection;
}

- (void)_openWithFinder:(id)sender
{
    // We don't want to write the file until we have a document to write (see 4892525).
    if (![PDFSubview document]) {
        NSBeep();
        return;
    }
    
    NSString *opath = [self _path];
    
    if (opath) {
        if (!written) {
            // Create a PDF file with the minimal permissions (only accessible to the current user, see 4145714)
            NSNumber *permissions = [[NSNumber alloc] initWithInt:S_IRUSR];
            NSDictionary *fileAttributes = [[NSDictionary alloc] initWithObjectsAndKeys:permissions, NSFilePosixPermissions, nil];
            [permissions release];

            [[NSFileManager defaultManager] createFileAtPath:opath contents:[dataSource data] attributes:fileAttributes];
            
            [fileAttributes release];
            written = YES;
        }
        
        if (![[NSWorkspace sharedWorkspace] openFile:opath]) {
            // NSWorkspace couldn't open file.  Do we need an alert
            // here?  We ignore the error elsewhere.
        }
    }
}

- (NSString *)_path
{
    // Generate path once.
    if (path)
        return path;
    
    NSString *filename = [[dataSource response] suggestedFilename];
    NSFileManager *manager = [NSFileManager defaultManager]; 
    NSString *temporaryPDFDirectoryPath = [self _temporaryPDFDirectoryPath];
    
    if (!temporaryPDFDirectoryPath) {
        // This should never happen; if it does we'll fail silently on non-debug builds.
        ASSERT_NOT_REACHED();
        return nil;
    }
    
    path = [temporaryPDFDirectoryPath stringByAppendingPathComponent:filename];
    if ([manager fileExistsAtPath:path]) {
        NSString *pathTemplatePrefix = [temporaryPDFDirectoryPath stringByAppendingPathComponent:@"XXXXXX-"];
        NSString *pathTemplate = [pathTemplatePrefix stringByAppendingString:filename];
        // fileSystemRepresentation returns a const char *; copy it into a char * so we can modify it safely
        char *cPath = strdup([pathTemplate fileSystemRepresentation]);
        int fd = mkstemps(cPath, strlen(cPath) - strlen([pathTemplatePrefix fileSystemRepresentation]) + 1);
        if (fd < 0) {
            // Couldn't create a temporary file! Should never happen; if it does we'll fail silently on non-debug builds.
            ASSERT_NOT_REACHED();
            path = nil;
        } else {
            close(fd);
            path = [manager stringWithFileSystemRepresentation:cPath length:strlen(cPath)];
        }
        free(cPath);
    }
    
    [path retain];
    
    return path;
}

- (void)_PDFDocumentViewMightHaveScrolled:(NSNotification *)notification
{ 
    NSClipView *clipView = [self _clipViewForPDFDocumentView];
    ASSERT([notification object] == clipView);
    
    NSPoint scrollPosition = [clipView bounds].origin;
    if (NSEqualPoints(scrollPosition, lastScrollPosition))
        return;
    
    lastScrollPosition = scrollPosition;
    WebView *webView = [self _webView];
    [[webView _UIDelegateForwarder] webView:webView didScrollDocumentInFrameView:[[dataSource webFrame] frameView]];
}

- (PDFView *)_PDFSubview
{
    return PDFSubview;
}

- (BOOL)_pointIsInSelection:(NSPoint)point
{
    PDFPage *page = [PDFSubview pageForPoint:point nearest:NO];
    if (!page)
        return NO;
    
    NSRect selectionRect = [PDFSubview convertRect:[[PDFSubview currentSelection] boundsForPage:page] fromPage:page];
    
    return NSPointInRect(point, selectionRect);
}

- (void)_scaleOrDisplayModeOrPageChanged:(NSNotification *)notification
{
    ASSERT([notification object] == PDFSubview);
    if (!_ignoreScaleAndDisplayModeAndPageNotifications) {
        [self _updatePreferencesSoon];
        // Notify UI delegate that the entire page has been redrawn, since (unlike for WebHTMLView)
        // we can't hook into the drawing mechanism itself. This fixes 5337529.
        WebView *webView = [self _webView];
        [[webView _UIDelegateForwarder] webView:webView didDrawRect:[webView bounds]];
    }
}

- (NSAttributedString *)_scaledAttributedString:(NSAttributedString *)unscaledAttributedString
{
    if (!unscaledAttributedString)
        return nil;
    
    float scaleFactor = [PDFSubview scaleFactor];
    if (scaleFactor == 1.0)
        return unscaledAttributedString;
    
    NSMutableAttributedString *result = [[unscaledAttributedString mutableCopy] autorelease];
    unsigned int length = [result length];
    NSRange effectiveRange = NSMakeRange(0,0);
    
    [result beginEditing];    
    while (NSMaxRange(effectiveRange) < length) {
        NSFont *unscaledFont = [result attribute:NSFontAttributeName atIndex:NSMaxRange(effectiveRange) effectiveRange:&effectiveRange];
        
        if (!unscaledFont) {
            // FIXME: We can't scale the font if we don't know what it is. We should always know what it is,
            // but sometimes don't due to PDFKit issue 5089411. When that's addressed, we can remove this
            // early continue.
            LOG_ERROR("no font attribute found in range %@ for attributed string \"%@\" on page %@ (see radar 5089411)", NSStringFromRange(effectiveRange), result, [[dataSource request] URL]);
            continue;
        }
        
        NSFont *scaledFont = [NSFont fontWithName:[unscaledFont fontName] size:[unscaledFont pointSize]*scaleFactor];
        [result addAttribute:NSFontAttributeName value:scaledFont range:effectiveRange];
    }
    [result endEditing];
    
    return result;
}

- (void)_setTextMatches:(NSArray *)array
{
    [array retain];
    [textMatches release];
    textMatches = array;
}

- (NSString *)_temporaryPDFDirectoryPath
{
    // Returns nil if the temporary PDF directory didn't exist and couldn't be created
    
    static NSString *_temporaryPDFDirectoryPath = nil;
    
    if (!_temporaryPDFDirectoryPath) {
        NSString *temporaryDirectoryTemplate = [NSTemporaryDirectory() stringByAppendingPathComponent:@"WebKitPDFs-XXXXXX"];
        char *cTemplate = strdup([temporaryDirectoryTemplate fileSystemRepresentation]);
        
        if (!mkdtemp(cTemplate)) {
            // This should never happen; if it does we'll fail silently on non-debug builds.
            ASSERT_NOT_REACHED();
        } else {
            // cTemplate has now been modified to be the just-created directory name. This directory has 700 permissions,
            // so only the current user can add to it or view its contents.
            _temporaryPDFDirectoryPath = [[[NSFileManager defaultManager] stringWithFileSystemRepresentation:cTemplate length:strlen(cTemplate)] retain];
        }
        
        free(cTemplate);
    }
    
    return _temporaryPDFDirectoryPath;
}

- (void)_trackFirstResponder
{
    ASSERT([self window]);
    BOOL newFirstResponderIsPDFDocumentView = [[self window] firstResponder] == [PDFSubview documentView];
    if (newFirstResponderIsPDFDocumentView == firstResponderIsPDFDocumentView)
        return;
    
    // This next clause is the entire purpose of _trackFirstResponder. In other WebDocument
    // view classes this is done in a resignFirstResponder override, but in this case the
    // first responder view is a PDFKit class that we can't subclass.
    if (newFirstResponderIsPDFDocumentView && ![[dataSource _webView] maintainsInactiveSelection])
        [self deselectAll];
    
    firstResponderIsPDFDocumentView = newFirstResponderIsPDFDocumentView;
}

- (void)_updatePreferences:(WebPreferences *)prefs
{
    float scaleFactor = [PDFSubview autoScales] ? 0.0f : [PDFSubview scaleFactor];
    [prefs setPDFScaleFactor:scaleFactor];
    [prefs setPDFDisplayMode:[PDFSubview displayMode]];
    _willUpdatePreferencesSoon = NO;
    [prefs release];
    [self release];
}

- (void)_updatePreferencesSoon
{   
    // Consolidate calls; due to the WebPDFPrefUpdatingProxy method, this can be called multiple times with a single user action
    // such as showing the context menu.
    if (_willUpdatePreferencesSoon)
        return;

    WebPreferences *prefs = [[dataSource _webView] preferences];

    [self retain];
    [prefs retain];
    [self performSelector:@selector(_updatePreferences:) withObject:prefs afterDelay:0];
    _willUpdatePreferencesSoon = YES;
}

- (NSSet *)_visiblePDFPages
{
    // Returns the set of pages that are at least partly visible, used to avoid processing non-visible pages
    PDFDocument *pdfDocument = [PDFSubview document];
    if (!pdfDocument)
        return nil;
    
    NSRect pdfViewBounds = [PDFSubview bounds];
    PDFPage *topLeftPage = [PDFSubview pageForPoint:NSMakePoint(NSMinX(pdfViewBounds), NSMaxY(pdfViewBounds)) nearest:YES];
    PDFPage *bottomRightPage = [PDFSubview pageForPoint:NSMakePoint(NSMaxX(pdfViewBounds), NSMinY(pdfViewBounds)) nearest:YES];
    
    // only page-free documents should return nil for either of these two since we passed YES for nearest:
    if (!topLeftPage) {
        ASSERT(!bottomRightPage);
        return nil;
    }
    
    NSUInteger firstVisiblePageIndex = [pdfDocument indexForPage:topLeftPage];
    NSUInteger lastVisiblePageIndex = [pdfDocument indexForPage:bottomRightPage];
    
    if (firstVisiblePageIndex > lastVisiblePageIndex) {
        NSUInteger swap = firstVisiblePageIndex;
        firstVisiblePageIndex = lastVisiblePageIndex;
        lastVisiblePageIndex = swap;
    }
    
    NSMutableSet *result = [NSMutableSet set];
    NSUInteger pageIndex;
    for (pageIndex = firstVisiblePageIndex; pageIndex <= lastVisiblePageIndex; ++pageIndex)
        [result addObject:[pdfDocument pageAtIndex:pageIndex]];

    return result;
}

@end

@implementation WebPDFPrefUpdatingProxy

- (id)initWithView:(WebPDFView *)aView
{
    // No [super init], since we inherit from NSProxy
    view = aView;
    return self;
}

- (void)forwardInvocation:(NSInvocation *)invocation
{
    [invocation invokeWithTarget:[view _PDFSubview]];    
    [view _updatePreferencesSoon];
}

- (NSMethodSignature *)methodSignatureForSelector:(SEL)sel
{
    return [[view _PDFSubview] methodSignatureForSelector:sel];
}

@end
