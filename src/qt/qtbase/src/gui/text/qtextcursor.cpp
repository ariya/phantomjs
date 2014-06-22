/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
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

#include "qtextcursor.h"
#include "qtextcursor_p.h"
#include "qglobal.h"
#include "qtextdocumentfragment.h"
#include "qtextdocumentfragment_p.h"
#include "qtextlist.h"
#include "qtexttable.h"
#include "qtexttable_p.h"
#include "qtextengine_p.h"
#include "qabstracttextdocumentlayout.h"

#include <qtextlayout.h>
#include <qdebug.h>

QT_BEGIN_NAMESPACE

enum {
    AdjustPrev = 0x1,
    AdjustUp = 0x3,
    AdjustNext = 0x4,
    AdjustDown = 0x12
};

QTextCursorPrivate::QTextCursorPrivate(QTextDocumentPrivate *p)
    : priv(p), x(0), position(0), anchor(0), adjusted_anchor(0),
      currentCharFormat(-1), visualNavigation(false), keepPositionOnInsert(false),
      changed(false)
{
    priv->addCursor(this);
}

QTextCursorPrivate::QTextCursorPrivate(const QTextCursorPrivate &rhs)
    : QSharedData(rhs)
{
    position = rhs.position;
    anchor = rhs.anchor;
    adjusted_anchor = rhs.adjusted_anchor;
    priv = rhs.priv;
    x = rhs.x;
    currentCharFormat = rhs.currentCharFormat;
    visualNavigation = rhs.visualNavigation;
    keepPositionOnInsert = rhs.keepPositionOnInsert;
    changed = rhs.changed;
    priv->addCursor(this);
}

QTextCursorPrivate::~QTextCursorPrivate()
{
    if (priv)
        priv->removeCursor(this);
}

QTextCursorPrivate::AdjustResult QTextCursorPrivate::adjustPosition(int positionOfChange, int charsAddedOrRemoved, QTextUndoCommand::Operation op)
{
    QTextCursorPrivate::AdjustResult result = QTextCursorPrivate::CursorMoved;
    // not(!) <= , so that inserting text adjusts the cursor correctly
    if (position < positionOfChange
        || (position == positionOfChange
            && (op == QTextUndoCommand::KeepCursor
                || keepPositionOnInsert)
            )
         ) {
        result = CursorUnchanged;
    } else {
        if (charsAddedOrRemoved < 0 && position < positionOfChange - charsAddedOrRemoved)
            position = positionOfChange;
        else
            position += charsAddedOrRemoved;

        currentCharFormat = -1;
    }

    if (anchor >= positionOfChange
        && (anchor != positionOfChange || op != QTextUndoCommand::KeepCursor)) {
        if (charsAddedOrRemoved < 0 && anchor < positionOfChange - charsAddedOrRemoved)
            anchor = positionOfChange;
        else
            anchor += charsAddedOrRemoved;
    }

    if (adjusted_anchor >= positionOfChange
        && (adjusted_anchor != positionOfChange || op != QTextUndoCommand::KeepCursor)) {
        if (charsAddedOrRemoved < 0 && adjusted_anchor < positionOfChange - charsAddedOrRemoved)
            adjusted_anchor = positionOfChange;
        else
            adjusted_anchor += charsAddedOrRemoved;
    }

    return result;
}

void QTextCursorPrivate::setX()
{
    if (priv->isInEditBlock() || priv->inContentsChange) {
        x = -1; // mark dirty
        return;
    }

    QTextBlock block = this->block();
    const QTextLayout *layout = blockLayout(block);
    int pos = position - block.position();

    QTextLine line = layout->lineForTextPosition(pos);
    if (line.isValid())
        x = line.cursorToX(pos);
    else
        x = -1; // delayed init.  Makes movePosition() call setX later on again.
}

void QTextCursorPrivate::remove()
{
    if (anchor == position)
        return;
    currentCharFormat = -1;
    int pos1 = position;
    int pos2 = adjusted_anchor;
    QTextUndoCommand::Operation op = QTextUndoCommand::KeepCursor;
    if (pos1 > pos2) {
        pos1 = adjusted_anchor;
        pos2 = position;
        op = QTextUndoCommand::MoveCursor;
    }

    // deleting inside table? -> delete only content
    QTextTable *table = complexSelectionTable();
    if (table) {
        priv->beginEditBlock();
        int startRow, startCol, numRows, numCols;
        selectedTableCells(&startRow, &numRows, &startCol, &numCols);
        clearCells(table, startRow, startCol, numRows, numCols, op);
        adjusted_anchor = anchor = position;
        priv->endEditBlock();
    } else {
        priv->remove(pos1, pos2-pos1, op);
        adjusted_anchor = anchor = position;
    }

}

void QTextCursorPrivate::clearCells(QTextTable *table, int startRow, int startCol, int numRows, int numCols, QTextUndoCommand::Operation op)
{
    priv->beginEditBlock();

    for (int row = startRow; row < startRow + numRows; ++row)
        for (int col = startCol; col < startCol + numCols; ++col) {
            QTextTableCell cell = table->cellAt(row, col);
            const int startPos = cell.firstPosition();
            const int endPos = cell.lastPosition();
            Q_ASSERT(startPos <= endPos);
            priv->remove(startPos, endPos - startPos, op);
        }

    priv->endEditBlock();
}

bool QTextCursorPrivate::canDelete(int pos) const
{
    QTextDocumentPrivate::FragmentIterator fit = priv->find(pos);
    QTextCharFormat fmt = priv->formatCollection()->charFormat((*fit)->format);
    return (fmt.objectIndex() == -1 || fmt.objectType() == QTextFormat::ImageObject);
}

void QTextCursorPrivate::insertBlock(const QTextBlockFormat &format, const QTextCharFormat &charFormat)
{
    QTextFormatCollection *formats = priv->formatCollection();
    int idx = formats->indexForFormat(format);
    Q_ASSERT(formats->format(idx).isBlockFormat());

    priv->insertBlock(position, idx, formats->indexForFormat(charFormat));
    currentCharFormat = -1;
}

void QTextCursorPrivate::adjustCursor(QTextCursor::MoveOperation m)
{
    adjusted_anchor = anchor;
    if (position == anchor)
        return;

    QTextFrame *f_position = priv->frameAt(position);
    QTextFrame *f_anchor = priv->frameAt(adjusted_anchor);

    if (f_position != f_anchor) {
        // find common parent frame
        QList<QTextFrame *> positionChain;
        QList<QTextFrame *> anchorChain;
        QTextFrame *f = f_position;
        while (f) {
            positionChain.prepend(f);
            f = f->parentFrame();
        }
        f = f_anchor;
        while (f) {
            anchorChain.prepend(f);
            f = f->parentFrame();
        }
        Q_ASSERT(positionChain.at(0) == anchorChain.at(0));
        int i = 1;
        int l = qMin(positionChain.size(), anchorChain.size());
        for (; i < l; ++i) {
            if (positionChain.at(i) != anchorChain.at(i))
                break;
        }

        if (m <= QTextCursor::WordLeft) {
            if (i < positionChain.size())
                position = positionChain.at(i)->firstPosition() - 1;
        } else {
            if (i < positionChain.size())
                position = positionChain.at(i)->lastPosition() + 1;
        }
        if (position < adjusted_anchor) {
            if (i < anchorChain.size())
                adjusted_anchor = anchorChain.at(i)->lastPosition() + 1;
        } else {
            if (i < anchorChain.size())
                adjusted_anchor = anchorChain.at(i)->firstPosition() - 1;
        }

        f_position = positionChain.at(i-1);
    }

    // same frame, either need to adjust to cell boundaries or return
    QTextTable *table = qobject_cast<QTextTable *>(f_position);
    if (!table)
        return;

    QTextTableCell c_position = table->cellAt(position);
    QTextTableCell c_anchor = table->cellAt(adjusted_anchor);
    if (c_position != c_anchor) {
        position = c_position.firstPosition();
        if (position < adjusted_anchor)
            adjusted_anchor = c_anchor.lastPosition();
        else
            adjusted_anchor = c_anchor.firstPosition();
    }
    currentCharFormat = -1;
}

