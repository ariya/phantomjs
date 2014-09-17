/****************************************************************************
**
** Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtGui module of the Qt Toolkit.
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

#include <private/qtreewidgetitemiterator_p.h>
#include "qtreewidget.h"
#include "qtreewidget_p.h"
#include "qwidgetitemdata_p.h"

#ifndef QT_NO_TREEWIDGET

QT_BEGIN_NAMESPACE

/*!
  \class QTreeWidgetItemIterator
  \ingroup model-view
  \brief The QTreeWidgetItemIterator class provides a way to iterate over the
  items in a QTreeWidget instance.

  The iterator will walk the items in a pre-order traversal order, thus visiting the
  parent node \e before it continues to the child nodes.

  For example, the following code examples each item in a tree, checking the
  text in the first column against a user-specified search string:

  \snippet doc/src/snippets/qtreewidgetitemiterator-using/mainwindow.cpp 0

  It is also possible to filter out certain types of node by passing certain
  \l{IteratorFlag}{flags} to the constructor of QTreeWidgetItemIterator.

  \sa QTreeWidget, {Model/View Programming}, QTreeWidgetItem
*/

/*!
    Constructs an iterator for the same QTreeWidget as \a it. The
    current iterator item is set to point on the current item of \a it.
*/

QTreeWidgetItemIterator::QTreeWidgetItemIterator(const QTreeWidgetItemIterator &it)
    :  d_ptr(new QTreeWidgetItemIteratorPrivate(*(it.d_ptr))),
    current(it.current), flags(it.flags)
{
    Q_D(QTreeWidgetItemIterator);
    Q_ASSERT(d->m_model);
    d->m_model->iterators.append(this);
}

/*!
    Constructs an iterator for the given \a widget that uses the specified \a flags
    to determine which items are found during iteration.
    The iterator is set to point to the first top-level item contained in the widget,
    or the next matching item if the top-level item doesn't match the flags.

    \sa QTreeWidgetItemIterator::IteratorFlag
*/

QTreeWidgetItemIterator::QTreeWidgetItemIterator(QTreeWidget *widget, IteratorFlags flags)
: current(0), flags(flags)
{
    Q_ASSERT(widget);
    QTreeModel *model = qobject_cast<QTreeModel*>(widget->model());
    Q_ASSERT(model);
    d_ptr.reset(new QTreeWidgetItemIteratorPrivate(this, model));
    model->iterators.append(this);
    if (!model->rootItem->children.isEmpty()) current = model->rootItem->children.first();
    if (current && !matchesFlags(current))
        ++(*this);
}

/*!
    Constructs an iterator for the given \a item that uses the specified \a flags
    to determine which items are found during iteration.
    The iterator is set to point to \a item, or the next matching item if \a item
    doesn't match the flags.

    \sa QTreeWidgetItemIterator::IteratorFlag
*/

QTreeWidgetItemIterator::QTreeWidgetItemIterator(QTreeWidgetItem *item, IteratorFlags flags)
    : d_ptr(new QTreeWidgetItemIteratorPrivate(
                this, qobject_cast<QTreeModel*>(item->view->model()))),
      current(item), flags(flags)
{
    Q_D(QTreeWidgetItemIterator);
    Q_ASSERT(item);
    QTreeModel *model = qobject_cast<QTreeModel*>(item->view->model());
    Q_ASSERT(model);
    model->iterators.append(this);

    // Initialize m_currentIndex and m_parentIndex as it would be if we had traversed from
    // the beginning.
    QTreeWidgetItem *parent = item;
    parent = parent->parent();
    QList<QTreeWidgetItem *> children = parent ? parent->children : d->m_model->rootItem->children;
    d->m_currentIndex = children.indexOf(item);

    while (parent) {
        QTreeWidgetItem *itm = parent;
        parent = parent->parent();
        QList<QTreeWidgetItem *> children = parent ? parent->children : d->m_model->rootItem->children;
        int index = children.indexOf(itm);
        d->m_parentIndex.prepend(index);
    }

    if (current && !matchesFlags(current))
        ++(*this);
}

/*!
    Destroys the iterator.
*/

QTreeWidgetItemIterator::~QTreeWidgetItemIterator()
{
    d_func()->m_model->iterators.removeAll(this);
}

/*!
    Assignment. Makes a copy of \a it and returns a reference to its
    iterator.
*/

QTreeWidgetItemIterator &QTreeWidgetItemIterator::operator=(const QTreeWidgetItemIterator &it)
{
    Q_D(QTreeWidgetItemIterator);
    if (d_func()->m_model != it.d_func()->m_model) {
        d_func()->m_model->iterators.removeAll(this);
        it.d_func()->m_model->iterators.append(this);
    }
    current = it.current;
    flags = it.flags;
    d->operator=(*it.d_func());
    return *this;
}

