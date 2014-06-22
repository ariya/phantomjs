/*
 * Copyright (C) 2010 Patrick Gansterer <paroga@paroga.com>
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

#ifndef ChromeClientWinCE_h
#define ChromeClientWinCE_h

#include "ChromeClient.h"

class WebView;

namespace WebKit {

class ChromeClientWinCE : public WebCore::ChromeClient {
public:
    ChromeClientWinCE(WebView* webView);

    virtual void chromeDestroyed();

    virtual void setWindowRect(const WebCore::FloatRect&);
    virtual WebCore::FloatRect windowRect();

    virtual WebCore::FloatRect pageRect();

    virtual void focus();
    virtual void unfocus();

    virtual bool canTakeFocus(WebCore::FocusDirection);
    virtual void takeFocus(WebCore::FocusDirection);

    virtual void focusedNodeChanged(WebCore::Node*);
    virtual void focusedFrameChanged(WebCore::Frame*);

    // The Frame pointer provides the ChromeClient with context about which
    // Frame wants to create the new Page.  Also, the newly created window
    // should not be shown to the user until the ChromeClient of the newly
    // created Page has its show method called.
    virtual WebCore::Page* createWindow(WebCore::Frame*, const WebCore::FrameLoadRequest&, const WebCore::WindowFeatures&, const WebCore::NavigationAction&);
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

    virtual void addMessageToConsole(WebCore::MessageSource, WebCore::MessageLevel, const WTF::String& message, unsigned lineNumber, unsigned columnNumber, const WTF::String& sourceID);

    virtual bool canRunBeforeUnloadConfirmPanel();
    virtual bool runBeforeUnloadConfirmPanel(const WTF::String& message, WebCore::Frame* frame);

    virtual void closeWindowSoon();

    virtual void runJavaScriptAlert(WebCore::Frame*, const WTF::String&);
    virtual bool runJavaScriptConfirm(WebCore::Frame*, const WTF::String&);
    virtual bool runJavaScriptPrompt(WebCore::Frame*, const WTF::String& message, const WTF::String& defaultValue, WTF::String& result);
    virtual void setStatusbarText(const WTF::String&);
    virtual bool shouldInterruptJavaScript();
    virtual WebCore::KeyboardUIMode keyboardUIMode();

    virtual WebCore::IntRect windowResizerRect() const;

    // Methods used by HostWindow.
    virtual void invalidateRootView(const WebCore::IntRect&, bool);
    virtual void invalidateContentsAndRootView(const WebCore::IntRect&, bool);
    virtual void invalidateContentsForSlowScroll(const WebCore::IntRect&, bool);
    virtual void scroll(const WebCore::IntSize&, const WebCore::IntRect&, const WebCore::IntRect&);
    virtual WebCore::IntPoint screenToRootView(const WebCore::IntPoint&) const;
    virtual WebCore::IntRect rootViewToScreen(const WebCore::IntRect&) const;
    virtual PlatformPageClient platformPageClient() const;
    virtual void contentsSizeChanged(WebCore::Frame*, const WebCore::IntSize&) const;
    virtual void scrollRectIntoView(const WebCore::IntRect&) const; // Currently only Mac has a non empty implementation.
    virtual void scrollbarsModeDidChange() const;
    virtual void setCursor(const WebCore::Cursor&);
    virtual void setCursorHiddenUntilMouseMoves(bool);
    // End methods used by HostWindow.

    virtual void mouseDidMoveOverElement(const WebCore::HitTestResult&, unsigned modifierFlags);

    virtual void setToolTip(const WTF::String&, WebCore::TextDirection);

    virtual void print(WebCore::Frame*);

#if ENABLE(SQL_DATABASE)
    virtual void exceededDatabaseQuota(WebCore::Frame*, const WTF::String& databaseName, WebCore::DatabaseDetails);
#endif

    // Callback invoked when the application cache fails to save a cache object
    // because storing it would grow the database file past its defined maximum
    // size or past the amount of free space on the device.
    // The chrome client would need to take some action such as evicting some
    // old caches.
    virtual void reachedMaxAppCacheSize(int64_t spaceNeeded);

    // Callback invoked when the application cache origin quota is reached. This
    // means that the resources attempting to be cached via the manifest are
    // more than allowed on this origin. This callback allows the chrome client
    // to take action, such as prompting the user to ask to increase the quota
    // for this origin.
    virtual void reachedApplicationCacheOriginQuota(WebCore::SecurityOrigin*, int64_t totalSpaceNeeded);

#if ENABLE(NOTIFICATIONS) || ENABLE(LEGACY_NOTIFICATIONS)
    virtual WebCore::NotificationClient* notificationPresenter() const;
#endif

    virtual void runOpenPanel(WebCore::Frame*, PassRefPtr<WebCore::FileChooser>);
    // Asynchronous request to load an icon for specified filenames.
    virtual void loadIconForFiles(const Vector<WTF::String>&, WebCore::FileIconLoader*);

    // Notification that the given form element has changed. This function
    // will be called frequently, so handling should be very fast.
    virtual void formStateDidChange(const WebCore::Node*);

#if USE(ACCELERATED_COMPOSITING)
    // Pass 0 as the GraphicsLayer to detatch the root layer.
    virtual void attachRootGraphicsLayer(WebCore::Frame*, WebCore::GraphicsLayer*);
    // Sets a flag to specify that the next time content is drawn to the window,
    // the changes appear on the screen in synchrony with updates to GraphicsLayers.
    virtual void setNeedsOneShotDrawingSynchronization();
    // Sets a flag to specify that the view needs to be updated, so we need
    // to do an eager layout before the drawing.
    virtual void scheduleCompositingLayerFlush();
#endif

    virtual void setLastSetCursorToCurrentCursor();
    virtual void AXStartFrameLoad();
    virtual void AXFinishFrameLoad();

#if ENABLE(TOUCH_EVENTS)
    virtual void needTouchEvents(bool);
#endif

    virtual bool selectItemWritingDirectionIsNatural();
    virtual bool selectItemAlignmentFollowsMenuWritingDirection();
    virtual bool hasOpenedPopup() const;
    virtual PassRefPtr<WebCore::PopupMenu> createPopupMenu(WebCore::PopupMenuClient*) const;
    virtual PassRefPtr<WebCore::SearchPopupMenu> createSearchPopupMenu(WebCore::PopupMenuClient*) const;

    virtual bool shouldRubberBandInDirection(WebCore::ScrollDirection) const { return true; }
    virtual void numWheelEventHandlersChanged(unsigned) { }

private:
    WebView* m_webView;
};

} // namespace WebKit

#endif // ChromeClientWinCE_h
