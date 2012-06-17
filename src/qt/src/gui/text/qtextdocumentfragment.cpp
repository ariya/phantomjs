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

#include "qtextdocumentfragment.h"
#include "qtextdocumentfragment_p.h"
#include "qtextcursor_p.h"
#include "qtextlist.h"

#include <qdebug.h>
#include <qtextcodec.h>
#include <qbytearray.h>
#include <qdatastream.h>
#include <qdatetime.h>

QT_BEGIN_NAMESPACE

QTextCopyHelper::QTextCopyHelper(const QTextCursor &_source, const QTextCursor &_destination, bool forceCharFormat, const QTextCharFormat &fmt)
#if defined(Q_CC_DIAB) // compiler bug
    : formatCollection(*_destination.d->priv->formatCollection()), originalText((const QString)_source.d->priv->buffer())
#else
    : formatCollection(*_destination.d->priv->formatCollection()), originalText(_source.d->priv->buffer())
#endif
{
    src = _source.d->priv;
    dst = _destination.d->priv;
    insertPos = _destination.position();
    this->forceCharFormat = forceCharFormat;
    primaryCharFormatIndex = convertFormatIndex(fmt);
    cursor = _source;
}

int QTextCopyHelper::convertFormatIndex(const QTextFormat &oldFormat, int objectIndexToSet)
{
    QTextFormat fmt = oldFormat;
    if (objectIndexToSet != -1) {
        fmt.setObjectIndex(objectIndexToSet);
    } else if (fmt.objectIndex() != -1) {
        int newObjectIndex = objectIndexMap.value(fmt.objectIndex(), -1);
        if (newObjectIndex == -1) {
            QTextFormat objFormat = src->formatCollection()->objectFormat(fmt.objectIndex());
            Q_ASSERT(objFormat.objectIndex() == -1);
            newObjectIndex = formatCollection.createObjectIndex(objFormat);
            objectIndexMap.insert(fmt.objectIndex(), newObjectIndex);
        }
        fmt.setObjectIndex(newObjectIndex);
    }
    int idx = formatCollection.indexForFormat(fmt);
    Q_ASSERT(formatCollection.format(idx).type() == oldFormat.type());
    return idx;
}

int QTextCopyHelper::appendFragment(int pos, int endPos, int objectIndex)
{
    QTextDocumentPrivate::FragmentIterator fragIt = src->find(pos);
    const QTextFragmentData * const frag = fragIt.value();

    Q_ASSERT(objectIndex == -1
             || (frag->size_array[0] == 1 && src->formatCollection()->format(frag->format).objectIndex() != -1));

    int charFormatIndex;
    if (forceCharFormat)
       charFormatIndex = primaryCharFormatIndex;
    else
       charFormatIndex = convertFormatIndex(frag->format, objectIndex);

    const int inFragmentOffset = qMax(0, pos - fragIt.position());
    int charsToCopy = qMin(int(frag->size_array[0] - inFragmentOffset), endPos - pos);

    QTextBlock nextBlock = src->blocksFind(pos + 1);

    int blockIdx = -2;
    if (nextBlock.position() == pos + 1) {
        blockIdx = convertFormatIndex(nextBlock.blockFormat());
    } else if (pos == 0 && insertPos == 0) {
        dst->setBlockFormat(dst->blocksBegin(), dst->blocksBegin(), convertFormat(src->blocksBegin().blockFormat()).toBlockFormat());
        dst->setCharFormat(-1, 1, convertFormat(src->blocksBegin().charFormat()).toCharFormat());
    }

    QString txtToInsert(originalText.constData() + frag->stringPosition + inFragmentOffset, charsToCopy);
    if (txtToInsert.length() == 1
        && (txtToInsert.at(0) == QChar::ParagraphSeparator
            || txtToInsert.at(0) == QTextBeginningOfFrame
            || txtToInsert.at(0) == QTextEndOfFrame
           )
       ) {
        dst->insertBlock(txtToInsert.at(0), insertPos, blockIdx, charFormatIndex);
        ++insertPos;
    } else {
        if (nextBlock.textList()) {
            QTextBlock dstBlock = dst->blocksFind(insertPos);
            if (!dstBlock.textList()) {
                // insert a new text block with the block and char format from the
                // source block to make sure that the following text fragments
                // end up in a list as they should
                int listBlockFormatIndex = convertFormatIndex(nextBlock.blockFormat());
                int listCharFormatIndex = convertFormatIndex(nextBlock.charFormat());
                dst->insertBlock(insertPos, listBlockFormatIndex, listCharFormatIndex);
                ++insertPos;
            }
        }
        dst->insert(insertPos, txtToInsert, charFormatIndex);
        const int userState = nextBlock.userState();
        if (userState != -1)
            dst->blocksFind(insertPos).setUserState(userState);
        insertPos += txtToInsert.length();
    }

    return charsToCopy;
}

void QTextCopyHelper::appendFragments(int pos, int endPos)
{
    Q_ASSERT(pos < endPos);

    while (pos < endPos)
        pos += appendFragment(pos, endPos);
}

