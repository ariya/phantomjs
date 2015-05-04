/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtWidgets module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia. For licensing terms and
** conditions see http://qt.digia.com/licensing. For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights. These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QTREEWIDGET_P_H
#define QTREEWIDGET_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API. This header file may change
// from version to version without notice, or even be removed.
//
// We mean it.
//

#include <QtCore/qabstractitemmodel.h>
#include <private/qabstractitemmodel_p.h>
#include <QtCore/qpair.h>
#include <QtCore/qbasictimer.h>
#include <QtWidgets/qtreewidget.h>
#include <private/qtreeview_p.h>
#include <QtWidgets/qheaderview.h>

#ifndef QT_NO_TREEWIDGET

QT_BEGIN_NAMESPACE

class QTreeWidgetItem;
class QTreeWidgetItemIterator;
class QTreeModelPrivate;

class QTreeModel : public QAbstractItemModel
{
    Q_OBJECT
    friend class QTreeWidget;
    friend class QTreeWidgetPrivate;
    friend class QTreeWidgetItem;
    friend class QTreeWidgetItemPrivate;
    friend class QTreeWidgetItemIterator;
    friend class QTreeWidgetItemIteratorPrivate;

public:
    explicit QTreeModel(int columns = 0, QTreeWidget *parent = 0);
    ~QTreeModel();

    inline QTreeWidget *view() const
        { return qobject_cast<QTreeWidget*>(QObject::parent()); }

    void clear();
    void setColumnCount(int columns);

    QTreeWidgetItem *item(const QModelIndex &index) const;
    void itemChanged(QTreeWidgetItem *item);

    QModelIndex index(const QTreeWidgetItem *item, int column) const;
    QModelIndex index(int row, int column, const QModelIndex &parent) const;
    QModelIndex parent(const QModelIndex &child) const;
    int rowCount(const QModelIndex &parent) const;
    int columnCount(const QModelIndex &parent = QModelIndex()) const;
    bool hasChildren(const QModelIndex &parent) const;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    bool setData(const QModelIndex &index, const QVariant &value, int role);

    QMap<int, QVariant> itemData(const QModelIndex &index) const;

    QVariant headerData(int section, Qt::Orientation orientation, int role) const;
    bool setHeaderData(int section, Qt::Orientation orientation, const QVariant &value,
                       int role);

    Qt::ItemFlags flags(const QModelIndex &index) const;

    void sort(int column, Qt::SortOrder order);
    void ensureSorted(int column, Qt::SortOrder order,
                      int start, int end, const QModelIndex &parent);
    static bool itemLessThan(const QPair<QTreeWidgetItem*,int> &left,
                             const QPair<QTreeWidgetItem*,int> &right);
    static bool itemGreaterThan(const QPair<QTreeWidgetItem*,int> &left,
                                const QPair<QTreeWidgetItem*,int> &right);
    static QList<QTreeWidgetItem*>::iterator sortedInsertionIterator(
        const QList<QTreeWidgetItem*>::iterator &begin,
        const QList<QTreeWidgetItem*>::iterator &end,
        Qt::SortOrder order, QTreeWidgetItem *item);

    bool insertRows(int row, int count, const QModelIndex &);
    bool insertColumns(int column, int count, const QModelIndex &);

    bool removeRows(int row, int count, const QModelIndex &parent = QModelIndex());

    // dnd
    QStringList mimeTypes() const;
    QMimeData *mimeData(const QModelIndexList &indexes) const;
    bool dropMimeData(const QMimeData *data, Qt::DropAction action,
                      int row, int column, const QModelIndex &parent);
    Qt::DropActions supportedDropActions() const;

    QMimeData *internalMimeData() const;

