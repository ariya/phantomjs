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

#include "qitemselectionmodel.h"
#include <private/qitemselectionmodel_p.h>
#include <qdebug.h>

#ifndef QT_NO_ITEMVIEWS

QT_BEGIN_NAMESPACE

/*!
    \class QItemSelectionRange

    \brief The QItemSelectionRange class manages information about a
    range of selected items in a model.

    \ingroup model-view

    A QItemSelectionRange contains information about a range of
    selected items in a model. A range of items is a contiguous array
    of model items, extending to cover a number of adjacent rows and
    columns with a common parent item; this can be visualized as a
    two-dimensional block of cells in a table. A selection range has a
    top(), left() a bottom(), right() and a parent().

    The QItemSelectionRange class is one of the \l{Model/View Classes}
    and is part of Qt's \l{Model/View Programming}{model/view framework}.

    The model items contained in the selection range can be obtained
    using the indexes() function. Use QItemSelectionModel::selectedIndexes()
    to get a list of all selected items for a view.

    You can determine whether a given model item lies within a
    particular range by using the contains() function. Ranges can also
    be compared using the overloaded operators for equality and
    inequality, and the intersects() function allows you to determine
    whether two ranges overlap.

    \sa {Model/View Programming}, QAbstractItemModel, QItemSelection,
        QItemSelectionModel
*/

/*!
    \fn QItemSelectionRange::QItemSelectionRange()

    Constructs an empty selection range.
*/

/*!
    \fn QItemSelectionRange::QItemSelectionRange(const QItemSelectionRange &other)

    Copy constructor. Constructs a new selection range with the same contents
    as the \a other range given.

*/

/*!
    \fn QItemSelectionRange::QItemSelectionRange(const QModelIndex &topLeft, const QModelIndex &bottomRight)

    Constructs a new selection range containing only the index specified
    by the \a topLeft and the index \a bottomRight.

*/

/*!
    \fn QItemSelectionRange::QItemSelectionRange(const QModelIndex &index)

    Constructs a new selection range containing only the model item specified
    by the model index \a index.
*/

/*!
    \fn int QItemSelectionRange::top() const

    Returns the row index corresponding to the uppermost selected row in the
    selection range.

*/

/*!
    \fn int QItemSelectionRange::left() const

    Returns the column index corresponding to the leftmost selected column in the
    selection range.
*/

/*!
    \fn int QItemSelectionRange::bottom() const

    Returns the row index corresponding to the lowermost selected row in the
    selection range.

*/

/*!
    \fn int QItemSelectionRange::right() const

    Returns the column index corresponding to the rightmost selected column in
    the selection range.

*/

/*!
    \fn int QItemSelectionRange::width() const

    Returns the number of selected columns in the selection range.

*/

/*!
    \fn int QItemSelectionRange::height() const

    Returns the number of selected rows in the selection range.

*/

/*!
    \fn const QAbstractItemModel *QItemSelectionRange::model() const

    Returns the model that the items in the selection range belong to.
*/

/*!
    \fn QModelIndex QItemSelectionRange::topLeft() const

    Returns the index for the item located at the top-left corner of
    the selection range.

    \sa top(), left(), bottomRight()
*/

/*!
    \fn QModelIndex QItemSelectionRange::bottomRight() const

    Returns the index for the item located at the bottom-right corner
    of the selection range.

    \sa bottom(), right(), topLeft()
*/

/*!
    \fn QModelIndex QItemSelectionRange::parent() const

    Returns the parent model item index of the items in the selection range.

*/

/*!
    \fn bool QItemSelectionRange::contains(const QModelIndex &index) const

    Returns true if the model item specified by the \a index lies within the
    range of selected items; otherwise returns false.
*/

/*!
    \fn bool QItemSelectionRange::contains(int row, int column,
                                           const QModelIndex &parentIndex) const
    \overload

    Returns true if the model item specified by (\a row, \a column)
    and with \a parentIndex as the parent item lies within the range
    of selected items; otherwise returns false.
*/

/*!
    \fn bool QItemSelectionRange::intersects(const QItemSelectionRange &other) const

    Returns true if this selection range intersects (overlaps with) the \a other
    range given; otherwise returns false.

*/
bool QItemSelectionRange::intersects(const QItemSelectionRange &other) const
{
    return (isValid() && other.isValid()
            && parent() == other.parent()
            && model() == other.model()
            && ((top() <= other.top() && bottom() >= other.top())
                || (top() >= other.top() && top() <= other.bottom()))
            && ((left() <= other.left() && right() >= other.left())
                || (left() >= other.left() && left() <= other.right())));
}

/*!
    \fn QItemSelectionRange QItemSelectionRange::intersect(const QItemSelectionRange &other) const
    \obsolete

    Use intersected(\a other) instead.
*/

/*!
    \fn QItemSelectionRange QItemSelectionRange::intersected(const QItemSelectionRange &other) const
    \since 4.2

    Returns a new selection range containing only the items that are found in
    both the selection range and the \a other selection range.
*/

QItemSelectionRange QItemSelectionRange::intersect(const QItemSelectionRange &other) const
{
    if (model() == other.model() && parent() == other.parent()) {
        QModelIndex topLeft = model()->index(qMax(top(), other.top()),
                                             qMax(left(), other.left()),
                                             other.parent());
        QModelIndex bottomRight = model()->index(qMin(bottom(), other.bottom()),
                                                 qMin(right(), other.right()),
                                                 other.parent());
        return QItemSelectionRange(topLeft, bottomRight);
    }
    return QItemSelectionRange();
}

/*!
    \fn bool QItemSelectionRange::operator==(const QItemSelectionRange &other) const

    Returns true if the selection range is exactly the same as the \a other
    range given; otherwise returns false.

*/

/*!
    \fn bool QItemSelectionRange::operator!=(const QItemSelectionRange &other) const

    Returns true if the selection range differs from the \a other range given;
    otherwise returns false.

*/

/*!
    \fn bool QItemSelectionRange::isValid() const

    Returns true if the selection range is valid; otherwise returns false.

*/

/*
  \internal

  utility function for getting the indexes from a range
  it avoid concatenating list and works on one
 */

static void indexesFromRange(const QItemSelectionRange &range, QModelIndexList &result)
{
    if (range.isValid() && range.model()) {
        for (int column = range.left(); column <= range.right(); ++column) {
            for (int row = range.top(); row <= range.bottom(); ++row) {
                QModelIndex index = range.model()->index(row, column, range.parent());
                Qt::ItemFlags flags = range.model()->flags(index);
                if ((flags & Qt::ItemIsSelectable) && (flags & Qt::ItemIsEnabled))
                    result.append(index);
            }
        }
    }
}