void QTextCursorPrivate::aboutToRemoveCell(int from, int to)
{
    Q_ASSERT(from <= to);
    if (position == anchor)
        return;

    QTextTable *t = qobject_cast<QTextTable *>(priv->frameAt(position));
    if (!t)
        return;
    QTextTableCell removedCellFrom = t->cellAt(from);
    QTextTableCell removedCellEnd = t->cellAt(to);
    if (! removedCellFrom.isValid() || !removedCellEnd.isValid())
        return;

    int curFrom = position;
    int curTo = adjusted_anchor;
    if (curTo < curFrom)
        qSwap(curFrom, curTo);

    QTextTableCell cellStart = t->cellAt(curFrom);
    QTextTableCell cellEnd = t->cellAt(curTo);

    if (cellStart.row() >= removedCellFrom.row() && cellEnd.row() <= removedCellEnd.row()
            && cellStart.column() >= removedCellFrom.column()
              && cellEnd.column() <= removedCellEnd.column()) { // selection is completely removed
        // find a new position, as close as possible to where we were.
        QTextTableCell cell;
        if (removedCellFrom.row() == 0 && removedCellEnd.row() == t->rows()-1) // removed n columns
            cell = t->cellAt(cellStart.row(), removedCellEnd.column()+1);
        else if (removedCellFrom.column() == 0 && removedCellEnd.column() == t->columns()-1) // removed n rows
            cell = t->cellAt(removedCellEnd.row() + 1, cellStart.column());

        int newPosition;
        if (cell.isValid())
            newPosition = cell.firstPosition();
        else
            newPosition = t->lastPosition()+1;

        setPosition(newPosition);
        anchor = newPosition;
        adjusted_anchor = newPosition;
        x = 0;
    }
    else if (cellStart.row() >= removedCellFrom.row() && cellStart.row() <= removedCellEnd.row()
        && cellEnd.row() > removedCellEnd.row()) {
        int newPosition = t->cellAt(removedCellEnd.row() + 1, cellStart.column()).firstPosition();
        if (position < anchor)
            position = newPosition;
        else
            anchor = adjusted_anchor = newPosition;
    }
    else if (cellStart.column() >= removedCellFrom.column() && cellStart.column() <= removedCellEnd.column()
        && cellEnd.column() > removedCellEnd.column()) {
        int newPosition = t->cellAt(cellStart.row(), removedCellEnd.column()+1).firstPosition();
        if (position < anchor)
            position = newPosition;
        else
            anchor = adjusted_anchor = newPosition;
    }
}

bool QTextCursorPrivate::movePosition(QTextCursor::MoveOperation op, QTextCursor::MoveMode mode)
{
    currentCharFormat = -1;
    bool adjustX = true;
    QTextBlock blockIt = block();
    bool visualMovement = priv->defaultCursorMoveStyle == Qt::VisualMoveStyle;

    if (!blockIt.isValid())
        return false;

    if (blockIt.textDirection() == Qt::RightToLeft) {
        if (op == QTextCursor::WordLeft)
            op = QTextCursor::NextWord;
        else if (op == QTextCursor::WordRight)
            op = QTextCursor::PreviousWord;

        if (!visualMovement) {
            if (op == QTextCursor::Left)
                op = QTextCursor::NextCharacter;
            else if (op == QTextCursor::Right)
                op = QTextCursor::PreviousCharacter;
        }
    }

    const QTextLayout *layout = blockLayout(blockIt);
    int relativePos = position - blockIt.position();
    QTextLine line;
    if (!priv->isInEditBlock())
        line = layout->lineForTextPosition(relativePos);

    Q_ASSERT(priv->frameAt(position) == priv->frameAt(adjusted_anchor));

    int newPosition = position;

    if (mode == QTextCursor::KeepAnchor && complexSelectionTable() != 0) {
        if ((op >= QTextCursor::EndOfLine && op <= QTextCursor::NextWord)
                || (op >= QTextCursor::Right && op <= QTextCursor::WordRight)) {
            QTextTable *t = qobject_cast<QTextTable *>(priv->frameAt(position));
            Q_ASSERT(t); // as we have already made sure we have a complex selection
            QTextTableCell cell_pos = t->cellAt(position);
            if (cell_pos.column() + cell_pos.columnSpan() != t->columns())
                op = QTextCursor::NextCell;
        }
    }

    if (x == -1 && !priv->isInEditBlock() && (op == QTextCursor::Up || op == QTextCursor::Down))
        setX();

    switch(op) {
    case QTextCursor::NoMove:
        return true;

    case QTextCursor::Start:
        newPosition = 0;
        break;
    case QTextCursor::StartOfLine: {
        newPosition = blockIt.position();
        if (line.isValid())
            newPosition += line.textStart();

        break;
    }
    case QTextCursor::StartOfBlock: {
        newPosition = blockIt.position();
        break;
    }
    case QTextCursor::PreviousBlock: {
        if (blockIt == priv->blocksBegin())
            return false;
        blockIt = blockIt.previous();

        newPosition = blockIt.position();
        break;
    }
    case QTextCursor::PreviousCharacter:
        if (mode == QTextCursor::MoveAnchor && position != adjusted_anchor)
            newPosition = qMin(position, adjusted_anchor);
        else
            newPosition = priv->previousCursorPosition(position, QTextLayout::SkipCharacters);
        break;
    case QTextCursor::Left:
        if (mode == QTextCursor::MoveAnchor && position != adjusted_anchor)
            newPosition = visualMovement ? qMax(position, adjusted_anchor)
                                         : qMin(position, adjusted_anchor);
        else
            newPosition = visualMovement ? priv->leftCursorPosition(position)
                                         : priv->previousCursorPosition(position, QTextLayout::SkipCharacters);
        break;
    case QTextCursor::StartOfWord: {
        if (relativePos == 0)
            break;

        // skip if already at word start
        QTextEngine *engine = layout->engine();
        engine->attributes();
        if ((relativePos == blockIt.length() - 1)
            && (engine->atSpace(relativePos - 1) || engine->atWordSeparator(relativePos - 1)))
            return false;

        if (relativePos < blockIt.length()-1)
            ++position;

        // FALL THROUGH!
    }
    case QTextCursor::PreviousWord:
    case QTextCursor::WordLeft:
        newPosition = priv->previousCursorPosition(position, QTextLayout::SkipWords);
        break;
    case QTextCursor::Up: {
        int i = line.lineNumber() - 1;
        if (i == -1) {
            if (blockIt == priv->blocksBegin())
                return false;
            int blockPosition = blockIt.position();
            QTextTable *table = qobject_cast<QTextTable *>(priv->frameAt(blockPosition));
            if (table) {
                QTextTableCell cell = table->cellAt(blockPosition);
                if (cell.firstPosition() == blockPosition) {
                    int row = cell.row() - 1;
                    if (row >= 0) {
                        blockPosition = table->cellAt(row, cell.column()).lastPosition();
                    } else {
                        // move to line above the table
                        blockPosition = table->firstPosition() - 1;
                    }
                    blockIt = priv->blocksFind(blockPosition);
                } else {
                    blockIt = blockIt.previous();
                }
            } else {
                blockIt = blockIt.previous();
            }
            layout = blockLayout(blockIt);
            i = layout->lineCount()-1;
        }
        if (layout->lineCount()) {
            QTextLine line = layout->lineAt(i);
            newPosition = line.xToCursor(x) + blockIt.position();
        } else {
            newPosition = blockIt.position();
        }
        adjustX = false;
        break;
    }

    case QTextCursor::End:
        newPosition = priv->length() - 1;
        break;
    case QTextCursor::EndOfLine: {
        if (!line.isValid() || line.textLength() == 0) {
            if (blockIt.length() >= 1)
                // position right before the block separator
                newPosition = blockIt.position() + blockIt.length() - 1;
            break;
        }
        newPosition = blockIt.position() + line.textStart() + line.textLength();
        if (newPosition >= priv->length())
            newPosition = priv->length() - 1;
        if (line.lineNumber() < layout->lineCount() - 1) {
            const QString text = blockIt.text();
            // ###### this relies on spaces being the cause for linebreaks.
            // this doesn't work with japanese
            if (text.at(line.textStart() + line.textLength() - 1).isSpace())
                --newPosition;
        }
        break;
    }
    case QTextCursor::EndOfWord: {
        QTextEngine *engine = layout->engine();
        engine->attributes();
        const int len = blockIt.length() - 1;
        if (relativePos >= len)
            return false;
        if (engine->atWordSeparator(relativePos)) {
            ++relativePos;
            while (relativePos < len && engine->atWordSeparator(relativePos))
                ++relativePos;
        } else {
            while (relativePos < len && !engine->atSpace(relativePos) && !engine->atWordSeparator(relativePos))
                ++relativePos;
        }
        newPosition = blockIt.position() + relativePos;
        break;
    }
    case QTextCursor::EndOfBlock:
        if (blockIt.length() >= 1)
            // position right before the block separator
            newPosition = blockIt.position() + blockIt.length() - 1;
        break;
    case QTextCursor::NextBlock: {
        blockIt = blockIt.next();
        if (!blockIt.isValid())
            return false;

        newPosition = blockIt.position();
        break;
    }
    case QTextCursor::NextCharacter:
        if (mode == QTextCursor::MoveAnchor && position != adjusted_anchor)
            newPosition = qMax(position, adjusted_anchor);
        else
            newPosition = priv->nextCursorPosition(position, QTextLayout::SkipCharacters);
        break;
    case QTextCursor::Right:
        if (mode == QTextCursor::MoveAnchor && position != adjusted_anchor)
            newPosition = visualMovement ? qMin(position, adjusted_anchor)
                                         : qMax(position, adjusted_anchor);
        else
            newPosition = visualMovement ? priv->rightCursorPosition(position)
                                         : priv->nextCursorPosition(position, QTextLayout::SkipCharacters);
        break;
    case QTextCursor::NextWord:
    case QTextCursor::WordRight:
        newPosition = priv->nextCursorPosition(position, QTextLayout::SkipWords);
        break;

    case QTextCursor::Down: {
        int i = line.lineNumber() + 1;

        if (i >= layout->lineCount()) {
            int blockPosition = blockIt.position() + blockIt.length() - 1;
            QTextTable *table = qobject_cast<QTextTable *>(priv->frameAt(blockPosition));
            if (table) {
                QTextTableCell cell = table->cellAt(blockPosition);
                if (cell.lastPosition() == blockPosition) {
                    int row = cell.row() + cell.rowSpan();
                    if (row < table->rows()) {
                        blockPosition = table->cellAt(row, cell.column()).firstPosition();
                    } else {
                        // move to line below the table
                        blockPosition = table->lastPosition() + 1;
                    }
                    blockIt = priv->blocksFind(blockPosition);
                } else {
                    blockIt = blockIt.next();
                }
            } else {
                blockIt = blockIt.next();
            }

            if (blockIt == priv->blocksEnd())
                return false;
            layout = blockLayout(blockIt);
            i = 0;
        }
        if (layout->lineCount()) {
            QTextLine line = layout->lineAt(i);
            newPosition = line.xToCursor(x) + blockIt.position();
        } else {
            newPosition = blockIt.position();
        }
        adjustX = false;
        break;
    }
    case QTextCursor::NextCell: // fall through
    case QTextCursor::PreviousCell: // fall through
    case QTextCursor::NextRow: // fall through
    case QTextCursor::PreviousRow: {
        QTextTable *table = qobject_cast<QTextTable *>(priv->frameAt(position));
        if (!table)
            return false;

        QTextTableCell cell = table->cellAt(position);
        Q_ASSERT(cell.isValid());
        int column = cell.column();
        int row = cell.row();
        const int currentRow = row;
        if (op == QTextCursor::NextCell || op == QTextCursor::NextRow) {
            do {
                column += cell.columnSpan();
                if (column >= table->columns()) {
                    column = 0;
                    ++row;
                }
                cell = table->cellAt(row, column);
                // note we also continue while we have not reached a cell thats not merged with one above us
            } while (cell.isValid()
                    && ((op == QTextCursor::NextRow && currentRow == cell.row())
                        || cell.row() < row));
        }
        else if (op == QTextCursor::PreviousCell || op == QTextCursor::PreviousRow) {
            do {
                --column;
                if (column < 0) {
                    column = table->columns()-1;
                    --row;
                }
                cell = table->cellAt(row, column);
                // note we also continue while we have not reached a cell thats not merged with one above us
            } while (cell.isValid()
                    && ((op == QTextCursor::PreviousRow && currentRow == cell.row())
                        || cell.row() < row));
        }
        if (cell.isValid())
            newPosition = cell.firstPosition();
        break;
    }
    }

    if (mode == QTextCursor::KeepAnchor) {
        QTextTable *table = qobject_cast<QTextTable *>(priv->frameAt(position));
        if (table && ((op >= QTextCursor::PreviousBlock && op <= QTextCursor::WordLeft)
                      || (op >= QTextCursor::NextBlock && op <= QTextCursor::WordRight))) {
            int oldColumn = table->cellAt(position).column();

            const QTextTableCell otherCell = table->cellAt(newPosition);
            if (!otherCell.isValid())
                return false;

            int newColumn = otherCell.column();
            if ((oldColumn > newColumn && op >= QTextCursor::End)
                || (oldColumn < newColumn && op <= QTextCursor::WordLeft))
                return false;
        }
    }

    const bool moved = setPosition(newPosition);

    if (mode == QTextCursor::MoveAnchor) {
        anchor = position;
        adjusted_anchor = position;
    } else {
        adjustCursor(op);
    }

    if (adjustX)
        setX();

    return moved;
}

