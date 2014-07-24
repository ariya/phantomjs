/*
    Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies)

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

/*
    This file is not part of the public Qt Api. It may change without notice at any time in future.
    We make no commitment regarding source compatibility or binary compatibility.
*/

#ifndef qrawwebview_p_p_h
#define qrawwebview_p_p_h

#include "FindIndicator.h"
#include "PageClient.h"
#include "WebContextMenuProxy.h"
#include "WebEditCommandProxy.h"
#include "WebPopupMenuProxy.h"
#include "qrawwebview_p.h"

class QRawWebViewPrivate : public WebKit::PageClient {
public:

    virtual void pageClosed() { }

    virtual void preferencesDidChange() { }

    virtual void setFindIndicator(PassRefPtr<WebKit::FindIndicator>, bool fadeOut, bool animate) { }
    virtual void clearAllEditCommands() { }

    virtual void didChangeViewportProperties(const WebCore::ViewportAttributes& attr);
    virtual void handleDownloadRequest(WebKit::DownloadProxy* download);

    virtual void handleAuthenticationRequiredRequest(const String& hostname, const String& realm, const String& prefilledUsername, String& username, String& password);
    virtual void handleCertificateVerificationRequest(const String& hostname, bool& ignoreErrors);
    virtual void handleProxyAuthenticationRequiredRequest(const String& hostname, uint16_t port, const String& prefilledUsername, String& username, String& password);

    virtual void registerEditCommand(PassRefPtr<WebKit::WebEditCommandProxy>, WebKit::WebPageProxy::UndoOrRedo);
    virtual bool canUndoRedo(WebKit::WebPageProxy::UndoOrRedo undoOrRedo);
    virtual void executeUndoRedo(WebKit::WebPageProxy::UndoOrRedo undoOrRedo);

    virtual WebCore::FloatRect convertToDeviceSpace(const WebCore::FloatRect& rect);
    virtual WebCore::FloatRect convertToUserSpace(const WebCore::FloatRect& rect);
    virtual WebCore::IntPoint screenToWindow(const WebCore::IntPoint& point);
    virtual WebCore::IntRect windowToScreen(const WebCore::IntRect& rect);

#if USE(ACCELERATED_COMPOSITING)
    virtual void enterAcceleratedCompositingMode(const WebKit::LayerTreeContext&);
    virtual void exitAcceleratedCompositingMode();
    virtual void updateAcceleratedCompositingMode(const WebKit::LayerTreeContext&);
#endif // USE(ACCELERATED_COMPOSITING)

    virtual void updateTextInputState();
    virtual void handleWillSetInputMethodState();
#if ENABLE(GESTURE_EVENTS)
    virtual void doneWithGestureEvent(const WebKit::WebGestureEvent& event, bool wasEventHandled);
#endif
    virtual void displayView();
    virtual bool canScrollView() { return false; }
    virtual void scrollView(const WebCore::IntRect& scrollRect, const WebCore::IntSize& scrollOffset);

    virtual void flashBackingStoreUpdates(const Vector<WebCore::IntRect>&);
    virtual void didFindZoomableArea(const WebCore::IntPoint& target, const WebCore::IntRect& area);

    virtual void setCursorHiddenUntilMouseMoves(bool hiddenUntilMouseMoves);
    virtual void toolTipChanged(const String&, const String& newTooltip);
    virtual void pageTransitionViewportReady();

    virtual void startDrag(const WebCore::DragData& dragData, PassRefPtr<WebKit::ShareableBitmap> dragImage);

    virtual PassRefPtr<WebKit::WebPopupMenuProxy> createPopupMenuProxy(WebKit::WebPageProxy* webPageProxy);
    virtual PassRefPtr<WebKit::WebContextMenuProxy> createContextMenuProxy(WebKit::WebPageProxy* webPageProxy);

#if ENABLE(INPUT_TYPE_COLOR)
    virtual PassRefPtr<WebKit::WebColorPicker> createColorPicker(WebKit::WebPageProxy*, const WebCore::Color& intialColor, const WebCore::IntRect&);
#endif

    QRawWebViewPrivate(WebKit::WebContext*, WebKit::WebPageGroup*, QRawWebViewClient*);
    ~QRawWebViewPrivate();

    // PageClient

    virtual PassOwnPtr<WebKit::DrawingAreaProxy> createDrawingAreaProxy();

    virtual void pageDidRequestScroll(const WebCore::IntPoint& pos);
    virtual void processDidCrash();
    virtual void didRelaunchProcess();
    virtual void setViewNeedsDisplay(const WebCore::IntRect& rect);
    virtual void didChangeContentsSize(const WebCore::IntSize& newSize);
    virtual void didRenderFrame(const WebCore::IntSize& contentsSize, const WebCore::IntRect& coveredRect) { }
    virtual void setCursor(const WebCore::Cursor&);

    virtual bool isViewFocused() { return m_focused; }
    virtual bool isViewVisible() { return m_visible; }
    virtual bool isViewWindowActive() { return m_active; }
    virtual bool isViewInWindow() { return true; } // FIXME
    virtual WebCore::IntSize viewSize() { return m_size; }

    virtual void doneWithKeyEvent(const WebKit::NativeWebKeyboardEvent&, bool wasEventHandled);
#if ENABLE(TOUCH_EVENTS)
    virtual void doneWithTouchEvent(const WebKit::NativeWebTouchEvent&, bool wasEventHandled);
#endif


    bool m_focused;
    bool m_visible;
    bool m_active;
    QSize m_size;

    QRawWebViewClient* m_client;
    WTF::RefPtr<WebKit::WebPageProxy> m_webPageProxy;

private:
    QRawWebView* q;
};

#endif // qrawwebview_p_p_h
