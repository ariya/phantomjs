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

#include "qtextdocumentlayout_p.h"
#include "qtextdocument_p.h"
#include "qtextimagehandler_p.h"
#include "qtexttable.h"
#include "qtextlist.h"
#include "qtextengine_p.h"
#include "private/qcssutil_p.h"

#include "qabstracttextdocumentlayout_p.h"
#include "qcssparser_p.h"

#include <qpainter.h>
#include <qmath.h>
#include <qrect.h>
#include <qpalette.h>
#include <qdebug.h>
#include <qvarlengtharray.h>
#include <limits.h>
#include <qstyle.h>
#include <qbasictimer.h>
#include "private/qfunctions_p.h"

// #define LAYOUT_DEBUG

#ifdef LAYOUT_DEBUG
#define LDEBUG qDebug()
#define INC_INDENT debug_indent += "  "
#define DEC_INDENT debug_indent = debug_indent.left(debug_indent.length()-2)
#else
#define LDEBUG if(0) qDebug()
#define INC_INDENT do {} while(0)
#define DEC_INDENT do {} while(0)
#endif

QT_BEGIN_NAMESPACE

// ################ should probably add frameFormatChange notification!

struct QTextLayoutStruct;

class QTextFrameData : public QTextFrameLayoutData
{
public:
    QTextFrameData();

    // relative to parent frame
    QFixedPoint position;
    QFixedSize size;

    // contents starts at (margin+border/margin+border)
    QFixed topMargin;
    QFixed bottomMargin;
    QFixed leftMargin;
    QFixed rightMargin;
    QFixed border;
    QFixed padding;
    // contents width includes padding (as we need to treat this on a per cell basis for tables)
    QFixed contentsWidth;
    QFixed contentsHeight;
    QFixed oldContentsWidth;

    // accumulated margins
    QFixed effectiveTopMargin;
    QFixed effectiveBottomMargin;

    QFixed minimumWidth;
    QFixed maximumWidth;

    QTextLayoutStruct *currentLayoutStruct;

    bool sizeDirty;
    bool layoutDirty;

    QList<QPointer<QTextFrame> > floats;
};

QTextFrameData::QTextFrameData()
    : maximumWidth(QFIXED_MAX),
      currentLayoutStruct(0), sizeDirty(true), layoutDirty(true)
{
}

struct QTextLayoutStruct {
    QTextLayoutStruct() : maximumWidth(QFIXED_MAX), fullLayout(false)
    {}
    QTextFrame *frame;
    QFixed x_left;
    QFixed x_right;
    QFixed frameY; // absolute y position of the current frame
    QFixed y; // always relative to the current frame
    QFixed contentsWidth;
    QFixed minimumWidth;
    QFixed maximumWidth;
    bool fullLayout;
    QList<QTextFrame *> pendingFloats;
    QFixed pageHeight;
    QFixed pageBottom;
    QFixed pageTopMargin;
    QFixed pageBottomMargin;
    QRectF updateRect;
    QRectF updateRectForFloats;

    inline void addUpdateRectForFloat(const QRectF &rect) {
        if (updateRectForFloats.isValid())
            updateRectForFloats |= rect;
        else
            updateRectForFloats = rect;
    }

    inline QFixed absoluteY() const
    { return frameY + y; }

    inline int currentPage() const
    { return pageHeight == 0 ? 0 : (absoluteY() / pageHeight).truncate(); }

    inline void newPage()
    { if (pageHeight == QFIXED_MAX) return; pageBottom += pageHeight; y = pageBottom - pageHeight + pageBottomMargin + pageTopMargin - frameY; }
};

class QTextTableData : public QTextFrameData
{
public:
    QFixed cellSpacing, cellPadding;
    qreal deviceScale;
    QVector<QFixed> minWidths;
    QVector<QFixed> maxWidths;
    QVector<QFixed> widths;
    QVector<QFixed> heights;
    QVector<QFixed> columnPositions;
    QVector<QFixed> rowPositions;

    QVector<QFixed> cellVerticalOffsets;

    QFixed headerHeight;

    // maps from cell index (row + col * rowCount) to child frames belonging to
    // the specific cell
    QMultiHash<int, QTextFrame *> childFrameMap;

    inline QFixed cellWidth(int column, int colspan) const
    { return columnPositions.at(column + colspan - 1) + widths.at(column + colspan - 1)
             - columnPositions.at(column); }

    inline void calcRowPosition(int row)
    {
        if (row > 0)
            rowPositions[row] = rowPositions.at(row - 1) + heights.at(row - 1) + border + cellSpacing + border;
    }

    QRectF cellRect(const QTextTableCell &cell) const;

    inline QFixed paddingProperty(const QTextFormat &format, QTextFormat::Property property) const
    {
        QVariant v = format.property(property);
        if (v.isNull()) {
            return cellPadding;
        } else {
            Q_ASSERT(v.userType() == QVariant::Double || v.userType() == QMetaType::Float);
            return QFixed::fromReal(v.toReal() * deviceScale);
        }
    }

    inline QFixed topPadding(const QTextFormat &format) const
    {
        return paddingProperty(format, QTextFormat::TableCellTopPadding);
    }

    inline QFixed bottomPadding(const QTextFormat &format) const
    {
        return paddingProperty(format, QTextFormat::TableCellBottomPadding);
    }

    inline QFixed leftPadding(const QTextFormat &format) const
    {
        return paddingProperty(format, QTextFormat::TableCellLeftPadding);
    }

    inline QFixed rightPadding(const QTextFormat &format) const
    {
        return paddingProperty(format, QTextFormat::TableCellRightPadding);
    }

    inline QFixedPoint cellPosition(const QTextTableCell &cell) const
    {
        const QTextFormat fmt = cell.format();
        return cellPosition(cell.row(), cell.column()) + QFixedPoint(leftPadding(fmt), topPadding(fmt));
    }

    void updateTableSize();

private:
    inline QFixedPoint cellPosition(int row, int col) const
    { return QFixedPoint(columnPositions.at(col), rowPositions.at(row) + cellVerticalOffsets.at(col + row * widths.size())); }
};

static QTextFrameData *createData(QTextFrame *f)
{
    QTextFrameData *data;
    if (qobject_cast<QTextTable *>(f))
        data = new QTextTableData;
    else
        data = new QTextFrameData;
    f->setLayoutData(data);
    return data;
}

static inline QTextFrameData *data(QTextFrame *f)
{
    QTextFrameData *data = static_cast<QTextFrameData *>(f->layoutData());
    if (!data)
        data = createData(f);
    return data;
}

static bool isFrameFromInlineObject(QTextFrame *f)
{
    return f->firstPosition() > f->lastPosition();
}

void QTextTableData::updateTableSize()
{
    const QFixed effectiveTopMargin = this->topMargin + border + padding;
    const QFixed effectiveBottomMargin = this->bottomMargin + border + padding;
    const QFixed effectiveLeftMargin = this->leftMargin + border + padding;
    const QFixed effectiveRightMargin = this->rightMargin + border + padding;
    size.height = contentsHeight == -1
                   ? rowPositions.last() + heights.last() + padding + border + cellSpacing + effectiveBottomMargin
                   : effectiveTopMargin + contentsHeight + effectiveBottomMargin;
    size.width = effectiveLeftMargin + contentsWidth + effectiveRightMargin;
}

QRectF QTextTableData::cellRect(const QTextTableCell &cell) const
{
    const int row = cell.row();
    const int rowSpan = cell.rowSpan();
    const int column = cell.column();
    const int colSpan = cell.columnSpan();

    return QRectF(columnPositions.at(column).toReal(),
                  rowPositions.at(row).toReal(),
                  (columnPositions.at(column + colSpan - 1) + widths.at(column + colSpan - 1) - columnPositions.at(column)).toReal(),
                  (rowPositions.at(row + rowSpan - 1) + heights.at(row + rowSpan - 1) - rowPositions.at(row)).toReal());
}

static inline bool isEmptyBlockBeforeTable(const QTextBlock &block, const QTextBlockFormat &format, const QTextFrame::Iterator &nextIt)
{
    return !nextIt.atEnd()
           && qobject_cast<QTextTable *>(nextIt.currentFrame())
           && block.isValid()
           && block.length() == 1
           && !format.hasProperty(QTextFormat::BlockTrailingHorizontalRulerWidth)
           && !format.hasProperty(QTextFormat::BackgroundBrush)
           && nextIt.currentFrame()->firstPosition() == block.position() + 1
           ;
}

static inline bool isEmptyBlockBeforeTable(QTextFrame::Iterator it)
{
    QTextFrame::Iterator next = it; ++next;
    if (it.currentFrame())
        return false;
    QTextBlock block = it.currentBlock();
    return isEmptyBlockBeforeTable(block, block.blockFormat(), next);
}

static inline bool isEmptyBlockAfterTable(const QTextBlock &block, const QTextFrame *previousFrame)
{
    return qobject_cast<const QTextTable *>(previousFrame)
           && block.isValid()
           && block.length() == 1
           && previousFrame->lastPosition() == block.position() - 1
           ;
}

static inline bool isLineSeparatorBlockAfterTable(const QTextBlock &block, const QTextFrame *previousFrame)
{
    return qobject_cast<const QTextTable *>(previousFrame)
           && block.isValid()
           && block.length() > 1
           && block.text().at(0) == QChar::LineSeparator
           && previousFrame->lastPosition() == block.position() - 1
           ;
}

/*

Optimization strategies:

HTML layout:

* Distinguish between normal and special flow. For normal flow the condition:
  y1 > y2 holds for all blocks with b1.key() > b2.key().
* Special flow is: floats, table cells

* Normal flow within table cells. Tables (not cells) are part of the normal flow.


* If blocks grows/shrinks in height and extends over whole page width at the end, move following blocks.
* If height doesn't change, no need to do anything

Table cells:

* If minWidth of cell changes, recalculate table width, relayout if needed.
* What about maxWidth when doing auto layout?

Floats:
* need fixed or proportional width, otherwise don't float!
* On width/height change relayout surrounding paragraphs.

Document width change:
* full relayout needed


Float handling:

* Floats are specified by a special format object.
* currently only floating images are implemented.

*/

/*

   On the table layouting:

   +---[ table border ]-------------------------
   |      [ cell spacing ]
   |  +------[ cell border ]-----+  +--------
   |  |                          |  |
   |  |
   |  |
   |  |
   |

   rowPositions[i] and columnPositions[i] point at the cell content
   position. So for example the left border is drawn at
   x = columnPositions[i] - fd->border and similar for y.

*/

struct QCheckPoint
{
    QFixed y;
    QFixed frameY; // absolute y position of the current frame
    int positionInFrame;
    QFixed minimumWidth;
    QFixed maximumWidth;
    QFixed contentsWidth;
};
Q_DECLARE_TYPEINFO(QCheckPoint, Q_PRIMITIVE_TYPE);

Q_STATIC_GLOBAL_OPERATOR bool operator<(const QCheckPoint &checkPoint, QFixed y)
{
    return checkPoint.y < y;
}

Q_STATIC_GLOBAL_OPERATOR bool operator<(const QCheckPoint &checkPoint, int pos)
{
    return checkPoint.positionInFrame < pos;
}

static void fillBackground(QPainter *p, const QRectF &rect, QBrush brush, const QPointF &origin, QRectF gradientRect = QRectF())
{
    p->save();
    if (brush.style() >= Qt::LinearGradientPattern && brush.style() <= Qt::ConicalGradientPattern) {
        if (!gradientRect.isNull()) {
            QTransform m;
            m.translate(gradientRect.left(), gradientRect.top());
            m.scale(gradientRect.width(), gradientRect.height());
            brush.setTransform(m);
            const_cast<QGradient *>(brush.gradient())->setCoordinateMode(QGradient::LogicalMode);
        }
    } else {
        p->setBrushOrigin(origin);
    }
    p->fillRect(rect, brush);
    p->restore();
}

class QTextDocumentLayoutPrivate : public QAbstractTextDocumentLayoutPrivate
{
    Q_DECLARE_PUBLIC(QTextDocumentLayout)
public:
    QTextDocumentLayoutPrivate();

    QTextOption::WrapMode wordWrapMode;
#ifdef LAYOUT_DEBUG
    mutable QString debug_indent;
#endif

    int fixedColumnWidth;
    int cursorWidth;

    QSizeF lastReportedSize;
    QRectF viewportRect;
    QRectF clipRect;

    mutable int currentLazyLayoutPosition;
    mutable int lazyLayoutStepSize;
    QBasicTimer layoutTimer;
    mutable QBasicTimer sizeChangedTimer;
    uint showLayoutProgress : 1;
    uint insideDocumentChange : 1;

    int lastPageCount;
    qreal idealWidth;
    bool contentHasAlignment;

    QFixed blockIndent(const QTextBlockFormat &blockFormat) const;

    void drawFrame(const QPointF &offset, QPainter *painter, const QAbstractTextDocumentLayout::PaintContext &context,
                   QTextFrame *f) const;
    void drawFlow(const QPointF &offset, QPainter *painter, const QAbstractTextDocumentLayout::PaintContext &context,
                  QTextFrame::Iterator it, const QList<QTextFrame *> &floats, QTextBlock *cursorBlockNeedingRepaint) const;
    void drawBlock(const QPointF &offset, QPainter *painter, const QAbstractTextDocumentLayout::PaintContext &context,
                   QTextBlock bl, bool inRootFrame) const;
    void drawListItem(const QPointF &offset, QPainter *painter, const QAbstractTextDocumentLayout::PaintContext &context,
                      QTextBlock bl, const QTextCharFormat *selectionFormat) const;
    void drawTableCell(const QRectF &cellRect, QPainter *painter, const QAbstractTextDocumentLayout::PaintContext &cell_context,
                       QTextTable *table, QTextTableData *td, int r, int c,
                       QTextBlock *cursorBlockNeedingRepaint, QPointF *cursorBlockOffset) const;
    void drawBorder(QPainter *painter, const QRectF &rect, qreal topMargin, qreal bottomMargin, qreal border,
                    const QBrush &brush, QTextFrameFormat::BorderStyle style) const;
    void drawFrameDecoration(QPainter *painter, QTextFrame *frame, QTextFrameData *fd, const QRectF &clip, const QRectF &rect) const;

    enum HitPoint {
        PointBefore,
        PointAfter,
        PointInside,
        PointExact
    };
    HitPoint hitTest(QTextFrame *frame, const QFixedPoint &point, int *position, QTextLayout **l, Qt::HitTestAccuracy accuracy) const;
    HitPoint hitTest(QTextFrame::Iterator it, HitPoint hit, const QFixedPoint &p,
                     int *position, QTextLayout **l, Qt::HitTestAccuracy accuracy) const;
    HitPoint hitTest(QTextTable *table, const QFixedPoint &point, int *position, QTextLayout **l, Qt::HitTestAccuracy accuracy) const;
    HitPoint hitTest(QTextBlock bl, const QFixedPoint &point, int *position, QTextLayout **l, Qt::HitTestAccuracy accuracy) const;

    QTextLayoutStruct layoutCell(QTextTable *t, const QTextTableCell &cell, QFixed width,
                                 int layoutFrom, int layoutTo, QTextTableData *tableData, QFixed absoluteTableY,
                                 bool withPageBreaks);
    void setCellPosition(QTextTable *t, const QTextTableCell &cell, const QPointF &pos);
    QRectF layoutTable(QTextTable *t, int layoutFrom, int layoutTo, QFixed parentY);

    void positionFloat(QTextFrame *frame, QTextLine *currentLine = 0);

    // calls the next one
    QRectF layoutFrame(QTextFrame *f, int layoutFrom, int layoutTo, QFixed parentY = 0);
    QRectF layoutFrame(QTextFrame *f, int layoutFrom, int layoutTo, QFixed frameWidth, QFixed frameHeight, QFixed parentY = 0);

