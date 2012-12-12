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

#ifndef QABSTRACTITEMVIEW_H
#define QABSTRACTITEMVIEW_H

#include <QtGui/qabstractscrollarea.h>
#include <QtCore/qabstractitemmodel.h>
#include <QtGui/qitemselectionmodel.h>
#include <QtGui/qabstractitemdelegate.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Gui)

#ifndef QT_NO_ITEMVIEWS

class QMenu;
class QDrag;
class QEvent;
class QAbstractItemViewPrivate;

class Q_GUI_EXPORT QAbstractItemView : public QAbstractScrollArea
{
    Q_OBJECT
    Q_ENUMS(SelectionMode SelectionBehavior ScrollHint ScrollMode DragDropMode)
    Q_FLAGS(EditTriggers)
    Q_PROPERTY(bool autoScroll READ hasAutoScroll WRITE setAutoScroll)
    Q_PROPERTY(int autoScrollMargin READ autoScrollMargin WRITE setAutoScrollMargin)
    Q_PROPERTY(EditTriggers editTriggers READ editTriggers WRITE setEditTriggers)
    Q_PROPERTY(bool tabKeyNavigation READ tabKeyNavigation WRITE setTabKeyNavigation)
#ifndef QT_NO_DRAGANDDROP
    Q_PROPERTY(bool showDropIndicator READ showDropIndicator WRITE setDropIndicatorShown)
    Q_PROPERTY(bool dragEnabled READ dragEnabled WRITE setDragEnabled)
    Q_PROPERTY(bool dragDropOverwriteMode READ dragDropOverwriteMode WRITE setDragDropOverwriteMode)
    Q_PROPERTY(DragDropMode dragDropMode READ dragDropMode WRITE setDragDropMode)
    Q_PROPERTY(Qt::DropAction defaultDropAction READ defaultDropAction WRITE setDefaultDropAction)
#endif
    Q_PROPERTY(bool alternatingRowColors READ alternatingRowColors WRITE setAlternatingRowColors)
    Q_PROPERTY(SelectionMode selectionMode READ selectionMode WRITE setSelectionMode)
    Q_PROPERTY(SelectionBehavior selectionBehavior READ selectionBehavior WRITE setSelectionBehavior)
    Q_PROPERTY(QSize iconSize READ iconSize WRITE setIconSize)
    Q_PROPERTY(Qt::TextElideMode textElideMode READ textElideMode WRITE setTextElideMode)
    Q_PROPERTY(ScrollMode verticalScrollMode READ verticalScrollMode WRITE setVerticalScrollMode)
    Q_PROPERTY(ScrollMode horizontalScrollMode READ horizontalScrollMode WRITE setHorizontalScrollMode)

public:
    enum SelectionMode {
        NoSelection,
        SingleSelection,
        MultiSelection,
        ExtendedSelection,
        ContiguousSelection
    };

    enum SelectionBehavior {
        SelectItems,
        SelectRows,
        SelectColumns
    };

    enum ScrollHint {
        EnsureVisible,
        PositionAtTop,
        PositionAtBottom,
        PositionAtCenter
    };

    enum EditTrigger {
        NoEditTriggers = 0,
        CurrentChanged = 1,
        DoubleClicked = 2,
        SelectedClicked = 4,
        EditKeyPressed = 8,
        AnyKeyPressed = 16,
        AllEditTriggers = 31
    };

    Q_DECLARE_FLAGS(EditTriggers, EditTrigger)

    enum ScrollMode {
        ScrollPerItem,
        ScrollPerPixel
    };

    explicit QAbstractItemView(QWidget *parent = 0);
    ~QAbstractItemView();

    virtual void setModel(QAbstractItemModel *model);
    QAbstractItemModel *model() const;

    virtual void setSelectionModel(QItemSelectionModel *selectionModel);
    QItemSelectionModel *selectionModel() const;

    void setItemDelegate(QAbstractItemDelegate *delegate);
    QAbstractItemDelegate *itemDelegate() const;

    void setSelectionMode(QAbstractItemView::SelectionMode mode);
    QAbstractItemView::SelectionMode selectionMode() const;

    void setSelectionBehavior(QAbstractItemView::SelectionBehavior behavior);
    QAbstractItemView::SelectionBehavior selectionBehavior() const;

    QModelIndex currentIndex() const;
    QModelIndex rootIndex() const;

    void setEditTriggers(EditTriggers triggers);
    EditTriggers editTriggers() const;