QTextTable *QTextCursorPrivate::complexSelectionTable() const
{
    if (position == anchor)
        return 0;

    QTextTable *t = qobject_cast<QTextTable *>(priv->frameAt(position));
    if (t) {
        QTextTableCell cell_pos = t->cellAt(position);
        QTextTableCell cell_anchor = t->cellAt(adjusted_anchor);

        Q_ASSERT(cell_anchor.isValid());

        if (cell_pos == cell_anchor)
            t = 0;
    }
    return t;
}

void QTextCursorPrivate::selectedTableCells(int *firstRow, int *numRows, int *firstColumn, int *numColumns) const
{
    *firstRow = -1;
    *firstColumn = -1;
    *numRows = -1;
    *numColumns = -1;

    if (position == anchor)
        return;

    QTextTable *t = qobject_cast<QTextTable *>(priv->frameAt(position));
    if (!t)
        return;

    QTextTableCell cell_pos = t->cellAt(position);
    QTextTableCell cell_anchor = t->cellAt(adjusted_anchor);

    Q_ASSERT(cell_anchor.isValid());

    if (cell_pos == cell_anchor)
        return;

    *firstRow = qMin(cell_pos.row(), cell_anchor.row());
    *firstColumn = qMin(cell_pos.column(), cell_anchor.column());
    *numRows = qMax(cell_pos.row() + cell_pos.rowSpan(), cell_anchor.row() + cell_anchor.rowSpan()) - *firstRow;
    *numColumns = qMax(cell_pos.column() + cell_pos.columnSpan(), cell_anchor.column() + cell_anchor.columnSpan()) - *firstColumn;
}

static void setBlockCharFormatHelper(QTextDocumentPrivate *priv, int pos1, int pos2,
                               const QTextCharFormat &format, QTextDocumentPrivate::FormatChangeMode changeMode)
{
    QTextBlock it = priv->blocksFind(pos1);
    QTextBlock end = priv->blocksFind(pos2);
    if (end.isValid())
        end = end.next();

    for (; it != end; it = it.next()) {
        priv->setCharFormat(it.position() - 1, 1, format, changeMode);
    }
}

void QTextCursorPrivate::setBlockCharFormat(const QTextCharFormat &_format,
    QTextDocumentPrivate::FormatChangeMode changeMode)
{
    priv->beginEditBlock();

    QTextCharFormat format = _format;
    format.clearProperty(QTextFormat::ObjectIndex);

    QTextTable *table = complexSelectionTable();
    if (table) {
        int row_start, col_start, num_rows, num_cols;
        selectedTableCells(&row_start, &num_rows, &col_start, &num_cols);

        Q_ASSERT(row_start != -1);
        for (int r = row_start; r < row_start + num_rows; ++r) {
            for (int c = col_start; c < col_start + num_cols; ++c) {
                QTextTableCell cell = table->cellAt(r, c);
                int rspan = cell.rowSpan();
                int cspan = cell.columnSpan();
                if (rspan != 1) {
                    int cr = cell.row();
                    if (cr != r)
                        continue;
                }
                if (cspan != 1) {
                    int cc = cell.column();
                    if (cc != c)
                        continue;
                }

                int pos1 = cell.firstPosition();
                int pos2 = cell.lastPosition();
                setBlockCharFormatHelper(priv, pos1, pos2, format, changeMode);
            }
        }
    } else {
        int pos1 = position;
        int pos2 = adjusted_anchor;
        if (pos1 > pos2) {
            pos1 = adjusted_anchor;
            pos2 = position;
        }

        setBlockCharFormatHelper(priv, pos1, pos2, format, changeMode);
    }
    priv->endEditBlock();
}


void QTextCursorPrivate::setBlockFormat(const QTextBlockFormat &format, QTextDocumentPrivate::FormatChangeMode changeMode)
{
    QTextTable *table = complexSelectionTable();
    if (table) {
        priv->beginEditBlock();
        int row_start, col_start, num_rows, num_cols;
        selectedTableCells(&row_start, &num_rows, &col_start, &num_cols);

        Q_ASSERT(row_start != -1);
        for (int r = row_start; r < row_start + num_rows; ++r) {
            for (int c = col_start; c < col_start + num_cols; ++c) {
                QTextTableCell cell = table->cellAt(r, c);
                int rspan = cell.rowSpan();
                int cspan = cell.columnSpan();
                if (rspan != 1) {
                    int cr = cell.row();
                    if (cr != r)
                        continue;
                }
                if (cspan != 1) {
                    int cc = cell.column();
                    if (cc != c)
                        continue;
                }

                int pos1 = cell.firstPosition();
                int pos2 = cell.lastPosition();
                priv->setBlockFormat(priv->blocksFind(pos1), priv->blocksFind(pos2), format, changeMode);
            }
        }
        priv->endEditBlock();
    } else {
        int pos1 = position;
        int pos2 = adjusted_anchor;
        if (pos1 > pos2) {
            pos1 = adjusted_anchor;
            pos2 = position;
        }

        priv->setBlockFormat(priv->blocksFind(pos1), priv->blocksFind(pos2), format, changeMode);
    }
}

void QTextCursorPrivate::setCharFormat(const QTextCharFormat &_format, QTextDocumentPrivate::FormatChangeMode changeMode)
{
    Q_ASSERT(position != anchor);

    QTextCharFormat format = _format;
    format.clearProperty(QTextFormat::ObjectIndex);

    QTextTable *table = complexSelectionTable();
    if (table) {
        priv->beginEditBlock();
        int row_start, col_start, num_rows, num_cols;
        selectedTableCells(&row_start, &num_rows, &col_start, &num_cols);

        Q_ASSERT(row_start != -1);
        for (int r = row_start; r < row_start + num_rows; ++r) {
            for (int c = col_start; c < col_start + num_cols; ++c) {
                QTextTableCell cell = table->cellAt(r, c);
                int rspan = cell.rowSpan();
                int cspan = cell.columnSpan();
                if (rspan != 1) {
                    int cr = cell.row();
                    if (cr != r)
                        continue;
                }
                if (cspan != 1) {
                    int cc = cell.column();
                    if (cc != c)
                        continue;
                }

                int pos1 = cell.firstPosition();
                int pos2 = cell.lastPosition();
                priv->setCharFormat(pos1, pos2-pos1, format, changeMode);
            }
        }
        priv->endEditBlock();
    } else {
        int pos1 = position;
        int pos2 = adjusted_anchor;
        if (pos1 > pos2) {
            pos1 = adjusted_anchor;
            pos2 = position;
        }

        priv->setCharFormat(pos1, pos2-pos1, format, changeMode);
    }
}


