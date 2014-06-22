/*
 * Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies)
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this program; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 */

#include "config.h"
#include "QtPageClient.h"

#include "DefaultUndoController.h"
#include "DrawingAreaProxy.h"
#include "LayerTreeContext.h"
#include "QtWebPageEventHandler.h"
#include "ShareableBitmap.h"
#if ENABLE(INPUT_TYPE_COLOR)
#include "WebColorPickerQt.h"
#endif
#include "WebContextMenuProxyQt.h"
#include "WebEditCommandProxy.h"
#include "WebPopupMenuProxyQt.h"
#include "qquickwebview_p.h"
#include "qquickwebview_p_p.h"
#include <QGuiApplication>
#include <QQuickWindow>
#include <WebCore/Cursor.h>
#include <WebCore/DragData.h>
#include <WebCore/FloatRect.h>
#include <WebCore/NotImplemented.h>

using namespace WebCore;

namespace WebKit {

QtPageClient::QtPageClient()
    : m_webView(0)
    , m_eventHandler(0)
    , m_undoController(0)
{
}

QtPageClient::~QtPageClient()
{
}

void QtPageClient::initialize(QQuickWebView* webView, QtWebPageEventHandler* eventHandler, DefaultUndoController* undoController)
{
    m_webView = webView;
    m_eventHandler = eventHandler;
    m_undoController = undoController;
}

PassOwnPtr<DrawingAreaProxy> QtPageClient::createDrawingAreaProxy()
{
    return QQuickWebViewPrivate::get(m_webView)->createDrawingAreaProxy();
}

void QtPageClient::setViewNeedsDisplay(const WebCore::IntRect& rect)
{
    QQuickWebViewPrivate::get(m_webView)->setNeedsDisplay();
}

void QtPageClient::didRenderFrame(const WebCore::IntSize& contentsSize, const WebCore::IntRect& coveredRect)
{
    // The viewport has to be notified first so that the viewport position
    // is adjusted before the loadVisuallyCommitted() signal.
    PageViewportController* pvc = QQuickWebViewPrivate::get(m_webView)->viewportController();
    if (pvc)
        pvc->didRenderFrame(contentsSize, coveredRect);
    QQuickWebViewPrivate::get(m_webView)->didRenderFrame();
}

void QtPageClient::pageDidRequestScroll(const IntPoint& pos)
{
    QQuickWebViewPrivate::get(m_webView)->pageDidRequestScroll(pos);
}

void QtPageClient::processDidCrash()
{
    QQuickWebViewPrivate::get(m_webView)->processDidCrash();
}

void QtPageClient::didRelaunchProcess()
{
    QQuickWebViewPrivate::get(m_webView)->didRelaunchProcess();
}

void QtPageClient::didChangeContentsSize(const IntSize& newSize)
{
    PageViewportController* pvc = QQuickWebViewPrivate::get(m_webView)->viewportController();
    if (pvc)
        pvc->didChangeContentsSize(newSize);
}

void QtPageClient::didChangeViewportProperties(const WebCore::ViewportAttributes& attr)
{
    QQuickWebViewPrivate::get(m_webView)->didChangeViewportProperties(attr);
}

void QtPageClient::startDrag(const WebCore::DragData& dragData, PassRefPtr<ShareableBitmap> dragImage)
{
    m_eventHandler->startDrag(dragData, dragImage);
}

void QtPageClient::handleDownloadRequest(DownloadProxy* download)
{
    QQuickWebViewPrivate::get(m_webView)->handleDownloadRequest(download);
}

void QtPageClient::handleApplicationSchemeRequest(PassRefPtr<QtRefCountedNetworkRequestData> requestData)
{
    if (!m_webView || !m_webView->experimental())
        return;
    m_webView->experimental()->invokeApplicationSchemeHandler(requestData);
}

void QtPageClient::handleAuthenticationRequiredRequest(const String& hostname, const String& realm, const String& prefilledUsername, String& username, String& password)
{
    QString qUsername;
    QString qPassword;

    QQuickWebViewPrivate::get(m_webView)->handleAuthenticationRequiredRequest(hostname, realm, prefilledUsername, qUsername, qPassword);

    username = qUsername;
    password = qPassword;
}

void QtPageClient::handleCertificateVerificationRequest(const String& hostname, bool& ignoreErrors)
{
    ignoreErrors = QQuickWebViewPrivate::get(m_webView)->handleCertificateVerificationRequest(hostname);
}

void QtPageClient::handleProxyAuthenticationRequiredRequest(const String& hostname, uint16_t port, const String& prefilledUsername, String& username, String& password)
{
    QString qUsername;
    QString qPassword;

    QQuickWebViewPrivate::get(m_webView)->handleProxyAuthenticationRequiredRequest(hostname, port, prefilledUsername, qUsername, qPassword);

    username = qUsername;
    password = qPassword;
}

void QtPageClient::setCursor(const WebCore::Cursor& cursor)
{
    m_webView->setCursor(*cursor.platformCursor());
}

void QtPageClient::setCursorHiddenUntilMouseMoves(bool hiddenUntilMouseMoves)
{
    notImplemented();
}

void QtPageClient::toolTipChanged(const String&, const String& newTooltip)
{
    // There is not yet any UI defined for the tooltips for mobile so we ignore the change.
}

void QtPageClient::registerEditCommand(PassRefPtr<WebEditCommandProxy> command, WebPageProxy::UndoOrRedo undoOrRedo)
{
    m_undoController->registerEditCommand(command, undoOrRedo);
}

void QtPageClient::clearAllEditCommands()
{
    m_undoController->clearAllEditCommands();
}

bool QtPageClient::canUndoRedo(WebPageProxy::UndoOrRedo undoOrRedo)
{
    return m_undoController->canUndoRedo(undoOrRedo);
}

void QtPageClient::executeUndoRedo(WebPageProxy::UndoOrRedo undoOrRedo)
{
    m_undoController->executeUndoRedo(undoOrRedo);
}

FloatRect QtPageClient::convertToDeviceSpace(const FloatRect& rect)
{
    return rect;
}

FloatRect QtPageClient::convertToUserSpace(const FloatRect& rect)
{
    return rect;
}

IntPoint QtPageClient::screenToWindow(const IntPoint& point)
{
    return point;
}

IntRect QtPageClient::windowToScreen(const IntRect& rect)
{
    return rect;
}

PassRefPtr<WebPopupMenuProxy> QtPageClient::createPopupMenuProxy(WebPageProxy* webPageProxy)
{
    return WebPopupMenuProxyQt::create(webPageProxy, m_webView);
}

PassRefPtr<WebContextMenuProxy> QtPageClient::createContextMenuProxy(WebPageProxy* webPageProxy)
{
    return WebContextMenuProxyQt::create(webPageProxy);
}

#if ENABLE(INPUT_TYPE_COLOR)
PassRefPtr<WebColorPicker> QtPageClient::createColorPicker(WebPageProxy* webPageProxy, const WebCore::Color& initialColor, const WebCore::IntRect& elementRect)
{
    return WebColorPickerQt::create(webPageProxy, m_webView, initialColor, elementRect);
}
#endif

void QtPageClient::flashBackingStoreUpdates(const Vector<IntRect>&)
{
    notImplemented();
}

void QtPageClient::pageTransitionViewportReady()
{
    PageViewportController* pvc = QQuickWebViewPrivate::get(m_webView)->viewportController();
    if (pvc)
        pvc->pageTransitionViewportReady();
}

void QtPageClient::didFindZoomableArea(const IntPoint& target, const IntRect& area)
{
    ASSERT(m_eventHandler);
    m_eventHandler->didFindZoomableArea(target, area);
}

void QtPageClient::updateTextInputState()
{
    ASSERT(m_eventHandler);
    m_eventHandler->updateTextInputState();
}

void QtPageClient::handleWillSetInputMethodState()
{
    ASSERT(m_eventHandler);
    m_eventHandler->handleWillSetInputMethodState();
}

#if ENABLE(GESTURE_EVENTS)
void QtPageClient::doneWithGestureEvent(const WebGestureEvent& event, bool wasEventHandled)
{
    ASSERT(m_eventHandler);
    m_eventHandler->doneWithGestureEvent(event, wasEventHandled);
}
#endif

#if ENABLE(TOUCH_EVENTS)
void QtPageClient::doneWithTouchEvent(const NativeWebTouchEvent& event, bool wasEventHandled)
{
    ASSERT(m_eventHandler);
    m_eventHandler->doneWithTouchEvent(event, wasEventHandled);
}
#endif

void QtPageClient::displayView()
{
    // FIXME: Implement.
}

void QtPageClient::scrollView(const WebCore::IntRect& scrollRect, const WebCore::IntSize& scrollOffset)
{
    // FIXME: Implement.
}

WebCore::IntSize QtPageClient::viewSize()
{
    return QQuickWebViewPrivate::get(m_webView)->viewSize();
}

bool QtPageClient::isViewWindowActive()
{
    // FIXME: The scene graph does not have the concept of being active or not when this was written.
    return true;
}

bool QtPageClient::isViewFocused()
{
    if (!m_webView)
        return false;
    return m_webView->hasFocus();
}

bool QtPageClient::isViewVisible()
{
    if (!m_webView)
        return false;

    // FIXME: this is a workaround while QWindow::isExposed() is not ready.
    if (m_webView->window() && m_webView->window()->windowState() == Qt::WindowMinimized)
        return false;

    return m_webView->isVisible() && m_webView->page()->isVisible();
}

bool QtPageClient::isViewInWindow()
{
    // FIXME: Implement.
    return true;
}

void QtPageClient::enterAcceleratedCompositingMode(const LayerTreeContext&)
{
    // FIXME: Implement.
}

void QtPageClient::exitAcceleratedCompositingMode()
{
    // FIXME: Implement.
}

void QtPageClient::updateAcceleratedCompositingMode(const LayerTreeContext&)
{
    // FIXME: Implement.
}

} // namespace WebKit
