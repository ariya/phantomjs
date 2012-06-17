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

#include "qtextdocument.h"
#include <qtextformat.h>
#include "qtextdocumentlayout_p.h"
#include "qtextdocumentfragment.h"
#include "qtextdocumentfragment_p.h"
#include "qtexttable.h"
#include "qtextlist.h"
#include <qdebug.h>
#include <qregexp.h>
#include <qvarlengtharray.h>
#include <qtextcodec.h>
#include <qthread.h>
#include "qtexthtmlparser_p.h"
#include "qpainter.h"
#include "qprinter.h"
#include "qtextedit.h"
#include <qfile.h>
#include <qfileinfo.h>
#include <qdir.h>
#include <qapplication.h>
#include "qtextcontrol_p.h"
#include "qfont_p.h"
#include "private/qtextedit_p.h"
#include "private/qdataurl_p.h"

#include "qtextdocument_p.h"
#include <private/qprinter_p.h>
#include <private/qabstracttextdocumentlayout_p.h>

#include <limits.h>

QT_BEGIN_NAMESPACE

Q_CORE_EXPORT unsigned int qt_int_sqrt(unsigned int n);

/*!
    Returns true if the string \a text is likely to be rich text;
    otherwise returns false.

    This function uses a fast and therefore simple heuristic. It
    mainly checks whether there is something that looks like a tag
    before the first line break. Although the result may be correct
    for common cases, there is no guarantee.

    This function is defined in the \c <QTextDocument> header file.
*/
bool Qt::mightBeRichText(const QString& text)
{
    if (text.isEmpty())
        return false;
    int start = 0;

    while (start < text.length() && text.at(start).isSpace())
        ++start;

    // skip a leading <?xml ... ?> as for example with xhtml
    if (text.mid(start, 5) == QLatin1String("<?xml")) {
        while (start < text.length()) {
            if (text.at(start) == QLatin1Char('?')
                && start + 2 < text.length()
                && text.at(start + 1) == QLatin1Char('>')) {
                start += 2;
                break;
            }
            ++start;
        }

        while (start < text.length() && text.at(start).isSpace())
            ++start;
    }

    if (text.mid(start, 5).toLower() == QLatin1String("<!doc"))
        return true;
    int open = start;
    while (open < text.length() && text.at(open) != QLatin1Char('<')
            && text.at(open) != QLatin1Char('\n')) {
        if (text.at(open) == QLatin1Char('&') &&  text.mid(open+1,3) == QLatin1String("lt;"))
            return true; // support desperate attempt of user to see <...>
        ++open;
    }
    if (open < text.length() && text.at(open) == QLatin1Char('<')) {
        const int close = text.indexOf(QLatin1Char('>'), open);
        if (close > -1) {
            QString tag;
            for (int i = open+1; i < close; ++i) {
                if (text[i].isDigit() || text[i].isLetter())
                    tag += text[i];
                else if (!tag.isEmpty() && text[i].isSpace())
                    break;
                else if (!tag.isEmpty() && text[i] == QLatin1Char('/') && i + 1 == close)
                    break;
                else if (!text[i].isSpace() && (!tag.isEmpty() || text[i] != QLatin1Char('!')))
                    return false; // that's not a tag
            }
#ifndef QT_NO_TEXTHTMLPARSER
            return QTextHtmlParser::lookupElement(tag.toLower()) != -1;
#else
            return false;
#endif // QT_NO_TEXTHTMLPARSER
        }
    }
    return false;
}

/*!
    Converts the plain text string \a plain to a HTML string with
    HTML metacharacters \c{<}, \c{>}, \c{&}, and \c{"} replaced by HTML
    entities.

    Example:

    \snippet doc/src/snippets/code/src_gui_text_qtextdocument.cpp 0

    This function is defined in the \c <QTextDocument> header file.

    \sa convertFromPlainText(), mightBeRichText()
*/
QString Qt::escape(const QString& plain)
{
    QString rich;
    rich.reserve(int(plain.length() * qreal(1.1)));
    for (int i = 0; i < plain.length(); ++i) {
        if (plain.at(i) == QLatin1Char('<'))
            rich += QLatin1String("&lt;");
        else if (plain.at(i) == QLatin1Char('>'))
            rich += QLatin1String("&gt;");
        else if (plain.at(i) == QLatin1Char('&'))
            rich += QLatin1String("&amp;");
        else if (plain.at(i) == QLatin1Char('"'))
            rich += QLatin1String("&quot;");
        else
            rich += plain.at(i);
    }
    return rich;
}

/*!
    \fn QString Qt::convertFromPlainText(const QString &plain, WhiteSpaceMode mode)

    Converts the plain text string \a plain to an HTML-formatted
    paragraph while preserving most of its look.

    \a mode defines how whitespace is handled.

    This function is defined in the \c <QTextDocument> header file.

    \sa escape(), mightBeRichText()
*/
QString Qt::convertFromPlainText(const QString &plain, Qt::WhiteSpaceMode mode)
{
    int col = 0;
    QString rich;
    rich += QLatin1String("<p>");
    for (int i = 0; i < plain.length(); ++i) {
        if (plain[i] == QLatin1Char('\n')){
            int c = 1;
            while (i+1 < plain.length() && plain[i+1] == QLatin1Char('\n')) {
                i++;
                c++;
            }
            if (c == 1)
                rich += QLatin1String("<br>\n");
            else {
                rich += QLatin1String("</p>\n");
                while (--c > 1)
                    rich += QLatin1String("<br>\n");
                rich += QLatin1String("<p>");
            }
            col = 0;
        } else {
            if (mode == Qt::WhiteSpacePre && plain[i] == QLatin1Char('\t')){
                rich += QChar(0x00a0U);
                ++col;
                while (col % 8) {
                    rich += QChar(0x00a0U);
                    ++col;
                }
            }
            else if (mode == Qt::WhiteSpacePre && plain[i].isSpace())
                rich += QChar(0x00a0U);
            else if (plain[i] == QLatin1Char('<'))
                rich += QLatin1String("&lt;");
            else if (plain[i] == QLatin1Char('>'))
                rich += QLatin1String("&gt;");
            else if (plain[i] == QLatin1Char('&'))
                rich += QLatin1String("&amp;");
            else
                rich += plain[i];
            ++col;
        }
    }
    if (col != 0)
        rich += QLatin1String("</p>");
    return rich;
}

#ifndef QT_NO_TEXTCODEC
/*!
    \internal

    This function is defined in the \c <QTextDocument> header file.
*/
QTextCodec *Qt::codecForHtml(const QByteArray &ba)
{
    return QTextCodec::codecForHtml(ba);
}
#endif

/*!
    \class QTextDocument
    \reentrant

    \brief The QTextDocument class holds formatted text that can be
    viewed and edited using a QTextEdit.

    \ingroup richtext-processing


    QTextDocument is a container for structured rich text documents, providing
    support for styled text and various types of document elements, such as
    lists, tables, frames, and images.
    They can be created for use in a QTextEdit, or used independently.

    Each document element is described by an associated format object. Each
    format object is treated as a unique object by QTextDocuments, and can be
    passed to objectForFormat() to obtain the document element that it is
    applied to.

    A QTextDocument can be edited programmatically using a QTextCursor, and
    its contents can be examined by traversing the document structure. The
    entire document structure is stored as a hierarchy of document elements
    beneath the root frame, found with the rootFrame() function. Alternatively,
    if you just want to iterate over the textual contents of the document you
    can use begin(), end(), and findBlock() to retrieve text blocks that you
    can examine and iterate over.

    The layout of a document is determined by the documentLayout();
    you can create your own QAbstractTextDocumentLayout subclass and
    set it using setDocumentLayout() if you want to use your own
    layout logic. The document's title and other meta-information can be
    obtained by calling the metaInformation() function. For documents that
    are exposed to users through the QTextEdit class, the document title
    is also available via the QTextEdit::documentTitle() function.

    The toPlainText() and toHtml() convenience functions allow you to retrieve the
    contents of the document as plain text and HTML.
    The document's text can be searched using the find() functions.

    Undo/redo of operations performed on the document can be controlled using
    the setUndoRedoEnabled() function. The undo/redo system can be controlled
    by an editor widget through the undo() and redo() slots; the document also
    provides contentsChanged(), undoAvailable(), and redoAvailable() signals
    that inform connected editor widgets about the state of the undo/redo
    system. The following are the undo/redo operations of a QTextDocument:

    \list
        \o Insertion or removal of characters. A sequence of insertions or removals
           within the same text block are regarded as a single undo/redo operation.
        \o Insertion or removal of text blocks. Sequences of insertion or removals
           in a single operation (e.g., by selecting and then deleting text) are
           regarded as a single undo/redo operation.
        \o Text character format changes.
        \o Text block format changes.
        \o Text block group format changes.
    \endlist

    \sa QTextCursor, QTextEdit, \link richtext.html Rich Text Processing\endlink , {Text Object Example}
*/

/*!
    \property QTextDocument::defaultFont
    \brief the default font used to display the document's text
*/

/*!
    \property QTextDocument::defaultTextOption
    \brief the default text option will be set on all \l{QTextLayout}s in the document.

    When \l{QTextBlock}s are created, the defaultTextOption is set on their
    QTextLayout. This allows setting global properties for the document such as the
    default word wrap mode.
 */

/*!
    Constructs an empty QTextDocument with the given \a parent.
*/
QTextDocument::QTextDocument(QObject *parent)
    : QObject(*new QTextDocumentPrivate, parent)
{
    Q_D(QTextDocument);
    d->init();
}

/*!
    Constructs a QTextDocument containing the plain (unformatted) \a text
    specified, and with the given \a parent.
*/
QTextDocument::QTextDocument(const QString &text, QObject *parent)
    : QObject(*new QTextDocumentPrivate, parent)
{
    Q_D(QTextDocument);
    d->init();
    QTextCursor(this).insertText(text);
}

/*!
    \internal
*/
QTextDocument::QTextDocument(QTextDocumentPrivate &dd, QObject *parent)
    : QObject(dd, parent)
{
    Q_D(QTextDocument);
    d->init();
}

/*!
    Destroys the document.
*/
QTextDocument::~QTextDocument()
{
}


/*!
  Creates a new QTextDocument that is a copy of this text document. \a
  parent is the parent of the returned text document.
*/
QTextDocument *QTextDocument::clone(QObject *parent) const
{
    Q_D(const QTextDocument);
    QTextDocument *doc = new QTextDocument(parent);
    QTextCursor(doc).insertFragment(QTextDocumentFragment(this));
    doc->rootFrame()->setFrameFormat(rootFrame()->frameFormat());
    QTextDocumentPrivate *priv = doc->d_func();
    priv->title = d->title;
    priv->url = d->url;
    priv->pageSize = d->pageSize;
    priv->indentWidth = d->indentWidth;
    priv->defaultTextOption = d->defaultTextOption;
    priv->setDefaultFont(d->defaultFont());
    priv->resources = d->resources;
    priv->cachedResources.clear();
#ifndef QT_NO_CSSPARSER
    priv->defaultStyleSheet = d->defaultStyleSheet;
    priv->parsedDefaultStyleSheet = d->parsedDefaultStyleSheet;
#endif
    return doc;
}

/*!
    Returns true if the document is empty; otherwise returns false.
*/
bool QTextDocument::isEmpty() const
{
    Q_D(const QTextDocument);
    /* because if we're empty we still have one single paragraph as
     * one single fragment */
    return d->length() <= 1;
}

/*!
  Clears the document.
*/
void QTextDocument::clear()
{
    Q_D(QTextDocument);
    d->clear();
    d->resources.clear();
}

