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

#include <qglobal.h>

#ifndef QT_NO_COLUMNVIEW

#include "qcolumnview.h"
#include "qcolumnview_p.h"
#include "qcolumnviewgrip_p.h"

#include <qlistview.h>
#include <qabstractitemdelegate.h>
#include <qscrollbar.h>
#include <qpainter.h>
#include <qdebug.h>

QT_BEGIN_NAMESPACE

#define ANIMATION_DURATION_MSEC 150

/*!
    \since 4.3
    \class QColumnView
    \brief The QColumnView class provides a model/view implementation of a column view.
    \ingroup model-view
    \ingroup advanced


    QColumnView displays a model in a number of QListViews, one for each
    hierarchy in the tree.  This is sometimes referred to as a cascading list.

    The QColumnView class is one of the \l{Model/View Classes}
    and is part of Qt's \l{Model/View Programming}{model/view framework}.

    QColumnView implements the interfaces defined by the
    QAbstractItemView class to allow it to display data provided by
    models derived from the QAbstractItemModel class.

    \image qcolumnview.png

    \sa \link model-view-programming.html Model/View Programming\endlink
*/

/*!
    Constructs a column view with a \a parent to represent a model's
    data. Use setModel() to set the model.

    \sa QAbstractItemModel
*/
QColumnView::QColumnView(QWidget * parent)
:  QAbstractItemView(*new QColumnViewPrivate, parent)
{
    Q_D(QColumnView);
    d->initialize();
}

/*!
    \internal
*/
QColumnView::QColumnView(QColumnViewPrivate & dd, QWidget * parent)
:  QAbstractItemView(dd, parent)
{
    Q_D(QColumnView);
    d->initialize();
}

void QColumnViewPrivate::initialize()
{
    Q_Q(QColumnView);
    q->setTextElideMode(Qt::ElideMiddle);
#ifndef QT_NO_ANIMATION
    QObject::connect(&currentAnimation, SIGNAL(finished()), q, SLOT(_q_changeCurrentColumn()));
    currentAnimation.setDuration(ANIMATION_DURATION_MSEC);
    currentAnimation.setTargetObject(hbar);
    currentAnimation.setPropertyName("value");
    currentAnimation.setEasingCurve(QEasingCurve::InOutQuad);
#endif //QT_NO_ANIMATION
    delete itemDelegate;
    q->setItemDelegate(new QColumnViewDelegate(q));
}

/*!
    Destroys the column view.
*/
QColumnView::~QColumnView()
{
}

/*!
    \property QColumnView::resizeGripsVisible
    \brief the way to specify if the list views gets resize grips or not

    By default, \c visible is set to true

    \sa setRootIndex()
*/
void QColumnView::setResizeGripsVisible(bool visible)
{
    Q_D(QColumnView);
    if (d->showResizeGrips == visible)
        return;
    d->showResizeGrips = visible;
    for (int i = 0; i < d->columns.count(); ++i) {
        QAbstractItemView *view = d->columns[i];
        if (visible) {
            QColumnViewGrip *grip = new QColumnViewGrip(view);
            view->setCornerWidget(grip);
            connect(grip, SIGNAL(gripMoved(int)), this, SLOT(_q_gripMoved(int)));
        } else {
            QWidget *widget = view->cornerWidget();
            view->setCornerWidget(0);
            widget->deleteLater();
        }
    }
}

bool QColumnView::resizeGripsVisible() const
{
    Q_D(const QColumnView);
    return d->showResizeGrips;
}

/*!
    \reimp
*/
void QColumnView::setModel(QAbstractItemModel *model)
{
    Q_D(QColumnView);
    if (model == d->model)
        return;
    d->closeColumns();
    QAbstractItemView::setModel(model);
}

/*!
    \reimp
*/
void QColumnView::setRootIndex(const QModelIndex &index)
{
    Q_D(QColumnView);
    if (!model())
        return;

    d->closeColumns();
    Q_ASSERT(d->columns.count() == 0);

    QAbstractItemView *view = d->createColumn(index, true);
    if (view->selectionModel())
        view->selectionModel()->deleteLater();
    if (view->model())
        view->setSelectionModel(selectionModel());

    QAbstractItemView::setRootIndex(index);
    d->updateScrollbars();
}