QTextLayout *QTextCursorPrivate::blockLayout(QTextBlock &block) const{
    QTextLayout *tl = block.layout();
    if (!tl->lineCount() && priv->layout())
        priv->layout()->blockBoundingRect(block);
    return tl;
}

/*!
    \class QTextCursor
    \reentrant
    \inmodule QtGui

    \brief The QTextCursor class offers an API to access and modify QTextDocuments.

    \ingroup richtext-processing
    \ingroup shared

    Text cursors are objects that are used to access and modify the
    contents and underlying structure of text documents via a
    programming interface that mimics the behavior of a cursor in a
    text editor. QTextCursor contains information about both the
    cursor's position within a QTextDocument and any selection that it
    has made.

    QTextCursor is modeled on the way a text cursor behaves in a text
    editor, providing a programmatic means of performing standard
    actions through the user interface. A document can be thought of
    as a single string of characters. The cursor's current position()
    then is always either \e between two consecutive characters in the
    string, or else \e before the very first character or \e after the
    very last character in the string.  Documents can also contain
    tables, lists, images, and other objects in addition to text but,
    from the developer's point of view, the document can be treated as
    one long string.  Some portions of that string can be considered
    to lie within particular blocks (e.g. paragraphs), or within a
    table's cell, or a list's item, or other structural elements. When
    we refer to "current character" we mean the character immediately
    \e before the cursor position() in the document. Similarly, the
    "current block" is the block that contains the cursor position().

    A QTextCursor also has an anchor() position. The text that is
    between the anchor() and the position() is the selection. If
    anchor() == position() there is no selection.

    The cursor position can be changed programmatically using
    setPosition() and movePosition(); the latter can also be used to
    select text. For selections see selectionStart(), selectionEnd(),
    hasSelection(), clearSelection(), and removeSelectedText().

    If the position() is at the start of a block atBlockStart()
    returns \c true; and if it is at the end of a block atBlockEnd() returns
    true. The format of the current character is returned by
    charFormat(), and the format of the current block is returned by
    blockFormat().

    Formatting can be applied to the current text document using the
    setCharFormat(), mergeCharFormat(), setBlockFormat() and
    mergeBlockFormat() functions. The 'set' functions will replace the
    cursor's current character or block format, while the 'merge'
    functions add the given format properties to the cursor's current
    format. If the cursor has a selection the given format is applied
    to the current selection. Note that when only parts of a block is
    selected the block format is applied to the entire block. The text
    at the current character position can be turned into a list using
    createList().

    Deletions can be achieved using deleteChar(),
    deletePreviousChar(), and removeSelectedText().

    Text strings can be inserted into the document with the insertText()
    function, blocks (representing new paragraphs) can be inserted with
    insertBlock().

    Existing fragments of text can be inserted with insertFragment() but,
    if you want to insert pieces of text in various formats, it is usually
    still easier to use insertText() and supply a character format.

    Various types of higher-level structure can also be inserted into the
    document with the cursor:

    \list
    \li Lists are ordered sequences of block elements that are decorated with
       bullet points or symbols. These are inserted in a specified format
       with insertList().
    \li Tables are inserted with the insertTable() function, and can be
       given an optional format. These contain an array of cells that can
       be traversed using the cursor.
    \li Inline images are inserted with insertImage(). The image to be
       used can be specified in an image format, or by name.
    \li Frames are inserted by calling insertFrame() with a specified format.
    \endlist

    Actions can be grouped (i.e. treated as a single action for
    undo/redo) using beginEditBlock() and endEditBlock().

    Cursor movements are limited to valid cursor positions. In Latin
    writing this is between any two consecutive characters in the
    text, before the first character, or after the last character. In
    some other writing systems cursor movements are limited to
    "clusters" (e.g. a syllable in Devanagari, or a base letter plus
    diacritics).  Functions such as movePosition() and deleteChar()
    limit cursor movement to these valid positions.

    \sa {Rich Text Processing}

*/

/*!
    \enum QTextCursor::MoveOperation

    \value NoMove Keep the cursor where it is

    \value Start Move to the start of the document.
    \value StartOfLine Move to the start of the current line.
    \value StartOfBlock Move to the start of the current block.
    \value StartOfWord Move to the start of the current word.
    \value PreviousBlock Move to the start of the previous block.
    \value PreviousCharacter Move to the previous character.
    \value PreviousWord Move to the beginning of the previous word.
    \value Up Move up one line.
    \value Left Move left one character.
    \value WordLeft Move left one word.

    \value End Move to the end of the document.
    \value EndOfLine Move to the end of the current line.
    \value EndOfWord Move to the end of the current word.
    \value EndOfBlock Move to the end of the current block.
    \value NextBlock Move to the beginning of the next block.
    \value NextCharacter Move to the next character.
    \value NextWord Move to the next word.
    \value Down Move down one line.
    \value Right Move right one character.
    \value WordRight Move right one word.

    \value NextCell  Move to the beginning of the next table cell inside the
           current table. If the current cell is the last cell in the row, the
           cursor will move to the first cell in the next row.
    \value PreviousCell  Move to the beginning of the previous table cell
           inside the current table. If the current cell is the first cell in
           the row, the cursor will move to the last cell in the previous row.
    \value NextRow  Move to the first new cell of the next row in the current
           table.
    \value PreviousRow  Move to the last cell of the previous row in the
           current table.

    \sa movePosition()
*/

/*!
    \enum QTextCursor::MoveMode

    \value MoveAnchor Moves the anchor to the same position as the cursor itself.
    \value KeepAnchor Keeps the anchor where it is.

    If the anchor() is kept where it is and the position() is moved,
    the text in between will be selected.
*/

/*!
    \enum QTextCursor::SelectionType

    This enum describes the types of selection that can be applied with the
    select() function.

    \value Document         Selects the entire document.
    \value BlockUnderCursor Selects the block of text under the cursor.
    \value LineUnderCursor  Selects the line of text under the cursor.
    \value WordUnderCursor  Selects the word under the cursor. If the cursor
           is not positioned within a string of selectable characters, no
           text is selected.
*/

/*!
    Constructs a null cursor.
 */
QTextCursor::QTextCursor()
    : d(0)
{
}

/*!
    Constructs a cursor pointing to the beginning of the \a document.
 */
QTextCursor::QTextCursor(QTextDocument *document)
    : d(new QTextCursorPrivate(document->docHandle()))
{
}

/*!
    Constructs a cursor pointing to the beginning of the \a frame.
*/
QTextCursor::QTextCursor(QTextFrame *frame)
    : d(new QTextCursorPrivate(frame->document()->docHandle()))
{
    d->adjusted_anchor = d->anchor = d->position = frame->firstPosition();
}


/*!
    Constructs a cursor pointing to the beginning of the \a block.
*/
QTextCursor::QTextCursor(const QTextBlock &block)
    : d(new QTextCursorPrivate(block.docHandle()))
{
    d->adjusted_anchor = d->anchor = d->position = block.position();
}


/*!
  \internal
 */
QTextCursor::QTextCursor(QTextDocumentPrivate *p, int pos)
    : d(new QTextCursorPrivate(p))
{
    d->adjusted_anchor = d->anchor = d->position = pos;

    d->setX();
}

/*!
    \internal
*/
QTextCursor::QTextCursor(QTextCursorPrivate *d)
{
    Q_ASSERT(d);
    this->d = d;
}

/*!
    Constructs a new cursor that is a copy of \a cursor.
 */
QTextCursor::QTextCursor(const QTextCursor &cursor)
{
    d = cursor.d;
}

/*!
    Makes a copy of \a cursor and assigns it to this QTextCursor. Note
    that QTextCursor is an \l{Implicitly Shared Classes}{implicitly
    shared} class.

 */
QTextCursor &QTextCursor::operator=(const QTextCursor &cursor)
{
    d = cursor.d;
    return *this;
}

/*!
    \fn void QTextCursor::swap(QTextCursor &other)
    \since 5.0

    Swaps this text cursor instance with \a other. This function is
    very fast and never fails.
*/

/*!
    Destroys the QTextCursor.
 */
QTextCursor::~QTextCursor()
{
}

/*!
    Returns \c true if the cursor is null; otherwise returns \c false. A null
    cursor is created by the default constructor.
 */
bool QTextCursor::isNull() const
{
    return !d || !d->priv;
}

