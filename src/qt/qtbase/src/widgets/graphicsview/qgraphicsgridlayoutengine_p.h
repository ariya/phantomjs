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

#ifndef QGRAPHICSGRIDLAYOUTENGINE_P_H
#define QGRAPHICSGRIDLAYOUTENGINE_P_H

#include <QtGui/private/qgridlayoutengine_p.h>

#ifndef QT_NO_GRAPHICSVIEW

#include <QtWidgets/qsizepolicy.h>
#include <QtWidgets/qstyle.h>
#include <QtWidgets/qstyleoption.h>
#include "qgraphicslayoutitem.h"

QT_BEGIN_NAMESPACE

class QGraphicsLayoutPrivate;

class QGraphicsGridLayoutEngineItem : public QGridLayoutItem {
public:
    QGraphicsGridLayoutEngineItem(QGraphicsLayoutItem *item, int row, int columns, int rowSpan = 1, int columnSpan = 1,
                            Qt::Alignment alignment = 0)
        : QGridLayoutItem(row, columns, rowSpan, columnSpan, alignment), q_layoutItem(item) {}

    virtual QLayoutPolicy::Policy sizePolicy(Qt::Orientation orientation) const Q_DECL_OVERRIDE
    {
        QSizePolicy sizePolicy(q_layoutItem->sizePolicy());
        return (QLayoutPolicy::Policy)((orientation == Qt::Horizontal) ? sizePolicy.horizontalPolicy()
                                               : sizePolicy.verticalPolicy());
    }

    virtual QLayoutPolicy::ControlTypes controlTypes(LayoutSide) const Q_DECL_OVERRIDE
    {
        const QSizePolicy::ControlType ct = q_layoutItem->sizePolicy().controlType();
        return (QLayoutPolicy::ControlTypes)ct;
    }

    virtual QSizeF sizeHint(Qt::SizeHint which, const QSizeF &constraint) const Q_DECL_OVERRIDE
    {
        return q_layoutItem->effectiveSizeHint(which, constraint);
    }

    virtual void setGeometry(const QRectF &rect) Q_DECL_OVERRIDE
    {
         q_layoutItem->setGeometry(rect);
    }

    virtual bool hasDynamicConstraint() const Q_DECL_OVERRIDE;
    virtual Qt::Orientation dynamicConstraintOrientation() const Q_DECL_OVERRIDE;

    QGraphicsLayoutItem *layoutItem() const { return q_layoutItem; }

protected:
    QGraphicsLayoutItem *q_layoutItem;
};


class QGraphicsGridLayoutEngine : public QGridLayoutEngine
{
public:
    QGraphicsGridLayoutEngineItem *findLayoutItem(QGraphicsLayoutItem *layoutItem) const
    {
        const int index = indexOf(layoutItem);
        if (index < 0)
            return 0;
        return static_cast<QGraphicsGridLayoutEngineItem*>(q_items.at(index));
    }

    int indexOf(QGraphicsLayoutItem *item) const
    {
        for (int i = 0; i < q_items.count(); ++i) {
            if (item == static_cast<QGraphicsGridLayoutEngineItem*>(q_items.at(i))->layoutItem())
                return i;
        }
        return -1;
    }

    void setAlignment(QGraphicsLayoutItem *graphicsLayoutItem, Qt::Alignment alignment);
    Qt::Alignment alignment(QGraphicsLayoutItem *graphicsLayoutItem) const;

    void setStretchFactor(QGraphicsLayoutItem *layoutItem, int stretch, Qt::Orientation orientation);
    int stretchFactor(QGraphicsLayoutItem *layoutItem, Qt::Orientation orientation) const;

};

QT_END_NAMESPACE

#endif // QT_NO_GRAPHICSVIEW

#endif // QGRAPHICSGRIDLAYOUTENGINE_P_H