    void layoutBlock(const QTextBlock &bl, int blockPosition, const QTextBlockFormat &blockFormat,
                     QTextLayoutStruct *layoutStruct, int layoutFrom, int layoutTo, const QTextBlockFormat *previousBlockFormat);
    void layoutFlow(QTextFrame::Iterator it, QTextLayoutStruct *layoutStruct, int layoutFrom, int layoutTo, QFixed width = 0);
    void pageBreakInsideTable(QTextTable *table, QTextLayoutStruct *layoutStruct);


    void floatMargins(const QFixed &y, const QTextLayoutStruct *layoutStruct, QFixed *left, QFixed *right) const;
    QFixed findY(QFixed yFrom, const QTextLayoutStruct *layoutStruct, QFixed requiredWidth) const;

    QVector<QCheckPoint> checkPoints;

    QTextFrame::Iterator frameIteratorForYPosition(QFixed y) const;
    QTextFrame::Iterator frameIteratorForTextPosition(int position) const;

    void ensureLayouted(QFixed y) const;
    void ensureLayoutedByPosition(int position) const;
    inline void ensureLayoutFinished() const
    { ensureLayoutedByPosition(INT_MAX); }
    void layoutStep() const;

    QRectF frameBoundingRectInternal(QTextFrame *frame) const;

    qreal scaleToDevice(qreal value) const;
    QFixed scaleToDevice(QFixed value) const;
};

QTextDocumentLayoutPrivate::QTextDocumentLayoutPrivate()
    : fixedColumnWidth(-1),
      cursorWidth(1),
      currentLazyLayoutPosition(-1),
      lazyLayoutStepSize(1000),
      lastPageCount(-1)
{
    showLayoutProgress = true;
    insideDocumentChange = false;
    idealWidth = 0;
    contentHasAlignment = false;
}

QTextFrame::Iterator QTextDocumentLayoutPrivate::frameIteratorForYPosition(QFixed y) const
{
    QTextFrame *rootFrame = document->rootFrame();

    if (checkPoints.isEmpty()
        || y < 0 || y > data(rootFrame)->size.height)
        return rootFrame->begin();

    QVector<QCheckPoint>::ConstIterator checkPoint = qLowerBound(checkPoints.begin(), checkPoints.end(), y);
    if (checkPoint == checkPoints.end())
        return rootFrame->begin();

    if (checkPoint != checkPoints.begin())
        --checkPoint;

    const int position = rootFrame->firstPosition() + checkPoint->positionInFrame;
    return frameIteratorForTextPosition(position);
}

QTextFrame::Iterator QTextDocumentLayoutPrivate::frameIteratorForTextPosition(int position) const
{
    QTextFrame *rootFrame = docPrivate->rootFrame();

    const QTextDocumentPrivate::BlockMap &map = docPrivate->blockMap();
    const int begin = map.findNode(rootFrame->firstPosition());
    const int end = map.findNode(rootFrame->lastPosition()+1);

    const int block = map.findNode(position);
    const int blockPos = map.position(block);

    QTextFrame::iterator it(rootFrame, block, begin, end);

    QTextFrame *containingFrame = docPrivate->frameAt(blockPos);
    if (containingFrame != rootFrame) {
        while (containingFrame->parentFrame() != rootFrame) {
            containingFrame = containingFrame->parentFrame();
            Q_ASSERT(containingFrame);
        }

        it.cf = containingFrame;
        it.cb = 0;
    }

    return it;
}

QTextDocumentLayoutPrivate::HitPoint
QTextDocumentLayoutPrivate::hitTest(QTextFrame *frame, const QFixedPoint &point, int *position, QTextLayout **l, Qt::HitTestAccuracy accuracy) const
{
    QTextFrameData *fd = data(frame);
    // #########
    if (fd->layoutDirty)
        return PointAfter;
    Q_ASSERT(!fd->layoutDirty);
    Q_ASSERT(!fd->sizeDirty);
    const QFixedPoint relativePoint(point.x - fd->position.x, point.y - fd->position.y);

    QTextFrame *rootFrame = docPrivate->rootFrame();

//     LDEBUG << "checking frame" << frame->firstPosition() << "point=" << point
//            << "position" << fd->position << "size" << fd->size;
    if (frame != rootFrame) {
        if (relativePoint.y < 0 || relativePoint.x < 0) {
            *position = frame->firstPosition() - 1;
//             LDEBUG << "before pos=" << *position;
            return PointBefore;
        } else if (relativePoint.y > fd->size.height || relativePoint.x > fd->size.width) {
            *position = frame->lastPosition() + 1;
//             LDEBUG << "after pos=" << *position;
            return PointAfter;
        }
    }

    if (isFrameFromInlineObject(frame)) {
        *position = frame->firstPosition() - 1;
        return PointExact;
    }

    if (QTextTable *table = qobject_cast<QTextTable *>(frame)) {
        const int rows = table->rows();
        const int columns = table->columns();
        QTextTableData *td = static_cast<QTextTableData *>(data(table));

        if (!td->childFrameMap.isEmpty()) {
            for (int r = 0; r < rows; ++r) {
                for (int c = 0; c < columns; ++c) {
                    QTextTableCell cell = table->cellAt(r, c);
                    if (cell.row() != r || cell.column() != c)
                        continue;

                    QRectF cellRect = td->cellRect(cell);
                    const QFixedPoint cellPos = QFixedPoint::fromPointF(cellRect.topLeft());
                    const QFixedPoint pointInCell = relativePoint - cellPos;

                    const QList<QTextFrame *> childFrames = td->childFrameMap.values(r + c * rows);
                    for (int i = 0; i < childFrames.size(); ++i) {
                        QTextFrame *child = childFrames.at(i);
                        if (isFrameFromInlineObject(child)
                            && child->frameFormat().position() != QTextFrameFormat::InFlow
                            && hitTest(child, pointInCell, position, l, accuracy) == PointExact)
                        {
                            return PointExact;
                        }
                    }
                }
            }
        }

        return hitTest(table, relativePoint, position, l, accuracy);
    }

    const QList<QTextFrame *> childFrames = frame->childFrames();
    for (int i = 0; i < childFrames.size(); ++i) {
        QTextFrame *child = childFrames.at(i);
        if (isFrameFromInlineObject(child)
            && child->frameFormat().position() != QTextFrameFormat::InFlow
            && hitTest(child, relativePoint, position, l, accuracy) == PointExact)
        {
            return PointExact;
        }
    }

    QTextFrame::Iterator it = frame->begin();

    if (frame == rootFrame) {
        it = frameIteratorForYPosition(relativePoint.y);

        Q_ASSERT(it.parentFrame() == frame);
    }

    if (it.currentFrame())
        *position = it.currentFrame()->firstPosition();
    else
        *position = it.currentBlock().position();

    return hitTest(it, PointBefore, relativePoint, position, l, accuracy);
}

QTextDocumentLayoutPrivate::HitPoint
QTextDocumentLayoutPrivate::hitTest(QTextFrame::Iterator it, HitPoint hit, const QFixedPoint &p,
                                    int *position, QTextLayout **l, Qt::HitTestAccuracy accuracy) const
{
    INC_INDENT;

    for (; !it.atEnd(); ++it) {
        QTextFrame *c = it.currentFrame();
        HitPoint hp;
        int pos = -1;
        if (c) {
            hp = hitTest(c, p, &pos, l, accuracy);
        } else {
            hp = hitTest(it.currentBlock(), p, &pos, l, accuracy);
        }
        if (hp >= PointInside) {
            if (isEmptyBlockBeforeTable(it))
                continue;
            hit = hp;
            *position = pos;
            break;
        }
        if (hp == PointBefore && pos < *position) {
            *position = pos;
            hit = hp;
        } else if (hp == PointAfter && pos > *position) {
            *position = pos;
            hit = hp;
        }
    }

    DEC_INDENT;
//     LDEBUG << "inside=" << hit << " pos=" << *position;
    return hit;
}

QTextDocumentLayoutPrivate::HitPoint
QTextDocumentLayoutPrivate::hitTest(QTextTable *table, const QFixedPoint &point,
                                    int *position, QTextLayout **l, Qt::HitTestAccuracy accuracy) const
{
    QTextTableData *td = static_cast<QTextTableData *>(data(table));

    QVector<QFixed>::ConstIterator rowIt = qLowerBound(td->rowPositions.constBegin(), td->rowPositions.constEnd(), point.y);
    if (rowIt == td->rowPositions.constEnd()) {
        rowIt = td->rowPositions.constEnd() - 1;
    } else if (rowIt != td->rowPositions.constBegin()) {
        --rowIt;
    }

    QVector<QFixed>::ConstIterator colIt = qLowerBound(td->columnPositions.constBegin(), td->columnPositions.constEnd(), point.x);
    if (colIt == td->columnPositions.constEnd()) {
        colIt = td->columnPositions.constEnd() - 1;
    } else if (colIt != td->columnPositions.constBegin()) {
        --colIt;
    }

    QTextTableCell cell = table->cellAt(rowIt - td->rowPositions.constBegin(),
                                        colIt - td->columnPositions.constBegin());
    if (!cell.isValid())
        return PointBefore;

    *position = cell.firstPosition();

    HitPoint hp = hitTest(cell.begin(), PointInside, point - td->cellPosition(cell), position, l, accuracy);

    if (hp == PointExact)
        return hp;
    if (hp == PointAfter)
        *position = cell.lastPosition();
    return PointInside;
}

QTextDocumentLayoutPrivate::HitPoint
QTextDocumentLayoutPrivate::hitTest(QTextBlock bl, const QFixedPoint &point, int *position, QTextLayout **l,
                                    Qt::HitTestAccuracy accuracy) const
{
    QTextLayout *tl = bl.layout();
    QRectF textrect = tl->boundingRect();
    textrect.translate(tl->position());
//     LDEBUG << "    checking block" << bl.position() << "point=" << point
//            << "    tlrect" << textrect;
    *position = bl.position();
    if (point.y.toReal() < textrect.top()) {
//             LDEBUG << "    before pos=" << *position;
        return PointBefore;
    } else if (point.y.toReal() > textrect.bottom()) {
        *position += bl.length();
//             LDEBUG << "    after pos=" << *position;
        return PointAfter;
    }

    QPointF pos = point.toPointF() - tl->position();

    // ### rtl?

    HitPoint hit = PointInside;
    *l = tl;
    int off = 0;
    for (int i = 0; i < tl->lineCount(); ++i) {
        QTextLine line = tl->lineAt(i);
        const QRectF lr = line.naturalTextRect();
        if (lr.top() > pos.y()) {
            off = qMin(off, line.textStart());
        } else if (lr.bottom() <= pos.y()) {
            off = qMax(off, line.textStart() + line.textLength());
        } else {
            if (lr.left() <= pos.x() && lr.right() >= pos.x())
                hit = PointExact;
            // when trying to hit an anchor we want it to hit not only in the left
            // half
            if (accuracy == Qt::ExactHit)
                off = line.xToCursor(pos.x(), QTextLine::CursorOnCharacter);
            else
                off = line.xToCursor(pos.x(), QTextLine::CursorBetweenCharacters);
            break;
        }
    }
    *position += off;

//     LDEBUG << "    inside=" << hit << " pos=" << *position;
    return hit;
}

// ### could be moved to QTextBlock
QFixed QTextDocumentLayoutPrivate::blockIndent(const QTextBlockFormat &blockFormat) const
{
    qreal indent = blockFormat.indent();

    QTextObject *object = document->objectForFormat(blockFormat);
    if (object)
        indent += object->format().toListFormat().indent();

    if (qIsNull(indent))
        return 0;

    qreal scale = 1;
    if (paintDevice) {
        scale = qreal(paintDevice->logicalDpiY()) / qreal(qt_defaultDpi());
    }

    return QFixed::fromReal(indent * scale * document->indentWidth());
}

void QTextDocumentLayoutPrivate::drawBorder(QPainter *painter, const QRectF &rect, qreal topMargin, qreal bottomMargin,
                                            qreal border, const QBrush &brush, QTextFrameFormat::BorderStyle style) const
{
    const qreal pageHeight = document->pageSize().height();
    const int topPage = pageHeight > 0 ? static_cast<int>(rect.top() / pageHeight) : 0;
    const int bottomPage = pageHeight > 0 ? static_cast<int>((rect.bottom() + border) / pageHeight) : 0;

#ifndef QT_NO_CSSPARSER
    QCss::BorderStyle cssStyle = static_cast<QCss::BorderStyle>(style + 1);
#endif //QT_NO_CSSPARSER

    bool turn_off_antialiasing = !(painter->renderHints() & QPainter::Antialiasing);
    painter->setRenderHint(QPainter::Antialiasing);

    for (int i = topPage; i <= bottomPage; ++i) {
        QRectF clipped = rect.toRect();

        if (topPage != bottomPage) {
            clipped.setTop(qMax(clipped.top(), i * pageHeight + topMargin - border));
            clipped.setBottom(qMin(clipped.bottom(), (i + 1) * pageHeight - bottomMargin));

            if (clipped.bottom() <= clipped.top())
                continue;
        }
#ifndef QT_NO_CSSPARSER
        qDrawEdge(painter, clipped.left(), clipped.top(), clipped.left() + border, clipped.bottom() + border, 0, 0, QCss::LeftEdge, cssStyle, brush);
        qDrawEdge(painter, clipped.left() + border, clipped.top(), clipped.right() + border, clipped.top() + border, 0, 0, QCss::TopEdge, cssStyle, brush);
        qDrawEdge(painter, clipped.right(), clipped.top() + border, clipped.right() + border, clipped.bottom(), 0, 0, QCss::RightEdge, cssStyle, brush);
        qDrawEdge(painter, clipped.left() + border, clipped.bottom(), clipped.right() + border, clipped.bottom() + border, 0, 0, QCss::BottomEdge, cssStyle, brush);
#else
        painter->save();
        painter->setPen(Qt::NoPen);
        painter->setBrush(brush);
        painter->drawRect(QRectF(clipped.left(), clipped.top(), clipped.left() + border, clipped.bottom() + border));
        painter->drawRect(QRectF(clipped.left() + border, clipped.top(), clipped.right() + border, clipped.top() + border));
        painter->drawRect(QRectF(clipped.right(), clipped.top() + border, clipped.right() + border, clipped.bottom()));
        painter->drawRect(QRectF(clipped.left() + border, clipped.bottom(), clipped.right() + border, clipped.bottom() + border));
        painter->restore();
#endif //QT_NO_CSSPARSER
    }
    if (turn_off_antialiasing)
        painter->setRenderHint(QPainter::Antialiasing, false);
}

void QTextDocumentLayoutPrivate::drawFrameDecoration(QPainter *painter, QTextFrame *frame, QTextFrameData *fd, const QRectF &clip, const QRectF &rect) const
{

    const QBrush bg = frame->frameFormat().background();
    if (bg != Qt::NoBrush) {
        QRectF bgRect = rect;
        bgRect.adjust((fd->leftMargin + fd->border).toReal(),
                      (fd->topMargin + fd->border).toReal(),
                      - (fd->rightMargin + fd->border).toReal(),
                      - (fd->bottomMargin + fd->border).toReal());

        QRectF gradientRect; // invalid makes it default to bgRect
        QPointF origin = bgRect.topLeft();
        if (!frame->parentFrame()) {
            bgRect = clip;
            gradientRect.setWidth(painter->device()->width());
            gradientRect.setHeight(painter->device()->height());
        }
        fillBackground(painter, bgRect, bg, origin, gradientRect);
    }
    if (fd->border != 0) {
        painter->save();
        painter->setBrush(Qt::lightGray);
        painter->setPen(Qt::NoPen);

        const qreal leftEdge = rect.left() + fd->leftMargin.toReal();
        const qreal border = fd->border.toReal();
        const qreal topMargin = fd->topMargin.toReal();
        const qreal leftMargin = fd->leftMargin.toReal();
        const qreal bottomMargin = fd->bottomMargin.toReal();
        const qreal rightMargin = fd->rightMargin.toReal();
        const qreal w = rect.width() - 2 * border - leftMargin - rightMargin;
        const qreal h = rect.height() - 2 * border - topMargin - bottomMargin;

        drawBorder(painter, QRectF(leftEdge, rect.top() + topMargin, w + border, h + border),
                   fd->effectiveTopMargin.toReal(), fd->effectiveBottomMargin.toReal(),
                   border, frame->frameFormat().borderBrush(), frame->frameFormat().borderStyle());

        painter->restore();
    }
}