/*!
    Returns true if the selection range contains no selectable item
    \since 4.7
*/

bool QItemSelectionRange::isEmpty() const
{
    if (!isValid() || !model())
        return true;

    for (int column = left(); column <= right(); ++column) {
        for (int row = top(); row <= bottom(); ++row) {
            QModelIndex index = model()->index(row, column, parent());
            Qt::ItemFlags flags = model()->flags(index);
            if ((flags & Qt::ItemIsSelectable) && (flags & Qt::ItemIsEnabled))
                return false;
        }
    }
    return true;
}

/*!
    Returns the list of model index items stored in the selection.
*/

QModelIndexList QItemSelectionRange::indexes() const
{
    QModelIndexList result;
    indexesFromRange(*this, result);
    return result;
}

/*!
    \class QItemSelection

    \brief The QItemSelection class manages information about selected items in a model.

    \ingroup model-view

    A QItemSelection describes the items in a model that have been
    selected by the user. A QItemSelection is basically a list of
    selection ranges, see QItemSelectionRange. It provides functions for
    creating and manipulating selections, and selecting a range of items
    from a model.

    The QItemSelection class is one of the \l{Model/View Classes}
    and is part of Qt's \l{Model/View Programming}{model/view framework}.

    An item selection can be constructed and initialized to contain a
    range of items from an existing model. The following example constructs
    a selection that contains a range of items from the given \c model,
    beginning at the \c topLeft, and ending at the \c bottomRight.

    \snippet doc/src/snippets/code/src_gui_itemviews_qitemselectionmodel.cpp 0

    An empty item selection can be constructed, and later populated as
    required. So, if the model is going to be unavailable when we construct
    the item selection, we can rewrite the above code in the following way:

    \snippet doc/src/snippets/code/src_gui_itemviews_qitemselectionmodel.cpp 1

    QItemSelection saves memory, and avoids unnecessary work, by working with
    selection ranges rather than recording the model item index for each
    item in the selection. Generally, an instance of this class will contain
    a list of non-overlapping selection ranges.

    Use merge() to merge one item selection into another without making
    overlapping ranges. Use split() to split one selection range into
    smaller ranges based on a another selection range.

    \sa {Model/View Programming}, QItemSelectionModel
*/

/*!
    \fn QItemSelection::QItemSelection()

    Constructs an empty selection.
*/

/*!
    Constructs an item selection that extends from the top-left model item,
    specified by the \a topLeft index, to the bottom-right item, specified
    by \a bottomRight.
*/
QItemSelection::QItemSelection(const QModelIndex &topLeft, const QModelIndex &bottomRight)
{
    select(topLeft, bottomRight);
}

/*!
    Adds the items in the range that extends from the top-left model
    item, specified by the \a topLeft index, to the bottom-right item,
    specified by \a bottomRight to the list.

    \note \a topLeft and \a bottomRight must have the same parent.
*/
void QItemSelection::select(const QModelIndex &topLeft, const QModelIndex &bottomRight)
{
    if (!topLeft.isValid() || !bottomRight.isValid())
        return;

    if ((topLeft.model() != bottomRight.model())
        || topLeft.parent() != bottomRight.parent()) {
        qWarning("Can't select indexes from different model or with different parents");
        return;
    }
    if (topLeft.row() > bottomRight.row() || topLeft.column() > bottomRight.column()) {
        int top = qMin(topLeft.row(), bottomRight.row());
        int bottom = qMax(topLeft.row(), bottomRight.row());
        int left = qMin(topLeft.column(), bottomRight.column());
        int right = qMax(topLeft.column(), bottomRight.column());
        QModelIndex tl = topLeft.sibling(top, left);
        QModelIndex br = bottomRight.sibling(bottom, right);
        append(QItemSelectionRange(tl, br));
        return;
    }
    append(QItemSelectionRange(topLeft, bottomRight));
}

/*!
    Returns true if the selection contains the given \a index; otherwise
    returns false.
*/

bool QItemSelection::contains(const QModelIndex &index) const
{
    if (index.flags() & Qt::ItemIsSelectable) {
        QList<QItemSelectionRange>::const_iterator it = begin();
        for (; it != end(); ++it)
            if ((*it).contains(index))
                return true;
    }
    return false;
}

/*!
    Returns a list of model indexes that correspond to the selected items.
*/

QModelIndexList QItemSelection::indexes() const
{
    QModelIndexList result;
    QList<QItemSelectionRange>::const_iterator it = begin();
    for (; it != end(); ++it)
        indexesFromRange(*it, result);
    return result;
}

/*!
    Merges the \a other selection with this QItemSelection using the
    \a command given. This method guarantees that no ranges are overlapping.

    Note that only QItemSelectionModel::Select,
    QItemSelectionModel::Deselect, and QItemSelectionModel::Toggle are
    supported.

    \sa split()
*/
void QItemSelection::merge(const QItemSelection &other, QItemSelectionModel::SelectionFlags command)
{
    if (other.isEmpty() ||
          !(command & QItemSelectionModel::Select ||
          command & QItemSelectionModel::Deselect ||
          command & QItemSelectionModel::Toggle))
        return;

    QItemSelection newSelection = other;
    // Collect intersections
    QItemSelection intersections;
    QItemSelection::iterator it = newSelection.begin();
    while (it != newSelection.end()) {
        if (!(*it).isValid()) {
            it = newSelection.erase(it);
            continue;
        }
        for (int t = 0; t < count(); ++t) {
            if ((*it).intersects(at(t)))
                intersections.append(at(t).intersected(*it));
        }
        ++it;
    }

    //  Split the old (and new) ranges using the intersections
    for (int i = 0; i < intersections.count(); ++i) { // for each intersection
        for (int t = 0; t < count();) { // splitt each old range
            if (at(t).intersects(intersections.at(i))) {
                split(at(t), intersections.at(i), this);
                removeAt(t);
            } else {
                ++t;
            }
        }
        // only split newSelection if Toggle is specified
        for (int n = 0; (command & QItemSelectionModel::Toggle) && n < newSelection.count();) {
            if (newSelection.at(n).intersects(intersections.at(i))) {
                split(newSelection.at(n), intersections.at(i), &newSelection);
                newSelection.removeAt(n);
            } else {
                ++n;
            }
        }
    }
    // do not add newSelection for Deselect
    if (!(command & QItemSelectionModel::Deselect))
        operator+=(newSelection);
}