/*!
    \since 4.2

    Undoes the last editing operation on the document if undo is
    available. The provided \a cursor is positioned at the end of the
    location where the edition operation was undone.

    See the \l {Overview of Qt's Undo Framework}{Qt Undo Framework}
    documentation for details.

    \sa undoAvailable(), isUndoRedoEnabled()
*/
void QTextDocument::undo(QTextCursor *cursor)
{
    Q_D(QTextDocument);
    const int pos = d->undoRedo(true);
    if (cursor && pos >= 0) {
        *cursor = QTextCursor(this);
        cursor->setPosition(pos);
    }
}

/*!
    \since 4.2
    Redoes the last editing operation on the document if \link
    QTextDocument::isRedoAvailable() redo is available\endlink.

    The provided \a cursor is positioned at the end of the location where
    the edition operation was redone.
*/
void QTextDocument::redo(QTextCursor *cursor)
{
    Q_D(QTextDocument);
    const int pos = d->undoRedo(false);
    if (cursor && pos >= 0) {
        *cursor = QTextCursor(this);
        cursor->setPosition(pos);
    }
}

/*! \enum QTextDocument::Stacks
  
  \value UndoStack              The undo stack.
  \value RedoStack              The redo stack.
  \value UndoAndRedoStacks      Both the undo and redo stacks.
*/
        
/*!
    \since 4.7
    Clears the stacks specified by \a stacksToClear.

    This method clears any commands on the undo stack, the redo stack,
    or both (the default). If commands are cleared, the appropriate
    signals are emitted, QTextDocument::undoAvailable() or
    QTextDocument::redoAvailable().

    \sa QTextDocument::undoAvailable() QTextDocument::redoAvailable()
*/
void QTextDocument::clearUndoRedoStacks(Stacks stacksToClear)
{
    Q_D(QTextDocument);
    d->clearUndoRedoStacks(stacksToClear, true);
}

/*!
    \overload

*/
void QTextDocument::undo()
{
    Q_D(QTextDocument);
    d->undoRedo(true);
}

/*!
    \overload
    Redoes the last editing operation on the document if \link
    QTextDocument::isRedoAvailable() redo is available\endlink.
*/
void QTextDocument::redo()
{
    Q_D(QTextDocument);
    d->undoRedo(false);
}

/*!
    \internal

    Appends a custom undo \a item to the undo stack.
*/
void QTextDocument::appendUndoItem(QAbstractUndoItem *item)
{
    Q_D(QTextDocument);
    d->appendUndoItem(item);
}

/*!
    \property QTextDocument::undoRedoEnabled
    \brief whether undo/redo are enabled for this document

    This defaults to true. If disabled, the undo stack is cleared and
    no items will be added to it.
*/
void QTextDocument::setUndoRedoEnabled(bool enable)
{
    Q_D(QTextDocument);
    d->enableUndoRedo(enable);
}

bool QTextDocument::isUndoRedoEnabled() const
{
    Q_D(const QTextDocument);
    return d->isUndoRedoEnabled();
}

/*!
    \property QTextDocument::maximumBlockCount
    \since 4.2
    \brief Specifies the limit for blocks in the document.

    Specifies the maximum number of blocks the document may have. If there are
    more blocks in the document that specified with this property blocks are removed
    from the beginning of the document.

    A negative or zero value specifies that the document may contain an unlimited
    amount of blocks.

    The default value is 0.

    Note that setting this property will apply the limit immediately to the document
    contents.

    Setting this property also disables the undo redo history.

    This property is undefined in documents with tables or frames.
*/
int QTextDocument::maximumBlockCount() const
{
    Q_D(const QTextDocument);
    return d->maximumBlockCount;
}

void QTextDocument::setMaximumBlockCount(int maximum)
{
    Q_D(QTextDocument);
    d->maximumBlockCount = maximum;
    d->ensureMaximumBlockCount();
    setUndoRedoEnabled(false);
}

/*!
    \since 4.3

    The default text option is used on all QTextLayout objects in the document.
    This allows setting global properties for the document such as the default
    word wrap mode.
*/
QTextOption QTextDocument::defaultTextOption() const
{
    Q_D(const QTextDocument);
    return d->defaultTextOption;
}

/*!
    \since 4.3

    Sets the default text option.
*/
void QTextDocument::setDefaultTextOption(const QTextOption &option)
{
    Q_D(QTextDocument);
    d->defaultTextOption = option;
    if (d->lout)
        d->lout->documentChanged(0, 0, d->length());
}

/*!
    \since 4.8

    The default cursor movement style is used by all QTextCursor objects
    created from the document. The default is Qt::LogicalMoveStyle.
*/
Qt::CursorMoveStyle QTextDocument::defaultCursorMoveStyle() const
{
    Q_D(const QTextDocument);
    return d->defaultCursorMoveStyle;
}

/*!
    \since 4.8

    Sets the default cursor movement style to the given \a style.
*/
void QTextDocument::setDefaultCursorMoveStyle(Qt::CursorMoveStyle style)
{
    Q_D(QTextDocument);
    d->defaultCursorMoveStyle = style;
}

/*!
    \fn void QTextDocument::markContentsDirty(int position, int length)

    Marks the contents specified by the given \a position and \a length
    as "dirty", informing the document that it needs to be laid out
    again.
*/
void QTextDocument::markContentsDirty(int from, int length)
{
    Q_D(QTextDocument);
    d->documentChange(from, length);
    if (!d->inContentsChange) {
        if (d->lout) {
            d->lout->documentChanged(d->docChangeFrom, d->docChangeOldLength, d->docChangeLength);
            d->docChangeFrom = -1;
        }
    }
}

/*!
    \property QTextDocument::useDesignMetrics
    \since 4.1
    \brief whether the document uses design metrics of fonts to improve the accuracy of text layout

    If this property is set to true, the layout will use design metrics.
    Otherwise, the metrics of the paint device as set on
    QAbstractTextDocumentLayout::setPaintDevice() will be used.

    Using design metrics makes a layout have a width that is no longer dependent on hinting
    and pixel-rounding. This means that WYSIWYG text layout becomes possible because the width
    scales much more linearly based on paintdevice metrics than it would otherwise.

    By default, this property is false.
*/

void QTextDocument::setUseDesignMetrics(bool b)
{
    Q_D(QTextDocument);
    if (b == d->defaultTextOption.useDesignMetrics())
        return;
    d->defaultTextOption.setUseDesignMetrics(b);
    if (d->lout)
        d->lout->documentChanged(0, 0, d->length());
}

bool QTextDocument::useDesignMetrics() const
{
    Q_D(const QTextDocument);
    return d->defaultTextOption.useDesignMetrics();
}

/*!
    \since 4.2

    Draws the content of the document with painter \a p, clipped to \a rect.
    If \a rect is a null rectangle (default) then the document is painted unclipped.
*/
void QTextDocument::drawContents(QPainter *p, const QRectF &rect)
{
    p->save();
    QAbstractTextDocumentLayout::PaintContext ctx;
    if (rect.isValid()) {
        p->setClipRect(rect);
        ctx.clip = rect;
    }
    documentLayout()->draw(p, ctx);
    p->restore();
}

/*!
    \property QTextDocument::textWidth
    \since 4.2

    The text width specifies the preferred width for text in the document. If
    the text (or content in general) is wider than the specified with it is broken
    into multiple lines and grows vertically. If the text cannot be broken into multiple
    lines to fit into the specified text width it will be larger and the size() and the
    idealWidth() property will reflect that.

    If the text width is set to -1 then the text will not be broken into multiple lines
    unless it is enforced through an explicit line break or a new paragraph.

    The default value is -1.

    Setting the text width will also set the page height to -1, causing the document to
    grow or shrink vertically in a continuous way. If you want the document layout to break
    the text into multiple pages then you have to set the pageSize property instead.

    \sa size(), idealWidth(), pageSize()
*/
void QTextDocument::setTextWidth(qreal width)
{
    Q_D(QTextDocument);
    QSizeF sz = d->pageSize;
    sz.setWidth(width);
    sz.setHeight(-1);
    setPageSize(sz);
}

qreal QTextDocument::textWidth() const
{
    Q_D(const QTextDocument);
    return d->pageSize.width();
}

/*!
    \since 4.2

    Returns the ideal width of the text document. The ideal width is the actually used width
    of the document without optional alignments taken into account. It is always <= size().width().

    \sa adjustSize(), textWidth
*/
qreal QTextDocument::idealWidth() const
{
    if (QTextDocumentLayout *lout = qobject_cast<QTextDocumentLayout *>(documentLayout()))
        return lout->idealWidth();
    return textWidth();
}

/*!
    \property QTextDocument::documentMargin
    \since 4.5

     The margin around the document. The default is 4.
*/
qreal QTextDocument::documentMargin() const
{
    Q_D(const QTextDocument);
    return d->documentMargin;
}

void QTextDocument::setDocumentMargin(qreal margin)
{
    Q_D(QTextDocument);
    if (d->documentMargin != margin) {
        d->documentMargin = margin;

        QTextFrame* root = rootFrame();
        QTextFrameFormat format = root->frameFormat();
        format.setMargin(margin);
        root->setFrameFormat(format);

        if (d->lout)
            d->lout->documentChanged(0, 0, d->length());
    }
}


/*!
    \property QTextDocument::indentWidth
    \since 4.4

    Returns the width used for text list and text block indenting.

    The indent properties of QTextListFormat and QTextBlockFormat specify
    multiples of this value. The default indent width is 40.
*/
qreal QTextDocument::indentWidth() const
{
    Q_D(const QTextDocument);
    return d->indentWidth;
}


/*!
    \since 4.4

    Sets the \a width used for text list and text block indenting.

    The indent properties of QTextListFormat and QTextBlockFormat specify
    multiples of this value. The default indent width is 40 .

    \sa indentWidth()
*/
void QTextDocument::setIndentWidth(qreal width)
{
    Q_D(QTextDocument);
    if (d->indentWidth != width) {
        d->indentWidth = width;
        if (d->lout)
            d->lout->documentChanged(0, 0, d->length());
    }
}




/*!
    \since 4.2

    Adjusts the document to a reasonable size.

    \sa idealWidth(), textWidth, size
*/
void QTextDocument::adjustSize()
{
    // Pull this private function in from qglobal.cpp
    QFont f = defaultFont();
    QFontMetrics fm(f);
    int mw =  fm.width(QLatin1Char('x')) * 80;
    int w = mw;
    setTextWidth(w);
    QSizeF size = documentLayout()->documentSize();
    if (size.width() != 0) {
        w = qt_int_sqrt((uint)(5 * size.height() * size.width() / 3));
        setTextWidth(qMin(w, mw));

        size = documentLayout()->documentSize();
        if (w*3 < 5*size.height()) {
            w = qt_int_sqrt((uint)(2 * size.height() * size.width()));
            setTextWidth(qMin(w, mw));
        }
    }
    setTextWidth(idealWidth());
}

/*!
    \property QTextDocument::size
    \since 4.2

    Returns the actual size of the document.
    This is equivalent to documentLayout()->documentSize();

    The size of the document can be changed either by setting
    a text width or setting an entire page size.

    Note that the width is always >= pageSize().width().

    By default, for a newly-created, empty document, this property contains
    a configuration-dependent size.

    \sa setTextWidth(), setPageSize(), idealWidth()
*/
QSizeF QTextDocument::size() const
{
    return documentLayout()->documentSize();
}

/*!
    \property QTextDocument::blockCount
    \since 4.2

    Returns the number of text blocks in the document.

    The value of this property is undefined in documents with tables or frames.

    By default, if defined, this property contains a value of 1.
    \sa lineCount(), characterCount()
*/
int QTextDocument::blockCount() const
{
    Q_D(const QTextDocument);
    return d->blockMap().numNodes();
}


