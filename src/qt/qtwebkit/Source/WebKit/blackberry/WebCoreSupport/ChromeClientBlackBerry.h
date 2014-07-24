/*
 * Copyright (C) 2009, 2010, 2011, 2012 Research In Motion Limited. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifndef ChromeClientBlackBerry_h
#define ChromeClientBlackBerry_h

#include "ChromeClient.h"

namespace BlackBerry {
namespace WebKit {
class WebPagePrivate;
}
}

namespace WebCore {

class ChromeClientBlackBerry : public ChromeClient {
public:
    ChromeClientBlackBerry(BlackBerry::WebKit::WebPagePrivate*);

    virtual void chromeDestroyed();
    virtual void setWindowRect(const FloatRect&);
    virtual FloatRect windowRect();
    virtual FloatRect pageRect();
    virtual float scaleFactor();
    virtual void focus();
    virtual void unfocus();
    virtual bool canTakeFocus(FocusDirection);
    virtual void takeFocus(FocusDirection);
    virtual void focusedNodeChanged(Node*);
    virtual void focusedFrameChanged(Frame*);
    virtual bool shouldForceDocumentStyleSelectorUpdate();
    virtual Page* createWindow(Frame*, const FrameLoadRequest&, const WindowFeatures&, const NavigationAction&);
    virtual void show();
    virtual bool canRunModal();
    virtual void runModal();
    virtual void setToolbarsVisible(bool);
    virtual bool toolbarsVisible();
    virtual void setStatusbarVisible(bool);
    virtual bool statusbarVisible();
    virtual void setScrollbarsVisible(bool);
    virtual bool scrollbarsVisible();
    virtual void setMenubarVisible(bool);
    virtual bool menubarVisible();
    virtual void setResizable(bool);
    virtual void addMessageToConsole(MessageSource, MessageLevel, const String& message, unsigned lineNumber, unsigned columnNumber, const String& sourceID);
    virtual bool canRunBeforeUnloadConfirmPanel();
    virtual bool runBeforeUnloadConfirmPanel(const String&, Frame*);
    virtual void closeWindowSoon();
    virtual void runJavaScriptAlert(Frame*, const String&);
    virtual bool runJavaScriptConfirm(Frame*, const String&);
    virtual bool runJavaScriptPrompt(Frame*, const String&, const String&, String&);
    virtual void setStatusbarText(const String&);
    virtual bool shouldInterruptJavaScript();
    virtual KeyboardUIMode keyboardUIMode();
    virtual IntRect windowResizerRect() const;
    virtual void invalidateRootView(const IntRect&, bool);
    virtual void invalidateContentsAndRootView(const IntRect&, bool);
    virtual void invalidateContentsForSlowScroll(const IntSize&, const IntRect&, bool, const ScrollView*);
    virtual void scroll(const IntSize&, const IntRect&, const IntRect&);
    virtual void scrollableAreasDidChange();
    virtual IntPoint screenToRootView(const IntPoint&) const;
    virtual IntRect rootViewToScreen(const IntRect&) const;
    virtual void contentsSizeChanged(Frame*, const IntSize&) const;
    virtual void scrollRectIntoView(const IntRect&, const ScrollView*) const;
    virtual void mouseDidMoveOverElement(const HitTestResult&, unsigned);
    virtual void setToolTip(const String&, TextDirection);
#if ENABLE(EVENT_MODE_METATAGS)
    virtual void didReceiveCursorEventMode(Frame*, CursorEventMode) const;
    virtual void didReceiveTouchEventMode(Frame*, TouchEventMode) const;
#endif
    virtual void dispatchViewportPropertiesDidChange(const ViewportArguments&) const;
    virtual bool shouldRubberBandInDirection(ScrollDirection) const { return true; }
    virtual void numWheelEventHandlersChanged(unsigned) { }
    virtual void print(Frame*);
    virtual void exceededDatabaseQuota(Frame*, const String&, DatabaseDetails);
    virtual void runOpenPanel(Frame*, PassRefPtr<FileChooser>);
    virtual void loadIconForFiles(const Vector<String>&, FileIconLoader*);
    virtual void setCursor(const Cursor&);
    virtual void setCursorHiddenUntilMouseMoves(bool) { }
    virtual void formStateDidChange(const Node*);
    virtual void scrollbarsModeDidChange() const;
    virtual PlatformPageClient platformPageClient() const;

#if ENABLE(TOUCH_EVENTS)
    virtual void needTouchEvents(bool);
#endif

#if ENABLE(INPUT_TYPE_COLOR)
    virtual PassOwnPtr<ColorChooser> createColorChooser(ColorChooserClient*, const Color&);
    void openColorChooser(ColorChooser*, const Color&) { }
    void cleanupColorChooser(ColorChooser*) { }
    void setSelectedColorInColorChooser(ColorChooser*, const Color&) { }
#endif

    virtual void reachedMaxAppCacheSize(int64_t spaceNeeded);
    virtual void reachedApplicationCacheOriginQuota(SecurityOrigin*, int64_t) { }

    virtual void layoutUpdated(Frame*) const;
    virtual void overflowExceedsContentsSize(Frame*) const;
    virtual void didDiscoverFrameSet(Frame*) const;

    virtual int reflowWidth() const;

    virtual void chooseIconForFiles(const Vector<String>&, FileChooser*);

    virtual bool supportsFullscreenForNode(const Node*);
    virtual void enterFullscreenForNode(Node*);
    virtual void exitFullscreenForNode(Node*);
#if ENABLE(FULLSCREEN_API)
    virtual bool supportsFullScreenForElement(const Element*, bool withKeyboard);
    virtual void enterFullScreenForElement(Element*);
    virtual void exitFullScreenForElement(Element*);
    virtual void fullScreenRendererChanged(RenderBox*);
#endif

#if ENABLE(SVG)
    virtual void didSetSVGZoomAndPan(Frame*, unsigned short zoomAndPan);
#endif
    virtual bool selectItemWritingDirectionIsNatural();
    virtual bool selectItemAlignmentFollowsMenuWritingDirection();
    virtual bool hasOpenedPopup() const;
    virtual PassRefPtr<PopupMenu> createPopupMenu(PopupMenuClient*) const;
    virtual PassRefPtr<SearchPopupMenu> createSearchPopupMenu(PopupMenuClient*) const;

#if USE(ACCELERATED_COMPOSITING)
    virtual void attachRootGraphicsLayer(Frame*, GraphicsLayer*);
    virtual void setNeedsOneShotDrawingSynchronization();
    virtual void scheduleCompositingLayerFlush();
    virtual bool allowsAcceleratedCompositing() const;
#endif

#if ENABLE(REQUEST_ANIMATION_FRAME) && !USE(REQUEST_ANIMATION_FRAME_TIMER)
    virtual void scheduleAnimation();
#endif

    BlackBerry::WebKit::WebPagePrivate* webPagePrivate() const { return m_webPagePrivate; }

private:
    BlackBerry::WebKit::WebPagePrivate* m_webPagePrivate;
    RefPtr<WebCore::Element> m_fullScreenElement;
};

} // WebCore

#endif // ChromeClientBlackBerry_h
