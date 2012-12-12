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

#ifndef QTREEVIEW_H
#define QTREEVIEW_H

#include <QtGui/qabstractitemview.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Gui)

#ifndef QT_NO_TREEVIEW

class QTreeViewPrivate;
class QHeaderView;

class Q_GUI_EXPORT QTreeView : public QAbstractItemView
{
    Q_OBJECT
    Q_PROPERTY(int autoExpandDelay READ autoExpandDelay WRITE setAutoExpandDelay)
    Q_PROPERTY(int indentation READ indentation WRITE setIndentation)
    Q_PROPERTY(bool rootIsDecorated READ rootIsDecorated WRITE setRootIsDecorated)
    Q_PROPERTY(bool uniformRowHeights READ uniformRowHeights WRITE setUniformRowHeights)
    Q_PROPERTY(bool itemsExpandable READ itemsExpandable WRITE setItemsExpandable)
    Q_PROPERTY(bool sortingEnabled READ isSortingEnabled WRITE setSortingEnabled)
    Q_PROPERTY(bool animated READ isAnimated WRITE setAnimated)
    Q_PROPERTY(bool allColumnsShowFocus READ allColumnsShowFocus WRITE setAllColumnsShowFocus)
    Q_PROPERTY(bool wordWrap READ wordWrap WRITE setWordWrap)
    Q_PROPERTY(bool headerHidden READ isHeaderHidden WRITE setHeaderHidden)
    Q_PROPERTY(bool expandsOnDoubleClick READ expandsOnDoubleClick WRITE setExpandsOnDoubleClick)

public:
    explicit QTreeView(QWidget *parent = 0);
    ~QTreeView();

    void setModel(QAbstractItemModel *model);
    void setRootIndex(const QModelIndex &index);
    void setSelectionModel(QItemSelectionModel *selectionModel);

    QHeaderView *header() const;
    void setHeader(QHeaderView *header);

    int autoExpandDelay() const;
    void setAutoExpandDelay(int delay);

    int indentation() const;
    void setIndentation(int i);

    bool rootIsDecorated() const;
    void setRootIsDecorated(bool show);

    bool uniformRowHeights() const;
    void setUniformRowHeights(bool uniform);

    bool itemsExpandable() const;
    void setItemsExpandable(bool enable);

    bool expandsOnDoubleClick() const;
    void setExpandsOnDoubleClick(bool enable);

    int columnViewportPosition(int column) const;
    int columnWidth(int column) const;
    void setColumnWidth(int column, int width);
    int columnAt(int x) const;

    bool isColumnHidden(int column) const;
    void setColumnHidden(int column, bool hide);

    bool isHeaderHidden() const;
    void setHeaderHidden(bool hide);

    bool isRowHidden(int row, const QModelIndex &parent) const;
    void setRowHidden(int row, const QModelIndex &parent, bool hide);

    bool isFirstColumnSpanned(int row, const QModelIndex &parent) const;
    void setFirstColumnSpanned(int row, const QModelIndex &parent, bool span);

    bool isExpanded(const QModelIndex &index) const;
    void setExpanded(const QModelIndex &index, bool expand);

    void setSortingEnabled(bool enable);
    bool isSortingEnabled() const;

    void setAnimated(bool enable);
    bool isAnimated() const;

    void setAllColumnsShowFocus(bool enable);
    bool allColumnsShowFocus() const;

    void setWordWrap(bool on);
    bool wordWrap() const;

    void keyboardSearch(const QString &search);

    QRect visualRect(const QModelIndex &index) const;
    void scrollTo(const QModelIndex &index, ScrollHint hint = EnsureVisible);
    QModelIndex indexAt(const QPoint &p) const;
    QModelIndex indexAbove(const QModelIndex &index) const;
    QModelIndex indexBelow(const QModelIndex &index) const;

    void doItemsLayout();
    void reset();

    void sortByColumn(int column, Qt::SortOrder order);

    void dataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight);
    void selectAll();

Q_SIGNALS:
    void expanded(const QModelIndex &index);
    void collapsed(const QModelIndex &index);

public Q_SLOTS:
    void hideColumn(int column);
    void showColumn(int column);
    void expand(const QModelIndex &index);
    void collapse(const QModelIndex &index);
    void resizeColumnToContents(int column);
    void sortByColumn(int column);
    void expandAll();
    void collapseAll();
    void expandToDepth(int depth);

protected Q_SLOTS:
    void columnResized(int column, int oldSize, int newSize);
    void columnCountChanged(int oldCount, int newCount);
    void columnMoved();
    void reexpand();
    void rowsRemoved(const QModelIndex &parent, int first, int last);

protected:
    QTreeView(QTreeViewPrivate &dd, QWidget *parent = 0);
    void scrollContentsBy(int dx, int dy);
    void rowsInserted(const QModelIndex &parent, int start, int end);
    void rowsAboutToBeRemoved(const QModelIndex &parent, int start, int end);

    QModelIndex moveCursor(CursorAction cursorAction, Qt::KeyboardModifiers modifiers);
    int horizontalOffset() const;
    int verticalOffset() const;

    void setSelection(const QRect &rect, QItemSelectionModel::SelectionFlags command);
    QRegion visualRegionForSelection(const QItemSelection &selection) const;
    QModelIndexList selectedIndexes() const;

    void timerEvent(QTimerEvent *event);
    void paintEvent(QPaintEvent *event);

    void drawTree(QPainter *painter, const QRegion &region) const;
    virtual void drawRow(QPainter *painter,
                         const QStyleOptionViewItem &options,
                         const QModelIndex &index) const;
    virtual void drawBranches(QPainter *painter,
                              const QRect &rect,
                              const QModelIndex &index) const;

    void mousePressEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
    void mouseDoubleClickEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void keyPressEvent(QKeyEvent *event);
#ifndef QT_NO_DRAGANDDROP
    void dragMoveEvent(QDragMoveEvent *event);
#endif
    bool viewportEvent(QEvent *event);

    void updateGeometries();

    int sizeHintForColumn(int column) const;
    int indexRowSizeHint(const QModelIndex &index) const;
    int rowHeight(const QModelIndex &index) const;

    void horizontalScrollbarAction(int action);

    bool isIndexHidden(const QModelIndex &index) const;
    void selectionChanged(const QItemSelection &selected,
                          const QItemSelection &deselected);
    void currentChanged(const QModelIndex &current, const QModelIndex &previous);

private:
    friend class QAccessibleItemView;
    friend class QAccessibleTable2;
    friend class QAccessibleTree;
    friend class QAccessibleTable2Cell;
    int visualIndex(const QModelIndex &index) const;

    Q_DECLARE_PRIVATE(QTreeView)
    Q_DISABLE_COPY(QTreeView)
#ifndef QT_NO_ANIMATION
    Q_PRIVATE_SLOT(d_func(), void _q_endAnimatedOperation())
#endif //QT_NO_ANIMATION
    Q_PRIVATE_SLOT(d_func(), void _q_modelAboutToBeReset())
    Q_PRIVATE_SLOT(d_func(), void _q_sortIndicatorChanged(int column, Qt::SortOrder order))
};

#endif // QT_NO_TREEVIEW

QT_END_NAMESPACE

QT_END_HEADER

#endif // QTREEVIEW_H