/*!
    \reimp
*/
bool QColumnView::isIndexHidden(const QModelIndex &index) const
{
    Q_UNUSED(index);
    return false;
}

/*!
    \reimp
*/
QModelIndex QColumnView::indexAt(const QPoint &point) const
{
    Q_D(const QColumnView);
    for (int i = 0; i < d->columns.size(); ++i) {
        QPoint topLeft = d->columns.at(i)->frameGeometry().topLeft();
        QPoint adjustedPoint(point.x() - topLeft.x(), point.y() - topLeft.y());
        QModelIndex index = d->columns.at(i)->indexAt(adjustedPoint);
        if (index.isValid())
            return index;
    }
    return QModelIndex();
}

/*!
    \reimp
*/
QRect QColumnView::visualRect(const QModelIndex &index) const
{
    if (!index.isValid())
        return QRect();

    Q_D(const QColumnView);
    for (int i = 0; i < d->columns.size(); ++i) {
        QRect rect = d->columns.at(i)->visualRect(index);
        if (!rect.isNull()) {
            rect.translate(d->columns.at(i)->frameGeometry().topLeft());
            return rect;
        }
    }
    return QRect();
}

/*!
    \reimp
 */
void QColumnView::scrollContentsBy(int dx, int dy)
{
    Q_D(QColumnView);
    if (d->columns.isEmpty() || dx == 0)
        return;

    dx = isRightToLeft() ? -dx : dx;
    for (int i = 0; i < d->columns.count(); ++i)
        d->columns.at(i)->move(d->columns.at(i)->x() + dx, 0);
    d->offset += dx;
    QAbstractItemView::scrollContentsBy(dx, dy);
}

/*!
    \reimp
*/
void QColumnView::scrollTo(const QModelIndex &index, ScrollHint hint)
{
    Q_D(QColumnView);
    Q_UNUSED(hint);
    if (!index.isValid() || d->columns.isEmpty())
        return;

#ifndef QT_NO_ANIMATION
    if (d->currentAnimation.state() == QPropertyAnimation::Running)
        return;

    d->currentAnimation.stop();
#endif //QT_NO_ANIMATION

    // Fill up what is needed to get to index
    d->closeColumns(index, true);

    QModelIndex indexParent = index.parent();
    // Find the left edge of the column that contains index
    int currentColumn = 0;
    int leftEdge = 0;
    while (currentColumn < d->columns.size()) {
        if (indexParent == d->columns.at(currentColumn)->rootIndex())
            break;
        leftEdge += d->columns.at(currentColumn)->width();
        ++currentColumn;
    }

    // Don't let us scroll above the root index
    if (currentColumn == d->columns.size())
        return;

    int indexColumn = currentColumn;
    // Find the width of what we want to show (i.e. the right edge)
    int visibleWidth = d->columns.at(currentColumn)->width();
    // We want to always try to show two columns
    if (currentColumn + 1 < d->columns.size()) {
        ++currentColumn;
        visibleWidth += d->columns.at(currentColumn)->width();
    }

    int rightEdge = leftEdge + visibleWidth;
    if (isRightToLeft()) {
        leftEdge = viewport()->width() - leftEdge;
        rightEdge = leftEdge - visibleWidth;
        qSwap(rightEdge, leftEdge);
    }

    // If it is already visible don't animate
    if (leftEdge > -horizontalOffset()
        && rightEdge <= ( -horizontalOffset() + viewport()->size().width())) {
            d->columns.at(indexColumn)->scrollTo(index);
            d->_q_changeCurrentColumn();
            return;
    }

    int newScrollbarValue = 0;
    if (isRightToLeft()) {
        if (leftEdge < 0) {
            // scroll to the right
            newScrollbarValue = viewport()->size().width() - leftEdge;
        } else {
            // scroll to the left
            newScrollbarValue = rightEdge + horizontalOffset();
        }
    } else {
        if (leftEdge > -horizontalOffset()) {
            // scroll to the right
            newScrollbarValue = rightEdge - viewport()->size().width();
        } else {
            // scroll to the left
            newScrollbarValue = leftEdge;
        }
    }

#ifndef QT_NO_ANIMATION
    d->currentAnimation.setEndValue(newScrollbarValue);
    d->currentAnimation.start();
#else
    horizontalScrollBar()->setValue(newScrollbarValue);
#endif //QT_NO_ANIMATION
}

