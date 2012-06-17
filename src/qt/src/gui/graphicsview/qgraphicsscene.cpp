/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtGui module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

/*!
    \class QGraphicsScene
    \brief The QGraphicsScene class provides a surface for managing a large
    number of 2D graphical items.
    \since 4.2
    \ingroup graphicsview-api


    The class serves as a container for QGraphicsItems. It is used together
    with QGraphicsView for visualizing graphical items, such as lines,
    rectangles, text, or even custom items, on a 2D surface. QGraphicsScene is
    part of the \l{Graphics View Framework}.

    QGraphicsScene also provides functionality that lets you efficiently
    determine both the location of items, and for determining what items are
    visible within an arbitrary area on the scene. With the QGraphicsView
    widget, you can either visualize the whole scene, or zoom in and view only
    parts of the scene.

    Example:

    \snippet doc/src/snippets/code/src_gui_graphicsview_qgraphicsscene.cpp 0

    Note that QGraphicsScene has no visual appearance of its own; it only
    manages the items. You need to create a QGraphicsView widget to visualize
    the scene.

    To add items to a scene, you start off by constructing a QGraphicsScene
    object. Then, you have two options: either add your existing QGraphicsItem
    objects by calling addItem(), or you can call one of the convenience
    functions addEllipse(), addLine(), addPath(), addPixmap(), addPolygon(),
    addRect(), or addText(), which all return a pointer to the newly added item.
    The dimensions of the items added with these functions are relative to the
    item's coordinate system, and the items position is initialized to (0,
    0) in the scene.

    You can then visualize the scene using QGraphicsView. When the scene
    changes, (e.g., when an item moves or is transformed) QGraphicsScene
    emits the changed() signal. To remove an item, call removeItem().

    QGraphicsScene uses an indexing algorithm to manage the location of items
    efficiently. By default, a BSP (Binary Space Partitioning) tree is used; an
    algorithm suitable for large scenes where most items remain static (i.e.,
    do not move around). You can choose to disable this index by calling
    setItemIndexMethod(). For more information about the available indexing
    algorithms, see the itemIndexMethod property.

    The scene's bounding rect is set by calling setSceneRect(). Items can be
    placed at any position on the scene, and the size of the scene is by
    default unlimited. The scene rect is used only for internal bookkeeping,
    maintaining the scene's item index. If the scene rect is unset,
    QGraphicsScene will use the bounding area of all items, as returned by
    itemsBoundingRect(), as the scene rect. However, itemsBoundingRect() is a
    relatively time consuming function, as it operates by collecting
    positional information for every item on the scene. Because of this, you
    should always set the scene rect when operating on large scenes.

    One of QGraphicsScene's greatest strengths is its ability to efficiently
    determine the location of items. Even with millions of items on the scene,
    the items() functions can determine the location of an item within few
    milliseconds. There are several overloads to items(): one that finds items
    at a certain position, one that finds items inside or intersecting with a
    polygon or a rectangle, and more. The list of returned items is sorted by
    stacking order, with the topmost item being the first item in the list.
    For convenience, there is also an itemAt() function that returns the
    topmost item at a given position.

    QGraphicsScene maintains selection information for the scene. To select
    items, call setSelectionArea(), and to clear the current selection, call
    clearSelection(). Call selectedItems() to get the list of all selected
    items.

    \section1 Event Handling and Propagation

    Another responsibility that QGraphicsScene has, is to propagate events
    from QGraphicsView. To send an event to a scene, you construct an event
    that inherits QEvent, and then send it using, for example,
    QApplication::sendEvent(). event() is responsible for dispatching
    the event to the individual items. Some common events are handled by
    convenience event handlers. For example, key press events are handled by
    keyPressEvent(), and mouse press events are handled by mousePressEvent().

    Key events are delivered to the \e {focus item}. To set the focus item,
    you can either call setFocusItem(), passing an item that accepts focus, or
    the item itself can call QGraphicsItem::setFocus().  Call focusItem() to
    get the current focus item. For compatibility with widgets, the scene also
    maintains its own focus information. By default, the scene does not have
    focus, and all key events are discarded. If setFocus() is called, or if an
    item on the scene gains focus, the scene automatically gains focus. If the
    scene has focus, hasFocus() will return true, and key events will be
    forwarded to the focus item, if any. If the scene loses focus, (i.e.,
    someone calls clearFocus()) while an item has focus, the scene will
    maintain its item focus information, and once the scene regains focus, it
    will make sure the last focus item regains focus.

    For mouse-over effects, QGraphicsScene dispatches \e {hover
    events}. If an item accepts hover events (see
    QGraphicsItem::acceptHoverEvents()), it will receive a \l
    {QEvent::}{GraphicsSceneHoverEnter} event when the mouse enters
    its area. As the mouse continues moving inside the item's area,
    QGraphicsScene will send it \l {QEvent::}{GraphicsSceneHoverMove}
    events. When the mouse leaves the item's area, the item will
    receive a \l {QEvent::}{GraphicsSceneHoverLeave} event.

    All mouse events are delivered to the current \e {mouse grabber}
    item. An item becomes the scene's mouse grabber if it accepts
    mouse events (see QGraphicsItem::acceptedMouseButtons()) and it
    receives a mouse press. It stays the mouse grabber until it
    receives a mouse release when no other mouse buttons are
    pressed. You can call mouseGrabberItem() to determine what item is
    currently grabbing the mouse.

    \sa QGraphicsItem, QGraphicsView
*/

/*!
    \enum QGraphicsScene::SceneLayer
    \since 4.3

    This enum describes the rendering layers in a QGraphicsScene. When
    QGraphicsScene draws the scene contents, it renders each of these layers
    separately, in order.

    Each layer represents a flag that can be OR'ed together when calling
    functions such as invalidate() or QGraphicsView::invalidateScene().

    \value ItemLayer The item layer. QGraphicsScene renders all items are in
    this layer by calling the virtual function drawItems(). The item layer is
    drawn after the background layer, but before the foreground layer.

    \value BackgroundLayer The background layer. QGraphicsScene renders the
    scene's background in this layer by calling the virtual function
    drawBackground(). The background layer is drawn first of all layers.

    \value ForegroundLayer The foreground layer. QGraphicsScene renders the
    scene's foreground in this layer by calling the virtual function
    drawForeground().  The foreground layer is drawn last of all layers.

    \value AllLayers All layers; this value represents a combination of all
    three layers.

    \sa invalidate(), QGraphicsView::invalidateScene()
*/

/*!
    \enum QGraphicsScene::ItemIndexMethod

    This enum describes the indexing algorithms QGraphicsScene provides for
    managing positional information about items on the scene.

    \value BspTreeIndex A Binary Space Partitioning tree is applied. All
    QGraphicsScene's item location algorithms are of an order close to
    logarithmic complexity, by making use of binary search. Adding, moving and
    removing items is logarithmic. This approach is best for static scenes
    (i.e., scenes where most items do not move).

    \value NoIndex No index is applied. Item location is of linear complexity,
    as all items on the scene are searched. Adding, moving and removing items,
    however, is done in constant time. This approach is ideal for dynamic
    scenes, where many items are added, moved or removed continuously.

    \sa setItemIndexMethod(), bspTreeDepth
*/

#include "qgraphicsscene.h"

#ifndef QT_NO_GRAPHICSVIEW

#include "qgraphicsitem.h"
#include "qgraphicsitem_p.h"
#include "qgraphicslayout.h"
#include "qgraphicsscene_p.h"
#include "qgraphicssceneevent.h"
#include "qgraphicsview.h"
#include "qgraphicsview_p.h"
#include "qgraphicswidget.h"
#include "qgraphicswidget_p.h"
#include "qgraphicssceneindex_p.h"
#include "qgraphicsscenebsptreeindex_p.h"
#include "qgraphicsscenelinearindex_p.h"

#include <QtCore/qdebug.h>
#include <QtCore/qlist.h>
#include <QtCore/qmath.h>
#include <QtCore/qrect.h>
#include <QtCore/qset.h>
#include <QtCore/qstack.h>
#include <QtCore/qtimer.h>
#include <QtCore/qvarlengtharray.h>
#include <QtCore/QMetaMethod>
#include <QtGui/qapplication.h>
#include <QtGui/qdesktopwidget.h>
#include <QtGui/qevent.h>
#include <QtGui/qgraphicslayout.h>
#include <QtGui/qgraphicsproxywidget.h>
#include <QtGui/qgraphicswidget.h>
#include <QtGui/qmatrix.h>
#include <QtGui/qpaintengine.h>
#include <QtGui/qpainter.h>
#include <QtGui/qpixmapcache.h>
#include <QtGui/qpolygon.h>
#include <QtGui/qstyleoption.h>
#include <QtGui/qtooltip.h>
#include <QtGui/qtransform.h>
#include <QtGui/qinputcontext.h>
#include <QtGui/qgraphicseffect.h>
#ifndef QT_NO_ACCESSIBILITY
# include <QtGui/qaccessible.h>
#endif

#include <private/qapplication_p.h>
#include <private/qobject_p.h>
#ifdef Q_WS_X11
#include <private/qt_x11_p.h>
#endif
#include <private/qgraphicseffect_p.h>
#include <private/qgesturemanager_p.h>
#include <private/qpathclipper_p.h>

// #define GESTURE_DEBUG
#ifndef GESTURE_DEBUG
# define G_DEBUG if (0) qDebug
#else
# define G_DEBUG qDebug
#endif

QT_BEGIN_NAMESPACE

bool qt_sendSpontaneousEvent(QObject *receiver, QEvent *event);

static void _q_hoverFromMouseEvent(QGraphicsSceneHoverEvent *hover, const QGraphicsSceneMouseEvent *mouseEvent)
{
    hover->setWidget(mouseEvent->widget());
    hover->setPos(mouseEvent->pos());
    hover->setScenePos(mouseEvent->scenePos());
    hover->setScreenPos(mouseEvent->screenPos());
    hover->setLastPos(mouseEvent->lastPos());
    hover->setLastScenePos(mouseEvent->lastScenePos());
    hover->setLastScreenPos(mouseEvent->lastScreenPos());
    hover->setModifiers(mouseEvent->modifiers());
    hover->setAccepted(mouseEvent->isAccepted());
}

/*!
    \internal
*/
QGraphicsScenePrivate::QGraphicsScenePrivate()
    : indexMethod(QGraphicsScene::BspTreeIndex),
      index(0),
      lastItemCount(0),
      hasSceneRect(false),
      dirtyGrowingItemsBoundingRect(true),
      updateAll(false),
      calledEmitUpdated(false),
      processDirtyItemsEmitted(false),
      needSortTopLevelItems(true),
      holesInTopLevelSiblingIndex(false),
      topLevelSequentialOrdering(true),
      scenePosDescendantsUpdatePending(false),
      stickyFocus(false),
      hasFocus(false),
      lastMouseGrabberItemHasImplicitMouseGrab(false),
      allItemsIgnoreHoverEvents(true),
      allItemsUseDefaultCursor(true),
      painterStateProtection(true),
      sortCacheEnabled(false),
      allItemsIgnoreTouchEvents(true),
      selectionChanging(0),
      rectAdjust(2),
      focusItem(0),
      lastFocusItem(0),
      passiveFocusItem(0),
      tabFocusFirst(0),
      activePanel(0),
      lastActivePanel(0),
      activationRefCount(0),
      childExplicitActivation(0),
      lastMouseGrabberItem(0),
      dragDropItem(0),
      enterWidget(0),
      lastDropAction(Qt::IgnoreAction),
      style(0)
{
}

/*!
    \internal
*/
void QGraphicsScenePrivate::init()
{
    Q_Q(QGraphicsScene);

    index = new QGraphicsSceneBspTreeIndex(q);

    // Keep this index so we can check for connected slots later on.
    changedSignalIndex = signalIndex("changed(QList<QRectF>)");
    processDirtyItemsIndex = q->metaObject()->indexOfSlot("_q_processDirtyItems()");
    polishItemsIndex = q->metaObject()->indexOfSlot("_q_polishItems()");

    qApp->d_func()->scene_list.append(q);
    q->update();
}

/*!
    \internal
*/
QGraphicsScenePrivate *QGraphicsScenePrivate::get(QGraphicsScene *q)
{
    return q->d_func();
}

void QGraphicsScenePrivate::_q_emitUpdated()
{
    Q_Q(QGraphicsScene);
    calledEmitUpdated = false;

    if (dirtyGrowingItemsBoundingRect) {
        if (!hasSceneRect) {
            const QRectF oldGrowingItemsBoundingRect = growingItemsBoundingRect;
            growingItemsBoundingRect |= q->itemsBoundingRect();
            if (oldGrowingItemsBoundingRect != growingItemsBoundingRect)
                emit q->sceneRectChanged(growingItemsBoundingRect);
        }
        dirtyGrowingItemsBoundingRect = false;
    }

    // Ensure all views are connected if anything is connected. This disables
    // the optimization that items send updates directly to the views, but it
    // needs to happen in order to keep compatibility with the behavior from
    // Qt 4.4 and backward.
    if (isSignalConnected(changedSignalIndex)) {
        for (int i = 0; i < views.size(); ++i) {
            QGraphicsView *view = views.at(i);
            if (!view->d_func()->connectedToScene) {
                view->d_func()->connectedToScene = true;
                q->connect(q, SIGNAL(changed(QList<QRectF>)),
                           views.at(i), SLOT(updateScene(QList<QRectF>)));
            }
        }
    } else {
        if (views.isEmpty()) {
            updateAll = false;
            return;
        }
        for (int i = 0; i < views.size(); ++i)
            views.at(i)->d_func()->processPendingUpdates();
        // It's important that we update all views before we dispatch, hence two for-loops.
        for (int i = 0; i < views.size(); ++i)
            views.at(i)->d_func()->dispatchPendingUpdateRequests();
        return;
    }

    // Notify the changes to anybody interested.
    QList<QRectF> oldUpdatedRects;
    oldUpdatedRects = updateAll ? (QList<QRectF>() << q->sceneRect()) : updatedRects;
    updateAll = false;
    updatedRects.clear();
    emit q->changed(oldUpdatedRects);
}

/*!
    \internal

    ### This function is almost identical to QGraphicsItemPrivate::addChild().
*/
void QGraphicsScenePrivate::registerTopLevelItem(QGraphicsItem *item)
{
    item->d_ptr->ensureSequentialSiblingIndex();
    needSortTopLevelItems = true; // ### maybe false
    item->d_ptr->siblingIndex = topLevelItems.size();
    topLevelItems.append(item);
}

/*!
    \internal

    ### This function is almost identical to QGraphicsItemPrivate::removeChild().
*/
void QGraphicsScenePrivate::unregisterTopLevelItem(QGraphicsItem *item)
{
    if (!holesInTopLevelSiblingIndex)
        holesInTopLevelSiblingIndex = item->d_ptr->siblingIndex != topLevelItems.size() - 1;
    if (topLevelSequentialOrdering && !holesInTopLevelSiblingIndex)
        topLevelItems.removeAt(item->d_ptr->siblingIndex);
    else
        topLevelItems.removeOne(item);
    // NB! Do not use topLevelItems.removeAt(item->d_ptr->siblingIndex) because
    // the item is not guaranteed to be at the index after the list is sorted
    // (see ensureSortedTopLevelItems()).
    item->d_ptr->siblingIndex = -1;
    if (topLevelSequentialOrdering)
        topLevelSequentialOrdering = !holesInTopLevelSiblingIndex;
}

/*!
    \internal
*/
void QGraphicsScenePrivate::_q_polishItems()
{
    if (unpolishedItems.isEmpty())
        return;

    const QVariant booleanTrueVariant(true);
    QGraphicsItem *item = 0;
    QGraphicsItemPrivate *itemd = 0;
    const int oldUnpolishedCount = unpolishedItems.count();

    for (int i = 0; i < oldUnpolishedCount; ++i) {
        item = unpolishedItems.at(i);
        if (!item)
            continue;
        itemd = item->d_ptr.data();
        itemd->pendingPolish = false;
        if (!itemd->explicitlyHidden) {
            item->itemChange(QGraphicsItem::ItemVisibleChange, booleanTrueVariant);
            item->itemChange(QGraphicsItem::ItemVisibleHasChanged, booleanTrueVariant);
        }
        if (itemd->isWidget) {
            QEvent event(QEvent::Polish);
            QApplication::sendEvent((QGraphicsWidget *)item, &event);
        }
    }

    if (unpolishedItems.count() == oldUnpolishedCount) {
        // No new items were added to the vector.
        unpolishedItems.clear();
    } else {
        // New items were appended; keep them and remove the old ones.
        unpolishedItems.remove(0, oldUnpolishedCount);
        unpolishedItems.squeeze();
        QMetaObject::invokeMethod(q_ptr, "_q_polishItems", Qt::QueuedConnection);
    }
}

void QGraphicsScenePrivate::_q_processDirtyItems()
{
    processDirtyItemsEmitted = false;

    if (updateAll) {
        Q_ASSERT(calledEmitUpdated);
        // No need for further processing (except resetting the dirty states).
        // The growingItemsBoundingRect is updated in _q_emitUpdated.
        for (int i = 0; i < topLevelItems.size(); ++i)
            resetDirtyItem(topLevelItems.at(i), /*recursive=*/true);
        return;
    }

    const bool wasPendingSceneUpdate = calledEmitUpdated;
    const QRectF oldGrowingItemsBoundingRect = growingItemsBoundingRect;

    // Process items recursively.
    for (int i = 0; i < topLevelItems.size(); ++i)
        processDirtyItemsRecursive(topLevelItems.at(i));

    dirtyGrowingItemsBoundingRect = false;
    if (!hasSceneRect && oldGrowingItemsBoundingRect != growingItemsBoundingRect)
        emit q_func()->sceneRectChanged(growingItemsBoundingRect);

    if (wasPendingSceneUpdate)
        return;

    for (int i = 0; i < views.size(); ++i)
        views.at(i)->d_func()->processPendingUpdates();

    if (calledEmitUpdated) {
        // We did a compatibility QGraphicsScene::update in processDirtyItemsRecursive
        // and we cannot wait for the control to reach the eventloop before the
        // changed signal is emitted, so we emit it now.
        _q_emitUpdated();
    }

    // Immediately dispatch all pending update requests on the views.
    for (int i = 0; i < views.size(); ++i)
        views.at(i)->d_func()->dispatchPendingUpdateRequests();
}

/*!
    \internal
*/
void QGraphicsScenePrivate::setScenePosItemEnabled(QGraphicsItem *item, bool enabled)
{
    QGraphicsItem *p = item->d_ptr->parent;
    while (p) {
        p->d_ptr->scenePosDescendants = enabled;
        p = p->d_ptr->parent;
    }
    if (!enabled && !scenePosDescendantsUpdatePending) {
        scenePosDescendantsUpdatePending = true;
        QMetaObject::invokeMethod(q_func(), "_q_updateScenePosDescendants", Qt::QueuedConnection);
    }
}

/*!
    \internal
*/
void QGraphicsScenePrivate::registerScenePosItem(QGraphicsItem *item)
{
    scenePosItems.insert(item);
    setScenePosItemEnabled(item, true);
}

/*!
    \internal
*/
void QGraphicsScenePrivate::unregisterScenePosItem(QGraphicsItem *item)
{
    scenePosItems.remove(item);
    setScenePosItemEnabled(item, false);
}

/*!
    \internal
*/
void QGraphicsScenePrivate::_q_updateScenePosDescendants()
{
    foreach (QGraphicsItem *item, scenePosItems) {
        QGraphicsItem *p = item->d_ptr->parent;
        while (p) {
            p->d_ptr->scenePosDescendants = 1;
            p = p->d_ptr->parent;
        }
    }
    scenePosDescendantsUpdatePending = false;
}

/*!
    \internal

    Schedules an item for removal. This function leaves some stale indexes
    around in the BSP tree if called from the item's destructor; these will
    be cleaned up the next time someone triggers purgeRemovedItems().

    Note: This function might get called from QGraphicsItem's destructor. \a item is
    being destroyed, so we cannot call any pure virtual functions on it (such
    as boundingRect()). Also, it is unnecessary to update the item's own state
    in any way.
*/
void QGraphicsScenePrivate::removeItemHelper(QGraphicsItem *item)
{
    Q_Q(QGraphicsScene);

    // Clear focus on the item to remove any reference in the focusWidget chain.
    item->clearFocus();

    markDirty(item, QRectF(), /*invalidateChildren=*/false, /*force=*/false,
              /*ignoreOpacity=*/false, /*removingItemFromScene=*/true);

    if (item->d_ptr->inDestructor) {
        // The item is actually in its destructor, we call the special method in the index.
        index->deleteItem(item);
    } else {
        // Can potentially call item->boundingRect() (virtual function), that's why
        // we only can call this function if the item is not in its destructor.
        index->removeItem(item);
    }

    item->d_ptr->clearSubFocus();

    if (item->flags() & QGraphicsItem::ItemSendsScenePositionChanges)
        unregisterScenePosItem(item);

    QGraphicsScene *oldScene = item->d_func()->scene;
    item->d_func()->scene = 0;

    //We need to remove all children first because they might use their parent
    //attributes (e.g. sceneTransform).
    if (!item->d_ptr->inDestructor) {
        // Remove all children recursively
        for (int i = 0; i < item->d_ptr->children.size(); ++i)
            q->removeItem(item->d_ptr->children.at(i));
    }

    if (!item->d_ptr->inDestructor && item == tabFocusFirst) {
        QGraphicsWidget *widget = static_cast<QGraphicsWidget *>(item);
        widget->d_func()->fixFocusChainBeforeReparenting(0, oldScene, 0);
    }

    // Unregister focus proxy.
    item->d_ptr->resetFocusProxy();

    // Remove from parent, or unregister from toplevels.
    if (QGraphicsItem *parentItem = item->parentItem()) {
        if (parentItem->scene()) {
            Q_ASSERT_X(parentItem->scene() == q, "QGraphicsScene::removeItem",
                       "Parent item's scene is different from this item's scene");
            item->setParentItem(0);
        }
    } else {
        unregisterTopLevelItem(item);
    }

    // Reset the mouse grabber and focus item data.
    if (item == focusItem)
        focusItem = 0;
    if (item == lastFocusItem)
        lastFocusItem = 0;
    if (item == passiveFocusItem)
        passiveFocusItem = 0;
    if (item == activePanel) {
        // ### deactivate...
        activePanel = 0;
    }
    if (item == lastActivePanel)
        lastActivePanel = 0;

    // Cancel active touches
    {
        QMap<int, QGraphicsItem *>::iterator it = itemForTouchPointId.begin();
        while (it != itemForTouchPointId.end()) {
            if (it.value() == item) {
                sceneCurrentTouchPoints.remove(it.key());
                it = itemForTouchPointId.erase(it);
            } else {
                ++it;
            }
        }
    }

    // Disable selectionChanged() for individual items
    ++selectionChanging;
    int oldSelectedItemsSize = selectedItems.size();

    // Update selected & hovered item bookkeeping
    selectedItems.remove(item);
    hoverItems.removeAll(item);
    cachedItemsUnderMouse.removeAll(item);
    if (item->d_ptr->pendingPolish) {
        const int unpolishedIndex = unpolishedItems.indexOf(item);
        if (unpolishedIndex != -1)
            unpolishedItems[unpolishedIndex] = 0;
        item->d_ptr->pendingPolish = false;
    }
    resetDirtyItem(item);

    //We remove all references of item from the sceneEventFilter arrays
    QMultiMap<QGraphicsItem*, QGraphicsItem*>::iterator iterator = sceneEventFilters.begin();
    while (iterator != sceneEventFilters.end()) {
        if (iterator.value() == item || iterator.key() == item)
            iterator = sceneEventFilters.erase(iterator);
        else
            ++iterator;
    }

    if (item->isPanel() && item->isVisible() && item->panelModality() != QGraphicsItem::NonModal)
        leaveModal(item);

    // Reset the mouse grabber and focus item data.
    if (mouseGrabberItems.contains(item))
        ungrabMouse(item, /* item is dying */ item->d_ptr->inDestructor);

    // Reset the keyboard grabber
    if (keyboardGrabberItems.contains(item))
        ungrabKeyboard(item, /* item is dying */ item->d_ptr->inDestructor);

    // Reset the last mouse grabber item
    if (item == lastMouseGrabberItem)
        lastMouseGrabberItem = 0;

    // Reset the current drop item
    if (item == dragDropItem)
        dragDropItem = 0;

    // Reenable selectionChanged() for individual items
    --selectionChanging;
    if (!selectionChanging && selectedItems.size() != oldSelectedItemsSize)
        emit q->selectionChanged();

#ifndef QT_NO_GESTURES
    QHash<QGesture *, QGraphicsObject *>::iterator it;
    for (it = gestureTargets.begin(); it != gestureTargets.end();) {
        if (it.value() == item)
            it = gestureTargets.erase(it);
        else
            ++it;
    }

    QGraphicsObject *dummy = static_cast<QGraphicsObject *>(item);
    cachedTargetItems.removeOne(dummy);
    cachedItemGestures.remove(dummy);
    cachedAlreadyDeliveredGestures.remove(dummy);

    foreach (Qt::GestureType gesture, item->d_ptr->gestureContext.keys())
        ungrabGesture(item, gesture);
#endif // QT_NO_GESTURES
}

/*!
    \internal
*/
void QGraphicsScenePrivate::setActivePanelHelper(QGraphicsItem *item, bool duringActivationEvent)
{
    Q_Q(QGraphicsScene);
    if (item && item->scene() != q) {
        qWarning("QGraphicsScene::setActivePanel: item %p must be part of this scene",
                 item);
        return;
    }

    // Ensure the scene has focus when we change panel activation.
    q->setFocus(Qt::ActiveWindowFocusReason);

    // Find the item's panel.
    QGraphicsItem *panel = item ? item->panel() : 0;
    lastActivePanel = panel ? activePanel : 0;
    if (panel == activePanel || (!q->isActive() && !duringActivationEvent))
        return;

    // Deactivate the last active panel.
    if (activePanel) {
        if (QGraphicsItem *fi = activePanel->focusItem()) {
            // Remove focus from the current focus item.
            if (fi == q->focusItem())
                q->setFocusItem(0, Qt::ActiveWindowFocusReason);
        }

        QEvent event(QEvent::WindowDeactivate);
        q->sendEvent(activePanel, &event);
    } else if (panel && !duringActivationEvent) {
        // Deactivate the scene if changing activation to a panel.
        QEvent event(QEvent::WindowDeactivate);
        foreach (QGraphicsItem *item, q->items()) {
            if (item->isVisible() && !item->isPanel() && !item->parentItem())
                q->sendEvent(item, &event);
        }
    }

    // Update activate state.
    activePanel = panel;
    QEvent event(QEvent::ActivationChange);
    QApplication::sendEvent(q, &event);

    // Activate
    if (panel) {
        QEvent event(QEvent::WindowActivate);
        q->sendEvent(panel, &event);

        // Set focus on the panel's focus item.
        if (QGraphicsItem *focusItem = panel->focusItem())
            focusItem->setFocus(Qt::ActiveWindowFocusReason);
    } else if (q->isActive()) {
        // Activate the scene
        QEvent event(QEvent::WindowActivate);
        foreach (QGraphicsItem *item, q->items()) {
            if (item->isVisible() && !item->isPanel() && !item->parentItem())
                q->sendEvent(item, &event);
        }
    }
}

/*!
    \internal
*/
void QGraphicsScenePrivate::setFocusItemHelper(QGraphicsItem *item,
                                               Qt::FocusReason focusReason)
{
    Q_Q(QGraphicsScene);
    if (item == focusItem)
        return;

    // Clear focus if asked to set focus on something that can't
    // accept input focus.
    if (item && (!(item->flags() & QGraphicsItem::ItemIsFocusable)
                 || !item->isVisible() || !item->isEnabled())) {
        item = 0;
    }

    // Set focus on the scene if an item requests focus.
    if (item) {
        q->setFocus(focusReason);
        if (item == focusItem)
            return;
    }

    if (focusItem) {
        lastFocusItem = focusItem;

#ifndef QT_NO_IM
        if (lastFocusItem
            && (lastFocusItem->flags() & QGraphicsItem::ItemAcceptsInputMethod)) {
            // Close any external input method panel. This happens
            // automatically by removing WA_InputMethodEnabled on
            // the views, but if we are changing focus, we have to
            // do it ourselves.
            for (int i = 0; i < views.size(); ++i)
                if (views.at(i)->inputContext())
                    views.at(i)->inputContext()->reset();
        }

        focusItem = 0;
        QFocusEvent event(QEvent::FocusOut, focusReason);
        sendEvent(lastFocusItem, &event);
#endif //QT_NO_IM
    }

    // This handles the case that the item has been removed from the
    // scene in response to the FocusOut event.
    if (item && item->scene() != q)
        item = 0;

    if (item)
        focusItem = item;
    updateInputMethodSensitivityInViews();

#ifndef QT_NO_ACCESSIBILITY
    if (focusItem) {
        if (QGraphicsObject *focusObj = focusItem->toGraphicsObject()) {
            QAccessible::updateAccessibility(focusObj, 0, QAccessible::Focus);
        }
    }
#endif
    if (item) {
        QFocusEvent event(QEvent::FocusIn, focusReason);
        sendEvent(item, &event);
    }
}