void QTextCopyHelper::copy()
{
    if (cursor.hasComplexSelection()) {
        QTextTable *table = cursor.currentTable();
        int row_start, col_start, num_rows, num_cols;
        cursor.selectedTableCells(&row_start, &num_rows, &col_start, &num_cols);

        QTextTableFormat tableFormat = table->format();
        tableFormat.setColumns(num_cols);
        tableFormat.clearColumnWidthConstraints();
        const int objectIndex = dst->formatCollection()->createObjectIndex(tableFormat);

        Q_ASSERT(row_start != -1);
        for (int r = row_start; r < row_start + num_rows; ++r) {
            for (int c = col_start; c < col_start + num_cols; ++c) {
                QTextTableCell cell = table->cellAt(r, c);
                const int rspan = cell.rowSpan();
                const int cspan = cell.columnSpan();
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

                // add the QTextBeginningOfFrame
                QTextCharFormat cellFormat = cell.format();
                if (r + rspan >= row_start + num_rows) {
                    cellFormat.setTableCellRowSpan(row_start + num_rows - r);
                }
                if (c + cspan >= col_start + num_cols) {
                    cellFormat.setTableCellColumnSpan(col_start + num_cols - c);
                }
                const int charFormatIndex = convertFormatIndex(cellFormat, objectIndex);

                int blockIdx = -2;
                const int cellPos = cell.firstPosition();
                QTextBlock block = src->blocksFind(cellPos);
                if (block.position() == cellPos) {
                    blockIdx = convertFormatIndex(block.blockFormat());
                }

                dst->insertBlock(QTextBeginningOfFrame, insertPos, blockIdx, charFormatIndex);
                ++insertPos;

                // nothing to add for empty cells
                if (cell.lastPosition() > cellPos) {
                    // add the contents
                    appendFragments(cellPos, cell.lastPosition());
                }
            }
        }

        // add end of table
        int end = table->lastPosition();
        appendFragment(end, end+1, objectIndex);
    } else {
        appendFragments(cursor.selectionStart(), cursor.selectionEnd());
    }
}

QTextDocumentFragmentPrivate::QTextDocumentFragmentPrivate(const QTextCursor &_cursor)
    : ref(1), doc(new QTextDocument), importedFromPlainText(false)
{
    doc->setUndoRedoEnabled(false);

    if (!_cursor.hasSelection())
        return;

    doc->docHandle()->beginEditBlock();
    QTextCursor destCursor(doc);
    QTextCopyHelper(_cursor, destCursor).copy();
    doc->docHandle()->endEditBlock();

    if (_cursor.d)
        doc->docHandle()->mergeCachedResources(_cursor.d->priv);
}

void QTextDocumentFragmentPrivate::insert(QTextCursor &_cursor) const
{
    if (_cursor.isNull())
        return;

    QTextDocumentPrivate *destPieceTable = _cursor.d->priv;
    destPieceTable->beginEditBlock();

    QTextCursor sourceCursor(doc);
    sourceCursor.movePosition(QTextCursor::End, QTextCursor::KeepAnchor);
    QTextCopyHelper(sourceCursor, _cursor, importedFromPlainText, _cursor.charFormat()).copy();

    destPieceTable->endEditBlock();
}

/*!
    \class QTextDocumentFragment
    \reentrant

    \brief The QTextDocumentFragment class represents a piece of formatted text
    from a QTextDocument.

    \ingroup richtext-processing
    \ingroup shared

    A QTextDocumentFragment is a fragment of rich text, that can be inserted into
    a QTextDocument. A document fragment can be created from a
    QTextDocument, from a QTextCursor's selection, or from another
    document fragment. Document fragments can also be created by the
    static functions, fromPlainText() and fromHtml().

    The contents of a document fragment can be obtained as plain text
    by using the toPlainText() function, or it can be obtained as HTML
    with toHtml().
*/


/*!
    Constructs an empty QTextDocumentFragment.

    \sa isEmpty()
*/
QTextDocumentFragment::QTextDocumentFragment()
    : d(0)
{
}

/*!
    Converts the given \a document into a QTextDocumentFragment.
    Note that the QTextDocumentFragment only stores the document contents, not meta information
    like the document's title.
*/
QTextDocumentFragment::QTextDocumentFragment(const QTextDocument *document)
    : d(0)
{
    if (!document)
        return;

    QTextCursor cursor(const_cast<QTextDocument *>(document));
    cursor.movePosition(QTextCursor::End, QTextCursor::KeepAnchor);
    d = new QTextDocumentFragmentPrivate(cursor);
}

/*!
    Creates a QTextDocumentFragment from the \a{cursor}'s selection.
    If the cursor doesn't have a selection, the created fragment is empty.

    \sa isEmpty() QTextCursor::selection()
*/
QTextDocumentFragment::QTextDocumentFragment(const QTextCursor &cursor)
    : d(0)
{
    if (!cursor.hasSelection())
        return;

    d = new QTextDocumentFragmentPrivate(cursor);
}

/*!
    \fn QTextDocumentFragment::QTextDocumentFragment(const QTextDocumentFragment &other)

    Copy constructor. Creates a copy of the \a other fragment.
*/
QTextDocumentFragment::QTextDocumentFragment(const QTextDocumentFragment &rhs)
    : d(rhs.d)
{
    if (d)
        d->ref.ref();
}

