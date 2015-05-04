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

#ifndef QTABLEVIEW_P_H
#define QTABLEVIEW_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtCore/QList>
#include <QtCore/QLinkedList>
#include <QtCore/QMap>
#include <QtCore/QSet>
#include <QtCore/QDebug>
#include "private/qabstractitemview_p.h"

#ifndef QT_NO_TABLEVIEW

QT_BEGIN_NAMESPACE

/** \internal
*
* This is a list of span with a binary index to look up quickly a span at a certain index.
*
* The index is a map of map.
* spans are mentaly divided into sub spans so that the start of any subspans doesn't overlap
* with any other subspans. There is no real representation of the subspans.
* The key of the first map is the row where the subspan starts, the value of the first map is
* a list (map) of all subspans that starts at the same row.  It is indexed with its row
*/
class Q_AUTOTEST_EXPORT QSpanCollection
{
public:
    struct Span
    {
        int m_top;
        int m_left;
        int m_bottom;
        int m_right;
        bool will_be_deleted;
        Span()
        : m_top(-1), m_left(-1), m_bottom(-1), m_right(-1), will_be_deleted(false) { }
        Span(int row, int column, int rowCount, int columnCount)
        : m_top(row), m_left(column), m_bottom(row+rowCount-1), m_right(column+columnCount-1), will_be_deleted(false) { }
        inline int top() const { return m_top; }
        inline int left() const { return m_left; }
        inline int bottom() const { return m_bottom; }
        inline int right() const { return m_right; }
        inline int height() const { return m_bottom - m_top + 1; }
        inline int width() const { return m_right - m_left + 1; }
    };

    ~QSpanCollection()
    {
        qDeleteAll(spans);
    }

    void addSpan(Span *span);
    void updateSpan(Span *span, int old_height);
    Span *spanAt(int x, int y) const;
    void clear();
    QList<Span *> spansInRect(int x, int y, int w, int h) const;

    void updateInsertedRows(int start, int end);
    void updateInsertedColumns(int start, int end);
    void updateRemovedRows(int start, int end);
    void updateRemovedColumns(int start, int end);

#ifdef QT_BUILD_INTERNAL
    bool checkConsistency() const;
#endif

    typedef QLinkedList<Span *> SpanList;
    SpanList spans; //lists of all spans
private:
    //the indexes are negative so the QMap::lowerBound do what i need.
    typedef QMap<int, Span *> SubIndex;
    typedef QMap<int, SubIndex> Index;
    Index index;

    bool cleanSpanSubIndex(SubIndex &subindex, int end, bool update = false);
};

Q_DECLARE_TYPEINFO ( QSpanCollection::Span, Q_MOVABLE_TYPE);


class QTableViewPrivate : public QAbstractItemViewPrivate
{
    Q_DECLARE_PUBLIC(QTableView)
public:
    QTableViewPrivate()
        : showGrid(true), gridStyle(Qt::SolidLine),
          rowSectionAnchor(-1), columnSectionAnchor(-1),
          columnResizeTimerID(0), rowResizeTimerID(0),
          horizontalHeader(0), verticalHeader(0),
          sortingEnabled(false), geometryRecursionBlock(false),
          visualCursor(QPoint())
 {
    wrapItemText = true;
#ifndef QT_NO_DRAGANDDROP
    overwrite = true;
#endif
 }
    void init();
    void trimHiddenSelections(QItemSelectionRange *range) const;

    inline bool isHidden(int row, int col) const {
        return verticalHeader->isSectionHidden(row)
            || horizontalHeader->isSectionHidden(col);
    }
    inline int visualRow(int logicalRow) const {
        return verticalHeader->visualIndex(logicalRow);
    }
    inline int visualColumn(int logicalCol) const {
        return horizontalHeader->visualIndex(logicalCol);
    }
    inline int logicalRow(int visualRow) const {
        return verticalHeader->logicalIndex(visualRow);
    }
    inline int logicalColumn(int visualCol) const {
        return horizontalHeader->logicalIndex(visualCol);
    }