/*!
    \reimp
    Move left should go to the parent index
    Move right should go to the child index or down if there is no child
*/
QModelIndex QColumnView::moveCursor(CursorAction cursorAction, Qt::KeyboardModifiers modifiers)
{
    // the child views which have focus get to deal with this first and if
    // they don't accept it then it comes up this view and we only grip left/right
    Q_UNUSED(modifiers);
    if (!model())
        return QModelIndex();

    QModelIndex current = currentIndex();
    if (isRightToLeft()) {
        if (cursorAction == MoveLeft)
            cursorAction = MoveRight;
        else if (cursorAction == MoveRight)
            cursorAction = MoveLeft;
    }
    switch (cursorAction) {
    case MoveLeft:
        if (current.parent().isValid() && current.parent() != rootIndex())
            return (current.parent());
        else
            return current;
        break;

    case MoveRight:
        if (model()->hasChildren(current))
            return model()->index(0, 0, current);
        else
            return current.sibling(current.row() + 1, current.column());
        break;

    default:
        break;
    }

    return QModelIndex();
}

/*!
    \reimp
*/
void QColumnView::resizeEvent(QResizeEvent *event)
{
    Q_D(QColumnView);
    d->doLayout();
    d->updateScrollbars();
    if (!isRightToLeft()) {
        int diff = event->oldSize().width() - event->size().width();
        if (diff < 0 && horizontalScrollBar()->isVisible()
            && horizontalScrollBar()->value() == horizontalScrollBar()->maximum()) {
            horizontalScrollBar()->setMaximum(horizontalScrollBar()->maximum() + diff);
        }
    }
    QAbstractItemView::resizeEvent(event);
}

/*!
    \internal
*/
void QColumnViewPrivate::updateScrollbars()
{
    Q_Q(QColumnView);
#ifndef QT_NO_ANIMATION
    if (currentAnimation.state() == QPropertyAnimation::Running)
        return;
#endif //QT_NO_ANIMATION

    // find the total horizontal length of the laid out columns
    int horizontalLength = 0;
    if (!columns.isEmpty()) {
        horizontalLength = (columns.last()->x() + columns.last()->width()) - columns.first()->x();
        if (horizontalLength <= 0) // reverse mode
            horizontalLength = (columns.first()->x() + columns.first()->width()) - columns.last()->x();
    }

    QSize viewportSize = viewport->size();
    if (horizontalLength < viewportSize.width() && hbar->value() == 0) {
        hbar->setRange(0, 0);
    } else {
        int visibleLength = qMin(horizontalLength + q->horizontalOffset(), viewportSize.width());
        int hiddenLength = horizontalLength - visibleLength;
        if (hiddenLength != hbar->maximum())
            hbar->setRange(0, hiddenLength);
    }
    if (!columns.isEmpty()) {
        int pageStepSize = columns.at(0)->width();
        if (pageStepSize != hbar->pageStep())
            hbar->setPageStep(pageStepSize);
    }
    bool visible = (hbar->maximum() > 0);
    if (visible != hbar->isVisible())
        hbar->setVisible(visible);
}

/*!
    \reimp
*/
int QColumnView::horizontalOffset() const
{
    Q_D(const QColumnView);
    return d->offset;
}

/*!
    \reimp
*/
int QColumnView::verticalOffset() const
{
    return 0;
}

/*!
    \reimp
*/
QRegion QColumnView::visualRegionForSelection(const QItemSelection &selection) const
{
    int ranges = selection.count();

    if (ranges == 0)
        return QRect();

    // Note that we use the top and bottom functions of the selection range
    // since the data is stored in rows.
    int firstRow = selection.at(0).top();
    int lastRow = selection.at(0).top();
    for (int i = 0; i < ranges; ++i) {
        firstRow = qMin(firstRow, selection.at(i).top());
        lastRow = qMax(lastRow, selection.at(i).bottom());
    }

    QModelIndex firstIdx = model()->index(qMin(firstRow, lastRow), 0, rootIndex());
    QModelIndex lastIdx = model()->index(qMax(firstRow, lastRow), 0, rootIndex());

    if (firstIdx == lastIdx)
        return visualRect(firstIdx);

    QRegion firstRegion = visualRect(firstIdx);
    QRegion lastRegion = visualRect(lastIdx);
    return firstRegion.unite(lastRegion);
}