/*!
    \fn QTextDocumentFragment &QTextDocumentFragment::operator=(const QTextDocumentFragment &other)

    Assigns the \a other fragment to this fragment.
*/
QTextDocumentFragment &QTextDocumentFragment::operator=(const QTextDocumentFragment &rhs)
{
    if (rhs.d)
        rhs.d->ref.ref();
    if (d && !d->ref.deref())
        delete d;
    d = rhs.d;
    return *this;
}

/*!
    Destroys the document fragment.
*/
QTextDocumentFragment::~QTextDocumentFragment()
{
    if (d && !d->ref.deref())
        delete d;
}

/*!
    Returns true if the fragment is empty; otherwise returns false.
*/
bool QTextDocumentFragment::isEmpty() const
{
    return !d || !d->doc || d->doc->docHandle()->length() <= 1;
}

/*!
    Returns the document fragment's text as plain text (i.e. with no
    formatting information).

    \sa toHtml()
*/
QString QTextDocumentFragment::toPlainText() const
{
    if (!d)
        return QString();

    return d->doc->toPlainText();
}

// #### Qt 5: merge with other overload
/*!
    \overload
*/

#ifndef QT_NO_TEXTHTMLPARSER

QString QTextDocumentFragment::toHtml() const
{
    return toHtml(QByteArray());
}

/*!
    \since 4.2

    Returns the contents of the document fragment as HTML,
    using the specified \a encoding (e.g., "UTF-8", "ISO 8859-1").

    \sa toPlainText(), QTextDocument::toHtml(), QTextCodec
*/
QString QTextDocumentFragment::toHtml(const QByteArray &encoding) const
{
    if (!d)
        return QString();

    return QTextHtmlExporter(d->doc).toHtml(encoding, QTextHtmlExporter::ExportFragment);
}

#endif // QT_NO_TEXTHTMLPARSER

/*!
    Returns a document fragment that contains the given \a plainText.

    When inserting such a fragment into a QTextDocument the current char format of
    the QTextCursor used for insertion is used as format for the text.
*/
QTextDocumentFragment QTextDocumentFragment::fromPlainText(const QString &plainText)
{
    QTextDocumentFragment res;

    res.d = new QTextDocumentFragmentPrivate;
    res.d->importedFromPlainText = true;
    QTextCursor cursor(res.d->doc);
    cursor.insertText(plainText);
    return res;
}

static QTextListFormat::Style nextListStyle(QTextListFormat::Style style)
{
    if (style == QTextListFormat::ListDisc)
        return QTextListFormat::ListCircle;
    else if (style == QTextListFormat::ListCircle)
        return QTextListFormat::ListSquare;
    return style;
}

#ifndef QT_NO_TEXTHTMLPARSER

QTextHtmlImporter::QTextHtmlImporter(QTextDocument *_doc, const QString &_html, ImportMode mode, const QTextDocument *resourceProvider)
    : indent(0), compressNextWhitespace(PreserveWhiteSpace), doc(_doc), importMode(mode)
{
    cursor = QTextCursor(doc);
    wsm = QTextHtmlParserNode::WhiteSpaceNormal;

    QString html = _html;
    const int startFragmentPos = html.indexOf(QLatin1String("<!--StartFragment-->"));
    if (startFragmentPos != -1) {
        QString qt3RichTextHeader(QLatin1String("<meta name=\"qrichtext\" content=\"1\" />"));

        // Hack for Qt3
        const bool hasQtRichtextMetaTag = html.contains(qt3RichTextHeader);

        const int endFragmentPos = html.indexOf(QLatin1String("<!--EndFragment-->"));
        if (startFragmentPos < endFragmentPos)
            html = html.mid(startFragmentPos, endFragmentPos - startFragmentPos);
        else
            html = html.mid(startFragmentPos);

        if (hasQtRichtextMetaTag)
            html.prepend(qt3RichTextHeader);
    }

    parse(html, resourceProvider ? resourceProvider : doc);
//    dumpHtml();
}

