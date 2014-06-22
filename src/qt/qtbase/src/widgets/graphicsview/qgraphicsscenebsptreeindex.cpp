/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtWidgets module of the Qt Toolkit.
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

/*!
    \class QGraphicsSceneBspTreeIndex
    \brief The QGraphicsSceneBspTreeIndex class provides an implementation of
    a BSP indexing algorithm for discovering items in QGraphicsScene.
    \since 4.6
    \ingroup graphicsview-api

    \internal

    QGraphicsSceneBspTreeIndex index use a BSP(Binary Space Partitioning)
    implementation to discover items quickly. This implementation is
    very efficient for static scenes. It has a depth that you can set.
    The depth directly affects performance and memory usage; the latter
    growing exponentially with the depth of the tree. With an optimal tree
    depth, the index can instantly determine the locality of items, even
    for scenes with thousands or millions of items. This also greatly improves
    rendering performance.

    By default, the depth value is 0, in which case Qt will guess a reasonable
    default depth based on the size, location and number of items in the
    scene. If these parameters change frequently, however, you may experience
    slowdowns as the index retunes the depth internally. You can avoid
    potential slowdowns by fixating the tree depth through setting this
    property.

    The depth of the tree and the size of the scene rectangle decide the
    granularity of the scene's partitioning. The size of each scene segment is
    determined by the following algorithm:

    The BSP tree has an optimal size when each segment contains between 0 and
    10 items.

    \sa QGraphicsScene, QGraphicsView, QGraphicsSceneIndex
*/

#include <QtCore/qglobal.h>

#ifndef QT_NO_GRAPHICSVIEW

#include <private/qgraphicsscene_p.h>
#include <private/qgraphicsscenebsptreeindex_p.h>
#include <private/qgraphicssceneindex_p.h>

#include <QtCore/qmath.h>
#include <QtCore/qdebug.h>

QT_BEGIN_NAMESPACE

static inline int intmaxlog(int n)
{
    return  (n > 0 ? qMax(qCeil(qLn(qreal(n)) / qLn(qreal(2))), 5) : 0);
}

/*!
    Constructs a private scene bsp index.
*/
QGraphicsSceneBspTreeIndexPrivate::QGraphicsSceneBspTreeIndexPrivate(QGraphicsScene *scene)
    : QGraphicsSceneIndexPrivate(scene),
    bspTreeDepth(0),
    indexTimerId(0),
    restartIndexTimer(false),
    regenerateIndex(true),
    lastItemCount(0),
    purgePending(false),
    sortCacheEnabled(false),
    updatingSortCache(false)
{
}


/*!
    This method will update the BSP index by removing the items from the temporary
    unindexed list and add them in the indexedItems list. This will also
    update the growingItemsBoundingRect if needed. This will update the BSP
    implementation as well.

    \internal
*/
void QGraphicsSceneBspTreeIndexPrivate::_q_updateIndex()
{
    Q_Q(QGraphicsSceneBspTreeIndex);
    if (!indexTimerId)
        return;

    q->killTimer(indexTimerId);
    indexTimerId = 0;

    purgeRemovedItems();

     // Add unindexedItems to indexedItems
    for (int i = 0; i < unindexedItems.size(); ++i) {
        if (QGraphicsItem *item = unindexedItems.at(i)) {
            Q_ASSERT(!item->d_ptr->itemDiscovered);
            if (!freeItemIndexes.isEmpty()) {
                int freeIndex = freeItemIndexes.takeFirst();
                item->d_func()->index = freeIndex;
                indexedItems[freeIndex] = item;
            } else {
                item->d_func()->index = indexedItems.size();
                indexedItems << item;
            }
        }
    }

    // Determine whether we should regenerate the BSP tree.
    if (bspTreeDepth == 0) {
        int oldDepth = intmaxlog(lastItemCount);
        bspTreeDepth = intmaxlog(indexedItems.size());
        static const int slack = 100;
        if (bsp.leafCount() == 0 || (oldDepth != bspTreeDepth && qAbs(lastItemCount - indexedItems.size()) > slack)) {
            // ### Crude algorithm.
            regenerateIndex = true;
        }
    }

    // Regenerate the tree.
    if (regenerateIndex) {
        regenerateIndex = false;
        bsp.initialize(sceneRect, bspTreeDepth);
        unindexedItems = indexedItems;
        lastItemCount = indexedItems.size();
    }

    // Insert all unindexed items into the tree.
    for (int i = 0; i < unindexedItems.size(); ++i) {
        if (QGraphicsItem *item = unindexedItems.at(i)) {
            if (item->d_ptr->itemIsUntransformable()) {
                untransformableItems << item;
                continue;
            }
            if (item->d_ptr->ancestorFlags & QGraphicsItemPrivate::AncestorClipsChildren)
                continue;

            bsp.insertItem(item, item->d_ptr->sceneEffectiveBoundingRect());
        }
    }
    unindexedItems.clear();
}


