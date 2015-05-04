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

#ifndef QLISTWIDGET_P_H
#define QLISTWIDGET_P_H

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
#include <QtWidgets/qabstractitemview.h>
#include <QtWidgets/qlistwidget.h>
#include <qitemdelegate.h>
#include <private/qlistview_p.h>
#include <private/qwidgetitemdata_p.h>

#ifndef QT_NO_LISTWIDGET

QT_BEGIN_NAMESPACE

class QListModelLessThan
{
public:
    inline bool operator()(QListWidgetItem *i1, QListWidgetItem *i2) const
        { return *i1 < *i2; }
};

class QListModelGreaterThan
{
public:
    inline bool operator()(QListWidgetItem *i1, QListWidgetItem *i2) const
        { return *i2 < *i1; }
};

class Q_AUTOTEST_EXPORT QListModel : public QAbstractListModel
{
    Q_OBJECT
public:
    QListModel(QListWidget *parent);
    ~QListModel();

    void clear();
    QListWidgetItem *at(int row) const;
    void insert(int row, QListWidgetItem *item);
    void insert(int row, const QStringList &items);
    void remove(QListWidgetItem *item);
    QListWidgetItem *take(int row);
    void move(int srcRow, int dstRow);

    int rowCount(const QModelIndex &parent = QModelIndex()) const;

    QModelIndex index(QListWidgetItem *item) const;
    QModelIndex index(int row, int column = 0, const QModelIndex &parent = QModelIndex()) const;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    bool setData(const QModelIndex &index, const QVariant &value, int role);

    QMap<int, QVariant> itemData(const QModelIndex &index) const;

    bool insertRows(int row, int count = 1, const QModelIndex &parent = QModelIndex());
    bool removeRows(int row, int count = 1, const QModelIndex &parent = QModelIndex());

    Qt::ItemFlags flags(const QModelIndex &index) const;

    void sort(int column, Qt::SortOrder order);
    void ensureSorted(int column, Qt::SortOrder order, int start, int end);
    static bool itemLessThan(const QPair<QListWidgetItem*,int> &left,
                             const QPair<QListWidgetItem*,int> &right);
    static bool itemGreaterThan(const QPair<QListWidgetItem*,int> &left,
                                const QPair<QListWidgetItem*,int> &right);
    static QList<QListWidgetItem*>::iterator sortedInsertionIterator(
        const QList<QListWidgetItem*>::iterator &begin,
        const QList<QListWidgetItem*>::iterator &end,
        Qt::SortOrder order, QListWidgetItem *item);

    void itemChanged(QListWidgetItem *item);

    // dnd
    QStringList mimeTypes() const;
    QMimeData *mimeData(const QModelIndexList &indexes) const;
#ifndef QT_NO_DRAGANDDROP
    bool dropMimeData(const QMimeData *data, Qt::DropAction action,
                      int row, int column, const QModelIndex &parent);
    Qt::DropActions supportedDropActions() const;
#endif

    QMimeData *internalMimeData()  const;
private:
    QList<QListWidgetItem*> items;

    // A cache must be mutable if get-functions should have const modifiers
    mutable QModelIndexList cachedIndexes;
};



class QListWidgetPrivate : public QListViewPrivate
{
    Q_DECLARE_PUBLIC(QListWidget)
public:
    QListWidgetPrivate() : QListViewPrivate(), sortOrder(Qt::AscendingOrder), sortingEnabled(false) {}
    inline QListModel *listModel() const { return qobject_cast<QListModel*>(model); }
    void setup();
    void _q_emitItemPressed(const QModelIndex &index);
    void _q_emitItemClicked(const QModelIndex &index);
    void _q_emitItemDoubleClicked(const QModelIndex &index);
    void _q_emitItemActivated(const QModelIndex &index);
    void _q_emitItemEntered(const QModelIndex &index);
    void _q_emitItemChanged(const QModelIndex &index);
    void _q_emitCurrentItemChanged(const QModelIndex &current, const QModelIndex &previous);
    void _q_sort();
    void _q_dataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight);
    Qt::SortOrder sortOrder;
    bool sortingEnabled;
};

class QListWidgetItemPrivate
{
public:
    QListWidgetItemPrivate(QListWidgetItem *item) : q(item), theid(-1) {}
    QListWidgetItem *q;
    QVector<QWidgetItemData> values;
    int theid;
};

QT_END_NAMESPACE

#endif // QT_NO_LISTWIDGET

#endif // QLISTWIDGET_P_H
