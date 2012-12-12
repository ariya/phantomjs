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
#include "qtreeview.h"

#ifndef QT_NO_TREEVIEW
#include <qheaderview.h>
#include <qitemdelegate.h>
#include <qapplication.h>
#include <qscrollbar.h>
#include <qpainter.h>
#include <qstack.h>
#include <qstyle.h>
#include <qstyleoption.h>
#include <qevent.h>
#include <qpen.h>
#include <qdebug.h>
#ifndef QT_NO_ACCESSIBILITY
#include <qaccessible.h>
#include <qaccessible2.h>
#endif

#include <private/qtreeview_p.h>

QT_BEGIN_NAMESPACE

/*!
    \class QTreeView
    \brief The QTreeView class provides a default model/view implementation of a tree view.

    \ingroup model-view
    \ingroup advanced


    A QTreeView implements a tree representation of items from a
    model. This class is used to provide standard hierarchical lists that
    were previously provided by the \c QListView class, but using the more
    flexible approach provided by Qt's model/view architecture.

    The QTreeView class is one of the \l{Model/View Classes} and is part of
    Qt's \l{Model/View Programming}{model/view framework}.

    QTreeView implements the interfaces defined by the
    QAbstractItemView class to allow it to display data provided by
    models derived from the QAbstractItemModel class.

    It is simple to construct a tree view displaying data from a
    model. In the following example, the contents of a directory are
    supplied by a QFileSystemModel and displayed as a tree:

    \snippet doc/src/snippets/shareddirmodel/main.cpp 3
    \snippet doc/src/snippets/shareddirmodel/main.cpp 6

    The model/view architecture ensures that the contents of the tree view
    are updated as the model changes.

    Items that have children can be in an expanded (children are
    visible) or collapsed (children are hidden) state. When this state
    changes a collapsed() or expanded() signal is emitted with the
    model index of the relevant item.

    The amount of indentation used to indicate levels of hierarchy is
    controlled by the \l indentation property.

    Headers in tree views are constructed using the QHeaderView class and can
    be hidden using \c{header()->hide()}. Note that each header is configured
    with its \l{QHeaderView::}{stretchLastSection} property set to true,
    ensuring that the view does not waste any of the space assigned to it for
    its header. If this value is set to true, this property will override the
    resize mode set on the last section in the header.


    \section1 Key Bindings

    QTreeView supports a set of key bindings that enable the user to
    navigate in the view and interact with the contents of items:

    \table
    \header \o Key \o Action
    \row \o Up   \o Moves the cursor to the item in the same column on
         the previous row. If the parent of the current item has no more rows to
         navigate to, the cursor moves to the relevant item in the last row
         of the sibling that precedes the parent.
    \row \o Down \o Moves the cursor to the item in the same column on
         the next row. If the parent of the current item has no more rows to
         navigate to, the cursor moves to the relevant item in the first row
         of the sibling that follows the parent.
    \row \o Left  \o Hides the children of the current item (if present)
         by collapsing a branch.
    \row \o Minus  \o Same as LeftArrow.
    \row \o Right \o Reveals the children of the current item (if present)
         by expanding a branch.
    \row \o Plus  \o Same as RightArrow.
    \row \o Asterisk  \o Expands all children of the current item (if present).
    \row \o PageUp   \o Moves the cursor up one page.
    \row \o PageDown \o Moves the cursor down one page.
    \row \o Home \o Moves the cursor to an item in the same column of the first
         row of the first top-level item in the model.
    \row \o End  \o Moves the cursor to an item in the same column of the last
         row of the last top-level item in the model.
    \row \o F2   \o In editable models, this opens the current item for editing.
         The Escape key can be used to cancel the editing process and revert
         any changes to the data displayed.
    \endtable

    \omit
    Describe the expanding/collapsing concept if not covered elsewhere.
    \endomit

    \table 100%
    \row \o \inlineimage windowsxp-treeview.png Screenshot of a Windows XP style tree view
         \o \inlineimage macintosh-treeview.png Screenshot of a Macintosh style tree view
         \o \inlineimage plastique-treeview.png Screenshot of a Plastique style tree view
    \row \o A \l{Windows XP Style Widget Gallery}{Windows XP style} tree view.
         \o A \l{Macintosh Style Widget Gallery}{Macintosh style} tree view.
         \o A \l{Plastique Style Widget Gallery}{Plastique style} tree view.
    \endtable

    \section1 Improving Performance

    It is possible to give the view hints about the data it is handling in order
    to improve its performance when displaying large numbers of items. One approach
    that can be taken for views that are intended to display items with equal heights
    is to set the \l uniformRowHeights property to true.

    \sa QListView, QTreeWidget, {View Classes}, QAbstractItemModel, QAbstractItemView,
        {Dir View Example}
*/


/*!
  \fn void QTreeView::expanded(const QModelIndex &index)

  This signal is emitted when the item specified by \a index is expanded.
*/


/*!
  \fn void QTreeView::collapsed(const QModelIndex &index)

  This signal is emitted when the item specified by \a index is collapsed.
*/

/*!
    Constructs a tree view with a \a parent to represent a model's
    data. Use setModel() to set the model.

    \sa QAbstractItemModel
*/
QTreeView::QTreeView(QWidget *parent)
    : QAbstractItemView(*new QTreeViewPrivate, parent)
{
    Q_D(QTreeView);
    d->initialize();
}

/*!
  \internal
*/
QTreeView::QTreeView(QTreeViewPrivate &dd, QWidget *parent)
    : QAbstractItemView(dd, parent)
{
    Q_D(QTreeView);
    d->initialize();
}

/*!
  Destroys the tree view.
*/
QTreeView::~QTreeView()
{
}

/*!
  \reimp
*/
void QTreeView::setModel(QAbstractItemModel *model)
{
    Q_D(QTreeView);
    if (model == d->model)
        return;
    if (d->model && d->model != QAbstractItemModelPrivate::staticEmptyModel()) {
        disconnect(d->model, SIGNAL(rowsRemoved(QModelIndex,int,int)),
                this, SLOT(rowsRemoved(QModelIndex,int,int)));

        disconnect(d->model, SIGNAL(modelAboutToBeReset()), this, SLOT(_q_modelAboutToBeReset()));
    }

    if (d->selectionModel) { // support row editing
        disconnect(d->selectionModel, SIGNAL(currentRowChanged(QModelIndex,QModelIndex)),
                   d->model, SLOT(submit()));
        disconnect(d->model, SIGNAL(rowsRemoved(QModelIndex,int,int)),
                   this, SLOT(rowsRemoved(QModelIndex,int,int)));
        disconnect(d->model, SIGNAL(modelAboutToBeReset()), this, SLOT(_q_modelAboutToBeReset()));
    }
    d->viewItems.clear();
    d->expandedIndexes.clear();
    d->hiddenIndexes.clear();
    d->header->setModel(model);
    QAbstractItemView::setModel(model);

    // QAbstractItemView connects to a private slot
    disconnect(d->model, SIGNAL(rowsRemoved(QModelIndex,int,int)),
               this, SLOT(_q_rowsRemoved(QModelIndex,int,int)));
    // do header layout after the tree
    disconnect(d->model, SIGNAL(layoutChanged()),
               d->header, SLOT(_q_layoutChanged()));
    // QTreeView has a public slot for this
    connect(d->model, SIGNAL(rowsRemoved(QModelIndex,int,int)),
            this, SLOT(rowsRemoved(QModelIndex,int,int)));

    connect(d->model, SIGNAL(modelAboutToBeReset()), SLOT(_q_modelAboutToBeReset()));

    if (d->sortingEnabled)
        d->_q_sortIndicatorChanged(header()->sortIndicatorSection(), header()->sortIndicatorOrder());
}

/*!
  \reimp
*/
void QTreeView::setRootIndex(const QModelIndex &index)
{
    Q_D(QTreeView);
    d->header->setRootIndex(index);
    QAbstractItemView::setRootIndex(index);
}

/*!
  \reimp
*/
void QTreeView::setSelectionModel(QItemSelectionModel *selectionModel)
{
    Q_D(QTreeView);
    Q_ASSERT(selectionModel);
    if (d->selectionModel) {
        // support row editing
        disconnect(d->selectionModel, SIGNAL(currentRowChanged(QModelIndex,QModelIndex)),
                   d->model, SLOT(submit()));
    }

    d->header->setSelectionModel(selectionModel);
    QAbstractItemView::setSelectionModel(selectionModel);

    if (d->selectionModel) {
        // support row editing
        connect(d->selectionModel, SIGNAL(currentRowChanged(QModelIndex,QModelIndex)),
                d->model, SLOT(submit()));
    }
}

/*!
  Returns the header for the tree view.

  \sa QAbstractItemModel::headerData()
*/
QHeaderView *QTreeView::header() const
{
    Q_D(const QTreeView);
    return d->header;
}

/*!
    Sets the header for the tree view, to the given \a header.

    The view takes ownership over the given \a header and deletes it
    when a new header is set.

    \sa QAbstractItemModel::headerData()
*/
void QTreeView::setHeader(QHeaderView *header)
{
    Q_D(QTreeView);
    if (header == d->header || !header)
        return;
    if (d->header && d->header->parent() == this)
        delete d->header;
    d->header = header;
    d->header->setParent(this);

    if (!d->header->model()) {
        d->header->setModel(d->model);
        if (d->selectionModel)
            d->header->setSelectionModel(d->selectionModel);
    }

    connect(d->header, SIGNAL(sectionResized(int,int,int)),
            this, SLOT(columnResized(int,int,int)));
    connect(d->header, SIGNAL(sectionMoved(int,int,int)),
            this, SLOT(columnMoved()));
    connect(d->header, SIGNAL(sectionCountChanged(int,int)),
            this, SLOT(columnCountChanged(int,int)));
    connect(d->header, SIGNAL(sectionHandleDoubleClicked(int)),
            this, SLOT(resizeColumnToContents(int)));
    connect(d->header, SIGNAL(geometriesChanged()),
            this, SLOT(updateGeometries()));

    setSortingEnabled(d->sortingEnabled);
}

/*!
  \property QTreeView::autoExpandDelay
  \brief The delay time before items in a tree are opened during a drag and drop operation.
  \since 4.3

  This property holds the amount of time in milliseconds that the user must wait over
  a node before that node will automatically open or close.  If the time is
  set to less then 0 then it will not be activated.

  By default, this property has a value of -1, meaning that auto-expansion is disabled.
*/
int QTreeView::autoExpandDelay() const
{
    Q_D(const QTreeView);
    return d->autoExpandDelay;
}

void QTreeView::setAutoExpandDelay(int delay)
{
    Q_D(QTreeView);
    d->autoExpandDelay = delay;
}

/*!
  \property QTreeView::indentation
  \brief indentation of the items in the tree view.

  This property holds the indentation measured in pixels of the items for each
  level in the tree view. For top-level items, the indentation specifies the
  horizontal distance from the viewport edge to the items in the first column;
  for child items, it specifies their indentation from their parent items.

  By default, this property has a value of 20.
*/
int QTreeView::indentation() const
{
    Q_D(const QTreeView);
    return d->indent;
}

void QTreeView::setIndentation(int i)
{
    Q_D(QTreeView);
    if (i != d->indent) {
        d->indent = i;
        d->viewport->update();
    }
}

/*!
  \property QTreeView::rootIsDecorated
  \brief whether to show controls for expanding and collapsing top-level items

  Items with children are typically shown with controls to expand and collapse
  them, allowing their children to be shown or hidden. If this property is
  false, these controls are not shown for top-level items. This can be used to
  make a single level tree structure appear like a simple list of items.

  By default, this property is true.
*/
bool QTreeView::rootIsDecorated() const
{
    Q_D(const QTreeView);
    return d->rootDecoration;
}

void QTreeView::setRootIsDecorated(bool show)
{
    Q_D(QTreeView);
    if (show != d->rootDecoration) {
        d->rootDecoration = show;
        d->viewport->update();
    }
}

/*!
  \property QTreeView::uniformRowHeights
  \brief whether all items in the treeview have the same height

  This property should only be set to true if it is guaranteed that all items
  in the view has the same height. This enables the view to do some
  optimizations.

  The height is obtained from the first item in the view.  It is updated
  when the data changes on that item.

  By default, this property is false.
*/
bool QTreeView::uniformRowHeights() const
{
    Q_D(const QTreeView);
    return d->uniformRowHeights;
}

void QTreeView::setUniformRowHeights(bool uniform)
{
    Q_D(QTreeView);
    d->uniformRowHeights = uniform;
}

/*!
  \property QTreeView::itemsExpandable
  \brief whether the items are expandable by the user.

  This property holds whether the user can expand and collapse items
  interactively.

  By default, this property is true.

*/
bool QTreeView::itemsExpandable() const
{
    Q_D(const QTreeView);
    return d->itemsExpandable;
}

void QTreeView::setItemsExpandable(bool enable)
{
    Q_D(QTreeView);
    d->itemsExpandable = enable;
}

/*!
  \property QTreeView::expandsOnDoubleClick
  \since 4.4
  \brief whether the items can be expanded by double-clicking.

  This property holds whether the user can expand and collapse items
  by double-clicking. The default value is true.

  \sa itemsExpandable
*/
bool QTreeView::expandsOnDoubleClick() const
{
    Q_D(const QTreeView);
    return d->expandsOnDoubleClick;
}

void QTreeView::setExpandsOnDoubleClick(bool enable)
{
    Q_D(QTreeView);
    d->expandsOnDoubleClick = enable;
}

/*!
  Returns the horizontal position of the \a column in the viewport.
*/
int QTreeView::columnViewportPosition(int column) const
{
    Q_D(const QTreeView);
    return d->header->sectionViewportPosition(column);
}

/*!
  Returns the width of the \a column.

  \sa resizeColumnToContents(), setColumnWidth()
*/
int QTreeView::columnWidth(int column) const
{
    Q_D(const QTreeView);
    return d->header->sectionSize(column);
}

/*!
  \since 4.2

  Sets the width of the given \a column to the \a width specified.

  \sa columnWidth(), resizeColumnToContents()
*/
void QTreeView::setColumnWidth(int column, int width)
{
    Q_D(QTreeView);
    d->header->resizeSection(column, width);
}

/*!
  Returns the column in the tree view whose header covers the \a x
  coordinate given.
*/
int QTreeView::columnAt(int x) const
{
    Q_D(const QTreeView);
    return d->header->logicalIndexAt(x);
}

/*!
    Returns true if the \a column is hidden; otherwise returns false.

    \sa hideColumn(), isRowHidden()
*/
bool QTreeView::isColumnHidden(int column) const
{
    Q_D(const QTreeView);
    return d->header->isSectionHidden(column);
}

/*!
  If \a hide is true the \a column is hidden, otherwise the \a column is shown.

  \sa hideColumn(), setRowHidden()
*/
void QTreeView::setColumnHidden(int column, bool hide)
{
    Q_D(QTreeView);
    if (column < 0 || column >= d->header->count())
        return;
    d->header->setSectionHidden(column, hide);
}

/*!
  \property QTreeView::headerHidden
  \brief whether the header is shown or not.
  \since 4.4

  If this property is true, the header is not shown otherwise it is.
  The default value is false.

  \sa header()
*/
bool QTreeView::isHeaderHidden() const
{
    Q_D(const QTreeView);
    return d->header->isHidden();
}

void QTreeView::setHeaderHidden(bool hide)
{
    Q_D(QTreeView);
    d->header->setHidden(hide);
}

