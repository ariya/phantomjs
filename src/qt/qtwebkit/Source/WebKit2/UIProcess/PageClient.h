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

#ifndef PageClient_h
#define PageClient_h

#include "ShareableBitmap.h"
#include "WebColorPicker.h"
#include "WebPageProxy.h"
#include "WebPopupMenuProxy.h"
#include <WebCore/AlternativeTextClient.h>
#include <WebCore/EditorClient.h>
#include <wtf/Forward.h>

#if PLATFORM(MAC)
#include "PluginComplexTextInputState.h"

OBJC_CLASS CALayer;

#if USE(APPKIT)
OBJC_CLASS WKView;
OBJC_CLASS NSTextAlternatives;
#endif
#endif

namespace WebCore {
    class Cursor;
    struct ViewportAttributes;
}

namespace WebKit {

class DrawingAreaProxy;
class FindIndicator;
class NativeWebKeyboardEvent;
#if ENABLE(TOUCH_EVENTS)
class NativeWebTouchEvent;
#endif
#if ENABLE(GESTURE_EVENTS)
class WebGestureEvent;
#endif
class WebContextMenuProxy;
class WebEditCommandProxy;
class WebPopupMenuProxy;
#if ENABLE(INPUT_TYPE_COLOR)
class WebColorPicker;
#endif

#if PLATFORM(MAC)
struct ColorSpaceData;
#endif

class PageClient {
public:
    virtual ~PageClient() { }

    // Create a new drawing area proxy for the given page.
    virtual PassOwnPtr<DrawingAreaProxy> createDrawingAreaProxy() = 0;

    // Tell the view to invalidate the given rect. The rect is in view coordinates.
    virtual void setViewNeedsDisplay(const WebCore::IntRect&) = 0;

    // Tell the view to immediately display its invalid rect.
    virtual void displayView() = 0;

    // Return true if scrollView() can copy bits in the view.
    virtual bool canScrollView() = 0;
    // Tell the view to scroll scrollRect by scrollOffset.
    virtual void scrollView(const WebCore::IntRect& scrollRect, const WebCore::IntSize& scrollOffset) = 0;

    // Return the size of the view the page is associated with.
    virtual WebCore::IntSize viewSize() = 0;

    // Return whether the view's containing window is active.
    virtual bool isViewWindowActive() = 0;

    // Return whether the view is focused.
    virtual bool isViewFocused() = 0;

    // Return whether the view is visible.
    virtual bool isViewVisible() = 0;

    // Return whether the view is in a window.
    virtual bool isViewInWindow() = 0;

    // Return the layer hosting mode for the view.
    virtual LayerHostingMode viewLayerHostingMode() { return LayerHostingModeDefault; }

    virtual void processDidCrash() = 0;
    virtual void didRelaunchProcess() = 0;
    virtual void pageClosed() = 0;

    virtual void preferencesDidChange() = 0;

    virtual void toolTipChanged(const String&, const String&) = 0;

#if USE(TILED_BACKING_STORE)
    virtual void pageDidRequestScroll(const WebCore::IntPoint&) = 0;
    virtual void didRenderFrame(const WebCore::IntSize& contentsSize, const WebCore::IntRect& coveredRect) = 0;
    virtual void pageTransitionViewportReady() = 0;
#endif
#if USE(COORDINATED_GRAPHICS)
    virtual void didFindZoomableArea(const WebCore::IntPoint&, const WebCore::IntRect&) = 0;
#endif
#if PLATFORM(QT)
    virtual void handleAuthenticationRequiredRequest(const String& hostname, const String& realm, const String& prefilledUsername, String& username, String& password) = 0;
    virtual void handleCertificateVerificationRequest(const String& hostname, bool& ignoreErrors) = 0;
    virtual void handleProxyAuthenticationRequiredRequest(const String& hostname, uint16_t port, const String& prefilledUsername, String& username, String& password) = 0;
    virtual void handleWillSetInputMethodState() = 0;
#endif // PLATFORM(QT).

#if PLATFORM(QT) || PLATFORM(EFL) || PLATFORM(GTK)
    virtual void updateTextInputState() = 0;
#endif // PLATFORM(QT) || PLATFORM(EFL) || PLATOFRM(GTK)

#if PLATFORM(QT) || PLATFORM(EFL) || PLATFORM(GTK)
    virtual void handleDownloadRequest(DownloadProxy*) = 0;
#endif // PLATFORM(QT) || PLATFORM(EFL) || PLATFORM(GTK)

#if PLATFORM(QT) || PLATFORM(EFL)
    virtual void didChangeContentsSize(const WebCore::IntSize&) = 0;
#endif

#if PLATFORM(QT) || PLATFORM(GTK)
    virtual void startDrag(const WebCore::DragData&, PassRefPtr<ShareableBitmap> dragImage) = 0;
#endif

    virtual void setCursor(const WebCore::Cursor&) = 0;
    virtual void setCursorHiddenUntilMouseMoves(bool) = 0;
    virtual void didChangeViewportProperties(const WebCore::ViewportAttributes&) = 0;

