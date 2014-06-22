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

#include "qsyntaxhighlighter.h"

#ifndef QT_NO_SYNTAXHIGHLIGHTER
#include <private/qobject_p.h>
#include <qtextdocument.h>
#include <private/qtextdocument_p.h>
#include <qtextlayout.h>
#include <qpointer.h>
#include <qtextobject.h>
#include <qtextcursor.h>
#include <qdebug.h>
#include <qtimer.h>

QT_BEGIN_NAMESPACE

class QSyntaxHighlighterPrivate : public QObjectPrivate
{
    Q_DECLARE_PUBLIC(QSyntaxHighlighter)
public:
    inline QSyntaxHighlighterPrivate()
        : rehighlightPending(false), inReformatBlocks(false)
    {}

    QPointer<QTextDocument> doc;

    void _q_reformatBlocks(int from, int charsRemoved, int charsAdded);
    void reformatBlocks(int from, int charsRemoved, int charsAdded);
    void reformatBlock(const QTextBlock &block);

    inline void rehighlight(QTextCursor &cursor, QTextCursor::MoveOperation operation) {
        inReformatBlocks = true;
        cursor.beginEditBlock();
        int from = cursor.position();
        cursor.movePosition(operation);
        reformatBlocks(from, 0, cursor.position() - from);
        cursor.endEditBlock();
        inReformatBlocks = false;
    }

    inline void _q_delayedRehighlight() {
        if (!rehighlightPending)
            return;
        rehighlightPending = false;
        q_func()->rehighlight();
    }

    void applyFormatChanges();
    QVector<QTextCharFormat> formatChanges;
    QTextBlock currentBlock;
    bool rehighlightPending;
    bool inReformatBlocks;
};

void QSyntaxHighlighterPrivate::applyFormatChanges()
{
    bool formatsChanged = false;

    QTextLayout *layout = currentBlock.layout();

    QList<QTextLayout::FormatRange> ranges = layout->additionalFormats();

    const int preeditAreaStart = layout->preeditAreaPosition();
    const int preeditAreaLength = layout->preeditAreaText().length();

    if (preeditAreaLength != 0) {
        QList<QTextLayout::FormatRange>::Iterator it = ranges.begin();
        while (it != ranges.end()) {
            if (it->start >= preeditAreaStart
                && it->start + it->length <= preeditAreaStart + preeditAreaLength) {
                ++it;
            } else {
                it = ranges.erase(it);
                formatsChanged = true;
            }
        }
    } else if (!ranges.isEmpty()) {
        ranges.clear();
        formatsChanged = true;
    }

    int i = 0;
    while (i < formatChanges.count()) {
        QTextLayout::FormatRange r;

        while (i < formatChanges.count() && formatChanges.at(i) == r.format)
            ++i;

        if (i == formatChanges.count())
            break;

        r.start = i;
        r.format = formatChanges.at(i);

        while (i < formatChanges.count() && formatChanges.at(i) == r.format)
            ++i;

        Q_ASSERT(i <= formatChanges.count());
        r.length = i - r.start;

        if (preeditAreaLength != 0) {
            if (r.start >= preeditAreaStart)
                r.start += preeditAreaLength;
            else if (r.start + r.length >= preeditAreaStart)
                r.length += preeditAreaLength;
        }

        ranges << r;
        formatsChanged = true;
    }

    if (formatsChanged) {
        layout->setAdditionalFormats(ranges);
        doc->markContentsDirty(currentBlock.position(), currentBlock.length());
    }
}

void QSyntaxHighlighterPrivate::_q_reformatBlocks(int from, int charsRemoved, int charsAdded)
{
    if (!inReformatBlocks)
        reformatBlocks(from, charsRemoved, charsAdded);
}

void QSyntaxHighlighterPrivate::reformatBlocks(int from, int charsRemoved, int charsAdded)
{
    rehighlightPending = false;

    QTextBlock block = doc->findBlock(from);
    if (!block.isValid())
        return;

    int endPosition;
    QTextBlock lastBlock = doc->findBlock(from + charsAdded + (charsRemoved > 0 ? 1 : 0));
    if (lastBlock.isValid())
        endPosition = lastBlock.position() + lastBlock.length();
    else
        endPosition = doc->docHandle()->length();

    bool forceHighlightOfNextBlock = false;

    while (block.isValid() && (block.position() < endPosition || forceHighlightOfNextBlock)) {
        const int stateBeforeHighlight = block.userState();

        reformatBlock(block);

        forceHighlightOfNextBlock = (block.userState() != stateBeforeHighlight);

        block = block.next();
    }

    formatChanges.clear();
}