/*!
    Returns true if the item in the given \a row of the \a parent is hidden;
    otherwise returns false.

    \sa setRowHidden(), isColumnHidden()
*/
bool QTreeView::isRowHidden(int row, const QModelIndex &parent) const
{
    Q_D(const QTreeView);
    if (!d->model)
        return false;
    return d->isRowHidden(d->model->index(row, 0, parent));
}

/*!
  If \a hide is true the \a row with the given \a parent is hidden, otherwise the \a row is shown.

  \sa isRowHidden(), setColumnHidden()
*/
void QTreeView::setRowHidden(int row, const QModelIndex &parent, bool hide)
{
    Q_D(QTreeView);
    if (!d->model)
        return;
    QModelIndex index = d->model->index(row, 0, parent);
    if (!index.isValid())
        return;

    if (hide) {
        d->hiddenIndexes.insert(index);
    } else if(d->isPersistent(index)) { //if the index is not persistent, it cannot be in the set
        d->hiddenIndexes.remove(index);
    }

    d->doDelayedItemsLayout();
}

/*!
  \since 4.3

  Returns true if the item in first column in the given \a row
  of the \a parent is spanning all the columns; otherwise returns false.

  \sa setFirstColumnSpanned()
*/
bool QTreeView::isFirstColumnSpanned(int row, const QModelIndex &parent) const
{
    Q_D(const QTreeView);
    if (d->spanningIndexes.isEmpty() || !d->model)
        return false;
    QModelIndex index = d->model->index(row, 0, parent);
    for (int i = 0; i < d->spanningIndexes.count(); ++i)
        if (d->spanningIndexes.at(i) == index)
            return true;
    return false;
}

/*!
  \since 4.3

  If \a span is true the item in the first column in the \a row
  with the given \a parent is set to span all columns, otherwise all items
  on the \a row are shown.

  \sa isFirstColumnSpanned()
*/
void QTreeView::setFirstColumnSpanned(int row, const QModelIndex &parent, bool span)
{
    Q_D(QTreeView);
    if (!d->model)
        return;
    QModelIndex index = d->model->index(row, 0, parent);
    if (!index.isValid())
        return;

    if (span) {
        QPersistentModelIndex persistent(index);
        if (!d->spanningIndexes.contains(persistent))
            d->spanningIndexes.append(persistent);
    } else {
        QPersistentModelIndex persistent(index);
        int i = d->spanningIndexes.indexOf(persistent);
        if (i >= 0)
            d->spanningIndexes.remove(i);
    }

    d->executePostedLayout();
    int i = d->viewIndex(index);
    if (i >= 0)
        d->viewItems[i].spanning = span;

    d->viewport->update();
}

/*!
  \reimp
*/
void QTreeView::dataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight)
{
    Q_D(QTreeView);

    // if we are going to do a complete relayout anyway, there is no need to update
    if (d->delayedPendingLayout)
        return;

    // refresh the height cache here; we don't really lose anything by getting the size hint,
    // since QAbstractItemView::dataChanged() will get the visualRect for the items anyway

    bool sizeChanged = false;
    int topViewIndex = d->viewIndex(topLeft);
    if (topViewIndex == 0) {
        int newDefaultItemHeight = indexRowSizeHint(topLeft);
        sizeChanged = d->defaultItemHeight != newDefaultItemHeight;
        d->defaultItemHeight = newDefaultItemHeight;
    }

    if (topViewIndex != -1) {
        if (topLeft.row() == bottomRight.row()) {
            int oldHeight = d->itemHeight(topViewIndex);
            d->invalidateHeightCache(topViewIndex);
            sizeChanged |= (oldHeight != d->itemHeight(topViewIndex));
            if (topLeft.column() == 0)
                d->viewItems[topViewIndex].hasChildren = d->hasVisibleChildren(topLeft);
        } else {
            int bottomViewIndex = d->viewIndex(bottomRight);
            for (int i = topViewIndex; i <= bottomViewIndex; ++i) {
                int oldHeight = d->itemHeight(i);
                d->invalidateHeightCache(i);
                sizeChanged |= (oldHeight != d->itemHeight(i));
                if (topLeft.column() == 0)
                    d->viewItems[i].hasChildren = d->hasVisibleChildren(d->viewItems.at(i).index);
            }
        }
    }

    if (sizeChanged) {
        d->updateScrollBars();
        d->viewport->update();
    }
    QAbstractItemView::dataChanged(topLeft, bottomRight);
}

/*!
  Hides the \a column given.

  \note This function should only be called after the model has been
  initialized, as the view needs to know the number of columns in order to
  hide \a column.

  \sa showColumn(), setColumnHidden()
*/
void QTreeView::hideColumn(int column)
{
    Q_D(QTreeView);
    d->header->hideSection(column);
}

/*!
  Shows the given \a column in the tree view.

  \sa hideColumn(), setColumnHidden()
*/
void QTreeView::showColumn(int column)
{
    Q_D(QTreeView);
    d->header->showSection(column);
}

/*!
  \fn void QTreeView::expand(const QModelIndex &index)

  Expands the model item specified by the \a index.

  \sa expanded()
*/
void QTreeView::expand(const QModelIndex &index)
{
    Q_D(QTreeView);
    if (!d->isIndexValid(index))
        return;
    if (d->delayedPendingLayout) {
        //A complete relayout is going to be performed, just store the expanded index, no need to layout.
        if (d->storeExpanded(index))
            emit expanded(index);
        return;
    }

    int i = d->viewIndex(index);
    if (i != -1) { // is visible
        d->expand(i, true);
        if (!d->isAnimating()) {
            updateGeometries();
            d->viewport->update();
        }
    } else if (d->storeExpanded(index)) {
        emit expanded(index);
    }
}

/*!
  \fn void QTreeView::collapse(const QModelIndex &index)

  Collapses the model item specified by the \a index.

  \sa collapsed()
*/
void QTreeView::collapse(const QModelIndex &index)
{
    Q_D(QTreeView);
    if (!d->isIndexValid(index))
        return;
    //if the current item is now invisible, the autoscroll will expand the tree to see it, so disable the autoscroll
    d->delayedAutoScroll.stop();

    if (d->delayedPendingLayout) {
        //A complete relayout is going to be performed, just un-store the expanded index, no need to layout.
        if (d->isPersistent(index) && d->expandedIndexes.remove(index))
            emit collapsed(index);
        return;
    }
    int i = d->viewIndex(index);
    if (i != -1) { // is visible
        d->collapse(i, true);
        if (!d->isAnimating()) {
            updateGeometries();
            viewport()->update();
        }
    } else {
        if (d->isPersistent(index) && d->expandedIndexes.remove(index))
            emit collapsed(index);
    }
}

/*!
  \fn bool QTreeView::isExpanded(const QModelIndex &index) const

  Returns true if the model item \a index is expanded; otherwise returns
  false.

  \sa expand(), expanded(), setExpanded()
*/
bool QTreeView::isExpanded(const QModelIndex &index) const
{
    Q_D(const QTreeView);
    return d->isIndexExpanded(index);
}

/*!
  Sets the item referred to by \a index to either collapse or expanded,
  depending on the value of \a expanded.

  \sa expanded(), expand(), isExpanded()
*/
void QTreeView::setExpanded(const QModelIndex &index, bool expanded)
{
    if (expanded)
        this->expand(index);
    else
        this->collapse(index);
}

/*!
    \since 4.2
    \property QTreeView::sortingEnabled
    \brief whether sorting is enabled

    If this property is true, sorting is enabled for the tree; if the property
    is false, sorting is not enabled. The default value is false.

    \note In order to avoid performance issues, it is recommended that
    sorting is enabled \e after inserting the items into the tree.
    Alternatively, you could also insert the items into a list before inserting
    the items into the tree.

    \sa sortByColumn()
*/

void QTreeView::setSortingEnabled(bool enable)
{
    Q_D(QTreeView);
    header()->setSortIndicatorShown(enable);
    header()->setClickable(enable);
    if (enable) {
        //sortByColumn has to be called before we connect or set the sortingEnabled flag
        // because otherwise it will not call sort on the model.
        sortByColumn(header()->sortIndicatorSection(), header()->sortIndicatorOrder());
        connect(header(), SIGNAL(sortIndicatorChanged(int,Qt::SortOrder)),
                this, SLOT(_q_sortIndicatorChanged(int,Qt::SortOrder)), Qt::UniqueConnection);
    } else {
        disconnect(header(), SIGNAL(sortIndicatorChanged(int,Qt::SortOrder)),
                   this, SLOT(_q_sortIndicatorChanged(int,Qt::SortOrder)));
    }
    d->sortingEnabled = enable;
}

bool QTreeView::isSortingEnabled() const
{
    Q_D(const QTreeView);
    return d->sortingEnabled;
}

/*!
    \since 4.2
    \property QTreeView::animated
    \brief whether animations are enabled

    If this property is true the treeview will animate expandsion
    and collasping of branches. If this property is false, the treeview
    will expand or collapse branches immediately without showing
    the animation.

    By default, this property is false.
*/

void QTreeView::setAnimated(bool animate)
{
    Q_D(QTreeView);
    d->animationsEnabled = animate;
}

bool QTreeView::isAnimated() const
{
    Q_D(const QTreeView);
    return d->animationsEnabled;
}

/*!
    \since 4.2
    \property QTreeView::allColumnsShowFocus
    \brief whether items should show keyboard focus using all columns

    If this property is true all columns will show focus, otherwise only
    one column will show focus.

    The default is false.
*/

void QTreeView::setAllColumnsShowFocus(bool enable)
{
    Q_D(QTreeView);
    if (d->allColumnsShowFocus == enable)
        return;
    d->allColumnsShowFocus = enable;
    d->viewport->update();
}

bool QTreeView::allColumnsShowFocus() const
{
    Q_D(const QTreeView);
    return d->allColumnsShowFocus;
}

/*!
    \property QTreeView::wordWrap
    \brief the item text word-wrapping policy
    \since 4.3

    If this property is true then the item text is wrapped where
    necessary at word-breaks; otherwise it is not wrapped at all.
    This property is false by default.

    Note that even if wrapping is enabled, the cell will not be
    expanded to fit all text. Ellipsis will be inserted according to
    the current \l{QAbstractItemView::}{textElideMode}.
*/
void QTreeView::setWordWrap(bool on)
{
    Q_D(QTreeView);
    if (d->wrapItemText == on)
        return;
    d->wrapItemText = on;
    d->doDelayedItemsLayout();
}

bool QTreeView::wordWrap() const
{
    Q_D(const QTreeView);
    return d->wrapItemText;
}


/*!
  \reimp
 */
void QTreeView::keyboardSearch(const QString &search)
{
    Q_D(QTreeView);
    if (!d->model->rowCount(d->root) || !d->model->columnCount(d->root))
        return;

    QModelIndex start;
    if (currentIndex().isValid())
        start = currentIndex();
    else
        start = d->model->index(0, 0, d->root);

    bool skipRow = false;
    bool keyboardTimeWasValid = d->keyboardInputTime.isValid();
    qint64 keyboardInputTimeElapsed = d->keyboardInputTime.restart();
    if (search.isEmpty() || !keyboardTimeWasValid
        || keyboardInputTimeElapsed > QApplication::keyboardInputInterval()) {
        d->keyboardInput = search;
        skipRow = currentIndex().isValid(); //if it is not valid we should really start at QModelIndex(0,0)
    } else {
        d->keyboardInput += search;
    }

    // special case for searches with same key like 'aaaaa'
    bool sameKey = false;
    if (d->keyboardInput.length() > 1) {
        int c = d->keyboardInput.count(d->keyboardInput.at(d->keyboardInput.length() - 1));
        sameKey = (c == d->keyboardInput.length());
        if (sameKey)
            skipRow = true;
    }

    // skip if we are searching for the same key or a new search started
    if (skipRow) {
        if (indexBelow(start).isValid())
            start = indexBelow(start);
        else
            start = d->model->index(0, start.column(), d->root);
    }

    d->executePostedLayout();
    int startIndex = d->viewIndex(start);
    if (startIndex <= -1)
        return;

    int previousLevel = -1;
    int bestAbove = -1;
    int bestBelow = -1;
    QString searchString = sameKey ? QString(d->keyboardInput.at(0)) : d->keyboardInput;
    for (int i = 0; i < d->viewItems.count(); ++i) {
        if ((int)d->viewItems.at(i).level > previousLevel) {
            QModelIndex searchFrom = d->viewItems.at(i).index;
            if (searchFrom.parent() == start.parent())
                searchFrom = start;
            QModelIndexList match = d->model->match(searchFrom, Qt::DisplayRole, searchString);
            if (match.count()) {
                int hitIndex = d->viewIndex(match.at(0));
                if (hitIndex >= 0 && hitIndex < startIndex)
                    bestAbove = bestAbove == -1 ? hitIndex : qMin(hitIndex, bestAbove);
                else if (hitIndex >= startIndex)
                    bestBelow = bestBelow == -1 ? hitIndex : qMin(hitIndex, bestBelow);
            }
        }
        previousLevel = d->viewItems.at(i).level;
    }

    QModelIndex index;
    if (bestBelow > -1)
        index = d->viewItems.at(bestBelow).index;
    else if (bestAbove > -1)
        index = d->viewItems.at(bestAbove).index;

    if (index.isValid()) {
        QItemSelectionModel::SelectionFlags flags = (d->selectionMode == SingleSelection
                                                     ? QItemSelectionModel::SelectionFlags(
                                                         QItemSelectionModel::ClearAndSelect
                                                         |d->selectionBehaviorFlags())
                                                     : QItemSelectionModel::SelectionFlags(
                                                         QItemSelectionModel::NoUpdate));
        selectionModel()->setCurrentIndex(index, flags);
    }
}

/*!
  Returns the rectangle on the viewport occupied by the item at \a index.
  If the index is not visible or explicitly hidden, the returned rectangle is invalid.
*/
QRect QTreeView::visualRect(const QModelIndex &index) const
{
    Q_D(const QTreeView);

    if (!d->isIndexValid(index) || isIndexHidden(index))
        return QRect();

    d->executePostedLayout();

    int vi = d->viewIndex(index);
    if (vi < 0)
        return QRect();

    bool spanning = d->viewItems.at(vi).spanning;

    // if we have a spanning item, make the selection stretch from left to right
    int x = (spanning ? 0 : columnViewportPosition(index.column()));
    int w = (spanning ? d->header->length() : columnWidth(index.column()));
    // handle indentation
    if (index.column() == 0) {
        int i = d->indentationForItem(vi);
        w -= i;
        if (!isRightToLeft())
            x += i;
    }

    int y = d->coordinateForItem(vi);
    int h = d->itemHeight(vi);

    return QRect(x, y, w, h);
}

