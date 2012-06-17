/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtCore module of the Qt Toolkit.
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

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of other Qt classes.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#ifndef QGRAPHICSBSPTREEINDEX_H
#define QGRAPHICSBSPTREEINDEX_H

#include <QtCore/qglobal.h>

#if !defined(QT_NO_GRAPHICSVIEW) || (QT_EDITION & QT_MODULE_GRAPHICSVIEW) != QT_MODULE_GRAPHICSVIEW

#include "qgraphicssceneindex_p.h"
#include "qgraphicsitem_p.h"
#include "qgraphicsscene_bsp_p.h"

#include <QtCore/qrect.h>
#include <QtCore/qlist.h>

QT_BEGIN_NAMESPACE

static const int QGRAPHICSSCENE_INDEXTIMER_TIMEOUT = 2000;

class QGraphicsScene;
class QGraphicsSceneBspTreeIndexPrivate;

class Q_AUTOTEST_EXPORT QGraphicsSceneBspTreeIndex : public QGraphicsSceneIndex
{
    Q_OBJECT
    Q_PROPERTY(int bspTreeDepth READ bspTreeDepth WRITE setBspTreeDepth)
public:
    QGraphicsSceneBspTreeIndex(QGraphicsScene *scene = 0);
    ~QGraphicsSceneBspTreeIndex();

    QList<QGraphicsItem *> estimateItems(const QRectF &rect, Qt::SortOrder order) const;
    QList<QGraphicsItem *> estimateTopLevelItems(const QRectF &rect, Qt::SortOrder order) const;
    QList<QGraphicsItem *> items(Qt::SortOrder order = Qt::DescendingOrder) const;

    int bspTreeDepth();
    void setBspTreeDepth(int depth);

protected Q_SLOTS:
    void updateSceneRect(const QRectF &rect);

protected:
    bool event(QEvent *event);
    void clear();

    void addItem(QGraphicsItem *item);
    void removeItem(QGraphicsItem *item);
    void prepareBoundingRectChange(const QGraphicsItem *item);

    void itemChange(const QGraphicsItem *item, QGraphicsItem::GraphicsItemChange change, const void *const value);

private :
    Q_DECLARE_PRIVATE(QGraphicsSceneBspTreeIndex)
    Q_DISABLE_COPY(QGraphicsSceneBspTreeIndex)
    Q_PRIVATE_SLOT(d_func(), void _q_updateSortCache())
    Q_PRIVATE_SLOT(d_func(), void _q_updateIndex())

    friend class QGraphicsScene;
    friend class QGraphicsScenePrivate;
};

class QGraphicsSceneBspTreeIndexPrivate : public QGraphicsSceneIndexPrivate
{
    Q_DECLARE_PUBLIC(QGraphicsSceneBspTreeIndex)
public:
    QGraphicsSceneBspTreeIndexPrivate(QGraphicsScene *scene);

    QGraphicsSceneBspTree bsp;
    QRectF sceneRect;
    int bspTreeDepth;
    int indexTimerId;
    bool restartIndexTimer;
    bool regenerateIndex;
    int lastItemCount;

    QList<QGraphicsItem *> indexedItems;
    QList<QGraphicsItem *> unindexedItems;
    QList<QGraphicsItem *> untransformableItems;
    QList<int> freeItemIndexes;

    bool purgePending;
    QSet<QGraphicsItem *> removedItems;
    void purgeRemovedItems();

    void _q_updateIndex();
    void startIndexTimer(int interval = QGRAPHICSSCENE_INDEXTIMER_TIMEOUT);
    void resetIndex();

    void _q_updateSortCache();
    bool sortCacheEnabled;
    bool updatingSortCache;
    void invalidateSortCache();
    void addItem(QGraphicsItem *item, bool recursive = false);
    void removeItem(QGraphicsItem *item, bool recursive = false, bool moveToUnindexedItems = false);
    QList<QGraphicsItem *> estimateItems(const QRectF &, Qt::SortOrder, bool b = false);

    static void climbTree(QGraphicsItem *item, int *stackingOrder);

    static inline bool closestItemFirst_withCache(const QGraphicsItem *item1, const QGraphicsItem *item2)
    {
        return item1->d_ptr->globalStackingOrder < item2->d_ptr->globalStackingOrder;
    }
    static inline bool closestItemLast_withCache(const QGraphicsItem *item1, const QGraphicsItem *item2)
    {
        return item1->d_ptr->globalStackingOrder >= item2->d_ptr->globalStackingOrder;
    }

    static void sortItems(QList<QGraphicsItem *> *itemList, Qt::SortOrder order,
                          bool cached, bool onlyTopLevelItems = false);
};

static inline bool QRectF_intersects(const QRectF &s, const QRectF &r)
{
    qreal xp = s.left();
    qreal yp = s.top();
    qreal w = s.width();
    qreal h = s.height();
    qreal l1 = xp;
    qreal r1 = xp;
    if (w < 0)
        l1 += w;
    else
        r1 += w;

    qreal l2 = r.left();
    qreal r2 = r.left();
    if (w < 0)
        l2 += r.width();
    else
        r2 += r.width();

    if (l1 >= r2 || l2 >= r1)
        return false;

    qreal t1 = yp;
    qreal b1 = yp;
    if (h < 0)
        t1 += h;
    else
        b1 += h;

    qreal t2 = r.top();
    qreal b2 = r.top();
    if (r.height() < 0)
        t2 += r.height();
    else
        b2 += r.height();

    return !(t1 >= b2 || t2 >= b1);
}

QT_END_NAMESPACE

#endif // QT_NO_GRAPHICSVIEW

#endif // QGRAPHICSBSPTREEINDEX_H