/*!
    Splits the selection \a range using the selection \a other range.
    Removes all items in \a other from \a range and puts the result in \a result.
    This can be compared with the semantics of the \e subtract operation of a set.
    \sa merge()
*/

void QItemSelection::split(const QItemSelectionRange &range,
                           const QItemSelectionRange &other, QItemSelection *result)
{
    if (range.parent() != other.parent() || range.model() != other.model())
        return;

    QModelIndex parent = other.parent();
    int top = range.top();
    int left = range.left();
    int bottom = range.bottom();
    int right = range.right();
    int other_top = other.top();
    int other_left = other.left();
    int other_bottom = other.bottom();
    int other_right = other.right();
    const QAbstractItemModel *model = range.model();
    Q_ASSERT(model);
    if (other_top > top) {
        QModelIndex tl = model->index(top, left, parent);
        QModelIndex br = model->index(other_top - 1, right, parent);
        result->append(QItemSelectionRange(tl, br));
        top = other_top;
    }
    if (other_bottom < bottom) {
        QModelIndex tl = model->index(other_bottom + 1, left, parent);
        QModelIndex br = model->index(bottom, right, parent);
        result->append(QItemSelectionRange(tl, br));
        bottom = other_bottom;
    }
    if (other_left > left) {
        QModelIndex tl = model->index(top, left, parent);
        QModelIndex br = model->index(bottom, other_left - 1, parent);
        result->append(QItemSelectionRange(tl, br));
        left = other_left;
    }
    if (other_right < right) {
        QModelIndex tl = model->index(top, other_right + 1, parent);
        QModelIndex br = model->index(bottom, right, parent);
        result->append(QItemSelectionRange(tl, br));
        right = other_right;
    }
}


void QItemSelectionModelPrivate::initModel(QAbstractItemModel *model)
{
    this->model = model;
    if (model) {
        Q_Q(QItemSelectionModel);
        QObject::connect(model, SIGNAL(rowsAboutToBeRemoved(QModelIndex,int,int)),
                q, SLOT(_q_rowsAboutToBeRemoved(QModelIndex,int,int)));
        QObject::connect(model, SIGNAL(columnsAboutToBeRemoved(QModelIndex,int,int)),
                q, SLOT(_q_columnsAboutToBeRemoved(QModelIndex,int,int)));
        QObject::connect(model, SIGNAL(rowsAboutToBeInserted(QModelIndex,int,int)),
                q, SLOT(_q_rowsAboutToBeInserted(QModelIndex,int,int)));
        QObject::connect(model, SIGNAL(columnsAboutToBeInserted(QModelIndex,int,int)),
                q, SLOT(_q_columnsAboutToBeInserted(QModelIndex,int,int)));
        QObject::connect(model, SIGNAL(layoutAboutToBeChanged()),
                q, SLOT(_q_layoutAboutToBeChanged()));
        QObject::connect(model, SIGNAL(layoutChanged()),
                q, SLOT(_q_layoutChanged()));
    }
}

/*!
    \internal

    returns a QItemSelection where all ranges have been expanded to:
    Rows: left: 0 and right: columnCount()-1
    Columns: top: 0 and bottom: rowCount()-1
*/

QItemSelection QItemSelectionModelPrivate::expandSelection(const QItemSelection &selection,
                                                           QItemSelectionModel::SelectionFlags command) const
{
    if (selection.isEmpty() && !((command & QItemSelectionModel::Rows) ||
                                 (command & QItemSelectionModel::Columns)))
        return selection;

    QItemSelection expanded;
    if (command & QItemSelectionModel::Rows) {
        for (int i = 0; i < selection.count(); ++i) {
            QModelIndex parent = selection.at(i).parent();
            int colCount = model->columnCount(parent);
            QModelIndex tl = model->index(selection.at(i).top(), 0, parent);
            QModelIndex br = model->index(selection.at(i).bottom(), colCount - 1, parent);
            //we need to merge because the same row could have already been inserted
            expanded.merge(QItemSelection(tl, br), QItemSelectionModel::Select);
        }
    }
    if (command & QItemSelectionModel::Columns) {
        for (int i = 0; i < selection.count(); ++i) {
            QModelIndex parent = selection.at(i).parent();
            int rowCount = model->rowCount(parent);
            QModelIndex tl = model->index(0, selection.at(i).left(), parent);
            QModelIndex br = model->index(rowCount - 1, selection.at(i).right(), parent);
            //we need to merge because the same column could have already been inserted
            expanded.merge(QItemSelection(tl, br), QItemSelectionModel::Select);
        }
    }
    return expanded;
}

/*!
    \internal
*/
void QItemSelectionModelPrivate::_q_rowsAboutToBeRemoved(const QModelIndex &parent,
                                                         int start, int end)
{
    Q_Q(QItemSelectionModel);
    finalize();

    // update current index
    if (currentIndex.isValid() && parent == currentIndex.parent()
        && currentIndex.row() >= start && currentIndex.row() <= end) {
        QModelIndex old = currentIndex;
        if (start > 0) // there are rows left above the change
            currentIndex = model->index(start - 1, old.column(), parent);
        else if (model && end < model->rowCount(parent) - 1) // there are rows left below the change
            currentIndex = model->index(end + 1, old.column(), parent);
        else // there are no rows left in the table
            currentIndex = QModelIndex();
        emit q->currentChanged(currentIndex, old);
        emit q->currentRowChanged(currentIndex, old);
        if (currentIndex.column() != old.column())
            emit q->currentColumnChanged(currentIndex, old);
    }

    QItemSelection deselected;
    QItemSelection newParts;
    QItemSelection::iterator it = ranges.begin();
    while (it != ranges.end()) {
        if (it->topLeft().parent() != parent) {  // Check parents until reaching root or contained in range
            QModelIndex itParent = it->topLeft().parent();
            while (itParent.isValid() && itParent.parent() != parent)
                itParent = itParent.parent();

            if (itParent.isValid() && start <= itParent.row() && itParent.row() <= end) {
                deselected.append(*it);
                it = ranges.erase(it);
            } else {
                ++it;
            }
        } else if (start <= it->bottom() && it->bottom() <= end    // Full inclusion
                   && start <= it->top() && it->top() <= end) {
            deselected.append(*it);
            it = ranges.erase(it);
        } else if (start <= it->top() && it->top() <= end) {      // Top intersection
            deselected.append(QItemSelectionRange(it->topLeft(), model->index(end, it->left(), it->parent())));
            *it = QItemSelectionRange(model->index(end + 1, it->left(), it->parent()), it->bottomRight());
            ++it;
        } else if (start <= it->bottom() && it->bottom() <= end) {    // Bottom intersection
            deselected.append(QItemSelectionRange(model->index(start, it->right(), it->parent()), it->bottomRight()));
            *it = QItemSelectionRange(it->topLeft(), model->index(start - 1, it->right(), it->parent()));
            ++it;
        } else if (it->top() < start && end < it->bottom()) { // Middle intersection
            // If the parent contains (1, 2, 3, 4, 5, 6, 7, 8) and [3, 4, 5, 6] is selected,
            // and [4, 5] is removed, we need to split [3, 4, 5, 6] into [3], [4, 5] and [6].
            // [4, 5] is appended to deselected, and [3] and [6] remain part of the selection
            // in ranges.
            const QItemSelectionRange removedRange(model->index(start, it->right(), it->parent()),
                                                    model->index(end, it->left(), it->parent()));
            deselected.append(removedRange);
            QItemSelection::split(*it, removedRange, &newParts);
            it = ranges.erase(it);
        } else
            ++it;
    }
    ranges.append(newParts);

    if (!deselected.isEmpty())
        emit q->selectionChanged(QItemSelection(), deselected);
}