    virtual void registerEditCommand(PassRefPtr<WebEditCommandProxy>, WebPageProxy::UndoOrRedo) = 0;
    virtual void clearAllEditCommands() = 0;
    virtual bool canUndoRedo(WebPageProxy::UndoOrRedo) = 0;
    virtual void executeUndoRedo(WebPageProxy::UndoOrRedo) = 0;
#if PLATFORM(MAC)
    virtual void accessibilityWebProcessTokenReceived(const CoreIPC::DataReference&) = 0;
    virtual bool interpretKeyEvent(const NativeWebKeyboardEvent&, Vector<WebCore::KeypressCommand>&) = 0;
    virtual bool executeSavedCommandBySelector(const String& selector) = 0;
    virtual void setDragImage(const WebCore::IntPoint& clientPosition, PassRefPtr<ShareableBitmap> dragImage, bool isLinkDrag) = 0;
    virtual void updateSecureInputState() = 0;
    virtual void resetSecureInputState() = 0;
    virtual void notifyInputContextAboutDiscardedComposition() = 0;
    virtual void makeFirstResponder() = 0;
    virtual void setPromisedData(const String& pasteboardName, PassRefPtr<WebCore::SharedBuffer> imageBuffer, const String& filename, const String& extension, const String& title,
                                 const String& url, const String& visibleUrl, PassRefPtr<WebCore::SharedBuffer> archiveBuffer) = 0;
#endif
#if PLATFORM(GTK)
    virtual void getEditorCommandsForKeyEvent(const NativeWebKeyboardEvent&, const AtomicString&, Vector<WTF::String>&) = 0;
#endif
    virtual WebCore::FloatRect convertToDeviceSpace(const WebCore::FloatRect&) = 0;
    virtual WebCore::FloatRect convertToUserSpace(const WebCore::FloatRect&) = 0;
    virtual WebCore::IntPoint screenToWindow(const WebCore::IntPoint&) = 0;
    virtual WebCore::IntRect windowToScreen(const WebCore::IntRect&) = 0;
    
    virtual void doneWithKeyEvent(const NativeWebKeyboardEvent&, bool wasEventHandled) = 0;
#if ENABLE(GESTURE_EVENTS)
    virtual void doneWithGestureEvent(const WebGestureEvent&, bool wasEventHandled) = 0;
#endif
#if ENABLE(TOUCH_EVENTS)
    virtual void doneWithTouchEvent(const NativeWebTouchEvent&, bool wasEventHandled) = 0;
#endif

    virtual PassRefPtr<WebPopupMenuProxy> createPopupMenuProxy(WebPageProxy*) = 0;
    virtual PassRefPtr<WebContextMenuProxy> createContextMenuProxy(WebPageProxy*) = 0;

#if ENABLE(INPUT_TYPE_COLOR)
    virtual PassRefPtr<WebColorPicker> createColorPicker(WebPageProxy*, const WebCore::Color& initialColor, const WebCore::IntRect&) = 0;
#endif

    virtual void setFindIndicator(PassRefPtr<FindIndicator>, bool fadeOut, bool animate) = 0;

#if USE(ACCELERATED_COMPOSITING)
    virtual void enterAcceleratedCompositingMode(const LayerTreeContext&) = 0;
    virtual void exitAcceleratedCompositingMode() = 0;
    virtual void updateAcceleratedCompositingMode(const LayerTreeContext&) = 0;
#endif

#if PLATFORM(MAC)
    virtual void pluginFocusOrWindowFocusChanged(uint64_t pluginComplexTextInputIdentifier, bool pluginHasFocusAndWindowHasFocus) = 0;
    virtual void setPluginComplexTextInputState(uint64_t pluginComplexTextInputIdentifier, PluginComplexTextInputState) = 0;
    virtual CGContextRef containingWindowGraphicsContext() = 0;
    virtual void didPerformDictionaryLookup(const AttributedString&, const DictionaryPopupInfo&) = 0;
    virtual void dismissDictionaryLookupPanel() = 0;
    virtual void showCorrectionPanel(WebCore::AlternativeTextType, const WebCore::FloatRect& boundingBoxOfReplacedString, const String& replacedString, const String& replacementString, const Vector<String>& alternativeReplacementStrings) = 0;
    virtual void dismissCorrectionPanel(WebCore::ReasonForDismissingAlternativeText) = 0;
    virtual String dismissCorrectionPanelSoon(WebCore::ReasonForDismissingAlternativeText) = 0;
    virtual void recordAutocorrectionResponse(WebCore::AutocorrectionResponseType, const String& replacedString, const String& replacementString) = 0;
    virtual void recommendedScrollbarStyleDidChange(int32_t newStyle) = 0;

    virtual ColorSpaceData colorSpace() = 0;
    virtual void setAcceleratedCompositingRootLayer(CALayer *) = 0;

#if USE(APPKIT)
    virtual WKView* wkView() const = 0;
    virtual void intrinsicContentSizeDidChange(const WebCore::IntSize& intrinsicContentSize) = 0;
#if USE(DICTATION_ALTERNATIVES)
    virtual uint64_t addDictationAlternatives(const RetainPtr<NSTextAlternatives>&) = 0;
    virtual void removeDictationAlternatives(uint64_t dictationContext) = 0;
    virtual void showDictationAlternativeUI(const WebCore::FloatRect& boundingBoxOfDictatedText, uint64_t dictationContext) = 0;
    virtual Vector<String> dictationAlternatives(uint64_t dictationContext) = 0;
#endif // USE(DICTATION_ALTERNATIVES)
#endif // USE(APPKIT)
#endif // PLATFORM(MAC)

    virtual void flashBackingStoreUpdates(const Vector<WebCore::IntRect>& updateRects) = 0;
};

} // namespace WebKit

#endif // PageClient_h