void QTextHtmlImporter::import()
{
    cursor.beginEditBlock();
    hasBlock = true;
    forceBlockMerging = false;
    compressNextWhitespace = RemoveWhiteSpace;
    blockTagClosed = false;
    for (currentNodeIdx = 0; currentNodeIdx < count(); ++currentNodeIdx) {
        currentNode = &at(currentNodeIdx);
        wsm = textEditMode ? QTextHtmlParserNode::WhiteSpacePreWrap : currentNode->wsm;

        /*
         * process each node in three stages:
         * 1) check if the hierarchy changed and we therefore passed the
         *    equivalent of a closing tag -> we may need to finish off
         *    some structures like tables
         *
         * 2) check if the current node is a special node like a
         *    <table>, <ul> or <img> tag that requires special processing
         *
         * 3) if the node should result in a QTextBlock create one and
         *    finally insert text that may be attached to the node
         */

        /* emit 'closing' table blocks or adjust current indent level
         * if we
         *  1) are beyond the first node
         *  2) the current node not being a child of the previous node
         *      means there was a tag closing in the input html
         */
        if (currentNodeIdx > 0 && (currentNode->parent != currentNodeIdx - 1)) {
            blockTagClosed = closeTag();
            // visually collapse subsequent block tags, but if the element after the closed block tag
            // is for example an inline element (!isBlock) we have to make sure we start a new paragraph by setting
            // hasBlock to false.
            if (blockTagClosed
                && !currentNode->isBlock()
                && currentNode->id != Html_unknown)
            {
                hasBlock = false;
            } else if (hasBlock) {
                // when collapsing subsequent block tags we need to clear the block format
                QTextBlockFormat blockFormat = currentNode->blockFormat;
                blockFormat.setIndent(indent);

                QTextBlockFormat oldFormat = cursor.blockFormat();
                if (oldFormat.hasProperty(QTextFormat::PageBreakPolicy)) {
                    QTextFormat::PageBreakFlags pageBreak = oldFormat.pageBreakPolicy();
                    if (pageBreak == QTextFormat::PageBreak_AlwaysAfter)
                        /* We remove an empty paragrah that requested a page break after.
                           moving that request to the next paragraph means we also need to make
                            that a pagebreak before to keep the same visual appearance.
                        */
                        pageBreak = QTextFormat::PageBreak_AlwaysBefore;
                    blockFormat.setPageBreakPolicy(pageBreak);
                }

                cursor.setBlockFormat(blockFormat);
            }
        }

        if (currentNode->displayMode == QTextHtmlElement::DisplayNone) {
            if (currentNode->id == Html_title)
                doc->setMetaInformation(QTextDocument::DocumentTitle, currentNode->text);
            // ignore explicitly 'invisible' elements
            continue;
        }

        if (processSpecialNodes() == ContinueWithNextNode)
            continue;

        // make sure there's a block for 'Blah' after <ul><li>foo</ul>Blah
        if (blockTagClosed
            && !hasBlock
            && !currentNode->isBlock()
            && !currentNode->text.isEmpty() && !currentNode->hasOnlyWhitespace()
            && currentNode->displayMode == QTextHtmlElement::DisplayInline) {

            QTextBlockFormat block = currentNode->blockFormat;
            block.setIndent(indent);

            appendBlock(block, currentNode->charFormat);

            hasBlock = true;
        }

        if (currentNode->isBlock()) {
            QTextHtmlImporter::ProcessNodeResult result = processBlockNode();
            if (result == ContinueWithNextNode) {
                continue;
            } else if (result == ContinueWithNextSibling) {
                currentNodeIdx += currentNode->children.size();
                continue;
            }
        }

        if (currentNode->charFormat.isAnchor() && !currentNode->charFormat.anchorName().isEmpty()) {
            namedAnchors.append(currentNode->charFormat.anchorName());
        }

        if (appendNodeText())
            hasBlock = false; // if we actually appended text then we don't
                              // have an empty block anymore
    }

    cursor.endEditBlock();
}

bool QTextHtmlImporter::appendNodeText()
{
    const int initialCursorPosition = cursor.position();
    QTextCharFormat format = currentNode->charFormat;

    if(wsm == QTextHtmlParserNode::WhiteSpacePre || wsm == QTextHtmlParserNode::WhiteSpacePreWrap)
        compressNextWhitespace = PreserveWhiteSpace;

    QString text = currentNode->text;

    QString textToInsert;
    textToInsert.reserve(text.size());

    for (int i = 0; i < text.length(); ++i) {
        QChar ch = text.at(i);

        if (ch.isSpace()
            && ch != QChar::Nbsp
            && ch != QChar::ParagraphSeparator) {

            if (compressNextWhitespace == CollapseWhiteSpace)
                compressNextWhitespace = RemoveWhiteSpace; // allow this one, and remove the ones coming next.
            else if(compressNextWhitespace == RemoveWhiteSpace)
                continue;

            if (wsm == QTextHtmlParserNode::WhiteSpacePre
                || textEditMode
               ) {
                if (ch == QLatin1Char('\n')) {
                    if (textEditMode)
                        continue;
                } else if (ch == QLatin1Char('\r')) {
                    continue;
                }
            } else if (wsm != QTextHtmlParserNode::WhiteSpacePreWrap) {
                compressNextWhitespace = RemoveWhiteSpace;
                if (wsm == QTextHtmlParserNode::WhiteSpaceNoWrap)
                    ch = QChar::Nbsp;
                else
                    ch = QLatin1Char(' ');
            }
        } else {
            compressNextWhitespace = PreserveWhiteSpace;
        }

        if (ch == QLatin1Char('\n')
            || ch == QChar::ParagraphSeparator) {

            if (!textToInsert.isEmpty()) {
                cursor.insertText(textToInsert, format);
                textToInsert.clear();
            }

            QTextBlockFormat fmt = cursor.blockFormat();

            if (fmt.hasProperty(QTextFormat::BlockBottomMargin)) {
                QTextBlockFormat tmp = fmt;
                tmp.clearProperty(QTextFormat::BlockBottomMargin);
                cursor.setBlockFormat(tmp);
            }

            fmt.clearProperty(QTextFormat::BlockTopMargin);
            appendBlock(fmt, cursor.charFormat());
        } else {
            if (!namedAnchors.isEmpty()) {
                if (!textToInsert.isEmpty()) {
                    cursor.insertText(textToInsert, format);
                    textToInsert.clear();
                }

                format.setAnchor(true);
                format.setAnchorNames(namedAnchors);
                cursor.insertText(ch, format);
                namedAnchors.clear();
                format.clearProperty(QTextFormat::IsAnchor);
                format.clearProperty(QTextFormat::AnchorName);
            } else {
                textToInsert += ch;
            }
        }
    }

    if (!textToInsert.isEmpty()) {
        cursor.insertText(textToInsert, format);
    }

    return cursor.position() != initialCursorPosition;
}