/*!
  \since 4.5

  Returns the number of lines of this document (if the layout supports
  this). Otherwise, this is identical to the number of blocks.

  \sa blockCount(), characterCount()
 */
int QTextDocument::lineCount() const
{
    Q_D(const QTextDocument);
    return d->blockMap().length(2);
}

/*!
  \since 4.5

  Returns the number of characters of this document.

  \sa blockCount(), characterAt()
 */
int QTextDocument::characterCount() const
{
    Q_D(const QTextDocument);
    return d->length();
}

/*!
  \since 4.5

  Returns the character at position \a pos, or a null character if the
  position is out of range.

  \sa characterCount()
 */
QChar QTextDocument::characterAt(int pos) const
{
    Q_D(const QTextDocument);
    if (pos < 0 || pos >= d->length())
        return QChar();
    QTextDocumentPrivate::FragmentIterator fragIt = d->find(pos);
    const QTextFragmentData * const frag = fragIt.value();
    const int offsetInFragment = qMax(0, pos - fragIt.position());
    return d->text.at(frag->stringPosition + offsetInFragment);
}


/*!
    \property QTextDocument::defaultStyleSheet
    \since 4.2

    The default style sheet is applied to all newly HTML formatted text that is
    inserted into the document, for example using setHtml() or QTextCursor::insertHtml().

    The style sheet needs to be compliant to CSS 2.1 syntax.

    \bold{Note:} Changing the default style sheet does not have any effect to the existing content
    of the document.

    \sa {Supported HTML Subset}
*/

#ifndef QT_NO_CSSPARSER
void QTextDocument::setDefaultStyleSheet(const QString &sheet)
{
    Q_D(QTextDocument);
    d->defaultStyleSheet = sheet;
    QCss::Parser parser(sheet);
    d->parsedDefaultStyleSheet = QCss::StyleSheet();
    d->parsedDefaultStyleSheet.origin = QCss::StyleSheetOrigin_UserAgent;
    parser.parse(&d->parsedDefaultStyleSheet);
}

QString QTextDocument::defaultStyleSheet() const
{
    Q_D(const QTextDocument);
    return d->defaultStyleSheet;
}
#endif // QT_NO_CSSPARSER

/*!
    \fn void QTextDocument::contentsChanged()

    This signal is emitted whenever the document's content changes; for
    example, when text is inserted or deleted, or when formatting is applied.

    \sa contentsChange()
*/

/*!
    \fn void QTextDocument::contentsChange(int position, int charsRemoved, int charsAdded)

    This signal is emitted whenever the document's content changes; for
    example, when text is inserted or deleted, or when formatting is applied.

    Information is provided about the \a position of the character in the
    document where the change occurred, the number of characters removed
    (\a charsRemoved), and the number of characters added (\a charsAdded).

    The signal is emitted before the document's layout manager is notified
    about the change. This hook allows you to implement syntax highlighting
    for the document.

    \sa QAbstractTextDocumentLayout::documentChanged(), contentsChanged()
*/


/*!
    \fn QTextDocument::undoAvailable(bool available);

    This signal is emitted whenever undo operations become available
    (\a available is true) or unavailable (\a available is false).

    See the \l {Overview of Qt's Undo Framework}{Qt Undo Framework}
    documentation for details.

    \sa undo(), isUndoRedoEnabled()
*/

/*!
    \fn QTextDocument::redoAvailable(bool available);

    This signal is emitted whenever redo operations become available
    (\a available is true) or unavailable (\a available is false).
*/

/*!
    \fn QTextDocument::cursorPositionChanged(const QTextCursor &cursor);

    This signal is emitted whenever the position of a cursor changed
    due to an editing operation. The cursor that changed is passed in
    \a cursor.  If you need a signal when the cursor is moved with the
    arrow keys you can use the \l{QTextEdit::}{cursorPositionChanged()} signal in
    QTextEdit.
*/

/*!
    \fn QTextDocument::blockCountChanged(int newBlockCount);
    \since 4.3

    This signal is emitted when the total number of text blocks in the
    document changes. The value passed in \a newBlockCount is the new
    total.
*/

/*!
    \fn QTextDocument::documentLayoutChanged();
    \since 4.4

    This signal is emitted when a new document layout is set.

    \sa setDocumentLayout()

*/


/*!
    Returns true if undo is available; otherwise returns false.

    \sa isRedoAvailable(), availableUndoSteps()
*/
bool QTextDocument::isUndoAvailable() const
{
    Q_D(const QTextDocument);
    return d->isUndoAvailable();
}

/*!
    Returns true if redo is available; otherwise returns false.

    \sa isUndoAvailable(), availableRedoSteps()
*/
bool QTextDocument::isRedoAvailable() const
{
    Q_D(const QTextDocument);
    return d->isRedoAvailable();
}

/*! \since 4.6

    Returns the number of available undo steps.

    \sa isUndoAvailable()
*/
int QTextDocument::availableUndoSteps() const
{
    Q_D(const QTextDocument);
    return d->availableUndoSteps();
}

/*! \since 4.6

    Returns the number of available redo steps.

    \sa isRedoAvailable()
*/
int QTextDocument::availableRedoSteps() const
{
    Q_D(const QTextDocument);
    return d->availableRedoSteps();
}

/*! \since 4.4

    Returns the document's revision (if undo is enabled).

    The revision is guaranteed to increase when a document that is not
    modified is edited.

    \sa QTextBlock::revision(), isModified()
 */
int QTextDocument::revision() const
{
    Q_D(const QTextDocument);
    return d->revision;
}



/*!
    Sets the document to use the given \a layout. The previous layout
    is deleted.

    \sa documentLayoutChanged()
*/
void QTextDocument::setDocumentLayout(QAbstractTextDocumentLayout *layout)
{
    Q_D(QTextDocument);
    d->setLayout(layout);
}

/*!
    Returns the document layout for this document.
*/
QAbstractTextDocumentLayout *QTextDocument::documentLayout() const
{
    Q_D(const QTextDocument);
    if (!d->lout) {
        QTextDocument *that = const_cast<QTextDocument *>(this);
        that->d_func()->setLayout(new QTextDocumentLayout(that));
    }
    return d->lout;
}


/*!
    Returns meta information about the document of the type specified by
    \a info.

    \sa setMetaInformation()
*/
QString QTextDocument::metaInformation(MetaInformation info) const
{
    Q_D(const QTextDocument);
    switch (info) {
    case DocumentTitle:
        return d->title;
    case DocumentUrl:
        return d->url;
    }
    return QString();
}

/*!
    Sets the document's meta information of the type specified by \a info
    to the given \a string.

    \sa metaInformation()
*/
void QTextDocument::setMetaInformation(MetaInformation info, const QString &string)
{
    Q_D(QTextDocument);
    switch (info) {
    case DocumentTitle:
        d->title = string;
        break;
    case DocumentUrl:
        d->url = string;
        break;
    }
}

/*!
    Returns the plain text contained in the document. If you want
    formatting information use a QTextCursor instead.

    \sa toHtml()
*/
QString QTextDocument::toPlainText() const
{
    Q_D(const QTextDocument);
    QString txt = d->plainText();

    QChar *uc = txt.data();
    QChar *e = uc + txt.size();

    for (; uc != e; ++uc) {
        switch (uc->unicode()) {
        case 0xfdd0: // QTextBeginningOfFrame
        case 0xfdd1: // QTextEndOfFrame
        case QChar::ParagraphSeparator:
        case QChar::LineSeparator:
            *uc = QLatin1Char('\n');
            break;
        case QChar::Nbsp:
            *uc = QLatin1Char(' ');
            break;
        default:
            ;
        }
    }
    return txt;
}

/*!
    Replaces the entire contents of the document with the given plain
    \a text.

    \sa setHtml()
*/
void QTextDocument::setPlainText(const QString &text)
{
    Q_D(QTextDocument);
    bool previousState = d->isUndoRedoEnabled();
    d->enableUndoRedo(false);
    d->beginEditBlock();
    d->clear();
    QTextCursor(this).insertText(text);
    d->endEditBlock();
    d->enableUndoRedo(previousState);
}

/*!
    Replaces the entire contents of the document with the given
    HTML-formatted text in the \a html string.

    The HTML formatting is respected as much as possible; for example,
    "<b>bold</b> text" will produce text where the first word has a font
    weight that gives it a bold appearance: "\bold{bold} text".

    \note It is the responsibility of the caller to make sure that the
    text is correctly decoded when a QString containing HTML is created
    and passed to setHtml().

    \sa setPlainText(), {Supported HTML Subset}
*/

#ifndef QT_NO_TEXTHTMLPARSER

void QTextDocument::setHtml(const QString &html)
{
    Q_D(QTextDocument);
    bool previousState = d->isUndoRedoEnabled();
    d->enableUndoRedo(false);
    d->beginEditBlock();
    d->clear();
    QTextHtmlImporter(this, html, QTextHtmlImporter::ImportToDocument).import();
    d->endEditBlock();
    d->enableUndoRedo(previousState);
}

#endif // QT_NO_TEXTHTMLPARSER

/*!
    \enum QTextDocument::FindFlag

    This enum describes the options available to QTextDocument's find function. The options
    can be OR-ed together from the following list:

    \value FindBackward Search backwards instead of forwards.
    \value FindCaseSensitively By default find works case insensitive. Specifying this option
    changes the behaviour to a case sensitive find operation.
    \value FindWholeWords Makes find match only complete words.
*/

/*!
    \enum QTextDocument::MetaInformation

    This enum describes the different types of meta information that can be
    added to a document.

    \value DocumentTitle    The title of the document.
    \value DocumentUrl      The url of the document. The loadResource() function uses
                            this url as the base when loading relative resources.

    \sa metaInformation(), setMetaInformation()
*/

/*!
    \fn QTextCursor QTextDocument::find(const QString &subString, int position, FindFlags options) const

    \overload

    Finds the next occurrence of the string, \a subString, in the document.
    The search starts at the given \a position, and proceeds forwards
    through the document unless specified otherwise in the search options.
    The \a options control the type of search performed.

    Returns a cursor with the match selected if \a subString
    was found; otherwise returns a null cursor.

    If the \a position is 0 (the default) the search begins from the beginning
    of the document; otherwise it begins at the specified position.
*/
QTextCursor QTextDocument::find(const QString &subString, int from, FindFlags options) const
{
    QRegExp expr(subString);
    expr.setPatternSyntax(QRegExp::FixedString);
    expr.setCaseSensitivity((options & QTextDocument::FindCaseSensitively) ? Qt::CaseSensitive : Qt::CaseInsensitive);

    return find(expr, from, options);
}

/*!
    \fn QTextCursor QTextDocument::find(const QString &subString, const QTextCursor &cursor, FindFlags options) const

    Finds the next occurrence of the string, \a subString, in the document.
    The search starts at the position of the given \a cursor, and proceeds
    forwards through the document unless specified otherwise in the search
    options. The \a options control the type of search performed.

    Returns a cursor with the match selected if \a subString was found; otherwise
    returns a null cursor.

    If the given \a cursor has a selection, the search begins after the
    selection; otherwise it begins at the cursor's position.

    By default the search is case-sensitive, and can match text anywhere in the
    document.
*/
QTextCursor QTextDocument::find(const QString &subString, const QTextCursor &from, FindFlags options) const
{
    int pos = 0;
    if (!from.isNull()) {
        if (options & QTextDocument::FindBackward)
            pos = from.selectionStart();
        else
            pos = from.selectionEnd();
    }
    QRegExp expr(subString);
    expr.setPatternSyntax(QRegExp::FixedString);
    expr.setCaseSensitivity((options & QTextDocument::FindCaseSensitively) ? Qt::CaseSensitive : Qt::CaseInsensitive);

    return find(expr, pos, options);
}