/*!
    \internal
*/
void QItemSelectionModelPrivate::_q_columnsAboutToBeRemoved(const QModelIndex &parent,
                                                            int start, int end)
{
    Q_Q(QItemSelectionModel);

    // update current index
    if (currentIndex.isValid() && parent == currentIndex.parent()
        && currentIndex.column() >= start && currentIndex.column() <= end) {
        QModelIndex old = currentIndex;
        if (start > 0) // there are columns to the left of the change
            currentIndex = model->index(old.row(), start - 1, parent);
        else if (model && end < model->columnCount() - 1) // there are columns to the right of the change
            currentIndex = model->index(old.row(), end + 1, parent);
        else // there are no columns left in the table
            currentIndex = QModelIndex();
        emit q->currentChanged(currentIndex, old);
        if (currentIndex.row() != old.row())
            emit q->currentRowChanged(currentIndex, old);
        emit q->currentColumnChanged(currentIndex, old);
    }

    // update selections
    QModelIndex tl = model->index(0, start, parent);
    QModelIndex br = model->index(model->rowCount(parent) - 1, end, parent);
    q->select(QItemSelection(tl, br), QItemSelectionModel::Deselect);
    finalize();
}

/*!
    \internal

    Split selection ranges if columns are about to be inserted in the middle.
*/
void QItemSelectionModelPrivate::_q_columnsAboutToBeInserted(const QModelIndex &parent,
                                                             int start, int end)
{
    Q_UNUSED(end);
    finalize();
    QList<QItemSelectionRange> split;
    QList<QItemSelectionRange>::iterator it = ranges.begin();
    for (; it != ranges.end(); ) {
        if ((*it).isValid() && (*it).parent() == parent
            && (*it).left() < start && (*it).right() >= start) {
            QModelIndex bottomMiddle = model->index((*it).bottom(), start - 1, (*it).parent());
            QItemSelectionRange left((*it).topLeft(), bottomMiddle);
            QModelIndex topMiddle = model->index((*it).top(), start, (*it).parent());
            QItemSelectionRange right(topMiddle, (*it).bottomRight());
            it = ranges.erase(it);
            split.append(left);
            split.append(right);
        } else {
            ++it;
        }
    }
    ranges += split;
}

/*!
    \internal

    Split selection ranges if rows are about to be inserted in the middle.
*/
void QItemSelectionModelPrivate::_q_rowsAboutToBeInserted(const QModelIndex &parent,
                                                          int start, int end)
{
    Q_UNUSED(end);
    finalize();
    QList<QItemSelectionRange> split;
    QList<QItemSelectionRange>::iterator it = ranges.begin();
    for (; it != ranges.end(); ) {
        if ((*it).isValid() && (*it).parent() == parent
            && (*it).top() < start && (*it).bottom() >= start) {
            QModelIndex middleRight = model->index(start - 1, (*it).right(), (*it).parent());
            QItemSelectionRange top((*it).topLeft(), middleRight);
            QModelIndex middleLeft = model->index(start, (*it).left(), (*it).parent());
            QItemSelectionRange bottom(middleLeft, (*it).bottomRight());
            it = ranges.erase(it);
            split.append(top);
            split.append(bottom);
        } else {
            ++it;
        }
    }
    ranges += split;
}

/*!
    \internal

    Split selection into individual (persistent) indexes. This is done in
    preparation for the layoutChanged() signal, where the indexes can be
    merged again.
*/
void QItemSelectionModelPrivate::_q_layoutAboutToBeChanged()
{
    savedPersistentIndexes.clear();
    savedPersistentCurrentIndexes.clear();

    // optimization for when all indexes are selected
    // (only if there is lots of items (1000) because this is not entirely correct)
    if (ranges.isEmpty() && currentSelection.count() == 1) {
        QItemSelectionRange range = currentSelection.first();
        QModelIndex parent = range.parent();
        tableRowCount = model->rowCount(parent);
        tableColCount = model->columnCount(parent);
        if (tableRowCount * tableColCount > 1000
            && range.top() == 0
            && range.left() == 0
            && range.bottom() == tableRowCount - 1
            && range.right() == tableColCount - 1) {
            tableSelected = true;
            tableParent = parent;
            return;
        }
    }
    tableSelected = false;

    QModelIndexList indexes = ranges.indexes();
    QModelIndexList::const_iterator it;
    for (it = indexes.constBegin(); it != indexes.constEnd(); ++it)
        savedPersistentIndexes.append(QPersistentModelIndex(*it));
    indexes = currentSelection.indexes();
    for (it = indexes.constBegin(); it != indexes.constEnd(); ++it)
        savedPersistentCurrentIndexes.append(QPersistentModelIndex(*it));
}

