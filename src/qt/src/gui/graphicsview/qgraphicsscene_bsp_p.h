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

#ifndef QGRAPHICSSCENEBSPTREE_P_H
#define QGRAPHICSSCENEBSPTREE_P_H

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

#include <QtCore/qlist.h>

#if !defined(QT_NO_GRAPHICSVIEW) || (QT_EDITION & QT_MODULE_GRAPHICSVIEW) != QT_MODULE_GRAPHICSVIEW

#include <QtCore/qrect.h>
#include <QtCore/qset.h>
#include <QtCore/qvector.h>

QT_BEGIN_NAMESPACE

class QGraphicsItem;
class QGraphicsSceneBspTreeVisitor;
class QGraphicsSceneInsertItemBspTreeVisitor;
class QGraphicsSceneRemoveItemBspTreeVisitor;
class QGraphicsSceneFindItemBspTreeVisitor;

class QGraphicsSceneBspTree
{
public:
    struct Node
    {
        enum Type { Horizontal, Vertical, Leaf };
        union {
            qreal offset;
            int leafIndex;
        };
        Type type;
    };

    QGraphicsSceneBspTree();
    ~QGraphicsSceneBspTree();

    void initialize(const QRectF &rect, int depth);
    void clear();

    void insertItem(QGraphicsItem *item, const QRectF &rect);
    void removeItem(QGraphicsItem *item, const QRectF &rect);
    void removeItems(const QSet<QGraphicsItem *> &items);

    QList<QGraphicsItem *> items(const QRectF &rect, bool onlyTopLevelItems = false) const;
    int leafCount() const;

    inline int firstChildIndex(int index) const
    { return index * 2 + 1; }

    inline int parentIndex(int index) const
    { return index > 0 ? ((index & 1) ? ((index - 1) / 2) : ((index - 2) / 2)) : -1; }

    QString debug(int index) const;

private:
    void initialize(const QRectF &rect, int depth, int index);
    void climbTree(QGraphicsSceneBspTreeVisitor *visitor, const QRectF &rect, int index = 0) const;
    QRectF rectForIndex(int index) const;

    QVector<Node> nodes;
    QVector<QList<QGraphicsItem *> > leaves;
    int leafCnt;
    QRectF rect;

    QGraphicsSceneInsertItemBspTreeVisitor *insertVisitor;
    QGraphicsSceneRemoveItemBspTreeVisitor *removeVisitor;
    QGraphicsSceneFindItemBspTreeVisitor *findVisitor;
};

class QGraphicsSceneBspTreeVisitor
{
public:
    virtual ~QGraphicsSceneBspTreeVisitor() { }
    virtual void visit(QList<QGraphicsItem *> *items) = 0;
};

QT_END_NAMESPACE

#endif // QT_NO_GRAPHICSVIEW

#endif // QGRAPHICSSCENEBSPTREE_P_H