QTextHtmlImporter::ProcessNodeResult QTextHtmlImporter::processSpecialNodes()
{
    switch (currentNode->id) {
        case Html_body:
            if (currentNode->charFormat.background().style() != Qt::NoBrush) {
                QTextFrameFormat fmt = doc->rootFrame()->frameFormat();
                fmt.setBackground(currentNode->charFormat.background());
                doc->rootFrame()->setFrameFormat(fmt);
                const_cast<QTextHtmlParserNode *>(currentNode)->charFormat.clearProperty(QTextFormat::BackgroundBrush);
            }
            compressNextWhitespace = RemoveWhiteSpace;
            break;

        case Html_ol:
        case Html_ul: {
            QTextListFormat::Style style = currentNode->listStyle;

            if (currentNode->id == Html_ul && !currentNode->hasOwnListStyle && currentNode->parent) {
                const QTextHtmlParserNode *n = &at(currentNode->parent);
                while (n) {
                    if (n->id == Html_ul) {
                        style = nextListStyle(currentNode->listStyle);
                    }
                    if (n->parent)
                        n = &at(n->parent);
                    else
                        n = 0;
                }
            }

            QTextListFormat listFmt;
            listFmt.setStyle(style);
            if (!currentNode->textListNumberPrefix.isNull())
                listFmt.setNumberPrefix(currentNode->textListNumberPrefix);
            if (!currentNode->textListNumberSuffix.isNull())
                listFmt.setNumberSuffix(currentNode->textListNumberSuffix);

            ++indent;
            if (currentNode->hasCssListIndent)
                listFmt.setIndent(currentNode->cssListIndent);
            else
                listFmt.setIndent(indent);

            List l;
            l.format = listFmt;
            l.listNode = currentNodeIdx;
            lists.append(l);
            compressNextWhitespace = RemoveWhiteSpace;

            // broken html: <ul>Text here<li>Foo
            const QString simpl = currentNode->text.simplified();
            if (simpl.isEmpty() || simpl.at(0).isSpace())
                return ContinueWithNextNode;
            break;
        }

        case Html_table: {
            Table t = scanTable(currentNodeIdx);
            tables.append(t);
            hasBlock = false;
            compressNextWhitespace = RemoveWhiteSpace;
            return ContinueWithNextNode;
        }

        case Html_tr:
            return ContinueWithNextNode;

        case Html_img: {
            QTextImageFormat fmt;
            fmt.setName(currentNode->imageName);

            fmt.merge(currentNode->charFormat);

            if (currentNode->imageWidth != -1)
                fmt.setWidth(currentNode->imageWidth);
            if (currentNode->imageHeight != -1)
                fmt.setHeight(currentNode->imageHeight);

            cursor.insertImage(fmt, QTextFrameFormat::Position(currentNode->cssFloat));

            cursor.movePosition(QTextCursor::Left, QTextCursor::KeepAnchor);
            cursor.mergeCharFormat(currentNode->charFormat);
            cursor.movePosition(QTextCursor::Right);
            compressNextWhitespace = CollapseWhiteSpace;

            hasBlock = false;
            return ContinueWithNextNode;
        }

        case Html_hr: {
            QTextBlockFormat blockFormat = currentNode->blockFormat;
            blockFormat.setTopMargin(topMargin(currentNodeIdx));
            blockFormat.setBottomMargin(bottomMargin(currentNodeIdx));
            blockFormat.setProperty(QTextFormat::BlockTrailingHorizontalRulerWidth, currentNode->width);
            if (hasBlock && importMode == ImportToDocument)
                cursor.mergeBlockFormat(blockFormat);
            else
                appendBlock(blockFormat);
            hasBlock = false;
            compressNextWhitespace = RemoveWhiteSpace;
            return ContinueWithNextNode;
        }

        default: break;
    }
    return ContinueWithCurrentNode;
}

