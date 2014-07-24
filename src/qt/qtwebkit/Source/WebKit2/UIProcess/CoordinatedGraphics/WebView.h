/*
 * Copyright (C) 2013 Intel Corporation. All rights reserved.
 * Copyright (C) 2013 Samsung Electronics. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef WebView_h
#define WebView_h

#if USE(COORDINATED_GRAPHICS)

#include "APIObject.h"
#include "DefaultUndoController.h"
#include "PageClient.h"
#include "WebContext.h"
#include "WebGeometry.h"
#include "WebPageGroup.h"
#include "WebPageProxy.h"
#include "WebPreferences.h"
#include "WebViewClient.h"
#include <WebCore/TransformationMatrix.h>

namespace WebCore {
class CoordinatedGraphicsScene;
}

namespace WebKit {

class WebView : public TypedAPIObject<APIObject::TypeView>, public PageClient {
public:
    virtual ~WebView();

    static PassRefPtr<WebView> create(WebContext*, WebPageGroup*);

    void initialize();

    void setSize(const WebCore::IntSize&);
    const WebCore::IntSize& size() const { return m_size; }

    bool isFocused() const { return m_focused; }
    void setFocused(bool);

    bool isVisible() const { return m_visible; }
    void setVisible(bool);

    void setContentScaleFactor(float scaleFactor) { m_contentScaleFactor = scaleFactor; }
    float contentScaleFactor() const { return m_contentScaleFactor; }

    void setContentPosition(const WebCore::FloatPoint& position) { m_contentPosition = position; }
    const WebCore::FloatPoint& contentPosition() const { return m_contentPosition; }

    void setUserViewportTranslation(double tx, double ty);
    WebCore::IntPoint userViewportToContents(const WebCore::IntPoint&) const;
    WebCore::IntPoint userViewportToScene(const WebCore::IntPoint&) const;
    WebCore::IntPoint contentsToUserViewport(const WebCore::IntPoint&) const;

    void paintToCurrentGLContext();

    WKPageRef pageRef() const { return toAPI(m_page.get()); }

    void setDrawsBackground(bool);
    bool drawsBackground() const;
    void setDrawsTransparentBackground(bool);
    bool drawsTransparentBackground() const;

    void suspendActiveDOMObjectsAndAnimations();
    void resumeActiveDOMObjectsAndAnimations();

    void setShowsAsSource(bool);
    bool showsAsSource() const;

#if ENABLE(FULLSCREEN_API)
    bool exitFullScreen();
#endif

    void findZoomableAreaForPoint(const WebCore::IntPoint&, const WebCore::IntSize&);

    // View client.
    void initializeClient(const WKViewClient*);

    WebPageProxy* page() { return m_page.get(); }

    void didChangeContentsSize(const WebCore::IntSize&);
    const WebCore::IntSize& contentsSize() const { return m_contentsSize; }
    WebCore::FloatSize visibleContentsSize() const;
    void didFindZoomableArea(const WebCore::IntPoint&, const WebCore::IntRect&);

    // FIXME: Should become private when Web Events creation is moved to WebView.
    WebCore::AffineTransform transformFromScene() const;
    WebCore::AffineTransform transformToScene() const;

    void setOpacity(double opacity) { m_opacity = clampTo(opacity, 0.0, 1.0); }
    double opacity() const { return m_opacity; }

protected:
    WebView(WebContext*, WebPageGroup*);
    WebCore::CoordinatedGraphicsScene* coordinatedGraphicsScene();

    void updateViewportSize();
    WebCore::FloatSize dipSize() const;
    // PageClient
    virtual PassOwnPtr<DrawingAreaProxy> createDrawingAreaProxy() OVERRIDE;

    virtual void setViewNeedsDisplay(const WebCore::IntRect&) OVERRIDE;

    virtual void displayView() OVERRIDE;

    virtual bool canScrollView() OVERRIDE { return false; }
    virtual void scrollView(const WebCore::IntRect&, const WebCore::IntSize&) OVERRIDE;

    virtual WebCore::IntSize viewSize() OVERRIDE;

    virtual bool isViewWindowActive() OVERRIDE;
    virtual bool isViewFocused() OVERRIDE;
    virtual bool isViewVisible() OVERRIDE;
    virtual bool isViewInWindow() OVERRIDE;

    virtual void processDidCrash() OVERRIDE;
    virtual void didRelaunchProcess() OVERRIDE;
    virtual void pageClosed() OVERRIDE;

    virtual void preferencesDidChange() OVERRIDE;

    virtual void toolTipChanged(const String&, const String&) OVERRIDE;

    virtual void pageDidRequestScroll(const WebCore::IntPoint&) OVERRIDE;
    virtual void didRenderFrame(const WebCore::IntSize& contentsSize, const WebCore::IntRect& coveredRect) OVERRIDE;
    virtual void pageTransitionViewportReady() OVERRIDE;

    virtual void setCursor(const WebCore::Cursor&) OVERRIDE;
    virtual void setCursorHiddenUntilMouseMoves(bool) OVERRIDE;

    virtual void didChangeViewportProperties(const WebCore::ViewportAttributes&) OVERRIDE;

    virtual void registerEditCommand(PassRefPtr<WebEditCommandProxy>, WebPageProxy::UndoOrRedo) OVERRIDE;
    virtual void clearAllEditCommands() OVERRIDE;
    virtual bool canUndoRedo(WebPageProxy::UndoOrRedo) OVERRIDE;
    virtual void executeUndoRedo(WebPageProxy::UndoOrRedo) OVERRIDE;

    virtual WebCore::FloatRect convertToDeviceSpace(const WebCore::FloatRect&) OVERRIDE;
    virtual WebCore::FloatRect convertToUserSpace(const WebCore::FloatRect&) OVERRIDE;
    virtual WebCore::IntPoint screenToWindow(const WebCore::IntPoint&) OVERRIDE;
    virtual WebCore::IntRect windowToScreen(const WebCore::IntRect&) OVERRIDE;

    virtual void updateTextInputState() OVERRIDE;

    virtual void handleDownloadRequest(DownloadProxy*) OVERRIDE;

    virtual void doneWithKeyEvent(const NativeWebKeyboardEvent&, bool) OVERRIDE;
#if ENABLE(TOUCH_EVENTS)
    virtual void doneWithTouchEvent(const NativeWebTouchEvent&, bool wasEventHandled) OVERRIDE;
#endif

    virtual PassRefPtr<WebPopupMenuProxy> createPopupMenuProxy(WebPageProxy*) OVERRIDE;
    virtual PassRefPtr<WebContextMenuProxy> createContextMenuProxy(WebPageProxy*) OVERRIDE;
#if ENABLE(INPUT_TYPE_COLOR)
    virtual PassRefPtr<WebColorPicker> createColorPicker(WebPageProxy*, const WebCore::Color& initialColor, const WebCore::IntRect&) OVERRIDE;
#endif

    virtual void setFindIndicator(PassRefPtr<FindIndicator>, bool, bool) OVERRIDE;

    virtual void enterAcceleratedCompositingMode(const LayerTreeContext&) OVERRIDE;
    virtual void exitAcceleratedCompositingMode() OVERRIDE;
    virtual void updateAcceleratedCompositingMode(const LayerTreeContext&) OVERRIDE;

    virtual void flashBackingStoreUpdates(const Vector<WebCore::IntRect>&) OVERRIDE;

protected:
    WebViewClient m_client;
    RefPtr<WebPageProxy> m_page;
    DefaultUndoController m_undoController;
    WebCore::TransformationMatrix m_userViewportTransform;
    WebCore::IntSize m_size; // Size in device units.
    bool m_focused;
    bool m_visible;
    float m_contentScaleFactor;
    double m_opacity;
    WebCore::FloatPoint m_contentPosition; // Position in UI units.
    WebCore::IntSize m_contentsSize;
};

} // namespace WebKit

#endif // USE(COORDINATED_GRAPHICS)

#endif // WebView_h