void QSyntaxHighlighterPrivate::reformatBlock(const QTextBlock &block)
{
    Q_Q(QSyntaxHighlighter);

    Q_ASSERT_X(!currentBlock.isValid(), "QSyntaxHighlighter::reformatBlock()", "reFormatBlock() called recursively");

    currentBlock = block;

    formatChanges.fill(QTextCharFormat(), block.length() - 1);
    q->highlightBlock(block.text());
    applyFormatChanges();

    currentBlock = QTextBlock();
}

/*!
    \class QSyntaxHighlighter
    \reentrant
    \inmodule QtGui

    \brief The QSyntaxHighlighter class allows you to define syntax
    highlighting rules, and in addition you can use the class to query
    a document's current formatting or user data.

    \since 4.1

    \ingroup richtext-processing

    The QSyntaxHighlighter class is a base class for implementing
    QTextDocument syntax highlighters.  A syntax highligher automatically
    highlights parts of the text in a QTextDocument. Syntax highlighters are
    often used when the user is entering text in a specific format (for example source code)
    and help the user to read the text and identify syntax errors.

    To provide your own syntax highlighting, you must subclass
    QSyntaxHighlighter and reimplement highlightBlock().

    When you create an instance of your QSyntaxHighlighter subclass,
    pass it the QTextDocument that you want the syntax
    highlighting to be applied to. For example:

    \snippet code/src_gui_text_qsyntaxhighlighter.cpp 0

    After this your highlightBlock() function will be called
    automatically whenever necessary. Use your highlightBlock()
    function to apply formatting (e.g. setting the font and color) to
    the text that is passed to it. QSyntaxHighlighter provides the
    setFormat() function which applies a given QTextCharFormat on
    the current text block. For example:

    \snippet code/src_gui_text_qsyntaxhighlighter.cpp 1

    Some syntaxes can have constructs that span several text
    blocks. For example, a C++ syntax highlighter should be able to
    cope with \c{/}\c{*...*}\c{/} multiline comments. To deal with
    these cases it is necessary to know the end state of the previous
    text block (e.g. "in comment").

    Inside your highlightBlock() implementation you can query the end
    state of the previous text block using the previousBlockState()
    function. After parsing the block you can save the last state
    using setCurrentBlockState().

    The currentBlockState() and previousBlockState() functions return
    an int value. If no state is set, the returned value is -1. You
    can designate any other value to identify any given state using
    the setCurrentBlockState() function. Once the state is set the
    QTextBlock keeps that value until it is set set again or until the
    corresponding paragraph of text is deleted.

    For example, if you're writing a simple C++ syntax highlighter,
    you might designate 1 to signify "in comment":

    \snippet code/src_gui_text_qsyntaxhighlighter.cpp 2

    In the example above, we first set the current block state to
    0. Then, if the previous block ended within a comment, we higlight
    from the beginning of the current block (\c {startIndex =
    0}). Otherwise, we search for the given start expression. If the
    specified end expression cannot be found in the text block, we
    change the current block state by calling setCurrentBlockState(),
    and make sure that the rest of the block is higlighted.

    In addition you can query the current formatting and user data
    using the format() and currentBlockUserData() functions
    respectively. You can also attach user data to the current text
    block using the setCurrentBlockUserData() function.
    QTextBlockUserData can be used to store custom settings. In the
    case of syntax highlighting, it is in particular interesting as
    cache storage for information that you may figure out while
    parsing the paragraph's text. For an example, see the
    setCurrentBlockUserData() documentation.

    \sa QTextDocument, {Syntax Highlighter Example}
*/

/*!
    Constructs a QSyntaxHighlighter with the given \a parent.

    If the parent is a QTextEdit, it installs the syntaxhighlighter on the
    parents document. The specified QTextEdit also becomes the owner of
    the QSyntaxHighlighter.
*/
QSyntaxHighlighter::QSyntaxHighlighter(QObject *parent)
    : QObject(*new QSyntaxHighlighterPrivate, parent)
{
    if (parent->inherits("QTextEdit")) {
        QTextDocument *doc = parent->property("document").value<QTextDocument *>();
        if (doc)
            setDocument(doc);
    }
}

/*!
    Constructs a QSyntaxHighlighter and installs it on \a parent.
    The specified QTextDocument also becomes the owner of the
    QSyntaxHighlighter.
*/
QSyntaxHighlighter::QSyntaxHighlighter(QTextDocument *parent)
    : QObject(*new QSyntaxHighlighterPrivate, parent)
{
    setDocument(parent);
}

/*!
    Destructor. Uninstalls this syntax highlighter from the text document.
*/
QSyntaxHighlighter::~QSyntaxHighlighter()
{
    setDocument(0);
}