static bool findInBlock(const QTextBlock &block, const QRegExp &expression, int offset,
                        QTextDocument::FindFlags options, QTextCursor &cursor)
{
    const QRegExp expr(expression);
    QString text = block.text();
    text.replace(QChar::Nbsp, QLatin1Char(' '));

    int idx = -1;
    while (offset >=0 && offset <= text.length()) {
        idx = (options & QTextDocument::FindBackward) ?
               expr.lastIndexIn(text, offset) : expr.indexIn(text, offset);
        if (idx == -1)
            return false;

        if (options & QTextDocument::FindWholeWords) {
            const int start = idx;
            const int end = start + expr.matchedLength();
            if ((start != 0 && text.at(start - 1).isLetterOrNumber())
                || (end != text.length() && text.at(end).isLetterOrNumber())) {
                //if this is not a whole word, continue the search in the string
                offset = (options & QTextDocument::FindBackward) ? idx-1 : end+1;
                idx = -1;
                continue;
            }
        }
        //we have a hit, return the cursor for that.
        break;
    }
    if (idx == -1)
        return false;
    cursor = QTextCursor(block.docHandle(), block.position() + idx);
    cursor.setPosition(cursor.position() + expr.matchedLength(), QTextCursor::KeepAnchor);
    return true;
}

/*!
    \fn QTextCursor QTextDocument::find(const QRegExp & expr, int position, FindFlags options) const

    \overload

    Finds the next occurrence, matching the regular expression, \a expr, in the document.
    The search starts at the given \a position, and proceeds forwards
    through the document unless specified otherwise in the search options.
    The \a options control the type of search performed. The FindCaseSensitively
    option is ignored for this overload, use QRegExp::caseSensitivity instead.

    Returns a cursor with the match selected if a match was found; otherwise
    returns a null cursor.

    If the \a position is 0 (the default) the search begins from the beginning
    of the document; otherwise it begins at the specified position.
*/
QTextCursor QTextDocument::find(const QRegExp & expr, int from, FindFlags options) const
{
    Q_D(const QTextDocument);

    if (expr.isEmpty())
        return QTextCursor();

    int pos = from;
    //the cursor is positioned between characters, so for a backward search
    //do not include the character given in the position.
    if (options & FindBackward) {
        --pos ;
        if(pos < 0)
            return QTextCursor();
    }

    QTextCursor cursor;
    QTextBlock block = d->blocksFind(pos);

    if (!(options & FindBackward)) {
       int blockOffset = qMax(0, pos - block.position());
        while (block.isValid()) {
            if (findInBlock(block, expr, blockOffset, options, cursor))
                return cursor;
            blockOffset = 0;
            block = block.next();
        }
    } else {
        int blockOffset = pos - block.position();
        while (block.isValid()) {
            if (findInBlock(block, expr, blockOffset, options, cursor))
                return cursor;
            block = block.previous();
            blockOffset = block.length() - 1;
        }
    }

    return QTextCursor();
}

/*!
    \fn QTextCursor QTextDocument::find(const QRegExp &expr, const QTextCursor &cursor, FindFlags options) const

    Finds the next occurrence, matching the regular expression, \a expr, in the document.
    The search starts at the position of the given \a cursor, and proceeds
    forwards through the document unless specified otherwise in the search
    options. The \a options control the type of search performed. The FindCaseSensitively
    option is ignored for this overload, use QRegExp::caseSensitivity instead.

    Returns a cursor with the match selected if a match was found; otherwise
    returns a null cursor.

    If the given \a cursor has a selection, the search begins after the
    selection; otherwise it begins at the cursor's position.

    By default the search is case-sensitive, and can match text anywhere in the
    document.
*/
QTextCursor QTextDocument::find(const QRegExp &expr, const QTextCursor &from, FindFlags options) const
{
    int pos = 0;
    if (!from.isNull()) {
        if (options & QTextDocument::FindBackward)
            pos = from.selectionStart();
        else
            pos = from.selectionEnd();
    }
    return find(expr, pos, options);
}


/*!
    \fn QTextObject *QTextDocument::createObject(const QTextFormat &format)

    Creates and returns a new document object (a QTextObject), based
    on the given \a format.

    QTextObjects will always get created through this method, so you
    must reimplement it if you use custom text objects inside your document.
*/
QTextObject *QTextDocument::createObject(const QTextFormat &f)
{
    QTextObject *obj = 0;
    if (f.isListFormat())
        obj = new QTextList(this);
    else if (f.isTableFormat())
        obj = new QTextTable(this);
    else if (f.isFrameFormat())
        obj = new QTextFrame(this);

    return obj;
}

/*!
    \internal

    Returns the frame that contains the text cursor position \a pos.
*/
QTextFrame *QTextDocument::frameAt(int pos) const
{
    Q_D(const QTextDocument);
    return d->frameAt(pos);
}

/*!
    Returns the document's root frame.
*/
QTextFrame *QTextDocument::rootFrame() const
{
    Q_D(const QTextDocument);
    return d->rootFrame();
}

/*!
    Returns the text object associated with the given \a objectIndex.
*/
QTextObject *QTextDocument::object(int objectIndex) const
{
    Q_D(const QTextDocument);
    return d->objectForIndex(objectIndex);
}

/*!
    Returns the text object associated with the format \a f.
*/
QTextObject *QTextDocument::objectForFormat(const QTextFormat &f) const
{
    Q_D(const QTextDocument);
    return d->objectForFormat(f);
}


/*!
    Returns the text block that contains the \a{pos}-th character.
*/
QTextBlock QTextDocument::findBlock(int pos) const
{
    Q_D(const QTextDocument);
    return QTextBlock(docHandle(), d->blockMap().findNode(pos));
}

/*!
    \since 4.4
    Returns the text block with the specified \a blockNumber.

    \sa QTextBlock::blockNumber()
*/
QTextBlock QTextDocument::findBlockByNumber(int blockNumber) const
{
    Q_D(const QTextDocument);
    return QTextBlock(docHandle(), d->blockMap().findNode(blockNumber, 1));
}

/*!
    \since 4.5
    Returns the text block that contains the specified \a lineNumber.

    \sa QTextBlock::firstLineNumber()
*/
QTextBlock QTextDocument::findBlockByLineNumber(int lineNumber) const
{
    Q_D(const QTextDocument);
    return QTextBlock(docHandle(), d->blockMap().findNode(lineNumber, 2));
}

/*!
    Returns the document's first text block.

    \sa firstBlock()
*/
QTextBlock QTextDocument::begin() const
{
    Q_D(const QTextDocument);
    return QTextBlock(docHandle(), d->blockMap().begin().n);
}

/*!
    This function returns a block to test for the end of the document
    while iterating over it.

    \snippet doc/src/snippets/textdocumentendsnippet.cpp 0

    The block returned is invalid and represents the block after the
    last block in the document. You can use lastBlock() to retrieve the
    last valid block of the document.

    \sa lastBlock()
*/
QTextBlock QTextDocument::end() const
{
    return QTextBlock(docHandle(), 0);
}

/*!
    \since 4.4
    Returns the document's first text block.
*/
QTextBlock QTextDocument::firstBlock() const
{
    Q_D(const QTextDocument);
    return QTextBlock(docHandle(), d->blockMap().begin().n);
}

/*!
    \since 4.4
    Returns the document's last (valid) text block.
*/
QTextBlock QTextDocument::lastBlock() const
{
    Q_D(const QTextDocument);
    return QTextBlock(docHandle(), d->blockMap().last().n);
}

/*!
    \property QTextDocument::pageSize
    \brief the page size that should be used for laying out the document

    By default, for a newly-created, empty document, this property contains
    an undefined size.

    \sa modificationChanged()
*/

void QTextDocument::setPageSize(const QSizeF &size)
{
    Q_D(QTextDocument);
    d->pageSize = size;
    if (d->lout)
        d->lout->documentChanged(0, 0, d->length());
}

QSizeF QTextDocument::pageSize() const
{
    Q_D(const QTextDocument);
    return d->pageSize;
}

/*!
  returns the number of pages in this document.
*/
int QTextDocument::pageCount() const
{
    return documentLayout()->pageCount();
}

/*!
    Sets the default \a font to use in the document layout.
*/
void QTextDocument::setDefaultFont(const QFont &font)
{
    Q_D(QTextDocument);
    d->setDefaultFont(font);
    if (d->lout)
        d->lout->documentChanged(0, 0, d->length());
}

/*!
    Returns the default font to be used in the document layout.
*/
QFont QTextDocument::defaultFont() const
{
    Q_D(const QTextDocument);
    return d->defaultFont();
}

/*!
    \fn QTextDocument::modificationChanged(bool changed)

    This signal is emitted whenever the content of the document
    changes in a way that affects the modification state. If \a
    changed is true, the document has been modified; otherwise it is
    false.

    For example, calling setModified(false) on a document and then
    inserting text causes the signal to get emitted. If you undo that
    operation, causing the document to return to its original
    unmodified state, the signal will get emitted again.
*/

/*!
    \property QTextDocument::modified
    \brief whether the document has been modified by the user

    By default, this property is false.

    \sa modificationChanged()
*/

bool QTextDocument::isModified() const
{
    return docHandle()->isModified();
}

void QTextDocument::setModified(bool m)
{
    docHandle()->setModified(m);
}

#ifndef QT_NO_PRINTER
static void printPage(int index, QPainter *painter, const QTextDocument *doc, const QRectF &body, const QPointF &pageNumberPos)
{
    painter->save();
    painter->translate(body.left(), body.top() - (index - 1) * body.height());
    QRectF view(0, (index - 1) * body.height(), body.width(), body.height());

    QAbstractTextDocumentLayout *layout = doc->documentLayout();
    QAbstractTextDocumentLayout::PaintContext ctx;

    painter->setClipRect(view);
    ctx.clip = view;

    // don't use the system palette text as default text color, on HP/UX
    // for example that's white, and white text on white paper doesn't
    // look that nice
    ctx.palette.setColor(QPalette::Text, Qt::black);

    layout->draw(painter, ctx);

    if (!pageNumberPos.isNull()) {
        painter->setClipping(false);
        painter->setFont(QFont(doc->defaultFont()));
        const QString pageString = QString::number(index);

        painter->drawText(qRound(pageNumberPos.x() - painter->fontMetrics().width(pageString)),
                          qRound(pageNumberPos.y() + view.top()),
                          pageString);
    }

    painter->restore();
}

/*!
    Prints the document to the given \a printer. The QPrinter must be
    set up before being used with this function.

    This is only a convenience method to print the whole document to the printer.

    If the document is already paginated through a specified height in the pageSize()
    property it is printed as-is.

    If the document is not paginated, like for example a document used in a QTextEdit,
    then a temporary copy of the document is created and the copy is broken into
    multiple pages according to the size of the QPrinter's paperRect(). By default
    a 2 cm margin is set around the document contents. In addition the current page
    number is printed at the bottom of each page.

    Note that QPrinter::Selection is not supported as print range with this function since
    the selection is a property of QTextCursor. If you have a QTextEdit associated with
    your QTextDocument then you can use QTextEdit's print() function because QTextEdit has
    access to the user's selection.

    \sa QTextEdit::print()
*/