/*!
    \internal

    Removes stale pointers from all data structures.
*/
void QGraphicsSceneBspTreeIndexPrivate::purgeRemovedItems()
{
    if (!purgePending && removedItems.isEmpty())
        return;

    // Remove stale items from the BSP tree.
    bsp.removeItems(removedItems);
    // Purge this list.
    removedItems.clear();
    freeItemIndexes.clear();
    for (int i = 0; i < indexedItems.size(); ++i) {
        if (!indexedItems.at(i))
            freeItemIndexes << i;
    }
    purgePending = false;
}

/*!
    \internal

    Starts or restarts the timer used for reindexing unindexed items.
*/
void QGraphicsSceneBspTreeIndexPrivate::startIndexTimer(int interval)
{
    Q_Q(QGraphicsSceneBspTreeIndex);
    if (indexTimerId) {
        restartIndexTimer = true;
    } else {
        indexTimerId = q->startTimer(interval);
    }
}

/*!
    \internal
*/
void QGraphicsSceneBspTreeIndexPrivate::resetIndex()
{
    purgeRemovedItems();
    for (int i = 0; i < indexedItems.size(); ++i) {
        if (QGraphicsItem *item = indexedItems.at(i)) {
            item->d_ptr->index = -1;
            Q_ASSERT(!item->d_ptr->itemDiscovered);
            unindexedItems << item;
        }
    }
    indexedItems.clear();
    freeItemIndexes.clear();
    untransformableItems.clear();
    regenerateIndex = true;
    startIndexTimer();
}

/*!
    \internal
*/
void QGraphicsSceneBspTreeIndexPrivate::climbTree(QGraphicsItem *item, int *stackingOrder)
{
    if (!item->d_ptr->children.isEmpty()) {
        QList<QGraphicsItem *> childList = item->d_ptr->children;
        std::sort(childList.begin(), childList.end(), qt_closestLeaf);
        for (int i = 0; i < childList.size(); ++i) {
            QGraphicsItem *item = childList.at(i);
            if (!(item->flags() & QGraphicsItem::ItemStacksBehindParent))
                climbTree(childList.at(i), stackingOrder);
        }
        item->d_ptr->globalStackingOrder = (*stackingOrder)++;
        for (int i = 0; i < childList.size(); ++i) {
            QGraphicsItem *item = childList.at(i);
            if (item->flags() & QGraphicsItem::ItemStacksBehindParent)
                climbTree(childList.at(i), stackingOrder);
        }
    } else {
        item->d_ptr->globalStackingOrder = (*stackingOrder)++;
    }
}

/*!
    \internal
*/
void QGraphicsSceneBspTreeIndexPrivate::_q_updateSortCache()
{
    Q_Q(QGraphicsSceneBspTreeIndex);
    _q_updateIndex();

    if (!sortCacheEnabled || !updatingSortCache)
        return;

    updatingSortCache = false;
    int stackingOrder = 0;

    QList<QGraphicsItem *> topLevels;
    const QList<QGraphicsItem *> items = q->items();
    for (int i = 0; i < items.size(); ++i) {
        QGraphicsItem *item = items.at(i);
        if (item && !item->d_ptr->parent)
            topLevels << item;
    }

    std::sort(topLevels.begin(), topLevels.end(), qt_closestLeaf);
    for (int i = 0; i < topLevels.size(); ++i)
        climbTree(topLevels.at(i), &stackingOrder);
}

