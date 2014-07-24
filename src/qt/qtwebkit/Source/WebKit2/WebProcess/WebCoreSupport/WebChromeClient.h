/*
 * Copyright (C) 2010, 2011, 2012 Apple Inc. All rights reserved.
 * Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef WebChromeClient_h
#define WebChromeClient_h

#include <WebCore/ChromeClient.h>
#include <WebCore/ViewportArguments.h>
#include <wtf/text/WTFString.h>

namespace WebKit {

class WebFrame;
class WebPage;

class WebChromeClient : public WebCore::ChromeClient {
public:
    WebChromeClient(WebPage* page)
        : m_cachedMainFrameHasHorizontalScrollbar(false)
        , m_cachedMainFrameHasVerticalScrollbar(false)
        , m_page(page)
    {
    }
    
    WebPage* page() const { return m_page; }

    virtual void* webView() const { return 0; }

private:
    virtual void chromeDestroyed() OVERRIDE;
    
    virtual void setWindowRect(const WebCore::FloatRect&) OVERRIDE;
    virtual WebCore::FloatRect windowRect() OVERRIDE;
    
    virtual WebCore::FloatRect pageRect() OVERRIDE;
    
    virtual void focus() OVERRIDE;
    virtual void unfocus() OVERRIDE;
    
    virtual bool canTakeFocus(WebCore::FocusDirection) OVERRIDE;
    virtual void takeFocus(WebCore::FocusDirection) OVERRIDE;

    virtual void focusedNodeChanged(WebCore::Node*) OVERRIDE;
    virtual void focusedFrameChanged(WebCore::Frame*) OVERRIDE;

    // The Frame pointer provides the ChromeClient with context about which
    // Frame wants to create the new Page.  Also, the newly created window
    // should not be shown to the user until the ChromeClient of the newly
    // created Page has its show method called.
    virtual WebCore::Page* createWindow(WebCore::Frame*, const WebCore::FrameLoadRequest&, const WebCore::WindowFeatures&, const WebCore::NavigationAction&) OVERRIDE;
    virtual void show() OVERRIDE;
    
    virtual bool canRunModal() OVERRIDE;
    virtual void runModal() OVERRIDE;
    
    virtual void setToolbarsVisible(bool) OVERRIDE;
    virtual bool toolbarsVisible() OVERRIDE;
    
    virtual void setStatusbarVisible(bool) OVERRIDE;
    virtual bool statusbarVisible() OVERRIDE;
    
    virtual void setScrollbarsVisible(bool) OVERRIDE;
    virtual bool scrollbarsVisible() OVERRIDE;
    
    virtual void setMenubarVisible(bool) OVERRIDE;
    virtual bool menubarVisible() OVERRIDE;
    
    virtual void setResizable(bool) OVERRIDE;
    
    virtual void addMessageToConsole(WebCore::MessageSource, WebCore::MessageLevel, const String& message, unsigned lineNumber, unsigned columnNumber, const String& sourceID) OVERRIDE;
    
    virtual bool canRunBeforeUnloadConfirmPanel() OVERRIDE;
    virtual bool runBeforeUnloadConfirmPanel(const String& message, WebCore::Frame*) OVERRIDE;
    
    virtual void closeWindowSoon() OVERRIDE;
    
    virtual void runJavaScriptAlert(WebCore::Frame*, const String&) OVERRIDE;
    virtual bool runJavaScriptConfirm(WebCore::Frame*, const String&) OVERRIDE;
    virtual bool runJavaScriptPrompt(WebCore::Frame*, const String& message, const String& defaultValue, String& result) OVERRIDE;
    virtual void setStatusbarText(const String&) OVERRIDE;
    virtual bool shouldInterruptJavaScript() OVERRIDE;

    virtual WebCore::KeyboardUIMode keyboardUIMode() OVERRIDE;

    virtual WebCore::IntRect windowResizerRect() const OVERRIDE;
    
    // HostWindow member function overrides.
    virtual void invalidateRootView(const WebCore::IntRect&, bool) OVERRIDE;
    virtual void invalidateContentsAndRootView(const WebCore::IntRect&, bool) OVERRIDE;
    virtual void invalidateContentsForSlowScroll(const WebCore::IntRect&, bool) OVERRIDE;
    virtual void scroll(const WebCore::IntSize& scrollDelta, const WebCore::IntRect& scrollRect, const WebCore::IntRect& clipRect) OVERRIDE;
#if USE(TILED_BACKING_STORE)
    virtual void delegatedScrollRequested(const WebCore::IntPoint& scrollOffset) OVERRIDE;
#endif
    virtual WebCore::IntPoint screenToRootView(const WebCore::IntPoint&) const OVERRIDE;
    virtual WebCore::IntRect rootViewToScreen(const WebCore::IntRect&) const OVERRIDE;
    virtual PlatformPageClient platformPageClient() const OVERRIDE;
    virtual void contentsSizeChanged(WebCore::Frame*, const WebCore::IntSize&) const OVERRIDE;
    virtual void scrollRectIntoView(const WebCore::IntRect&) const OVERRIDE; // Currently only Mac has a non empty implementation.

    virtual bool shouldUnavailablePluginMessageBeButton(WebCore::RenderEmbeddedObject::PluginUnavailabilityReason) const OVERRIDE;
    virtual void unavailablePluginButtonClicked(WebCore::Element*, WebCore::RenderEmbeddedObject::PluginUnavailabilityReason) const OVERRIDE;

    virtual void scrollbarsModeDidChange() const OVERRIDE;
    virtual void mouseDidMoveOverElement(const WebCore::HitTestResult&, unsigned modifierFlags) OVERRIDE;
    
    virtual void setToolTip(const String&, WebCore::TextDirection) OVERRIDE;
    
    virtual void print(WebCore::Frame*) OVERRIDE;
    
#if ENABLE(SQL_DATABASE)
    virtual void exceededDatabaseQuota(WebCore::Frame*, const String& databaseName, WebCore::DatabaseDetails) OVERRIDE;
#endif

    virtual void reachedMaxAppCacheSize(int64_t spaceNeeded) OVERRIDE;
    virtual void reachedApplicationCacheOriginQuota(WebCore::SecurityOrigin*, int64_t spaceNeeded) OVERRIDE;

#if ENABLE(DASHBOARD_SUPPORT)
    virtual void annotatedRegionsChanged() OVERRIDE;
#endif

    virtual void populateVisitedLinks() OVERRIDE;
    
    virtual WebCore::FloatRect customHighlightRect(WebCore::Node*, const WTF::AtomicString& type, const WebCore::FloatRect& lineRect) OVERRIDE;
    virtual void paintCustomHighlight(WebCore::Node*, const AtomicString& type, const WebCore::FloatRect& boxRect, const WebCore::FloatRect& lineRect,
                                      bool behindText, bool entireLine) OVERRIDE;
    
    virtual bool shouldReplaceWithGeneratedFileForUpload(const String& path, String& generatedFilename) OVERRIDE;
    virtual String generateReplacementFile(const String& path) OVERRIDE;
    
    virtual bool paintCustomOverhangArea(WebCore::GraphicsContext*, const WebCore::IntRect&, const WebCore::IntRect&, const WebCore::IntRect&) OVERRIDE;

#if ENABLE(INPUT_TYPE_COLOR)
    virtual PassOwnPtr<WebCore::ColorChooser> createColorChooser(WebCore::ColorChooserClient*, const WebCore::Color&) OVERRIDE;
#endif

    virtual void runOpenPanel(WebCore::Frame*, PassRefPtr<WebCore::FileChooser>) OVERRIDE;
    virtual void loadIconForFiles(const Vector<String>&, WebCore::FileIconLoader*) OVERRIDE;

    virtual void setCursor(const WebCore::Cursor&) OVERRIDE;
    virtual void setCursorHiddenUntilMouseMoves(bool) OVERRIDE;
#if ENABLE(REQUEST_ANIMATION_FRAME) && !USE(REQUEST_ANIMATION_FRAME_TIMER)
    virtual void scheduleAnimation() OVERRIDE;
#endif

    // Notification that the given form element has changed. This function
    // will be called frequently, so handling should be very fast.
    virtual void formStateDidChange(const WebCore::Node*) OVERRIDE;

    virtual void didAssociateFormControls(const Vector<RefPtr<WebCore::Element> >&) OVERRIDE;
    virtual bool shouldNotifyOnFormChanges() OVERRIDE;

    virtual bool selectItemWritingDirectionIsNatural() OVERRIDE;
    virtual bool selectItemAlignmentFollowsMenuWritingDirection() OVERRIDE;
    virtual bool hasOpenedPopup() const OVERRIDE;
    virtual PassRefPtr<WebCore::PopupMenu> createPopupMenu(WebCore::PopupMenuClient*) const OVERRIDE;
    virtual PassRefPtr<WebCore::SearchPopupMenu> createSearchPopupMenu(WebCore::PopupMenuClient*) const OVERRIDE;

#if USE(ACCELERATED_COMPOSITING)
    virtual WebCore::GraphicsLayerFactory* graphicsLayerFactory() const OVERRIDE;
    virtual void attachRootGraphicsLayer(WebCore::Frame*, WebCore::GraphicsLayer*) OVERRIDE;
    virtual void setNeedsOneShotDrawingSynchronization() OVERRIDE;
    virtual void scheduleCompositingLayerFlush() OVERRIDE;

    virtual CompositingTriggerFlags allowedCompositingTriggers() const
    {
        return static_cast<CompositingTriggerFlags>(
            ThreeDTransformTrigger |
            VideoTrigger |
            PluginTrigger| 
            CanvasTrigger |
            AnimationTrigger);
    }

    virtual bool layerTreeStateIsFrozen() const OVERRIDE;
#endif

#if ENABLE(TOUCH_EVENTS)
    virtual void needTouchEvents(bool) OVERRIDE;
#endif

#if ENABLE(FULLSCREEN_API)
    virtual bool supportsFullScreenForElement(const WebCore::Element*, bool withKeyboard) OVERRIDE;
    virtual void enterFullScreenForElement(WebCore::Element*) OVERRIDE;
    virtual void exitFullScreenForElement(WebCore::Element*) OVERRIDE;
#endif

#if PLATFORM(MAC)
    virtual void makeFirstResponder() OVERRIDE;
#endif

    virtual void enableSuddenTermination() OVERRIDE;
    virtual void disableSuddenTermination() OVERRIDE;
    
    virtual void dispatchViewportPropertiesDidChange(const WebCore::ViewportArguments&) const OVERRIDE;

    virtual void notifyScrollerThumbIsVisibleInRect(const WebCore::IntRect&) OVERRIDE;
    virtual void recommendedScrollbarStyleDidChange(int32_t newStyle) OVERRIDE;
    virtual bool shouldRubberBandInDirection(WebCore::ScrollDirection) const OVERRIDE;

    virtual WebCore::Color underlayColor() const OVERRIDE;
    
    virtual void numWheelEventHandlersChanged(unsigned) OVERRIDE;

    virtual void logDiagnosticMessage(const String& message, const String& description, const String& success) OVERRIDE;

    virtual String plugInStartLabelTitle(const String& mimeType) const OVERRIDE;
    virtual String plugInStartLabelSubtitle(const String& mimeType) const OVERRIDE;
    virtual String plugInExtraStyleSheet() const OVERRIDE;
    virtual String plugInExtraScript() const OVERRIDE;

    virtual void didAddHeaderLayer(WebCore::GraphicsLayer*) OVERRIDE;
    virtual void didAddFooterLayer(WebCore::GraphicsLayer*) OVERRIDE;

    virtual void incrementActivePageCount() OVERRIDE;
    virtual void decrementActivePageCount() OVERRIDE;

    String m_cachedToolTip;
    mutable RefPtr<WebFrame> m_cachedFrameSetLargestFrame;
    mutable bool m_cachedMainFrameHasHorizontalScrollbar;
    mutable bool m_cachedMainFrameHasVerticalScrollbar;

    WebPage* m_page;
};

} // namespace WebKit

#endif // WebChromeClient_h
