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
#import "PageClientImpl.h"

#if USE(DICTATION_ALTERNATIVES)
#import <AppKit/NSTextAlternatives.h>
#endif
#import "AttributedString.h"
#import "ColorSpaceData.h"
#import "DataReference.h"
#import "DictionaryPopupInfo.h"
#import "FindIndicator.h"
#import "NativeWebKeyboardEvent.h"
#import "WKAPICast.h"
#import "WKStringCF.h"
#import "WKViewInternal.h"
#import "StringUtilities.h"
#import "WebContextMenuProxyMac.h"
#import "WebEditCommandProxy.h"
#import "WebPopupMenuProxyMac.h"
#import <WebCore/AlternativeTextUIController.h>
#import <WebCore/BitmapImage.h>
#import <WebCore/Cursor.h>
#import <WebCore/FloatRect.h>
#import <WebCore/FoundationExtras.h>
#import <WebCore/GraphicsContext.h>
#import <WebCore/Image.h>
#import <WebCore/KeyboardEvent.h>
#import <WebCore/NotImplemented.h>
#import <WebCore/SharedBuffer.h>
#import <wtf/PassOwnPtr.h>
#import <wtf/text/CString.h>
#import <wtf/text/WTFString.h>
#import <WebKitSystemInterface.h>

@interface NSApplication (WebNSApplicationDetails)
- (NSCursor *)_cursorRectCursor;
@end

#if HAVE(LAYER_HOSTING_IN_WINDOW_SERVER)
@interface NSWindow (WebNSWindowDetails)
- (BOOL)_hostsLayersInWindowServer;
@end
#endif

using namespace WebCore;
using namespace WebKit;

@interface WKEditCommandObjC : NSObject
{
    RefPtr<WebEditCommandProxy> m_command;
}
- (id)initWithWebEditCommandProxy:(PassRefPtr<WebEditCommandProxy>)command;
- (WebEditCommandProxy*)command;
@end

@interface WKEditorUndoTargetObjC : NSObject
- (void)undoEditing:(id)sender;
- (void)redoEditing:(id)sender;
@end

@implementation WKEditCommandObjC

- (id)initWithWebEditCommandProxy:(PassRefPtr<WebEditCommandProxy>)command
{
    self = [super init];
    if (!self)
        return nil;

    m_command = command;
    return self;
}

- (WebEditCommandProxy*)command
{
    return m_command.get();
}

@end

@implementation WKEditorUndoTargetObjC

- (void)undoEditing:(id)sender
{
    ASSERT([sender isKindOfClass:[WKEditCommandObjC class]]);
    [sender command]->unapply();
}

- (void)redoEditing:(id)sender
{
    ASSERT([sender isKindOfClass:[WKEditCommandObjC class]]);
    [sender command]->reapply();
}

@end

