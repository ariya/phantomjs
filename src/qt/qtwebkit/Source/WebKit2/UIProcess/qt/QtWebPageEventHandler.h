/*
 * Copyright (C) 2010, 2011 Nokia Corporation and/or its subsidiary(-ies)
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

#ifndef QtWebPageEventHandler_h
#define QtWebPageEventHandler_h

#include "QtPanGestureRecognizer.h"
#include "QtPinchGestureRecognizer.h"
#include "QtTapGestureRecognizer.h"
#include <QBasicTimer>
#include <QTouchEvent>
#include <WKPage.h>
#include <wtf/PassRefPtr.h>

QT_BEGIN_NAMESPACE
class QInputMethodEvent;
class QKeyEvent;
QT_END_NAMESPACE

class QQuickWebPage;
class QQuickWebView;

namespace WebCore {
class DragData;
class IntPoint;
class IntRect;
}

namespace WebKit {

class NativeWebTouchEvent;
class PageViewportControllerClientQt;
class ShareableBitmap;
class WebGestureEvent;
class WebPageProxy;

class QtWebPageEventHandler : public QObject {
    Q_OBJECT

public:
    QtWebPageEventHandler(WKPageRef, QQuickWebPage*, QQuickWebView*);
    ~QtWebPageEventHandler();

    void handleKeyPressEvent(QKeyEvent*);
    void handleKeyReleaseEvent(QKeyEvent*);
    void handleFocusInEvent(QFocusEvent*);
    void handleFocusLost();
    void handleMouseMoveEvent(QMouseEvent*);
    void handleMousePressEvent(QMouseEvent*);
    void handleMouseReleaseEvent(QMouseEvent*);
    void handleWheelEvent(QWheelEvent*);
    void handleHoverLeaveEvent(QHoverEvent*);
    void handleHoverMoveEvent(QHoverEvent*);
    void handleDragEnterEvent(QDragEnterEvent*);
    void handleDragLeaveEvent(QDragLeaveEvent*);
    void handleDragMoveEvent(QDragMoveEvent*);
    void handleDropEvent(QDropEvent*);
    void handleInputMethodEvent(QInputMethodEvent*);
    void handleTouchEvent(QTouchEvent*);

    void setViewportController(PageViewportControllerClientQt*);

    void activateTapHighlight(const QTouchEvent::TouchPoint&);
    void deactivateTapHighlight();
    void handleSingleTapEvent(const QTouchEvent::TouchPoint&);
    void handleDoubleTapEvent(const QTouchEvent::TouchPoint&);

    void didFindZoomableArea(const WebCore::IntPoint& target, const WebCore::IntRect& area);
    void updateTextInputState();
    void doneWithGestureEvent(const WebGestureEvent& event, bool wasEventHandled);
#if ENABLE(TOUCH_EVENTS)
    void doneWithTouchEvent(const NativeWebTouchEvent&, bool wasEventHandled);
#endif
    void handleInputEvent(const QInputEvent*);
    void handleWillSetInputMethodState();
    void resetGestureRecognizers();

    PageViewportControllerClientQt* viewportController() { return m_viewportController; }

    void startDrag(const WebCore::DragData&, PassRefPtr<ShareableBitmap> dragImage);

protected:
    WebPageProxy* m_webPageProxy;
    PageViewportControllerClientQt* m_viewportController;
    QtPanGestureRecognizer m_panGestureRecognizer;
    QtPinchGestureRecognizer m_pinchGestureRecognizer;
    QtTapGestureRecognizer m_tapGestureRecognizer;
    QQuickWebPage* m_webPage;
    QQuickWebView* m_webView;

private Q_SLOTS:
    void inputPanelVisibleChanged();

private:
    void timerEvent(QTimerEvent*);

    QPointF m_lastClick;
    QBasicTimer m_clickTimer;
    Qt::MouseButton m_previousClickButton;
    int m_clickCount;
    bool m_postponeTextInputStateChanged;
    bool m_isTapHighlightActive;
    bool m_isMouseButtonPressed;
};

} // namespace WebKit

#endif /* QtWebPageEventHandler_h */