/*!
    \internal
*/
void QGraphicsSceneBspTreeIndexPrivate::invalidateSortCache()
{
    Q_Q(QGraphicsSceneBspTreeIndex);
    if (!sortCacheEnabled || updatingSortCache)
        return;

    updatingSortCache = true;
    QMetaObject::invokeMethod(q, "_q_updateSortCache", Qt::QueuedConnection);
}

void QGraphicsSceneBspTreeIndexPrivate::addItem(QGraphicsItem *item, bool recursive)
{
    if (!item)
        return;

    // Prevent reusing a recently deleted pointer: purge all removed item from our lists.
    purgeRemovedItems();

    // Invalidate any sort caching; arrival of a new item means we need to resort.
    // Update the scene's sort cache settings.
    item->d_ptr->globalStackingOrder = -1;
    invalidateSortCache();

    // Indexing requires sceneBoundingRect(), but because \a item might
    // not be completely constructed at this point, we need to store it in
    // a temporary list and schedule an indexing for later.
    if (item->d_ptr->index == -1) {
        Q_ASSERT(!unindexedItems.contains(item));
        unindexedItems << item;
        startIndexTimer(0);
    } else {
        Q_ASSERT(indexedItems.contains(item));
        qWarning("QGraphicsSceneBspTreeIndex::addItem: item has already been added to this BSP");
    }

    if (recursive) {
        for (int i = 0; i < item->d_ptr->children.size(); ++i)
            addItem(item->d_ptr->children.at(i), recursive);
    }
}

void QGraphicsSceneBspTreeIndexPrivate::removeItem(QGraphicsItem *item, bool recursive,
                                                   bool moveToUnindexedItems)
{
    if (!item)
        return;

    if (item->d_ptr->index != -1) {
        Q_ASSERT(item->d_ptr->index < indexedItems.size());
        Q_ASSERT(indexedItems.at(item->d_ptr->index) == item);
        Q_ASSERT(!item->d_ptr->itemDiscovered);
        freeItemIndexes << item->d_ptr->index;
        indexedItems[item->d_ptr->index] = 0;
        item->d_ptr->index = -1;

        if (item->d_ptr->itemIsUntransformable()) {
            untransformableItems.removeOne(item);
        } else if (item->d_ptr->inDestructor) {
            // Avoid virtual function calls from the destructor.
            purgePending = true;
            removedItems << item;
        } else if (!(item->d_ptr->ancestorFlags & QGraphicsItemPrivate::AncestorClipsChildren)) {
            bsp.removeItem(item, item->d_ptr->sceneEffectiveBoundingRect());
        }
    } else {
        unindexedItems.removeOne(item);
    }
    invalidateSortCache(); // ### Only do this when removing from BSP?

    Q_ASSERT(item->d_ptr->index == -1);
    Q_ASSERT(!indexedItems.contains(item));
    Q_ASSERT(!unindexedItems.contains(item));
    Q_ASSERT(!untransformableItems.contains(item));

    if (moveToUnindexedItems)
        addItem(item);

    if (recursive) {
        for (int i = 0; i < item->d_ptr->children.size(); ++i)
            removeItem(item->d_ptr->children.at(i), recursive, moveToUnindexedItems);
    }
}

QList<QGraphicsItem *> QGraphicsSceneBspTreeIndexPrivate::estimateItems(const QRectF &rect, Qt::SortOrder order,
                                                                        bool onlyTopLevelItems)
{
    Q_Q(QGraphicsSceneBspTreeIndex);
    if (onlyTopLevelItems && rect.isNull())
        return q->QGraphicsSceneIndex::estimateTopLevelItems(rect, order);

    purgeRemovedItems();
    _q_updateSortCache();
    Q_ASSERT(unindexedItems.isEmpty());

    QList<QGraphicsItem *> rectItems = bsp.items(rect, onlyTopLevelItems);
    if (onlyTopLevelItems) {
        for (int i = 0; i < untransformableItems.size(); ++i) {
            QGraphicsItem *item = untransformableItems.at(i);
            if (!item->d_ptr->parent) {
                rectItems << item;
            } else {
                item = item->topLevelItem();
                if (!rectItems.contains(item))
                    rectItems << item;
            }
        }
    } else {
        rectItems += untransformableItems;
    }

    sortItems(&rectItems, order, sortCacheEnabled, onlyTopLevelItems);
    return rectItems;
}