static void adjustContextSelectionsForCell(QAbstractTextDocumentLayout::PaintContext &cell_context,
                                           const QTextTableCell &cell,
                                           int r, int c,
                                           const int *selectedTableCells)
{
    for (int i = 0; i < cell_context.selections.size(); ++i) {
        int row_start = selectedTableCells[i * 4];
        int col_start = selectedTableCells[i * 4 + 1];
        int num_rows = selectedTableCells[i * 4 + 2];
        int num_cols = selectedTableCells[i * 4 + 3];

        if (row_start != -1) {
            if (r >= row_start && r < row_start + num_rows
                && c >= col_start && c < col_start + num_cols)
            {
                int firstPosition = cell.firstPosition();
                int lastPosition = cell.lastPosition();

                // make sure empty cells are still selected
                if (firstPosition == lastPosition)
                    ++lastPosition;

                cell_context.selections[i].cursor.setPosition(firstPosition);
                cell_context.selections[i].cursor.setPosition(lastPosition, QTextCursor::KeepAnchor);
            } else {
                cell_context.selections[i].cursor.clearSelection();
            }
        }

        // FullWidthSelection is not useful for tables
        cell_context.selections[i].format.clearProperty(QTextFormat::FullWidthSelection);
    }
}

void QTextDocumentLayoutPrivate::drawFrame(const QPointF &offset, QPainter *painter,
                                           const QAbstractTextDocumentLayout::PaintContext &context,
                                           QTextFrame *frame) const
{
    QTextFrameData *fd = data(frame);
    // #######
    if (fd->layoutDirty)
        return;
    Q_ASSERT(!fd->sizeDirty);
    Q_ASSERT(!fd->layoutDirty);

    const QPointF off = offset + fd->position.toPointF();
    if (context.clip.isValid()
        && (off.y() > context.clip.bottom() || off.y() + fd->size.height.toReal() < context.clip.top()
            || off.x() > context.clip.right() || off.x() + fd->size.width.toReal() < context.clip.left()))
        return;

//     LDEBUG << debug_indent << "drawFrame" << frame->firstPosition() << "--" << frame->lastPosition() << "at" << offset;
//     INC_INDENT;

    // if the cursor is /on/ a table border we may need to repaint it
    // afterwards, as we usually draw the decoration first
    QTextBlock cursorBlockNeedingRepaint;
    QPointF offsetOfRepaintedCursorBlock = off;

    QTextTable *table = qobject_cast<QTextTable *>(frame);
    const QRectF frameRect(off, fd->size.toSizeF());

    if (table) {
        const int rows = table->rows();
        const int columns = table->columns();
        QTextTableData *td = static_cast<QTextTableData *>(data(table));

        QVarLengthArray<int> selectedTableCells(context.selections.size() * 4);
        for (int i = 0; i < context.selections.size(); ++i) {
            const QAbstractTextDocumentLayout::Selection &s = context.selections.at(i);
            int row_start = -1, col_start = -1, num_rows = -1, num_cols = -1;

            if (s.cursor.currentTable() == table)
                s.cursor.selectedTableCells(&row_start, &num_rows, &col_start, &num_cols);

            selectedTableCells[i * 4] = row_start;
            selectedTableCells[i * 4 + 1] = col_start;
            selectedTableCells[i * 4 + 2] = num_rows;
            selectedTableCells[i * 4 + 3] = num_cols;
        }

        QFixed pageHeight = QFixed::fromReal(document->pageSize().height());
        if (pageHeight <= 0)
            pageHeight = QFIXED_MAX;

        const int tableStartPage = (td->position.y / pageHeight).truncate();
        const int tableEndPage = ((td->position.y + td->size.height) / pageHeight).truncate();

        qreal border = td->border.toReal();
        drawFrameDecoration(painter, frame, fd, context.clip, frameRect);

        // draw the table headers
        const int headerRowCount = qMin(table->format().headerRowCount(), rows - 1);
        int page = tableStartPage + 1;
        while (page <= tableEndPage) {
            const QFixed pageTop = page * pageHeight + td->effectiveTopMargin + td->cellSpacing + td->border;
            const qreal headerOffset = (pageTop - td->rowPositions.at(0)).toReal();
            for (int r = 0; r < headerRowCount; ++r) {
                for (int c = 0; c < columns; ++c) {
                    QTextTableCell cell = table->cellAt(r, c);
                    QAbstractTextDocumentLayout::PaintContext cell_context = context;
                    adjustContextSelectionsForCell(cell_context, cell, r, c, selectedTableCells.data());
                    QRectF cellRect = td->cellRect(cell);

                    cellRect.translate(off.x(), headerOffset);
                    // we need to account for the cell border in the clipping test
                    int leftAdjust = qMin(qreal(0), 1 - border);
                    if (cell_context.clip.isValid() && !cellRect.adjusted(leftAdjust, leftAdjust, border, border).intersects(cell_context.clip))
                        continue;

                    drawTableCell(cellRect, painter, cell_context, table, td, r, c, &cursorBlockNeedingRepaint,
                                  &offsetOfRepaintedCursorBlock);
                }
            }
            ++page;
        }

        int firstRow = 0;
        int lastRow = rows;

        if (context.clip.isValid()) {
            QVector<QFixed>::ConstIterator rowIt = qLowerBound(td->rowPositions.constBegin(), td->rowPositions.constEnd(), QFixed::fromReal(context.clip.top() - off.y()));
            if (rowIt != td->rowPositions.constEnd() && rowIt != td->rowPositions.constBegin()) {
                --rowIt;
                firstRow = rowIt - td->rowPositions.constBegin();
            }

            rowIt = qUpperBound(td->rowPositions.constBegin(), td->rowPositions.constEnd(), QFixed::fromReal(context.clip.bottom() - off.y()));
            if (rowIt != td->rowPositions.constEnd()) {
                ++rowIt;
                lastRow = rowIt - td->rowPositions.constBegin();
            }
        }

        for (int c = 0; c < columns; ++c) {
            QTextTableCell cell = table->cellAt(firstRow, c);
            firstRow = qMin(firstRow, cell.row());
        }

        for (int r = firstRow; r < lastRow; ++r) {
            for (int c = 0; c < columns; ++c) {
                QTextTableCell cell = table->cellAt(r, c);
                QAbstractTextDocumentLayout::PaintContext cell_context = context;
                adjustContextSelectionsForCell(cell_context, cell, r, c, selectedTableCells.data());
                QRectF cellRect = td->cellRect(cell);

                cellRect.translate(off);
                // we need to account for the cell border in the clipping test
                int leftAdjust = qMin(qreal(0), 1 - border);
                if (cell_context.clip.isValid() && !cellRect.adjusted(leftAdjust, leftAdjust, border, border).intersects(cell_context.clip))
                    continue;

                drawTableCell(cellRect, painter, cell_context, table, td, r, c, &cursorBlockNeedingRepaint,
                              &offsetOfRepaintedCursorBlock);
            }
        }

    } else {
        drawFrameDecoration(painter, frame, fd, context.clip, frameRect);

        QTextFrame::Iterator it = frame->begin();

        if (frame == docPrivate->rootFrame())
            it = frameIteratorForYPosition(QFixed::fromReal(context.clip.top()));

        QList<QTextFrame *> floats;
        for (int i = 0; i < fd->floats.count(); ++i)
            floats.append(fd->floats.at(i));

        drawFlow(off, painter, context, it, floats, &cursorBlockNeedingRepaint);
    }

    if (cursorBlockNeedingRepaint.isValid()) {
        const QPen oldPen = painter->pen();
        painter->setPen(context.palette.color(QPalette::Text));
        const int cursorPos = context.cursorPosition - cursorBlockNeedingRepaint.position();
        cursorBlockNeedingRepaint.layout()->drawCursor(painter, offsetOfRepaintedCursorBlock,
                                                       cursorPos, cursorWidth);
        painter->setPen(oldPen);
    }

//     DEC_INDENT;

    return;
}

void QTextDocumentLayoutPrivate::drawTableCell(const QRectF &cellRect, QPainter *painter, const QAbstractTextDocumentLayout::PaintContext &cell_context,
                                               QTextTable *table, QTextTableData *td, int r, int c,
                                               QTextBlock *cursorBlockNeedingRepaint, QPointF *cursorBlockOffset) const
{
    QTextTableCell cell = table->cellAt(r, c);
    int rspan = cell.rowSpan();
    int cspan = cell.columnSpan();
    if (rspan != 1) {
        int cr = cell.row();
        if (cr != r)
            return;
    }
    if (cspan != 1) {
        int cc = cell.column();
        if (cc != c)
            return;
    }

    QTextFormat fmt = cell.format();
    const QFixed leftPadding = td->leftPadding(fmt);
    const QFixed topPadding = td->topPadding(fmt);

    if (td->border != 0) {
        const QBrush oldBrush = painter->brush();
        const QPen oldPen = painter->pen();

        const qreal border = td->border.toReal();

        QRectF borderRect(cellRect.left() - border, cellRect.top() - border, cellRect.width() + border, cellRect.height() + border);

        // invert the border style for cells
        QTextFrameFormat::BorderStyle cellBorder = table->format().borderStyle();
        switch (cellBorder) {
        case QTextFrameFormat::BorderStyle_Inset:
            cellBorder = QTextFrameFormat::BorderStyle_Outset;
            break;
        case QTextFrameFormat::BorderStyle_Outset:
            cellBorder = QTextFrameFormat::BorderStyle_Inset;
            break;
        case QTextFrameFormat::BorderStyle_Groove:
            cellBorder = QTextFrameFormat::BorderStyle_Ridge;
            break;
        case QTextFrameFormat::BorderStyle_Ridge:
            cellBorder = QTextFrameFormat::BorderStyle_Groove;
            break;
        default:
            break;
        }

        qreal topMargin = (td->effectiveTopMargin + td->cellSpacing + td->border).toReal();
        qreal bottomMargin = (td->effectiveBottomMargin + td->cellSpacing + td->border).toReal();

        const int headerRowCount = qMin(table->format().headerRowCount(), table->rows() - 1);
        if (r >= headerRowCount)
            topMargin += td->headerHeight.toReal();

        drawBorder(painter, borderRect, topMargin, bottomMargin,
                   border, table->format().borderBrush(), cellBorder);

        painter->setBrush(oldBrush);
        painter->setPen(oldPen);
    }

    const QBrush bg = cell.format().background();
    const QPointF brushOrigin = painter->brushOrigin();
    if (bg.style() != Qt::NoBrush) {
        fillBackground(painter, cellRect, bg, cellRect.topLeft());

        if (bg.style() > Qt::SolidPattern)
            painter->setBrushOrigin(cellRect.topLeft());
    }

    const QFixed verticalOffset = td->cellVerticalOffsets.at(c + r * table->columns());

    const QPointF cellPos = QPointF(cellRect.left() + leftPadding.toReal(),
                                    cellRect.top() + (topPadding + verticalOffset).toReal());

    QTextBlock repaintBlock;
    drawFlow(cellPos, painter, cell_context, cell.begin(),
             td->childFrameMap.values(r + c * table->rows()),
             &repaintBlock);
    if (repaintBlock.isValid()) {
        *cursorBlockNeedingRepaint = repaintBlock;
        *cursorBlockOffset = cellPos;
    }

    if (bg.style() > Qt::SolidPattern)
        painter->setBrushOrigin(brushOrigin);
}

void QTextDocumentLayoutPrivate::drawFlow(const QPointF &offset, QPainter *painter, const QAbstractTextDocumentLayout::PaintContext &context,
                                          QTextFrame::Iterator it, const QList<QTextFrame *> &floats, QTextBlock *cursorBlockNeedingRepaint) const
{
    Q_Q(const QTextDocumentLayout);
    const bool inRootFrame = (!it.atEnd() && it.parentFrame() && it.parentFrame()->parentFrame() == 0);

    QVector<QCheckPoint>::ConstIterator lastVisibleCheckPoint = checkPoints.end();
    if (inRootFrame && context.clip.isValid()) {
        lastVisibleCheckPoint = qLowerBound(checkPoints.begin(), checkPoints.end(), QFixed::fromReal(context.clip.bottom()));
    }

    QTextBlock previousBlock;
    QTextFrame *previousFrame = 0;

    for (; !it.atEnd(); ++it) {
        QTextFrame *c = it.currentFrame();

        if (inRootFrame && !checkPoints.isEmpty()) {
            int currentPosInDoc;
            if (c)
                currentPosInDoc = c->firstPosition();
            else
                currentPosInDoc = it.currentBlock().position();

            // if we're past what is already laid out then we're better off
            // not trying to draw things that may not be positioned correctly yet
            if (currentPosInDoc >= checkPoints.last().positionInFrame)
                break;

            if (lastVisibleCheckPoint != checkPoints.end()
                && context.clip.isValid()
                && currentPosInDoc >= lastVisibleCheckPoint->positionInFrame
               )
                break;
        }

        if (c)
            drawFrame(offset, painter, context, c);
        else {
            QAbstractTextDocumentLayout::PaintContext pc = context;
            if (isEmptyBlockAfterTable(it.currentBlock(), previousFrame))
                pc.selections.clear();
            drawBlock(offset, painter, pc, it.currentBlock(), inRootFrame);
        }

        // when entering a table and the previous block is empty
        // then layoutFlow 'hides' the block that just causes a
        // new line by positioning it /on/ the table border. as we
        // draw that block before the table itself the decoration
        // 'overpaints' the cursor and we need to paint it afterwards
        // again
        if (isEmptyBlockBeforeTable(previousBlock, previousBlock.blockFormat(), it)
            && previousBlock.contains(context.cursorPosition)
           ) {
            *cursorBlockNeedingRepaint = previousBlock;
        }

        previousBlock = it.currentBlock();
        previousFrame = c;
    }

    for (int i = 0; i < floats.count(); ++i) {
        QTextFrame *frame = floats.at(i);
        if (!isFrameFromInlineObject(frame)
            || frame->frameFormat().position() == QTextFrameFormat::InFlow)
            continue;

        const int pos = frame->firstPosition() - 1;
        QTextCharFormat format = const_cast<QTextDocumentLayout *>(q)->format(pos);
        QTextObjectInterface *handler = q->handlerForObject(format.objectType());
        if (handler) {
            QRectF rect = frameBoundingRectInternal(frame);
            handler->drawObject(painter, rect, document, pos, format);
        }
    }
}