/*!
    \internal
*/
void QGraphicsScenePrivate::addPopup(QGraphicsWidget *widget)
{
    Q_ASSERT(widget);
    Q_ASSERT(!popupWidgets.contains(widget));
    popupWidgets << widget;
    if (QGraphicsWidget *focusWidget = widget->focusWidget()) {
        focusWidget->setFocus(Qt::PopupFocusReason);
    } else {
        grabKeyboard((QGraphicsItem *)widget);
        if (focusItem && popupWidgets.size() == 1) {
            QFocusEvent event(QEvent::FocusOut, Qt::PopupFocusReason);
            sendEvent(focusItem, &event);
        }
    }
    grabMouse((QGraphicsItem *)widget);
}

/*!
    \internal

    Remove \a widget from the popup list. Important notes:

    \a widget is guaranteed to be in the list of popups, but it might not be
    the last entry; you can hide any item in the pop list before the others,
    and this must cause all later mouse grabbers to lose the grab.
*/
void QGraphicsScenePrivate::removePopup(QGraphicsWidget *widget, bool itemIsDying)
{
    Q_ASSERT(widget);
    int index = popupWidgets.indexOf(widget);
    Q_ASSERT(index != -1);

    for (int i = popupWidgets.size() - 1; i >= index; --i) {
        QGraphicsWidget *widget = popupWidgets.takeLast();
        ungrabMouse(widget, itemIsDying);
        if (focusItem && popupWidgets.isEmpty()) {
            QFocusEvent event(QEvent::FocusIn, Qt::PopupFocusReason);
            sendEvent(focusItem, &event);
        } else if (keyboardGrabberItems.contains(static_cast<QGraphicsItem *>(widget))) {
            ungrabKeyboard(static_cast<QGraphicsItem *>(widget), itemIsDying);
        }
        if (!itemIsDying && widget->isVisible()) {
            widget->QGraphicsItem::d_ptr->setVisibleHelper(false, /* explicit = */ false);
        }
    }
}

/*!
    \internal
*/
void QGraphicsScenePrivate::grabMouse(QGraphicsItem *item, bool implicit)
{
    // Append to list of mouse grabber items, and send a mouse grab event.
    if (mouseGrabberItems.contains(item)) {
        if (mouseGrabberItems.last() == item) {
            Q_ASSERT(!implicit);
            if (!lastMouseGrabberItemHasImplicitMouseGrab) {
                qWarning("QGraphicsItem::grabMouse: already a mouse grabber");
            } else {
                // Upgrade to an explicit mouse grab
                lastMouseGrabberItemHasImplicitMouseGrab = false;
            }
        } else {
            qWarning("QGraphicsItem::grabMouse: already blocked by mouse grabber: %p",
                     mouseGrabberItems.last());
        }
        return;
    }

    // Send ungrab event to the last grabber.
    if (!mouseGrabberItems.isEmpty()) {
        QGraphicsItem *last = mouseGrabberItems.last();
        if (lastMouseGrabberItemHasImplicitMouseGrab) {
            // Implicit mouse grab is immediately lost.
            last->ungrabMouse();
        } else {
            // Just send ungrab event to current grabber.
            QEvent ungrabEvent(QEvent::UngrabMouse);
            sendEvent(last, &ungrabEvent);
        }
    }

    mouseGrabberItems << item;
    lastMouseGrabberItemHasImplicitMouseGrab = implicit;

    // Send grab event to current grabber.
    QEvent grabEvent(QEvent::GrabMouse);
    sendEvent(item, &grabEvent);
}

/*!
    \internal
*/
void QGraphicsScenePrivate::ungrabMouse(QGraphicsItem *item, bool itemIsDying)
{
    int index = mouseGrabberItems.indexOf(item);
    if (index == -1) {
        qWarning("QGraphicsItem::ungrabMouse: not a mouse grabber");
        return;
    }

    if (item != mouseGrabberItems.last()) {
        // Recursively ungrab the next mouse grabber until we reach this item
        // to ensure state consistency.
        ungrabMouse(mouseGrabberItems.at(index + 1), itemIsDying);
    }
    if (!popupWidgets.isEmpty() && item == popupWidgets.last()) {
        // If the item is a popup, go via removePopup to ensure state
        // consistency and that it gets hidden correctly - beware that
        // removePopup() reenters this function to continue removing the grab.
        removePopup((QGraphicsWidget *)item, itemIsDying);
        return;
    }

    // Send notification about mouse ungrab.
    if (!itemIsDying) {
        QEvent event(QEvent::UngrabMouse);
        sendEvent(item, &event);
    }

    // Remove the item from the list of grabbers. Whenever this happens, we
    // reset the implicitGrab (there can be only ever be one implicit grabber
    // in a scene, and it is always the latest grabber; if the implicit grab
    // is lost, it is not automatically regained.
    mouseGrabberItems.takeLast();
    lastMouseGrabberItemHasImplicitMouseGrab = false;

    // Send notification about mouse regrab. ### It's unfortunate that all the
    // items get a GrabMouse event, but this is a rare case with a simple
    // implementation and it does ensure a consistent state.
    if (!itemIsDying && !mouseGrabberItems.isEmpty()) {
        QGraphicsItem *last = mouseGrabberItems.last();
        QEvent event(QEvent::GrabMouse);
        sendEvent(last, &event);
    }
}

/*!
    \internal
*/
void QGraphicsScenePrivate::clearMouseGrabber()
{
    if (!mouseGrabberItems.isEmpty())
        mouseGrabberItems.first()->ungrabMouse();
    lastMouseGrabberItem = 0;
}

/*!
    \internal
*/
void QGraphicsScenePrivate::grabKeyboard(QGraphicsItem *item)
{
    if (keyboardGrabberItems.contains(item)) {
        if (keyboardGrabberItems.last() == item)
            qWarning("QGraphicsItem::grabKeyboard: already a keyboard grabber");
        else
            qWarning("QGraphicsItem::grabKeyboard: already blocked by keyboard grabber: %p",
                     keyboardGrabberItems.last());
        return;
    }

    // Send ungrab event to the last grabber.
    if (!keyboardGrabberItems.isEmpty()) {
        // Just send ungrab event to current grabber.
        QEvent ungrabEvent(QEvent::UngrabKeyboard);
        sendEvent(keyboardGrabberItems.last(), &ungrabEvent);
    }

    keyboardGrabberItems << item;

    // Send grab event to current grabber.
    QEvent grabEvent(QEvent::GrabKeyboard);
    sendEvent(item, &grabEvent);
}

/*!
    \internal
*/
void QGraphicsScenePrivate::ungrabKeyboard(QGraphicsItem *item, bool itemIsDying)
{
    int index = keyboardGrabberItems.lastIndexOf(item);
    if (index == -1) {
        qWarning("QGraphicsItem::ungrabKeyboard: not a keyboard grabber");
        return;
    }
    if (item != keyboardGrabberItems.last()) {
        // Recursively ungrab the topmost keyboard grabber until we reach this
        // item to ensure state consistency.
        ungrabKeyboard(keyboardGrabberItems.at(index + 1), itemIsDying);
    }

    // Send notification about keyboard ungrab.
    if (!itemIsDying) {
        QEvent event(QEvent::UngrabKeyboard);
        sendEvent(item, &event);
    }

    // Remove the item from the list of grabbers.
    keyboardGrabberItems.takeLast();

    // Send notification about mouse regrab.
    if (!itemIsDying && !keyboardGrabberItems.isEmpty()) {
        QGraphicsItem *last = keyboardGrabberItems.last();
        QEvent event(QEvent::GrabKeyboard);
        sendEvent(last, &event);
    }
}

/*!
    \internal
*/
void QGraphicsScenePrivate::clearKeyboardGrabber()
{
    if (!keyboardGrabberItems.isEmpty())
        ungrabKeyboard(keyboardGrabberItems.first());
}

void QGraphicsScenePrivate::enableMouseTrackingOnViews()
{
    foreach (QGraphicsView *view, views)
        view->viewport()->setMouseTracking(true);
}

/*!
    Returns all items for the screen position in \a event.
*/
QList<QGraphicsItem *> QGraphicsScenePrivate::itemsAtPosition(const QPoint &screenPos,
                                                              const QPointF &scenePos,
                                                              QWidget *widget) const
{
    Q_Q(const QGraphicsScene);
    QGraphicsView *view = widget ? qobject_cast<QGraphicsView *>(widget->parentWidget()) : 0;
    if (!view)
        return q->items(scenePos, Qt::IntersectsItemShape, Qt::DescendingOrder, QTransform());

    const QRectF pointRect(QPointF(widget->mapFromGlobal(screenPos)), QSizeF(1, 1));
    if (!view->isTransformed())
        return q->items(pointRect, Qt::IntersectsItemShape, Qt::DescendingOrder);

    const QTransform viewTransform = view->viewportTransform();
    if (viewTransform.type() <= QTransform::TxScale) {
        return q->items(viewTransform.inverted().mapRect(pointRect), Qt::IntersectsItemShape,
                        Qt::DescendingOrder, viewTransform);
    }
    return q->items(viewTransform.inverted().map(pointRect), Qt::IntersectsItemShape,
                    Qt::DescendingOrder, viewTransform);
}

/*!
    \internal
*/
void QGraphicsScenePrivate::storeMouseButtonsForMouseGrabber(QGraphicsSceneMouseEvent *event)
{
    for (int i = 0x1; i <= 0x10; i <<= 1) {
        if (event->buttons() & i) {
            mouseGrabberButtonDownPos.insert(Qt::MouseButton(i),
                                             mouseGrabberItems.last()->d_ptr->genericMapFromScene(event->scenePos(),
                                                                                                  event->widget()));
            mouseGrabberButtonDownScenePos.insert(Qt::MouseButton(i), event->scenePos());
            mouseGrabberButtonDownScreenPos.insert(Qt::MouseButton(i), event->screenPos());
        }
    }
}

/*!
    \internal
*/
void QGraphicsScenePrivate::installSceneEventFilter(QGraphicsItem *watched, QGraphicsItem *filter)
{
    sceneEventFilters.insert(watched, filter);
}

/*!
    \internal
*/
void QGraphicsScenePrivate::removeSceneEventFilter(QGraphicsItem *watched, QGraphicsItem *filter)
{
    if (!sceneEventFilters.contains(watched))
        return;

    QMultiMap<QGraphicsItem *, QGraphicsItem *>::Iterator it = sceneEventFilters.lowerBound(watched);
    QMultiMap<QGraphicsItem *, QGraphicsItem *>::Iterator end = sceneEventFilters.upperBound(watched);
    do {
        if (it.value() == filter)
            it = sceneEventFilters.erase(it);
        else
            ++it;
    } while (it != end);
}

/*!
  \internal
*/
bool QGraphicsScenePrivate::filterDescendantEvent(QGraphicsItem *item, QEvent *event)
{
    if (item && (item->d_ptr->ancestorFlags & QGraphicsItemPrivate::AncestorFiltersChildEvents)) {
        QGraphicsItem *parent = item->parentItem();
        while (parent) {
            if (parent->d_ptr->filtersDescendantEvents && parent->sceneEventFilter(item, event))
                return true;
            if (!(parent->d_ptr->ancestorFlags & QGraphicsItemPrivate::AncestorFiltersChildEvents))
                return false;
            parent = parent->parentItem();
        }
    }
    return false;
}

/*!
    \internal
*/
bool QGraphicsScenePrivate::filterEvent(QGraphicsItem *item, QEvent *event)
{
    if (item && !sceneEventFilters.contains(item))
        return false;

    QMultiMap<QGraphicsItem *, QGraphicsItem *>::Iterator it = sceneEventFilters.lowerBound(item);
    QMultiMap<QGraphicsItem *, QGraphicsItem *>::Iterator end = sceneEventFilters.upperBound(item);
    while (it != end) {
        // ### The filterer and filteree might both be deleted.
        if (it.value()->sceneEventFilter(it.key(), event))
            return true;
        ++it;
    }
    return false;
}

/*!
    \internal

    This is the final dispatch point for any events from the scene to the
    item. It filters the event first - if the filter returns true, the event
    is considered to have been eaten by the filter, and is therefore stopped
    (the default filter returns false). Then/otherwise, if the item is
    enabled, the event is sent; otherwise it is stopped.
*/
bool QGraphicsScenePrivate::sendEvent(QGraphicsItem *item, QEvent *event)
{
    if (QGraphicsObject *object = item->toGraphicsObject()) {
#ifndef QT_NO_GESTURES
        QGestureManager *gestureManager = QApplicationPrivate::instance()->gestureManager;
        if (gestureManager) {
            if (gestureManager->filterEvent(object, event))
                return true;
        }
#endif // QT_NO_GESTURES
    }

    if (filterEvent(item, event))
        return false;
    if (filterDescendantEvent(item, event))
        return false;
    if (!item || !item->isEnabled())
        return false;
    if (QGraphicsObject *o = item->toGraphicsObject()) {
        bool spont = event->spontaneous();
        if (spont ? qt_sendSpontaneousEvent(o, event) : QApplication::sendEvent(o, event))
            return true;
        event->spont = spont;
    }
    return item->sceneEvent(event);
}

/*!
    \internal
*/
void QGraphicsScenePrivate::cloneDragDropEvent(QGraphicsSceneDragDropEvent *dest,
                                               QGraphicsSceneDragDropEvent *source)
{
    dest->setWidget(source->widget());
    dest->setPos(source->pos());
    dest->setScenePos(source->scenePos());
    dest->setScreenPos(source->screenPos());
    dest->setButtons(source->buttons());
    dest->setModifiers(source->modifiers());
    dest->setPossibleActions(source->possibleActions());
    dest->setProposedAction(source->proposedAction());
    dest->setDropAction(source->dropAction());
    dest->setSource(source->source());
    dest->setMimeData(source->mimeData());
}

/*!
    \internal
*/
void QGraphicsScenePrivate::sendDragDropEvent(QGraphicsItem *item,
                                              QGraphicsSceneDragDropEvent *dragDropEvent)
{
    dragDropEvent->setPos(item->d_ptr->genericMapFromScene(dragDropEvent->scenePos(), dragDropEvent->widget()));
    sendEvent(item, dragDropEvent);
}

/*!
    \internal
*/
void QGraphicsScenePrivate::sendHoverEvent(QEvent::Type type, QGraphicsItem *item,
                                           QGraphicsSceneHoverEvent *hoverEvent)
{
    QGraphicsSceneHoverEvent event(type);
    event.setWidget(hoverEvent->widget());
    event.setPos(item->d_ptr->genericMapFromScene(hoverEvent->scenePos(), hoverEvent->widget()));
    event.setScenePos(hoverEvent->scenePos());
    event.setScreenPos(hoverEvent->screenPos());
    event.setLastPos(item->d_ptr->genericMapFromScene(hoverEvent->lastScenePos(), hoverEvent->widget()));
    event.setLastScenePos(hoverEvent->lastScenePos());
    event.setLastScreenPos(hoverEvent->lastScreenPos());
    event.setModifiers(hoverEvent->modifiers());
    sendEvent(item, &event);
}

/*!
    \internal
*/
void QGraphicsScenePrivate::sendMouseEvent(QGraphicsSceneMouseEvent *mouseEvent)
{
    if (mouseEvent->button() == 0 && mouseEvent->buttons() == 0 && lastMouseGrabberItemHasImplicitMouseGrab) {
        // ### This is a temporary fix for until we get proper mouse
        // grab events.
        clearMouseGrabber();
        return;
    }

    QGraphicsItem *item = mouseGrabberItems.last();
    if (item->isBlockedByModalPanel())
        return;

    for (int i = 0x1; i <= 0x10; i <<= 1) {
        Qt::MouseButton button = Qt::MouseButton(i);
        mouseEvent->setButtonDownPos(button, mouseGrabberButtonDownPos.value(button, item->d_ptr->genericMapFromScene(mouseEvent->scenePos(), mouseEvent->widget())));
        mouseEvent->setButtonDownScenePos(button, mouseGrabberButtonDownScenePos.value(button, mouseEvent->scenePos()));
        mouseEvent->setButtonDownScreenPos(button, mouseGrabberButtonDownScreenPos.value(button, mouseEvent->screenPos()));
    }
    mouseEvent->setPos(item->d_ptr->genericMapFromScene(mouseEvent->scenePos(), mouseEvent->widget()));
    mouseEvent->setLastPos(item->d_ptr->genericMapFromScene(mouseEvent->lastScenePos(), mouseEvent->widget()));
    sendEvent(item, mouseEvent);
}

/*!
    \internal
*/
void QGraphicsScenePrivate::mousePressEventHandler(QGraphicsSceneMouseEvent *mouseEvent)
{
    Q_Q(QGraphicsScene);

    // Ignore by default, unless we find a mouse grabber that accepts it.
    mouseEvent->ignore();

    // Deliver to any existing mouse grabber.
    if (!mouseGrabberItems.isEmpty()) {
        if (mouseGrabberItems.last()->isBlockedByModalPanel())
            return;
        // The event is ignored by default, but we disregard the event's
        // accepted state after delivery; the mouse is grabbed, after all.
        sendMouseEvent(mouseEvent);
        return;
    }

    // Start by determining the number of items at the current position.
    // Reuse value from earlier calculations if possible.
    if (cachedItemsUnderMouse.isEmpty()) {
        cachedItemsUnderMouse = itemsAtPosition(mouseEvent->screenPos(),
                                                mouseEvent->scenePos(),
                                                mouseEvent->widget());
    }

    // Update window activation.
    QGraphicsItem *topItem = cachedItemsUnderMouse.value(0);
    QGraphicsWidget *newActiveWindow = topItem ? topItem->window() : 0;
    if (newActiveWindow && newActiveWindow->isBlockedByModalPanel(&topItem)) {
        // pass activation to the blocking modal window
        newActiveWindow = topItem ? topItem->window() : 0;
    }

    if (newActiveWindow != q->activeWindow())
        q->setActiveWindow(newActiveWindow);

    // Set focus on the topmost enabled item that can take focus.
    bool setFocus = false;

    foreach (QGraphicsItem *item, cachedItemsUnderMouse) {
        if (item->isBlockedByModalPanel()
            || (item->d_ptr->flags & QGraphicsItem::ItemStopsFocusHandling)) {
            // Make sure we don't clear focus.
            setFocus = true;
            break;
        }
        if (item->isEnabled() && ((item->flags() & QGraphicsItem::ItemIsFocusable))) {
            if (!item->isWidget() || ((QGraphicsWidget *)item)->focusPolicy() & Qt::ClickFocus) {
                setFocus = true;
                if (item != q->focusItem() && item->d_ptr->mouseSetsFocus)
                    q->setFocusItem(item, Qt::MouseFocusReason);
                break;
            }
        }
        if (item->isPanel())
            break;
        if (item->d_ptr->flags & QGraphicsItem::ItemStopsClickFocusPropagation)
            break;
    }

    // Check for scene modality.
    bool sceneModality = false;
    for (int i = 0; i < modalPanels.size(); ++i) {
        if (modalPanels.at(i)->panelModality() == QGraphicsItem::SceneModal) {
            sceneModality = true;
            break;
        }
    }

    // If nobody could take focus, clear it.
    if (!stickyFocus && !setFocus && !sceneModality)
        q->setFocusItem(0, Qt::MouseFocusReason);

    // Any item will do.
    if (sceneModality && cachedItemsUnderMouse.isEmpty())
        cachedItemsUnderMouse << modalPanels.first();

    // Find a mouse grabber by sending mouse press events to all mouse grabber
    // candidates one at a time, until the event is accepted. It's accepted by
    // default, so the receiver has to explicitly ignore it for it to pass
    // through.
    foreach (QGraphicsItem *item, cachedItemsUnderMouse) {
        if (!(item->acceptedMouseButtons() & mouseEvent->button())) {
            // Skip items that don't accept the event's mouse button.
            continue;
        }

        // Check if this item is blocked by a modal panel and deliver the mouse event to the
        // blocking panel instead of this item if blocked.
        (void) item->isBlockedByModalPanel(&item);

        grabMouse(item, /* implicit = */ true);
        mouseEvent->accept();

        // check if the item we are sending to are disabled (before we send the event)
        bool disabled = !item->isEnabled();
        bool isPanel = item->isPanel();
        if (mouseEvent->type() == QEvent::GraphicsSceneMouseDoubleClick
            && item != lastMouseGrabberItem && lastMouseGrabberItem) {
            // If this item is different from the item that received the last
            // mouse event, and mouseEvent is a doubleclick event, then the
            // event is converted to a press. Known limitation:
            // Triple-clicking will not generate a doubleclick, though.
            QGraphicsSceneMouseEvent mousePress(QEvent::GraphicsSceneMousePress);
            mousePress.spont = mouseEvent->spont;
            mousePress.accept();
            mousePress.setButton(mouseEvent->button());
            mousePress.setButtons(mouseEvent->buttons());
            mousePress.setScreenPos(mouseEvent->screenPos());
            mousePress.setScenePos(mouseEvent->scenePos());
            mousePress.setModifiers(mouseEvent->modifiers());
            mousePress.setWidget(mouseEvent->widget());
            mousePress.setButtonDownPos(mouseEvent->button(),
                                        mouseEvent->buttonDownPos(mouseEvent->button()));
            mousePress.setButtonDownScenePos(mouseEvent->button(),
                                             mouseEvent->buttonDownScenePos(mouseEvent->button()));
            mousePress.setButtonDownScreenPos(mouseEvent->button(),
                                              mouseEvent->buttonDownScreenPos(mouseEvent->button()));
            sendMouseEvent(&mousePress);
            mouseEvent->setAccepted(mousePress.isAccepted());
        } else {
            sendMouseEvent(mouseEvent);
        }

        bool dontSendUngrabEvents = mouseGrabberItems.isEmpty() || mouseGrabberItems.last() != item;
        if (disabled) {
            ungrabMouse(item, /* itemIsDying = */ dontSendUngrabEvents);
            break;
        }
        if (mouseEvent->isAccepted()) {
            if (!mouseGrabberItems.isEmpty())
                storeMouseButtonsForMouseGrabber(mouseEvent);
            lastMouseGrabberItem = item;
            return;
        }
        ungrabMouse(item, /* itemIsDying = */ dontSendUngrabEvents);

        // Don't propagate through panels.
        if (isPanel)
            break;
    }

    // Is the event still ignored? Then the mouse press goes to the scene.
    // Reset the mouse grabber, clear the selection, clear focus, and leave
    // the event ignored so that it can propagate through the originating
    // view.
    if (!mouseEvent->isAccepted()) {
        clearMouseGrabber();

        QGraphicsView *view = mouseEvent->widget() ? qobject_cast<QGraphicsView *>(mouseEvent->widget()->parentWidget()) : 0;
        bool dontClearSelection = view && view->dragMode() == QGraphicsView::ScrollHandDrag;
        if (!dontClearSelection) {
            // Clear the selection if the originating view isn't in scroll
            // hand drag mode. The view will clear the selection if no drag
            // happened.
            q->clearSelection();
        }
    }
}

/*!
    \internal

    Ensures that the list of toplevels is sorted by insertion order, and that
    the siblingIndexes are packed (no gaps), and start at 0.

    ### This function is almost identical to
    QGraphicsItemPrivate::ensureSequentialSiblingIndex().
*/
void QGraphicsScenePrivate::ensureSequentialTopLevelSiblingIndexes()
{
    if (!topLevelSequentialOrdering) {
        qSort(topLevelItems.begin(), topLevelItems.end(), QGraphicsItemPrivate::insertionOrder);
        topLevelSequentialOrdering = true;
        needSortTopLevelItems = 1;
    }
    if (holesInTopLevelSiblingIndex) {
        holesInTopLevelSiblingIndex = 0;
        for (int i = 0; i < topLevelItems.size(); ++i)
            topLevelItems[i]->d_ptr->siblingIndex = i;
    }
}

/*!
    \internal

    Set the font and propagate the changes if the font is different from the
    current font.
*/
void QGraphicsScenePrivate::setFont_helper(const QFont &font)
{
    if (this->font == font && this->font.resolve() == font.resolve())
        return;
    updateFont(font);
}

/*!
    \internal

    Resolve the scene's font against the application font, and propagate the
    changes too all items in the scene.
*/
void QGraphicsScenePrivate::resolveFont()
{
    QFont naturalFont = QApplication::font();
    naturalFont.resolve(0);
    QFont resolvedFont = font.resolve(naturalFont);
    updateFont(resolvedFont);
}

/*!
    \internal

    Update the font, and whether or not it has changed, reresolve all fonts in
    the scene.
*/
void QGraphicsScenePrivate::updateFont(const QFont &font)
{
    Q_Q(QGraphicsScene);

    // Update local font setting.
    this->font = font;

    // Resolve the fonts of all top-level widget items, or widget items
    // whose parent is not a widget.
    foreach (QGraphicsItem *item, q->items()) {
        if (!item->parentItem()) {
            // Resolvefont for an item is a noop operation, but
            // every item can be a widget, or can have a widget
            // childre.
            item->d_ptr->resolveFont(font.resolve());
        }
    }

    // Send the scene a FontChange event.
    QEvent event(QEvent::FontChange);
    QApplication::sendEvent(q, &event);
}

/*!
    \internal

    Set the palette and propagate the changes if the palette is different from
    the current palette.
*/
void QGraphicsScenePrivate::setPalette_helper(const QPalette &palette)
{
    if (this->palette == palette && this->palette.resolve() == palette.resolve())
        return;
    updatePalette(palette);
}

/*!
    \internal

    Resolve the scene's palette against the application palette, and propagate
    the changes too all items in the scene.
*/
void QGraphicsScenePrivate::resolvePalette()
{
    QPalette naturalPalette = QApplication::palette();
    naturalPalette.resolve(0);
    QPalette resolvedPalette = palette.resolve(naturalPalette);
    updatePalette(resolvedPalette);
}

/*!
    \internal

    Update the palette, and whether or not it has changed, reresolve all
    palettes in the scene.
*/
void QGraphicsScenePrivate::updatePalette(const QPalette &palette)
{
    Q_Q(QGraphicsScene);

    // Update local palette setting.
    this->palette = palette;

    // Resolve the palettes of all top-level widget items, or widget items
    // whose parent is not a widget.
    foreach (QGraphicsItem *item, q->items()) {
        if (!item->parentItem()) {
            // Resolvefont for an item is a noop operation, but
            // every item can be a widget, or can have a widget
            // childre.
            item->d_ptr->resolvePalette(palette.resolve());
        }
    }

    // Send the scene a PaletteChange event.
    QEvent event(QEvent::PaletteChange);
    QApplication::sendEvent(q, &event);
}

/*!
    Constructs a QGraphicsScene object. The \a parent parameter is
    passed to QObject's constructor.
*/
QGraphicsScene::QGraphicsScene(QObject *parent)
    : QObject(*new QGraphicsScenePrivate, parent)
{
    d_func()->init();
}

/*!
    Constructs a QGraphicsScene object, using \a sceneRect for its
    scene rectangle. The \a parent parameter is passed to QObject's
    constructor.

    \sa sceneRect
*/
QGraphicsScene::QGraphicsScene(const QRectF &sceneRect, QObject *parent)
    : QObject(*new QGraphicsScenePrivate, parent)
{
    d_func()->init();
    setSceneRect(sceneRect);
}

/*!
    Constructs a QGraphicsScene object, using the rectangle specified
    by (\a x, \a y), and the given \a width and \a height for its
    scene rectangle. The \a parent parameter is passed to QObject's
    constructor.

    \sa sceneRect
*/
QGraphicsScene::QGraphicsScene(qreal x, qreal y, qreal width, qreal height, QObject *parent)
    : QObject(*new QGraphicsScenePrivate, parent)
{
    d_func()->init();
    setSceneRect(x, y, width, height);
}

/*!
  Removes and deletes all items from the scene object
  before destroying the scene object. The scene object
  is removed from the application's global scene list,
  and it is removed from all associated views.
*/
QGraphicsScene::~QGraphicsScene()
{
    Q_D(QGraphicsScene);

    // Remove this scene from qApp's global scene list.
    if (!QApplicationPrivate::is_app_closing)
        qApp->d_func()->scene_list.removeAll(this);

    clear();

    // Remove this scene from all associated views.
    for (int j = 0; j < d->views.size(); ++j)
        d->views.at(j)->setScene(0);
}

/*!
    \property QGraphicsScene::sceneRect
    \brief the scene rectangle; the bounding rectangle of the scene

    The scene rectangle defines the extent of the scene. It is
    primarily used by QGraphicsView to determine the view's default
    scrollable area, and by QGraphicsScene to manage item indexing.

    If unset, or if set to a null QRectF, sceneRect() will return the largest
    bounding rect of all items on the scene since the scene was created (i.e.,
    a rectangle that grows when items are added to or moved in the scene, but
    never shrinks).

    \sa width(), height(), QGraphicsView::sceneRect
*/
QRectF QGraphicsScene::sceneRect() const
{
    Q_D(const QGraphicsScene);
    if (d->hasSceneRect)
        return d->sceneRect;

    if (d->dirtyGrowingItemsBoundingRect) {
        // Lazily update the growing items bounding rect
        QGraphicsScenePrivate *thatd = const_cast<QGraphicsScenePrivate *>(d);
        QRectF oldGrowingBoundingRect = thatd->growingItemsBoundingRect;
        thatd->growingItemsBoundingRect |= itemsBoundingRect();
        thatd->dirtyGrowingItemsBoundingRect = false;
        if (oldGrowingBoundingRect != thatd->growingItemsBoundingRect)
            emit const_cast<QGraphicsScene *>(this)->sceneRectChanged(thatd->growingItemsBoundingRect);
    }
    return d->growingItemsBoundingRect;
}
void QGraphicsScene::setSceneRect(const QRectF &rect)
{
    Q_D(QGraphicsScene);
    if (rect != d->sceneRect) {
        d->hasSceneRect = !rect.isNull();
        d->sceneRect = rect;
        emit sceneRectChanged(d->hasSceneRect ? rect : d->growingItemsBoundingRect);
    }
}

