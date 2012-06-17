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

#ifndef QSTANDARDITEMMODEL_P_H
#define QSTANDARDITEMMODEL_P_H

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

#include "private/qabstractitemmodel_p.h"

#ifndef QT_NO_STANDARDITEMMODEL

#include <private/qwidgetitemdata_p.h>
#include <QtCore/qlist.h>
#include <QtCore/qpair.h>
#include <QtCore/qstack.h>
#include <QtCore/qvariant.h>
#include <QtCore/qvector.h>

QT_BEGIN_NAMESPACE

class QStandardItemPrivate
{
    Q_DECLARE_PUBLIC(QStandardItem)
public:
    inline QStandardItemPrivate()
        : model(0),
          parent(0),
          rows(0),
          columns(0),
          q_ptr(0),
          lastIndexOf(2)
        { }
    virtual ~QStandardItemPrivate();

    inline int childIndex(int row, int column) const {
        if ((row < 0) || (column < 0)
            || (row >= rowCount()) || (column >= columnCount())) {
            return -1;
        }
        return (row * columnCount()) + column;
    }
    inline int childIndex(const QStandardItem *child) {
        int start = qMax(0, lastIndexOf -2);
        lastIndexOf = children.indexOf(const_cast<QStandardItem*>(child), start);
        if (lastIndexOf == -1 && start != 0)
            lastIndexOf = children.lastIndexOf(const_cast<QStandardItem*>(child), start);
        return lastIndexOf;
    }
    QPair<int, int> position() const;
    void setChild(int row, int column, QStandardItem *item,
                  bool emitChanged = false);
    inline int rowCount() const {
        return rows;
    }
    inline int columnCount() const {
        return columns;
    }
    void childDeleted(QStandardItem *child);

    void setModel(QStandardItemModel *mod);

    inline void setParentAndModel(
        QStandardItem *par,
        QStandardItemModel *mod) {
        setModel(mod);
        parent = par;
    }

    void changeFlags(bool enable, Qt::ItemFlags f);
    void setItemData(const QMap<int, QVariant> &roles);
    const QMap<int, QVariant> itemData() const;

    bool insertRows(int row, int count, const QList<QStandardItem*> &items);
    bool insertRows(int row, const QList<QStandardItem*> &items);
    bool insertColumns(int column, int count, const QList<QStandardItem*> &items);

    void sortChildren(int column, Qt::SortOrder order);

    QStandardItemModel *model;
    QStandardItem *parent;
    QVector<QWidgetItemData> values;
    QVector<QStandardItem*> children;
    int rows;
    int columns;

    QStandardItem *q_ptr;

    int lastIndexOf;
};

class QStandardItemModelPrivate : public QAbstractItemModelPrivate
{
    Q_DECLARE_PUBLIC(QStandardItemModel)

public:
    QStandardItemModelPrivate();
    virtual ~QStandardItemModelPrivate();

    void init();

    inline QStandardItem *createItem() const {
        return itemPrototype ? itemPrototype->clone() : new QStandardItem;
    }

    inline QStandardItem *itemFromIndex(const QModelIndex &index) const {
        Q_Q(const QStandardItemModel);
        if (!index.isValid())
            return root.data();
        if (index.model() != q)
            return 0;
        QStandardItem *parent = static_cast<QStandardItem*>(index.internalPointer());
        if (parent == 0)
            return 0;
        return parent->child(index.row(), index.column());
    }

    void sort(QStandardItem *parent, int column, Qt::SortOrder order);
    void itemChanged(QStandardItem *item);
    void rowsAboutToBeInserted(QStandardItem *parent, int start, int end);
    void columnsAboutToBeInserted(QStandardItem *parent, int start, int end);
    void rowsAboutToBeRemoved(QStandardItem *parent, int start, int end);
    void columnsAboutToBeRemoved(QStandardItem *parent, int start, int end);
    void rowsInserted(QStandardItem *parent, int row, int count);
    void columnsInserted(QStandardItem *parent, int column, int count);
    void rowsRemoved(QStandardItem *parent, int row, int count);
    void columnsRemoved(QStandardItem *parent, int column, int count);

    void _q_emitItemChanged(const QModelIndex &topLeft,
                            const QModelIndex &bottomRight);

    void decodeDataRecursive(QDataStream &stream, QStandardItem *item);

    QVector<QStandardItem*> columnHeaderItems;
    QVector<QStandardItem*> rowHeaderItems;
    QScopedPointer<QStandardItem> root;
    const QStandardItem *itemPrototype;
    int sortRole;
};

QT_END_NAMESPACE

#endif // QT_NO_STANDARDITEMMODEL

#endif // QSTANDARDITEMMODEL_P_H
