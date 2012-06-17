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

#include <private/qtools_p.h>
#include <qdebug.h>

#include "qtextdocument_p.h"
#include "qtextdocument.h"
#include <qtextformat.h>
#include "qtextformat_p.h"
#include "qtextobject_p.h"
#include "qtextcursor.h"
#include "qtextimagehandler_p.h"
#include "qtextcursor_p.h"
#include "qtextdocumentlayout_p.h"
#include "qtexttable.h"
#include "qtextengine_p.h"

#include <stdlib.h>

QT_BEGIN_NAMESPACE

#define PMDEBUG if(0) qDebug

// The VxWorks DIAB compiler crashes when initializing the anonymouse union with { a7 }
#if !defined(Q_CC_DIAB)
#  define QT_INIT_TEXTUNDOCOMMAND(c, a1, a2, a3, a4, a5, a6, a7, a8) \
          QTextUndoCommand c = { a1, a2, 0, 0, quint8(a3), a4, a5, a6, { a7 }, a8 }
#else
#  define QT_INIT_TEXTUNDOCOMMAND(c, a1, a2, a3, a4, a5, a6, a7, a8) \
          QTextUndoCommand c = { a1, a2, 0, 0, a3, a4, a5, a6 }; c.blockFormat = a7; c.revision = a8
#endif

/*
  Structure of a document:

  DOCUMENT :== FRAME_CONTENTS
  FRAME :== START_OF_FRAME  FRAME_CONTENTS END_OF_FRAME
  FRAME_CONTENTS = LIST_OF_BLOCKS ((FRAME | TABLE) LIST_OF_BLOCKS)*
  TABLE :== (START_OF_FRAME TABLE_CELL)+ END_OF_FRAME
  TABLE_CELL = FRAME_CONTENTS
  LIST_OF_BLOCKS :== (BLOCK END_OF_PARA)* BLOCK
  BLOCK :== (FRAGMENT)*
  FRAGMENT :== String of characters

  END_OF_PARA :== 0x2029 # Paragraph separator in Unicode
  START_OF_FRAME :== 0xfdd0
  END_OF_FRAME := 0xfdd1

  Note also that LIST_OF_BLOCKS can be empty. Nevertheless, there is
  at least one valid cursor position there where you could start
  typing. The block format is in this case determined by the last
  END_OF_PARA/START_OF_FRAME/END_OF_FRAME (see below).

  Lists are not in here, as they are treated specially. A list is just
  a collection of (not necessarily connected) blocks, that share the
  same objectIndex() in the format that refers to the list format and
  object.

  The above does not clearly note where formats are. Here's
  how it looks currently:

  FRAGMENT: one charFormat associated

  END_OF_PARA: one charFormat, and a blockFormat for the _next_ block.

  START_OF_FRAME: one char format, and a blockFormat (for the next
  block). The format associated with the objectIndex() of the
  charFormat decides whether this is a frame or table and its
  properties

  END_OF_FRAME: one charFormat and a blockFormat (for the next
  block). The object() of the charFormat is the same as for the
  corresponding START_OF_BLOCK.


  The document is independent of the layout with certain restrictions:

  * Cursor movement (esp. up and down) depend on the layout.
  * You cannot have more than one layout, as the layout data of QTextObjects
    is stored in the text object itself.

*/

void QTextBlockData::invalidate() const
{
    if (layout)
        layout->engine()->invalidate();
}

static bool isValidBlockSeparator(const QChar &ch)
{
    return ch == QChar::ParagraphSeparator
        || ch == QTextBeginningOfFrame
        || ch == QTextEndOfFrame;
}

#ifndef QT_NO_DEBUG
static bool noBlockInString(const QString &str)
{
    return !str.contains(QChar::ParagraphSeparator)
        && !str.contains(QTextBeginningOfFrame)
        && !str.contains(QTextEndOfFrame);
}
#endif

bool QTextUndoCommand::tryMerge(const QTextUndoCommand &other)
{
    if (command != other.command)
        return false;

    if (command == Inserted
        && (pos + length == other.pos)
        && (strPos + length == other.strPos)
        && format == other.format) {

        length += other.length;
        return true;
    }

    // removal to the 'right' using 'Delete' key
    if (command == Removed
        && pos == other.pos
        && (strPos + length == other.strPos)
        && format == other.format) {

        length += other.length;
        return true;
    }

    // removal to the 'left' using 'Backspace'
    if (command == Removed
        && (other.pos + other.length == pos)
        && (other.strPos + other.length == strPos)
        && (format == other.format)) {

        int l = length;
        (*this) = other;

        length += l;
        return true;
    }

    return false;
}

QTextDocumentPrivate::QTextDocumentPrivate()
    : wasUndoAvailable(false),
    wasRedoAvailable(false),
    docChangeOldLength(0),
    docChangeLength(0),
    framesDirty(true),
    rtFrame(0),
    initialBlockCharFormatIndex(-1) // set correctly later in init()
{
    editBlock = 0;
    editBlockCursorPosition = -1;
    docChangeFrom = -1;

    undoState = 0;
    revision = -1; // init() inserts a block, bringing it to 0

    lout = 0;

    modified = false;
    modifiedState = 0;

    undoEnabled = true;
    inContentsChange = false;
    blockCursorAdjustment = false;

    defaultTextOption.setTabStop(80); // same as in qtextengine.cpp
    defaultTextOption.setWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
    defaultCursorMoveStyle = Qt::LogicalMoveStyle;

    indentWidth = 40;
    documentMargin = 4;

    maximumBlockCount = 0;
    needsEnsureMaximumBlockCount = false;
    unreachableCharacterCount = 0;
    lastBlockCount = 0;
}

void QTextDocumentPrivate::init()
{
    framesDirty = false;

    bool undoState = undoEnabled;
    undoEnabled = false;
    initialBlockCharFormatIndex = formats.indexForFormat(QTextCharFormat());
    insertBlock(0, formats.indexForFormat(QTextBlockFormat()), formats.indexForFormat(QTextCharFormat()));
    undoEnabled = undoState;
    modified = false;
    modifiedState = 0;
}

void QTextDocumentPrivate::clear()
{
    Q_Q(QTextDocument);

    foreach (QTextCursorPrivate *curs, cursors) {
        curs->setPosition(0);
        curs->currentCharFormat = -1;
        curs->anchor = 0;
        curs->adjusted_anchor = 0;
    }

    QList<QTextCursorPrivate *>oldCursors = cursors;
    QT_TRY{
        cursors.clear();

        QMap<int, QTextObject *>::Iterator objectIt = objects.begin();
        while (objectIt != objects.end()) {
            if (*objectIt != rtFrame) {
                delete *objectIt;
                objectIt = objects.erase(objectIt);
            } else {
                ++objectIt;
            }
        }
        // also clear out the remaining root frame pointer
        // (we're going to delete the object further down)
        objects.clear();

        title.clear();
        clearUndoRedoStacks(QTextDocument::UndoAndRedoStacks);
        text = QString();
        unreachableCharacterCount = 0;
        modifiedState = 0;
        modified = false;
        formats = QTextFormatCollection();
        int len = fragments.length();
        fragments.clear();
        blocks.clear();
        cachedResources.clear();
        delete rtFrame;
        rtFrame = 0;
        init();
        cursors = oldCursors;
        inContentsChange = true;
        q->contentsChange(0, len, 0);
        inContentsChange = false;
        if (lout)
            lout->documentChanged(0, len, 0);
    } QT_CATCH(...) {
        cursors = oldCursors; // at least recover the cursors
        QT_RETHROW;
    }
}

