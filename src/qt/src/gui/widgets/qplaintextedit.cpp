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

#include "qplaintextedit_p.h"


#include <qfont.h>
#include <qpainter.h>
#include <qevent.h>
#include <qdebug.h>
#include <qmime.h>
#include <qdrag.h>
#include <qclipboard.h>
#include <qmenu.h>
#include <qstyle.h>
#include <qtimer.h>
#include "private/qtextdocumentlayout_p.h"
#include "private/qabstracttextdocumentlayout_p.h"
#include "qtextdocument.h"
#include "private/qtextdocument_p.h"
#include "qtextlist.h"
#include "private/qtextcontrol_p.h"
#include "qaccessible.h"

#include <qtextformat.h>
#include <qdatetime.h>
#include <qapplication.h>
#include <limits.h>
#include <qtexttable.h>
#include <qvariant.h>
#include <qinputcontext.h>

#ifndef QT_NO_TEXTEDIT

QT_BEGIN_NAMESPACE

static inline bool shouldEnableInputMethod(QPlainTextEdit *plaintextedit)
{
    return !plaintextedit->isReadOnly();
}

class QPlainTextDocumentLayoutPrivate : public QAbstractTextDocumentLayoutPrivate
{
    Q_DECLARE_PUBLIC(QPlainTextDocumentLayout)
public:
    QPlainTextDocumentLayoutPrivate() {
        mainViewPrivate = 0;
        width = 0;
        maximumWidth = 0;
        maximumWidthBlockNumber = 0;
        blockCount = 1;
        blockUpdate = blockDocumentSizeChanged = false;
        cursorWidth = 1;
        textLayoutFlags = 0;
    }

    qreal width;
    qreal maximumWidth;
    int maximumWidthBlockNumber;
    int blockCount;
    QPlainTextEditPrivate *mainViewPrivate;
    bool blockUpdate;
    bool blockDocumentSizeChanged;
    int cursorWidth;
    int textLayoutFlags;

    void layoutBlock(const QTextBlock &block);
    qreal blockWidth(const QTextBlock &block);

    void relayout();
};



/*! \class QPlainTextDocumentLayout
    \since 4.4
    \brief The QPlainTextDocumentLayout class implements a plain text layout for QTextDocument

    \ingroup richtext-processing

   A QPlainTextDocumentLayout is required for text documents that can
   be display or edited in a QPlainTextEdit. See
   QTextDocument::setDocumentLayout().

   QPlainTextDocumentLayout uses the QAbstractTextDocumentLayout API
   that QTextDocument requires, but redefines it partially in order to
   support plain text better. For instances, it does not operate on
   vertical pixels, but on paragraphs (called blocks) instead. The
   height of a document is identical to the number of paragraphs it
   contains. The layout also doesn't support tables or nested frames,
   or any sort of advanced text layout that goes beyond a list of
   paragraphs with syntax highlighting.

*/



/*!
  Constructs a plain text document layout for the text \a document.
 */
QPlainTextDocumentLayout::QPlainTextDocumentLayout(QTextDocument *document)
    :QAbstractTextDocumentLayout(* new QPlainTextDocumentLayoutPrivate, document) {
}
/*!
  Destructs a plain text document layout.
 */
QPlainTextDocumentLayout::~QPlainTextDocumentLayout() {}


/*!
  \reimp
 */
void QPlainTextDocumentLayout::draw(QPainter *, const PaintContext &)
{
}

/*!
  \reimp
 */
int QPlainTextDocumentLayout::hitTest(const QPointF &, Qt::HitTestAccuracy ) const
{
//     this function is used from
//     QAbstractTextDocumentLayout::anchorAt(), but is not
//     implementable in a plain text document layout, because the
//     layout depends on the top block and top line which depends on
//     the view
    return -1;
}

/*!
  \reimp
 */
int QPlainTextDocumentLayout::pageCount() const
{ return 1; }

/*!
  \reimp
 */
QSizeF QPlainTextDocumentLayout::documentSize() const
{
    Q_D(const QPlainTextDocumentLayout);
    return QSizeF(d->maximumWidth, document()->lineCount());
}

/*!
  \reimp
 */
QRectF QPlainTextDocumentLayout::frameBoundingRect(QTextFrame *) const
{
    Q_D(const QPlainTextDocumentLayout);
    return QRectF(0, 0, qMax(d->width, d->maximumWidth), qreal(INT_MAX));
}

/*!
  \reimp
 */
QRectF QPlainTextDocumentLayout::blockBoundingRect(const QTextBlock &block) const
{
    if (!block.isValid()) { return QRectF(); }
    QTextLayout *tl = block.layout();
    if (!tl->lineCount())
        const_cast<QPlainTextDocumentLayout*>(this)->layoutBlock(block);
    QRectF br;
    if (block.isVisible()) {
        br = QRectF(QPointF(0, 0), tl->boundingRect().bottomRight());
        if (tl->lineCount() == 1)
            br.setWidth(qMax(br.width(), tl->lineAt(0).naturalTextWidth()));
        qreal margin = document()->documentMargin();
        br.adjust(0, 0, margin, 0);
        if (!block.next().isValid())
            br.adjust(0, 0, 0, margin);
    }
    return br;

}

/*!
  Ensures that \a block has a valid layout
 */
void QPlainTextDocumentLayout::ensureBlockLayout(const QTextBlock &block) const
{
    if (!block.isValid())
        return;
    QTextLayout *tl = block.layout();
    if (!tl->lineCount())
        const_cast<QPlainTextDocumentLayout*>(this)->layoutBlock(block);
}


/*! \property QPlainTextDocumentLayout::cursorWidth

    This property specifies the width of the cursor in pixels. The default value is 1.
*/
void QPlainTextDocumentLayout::setCursorWidth(int width)
{
    Q_D(QPlainTextDocumentLayout);
    d->cursorWidth = width;
}

int QPlainTextDocumentLayout::cursorWidth() const
{
    Q_D(const QPlainTextDocumentLayout);
    return d->cursorWidth;
}

QPlainTextDocumentLayoutPrivate *QPlainTextDocumentLayout::priv() const
{
    Q_D(const QPlainTextDocumentLayout);
    return const_cast<QPlainTextDocumentLayoutPrivate*>(d);
}


/*!

   Requests a complete update on all views.
 */
void QPlainTextDocumentLayout::requestUpdate()
{
    emit update(QRectF(0., -document()->documentMargin(), 1000000000., 1000000000.));
}


void QPlainTextDocumentLayout::setTextWidth(qreal newWidth)
{
    Q_D(QPlainTextDocumentLayout);
    d->width = d->maximumWidth = newWidth;
    d->relayout();
}

qreal QPlainTextDocumentLayout::textWidth() const
{
    Q_D(const QPlainTextDocumentLayout);
    return d->width;
}

void QPlainTextDocumentLayoutPrivate::relayout()
{
    Q_Q(QPlainTextDocumentLayout);
    QTextBlock block = q->document()->firstBlock();
    while (block.isValid()) {
        block.layout()->clearLayout();
        block.setLineCount(block.isVisible() ? 1 : 0);
        block = block.next();
    }
    emit q->update();
}


/*! \reimp
 */
void QPlainTextDocumentLayout::documentChanged(int from, int /*charsRemoved*/, int charsAdded)
{
    Q_D(QPlainTextDocumentLayout);
    QTextDocument *doc = document();
    int newBlockCount = doc->blockCount();

    QTextBlock changeStartBlock = doc->findBlock(from);
    QTextBlock changeEndBlock = doc->findBlock(qMax(0, from + charsAdded - 1));

    if (changeStartBlock == changeEndBlock && newBlockCount == d->blockCount) {
        QTextBlock block = changeStartBlock;
        int blockLineCount = block.layout()->lineCount();
        if (block.isValid() && blockLineCount) {
            QRectF oldBr = blockBoundingRect(block);
            layoutBlock(block);
            QRectF newBr = blockBoundingRect(block);
            if (newBr.height() == oldBr.height()) {
                if (!d->blockUpdate)
                    emit updateBlock(block);
                return;
            }
        }
    } else {
        QTextBlock block = changeStartBlock;
        do {
            block.clearLayout();
            if (block == changeEndBlock)
                break;
            block = block.next();
        } while(block.isValid());
    }

    if (newBlockCount != d->blockCount) {

        int changeEnd = changeEndBlock.blockNumber();
        int blockDiff = newBlockCount - d->blockCount;
        int oldChangeEnd = changeEnd - blockDiff;

        if (d->maximumWidthBlockNumber > oldChangeEnd)
            d->maximumWidthBlockNumber += blockDiff;

        d->blockCount = newBlockCount;
        if (d->blockCount == 1)
            d->maximumWidth = blockWidth(doc->firstBlock());

        if (!d->blockDocumentSizeChanged)
            emit documentSizeChanged(documentSize());

        if (blockDiff == 1 && changeEnd == newBlockCount -1 ) {
            if (!d->blockUpdate) {
                QTextBlock b = changeStartBlock;
                for(;;) {
                    emit updateBlock(b);
                    if (b == changeEndBlock)
                        break;
                    b = b.next();
                }
            }
            return;
        }
    }

    if (!d->blockUpdate)
	emit update(QRectF(0., -doc->documentMargin(), 1000000000., 1000000000.)); // optimization potential
}


void QPlainTextDocumentLayout::layoutBlock(const QTextBlock &block)
{
    Q_D(QPlainTextDocumentLayout);
    QTextDocument *doc = document();
    qreal margin = doc->documentMargin();
    qreal blockMaximumWidth = 0;

    qreal height = 0;
    QTextLayout *tl = block.layout();
    QTextOption option = doc->defaultTextOption();
    tl->setTextOption(option);

    int extraMargin = 0;
    if (option.flags() & QTextOption::AddSpaceForLineAndParagraphSeparators) {
        QFontMetrics fm(block.charFormat().font());
        extraMargin += fm.width(QChar(0x21B5));
    }
    tl->beginLayout();
    qreal availableWidth = d->width;
    if (availableWidth <= 0) {
        availableWidth = qreal(INT_MAX); // similar to text edit with pageSize.width == 0
    }
    availableWidth -= 2*margin + extraMargin;
    while (1) {
        QTextLine line = tl->createLine();
        if (!line.isValid())
            break;
        line.setLeadingIncluded(true);
        line.setLineWidth(availableWidth);
        line.setPosition(QPointF(margin, height));
        height += line.height();
        blockMaximumWidth = qMax(blockMaximumWidth, line.naturalTextWidth() + 2*margin);
    }
    tl->endLayout();

    int previousLineCount = doc->lineCount();
    const_cast<QTextBlock&>(block).setLineCount(block.isVisible() ? tl->lineCount() : 0);
    int lineCount = doc->lineCount();

    bool emitDocumentSizeChanged = previousLineCount != lineCount;
    if (blockMaximumWidth > d->maximumWidth) {
        // new longest line
        d->maximumWidth = blockMaximumWidth;
        d->maximumWidthBlockNumber = block.blockNumber();
        emitDocumentSizeChanged = true;
    } else if (block.blockNumber() == d->maximumWidthBlockNumber && blockMaximumWidth < d->maximumWidth) {
        // longest line shrinking
        QTextBlock b = doc->firstBlock();
        d->maximumWidth = 0;
        QTextBlock maximumBlock;
        while (b.isValid()) {
            qreal blockMaximumWidth = blockWidth(b);
            if (blockMaximumWidth > d->maximumWidth) {
                d->maximumWidth = blockMaximumWidth;
                maximumBlock = b;
            }
            b = b.next();
        }
        if (maximumBlock.isValid()) {
            d->maximumWidthBlockNumber = maximumBlock.blockNumber();
            emitDocumentSizeChanged = true;
        }
    }
    if (emitDocumentSizeChanged && !d->blockDocumentSizeChanged)
        emit documentSizeChanged(documentSize());
}

