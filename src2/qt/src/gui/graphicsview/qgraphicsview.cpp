/****************************************************************************
**
** Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtGui module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

static const int QGRAPHICSVIEW_REGION_RECT_THRESHOLD = 50;

static const int QGRAPHICSVIEW_PREALLOC_STYLE_OPTIONS = 503; // largest prime < 2^9

/*!
    \class QGraphicsView
    \brief The QGraphicsView class provides a widget for displaying the
    contents of a QGraphicsScene.
    \since 4.2
    \ingroup graphicsview-api


    QGraphicsView visualizes the contents of a QGraphicsScene in a scrollable
    viewport. To create a scene with geometrical items, see QGraphicsScene's
    documentation. QGraphicsView is part of the \l{Graphics View Framework}.

    To visualize a scene, you start by constructing a QGraphicsView object,
    passing the address of the scene you want to visualize to QGraphicsView's
    constructor. Alternatively, you can call setScene() to set the scene at a
    later point. After you call show(), the view will by default scroll to the
    center of the scene and display any items that are visible at this
    point. For example:

    \snippet doc/src/snippets/code/src_gui_graphicsview_qgraphicsview.cpp 0

    You can explicitly scroll to any position on the scene by using the
    scroll bars, or by calling centerOn(). By passing a point to centerOn(),
    QGraphicsView will scroll its viewport to ensure that the point is
    centered in the view. An overload is provided for scrolling to a
    QGraphicsItem, in which case QGraphicsView will see to that the center of
    the item is centered in the view. If all you want is to ensure that a
    certain area is visible, (but not necessarily centered,) you can call
    ensureVisible() instead.

    QGraphicsView can be used to visualize a whole scene, or only parts of it.
    The visualized area is by default detected automatically when the view is
    displayed for the first time (by calling
    QGraphicsScene::itemsBoundingRect()). To set the visualized area rectangle
    yourself, you can call setSceneRect(). This will adjust the scroll bars'
    ranges appropriately. Note that although the scene supports a virtually
    unlimited size, the range of the scroll bars will never exceed the range of
    an integer (INT_MIN, INT_MAX).

    QGraphicsView visualizes the scene by calling render(). By default, the
    items are drawn onto the viewport by using a regular QPainter, and using
    default render hints. To change the default render hints that
    QGraphicsView passes to QPainter when painting items, you can call
    setRenderHints().

    By default, QGraphicsView provides a regular QWidget for the viewport
    widget. You can access this widget by calling viewport(), or you can
    replace it by calling setViewport(). To render using OpenGL, simply call
    setViewport(new QGLWidget). QGraphicsView takes ownership of the viewport
    widget.

    QGraphicsView supports affine transformations, using QTransform. You can
    either pass a matrix to setTransform(), or you can call one of the
    convenience functions rotate(), scale(), translate() or shear(). The most
    two common transformations are scaling, which is used to implement
    zooming, and rotation. QGraphicsView keeps the center of the view fixed
    during a transformation. Because of the scene alignment (setAligment()),
    translating the view will have no visual impact.

    You can interact with the items on the scene by using the mouse and
    keyboard. QGraphicsView translates the mouse and key events into \e scene
    events, (events that inherit QGraphicsSceneEvent,), and forward them to
    the visualized scene. In the end, it's the individual item that handles
    the events and reacts to them. For example, if you click on a selectable
    item, the item will typically let the scene know that it has been
    selected, and it will also redraw itself to display a selection
    rectangle. Similiary, if you click and drag the mouse to move a movable
    item, it's the item that handles the mouse moves and moves itself.  Item
    interaction is enabled by default, and you can toggle it by calling
    setInteractive().

    You can also provide your own custom scene interaction, by creating a
    subclass of QGraphicsView, and reimplementing the mouse and key event
    handlers. To simplify how you programmatically interact with items in the
    view, QGraphicsView provides the mapping functions mapToScene() and
    mapFromScene(), and the item accessors items() and itemAt(). These
    functions allow you to map points, rectangles, polygons and paths between
    view coordinates and scene coordinates, and to find items on the scene
    using view coordinates.

    \img graphicsview-view.png

    \sa QGraphicsScene, QGraphicsItem, QGraphicsSceneEvent
*/

/*!
    \enum QGraphicsView::ViewportAnchor

    This enums describe the possible anchors that QGraphicsView can
    use when the user resizes the view or when the view is
    transformed.

    \value NoAnchor No anchor, i.e. the view leaves the scene's
                    position unchanged.
    \value AnchorViewCenter The scene point at the center of the view
                            is used as the anchor.
    \value AnchorUnderMouse The point under the mouse is used as the anchor.

    \sa resizeAnchor, transformationAnchor
*/

/*!
    \enum QGraphicsView::ViewportUpdateMode

    \since 4.3

    This enum describes how QGraphicsView updates its viewport when the scene
    contents change or are exposed.

    \value FullViewportUpdate When any visible part of the scene changes or is
    reexposed, QGraphicsView will update the entire viewport. This approach is
    fastest when QGraphicsView spends more time figuring out what to draw than
    it would spend drawing (e.g., when very many small items are repeatedly
    updated). This is the preferred update mode for viewports that do not
    support partial updates, such as QGLWidget, and for viewports that need to
    disable scroll optimization.

    \value MinimalViewportUpdate QGraphicsView will determine the minimal
    viewport region that requires a redraw, minimizing the time spent drawing
    by avoiding a redraw of areas that have not changed. This is
    QGraphicsView's default mode. Although this approach provides the best
    performance in general, if there are many small visible changes on the
    scene, QGraphicsView might end up spending more time finding the minimal
    approach than it will spend drawing.

    \value SmartViewportUpdate QGraphicsView will attempt to find an optimal
    update mode by analyzing the areas that require a redraw.

    \value BoundingRectViewportUpdate The bounding rectangle of all changes in
    the viewport will be redrawn. This mode has the advantage that
    QGraphicsView searches only one region for changes, minimizing time spent
    determining what needs redrawing. The disadvantage is that areas that have
    not changed also need to be redrawn.

    \value NoViewportUpdate QGraphicsView will never update its viewport when
    the scene changes; the user is expected to control all updates. This mode
    disables all (potentially slow) item visibility testing in QGraphicsView,
    and is suitable for scenes that either require a fixed frame rate, or where
    the viewport is otherwise updated externally.

    \sa viewportUpdateMode
*/

/*!
    \enum QGraphicsView::OptimizationFlag

    \since 4.3

    This enum describes flags that you can enable to improve rendering
    performance in QGraphicsView. By default, none of these flags are set.
    Note that setting a flag usually imposes a side effect, and this effect
    can vary between paint devices and platforms.

    \value DontClipPainter This value is obsolete and has no effect.

    \value DontSavePainterState When rendering, QGraphicsView protects the
    painter state (see QPainter::save()) when rendering the background or
    foreground, and when rendering each item. This allows you to leave the
    painter in an altered state (i.e., you can call QPainter::setPen() or
    QPainter::setBrush() without restoring the state after painting). However,
    if the items consistently do restore the state, you should enable this
    flag to prevent QGraphicsView from doing the same.

    \value DontAdjustForAntialiasing Disables QGraphicsView's antialiasing
    auto-adjustment of exposed areas. Items that render antialiased lines on
    the boundaries of their QGraphicsItem::boundingRect() can end up rendering
    parts of the line outside. To prevent rendering artifacts, QGraphicsView
    expands all exposed regions by 2 pixels in all directions. If you enable
    this flag, QGraphicsView will no longer perform these adjustments,
    minimizing the areas that require redrawing, which improves performance. A
    common side effect is that items that do draw with antialiasing can leave
    painting traces behind on the scene as they are moved.

    \value IndirectPainting Since Qt 4.6, restore the old painting algorithm
    that calls QGraphicsView::drawItems() and QGraphicsScene::drawItems().
    To be used only for compatibility with old code.
*/

/*!
    \enum QGraphicsView::CacheModeFlag

    This enum describes the flags that you can set for a QGraphicsView's cache
    mode.

    \value CacheNone All painting is done directly onto the viewport.

    \value CacheBackground The background is cached. This affects both custom
    backgrounds, and backgrounds based on the backgroundBrush property. When
    this flag is enabled, QGraphicsView will allocate one pixmap with the full
    size of the viewport.

    \sa cacheMode
*/

/*!
    \enum QGraphicsView::DragMode

    This enum describes the default action for the view when pressing and
    dragging the mouse over the viewport.

    \value NoDrag Nothing happens; the mouse event is ignored.

    \value ScrollHandDrag The cursor changes into a pointing hand, and
    dragging the mouse around will scroll the scrolbars. This mode works both
    in \l{QGraphicsView::interactive}{interactive} and non-interactive mode.

    \value RubberBandDrag A rubber band will appear. Dragging the mouse will
    set the rubber band geometry, and all items covered by the rubber band are
    selected. This mode is disabled for non-interactive views.

    \sa dragMode, QGraphicsScene::setSelectionArea()
*/

#include "qgraphicsview.h"
#include "qgraphicsview_p.h"

#ifndef QT_NO_GRAPHICSVIEW

#include "qgraphicsitem.h"
#include "qgraphicsitem_p.h"
#include "qgraphicsscene.h"
#include "qgraphicsscene_p.h"
#include "qgraphicssceneevent.h"
#include "qgraphicswidget.h"

#include <QtCore/qdatetime.h>
#include <QtCore/qdebug.h>
#include <QtCore/qmath.h>
#include <QtGui/qapplication.h>
#include <QtGui/qdesktopwidget.h>
#include <QtGui/qevent.h>
#include <QtGui/qlayout.h>
#include <QtGui/qtransform.h>
#include <QtGui/qmatrix.h>
#include <QtGui/qpainter.h>
#include <QtGui/qscrollbar.h>
#include <QtGui/qstyleoption.h>
#include <QtGui/qinputcontext.h>
#ifdef Q_WS_X11
#include <QtGui/qpaintengine.h>
#include <private/qt_x11_p.h>
#endif

#include <private/qevent_p.h>

QT_BEGIN_NAMESPACE

bool qt_sendSpontaneousEvent(QObject *receiver, QEvent *event);

inline int q_round_bound(qreal d) //### (int)(qreal) INT_MAX != INT_MAX for single precision
{
    if (d <= (qreal) INT_MIN)
        return INT_MIN;
    else if (d >= (qreal) INT_MAX)
        return INT_MAX;
    return d >= 0.0 ? int(d + qreal(0.5)) : int(d - int(d-1) + qreal(0.5)) + int(d-1);
}

void QGraphicsViewPrivate::translateTouchEvent(QGraphicsViewPrivate *d, QTouchEvent *touchEvent)
{
    QList<QTouchEvent::TouchPoint> touchPoints = touchEvent->touchPoints();
    for (int i = 0; i < touchPoints.count(); ++i) {
        QTouchEvent::TouchPoint &touchPoint = touchPoints[i];
        // the scene will set the item local pos, startPos, lastPos, and rect before delivering to
        // an item, but for now those functions are returning the view's local coordinates
        touchPoint.setSceneRect(d->mapToScene(touchPoint.rect()));
        touchPoint.setStartScenePos(d->mapToScene(touchPoint.startPos()));
        touchPoint.setLastScenePos(d->mapToScene(touchPoint.lastPos()));

        // screenPos, startScreenPos, lastScreenPos, and screenRect are already set
    }

    touchEvent->setTouchPoints(touchPoints);
}

/*!
    \internal
*/
QGraphicsViewPrivate::QGraphicsViewPrivate()
    : renderHints(QPainter::TextAntialiasing),
      dragMode(QGraphicsView::NoDrag),
      sceneInteractionAllowed(true), hasSceneRect(false),
      connectedToScene(false),
      useLastMouseEvent(false),
      identityMatrix(true),
      dirtyScroll(true),
      accelerateScrolling(true),
      keepLastCenterPoint(true),
      transforming(false),
      handScrolling(false),
      mustAllocateStyleOptions(false),
      mustResizeBackgroundPixmap(true),
      fullUpdatePending(true),
      hasUpdateClip(false),
      mousePressButton(Qt::NoButton),
      leftIndent(0), topIndent(0),
      lastMouseEvent(QEvent::None, QPoint(), Qt::NoButton, 0, 0),
      alignment(Qt::AlignCenter),
      transformationAnchor(QGraphicsView::AnchorViewCenter), resizeAnchor(QGraphicsView::NoAnchor),
      viewportUpdateMode(QGraphicsView::MinimalViewportUpdate),
      optimizationFlags(0),
      scene(0),
#ifndef QT_NO_RUBBERBAND
      rubberBanding(false),
      rubberBandSelectionMode(Qt::IntersectsItemShape),
#endif
      handScrollMotions(0), cacheMode(0),
#ifndef QT_NO_CURSOR
      hasStoredOriginalCursor(false),
#endif
      lastDragDropEvent(0),
      updateSceneSlotReimplementedChecked(false)
{
    styleOptions.reserve(QGRAPHICSVIEW_PREALLOC_STYLE_OPTIONS);
}

/*!
    \internal
*/
void QGraphicsViewPrivate::recalculateContentSize()
{
    Q_Q(QGraphicsView);

    QSize maxSize = q->maximumViewportSize();
    int width = maxSize.width();
    int height = maxSize.height();
    QRectF viewRect = matrix.mapRect(q->sceneRect());

    bool frameOnlyAround = (q->style()->styleHint(QStyle::SH_ScrollView_FrameOnlyAroundContents, 0, q));
    if (frameOnlyAround) {
        if (hbarpolicy == Qt::ScrollBarAlwaysOn)
            height -= frameWidth * 2;
        if (vbarpolicy == Qt::ScrollBarAlwaysOn)
            width -= frameWidth * 2;
    }

    // Adjust the maximum width and height of the viewport based on the width
    // of visible scroll bars.
    int scrollBarExtent = q->style()->pixelMetric(QStyle::PM_ScrollBarExtent, 0, q);
    if (frameOnlyAround)
        scrollBarExtent += frameWidth * 2;

    bool useHorizontalScrollBar = (viewRect.width() > width) && hbarpolicy != Qt::ScrollBarAlwaysOff;
    bool useVerticalScrollBar = (viewRect.height() > height) && vbarpolicy != Qt::ScrollBarAlwaysOff;
    if (useHorizontalScrollBar && !useVerticalScrollBar) {
        if (viewRect.height() > height - scrollBarExtent)
            useVerticalScrollBar = true;
    }
    if (useVerticalScrollBar && !useHorizontalScrollBar) {
        if (viewRect.width() > width - scrollBarExtent)
            useHorizontalScrollBar = true;
    }
    if (useHorizontalScrollBar && hbarpolicy != Qt::ScrollBarAlwaysOn)
        height -= scrollBarExtent;
    if (useVerticalScrollBar && vbarpolicy != Qt::ScrollBarAlwaysOn)
        width -= scrollBarExtent;

    // Setting the ranges of these scroll bars can/will cause the values to
    // change, and scrollContentsBy() will be called correspondingly. This
    // will reset the last center point.
    QPointF savedLastCenterPoint = lastCenterPoint;

    // Remember the former indent settings
    qreal oldLeftIndent = leftIndent;
    qreal oldTopIndent = topIndent;

    // If the whole scene fits horizontally, we center the scene horizontally,
    // and ignore the horizontal scroll bars.
    int left =  q_round_bound(viewRect.left());
    int right = q_round_bound(viewRect.right() - width);
    if (left >= right) {
        hbar->setRange(0, 0);

        switch (alignment & Qt::AlignHorizontal_Mask) {
        case Qt::AlignLeft:
            leftIndent = -viewRect.left();
            break;
        case Qt::AlignRight:
            leftIndent = width - viewRect.width() - viewRect.left() - 1;
            break;
        case Qt::AlignHCenter:
        default:
            leftIndent = width / 2 - (viewRect.left() + viewRect.right()) / 2;
            break;
        }
    } else {
        hbar->setRange(left, right);
        hbar->setPageStep(width);
        hbar->setSingleStep(width / 20);
        leftIndent = 0;
    }

    // If the whole scene fits vertically, we center the scene vertically, and
    // ignore the vertical scroll bars.
    int top = q_round_bound(viewRect.top());
    int bottom = q_round_bound(viewRect.bottom()  - height);
    if (top >= bottom) {
        vbar->setRange(0, 0);

        switch (alignment & Qt::AlignVertical_Mask) {
        case Qt::AlignTop:
            topIndent = -viewRect.top();
            break;
        case Qt::AlignBottom:
            topIndent = height - viewRect.height() - viewRect.top() - 1;
            break;
        case Qt::AlignVCenter:
        default:
            topIndent = height / 2 - (viewRect.top() + viewRect.bottom()) / 2;
            break;
        }
    } else {
        vbar->setRange(top, bottom);
        vbar->setPageStep(height);
        vbar->setSingleStep(height / 20);
        topIndent = 0;
    }

    // Restorethe center point from before the ranges changed.
    lastCenterPoint = savedLastCenterPoint;

    // Issue a full update if the indents change.
    // ### If the transform is still the same, we can get away with just a
    // scroll instead.
    if (oldLeftIndent != leftIndent || oldTopIndent != topIndent) {
        dirtyScroll = true;
        updateAll();
    } else if (q->isRightToLeft() && !leftIndent) {
        // In reverse mode, the horizontal scroll always changes after the content
        // size has changed, as the scroll is calculated by summing the min and
        // max values of the range and subtracting the current value. In normal
        // mode the scroll remains unchanged unless the indent has changed.
        dirtyScroll = true;
    }

    if (cacheMode & QGraphicsView::CacheBackground) {
        // Invalidate the background pixmap
        mustResizeBackgroundPixmap = true;
    }
}