/*!
    Moves the cursor to the absolute position in the document specified by
    \a pos using a \c MoveMode specified by \a m. The cursor is positioned
    between characters.

    \sa position(), movePosition(), anchor()
*/
void QTextCursor::setPosition(int pos, MoveMode m)
{
    if (!d || !d->priv)
        return;

    if (pos < 0 || pos >= d->priv->length()) {
        qWarning("QTextCursor::setPosition: Position '%d' out of range", pos);
        return;
    }

    d->setPosition(pos);
    if (m == MoveAnchor) {
        d->anchor = pos;
        d->adjusted_anchor = pos;
    } else { // keep anchor
        QTextCursor::MoveOperation op;
        if (pos < d->anchor)
            op = QTextCursor::Left;
        else
            op = QTextCursor::Right;
        d->adjustCursor(op);
    }
    d->setX();
}

/*!
    Returns the absolute position of the cursor within the document.
    The cursor is positioned between characters.

    \sa setPosition(), movePosition(), anchor(), positionInBlock()
*/
int QTextCursor::position() const
{
    if (!d || !d->priv)
        return -1;
    return d->position;
}

/*!
    \since 4.7
    Returns the relative position of the cursor within the block.
    The cursor is positioned between characters.

    This is equivalent to \c{ position() - block().position()}.

    \sa position()
*/
int QTextCursor::positionInBlock() const
{
    if (!d || !d->priv)
        return 0;
    return d->position - d->block().position();
}

/*!
    Returns the anchor position; this is the same as position() unless
    there is a selection in which case position() marks one end of the
    selection and anchor() marks the other end. Just like the cursor
    position, the anchor position is between characters.

    \sa position(), setPosition(), movePosition(), selectionStart(), selectionEnd()
*/
int QTextCursor::anchor() const
{
    if (!d || !d->priv)
        return -1;
    return d->anchor;
}

/*!
    \fn bool QTextCursor::movePosition(MoveOperation operation, MoveMode mode, int n)

    Moves the cursor by performing the given \a operation \a n times, using the specified
    \a mode, and returns \c true if all operations were completed successfully; otherwise
    returns \c false.

    For example, if this function is repeatedly used to seek to the end of the next
    word, it will eventually fail when the end of the document is reached.

    By default, the move operation is performed once (\a n = 1).

    If \a mode is \c KeepAnchor, the cursor selects the text it moves
    over. This is the same effect that the user achieves when they
    hold down the Shift key and move the cursor with the cursor keys.

    \sa setVisualNavigation()
*/
bool QTextCursor::movePosition(MoveOperation op, MoveMode mode, int n)
{
    if (!d || !d->priv)
        return false;
    switch (op) {
    case Start:
    case StartOfLine:
    case End:
    case EndOfLine:
        n = 1;
        break;
    default: break;
    }

    int previousPosition = d->position;
    for (; n > 0; --n) {
        if (!d->movePosition(op, mode))
            return false;
    }

    if (d->visualNavigation && !d->block().isVisible()) {
        QTextBlock b = d->block();
        if (previousPosition < d->position) {
            while (!b.next().isVisible())
                b = b.next();
            d->setPosition(b.position() + b.length() - 1);
        } else {
            while (!b.previous().isVisible())
                b = b.previous();
            d->setPosition(b.position());
        }
        if (mode == QTextCursor::MoveAnchor)
            d->anchor = d->position;
        while (d->movePosition(op, mode)
               && !d->block().isVisible())
            ;

    }
    return true;
}

/*!
  \since 4.4

  Returns \c true if the cursor does visual navigation; otherwise
  returns \c false.

  Visual navigation means skipping over hidden text paragraphs. The
  default is false.

  \sa setVisualNavigation(), movePosition()
 */
bool QTextCursor::visualNavigation() const
{
    return d ? d->visualNavigation : false;
}

/*!
  \since 4.4

  Sets visual navigation to \a b.

  Visual navigation means skipping over hidden text paragraphs. The
  default is false.

  \sa visualNavigation(), movePosition()
 */
void QTextCursor::setVisualNavigation(bool b)
{
    if (d)
        d->visualNavigation = b;
}


/*!
  \since 4.7

  Sets the visual x position for vertical cursor movements to \a x.

  The vertical movement x position is cleared automatically when the cursor moves horizontally, and kept
  unchanged when the cursor moves vertically. The mechanism allows the cursor to move up and down on a
  visually straight line with proportional fonts, and to gently "jump" over short lines.

  A value of -1 indicates no predefined x position. It will then be set automatically the next time the
  cursor moves up or down.

  \sa verticalMovementX()
  */
void QTextCursor::setVerticalMovementX(int x)
{
    if (d)
        d->x = x;
}

/*! \since 4.7

  Returns the visual x position for vertical cursor movements.

  A value of -1 indicates no predefined x position. It will then be set automatically the next time the
  cursor moves up or down.

  \sa setVerticalMovementX()
  */
int QTextCursor::verticalMovementX() const
{
    return d ? d->x : -1;
}

/*!
  \since 4.7

  Returns whether the cursor should keep its current position when text gets inserted at the position of the
  cursor.

  The default is false;

  \sa setKeepPositionOnInsert()
 */
bool QTextCursor::keepPositionOnInsert() const
{
    return d ? d->keepPositionOnInsert : false;
}

/*!
  \since 4.7

  Defines whether the cursor should keep its current position when text gets inserted at the current position of the
  cursor.

  If \a b is true, the cursor keeps its current position when text gets inserted at the positing of the cursor.
  If \a b is false, the cursor moves along with the inserted text.

  The default is false.

  Note that a cursor always moves when text is inserted before the current position of the cursor, and it
  always keeps its position when text is inserted after the current position of the cursor.

  \sa keepPositionOnInsert()
 */
void QTextCursor::setKeepPositionOnInsert(bool b)
{
    if (d)
        d->keepPositionOnInsert = b;
}



/*!
    Inserts \a text at the current position, using the current
    character format.

    If there is a selection, the selection is deleted and replaced by
    \a text, for example:
    \snippet code/src_gui_text_qtextcursor.cpp 0
    This clears any existing selection, selects the word at the cursor
    (i.e. from position() forward), and replaces the selection with
    the phrase "Hello World".

    Any ASCII linefeed characters (\\n) in the inserted text are transformed
    into unicode block separators, corresponding to insertBlock() calls.

    \sa charFormat(), hasSelection()
*/
void QTextCursor::insertText(const QString &text)
{
    QTextCharFormat fmt = charFormat();
    fmt.clearProperty(QTextFormat::ObjectType);
    insertText(text, fmt);
}

/*!
    \fn void QTextCursor::insertText(const QString &text, const QTextCharFormat &format)
    \overload

    Inserts \a text at the current position with the given \a format.
*/
void QTextCursor::insertText(const QString &text, const QTextCharFormat &_format)
{
    if (!d || !d->priv)
        return;

    Q_ASSERT(_format.isValid());

    QTextCharFormat format = _format;
    format.clearProperty(QTextFormat::ObjectIndex);

    bool hasEditBlock = false;

    if (d->anchor != d->position) {
        hasEditBlock = true;
        d->priv->beginEditBlock();
        d->remove();
    }

    if (!text.isEmpty()) {
        QTextFormatCollection *formats = d->priv->formatCollection();
        int formatIdx = formats->indexForFormat(format);
        Q_ASSERT(formats->format(formatIdx).isCharFormat());

        QTextBlockFormat blockFmt = blockFormat();


        int textStart = d->priv->text.length();
        int blockStart = 0;
        d->priv->text += text;
        int textEnd = d->priv->text.length();

        for (int i = 0; i < text.length(); ++i) {
            QChar ch = text.at(i);

            const int blockEnd = i;

            if (ch == QLatin1Char('\r')
                && (i + 1) < text.length()
                && text.at(i + 1) == QLatin1Char('\n')) {
                ++i;
                ch = text.at(i);
            }

            if (ch == QLatin1Char('\n')
                || ch == QChar::ParagraphSeparator
                || ch == QTextBeginningOfFrame
                || ch == QTextEndOfFrame
                || ch == QLatin1Char('\r')) {

                if (!hasEditBlock) {
                    hasEditBlock = true;
                    d->priv->beginEditBlock();
                }

                if (blockEnd > blockStart)
                    d->priv->insert(d->position, textStart + blockStart, blockEnd - blockStart, formatIdx);

                d->insertBlock(blockFmt, format);
                blockStart = i + 1;
            }
        }
        if (textStart + blockStart < textEnd)
            d->priv->insert(d->position, textStart + blockStart, textEnd - textStart - blockStart, formatIdx);
    }
    if (hasEditBlock)
        d->priv->endEditBlock();
    d->setX();
}

/*!
    If there is no selected text, deletes the character \e at the
    current cursor position; otherwise deletes the selected text.

    \sa deletePreviousChar(), hasSelection(), clearSelection()
*/
void QTextCursor::deleteChar()
{
    if (!d || !d->priv)
        return;

    if (d->position != d->anchor) {
        removeSelectedText();
        return;
    }

    if (!d->canDelete(d->position))
        return;
    d->adjusted_anchor = d->anchor =
                         d->priv->nextCursorPosition(d->anchor, QTextLayout::SkipCharacters);
    d->remove();
    d->setX();
}