QTextDocumentPrivate::~QTextDocumentPrivate()
{
    foreach (QTextCursorPrivate *curs, cursors)
        curs->priv = 0;
    cursors.clear();
    undoState = 0;
    undoEnabled = true;
    clearUndoRedoStacks(QTextDocument::RedoStack);
}

void QTextDocumentPrivate::setLayout(QAbstractTextDocumentLayout *layout)
{
    Q_Q(QTextDocument);
    if (lout == layout)
        return;
    const bool firstLayout = !lout;
    delete lout;
    lout = layout;

    if (!firstLayout)
        for (BlockMap::Iterator it = blocks.begin(); !it.atEnd(); ++it)
            it->free();

    emit q->documentLayoutChanged();
    inContentsChange = true;
    emit q->contentsChange(0, 0, length());
    inContentsChange = false;
    if (lout)
        lout->documentChanged(0, 0, length());
}


void QTextDocumentPrivate::insert_string(int pos, uint strPos, uint length, int format, QTextUndoCommand::Operation op)
{
    // ##### optimize when only appending to the fragment!
    Q_ASSERT(noBlockInString(text.mid(strPos, length)));

    split(pos);
    uint x = fragments.insert_single(pos, length);
    QTextFragmentData *X = fragments.fragment(x);
    X->format = format;
    X->stringPosition = strPos;
    uint w = fragments.previous(x);
    if (w)
        unite(w);

    int b = blocks.findNode(pos);
    blocks.setSize(b, blocks.size(b)+length);

    Q_ASSERT(blocks.length() == fragments.length());

    QTextFrame *frame = qobject_cast<QTextFrame *>(objectForFormat(format));
    if (frame) {
        frame->d_func()->fragmentAdded(text.at(strPos), x);
        framesDirty = true;
    }

    adjustDocumentChangesAndCursors(pos, length, op);
}

int QTextDocumentPrivate::insert_block(int pos, uint strPos, int format, int blockFormat, QTextUndoCommand::Operation op, int command)
{
    split(pos);
    uint x = fragments.insert_single(pos, 1);
    QTextFragmentData *X = fragments.fragment(x);
    X->format = format;
    X->stringPosition = strPos;
    // no need trying to unite, since paragraph separators are always in a fragment of their own

    Q_ASSERT(isValidBlockSeparator(text.at(strPos)));
    Q_ASSERT(blocks.length()+1 == fragments.length());

    int block_pos = pos;
    if (blocks.length() && command == QTextUndoCommand::BlockRemoved)
        ++block_pos;
    int size = 1;
    int n = blocks.findNode(block_pos);
    int key = n ? blocks.position(n) : blocks.length();

    Q_ASSERT(n || (!n && block_pos == blocks.length()));
    if (key != block_pos) {
        Q_ASSERT(key < block_pos);
        int oldSize = blocks.size(n);
        blocks.setSize(n, block_pos-key);
        size += oldSize - (block_pos-key);
    }
    int b = blocks.insert_single(block_pos, size);
    QTextBlockData *B = blocks.fragment(b);
    B->format = blockFormat;

    Q_ASSERT(blocks.length() == fragments.length());

    QTextBlockGroup *group = qobject_cast<QTextBlockGroup *>(objectForFormat(blockFormat));
    if (group)
        group->blockInserted(QTextBlock(this, b));

    QTextFrame *frame = qobject_cast<QTextFrame *>(objectForFormat(formats.format(format)));
    if (frame) {
        frame->d_func()->fragmentAdded(text.at(strPos), x);
        framesDirty = true;
    }

    adjustDocumentChangesAndCursors(pos, 1, op);
    return x;
}

int QTextDocumentPrivate::insertBlock(const QChar &blockSeparator,
                                  int pos, int blockFormat, int charFormat, QTextUndoCommand::Operation op)
{
    Q_ASSERT(formats.format(blockFormat).isBlockFormat());
    Q_ASSERT(formats.format(charFormat).isCharFormat());
    Q_ASSERT(pos >= 0 && (pos < fragments.length() || (pos == 0 && fragments.length() == 0)));
    Q_ASSERT(isValidBlockSeparator(blockSeparator));

    beginEditBlock();

    int strPos = text.length();
    text.append(blockSeparator);

    int ob = blocks.findNode(pos);
    bool atBlockEnd = true;
    bool atBlockStart = true;
    int oldRevision = 0;
    if (ob) {
        atBlockEnd = (pos - blocks.position(ob) == blocks.size(ob)-1);
        atBlockStart = ((int)blocks.position(ob) == pos);
        oldRevision = blocks.fragment(ob)->revision;
    }

    const int fragment = insert_block(pos, strPos, charFormat, blockFormat, op, QTextUndoCommand::BlockRemoved);

    Q_ASSERT(blocks.length() == fragments.length());

    int b = blocks.findNode(pos);
    QTextBlockData *B = blocks.fragment(b);

    QT_INIT_TEXTUNDOCOMMAND(c, QTextUndoCommand::BlockInserted, (editBlock != 0),
                            op, charFormat, strPos, pos, blockFormat,
                            B->revision);

    appendUndoItem(c);
    Q_ASSERT(undoState == undoStack.size());

    // update revision numbers of the modified blocks.
    B->revision = (atBlockEnd && !atBlockStart)? oldRevision : revision;
    b = blocks.next(b);
    if (b) {
        B = blocks.fragment(b);
        B->revision = atBlockStart ? oldRevision : revision;
    }

    if (formats.charFormat(charFormat).objectIndex() == -1)
        needsEnsureMaximumBlockCount = true;

    endEditBlock();
    return fragment;
}

int QTextDocumentPrivate::insertBlock(int pos, int blockFormat, int charFormat, QTextUndoCommand::Operation op)
{
    return insertBlock(QChar::ParagraphSeparator, pos, blockFormat, charFormat, op);
}

void QTextDocumentPrivate::insert(int pos, int strPos, int strLength, int format)
{
    if (strLength <= 0)
        return;

    Q_ASSERT(pos >= 0 && pos < fragments.length());
    Q_ASSERT(formats.format(format).isCharFormat());

    insert_string(pos, strPos, strLength, format, QTextUndoCommand::MoveCursor);
    if (undoEnabled) {
        int b = blocks.findNode(pos);
        QTextBlockData *B = blocks.fragment(b);

        QT_INIT_TEXTUNDOCOMMAND(c, QTextUndoCommand::Inserted, (editBlock != 0),
                                QTextUndoCommand::MoveCursor, format, strPos, pos, strLength,
                                B->revision);
        appendUndoItem(c);
        B->revision = revision;
        Q_ASSERT(undoState == undoStack.size());
    }
    finishEdit();
}

void QTextDocumentPrivate::insert(int pos, const QString &str, int format)
{
    if (str.size() == 0)
        return;

    Q_ASSERT(noBlockInString(str));

    int strPos = text.length();
    text.append(str);
    insert(pos, strPos, str.length(), format);
}