    void setVerticalScrollMode(ScrollMode mode);
    ScrollMode verticalScrollMode() const;

    void setHorizontalScrollMode(ScrollMode mode);
    ScrollMode horizontalScrollMode() const;

    void setAutoScroll(bool enable);
    bool hasAutoScroll() const;

    void setAutoScrollMargin(int margin);
    int autoScrollMargin() const;

    void setTabKeyNavigation(bool enable);
    bool tabKeyNavigation() const;

#ifndef QT_NO_DRAGANDDROP
    void setDropIndicatorShown(bool enable);
    bool showDropIndicator() const;

    void setDragEnabled(bool enable);
    bool dragEnabled() const;

    void setDragDropOverwriteMode(bool overwrite);
    bool dragDropOverwriteMode() const;

    enum DragDropMode {
        NoDragDrop,
        DragOnly,
        DropOnly,
        DragDrop,
        InternalMove
    };

    void setDragDropMode(DragDropMode behavior);
    DragDropMode dragDropMode() const;

    void setDefaultDropAction(Qt::DropAction dropAction);
    Qt::DropAction defaultDropAction() const;
#endif

    void setAlternatingRowColors(bool enable);
    bool alternatingRowColors() const;

    void setIconSize(const QSize &size);
    QSize iconSize() const;

    void setTextElideMode(Qt::TextElideMode mode);
    Qt::TextElideMode textElideMode() const;

    virtual void keyboardSearch(const QString &search);

    virtual QRect visualRect(const QModelIndex &index) const = 0;
    virtual void scrollTo(const QModelIndex &index, ScrollHint hint = EnsureVisible) = 0;
    virtual QModelIndex indexAt(const QPoint &point) const = 0;

    QSize sizeHintForIndex(const QModelIndex &index) const;
    virtual int sizeHintForRow(int row) const;
    virtual int sizeHintForColumn(int column) const;

    void openPersistentEditor(const QModelIndex &index);
    void closePersistentEditor(const QModelIndex &index);

    void setIndexWidget(const QModelIndex &index, QWidget *widget);
    QWidget *indexWidget(const QModelIndex &index) const;

    void setItemDelegateForRow(int row, QAbstractItemDelegate *delegate);
    QAbstractItemDelegate *itemDelegateForRow(int row) const;

    void setItemDelegateForColumn(int column, QAbstractItemDelegate *delegate);
    QAbstractItemDelegate *itemDelegateForColumn(int column) const;

    QAbstractItemDelegate *itemDelegate(const QModelIndex &index) const;

    virtual QVariant inputMethodQuery(Qt::InputMethodQuery query) const;

#ifdef Q_NO_USING_KEYWORD
    inline void update() { QAbstractScrollArea::update(); }
#else
    using QAbstractScrollArea::update;
#endif

public Q_SLOTS:
    virtual void reset();
    virtual void setRootIndex(const QModelIndex &index);
    virtual void doItemsLayout();
    virtual void selectAll();
    void edit(const QModelIndex &index);
    void clearSelection();
    void setCurrentIndex(const QModelIndex &index);
    void scrollToTop();
    void scrollToBottom();
    void update(const QModelIndex &index);

protected Q_SLOTS:
    virtual void dataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight);
    virtual void rowsInserted(const QModelIndex &parent, int start, int end);
    virtual void rowsAboutToBeRemoved(const QModelIndex &parent, int start, int end);
    virtual void selectionChanged(const QItemSelection &selected, const QItemSelection &deselected);
    virtual void currentChanged(const QModelIndex &current, const QModelIndex &previous);
    virtual void updateEditorData();
    virtual void updateEditorGeometries();
    virtual void updateGeometries();
    virtual void verticalScrollbarAction(int action);
    virtual void horizontalScrollbarAction(int action);
    virtual void verticalScrollbarValueChanged(int value);
    virtual void horizontalScrollbarValueChanged(int value);
    virtual void closeEditor(QWidget *editor, QAbstractItemDelegate::EndEditHint hint);
    virtual void commitData(QWidget *editor);
    virtual void editorDestroyed(QObject *editor);

Q_SIGNALS:
    void pressed(const QModelIndex &index);
    void clicked(const QModelIndex &index);
    void doubleClicked(const QModelIndex &index);

    void activated(const QModelIndex &index);
    void entered(const QModelIndex &index);
    void viewportEntered();