    inline QModelIndex createIndexFromItem(int row, int col, QTreeWidgetItem *item) const
    { return createIndex(row, col, item); }

protected:
    QTreeModel(QTreeModelPrivate &, QTreeWidget *parent = 0);
    void emitDataChanged(QTreeWidgetItem *item, int column);
    void beginInsertItems(QTreeWidgetItem *parent, int row, int count);
    void endInsertItems();
    void beginRemoveItems(QTreeWidgetItem *parent, int row, int count);
    void endRemoveItems();
    void sortItems(QList<QTreeWidgetItem*> *items, int column, Qt::SortOrder order);
    void timerEvent(QTimerEvent *);

private:
    QTreeWidgetItem *rootItem;
    QTreeWidgetItem *headerItem;

    mutable QModelIndexList cachedIndexes;
    QList<QTreeWidgetItemIterator*> iterators;

    mutable QBasicTimer sortPendingTimer;
    mutable bool skipPendingSort; //while doing internal operation we don't care about sorting
    bool inline executePendingSort() const;

    bool isChanging() const;

private:
    Q_DECLARE_PRIVATE(QTreeModel)
public:
    struct SkipSorting
    {
        const QTreeModel * const model;
        const bool previous;
        SkipSorting(const QTreeModel *m) : model(m), previous(model->skipPendingSort)
        { model->skipPendingSort = true; }
        ~SkipSorting() { model->skipPendingSort = previous; }
    };
    friend struct SkipSorting;
};

QT_BEGIN_INCLUDE_NAMESPACE
#include "private/qabstractitemmodel_p.h"
QT_END_INCLUDE_NAMESPACE

class QTreeModelPrivate : public QAbstractItemModelPrivate
{
    Q_DECLARE_PUBLIC(QTreeModel)
};

class QTreeWidgetItemPrivate
{
public:
    QTreeWidgetItemPrivate(QTreeWidgetItem *item)
        : q(item), disabled(false), selected(false), rowGuess(-1), policy(QTreeWidgetItem::DontShowIndicatorWhenChildless) {}
    void propagateDisabled(QTreeWidgetItem *item);
    void sortChildren(int column, Qt::SortOrder order, bool climb);
    QTreeWidgetItem *q;
    QVariantList display;
    uint disabled : 1;
    uint selected : 1;
    int rowGuess;
    QTreeWidgetItem::ChildIndicatorPolicy policy;
};


inline bool QTreeModel::executePendingSort() const
{
    if (!skipPendingSort && sortPendingTimer.isActive() && !isChanging()) {
        sortPendingTimer.stop();
        int column = view()->header()->sortIndicatorSection();
        Qt::SortOrder order = view()->header()->sortIndicatorOrder();
        QTreeModel *that = const_cast<QTreeModel*>(this);
        that->sort(column, order);
        return true;
    }
    return false;
}

class QTreeWidgetPrivate : public QTreeViewPrivate
{
    friend class QTreeModel;
    Q_DECLARE_PUBLIC(QTreeWidget)
public:
    QTreeWidgetPrivate() : QTreeViewPrivate(), explicitSortColumn(-1) {}
    inline QTreeModel *treeModel() const { return qobject_cast<QTreeModel*>(model); }
    inline QModelIndex index(const QTreeWidgetItem *item, int column = 0) const
        { return treeModel()->index(item, column); }
    inline QTreeWidgetItem *item(const QModelIndex &index) const
        { return treeModel()->item(index); }
    void _q_emitItemPressed(const QModelIndex &index);
    void _q_emitItemClicked(const QModelIndex &index);
    void _q_emitItemDoubleClicked(const QModelIndex &index);
    void _q_emitItemActivated(const QModelIndex &index);
    void _q_emitItemEntered(const QModelIndex &index);
    void _q_emitItemChanged(const QModelIndex &index);
    void _q_emitItemExpanded(const QModelIndex &index);
    void _q_emitItemCollapsed(const QModelIndex &index);
    void _q_emitCurrentItemChanged(const QModelIndex &previous, const QModelIndex &index);
    void _q_sort();
    void _q_dataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight);
    void _q_selectionChanged(const QItemSelection &selected, const QItemSelection &deselected);

     // used by QTreeWidgetItem::sortChildren to make sure the column argument is used
    int explicitSortColumn;
};

QT_END_NAMESPACE

#endif // QT_NO_TREEWIDGET

#endif // QTREEWIDGET_P_H