void QTextDocument::print(QPrinter *printer) const
{
    Q_D(const QTextDocument);

    if (!printer || !printer->isValid())
        return;

    if (!d->title.isEmpty())
        printer->setDocName(d->title);

    bool documentPaginated = d->pageSize.isValid() && !d->pageSize.isNull()
                             && d->pageSize.height() != INT_MAX;

    if (!documentPaginated && !printer->fullPage() && !printer->d_func()->hasCustomPageMargins)
        printer->setPageMargins(23.53, 23.53, 23.53, 23.53, QPrinter::Millimeter);

    QPainter p(printer);

    // Check that there is a valid device to print to.
    if (!p.isActive())
        return;

    const QTextDocument *doc = this;
    QScopedPointer<QTextDocument> clonedDoc;
    (void)doc->documentLayout(); // make sure that there is a layout

    QRectF body = QRectF(QPointF(0, 0), d->pageSize);
    QPointF pageNumberPos;

    if (documentPaginated) {
        qreal sourceDpiX = qt_defaultDpi();
        qreal sourceDpiY = sourceDpiX;

        QPaintDevice *dev = doc->documentLayout()->paintDevice();
        if (dev) {
            sourceDpiX = dev->logicalDpiX();
            sourceDpiY = dev->logicalDpiY();
        }

        const qreal dpiScaleX = qreal(printer->logicalDpiX()) / sourceDpiX;
        const qreal dpiScaleY = qreal(printer->logicalDpiY()) / sourceDpiY;

        // scale to dpi
        p.scale(dpiScaleX, dpiScaleY);

        QSizeF scaledPageSize = d->pageSize;
        scaledPageSize.rwidth() *= dpiScaleX;
        scaledPageSize.rheight() *= dpiScaleY;

        const QSizeF printerPageSize(printer->pageRect().size());

        // scale to page
        p.scale(printerPageSize.width() / scaledPageSize.width(),
                printerPageSize.height() / scaledPageSize.height());
    } else {
        doc = clone(const_cast<QTextDocument *>(this));
        clonedDoc.reset(const_cast<QTextDocument *>(doc));

        for (QTextBlock srcBlock = firstBlock(), dstBlock = clonedDoc->firstBlock();
             srcBlock.isValid() && dstBlock.isValid();
             srcBlock = srcBlock.next(), dstBlock = dstBlock.next()) {
            dstBlock.layout()->setAdditionalFormats(srcBlock.layout()->additionalFormats());
        }

        QAbstractTextDocumentLayout *layout = doc->documentLayout();
        layout->setPaintDevice(p.device());

        // copy the custom object handlers
        layout->d_func()->handlers = documentLayout()->d_func()->handlers;

        int dpiy = p.device()->logicalDpiY();
        int margin = 0;
        if (printer->fullPage() && !printer->d_func()->hasCustomPageMargins) {
            // for compatibility
            margin = (int) ((2/2.54)*dpiy); // 2 cm margins
            QTextFrameFormat fmt = doc->rootFrame()->frameFormat();
            fmt.setMargin(margin);
            doc->rootFrame()->setFrameFormat(fmt);
        }

        QRectF pageRect(printer->pageRect());
        body = QRectF(0, 0, pageRect.width(), pageRect.height());
        pageNumberPos = QPointF(body.width() - margin,
                                body.height() - margin
                                + QFontMetrics(doc->defaultFont(), p.device()).ascent()
                                + 5 * dpiy / 72.0);
        clonedDoc->setPageSize(body.size());
    }

    int docCopies;
    int pageCopies;
    if (printer->collateCopies() == true){
        docCopies = 1;
        pageCopies = printer->supportsMultipleCopies() ? 1 : printer->copyCount();
    } else {
        docCopies = printer->supportsMultipleCopies() ? 1 : printer->copyCount();
        pageCopies = 1;
    }

    int fromPage = printer->fromPage();
    int toPage = printer->toPage();
    bool ascending = true;

    if (fromPage == 0 && toPage == 0) {
        fromPage = 1;
        toPage = doc->pageCount();
    }
    // paranoia check
    fromPage = qMax(1, fromPage);
    toPage = qMin(doc->pageCount(), toPage);

    if (toPage < fromPage) {
        // if the user entered a page range outside the actual number
        // of printable pages, just return
        return;
    }

    if (printer->pageOrder() == QPrinter::LastPageFirst) {
        int tmp = fromPage;
        fromPage = toPage;
        toPage = tmp;
        ascending = false;
    }

    for (int i = 0; i < docCopies; ++i) {

        int page = fromPage;
        while (true) {
            for (int j = 0; j < pageCopies; ++j) {
                if (printer->printerState() == QPrinter::Aborted
                    || printer->printerState() == QPrinter::Error)
                    return;
                printPage(page, &p, doc, body, pageNumberPos);
                if (j < pageCopies - 1)
                    printer->newPage();
            }

            if (page == toPage)
                break;

            if (ascending)
                ++page;
            else
                --page;

            printer->newPage();
        }

        if ( i < docCopies - 1)
            printer->newPage();
    }
}
#endif

/*!
    \enum QTextDocument::ResourceType

    This enum describes the types of resources that can be loaded by
    QTextDocument's loadResource() function.

    \value HtmlResource  The resource contains HTML.
    \value ImageResource The resource contains image data.
                         Currently supported data types are QVariant::Pixmap and
                         QVariant::Image. If the corresponding variant is of type
                         QVariant::ByteArray then Qt attempts to load the image using
                         QImage::loadFromData. QVariant::Icon is currently not supported.
                         The icon needs to be converted to one of the supported types first,
                         for example using QIcon::pixmap.
    \value StyleSheetResource The resource contains CSS.
    \value UserResource  The first available value for user defined
                         resource types.

    \sa loadResource()
*/

/*!
    Returns data of the specified \a type from the resource with the
    given \a name.

    This function is called by the rich text engine to request data that isn't
    directly stored by QTextDocument, but still associated with it. For example,
    images are referenced indirectly by the name attribute of a QTextImageFormat
    object.

    Resources are cached internally in the document. If a resource can
    not be found in the cache, loadResource is called to try to load
    the resource. loadResource should then use addResource to add the
    resource to the cache.

    \sa QTextDocument::ResourceType
*/
QVariant QTextDocument::resource(int type, const QUrl &name) const
{
    Q_D(const QTextDocument);
    QVariant r = d->resources.value(name);
    if (!r.isValid()) {
        r = d->cachedResources.value(name);
        if (!r.isValid())
            r = const_cast<QTextDocument *>(this)->loadResource(type, name);
    }
    return r;
}

/*!
    Adds the resource \a resource to the resource cache, using \a
    type and \a name as identifiers. \a type should be a value from
    QTextDocument::ResourceType.

    For example, you can add an image as a resource in order to reference it
    from within the document:

    \snippet snippets/textdocument-resources/main.cpp Adding a resource

    The image can be inserted into the document using the QTextCursor API:

    \snippet snippets/textdocument-resources/main.cpp Inserting an image with a cursor

    Alternatively, you can insert images using the HTML \c img tag:

    \snippet snippets/textdocument-resources/main.cpp Inserting an image using HTML
*/
void QTextDocument::addResource(int type, const QUrl &name, const QVariant &resource)
{
    Q_UNUSED(type);
    Q_D(QTextDocument);
    d->resources.insert(name, resource);
}

/*!
    Loads data of the specified \a type from the resource with the
    given \a name.

    This function is called by the rich text engine to request data that isn't
    directly stored by QTextDocument, but still associated with it. For example,
    images are referenced indirectly by the name attribute of a QTextImageFormat
    object.

    When called by Qt, \a type is one of the values of
    QTextDocument::ResourceType.

    If the QTextDocument is a child object of a QTextEdit, QTextBrowser,
    or a QTextDocument itself then the default implementation tries
    to retrieve the data from the parent.
*/
QVariant QTextDocument::loadResource(int type, const QUrl &name)
{
    Q_D(QTextDocument);
    QVariant r;

    QTextDocument *doc = qobject_cast<QTextDocument *>(parent());
    if (doc) {
        r = doc->loadResource(type, name);
    }
#ifndef QT_NO_TEXTEDIT
    else if (QTextEdit *edit = qobject_cast<QTextEdit *>(parent())) {
        QUrl resolvedName = edit->d_func()->resolveUrl(name);
        r = edit->loadResource(type, resolvedName);
    }
#endif
#ifndef QT_NO_TEXTCONTROL
    else if (QTextControl *control = qobject_cast<QTextControl *>(parent())) {
        r = control->loadResource(type, name);
    }
#endif

    // handle data: URLs
    if (r.isNull() && name.scheme().compare(QLatin1String("data"), Qt::CaseInsensitive) == 0)
        r = qDecodeDataUrl(name).second;

    // if resource was not loaded try to load it here
    if (!doc && r.isNull() && name.isRelative()) {
        QUrl currentURL = d->url;
        QUrl resourceUrl = name;

        // For the second case QUrl can merge "#someanchor" with "foo.html"
        // correctly to "foo.html#someanchor"
        if (!(currentURL.isRelative()
              || (currentURL.scheme() == QLatin1String("file")
                  && !QFileInfo(currentURL.toLocalFile()).isAbsolute()))
            || (name.hasFragment() && name.path().isEmpty())) {
            resourceUrl =  currentURL.resolved(name);
        } else {
            // this is our last resort when current url and new url are both relative
            // we try to resolve against the current working directory in the local
            // file system.
            QFileInfo fi(currentURL.toLocalFile());
            if (fi.exists()) {
                resourceUrl =
                    QUrl::fromLocalFile(fi.absolutePath() + QDir::separator()).resolved(name);
            } else if (currentURL.isEmpty()) {
                resourceUrl.setScheme(QLatin1String("file"));
            }
        }

        QString s = resourceUrl.toLocalFile();
        QFile f(s);
        if (!s.isEmpty() && f.open(QFile::ReadOnly)) {
            r = f.readAll();
            f.close();
        }
    }

    if (!r.isNull()) {
        if (type == ImageResource && r.type() == QVariant::ByteArray) {
            if (qApp->thread() != QThread::currentThread()) {
                // must use images in non-GUI threads
                QImage image;
                image.loadFromData(r.toByteArray());
                if (!image.isNull())
                    r = image;
            } else {
                QPixmap pm;
                pm.loadFromData(r.toByteArray());
                if (!pm.isNull())
                    r = pm;
            }
        }
        d->cachedResources.insert(name, r);
    }
    return r;
}

static QTextFormat formatDifference(const QTextFormat &from, const QTextFormat &to)
{
    QTextFormat diff = to;

    const QMap<int, QVariant> props = to.properties();
    for (QMap<int, QVariant>::ConstIterator it = props.begin(), end = props.end();
         it != end; ++it)
        if (it.value() == from.property(it.key()))
            diff.clearProperty(it.key());

    return diff;
}

QTextHtmlExporter::QTextHtmlExporter(const QTextDocument *_doc)
    : doc(_doc), fragmentMarkers(false)
{
    const QFont defaultFont = doc->defaultFont();
    defaultCharFormat.setFont(defaultFont);
    // don't export those for the default font since we cannot turn them off with CSS
    defaultCharFormat.clearProperty(QTextFormat::FontUnderline);
    defaultCharFormat.clearProperty(QTextFormat::FontOverline);
    defaultCharFormat.clearProperty(QTextFormat::FontStrikeOut);
    defaultCharFormat.clearProperty(QTextFormat::TextUnderlineStyle);
}

