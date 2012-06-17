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

#ifndef QGRAPHICSSCENEINDEX_H
#define QGRAPHICSSCENEINDEX_H

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

#include "qgraphicsscene_p.h"
#include "qgraphicsscene.h"
#include <private/qobject_p.h>

#include <QtCore/qnamespace.h>
#include <QtCore/qobject.h>
#include <QtGui/qtransform.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Gui)

#if !defined(QT_NO_GRAPHICSVIEW) || (QT_EDITION & QT_MODULE_GRAPHICSVIEW) != QT_MODULE_GRAPHICSVIEW

class QGraphicsSceneIndexIntersector;
class QGraphicsSceneIndexPointIntersector;
class QGraphicsSceneIndexRectIntersector;
class QGraphicsSceneIndexPathIntersector;
class QGraphicsSceneIndexPrivate;
class QPointF;
class QRectF;
template<typename T> class QList;

class Q_AUTOTEST_EXPORT QGraphicsSceneIndex : public QObject
{
    Q_OBJECT

public:
    QGraphicsSceneIndex(QGraphicsScene *scene = 0);
    virtual ~QGraphicsSceneIndex();

    QGraphicsScene *scene() const;

    virtual QList<QGraphicsItem *> items(Qt::SortOrder order = Qt::DescendingOrder) const  = 0;
    virtual QList<QGraphicsItem *> items(const QPointF &pos, Qt::ItemSelectionMode mode,
                                         Qt::SortOrder order, const QTransform &deviceTransform = QTransform()) const;
    virtual QList<QGraphicsItem *> items(const QRectF &rect, Qt::ItemSelectionMode mode,
                                         Qt::SortOrder order, const QTransform &deviceTransform = QTransform()) const;
    virtual QList<QGraphicsItem *> items(const QPolygonF &polygon, Qt::ItemSelectionMode mode,
                                         Qt::SortOrder order, const QTransform &deviceTransform = QTransform()) const;
    virtual QList<QGraphicsItem *> items(const QPainterPath &path, Qt::ItemSelectionMode mode,
                                         Qt::SortOrder order, const QTransform &deviceTransform = QTransform()) const;
    virtual QList<QGraphicsItem *> estimateItems(const QPointF &point, Qt::SortOrder order) const;
    virtual QList<QGraphicsItem *> estimateItems(const QRectF &rect, Qt::SortOrder order) const = 0;
    virtual QList<QGraphicsItem *> estimateTopLevelItems(const QRectF &, Qt::SortOrder order) const;

protected Q_SLOTS:
    virtual void updateSceneRect(const QRectF &rect);

protected:
    virtual void clear();
    virtual void addItem(QGraphicsItem *item) = 0;
    virtual void removeItem(QGraphicsItem *item) = 0;
    virtual void deleteItem(QGraphicsItem *item);

    virtual void itemChange(const QGraphicsItem *item, QGraphicsItem::GraphicsItemChange, const void *const value);
    virtual void prepareBoundingRectChange(const QGraphicsItem *item);

    QGraphicsSceneIndex(QGraphicsSceneIndexPrivate &dd, QGraphicsScene *scene);

    friend class QGraphicsScene;
    friend class QGraphicsScenePrivate;
    friend class QGraphicsItem;
    friend class QGraphicsItemPrivate;
    friend class QGraphicsSceneBspTreeIndex;
private:
    Q_DISABLE_COPY(QGraphicsSceneIndex)
    Q_DECLARE_PRIVATE(QGraphicsSceneIndex)
};

class QGraphicsSceneIndexPrivate : public QObjectPrivate
{
    Q_DECLARE_PUBLIC(QGraphicsSceneIndex)
public:
    QGraphicsSceneIndexPrivate(QGraphicsScene *scene);
    ~QGraphicsSceneIndexPrivate();

    void init();
    static bool itemCollidesWithPath(const QGraphicsItem *item, const QPainterPath &path, Qt::ItemSelectionMode mode);

    void recursive_items_helper(QGraphicsItem *item, QRectF exposeRect,
                                QGraphicsSceneIndexIntersector *intersector, QList<QGraphicsItem *> *items,
                                const QTransform &viewTransform,
                                Qt::ItemSelectionMode mode, qreal parentOpacity = 1.0) const;
    inline void items_helper(const QRectF &rect, QGraphicsSceneIndexIntersector *intersector,
                             QList<QGraphicsItem *> *items, const QTransform &viewTransform,
                             Qt::ItemSelectionMode mode, Qt::SortOrder order) const;

    QGraphicsScene *scene;
    QGraphicsSceneIndexPointIntersector *pointIntersector;
    QGraphicsSceneIndexRectIntersector *rectIntersector;
    QGraphicsSceneIndexPathIntersector *pathIntersector;
};

inline void QGraphicsSceneIndexPrivate::items_helper(const QRectF &rect, QGraphicsSceneIndexIntersector *intersector,
                                                     QList<QGraphicsItem *> *items, const QTransform &viewTransform,
                                                     Qt::ItemSelectionMode mode, Qt::SortOrder order) const
{
    Q_Q(const QGraphicsSceneIndex);
    const QList<QGraphicsItem *> tli = q->estimateTopLevelItems(rect, Qt::AscendingOrder);
    for (int i = 0; i < tli.size(); ++i)
        recursive_items_helper(tli.at(i), rect, intersector, items, viewTransform, mode);
    if (order == Qt::DescendingOrder) {
        const int n = items->size();
        for (int i = 0; i < n / 2; ++i)
            items->swap(i, n - i - 1);
    }
}

class QGraphicsSceneIndexIntersector
{
public:
    QGraphicsSceneIndexIntersector() { }
    virtual ~QGraphicsSceneIndexIntersector() { }
    virtual bool intersect(const QGraphicsItem *item, const QRectF &exposeRect, Qt::ItemSelectionMode mode,
                           const QTransform &deviceTransform) const = 0;
};

#endif // QT_NO_GRAPHICSVIEW

QT_END_NAMESPACE

QT_END_HEADER

#endif // QGRAPHICSSCENEINDEX_H