void QTextDocumentLayoutPrivate::drawBlock(const QPointF &offset, QPainter *painter,
                                           const QAbstractTextDocumentLayout::PaintContext &context,
                                           QTextBlock bl, bool inRootFrame) const
{
    const QTextLayout *tl = bl.layout();
    QRectF r = tl->boundingRect();
    r.translate(offset + tl->position());
    if (context.clip.isValid() && (r.bottom() < context.clip.y() || r.top() > context.clip.bottom()))
        return;
//      LDEBUG << debug_indent << "drawBlock" << bl.position() << "at" << offset << "br" << tl->boundingRect();

    QTextBlockFormat blockFormat = bl.blockFormat();

    QBrush bg = blockFormat.background();
    if (bg != Qt::NoBrush) {
        QRectF rect = r;

        // extend the background rectangle if we're in the root frame with NoWrap,
        // as the rect of the text block will then be only the width of the text
        // instead of the full page width
        if (inRootFrame && document->pageSize().width() <= 0) {
            const QTextFrameData *fd = data(document->rootFrame());
            rect.setRight((fd->size.width - fd->rightMargin).toReal());
        }

        fillBackground(painter, rect, bg, r.topLeft());
    }

    QVector<QTextLayout::FormatRange> selections;
    int blpos = bl.position();
    int bllen = bl.length();
    const QTextCharFormat *selFormat = 0;
    for (int i = 0; i < context.selections.size(); ++i) {
        const QAbstractTextDocumentLayout::Selection &range = context.selections.at(i);
        const int selStart = range.cursor.selectionStart() - blpos;
        const int selEnd = range.cursor.selectionEnd() - blpos;
        if (selStart < bllen && selEnd > 0
             && selEnd > selStart) {
            QTextLayout::FormatRange o;
            o.start = selStart;
            o.length = selEnd - selStart;
            o.format = range.format;
            selections.append(o);
        } else if (! range.cursor.hasSelection() && range.format.hasProperty(QTextFormat::FullWidthSelection)
                   && bl.contains(range.cursor.position())) {
            // for full width selections we don't require an actual selection, just
            // a position to specify the line. that's more convenience in usage.
            QTextLayout::FormatRange o;
            QTextLine l = tl->lineForTextPosition(range.cursor.position() - blpos);
            o.start = l.textStart();
            o.length = l.textLength();
            if (o.start + o.length == bllen - 1)
                ++o.length; // include newline
            o.format = range.format;
            selections.append(o);
       }
        if (selStart < 0 && selEnd >= 1)
            selFormat = &range.format;
    }

    QTextObject *object = document->objectForFormat(bl.blockFormat());
    if (object && object->format().toListFormat().style() != QTextListFormat::ListStyleUndefined)
        drawListItem(offset, painter, context, bl, selFormat);

    QPen oldPen = painter->pen();
    painter->setPen(context.palette.color(QPalette::Text));

    tl->draw(painter, offset, selections, context.clip.isValid() ? (context.clip & clipRect) : clipRect);

    if ((context.cursorPosition >= blpos && context.cursorPosition < blpos + bllen)
        || (context.cursorPosition < -1 && !tl->preeditAreaText().isEmpty())) {
        int cpos = context.cursorPosition;
        if (cpos < -1)
            cpos = tl->preeditAreaPosition() - (cpos + 2);
        else
            cpos -= blpos;
        tl->drawCursor(painter, offset, cpos, cursorWidth);
    }

    if (blockFormat.hasProperty(QTextFormat::BlockTrailingHorizontalRulerWidth)) {
        const qreal width = blockFormat.lengthProperty(QTextFormat::BlockTrailingHorizontalRulerWidth).value(r.width());
        painter->setPen(context.palette.color(QPalette::Dark));
        qreal y = r.bottom();
        if (bl.length() == 1)
            y = r.top() + r.height() / 2;

        const qreal middleX = r.left() + r.width() / 2;
        painter->drawLine(QLineF(middleX - width / 2, y, middleX + width / 2, y));
    }

    painter->setPen(oldPen);
}


void QTextDocumentLayoutPrivate::drawListItem(const QPointF &offset, QPainter *painter,
                                              const QAbstractTextDocumentLayout::PaintContext &context,
                                              QTextBlock bl, const QTextCharFormat *selectionFormat) const
{
    Q_Q(const QTextDocumentLayout);
    const QTextBlockFormat blockFormat = bl.blockFormat();
    const QTextCharFormat charFormat = QTextCursor(bl).charFormat();
    QFont font(charFormat.font());
    if (q->paintDevice())
        font = QFont(font, q->paintDevice());

    const QFontMetrics fontMetrics(font);
    QTextObject * const object = document->objectForFormat(blockFormat);
    const QTextListFormat lf = object->format().toListFormat();
    int style = lf.style();
    QString itemText;
    QSizeF size;

    if (blockFormat.hasProperty(QTextFormat::ListStyle))
        style = QTextListFormat::Style(blockFormat.intProperty(QTextFormat::ListStyle));

    QTextLayout *layout = bl.layout();
    if (layout->lineCount() == 0)
        return;
    QTextLine firstLine = layout->lineAt(0);
    Q_ASSERT(firstLine.isValid());
    QPointF pos = (offset + layout->position()).toPoint();
    Qt::LayoutDirection dir = bl.textDirection();
    {
        QRectF textRect = firstLine.naturalTextRect();
        pos += textRect.topLeft().toPoint();
        if (dir == Qt::RightToLeft)
            pos.rx() += textRect.width();
    }

    switch (style) {
    case QTextListFormat::ListDecimal:
    case QTextListFormat::ListLowerAlpha:
    case QTextListFormat::ListUpperAlpha:
    case QTextListFormat::ListLowerRoman:
    case QTextListFormat::ListUpperRoman:
        itemText = static_cast<QTextList *>(object)->itemText(bl);
        size.setWidth(fontMetrics.width(itemText));
        size.setHeight(fontMetrics.height());
        break;

    case QTextListFormat::ListSquare:
    case QTextListFormat::ListCircle:
    case QTextListFormat::ListDisc:
        size.setWidth(fontMetrics.lineSpacing() / 3);
        size.setHeight(size.width());
        break;

    case QTextListFormat::ListStyleUndefined:
        return;
    default: return;
    }

    QRectF r(pos, size);

    qreal xoff = fontMetrics.width(QLatin1Char(' '));
    if (dir == Qt::LeftToRight)
        xoff = -xoff - size.width();
    r.translate( xoff, (fontMetrics.height() / 2 - size.height() / 2));

    painter->save();

    painter->setRenderHint(QPainter::Antialiasing);

    if (selectionFormat) {
        painter->setPen(QPen(selectionFormat->foreground(), 0));
        painter->fillRect(r, selectionFormat->background());
    } else {
        QBrush fg = charFormat.foreground();
        if (fg == Qt::NoBrush)
            fg = context.palette.text();
        painter->setPen(QPen(fg, 0));
    }

    QBrush brush = context.palette.brush(QPalette::Text);

    switch (style) {
    case QTextListFormat::ListDecimal:
    case QTextListFormat::ListLowerAlpha:
    case QTextListFormat::ListUpperAlpha:
    case QTextListFormat::ListLowerRoman:
    case QTextListFormat::ListUpperRoman: {
        QTextLayout layout(itemText, font, q->paintDevice());
        layout.setCacheEnabled(true);
        QTextOption option(Qt::AlignLeft | Qt::AlignAbsolute);
        option.setTextDirection(dir);
        layout.setTextOption(option);
        layout.beginLayout();
        QTextLine line = layout.createLine();
        if (line.isValid())
            line.setLeadingIncluded(true);
        layout.endLayout();
        layout.draw(painter, QPointF(r.left(), pos.y()));
        break;
    }
    case QTextListFormat::ListSquare:
        painter->fillRect(r, brush);
        break;
    case QTextListFormat::ListCircle:
        painter->setPen(QPen(brush, 0));
        painter->drawEllipse(r.translated(0.5, 0.5)); // pixel align for sharper rendering
        break;
    case QTextListFormat::ListDisc:
        painter->setBrush(brush);
        painter->setPen(Qt::NoPen);
        painter->drawEllipse(r);
        break;
    case QTextListFormat::ListStyleUndefined:
        break;
    default:
        break;
    }

    painter->restore();
}

static QFixed flowPosition(const QTextFrame::iterator it)
{
    if (it.atEnd())
        return 0;

    if (it.currentFrame()) {
        return data(it.currentFrame())->position.y;
    } else {
        QTextBlock block = it.currentBlock();
        QTextLayout *layout = block.layout();
        if (layout->lineCount() == 0)
            return QFixed::fromReal(layout->position().y());
        else
            return QFixed::fromReal(layout->position().y() + layout->lineAt(0).y());
    }
}

static QFixed firstChildPos(const QTextFrame *f)
{
    return flowPosition(f->begin());
}

QTextLayoutStruct QTextDocumentLayoutPrivate::layoutCell(QTextTable *t, const QTextTableCell &cell, QFixed width,
                                                        int layoutFrom, int layoutTo, QTextTableData *td,
                                                        QFixed absoluteTableY, bool withPageBreaks)
{
    LDEBUG << "layoutCell";
    QTextLayoutStruct layoutStruct;
    layoutStruct.frame = t;
    layoutStruct.minimumWidth = 0;
    layoutStruct.maximumWidth = QFIXED_MAX;
    layoutStruct.y = 0;

    const QTextFormat fmt = cell.format();
    const QFixed topPadding = td->topPadding(fmt);
    if (withPageBreaks) {
        layoutStruct.frameY = absoluteTableY + td->rowPositions.at(cell.row()) + topPadding;
    }
    layoutStruct.x_left = 0;
    layoutStruct.x_right = width;
    // we get called with different widths all the time (for example for figuring
    // out the min/max widths), so we always have to do the full layout ;(
    // also when for example in a table layoutFrom/layoutTo affect only one cell,
    // making that one cell grow the available width of the other cells may change
    // (shrink) and therefore when layoutCell gets called for them they have to
    // be re-laid out, even if layoutFrom/layoutTo is not in their range. Hence
    // this line:

    layoutStruct.pageHeight = QFixed::fromReal(document->pageSize().height());
    if (layoutStruct.pageHeight < 0 || !withPageBreaks)
        layoutStruct.pageHeight = QFIXED_MAX;
    const int currentPage = layoutStruct.currentPage();
    layoutStruct.pageTopMargin = td->effectiveTopMargin + td->cellSpacing + td->border + topPadding;
    layoutStruct.pageBottomMargin = td->effectiveBottomMargin + td->cellSpacing + td->border + td->bottomPadding(fmt);
    layoutStruct.pageBottom = (currentPage + 1) * layoutStruct.pageHeight - layoutStruct.pageBottomMargin;

    layoutStruct.fullLayout = true;

    QFixed pageTop = currentPage * layoutStruct.pageHeight + layoutStruct.pageTopMargin - layoutStruct.frameY;
    layoutStruct.y = qMax(layoutStruct.y, pageTop);

    const QList<QTextFrame *> childFrames = td->childFrameMap.values(cell.row() + cell.column() * t->rows());
    for (int i = 0; i < childFrames.size(); ++i) {
        QTextFrame *frame = childFrames.at(i);
        QTextFrameData *cd = data(frame);
        cd->sizeDirty = true;
    }

    layoutFlow(cell.begin(), &layoutStruct, layoutFrom, layoutTo, width);

    QFixed floatMinWidth;

    // floats that are located inside the text (like inline images) aren't taken into account by
    // layoutFlow with regards to the cell height (layoutStruct->y), so for a safety measure we
    // do that here. For example with <td><img align="right" src="..." />blah</td>
    // when the image happens to be higher than the text
    for (int i = 0; i < childFrames.size(); ++i) {
        QTextFrame *frame = childFrames.at(i);
        QTextFrameData *cd = data(frame);

        if (frame->frameFormat().position() != QTextFrameFormat::InFlow)
            layoutStruct.y = qMax(layoutStruct.y, cd->position.y + cd->size.height);

        floatMinWidth = qMax(floatMinWidth, cd->minimumWidth);
    }

    // constraint the maximumWidth by the minimum width of the fixed size floats, to
    // keep them visible
    layoutStruct.maximumWidth = qMax(layoutStruct.maximumWidth, floatMinWidth);

    // as floats in cells get added to the table's float list but must not affect
    // floats in other cells we must clear the list here.
    data(t)->floats.clear();

//    qDebug() << "layoutCell done";

    return layoutStruct;
}