/*!
    Returns the document in HTML format. The conversion may not be
    perfect, especially for complex documents, due to the limitations
    of HTML.
*/
QString QTextHtmlExporter::toHtml(const QByteArray &encoding, ExportMode mode)
{
    html = QLatin1String("<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.0//EN\" "
            "\"http://www.w3.org/TR/REC-html40/strict.dtd\">\n"
            "<html><head><meta name=\"qrichtext\" content=\"1\" />");
    html.reserve(doc->docHandle()->length());

    fragmentMarkers = (mode == ExportFragment);

    if (!encoding.isEmpty())
        html += QString::fromLatin1("<meta http-equiv=\"Content-Type\" content=\"text/html; charset=%1\" />").arg(QString::fromAscii(encoding));

    QString title  = doc->metaInformation(QTextDocument::DocumentTitle);
    if (!title.isEmpty())
        html += QString::fromLatin1("<title>") + title + QString::fromLatin1("</title>");
    html += QLatin1String("<style type=\"text/css\">\n");
    html += QLatin1String("p, li { white-space: pre-wrap; }\n");
    html += QLatin1String("</style>");
    html += QLatin1String("</head><body");

    if (mode == ExportEntireDocument) {
        html += QLatin1String(" style=\"");

        emitFontFamily(defaultCharFormat.fontFamily());

        if (defaultCharFormat.hasProperty(QTextFormat::FontPointSize)) {
            html += QLatin1String(" font-size:");
            html += QString::number(defaultCharFormat.fontPointSize());
            html += QLatin1String("pt;");
        } else if (defaultCharFormat.hasProperty(QTextFormat::FontPixelSize)) {
            html += QLatin1String(" font-size:");
            html += QString::number(defaultCharFormat.intProperty(QTextFormat::FontPixelSize));
            html += QLatin1String("px;");
        }

        html += QLatin1String(" font-weight:");
        html += QString::number(defaultCharFormat.fontWeight() * 8);
        html += QLatin1Char(';');

        html += QLatin1String(" font-style:");
        html += (defaultCharFormat.fontItalic() ? QLatin1String("italic") : QLatin1String("normal"));
        html += QLatin1Char(';');

        // do not set text-decoration on the default font since those values are /always/ propagated
        // and cannot be turned off with CSS

        html += QLatin1Char('\"');

        const QTextFrameFormat fmt = doc->rootFrame()->frameFormat();
        emitBackgroundAttribute(fmt);

    } else {
        defaultCharFormat = QTextCharFormat();
    }
    html += QLatin1Char('>');

    QTextFrameFormat rootFmt = doc->rootFrame()->frameFormat();
    rootFmt.clearProperty(QTextFormat::BackgroundBrush);

    QTextFrameFormat defaultFmt;
    defaultFmt.setMargin(doc->documentMargin());

    if (rootFmt == defaultFmt)
        emitFrame(doc->rootFrame()->begin());
    else
        emitTextFrame(doc->rootFrame());

    html += QLatin1String("</body></html>");
    return html;
}

void QTextHtmlExporter::emitAttribute(const char *attribute, const QString &value)
{
    html += QLatin1Char(' ');
    html += QLatin1String(attribute);
    html += QLatin1String("=\"");
    html += Qt::escape(value);
    html += QLatin1Char('"');
}

bool QTextHtmlExporter::emitCharFormatStyle(const QTextCharFormat &format)
{
    bool attributesEmitted = false;

    {
        const QString family = format.fontFamily();
        if (!family.isEmpty() && family != defaultCharFormat.fontFamily()) {
            emitFontFamily(family);
            attributesEmitted = true;
        }
    }

    if (format.hasProperty(QTextFormat::FontPointSize)
        && format.fontPointSize() != defaultCharFormat.fontPointSize()) {
        html += QLatin1String(" font-size:");
        html += QString::number(format.fontPointSize());
        html += QLatin1String("pt;");
        attributesEmitted = true;
    } else if (format.hasProperty(QTextFormat::FontSizeAdjustment)) {
        static const char * const sizeNames[] = {
            "small", "medium", "large", "x-large", "xx-large"
        };
        const char *name = 0;
        const int idx = format.intProperty(QTextFormat::FontSizeAdjustment) + 1;
        if (idx >= 0 && idx <= 4) {
            name = sizeNames[idx];
        }
        if (name) {
            html += QLatin1String(" font-size:");
            html += QLatin1String(name);
            html += QLatin1Char(';');
            attributesEmitted = true;
        }
    } else if (format.hasProperty(QTextFormat::FontPixelSize)) {
        html += QLatin1String(" font-size:");
        html += QString::number(format.intProperty(QTextFormat::FontPixelSize));
        html += QLatin1String("px;");
    }

    if (format.hasProperty(QTextFormat::FontWeight)
        && format.fontWeight() != defaultCharFormat.fontWeight()) {
        html += QLatin1String(" font-weight:");
        html += QString::number(format.fontWeight() * 8);
        html += QLatin1Char(';');
        attributesEmitted = true;
    }

    if (format.hasProperty(QTextFormat::FontItalic)
        && format.fontItalic() != defaultCharFormat.fontItalic()) {
        html += QLatin1String(" font-style:");
        html += (format.fontItalic() ? QLatin1String("italic") : QLatin1String("normal"));
        html += QLatin1Char(';');
        attributesEmitted = true;
    }

    QLatin1String decorationTag(" text-decoration:");
    html += decorationTag;
    bool hasDecoration = false;
    bool atLeastOneDecorationSet = false;

    if ((format.hasProperty(QTextFormat::FontUnderline) || format.hasProperty(QTextFormat::TextUnderlineStyle))
        && format.fontUnderline() != defaultCharFormat.fontUnderline()) {
        hasDecoration = true;
        if (format.fontUnderline()) {
            html += QLatin1String(" underline");
            atLeastOneDecorationSet = true;
        }
    }

    if (format.hasProperty(QTextFormat::FontOverline)
        && format.fontOverline() != defaultCharFormat.fontOverline()) {
        hasDecoration = true;
        if (format.fontOverline()) {
            html += QLatin1String(" overline");
            atLeastOneDecorationSet = true;
        }
    }

    if (format.hasProperty(QTextFormat::FontStrikeOut)
        && format.fontStrikeOut() != defaultCharFormat.fontStrikeOut()) {
        hasDecoration = true;
        if (format.fontStrikeOut()) {
            html += QLatin1String(" line-through");
            atLeastOneDecorationSet = true;
        }
    }

    if (hasDecoration) {
        if (!atLeastOneDecorationSet)
            html += QLatin1String("none");
        html += QLatin1Char(';');
        attributesEmitted = true;
    } else {
        html.chop(qstrlen(decorationTag.latin1()));
    }

    if (format.foreground() != defaultCharFormat.foreground()
        && format.foreground().style() != Qt::NoBrush) {
        html += QLatin1String(" color:");
        html += format.foreground().color().name();
        html += QLatin1Char(';');
        attributesEmitted = true;
    }

    if (format.background() != defaultCharFormat.background()
        && format.background().style() == Qt::SolidPattern) {
        html += QLatin1String(" background-color:");
        html += format.background().color().name();
        html += QLatin1Char(';');
        attributesEmitted = true;
    }

    if (format.verticalAlignment() != defaultCharFormat.verticalAlignment()
        && format.verticalAlignment() != QTextCharFormat::AlignNormal)
    {
        html += QLatin1String(" vertical-align:");

        QTextCharFormat::VerticalAlignment valign = format.verticalAlignment();
        if (valign == QTextCharFormat::AlignSubScript)
            html += QLatin1String("sub");
        else if (valign == QTextCharFormat::AlignSuperScript)
            html += QLatin1String("super");
        else if (valign == QTextCharFormat::AlignMiddle)
            html += QLatin1String("middle");
        else if (valign == QTextCharFormat::AlignTop)
            html += QLatin1String("top");
        else if (valign == QTextCharFormat::AlignBottom)
            html += QLatin1String("bottom");

        html += QLatin1Char(';');
        attributesEmitted = true;
    }

    if (format.fontCapitalization() != QFont::MixedCase) {
        const QFont::Capitalization caps = format.fontCapitalization();
        if (caps == QFont::AllUppercase)
            html += QLatin1String(" text-transform:uppercase;");
        else if (caps == QFont::AllLowercase)
            html += QLatin1String(" text-transform:lowercase;");
        else if (caps == QFont::SmallCaps)
            html += QLatin1String(" font-variant:small-caps;");
        attributesEmitted = true;
    }

    if (format.fontWordSpacing() != 0.0) {
        html += QLatin1String(" word-spacing:");
        html += QString::number(format.fontWordSpacing());
        html += QLatin1String("px;");
        attributesEmitted = true;
    }

    return attributesEmitted;
}

void QTextHtmlExporter::emitTextLength(const char *attribute, const QTextLength &length)
{
    if (length.type() == QTextLength::VariableLength) // default
        return;

    html += QLatin1Char(' ');
    html += QLatin1String(attribute);
    html += QLatin1String("=\"");
    html += QString::number(length.rawValue());

    if (length.type() == QTextLength::PercentageLength)
        html += QLatin1String("%\"");
    else
        html += QLatin1Char('\"');
}

void QTextHtmlExporter::emitAlignment(Qt::Alignment align)
{
    if (align & Qt::AlignLeft)
        return;
    else if (align & Qt::AlignRight)
        html += QLatin1String(" align=\"right\"");
    else if (align & Qt::AlignHCenter)
        html += QLatin1String(" align=\"center\"");
    else if (align & Qt::AlignJustify)
        html += QLatin1String(" align=\"justify\"");
}

void QTextHtmlExporter::emitFloatStyle(QTextFrameFormat::Position pos, StyleMode mode)
{
    if (pos == QTextFrameFormat::InFlow)
        return;

    if (mode == EmitStyleTag)
        html += QLatin1String(" style=\"float:");
    else
        html += QLatin1String(" float:");

    if (pos == QTextFrameFormat::FloatLeft)
        html += QLatin1String(" left;");
    else if (pos == QTextFrameFormat::FloatRight)
        html += QLatin1String(" right;");
    else
        Q_ASSERT_X(0, "QTextHtmlExporter::emitFloatStyle()", "pos should be a valid enum type");

    if (mode == EmitStyleTag)
        html += QLatin1Char('\"');
}

void QTextHtmlExporter::emitBorderStyle(QTextFrameFormat::BorderStyle style)
{
    Q_ASSERT(style <= QTextFrameFormat::BorderStyle_Outset);

    html += QLatin1String(" border-style:");

    switch (style) {
    case QTextFrameFormat::BorderStyle_None:
        html += QLatin1String("none");
        break;
    case QTextFrameFormat::BorderStyle_Dotted:
        html += QLatin1String("dotted");
        break;
    case QTextFrameFormat::BorderStyle_Dashed:
        html += QLatin1String("dashed");
        break;
    case QTextFrameFormat::BorderStyle_Solid:
        html += QLatin1String("solid");
        break;
    case QTextFrameFormat::BorderStyle_Double:
        html += QLatin1String("double");
        break;
    case QTextFrameFormat::BorderStyle_DotDash:
        html += QLatin1String("dot-dash");
        break;
    case QTextFrameFormat::BorderStyle_DotDotDash:
        html += QLatin1String("dot-dot-dash");
        break;
    case QTextFrameFormat::BorderStyle_Groove:
        html += QLatin1String("groove");
        break;
    case QTextFrameFormat::BorderStyle_Ridge:
        html += QLatin1String("ridge");
        break;
    case QTextFrameFormat::BorderStyle_Inset:
        html += QLatin1String("inset");
        break;
    case QTextFrameFormat::BorderStyle_Outset:
        html += QLatin1String("outset");
        break;
    default:
        Q_ASSERT(false);
        break;
    };

    html += QLatin1Char(';');
}