/*!
    \internal
*/
void QGraphicsViewPrivate::centerView(QGraphicsView::ViewportAnchor anchor)
{
    Q_Q(QGraphicsView);
    switch (anchor) {
    case QGraphicsView::AnchorUnderMouse: {
        if (q->underMouse()) {
            // Last scene pos: lastMouseMoveScenePoint
            // Current mouse pos:
            QPointF transformationDiff = q->mapToScene(viewport->rect().center())
                                         - q->mapToScene(viewport->mapFromGlobal(QCursor::pos()));
            q->centerOn(lastMouseMoveScenePoint + transformationDiff);
        } else {
            q->centerOn(lastCenterPoint);
        }
        break;
    }
    case QGraphicsView::AnchorViewCenter:
        q->centerOn(lastCenterPoint);
        break;
    case QGraphicsView::NoAnchor:
        break;
    }
}

/*!
    \internal
*/
void QGraphicsViewPrivate::updateLastCenterPoint()
{
    Q_Q(QGraphicsView);
    lastCenterPoint = q->mapToScene(viewport->rect().center());
}

/*!
    \internal

    Returns the horizontal scroll value (the X value of the left edge of the
    viewport).
*/
qint64 QGraphicsViewPrivate::horizontalScroll() const
{
    if (dirtyScroll)
        const_cast<QGraphicsViewPrivate *>(this)->updateScroll();
    return scrollX;
}

/*!
    \internal

    Returns the vertical scroll value (the X value of the top edge of the
    viewport).
*/
qint64 QGraphicsViewPrivate::verticalScroll() const
{
    if (dirtyScroll)
        const_cast<QGraphicsViewPrivate *>(this)->updateScroll();
    return scrollY;
}

/*!
    \internal

    Maps the given rectangle to the scene using QTransform::mapRect()
*/
QRectF QGraphicsViewPrivate::mapRectToScene(const QRect &rect) const
{
    if (dirtyScroll)
        const_cast<QGraphicsViewPrivate *>(this)->updateScroll();
    QRectF scrolled = QRectF(rect.translated(scrollX, scrollY));
    return identityMatrix ? scrolled : matrix.inverted().mapRect(scrolled);
}


/*!
    \internal

    Maps the given rectangle from the scene using QTransform::mapRect()
*/
QRectF QGraphicsViewPrivate::mapRectFromScene(const QRectF &rect) const
{
    if (dirtyScroll)
        const_cast<QGraphicsViewPrivate *>(this)->updateScroll();
    return (identityMatrix ? rect : matrix.mapRect(rect)).translated(-scrollX, -scrollY);
}

/*!
    \internal
*/
void QGraphicsViewPrivate::updateScroll()
{
    Q_Q(QGraphicsView);
    scrollX = qint64(-leftIndent);
    if (q->isRightToLeft()) {
        if (!leftIndent) {
            scrollX += hbar->minimum();
            scrollX += hbar->maximum();
            scrollX -= hbar->value();
        }
    } else {
        scrollX += hbar->value();
    }

    scrollY = qint64(vbar->value() - topIndent);

    dirtyScroll = false;
}

/*!
    \internal
*/
void QGraphicsViewPrivate::replayLastMouseEvent()
{
    if (!useLastMouseEvent || !scene)
        return;
    mouseMoveEventHandler(&lastMouseEvent);
}

/*!
    \internal
*/
void QGraphicsViewPrivate::storeMouseEvent(QMouseEvent *event)
{
    useLastMouseEvent = true;
    lastMouseEvent = QMouseEvent(QEvent::MouseMove, event->pos(), event->globalPos(),
                                 event->button(), event->buttons(), event->modifiers());
}

void QGraphicsViewPrivate::mouseMoveEventHandler(QMouseEvent *event)
{
    Q_Q(QGraphicsView);

    storeMouseEvent(event);
    lastMouseEvent.setAccepted(false);

    if (!sceneInteractionAllowed)
        return;
    if (handScrolling)
        return;
    if (!scene)
        return;

    QGraphicsSceneMouseEvent mouseEvent(QEvent::GraphicsSceneMouseMove);
    mouseEvent.setWidget(viewport);
    mouseEvent.setButtonDownScenePos(mousePressButton, mousePressScenePoint);
    mouseEvent.setButtonDownScreenPos(mousePressButton, mousePressScreenPoint);
    mouseEvent.setScenePos(q->mapToScene(event->pos()));
    mouseEvent.setScreenPos(event->globalPos());
    mouseEvent.setLastScenePos(lastMouseMoveScenePoint);
    mouseEvent.setLastScreenPos(lastMouseMoveScreenPoint);
    mouseEvent.setButtons(event->buttons());
    mouseEvent.setButton(event->button());
    mouseEvent.setModifiers(event->modifiers());
    lastMouseMoveScenePoint = mouseEvent.scenePos();
    lastMouseMoveScreenPoint = mouseEvent.screenPos();
    mouseEvent.setAccepted(false);
    if (event->spontaneous())
        qt_sendSpontaneousEvent(scene, &mouseEvent);
    else
        QApplication::sendEvent(scene, &mouseEvent);

    // Remember whether the last event was accepted or not.
    lastMouseEvent.setAccepted(mouseEvent.isAccepted());

    if (mouseEvent.isAccepted() && mouseEvent.buttons() != 0) {
        // The event was delivered to a mouse grabber; the press is likely to
        // have set a cursor, and we must not change it.
        return;
    }

#ifndef QT_NO_CURSOR
    // If all the items ignore hover events, we don't look-up any items
    // in QGraphicsScenePrivate::dispatchHoverEvent, hence the
    // cachedItemsUnderMouse list will be empty. We therefore do the look-up
    // for cursor items here if not all items use the default cursor.
    if (scene->d_func()->allItemsIgnoreHoverEvents && !scene->d_func()->allItemsUseDefaultCursor
        && scene->d_func()->cachedItemsUnderMouse.isEmpty()) {
        scene->d_func()->cachedItemsUnderMouse = scene->d_func()->itemsAtPosition(mouseEvent.screenPos(),
                                                                                  mouseEvent.scenePos(),
                                                                                  mouseEvent.widget());
    }
    // Find the topmost item under the mouse with a cursor.
    foreach (QGraphicsItem *item, scene->d_func()->cachedItemsUnderMouse) {
        if (item->hasCursor()) {
            _q_setViewportCursor(item->cursor());
            return;
        }
    }

    // No items with cursors found; revert to the view cursor.
    if (hasStoredOriginalCursor) {
        // Restore the original viewport cursor.
        hasStoredOriginalCursor = false;
        viewport->setCursor(originalCursor);
    }
#endif
}

/*!
    \internal
*/
#ifndef QT_NO_RUBBERBAND
QRegion QGraphicsViewPrivate::rubberBandRegion(const QWidget *widget, const QRect &rect) const
{
    QStyleHintReturnMask mask;
    QStyleOptionRubberBand option;
    option.initFrom(widget);
    option.rect = rect;
    option.opaque = false;
    option.shape = QRubberBand::Rectangle;

    QRegion tmp;
    tmp += rect;
    if (widget->style()->styleHint(QStyle::SH_RubberBand_Mask, &option, widget, &mask))
        tmp &= mask.region;
    return tmp;
}
#endif

/*!
    \internal
*/
#ifndef QT_NO_CURSOR
void QGraphicsViewPrivate::_q_setViewportCursor(const QCursor &cursor)
{
    if (!hasStoredOriginalCursor) {
        hasStoredOriginalCursor = true;
        originalCursor = viewport->cursor();
    }
    viewport->setCursor(cursor);
}
#endif

/*!
    \internal
*/
#ifndef QT_NO_CURSOR
void QGraphicsViewPrivate::_q_unsetViewportCursor()
{
    Q_Q(QGraphicsView);
    foreach (QGraphicsItem *item, q->items(lastMouseEvent.pos())) {
        if (item->hasCursor()) {
            _q_setViewportCursor(item->cursor());
            return;
        }
    }

    // Restore the original viewport cursor.
    if (hasStoredOriginalCursor) {
        hasStoredOriginalCursor = false;
        if (dragMode == QGraphicsView::ScrollHandDrag)
            viewport->setCursor(Qt::OpenHandCursor);
        else
            viewport->setCursor(originalCursor);
    }
}
#endif

/*!
    \internal
*/
void QGraphicsViewPrivate::storeDragDropEvent(const QGraphicsSceneDragDropEvent *event)
{
    delete lastDragDropEvent;
    lastDragDropEvent = new QGraphicsSceneDragDropEvent(event->type());
    lastDragDropEvent->setScenePos(event->scenePos());
    lastDragDropEvent->setScreenPos(event->screenPos());
    lastDragDropEvent->setButtons(event->buttons());
    lastDragDropEvent->setModifiers(event->modifiers());
    lastDragDropEvent->setPossibleActions(event->possibleActions());
    lastDragDropEvent->setProposedAction(event->proposedAction());
    lastDragDropEvent->setDropAction(event->dropAction());
    lastDragDropEvent->setMimeData(event->mimeData());
    lastDragDropEvent->setWidget(event->widget());
    lastDragDropEvent->setSource(event->source());
}

/*!
    \internal
*/
void QGraphicsViewPrivate::populateSceneDragDropEvent(QGraphicsSceneDragDropEvent *dest,
                                                      QDropEvent *source)
{
#ifndef QT_NO_DRAGANDDROP
    Q_Q(QGraphicsView);
    dest->setScenePos(q->mapToScene(source->pos()));
    dest->setScreenPos(q->mapToGlobal(source->pos()));
    dest->setButtons(source->mouseButtons());
    dest->setModifiers(source->keyboardModifiers());
    dest->setPossibleActions(source->possibleActions());
    dest->setProposedAction(source->proposedAction());
    dest->setDropAction(source->dropAction());
    dest->setMimeData(source->mimeData());
    dest->setWidget(viewport);
    dest->setSource(source->source());
#else
    Q_UNUSED(dest)
    Q_UNUSED(source)
#endif
}

/*!
    \internal
*/
QRect QGraphicsViewPrivate::mapToViewRect(const QGraphicsItem *item, const QRectF &rect) const
{
    Q_Q(const QGraphicsView);
    if (dirtyScroll)
        const_cast<QGraphicsViewPrivate *>(this)->updateScroll();

    if (item->d_ptr->itemIsUntransformable()) {
        QTransform itv = item->deviceTransform(q->viewportTransform());
        return itv.mapRect(rect).toAlignedRect();
    }

    // Translate-only
    // COMBINE
    QPointF offset;
    const QGraphicsItem *parentItem = item;
    const QGraphicsItemPrivate *itemd;
    do {
        itemd = parentItem->d_ptr.data();
        if (itemd->transformData)
            break;
        offset += itemd->pos;
    } while ((parentItem = itemd->parent));

    QRectF baseRect = rect.translated(offset.x(), offset.y());
    if (!parentItem) {
        if (identityMatrix) {
            baseRect.translate(-scrollX, -scrollY);
            return baseRect.toAlignedRect();
        }
        return matrix.mapRect(baseRect).translated(-scrollX, -scrollY).toAlignedRect();
    }

    QTransform tr = parentItem->sceneTransform();
    if (!identityMatrix)
        tr *= matrix;
    QRectF r = tr.mapRect(baseRect);
    r.translate(-scrollX, -scrollY);
    return r.toAlignedRect();
}

/*!
    \internal
*/
QRegion QGraphicsViewPrivate::mapToViewRegion(const QGraphicsItem *item, const QRectF &rect) const
{
    Q_Q(const QGraphicsView);
    if (dirtyScroll)
        const_cast<QGraphicsViewPrivate *>(this)->updateScroll();

    // Accurate bounding region
    QTransform itv = item->deviceTransform(q->viewportTransform());
    return item->boundingRegion(itv) & itv.mapRect(rect).toAlignedRect();
}

/*!
    \internal
*/
void QGraphicsViewPrivate::processPendingUpdates()
{
    if (!scene)
        return;

    if (fullUpdatePending) {
        viewport->update();
    } else if (viewportUpdateMode == QGraphicsView::BoundingRectViewportUpdate) {
        viewport->update(dirtyBoundingRect);
    } else {
        viewport->update(dirtyRegion); // Already adjusted in updateRect/Region.
    }

    dirtyBoundingRect = QRect();
    dirtyRegion = QRegion();
}

static inline bool intersectsViewport(const QRect &r, int width, int height)
{ return !(r.left() > width) && !(r.right() < 0) && !(r.top() >= height) && !(r.bottom() < 0); }

static inline bool containsViewport(const QRect &r, int width, int height)
{ return r.left() <= 0 && r.top() <= 0 && r.right() >= width - 1 && r.bottom() >= height - 1; }

static inline void QRect_unite(QRect *rect, const QRect &other)
{
    if (rect->isEmpty()) {
        *rect = other;
    } else {
        rect->setCoords(qMin(rect->left(), other.left()), qMin(rect->top(), other.top()),
                        qMax(rect->right(), other.right()), qMax(rect->bottom(), other.bottom()));
    }
}

/*
   Calling this function results in update rects being clipped to the item's
   bounding rect. Note that updates prior to this function call is not clipped.
   The clip is removed by passing 0.
*/
void QGraphicsViewPrivate::setUpdateClip(QGraphicsItem *item)
{
    Q_Q(QGraphicsView);
    // We simply ignore the request if the update mode is either FullViewportUpdate
    // or NoViewportUpdate; in that case there's no point in clipping anything.
    if (!item || viewportUpdateMode == QGraphicsView::NoViewportUpdate
        || viewportUpdateMode == QGraphicsView::FullViewportUpdate) {
        hasUpdateClip = false;
        return;
    }

    // Calculate the clip (item's bounding rect in view coordinates).
    // Optimized version of:
    // QRect clip = item->deviceTransform(q->viewportTransform())
    //              .mapRect(item->boundingRect()).toAlignedRect();
    QRect clip;
    if (item->d_ptr->itemIsUntransformable()) {
        QTransform xform = item->deviceTransform(q->viewportTransform());
        clip = xform.mapRect(item->boundingRect()).toAlignedRect();
    } else if (item->d_ptr->sceneTransformTranslateOnly && identityMatrix) {
        QRectF r(item->boundingRect());
        r.translate(item->d_ptr->sceneTransform.dx() - horizontalScroll(),
                    item->d_ptr->sceneTransform.dy() - verticalScroll());
        clip = r.toAlignedRect();
    } else if (!q->isTransformed()) {
        clip = item->d_ptr->sceneTransform.mapRect(item->boundingRect()).toAlignedRect();
    } else {
        QTransform xform = item->d_ptr->sceneTransform;
        xform *= q->viewportTransform();
        clip = xform.mapRect(item->boundingRect()).toAlignedRect();
    }

    if (hasUpdateClip) {
        // Intersect with old clip.
        updateClip &= clip;
    } else {
        updateClip = clip;
        hasUpdateClip = true;
    }
}

bool QGraphicsViewPrivate::updateRegion(const QRectF &rect, const QTransform &xform)
{
    if (rect.isEmpty())
        return false;

    if (viewportUpdateMode != QGraphicsView::MinimalViewportUpdate
        && viewportUpdateMode != QGraphicsView::SmartViewportUpdate) {
        // No point in updating with QRegion granularity; use the rect instead.
        return updateRectF(xform.mapRect(rect));
    }

    // Update mode is either Minimal or Smart, so we have to do a potentially slow operation,
    // which is clearly documented here: QGraphicsItem::setBoundingRegionGranularity.
    const QRegion region = xform.map(QRegion(rect.toAlignedRect()));
    QRect viewRect = region.boundingRect();
    const bool dontAdjustForAntialiasing = optimizationFlags & QGraphicsView::DontAdjustForAntialiasing;
    if (dontAdjustForAntialiasing)
        viewRect.adjust(-1, -1, 1, 1);
    else
        viewRect.adjust(-2, -2, 2, 2);
    if (!intersectsViewport(viewRect, viewport->width(), viewport->height()))
        return false; // Update region for sure outside viewport.

    const QVector<QRect> &rects = region.rects();
    for (int i = 0; i < rects.size(); ++i) {
        viewRect = rects.at(i);
        if (dontAdjustForAntialiasing)
            viewRect.adjust(-1, -1, 1, 1);
        else
            viewRect.adjust(-2, -2, 2, 2);
        if (hasUpdateClip)
            viewRect &= updateClip;
        dirtyRegion += viewRect;
    }

    return true;
}