int QTextDocumentPrivate::remove_string(int pos, uint length, QTextUndoCommand::Operation op)
{
    Q_ASSERT(pos >= 0);
    Q_ASSERT(blocks.length() == fragments.length());
    Q_ASSERT(blocks.length() >= pos+(int)length);

    int b = blocks.findNode(pos);
    uint x = fragments.findNode(pos);

    Q_ASSERT(blocks.size(b) > length);
    Q_ASSERT(x && fragments.position(x) == (uint)pos && fragments.size(x) == length);
    Q_ASSERT(noBlockInString(text.mid(fragments.fragment(x)->stringPosition, length)));

    blocks.setSize(b, blocks.size(b)-length);

    QTextFrame *frame = qobject_cast<QTextFrame *>(objectForFormat(fragments.fragment(x)->format));
    if (frame) {
        frame->d_func()->fragmentRemoved(text.at(fragments.fragment(x)->stringPosition), x);
        framesDirty = true;
    }

    const int w = fragments.erase_single(x);

    if (!undoEnabled)
        unreachableCharacterCount += length;

    adjustDocumentChangesAndCursors(pos, -int(length), op);

    return w;
}

int QTextDocumentPrivate::remove_block(int pos, int *blockFormat, int command, QTextUndoCommand::Operation op)
{
    Q_ASSERT(pos >= 0);
    Q_ASSERT(blocks.length() == fragments.length());
    Q_ASSERT(blocks.length() > pos);

    int b = blocks.findNode(pos);
    uint x = fragments.findNode(pos);

    Q_ASSERT(x && (int)fragments.position(x) == pos);
    Q_ASSERT(fragments.size(x) == 1);
    Q_ASSERT(isValidBlockSeparator(text.at(fragments.fragment(x)->stringPosition)));
    Q_ASSERT(b);

    if (blocks.size(b) == 1 && command == QTextUndoCommand::BlockAdded) {
	Q_ASSERT((int)blocks.position(b) == pos);
//  	qDebug("removing empty block");
	// empty block remove the block itself
    } else {
	// non empty block, merge with next one into this block
//  	qDebug("merging block with next");
	int n = blocks.next(b);
	Q_ASSERT((int)blocks.position(n) == pos + 1);
	blocks.setSize(b, blocks.size(b) + blocks.size(n) - 1);
        blocks.fragment(b)->userState = blocks.fragment(n)->userState;
	b = n;
    }
    *blockFormat = blocks.fragment(b)->format;

    QTextBlockGroup *group = qobject_cast<QTextBlockGroup *>(objectForFormat(blocks.fragment(b)->format));
    if (group)
        group->blockRemoved(QTextBlock(this, b));

    QTextFrame *frame = qobject_cast<QTextFrame *>(objectForFormat(fragments.fragment(x)->format));
    if (frame) {
        frame->d_func()->fragmentRemoved(text.at(fragments.fragment(x)->stringPosition), x);
        framesDirty = true;
    }

    blocks.erase_single(b);
    const int w = fragments.erase_single(x);

    adjustDocumentChangesAndCursors(pos, -1, op);

    return w;
}

#if !defined(QT_NO_DEBUG)
static bool isAncestorFrame(QTextFrame *possibleAncestor, QTextFrame *child)
{
    while (child) {
        if (child == possibleAncestor)
            return true;
        child = child->parentFrame();
    }
    return false;
}
#endif

void QTextDocumentPrivate::move(int pos, int to, int length, QTextUndoCommand::Operation op)
{
    Q_ASSERT(to <= fragments.length() && to <= pos);
    Q_ASSERT(pos >= 0 && pos+length <= fragments.length());
    Q_ASSERT(blocks.length() == fragments.length());

    if (pos == to)
        return;

    const bool needsInsert = to != -1;

#if !defined(QT_NO_DEBUG)
    const bool startAndEndInSameFrame = (frameAt(pos) == frameAt(pos + length - 1));

    const bool endIsEndOfChildFrame = (isAncestorFrame(frameAt(pos), frameAt(pos + length - 1))
                                       && text.at(find(pos + length - 1)->stringPosition) == QTextEndOfFrame);

    const bool startIsStartOfFrameAndEndIsEndOfFrameWithCommonParent
               = (text.at(find(pos)->stringPosition) == QTextBeginningOfFrame
                  && text.at(find(pos + length - 1)->stringPosition) == QTextEndOfFrame
                  && frameAt(pos)->parentFrame() == frameAt(pos + length - 1)->parentFrame());

    const bool isFirstTableCell = (qobject_cast<QTextTable *>(frameAt(pos + length - 1))
                                  && frameAt(pos + length - 1)->parentFrame() == frameAt(pos));

    Q_ASSERT(startAndEndInSameFrame || endIsEndOfChildFrame || startIsStartOfFrameAndEndIsEndOfFrameWithCommonParent || isFirstTableCell);
#endif

    split(pos);
    split(pos+length);

    uint dst = needsInsert ? fragments.findNode(to) : 0;
    uint dstKey = needsInsert ? fragments.position(dst) : 0;

    uint x = fragments.findNode(pos);
    uint end = fragments.findNode(pos+length);

    uint w = 0;
    while (x != end) {
        uint n = fragments.next(x);

        uint key = fragments.position(x);
        uint b = blocks.findNode(key+1);
        QTextBlockData *B = blocks.fragment(b);
        int blockRevision = B->revision;

        QTextFragmentData *X = fragments.fragment(x);
        QT_INIT_TEXTUNDOCOMMAND(c, QTextUndoCommand::Removed, (editBlock != 0),
                                op, X->format, X->stringPosition, key, X->size_array[0],
                                blockRevision);
        QT_INIT_TEXTUNDOCOMMAND(cInsert, QTextUndoCommand::Inserted, (editBlock != 0),
                                op, X->format, X->stringPosition, dstKey, X->size_array[0],
                                blockRevision);

        if (key+1 != blocks.position(b)) {
//	    qDebug("remove_string from %d length %d", key, X->size_array[0]);
            Q_ASSERT(noBlockInString(text.mid(X->stringPosition, X->size_array[0])));
            w = remove_string(key, X->size_array[0], op);

            if (needsInsert) {
                insert_string(dstKey, X->stringPosition, X->size_array[0], X->format, op);
                dstKey += X->size_array[0];
            }
        } else {
//	    qDebug("remove_block at %d", key);
            Q_ASSERT(X->size_array[0] == 1 && isValidBlockSeparator(text.at(X->stringPosition)));
            b = blocks.previous(b);
            B = 0;
            c.command = blocks.size(b) == 1 ? QTextUndoCommand::BlockDeleted : QTextUndoCommand::BlockRemoved;
            w = remove_block(key, &c.blockFormat, QTextUndoCommand::BlockAdded, op);

            if (needsInsert) {
                insert_block(dstKey++, X->stringPosition, X->format, c.blockFormat, op, QTextUndoCommand::BlockRemoved);
                cInsert.command = blocks.size(b) == 1 ? QTextUndoCommand::BlockAdded : QTextUndoCommand::BlockInserted;
                cInsert.blockFormat = c.blockFormat;
            }
        }
        appendUndoItem(c);
        if (B)
            B->revision = revision;
        x = n;

        if (needsInsert)
            appendUndoItem(cInsert);
    }
    if (w)
        unite(w);

    Q_ASSERT(blocks.length() == fragments.length());

    if (!blockCursorAdjustment)
        finishEdit();
}