qreal QPlainTextDocumentLayout::blockWidth(const QTextBlock &block)
{
    QTextLayout *layout = block.layout();
    if (!layout->lineCount())
        return 0; // only for layouted blocks
    qreal blockWidth = 0;
    for (int i = 0; i < layout->lineCount(); ++i) {
        QTextLine line = layout->lineAt(i);
        blockWidth = qMax(line.naturalTextWidth() + 8, blockWidth);
    }
    return blockWidth;
}


QPlainTextEditControl::QPlainTextEditControl(QPlainTextEdit *parent)
    : QTextControl(parent), textEdit(parent),
      topBlock(0)
{
    setAcceptRichText(false);
}

void QPlainTextEditPrivate::_q_cursorPositionChanged()
{
    pageUpDownLastCursorYIsValid = false;
#ifndef QT_NO_ACCESSIBILITY
    Q_Q(QPlainTextEdit);
    QAccessible::updateAccessibility(q, 0, QAccessible::TextCaretMoved);
#endif
}

void QPlainTextEditPrivate::_q_verticalScrollbarActionTriggered(int action) {
    if (action == QAbstractSlider::SliderPageStepAdd) {
        pageUpDown(QTextCursor::Down, QTextCursor::MoveAnchor, false);
    } else if (action == QAbstractSlider::SliderPageStepSub) {
        pageUpDown(QTextCursor::Up, QTextCursor::MoveAnchor, false);
    }
}

QMimeData *QPlainTextEditControl::createMimeDataFromSelection() const {
        QPlainTextEdit *ed = qobject_cast<QPlainTextEdit *>(parent());
        if (!ed)
            return QTextControl::createMimeDataFromSelection();
        return ed->createMimeDataFromSelection();
    }
bool QPlainTextEditControl::canInsertFromMimeData(const QMimeData *source) const {
    QPlainTextEdit *ed = qobject_cast<QPlainTextEdit *>(parent());
    if (!ed)
        return QTextControl::canInsertFromMimeData(source);
    return ed->canInsertFromMimeData(source);
}
void QPlainTextEditControl::insertFromMimeData(const QMimeData *source) {
    QPlainTextEdit *ed = qobject_cast<QPlainTextEdit *>(parent());
    if (!ed)
        QTextControl::insertFromMimeData(source);
    else
        ed->insertFromMimeData(source);
}

int QPlainTextEditPrivate::verticalOffset(int topBlock, int topLine) const
{
    qreal offset = 0;
    QTextDocument *doc = control->document();

    if (topLine) {
        QTextBlock currentBlock = doc->findBlockByNumber(topBlock);
        QPlainTextDocumentLayout *documentLayout = qobject_cast<QPlainTextDocumentLayout*>(doc->documentLayout());
        Q_ASSERT(documentLayout);
        QRectF r = documentLayout->blockBoundingRect(currentBlock);
        Q_UNUSED(r);
        QTextLayout *layout = currentBlock.layout();
        if (layout && topLine <= layout->lineCount()) {
            QTextLine line = layout->lineAt(topLine - 1);
            const QRectF lr = line.naturalTextRect();
            offset = lr.bottom();
        }
    }
    if (topBlock == 0 && topLine == 0)
        offset -= doc->documentMargin(); // top margin
    return (int)offset;
}


int QPlainTextEditPrivate::verticalOffset() const {
    return verticalOffset(control->topBlock, topLine);
}


QTextBlock QPlainTextEditControl::firstVisibleBlock() const
{
    return document()->findBlockByNumber(topBlock);
}



int QPlainTextEditControl::hitTest(const QPointF &point, Qt::HitTestAccuracy ) const {
    int currentBlockNumber = topBlock;
    QTextBlock currentBlock = document()->findBlockByNumber(currentBlockNumber);
    if (!currentBlock.isValid())
        return -1;

    QPlainTextDocumentLayout *documentLayout = qobject_cast<QPlainTextDocumentLayout*>(document()->documentLayout());
    Q_ASSERT(documentLayout);

    QPointF offset;
    QRectF r = documentLayout->blockBoundingRect(currentBlock);
    while (currentBlock.next().isValid() && r.bottom() + offset.y() <= point.y()) {
        offset.ry() += r.height();
        currentBlock = currentBlock.next();
        ++currentBlockNumber;
        r = documentLayout->blockBoundingRect(currentBlock);
    }
    while (currentBlock.previous().isValid() && r.top() + offset.y() > point.y()) {
        offset.ry() -= r.height();
        currentBlock = currentBlock.previous();
        --currentBlockNumber;
        r = documentLayout->blockBoundingRect(currentBlock);
    }


    if (!currentBlock.isValid())
        return -1;
    QTextLayout *layout = currentBlock.layout();
    int off = 0;
    QPointF pos = point - offset;
    for (int i = 0; i < layout->lineCount(); ++i) {
        QTextLine line = layout->lineAt(i);
        const QRectF lr = line.naturalTextRect();
        if (lr.top() > pos.y()) {
            off = qMin(off, line.textStart());
        } else if (lr.bottom() <= pos.y()) {
            off = qMax(off, line.textStart() + line.textLength());
        } else {
            off = line.xToCursor(pos.x(), overwriteMode() ?
                                 QTextLine::CursorOnCharacter : QTextLine::CursorBetweenCharacters);
            break;
        }
    }

    return currentBlock.position() + off;
}

QRectF QPlainTextEditControl::blockBoundingRect(const QTextBlock &block) const {
    int currentBlockNumber = topBlock;
    int blockNumber = block.blockNumber();
    QTextBlock currentBlock = document()->findBlockByNumber(currentBlockNumber);
    if (!currentBlock.isValid())
        return QRectF();
    Q_ASSERT(currentBlock.blockNumber() == currentBlockNumber);
    QTextDocument *doc = document();
    QPlainTextDocumentLayout *documentLayout = qobject_cast<QPlainTextDocumentLayout*>(doc->documentLayout());
    Q_ASSERT(documentLayout);

    QPointF offset;
    if (!block.isValid())
        return QRectF();
    QRectF r = documentLayout->blockBoundingRect(currentBlock);
    int maxVerticalOffset = r.height();
    while (currentBlockNumber < blockNumber && offset.y() - maxVerticalOffset <= 2* textEdit->viewport()->height()) {
        offset.ry() += r.height();
        currentBlock = currentBlock.next();
        ++currentBlockNumber;
        if (!currentBlock.isVisible()) {
            currentBlock = doc->findBlockByLineNumber(currentBlock.firstLineNumber());
            currentBlockNumber = currentBlock.blockNumber();
        }
        r = documentLayout->blockBoundingRect(currentBlock);
    }
    while (currentBlockNumber > blockNumber && offset.y() + maxVerticalOffset >= -textEdit->viewport()->height()) {
        currentBlock = currentBlock.previous();
        --currentBlockNumber;
        while (!currentBlock.isVisible()) {
            currentBlock = currentBlock.previous();
            --currentBlockNumber;
        }
        if (!currentBlock.isValid())
            break;

        r = documentLayout->blockBoundingRect(currentBlock);
        offset.ry() -= r.height();
    }

    if (currentBlockNumber != blockNumber) {
        // fallback for blocks out of reach. Give it some geometry at
        // least, and ensure the layout is up to date.
        r = documentLayout->blockBoundingRect(block);
        if (currentBlockNumber > blockNumber)
            offset.ry() -= r.height();
    }
    r.translate(offset);
    return r;
}


void QPlainTextEditPrivate::setTopLine(int visualTopLine, int dx)
{
    QTextDocument *doc = control->document();
    QTextBlock block = doc->findBlockByLineNumber(visualTopLine);
    int blockNumber = block.blockNumber();
    int lineNumber = visualTopLine - block.firstLineNumber();
    setTopBlock(blockNumber, lineNumber, dx);
}

void QPlainTextEditPrivate::setTopBlock(int blockNumber, int lineNumber, int dx)
{
    Q_Q(QPlainTextEdit);
    blockNumber = qMax(0, blockNumber);
    lineNumber = qMax(0, lineNumber);
    QTextDocument *doc = control->document();
    QTextBlock block = doc->findBlockByNumber(blockNumber);

    int newTopLine = block.firstLineNumber() + lineNumber;
    int maxTopLine = vbar->maximum();

    if (newTopLine > maxTopLine) {
        block = doc->findBlockByLineNumber(maxTopLine);
        blockNumber = block.blockNumber();
        lineNumber = maxTopLine - block.firstLineNumber();
    }

    bool vbarSignalsBlocked = vbar->blockSignals(true);
    vbar->setValue(newTopLine);
    vbar->blockSignals(vbarSignalsBlocked);

    if (!dx && blockNumber == control->topBlock && lineNumber == topLine)
        return;

    if (viewport->updatesEnabled() && viewport->isVisible()) {
        int dy = 0;
        if (doc->findBlockByNumber(control->topBlock).isValid()) {
            dy = (int)(-q->blockBoundingGeometry(block).y())
                 + verticalOffset() - verticalOffset(blockNumber, lineNumber);
        }
        control->topBlock = blockNumber;
        topLine = lineNumber;

        bool vbarSignalsBlocked = vbar->blockSignals(true);
        vbar->setValue(block.firstLineNumber() + lineNumber);
        vbar->blockSignals(vbarSignalsBlocked);

        if (dx || dy)
            viewport->scroll(q->isRightToLeft() ? -dx : dx, dy);
        else
            viewport->update();
        emit q->updateRequest(viewport->rect(), dy);
    } else {
        control->topBlock = blockNumber;
        topLine = lineNumber;
    }

}



void QPlainTextEditPrivate::ensureVisible(int position, bool center, bool forceCenter) {
    Q_Q(QPlainTextEdit);
    QRectF visible = QRectF(viewport->rect()).translated(-q->contentOffset());
    QTextBlock block = control->document()->findBlock(position);
    if (!block.isValid())
        return;
    QRectF br = control->blockBoundingRect(block);
    if (!br.isValid())
        return;
    QRectF lr = br;
    QTextLine line = block.layout()->lineForTextPosition(position - block.position());
    Q_ASSERT(line.isValid());
    lr = line.naturalTextRect().translated(br.topLeft());

    if (lr.bottom() >= visible.bottom() || (center && lr.top() < visible.top()) || forceCenter){

        qreal height = visible.height();
        if (center)
            height /= 2;

        qreal h = center ? line.naturalTextRect().center().y() : line.naturalTextRect().bottom();

        QTextBlock previousVisibleBlock = block;
        while (h < height && block.previous().isValid()) {
            previousVisibleBlock = block;
            do {
                block = block.previous();
            } while (!block.isVisible() && block.previous().isValid());
            h += q->blockBoundingRect(block).height();
        }

        int l = 0;
        int lineCount = block.layout()->lineCount();
        int voffset = verticalOffset(block.blockNumber(), 0);
        while (l < lineCount) {
            QRectF lineRect = block.layout()->lineAt(l).naturalTextRect();
            if (h - voffset - lineRect.top() <= height)
                break;
            ++l;
        }

        if (l >= lineCount) {
            block = previousVisibleBlock;
            l = 0;
        }
        setTopBlock(block.blockNumber(), l);
    } else if (lr.top() < visible.top()) {
        setTopBlock(block.blockNumber(), line.lineNumber());
    }

}


void QPlainTextEditPrivate::updateViewport()
{
    Q_Q(QPlainTextEdit);
    viewport->update();
    emit q->updateRequest(viewport->rect(), 0);
}

QPlainTextEditPrivate::QPlainTextEditPrivate()
    : control(0),
      tabChangesFocus(false),
      lineWrap(QPlainTextEdit::WidgetWidth),
      wordWrap(QTextOption::WrapAtWordBoundaryOrAnywhere),
      clickCausedFocus(0),topLine(0), 
      pageUpDownLastCursorYIsValid(false)
{
    showCursorOnInitialShow = true;
    backgroundVisible = false;
    centerOnScroll = false;
    inDrag = false;
}


