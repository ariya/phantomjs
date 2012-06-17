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

#include "qstandarditemmodel.h"

#ifndef QT_NO_STANDARDITEMMODEL

#include <QtCore/qdatetime.h>
#include <QtCore/qlist.h>
#include <QtCore/qmap.h>
#include <QtCore/qpair.h>
#include <QtCore/qvariant.h>
#include <QtCore/qvector.h>
#include <QtCore/qstringlist.h>
#include <QtCore/qbitarray.h>
#include <QtCore/qmimedata.h>

#include <private/qstandarditemmodel_p.h>
#include <qdebug.h>

QT_BEGIN_NAMESPACE

class QStandardItemModelLessThan
{
public:
    inline QStandardItemModelLessThan()
        { }

    inline bool operator()(const QPair<QStandardItem*, int> &l,
                           const QPair<QStandardItem*, int> &r) const
    {
        return *(l.first) < *(r.first);
    }
};

class QStandardItemModelGreaterThan
{
public:
    inline QStandardItemModelGreaterThan()
        { }

    inline bool operator()(const QPair<QStandardItem*, int> &l,
                           const QPair<QStandardItem*, int> &r) const
    {
        return *(r.first) < *(l.first);
    }
};

/*!
  \internal
*/
QStandardItemPrivate::~QStandardItemPrivate()
{
    QVector<QStandardItem*>::const_iterator it;
    for (it = children.constBegin(); it != children.constEnd(); ++it) {
        QStandardItem *child = *it;
        if (child)
            child->d_func()->setModel(0);
        delete child;
    }
    children.clear();
    if (parent && model)
        parent->d_func()->childDeleted(q_func());
}

/*!
  \internal
*/
QPair<int, int> QStandardItemPrivate::position() const
{
    if (QStandardItem *par = parent) {
        int idx = par->d_func()->childIndex(q_func());
        if (idx == -1)
            return QPair<int, int>(-1, -1);
        return QPair<int, int>(idx / par->columnCount(), idx % par->columnCount());
    }
    // ### support header items?
    return QPair<int, int>(-1, -1);
}

/*!
  \internal
*/
void QStandardItemPrivate::setChild(int row, int column, QStandardItem *item,
                                    bool emitChanged)
{
    Q_Q(QStandardItem);
    if (item == q) {
        qWarning("QStandardItem::setChild: Can't make an item a child of itself %p",
                 item);
        return;
    }
    if ((row < 0) || (column < 0))
        return;
    if (rows <= row)
        q->setRowCount(row + 1);
    if (columns <= column)
        q->setColumnCount(column + 1);
    int index = childIndex(row, column);
    Q_ASSERT(index != -1);
    QStandardItem *oldItem = children.at(index);
    if (item == oldItem)
        return;
    if (item) {
        if (item->d_func()->parent == 0) {
            item->d_func()->setParentAndModel(q, model);
        } else {
            qWarning("QStandardItem::setChild: Ignoring duplicate insertion of item %p",
                     item);
            return;
        }
    }
    if (oldItem)
        oldItem->d_func()->setModel(0);
    delete oldItem;
    children.replace(index, item);
    if (emitChanged && model)
        model->d_func()->itemChanged(item);
}


/*!
  \internal
*/
void QStandardItemPrivate::changeFlags(bool enable, Qt::ItemFlags f)
{
    Q_Q(QStandardItem);
    Qt::ItemFlags flags = q->flags();
    if (enable)
        flags |= f;
    else
        flags &= ~f;
    q->setFlags(flags);
}

/*!
  \internal
*/
void QStandardItemPrivate::childDeleted(QStandardItem *child)
{
    int index = childIndex(child);
    Q_ASSERT(index != -1);
    children.replace(index, 0);
}

/*!
  \internal
*/
void QStandardItemPrivate::setItemData(const QMap<int, QVariant> &roles)
{
    Q_Q(QStandardItem);

    //let's build the vector of new values
    QVector<QWidgetItemData> newValues;
    QMap<int, QVariant>::const_iterator it;
    for (it = roles.begin(); it != roles.end(); ++it) {
        QVariant value = it.value();
        if (value.isValid()) {
            int role = it.key();
            role = (role == Qt::EditRole) ? Qt::DisplayRole : role;
            QWidgetItemData wid(role,it.value());
            newValues.append(wid);
        }
    }

    if (values!=newValues) {
        values=newValues;
        if (model)
            model->d_func()->itemChanged(q);
    }
}

/*!
  \internal
*/
const QMap<int, QVariant> QStandardItemPrivate::itemData() const
{
    QMap<int, QVariant> result;
    QVector<QWidgetItemData>::const_iterator it;
    for (it = values.begin(); it != values.end(); ++it)
        result.insert((*it).role, (*it).value);
    return result;
}

/*!
  \internal
*/
void QStandardItemPrivate::sortChildren(int column, Qt::SortOrder order)
{
    Q_Q(QStandardItem);
    if (column >= columnCount())
        return;

    QVector<QPair<QStandardItem*, int> > sortable;
    QVector<int> unsortable;

    sortable.reserve(rowCount());
    unsortable.reserve(rowCount());

    for (int row = 0; row < rowCount(); ++row) {
        QStandardItem *itm = q->child(row, column);
        if (itm)
            sortable.append(QPair<QStandardItem*,int>(itm, row));
        else
            unsortable.append(row);
    }

    if (order == Qt::AscendingOrder) {
        QStandardItemModelLessThan lt;
        qStableSort(sortable.begin(), sortable.end(), lt);
    } else {
        QStandardItemModelGreaterThan gt;
        qStableSort(sortable.begin(), sortable.end(), gt);
    }

    QModelIndexList changedPersistentIndexesFrom, changedPersistentIndexesTo;
    QVector<QStandardItem*> sorted_children(children.count());
    for (int i = 0; i < rowCount(); ++i) {
        int r = (i < sortable.count()
                 ? sortable.at(i).second
                 : unsortable.at(i - sortable.count()));
        for (int c = 0; c < columnCount(); ++c) {
            QStandardItem *itm = q->child(r, c);
            sorted_children[childIndex(i, c)] = itm;
            if (model) {
                QModelIndex from = model->createIndex(r, c, q);
                if (model->d_func()->persistent.indexes.contains(from)) {
                    QModelIndex to = model->createIndex(i, c, q);
                    changedPersistentIndexesFrom.append(from);
                    changedPersistentIndexesTo.append(to);
                }
            }
        }
    }

    children = sorted_children;

    if (model) {
        model->changePersistentIndexList(changedPersistentIndexesFrom, changedPersistentIndexesTo);
    }

    QVector<QStandardItem*>::iterator it;
    for (it = children.begin(); it != children.end(); ++it) {
        if (*it)
            (*it)->d_func()->sortChildren(column, order);
    }
}

/*!
  \internal
  set the model of this item and all its children
  */
void QStandardItemPrivate::setModel(QStandardItemModel *mod)
{
    if (children.isEmpty()) {
        if (model)
            model->d_func()->invalidatePersistentIndex(model->indexFromItem(q_ptr));
        model = mod;
    } else {
        QStack<QStandardItem*> stack;
        stack.push(q_ptr);
        while (!stack.isEmpty()) {
            QStandardItem *itm = stack.pop();
            if (itm->d_func()->model) {
                itm->d_func()->model->d_func()->invalidatePersistentIndex(itm->d_func()->model->indexFromItem(itm));
            }
            itm->d_func()->model = mod;
            const QVector<QStandardItem*> &childList = itm->d_func()->children;
            for (int i = 0; i < childList.count(); ++i) {
                QStandardItem *chi = childList.at(i);
                if (chi)
                    stack.push(chi);
            }
        }
    }
}

/*!
  \internal
*/
QStandardItemModelPrivate::QStandardItemModelPrivate()
    : root(new QStandardItem),
      itemPrototype(0),
      sortRole(Qt::DisplayRole)
{
    root->setFlags(Qt::ItemIsDropEnabled);
}

/*!
  \internal
*/
QStandardItemModelPrivate::~QStandardItemModelPrivate()
{
    delete itemPrototype;
    qDeleteAll(columnHeaderItems);
    qDeleteAll(rowHeaderItems);
}

/*!
  \internal
*/
void QStandardItemModelPrivate::init()
{
    Q_Q(QStandardItemModel);
    QObject::connect(q, SIGNAL(dataChanged(QModelIndex,QModelIndex)),
                     q, SLOT(_q_emitItemChanged(QModelIndex,QModelIndex)));
}

/*!
    \internal
*/
void QStandardItemModelPrivate::_q_emitItemChanged(const QModelIndex &topLeft,
                                                   const QModelIndex &bottomRight)
{
    Q_Q(QStandardItemModel);
    QModelIndex parent = topLeft.parent();
    for (int row = topLeft.row(); row <= bottomRight.row(); ++row) {
        for (int column = topLeft.column(); column <= bottomRight.column(); ++column) {
            QModelIndex index = q->index(row, column, parent);
            if (QStandardItem *item = itemFromIndex(index))
                emit q->itemChanged(item);
        }
    }
}

/*!
    \internal
*/
bool QStandardItemPrivate::insertRows(int row, const QList<QStandardItem*> &items)
{
    Q_Q(QStandardItem);
    if ((row < 0) || (row > rowCount()))
        return false;
    int count = items.count();
    if (model)
        model->d_func()->rowsAboutToBeInserted(q, row, row + count - 1);
    if (rowCount() == 0) {
        if (columnCount() == 0)
            q->setColumnCount(1);
        children.resize(columnCount() * count);
        rows = count;
    } else {
        rows += count;
        int index = childIndex(row, 0);
        if (index != -1)
            children.insert(index, columnCount() * count, 0);
    }
    for (int i = 0; i < items.count(); ++i) {
        QStandardItem *item = items.at(i);
        item->d_func()->model = model;
        item->d_func()->parent = q;
        int index = childIndex(i + row, 0);
        children.replace(index, item);
    }
    if (model)
        model->d_func()->rowsInserted(q, row, count);
    return true;
}

bool QStandardItemPrivate::insertRows(int row, int count, const QList<QStandardItem*> &items)
{
    Q_Q(QStandardItem);
    if ((count < 1) || (row < 0) || (row > rowCount()))
        return false;
    if (model)
        model->d_func()->rowsAboutToBeInserted(q, row, row + count - 1);
    if (rowCount() == 0) {
        children.resize(columnCount() * count);
        rows = count;
    } else {
        rows += count;
        int index = childIndex(row, 0);
        if (index != -1)
            children.insert(index, columnCount() * count, 0);
    }
    if (!items.isEmpty()) {
        int index = childIndex(row, 0);
        int limit = qMin(items.count(), columnCount() * count);
        for (int i = 0; i < limit; ++i) {
            QStandardItem *item = items.at(i);
            if (item) {
                if (item->d_func()->parent == 0) {
                    item->d_func()->setParentAndModel(q, model);
                } else {
                    qWarning("QStandardItem::insertRows: Ignoring duplicate insertion of item %p",
                             item);
                    item = 0;
                }
            }
            children.replace(index, item);
            ++index;
        }
    }
    if (model)
        model->d_func()->rowsInserted(q, row, count);
    return true;
}