/*!
    Scroll the contents of the tree view until the given model item
    \a index is visible. The \a hint parameter specifies more
    precisely where the item should be located after the
    operation.
    If any of the parents of the model item are collapsed, they will
    be expanded to ensure that the model item is visible.
*/
void QTreeView::scrollTo(const QModelIndex &index, ScrollHint hint)
{
    Q_D(QTreeView);

    if (!d->isIndexValid(index))
        return;

    d->executePostedLayout();
    d->updateScrollBars();

    // Expand all parents if the parent(s) of the node are not expanded.
    QModelIndex parent = index.parent();
    while (parent.isValid() && state() == NoState && d->itemsExpandable) {
        if (!isExpanded(parent))
            expand(parent);
        parent = d->model->parent(parent);
    }

    int item = d->viewIndex(index);
    if (item < 0)
        return;

    QRect area = d->viewport->rect();

    // vertical
    if (verticalScrollMode() == QAbstractItemView::ScrollPerItem) {
        int top = verticalScrollBar()->value();
        int bottom = top + verticalScrollBar()->pageStep();
        if (hint == EnsureVisible && item >= top && item < bottom) {
            // nothing to do
        } else if (hint == PositionAtTop || (hint == EnsureVisible && item < top)) {
            verticalScrollBar()->setValue(item);
        } else { // PositionAtBottom or PositionAtCenter
            const int currentItemHeight = d->itemHeight(item);
            int y = (hint == PositionAtCenter
                 //we center on the current item with a preference to the top item (ie. -1)
                     ? area.height() / 2 + currentItemHeight - 1
                 //otherwise we simply take the whole space
                     : area.height());
            if (y > currentItemHeight) {
                while (item >= 0) {
                    y -= d->itemHeight(item);
                    if (y < 0) { //there is no more space left
                        item++;
                        break;
                    }
                    item--;
                }
            }
            verticalScrollBar()->setValue(item);
        }
    } else { // ScrollPerPixel
        QRect rect(columnViewportPosition(index.column()),
                   d->coordinateForItem(item), // ### slow for items outside the view
                   columnWidth(index.column()),
                   d->itemHeight(item));

        if (rect.isEmpty()) {
            // nothing to do
        } else if (hint == EnsureVisible && area.contains(rect)) {
            d->viewport->update(rect);
            // nothing to do
        } else {
            bool above = (hint == EnsureVisible
                        && (rect.top() < area.top()
                            || area.height() < rect.height()));
            bool below = (hint == EnsureVisible
                        && rect.bottom() > area.bottom()
                        && rect.height() < area.height());

            int verticalValue = verticalScrollBar()->value();
            if (hint == PositionAtTop || above)
                verticalValue += rect.top();
            else if (hint == PositionAtBottom || below)
                verticalValue += rect.bottom() - area.height();
            else if (hint == PositionAtCenter)
                verticalValue += rect.top() - ((area.height() - rect.height()) / 2);
            verticalScrollBar()->setValue(verticalValue);
        }
    }
    // horizontal
    int viewportWidth = d->viewport->width();
    int horizontalOffset = d->header->offset();
    int horizontalPosition = d->header->sectionPosition(index.column());
    int cellWidth = d->header->sectionSize(index.column());

    if (hint == PositionAtCenter) {
        horizontalScrollBar()->setValue(horizontalPosition - ((viewportWidth - cellWidth) / 2));
    } else {
        if (horizontalPosition - horizontalOffset < 0 || cellWidth > viewportWidth)
            horizontalScrollBar()->setValue(horizontalPosition);
        else if (horizontalPosition - horizontalOffset + cellWidth > viewportWidth)
            horizontalScrollBar()->setValue(horizontalPosition - viewportWidth + cellWidth);
    }
}

/*!
  \reimp
*/
void QTreeView::timerEvent(QTimerEvent *event)
{
    Q_D(QTreeView);
    if (event->timerId() == d->columnResizeTimerID) {
        updateGeometries();
        killTimer(d->columnResizeTimerID);
        d->columnResizeTimerID = 0;
        QRect rect;
        int viewportHeight = d->viewport->height();
        int viewportWidth = d->viewport->width();
        for (int i = d->columnsToUpdate.size() - 1; i >= 0; --i) {
            int column = d->columnsToUpdate.at(i);
            int x = columnViewportPosition(column);
            if (isRightToLeft())
                rect |= QRect(0, 0, x + columnWidth(column), viewportHeight);
            else
                rect |= QRect(x, 0, viewportWidth - x, viewportHeight);
        }
        d->viewport->update(rect.normalized());
        d->columnsToUpdate.clear();
    } else if (event->timerId() == d->openTimer.timerId()) {
        QPoint pos = d->viewport->mapFromGlobal(QCursor::pos());
        if (state() == QAbstractItemView::DraggingState
            && d->viewport->rect().contains(pos)) {
            QModelIndex index = indexAt(pos);
            setExpanded(index, !isExpanded(index));
        }
        d->openTimer.stop();
    }

    QAbstractItemView::timerEvent(event);
}

/*!
  \reimp
*/
#ifndef QT_NO_DRAGANDDROP
void QTreeView::dragMoveEvent(QDragMoveEvent *event)
{
    Q_D(QTreeView);
    if (d->autoExpandDelay >= 0)
        d->openTimer.start(d->autoExpandDelay, this);
    QAbstractItemView::dragMoveEvent(event);
}
#endif

/*!
  \reimp
*/
bool QTreeView::viewportEvent(QEvent *event)
{
    Q_D(QTreeView);
    switch (event->type()) {
    case QEvent::HoverEnter:
    case QEvent::HoverLeave:
    case QEvent::HoverMove: {
        QHoverEvent *he = static_cast<QHoverEvent*>(event);
        int oldBranch = d->hoverBranch;
        d->hoverBranch = d->itemDecorationAt(he->pos());
        if (oldBranch != d->hoverBranch) {
            //we need to paint the whole items (including the decoration) so that when the user
            //moves the mouse over those elements they are updated
            if (oldBranch >= 0) {
                int y = d->coordinateForItem(oldBranch);
                int h = d->itemHeight(oldBranch);
                viewport()->update(QRect(0, y, viewport()->width(), h));
            }
            if (d->hoverBranch >= 0) {
                int y = d->coordinateForItem(d->hoverBranch);
                int h = d->itemHeight(d->hoverBranch);
                viewport()->update(QRect(0, y, viewport()->width(), h));
            }
        }
        break; }
    default:
        break;
    }
    return QAbstractItemView::viewportEvent(event);
}

/*!
  \reimp
*/
void QTreeView::paintEvent(QPaintEvent *event)
{
    Q_D(QTreeView);
    d->executePostedLayout();
    QPainter painter(viewport());
#ifndef QT_NO_ANIMATION
    if (d->isAnimating()) {
        drawTree(&painter, event->region() - d->animatedOperation.rect());
        d->drawAnimatedOperation(&painter);
    } else
#endif //QT_NO_ANIMATION
    {
        drawTree(&painter, event->region());
#ifndef QT_NO_DRAGANDDROP
        d->paintDropIndicator(&painter);
#endif
    }
}

void QTreeViewPrivate::paintAlternatingRowColors(QPainter *painter, QStyleOptionViewItemV4 *option, int y, int bottom) const
{
    Q_Q(const QTreeView);
    if (!alternatingColors || !q->style()->styleHint(QStyle::SH_ItemView_PaintAlternatingRowColorsForEmptyArea, option, q))
        return;
    int rowHeight = defaultItemHeight;
    if (rowHeight <= 0) {
        rowHeight = itemDelegate->sizeHint(*option, QModelIndex()).height();
        if (rowHeight <= 0)
            return;
    }
    while (y <= bottom) {
        option->rect.setRect(0, y, viewport->width(), rowHeight);
        if (current & 1) {
            option->features |= QStyleOptionViewItemV2::Alternate;
        } else {
            option->features &= ~QStyleOptionViewItemV2::Alternate;
        }
        ++current;
        q->style()->drawPrimitive(QStyle::PE_PanelItemViewRow, option, painter, q);
        y += rowHeight;
    }
}

bool QTreeViewPrivate::expandOrCollapseItemAtPos(const QPoint &pos)
{
    Q_Q(QTreeView);
    // we want to handle mousePress in EditingState (persistent editors)
    if ((state != QAbstractItemView::NoState
		&& state != QAbstractItemView::EditingState)
		|| !viewport->rect().contains(pos))
        return true;

    int i = itemDecorationAt(pos);
    if ((i != -1) && itemsExpandable && hasVisibleChildren(viewItems.at(i).index)) {
        if (viewItems.at(i).expanded)
            collapse(i, true);
        else
            expand(i, true);
        if (!isAnimating()) {
            q->updateGeometries();
            viewport->update();
        }
        return true;
    }
    return false;
}

void QTreeViewPrivate::_q_modelDestroyed()
{
    //we need to clear that list because it contais QModelIndex to 
    //the model currently being destroyed
    viewItems.clear();
    QAbstractItemViewPrivate::_q_modelDestroyed();
}

/*!
  \reimp

  We have a QTreeView way of knowing what elements are on the viewport
*/
QItemViewPaintPairs QTreeViewPrivate::draggablePaintPairs(const QModelIndexList &indexes, QRect *r) const
{
    Q_ASSERT(r);
    Q_Q(const QTreeView);
    if (spanningIndexes.isEmpty())
        return QAbstractItemViewPrivate::draggablePaintPairs(indexes, r);
    QModelIndexList list;
    foreach (const QModelIndex &idx, indexes) {
        if (idx.column() > 0 && q->isFirstColumnSpanned(idx.row(), idx.parent()))
            continue;
        list << idx;
    }
    return QAbstractItemViewPrivate::draggablePaintPairs(list, r);
}

void QTreeViewPrivate::adjustViewOptionsForIndex(QStyleOptionViewItemV4 *option, const QModelIndex &current) const
{
    const int row = viewIndex(current); // get the index in viewItems[]
    option->state = option->state | (viewItems.at(row).expanded ? QStyle::State_Open : QStyle::State_None)
                                  | (viewItems.at(row).hasChildren ? QStyle::State_Children : QStyle::State_None)
                                  | (viewItems.at(row).hasMoreSiblings ? QStyle::State_Sibling : QStyle::State_None);

    option->showDecorationSelected = (selectionBehavior & QTreeView::SelectRows)
                                     || option->showDecorationSelected;

    QVector<int> logicalIndices; // index = visual index of visible columns only. data = logical index.
    QVector<QStyleOptionViewItemV4::ViewItemPosition> viewItemPosList; // vector of left/middle/end for each logicalIndex, visible columns only.
    const bool spanning = viewItems.at(row).spanning;
    const int left = (spanning ? header->visualIndex(0) : 0);
    const int right = (spanning ? header->visualIndex(0) : header->count() - 1 );
    calcLogicalIndices(&logicalIndices, &viewItemPosList, left, right);

    int columnIndex = 0;
    for (int visualIndex = 0; visualIndex < current.column(); ++visualIndex) {
        int logicalIndex = header->logicalIndex(visualIndex);
        if (!header->isSectionHidden(logicalIndex)) {
            ++columnIndex;
        }
    }

    option->viewItemPosition = viewItemPosList.at(columnIndex);
}


/*!
  \since 4.2
  Draws the part of the tree intersecting the given \a region using the specified
  \a painter.

  \sa paintEvent()
*/
void QTreeView::drawTree(QPainter *painter, const QRegion &region) const
{
    Q_D(const QTreeView);
    const QVector<QTreeViewItem> viewItems = d->viewItems;

    QStyleOptionViewItemV4 option = d->viewOptionsV4();
    const QStyle::State state = option.state;
    d->current = 0;

    if (viewItems.count() == 0 || d->header->count() == 0 || !d->itemDelegate) {
        d->paintAlternatingRowColors(painter, &option, 0, region.boundingRect().bottom()+1);
        return;
    }

    int firstVisibleItemOffset = 0;
    const int firstVisibleItem = d->firstVisibleItem(&firstVisibleItemOffset);
    if (firstVisibleItem < 0) {
        d->paintAlternatingRowColors(painter, &option, 0, region.boundingRect().bottom()+1);
        return;
    }

    const int viewportWidth = d->viewport->width();

    QVector<QRect> rects = region.rects();
    QVector<int> drawn;
    bool multipleRects = (rects.size() > 1);
    for (int a = 0; a < rects.size(); ++a) {
        const QRect area = (multipleRects
                            ? QRect(0, rects.at(a).y(), viewportWidth, rects.at(a).height())
                            : rects.at(a));
        d->leftAndRight = d->startAndEndColumns(area);

        int i = firstVisibleItem; // the first item at the top of the viewport
        int y = firstVisibleItemOffset; // we may only see part of the first item

        // start at the top of the viewport  and iterate down to the update area
        for (; i < viewItems.count(); ++i) {
            const int itemHeight = d->itemHeight(i);
            if (y + itemHeight > area.top())
                break;
            y += itemHeight;
        }

        // paint the visible rows
        for (; i < viewItems.count() && y <= area.bottom(); ++i) {
            const int itemHeight = d->itemHeight(i);
            option.rect.setRect(0, y, viewportWidth, itemHeight);
            option.state = state | (viewItems.at(i).expanded ? QStyle::State_Open : QStyle::State_None)
                                 | (viewItems.at(i).hasChildren ? QStyle::State_Children : QStyle::State_None)
                                 | (viewItems.at(i).hasMoreSiblings ? QStyle::State_Sibling : QStyle::State_None);
            d->current = i;
            d->spanning = viewItems.at(i).spanning;
            if (!multipleRects || !drawn.contains(i)) {
                drawRow(painter, option, viewItems.at(i).index);
                if (multipleRects)   // even if the rect only intersects the item,
                    drawn.append(i); // the entire item will be painted
            }
            y += itemHeight;
        }

        if (y <= area.bottom()) {
            d->current = i;
            d->paintAlternatingRowColors(painter, &option, y, area.bottom());
        }
    }
}

/// ### move to QObject :)
static inline bool ancestorOf(QObject *widget, QObject *other)
{
    for (QObject *parent = other; parent != 0; parent = parent->parent()) {
        if (parent == widget)
            return true;
    }
    return false;
}

void QTreeViewPrivate::calcLogicalIndices(QVector<int> *logicalIndices, QVector<QStyleOptionViewItemV4::ViewItemPosition> *itemPositions, int left, int right) const
{
    const int columnCount = header->count();
    /* 'left' and 'right' are the left-most and right-most visible visual indices.
       Compute the first visible logical indices before and after the left and right.
       We will use these values to determine the QStyleOptionViewItemV4::viewItemPosition. */
    int logicalIndexBeforeLeft = -1, logicalIndexAfterRight = -1;
    for (int visualIndex = left - 1; visualIndex >= 0; --visualIndex) {
        int logicalIndex = header->logicalIndex(visualIndex);
        if (!header->isSectionHidden(logicalIndex)) {
            logicalIndexBeforeLeft = logicalIndex;
            break;
        }
    }

    for (int visualIndex = left; visualIndex < columnCount; ++visualIndex) {
        int logicalIndex = header->logicalIndex(visualIndex);
        if (!header->isSectionHidden(logicalIndex)) {
            if (visualIndex > right) {
                logicalIndexAfterRight = logicalIndex;
                break;
            }
            logicalIndices->append(logicalIndex);
        }
    }

    itemPositions->resize(logicalIndices->count());
    for (int currentLogicalSection = 0; currentLogicalSection < logicalIndices->count(); ++currentLogicalSection) {
        const int headerSection = logicalIndices->at(currentLogicalSection);
        // determine the viewItemPosition depending on the position of column 0
        int nextLogicalSection = currentLogicalSection + 1 >= logicalIndices->count()
                                 ? logicalIndexAfterRight
                                 : logicalIndices->at(currentLogicalSection + 1);
        int prevLogicalSection = currentLogicalSection - 1 < 0
                                 ? logicalIndexBeforeLeft
                                 : logicalIndices->at(currentLogicalSection - 1);
        QStyleOptionViewItemV4::ViewItemPosition pos;
        if (columnCount == 1 || (nextLogicalSection == 0 && prevLogicalSection == -1)
            || (headerSection == 0 && nextLogicalSection == -1) || spanning)
            pos = QStyleOptionViewItemV4::OnlyOne;
        else if (headerSection == 0 || (nextLogicalSection != 0 && prevLogicalSection == -1))
            pos = QStyleOptionViewItemV4::Beginning;
        else if (nextLogicalSection == 0 || nextLogicalSection == -1)
            pos = QStyleOptionViewItemV4::End;
        else
            pos = QStyleOptionViewItemV4::Middle;
        (*itemPositions)[currentLogicalSection] = pos;
    }
}


