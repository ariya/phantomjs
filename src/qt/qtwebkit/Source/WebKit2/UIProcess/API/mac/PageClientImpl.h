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

#ifndef PageClientImpl_h
#define PageClientImpl_h

#include "CorrectionPanel.h"
#include "PageClient.h"
#include <wtf/RetainPtr.h>

@class WKEditorUndoTargetObjC;
@class WKView;

namespace WebCore {
class AlternativeTextUIController;
}

namespace WebKit {
class FindIndicatorWindow;

class PageClientImpl FINAL : public PageClient {
public:
    static PassOwnPtr<PageClientImpl> create(WKView*);
    virtual ~PageClientImpl();
    
    void viewWillMoveToAnotherWindow();

private:
    explicit PageClientImpl(WKView*);

    virtual PassOwnPtr<DrawingAreaProxy> createDrawingAreaProxy();
    virtual void setViewNeedsDisplay(const WebCore::IntRect&);
    virtual void displayView();
    virtual bool canScrollView();
    virtual void scrollView(const WebCore::IntRect& scrollRect, const WebCore::IntSize& scrollOffset);

    virtual WebCore::IntSize viewSize();
    virtual bool isViewWindowActive();
    virtual bool isViewFocused();
    virtual bool isViewVisible();
    virtual bool isViewInWindow();
    virtual LayerHostingMode viewLayerHostingMode() OVERRIDE;
    virtual ColorSpaceData colorSpace() OVERRIDE;
    virtual void setAcceleratedCompositingRootLayer(CALayer *) OVERRIDE;

    virtual void processDidCrash();
    virtual void pageClosed();
    virtual void didRelaunchProcess();
    virtual void preferencesDidChange() OVERRIDE;
    virtual void toolTipChanged(const String& oldToolTip, const String& newToolTip);
    virtual void setCursor(const WebCore::Cursor&);
    virtual void setCursorHiddenUntilMouseMoves(bool);
    virtual void didChangeViewportProperties(const WebCore::ViewportAttributes&);

    virtual void registerEditCommand(PassRefPtr<WebEditCommandProxy>, WebPageProxy::UndoOrRedo);
    virtual void clearAllEditCommands();
    virtual bool canUndoRedo(WebPageProxy::UndoOrRedo);
    virtual void executeUndoRedo(WebPageProxy::UndoOrRedo);
    virtual bool interpretKeyEvent(const NativeWebKeyboardEvent&, Vector<WebCore::KeypressCommand>&);
    virtual bool executeSavedCommandBySelector(const String& selector);
    virtual void setDragImage(const WebCore::IntPoint& clientPosition, PassRefPtr<ShareableBitmap> dragImage, bool isLinkDrag);
    virtual void setPromisedData(const String& pasteboardName, PassRefPtr<WebCore::SharedBuffer> imageBuffer, const String& filename, const String& extension, const String& title,
                                 const String& url, const String& visibleUrl, PassRefPtr<WebCore::SharedBuffer> archiveBuffer);
    virtual void updateSecureInputState() OVERRIDE;
    virtual void resetSecureInputState() OVERRIDE;
    virtual void notifyInputContextAboutDiscardedComposition() OVERRIDE;

    virtual WebCore::FloatRect convertToDeviceSpace(const WebCore::FloatRect&);
    virtual WebCore::FloatRect convertToUserSpace(const WebCore::FloatRect&);
    virtual WebCore::IntPoint screenToWindow(const WebCore::IntPoint&);
    virtual WebCore::IntRect windowToScreen(const WebCore::IntRect&);

#if ENABLE(GESTURE_EVENTS)
    virtual void doneWithGestureEvent(const WebGestureEvent&, bool wasEventHandled);
#endif
    virtual void doneWithKeyEvent(const NativeWebKeyboardEvent&, bool wasEventHandled);

    virtual PassRefPtr<WebPopupMenuProxy> createPopupMenuProxy(WebPageProxy*);
    virtual PassRefPtr<WebContextMenuProxy> createContextMenuProxy(WebPageProxy*);

#if ENABLE(INPUT_TYPE_COLOR)
    virtual PassRefPtr<WebColorPicker> createColorPicker(WebPageProxy*, const WebCore::Color& initialColor, const WebCore::IntRect&);
#endif

    void setFindIndicator(PassRefPtr<FindIndicator>, bool fadeOut, bool animate);

    virtual void enterAcceleratedCompositingMode(const LayerTreeContext&);
    virtual void exitAcceleratedCompositingMode();
    virtual void updateAcceleratedCompositingMode(const LayerTreeContext&);

    virtual void accessibilityWebProcessTokenReceived(const CoreIPC::DataReference&);

    virtual void pluginFocusOrWindowFocusChanged(uint64_t pluginComplexTextInputIdentifier, bool pluginHasFocusAndWindowHasFocus);
    virtual void setPluginComplexTextInputState(uint64_t pluginComplexTextInputIdentifier, PluginComplexTextInputState);

    virtual void makeFirstResponder();
    
    virtual CGContextRef containingWindowGraphicsContext();

    virtual void flashBackingStoreUpdates(const Vector<WebCore::IntRect>& updateRects);

    virtual void didPerformDictionaryLookup(const AttributedString&, const DictionaryPopupInfo&);
    virtual void dismissDictionaryLookupPanel();

    virtual void showCorrectionPanel(WebCore::AlternativeTextType, const WebCore::FloatRect& boundingBoxOfReplacedString, const String& replacedString, const String& replacementString, const Vector<String>& alternativeReplacementStrings);
    virtual void dismissCorrectionPanel(WebCore::ReasonForDismissingAlternativeText);
    virtual String dismissCorrectionPanelSoon(WebCore::ReasonForDismissingAlternativeText);
    virtual void recordAutocorrectionResponse(WebCore::AutocorrectionResponseType, const String& replacedString, const String& replacementString);

    virtual void recommendedScrollbarStyleDidChange(int32_t newStyle);

    virtual WKView* wkView() const { return m_wkView; }
    virtual void intrinsicContentSizeDidChange(const WebCore::IntSize& intrinsicContentSize) OVERRIDE;

#if USE(DICTATION_ALTERNATIVES)
    virtual uint64_t addDictationAlternatives(const RetainPtr<NSTextAlternatives>&);
    virtual void removeDictationAlternatives(uint64_t dictationContext);
    virtual void showDictationAlternativeUI(const WebCore::FloatRect& boundingBoxOfDictatedText, uint64_t dictationContext);
    virtual Vector<String> dictationAlternatives(uint64_t dictationContext);
#endif

    WKView* m_wkView;
    RetainPtr<WKEditorUndoTargetObjC> m_undoTarget;
#if USE(AUTOCORRECTION_PANEL)
    CorrectionPanel m_correctionPanel;
#endif
#if USE(DICTATION_ALTERNATIVES)
    OwnPtr<WebCore::AlternativeTextUIController> m_alternativeTextUIController;
#endif
};

} // namespace WebKit

#endif // PageClientImpl_h