/*!
    Sort a list of \a itemList in a specific \a order and use the cache if requested.

    \internal
*/
void QGraphicsSceneBspTreeIndexPrivate::sortItems(QList<QGraphicsItem *> *itemList, Qt::SortOrder order,
                                                  bool sortCacheEnabled, bool onlyTopLevelItems)
{
    if (order == Qt::SortOrder(-1))
        return;

    if (onlyTopLevelItems) {
        if (order == Qt::DescendingOrder)
            std::sort(itemList->begin(), itemList->end(), qt_closestLeaf);
        else if (order == Qt::AscendingOrder)
            std::sort(itemList->begin(), itemList->end(), qt_notclosestLeaf);
        return;
    }

    if (sortCacheEnabled) {
        if (order == Qt::DescendingOrder) {
            std::sort(itemList->begin(), itemList->end(), closestItemFirst_withCache);
        } else if (order == Qt::AscendingOrder) {
            std::sort(itemList->begin(), itemList->end(), closestItemLast_withCache);
        }
    } else {
        if (order == Qt::DescendingOrder) {
            std::sort(itemList->begin(), itemList->end(), qt_closestItemFirst);
        } else if (order == Qt::AscendingOrder) {
            std::sort(itemList->begin(), itemList->end(), qt_closestItemLast);
        }
    }
}

/*!
    Constructs a BSP scene index for the given \a scene.
*/
QGraphicsSceneBspTreeIndex::QGraphicsSceneBspTreeIndex(QGraphicsScene *scene)
    : QGraphicsSceneIndex(*new QGraphicsSceneBspTreeIndexPrivate(scene), scene)
{

}

QGraphicsSceneBspTreeIndex::~QGraphicsSceneBspTreeIndex()
{
    Q_D(QGraphicsSceneBspTreeIndex);
    for (int i = 0; i < d->indexedItems.size(); ++i) {
        // Ensure item bits are reset properly.
        if (QGraphicsItem *item = d->indexedItems.at(i)) {
            Q_ASSERT(!item->d_ptr->itemDiscovered);
            item->d_ptr->index = -1;
        }
    }
}

/*!
    \internal
    Clear the all the BSP index.
*/
void QGraphicsSceneBspTreeIndex::clear()
{
    Q_D(QGraphicsSceneBspTreeIndex);
    d->bsp.clear();
    d->lastItemCount = 0;
    d->freeItemIndexes.clear();
    for (int i = 0; i < d->indexedItems.size(); ++i) {
        // Ensure item bits are reset properly.
        if (QGraphicsItem *item = d->indexedItems.at(i)) {
            Q_ASSERT(!item->d_ptr->itemDiscovered);
            item->d_ptr->index = -1;
        }
    }
    d->indexedItems.clear();
    d->unindexedItems.clear();
    d->untransformableItems.clear();
    d->regenerateIndex = true;
}

/*!
    Add the \a item into the BSP index.
*/
void QGraphicsSceneBspTreeIndex::addItem(QGraphicsItem *item)
{
    Q_D(QGraphicsSceneBspTreeIndex);
    d->addItem(item);
}

/*!
    Remove the \a item from the BSP index.
*/
void QGraphicsSceneBspTreeIndex::removeItem(QGraphicsItem *item)
{
    Q_D(QGraphicsSceneBspTreeIndex);
    d->removeItem(item);
}

/*!
    \internal
    Update the BSP when the \a item 's bounding rect has changed.
*/
void QGraphicsSceneBspTreeIndex::prepareBoundingRectChange(const QGraphicsItem *item)
{
    if (!item)
        return;

    if (item->d_ptr->index == -1 || item->d_ptr->itemIsUntransformable()
        || (item->d_ptr->ancestorFlags & QGraphicsItemPrivate::AncestorClipsChildren)) {
        return; // Item is not in BSP tree; nothing to do.
    }

    Q_D(QGraphicsSceneBspTreeIndex);
    QGraphicsItem *thatItem = const_cast<QGraphicsItem *>(item);
    d->removeItem(thatItem, /*recursive=*/false, /*moveToUnindexedItems=*/true);
    for (int i = 0; i < item->d_ptr->children.size(); ++i)  // ### Do we really need this?
        prepareBoundingRectChange(item->d_ptr->children.at(i));
}