/*!
    Draws the row in the tree view that contains the model item \a index,
    using the \a painter given. The \a option control how the item is
    displayed.

    \sa setAlternatingRowColors()
*/
void QTreeView::drawRow(QPainter *painter, const QStyleOptionViewItem &option,
                        const QModelIndex &index) const
{
    Q_D(const QTreeView);
    QStyleOptionViewItemV4 opt = option;
    const QPoint offset = d->scrollDelayOffset;
    const int y = option.rect.y() + offset.y();
    const QModelIndex parent = index.parent();
    const QHeaderView *header = d->header;
    const QModelIndex current = currentIndex();
    const QModelIndex hover = d->hover;
    const bool reverse = isRightToLeft();
    const QStyle::State state = opt.state;
    const bool spanning = d->spanning;
    const int left = (spanning ? header->visualIndex(0) : d->leftAndRight.first);
    const int right = (spanning ? header->visualIndex(0) : d->leftAndRight.second);
    const bool alternate = d->alternatingColors;
    const bool enabled = (state & QStyle::State_Enabled) != 0;
    const bool allColumnsShowFocus = d->allColumnsShowFocus;


    // when the row contains an index widget which has focus,
    // we want to paint the entire row as active
    bool indexWidgetHasFocus = false;
    if ((current.row() == index.row()) && !d->editorIndexHash.isEmpty()) {
        const int r = index.row();
        QWidget *fw = QApplication::focusWidget();
        for (int c = 0; c < header->count(); ++c) {
            QModelIndex idx = d->model->index(r, c, parent);
            if (QWidget *editor = indexWidget(idx)) {
                if (ancestorOf(editor, fw)) {
                    indexWidgetHasFocus = true;
                    break;
                }
            }
        }
    }

    const bool widgetHasFocus = hasFocus();
    bool currentRowHasFocus = false;
    if (allColumnsShowFocus && widgetHasFocus && current.isValid()) {
        // check if the focus index is before or after the visible columns
        const int r = index.row();
        for (int c = 0; c < left && !currentRowHasFocus; ++c) {
            QModelIndex idx = d->model->index(r, c, parent);
            currentRowHasFocus = (idx == current);
        }
        QModelIndex parent = d->model->parent(index);
        for (int c = right; c < header->count() && !currentRowHasFocus; ++c) {
            currentRowHasFocus = (d->model->index(r, c, parent) == current);
        }
    }

    // ### special case: treeviews with multiple columns draw
    // the selections differently than with only one column
    opt.showDecorationSelected = (d->selectionBehavior & SelectRows)
                                 || option.showDecorationSelected;

    int width, height = option.rect.height();
    int position;
    QModelIndex modelIndex;
    const bool hoverRow = selectionBehavior() == QAbstractItemView::SelectRows
                  && index.parent() == hover.parent()
                  && index.row() == hover.row();

    QVector<int> logicalIndices;
    QVector<QStyleOptionViewItemV4::ViewItemPosition> viewItemPosList; // vector of left/middle/end for each logicalIndex
    d->calcLogicalIndices(&logicalIndices, &viewItemPosList, left, right);

    for (int currentLogicalSection = 0; currentLogicalSection < logicalIndices.count(); ++currentLogicalSection) {
        int headerSection = logicalIndices.at(currentLogicalSection);
        position = columnViewportPosition(headerSection) + offset.x();
        width = header->sectionSize(headerSection);

        if (spanning) {
            int lastSection = header->logicalIndex(header->count() - 1);
            if (!reverse) {
                width = columnViewportPosition(lastSection) + header->sectionSize(lastSection) - position;
            } else {
                width += position - columnViewportPosition(lastSection);
                position = columnViewportPosition(lastSection);
            }
        }

        modelIndex = d->model->index(index.row(), headerSection, parent);
        if (!modelIndex.isValid())
            continue;
        opt.state = state;

        opt.viewItemPosition = viewItemPosList.at(currentLogicalSection);

        // fake activeness when row editor has focus
        if (indexWidgetHasFocus)
            opt.state |= QStyle::State_Active;

        if (d->selectionModel->isSelected(modelIndex))
            opt.state |= QStyle::State_Selected;
        if (widgetHasFocus && (current == modelIndex)) {
            if (allColumnsShowFocus)
                currentRowHasFocus = true;
            else
                opt.state |= QStyle::State_HasFocus;
        }
        if ((hoverRow || modelIndex == hover)
            && (option.showDecorationSelected || (d->hoverBranch == -1)))
            opt.state |= QStyle::State_MouseOver;
        else
            opt.state &= ~QStyle::State_MouseOver;

        if (enabled) {
            QPalette::ColorGroup cg;
            if ((d->model->flags(modelIndex) & Qt::ItemIsEnabled) == 0) {
                opt.state &= ~QStyle::State_Enabled;
                cg = QPalette::Disabled;
            } else if (opt.state & QStyle::State_Active) {
                cg = QPalette::Active;
            } else {
                cg = QPalette::Inactive;
            }
            opt.palette.setCurrentColorGroup(cg);
        }

        if (alternate) {
            if (d->current & 1) {
                opt.features |= QStyleOptionViewItemV2::Alternate;
            } else {
                opt.features &= ~QStyleOptionViewItemV2::Alternate;
            }
        }

        /* Prior to Qt 4.3, the background of the branch (in selected state and
           alternate row color was provided by the view. For backward compatibility,
           this is now delegated to the style using PE_PanelViewItemRow which
           does the appropriate fill */
        if (headerSection == 0) {
            const int i = d->indentationForItem(d->current);
            QRect branches(reverse ? position + width - i : position, y, i, height);
            const bool setClipRect = branches.width() > width;
            if (setClipRect) {
                painter->save();
                painter->setClipRect(QRect(position, y, width, height));
            }
            // draw background for the branch (selection + alternate row)
            opt.rect = branches;
            style()->drawPrimitive(QStyle::PE_PanelItemViewRow, &opt, painter, this);

            // draw background of the item (only alternate row). rest of the background
            // is provided by the delegate
            QStyle::State oldState = opt.state;
            opt.state &= ~QStyle::State_Selected;
            opt.rect.setRect(reverse ? position : i + position, y, width - i, height);
            style()->drawPrimitive(QStyle::PE_PanelItemViewRow, &opt, painter, this);
            opt.state = oldState;

            drawBranches(painter, branches, index);
            if (setClipRect)
                painter->restore();
        } else {
            QStyle::State oldState = opt.state;
            opt.state &= ~QStyle::State_Selected;
            opt.rect.setRect(position, y, width, height);
            style()->drawPrimitive(QStyle::PE_PanelItemViewRow, &opt, painter, this);
            opt.state = oldState;
        }

        d->delegateForIndex(modelIndex)->paint(painter, opt, modelIndex);
    }

    if (currentRowHasFocus) {
        QStyleOptionFocusRect o;
        o.QStyleOption::operator=(option);
        o.state |= QStyle::State_KeyboardFocusChange;
        QPalette::ColorGroup cg = (option.state & QStyle::State_Enabled)
                                  ? QPalette::Normal : QPalette::Disabled;
        o.backgroundColor = option.palette.color(cg, d->selectionModel->isSelected(index)
                                                 ? QPalette::Highlight : QPalette::Background);
        int x = 0;
        if (!option.showDecorationSelected)
            x = header->sectionPosition(0) + d->indentationForItem(d->current);
	QRect focusRect(x - header->offset(), y, header->length() - x, height);
        o.rect = style()->visualRect(layoutDirection(), d->viewport->rect(), focusRect);
        style()->drawPrimitive(QStyle::PE_FrameFocusRect, &o, painter);
        // if we show focus on all columns and the first section is moved,
        // we have to split the focus rect into two rects
        if (allColumnsShowFocus && !option.showDecorationSelected
            && header->sectionsMoved() && (header->visualIndex(0) != 0)) {
	    QRect sectionRect(0, y, header->sectionPosition(0), height); 
            o.rect = style()->visualRect(layoutDirection(), d->viewport->rect(), sectionRect);
            style()->drawPrimitive(QStyle::PE_FrameFocusRect, &o, painter);
        }
    }
}

/*!
  Draws the branches in the tree view on the same row as the model item
  \a index, using the \a painter given. The branches are drawn in the
  rectangle specified by \a rect.
*/
void QTreeView::drawBranches(QPainter *painter, const QRect &rect,
                             const QModelIndex &index) const
{
    Q_D(const QTreeView);
    const bool reverse = isRightToLeft();
    const int indent = d->indent;
    const int outer = d->rootDecoration ? 0 : 1;
    const int item = d->current;
    const QTreeViewItem &viewItem = d->viewItems.at(item);
    int level = viewItem.level;
    QRect primitive(reverse ? rect.left() : rect.right() + 1, rect.top(), indent, rect.height());

    QModelIndex parent = index.parent();
    QModelIndex current = parent;
    QModelIndex ancestor = current.parent();

    QStyleOptionViewItemV2 opt = viewOptions();
    QStyle::State extraFlags = QStyle::State_None;
    if (isEnabled())
        extraFlags |= QStyle::State_Enabled;
    if (window()->isActiveWindow())
        extraFlags |= QStyle::State_Active;
    QPoint oldBO = painter->brushOrigin();
    if (verticalScrollMode() == QAbstractItemView::ScrollPerPixel)
        painter->setBrushOrigin(QPoint(0, verticalOffset()));

    if (d->alternatingColors) {
        if (d->current & 1) {
            opt.features |= QStyleOptionViewItemV2::Alternate;
        } else {
            opt.features &= ~QStyleOptionViewItemV2::Alternate;
        }
    }

    // When hovering over a row, pass State_Hover for painting the branch
    // indicators if it has the decoration (aka branch) selected.
    bool hoverRow = selectionBehavior() == QAbstractItemView::SelectRows
                    && opt.showDecorationSelected
                    && index.parent() == d->hover.parent()
                    && index.row() == d->hover.row();

    if (d->selectionModel->isSelected(index))
        extraFlags |= QStyle::State_Selected;

    if (level >= outer) {
        // start with the innermost branch
        primitive.moveLeft(reverse ? primitive.left() : primitive.left() - indent);
        opt.rect = primitive;

        const bool expanded = viewItem.expanded;
        const bool children = viewItem.hasChildren;
        bool moreSiblings = viewItem.hasMoreSiblings;

        opt.state = QStyle::State_Item | extraFlags
                    | (moreSiblings ? QStyle::State_Sibling : QStyle::State_None)
                    | (children ? QStyle::State_Children : QStyle::State_None)
                    | (expanded ? QStyle::State_Open : QStyle::State_None);
        if (hoverRow || item == d->hoverBranch)
            opt.state |= QStyle::State_MouseOver;
        else
            opt.state &= ~QStyle::State_MouseOver;
        style()->drawPrimitive(QStyle::PE_IndicatorBranch, &opt, painter, this);
    }
    // then go out level by level
    for (--level; level >= outer; --level) { // we have already drawn the innermost branch
        primitive.moveLeft(reverse ? primitive.left() + indent : primitive.left() - indent);
        opt.rect = primitive;
        opt.state = extraFlags;
        bool moreSiblings = false;
        if (d->hiddenIndexes.isEmpty()) {
            moreSiblings = (d->model->rowCount(ancestor) - 1 > current.row());
        } else {
            int successor = item + viewItem.total + 1;
            while (successor < d->viewItems.size()
                   && d->viewItems.at(successor).level >= uint(level)) {
                const QTreeViewItem &successorItem = d->viewItems.at(successor);
                if (successorItem.level == uint(level)) {
                    moreSiblings = true;
                    break;
                }
                successor += successorItem.total + 1;
            }
        }
        if (moreSiblings)
            opt.state |= QStyle::State_Sibling;
        if (hoverRow || item == d->hoverBranch)
            opt.state |= QStyle::State_MouseOver;
        else
            opt.state &= ~QStyle::State_MouseOver;
        style()->drawPrimitive(QStyle::PE_IndicatorBranch, &opt, painter, this);
        current = ancestor;
        ancestor = current.parent();
    }
    painter->setBrushOrigin(oldBO);
}

/*!
  \reimp
*/
void QTreeView::mousePressEvent(QMouseEvent *event)
{
	Q_D(QTreeView);
    bool handled = false;
    if (style()->styleHint(QStyle::SH_Q3ListViewExpand_SelectMouseType, 0, this) == QEvent::MouseButtonPress)
        handled = d->expandOrCollapseItemAtPos(event->pos());
	if (!handled && d->itemDecorationAt(event->pos()) == -1)
        QAbstractItemView::mousePressEvent(event);
}

/*!
  \reimp
*/
void QTreeView::mouseReleaseEvent(QMouseEvent *event)
{
    Q_D(QTreeView);
    if (d->itemDecorationAt(event->pos()) == -1) {
        QAbstractItemView::mouseReleaseEvent(event);
    } else {
        if (state() == QAbstractItemView::DragSelectingState)
            setState(QAbstractItemView::NoState);
        if (style()->styleHint(QStyle::SH_Q3ListViewExpand_SelectMouseType, 0, this) == QEvent::MouseButtonRelease)
            d->expandOrCollapseItemAtPos(event->pos());
    }
}

/*!
  \reimp
*/
void QTreeView::mouseDoubleClickEvent(QMouseEvent *event)
{
    Q_D(QTreeView);
    if (state() != NoState || !d->viewport->rect().contains(event->pos()))
        return;

    int i = d->itemDecorationAt(event->pos());
    if (i == -1) {
        i = d->itemAtCoordinate(event->y());
        if (i == -1)
            return; // user clicked outside the items

        const QPersistentModelIndex firstColumnIndex = d->viewItems.at(i).index;
        const QPersistentModelIndex persistent = indexAt(event->pos());

        if (d->pressedIndex != persistent) {
            mousePressEvent(event);
            return;
        }

        // signal handlers may change the model
        emit doubleClicked(persistent);

        if (!persistent.isValid())
            return;

        if (edit(persistent, DoubleClicked, event) || state() != NoState)
            return; // the double click triggered editing

        if (!style()->styleHint(QStyle::SH_ItemView_ActivateItemOnSingleClick, 0, this))
            emit activated(persistent);

        d->executePostedLayout(); // we need to make sure viewItems is updated
        if (d->itemsExpandable
            && d->expandsOnDoubleClick
            && d->hasVisibleChildren(persistent)) {
            if (!((i < d->viewItems.count()) && (d->viewItems.at(i).index == firstColumnIndex))) {
                // find the new index of the item
                for (i = 0; i < d->viewItems.count(); ++i) {
                    if (d->viewItems.at(i).index == firstColumnIndex)
                        break;
                }
                if (i == d->viewItems.count())
                    return;
            }
            if (d->viewItems.at(i).expanded)
                d->collapse(i, true);
            else
                d->expand(i, true);
            updateGeometries();
            viewport()->update();
        }
    }
}

/*!
  \reimp
*/
void QTreeView::mouseMoveEvent(QMouseEvent *event)
{
    Q_D(QTreeView);
    if (d->itemDecorationAt(event->pos()) == -1) // ### what about expanding/collapsing state ?
        QAbstractItemView::mouseMoveEvent(event);
}

