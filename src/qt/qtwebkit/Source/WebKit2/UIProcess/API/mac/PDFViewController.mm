/*
 * Copyright (C) 2010, 2011 Apple Inc. All rights reserved.
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

#import "config.h"
#import "PDFViewController.h"

#import "DataReference.h"
#import "WKAPICast.h"
#import "WKViewPrivate.h"
#import "WebData.h"
#import "WebEventFactory.h"
#import "WebPageGroup.h"
#import "WebPageProxy.h"
#import "WebPreferences.h"
#import <PDFKit/PDFKit.h>
#import <WebCore/LocalizedStrings.h>
#import <WebCore/UUID.h>
#import <wtf/ObjcRuntimeExtras.h>
#import <wtf/text/CString.h>
#import <wtf/text/WTFString.h>

// Redeclarations of PDFKit notifications. We can't use the API since we use a weak link to the framework.
#define _webkit_PDFViewDisplayModeChangedNotification @"PDFViewDisplayModeChanged"
#define _webkit_PDFViewScaleChangedNotification @"PDFViewScaleChanged"
#define _webkit_PDFViewPageChangedNotification @"PDFViewChangedPage"

using namespace WebCore;
using namespace WebKit;

@class PDFDocument;
@class PDFView;

@interface PDFDocument (PDFDocumentDetails)
- (NSPrintOperation *)getPrintOperationForPrintInfo:(NSPrintInfo *)printInfo autoRotate:(BOOL)doRotate;
@end

extern "C" NSString *_NSPathForSystemFramework(NSString *framework);

// MARK: C UTILITY FUNCTIONS

static void _applicationInfoForMIMEType(NSString *type, NSString **name, NSImage **image)
{
    ASSERT(name);
    ASSERT(image);
    
    CFURLRef appURL = 0;

    OSStatus error = LSCopyApplicationForMIMEType((CFStringRef)type, kLSRolesAll, &appURL);
    if (error != noErr)
        return;

    NSString *appPath = [(NSURL *)appURL path];
    if (appURL)
        CFRelease(appURL);

    *image = [[NSWorkspace sharedWorkspace] iconForFile:appPath];
    [*image setSize:NSMakeSize(16, 16)];

    *name = [[NSFileManager defaultManager] displayNameAtPath:appPath];
}

// FIXME 4182876: We can eliminate this function in favor if -isEqual: if [PDFSelection isEqual:] is overridden
// to compare contents.
static BOOL _PDFSelectionsAreEqual(PDFSelection *selectionA, PDFSelection *selectionB)
{
    NSArray *aPages = [selectionA pages];
    NSArray *bPages = [selectionB pages];

    if (![aPages isEqual:bPages])
        return NO;

    NSUInteger count = [aPages count];
    for (NSUInteger i = 0; i < count; ++i) {
        NSRect aBounds = [selectionA boundsForPage:[aPages objectAtIndex:i]];
        NSRect bBounds = [selectionB boundsForPage:[bPages objectAtIndex:i]];
        if (!NSEqualRects(aBounds, bBounds))
            return NO;
    }

    return YES;
}

@interface WKPDFView : NSView
{
    PDFViewController* _pdfViewController;

    RetainPtr<NSView> _pdfPreviewView;
    PDFView *_pdfView;
    BOOL _ignoreScaleAndDisplayModeAndPageNotifications;
    BOOL _willUpdatePreferencesSoon;
}

- (id)initWithFrame:(NSRect)frame PDFViewController:(PDFViewController*)pdfViewController;
- (void)invalidate;
- (PDFView *)pdfView;
- (void)setDocument:(PDFDocument *)pdfDocument;

- (BOOL)forwardScrollWheelEvent:(NSEvent *)wheelEvent;
- (void)_applyPDFPreferences;
- (PDFSelection *)_nextMatchFor:(NSString *)string direction:(BOOL)forward caseSensitive:(BOOL)caseFlag wrap:(BOOL)wrapFlag fromSelection:(PDFSelection *)initialSelection startInSelection:(BOOL)startInSelection;
@end

@implementation WKPDFView

- (id)initWithFrame:(NSRect)frame PDFViewController:(PDFViewController*)pdfViewController
{
    if ((self = [super initWithFrame:frame])) {
        _pdfViewController = pdfViewController;

        [self setAutoresizingMask:NSViewWidthSizable | NSViewHeightSizable];

        Class previewViewClass = PDFViewController::pdfPreviewViewClass();
        ASSERT(previewViewClass);

        _pdfPreviewView = adoptNS([[previewViewClass alloc] initWithFrame:frame]);
        [_pdfPreviewView.get() setAutoresizingMask:NSViewWidthSizable | NSViewHeightSizable];
        [self addSubview:_pdfPreviewView.get()];

        _pdfView = [_pdfPreviewView.get() performSelector:@selector(pdfView)];
        [_pdfView setDelegate:self];
    }

    return self;
}

- (void)invalidate
{
    _pdfViewController = 0;
}

- (PDFView *)pdfView
{
    return _pdfView;
}

- (void)setDocument:(PDFDocument *)pdfDocument
{
    _ignoreScaleAndDisplayModeAndPageNotifications = YES;
    [_pdfView setDocument:pdfDocument];
    [self _applyPDFPreferences];
    _ignoreScaleAndDisplayModeAndPageNotifications = NO;
}

- (void)_applyPDFPreferences
{
    if (!_pdfViewController)
        return;

    WebPreferences *preferences = _pdfViewController->page()->pageGroup()->preferences();

    CGFloat scaleFactor = preferences->pdfScaleFactor();
    if (!scaleFactor)
        [_pdfView setAutoScales:YES];
    else {
        [_pdfView setAutoScales:NO];
        [_pdfView setScaleFactor:scaleFactor];
    }
    [_pdfView setDisplayMode:preferences->pdfDisplayMode()];
}

- (void)_updatePreferences:(id)ignored
{
    _willUpdatePreferencesSoon = NO;

    if (!_pdfViewController)
        return;

    WebPreferences* preferences = _pdfViewController->page()->pageGroup()->preferences();

    CGFloat scaleFactor = [_pdfView autoScales] ? 0 : [_pdfView scaleFactor];
    preferences->setPDFScaleFactor(scaleFactor);
    preferences->setPDFDisplayMode([_pdfView displayMode]);
}

- (void)_updatePreferencesSoon
{   
    if (_willUpdatePreferencesSoon)
        return;

    [self performSelector:@selector(_updatePreferences:) withObject:nil afterDelay:0];
    _willUpdatePreferencesSoon = YES;
}

- (void)_scaleOrDisplayModeOrPageChanged:(NSNotification *)notification
{
    ASSERT_ARG(notification, [notification object] == _pdfView);
    if (!_ignoreScaleAndDisplayModeAndPageNotifications)
        [self _updatePreferencesSoon];
}

- (void)_openWithFinder:(id)sender
{
    _pdfViewController->openPDFInFinder();
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

    PDFDocument *document = [_pdfView document];

    PDFSelection *selectionForInitialSearch = [initialSelection copy];
    if (startInSelection) {
        // Initially we want to include the selected text in the search.  So we must modify the starting search 
        // selection to fit PDFDocument's search requirements: selection must have a length >= 1, begin before 
        // the current selection (if searching forwards) or after (if searching backwards).
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

- (NSUInteger)_countMatches:(NSString *)string caseSensitive:(BOOL)caseFlag
{
    if (![string length])
        return 0;

    int options = caseFlag ? 0 : NSCaseInsensitiveSearch;

    return [[[_pdfView document] findString:string withOptions:options] count];
}

// MARK: NSView overrides

- (void)viewDidMoveToWindow
{
    if (![self window])
        return;

    NSNotificationCenter *notificationCenter = [NSNotificationCenter defaultCenter];
    [notificationCenter addObserver:self selector:@selector(_scaleOrDisplayModeOrPageChanged:) name:_webkit_PDFViewScaleChangedNotification object:_pdfView];
    [notificationCenter addObserver:self selector:@selector(_scaleOrDisplayModeOrPageChanged:) name:_webkit_PDFViewDisplayModeChangedNotification object:_pdfView];
    [notificationCenter addObserver:self selector:@selector(_scaleOrDisplayModeOrPageChanged:) name:_webkit_PDFViewPageChangedNotification object:_pdfView];
}

- (void)viewWillMoveToWindow:(NSWindow *)newWindow
{
    if (![self window])
        return;

    NSNotificationCenter *notificationCenter = [NSNotificationCenter defaultCenter];
    [notificationCenter removeObserver:self name:_webkit_PDFViewScaleChangedNotification object:_pdfView];
    [notificationCenter removeObserver:self name:_webkit_PDFViewDisplayModeChangedNotification object:_pdfView];
    [notificationCenter removeObserver:self name:_webkit_PDFViewPageChangedNotification object:_pdfView];
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

static void insertOpenWithDefaultPDFMenuItem(NSMenu *menu, NSUInteger index)
{
    // Add in an "Open with <default PDF viewer>" item
    NSString *appName = nil;
    NSImage *appIcon = nil;
    
    _applicationInfoForMIMEType(@"application/pdf", &appName, &appIcon);
    if (!appName)
        appName = WEB_UI_STRING("Finder", "Default application name for Open With context menu");
    
    // To match the PDFKit style, we'll add Open with Preview even when there's no document yet to view, and
    // disable it using validateUserInterfaceItem.
    NSString *title = [NSString stringWithFormat:WEB_UI_STRING("Open with %@", "context menu item for PDF"), appName];
    
    NSMenuItem *item = [[NSMenuItem alloc] initWithTitle:title action:@selector(_openWithFinder:) keyEquivalent:@""];
    if (appIcon)
        [item setImage:appIcon];
    [menu insertItem:item atIndex:index];
    [item release];
}

- (NSMenu *)menuForEvent:(NSEvent *)theEvent
{
    NSMenu *menu = [[NSMenu alloc] initWithTitle:@""];

    bool insertedOpenWithItem = false;
    
    NSEnumerator *menuItemEnumerator = [[[_pdfView menuForEvent:theEvent] itemArray] objectEnumerator];
    while (NSMenuItem *item = [menuItemEnumerator nextObject]) {
        NSMenuItem *itemCopy = [item copy];
        [menu addItem:itemCopy];
        [itemCopy release];

        if (insertedOpenWithItem)
            continue;
        
        // If a "Copy" item is present, place the "Open With" item just after it, with an intervening separator.
        if ([item action] != @selector(copy:))
            continue;
        
        [menu addItem:[NSMenuItem separatorItem]];
        insertOpenWithDefaultPDFMenuItem(menu, [menu numberOfItems]);
        insertedOpenWithItem = true;
    }
    
    if (!insertedOpenWithItem) {
        // No "Copy" item was found; place the "Open With" item at the top of the menu, with a trailing separator.
        insertOpenWithDefaultPDFMenuItem(menu, 0);
        [menu insertItem:[NSMenuItem separatorItem] atIndex:1];
    }

    return [menu autorelease];
}

// MARK: NSUserInterfaceValidations PROTOCOL IMPLEMENTATION

- (BOOL)validateUserInterfaceItem:(id <NSValidatedUserInterfaceItem>)item
{
    SEL action = [item action];
    if (action == @selector(_openWithFinder:))
        return [_pdfView document] != nil;
    return YES;
}

// MARK: PDFView delegate methods

- (void)PDFViewWillClickOnLink:(PDFView *)sender withURL:(NSURL *)URL
{
    _pdfViewController->linkClicked([URL absoluteString]);
}

- (void)PDFViewOpenPDFInNativeApplication:(PDFView *)sender
{
    _pdfViewController->openPDFInFinder();
}

- (void)PDFViewSavePDFToDownloadFolder:(PDFView *)sender
{
    _pdfViewController->savePDFToDownloadsFolder();
}

- (void)PDFViewPerformPrint:(PDFView *)sender
{
    _pdfViewController->print();
}

- (BOOL)forwardScrollWheelEvent:(NSEvent *)wheelEvent
{
    return _pdfViewController->forwardScrollWheelEvent(wheelEvent);
}

@end

namespace WebKit {

PassOwnPtr<PDFViewController> PDFViewController::create(WKView *wkView)
{
    return adoptPtr(new PDFViewController(wkView));
}

PDFViewController::PDFViewController(WKView *wkView)
    : m_wkView(wkView)
    , m_wkPDFView(adoptNS([[WKPDFView alloc] initWithFrame:[m_wkView bounds] PDFViewController:this]))
    , m_pdfView([m_wkPDFView.get() pdfView])
{
    [m_wkView addSubview:m_wkPDFView.get()];
}

PDFViewController::~PDFViewController()
{
    [m_wkPDFView.get() removeFromSuperview];
    [m_wkPDFView.get() invalidate];
    m_wkPDFView = nullptr;
}

WebPageProxy* PDFViewController::page() const
{
    return toImpl([m_wkView pageRef]);
}

NSView* PDFViewController::pdfView() const
{ 
    return m_wkPDFView.get(); 
}
    
static RetainPtr<CFDataRef> convertPostScriptDataSourceToPDF(const CoreIPC::DataReference& dataReference)
{
    // Convert PostScript to PDF using Quartz 2D API
    // http://developer.apple.com/documentation/GraphicsImaging/Conceptual/drawingwithquartz2d/dq_ps_convert/chapter_16_section_1.html
    
    CGPSConverterCallbacks callbacks = { 0, 0, 0, 0, 0, 0, 0, 0 };    
    RetainPtr<CGPSConverterRef> converter = adoptCF(CGPSConverterCreate(0, &callbacks, 0));
    ASSERT(converter);

    RetainPtr<NSData> nsData = adoptNS([[NSData alloc] initWithBytesNoCopy:const_cast<uint8_t*>(dataReference.data()) length:dataReference.size() freeWhenDone:NO]);   

    RetainPtr<CGDataProviderRef> provider = adoptCF(CGDataProviderCreateWithCFData((CFDataRef)nsData.get()));
    ASSERT(provider);

    RetainPtr<CFMutableDataRef> result = adoptCF(CFDataCreateMutable(kCFAllocatorDefault, 0));
    ASSERT(result);
    
    RetainPtr<CGDataConsumerRef> consumer = adoptCF(CGDataConsumerCreateWithCFData(result.get()));
    ASSERT(consumer);
    
    CGPSConverterConvert(converter.get(), provider.get(), consumer.get(), 0);

    if (!result)
        return 0;

    return result;
}

void PDFViewController::setPDFDocumentData(const String& mimeType, const String& suggestedFilename, const CoreIPC::DataReference& dataReference)
{
    if (equalIgnoringCase(mimeType, "application/postscript")) {
        m_pdfData = convertPostScriptDataSourceToPDF(dataReference);
        if (!m_pdfData)
            return;
        m_suggestedFilename = String(suggestedFilename + ".pdf");
    } else {
        // Make sure to copy the data.
        m_pdfData = adoptCF(CFDataCreate(0, dataReference.data(), dataReference.size()));
        m_suggestedFilename = suggestedFilename;
    }

    RetainPtr<PDFDocument> pdfDocument = adoptNS([[pdfDocumentClass() alloc] initWithData:(NSData *)m_pdfData.get()]);
    [m_wkPDFView.get() setDocument:pdfDocument.get()];
}

double PDFViewController::zoomFactor() const
{
    return [m_pdfView scaleFactor];
}

void PDFViewController::setZoomFactor(double zoomFactor)
{
    [m_pdfView setScaleFactor:zoomFactor];
}

Class PDFViewController::pdfDocumentClass()
{
    static Class pdfDocumentClass = [pdfKitBundle() classNamed:@"PDFDocument"];
    
    return pdfDocumentClass;
}

Class PDFViewController::pdfPreviewViewClass()
{
    static Class pdfPreviewViewClass = [pdfKitBundle() classNamed:@"PDFPreviewView"];
    
    return pdfPreviewViewClass;
}

bool PDFViewController::forwardScrollWheelEvent(NSEvent *wheelEvent)
{
    CGFloat deltaX = [wheelEvent deltaX];
    if ((deltaX > 0 && !page()->canGoBack()) || (deltaX < 0 && !page()->canGoForward()))
        return false;

    [m_wkView scrollWheel:wheelEvent];
    return true;
}

static IMP oldPDFViewScrollView_scrollWheel;

static WKPDFView *findEnclosingWKPDFView(NSView *view)
{
    for (NSView *superview = [view superview]; superview; superview = [superview superview]) {
        if ([superview isKindOfClass:[WKPDFView class]])
            return static_cast<WKPDFView *>(superview);
    }

    return nil;
}

static void PDFViewScrollView_scrollWheel(NSScrollView* self, SEL _cmd, NSEvent *wheelEvent)
{
    CGFloat deltaX = [wheelEvent deltaX];
    CGFloat deltaY = [wheelEvent deltaY];

    NSSize contentsSize = [[self documentView] bounds].size;
    NSRect visibleRect = [self documentVisibleRect];

    // We only want to forward the wheel events if the horizontal delta is non-zero,
    // and only if we're pinned to either the left or right side.
    // We also never want to forward momentum scroll events.
    if ([wheelEvent momentumPhase] == NSEventPhaseNone && deltaX && fabsf(deltaY) < fabsf(deltaX)
        && ((deltaX > 0 && visibleRect.origin.x <= 0) || (deltaX < 0 && contentsSize.width <= NSMaxX(visibleRect)))) {
    
        if (WKPDFView *pdfView = findEnclosingWKPDFView(self)) {
            if ([pdfView forwardScrollWheelEvent:wheelEvent])
                return;
        }
    }

    wtfCallIMP<void>(oldPDFViewScrollView_scrollWheel, self, _cmd, wheelEvent);
}

NSBundle* PDFViewController::pdfKitBundle()
{
    static NSBundle *pdfKitBundle;
    if (pdfKitBundle)
        return pdfKitBundle;

    NSString *pdfKitPath = [_NSPathForSystemFramework(@"Quartz.framework") stringByAppendingString:@"/Frameworks/PDFKit.framework"];
    if (!pdfKitPath) {
        LOG_ERROR("Couldn't find PDFKit.framework");
        return nil;
    }

    pdfKitBundle = [NSBundle bundleWithPath:pdfKitPath];
    if (![pdfKitBundle load])
        LOG_ERROR("Couldn't load PDFKit.framework");

    if (Class pdfViewScrollViewClass = [pdfKitBundle classNamed:@"PDFViewScrollView"]) {
        if (Method scrollWheel = class_getInstanceMethod(pdfViewScrollViewClass, @selector(scrollWheel:)))
            oldPDFViewScrollView_scrollWheel = method_setImplementation(scrollWheel, reinterpret_cast<IMP>(PDFViewScrollView_scrollWheel));
    }

    return pdfKitBundle;
}

NSPrintOperation *PDFViewController::makePrintOperation(NSPrintInfo *printInfo)
{
    return [[m_pdfView document] getPrintOperationForPrintInfo:printInfo autoRotate:YES];
}

void PDFViewController::openPDFInFinder()
{
    // We don't want to open the PDF until we have a document to write. (see 4892525).
    if (![m_pdfView document]) {
        NSBeep();
        return;
    }

    if (!m_temporaryPDFUUID) {
        ASSERT(m_pdfData);
        m_temporaryPDFUUID = createCanonicalUUIDString();
        page()->savePDFToTemporaryFolderAndOpenWithNativeApplicationRaw(m_suggestedFilename.get(), page()->mainFrame()->url(), CFDataGetBytePtr(m_pdfData.get()), CFDataGetLength(m_pdfData.get()), m_temporaryPDFUUID);
        return;
    }

    page()->openPDFFromTemporaryFolderWithNativeApplication(m_temporaryPDFUUID);
}

static void releaseCFData(unsigned char*, const void* data)
{
    ASSERT(CFGetTypeID(data) == CFDataGetTypeID());

    // Balanced by CFRetain in savePDFToDownloadsFolder.
    CFRelease(data);
}

void PDFViewController::savePDFToDownloadsFolder()
{
    // We don't want to write the file until we have a document to write. (see 5267607).
    if (![m_pdfView document]) {
        NSBeep();
        return;
    }

    ASSERT(m_pdfData);

    // Balanced by CFRelease in releaseCFData.
    CFRetain(m_pdfData.get());

    RefPtr<WebData> data = WebData::createWithoutCopying(CFDataGetBytePtr(m_pdfData.get()), CFDataGetLength(m_pdfData.get()), releaseCFData, m_pdfData.get());

    page()->saveDataToFileInDownloadsFolder(m_suggestedFilename.get(), page()->mainFrame()->mimeType(), page()->mainFrame()->url(), data.get());
}

void PDFViewController::linkClicked(const String& url)
{
    NSEvent* nsEvent = [NSApp currentEvent];
    WebMouseEvent event;
    switch ([nsEvent type]) {
    case NSLeftMouseUp:
    case NSRightMouseUp:
    case NSOtherMouseUp:
        event = WebEventFactory::createWebMouseEvent(nsEvent, m_pdfView);
    default:
        // For non mouse-clicks or for keyboard events, pass an empty WebMouseEvent
        // through.  The event is only used by the WebFrameLoaderClient to determine
        // the modifier keys and which mouse button is down.  These queries will be
        // valid with an empty event.
        break;
    }
    
    page()->linkClicked(url, event);
}

void PDFViewController::print()
{
    page()->printMainFrame();
}

void PDFViewController::findString(const String& string, FindOptions options, unsigned maxMatchCount)
{
    BOOL forward = !(options & FindOptionsBackwards);
    BOOL caseFlag = !(options & FindOptionsCaseInsensitive);
    BOOL wrapFlag = options & FindOptionsWrapAround;

    PDFSelection *selection = [m_wkPDFView.get() _nextMatchFor:string direction:forward caseSensitive:caseFlag wrap:wrapFlag fromSelection:[m_pdfView currentSelection] startInSelection:NO];
    if (!selection) {
        page()->didFailToFindString(string);
        return;
    }

    NSUInteger matchCount;
    if (!maxMatchCount) {
        // If the max was zero, any result means we exceeded the max. We can skip computing the actual count.
        matchCount = static_cast<unsigned>(kWKMoreThanMaximumMatchCount);
    } else {
        matchCount = [m_wkPDFView.get() _countMatches:string caseSensitive:caseFlag];
        if (matchCount > maxMatchCount)
            matchCount = static_cast<unsigned>(kWKMoreThanMaximumMatchCount);
    }

    [m_pdfView setCurrentSelection:selection];
    [m_pdfView scrollSelectionToVisible:nil];
    page()->didFindString(string, matchCount);
}

void PDFViewController::countStringMatches(const String& string, FindOptions options, unsigned maxMatchCount)
{
    BOOL caseFlag = !(options & FindOptionsCaseInsensitive);

    NSUInteger matchCount = [m_wkPDFView.get() _countMatches:string caseSensitive:caseFlag];
    if (matchCount > maxMatchCount)
        matchCount = maxMatchCount;
    page()->didCountStringMatches(string, matchCount);
}

} // namespace WebKit