/*!
    \reimp
*/
void QColumnView::setSelection(const QRect &rect, QItemSelectionModel::SelectionFlags command)
{
    Q_UNUSED(rect);
    Q_UNUSED(command);
}

/*!
    \reimp
*/
void QColumnView::setSelectionModel(QItemSelectionModel *newSelectionModel)
{
    Q_D(const QColumnView);
    for (int i = 0; i < d->columns.size(); ++i) {
        if (d->columns.at(i)->selectionModel() == selectionModel()) {
            d->columns.at(i)->setSelectionModel(newSelectionModel);
            break;
        }
    }
    QAbstractItemView::setSelectionModel(newSelectionModel);
}

/*!
    \reimp
*/
QSize QColumnView::sizeHint() const
{
    Q_D(const QColumnView);
    QSize sizeHint;
    for (int i = 0; i < d->columns.size(); ++i) {
        sizeHint += d->columns.at(i)->sizeHint();
    }
    return sizeHint.expandedTo(QAbstractItemView::sizeHint());
}

/*!
    \internal
    Move all widgets from the corner grip and to the right
  */
void QColumnViewPrivate::_q_gripMoved(int offset)
{
    Q_Q(QColumnView);

    QObject *grip = q->sender();
    Q_ASSERT(grip);

    if (q->isRightToLeft())
        offset = -1 * offset;

    bool found = false;
    for (int i = 0; i < columns.size(); ++i) {
        if (!found && columns.at(i)->cornerWidget() == grip) {
            found = true;
            columnSizes[i] = columns.at(i)->width();
            if (q->isRightToLeft())
                columns.at(i)->move(columns.at(i)->x() + offset, 0);
            continue;
        }
        if (!found)
            continue;

        int currentX = columns.at(i)->x();
        columns.at(i)->move(currentX + offset, 0);
    }

    updateScrollbars();
}

/*!
    \internal

    Find where the current columns intersect parent's columns

    Delete any extra columns and insert any needed columns.
  */
void QColumnViewPrivate::closeColumns(const QModelIndex &parent, bool build)
{
    if (columns.isEmpty())
        return;

    bool clearAll = !parent.isValid();
    bool passThroughRoot = false;

    QList<QModelIndex> dirsToAppend;

    // Find the last column that matches the parent's tree
    int currentColumn = -1;
    QModelIndex parentIndex = parent;
    while (currentColumn == -1 && parentIndex.isValid()) {
        if (columns.isEmpty())
            break;
        parentIndex = parentIndex.parent();
        if (root == parentIndex)
            passThroughRoot = true;
        if (!parentIndex.isValid())
            break;
        for (int i = columns.size() - 1; i >= 0; --i) {
            if (columns.at(i)->rootIndex() == parentIndex) {
                currentColumn = i;
                break;
            }
        }
        if (currentColumn == -1)
            dirsToAppend.append(parentIndex);
    }

    // Someone wants to go to an index that can be reached without changing
    // the root index, don't allow them
    if (!clearAll && !passThroughRoot && currentColumn == -1)
        return;

    if (currentColumn == -1 && parent.isValid())
        currentColumn = 0;

    // Optimization so we don't go deleting and then creating the same thing
    bool alreadyExists = false;
    if (build && columns.size() > currentColumn + 1) {
        bool viewingParent = (columns.at(currentColumn + 1)->rootIndex() == parent);
        bool viewingChild = (!model->hasChildren(parent)
                             && !columns.at(currentColumn + 1)->rootIndex().isValid());
        if (viewingParent || viewingChild) {
            currentColumn++;
            alreadyExists = true;
        }
    }

    // Delete columns that don't match our path
    for (int i = columns.size() - 1; i > currentColumn; --i) {
        QAbstractItemView* notShownAnymore = columns.at(i);
        columns.removeAt(i);
        notShownAnymore->setVisible(false);
        if (notShownAnymore != previewColumn)
            notShownAnymore->deleteLater();
    }

    if (columns.isEmpty()) {
        offset = 0;
        updateScrollbars();
    }

    // Now fill in missing columns
    while (!dirsToAppend.isEmpty()) {
        QAbstractItemView *newView = createColumn(dirsToAppend.takeLast(), true);
        if (!dirsToAppend.isEmpty())
            newView->setCurrentIndex(dirsToAppend.last());
    }

    if (build && !alreadyExists)
        createColumn(parent, false);
}