/*!
    Returns an estimation visible items that are either inside or
    intersect with the specified \a rect and return a list sorted using \a order.

    \a deviceTransform is the transformation apply to the view.

*/
QList<QGraphicsItem *> QGraphicsSceneBspTreeIndex::estimateItems(const QRectF &rect, Qt::SortOrder order) const
{
    Q_D(const QGraphicsSceneBspTreeIndex);
    return const_cast<QGraphicsSceneBspTreeIndexPrivate*>(d)->estimateItems(rect, order);
}

QList<QGraphicsItem *> QGraphicsSceneBspTreeIndex::estimateTopLevelItems(const QRectF &rect, Qt::SortOrder order) const
{
    Q_D(const QGraphicsSceneBspTreeIndex);
    return const_cast<QGraphicsSceneBspTreeIndexPrivate*>(d)->estimateItems(rect, order, /*onlyTopLevels=*/true);
}

/*!
    \fn QList<QGraphicsItem *> QGraphicsSceneBspTreeIndex::items(Qt::SortOrder order = Qt::DescendingOrder) const;

    Return all items in the BSP index and sort them using \a order.
*/
QList<QGraphicsItem *> QGraphicsSceneBspTreeIndex::items(Qt::SortOrder order) const
{
    Q_D(const QGraphicsSceneBspTreeIndex);
    const_cast<QGraphicsSceneBspTreeIndexPrivate*>(d)->purgeRemovedItems();
    QList<QGraphicsItem *> itemList;

    // If freeItemIndexes is empty, we know there are no holes in indexedItems and
    // unindexedItems.
    if (d->freeItemIndexes.isEmpty()) {
        if (d->unindexedItems.isEmpty()) {
            itemList = d->indexedItems;
        } else {
            itemList = d->indexedItems + d->unindexedItems;
        }
    } else {
        // Rebuild the list of items to avoid holes. ### We could also just
        // compress the item lists at this point.
        foreach (QGraphicsItem *item, d->indexedItems + d->unindexedItems) {
            if (item)
                itemList << item;
        }
    }
    d->sortItems(&itemList, order, d->sortCacheEnabled);
    return itemList;
}

/*!
    \property QGraphicsSceneBspTreeIndex::bspTreeDepth
    \brief the depth of the BSP index tree
    \since 4.6

    This value determines the depth of BSP tree. The depth
    directly affects performance and memory usage; the latter
    growing exponentially with the depth of the tree. With an optimal tree
    depth, the index can instantly determine the locality of items, even
    for scenes with thousands or millions of items. This also greatly improves
    rendering performance.

    By default, the value is 0, in which case Qt will guess a reasonable
    default depth based on the size, location and number of items in the
    scene. If these parameters change frequently, however, you may experience
    slowdowns as the index retunes the depth internally. You can avoid
    potential slowdowns by fixating the tree depth through setting this
    property.

    The depth of the tree and the size of the scene rectangle decide the
    granularity of the scene's partitioning. The size of each scene segment is
    determined by the following algorithm:

    The BSP tree has an optimal size when each segment contains between 0 and
    10 items.

*/
int QGraphicsSceneBspTreeIndex::bspTreeDepth() const
{
    Q_D(const QGraphicsSceneBspTreeIndex);
    return d->bspTreeDepth;
}

void QGraphicsSceneBspTreeIndex::setBspTreeDepth(int depth)
{
    Q_D(QGraphicsSceneBspTreeIndex);
    if (d->bspTreeDepth == depth)
        return;
    d->bspTreeDepth = depth;
    d->resetIndex();
}