/*!
    \internal
*/
bool QStandardItemPrivate::insertColumns(int column, int count, const QList<QStandardItem*> &items)
{
    Q_Q(QStandardItem);
    if ((count < 1) || (column < 0) || (column > columnCount()))
        return false;
    if (model)
        model->d_func()->columnsAboutToBeInserted(q, column, column + count - 1);
    if (columnCount() == 0) {
        children.resize(rowCount() * count);
        columns = count;
    } else {
        columns += count;
        int index = childIndex(0, column);
        for (int row = 0; row < rowCount(); ++row) {
            children.insert(index, count, 0);
            index += columnCount();
        }
    }
    if (!items.isEmpty()) {
        int limit = qMin(items.count(), rowCount() * count);
        for (int i = 0; i < limit; ++i) {
            QStandardItem *item = items.at(i);
            if (item) {
                if (item->d_func()->parent == 0) {
                    item->d_func()->setParentAndModel(q, model);
                } else {
                    qWarning("QStandardItem::insertColumns: Ignoring duplicate insertion of item %p",
                             item);
                    item = 0;
                }
            }
            int r = i / count;
            int c = column + (i % count);
            int index = childIndex(r, c);
            children.replace(index, item);
        }
    }
    if (model)
        model->d_func()->columnsInserted(q, column, count);
    return true;
}

/*!
  \internal
*/
void QStandardItemModelPrivate::itemChanged(QStandardItem *item)
{
    Q_Q(QStandardItemModel);
    if (item->d_func()->parent == 0) {
        // Header item
        int idx = columnHeaderItems.indexOf(item);
        if (idx != -1) {
            emit q->headerDataChanged(Qt::Horizontal, idx, idx);
        } else {
            idx = rowHeaderItems.indexOf(item);
            if (idx != -1)
                emit q->headerDataChanged(Qt::Vertical, idx, idx);
        }
    } else {
        // Normal item
        QModelIndex index = q->indexFromItem(item);
        emit q->dataChanged(index, index);
    }
}

/*!
  \internal
*/
void QStandardItemModelPrivate::rowsAboutToBeInserted(QStandardItem *parent,
                                                      int start, int end)
{
    Q_Q(QStandardItemModel);
    QModelIndex index = q->indexFromItem(parent);
    q->beginInsertRows(index, start, end);
}

/*!
  \internal
*/
void QStandardItemModelPrivate::columnsAboutToBeInserted(QStandardItem *parent,
                                                         int start, int end)
{
    Q_Q(QStandardItemModel);
    QModelIndex index = q->indexFromItem(parent);
    q->beginInsertColumns(index, start, end);
}

/*!
  \internal
*/
void QStandardItemModelPrivate::rowsAboutToBeRemoved(QStandardItem *parent,
                                                     int start, int end)
{
    Q_Q(QStandardItemModel);
    QModelIndex index = q->indexFromItem(parent);
    q->beginRemoveRows(index, start, end);
}

/*!
  \internal
*/
void QStandardItemModelPrivate::columnsAboutToBeRemoved(QStandardItem *parent,
                                                        int start, int end)
{
    Q_Q(QStandardItemModel);
    QModelIndex index = q->indexFromItem(parent);
    q->beginRemoveColumns(index, start, end);
}

/*!
  \internal
*/
void QStandardItemModelPrivate::rowsInserted(QStandardItem *parent,
                                             int row, int count)
{
    Q_Q(QStandardItemModel);
    if (parent == root.data())
        rowHeaderItems.insert(row, count, 0);
    q->endInsertRows();
}

/*!
  \internal
*/
void QStandardItemModelPrivate::columnsInserted(QStandardItem *parent,
                                                int column, int count)
{
    Q_Q(QStandardItemModel);
    if (parent == root.data())
        columnHeaderItems.insert(column, count, 0);
    q->endInsertColumns();
}

/*!
  \internal
*/
void QStandardItemModelPrivate::rowsRemoved(QStandardItem *parent,
                                            int row, int count)
{
    Q_Q(QStandardItemModel);
    if (parent == root.data()) {
        for (int i = row; i < row + count; ++i) {
            QStandardItem *oldItem = rowHeaderItems.at(i);
            if (oldItem)
                oldItem->d_func()->setModel(0);
            delete oldItem;
        }
        rowHeaderItems.remove(row, count);
    }
    q->endRemoveRows();
}

/*!
  \internal
*/
void QStandardItemModelPrivate::columnsRemoved(QStandardItem *parent,
                                               int column, int count)
{
    Q_Q(QStandardItemModel);
    if (parent == root.data()) {
        for (int i = column; i < column + count; ++i) {
            QStandardItem *oldItem = columnHeaderItems.at(i);
            if (oldItem)
                oldItem->d_func()->setModel(0);
            delete oldItem;
        }
        columnHeaderItems.remove(column, count);
    }
    q->endRemoveColumns();
}

/*!
    \class QStandardItem
    \brief The QStandardItem class provides an item for use with the
    QStandardItemModel class.
    \since 4.2
    \ingroup model-view

    Items usually contain text, icons, or checkboxes.

    Each item can have its own background brush which is set with the
    setBackground() function. The current background brush can be found with
    background().  The text label for each item can be rendered with its own
    font and brush. These are specified with the setFont() and setForeground()
    functions, and read with font() and foreground().

    By default, items are enabled, editable, selectable, checkable, and can be
    used both as the source of a drag and drop operation and as a drop target.
    Each item's flags can be changed by calling setFlags(). Checkable items
    can be checked and unchecked with the setCheckState() function. The
    corresponding checkState() function indicates whether the item is
    currently checked.

    You can store application-specific data in an item by calling setData().

    Each item can have a two-dimensional table of child items. This makes it
    possible to build hierarchies of items. The typical hierarchy is the tree,
    in which case the child table is a table with a single column (a list).

    The dimensions of the child table can be set with setRowCount() and
    setColumnCount(). Items can be positioned in the child table with
    setChild(). Get a pointer to a child item with child(). New rows and
    columns of children can also be inserted with insertRow() and
    insertColumn(), or appended with appendRow() and appendColumn(). When
    using the append and insert functions, the dimensions of the child table
    will grow as needed.

    An existing row of children can be removed with removeRow() or takeRow();
    correspondingly, a column can be removed with removeColumn() or
    takeColumn().

    An item's children can be sorted by calling sortChildren().

    \section1 Subclassing

    When subclassing QStandardItem to provide custom items, it is possible to
    define new types for them so that they can be distinguished from the base
    class. The type() function should be reimplemented to return a new type
    value equal to or greater than \l UserType.

    Reimplement data() and setData() if you want to perform custom handling of
    data queries and/or control how an item's data is represented.

    Reimplement clone() if you want QStandardItemModel to be able to create
    instances of your custom item class on demand (see
    QStandardItemModel::setItemPrototype()).

    Reimplement read() and write() if you want to control how items are
    represented in their serialized form.

    Reimplement \l{operator<()} if you want to control the semantics of item
    comparison. \l{operator<()} determines the sorted order when sorting items
    with sortChildren() or with QStandardItemModel::sort().

    \sa QStandardItemModel, {Item View Convenience Classes}, {Model/View Programming}
*/

/*!
    \enum QStandardItem::ItemType

    This enum describes the types that are used to describe standard items.

    \value Type     The default type for standard items.
    \value UserType The minimum value for custom types. Values below UserType are
                    reserved by Qt.

    You can define new user types in QStandardItem subclasses to ensure that
    custom items are treated specially; for example, when items are sorted.

    \sa type()
*/

/*!
    Constructs an item.
*/
QStandardItem::QStandardItem()
    : d_ptr(new QStandardItemPrivate)
{
    Q_D(QStandardItem);
    d->q_ptr = this;
}

/*!
    Constructs an item with the given \a text.
*/
QStandardItem::QStandardItem(const QString &text)
    : d_ptr(new QStandardItemPrivate)
{
    Q_D(QStandardItem);
    d->q_ptr = this;
    setText(text);
}

/*!
    Constructs an item with the given \a icon and \a text.
*/
QStandardItem::QStandardItem(const QIcon &icon, const QString &text)
    : d_ptr(new QStandardItemPrivate)
{
    Q_D(QStandardItem);
    d->q_ptr = this;
    setIcon(icon);
    setText(text);
}

/*!
   Constructs an item with \a rows rows and \a columns columns of child items.
*/
QStandardItem::QStandardItem(int rows, int columns)
    : d_ptr(new QStandardItemPrivate)
{
    Q_D(QStandardItem);
    d->q_ptr = this;
    setRowCount(rows);
    setColumnCount(columns);
}

/*!
  \internal
*/
QStandardItem::QStandardItem(QStandardItemPrivate &dd)
    : d_ptr(&dd)
{
    Q_D(QStandardItem);
    d->q_ptr = this;
}

/*!
  Constructs a copy of \a other. Note that model() is
  not copied.

  This function is useful when reimplementing clone().
*/
QStandardItem::QStandardItem(const QStandardItem &other)
    : d_ptr(new QStandardItemPrivate)
{
    Q_D(QStandardItem);
    d->q_ptr = this;
    operator=(other);
}

/*!
  Assigns \a other's data and flags to this item. Note that
  type() and model() are not copied.

  This function is useful when reimplementing clone().
*/
QStandardItem &QStandardItem::operator=(const QStandardItem &other)
{
    Q_D(QStandardItem);
    d->values = other.d_func()->values;
    return *this;
}

/*!
  Destructs the item.
  This causes the item's children to be destructed as well.
*/
QStandardItem::~QStandardItem()
{
}

/*!
  Returns the item's parent item, or 0 if the item has no parent.

  \sa child()
*/
QStandardItem *QStandardItem::parent() const
{
    Q_D(const QStandardItem);
    if (!d->model || (d->model->d_func()->root.data() != d->parent))
        return d->parent;
    return 0;
}

