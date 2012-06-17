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

#ifndef QACCESSIBLE2_H
#define QACCESSIBLE2_H

#include <QtGui/qaccessible.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Gui)

#ifndef QT_NO_ACCESSIBILITY

class QModelIndex;

namespace QAccessible2
{
    enum CoordinateType
    {
        RelativeToScreen = 0,
        RelativeToParent = 1
    };

    enum BoundaryType {
        CharBoundary,
        WordBoundary,
        SentenceBoundary,
        ParagraphBoundary,
        LineBoundary,
        NoBoundary
    };

    enum TableModelChangeType {
        TableModelChangeInsert,
        TableModelChangeDelete,
        TableModelChangeUpdate
    };

    struct TableModelChange {
        int firstColumn;
        int firstRow;
        int lastColumn;
        int lastRow;
        TableModelChangeType type;

        TableModelChange()
            : firstColumn(0), firstRow(0), lastColumn(0), lastRow(0), type(TableModelChangeUpdate)
        {}
    };
}

class Q_GUI_EXPORT QAccessible2Interface
{
public:
    virtual ~QAccessible2Interface() {}
};

// catch-all functions. If an accessible class doesn't implement interface T, return 0
inline QAccessible2Interface *qAccessibleValueCastHelper() { return 0; }
inline QAccessible2Interface *qAccessibleTextCastHelper() { return 0; }
inline QAccessible2Interface *qAccessibleEditableTextCastHelper() { return 0; }
inline QAccessible2Interface *qAccessibleTableCastHelper() { return 0; }
inline QAccessible2Interface *qAccessibleActionCastHelper() { return 0; }
inline QAccessible2Interface *qAccessibleImageCastHelper() { return 0; }
inline QAccessible2Interface *qAccessibleTable2CastHelper() { return 0; }

#define Q_ACCESSIBLE_OBJECT \
    public: \
    QAccessible2Interface *interface_cast(QAccessible2::InterfaceType t) \
    { \
        switch (t) { \
        case QAccessible2::TextInterface: \
            return qAccessibleTextCastHelper(); \
        case QAccessible2::EditableTextInterface: \
            return qAccessibleEditableTextCastHelper(); \
        case QAccessible2::ValueInterface: \
            return qAccessibleValueCastHelper(); \
        case QAccessible2::TableInterface: \
            return qAccessibleTableCastHelper(); \
        case QAccessible2::ActionInterface: \
            return qAccessibleActionCastHelper(); \
        case QAccessible2::ImageInterface: \
            return qAccessibleImageCastHelper(); \
        case QAccessible2::Table2Interface: \
            return qAccessibleTable2CastHelper(); \
        } \
        return 0; \
    } \
    private:

class Q_GUI_EXPORT QAccessibleTextInterface: public QAccessible2Interface
{
public:
    inline QAccessible2Interface *qAccessibleTextCastHelper() { return this; }

    virtual ~QAccessibleTextInterface() {}

    virtual void addSelection(int startOffset, int endOffset) = 0;
    virtual QString attributes(int offset, int *startOffset, int *endOffset) = 0;
    virtual int cursorPosition() = 0;
    virtual QRect characterRect(int offset, QAccessible2::CoordinateType coordType) = 0;
    virtual int selectionCount() = 0;
    virtual int offsetAtPoint(const QPoint &point, QAccessible2::CoordinateType coordType) = 0;
    virtual void selection(int selectionIndex, int *startOffset, int *endOffset) = 0;
    virtual QString text(int startOffset, int endOffset) = 0;
    virtual QString textBeforeOffset (int offset, QAccessible2::BoundaryType boundaryType,
                              int *startOffset, int *endOffset) = 0;
    virtual QString textAfterOffset(int offset, QAccessible2::BoundaryType boundaryType,
                            int *startOffset, int *endOffset) = 0;
    virtual QString textAtOffset(int offset, QAccessible2::BoundaryType boundaryType,
                         int *startOffset, int *endOffset) = 0;
    virtual void removeSelection(int selectionIndex) = 0;
    virtual void setCursorPosition(int position) = 0;
    virtual void setSelection(int selectionIndex, int startOffset, int endOffset) = 0;
    virtual int characterCount() = 0;
    virtual void scrollToSubstring(int startIndex, int endIndex) = 0;
};

class Q_GUI_EXPORT QAccessibleEditableTextInterface: public QAccessible2Interface
{
public:
    inline QAccessible2Interface *qAccessibleEditableTextCastHelper() { return this; }

