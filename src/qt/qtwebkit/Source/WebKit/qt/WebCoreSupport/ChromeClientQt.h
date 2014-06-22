/*
 * Copyright (C) 2006 Zack Rusin <zack@kde.org>
 * Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
 *
 * All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef ChromeClientQt_h
#define ChromeClientQt_h

#include "ChromeClient.h"
#include "FloatRect.h"
#include "KURL.h"
#include "QtPlatformPlugin.h"
#include <wtf/PassOwnPtr.h>
#include <wtf/RefCounted.h>
#include <wtf/text/WTFString.h>

QT_BEGIN_NAMESPACE
class QEventLoop;
QT_END_NAMESPACE

class QWebPage;
class QWebPageAdapter;
class QWebFullScreenVideoHandler;

namespace WebCore {

class FileChooser;
class FileIconLoader;
class FloatRect;
class Page;
class RefreshAnimation;
struct FrameLoadRequest;
class QtAbstractWebPopup;
struct ViewportArguments;
#if ENABLE(VIDEO)
class FullScreenVideoQt;
#endif
class TextureMapperLayerClientQt;

class ChromeClientQt : public ChromeClient {
public:
    ChromeClientQt(QWebPageAdapter*);
    virtual ~ChromeClientQt();
    virtual void chromeDestroyed();

    virtual void setWindowRect(const FloatRect&);
    virtual FloatRect windowRect();

    virtual FloatRect pageRect();

    virtual void focus();
    virtual void unfocus();

    virtual bool canTakeFocus(FocusDirection);
    virtual void takeFocus(FocusDirection);

    virtual void focusedNodeChanged(Node*);
    virtual void focusedFrameChanged(Frame*);

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

    virtual void addMessageToConsole(MessageSource, MessageLevel, const String& message, unsigned lineNumber, unsigned columnNumber, const String& sourceID, const String& stack);

    virtual bool canRunBeforeUnloadConfirmPanel();
    virtual bool runBeforeUnloadConfirmPanel(const String& message, Frame*);

    virtual void closeWindowSoon();

    virtual void runJavaScriptAlert(Frame*, const String&);
    virtual bool runJavaScriptConfirm(Frame*, const String&);
    virtual bool runJavaScriptPrompt(Frame*, const String& message, const String& defaultValue, String& result);
    virtual bool shouldInterruptJavaScript();

    virtual void setStatusbarText(const String&);

    virtual KeyboardUIMode keyboardUIMode();
    virtual IntRect windowResizerRect() const;

    virtual void invalidateRootView(const IntRect&, bool);
    virtual void invalidateContentsAndRootView(const IntRect&, bool);
    virtual void invalidateContentsForSlowScroll(const IntRect&, bool);
    virtual void scroll(const IntSize& scrollDelta, const IntRect& rectToScroll, const IntRect& clipRect);
#if USE(TILED_BACKING_STORE)
    virtual void delegatedScrollRequested(const IntPoint& scrollPoint);
#endif

    virtual IntPoint screenToRootView(const IntPoint&) const;
    virtual IntRect rootViewToScreen(const IntRect&) const;
    virtual PlatformPageClient platformPageClient() const;
    virtual void contentsSizeChanged(Frame*, const IntSize&) const;

    virtual void scrollbarsModeDidChange() const { }
    virtual void mouseDidMoveOverElement(const HitTestResult&, unsigned modifierFlags);

    virtual void setToolTip(const String&, TextDirection);

    virtual void print(Frame*);
#if ENABLE(SQL_DATABASE)
    virtual void exceededDatabaseQuota(Frame*, const String&, DatabaseDetails);
#endif
    virtual void reachedMaxAppCacheSize(int64_t spaceNeeded);
    virtual void reachedApplicationCacheOriginQuota(SecurityOrigin*, int64_t totalSpaceNeeded);

#if USE(ACCELERATED_COMPOSITING)
    // This is a hook for WebCore to tell us what we need to do with the GraphicsLayers.
    virtual void attachRootGraphicsLayer(Frame*, GraphicsLayer*);
    virtual void setNeedsOneShotDrawingSynchronization();
    virtual void scheduleCompositingLayerFlush();
    virtual CompositingTriggerFlags allowedCompositingTriggers() const;
#endif
    virtual bool allowsAcceleratedCompositing() const;

#if USE(TILED_BACKING_STORE)
    virtual IntRect visibleRectForTiledBackingStore() const;
#endif

#if ENABLE(TOUCH_EVENTS)
    virtual void needTouchEvents(bool) { }
#endif

#if ENABLE(VIDEO) && ((USE(GSTREAMER) && USE(NATIVE_FULLSCREEN_VIDEO)) || USE(QT_MULTIMEDIA))
    virtual bool supportsFullscreenForNode(const Node*);
    virtual void enterFullscreenForNode(Node*);
    virtual void exitFullscreenForNode(Node*);
    virtual bool requiresFullscreenForVideoPlayback();
    FullScreenVideoQt* fullScreenVideo();
#endif

#if ENABLE(INPUT_TYPE_COLOR)
    virtual PassOwnPtr<ColorChooser> createColorChooser(ColorChooserClient*, const Color&);
#endif

    virtual void runOpenPanel(Frame*, PassRefPtr<FileChooser>);
    virtual void loadIconForFiles(const Vector<String>&, FileIconLoader*);

    virtual void formStateDidChange(const Node*) { }

    virtual void setCursor(const Cursor&);
    virtual void setCursorHiddenUntilMouseMoves(bool) { }

#if ENABLE(REQUEST_ANIMATION_FRAME) && !USE(REQUEST_ANIMATION_FRAME_TIMER)
    virtual void scheduleAnimation();
    virtual void serviceScriptedAnimations();
#endif

    virtual void scrollRectIntoView(const LayoutRect) const { }

    virtual bool selectItemWritingDirectionIsNatural();
    virtual bool selectItemAlignmentFollowsMenuWritingDirection();
    virtual bool hasOpenedPopup() const;
    virtual PassRefPtr<PopupMenu> createPopupMenu(PopupMenuClient*) const;
    virtual PassRefPtr<SearchPopupMenu> createSearchPopupMenu(PopupMenuClient*) const;
    virtual void populateVisitedLinks();

    PassOwnPtr<QWebSelectMethod> createSelectPopup() const;

    virtual void dispatchViewportPropertiesDidChange(const ViewportArguments&) const;

    virtual bool shouldRubberBandInDirection(WebCore::ScrollDirection) const { return true; }
    virtual void numWheelEventHandlersChanged(unsigned) { }

    QWebFullScreenVideoHandler* createFullScreenVideoHandler();

    QWebPageAdapter* m_webPage;
    KURL lastHoverURL;
    String lastHoverTitle;
    String lastHoverContent;

    bool toolBarsVisible;
    bool statusBarVisible;
    bool menuBarVisible;
    QEventLoop* m_eventLoop;
#if ENABLE(REQUEST_ANIMATION_FRAME) && !USE(REQUEST_ANIMATION_FRAME_TIMER)
    OwnPtr<RefreshAnimation> m_refreshAnimation;
#endif

#if ENABLE(VIDEO) && (USE(GSTREAMER) || USE(QT_MULTIMEDIA))
    FullScreenVideoQt* m_fullScreenVideo;
#endif

    static bool dumpVisitedLinksCallbacks;

    mutable QtPlatformPlugin m_platformPlugin;

#if USE(ACCELERATED_COMPOSITING)
    OwnPtr<TextureMapperLayerClientQt> m_textureMapperLayerClient;
#endif
};
}

#endif