/*!
    If there is no selected text, deletes the character \e before the
    current cursor position; otherwise deletes the selected text.

    \sa deleteChar(), hasSelection(), clearSelection()
*/
void QTextCursor::deletePreviousChar()
{
    if (!d || !d->priv)
        return;

    if (d->position != d->anchor) {
        removeSelectedText();
        return;
    }

    if (d->anchor < 1 || !d->canDelete(d->anchor-1))
        return;
    d->anchor--;

    QTextDocumentPrivate::FragmentIterator fragIt = d->priv->find(d->anchor);
    const QTextFragmentData * const frag = fragIt.value();
    int fpos = fragIt.position();
    QChar uc = d->priv->buffer().at(d->anchor - fpos + frag->stringPosition);
    if (d->anchor > fpos && uc.isLowSurrogate()) {
        // second half of a surrogate, check if we have the first half as well,
        // if yes delete both at once
        uc = d->priv->buffer().at(d->anchor - 1 - fpos + frag->stringPosition);
        if (uc.isHighSurrogate())
            --d->anchor;
    }

    d->adjusted_anchor = d->anchor;
    d->remove();
    d->setX();
}

/*!
    Selects text in the document according to the given \a selection.
*/
void QTextCursor::select(SelectionType selection)
{
    if (!d || !d->priv)
        return;

    clearSelection();

    const QTextBlock block = d->block();

    switch (selection) {
        case LineUnderCursor:
            movePosition(StartOfLine);
            movePosition(EndOfLine, KeepAnchor);
            break;
        case WordUnderCursor:
            movePosition(StartOfWord);
            movePosition(EndOfWord, KeepAnchor);
            break;
        case BlockUnderCursor:
            if (block.length() == 1) // no content
                break;
            movePosition(StartOfBlock);
            // also select the paragraph separator
            if (movePosition(PreviousBlock)) {
                movePosition(EndOfBlock);
                movePosition(NextBlock, KeepAnchor);
            }
            movePosition(EndOfBlock, KeepAnchor);
            break;
        case Document:
            movePosition(Start);
            movePosition(End, KeepAnchor);
            break;
    }
}

/*!
    Returns \c true if the cursor contains a selection; otherwise returns \c false.
*/
bool QTextCursor::hasSelection() const
{
    return !!d && d->position != d->anchor;
}


/*!
    Returns \c true if the cursor contains a selection that is not simply a
    range from selectionStart() to selectionEnd(); otherwise returns \c false.

    Complex selections are ones that span at least two cells in a table;
    their extent is specified by selectedTableCells().
*/
bool QTextCursor::hasComplexSelection() const
{
    if (!d)
        return false;

    return d->complexSelectionTable() != 0;
}

/*!
    If the selection spans over table cells, \a firstRow is populated
    with the number of the first row in the selection, \a firstColumn
    with the number of the first column in the selection, and \a
    numRows and \a numColumns with the number of rows and columns in
    the selection. If the selection does not span any table cells the
    results are harmless but undefined.
*/
void QTextCursor::selectedTableCells(int *firstRow, int *numRows, int *firstColumn, int *numColumns) const
{
    *firstRow = -1;
    *firstColumn = -1;
    *numRows = -1;
    *numColumns = -1;

    if (!d || d->position == d->anchor)
        return;

    d->selectedTableCells(firstRow, numRows, firstColumn, numColumns);
}


/*!
    Clears the current selection by setting the anchor to the cursor position.

    Note that it does \b{not} delete the text of the selection.

    \sa removeSelectedText(), hasSelection()
*/
void QTextCursor::clearSelection()
{
    if (!d)
        return;
    d->adjusted_anchor = d->anchor = d->position;
    d->currentCharFormat = -1;
}

/*!
    If there is a selection, its content is deleted; otherwise does
    nothing.

    \sa hasSelection()
*/
void QTextCursor::removeSelectedText()
{
    if (!d || !d->priv || d->position == d->anchor)
        return;

    d->priv->beginEditBlock();
    d->remove();
    d->priv->endEditBlock();
    d->setX();
}

/*!
    Returns the start of the selection or position() if the
    cursor doesn't have a selection.

    \sa selectionEnd(), position(), anchor()
*/
int QTextCursor::selectionStart() const
{
    if (!d || !d->priv)
        return -1;
    return qMin(d->position, d->adjusted_anchor);
}

/*!
    Returns the end of the selection or position() if the cursor
    doesn't have a selection.

    \sa selectionStart(), position(), anchor()
*/
int QTextCursor::selectionEnd() const
{
    if (!d || !d->priv)
        return -1;
    return qMax(d->position, d->adjusted_anchor);
}

static void getText(QString &text, QTextDocumentPrivate *priv, const QString &docText, int pos, int end)
{
    while (pos < end) {
        QTextDocumentPrivate::FragmentIterator fragIt = priv->find(pos);
        const QTextFragmentData * const frag = fragIt.value();

        const int offsetInFragment = qMax(0, pos - fragIt.position());
        const int len = qMin(int(frag->size_array[0] - offsetInFragment), end - pos);

        text += QString(docText.constData() + frag->stringPosition + offsetInFragment, len);
        pos += len;
    }
}

/*!
    Returns the current selection's text (which may be empty). This
    only returns the text, with no rich text formatting information.
    If you want a document fragment (i.e. formatted rich text) use
    selection() instead.

    \note If the selection obtained from an editor spans a line break,
    the text will contain a Unicode U+2029 paragraph separator character
    instead of a newline \c{\n} character. Use QString::replace() to
    replace these characters with newlines.
*/
QString QTextCursor::selectedText() const
{
    if (!d || !d->priv || d->position == d->anchor)
        return QString();

    const QString docText = d->priv->buffer();
    QString text;

    QTextTable *table = d->complexSelectionTable();
    if (table) {
        int row_start, col_start, num_rows, num_cols;
        selectedTableCells(&row_start, &num_rows, &col_start, &num_cols);

        Q_ASSERT(row_start != -1);
        for (int r = row_start; r < row_start + num_rows; ++r) {
            for (int c = col_start; c < col_start + num_cols; ++c) {
                QTextTableCell cell = table->cellAt(r, c);
                int rspan = cell.rowSpan();
                int cspan = cell.columnSpan();
                if (rspan != 1) {
                    int cr = cell.row();
                    if (cr != r)
                        continue;
                }
                if (cspan != 1) {
                    int cc = cell.column();
                    if (cc != c)
                        continue;
                }

                getText(text, d->priv, docText, cell.firstPosition(), cell.lastPosition());
            }
        }
    } else {
        getText(text, d->priv, docText, selectionStart(), selectionEnd());
    }

    return text;
}

/*!
    Returns the current selection (which may be empty) with all its
    formatting information. If you just want the selected text (i.e.
    plain text) use selectedText() instead.

    \note Unlike QTextDocumentFragment::toPlainText(),
    selectedText() may include special unicode characters such as
    QChar::ParagraphSeparator.

    \sa QTextDocumentFragment::toPlainText()
*/
QTextDocumentFragment QTextCursor::selection() const
{
    return QTextDocumentFragment(*this);
}

/*!
    Returns the block that contains the cursor.
*/
QTextBlock QTextCursor::block() const
{
    if (!d || !d->priv)
        return QTextBlock();
    return d->block();
}

/*!
    Returns the block format of the block the cursor is in.

    \sa setBlockFormat(), charFormat()
 */
QTextBlockFormat QTextCursor::blockFormat() const
{
    if (!d || !d->priv)
        return QTextBlockFormat();

    return d->block().blockFormat();
}

/*!
    Sets the block format of the current block (or all blocks that
    are contained in the selection) to \a format.

    \sa blockFormat(), mergeBlockFormat()
*/
void QTextCursor::setBlockFormat(const QTextBlockFormat &format)
{
    if (!d || !d->priv)
        return;

    d->setBlockFormat(format, QTextDocumentPrivate::SetFormat);
}

/*!
    Modifies the block format of the current block (or all blocks that
    are contained in the selection) with the block format specified by
    \a modifier.

    \sa setBlockFormat(), blockFormat()
*/
void QTextCursor::mergeBlockFormat(const QTextBlockFormat &modifier)
{
    if (!d || !d->priv)
        return;

    d->setBlockFormat(modifier, QTextDocumentPrivate::MergeFormat);
}

/*!
    Returns the block character format of the block the cursor is in.

    The block char format is the format used when inserting text at the
    beginning of an empty block.

    \sa setBlockCharFormat()
 */
QTextCharFormat QTextCursor::blockCharFormat() const
{
    if (!d || !d->priv)
        return QTextCharFormat();

    return d->block().charFormat();
}

/*!
    Sets the block char format of the current block (or all blocks that
    are contained in the selection) to \a format.

    \sa blockCharFormat()
*/
void QTextCursor::setBlockCharFormat(const QTextCharFormat &format)
{
    if (!d || !d->priv)
        return;

    d->setBlockCharFormat(format, QTextDocumentPrivate::SetFormatAndPreserveObjectIndices);
}

/*!
    Modifies the block char format of the current block (or all blocks that
    are contained in the selection) with the block format specified by
    \a modifier.

    \sa setBlockCharFormat()
*/
void QTextCursor::mergeBlockCharFormat(const QTextCharFormat &modifier)
{
    if (!d || !d->priv)
        return;

    d->setBlockCharFormat(modifier, QTextDocumentPrivate::MergeFormat);
}