// returns true if a block tag was closed
bool QTextHtmlImporter::closeTag()
{
    const QTextHtmlParserNode *closedNode = &at(currentNodeIdx - 1);
    const int endDepth = depth(currentNodeIdx) - 1;
    int depth = this->depth(currentNodeIdx - 1);
    bool blockTagClosed = false;

    while (depth > endDepth) {
        Table *t = 0;
        if (!tables.isEmpty())
            t = &tables.last();

        switch (closedNode->id) {
            case Html_tr:
                if (t && !t->isTextFrame) {
                    ++t->currentRow;

                    // for broken html with rowspans but missing tr tags
                    while (!t->currentCell.atEnd() && t->currentCell.row < t->currentRow)
                        ++t->currentCell;
                }

                blockTagClosed = true;
                break;

            case Html_table:
                if (!t)
                    break;
                indent = t->lastIndent;

                tables.resize(tables.size() - 1);
                t = 0;

                if (tables.isEmpty()) {
                    cursor = doc->rootFrame()->lastCursorPosition();
                } else {
                    t = &tables.last();
                    if (t->isTextFrame)
                        cursor = t->frame->lastCursorPosition();
                    else if (!t->currentCell.atEnd())
                        cursor = t->currentCell.cell().lastCursorPosition();
                }

                // we don't need an extra block after tables, so we don't
                // claim to have closed one for the creation of a new one
                // in import()
                blockTagClosed = false;
                compressNextWhitespace = RemoveWhiteSpace;
                break;

            case Html_th:
            case Html_td:
                if (t && !t->isTextFrame)
                    ++t->currentCell;
                blockTagClosed = true;
                compressNextWhitespace = RemoveWhiteSpace;
                break;

            case Html_ol:
            case Html_ul:
                if (lists.isEmpty())
                    break;
                lists.resize(lists.size() - 1);
                --indent;
                blockTagClosed = true;
                break;

            case Html_br:
                compressNextWhitespace = RemoveWhiteSpace;
                break;

            case Html_div:
                if (closedNode->children.isEmpty())
                    break;
                // fall through
            default:
                if (closedNode->isBlock())
                    blockTagClosed = true;
                break;
        }

        closedNode = &at(closedNode->parent);
        --depth;
    }

    return blockTagClosed;
}

QTextHtmlImporter::Table QTextHtmlImporter::scanTable(int tableNodeIdx)
{
    Table table;
    table.columns = 0;

    QVector<QTextLength> columnWidths;

    int tableHeaderRowCount = 0;
    QVector<int> rowNodes;
    rowNodes.reserve(at(tableNodeIdx).children.count());
    foreach (int row, at(tableNodeIdx).children)
        switch (at(row).id) {
            case Html_tr:
                rowNodes += row;
                break;
            case Html_thead:
            case Html_tbody:
            case Html_tfoot:
                foreach (int potentialRow, at(row).children)
                    if (at(potentialRow).id == Html_tr) {
                        rowNodes += potentialRow;
                        if (at(row).id == Html_thead)
                            ++tableHeaderRowCount;
                    }
                break;
            default: break;
        }

    QVector<RowColSpanInfo> rowColSpans;
    QVector<RowColSpanInfo> rowColSpanForColumn;

    int effectiveRow = 0;
    foreach (int row, rowNodes) {
        int colsInRow = 0;

        foreach (int cell, at(row).children)
            if (at(cell).isTableCell()) {
                // skip all columns with spans from previous rows
                while (colsInRow < rowColSpanForColumn.size()) {
                    const RowColSpanInfo &spanInfo = rowColSpanForColumn[colsInRow];

                    if (spanInfo.row + spanInfo.rowSpan > effectiveRow) {
                        Q_ASSERT(spanInfo.col == colsInRow);
                        colsInRow += spanInfo.colSpan;
                    } else
                        break;
                }

                const QTextHtmlParserNode &c = at(cell);
                const int currentColumn = colsInRow;
                colsInRow += c.tableCellColSpan;

                RowColSpanInfo spanInfo;
                spanInfo.row = effectiveRow;
                spanInfo.col = currentColumn;
                spanInfo.colSpan = c.tableCellColSpan;
                spanInfo.rowSpan = c.tableCellRowSpan;
                if (spanInfo.colSpan > 1 || spanInfo.rowSpan > 1)
                    rowColSpans.append(spanInfo);

                columnWidths.resize(qMax(columnWidths.count(), colsInRow));
                rowColSpanForColumn.resize(columnWidths.size());
                for (int i = currentColumn; i < currentColumn + c.tableCellColSpan; ++i) {
                    if (columnWidths.at(i).type() == QTextLength::VariableLength) {
                        QTextLength w = c.width;
                        if (c.tableCellColSpan > 1 && w.type() != QTextLength::VariableLength)
                            w = QTextLength(w.type(), w.value(100.) / c.tableCellColSpan);
                        columnWidths[i] = w;
                    }
                    rowColSpanForColumn[i] = spanInfo;
                }
            }

        table.columns = qMax(table.columns, colsInRow);

        ++effectiveRow;
    }
    table.rows = effectiveRow;

    table.lastIndent = indent;
    indent = 0;

    if (table.rows == 0 || table.columns == 0)
        return table;

    QTextFrameFormat fmt;
    const QTextHtmlParserNode &node = at(tableNodeIdx);

    if (!node.isTextFrame) {
        QTextTableFormat tableFmt;
        tableFmt.setCellSpacing(node.tableCellSpacing);
        tableFmt.setCellPadding(node.tableCellPadding);
        if (node.blockFormat.hasProperty(QTextFormat::BlockAlignment))
            tableFmt.setAlignment(node.blockFormat.alignment());
        tableFmt.setColumns(table.columns);
        tableFmt.setColumnWidthConstraints(columnWidths);
        tableFmt.setHeaderRowCount(tableHeaderRowCount);
        fmt = tableFmt;
    }

    fmt.setTopMargin(topMargin(tableNodeIdx));
    fmt.setBottomMargin(bottomMargin(tableNodeIdx));
    fmt.setLeftMargin(leftMargin(tableNodeIdx)
                      + table.lastIndent * 40 // ##### not a good emulation
                      );
    fmt.setRightMargin(rightMargin(tableNodeIdx));

    // compatibility
    if (qFuzzyCompare(fmt.leftMargin(), fmt.rightMargin())
        && qFuzzyCompare(fmt.leftMargin(), fmt.topMargin())
        && qFuzzyCompare(fmt.leftMargin(), fmt.bottomMargin()))
        fmt.setProperty(QTextFormat::FrameMargin, fmt.leftMargin());

    fmt.setBorderStyle(node.borderStyle);
    fmt.setBorderBrush(node.borderBrush);
    fmt.setBorder(node.tableBorder);
    fmt.setWidth(node.width);
    fmt.setHeight(node.height);
    if (node.blockFormat.hasProperty(QTextFormat::PageBreakPolicy))
        fmt.setPageBreakPolicy(node.blockFormat.pageBreakPolicy());

    if (node.blockFormat.hasProperty(QTextFormat::LayoutDirection))
        fmt.setLayoutDirection(node.blockFormat.layoutDirection());
    if (node.charFormat.background().style() != Qt::NoBrush)
        fmt.setBackground(node.charFormat.background());
    fmt.setPosition(QTextFrameFormat::Position(node.cssFloat));

    if (node.isTextFrame) {
        if (node.isRootFrame) {
            table.frame = cursor.currentFrame();
            table.frame->setFrameFormat(fmt);
        } else
            table.frame = cursor.insertFrame(fmt);

        table.isTextFrame = true;
    } else {
        const int oldPos = cursor.position();
        QTextTable *textTable = cursor.insertTable(table.rows, table.columns, fmt.toTableFormat());
        table.frame = textTable;

        for (int i = 0; i < rowColSpans.count(); ++i) {
            const RowColSpanInfo &nfo = rowColSpans.at(i);
            textTable->mergeCells(nfo.row, nfo.col, nfo.rowSpan, nfo.colSpan);
        }

        table.currentCell = TableCellIterator(textTable);
        cursor.setPosition(oldPos); // restore for caption support which needs to be inserted right before the table
    }
    return table;
}