QRectF QTextDocumentLayoutPrivate::layoutTable(QTextTable *table, int layoutFrom, int layoutTo, QFixed parentY)
{
    LDEBUG << "layoutTable";
    QTextTableData *td = static_cast<QTextTableData *>(data(table));
    Q_ASSERT(td->sizeDirty);
    const int rows = table->rows();
    const int columns = table->columns();

    const QTextTableFormat fmt = table->format();

    td->childFrameMap.clear();
    {
        const QList<QTextFrame *> children = table->childFrames();
        for (int i = 0; i < children.count(); ++i) {
            QTextFrame *frame = children.at(i);
            QTextTableCell cell = table->cellAt(frame->firstPosition());
            td->childFrameMap.insertMulti(cell.row() + cell.column() * rows, frame);
        }
    }

    QVector<QTextLength> columnWidthConstraints = fmt.columnWidthConstraints();
    if (columnWidthConstraints.size() != columns)
        columnWidthConstraints.resize(columns);
    Q_ASSERT(columnWidthConstraints.count() == columns);

    const QFixed cellSpacing = td->cellSpacing = QFixed::fromReal(scaleToDevice(fmt.cellSpacing()));
    td->deviceScale = scaleToDevice(qreal(1));
    td->cellPadding = QFixed::fromReal(scaleToDevice(fmt.cellPadding()));
    const QFixed leftMargin = td->leftMargin + td->border + td->padding;
    const QFixed rightMargin = td->rightMargin + td->border + td->padding;
    const QFixed topMargin = td->topMargin + td->border + td->padding;

    const QFixed absoluteTableY = parentY + td->position.y;

    const QTextOption::WrapMode oldDefaultWrapMode = docPrivate->defaultTextOption.wrapMode();

recalc_minmax_widths:

    QFixed remainingWidth = td->contentsWidth;
    // two (vertical) borders per cell per column
    remainingWidth -= columns * 2 * td->border;
    // inter-cell spacing
    remainingWidth -= (columns - 1) * cellSpacing;
    // cell spacing at the left and right hand side
    remainingWidth -= 2 * cellSpacing;
    // remember the width used to distribute to percentaged columns
    const QFixed initialTotalWidth = remainingWidth;

    td->widths.resize(columns);
    td->widths.fill(0);

    td->minWidths.resize(columns);
    // start with a minimum width of 0. totally empty
    // cells of default created tables are invisible otherwise
    // and therefore hardly editable
    td->minWidths.fill(1);

    td->maxWidths.resize(columns);
    td->maxWidths.fill(QFIXED_MAX);

    // calculate minimum and maximum sizes of the columns
    for (int i = 0; i < columns; ++i) {
        for (int row = 0; row < rows; ++row) {
            const QTextTableCell cell = table->cellAt(row, i);
            const int cspan = cell.columnSpan();

            if (cspan > 1 && i != cell.column())
                continue;

            const QTextFormat fmt = cell.format();
            const QFixed leftPadding = td->leftPadding(fmt);
            const QFixed rightPadding = td->rightPadding(fmt);
            const QFixed widthPadding = leftPadding + rightPadding;

            // to figure out the min and the max width lay out the cell at
            // maximum width. otherwise the maxwidth calculation sometimes
            // returns wrong values
            QTextLayoutStruct layoutStruct = layoutCell(table, cell, QFIXED_MAX, layoutFrom,
                                                        layoutTo, td, absoluteTableY,
                                                        /*withPageBreaks =*/false);

            // distribute the minimum width over all columns the cell spans
            QFixed widthToDistribute = layoutStruct.minimumWidth + widthPadding;
            for (int n = 0; n < cspan; ++n) {
                const int col = i + n;
                QFixed w = widthToDistribute / (cspan - n);
                td->minWidths[col] = qMax(td->minWidths.at(col), w);
                widthToDistribute -= td->minWidths.at(col);
                if (widthToDistribute <= 0)
                    break;
            }

            QFixed maxW = td->maxWidths.at(i);
            if (layoutStruct.maximumWidth != QFIXED_MAX) {
                if (maxW == QFIXED_MAX)
                    maxW = layoutStruct.maximumWidth + widthPadding;
                else
                    maxW = qMax(maxW, layoutStruct.maximumWidth + widthPadding);
            }
            if (maxW == QFIXED_MAX)
                continue;

            widthToDistribute = maxW;
            for (int n = 0; n < cspan; ++n) {
                const int col = i + n;
                QFixed w = widthToDistribute / (cspan - n);
                td->maxWidths[col] = qMax(td->minWidths.at(col), w);
                widthToDistribute -= td->maxWidths.at(col);
                if (widthToDistribute <= 0)
                    break;
            }
        }
    }

    // set fixed values, figure out total percentages used and number of
    // variable length cells. Also assign the minimum width for variable columns.
    QFixed totalPercentage;
    int variableCols = 0;
    QFixed totalMinWidth = 0;
    for (int i = 0; i < columns; ++i) {
        const QTextLength &length = columnWidthConstraints.at(i);
        if (length.type() == QTextLength::FixedLength) {
            td->minWidths[i] = td->widths[i] = qMax(scaleToDevice(QFixed::fromReal(length.rawValue())), td->minWidths.at(i));
            remainingWidth -= td->widths.at(i);
        } else if (length.type() == QTextLength::PercentageLength) {
            totalPercentage += QFixed::fromReal(length.rawValue());
        } else if (length.type() == QTextLength::VariableLength) {
            variableCols++;

            td->widths[i] = td->minWidths.at(i);
            remainingWidth -= td->minWidths.at(i);
        }
        totalMinWidth += td->minWidths.at(i);
    }

    // set percentage values
    {
        const QFixed totalPercentagedWidth = initialTotalWidth * totalPercentage / 100;
        QFixed remainingMinWidths = totalMinWidth;
        for (int i = 0; i < columns; ++i) {
            remainingMinWidths -= td->minWidths.at(i);
            if (columnWidthConstraints.at(i).type() == QTextLength::PercentageLength) {
                const QFixed allottedPercentage = QFixed::fromReal(columnWidthConstraints.at(i).rawValue());

                const QFixed percentWidth = totalPercentagedWidth * allottedPercentage / totalPercentage;
                if (percentWidth >= td->minWidths.at(i)) {
                    td->widths[i] = qBound(td->minWidths.at(i), percentWidth, remainingWidth - remainingMinWidths);
                } else {
                    td->widths[i] = td->minWidths.at(i);
                }
                remainingWidth -= td->widths.at(i);
            }
        }
    }

    // for variable columns distribute the remaining space
    if (variableCols > 0 && remainingWidth > 0) {
        QVarLengthArray<int> columnsWithProperMaxSize;
        for (int i = 0; i < columns; ++i)
            if (columnWidthConstraints.at(i).type() == QTextLength::VariableLength
                && td->maxWidths.at(i) != QFIXED_MAX)
                columnsWithProperMaxSize.append(i);

        QFixed lastRemainingWidth = remainingWidth;
        while (remainingWidth > 0) {
            for (int k = 0; k < columnsWithProperMaxSize.count(); ++k) {
                const int col = columnsWithProperMaxSize[k];
                const int colsLeft = columnsWithProperMaxSize.count() - k;
                const QFixed w = qMin(td->maxWidths.at(col) - td->widths.at(col), remainingWidth / colsLeft);
                td->widths[col] += w;
                remainingWidth -= w;
            }
            if (remainingWidth == lastRemainingWidth)
                break;
            lastRemainingWidth = remainingWidth;
        }

        if (remainingWidth > 0
            // don't unnecessarily grow variable length sized tables
            && fmt.width().type() != QTextLength::VariableLength) {
            const QFixed widthPerAnySizedCol = remainingWidth / variableCols;
            for (int col = 0; col < columns; ++col) {
                if (columnWidthConstraints.at(col).type() == QTextLength::VariableLength)
                    td->widths[col] += widthPerAnySizedCol;
            }
        }
    }

    td->columnPositions.resize(columns);
    td->columnPositions[0] = leftMargin /*includes table border*/ + cellSpacing + td->border;

    for (int i = 1; i < columns; ++i)
        td->columnPositions[i] = td->columnPositions.at(i-1) + td->widths.at(i-1) + 2 * td->border + cellSpacing;

    // - margin to compensate the + margin in columnPositions[0]
    const QFixed contentsWidth = td->columnPositions.last() + td->widths.last() + td->padding + td->border + cellSpacing - leftMargin;

    // if the table is too big and causes an overflow re-do the layout with WrapAnywhere as wrap
    // mode
    if (docPrivate->defaultTextOption.wrapMode() == QTextOption::WrapAtWordBoundaryOrAnywhere
        && contentsWidth > td->contentsWidth) {
        docPrivate->defaultTextOption.setWrapMode(QTextOption::WrapAnywhere);
        // go back to the top of the function
        goto recalc_minmax_widths;
    }

    td->contentsWidth = contentsWidth;

    docPrivate->defaultTextOption.setWrapMode(oldDefaultWrapMode);

    td->heights.resize(rows);
    td->heights.fill(0);

    td->rowPositions.resize(rows);
    td->rowPositions[0] = topMargin /*includes table border*/ + cellSpacing + td->border;

    bool haveRowSpannedCells = false;

    // need to keep track of cell heights for vertical alignment
    QVector<QFixed> cellHeights;
    cellHeights.reserve(rows * columns);

    QFixed pageHeight = QFixed::fromReal(document->pageSize().height());
    if (pageHeight <= 0)
        pageHeight = QFIXED_MAX;

    QVector<QFixed> heightToDistribute;
    heightToDistribute.resize(columns);

    td->headerHeight = 0;
    const int headerRowCount = qMin(table->format().headerRowCount(), rows - 1);
    const QFixed originalTopMargin = td->effectiveTopMargin;
    bool hasDroppedTable = false;

    // now that we have the column widths we can lay out all cells with the right width.
    // spanning cells are only allowed to grow the last row spanned by the cell.
    //
    // ### this could be made faster by iterating over the cells array of QTextTable
    for (int r = 0; r < rows; ++r) {
        td->calcRowPosition(r);

        const int tableStartPage = (absoluteTableY / pageHeight).truncate();
        const int currentPage = ((td->rowPositions[r] + absoluteTableY) / pageHeight).truncate();
        const QFixed pageBottom = (currentPage + 1) * pageHeight - td->effectiveBottomMargin - absoluteTableY - cellSpacing - td->border;
        const QFixed pageTop = currentPage * pageHeight + td->effectiveTopMargin - absoluteTableY + cellSpacing + td->border;
        const QFixed nextPageTop = pageTop + pageHeight;

        if (td->rowPositions[r] > pageBottom)
            td->rowPositions[r] = nextPageTop;
        else if (td->rowPositions[r] < pageTop)
            td->rowPositions[r] = pageTop;

        bool dropRowToNextPage = true;
        int cellCountBeforeRow = cellHeights.size();

        // if we drop the row to the next page we need to subtract the drop
        // distance from any row spanning cells
        QFixed dropDistance = 0;

relayout:
        const int rowStartPage = ((td->rowPositions[r] + absoluteTableY) / pageHeight).truncate();
        // if any of the header rows or the first non-header row start on the next page
        // then the entire header should be dropped
        if (r <= headerRowCount && rowStartPage > tableStartPage && !hasDroppedTable) {
            td->rowPositions[0] = nextPageTop;
            cellHeights.clear();
            td->effectiveTopMargin = originalTopMargin;
            hasDroppedTable = true;
            r = -1;
            continue;
        }

        int rowCellCount = 0;
        for (int c = 0; c < columns; ++c) {
            QTextTableCell cell = table->cellAt(r, c);
            const int rspan = cell.rowSpan();
            const int cspan = cell.columnSpan();

            if (cspan > 1 && cell.column() != c)
                continue;

            if (rspan > 1) {
                haveRowSpannedCells = true;

                const int cellRow = cell.row();
                if (cellRow != r) {
                    // the last row gets all the remaining space
                    if (cellRow + rspan - 1 == r)
                        td->heights[r] = qMax(td->heights.at(r), heightToDistribute.at(c) - dropDistance);
                    continue;
                }
            }

            const QTextFormat fmt = cell.format();

            const QFixed topPadding = td->topPadding(fmt);
            const QFixed bottomPadding = td->bottomPadding(fmt);
            const QFixed leftPadding = td->leftPadding(fmt);
            const QFixed rightPadding = td->rightPadding(fmt);
            const QFixed widthPadding = leftPadding + rightPadding;

            ++rowCellCount;

            const QFixed width = td->cellWidth(c, cspan) - widthPadding;
            QTextLayoutStruct layoutStruct = layoutCell(table, cell, width,
                                                       layoutFrom, layoutTo,
                                                       td, absoluteTableY,
                                                       /*withPageBreaks =*/true);

            const QFixed height = layoutStruct.y + bottomPadding + topPadding;

            if (rspan > 1)
                heightToDistribute[c] = height + dropDistance;
            else
                td->heights[r] = qMax(td->heights.at(r), height);

            cellHeights.append(layoutStruct.y);

            QFixed childPos = td->rowPositions.at(r) + topPadding + flowPosition(cell.begin());
            if (childPos < pageBottom)
                dropRowToNextPage = false;
        }

        if (rowCellCount > 0 && dropRowToNextPage) {
            dropDistance = nextPageTop - td->rowPositions[r];
            td->rowPositions[r] = nextPageTop;
            td->heights[r] = 0;
            dropRowToNextPage = false;
            cellHeights.resize(cellCountBeforeRow);
            if (r > headerRowCount)
                td->heights[r-1] = pageBottom - td->rowPositions[r-1];
            goto relayout;
        }

        if (haveRowSpannedCells) {
            const QFixed effectiveHeight = td->heights.at(r) + td->border + cellSpacing + td->border;
            for (int c = 0; c < columns; ++c)
                heightToDistribute[c] = qMax(heightToDistribute.at(c) - effectiveHeight - dropDistance, QFixed(0));
        }

        if (r == headerRowCount - 1) {
            td->headerHeight = td->rowPositions[r] + td->heights[r] - td->rowPositions[0] + td->cellSpacing + 2 * td->border;
            td->headerHeight -= td->headerHeight * (td->headerHeight / pageHeight).truncate();
            td->effectiveTopMargin += td->headerHeight;
        }
    }

    td->effectiveTopMargin = originalTopMargin;

    // now that all cells have been properly laid out, we can compute the
    // vertical offsets for vertical alignment
    td->cellVerticalOffsets.resize(rows * columns);
    int cellIndex = 0;
    for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < columns; ++c) {
            QTextTableCell cell = table->cellAt(r, c);
            if (cell.row() != r || cell.column() != c)
                continue;

            const int rowSpan = cell.rowSpan();
            const QFixed availableHeight = td->rowPositions.at(r + rowSpan - 1) + td->heights.at(r + rowSpan - 1) - td->rowPositions.at(r);

            const QTextCharFormat cellFormat = cell.format();
            const QFixed cellHeight = cellHeights.at(cellIndex++) + td->topPadding(cellFormat) + td->bottomPadding(cellFormat);

            QFixed offset = 0;
            switch (cellFormat.verticalAlignment()) {
            case QTextCharFormat::AlignMiddle:
                offset = (availableHeight - cellHeight) / 2;
                break;
            case QTextCharFormat::AlignBottom:
                offset = availableHeight - cellHeight;
                break;
            default:
                break;
            };

            for (int rd = 0; rd < cell.rowSpan(); ++rd) {
                for (int cd = 0; cd < cell.columnSpan(); ++cd) {
                    const int index = (c + cd) + (r + rd) * columns;
                    td->cellVerticalOffsets[index] = offset;
                }
            }
        }
    }

    td->minimumWidth = td->columnPositions.at(0);
    for (int i = 0; i < columns; ++i) {
        td->minimumWidth += td->minWidths.at(i) + 2 * td->border + cellSpacing;
    }
    td->minimumWidth += rightMargin - td->border;

    td->maximumWidth = td->columnPositions.at(0);
    for (int i = 0; i < columns; ++i)
        if (td->maxWidths.at(i) != QFIXED_MAX)
            td->maximumWidth += td->maxWidths.at(i) + 2 * td->border + cellSpacing;
    td->maximumWidth += rightMargin - td->border;

    td->updateTableSize();
    td->sizeDirty = false;
    return QRectF(); // invalid rect -> update everything
}

void QTextDocumentLayoutPrivate::positionFloat(QTextFrame *frame, QTextLine *currentLine)
{
    QTextFrameData *fd = data(frame);

    QTextFrame *parent = frame->parentFrame();
    Q_ASSERT(parent);
    QTextFrameData *pd = data(parent);
    Q_ASSERT(pd && pd->currentLayoutStruct);

    QTextLayoutStruct *layoutStruct = pd->currentLayoutStruct;

    if (!pd->floats.contains(frame))
        pd->floats.append(frame);
    fd->layoutDirty = true;
    Q_ASSERT(!fd->sizeDirty);

//     qDebug() << "positionFloat:" << frame << "width=" << fd->size.width;
    QFixed y = layoutStruct->y;
    if (currentLine) {
        QFixed left, right;
        floatMargins(y, layoutStruct, &left, &right);
//         qDebug() << "have line: right=" << right << "left=" << left << "textWidth=" << currentLine->width();
        if (right - left < QFixed::fromReal(currentLine->naturalTextWidth()) + fd->size.width) {
            layoutStruct->pendingFloats.append(frame);
//             qDebug() << "    adding to pending list";
            return;
        }
    }

    bool frameSpansIntoNextPage = (y + layoutStruct->frameY + fd->size.height > layoutStruct->pageBottom);
    if (frameSpansIntoNextPage && fd->size.height <= layoutStruct->pageHeight) {
        layoutStruct->newPage();
        y = layoutStruct->y;

        frameSpansIntoNextPage = false;
    }

    y = findY(y, layoutStruct, fd->size.width);

    QFixed left, right;
    floatMargins(y, layoutStruct, &left, &right);

    if (frame->frameFormat().position() == QTextFrameFormat::FloatLeft) {
        fd->position.x = left;
        fd->position.y = y;
    } else {
        fd->position.x = right - fd->size.width;
        fd->position.y = y;
    }

    layoutStruct->minimumWidth = qMax(layoutStruct->minimumWidth, fd->minimumWidth);
    layoutStruct->maximumWidth = qMin(layoutStruct->maximumWidth, fd->maximumWidth);

//     qDebug()<< "float positioned at " << fd->position.x << fd->position.y;
    fd->layoutDirty = false;

    // If the frame is a table, then positioning it will affect the size if it covers more than
    // one page, because of page breaks and repeating the header.
    if (qobject_cast<QTextTable *>(frame) != 0)
        fd->sizeDirty = frameSpansIntoNextPage;
}

QRectF QTextDocumentLayoutPrivate::layoutFrame(QTextFrame *f, int layoutFrom, int layoutTo, QFixed parentY)
{
    LDEBUG << "layoutFrame (pre)";
    Q_ASSERT(data(f)->sizeDirty);
//     qDebug("layouting frame (%d--%d), parent=%p", f->firstPosition(), f->lastPosition(), f->parentFrame());

    QTextFrameFormat fformat = f->frameFormat();

    QTextFrame *parent = f->parentFrame();
    const QTextFrameData *pd = parent ? data(parent) : 0;

    const qreal maximumWidth = qMax(qreal(0), pd ? pd->contentsWidth.toReal() : document->pageSize().width());
    QFixed width = QFixed::fromReal(fformat.width().value(maximumWidth));
    if (fformat.width().type() == QTextLength::FixedLength)
        width = scaleToDevice(width);

    const QFixed maximumHeight = pd ? pd->contentsHeight : -1;
    const QFixed height = (maximumHeight != -1 || fformat.height().type() != QTextLength::PercentageLength)
                            ? QFixed::fromReal(fformat.height().value(maximumHeight.toReal()))
                            : -1;

    return layoutFrame(f, layoutFrom, layoutTo, width, height, parentY);
}