/*!
    Returns the format of the character immediately before the cursor
    position(). If the cursor is positioned at the beginning of a text
    block that is not empty then the format of the character
    immediately after the cursor is returned.

    \sa insertText(), blockFormat()
 */
QTextCharFormat QTextCursor::charFormat() const
{
    if (!d || !d->priv)
        return QTextCharFormat();

    int idx = d->currentCharFormat;
    if (idx == -1) {
        QTextBlock block = d->block();

        int pos;
        if (d->position == block.position()
            && block.length() > 1)
            pos = d->position;
        else
            pos = d->position - 1;

        if (pos == -1) {
            idx = d->priv->blockCharFormatIndex(d->priv->blockMap().firstNode());
        } else {
            Q_ASSERT(pos >= 0 && pos < d->priv->length());

            QTextDocumentPrivate::FragmentIterator it = d->priv->find(pos);
            Q_ASSERT(!it.atEnd());
            idx = it.value()->format;
        }
    }

    QTextCharFormat cfmt = d->priv->formatCollection()->charFormat(idx);
    cfmt.clearProperty(QTextFormat::ObjectIndex);

    Q_ASSERT(cfmt.isValid());
    return cfmt;
}

/*!
    Sets the cursor's current character format to the given \a
    format. If the cursor has a selection, the given \a format is
    applied to the current selection.

    \sa hasSelection(), mergeCharFormat()
*/
void QTextCursor::setCharFormat(const QTextCharFormat &format)
{
    if (!d || !d->priv)
        return;
    if (d->position == d->anchor) {
        d->currentCharFormat = d->priv->formatCollection()->indexForFormat(format);
        return;
    }
    d->setCharFormat(format, QTextDocumentPrivate::SetFormatAndPreserveObjectIndices);
}

/*!
    Merges the cursor's current character format with the properties
    described by format \a modifier. If the cursor has a selection,
    this function applies all the properties set in \a modifier to all
    the character formats that are part of the selection.

    \sa hasSelection(), setCharFormat()
*/
void QTextCursor::mergeCharFormat(const QTextCharFormat &modifier)
{
    if (!d || !d->priv)
        return;
    if (d->position == d->anchor) {
        QTextCharFormat format = charFormat();
        format.merge(modifier);
        d->currentCharFormat = d->priv->formatCollection()->indexForFormat(format);
        return;
    }

    d->setCharFormat(modifier, QTextDocumentPrivate::MergeFormat);
}

/*!
    Returns \c true if the cursor is at the start of a block; otherwise
    returns \c false.

    \sa atBlockEnd(), atStart()
*/
bool QTextCursor::atBlockStart() const
{
    if (!d || !d->priv)
        return false;

    return d->position == d->block().position();
}

/*!
    Returns \c true if the cursor is at the end of a block; otherwise
    returns \c false.

    \sa atBlockStart(), atEnd()
*/
bool QTextCursor::atBlockEnd() const
{
    if (!d || !d->priv)
        return false;

    return d->position == d->block().position() + d->block().length() - 1;
}

/*!
    Returns \c true if the cursor is at the start of the document;
    otherwise returns \c false.

    \sa atBlockStart(), atEnd()
*/
bool QTextCursor::atStart() const
{
    if (!d || !d->priv)
        return false;

    return d->position == 0;
}

/*!
    \since 4.6

    Returns \c true if the cursor is at the end of the document;
    otherwise returns \c false.

    \sa atStart(), atBlockEnd()
*/
bool QTextCursor::atEnd() const
{
    if (!d || !d->priv)
        return false;

    return d->position == d->priv->length() - 1;
}

/*!
    Inserts a new empty block at the cursor position() with the
    current blockFormat() and charFormat().

    \sa setBlockFormat()
*/
void QTextCursor::insertBlock()
{
    insertBlock(blockFormat());
}

/*!
    \overload

    Inserts a new empty block at the cursor position() with block
    format \a format and the current charFormat() as block char format.

    \sa setBlockFormat()
*/
void QTextCursor::insertBlock(const QTextBlockFormat &format)
{
    QTextCharFormat charFmt = charFormat();
    charFmt.clearProperty(QTextFormat::ObjectType);
    insertBlock(format, charFmt);
}

/*!
    \fn void QTextCursor::insertBlock(const QTextBlockFormat &format, const QTextCharFormat &charFormat)
    \overload

    Inserts a new empty block at the cursor position() with block
    format \a format and \a charFormat as block char format.

    \sa setBlockFormat()
*/
void QTextCursor::insertBlock(const QTextBlockFormat &format, const QTextCharFormat &_charFormat)
{
    if (!d || !d->priv)
        return;

    QTextCharFormat charFormat = _charFormat;
    charFormat.clearProperty(QTextFormat::ObjectIndex);

    d->priv->beginEditBlock();
    d->remove();
    d->insertBlock(format, charFormat);
    d->priv->endEditBlock();
    d->setX();
}

/*!
    Inserts a new block at the current position and makes it the first
    list item of a newly created list with the given \a format. Returns
    the created list.

    \sa currentList(), createList(), insertBlock()
 */
QTextList *QTextCursor::insertList(const QTextListFormat &format)
{
    insertBlock();
    return createList(format);
}

/*!
    \overload

    Inserts a new block at the current position and makes it the first
    list item of a newly created list with the given \a style. Returns
    the created list.

    \sa currentList(), createList(), insertBlock()
 */
QTextList *QTextCursor::insertList(QTextListFormat::Style style)
{
    insertBlock();
    return createList(style);
}

/*!
    Creates and returns a new list with the given \a format, and makes the
    current paragraph the cursor is in the first list item.

    \sa insertList(), currentList()
 */
QTextList *QTextCursor::createList(const QTextListFormat &format)
{
    if (!d || !d->priv)
        return 0;

    QTextList *list = static_cast<QTextList *>(d->priv->createObject(format));
    QTextBlockFormat modifier;
    modifier.setObjectIndex(list->objectIndex());
    mergeBlockFormat(modifier);
    return list;
}

/*!
    \overload

    Creates and returns a new list with the given \a style, making the
    cursor's current paragraph the first list item.

    The style to be used is defined by the QTextListFormat::Style enum.

    \sa insertList(), currentList()
 */
QTextList *QTextCursor::createList(QTextListFormat::Style style)
{
    QTextListFormat fmt;
    fmt.setStyle(style);
    return createList(fmt);
}

/*!
    Returns the current list if the cursor position() is inside a
    block that is part of a list; otherwise returns 0.

    \sa insertList(), createList()
 */
QTextList *QTextCursor::currentList() const
{
    if (!d || !d->priv)
        return 0;

    QTextBlockFormat b = blockFormat();
    QTextObject *o = d->priv->objectForFormat(b);
    return qobject_cast<QTextList *>(o);
}

/*!
    \fn QTextTable *QTextCursor::insertTable(int rows, int columns)

    \overload

    Creates a new table with the given number of \a rows and \a columns,
    inserts it at the current cursor position() in the document, and returns
    the table object. The cursor is moved to the beginning of the first cell.

    There must be at least one row and one column in the table.

    \sa currentTable()
 */
QTextTable *QTextCursor::insertTable(int rows, int cols)
{
    return insertTable(rows, cols, QTextTableFormat());
}

/*!
    \fn QTextTable *QTextCursor::insertTable(int rows, int columns, const QTextTableFormat &format)

    Creates a new table with the given number of \a rows and \a columns
    in the specified \a format, inserts it at the current cursor position()
    in the document, and returns the table object. The cursor is moved to
    the beginning of the first cell.

    There must be at least one row and one column in the table.

    \sa currentTable()
*/
QTextTable *QTextCursor::insertTable(int rows, int cols, const QTextTableFormat &format)
{
    if(!d || !d->priv || rows == 0 || cols == 0)
        return 0;

    int pos = d->position;
    QTextTable *t = QTextTablePrivate::createTable(d->priv, d->position, rows, cols, format);
    d->setPosition(pos+1);
    // ##### what should we do if we have a selection?
    d->anchor = d->position;
    d->adjusted_anchor = d->anchor;
    return t;
}

/*!
    Returns a pointer to the current table if the cursor position()
    is inside a block that is part of a table; otherwise returns 0.

    \sa insertTable()
*/
QTextTable *QTextCursor::currentTable() const
{
    if(!d || !d->priv)
        return 0;

    QTextFrame *frame = d->priv->frameAt(d->position);
    while (frame) {
        QTextTable *table = qobject_cast<QTextTable *>(frame);
        if (table)
            return table;
        frame = frame->parentFrame();
    }
    return 0;
}

/*!
    Inserts a frame with the given \a format at the current cursor position(),
    moves the cursor position() inside the frame, and returns the frame.

    If the cursor holds a selection, the whole selection is moved inside the
    frame.

    \sa hasSelection()
*/
QTextFrame *QTextCursor::insertFrame(const QTextFrameFormat &format)
{
    if (!d || !d->priv)
        return 0;

    return d->priv->insertFrame(selectionStart(), selectionEnd(), format);
}

/*!
    Returns a pointer to the current frame. Returns 0 if the cursor is invalid.

    \sa insertFrame()
*/
QTextFrame *QTextCursor::currentFrame() const
{
    if(!d || !d->priv)
        return 0;

    return d->priv->frameAt(d->position);
}


