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

#include "config.h"
#include "QtWebPageEventHandler.h"

#include "NativeWebKeyboardEvent.h"
#include "NativeWebMouseEvent.h"
#include "NativeWebWheelEvent.h"
#include "PageViewportControllerClientQt.h"
#include "WebPageProxy.h"
#include "qquickwebpage_p.h"
#include "qquickwebview_p.h"
#include <QCursor>
#include <QDrag>
#include <QGuiApplication>
#include <QInputEvent>
#include <QInputMethod>
#include <QMimeData>
#include <QMouseEvent>
#include <QQuickWindow>
#include <QStyleHints>
#include <QTextFormat>
#include <QTouchEvent>
#include <QTransform>
#include <WebCore/DragData.h>
#include <WebCore/Editor.h>

using namespace WebCore;

namespace WebKit {

static inline Qt::DropAction dragOperationToDropAction(unsigned dragOperation)
{
    Qt::DropAction result = Qt::IgnoreAction;
    if (dragOperation & DragOperationCopy)
        result = Qt::CopyAction;
    else if (dragOperation & DragOperationMove)
        result = Qt::MoveAction;
    else if (dragOperation & DragOperationGeneric)
        result = Qt::MoveAction;
    else if (dragOperation & DragOperationLink)
        result = Qt::LinkAction;
    return result;
}

static inline Qt::DropActions dragOperationToDropActions(unsigned dragOperations)
{
    Qt::DropActions result = Qt::IgnoreAction;
    if (dragOperations & DragOperationCopy)
        result |= Qt::CopyAction;
    if (dragOperations & DragOperationMove)
        result |= Qt::MoveAction;
    if (dragOperations & DragOperationGeneric)
        result |= Qt::MoveAction;
    if (dragOperations & DragOperationLink)
        result |= Qt::LinkAction;
    return result;
}

static inline WebCore::DragOperation dropActionToDragOperation(Qt::DropActions actions)
{
    unsigned result = 0;
    if (actions & Qt::CopyAction)
        result |= DragOperationCopy;
    if (actions & Qt::MoveAction)
        result |= (DragOperationMove | DragOperationGeneric);
    if (actions & Qt::LinkAction)
        result |= DragOperationLink;
    if (result == (DragOperationCopy | DragOperationMove | DragOperationGeneric | DragOperationLink))
        result = DragOperationEvery;
    return (DragOperation)result;
}

QtWebPageEventHandler::QtWebPageEventHandler(WKPageRef pageRef, QQuickWebPage* qmlWebPage, QQuickWebView* qmlWebView)
    : m_webPageProxy(toImpl(pageRef))
    , m_viewportController(0)
    , m_panGestureRecognizer(this)
    , m_pinchGestureRecognizer(this)
    , m_tapGestureRecognizer(this)
    , m_webPage(qmlWebPage)
    , m_webView(qmlWebView)
    , m_previousClickButton(Qt::NoButton)
    , m_clickCount(0)
    , m_postponeTextInputStateChanged(false)
    , m_isTapHighlightActive(false)
    , m_isMouseButtonPressed(false)
{
    connect(qApp->inputMethod(), SIGNAL(visibleChanged()), this, SLOT(inputPanelVisibleChanged()));
}

QtWebPageEventHandler::~QtWebPageEventHandler()
{
    disconnect(qApp->inputMethod(), SIGNAL(visibleChanged()), this, SLOT(inputPanelVisibleChanged()));
}

void QtWebPageEventHandler::handleMouseMoveEvent(QMouseEvent* ev)
{
    // For some reason mouse press results in mouse hover (which is
    // converted to mouse move for WebKit). We ignore these hover
    // events by comparing lastPos with newPos.
    // NOTE: lastPos from the event always comes empty, so we work
    // around that here.
    static QPointF lastPos = QPointF();
    QTransform fromItemTransform = m_webPage->transformFromItem();
    QPointF webPagePoint = fromItemTransform.map(ev->localPos());
    ev->accept();
    if (lastPos == webPagePoint)
        return;
    lastPos = webPagePoint;

    m_webPageProxy->handleMouseEvent(NativeWebMouseEvent(ev, fromItemTransform, /*eventClickCount*/ 0));
}

void QtWebPageEventHandler::handleMousePressEvent(QMouseEvent* ev)
{
    QTransform fromItemTransform = m_webPage->transformFromItem();
    QPointF webPagePoint = fromItemTransform.map(ev->localPos());

    if (m_clickTimer.isActive()
        && m_previousClickButton == ev->button()
        && (webPagePoint - m_lastClick).manhattanLength() < qApp->styleHints()->startDragDistance()) {
        m_clickCount++;
    } else {
        m_clickCount = 1;
        m_previousClickButton = ev->button();
    }

    ev->accept();
    m_webPageProxy->handleMouseEvent(NativeWebMouseEvent(ev, fromItemTransform, m_clickCount));

    m_lastClick = webPagePoint;
    m_clickTimer.start(qApp->styleHints()->mouseDoubleClickInterval(), this);
}

void QtWebPageEventHandler::handleMouseReleaseEvent(QMouseEvent* ev)
{
    ev->accept();
    QTransform fromItemTransform = m_webPage->transformFromItem();
    m_webPageProxy->handleMouseEvent(NativeWebMouseEvent(ev, fromItemTransform, /*eventClickCount*/ 0));
}

void QtWebPageEventHandler::handleWheelEvent(QWheelEvent* ev)
{
    QTransform fromItemTransform = m_webPage->transformFromItem();
    m_webPageProxy->handleWheelEvent(NativeWebWheelEvent(ev, fromItemTransform));
}

void QtWebPageEventHandler::handleHoverLeaveEvent(QHoverEvent* ev)
{
    // To get the correct behavior of mouseout, we need to turn the Leave event of our webview into a mouse move
    // to a very far region.
    QHoverEvent fakeEvent(QEvent::HoverMove, QPoint(INT_MIN, INT_MIN), ev->oldPosF());
    fakeEvent.setTimestamp(ev->timestamp());
    // This will apply the transform on the event.
    handleHoverMoveEvent(&fakeEvent);
}

void QtWebPageEventHandler::handleHoverMoveEvent(QHoverEvent* ev)
{
    QMouseEvent me(QEvent::MouseMove, ev->posF(), Qt::NoButton, Qt::NoButton, Qt::NoModifier);
    me.setAccepted(ev->isAccepted());
    me.setTimestamp(ev->timestamp());
    // This will apply the transform on the event.
    handleMouseMoveEvent(&me);
}

void QtWebPageEventHandler::handleDragEnterEvent(QDragEnterEvent* ev)
{
    m_webPageProxy->resetDragOperation();
    QTransform fromItemTransform = m_webPage->transformFromItem();
    // FIXME: Should not use QCursor::pos()
    DragData dragData(ev->mimeData(), fromItemTransform.map(ev->pos()), QCursor::pos(), dropActionToDragOperation(ev->possibleActions()));
    m_webPageProxy->dragEntered(&dragData);
    ev->acceptProposedAction();
}

void QtWebPageEventHandler::handleDragLeaveEvent(QDragLeaveEvent* ev)
{
    bool accepted = ev->isAccepted();

    // FIXME: Should not use QCursor::pos()
    DragData dragData(0, IntPoint(), QCursor::pos(), DragOperationNone);
    m_webPageProxy->dragExited(&dragData);
    m_webPageProxy->resetDragOperation();

    ev->setAccepted(accepted);
}

void QtWebPageEventHandler::handleDragMoveEvent(QDragMoveEvent* ev)
{
    bool accepted = ev->isAccepted();

    QTransform fromItemTransform = m_webPage->transformFromItem();
    // FIXME: Should not use QCursor::pos()
    DragData dragData(ev->mimeData(), fromItemTransform.map(ev->pos()), QCursor::pos(), dropActionToDragOperation(ev->possibleActions()));
    m_webPageProxy->dragUpdated(&dragData);
    ev->setDropAction(dragOperationToDropAction(m_webPageProxy->dragSession().operation));
    if (m_webPageProxy->dragSession().operation != DragOperationNone)
        ev->accept();

    ev->setAccepted(accepted);
}

void QtWebPageEventHandler::handleDropEvent(QDropEvent* ev)
{
    bool accepted = ev->isAccepted();
    QTransform fromItemTransform = m_webPage->transformFromItem();
    // FIXME: Should not use QCursor::pos()
    DragData dragData(ev->mimeData(), fromItemTransform.map(ev->pos()), QCursor::pos(), dropActionToDragOperation(ev->possibleActions()));
    SandboxExtension::Handle handle;
    SandboxExtension::HandleArray sandboxExtensionForUpload;
    m_webPageProxy->performDrag(&dragData, String(), handle, sandboxExtensionForUpload);
    ev->setDropAction(dragOperationToDropAction(m_webPageProxy->dragSession().operation));
    ev->accept();

    ev->setAccepted(accepted);
}

void QtWebPageEventHandler::activateTapHighlight(const QTouchEvent::TouchPoint& point)
{
#if ENABLE(TOUCH_EVENTS)
    ASSERT(!point.pos().toPoint().isNull());
    ASSERT(!m_isTapHighlightActive);
    m_isTapHighlightActive = true;
    QTransform fromItemTransform = m_webPage->transformFromItem();
    m_webPageProxy->handlePotentialActivation(IntPoint(fromItemTransform.map(point.pos()).toPoint()), IntSize(point.rect().size().toSize()));
#else
    Q_UNUSED(point);
#endif
}

void QtWebPageEventHandler::deactivateTapHighlight()
{
#if ENABLE(TOUCH_EVENTS)
    if (!m_isTapHighlightActive)
        return;

    // An empty point deactivates the highlighting.
    m_webPageProxy->handlePotentialActivation(IntPoint(), IntSize());
    m_isTapHighlightActive = false;
#endif
}

void QtWebPageEventHandler::handleSingleTapEvent(const QTouchEvent::TouchPoint& point)
{
    deactivateTapHighlight();
    m_postponeTextInputStateChanged = true;

    QTransform fromItemTransform = m_webPage->transformFromItem();
    WebGestureEvent gesture(WebEvent::GestureSingleTap, fromItemTransform.map(point.pos()).toPoint(), point.screenPos().toPoint(), WebEvent::Modifiers(0), 0, IntSize(point.rect().size().toSize()), FloatPoint(0, 0));
    m_webPageProxy->handleGestureEvent(gesture);
}

void QtWebPageEventHandler::handleDoubleTapEvent(const QTouchEvent::TouchPoint& point)
{
    if (!m_webView->isInteractive())
        return;

    deactivateTapHighlight();
    QTransform fromItemTransform = m_webPage->transformFromItem();
    m_webPageProxy->findZoomableAreaForPoint(fromItemTransform.map(point.pos()).toPoint(), IntSize(point.rect().size().toSize()));
}

void QtWebPageEventHandler::timerEvent(QTimerEvent* ev)
{
    int timerId = ev->timerId();
    if (timerId == m_clickTimer.timerId())
        m_clickTimer.stop();
    else
        QObject::timerEvent(ev);
}

void QtWebPageEventHandler::handleKeyPressEvent(QKeyEvent* ev)
{
    m_webPageProxy->handleKeyboardEvent(NativeWebKeyboardEvent(ev));
}

void QtWebPageEventHandler::handleKeyReleaseEvent(QKeyEvent* ev)
{
    m_webPageProxy->handleKeyboardEvent(NativeWebKeyboardEvent(ev));
}

void QtWebPageEventHandler::handleFocusInEvent(QFocusEvent*)
{
    m_webPageProxy->viewStateDidChange(WebPageProxy::ViewIsFocused | WebPageProxy::ViewWindowIsActive);
}

void QtWebPageEventHandler::handleFocusLost()
{
    m_webPageProxy->viewStateDidChange(WebPageProxy::ViewIsFocused | WebPageProxy::ViewWindowIsActive);
}

void QtWebPageEventHandler::setViewportController(PageViewportControllerClientQt* controller)
{
    m_viewportController = controller;
}

void QtWebPageEventHandler::handleInputMethodEvent(QInputMethodEvent* ev)
{
    QString commit = ev->commitString();
    QString composition = ev->preeditString();

    int replacementStart = ev->replacementStart();
    int replacementLength = ev->replacementLength();

    // NOTE: We might want to handle events of one char as special
    // and resend them as key events to make web site completion work.

    int cursorPositionWithinComposition = 0;

    Vector<CompositionUnderline> underlines;

    for (int i = 0; i < ev->attributes().size(); ++i) {
        const QInputMethodEvent::Attribute& attr = ev->attributes().at(i);
        switch (attr.type) {
        case QInputMethodEvent::TextFormat: {
            if (composition.isEmpty())
                break;

            QTextCharFormat textCharFormat = attr.value.value<QTextFormat>().toCharFormat();
            QColor qcolor = textCharFormat.underlineColor();
            Color color = makeRGBA(qcolor.red(), qcolor.green(), qcolor.blue(), qcolor.alpha());
            int start = qMin(attr.start, (attr.start + attr.length));
            int end = qMax(attr.start, (attr.start + attr.length));
            underlines.append(CompositionUnderline(start, end, color, false));
            break;
        }
        case QInputMethodEvent::Cursor:
            if (attr.length)
                cursorPositionWithinComposition = attr.start;
            break;
        // Selection is handled further down.
        default: break;
        }
    }

    if (composition.isEmpty()) {
        int selectionStart = -1;
        int selectionLength = 0;
        for (int i = 0; i < ev->attributes().size(); ++i) {
            const QInputMethodEvent::Attribute& attr = ev->attributes().at(i);
            if (attr.type == QInputMethodEvent::Selection) {
                selectionStart = attr.start;
                selectionLength = attr.length;

                ASSERT(selectionStart >= 0);
                ASSERT(selectionLength >= 0);
                break;
            }
        }

        m_webPageProxy->confirmComposition(commit, selectionStart, selectionLength);
    } else {
        ASSERT(cursorPositionWithinComposition >= 0);
        ASSERT(replacementStart >= 0);

        m_webPageProxy->setComposition(composition, underlines,
            cursorPositionWithinComposition, cursorPositionWithinComposition,
            replacementStart, replacementLength);
    }

    ev->accept();
}

void QtWebPageEventHandler::handleTouchEvent(QTouchEvent* event)
{
#if ENABLE(TOUCH_EVENTS)
    QTransform fromItemTransform = m_webPage->transformFromItem();
    m_webPageProxy->handleTouchEvent(NativeWebTouchEvent(event, fromItemTransform));
    event->accept();
#else
    ASSERT_NOT_REACHED();
    event->ignore();
#endif
}

void QtWebPageEventHandler::resetGestureRecognizers()
{
    m_panGestureRecognizer.cancel();
    m_pinchGestureRecognizer.cancel();
    m_tapGestureRecognizer.cancel();
}

static void setInputPanelVisible(bool visible)
{
    if (qApp->inputMethod()->isVisible() == visible)
        return;

    qApp->inputMethod()->setVisible(visible);
}

void QtWebPageEventHandler::inputPanelVisibleChanged()
{
    if (!m_viewportController)
        return;

    // We only respond to the input panel becoming visible.
    if (!m_webView->hasActiveFocus() || !qApp->inputMethod()->isVisible())
        return;

    const EditorState& editor = m_webPageProxy->editorState();
    if (editor.isContentEditable)
        m_viewportController->focusEditableArea(QRectF(editor.cursorRect), QRectF(editor.editorRect));
}

void QtWebPageEventHandler::updateTextInputState()
{
    if (m_postponeTextInputStateChanged)
        return;

    const EditorState& editor = m_webPageProxy->editorState();

    m_webView->setFlag(QQuickItem::ItemAcceptsInputMethod, editor.isContentEditable);

    if (!m_webView->hasActiveFocus())
        return;

    qApp->inputMethod()->update(Qt::ImQueryInput | Qt::ImEnabled | Qt::ImHints);

    setInputPanelVisible(editor.isContentEditable);
}

void QtWebPageEventHandler::handleWillSetInputMethodState()
{
    if (qApp->inputMethod()->isVisible())
        qApp->inputMethod()->commit();
}

void QtWebPageEventHandler::doneWithGestureEvent(const WebGestureEvent& event, bool wasEventHandled)
{
    if (event.type() != WebEvent::GestureSingleTap)
        return;

    m_postponeTextInputStateChanged = false;

    if (!wasEventHandled || !m_webView->hasActiveFocus())
        return;

    updateTextInputState();
}

void QtWebPageEventHandler::handleInputEvent(const QInputEvent* event)
{
    if (m_viewportController) {
        switch (event->type()) {
        case QEvent::MouseButtonPress:
        case QEvent::TouchBegin:
            m_viewportController->touchBegin();

            // The page viewport controller might still be animating kinetic scrolling or a scale animation
            // such as double-tap to zoom or the bounce back effect. A touch stops the kinetic scrolling
            // where as it does not stop the scale animation.
            // The gesture recognizer stops the kinetic scrolling animation if needed.
            break;
        case QEvent::MouseMove:
        case QEvent::TouchUpdate:
            // The scale animation can only be interrupted by a pinch gesture, which will then take over.
            if (m_viewportController->scaleAnimationActive() && m_pinchGestureRecognizer.isRecognized())
                m_viewportController->interruptScaleAnimation();
            break;
        case QEvent::MouseButtonRelease:
        case QEvent::TouchEnd:
            m_viewportController->touchEnd();
            break;
        default:
            break;
        }

        // If the scale animation is active we don't pass the event to the recognizers. In the future
        // we would want to queue the event here and repost then when the animation ends.
        if (m_viewportController->scaleAnimationActive())
            return;
    }

    bool isMouseEvent = false;

    switch (event->type()) {
    case QEvent::MouseButtonPress:
        isMouseEvent = true;
        m_isMouseButtonPressed = true;
        break;
    case QEvent::MouseMove:
        if (!m_isMouseButtonPressed)
            return;
        isMouseEvent = true;
        break;
    case QEvent::MouseButtonRelease:
        isMouseEvent = true;
        m_isMouseButtonPressed = false;
        break;
    case QEvent::MouseButtonDblClick:
        return;
    default:
        break;
    }

    QList<QTouchEvent::TouchPoint> activeTouchPoints;
    QTouchEvent::TouchPoint currentTouchPoint;
    qint64 eventTimestampMillis = event->timestamp();
    int touchPointCount = 0;

    if (!isMouseEvent) {
        const QTouchEvent* touchEvent = static_cast<const QTouchEvent*>(event);
        const QList<QTouchEvent::TouchPoint>& touchPoints = touchEvent->touchPoints();
        currentTouchPoint = touchPoints.first();
        touchPointCount = touchPoints.size();
        activeTouchPoints.reserve(touchPointCount);

        for (int i = 0; i < touchPointCount; ++i) {
            if (touchPoints[i].state() != Qt::TouchPointReleased)
                activeTouchPoints << touchPoints[i];
        }
    } else {
        const QMouseEvent* mouseEvent = static_cast<const QMouseEvent*>(event);
        touchPointCount = 1;

        // Make a distinction between mouse events on the basis of pressed buttons.
        currentTouchPoint.setId(mouseEvent->buttons());
        currentTouchPoint.setScreenPos(mouseEvent->screenPos());
        // For tap gesture hit testing the float touch rect is translated to
        // an int rect representing the radius of the touch point (size/2),
        // thus the touch rect has to have a size of at least 2.
        currentTouchPoint.setRect(QRectF(mouseEvent->localPos(), QSizeF(2, 2)));

        if (m_isMouseButtonPressed)
            activeTouchPoints << currentTouchPoint;
    }

    const int activeTouchPointCount = activeTouchPoints.size();

    if (!activeTouchPointCount) {
        if (touchPointCount == 1) {
            // No active touch points, one finger released.
            if (m_panGestureRecognizer.isRecognized())
                m_panGestureRecognizer.finish(currentTouchPoint, eventTimestampMillis);
            else {
                // The events did not result in a pan gesture.
                m_panGestureRecognizer.cancel();
                m_tapGestureRecognizer.finish(currentTouchPoint);
            }

        } else
            m_pinchGestureRecognizer.finish();

        // Early return since this was a touch-end event.
        return;
    } else if (activeTouchPointCount == 1) {
        // If the pinch gesture recognizer was previously in active state the content might
        // be out of valid zoom boundaries, thus we need to finish the pinch gesture here.
        // This will resume the content to valid zoom levels before the pan gesture is started.
        m_pinchGestureRecognizer.finish();
        m_panGestureRecognizer.update(activeTouchPoints.first(), eventTimestampMillis);
    } else if (activeTouchPointCount == 2) {
        m_panGestureRecognizer.cancel();
        m_pinchGestureRecognizer.update(activeTouchPoints.first(), activeTouchPoints.last());
    }

    if (m_panGestureRecognizer.isRecognized() || m_pinchGestureRecognizer.isRecognized() || m_webView->isMoving())
        m_tapGestureRecognizer.cancel();
    else if (touchPointCount == 1)
        m_tapGestureRecognizer.update(currentTouchPoint);

}

#if ENABLE(TOUCH_EVENTS)
void QtWebPageEventHandler::doneWithTouchEvent(const NativeWebTouchEvent& event, bool wasEventHandled)
{
    if (wasEventHandled || event.type() == WebEvent::TouchCancel) {
        m_panGestureRecognizer.cancel();
        m_pinchGestureRecognizer.cancel();
        if (event.type() != WebEvent::TouchMove)
            m_tapGestureRecognizer.cancel();
        return;
    }

    const QTouchEvent* ev = event.nativeEvent();

    handleInputEvent(ev);
}
#endif

void QtWebPageEventHandler::didFindZoomableArea(const IntPoint& target, const IntRect& area)
{
    if (!m_viewportController)
        return;

    // FIXME: As the find method might not respond immediately during load etc,
    // we should ignore all but the latest request.
    m_viewportController->zoomToAreaGestureEnded(QPointF(target), QRectF(area));
}

void QtWebPageEventHandler::startDrag(const WebCore::DragData& dragData, PassRefPtr<ShareableBitmap> dragImage)
{
    QImage dragQImage;
    if (dragImage)
        dragQImage = dragImage->createQImage();
    else if (dragData.platformData() && dragData.platformData()->hasImage())
        dragQImage = qvariant_cast<QImage>(dragData.platformData()->imageData());

    DragOperation dragOperationMask = dragData.draggingSourceOperationMask();
    QMimeData* mimeData = const_cast<QMimeData*>(dragData.platformData());
    Qt::DropActions supportedDropActions = dragOperationToDropActions(dragOperationMask);

    QPoint clientPosition;
    QPoint globalPosition;
    Qt::DropAction actualDropAction = Qt::IgnoreAction;

    if (QWindow* window = m_webPage->window()) {
        QDrag* drag = new QDrag(window);
        drag->setPixmap(QPixmap::fromImage(dragQImage));
        drag->setMimeData(mimeData);
        actualDropAction = drag->exec(supportedDropActions);
        globalPosition = QCursor::pos();
        clientPosition = window->mapFromGlobal(globalPosition);
    }

    m_webPageProxy->dragEnded(clientPosition, globalPosition, dropActionToDragOperation(actualDropAction));
}

} // namespace WebKit

#include "moc_QtWebPageEventHandler.cpp"