/*!
    Sets the item's data for the given \a role to the specified \a value.

    If you subclass QStandardItem and reimplement this function, your
    reimplementation should call emitDataChanged() if you do not call
    the base implementation of setData(). This will ensure that e.g.
    views using the model are notified of the changes.

    \note The default implementation treats Qt::EditRole and Qt::DisplayRole
    as referring to the same data.

    \sa Qt::ItemDataRole, data(), setFlags()
*/
void QStandardItem::setData(const QVariant &value, int role)
{
    Q_D(QStandardItem);
    role = (role == Qt::EditRole) ? Qt::DisplayRole : role;
    QVector<QWidgetItemData>::iterator it;
    for (it = d->values.begin(); it != d->values.end(); ++it) {
        if ((*it).role == role) {
            if (value.isValid()) {
                if ((*it).value.type() == value.type() && (*it).value == value)
                    return;
                (*it).value = value;
            } else {
                d->values.erase(it);
            }
            if (d->model)
                d->model->d_func()->itemChanged(this);
            return;
        }
    }
    d->values.append(QWidgetItemData(role, value));
    if (d->model)
        d->model->d_func()->itemChanged(this);
}

/*!
    Returns the item's data for the given \a role, or an invalid
    QVariant if there is no data for the role.

    \note The default implementation treats Qt::EditRole and Qt::DisplayRole
    as referring to the same data.
*/
QVariant QStandardItem::data(int role) const
{
    Q_D(const QStandardItem);
    role = (role == Qt::EditRole) ? Qt::DisplayRole : role;
    QVector<QWidgetItemData>::const_iterator it;
    for (it = d->values.begin(); it != d->values.end(); ++it) {
        if ((*it).role == role)
            return (*it).value;
    }
    return QVariant();
}

/*!
  \since 4.4

  Causes the model associated with this item to emit a
  \l{QAbstractItemModel::dataChanged()}{dataChanged}() signal for this
  item.

  You normally only need to call this function if you have subclassed
  QStandardItem and reimplemented data() and/or setData().

  \sa setData()
*/
void QStandardItem::emitDataChanged()
{
    Q_D(QStandardItem);
    if (d->model)
        d->model->d_func()->itemChanged(this);
}

/*!
  Sets the item flags for the item to \a flags.

  The item flags determine how the user can interact with the item.
  This is often used to disable an item.

  \sa flags(), setData()
*/
void QStandardItem::setFlags(Qt::ItemFlags flags)
{
    setData((int)flags, Qt::UserRole - 1);
}

/*!
  Returns the item flags for the item.

  The item flags determine how the user can interact with the item.

  By default, items are enabled, editable, selectable, checkable, and can be
  used both as the source of a drag and drop operation and as a drop target.

  \sa setFlags()
*/
Qt::ItemFlags QStandardItem::flags() const
{
    QVariant v = data(Qt::UserRole - 1);
    if (!v.isValid())
        return (Qt::ItemIsSelectable|Qt::ItemIsEnabled|Qt::ItemIsEditable
                |Qt::ItemIsDragEnabled|Qt::ItemIsDropEnabled);
    return Qt::ItemFlags(v.toInt());
}

/*!
    \fn QString QStandardItem::text() const

    Returns the item's text. This is the text that's presented to the user
    in a view.

    \sa setText()
*/

/*!
    \fn void QStandardItem::setText(const QString &text)

    Sets the item's text to the \a text specified.

    \sa text(), setFont(), setForeground()
*/

/*!
    \fn QIcon QStandardItem::icon() const

    Returns the item's icon.

    \sa setIcon(), {QAbstractItemView::iconSize}{iconSize}
*/

/*!
    \fn void QStandardItem::setIcon(const QIcon &icon)

    Sets the item's icon to the \a icon specified.
*/

/*!
    \fn QString QStandardItem::statusTip() const

    Returns the item's status tip.

    \sa setStatusTip(), toolTip(), whatsThis()
*/

/*!
    \fn void QStandardItem::setStatusTip(const QString &statusTip)

    Sets the item's status tip to the string specified by \a statusTip.

    \sa statusTip(), setToolTip(), setWhatsThis()
*/

/*!
    \fn QString QStandardItem::toolTip() const

    Returns the item's tooltip.

    \sa setToolTip(), statusTip(), whatsThis()
*/

/*!
    \fn void QStandardItem::setToolTip(const QString &toolTip)

    Sets the item's tooltip to the string specified by \a toolTip.

    \sa toolTip(), setStatusTip(), setWhatsThis()
*/

/*!
    \fn QString QStandardItem::whatsThis() const

    Returns the item's "What's This?" help.

    \sa setWhatsThis(), toolTip(), statusTip()
*/

/*!
    \fn void QStandardItem::setWhatsThis(const QString &whatsThis)

    Sets the item's "What's This?" help to the string specified by \a whatsThis.

    \sa whatsThis(), setStatusTip(), setToolTip()
*/

/*!
    \fn QFont QStandardItem::font() const

    Returns the font used to render the item's text.

    \sa setFont()
*/

/*!
    \fn void QStandardItem::setFont(const QFont &font)

    Sets the font used to display the item's text to the given \a font.

    \sa font() setText() setForeground()
*/

/*!
    \fn QBrush QStandardItem::background() const

    Returns the brush used to render the item's background.

    \sa  foreground() setBackground()
*/

/*!
    \fn void QStandardItem::setBackground(const QBrush &brush)

    Sets the item's background brush to the specified \a brush.

    \sa background() setForeground()
*/

/*!
    \fn QBrush QStandardItem::foreground() const

    Returns the brush used to render the item's foreground (e.g. text).

    \sa setForeground() background()
*/

/*!
    \fn void QStandardItem::setForeground(const QBrush &brush)

    Sets the brush used to display the item's foreground (e.g. text) to the
    given \a brush.

    \sa foreground() setBackground() setFont()
*/

/*!
    \fn int QStandardItem::textAlignment() const

    Returns the text alignment for the item's text.
*/

/*!
    \fn void QStandardItem::setTextAlignment(Qt::Alignment alignment)

    Sets the text alignment for the item's text to the \a alignment
    specified.

    \sa textAlignment()
*/

/*!
    \fn QSize QStandardItem::sizeHint() const

    Returns the size hint set for the item, or an invalid QSize if no
    size hint has been set.

    If no size hint has been set, the item delegate will compute the
    size hint based on the item data.

    \sa setSizeHint()
*/

/*!
    \fn void QStandardItem::setSizeHint(const QSize &size)

    Sets the size hint for the item to be \a size.
    If no size hint is set, the item delegate will compute the
    size hint based on the item data.

    \sa sizeHint()
*/

/*!
    \fn Qt::CheckState QStandardItem::checkState() const

    Returns the checked state of the item.

    \sa setCheckState(), isCheckable()
*/

/*!
    \fn void QStandardItem::setCheckState(Qt::CheckState state)

    Sets the check state of the item to be \a state.

    \sa checkState(), setCheckable()
*/

/*!
    \fn QString QStandardItem::accessibleText() const

    Returns the item's accessible text.

    The accessible text is used by assistive technologies (i.e. for users who
    cannot use conventional means of interaction).

    \sa setAccessibleText(), accessibleDescription()
*/

/*!
    \fn void QStandardItem::setAccessibleText(const QString &accessibleText)

    Sets the item's accessible text to the string specified by \a accessibleText.

    The accessible text is used by assistive technologies (i.e. for users who
    cannot use conventional means of interaction).

    \sa accessibleText(), setAccessibleDescription()
*/

/*!
    \fn QString QStandardItem::accessibleDescription() const

    Returns the item's accessible description.

    The accessible description is used by assistive technologies (i.e. for
    users who cannot use conventional means of interaction).

    \sa setAccessibleDescription(), accessibleText()
*/

/*!
    \fn void QStandardItem::setAccessibleDescription(const QString &accessibleDescription)

    Sets the item's accessible description to the string specified by \a
    accessibleDescription.

    The accessible description is used by assistive technologies (i.e. for
    users who cannot use conventional means of interaction).

    \sa accessibleDescription(), setAccessibleText()
*/

/*!
  Sets whether the item is enabled. If \a enabled is true, the item is enabled,
  meaning that the user can interact with the item; if \a enabled is false, the
  user cannot interact with the item.

  This flag takes precedence over the other item flags; e.g. if an item is not
  enabled, it cannot be selected by the user, even if the Qt::ItemIsSelectable
  flag has been set.

  \sa isEnabled(), Qt::ItemIsEnabled, setFlags()
*/
void QStandardItem::setEnabled(bool enabled)
{
    Q_D(QStandardItem);
    d->changeFlags(enabled, Qt::ItemIsEnabled);
}

/*!
  \fn bool QStandardItem::isEnabled() const

  Returns whether the item is enabled.

  When an item is enabled, the user can interact with it. The possible
  types of interaction are specified by the other item flags, such as
  isEditable() and isSelectable().

  The default value is true.

  \sa setEnabled(), flags()
*/

/*!
  Sets whether the item is editable. If \a editable is true, the item can be
  edited by the user; otherwise, the user cannot edit the item.

  How the user can edit items in a view is determined by the view's edit
  triggers; see QAbstractItemView::editTriggers.

  \sa isEditable(), setFlags()
*/
void QStandardItem::setEditable(bool editable)
{
    Q_D(QStandardItem);
    d->changeFlags(editable, Qt::ItemIsEditable);
}

/*!
  \fn bool QStandardItem::isEditable() const

  Returns whether the item can be edited by the user.

  When an item is editable (and enabled), the user can edit the item by
  invoking one of the view's edit triggers; see
  QAbstractItemView::editTriggers.

  The default value is true.

  \sa setEditable(), flags()
*/

/*!
  Sets whether the item is selectable. If \a selectable is true, the item
  can be selected by the user; otherwise, the user cannot select the item.

  You can control the selection behavior and mode by manipulating their
  view properties; see QAbstractItemView::selectionMode and
  QAbstractItemView::selectionBehavior.

  \sa isSelectable(), setFlags()
*/
void QStandardItem::setSelectable(bool selectable)
{
    Q_D(QStandardItem);
    d->changeFlags(selectable, Qt::ItemIsSelectable);
}

/*!
  \fn bool QStandardItem::isSelectable() const

  Returns whether the item is selectable by the user.

  The default value is true.

  \sa setSelectable(), flags()
*/

/*!
  Sets whether the item is user-checkable. If \a checkable is true, the
  item can be checked by the user; otherwise, the user cannot check
  the item.

  The item delegate will render a checkable item with a check box next to the
  item's text.

  \sa isCheckable(), setCheckState(), setTristate()
*/
void QStandardItem::setCheckable(bool checkable)
{
    Q_D(QStandardItem);
    if (checkable && !isCheckable()) {
        // make sure there's data for the checkstate role
        if (!data(Qt::CheckStateRole).isValid())
            setData(Qt::Unchecked, Qt::CheckStateRole);
    }
    d->changeFlags(checkable, Qt::ItemIsUserCheckable);
}