// NB! Assumes the rect 'r' is already aligned and adjusted for antialiasing.
// For QRectF use updateRectF(const QRectF &) to ensure proper adjustments.
bool QGraphicsViewPrivate::updateRect(const QRect &r)
{
    if (fullUpdatePending || viewportUpdateMode == QGraphicsView::NoViewportUpdate
        || !intersectsViewport(r, viewport->width(), viewport->height())) {
        return false;
    }

    switch (viewportUpdateMode) {
    case QGraphicsView::FullViewportUpdate:
        fullUpdatePending = true;
        viewport->update();
        break;
    case QGraphicsView::BoundingRectViewportUpdate:
        if (hasUpdateClip)
            QRect_unite(&dirtyBoundingRect, r & updateClip);
        else
            QRect_unite(&dirtyBoundingRect, r);
        if (containsViewport(dirtyBoundingRect, viewport->width(), viewport->height())) {
            fullUpdatePending = true;
            viewport->update();
        }
        break;
    case QGraphicsView::SmartViewportUpdate: // ### DEPRECATE
    case QGraphicsView::MinimalViewportUpdate:
        if (hasUpdateClip)
            dirtyRegion += r & updateClip;
        else
            dirtyRegion += r;
        break;
    default:
        break;
    }

    return true;
}

QStyleOptionGraphicsItem *QGraphicsViewPrivate::allocStyleOptionsArray(int numItems)
{
    if (mustAllocateStyleOptions || (numItems > styleOptions.capacity()))
        // too many items, let's allocate on-the-fly
        return new QStyleOptionGraphicsItem[numItems];

    // expand only whenever necessary
    if (numItems > styleOptions.size())
        styleOptions.resize(numItems);

    mustAllocateStyleOptions = true;
    return styleOptions.data();
}

void QGraphicsViewPrivate::freeStyleOptionsArray(QStyleOptionGraphicsItem *array)
{
    mustAllocateStyleOptions = false;
    if (array != styleOptions.data())
        delete [] array;
}

extern Q_AUTOTEST_EXPORT QPainterPath qt_regionToPath(const QRegion &region);

/*!
    ### Adjustments in findItems: mapToScene(QRect) forces us to adjust the
    input rectangle by (0, 0, 1, 1), because it uses QRect::bottomRight()
    (etc) when mapping the rectangle to a polygon (which is _wrong_). In
    addition, as QGraphicsItem::boundingRect() is defined in logical space,
    but the default pen for QPainter is cosmetic with a width of 0, QPainter
    is at risk of painting 1 pixel outside the bounding rect. Therefore we
    must search for items with an adjustment of (-1, -1, 1, 1).
*/
QList<QGraphicsItem *> QGraphicsViewPrivate::findItems(const QRegion &exposedRegion, bool *allItems,
                                                       const QTransform &viewTransform) const
{
    Q_Q(const QGraphicsView);

    // Step 1) If all items are contained within the expose region, then
    // return a list of all visible items. ### the scene's growing bounding
    // rect does not take into account untransformable items.
    const QRectF exposedRegionSceneBounds = q->mapToScene(exposedRegion.boundingRect().adjusted(-1, -1, 1, 1))
                                            .boundingRect();
    if (exposedRegionSceneBounds.contains(scene->sceneRect())) {
        Q_ASSERT(allItems);
        *allItems = true;

        // All items are guaranteed within the exposed region.
        return scene->items(Qt::AscendingOrder);
    }

    // Step 2) If the expose region is a simple rect and the view is only
    // translated or scaled, search for items using
    // QGraphicsScene::items(QRectF).
    bool simpleRectLookup =  exposedRegion.rectCount() == 1 && matrix.type() <= QTransform::TxScale;
    if (simpleRectLookup) {
        return scene->items(exposedRegionSceneBounds,
                            Qt::IntersectsItemBoundingRect,
                            Qt::AscendingOrder, viewTransform);
    }

    // If the region is complex or the view has a complex transform, adjust
    // the expose region, convert it to a path, and then search for items
    // using QGraphicsScene::items(QPainterPath);
    QRegion adjustedRegion;
    foreach (const QRect &r, exposedRegion.rects())
        adjustedRegion += r.adjusted(-1, -1, 1, 1);

    const QPainterPath exposedScenePath(q->mapToScene(qt_regionToPath(adjustedRegion)));
    return scene->items(exposedScenePath, Qt::IntersectsItemBoundingRect,
                        Qt::AscendingOrder, viewTransform);
}

/*!
    \internal

    Enables input methods for the view if and only if the current focus item of
    the scene accepts input methods. Call function whenever that condition has
    potentially changed.
*/
void QGraphicsViewPrivate::updateInputMethodSensitivity()
{
    Q_Q(QGraphicsView);
    QGraphicsItem *focusItem = 0;
    bool enabled = scene && (focusItem = scene->focusItem())
                   && (focusItem->d_ptr->flags & QGraphicsItem::ItemAcceptsInputMethod);
    q->setAttribute(Qt::WA_InputMethodEnabled, enabled);
    q->viewport()->setAttribute(Qt::WA_InputMethodEnabled, enabled);

    if (!enabled) {
        q->setInputMethodHints(0);
        return;
    }

    QGraphicsProxyWidget *proxy = focusItem->d_ptr->isWidget && focusItem->d_ptr->isProxyWidget()
                                    ? static_cast<QGraphicsProxyWidget *>(focusItem) : 0;
    if (!proxy) {
        q->setInputMethodHints(focusItem->inputMethodHints());
    } else if (QWidget *widget = proxy->widget()) {
    if (QWidget *fw = widget->focusWidget())
        widget = fw;
        q->setInputMethodHints(widget->inputMethodHints());
    } else {
        q->setInputMethodHints(0);
    }
}

/*!
    Constructs a QGraphicsView. \a parent is passed to QWidget's constructor.
*/
QGraphicsView::QGraphicsView(QWidget *parent)
    : QAbstractScrollArea(*new QGraphicsViewPrivate, parent)
{
    setViewport(0);
    setAcceptDrops(true);
    setBackgroundRole(QPalette::Base);
    // Investigate leaving these disabled by default.
    setAttribute(Qt::WA_InputMethodEnabled);
    viewport()->setAttribute(Qt::WA_InputMethodEnabled);
}

/*!
    Constructs a QGraphicsView and sets the visualized scene to \a
    scene. \a parent is passed to QWidget's constructor.
*/
QGraphicsView::QGraphicsView(QGraphicsScene *scene, QWidget *parent)
    : QAbstractScrollArea(*new QGraphicsViewPrivate, parent)
{
    setScene(scene);
    setViewport(0);
    setAcceptDrops(true);
    setBackgroundRole(QPalette::Base);
    // Investigate leaving these disabled by default.
    setAttribute(Qt::WA_InputMethodEnabled);
    viewport()->setAttribute(Qt::WA_InputMethodEnabled);
}

/*!
  \internal
 */
QGraphicsView::QGraphicsView(QGraphicsViewPrivate &dd, QWidget *parent)
  : QAbstractScrollArea(dd, parent)
{
    setViewport(0);
    setAcceptDrops(true);
    setBackgroundRole(QPalette::Base);
    // Investigate leaving these disabled by default.
    setAttribute(Qt::WA_InputMethodEnabled);
    viewport()->setAttribute(Qt::WA_InputMethodEnabled);
}

/*!
    Destructs the QGraphicsView object.
*/
QGraphicsView::~QGraphicsView()
{
    Q_D(QGraphicsView);
    if (d->scene)
        d->scene->d_func()->views.removeAll(this);
    delete d->lastDragDropEvent;
}

/*!
    \reimp
*/
QSize QGraphicsView::sizeHint() const
{
    Q_D(const QGraphicsView);
    if (d->scene) {
        QSizeF baseSize = d->matrix.mapRect(sceneRect()).size();
        baseSize += QSizeF(d->frameWidth * 2, d->frameWidth * 2);
        return baseSize.boundedTo((3 * QApplication::desktop()->size()) / 4).toSize();
    }
    return QAbstractScrollArea::sizeHint();
}

/*!
    \property QGraphicsView::renderHints
    \brief the default render hints for the view

    These hints are
    used to initialize QPainter before each visible item is drawn. QPainter
    uses render hints to toggle rendering features such as antialiasing and
    smooth pixmap transformation.

    QPainter::TextAntialiasing is enabled by default.

    Example:

    \snippet doc/src/snippets/code/src_gui_graphicsview_qgraphicsview.cpp 1
*/
QPainter::RenderHints QGraphicsView::renderHints() const
{
    Q_D(const QGraphicsView);
    return d->renderHints;
}
void QGraphicsView::setRenderHints(QPainter::RenderHints hints)
{
    Q_D(QGraphicsView);
    if (hints == d->renderHints)
        return;
    d->renderHints = hints;
    d->updateAll();
}

/*!
    If \a enabled is true, the render hint \a hint is enabled; otherwise it
    is disabled.

    \sa renderHints
*/
void QGraphicsView::setRenderHint(QPainter::RenderHint hint, bool enabled)
{
    Q_D(QGraphicsView);
    QPainter::RenderHints oldHints = d->renderHints;
    if (enabled)
        d->renderHints |= hint;
    else
        d->renderHints &= ~hint;
    if (oldHints != d->renderHints)
        d->updateAll();
}

/*!
    \property QGraphicsView::alignment
    \brief the alignment of the scene in the view when the whole
    scene is visible.

    If the whole scene is visible in the view, (i.e., there are no visible
    scroll bars,) the view's alignment will decide where the scene will be
    rendered in the view. For example, if the alignment is Qt::AlignCenter,
    which is default, the scene will be centered in the view, and if the
    alignment is (Qt::AlignLeft | Qt::AlignTop), the scene will be rendered in
    the top-left corner of the view.
*/
Qt::Alignment QGraphicsView::alignment() const
{
    Q_D(const QGraphicsView);
    return d->alignment;
}
void QGraphicsView::setAlignment(Qt::Alignment alignment)
{
    Q_D(QGraphicsView);
    if (d->alignment != alignment) {
        d->alignment = alignment;
        d->recalculateContentSize();
    }
}

/*!
    \property QGraphicsView::transformationAnchor
    \brief how the view should position the scene during transformations.

    QGraphicsView uses this property to decide how to position the scene in
    the viewport when the transformation matrix changes, and the coordinate
    system of the view is transformed. The default behavior, AnchorViewCenter,
    ensures that the scene point at the center of the view remains unchanged
    during transformations (e.g., when rotating, the scene will appear to
    rotate around the center of the view).

    Note that the effect of this property is noticeable when only a part of the
    scene is visible (i.e., when there are scroll bars). Otherwise, if the
    whole scene fits in the view, QGraphicsScene uses the view \l alignment to
    position the scene in the view.

    \sa alignment, resizeAnchor
*/
QGraphicsView::ViewportAnchor QGraphicsView::transformationAnchor() const
{
    Q_D(const QGraphicsView);
    return d->transformationAnchor;
}
void QGraphicsView::setTransformationAnchor(ViewportAnchor anchor)
{
    Q_D(QGraphicsView);
    d->transformationAnchor = anchor;

    // Ensure mouse tracking is enabled in the case we are using AnchorUnderMouse
    // in order to have up-to-date information for centering the view.
    if (d->transformationAnchor == AnchorUnderMouse)
        d->viewport->setMouseTracking(true);
}

/*!
    \property QGraphicsView::resizeAnchor
    \brief how the view should position the scene when the view is resized.

    QGraphicsView uses this property to decide how to position the scene in
    the viewport when the viewport widget's size changes. The default
    behavior, NoAnchor, leaves the scene's position unchanged during a resize;
    the top-left corner of the view will appear to be anchored while resizing.

    Note that the effect of this property is noticeable when only a part of the
    scene is visible (i.e., when there are scroll bars). Otherwise, if the
    whole scene fits in the view, QGraphicsScene uses the view \l alignment to
    position the scene in the view.

    \sa alignment, transformationAnchor, Qt::WNorthWestGravity
*/
QGraphicsView::ViewportAnchor QGraphicsView::resizeAnchor() const
{
    Q_D(const QGraphicsView);
    return d->resizeAnchor;
}
void QGraphicsView::setResizeAnchor(ViewportAnchor anchor)
{
    Q_D(QGraphicsView);
    d->resizeAnchor = anchor;

    // Ensure mouse tracking is enabled in the case we are using AnchorUnderMouse
    // in order to have up-to-date information for centering the view.
    if (d->resizeAnchor == AnchorUnderMouse)
        d->viewport->setMouseTracking(true);
}

/*!
    \property QGraphicsView::viewportUpdateMode
    \brief how the viewport should update its contents.

    \since 4.3

    QGraphicsView uses this property to decide how to update areas of the
    scene that have been reexposed or changed. Usually you do not need to
    modify this property, but there are some cases where doing so can improve
    rendering performance. See the ViewportUpdateMode documentation for
    specific details.

    The default value is MinimalViewportUpdate, where QGraphicsView will
    update as small an area of the viewport as possible when the contents
    change.

    \sa ViewportUpdateMode, cacheMode
*/
QGraphicsView::ViewportUpdateMode QGraphicsView::viewportUpdateMode() const
{
    Q_D(const QGraphicsView);
    return d->viewportUpdateMode;
}
void QGraphicsView::setViewportUpdateMode(ViewportUpdateMode mode)
{
    Q_D(QGraphicsView);
    d->viewportUpdateMode = mode;
}

/*!
    \property QGraphicsView::optimizationFlags
    \brief flags that can be used to tune QGraphicsView's performance.

    \since 4.3

    QGraphicsView uses clipping, extra bounding rect adjustments, and certain
    other aids to improve rendering quality and performance for the common
    case graphics scene. However, depending on the target platform, the scene,
    and the viewport in use, some of these operations can degrade performance.

    The effect varies from flag to flag; see the OptimizationFlags
    documentation for details.

    By default, no optimization flags are enabled.

    \sa setOptimizationFlag()
*/
QGraphicsView::OptimizationFlags QGraphicsView::optimizationFlags() const
{
    Q_D(const QGraphicsView);
    return d->optimizationFlags;
}
void QGraphicsView::setOptimizationFlags(OptimizationFlags flags)
{
    Q_D(QGraphicsView);
    d->optimizationFlags = flags;
}

/*!
    Enables \a flag if \a enabled is true; otherwise disables \a flag.

    \sa optimizationFlags
*/
void QGraphicsView::setOptimizationFlag(OptimizationFlag flag, bool enabled)
{
    Q_D(QGraphicsView);
    if (enabled)
        d->optimizationFlags |= flag;
    else
        d->optimizationFlags &= ~flag;
}

/*!
    \property QGraphicsView::dragMode
    \brief the behavior for dragging the mouse over the scene while
    the left mouse button is pressed.

    This property defines what should happen when the user clicks on the scene
    background and drags the mouse (e.g., scrolling the viewport contents
    using a pointing hand cursor, or selecting multiple items with a rubber
    band). The default value, NoDrag, does nothing.

    This behavior only affects mouse clicks that are not handled by any item.
    You can define a custom behavior by creating a subclass of QGraphicsView
    and reimplementing mouseMoveEvent().
*/
QGraphicsView::DragMode QGraphicsView::dragMode() const
{
    Q_D(const QGraphicsView);
    return d->dragMode;
}
void QGraphicsView::setDragMode(DragMode mode)
{
    Q_D(QGraphicsView);
    if (d->dragMode == mode)
        return;

#ifndef QT_NO_CURSOR
    if (d->dragMode == ScrollHandDrag)
        viewport()->unsetCursor();
#endif

    // If dragMode is unset while dragging, e.g. via a keyEvent, we
    // don't unset the handScrolling state. When enabling scrolling
    // again the mouseMoveEvent will automatically start scrolling,
    // without a mousePress
    if (d->dragMode == ScrollHandDrag && mode == NoDrag && d->handScrolling)
        d->handScrolling = false;

    d->dragMode = mode;

#ifndef QT_NO_CURSOR
    if (d->dragMode == ScrollHandDrag) {
        // Forget the stored viewport cursor when we enter scroll hand drag mode.
        d->hasStoredOriginalCursor = false;
        viewport()->setCursor(Qt::OpenHandCursor);
    }
#endif
}