void QPlainTextEditPrivate::init(const QString &txt)
{
    Q_Q(QPlainTextEdit);
    control = new QPlainTextEditControl(q);

    QTextDocument *doc = new QTextDocument(control);
    QAbstractTextDocumentLayout *layout = new QPlainTextDocumentLayout(doc);
    doc->setDocumentLayout(layout);
    control->setDocument(doc);

    control->setPalette(q->palette());

    QObject::connect(vbar, SIGNAL(actionTriggered(int)), q, SLOT(_q_verticalScrollbarActionTriggered(int)));

    QObject::connect(control, SIGNAL(microFocusChanged()), q, SLOT(updateMicroFocus()));
    QObject::connect(control, SIGNAL(documentSizeChanged(QSizeF)), q, SLOT(_q_adjustScrollbars()));
    QObject::connect(control, SIGNAL(blockCountChanged(int)), q, SIGNAL(blockCountChanged(int)));
    QObject::connect(control, SIGNAL(updateRequest(QRectF)), q, SLOT(_q_repaintContents(QRectF)));
    QObject::connect(control, SIGNAL(modificationChanged(bool)), q, SIGNAL(modificationChanged(bool)));

    QObject::connect(control, SIGNAL(textChanged()), q, SIGNAL(textChanged()));
    QObject::connect(control, SIGNAL(undoAvailable(bool)), q, SIGNAL(undoAvailable(bool)));
    QObject::connect(control, SIGNAL(redoAvailable(bool)), q, SIGNAL(redoAvailable(bool)));
    QObject::connect(control, SIGNAL(copyAvailable(bool)), q, SIGNAL(copyAvailable(bool)));
    QObject::connect(control, SIGNAL(selectionChanged()), q, SIGNAL(selectionChanged()));
    QObject::connect(control, SIGNAL(cursorPositionChanged()), q, SLOT(_q_cursorPositionChanged()));
    QObject::connect(control, SIGNAL(cursorPositionChanged()), q, SIGNAL(cursorPositionChanged()));

    QObject::connect(control, SIGNAL(textChanged()), q, SLOT(updateMicroFocus()));

    // set a null page size initially to avoid any relayouting until the textedit
    // is shown. relayoutDocument() will take care of setting the page size to the
    // viewport dimensions later.
    doc->setTextWidth(-1);
    doc->documentLayout()->setPaintDevice(viewport);
    doc->setDefaultFont(q->font());


    if (!txt.isEmpty())
        control->setPlainText(txt);

    hbar->setSingleStep(20);
    vbar->setSingleStep(1);

    viewport->setBackgroundRole(QPalette::Base);
    q->setAcceptDrops(true);
    q->setFocusPolicy(Qt::WheelFocus);
    q->setAttribute(Qt::WA_KeyCompression);
    q->setAttribute(Qt::WA_InputMethodEnabled);

#ifndef QT_NO_CURSOR
    viewport->setCursor(Qt::IBeamCursor);
#endif
    originalOffsetY = 0;
#ifdef Q_WS_WIN
    setSingleFingerPanEnabled(true);
#endif
}

void QPlainTextEditPrivate::_q_repaintContents(const QRectF &contentsRect)
{
    Q_Q(QPlainTextEdit);
    if (!contentsRect.isValid()) {
        updateViewport();
        return;
    }
    const int xOffset = horizontalOffset();
    const int yOffset = verticalOffset();
    const QRect visibleRect(xOffset, yOffset, viewport->width(), viewport->height());

    QRect r = contentsRect.adjusted(-1, -1, 1, 1).intersected(visibleRect).toAlignedRect();
    if (r.isEmpty())
        return;

    r.translate(-xOffset, -yOffset);
    viewport->update(r);
    emit q->updateRequest(r, 0);
}

void QPlainTextEditPrivate::pageUpDown(QTextCursor::MoveOperation op, QTextCursor::MoveMode moveMode, bool moveCursor)
{

    Q_Q(QPlainTextEdit);

    QTextCursor cursor = control->textCursor();
    if (moveCursor) {
        ensureCursorVisible();
        if (!pageUpDownLastCursorYIsValid)
            pageUpDownLastCursorY = control->cursorRect(cursor).top() - verticalOffset();
    }

    qreal lastY = pageUpDownLastCursorY;


    if (op == QTextCursor::Down) {
        QRectF visible = QRectF(viewport->rect()).translated(-q->contentOffset());
        QTextBlock firstVisibleBlock = q->firstVisibleBlock();
        QTextBlock block = firstVisibleBlock;
        QRectF br = q->blockBoundingRect(block);
        qreal h = 0;
        int atEnd = false;
        while (h + br.height() <= visible.bottom()) {
            if (!block.next().isValid()) {
                atEnd = true;
                lastY = visible.bottom(); // set cursor to last line
                break;
            }
            h += br.height();
            block = block.next();
            br = q->blockBoundingRect(block);
        }

        if (!atEnd) {
            int line = 0;
            qreal diff = visible.bottom() - h;
            int lineCount = block.layout()->lineCount();
            while (line < lineCount - 1) {
                if (block.layout()->lineAt(line).naturalTextRect().bottom() > diff) {
                    // the first line that did not completely fit the screen
                    break;
                }
                ++line;
            }
            setTopBlock(block.blockNumber(), line);
        }

        if (moveCursor) {
            // move using movePosition to keep the cursor's x
            lastY += verticalOffset();
            bool moved = false;
            do {
                moved = cursor.movePosition(op, moveMode);
            } while (moved && control->cursorRect(cursor).top() < lastY);
        }

    } else if (op == QTextCursor::Up) {

        QRectF visible = QRectF(viewport->rect()).translated(-q->contentOffset());
        visible.translate(0, -visible.height()); // previous page
        QTextBlock block = q->firstVisibleBlock();
        qreal h = 0;
        while (h >= visible.top()) {
            if (!block.previous().isValid()) {
                if (control->topBlock == 0 && topLine == 0) {
                    lastY = 0; // set cursor to first line
                }
                break;
            }
            block = block.previous();
            QRectF br = q->blockBoundingRect(block);
            h -= br.height();
        }

        int line = 0;
        if (block.isValid()) {
            qreal diff = visible.top() - h;
            int lineCount = block.layout()->lineCount();
            while (line < lineCount) {
                if (block.layout()->lineAt(line).naturalTextRect().top() >= diff)
                    break;
                ++line;
            }
            if (line == lineCount) {
                if (block.next().isValid() && block.next() != q->firstVisibleBlock()) {
                    block = block.next();
                    line = 0;
                } else {
                    --line;
                }
            }
        }
        setTopBlock(block.blockNumber(), line);

        if (moveCursor) {
            cursor.setVisualNavigation(true);
            // move using movePosition to keep the cursor's x
            lastY += verticalOffset();
            bool moved = false;
            do {
                moved = cursor.movePosition(op, moveMode);
            } while (moved && control->cursorRect(cursor).top() > lastY);
        }
    }

    if (moveCursor) {
        control->setTextCursor(cursor);
        pageUpDownLastCursorYIsValid = true;
    }
}

#ifndef QT_NO_SCROLLBAR

void QPlainTextEditPrivate::_q_adjustScrollbars()
{
    Q_Q(QPlainTextEdit);
    QTextDocument *doc = control->document();
    QPlainTextDocumentLayout *documentLayout = qobject_cast<QPlainTextDocumentLayout*>(doc->documentLayout());
    Q_ASSERT(documentLayout);
    bool documentSizeChangedBlocked = documentLayout->priv()->blockDocumentSizeChanged;
    documentLayout->priv()->blockDocumentSizeChanged = true;
    qreal margin = doc->documentMargin();

    int vmax = 0;

    int vSliderLength = 0;
    if (!centerOnScroll && q->isVisible()) {
        QTextBlock block = doc->lastBlock();
        const qreal visible = viewport->rect().height() - margin - 1;
        qreal y = 0;
        int visibleFromBottom = 0;

        while (block.isValid()) {
            if (!block.isVisible()) {
                block = block.previous();
                continue;
            }
            y += documentLayout->blockBoundingRect(block).height();

            QTextLayout *layout = block.layout();
            int layoutLineCount = layout->lineCount();
            if (y > visible) {
                int lineNumber = 0;
                while (lineNumber < layoutLineCount) {
                    QTextLine line = layout->lineAt(lineNumber);
                    const QRectF lr = line.naturalTextRect();
                    if (lr.top() >= y - visible)
                        break;
                    ++lineNumber;
                }
                if (lineNumber < layoutLineCount)
                    visibleFromBottom += (layoutLineCount - lineNumber);
                break;

            }
            visibleFromBottom += layoutLineCount;
            block = block.previous();
        }
        vmax = qMax(0, doc->lineCount() - visibleFromBottom);
        vSliderLength = visibleFromBottom;

    } else {
        vmax = qMax(0, doc->lineCount() - 1);
        vSliderLength = viewport->height() / q->fontMetrics().lineSpacing();
    }



    QSizeF documentSize = documentLayout->documentSize();
    vbar->setRange(0, qMax(0, vmax));
    vbar->setPageStep(vSliderLength);
    int visualTopLine = vmax;
    QTextBlock firstVisibleBlock = q->firstVisibleBlock();
    if (firstVisibleBlock.isValid())
        visualTopLine = firstVisibleBlock.firstLineNumber() + topLine;
    bool vbarSignalsBlocked = vbar->blockSignals(true);
    vbar->setValue(visualTopLine);
    vbar->blockSignals(vbarSignalsBlocked);

    hbar->setRange(0, (int)documentSize.width() - viewport->width());
    hbar->setPageStep(viewport->width());
    documentLayout->priv()->blockDocumentSizeChanged = documentSizeChangedBlocked;
    setTopLine(vbar->value());
}

#endif


void QPlainTextEditPrivate::ensureViewportLayouted()
{
}