QRectF QTextDocumentLayoutPrivate::layoutFrame(QTextFrame *f, int layoutFrom, int layoutTo, QFixed frameWidth, QFixed frameHeight, QFixed parentY)
{
    LDEBUG << "layoutFrame from=" << layoutFrom << "to=" << layoutTo;
    Q_ASSERT(data(f)->sizeDirty);
//     qDebug("layouting frame (%d--%d), parent=%p", f->firstPosition(), f->lastPosition(), f->parentFrame());

    QTextFrameData *fd = data(f);
    QFixed newContentsWidth;

    {
        QTextFrameFormat fformat = f->frameFormat();
        // set sizes of this frame from the format
        fd->topMargin = QFixed::fromReal(fformat.topMargin());
        fd->bottomMargin = QFixed::fromReal(fformat.bottomMargin());
        fd->leftMargin = QFixed::fromReal(fformat.leftMargin());
        fd->rightMargin = QFixed::fromReal(fformat.rightMargin());
        fd->border = QFixed::fromReal(fformat.border());
        fd->padding = QFixed::fromReal(fformat.padding());

        QTextFrame *parent = f->parentFrame();
        const QTextFrameData *pd = parent ? data(parent) : 0;

        // accumulate top and bottom margins
        if (parent) {
            fd->effectiveTopMargin = pd->effectiveTopMargin + fd->topMargin + fd->border + fd->padding;
            fd->effectiveBottomMargin = pd->effectiveBottomMargin + fd->topMargin + fd->border + fd->padding;

            if (qobject_cast<QTextTable *>(parent)) {
                const QTextTableData *td = static_cast<const QTextTableData *>(pd);
                fd->effectiveTopMargin += td->cellSpacing + td->border + td->cellPadding;
                fd->effectiveBottomMargin += td->cellSpacing + td->border + td->cellPadding;
            }
        } else {
            fd->effectiveTopMargin = fd->topMargin + fd->border + fd->padding;
            fd->effectiveBottomMargin = fd->bottomMargin + fd->border + fd->padding;
        }

        newContentsWidth = frameWidth - 2*(fd->border + fd->padding)
                           - fd->leftMargin - fd->rightMargin;

        if (frameHeight != -1) {
            fd->contentsHeight = frameHeight - 2*(fd->border + fd->padding)
                                 - fd->topMargin - fd->bottomMargin;
        } else {
            fd->contentsHeight = frameHeight;
        }
    }

    if (isFrameFromInlineObject(f)) {
        // never reached, handled in resizeInlineObject/positionFloat instead
        return QRectF();
    }

    if (QTextTable *table = qobject_cast<QTextTable *>(f)) {
        fd->contentsWidth = newContentsWidth;
        return layoutTable(table, layoutFrom, layoutTo, parentY);
    }

    // set fd->contentsWidth temporarily, so that layoutFrame for the children
    // picks the right width. We'll initialize it properly at the end of this
    // function.
    fd->contentsWidth = newContentsWidth;

    QTextLayoutStruct layoutStruct;
    layoutStruct.frame = f;
    layoutStruct.x_left = fd->leftMargin + fd->border + fd->padding;
    layoutStruct.x_right = layoutStruct.x_left + newContentsWidth;
    layoutStruct.y = fd->topMargin + fd->border + fd->padding;
    layoutStruct.frameY = parentY + fd->position.y;
    layoutStruct.contentsWidth = 0;
    layoutStruct.minimumWidth = 0;
    layoutStruct.maximumWidth = QFIXED_MAX;
    layoutStruct.fullLayout = fd->oldContentsWidth != newContentsWidth;
    layoutStruct.updateRect = QRectF(QPointF(0, 0), QSizeF(qreal(INT_MAX), qreal(INT_MAX)));
    LDEBUG << "layoutStruct: x_left" << layoutStruct.x_left << "x_right" << layoutStruct.x_right
           << "fullLayout" << layoutStruct.fullLayout;
    fd->oldContentsWidth = newContentsWidth;

    layoutStruct.pageHeight = QFixed::fromReal(document->pageSize().height());
    if (layoutStruct.pageHeight < 0)
        layoutStruct.pageHeight = QFIXED_MAX;

    const int currentPage = layoutStruct.pageHeight == 0 ? 0 : (layoutStruct.frameY / layoutStruct.pageHeight).truncate();
    layoutStruct.pageTopMargin = fd->effectiveTopMargin;
    layoutStruct.pageBottomMargin = fd->effectiveBottomMargin;
    layoutStruct.pageBottom = (currentPage + 1) * layoutStruct.pageHeight - layoutStruct.pageBottomMargin;

    if (!f->parentFrame())
        idealWidth = 0; // reset

    QTextFrame::Iterator it = f->begin();
    layoutFlow(it, &layoutStruct, layoutFrom, layoutTo);

    QFixed maxChildFrameWidth = 0;
    QList<QTextFrame *> children = f->childFrames();
    for (int i = 0; i < children.size(); ++i) {
        QTextFrame *c = children.at(i);
        QTextFrameData *cd = data(c);
        maxChildFrameWidth = qMax(maxChildFrameWidth, cd->size.width);
    }

    const QFixed marginWidth = 2*(fd->border + fd->padding) + fd->leftMargin + fd->rightMargin;
    if (!f->parentFrame()) {
        idealWidth = qMax(maxChildFrameWidth, layoutStruct.contentsWidth).toReal();
        idealWidth += marginWidth.toReal();
    }

    QFixed actualWidth = qMax(newContentsWidth, qMax(maxChildFrameWidth, layoutStruct.contentsWidth));
    fd->contentsWidth = actualWidth;
    if (newContentsWidth <= 0) { // nowrap layout?
        fd->contentsWidth = newContentsWidth;
    }

    fd->minimumWidth = layoutStruct.minimumWidth;
    fd->maximumWidth = layoutStruct.maximumWidth;

    fd->size.height = fd->contentsHeight == -1
                 ? layoutStruct.y + fd->border + fd->padding + fd->bottomMargin
                 : fd->contentsHeight + 2*(fd->border + fd->padding) + fd->topMargin + fd->bottomMargin;
    fd->size.width = actualWidth + marginWidth;
    fd->sizeDirty = false;
    if (layoutStruct.updateRectForFloats.isValid())
        layoutStruct.updateRect |= layoutStruct.updateRectForFloats;
    return layoutStruct.updateRect;
}

void QTextDocumentLayoutPrivate::layoutFlow(QTextFrame::Iterator it, QTextLayoutStruct *layoutStruct,
                                            int layoutFrom, int layoutTo, QFixed width)
{
    LDEBUG << "layoutFlow from=" << layoutFrom << "to=" << layoutTo;
    QTextFrameData *fd = data(layoutStruct->frame);

    fd->currentLayoutStruct = layoutStruct;

    QTextFrame::Iterator previousIt;

    const bool inRootFrame = (it.parentFrame() == document->rootFrame());
    if (inRootFrame) {
        bool redoCheckPoints = layoutStruct->fullLayout || checkPoints.isEmpty();

        if (!redoCheckPoints) {
            QVector<QCheckPoint>::Iterator checkPoint = qLowerBound(checkPoints.begin(), checkPoints.end(), layoutFrom);
            if (checkPoint != checkPoints.end()) {
                if (checkPoint != checkPoints.begin())
                    --checkPoint;

                layoutStruct->y = checkPoint->y;
                layoutStruct->frameY = checkPoint->frameY;
                layoutStruct->minimumWidth = checkPoint->minimumWidth;
                layoutStruct->maximumWidth = checkPoint->maximumWidth;
                layoutStruct->contentsWidth = checkPoint->contentsWidth;

                if (layoutStruct->pageHeight > 0) {
                    int page = layoutStruct->currentPage();
                    layoutStruct->pageBottom = (page + 1) * layoutStruct->pageHeight - layoutStruct->pageBottomMargin;
                }

                it = frameIteratorForTextPosition(checkPoint->positionInFrame);
                checkPoints.resize(checkPoint - checkPoints.begin() + 1);

                if (checkPoint != checkPoints.begin()) {
                    previousIt = it;
                    --previousIt;
                }
            } else {
                redoCheckPoints = true;
            }
        }

        if (redoCheckPoints) {
            checkPoints.clear();
            QCheckPoint cp;
            cp.y = layoutStruct->y;
            cp.frameY = layoutStruct->frameY;
            cp.positionInFrame = 0;
            cp.minimumWidth = layoutStruct->minimumWidth;
            cp.maximumWidth = layoutStruct->maximumWidth;
            cp.contentsWidth = layoutStruct->contentsWidth;
            checkPoints.append(cp);
        }
    }

    QTextBlockFormat previousBlockFormat = previousIt.currentBlock().blockFormat();

    QFixed maximumBlockWidth = 0;
    while (!it.atEnd()) {
        QTextFrame *c = it.currentFrame();

        int docPos;
        if (it.currentFrame())
            docPos = it.currentFrame()->firstPosition();
        else
            docPos = it.currentBlock().position();

        if (inRootFrame) {
            if (qAbs(layoutStruct->y - checkPoints.last().y) > 2000) {
                QFixed left, right;
                floatMargins(layoutStruct->y, layoutStruct, &left, &right);
                if (left == layoutStruct->x_left && right == layoutStruct->x_right) {
                    QCheckPoint p;
                    p.y = layoutStruct->y;
                    p.frameY = layoutStruct->frameY;
                    p.positionInFrame = docPos;
                    p.minimumWidth = layoutStruct->minimumWidth;
                    p.maximumWidth = layoutStruct->maximumWidth;
                    p.contentsWidth = layoutStruct->contentsWidth;
                    checkPoints.append(p);

                    if (currentLazyLayoutPosition != -1
                        && docPos > currentLazyLayoutPosition + lazyLayoutStepSize)
                        break;

                }
            }
        }

        if (c) {
            // position child frame
            QTextFrameData *cd = data(c);

            QTextFrameFormat fformat = c->frameFormat();

            if (fformat.position() == QTextFrameFormat::InFlow) {
                if (fformat.pageBreakPolicy() & QTextFormat::PageBreak_AlwaysBefore)
                    layoutStruct->newPage();

                QFixed left, right;
                floatMargins(layoutStruct->y, layoutStruct, &left, &right);
                left = qMax(left, layoutStruct->x_left);
                right = qMin(right, layoutStruct->x_right);

                if (right - left < cd->size.width) {
                    layoutStruct->y = findY(layoutStruct->y, layoutStruct, cd->size.width);
                    floatMargins(layoutStruct->y, layoutStruct, &left, &right);
                }

                QFixedPoint pos(left, layoutStruct->y);

                Qt::Alignment align = Qt::AlignLeft;

                QTextTable *table = qobject_cast<QTextTable *>(c);

                if (table)
                    align = table->format().alignment() & Qt::AlignHorizontal_Mask;

                // detect whether we have any alignment in the document that disallows optimizations,
                // such as not laying out the document again in a textedit with wrapping disabled.
                if (inRootFrame && !(align & Qt::AlignLeft))
                    contentHasAlignment = true;

                cd->position = pos;

                if (document->pageSize().height() > 0.0f)
                    cd->sizeDirty = true;

                if (cd->sizeDirty) {
                    if (width != 0)
                        layoutFrame(c, layoutFrom, layoutTo, width, -1, layoutStruct->frameY);
                    else
                        layoutFrame(c, layoutFrom, layoutTo, layoutStruct->frameY);

                    QFixed absoluteChildPos = table ? pos.y + static_cast<QTextTableData *>(data(table))->rowPositions.at(0) : pos.y + firstChildPos(c);
                    absoluteChildPos += layoutStruct->frameY;

                    // drop entire frame to next page if first child of frame is on next page
                    if (absoluteChildPos > layoutStruct->pageBottom) {
                        layoutStruct->newPage();
                        pos.y = layoutStruct->y;

                        cd->position = pos;
                        cd->sizeDirty = true;

                        if (width != 0)
                            layoutFrame(c, layoutFrom, layoutTo, width, -1, layoutStruct->frameY);
                        else
                            layoutFrame(c, layoutFrom, layoutTo, layoutStruct->frameY);
                    }
                }

                // align only if there is space for alignment
                if (right - left > cd->size.width) {
                    if (align & Qt::AlignRight)
                        pos.x += layoutStruct->x_right - cd->size.width;
                    else if (align & Qt::AlignHCenter)
                        pos.x += (layoutStruct->x_right - cd->size.width) / 2;
                }

                cd->position = pos;

                layoutStruct->y += cd->size.height;
                const int page = layoutStruct->currentPage();
                layoutStruct->pageBottom = (page + 1) * layoutStruct->pageHeight - layoutStruct->pageBottomMargin;

                cd->layoutDirty = false;

                if (c->frameFormat().pageBreakPolicy() & QTextFormat::PageBreak_AlwaysAfter)
                    layoutStruct->newPage();
            } else {
                QRectF oldFrameRect(cd->position.toPointF(), cd->size.toSizeF());
                QRectF updateRect;

                if (cd->sizeDirty)
                    updateRect = layoutFrame(c, layoutFrom, layoutTo);

                positionFloat(c);

                // If the size was made dirty when the position was set, layout again
                if (cd->sizeDirty)
                    updateRect = layoutFrame(c, layoutFrom, layoutTo);

                QRectF frameRect(cd->position.toPointF(), cd->size.toSizeF());

                if (frameRect == oldFrameRect && updateRect.isValid())
                    updateRect.translate(cd->position.toPointF());
                else
                    updateRect = frameRect;

                layoutStruct->addUpdateRectForFloat(updateRect);
                if (oldFrameRect.isValid())
                    layoutStruct->addUpdateRectForFloat(oldFrameRect);
            }

            layoutStruct->minimumWidth = qMax(layoutStruct->minimumWidth, cd->minimumWidth);
            layoutStruct->maximumWidth = qMin(layoutStruct->maximumWidth, cd->maximumWidth);

            previousIt = it;
            ++it;
        } else {
            QTextFrame::Iterator lastIt;
            if (!previousIt.atEnd())
                lastIt = previousIt;
            previousIt = it;
            QTextBlock block = it.currentBlock();
            ++it;

            const QTextBlockFormat blockFormat = block.blockFormat();

            if (blockFormat.pageBreakPolicy() & QTextFormat::PageBreak_AlwaysBefore)
                layoutStruct->newPage();

            const QFixed origY = layoutStruct->y;
            const QFixed origPageBottom = layoutStruct->pageBottom;
            const QFixed origMaximumWidth = layoutStruct->maximumWidth;
            layoutStruct->maximumWidth = 0;

            const QTextBlockFormat *previousBlockFormatPtr = 0;
            if (lastIt.currentBlock().isValid())
                previousBlockFormatPtr = &previousBlockFormat;

            // layout and position child block
            layoutBlock(block, docPos, blockFormat, layoutStruct, layoutFrom, layoutTo, previousBlockFormatPtr);

            // detect whether we have any alignment in the document that disallows optimizations,
            // such as not laying out the document again in a textedit with wrapping disabled.
            if (inRootFrame && !(block.layout()->textOption().alignment() & Qt::AlignLeft))
                contentHasAlignment = true;

            // if the block right before a table is empty 'hide' it by
            // positioning it into the table border
            if (isEmptyBlockBeforeTable(block, blockFormat, it)) {
                const QTextBlock lastBlock = lastIt.currentBlock();
                const qreal lastBlockBottomMargin = lastBlock.isValid() ? lastBlock.blockFormat().bottomMargin() : 0.0f;
                layoutStruct->y = origY + QFixed::fromReal(qMax(lastBlockBottomMargin, block.blockFormat().topMargin()));
                layoutStruct->pageBottom = origPageBottom;
            } else {
                // if the block right after a table is empty then 'hide' it, too
                if (isEmptyBlockAfterTable(block, lastIt.currentFrame())) {
                    QTextTableData *td = static_cast<QTextTableData *>(data(lastIt.currentFrame()));
                    QTextLayout *layout = block.layout();

                    QPointF pos((td->position.x + td->size.width).toReal(),
                                (td->position.y + td->size.height).toReal() - layout->boundingRect().height());

                    layout->setPosition(pos);
                    layoutStruct->y = origY;
                    layoutStruct->pageBottom = origPageBottom;
                }

                // if the block right after a table starts with a line separator, shift it up by one line
                if (isLineSeparatorBlockAfterTable(block, lastIt.currentFrame())) {
                    QTextTableData *td = static_cast<QTextTableData *>(data(lastIt.currentFrame()));
                    QTextLayout *layout = block.layout();

                    QFixed height = QFixed::fromReal(layout->lineAt(0).height());

                    if (layoutStruct->pageBottom == origPageBottom) {
                        layoutStruct->y -= height;
                        layout->setPosition(layout->position() - QPointF(0, height.toReal()));
                    } else {
                        // relayout block to correctly handle page breaks
                        layoutStruct->y = origY - height;
                        layoutStruct->pageBottom = origPageBottom;
                        layoutBlock(block, docPos, blockFormat, layoutStruct, layoutFrom, layoutTo, previousBlockFormatPtr);
                    }

                    QPointF linePos((td->position.x + td->size.width).toReal(),
                                    (td->position.y + td->size.height - height).toReal());

                    layout->lineAt(0).setPosition(linePos - layout->position());
                }

                if (blockFormat.pageBreakPolicy() & QTextFormat::PageBreak_AlwaysAfter)
                    layoutStruct->newPage();
            }

            maximumBlockWidth = qMax(maximumBlockWidth, layoutStruct->maximumWidth);
            layoutStruct->maximumWidth = origMaximumWidth;
            previousBlockFormat = blockFormat;
        }
    }
    if (layoutStruct->maximumWidth == QFIXED_MAX && maximumBlockWidth > 0)
        layoutStruct->maximumWidth = maximumBlockWidth;
    else
        layoutStruct->maximumWidth = qMax(layoutStruct->maximumWidth, maximumBlockWidth);

    // a float at the bottom of a frame may make it taller, hence the qMax() for layoutStruct->y.
    // we don't need to do it for tables though because floats in tables are per table
    // and not per cell and layoutCell already takes care of doing the same as we do here
    if (!qobject_cast<QTextTable *>(layoutStruct->frame)) {
        QList<QTextFrame *> children = layoutStruct->frame->childFrames();
        for (int i = 0; i < children.count(); ++i) {
            QTextFrameData *fd = data(children.at(i));
            if (!fd->layoutDirty && children.at(i)->frameFormat().position() != QTextFrameFormat::InFlow)
                layoutStruct->y = qMax(layoutStruct->y, fd->position.y + fd->size.height);
        }
    }

    if (inRootFrame) {
        // we assume that any float is aligned in a way that disallows the optimizations that rely
        // on unaligned content.
        if (!fd->floats.isEmpty())
            contentHasAlignment = true;

        if (it.atEnd()) {
            //qDebug() << "layout done!";
            currentLazyLayoutPosition = -1;
            QCheckPoint cp;
            cp.y = layoutStruct->y;
            cp.positionInFrame = docPrivate->length();
            cp.minimumWidth = layoutStruct->minimumWidth;
            cp.maximumWidth = layoutStruct->maximumWidth;
            cp.contentsWidth = layoutStruct->contentsWidth;
            checkPoints.append(cp);
            checkPoints.reserve(checkPoints.size());
        } else {
            currentLazyLayoutPosition = checkPoints.last().positionInFrame;
            // #######
            //checkPoints.last().positionInFrame = q->document()->docHandle()->length();
        }
    }


    fd->currentLayoutStruct = 0;
}