void QTextHtmlExporter::emitPageBreakPolicy(QTextFormat::PageBreakFlags policy)
{
    if (policy & QTextFormat::PageBreak_AlwaysBefore)
        html += QLatin1String(" page-break-before:always;");

    if (policy & QTextFormat::PageBreak_AlwaysAfter)
        html += QLatin1String(" page-break-after:always;");
}

void QTextHtmlExporter::emitFontFamily(const QString &family)
{
    html += QLatin1String(" font-family:");

    QLatin1String quote("\'");
    if (family.contains(QLatin1Char('\'')))
        quote = QLatin1String("&quot;");

    html += quote;
    html += Qt::escape(family);
    html += quote;
    html += QLatin1Char(';');
}

void QTextHtmlExporter::emitMargins(const QString &top, const QString &bottom, const QString &left, const QString &right)
{
    html += QLatin1String(" margin-top:");
    html += top;
    html += QLatin1String("px;");

    html += QLatin1String(" margin-bottom:");
    html += bottom;
    html += QLatin1String("px;");

    html += QLatin1String(" margin-left:");
    html += left;
    html += QLatin1String("px;");

    html += QLatin1String(" margin-right:");
    html += right;
    html += QLatin1String("px;");
}

void QTextHtmlExporter::emitFragment(const QTextFragment &fragment)
{
    const QTextCharFormat format = fragment.charFormat();

    bool closeAnchor = false;

    if (format.isAnchor()) {
        const QString name = format.anchorName();
        if (!name.isEmpty()) {
            html += QLatin1String("<a name=\"");
            html += Qt::escape(name);
            html += QLatin1String("\"></a>");
        }
        const QString href = format.anchorHref();
        if (!href.isEmpty()) {
            html += QLatin1String("<a href=\"");
            html += Qt::escape(href);
            html += QLatin1String("\">");
            closeAnchor = true;
        }
    }

    QString txt = fragment.text();
    const bool isObject = txt.contains(QChar::ObjectReplacementCharacter);
    const bool isImage = isObject && format.isImageFormat();

    QLatin1String styleTag("<span style=\"");
    html += styleTag;

    bool attributesEmitted = false;
    if (!isImage)
        attributesEmitted = emitCharFormatStyle(format);
    if (attributesEmitted)
        html += QLatin1String("\">");
    else
        html.chop(qstrlen(styleTag.latin1()));

    if (isObject) {
        for (int i = 0; isImage && i < txt.length(); ++i) {
            QTextImageFormat imgFmt = format.toImageFormat();

            html += QLatin1String("<img");

            if (imgFmt.hasProperty(QTextFormat::ImageName))
                emitAttribute("src", imgFmt.name());

            if (imgFmt.hasProperty(QTextFormat::ImageWidth))
                emitAttribute("width", QString::number(imgFmt.width()));

            if (imgFmt.hasProperty(QTextFormat::ImageHeight))
                emitAttribute("height", QString::number(imgFmt.height()));

            if (imgFmt.verticalAlignment() == QTextCharFormat::AlignMiddle)
                html += QLatin1String(" style=\"vertical-align: middle;\"");
            else if (imgFmt.verticalAlignment() == QTextCharFormat::AlignTop)
                html += QLatin1String(" style=\"vertical-align: top;\"");

            if (QTextFrame *imageFrame = qobject_cast<QTextFrame *>(doc->objectForFormat(imgFmt)))
                emitFloatStyle(imageFrame->frameFormat().position());

            html += QLatin1String(" />");
        }
    } else {
        Q_ASSERT(!txt.contains(QChar::ObjectReplacementCharacter));

        txt = Qt::escape(txt);

        // split for [\n{LineSeparator}]
        QString forcedLineBreakRegExp = QString::fromLatin1("[\\na]");
        forcedLineBreakRegExp[3] = QChar::LineSeparator;

        const QStringList lines = txt.split(QRegExp(forcedLineBreakRegExp));
        for (int i = 0; i < lines.count(); ++i) {
            if (i > 0)
                html += QLatin1String("<br />"); // space on purpose for compatibility with Netscape, Lynx & Co.
            html += lines.at(i);
        }
    }

    if (attributesEmitted)
        html += QLatin1String("</span>");

    if (closeAnchor)
        html += QLatin1String("</a>");
}

static bool isOrderedList(int style)
{
    return style == QTextListFormat::ListDecimal || style == QTextListFormat::ListLowerAlpha
           || style == QTextListFormat::ListUpperAlpha
           || style == QTextListFormat::ListUpperRoman
           || style == QTextListFormat::ListLowerRoman
	   ;
}

void QTextHtmlExporter::emitBlockAttributes(const QTextBlock &block)
{
    QTextBlockFormat format = block.blockFormat();
    emitAlignment(format.alignment());

    // assume default to not bloat the html too much
    // html += QLatin1String(" dir='ltr'");
    if (block.textDirection() == Qt::RightToLeft)
        html += QLatin1String(" dir='rtl'");

    QLatin1String style(" style=\"");
    html += style;

    const bool emptyBlock = block.begin().atEnd();
    if (emptyBlock) {
        html += QLatin1String("-qt-paragraph-type:empty;");
    }

    emitMargins(QString::number(format.topMargin()),
                QString::number(format.bottomMargin()),
                QString::number(format.leftMargin()),
                QString::number(format.rightMargin()));

    html += QLatin1String(" -qt-block-indent:");
    html += QString::number(format.indent());
    html += QLatin1Char(';');

    html += QLatin1String(" text-indent:");
    html += QString::number(format.textIndent());
    html += QLatin1String("px;");

    if (block.userState() != -1) {
        html += QLatin1String(" -qt-user-state:");
        html += QString::number(block.userState());
        html += QLatin1Char(';');
    }

    emitPageBreakPolicy(format.pageBreakPolicy());

    QTextCharFormat diff;
    if (emptyBlock) { // only print character properties when we don't expect them to be repeated by actual text in the parag
        const QTextCharFormat blockCharFmt = block.charFormat();
        diff = formatDifference(defaultCharFormat, blockCharFmt).toCharFormat();
    }

    diff.clearProperty(QTextFormat::BackgroundBrush);
    if (format.hasProperty(QTextFormat::BackgroundBrush)) {
        QBrush bg = format.background();
        if (bg.style() != Qt::NoBrush)
            diff.setProperty(QTextFormat::BackgroundBrush, format.property(QTextFormat::BackgroundBrush));
    }

    if (!diff.properties().isEmpty())
        emitCharFormatStyle(diff);

    html += QLatin1Char('"');

}

void QTextHtmlExporter::emitBlock(const QTextBlock &block)
{
    if (block.begin().atEnd()) {
        // ### HACK, remove once QTextFrame::Iterator is fixed
        int p = block.position();
        if (p > 0)
            --p;
        QTextDocumentPrivate::FragmentIterator frag = doc->docHandle()->find(p);
        QChar ch = doc->docHandle()->buffer().at(frag->stringPosition);
        if (ch == QTextBeginningOfFrame
            || ch == QTextEndOfFrame)
            return;
    }

    html += QLatin1Char('\n');

    // save and later restore, in case we 'change' the default format by
    // emitting block char format information
    QTextCharFormat oldDefaultCharFormat = defaultCharFormat;

    QTextList *list = block.textList();
    if (list) {
        if (list->itemNumber(block) == 0) { // first item? emit <ul> or appropriate
            const QTextListFormat format = list->format();
            const int style = format.style();
            switch (style) {
                case QTextListFormat::ListDecimal: html += QLatin1String("<ol"); break;
                case QTextListFormat::ListDisc: html += QLatin1String("<ul"); break;
                case QTextListFormat::ListCircle: html += QLatin1String("<ul type=\"circle\""); break;
                case QTextListFormat::ListSquare: html += QLatin1String("<ul type=\"square\""); break;
                case QTextListFormat::ListLowerAlpha: html += QLatin1String("<ol type=\"a\""); break;
                case QTextListFormat::ListUpperAlpha: html += QLatin1String("<ol type=\"A\""); break;
                case QTextListFormat::ListLowerRoman: html += QLatin1String("<ol type=\"i\""); break;
                case QTextListFormat::ListUpperRoman: html += QLatin1String("<ol type=\"I\""); break;
                default: html += QLatin1String("<ul"); // ### should not happen
            }

            QString styleString = QString::fromLatin1("margin-top: 0px; margin-bottom: 0px; margin-left: 0px; margin-right: 0px;");

            if (format.hasProperty(QTextFormat::ListIndent)) {
                styleString += QLatin1String(" -qt-list-indent: ");
                styleString += QString::number(format.indent());
                styleString += QLatin1Char(';');
            }

            if (format.hasProperty(QTextFormat::ListNumberPrefix)) {
                QString numberPrefix = format.numberPrefix();
                numberPrefix.replace(QLatin1Char('"'), QLatin1String("\\22"));
                numberPrefix.replace(QLatin1Char('\''), QLatin1String("\\27")); // FIXME: There's a problem in the CSS parser the prevents this from being correctly restored
                styleString += QLatin1String(" -qt-list-number-prefix: ");
                styleString += QLatin1Char('\'');
                styleString += numberPrefix;
                styleString += QLatin1Char('\'');
                styleString += QLatin1Char(';');
            }

            if (format.hasProperty(QTextFormat::ListNumberSuffix)) {
                if (format.numberSuffix() != QLatin1String(".")) { // this is our default
                    QString numberSuffix = format.numberSuffix();
                    numberSuffix.replace(QLatin1Char('"'), QLatin1String("\\22"));
                    numberSuffix.replace(QLatin1Char('\''), QLatin1String("\\27")); // see above
                    styleString += QLatin1String(" -qt-list-number-suffix: ");
                    styleString += QLatin1Char('\'');
                    styleString += numberSuffix;
                    styleString += QLatin1Char('\'');
                    styleString += QLatin1Char(';');
                }
            }

            html += QLatin1String(" style=\"");
            html += styleString;
            html += QLatin1String("\">");
        }

        html += QLatin1String("<li");

        const QTextCharFormat blockFmt = formatDifference(defaultCharFormat, block.charFormat()).toCharFormat();
        if (!blockFmt.properties().isEmpty()) {
            html += QLatin1String(" style=\"");
            emitCharFormatStyle(blockFmt);
            html += QLatin1Char('\"');

            defaultCharFormat.merge(block.charFormat());
        }
    }

    const QTextBlockFormat blockFormat = block.blockFormat();
    if (blockFormat.hasProperty(QTextFormat::BlockTrailingHorizontalRulerWidth)) {
        html += QLatin1String("<hr");

        QTextLength width = blockFormat.lengthProperty(QTextFormat::BlockTrailingHorizontalRulerWidth);
        if (width.type() != QTextLength::VariableLength)
            emitTextLength("width", width);
        else
            html += QLatin1Char(' ');

        html += QLatin1String("/>");
        return;
    }

    const bool pre = blockFormat.nonBreakableLines();
    if (pre) {
        if (list)
            html += QLatin1Char('>');
        html += QLatin1String("<pre");
    } else if (!list) {
        html += QLatin1String("<p");
    }

    emitBlockAttributes(block);

    html += QLatin1Char('>');
    if (block.begin().atEnd())
        html += QLatin1String("<br />");

    QTextBlock::Iterator it = block.begin();
    if (fragmentMarkers && !it.atEnd() && block == doc->begin())
        html += QLatin1String("<!--StartFragment-->");

    for (; !it.atEnd(); ++it)
        emitFragment(it.fragment());

    if (fragmentMarkers && block.position() + block.length() == doc->docHandle()->length())
        html += QLatin1String("<!--EndFragment-->");

    if (pre)
        html += QLatin1String("</pre>");
    else if (list)
        html += QLatin1String("</li>");
    else
        html += QLatin1String("</p>");

    if (list) {
        if (list->itemNumber(block) == list->count() - 1) { // last item? close list
            if (isOrderedList(list->format().style()))
                html += QLatin1String("</ol>");
            else
                html += QLatin1String("</ul>");
        }
    }

    defaultCharFormat = oldDefaultCharFormat;
}