/*!
    \class QPlainTextEdit
    \since 4.4
    \brief The QPlainTextEdit class provides a widget that is used to edit and display
    plain text.

    \ingroup richtext-processing


    \tableofcontents

    \section1 Introduction and Concepts

    QPlainTextEdit is an advanced viewer/editor supporting plain
    text. It is optimized to handle large documents and to respond
    quickly to user input.

    QPlainText uses very much the same technology and concepts as
    QTextEdit, but is optimized for plain text handling.

    QPlainTextEdit works on paragraphs and characters. A paragraph is
    a formatted string which is word-wrapped to fit into the width of
    the widget. By default when reading plain text, one newline
    signifies a paragraph. A document consists of zero or more
    paragraphs. Paragraphs are separated by hard line breaks. Each
    character within a paragraph has its own attributes, for example,
    font and color.

    The shape of the mouse cursor on a QPlainTextEdit is
    Qt::IBeamCursor by default.  It can be changed through the
    viewport()'s cursor property.

    \section1 Using QPlainTextEdit as a Display Widget

    The text is set or replaced using setPlainText() which deletes the
    existing text and replaces it with the text passed to setPlainText().

    Text can be inserted using the QTextCursor class or using the
    convenience functions insertPlainText(), appendPlainText() or
    paste().

    By default, the text edit wraps words at whitespace to fit within
    the text edit widget. The setLineWrapMode() function is used to
    specify the kind of line wrap you want, \l WidgetWidth or \l
    NoWrap if you don't want any wrapping.  If you use word wrap to
    the widget's width \l WidgetWidth, you can specify whether to
    break on whitespace or anywhere with setWordWrapMode().

    The find() function can be used to find and select a given string
    within the text.

    If you want to limit the total number of paragraphs in a
    QPlainTextEdit, as it is for example useful in a log viewer, then
    you can use the maximumBlockCount property. The combination of
    setMaximumBlockCount() and appendPlainText() turns QPlainTextEdit
    into an efficient viewer for log text. The scrolling can be
    reduced with the centerOnScroll() property, making the log viewer
    even faster. Text can be formatted in a limited way, either using
    a syntax highlighter (see below), or by appending html-formatted
    text with appendHtml(). While QPlainTextEdit does not support
    complex rich text rendering with tables and floats, it does
    support limited paragraph-based formatting that you may need in a
    log viewer.

    \section2 Read-only Key Bindings

    When QPlainTextEdit is used read-only the key bindings are limited to
    navigation, and text may only be selected with the mouse:
    \table
    \header \i Keypresses \i Action
    \row \i Qt::UpArrow        \i Moves one line up.
    \row \i Qt::DownArrow        \i Moves one line down.
    \row \i Qt::LeftArrow        \i Moves one character to the left.
    \row \i Qt::RightArrow        \i Moves one character to the right.
    \row \i PageUp        \i Moves one (viewport) page up.
    \row \i PageDown        \i Moves one (viewport) page down.
    \row \i Home        \i Moves to the beginning of the text.
    \row \i End                \i Moves to the end of the text.
    \row \i Alt+Wheel
         \i Scrolls the page horizontally (the Wheel is the mouse wheel).
    \row \i Ctrl+Wheel        \i Zooms the text.
    \row \i Ctrl+A            \i Selects all text.
    \endtable


    \section1 Using QPlainTextEdit as an Editor

    All the information about using QPlainTextEdit as a display widget also
    applies here.

    Selection of text is handled by the QTextCursor class, which provides
    functionality for creating selections, retrieving the text contents or
    deleting selections. You can retrieve the object that corresponds with
    the user-visible cursor using the textCursor() method. If you want to set
    a selection in QPlainTextEdit just create one on a QTextCursor object and
    then make that cursor the visible cursor using setCursor(). The selection
    can be copied to the clipboard with copy(), or cut to the clipboard with
    cut(). The entire text can be selected using selectAll().

    QPlainTextEdit holds a QTextDocument object which can be retrieved using the
    document() method. You can also set your own document object using setDocument().
    QTextDocument emits a textChanged() signal if the text changes and it also
    provides a isModified() function which will return true if the text has been
    modified since it was either loaded or since the last call to setModified
    with false as argument. In addition it provides methods for undo and redo.

    \section2 Syntax Highlighting

    Just like QTextEdit, QPlainTextEdit works together with
    QSyntaxHighlighter.

    \section2 Editing Key Bindings

    The list of key bindings which are implemented for editing:
    \table
    \header \i Keypresses \i Action
    \row \i Backspace \i Deletes the character to the left of the cursor.
    \row \i Delete \i Deletes the character to the right of the cursor.
    \row \i Ctrl+C \i Copy the selected text to the clipboard.
    \row \i Ctrl+Insert \i Copy the selected text to the clipboard.
    \row \i Ctrl+K \i Deletes to the end of the line.
    \row \i Ctrl+V \i Pastes the clipboard text into text edit.
    \row \i Shift+Insert \i Pastes the clipboard text into text edit.
    \row \i Ctrl+X \i Deletes the selected text and copies it to the clipboard.
    \row \i Shift+Delete \i Deletes the selected text and copies it to the clipboard.
    \row \i Ctrl+Z \i Undoes the last operation.
    \row \i Ctrl+Y \i Redoes the last operation.
    \row \i LeftArrow \i Moves the cursor one character to the left.
    \row \i Ctrl+LeftArrow \i Moves the cursor one word to the left.
    \row \i RightArrow \i Moves the cursor one character to the right.
    \row \i Ctrl+RightArrow \i Moves the cursor one word to the right.
    \row \i UpArrow \i Moves the cursor one line up.
    \row \i Ctrl+UpArrow \i Moves the cursor one word up.
    \row \i DownArrow \i Moves the cursor one line down.
    \row \i Ctrl+Down Arrow \i Moves the cursor one word down.
    \row \i PageUp \i Moves the cursor one page up.
    \row \i PageDown \i Moves the cursor one page down.
    \row \i Home \i Moves the cursor to the beginning of the line.
    \row \i Ctrl+Home \i Moves the cursor to the beginning of the text.
    \row \i End \i Moves the cursor to the end of the line.
    \row \i Ctrl+End \i Moves the cursor to the end of the text.
    \row \i Alt+Wheel \i Scrolls the page horizontally (the Wheel is the mouse wheel).
    \row \i Ctrl+Wheel \i Zooms the text.
    \endtable

    To select (mark) text hold down the Shift key whilst pressing one
    of the movement keystrokes, for example, \e{Shift+Right Arrow}
    will select the character to the right, and \e{Shift+Ctrl+Right
    Arrow} will select the word to the right, etc.

   \section1 Differences to QTextEdit

   QPlainTextEdit is a thin class, implemented by using most of the
   technology that is behind QTextEdit and QTextDocument. Its
   performance benefits over QTextEdit stem mostly from using a
   different and simplified text layout called
   QPlainTextDocumentLayout on the text document (see
   QTextDocument::setDocumentLayout()). The plain text document layout
   does not support tables nor embedded frames, and \e{replaces a
   pixel-exact height calculation with a line-by-line respectively
   paragraph-by-paragraph scrolling approach}. This makes it possible
   to handle significantly larger documents, and still resize the
   editor with line wrap enabled in real time. It also makes for a
   fast log viewer (see setMaximumBlockCount()).


    \sa QTextDocument, QTextCursor, {Application Example},
        {Code Editor Example}, {Syntax Highlighter Example},
        {Rich Text Processing}

*/

/*!
    \property QPlainTextEdit::plainText

    This property gets and sets the plain text editor's contents. The previous
    contents are removed and undo/redo history is reset when this property is set.

    By default, for an editor with no contents, this property contains an empty string.
*/

/*!
    \property QPlainTextEdit::undoRedoEnabled
    \brief whether undo and redo are enabled

    Users are only able to undo or redo actions if this property is
    true, and if there is an action that can be undone (or redone).

    By default, this property is true.
*/

/*!
    \enum QPlainTextEdit::LineWrapMode

    \value NoWrap
    \value WidgetWidth
*/


/*!
    Constructs an empty QPlainTextEdit with parent \a
    parent.
*/
QPlainTextEdit::QPlainTextEdit(QWidget *parent)
    : QAbstractScrollArea(*new QPlainTextEditPrivate, parent)
{
    Q_D(QPlainTextEdit);
    d->init();
}

/*!
    \internal
*/
QPlainTextEdit::QPlainTextEdit(QPlainTextEditPrivate &dd, QWidget *parent)
    : QAbstractScrollArea(dd, parent)
{
    Q_D(QPlainTextEdit);
    d->init();
}

/*!
    Constructs a QPlainTextEdit with parent \a parent. The text edit will display
    the plain text \a text.
*/
QPlainTextEdit::QPlainTextEdit(const QString &text, QWidget *parent)
    : QAbstractScrollArea(*new QPlainTextEditPrivate, parent)
{
    Q_D(QPlainTextEdit);
    d->init(text);
}


/*!
    Destructor.
*/
QPlainTextEdit::~QPlainTextEdit()
{
    Q_D(QPlainTextEdit);
    if (d->documentLayoutPtr) {
        if (d->documentLayoutPtr->priv()->mainViewPrivate == d)
            d->documentLayoutPtr->priv()->mainViewPrivate = 0;
    }
}

/*!
    Makes \a document the new document of the text editor.

    The parent QObject of the provided document remains the owner
    of the object. If the current document is a child of the text
    editor, then it is deleted.

    The document must have a document layout that inherits
    QPlainTextDocumentLayout (see QTextDocument::setDocumentLayout()).

    \sa document()
*/
void QPlainTextEdit::setDocument(QTextDocument *document)
{
    Q_D(QPlainTextEdit);
    QPlainTextDocumentLayout *documentLayout = 0;

    if (!document) {
        document = new QTextDocument(d->control);
        documentLayout = new QPlainTextDocumentLayout(document);
        document->setDocumentLayout(documentLayout);
    } else {
        documentLayout = qobject_cast<QPlainTextDocumentLayout*>(document->documentLayout());
        if (!documentLayout) {
            qWarning("QPlainTextEdit::setDocument: Document set does not support QPlainTextDocumentLayout");
            return;
        }
    }
    d->control->setDocument(document);
    if (!documentLayout->priv()->mainViewPrivate)
        documentLayout->priv()->mainViewPrivate = d;
    d->documentLayoutPtr = documentLayout;
    d->updateDefaultTextOption();
    d->relayoutDocument();
    d->_q_adjustScrollbars();
}

/*!
    Returns a pointer to the underlying document.

    \sa setDocument()
*/
QTextDocument *QPlainTextEdit::document() const
{
    Q_D(const QPlainTextEdit);
    return d->control->document();
}

/*!
    Sets the visible \a cursor.
*/
void QPlainTextEdit::setTextCursor(const QTextCursor &cursor)
{
    Q_D(QPlainTextEdit);
    d->control->setTextCursor(cursor);
}

/*!
    Returns a copy of the QTextCursor that represents the currently visible cursor.
    Note that changes on the returned cursor do not affect QPlainTextEdit's cursor; use
    setTextCursor() to update the visible cursor.
 */
QTextCursor QPlainTextEdit::textCursor() const
{
    Q_D(const QPlainTextEdit);
    return d->control->textCursor();
}

/*!
    Returns the reference of the anchor at position \a pos, or an
    empty string if no anchor exists at that point.

    \since 4.7
 */
QString QPlainTextEdit::anchorAt(const QPoint &pos) const
{
    Q_D(const QPlainTextEdit);
    int cursorPos = d->control->hitTest(pos + QPoint(d->horizontalOffset(),
                                                     d->verticalOffset()),
                                        Qt::ExactHit);
    if (cursorPos < 0)
        return QString();

    QTextDocumentPrivate *pieceTable = document()->docHandle();
    QTextDocumentPrivate::FragmentIterator it = pieceTable->find(cursorPos);
    QTextCharFormat fmt = pieceTable->formatCollection()->charFormat(it->format);
    return fmt.anchorHref();
}

/*!
    Undoes the last operation.

    If there is no operation to undo, i.e. there is no undo step in
    the undo/redo history, nothing happens.

    \sa redo()
*/
void QPlainTextEdit::undo()
{
    Q_D(QPlainTextEdit);
    d->control->undo();
}

void QPlainTextEdit::redo()
{
    Q_D(QPlainTextEdit);
    d->control->redo();
}

/*!
    \fn void QPlainTextEdit::redo()

    Redoes the last operation.

    If there is no operation to redo, i.e. there is no redo step in
    the undo/redo history, nothing happens.

    \sa undo()
*/

#ifndef QT_NO_CLIPBOARD
/*!
    Copies the selected text to the clipboard and deletes it from
    the text edit.

    If there is no selected text nothing happens.

    \sa copy() paste()
*/

void QPlainTextEdit::cut()
{
    Q_D(QPlainTextEdit);
    d->control->cut();
}

/*!
    Copies any selected text to the clipboard.

    \sa copyAvailable()
*/

void QPlainTextEdit::copy()
{
    Q_D(QPlainTextEdit);
    d->control->copy();
}

/*!
    Pastes the text from the clipboard into the text edit at the
    current cursor position.

    If there is no text in the clipboard nothing happens.

    To change the behavior of this function, i.e. to modify what
    QPlainTextEdit can paste and how it is being pasted, reimplement the
    virtual canInsertFromMimeData() and insertFromMimeData()
    functions.

    \sa cut() copy()
*/

void QPlainTextEdit::paste()
{
    Q_D(QPlainTextEdit);
    d->control->paste();
}
#endif