/*!
    Installs the syntax highlighter on the given QTextDocument \a doc.
    A QSyntaxHighlighter can only be used with one document at a time.
*/
void QSyntaxHighlighter::setDocument(QTextDocument *doc)
{
    Q_D(QSyntaxHighlighter);
    if (d->doc) {
        disconnect(d->doc, SIGNAL(contentsChange(int,int,int)),
                   this, SLOT(_q_reformatBlocks(int,int,int)));

        QTextCursor cursor(d->doc);
        cursor.beginEditBlock();
        for (QTextBlock blk = d->doc->begin(); blk.isValid(); blk = blk.next())
            blk.layout()->clearAdditionalFormats();
        cursor.endEditBlock();
    }
    d->doc = doc;
    if (d->doc) {
        connect(d->doc, SIGNAL(contentsChange(int,int,int)),
                this, SLOT(_q_reformatBlocks(int,int,int)));
        d->rehighlightPending = true;
        QTimer::singleShot(0, this, SLOT(_q_delayedRehighlight()));
    }
}

/*!
    Returns the QTextDocument on which this syntax highlighter is
    installed.
*/
QTextDocument *QSyntaxHighlighter::document() const
{
    Q_D(const QSyntaxHighlighter);
    return d->doc;
}

/*!
    \since 4.2

    Reapplies the highlighting to the whole document.

    \sa rehighlightBlock()
*/
void QSyntaxHighlighter::rehighlight()
{
    Q_D(QSyntaxHighlighter);
    if (!d->doc)
        return;

    QTextCursor cursor(d->doc);
    d->rehighlight(cursor, QTextCursor::End);
}

/*!
    \since 4.6

    Reapplies the highlighting to the given QTextBlock \a block.

    \sa rehighlight()
*/
void QSyntaxHighlighter::rehighlightBlock(const QTextBlock &block)
{
    Q_D(QSyntaxHighlighter);
    if (!d->doc || !block.isValid() || block.document() != d->doc)
        return;

    const bool rehighlightPending = d->rehighlightPending;

    QTextCursor cursor(block);
    d->rehighlight(cursor, QTextCursor::EndOfBlock);

    if (rehighlightPending)
        d->rehighlightPending = rehighlightPending;
}

/*!
    \fn void QSyntaxHighlighter::highlightBlock(const QString &text)

    Highlights the given text block. This function is called when
    necessary by the rich text engine, i.e. on text blocks which have
    changed.

    To provide your own syntax highlighting, you must subclass
    QSyntaxHighlighter and reimplement highlightBlock(). In your
    reimplementation you should parse the block's \a text and call
    setFormat() as often as necessary to apply any font and color
    changes that you require. For example:

    \snippet code/src_gui_text_qsyntaxhighlighter.cpp 3

    Some syntaxes can have constructs that span several text
    blocks. For example, a C++ syntax highlighter should be able to
    cope with \c{/}\c{*...*}\c{/} multiline comments. To deal with
    these cases it is necessary to know the end state of the previous
    text block (e.g. "in comment").

    Inside your highlightBlock() implementation you can query the end
    state of the previous text block using the previousBlockState()
    function. After parsing the block you can save the last state
    using setCurrentBlockState().

    The currentBlockState() and previousBlockState() functions return
    an int value. If no state is set, the returned value is -1. You
    can designate any other value to identify any given state using
    the setCurrentBlockState() function. Once the state is set the
    QTextBlock keeps that value until it is set set again or until the
    corresponding paragraph of text gets deleted.

    For example, if you're writing a simple C++ syntax highlighter,
    you might designate 1 to signify "in comment". For a text block
    that ended in the middle of a comment you'd set 1 using
    setCurrentBlockState, and for other paragraphs you'd set 0.
    In your parsing code if the return value of previousBlockState()
    is 1, you would highlight the text as a C++ comment until you
    reached the closing \c{*}\c{/}.

    \sa previousBlockState(), setFormat(), setCurrentBlockState()
*/

/*!
    This function is applied to the syntax highlighter's current text
    block (i.e. the text that is passed to the highlightBlock()
    function).

    The specified \a format is applied to the text from the \a start
    position for a length of \a count characters (if \a count is 0,
    nothing is done). The formatting properties set in \a format are
    merged at display time with the formatting information stored
    directly in the document, for example as previously set with
    QTextCursor's functions. Note that the document itself remains
    unmodified by the format set through this function.

    \sa format(), highlightBlock()
*/
void QSyntaxHighlighter::setFormat(int start, int count, const QTextCharFormat &format)
{
    Q_D(QSyntaxHighlighter);
    if (start < 0 || start >= d->formatChanges.count())
        return;

    const int end = qMin(start + count, d->formatChanges.count());
    for (int i = start; i < end; ++i)
        d->formatChanges[i] = format;
}

