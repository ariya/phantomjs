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

#include "config.h"
#include "qrawwebview_p.h"

#include "CoordinatedLayerTreeHostProxy.h"
#include "Cursor.h"
#include "DrawingAreaProxyImpl.h"
#include "NativeWebKeyboardEvent.h"
#include "NativeWebMouseEvent.h"
#if ENABLE(TOUCH_EVENTS)
#include "NativeWebTouchEvent.h"
#endif
#include "NativeWebWheelEvent.h"
#include "NotImplemented.h"
#include "WebContext.h"
#include "WebPageGroup.h"
#include "WebPreferences.h"
#include "qrawwebview_p_p.h"
#include <WebCore/CoordinatedGraphicsScene.h>
#include <WebKit2/qrawwebview_p.h>

void QRawWebViewPrivate::didChangeViewportProperties(const WebCore::ViewportAttributes& attr)
{
    notImplemented();
}

void QRawWebViewPrivate::handleDownloadRequest(WebKit::DownloadProxy* download)
{
    notImplemented();
}

void QRawWebViewPrivate::handleAuthenticationRequiredRequest(const String& hostname, const String& realm, const String& prefilledUsername, String& username, String& password)
{
    notImplemented();
}

void QRawWebViewPrivate::handleCertificateVerificationRequest(const String& hostname, bool& ignoreErrors)
{
    notImplemented();
}

void QRawWebViewPrivate::handleProxyAuthenticationRequiredRequest(const String& hostname, uint16_t port, const String& prefilledUsername, String& username, String& password)
{
    notImplemented();
}

void QRawWebViewPrivate::registerEditCommand(PassRefPtr<WebKit::WebEditCommandProxy>, WebKit::WebPageProxy::UndoOrRedo)
{
    notImplemented();
}

bool QRawWebViewPrivate::canUndoRedo(WebKit::WebPageProxy::UndoOrRedo undoOrRedo)
{
    notImplemented();
    return false;
}

void QRawWebViewPrivate::executeUndoRedo(WebKit::WebPageProxy::UndoOrRedo undoOrRedo)
{
    notImplemented();
}

WebCore::FloatRect QRawWebViewPrivate::convertToDeviceSpace(const WebCore::FloatRect& rect)
{
    notImplemented();
    return rect;
}

WebCore::FloatRect QRawWebViewPrivate::convertToUserSpace(const WebCore::FloatRect& rect)
{
    notImplemented();
    return rect;
}

WebCore::IntPoint QRawWebViewPrivate::screenToWindow(const WebCore::IntPoint& point)
{
    notImplemented();
    return point;
}

WebCore::IntRect QRawWebViewPrivate::windowToScreen(const WebCore::IntRect& rect)
{
    notImplemented();
    return rect;
}

#if USE(ACCELERATED_COMPOSITING)
void QRawWebViewPrivate::enterAcceleratedCompositingMode(const WebKit::LayerTreeContext&)
{
    notImplemented();
}

void QRawWebViewPrivate::exitAcceleratedCompositingMode()
{
    notImplemented();
}

void QRawWebViewPrivate::updateAcceleratedCompositingMode(const WebKit::LayerTreeContext&)
{
    notImplemented();
}

#endif // USE(ACCELERATED_COMPOSITING)

void QRawWebViewPrivate::updateTextInputState()
{
    notImplemented();
}

void QRawWebViewPrivate::handleWillSetInputMethodState()
{
    notImplemented();
}

#if ENABLE(GESTURE_EVENTS)
void QRawWebViewPrivate::doneWithGestureEvent(const WebKit::WebGestureEvent& event, bool wasEventHandled)
{
    notImplemented();
}

#endif
void QRawWebViewPrivate::displayView()
{
    notImplemented();
}

void QRawWebViewPrivate::scrollView(const WebCore::IntRect& scrollRect, const WebCore::IntSize& scrollOffset)
{
    notImplemented();
}

void QRawWebViewPrivate::flashBackingStoreUpdates(const Vector<WebCore::IntRect>&)
{
    notImplemented();
}