void QTextDocumentPrivate::remove(int pos, int length, QTextUndoCommand::Operation op)
{
    if (length == 0)
        return;
    blockCursorAdjustment = true;
    move(pos, -1, length, op);
    blockCursorAdjustment = false;
    foreach (QTextCursorPrivate *curs, cursors) {
        if (curs->adjustPosition(pos, -length, op) == QTextCursorPrivate::CursorMoved) {
            curs->changed = true;
        }
    }
    finishEdit();
}

void QTextDocumentPrivate::setCharFormat(int pos, int length, const QTextCharFormat &newFormat, FormatChangeMode mode)
{
    beginEditBlock();

    Q_ASSERT(newFormat.isValid());

    int newFormatIdx = -1;
    if (mode == SetFormatAndPreserveObjectIndices) {
        QTextCharFormat cleanFormat = newFormat;
        cleanFormat.clearProperty(QTextFormat::ObjectIndex);
        newFormatIdx = formats.indexForFormat(cleanFormat);
    } else if (mode == SetFormat) {
        newFormatIdx = formats.indexForFormat(newFormat);
    }

    if (pos == -1) {
        if (mode == MergeFormat) {
            QTextFormat format = formats.format(initialBlockCharFormatIndex);
            format.merge(newFormat);
            initialBlockCharFormatIndex = formats.indexForFormat(format);
        } else if (mode == SetFormatAndPreserveObjectIndices
                   && formats.format(initialBlockCharFormatIndex).objectIndex() != -1) {
            QTextCharFormat f = newFormat;
            f.setObjectIndex(formats.format(initialBlockCharFormatIndex).objectIndex());
            initialBlockCharFormatIndex = formats.indexForFormat(f);
        } else {
            initialBlockCharFormatIndex = newFormatIdx;
        }

        ++pos;
        --length;
    }

    const int startPos = pos;
    const int endPos = pos + length;

    split(startPos);
    split(endPos);

    while (pos < endPos) {
        FragmentMap::Iterator it = fragments.find(pos);
        Q_ASSERT(!it.atEnd());

        QTextFragmentData *fragment = it.value();

        Q_ASSERT(formats.format(fragment->format).type() == QTextFormat::CharFormat);

        int offset = pos - it.position();
        int length = qMin(endPos - pos, int(fragment->size_array[0] - offset));
        int oldFormat = fragment->format;

        if (mode == MergeFormat) {
            QTextFormat format = formats.format(fragment->format);
            format.merge(newFormat);
            fragment->format = formats.indexForFormat(format);
        } else if (mode == SetFormatAndPreserveObjectIndices
                   && formats.format(oldFormat).objectIndex() != -1) {
            QTextCharFormat f = newFormat;
            f.setObjectIndex(formats.format(oldFormat).objectIndex());
            fragment->format = formats.indexForFormat(f);
        } else {
            fragment->format = newFormatIdx;
        }

        QT_INIT_TEXTUNDOCOMMAND(c, QTextUndoCommand::CharFormatChanged, true, QTextUndoCommand::MoveCursor, oldFormat,
                                0, pos, length, 0);
        appendUndoItem(c);

        pos += length;
        Q_ASSERT(pos == (int)(it.position() + fragment->size_array[0]) || pos >= endPos);
    }

    int n = fragments.findNode(startPos - 1);
    if (n)
        unite(n);

    n = fragments.findNode(endPos);
    if (n)
        unite(n);

    QTextBlock blockIt = blocksFind(startPos);
    QTextBlock endIt = blocksFind(endPos);
    if (endIt.isValid())
        endIt = endIt.next();
    for (; blockIt.isValid() && blockIt != endIt; blockIt = blockIt.next())
        QTextDocumentPrivate::block(blockIt)->invalidate();

    documentChange(startPos, length);

    endEditBlock();
}

void QTextDocumentPrivate::setBlockFormat(const QTextBlock &from, const QTextBlock &to,
				     const QTextBlockFormat &newFormat, FormatChangeMode mode)
{
    beginEditBlock();

    Q_ASSERT(mode != SetFormatAndPreserveObjectIndices); // only implemented for setCharFormat

    Q_ASSERT(newFormat.isValid());

    int newFormatIdx = -1;
    if (mode == SetFormat)
        newFormatIdx = formats.indexForFormat(newFormat);
    QTextBlockGroup *group = qobject_cast<QTextBlockGroup *>(objectForFormat(newFormat));

    QTextBlock it = from;
    QTextBlock end = to;
    if (end.isValid())
	end = end.next();

    for (; it != end; it = it.next()) {
        int oldFormat = block(it)->format;
        QTextBlockFormat format = formats.blockFormat(oldFormat);
        QTextBlockGroup *oldGroup = qobject_cast<QTextBlockGroup *>(objectForFormat(format));
        if (mode == MergeFormat) {
            format.merge(newFormat);
            newFormatIdx = formats.indexForFormat(format);
            group = qobject_cast<QTextBlockGroup *>(objectForFormat(format));
        }
        block(it)->format = newFormatIdx;

        block(it)->invalidate();

        QT_INIT_TEXTUNDOCOMMAND(c, QTextUndoCommand::BlockFormatChanged, true, QTextUndoCommand::MoveCursor, oldFormat,
                                0, it.position(), 1, 0);
        appendUndoItem(c);

        if (group != oldGroup) {
            if (oldGroup)
                oldGroup->blockRemoved(it);
            if (group)
                group->blockInserted(it);
        } else if (group) {
	    group->blockFormatChanged(it);
	}
    }

    documentChange(from.position(), to.position() + to.length() - from.position());

    endEditBlock();
}


bool QTextDocumentPrivate::split(int pos)
{
    uint x = fragments.findNode(pos);
    if (x) {
        int k = fragments.position(x);
//          qDebug("found fragment with key %d, size_left=%d, size=%d to split at %d",
//                k, (*it)->size_left[0], (*it)->size_array[0], pos);
        if (k != pos) {
            Q_ASSERT(k <= pos);
            // need to resize the first fragment and add a new one
            QTextFragmentData *X = fragments.fragment(x);
            int oldsize = X->size_array[0];
            fragments.setSize(x, pos-k);
            uint n = fragments.insert_single(pos, oldsize-(pos-k));
            X = fragments.fragment(x);
            QTextFragmentData *N = fragments.fragment(n);
            N->stringPosition = X->stringPosition + pos-k;
            N->format = X->format;
            return true;
        }
    }
    return false;
}

bool QTextDocumentPrivate::unite(uint f)
{
    uint n = fragments.next(f);
    if (!n)
        return false;

    QTextFragmentData *ff = fragments.fragment(f);
    QTextFragmentData *nf = fragments.fragment(n);

    if (nf->format == ff->format && (ff->stringPosition + (int)ff->size_array[0] == nf->stringPosition)) {
        if (isValidBlockSeparator(text.at(ff->stringPosition))
            || isValidBlockSeparator(text.at(nf->stringPosition)))
            return false;

        fragments.setSize(f, ff->size_array[0] + nf->size_array[0]);
        fragments.erase_single(n);
        return true;
    }
    return false;
}