/*!
    \overload

    The specified \a color is applied to the current text block from
    the \a start position for a length of \a count characters.

    The other attributes of the current text block, e.g. the font and
    background color, are reset to default values.

    \sa format(), highlightBlock()
*/
void QSyntaxHighlighter::setFormat(int start, int count, const QColor &color)
{
    QTextCharFormat format;
    format.setForeground(color);
    setFormat(start, count, format);
}

/*!
    \overload

    The specified \a font is applied to the current text block from
    the \a start position for a length of \a count characters.

    The other attributes of the current text block, e.g. the font and
    background color, are reset to default values.

    \sa format(), highlightBlock()
*/
void QSyntaxHighlighter::setFormat(int start, int count, const QFont &font)
{
    QTextCharFormat format;
    format.setFont(font);
    setFormat(start, count, format);
}

/*!
    \fn QTextCharFormat QSyntaxHighlighter::format(int position) const

    Returns the format at \a position inside the syntax highlighter's
    current text block.
*/
QTextCharFormat QSyntaxHighlighter::format(int pos) const
{
    Q_D(const QSyntaxHighlighter);
    if (pos < 0 || pos >= d->formatChanges.count())
        return QTextCharFormat();
    return d->formatChanges.at(pos);
}

/*!
    Returns the end state of the text block previous to the
    syntax highlighter's current block. If no value was
    previously set, the returned value is -1.

    \sa highlightBlock(), setCurrentBlockState()
*/
int QSyntaxHighlighter::previousBlockState() const
{
    Q_D(const QSyntaxHighlighter);
    if (!d->currentBlock.isValid())
        return -1;

    const QTextBlock previous = d->currentBlock.previous();
    if (!previous.isValid())
        return -1;

    return previous.userState();
}

/*!
    Returns the state of the current text block. If no value is set,
    the returned value is -1.
*/
int QSyntaxHighlighter::currentBlockState() const
{
    Q_D(const QSyntaxHighlighter);
    if (!d->currentBlock.isValid())
        return -1;

    return d->currentBlock.userState();
}

/*!
    Sets the state of the current text block to \a newState.

    \sa highlightBlock()
*/
void QSyntaxHighlighter::setCurrentBlockState(int newState)
{
    Q_D(QSyntaxHighlighter);
    if (!d->currentBlock.isValid())
        return;

    d->currentBlock.setUserState(newState);
}

/*!
    Attaches the given \a data to the current text block.  The
    ownership is passed to the underlying text document, i.e. the
    provided QTextBlockUserData object will be deleted if the
    corresponding text block gets deleted.

    QTextBlockUserData can be used to store custom settings. In the
    case of syntax highlighting, it is in particular interesting as
    cache storage for information that you may figure out while
    parsing the paragraph's text.

    For example while parsing the text, you can keep track of
    parenthesis characters that you encounter ('{[(' and the like),
    and store their relative position and the actual QChar in a simple
    class derived from QTextBlockUserData:

    \snippet code/src_gui_text_qsyntaxhighlighter.cpp 4

    During cursor navigation in the associated editor, you can ask the
    current QTextBlock (retrieved using the QTextCursor::block()
    function) if it has a user data object set and cast it to your \c
    BlockData object. Then you can check if the current cursor
    position matches with a previously recorded parenthesis position,
    and, depending on the type of parenthesis (opening or closing),
    find the next opening or closing parenthesis on the same level.

    In this way you can do a visual parenthesis matching and highlight
    from the current cursor position to the matching parenthesis. That
    makes it easier to spot a missing parenthesis in your code and to
    find where a corresponding opening/closing parenthesis is when
    editing parenthesis intensive code.

    \sa QTextBlock::setUserData()
*/
void QSyntaxHighlighter::setCurrentBlockUserData(QTextBlockUserData *data)
{
    Q_D(QSyntaxHighlighter);
    if (!d->currentBlock.isValid())
        return;

    d->currentBlock.setUserData(data);
}

/*!
    Returns the QTextBlockUserData object previously attached to the
    current text block.

    \sa QTextBlock::userData(), setCurrentBlockUserData()
*/
QTextBlockUserData *QSyntaxHighlighter::currentBlockUserData() const
{
    Q_D(const QSyntaxHighlighter);
    if (!d->currentBlock.isValid())
        return 0;

    return d->currentBlock.userData();
}

/*!
    \since 4.4

    Returns the current text block.
*/
QTextBlock QSyntaxHighlighter::currentBlock() const
{
    Q_D(const QSyntaxHighlighter);
    return d->currentBlock;
}

QT_END_NAMESPACE

#include "moc_qsyntaxhighlighter.cpp"

#endif // QT_NO_SYNTAXHIGHLIGHTER
