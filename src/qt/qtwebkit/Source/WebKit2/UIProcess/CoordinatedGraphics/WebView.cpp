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

#include "config.h"
#if USE(COORDINATED_GRAPHICS)

#include "WebView.h"

#include "CoordinatedLayerTreeHostProxy.h"
#include "DrawingAreaProxyImpl.h"
#include "NotImplemented.h"
#include "WebContextMenuProxy.h"
#include "WebPageProxy.h"
#include <WebCore/CoordinatedGraphicsScene.h>

#if ENABLE(FULLSCREEN_API)
#include "WebFullScreenManagerProxy.h"
#endif

using namespace WebCore;

namespace WebKit {

WebView::WebView(WebContext* context, WebPageGroup* pageGroup)
    : m_focused(false)
    , m_visible(false)
    , m_contentScaleFactor(1.0)
    , m_opacity(1.0)
{
    // Need to call createWebPage after other data members, specifically m_visible, are initialized.
    m_page = context->createWebPage(this, pageGroup);

    m_page->pageGroup()->preferences()->setAcceleratedCompositingEnabled(true);
    m_page->pageGroup()->preferences()->setForceCompositingMode(true);

    char* debugVisualsEnvironment = getenv("WEBKIT_SHOW_COMPOSITING_DEBUG_VISUALS");
    bool showDebugVisuals = debugVisualsEnvironment && !strcmp(debugVisualsEnvironment, "1");
    m_page->pageGroup()->preferences()->setCompositingBordersVisible(showDebugVisuals);
    m_page->pageGroup()->preferences()->setCompositingRepaintCountersVisible(showDebugVisuals);
}

WebView::~WebView()
{
    if (m_page->isClosed())
        return;

    m_page->close();
}

void WebView::initialize()
{
    m_page->initializeWebPage();
    if (CoordinatedGraphicsScene* scene = coordinatedGraphicsScene())
        scene->setActive(true);
}

void WebView::setSize(const WebCore::IntSize& size)
{
    if (m_size == size)
        return;

    m_size = size;

    updateViewportSize();
}

void WebView::setFocused(bool focused)
{
    if (m_focused == focused)
        return;

    m_focused = focused;
    m_page->viewStateDidChange(WebPageProxy::ViewIsFocused | WebPageProxy::ViewWindowIsActive);
}

void WebView::setVisible(bool visible)
{
    if (m_visible == visible)
        return;

    m_visible = visible;
    m_page->viewStateDidChange(WebPageProxy::ViewIsVisible);
}

void WebView::setUserViewportTranslation(double tx, double ty)
{
    m_userViewportTransform = TransformationMatrix().translate(tx, ty);
}

IntPoint WebView::userViewportToContents(const IntPoint& point) const
{
    return transformFromScene().mapPoint(point);
}

IntPoint WebView::userViewportToScene(const WebCore::IntPoint& point) const
{
    return m_userViewportTransform.mapPoint(point);
}

IntPoint WebView::contentsToUserViewport(const IntPoint& point) const
{
    return transformToScene().mapPoint(point);
}

void WebView::paintToCurrentGLContext()
{
    CoordinatedGraphicsScene* scene = coordinatedGraphicsScene();
    if (!scene)
        return;

    // FIXME: We need to clean up this code as it is split over CoordGfx and Page.
    scene->setDrawsBackground(m_page->drawsBackground());
    const FloatRect& viewport = m_userViewportTransform.mapRect(IntRect(IntPoint(), m_size));

    scene->paintToCurrentGLContext(transformToScene().toTransformationMatrix(), m_opacity, viewport);
}

void WebView::setDrawsBackground(bool drawsBackground)
{
    m_page->setDrawsBackground(drawsBackground);
}

bool WebView::drawsBackground() const
{
    return m_page->drawsBackground();
}

void WebView::setDrawsTransparentBackground(bool transparentBackground)
{
    m_page->setDrawsTransparentBackground(transparentBackground);
}

bool WebView::drawsTransparentBackground() const
{
    return m_page->drawsTransparentBackground();
}

void WebView::suspendActiveDOMObjectsAndAnimations()
{
    m_page->suspendActiveDOMObjectsAndAnimations();
}

void WebView::resumeActiveDOMObjectsAndAnimations()
{
    m_page->resumeActiveDOMObjectsAndAnimations();
}

void WebView::setShowsAsSource(bool showsAsSource)
{
    m_page->setMainFrameInViewSourceMode(showsAsSource);
}

bool WebView::showsAsSource() const
{
    return m_page->mainFrameInViewSourceMode();
}

#if ENABLE(FULLSCREEN_API)
bool WebView::exitFullScreen()
{
#if PLATFORM(EFL)
    // FIXME: Implement this for other platforms.
    if (!m_page->fullScreenManager()->isFullScreen())
        return false;
#endif
    m_page->fullScreenManager()->requestExitFullScreen();
    return true;
}
#endif

void WebView::initializeClient(const WKViewClient* client)
{
    m_client.initialize(client);
}

void WebView::didChangeContentsSize(const WebCore::IntSize& size)
{
    if (m_contentsSize == size)
        return;

    m_contentsSize = size;
    m_client.didChangeContentsSize(this, size);

    updateViewportSize();
}

void WebView::didFindZoomableArea(const WebCore::IntPoint& target, const WebCore::IntRect& area)
{
    m_client.didFindZoomableArea(this, target, area);
}

AffineTransform WebView::transformFromScene() const
{
    return transformToScene().inverse();
}

AffineTransform WebView::transformToScene() const
{
    FloatPoint position = -m_contentPosition;
    float effectiveScale = m_contentScaleFactor * m_page->deviceScaleFactor();
    position.scale(effectiveScale, effectiveScale);

    TransformationMatrix transform = m_userViewportTransform;
    transform.translate(position.x(), position.y());
    transform.scale(effectiveScale);

    return transform.toAffineTransform();
}

CoordinatedGraphicsScene* WebView::coordinatedGraphicsScene()
{
    DrawingAreaProxy* drawingArea = m_page->drawingArea();
    if (!drawingArea)
        return 0;

    WebKit::CoordinatedLayerTreeHostProxy* layerTreeHostProxy = drawingArea->coordinatedLayerTreeHostProxy();
    if (!layerTreeHostProxy)
        return 0;

    return layerTreeHostProxy->coordinatedGraphicsScene();
}

void WebView::updateViewportSize()
{
    if (DrawingAreaProxy* drawingArea = page()->drawingArea()) {
        // Web Process expects sizes in UI units, and not raw device units.
        drawingArea->setSize(roundedIntSize(dipSize()), IntSize(), IntSize());
        FloatRect visibleContentsRect(contentPosition(), visibleContentsSize());
        visibleContentsRect.intersect(FloatRect(FloatPoint(), contentsSize()));
        drawingArea->setVisibleContentsRect(visibleContentsRect, FloatPoint());
    }
}

inline WebCore::FloatSize WebView::dipSize() const
{
    FloatSize dipSize(size());
    dipSize.scale(1 / m_page->deviceScaleFactor());

    return dipSize;
}

WebCore::FloatSize WebView::visibleContentsSize() const
{
    FloatSize visibleContentsSize(dipSize());
    visibleContentsSize.scale(1 / m_contentScaleFactor);

    return visibleContentsSize;
}

// Page Client

PassOwnPtr<DrawingAreaProxy> WebView::createDrawingAreaProxy()
{
    OwnPtr<DrawingAreaProxy> drawingArea = DrawingAreaProxyImpl::create(page());
    return drawingArea.release();
}

void WebView::setViewNeedsDisplay(const WebCore::IntRect& area)
{
    m_client.viewNeedsDisplay(this, area);
}

void WebView::displayView()
{
    notImplemented();
}

void WebView::scrollView(const WebCore::IntRect& scrollRect, const WebCore::IntSize&)
{
    setViewNeedsDisplay(scrollRect);
}

WebCore::IntSize WebView::viewSize()
{
    return roundedIntSize(dipSize());
}

bool WebView::isViewWindowActive()
{
    notImplemented();
    return true;
}

bool WebView::isViewFocused()
{
    return isFocused();
}

bool WebView::isViewVisible()
{
    return isVisible();
}

bool WebView::isViewInWindow()
{
    notImplemented();
    return true;
}

void WebView::processDidCrash()
{
    m_client.webProcessCrashed(this, m_page->urlAtProcessExit());
}

void WebView::didRelaunchProcess()
{
    m_client.webProcessDidRelaunch(this);
}

void WebView::pageClosed()
{
    notImplemented();
}

void WebView::preferencesDidChange()
{
    notImplemented();
}

void WebView::toolTipChanged(const String&, const String& newToolTip)
{
    m_client.didChangeTooltip(this, newToolTip);
}

void WebView::setCursor(const WebCore::Cursor&)
{
    notImplemented();
}

void WebView::setCursorHiddenUntilMouseMoves(bool)
{
    notImplemented();
}

void WebView::registerEditCommand(PassRefPtr<WebEditCommandProxy> command, WebPageProxy::UndoOrRedo undoOrRedo)
{
    m_undoController.registerEditCommand(command, undoOrRedo);
}

void WebView::clearAllEditCommands()
{
    m_undoController.clearAllEditCommands();
}

bool WebView::canUndoRedo(WebPageProxy::UndoOrRedo undoOrRedo)
{
    return m_undoController.canUndoRedo(undoOrRedo);
}

void WebView::executeUndoRedo(WebPageProxy::UndoOrRedo undoOrRedo)
{
    m_undoController.executeUndoRedo(undoOrRedo);
}

IntPoint WebView::screenToWindow(const IntPoint& point)
{
    notImplemented();
    return point;
}

IntRect WebView::windowToScreen(const IntRect&)
{
    notImplemented();
    return IntRect();
}

void WebView::doneWithKeyEvent(const NativeWebKeyboardEvent&, bool)
{
    notImplemented();
}

#if ENABLE(TOUCH_EVENTS)
void WebView::doneWithTouchEvent(const NativeWebTouchEvent& event, bool wasEventHandled)
{
    m_client.doneWithTouchEvent(this, event, wasEventHandled);
}
#endif

PassRefPtr<WebPopupMenuProxy> WebView::createPopupMenuProxy(WebPageProxy*)
{
    notImplemented();
    return 0;
}

PassRefPtr<WebContextMenuProxy> WebView::createContextMenuProxy(WebPageProxy*)
{
    notImplemented();
    return 0;
}

#if ENABLE(INPUT_TYPE_COLOR)
PassRefPtr<WebColorPicker> WebView::createColorPicker(WebPageProxy*, const WebCore::Color&, const WebCore::IntRect&)
{
    notImplemented();
    return 0;
}
#endif

void WebView::setFindIndicator(PassRefPtr<FindIndicator>, bool, bool)
{
    notImplemented();
}

void WebView::enterAcceleratedCompositingMode(const LayerTreeContext&)
{
    if (CoordinatedGraphicsScene* scene = coordinatedGraphicsScene())
        scene->setActive(true);
}

void WebView::exitAcceleratedCompositingMode()
{
    if (CoordinatedGraphicsScene* scene = coordinatedGraphicsScene())
        scene->setActive(false);
}

void WebView::updateAcceleratedCompositingMode(const LayerTreeContext&)
{
    notImplemented();
}

void WebView::flashBackingStoreUpdates(const Vector<IntRect>&)
{
    notImplemented();
}

void WebView::updateTextInputState()
{
    notImplemented();
}

void WebView::handleDownloadRequest(DownloadProxy*)
{
    notImplemented();
}

FloatRect WebView::convertToDeviceSpace(const FloatRect& userRect)
{
    if (m_page->useFixedLayout()) {
        FloatRect result = userRect;
        result.scale(m_page->deviceScaleFactor());
        return result;
    }
    // Legacy mode.
    notImplemented();
    return userRect;
}

FloatRect WebView::convertToUserSpace(const FloatRect& deviceRect)
{
    if (m_page->useFixedLayout()) {
        FloatRect result = deviceRect;
        result.scale(1 / m_page->deviceScaleFactor());
        return result;
    }
    // Legacy mode.
    notImplemented();
    return deviceRect;
}

void WebView::didChangeViewportProperties(const WebCore::ViewportAttributes& attr)
{
    m_client.didChangeViewportAttributes(this, attr);
}

void WebView::pageDidRequestScroll(const IntPoint& position)
{
    FloatPoint uiPosition(position);
    uiPosition.scale(contentScaleFactor(), contentScaleFactor());
    setContentPosition(uiPosition);

    m_client.didChangeContentsPosition(this, position);
}

void WebView::didRenderFrame(const WebCore::IntSize& contentsSize, const WebCore::IntRect& coveredRect)
{
    m_client.didRenderFrame(this, contentsSize, coveredRect);
}

void WebView::pageTransitionViewportReady()
{
    m_client.didCompletePageTransition(this);
}

void WebView::findZoomableAreaForPoint(const IntPoint& point, const IntSize& size)
{
    m_page->findZoomableAreaForPoint(transformFromScene().mapPoint(point), transformFromScene().mapSize(size));
}

} // namespace WebKit

#endif // USE(COORDINATED_GRAPHICS)