#ifndef QT_NO_RUBBERBAND
/*!
    \property QGraphicsView::rubberBandSelectionMode
    \brief the behavior for selecting items with a rubber band selection rectangle.
    \since 4.3

    This property defines how items are selected when using the RubberBandDrag
    drag mode.

    The default value is Qt::IntersectsItemShape; all items whose shape
    intersects with or is contained by the rubber band are selected.

    \sa dragMode, items()
*/
Qt::ItemSelectionMode QGraphicsView::rubberBandSelectionMode() const
{
    Q_D(const QGraphicsView);
    return d->rubberBandSelectionMode;
}
void QGraphicsView::setRubberBandSelectionMode(Qt::ItemSelectionMode mode)
{
    Q_D(QGraphicsView);
    d->rubberBandSelectionMode = mode;
}
#endif

/*!
    \property QGraphicsView::cacheMode
    \brief which parts of the view are cached

    QGraphicsView can cache pre-rendered content in a QPixmap, which is then
    drawn onto the viewport. The purpose of such caching is to speed up the
    total rendering time for areas that are slow to render.  Texture, gradient
    and alpha blended backgrounds, for example, can be notibly slow to render;
    especially with a transformed view. The CacheBackground flag enables
    caching of the view's background. For example:

    \snippet doc/src/snippets/code/src_gui_graphicsview_qgraphicsview.cpp 2

    The cache is invalidated every time the view is transformed. However, when
    scrolling, only partial invalidation is required.

    By default, nothing is cached.

    \sa resetCachedContent(), QPixmapCache
*/
QGraphicsView::CacheMode QGraphicsView::cacheMode() const
{
    Q_D(const QGraphicsView);
    return d->cacheMode;
}
void QGraphicsView::setCacheMode(CacheMode mode)
{
    Q_D(QGraphicsView);
    if (mode == d->cacheMode)
        return;
    d->cacheMode = mode;
    resetCachedContent();
}

/*!
    Resets any cached content. Calling this function will clear
    QGraphicsView's cache. If the current cache mode is \l CacheNone, this
    function does nothing.

    This function is called automatically for you when the backgroundBrush or
    QGraphicsScene::backgroundBrush properties change; you only need to call
    this function if you have reimplemented QGraphicsScene::drawBackground()
    or QGraphicsView::drawBackground() to draw a custom background, and need
    to trigger a full redraw.

    \sa cacheMode()
*/
void QGraphicsView::resetCachedContent()
{
    Q_D(QGraphicsView);
    if (d->cacheMode == CacheNone)
        return;

    if (d->cacheMode & CacheBackground) {
        // Background caching is enabled.
        d->mustResizeBackgroundPixmap = true;
        d->updateAll();
    } else if (d->mustResizeBackgroundPixmap) {
        // Background caching is disabled.
        // Cleanup, free some resources.
        d->mustResizeBackgroundPixmap = false;
        d->backgroundPixmap = QPixmap();
        d->backgroundPixmapExposed = QRegion();
    }
}

/*!
    Invalidates and schedules a redraw of \a layers inside \a rect. \a rect is
    in scene coordinates. Any cached content for \a layers inside \a rect is
    unconditionally invalidated and redrawn.

    You can call this function to notify QGraphicsView of changes to the
    background or the foreground of the scene. It is commonly used for scenes
    with tile-based backgrounds to notify changes when QGraphicsView has
    enabled background caching.

    Note that QGraphicsView currently supports background caching only (see
    QGraphicsView::CacheBackground). This function is equivalent to calling update() if any
    layer but QGraphicsScene::BackgroundLayer is passed.

    \sa QGraphicsScene::invalidate(), update()
*/
void QGraphicsView::invalidateScene(const QRectF &rect, QGraphicsScene::SceneLayers layers)
{
    Q_D(QGraphicsView);
    if ((layers & QGraphicsScene::BackgroundLayer) && !d->mustResizeBackgroundPixmap) {
        QRect viewRect = mapFromScene(rect).boundingRect();
        if (viewport()->rect().intersects(viewRect)) {
            // The updated background area is exposed; schedule this area for
            // redrawing.
            d->backgroundPixmapExposed += viewRect;
            if (d->scene)
                d->scene->update(rect);
        }
    }
}

/*!
    \property QGraphicsView::interactive
    \brief whether the view allowed scene interaction.

    If enabled, this view is set to allow scene interaction. Otherwise, this
    view will not allow interaction, and any mouse or key events are ignored
    (i.e., it will act as a read-only view).

    By default, this property is true.
*/
bool QGraphicsView::isInteractive() const
{
    Q_D(const QGraphicsView);
    return d->sceneInteractionAllowed;
}
void QGraphicsView::setInteractive(bool allowed)
{
    Q_D(QGraphicsView);
    d->sceneInteractionAllowed = allowed;
}

/*!
    Returns a pointer to the scene that is currently visualized in the
    view. If no scene is currently visualized, 0 is returned.

    \sa setScene()
*/
QGraphicsScene *QGraphicsView::scene() const
{
    Q_D(const QGraphicsView);
    return d->scene;
}

/*!
    Sets the current scene to \a scene. If \a scene is already being
    viewed, this function does nothing.

    When a scene is set on a view, the QGraphicsScene::changed() signal
    is automatically connected to this view's updateScene() slot, and the
    view's scroll bars are adjusted to fit the size of the scene.
*/
void QGraphicsView::setScene(QGraphicsScene *scene)
{
    Q_D(QGraphicsView);
    if (d->scene == scene)
        return;

    // Always update the viewport when the scene changes.
    d->updateAll();

    // Remove the previously assigned scene.
    if (d->scene) {
        disconnect(d->scene, SIGNAL(changed(QList<QRectF>)),
                   this, SLOT(updateScene(QList<QRectF>)));
        disconnect(d->scene, SIGNAL(sceneRectChanged(QRectF)),
                   this, SLOT(updateSceneRect(QRectF)));
        d->scene->d_func()->removeView(this);
        d->connectedToScene = false;

        if (isActiveWindow() && isVisible()) {
            QEvent windowDeactivate(QEvent::WindowDeactivate);
            QApplication::sendEvent(d->scene, &windowDeactivate);
        }
        if(hasFocus())
            d->scene->clearFocus();
    }

    // Assign the new scene and update the contents (scrollbars, etc.)).
    if ((d->scene = scene)) {
        connect(d->scene, SIGNAL(sceneRectChanged(QRectF)),
                this, SLOT(updateSceneRect(QRectF)));
        d->updateSceneSlotReimplementedChecked = false;
        d->scene->d_func()->addView(this);
        d->recalculateContentSize();
        d->lastCenterPoint = sceneRect().center();
        d->keepLastCenterPoint = true;
        // We are only interested in mouse tracking if items accept
        // hover events or use non-default cursors.
        if (!d->scene->d_func()->allItemsIgnoreHoverEvents
            || !d->scene->d_func()->allItemsUseDefaultCursor) {
            d->viewport->setMouseTracking(true);
        }

        // enable touch events if any items is interested in them
        if (!d->scene->d_func()->allItemsIgnoreTouchEvents)
            d->viewport->setAttribute(Qt::WA_AcceptTouchEvents);

        if (isActiveWindow() && isVisible()) {
            QEvent windowActivate(QEvent::WindowActivate);
            QApplication::sendEvent(d->scene, &windowActivate);
        }
    } else {
        d->recalculateContentSize();
    }

    d->updateInputMethodSensitivity();

    if (d->scene && hasFocus())
        d->scene->setFocus();
}

/*!
    \property QGraphicsView::sceneRect
    \brief the area of the scene visualized by this view.

    The scene rectangle defines the extent of the scene, and in the view's case,
    this means the area of the scene that you can navigate using the scroll
    bars.

    If unset, or if a null QRectF is set, this property has the same value as
    QGraphicsScene::sceneRect, and it changes with
    QGraphicsScene::sceneRect. Otherwise, the view's scene rect is unaffected
    by the scene.

    Note that, although the scene supports a virtually unlimited size, the
    range of the scroll bars will never exceed the range of an integer
    (INT_MIN, INT_MAX). When the scene is larger than the scroll bars' values,
    you can choose to use translate() to navigate the scene instead.

    By default, this property contains a rectangle at the origin with zero
    width and height.

    \sa QGraphicsScene::sceneRect
*/
QRectF QGraphicsView::sceneRect() const
{
    Q_D(const QGraphicsView);
    if (d->hasSceneRect)
        return d->sceneRect;
    if (d->scene)
        return d->scene->sceneRect();
    return QRectF();
}
void QGraphicsView::setSceneRect(const QRectF &rect)
{
    Q_D(QGraphicsView);
    d->hasSceneRect = !rect.isNull();
    d->sceneRect = rect;
    d->recalculateContentSize();
}

/*!
    Returns the current transformation matrix for the view. If no current
    transformation is set, the identity matrix is returned.

    \sa setMatrix(), transform(), rotate(), scale(), shear(), translate()
*/
QMatrix QGraphicsView::matrix() const
{
    Q_D(const QGraphicsView);
    return d->matrix.toAffine();
}

/*!
    Sets the view's current transformation matrix to \a matrix.

    If \a combine is true, then \a matrix is combined with the current matrix;
    otherwise, \a matrix \e replaces the current matrix. \a combine is false
    by default.

    The transformation matrix tranforms the scene into view coordinates. Using
    the default transformation, provided by the identity matrix, one pixel in
    the view represents one unit in the scene (e.g., a 10x10 rectangular item
    is drawn using 10x10 pixels in the view). If a 2x2 scaling matrix is
    applied, the scene will be drawn in 1:2 (e.g., a 10x10 rectangular item is
    then drawn using 20x20 pixels in the view).

    Example:

    \snippet doc/src/snippets/code/src_gui_graphicsview_qgraphicsview.cpp 3

    To simplify interation with items using a transformed view, QGraphicsView
    provides mapTo... and mapFrom... functions that can translate between
    scene and view coordinates. For example, you can call mapToScene() to map
    a view coordinate to a floating point scene coordinate, or mapFromScene()
    to map from floating point scene coordinates to view coordinates.

    \sa matrix(), setTransform(), rotate(), scale(), shear(), translate()
*/
void QGraphicsView::setMatrix(const QMatrix &matrix, bool combine)
{
    setTransform(QTransform(matrix), combine);
}

/*!
    Resets the view transformation matrix to the identity matrix.

    \sa resetTransform()
*/
void QGraphicsView::resetMatrix()
{
    resetTransform();
}

/*!
    Rotates the current view transformation \a angle degrees clockwise.

    \sa setTransform(), transform(), scale(), shear(), translate()
*/
void QGraphicsView::rotate(qreal angle)
{
    Q_D(QGraphicsView);
    QTransform matrix = d->matrix;
    matrix.rotate(angle);
    setTransform(matrix);
}

/*!
    Scales the current view transformation by (\a sx, \a sy).

    \sa setTransform(), transform(), rotate(), shear(), translate()
*/
void QGraphicsView::scale(qreal sx, qreal sy)
{
    Q_D(QGraphicsView);
    QTransform matrix = d->matrix;
    matrix.scale(sx, sy);
    setTransform(matrix);
}

/*!
    Shears the current view transformation by (\a sh, \a sv).

    \sa setTransform(), transform(), rotate(), scale(), translate()
*/
void QGraphicsView::shear(qreal sh, qreal sv)
{
    Q_D(QGraphicsView);
    QTransform matrix = d->matrix;
    matrix.shear(sh, sv);
    setTransform(matrix);
}

/*!
    Translates the current view transformation by (\a dx, \a dy).

    \sa setTransform(), transform(), rotate(), shear()
*/
void QGraphicsView::translate(qreal dx, qreal dy)
{
    Q_D(QGraphicsView);
    QTransform matrix = d->matrix;
    matrix.translate(dx, dy);
    setTransform(matrix);
}

/*!
    Scrolls the contents of the viewport to ensure that the scene
    coordinate \a pos, is centered in the view.

    Because \a pos is a floating point coordinate, and the scroll bars operate
    on integer coordinates, the centering is only an approximation.

    \note If the item is close to or outside the border, it will be visible
    in the view, but not centered.

    \sa ensureVisible()
*/
void QGraphicsView::centerOn(const QPointF &pos)
{
    Q_D(QGraphicsView);
    qreal width = viewport()->width();
    qreal height = viewport()->height();
    QPointF viewPoint = d->matrix.map(pos);
    QPointF oldCenterPoint = pos;

    if (!d->leftIndent) {
        if (isRightToLeft()) {
            qint64 horizontal = 0;
            horizontal += horizontalScrollBar()->minimum();
            horizontal += horizontalScrollBar()->maximum();
            horizontal -= int(viewPoint.x() - width / qreal(2.0));
            horizontalScrollBar()->setValue(horizontal);
        } else {
            horizontalScrollBar()->setValue(int(viewPoint.x() - width / qreal(2.0)));
        }
    }
    if (!d->topIndent)
        verticalScrollBar()->setValue(int(viewPoint.y() - height / qreal(2.0)));
    d->lastCenterPoint = oldCenterPoint;
}

/*!
    \fn QGraphicsView::centerOn(qreal x, qreal y)
    \overload

    This function is provided for convenience. It's equivalent to calling
    centerOn(QPointF(\a x, \a y)).
*/

/*!
    \overload

    Scrolls the contents of the viewport to ensure that \a item
    is centered in the view.

    \sa ensureVisible()
*/
void QGraphicsView::centerOn(const QGraphicsItem *item)
{
    centerOn(item->sceneBoundingRect().center());
}

/*!
    Scrolls the contents of the viewport so that the scene rectangle \a rect
    is visible, with margins specified in pixels by \a xmargin and \a
    ymargin. If the specified rect cannot be reached, the contents are
    scrolled to the nearest valid position. The default value for both margins
    is 50 pixels.

    \sa centerOn()
*/
void QGraphicsView::ensureVisible(const QRectF &rect, int xmargin, int ymargin)
{
    Q_D(QGraphicsView);
    qreal width = viewport()->width();
    qreal height = viewport()->height();
    QRectF viewRect = d->matrix.mapRect(rect);

    qreal left = d->horizontalScroll();
    qreal right = left + width;
    qreal top = d->verticalScroll();
    qreal bottom = top + height;

    if (viewRect.left() <= left + xmargin) {
        // need to scroll from the left
        if (!d->leftIndent)
            horizontalScrollBar()->setValue(int(viewRect.left() - xmargin - qreal(0.5)));
    }
    if (viewRect.right() >= right - xmargin) {
        // need to scroll from the right
        if (!d->leftIndent)
            horizontalScrollBar()->setValue(int(viewRect.right() - width + xmargin + qreal(0.5)));
    }
    if (viewRect.top() <= top + ymargin) {
        // need to scroll from the top
        if (!d->topIndent)
            verticalScrollBar()->setValue(int(viewRect.top() - ymargin - qreal(0.5)));
    }
    if (viewRect.bottom() >= bottom - ymargin) {
        // need to scroll from the bottom
        if (!d->topIndent)
            verticalScrollBar()->setValue(int(viewRect.bottom() - height + ymargin + qreal(0.5)));
    }
}

/*!
    \fn QGraphicsView::ensureVisible(qreal x, qreal y, qreal w, qreal h,
    int xmargin, int ymargin)
    \overload

    This function is provided for convenience. It's equivalent to calling
    ensureVisible(QRectF(\a x, \a y, \a w, \a h), \a xmargin, \a ymargin).
*/

/*!
    \overload

    Scrolls the contents of the viewport so that the center of item \a item is
    visible, with margins specified in pixels by \a xmargin and \a ymargin. If
    the specified point cannot be reached, the contents are scrolled to the
    nearest valid position. The default value for both margins is 50 pixels.

    \sa centerOn()
*/
void QGraphicsView::ensureVisible(const QGraphicsItem *item, int xmargin, int ymargin)
{
    ensureVisible(item->sceneBoundingRect(), xmargin, ymargin);
}