int QTextDocumentPrivate::undoRedo(bool undo)
{
    PMDEBUG("%s, undoState=%d, undoStack size=%d", undo ? "undo:" : "redo:", undoState, undoStack.size());
    if (!undoEnabled || (undo && undoState == 0) || (!undo && undoState == undoStack.size()))
        return -1;

    undoEnabled = false;
    beginEditBlock();
    int editPos = -1;
    int editLength = -1;
    while (1) {
        if (undo)
            --undoState;
        QTextUndoCommand &c = undoStack[undoState];
        int resetBlockRevision = c.pos;

	switch(c.command) {
        case QTextUndoCommand::Inserted:
            remove(c.pos, c.length, (QTextUndoCommand::Operation)c.operation);
            PMDEBUG("   erase: from %d, length %d", c.pos, c.length);
            c.command = QTextUndoCommand::Removed;
            editPos = c.pos;
            editLength = 0;
	    break;
        case QTextUndoCommand::Removed:
            PMDEBUG("   insert: format %d (from %d, length %d, strpos=%d)", c.format, c.pos, c.length, c.strPos);
            insert_string(c.pos, c.strPos, c.length, c.format, (QTextUndoCommand::Operation)c.operation);
            c.command = QTextUndoCommand::Inserted;
            if (editPos != (int)c.pos)
                editLength = 0;
            editPos = c.pos;
            editLength += c.length;
	    break;
	case QTextUndoCommand::BlockInserted:
	case QTextUndoCommand::BlockAdded:
            remove_block(c.pos, &c.blockFormat, c.command, (QTextUndoCommand::Operation)c.operation);
            PMDEBUG("   blockremove: from %d", c.pos);
	    if (c.command == QTextUndoCommand::BlockInserted)
		c.command = QTextUndoCommand::BlockRemoved;
	    else
		c.command = QTextUndoCommand::BlockDeleted;
            editPos = c.pos;
            editLength = 0;
	    break;
	case QTextUndoCommand::BlockRemoved:
	case QTextUndoCommand::BlockDeleted:
            PMDEBUG("   blockinsert: charformat %d blockformat %d (pos %d, strpos=%d)", c.format, c.blockFormat, c.pos, c.strPos);
            insert_block(c.pos, c.strPos, c.format, c.blockFormat, (QTextUndoCommand::Operation)c.operation, c.command);
            resetBlockRevision += 1;
	    if (c.command == QTextUndoCommand::BlockRemoved)
		c.command = QTextUndoCommand::BlockInserted;
	    else
		c.command = QTextUndoCommand::BlockAdded;
            if (editPos != (int)c.pos)
                editLength = 0;
            editPos = c.pos;
            editLength += 1;
	    break;
	case QTextUndoCommand::CharFormatChanged: {
            resetBlockRevision = -1; // ## TODO
            PMDEBUG("   charFormat: format %d (from %d, length %d)", c.format, c.pos, c.length);
            FragmentIterator it = find(c.pos);
            Q_ASSERT(!it.atEnd());

            int oldFormat = it.value()->format;
            setCharFormat(c.pos, c.length, formats.charFormat(c.format));
            c.format = oldFormat;
            if (editPos != (int)c.pos)
                editLength = 0;
            editPos = c.pos;
            editLength += c.length;
	    break;
	}
	case QTextUndoCommand::BlockFormatChanged: {
            resetBlockRevision = -1; // ## TODO
            PMDEBUG("   blockformat: format %d pos %d", c.format, c.pos);
            QTextBlock it = blocksFind(c.pos);
            Q_ASSERT(it.isValid());

            int oldFormat = block(it)->format;
            block(it)->format = c.format;
            QTextBlockGroup *oldGroup = qobject_cast<QTextBlockGroup *>(objectForFormat(formats.blockFormat(oldFormat)));
            QTextBlockGroup *group = qobject_cast<QTextBlockGroup *>(objectForFormat(formats.blockFormat(c.format)));
            c.format = oldFormat;
            if (group != oldGroup) {
                if (oldGroup)
                    oldGroup->blockRemoved(it);
                if (group)
                    group->blockInserted(it);
            } else if (group) {
                group->blockFormatChanged(it);
            }
            documentChange(it.position(), it.length());
            editPos = -1;
	    break;
	}
	case QTextUndoCommand::GroupFormatChange: {
            resetBlockRevision = -1; // ## TODO
            PMDEBUG("   group format change");
            QTextObject *object = objectForIndex(c.objectIndex);
            int oldFormat = formats.objectFormatIndex(c.objectIndex);
            changeObjectFormat(object, c.format);
            c.format = oldFormat;
            editPos = -1;
	    break;
	}
        case QTextUndoCommand::CursorMoved:
            editPos = c.pos;
            editLength = 0;
            break;
	case QTextUndoCommand::Custom:
            resetBlockRevision = -1; // ## TODO
            if (undo)
                c.custom->undo();
            else
                c.custom->redo();
            editPos = -1;
	    break;
	default:
	    Q_ASSERT(false);
        }

        if (resetBlockRevision >= 0) {
            int b = blocks.findNode(resetBlockRevision);
            QTextBlockData *B = blocks.fragment(b);
            B->revision = c.revision;
        }

        if (!undo)
            ++undoState;

        bool inBlock = (
                undoState > 0
                && undoState < undoStack.size()
                && undoStack[undoState].block_part
                && undoStack[undoState-1].block_part
                && !undoStack[undoState-1].block_end
                );
        if (!inBlock)
            break;
    }
    undoEnabled = true;

    int newCursorPos = -1;

    if (editPos >=0)
        newCursorPos = editPos + editLength;
    else if (docChangeFrom >= 0)
        newCursorPos= qMin(docChangeFrom + docChangeLength, length() - 1);

    endEditBlock();
    emitUndoAvailable(isUndoAvailable());
    emitRedoAvailable(isRedoAvailable());

    return newCursorPos;
}

/*!
    Appends a custom undo \a item to the undo stack.
*/
void QTextDocumentPrivate::appendUndoItem(QAbstractUndoItem *item)
{
    if (!undoEnabled) {
        delete item;
        return;
    }

    QTextUndoCommand c;
    c.command = QTextUndoCommand::Custom;
    c.block_part = editBlock != 0;
    c.block_end = 0;
    c.operation = QTextUndoCommand::MoveCursor;
    c.format = 0;
    c.strPos = 0;
    c.pos = 0;
    c.blockFormat = 0;

    c.custom = item;
    appendUndoItem(c);
}

void QTextDocumentPrivate::appendUndoItem(const QTextUndoCommand &c)
{
    PMDEBUG("appendUndoItem, command=%d enabled=%d", c.command, undoEnabled);
    if (!undoEnabled)
        return;
    if (undoState < undoStack.size())
        clearUndoRedoStacks(QTextDocument::RedoStack);

    if (editBlock != 0 && editBlockCursorPosition >= 0) { // we had a beginEditBlock() with a cursor position
        if (c.pos != (quint32) editBlockCursorPosition) { // and that cursor position is different from the command
            // generate a CursorMoved undo item
            QT_INIT_TEXTUNDOCOMMAND(cc, QTextUndoCommand::CursorMoved, true, QTextUndoCommand::MoveCursor,
                                    0, 0, editBlockCursorPosition, 0, 0);
            undoStack.append(cc);
            undoState++;
            editBlockCursorPosition = -1;
        }
    }


    if (!undoStack.isEmpty() && modified) {
        QTextUndoCommand &last = undoStack[undoState - 1];

        if ( (last.block_part && c.block_part && !last.block_end) // part of the same block => can merge
            || (!c.block_part && !last.block_part)) {  // two single undo items => can merge

            if (last.tryMerge(c))
                return;
        }
    }
    if (modifiedState > undoState)
        modifiedState = -1;
    undoStack.append(c);
    undoState++;
    emitUndoAvailable(true);
    emitRedoAvailable(false);

    if (!c.block_part)
        emit document()->undoCommandAdded();
}