/*!
    The prefix ++ operator (++it) advances the iterator to the next matching item
    and returns a reference to the resulting iterator.
    Sets the current pointer to 0 if the current item is the last matching item.
*/

QTreeWidgetItemIterator &QTreeWidgetItemIterator::operator++()
{
    if (current)
        do {
            current = d_func()->next(current);
        } while (current && !matchesFlags(current));
    return *this;
}

/*!
    The prefix -- operator (--it) advances the iterator to the previous matching item
    and returns a reference to the resulting iterator.
    Sets the current pointer to 0 if the current item is the first matching item.
*/

QTreeWidgetItemIterator &QTreeWidgetItemIterator::operator--()
{
    if (current)
        do {
            current = d_func()->previous(current);
        } while (current && !matchesFlags(current));
    return *this;
}

/*!
  \internal
*/
bool QTreeWidgetItemIterator::matchesFlags(const QTreeWidgetItem *item) const
{
    if (!item)
        return false;

    if (flags == All)
        return true;

    {
        Qt::ItemFlags itemFlags = item->flags();
        if ((flags & Selectable) && !(itemFlags & Qt::ItemIsSelectable))
            return false;
        if ((flags & NotSelectable) && (itemFlags & Qt::ItemIsSelectable))
            return false;
        if ((flags & DragEnabled) && !(itemFlags & Qt::ItemIsDragEnabled))
            return false;
        if ((flags & DragDisabled) && (itemFlags & Qt::ItemIsDragEnabled))
            return false;
        if ((flags & DropEnabled) && !(itemFlags & Qt::ItemIsDropEnabled))
            return false;
        if ((flags & DropDisabled) && (itemFlags & Qt::ItemIsDropEnabled))
            return false;
        if ((flags & Enabled) && !(itemFlags & Qt::ItemIsEnabled))
            return false;
        if ((flags & Disabled) && (itemFlags & Qt::ItemIsEnabled))
            return false;
        if ((flags & Editable) && !(itemFlags & Qt::ItemIsEditable))
            return false;
        if ((flags & NotEditable) && (itemFlags & Qt::ItemIsEditable))
            return false;
    }

    if (flags & (Checked|NotChecked)) {
        // ### We only test the check state for column 0
        Qt::CheckState check = item->checkState(0);
        // PartiallyChecked matches as Checked.
        if ((flags & Checked) && (check == Qt::Unchecked))
            return false;
        if ((flags & NotChecked) && (check != Qt::Unchecked))
            return false;
    }

    if ((flags & HasChildren) && !item->childCount())
        return false;
    if ((flags & NoChildren) && item->childCount())
        return false;

    if ((flags & Hidden) && !item->isHidden())
        return false;
    if ((flags & NotHidden) && item->isHidden())
        return false;

    if ((flags & Selected) && !item->isSelected())
        return false;
    if ((flags & Unselected) && item->isSelected())
        return false;

    return true;
}

/*
 * Implementation of QTreeWidgetItemIteratorPrivate
 */
QTreeWidgetItem* QTreeWidgetItemIteratorPrivate::nextSibling(const QTreeWidgetItem* item) const
{
    Q_ASSERT(item);
    QTreeWidgetItem *next = 0;
    if (QTreeWidgetItem *par = item->parent()) {
        int i = par->indexOfChild(const_cast<QTreeWidgetItem*>(item));
        next = par->child(i + 1);
    } else {
        QTreeWidget *tw = item->treeWidget();
        int i = tw->indexOfTopLevelItem(const_cast<QTreeWidgetItem*>(item));
        next = tw->topLevelItem(i + 1);
    }
    return next;
}

QTreeWidgetItem *QTreeWidgetItemIteratorPrivate::next(const QTreeWidgetItem *current)
{
    if (!current) return 0;

    QTreeWidgetItem *next = 0;
    if (current->childCount()) {
        // walk the child
        m_parentIndex.push(m_currentIndex);
        m_currentIndex = 0;
        next = current->child(0);
    } else {
        // walk the sibling
        QTreeWidgetItem *parent = current->parent();
        next = parent ? parent->child(m_currentIndex + 1)
                      : m_model->rootItem->child(m_currentIndex + 1);
        while (!next && parent) {
            // if we had no sibling walk up the parent and try the sibling of that
            parent = parent->parent();
            m_currentIndex = m_parentIndex.pop();
            next = parent ? parent->child(m_currentIndex + 1)
                          : m_model->rootItem->child(m_currentIndex + 1);
        }
        if (next) ++(m_currentIndex);
    }
    return next;
}