/*!
    \internal

    Merges \a indexes into an item selection made up of ranges.
    Assumes that the indexes are sorted.
*/
static QItemSelection mergeIndexes(const QList<QPersistentModelIndex> &indexes)
{
    QItemSelection colSpans;
    // merge columns
    int i = 0;
    while (i < indexes.count()) {
        QModelIndex tl = indexes.at(i);
        QModelIndex br = tl;
        while (++i < indexes.count()) {
            QModelIndex next = indexes.at(i);
            if ((next.parent() == br.parent())
                 && (next.row() == br.row())
                 && (next.column() == br.column() + 1))
                br = next;
            else
                break;
        }
        colSpans.append(QItemSelectionRange(tl, br));
    }
    // merge rows
    QItemSelection rowSpans;
    i = 0;
    while (i < colSpans.count()) {
        QModelIndex tl = colSpans.at(i).topLeft();
        QModelIndex br = colSpans.at(i).bottomRight();
        QModelIndex prevTl = tl;
        while (++i < colSpans.count()) {
            QModelIndex nextTl = colSpans.at(i).topLeft();
            QModelIndex nextBr = colSpans.at(i).bottomRight();

            if (nextTl.parent() != tl.parent())
                break; // we can't merge selection ranges from different parents

            if ((nextTl.column() == prevTl.column()) && (nextBr.column() == br.column())
                && (nextTl.row() == prevTl.row() + 1) && (nextBr.row() == br.row() + 1)) {
                br = nextBr;
                prevTl = nextTl;
            } else {
                break;
            }
        }
        rowSpans.append(QItemSelectionRange(tl, br));
    }
    return rowSpans;
}

/*!
    \internal

    Merge the selected indexes into selection ranges again.
*/
void QItemSelectionModelPrivate::_q_layoutChanged()
{
    // special case for when all indexes are selected
    if (tableSelected && tableColCount == model->columnCount(tableParent)
        && tableRowCount == model->rowCount(tableParent)) {
        ranges.clear();
        currentSelection.clear();
        int bottom = tableRowCount - 1;
        int right = tableColCount - 1;
        QModelIndex tl = model->index(0, 0, tableParent);
        QModelIndex br = model->index(bottom, right, tableParent);
        currentSelection << QItemSelectionRange(tl, br);
        tableParent = QModelIndex();
        tableSelected = false;
        return;
    }

    if (savedPersistentCurrentIndexes.isEmpty() && savedPersistentIndexes.isEmpty()) {
        // either the selection was actually empty, or we
        // didn't get the layoutAboutToBeChanged() signal
        return;
    }
    // clear the "old" selection
    ranges.clear();
    currentSelection.clear();

    // sort the "new" selection, as preparation for merging
    qStableSort(savedPersistentIndexes.begin(), savedPersistentIndexes.end());
    qStableSort(savedPersistentCurrentIndexes.begin(), savedPersistentCurrentIndexes.end());

    // update the selection by merging the individual indexes
    ranges = mergeIndexes(savedPersistentIndexes);
    currentSelection = mergeIndexes(savedPersistentCurrentIndexes);

    // release the persistent indexes
    savedPersistentIndexes.clear();
    savedPersistentCurrentIndexes.clear();
}

/*!
    \class QItemSelectionModel

    \brief The QItemSelectionModel class keeps track of a view's selected items.

    \ingroup model-view

    A QItemSelectionModel keeps track of the selected items in a view, or
    in several views onto the same model. It also keeps track of the
    currently selected item in a view.

    The QItemSelectionModel class is one of the \l{Model/View Classes}
    and is part of Qt's \l{Model/View Programming}{model/view framework}.

    The selected items are stored using ranges. Whenever you want to
    modify the selected items use select() and provide either a
    QItemSelection, or a QModelIndex and a QItemSelectionModel::SelectionFlag.

    The QItemSelectionModel takes a two layer approach to selection
    management, dealing with both selected items that have been committed
    and items that are part of the current selection. The current
    selected items are part of the current interactive selection (for
    example with rubber-band selection or keyboard-shift selections).

    To update the currently selected items, use the bitwise OR of
    QItemSelectionModel::Current and any of the other SelectionFlags.
    If you omit the QItemSelectionModel::Current command, a new current
    selection will be created, and the previous one added to the whole
    selection. All functions operate on both layers; for example,
    selectedItems() will return items from both layers.

    \sa {Model/View Programming}, QAbstractItemModel, {Chart Example}
*/

/*!
    Constructs a selection model that operates on the specified item \a model.
*/
QItemSelectionModel::QItemSelectionModel(QAbstractItemModel *model)
    : QObject(*new QItemSelectionModelPrivate, model)
{
    d_func()->initModel(model);
}

/*!
    Constructs a selection model that operates on the specified item \a model with \a parent.
*/
QItemSelectionModel::QItemSelectionModel(QAbstractItemModel *model, QObject *parent)
    : QObject(*new QItemSelectionModelPrivate, parent)
{
    d_func()->initModel(model);
}

/*!
    \internal
*/
QItemSelectionModel::QItemSelectionModel(QItemSelectionModelPrivate &dd, QAbstractItemModel *model)
    : QObject(dd, model)
{
    dd.initModel(model);
}

/*!
    Destroys the selection model.
*/
QItemSelectionModel::~QItemSelectionModel()
{
}

/*!
    Selects the model item \a index using the specified \a command, and emits
    selectionChanged().

    \sa QItemSelectionModel::SelectionFlags
*/
void QItemSelectionModel::select(const QModelIndex &index, QItemSelectionModel::SelectionFlags command)
{
    QItemSelection selection(index, index);
    select(selection, command);
}

/*!
    \fn void QItemSelectionModel::currentChanged(const QModelIndex &current, const QModelIndex &previous)

    This signal is emitted whenever the current item changes. The \a previous
    model item index is replaced by the \a current index as the selection's
    current item.

    Note that this signal will not be emitted when the item model is reset.

    \sa currentIndex() setCurrentIndex() selectionChanged()
*/

/*!
    \fn void QItemSelectionModel::currentColumnChanged(const QModelIndex &current, const QModelIndex &previous)

    This signal is emitted if the \a current item changes and its column is
    different to the column of the \a previous current item.

    Note that this signal will not be emitted when the item model is reset.

    \sa currentChanged() currentRowChanged() currentIndex() setCurrentIndex()
*/

/*!
    \fn void QItemSelectionModel::currentRowChanged(const QModelIndex &current, const QModelIndex &previous)

    This signal is emitted if the \a current item changes and its row is
    different to the row of the \a previous current item.

    Note that this signal will not be emitted when the item model is reset.

    \sa currentChanged() currentColumnChanged() currentIndex() setCurrentIndex()
*/

