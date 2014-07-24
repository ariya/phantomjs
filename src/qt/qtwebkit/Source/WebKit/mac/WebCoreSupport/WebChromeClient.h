/*
 * Copyright (C) 2006, 2007, 2008, 2009 Apple Inc. All rights reserved.
 * Copyright (C) 2008, 2010 Nokia Corporation and/or its subsidiary(-ies)
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

#import <WebCore/ChromeClient.h>
#import <WebCore/FocusDirection.h>
#import <wtf/Forward.h>

@class WebView;

class WebChromeClient : public WebCore::ChromeClient {
public:
    WebChromeClient(WebView*);
    
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
    
    virtual void addMessageToConsole(WebCore::MessageSource, WebCore::MessageLevel, const WTF::String& message, unsigned lineNumber, unsigned columnNumber, const WTF::String& sourceURL) OVERRIDE;

    virtual bool canRunBeforeUnloadConfirmPanel() OVERRIDE;
    virtual bool runBeforeUnloadConfirmPanel(const WTF::String& message, WebCore::Frame*) OVERRIDE;

    virtual void closeWindowSoon() OVERRIDE;

    virtual void runJavaScriptAlert(WebCore::Frame*, const WTF::String&) OVERRIDE;
    virtual bool runJavaScriptConfirm(WebCore::Frame*, const WTF::String&) OVERRIDE;
    virtual bool runJavaScriptPrompt(WebCore::Frame*, const WTF::String& message, const WTF::String& defaultValue, WTF::String& result) OVERRIDE;
    virtual bool shouldInterruptJavaScript() OVERRIDE;

    virtual WebCore::IntRect windowResizerRect() const OVERRIDE;

    virtual bool supportsImmediateInvalidation() OVERRIDE;
    virtual void invalidateRootView(const WebCore::IntRect&, bool) OVERRIDE;
    virtual void invalidateContentsAndRootView(const WebCore::IntRect&, bool) OVERRIDE;
    virtual void invalidateContentsForSlowScroll(const WebCore::IntRect&, bool) OVERRIDE;
    virtual void scroll(const WebCore::IntSize& scrollDelta, const WebCore::IntRect& rectToScroll, const WebCore::IntRect& clipRect) OVERRIDE;

    virtual WebCore::IntPoint screenToRootView(const WebCore::IntPoint&) const OVERRIDE;
    virtual WebCore::IntRect rootViewToScreen(const WebCore::IntRect&) const OVERRIDE;
    virtual PlatformPageClient platformPageClient() const OVERRIDE;
    virtual void contentsSizeChanged(WebCore::Frame*, const WebCore::IntSize&) const OVERRIDE;
    virtual void scrollRectIntoView(const WebCore::IntRect&) const OVERRIDE;
    
    virtual void setStatusbarText(const WTF::String&) OVERRIDE;

    virtual void scrollbarsModeDidChange() const OVERRIDE { }
    virtual bool shouldUnavailablePluginMessageBeButton(WebCore::RenderEmbeddedObject::PluginUnavailabilityReason) const OVERRIDE;
    virtual void unavailablePluginButtonClicked(WebCore::Element*, WebCore::RenderEmbeddedObject::PluginUnavailabilityReason) const OVERRIDE;
    virtual void mouseDidMoveOverElement(const WebCore::HitTestResult&, unsigned modifierFlags) OVERRIDE;

    virtual void setToolTip(const WTF::String&, WebCore::TextDirection) OVERRIDE;

    virtual void print(WebCore::Frame*) OVERRIDE;
#if ENABLE(SQL_DATABASE)
    virtual void exceededDatabaseQuota(WebCore::Frame*, const WTF::String& databaseName, WebCore::DatabaseDetails) OVERRIDE;
#endif
    virtual void reachedMaxAppCacheSize(int64_t spaceNeeded) OVERRIDE;
    virtual void reachedApplicationCacheOriginQuota(WebCore::SecurityOrigin*, int64_t totalSpaceNeeded) OVERRIDE;
    virtual void populateVisitedLinks() OVERRIDE;

#if ENABLE(DASHBOARD_SUPPORT)
    virtual void annotatedRegionsChanged() OVERRIDE;
#endif

    virtual void runOpenPanel(WebCore::Frame*, PassRefPtr<WebCore::FileChooser>) OVERRIDE;
    virtual void loadIconForFiles(const Vector<WTF::String>&, WebCore::FileIconLoader*) OVERRIDE;

    virtual void setCursor(const WebCore::Cursor&) OVERRIDE;
    virtual void setCursorHiddenUntilMouseMoves(bool) OVERRIDE;

    virtual WebCore::FloatRect customHighlightRect(WebCore::Node*, const WTF::AtomicString& type, const WebCore::FloatRect& lineRect) OVERRIDE;
    virtual void paintCustomHighlight(WebCore::Node*, const WTF::AtomicString& type, const WebCore::FloatRect& boxRect, const WebCore::FloatRect& lineRect, bool behindText, bool entireLine) OVERRIDE;

    virtual WebCore::KeyboardUIMode keyboardUIMode() OVERRIDE;

    virtual NSResponder *firstResponder() OVERRIDE;
    virtual void makeFirstResponder(NSResponder *) OVERRIDE;

    virtual void enableSuddenTermination() OVERRIDE;
    virtual void disableSuddenTermination() OVERRIDE;
    
    virtual bool shouldReplaceWithGeneratedFileForUpload(const WTF::String& path, WTF::String &generatedFilename) OVERRIDE;
    virtual WTF::String generateReplacementFile(const WTF::String& path) OVERRIDE;

    virtual void formStateDidChange(const WebCore::Node*) OVERRIDE { }

    virtual void elementDidFocus(const WebCore::Node*) OVERRIDE;
    virtual void elementDidBlur(const WebCore::Node*) OVERRIDE;

    virtual bool shouldPaintEntireContents() const OVERRIDE;

#if USE(ACCELERATED_COMPOSITING)
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
#endif

#if ENABLE(VIDEO)
    virtual bool supportsFullscreenForNode(const WebCore::Node*) OVERRIDE;
    virtual void enterFullscreenForNode(WebCore::Node*) OVERRIDE;
    virtual void exitFullscreenForNode(WebCore::Node*) OVERRIDE;
#endif
    
#if ENABLE(FULLSCREEN_API)
    virtual bool supportsFullScreenForElement(const WebCore::Element*, bool withKeyboard) OVERRIDE;
    virtual void enterFullScreenForElement(WebCore::Element*) OVERRIDE;
    virtual void exitFullScreenForElement(WebCore::Element*) OVERRIDE;
    virtual void fullScreenRendererChanged(WebCore::RenderBox*) OVERRIDE;
#endif

    virtual bool selectItemWritingDirectionIsNatural() OVERRIDE;
    virtual bool selectItemAlignmentFollowsMenuWritingDirection() OVERRIDE;
    virtual bool hasOpenedPopup() const OVERRIDE;
    virtual PassRefPtr<WebCore::PopupMenu> createPopupMenu(WebCore::PopupMenuClient*) const OVERRIDE;
    virtual PassRefPtr<WebCore::SearchPopupMenu> createSearchPopupMenu(WebCore::PopupMenuClient*) const OVERRIDE;

    virtual void numWheelEventHandlersChanged(unsigned) OVERRIDE { }
    virtual bool shouldRubberBandInDirection(WebCore::ScrollDirection) const OVERRIDE { return false; }

    WebView* webView() { return m_webView; }

private:
    WebView *m_webView;
};