/*!
  \reimp
*/
void QTreeView::keyPressEvent(QKeyEvent *event)
{
    Q_D(QTreeView);
    QModelIndex current = currentIndex();
    //this is the management of the expansion
    if (d->isIndexValid(current) && d->model && d->itemsExpandable) {
        switch (event->key()) {
        case Qt::Key_Asterisk: {
            QStack<QModelIndex> parents;
            parents.push(current);
                while (!parents.isEmpty()) {
                    QModelIndex parent = parents.pop();
                    for (int row = 0; row < d->model->rowCount(parent); ++row) {
                        QModelIndex child = d->model->index(row, 0, parent);
                        if (!d->isIndexValid(child))
                            break;
                        parents.push(child);
                        expand(child);
                    }
                }
                expand(current);
            break; }
        case Qt::Key_Plus:
            expand(current);
            break;
        case Qt::Key_Minus:
            collapse(current);
            break;
        }
    }

    QAbstractItemView::keyPressEvent(event);
}

/*!
  \reimp
*/
QModelIndex QTreeView::indexAt(const QPoint &point) const
{
    Q_D(const QTreeView);
    d->executePostedLayout();

    int visualIndex = d->itemAtCoordinate(point.y());
    QModelIndex idx = d->modelIndex(visualIndex);
    if (!idx.isValid())
        return QModelIndex();

    if (d->viewItems.at(visualIndex).spanning)
        return idx;

    int column = d->columnAt(point.x());
    if (column == idx.column())
        return idx;
    if (column < 0)
        return QModelIndex();
    return idx.sibling(idx.row(), column);
}

/*!
  Returns the model index of the item above \a index.
*/
QModelIndex QTreeView::indexAbove(const QModelIndex &index) const
{
    Q_D(const QTreeView);
    if (!d->isIndexValid(index))
        return QModelIndex();
    d->executePostedLayout();
    int i = d->viewIndex(index);
    if (--i < 0)
        return QModelIndex();
    return d->viewItems.at(i).index;
}

/*!
  Returns the model index of the item below \a index.
*/
QModelIndex QTreeView::indexBelow(const QModelIndex &index) const
{
    Q_D(const QTreeView);
    if (!d->isIndexValid(index))
        return QModelIndex();
    d->executePostedLayout();
    int i = d->viewIndex(index);
    if (++i >= d->viewItems.count())
        return QModelIndex();
    return d->viewItems.at(i).index;
}

/*!
    \internal

    Lays out the items in the tree view.
*/
void QTreeView::doItemsLayout()
{
    Q_D(QTreeView);
    if (d->hasRemovedItems) {
        //clean the QSet that may contains old (and this invalid) indexes
        d->hasRemovedItems = false;
        QSet<QPersistentModelIndex>::iterator it = d->expandedIndexes.begin();
        while (it != d->expandedIndexes.constEnd()) {
            if (!it->isValid())
                it = d->expandedIndexes.erase(it);
            else
                ++it;
        }
        it = d->hiddenIndexes.begin();
        while (it != d->hiddenIndexes.constEnd()) {
            if (!it->isValid())
                it = d->hiddenIndexes.erase(it);
            else
                ++it;
        }
    }
    d->viewItems.clear(); // prepare for new layout
    QModelIndex parent = d->root;
    if (d->model->hasChildren(parent)) {
        d->layout(-1);
    }
    QAbstractItemView::doItemsLayout();
    d->header->doItemsLayout();
}

/*!
  \reimp
*/
void QTreeView::reset()
{
    Q_D(QTreeView);
    d->expandedIndexes.clear();
    d->hiddenIndexes.clear();
    d->spanningIndexes.clear();
    d->viewItems.clear();
    QAbstractItemView::reset();
}

/*!
  Returns the horizontal offset of the items in the treeview.

  Note that the tree view uses the horizontal header section
  positions to determine the positions of columns in the view.

  \sa verticalOffset()
*/
int QTreeView::horizontalOffset() const
{
    Q_D(const QTreeView);
    return d->header->offset();
}

/*!
  Returns the vertical offset of the items in the tree view.

  \sa horizontalOffset()
*/
int QTreeView::verticalOffset() const
{
    Q_D(const QTreeView);
    if (d->verticalScrollMode == QAbstractItemView::ScrollPerItem) {
        if (d->uniformRowHeights)
            return verticalScrollBar()->value() * d->defaultItemHeight;
        // If we are scrolling per item and have non-uniform row heights,
        // finding the vertical offset in pixels is going to be relatively slow.
        // ### find a faster way to do this
        d->executePostedLayout();
        int offset = 0;
        for (int i = 0; i < d->viewItems.count(); ++i) {
            if (i == verticalScrollBar()->value())
                return offset;
            offset += d->itemHeight(i);
        }
        return 0;
    }
    // scroll per pixel
    return verticalScrollBar()->value();
}

/*!
    Move the cursor in the way described by \a cursorAction, using the
    information provided by the button \a modifiers.
*/
QModelIndex QTreeView::moveCursor(CursorAction cursorAction, Qt::KeyboardModifiers modifiers)
{
    Q_D(QTreeView);
    Q_UNUSED(modifiers);

    d->executePostedLayout();

    QModelIndex current = currentIndex();
    if (!current.isValid()) {
        int i = d->below(-1);
        int c = 0;
        while (c < d->header->count() && d->header->isSectionHidden(c))
            ++c;
        if (i < d->viewItems.count() && c < d->header->count()) {
            return d->modelIndex(i, c);
        }
        return QModelIndex();
    }
    int vi = -1;
#if defined(Q_WS_MAC) && !defined(QT_NO_STYLE_MAC)
    // Selection behavior is slightly different on the Mac.
    if (d->selectionMode == QAbstractItemView::ExtendedSelection
        && d->selectionModel
        && d->selectionModel->hasSelection()) {

        const bool moveUpDown = (cursorAction == MoveUp || cursorAction == MoveDown);
        const bool moveNextPrev = (cursorAction == MoveNext || cursorAction == MovePrevious);
        const bool contiguousSelection = moveUpDown && (modifiers & Qt::ShiftModifier);

        // Use the outermost index in the selection as the current index
        if (!contiguousSelection && (moveUpDown || moveNextPrev)) {

            // Find outermost index.
            const bool useTopIndex = (cursorAction == MoveUp || cursorAction == MovePrevious);
            int index = useTopIndex ? INT_MAX : INT_MIN;
            const QItemSelection selection = d->selectionModel->selection();
            for (int i = 0; i < selection.count(); ++i) {
                const QItemSelectionRange &range = selection.at(i);
                int candidate = d->viewIndex(useTopIndex ? range.topLeft() : range.bottomRight());
                if (candidate >= 0)
                    index = useTopIndex ? qMin(index, candidate) : qMax(index, candidate);
            }

            if (index >= 0 && index < INT_MAX)
                vi = index;
        }
    }
#endif
    if (vi < 0)
        vi = qMax(0, d->viewIndex(current));

    if (isRightToLeft()) {
        if (cursorAction == MoveRight)
            cursorAction = MoveLeft;
        else if (cursorAction == MoveLeft)
            cursorAction = MoveRight;
    }
    switch (cursorAction) {
    case MoveNext:
    case MoveDown:
#ifdef QT_KEYPAD_NAVIGATION
        if (vi == d->viewItems.count()-1 && QApplication::keypadNavigationEnabled())
            return d->model->index(0, current.column(), d->root);
#endif
        return d->modelIndex(d->below(vi), current.column());
    case MovePrevious:
    case MoveUp:
#ifdef QT_KEYPAD_NAVIGATION
        if (vi == 0 && QApplication::keypadNavigationEnabled())
            return d->modelIndex(d->viewItems.count() - 1, current.column());
#endif
        return d->modelIndex(d->above(vi), current.column());
    case MoveLeft: {
        QScrollBar *sb = horizontalScrollBar();
        if (vi < d->viewItems.count() && d->viewItems.at(vi).expanded && d->itemsExpandable && sb->value() == sb->minimum()) {
            d->collapse(vi, true);
            d->moveCursorUpdatedView = true;
        } else {
            bool descend = style()->styleHint(QStyle::SH_ItemView_ArrowKeysNavigateIntoChildren, 0, this);
            if (descend) {
                QModelIndex par = current.parent();
                if (par.isValid() && par != rootIndex())
                    return par;
                else
                    descend = false;
            }
            if (!descend) {
                if (d->selectionBehavior == SelectItems || d->selectionBehavior == SelectColumns) {
                    int visualColumn = d->header->visualIndex(current.column()) - 1;
                    while (visualColumn >= 0 && isColumnHidden(d->header->logicalIndex(visualColumn)))
                        visualColumn--;
                    int newColumn = d->header->logicalIndex(visualColumn);
                    QModelIndex next = current.sibling(current.row(), newColumn);
                    if (next.isValid())
                        return next;
                }

                int oldValue = sb->value();
                sb->setValue(sb->value() - sb->singleStep());
                if (oldValue != sb->value())
                    d->moveCursorUpdatedView = true;
            }

        }
        updateGeometries();
        viewport()->update();
        break;
    }
    case MoveRight:
        if (vi < d->viewItems.count() && !d->viewItems.at(vi).expanded && d->itemsExpandable
            && d->hasVisibleChildren(d->viewItems.at(vi).index)) {
            d->expand(vi, true);
            d->moveCursorUpdatedView = true;
        } else {
            bool descend = style()->styleHint(QStyle::SH_ItemView_ArrowKeysNavigateIntoChildren, 0, this);
            if (descend) {
                QModelIndex idx = d->modelIndex(d->below(vi));
                if (idx.parent() == current)
                    return idx;
                else
                    descend = false;
            }
            if (!descend) {
                if (d->selectionBehavior == SelectItems || d->selectionBehavior == SelectColumns) {
                    int visualColumn = d->header->visualIndex(current.column()) + 1;
                    while (visualColumn < d->model->columnCount(current.parent()) && isColumnHidden(d->header->logicalIndex(visualColumn)))
                        visualColumn++;

                    QModelIndex next = current.sibling(current.row(), visualColumn);
                    if (next.isValid())
                        return next;
                }

                //last restort: we change the scrollbar value
                QScrollBar *sb = horizontalScrollBar();
                int oldValue = sb->value();
                sb->setValue(sb->value() + sb->singleStep());
                if (oldValue != sb->value())
                    d->moveCursorUpdatedView = true;
            }
        }
        updateGeometries();
        viewport()->update();
        break;
    case MovePageUp:
        return d->modelIndex(d->pageUp(vi), current.column());
    case MovePageDown:
        return d->modelIndex(d->pageDown(vi), current.column());
    case MoveHome:
        return d->model->index(0, current.column(), d->root);
    case MoveEnd:
        return d->modelIndex(d->viewItems.count() - 1, current.column());
    }
    return current;
}

/*!
  Applies the selection \a command to the items in or touched by the
  rectangle, \a rect.

  \sa selectionCommand()
*/
void QTreeView::setSelection(const QRect &rect, QItemSelectionModel::SelectionFlags command)
{
    Q_D(QTreeView);
    if (!selectionModel() || rect.isNull())
        return;

    d->executePostedLayout();
    QPoint tl(isRightToLeft() ? qMax(rect.left(), rect.right())
              : qMin(rect.left(), rect.right()), qMin(rect.top(), rect.bottom()));
    QPoint br(isRightToLeft() ? qMin(rect.left(), rect.right()) :
              qMax(rect.left(), rect.right()), qMax(rect.top(), rect.bottom()));
    QModelIndex topLeft = indexAt(tl);
    QModelIndex bottomRight = indexAt(br);
    if (!topLeft.isValid() && !bottomRight.isValid()) {
        if (command & QItemSelectionModel::Clear)
            selectionModel()->clear();
        return;
    }
    if (!topLeft.isValid() && !d->viewItems.isEmpty())
        topLeft = d->viewItems.first().index;
    if (!bottomRight.isValid() && !d->viewItems.isEmpty()) {
        const int column = d->header->logicalIndex(d->header->count() - 1);
        const QModelIndex index = d->viewItems.last().index;
        bottomRight = index.sibling(index.row(), column);
    }

    if (!d->isIndexEnabled(topLeft) || !d->isIndexEnabled(bottomRight))
        return;

    d->select(topLeft, bottomRight, command);
}

/*!
  Returns the rectangle from the viewport of the items in the given
  \a selection.

  Since 4.7, the returned region only contains rectangles intersecting
  (or included in) the viewport.
*/
QRegion QTreeView::visualRegionForSelection(const QItemSelection &selection) const
{
    Q_D(const QTreeView);
    if (selection.isEmpty())
        return QRegion();

    QRegion selectionRegion;
    const QRect &viewportRect = d->viewport->rect();
    for (int i = 0; i < selection.count(); ++i) {
        QItemSelectionRange range = selection.at(i);
        if (!range.isValid())
            continue;
        QModelIndex parent = range.parent();
        QModelIndex leftIndex = range.topLeft();
        int columnCount = d->model->columnCount(parent);
        while (leftIndex.isValid() && isIndexHidden(leftIndex)) {
            if (leftIndex.column() + 1 < columnCount)
                leftIndex = d->model->index(leftIndex.row(), leftIndex.column() + 1, parent);
            else
                leftIndex = QModelIndex();
        }
        if (!leftIndex.isValid())
            continue;
        const QRect leftRect = visualRect(leftIndex);
        int top = leftRect.top();
        QModelIndex rightIndex = range.bottomRight();
        while (rightIndex.isValid() && isIndexHidden(rightIndex)) {
            if (rightIndex.column() - 1 >= 0)
                rightIndex = d->model->index(rightIndex.row(), rightIndex.column() - 1, parent);
            else
                rightIndex = QModelIndex();
        }
        if (!rightIndex.isValid())
            continue;
        const QRect rightRect = visualRect(rightIndex);
        int bottom = rightRect.bottom();
        if (top > bottom)
            qSwap<int>(top, bottom);
        int height = bottom - top + 1;
        if (d->header->sectionsMoved()) {
            for (int c = range.left(); c <= range.right(); ++c) {
                const QRect rangeRect(columnViewportPosition(c), top, columnWidth(c), height);
                if (viewportRect.intersects(rangeRect))
                    selectionRegion += rangeRect;
            }
        } else {
            QRect combined = leftRect|rightRect;
            combined.setX(columnViewportPosition(isRightToLeft() ? range.right() : range.left()));
            if (viewportRect.intersects(combined))
                selectionRegion += combined;
        }
    }
    return selectionRegion;
}

/*!
  \reimp
*/
QModelIndexList QTreeView::selectedIndexes() const
{
    QModelIndexList viewSelected;
    QModelIndexList modelSelected;
    if (selectionModel())
        modelSelected = selectionModel()->selectedIndexes();
    for (int i = 0; i < modelSelected.count(); ++i) {
        // check that neither the parents nor the index is hidden before we add
        QModelIndex index = modelSelected.at(i);
        while (index.isValid() && !isIndexHidden(index))
            index = index.parent();
        if (index.isValid())
            continue;
        viewSelected.append(modelSelected.at(i));
    }
    return viewSelected;
}