    inline int accessibleTable2Index(const QModelIndex &index) const {
        const int vHeader = verticalHeader ? 1 : 0;
        return (index.row() + (horizontalHeader ? 1 : 0)) * (index.model()->columnCount() + vHeader)
            + index.column() + vHeader;
    }

    int sectionSpanEndLogical(const QHeaderView *header, int logical, int span) const;
    int sectionSpanSize(const QHeaderView *header, int logical, int span) const;
    bool spanContainsSection(const QHeaderView *header, int logical, int spanLogical, int span) const;
    void drawAndClipSpans(const QRegion &area, QPainter *painter,
                          const QStyleOptionViewItem &option, QBitArray *drawn,
                          int firstVisualRow, int lastVisualRow, int firstVisualColumn, int lastVisualColumn);
    void drawCell(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index);
    int widthHintForIndex(const QModelIndex &index, int hint, const QStyleOptionViewItem &option) const;
    int heightHintForIndex(const QModelIndex &index, int hint, QStyleOptionViewItem &option) const;

    bool showGrid;
    Qt::PenStyle gridStyle;
    int rowSectionAnchor;
    int columnSectionAnchor;
    int columnResizeTimerID;
    int rowResizeTimerID;
    QList<int> columnsToUpdate;
    QList<int> rowsToUpdate;
    QHeaderView *horizontalHeader;
    QHeaderView *verticalHeader;
    QWidget *cornerWidget;
    bool sortingEnabled;
    bool geometryRecursionBlock;
    QPoint visualCursor;  // (Row,column) cell coordinates to track through span navigation.

    QSpanCollection spans;

    void setSpan(int row, int column, int rowSpan, int columnSpan);
    QSpanCollection::Span span(int row, int column) const;
    inline int rowSpan(int row, int column) const {
        return span(row, column).height();
    }
    inline int columnSpan(int row, int column) const {
        return span(row, column).width();
    }
    inline bool hasSpans() const {
        return !spans.spans.isEmpty();
    }
    inline int rowSpanHeight(int row, int span) const {
        return sectionSpanSize(verticalHeader, row, span);
    }
    inline int columnSpanWidth(int column, int span) const {
        return sectionSpanSize(horizontalHeader, column, span);
    }
    inline int rowSpanEndLogical(int row, int span) const {
        return sectionSpanEndLogical(verticalHeader, row, span);
    }
    inline int columnSpanEndLogical(int column, int span) const {
        return sectionSpanEndLogical(horizontalHeader, column, span);
    }

    inline bool isRowHidden(int row) const {
        return verticalHeader->isSectionHidden(row);
    }
    inline bool isColumnHidden(int column) const {
        return horizontalHeader->isSectionHidden(column);
    }
    inline bool isCellEnabled(int row, int column) const {
        return isIndexEnabled(model->index(row, column, root));
    }
    inline bool isVisualRowHiddenOrDisabled(int row, int column) const {
        int r = logicalRow(row);
        int c = logicalColumn(column);
        return isRowHidden(r) || !isCellEnabled(r, c);
    }
    inline bool isVisualColumnHiddenOrDisabled(int row, int column) const {
        int r = logicalRow(row);
        int c = logicalColumn(column);
        return isColumnHidden(c) || !isCellEnabled(r, c);
    }

    QRect visualSpanRect(const QSpanCollection::Span &span) const;

    void _q_selectRow(int row);
    void _q_selectColumn(int column);

    void selectRow(int row, bool anchor);
    void selectColumn(int column, bool anchor);

    void _q_updateSpanInsertedRows(const QModelIndex &parent, int start, int end);
    void _q_updateSpanInsertedColumns(const QModelIndex &parent, int start, int end);
    void _q_updateSpanRemovedRows(const QModelIndex &parent, int start, int end);
    void _q_updateSpanRemovedColumns(const QModelIndex &parent, int start, int end);
};

QT_END_NAMESPACE

#endif // QT_NO_TABLEVIEW

#endif // QTABLEVIEW_P_H