QTextHtmlImporter::ProcessNodeResult QTextHtmlImporter::processBlockNode()
{
    QTextBlockFormat block;
    QTextCharFormat charFmt;
    bool modifiedBlockFormat = true;
    bool modifiedCharFormat = true;

    if (currentNode->isTableCell() && !tables.isEmpty()) {
        Table &t = tables.last();
        if (!t.isTextFrame && !t.currentCell.atEnd()) {
            QTextTableCell cell = t.currentCell.cell();
            if (cell.isValid()) {
                QTextTableCellFormat fmt = cell.format().toTableCellFormat();
                if (topPadding(currentNodeIdx) >= 0)
                    fmt.setTopPadding(topPadding(currentNodeIdx));
                if (bottomPadding(currentNodeIdx) >= 0)
                    fmt.setBottomPadding(bottomPadding(currentNodeIdx));
                if (leftPadding(currentNodeIdx) >= 0)
                    fmt.setLeftPadding(leftPadding(currentNodeIdx));
                if (rightPadding(currentNodeIdx) >= 0)
                    fmt.setRightPadding(rightPadding(currentNodeIdx));
                cell.setFormat(fmt);

                cursor.setPosition(cell.firstPosition());
            }
        }
        hasBlock = true;
        compressNextWhitespace = RemoveWhiteSpace;

        if (currentNode->charFormat.background().style() != Qt::NoBrush) {
            charFmt.setBackground(currentNode->charFormat.background());
            cursor.mergeBlockCharFormat(charFmt);
        }
    }

    if (hasBlock) {
        block = cursor.blockFormat();
        charFmt = cursor.blockCharFormat();
        modifiedBlockFormat = false;
        modifiedCharFormat = false;
    }

    // collapse
    {
        qreal tm = qreal(topMargin(currentNodeIdx));
        if (tm > block.topMargin()) {
            block.setTopMargin(tm);
            modifiedBlockFormat = true;
        }
    }

    int bottomMargin = this->bottomMargin(currentNodeIdx);

    // for list items we may want to collapse with the bottom margin of the
    // list.
    const QTextHtmlParserNode *parentNode = currentNode->parent ? &at(currentNode->parent) : 0;
    if ((currentNode->id == Html_li || currentNode->id == Html_dt || currentNode->id == Html_dd)
        && parentNode
        && (parentNode->isListStart() || parentNode->id == Html_dl)
        && (parentNode->children.last() == currentNodeIdx)) {
        bottomMargin = qMax(bottomMargin, this->bottomMargin(currentNode->parent));
    }

    if (block.bottomMargin() != bottomMargin) {
        block.setBottomMargin(bottomMargin);
        modifiedBlockFormat = true;
    }

    {
        const qreal lm = leftMargin(currentNodeIdx);
        const qreal rm = rightMargin(currentNodeIdx);

        if (block.leftMargin() != lm) {
            block.setLeftMargin(lm);
            modifiedBlockFormat = true;
        }
        if (block.rightMargin() != rm) {
            block.setRightMargin(rm);
            modifiedBlockFormat = true;
        }
    }

    if (currentNode->id != Html_li
        && indent != 0
        && (lists.isEmpty()
            || !hasBlock
            || !lists.last().list
            || lists.last().list->itemNumber(cursor.block()) == -1
           )
       ) {
        block.setIndent(indent);
        modifiedBlockFormat = true;
    }

    if (currentNode->blockFormat.propertyCount() > 0) {
        modifiedBlockFormat = true;
        block.merge(currentNode->blockFormat);
    }

    if (currentNode->charFormat.propertyCount() > 0) {
        modifiedCharFormat = true;
        charFmt.merge(currentNode->charFormat);
    }

    // ####################
    //                block.setFloatPosition(node->cssFloat);

    if (wsm == QTextHtmlParserNode::WhiteSpacePre) {
        block.setNonBreakableLines(true);
        modifiedBlockFormat = true;
    }

    if (currentNode->charFormat.background().style() != Qt::NoBrush && !currentNode->isTableCell()) {
        block.setBackground(currentNode->charFormat.background());
        modifiedBlockFormat = true;
    }

    if (hasBlock && (!currentNode->isEmptyParagraph || forceBlockMerging)) {
        if (modifiedBlockFormat)
            cursor.setBlockFormat(block);
        if (modifiedCharFormat)
            cursor.setBlockCharFormat(charFmt);
    } else {
        if (currentNodeIdx == 1 && cursor.position() == 0 && currentNode->isEmptyParagraph) {
            cursor.setBlockFormat(block);
            cursor.setBlockCharFormat(charFmt);
        } else {
            appendBlock(block, charFmt);
        }
    }

    if (currentNode->userState != -1)
        cursor.block().setUserState(currentNode->userState);

    if (currentNode->id == Html_li && !lists.isEmpty()) {
        List &l = lists.last();
        if (l.list) {
            l.list->add(cursor.block());
        } else {
            l.list = cursor.createList(l.format);
            const qreal listTopMargin = topMargin(l.listNode);
            if (listTopMargin > block.topMargin()) {
                block.setTopMargin(listTopMargin);
                cursor.mergeBlockFormat(block);
            }
        }
        if (hasBlock) {
            QTextBlockFormat fmt;
            fmt.setIndent(0);
            cursor.mergeBlockFormat(fmt);
        }
    }

    forceBlockMerging = false;
    if (currentNode->id == Html_body || currentNode->id == Html_html)
        forceBlockMerging = true;

    if (currentNode->isEmptyParagraph) {
        hasBlock = false;
        return ContinueWithNextSibling;
    }

    hasBlock = true;
    blockTagClosed = false;
    return ContinueWithCurrentNode;
}