/*!
  \fn bool QStandardItem::isCheckable() const

  Returns whether the item is user-checkable.

  The default value is false.

  \sa setCheckable(), checkState(), isTristate()
*/

/*!
  Sets whether the item is tristate. If \a tristate is true, the
  item is checkable with three separate states; otherwise, the item
  is checkable with two states. (Note that this also requires that
  the item is checkable; see isCheckable().)

  \sa isTristate(), setCheckable(), setCheckState()
*/
void QStandardItem::setTristate(bool tristate)
{
    Q_D(QStandardItem);
    d->changeFlags(tristate, Qt::ItemIsTristate);
}

/*!
  \fn bool QStandardItem::isTristate() const

  Returns whether the item is tristate; that is, if it's checkable with three
  separate states.

  The default value is false.

  \sa setTristate(), isCheckable(), checkState()
*/

#ifndef QT_NO_DRAGANDDROP

/*!
  Sets whether the item is drag enabled. If \a dragEnabled is true, the item
  can be dragged by the user; otherwise, the user cannot drag the item.

  Note that you also need to ensure that item dragging is enabled in the view;
  see QAbstractItemView::dragEnabled.

  \sa isDragEnabled(), setDropEnabled(), setFlags()
*/
void QStandardItem::setDragEnabled(bool dragEnabled)
{
    Q_D(QStandardItem);
    d->changeFlags(dragEnabled, Qt::ItemIsDragEnabled);
}

/*!
  \fn bool QStandardItem::isDragEnabled() const

  Returns whether the item is drag enabled. An item that is drag enabled can
  be dragged by the user.

  The default value is true.

  Note that item dragging must be enabled in the view for dragging to work;
  see QAbstractItemView::dragEnabled.

  \sa setDragEnabled(), isDropEnabled(), flags()
*/

/*!
  Sets whether the item is drop enabled. If \a dropEnabled is true, the item
  can be used as a drop target; otherwise, it cannot.

  Note that you also need to ensure that drops are enabled in the view; see
  QWidget::acceptDrops(); and that the model supports the desired drop actions;
  see QAbstractItemModel::supportedDropActions().

  \sa isDropEnabled(), setDragEnabled(), setFlags()
*/
void QStandardItem::setDropEnabled(bool dropEnabled)
{
    Q_D(QStandardItem);
    d->changeFlags(dropEnabled, Qt::ItemIsDropEnabled);
}

/*!
  \fn bool QStandardItem::isDropEnabled() const

  Returns whether the item is drop enabled. When an item is drop enabled, it
  can be used as a drop target.

  The default value is true.

  \sa setDropEnabled(), isDragEnabled(), flags()
*/

#endif // QT_NO_DRAGANDDROP

/*!
  Returns the row where the item is located in its parent's child table, or
  -1 if the item has no parent.

  \sa column(), parent()
*/
int QStandardItem::row() const
{
    Q_D(const QStandardItem);
    QPair<int, int> pos = d->position();
    return pos.first;
}

/*!
  Returns the column where the item is located in its parent's child table,
  or -1 if the item has no parent.

  \sa row(), parent()
*/
int QStandardItem::column() const
{
    Q_D(const QStandardItem);
    QPair<int, int> pos = d->position();
    return pos.second;
}

/*!
  Returns the QModelIndex associated with this item.

  When you need to invoke item functionality in a QModelIndex-based API (e.g.
  QAbstractItemView), you can call this function to obtain an index that
  corresponds to the item's location in the model.

  If the item is not associated with a model, an invalid QModelIndex is
  returned.

  \sa model(), QStandardItemModel::itemFromIndex()
*/
QModelIndex QStandardItem::index() const
{
    Q_D(const QStandardItem);
    return d->model ? d->model->indexFromItem(this) : QModelIndex();
}

/*!
  Returns the QStandardItemModel that this item belongs to.

  If the item is not a child of another item that belongs to the model, this
  function returns 0.

  \sa index()
*/
QStandardItemModel *QStandardItem::model() const
{
    Q_D(const QStandardItem);
    return d->model;
}

/*!
    Sets the number of child item rows to \a rows. If this is less than
    rowCount(), the data in the unwanted rows is discarded.

    \sa rowCount(), setColumnCount()
*/
void QStandardItem::setRowCount(int rows)
{
    int rc = rowCount();
    if (rc == rows)
        return;
    if (rc < rows)
        insertRows(qMax(rc, 0), rows - rc);
    else
        removeRows(qMax(rows, 0), rc - rows);
}

/*!
    Returns the number of child item rows that the item has.

    \sa setRowCount(), columnCount()
*/
int QStandardItem::rowCount() const
{
    Q_D(const QStandardItem);
    return d->rowCount();
}

/*!
    Sets the number of child item columns to \a columns. If this is less than
    columnCount(), the data in the unwanted columns is discarded.

    \sa columnCount(), setRowCount()
*/
void QStandardItem::setColumnCount(int columns)
{
    int cc = columnCount();
    if (cc == columns)
        return;
    if (cc < columns)
        insertColumns(qMax(cc, 0), columns - cc);
    else
        removeColumns(qMax(columns, 0), cc - columns);
}

/*!
    Returns the number of child item columns that the item has.

    \sa setColumnCount(), rowCount()
*/
int QStandardItem::columnCount() const
{
    Q_D(const QStandardItem);
    return d->columnCount();
}

/*!
    Inserts a row at \a row containing \a items. If necessary, the column
    count is increased to the size of \a items.

    \sa insertRows(), insertColumn()
*/
void QStandardItem::insertRow(int row, const QList<QStandardItem*> &items)
{
    Q_D(QStandardItem);
    if (row < 0)
        return;
    if (columnCount() < items.count())
        setColumnCount(items.count());
    d->insertRows(row, 1, items);
}

/*!
    Inserts \a items at \a row. The column count wont be changed.

    \sa insertRow(), insertColumn()
*/
void QStandardItem::insertRows(int row, const QList<QStandardItem*> &items)
{
    Q_D(QStandardItem);
    if (row < 0)
        return;
    d->insertRows(row, items);
}

/*!
    Inserts a column at \a column containing \a items. If necessary,
    the row count is increased to the size of \a items.

    \sa insertColumns(), insertRow()
*/
void QStandardItem::insertColumn(int column, const QList<QStandardItem*> &items)
{
    Q_D(QStandardItem);
    if (column < 0)
        return;
    if (rowCount() < items.count())
        setRowCount(items.count());
    d->insertColumns(column, 1, items);
}

/*!
    Inserts \a count rows of child items at row \a row.

    \sa insertRow(), insertColumns()
*/
void QStandardItem::insertRows(int row, int count)
{
    Q_D(QStandardItem);
    if (rowCount() < row) {
        count += row - rowCount();
        row = rowCount();
    }
    d->insertRows(row, count, QList<QStandardItem*>());
}

/*!
    Inserts \a count columns of child items at column \a column.

    \sa insertColumn(), insertRows()
*/
void QStandardItem::insertColumns(int column, int count)
{
    Q_D(QStandardItem);
    if (columnCount() < column) {
        count += column - columnCount();
        column = columnCount();
    }
    d->insertColumns(column, count, QList<QStandardItem*>());
}

/*!
    \fn void QStandardItem::appendRow(const QList<QStandardItem*> &items)

    Appends a row containing \a items. If necessary, the column count is
    increased to the size of \a items.

    \sa insertRow()
*/

/*!
    \fn void QStandardItem::appendRows(const QList<QStandardItem*> &items)

    Appends rows containing \a items.  The column count will not change.

    \sa insertRow()
*/

/*!
    \fn void QStandardItem::appendColumn(const QList<QStandardItem*> &items)

    Appends a column containing \a items. If necessary, the row count is
    increased to the size of \a items.

    \sa insertColumn()
*/

/*!
    \fn bool QStandardItemModel::insertRow(int row, const QModelIndex &parent)

    Inserts a single row before the given \a row in the child items of the
    \a parent specified. Returns true if the row is inserted; otherwise
    returns false.

    \sa insertRows(), insertColumn(), removeRow()
*/

/*!
    \fn bool QStandardItemModel::insertColumn(int column, const QModelIndex &parent)

    Inserts a single column before the given \a column in the child items of
    the \a parent specified. Returns true if the column is inserted; otherwise
    returns false.

    \sa insertColumns(), insertRow(), removeColumn()
*/

/*!
    \fn QStandardItem::insertRow(int row, QStandardItem *item)
    \overload

    Inserts a row at \a row containing \a item.

    When building a list or a tree that has only one column, this function
    provides a convenient way to insert a single new item.
*/

/*!
    \fn QStandardItem::appendRow(QStandardItem *item)
    \overload

    Appends a row containing \a item.

    When building a list or a tree that has only one column, this function
    provides a convenient way to append a single new item.
*/

/*!
    Removes the given \a row. The items that were in the row are deleted.

    \sa takeRow(), removeRows(), removeColumn()
*/
void QStandardItem::removeRow(int row)
{
    removeRows(row, 1);
}

/*!
    Removes the given \a column. The items that were in the
    column are deleted.

    \sa takeColumn(), removeColumns(), removeRow()
*/
void QStandardItem::removeColumn(int column)
{
    removeColumns(column, 1);
}

/*!
    Removes \a count rows at row \a row. The items that were in those rows are
    deleted.

    \sa removeRow(), removeColumn()
*/
void QStandardItem::removeRows(int row, int count)
{
    Q_D(QStandardItem);
    if ((count < 1) || (row < 0) || ((row + count) > rowCount()))
        return;
    if (d->model)
        d->model->d_func()->rowsAboutToBeRemoved(this, row, row + count - 1);
    int i = d->childIndex(row, 0);
    int n = count * d->columnCount();
    for (int j = i; j < n+i; ++j) {
        QStandardItem *oldItem = d->children.at(j);
        if (oldItem)
            oldItem->d_func()->setModel(0);
        delete oldItem;
    }
    d->children.remove(qMax(i, 0), n);
    d->rows -= count;
    if (d->model)
        d->model->d_func()->rowsRemoved(this, row, count);
}