    virtual ~QAccessibleEditableTextInterface() {}

    virtual void copyText(int startOffset, int endOffset) = 0;
    virtual void deleteText(int startOffset, int endOffset) = 0;
    virtual void insertText(int offset, const QString &text) = 0;
    virtual void cutText(int startOffset, int endOffset) = 0;
    virtual void pasteText(int offset) = 0;
    virtual void replaceText(int startOffset, int endOffset, const QString &text) = 0;
    virtual void setAttributes(int startOffset, int endOffset, const QString &attributes) = 0;
};

class Q_GUI_EXPORT QAccessibleSimpleEditableTextInterface: public QAccessibleEditableTextInterface
{
public:
    QAccessibleSimpleEditableTextInterface(QAccessibleInterface *accessibleInterface);

    void copyText(int startOffset, int endOffset);
    void deleteText(int startOffset, int endOffset);
    void insertText(int offset, const QString &text);
    void cutText(int startOffset, int endOffset);
    void pasteText(int offset);
    void replaceText(int startOffset, int endOffset, const QString &text);
    inline void setAttributes(int, int, const QString &) {}

private:
    QAccessibleInterface *iface;
};

class Q_GUI_EXPORT QAccessibleValueInterface: public QAccessible2Interface
{
public:
    inline QAccessible2Interface *qAccessibleValueCastHelper() { return this; }

    virtual ~QAccessibleValueInterface() {}

    virtual QVariant currentValue() = 0;
    virtual void setCurrentValue(const QVariant &value) = 0;
    virtual QVariant maximumValue() = 0;
    virtual QVariant minimumValue() = 0;
};

class Q_GUI_EXPORT QAccessibleTableInterface: public QAccessible2Interface
{
public:
    inline QAccessible2Interface *qAccessibleTableCastHelper() { return this; }

    virtual QAccessibleInterface *accessibleAt(int row, int column) = 0;
    virtual QAccessibleInterface *caption() = 0;
    virtual int childIndex(int rowIndex, int columnIndex) = 0;
    virtual QString columnDescription(int column) = 0;
    virtual int columnSpan(int row, int column) = 0;
    virtual QAccessibleInterface *columnHeader() = 0;
    virtual int columnIndex(int childIndex) = 0;
    virtual int columnCount() = 0;
    virtual int rowCount() = 0;
    virtual int selectedColumnCount() = 0;
    virtual int selectedRowCount() = 0;
    virtual QString rowDescription(int row) = 0;
    virtual int rowSpan(int row, int column) = 0;
    virtual QAccessibleInterface *rowHeader() = 0;
    virtual int rowIndex(int childIndex) = 0;
    virtual int selectedRows(int maxRows, QList<int> *rows) = 0;
    virtual int selectedColumns(int maxColumns, QList<int> *columns) = 0;
    virtual QAccessibleInterface *summary() = 0;
    virtual bool isColumnSelected(int column) = 0;
    virtual bool isRowSelected(int row) = 0;
    virtual bool isSelected(int row, int column) = 0;
    virtual void selectRow(int row) = 0;
    virtual void selectColumn(int column) = 0;
    virtual void unselectRow(int row) = 0;
    virtual void unselectColumn(int column) = 0;
    virtual void cellAtIndex(int index, int *row, int *column, int *rowSpan,
                             int *columnSpan, bool *isSelected) = 0;
};

class Q_GUI_EXPORT QAccessibleTable2CellInterface: public QAccessibleInterface
{
public:
    //            Returns the number of columns occupied by this cell accessible.
    virtual int columnExtent() const = 0;

    //            Returns the column headers as an array of cell accessibles.
    virtual QList<QAccessibleInterface*> columnHeaderCells() const = 0;

    //            Translates this cell accessible into the corresponding column index.
    virtual int columnIndex() const = 0;
    //            Returns the number of rows occupied by this cell accessible.
    virtual int rowExtent() const = 0;
    //            Returns the row headers as an array of cell accessibles.
    virtual QList<QAccessibleInterface*> rowHeaderCells() const = 0;
    //            Translates this cell accessible into the corresponding row index.
    virtual int rowIndex() const = 0;
    //            Returns a boolean value indicating whether this cell is selected.
    virtual bool isSelected() const = 0;