/*!
    Scales the view matrix and scrolls the scroll bars to ensure that the
    scene rectangle \a rect fits inside the viewport. \a rect must be inside
    the scene rect; otherwise, fitInView() cannot guarantee that the whole
    rect is visible.

    This function keeps the view's rotation, translation, or shear. The view
    is scaled according to \a aspectRatioMode. \a rect will be centered in the
    view if it does not fit tightly.

    It's common to call fitInView() from inside a reimplementation of
    resizeEvent(), to ensure that the whole scene, or parts of the scene,
    scales automatically to fit the new size of the viewport as the view is
    resized. Note though, that calling fitInView() from inside resizeEvent()
    can lead to unwanted resize recursion, if the new transformation toggles
    the automatic state of the scrollbars. You can toggle the scrollbar
    policies to always on or always off to prevent this (see
    horizontalScrollBarPolicy() and verticalScrollBarPolicy()).

    If \a rect is empty, or if the viewport is too small, this
    function will do nothing.

    \sa setTransform(), ensureVisible(), centerOn()
*/
void QGraphicsView::fitInView(const QRectF &rect, Qt::AspectRatioMode aspectRatioMode)
{
    Q_D(QGraphicsView);
    if (!d->scene || rect.isNull())
        return;

    // Reset the view scale to 1:1.
    QRectF unity = d->matrix.mapRect(QRectF(0, 0, 1, 1));
    if (unity.isEmpty())
        return;
    scale(1 / unity.width(), 1 / unity.height());

    // Find the ideal x / y scaling ratio to fit \a rect in the view.
    int margin = 2;
    QRectF viewRect = viewport()->rect().adjusted(margin, margin, -margin, -margin);
    if (viewRect.isEmpty())
        return;
    QRectF sceneRect = d->matrix.mapRect(rect);
    if (sceneRect.isEmpty())
        return;
    qreal xratio = viewRect.width() / sceneRect.width();
    qreal yratio = viewRect.height() / sceneRect.height();

    // Respect the aspect ratio mode.
    switch (aspectRatioMode) {
    case Qt::KeepAspectRatio:
        xratio = yratio = qMin(xratio, yratio);
        break;
    case Qt::KeepAspectRatioByExpanding:
        xratio = yratio = qMax(xratio, yratio);
        break;
    case Qt::IgnoreAspectRatio:
        break;
    }

    // Scale and center on the center of \a rect.
    scale(xratio, yratio);
    centerOn(rect.center());
}

/*!
    \fn void QGraphicsView::fitInView(qreal x, qreal y, qreal w, qreal h,
    Qt::AspectRatioMode aspectRatioMode = Qt::IgnoreAspectRatio)

    \overload

    This convenience function is equivalent to calling
    fitInView(QRectF(\a x, \a y, \a w, \a h), \a aspectRatioMode).

    \sa ensureVisible(), centerOn()
*/

/*!
    \overload

    Ensures that \a item fits tightly inside the view, scaling the view
    according to \a aspectRatioMode.

    \sa ensureVisible(), centerOn()
*/
void QGraphicsView::fitInView(const QGraphicsItem *item, Qt::AspectRatioMode aspectRatioMode)
{
    QPainterPath path = item->isClipped() ? item->clipPath() : item->shape();
    if (item->d_ptr->hasTranslateOnlySceneTransform()) {
        path.translate(item->d_ptr->sceneTransform.dx(), item->d_ptr->sceneTransform.dy());
        fitInView(path.boundingRect(), aspectRatioMode);
    } else {
        fitInView(item->d_ptr->sceneTransform.map(path).boundingRect(), aspectRatioMode);
    }
}

/*!
    Renders the \a source rect, which is in view coordinates, from the scene
    into \a target, which is in paint device coordinates, using \a
    painter. This function is useful for capturing the contents of the view
    onto a paint device, such as a QImage (e.g., to take a screenshot), or for
    printing to QPrinter. For example:

    \snippet doc/src/snippets/code/src_gui_graphicsview_qgraphicsview.cpp 4

    If \a source is a null rect, this function will use viewport()->rect() to
    determine what to draw. If \a target is a null rect, the full dimensions
    of \a painter's paint device (e.g., for a QPrinter, the page size) will be
    used.

    The source rect contents will be transformed according to \a
    aspectRatioMode to fit into the target rect. By default, the aspect ratio
    is kept, and \a source is scaled to fit in \a target.

    \sa QGraphicsScene::render()
*/
void QGraphicsView::render(QPainter *painter, const QRectF &target, const QRect &source,
                           Qt::AspectRatioMode aspectRatioMode)
{
    // ### Switch to using the recursive rendering algorithm instead.

    Q_D(QGraphicsView);
    if (!d->scene || !(painter && painter->isActive()))
        return;

    // Default source rect = viewport rect
    QRect sourceRect = source;
    if (source.isNull())
        sourceRect = viewport()->rect();

    // Default target rect = device rect
    QRectF targetRect = target;
    if (target.isNull()) {
        if (painter->device()->devType() == QInternal::Picture)
            targetRect = sourceRect;
        else
            targetRect.setRect(0, 0, painter->device()->width(), painter->device()->height());
    }

    // Find the ideal x / y scaling ratio to fit \a source into \a target.
    qreal xratio = targetRect.width() / sourceRect.width();
    qreal yratio = targetRect.height() / sourceRect.height();

    // Scale according to the aspect ratio mode.
    switch (aspectRatioMode) {
    case Qt::KeepAspectRatio:
        xratio = yratio = qMin(xratio, yratio);
        break;
    case Qt::KeepAspectRatioByExpanding:
        xratio = yratio = qMax(xratio, yratio);
        break;
    case Qt::IgnoreAspectRatio:
        break;
    }

    // Find all items to draw, and reverse the list (we want to draw
    // in reverse order).
    QPolygonF sourceScenePoly = mapToScene(sourceRect.adjusted(-1, -1, 1, 1));
    QList<QGraphicsItem *> itemList = d->scene->items(sourceScenePoly,
                                                      Qt::IntersectsItemBoundingRect);
    QGraphicsItem **itemArray = new QGraphicsItem *[itemList.size()];
    int numItems = itemList.size();
    for (int i = 0; i < numItems; ++i)
        itemArray[numItems - i - 1] = itemList.at(i);
    itemList.clear();

    // Setup painter matrix.
    QTransform moveMatrix = QTransform::fromTranslate(-d->horizontalScroll(), -d->verticalScroll());
    QTransform painterMatrix = d->matrix * moveMatrix;
    painterMatrix *= QTransform()
                     .translate(targetRect.left(), targetRect.top())
                     .scale(xratio, yratio)
                     .translate(-sourceRect.left(), -sourceRect.top());

    // Generate the style options
    QStyleOptionGraphicsItem *styleOptionArray = d->allocStyleOptionsArray(numItems);
    for (int i = 0; i < numItems; ++i)
        itemArray[i]->d_ptr->initStyleOption(&styleOptionArray[i], painterMatrix, targetRect.toRect());

    painter->save();

    // Clip in device coordinates to avoid QRegion transformations.
    painter->setClipRect(targetRect);
    QPainterPath path;
    path.addPolygon(sourceScenePoly);
    path.closeSubpath();
    painter->setClipPath(painterMatrix.map(path), Qt::IntersectClip);

    // Transform the painter.
    painter->setTransform(painterMatrix, true);

    // Render the scene.
    QRectF sourceSceneRect = sourceScenePoly.boundingRect();
    drawBackground(painter, sourceSceneRect);
    drawItems(painter, numItems, itemArray, styleOptionArray);
    drawForeground(painter, sourceSceneRect);

    delete [] itemArray;
    d->freeStyleOptionsArray(styleOptionArray);

    painter->restore();
}

/*!
    Returns a list of all the items in the associated scene, in descending
    stacking order (i.e., the first item in the returned list is the uppermost
    item).

    \sa QGraphicsScene::items(), {QGraphicsItem#Sorting}{Sorting}
*/
QList<QGraphicsItem *> QGraphicsView::items() const
{
    Q_D(const QGraphicsView);
    if (!d->scene)
        return QList<QGraphicsItem *>();
    return d->scene->items();
}

/*!
    Returns a list of all the items at the position \a pos in the view. The
    items are listed in descending stacking order (i.e., the first item in the
    list is the uppermost item, and the last item is the lowermost item). \a
    pos is in viewport coordinates.

    This function is most commonly called from within mouse event handlers in
    a subclass in QGraphicsView. \a pos is in untransformed viewport
    coordinates, just like QMouseEvent::pos().

    \snippet doc/src/snippets/code/src_gui_graphicsview_qgraphicsview.cpp 5

    \sa QGraphicsScene::items(), {QGraphicsItem#Sorting}{Sorting}
*/
QList<QGraphicsItem *> QGraphicsView::items(const QPoint &pos) const
{
    Q_D(const QGraphicsView);
    if (!d->scene)
        return QList<QGraphicsItem *>();
    // ### Unify these two, and use the items(QPointF) version in
    // QGraphicsScene instead. The scene items function could use the viewport
    // transform to map the point to a rect/polygon.
    if ((d->identityMatrix || d->matrix.type() <= QTransform::TxScale)) {
        // Use the rect version
        QTransform xinv = viewportTransform().inverted();
        return d->scene->items(xinv.mapRect(QRectF(pos.x(), pos.y(), 1, 1)),
                               Qt::IntersectsItemShape,
                               Qt::DescendingOrder,
                               viewportTransform());
    }
    // Use the polygon version
    return d->scene->items(mapToScene(pos.x(), pos.y(), 1, 1),
                           Qt::IntersectsItemShape,
                           Qt::DescendingOrder,
                           viewportTransform());
}

/*!
    \fn QGraphicsView::items(int x, int y) const

    This function is provided for convenience. It's equivalent to calling
    items(QPoint(\a x, \a y)).
*/

/*!
    \overload

    Returns a list of all the items that, depending on \a mode, are either
    contained by or intersect with \a rect. \a rect is in viewport
    coordinates.

    The default value for \a mode is Qt::IntersectsItemShape; all items whose
    exact shape intersects with or is contained by \a rect are returned.

    The items are sorted in descending stacking order (i.e., the first item in
    the returned list is the uppermost item).

    \sa itemAt(), items(), mapToScene(), {QGraphicsItem#Sorting}{Sorting}
*/
QList<QGraphicsItem *> QGraphicsView::items(const QRect &rect, Qt::ItemSelectionMode mode) const
{
    Q_D(const QGraphicsView);
    if (!d->scene)
        return QList<QGraphicsItem *>();
    return d->scene->items(mapToScene(rect), mode, Qt::DescendingOrder, viewportTransform());
}

/*!
    \fn QList<QGraphicsItem *> QGraphicsView::items(int x, int y, int w, int h, Qt::ItemSelectionMode mode) const
    \since 4.3

    This convenience function is equivalent to calling items(QRectF(\a x, \a
    y, \a w, \a h), \a mode).
*/

/*!
    \overload

    Returns a list of all the items that, depending on \a mode, are either
    contained by or intersect with \a polygon. \a polygon is in viewport
    coordinates.

    The default value for \a mode is Qt::IntersectsItemShape; all items whose
    exact shape intersects with or is contained by \a polygon are returned.

    The items are sorted by descending stacking order (i.e., the first item in
    the returned list is the uppermost item).

    \sa itemAt(), items(), mapToScene(), {QGraphicsItem#Sorting}{Sorting}
*/
QList<QGraphicsItem *> QGraphicsView::items(const QPolygon &polygon, Qt::ItemSelectionMode mode) const
{
    Q_D(const QGraphicsView);
    if (!d->scene)
        return QList<QGraphicsItem *>();
    return d->scene->items(mapToScene(polygon), mode, Qt::DescendingOrder, viewportTransform());
}

/*!
    \overload

    Returns a list of all the items that, depending on \a mode, are either
    contained by or intersect with \a path. \a path is in viewport
    coordinates.

    The default value for \a mode is Qt::IntersectsItemShape; all items whose
    exact shape intersects with or is contained by \a path are returned.

    \sa itemAt(), items(), mapToScene(), {QGraphicsItem#Sorting}{Sorting}
*/
QList<QGraphicsItem *> QGraphicsView::items(const QPainterPath &path, Qt::ItemSelectionMode mode) const
{
    Q_D(const QGraphicsView);
    if (!d->scene)
        return QList<QGraphicsItem *>();
    return d->scene->items(mapToScene(path), mode, Qt::DescendingOrder, viewportTransform());
}

/*!
    Returns the item at position \a pos, which is in viewport coordinates.
    If there are several items at this position, this function returns
    the topmost item.

    Example:

    \snippet doc/src/snippets/code/src_gui_graphicsview_qgraphicsview.cpp 6

    \sa items(), {QGraphicsItem#Sorting}{Sorting}
*/
QGraphicsItem *QGraphicsView::itemAt(const QPoint &pos) const
{
    Q_D(const QGraphicsView);
    if (!d->scene)
        return 0;
    QList<QGraphicsItem *> itemsAtPos = items(pos);
    return itemsAtPos.isEmpty() ? 0 : itemsAtPos.first();
}

/*!
    \overload
    \fn QGraphicsItem *QGraphicsView::itemAt(int x, int y) const

    This function is provided for convenience. It's equivalent to
    calling itemAt(QPoint(\a x, \a y)).
*/

/*!
    Returns the viewport coordinate \a point mapped to scene coordinates.

    Note: It can be useful to map the whole rectangle covered by the pixel at
    \a point instead of the point itself. To do this, you can call
    mapToScene(QRect(\a point, QSize(2, 2))).

    \sa mapFromScene()
*/
QPointF QGraphicsView::mapToScene(const QPoint &point) const
{
    Q_D(const QGraphicsView);
    QPointF p = point;
    p.rx() += d->horizontalScroll();
    p.ry() += d->verticalScroll();
    return d->identityMatrix ? p : d->matrix.inverted().map(p);
}

/*!
    \fn QGraphicsView::mapToScene(int x, int y) const

    This function is provided for convenience. It's equivalent to calling
    mapToScene(QPoint(\a x, \a y)).
*/

/*!
    Returns the viewport rectangle \a rect mapped to a scene coordinate
    polygon.

    \sa mapFromScene()
*/
QPolygonF QGraphicsView::mapToScene(const QRect &rect) const
{
    Q_D(const QGraphicsView);
    if (!rect.isValid())
        return QPolygonF();

    QPointF scrollOffset(d->horizontalScroll(), d->verticalScroll());
    QRect r = rect.adjusted(0, 0, 1, 1);
    QPointF tl = scrollOffset + r.topLeft();
    QPointF tr = scrollOffset + r.topRight();
    QPointF br = scrollOffset + r.bottomRight();
    QPointF bl = scrollOffset + r.bottomLeft();

    QPolygonF poly(4);
    if (!d->identityMatrix) {
        QTransform x = d->matrix.inverted();
        poly[0] = x.map(tl);
        poly[1] = x.map(tr);
        poly[2] = x.map(br);
        poly[3] = x.map(bl);
    } else {
        poly[0] = tl;
        poly[1] = tr;
        poly[2] = br;
        poly[3] = bl;
    }
    return poly;
}

/*!
    \fn QGraphicsView::mapToScene(int x, int y, int w, int h) const

    This function is provided for convenience. It's equivalent to calling
    mapToScene(QRect(\a x, \a y, \a w, \a h)).
*/

/*!
    Returns the viewport polygon \a polygon mapped to a scene coordinate
    polygon.

    \sa mapFromScene()
*/
QPolygonF QGraphicsView::mapToScene(const QPolygon &polygon) const
{
    QPolygonF poly;
    foreach (const QPoint &point, polygon)
        poly << mapToScene(point);
    return poly;
}

/*!
    Returns the viewport painter path \a path mapped to a scene coordinate
    painter path.

    \sa mapFromScene()
*/
QPainterPath QGraphicsView::mapToScene(const QPainterPath &path) const
{
    Q_D(const QGraphicsView);
    QTransform matrix = QTransform::fromTranslate(d->horizontalScroll(), d->verticalScroll());
    matrix *= d->matrix.inverted();
    return matrix.map(path);
}

/*!
    Returns the scene coordinate \a point to viewport coordinates.

    \sa mapToScene()
*/
QPoint QGraphicsView::mapFromScene(const QPointF &point) const
{
    Q_D(const QGraphicsView);
    QPointF p = d->identityMatrix ? point : d->matrix.map(point);
    p.rx() -= d->horizontalScroll();
    p.ry() -= d->verticalScroll();
    return p.toPoint();
}

/*!
    \fn QGraphicsView::mapFromScene(qreal x, qreal y) const

    This function is provided for convenience. It's equivalent to
    calling mapFromScene(QPointF(\a x, \a y)).
*/

/*!
    Returns the scene rectangle \a rect to a viewport coordinate
    polygon.

    \sa mapToScene()
*/
QPolygon QGraphicsView::mapFromScene(const QRectF &rect) const
{
    Q_D(const QGraphicsView);
    QPointF tl;
    QPointF tr;
    QPointF br;
    QPointF bl;
    if (!d->identityMatrix) {
        const QTransform &x = d->matrix;
        tl = x.map(rect.topLeft());
        tr = x.map(rect.topRight());
        br = x.map(rect.bottomRight());
        bl = x.map(rect.bottomLeft());
    } else {
        tl = rect.topLeft();
        tr = rect.topRight();
        br = rect.bottomRight();
        bl = rect.bottomLeft();
    }
    QPointF scrollOffset(d->horizontalScroll(), d->verticalScroll());
    tl -= scrollOffset;
    tr -= scrollOffset;
    br -= scrollOffset;
    bl -= scrollOffset;

    QPolygon poly(4);
    poly[0] = tl.toPoint();
    poly[1] = tr.toPoint();
    poly[2] = br.toPoint();
    poly[3] = bl.toPoint();
    return poly;
}