/*!
  Scrolls the contents of the tree view by (\a dx, \a dy).
*/
void QTreeView::scrollContentsBy(int dx, int dy)
{
    Q_D(QTreeView);

    d->delayedAutoScroll.stop(); // auto scroll was canceled by the user scrolling

    dx = isRightToLeft() ? -dx : dx;
    if (dx) {
        if (horizontalScrollMode() == QAbstractItemView::ScrollPerItem) {
            int oldOffset = d->header->offset();
            if (horizontalScrollBar()->value() == horizontalScrollBar()->maximum())
                d->header->setOffsetToLastSection();
            else
                d->header->setOffsetToSectionPosition(horizontalScrollBar()->value());
            int newOffset = d->header->offset();
            dx = isRightToLeft() ? newOffset - oldOffset : oldOffset - newOffset;
        } else {
            d->header->setOffset(horizontalScrollBar()->value());
        }
    }

    const int itemHeight = d->defaultItemHeight <= 0 ? sizeHintForRow(0) : d->defaultItemHeight;
    if (d->viewItems.isEmpty() || itemHeight == 0)
        return;

    // guestimate the number of items in the viewport
    int viewCount = d->viewport->height() / itemHeight;
    int maxDeltaY = qMin(d->viewItems.count(), viewCount);
    // no need to do a lot of work if we are going to redraw the whole thing anyway
    if (qAbs(dy) > qAbs(maxDeltaY) && d->editorIndexHash.isEmpty()) {
        verticalScrollBar()->update();
        d->viewport->update();
        return;
    }

    if (dy && verticalScrollMode() == QAbstractItemView::ScrollPerItem) {
        int currentScrollbarValue = verticalScrollBar()->value();
        int previousScrollbarValue = currentScrollbarValue + dy; // -(-dy)
        int currentViewIndex = currentScrollbarValue; // the first visible item
        int previousViewIndex = previousScrollbarValue;
        const QVector<QTreeViewItem> viewItems = d->viewItems;
        dy = 0;
        if (previousViewIndex < currentViewIndex) { // scrolling down
            for (int i = previousViewIndex; i < currentViewIndex; ++i) {
                if (i < d->viewItems.count())
                    dy -= d->itemHeight(i);
            }
        } else if (previousViewIndex > currentViewIndex) { // scrolling up
            for (int i = previousViewIndex - 1; i >= currentViewIndex; --i) {
                if (i < d->viewItems.count())
                    dy += d->itemHeight(i);
            }
        }
    }

    d->scrollContentsBy(dx, dy);
}

/*!
  This slot is called whenever a column has been moved.
*/
void QTreeView::columnMoved()
{
    Q_D(QTreeView);
    updateEditorGeometries();
    d->viewport->update();
}

/*!
  \internal
*/
void QTreeView::reexpand()
{
    // do nothing
}

/*!
  Informs the view that the rows from the \a start row to the \a end row
  inclusive have been inserted into the \a parent model item.
*/
void QTreeView::rowsInserted(const QModelIndex &parent, int start, int end)
{
    Q_D(QTreeView);
    // if we are going to do a complete relayout anyway, there is no need to update
    if (d->delayedPendingLayout) {
        QAbstractItemView::rowsInserted(parent, start, end);
        return;
    }

    //don't add a hierarchy on a column != 0
    if (parent.column() != 0 && parent.isValid()) {
        QAbstractItemView::rowsInserted(parent, start, end);
        return;
    }

    const int parentRowCount = d->model->rowCount(parent);
    const int delta = end - start + 1;
    if (parent != d->root && !d->isIndexExpanded(parent) && parentRowCount > delta) {
        QAbstractItemView::rowsInserted(parent, start, end);
        return;
    }

    const int parentItem = d->viewIndex(parent);
    if (((parentItem != -1) && d->viewItems.at(parentItem).expanded)
        || (parent == d->root)) {
        d->doDelayedItemsLayout();
    } else if (parentItem != -1 && (d->model->rowCount(parent) == end - start + 1)) {
        // the parent just went from 0 children to more. update to re-paint the decoration
        d->viewItems[parentItem].hasChildren = true;
        viewport()->update();
    }
    QAbstractItemView::rowsInserted(parent, start, end);
}

/*!
  Informs the view that the rows from the \a start row to the \a end row
  inclusive are about to removed from the given \a parent model item.
*/
void QTreeView::rowsAboutToBeRemoved(const QModelIndex &parent, int start, int end)
{
    Q_D(QTreeView);
    QAbstractItemView::rowsAboutToBeRemoved(parent, start, end);
    d->viewItems.clear();
}

/*!
    \since 4.1

    Informs the view that the rows from the \a start row to the \a end row
    inclusive have been removed from the given \a parent model item.
*/
void QTreeView::rowsRemoved(const QModelIndex &parent, int start, int end)
{
    Q_D(QTreeView);
    d->viewItems.clear();
    d->doDelayedItemsLayout();
    d->hasRemovedItems = true;
    d->_q_rowsRemoved(parent, start, end);
}

/*!
  Informs the tree view that the number of columns in the tree view has
  changed from \a oldCount to \a newCount.
*/
void QTreeView::columnCountChanged(int oldCount, int newCount)
{
    Q_D(QTreeView);
    if (oldCount == 0 && newCount > 0) {
        //if the first column has just been added we need to relayout.
        d->doDelayedItemsLayout();
    }

    if (isVisible())
        updateGeometries();
	viewport()->update();
}

/*!
  Resizes the \a column given to the size of its contents.

  \sa columnWidth(), setColumnWidth()
*/
void QTreeView::resizeColumnToContents(int column)
{
    Q_D(QTreeView);
    d->executePostedLayout();
    if (column < 0 || column >= d->header->count())
        return;
    int contents = sizeHintForColumn(column);
    int header = d->header->isHidden() ? 0 : d->header->sectionSizeHint(column);
    d->header->resizeSection(column, qMax(contents, header));
}

/*!
  \obsolete
  \overload

  Sorts the model by the values in the given \a column.
*/
void QTreeView::sortByColumn(int column)
{
    Q_D(QTreeView);
    sortByColumn(column, d->header->sortIndicatorOrder());
}

/*!
  \since 4.2

  Sets the model up for sorting by the values in the given \a column and \a order.

  \a column may be -1, in which case no sort indicator will be shown
  and the model will return to its natural, unsorted order. Note that not
  all models support this and may even crash in this case.

  \sa sortingEnabled
*/
void QTreeView::sortByColumn(int column, Qt::SortOrder order)
{
    Q_D(QTreeView);

    //If sorting is enabled  will emit a signal connected to _q_sortIndicatorChanged, which then actually sorts
    d->header->setSortIndicator(column, order);
    //If sorting is not enabled, force to sort now.
    if (!d->sortingEnabled)
        d->model->sort(column, order);
}

/*!
  \reimp
*/
void QTreeView::selectAll()
{
    Q_D(QTreeView);
    if (!selectionModel())
        return;
    SelectionMode mode = d->selectionMode;
    d->executePostedLayout(); //make sure we lay out the items
    if (mode != SingleSelection && !d->viewItems.isEmpty()) {
        const QModelIndex &idx = d->viewItems.last().index;
        QModelIndex lastItemIndex = idx.sibling(idx.row(), d->model->columnCount(idx.parent()) - 1);
        d->select(d->viewItems.first().index, lastItemIndex,
                  QItemSelectionModel::ClearAndSelect
                  |QItemSelectionModel::Rows);
    }
}

/*!
  \since 4.2
  Expands all expandable items.

  Warning: if the model contains a large number of items,
  this function will take some time to execute.

  \sa collapseAll() expand()  collapse() setExpanded()
*/
void QTreeView::expandAll()
{
    Q_D(QTreeView);
    d->viewItems.clear();
    d->interruptDelayedItemsLayout();
    d->layout(-1, true);
    updateGeometries();
    d->viewport->update();
}

/*!
  \since 4.2

  Collapses all expanded items.

  \sa expandAll() expand()  collapse() setExpanded()
*/
void QTreeView::collapseAll()
{
    Q_D(QTreeView);
    d->expandedIndexes.clear();
    doItemsLayout();
}

/*!
  \since 4.3
  Expands all expandable items to the given \a depth.

  \sa expandAll() collapseAll() expand()  collapse() setExpanded()
*/
void QTreeView::expandToDepth(int depth)
{
    Q_D(QTreeView);
    d->viewItems.clear();
    d->expandedIndexes.clear();
    d->interruptDelayedItemsLayout();
    d->layout(-1);
    for (int i = 0; i < d->viewItems.count(); ++i) {
        if (d->viewItems.at(i).level <= (uint)depth) {
            d->viewItems[i].expanded = true;
            d->layout(i);
            d->storeExpanded(d->viewItems.at(i).index);
        }
    }
    updateGeometries();
    d->viewport->update();
}

/*!
    This function is called whenever \a{column}'s size is changed in
    the header. \a oldSize and \a newSize give the previous size and
    the new size in pixels.

    \sa setColumnWidth()
*/
void QTreeView::columnResized(int column, int /* oldSize */, int /* newSize */)
{
    Q_D(QTreeView);
    d->columnsToUpdate.append(column);
    if (d->columnResizeTimerID == 0)
        d->columnResizeTimerID = startTimer(0);
}

/*!
  \reimp
*/
void QTreeView::updateGeometries()
{
    Q_D(QTreeView);
    if (d->header) {
        if (d->geometryRecursionBlock)
            return;
        d->geometryRecursionBlock = true;
        QSize hint = d->header->isHidden() ? QSize(0, 0) : d->header->sizeHint();
        setViewportMargins(0, hint.height(), 0, 0);
        QRect vg = d->viewport->geometry();
        QRect geometryRect(vg.left(), vg.top() - hint.height(), vg.width(), hint.height());
        d->header->setGeometry(geometryRect);
        //d->header->setOffset(horizontalScrollBar()->value()); // ### bug ???
        QMetaObject::invokeMethod(d->header, "updateGeometries");
        d->updateScrollBars();
        d->geometryRecursionBlock = false;
    }
    QAbstractItemView::updateGeometries();
}

/*!
  Returns the size hint for the \a column's width or -1 if there is no
  model.

  If you need to set the width of a given column to a fixed value, call
  QHeaderView::resizeSection() on the view's header.

  If you reimplement this function in a subclass, note that the value you
  return is only used when resizeColumnToContents() is called. In that case,
  if a larger column width is required by either the view's header or
  the item delegate, that width will be used instead.

  \sa QWidget::sizeHint, header()
*/
int QTreeView::sizeHintForColumn(int column) const
{
    Q_D(const QTreeView);
    d->executePostedLayout();
    if (d->viewItems.isEmpty())
        return -1;
    ensurePolished();
    int w = 0;
    QStyleOptionViewItemV4 option = d->viewOptionsV4();
    const QVector<QTreeViewItem> viewItems = d->viewItems;

    int start = 0;
    int end = viewItems.count();
    if(end > 1000) { //if we have too many item this function would be too slow.
        //we get a good approximation by only iterate over 1000 items.
        start = qMax(0, d->firstVisibleItem() - 100);
        end = qMin(end, start + 900);
    }

    for (int i = start; i < end; ++i) {
        if (viewItems.at(i).spanning)
            continue; // we have no good size hint
        QModelIndex index = viewItems.at(i).index;
        index = index.sibling(index.row(), column);
        QWidget *editor = d->editorForIndex(index).widget.data();
        if (editor && d->persistent.contains(editor)) {
            w = qMax(w, editor->sizeHint().width());
            int min = editor->minimumSize().width();
            int max = editor->maximumSize().width();
            w = qBound(min, w, max);
        }
        int hint = d->delegateForIndex(index)->sizeHint(option, index).width();
        w = qMax(w, hint + (column == 0 ? d->indentationForItem(i) : 0));
    }
    return w;
}

/*!
  Returns the size hint for the row indicated by \a index.

  \sa sizeHintForColumn(), uniformRowHeights()
*/
int QTreeView::indexRowSizeHint(const QModelIndex &index) const
{
    Q_D(const QTreeView);
    if (!d->isIndexValid(index) || !d->itemDelegate)
        return 0;

    int start = -1;
    int end = -1;
    int indexRow = index.row();
    int count = d->header->count();
    bool emptyHeader = (count == 0);
    QModelIndex parent = index.parent();

    if (count && isVisible()) {
        // If the sections have moved, we end up checking too many or too few
        start = d->header->visualIndexAt(0);
    } else {
        // If the header has not been laid out yet, we use the model directly
        count = d->model->columnCount(parent);
    }

    if (isRightToLeft()) {
        start = (start == -1 ? count - 1 : start);
        end = 0;
    } else {
        start = (start == -1 ? 0 : start);
        end = count - 1;
    }

    if (end < start)
        qSwap(end, start);

    int height = -1;
    QStyleOptionViewItemV4 option = d->viewOptionsV4();
    // ### If we want word wrapping in the items,
    // ### we need to go through all the columns
    // ### and set the width of the column

    // Hack to speed up the function
    option.rect.setWidth(-1);

    for (int column = start; column <= end; ++column) {
        int logicalColumn = emptyHeader ? column : d->header->logicalIndex(column);
        if (d->header->isSectionHidden(logicalColumn))
            continue;
        QModelIndex idx = d->model->index(indexRow, logicalColumn, parent);
        if (idx.isValid()) {
            QWidget *editor = d->editorForIndex(idx).widget.data();
            if (editor && d->persistent.contains(editor)) {
                height = qMax(height, editor->sizeHint().height());
                int min = editor->minimumSize().height();
                int max = editor->maximumSize().height();
                height = qBound(min, height, max);
            }
            int hint = d->delegateForIndex(idx)->sizeHint(option, idx).height();
            height = qMax(height, hint);
        }
    }

    return height;
}

/*!
    \since 4.3
    Returns the height of the row indicated by the given \a index.
    \sa indexRowSizeHint()
*/
int QTreeView::rowHeight(const QModelIndex &index) const
{
    Q_D(const QTreeView);
    d->executePostedLayout();
    int i = d->viewIndex(index);
    if (i == -1)
        return 0;
    return d->itemHeight(i);
}

/*!
  \internal
*/
void QTreeView::horizontalScrollbarAction(int action)
{
    QAbstractItemView::horizontalScrollbarAction(action);
}

/*!
  \reimp
*/
bool QTreeView::isIndexHidden(const QModelIndex &index) const
{
    return (isColumnHidden(index.column()) || isRowHidden(index.row(), index.parent()));
}

/*
  private implementation
*/
void QTreeViewPrivate::initialize()
{
    Q_Q(QTreeView);
    updateStyledFrameWidths();
    q->setSelectionBehavior(QAbstractItemView::SelectRows);
    q->setSelectionMode(QAbstractItemView::SingleSelection);
    q->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
    q->setAttribute(Qt::WA_MacShowFocusRect);

    QHeaderView *header = new QHeaderView(Qt::Horizontal, q);
    header->setMovable(true);
    header->setStretchLastSection(true);
    header->setDefaultAlignment(Qt::AlignLeft|Qt::AlignVCenter);
    q->setHeader(header);
#ifndef QT_NO_ANIMATION
    QObject::connect(&animatedOperation, SIGNAL(finished()), q, SLOT(_q_endAnimatedOperation()));
#endif //QT_NO_ANIMATION
}

void QTreeViewPrivate::expand(int item, bool emitSignal)
{
    Q_Q(QTreeView);

    if (item == -1 || viewItems.at(item).expanded)
        return;

#ifndef QT_NO_ANIMATION
    if (emitSignal && animationsEnabled)
        prepareAnimatedOperation(item, QVariantAnimation::Forward);
#endif //QT_NO_ANIMATION
     //if already animating, stateBeforeAnimation is set to the correct value
    if (state != QAbstractItemView::AnimatingState)
        stateBeforeAnimation = state;
    q->setState(QAbstractItemView::ExpandingState);
    const QModelIndex index = viewItems.at(item).index;
    storeExpanded(index);
    viewItems[item].expanded = true;
    layout(item);
    q->setState(stateBeforeAnimation);

    if (model->canFetchMore(index))
        model->fetchMore(index);
    if (emitSignal) {
        emit q->expanded(index);
#ifndef QT_NO_ANIMATION
        if (animationsEnabled)
            beginAnimatedOperation();
#endif //QT_NO_ANIMATION
    }
}

