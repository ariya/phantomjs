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

#ifndef QGRAPHICSSCENELINEARINDEX_H
#define QGRAPHICSSCENELINEARINDEX_H

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

#include <QtCore/qglobal.h>

#if !defined(QT_NO_GRAPHICSVIEW) || (QT_EDITION & QT_MODULE_GRAPHICSVIEW) != QT_MODULE_GRAPHICSVIEW

#include <QtCore/qrect.h>
#include <QtCore/qlist.h>
#include <QtGui/qgraphicsitem.h>
#include <private/qgraphicssceneindex_p.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Gui)

class Q_AUTOTEST_EXPORT QGraphicsSceneLinearIndex : public QGraphicsSceneIndex
{
    Q_OBJECT

public:
    QGraphicsSceneLinearIndex(QGraphicsScene *scene = 0) : QGraphicsSceneIndex(scene)
    { }

    QList<QGraphicsItem *> items(Qt::SortOrder order = Qt::DescendingOrder) const
    { Q_UNUSED(order); return m_items; }

    virtual QList<QGraphicsItem *> estimateItems(const QRectF &rect, Qt::SortOrder order) const
    {
        Q_UNUSED(rect);
        Q_UNUSED(order);
        return m_items;
    }

protected :
    virtual void clear()
    { m_items.clear(); }

    virtual void addItem(QGraphicsItem *item)
    { m_items << item; }

    virtual void removeItem(QGraphicsItem *item)
    { m_items.removeOne(item); }

private:
    QList<QGraphicsItem*> m_items;
};

#endif // QT_NO_GRAPHICSVIEW

QT_END_NAMESPACE

QT_END_HEADER

#endif // QGRAPHICSSCENELINEARINDEX_H