protected:
    QAbstractItemView(QAbstractItemViewPrivate &, QWidget *parent = 0);

    void setHorizontalStepsPerItem(int steps);
    int horizontalStepsPerItem() const;
    void setVerticalStepsPerItem(int steps);
    int verticalStepsPerItem() const;

    enum CursorAction { MoveUp, MoveDown, MoveLeft, MoveRight,
                        MoveHome, MoveEnd, MovePageUp, MovePageDown,
                        MoveNext, MovePrevious };
    virtual QModelIndex moveCursor(CursorAction cursorAction,
                                   Qt::KeyboardModifiers modifiers) = 0;

    virtual int horizontalOffset() const = 0;
    virtual int verticalOffset() const = 0;

    virtual bool isIndexHidden(const QModelIndex &index) const = 0;

    virtual void setSelection(const QRect &rect, QItemSelectionModel::SelectionFlags command) = 0;
    virtual QRegion visualRegionForSelection(const QItemSelection &selection) const = 0;
    virtual QModelIndexList selectedIndexes() const;

    virtual bool edit(const QModelIndex &index, EditTrigger trigger, QEvent *event);

    virtual QItemSelectionModel::SelectionFlags selectionCommand(const QModelIndex &index,
                                                                 const QEvent *event = 0) const;

#ifndef QT_NO_DRAGANDDROP
    virtual void startDrag(Qt::DropActions supportedActions);
#endif

    virtual QStyleOptionViewItem viewOptions() const;

    enum State {
        NoState,
        DraggingState,
        DragSelectingState,
        EditingState,
        ExpandingState,
        CollapsingState,
        AnimatingState
    };

    State state() const;
    void setState(State state);

    void scheduleDelayedItemsLayout();
    void executeDelayedItemsLayout();

    void setDirtyRegion(const QRegion &region);
    void scrollDirtyRegion(int dx, int dy);
    QPoint dirtyRegionOffset() const;

    void startAutoScroll();
    void stopAutoScroll();
    void doAutoScroll();

    bool focusNextPrevChild(bool next);
    bool event(QEvent *event);
    bool viewportEvent(QEvent *event);
    void mousePressEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
    void mouseDoubleClickEvent(QMouseEvent *event);
#ifndef QT_NO_DRAGANDDROP
    void dragEnterEvent(QDragEnterEvent *event);
    void dragMoveEvent(QDragMoveEvent *event);
    void dragLeaveEvent(QDragLeaveEvent *event);
    void dropEvent(QDropEvent *event);
#endif
    void focusInEvent(QFocusEvent *event);
    void focusOutEvent(QFocusEvent *event);
    void keyPressEvent(QKeyEvent *event);
    void resizeEvent(QResizeEvent *event);
    void timerEvent(QTimerEvent *event);
    void inputMethodEvent(QInputMethodEvent *event);

#ifndef QT_NO_DRAGANDDROP
    enum DropIndicatorPosition { OnItem, AboveItem, BelowItem, OnViewport };
    DropIndicatorPosition dropIndicatorPosition() const;
#endif

private:
    Q_DECLARE_PRIVATE(QAbstractItemView)
    Q_DISABLE_COPY(QAbstractItemView)
    Q_PRIVATE_SLOT(d_func(), void _q_columnsAboutToBeRemoved(const QModelIndex&, int, int))
    Q_PRIVATE_SLOT(d_func(), void _q_columnsRemoved(const QModelIndex&, int, int))
    Q_PRIVATE_SLOT(d_func(), void _q_columnsInserted(const QModelIndex&, int, int))
    Q_PRIVATE_SLOT(d_func(), void _q_rowsInserted(const QModelIndex&, int, int))
    Q_PRIVATE_SLOT(d_func(), void _q_rowsRemoved(const QModelIndex&, int, int))
    Q_PRIVATE_SLOT(d_func(), void _q_modelDestroyed())
    Q_PRIVATE_SLOT(d_func(), void _q_layoutChanged())
    Q_PRIVATE_SLOT(d_func(), void _q_headerDataChanged())

    friend class QTreeViewPrivate; // needed to compile with MSVC
    friend class QAccessibleItemRow;
    friend class QListModeViewBase;
    friend class QListViewPrivate; // needed to compile for Symbian emulator
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QAbstractItemView::EditTriggers)

#endif // QT_NO_ITEMVIEWS

QT_END_NAMESPACE

QT_END_HEADER

#endif // QABSTRACTITEMVIEW_H