void QTextDocumentPrivate::clearUndoRedoStacks(QTextDocument::Stacks stacksToClear,
                                               bool emitSignals)
{
    bool undoCommandsAvailable = undoState != 0;
    bool redoCommandsAvailable = undoState != undoStack.size();
    if (stacksToClear == QTextDocument::UndoStack && undoCommandsAvailable) {
        for (int i = 0; i < undoState; ++i) {
            QTextUndoCommand c = undoStack[undoState];
            if (c.command & QTextUndoCommand::Custom)
                delete c.custom;
        }
        undoStack.remove(0, undoState);
        undoStack.resize(undoStack.size() - undoState);
        undoState = 0;
        if (emitSignals)
            emitUndoAvailable(false);
    } else if (stacksToClear == QTextDocument::RedoStack
               && redoCommandsAvailable) {
        for (int i = undoState; i < undoStack.size(); ++i) {
            QTextUndoCommand c = undoStack[i];
            if (c.command & QTextUndoCommand::Custom)
                delete c.custom;
        }
        undoStack.resize(undoState);
        if (emitSignals)
            emitRedoAvailable(false);
    } else if (stacksToClear == QTextDocument::UndoAndRedoStacks
               && !undoStack.isEmpty()) {
        for (int i = 0; i < undoStack.size(); ++i) {
            QTextUndoCommand c = undoStack[i];
            if (c.command & QTextUndoCommand::Custom)
                delete c.custom;
        }
        undoState = 0;
        undoStack.resize(0);
        if (emitSignals && undoCommandsAvailable)
            emitUndoAvailable(false);
        if (emitSignals && redoCommandsAvailable)
            emitRedoAvailable(false);
    }
}

void QTextDocumentPrivate::emitUndoAvailable(bool available)
{
    if (available != wasUndoAvailable) {
        Q_Q(QTextDocument);
        emit q->undoAvailable(available);
        wasUndoAvailable = available;
    }
}

void QTextDocumentPrivate::emitRedoAvailable(bool available)
{
    if (available != wasRedoAvailable) {
        Q_Q(QTextDocument);
        emit q->redoAvailable(available);
        wasRedoAvailable = available;
    }
}

void QTextDocumentPrivate::enableUndoRedo(bool enable)
{
    if (enable && maximumBlockCount > 0)
        return;

    if (!enable) {
        undoState = 0;
        clearUndoRedoStacks(QTextDocument::RedoStack);
        emitUndoAvailable(false);
        emitRedoAvailable(false);
    }
    modifiedState = modified ? -1 : undoState;
    undoEnabled = enable;
    if (!undoEnabled)
        compressPieceTable();
}

void QTextDocumentPrivate::joinPreviousEditBlock()
{
    beginEditBlock();

    if (undoEnabled && undoState)
        undoStack[undoState - 1].block_end = false;
}

void QTextDocumentPrivate::endEditBlock()
{
    Q_ASSERT(editBlock > 0);
    if (--editBlock)
        return;

    if (undoEnabled && undoState > 0) {
        const bool wasBlocking = !undoStack[undoState - 1].block_end;
        if (undoStack[undoState - 1].block_part) {
            undoStack[undoState - 1].block_end = true;
            if (wasBlocking)
                emit document()->undoCommandAdded();
        }
    }

    editBlockCursorPosition = -1;

    finishEdit();
}

void QTextDocumentPrivate::finishEdit()
{
    Q_Q(QTextDocument);

    if (editBlock)
        return;

    if (framesDirty)
        scan_frames(docChangeFrom, docChangeOldLength, docChangeLength);

    if (lout && docChangeFrom >= 0) {
        if (!inContentsChange) {
            inContentsChange = true;
            emit q->contentsChange(docChangeFrom, docChangeOldLength, docChangeLength);
            inContentsChange = false;
        }
        lout->documentChanged(docChangeFrom, docChangeOldLength, docChangeLength);
    }

    docChangeFrom = -1;

    if (needsEnsureMaximumBlockCount) {
        needsEnsureMaximumBlockCount = false;
        if (ensureMaximumBlockCount()) {
            // if ensureMaximumBlockCount() returns true
            // it will have called endEditBlock() and
            // compressPieceTable() itself, so we return here
            // to prevent getting two contentsChanged emits
            return;
        }
    }

    QList<QTextCursor> changedCursors;
    foreach (QTextCursorPrivate *curs, cursors) {
        if (curs->changed) {
            curs->changed = false;
            changedCursors.append(QTextCursor(curs));
        }
    }
    foreach (const QTextCursor &cursor, changedCursors)
        emit q->cursorPositionChanged(cursor);

    contentsChanged();

    if (blocks.numNodes() != lastBlockCount) {
        lastBlockCount = blocks.numNodes();
        emit q->blockCountChanged(lastBlockCount);
    }

    if (!undoEnabled && unreachableCharacterCount)
        compressPieceTable();
}

void QTextDocumentPrivate::documentChange(int from, int length)
{
//     qDebug("QTextDocumentPrivate::documentChange: from=%d,length=%d", from, length);
    if (docChangeFrom < 0) {
        docChangeFrom = from;
        docChangeOldLength = length;
        docChangeLength = length;
        return;
    }
    int start = qMin(from, docChangeFrom);
    int end = qMax(from + length, docChangeFrom + docChangeLength);
    int diff = qMax(0, end - start - docChangeLength);
    docChangeFrom = start;
    docChangeOldLength += diff;
    docChangeLength += diff;
}

