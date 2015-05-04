/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtGui module of the Qt Toolkit.
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

#ifndef QTEXTDOCUMENTFRAGMENT_P_H
#define QTEXTDOCUMENTFRAGMENT_P_H

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

#include "QtGui/qtextdocument.h"
#include "private/qtexthtmlparser_p.h"
#include "private/qtextdocument_p.h"
#include "QtGui/qtexttable.h"
#include "QtCore/qatomic.h"
#include "QtCore/qlist.h"
#include "QtCore/qmap.h"
#include "QtCore/qpointer.h"
#include "QtCore/qvarlengtharray.h"
#include "QtCore/qdatastream.h"

QT_BEGIN_NAMESPACE

class QTextDocumentFragmentPrivate;

class QTextCopyHelper
{
public:
    QTextCopyHelper(const QTextCursor &_source, const QTextCursor &_destination, bool forceCharFormat = false, const QTextCharFormat &fmt = QTextCharFormat());

    void copy();

private:
    void appendFragments(int pos, int endPos);
    int appendFragment(int pos, int endPos, int objectIndex = -1);
    int convertFormatIndex(const QTextFormat &oldFormat, int objectIndexToSet = -1);
    inline int convertFormatIndex(int oldFormatIndex, int objectIndexToSet = -1)
    { return convertFormatIndex(src->formatCollection()->format(oldFormatIndex), objectIndexToSet); }
    inline QTextFormat convertFormat(const QTextFormat &fmt)
    { return dst->formatCollection()->format(convertFormatIndex(fmt)); }

    int insertPos;

    bool forceCharFormat;
    int primaryCharFormatIndex;

    QTextCursor cursor;
    QTextDocumentPrivate *dst;
    QTextDocumentPrivate *src;
    QTextFormatCollection &formatCollection;
    const QString originalText;
    QMap<int, int> objectIndexMap;
};

class QTextDocumentFragmentPrivate
{
public:
    QTextDocumentFragmentPrivate(const QTextCursor &cursor = QTextCursor());
    inline ~QTextDocumentFragmentPrivate() { delete doc; }

    void insert(QTextCursor &cursor) const;

    QAtomicInt ref;
    QTextDocument *doc;

    uint importedFromPlainText : 1;
private:
    Q_DISABLE_COPY(QTextDocumentFragmentPrivate)
};

#ifndef QT_NO_TEXTHTMLPARSER

class QTextHtmlImporter : public QTextHtmlParser
{
    struct Table;
public:
    enum ImportMode {
        ImportToFragment,
        ImportToDocument
    };

    QTextHtmlImporter(QTextDocument *_doc, const QString &html,
                      ImportMode mode,
                      const QTextDocument *resourceProvider = 0);

    void import();

private:
    bool closeTag();

    Table scanTable(int tableNodeIdx);

    enum ProcessNodeResult { ContinueWithNextNode, ContinueWithCurrentNode, ContinueWithNextSibling };

    void appendBlock(const QTextBlockFormat &format, QTextCharFormat charFmt = QTextCharFormat());
    bool appendNodeText();

    ProcessNodeResult processBlockNode();
    ProcessNodeResult processSpecialNodes();

    struct List
    {
        inline List() : listNode(0) {}
        QTextListFormat format;
        int listNode;
        QPointer<QTextList> list;
    };
    QVector<List> lists;
    int indent;

    // insert a named anchor the next time we emit a char format,
    // either in a block or in regular text
    QStringList namedAnchors;

#ifdef Q_CC_SUN
    friend struct QTextHtmlImporter::Table;
#endif
    struct TableCellIterator
    {
        inline TableCellIterator(QTextTable *t = 0) : table(t), row(0), column(0) {}

        inline TableCellIterator &operator++() {
            if (atEnd())
                return *this;
            do {
                const QTextTableCell cell = table->cellAt(row, column);
                if (!cell.isValid())
                    break;
                column += cell.columnSpan();
                if (column >= table->columns()) {
                    column = 0;
                    ++row;
                }
            } while (row < table->rows() && table->cellAt(row, column).row() != row);

            return *this;
        }

        inline bool atEnd() const { return table == 0 || row >= table->rows(); }

        QTextTableCell cell() const { return table->cellAt(row, column); }

        QTextTable *table;
        int row;
        int column;
    };

    friend struct Table;
    struct Table
    {
        Table() : isTextFrame(false), rows(0), columns(0), currentRow(0), lastIndent(0) {}
        QPointer<QTextFrame> frame;
        bool isTextFrame;
        int rows;
        int columns;
        int currentRow; // ... for buggy html (see html_skipCell testcase)
        TableCellIterator currentCell;
        int lastIndent;
    };
    QVector<Table> tables;

    struct RowColSpanInfo
    {
        int row, col;
        int rowSpan, colSpan;
    };

    enum WhiteSpace
    {
        RemoveWhiteSpace,
        CollapseWhiteSpace,
        PreserveWhiteSpace
    };

    WhiteSpace compressNextWhitespace;

    QTextDocument *doc;
    QTextCursor cursor;
    QTextHtmlParserNode::WhiteSpaceMode wsm;
    ImportMode importMode;
    bool hasBlock;
    bool forceBlockMerging;
    bool blockTagClosed;
    int currentNodeIdx;
    const QTextHtmlParserNode *currentNode;
};

QT_END_NAMESPACE
#endif // QT_NO_TEXTHTMLPARSER

#endif // QTEXTDOCUMENTFRAGMENT_P_H
