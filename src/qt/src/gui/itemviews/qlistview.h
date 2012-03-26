/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef QLISTVIEW_H
#define QLISTVIEW_H

#include <QtGui/qabstractitemview.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Gui)

#ifndef QT_NO_LISTVIEW

class QListViewPrivate;

class Q_GUI_EXPORT QListView : public QAbstractItemView
{
    Q_OBJECT
    Q_ENUMS(Movement Flow ResizeMode LayoutMode ViewMode)
    Q_PROPERTY(Movement movement READ movement WRITE setMovement)
    Q_PROPERTY(Flow flow READ flow WRITE setFlow)
    Q_PROPERTY(bool isWrapping READ isWrapping WRITE setWrapping)
    Q_PROPERTY(ResizeMode resizeMode READ resizeMode WRITE setResizeMode)
    Q_PROPERTY(LayoutMode layoutMode READ layoutMode WRITE setLayoutMode)
    Q_PROPERTY(int spacing READ spacing WRITE setSpacing)
    Q_PROPERTY(QSize gridSize READ gridSize WRITE setGridSize)
    Q_PROPERTY(ViewMode viewMode READ viewMode WRITE setViewMode)
    Q_PROPERTY(int modelColumn READ modelColumn WRITE setModelColumn)
    Q_PROPERTY(bool uniformItemSizes READ uniformItemSizes WRITE setUniformItemSizes)
    Q_PROPERTY(int batchSize READ batchSize WRITE setBatchSize)
    Q_PROPERTY(bool wordWrap READ wordWrap WRITE setWordWrap)
    Q_PROPERTY(bool selectionRectVisible READ isSelectionRectVisible WRITE setSelectionRectVisible)

public:
    enum Movement { Static, Free, Snap };
    enum Flow { LeftToRight, TopToBottom };
    enum ResizeMode { Fixed, Adjust };
    enum LayoutMode { SinglePass, Batched };
    enum ViewMode { ListMode, IconMode };

    explicit QListView(QWidget *parent = 0);
    ~QListView();

    void setMovement(Movement movement);
    Movement movement() const;

    void setFlow(Flow flow);
    Flow flow() const;

    void setWrapping(bool enable);
    bool isWrapping() const;

    void setResizeMode(ResizeMode mode);
    ResizeMode resizeMode() const;

    void setLayoutMode(LayoutMode mode);
    LayoutMode layoutMode() const;

    void setSpacing(int space);
    int spacing() const;

    void setBatchSize(int batchSize);
    int batchSize() const;

    void setGridSize(const QSize &size);
    QSize gridSize() const;

    void setViewMode(ViewMode mode);
    ViewMode viewMode() const;

    void clearPropertyFlags();

    bool isRowHidden(int row) const;
    void setRowHidden(int row, bool hide);

    void setModelColumn(int column);
    int modelColumn() const;

    void setUniformItemSizes(bool enable);
    bool uniformItemSizes() const;

    void setWordWrap(bool on);
    bool wordWrap() const;

    void setSelectionRectVisible(bool show);
    bool isSelectionRectVisible() const;

    QRect visualRect(const QModelIndex &index) const;
    void scrollTo(const QModelIndex &index, ScrollHint hint = EnsureVisible);
    QModelIndex indexAt(const QPoint &p) const;

    void doItemsLayout();
    void reset();
    void setRootIndex(const QModelIndex &index);

Q_SIGNALS:
    void indexesMoved(const QModelIndexList &indexes);

protected:
    QListView(QListViewPrivate &, QWidget *parent = 0);

    bool event(QEvent *e);

    void scrollContentsBy(int dx, int dy);

    void resizeContents(int width, int height);
    QSize contentsSize() const;

    void dataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight);
    void rowsInserted(const QModelIndex &parent, int start, int end);
    void rowsAboutToBeRemoved(const QModelIndex &parent, int start, int end);

    void mouseMoveEvent(QMouseEvent *e);
    void mouseReleaseEvent(QMouseEvent *e);

    void timerEvent(QTimerEvent *e);
    void resizeEvent(QResizeEvent *e);
#ifndef QT_NO_DRAGANDDROP
    void dragMoveEvent(QDragMoveEvent *e);
    void dragLeaveEvent(QDragLeaveEvent *e);
    void dropEvent(QDropEvent *e);
    void startDrag(Qt::DropActions supportedActions);

    void internalDrop(QDropEvent *e);
    void internalDrag(Qt::DropActions supportedActions);
#endif // QT_NO_DRAGANDDROP

    QStyleOptionViewItem viewOptions() const;
    void paintEvent(QPaintEvent *e);

    int horizontalOffset() const;
    int verticalOffset() const;
    QModelIndex moveCursor(CursorAction cursorAction, Qt::KeyboardModifiers modifiers);
    QRect rectForIndex(const QModelIndex &index) const;
    void setPositionForIndex(const QPoint &position, const QModelIndex &index);

    void setSelection(const QRect &rect, QItemSelectionModel::SelectionFlags command);
    QRegion visualRegionForSelection(const QItemSelection &selection) const;
    QModelIndexList selectedIndexes() const;

    void updateGeometries();

    bool isIndexHidden(const QModelIndex &index) const;

    void selectionChanged(const QItemSelection &selected, const QItemSelection &deselected);
    void currentChanged(const QModelIndex &current, const QModelIndex &previous);

private:
    friend class QAccessibleItemView;
    int visualIndex(const QModelIndex &index) const;

    Q_DECLARE_PRIVATE(QListView)
    Q_DISABLE_COPY(QListView)
};

#endif // QT_NO_LISTVIEW

QT_END_NAMESPACE

QT_END_HEADER

#endif // QLISTVIEW_H