static inline void getLineHeightParams(const QTextBlockFormat &blockFormat, const QTextLine &line, qreal scaling,
                                       QFixed *lineAdjustment, QFixed *lineBreakHeight, QFixed *lineHeight)
{
    *lineHeight = QFixed::fromReal(blockFormat.lineHeight(line.height(), scaling));

    if (blockFormat.lineHeightType() == QTextBlockFormat::FixedHeight || blockFormat.lineHeightType() == QTextBlockFormat::MinimumHeight) {
        *lineBreakHeight = *lineHeight;
        if (blockFormat.lineHeightType() == QTextBlockFormat::FixedHeight)
            *lineAdjustment = QFixed::fromReal(line.ascent() + qMax(line.leading(), qreal(0.0))) - ((*lineHeight * 4) / 5);
        else
            *lineAdjustment = QFixed::fromReal(line.height()) - *lineHeight;
    }
    else {
        *lineBreakHeight = QFixed::fromReal(line.height());
        *lineAdjustment = 0;
    }
}

void QTextDocumentLayoutPrivate::layoutBlock(const QTextBlock &bl, int blockPosition, const QTextBlockFormat &blockFormat,
                                             QTextLayoutStruct *layoutStruct, int layoutFrom, int layoutTo, const QTextBlockFormat *previousBlockFormat)
{
    Q_Q(QTextDocumentLayout);

    QTextLayout *tl = bl.layout();
    const int blockLength = bl.length();

    LDEBUG << "layoutBlock from=" << layoutFrom << "to=" << layoutTo;

//    qDebug() << "layoutBlock; width" << layoutStruct->x_right - layoutStruct->x_left << "(maxWidth is btw" << tl->maximumWidth() << ')';

    if (previousBlockFormat) {
        qreal margin = qMax(blockFormat.topMargin(), previousBlockFormat->bottomMargin());
        if (margin > 0 && q->paintDevice()) {
            margin *= qreal(q->paintDevice()->logicalDpiY()) / qreal(qt_defaultDpi());
        }
        layoutStruct->y += QFixed::fromReal(margin);
    }

    //QTextFrameData *fd = data(layoutStruct->frame);

    Qt::LayoutDirection dir = bl.textDirection();

    QFixed extraMargin;
    if (docPrivate->defaultTextOption.flags() & QTextOption::AddSpaceForLineAndParagraphSeparators) {
        QFontMetricsF fm(bl.charFormat().font());
        extraMargin = QFixed::fromReal(fm.width(QChar(QChar(0x21B5))));
    }

    const QFixed indent = this->blockIndent(blockFormat);
    const QFixed totalLeftMargin = QFixed::fromReal(blockFormat.leftMargin()) + (dir == Qt::RightToLeft ? extraMargin : indent);
    const QFixed totalRightMargin = QFixed::fromReal(blockFormat.rightMargin()) + (dir == Qt::RightToLeft ? indent : extraMargin);

    const QPointF oldPosition = tl->position();
    tl->setPosition(QPointF(layoutStruct->x_left.toReal(), layoutStruct->y.toReal()));

    if (layoutStruct->fullLayout
        || (blockPosition + blockLength > layoutFrom && blockPosition <= layoutTo)
        // force relayout if we cross a page boundary
        || (layoutStruct->pageHeight != QFIXED_MAX && layoutStruct->absoluteY() + QFixed::fromReal(tl->boundingRect().height()) > layoutStruct->pageBottom)) {

        LDEBUG << " do layout";
        QTextOption option = docPrivate->defaultTextOption;
        option.setTextDirection(dir);
        option.setTabs( blockFormat.tabPositions() );

        Qt::Alignment align = docPrivate->defaultTextOption.alignment();
        if (blockFormat.hasProperty(QTextFormat::BlockAlignment))
            align = blockFormat.alignment();
        option.setAlignment(QStyle::visualAlignment(dir, align)); // for paragraph that are RTL, alignment is auto-reversed;

        if (blockFormat.nonBreakableLines() || document->pageSize().width() < 0) {
            option.setWrapMode(QTextOption::ManualWrap);
        }

        tl->setTextOption(option);

        const bool haveWordOrAnyWrapMode = (option.wrapMode() == QTextOption::WrapAtWordBoundaryOrAnywhere);

//         qDebug() << "    layouting block at" << bl.position();
        const QFixed cy = layoutStruct->y;
        const QFixed l = layoutStruct->x_left  + totalLeftMargin;
        const QFixed r = layoutStruct->x_right - totalRightMargin;

        tl->beginLayout();
        bool firstLine = true;
        while (1) {
            QTextLine line = tl->createLine();
            if (!line.isValid())
                break;
            line.setLeadingIncluded(true);

            QFixed left, right;
            floatMargins(layoutStruct->y, layoutStruct, &left, &right);
            left = qMax(left, l);
            right = qMin(right, r);
            QFixed text_indent;
            if (firstLine) {
                text_indent = QFixed::fromReal(blockFormat.textIndent());
                if (dir == Qt::LeftToRight)
                    left += text_indent;
                else
                    right -= text_indent;
                firstLine = false;
            }
//         qDebug() << "layout line y=" << currentYPos << "left=" << left << "right=" <<right;

            if (fixedColumnWidth != -1)
                line.setNumColumns(fixedColumnWidth, (right - left).toReal());
            else
                line.setLineWidth((right - left).toReal());

//        qDebug() << "layoutBlock; layouting line with width" << right - left << "->textWidth" << line.textWidth();
            floatMargins(layoutStruct->y, layoutStruct, &left, &right);
            left = qMax(left, l);
            right = qMin(right, r);
            if (dir == Qt::LeftToRight)
                left += text_indent;
            else
                right -= text_indent;

            if (fixedColumnWidth == -1 && QFixed::fromReal(line.naturalTextWidth()) > right-left) {
                // float has been added in the meantime, redo
                layoutStruct->pendingFloats.clear();

                line.setLineWidth((right-left).toReal());
                if (QFixed::fromReal(line.naturalTextWidth()) > right-left) {
                    if (haveWordOrAnyWrapMode) {
                        option.setWrapMode(QTextOption::WrapAnywhere);
                        tl->setTextOption(option);
                    }

                    layoutStruct->pendingFloats.clear();
                    // lines min width more than what we have
                    layoutStruct->y = findY(layoutStruct->y, layoutStruct, QFixed::fromReal(line.naturalTextWidth()));
                    floatMargins(layoutStruct->y, layoutStruct, &left, &right);
                    left = qMax(left, l);
                    right = qMin(right, r);
                    if (dir == Qt::LeftToRight)
                        left += text_indent;
                    else
                        right -= text_indent;
                    line.setLineWidth(qMax<qreal>(line.naturalTextWidth(), (right-left).toReal()));

                    if (haveWordOrAnyWrapMode) {
                        option.setWrapMode(QTextOption::WordWrap);
                        tl->setTextOption(option);
                    }
                }

            }

            QFixed lineBreakHeight, lineHeight, lineAdjustment;
            qreal scaling = (q->paintDevice() && q->paintDevice()->logicalDpiY() != qt_defaultDpi()) ?
                            qreal(q->paintDevice()->logicalDpiY()) / qreal(qt_defaultDpi()) : 1;
            getLineHeightParams(blockFormat, line, scaling, &lineAdjustment, &lineBreakHeight, &lineHeight);

            if (layoutStruct->pageHeight > 0 && layoutStruct->absoluteY() + lineBreakHeight > layoutStruct->pageBottom) {
                layoutStruct->newPage();

                floatMargins(layoutStruct->y, layoutStruct, &left, &right);
                left = qMax(left, l);
                right = qMin(right, r);
                if (dir == Qt::LeftToRight)
                    left += text_indent;
                else
                    right -= text_indent;
            }

            line.setPosition(QPointF((left - layoutStruct->x_left).toReal(), (layoutStruct->y - cy - lineAdjustment).toReal()));
            layoutStruct->y += lineHeight;
            layoutStruct->contentsWidth
                = qMax<QFixed>(layoutStruct->contentsWidth, QFixed::fromReal(line.x() + line.naturalTextWidth()) + totalRightMargin);

            // position floats
            for (int i = 0; i < layoutStruct->pendingFloats.size(); ++i) {
                QTextFrame *f = layoutStruct->pendingFloats.at(i);
                positionFloat(f);
            }
            layoutStruct->pendingFloats.clear();
        }
        tl->endLayout();
    } else {
        const int cnt = tl->lineCount();
        for (int i = 0; i < cnt; ++i) {
            LDEBUG << "going to move text line" << i;
            QTextLine line = tl->lineAt(i);
            layoutStruct->contentsWidth
                = qMax(layoutStruct->contentsWidth, QFixed::fromReal(line.x() + tl->lineAt(i).naturalTextWidth()) + totalRightMargin);

            QFixed lineBreakHeight, lineHeight, lineAdjustment;
            qreal scaling = (q->paintDevice() && q->paintDevice()->logicalDpiY() != qt_defaultDpi()) ?
                            qreal(q->paintDevice()->logicalDpiY()) / qreal(qt_defaultDpi()) : 1;
            getLineHeightParams(blockFormat, line, scaling, &lineAdjustment, &lineBreakHeight, &lineHeight);

            if (layoutStruct->pageHeight != QFIXED_MAX) {
                if (layoutStruct->absoluteY() + lineBreakHeight > layoutStruct->pageBottom)
                    layoutStruct->newPage();
                line.setPosition(QPointF(line.position().x(), (layoutStruct->y - lineAdjustment).toReal() - tl->position().y()));
            }
            layoutStruct->y += lineHeight;
        }
        if (layoutStruct->updateRect.isValid()
            && blockLength > 1) {
            if (layoutFrom >= blockPosition + blockLength) {
                // if our height didn't change and the change in the document is
                // in one of the later paragraphs, then we don't need to repaint
                // this one
                layoutStruct->updateRect.setTop(qMax(layoutStruct->updateRect.top(), layoutStruct->y.toReal()));
            } else if (layoutTo < blockPosition) {
                if (oldPosition == tl->position())
                    // if the change in the document happened earlier in the document
                    // and our position did /not/ change because none of the earlier paragraphs
                    // or frames changed their height, then we don't need to repaint
                    // this one
                    layoutStruct->updateRect.setBottom(qMin(layoutStruct->updateRect.bottom(), tl->position().y()));
                else
                    layoutStruct->updateRect.setBottom(qreal(INT_MAX)); // reset
            }
        }
    }

    // ### doesn't take floats into account. would need to do it per line. but how to retrieve then? (Simon)
    const QFixed margins = totalLeftMargin + totalRightMargin;
    layoutStruct->minimumWidth = qMax(layoutStruct->minimumWidth, QFixed::fromReal(tl->minimumWidth()) + margins);

    const QFixed maxW = QFixed::fromReal(tl->maximumWidth()) + margins;

    if (maxW > 0) {
        if (layoutStruct->maximumWidth == QFIXED_MAX)
            layoutStruct->maximumWidth = maxW;
        else
            layoutStruct->maximumWidth = qMax(layoutStruct->maximumWidth, maxW);
    }
}

void QTextDocumentLayoutPrivate::floatMargins(const QFixed &y, const QTextLayoutStruct *layoutStruct,
                                              QFixed *left, QFixed *right) const
{
//     qDebug() << "floatMargins y=" << y;
    *left = layoutStruct->x_left;
    *right = layoutStruct->x_right;
    QTextFrameData *lfd = data(layoutStruct->frame);
    for (int i = 0; i < lfd->floats.size(); ++i) {
        QTextFrameData *fd = data(lfd->floats.at(i));
        if (!fd->layoutDirty) {
            if (fd->position.y <= y && fd->position.y + fd->size.height > y) {
//                 qDebug() << "adjusting with float" << f << fd->position.x()<< fd->size.width();
                if (lfd->floats.at(i)->frameFormat().position() == QTextFrameFormat::FloatLeft)
                    *left = qMax(*left, fd->position.x + fd->size.width);
                else
                    *right = qMin(*right, fd->position.x);
            }
        }
    }
//     qDebug() << "floatMargins: left="<<*left<<"right="<<*right<<"y="<<y;
}

QFixed QTextDocumentLayoutPrivate::findY(QFixed yFrom, const QTextLayoutStruct *layoutStruct, QFixed requiredWidth) const
{
    QFixed right, left;
    requiredWidth = qMin(requiredWidth, layoutStruct->x_right - layoutStruct->x_left);

//     qDebug() << "findY:" << yFrom;
    while (1) {
        floatMargins(yFrom, layoutStruct, &left, &right);
//         qDebug() << "    yFrom=" << yFrom<<"right=" << right << "left=" << left << "requiredWidth=" << requiredWidth;
        if (right-left >= requiredWidth)
            break;

        // move float down until we find enough space
        QFixed newY = QFIXED_MAX;
        QTextFrameData *lfd = data(layoutStruct->frame);
        for (int i = 0; i < lfd->floats.size(); ++i) {
            QTextFrameData *fd = data(lfd->floats.at(i));
            if (!fd->layoutDirty) {
                if (fd->position.y <= yFrom && fd->position.y + fd->size.height > yFrom)
                    newY = qMin(newY, fd->position.y + fd->size.height);
            }
        }
        if (newY == QFIXED_MAX)
            break;
        yFrom = newY;
    }
    return yFrom;
}

QTextDocumentLayout::QTextDocumentLayout(QTextDocument *doc)
    : QAbstractTextDocumentLayout(*new QTextDocumentLayoutPrivate, doc)
{
    registerHandler(QTextFormat::ImageObject, new QTextImageHandler(this));
}