void QColumnViewPrivate::_q_clicked(const QModelIndex &index)
{
    Q_Q(QColumnView);
    QModelIndex parent = index.parent();
    QAbstractItemView *columnClicked = 0;
    for (int column = 0; column < columns.count(); ++column) {
        if (columns.at(column)->rootIndex() == parent) {
            columnClicked = columns[column];
            break;
        }
    }
    if (q->selectionModel() && columnClicked) {
        QItemSelectionModel::SelectionFlags flags = QItemSelectionModel::Current;
        if (columnClicked->selectionModel()->isSelected(index))
            flags |= QItemSelectionModel::Select;
        q->selectionModel()->setCurrentIndex(index, flags);
    }
}

/*!
    \internal
    Create a new column for \a index.  A grip is attached if requested and it is shown
    if requested.

    Return the new view

    \sa createColumn() setPreviewWidget()
    \sa doLayout()
*/
QAbstractItemView *QColumnViewPrivate::createColumn(const QModelIndex &index, bool show)
{
    Q_Q(QColumnView);
    QAbstractItemView *view = 0;
    if (model->hasChildren(index)) {
        view = q->createColumn(index);
        q->connect(view, SIGNAL(clicked(QModelIndex)),
                   q, SLOT(_q_clicked(QModelIndex)));
    } else {
        if (!previewColumn)
            setPreviewWidget(new QWidget(q));
        view = previewColumn;
        view->setMinimumWidth(qMax(view->minimumWidth(), previewWidget->minimumWidth()));
    }

    q->connect(view, SIGNAL(activated(QModelIndex)),
            q, SIGNAL(activated(QModelIndex)));
    q->connect(view, SIGNAL(clicked(QModelIndex)),
            q, SIGNAL(clicked(QModelIndex)));
    q->connect(view, SIGNAL(doubleClicked(QModelIndex)),
            q, SIGNAL(doubleClicked(QModelIndex)));
    q->connect(view, SIGNAL(entered(QModelIndex)),
            q, SIGNAL(entered(QModelIndex)));
    q->connect(view, SIGNAL(pressed(QModelIndex)),
            q, SIGNAL(pressed(QModelIndex)));

    view->setFocusPolicy(Qt::NoFocus);
    view->setParent(viewport);
    Q_ASSERT(view);

    // Setup corner grip
    if (showResizeGrips) {
        QColumnViewGrip *grip = new QColumnViewGrip(view);
        view->setCornerWidget(grip);
        q->connect(grip, SIGNAL(gripMoved(int)), q, SLOT(_q_gripMoved(int)));
    }

    if (columnSizes.count() > columns.count()) {
        view->setGeometry(0, 0, columnSizes.at(columns.count()), viewport->height());
    } else {
        int initialWidth = view->sizeHint().width();
        if (q->isRightToLeft())
            view->setGeometry(viewport->width() - initialWidth, 0, initialWidth, viewport->height());
        else
            view->setGeometry(0, 0, initialWidth, viewport->height());
        columnSizes.resize(qMax(columnSizes.count(), columns.count() + 1));
        columnSizes[columns.count()] = initialWidth;
    }
    if (!columns.isEmpty() && columns.last()->isHidden())
        columns.last()->setVisible(true);

    columns.append(view);
    doLayout();
    updateScrollbars();
    if (show && view->isHidden())
        view->setVisible(true);
    return view;
}

/*!
    \fn void QColumnView::updatePreviewWidget(const QModelIndex &index)

    This signal is emitted when the preview widget should be updated to
    provide rich information about \a index

    \sa previewWidget()
 */

/*!
    To use a custom widget for the final column when you select
    an item overload this function and return a widget.
    \a index is the root index that will be assigned to the view.

    Return the new view.  QColumnView will automatically take ownership of the widget.

    \sa setPreviewWidget()
 */
QAbstractItemView *QColumnView::createColumn(const QModelIndex &index)
{
    QListView *view = new QListView(viewport());

    initializeColumn(view);

    view->setRootIndex(index);
    if (model()->canFetchMore(index))
        model()->fetchMore(index);

    return view;
}