void QTreeViewPrivate::insertViewItems(int pos, int count, const QTreeViewItem &viewItem)
{
    viewItems.insert(pos, count, viewItem);
    QTreeViewItem *items = viewItems.data();
    for (int i = pos + count; i < viewItems.count(); i++)
        if (items[i].parentItem >= pos)
            items[i].parentItem += count;
#ifndef QT_NO_ACCESSIBILITY
#ifdef Q_WS_X11
    Q_Q(QTreeView);
    if (QAccessible::isActive()) {
        QAccessible::updateAccessibility(q, 0, QAccessible::TableModelChanged);
    }
#endif
#endif
}

void QTreeViewPrivate::removeViewItems(int pos, int count)
{
    viewItems.remove(pos, count);
    QTreeViewItem *items = viewItems.data();
    for (int i = pos; i < viewItems.count(); i++)
        if (items[i].parentItem >= pos)
            items[i].parentItem -= count;
#ifndef QT_NO_ACCESSIBILITY
#ifdef Q_WS_X11
    Q_Q(QTreeView);
    if (QAccessible::isActive()) {
        QAccessible::updateAccessibility(q, 0, QAccessible::TableModelChanged);
    }
#endif
#endif
}

#if 0
bool QTreeViewPrivate::checkViewItems() const
{
    for (int i = 0; i < viewItems.count(); ++i) {
        const QTreeViewItem &vi = viewItems.at(i);
        if (vi.parentItem == -1) {
            Q_ASSERT(!vi.index.parent().isValid() || vi.index.parent() == root);
        } else {
            Q_ASSERT(vi.index.parent() == viewItems.at(vi.parentItem).index);
        }
    }
    return true;
}
#endif

void QTreeViewPrivate::collapse(int item, bool emitSignal)
{
    Q_Q(QTreeView);

    if (item == -1 || expandedIndexes.isEmpty())
        return;

    //if the current item is now invisible, the autoscroll will expand the tree to see it, so disable the autoscroll
    delayedAutoScroll.stop();

    int total = viewItems.at(item).total;
    const QModelIndex &modelIndex = viewItems.at(item).index;
    if (!isPersistent(modelIndex))
        return; // if the index is not persistent, no chances it is expanded
    QSet<QPersistentModelIndex>::iterator it = expandedIndexes.find(modelIndex);
    if (it == expandedIndexes.end() || viewItems.at(item).expanded == false)
        return; // nothing to do

#ifndef QT_NO_ANIMATION
    if (emitSignal && animationsEnabled)
        prepareAnimatedOperation(item, QVariantAnimation::Backward);
#endif //QT_NO_ANIMATION

    //if already animating, stateBeforeAnimation is set to the correct value
    if (state != QAbstractItemView::AnimatingState)
        stateBeforeAnimation = state;
    q->setState(QAbstractItemView::CollapsingState);
    expandedIndexes.erase(it);
    viewItems[item].expanded = false;
    int index = item;
    while (index > -1) {
        viewItems[index].total -= total;
        index = viewItems[index].parentItem;
    }
    removeViewItems(item + 1, total); // collapse
    q->setState(stateBeforeAnimation);

    if (emitSignal) {
        emit q->collapsed(modelIndex);
#ifndef QT_NO_ANIMATION
        if (animationsEnabled)
            beginAnimatedOperation();
#endif //QT_NO_ANIMATION
    }
}

#ifndef QT_NO_ANIMATION
void QTreeViewPrivate::prepareAnimatedOperation(int item, QVariantAnimation::Direction direction)
{
    animatedOperation.item = item;
    animatedOperation.viewport = viewport;
    animatedOperation.setDirection(direction);

    int top = coordinateForItem(item) + itemHeight(item);
    QRect rect = viewport->rect();
    rect.setTop(top);
    if (direction == QVariantAnimation::Backward) {
        const int limit = rect.height() * 2;
        int h = 0;
        int c = item + viewItems.at(item).total + 1;
        for (int i = item + 1; i < c && h < limit; ++i)
            h += itemHeight(i);
        rect.setHeight(h);
        animatedOperation.setEndValue(top + h);
    }
    animatedOperation.setStartValue(top);
    animatedOperation.before = renderTreeToPixmapForAnimation(rect);
}

void QTreeViewPrivate::beginAnimatedOperation()
{
    Q_Q(QTreeView);

    QRect rect = viewport->rect();
    rect.setTop(animatedOperation.top());
    if (animatedOperation.direction() == QVariantAnimation::Forward) {
        const int limit = rect.height() * 2;
        int h = 0;
        int c = animatedOperation.item + viewItems.at(animatedOperation.item).total + 1;
        for (int i = animatedOperation.item + 1; i < c && h < limit; ++i)
            h += itemHeight(i);
        rect.setHeight(h);
        animatedOperation.setEndValue(animatedOperation.top() + h);
    }

    if (!rect.isEmpty()) {
        animatedOperation.after = renderTreeToPixmapForAnimation(rect);

        q->setState(QAbstractItemView::AnimatingState);
        animatedOperation.start(); //let's start the animation
    }
}

void QTreeViewPrivate::drawAnimatedOperation(QPainter *painter) const
{
    const int start = animatedOperation.startValue().toInt(),
        end = animatedOperation.endValue().toInt(),
        current = animatedOperation.currentValue().toInt();
    bool collapsing = animatedOperation.direction() == QVariantAnimation::Backward;
    const QPixmap top = collapsing ? animatedOperation.before : animatedOperation.after;
    painter->drawPixmap(0, start, top, 0, end - current - 1, top.width(), top.height());
    const QPixmap bottom = collapsing ? animatedOperation.after : animatedOperation.before;
    painter->drawPixmap(0, current, bottom);
}

QPixmap QTreeViewPrivate::renderTreeToPixmapForAnimation(const QRect &rect) const
{
    Q_Q(const QTreeView);
    QPixmap pixmap(rect.size());
    if (rect.size().isEmpty())
        return pixmap;
    pixmap.fill(Qt::transparent); //the base might not be opaque, and we don't want uninitialized pixels.
    QPainter painter(&pixmap);
    painter.fillRect(QRect(QPoint(0,0), rect.size()), q->palette().base());
    painter.translate(0, -rect.top());
    q->drawTree(&painter, QRegion(rect));
    painter.end();

    //and now let's render the editors the editors
    QStyleOptionViewItemV4 option = viewOptionsV4();
    for (QEditorIndexHash::const_iterator it = editorIndexHash.constBegin(); it != editorIndexHash.constEnd(); ++it) {
        QWidget *editor = it.key();
        const QModelIndex &index = it.value();
        option.rect = q->visualRect(index);
        if (option.rect.isValid()) {

            if (QAbstractItemDelegate *delegate = delegateForIndex(index))
                delegate->updateEditorGeometry(editor, option, index);

            const QPoint pos = editor->pos();
            if (rect.contains(pos)) {
                editor->render(&pixmap, pos - rect.topLeft());
                //the animation uses pixmap to display the treeview's content
                //the editor is rendered on this pixmap and thus can (should) be hidden
                editor->hide();
            }
        }
    }


    return pixmap;
}

void QTreeViewPrivate::_q_endAnimatedOperation()
{
    Q_Q(QTreeView);
    q->setState(stateBeforeAnimation);
    q->updateGeometries();
    viewport->update();
}
#endif //QT_NO_ANIMATION

void QTreeViewPrivate::_q_modelAboutToBeReset()
{
    viewItems.clear();
}

void QTreeViewPrivate::_q_columnsAboutToBeRemoved(const QModelIndex &parent, int start, int end)
{
    if (start <= 0 && 0 <= end)
        viewItems.clear();
    QAbstractItemViewPrivate::_q_columnsAboutToBeRemoved(parent, start, end);
}

void QTreeViewPrivate::_q_columnsRemoved(const QModelIndex &parent, int start, int end)
{
    if (start <= 0 && 0 <= end)
        doDelayedItemsLayout();
    QAbstractItemViewPrivate::_q_columnsRemoved(parent, start, end);
}

/** \internal
    creates and initialize the viewItem structure of the children of the element \i

    set \a recursiveExpanding if the function has to expand all the children (called from expandAll)
    \a afterIsUninitialized is when we recurse from layout(-1), it means all the items after 'i' are
    not yet initialized and need not to be moved
 */
void QTreeViewPrivate::layout(int i, bool recursiveExpanding, bool afterIsUninitialized)
{
    Q_Q(QTreeView);
    QModelIndex current;
    QModelIndex parent = (i < 0) ? (QModelIndex)root : modelIndex(i);

    if (i>=0 && !parent.isValid()) {
        //modelIndex() should never return something invalid for the real items.
        //This can happen if columncount has been set to 0.
        //To avoid infinite loop we stop here.
        return;
    }

    int count = 0;
    if (model->hasChildren(parent)) {
        if (model->canFetchMore(parent))
            model->fetchMore(parent);
        count = model->rowCount(parent);
    }

    bool expanding = true;
    if (i == -1) {
        if (uniformRowHeights) {
            QModelIndex index = model->index(0, 0, parent);
            defaultItemHeight = q->indexRowSizeHint(index);
        }
        viewItems.resize(count);
        afterIsUninitialized = true;
    } else if (viewItems[i].total != (uint)count) {
        if (!afterIsUninitialized)
            insertViewItems(i + 1, count, QTreeViewItem()); // expand
        else if (count > 0)
            viewItems.resize(viewItems.count() + count);
    } else {
        expanding = false;
    }

    int first = i + 1;
    int level = (i >= 0 ? viewItems.at(i).level + 1 : 0);
    int hidden = 0;
    int last = 0;
    int children = 0;
    QTreeViewItem *item = 0;
    for (int j = first; j < first + count; ++j) {
        current = model->index(j - first, 0, parent);
        if (isRowHidden(current)) {
            ++hidden;
            last = j - hidden + children;
        } else {
            last = j - hidden + children;
            if (item)
                item->hasMoreSiblings = true;
            item = &viewItems[last];
            item->index = current;
            item->parentItem = i;
            item->level = level;
            item->height = 0;
            item->spanning = q->isFirstColumnSpanned(current.row(), parent);
            item->expanded = false;
            item->total = 0;
            item->hasMoreSiblings = false;
            if (recursiveExpanding || isIndexExpanded(current)) {
                if (recursiveExpanding)
                    expandedIndexes.insert(current);
                item->expanded = true;
                layout(last, recursiveExpanding, afterIsUninitialized);
                item = &viewItems[last];
                children += item->total;
                item->hasChildren = item->total > 0;
                last = j - hidden + children;
            } else {
                item->hasChildren = hasVisibleChildren(current);
            }
        }
    }

    // remove hidden items
    if (hidden > 0) {
        if (!afterIsUninitialized)
            removeViewItems(last + 1, hidden);
        else
            viewItems.resize(viewItems.size() - hidden);
    }

    if (!expanding)
        return; // nothing changed

    while (i > -1) {
        viewItems[i].total += count - hidden;
        i = viewItems[i].parentItem;
    }
}

int QTreeViewPrivate::pageUp(int i) const
{
    int index = itemAtCoordinate(coordinateForItem(i) - viewport->height());
    while (isItemHiddenOrDisabled(index))
        index--;
    return index == -1 ? 0 : index;
}

int QTreeViewPrivate::pageDown(int i) const
{
    int index = itemAtCoordinate(coordinateForItem(i) + viewport->height());
    while (isItemHiddenOrDisabled(index))
        index++;
    return index == -1 ? viewItems.count() - 1 : index;
}

int QTreeViewPrivate::indentationForItem(int item) const
{
    if (item < 0 || item >= viewItems.count())
        return 0;
    int level = viewItems.at(item).level;
    if (rootDecoration)
        ++level;
    return level * indent;
}

int QTreeViewPrivate::itemHeight(int item) const
{
    if (uniformRowHeights)
        return defaultItemHeight;
    if (viewItems.isEmpty())
        return 0;
    const QModelIndex &index = viewItems.at(item).index;
    if (!index.isValid())
        return 0;
    int height = viewItems.at(item).height;
    if (height <= 0) {
        height = q_func()->indexRowSizeHint(index);
        viewItems[item].height = height;
    }
    return qMax(height, 0);
}


/*!
  \internal
  Returns the viewport y coordinate for \a item.
*/
int QTreeViewPrivate::coordinateForItem(int item) const
{
    if (verticalScrollMode == QAbstractItemView::ScrollPerPixel) {
        if (uniformRowHeights)
            return (item * defaultItemHeight) - vbar->value();
        // ### optimize (spans or caching)
        int y = 0;
        for (int i = 0; i < viewItems.count(); ++i) {
            if (i == item)
                return y - vbar->value();
            y += itemHeight(i);
        }
    } else { // ScrollPerItem
        int topViewItemIndex = vbar->value();
        if (uniformRowHeights)
            return defaultItemHeight * (item - topViewItemIndex);
        if (item >= topViewItemIndex) {
            // search in the visible area first and continue down
            // ### slow if the item is not visible
            int viewItemCoordinate = 0;
            int viewItemIndex = topViewItemIndex;
            while (viewItemIndex < viewItems.count()) {
                if (viewItemIndex == item)
                    return viewItemCoordinate;
                viewItemCoordinate += itemHeight(viewItemIndex);
                ++viewItemIndex;
            }
            // below the last item in the view
            Q_ASSERT(false);
            return viewItemCoordinate;
        } else {
            // search the area above the viewport (used for editor widgets)
            int viewItemCoordinate = 0;
            for (int viewItemIndex = topViewItemIndex; viewItemIndex > 0; --viewItemIndex) {
                if (viewItemIndex == item)
                    return viewItemCoordinate;
                viewItemCoordinate -= itemHeight(viewItemIndex - 1);
            }
            return viewItemCoordinate;
        }
    }
    return 0;
}

/*!
  \internal
  Returns the index of the view item at the
  given viewport \a coordinate.

  \sa modelIndex()
*/
int QTreeViewPrivate::itemAtCoordinate(int coordinate) const
{
    const int itemCount = viewItems.count();
    if (itemCount == 0)
        return -1;
    if (uniformRowHeights && defaultItemHeight <= 0)
        return -1;
    if (verticalScrollMode == QAbstractItemView::ScrollPerPixel) {
        if (uniformRowHeights) {
            const int viewItemIndex = (coordinate + vbar->value()) / defaultItemHeight;
            return ((viewItemIndex >= itemCount || viewItemIndex < 0) ? -1 : viewItemIndex);
        }
        // ### optimize
        int viewItemCoordinate = 0;
        const int contentsCoordinate = coordinate + vbar->value();
        for (int viewItemIndex = 0; viewItemIndex < viewItems.count(); ++viewItemIndex) {
            viewItemCoordinate += itemHeight(viewItemIndex);
            if (viewItemCoordinate >= contentsCoordinate)
                return (viewItemIndex >= itemCount ? -1 : viewItemIndex);
        }
    } else { // ScrollPerItem
        int topViewItemIndex = vbar->value();
        if (uniformRowHeights) {
            if (coordinate < 0)
                coordinate -= defaultItemHeight - 1;
            const int viewItemIndex = topViewItemIndex + (coordinate / defaultItemHeight);
            return ((viewItemIndex >= itemCount || viewItemIndex < 0) ? -1 : viewItemIndex);
        }
        if (coordinate >= 0) {
            // the coordinate is in or below the viewport
            int viewItemCoordinate = 0;
            for (int viewItemIndex = topViewItemIndex; viewItemIndex < viewItems.count(); ++viewItemIndex) {
                viewItemCoordinate += itemHeight(viewItemIndex);
                if (viewItemCoordinate > coordinate)
                    return (viewItemIndex >= itemCount ? -1 : viewItemIndex);
            }
        } else {
            // the coordinate is above the viewport
            int viewItemCoordinate = 0;
            for (int viewItemIndex = topViewItemIndex; viewItemIndex >= 0; --viewItemIndex) {
                if (viewItemCoordinate <= coordinate)
                    return (viewItemIndex >= itemCount ? -1 : viewItemIndex);
                viewItemCoordinate -= itemHeight(viewItemIndex);
            }
        }
    }
    return -1;
}