void QRawWebViewPrivate::didFindZoomableArea(const WebCore::IntPoint& target, const WebCore::IntRect& area)
{
    notImplemented();
}

void QRawWebViewPrivate::setCursorHiddenUntilMouseMoves(bool hiddenUntilMouseMoves)
{
    notImplemented();
}

void QRawWebViewPrivate::toolTipChanged(const String&, const String& newTooltip)
{
    notImplemented();
}

void QRawWebViewPrivate::pageTransitionViewportReady()
{
    m_webPageProxy->commitPageTransitionViewport();
}

void QRawWebViewPrivate::startDrag(const WebCore::DragData& dragData, PassRefPtr<WebKit::ShareableBitmap> dragImage)
{
    notImplemented();
}

PassRefPtr<WebKit::WebPopupMenuProxy> QRawWebViewPrivate::createPopupMenuProxy(WebKit::WebPageProxy* webPageProxy)
{
    notImplemented();
    return PassRefPtr<WebKit::WebPopupMenuProxy>();
}

PassRefPtr<WebKit::WebContextMenuProxy> QRawWebViewPrivate::createContextMenuProxy(WebKit::WebPageProxy* webPageProxy)
{
    notImplemented();
    return PassRefPtr<WebKit::WebContextMenuProxy>();
}

#if ENABLE(INPUT_TYPE_COLOR)
PassRefPtr<WebKit::WebColorPicker> QRawWebViewPrivate::createColorPicker(WebKit::WebPageProxy*, const WebCore::Color& intialColor, const WebCore::IntRect&)
{
    notImplemented();
    return PassRefPtr<WebKit::WebColorPicker>();
}
#endif

void QRawWebViewPrivate::pageDidRequestScroll(const WebCore::IntPoint& pos)
{
    m_client->viewRequestedScroll(pos);

}
void QRawWebViewPrivate::processDidCrash()
{
    m_client->viewProcessCrashed();
}

void QRawWebViewPrivate::didRelaunchProcess()
{
    m_client->viewProcessRelaunched();
}

void QRawWebViewPrivate::setViewNeedsDisplay(const WebCore::IntRect& rect)
{
    m_client->viewNeedsDisplay(rect);
}

void QRawWebViewPrivate::didChangeContentsSize(const WebCore::IntSize& newSize)
{
    m_client->viewContentSizeChanged(newSize);
}

void QRawWebViewPrivate::setCursor(const WebCore::Cursor& cursor)
{
    m_client->viewRequestedCursorOverride(*cursor.platformCursor());
}

#if ENABLE(TOUCH_EVENTS)
void QRawWebViewPrivate::doneWithTouchEvent(const WebKit::NativeWebTouchEvent& event, bool wasEventHandled)
{
    m_client->doneWithTouchEvent(event.nativeEvent(), wasEventHandled);
}
#endif

void QRawWebViewPrivate::doneWithKeyEvent(const WebKit::NativeWebKeyboardEvent& event, bool wasEventHandled)
{
    m_client->doneWithKeyEvent(event.nativeEvent(), wasEventHandled);
}

PassOwnPtr<WebKit::DrawingAreaProxy> QRawWebViewPrivate::createDrawingAreaProxy()
{
    return WebKit::DrawingAreaProxyImpl::create(m_webPageProxy.get());
}

QRawWebViewPrivate::QRawWebViewPrivate(WebKit::WebContext* context, WebKit::WebPageGroup* pageGroup, QRawWebViewClient* client)
    : m_focused(false)
    , m_visible(false)
    , m_active(false)
    , m_client(client)
    , m_webPageProxy(context->createWebPage(this, pageGroup))
{
    m_webPageProxy->pageGroup()->preferences()->setForceCompositingMode(true);
}

QRawWebViewPrivate::~QRawWebViewPrivate()
{
}

QRawWebView::QRawWebView(WKContextRef contextRef, WKPageGroupRef pageGroupRef, QRawWebViewClient* client)
    : d(new QRawWebViewPrivate(WebKit::toImpl(contextRef), WebKit::toImpl(pageGroupRef), client))
{
}

QRawWebView::~QRawWebView()
{
    delete d;
}