extern bool qHasPixmapTexture(const QBrush& brush);

QString QTextHtmlExporter::findUrlForImage(const QTextDocument *doc, qint64 cacheKey, bool isPixmap)
{
    QString url;
    if (!doc)
        return url;

    if (QTextDocument *parent = qobject_cast<QTextDocument *>(doc->parent()))
        return findUrlForImage(parent, cacheKey, isPixmap);

    if (doc && doc->docHandle()) {
        QTextDocumentPrivate *priv = doc->docHandle();
        QMap<QUrl, QVariant>::const_iterator it = priv->cachedResources.constBegin();
        for (; it != priv->cachedResources.constEnd(); ++it) {

            const QVariant &v = it.value();
            if (v.type() == QVariant::Image && !isPixmap) {
                if (qvariant_cast<QImage>(v).cacheKey() == cacheKey)
                    break;
            }

            if (v.type() == QVariant::Pixmap && isPixmap) {
                if (qvariant_cast<QPixmap>(v).cacheKey() == cacheKey)
                    break;
            }
        }

        if (it != priv->cachedResources.constEnd())
            url = it.key().toString();
    }

    return url;
}

void QTextDocumentPrivate::mergeCachedResources(const QTextDocumentPrivate *priv)
{
    if (!priv)
        return;

    cachedResources.unite(priv->cachedResources);
}

void QTextHtmlExporter::emitBackgroundAttribute(const QTextFormat &format)
{
    if (format.hasProperty(QTextFormat::BackgroundImageUrl)) {
        QString url = format.property(QTextFormat::BackgroundImageUrl).toString();
        emitAttribute("background", url);
    } else {
        const QBrush &brush = format.background();
        if (brush.style() == Qt::SolidPattern) {
            emitAttribute("bgcolor", brush.color().name());
        } else if (brush.style() == Qt::TexturePattern) {
            const bool isPixmap = qHasPixmapTexture(brush);
            const qint64 cacheKey = isPixmap ? brush.texture().cacheKey() : brush.textureImage().cacheKey();

            const QString url = findUrlForImage(doc, cacheKey, isPixmap);

            if (!url.isEmpty())
                emitAttribute("background", url);
        }
    }
}

void QTextHtmlExporter::emitTable(const QTextTable *table)
{
    QTextTableFormat format = table->format();

    html += QLatin1String("\n<table");

    if (format.hasProperty(QTextFormat::FrameBorder))
        emitAttribute("border", QString::number(format.border()));

    emitFrameStyle(format, TableFrame);

    emitAlignment(format.alignment());
    emitTextLength("width", format.width());

    if (format.hasProperty(QTextFormat::TableCellSpacing))
        emitAttribute("cellspacing", QString::number(format.cellSpacing()));
    if (format.hasProperty(QTextFormat::TableCellPadding))
        emitAttribute("cellpadding", QString::number(format.cellPadding()));

    emitBackgroundAttribute(format);

    html += QLatin1Char('>');

    const int rows = table->rows();
    const int columns = table->columns();

    QVector<QTextLength> columnWidths = format.columnWidthConstraints();
    if (columnWidths.isEmpty()) {
        columnWidths.resize(columns);
        columnWidths.fill(QTextLength());
    }
    Q_ASSERT(columnWidths.count() == columns);

    QVarLengthArray<bool> widthEmittedForColumn(columns);
    for (int i = 0; i < columns; ++i)
        widthEmittedForColumn[i] = false;

    const int headerRowCount = qMin(format.headerRowCount(), rows);
    if (headerRowCount > 0)
        html += QLatin1String("<thead>");

    for (int row = 0; row < rows; ++row) {
        html += QLatin1String("\n<tr>");

        for (int col = 0; col < columns; ++col) {
            const QTextTableCell cell = table->cellAt(row, col);

            // for col/rowspans
            if (cell.row() != row)
                continue;

            if (cell.column() != col)
                continue;

            html += QLatin1String("\n<td");

            if (!widthEmittedForColumn[col] && cell.columnSpan() == 1) {
                emitTextLength("width", columnWidths.at(col));
                widthEmittedForColumn[col] = true;
            }

            if (cell.columnSpan() > 1)
                emitAttribute("colspan", QString::number(cell.columnSpan()));

            if (cell.rowSpan() > 1)
                emitAttribute("rowspan", QString::number(cell.rowSpan()));

            const QTextTableCellFormat cellFormat = cell.format().toTableCellFormat();
            emitBackgroundAttribute(cellFormat);

            QTextCharFormat oldDefaultCharFormat = defaultCharFormat;

            QTextCharFormat::VerticalAlignment valign = cellFormat.verticalAlignment();

            QString styleString;
            if (valign >= QTextCharFormat::AlignMiddle && valign <= QTextCharFormat::AlignBottom) {
                styleString += QLatin1String(" vertical-align:");
                switch (valign) {
                case QTextCharFormat::AlignMiddle:
                    styleString += QLatin1String("middle");
                    break;
                case QTextCharFormat::AlignTop:
                    styleString += QLatin1String("top");
                    break;
                case QTextCharFormat::AlignBottom:
                    styleString += QLatin1String("bottom");
                    break;
                default:
                    break;
                }
                styleString += QLatin1Char(';');

                QTextCharFormat temp;
                temp.setVerticalAlignment(valign);
                defaultCharFormat.merge(temp);
            }

            if (cellFormat.hasProperty(QTextFormat::TableCellLeftPadding))
                styleString += QLatin1String(" padding-left:") + QString::number(cellFormat.leftPadding()) + QLatin1Char(';');
            if (cellFormat.hasProperty(QTextFormat::TableCellRightPadding))
                styleString += QLatin1String(" padding-right:") + QString::number(cellFormat.rightPadding()) + QLatin1Char(';');
            if (cellFormat.hasProperty(QTextFormat::TableCellTopPadding))
                styleString += QLatin1String(" padding-top:") + QString::number(cellFormat.topPadding()) + QLatin1Char(';');
            if (cellFormat.hasProperty(QTextFormat::TableCellBottomPadding))
                styleString += QLatin1String(" padding-bottom:") + QString::number(cellFormat.bottomPadding()) + QLatin1Char(';');

            if (!styleString.isEmpty())
                html += QLatin1String(" style=\"") + styleString + QLatin1Char('\"');

            html += QLatin1Char('>');

            emitFrame(cell.begin());

            html += QLatin1String("</td>");

            defaultCharFormat = oldDefaultCharFormat;
        }

        html += QLatin1String("</tr>");
        if (headerRowCount > 0 && row == headerRowCount - 1)
            html += QLatin1String("</thead>");
    }

    html += QLatin1String("</table>");
}

void QTextHtmlExporter::emitFrame(QTextFrame::Iterator frameIt)
{
    if (!frameIt.atEnd()) {
        QTextFrame::Iterator next = frameIt;
        ++next;
        if (next.atEnd()
            && frameIt.currentFrame() == 0
            && frameIt.parentFrame() != doc->rootFrame()
            && frameIt.currentBlock().begin().atEnd())
            return;
    }

    for (QTextFrame::Iterator it = frameIt;
         !it.atEnd(); ++it) {
        if (QTextFrame *f = it.currentFrame()) {
            if (QTextTable *table = qobject_cast<QTextTable *>(f)) {
                emitTable(table);
            } else {
                emitTextFrame(f);
            }
        } else if (it.currentBlock().isValid()) {
            emitBlock(it.currentBlock());
        }
    }
}

void QTextHtmlExporter::emitTextFrame(const QTextFrame *f)
{
    FrameType frameType = f->parentFrame() ? TextFrame : RootFrame;

    html += QLatin1String("\n<table");
    QTextFrameFormat format = f->frameFormat();

    if (format.hasProperty(QTextFormat::FrameBorder))
        emitAttribute("border", QString::number(format.border()));

    emitFrameStyle(format, frameType);

    emitTextLength("width", format.width());
    emitTextLength("height", format.height());

    // root frame's bcolor goes in the <body> tag
    if (frameType != RootFrame)
        emitBackgroundAttribute(format);

    html += QLatin1Char('>');
    html += QLatin1String("\n<tr>\n<td style=\"border: none;\">");
    emitFrame(f->begin());
    html += QLatin1String("</td></tr></table>");
}

void QTextHtmlExporter::emitFrameStyle(const QTextFrameFormat &format, FrameType frameType)
{
    QLatin1String styleAttribute(" style=\"");
    html += styleAttribute;
    const int originalHtmlLength = html.length();

    if (frameType == TextFrame)
        html += QLatin1String("-qt-table-type: frame;");
    else if (frameType == RootFrame)
        html += QLatin1String("-qt-table-type: root;");

    const QTextFrameFormat defaultFormat;

    emitFloatStyle(format.position(), OmitStyleTag);
    emitPageBreakPolicy(format.pageBreakPolicy());

    if (format.borderBrush() != defaultFormat.borderBrush()) {
        html += QLatin1String(" border-color:");
        html += format.borderBrush().color().name();
        html += QLatin1Char(';');
    }

    if (format.borderStyle() != defaultFormat.borderStyle())
        emitBorderStyle(format.borderStyle());

    if (format.hasProperty(QTextFormat::FrameMargin)
        || format.hasProperty(QTextFormat::FrameLeftMargin)
        || format.hasProperty(QTextFormat::FrameRightMargin)
        || format.hasProperty(QTextFormat::FrameTopMargin)
        || format.hasProperty(QTextFormat::FrameBottomMargin))
        emitMargins(QString::number(format.topMargin()),
                    QString::number(format.bottomMargin()),
                    QString::number(format.leftMargin()),
                    QString::number(format.rightMargin()));

    if (html.length() == originalHtmlLength) // nothing emitted?
        html.chop(qstrlen(styleAttribute.latin1()));
    else
        html += QLatin1Char('\"');
}

/*!
    Returns a string containing an HTML representation of the document.

    The \a encoding parameter specifies the value for the charset attribute
    in the html header. For example if 'utf-8' is specified then the
    beginning of the generated html will look like this:
    \snippet doc/src/snippets/code/src_gui_text_qtextdocument.cpp 1

    If no encoding is specified then no such meta information is generated.

    If you later on convert the returned html string into a byte array for
    transmission over a network or when saving to disk you should specify
    the encoding you're going to use for the conversion to a byte array here.

    \sa {Supported HTML Subset}
*/
#ifndef QT_NO_TEXTHTMLPARSER
QString QTextDocument::toHtml(const QByteArray &encoding) const
{
    return QTextHtmlExporter(this).toHtml(encoding);
}
#endif // QT_NO_TEXTHTMLPARSER

/*!
    Returns a vector of text formats for all the formats used in the document.
*/
QVector<QTextFormat> QTextDocument::allFormats() const
{
    Q_D(const QTextDocument);
    return d->formatCollection()->formats;
}


/*!
  \internal

  So that not all classes have to be friends of each other...
*/
QTextDocumentPrivate *QTextDocument::docHandle() const
{
    Q_D(const QTextDocument);
    return const_cast<QTextDocumentPrivate *>(d);
}

/*!
    \since 4.4
    \fn QTextDocument::undoCommandAdded()

    This signal is emitted  every time a new level of undo is added to the QTextDocument.
*/

QT_END_NAMESPACE