/*!
    Inserts the text \a fragment at the current position().
*/
void QTextCursor::insertFragment(const QTextDocumentFragment &fragment)
{
    if (!d || !d->priv || fragment.isEmpty())
        return;

    d->priv->beginEditBlock();
    d->remove();
    fragment.d->insert(*this);
    d->priv->endEditBlock();

    if (fragment.d && fragment.d->doc)
        d->priv->mergeCachedResources(fragment.d->doc->docHandle());
}

/*!
    \since 4.2
    Inserts the text \a html at the current position(). The text is interpreted as
    HTML.

    \note When using this function with a style sheet, the style sheet will
    only apply to the current block in the document. In order to apply a style
    sheet throughout a document, use QTextDocument::setDefaultStyleSheet()
    instead.
*/

#ifndef QT_NO_TEXTHTMLPARSER

void QTextCursor::insertHtml(const QString &html)
{
    if (!d || !d->priv)
        return;
    QTextDocumentFragment fragment = QTextDocumentFragment::fromHtml(html, d->priv->document());
    insertFragment(fragment);
}

#endif // QT_NO_TEXTHTMLPARSER

/*!
    \overload
    \since 4.2

    Inserts the image defined by the given \a format at the cursor's current position
    with the specified \a alignment.

    \sa position()
*/
void QTextCursor::insertImage(const QTextImageFormat &format, QTextFrameFormat::Position alignment)
{
    if (!d || !d->priv)
        return;

    QTextFrameFormat ffmt;
    ffmt.setPosition(alignment);
    QTextObject *obj = d->priv->createObject(ffmt);

    QTextImageFormat fmt = format;
    fmt.setObjectIndex(obj->objectIndex());

    d->priv->beginEditBlock();
    d->remove();
    const int idx = d->priv->formatCollection()->indexForFormat(fmt);
    d->priv->insert(d->position, QString(QChar(QChar::ObjectReplacementCharacter)), idx);
    d->priv->endEditBlock();
}

/*!
    Inserts the image defined by \a format at the current position().
*/
void QTextCursor::insertImage(const QTextImageFormat &format)
{
    insertText(QString(QChar::ObjectReplacementCharacter), format);
}

/*!
    \overload

    Convenience method for inserting the image with the given \a name at the
    current position().

    \snippet code/src_gui_text_qtextcursor.cpp 1
*/
void QTextCursor::insertImage(const QString &name)
{
    QTextImageFormat format;
    format.setName(name);
    insertImage(format);
}

/*!
    \since 4.5
    \overload

    Convenience function for inserting the given \a image with an optional
    \a name at the current position().
*/
void QTextCursor::insertImage(const QImage &image, const QString &name)
{
    if (image.isNull()) {
        qWarning("QTextCursor::insertImage: attempt to add an invalid image");
        return;
    }
    QString imageName = name;
    if (name.isEmpty())
        imageName = QString::number(image.cacheKey());
    d->priv->document()->addResource(QTextDocument::ImageResource, QUrl(imageName), image);
    QTextImageFormat format;
    format.setName(imageName);
    insertImage(format);
}

/*!
    \fn bool QTextCursor::operator!=(const QTextCursor &other) const

    Returns \c true if the \a other cursor is at a different position in
    the document as this cursor; otherwise returns \c false.
*/
bool QTextCursor::operator!=(const QTextCursor &rhs) const
{
    return !operator==(rhs);
}

/*!
    \fn bool QTextCursor::operator<(const QTextCursor &other) const

    Returns \c true if the \a other cursor is positioned later in the
    document than this cursor; otherwise returns \c false.
*/
bool QTextCursor::operator<(const QTextCursor &rhs) const
{
    if (!d)
        return !!rhs.d;

    if (!rhs.d)
        return false;

    Q_ASSERT_X(d->priv == rhs.d->priv, "QTextCursor::operator<", "cannot compare cursors attached to different documents");

    return d->position < rhs.d->position;
}

/*!
    \fn bool QTextCursor::operator<=(const QTextCursor &other) const

    Returns \c true if the \a other cursor is positioned later or at the
    same position in the document as this cursor; otherwise returns
    false.
*/
bool QTextCursor::operator<=(const QTextCursor &rhs) const
{
    if (!d)
        return true;

    if (!rhs.d)
        return false;

    Q_ASSERT_X(d->priv == rhs.d->priv, "QTextCursor::operator<=", "cannot compare cursors attached to different documents");

    return d->position <= rhs.d->position;
}

/*!
    \fn bool QTextCursor::operator==(const QTextCursor &other) const

    Returns \c true if the \a other cursor is at the same position in the
    document as this cursor; otherwise returns \c false.
*/
bool QTextCursor::operator==(const QTextCursor &rhs) const
{
    if (!d)
        return !rhs.d;

    if (!rhs.d)
        return false;

    return d->position == rhs.d->position && d->priv == rhs.d->priv;
}

/*!
    \fn bool QTextCursor::operator>=(const QTextCursor &other) const

    Returns \c true if the \a other cursor is positioned earlier or at the
    same position in the document as this cursor; otherwise returns
    false.
*/
bool QTextCursor::operator>=(const QTextCursor &rhs) const
{
    if (!d)
        return false;

    if (!rhs.d)
        return true;

    Q_ASSERT_X(d->priv == rhs.d->priv, "QTextCursor::operator>=", "cannot compare cursors attached to different documents");

    return d->position >= rhs.d->position;
}

/*!
    \fn bool QTextCursor::operator>(const QTextCursor &other) const

    Returns \c true if the \a other cursor is positioned earlier in the
    document than this cursor; otherwise returns \c false.
*/
bool QTextCursor::operator>(const QTextCursor &rhs) const
{
    if (!d)
        return false;

    if (!rhs.d)
        return true;

    Q_ASSERT_X(d->priv == rhs.d->priv, "QTextCursor::operator>", "cannot compare cursors attached to different documents");

    return d->position > rhs.d->position;
}

/*!
    Indicates the start of a block of editing operations on the
    document that should appear as a single operation from an
    undo/redo point of view.

    For example:

    \snippet code/src_gui_text_qtextcursor.cpp 2

    The call to undo() will cause both insertions to be undone,
    causing both "World" and "Hello" to be removed.

    It is possible to nest calls to beginEditBlock and endEditBlock. The
    top-most pair will determine the scope of the undo/redo operation.

    \sa endEditBlock()
 */
void QTextCursor::beginEditBlock()
{
    if (!d || !d->priv)
        return;

    if (d->priv->editBlock == 0) // we are the initial edit block, store current cursor position for undo
        d->priv->editBlockCursorPosition = d->position;

    d->priv->beginEditBlock();
}

/*!
    Like beginEditBlock() indicates the start of a block of editing operations
    that should appear as a single operation for undo/redo. However unlike
    beginEditBlock() it does not start a new block but reverses the previous call to
    endEditBlock() and therefore makes following operations part of the previous edit block created.

    For example:

    \snippet code/src_gui_text_qtextcursor.cpp 3

    The call to undo() will cause all three insertions to be undone.

    \sa beginEditBlock(), endEditBlock()
 */
void QTextCursor::joinPreviousEditBlock()
{
    if (!d || !d->priv)
        return;

    d->priv->joinPreviousEditBlock();
}

/*!
    Indicates the end of a block of editing operations on the document
    that should appear as a single operation from an undo/redo point
    of view.

    \sa beginEditBlock()
 */

void QTextCursor::endEditBlock()
{
    if (!d || !d->priv)
        return;

    d->priv->endEditBlock();
}

/*!
    Returns \c true if this cursor and \a other are copies of each other, i.e.
    one of them was created as a copy of the other and neither has moved since.
    This is much stricter than equality.

    \sa operator=(), operator==()
*/
bool QTextCursor::isCopyOf(const QTextCursor &other) const
{
    return d == other.d;
}

/*!
    \since 4.2
    Returns the number of the block the cursor is in, or 0 if the cursor is invalid.

    Note that this function only makes sense in documents without complex objects such
    as tables or frames.
*/
int QTextCursor::blockNumber() const
{
    if (!d || !d->priv)
        return 0;

    return d->block().blockNumber();
}


/*!
    \since 4.2
    Returns the position of the cursor within its containing line.

    Note that this is the column number relative to a wrapped line,
    not relative to the block (i.e. the paragraph).

    You probably want to call positionInBlock() instead.

    \sa positionInBlock()
*/
int QTextCursor::columnNumber() const
{
    if (!d || !d->priv)
        return 0;

    QTextBlock block = d->block();
    if (!block.isValid())
        return 0;

    const QTextLayout *layout = d->blockLayout(block);

    const int relativePos = d->position - block.position();

    if (layout->lineCount() == 0)
        return relativePos;

    QTextLine line = layout->lineForTextPosition(relativePos);
    if (!line.isValid())
        return 0;
    return relativePos - line.textStart();
}

/*!
    \since 4.5
    Returns the document this cursor is associated with.
*/
QTextDocument *QTextCursor::document() const
{
    if (d->priv)
        return d->priv->document();
    return 0; // document went away
}

QT_END_NAMESPACE