void QRawWebView::create()
{
    d->m_webPageProxy->initializeWebPage();
}

void QRawWebView::setTransparentBackground(bool value)
{
    d->m_webPageProxy->setDrawsTransparentBackground(value);
}

bool QRawWebView::transparentBackground() const
{
    return d->m_webPageProxy->drawsTransparentBackground();
}

void QRawWebView::setDrawBackground(bool value)
{
    d->m_webPageProxy->setDrawsBackground(value);
}

bool QRawWebView::drawBackground() const
{
    return d->m_webPageProxy->drawsBackground();
}

bool QRawWebView::isFocused() const
{
    return d->m_focused;
}

void QRawWebView::setFocused(bool focused)
{
    d->m_focused = focused;
    d->m_webPageProxy->viewStateDidChange(WebKit::WebPageProxy::ViewIsFocused);
}

bool QRawWebView::isVisible() const
{
    return d->m_visible;
}

void QRawWebView::setVisible(bool visible)
{
    d->m_visible = visible;
    d->m_webPageProxy->viewStateDidChange(WebKit::WebPageProxy::ViewIsVisible);
}

bool QRawWebView::isActive() const
{
    return d->m_active;
}

void QRawWebView::setActive(bool active)
{
    d->m_active = active;
    d->m_webPageProxy->viewStateDidChange(WebKit::WebPageProxy::ViewWindowIsActive);
    coordinatedGraphicsScene()->setActive(active);
}

QSize QRawWebView::size() const
{
    return d->m_size;
}

void QRawWebView::setSize(const QSize& size)
{
    WebKit::DrawingAreaProxy* drawingArea = d->m_webPageProxy->drawingArea();
    if (!drawingArea)
        return;

    if (d->m_webPageProxy->useFixedLayout())
        drawingArea->setSize(size, WebCore::IntSize(), WebCore::IntSize());

    d->m_size = size;

    drawingArea->setSize(d->m_size, WebCore::IntSize(), WebCore::IntSize());
    drawingArea->setVisibleContentsRect(WebCore::IntRect(WebCore::IntPoint(), d->m_size), WebCore::FloatPoint());
}

WKPageRef QRawWebView::pageRef()
{
    return toAPI(d->m_webPageProxy.get());
}

WebCore::CoordinatedGraphicsScene* QRawWebView::coordinatedGraphicsScene() const
{
    WebKit::DrawingAreaProxy* drawingArea = d->m_webPageProxy->drawingArea();
    if (!drawingArea)
        return 0;
    WebKit::CoordinatedLayerTreeHostProxy* coordinatedLayerTreeHostProxy = drawingArea->coordinatedLayerTreeHostProxy();
    if (!coordinatedLayerTreeHostProxy)
        return 0;
    return coordinatedLayerTreeHostProxy->coordinatedGraphicsScene();
}

void QRawWebView::paint(const QMatrix4x4& transform, float opacity, unsigned paintFlags)
{
    WebCore::CoordinatedGraphicsScene* scene = coordinatedGraphicsScene();
    if (!scene)
        return;

    scene->setActive(true);

    WebCore::FloatRect rect(0, 0, d->m_size.width(), d->m_size.height());

    scene->paintToCurrentGLContext(transform, opacity, transform.mapRect(rect), paintFlags);
}

void QRawWebView::sendKeyEvent(QKeyEvent* event)
{
    d->m_webPageProxy->handleKeyboardEvent(WebKit::NativeWebKeyboardEvent(event));
}

void QRawWebView::sendMouseEvent(QMouseEvent* event, int clickCount)
{
    d->m_webPageProxy->handleMouseEvent(WebKit::NativeWebMouseEvent(event, QTransform(), clickCount));
}

void QRawWebView::sendWheelEvent(QWheelEvent* event)
{
    d->m_webPageProxy->handleWheelEvent(WebKit::NativeWebWheelEvent(event, QTransform()));
}

void QRawWebView::sendTouchEvent(QTouchEvent* event)
{
#if ENABLE(TOUCH_EVENTS)
    d->m_webPageProxy->handleTouchEvent(WebKit::NativeWebTouchEvent(event, QTransform()));
#endif
}