/*!
    Copies the behavior and options of the column view and applies them to
    the \a column such as the iconSize(), textElideMode() and
    alternatingRowColors(). This can be useful when reimplementing
    createColumn().

    \since 4.4
    \sa createColumn()
 */
void QColumnView::initializeColumn(QAbstractItemView *column) const
{
    Q_D(const QColumnView);

    column->setFrameShape(QFrame::NoFrame);
    column->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    column->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    column->setMinimumWidth(100);
    column->setAttribute(Qt::WA_MacShowFocusRect, false);

#ifndef QT_NO_DRAGANDDROP
    column->setDragDropMode(dragDropMode());
    column->setDragDropOverwriteMode(dragDropOverwriteMode());
    column->setDropIndicatorShown(showDropIndicator());
#endif
    column->setAlternatingRowColors(alternatingRowColors());
    column->setAutoScroll(hasAutoScroll());
    column->setEditTriggers(editTriggers());
    column->setHorizontalScrollMode(horizontalScrollMode());
    column->setIconSize(iconSize());
    column->setSelectionBehavior(selectionBehavior());
    column->setSelectionMode(selectionMode());
    column->setTabKeyNavigation(tabKeyNavigation());
    column->setTextElideMode(textElideMode());
    column->setVerticalScrollMode(verticalScrollMode());

    column->setModel(model());

    // Copy the custom delegate per row
    QMapIterator<int, QPointer<QAbstractItemDelegate> > i(d->rowDelegates);
    while (i.hasNext()) {
        i.next();
        column->setItemDelegateForRow(i.key(), i.value());
    }

    // set the delegate to be the columnview delegate
    QAbstractItemDelegate *delegate = column->itemDelegate();
    column->setItemDelegate(d->itemDelegate);
    delete delegate;
}

/*!
    Returns the preview widget, or 0 if there is none.

    \sa setPreviewWidget(), updatePreviewWidget()
*/
QWidget *QColumnView::previewWidget() const
{
    Q_D(const QColumnView);
    return d->previewWidget;
}

/*!
    Sets the preview \a widget.

    The \a widget becomes a child of the column view, and will be
    destroyed when the column area is deleted or when a new widget is
    set.

    \sa previewWidget(), updatePreviewWidget()
*/
void QColumnView::setPreviewWidget(QWidget *widget)
{
    Q_D(QColumnView);
    d->setPreviewWidget(widget);
}

/*!
    \internal
*/
void QColumnViewPrivate::setPreviewWidget(QWidget *widget)
{
    Q_Q(QColumnView);
    if (previewColumn) {
        if (!columns.isEmpty() && columns.last() == previewColumn)
            columns.removeLast();
        previewColumn->deleteLater();
    }
    QColumnViewPreviewColumn *column = new QColumnViewPreviewColumn(q);
    column->setPreviewWidget(widget);
    previewColumn = column;
    previewColumn->hide();
    previewColumn->setFrameShape(QFrame::NoFrame);
    previewColumn->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    previewColumn->setSelectionMode(QAbstractItemView::NoSelection);
    previewColumn->setMinimumWidth(qMax(previewColumn->verticalScrollBar()->width(),
                previewColumn->minimumWidth()));
    previewWidget = widget;
    previewWidget->setParent(previewColumn->viewport());
}

/*!
    Sets the column widths to the values given in the \a list.  Extra values in the list are
    kept and used when the columns are created.

    If list contains too few values, only width of the rest of the columns will not be modified.

    \sa columnWidths(), createColumn()
*/
void QColumnView::setColumnWidths(const QList<int> &list)
{
    Q_D(QColumnView);
    int i = 0;
    for (; (i < list.count() && i < d->columns.count()); ++i) {
        d->columns.at(i)->resize(list.at(i), d->columns.at(i)->height());
        d->columnSizes[i] = list.at(i);
    }
    for (; i < list.count(); ++i)
        d->columnSizes.append(list.at(i));
}

/*!
    Returns a list of the width of all the columns in this view.

    \sa setColumnWidths()
*/
QList<int> QColumnView::columnWidths() const
{
    Q_D(const QColumnView);
    QList<int> list;
    for (int i = 0; i < d->columns.count(); ++i)
        list.append(d->columnSizes.at(i));
    return list;
}