    //            Gets the row and column indexes and extents of this cell accessible and whether or not it is selected.
    virtual void rowColumnExtents(int *row, int *column, int *rowExtents, int *columnExtents, bool *selected) const = 0;
    //            Returns a reference to the accessbile of the containing table.
    virtual QAccessibleTable2Interface* table() const = 0;

    // #### Qt5 this should not be here but part of the state
    virtual bool isExpandable() const = 0;
};

class Q_GUI_EXPORT QAccessibleTable2Interface: public QAccessible2Interface
{
public:
    inline QAccessible2Interface *qAccessibleTable2CastHelper() { return this; }

    // Returns the cell at the specified row and column in the table.
    virtual QAccessibleTable2CellInterface *cellAt (int row, int column) const = 0;
    // Returns the caption for the table.
    virtual QAccessibleInterface *caption() const = 0;
    // Returns the description text of the specified column in the table.
    virtual QString columnDescription(int column) const = 0;
    // Returns the total number of columns in table.
    virtual int columnCount() const = 0;
    // Returns the total number of rows in table.
    virtual int rowCount() const = 0;
    // Returns the total number of selected cells.
    virtual int selectedCellCount() const = 0;
    // Returns the total number of selected columns.
    virtual int selectedColumnCount() const = 0;
    // Returns the total number of selected rows.
    virtual int selectedRowCount() const = 0;
    // Returns the description text of the specified row in the table.
    virtual QString rowDescription(int row) const = 0;
    // Returns a list of accessibles currently selected.
    virtual QList<QAccessibleTable2CellInterface*> selectedCells() const = 0;
    // Returns a list of column indexes currently selected (0 based).
    virtual QList<int> selectedColumns() const = 0;
    // Returns a list of row indexes currently selected (0 based).
    virtual QList<int> selectedRows() const = 0;
    // Returns the summary description of the table.
    virtual QAccessibleInterface *summary() const = 0;
    // Returns a boolean value indicating whether the specified column is completely selected.
    virtual bool isColumnSelected(int column) const = 0;
    // Returns a boolean value indicating whether the specified row is completely selected.
    virtual bool isRowSelected(int row) const = 0;
    // Selects a row and unselects all previously selected rows.
    virtual bool selectRow(int row) = 0;
    // Selects a column and unselects all previously selected columns.
    virtual bool selectColumn(int column) = 0;
    // Unselects one row, leaving other selected rows selected (if any).
    virtual bool unselectRow(int row) = 0;
    // Unselects one column, leaving other selected columns selected (if any).
    virtual bool unselectColumn(int column) = 0;
    // Returns the type and extents describing how a table changed.
    virtual QAccessible2::TableModelChange modelChange() const = 0;

protected:
    // These functions are called when the model changes.
    virtual void modelReset() = 0;
    virtual void rowsInserted(const QModelIndex &parent, int first, int last) = 0;
    virtual void rowsRemoved(const QModelIndex &parent, int first, int last) = 0;
    virtual void columnsInserted(const QModelIndex &parent, int first, int last) = 0;
    virtual void columnsRemoved(const QModelIndex &parent, int first, int last) = 0;
    virtual void rowsMoved( const QModelIndex &parent, int start, int end, const QModelIndex &destination, int row) = 0;
    virtual void columnsMoved( const QModelIndex &parent, int start, int end, const QModelIndex &destination, int column) = 0;

friend class QAbstractItemView;
friend class QAbstractItemViewPrivate;
};

class Q_GUI_EXPORT QAccessibleActionInterface : public QAccessible2Interface
{
public:
    inline QAccessible2Interface *qAccessibleActionCastHelper() { return this; }

    virtual int actionCount() = 0;
    virtual void doAction(int actionIndex) = 0;
    virtual QString description(int actionIndex) = 0;
    virtual QString name(int actionIndex) = 0;
    virtual QString localizedName(int actionIndex) = 0;
    virtual QStringList keyBindings(int actionIndex) = 0;
};

class Q_GUI_EXPORT QAccessibleImageInterface : public QAccessible2Interface
{
public:
    inline QAccessible2Interface *qAccessibleImageCastHelper() { return this; }

    virtual QString imageDescription() = 0;
    virtual QSize imageSize() = 0;
    virtual QRect imagePosition(QAccessible2::CoordinateType coordType) = 0;
};

#endif // QT_NO_ACCESSIBILITY

QT_END_NAMESPACE

QT_END_HEADER

#endif