/*!
    \fn void QItemSelectionModel::selectionChanged(const QItemSelection &selected, const QItemSelection &deselected)

    This signal is emitted whenever the selection changes. The change in the
    selection is represented as an item selection of \a deselected items and
    an item selection of \a selected items.

    Note the that the current index changes independently from the selection.
    Also note that this signal will not be emitted when the item model is reset.

    \sa select() currentChanged()
*/

/*!
    \enum QItemSelectionModel::SelectionFlag

    This enum describes the way the selection model will be updated.

    \value NoUpdate       No selection will be made.
    \value Clear          The complete selection will be cleared.
    \value Select         All specified indexes will be selected.
    \value Deselect       All specified indexes will be deselected.
    \value Toggle         All specified indexes will be selected or
                          deselected depending on their current state.
    \value Current        The current selection will be updated.
    \value Rows           All indexes will be expanded to span rows.
    \value Columns        All indexes will be expanded to span columns.
    \value SelectCurrent  A combination of Select and Current, provided for
                          convenience.
    \value ToggleCurrent  A combination of Toggle and Current, provided for
                          convenience.
    \value ClearAndSelect A combination of Clear and Select, provided for
                          convenience.
*/

/*!
    Selects the item \a selection using the specified \a command, and emits
    selectionChanged().

    \sa QItemSelectionModel::SelectionFlag
*/
void QItemSelectionModel::select(const QItemSelection &selection, QItemSelectionModel::SelectionFlags command)
{
    Q_D(QItemSelectionModel);
    if (command == NoUpdate)
        return;

    // store old selection
    QItemSelection sel = selection;
    // If d->ranges is non-empty when the source model is reset the persistent indexes
    // it contains will be invalid. We can't clear them in a modelReset slot because that might already
    // be too late if another model observer is connected to the same modelReset slot and is invoked first
    // it might call select() on this selection model before any such QItemSelectionModelPrivate::_q_modelReset() slot
    // is invoked, so it would not be cleared yet. We clear it invalid ranges in it here.
    QItemSelection::iterator it = d->ranges.begin();
    while (it != d->ranges.end()) {
      if (!it->isValid())
        it = d->ranges.erase(it);
      else
        ++it;
    }

    QItemSelection old = d->ranges;
    old.merge(d->currentSelection, d->currentCommand);

    // expand selection according to SelectionBehavior
    if (command & Rows || command & Columns)
        sel = d->expandSelection(sel, command);

    // clear ranges and currentSelection
    if (command & Clear) {
        d->ranges.clear();
        d->currentSelection.clear();
    }

    // merge and clear currentSelection if Current was not set (ie. start new currentSelection)
    if (!(command & Current))
        d->finalize();

    // update currentSelection
    if (command & Toggle || command & Select || command & Deselect) {
        d->currentCommand = command;
        d->currentSelection = sel;
    }

    // generate new selection, compare with old and emit selectionChanged()
    QItemSelection newSelection = d->ranges;
    newSelection.merge(d->currentSelection, d->currentCommand);
    emitSelectionChanged(newSelection, old);
}

/*!
    Clears the selection model. Emits selectionChanged() and currentChanged().
*/
void QItemSelectionModel::clear()
{
    Q_D(QItemSelectionModel);
    clearSelection();
    QModelIndex previous = d->currentIndex;
    d->currentIndex = QModelIndex();
    if (previous.isValid()) {
        emit currentChanged(d->currentIndex, previous);
        emit currentRowChanged(d->currentIndex, previous);
        emit currentColumnChanged(d->currentIndex, previous);
    }
}

/*!
    Clears the selection model. Does not emit any signals.
*/
void QItemSelectionModel::reset()
{
    bool block = blockSignals(true);
    clear();
    blockSignals(block);
}

/*!
    \since 4.2
    Clears the selection in the selection model. Emits selectionChanged().
*/
void QItemSelectionModel::clearSelection()
{
    Q_D(QItemSelectionModel);
    if (d->ranges.count() == 0 && d->currentSelection.count() == 0)
        return;

    select(QItemSelection(), Clear);
}


/*!
    Sets the model item \a index to be the current item, and emits
    currentChanged(). The current item is used for keyboard navigation and
    focus indication; it is independent of any selected items, although a
    selected item can also be the current item.

    Depending on the specified \a command, the \a index can also become part
    of the current selection.
    \sa select()
*/
void QItemSelectionModel::setCurrentIndex(const QModelIndex &index, QItemSelectionModel::SelectionFlags command)
{
    Q_D(QItemSelectionModel);
    if (index == d->currentIndex) {
        if (command != NoUpdate)
            select(index, command); // select item
        return;
    }
    QPersistentModelIndex previous = d->currentIndex;
    d->currentIndex = index; // set current before emitting selection changed below
    if (command != NoUpdate)
        select(d->currentIndex, command); // select item
    emit currentChanged(d->currentIndex, previous);
    if (d->currentIndex.row() != previous.row() ||
            d->currentIndex.parent() != previous.parent())
        emit currentRowChanged(d->currentIndex, previous);
    if (d->currentIndex.column() != previous.column() ||
            d->currentIndex.parent() != previous.parent())
        emit currentColumnChanged(d->currentIndex, previous);
}

/*!
    Returns the model item index for the current item, or an invalid index
    if there is no current item.
*/
QModelIndex QItemSelectionModel::currentIndex() const
{
    return static_cast<QModelIndex>(d_func()->currentIndex);
}

/*!
    Returns true if the given model item \a index is selected.
*/
bool QItemSelectionModel::isSelected(const QModelIndex &index) const
{
    Q_D(const QItemSelectionModel);
    if (d->model != index.model() || !index.isValid())
        return false;

    bool selected = false;
    //  search model ranges
    QList<QItemSelectionRange>::const_iterator it = d->ranges.begin();
    for (; it != d->ranges.end(); ++it) {
        if ((*it).isValid() && (*it).contains(index)) {
            selected = true;
            break;
        }
    }

    // check  currentSelection
    if (d->currentSelection.count()) {
        if ((d->currentCommand & Deselect) && selected)
            selected = !d->currentSelection.contains(index);
        else if (d->currentCommand & Toggle)
            selected ^= d->currentSelection.contains(index);
        else if ((d->currentCommand & Select) && !selected)
            selected = d->currentSelection.contains(index);
    }

    if (selected) {
        Qt::ItemFlags flags = d->model->flags(index);
        return (flags & Qt::ItemIsSelectable);
    }

    return false;
}