namespace WebKit {

PassOwnPtr<PageClientImpl> PageClientImpl::create(WKView* wkView)
{
    return adoptPtr(new PageClientImpl(wkView));
}

PageClientImpl::PageClientImpl(WKView* wkView)
    : m_wkView(wkView)
    , m_undoTarget(adoptNS([[WKEditorUndoTargetObjC alloc] init]))
#if USE(DICTATION_ALTERNATIVES)
    , m_alternativeTextUIController(adoptPtr(new AlternativeTextUIController))
#endif
{
}

PageClientImpl::~PageClientImpl()
{
}

PassOwnPtr<DrawingAreaProxy> PageClientImpl::createDrawingAreaProxy()
{
    return [m_wkView _createDrawingAreaProxy];
}

void PageClientImpl::setViewNeedsDisplay(const WebCore::IntRect& rect)
{
    [m_wkView setNeedsDisplayInRect:rect];
}

void PageClientImpl::displayView()
{
    [m_wkView displayIfNeeded];
}

bool PageClientImpl::canScrollView()
{
    // -scrollRect:by: does nothing in layer-backed views <rdar://problem/12961719>.
    return ![m_wkView layer];
}

void PageClientImpl::scrollView(const IntRect& scrollRect, const IntSize& scrollOffset)
{
    NSRect clippedScrollRect = NSIntersectionRect(scrollRect, NSOffsetRect(scrollRect, -scrollOffset.width(), -scrollOffset.height()));

    [m_wkView _cacheWindowBottomCornerRect];

    [m_wkView translateRectsNeedingDisplayInRect:clippedScrollRect by:scrollOffset];
    [m_wkView scrollRect:clippedScrollRect by:scrollOffset];
}

IntSize PageClientImpl::viewSize()
{
    return IntSize([m_wkView bounds].size);
}

bool PageClientImpl::isViewWindowActive()
{
    return [[m_wkView window] isKeyWindow] || [NSApp keyWindow] == [m_wkView window];
}

bool PageClientImpl::isViewFocused()
{
    return [m_wkView _isFocused];
}

void PageClientImpl::makeFirstResponder()
{
     [[m_wkView window] makeFirstResponder:m_wkView];
}
    
bool PageClientImpl::isViewVisible()
{
    if (![m_wkView window])
        return false;

    if (![[m_wkView window] isVisible])
        return false;

#if __MAC_OS_X_VERSION_MIN_REQUIRED <= 1080
    // Mountain Lion and previous do not support occlusion notifications, and as such will
    // continue to report as "visible" when not on the active space.
    if (![[m_wkView window] isOnActiveSpace])
        return false;
#endif

    if ([m_wkView isHiddenOrHasHiddenAncestor])
        return false;

    if ([m_wkView _isWindowOccluded])
        return false;

    return true;
}

bool PageClientImpl::isViewInWindow()
{
    return [m_wkView window];
}

void PageClientImpl::viewWillMoveToAnotherWindow()
{
    clearAllEditCommands();
}

LayerHostingMode PageClientImpl::viewLayerHostingMode()
{
#if HAVE(LAYER_HOSTING_IN_WINDOW_SERVER)
    if (![m_wkView window])
        return LayerHostingModeDefault;

    return [[m_wkView window] _hostsLayersInWindowServer] ? LayerHostingModeInWindowServer : LayerHostingModeDefault;
#else
    return LayerHostingModeDefault;
#endif
}

ColorSpaceData PageClientImpl::colorSpace()
{
    return [m_wkView _colorSpace];
}

void PageClientImpl::processDidCrash()
{
    [m_wkView _processDidCrash];
}

void PageClientImpl::pageClosed()
{
    [m_wkView _pageClosed];
#if USE(DICTATION_ALTERNATIVES)
    m_alternativeTextUIController->clear();
#endif
}

void PageClientImpl::didRelaunchProcess()
{
    [m_wkView _didRelaunchProcess];
}

void PageClientImpl::preferencesDidChange()
{
    [m_wkView _preferencesDidChange];
}

void PageClientImpl::toolTipChanged(const String& oldToolTip, const String& newToolTip)
{
    [m_wkView _toolTipChangedFrom:nsStringFromWebCoreString(oldToolTip) to:nsStringFromWebCoreString(newToolTip)];
}

void PageClientImpl::setCursor(const WebCore::Cursor& cursor)
{
    if (![NSApp _cursorRectCursor])
        [m_wkView _setCursor:cursor.platformCursor()];
}

void PageClientImpl::setCursorHiddenUntilMouseMoves(bool hiddenUntilMouseMoves)
{
    [NSCursor setHiddenUntilMouseMoves:hiddenUntilMouseMoves];
}

void PageClientImpl::didChangeViewportProperties(const WebCore::ViewportAttributes&)
{
}

void PageClientImpl::registerEditCommand(PassRefPtr<WebEditCommandProxy> prpCommand, WebPageProxy::UndoOrRedo undoOrRedo)
{
    RefPtr<WebEditCommandProxy> command = prpCommand;

    RetainPtr<WKEditCommandObjC> commandObjC = adoptNS([[WKEditCommandObjC alloc] initWithWebEditCommandProxy:command]);
    String actionName = WebEditCommandProxy::nameForEditAction(command->editAction());

    NSUndoManager *undoManager = [m_wkView undoManager];
    [undoManager registerUndoWithTarget:m_undoTarget.get() selector:((undoOrRedo == WebPageProxy::Undo) ? @selector(undoEditing:) : @selector(redoEditing:)) object:commandObjC.get()];
    if (!actionName.isEmpty())
        [undoManager setActionName:(NSString *)actionName];
}

void PageClientImpl::clearAllEditCommands()
{
    [[m_wkView undoManager] removeAllActionsWithTarget:m_undoTarget.get()];
}

bool PageClientImpl::canUndoRedo(WebPageProxy::UndoOrRedo undoOrRedo)
{
    return (undoOrRedo == WebPageProxy::Undo) ? [[m_wkView undoManager] canUndo] : [[m_wkView undoManager] canRedo];
}

void PageClientImpl::executeUndoRedo(WebPageProxy::UndoOrRedo undoOrRedo)
{
    return (undoOrRedo == WebPageProxy::Undo) ? [[m_wkView undoManager] undo] : [[m_wkView undoManager] redo];
}

bool PageClientImpl::interpretKeyEvent(const NativeWebKeyboardEvent& event, Vector<WebCore::KeypressCommand>& commands)
{
    return [m_wkView _interpretKeyEvent:event.nativeEvent() savingCommandsTo:commands];
}

void PageClientImpl::setDragImage(const IntPoint& clientPosition, PassRefPtr<ShareableBitmap> dragImage, bool isLinkDrag)
{
    RetainPtr<CGImageRef> dragCGImage = dragImage->makeCGImage();
    RetainPtr<NSImage> dragNSImage = adoptNS([[NSImage alloc] initWithCGImage:dragCGImage.get() size:dragImage->size()]);

    [m_wkView _setDragImage:dragNSImage.get() at:clientPosition linkDrag:isLinkDrag];
}

void PageClientImpl::setPromisedData(const String& pasteboardName, PassRefPtr<SharedBuffer> imageBuffer, const String& filename, const String& extension, const String& title, const String& url, const String& visibleUrl, PassRefPtr<SharedBuffer> archiveBuffer)
{
    RefPtr<Image> image = BitmapImage::create();
    image->setData(imageBuffer.get(), true);
    [m_wkView _setPromisedData:image.get() withFileName:filename withExtension:extension withTitle:title withURL:url withVisibleURL:visibleUrl withArchive:archiveBuffer.get() forPasteboard:pasteboardName];
}

void PageClientImpl::updateSecureInputState()
{
    [m_wkView _updateSecureInputState];
}

void PageClientImpl::resetSecureInputState()
{
    [m_wkView _resetSecureInputState];
}

void PageClientImpl::notifyInputContextAboutDiscardedComposition()
{
    [m_wkView _notifyInputContextAboutDiscardedComposition];
}

FloatRect PageClientImpl::convertToDeviceSpace(const FloatRect& rect)
{
    return [m_wkView _convertToDeviceSpace:rect];
}

FloatRect PageClientImpl::convertToUserSpace(const FloatRect& rect)
{
    return [m_wkView _convertToUserSpace:rect];
}
   
IntPoint PageClientImpl::screenToWindow(const IntPoint& point)
{
    NSPoint windowCoord = [[m_wkView window] convertScreenToBase:point];
    return IntPoint([m_wkView convertPoint:windowCoord fromView:nil]);
}
    
IntRect PageClientImpl::windowToScreen(const IntRect& rect)
{
    NSRect tempRect = rect;
    tempRect = [m_wkView convertRect:tempRect toView:nil];
    tempRect.origin = [[m_wkView window] convertBaseToScreen:tempRect.origin];
    return enclosingIntRect(tempRect);
}

#if ENABLE(GESTURE_EVENTS)
void PageClientImpl::doneWithGestureEvent(const WebGestureEvent&, bool wasEventHandled)
{
    notImplemented();
}
#endif

void PageClientImpl::doneWithKeyEvent(const NativeWebKeyboardEvent& event, bool eventWasHandled)
{
    [m_wkView _doneWithKeyEvent:event.nativeEvent() eventWasHandled:eventWasHandled];
}

PassRefPtr<WebPopupMenuProxy> PageClientImpl::createPopupMenuProxy(WebPageProxy* page)
{
    return WebPopupMenuProxyMac::create(m_wkView, page);
}

PassRefPtr<WebContextMenuProxy> PageClientImpl::createContextMenuProxy(WebPageProxy* page)
{
    return WebContextMenuProxyMac::create(m_wkView, page);
}

#if ENABLE(INPUT_TYPE_COLOR)
PassRefPtr<WebColorPicker> PageClientImpl::createColorPicker(WebPageProxy*, const WebCore::Color&,  const WebCore::IntRect&)
{
    notImplemented();
    return 0;
}
#endif

void PageClientImpl::setFindIndicator(PassRefPtr<FindIndicator> findIndicator, bool fadeOut, bool animate)
{
    [m_wkView _setFindIndicator:findIndicator fadeOut:fadeOut animate:animate];
}

void PageClientImpl::accessibilityWebProcessTokenReceived(const CoreIPC::DataReference& data)
{
    NSData* remoteToken = [NSData dataWithBytes:data.data() length:data.size()];
    [m_wkView _setAccessibilityWebProcessToken:remoteToken];
}
    
void PageClientImpl::enterAcceleratedCompositingMode(const LayerTreeContext& layerTreeContext)
{
    ASSERT(!layerTreeContext.isEmpty());

    CALayer *renderLayer = WKMakeRenderLayer(layerTreeContext.contextID);
    [m_wkView _setAcceleratedCompositingModeRootLayer:renderLayer];
}

void PageClientImpl::exitAcceleratedCompositingMode()
{
    [m_wkView _setAcceleratedCompositingModeRootLayer:nil];
}

void PageClientImpl::updateAcceleratedCompositingMode(const LayerTreeContext& layerTreeContext)
{
    ASSERT(!layerTreeContext.isEmpty());

    CALayer *renderLayer = WKMakeRenderLayer(layerTreeContext.contextID);
    [m_wkView _setAcceleratedCompositingModeRootLayer:renderLayer];
}

void PageClientImpl::setAcceleratedCompositingRootLayer(CALayer *rootLayer)
{
    [m_wkView _setAcceleratedCompositingModeRootLayer:rootLayer];
}

void PageClientImpl::pluginFocusOrWindowFocusChanged(uint64_t pluginComplexTextInputIdentifier, bool pluginHasFocusAndWindowHasFocus)
{
    [m_wkView _pluginFocusOrWindowFocusChanged:pluginHasFocusAndWindowHasFocus pluginComplexTextInputIdentifier:pluginComplexTextInputIdentifier];
}

void PageClientImpl::setPluginComplexTextInputState(uint64_t pluginComplexTextInputIdentifier, PluginComplexTextInputState pluginComplexTextInputState)
{
    [m_wkView _setPluginComplexTextInputState:pluginComplexTextInputState pluginComplexTextInputIdentifier:pluginComplexTextInputIdentifier];
}

CGContextRef PageClientImpl::containingWindowGraphicsContext()
{
    NSWindow *window = [m_wkView window];

    // Don't try to get the graphics context if the NSWindow doesn't have a window device.
    if ([window windowNumber] <= 0)
        return 0;

    return static_cast<CGContextRef>([[window graphicsContext] graphicsPort]);
}

void PageClientImpl::flashBackingStoreUpdates(const Vector<IntRect>&)
{
    notImplemented();
}

void PageClientImpl::didPerformDictionaryLookup(const AttributedString& text, const DictionaryPopupInfo& dictionaryPopupInfo)
{
    RetainPtr<NSAttributedString> attributedString = text.string;
    NSPoint textBaselineOrigin = dictionaryPopupInfo.origin;

    // Convert to screen coordinates.
    textBaselineOrigin = [m_wkView convertPoint:textBaselineOrigin toView:nil];
    textBaselineOrigin = [m_wkView.window convertRectToScreen:NSMakeRect(textBaselineOrigin.x, textBaselineOrigin.y, 0, 0)].origin;

    WKShowWordDefinitionWindow(attributedString.get(), textBaselineOrigin, (NSDictionary *)dictionaryPopupInfo.options.get());
}

void PageClientImpl::dismissDictionaryLookupPanel()
{
    // FIXME: We don't know which panel we are dismissing, it may not even be in the current page (see <rdar://problem/13875766>).
    WKHideWordDefinitionWindow();
}

void PageClientImpl::showCorrectionPanel(AlternativeTextType type, const FloatRect& boundingBoxOfReplacedString, const String& replacedString, const String& replacementString, const Vector<String>& alternativeReplacementStrings)
{
#if USE(AUTOCORRECTION_PANEL)
    if (!isViewVisible() || !isViewInWindow())
        return;
    m_correctionPanel.show(m_wkView, type, boundingBoxOfReplacedString, replacedString, replacementString, alternativeReplacementStrings);
#endif
}

void PageClientImpl::dismissCorrectionPanel(ReasonForDismissingAlternativeText reason)
{
#if USE(AUTOCORRECTION_PANEL)
    m_correctionPanel.dismiss(reason);
#endif
}

String PageClientImpl::dismissCorrectionPanelSoon(WebCore::ReasonForDismissingAlternativeText reason)
{
#if USE(AUTOCORRECTION_PANEL)
    return m_correctionPanel.dismiss(reason);
#else
    return String();
#endif
}

void PageClientImpl::recordAutocorrectionResponse(AutocorrectionResponseType responseType, const String& replacedString, const String& replacementString)
{
    NSCorrectionResponse response = responseType == AutocorrectionReverted ? NSCorrectionResponseReverted : NSCorrectionResponseEdited;
    CorrectionPanel::recordAutocorrectionResponse(m_wkView, response, replacedString, replacementString);
}

void PageClientImpl::recommendedScrollbarStyleDidChange(int32_t newStyle)
{
    NSArray *trackingAreas = [m_wkView trackingAreas];
    NSUInteger count = [trackingAreas count];
    ASSERT(count == 1);
    
    for (NSUInteger i = 0; i < count; ++i)
        [m_wkView removeTrackingArea:[trackingAreas objectAtIndex:i]];

    // Now re-create a tracking area with the appropriate options given the new scrollbar style
    NSTrackingAreaOptions options = NSTrackingMouseMoved | NSTrackingMouseEnteredAndExited | NSTrackingInVisibleRect;
    if (newStyle == NSScrollerStyleLegacy)
        options |= NSTrackingActiveAlways;
    else
        options |= NSTrackingActiveInKeyWindow;

    NSTrackingArea *trackingArea = [[NSTrackingArea alloc] initWithRect:[m_wkView frame]
                                                                options:options
                                                                  owner:m_wkView
                                                               userInfo:nil];
    [m_wkView addTrackingArea:trackingArea];
    [trackingArea release];
}

void PageClientImpl::intrinsicContentSizeDidChange(const IntSize& intrinsicContentSize)
{
    [m_wkView _setIntrinsicContentSize:intrinsicContentSize];
}

bool PageClientImpl::executeSavedCommandBySelector(const String& selectorString)
{
    return [m_wkView _executeSavedCommandBySelector:NSSelectorFromString(selectorString)];
}

#if USE(DICTATION_ALTERNATIVES)
uint64_t PageClientImpl::addDictationAlternatives(const RetainPtr<NSTextAlternatives>& alternatives)
{
    return m_alternativeTextUIController->addAlternatives(alternatives);
}

void PageClientImpl::removeDictationAlternatives(uint64_t dictationContext)
{
    m_alternativeTextUIController->removeAlternatives(dictationContext);
}

void PageClientImpl::showDictationAlternativeUI(const WebCore::FloatRect& boundingBoxOfDictatedText, uint64_t dictationContext)
{
    if (!isViewVisible() || !isViewInWindow())
        return;
    m_alternativeTextUIController->showAlternatives(m_wkView, boundingBoxOfDictatedText, dictationContext, ^(NSString* acceptedAlternative){
        [m_wkView handleAcceptedAlternativeText:acceptedAlternative];
    });
}

Vector<String> PageClientImpl::dictationAlternatives(uint64_t dictationContext)
{
    return m_alternativeTextUIController->alternativesForContext(dictationContext);
}
#endif

} // namespace WebKit