/*!
    Deletes all the text in the text edit.

    Note that the undo/redo history is cleared by this function.

    \sa cut() setPlainText()
*/
void QPlainTextEdit::clear()
{
    Q_D(QPlainTextEdit);
    // clears and sets empty content
    d->control->topBlock = d->topLine = 0;
    d->control->clear();
}


/*!
    Selects all text.

    \sa copy() cut() textCursor()
 */
void QPlainTextEdit::selectAll()
{
    Q_D(QPlainTextEdit);
    d->control->selectAll();
}

/*! \internal
*/
bool QPlainTextEdit::event(QEvent *e)
{
    Q_D(QPlainTextEdit);

#ifndef QT_NO_CONTEXTMENU
    if (e->type() == QEvent::ContextMenu
        && static_cast<QContextMenuEvent *>(e)->reason() == QContextMenuEvent::Keyboard) {
        ensureCursorVisible();
        const QPoint cursorPos = cursorRect().center();
        QContextMenuEvent ce(QContextMenuEvent::Keyboard, cursorPos, d->viewport->mapToGlobal(cursorPos));
        ce.setAccepted(e->isAccepted());
        const bool result = QAbstractScrollArea::event(&ce);
        e->setAccepted(ce.isAccepted());
        return result;
    }
#endif // QT_NO_CONTEXTMENU
    if (e->type() == QEvent::ShortcutOverride
               || e->type() == QEvent::ToolTip) {
        d->sendControlEvent(e);
    }
#ifdef QT_KEYPAD_NAVIGATION
    else if (e->type() == QEvent::EnterEditFocus || e->type() == QEvent::LeaveEditFocus) {
        if (QApplication::keypadNavigationEnabled())
            d->sendControlEvent(e);
    }
#endif
#ifndef QT_NO_GESTURES
    else if (e->type() == QEvent::Gesture) {
        QGestureEvent *ge = static_cast<QGestureEvent *>(e);
        QPanGesture *g = static_cast<QPanGesture *>(ge->gesture(Qt::PanGesture));
        if (g) {
            QScrollBar *hBar = horizontalScrollBar();
            QScrollBar *vBar = verticalScrollBar();
            if (g->state() == Qt::GestureStarted)
                d->originalOffsetY = vBar->value();
            QPointF offset = g->offset();
            if (!offset.isNull()) {
                if (QApplication::isRightToLeft())
                    offset.rx() *= -1;
                // QPlainTextEdit scrolls by lines only in vertical direction
                QFontMetrics fm(document()->defaultFont());
                int lineHeight = fm.height();
                int newX = hBar->value() - g->delta().x();
                int newY = d->originalOffsetY - offset.y()/lineHeight;
                hBar->setValue(newX);
                vBar->setValue(newY);
            }
        }
        return true;
    }
#endif // QT_NO_GESTURES
    return QAbstractScrollArea::event(e);
}

/*! \internal
*/

void QPlainTextEdit::timerEvent(QTimerEvent *e)
{
    Q_D(QPlainTextEdit);
    if (e->timerId() == d->autoScrollTimer.timerId()) {
        QRect visible = d->viewport->rect();
        QPoint pos;
        if (d->inDrag) {
            pos = d->autoScrollDragPos;
            visible.adjust(qMin(visible.width()/3,20), qMin(visible.height()/3,20),
                           -qMin(visible.width()/3,20), -qMin(visible.height()/3,20));
        } else {
            const QPoint globalPos = QCursor::pos();
            pos = d->viewport->mapFromGlobal(globalPos);
            QMouseEvent ev(QEvent::MouseMove, pos, globalPos, Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
            mouseMoveEvent(&ev);
        }
        int deltaY = qMax(pos.y() - visible.top(), visible.bottom() - pos.y()) - visible.height();
        int deltaX = qMax(pos.x() - visible.left(), visible.right() - pos.x()) - visible.width();
        int delta = qMax(deltaX, deltaY);
        if (delta >= 0) {
            if (delta < 7)
                delta = 7;
            int timeout = 4900 / (delta * delta);
            d->autoScrollTimer.start(timeout, this);

            if (deltaY > 0)
                d->vbar->triggerAction(pos.y() < visible.center().y() ?
                                       QAbstractSlider::SliderSingleStepSub
                                       : QAbstractSlider::SliderSingleStepAdd);
            if (deltaX > 0)
                d->hbar->triggerAction(pos.x() < visible.center().x() ?
                                       QAbstractSlider::SliderSingleStepSub
                                       : QAbstractSlider::SliderSingleStepAdd);
        }
    }
#ifdef QT_KEYPAD_NAVIGATION
    else if (e->timerId() == d->deleteAllTimer.timerId()) {
        d->deleteAllTimer.stop();
        clear();
    }
#endif
}

/*!
    Changes the text of the text edit to the string \a text.
    Any previous text is removed.

    \a text is interpreted as plain text.

    Note that the undo/redo history is cleared by this function.

    \sa toText()
*/

void QPlainTextEdit::setPlainText(const QString &text)
{
    Q_D(QPlainTextEdit);
    d->control->setPlainText(text);
}

/*!
    \fn QString QPlainTextEdit::toPlainText() const

    Returns the text of the text edit as plain text.

    \sa QPlainTextEdit::setPlainText()
 */

/*! \reimp
*/
void QPlainTextEdit::keyPressEvent(QKeyEvent *e)
{
    Q_D(QPlainTextEdit);

#ifdef QT_KEYPAD_NAVIGATION
    switch (e->key()) {
        case Qt::Key_Select:
            if (QApplication::keypadNavigationEnabled()) {
                if (!(d->control->textInteractionFlags() & Qt::LinksAccessibleByKeyboard))
                    setEditFocus(!hasEditFocus());
                else {
                    if (!hasEditFocus())
                        setEditFocus(true);
                    else {
                        QTextCursor cursor = d->control->textCursor();
                        QTextCharFormat charFmt = cursor.charFormat();
                        if (!cursor.hasSelection() || charFmt.anchorHref().isEmpty()) {
                            setEditFocus(false);
                        }
                    }
                }
            }
            break;
        case Qt::Key_Back:
        case Qt::Key_No:
            if (!QApplication::keypadNavigationEnabled()
                    || (QApplication::keypadNavigationEnabled() && !hasEditFocus())) {
                e->ignore();
                return;
            }
            break;
        default:
            if (QApplication::keypadNavigationEnabled()) {
                if (!hasEditFocus() && !(e->modifiers() & Qt::ControlModifier)) {
                    if (e->text()[0].isPrint()) {
                        setEditFocus(true);
                        clear();
                    } else {
                        e->ignore();
                        return;
                    }
                }
            }
            break;
    }
#endif

#ifndef QT_NO_SHORTCUT

    Qt::TextInteractionFlags tif = d->control->textInteractionFlags();

    if (tif & Qt::TextSelectableByKeyboard){
        if (e == QKeySequence::SelectPreviousPage) {
            e->accept();
            d->pageUpDown(QTextCursor::Up, QTextCursor::KeepAnchor);
            return;
        } else if (e ==QKeySequence::SelectNextPage) {
            e->accept();
            d->pageUpDown(QTextCursor::Down, QTextCursor::KeepAnchor);
            return;
        }
    }
    if (tif & (Qt::TextSelectableByKeyboard | Qt::TextEditable)) {
        if (e == QKeySequence::MoveToPreviousPage) {
            e->accept();
            d->pageUpDown(QTextCursor::Up, QTextCursor::MoveAnchor);
            return;
        } else if (e == QKeySequence::MoveToNextPage) {
            e->accept();
            d->pageUpDown(QTextCursor::Down, QTextCursor::MoveAnchor);
            return;
        }
    }

    if (!(tif & Qt::TextEditable)) {
        switch (e->key()) {
            case Qt::Key_Space:
                e->accept();
                if (e->modifiers() & Qt::ShiftModifier)
                    d->vbar->triggerAction(QAbstractSlider::SliderPageStepSub);
                else
                    d->vbar->triggerAction(QAbstractSlider::SliderPageStepAdd);
                break;
            default:
                d->sendControlEvent(e);
                if (!e->isAccepted() && e->modifiers() == Qt::NoModifier) {
                    if (e->key() == Qt::Key_Home) {
                        d->vbar->triggerAction(QAbstractSlider::SliderToMinimum);
                        e->accept();
                    } else if (e->key() == Qt::Key_End) {
                        d->vbar->triggerAction(QAbstractSlider::SliderToMaximum);
                        e->accept();
                    }
                }
                if (!e->isAccepted()) {
                    QAbstractScrollArea::keyPressEvent(e);
                }
        }
        return;
    }
#endif // QT_NO_SHORTCUT

    d->sendControlEvent(e);
#ifdef QT_KEYPAD_NAVIGATION
    if (!e->isAccepted()) {
        switch (e->key()) {
            case Qt::Key_Up:
            case Qt::Key_Down:
                if (QApplication::keypadNavigationEnabled()) {
                    // Cursor position didn't change, so we want to leave
                    // these keys to change focus.
                    e->ignore();
                    return;
                }
                break;
            case Qt::Key_Left:
            case Qt::Key_Right:
                if (QApplication::keypadNavigationEnabled()
                        && QApplication::navigationMode() == Qt::NavigationModeKeypadDirectional) {
                    // Same as for Key_Up and Key_Down.
                    e->ignore();
                    return;
                }
                break;
            case Qt::Key_Back:
                if (!e->isAutoRepeat()) {
                    if (QApplication::keypadNavigationEnabled()) {
                        if (document()->isEmpty()) {
                            setEditFocus(false);
                            e->accept();
                        } else if (!d->deleteAllTimer.isActive()) {
                            e->accept();
                            d->deleteAllTimer.start(750, this);
                        }
                    } else {
                        e->ignore();
                        return;
                    }
                }
                break;
            default: break;
        }
    }
#endif
}

/*! \reimp
*/
void QPlainTextEdit::keyReleaseEvent(QKeyEvent *e)
{
#ifdef QT_KEYPAD_NAVIGATION
    Q_D(QPlainTextEdit);
    if (QApplication::keypadNavigationEnabled()) {
        if (!e->isAutoRepeat() && e->key() == Qt::Key_Back
            && d->deleteAllTimer.isActive()) {
            d->deleteAllTimer.stop();
            QTextCursor cursor = d->control->textCursor();
            QTextBlockFormat blockFmt = cursor.blockFormat();

            QTextList *list = cursor.currentList();
            if (list && cursor.atBlockStart()) {
                list->remove(cursor.block());
            } else if (cursor.atBlockStart() && blockFmt.indent() > 0) {
                blockFmt.setIndent(blockFmt.indent() - 1);
                cursor.setBlockFormat(blockFmt);
            } else {
                cursor.deletePreviousChar();
            }
            setTextCursor(cursor);
        }
    }
#else
    Q_UNUSED(e);
#endif
}

/*!
    Loads the resource specified by the given \a type and \a name.

    This function is an extension of QTextDocument::loadResource().

    \sa QTextDocument::loadResource()
*/
QVariant QPlainTextEdit::loadResource(int type, const QUrl &name)
{
    Q_UNUSED(type);
    Q_UNUSED(name);
    return QVariant();
}

/*! \reimp
*/
void QPlainTextEdit::resizeEvent(QResizeEvent *e)
{
    Q_D(QPlainTextEdit);
    if (e->oldSize().width() != e->size().width())
        d->relayoutDocument();
    d->_q_adjustScrollbars();
}

void QPlainTextEditPrivate::relayoutDocument()
{
    QTextDocument *doc = control->document();
    QPlainTextDocumentLayout *documentLayout = qobject_cast<QPlainTextDocumentLayout*>(doc->documentLayout());
    Q_ASSERT(documentLayout);
    documentLayoutPtr = documentLayout;

    int width = viewport->width();

    if (documentLayout->priv()->mainViewPrivate == 0
        || documentLayout->priv()->mainViewPrivate == this
        || width > documentLayout->textWidth()) {
        documentLayout->priv()->mainViewPrivate = this;
        documentLayout->setTextWidth(width);
    }
}

static void fillBackground(QPainter *p, const QRectF &rect, QBrush brush, QRectF gradientRect = QRectF())
{
    p->save();
    if (brush.style() >= Qt::LinearGradientPattern && brush.style() <= Qt::ConicalGradientPattern) {
        if (!gradientRect.isNull()) {
            QTransform m = QTransform::fromTranslate(gradientRect.left(), gradientRect.top());
            m.scale(gradientRect.width(), gradientRect.height());
            brush.setTransform(m);
            const_cast<QGradient *>(brush.gradient())->setCoordinateMode(QGradient::LogicalMode);
        }
    } else {
        p->setBrushOrigin(rect.topLeft());
    }
    p->fillRect(rect, brush);
    p->restore();
}



/*! \reimp
*/
void QPlainTextEdit::paintEvent(QPaintEvent *e)
{
    QPainter painter(viewport());
    Q_ASSERT(qobject_cast<QPlainTextDocumentLayout*>(document()->documentLayout()));

    QPointF offset(contentOffset());

    QRect er = e->rect();
    QRect viewportRect = viewport()->rect();

    bool editable = !isReadOnly();

    QTextBlock block = firstVisibleBlock();
    qreal maximumWidth = document()->documentLayout()->documentSize().width();

    // Set a brush origin so that the WaveUnderline knows where the wave started
    painter.setBrushOrigin(offset);

    // keep right margin clean from full-width selection
    int maxX = offset.x() + qMax((qreal)viewportRect.width(), maximumWidth)
               - document()->documentMargin();
    er.setRight(qMin(er.right(), maxX));
    painter.setClipRect(er);


    QAbstractTextDocumentLayout::PaintContext context = getPaintContext();

    while (block.isValid()) {

        QRectF r = blockBoundingRect(block).translated(offset);
        QTextLayout *layout = block.layout();

        if (!block.isVisible()) {
            offset.ry() += r.height();
            block = block.next();
            continue;
        }

        if (r.bottom() >= er.top() && r.top() <= er.bottom()) {

            QTextBlockFormat blockFormat = block.blockFormat();

            QBrush bg = blockFormat.background();
            if (bg != Qt::NoBrush) {
                QRectF contentsRect = r;
                contentsRect.setWidth(qMax(r.width(), maximumWidth));
                fillBackground(&painter, contentsRect, bg);
            }


            QVector<QTextLayout::FormatRange> selections;
            int blpos = block.position();
            int bllen = block.length();
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
                } else if (!range.cursor.hasSelection() && range.format.hasProperty(QTextFormat::FullWidthSelection)
                           && block.contains(range.cursor.position())) {
                    // for full width selections we don't require an actual selection, just
                    // a position to specify the line. that's more convenience in usage.
                    QTextLayout::FormatRange o;
                    QTextLine l = layout->lineForTextPosition(range.cursor.position() - blpos);
                    o.start = l.textStart();
                    o.length = l.textLength();
                    if (o.start + o.length == bllen - 1)
                        ++o.length; // include newline
                    o.format = range.format;
                    selections.append(o);
                }
            }

            bool drawCursor = ((editable || (textInteractionFlags() & Qt::TextSelectableByKeyboard))
                               && context.cursorPosition >= blpos
                               && context.cursorPosition < blpos + bllen);

            bool drawCursorAsBlock = drawCursor && overwriteMode() ;

            if (drawCursorAsBlock) {
                if (context.cursorPosition == blpos + bllen - 1) {
                    drawCursorAsBlock = false;
                } else {
                    QTextLayout::FormatRange o;
                    o.start = context.cursorPosition - blpos;
                    o.length = 1;
                    o.format.setForeground(palette().base());
                    o.format.setBackground(palette().text());
                    selections.append(o);
                }
            }


            layout->draw(&painter, offset, selections, er);
            if ((drawCursor && !drawCursorAsBlock)
                || (editable && context.cursorPosition < -1
                    && !layout->preeditAreaText().isEmpty())) {
                int cpos = context.cursorPosition;
                if (cpos < -1)
                    cpos = layout->preeditAreaPosition() - (cpos + 2);
                else
                    cpos -= blpos;
                layout->drawCursor(&painter, offset, cpos, cursorWidth());
            }
        }

        offset.ry() += r.height();
        if (offset.y() > viewportRect.height())
            break;
        block = block.next();
    }

    if (backgroundVisible() && !block.isValid() && offset.y() <= er.bottom()
        && (centerOnScroll() || verticalScrollBar()->maximum() == verticalScrollBar()->minimum())) {
        painter.fillRect(QRect(QPoint((int)er.left(), (int)offset.y()), er.bottomRight()), palette().background());
    }
}


