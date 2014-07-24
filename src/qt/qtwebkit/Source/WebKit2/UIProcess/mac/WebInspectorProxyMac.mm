/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
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
#import "WebInspectorProxy.h"

#if ENABLE(INSPECTOR)

#import "WKAPICast.h"
#import "WebContext.h"
#import "WKInspectorPrivateMac.h"
#import "WKMutableArray.h"
#import "WKOpenPanelParameters.h"
#import "WKOpenPanelResultListener.h"
#import "WKRetainPtr.h"
#import "WKURLCF.h"
#import "WKViewPrivate.h"
#import "WebInspectorMessages.h"
#import "WebPageGroup.h"
#import "WebPageProxy.h"
#import "WebPreferences.h"
#import "WebProcessProxy.h"
#import <algorithm>
#import <mach-o/dyld.h>
#import <WebKitSystemInterface.h>
#import <WebCore/InspectorFrontendClientLocal.h>
#import <WebCore/LocalizedStrings.h>
#import <WebCore/SoftLinking.h>
#import <wtf/text/WTFString.h>

SOFT_LINK_STAGED_FRAMEWORK(WebInspectorUI, PrivateFrameworks, A)

using namespace WebCore;
using namespace WebKit;

// The height needed to match a typical NSToolbar.
static const CGFloat windowContentBorderThickness = 55;

// The margin from the top and right of the dock button (same as the full screen button).
static const CGFloat dockButtonMargin = 3;

// The spacing between the dock buttons.
static const CGFloat dockButtonSpacing = dockButtonMargin * 2;

static const NSUInteger windowStyleMask = NSTitledWindowMask | NSClosableWindowMask | NSMiniaturizableWindowMask | NSResizableWindowMask | NSTexturedBackgroundWindowMask;

// WKWebInspectorProxyObjCAdapter is a helper ObjC object used as a delegate or notification observer
// for the sole purpose of getting back into the C++ code from an ObjC caller.

@interface WKWebInspectorProxyObjCAdapter ()

- (id)initWithWebInspectorProxy:(WebInspectorProxy*)inspectorProxy;
- (void)close;

@end

@implementation WKWebInspectorProxyObjCAdapter

- (WKInspectorRef)inspectorRef
{
    return toAPI(static_cast<WebInspectorProxy*>(_inspectorProxy));
}

- (id)initWithWebInspectorProxy:(WebInspectorProxy*)inspectorProxy
{
    ASSERT_ARG(inspectorProxy, inspectorProxy);

    if (!(self = [super init]))
        return nil;

    _inspectorProxy = static_cast<void*>(inspectorProxy); // Not retained to prevent cycles

    return self;
}

- (IBAction)attachRight:(id)sender
{
    static_cast<WebInspectorProxy*>(_inspectorProxy)->attach(AttachmentSideRight);
}

- (IBAction)attachBottom:(id)sender
{
    static_cast<WebInspectorProxy*>(_inspectorProxy)->attach(AttachmentSideBottom);
}

- (void)close
{
    _inspectorProxy = 0;
}

- (void)windowDidMove:(NSNotification *)notification
{
    static_cast<WebInspectorProxy*>(_inspectorProxy)->windowFrameDidChange();
}

- (void)windowDidResize:(NSNotification *)notification
{
    static_cast<WebInspectorProxy*>(_inspectorProxy)->windowFrameDidChange();
}

- (void)windowWillClose:(NSNotification *)notification
{
    static_cast<WebInspectorProxy*>(_inspectorProxy)->close();
}

- (void)inspectedViewFrameDidChange:(NSNotification *)notification
{
    // Resizing the views while inside this notification can lead to bad results when entering
    // or exiting full screen. To avoid that we need to perform the work after a delay. We only
    // depend on this for enforcing the height constraints, so a small delay isn't terrible. Most
    // of the time the views will already have the correct frames because of autoresizing masks.

    dispatch_after(DISPATCH_TIME_NOW, dispatch_get_main_queue(), ^{
        if (!_inspectorProxy)
            return;
        static_cast<WebInspectorProxy*>(_inspectorProxy)->inspectedViewFrameDidChange();
    });
}

@end

@interface WKWebInspectorWKView : WKView
@end

@implementation WKWebInspectorWKView