/*!
    \fn QGraphicsView::mapFromScene(qreal x, qreal y, qreal w, qreal h) const

    This function is provided for convenience. It's equivalent to
    calling mapFromScene(QRectF(\a x, \a y, \a w, \a h)).
*/

/*!
    Returns the scene coordinate polygon \a polygon to a viewport coordinate
    polygon.

    \sa mapToScene()
*/
QPolygon QGraphicsView::mapFromScene(const QPolygonF &polygon) const
{
    QPolygon poly;
    foreach (const QPointF &point, polygon)
        poly << mapFromScene(point);
    return poly;
}

/*!
    Returns the scene coordinate painter path \a path to a viewport coordinate
    painter path.

    \sa mapToScene()
*/
QPainterPath QGraphicsView::mapFromScene(const QPainterPath &path) const
{
    Q_D(const QGraphicsView);
    QTransform matrix = d->matrix;
    matrix *= QTransform::fromTranslate(-d->horizontalScroll(), -d->verticalScroll());
    return matrix.map(path);
}

/*!
    \reimp
*/
QVariant QGraphicsView::inputMethodQuery(Qt::InputMethodQuery query) const
{
    Q_D(const QGraphicsView);
    if (!d->scene)
        return QVariant();

    QVariant value = d->scene->inputMethodQuery(query);
    if (value.type() == QVariant::RectF)
        value = d->mapRectFromScene(value.toRectF());
    else if (value.type() == QVariant::PointF)
        value = mapFromScene(value.toPointF());
    else if (value.type() == QVariant::Rect)
        value = d->mapRectFromScene(value.toRect()).toRect();
    else if (value.type() == QVariant::Point)
        value = mapFromScene(value.toPoint());
    return value;
}

/*!
    \property QGraphicsView::backgroundBrush
    \brief the background brush of the scene.

    This property sets the background brush for the scene in this view. It is
    used to override the scene's own background, and defines the behavior of
    drawBackground(). To provide custom background drawing for this view, you
    can reimplement drawBackground() instead.

    By default, this property contains a brush with the Qt::NoBrush pattern.

    \sa QGraphicsScene::backgroundBrush, foregroundBrush
*/
QBrush QGraphicsView::backgroundBrush() const
{
    Q_D(const QGraphicsView);
    return d->backgroundBrush;
}
void QGraphicsView::setBackgroundBrush(const QBrush &brush)
{
    Q_D(QGraphicsView);
    d->backgroundBrush = brush;
    d->updateAll();

    if (d->cacheMode & CacheBackground) {
        // Invalidate the background pixmap
        d->mustResizeBackgroundPixmap = true;
    }
}

/*!
    \property QGraphicsView::foregroundBrush
    \brief the foreground brush of the scene.

    This property sets the foreground brush for the scene in this view. It is
    used to override the scene's own foreground, and defines the behavior of
    drawForeground(). To provide custom foreground drawing for this view, you
    can reimplement drawForeground() instead.

    By default, this property contains a brush with the Qt::NoBrush pattern.

    \sa QGraphicsScene::foregroundBrush, backgroundBrush
*/
QBrush QGraphicsView::foregroundBrush() const
{
    Q_D(const QGraphicsView);
    return d->foregroundBrush;
}
void QGraphicsView::setForegroundBrush(const QBrush &brush)
{
    Q_D(QGraphicsView);
    d->foregroundBrush = brush;
    d->updateAll();
}

/*!
    Schedules an update of the scene rectangles \a rects.

    \sa QGraphicsScene::changed()
*/
void QGraphicsView::updateScene(const QList<QRectF> &rects)
{
    // ### Note: Since 4.5, this slot is only called if the user explicitly
    // establishes a connection between the scene and the view, as the scene
    // and view are no longer connected. We need to keep it working (basically
    // leave it as it is), but the new delivery path is through
    // QGraphicsScenePrivate::itemUpdate().
    Q_D(QGraphicsView);
    if (d->fullUpdatePending || d->viewportUpdateMode == QGraphicsView::NoViewportUpdate)
        return;

    // Extract and reset dirty scene rect info.
    QVector<QRect> dirtyViewportRects;
    const QVector<QRect> &dirtyRects = d->dirtyRegion.rects();
    for (int i = 0; i < dirtyRects.size(); ++i)
        dirtyViewportRects += dirtyRects.at(i);
    d->dirtyRegion = QRegion();
    d->dirtyBoundingRect = QRect();

    bool fullUpdate = !d->accelerateScrolling || d->viewportUpdateMode == QGraphicsView::FullViewportUpdate;
    bool boundingRectUpdate = (d->viewportUpdateMode == QGraphicsView::BoundingRectViewportUpdate)
                              || (d->viewportUpdateMode == QGraphicsView::SmartViewportUpdate
                                  && ((dirtyViewportRects.size() + rects.size()) >= QGRAPHICSVIEW_REGION_RECT_THRESHOLD));

    QRegion updateRegion;
    QRect boundingRect;
    QRect viewportRect = viewport()->rect();
    bool redraw = false;
    QTransform transform = viewportTransform();

    // Convert scene rects to viewport rects.
    foreach (const QRectF &rect, rects) {
        QRect xrect = transform.mapRect(rect).toAlignedRect();
        if (!(d->optimizationFlags & DontAdjustForAntialiasing))
            xrect.adjust(-2, -2, 2, 2);
        else
            xrect.adjust(-1, -1, 1, 1);
        if (!viewportRect.intersects(xrect))
            continue;
        dirtyViewportRects << xrect;
    }

    foreach (const QRect &rect, dirtyViewportRects) {
        // Add the exposed rect to the update region. In rect update
        // mode, we only count the bounding rect of items.
        if (!boundingRectUpdate) {
            updateRegion += rect;
        } else {
            boundingRect |= rect;
        }
        redraw = true;
        if (fullUpdate) {
            // If fullUpdate is true and we found a visible dirty rect,
            // we're done.
            break;
        }
    }

    if (!redraw)
        return;

    if (fullUpdate)
        viewport()->update();
    else if (boundingRectUpdate)
        viewport()->update(boundingRect);
    else
        viewport()->update(updateRegion);
}

/*!
    Notifies QGraphicsView that the scene's scene rect has changed.  \a rect
    is the new scene rect. If the view already has an explicitly set scene
    rect, this function does nothing.

    \sa sceneRect, QGraphicsScene::sceneRectChanged()
*/
void QGraphicsView::updateSceneRect(const QRectF &rect)
{
    Q_D(QGraphicsView);
    if (!d->hasSceneRect) {
        d->sceneRect = rect;
        d->recalculateContentSize();
    }
}

/*!
    This slot is called by QAbstractScrollArea after setViewport() has been
    called. Reimplement this function in a subclass of QGraphicsView to
    initialize the new viewport \a widget before it is used.

    \sa setViewport()
*/
void QGraphicsView::setupViewport(QWidget *widget)
{
    Q_D(QGraphicsView);

    if (!widget) {
        qWarning("QGraphicsView::setupViewport: cannot initialize null widget");
        return;
    }

    const bool isGLWidget = widget->inherits("QGLWidget");

    d->accelerateScrolling = !(isGLWidget);

    widget->setFocusPolicy(Qt::StrongFocus);

    if (!isGLWidget) {
        // autoFillBackground enables scroll acceleration.
        widget->setAutoFillBackground(true);
    }

    // We are only interested in mouse tracking if items
    // accept hover events or use non-default cursors or if
    // AnchorUnderMouse is used as transformation or resize anchor.
    if ((d->scene && (!d->scene->d_func()->allItemsIgnoreHoverEvents
                     || !d->scene->d_func()->allItemsUseDefaultCursor))
        || d->transformationAnchor == AnchorUnderMouse
        || d->resizeAnchor == AnchorUnderMouse) {
        widget->setMouseTracking(true);
    }

    // enable touch events if any items is interested in them
    if (d->scene && !d->scene->d_func()->allItemsIgnoreTouchEvents)
        widget->setAttribute(Qt::WA_AcceptTouchEvents);

#ifndef QT_NO_GESTURES
    if (d->scene) {
        foreach (Qt::GestureType gesture, d->scene->d_func()->grabbedGestures.keys())
            widget->grabGesture(gesture);
    }
#endif

    widget->setAcceptDrops(acceptDrops());
}

/*!
    \reimp
*/
bool QGraphicsView::event(QEvent *event)
{
    Q_D(QGraphicsView);

    if (d->sceneInteractionAllowed) {
        switch (event->type()) {
        case QEvent::ShortcutOverride:
            if (d->scene)
                return QApplication::sendEvent(d->scene, event);
            break;
        case QEvent::KeyPress:
            if (d->scene) {
                QKeyEvent *k = static_cast<QKeyEvent *>(event);
                if (k->key() == Qt::Key_Tab || k->key() == Qt::Key_Backtab) {
                    // Send the key events to the scene. This will invoke the
                    // scene's tab focus handling, and if the event is
                    // accepted, we return (prevent further event delivery),
                    // and the base implementation will call QGraphicsView's
                    // focusNextPrevChild() function. If the event is ignored,
                    // we fall back to standard tab focus handling.
                    QApplication::sendEvent(d->scene, event);
                    if (event->isAccepted())
                        return true;
                    // Ensure the event doesn't propagate just because the
                    // scene ignored it. If the event propagates, then tab
                    // handling will be called twice (this and parent).
                    event->accept();
                }
            }
            break;
        default:
            break;
        }
    }

    return QAbstractScrollArea::event(event);
}

/*!
    \reimp
*/
bool QGraphicsView::viewportEvent(QEvent *event)
{
    Q_D(QGraphicsView);
    if (!d->scene)
        return QAbstractScrollArea::viewportEvent(event);

    switch (event->type()) {
    case QEvent::Enter:
        QApplication::sendEvent(d->scene, event);
        break;
    case QEvent::WindowActivate:
        QApplication::sendEvent(d->scene, event);
        break;
    case QEvent::WindowDeactivate:
        // ### This is a temporary fix for until we get proper mouse
        // grab events. mouseGrabberItem should be set to 0 if we lose
        // the mouse grab.
        // Remove all popups when the scene loses focus.
        if (!d->scene->d_func()->popupWidgets.isEmpty())
            d->scene->d_func()->removePopup(d->scene->d_func()->popupWidgets.first());
        QApplication::sendEvent(d->scene, event);
        break;
    case QEvent::Show:
        if (d->scene && isActiveWindow()) {
            QEvent windowActivate(QEvent::WindowActivate);
            QApplication::sendEvent(d->scene, &windowActivate);
        }
        break;
    case QEvent::Hide:
        // spontaneous event will generate a WindowDeactivate.
        if (!event->spontaneous() && d->scene && isActiveWindow()) {
            QEvent windowDeactivate(QEvent::WindowDeactivate);
            QApplication::sendEvent(d->scene, &windowDeactivate);
        }
        break;
    case QEvent::Leave:
        // ### This is a temporary fix for until we get proper mouse grab
        // events. activeMouseGrabberItem should be set to 0 if we lose the
        // mouse grab.
        if ((QApplication::activePopupWidget() && QApplication::activePopupWidget() != window())
            || (QApplication::activeModalWidget() && QApplication::activeModalWidget() != window())
            || (QApplication::activeWindow() != window())) {
            if (!d->scene->d_func()->popupWidgets.isEmpty())
                d->scene->d_func()->removePopup(d->scene->d_func()->popupWidgets.first());
        }
        d->useLastMouseEvent = false;
        // a hack to pass a viewport pointer to the scene inside the leave event
        Q_ASSERT(event->d == 0);
        event->d = reinterpret_cast<QEventPrivate *>(viewport());
        QApplication::sendEvent(d->scene, event);
        break;
#ifndef QT_NO_TOOLTIP
    case QEvent::ToolTip: {
        QHelpEvent *toolTip = static_cast<QHelpEvent *>(event);
        QGraphicsSceneHelpEvent helpEvent(QEvent::GraphicsSceneHelp);
        helpEvent.setWidget(viewport());
        helpEvent.setScreenPos(toolTip->globalPos());
        helpEvent.setScenePos(mapToScene(toolTip->pos()));
        QApplication::sendEvent(d->scene, &helpEvent);
        toolTip->setAccepted(helpEvent.isAccepted());
        return true;
    }
#endif
    case QEvent::Paint:
        // Reset full update
        d->fullUpdatePending = false;
        d->dirtyScrollOffset = QPoint();
        if (d->scene) {
            // Check if this view reimplements the updateScene slot; if it
            // does, we can't do direct update delivery and have to fall back
            // to connecting the changed signal.
            if (!d->updateSceneSlotReimplementedChecked) {
                d->updateSceneSlotReimplementedChecked = true;
                const QMetaObject *mo = metaObject();
                if (mo != &QGraphicsView::staticMetaObject) {
                    if (mo->indexOfSlot("updateScene(QList<QRectF>)")
                        != QGraphicsView::staticMetaObject.indexOfSlot("updateScene(QList<QRectF>)")) {
                        connect(d->scene, SIGNAL(changed(QList<QRectF>)),
                                this, SLOT(updateScene(QList<QRectF>)));
                    }
                }
            }
        }
        break;
    case QEvent::TouchBegin:
    case QEvent::TouchUpdate:
    case QEvent::TouchEnd:
    {
        if (!isEnabled())
            return false;

        if (d->scene && d->sceneInteractionAllowed) {
            // Convert and deliver the touch event to the scene.
            QTouchEvent *touchEvent = static_cast<QTouchEvent *>(event);
            touchEvent->setWidget(viewport());
            QGraphicsViewPrivate::translateTouchEvent(d, touchEvent);
            (void) QApplication::sendEvent(d->scene, touchEvent);
        }

        return true;
    }
#ifndef QT_NO_GESTURES
    case QEvent::Gesture:
    case QEvent::GestureOverride:
    {
        if (!isEnabled())
            return false;

        if (d->scene && d->sceneInteractionAllowed) {
            QGestureEvent *gestureEvent = static_cast<QGestureEvent *>(event);
            gestureEvent->setWidget(viewport());
            (void) QApplication::sendEvent(d->scene, gestureEvent);
        }
        return true;
    }
#endif // QT_NO_GESTURES
    default:
        break;
    }

    return QAbstractScrollArea::viewportEvent(event);
}

#ifndef QT_NO_CONTEXTMENU
/*!
    \reimp
*/
void QGraphicsView::contextMenuEvent(QContextMenuEvent *event)
{
    Q_D(QGraphicsView);
    if (!d->scene || !d->sceneInteractionAllowed)
        return;

    d->mousePressViewPoint = event->pos();
    d->mousePressScenePoint = mapToScene(d->mousePressViewPoint);
    d->mousePressScreenPoint = event->globalPos();
    d->lastMouseMoveScenePoint = d->mousePressScenePoint;
    d->lastMouseMoveScreenPoint = d->mousePressScreenPoint;

    QGraphicsSceneContextMenuEvent contextEvent(QEvent::GraphicsSceneContextMenu);
    contextEvent.setWidget(viewport());
    contextEvent.setScenePos(d->mousePressScenePoint);
    contextEvent.setScreenPos(d->mousePressScreenPoint);
    contextEvent.setModifiers(event->modifiers());
    contextEvent.setReason((QGraphicsSceneContextMenuEvent::Reason)(event->reason()));
    contextEvent.setAccepted(event->isAccepted());
    QApplication::sendEvent(d->scene, &contextEvent);
    event->setAccepted(contextEvent.isAccepted());
}
#endif // QT_NO_CONTEXTMENU

/*!
    \reimp
*/
void QGraphicsView::dropEvent(QDropEvent *event)
{
#ifndef QT_NO_DRAGANDDROP
    Q_D(QGraphicsView);
    if (!d->scene || !d->sceneInteractionAllowed)
        return;

    // Generate a scene event.
    QGraphicsSceneDragDropEvent sceneEvent(QEvent::GraphicsSceneDrop);
    d->populateSceneDragDropEvent(&sceneEvent, event);

    // Send it to the scene.
    QApplication::sendEvent(d->scene, &sceneEvent);

    // Accept the originating event if the scene accepted the scene event.
    event->setAccepted(sceneEvent.isAccepted());
    if (sceneEvent.isAccepted())
        event->setDropAction(sceneEvent.dropAction());

    delete d->lastDragDropEvent;
    d->lastDragDropEvent = 0;

#else
    Q_UNUSED(event)
#endif
}