void QPlainTextEditPrivate::updateDefaultTextOption()
{
    QTextDocument *doc = control->document();

    QTextOption opt = doc->defaultTextOption();
    QTextOption::WrapMode oldWrapMode = opt.wrapMode();

    if (lineWrap == QPlainTextEdit::NoWrap)
        opt.setWrapMode(QTextOption::NoWrap);
    else
        opt.setWrapMode(wordWrap);

    if (opt.wrapMode() != oldWrapMode)
        doc->setDefaultTextOption(opt);
}


/*! \reimp
*/
void QPlainTextEdit::mousePressEvent(QMouseEvent *e)
{
    Q_D(QPlainTextEdit);
#ifdef QT_KEYPAD_NAVIGATION
    if (QApplication::keypadNavigationEnabled() && !hasEditFocus())
        setEditFocus(true);
#endif
    d->sendControlEvent(e);
}

/*! \reimp
*/
void QPlainTextEdit::mouseMoveEvent(QMouseEvent *e)
{
    Q_D(QPlainTextEdit);
    d->inDrag = false; // paranoia
    const QPoint pos = e->pos();
    d->sendControlEvent(e);
    if (!(e->buttons() & Qt::LeftButton))
        return;
    QRect visible = d->viewport->rect();
    if (visible.contains(pos))
        d->autoScrollTimer.stop();
    else if (!d->autoScrollTimer.isActive())
        d->autoScrollTimer.start(100, this);
}

/*! \reimp
*/
void QPlainTextEdit::mouseReleaseEvent(QMouseEvent *e)
{
    Q_D(QPlainTextEdit);
    d->sendControlEvent(e);
    if (d->autoScrollTimer.isActive()) {
        d->autoScrollTimer.stop();
        d->ensureCursorVisible();
    }

    if (!isReadOnly() && rect().contains(e->pos()))
        d->handleSoftwareInputPanel(e->button(), d->clickCausedFocus);
    d->clickCausedFocus = 0;
}

/*! \reimp
*/
void QPlainTextEdit::mouseDoubleClickEvent(QMouseEvent *e)
{
    Q_D(QPlainTextEdit);
    d->sendControlEvent(e);
}

/*! \reimp
*/
bool QPlainTextEdit::focusNextPrevChild(bool next)
{
    Q_D(const QPlainTextEdit);
    if (!d->tabChangesFocus && d->control->textInteractionFlags() & Qt::TextEditable)
        return false;
    return QAbstractScrollArea::focusNextPrevChild(next);
}

#ifndef QT_NO_CONTEXTMENU
/*!
  \fn void QPlainTextEdit::contextMenuEvent(QContextMenuEvent *event)

  Shows the standard context menu created with createStandardContextMenu().

  If you do not want the text edit to have a context menu, you can set
  its \l contextMenuPolicy to Qt::NoContextMenu. If you want to
  customize the context menu, reimplement this function. If you want
  to extend the standard context menu, reimplement this function, call
  createStandardContextMenu() and extend the menu returned.

  Information about the event is passed in the \a event object.

  \snippet doc/src/snippets/code/src_gui_widgets_qplaintextedit.cpp 0
*/
void QPlainTextEdit::contextMenuEvent(QContextMenuEvent *e)
{
    Q_D(QPlainTextEdit);
    d->sendControlEvent(e);
}
#endif // QT_NO_CONTEXTMENU

#ifndef QT_NO_DRAGANDDROP
/*! \reimp
*/
void QPlainTextEdit::dragEnterEvent(QDragEnterEvent *e)
{
    Q_D(QPlainTextEdit);
    d->inDrag = true;
    d->sendControlEvent(e);
}

/*! \reimp
*/
void QPlainTextEdit::dragLeaveEvent(QDragLeaveEvent *e)
{
    Q_D(QPlainTextEdit);
    d->inDrag = false;
    d->autoScrollTimer.stop();
    d->sendControlEvent(e);
}

/*! \reimp
*/
void QPlainTextEdit::dragMoveEvent(QDragMoveEvent *e)
{
    Q_D(QPlainTextEdit);
    d->autoScrollDragPos = e->pos();
    if (!d->autoScrollTimer.isActive())
        d->autoScrollTimer.start(100, this);
    d->sendControlEvent(e);
}

/*! \reimp
*/
void QPlainTextEdit::dropEvent(QDropEvent *e)
{
    Q_D(QPlainTextEdit);
    d->inDrag = false;
    d->autoScrollTimer.stop();
    d->sendControlEvent(e);
}

#endif // QT_NO_DRAGANDDROP

/*! \reimp
 */
void QPlainTextEdit::inputMethodEvent(QInputMethodEvent *e)
{
    Q_D(QPlainTextEdit);
#ifdef QT_KEYPAD_NAVIGATION
    if (d->control->textInteractionFlags() & Qt::TextEditable
        && QApplication::keypadNavigationEnabled()
        && !hasEditFocus()) {
        setEditFocus(true);
        selectAll();    // so text is replaced rather than appended to
    }
#endif
    d->sendControlEvent(e);
    ensureCursorVisible();
}

/*!\reimp
*/
void QPlainTextEdit::scrollContentsBy(int dx, int /*dy*/)
{
    Q_D(QPlainTextEdit);
    d->setTopLine(d->vbar->value(), dx);
}

/*!\reimp
*/
QVariant QPlainTextEdit::inputMethodQuery(Qt::InputMethodQuery property) const
{
    Q_D(const QPlainTextEdit);
    QVariant v = d->control->inputMethodQuery(property);
    const QPoint offset(-d->horizontalOffset(), -0);
    if (v.type() == QVariant::RectF)
        v = v.toRectF().toRect().translated(offset);
    else if (v.type() == QVariant::PointF)
        v = v.toPointF().toPoint() + offset;
    else if (v.type() == QVariant::Rect)
        v = v.toRect().translated(offset);
    else if (v.type() == QVariant::Point)
        v = v.toPoint() + offset;
    return v;
}

/*! \reimp
*/
void QPlainTextEdit::focusInEvent(QFocusEvent *e)
{
    Q_D(QPlainTextEdit);
    if (e->reason() == Qt::MouseFocusReason) {
        d->clickCausedFocus = 1;
    }
    QAbstractScrollArea::focusInEvent(e);
    d->sendControlEvent(e);
}

/*! \reimp
*/
void QPlainTextEdit::focusOutEvent(QFocusEvent *e)
{
    Q_D(QPlainTextEdit);
    QAbstractScrollArea::focusOutEvent(e);
    d->sendControlEvent(e);
}

/*! \reimp
*/
void QPlainTextEdit::showEvent(QShowEvent *)
{
    Q_D(QPlainTextEdit);
    if (d->showCursorOnInitialShow) {
        d->showCursorOnInitialShow = false;
        ensureCursorVisible();
    }
}