/*!
    Removes \a count columns at column \a column. The items that were in those
    columns are deleted.

    \sa removeColumn(), removeRows()
*/
void QStandardItem::removeColumns(int column, int count)
{
    Q_D(QStandardItem);
    if ((count < 1) || (column < 0) || ((column + count) > columnCount()))
        return;
    if (d->model)
        d->model->d_func()->columnsAboutToBeRemoved(this, column, column + count - 1);
    for (int row = d->rowCount() - 1; row >= 0; --row) {
        int i = d->childIndex(row, column);
        for (int j=i; j<i+count; ++j) {
            QStandardItem *oldItem = d->children.at(j);
            if (oldItem)
                oldItem->d_func()->setModel(0);
            delete oldItem;
        }
        d->children.remove(i, count);
    }
    d->columns -= count;
    if (d->model)
        d->model->d_func()->columnsRemoved(this, column, count);
}

/*!
    Returns true if this item has any children; otherwise returns false.

    \sa rowCount(), columnCount(), child()
*/
bool QStandardItem::hasChildren() const
{
    return (rowCount() > 0) && (columnCount() > 0);
}

/*!
    Sets the child item at (\a row, \a column) to \a item. This item (the parent
    item) takes ownership of \a item. If necessary, the row count and column
    count are increased to fit the item.

    \sa child()
*/
void QStandardItem::setChild(int row, int column, QStandardItem *item)
{
    Q_D(QStandardItem);
    d->setChild(row, column, item, true);
}

/*!
    \fn QStandardItem::setChild(int row, QStandardItem *item)
    \overload

    Sets the child at \a row to \a item.
*/

/*!
    Returns the child item at (\a row, \a column) if one has been set; otherwise
    returns 0.

    \sa setChild(), takeChild(), parent()
*/
QStandardItem *QStandardItem::child(int row, int column) const
{
    Q_D(const QStandardItem);
    int index = d->childIndex(row, column);
    if (index == -1)
        return 0;
    return d->children.at(index);
}

/*!
    Removes the child item at (\a row, \a column) without deleting it, and returns
    a pointer to the item. If there was no child at the given location, then
    this function returns 0.

    Note that this function, unlike takeRow() and takeColumn(), does not affect
    the dimensions of the child table.

    \sa child(), takeRow(), takeColumn()
*/
QStandardItem *QStandardItem::takeChild(int row, int column)
{
    Q_D(QStandardItem);
    QStandardItem *item = 0;
    int index = d->childIndex(row, column);
    if (index != -1) {
        item = d->children.at(index);
        if (item)
            item->d_func()->setParentAndModel(0, 0);
        d->children.replace(index, 0);
    }
    return item;
}

/*!
    Removes \a row without deleting the row items, and returns a list of
    pointers to the removed items. For items in the row that have not been
    set, the corresponding pointers in the list will be 0.

    \sa removeRow(), insertRow(), takeColumn()
*/
QList<QStandardItem*> QStandardItem::takeRow(int row)
{
    Q_D(QStandardItem);
    if ((row < 0) || (row >= rowCount()))
        return QList<QStandardItem*>();
    if (d->model)
        d->model->d_func()->rowsAboutToBeRemoved(this, row, row);
    QList<QStandardItem*> items;
    int index = d->childIndex(row, 0);  // Will return -1 if there are no columns
    if (index != -1) {
        int col_count = d->columnCount();
        for (int column = 0; column < col_count; ++column) {
            QStandardItem *ch = d->children.at(index + column);
            if (ch)
                ch->d_func()->setParentAndModel(0, 0);
            items.append(ch);
        }
        d->children.remove(index, col_count);
    }
    d->rows--;
    if (d->model)
        d->model->d_func()->rowsRemoved(this, row, 1);
    return items;
}

/*!
    Removes \a column without deleting the column items, and returns a list of
    pointers to the removed items. For items in the column that have not been
    set, the corresponding pointers in the list will be 0.

    \sa removeColumn(), insertColumn(), takeRow()
*/
QList<QStandardItem*> QStandardItem::takeColumn(int column)
{
    Q_D(QStandardItem);
    if ((column < 0) || (column >= columnCount()))
        return QList<QStandardItem*>();
    if (d->model)
        d->model->d_func()->columnsAboutToBeRemoved(this, column, column);
    QList<QStandardItem*> items;

    for (int row = d->rowCount() - 1; row >= 0; --row) {
        int index = d->childIndex(row, column);
        QStandardItem *ch = d->children.at(index);
        if (ch)
            ch->d_func()->setParentAndModel(0, 0);
        d->children.remove(index);
        items.prepend(ch);
    }
    d->columns--;
    if (d->model)
        d->model->d_func()->columnsRemoved(this, column, 1);
    return items;
}

/*!
    Returns true if this item is less than \a other; otherwise returns false.

    The default implementation uses the data for the item's sort role (see
    QStandardItemModel::sortRole) to perform the comparison if the item
    belongs to a model; otherwise, the data for the item's Qt::DisplayRole
    (text()) is used to perform the comparison.

    sortChildren() and QStandardItemModel::sort() use this function when
    sorting items. If you want custom sorting, you can subclass QStandardItem
    and reimplement this function.
*/
bool QStandardItem::operator<(const QStandardItem &other) const
{
    const int role = model() ? model()->sortRole() : Qt::DisplayRole;
    const QVariant l = data(role), r = other.data(role);
    // this code is copied from QSortFilterProxyModel::lessThan()
    switch (l.userType()) {
    case QVariant::Invalid:
        return (r.type() == QVariant::Invalid);
    case QVariant::Int:
        return l.toInt() < r.toInt();
    case QVariant::UInt:
        return l.toUInt() < r.toUInt();
    case QVariant::LongLong:
        return l.toLongLong() < r.toLongLong();
    case QVariant::ULongLong:
        return l.toULongLong() < r.toULongLong();
    case QMetaType::Float:
        return l.toFloat() < r.toFloat();
    case QVariant::Double:
        return l.toDouble() < r.toDouble();
    case QVariant::Char:
        return l.toChar() < r.toChar();
    case QVariant::Date:
        return l.toDate() < r.toDate();
    case QVariant::Time:
        return l.toTime() < r.toTime();
    case QVariant::DateTime:
        return l.toDateTime() < r.toDateTime();
    case QVariant::String:
    default:
        return l.toString().compare(r.toString()) < 0;
    }
}

/*!
    Sorts the children of the item using the given \a order, by the values in
    the given \a column.

    \note This function is recursive, therefore it sorts the children of the
    item, its grandchildren, etc.

    \sa {operator<()}
*/
void QStandardItem::sortChildren(int column, Qt::SortOrder order)
{
    Q_D(QStandardItem);
    if ((column < 0) || (rowCount() == 0))
        return;
    if (d->model)
        emit d->model->layoutAboutToBeChanged();
    d->sortChildren(column, order);
    if (d->model)
        emit d->model->layoutChanged();
}

/*!
    Returns a copy of this item. The item's children are not copied.

    When subclassing QStandardItem, you can reimplement this function
    to provide QStandardItemModel with a factory that it can use to
    create new items on demand.

    \sa QStandardItemModel::setItemPrototype(), operator=()
*/
QStandardItem *QStandardItem::clone() const
{
    return new QStandardItem(*this);
}

/*!
    Returns the type of this item. The type is used to distinguish custom
    items from the base class. When subclassing QStandardItem, you should
    reimplement this function and return a new value greater than or equal
    to \l UserType.

    \sa QStandardItem::Type
*/
int QStandardItem::type() const
{
    return Type;
}

#ifndef QT_NO_DATASTREAM

/*!
    Reads the item from stream \a in. Only the data and flags of the item are
    read, not the child items.

    \sa write()
*/
void QStandardItem::read(QDataStream &in)
{
    Q_D(QStandardItem);
    in >> d->values;
    qint32 flags;
    in >> flags;
    setFlags(Qt::ItemFlags(flags));
}

/*!
    Writes the item to stream \a out. Only the data and flags of the item
    are written, not the child items.

    \sa read()
*/
void QStandardItem::write(QDataStream &out) const
{
    Q_D(const QStandardItem);
    out << d->values;
    out << flags();
}

/*!
    \relates QStandardItem
    \since 4.2

    Reads a QStandardItem from stream \a in into \a item.

    This operator uses QStandardItem::read().

    \sa {Serializing Qt Data Types}
*/
QDataStream &operator>>(QDataStream &in, QStandardItem &item)
{
    item.read(in);
    return in;
}

/*!
    \relates QStandardItem
    \since 4.2

    Writes the QStandardItem \a item to stream \a out.

    This operator uses QStandardItem::write().

    \sa {Serializing Qt Data Types}
*/
QDataStream &operator<<(QDataStream &out, const QStandardItem &item)
{
    item.write(out);
    return out;
}

#endif // QT_NO_DATASTREAM

/*!
    \class QStandardItemModel
    \brief The QStandardItemModel class provides a generic model for storing custom data.
    \ingroup model-view

    QStandardItemModel can be used as a repository for standard Qt
    data types. It is one of the \l {Model/View Classes} and is part
    of Qt's \l {Model/View Programming}{model/view} framework.

    QStandardItemModel provides a classic item-based approach to working with
    the model.  The items in a QStandardItemModel are provided by
    QStandardItem.

    QStandardItemModel implements the QAbstractItemModel interface, which
    means that the model can be used to provide data in any view that supports
    that interface (such as QListView, QTableView and QTreeView, and your own
    custom views). For performance and flexibility, you may want to subclass
    QAbstractItemModel to provide support for different kinds of data
    repositories. For example, the QDirModel provides a model interface to the
    underlying file system.

    When you want a list or tree, you typically create an empty
    QStandardItemModel and use appendRow() to add items to the model, and
    item() to access an item.  If your model represents a table, you typically
    pass the dimensions of the table to the QStandardItemModel constructor and
    use setItem() to position items into the table. You can also use setRowCount()
    and setColumnCount() to alter the dimensions of the model. To insert items,
    use insertRow() or insertColumn(), and to remove items, use removeRow() or
    removeColumn().

    You can set the header labels of your model with setHorizontalHeaderLabels()
    and setVerticalHeaderLabels().

    You can search for items in the model with findItems(), and sort the model by
    calling sort().

    Call clear() to remove all items from the model.

    An example usage of QStandardItemModel to create a table:

    \snippet doc/src/snippets/code/src_gui_itemviews_qstandarditemmodel.cpp 0

    An example usage of QStandardItemModel to create a tree:

    \snippet doc/src/snippets/code/src_gui_itemviews_qstandarditemmodel.cpp 1

    After setting the model on a view, you typically want to react to user
    actions, such as an item being clicked. Since a QAbstractItemView provides
    QModelIndex-based signals and functions, you need a way to obtain the
    QStandardItem that corresponds to a given QModelIndex, and vice
    versa. itemFromIndex() and indexFromItem() provide this mapping. Typical
    usage of itemFromIndex() includes obtaining the item at the current index
    in a view, and obtaining the item that corresponds to an index carried by
    a QAbstractItemView signal, such as QAbstractItemView::clicked(). First
    you connect the view's signal to a slot in your class:

    \snippet doc/src/snippets/code/src_gui_itemviews_qstandarditemmodel.cpp 2

    When you receive the signal, you call itemFromIndex() on the given model
    index to get a pointer to the item:

    \snippet doc/src/snippets/code/src_gui_itemviews_qstandarditemmodel.cpp 3

    Conversely, you must obtain the QModelIndex of an item when you want to
    invoke a model/view function that takes an index as argument. You can
    obtain the index either by using the model's indexFromItem() function, or,
    equivalently, by calling QStandardItem::index():

    \snippet doc/src/snippets/code/src_gui_itemviews_qstandarditemmodel.cpp 4

    You are, of course, not required to use the item-based approach; you could
    instead rely entirely on the QAbstractItemModel interface when working with
    the model, or use a combination of the two as appropriate.

    \sa QStandardItem, {Model/View Programming}, QAbstractItemModel,
    {itemviews/simpletreemodel}{Simple Tree Model example},
    {Item View Convenience Classes}
*/