void QTextDocumentLayout::draw(QPainter *painter, const PaintContext &context)
{
    Q_D(QTextDocumentLayout);
    QTextFrame *frame = d->document->rootFrame();
    QTextFrameData *fd = data(frame);

    if(fd->sizeDirty)
        return;

    if (context.clip.isValid()) {
        d->ensureLayouted(QFixed::fromReal(context.clip.bottom()));
    } else {
        d->ensureLayoutFinished();
    }

    QFixed width = fd->size.width;
    if (d->document->pageSize().width() == 0 && d->viewportRect.isValid()) {
        // we're in NoWrap mode, meaning the frame should expand to the viewport
        // so that backgrounds are drawn correctly
        fd->size.width = qMax(width, QFixed::fromReal(d->viewportRect.right()));
    }

    // Make sure we conform to the root frames bounds when drawing.
    d->clipRect = QRectF(fd->position.toPointF(), fd->size.toSizeF()).adjusted(fd->leftMargin.toReal(), 0, -fd->rightMargin.toReal(), 0);
    d->drawFrame(QPointF(), painter, context, frame);
    fd->size.width = width;
}

void QTextDocumentLayout::setViewport(const QRectF &viewport)
{
    Q_D(QTextDocumentLayout);
    d->viewportRect = viewport;
}

static void markFrames(QTextFrame *current, int from, int oldLength, int length)
{
    int end = qMax(oldLength, length) + from;

    if (current->firstPosition() >= end || current->lastPosition() < from)
        return;

    QTextFrameData *fd = data(current);
    for (int i = 0; i < fd->floats.size(); ++i) {
        QTextFrame *f = fd->floats[i];
        if (!f) {
            // float got removed in editing operation
            fd->floats.removeAt(i);
            --i;
        }
    }

    fd->layoutDirty = true;
    fd->sizeDirty = true;

//     qDebug("    marking frame (%d--%d) as dirty", current->firstPosition(), current->lastPosition());
    QList<QTextFrame *> children = current->childFrames();
    for (int i = 0; i < children.size(); ++i)
        markFrames(children.at(i), from, oldLength, length);
}

void QTextDocumentLayout::documentChanged(int from, int oldLength, int length)
{
    Q_D(QTextDocumentLayout);

    QTextBlock blockIt = document()->findBlock(from);
    QTextBlock endIt = document()->findBlock(qMax(0, from + length - 1));
    if (endIt.isValid())
        endIt = endIt.next();
     for (; blockIt.isValid() && blockIt != endIt; blockIt = blockIt.next())
         blockIt.clearLayout();

    if (d->docPrivate->pageSize.isNull())
        return;

    QRectF updateRect;

    d->lazyLayoutStepSize = 1000;
    d->sizeChangedTimer.stop();
    d->insideDocumentChange = true;

    const int documentLength = d->docPrivate->length();
    const bool fullLayout = (oldLength == 0 && length == documentLength);
    const bool smallChange = documentLength > 0
                             && (qMax(length, oldLength) * 100 / documentLength) < 5;

    // don't show incremental layout progress (avoid scroll bar flicker)
    // if we see only a small change in the document and we're either starting
    // a layout run or we're already in progress for that and we haven't seen
    // any bigger change previously (showLayoutProgress already false)
    if (smallChange
        && (d->currentLazyLayoutPosition == -1 || d->showLayoutProgress == false))
        d->showLayoutProgress = false;
    else
        d->showLayoutProgress = true;

    if (fullLayout) {
        d->contentHasAlignment = false;
        d->currentLazyLayoutPosition = 0;
        d->checkPoints.clear();
        d->layoutStep();
    } else {
        d->ensureLayoutedByPosition(from);
        updateRect = doLayout(from, oldLength, length);
    }

    if (!d->layoutTimer.isActive() && d->currentLazyLayoutPosition != -1)
        d->layoutTimer.start(10, this);

    d->insideDocumentChange = false;

    if (d->showLayoutProgress) {
        const QSizeF newSize = dynamicDocumentSize();
        if (newSize != d->lastReportedSize) {
            d->lastReportedSize = newSize;
            emit documentSizeChanged(newSize);
        }
    }

    if (!updateRect.isValid()) {
        // don't use the frame size, it might have shrunken
        updateRect = QRectF(QPointF(0, 0), QSizeF(qreal(INT_MAX), qreal(INT_MAX)));
    }

    emit update(updateRect);
}

QRectF QTextDocumentLayout::doLayout(int from, int oldLength, int length)
{
    Q_D(QTextDocumentLayout);

//     qDebug("documentChange: from=%d, oldLength=%d, length=%d", from, oldLength, length);

    // mark all frames between f_start and f_end as dirty
    markFrames(d->docPrivate->rootFrame(), from, oldLength, length);

    QRectF updateRect;

    QTextFrame *root = d->docPrivate->rootFrame();
    if(data(root)->sizeDirty)
        updateRect = d->layoutFrame(root, from, from + length);
    data(root)->layoutDirty = false;

    if (d->currentLazyLayoutPosition == -1)
        layoutFinished();
    else if (d->showLayoutProgress)
        d->sizeChangedTimer.start(0, this);

    return updateRect;
}

int QTextDocumentLayout::hitTest(const QPointF &point, Qt::HitTestAccuracy accuracy) const
{
    Q_D(const QTextDocumentLayout);
    d->ensureLayouted(QFixed::fromReal(point.y()));
    QTextFrame *f = d->docPrivate->rootFrame();
    int position = 0;
    QTextLayout *l = 0;
    QFixedPoint pointf;
    pointf.x = QFixed::fromReal(point.x());
    pointf.y = QFixed::fromReal(point.y());
    QTextDocumentLayoutPrivate::HitPoint p = d->hitTest(f, pointf, &position, &l, accuracy);
    if (accuracy == Qt::ExactHit && p < QTextDocumentLayoutPrivate::PointExact)
        return -1;

    // ensure we stay within document bounds
    int lastPos = f->lastPosition();
    if (l && !l->preeditAreaText().isEmpty())
        lastPos += l->preeditAreaText().length();
    if (position > lastPos)
        position = lastPos;
    else if (position < 0)
        position = 0;

    return position;
}

void QTextDocumentLayout::resizeInlineObject(QTextInlineObject item, int posInDocument, const QTextFormat &format)
{
    Q_D(QTextDocumentLayout);
    QTextCharFormat f = format.toCharFormat();
    Q_ASSERT(f.isValid());
    QTextObjectHandler handler = d->handlers.value(f.objectType());
    if (!handler.component)
        return;

    QSizeF intrinsic = handler.iface->intrinsicSize(d->document, posInDocument, format);

    QTextFrameFormat::Position pos = QTextFrameFormat::InFlow;
    QTextFrame *frame = qobject_cast<QTextFrame *>(d->document->objectForFormat(f));
    if (frame) {
        pos = frame->frameFormat().position();
        QTextFrameData *fd = data(frame);
        fd->sizeDirty = false;
        fd->size = QFixedSize::fromSizeF(intrinsic);
        fd->minimumWidth = fd->maximumWidth = fd->size.width;
    }

    QSizeF inlineSize = (pos == QTextFrameFormat::InFlow ? intrinsic : QSizeF(0, 0));
    item.setWidth(inlineSize.width());

    QFontMetrics m(f.font());
    switch (f.verticalAlignment())
    {
    case QTextCharFormat::AlignMiddle:
        item.setDescent(inlineSize.height() / 2);
        item.setAscent(inlineSize.height() / 2 - 1);
        break;
    case QTextCharFormat::AlignBaseline:
        item.setDescent(m.descent());
        item.setAscent(inlineSize.height() - m.descent() - 1);
        break;
    default:
        item.setDescent(0);
        item.setAscent(inlineSize.height() - 1);
    }
}

void QTextDocumentLayout::positionInlineObject(QTextInlineObject item, int posInDocument, const QTextFormat &format)
{
    Q_D(QTextDocumentLayout);
    Q_UNUSED(posInDocument);
    if (item.width() != 0)
        // inline
        return;

    QTextCharFormat f = format.toCharFormat();
    Q_ASSERT(f.isValid());
    QTextObjectHandler handler = d->handlers.value(f.objectType());
    if (!handler.component)
        return;

    QTextFrame *frame = qobject_cast<QTextFrame *>(d->document->objectForFormat(f));
    if (!frame)
        return;

    QTextBlock b = d->document->findBlock(frame->firstPosition());
    QTextLine line;
    if (b.position() <= frame->firstPosition() && b.position() + b.length() > frame->lastPosition())
        line = b.layout()->lineAt(b.layout()->lineCount()-1);
//     qDebug() << "layoutObject: line.isValid" << line.isValid() << b.position() << b.length() <<
//         frame->firstPosition() << frame->lastPosition();
    d->positionFloat(frame, line.isValid() ? &line : 0);
}

void QTextDocumentLayout::drawInlineObject(QPainter *p, const QRectF &rect, QTextInlineObject item,
                                           int posInDocument, const QTextFormat &format)
{
    Q_D(QTextDocumentLayout);
    QTextCharFormat f = format.toCharFormat();
    Q_ASSERT(f.isValid());
    QTextFrame *frame = qobject_cast<QTextFrame *>(d->document->objectForFormat(f));
    if (frame && frame->frameFormat().position() != QTextFrameFormat::InFlow)
        return; // don't draw floating frames from inline objects here but in drawFlow instead

//    qDebug() << "drawObject at" << r;
    QAbstractTextDocumentLayout::drawInlineObject(p, rect, item, posInDocument, format);
}

int QTextDocumentLayout::dynamicPageCount() const
{
    Q_D(const QTextDocumentLayout);
    const QSizeF pgSize = d->document->pageSize();
    if (pgSize.height() < 0)
        return 1;
    return qCeil(dynamicDocumentSize().height() / pgSize.height());
}

QSizeF QTextDocumentLayout::dynamicDocumentSize() const
{
    Q_D(const QTextDocumentLayout);
    return data(d->docPrivate->rootFrame())->size.toSizeF();
}

int QTextDocumentLayout::pageCount() const
{
    Q_D(const QTextDocumentLayout);
    d->ensureLayoutFinished();
    return dynamicPageCount();
}

QSizeF QTextDocumentLayout::documentSize() const
{
    Q_D(const QTextDocumentLayout);
    d->ensureLayoutFinished();
    return dynamicDocumentSize();
}

void QTextDocumentLayoutPrivate::ensureLayouted(QFixed y) const
{
    Q_Q(const QTextDocumentLayout);
    if (currentLazyLayoutPosition == -1)
        return;
    const QSizeF oldSize = q->dynamicDocumentSize();
    Q_UNUSED(oldSize);

    if (checkPoints.isEmpty())
        layoutStep();

    while (currentLazyLayoutPosition != -1
           && checkPoints.last().y < y)
        layoutStep();
}

void QTextDocumentLayoutPrivate::ensureLayoutedByPosition(int position) const
{
    if (currentLazyLayoutPosition == -1)
        return;
    if (position < currentLazyLayoutPosition)
        return;
    while (currentLazyLayoutPosition != -1
           && currentLazyLayoutPosition < position) {
        const_cast<QTextDocumentLayout *>(q_func())->doLayout(currentLazyLayoutPosition, 0, INT_MAX - currentLazyLayoutPosition);
    }
}

void QTextDocumentLayoutPrivate::layoutStep() const
{
    ensureLayoutedByPosition(currentLazyLayoutPosition + lazyLayoutStepSize);
    lazyLayoutStepSize = qMin(200000, lazyLayoutStepSize * 2);
}

void QTextDocumentLayout::setCursorWidth(int width)
{
    Q_D(QTextDocumentLayout);
    d->cursorWidth = width;
}

int QTextDocumentLayout::cursorWidth() const
{
    Q_D(const QTextDocumentLayout);
    return d->cursorWidth;
}

void QTextDocumentLayout::setFixedColumnWidth(int width)
{
    Q_D(QTextDocumentLayout);
    d->fixedColumnWidth = width;
}

QRectF QTextDocumentLayout::frameBoundingRect(QTextFrame *frame) const
{
    Q_D(const QTextDocumentLayout);
    if (d->docPrivate->pageSize.isNull())
        return QRectF();
    d->ensureLayoutFinished();
    return d->frameBoundingRectInternal(frame);
}

QRectF QTextDocumentLayoutPrivate::frameBoundingRectInternal(QTextFrame *frame) const
{
    QPointF pos;
    const int framePos = frame->firstPosition();
    QTextFrame *f = frame;
    while (f) {
        QTextFrameData *fd = data(f);
        pos += fd->position.toPointF();

        if (QTextTable *table = qobject_cast<QTextTable *>(f)) {
            QTextTableCell cell = table->cellAt(framePos);
            if (cell.isValid())
                pos += static_cast<QTextTableData *>(fd)->cellPosition(cell).toPointF();
        }

        f = f->parentFrame();
    }
    return QRectF(pos, data(frame)->size.toSizeF());
}

QRectF QTextDocumentLayout::blockBoundingRect(const QTextBlock &block) const
{
    Q_D(const QTextDocumentLayout);
    if (d->docPrivate->pageSize.isNull() || !block.isValid())
        return QRectF();
    d->ensureLayoutedByPosition(block.position() + block.length());
    QTextFrame *frame = d->document->frameAt(block.position());
    QPointF offset;
    const int blockPos = block.position();

    while (frame) {
        QTextFrameData *fd = data(frame);
        offset += fd->position.toPointF();

        if (QTextTable *table = qobject_cast<QTextTable *>(frame)) {
            QTextTableCell cell = table->cellAt(blockPos);
            if (cell.isValid())
                offset += static_cast<QTextTableData *>(fd)->cellPosition(cell).toPointF();
        }

        frame = frame->parentFrame();
    }

    const QTextLayout *layout = block.layout();
    QRectF rect = layout->boundingRect();
    rect.moveTopLeft(layout->position() + offset);
    return rect;
}

int QTextDocumentLayout::layoutStatus() const
{
    Q_D(const QTextDocumentLayout);
    int pos = d->currentLazyLayoutPosition;
    if (pos == -1)
        return 100;
    return pos * 100 / d->document->docHandle()->length();
}

void QTextDocumentLayout::timerEvent(QTimerEvent *e)
{
    Q_D(QTextDocumentLayout);
    if (e->timerId() == d->layoutTimer.timerId()) {
        if (d->currentLazyLayoutPosition != -1)
            d->layoutStep();
    } else if (e->timerId() == d->sizeChangedTimer.timerId()) {
        d->lastReportedSize = dynamicDocumentSize();
        emit documentSizeChanged(d->lastReportedSize);
        d->sizeChangedTimer.stop();

        if (d->currentLazyLayoutPosition == -1) {
            const int newCount = dynamicPageCount();
            if (newCount != d->lastPageCount) {
                d->lastPageCount = newCount;
                emit pageCountChanged(newCount);
            }
        }
    } else {
        QAbstractTextDocumentLayout::timerEvent(e);
    }
}

void QTextDocumentLayout::layoutFinished()
{
    Q_D(QTextDocumentLayout);
    d->layoutTimer.stop();
    if (!d->insideDocumentChange)
        d->sizeChangedTimer.start(0, this);
    // reset
    d->showLayoutProgress = true;
}

void QTextDocumentLayout::ensureLayouted(qreal y)
{
    d_func()->ensureLayouted(QFixed::fromReal(y));
}

qreal QTextDocumentLayout::idealWidth() const
{
    Q_D(const QTextDocumentLayout);
    d->ensureLayoutFinished();
    return d->idealWidth;
}

bool QTextDocumentLayout::contentHasAlignment() const
{
    Q_D(const QTextDocumentLayout);
    return d->contentHasAlignment;
}

qreal QTextDocumentLayoutPrivate::scaleToDevice(qreal value) const
{
    if (!paintDevice)
        return value;
    return value * paintDevice->logicalDpiY() / qreal(qt_defaultDpi());
}

QFixed QTextDocumentLayoutPrivate::scaleToDevice(QFixed value) const
{
    if (!paintDevice)
        return value;
    return value * QFixed(paintDevice->logicalDpiY()) / QFixed(qt_defaultDpi());
}

QT_END_NAMESPACE

#include "moc_qtextdocumentlayout_p.cpp"