/*! \reimp
*/
void QPlainTextEdit::changeEvent(QEvent *e)
{
    Q_D(QPlainTextEdit);
    QAbstractScrollArea::changeEvent(e);
    if (e->type() == QEvent::ApplicationFontChange
        || e->type() == QEvent::FontChange) {
        d->control->document()->setDefaultFont(font());
    }  else if(e->type() == QEvent::ActivationChange) {
        if (!isActiveWindow())
            d->autoScrollTimer.stop();
    } else if (e->type() == QEvent::EnabledChange) {
        e->setAccepted(isEnabled());
        d->sendControlEvent(e);
    } else if (e->type() == QEvent::PaletteChange) {
        d->control->setPalette(palette());
    } else if (e->type() == QEvent::LayoutDirectionChange) {
        d->sendControlEvent(e);
    }
}

/*! \reimp
*/
#ifndef QT_NO_WHEELEVENT
void QPlainTextEdit::wheelEvent(QWheelEvent *e)
{
    QAbstractScrollArea::wheelEvent(e);
    updateMicroFocus();
}
#endif

#ifndef QT_NO_CONTEXTMENU
/*!  This function creates the standard context menu which is shown
  when the user clicks on the line edit with the right mouse
  button. It is called from the default contextMenuEvent() handler.
  The popup menu's ownership is transferred to the caller.
*/

QMenu *QPlainTextEdit::createStandardContextMenu()
{
    Q_D(QPlainTextEdit);
    return d->control->createStandardContextMenu(QPointF(), this);
}
#endif // QT_NO_CONTEXTMENU

/*!
  returns a QTextCursor at position \a pos (in viewport coordinates).
*/
QTextCursor QPlainTextEdit::cursorForPosition(const QPoint &pos) const
{
    Q_D(const QPlainTextEdit);
    return d->control->cursorForPosition(d->mapToContents(pos));
}

/*!
  returns a rectangle (in viewport coordinates) that includes the
  \a cursor.
 */
QRect QPlainTextEdit::cursorRect(const QTextCursor &cursor) const
{
    Q_D(const QPlainTextEdit);
    if (cursor.isNull())
        return QRect();

    QRect r = d->control->cursorRect(cursor).toRect();
    r.translate(-d->horizontalOffset(),-d->verticalOffset());
    return r;
}

/*!
  returns a rectangle (in viewport coordinates) that includes the
  cursor of the text edit.
 */
QRect QPlainTextEdit::cursorRect() const
{
    Q_D(const QPlainTextEdit);
    QRect r = d->control->cursorRect().toRect();
    r.translate(-d->horizontalOffset(),-d->verticalOffset());
    return r;
}


/*!
   \property QPlainTextEdit::overwriteMode
   \brief whether text entered by the user will overwrite existing text

   As with many text editors, the plain text editor widget can be configured
   to insert or overwrite existing text with new text entered by the user.

   If this property is true, existing text is overwritten, character-for-character
   by new text; otherwise, text is inserted at the cursor position, displacing
   existing text.

   By default, this property is false (new text does not overwrite existing text).
*/

bool QPlainTextEdit::overwriteMode() const
{
    Q_D(const QPlainTextEdit);
    return d->control->overwriteMode();
}

void QPlainTextEdit::setOverwriteMode(bool overwrite)
{
    Q_D(QPlainTextEdit);
    d->control->setOverwriteMode(overwrite);
}

/*!
    \property QPlainTextEdit::tabStopWidth
    \brief the tab stop width in pixels

    By default, this property contains a value of 80.
*/

int QPlainTextEdit::tabStopWidth() const
{
    Q_D(const QPlainTextEdit);
    return qRound(d->control->document()->defaultTextOption().tabStop());
}

void QPlainTextEdit::setTabStopWidth(int width)
{
    Q_D(QPlainTextEdit);
    QTextOption opt = d->control->document()->defaultTextOption();
    if (opt.tabStop() == width || width < 0)
        return;
    opt.setTabStop(width);
    d->control->document()->setDefaultTextOption(opt);
}

/*!
    \property QPlainTextEdit::cursorWidth

    This property specifies the width of the cursor in pixels. The default value is 1.
*/
int QPlainTextEdit::cursorWidth() const
{
    Q_D(const QPlainTextEdit);
    return d->control->cursorWidth();
}

void QPlainTextEdit::setCursorWidth(int width)
{
    Q_D(QPlainTextEdit);
    d->control->setCursorWidth(width);
}



/*!
    This function allows temporarily marking certain regions in the document
    with a given color, specified as \a selections. This can be useful for
    example in a programming editor to mark a whole line of text with a given
    background color to indicate the existence of a breakpoint.

    \sa QTextEdit::ExtraSelection, extraSelections()
*/
void QPlainTextEdit::setExtraSelections(const QList<QTextEdit::ExtraSelection> &selections)
{
    Q_D(QPlainTextEdit);
    d->control->setExtraSelections(selections);
}

/*!
    Returns previously set extra selections.

    \sa setExtraSelections()
*/
QList<QTextEdit::ExtraSelection> QPlainTextEdit::extraSelections() const
{
    Q_D(const QPlainTextEdit);
    return d->control->extraSelections();
}

/*!
    This function returns a new MIME data object to represent the contents
    of the text edit's current selection. It is called when the selection needs
    to be encapsulated into a new QMimeData object; for example, when a drag
    and drop operation is started, or when data is copied to the clipboard.

    If you reimplement this function, note that the ownership of the returned
    QMimeData object is passed to the caller. The selection can be retrieved
    by using the textCursor() function.
*/
QMimeData *QPlainTextEdit::createMimeDataFromSelection() const
{
    Q_D(const QPlainTextEdit);
    return d->control->QTextControl::createMimeDataFromSelection();
}

/*!
    This function returns true if the contents of the MIME data object, specified
    by \a source, can be decoded and inserted into the document. It is called
    for example when during a drag operation the mouse enters this widget and it
    is necessary to determine whether it is possible to accept the drag.
 */
bool QPlainTextEdit::canInsertFromMimeData(const QMimeData *source) const
{
    Q_D(const QPlainTextEdit);
    return d->control->QTextControl::canInsertFromMimeData(source);
}

/*!
    This function inserts the contents of the MIME data object, specified
    by \a source, into the text edit at the current cursor position. It is
    called whenever text is inserted as the result of a clipboard paste
    operation, or when the text edit accepts data from a drag and drop
    operation.
*/
void QPlainTextEdit::insertFromMimeData(const QMimeData *source)
{
    Q_D(QPlainTextEdit);
    d->control->QTextControl::insertFromMimeData(source);
}

/*!
    \property QPlainTextEdit::readOnly
    \brief whether the text edit is read-only

    In a read-only text edit the user can only navigate through the
    text and select text; modifying the text is not possible.

    This property's default is false.
*/

bool QPlainTextEdit::isReadOnly() const
{
    Q_D(const QPlainTextEdit);
    return !(d->control->textInteractionFlags() & Qt::TextEditable);
}

void QPlainTextEdit::setReadOnly(bool ro)
{
    Q_D(QPlainTextEdit);
    Qt::TextInteractionFlags flags = Qt::NoTextInteraction;
    if (ro) {
        flags = Qt::TextSelectableByMouse;
    } else {
        flags = Qt::TextEditorInteraction;
    }
    setAttribute(Qt::WA_InputMethodEnabled, shouldEnableInputMethod(this));
    d->control->setTextInteractionFlags(flags);
}

/*!
    \property QPlainTextEdit::textInteractionFlags

    Specifies how the label should interact with user input if it displays text.

    If the flags contain either Qt::LinksAccessibleByKeyboard or Qt::TextSelectableByKeyboard
    then the focus policy is also automatically set to Qt::ClickFocus.

    The default value depends on whether the QPlainTextEdit is read-only
    or editable.
*/

void QPlainTextEdit::setTextInteractionFlags(Qt::TextInteractionFlags flags)
{
    Q_D(QPlainTextEdit);
    d->control->setTextInteractionFlags(flags);
}

Qt::TextInteractionFlags QPlainTextEdit::textInteractionFlags() const
{
    Q_D(const QPlainTextEdit);
    return d->control->textInteractionFlags();
}

/*!
    Merges the properties specified in \a modifier into the current character
    format by calling QTextCursor::mergeCharFormat on the editor's cursor.
    If the editor has a selection then the properties of \a modifier are
    directly applied to the selection.

    \sa QTextCursor::mergeCharFormat()
 */
void QPlainTextEdit::mergeCurrentCharFormat(const QTextCharFormat &modifier)
{
    Q_D(QPlainTextEdit);
    d->control->mergeCurrentCharFormat(modifier);
}

/*!
    Sets the char format that is be used when inserting new text to \a
    format by calling QTextCursor::setCharFormat() on the editor's
    cursor.  If the editor has a selection then the char format is
    directly applied to the selection.
 */
void QPlainTextEdit::setCurrentCharFormat(const QTextCharFormat &format)
{
    Q_D(QPlainTextEdit);
    d->control->setCurrentCharFormat(format);
}

/*!
    Returns the char format that is used when inserting new text.
 */
QTextCharFormat QPlainTextEdit::currentCharFormat() const
{
    Q_D(const QPlainTextEdit);
    return d->control->currentCharFormat();
}



/*!
    Convenience slot that inserts \a text at the current
    cursor position.

    It is equivalent to

    \snippet doc/src/snippets/code/src_gui_widgets_qplaintextedit.cpp 1
 */
void QPlainTextEdit::insertPlainText(const QString &text)
{
    Q_D(QPlainTextEdit);
    d->control->insertPlainText(text);
}


/*!
    Moves the cursor by performing the given \a operation.

    If \a mode is QTextCursor::KeepAnchor, the cursor selects the text it moves over.
    This is the same effect that the user achieves when they hold down the Shift key
    and move the cursor with the cursor keys.

    \sa QTextCursor::movePosition()
*/
void QPlainTextEdit::moveCursor(QTextCursor::MoveOperation operation, QTextCursor::MoveMode mode)
{
    Q_D(QPlainTextEdit);
    d->control->moveCursor(operation, mode);
}

/*!
    Returns whether text can be pasted from the clipboard into the textedit.
*/
bool QPlainTextEdit::canPaste() const
{
    Q_D(const QPlainTextEdit);
    return d->control->canPaste();
}

#ifndef QT_NO_PRINTER
/*!
    Convenience function to print the text edit's document to the given \a printer. This
    is equivalent to calling the print method on the document directly except that this
    function also supports QPrinter::Selection as print range.

    \sa QTextDocument::print()
*/
void QPlainTextEdit::print(QPrinter *printer) const
{
    Q_D(const QPlainTextEdit);
    d->control->print(printer);
}
#endif // QT _NO_PRINTER

/*! \property QPlainTextEdit::tabChangesFocus
  \brief whether \gui Tab changes focus or is accepted as input

  In some occasions text edits should not allow the user to input
  tabulators or change indentation using the \gui Tab key, as this breaks
  the focus chain. The default is false.

*/

bool QPlainTextEdit::tabChangesFocus() const
{
    Q_D(const QPlainTextEdit);
    return d->tabChangesFocus;
}

void QPlainTextEdit::setTabChangesFocus(bool b)
{
    Q_D(QPlainTextEdit);
    d->tabChangesFocus = b;
}

/*!
    \property QPlainTextEdit::documentTitle
    \brief the title of the document parsed from the text.

    By default, this property contains an empty string.
*/

/*!
    \property QPlainTextEdit::lineWrapMode
    \brief the line wrap mode

    The default mode is WidgetWidth which causes words to be
    wrapped at the right edge of the text edit. Wrapping occurs at
    whitespace, keeping whole words intact. If you want wrapping to
    occur within words use setWordWrapMode().
*/

QPlainTextEdit::LineWrapMode QPlainTextEdit::lineWrapMode() const
{
    Q_D(const QPlainTextEdit);
    return d->lineWrap;
}

void QPlainTextEdit::setLineWrapMode(LineWrapMode wrap)
{
    Q_D(QPlainTextEdit);
    if (d->lineWrap == wrap)
        return;
    d->lineWrap = wrap;
    d->updateDefaultTextOption();
    d->relayoutDocument();
    d->_q_adjustScrollbars();
    ensureCursorVisible();
}