/*!
    Returns true if all items are selected in the \a row with the given
    \a parent.

    Note that this function is usually faster than calling isSelected()
    on all items in the same row and that unselectable items are
    ignored.
*/
bool QItemSelectionModel::isRowSelected(int row, const QModelIndex &parent) const
{
    Q_D(const QItemSelectionModel);
    if (parent.isValid() && d->model != parent.model())
        return false;

    // return false if row exist in currentSelection (Deselect)
    if (d->currentCommand & Deselect && d->currentSelection.count()) {
        for (int i=0; i<d->currentSelection.count(); ++i) {
            if (d->currentSelection.at(i).parent() == parent &&
                row >= d->currentSelection.at(i).top() &&
                row <= d->currentSelection.at(i).bottom())
                return false;
        }
    }
    // return false if ranges in both currentSelection and ranges
    // intersect and have the same row contained
    if (d->currentCommand & Toggle && d->currentSelection.count()) {
        for (int i=0; i<d->currentSelection.count(); ++i)
            if (d->currentSelection.at(i).top() <= row &&
                d->currentSelection.at(i).bottom() >= row)
                for (int j=0; j<d->ranges.count(); ++j)
                    if (d->ranges.at(j).top() <= row && d->ranges.at(j).bottom() >= row
                        && d->currentSelection.at(i).intersected(d->ranges.at(j)).isValid())
                        return false;
    }
    // add ranges and currentSelection and check through them all
    QList<QItemSelectionRange>::const_iterator it;
    QList<QItemSelectionRange> joined = d->ranges;
    if (d->currentSelection.count())
        joined += d->currentSelection;
    int colCount = d->model->columnCount(parent);
    for (int column = 0; column < colCount; ++column) {
        for (it = joined.constBegin(); it != joined.constEnd(); ++it) {
            if ((*it).contains(row, column, parent)) {
                bool selectable = false;
                for (int i = column; !selectable && i <= (*it).right(); ++i) {
                    Qt::ItemFlags flags = d->model->index(row, i, parent).flags();
                    selectable = flags & Qt::ItemIsSelectable;
                }
                if (selectable){
                    column = qMax(column, (*it).right());
                    break;
                }
            }
        }
        if (it == joined.constEnd())
            return false;
    }
    return colCount > 0; // no columns means no selected items
}

/*!
    Returns true if all items are selected in the \a column with the given
    \a parent.

    Note that this function is usually faster than calling isSelected()
    on all items in the same column and that unselectable items are
    ignored.
*/
bool QItemSelectionModel::isColumnSelected(int column, const QModelIndex &parent) const
{
    Q_D(const QItemSelectionModel);
    if (parent.isValid() && d->model != parent.model())
        return false;

    // return false if column exist in currentSelection (Deselect)
    if (d->currentCommand & Deselect && d->currentSelection.count()) {
        for (int i = 0; i < d->currentSelection.count(); ++i) {
            if (d->currentSelection.at(i).parent() == parent &&
                column >= d->currentSelection.at(i).left() &&
                column <= d->currentSelection.at(i).right())
                return false;
        }
    }
    // return false if ranges in both currentSelection and the selection model
    // intersect and have the same column contained
    if (d->currentCommand & Toggle && d->currentSelection.count()) {
        for (int i = 0; i < d->currentSelection.count(); ++i) {
            if (d->currentSelection.at(i).left() <= column &&
                d->currentSelection.at(i).right() >= column) {
                for (int j = 0; j < d->ranges.count(); ++j) {
                    if (d->ranges.at(j).left() <= column && d->ranges.at(j).right() >= column
                        && d->currentSelection.at(i).intersected(d->ranges.at(j)).isValid()) {
                        return false;
                    }
                }
            }
        }
    }
    // add ranges and currentSelection and check through them all
    QList<QItemSelectionRange>::const_iterator it;
    QList<QItemSelectionRange> joined = d->ranges;
    if (d->currentSelection.count())
        joined += d->currentSelection;
    int rowCount = d->model->rowCount(parent);
    for (int row = 0; row < rowCount; ++row) {
         for (it = joined.constBegin(); it != joined.constEnd(); ++it) {
             if ((*it).contains(row, column, parent)) {
                 Qt::ItemFlags flags = d->model->index(row, column, parent).flags();
                 if ((flags & Qt::ItemIsSelectable) && (flags & Qt::ItemIsEnabled)) {
                     row = qMax(row, (*it).bottom());
                     break;
                 }
             }
         }
         if (it == joined.constEnd())
             return false;
    }
    return rowCount > 0; // no rows means no selected items
}

/*!
    Returns true if there are any items selected in the \a row with the given
    \a parent.
*/
bool QItemSelectionModel::rowIntersectsSelection(int row, const QModelIndex &parent) const
{
    Q_D(const QItemSelectionModel);
    if (parent.isValid() && d->model != parent.model())
         return false;

    QItemSelection sel = d->ranges;
    sel.merge(d->currentSelection, d->currentCommand);
    for (int i = 0; i < sel.count(); ++i) {
        int top = sel.at(i).top();
        int bottom = sel.at(i).bottom();
        int left = sel.at(i).left();
        int right = sel.at(i).right();
        if (top <= row && bottom >= row) {
            for (int j = left; j <= right; j++) {
                const Qt::ItemFlags flags = d->model->index(row, j, parent).flags();
                if ((flags & Qt::ItemIsSelectable) && (flags & Qt::ItemIsEnabled))
                    return true;
            }
        }
    }

    return false;
}

/*!
    Returns true if there are any items selected in the \a column with the given
    \a parent.
*/
bool QItemSelectionModel::columnIntersectsSelection(int column, const QModelIndex &parent) const
{
    Q_D(const QItemSelectionModel);
    if (parent.isValid() && d->model != parent.model())
        return false;

    QItemSelection sel = d->ranges;
    sel.merge(d->currentSelection, d->currentCommand);
    for (int i = 0; i < sel.count(); ++i) {
        int left = sel.at(i).left();
        int right = sel.at(i).right();
        int top =  sel.at(i).top();
        int bottom =  sel.at(i).bottom();
        if (left <= column && right >= column) {
            for (int j = top; j <= bottom; j++) {
                const Qt::ItemFlags flags = d->model->index(j, column, parent).flags();
                if ((flags & Qt::ItemIsSelectable) && (flags & Qt::ItemIsEnabled))
                    return true;
            }
        }
    }

    return false;
}