/*!
     \fn qreal QGraphicsScene::width() const

     This convenience function is equivalent to calling sceneRect().width().

     \sa height()
*/

/*!
     \fn qreal QGraphicsScene::height() const

     This convenience function is equivalent to calling \c sceneRect().height().

     \sa width()
*/

/*!
    Renders the \a source rect from scene into \a target, using \a painter. This
    function is useful for capturing the contents of the scene onto a paint
    device, such as a QImage (e.g., to take a screenshot), or for printing
    with QPrinter. For example:

    \snippet doc/src/snippets/code/src_gui_graphicsview_qgraphicsscene.cpp 1

    If \a source is a null rect, this function will use sceneRect() to
    determine what to render. If \a target is a null rect, the dimensions of \a
    painter's paint device will be used.

    The source rect contents will be transformed according to \a
    aspectRatioMode to fit into the target rect. By default, the aspect ratio
    is kept, and \a source is scaled to fit in \a target.

    \sa QGraphicsView::render()
*/
void QGraphicsScene::render(QPainter *painter, const QRectF &target, const QRectF &source,
                            Qt::AspectRatioMode aspectRatioMode)
{
    // ### Switch to using the recursive rendering algorithm instead.

    // Default source rect = scene rect
    QRectF sourceRect = source;
    if (sourceRect.isNull())
        sourceRect = sceneRect();

    // Default target rect = device rect
    QRectF targetRect = target;
    if (targetRect.isNull()) {
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
    QList<QGraphicsItem *> itemList = items(sourceRect, Qt::IntersectsItemBoundingRect);
    QGraphicsItem **itemArray = new QGraphicsItem *[itemList.size()];
    int numItems = itemList.size();
    for (int i = 0; i < numItems; ++i)
        itemArray[numItems - i - 1] = itemList.at(i);
    itemList.clear();

    painter->save();

    // Transform the painter.
    painter->setClipRect(targetRect, Qt::IntersectClip);
    QTransform painterTransform;
    painterTransform *= QTransform()
                        .translate(targetRect.left(), targetRect.top())
                        .scale(xratio, yratio)
                        .translate(-sourceRect.left(), -sourceRect.top());
    painter->setWorldTransform(painterTransform, true);

    // Two unit vectors.
    QLineF v1(0, 0, 1, 0);
    QLineF v2(0, 0, 0, 1);

    // Generate the style options
    QStyleOptionGraphicsItem *styleOptionArray = new QStyleOptionGraphicsItem[numItems];
    for (int i = 0; i < numItems; ++i)
        itemArray[i]->d_ptr->initStyleOption(&styleOptionArray[i], painterTransform, targetRect.toRect());

    // Render the scene.
    drawBackground(painter, sourceRect);
    drawItems(painter, numItems, itemArray, styleOptionArray);
    drawForeground(painter, sourceRect);

    delete [] itemArray;
    delete [] styleOptionArray;

    painter->restore();
}

/*!
    \property QGraphicsScene::itemIndexMethod
    \brief the item indexing method.

    QGraphicsScene applies an indexing algorithm to the scene, to speed up
    item discovery functions like items() and itemAt(). Indexing is most
    efficient for static scenes (i.e., where items don't move around). For
    dynamic scenes, or scenes with many animated items, the index bookkeeping
    can outweight the fast lookup speeds.

    For the common case, the default index method BspTreeIndex works fine.  If
    your scene uses many animations and you are experiencing slowness, you can
    disable indexing by calling \c setItemIndexMethod(NoIndex).

    \sa bspTreeDepth
*/
QGraphicsScene::ItemIndexMethod QGraphicsScene::itemIndexMethod() const
{
    Q_D(const QGraphicsScene);
    return d->indexMethod;
}
void QGraphicsScene::setItemIndexMethod(ItemIndexMethod method)
{
    Q_D(QGraphicsScene);
    if (d->indexMethod == method)
        return;

    d->indexMethod = method;

    QList<QGraphicsItem *> oldItems = d->index->items(Qt::DescendingOrder);
    delete d->index;
    if (method == BspTreeIndex)
        d->index = new QGraphicsSceneBspTreeIndex(this);
    else
        d->index = new QGraphicsSceneLinearIndex(this);
    for (int i = oldItems.size() - 1; i >= 0; --i)
        d->index->addItem(oldItems.at(i));
}

/*!
    \property QGraphicsScene::bspTreeDepth
    \brief the depth of QGraphicsScene's BSP index tree
    \since 4.3

    This property has no effect when NoIndex is used.

    This value determines the depth of QGraphicsScene's BSP tree. The depth
    directly affects QGraphicsScene's performance and memory usage; the latter
    growing exponentially with the depth of the tree. With an optimal tree
    depth, QGraphicsScene can instantly determine the locality of items, even
    for scenes with thousands or millions of items. This also greatly improves
    rendering performance.

    By default, the value is 0, in which case Qt will guess a reasonable
    default depth based on the size, location and number of items in the
    scene. If these parameters change frequently, however, you may experience
    slowdowns as QGraphicsScene retunes the depth internally. You can avoid
    potential slowdowns by fixating the tree depth through setting this
    property.

    The depth of the tree and the size of the scene rectangle decide the
    granularity of the scene's partitioning. The size of each scene segment is
    determined by the following algorithm:

    \snippet doc/src/snippets/code/src_gui_graphicsview_qgraphicsscene.cpp 2

    The BSP tree has an optimal size when each segment contains between 0 and
    10 items.

    \sa itemIndexMethod
*/
int QGraphicsScene::bspTreeDepth() const
{
    Q_D(const QGraphicsScene);
    QGraphicsSceneBspTreeIndex *bspTree = qobject_cast<QGraphicsSceneBspTreeIndex *>(d->index);
    return bspTree ? bspTree->bspTreeDepth() : 0;
}
void QGraphicsScene::setBspTreeDepth(int depth)
{
    Q_D(QGraphicsScene);
    if (depth < 0) {
        qWarning("QGraphicsScene::setBspTreeDepth: invalid depth %d ignored; must be >= 0", depth);
        return;
    }

    QGraphicsSceneBspTreeIndex *bspTree = qobject_cast<QGraphicsSceneBspTreeIndex *>(d->index);
    if (!bspTree) {
        qWarning("QGraphicsScene::setBspTreeDepth: can not apply if indexing method is not BSP");
        return;
    }
    bspTree->setBspTreeDepth(depth);
}

/*!
    \property QGraphicsScene::sortCacheEnabled
    \brief whether sort caching is enabled
    \since 4.5
    \obsolete

    Since Qt 4.6, this property has no effect.
*/
bool QGraphicsScene::isSortCacheEnabled() const
{
    Q_D(const QGraphicsScene);
    return d->sortCacheEnabled;
}
void QGraphicsScene::setSortCacheEnabled(bool enabled)
{
    Q_D(QGraphicsScene);
    if (d->sortCacheEnabled == enabled)
        return;
    d->sortCacheEnabled = enabled;
}

/*!
    Calculates and returns the bounding rect of all items on the scene. This
    function works by iterating over all items, and because if this, it can
    be slow for large scenes.

    \sa sceneRect()
*/
QRectF QGraphicsScene::itemsBoundingRect() const
{
    // Does not take untransformable items into account.
    QRectF boundingRect;
    foreach (QGraphicsItem *item, items())
        boundingRect |= item->sceneBoundingRect();
    return boundingRect;
}

/*!
    Returns a list of all items in the scene in descending stacking order.

    \sa addItem(), removeItem(), {QGraphicsItem#Sorting}{Sorting}
*/
QList<QGraphicsItem *> QGraphicsScene::items() const
{
    Q_D(const QGraphicsScene);
    return d->index->items(Qt::DescendingOrder);
}

/*!
    Returns an ordered list of all items on the scene. \a order decides the
    stacking order.

    \sa addItem(), removeItem(), {QGraphicsItem#Sorting}{Sorting}
*/
QList<QGraphicsItem *> QGraphicsScene::items(Qt::SortOrder order) const
{
    Q_D(const QGraphicsScene);
    return d->index->items(order);
}

/*!
    \obsolete

    Returns all visible items at position \a pos in the scene. The items are
    listed in descending stacking order (i.e., the first item in the list is the
    top-most item, and the last item is the bottom-most item).

    This function is deprecated and returns incorrect results if the scene
    contains items that ignore transformations. Use the overload that takes
    a QTransform instead.

    \sa itemAt(), {QGraphicsItem#Sorting}{Sorting}
*/
QList<QGraphicsItem *> QGraphicsScene::items(const QPointF &pos) const
{
    Q_D(const QGraphicsScene);
    return d->index->items(pos, Qt::IntersectsItemShape, Qt::DescendingOrder);
}

/*!
    \overload
    \obsolete

    Returns all visible items that, depending on \a mode, are either inside or
    intersect with the specified \a rectangle.

    The default value for \a mode is Qt::IntersectsItemShape; all items whose
    exact shape intersects with or is contained by \a rectangle are returned.

    This function is deprecated and returns incorrect results if the scene
    contains items that ignore transformations. Use the overload that takes
    a QTransform instead.

    \sa itemAt(), {QGraphicsItem#Sorting}{Sorting}
*/
QList<QGraphicsItem *> QGraphicsScene::items(const QRectF &rectangle, Qt::ItemSelectionMode mode) const
{
    Q_D(const QGraphicsScene);
    return d->index->items(rectangle, mode, Qt::DescendingOrder);
}

/*!
    \fn QList<QGraphicsItem *> QGraphicsScene::items(qreal x, qreal y, qreal w, qreal h, Qt::ItemSelectionMode mode) const
    \obsolete
    \since 4.3

    This convenience function is equivalent to calling items(QRectF(\a x, \a y, \a w, \a h), \a mode).

    This function is deprecated and returns incorrect results if the scene
    contains items that ignore transformations. Use the overload that takes
    a QTransform instead.
*/

/*!
    \fn QList<QGraphicsItem *> QGraphicsScene::items(qreal x, qreal y, qreal w, qreal h, Qt::ItemSelectionMode mode, Qt::SortOrder order, const QTransform &deviceTransform) const
    \overload
    \since 4.6

    \brief Returns all visible items that, depending on \a mode, are
    either inside or intersect with the rectangle defined by \a x, \a y,
    \a w and \a h, in a list sorted using \a order.

    \a deviceTransform is the transformation that applies to the view, and needs to
    be provided if the scene contains items that ignore transformations.
*/

/*!
    \fn QList<QGraphicsItem *> QGraphicsScene::items(const QPolygonF &polygon, Qt::ItemSelectionMode mode) const
    \overload
    \obsolete

    Returns all visible items that, depending on \a mode, are either inside or
    intersect with the polygon \a polygon.

    The default value for \a mode is Qt::IntersectsItemShape; all items whose
    exact shape intersects with or is contained by \a polygon are returned.

    This function is deprecated and returns incorrect results if the scene
    contains items that ignore transformations. Use the overload that takes
    a QTransform instead.

    \sa itemAt(), {QGraphicsItem#Sorting}{Sorting}
*/
QList<QGraphicsItem *> QGraphicsScene::items(const QPolygonF &polygon, Qt::ItemSelectionMode mode) const
{
    Q_D(const QGraphicsScene);
    return d->index->items(polygon, mode, Qt::DescendingOrder);
}

/*!
    \fn QList<QGraphicsItem *> QGraphicsScene::items(const QPainterPath &path, Qt::ItemSelectionMode mode) const
    \overload
    \obsolete

    Returns all visible items that, depending on \a path, are either inside or
    intersect with the path \a path.

    The default value for \a mode is Qt::IntersectsItemShape; all items whose
    exact shape intersects with or is contained by \a path are returned.

    This function is deprecated and returns incorrect results if the scene
    contains items that ignore transformations. Use the overload that takes
    a QTransform instead.

    \sa itemAt(), {QGraphicsItem#Sorting}{Sorting}
*/
QList<QGraphicsItem *> QGraphicsScene::items(const QPainterPath &path, Qt::ItemSelectionMode mode) const
{
    Q_D(const QGraphicsScene);
    return d->index->items(path, mode, Qt::DescendingOrder);
}

/*!
    \fn QList<QGraphicsItem *> QGraphicsScene::items(const QPointF &pos, Qt::ItemSelectionMode mode, Qt::SortOrder order, const QTransform &deviceTransform) const
    \since 4.6

    \brief Returns all visible items that, depending on \a mode, are at
    the specified \a pos in a list sorted using \a order.

    The default value for \a mode is Qt::IntersectsItemShape; all items whose
    exact shape intersects with \a pos are returned.

    \a deviceTransform is the transformation that applies to the view, and needs to
    be provided if the scene contains items that ignore transformations.

    \sa itemAt(), {QGraphicsItem#Sorting}{Sorting}
*/
QList<QGraphicsItem *> QGraphicsScene::items(const QPointF &pos, Qt::ItemSelectionMode mode,
                                             Qt::SortOrder order, const QTransform &deviceTransform) const
{
    Q_D(const QGraphicsScene);
    return d->index->items(pos, mode, order, deviceTransform);
}

/*!
    \fn QList<QGraphicsItem *> QGraphicsScene::items(const QRectF &rect, Qt::ItemSelectionMode mode, Qt::SortOrder order, const QTransform &deviceTransform) const
    \overload
    \since 4.6

    \brief Returns all visible items that, depending on \a mode, are
    either inside or intersect with the specified \a rect and return a
    list sorted using \a order.

    The default value for \a mode is Qt::IntersectsItemShape; all items whose
    exact shape intersects with or is contained by \a rect are returned.

    \a deviceTransform is the transformation that applies to the view, and needs to
    be provided if the scene contains items that ignore transformations.

    \sa itemAt(), {QGraphicsItem#Sorting}{Sorting}
*/
QList<QGraphicsItem *> QGraphicsScene::items(const QRectF &rect, Qt::ItemSelectionMode mode,
                                             Qt::SortOrder order, const QTransform &deviceTransform) const
{
    Q_D(const QGraphicsScene);
    return d->index->items(rect, mode, order, deviceTransform);
}

/*!
    \fn QList<QGraphicsItem *> QGraphicsScene::items(const QPolygonF &polygon, Qt::ItemSelectionMode mode, Qt::SortOrder order, const QTransform &deviceTransform) const
    \overload
    \since 4.6

    \brief Returns all visible items that, depending on \a mode, are
    either inside or intersect with the specified \a polygon and return
    a list sorted using \a order.

    The default value for \a mode is Qt::IntersectsItemShape; all items whose
    exact shape intersects with or is contained by \a polygon are returned.

    \a deviceTransform is the transformation that applies to the view, and needs to
    be provided if the scene contains items that ignore transformations.

    \sa itemAt(), {QGraphicsItem#Sorting}{Sorting}
*/
QList<QGraphicsItem *> QGraphicsScene::items(const QPolygonF &polygon, Qt::ItemSelectionMode mode,
                                             Qt::SortOrder order, const QTransform &deviceTransform) const
{
    Q_D(const QGraphicsScene);
    return d->index->items(polygon, mode, order, deviceTransform);
}

/*!
    \fn QList<QGraphicsItem *> QGraphicsScene::items(const QPainterPath &path, Qt::ItemSelectionMode mode, Qt::SortOrder order, const QTransform &deviceTransform) const
    \overload
    \since 4.6

    \brief Returns all visible items that, depending on \a mode, are
    either inside or intersect with the specified \a path and return a
    list sorted using \a order.

    The default value for \a mode is Qt::IntersectsItemShape; all items whose
    exact shape intersects with or is contained by \a path are returned.

    \a deviceTransform is the transformation that applies to the view, and needs to
    be provided if the scene contains items that ignore transformations.

    \sa itemAt(), {QGraphicsItem#Sorting}{Sorting}
*/
QList<QGraphicsItem *> QGraphicsScene::items(const QPainterPath &path, Qt::ItemSelectionMode mode,
                                             Qt::SortOrder order, const QTransform &deviceTransform) const
{
    Q_D(const QGraphicsScene);
    return d->index->items(path, mode, order, deviceTransform);
}

/*!
    Returns a list of all items that collide with \a item. Collisions are
    determined by calling QGraphicsItem::collidesWithItem(); the collision
    detection is determined by \a mode. By default, all items whose shape
    intersects \a item or is contained inside \a item's shape are returned.

    The items are returned in descending stacking order (i.e., the first item
    in the list is the uppermost item, and the last item is the lowermost
    item).

    \sa items(), itemAt(), QGraphicsItem::collidesWithItem(), {QGraphicsItem#Sorting}{Sorting}
*/
QList<QGraphicsItem *> QGraphicsScene::collidingItems(const QGraphicsItem *item,
                                                      Qt::ItemSelectionMode mode) const
{
    Q_D(const QGraphicsScene);
    if (!item) {
        qWarning("QGraphicsScene::collidingItems: cannot find collisions for null item");
        return QList<QGraphicsItem *>();
    }

    // Does not support ItemIgnoresTransformations.
    QList<QGraphicsItem *> tmp;
    foreach (QGraphicsItem *itemInVicinity, d->index->estimateItems(item->sceneBoundingRect(), Qt::DescendingOrder)) {
        if (item != itemInVicinity && item->collidesWithItem(itemInVicinity, mode))
            tmp << itemInVicinity;
    }
    return tmp;
}

/*!
    \overload
    \obsolete

    Returns the topmost visible item at the specified \a position, or 0 if
    there are no items at this position.

    This function is deprecated and returns incorrect results if the scene
    contains items that ignore transformations. Use the overload that takes
    a QTransform instead.

    \sa items(), collidingItems(), {QGraphicsItem#Sorting}{Sorting}
*/
QGraphicsItem *QGraphicsScene::itemAt(const QPointF &position) const
{
    QList<QGraphicsItem *> itemsAtPoint = items(position);
    return itemsAtPoint.isEmpty() ? 0 : itemsAtPoint.first();
}

/*!
    \since 4.6

    Returns the topmost visible item at the specified \a position, or 0
    if there are no items at this position.

    \a deviceTransform is the transformation that applies to the view, and needs to
    be provided if the scene contains items that ignore transformations.

    \sa items(), collidingItems(), {QGraphicsItem#Sorting}{Sorting}
*/
QGraphicsItem *QGraphicsScene::itemAt(const QPointF &position, const QTransform &deviceTransform) const
{
    QList<QGraphicsItem *> itemsAtPoint = items(position, Qt::IntersectsItemShape,
                                                Qt::DescendingOrder, deviceTransform);
    return itemsAtPoint.isEmpty() ? 0 : itemsAtPoint.first();
}

/*!
    \fn QGraphicsScene::itemAt(qreal x, qreal y, const QTransform &deviceTransform) const
    \overload
    \since 4.6

    Returns the topmost item at the position specified by (\a x, \a
    y), or 0 if there are no items at this position.

    \a deviceTransform is the transformation that applies to the view, and needs to
    be provided if the scene contains items that ignore transformations.

    This convenience function is equivalent to calling \c
    {itemAt(QPointF(x, y), deviceTransform)}.
*/

/*!
    \fn QGraphicsScene::itemAt(qreal x, qreal y) const
    \overload
    \obsolete

    Returns the topmost item at the position specified by (\a x, \a
    y), or 0 if there are no items at this position.

    This convenience function is equivalent to calling \c
    {itemAt(QPointF(x, y))}.

    This function is deprecated and returns incorrect results if the scene
    contains items that ignore transformations. Use the overload that takes
    a QTransform instead.
*/

/*!
    Returns a list of all currently selected items. The items are
    returned in no particular order.

    \sa setSelectionArea()
*/
QList<QGraphicsItem *> QGraphicsScene::selectedItems() const
{
    Q_D(const QGraphicsScene);

    // Optimization: Lazily removes items that are not selected.
    QGraphicsScene *that = const_cast<QGraphicsScene *>(this);
    QSet<QGraphicsItem *> actuallySelectedSet;
    foreach (QGraphicsItem *item, that->d_func()->selectedItems) {
        if (item->isSelected())
            actuallySelectedSet << item;
    }

    that->d_func()->selectedItems = actuallySelectedSet;

    return d->selectedItems.values();
}

/*!
    Returns the selection area that was previously set with
    setSelectionArea(), or an empty QPainterPath if no selection area has been
    set.

    \sa setSelectionArea()
*/
QPainterPath QGraphicsScene::selectionArea() const
{
    Q_D(const QGraphicsScene);
    return d->selectionArea;
}

/*!
    \since 4.6

    Sets the selection area to \a path. All items within this area are
    immediately selected, and all items outside are unselected. You can get
    the list of all selected items by calling selectedItems().

    \a deviceTransform is the transformation that applies to the view, and needs to
    be provided if the scene contains items that ignore transformations.

    For an item to be selected, it must be marked as \e selectable
    (QGraphicsItem::ItemIsSelectable).

    \sa clearSelection(), selectionArea()
*/
void QGraphicsScene::setSelectionArea(const QPainterPath &path, const QTransform &deviceTransform)
{
    setSelectionArea(path, Qt::IntersectsItemShape, deviceTransform);
}

/*!
    \obsolete
    \overload

    Sets the selection area to \a path.

    This function is deprecated and leads to incorrect results if the scene
    contains items that ignore transformations. Use the overload that takes
    a QTransform instead.
*/
void QGraphicsScene::setSelectionArea(const QPainterPath &path)
{
    setSelectionArea(path, Qt::IntersectsItemShape, QTransform());
}

/*!
    \obsolete
    \overload
    \since 4.3

    Sets the selection area to \a path using \a mode to determine if items are
    included in the selection area.

    \sa clearSelection(), selectionArea()
*/
void QGraphicsScene::setSelectionArea(const QPainterPath &path, Qt::ItemSelectionMode mode)
{
    setSelectionArea(path, mode, QTransform());
}

/*!
    \overload
    \since 4.6

    Sets the selection area to \a path using \a mode to determine if items are
    included in the selection area.

    \a deviceTransform is the transformation that applies to the view, and needs to
    be provided if the scene contains items that ignore transformations.

    \sa clearSelection(), selectionArea()
*/
void QGraphicsScene::setSelectionArea(const QPainterPath &path, Qt::ItemSelectionMode mode,
                                      const QTransform &deviceTransform)
{
    Q_D(QGraphicsScene);

    // Note: with boolean path operations, we can improve performance here
    // quite a lot by "growing" the old path instead of replacing it. That
    // allows us to only check the intersect area for changes, instead of
    // reevaluating the whole path over again.
    d->selectionArea = path;

    QSet<QGraphicsItem *> unselectItems = d->selectedItems;

    // Disable emitting selectionChanged() for individual items.
    ++d->selectionChanging;
    bool changed = false;

    // Set all items in path to selected.
    foreach (QGraphicsItem *item, items(path, mode, Qt::DescendingOrder, deviceTransform)) {
        if (item->flags() & QGraphicsItem::ItemIsSelectable) {
            if (!item->isSelected())
                changed = true;
            unselectItems.remove(item);
            item->setSelected(true);
        }
    }

    // Unselect all items outside path.
    foreach (QGraphicsItem *item, unselectItems) {
        item->setSelected(false);
        changed = true;
    }

    // Reenable emitting selectionChanged() for individual items.
    --d->selectionChanging;

    if (!d->selectionChanging && changed)
        emit selectionChanged();
}

/*!
   Clears the current selection.

   \sa setSelectionArea(), selectedItems()
*/
void QGraphicsScene::clearSelection()
{
    Q_D(QGraphicsScene);

    // Disable emitting selectionChanged
    ++d->selectionChanging;
    bool changed = !d->selectedItems.isEmpty();

    foreach (QGraphicsItem *item, d->selectedItems)
        item->setSelected(false);
    d->selectedItems.clear();

    // Reenable emitting selectionChanged() for individual items.
    --d->selectionChanging;

    if (!d->selectionChanging && changed)
        emit selectionChanged();
}

/*!
    \since 4.4

    Removes and deletes all items from the scene, but otherwise leaves the
    state of the scene unchanged.

    \sa addItem()
*/
void QGraphicsScene::clear()
{
    Q_D(QGraphicsScene);
    // NB! We have to clear the index before deleting items; otherwise the
    // index might try to access dangling item pointers.
    d->index->clear();
    // NB! QGraphicsScenePrivate::unregisterTopLevelItem() removes items
    while (!d->topLevelItems.isEmpty())
        delete d->topLevelItems.first();
    Q_ASSERT(d->topLevelItems.isEmpty());
    d->lastItemCount = 0;
    d->allItemsIgnoreHoverEvents = true;
    d->allItemsUseDefaultCursor = true;
    d->allItemsIgnoreTouchEvents = true;
}

/*!
    Groups all items in \a items into a new QGraphicsItemGroup, and returns a
    pointer to the group. The group is created with the common ancestor of \a
    items as its parent, and with position (0, 0). The items are all
    reparented to the group, and their positions and transformations are
    mapped to the group. If \a items is empty, this function will return an
    empty top-level QGraphicsItemGroup.

    QGraphicsScene has ownership of the group item; you do not need to delete
    it. To dismantle (ungroup) a group, call destroyItemGroup().

    \sa destroyItemGroup(), QGraphicsItemGroup::addToGroup()
*/
QGraphicsItemGroup *QGraphicsScene::createItemGroup(const QList<QGraphicsItem *> &items)
{
    // Build a list of the first item's ancestors
    QList<QGraphicsItem *> ancestors;
    int n = 0;
    if (!items.isEmpty()) {
        QGraphicsItem *parent = items.at(n++);
        while ((parent = parent->parentItem()))
            ancestors.append(parent);
    }

    // Find the common ancestor for all items
    QGraphicsItem *commonAncestor = 0;
    if (!ancestors.isEmpty()) {
        while (n < items.size()) {
            int commonIndex = -1;
            QGraphicsItem *parent = items.at(n++);
            do {
                int index = ancestors.indexOf(parent, qMax(0, commonIndex));
                if (index != -1) {
                    commonIndex = index;
                    break;
                }
            } while ((parent = parent->parentItem()));

            if (commonIndex == -1) {
                commonAncestor = 0;
                break;
            }

            commonAncestor = ancestors.at(commonIndex);
        }
    }

    // Create a new group at that level
    QGraphicsItemGroup *group = new QGraphicsItemGroup(commonAncestor);
    if (!commonAncestor)
        addItem(group);
    foreach (QGraphicsItem *item, items)
        group->addToGroup(item);
    return group;
}

/*!
    Reparents all items in \a group to \a group's parent item, then removes \a
    group from the scene, and finally deletes it. The items' positions and
    transformations are mapped from the group to the group's parent.

    \sa createItemGroup(), QGraphicsItemGroup::removeFromGroup()
*/
void QGraphicsScene::destroyItemGroup(QGraphicsItemGroup *group)
{
    foreach (QGraphicsItem *item, group->children())
        group->removeFromGroup(item);
    removeItem(group);
    delete group;
}

/*!
    Adds or moves the \a item and all its childen to this scene.
    This scene takes ownership of the \a item.

    If the item is visible (i.e., QGraphicsItem::isVisible() returns
    true), QGraphicsScene will emit changed() once control goes back
    to the event loop.

    If the item is already in a different scene, it will first be
    removed from its old scene, and then added to this scene as a
    top-level.

    QGraphicsScene will send ItemSceneChange notifications to \a item
    while it is added to the scene. If item does not currently belong
    to a scene, only one notification is sent. If it does belong to
    scene already (i.e., it is moved to this scene), QGraphicsScene
    will send an addition notification as the item is removed from its
    previous scene.

    If the item is a panel, the scene is active, and there is no
    active panel in the scene, then the item will be activated.

    \sa removeItem(), addEllipse(), addLine(), addPath(), addPixmap(),
    addRect(), addText(), addWidget(), {QGraphicsItem#Sorting}{Sorting}
*/
void QGraphicsScene::addItem(QGraphicsItem *item)
{
    Q_D(QGraphicsScene);
    if (!item) {
        qWarning("QGraphicsScene::addItem: cannot add null item");
        return;
    }
    if (item->d_ptr->scene == this) {
        qWarning("QGraphicsScene::addItem: item has already been added to this scene");
        return;
    }
    // Remove this item from its existing scene
    if (QGraphicsScene *oldScene = item->d_ptr->scene)
        oldScene->removeItem(item);

    // Notify the item that its scene is changing, and allow the item to
    // react.
    const QVariant newSceneVariant(item->itemChange(QGraphicsItem::ItemSceneChange,
                                                    QVariant::fromValue<QGraphicsScene *>(this)));
    QGraphicsScene *targetScene = qvariant_cast<QGraphicsScene *>(newSceneVariant);
    if (targetScene != this) {
        if (targetScene && item->d_ptr->scene != targetScene)
            targetScene->addItem(item);
        return;
    }

    // QDeclarativeItems do not rely on initial itemChanged message, as the componentComplete
    // function allows far more opportunity for delayed-construction optimization.
    if (!item->d_ptr->isDeclarativeItem) {
        if (d->unpolishedItems.isEmpty()) {
            QMetaMethod method = metaObject()->method(d->polishItemsIndex);
            method.invoke(this, Qt::QueuedConnection);
        }
        d->unpolishedItems.append(item);
        item->d_ptr->pendingPolish = true;
    }

    // Detach this item from its parent if the parent's scene is different
    // from this scene.
    if (QGraphicsItem *itemParent = item->d_ptr->parent) {
        if (itemParent->d_ptr->scene != this)
            item->setParentItem(0);
    }

    // Add the item to this scene
    item->d_func()->scene = targetScene;

    // Add the item in the index
    d->index->addItem(item);

    // Add to list of toplevels if this item is a toplevel.
    if (!item->d_ptr->parent)
        d->registerTopLevelItem(item);

    // Add to list of items that require an update. We cannot assume that the
    // item is fully constructed, so calling item->update() can lead to a pure
    // virtual function call to boundingRect().
    d->markDirty(item);
    d->dirtyGrowingItemsBoundingRect = true;

    // Disable selectionChanged() for individual items
    ++d->selectionChanging;
    int oldSelectedItemSize = d->selectedItems.size();

    // Enable mouse tracking if the item accepts hover events or has a cursor set.
    if (d->allItemsIgnoreHoverEvents && d->itemAcceptsHoverEvents_helper(item)) {
        d->allItemsIgnoreHoverEvents = false;
        d->enableMouseTrackingOnViews();
    }
#ifndef QT_NO_CURSOR
    if (d->allItemsUseDefaultCursor && item->d_ptr->hasCursor) {
        d->allItemsUseDefaultCursor = false;
        if (d->allItemsIgnoreHoverEvents) // already enabled otherwise
            d->enableMouseTrackingOnViews();
    }
#endif //QT_NO_CURSOR

    // Enable touch events if the item accepts touch events.
    if (d->allItemsIgnoreTouchEvents && item->d_ptr->acceptTouchEvents) {
        d->allItemsIgnoreTouchEvents = false;
        d->enableTouchEventsOnViews();
    }

#ifndef QT_NO_GESTURES
    foreach (Qt::GestureType gesture, item->d_ptr->gestureContext.keys())
        d->grabGesture(item, gesture);
#endif

    // Update selection lists
    if (item->isSelected())
        d->selectedItems << item;
    if (item->isWidget() && item->isVisible() && static_cast<QGraphicsWidget *>(item)->windowType() == Qt::Popup)
        d->addPopup(static_cast<QGraphicsWidget *>(item));
    if (item->isPanel() && item->isVisible() && item->panelModality() != QGraphicsItem::NonModal)
        d->enterModal(item);

    // Update creation order focus chain. Make sure to leave the widget's
    // internal tab order intact.
    if (item->isWidget()) {
        QGraphicsWidget *widget = static_cast<QGraphicsWidget *>(item);
        if (!d->tabFocusFirst) {
            // No first tab focus widget - make this the first tab focus
            // widget.
            d->tabFocusFirst = widget;
        } else if (!widget->parentWidget()) {
            // Adding a widget that is not part of a tab focus chain.
            QGraphicsWidget *last = d->tabFocusFirst->d_func()->focusPrev;
            QGraphicsWidget *lastNew = widget->d_func()->focusPrev;
            last->d_func()->focusNext = widget;
            widget->d_func()->focusPrev = last;
            d->tabFocusFirst->d_func()->focusPrev = lastNew;
            lastNew->d_func()->focusNext = d->tabFocusFirst;
        }
    }

    // Add all children recursively
    item->d_ptr->ensureSortedChildren();
    for (int i = 0; i < item->d_ptr->children.size(); ++i)
        addItem(item->d_ptr->children.at(i));

    // Resolve font and palette.
    item->d_ptr->resolveFont(d->font.resolve());
    item->d_ptr->resolvePalette(d->palette.resolve());


    // Reenable selectionChanged() for individual items
    --d->selectionChanging;
    if (!d->selectionChanging && d->selectedItems.size() != oldSelectedItemSize)
        emit selectionChanged();

    // Deliver post-change notification
    item->itemChange(QGraphicsItem::ItemSceneHasChanged, newSceneVariant);

    // Update explicit activation
    bool autoActivate = true;
    if (!d->childExplicitActivation && item->d_ptr->explicitActivate)
        d->childExplicitActivation = item->d_ptr->wantsActive ? 1 : 2;
    if (d->childExplicitActivation && item->isPanel()) {
        if (d->childExplicitActivation == 1)
            setActivePanel(item);
        else
            autoActivate = false;
        d->childExplicitActivation = 0;
    } else if (!item->d_ptr->parent) {
        d->childExplicitActivation = 0;
    }

    // Auto-activate this item's panel if nothing else has been activated
    if (autoActivate) {
        if (!d->lastActivePanel && !d->activePanel && item->isPanel()) {
            if (isActive())
                setActivePanel(item);
            else
                d->lastActivePanel = item;
        }
    }

    if (item->d_ptr->flags & QGraphicsItem::ItemSendsScenePositionChanges)
        d->registerScenePosItem(item);

    // Ensure that newly added items that have subfocus set, gain
    // focus automatically if there isn't a focus item already.
    if (!d->focusItem && item != d->lastFocusItem && item->focusItem() == item)
        item->focusItem()->setFocus();

    d->updateInputMethodSensitivityInViews();
}

/*!
    Creates and adds an ellipse item to the scene, and returns the item
    pointer. The geometry of the ellipse is defined by \a rect, and its pen
    and brush are initialized to \a pen and \a brush.

    Note that the item's geometry is provided in item coordinates, and its
    position is initialized to (0, 0).

    If the item is visible (i.e., QGraphicsItem::isVisible() returns true),
    QGraphicsScene will emit changed() once control goes back to the event
    loop.

    \sa addLine(), addPath(), addPixmap(), addRect(), addText(), addItem(),
    addWidget()
*/
QGraphicsEllipseItem *QGraphicsScene::addEllipse(const QRectF &rect, const QPen &pen, const QBrush &brush)
{
    QGraphicsEllipseItem *item = new QGraphicsEllipseItem(rect);
    item->setPen(pen);
    item->setBrush(brush);
    addItem(item);
    return item;
}

/*!
    \fn QGraphicsEllipseItem *QGraphicsScene::addEllipse(qreal x, qreal y, qreal w, qreal h, const QPen &pen, const QBrush &brush)
    \since 4.3

    This convenience function is equivalent to calling addEllipse(QRectF(\a x,
    \a y, \a w, \a h), \a pen, \a brush).
*/

/*!
    Creates and adds a line item to the scene, and returns the item
    pointer. The geometry of the line is defined by \a line, and its pen
    is initialized to \a pen.

    Note that the item's geometry is provided in item coordinates, and its
    position is initialized to (0, 0).

    If the item is visible (i.e., QGraphicsItem::isVisible() returns true),
    QGraphicsScene will emit changed() once control goes back to the event
    loop.

    \sa addEllipse(), addPath(), addPixmap(), addRect(), addText(), addItem(),
    addWidget()
*/
QGraphicsLineItem *QGraphicsScene::addLine(const QLineF &line, const QPen &pen)
{
    QGraphicsLineItem *item = new QGraphicsLineItem(line);
    item->setPen(pen);
    addItem(item);
    return item;
}

/*!
    \fn QGraphicsLineItem *QGraphicsScene::addLine(qreal x1, qreal y1, qreal x2, qreal y2, const QPen &pen)
    \since 4.3

    This convenience function is equivalent to calling addLine(QLineF(\a x1,
    \a y1, \a x2, \a y2), \a pen).
*/

/*!
    Creates and adds a path item to the scene, and returns the item
    pointer. The geometry of the path is defined by \a path, and its pen and
    brush are initialized to \a pen and \a brush.

    Note that the item's geometry is provided in item coordinates, and its
    position is initialized to (0, 0).

    If the item is visible (i.e., QGraphicsItem::isVisible() returns true),
    QGraphicsScene will emit changed() once control goes back to the event
    loop.

    \sa addEllipse(), addLine(), addPixmap(), addRect(), addText(), addItem(),
    addWidget()
*/
QGraphicsPathItem *QGraphicsScene::addPath(const QPainterPath &path, const QPen &pen, const QBrush &brush)
{
    QGraphicsPathItem *item = new QGraphicsPathItem(path);
    item->setPen(pen);
    item->setBrush(brush);
    addItem(item);
    return item;
}

/*!
    Creates and adds a pixmap item to the scene, and returns the item
    pointer. The pixmap is defined by \a pixmap.

    Note that the item's geometry is provided in item coordinates, and its
    position is initialized to (0, 0).

    If the item is visible (i.e., QGraphicsItem::isVisible() returns true),
    QGraphicsScene will emit changed() once control goes back to the event
    loop.

    \sa addEllipse(), addLine(), addPath(), addRect(), addText(), addItem(),
    addWidget()
*/
QGraphicsPixmapItem *QGraphicsScene::addPixmap(const QPixmap &pixmap)
{
    QGraphicsPixmapItem *item = new QGraphicsPixmapItem(pixmap);
    addItem(item);
    return item;
}

/*!
    Creates and adds a polygon item to the scene, and returns the item
    pointer. The polygon is defined by \a polygon, and its pen and
    brush are initialized to \a pen and \a brush.

    Note that the item's geometry is provided in item coordinates, and its
    position is initialized to (0, 0).

    If the item is visible (i.e., QGraphicsItem::isVisible() returns true),
    QGraphicsScene will emit changed() once control goes back to the event
    loop.

    \sa addEllipse(), addLine(), addPath(), addRect(), addText(), addItem(),
    addWidget()
*/
QGraphicsPolygonItem *QGraphicsScene::addPolygon(const QPolygonF &polygon,
                                                 const QPen &pen, const QBrush &brush)
{
    QGraphicsPolygonItem *item = new QGraphicsPolygonItem(polygon);
    item->setPen(pen);
    item->setBrush(brush);
    addItem(item);
    return item;
}

/*!
    Creates and adds a rectangle item to the scene, and returns the item
    pointer. The geometry of the rectangle is defined by \a rect, and its pen
    and brush are initialized to \a pen and \a brush.

    Note that the item's geometry is provided in item coordinates, and its
    position is initialized to (0, 0). For example, if a QRect(50, 50, 100,
    100) is added, its top-left corner will be at (50, 50) relative to the
    origin in the items coordinate system.

    If the item is visible (i.e., QGraphicsItem::isVisible() returns true),
    QGraphicsScene will emit changed() once control goes back to the event
    loop.

    \sa addEllipse(), addLine(), addPixmap(), addPixmap(), addText(),
    addItem(), addWidget()
*/
QGraphicsRectItem *QGraphicsScene::addRect(const QRectF &rect, const QPen &pen, const QBrush &brush)
{
    QGraphicsRectItem *item = new QGraphicsRectItem(rect);
    item->setPen(pen);
    item->setBrush(brush);
    addItem(item);
    return item;
}

/*!
    \fn QGraphicsRectItem *QGraphicsScene::addRect(qreal x, qreal y, qreal w, qreal h, const QPen &pen, const QBrush &brush)
    \since 4.3

    This convenience function is equivalent to calling addRect(QRectF(\a x,
    \a y, \a w, \a h), \a pen, \a brush).
*/

/*!
    Creates and adds a text item to the scene, and returns the item
    pointer. The text string is initialized to \a text, and its font
    is initialized to \a font.

    The item's position is initialized to (0, 0).

    If the item is visible (i.e., QGraphicsItem::isVisible() returns true),
    QGraphicsScene will emit changed() once control goes back to the event
    loop.

    \sa addEllipse(), addLine(), addPixmap(), addPixmap(), addRect(),
    addItem(), addWidget()
*/
QGraphicsTextItem *QGraphicsScene::addText(const QString &text, const QFont &font)
{
    QGraphicsTextItem *item = new QGraphicsTextItem(text);
    item->setFont(font);
    addItem(item);
    return item;
}

/*!
    Creates and adds a QGraphicsSimpleTextItem to the scene, and returns the
    item pointer. The text string is initialized to \a text, and its font is
    initialized to \a font.

    The item's position is initialized to (0, 0).

    If the item is visible (i.e., QGraphicsItem::isVisible() returns true),
    QGraphicsScene will emit changed() once control goes back to the event
    loop.

    \sa addEllipse(), addLine(), addPixmap(), addPixmap(), addRect(),
    addItem(), addWidget()
*/
QGraphicsSimpleTextItem *QGraphicsScene::addSimpleText(const QString &text, const QFont &font)
{
    QGraphicsSimpleTextItem *item = new QGraphicsSimpleTextItem(text);
    item->setFont(font);
    addItem(item);
    return item;
}

/*!
    Creates a new QGraphicsProxyWidget for \a widget, adds it to the scene,
    and returns a pointer to the proxy. \a wFlags set the default window flags
    for the embedding proxy widget.

    The item's position is initialized to (0, 0).

    If the item is visible (i.e., QGraphicsItem::isVisible() returns true),
    QGraphicsScene will emit changed() once control goes back to the event
    loop.

    Note that widgets with the Qt::WA_PaintOnScreen widget attribute
    set and widgets that wrap an external application or controller
    are not supported. Examples are QGLWidget and QAxWidget.

    \sa addEllipse(), addLine(), addPixmap(), addPixmap(), addRect(),
    addText(), addSimpleText(), addItem()
*/
QGraphicsProxyWidget *QGraphicsScene::addWidget(QWidget *widget, Qt::WindowFlags wFlags)
{
    QGraphicsProxyWidget *proxy = new QGraphicsProxyWidget(0, wFlags);
    proxy->setWidget(widget);
    addItem(proxy);
    return proxy;
}

/*!
    Removes the item \a item and all its children from the scene.  The
    ownership of \a item is passed on to the caller (i.e.,
    QGraphicsScene will no longer delete \a item when destroyed).

    \sa addItem()
*/
void QGraphicsScene::removeItem(QGraphicsItem *item)
{
    // ### Refactoring: This function shares much functionality with _q_removeItemLater()
    Q_D(QGraphicsScene);
    if (!item) {
        qWarning("QGraphicsScene::removeItem: cannot remove 0-item");
        return;
    }
    if (item->scene() != this) {
        qWarning("QGraphicsScene::removeItem: item %p's scene (%p)"
                 " is different from this scene (%p)",
                 item, item->scene(), this);
        return;
    }

    // Notify the item that it's scene is changing to 0, allowing the item to
    // react.
    const QVariant newSceneVariant(item->itemChange(QGraphicsItem::ItemSceneChange,
                                                    QVariant::fromValue<QGraphicsScene *>(0)));
    QGraphicsScene *targetScene = qvariant_cast<QGraphicsScene *>(newSceneVariant);
    if (targetScene != 0 && targetScene != this) {
        targetScene->addItem(item);
        return;
    }

    d->removeItemHelper(item);

    // Deliver post-change notification
    item->itemChange(QGraphicsItem::ItemSceneHasChanged, newSceneVariant);

    d->updateInputMethodSensitivityInViews();
}

/*!
    When the scene is active, this functions returns the scene's current focus
    item, or 0 if no item currently has focus. When the scene is inactive, this
    functions returns the item that will gain input focus when the scene becomes
    active.

    The focus item receives keyboard input when the scene receives a
    key event.

    \sa setFocusItem(), QGraphicsItem::hasFocus(), isActive()
*/
QGraphicsItem *QGraphicsScene::focusItem() const
{
    Q_D(const QGraphicsScene);
    return isActive() ? d->focusItem : d->passiveFocusItem;
}

/*!
    Sets the scene's focus item to \a item, with the focus reason \a
    focusReason, after removing focus from any previous item that may have had
    focus.

    If \a item is 0, or if it either does not accept focus (i.e., it does not
    have the QGraphicsItem::ItemIsFocusable flag enabled), or is not visible
    or not enabled, this function only removes focus from any previous
    focusitem.

    If item is not 0, and the scene does not currently have focus (i.e.,
    hasFocus() returns false), this function will call setFocus()
    automatically.

    \sa focusItem(), hasFocus(), setFocus()
*/
void QGraphicsScene::setFocusItem(QGraphicsItem *item, Qt::FocusReason focusReason)
{
    Q_D(QGraphicsScene);
    if (item)
        item->setFocus(focusReason);
    else
        d->setFocusItemHelper(item, focusReason);
}

/*!
    Returns true if the scene has focus; otherwise returns false. If the scene
    has focus, it will will forward key events from QKeyEvent to any item that
    has focus.

    \sa setFocus(), setFocusItem()
*/
bool QGraphicsScene::hasFocus() const
{
    Q_D(const QGraphicsScene);
    return d->hasFocus;
}

/*!
    Sets focus on the scene by sending a QFocusEvent to the scene, passing \a
    focusReason as the reason. If the scene regains focus after having
    previously lost it while an item had focus, the last focus item will
    receive focus with \a focusReason as the reason.

    If the scene already has focus, this function does nothing.

    \sa hasFocus(), clearFocus(), setFocusItem()
*/
void QGraphicsScene::setFocus(Qt::FocusReason focusReason)
{
    Q_D(QGraphicsScene);
    if (d->hasFocus || !isActive())
        return;
    QFocusEvent event(QEvent::FocusIn, focusReason);
    QCoreApplication::sendEvent(this, &event);
}

/*!
    Clears focus from the scene. If any item has focus when this function is
    called, it will lose focus, and regain focus again once the scene regains
    focus.

    A scene that does not have focus ignores key events.

    \sa hasFocus(), setFocus(), setFocusItem()
*/
void QGraphicsScene::clearFocus()
{
    Q_D(QGraphicsScene);
    if (d->hasFocus) {
        d->hasFocus = false;
        d->passiveFocusItem = d->focusItem;
        setFocusItem(0, Qt::OtherFocusReason);
    }
}

/*!
    \property QGraphicsScene::stickyFocus
    \brief whether clicking into the scene background will clear focus

    \since 4.6

    In a QGraphicsScene with stickyFocus set to true, focus will remain
    unchanged when the user clicks into the scene background or on an item
    that does not accept focus. Otherwise, focus will be cleared.

    By default, this property is false.

    Focus changes in response to a mouse press. You can reimplement
    mousePressEvent() in a subclass of QGraphicsScene to toggle this property
    based on where the user has clicked.

    \sa clearFocus(), setFocusItem()
*/
void QGraphicsScene::setStickyFocus(bool enabled)
{
    Q_D(QGraphicsScene);
    d->stickyFocus = enabled;
}
bool QGraphicsScene::stickyFocus() const
{
    Q_D(const QGraphicsScene);
    return d->stickyFocus;
}

/*!
    Returns the current mouse grabber item, or 0 if no item is currently
    grabbing the mouse. The mouse grabber item is the item that receives all
    mouse events sent to the scene.

    An item becomes a mouse grabber when it receives and accepts a
    mouse press event, and it stays the mouse grabber until either of
    the following events occur:

    \list
    \o If the item receives a mouse release event when there are no other
    buttons pressed, it loses the mouse grab.
    \o If the item becomes invisible (i.e., someone calls \c {item->setVisible(false)}),
    or if it becomes disabled (i.e., someone calls \c {item->setEnabled(false)}),
    it loses the mouse grab.
    \o If the item is removed from the scene, it loses the mouse grab.
    \endlist

    If the item loses its mouse grab, the scene will ignore all mouse events
    until a new item grabs the mouse (i.e., until a new item receives a mouse
    press event).
*/
QGraphicsItem *QGraphicsScene::mouseGrabberItem() const
{
    Q_D(const QGraphicsScene);
    return !d->mouseGrabberItems.isEmpty() ? d->mouseGrabberItems.last() : 0;
}

/*!
    \property QGraphicsScene::backgroundBrush
    \brief the background brush of the scene.

    Set this property to changes the scene's background to a different color,
    gradient or texture. The default background brush is Qt::NoBrush. The
    background is drawn before (behind) the items.

    Example:

    \snippet doc/src/snippets/code/src_gui_graphicsview_qgraphicsscene.cpp 3

    QGraphicsScene::render() calls drawBackground() to draw the scene
    background. For more detailed control over how the background is drawn,
    you can reimplement drawBackground() in a subclass of QGraphicsScene.
*/
QBrush QGraphicsScene::backgroundBrush() const
{
    Q_D(const QGraphicsScene);
    return d->backgroundBrush;
}
void QGraphicsScene::setBackgroundBrush(const QBrush &brush)
{
    Q_D(QGraphicsScene);
    d->backgroundBrush = brush;
    foreach (QGraphicsView *view, d->views) {
        view->resetCachedContent();
        view->viewport()->update();
    }
    update();
}

/*!
    \property QGraphicsScene::foregroundBrush
    \brief the foreground brush of the scene.

    Change this property to set the scene's foreground to a different
    color, gradient or texture.

    The foreground is drawn after (on top of) the items. The default
    foreground brush is Qt::NoBrush ( i.e. the foreground is not
    drawn).

    Example:

    \snippet doc/src/snippets/code/src_gui_graphicsview_qgraphicsscene.cpp 4

    QGraphicsScene::render() calls drawForeground() to draw the scene
    foreground. For more detailed control over how the foreground is
    drawn, you can reimplement the drawForeground() function in a
    QGraphicsScene subclass.
*/
QBrush QGraphicsScene::foregroundBrush() const
{
    Q_D(const QGraphicsScene);
    return d->foregroundBrush;
}
void QGraphicsScene::setForegroundBrush(const QBrush &brush)
{
    Q_D(QGraphicsScene);
    d->foregroundBrush = brush;
    foreach (QGraphicsView *view, views())
        view->viewport()->update();
    update();
}

/*!
    This method is used by input methods to query a set of properties of
    the scene to be able to support complex input method operations as support
    for surrounding text and reconversions.

    The \a query parameter specifies which property is queried.

    \sa QWidget::inputMethodQuery()
*/
QVariant QGraphicsScene::inputMethodQuery(Qt::InputMethodQuery query) const
{
    Q_D(const QGraphicsScene);
    if (!d->focusItem || !(d->focusItem->flags() & QGraphicsItem::ItemAcceptsInputMethod))
        return QVariant();
    const QTransform matrix = d->focusItem->sceneTransform();
    QVariant value = d->focusItem->inputMethodQuery(query);
    if (value.type() == QVariant::RectF)
        value = matrix.mapRect(value.toRectF());
    else if (value.type() == QVariant::PointF)
        value = matrix.map(value.toPointF());
    else if (value.type() == QVariant::Rect)
        value = matrix.mapRect(value.toRect());
    else if (value.type() == QVariant::Point)
        value = matrix.map(value.toPoint());
    return value;
}

/*!
    \fn void QGraphicsScene::update(const QRectF &rect)
    Schedules a redraw of the area \a rect on the scene.

    \sa sceneRect(), changed()
*/
void QGraphicsScene::update(const QRectF &rect)
{
    Q_D(QGraphicsScene);
    if (d->updateAll || (rect.isEmpty() && !rect.isNull()))
        return;

    // Check if anyone's connected; if not, we can send updates directly to
    // the views. Otherwise or if there are no views, use old behavior.
    bool directUpdates = !(d->isSignalConnected(d->changedSignalIndex)) && !d->views.isEmpty();
    if (rect.isNull()) {
        d->updateAll = true;
        d->updatedRects.clear();
        if (directUpdates) {
            // Update all views.
            for (int i = 0; i < d->views.size(); ++i)
                d->views.at(i)->d_func()->fullUpdatePending = true;
        }
    } else {
        if (directUpdates) {
            // Update all views.
            for (int i = 0; i < d->views.size(); ++i) {
                QGraphicsView *view = d->views.at(i);
                if (view->isTransformed())
                    view->d_func()->updateRectF(view->viewportTransform().mapRect(rect));
                else
                    view->d_func()->updateRectF(rect);
            }
        } else {
            d->updatedRects << rect;
        }
    }

    if (!d->calledEmitUpdated) {
        d->calledEmitUpdated = true;
        QMetaObject::invokeMethod(this, "_q_emitUpdated", Qt::QueuedConnection);
    }
}

/*!
    \fn void QGraphicsScene::update(qreal x, qreal y, qreal w, qreal h)
    \overload
    \since 4.3

    This function is equivalent to calling update(QRectF(\a x, \a y, \a w,
    \a h));
*/

/*!
    Invalidates and schedules a redraw of the \a layers in \a rect on the
    scene. Any cached content in \a layers is unconditionally invalidated and
    redrawn.

    You can use this function overload to notify QGraphicsScene of changes to
    the background or the foreground of the scene. This function is commonly
    used for scenes with tile-based backgrounds to notify changes when
    QGraphicsView has enabled
    \l{QGraphicsView::CacheBackground}{CacheBackground}.

    Example:

    \snippet doc/src/snippets/code/src_gui_graphicsview_qgraphicsscene.cpp 5

    Note that QGraphicsView currently supports background caching only (see
    QGraphicsView::CacheBackground). This function is equivalent to calling
    update() if any layer but BackgroundLayer is passed.

    \sa QGraphicsView::resetCachedContent()
*/
void QGraphicsScene::invalidate(const QRectF &rect, SceneLayers layers)
{
    foreach (QGraphicsView *view, views())
        view->invalidateScene(rect, layers);
    update(rect);
}

/*!
    \fn void QGraphicsScene::invalidate(qreal x, qreal y, qreal w, qreal h, SceneLayers layers)
    \overload
    \since 4.3

    This convenience function is equivalent to calling invalidate(QRectF(\a x, \a
    y, \a w, \a h), \a layers);
*/

/*!
    Returns a list of all the views that display this scene.

    \sa QGraphicsView::scene()
*/
QList <QGraphicsView *> QGraphicsScene::views() const
{
    Q_D(const QGraphicsScene);
    return d->views;
}

/*!
    This slot \e advances the scene by one step, by calling
    QGraphicsItem::advance() for all items on the scene. This is done in two
    phases: in the first phase, all items are notified that the scene is about
    to change, and in the second phase all items are notified that they can
    move. In the first phase, QGraphicsItem::advance() is called passing a
    value of 0 as an argument, and 1 is passed in the second phase.

    \sa QGraphicsItem::advance(), QGraphicsItemAnimation, QTimeLine
*/
void QGraphicsScene::advance()
{
    for (int i = 0; i < 2; ++i) {
        foreach (QGraphicsItem *item, items())
            item->advance(i);
    }
}

/*!
    Processes the event \a event, and dispatches it to the respective
    event handlers.

    In addition to calling the convenience event handlers, this
    function is responsible for converting mouse move events to hover
    events for when there is no mouse grabber item. Hover events are
    delivered directly to items; there is no convenience function for
    them.

    Unlike QWidget, QGraphicsScene does not have the convenience functions
    \l{QWidget::}{enterEvent()} and \l{QWidget::}{leaveEvent()}. Use this
    function to obtain those events instead.

    \sa contextMenuEvent(), keyPressEvent(), keyReleaseEvent(),
    mousePressEvent(), mouseMoveEvent(), mouseReleaseEvent(),
    mouseDoubleClickEvent(), focusInEvent(), focusOutEvent()
*/
bool QGraphicsScene::event(QEvent *event)
{
    Q_D(QGraphicsScene);

    switch (event->type()) {
    case QEvent::GraphicsSceneMousePress:
    case QEvent::GraphicsSceneMouseMove:
    case QEvent::GraphicsSceneMouseRelease:
    case QEvent::GraphicsSceneMouseDoubleClick:
    case QEvent::GraphicsSceneHoverEnter:
    case QEvent::GraphicsSceneHoverLeave:
    case QEvent::GraphicsSceneHoverMove:
    case QEvent::TouchBegin:
    case QEvent::TouchUpdate:
    case QEvent::TouchEnd:
        // Reset the under-mouse list to ensure that this event gets fresh
        // item-under-mouse data. Be careful about this list; if people delete
        // items from inside event handlers, this list can quickly end up
        // having stale pointers in it. We need to clear it before dispatching
        // events that use it.
        // ### this should only be cleared if we received a new mouse move event,
        // which relies on us fixing the replay mechanism in QGraphicsView.
        d->cachedItemsUnderMouse.clear();
    default:
        break;
    }

    switch (event->type()) {
    case QEvent::GraphicsSceneDragEnter:
        dragEnterEvent(static_cast<QGraphicsSceneDragDropEvent *>(event));
        break;
    case QEvent::GraphicsSceneDragMove:
        dragMoveEvent(static_cast<QGraphicsSceneDragDropEvent *>(event));
        break;
    case QEvent::GraphicsSceneDragLeave:
        dragLeaveEvent(static_cast<QGraphicsSceneDragDropEvent *>(event));
        break;
    case QEvent::GraphicsSceneDrop:
        dropEvent(static_cast<QGraphicsSceneDragDropEvent *>(event));
        break;
    case QEvent::GraphicsSceneContextMenu:
        contextMenuEvent(static_cast<QGraphicsSceneContextMenuEvent *>(event));
        break;
    case QEvent::KeyPress:
        if (!d->focusItem) {
            QKeyEvent *k = static_cast<QKeyEvent *>(event);
            if (k->key() == Qt::Key_Tab || k->key() == Qt::Key_Backtab) {
                if (!(k->modifiers() & (Qt::ControlModifier | Qt::AltModifier))) {  //### Add MetaModifier?
                    bool res = false;
                    if (k->key() == Qt::Key_Backtab
                        || (k->key() == Qt::Key_Tab && (k->modifiers() & Qt::ShiftModifier))) {
                        res = focusNextPrevChild(false);
                    } else if (k->key() == Qt::Key_Tab) {
                        res = focusNextPrevChild(true);
                    }
                    if (!res)
                        event->ignore();
                    return true;
                }
            }
        }
        keyPressEvent(static_cast<QKeyEvent *>(event));
        break;
    case QEvent::KeyRelease:
        keyReleaseEvent(static_cast<QKeyEvent *>(event));
        break;
    case QEvent::ShortcutOverride: {
            QGraphicsItem *parent = focusItem();
            while (parent) {
                d->sendEvent(parent, event);
                if (event->isAccepted())
                    return true;
                parent = parent->parentItem();
            }
        }
        return false;
    case QEvent::GraphicsSceneMouseMove:
    {
        QGraphicsSceneMouseEvent *mouseEvent = static_cast<QGraphicsSceneMouseEvent *>(event);
        d->lastSceneMousePos = mouseEvent->scenePos();
        mouseMoveEvent(mouseEvent);
        break;
    }
    case QEvent::GraphicsSceneMousePress:
        mousePressEvent(static_cast<QGraphicsSceneMouseEvent *>(event));
        break;
    case QEvent::GraphicsSceneMouseRelease:
        mouseReleaseEvent(static_cast<QGraphicsSceneMouseEvent *>(event));
        break;
    case QEvent::GraphicsSceneMouseDoubleClick:
        mouseDoubleClickEvent(static_cast<QGraphicsSceneMouseEvent *>(event));
        break;
    case QEvent::GraphicsSceneWheel:
        wheelEvent(static_cast<QGraphicsSceneWheelEvent *>(event));
        break;
    case QEvent::FocusIn:
        focusInEvent(static_cast<QFocusEvent *>(event));
        break;
    case QEvent::FocusOut:
        focusOutEvent(static_cast<QFocusEvent *>(event));
        break;
    case QEvent::GraphicsSceneHoverEnter:
    case QEvent::GraphicsSceneHoverLeave:
    case QEvent::GraphicsSceneHoverMove:
    {
        QGraphicsSceneHoverEvent *hoverEvent = static_cast<QGraphicsSceneHoverEvent *>(event);
        d->lastSceneMousePos = hoverEvent->scenePos();
        d->dispatchHoverEvent(hoverEvent);
        break;
    }
    case QEvent::Leave:
        // hackieshly unpacking the viewport pointer from the leave event.
        d->leaveScene(reinterpret_cast<QWidget *>(event->d));
        break;
    case QEvent::GraphicsSceneHelp:
        helpEvent(static_cast<QGraphicsSceneHelpEvent *>(event));
        break;
    case QEvent::InputMethod:
        inputMethodEvent(static_cast<QInputMethodEvent *>(event));
        break;
    case QEvent::WindowActivate:
        if (!d->activationRefCount++) {
            if (d->lastActivePanel) {
                // Activate the last panel.
                d->setActivePanelHelper(d->lastActivePanel, true);
            } else if (d->tabFocusFirst && d->tabFocusFirst->isPanel()) {
                // Activate the panel of the first item in the tab focus
                // chain.
                d->setActivePanelHelper(d->tabFocusFirst, true);
            } else {
                // Activate all toplevel items.
                QEvent event(QEvent::WindowActivate);
                foreach (QGraphicsItem *item, items()) {
                    if (item->isVisible() && !item->isPanel() && !item->parentItem())
                        sendEvent(item, &event);
                }
            }
        }
        break;
    case QEvent::WindowDeactivate:
        if (d->activationRefCount > 0)
            --d->activationRefCount;
        if (!d->activationRefCount) {
            if (d->activePanel) {
                // Deactivate the active panel (but keep it so we can
                // reactivate it later).
                QGraphicsItem *lastActivePanel = d->activePanel;
                d->setActivePanelHelper(0, true);
                d->lastActivePanel = lastActivePanel;
            } else {
                // Activate all toplevel items.
                QEvent event(QEvent::WindowDeactivate);
                foreach (QGraphicsItem *item, items()) {
                    if (item->isVisible() && !item->isPanel() && !item->parentItem())
                        sendEvent(item, &event);
                }
            }
        }
        break;
    case QEvent::ApplicationFontChange: {
        // Resolve the existing scene font.
        d->resolveFont();
        break;
    }
    case QEvent::FontChange:
        // Update the entire scene when the font changes.
        update();
        break;
    case QEvent::ApplicationPaletteChange: {
        // Resolve the existing scene palette.
        d->resolvePalette();
        break;
    }
    case QEvent::PaletteChange:
        // Update the entire scene when the palette changes.
        update();
        break;
    case QEvent::StyleChange:
        // Reresolve all widgets' styles. Update all top-level widgets'
        // geometries that do not have an explicit style set.
        update();
        break;
    case QEvent::TouchBegin:
    case QEvent::TouchUpdate:
    case QEvent::TouchEnd:
        d->touchEventHandler(static_cast<QTouchEvent *>(event));
        break;
#ifndef QT_NO_GESTURES
    case QEvent::Gesture:
    case QEvent::GestureOverride:
        d->gestureEventHandler(static_cast<QGestureEvent *>(event));
        break;
#endif // QT_NO_GESTURES
    default:
        return QObject::event(event);
    }
    return true;
}

/*!
    \reimp

    QGraphicsScene filters QApplication's events to detect palette and font
    changes.
*/
bool QGraphicsScene::eventFilter(QObject *watched, QEvent *event)
{
    if (watched != qApp)
        return false;

    switch (event->type()) {
    case QEvent::ApplicationPaletteChange:
        QApplication::postEvent(this, new QEvent(QEvent::ApplicationPaletteChange));
        break;
    case QEvent::ApplicationFontChange:
        QApplication::postEvent(this, new QEvent(QEvent::ApplicationFontChange));
        break;
    default:
        break;
    }
    return false;
}

/*!
    This event handler, for event \a contextMenuEvent, can be reimplemented in
    a subclass to receive context menu events. The default implementation
    forwards the event to the topmost item that accepts context menu events at
    the position of the event. If no items accept context menu events at this
    position, the event is ignored.

    \sa QGraphicsItem::contextMenuEvent()
*/
void QGraphicsScene::contextMenuEvent(QGraphicsSceneContextMenuEvent *contextMenuEvent)
{
    Q_D(QGraphicsScene);
    // Ignore by default.
    contextMenuEvent->ignore();

    // Send the event to all items at this position until one item accepts the
    // event.
    foreach (QGraphicsItem *item, d->itemsAtPosition(contextMenuEvent->screenPos(),
                                                     contextMenuEvent->scenePos(),
                                                     contextMenuEvent->widget())) {
        contextMenuEvent->setPos(item->d_ptr->genericMapFromScene(contextMenuEvent->scenePos(),
                                                                  contextMenuEvent->widget()));
        contextMenuEvent->accept();
        if (!d->sendEvent(item, contextMenuEvent))
            break;

        if (contextMenuEvent->isAccepted())
            break;
    }
}

/*!
    This event handler, for event \a event, can be reimplemented in a subclass
    to receive drag enter events for the scene.

    The default implementation accepts the event and prepares the scene to
    accept drag move events.

    \sa QGraphicsItem::dragEnterEvent(), dragMoveEvent(), dragLeaveEvent(),
    dropEvent()
*/
void QGraphicsScene::dragEnterEvent(QGraphicsSceneDragDropEvent *event)
{
    Q_D(QGraphicsScene);
    d->dragDropItem = 0;
    d->lastDropAction = Qt::IgnoreAction;
    event->accept();
}

/*!
    This event handler, for event \a event, can be reimplemented in a subclass
    to receive drag move events for the scene.

    \sa QGraphicsItem::dragMoveEvent(), dragEnterEvent(), dragLeaveEvent(),
    dropEvent()
*/
void QGraphicsScene::dragMoveEvent(QGraphicsSceneDragDropEvent *event)
{
    Q_D(QGraphicsScene);
    event->ignore();

    if (!d->mouseGrabberItems.isEmpty()) {
        // Mouse grabbers that start drag events lose the mouse grab.
        d->clearMouseGrabber();
        d->mouseGrabberButtonDownPos.clear();
        d->mouseGrabberButtonDownScenePos.clear();
        d->mouseGrabberButtonDownScreenPos.clear();
    }

    bool eventDelivered = false;

    // Find the topmost enabled items under the cursor. They are all
    // candidates for accepting drag & drop events.
    foreach (QGraphicsItem *item, d->itemsAtPosition(event->screenPos(),
                                                     event->scenePos(),
                                                     event->widget())) {
        if (!item->isEnabled() || !item->acceptDrops())
            continue;

        if (item != d->dragDropItem) {
            // Enter the new drag drop item. If it accepts the event, we send
            // the leave to the parent item.
            QGraphicsSceneDragDropEvent dragEnter(QEvent::GraphicsSceneDragEnter);
            d->cloneDragDropEvent(&dragEnter, event);
            dragEnter.setDropAction(event->proposedAction());
            d->sendDragDropEvent(item, &dragEnter);
            event->setAccepted(dragEnter.isAccepted());
            event->setDropAction(dragEnter.dropAction());
            if (!event->isAccepted()) {
                // Propagate to the item under
                continue;
            }

            d->lastDropAction = event->dropAction();

            if (d->dragDropItem) {
                // Leave the last drag drop item. A perfect implementation
                // would set the position of this event to the point where
                // this event and the last event intersect with the item's
                // shape, but that's not easy to do. :-)
                QGraphicsSceneDragDropEvent dragLeave(QEvent::GraphicsSceneDragLeave);
                d->cloneDragDropEvent(&dragLeave, event);
                d->sendDragDropEvent(d->dragDropItem, &dragLeave);
            }

            // We've got a new drag & drop item
            d->dragDropItem = item;
        }

        // Send the move event.
        event->setDropAction(d->lastDropAction);
        event->accept();
        d->sendDragDropEvent(item, event);
        if (event->isAccepted())
            d->lastDropAction = event->dropAction();
        eventDelivered = true;
        break;
    }

    if (!eventDelivered) {
        if (d->dragDropItem) {
            // Leave the last drag drop item
            QGraphicsSceneDragDropEvent dragLeave(QEvent::GraphicsSceneDragLeave);
            d->cloneDragDropEvent(&dragLeave, event);
            d->sendDragDropEvent(d->dragDropItem, &dragLeave);
            d->dragDropItem = 0;
        }
        // Propagate
        event->setDropAction(Qt::IgnoreAction);
    }
}

/*!
    This event handler, for event \a event, can be reimplemented in a subclass
    to receive drag leave events for the scene.

    \sa QGraphicsItem::dragLeaveEvent(), dragEnterEvent(), dragMoveEvent(),
    dropEvent()
*/
void QGraphicsScene::dragLeaveEvent(QGraphicsSceneDragDropEvent *event)
{
    Q_D(QGraphicsScene);
    if (d->dragDropItem) {
        // Leave the last drag drop item
        d->sendDragDropEvent(d->dragDropItem, event);
        d->dragDropItem = 0;
    }
}

/*!
    This event handler, for event \a event, can be reimplemented in a subclass
    to receive drop events for the scene.

    \sa QGraphicsItem::dropEvent(), dragEnterEvent(), dragMoveEvent(),
    dragLeaveEvent()
*/
void QGraphicsScene::dropEvent(QGraphicsSceneDragDropEvent *event)
{
    Q_UNUSED(event);
    Q_D(QGraphicsScene);
    if (d->dragDropItem) {
        // Drop on the last drag drop item
        d->sendDragDropEvent(d->dragDropItem, event);
        d->dragDropItem = 0;
    }
}

/*!
    This event handler, for event \a focusEvent, can be reimplemented in a
    subclass to receive focus in events.

    The default implementation sets focus on the scene, and then on the last
    focus item.

    \sa QGraphicsItem::focusOutEvent()
*/
void QGraphicsScene::focusInEvent(QFocusEvent *focusEvent)
{
    Q_D(QGraphicsScene);

    d->hasFocus = true;
    switch (focusEvent->reason()) {
    case Qt::TabFocusReason:
        if (!focusNextPrevChild(true))
            focusEvent->ignore();
        break;
    case Qt::BacktabFocusReason:
        if (!focusNextPrevChild(false))
            focusEvent->ignore();
        break;
    default:
        if (d->passiveFocusItem) {
            // Set focus on the last focus item
            setFocusItem(d->passiveFocusItem, focusEvent->reason());
        }
        break;
    }
}

/*!
    This event handler, for event \a focusEvent, can be reimplemented in a
    subclass to receive focus out events.

    The default implementation removes focus from any focus item, then removes
    focus from the scene.

    \sa QGraphicsItem::focusInEvent()
*/
void QGraphicsScene::focusOutEvent(QFocusEvent *focusEvent)
{
    Q_D(QGraphicsScene);
    d->hasFocus = false;
    d->passiveFocusItem = d->focusItem;
    setFocusItem(0, focusEvent->reason());

    // Remove all popups when the scene loses focus.
    if (!d->popupWidgets.isEmpty())
        d->removePopup(d->popupWidgets.first());
}

/*!
    This event handler, for event \a helpEvent, can be
    reimplemented in a subclass to receive help events. The events
    are of type QEvent::ToolTip, which are created when a tooltip is
    requested.

    The default implementation shows the tooltip of the topmost
    item, i.e., the item with the highest z-value, at the mouse
    cursor position. If no item has a tooltip set, this function
    does nothing.

   \sa QGraphicsItem::toolTip(), QGraphicsSceneHelpEvent
*/
void QGraphicsScene::helpEvent(QGraphicsSceneHelpEvent *helpEvent)
{
#ifdef QT_NO_TOOLTIP
    Q_UNUSED(helpEvent);
#else
    // Find the first item that does tooltips
    Q_D(QGraphicsScene);
    QList<QGraphicsItem *> itemsAtPos = d->itemsAtPosition(helpEvent->screenPos(),
                                                           helpEvent->scenePos(),
                                                           helpEvent->widget());
    QGraphicsItem *toolTipItem = 0;
    for (int i = 0; i < itemsAtPos.size(); ++i) {
        QGraphicsItem *tmp = itemsAtPos.at(i);
        if (tmp->d_func()->isProxyWidget()) {
            // if the item is a proxy widget, the event is forwarded to it
            sendEvent(tmp, helpEvent);
            if (helpEvent->isAccepted())
                return;
        }
        if (!tmp->toolTip().isEmpty()) {
            toolTipItem = tmp;
            break;
        }
    }

    // Show or hide the tooltip
    QString text;
    QPoint point;
    if (toolTipItem && !toolTipItem->toolTip().isEmpty()) {
        text = toolTipItem->toolTip();
        point = helpEvent->screenPos();
    }
    QToolTip::showText(point, text, helpEvent->widget());
    helpEvent->setAccepted(!text.isEmpty());
#endif
}

bool QGraphicsScenePrivate::itemAcceptsHoverEvents_helper(const QGraphicsItem *item) const
{
    return (item->d_ptr->acceptsHover
            || (item->d_ptr->isWidget
                && static_cast<const QGraphicsWidget *>(item)->d_func()->hasDecoration()))
           && !item->isBlockedByModalPanel();
}

/*!
    This event handler, for event \a hoverEvent, can be reimplemented in a
    subclass to receive hover enter events. The default implementation
    forwards the event to the topmost item that accepts hover events at the
    scene position from the event.

    \sa QGraphicsItem::hoverEvent(), QGraphicsItem::setAcceptHoverEvents()
*/
bool QGraphicsScenePrivate::dispatchHoverEvent(QGraphicsSceneHoverEvent *hoverEvent)
{
    if (allItemsIgnoreHoverEvents)
        return false;

    // Find the first item that accepts hover events, reusing earlier
    // calculated data is possible.
    if (cachedItemsUnderMouse.isEmpty()) {
        cachedItemsUnderMouse = itemsAtPosition(hoverEvent->screenPos(),
                                                hoverEvent->scenePos(),
                                                hoverEvent->widget());
    }

    QGraphicsItem *item = 0;
    for (int i = 0; i < cachedItemsUnderMouse.size(); ++i) {
        QGraphicsItem *tmp = cachedItemsUnderMouse.at(i);
        if (itemAcceptsHoverEvents_helper(tmp)) {
            item = tmp;
            break;
        }
    }

    // Find the common ancestor item for the new topmost hoverItem and the
    // last item in the hoverItem list.
    QGraphicsItem *commonAncestorItem = (item && !hoverItems.isEmpty()) ? item->commonAncestorItem(hoverItems.last()) : 0;
    while (commonAncestorItem && !itemAcceptsHoverEvents_helper(commonAncestorItem))
        commonAncestorItem = commonAncestorItem->parentItem();
    if (commonAncestorItem && commonAncestorItem->panel() != item->panel()) {
        // The common ancestor isn't in the same panel as the two hovered
        // items.
        commonAncestorItem = 0;
    }

    // Check if the common ancestor item is known.
    int index = commonAncestorItem ? hoverItems.indexOf(commonAncestorItem) : -1;
    // Send hover leaves to any existing hovered children of the common
    // ancestor item.
    for (int i = hoverItems.size() - 1; i > index; --i) {
        QGraphicsItem *lastItem = hoverItems.takeLast();
        if (itemAcceptsHoverEvents_helper(lastItem))
            sendHoverEvent(QEvent::GraphicsSceneHoverLeave, lastItem, hoverEvent);
    }

    // Item is a child of a known item. Generate enter events for the
    // missing links.
    QList<QGraphicsItem *> parents;
    QGraphicsItem *parent = item;
    while (parent && parent != commonAncestorItem) {
        parents.prepend(parent);
        if (parent->isPanel()) {
            // Stop at the panel - we don't deliver beyond this point.
            break;
        }
        parent = parent->parentItem();
    }
    for (int i = 0; i < parents.size(); ++i) {
        parent = parents.at(i);
        hoverItems << parent;
        if (itemAcceptsHoverEvents_helper(parent))
            sendHoverEvent(QEvent::GraphicsSceneHoverEnter, parent, hoverEvent);
    }

    // Generate a move event for the item itself
    if (item
        && !hoverItems.isEmpty()
        && item == hoverItems.last()) {
        sendHoverEvent(QEvent::GraphicsSceneHoverMove, item, hoverEvent);
        return true;
    }
    return false;
}

/*!
    \internal

    Handles all actions necessary to clean up the scene when the mouse leaves
    the view.
*/
void QGraphicsScenePrivate::leaveScene(QWidget *viewport)
{
#ifndef QT_NO_TOOLTIP
    QToolTip::hideText();
#endif
    QGraphicsView *view = qobject_cast<QGraphicsView *>(viewport->parent());
    // Send HoverLeave events to all existing hover items, topmost first.
    QGraphicsSceneHoverEvent hoverEvent;
    hoverEvent.setWidget(viewport);

    if (view) {
        QPoint cursorPos = QCursor::pos();
        hoverEvent.setScenePos(view->mapToScene(viewport->mapFromGlobal(cursorPos)));
        hoverEvent.setLastScenePos(hoverEvent.scenePos());
        hoverEvent.setScreenPos(cursorPos);
        hoverEvent.setLastScreenPos(hoverEvent.screenPos());
    }

    while (!hoverItems.isEmpty()) {
        QGraphicsItem *lastItem = hoverItems.takeLast();
        if (itemAcceptsHoverEvents_helper(lastItem))
            sendHoverEvent(QEvent::GraphicsSceneHoverLeave, lastItem, &hoverEvent);
    }
}

/*!
    This event handler, for event \a keyEvent, can be reimplemented in a
    subclass to receive keypress events. The default implementation forwards
    the event to current focus item.

    \sa QGraphicsItem::keyPressEvent(), focusItem()
*/
void QGraphicsScene::keyPressEvent(QKeyEvent *keyEvent)
{
    // ### Merge this function with keyReleaseEvent; they are identical
    // ### (except this comment).
    Q_D(QGraphicsScene);
    QGraphicsItem *item = !d->keyboardGrabberItems.isEmpty() ? d->keyboardGrabberItems.last() : 0;
    if (!item)
        item = focusItem();
    if (item) {
        QGraphicsItem *p = item;
        do {
            // Accept the event by default
            keyEvent->accept();
            // Send it; QGraphicsItem::keyPressEvent ignores it.  If the event
            // is filtered out, stop propagating it.
            if (p->isBlockedByModalPanel())
                break;
            if (!d->sendEvent(p, keyEvent))
                break;
        } while (!keyEvent->isAccepted() && !p->isPanel() && (p = p->parentItem()));
    } else {
        keyEvent->ignore();
    }
}

/*!
    This event handler, for event \a keyEvent, can be reimplemented in a
    subclass to receive key release events. The default implementation
    forwards the event to current focus item.

    \sa QGraphicsItem::keyReleaseEvent(), focusItem()
*/
void QGraphicsScene::keyReleaseEvent(QKeyEvent *keyEvent)
{
    // ### Merge this function with keyPressEvent; they are identical (except
    // ### this comment).
    Q_D(QGraphicsScene);
    QGraphicsItem *item = !d->keyboardGrabberItems.isEmpty() ? d->keyboardGrabberItems.last() : 0;
    if (!item)
        item = focusItem();
    if (item) {
        QGraphicsItem *p = item;
        do {
            // Accept the event by default
            keyEvent->accept();
            // Send it; QGraphicsItem::keyPressEvent ignores it.  If the event
            // is filtered out, stop propagating it.
            if (p->isBlockedByModalPanel())
                break;
            if (!d->sendEvent(p, keyEvent))
                break;
        } while (!keyEvent->isAccepted() && !p->isPanel() && (p = p->parentItem()));
    } else {
        keyEvent->ignore();
    }
}

/*!
    This event handler, for event \a mouseEvent, can be reimplemented
    in a subclass to receive mouse press events for the scene.

    The default implementation depends on the state of the scene. If
    there is a mouse grabber item, then the event is sent to the mouse
    grabber. Otherwise, it is forwarded to the topmost item that
    accepts mouse events at the scene position from the event, and
    that item promptly becomes the mouse grabber item.

    If there is no item at the given position on the scene, the
    selection area is reset, any focus item loses its input focus, and
    the event is then ignored.

    \sa QGraphicsItem::mousePressEvent(),
    QGraphicsItem::setAcceptedMouseButtons()
*/
void QGraphicsScene::mousePressEvent(QGraphicsSceneMouseEvent *mouseEvent)
{
    Q_D(QGraphicsScene);
    if (d->mouseGrabberItems.isEmpty()) {
        // Dispatch hover events
        QGraphicsSceneHoverEvent hover;
        _q_hoverFromMouseEvent(&hover, mouseEvent);
        d->dispatchHoverEvent(&hover);
    }

    d->mousePressEventHandler(mouseEvent);
}

/*!
    This event handler, for event \a mouseEvent, can be reimplemented
    in a subclass to receive mouse move events for the scene.

    The default implementation depends on the mouse grabber state. If there is
    a mouse grabber item, the event is sent to the mouse grabber.  If there
    are any items that accept hover events at the current position, the event
    is translated into a hover event and accepted; otherwise it's ignored.

    \sa QGraphicsItem::mousePressEvent(), QGraphicsItem::mouseReleaseEvent(),
    QGraphicsItem::mouseDoubleClickEvent(), QGraphicsItem::setAcceptedMouseButtons()
*/
void QGraphicsScene::mouseMoveEvent(QGraphicsSceneMouseEvent *mouseEvent)
{
    Q_D(QGraphicsScene);
    if (d->mouseGrabberItems.isEmpty()) {
        if (mouseEvent->buttons())
            return;
        QGraphicsSceneHoverEvent hover;
        _q_hoverFromMouseEvent(&hover, mouseEvent);
        mouseEvent->setAccepted(d->dispatchHoverEvent(&hover));
        return;
    }

    // Forward the event to the mouse grabber
    d->sendMouseEvent(mouseEvent);
    mouseEvent->accept();
}

/*!
    This event handler, for event \a mouseEvent, can be reimplemented
    in a subclass to receive mouse release events for the scene.

    The default implementation depends on the mouse grabber state.  If
    there is no mouse grabber, the event is ignored.  Otherwise, if
    there is a mouse grabber item, the event is sent to the mouse
    grabber. If this mouse release represents the last pressed button
    on the mouse, the mouse grabber item then loses the mouse grab.

    \sa QGraphicsItem::mousePressEvent(), QGraphicsItem::mouseMoveEvent(),
    QGraphicsItem::mouseDoubleClickEvent(), QGraphicsItem::setAcceptedMouseButtons()
*/
void QGraphicsScene::mouseReleaseEvent(QGraphicsSceneMouseEvent *mouseEvent)
{
    Q_D(QGraphicsScene);
    if (d->mouseGrabberItems.isEmpty()) {
        mouseEvent->ignore();
        return;
    }

    // Forward the event to the mouse grabber
    d->sendMouseEvent(mouseEvent);
    mouseEvent->accept();

    // Reset the mouse grabber when the last mouse button has been released.
    if (!mouseEvent->buttons()) {
        if (!d->mouseGrabberItems.isEmpty()) {
            d->lastMouseGrabberItem = d->mouseGrabberItems.last();
            if (d->lastMouseGrabberItemHasImplicitMouseGrab)
                d->mouseGrabberItems.last()->ungrabMouse();
        } else {
            d->lastMouseGrabberItem = 0;
        }

        // Generate a hoverevent
        QGraphicsSceneHoverEvent hoverEvent;
        _q_hoverFromMouseEvent(&hoverEvent, mouseEvent);
        d->dispatchHoverEvent(&hoverEvent);
    }
}

/*!
    This event handler, for event \a mouseEvent, can be reimplemented
    in a subclass to receive mouse doubleclick events for the scene.

    If someone doubleclicks on the scene, the scene will first receive
    a mouse press event, followed by a release event (i.e., a click),
    then a doubleclick event, and finally a release event. If the
    doubleclick event is delivered to a different item than the one
    that received the first press and release, it will be delivered as
    a press event. However, tripleclick events are not delivered as
    doubleclick events in this case.

    The default implementation is similar to mousePressEvent().

    \sa QGraphicsItem::mousePressEvent(), QGraphicsItem::mouseMoveEvent(),
    QGraphicsItem::mouseReleaseEvent(), QGraphicsItem::setAcceptedMouseButtons()
*/
void QGraphicsScene::mouseDoubleClickEvent(QGraphicsSceneMouseEvent *mouseEvent)
{
    Q_D(QGraphicsScene);
    d->mousePressEventHandler(mouseEvent);
}

/*!
    This event handler, for event \a wheelEvent, can be reimplemented in a
    subclass to receive mouse wheel events for the scene.

    By default, the event is delivered to the topmost visible item under the
    cursor. If ignored, the event propagates to the item beneath, and again
    until the event is accepted, or it reaches the scene. If no items accept
    the event, it is ignored.

    \sa QGraphicsItem::wheelEvent()
*/
void QGraphicsScene::wheelEvent(QGraphicsSceneWheelEvent *wheelEvent)
{
    Q_D(QGraphicsScene);
    QList<QGraphicsItem *> wheelCandidates = d->itemsAtPosition(wheelEvent->screenPos(),
                                                                wheelEvent->scenePos(),
                                                                wheelEvent->widget());

#ifdef Q_WS_MAC
    // On Mac, ignore the event if the first item under the mouse is not the last opened
    // popup (or one of its descendant)
    if (!d->popupWidgets.isEmpty() && !wheelCandidates.isEmpty() && wheelCandidates.first() != d->popupWidgets.back() && !d->popupWidgets.back()->isAncestorOf(wheelCandidates.first())) {
        wheelEvent->accept();
        return;
    }
#else
    // Find the first popup under the mouse (including the popup's descendants) starting from the last.
    // Remove all popups after the one found, or all or them if no popup is under the mouse.
    // Then continue with the event.
    QList<QGraphicsWidget *>::const_iterator iter = d->popupWidgets.end();
    while (--iter >= d->popupWidgets.begin() && !wheelCandidates.isEmpty()) {
        if (wheelCandidates.first() == *iter || (*iter)->isAncestorOf(wheelCandidates.first()))
            break;
        d->removePopup(*iter);
    }
#endif

    bool hasSetFocus = false;
    foreach (QGraphicsItem *item, wheelCandidates) {
        if (!hasSetFocus && item->isEnabled()
            && ((item->flags() & QGraphicsItem::ItemIsFocusable) && item->d_ptr->mouseSetsFocus)) {
            if (item->isWidget() && static_cast<QGraphicsWidget *>(item)->focusPolicy() == Qt::WheelFocus) {
                hasSetFocus = true;
                if (item != focusItem())
                    setFocusItem(item, Qt::MouseFocusReason);
            }
        }

        wheelEvent->setPos(item->d_ptr->genericMapFromScene(wheelEvent->scenePos(),
                                                            wheelEvent->widget()));
        wheelEvent->accept();
        bool isPanel = item->isPanel();
        d->sendEvent(item, wheelEvent);
        if (isPanel || wheelEvent->isAccepted())
            break;
    }
}

/*!
    This event handler, for event \a event, can be reimplemented in a
    subclass to receive input method events for the scene.

    The default implementation forwards the event to the focusItem().
    If no item currently has focus or the current focus item does not
    accept input methods, this function does nothing.

    \sa QGraphicsItem::inputMethodEvent()
*/
void QGraphicsScene::inputMethodEvent(QInputMethodEvent *event)
{
    Q_D(QGraphicsScene);
    if (d->focusItem && (d->focusItem->flags() & QGraphicsItem::ItemAcceptsInputMethod))
        d->sendEvent(d->focusItem, event);
}

/*!
    Draws the background of the scene using \a painter, before any items and
    the foreground are drawn. Reimplement this function to provide a custom
    background for the scene.

    All painting is done in \e scene coordinates. The \a rect
    parameter is the exposed rectangle.

    If all you want is to define a color, texture, or gradient for the
    background, you can call setBackgroundBrush() instead.

    \sa drawForeground(), drawItems()
*/
void QGraphicsScene::drawBackground(QPainter *painter, const QRectF &rect)
{
    Q_D(QGraphicsScene);

    if (d->backgroundBrush.style() != Qt::NoBrush) {
        if (d->painterStateProtection)
            painter->save();
        painter->setBrushOrigin(0, 0);
        painter->fillRect(rect, backgroundBrush());
        if (d->painterStateProtection)
            painter->restore();
    }
}

/*!
    Draws the foreground of the scene using \a painter, after the background
    and all items have been drawn. Reimplement this function to provide a
    custom foreground for the scene.

    All painting is done in \e scene coordinates. The \a rect
    parameter is the exposed rectangle.

    If all you want is to define a color, texture or gradient for the
    foreground, you can call setForegroundBrush() instead.

    \sa drawBackground(), drawItems()
*/
void QGraphicsScene::drawForeground(QPainter *painter, const QRectF &rect)
{
    Q_D(QGraphicsScene);

    if (d->foregroundBrush.style() != Qt::NoBrush) {
        if (d->painterStateProtection)
            painter->save();
        painter->setBrushOrigin(0, 0);
        painter->fillRect(rect, foregroundBrush());
        if (d->painterStateProtection)
            painter->restore();
    }
}

static void _q_paintItem(QGraphicsItem *item, QPainter *painter,
                         const QStyleOptionGraphicsItem *option, QWidget *widget,
                         bool useWindowOpacity, bool painterStateProtection)
{
    if (!item->isWidget()) {
        item->paint(painter, option, widget);
        return;
    }
    QGraphicsWidget *widgetItem = static_cast<QGraphicsWidget *>(item);
    QGraphicsProxyWidget *proxy = qobject_cast<QGraphicsProxyWidget *>(widgetItem);
    const qreal windowOpacity = (proxy && proxy->widget() && useWindowOpacity)
                                ? proxy->widget()->windowOpacity() : qreal(1.0);
    const qreal oldPainterOpacity = painter->opacity();

    if (qFuzzyIsNull(windowOpacity))
        return;
    // Set new painter opacity.
    if (windowOpacity < 1.0)
        painter->setOpacity(oldPainterOpacity * windowOpacity);

    // set layoutdirection on the painter
    Qt::LayoutDirection oldLayoutDirection = painter->layoutDirection();
    painter->setLayoutDirection(widgetItem->layoutDirection());

    if (widgetItem->isWindow() && widgetItem->windowType() != Qt::Popup && widgetItem->windowType() != Qt::ToolTip
        && !(widgetItem->windowFlags() & Qt::FramelessWindowHint)) {
        if (painterStateProtection)
            painter->save();
        widgetItem->paintWindowFrame(painter, option, widget);
        if (painterStateProtection)
            painter->restore();
    } else if (widgetItem->autoFillBackground()) {
        painter->fillRect(option->exposedRect, widgetItem->palette().window());
    }

    widgetItem->paint(painter, option, widget);

    // Restore layoutdirection on the painter.
    painter->setLayoutDirection(oldLayoutDirection);
    // Restore painter opacity.
    if (windowOpacity < 1.0)
        painter->setOpacity(oldPainterOpacity);
}

static void _q_paintIntoCache(QPixmap *pix, QGraphicsItem *item, const QRegion &pixmapExposed,
                              const QTransform &itemToPixmap, QPainter::RenderHints renderHints,
                              const QStyleOptionGraphicsItem *option, bool painterStateProtection)
{
    QPixmap subPix;
    QPainter pixmapPainter;
    QRect br = pixmapExposed.boundingRect();

    // Don't use subpixmap if we get a full update.
    if (pixmapExposed.isEmpty() || (pixmapExposed.rectCount() == 1 && br.contains(pix->rect()))) {
        pix->fill(Qt::transparent);
        pixmapPainter.begin(pix);
    } else {
        subPix = QPixmap(br.size());
        subPix.fill(Qt::transparent);
        pixmapPainter.begin(&subPix);
        pixmapPainter.translate(-br.topLeft());
        if (!pixmapExposed.isEmpty()) {
            // Applied to subPix; paint is adjusted to the coordinate space is
            // correct.
            pixmapPainter.setClipRegion(pixmapExposed);
        }
    }

    pixmapPainter.setRenderHints(pixmapPainter.renderHints(), false);
    pixmapPainter.setRenderHints(renderHints, true);
    pixmapPainter.setWorldTransform(itemToPixmap, true);

    // Render.
    _q_paintItem(item, &pixmapPainter, option, 0, false, painterStateProtection);
    pixmapPainter.end();

    if (!subPix.isNull()) {
        // Blit the subpixmap into the main pixmap.
        pixmapPainter.begin(pix);
        pixmapPainter.setCompositionMode(QPainter::CompositionMode_Source);
        pixmapPainter.setClipRegion(pixmapExposed);
        pixmapPainter.drawPixmap(br.topLeft(), subPix);
        pixmapPainter.end();
    }
}

// Copied from qpaintengine_vg.cpp
// Returns true for 90, 180, and 270 degree rotations.
static inline bool transformIsSimple(const QTransform& transform)
{
    QTransform::TransformationType type = transform.type();
    if (type <= QTransform::TxScale) {
        return true;
    } else if (type == QTransform::TxRotate) {
        // Check for 90, and 270 degree rotations.
        qreal m11 = transform.m11();
        qreal m12 = transform.m12();
        qreal m21 = transform.m21();
        qreal m22 = transform.m22();
        if (m11 == 0.0f && m22 == 0.0f) {
            if (m12 == 1.0f && m21 == -1.0f)
                return true; // 90 degrees.
            else if (m12 == -1.0f && m21 == 1.0f)
                return true; // 270 degrees.
            else if (m12 == -1.0f && m21 == -1.0f)
                return true; // 90 degrees inverted y.
            else if (m12 == 1.0f && m21 == 1.0f)
                return true; // 270 degrees inverted y.
        }
    }
    return false;
}

/*!
    \internal

    Draws items directly, or using cache.
*/
void QGraphicsScenePrivate::drawItemHelper(QGraphicsItem *item, QPainter *painter,
                                           const QStyleOptionGraphicsItem *option, QWidget *widget,
                                           bool painterStateProtection)
{
    QGraphicsItemPrivate *itemd = item->d_ptr.data();
    QGraphicsItem::CacheMode cacheMode = QGraphicsItem::CacheMode(itemd->cacheMode);

    // Render directly, using no cache.
    if (cacheMode == QGraphicsItem::NoCache
#ifdef Q_WS_X11
        || !X11->use_xrender
#endif
        ) {
        _q_paintItem(static_cast<QGraphicsWidget *>(item), painter, option, widget, true, painterStateProtection);
        return;
    }

    const qreal oldPainterOpacity = painter->opacity();
    qreal newPainterOpacity = oldPainterOpacity;
    QGraphicsProxyWidget *proxy = item->isWidget() ? qobject_cast<QGraphicsProxyWidget *>(static_cast<QGraphicsWidget *>(item)) : 0;
    if (proxy && proxy->widget()) {
        const qreal windowOpacity = proxy->widget()->windowOpacity();
        if (windowOpacity < 1.0)
            newPainterOpacity *= windowOpacity;
    }

    // Item's (local) bounding rect
    QRectF brect = item->boundingRect();
    QRectF adjustedBrect(brect);
    _q_adjustRect(&adjustedBrect);
    if (adjustedBrect.isEmpty())
        return;

    // Fetch the off-screen transparent buffer and exposed area info.
    QPixmapCache::Key pixmapKey;
    QPixmap pix;
    bool pixmapFound;
    QGraphicsItemCache *itemCache = itemd->extraItemCache();
    if (cacheMode == QGraphicsItem::ItemCoordinateCache) {
        pixmapKey = itemCache->key;
    } else {
        pixmapKey = itemCache->deviceData.value(widget).key;
    }

    // Find pixmap in cache.
    pixmapFound = QPixmapCache::find(pixmapKey, &pix);

    // Render using item coordinate cache mode.
    if (cacheMode == QGraphicsItem::ItemCoordinateCache) {
        QSize pixmapSize;
        bool fixedCacheSize = false;
        QRect br = brect.toAlignedRect();
        if ((fixedCacheSize = itemCache->fixedSize.isValid())) {
            pixmapSize = itemCache->fixedSize;
        } else {
            pixmapSize = br.size();
        }

        // Create or recreate the pixmap.
        int adjust = itemCache->fixedSize.isValid() ? 0 : 2;
        QSize adjustSize(adjust*2, adjust*2);
        br.adjust(-adjust, -adjust, adjust, adjust);
        if (pix.isNull() || (!fixedCacheSize && (pixmapSize + adjustSize) != pix.size())) {
            pix = QPixmap(pixmapSize + adjustSize);
            itemCache->boundingRect = br;
            itemCache->exposed.clear();
            itemCache->allExposed = true;
        } else if (itemCache->boundingRect != br) {
            itemCache->boundingRect = br;
            itemCache->exposed.clear();
            itemCache->allExposed = true;
        }

        // Redraw any newly exposed areas.
        if (itemCache->allExposed || !itemCache->exposed.isEmpty()) {

            //We know that we will modify the pixmap, removing it from the cache
            //will detach the one we have and avoid a deep copy
            if (pixmapFound)
                QPixmapCache::remove(pixmapKey);

            // Fit the item's bounding rect into the pixmap's coordinates.
            QTransform itemToPixmap;
            if (fixedCacheSize) {
                const QPointF scale(pixmapSize.width() / brect.width(), pixmapSize.height() / brect.height());
                itemToPixmap.scale(scale.x(), scale.y());
            }
            itemToPixmap.translate(-br.x(), -br.y());

            // Generate the item's exposedRect and map its list of expose
            // rects to device coordinates.
            styleOptionTmp = *option;
            QRegion pixmapExposed;
            QRectF exposedRect;
            if (!itemCache->allExposed) {
                for (int i = 0; i < itemCache->exposed.size(); ++i) {
                    QRectF r = itemCache->exposed.at(i);
                    exposedRect |= r;
                    pixmapExposed += itemToPixmap.mapRect(r).toAlignedRect();
                }
            } else {
                exposedRect = brect;
            }
            styleOptionTmp.exposedRect = exposedRect;

            // Render.
            _q_paintIntoCache(&pix, item, pixmapExposed, itemToPixmap, painter->renderHints(),
                              &styleOptionTmp, painterStateProtection);

            // insert this pixmap into the cache.
            itemCache->key = QPixmapCache::insert(pix);

            // Reset expose data.
            itemCache->allExposed = false;
            itemCache->exposed.clear();
        }

        // Redraw the exposed area using the transformed painter. Depending on
        // the hardware, this may be a server-side operation, or an expensive
        // qpixmap-image-transform-pixmap roundtrip.
        if (newPainterOpacity != oldPainterOpacity) {
            painter->setOpacity(newPainterOpacity);
            painter->drawPixmap(br.topLeft(), pix);
            painter->setOpacity(oldPainterOpacity);
        } else {
            painter->drawPixmap(br.topLeft(), pix);
        }
        return;
    }

    // Render using device coordinate cache mode.
    if (cacheMode == QGraphicsItem::DeviceCoordinateCache) {
        // Find the item's bounds in device coordinates.
        QRectF deviceBounds = painter->worldTransform().mapRect(brect);
        QRect deviceRect = deviceBounds.toRect().adjusted(-1, -1, 1, 1);
        if (deviceRect.isEmpty())
            return;
        QRect viewRect = widget ? widget->rect() : QRect();
        if (widget && !viewRect.intersects(deviceRect))
            return;

        // Resort to direct rendering if the device rect exceeds the
        // (optional) maximum bounds. (QGraphicsSvgItem uses this).
        QSize maximumCacheSize =
            itemd->extra(QGraphicsItemPrivate::ExtraMaxDeviceCoordCacheSize).toSize();
        if (!maximumCacheSize.isEmpty()
            && (deviceRect.width() > maximumCacheSize.width()
                || deviceRect.height() > maximumCacheSize.height())) {
            _q_paintItem(static_cast<QGraphicsWidget *>(item), painter, option, widget,
                         oldPainterOpacity != newPainterOpacity, painterStateProtection);
            return;
        }

        // Create or reuse offscreen pixmap, possibly scroll/blit from the old one.
        // If the world transform is rotated we always recreate the cache to avoid
        // wrong blending.
        bool pixModified = false;
        QGraphicsItemCache::DeviceData *deviceData = &itemCache->deviceData[widget];
        bool invertable = true;
        QTransform diff = deviceData->lastTransform.inverted(&invertable);
        if (invertable)
            diff *= painter->worldTransform();
        deviceData->lastTransform = painter->worldTransform();
        bool allowPartialCacheExposure = false;
        bool simpleTransform = invertable && diff.type() <= QTransform::TxTranslate
                               && transformIsSimple(painter->worldTransform());
        if (!simpleTransform) {
            pixModified = true;
            itemCache->allExposed = true;
            itemCache->exposed.clear();
            deviceData->cacheIndent = QPoint();
            pix = QPixmap();
        } else if (!viewRect.isNull()) {
            allowPartialCacheExposure = deviceData->cacheIndent != QPoint();
        }

        // Allow partial cache exposure if the device rect isn't fully contained and
        // deviceRect is 20% taller or wider than the viewRect.
        if (!allowPartialCacheExposure && !viewRect.isNull() && !viewRect.contains(deviceRect)) {
            allowPartialCacheExposure = (viewRect.width() * 1.2 < deviceRect.width())
                                         || (viewRect.height() * 1.2 < deviceRect.height());
        }

        QRegion scrollExposure;
        if (allowPartialCacheExposure) {
            // Part of pixmap is drawn. Either device contains viewrect (big
            // item covers whole screen) or parts of device are outside the
            // viewport. In either case the device rect must be the intersect
            // between the two.
            int dx = deviceRect.left() < viewRect.left() ? viewRect.left() - deviceRect.left() : 0;
            int dy = deviceRect.top() < viewRect.top() ? viewRect.top() - deviceRect.top() : 0;
            QPoint newCacheIndent(dx, dy);
            deviceRect &= viewRect;

            if (pix.isNull()) {
                deviceData->cacheIndent = QPoint();
                itemCache->allExposed = true;
                itemCache->exposed.clear();
                pixModified = true;
            }

            // Copy / "scroll" the old pixmap onto the new ole and calculate
            // scrolled exposure.
            if (newCacheIndent != deviceData->cacheIndent || deviceRect.size() != pix.size()) {
                QPoint diff = newCacheIndent - deviceData->cacheIndent;
                QPixmap newPix(deviceRect.size());
                // ### Investigate removing this fill (test with Plasma and
                // graphicssystem raster).
                newPix.fill(Qt::transparent);
                if (!pix.isNull()) {
                    QPainter newPixPainter(&newPix);
                    newPixPainter.drawPixmap(-diff, pix);
                    newPixPainter.end();
                }
                QRegion exposed;
                exposed += newPix.rect();
                if (!pix.isNull())
                    exposed -= QRect(-diff, pix.size());
                scrollExposure = exposed;

                pix = newPix;
                pixModified = true;
            }
            deviceData->cacheIndent = newCacheIndent;
        } else {
            // Full pixmap is drawn.
            deviceData->cacheIndent = QPoint();

            // Auto-adjust the pixmap size.
            if (deviceRect.size() != pix.size()) {
                // exposed needs to cover the whole pixmap
                pix = QPixmap(deviceRect.size());
                pixModified = true;
                itemCache->allExposed = true;
                itemCache->exposed.clear();
            }
        }

        // Check for newly invalidated areas.
        if (itemCache->allExposed || !itemCache->exposed.isEmpty() || !scrollExposure.isEmpty()) {
            //We know that we will modify the pixmap, removing it from the cache
            //will detach the one we have and avoid a deep copy
            if (pixmapFound)
                QPixmapCache::remove(pixmapKey);

            // Construct an item-to-pixmap transform.
            QPointF p = deviceRect.topLeft();
            QTransform itemToPixmap = painter->worldTransform();
            if (!p.isNull())
                itemToPixmap *= QTransform::fromTranslate(-p.x(), -p.y());

            // Map the item's logical expose to pixmap coordinates.
            QRegion pixmapExposed = scrollExposure;
            if (!itemCache->allExposed) {
                const QVector<QRectF> &exposed = itemCache->exposed;
                for (int i = 0; i < exposed.size(); ++i)
                    pixmapExposed += itemToPixmap.mapRect(exposed.at(i)).toRect().adjusted(-1, -1, 1, 1);
            }

            // Calculate the style option's exposedRect.
            QRectF br;
            if (itemCache->allExposed) {
                br = item->boundingRect();
            } else {
                const QVector<QRectF> &exposed = itemCache->exposed;
                for (int i = 0; i < exposed.size(); ++i)
                    br |= exposed.at(i);
                QTransform pixmapToItem = itemToPixmap.inverted();
                foreach (QRect r, scrollExposure.rects())
                    br |= pixmapToItem.mapRect(r);
            }
            styleOptionTmp = *option;
            styleOptionTmp.exposedRect = br.adjusted(-1, -1, 1, 1);

            // Render the exposed areas.
            _q_paintIntoCache(&pix, item, pixmapExposed, itemToPixmap, painter->renderHints(),
                              &styleOptionTmp, painterStateProtection);

            // Reset expose data.
            pixModified = true;
            itemCache->allExposed = false;
            itemCache->exposed.clear();
        }

        if (pixModified) {
            // Insert this pixmap into the cache.
            deviceData->key = QPixmapCache::insert(pix);
        }

        // Redraw the exposed area using an untransformed painter. This
        // effectively becomes a bitblit that does not transform the cache.
        QTransform restoreTransform = painter->worldTransform();
        painter->setWorldTransform(QTransform());
        if (newPainterOpacity != oldPainterOpacity) {
            painter->setOpacity(newPainterOpacity);
            painter->drawPixmap(deviceRect.topLeft(), pix);
            painter->setOpacity(oldPainterOpacity);
        } else {
            painter->drawPixmap(deviceRect.topLeft(), pix);
        }
        painter->setWorldTransform(restoreTransform);
        return;
    }
}

void QGraphicsScenePrivate::drawItems(QPainter *painter, const QTransform *const viewTransform,
                                      QRegion *exposedRegion, QWidget *widget)
{
    // Make sure we don't have unpolished items before we draw.
    if (!unpolishedItems.isEmpty())
        _q_polishItems();

    updateAll = false;
    QRectF exposedSceneRect;
    if (exposedRegion && indexMethod != QGraphicsScene::NoIndex) {
        exposedSceneRect = exposedRegion->boundingRect().adjusted(-1, -1, 1, 1);
        if (viewTransform)
            exposedSceneRect = viewTransform->inverted().mapRect(exposedSceneRect);
    }
    const QList<QGraphicsItem *> tli = index->estimateTopLevelItems(exposedSceneRect, Qt::AscendingOrder);
    for (int i = 0; i < tli.size(); ++i)
        drawSubtreeRecursive(tli.at(i), painter, viewTransform, exposedRegion, widget);
}

void QGraphicsScenePrivate::drawSubtreeRecursive(QGraphicsItem *item, QPainter *painter,
                                                 const QTransform *const viewTransform,
                                                 QRegion *exposedRegion, QWidget *widget,
                                                 qreal parentOpacity, const QTransform *const effectTransform)
{
    Q_ASSERT(item);

    if (!item->d_ptr->visible)
        return;

    const bool itemHasContents = !(item->d_ptr->flags & QGraphicsItem::ItemHasNoContents);
    const bool itemHasChildren = !item->d_ptr->children.isEmpty();
    if (!itemHasContents && !itemHasChildren)
        return; // Item has neither contents nor children!(?)

    const qreal opacity = item->d_ptr->combineOpacityFromParent(parentOpacity);
    const bool itemIsFullyTransparent = QGraphicsItemPrivate::isOpacityNull(opacity);
    if (itemIsFullyTransparent && (!itemHasChildren || item->d_ptr->childrenCombineOpacity()))
        return;

    QTransform transform(Qt::Uninitialized);
    QTransform *transformPtr = 0;
    bool translateOnlyTransform = false;
#define ENSURE_TRANSFORM_PTR \
    if (!transformPtr) { \
        Q_ASSERT(!itemIsUntransformable); \
        if (viewTransform) { \
            transform = item->d_ptr->sceneTransform; \
            transform *= *viewTransform; \
            transformPtr = &transform; \
        } else { \
            transformPtr = &item->d_ptr->sceneTransform; \
            translateOnlyTransform = item->d_ptr->sceneTransformTranslateOnly; \
        } \
    }

    // Update the item's scene transform if the item is transformable;
    // otherwise calculate the full transform,
    bool wasDirtyParentSceneTransform = false;
    const bool itemIsUntransformable = item->d_ptr->itemIsUntransformable();
    if (itemIsUntransformable) {
        transform = item->deviceTransform(viewTransform ? *viewTransform : QTransform());
        transformPtr = &transform;
    } else if (item->d_ptr->dirtySceneTransform) {
        item->d_ptr->updateSceneTransformFromParent();
        Q_ASSERT(!item->d_ptr->dirtySceneTransform);
        wasDirtyParentSceneTransform = true;
    }

    const bool itemClipsChildrenToShape = (item->d_ptr->flags & QGraphicsItem::ItemClipsChildrenToShape);
    bool drawItem = itemHasContents && !itemIsFullyTransparent;
    if (drawItem) {
        const QRectF brect = adjustedItemEffectiveBoundingRect(item);
        ENSURE_TRANSFORM_PTR
        QRect viewBoundingRect = translateOnlyTransform ? brect.translated(transformPtr->dx(), transformPtr->dy()).toAlignedRect()
                                                        : transformPtr->mapRect(brect).toAlignedRect();
        viewBoundingRect.adjust(-int(rectAdjust), -int(rectAdjust), rectAdjust, rectAdjust);
        if (widget)
            item->d_ptr->paintedViewBoundingRects.insert(widget, viewBoundingRect);
        drawItem = exposedRegion ? exposedRegion->intersects(viewBoundingRect)
                                 : !viewBoundingRect.normalized().isEmpty();
        if (!drawItem) {
            if (!itemHasChildren)
                return;
            if (itemClipsChildrenToShape) {
                if (wasDirtyParentSceneTransform)
                    item->d_ptr->invalidateChildrenSceneTransform();
                return;
            }
        }
    } // else we know for sure this item has children we must process.

    if (itemHasChildren && itemClipsChildrenToShape)
        ENSURE_TRANSFORM_PTR;

#ifndef QT_NO_GRAPHICSEFFECT
    if (item->d_ptr->graphicsEffect && item->d_ptr->graphicsEffect->isEnabled()) {
        ENSURE_TRANSFORM_PTR;
        QGraphicsItemPaintInfo info(viewTransform, transformPtr, effectTransform, exposedRegion, widget, &styleOptionTmp,
                                    painter, opacity, wasDirtyParentSceneTransform, itemHasContents && !itemIsFullyTransparent);
        QGraphicsEffectSource *source = item->d_ptr->graphicsEffect->d_func()->source;
        QGraphicsItemEffectSourcePrivate *sourced = static_cast<QGraphicsItemEffectSourcePrivate *>
                                                    (source->d_func());
        sourced->info = &info;
        const QTransform restoreTransform = painter->worldTransform();
        if (effectTransform)
            painter->setWorldTransform(*transformPtr * *effectTransform);
        else
            painter->setWorldTransform(*transformPtr);
        painter->setOpacity(opacity);

        if (sourced->currentCachedSystem() != Qt::LogicalCoordinates
            && sourced->lastEffectTransform != painter->worldTransform())
        {
            if (sourced->lastEffectTransform.type() <= QTransform::TxTranslate
                && painter->worldTransform().type() <= QTransform::TxTranslate)
            {
                QRectF sourceRect = sourced->boundingRect(Qt::DeviceCoordinates);
                QRect effectRect = sourced->paddedEffectRect(Qt::DeviceCoordinates, sourced->currentCachedMode(), sourceRect);

                sourced->setCachedOffset(effectRect.topLeft());
            } else {
                sourced->invalidateCache(QGraphicsEffectSourcePrivate::TransformChanged);
            }

            sourced->lastEffectTransform = painter->worldTransform();
        }

        item->d_ptr->graphicsEffect->draw(painter);
        painter->setWorldTransform(restoreTransform);
        sourced->info = 0;
    } else
#endif //QT_NO_GRAPHICSEFFECT
    {
        draw(item, painter, viewTransform, transformPtr, exposedRegion, widget, opacity,
             effectTransform, wasDirtyParentSceneTransform, drawItem);
    }
}

static inline void setClip(QPainter *painter, QGraphicsItem *item)
{
    painter->save();
    QRectF clipRect;
    const QPainterPath clipPath(item->shape());
    if (QPathClipper::pathToRect(clipPath, &clipRect))
        painter->setClipRect(clipRect, Qt::IntersectClip);
    else
        painter->setClipPath(clipPath, Qt::IntersectClip);
}

static inline void setWorldTransform(QPainter *painter, const QTransform *const transformPtr,
                                     const QTransform *effectTransform)
{
    Q_ASSERT(transformPtr);
    if (effectTransform)
        painter->setWorldTransform(*transformPtr * *effectTransform);
    else
        painter->setWorldTransform(*transformPtr);
}

void QGraphicsScenePrivate::draw(QGraphicsItem *item, QPainter *painter, const QTransform *const viewTransform,
                                 const QTransform *const transformPtr, QRegion *exposedRegion, QWidget *widget,
                                 qreal opacity, const QTransform *effectTransform,
                                 bool wasDirtyParentSceneTransform, bool drawItem)
{
    const bool itemIsFullyTransparent = QGraphicsItemPrivate::isOpacityNull(opacity);
    const bool itemClipsChildrenToShape = (item->d_ptr->flags & QGraphicsItem::ItemClipsChildrenToShape);
    const bool itemHasChildren = !item->d_ptr->children.isEmpty();
    bool setChildClip = itemClipsChildrenToShape;
    bool itemHasChildrenStackedBehind = false;

    int i = 0;
    if (itemHasChildren) {
        if (itemClipsChildrenToShape)
            setWorldTransform(painter, transformPtr, effectTransform);

        item->d_ptr->ensureSortedChildren();
        // Items with the 'ItemStacksBehindParent' flag are put in front of the list
        // so all we have to do is to check the first item.
        itemHasChildrenStackedBehind = (item->d_ptr->children.at(0)->d_ptr->flags
                                        & QGraphicsItem::ItemStacksBehindParent);

        if (itemHasChildrenStackedBehind) {
            if (itemClipsChildrenToShape) {
                setClip(painter, item);
                setChildClip = false;
            }

            // Draw children behind
            for (i = 0; i < item->d_ptr->children.size(); ++i) {
                QGraphicsItem *child = item->d_ptr->children.at(i);
                if (wasDirtyParentSceneTransform)
                    child->d_ptr->dirtySceneTransform = 1;
                if (!(child->d_ptr->flags & QGraphicsItem::ItemStacksBehindParent))
                    break;
                if (itemIsFullyTransparent && !(child->d_ptr->flags & QGraphicsItem::ItemIgnoresParentOpacity))
                    continue;
                drawSubtreeRecursive(child, painter, viewTransform, exposedRegion, widget, opacity, effectTransform);
            }
        }
    }

    // Draw item
    if (drawItem) {
        Q_ASSERT(!itemIsFullyTransparent);
        Q_ASSERT(!(item->d_ptr->flags & QGraphicsItem::ItemHasNoContents));
        Q_ASSERT(transformPtr);
        item->d_ptr->initStyleOption(&styleOptionTmp, *transformPtr, exposedRegion
                                     ? *exposedRegion : QRegion(), exposedRegion == 0);

        const bool itemClipsToShape = item->d_ptr->flags & QGraphicsItem::ItemClipsToShape;
        bool restorePainterClip = false;

        if (!itemHasChildren || !itemClipsChildrenToShape) {
            // Item does not have children or clip children to shape.
            setWorldTransform(painter, transformPtr, effectTransform);
            if ((restorePainterClip = itemClipsToShape))
                setClip(painter, item);
        } else if (itemHasChildrenStackedBehind){
            // Item clips children to shape and has children stacked behind, which means
            // the painter is already clipped to the item's shape.
            if (itemClipsToShape) {
                // The clip is already correct. Ensure correct world transform.
                setWorldTransform(painter, transformPtr, effectTransform);
            } else {
                // Remove clip (this also ensures correct world transform).
                painter->restore();
                setChildClip = true;
            }
        } else if (itemClipsToShape) {
            // Item clips children and itself to shape. It does not have hildren stacked
            // behind, which means the clip has not yet been set. We set it now and re-use it
            // for the children.
            setClip(painter, item);
            setChildClip = false;
        }

        if (painterStateProtection && !restorePainterClip)
            painter->save();

        painter->setOpacity(opacity);
        if (!item->d_ptr->cacheMode && !item->d_ptr->isWidget)
            item->paint(painter, &styleOptionTmp, widget);
        else
            drawItemHelper(item, painter, &styleOptionTmp, widget, painterStateProtection);

        if (painterStateProtection || restorePainterClip)
            painter->restore();

        static int drawRect = qgetenv("QT_DRAW_SCENE_ITEM_RECTS").toInt();
        if (drawRect) {
            QPen oldPen = painter->pen();
            QBrush oldBrush = painter->brush();
            quintptr ptr = reinterpret_cast<quintptr>(item);
            const QColor color = QColor::fromHsv(ptr % 255, 255, 255);
            painter->setPen(color);
            painter->setBrush(Qt::NoBrush);
            painter->drawRect(adjustedItemBoundingRect(item));
            painter->setPen(oldPen);
            painter->setBrush(oldBrush);
        }
    }

    // Draw children in front
    if (itemHasChildren) {
        if (setChildClip)
            setClip(painter, item);

        for (; i < item->d_ptr->children.size(); ++i) {
            QGraphicsItem *child = item->d_ptr->children.at(i);
            if (wasDirtyParentSceneTransform)
                child->d_ptr->dirtySceneTransform = 1;
            if (itemIsFullyTransparent && !(child->d_ptr->flags & QGraphicsItem::ItemIgnoresParentOpacity))
                continue;
            drawSubtreeRecursive(child, painter, viewTransform, exposedRegion, widget, opacity, effectTransform);
        }

        // Restore child clip
        if (itemClipsChildrenToShape)
            painter->restore();
    }
}

void QGraphicsScenePrivate::markDirty(QGraphicsItem *item, const QRectF &rect, bool invalidateChildren,
                                      bool force, bool ignoreOpacity, bool removingItemFromScene,
                                      bool updateBoundingRect)
{
    Q_ASSERT(item);
    if (updateAll)
        return;

    if (removingItemFromScene && !ignoreOpacity && !item->d_ptr->ignoreOpacity) {
        // If any of the item's ancestors ignore opacity, it means that the opacity
        // was set to 0 (and the update request has not yet been processed). That
        // also means that we have to ignore the opacity for the item itself; otherwise
        // things like: parent->setOpacity(0); scene->removeItem(child) won't work.
        // Note that we only do this when removing items from the scene. In all other
        // cases the ignoreOpacity bit propagates properly in processDirtyItems, but
        // since the item is removed immediately it won't be processed there.
        QGraphicsItem *p = item->d_ptr->parent;
        while (p) {
            if (p->d_ptr->ignoreOpacity) {
                item->d_ptr->ignoreOpacity = true;
                break;
            }
            p = p->d_ptr->parent;
        }
    }

    if (item->d_ptr->discardUpdateRequest(/*ignoreVisibleBit=*/force,
                                          /*ignoreDirtyBit=*/removingItemFromScene || invalidateChildren,
                                          /*ignoreOpacity=*/ignoreOpacity)) {
        if (item->d_ptr->dirty) {
            // The item is already marked as dirty and will be processed later. However,
            // we have to make sure ignoreVisible and ignoreOpacity are set properly;
            // otherwise things like: item->update(); item->hide() (force is now true)
            // won't work as expected.
            if (force)
                item->d_ptr->ignoreVisible = 1;
            if (ignoreOpacity)
                item->d_ptr->ignoreOpacity = 1;
        }
        return;
    }

    const bool fullItemUpdate = rect.isNull();
    if (!fullItemUpdate && rect.isEmpty())
        return;

    if (!processDirtyItemsEmitted) {
        QMetaMethod method = q_ptr->metaObject()->method(processDirtyItemsIndex);
        method.invoke(q_ptr, Qt::QueuedConnection);
//        QMetaObject::invokeMethod(q_ptr, "_q_processDirtyItems", Qt::QueuedConnection);
        processDirtyItemsEmitted = true;
    }

    if (removingItemFromScene) {
        // Note that this function can be called from the item's destructor, so
        // do NOT call any virtual functions on it within this block.
        if (isSignalConnected(changedSignalIndex) || views.isEmpty()) {
            // This block of code is kept for compatibility. Since 4.5, by default
            // QGraphicsView does not connect the signal and we use the below
            // method of delivering updates.
            q_func()->update();
            return;
        }

        for (int i = 0; i < views.size(); ++i) {
            QGraphicsViewPrivate *viewPrivate = views.at(i)->d_func();
            QRect rect = item->d_ptr->paintedViewBoundingRects.value(viewPrivate->viewport);
            rect.translate(viewPrivate->dirtyScrollOffset);
            viewPrivate->updateRect(rect);
        }
        return;
    }

    bool hasNoContents = item->d_ptr->flags & QGraphicsItem::ItemHasNoContents;
    if (!hasNoContents) {
        item->d_ptr->dirty = 1;
        if (fullItemUpdate)
            item->d_ptr->fullUpdatePending = 1;
        else if (!item->d_ptr->fullUpdatePending)
            item->d_ptr->needsRepaint |= rect;
    } else if (item->d_ptr->graphicsEffect) {
        invalidateChildren = true;
    }

    if (invalidateChildren) {
        item->d_ptr->allChildrenDirty = 1;
        item->d_ptr->dirtyChildren = 1;
    }

    if (force)
        item->d_ptr->ignoreVisible = 1;
    if (ignoreOpacity)
        item->d_ptr->ignoreOpacity = 1;

    if (!updateBoundingRect)
        item->d_ptr->markParentDirty();
}

static inline bool updateHelper(QGraphicsViewPrivate *view, QGraphicsItemPrivate *item,
                                const QRectF &rect, bool itemIsUntransformable)
{
    Q_ASSERT(view);
    Q_ASSERT(item);

    QGraphicsItem *itemq = static_cast<QGraphicsItem *>(item->q_ptr);
    QGraphicsView *viewq = static_cast<QGraphicsView *>(view->q_ptr);

    if (itemIsUntransformable) {
        const QTransform xform = itemq->deviceTransform(viewq->viewportTransform());
        if (!item->hasBoundingRegionGranularity)
            return view->updateRectF(xform.mapRect(rect));
        return view->updateRegion(rect, xform);
    }

    if (item->sceneTransformTranslateOnly && view->identityMatrix) {
        const qreal dx = item->sceneTransform.dx();
        const qreal dy = item->sceneTransform.dy();
        QRectF r(rect);
        r.translate(dx - view->horizontalScroll(), dy - view->verticalScroll());
        return view->updateRectF(r);
    }

    if (!viewq->isTransformed()) {
        if (!item->hasBoundingRegionGranularity)
            return view->updateRectF(item->sceneTransform.mapRect(rect));
        return view->updateRegion(rect, item->sceneTransform);
    }

    QTransform xform = item->sceneTransform;
    xform *= viewq->viewportTransform();
    if (!item->hasBoundingRegionGranularity)
        return view->updateRectF(xform.mapRect(rect));
    return view->updateRegion(rect, xform);
}

void QGraphicsScenePrivate::processDirtyItemsRecursive(QGraphicsItem *item, bool dirtyAncestorContainsChildren,
                                                       qreal parentOpacity)
{
    Q_Q(QGraphicsScene);
    Q_ASSERT(item);
    Q_ASSERT(!updateAll);

    if (!item->d_ptr->dirty && !item->d_ptr->dirtyChildren) {
        resetDirtyItem(item);
        return;
    }

    const bool itemIsHidden = !item->d_ptr->ignoreVisible && !item->d_ptr->visible;
    if (itemIsHidden) {
        resetDirtyItem(item, /*recursive=*/true);
        return;
    }

    bool itemHasContents = !(item->d_ptr->flags & QGraphicsItem::ItemHasNoContents);
    const bool itemHasChildren = !item->d_ptr->children.isEmpty();
    if (!itemHasContents) {
        if (!itemHasChildren) {
            resetDirtyItem(item);
            return; // Item has neither contents nor children!(?)
        }
        if (item->d_ptr->graphicsEffect)
            itemHasContents = true;
    }

    const qreal opacity = item->d_ptr->combineOpacityFromParent(parentOpacity);
    const bool itemIsFullyTransparent = !item->d_ptr->ignoreOpacity
                                        && QGraphicsItemPrivate::isOpacityNull(opacity);
    if (itemIsFullyTransparent && (!itemHasChildren || item->d_ptr->childrenCombineOpacity())) {
        resetDirtyItem(item, /*recursive=*/itemHasChildren);
        return;
    }

    bool wasDirtyParentSceneTransform = item->d_ptr->dirtySceneTransform;
    const bool itemIsUntransformable = item->d_ptr->itemIsUntransformable();
    if (wasDirtyParentSceneTransform && !itemIsUntransformable) {
        item->d_ptr->updateSceneTransformFromParent();
        Q_ASSERT(!item->d_ptr->dirtySceneTransform);
    }

    const bool wasDirtyParentViewBoundingRects = item->d_ptr->paintedViewBoundingRectsNeedRepaint;
    if (itemIsFullyTransparent || !itemHasContents || dirtyAncestorContainsChildren) {
        // Make sure we don't process invisible items or items with no content.
        item->d_ptr->dirty = 0;
        item->d_ptr->fullUpdatePending = 0;
        // Might have a dirty view bounding rect otherwise.
        if (itemIsFullyTransparent || !itemHasContents)
            item->d_ptr->paintedViewBoundingRectsNeedRepaint = 0;
    }

    if (!hasSceneRect && item->d_ptr->geometryChanged && item->d_ptr->visible) {
        // Update growingItemsBoundingRect.
        if (item->d_ptr->sceneTransformTranslateOnly) {
            growingItemsBoundingRect |= item->boundingRect().translated(item->d_ptr->sceneTransform.dx(),
                                                                        item->d_ptr->sceneTransform.dy());
        } else {
            growingItemsBoundingRect |= item->d_ptr->sceneTransform.mapRect(item->boundingRect());
        }
    }

    // Process item.
    if (item->d_ptr->dirty || item->d_ptr->paintedViewBoundingRectsNeedRepaint) {
        const bool useCompatUpdate = views.isEmpty() || isSignalConnected(changedSignalIndex);
        const QRectF itemBoundingRect = adjustedItemEffectiveBoundingRect(item);

        if (useCompatUpdate && !itemIsUntransformable && qFuzzyIsNull(item->boundingRegionGranularity())) {
            // This block of code is kept for compatibility. Since 4.5, by default
            // QGraphicsView does not connect the signal and we use the below
            // method of delivering updates.
            if (item->d_ptr->sceneTransformTranslateOnly) {
                q->update(itemBoundingRect.translated(item->d_ptr->sceneTransform.dx(),
                                                      item->d_ptr->sceneTransform.dy()));
            } else {
                QRectF rect = item->d_ptr->sceneTransform.mapRect(itemBoundingRect);
                if (!rect.isEmpty())
                    q->update(rect);
            }
        } else {
            QRectF dirtyRect;
            bool uninitializedDirtyRect = true;

            for (int j = 0; j < views.size(); ++j) {
                QGraphicsView *view = views.at(j);
                QGraphicsViewPrivate *viewPrivate = view->d_func();
                QRect &paintedViewBoundingRect = item->d_ptr->paintedViewBoundingRects[viewPrivate->viewport];
                if (viewPrivate->fullUpdatePending
                    || viewPrivate->viewportUpdateMode == QGraphicsView::NoViewportUpdate) {
                    // Okay, if we have a full update pending or no viewport update, this item's
                    // paintedViewBoundingRect  will be updated correctly in the next paintEvent if
                    // it is inside the viewport, but for now we can pretend that it is outside.
                    paintedViewBoundingRect = QRect(-1, -1, -1, -1);
                    continue;
                }

                if (item->d_ptr->paintedViewBoundingRectsNeedRepaint) {
                    paintedViewBoundingRect.translate(viewPrivate->dirtyScrollOffset);
                    if (!viewPrivate->updateRect(paintedViewBoundingRect))
                        paintedViewBoundingRect = QRect(-1, -1, -1, -1); // Outside viewport.
                }

                if (!item->d_ptr->dirty)
                    continue;

                if (!item->d_ptr->paintedViewBoundingRectsNeedRepaint
                    && paintedViewBoundingRect.x() == -1 && paintedViewBoundingRect.y() == -1
                    && paintedViewBoundingRect.width() == -1 && paintedViewBoundingRect.height() == -1) {
                    continue; // Outside viewport.
                }

                if (uninitializedDirtyRect) {
                    dirtyRect = itemBoundingRect;
                    if (!item->d_ptr->fullUpdatePending) {
                        _q_adjustRect(&item->d_ptr->needsRepaint);
                        dirtyRect &= item->d_ptr->needsRepaint;
                    }
                    uninitializedDirtyRect = false;
                }

                if (dirtyRect.isEmpty())
                    continue; // Discard updates outside the bounding rect.

                if (!updateHelper(viewPrivate, item->d_ptr.data(), dirtyRect, itemIsUntransformable)
                    && item->d_ptr->paintedViewBoundingRectsNeedRepaint) {
                    paintedViewBoundingRect = QRect(-1, -1, -1, -1); // Outside viewport.
                }
            }
        }
    }

    // Process children.
    if (itemHasChildren && item->d_ptr->dirtyChildren) {
        const bool itemClipsChildrenToShape = item->d_ptr->flags & QGraphicsItem::ItemClipsChildrenToShape;
        // Items with no content are threated as 'dummy' items which means they are never drawn and
        // 'processed', so the painted view bounding rect is never up-to-date. This means that whenever
        // such an item changes geometry, its children have to take care of the update regardless
        // of whether the item clips children to shape or not.
        const bool bypassUpdateClip = !itemHasContents && wasDirtyParentViewBoundingRects;
        if (itemClipsChildrenToShape && !bypassUpdateClip) {
            // Make sure child updates are clipped to the item's bounding rect.
            for (int i = 0; i < views.size(); ++i)
                views.at(i)->d_func()->setUpdateClip(item);
        }
        if (!dirtyAncestorContainsChildren) {
            dirtyAncestorContainsChildren = item->d_ptr->fullUpdatePending
                                            && itemClipsChildrenToShape;
        }
        const bool allChildrenDirty = item->d_ptr->allChildrenDirty;
        const bool parentIgnoresVisible = item->d_ptr->ignoreVisible;
        const bool parentIgnoresOpacity = item->d_ptr->ignoreOpacity;
        for (int i = 0; i < item->d_ptr->children.size(); ++i) {
            QGraphicsItem *child = item->d_ptr->children.at(i);
            if (wasDirtyParentSceneTransform)
                child->d_ptr->dirtySceneTransform = 1;
            if (wasDirtyParentViewBoundingRects)
                child->d_ptr->paintedViewBoundingRectsNeedRepaint = 1;
            if (parentIgnoresVisible)
                child->d_ptr->ignoreVisible = 1;
            if (parentIgnoresOpacity)
                child->d_ptr->ignoreOpacity = 1;
            if (allChildrenDirty) {
                child->d_ptr->dirty = 1;
                child->d_ptr->fullUpdatePending = 1;
                child->d_ptr->dirtyChildren = 1;
                child->d_ptr->allChildrenDirty = 1;
            }
            processDirtyItemsRecursive(child, dirtyAncestorContainsChildren, opacity);
        }

        if (itemClipsChildrenToShape) {
            // Reset updateClip.
            for (int i = 0; i < views.size(); ++i)
                views.at(i)->d_func()->setUpdateClip(0);
        }
    } else if (wasDirtyParentSceneTransform) {
        item->d_ptr->invalidateChildrenSceneTransform();
    }

    resetDirtyItem(item);
}

/*!
    \obsolete

    Paints the given \a items using the provided \a painter, after the
    background has been drawn, and before the foreground has been
    drawn.  All painting is done in \e scene coordinates. Before
    drawing each item, the painter must be transformed using
    QGraphicsItem::sceneTransform().

    The \a options parameter is the list of style option objects for
    each item in \a items. The \a numItems parameter is the number of
    items in \a items and options in \a options. The \a widget
    parameter is optional; if specified, it should point to the widget
    that is being painted on.

    The default implementation prepares the painter matrix, and calls
    QGraphicsItem::paint() on all items. Reimplement this function to
    provide custom painting of all items for the scene; gaining
    complete control over how each item is drawn. In some cases this
    can increase drawing performance significantly.

    Example:

    \snippet doc/src/snippets/graphicssceneadditemsnippet.cpp 0

    Since Qt 4.6, this function is not called anymore unless
    the QGraphicsView::IndirectPainting flag is given as an Optimization
    flag.

    \sa drawBackground(), drawForeground()
*/
void QGraphicsScene::drawItems(QPainter *painter,
                               int numItems,
                               QGraphicsItem *items[],
                               const QStyleOptionGraphicsItem options[], QWidget *widget)
{
    Q_D(QGraphicsScene);
    // Make sure we don't have unpolished items before we draw.
    if (!d->unpolishedItems.isEmpty())
        d->_q_polishItems();

    const qreal opacity = painter->opacity();
    QTransform viewTransform = painter->worldTransform();
    Q_UNUSED(options);

    // Determine view, expose and flags.
    QGraphicsView *view = widget ? qobject_cast<QGraphicsView *>(widget->parentWidget()) : 0;
    QRegion *expose = 0;
    const quint32 oldRectAdjust = d->rectAdjust;
    if (view) {
        d->updateAll = false;
        expose = &view->d_func()->exposedRegion;
        if (view->d_func()->optimizationFlags & QGraphicsView::DontAdjustForAntialiasing)
            d->rectAdjust = 1;
        else
            d->rectAdjust = 2;
    }

    // Find all toplevels, they are already sorted.
    QList<QGraphicsItem *> topLevelItems;
    for (int i = 0; i < numItems; ++i) {
        QGraphicsItem *item = items[i]->topLevelItem();
        if (!item->d_ptr->itemDiscovered) {
            topLevelItems << item;
            item->d_ptr->itemDiscovered = 1;
            d->drawSubtreeRecursive(item, painter, &viewTransform, expose, widget);
        }
    }

    d->rectAdjust = oldRectAdjust;
    // Reset discovery bits.
    for (int i = 0; i < topLevelItems.size(); ++i)
        topLevelItems.at(i)->d_ptr->itemDiscovered = 0;

    painter->setWorldTransform(viewTransform);
    painter->setOpacity(opacity);
}

/*!
    \since 4.4

    Finds a new widget to give the keyboard focus to, as appropriate for Tab
    and Shift+Tab, and returns true if it can find a new widget, or false if
    it cannot. If \a next is true, this function searches forward; if \a next
    is false, it searches backward.

    You can reimplement this function in a subclass of QGraphicsScene to
    provide fine-grained control over how tab focus passes inside your
    scene. The default implementation is based on the tab focus chain defined
    by QGraphicsWidget::setTabOrder().
*/
bool QGraphicsScene::focusNextPrevChild(bool next)
{
    Q_D(QGraphicsScene);

    QGraphicsItem *item = focusItem();
    if (item && !item->isWidget()) {
        // Tab out of the scene.
        return false;
    }
    if (!item) {
        if (d->lastFocusItem && !d->lastFocusItem->isWidget()) {
            // Restore focus to the last focusable non-widget item that had
            // focus.
            setFocusItem(d->lastFocusItem, next ? Qt::TabFocusReason : Qt::BacktabFocusReason);
            return true;
        }
    }
    if (!d->tabFocusFirst) {
        // No widgets...
        return false;
    }

    // The item must be a widget.
    QGraphicsWidget *widget = 0;
    if (!item) {
        widget = next ? d->tabFocusFirst : d->tabFocusFirst->d_func()->focusPrev;
    } else {
        QGraphicsWidget *test = static_cast<QGraphicsWidget *>(item);
        widget = next ? test->d_func()->focusNext : test->d_func()->focusPrev;
        if ((next && widget == d->tabFocusFirst) || (!next && widget == d->tabFocusFirst->d_func()->focusPrev))
            return false;
    }
    QGraphicsWidget *widgetThatHadFocus = widget;

    // Run around the focus chain until we find a widget that can take tab focus.
    do {
        if (widget->flags() & QGraphicsItem::ItemIsFocusable
            && widget->isEnabled() && widget->isVisibleTo(0)
            && (widget->focusPolicy() & Qt::TabFocus)
            && (!item || !item->isPanel() || item->isAncestorOf(widget))
            ) {
            setFocusItem(widget, next ? Qt::TabFocusReason : Qt::BacktabFocusReason);
            return true;
        }
        widget = next ? widget->d_func()->focusNext : widget->d_func()->focusPrev;
        if ((next && widget == d->tabFocusFirst) || (!next && widget == d->tabFocusFirst->d_func()->focusPrev))
            return false;
    } while (widget != widgetThatHadFocus);

    return false;
}

/*!
    \fn QGraphicsScene::changed(const QList<QRectF> &region)

    This signal is emitted by QGraphicsScene when control reaches the
    event loop, if the scene content changes. The \a region parameter
    contains a list of scene rectangles that indicate the area that
    has been changed.

    \sa QGraphicsView::updateScene()
*/

/*!
    \fn QGraphicsScene::sceneRectChanged(const QRectF &rect)

    This signal is emitted by QGraphicsScene whenever the scene rect changes.
    The \a rect parameter is the new scene rectangle.

    \sa QGraphicsView::updateSceneRect()
*/

/*!
    \fn QGraphicsScene::selectionChanged()
    \since 4.3

    This signal is emitted by QGraphicsScene whenever the selection
    changes. You can call selectedItems() to get the new list of selected
    items.

    The selection changes whenever an item is selected or unselected, a
    selection area is set, cleared or otherwise changed, if a preselected item
    is added to the scene, or if a selected item is removed from the scene.

    QGraphicsScene emits this signal only once for group selection operations.
    For example, if you set a selection area, select or unselect a
    QGraphicsItemGroup, or if you add or remove from the scene a parent item
    that contains several selected items, selectionChanged() is emitted only
    once after the operation has completed (instead of once for each item).

    \sa setSelectionArea(), selectedItems(), QGraphicsItem::setSelected()
*/

/*!
    \since 4.4

    Returns the scene's style, or the same as QApplication::style() if the
    scene has not been explicitly assigned a style.

    \sa setStyle()
*/
QStyle *QGraphicsScene::style() const
{
    Q_D(const QGraphicsScene);
    // ### This function, and the use of styles in general, is non-reentrant.
    return d->style ? d->style : QApplication::style();
}

/*!
    \since 4.4

    Sets or replaces the style of the scene to \a style, and reparents the
    style to this scene. Any previously assigned style is deleted. The scene's
    style defaults to QApplication::style(), and serves as the default for all
    QGraphicsWidget items in the scene.

    Changing the style, either directly by calling this function, or
    indirectly by calling QApplication::setStyle(), will automatically update
    the style for all widgets in the scene that do not have a style explicitly
    assigned to them.

    If \a style is 0, QGraphicsScene will revert to QApplication::style().

    \sa style()
*/
void QGraphicsScene::setStyle(QStyle *style)
{
    Q_D(QGraphicsScene);
    // ### This function, and the use of styles in general, is non-reentrant.
    if (style == d->style)
        return;

    // Delete the old style,
    delete d->style;
    if ((d->style = style))
        d->style->setParent(this);

    // Notify the scene.
    QEvent event(QEvent::StyleChange);
    QApplication::sendEvent(this, &event);

    // Notify all widgets that don't have a style explicitly set.
    foreach (QGraphicsItem *item, items()) {
        if (item->isWidget()) {
            QGraphicsWidget *widget = static_cast<QGraphicsWidget *>(item);
            if (!widget->testAttribute(Qt::WA_SetStyle))
                QApplication::sendEvent(widget, &event);
        }
    }
}

/*!
    \property QGraphicsScene::font
    \since 4.4
    \brief the scene's default font

    This property provides the scene's font. The scene font defaults to,
    and resolves all its entries from, QApplication::font.

    If the scene's font changes, either directly through setFont() or
    indirectly when the application font changes, QGraphicsScene first
    sends itself a \l{QEvent::FontChange}{FontChange} event, and it then
    sends \l{QEvent::FontChange}{FontChange} events to all top-level
    widget items in the scene. These items respond by resolving their own
    fonts to the scene, and they then notify their children, who again
    notify their children, and so on, until all widget items have updated
    their fonts.

    Changing the scene font, (directly or indirectly through
    QApplication::setFont(),) automatically schedules a redraw the entire
    scene.

    \sa QWidget::font, QApplication::setFont(), palette, style()
*/
QFont QGraphicsScene::font() const
{
    Q_D(const QGraphicsScene);
    return d->font;
}
void QGraphicsScene::setFont(const QFont &font)
{
    Q_D(QGraphicsScene);
    QFont naturalFont = QApplication::font();
    naturalFont.resolve(0);
    QFont resolvedFont = font.resolve(naturalFont);
    d->setFont_helper(resolvedFont);
}

/*!
    \property QGraphicsScene::palette
    \since 4.4
    \brief the scene's default palette

    This property provides the scene's palette. The scene palette defaults to,
    and resolves all its entries from, QApplication::palette.

    If the scene's palette changes, either directly through setPalette() or
    indirectly when the application palette changes, QGraphicsScene first
    sends itself a \l{QEvent::PaletteChange}{PaletteChange} event, and it then
    sends \l{QEvent::PaletteChange}{PaletteChange} events to all top-level
    widget items in the scene. These items respond by resolving their own
    palettes to the scene, and they then notify their children, who again
    notify their children, and so on, until all widget items have updated
    their palettes.

    Changing the scene palette, (directly or indirectly through
    QApplication::setPalette(),) automatically schedules a redraw the entire
    scene.

    \sa QWidget::palette, QApplication::setPalette(), font, style()
*/
QPalette QGraphicsScene::palette() const
{
    Q_D(const QGraphicsScene);
    return d->palette;
}
void QGraphicsScene::setPalette(const QPalette &palette)
{
    Q_D(QGraphicsScene);
    QPalette naturalPalette = QApplication::palette();
    naturalPalette.resolve(0);
    QPalette resolvedPalette = palette.resolve(naturalPalette);
    d->setPalette_helper(resolvedPalette);
}

/*!
    \since 4.6

    Returns true if the scene is active (e.g., it's viewed by
    at least one QGraphicsView that is active); otherwise returns false.

    \sa QGraphicsItem::isActive(), QWidget::isActiveWindow()
*/
bool QGraphicsScene::isActive() const
{
    Q_D(const QGraphicsScene);
    return d->activationRefCount > 0;
}

/*!
    \since 4.6
    Returns the current active panel, or 0 if no panel is currently active.

    \sa QGraphicsScene::setActivePanel()
*/
QGraphicsItem *QGraphicsScene::activePanel() const
{
    Q_D(const QGraphicsScene);
    return d->activePanel;
}

/*!
    \since 4.6
    Activates \a item, which must be an item in this scene. You
    can also pass 0 for \a item, in which case QGraphicsScene will
    deactivate any currently active panel.

    If the scene is currently inactive, \a item remains inactive until the
    scene becomes active (or, ir \a item is 0, no item will be activated).

    \sa activePanel(), isActive(), QGraphicsItem::isActive()
*/
void QGraphicsScene::setActivePanel(QGraphicsItem *item)
{
    Q_D(QGraphicsScene);
    d->setActivePanelHelper(item, false);
}

/*!
    \since 4.4

    Returns the current active window, or 0 if no window is currently
    active.

    \sa QGraphicsScene::setActiveWindow()
*/
QGraphicsWidget *QGraphicsScene::activeWindow() const
{
    Q_D(const QGraphicsScene);
    if (d->activePanel && d->activePanel->isWindow())
        return static_cast<QGraphicsWidget *>(d->activePanel);
    return 0;
}

/*!
    \since 4.4
    Activates \a widget, which must be a widget in this scene. You can also
    pass 0 for \a widget, in which case QGraphicsScene will deactivate any
    currently active window.

    \sa activeWindow(), QGraphicsWidget::isActiveWindow()
*/
void QGraphicsScene::setActiveWindow(QGraphicsWidget *widget)
{
    if (widget && widget->scene() != this) {
        qWarning("QGraphicsScene::setActiveWindow: widget %p must be part of this scene",
                 widget);
        return;
    }

    // Activate the widget's panel (all windows are panels).
    QGraphicsItem *panel = widget ? widget->panel() : 0;
    setActivePanel(panel);

    // Raise
    if (panel) {
        QList<QGraphicsItem *> siblingWindows;
        QGraphicsItem *parent = panel->parentItem();
        // Raise ### inefficient for toplevels
        foreach (QGraphicsItem *sibling, parent ? parent->children() : items()) {
            if (sibling != panel && sibling->isWindow())
                siblingWindows << sibling;
        }

        // Find the highest z value.
        qreal z = panel->zValue();
        for (int i = 0; i < siblingWindows.size(); ++i)
            z = qMax(z, siblingWindows.at(i)->zValue());

        // This will probably never overflow.
        const qreal litt = qreal(0.001);
        panel->setZValue(z + litt);
    }
}

/*!
    \since 4.6

    Sends event \a event to item \a item through possible event filters.

    The event is sent only if the item is enabled.

    Returns \c false if the event was filtered or if the item is disabled.
    Otherwise returns the value that was returned from the event handler.

    \sa QGraphicsItem::sceneEvent(), QGraphicsItem::sceneEventFilter()
*/
bool QGraphicsScene::sendEvent(QGraphicsItem *item, QEvent *event)
{
    Q_D(QGraphicsScene);
    if (!item) {
        qWarning("QGraphicsScene::sendEvent: cannot send event to a null item");
        return false;
    }
    if (item->scene() != this) {
        qWarning("QGraphicsScene::sendEvent: item %p's scene (%p)"
                 " is different from this scene (%p)",
                 item, item->scene(), this);
        return false;
    }
    return d->sendEvent(item, event);
}

void QGraphicsScenePrivate::addView(QGraphicsView *view)
{
    views << view;
#ifndef QT_NO_GESTURES
    foreach (Qt::GestureType gesture, grabbedGestures.keys())
        view->viewport()->grabGesture(gesture);
#endif
}

void QGraphicsScenePrivate::removeView(QGraphicsView *view)
{
    views.removeAll(view);
}

void QGraphicsScenePrivate::updateTouchPointsForItem(QGraphicsItem *item, QTouchEvent *touchEvent)
{
    QList<QTouchEvent::TouchPoint> touchPoints = touchEvent->touchPoints();
    for (int i = 0; i < touchPoints.count(); ++i) {
        QTouchEvent::TouchPoint &touchPoint = touchPoints[i];
        touchPoint.setRect(item->mapFromScene(touchPoint.sceneRect()).boundingRect());
        touchPoint.setStartPos(item->d_ptr->genericMapFromScene(touchPoint.startScenePos(), touchEvent->widget()));
        touchPoint.setLastPos(item->d_ptr->genericMapFromScene(touchPoint.lastScenePos(), touchEvent->widget()));
    }
    touchEvent->setTouchPoints(touchPoints);
}

int QGraphicsScenePrivate::findClosestTouchPointId(const QPointF &scenePos)
{
    int closestTouchPointId = -1;
    qreal closestDistance = qreal(0.);
    foreach (const QTouchEvent::TouchPoint &touchPoint, sceneCurrentTouchPoints) {
        qreal distance = QLineF(scenePos, touchPoint.scenePos()).length();
        if (closestTouchPointId == -1|| distance < closestDistance) {
            closestTouchPointId = touchPoint.id();
            closestDistance = distance;
        }
    }
    return closestTouchPointId;
}

void QGraphicsScenePrivate::touchEventHandler(QTouchEvent *sceneTouchEvent)
{
    typedef QPair<Qt::TouchPointStates, QList<QTouchEvent::TouchPoint> > StatesAndTouchPoints;
    QHash<QGraphicsItem *, StatesAndTouchPoints> itemsNeedingEvents;

    for (int i = 0; i < sceneTouchEvent->touchPoints().count(); ++i) {
        const QTouchEvent::TouchPoint &touchPoint = sceneTouchEvent->touchPoints().at(i);

        // update state
        QGraphicsItem *item = 0;
        if (touchPoint.state() == Qt::TouchPointPressed) {
            if (sceneTouchEvent->deviceType() == QTouchEvent::TouchPad) {
                // on touch-pad devices, send all touch points to the same item
                item = itemForTouchPointId.isEmpty()
                       ? 0
                       : itemForTouchPointId.constBegin().value();
            }

            if (!item) {
                // determine which item this touch point will go to
                cachedItemsUnderMouse = itemsAtPosition(touchPoint.screenPos().toPoint(),
                                                        touchPoint.scenePos(),
                                                        sceneTouchEvent->widget());
                item = cachedItemsUnderMouse.isEmpty() ? 0 : cachedItemsUnderMouse.first();
            }

            if (sceneTouchEvent->deviceType() == QTouchEvent::TouchScreen) {
                // on touch-screens, combine this touch point with the closest one we find
                int closestTouchPointId = findClosestTouchPointId(touchPoint.scenePos());
                QGraphicsItem *closestItem = itemForTouchPointId.value(closestTouchPointId);
                if (!item || (closestItem && cachedItemsUnderMouse.contains(closestItem)))
                    item = closestItem;
            }
            if (!item)
                continue;

            itemForTouchPointId.insert(touchPoint.id(), item);
            sceneCurrentTouchPoints.insert(touchPoint.id(), touchPoint);
        } else if (touchPoint.state() == Qt::TouchPointReleased) {
            item = itemForTouchPointId.take(touchPoint.id());
            if (!item)
                continue;

            sceneCurrentTouchPoints.remove(touchPoint.id());
        } else {
            item = itemForTouchPointId.value(touchPoint.id());
            if (!item)
                continue;
            Q_ASSERT(sceneCurrentTouchPoints.contains(touchPoint.id()));
            sceneCurrentTouchPoints[touchPoint.id()] = touchPoint;
        }

        StatesAndTouchPoints &statesAndTouchPoints = itemsNeedingEvents[item];
        statesAndTouchPoints.first |= touchPoint.state();
        statesAndTouchPoints.second.append(touchPoint);
    }

    if (itemsNeedingEvents.isEmpty()) {
        sceneTouchEvent->accept();
        return;
    }

    bool ignoreSceneTouchEvent = true;
    QHash<QGraphicsItem *, StatesAndTouchPoints>::ConstIterator it = itemsNeedingEvents.constBegin();
    const QHash<QGraphicsItem *, StatesAndTouchPoints>::ConstIterator end = itemsNeedingEvents.constEnd();
    for (; it != end; ++it) {
        QGraphicsItem *item = it.key();

        (void) item->isBlockedByModalPanel(&item);

        // determine event type from the state mask
        QEvent::Type eventType;
        switch (it.value().first) {
        case Qt::TouchPointPressed:
            // all touch points have pressed state
            eventType = QEvent::TouchBegin;
            break;
        case Qt::TouchPointReleased:
            // all touch points have released state
            eventType = QEvent::TouchEnd;
            break;
        case Qt::TouchPointStationary:
            // don't send the event if nothing changed
            continue;
        default:
            // all other combinations
            eventType = QEvent::TouchUpdate;
            break;
        }

        QTouchEvent touchEvent(eventType);
        touchEvent.setWidget(sceneTouchEvent->widget());
        touchEvent.setDeviceType(sceneTouchEvent->deviceType());
        touchEvent.setModifiers(sceneTouchEvent->modifiers());
        touchEvent.setTouchPointStates(it.value().first);
        touchEvent.setTouchPoints(it.value().second);

        switch (touchEvent.type()) {
        case QEvent::TouchBegin:
        {
            // if the TouchBegin handler recurses, we assume that means the event
            // has been implicitly accepted and continue to send touch events
            item->d_ptr->acceptedTouchBeginEvent = true;
            bool res = sendTouchBeginEvent(item, &touchEvent)
                       && touchEvent.isAccepted();
            if (!res) {
                // forget about these touch points, we didn't handle them
                for (int i = 0; i < touchEvent.touchPoints().count(); ++i) {
                    const QTouchEvent::TouchPoint &touchPoint = touchEvent.touchPoints().at(i);
                    itemForTouchPointId.remove(touchPoint.id());
                    sceneCurrentTouchPoints.remove(touchPoint.id());
                }
                ignoreSceneTouchEvent = false;
            }
            break;
        }
        default:
            if (item->d_ptr->acceptedTouchBeginEvent) {
                updateTouchPointsForItem(item, &touchEvent);
                (void) sendEvent(item, &touchEvent);
                ignoreSceneTouchEvent = false;
            }
            break;
        }
    }
    sceneTouchEvent->setAccepted(ignoreSceneTouchEvent);
}

bool QGraphicsScenePrivate::sendTouchBeginEvent(QGraphicsItem *origin, QTouchEvent *touchEvent)
{
    Q_Q(QGraphicsScene);

    if (cachedItemsUnderMouse.isEmpty() || cachedItemsUnderMouse.first() != origin) {
        const QTouchEvent::TouchPoint &firstTouchPoint = touchEvent->touchPoints().first();
        cachedItemsUnderMouse = itemsAtPosition(firstTouchPoint.screenPos().toPoint(),
                                                firstTouchPoint.scenePos(),
                                                touchEvent->widget());
    }
    Q_ASSERT(cachedItemsUnderMouse.first() == origin);

    // Set focus on the topmost enabled item that can take focus.
    bool setFocus = false;

    foreach (QGraphicsItem *item, cachedItemsUnderMouse) {
        if (item->isEnabled() && ((item->flags() & QGraphicsItem::ItemIsFocusable) && item->d_ptr->mouseSetsFocus)) {
            if (!item->isWidget() || ((QGraphicsWidget *)item)->focusPolicy() & Qt::ClickFocus) {
                setFocus = true;
                if (item != q->focusItem())
                    q->setFocusItem(item, Qt::MouseFocusReason);
                break;
            }
        }
        if (item->isPanel())
            break;
        if (item->d_ptr->flags & QGraphicsItem::ItemStopsClickFocusPropagation)
            break;
        if (item->d_ptr->flags & QGraphicsItem::ItemStopsFocusHandling) {
            // Make sure we don't clear focus.
            setFocus = true;
            break;
        }
    }

    // If nobody could take focus, clear it.
    if (!stickyFocus && !setFocus)
        q->setFocusItem(0, Qt::MouseFocusReason);

    bool res = false;
    bool eventAccepted = touchEvent->isAccepted();
    foreach (QGraphicsItem *item, cachedItemsUnderMouse) {
        // first, try to deliver the touch event
        updateTouchPointsForItem(item, touchEvent);
        bool acceptTouchEvents = item->acceptTouchEvents();
        touchEvent->setAccepted(acceptTouchEvents);
        res = acceptTouchEvents && sendEvent(item, touchEvent);
        eventAccepted = touchEvent->isAccepted();
        if (itemForTouchPointId.value(touchEvent->touchPoints().first().id()) == 0) {
            // item was deleted
            item = 0;
        } else {
            item->d_ptr->acceptedTouchBeginEvent = (res && eventAccepted);
        }
        touchEvent->spont = false;
        if (res && eventAccepted) {
            // the first item to accept the TouchBegin gets an implicit grab.
            for (int i = 0; i < touchEvent->touchPoints().count(); ++i) {
                const QTouchEvent::TouchPoint &touchPoint = touchEvent->touchPoints().at(i);
                itemForTouchPointId[touchPoint.id()] = item; // can be zero
            }
            break;
        }
        if (item && item->isPanel())
            break;
    }

    touchEvent->setAccepted(eventAccepted);
    return res;
}

void QGraphicsScenePrivate::enableTouchEventsOnViews()
{
    foreach (QGraphicsView *view, views)
        view->viewport()->setAttribute(Qt::WA_AcceptTouchEvents, true);
}

void QGraphicsScenePrivate::updateInputMethodSensitivityInViews()
{
    for (int i = 0; i < views.size(); ++i)
        views.at(i)->d_func()->updateInputMethodSensitivity();
}

void QGraphicsScenePrivate::enterModal(QGraphicsItem *panel, QGraphicsItem::PanelModality previousModality)
{
    Q_Q(QGraphicsScene);
    Q_ASSERT(panel && panel->isPanel());

    QGraphicsItem::PanelModality panelModality = panel->d_ptr->panelModality;
    if (previousModality != QGraphicsItem::NonModal) {
        // the panel is changing from one modality type to another... temporarily set it back so
        // that blockedPanels is populated correctly
        panel->d_ptr->panelModality = previousModality;
    }

    QSet<QGraphicsItem *> blockedPanels;
    QList<QGraphicsItem *> items = q->items(); // ### store panels separately
    for (int i = 0; i < items.count(); ++i) {
        QGraphicsItem *item = items.at(i);
        if (item->isPanel() && item->isBlockedByModalPanel())
            blockedPanels.insert(item);
    }
    // blockedPanels contains all currently blocked panels

    if (previousModality != QGraphicsItem::NonModal) {
        // reset the modality to the proper value, since we changed it above
        panel->d_ptr->panelModality = panelModality;
        // remove this panel so that it will be reinserted at the front of the stack
        modalPanels.removeAll(panel);
    }

    modalPanels.prepend(panel);

    if (!hoverItems.isEmpty()) {
        // send GraphicsSceneHoverLeave events to newly blocked hoverItems
        QGraphicsSceneHoverEvent hoverEvent;
        hoverEvent.setScenePos(lastSceneMousePos);
        dispatchHoverEvent(&hoverEvent);
    }

    if (!mouseGrabberItems.isEmpty() && lastMouseGrabberItemHasImplicitMouseGrab) {
        QGraphicsItem *item = mouseGrabberItems.last();
        if (item->isBlockedByModalPanel())
            ungrabMouse(item, /*itemIsDying =*/ false);
    }

    QEvent windowBlockedEvent(QEvent::WindowBlocked);
    QEvent windowUnblockedEvent(QEvent::WindowUnblocked);
    for (int i = 0; i < items.count(); ++i) {
        QGraphicsItem *item = items.at(i);
        if (item->isPanel()) {
            if (!blockedPanels.contains(item) && item->isBlockedByModalPanel()) {
                // send QEvent::WindowBlocked to newly blocked panels
                sendEvent(item, &windowBlockedEvent);
            } else if (blockedPanels.contains(item) && !item->isBlockedByModalPanel()) {
                // send QEvent::WindowUnblocked to unblocked panels when downgrading
                // a panel from SceneModal to PanelModal
                sendEvent(item, &windowUnblockedEvent);
            }
        }
    }
}

void QGraphicsScenePrivate::leaveModal(QGraphicsItem *panel)
{
    Q_Q(QGraphicsScene);
    Q_ASSERT(panel && panel->isPanel());

    QSet<QGraphicsItem *> blockedPanels;
    QList<QGraphicsItem *> items = q->items(); // ### same as above
    for (int i = 0; i < items.count(); ++i) {
        QGraphicsItem *item = items.at(i);
        if (item->isPanel() && item->isBlockedByModalPanel())
            blockedPanels.insert(item);
    }

    modalPanels.removeAll(panel);

    QEvent e(QEvent::WindowUnblocked);
    for (int i = 0; i < items.count(); ++i) {
        QGraphicsItem *item = items.at(i);
        if (item->isPanel() && blockedPanels.contains(item) && !item->isBlockedByModalPanel())
            sendEvent(item, &e);
    }

    // send GraphicsSceneHoverEnter events to newly unblocked items
    QGraphicsSceneHoverEvent hoverEvent;
    hoverEvent.setScenePos(lastSceneMousePos);
    dispatchHoverEvent(&hoverEvent);
}

#ifndef QT_NO_GESTURES
void QGraphicsScenePrivate::gestureTargetsAtHotSpots(const QSet<QGesture *> &gestures,
                                              Qt::GestureFlag flag,
                                              QHash<QGraphicsObject *, QSet<QGesture *> > *targets,
                                              QSet<QGraphicsObject *> *itemsSet,
                                              QSet<QGesture *> *normal,
                                              QSet<QGesture *> *conflicts)
{
    QSet<QGesture *> normalGestures; // that are not in conflicted state.
    foreach (QGesture *gesture, gestures) {
        if (!gesture->hasHotSpot())
            continue;
        const Qt::GestureType gestureType = gesture->gestureType();
        QList<QGraphicsItem *> items = itemsAtPosition(QPoint(), gesture->d_func()->sceneHotSpot, 0);
        for (int j = 0; j < items.size(); ++j) {
            QGraphicsItem *item = items.at(j);

            // Check if the item is blocked by a modal panel and use it as
            // a target instead of this item.
            (void) item->isBlockedByModalPanel(&item);

            if (QGraphicsObject *itemobj = item->toGraphicsObject()) {
                QGraphicsItemPrivate *d = item->QGraphicsItem::d_func();
                QMap<Qt::GestureType, Qt::GestureFlags>::const_iterator it =
                        d->gestureContext.find(gestureType);
                if (it != d->gestureContext.end() && (!flag || (it.value() & flag))) {
                    if (normalGestures.contains(gesture)) {
                        normalGestures.remove(gesture);
                        if (conflicts)
                            conflicts->insert(gesture);
                    } else {
                        normalGestures.insert(gesture);
                    }
                    if (targets)
                        (*targets)[itemobj].insert(gesture);
                    if (itemsSet)
                        (*itemsSet).insert(itemobj);
                }
            }
            // Don't propagate through panels.
            if (item->isPanel())
                break;
        }
    }
    if (normal)
        *normal = normalGestures;
}

void QGraphicsScenePrivate::gestureEventHandler(QGestureEvent *event)
{
    QWidget *viewport = event->widget();
    if (!viewport)
        return;
    QGraphicsView *graphicsView = qobject_cast<QGraphicsView *>(viewport->parent());
    if (!graphicsView)
        return;

    QList<QGesture *> allGestures = event->gestures();
    G_DEBUG() << "QGraphicsScenePrivate::gestureEventHandler:"
            << "Gestures:" <<  allGestures;

    QSet<QGesture *> startedGestures;
    QPoint delta = viewport->mapFromGlobal(QPoint());
    QTransform toScene = QTransform::fromTranslate(delta.x(), delta.y())
                         * graphicsView->viewportTransform().inverted();
    foreach (QGesture *gesture, allGestures) {
        // cache scene coordinates of the hot spot
        if (gesture->hasHotSpot()) {
            gesture->d_func()->sceneHotSpot = toScene.map(gesture->hotSpot());
        } else {
            gesture->d_func()->sceneHotSpot = QPointF();
        }

        QGraphicsObject *target = gestureTargets.value(gesture, 0);
        if (!target) {
            // when we are not in started mode but don't have a target
            // then the only one interested in gesture is the view/scene
            if (gesture->state() == Qt::GestureStarted)
                startedGestures.insert(gesture);
        }
    }

    if (!startedGestures.isEmpty()) {
        QSet<QGesture *> normalGestures; // that have just one target
        QSet<QGesture *> conflictedGestures; // that have multiple possible targets
        gestureTargetsAtHotSpots(startedGestures, Qt::GestureFlag(0), &cachedItemGestures, 0,
                                 &normalGestures, &conflictedGestures);
        cachedTargetItems = cachedItemGestures.keys();
        qSort(cachedTargetItems.begin(), cachedTargetItems.end(), qt_closestItemFirst);
        G_DEBUG() << "QGraphicsScenePrivate::gestureEventHandler:"
                << "Normal gestures:" << normalGestures
                << "Conflicting gestures:" << conflictedGestures;

        // deliver conflicted gestures as override events AND remember
        // initial gesture targets
        if (!conflictedGestures.isEmpty()) {
            for (int i = 0; i < cachedTargetItems.size(); ++i) {
                QWeakPointer<QGraphicsObject> item = cachedTargetItems.at(i);

                // get gestures to deliver to the current item
                QSet<QGesture *> gestures = conflictedGestures & cachedItemGestures.value(item.data());
                if (gestures.isEmpty())
                    continue;

                G_DEBUG() << "QGraphicsScenePrivate::gestureEventHandler:"
                        << "delivering override to"
                        << item.data() << gestures;
                // send gesture override
                QGestureEvent ev(gestures.toList());
                ev.t = QEvent::GestureOverride;
                ev.setWidget(event->widget());
                // mark event and individual gestures as ignored
                ev.ignore();
                foreach(QGesture *g, gestures)
                    ev.setAccepted(g, false);
                sendEvent(item.data(), &ev);
                // mark all accepted gestures to deliver them as normal gesture events
                foreach (QGesture *g, gestures) {
                    if (ev.isAccepted() || ev.isAccepted(g)) {
                        conflictedGestures.remove(g);
                        // mark the item as a gesture target
                        if (item) {
                            gestureTargets.insert(g, item.data());
                            QHash<QGraphicsObject *, QSet<QGesture *> >::iterator it, e;
                            it = cachedItemGestures.begin();
                            e = cachedItemGestures.end();
                            for(; it != e; ++it)
                                it.value().remove(g);
                            cachedItemGestures[item.data()].insert(g);
                        }
                        G_DEBUG() << "QGraphicsScenePrivate::gestureEventHandler:"
                                << "override was accepted:"
                                << g << item.data();
                    }
                    // remember the first item that received the override event
                    // as it most likely become a target if no one else accepts
                    // the override event
                    if (!gestureTargets.contains(g) && item)
                        gestureTargets.insert(g, item.data());

                }
                if (conflictedGestures.isEmpty())
                    break;
            }
        }
        // remember the initial target item for each gesture that was not in
        // the conflicted state.
        if (!normalGestures.isEmpty()) {
            for (int i = 0; i < cachedTargetItems.size() && !normalGestures.isEmpty(); ++i) {
                QGraphicsObject *item = cachedTargetItems.at(i);

                // get gestures to deliver to the current item
                foreach (QGesture *g, cachedItemGestures.value(item)) {
                    if (!gestureTargets.contains(g)) {
                        gestureTargets.insert(g, item);
                        normalGestures.remove(g);
                    }
                }
            }
        }
    }


    // deliver all gesture events
    QSet<QGesture *> undeliveredGestures;
    QSet<QGesture *> parentPropagatedGestures;
    foreach (QGesture *gesture, allGestures) {
        if (QGraphicsObject *target = gestureTargets.value(gesture, 0)) {
            cachedItemGestures[target].insert(gesture);
            cachedTargetItems.append(target);
            undeliveredGestures.insert(gesture);
            QGraphicsItemPrivate *d = target->QGraphicsItem::d_func();
            const Qt::GestureFlags flags = d->gestureContext.value(gesture->gestureType());
            if (flags & Qt::IgnoredGesturesPropagateToParent)
                parentPropagatedGestures.insert(gesture);
        } else {
            G_DEBUG() << "QGraphicsScenePrivate::gestureEventHandler:"
                    << "no target for" << gesture << "at"
                    << gesture->hotSpot() << gesture->d_func()->sceneHotSpot;
        }
    }
    qSort(cachedTargetItems.begin(), cachedTargetItems.end(), qt_closestItemFirst);
    for (int i = 0; i < cachedTargetItems.size(); ++i) {
        QWeakPointer<QGraphicsObject> receiver = cachedTargetItems.at(i);
        QSet<QGesture *> gestures =
                undeliveredGestures & cachedItemGestures.value(receiver.data());
        gestures -= cachedAlreadyDeliveredGestures.value(receiver.data());

        if (gestures.isEmpty())
            continue;

        cachedAlreadyDeliveredGestures[receiver.data()] += gestures;
        const bool isPanel = receiver.data()->isPanel();

        G_DEBUG() << "QGraphicsScenePrivate::gestureEventHandler:"
                << "delivering to"
                << receiver.data() << gestures;
        QGestureEvent ev(gestures.toList());
        ev.setWidget(event->widget());
        sendEvent(receiver.data(), &ev);
        QSet<QGesture *> ignoredGestures;
        foreach (QGesture *g, gestures) {
            if (!ev.isAccepted() && !ev.isAccepted(g)) {
                // if the gesture was ignored by its target, we will update the
                // targetItems list with a possible target items (items that
                // want to receive partial gestures).
                // ### wont' work if the target was destroyed in the event
                //     we will just stop delivering it.
                if (receiver && receiver.data() == gestureTargets.value(g, 0))
                    ignoredGestures.insert(g);
            } else {
                if (receiver && g->state() == Qt::GestureStarted) {
                    // someone accepted the propagated initial GestureStarted
                    // event, let it be the new target for all following events.
                    gestureTargets[g] = receiver.data();
                }
                undeliveredGestures.remove(g);
            }
        }
        if (undeliveredGestures.isEmpty())
            break;

        // ignoredGestures list is only filled when delivering to the gesture
        // target item, so it is safe to assume item == target.
        if (!ignoredGestures.isEmpty() && !isPanel) {
            // look for new potential targets for gestures that were ignored
            // and should be propagated.

            QSet<QGraphicsObject *> targetsSet = cachedTargetItems.toSet();

            if (receiver) {
                // first if the gesture should be propagated to parents only
                for (QSet<QGesture *>::iterator it = ignoredGestures.begin();
                     it != ignoredGestures.end();) {
                    if (parentPropagatedGestures.contains(*it)) {
                        QGesture *gesture = *it;
                        const Qt::GestureType gestureType = gesture->gestureType();
                        QGraphicsItem *item = receiver.data();
                        while (item) {
                            if (QGraphicsObject *obj = item->toGraphicsObject()) {
                                if (item->d_func()->gestureContext.contains(gestureType)) {
                                    targetsSet.insert(obj);
                                    cachedItemGestures[obj].insert(gesture);
                                }
                            }
                            if (item->isPanel())
                                break;
                            item = item->parentItem();
                        }

                        it = ignoredGestures.erase(it);
                        continue;
                    }
                    ++it;
                }
            }

            gestureTargetsAtHotSpots(ignoredGestures, Qt::ReceivePartialGestures,
                                     &cachedItemGestures, &targetsSet, 0, 0);

            cachedTargetItems = targetsSet.toList();
            qSort(cachedTargetItems.begin(), cachedTargetItems.end(), qt_closestItemFirst);
            G_DEBUG() << "QGraphicsScenePrivate::gestureEventHandler:"
                    << "new targets:" << cachedTargetItems;
            i = -1; // start delivery again
            continue;
        }
    }

    foreach (QGesture *g, startedGestures) {
        if (g->gestureCancelPolicy() == QGesture::CancelAllInContext) {
            G_DEBUG() << "lets try to cancel some";
            // find gestures in context in Qt::GestureStarted or Qt::GestureUpdated state and cancel them
            cancelGesturesForChildren(g);
        }
    }

    // forget about targets for gestures that have ended
    foreach (QGesture *g, allGestures) {
        switch (g->state()) {
        case Qt::GestureFinished:
        case Qt::GestureCanceled:
            gestureTargets.remove(g);
            break;
        default:
            break;
        }
    }

    cachedTargetItems.clear();
    cachedItemGestures.clear();
    cachedAlreadyDeliveredGestures.clear();
}

void QGraphicsScenePrivate::cancelGesturesForChildren(QGesture *original)
{
    Q_ASSERT(original);
    QGraphicsItem *originalItem = gestureTargets.value(original);
    if (originalItem == 0) // we only act on accepted gestures, which implies it has a target.
        return;

    // iterate over all active gestures and for each find the owner
    // if the owner is part of our sub-hierarchy, cancel it.

    QSet<QGesture *> canceledGestures;
    QHash<QGesture *, QGraphicsObject *>::Iterator iter = gestureTargets.begin();
    while (iter != gestureTargets.end()) {
        QGraphicsObject *item = iter.value();
        // note that we don't touch the gestures for our originalItem
        if (item != originalItem && originalItem->isAncestorOf(item)) {
            G_DEBUG() << "  found a gesture to cancel" << iter.key();
            iter.key()->d_func()->state = Qt::GestureCanceled;
            canceledGestures << iter.key();
        }
        ++iter;
    }

    // sort them per target item by cherry picking from almostCanceledGestures and delivering
    QSet<QGesture *> almostCanceledGestures = canceledGestures;
    QSet<QGesture *>::Iterator setIter;
    while (!almostCanceledGestures.isEmpty()) {
        QGraphicsObject *target = 0;
        QSet<QGesture*> gestures;
        setIter = almostCanceledGestures.begin();
        // sort per target item
        while (setIter != almostCanceledGestures.end()) {
            QGraphicsObject *item = gestureTargets.value(*setIter);
            if (target == 0)
                target = item;
            if (target == item) {
                gestures << *setIter;
                setIter = almostCanceledGestures.erase(setIter);
            } else {
                ++setIter;
            }
        }
        Q_ASSERT(target);

        QList<QGesture *> list = gestures.toList();
        QGestureEvent ev(list);
        sendEvent(target, &ev);

        foreach (QGesture *g, list) {
            if (ev.isAccepted() || ev.isAccepted(g))
                gestures.remove(g);
        }

        foreach (QGesture *g, gestures) {
            if (!g->hasHotSpot())
                continue;

            QList<QGraphicsItem *> items = itemsAtPosition(QPoint(), g->d_func()->sceneHotSpot, 0);
            for (int j = 0; j < items.size(); ++j) {
                QGraphicsObject *item = items.at(j)->toGraphicsObject();
                if (!item)
                    continue;
                QGraphicsItemPrivate *d = item->QGraphicsItem::d_func();
                if (d->gestureContext.contains(g->gestureType())) {
                    QList<QGesture *> list;
                    list << g;
                    QGestureEvent ev(list);
                    sendEvent(item, &ev);
                    if (ev.isAccepted() || ev.isAccepted(g))
                        break; // successfully delivered
                }
            }
        }
    }

    QGestureManager *gestureManager = QApplicationPrivate::instance()->gestureManager;
    Q_ASSERT(gestureManager); // it would be very odd if we got called without a manager.
    for (setIter = canceledGestures.begin(); setIter != canceledGestures.end(); ++setIter) {
        gestureManager->recycle(*setIter);
        gestureTargets.remove(*setIter);
    }
}

void QGraphicsScenePrivate::grabGesture(QGraphicsItem *, Qt::GestureType gesture)
{
    (void)QGestureManager::instance(); // create a gesture manager
    if (!grabbedGestures[gesture]++) {
        foreach (QGraphicsView *view, views)
            view->viewport()->grabGesture(gesture);
    }
}

void QGraphicsScenePrivate::ungrabGesture(QGraphicsItem *item, Qt::GestureType gesture)
{
    // we know this can only be an object
    Q_ASSERT(item->d_ptr->isObject);
    QGraphicsObject *obj = static_cast<QGraphicsObject *>(item);
    QGestureManager::instance()->cleanupCachedGestures(obj, gesture);
    if (!--grabbedGestures[gesture]) {
        foreach (QGraphicsView *view, views)
            view->viewport()->ungrabGesture(gesture);
    }
}
#endif // QT_NO_GESTURES

QT_END_NAMESPACE

#include "moc_qgraphicsscene.cpp"

#endif // QT_NO_GRAPHICSVIEW