void QTextHtmlImporter::appendBlock(const QTextBlockFormat &format, QTextCharFormat charFmt)
{
    if (!namedAnchors.isEmpty()) {
        charFmt.setAnchor(true);
        charFmt.setAnchorNames(namedAnchors);
        namedAnchors.clear();
    }

    cursor.insertBlock(format, charFmt);

    if (wsm != QTextHtmlParserNode::WhiteSpacePre && wsm != QTextHtmlParserNode::WhiteSpacePreWrap)
        compressNextWhitespace = RemoveWhiteSpace;
}

#endif // QT_NO_TEXTHTMLPARSER

/*!
    \fn QTextDocumentFragment QTextDocumentFragment::fromHtml(const QString &text)

    Returns a QTextDocumentFragment based on the arbitrary piece of
    HTML in the given \a text. The formatting is preserved as much as
    possible; for example, "<b>bold</b>" will become a document
    fragment with the text "bold" with a bold character format.
*/

#ifndef QT_NO_TEXTHTMLPARSER

QTextDocumentFragment QTextDocumentFragment::fromHtml(const QString &html)
{
    return fromHtml(html, 0);
}

/*!
    \fn QTextDocumentFragment QTextDocumentFragment::fromHtml(const QString &text, const QTextDocument *resourceProvider)
    \since 4.2

    Returns a QTextDocumentFragment based on the arbitrary piece of
    HTML in the given \a text. The formatting is preserved as much as
    possible; for example, "<b>bold</b>" will become a document
    fragment with the text "bold" with a bold character format.

    If the provided HTML contains references to external resources such as imported style sheets, then
    they will be loaded through the \a resourceProvider.
*/

QTextDocumentFragment QTextDocumentFragment::fromHtml(const QString &html, const QTextDocument *resourceProvider)
{
    QTextDocumentFragment res;
    res.d = new QTextDocumentFragmentPrivate;

    QTextHtmlImporter importer(res.d->doc, html, QTextHtmlImporter::ImportToFragment, resourceProvider);
    importer.import();
    return res;
}

QT_END_NAMESPACE
#endif // QT_NO_TEXTHTMLPARSER