int QTreeViewPrivate::viewIndex(const QModelIndex &_index) const
{
    if (!_index.isValid() || viewItems.isEmpty())
        return -1;

    const int totalCount = viewItems.count();
    const QModelIndex index = _index.sibling(_index.row(), 0);
    const int row = index.row();
    const qint64 internalId = index.internalId();

    // We start nearest to the lastViewedItem
    int localCount = qMin(lastViewedItem - 1, totalCount - lastViewedItem);
    for (int i = 0; i < localCount; ++i) {
        const QModelIndex &idx1 = viewItems.at(lastViewedItem + i).index;
        if (idx1.row() == row && idx1.internalId() == internalId) {
            lastViewedItem = lastViewedItem + i;
            return lastViewedItem;
        }
        const QModelIndex &idx2 = viewItems.at(lastViewedItem - i - 1).index;
        if (idx2.row() == row && idx2.internalId() == internalId) {
            lastViewedItem = lastViewedItem - i - 1;
            return lastViewedItem;
        }
    }

    for (int j = qMax(0, lastViewedItem + localCount); j < totalCount; ++j) {
        const QModelIndex &idx = viewItems.at(j).index;
        if (idx.row() == row && idx.internalId() == internalId) {
            lastViewedItem = j;
            return j;
        }
    }
    for (int j = qMin(totalCount, lastViewedItem - localCount) - 1; j >= 0; --j) {
        const QModelIndex &idx = viewItems.at(j).index;
        if (idx.row() == row && idx.internalId() == internalId) {
            lastViewedItem = j;
            return j;
        }
    }

    // nothing found
    return -1;
}

QModelIndex QTreeViewPrivate::modelIndex(int i, int column) const
{
    if (i < 0 || i >= viewItems.count())
        return QModelIndex();

    QModelIndex ret = viewItems.at(i).index;
    if (column)
        ret = ret.sibling(ret.row(), column);
    return ret;
}

int QTreeViewPrivate::firstVisibleItem(int *offset) const
{
    const int value = vbar->value();
    if (verticalScrollMode == QAbstractItemView::ScrollPerItem) {
        if (offset)
            *offset = 0;
        return (value < 0 || value >= viewItems.count()) ? -1 : value;
    }
    // ScrollMode == ScrollPerPixel
    if (uniformRowHeights) {
        if (!defaultItemHeight)
            return -1;

        if (offset)
            *offset = -(value % defaultItemHeight);
        return value / defaultItemHeight;
    }
    int y = 0; // ### optimize (use spans ?)
    for (int i = 0; i < viewItems.count(); ++i) {
        y += itemHeight(i); // the height value is cached
        if (y > value) {
            if (offset)
                *offset = y - value - itemHeight(i);
            return i;
        }
    }
    return -1;
}

int QTreeViewPrivate::columnAt(int x) const
{
    return header->logicalIndexAt(x);
}

void QTreeViewPrivate::updateScrollBars()
{
    Q_Q(QTreeView);
    QSize viewportSize = viewport->size();
    if (!viewportSize.isValid())
        viewportSize = QSize(0, 0);

    if (viewItems.isEmpty()) {
        q->doItemsLayout();
    }

    int itemsInViewport = 0;
    if (uniformRowHeights) {
        if (defaultItemHeight <= 0)
            itemsInViewport = viewItems.count();
        else
            itemsInViewport = viewportSize.height() / defaultItemHeight;
    } else {
        const int itemsCount = viewItems.count();
        const int viewportHeight = viewportSize.height();
        for (int height = 0, item = itemsCount - 1; item >= 0; --item) {
            height += itemHeight(item);
            if (height > viewportHeight)
                break;
            ++itemsInViewport;
        }
    }
    if (verticalScrollMode == QAbstractItemView::ScrollPerItem) {
        if (!viewItems.isEmpty())
            itemsInViewport = qMax(1, itemsInViewport);
        vbar->setRange(0, viewItems.count() - itemsInViewport);
        vbar->setPageStep(itemsInViewport);
        vbar->setSingleStep(1);
    } else { // scroll per pixel
        int contentsHeight = 0;
        if (uniformRowHeights) {
            contentsHeight = defaultItemHeight * viewItems.count();
        } else { // ### optimize (spans or caching)
            for (int i = 0; i < viewItems.count(); ++i)
                contentsHeight += itemHeight(i);
        }
        vbar->setRange(0, contentsHeight - viewportSize.height());
        vbar->setPageStep(viewportSize.height());
        vbar->setSingleStep(qMax(viewportSize.height() / (itemsInViewport + 1), 2));
    }

    const int columnCount = header->count();
    const int viewportWidth = viewportSize.width();
    int columnsInViewport = 0;
    for (int width = 0, column = columnCount - 1; column >= 0; --column) {
        int logical = header->logicalIndex(column);
        width += header->sectionSize(logical);
        if (width > viewportWidth)
            break;
        ++columnsInViewport;
    }
    if (columnCount > 0)
        columnsInViewport = qMax(1, columnsInViewport);
    if (horizontalScrollMode == QAbstractItemView::ScrollPerItem) {
        hbar->setRange(0, columnCount - columnsInViewport);
        hbar->setPageStep(columnsInViewport);
        hbar->setSingleStep(1);
    } else { // scroll per pixel
        const int horizontalLength = header->length();
        const QSize maxSize = q->maximumViewportSize();
        if (maxSize.width() >= horizontalLength && vbar->maximum() <= 0)
            viewportSize = maxSize;
        hbar->setPageStep(viewportSize.width());
        hbar->setRange(0, qMax(horizontalLength - viewportSize.width(), 0));
        hbar->setSingleStep(qMax(viewportSize.width() / (columnsInViewport + 1), 2));
    }
}

int QTreeViewPrivate::itemDecorationAt(const QPoint &pos) const
{
    executePostedLayout();
    int x = pos.x();
    int column = header->logicalIndexAt(x);
    if (column != 0)
        return -1; // no logical index at x

    int viewItemIndex = itemAtCoordinate(pos.y());
    QRect returning = itemDecorationRect(modelIndex(viewItemIndex));
    if (!returning.contains(pos))
        return -1;

    return viewItemIndex;
}

QRect QTreeViewPrivate::itemDecorationRect(const QModelIndex &index) const
{
    Q_Q(const QTreeView);
    if (!rootDecoration && index.parent() == root)
        return QRect(); // no decoration at root

    int viewItemIndex = viewIndex(index);
    if (viewItemIndex < 0 || !hasVisibleChildren(viewItems.at(viewItemIndex).index))
        return QRect();

    int itemIndentation = indentationForItem(viewItemIndex);
    int position = header->sectionViewportPosition(0);
    int size = header->sectionSize(0);

    QRect rect;
    if (q->isRightToLeft())
        rect = QRect(position + size - itemIndentation, coordinateForItem(viewItemIndex),
                     indent, itemHeight(viewItemIndex));
    else
        rect = QRect(position + itemIndentation - indent, coordinateForItem(viewItemIndex),
                     indent, itemHeight(viewItemIndex));
    QStyleOption opt;
    opt.initFrom(q);
    opt.rect = rect;
    return q->style()->subElementRect(QStyle::SE_TreeViewDisclosureItem, &opt, q);
}

QList<QPair<int, int> > QTreeViewPrivate::columnRanges(const QModelIndex &topIndex,
                                                          const QModelIndex &bottomIndex) const
{
    const int topVisual = header->visualIndex(topIndex.column()),
        bottomVisual = header->visualIndex(bottomIndex.column());

    const int start = qMin(topVisual, bottomVisual);
    const int end = qMax(topVisual, bottomVisual);

    QList<int> logicalIndexes;

    //we iterate over the visual indexes to get the logical indexes
    for (int c = start; c <= end; c++) {
        const int logical = header->logicalIndex(c);
        if (!header->isSectionHidden(logical)) {
            logicalIndexes << logical;
        }
    }
    //let's sort the list
    qSort(logicalIndexes.begin(), logicalIndexes.end());

    QList<QPair<int, int> > ret;
    QPair<int, int> current;
    current.first = -2; // -1 is not enough because -1+1 = 0
    current.second = -2;
    for(int i = 0; i < logicalIndexes.count(); ++i) {
        const int logicalColumn = logicalIndexes.at(i);
        if (current.second + 1 != logicalColumn) {
            if (current.first != -2) {
                //let's save the current one
                ret += current;
            }
            //let's start a new one
            current.first = current.second = logicalColumn;
        } else {
            current.second++;
        }
    }

    //let's get the last range
    if (current.first != -2) {
        ret += current;
    }

    return ret;
}

void QTreeViewPrivate::select(const QModelIndex &topIndex, const QModelIndex &bottomIndex,
                              QItemSelectionModel::SelectionFlags command)
{
    Q_Q(QTreeView);
    QItemSelection selection;
    const int top = viewIndex(topIndex),
        bottom = viewIndex(bottomIndex);

    const QList< QPair<int, int> > colRanges = columnRanges(topIndex, bottomIndex);
    QList< QPair<int, int> >::const_iterator it;
    for (it = colRanges.begin(); it != colRanges.end(); ++it) {
        const int left = (*it).first,
            right = (*it).second;

        QModelIndex previous;
        QItemSelectionRange currentRange;
        QStack<QItemSelectionRange> rangeStack;
        for (int i = top; i <= bottom; ++i) {
            QModelIndex index = modelIndex(i);
            QModelIndex parent = index.parent();
            QModelIndex previousParent = previous.parent();
            if (previous.isValid() && parent == previousParent) {
                // same parent
                if (qAbs(previous.row() - index.row()) > 1) {
                    //a hole (hidden index inside a range) has been detected
                    if (currentRange.isValid()) {
                        selection.append(currentRange);
                    }
                    //let's start a new range
                    currentRange = QItemSelectionRange(index.sibling(index.row(), left), index.sibling(index.row(), right));
                } else {
                    QModelIndex tl = model->index(currentRange.top(), currentRange.left(),
                        currentRange.parent());
                    currentRange = QItemSelectionRange(tl, index.sibling(index.row(), right));
                }
            } else if (previous.isValid() && parent == model->index(previous.row(), 0, previousParent)) {
                // item is child of previous
                rangeStack.push(currentRange);
                currentRange = QItemSelectionRange(index.sibling(index.row(), left), index.sibling(index.row(), right));
            } else {
                if (currentRange.isValid())
                    selection.append(currentRange);
                if (rangeStack.isEmpty()) {
                    currentRange = QItemSelectionRange(index.sibling(index.row(), left), index.sibling(index.row(), right));
                } else {
                    currentRange = rangeStack.pop();
                    index = currentRange.bottomRight(); //let's resume the range
                    --i; //we process again the current item
                }
            }
            previous = index;
        }
        if (currentRange.isValid())
            selection.append(currentRange);
        for (int i = 0; i < rangeStack.count(); ++i)
            selection.append(rangeStack.at(i));
    }
    q->selectionModel()->select(selection, command);
}

QPair<int,int> QTreeViewPrivate::startAndEndColumns(const QRect &rect) const
{
    Q_Q(const QTreeView);
    int start = header->visualIndexAt(rect.left());
    int end = header->visualIndexAt(rect.right());
    if (q->isRightToLeft()) {
        start = (start == -1 ? header->count() - 1 : start);
        end = (end == -1 ? 0 : end);
    } else {
        start = (start == -1 ? 0 : start);
        end = (end == -1 ? header->count() - 1 : end);
    }
    return qMakePair<int,int>(qMin(start, end), qMax(start, end));
}

bool QTreeViewPrivate::hasVisibleChildren(const QModelIndex& parent) const
{
    Q_Q(const QTreeView);
    if (model->hasChildren(parent)) {
        if (hiddenIndexes.isEmpty())
            return true;
        if (q->isIndexHidden(parent))
            return false;
        int rowCount = model->rowCount(parent);
        for (int i = 0; i < rowCount; ++i) {
            if (!q->isRowHidden(i, parent))
                return true;
        }
        if (rowCount == 0)
            return true;
    }
    return false;
}

void QTreeViewPrivate::_q_sortIndicatorChanged(int column, Qt::SortOrder order)
{
    model->sort(column, order);
}

/*!
  \reimp
 */
void QTreeView::currentChanged(const QModelIndex &current, const QModelIndex &previous)
{
    QAbstractItemView::currentChanged(current, previous);

    if (allColumnsShowFocus()) {
        if (previous.isValid()) {
            QRect previousRect = visualRect(previous);
            previousRect.setX(0);
            previousRect.setWidth(viewport()->width());
            viewport()->update(previousRect);
        }
        if (current.isValid()) {
            QRect currentRect = visualRect(current);
            currentRect.setX(0);
            currentRect.setWidth(viewport()->width());
            viewport()->update(currentRect);
        }
    }
#ifndef QT_NO_ACCESSIBILITY
    if (QAccessible::isActive() && current.isValid()) {
#ifdef Q_WS_X11
        int entry = (visualIndex(current) + (header()?1:0))*current.model()->columnCount()+current.column() + 1;
        QAccessible::updateAccessibility(this, entry, QAccessible::Focus);
#else
        int entry = visualIndex(current) + 1;
        if (header())
            ++entry;
        QAccessible::updateAccessibility(viewport(), entry, QAccessible::Focus);
#endif
    }
#endif
}

/*!
  \reimp
 */
void QTreeView::selectionChanged(const QItemSelection &selected,
                                 const QItemSelection &deselected)
{
    QAbstractItemView::selectionChanged(selected, deselected);
#ifndef QT_NO_ACCESSIBILITY
    if (QAccessible::isActive()) {
        // ### does not work properly for selection ranges.
        QModelIndex sel = selected.indexes().value(0);
        if (sel.isValid()) {
#ifdef Q_WS_X11
            int entry = (visualIndex(sel) + (header()?1:0))*sel.model()->columnCount()+sel.column() + 1;
            Q_ASSERT(entry > 0);
            QAccessible::updateAccessibility(this, entry, QAccessible::Selection);
#else
            int entry = visualIndex(sel) + 1;
            if (header())
                ++entry;
            QAccessible::updateAccessibility(viewport(), entry, QAccessible::Selection);
#endif
        }
        QModelIndex desel = deselected.indexes().value(0);
        if (desel.isValid()) {
#ifdef Q_WS_X11
            int entry = (visualIndex(desel) + (header()?1:0))*desel.model()->columnCount()+desel.column() + 1;
            Q_ASSERT(entry > 0);
            QAccessible::updateAccessibility(this, entry, QAccessible::SelectionRemove);
#else
            int entry = visualIndex(desel) + 1;
            if (header())
                ++entry;
            QAccessible::updateAccessibility(viewport(), entry, QAccessible::SelectionRemove);
#endif
        }
    }
#endif
}

int QTreeView::visualIndex(const QModelIndex &index) const
{
    Q_D(const QTreeView);
    d->executePostedLayout();
    return d->viewIndex(index);
}

QT_END_NAMESPACE

#include "moc_qtreeview.cpp"

#endif // QT_NO_TREEVIEW