/*!
    \fn void QStandardItemModel::itemChanged(QStandardItem *item)
    \since 4.2

    This signal is emitted whenever the data of \a item has changed.
*/

/*!
    Constructs a new item model with the given \a parent.
*/
QStandardItemModel::QStandardItemModel(QObject *parent)
    : QAbstractItemModel(*new QStandardItemModelPrivate, parent)
{
    Q_D(QStandardItemModel);
    d->init();
    d->root->d_func()->setModel(this);
}

/*!
    Constructs a new item model that initially has \a rows rows and \a columns
    columns, and that has the given \a parent.
*/
QStandardItemModel::QStandardItemModel(int rows, int columns, QObject *parent)
    : QAbstractItemModel(*new QStandardItemModelPrivate, parent)
{
    Q_D(QStandardItemModel);
    d->init();
    d->root->insertColumns(0, columns);
    d->columnHeaderItems.insert(0, columns, 0);
    d->root->insertRows(0, rows);
    d->rowHeaderItems.insert(0, rows, 0);
    d->root->d_func()->setModel(this);
}

/*!
  \internal
*/
QStandardItemModel::QStandardItemModel(QStandardItemModelPrivate &dd, QObject *parent)
    : QAbstractItemModel(dd, parent)
{
    Q_D(QStandardItemModel);
    d->init();
}

/*!
    Destructs the model. The model destroys all its items.
*/
QStandardItemModel::~QStandardItemModel()
{
}

/*!
    Removes all items (including header items) from the model and sets the
    number of rows and columns to zero.

    \sa removeColumns(), removeRows()
*/
void QStandardItemModel::clear()
{
    Q_D(QStandardItemModel);
    d->root.reset(new QStandardItem);
    d->root->d_func()->setModel(this);
    qDeleteAll(d->columnHeaderItems);
    d->columnHeaderItems.clear();
    qDeleteAll(d->rowHeaderItems);
    d->rowHeaderItems.clear();
    reset();
}

/*!
    \since 4.2

    Returns a pointer to the QStandardItem associated with the given \a index.

    Calling this function is typically the initial step when processing
    QModelIndex-based signals from a view, such as
    QAbstractItemView::activated(). In your slot, you call itemFromIndex(),
    with the QModelIndex carried by the signal as argument, to obtain a
    pointer to the corresponding QStandardItem.

    Note that this function will lazily create an item for the index (using
    itemPrototype()), and set it in the parent item's child table, if no item
    already exists at that index.

    If \a index is an invalid index, this function returns 0.

    \sa indexFromItem()
*/
QStandardItem *QStandardItemModel::itemFromIndex(const QModelIndex &index) const
{
    Q_D(const QStandardItemModel);
    if ((index.row() < 0) || (index.column() < 0) || (index.model() != this))
        return 0;
    QStandardItem *parent = static_cast<QStandardItem*>(index.internalPointer());
    if (parent == 0)
        return 0;
    QStandardItem *item = parent->child(index.row(), index.column());
    // lazy part
    if (item == 0) {
        item = d->createItem();
        parent->d_func()->setChild(index.row(), index.column(), item);
    }
    return item;
}

/*!
    \since 4.2

    Returns the QModelIndex associated with the given \a item.

    Use this function when you want to perform an operation that requires the
    QModelIndex of the item, such as
    QAbstractItemView::scrollTo(). QStandardItem::index() is provided as
    convenience; it is equivalent to calling this function.

    \sa itemFromIndex(), QStandardItem::index()
*/
QModelIndex QStandardItemModel::indexFromItem(const QStandardItem *item) const
{
    if (item && item->d_func()->parent) {
        QPair<int, int> pos = item->d_func()->position();
        return createIndex(pos.first, pos.second, item->d_func()->parent);
    }
    return QModelIndex();
}

/*!
    \since 4.2

    Sets the number of rows in this model to \a rows. If
    this is less than rowCount(), the data in the unwanted rows
    is discarded.

    \sa setColumnCount()
*/
void QStandardItemModel::setRowCount(int rows)
{
    Q_D(QStandardItemModel);
    d->root->setRowCount(rows);
}

/*!
    \since 4.2

    Sets the number of columns in this model to \a columns. If
    this is less than columnCount(), the data in the unwanted columns
    is discarded.

    \sa setRowCount()
*/
void QStandardItemModel::setColumnCount(int columns)
{
    Q_D(QStandardItemModel);
    d->root->setColumnCount(columns);
}

/*!
    \since 4.2

    Sets the item for the given \a row and \a column to \a item. The model
    takes ownership of the item. If necessary, the row count and column count
    are increased to fit the item. The previous item at the given location (if
    there was one) is deleted.

    \sa item()
*/
void QStandardItemModel::setItem(int row, int column, QStandardItem *item)
{
    Q_D(QStandardItemModel);
    d->root->d_func()->setChild(row, column, item, true);
}

/*!
  \fn QStandardItemModel::setItem(int row, QStandardItem *item)
  \overload
*/

/*!
    \since 4.2

    Returns the item for the given \a row and \a column if one has been set;
    otherwise returns 0.

    \sa setItem(), takeItem(), itemFromIndex()
*/
QStandardItem *QStandardItemModel::item(int row, int column) const
{
    Q_D(const QStandardItemModel);
    return d->root->child(row, column);
}

/*!
    \since 4.2

    Returns the model's invisible root item.

    The invisible root item provides access to the model's top-level items
    through the QStandardItem API, making it possible to write functions that
    can treat top-level items and their children in a uniform way; for
    example, recursive functions involving a tree model.

    \note Calling \l{QAbstractItemModel::index()}{index()} on the QStandardItem object
    retrieved from this function is not valid.
*/
QStandardItem *QStandardItemModel::invisibleRootItem() const
{
    Q_D(const QStandardItemModel);
    return d->root.data();
}

/*!
    \since 4.2

    Sets the horizontal header item for \a column to \a item.  The model takes
    ownership of the item. If necessary, the column count is increased to fit
    the item. The previous header item (if there was one) is deleted.

    \sa horizontalHeaderItem(), setHorizontalHeaderLabels(),
    setVerticalHeaderItem()
*/
void QStandardItemModel::setHorizontalHeaderItem(int column, QStandardItem *item)
{
    Q_D(QStandardItemModel);
    if (column < 0)
        return;
    if (columnCount() <= column)
        setColumnCount(column + 1);

    QStandardItem *oldItem = d->columnHeaderItems.at(column);
    if (item == oldItem)
        return;

    if (item) {
        if (item->model() == 0) {
            item->d_func()->setModel(this);
        } else {
            qWarning("QStandardItem::setHorizontalHeaderItem: Ignoring duplicate insertion of item %p",
                     item);
            return;
        }
    }

    if (oldItem)
        oldItem->d_func()->setModel(0);
    delete oldItem;

    d->columnHeaderItems.replace(column, item);
    emit headerDataChanged(Qt::Horizontal, column, column);
}

/*!
    \since 4.2

    Returns the horizontal header item for \a column if one has been set;
    otherwise returns 0.

    \sa setHorizontalHeaderItem(), verticalHeaderItem()
*/
QStandardItem *QStandardItemModel::horizontalHeaderItem(int column) const
{
    Q_D(const QStandardItemModel);
    if ((column < 0) || (column >= columnCount()))
        return 0;
    return d->columnHeaderItems.at(column);
}

/*!
    \since 4.2

    Sets the vertical header item for \a row to \a item.  The model takes
    ownership of the item. If necessary, the row count is increased to fit the
    item. The previous header item (if there was one) is deleted.

    \sa verticalHeaderItem(), setVerticalHeaderLabels(),
    setHorizontalHeaderItem()
*/
void QStandardItemModel::setVerticalHeaderItem(int row, QStandardItem *item)
{
    Q_D(QStandardItemModel);
    if (row < 0)
        return;
    if (rowCount() <= row)
        setRowCount(row + 1);

    QStandardItem *oldItem = d->rowHeaderItems.at(row);
    if (item == oldItem)
        return;

    if (item) {
        if (item->model() == 0) {
            item->d_func()->setModel(this);
        } else {
            qWarning("QStandardItem::setVerticalHeaderItem: Ignoring duplicate insertion of item %p",
                     item);
            return;
        }
    }

    if (oldItem)
        oldItem->d_func()->setModel(0);
    delete oldItem;

    d->rowHeaderItems.replace(row, item);
    emit headerDataChanged(Qt::Vertical, row, row);
}

/*!
    \since 4.2

    Returns the vertical header item for row \a row if one has been set;
    otherwise returns 0.

    \sa setVerticalHeaderItem(), horizontalHeaderItem()
*/
QStandardItem *QStandardItemModel::verticalHeaderItem(int row) const
{
    Q_D(const QStandardItemModel);
    if ((row < 0) || (row >= rowCount()))
        return 0;
    return d->rowHeaderItems.at(row);
}

/*!
    \since 4.2

    Sets the horizontal header labels using \a labels. If necessary, the
    column count is increased to the size of \a labels.

    \sa setHorizontalHeaderItem()
*/
void QStandardItemModel::setHorizontalHeaderLabels(const QStringList &labels)
{
    Q_D(QStandardItemModel);
    if (columnCount() < labels.count())
        setColumnCount(labels.count());
    for (int i = 0; i < labels.count(); ++i) {
        QStandardItem *item = horizontalHeaderItem(i);
        if (!item) {
            item = d->createItem();
            setHorizontalHeaderItem(i, item);
        }
        item->setText(labels.at(i));
    }
}