- (NSInteger)tag
{
    return WKInspectorViewTag;
}

@end

@interface NSWindow (AppKitDetails)
- (NSCursor *)_cursorForResizeDirection:(NSInteger)direction;
- (NSRect)_customTitleFrame;
@end

@interface WKWebInspectorWindow : NSWindow {
@public
    RetainPtr<NSButton> _dockBottomButton;
    RetainPtr<NSButton> _dockRightButton;
}
@end

@implementation WKWebInspectorWindow

- (NSCursor *)_cursorForResizeDirection:(NSInteger)direction
{
    // Don't show a resize cursor for the northeast (top right) direction if the dock button is visible.
    // This matches what happens when the full screen button is visible.
    if (direction == 1 && ![_dockRightButton isHidden])
        return nil;
    return [super _cursorForResizeDirection:direction];
}

- (NSRect)_customTitleFrame
{
    // Adjust the title frame if needed to prevent it from intersecting the dock button.
    NSRect titleFrame = [super _customTitleFrame];
    NSRect dockButtonFrame = _dockBottomButton.get().frame;
    if (NSMaxX(titleFrame) > NSMinX(dockButtonFrame) - dockButtonMargin)
        titleFrame.size.width -= (NSMaxX(titleFrame) - NSMinX(dockButtonFrame)) + dockButtonMargin;
    return titleFrame;
}

@end