/*!
    \reimp
*/
void QGraphicsView::dragEnterEvent(QDragEnterEvent *event)
{
#ifndef QT_NO_DRAGANDDROP
    Q_D(QGraphicsView);
    if (!d->scene || !d->sceneInteractionAllowed)
        return;

    // Disable replaying of mouse move events.
    d->useLastMouseEvent = false;

    // Generate a scene event.
    QGraphicsSceneDragDropEvent sceneEvent(QEvent::GraphicsSceneDragEnter);
    d->populateSceneDragDropEvent(&sceneEvent, event);

    // Store it for later use.
    d->storeDragDropEvent(&sceneEvent);

    // Send it to the scene.
    QApplication::sendEvent(d->scene, &sceneEvent);

    // Accept the originating event if the scene accepted the scene event.
    if (sceneEvent.isAccepted()) {
        event->setAccepted(true);
        event->setDropAction(sceneEvent.dropAction());
    }
#else
    Q_UNUSED(event)
#endif
}

/*!
    \reimp
*/
void QGraphicsView::dragLeaveEvent(QDragLeaveEvent *event)
{
#ifndef QT_NO_DRAGANDDROP
    Q_D(QGraphicsView);
    if (!d->scene || !d->sceneInteractionAllowed)
        return;
    if (!d->lastDragDropEvent) {
        qWarning("QGraphicsView::dragLeaveEvent: drag leave received before drag enter");
        return;
    }

    // Generate a scene event.
    QGraphicsSceneDragDropEvent sceneEvent(QEvent::GraphicsSceneDragLeave);
    sceneEvent.setScenePos(d->lastDragDropEvent->scenePos());
    sceneEvent.setScreenPos(d->lastDragDropEvent->screenPos());
    sceneEvent.setButtons(d->lastDragDropEvent->buttons());
    sceneEvent.setModifiers(d->lastDragDropEvent->modifiers());
    sceneEvent.setPossibleActions(d->lastDragDropEvent->possibleActions());
    sceneEvent.setProposedAction(d->lastDragDropEvent->proposedAction());
    sceneEvent.setDropAction(d->lastDragDropEvent->dropAction());
    sceneEvent.setMimeData(d->lastDragDropEvent->mimeData());
    sceneEvent.setWidget(d->lastDragDropEvent->widget());
    sceneEvent.setSource(d->lastDragDropEvent->source());
    delete d->lastDragDropEvent;
    d->lastDragDropEvent = 0;

    // Send it to the scene.
    QApplication::sendEvent(d->scene, &sceneEvent);

    // Accept the originating event if the scene accepted the scene event.
    if (sceneEvent.isAccepted())
        event->setAccepted(true);
#else
    Q_UNUSED(event)
#endif
}

/*!
    \reimp
*/
void QGraphicsView::dragMoveEvent(QDragMoveEvent *event)
{
#ifndef QT_NO_DRAGANDDROP
    Q_D(QGraphicsView);
    if (!d->scene || !d->sceneInteractionAllowed)
        return;

    // Generate a scene event.
    QGraphicsSceneDragDropEvent sceneEvent(QEvent::GraphicsSceneDragMove);
    d->populateSceneDragDropEvent(&sceneEvent, event);

    // Store it for later use.
    d->storeDragDropEvent(&sceneEvent);

    // Send it to the scene.
    QApplication::sendEvent(d->scene, &sceneEvent);

    // Ignore the originating event if the scene ignored the scene event.
    event->setAccepted(sceneEvent.isAccepted());
    if (sceneEvent.isAccepted())
        event->setDropAction(sceneEvent.dropAction());
#else
    Q_UNUSED(event)
#endif
}

/*!
    \reimp
*/
void QGraphicsView::focusInEvent(QFocusEvent *event)
{
    Q_D(QGraphicsView);
    d->updateInputMethodSensitivity();
    QAbstractScrollArea::focusInEvent(event);
    if (d->scene)
        QApplication::sendEvent(d->scene, event);
    // Pass focus on if the scene cannot accept focus.
    if (!d->scene || !event->isAccepted())
        QAbstractScrollArea::focusInEvent(event);
}

/*!
    \reimp
*/
bool QGraphicsView::focusNextPrevChild(bool next)
{
    return QAbstractScrollArea::focusNextPrevChild(next);
}

/*!
    \reimp
*/
void QGraphicsView::focusOutEvent(QFocusEvent *event)
{
    Q_D(QGraphicsView);
    QAbstractScrollArea::focusOutEvent(event);
    if (d->scene)
        QApplication::sendEvent(d->scene, event);
}

/*!
    \reimp
*/
void QGraphicsView::keyPressEvent(QKeyEvent *event)
{
    Q_D(QGraphicsView);
    if (!d->scene || !d->sceneInteractionAllowed) {
        QAbstractScrollArea::keyPressEvent(event);
        return;
    }
    QApplication::sendEvent(d->scene, event);
    if (!event->isAccepted())
        QAbstractScrollArea::keyPressEvent(event);
}

/*!
    \reimp
*/
void QGraphicsView::keyReleaseEvent(QKeyEvent *event)
{
    Q_D(QGraphicsView);
    if (!d->scene || !d->sceneInteractionAllowed)
        return;
    QApplication::sendEvent(d->scene, event);
    if (!event->isAccepted())
        QAbstractScrollArea::keyReleaseEvent(event);
}

/*!
    \reimp
*/
void QGraphicsView::mouseDoubleClickEvent(QMouseEvent *event)
{
    Q_D(QGraphicsView);
    if (!d->scene || !d->sceneInteractionAllowed)
        return;

    d->storeMouseEvent(event);
    d->mousePressViewPoint = event->pos();
    d->mousePressScenePoint = mapToScene(d->mousePressViewPoint);
    d->mousePressScreenPoint = event->globalPos();
    d->lastMouseMoveScenePoint = d->mousePressScenePoint;
    d->lastMouseMoveScreenPoint = d->mousePressScreenPoint;
    d->mousePressButton = event->button();

    QGraphicsSceneMouseEvent mouseEvent(QEvent::GraphicsSceneMouseDoubleClick);
    mouseEvent.setWidget(viewport());
    mouseEvent.setButtonDownScenePos(d->mousePressButton, d->mousePressScenePoint);
    mouseEvent.setButtonDownScreenPos(d->mousePressButton, d->mousePressScreenPoint);
    mouseEvent.setScenePos(mapToScene(d->mousePressViewPoint));
    mouseEvent.setScreenPos(d->mousePressScreenPoint);
    mouseEvent.setLastScenePos(d->lastMouseMoveScenePoint);
    mouseEvent.setLastScreenPos(d->lastMouseMoveScreenPoint);
    mouseEvent.setButtons(event->buttons());
    mouseEvent.setButtons(event->buttons());
    mouseEvent.setAccepted(false);
    mouseEvent.setButton(event->button());
    mouseEvent.setModifiers(event->modifiers());
    if (event->spontaneous())
        qt_sendSpontaneousEvent(d->scene, &mouseEvent);
    else
        QApplication::sendEvent(d->scene, &mouseEvent);
}

/*!
    \reimp
*/
void QGraphicsView::mousePressEvent(QMouseEvent *event)
{
    Q_D(QGraphicsView);

    // Store this event for replaying, finding deltas, and for
    // scroll-dragging; even in non-interactive mode, scroll hand dragging is
    // allowed, so we store the event at the very top of this function.
    d->storeMouseEvent(event);
    d->lastMouseEvent.setAccepted(false);

    if (d->sceneInteractionAllowed) {
        // Store some of the event's button-down data.
        d->mousePressViewPoint = event->pos();
        d->mousePressScenePoint = mapToScene(d->mousePressViewPoint);
        d->mousePressScreenPoint = event->globalPos();
        d->lastMouseMoveScenePoint = d->mousePressScenePoint;
        d->lastMouseMoveScreenPoint = d->mousePressScreenPoint;
        d->mousePressButton = event->button();

        if (d->scene) {
            // Convert and deliver the mouse event to the scene.
            QGraphicsSceneMouseEvent mouseEvent(QEvent::GraphicsSceneMousePress);
            mouseEvent.setWidget(viewport());
            mouseEvent.setButtonDownScenePos(d->mousePressButton, d->mousePressScenePoint);
            mouseEvent.setButtonDownScreenPos(d->mousePressButton, d->mousePressScreenPoint);
            mouseEvent.setScenePos(d->mousePressScenePoint);
            mouseEvent.setScreenPos(d->mousePressScreenPoint);
            mouseEvent.setLastScenePos(d->lastMouseMoveScenePoint);
            mouseEvent.setLastScreenPos(d->lastMouseMoveScreenPoint);
            mouseEvent.setButtons(event->buttons());
            mouseEvent.setButton(event->button());
            mouseEvent.setModifiers(event->modifiers());
            mouseEvent.setAccepted(false);
            if (event->spontaneous())
                qt_sendSpontaneousEvent(d->scene, &mouseEvent);
            else
                QApplication::sendEvent(d->scene, &mouseEvent);

            // Update the original mouse event accepted state.
            bool isAccepted = mouseEvent.isAccepted();
            event->setAccepted(isAccepted);

            // Update the last mouse event accepted state.
            d->lastMouseEvent.setAccepted(isAccepted);

            if (isAccepted)
                return;
        }
    }

#ifndef QT_NO_RUBBERBAND
    if (d->dragMode == QGraphicsView::RubberBandDrag && !d->rubberBanding) {
        if (d->sceneInteractionAllowed) {
            // Rubberbanding is only allowed in interactive mode.
            event->accept();
            d->rubberBanding = true;
            d->rubberBandRect = QRect();
            if (d->scene) {
                // Initiating a rubber band always clears the selection.
                d->scene->clearSelection();
            }
        }
    } else
#endif
        if (d->dragMode == QGraphicsView::ScrollHandDrag && event->button() == Qt::LeftButton) {
            // Left-button press in scroll hand mode initiates hand scrolling.
            event->accept();
            d->handScrolling = true;
            d->handScrollMotions = 0;
#ifndef QT_NO_CURSOR
            viewport()->setCursor(Qt::ClosedHandCursor);
#endif
        }
}

/*!
    \reimp
*/
void QGraphicsView::mouseMoveEvent(QMouseEvent *event)
{
    Q_D(QGraphicsView);

#ifndef QT_NO_RUBBERBAND
    if (d->dragMode == QGraphicsView::RubberBandDrag && d->sceneInteractionAllowed) {
        d->storeMouseEvent(event);
        if (d->rubberBanding) {
            // Check for enough drag distance
            if ((d->mousePressViewPoint - event->pos()).manhattanLength()
                < QApplication::startDragDistance()) {
                return;
            }

            // Update old rubberband
            if (d->viewportUpdateMode != QGraphicsView::NoViewportUpdate && !d->rubberBandRect.isEmpty()) {
                if (d->viewportUpdateMode != FullViewportUpdate)
                    viewport()->update(d->rubberBandRegion(viewport(), d->rubberBandRect));
                else
                    d->updateAll();
            }

            // Stop rubber banding if the user has let go of all buttons (even
            // if we didn't get the release events).
            if (!event->buttons()) {
                d->rubberBanding = false;
                d->rubberBandRect = QRect();
                return;
            }

            // Update rubberband position
            const QPoint &mp = d->mousePressViewPoint;
            QPoint ep = event->pos();
            d->rubberBandRect = QRect(qMin(mp.x(), ep.x()), qMin(mp.y(), ep.y()),
                                      qAbs(mp.x() - ep.x()) + 1, qAbs(mp.y() - ep.y()) + 1);

            // Update new rubberband
            if (d->viewportUpdateMode != QGraphicsView::NoViewportUpdate){
                if (d->viewportUpdateMode != FullViewportUpdate)
                    viewport()->update(d->rubberBandRegion(viewport(), d->rubberBandRect));
                else
                    d->updateAll();
            }
            // Set the new selection area
            QPainterPath selectionArea;
            selectionArea.addPolygon(mapToScene(d->rubberBandRect));
            selectionArea.closeSubpath();
            if (d->scene)
                d->scene->setSelectionArea(selectionArea, d->rubberBandSelectionMode,
                                           viewportTransform());
            return;
        }
    } else
#endif // QT_NO_RUBBERBAND
    if (d->dragMode == QGraphicsView::ScrollHandDrag) {
        if (d->handScrolling) {
            QScrollBar *hBar = horizontalScrollBar();
            QScrollBar *vBar = verticalScrollBar();
            QPoint delta = event->pos() - d->lastMouseEvent.pos();
            hBar->setValue(hBar->value() + (isRightToLeft() ? delta.x() : -delta.x()));
            vBar->setValue(vBar->value() - delta.y());

            // Detect how much we've scrolled to disambiguate scrolling from
            // clicking.
            ++d->handScrollMotions;
        }
    }

    d->mouseMoveEventHandler(event);
}

/*!
    \reimp
*/
void QGraphicsView::mouseReleaseEvent(QMouseEvent *event)
{
    Q_D(QGraphicsView);

#ifndef QT_NO_RUBBERBAND
    if (d->dragMode == QGraphicsView::RubberBandDrag && d->sceneInteractionAllowed && !event->buttons()) {
        if (d->rubberBanding) {
            if (d->viewportUpdateMode != QGraphicsView::NoViewportUpdate){
                if (d->viewportUpdateMode != FullViewportUpdate)
                    viewport()->update(d->rubberBandRegion(viewport(), d->rubberBandRect));
                else
                    d->updateAll();
            }
            d->rubberBanding = false;
            d->rubberBandRect = QRect();
        }
    } else
#endif
    if (d->dragMode == QGraphicsView::ScrollHandDrag && event->button() == Qt::LeftButton) {
#ifndef QT_NO_CURSOR
        // Restore the open hand cursor. ### There might be items
        // under the mouse that have a valid cursor at this time, so
        // we could repeat the steps from mouseMoveEvent().
        viewport()->setCursor(Qt::OpenHandCursor);
#endif
        d->handScrolling = false;

        if (d->scene && d->sceneInteractionAllowed && !d->lastMouseEvent.isAccepted() && d->handScrollMotions <= 6) {
            // If we've detected very little motion during the hand drag, and
            // no item accepted the last event, we'll interpret that as a
            // click to the scene, and reset the selection.
            d->scene->clearSelection();
        }
    }

    d->storeMouseEvent(event);

    if (!d->sceneInteractionAllowed)
        return;

    if (!d->scene)
        return;

    QGraphicsSceneMouseEvent mouseEvent(QEvent::GraphicsSceneMouseRelease);
    mouseEvent.setWidget(viewport());
    mouseEvent.setButtonDownScenePos(d->mousePressButton, d->mousePressScenePoint);
    mouseEvent.setButtonDownScreenPos(d->mousePressButton, d->mousePressScreenPoint);
    mouseEvent.setScenePos(mapToScene(event->pos()));
    mouseEvent.setScreenPos(event->globalPos());
    mouseEvent.setLastScenePos(d->lastMouseMoveScenePoint);
    mouseEvent.setLastScreenPos(d->lastMouseMoveScreenPoint);
    mouseEvent.setButtons(event->buttons());
    mouseEvent.setButton(event->button());
    mouseEvent.setModifiers(event->modifiers());
    mouseEvent.setAccepted(false);
    if (event->spontaneous())
        qt_sendSpontaneousEvent(d->scene, &mouseEvent);
    else
        QApplication::sendEvent(d->scene, &mouseEvent);

    // Update the last mouse event selected state.
    d->lastMouseEvent.setAccepted(mouseEvent.isAccepted());

#ifndef QT_NO_CURSOR
    if (mouseEvent.isAccepted() && mouseEvent.buttons() == 0 && viewport()->testAttribute(Qt::WA_SetCursor)) {
        // The last mouse release on the viewport will trigger clearing the cursor.
        d->_q_unsetViewportCursor();
    }
#endif
}

#ifndef QT_NO_WHEELEVENT
/*!
    \reimp
*/
void QGraphicsView::wheelEvent(QWheelEvent *event)
{
    Q_D(QGraphicsView);
    if (!d->scene || !d->sceneInteractionAllowed) {
        QAbstractScrollArea::wheelEvent(event);
        return;
    }

    event->ignore();

    QGraphicsSceneWheelEvent wheelEvent(QEvent::GraphicsSceneWheel);
    wheelEvent.setWidget(viewport());
    wheelEvent.setScenePos(mapToScene(event->pos()));
    wheelEvent.setScreenPos(event->globalPos());
    wheelEvent.setButtons(event->buttons());
    wheelEvent.setModifiers(event->modifiers());
    wheelEvent.setDelta(event->delta());
    wheelEvent.setOrientation(event->orientation());
    wheelEvent.setAccepted(false);
    QApplication::sendEvent(d->scene, &wheelEvent);
    event->setAccepted(wheelEvent.isAccepted());
    if (!event->isAccepted())
        QAbstractScrollArea::wheelEvent(event);
}
#endif // QT_NO_WHEELEVENT