/*!
    \since 4.2

    Sets the vertical header labels using \a labels. If necessary, the row
    count is increased to the size of \a labels.

    \sa setVerticalHeaderItem()
*/
void QStandardItemModel::setVerticalHeaderLabels(const QStringList &labels)
{
    Q_D(QStandardItemModel);
    if (rowCount() < labels.count())
        setRowCount(labels.count());
    for (int i = 0; i < labels.count(); ++i) {
        QStandardItem *item = verticalHeaderItem(i);
        if (!item) {
            item = d->createItem();
            setVerticalHeaderItem(i, item);
        }
        item->setText(labels.at(i));
    }
}

/*!
    \since 4.2

    Sets the item prototype for the model to the specified \a item. The model
    takes ownership of the prototype.

    The item prototype acts as a QStandardItem factory, by relying on the
    QStandardItem::clone() function.  To provide your own prototype, subclass
    QStandardItem, reimplement QStandardItem::clone() and set the prototype to
    be an instance of your custom class. Whenever QStandardItemModel needs to
    create an item on demand (for instance, when a view or item delegate calls
    setData())), the new items will be instances of your custom class.

    \sa itemPrototype(), QStandardItem::clone()
*/
void QStandardItemModel::setItemPrototype(const QStandardItem *item)
{
    Q_D(QStandardItemModel);
    if (d->itemPrototype != item) {
        delete d->itemPrototype;
        d->itemPrototype = item;
    }
}

/*!
    \since 4.2

    Returns the item prototype used by the model. The model uses the item
    prototype as an item factory when it needs to construct new items on
    demand (for instance, when a view or item delegate calls setData()).

    \sa setItemPrototype()
*/
const QStandardItem *QStandardItemModel::itemPrototype() const
{
    Q_D(const QStandardItemModel);
    return d->itemPrototype;
}

/*!
    \since 4.2

    Returns a list of items that match the given \a text, using the given \a
    flags, in the given \a column.
*/
QList<QStandardItem*> QStandardItemModel::findItems(const QString &text,
                                                    Qt::MatchFlags flags, int column) const
{
    QModelIndexList indexes = match(index(0, column, QModelIndex()),
                                    Qt::DisplayRole, text, -1, flags);
    QList<QStandardItem*> items;
    for (int i = 0; i < indexes.size(); ++i)
        items.append(itemFromIndex(indexes.at(i)));
    return items;
}

/*!
    \since 4.2

    Appends a row containing \a items. If necessary, the column count is
    increased to the size of \a items.

    \sa insertRow(), appendColumn()
*/
void QStandardItemModel::appendRow(const QList<QStandardItem*> &items)
{
    invisibleRootItem()->appendRow(items);
}

/*!
    \since 4.2

    Appends a column containing \a items. If necessary, the row count is
    increased to the size of \a items.

    \sa insertColumn(), appendRow()
*/
void QStandardItemModel::appendColumn(const QList<QStandardItem*> &items)
{
    invisibleRootItem()->appendColumn(items);
}

/*!
    \since 4.2
    \fn QStandardItemModel::appendRow(QStandardItem *item)
    \overload

    When building a list or a tree that has only one column, this function
    provides a convenient way to append a single new \a item.
*/

/*!
    \since 4.2

    Inserts a row at \a row containing \a items. If necessary, the column
    count is increased to the size of \a items.

    \sa takeRow(), appendRow(), insertColumn()
*/
void QStandardItemModel::insertRow(int row, const QList<QStandardItem*> &items)
{
    invisibleRootItem()->insertRow(row, items);
}

/*!
    \since 4.2

    \fn void QStandardItemModel::insertRow(int row, QStandardItem *item)
    \overload

    Inserts a row at \a row containing \a item.

    When building a list or a tree that has only one column, this function
    provides a convenient way to append a single new item.
*/

/*!
    \since 4.2

    Inserts a column at \a column containing \a items. If necessary, the row
    count is increased to the size of \a items.

    \sa takeColumn(), appendColumn(), insertRow()
*/
void QStandardItemModel::insertColumn(int column, const QList<QStandardItem*> &items)
{
    invisibleRootItem()->insertColumn(column, items);
}

/*!
    \since 4.2

    Removes the item at (\a row, \a column) without deleting it. The model
    releases ownership of the item.

    \sa item(), takeRow(), takeColumn()
*/
QStandardItem *QStandardItemModel::takeItem(int row, int column)
{
    Q_D(QStandardItemModel);
    return d->root->takeChild(row, column);
}

/*!
    \since 4.2

    Removes the given \a row without deleting the row items, and returns a
    list of pointers to the removed items. The model releases ownership of the
    items. For items in the row that have not been set, the corresponding
    pointers in the list will be 0.

    \sa takeColumn()
*/
QList<QStandardItem*> QStandardItemModel::takeRow(int row)
{
    Q_D(QStandardItemModel);
    return d->root->takeRow(row);
}

/*!
    \since 4.2

    Removes the given \a column without deleting the column items, and returns
    a list of pointers to the removed items. The model releases ownership of
    the items. For items in the column that have not been set, the
    corresponding pointers in the list will be 0.

    \sa takeRow()
*/
QList<QStandardItem*> QStandardItemModel::takeColumn(int column)
{
    Q_D(QStandardItemModel);
    return d->root->takeColumn(column);
}

/*!
    \since 4.2

    Removes the horizontal header item at \a column from the header without
    deleting it, and returns a pointer to the item. The model releases
    ownership of the item.

    \sa horizontalHeaderItem(), takeVerticalHeaderItem()
*/
QStandardItem *QStandardItemModel::takeHorizontalHeaderItem(int column)
{
    Q_D(QStandardItemModel);
    if ((column < 0) || (column >= columnCount()))
        return 0;
    QStandardItem *headerItem = d->columnHeaderItems.at(column);
    if (headerItem) {
        headerItem->d_func()->setParentAndModel(0, 0);
        d->columnHeaderItems.replace(column, 0);
    }
    return headerItem;
}

/*!
    \since 4.2

    Removes the vertical header item at \a row from the header without
    deleting it, and returns a pointer to the item. The model releases
    ownership of the item.

    \sa verticalHeaderItem(), takeHorizontalHeaderItem()
*/
QStandardItem *QStandardItemModel::takeVerticalHeaderItem(int row)
{
    Q_D(QStandardItemModel);
    if ((row < 0) || (row >= rowCount()))
        return 0;
    QStandardItem *headerItem = d->rowHeaderItems.at(row);
    if (headerItem) {
        headerItem->d_func()->setParentAndModel(0, 0);
        d->rowHeaderItems.replace(row, 0);
    }
    return headerItem;
}

/*!
    \since 4.2
    \property QStandardItemModel::sortRole
    \brief the item role that is used to query the model's data when sorting items

    The default value is Qt::DisplayRole.

    \sa sort(), QStandardItem::sortChildren()
*/
int QStandardItemModel::sortRole() const
{
    Q_D(const QStandardItemModel);
    return d->sortRole;
}

void QStandardItemModel::setSortRole(int role)
{
    Q_D(QStandardItemModel);
    d->sortRole = role;
}

/*!
  \reimp
*/
int QStandardItemModel::columnCount(const QModelIndex &parent) const
{
    Q_D(const QStandardItemModel);
    QStandardItem *item = d->itemFromIndex(parent);
    return item ? item->columnCount() : 0;
}

/*!
  \reimp
*/
QVariant QStandardItemModel::data(const QModelIndex &index, int role) const
{
    Q_D(const QStandardItemModel);
    QStandardItem *item = d->itemFromIndex(index);
    return item ? item->data(role) : QVariant();
}

/*!
  \reimp
*/
Qt::ItemFlags QStandardItemModel::flags(const QModelIndex &index) const
{
    Q_D(const QStandardItemModel);
    if (!d->indexValid(index))
        return d->root->flags();
    QStandardItem *item = d->itemFromIndex(index);
    if (item)
        return item->flags();
    return Qt::ItemIsSelectable
        |Qt::ItemIsEnabled
        |Qt::ItemIsEditable
        |Qt::ItemIsDragEnabled
        |Qt::ItemIsDropEnabled;
}

/*!
  \reimp
*/
bool QStandardItemModel::hasChildren(const QModelIndex &parent) const
{
    Q_D(const QStandardItemModel);
    QStandardItem *item = d->itemFromIndex(parent);
    return item ? item->hasChildren() : false;
}

/*!
  \reimp
*/
QVariant QStandardItemModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    Q_D(const QStandardItemModel);
    if ((section < 0)
        || ((orientation == Qt::Horizontal) && (section >= columnCount()))
        || ((orientation == Qt::Vertical) && (section >= rowCount()))) {
        return QVariant();
    }
    QStandardItem *headerItem = 0;
    if (orientation == Qt::Horizontal)
        headerItem = d->columnHeaderItems.at(section);
    else if (orientation == Qt::Vertical)
        headerItem = d->rowHeaderItems.at(section);
    return headerItem ? headerItem->data(role)
        : QAbstractItemModel::headerData(section, orientation, role);
}

/*!
    \reimp

    QStandardItemModel supports both copy and move.
*/
Qt::DropActions QStandardItemModel::supportedDropActions () const
{
    return Qt::CopyAction | Qt::MoveAction;
}

/*!
  \reimp
*/
QModelIndex QStandardItemModel::index(int row, int column, const QModelIndex &parent) const
{
    Q_D(const QStandardItemModel);
    QStandardItem *parentItem = d->itemFromIndex(parent);
    if ((parentItem == 0)
        || (row < 0)
        || (column < 0)
        || (row >= parentItem->rowCount())
        || (column >= parentItem->columnCount())) {
        return QModelIndex();
    }
    return createIndex(row, column, parentItem);
}

/*!
  \reimp
*/
bool QStandardItemModel::insertColumns(int column, int count, const QModelIndex &parent)
{
    Q_D(QStandardItemModel);
    QStandardItem *item = parent.isValid() ? itemFromIndex(parent) : d->root.data();
    if (item == 0)
        return false;
    return item->d_func()->insertColumns(column, count, QList<QStandardItem*>());
}

/*!
  \reimp
*/
bool QStandardItemModel::insertRows(int row, int count, const QModelIndex &parent)
{
    Q_D(QStandardItemModel);
    QStandardItem *item = parent.isValid() ? itemFromIndex(parent) : d->root.data();
    if (item == 0)
        return false;
    return item->d_func()->insertRows(row, count, QList<QStandardItem*>());
}