namespace WebKit {

static bool inspectorReallyUsesWebKitUserInterface(WebPreferences* preferences)
{
    // This matches a similar check in WebInspectorMac.mm. Keep them in sync.

    // Call the soft link framework function to dlopen it, then [NSBundle bundleWithIdentifier:] will work.
    WebInspectorUILibrary();

    if (![[NSBundle bundleWithIdentifier:@"com.apple.WebInspectorUI"] pathForResource:@"Main" ofType:@"html"])
        return true;

    if (![[NSBundle bundleWithIdentifier:@"com.apple.WebCore"] pathForResource:@"inspector" ofType:@"html" inDirectory:@"inspector"])
        return false;

    return preferences->inspectorUsesWebKitUserInterface();
}

static WKRect getWindowFrame(WKPageRef, const void* clientInfo)
{
    WebInspectorProxy* webInspectorProxy = static_cast<WebInspectorProxy*>(const_cast<void*>(clientInfo));
    ASSERT(webInspectorProxy);

    return webInspectorProxy->inspectorWindowFrame();
}

static void setWindowFrame(WKPageRef, WKRect frame, const void* clientInfo)
{
    WebInspectorProxy* webInspectorProxy = static_cast<WebInspectorProxy*>(const_cast<void*>(clientInfo));
    ASSERT(webInspectorProxy);

    webInspectorProxy->setInspectorWindowFrame(frame);
}

static unsigned long long exceededDatabaseQuota(WKPageRef, WKFrameRef, WKSecurityOriginRef, WKStringRef, WKStringRef, unsigned long long, unsigned long long, unsigned long long currentDatabaseUsage, unsigned long long expectedUsage, const void*)
{
    return std::max<unsigned long long>(expectedUsage, currentDatabaseUsage * 1.25);
}

static void runOpenPanel(WKPageRef page, WKFrameRef frame, WKOpenPanelParametersRef parameters, WKOpenPanelResultListenerRef listener, const void* clientInfo)
{
    WebInspectorProxy* webInspectorProxy = static_cast<WebInspectorProxy*>(const_cast<void*>(clientInfo));
    ASSERT(webInspectorProxy);

    NSOpenPanel *openPanel = [NSOpenPanel openPanel];
    [openPanel setAllowsMultipleSelection:WKOpenPanelParametersGetAllowsMultipleFiles(parameters)];

    WKRetain(listener);

    // If the inspector is detached, then openPanel will be window-modal; otherwise, openPanel is opened in a new window.
    [openPanel beginSheetModalForWindow:webInspectorProxy->inspectorWindow() completionHandler:^(NSInteger result) {
        if (result == NSFileHandlingPanelOKButton) {
            WKMutableArrayRef fileURLs = WKMutableArrayCreate();

            for (NSURL* nsURL in [openPanel URLs]) {
                WKURLRef wkURL = WKURLCreateWithCFURL(reinterpret_cast<CFURLRef>(nsURL));
                WKArrayAppendItem(fileURLs, wkURL);
                WKRelease(wkURL);
            }

            WKOpenPanelResultListenerChooseFiles(listener, fileURLs);

            WKRelease(fileURLs);
        } else
            WKOpenPanelResultListenerCancel(listener);
        
        WKRelease(listener);
    }];
}

void WebInspectorProxy::setInspectorWindowFrame(WKRect& frame)
{
    if (m_isAttached)
        return;
    [m_inspectorWindow setFrame:NSMakeRect(frame.origin.x, frame.origin.y, frame.size.width, frame.size.height) display:YES];
}

WKRect WebInspectorProxy::inspectorWindowFrame()
{
    if (m_isAttached)
        return WKRectMake(0, 0, 0, 0);

    NSRect frame = m_inspectorWindow.get().frame;
    return WKRectMake(frame.origin.x, frame.origin.y, frame.size.width, frame.size.height);
}

static NSButton *createDockButton(NSString *imageName)
{
    // Create a full screen button so we can turn it into a dock button.
    NSButton *dockButton = [NSWindow standardWindowButton:NSWindowFullScreenButton forStyleMask:windowStyleMask];

    // Set the autoresizing mask to keep the dock button pinned to the top right corner.
    dockButton.autoresizingMask = NSViewMinXMargin | NSViewMinYMargin;

    // Get the dock image and make it a template so the button cell effects will apply.
    NSImage *dockImage = [[NSBundle bundleForClass:[WKWebInspectorWKView class]] imageForResource:imageName];
    [dockImage setTemplate:YES];

    // Set the dock image on the button cell.
    NSCell *dockButtonCell = dockButton.cell;
    dockButtonCell.image = dockImage;

    return [dockButton retain];
}

void WebInspectorProxy::createInspectorWindow()
{
    ASSERT(!m_inspectorWindow);

    NSRect windowFrame = NSMakeRect(0, 0, initialWindowWidth, initialWindowHeight);

    // Restore the saved window frame, if there was one.
    NSString *savedWindowFrameString = page()->pageGroup()->preferences()->inspectorWindowFrame();
    NSRect savedWindowFrame = NSRectFromString(savedWindowFrameString);
    if (!NSIsEmptyRect(savedWindowFrame))
        windowFrame = savedWindowFrame;

    WKWebInspectorWindow *window = [[WKWebInspectorWindow alloc] initWithContentRect:windowFrame styleMask:windowStyleMask backing:NSBackingStoreBuffered defer:NO];
    [window setDelegate:m_inspectorProxyObjCAdapter.get()];
    [window setMinSize:NSMakeSize(minimumWindowWidth, minimumWindowHeight)];
    [window setReleasedWhenClosed:NO];
    [window setAutorecalculatesContentBorderThickness:NO forEdge:NSMaxYEdge];
    [window setContentBorderThickness:windowContentBorderThickness forEdge:NSMaxYEdge];
    WKNSWindowMakeBottomCornersSquare(window);

    m_inspectorWindow = adoptNS(window);

    NSView *contentView = [window contentView];

    static const int32_t firstVersionOfSafariWithDockToRightSupport = 0x02181d0d; // 536.29.13
    static bool supportsDockToRight = NSVersionOfLinkTimeLibrary("Safari") >= firstVersionOfSafariWithDockToRightSupport;

    m_dockBottomButton = adoptNS(createDockButton(@"DockBottom"));
    m_dockRightButton = adoptNS(createDockButton(@"DockRight"));

    m_dockBottomButton.get().target = m_inspectorProxyObjCAdapter.get();
    m_dockBottomButton.get().action = @selector(attachBottom:);

    m_dockRightButton.get().target = m_inspectorProxyObjCAdapter.get();
    m_dockRightButton.get().action = @selector(attachRight:);
    m_dockRightButton.get().enabled = supportsDockToRight;
    m_dockRightButton.get().alphaValue = supportsDockToRight ? 1 : 0.5;

    // Store the dock buttons on the window too so it can check its visibility.
    window->_dockBottomButton = m_dockBottomButton;
    window->_dockRightButton = m_dockRightButton;

    // Get the frame view, the superview of the content view, and its frame.
    // This will be the superview of the dock button too.
    NSView *frameView = contentView.superview;
    NSRect frameViewBounds = frameView.bounds;
    NSSize dockButtonSize = m_dockBottomButton.get().frame.size;

    ASSERT(!frameView.isFlipped);

    // Position the dock button in the corner to match where the full screen button is normally.
    NSPoint dockButtonOrigin;
    dockButtonOrigin.x = NSMaxX(frameViewBounds) - dockButtonSize.width - dockButtonMargin;
    dockButtonOrigin.y = NSMaxY(frameViewBounds) - dockButtonSize.height - dockButtonMargin;
    m_dockRightButton.get().frameOrigin = dockButtonOrigin;

    dockButtonOrigin.x -= dockButtonSize.width + dockButtonSpacing;
    m_dockBottomButton.get().frameOrigin = dockButtonOrigin;

    [frameView addSubview:m_dockBottomButton.get()];
    [frameView addSubview:m_dockRightButton.get()];

    // Hide the dock buttons if we can't attach.
    m_dockBottomButton.get().hidden = !canAttach();
    m_dockRightButton.get().hidden = !canAttach();

    [m_inspectorView.get() setFrame:[contentView bounds]];
    [m_inspectorView.get() setAutoresizingMask:NSViewWidthSizable | NSViewHeightSizable];
    [contentView addSubview:m_inspectorView.get()];

    // Center the window if the saved frame was empty.
    if (NSIsEmptyRect(savedWindowFrame))
        [window center];

    updateInspectorWindowTitle();
}

void WebInspectorProxy::updateInspectorWindowTitle() const
{
    if (!m_inspectorWindow)
        return;

    NSString *title = [NSString stringWithFormat:WEB_UI_STRING("Web Inspector — %@", "Web Inspector window title"), (NSString *)m_urlString];
    [m_inspectorWindow.get() setTitle:title];
}

WebPageProxy* WebInspectorProxy::platformCreateInspectorPage()
{
    ASSERT(m_page);
    ASSERT(!m_inspectorView);

    NSRect initialRect;
    if (m_isAttached) {
        NSRect inspectedViewFrame = m_page->wkView().frame;

        switch (m_attachmentSide) {
        case AttachmentSideBottom:
            initialRect = NSMakeRect(0, 0, NSWidth(inspectedViewFrame), inspectorPageGroup()->preferences()->inspectorAttachedHeight());
            break;
        case AttachmentSideRight:
            initialRect = NSMakeRect(0, 0, inspectorPageGroup()->preferences()->inspectorAttachedWidth(), NSHeight(inspectedViewFrame));
            break;
        }
    } else {
        initialRect = NSMakeRect(0, 0, initialWindowWidth, initialWindowHeight);

        NSString *windowFrameString = page()->pageGroup()->preferences()->inspectorWindowFrame();
        NSRect windowFrame = NSRectFromString(windowFrameString);
        if (!NSIsEmptyRect(windowFrame))
            initialRect = [NSWindow contentRectForFrameRect:windowFrame styleMask:windowStyleMask];
    }

    m_inspectorView = adoptNS([[WKWebInspectorWKView alloc] initWithFrame:initialRect contextRef:toAPI(page()->process()->context()) pageGroupRef:toAPI(inspectorPageGroup()) relatedToPage:toAPI(m_page)]);
    ASSERT(m_inspectorView);

    [m_inspectorView.get() setDrawsBackground:NO];

    m_inspectorProxyObjCAdapter = adoptNS([[WKWebInspectorProxyObjCAdapter alloc] initWithWebInspectorProxy:this]);

    WebPageProxy* inspectorPage = toImpl(m_inspectorView.get().pageRef);

    WKPageUIClient uiClient = {
        kWKPageUIClientCurrentVersion,
        this,   /* clientInfo */
        0, // createNewPage_deprecatedForUseWithV0
        0, // showPage
        0, // closePage
        0, // takeFocus
        0, // focus
        0, // unfocus
        0, // runJavaScriptAlert
        0, // runJavaScriptConfirm
        0, // runJavaScriptPrompt
        0, // setStatusText
        0, // mouseDidMoveOverElement_deprecatedForUseWithV0
        0, // missingPluginButtonClicked_deprecatedForUseWithV0
        0, // didNotHandleKeyEvent
        0, // didNotHandleWheelEvent
        0, // areToolbarsVisible
        0, // setToolbarsVisible
        0, // isMenuBarVisible
        0, // setMenuBarVisible
        0, // isStatusBarVisible
        0, // setStatusBarVisible
        0, // isResizable
        0, // setResizable
        getWindowFrame,
        setWindowFrame,
        0, // runBeforeUnloadConfirmPanel
        0, // didDraw
        0, // pageDidScroll
        exceededDatabaseQuota,
        runOpenPanel,
        0, // decidePolicyForGeolocationPermissionRequest
        0, // headerHeight
        0, // footerHeight
        0, // drawHeader
        0, // drawFooter
        0, // printFrame
        0, // runModal
        0, // unused
        0, // saveDataToFileInDownloadsFolder
        0, // shouldInterruptJavaScript
        0, // createPage
        0, // mouseDidMoveOverElement
        0, // decidePolicyForNotificationPermissionRequest
        0, // unavailablePluginButtonClicked_deprecatedForUseWithV1
        0, // showColorPicker
        0, // hideColorPicker
        0, // unavailablePluginButtonClicked
    };

    inspectorPage->initializeUIClient(&uiClient);

    return inspectorPage;
}

void WebInspectorProxy::platformOpen()
{
    if (m_isAttached)
        platformAttach();
    else
        createInspectorWindow();

    platformBringToFront();
}

void WebInspectorProxy::platformDidClose()
{
    if (m_inspectorWindow) {
        [m_inspectorWindow.get() setDelegate:nil];
        [m_inspectorWindow.get() orderOut:nil];
        m_inspectorWindow = 0;
    }

    m_inspectorView = 0;

    [m_inspectorProxyObjCAdapter.get() close];
    m_inspectorProxyObjCAdapter = 0;
}

void WebInspectorProxy::platformHide()
{
    if (m_isAttached) {
        platformDetach();
        return;
    }

    if (m_inspectorWindow) {
        [m_inspectorWindow.get() setDelegate:nil];
        [m_inspectorWindow.get() orderOut:nil];
        m_inspectorWindow = 0;
    }
}

void WebInspectorProxy::platformBringToFront()
{
    // If the Web Inspector is no longer in the same window as the inspected view,
    // then we need to reopen the Inspector to get it attached to the right window.
    // This can happen when dragging tabs to another window in Safari.
    if (m_isAttached && m_inspectorView.get().window != m_page->wkView().window) {
        platformOpen();
        return;
    }

    // FIXME <rdar://problem/10937688>: this will not bring a background tab in Safari to the front, only its window.
    [m_inspectorView.get().window makeKeyAndOrderFront:nil];
    [m_inspectorView.get().window makeFirstResponder:m_inspectorView.get()];
}

bool WebInspectorProxy::platformIsFront()
{
    // FIXME <rdar://problem/10937688>: this will not return false for a background tab in Safari, only a background window.
    return m_isVisible && [m_inspectorView.get().window isMainWindow];
}

void WebInspectorProxy::platformAttachAvailabilityChanged(bool available)
{
    m_dockBottomButton.get().hidden = !available;
    m_dockRightButton.get().hidden = !available;
}

void WebInspectorProxy::platformInspectedURLChanged(const String& urlString)
{
    m_urlString = urlString;

    updateInspectorWindowTitle();
}

void WebInspectorProxy::platformSave(const String& suggestedURL, const String& content, bool forceSaveDialog)
{
    ASSERT(!suggestedURL.isEmpty());
    
    NSURL *platformURL = m_suggestedToActualURLMap.get(suggestedURL).get();
    if (!platformURL) {
        platformURL = [NSURL URLWithString:suggestedURL];
        // The user must confirm new filenames before we can save to them.
        forceSaveDialog = true;
    }
    
    ASSERT(platformURL);
    if (!platformURL)
        return;

    // Necessary for the block below.
    String suggestedURLCopy = suggestedURL;
    String contentCopy = content;

    auto saveToURL = ^(NSURL *actualURL) {
        ASSERT(actualURL);
        
        m_suggestedToActualURLMap.set(suggestedURLCopy, actualURL);
        [contentCopy writeToURL:actualURL atomically:YES encoding:NSUTF8StringEncoding error:NULL];
        m_page->process()->send(Messages::WebInspector::DidSave([actualURL absoluteString]), m_page->pageID());
    };

    if (!forceSaveDialog) {
        saveToURL(platformURL);
        return;
    }

    NSSavePanel *panel = [NSSavePanel savePanel];
    panel.nameFieldStringValue = platformURL.lastPathComponent;
    panel.directoryURL = [platformURL URLByDeletingLastPathComponent];

    [panel beginSheetModalForWindow:m_inspectorWindow.get() completionHandler:^(NSInteger result) {
        if (result == NSFileHandlingPanelCancelButton)
            return;
        ASSERT(result == NSFileHandlingPanelOKButton);
        saveToURL(panel.URL);
    }];
}

void WebInspectorProxy::platformAppend(const String& suggestedURL, const String& content)
{
    ASSERT(!suggestedURL.isEmpty());
    
    RetainPtr<NSURL> actualURL = m_suggestedToActualURLMap.get(suggestedURL);
    // Do not append unless the user has already confirmed this filename in save().
    if (!actualURL)
        return;

    NSFileHandle *handle = [NSFileHandle fileHandleForWritingToURL:actualURL.get() error:NULL];
    [handle seekToEndOfFile];
    [handle writeData:[content dataUsingEncoding:NSUTF8StringEncoding]];
    [handle closeFile];

    m_page->process()->send(Messages::WebInspector::DidAppend([actualURL absoluteString]), m_page->pageID());
}

void WebInspectorProxy::windowFrameDidChange()
{
    ASSERT(!m_isAttached);
    ASSERT(m_isVisible);
    ASSERT(m_inspectorWindow);

    if (m_isAttached || !m_isVisible || !m_inspectorWindow)
        return;

    NSString *frameString = NSStringFromRect([m_inspectorWindow frame]);
    page()->pageGroup()->preferences()->setInspectorWindowFrame(frameString);
}

void WebInspectorProxy::inspectedViewFrameDidChange(CGFloat currentDimension)
{
    if (!m_isAttached || !m_isVisible)
        return;

    WKView *inspectedView = m_page->wkView();
    NSRect inspectedViewFrame = [inspectedView frame];
    NSRect inspectorFrame = NSZeroRect;
    NSRect parentBounds = [[inspectedView superview] bounds];
    CGFloat inspectedViewTop = NSMaxY(inspectedViewFrame);

    switch (m_attachmentSide) {
        case AttachmentSideBottom: {
            if (!currentDimension)
                currentDimension = NSHeight([m_inspectorView.get() frame]);

            CGFloat parentHeight = NSHeight(parentBounds);
            CGFloat inspectorHeight = InspectorFrontendClientLocal::constrainedAttachedWindowHeight(currentDimension, parentHeight);

            // Preserve the top position of the inspected view so banners in Safari still work.
            inspectedViewFrame = NSMakeRect(0, inspectorHeight, NSWidth(parentBounds), inspectedViewTop - inspectorHeight);
            inspectorFrame = NSMakeRect(0, 0, NSWidth(inspectedViewFrame), inspectorHeight);
            break;
        }

        case AttachmentSideRight: {
            if (!currentDimension)
                currentDimension = NSWidth([m_inspectorView.get() frame]);

            CGFloat parentWidth = NSWidth(parentBounds);
            CGFloat inspectorWidth = InspectorFrontendClientLocal::constrainedAttachedWindowWidth(currentDimension, parentWidth);

            // Preserve the top position of the inspected view so banners in Safari still work. But don't use that
            // top position for the inspector view since the banners only stretch as wide as the the inspected view.
            inspectedViewFrame = NSMakeRect(0, 0, parentWidth - inspectorWidth, inspectedViewTop);
            inspectorFrame = NSMakeRect(parentWidth - inspectorWidth, 0, inspectorWidth, NSHeight(parentBounds));
            break;
        }
    }

    // Disable screen updates to make sure the layers for both views resize in sync.
    [[m_inspectorView window] disableScreenUpdatesUntilFlush];

    [m_inspectorView setFrame:inspectorFrame];
    [inspectedView setFrame:inspectedViewFrame];
}

unsigned WebInspectorProxy::platformInspectedWindowHeight()
{
    WKView *inspectedView = m_page->wkView();
    NSRect inspectedViewRect = [inspectedView frame];
    return static_cast<unsigned>(inspectedViewRect.size.height);
}

unsigned WebInspectorProxy::platformInspectedWindowWidth()
{
    WKView *inspectedView = m_page->wkView();
    NSRect inspectedViewRect = [inspectedView frame];
    return static_cast<unsigned>(inspectedViewRect.size.width);
}

void WebInspectorProxy::platformAttach()
{
    WKView *inspectedView = m_page->wkView();
    [[NSNotificationCenter defaultCenter] addObserver:m_inspectorProxyObjCAdapter.get() selector:@selector(inspectedViewFrameDidChange:) name:NSViewFrameDidChangeNotification object:inspectedView];

    if (m_inspectorWindow) {
        [m_inspectorWindow.get() setDelegate:nil];
        [m_inspectorWindow.get() orderOut:nil];
        m_inspectorWindow = 0;
    }

    [m_inspectorView.get() removeFromSuperview];

    [m_inspectorView.get() setAutoresizingMask:NSViewWidthSizable | NSViewMaxYMargin];

    CGFloat currentDimension;

    switch (m_attachmentSide) {
    case AttachmentSideBottom:
        currentDimension = inspectorPageGroup()->preferences()->inspectorAttachedHeight();
        break;
    case AttachmentSideRight:
        currentDimension = inspectorPageGroup()->preferences()->inspectorAttachedWidth();
        break;
    }

    inspectedViewFrameDidChange(currentDimension);

    [[inspectedView superview] addSubview:m_inspectorView.get() positioned:NSWindowBelow relativeTo:inspectedView];

    [[inspectedView window] makeFirstResponder:m_inspectorView.get()];
}

void WebInspectorProxy::platformDetach()
{
    WKView *inspectedView = m_page->wkView();
    [[NSNotificationCenter defaultCenter] removeObserver:m_inspectorProxyObjCAdapter.get() name:NSViewFrameDidChangeNotification object:inspectedView];

    [m_inspectorView.get() removeFromSuperview];

    // Make sure that we size the inspected view's frame after detaching so that it takes up the space that the
    // attached inspector used to. Preserve the top position of the inspected view so banners in Safari still work.

    inspectedView.frame = NSMakeRect(0, 0, NSWidth(inspectedView.superview.bounds), NSMaxY(inspectedView.frame));

    // Return early if we are not visible. This means the inspector was closed while attached
    // and we should not create and show the inspector window.
    if (!m_isVisible)
        return;

    createInspectorWindow();

    platformBringToFront();
}

void WebInspectorProxy::platformSetAttachedWindowHeight(unsigned height)
{
    if (!m_isAttached)
        return;

    inspectedViewFrameDidChange(height);
}

void WebInspectorProxy::platformSetAttachedWindowWidth(unsigned width)
{
    if (!m_isAttached)
        return;

    inspectedViewFrameDidChange(width);
}

void WebInspectorProxy::platformSetToolbarHeight(unsigned height)
{
    [m_inspectorWindow setContentBorderThickness:height forEdge:NSMaxYEdge];
}

String WebInspectorProxy::inspectorPageURL() const
{
    NSString *path;
    if (inspectorReallyUsesWebKitUserInterface(page()->pageGroup()->preferences()))
        path = [[NSBundle bundleWithIdentifier:@"com.apple.WebCore"] pathForResource:@"inspector" ofType:@"html" inDirectory:@"inspector"];
    else
        path = [[NSBundle bundleWithIdentifier:@"com.apple.WebInspectorUI"] pathForResource:@"Main" ofType:@"html"];

    ASSERT([path length]);

    return [[NSURL fileURLWithPath:path] absoluteString];
}

String WebInspectorProxy::inspectorBaseURL() const
{
    NSString *path;
    if (inspectorReallyUsesWebKitUserInterface(page()->pageGroup()->preferences()))
        path = [[NSBundle bundleWithIdentifier:@"com.apple.WebCore"] resourcePath];
    else
        path = [[NSBundle bundleWithIdentifier:@"com.apple.WebInspectorUI"] resourcePath];

    ASSERT([path length]);

    return [[NSURL fileURLWithPath:path] absoluteString];
}

} // namespace WebKit

#endif // ENABLE(INSPECTOR)