/*
    adjustDocumentChangesAndCursors is called whenever there is an insert or remove of characters.
    param from is the cursor position in the document
    param addedOrRemoved is the amount of characters added or removed.  A negative number means characters are removed.

    The function stores information to be emitted when finishEdit() is called.
*/
void QTextDocumentPrivate::adjustDocumentChangesAndCursors(int from, int addedOrRemoved, QTextUndoCommand::Operation op)
{
    if (!editBlock)
        ++revision;

    if (blockCursorAdjustment)  {
        ; // postpone, will be called again from QTextDocumentPrivate::remove()
    } else {
        foreach (QTextCursorPrivate *curs, cursors) {
            if (curs->adjustPosition(from, addedOrRemoved, op) == QTextCursorPrivate::CursorMoved) {
                curs->changed = true;
            }
        }
    }

//     qDebug("QTextDocumentPrivate::adjustDocumentChanges: from=%d,addedOrRemoved=%d", from, addedOrRemoved);
    if (docChangeFrom < 0) {
        docChangeFrom = from;
        if (addedOrRemoved > 0) {
            docChangeOldLength = 0;
            docChangeLength = addedOrRemoved;
        } else {
            docChangeOldLength = -addedOrRemoved;
            docChangeLength = 0;
        }
//         qDebug("adjustDocumentChanges:");
//         qDebug("    -> %d %d %d", docChangeFrom, docChangeOldLength, docChangeLength);
        return;
    }

    // have to merge the new change with the already existing one.
    int added = qMax(0, addedOrRemoved);
    int removed = qMax(0, -addedOrRemoved);

    int diff = 0;
    if(from + removed < docChangeFrom)
        diff = docChangeFrom - from - removed;
    else if(from > docChangeFrom + docChangeLength)
        diff = from - (docChangeFrom + docChangeLength);

    int overlap_start = qMax(from, docChangeFrom);
    int overlap_end = qMin(from + removed, docChangeFrom + docChangeLength);
    int removedInside = qMax(0, overlap_end - overlap_start);
    removed -= removedInside;

//     qDebug("adjustDocumentChanges: from=%d, addedOrRemoved=%d, diff=%d, removedInside=%d", from, addedOrRemoved, diff, removedInside);
    docChangeFrom = qMin(docChangeFrom, from);
    docChangeOldLength += removed + diff;
    docChangeLength += added - removedInside + diff;
//     qDebug("    -> %d %d %d", docChangeFrom, docChangeOldLength, docChangeLength);

}


QString QTextDocumentPrivate::plainText() const
{
    QString result;
    result.resize(length());
    const QChar *text_unicode = text.unicode();
    QChar *data = result.data();
    for (QTextDocumentPrivate::FragmentIterator it = begin(); it != end(); ++it) {
        const QTextFragmentData *f = *it;
        ::memcpy(data, text_unicode + f->stringPosition, f->size_array[0] * sizeof(QChar));
        data += f->size_array[0];
    }
    // remove trailing block separator
    result.chop(1);
    return result;
}

int QTextDocumentPrivate::blockCharFormatIndex(int node) const
{
    int pos = blocks.position(node);
    if (pos == 0)
        return initialBlockCharFormatIndex;

    return fragments.find(pos - 1)->format;
}

int QTextDocumentPrivate::nextCursorPosition(int position, QTextLayout::CursorMode mode) const
{
    if (position == length()-1)
        return position;

    QTextBlock it = blocksFind(position);
    int start = it.position();
    int end = start + it.length() - 1;
    if (position == end)
        return end + 1;

    return it.layout()->nextCursorPosition(position-start, mode) + start;
}

int QTextDocumentPrivate::previousCursorPosition(int position, QTextLayout::CursorMode mode) const
{
    if (position == 0)
        return position;

    QTextBlock it = blocksFind(position);
    int start = it.position();
    if (position == start)
        return start - 1;

    return it.layout()->previousCursorPosition(position-start, mode) + start;
}

int QTextDocumentPrivate::leftCursorPosition(int position) const
{
    QTextBlock it = blocksFind(position);
    int start = it.position();
    return it.layout()->leftCursorPosition(position-start) + start;
}

int QTextDocumentPrivate::rightCursorPosition(int position) const
{
    QTextBlock it = blocksFind(position);
    int start = it.position();
    return it.layout()->rightCursorPosition(position-start) + start;
}

void QTextDocumentPrivate::changeObjectFormat(QTextObject *obj, int format)
{
    beginEditBlock();
    int objectIndex = obj->objectIndex();
    int oldFormatIndex = formats.objectFormatIndex(objectIndex);
    formats.setObjectFormatIndex(objectIndex, format);

    QTextBlockGroup *b = qobject_cast<QTextBlockGroup *>(obj);
    if (b) {
        b->d_func()->markBlocksDirty();
    }
    QTextFrame *f = qobject_cast<QTextFrame *>(obj);
    if (f)
        documentChange(f->firstPosition(), f->lastPosition() - f->firstPosition());

    QT_INIT_TEXTUNDOCOMMAND(c, QTextUndoCommand::GroupFormatChange, (editBlock != 0), QTextUndoCommand::MoveCursor, oldFormatIndex,
                            0, 0, obj->d_func()->objectIndex, 0);
    appendUndoItem(c);

    endEditBlock();
}

static QTextFrame *findChildFrame(QTextFrame *f, int pos)
{
    /* Binary search for frame at pos */
    const QList<QTextFrame *> children = f->childFrames();
    int first = 0;
    int last = children.size() - 1;
    while (first <= last) {
        int mid = (first + last) / 2;
        QTextFrame *c = children.at(mid);
        if (pos > c->lastPosition())
            first = mid + 1;
        else if (pos < c->firstPosition())
            last = mid - 1;
        else
            return c;
    }
    return 0;
}

QTextFrame *QTextDocumentPrivate::rootFrame() const
{
    if (!rtFrame) {
        QTextFrameFormat defaultRootFrameFormat;
        defaultRootFrameFormat.setMargin(documentMargin);
        rtFrame = qobject_cast<QTextFrame *>(const_cast<QTextDocumentPrivate *>(this)->createObject(defaultRootFrameFormat));
    }
    return rtFrame;
}

QTextFrame *QTextDocumentPrivate::frameAt(int pos) const
{
    QTextFrame *f = rootFrame();

    while (1) {
        QTextFrame *c = findChildFrame(f, pos);
        if (!c)
            return f;
        f = c;
    }
}

void QTextDocumentPrivate::clearFrame(QTextFrame *f)
{
    for (int i = 0; i < f->d_func()->childFrames.count(); ++i)
        clearFrame(f->d_func()->childFrames.at(i));
    f->d_func()->childFrames.clear();
    f->d_func()->parentFrame = 0;
}

void QTextDocumentPrivate::scan_frames(int pos, int charsRemoved, int charsAdded)
{
    // ###### optimize
    Q_UNUSED(pos);
    Q_UNUSED(charsRemoved);
    Q_UNUSED(charsAdded);

    QTextFrame *f = rootFrame();
    clearFrame(f);

    for (FragmentIterator it = begin(); it != end(); ++it) {
        // QTextFormat fmt = formats.format(it->format);
        QTextFrame *frame = qobject_cast<QTextFrame *>(objectForFormat(it->format));
        if (!frame)
            continue;

        Q_ASSERT(it.size() == 1);
        QChar ch = text.at(it->stringPosition);

        if (ch == QTextBeginningOfFrame) {
            if (f != frame) {
                // f == frame happens for tables
                Q_ASSERT(frame->d_func()->fragment_start == it.n || frame->d_func()->fragment_start == 0);
                frame->d_func()->parentFrame = f;
                f->d_func()->childFrames.append(frame);
                f = frame;
            }
        } else if (ch == QTextEndOfFrame) {
            Q_ASSERT(f == frame);
            Q_ASSERT(frame->d_func()->fragment_end == it.n || frame->d_func()->fragment_end == 0);
            f = frame->d_func()->parentFrame;
        } else if (ch == QChar::ObjectReplacementCharacter) {
            Q_ASSERT(f != frame);
            Q_ASSERT(frame->d_func()->fragment_start == it.n || frame->d_func()->fragment_start == 0);
            Q_ASSERT(frame->d_func()->fragment_end == it.n || frame->d_func()->fragment_end == 0);
            frame->d_func()->parentFrame = f;
            f->d_func()->childFrames.append(frame);
        } else {
            Q_ASSERT(false);
        }
    }
    Q_ASSERT(f == rtFrame);
    framesDirty = false;
}