/*!
    \reimp
*/
void QColumnView::rowsInserted(const QModelIndex &parent, int start, int end)
{
    QAbstractItemView::rowsInserted(parent, start, end);
    d_func()->checkColumnCreation(parent);
}

/*!
    \reimp
*/
void QColumnView::currentChanged(const QModelIndex &current, const QModelIndex &previous)
{
    Q_D(QColumnView);
    if (!current.isValid()) {
        QAbstractItemView::currentChanged(current, previous);
        return;
    }

    QModelIndex currentParent = current.parent();
    // optimize for just moving up/down in a list where the child view doesn't change
    if (currentParent == previous.parent()
            && model()->hasChildren(current) && model()->hasChildren(previous)) {
        for (int i = 0; i < d->columns.size(); ++i) {
            if (currentParent == d->columns.at(i)->rootIndex()) {
                if (d->columns.size() > i + 1) {
                    QAbstractItemView::currentChanged(current, previous);
                    return;
                }
                break;
            }
        }
    }

    // Scrolling to the right we need to have an empty spot
    bool found = false;
    if (currentParent == previous) {
        for (int i = 0; i < d->columns.size(); ++i) {
            if (currentParent == d->columns.at(i)->rootIndex()) {
                found = true;
                if (d->columns.size() < i + 2) {
                    d->createColumn(current, false);
                }
                break;
            }
        }
    }
    if (!found)
        d->closeColumns(current, true);

    if (!model()->hasChildren(current))
        emit updatePreviewWidget(current);

    QAbstractItemView::currentChanged(current, previous);
}

/*
    We have change the current column and need to update focus and selection models
    on the new current column.
*/
void QColumnViewPrivate::_q_changeCurrentColumn()
{
    Q_Q(QColumnView);
    if (columns.isEmpty())
        return;

    QModelIndex current = q->currentIndex();
    if (!current.isValid())
        return;

    // We might have scrolled far to the left so we need to close all of the children
    closeColumns(current, true);

    // Set up the "current" column with focus
    int currentColumn = qMax(0, columns.size() - 2);
    QAbstractItemView *parentColumn = columns.at(currentColumn);
    if (q->hasFocus())
        parentColumn->setFocus(Qt::OtherFocusReason);
    q->setFocusProxy(parentColumn);

    // find the column that is our current selection model and give it a new one.
    for (int i = 0; i < columns.size(); ++i) {
        if (columns.at(i)->selectionModel() == q->selectionModel()) {
            QItemSelectionModel *replacementSelectionModel =
                new QItemSelectionModel(parentColumn->model());
            replacementSelectionModel->setCurrentIndex(
                q->selectionModel()->currentIndex(), QItemSelectionModel::Current);
            replacementSelectionModel->select(
                q->selectionModel()->selection(), QItemSelectionModel::Select);
            QAbstractItemView *view = columns.at(i);
            view->setSelectionModel(replacementSelectionModel);
            view->setFocusPolicy(Qt::NoFocus);
            if (columns.size() > i + 1)
                view->setCurrentIndex(columns.at(i+1)->rootIndex());
            break;
        }
    }
    parentColumn->selectionModel()->deleteLater();
    parentColumn->setFocusPolicy(Qt::StrongFocus);
    parentColumn->setSelectionModel(q->selectionModel());
    // We want the parent selection to stay highlighted (but dimmed depending upon the color theme)
    if (currentColumn > 0) {
        parentColumn = columns.at(currentColumn - 1);
        if (parentColumn->currentIndex() != current.parent())
            parentColumn->setCurrentIndex(current.parent());
    }

    if (columns.last()->isHidden()) {
        columns.last()->setVisible(true);
    }
    if (columns.last()->selectionModel())
        columns.last()->selectionModel()->clear();
    updateScrollbars();
}

/*!
    \reimp
*/
void QColumnView::selectAll()
{
    if (!model() || !selectionModel())
        return;

    QModelIndexList indexList = selectionModel()->selectedIndexes();
    QModelIndex parent = rootIndex();
    QItemSelection selection;
    if (indexList.count() >= 1)
        parent = indexList.at(0).parent();
    if (indexList.count() == 1) {
        parent = indexList.at(0);
        if (!model()->hasChildren(parent))
            parent = parent.parent();
        else
            selection.append(QItemSelectionRange(parent, parent));
    }

    QModelIndex tl = model()->index(0, 0, parent);
    QModelIndex br = model()->index(model()->rowCount(parent) - 1,
                                    model()->columnCount(parent) - 1,
                                    parent);
    selection.append(QItemSelectionRange(tl, br));
    selectionModel()->select(selection, QItemSelectionModel::ClearAndSelect);
}