/*!
    \internal

    This method react to the  \a rect change of the scene and
    reset the BSP tree index.
*/
void QGraphicsSceneBspTreeIndex::updateSceneRect(const QRectF &rect)
{
    Q_D(QGraphicsSceneBspTreeIndex);
    d->sceneRect = rect;
    d->resetIndex();
}

/*!
    \internal

    This method react to the \a change of the \a item and use the \a value to
    update the BSP tree if necessary.
*/
void QGraphicsSceneBspTreeIndex::itemChange(const QGraphicsItem *item, QGraphicsItem::GraphicsItemChange change, const void *const value)
{
    Q_D(QGraphicsSceneBspTreeIndex);
    switch (change) {
    case QGraphicsItem::ItemFlagsChange: {
        // Handle ItemIgnoresTransformations
        QGraphicsItem::GraphicsItemFlags newFlags = *static_cast<const QGraphicsItem::GraphicsItemFlags *>(value);
        bool ignoredTransform = item->d_ptr->flags & QGraphicsItem::ItemIgnoresTransformations;
        bool willIgnoreTransform = newFlags & QGraphicsItem::ItemIgnoresTransformations;
        bool clipsChildren = item->d_ptr->flags & QGraphicsItem::ItemClipsChildrenToShape;
        bool willClipChildren = newFlags & QGraphicsItem::ItemClipsChildrenToShape;
        if ((ignoredTransform != willIgnoreTransform) || (clipsChildren != willClipChildren)) {
            QGraphicsItem *thatItem = const_cast<QGraphicsItem *>(item);
            // Remove item and its descendants from the index and append
            // them to the list of unindexed items. Then, when the index
            // is updated, they will be put into the bsp-tree or the list
            // of untransformable items.
            d->removeItem(thatItem, /*recursive=*/true, /*moveToUnidexedItems=*/true);
        }
        break;
    }
    case QGraphicsItem::ItemZValueChange:
        d->invalidateSortCache();
        break;
    case QGraphicsItem::ItemParentChange: {
        d->invalidateSortCache();
        // Handle ItemIgnoresTransformations
        const QGraphicsItem *newParent = static_cast<const QGraphicsItem *>(value);
        bool ignoredTransform = item->d_ptr->itemIsUntransformable();
        bool willIgnoreTransform = (item->d_ptr->flags & QGraphicsItem::ItemIgnoresTransformations)
                                   || (newParent && newParent->d_ptr->itemIsUntransformable());
        bool ancestorClippedChildren = item->d_ptr->ancestorFlags & QGraphicsItemPrivate::AncestorClipsChildren;
        bool ancestorWillClipChildren = newParent
                            && ((newParent->d_ptr->flags & QGraphicsItem::ItemClipsChildrenToShape)
                                || (newParent->d_ptr->ancestorFlags & QGraphicsItemPrivate::AncestorClipsChildren));
        if ((ignoredTransform != willIgnoreTransform) || (ancestorClippedChildren != ancestorWillClipChildren)) {
            QGraphicsItem *thatItem = const_cast<QGraphicsItem *>(item);
            // Remove item and its descendants from the index and append
            // them to the list of unindexed items. Then, when the index
            // is updated, they will be put into the bsp-tree or the list
            // of untransformable items.
            d->removeItem(thatItem, /*recursive=*/true, /*moveToUnidexedItems=*/true);
        }
        break;
    }
    default:
        break;
    }
}
/*!
    \reimp

    Used to catch the timer event.

    \internal
*/
bool QGraphicsSceneBspTreeIndex::event(QEvent *event)
{
    Q_D(QGraphicsSceneBspTreeIndex);
    switch (event->type()) {
    case QEvent::Timer:
            if (d->indexTimerId && static_cast<QTimerEvent *>(event)->timerId() == d->indexTimerId) {
                if (d->restartIndexTimer) {
                    d->restartIndexTimer = false;
                } else {
                    // this call will kill the timer
                    d->_q_updateIndex();
                }
            }
     // Fallthrough intended - support timers in subclasses.
    default:
        return QObject::event(event);
    }
    return true;
}

QT_END_NAMESPACE

#include "moc_qgraphicsscenebsptreeindex_p.cpp"

#endif  // QT_NO_GRAPHICSVIEW