/*!
    \since 4.2

    Returns true if the selection model contains any selection ranges;
    otherwise returns false.
*/
bool QItemSelectionModel::hasSelection() const
{
    Q_D(const QItemSelectionModel);
    if (d->currentCommand & (Toggle | Deselect)) {
        QItemSelection sel = d->ranges;
        sel.merge(d->currentSelection, d->currentCommand);
        return !sel.isEmpty();
    } else {
        return !(d->ranges.isEmpty() && d->currentSelection.isEmpty());
    }
}

/*!
    Returns a list of all selected model item indexes. The list contains no
    duplicates, and is not sorted.
*/
QModelIndexList QItemSelectionModel::selectedIndexes() const
{
    Q_D(const QItemSelectionModel);
    QItemSelection selected = d->ranges;
    selected.merge(d->currentSelection, d->currentCommand);
    return selected.indexes();
}

/*!
    \since 4.2
    Returns the indexes in the given \a column for the rows where all columns are selected.

    \sa selectedIndexes(), selectedColumns()
*/

QModelIndexList QItemSelectionModel::selectedRows(int column) const
{
    QModelIndexList indexes;
    //the QSet contains pairs of parent modelIndex
    //and row number
    QSet< QPair<QModelIndex, int> > rowsSeen;

    const QItemSelection ranges = selection();
    for (int i = 0; i < ranges.count(); ++i) {
        const QItemSelectionRange &range = ranges.at(i);
        QModelIndex parent = range.parent();
        for (int row = range.top(); row <= range.bottom(); row++) {
            QPair<QModelIndex, int> rowDef = qMakePair(parent, row);
            if (!rowsSeen.contains(rowDef)) {
                rowsSeen << rowDef;
                if (isRowSelected(row, parent)) {
                    indexes.append(model()->index(row, column, parent));
                }
            }
        }
    }

    return indexes;
}

/*!
    \since 4.2
    Returns the indexes in the given \a row for columns where all rows are selected.

    \sa selectedIndexes(), selectedRows()
*/

QModelIndexList QItemSelectionModel::selectedColumns(int row) const
{
    QModelIndexList indexes;
    //the QSet contains pairs of parent modelIndex
    //and column number
    QSet< QPair<QModelIndex, int> > columnsSeen;

    const QItemSelection ranges = selection();
    for (int i = 0; i < ranges.count(); ++i) {
        const QItemSelectionRange &range = ranges.at(i);
        QModelIndex parent = range.parent();
        for (int column = range.left(); column <= range.right(); column++) {
            QPair<QModelIndex, int> columnDef = qMakePair(parent, column);
            if (!columnsSeen.contains(columnDef)) {
                columnsSeen << columnDef;
                if (isColumnSelected(column, parent)) {
                    indexes.append(model()->index(row, column, parent));
                }
            }
        }
    }

    return indexes;
}

/*!
    Returns the selection ranges stored in the selection model.
*/
const QItemSelection QItemSelectionModel::selection() const
{
    Q_D(const QItemSelectionModel);
    QItemSelection selected = d->ranges;
    selected.merge(d->currentSelection, d->currentCommand);
    int i = 0;
    // make sure we have no invalid ranges
    // ###  should probably be handled more generic somewhere else
    while (i<selected.count()) {
        if (selected.at(i).isValid())
            ++i;
        else
            (selected.removeAt(i));
    }
    return selected;
}

/*!
    Returns the item model operated on by the selection model.
*/
const QAbstractItemModel *QItemSelectionModel::model() const
{
    return d_func()->model;
}

/*!
    Compares the two selections \a newSelection and \a oldSelection
    and emits selectionChanged() with the deselected and selected items.
*/
void QItemSelectionModel::emitSelectionChanged(const QItemSelection &newSelection,
                                               const QItemSelection &oldSelection)
{
    // if both selections are empty or equal we return
    if ((oldSelection.isEmpty() && newSelection.isEmpty()) ||
        oldSelection == newSelection)
        return;

    // if either selection is empty we do not need to compare
    if (oldSelection.isEmpty() || newSelection.isEmpty()) {
        emit selectionChanged(newSelection, oldSelection);
        return;
    }

    QItemSelection deselected = oldSelection;
    QItemSelection selected = newSelection;

    // remove equal ranges
    bool advance;
    for (int o = 0; o < deselected.count(); ++o) {
        advance = true;
        for (int s = 0; s < selected.count() && o < deselected.count();) {
            if (deselected.at(o) == selected.at(s)) {
                deselected.removeAt(o);
                selected.removeAt(s);
                advance = false;
            } else {
                ++s;
            }
        }
        if (advance)
            ++o;
    }

    // find intersections
    QItemSelection intersections;
    for (int o = 0; o < deselected.count(); ++o) {
        for (int s = 0; s < selected.count(); ++s) {
            if (deselected.at(o).intersects(selected.at(s)))
                intersections.append(deselected.at(o).intersected(selected.at(s)));
        }
    }

    // compare remaining ranges with intersections and split them to find deselected and selected
    for (int i = 0; i < intersections.count(); ++i) {
        // split deselected
        for (int o = 0; o < deselected.count();) {
            if (deselected.at(o).intersects(intersections.at(i))) {
                QItemSelection::split(deselected.at(o), intersections.at(i), &deselected);
                deselected.removeAt(o);
            } else {
                ++o;
            }
        }
        // split selected
        for (int s = 0; s < selected.count();) {
            if (selected.at(s).intersects(intersections.at(i))) {
                QItemSelection::split(selected.at(s), intersections.at(i), &selected);
                selected.removeAt(s);
            } else {
                ++s;
            }
        }
    }

    if (!selected.isEmpty() || !deselected.isEmpty())
        emit selectionChanged(selected, deselected);
}

#ifndef QT_NO_DEBUG_STREAM
QDebug operator<<(QDebug dbg, const QItemSelectionRange &range)
{
#ifndef Q_BROKEN_DEBUG_STREAM
    dbg.nospace() << "QItemSelectionRange(" << range.topLeft()
                  << ',' << range.bottomRight() << ')';
    return dbg.space();
#else
    qWarning("This compiler doesn't support streaming QItemSelectionRange to QDebug");
    return dbg;
    Q_UNUSED(range);
#endif
}
#endif

QT_END_NAMESPACE

#include "moc_qitemselectionmodel.cpp"

#endif // QT_NO_ITEMVIEWS