/*!
  \reimp
*/
QMap<int, QVariant> QStandardItemModel::itemData(const QModelIndex &index) const
{
    Q_D(const QStandardItemModel);
    QStandardItem *item = d->itemFromIndex(index);
    return item ? item->d_func()->itemData() : QMap<int, QVariant>();
}

/*!
  \reimp
*/
QModelIndex QStandardItemModel::parent(const QModelIndex &child) const
{
    Q_D(const QStandardItemModel);
    if (!d->indexValid(child))
        return QModelIndex();
    QStandardItem *parentItem = static_cast<QStandardItem*>(child.internalPointer());
    return indexFromItem(parentItem);
}

/*!
  \reimp
*/
bool QStandardItemModel::removeColumns(int column, int count, const QModelIndex &parent)
{
    Q_D(QStandardItemModel);
    QStandardItem *item = d->itemFromIndex(parent);
    if ((item == 0) || (count < 1) || (column < 0) || ((column + count) > item->columnCount()))
        return false;
    item->removeColumns(column, count);
    return true;
}

/*!
  \reimp
*/
bool QStandardItemModel::removeRows(int row, int count, const QModelIndex &parent)
{
    Q_D(QStandardItemModel);
    QStandardItem *item = d->itemFromIndex(parent);
    if ((item == 0) || (count < 1) || (row < 0) || ((row + count) > item->rowCount()))
        return false;
    item->removeRows(row, count);
    return true;
}

/*!
  \reimp
*/
int QStandardItemModel::rowCount(const QModelIndex &parent) const
{
    Q_D(const QStandardItemModel);
    QStandardItem *item = d->itemFromIndex(parent);
    return item ? item->rowCount() : 0;
}

/*!
  \reimp
*/
bool QStandardItemModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!index.isValid())
        return false;
    QStandardItem *item = itemFromIndex(index);
    if (item == 0)
        return false;
    item->setData(value, role);
    return true;
}

/*!
  \reimp
*/
bool QStandardItemModel::setHeaderData(int section, Qt::Orientation orientation, const QVariant &value, int role)
{
    Q_D(QStandardItemModel);
    if ((section < 0)
        || ((orientation == Qt::Horizontal) && (section >= columnCount()))
        || ((orientation == Qt::Vertical) && (section >= rowCount()))) {
        return false;
    }
    QStandardItem *headerItem = 0;
    if (orientation == Qt::Horizontal) {
        headerItem = d->columnHeaderItems.at(section);
        if (headerItem == 0) {
            headerItem = d->createItem();
            headerItem->d_func()->setModel(this);
            d->columnHeaderItems.replace(section, headerItem);
        }
    } else if (orientation == Qt::Vertical) {
        headerItem = d->rowHeaderItems.at(section);
        if (headerItem == 0) {
            headerItem = d->createItem();
            headerItem->d_func()->setModel(this);
            d->rowHeaderItems.replace(section, headerItem);
        }
    }
    if (headerItem) {
        headerItem->setData(value, role);
        return true;
    }
    return false;
}

/*!
  \reimp
*/
bool QStandardItemModel::setItemData(const QModelIndex &index, const QMap<int, QVariant> &roles)
{
    QStandardItem *item = itemFromIndex(index);
    if (item == 0)
        return false;
    item->d_func()->setItemData(roles);
    return true;
}

/*!
  \reimp
*/
void QStandardItemModel::sort(int column, Qt::SortOrder order)
{
    Q_D(QStandardItemModel);
    d->root->sortChildren(column, order);
}

/*!
  \fn QObject *QStandardItemModel::parent() const
  \internal
*/


/*!
  \reimp
*/
QStringList QStandardItemModel::mimeTypes() const
{
    return QAbstractItemModel::mimeTypes() <<  QLatin1String("application/x-qstandarditemmodeldatalist");
}

/*!
  \reimp
*/
QMimeData *QStandardItemModel::mimeData(const QModelIndexList &indexes) const
{
    QMimeData *data = QAbstractItemModel::mimeData(indexes);
    if(!data)
        return 0;

    QString format = QLatin1String("application/x-qstandarditemmodeldatalist");
    if (!mimeTypes().contains(format))
        return data;
    QByteArray encoded;
    QDataStream stream(&encoded, QIODevice::WriteOnly);
    
    QSet<QStandardItem*> itemsSet;
    QStack<QStandardItem*> stack;
    itemsSet.reserve(indexes.count());
    stack.reserve(indexes.count());
    for (int i = 0; i < indexes.count(); ++i) {
        QStandardItem *item = itemFromIndex(indexes.at(i));
        itemsSet << item;
        stack.push(item);
    }
    
    //remove duplicates childrens
    {
        QSet<QStandardItem *> seen;
        while (!stack.isEmpty()) {
            QStandardItem *itm = stack.pop();
            if (seen.contains(itm))
                continue;
            seen.insert(itm);
            
            const QVector<QStandardItem*> &childList = itm->d_func()->children;
            for (int i = 0; i < childList.count(); ++i) {
                QStandardItem *chi = childList.at(i);
                if (chi) {
                    QSet<QStandardItem *>::iterator it = itemsSet.find(chi);
                    if (it != itemsSet.end()) {
                        itemsSet.erase(it);
                    }
                    stack.push(chi);
                }
            }
        }
    }
    
    stack.reserve(itemsSet.count());
    foreach (QStandardItem *item, itemsSet) {
        stack.push(item);
    }
    
    //stream everything recursively
    while (!stack.isEmpty()) {
        QStandardItem *item = stack.pop();
        if(itemsSet.contains(item)) { //if the item is selection 'top-level', strem its position
            stream << item->row() << item->column(); 
        }
        if(item) {
            stream << *item << item->columnCount() << item->d_ptr->children.count();
            stack += item->d_ptr->children;
        } else {
            QStandardItem dummy;
            stream << dummy << 0 << 0;
        }
    }

    data->setData(format, encoded);
    return data;
}


/* \internal
    Used by QStandardItemModel::dropMimeData
    stream out an item and his children 
 */
void QStandardItemModelPrivate::decodeDataRecursive(QDataStream &stream, QStandardItem *item)
{
    int colCount, childCount;
    stream >> *item;
    stream >> colCount >> childCount;
    item->setColumnCount(colCount);
    
    int childPos = childCount;
    
    while(childPos > 0) {
        childPos--;
        QStandardItem *child = createItem();
        decodeDataRecursive(stream, child);
        item->setChild( childPos / colCount, childPos % colCount, child);
    }
}


/*!
  \reimp
*/
bool QStandardItemModel::dropMimeData(const QMimeData *data, Qt::DropAction action,
                                      int row, int column, const QModelIndex &parent)
{
    Q_D(QStandardItemModel);
    // check if the action is supported
    if (!data || !(action == Qt::CopyAction || action == Qt::MoveAction))
        return false;
    // check if the format is supported
    QString format = QLatin1String("application/x-qstandarditemmodeldatalist");
    if (!data->hasFormat(format))
        return QAbstractItemModel::dropMimeData(data, action, row, column, parent);

    if (row > rowCount(parent))
        row = rowCount(parent);
    if (row == -1)
        row = rowCount(parent);
    if (column == -1)
        column = 0;

    // decode and insert
    QByteArray encoded = data->data(format);
    QDataStream stream(&encoded, QIODevice::ReadOnly);


    //code based on QAbstractItemModel::decodeData
    // adapted to work with QStandardItem
    int top = INT_MAX;
    int left = INT_MAX;
    int bottom = 0;
    int right = 0;
    QVector<int> rows, columns;
    QVector<QStandardItem *> items;

    while (!stream.atEnd()) {
        int r, c;
        QStandardItem *item = d->createItem();
        stream >> r >> c;
        d->decodeDataRecursive(stream, item);

        rows.append(r);
        columns.append(c);
        items.append(item);
        top = qMin(r, top);
        left = qMin(c, left);
        bottom = qMax(r, bottom);
        right = qMax(c, right);
    }

    // insert the dragged items into the table, use a bit array to avoid overwriting items,
    // since items from different tables can have the same row and column
    int dragRowCount = 0;
    int dragColumnCount = right - left + 1;

    // Compute the number of continuous rows upon insertion and modify the rows to match
    QVector<int> rowsToInsert(bottom + 1);
    for (int i = 0; i < rows.count(); ++i)
        rowsToInsert[rows.at(i)] = 1;
    for (int i = 0; i < rowsToInsert.count(); ++i) {
        if (rowsToInsert[i] == 1){
            rowsToInsert[i] = dragRowCount;
            ++dragRowCount;
        }
    }
    for (int i = 0; i < rows.count(); ++i)
        rows[i] = top + rowsToInsert[rows[i]];

    QBitArray isWrittenTo(dragRowCount * dragColumnCount);

    // make space in the table for the dropped data
    int colCount = columnCount(parent);
    if (colCount < dragColumnCount + column) {
        insertColumns(colCount, dragColumnCount + column - colCount, parent);
        colCount = columnCount(parent);
    }
    insertRows(row, dragRowCount, parent);

    row = qMax(0, row);
    column = qMax(0, column);

    QStandardItem *parentItem = itemFromIndex (parent);
    if (!parentItem)
        parentItem = invisibleRootItem();

    QVector<QPersistentModelIndex> newIndexes(items.size());
    // set the data in the table
    for (int j = 0; j < items.size(); ++j) {
        int relativeRow = rows.at(j) - top;
        int relativeColumn = columns.at(j) - left;
        int destinationRow = relativeRow + row;
        int destinationColumn = relativeColumn + column;
        int flat = (relativeRow * dragColumnCount) + relativeColumn;
        // if the item was already written to, or we just can't fit it in the table, create a new row
        if (destinationColumn >= colCount || isWrittenTo.testBit(flat)) {
            destinationColumn = qBound(column, destinationColumn, colCount - 1);
            destinationRow = row + dragRowCount;
            insertRows(row + dragRowCount, 1, parent);
            flat = (dragRowCount * dragColumnCount) + relativeColumn;
            isWrittenTo.resize(++dragRowCount * dragColumnCount);
        }
        if (!isWrittenTo.testBit(flat)) {
            newIndexes[j] = index(destinationRow, destinationColumn, parentItem->index());
            isWrittenTo.setBit(flat);
        }
    }

    for(int k = 0; k < newIndexes.size(); k++) {
        if (newIndexes.at(k).isValid()) {
            parentItem->setChild(newIndexes.at(k).row(), newIndexes.at(k).column(), items.at(k));
        } else {
            delete items.at(k);
        }
    }

    return true;
}

QT_END_NAMESPACE

#include "moc_qstandarditemmodel.cpp"

#endif // QT_NO_STANDARDITEMMODEL