void QTextDocumentPrivate::insert_frame(QTextFrame *f)
{
    int start = f->firstPosition();
    int end = f->lastPosition();
    QTextFrame *parent = frameAt(start-1);
    Q_ASSERT(parent == frameAt(end+1));

    if (start != end) {
        // iterator over the parent and move all children contained in my frame to myself
        for (int i = 0; i < parent->d_func()->childFrames.size(); ++i) {
            QTextFrame *c = parent->d_func()->childFrames.at(i);
            if (start < c->firstPosition() && end > c->lastPosition()) {
                parent->d_func()->childFrames.removeAt(i);
                f->d_func()->childFrames.append(c);
                c->d_func()->parentFrame = f;
            }
        }
    }
    // insert at the correct position
    int i = 0;
    for (; i < parent->d_func()->childFrames.size(); ++i) {
        QTextFrame *c = parent->d_func()->childFrames.at(i);
        if (c->firstPosition() > end)
            break;
    }
    parent->d_func()->childFrames.insert(i, f);
    f->d_func()->parentFrame = parent;
}

QTextFrame *QTextDocumentPrivate::insertFrame(int start, int end, const QTextFrameFormat &format)
{
    Q_ASSERT(start >= 0 && start < length());
    Q_ASSERT(end >= 0 && end < length());
    Q_ASSERT(start <= end || end == -1);

    if (start != end && frameAt(start) != frameAt(end))
        return 0;

    beginEditBlock();

    QTextFrame *frame = qobject_cast<QTextFrame *>(createObject(format));
    Q_ASSERT(frame);

    // #### using the default block and char format below might be wrong
    int idx = formats.indexForFormat(QTextBlockFormat());
    QTextCharFormat cfmt;
    cfmt.setObjectIndex(frame->objectIndex());
    int charIdx = formats.indexForFormat(cfmt);

    insertBlock(QTextBeginningOfFrame, start, idx, charIdx, QTextUndoCommand::MoveCursor);
    insertBlock(QTextEndOfFrame, ++end, idx, charIdx, QTextUndoCommand::KeepCursor);

    frame->d_func()->fragment_start = find(start).n;
    frame->d_func()->fragment_end = find(end).n;

    insert_frame(frame);

    endEditBlock();

    return frame;
}

void QTextDocumentPrivate::removeFrame(QTextFrame *frame)
{
    QTextFrame *parent = frame->d_func()->parentFrame;
    if (!parent)
        return;

    int start = frame->firstPosition();
    int end = frame->lastPosition();
    Q_ASSERT(end >= start);

    beginEditBlock();

    // remove already removes the frames from the tree
    remove(end, 1);
    remove(start-1, 1);

    endEditBlock();
}

QTextObject *QTextDocumentPrivate::objectForIndex(int objectIndex) const
{
    if (objectIndex < 0)
        return 0;

    QTextObject *object = objects.value(objectIndex, 0);
    if (!object) {
        QTextDocumentPrivate *that = const_cast<QTextDocumentPrivate *>(this);
        QTextFormat fmt = formats.objectFormat(objectIndex);
        object = that->createObject(fmt, objectIndex);
    }
    return object;
}

QTextObject *QTextDocumentPrivate::objectForFormat(int formatIndex) const
{
    int objectIndex = formats.format(formatIndex).objectIndex();
    return objectForIndex(objectIndex);
}

QTextObject *QTextDocumentPrivate::objectForFormat(const QTextFormat &f) const
{
    return objectForIndex(f.objectIndex());
}

QTextObject *QTextDocumentPrivate::createObject(const QTextFormat &f, int objectIndex)
{
    QTextObject *obj = document()->createObject(f);

    if (obj) {
        obj->d_func()->objectIndex = objectIndex == -1 ? formats.createObjectIndex(f) : objectIndex;
        objects[obj->d_func()->objectIndex] = obj;
    }

    return obj;
}

void QTextDocumentPrivate::deleteObject(QTextObject *object)
{
    const int objIdx = object->d_func()->objectIndex;
    objects.remove(objIdx);
    delete object;
}

void QTextDocumentPrivate::contentsChanged()
{
    Q_Q(QTextDocument);
    if (editBlock)
        return;

    bool m = undoEnabled ? (modifiedState != undoState) : true;
    if (modified != m) {
        modified = m;
        emit q->modificationChanged(modified);
    }

    emit q->contentsChanged();
}

void QTextDocumentPrivate::compressPieceTable()
{
    if (undoEnabled)
        return;

    const uint garbageCollectionThreshold = 96 * 1024; // bytes

    //qDebug() << "unreachable bytes:" << unreachableCharacterCount * sizeof(QChar) << " -- limit" << garbageCollectionThreshold << "text size =" << text.size() << "capacity:" << text.capacity();

    bool compressTable = unreachableCharacterCount * sizeof(QChar) > garbageCollectionThreshold
                         && text.size() >= text.capacity() * 0.9;
    if (!compressTable)
        return;

    QString newText;
    newText.resize(text.size());
    QChar *newTextPtr = newText.data();
    int newLen = 0;

    for (FragmentMap::Iterator it = fragments.begin(); !it.atEnd(); ++it) {
        memcpy(newTextPtr, text.constData() + it->stringPosition, it->size_array[0] * sizeof(QChar));
        it->stringPosition = newLen;
        newTextPtr += it->size_array[0];
        newLen += it->size_array[0];
    }

    newText.resize(newLen);
    newText.squeeze();
    //qDebug() << "removed" << text.size() - newText.size() << "characters";
    text = newText;
    unreachableCharacterCount = 0;
}

void QTextDocumentPrivate::setModified(bool m)
{
    Q_Q(QTextDocument);
    if (m == modified)
        return;

    modified = m;
    if (!modified)
        modifiedState = undoState;
    else
        modifiedState = -1;

    emit q->modificationChanged(modified);
}

bool QTextDocumentPrivate::ensureMaximumBlockCount()
{
    if (maximumBlockCount <= 0)
        return false;
    if (blocks.numNodes() <= maximumBlockCount)
        return false;

    beginEditBlock();

    const int blocksToRemove = blocks.numNodes() - maximumBlockCount;
    QTextCursor cursor(this, 0);
    cursor.movePosition(QTextCursor::NextBlock, QTextCursor::KeepAnchor, blocksToRemove);

    unreachableCharacterCount += cursor.selectionEnd() - cursor.selectionStart();

    // preserve the char format of the paragraph that is to become the new first one
    QTextCharFormat charFmt = cursor.blockCharFormat();
    cursor.removeSelectedText();
    cursor.setBlockCharFormat(charFmt);

    endEditBlock();

    compressPieceTable();

    return true;
}

/// This method is called from QTextTable when it is about to remove a table-cell to allow cursors to update their selection.
void QTextDocumentPrivate::aboutToRemoveCell(int from, int to)
{
    Q_ASSERT(from <= to);
    foreach (QTextCursorPrivate *curs, cursors)
        curs->aboutToRemoveCell(from, to);
}

QT_END_NAMESPACE