/*
 * private object implementation
 */
QColumnViewPrivate::QColumnViewPrivate()
:  QAbstractItemViewPrivate()
,showResizeGrips(true)
,offset(0)
,previewWidget(0)
,previewColumn(0)
{
}

QColumnViewPrivate::~QColumnViewPrivate()
{
}

/*!
    \internal

  */
void QColumnViewPrivate::_q_columnsInserted(const QModelIndex &parent, int start, int end)
{
    QAbstractItemViewPrivate::_q_columnsInserted(parent, start, end);
    checkColumnCreation(parent);
}

/*!
    \internal

    Makes sure we create a corresponding column as a result of changing the model.

  */
void QColumnViewPrivate::checkColumnCreation(const QModelIndex &parent)
{
    if (parent == q_func()->currentIndex() && model->hasChildren(parent)) {
        //the parent has children and is the current
        //let's try to find out if there is already a mapping that is good
        for (int i = 0; i < columns.count(); ++i) {
            QAbstractItemView *view = columns.at(i);
            if (view->rootIndex() == parent) {
                if (view == previewColumn) {
                    //let's recreate the parent
                    closeColumns(parent, false);
                    createColumn(parent, true /*show*/);
                }
                break;
            }
        }
    }
}

/*!
    \internal
    Place all of the columns where they belong inside of the viewport, resize as necessary.
*/
void QColumnViewPrivate::doLayout()
{
    Q_Q(QColumnView);
    if (!model || columns.isEmpty())
        return;

    int viewportHeight = viewport->height();
    int x = columns.at(0)->x();

    if (q->isRightToLeft()) {
        x = viewport->width() + q->horizontalOffset();
        for (int i = 0; i < columns.size(); ++i) {
            QAbstractItemView *view = columns.at(i);
            x -= view->width();
            if (x != view->x() || viewportHeight != view->height())
                view->setGeometry(x, 0, view->width(), viewportHeight);
        }
    } else {
        for (int i = 0; i < columns.size(); ++i) {
            QAbstractItemView *view = columns.at(i);
            int currentColumnWidth = view->width();
            if (x != view->x() || viewportHeight != view->height())
                view->setGeometry(x, 0, currentColumnWidth, viewportHeight);
            x += currentColumnWidth;
        }
    }
}

/*!
    \internal

    Draws a delegate with a > if an object has children.

    \sa {Model/View Programming}, QItemDelegate
*/
void QColumnViewDelegate::paint(QPainter *painter,
                          const QStyleOptionViewItem &option,
                          const QModelIndex &index) const
{
    drawBackground(painter, option, index );

    bool reverse = (option.direction == Qt::RightToLeft);
    int width = ((option.rect.height() * 2) / 3);
    // Modify the options to give us room to add an arrow
    QStyleOptionViewItemV4 opt = option;
    if (reverse)
        opt.rect.adjust(width,0,0,0);
    else
        opt.rect.adjust(0,0,-width,0);

    if (!(index.model()->flags(index) & Qt::ItemIsEnabled)) {
        opt.showDecorationSelected = true;
        opt.state |= QStyle::State_Selected;
    }

    QItemDelegate::paint(painter, opt, index);

    if (reverse)
        opt.rect = QRect(option.rect.x(), option.rect.y(), width, option.rect.height());
    else
        opt.rect = QRect(option.rect.x() + option.rect.width() - width, option.rect.y(),
                         width, option.rect.height());

    // Draw >
    if (index.model()->hasChildren(index)) {
        const QWidget *view = opt.widget;
        QStyle *style = view ? view->style() : QApplication::style();
        style->drawPrimitive(QStyle::PE_IndicatorColumnViewArrow, &opt, painter, view);
    }
}

QT_END_NAMESPACE

#include "moc_qcolumnview.cpp"

#endif // QT_NO_COLUMNVIEW