/*!
    \reimp
*/
void QGraphicsView::paintEvent(QPaintEvent *event)
{
    Q_D(QGraphicsView);
    if (!d->scene) {
        QAbstractScrollArea::paintEvent(event);
        return;
    }

    // Set up painter state protection.
    d->scene->d_func()->painterStateProtection = !(d->optimizationFlags & DontSavePainterState);

    // Determine the exposed region
    d->exposedRegion = event->region();
    QRectF exposedSceneRect = mapToScene(d->exposedRegion.boundingRect()).boundingRect();

    // Set up the painter
    QPainter painter(viewport());
#ifndef QT_NO_RUBBERBAND
    if (d->rubberBanding && !d->rubberBandRect.isEmpty())
        painter.save();
#endif
    // Set up render hints
    painter.setRenderHints(painter.renderHints(), false);
    painter.setRenderHints(d->renderHints, true);

    // Set up viewport transform
    const bool viewTransformed = isTransformed();
    if (viewTransformed)
        painter.setWorldTransform(viewportTransform());
    const QTransform viewTransform = painter.worldTransform();

    // Draw background
    if ((d->cacheMode & CacheBackground)
#ifdef Q_WS_X11
        && X11->use_xrender
#endif
        ) {
        // Recreate the background pixmap, and flag the whole background as
        // exposed.
        if (d->mustResizeBackgroundPixmap) {
            d->backgroundPixmap = QPixmap(viewport()->size());
            QBrush bgBrush = viewport()->palette().brush(viewport()->backgroundRole());
            if (!bgBrush.isOpaque())
                d->backgroundPixmap.fill(Qt::transparent);
            QPainter p(&d->backgroundPixmap);
            p.fillRect(0, 0, d->backgroundPixmap.width(), d->backgroundPixmap.height(), bgBrush);
            d->backgroundPixmapExposed = QRegion(viewport()->rect());
            d->mustResizeBackgroundPixmap = false;
        }

        // Redraw exposed areas
        if (!d->backgroundPixmapExposed.isEmpty()) {
            QPainter backgroundPainter(&d->backgroundPixmap);
            backgroundPainter.setClipRegion(d->backgroundPixmapExposed, Qt::ReplaceClip);
            if (viewTransformed)
                backgroundPainter.setTransform(viewTransform);
            QRectF backgroundExposedSceneRect = mapToScene(d->backgroundPixmapExposed.boundingRect()).boundingRect();
            drawBackground(&backgroundPainter, backgroundExposedSceneRect);
            d->backgroundPixmapExposed = QRegion();
        }

        // Blit the background from the background pixmap
        if (viewTransformed) {
            painter.setWorldTransform(QTransform());
            painter.drawPixmap(QPoint(), d->backgroundPixmap);
            painter.setWorldTransform(viewTransform);
        } else {
            painter.drawPixmap(QPoint(), d->backgroundPixmap);
        }
    } else {
        if (!(d->optimizationFlags & DontSavePainterState))
            painter.save();
        drawBackground(&painter, exposedSceneRect);
        if (!(d->optimizationFlags & DontSavePainterState))
            painter.restore();
    }

    // Items
    if (!(d->optimizationFlags & IndirectPainting)) {
        const quint32 oldRectAdjust = d->scene->d_func()->rectAdjust;
        if (d->optimizationFlags & QGraphicsView::DontAdjustForAntialiasing)
            d->scene->d_func()->rectAdjust = 1;
        else
            d->scene->d_func()->rectAdjust = 2;
        d->scene->d_func()->drawItems(&painter, viewTransformed ? &viewTransform : 0,
                                      &d->exposedRegion, viewport());
        d->scene->d_func()->rectAdjust = oldRectAdjust;
        // Make sure the painter's world transform is restored correctly when
        // drawing without painter state protection (DontSavePainterState).
        // We only change the worldTransform() so there's no need to do a full-blown
        // save() and restore(). Also note that we don't have to do this in case of
        // IndirectPainting (the else branch), because in that case we always save()
        // and restore() in QGraphicsScene::drawItems().
        if (!d->scene->d_func()->painterStateProtection)
            painter.setOpacity(1.0);
        painter.setWorldTransform(viewTransform);
    } else {
        // Make sure we don't have unpolished items before we draw
        if (!d->scene->d_func()->unpolishedItems.isEmpty())
            d->scene->d_func()->_q_polishItems();
        // We reset updateAll here (after we've issued polish events)
        // so that we can discard update requests coming from polishEvent().
        d->scene->d_func()->updateAll = false;

        // Find all exposed items
        bool allItems = false;
        QList<QGraphicsItem *> itemList = d->findItems(d->exposedRegion, &allItems, viewTransform);
        if (!itemList.isEmpty()) {
            // Generate the style options.
            const int numItems = itemList.size();
            QGraphicsItem **itemArray = &itemList[0]; // Relies on QList internals, but is perfectly valid.
            QStyleOptionGraphicsItem *styleOptionArray = d->allocStyleOptionsArray(numItems);
            QTransform transform(Qt::Uninitialized);
            for (int i = 0; i < numItems; ++i) {
                QGraphicsItem *item = itemArray[i];
                QGraphicsItemPrivate *itemd = item->d_ptr.data();
                itemd->initStyleOption(&styleOptionArray[i], viewTransform, d->exposedRegion, allItems);
                // Cache the item's area in view coordinates.
                // Note that we have to do this here in case the base class implementation
                // (QGraphicsScene::drawItems) is not called. If it is, we'll do this
                // operation twice, but that's the price one has to pay for using indirect
                // painting :-/.
                const QRectF brect = adjustedItemEffectiveBoundingRect(item);
                if (!itemd->itemIsUntransformable()) {
                    transform = item->sceneTransform();
                    if (viewTransformed)
                        transform *= viewTransform;
                } else {
                    transform = item->deviceTransform(viewTransform);
                }
                itemd->paintedViewBoundingRects.insert(d->viewport, transform.mapRect(brect).toRect());
            }
            // Draw the items.
            drawItems(&painter, numItems, itemArray, styleOptionArray);
            d->freeStyleOptionsArray(styleOptionArray);
        }
    }

    // Foreground
    drawForeground(&painter, exposedSceneRect);

#ifndef QT_NO_RUBBERBAND
    // Rubberband
    if (d->rubberBanding && !d->rubberBandRect.isEmpty()) {
        painter.restore();
        QStyleOptionRubberBand option;
        option.initFrom(viewport());
        option.rect = d->rubberBandRect;
        option.shape = QRubberBand::Rectangle;

        QStyleHintReturnMask mask;
        if (viewport()->style()->styleHint(QStyle::SH_RubberBand_Mask, &option, viewport(), &mask)) {
            // painter clipping for masked rubberbands
            painter.setClipRegion(mask.region, Qt::IntersectClip);
        }

        viewport()->style()->drawControl(QStyle::CE_RubberBand, &option, &painter, viewport());
    }
#endif

    painter.end();

    // Restore painter state protection.
    d->scene->d_func()->painterStateProtection = true;
}

/*!
    \reimp
*/
void QGraphicsView::resizeEvent(QResizeEvent *event)
{
    Q_D(QGraphicsView);
    // Save the last center point - the resize may scroll the view, which
    // changes the center point.
    QPointF oldLastCenterPoint = d->lastCenterPoint;

    QAbstractScrollArea::resizeEvent(event);
    d->recalculateContentSize();

    // Restore the center point again.
    if (d->resizeAnchor == NoAnchor && !d->keepLastCenterPoint) {
        d->updateLastCenterPoint();
    } else {
        d->lastCenterPoint = oldLastCenterPoint;
    }
    d->centerView(d->resizeAnchor);
    d->keepLastCenterPoint = false;

    if (d->cacheMode & CacheBackground) {
        // Invalidate the background pixmap
        d->mustResizeBackgroundPixmap = true;
    }
}

/*!
    \reimp
*/
void QGraphicsView::scrollContentsBy(int dx, int dy)
{
    Q_D(QGraphicsView);
    d->dirtyScroll = true;
    if (d->transforming)
        return;
    if (isRightToLeft())
        dx = -dx;

    if (d->viewportUpdateMode != QGraphicsView::NoViewportUpdate) {
        if (d->viewportUpdateMode != QGraphicsView::FullViewportUpdate) {
            if (d->accelerateScrolling) {
#ifndef QT_NO_RUBBERBAND
                // Update new and old rubberband regions
                if (!d->rubberBandRect.isEmpty()) {
                    QRegion rubberBandRegion(d->rubberBandRegion(viewport(), d->rubberBandRect));
                    rubberBandRegion += rubberBandRegion.translated(-dx, -dy);
                    viewport()->update(rubberBandRegion);
                }
#endif
                d->dirtyScrollOffset.rx() += dx;
                d->dirtyScrollOffset.ry() += dy;
                d->dirtyRegion.translate(dx, dy);
                viewport()->scroll(dx, dy);
            } else {
                d->updateAll();
            }
        } else {
            d->updateAll();
        }
    }

    d->updateLastCenterPoint();

    if ((d->cacheMode & CacheBackground)
#ifdef Q_WS_X11
        && X11->use_xrender
#endif
        ) {
        // Scroll the background pixmap
        QRegion exposed;
        if (!d->backgroundPixmap.isNull())
            d->backgroundPixmap.scroll(dx, dy, d->backgroundPixmap.rect(), &exposed);

        // Invalidate the background pixmap
        d->backgroundPixmapExposed.translate(dx, dy);
        d->backgroundPixmapExposed += exposed;
    }

    // Always replay on scroll.
    if (d->sceneInteractionAllowed)
        d->replayLastMouseEvent();
}

/*!
    \reimp
*/
void QGraphicsView::showEvent(QShowEvent *event)
{
    Q_D(QGraphicsView);
    d->recalculateContentSize();
    d->centerView(d->transformationAnchor);
    QAbstractScrollArea::showEvent(event);
}

/*!
    \reimp
*/
void QGraphicsView::inputMethodEvent(QInputMethodEvent *event)
{
    Q_D(QGraphicsView);
    if (d->scene)
        QApplication::sendEvent(d->scene, event);
}

/*!
    Draws the background of the scene using \a painter, before any items and
    the foreground are drawn. Reimplement this function to provide a custom
    background for this view.

    If all you want is to define a color, texture or gradient for the
    background, you can call setBackgroundBrush() instead.

    All painting is done in \e scene coordinates. \a rect is the exposed
    rectangle.

    The default implementation fills \a rect using the view's backgroundBrush.
    If no such brush is defined (the default), the scene's drawBackground()
    function is called instead.

    \sa drawForeground(), QGraphicsScene::drawBackground()
*/
void QGraphicsView::drawBackground(QPainter *painter, const QRectF &rect)
{
    Q_D(QGraphicsView);
    if (d->scene && d->backgroundBrush.style() == Qt::NoBrush) {
        d->scene->drawBackground(painter, rect);
        return;
    }

    painter->fillRect(rect, d->backgroundBrush);
}

/*!
    Draws the foreground of the scene using \a painter, after the background
    and all items are drawn. Reimplement this function to provide a custom
    foreground for this view.

    If all you want is to define a color, texture or gradient for the
    foreground, you can call setForegroundBrush() instead.

    All painting is done in \e scene coordinates. \a rect is the exposed
    rectangle.

    The default implementation fills \a rect using the view's foregroundBrush.
    If no such brush is defined (the default), the scene's drawForeground()
    function is called instead.

    \sa drawBackground(), QGraphicsScene::drawForeground()
*/
void QGraphicsView::drawForeground(QPainter *painter, const QRectF &rect)
{
    Q_D(QGraphicsView);
    if (d->scene && d->foregroundBrush.style() == Qt::NoBrush) {
        d->scene->drawForeground(painter, rect);
        return;
    }

    painter->fillRect(rect, d->foregroundBrush);
}

/*!
    \obsolete

    Draws the items \a items in the scene using \a painter, after the
    background and before the foreground are drawn. \a numItems is the number
    of items in \a items and options in \a options. \a options is a list of
    styleoptions; one for each item. Reimplement this function to provide
    custom item drawing for this view.

    The default implementation calls the scene's drawItems() function.

    Since Qt 4.6, this function is not called anymore unless
    the QGraphicsView::IndirectPainting flag is given as an Optimization
    flag.

    \sa drawForeground(), drawBackground(), QGraphicsScene::drawItems()
*/
void QGraphicsView::drawItems(QPainter *painter, int numItems,
                              QGraphicsItem *items[],
                              const QStyleOptionGraphicsItem options[])
{
    Q_D(QGraphicsView);
    if (d->scene) {
        QWidget *widget = painter->device() == viewport() ? viewport() : 0;
        d->scene->drawItems(painter, numItems, items, options, widget);
    }
}

/*!
    Returns the current transformation matrix for the view. If no current
    transformation is set, the identity matrix is returned.

    \sa setTransform(), rotate(), scale(), shear(), translate()
*/
QTransform QGraphicsView::transform() const
{
    Q_D(const QGraphicsView);
    return d->matrix;
}

/*!
    Returns a matrix that maps viewport coordinates to scene coordinates.

    \sa mapToScene(), mapFromScene()
*/
QTransform QGraphicsView::viewportTransform() const
{
    Q_D(const QGraphicsView);
    QTransform moveMatrix = QTransform::fromTranslate(-d->horizontalScroll(), -d->verticalScroll());
    return d->identityMatrix ? moveMatrix : d->matrix * moveMatrix;
}

/*!
    \since 4.6

    Returns true if the view is transformed (i.e., a non-identity transform
    has been assigned, or the scrollbars are adjusted).

    \sa setTransform(), horizontalScrollBar(), verticalScrollBar()
*/
bool QGraphicsView::isTransformed() const
{
    Q_D(const QGraphicsView);
    return !d->identityMatrix || d->horizontalScroll() || d->verticalScroll();
}

/*!
    Sets the view's current transformation matrix to \a matrix.

    If \a combine is true, then \a matrix is combined with the current matrix;
    otherwise, \a matrix \e replaces the current matrix. \a combine is false
    by default.

    The transformation matrix tranforms the scene into view coordinates. Using
    the default transformation, provided by the identity matrix, one pixel in
    the view represents one unit in the scene (e.g., a 10x10 rectangular item
    is drawn using 10x10 pixels in the view). If a 2x2 scaling matrix is
    applied, the scene will be drawn in 1:2 (e.g., a 10x10 rectangular item is
    then drawn using 20x20 pixels in the view).

    Example:

    \snippet doc/src/snippets/code/src_gui_graphicsview_qgraphicsview.cpp 7

    To simplify interation with items using a transformed view, QGraphicsView
    provides mapTo... and mapFrom... functions that can translate between
    scene and view coordinates. For example, you can call mapToScene() to map
    a view coordiate to a floating point scene coordinate, or mapFromScene()
    to map from floating point scene coordinates to view coordinates.

    \sa transform(), rotate(), scale(), shear(), translate()
*/
void QGraphicsView::setTransform(const QTransform &matrix, bool combine )
{
    Q_D(QGraphicsView);
    QTransform oldMatrix = d->matrix;
    if (!combine)
        d->matrix = matrix;
    else
        d->matrix = matrix * d->matrix;
    if (oldMatrix == d->matrix)
        return;

    d->identityMatrix = d->matrix.isIdentity();
    d->transforming = true;
    if (d->scene) {
        d->recalculateContentSize();
        d->centerView(d->transformationAnchor);
    } else {
        d->updateLastCenterPoint();
    }

    if (d->sceneInteractionAllowed)
        d->replayLastMouseEvent();
    d->transforming = false;

    // Any matrix operation requires a full update.
    d->updateAll();
}

/*!
    Resets the view transformation to the identity matrix.

    \sa transform(), setTransform()
*/
void QGraphicsView::resetTransform()
{
    setTransform(QTransform());
}

QPointF QGraphicsViewPrivate::mapToScene(const QPointF &point) const
{
    QPointF p = point;
    p.rx() += horizontalScroll();
    p.ry() += verticalScroll();
    return identityMatrix ? p : matrix.inverted().map(p);
}

QRectF QGraphicsViewPrivate::mapToScene(const QRectF &rect) const
{
    QPointF scrollOffset(horizontalScroll(), verticalScroll());
    QPointF tl = scrollOffset + rect.topLeft();
    QPointF tr = scrollOffset + rect.topRight();
    QPointF br = scrollOffset + rect.bottomRight();
    QPointF bl = scrollOffset + rect.bottomLeft();

    QPolygonF poly(4);
    if (!identityMatrix) {
        QTransform x = matrix.inverted();
        poly[0] = x.map(tl);
        poly[1] = x.map(tr);
        poly[2] = x.map(br);
        poly[3] = x.map(bl);
    } else {
        poly[0] = tl;
        poly[1] = tr;
        poly[2] = br;
        poly[3] = bl;
    }
    return poly.boundingRect();
}

QT_END_NAMESPACE

#include "moc_qgraphicsview.cpp"

#endif // QT_NO_GRAPHICSVIEW