QTreeWidgetItem *QTreeWidgetItemIteratorPrivate::previous(const QTreeWidgetItem *current)
{
    if (!current) return 0;

    QTreeWidgetItem *prev = 0;
    // walk the previous sibling
    QTreeWidgetItem *parent = current->parent();
    prev = parent ? parent->child(m_currentIndex - 1)
                  : m_model->rootItem->child(m_currentIndex - 1);
    if (prev) {
        // Yes, we had a previous sibling but we need go down to the last leafnode.
        --m_currentIndex;
        while (prev && prev->childCount()) {
            m_parentIndex.push(m_currentIndex);
            m_currentIndex = prev->childCount() - 1;
            prev = prev->child(m_currentIndex);
        }
    } else if (parent) {
        m_currentIndex = m_parentIndex.pop();
        prev = parent;
    }
    return prev;
}

void QTreeWidgetItemIteratorPrivate::ensureValidIterator(const QTreeWidgetItem *itemToBeRemoved)
{
    Q_Q(QTreeWidgetItemIterator);
    Q_ASSERT(itemToBeRemoved);

    if (!q->current) return;
    QTreeWidgetItem *nextItem = q->current;

    // Do not walk to the ancestor to find the other item if they have the same parent.
    if (nextItem->parent() != itemToBeRemoved->parent()) {
        while (nextItem->parent() && nextItem != itemToBeRemoved) {
            nextItem = nextItem->parent();
        }
    }
    // If the item to be removed is an ancestor of the current iterator item,
    // we need to adjust the iterator.
    if (nextItem == itemToBeRemoved) {
        QTreeWidgetItem *parent = nextItem;
        nextItem = 0;
        while (parent && !nextItem) {
            nextItem = nextSibling(parent);
            parent = parent->parent();
        }
        if (nextItem) {
            // Ooooh... Set the iterator to the next valid item
            *q = QTreeWidgetItemIterator(nextItem, q->flags);
            if (!(q->matchesFlags(nextItem))) ++(*q);
        } else {
            // set it to null.
            q->current = 0;
            m_parentIndex.clear();
            return;
        }
    }
    if (nextItem->parent() == itemToBeRemoved->parent()) {
        // They have the same parent, i.e. we have to adjust the m_currentIndex member of the iterator
        // if the deleted item is to the left of the nextItem.

        QTreeWidgetItem *par = itemToBeRemoved->parent();   // We know they both have the same parent.
        QTreeWidget *tw = itemToBeRemoved->treeWidget();    // ..and widget
        int indexOfItemToBeRemoved = par ? par->indexOfChild(const_cast<QTreeWidgetItem *>(itemToBeRemoved))
            : tw->indexOfTopLevelItem(const_cast<QTreeWidgetItem *>(itemToBeRemoved));
        int indexOfNextItem = par ? par->indexOfChild(nextItem) : tw->indexOfTopLevelItem(nextItem);

        if (indexOfItemToBeRemoved <= indexOfNextItem) {
            // A sibling to the left of us was deleted, adjust the m_currentIndex member of the iterator.
            // Note that the m_currentIndex will be wrong until the item is actually removed!
            m_currentIndex--;
        }
    }
}

/*!
  \fn const QTreeWidgetItemIterator QTreeWidgetItemIterator::operator++(int)

  The postfix ++ operator (it++) advances the iterator to the next matching item
  and returns an iterator to the previously current item.
*/

/*!
  \fn QTreeWidgetItemIterator &QTreeWidgetItemIterator::operator+=(int n)

  Makes the iterator go forward by \a n matching items. (If n is negative, the
  iterator goes backward.)

  If the current item is beyond the last item, the current item pointer is
  set to 0. Returns the resulting iterator.
*/

/*!
  \fn const QTreeWidgetItemIterator QTreeWidgetItemIterator::operator--(int)

  The postfix -- operator (it--) makes the preceding matching item current and returns an iterator to the previously current item.
*/

/*!
  \fn QTreeWidgetItemIterator &QTreeWidgetItemIterator::operator-=(int n)

  Makes the iterator go backward by \a n matching items. (If n is negative, the
  iterator goes forward.)

  If the current item is ahead of the last item, the current item pointer is
  set to 0. Returns the resulting iterator.
*/

/*!
  \fn QTreeWidgetItem *QTreeWidgetItemIterator::operator*() const

  Dereference operator. Returns a pointer to the current item.
*/


/*!
    \enum QTreeWidgetItemIterator::IteratorFlag

    These flags can be passed to a QTreeWidgetItemIterator constructor
    (OR-ed together if more than one is used), so that the iterator
    will only iterate over items that match the given flags.

    \value All
    \value Hidden
    \value NotHidden
    \value Selected
    \value Unselected
    \value Selectable
    \value NotSelectable
    \value DragEnabled
    \value DragDisabled
    \value DropEnabled
    \value DropDisabled
    \value HasChildren
    \value NoChildren
    \value Checked
    \value NotChecked
    \value Enabled
    \value Disabled
    \value Editable
    \value NotEditable
    \value UserFlag
*/

QT_END_NAMESPACE

#endif // QT_NO_TREEWIDGET