/*!
    \property QPlainTextEdit::wordWrapMode
    \brief the mode QPlainTextEdit will use when wrapping text by words

    By default, this property is set to QTextOption::WrapAtWordBoundaryOrAnywhere.

    \sa QTextOption::WrapMode
*/

QTextOption::WrapMode QPlainTextEdit::wordWrapMode() const
{
    Q_D(const QPlainTextEdit);
    return d->wordWrap;
}

void QPlainTextEdit::setWordWrapMode(QTextOption::WrapMode mode)
{
    Q_D(QPlainTextEdit);
    if (mode == d->wordWrap)
        return;
    d->wordWrap = mode;
    d->updateDefaultTextOption();
}

/*!
    \property QPlainTextEdit::backgroundVisible
    \brief whether the palette background is visible outside the document area

    If set to true, the plain text edit paints the palette background
    on the viewport area not covered by the text document. Otherwise,
    if set to false, it won't. The feature makes it possible for
    the user to visually distinguish between the area of the document,
    painted with the base color of the palette, and the empty
    area not covered by any document.

    The default is false.
*/

bool QPlainTextEdit::backgroundVisible() const
{
    Q_D(const QPlainTextEdit);
    return d->backgroundVisible;
}

void QPlainTextEdit::setBackgroundVisible(bool visible)
{
    Q_D(QPlainTextEdit);
    if (visible == d->backgroundVisible)
        return;
    d->backgroundVisible = visible;
    d->updateViewport();
}

/*!
    \property QPlainTextEdit::centerOnScroll
    \brief whether the cursor should be centered on screen

    If set to true, the plain text edit scrolls the document
    vertically to make the cursor visible at the center of the
    viewport. This also allows the text edit to scroll below the end
    of the document. Otherwise, if set to false, the plain text edit
    scrolls the smallest amount possible to ensure the cursor is
    visible.  The same algorithm is applied to any new line appended
    through appendPlainText().

    The default is false.

    \sa centerCursor(), ensureCursorVisible()
*/

bool QPlainTextEdit::centerOnScroll() const
{
    Q_D(const QPlainTextEdit);
    return d->centerOnScroll;
}

void QPlainTextEdit::setCenterOnScroll(bool enabled)
{
    Q_D(QPlainTextEdit);
    if (enabled == d->centerOnScroll)
        return;
    d->centerOnScroll = enabled;
}



/*!
    Finds the next occurrence of the string, \a exp, using the given
    \a options. Returns true if \a exp was found and changes the
    cursor to select the match; otherwise returns false.
*/
bool QPlainTextEdit::find(const QString &exp, QTextDocument::FindFlags options)
{
    Q_D(QPlainTextEdit);
    return d->control->find(exp, options);
}

/*!
    \fn void QPlainTextEdit::copyAvailable(bool yes)

    This signal is emitted when text is selected or de-selected in the
    text edit.

    When text is selected this signal will be emitted with \a yes set
    to true. If no text has been selected or if the selected text is
    de-selected this signal is emitted with \a yes set to false.

    If \a yes is true then copy() can be used to copy the selection to
    the clipboard. If \a yes is false then copy() does nothing.

    \sa selectionChanged()
*/


/*!
    \fn void QPlainTextEdit::selectionChanged()

    This signal is emitted whenever the selection changes.

    \sa copyAvailable()
*/

/*!
    \fn void QPlainTextEdit::cursorPositionChanged()

    This signal is emitted whenever the position of the
    cursor changed.
*/



/*!
    \fn void QPlainTextEdit::updateRequest(const QRect &rect, int dy)

    This signal is emitted when the text document needs an update of
    the specified \a rect. If the text is scrolled, \a rect will cover
    the entire viewport area. If the text is scrolled vertically, \a
    dy carries the amount of pixels the viewport was scrolled.

    The purpose of the signal is to support extra widgets in plain
    text edit subclasses that e.g. show line numbers, breakpoints, or
    other extra information.
*/

/*!  \fn void QPlainTextEdit::blockCountChanged(int newBlockCount);

    This signal is emitted whenever the block count changes. The new
    block count is passed in \a newBlockCount.
*/

/*!  \fn void QPlainTextEdit::modificationChanged(bool changed);

    This signal is emitted whenever the content of the document
    changes in a way that affects the modification state. If \a
    changed is true, the document has been modified; otherwise it is
    false.

    For example, calling setModified(false) on a document and then
    inserting text causes the signal to get emitted. If you undo that
    operation, causing the document to return to its original
    unmodified state, the signal will get emitted again.
*/




void QPlainTextEditPrivate::append(const QString &text, Qt::TextFormat format)
{
    Q_Q(QPlainTextEdit);

    QTextDocument *document = control->document();
    QPlainTextDocumentLayout *documentLayout = qobject_cast<QPlainTextDocumentLayout*>(document->documentLayout());
    Q_ASSERT(documentLayout);

    int maximumBlockCount = document->maximumBlockCount();
    if (maximumBlockCount)
        document->setMaximumBlockCount(0);

    const bool atBottom =  q->isVisible()
                           && (control->blockBoundingRect(document->lastBlock()).bottom() - verticalOffset()
                               <= viewport->rect().bottom());

    if (!q->isVisible())
        showCursorOnInitialShow = true;

    bool documentSizeChangedBlocked = documentLayout->priv()->blockDocumentSizeChanged;
    documentLayout->priv()->blockDocumentSizeChanged = true;

    if (format == Qt::RichText)
        control->appendHtml(text);
    else if (format == Qt::PlainText)
        control->appendPlainText(text);
    else
        control->append(text);

    if (maximumBlockCount > 0) {
        if (document->blockCount() > maximumBlockCount) {
            bool blockUpdate = false;
            if (control->topBlock) {
                control->topBlock--;
                blockUpdate = true;
                emit q->updateRequest(viewport->rect(), 0);
            }

            bool updatesBlocked = documentLayout->priv()->blockUpdate;
            documentLayout->priv()->blockUpdate = blockUpdate;
            QTextCursor cursor(document);
            cursor.movePosition(QTextCursor::NextBlock, QTextCursor::KeepAnchor);
            cursor.removeSelectedText();
            documentLayout->priv()->blockUpdate = updatesBlocked;
        }
        document->setMaximumBlockCount(maximumBlockCount);
    }

    documentLayout->priv()->blockDocumentSizeChanged = documentSizeChangedBlocked;
    _q_adjustScrollbars();


    if (atBottom) {
        const bool needScroll =  !centerOnScroll
                                 || control->blockBoundingRect(document->lastBlock()).bottom() - verticalOffset()
                                 > viewport->rect().bottom();
        if (needScroll)
            vbar->setValue(vbar->maximum());
    }
}


/*!
    Appends a new paragraph with \a text to the end of the text edit.

    \sa appendHtml()
*/

void QPlainTextEdit::appendPlainText(const QString &text)
{
    Q_D(QPlainTextEdit);
    d->append(text, Qt::PlainText);
}

/*!
    Appends a new paragraph with \a html to the end of the text edit.

    appendPlainText()
*/

void QPlainTextEdit::appendHtml(const QString &html)
{
    Q_D(QPlainTextEdit);
    d->append(html, Qt::RichText);
}

void QPlainTextEditPrivate::ensureCursorVisible(bool center)
{
    Q_Q(QPlainTextEdit);
    QRect visible = viewport->rect();
    QRect cr = q->cursorRect();
    if (cr.top() < visible.top() || cr.bottom() > visible.bottom()) {
        ensureVisible(control->textCursor().position(), center);
    }

    const bool rtl = q->isRightToLeft();
    if (cr.left() < visible.left() || cr.right() > visible.right()) {
        int x = cr.center().x() + horizontalOffset() - visible.width()/2;
        hbar->setValue(rtl ? hbar->maximum() - x : x);
    }
}

/*!
    Ensures that the cursor is visible by scrolling the text edit if
    necessary.

    \sa centerCursor(), centerOnScroll
*/
void QPlainTextEdit::ensureCursorVisible()
{
    Q_D(QPlainTextEdit);
    d->ensureCursorVisible(d->centerOnScroll);
}


/*!  Scrolls the document in order to center the cursor vertically.

\sa ensureCursorVisible(), centerOnScroll
 */
void QPlainTextEdit::centerCursor()
{
    Q_D(QPlainTextEdit);
    d->ensureVisible(textCursor().position(), true, true);
}

/*!
  Returns the first visible block.

  \sa blockBoundingRect()
 */
QTextBlock QPlainTextEdit::firstVisibleBlock() const
{
    Q_D(const QPlainTextEdit);
    return d->control->firstVisibleBlock();
}

/*!  Returns the content's origin in viewport coordinates.

     The origin of the content of a plain text edit is always the top
     left corner of the first visible text block. The content offset
     is different from (0,0) when the text has been scrolled
     horizontally, or when the first visible block has been scrolled
     partially off the screen, i.e. the visible text does not start
     with the first line of the first visible block, or when the first
     visible block is the very first block and the editor displays a
     margin.

     \sa firstVisibleBlock(), horizontalScrollBar(), verticalScrollBar()
 */
QPointF QPlainTextEdit::contentOffset() const
{
    Q_D(const QPlainTextEdit);
    return QPointF(-d->horizontalOffset(), -d->verticalOffset());
}


/*!  Returns the bounding rectangle of the text \a block in content
  coordinates. Translate the rectangle with the contentOffset() to get
  visual coordinates on the viewport.

  \sa firstVisibleBlock(), blockBoundingRect()
 */
QRectF QPlainTextEdit::blockBoundingGeometry(const QTextBlock &block) const
{
    Q_D(const QPlainTextEdit);
    return d->control->blockBoundingRect(block);
}

/*!
  Returns the bounding rectangle of the text \a block in the block's own coordinates.

  \sa blockBoundingGeometry()
 */
QRectF QPlainTextEdit::blockBoundingRect(const QTextBlock &block) const
{
    QPlainTextDocumentLayout *documentLayout = qobject_cast<QPlainTextDocumentLayout*>(document()->documentLayout());
    Q_ASSERT(documentLayout);
    return documentLayout->blockBoundingRect(block);
}

/*!
    \property QPlainTextEdit::blockCount
    \brief the number of text blocks in the document.

    By default, in an empty document, this property contains a value of 1.
*/
int QPlainTextEdit::blockCount() const
{
    return document()->blockCount();
}

/*!  Returns the paint context for the viewport(), useful only when
  reimplementing paintEvent().
 */
QAbstractTextDocumentLayout::PaintContext QPlainTextEdit::getPaintContext() const
{
    Q_D(const QPlainTextEdit);
    return d->control->getPaintContext(d->viewport);
}

/*!
    \property QPlainTextEdit::maximumBlockCount
    \brief the limit for blocks in the document.

    Specifies the maximum number of blocks the document may have. If there are
    more blocks in the document that specified with this property blocks are removed
    from the beginning of the document.

    A negative or zero value specifies that the document may contain an unlimited
    amount of blocks.

    The default value is 0.

    Note that setting this property will apply the limit immediately to the document
    contents. Setting this property also disables the undo redo history.

*/


/*!
    \fn void QPlainTextEdit::textChanged()

    This signal is emitted whenever the document's content changes; for
    example, when text is inserted or deleted, or when formatting is applied.
*/

/*!
    \fn void QPlainTextEdit::undoAvailable(bool available)

    This signal is emitted whenever undo operations become available
    (\a available is true) or unavailable (\a available is false).
*/

/*!
    \fn void QPlainTextEdit::redoAvailable(bool available)

    This signal is emitted whenever redo operations become available
    (\a available is true) or unavailable (\a available is false).
*/

QT_END_NAMESPACE

#include "moc_qplaintextedit.cpp"
#include "moc_qplaintextedit_p.cpp"

#endif // QT_NO_TEXTEDIT
