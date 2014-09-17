/****************************************************************************
**
** Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
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

#include "qtextedit_p.h"
#include "qlineedit.h"
#include "qtextbrowser.h"

#ifndef QT_NO_TEXTEDIT
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
#include "qtextdocument.h"
#include "private/qtextdocument_p.h"
#include "qtextlist.h"
#include "private/qtextcontrol_p.h"

#include <qtextformat.h>
#include <qdatetime.h>
#include <qapplication.h>
#include <limits.h>
#include <qtexttable.h>
#include <qvariant.h>

#include <qinputcontext.h>
#endif

QT_BEGIN_NAMESPACE


#ifndef QT_NO_TEXTEDIT
static inline bool shouldEnableInputMethod(QTextEdit *textedit)
{
    return !textedit->isReadOnly();
}

class QTextEditControl : public QTextControl
{
public:
    inline QTextEditControl(QObject *parent) : QTextControl(parent) {}

    virtual QMimeData *createMimeDataFromSelection() const {
        QTextEdit *ed = qobject_cast<QTextEdit *>(parent());
        if (!ed)
            return QTextControl::createMimeDataFromSelection();
        return ed->createMimeDataFromSelection();
    }
    virtual bool canInsertFromMimeData(const QMimeData *source) const {
        QTextEdit *ed = qobject_cast<QTextEdit *>(parent());
        if (!ed)
            return QTextControl::canInsertFromMimeData(source);
        return ed->canInsertFromMimeData(source);
    }
    virtual void insertFromMimeData(const QMimeData *source) {
        QTextEdit *ed = qobject_cast<QTextEdit *>(parent());
        if (!ed)
            QTextControl::insertFromMimeData(source);
        else
            ed->insertFromMimeData(source);
    }
};

QTextEditPrivate::QTextEditPrivate()
    : control(0),
      autoFormatting(QTextEdit::AutoNone), tabChangesFocus(false),
      lineWrap(QTextEdit::WidgetWidth), lineWrapColumnOrWidth(0),
      wordWrap(QTextOption::WrapAtWordBoundaryOrAnywhere), clickCausedFocus(0),
      textFormat(Qt::AutoText)
{
    ignoreAutomaticScrollbarAdjustment = false;
    preferRichText = false;
    showCursorOnInitialShow = true;
    inDrag = false;
}

void QTextEditPrivate::createAutoBulletList()
{
    QTextCursor cursor = control->textCursor();
    cursor.beginEditBlock();

    QTextBlockFormat blockFmt = cursor.blockFormat();

    QTextListFormat listFmt;
    listFmt.setStyle(QTextListFormat::ListDisc);
    listFmt.setIndent(blockFmt.indent() + 1);

    blockFmt.setIndent(0);
    cursor.setBlockFormat(blockFmt);

    cursor.createList(listFmt);

    cursor.endEditBlock();
    control->setTextCursor(cursor);
}

void QTextEditPrivate::init(const QString &html)
{
    Q_Q(QTextEdit);
    control = new QTextEditControl(q);
    control->setPalette(q->palette());

    QObject::connect(control, SIGNAL(microFocusChanged()), q, SLOT(updateMicroFocus()));
    QObject::connect(control, SIGNAL(documentSizeChanged(QSizeF)), q, SLOT(_q_adjustScrollbars()));
    QObject::connect(control, SIGNAL(updateRequest(QRectF)), q, SLOT(_q_repaintContents(QRectF)));
    QObject::connect(control, SIGNAL(visibilityRequest(QRectF)), q, SLOT(_q_ensureVisible(QRectF)));
    QObject::connect(control, SIGNAL(currentCharFormatChanged(QTextCharFormat)),
                     q, SLOT(_q_currentCharFormatChanged(QTextCharFormat)));

    QObject::connect(control, SIGNAL(textChanged()), q, SIGNAL(textChanged()));
    QObject::connect(control, SIGNAL(undoAvailable(bool)), q, SIGNAL(undoAvailable(bool)));
    QObject::connect(control, SIGNAL(redoAvailable(bool)), q, SIGNAL(redoAvailable(bool)));
    QObject::connect(control, SIGNAL(copyAvailable(bool)), q, SIGNAL(copyAvailable(bool)));
    QObject::connect(control, SIGNAL(selectionChanged()), q, SIGNAL(selectionChanged()));
    QObject::connect(control, SIGNAL(cursorPositionChanged()), q, SIGNAL(cursorPositionChanged()));

    QObject::connect(control, SIGNAL(textChanged()), q, SLOT(updateMicroFocus()));

    QTextDocument *doc = control->document();
    // set a null page size initially to avoid any relayouting until the textedit
    // is shown. relayoutDocument() will take care of setting the page size to the
    // viewport dimensions later.
    doc->setPageSize(QSize(0, 0));
    doc->documentLayout()->setPaintDevice(viewport);
    doc->setDefaultFont(q->font());
    doc->setUndoRedoEnabled(false); // flush undo buffer.
    doc->setUndoRedoEnabled(true);

    if (!html.isEmpty())
        control->setHtml(html);

    hbar->setSingleStep(20);
    vbar->setSingleStep(20);

    viewport->setBackgroundRole(QPalette::Base);
    q->setAcceptDrops(true);
    q->setFocusPolicy(Qt::WheelFocus);
    q->setAttribute(Qt::WA_KeyCompression);
    q->setAttribute(Qt::WA_InputMethodEnabled);

#ifndef QT_NO_CURSOR
    viewport->setCursor(Qt::IBeamCursor);
#endif
#ifdef Q_WS_WIN
    setSingleFingerPanEnabled(true);
#endif
}

void QTextEditPrivate::_q_repaintContents(const QRectF &contentsRect)
{
    if (!contentsRect.isValid()) {
        viewport->update();
        return;
    }
    const int xOffset = horizontalOffset();
    const int yOffset = verticalOffset();
    const QRectF visibleRect(xOffset, yOffset, viewport->width(), viewport->height());

    QRect r = contentsRect.intersected(visibleRect).toAlignedRect();
    if (r.isEmpty())
        return;

    r.translate(-xOffset, -yOffset);
    viewport->update(r);
}

void QTextEditPrivate::pageUpDown(QTextCursor::MoveOperation op, QTextCursor::MoveMode moveMode)
{
    QTextCursor cursor = control->textCursor();
    bool moved = false;
    qreal lastY = control->cursorRect(cursor).top();
    qreal distance = 0;
    // move using movePosition to keep the cursor's x
    do {
        qreal y = control->cursorRect(cursor).top();
        distance += qAbs(y - lastY);
        lastY = y;
        moved = cursor.movePosition(op, moveMode);
    } while (moved && distance < viewport->height());

    if (moved) {
        if (op == QTextCursor::Up) {
            cursor.movePosition(QTextCursor::Down, moveMode);
            vbar->triggerAction(QAbstractSlider::SliderPageStepSub);
        } else {
            cursor.movePosition(QTextCursor::Up, moveMode);
            vbar->triggerAction(QAbstractSlider::SliderPageStepAdd);
        }
    }
    control->setTextCursor(cursor);
}

#ifndef QT_NO_SCROLLBAR
static QSize documentSize(QTextControl *control)
{
    QTextDocument *doc = control->document();
    QAbstractTextDocumentLayout *layout = doc->documentLayout();

    QSize docSize;

    if (QTextDocumentLayout *tlayout = qobject_cast<QTextDocumentLayout *>(layout)) {
        docSize = tlayout->dynamicDocumentSize().toSize();
        int percentageDone = tlayout->layoutStatus();
        // extrapolate height
        if (percentageDone > 0)
            docSize.setHeight(docSize.height() * 100 / percentageDone);
    } else {
        docSize = layout->documentSize().toSize();
    }

    return docSize;
}

void QTextEditPrivate::_q_adjustScrollbars()
{
    if (ignoreAutomaticScrollbarAdjustment)
        return;
    ignoreAutomaticScrollbarAdjustment = true; // avoid recursion, #106108

    QSize viewportSize = viewport->size();
    QSize docSize = documentSize(control);

    // due to the recursion guard we have to repeat this step a few times,
    // as adding/removing a scroll bar will cause the document or viewport
    // size to change
    // ideally we should loop until the viewport size and doc size stabilize,
    // but in corner cases they might fluctuate, so we need to limit the
    // number of iterations
    for (int i = 0; i < 4; ++i) {
        hbar->setRange(0, docSize.width() - viewportSize.width());
        hbar->setPageStep(viewportSize.width());

        vbar->setRange(0, docSize.height() - viewportSize.height());
        vbar->setPageStep(viewportSize.height());

        // if we are in left-to-right mode widening the document due to
        // lazy layouting does not require a repaint. If in right-to-left
        // the scroll bar has the value zero and it visually has the maximum
        // value (it is visually at the right), then widening the document
        // keeps it at value zero but visually adjusts it to the new maximum
        // on the right, hence we need an update.
        if (q_func()->isRightToLeft())
            viewport->update();

        _q_showOrHideScrollBars();

        const QSize oldViewportSize = viewportSize;
        const QSize oldDocSize = docSize;

        // make sure the document is layouted if the viewport width changes
        viewportSize = viewport->size();
        if (viewportSize.width() != oldViewportSize.width())
            relayoutDocument();

        docSize = documentSize(control);
        if (viewportSize == oldViewportSize && docSize == oldDocSize)
            break;
    }
    ignoreAutomaticScrollbarAdjustment = false;
}
#endif

// rect is in content coordinates
void QTextEditPrivate::_q_ensureVisible(const QRectF &_rect)
{
    const QRect rect = _rect.toRect();
    if ((vbar->isVisible() && vbar->maximum() < rect.bottom())
        || (hbar->isVisible() && hbar->maximum() < rect.right()))
        _q_adjustScrollbars();
    const int visibleWidth = viewport->width();
    const int visibleHeight = viewport->height();
    const bool rtl = q_func()->isRightToLeft();

    if (rect.x() < horizontalOffset()) {
        if (rtl)
            hbar->setValue(hbar->maximum() - rect.x());
        else
            hbar->setValue(rect.x());
    } else if (rect.x() + rect.width() > horizontalOffset() + visibleWidth) {
        if (rtl)
            hbar->setValue(hbar->maximum() - (rect.x() + rect.width() - visibleWidth));
        else
            hbar->setValue(rect.x() + rect.width() - visibleWidth);
    }

    if (rect.y() < verticalOffset())
        vbar->setValue(rect.y());
    else if (rect.y() + rect.height() > verticalOffset() + visibleHeight)
        vbar->setValue(rect.y() + rect.height() - visibleHeight);
}

/*!
    \class QTextEdit
    \brief The QTextEdit class provides a widget that is used to edit and display
    both plain and rich text.

    \ingroup richtext-processing


    \tableofcontents

    \section1 Introduction and Concepts

    QTextEdit is an advanced WYSIWYG viewer/editor supporting rich
    text formatting using HTML-style tags. It is optimized to handle
    large documents and to respond quickly to user input.

    QTextEdit works on paragraphs and characters. A paragraph is a
    formatted string which is word-wrapped to fit into the width of
    the widget. By default when reading plain text, one newline
    signifies a paragraph. A document consists of zero or more
    paragraphs. The words in the paragraph are aligned in accordance
    with the paragraph's alignment. Paragraphs are separated by hard
    line breaks. Each character within a paragraph has its own
    attributes, for example, font and color.

    QTextEdit can display images, lists and tables. If the text is
    too large to view within the text edit's viewport, scroll bars will
    appear. The text edit can load both plain text and HTML files (a
    subset of HTML 3.2 and 4).

    If you just need to display a small piece of rich text use QLabel.

    The rich text support in Qt is designed to provide a fast, portable and
    efficient way to add reasonable online help facilities to
    applications, and to provide a basis for rich text editors. If
    you find the HTML support insufficient for your needs you may consider
    the use of QtWebKit, which provides a full-featured web browser
    widget.

    The shape of the mouse cursor on a QTextEdit is Qt::IBeamCursor by default.
    It can be changed through the viewport()'s cursor property.

    \section1 Using QTextEdit as a Display Widget

    QTextEdit can display a large HTML subset, including tables and
    images.

    The text is set or replaced using setHtml() which deletes any
    existing text and replaces it with the text passed in the
    setHtml() call. If you call setHtml() with legacy HTML, and then
    call toHtml(), the text that is returned may have different markup,
    but will render the same. The entire text can be deleted with clear().

    Text itself can be inserted using the QTextCursor class or using the
    convenience functions insertHtml(), insertPlainText(), append() or
    paste(). QTextCursor is also able to insert complex objects like tables
    or lists into the document, and it deals with creating selections
    and applying changes to selected text.

    By default the text edit wraps words at whitespace to fit within
    the text edit widget. The setLineWrapMode() function is used to
    specify the kind of line wrap you want, or \l NoWrap if you don't
    want any wrapping. Call setLineWrapMode() to set a fixed pixel width
    \l FixedPixelWidth, or character column (e.g. 80 column) \l
    FixedColumnWidth with the pixels or columns specified with
    setLineWrapColumnOrWidth(). If you use word wrap to the widget's width
    \l WidgetWidth, you can specify whether to break on whitespace or
    anywhere with setWordWrapMode().

    The find() function can be used to find and select a given string
    within the text.

    If you want to limit the total number of paragraphs in a QTextEdit,
    as for example it is often useful in a log viewer, then you can use
    QTextDocument's maximumBlockCount property for that.

    \section2 Read-only Key Bindings

    When QTextEdit is used read-only the key bindings are limited to
    navigation, and text may only be selected with the mouse:
    \table
    \header \i Keypresses \i Action
    \row \i Up        \i Moves one line up.
    \row \i Down        \i Moves one line down.
    \row \i Left        \i Moves one character to the left.
    \row \i Right        \i Moves one character to the right.
    \row \i PageUp        \i Moves one (viewport) page up.
    \row \i PageDown        \i Moves one (viewport) page down.
    \row \i Home        \i Moves to the beginning of the text.
    \row \i End                \i Moves to the end of the text.
    \row \i Alt+Wheel
         \i Scrolls the page horizontally (the Wheel is the mouse wheel).
    \row \i Ctrl+Wheel        \i Zooms the text.
    \row \i Ctrl+A            \i Selects all text.
    \endtable

    The text edit may be able to provide some meta-information. For
    example, the documentTitle() function will return the text from
    within HTML \c{<title>} tags.

    \section1 Using QTextEdit as an Editor

    All the information about using QTextEdit as a display widget also
    applies here.

    The current char format's attributes are set with setFontItalic(),
    setFontWeight(), setFontUnderline(), setFontFamily(),
    setFontPointSize(), setTextColor() and setCurrentFont(). The current
    paragraph's alignment is set with setAlignment().

    Selection of text is handled by the QTextCursor class, which provides
    functionality for creating selections, retrieving the text contents or
    deleting selections. You can retrieve the object that corresponds with
    the user-visible cursor using the textCursor() method. If you want to set
    a selection in QTextEdit just create one on a QTextCursor object and
    then make that cursor the visible cursor using setTextCursor(). The selection
    can be copied to the clipboard with copy(), or cut to the clipboard with
    cut(). The entire text can be selected using selectAll().

    When the cursor is moved and the underlying formatting attributes change,
    the currentCharFormatChanged() signal is emitted to reflect the new attributes
    at the new cursor position.

    QTextEdit holds a QTextDocument object which can be retrieved using the
    document() method. You can also set your own document object using setDocument().
    QTextDocument emits a textChanged() signal if the text changes and it also
    provides a isModified() function which will return true if the text has been
    modified since it was either loaded or since the last call to setModified
    with false as argument. In addition it provides methods for undo and redo.

    \section2 Drag and Drop

    QTextEdit also supports custom drag and drop behavior. By default,
    QTextEdit will insert plain text, HTML and rich text when the user drops
    data of these MIME types onto a document. Reimplement
    canInsertFromMimeData() and insertFromMimeData() to add support for
    additional MIME types.

    For example, to allow the user to drag and drop an image onto a QTextEdit,
    you could the implement these functions in the following way:

    \snippet doc/src/snippets/textdocument-imagedrop/textedit.cpp 0

    We add support for image MIME types by returning true. For all other
    MIME types, we use the default implementation.

    \snippet doc/src/snippets/textdocument-imagedrop/textedit.cpp 1

    We unpack the image from the QVariant held by the MIME source and insert
    it into the document as a resource.

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
    \row \i Left \i Moves the cursor one character to the left.
    \row \i Ctrl+Left \i Moves the cursor one word to the left.
    \row \i Right \i Moves the cursor one character to the right.
    \row \i Ctrl+Right \i Moves the cursor one word to the right.
    \row \i Up \i Moves the cursor one line up.
    \row \i Down \i Moves the cursor one line down.
    \row \i PageUp \i Moves the cursor one page up.
    \row \i PageDown \i Moves the cursor one page down.
    \row \i Home \i Moves the cursor to the beginning of the line.
    \row \i Ctrl+Home \i Moves the cursor to the beginning of the text.
    \row \i End \i Moves the cursor to the end of the line.
    \row \i Ctrl+End \i Moves the cursor to the end of the text.
    \row \i Alt+Wheel \i Scrolls the page horizontally (the Wheel is the mouse wheel).
    \endtable

    To select (mark) text hold down the Shift key whilst pressing one
    of the movement keystrokes, for example, \e{Shift+Right}
    will select the character to the right, and \e{Shift+Ctrl+Right} will select the word to the right, etc.

    \sa QTextDocument, QTextCursor, {Application Example},
        {Syntax Highlighter Example}, {Rich Text Processing}
*/

/*!
    \property QTextEdit::plainText
    \since 4.3

    This property gets and sets the text editor's contents as plain
    text. Previous contents are removed and undo/redo history is reset
    when the property is set.

    If the text edit has another content type, it will not be replaced
    by plain text if you call toPlainText(). The only exception to this
    is the non-break space, \e{nbsp;}, that will be converted into
    standard space.

    By default, for an editor with no contents, this property contains
    an empty string.

    \sa html
*/

/*!
    \property QTextEdit::undoRedoEnabled
    \brief whether undo and redo are enabled

    Users are only able to undo or redo actions if this property is
    true, and if there is an action that can be undone (or redone).
*/

/*!
    \enum QTextEdit::LineWrapMode

    \value NoWrap
    \value WidgetWidth
    \value FixedPixelWidth
    \value FixedColumnWidth
*/

/*!
    \enum QTextEdit::AutoFormattingFlag

    \value AutoNone Don't do any automatic formatting.
    \value AutoBulletList Automatically create bullet lists (e.g. when
    the user enters an asterisk ('*') in the left most column, or
    presses Enter in an existing list item.
    \value AutoAll Apply all automatic formatting. Currently only
    automatic bullet lists are supported.
*/

#ifdef QT3_SUPPORT
/*!
    \enum QTextEdit::CursorAction
    \compat

    \value MoveBackward
    \value MoveForward
    \value MoveWordBackward
    \value MoveWordForward
    \value MoveUp
    \value MoveDown
    \value MoveLineStart
    \value MoveLineEnd
    \value MoveHome
    \value MoveEnd
    \value MovePageUp
    \value MovePageDown

    \omitvalue MovePgUp
    \omitvalue MovePgDown
*/
#endif

/*!
    Constructs an empty QTextEdit with parent \a
    parent.
*/
QTextEdit::QTextEdit(QWidget *parent)
    : QAbstractScrollArea(*new QTextEditPrivate, parent)
{
    Q_D(QTextEdit);
    d->init();
}

/*!
    \internal
*/
QTextEdit::QTextEdit(QTextEditPrivate &dd, QWidget *parent)
    : QAbstractScrollArea(dd, parent)
{
    Q_D(QTextEdit);
    d->init();
}

/*!
    Constructs a QTextEdit with parent \a parent. The text edit will display
    the text \a text. The text is interpreted as html.
*/
QTextEdit::QTextEdit(const QString &text, QWidget *parent)
    : QAbstractScrollArea(*new QTextEditPrivate, parent)
{
    Q_D(QTextEdit);
    d->init(text);
}

#ifdef QT3_SUPPORT
/*!
    Use one of the constructors that doesn't take the \a name
    argument and then use setObjectName() instead.
*/
QTextEdit::QTextEdit(QWidget *parent, const char *name)
    : QAbstractScrollArea(*new QTextEditPrivate, parent)
{
    Q_D(QTextEdit);
    d->init();
    setObjectName(QString::fromAscii(name));
}
#endif


/*!
    Destructor.
*/
QTextEdit::~QTextEdit()
{
}

/*!
    Returns the point size of the font of the current format.

    \sa setFontFamily() setCurrentFont() setFontPointSize()
*/
qreal QTextEdit::fontPointSize() const
{
    Q_D(const QTextEdit);
    return d->control->textCursor().charFormat().fontPointSize();
}

/*!
    Returns the font family of the current format.

    \sa setFontFamily() setCurrentFont() setFontPointSize()
*/
QString QTextEdit::fontFamily() const
{
    Q_D(const QTextEdit);
    return d->control->textCursor().charFormat().fontFamily();
}

/*!
    Returns the font weight of the current format.

    \sa setFontWeight() setCurrentFont() setFontPointSize() QFont::Weight
*/
int QTextEdit::fontWeight() const
{
    Q_D(const QTextEdit);
    return d->control->textCursor().charFormat().fontWeight();
}

/*!
    Returns true if the font of the current format is underlined; otherwise returns
    false.

    \sa setFontUnderline()
*/
bool QTextEdit::fontUnderline() const
{
    Q_D(const QTextEdit);
    return d->control->textCursor().charFormat().fontUnderline();
}

/*!
    Returns true if the font of the current format is italic; otherwise returns
    false.

    \sa setFontItalic()
*/
bool QTextEdit::fontItalic() const
{
    Q_D(const QTextEdit);
    return d->control->textCursor().charFormat().fontItalic();
}

/*!
    Returns the text color of the current format.

    \sa setTextColor()
*/
QColor QTextEdit::textColor() const
{
    Q_D(const QTextEdit);
    return d->control->textCursor().charFormat().foreground().color();
}

/*!
    \since 4.4

    Returns the text background color of the current format.

    \sa setTextBackgroundColor()
*/
QColor QTextEdit::textBackgroundColor() const
{
    Q_D(const QTextEdit);
    return d->control->textCursor().charFormat().background().color();
}

/*!
    Returns the font of the current format.

    \sa setCurrentFont() setFontFamily() setFontPointSize()
*/
QFont QTextEdit::currentFont() const
{
    Q_D(const QTextEdit);
    return d->control->textCursor().charFormat().font();
}

/*!
    Sets the alignment of the current paragraph to \a a. Valid
    alignments are Qt::AlignLeft, Qt::AlignRight,
    Qt::AlignJustify and Qt::AlignCenter (which centers
    horizontally).
*/
void QTextEdit::setAlignment(Qt::Alignment a)
{
    Q_D(QTextEdit);
    QTextBlockFormat fmt;
    fmt.setAlignment(a);
    QTextCursor cursor = d->control->textCursor();
    cursor.mergeBlockFormat(fmt);
    d->control->setTextCursor(cursor);
}

/*!
    Returns the alignment of the current paragraph.

    \sa setAlignment()
*/
Qt::Alignment QTextEdit::alignment() const
{
    Q_D(const QTextEdit);
    return d->control->textCursor().blockFormat().alignment();
}

/*!
    Makes \a document the new document of the text editor.

    \note The editor \e{does not take ownership of the document} unless it
    is the document's parent object. The parent object of the provided document
    remains the owner of the object.

    The editor does not delete the current document, even if it is a child of the editor.

    \sa document()
*/
void QTextEdit::setDocument(QTextDocument *document)
{
    Q_D(QTextEdit);
    d->control->setDocument(document);
    d->updateDefaultTextOption();
    d->relayoutDocument();
}

/*!
    Returns a pointer to the underlying document.

    \sa setDocument()
*/
QTextDocument *QTextEdit::document() const
{
    Q_D(const QTextEdit);
    return d->control->document();
}

/*!
    Sets the visible \a cursor.
*/
void QTextEdit::setTextCursor(const QTextCursor &cursor)
{
    Q_D(QTextEdit);
    d->control->setTextCursor(cursor);
}

/*!
    Returns a copy of the QTextCursor that represents the currently visible cursor.
    Note that changes on the returned cursor do not affect QTextEdit's cursor; use
    setTextCursor() to update the visible cursor.
 */
QTextCursor QTextEdit::textCursor() const
{
    Q_D(const QTextEdit);
    return d->control->textCursor();
}

/*!
    Sets the font family of the current format to \a fontFamily.

    \sa fontFamily() setCurrentFont()
*/
void QTextEdit::setFontFamily(const QString &fontFamily)
{
    QTextCharFormat fmt;
    fmt.setFontFamily(fontFamily);
    mergeCurrentCharFormat(fmt);
}

/*!
    Sets the point size of the current format to \a s.

    Note that if \a s is zero or negative, the behavior of this
    function is not defined.

    \sa fontPointSize() setCurrentFont() setFontFamily()
*/
void QTextEdit::setFontPointSize(qreal s)
{
    QTextCharFormat fmt;
    fmt.setFontPointSize(s);
    mergeCurrentCharFormat(fmt);
}

/*!
    \fn void QTextEdit::setFontWeight(int weight)

    Sets the font weight of the current format to the given \a weight,
    where the value used is in the range defined by the QFont::Weight
    enum.

    \sa fontWeight(), setCurrentFont(), setFontFamily()
*/
void QTextEdit::setFontWeight(int w)
{
    QTextCharFormat fmt;
    fmt.setFontWeight(w);
    mergeCurrentCharFormat(fmt);
}

/*!
    If \a underline is true, sets the current format to underline;
    otherwise sets the current format to non-underline.

    \sa fontUnderline()
*/
void QTextEdit::setFontUnderline(bool underline)
{
    QTextCharFormat fmt;
    fmt.setFontUnderline(underline);
    mergeCurrentCharFormat(fmt);
}

/*!
    If \a italic is true, sets the current format to italic;
    otherwise sets the current format to non-italic.

    \sa fontItalic()
*/
void QTextEdit::setFontItalic(bool italic)
{
    QTextCharFormat fmt;
    fmt.setFontItalic(italic);
    mergeCurrentCharFormat(fmt);
}

/*!
    Sets the text color of the current format to \a c.

    \sa textColor()
*/
void QTextEdit::setTextColor(const QColor &c)
{
    QTextCharFormat fmt;
    fmt.setForeground(QBrush(c));
    mergeCurrentCharFormat(fmt);
}

/*!
    \since 4.4

    Sets the text background color of the current format to \a c.

    \sa textBackgroundColor()
*/
void QTextEdit::setTextBackgroundColor(const QColor &c)
{
    QTextCharFormat fmt;
    fmt.setBackground(QBrush(c));
    mergeCurrentCharFormat(fmt);
}

/*!
    Sets the font of the current format to \a f.

    \sa currentFont() setFontPointSize() setFontFamily()
*/
void QTextEdit::setCurrentFont(const QFont &f)
{
    QTextCharFormat fmt;
    fmt.setFont(f);
    mergeCurrentCharFormat(fmt);
}

/*!
    \since 4.2

    Undoes the last operation.

    If there is no operation to undo, i.e. there is no undo step in
    the undo/redo history, nothing happens.

    \sa redo()
*/
void QTextEdit::undo()
{
    Q_D(QTextEdit);
    d->control->undo();
}

void QTextEdit::redo()
{
    Q_D(QTextEdit);
    d->control->redo();
}

/*!
    \fn void QTextEdit::undo() const
    \fn void QTextEdit::redo() const
    \overload

    Use the non-const overload instead.
*/

/*!
    \fn void QTextEdit::redo()
    \since 4.2

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

void QTextEdit::cut()
{
    Q_D(QTextEdit);
    d->control->cut();
}

/*!
    Copies any selected text to the clipboard.

    \sa copyAvailable()
*/

void QTextEdit::copy()
{
    Q_D(QTextEdit);
    d->control->copy();
}

/*!
    Pastes the text from the clipboard into the text edit at the
    current cursor position.

    If there is no text in the clipboard nothing happens.

    To change the behavior of this function, i.e. to modify what
    QTextEdit can paste and how it is being pasted, reimplement the
    virtual canInsertFromMimeData() and insertFromMimeData()
    functions.

    \sa cut() copy()
*/

void QTextEdit::paste()
{
    Q_D(QTextEdit);
    d->control->paste();
}
#endif

/*!
    Deletes all the text in the text edit.

    Note that the undo/redo history is cleared by this function.

    \sa cut() setPlainText() setHtml()
*/
void QTextEdit::clear()
{
    Q_D(QTextEdit);
    // clears and sets empty content
    d->control->clear();
}


/*!
    Selects all text.

    \sa copy() cut() textCursor()
 */
void QTextEdit::selectAll()
{
    Q_D(QTextEdit);
    d->control->selectAll();
}

/*! \internal
*/
bool QTextEdit::event(QEvent *e)
{
    Q_D(QTextEdit);
#ifndef QT_NO_CONTEXTMENU
    if (e->type() == QEvent::ContextMenu
        && static_cast<QContextMenuEvent *>(e)->reason() == QContextMenuEvent::Keyboard) {
        Q_D(QTextEdit);
        ensureCursorVisible();
        const QPoint cursorPos = cursorRect().center();
        QContextMenuEvent ce(QContextMenuEvent::Keyboard, cursorPos, d->viewport->mapToGlobal(cursorPos));
        ce.setAccepted(e->isAccepted());
        const bool result = QAbstractScrollArea::event(&ce);
        e->setAccepted(ce.isAccepted());
        return result;
    } else if (e->type() == QEvent::ShortcutOverride
               || e->type() == QEvent::ToolTip) {
        d->sendControlEvent(e);
    }
#endif // QT_NO_CONTEXTMENU
#ifdef QT_KEYPAD_NAVIGATION
    if (e->type() == QEvent::EnterEditFocus || e->type() == QEvent::LeaveEditFocus) {
        if (QApplication::keypadNavigationEnabled())
            d->sendControlEvent(e);
    }
#endif
    return QAbstractScrollArea::event(e);
}

/*! \internal
*/

void QTextEdit::timerEvent(QTimerEvent *e)
{
    Q_D(QTextEdit);
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

    \sa toPlainText()
*/

void QTextEdit::setPlainText(const QString &text)
{
    Q_D(QTextEdit);
    d->control->setPlainText(text);
    d->preferRichText = false;
}

/*!
    \fn QString QTextEdit::toPlainText() const

    Returns the text of the text edit as plain text.

    \sa QTextEdit::setPlainText()
 */


/*!
    \property QTextEdit::html

    This property provides an HTML interface to the text of the text edit.

    toHtml() returns the text of the text edit as html.

    setHtml() changes the text of the text edit.  Any previous text is
    removed and the undo/redo history is cleared. The input text is
    interpreted as rich text in html format.

    \note It is the responsibility of the caller to make sure that the
    text is correctly decoded when a QString containing HTML is created
    and passed to setHtml().

    By default, for a newly-created, empty document, this property contains
    text to describe an HTML 4.0 document with no body text.

    \sa {Supported HTML Subset}, plainText
*/

#ifndef QT_NO_TEXTHTMLPARSER
void QTextEdit::setHtml(const QString &text)
{
    Q_D(QTextEdit);
    d->control->setHtml(text);
    d->preferRichText = true;
}
#endif

/*! \reimp
*/
void QTextEdit::keyPressEvent(QKeyEvent *e)
{
    Q_D(QTextEdit);

#ifdef QT_KEYPAD_NAVIGATION
    switch (e->key()) {
        case Qt::Key_Select:
            if (QApplication::keypadNavigationEnabled()) {
                // code assumes linksaccessible + editable isn't meaningful
                if (d->control->textInteractionFlags() & Qt::TextEditable) {
                    setEditFocus(!hasEditFocus());
                } else {
                    if (!hasEditFocus())
                        setEditFocus(true);
                    else {
                        QTextCursor cursor = d->control->textCursor();
                        QTextCharFormat charFmt = cursor.charFormat();
                        if (!(d->control->textInteractionFlags() & Qt::LinksAccessibleByKeyboard)
                            || !cursor.hasSelection() || charFmt.anchorHref().isEmpty()) {
                            e->accept();
                            return;
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
                    if (e->text()[0].isPrint())
                        setEditFocus(true);
                    else {
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

    {
        QTextCursor cursor = d->control->textCursor();
        const QString text = e->text();
        if (cursor.atBlockStart()
            && (d->autoFormatting & AutoBulletList)
            && (text.length() == 1)
            && (text.at(0) == QLatin1Char('-') || text.at(0) == QLatin1Char('*'))
            && (!cursor.currentList())) {

            d->createAutoBulletList();
            e->accept();
            return;
        }
    }

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
            case Qt::Key_Back:
                if (!e->isAutoRepeat()) {
                    if (QApplication::keypadNavigationEnabled()) {
                        if (document()->isEmpty() || !(d->control->textInteractionFlags() & Qt::TextEditable)) {
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
void QTextEdit::keyReleaseEvent(QKeyEvent *e)
{
#ifdef QT_KEYPAD_NAVIGATION
    Q_D(QTextEdit);
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
            e->accept();
            return;
        }
    }
#endif
    e->ignore();
}

/*!
    Loads the resource specified by the given \a type and \a name.

    This function is an extension of QTextDocument::loadResource().

    \sa QTextDocument::loadResource()
*/
QVariant QTextEdit::loadResource(int type, const QUrl &name)
{
    Q_UNUSED(type);
    Q_UNUSED(name);
    return QVariant();
}

/*! \reimp
*/
void QTextEdit::resizeEvent(QResizeEvent *e)
{
    Q_D(QTextEdit);

    if (d->lineWrap == NoWrap) {
        QTextDocument *doc = d->control->document();
        QVariant alignmentProperty = doc->documentLayout()->property("contentHasAlignment");

        if (!doc->pageSize().isNull()
            && alignmentProperty.type() == QVariant::Bool
            && !alignmentProperty.toBool()) {

            d->_q_adjustScrollbars();
            return;
        }
    }

    if (d->lineWrap != FixedPixelWidth
        && e->oldSize().width() != e->size().width())
        d->relayoutDocument();
    else
        d->_q_adjustScrollbars();
}

void QTextEditPrivate::relayoutDocument()
{
    QTextDocument *doc = control->document();
    QAbstractTextDocumentLayout *layout = doc->documentLayout();

    if (QTextDocumentLayout *tlayout = qobject_cast<QTextDocumentLayout *>(layout)) {
        if (lineWrap == QTextEdit::FixedColumnWidth)
            tlayout->setFixedColumnWidth(lineWrapColumnOrWidth);
        else
            tlayout->setFixedColumnWidth(-1);
    }

    QTextDocumentLayout *tlayout = qobject_cast<QTextDocumentLayout *>(layout);
    QSize lastUsedSize;
    if (tlayout)
        lastUsedSize = tlayout->dynamicDocumentSize().toSize();
    else
        lastUsedSize = layout->documentSize().toSize();

    // ignore calls to _q_adjustScrollbars caused by an emission of the
    // usedSizeChanged() signal in the layout, as we're calling it
    // later on our own anyway (or deliberately not) .
    const bool oldIgnoreScrollbarAdjustment = ignoreAutomaticScrollbarAdjustment;
    ignoreAutomaticScrollbarAdjustment = true;

    int width = viewport->width();
    if (lineWrap == QTextEdit::FixedPixelWidth)
        width = lineWrapColumnOrWidth;
    else if (lineWrap == QTextEdit::NoWrap) {
        QVariant alignmentProperty = doc->documentLayout()->property("contentHasAlignment");
        if (alignmentProperty.type() == QVariant::Bool && !alignmentProperty.toBool()) {

            width = 0;
        }
    }

    doc->setPageSize(QSize(width, -1));
    if (tlayout)
        tlayout->ensureLayouted(verticalOffset() + viewport->height());

    ignoreAutomaticScrollbarAdjustment = oldIgnoreScrollbarAdjustment;

    QSize usedSize;
    if (tlayout)
        usedSize = tlayout->dynamicDocumentSize().toSize();
    else
        usedSize = layout->documentSize().toSize();

    // this is an obscure situation in the layout that can happen:
    // if a character at the end of a line is the tallest one and therefore
    // influencing the total height of the line and the line right below it
    // is always taller though, then it can happen that if due to line breaking
    // that tall character wraps into the lower line the document not only shrinks
    // horizontally (causing the character to wrap in the first place) but also
    // vertically, because the original line is now smaller and the one below kept
    // its size. So a layout with less width _can_ take up less vertical space, too.
    // If the wider case causes a vertical scroll bar to appear and the narrower one
    // (narrower because the vertical scroll bar takes up horizontal space)) to disappear
    // again then we have an endless loop, as _q_adjustScrollBars sets new ranges on the
    // scroll bars, the QAbstractScrollArea will find out about it and try to show/hide
    // the scroll bars again. That's why we try to detect this case here and break out.
    //
    // (if you change this please also check the layoutingLoop() testcase in
    // QTextEdit's autotests)
    if (lastUsedSize.isValid()
        && !vbar->isHidden()
        && viewport->width() < lastUsedSize.width()
        && usedSize.height() < lastUsedSize.height()
        && usedSize.height() <= viewport->height())
        return;

    _q_adjustScrollbars();
}

void QTextEditPrivate::paint(QPainter *p, QPaintEvent *e)
{
    const int xOffset = horizontalOffset();
    const int yOffset = verticalOffset();

    QRect r = e->rect();
    p->translate(-xOffset, -yOffset);
    r.translate(xOffset, yOffset);

    QTextDocument *doc = control->document();
    QTextDocumentLayout *layout = qobject_cast<QTextDocumentLayout *>(doc->documentLayout());

    // the layout might need to expand the root frame to
    // the viewport if NoWrap is set
    if (layout)
        layout->setViewport(viewport->rect());

    control->drawContents(p, r, q_func());

    if (layout)
        layout->setViewport(QRect());
}

/*! \fn void QTextEdit::paintEvent(QPaintEvent *event)

This event handler can be reimplemented in a subclass to receive paint events passed in \a event.
It is usually unnecessary to reimplement this function in a subclass of QTextEdit.

\warning The underlying text document must not be modified from within a reimplementation
of this function.
*/
void QTextEdit::paintEvent(QPaintEvent *e)
{
    Q_D(QTextEdit);
    QPainter p(d->viewport);
    d->paint(&p, e);
}

void QTextEditPrivate::_q_currentCharFormatChanged(const QTextCharFormat &fmt)
{
    Q_Q(QTextEdit);
    emit q->currentCharFormatChanged(fmt);
#ifdef QT3_SUPPORT
    // compat signals
    emit q->currentFontChanged(fmt.font());
    emit q->currentColorChanged(fmt.foreground().color());
#endif
}

void QTextEditPrivate::updateDefaultTextOption()
{
    QTextDocument *doc = control->document();

    QTextOption opt = doc->defaultTextOption();
    QTextOption::WrapMode oldWrapMode = opt.wrapMode();

    if (lineWrap == QTextEdit::NoWrap)
        opt.setWrapMode(QTextOption::NoWrap);
    else
        opt.setWrapMode(wordWrap);

    if (opt.wrapMode() != oldWrapMode)
        doc->setDefaultTextOption(opt);
}

/*! \reimp
*/
void QTextEdit::mousePressEvent(QMouseEvent *e)
{
    Q_D(QTextEdit);
#ifdef QT_KEYPAD_NAVIGATION
    if (QApplication::keypadNavigationEnabled() && !hasEditFocus())
        setEditFocus(true);
#endif
    d->sendControlEvent(e);
}

/*! \reimp
*/
void QTextEdit::mouseMoveEvent(QMouseEvent *e)
{
    Q_D(QTextEdit);
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
void QTextEdit::mouseReleaseEvent(QMouseEvent *e)
{
    Q_D(QTextEdit);
    d->sendControlEvent(e);
    if (d->autoScrollTimer.isActive()) {
        d->autoScrollTimer.stop();
        ensureCursorVisible();
    }
    if (!isReadOnly() && rect().contains(e->pos()))
        d->handleSoftwareInputPanel(e->button(), d->clickCausedFocus);
    d->clickCausedFocus = 0;
}

/*! \reimp
*/
void QTextEdit::mouseDoubleClickEvent(QMouseEvent *e)
{
    Q_D(QTextEdit);
    d->sendControlEvent(e);
}

/*! \reimp
*/
bool QTextEdit::focusNextPrevChild(bool next)
{
    Q_D(const QTextEdit);
    if (!d->tabChangesFocus && d->control->textInteractionFlags() & Qt::TextEditable)
        return false;
    return QAbstractScrollArea::focusNextPrevChild(next);
}

#ifndef QT_NO_CONTEXTMENU
/*!
  \fn void QTextEdit::contextMenuEvent(QContextMenuEvent *event)

  Shows the standard context menu created with createStandardContextMenu().

  If you do not want the text edit to have a context menu, you can set
  its \l contextMenuPolicy to Qt::NoContextMenu. If you want to
  customize the context menu, reimplement this function. If you want
  to extend the standard context menu, reimplement this function, call
  createStandardContextMenu() and extend the menu returned.

  Information about the event is passed in the \a event object.

  \snippet doc/src/snippets/code/src_gui_widgets_qtextedit.cpp 0
*/
void QTextEdit::contextMenuEvent(QContextMenuEvent *e)
{
    Q_D(QTextEdit);
    d->sendControlEvent(e);
}
#endif // QT_NO_CONTEXTMENU

#ifndef QT_NO_DRAGANDDROP
/*! \reimp
*/
void QTextEdit::dragEnterEvent(QDragEnterEvent *e)
{
    Q_D(QTextEdit);
    d->inDrag = true;
    d->sendControlEvent(e);
}

/*! \reimp
*/
void QTextEdit::dragLeaveEvent(QDragLeaveEvent *e)
{
    Q_D(QTextEdit);
    d->inDrag = false;
    d->autoScrollTimer.stop();
    d->sendControlEvent(e);
}

/*! \reimp
*/
void QTextEdit::dragMoveEvent(QDragMoveEvent *e)
{
    Q_D(QTextEdit);
    d->autoScrollDragPos = e->pos();
    if (!d->autoScrollTimer.isActive())
        d->autoScrollTimer.start(100, this);
    d->sendControlEvent(e);
}

/*! \reimp
*/
void QTextEdit::dropEvent(QDropEvent *e)
{
    Q_D(QTextEdit);
    d->inDrag = false;
    d->autoScrollTimer.stop();
    d->sendControlEvent(e);
}

#endif // QT_NO_DRAGANDDROP

/*! \reimp
 */
void QTextEdit::inputMethodEvent(QInputMethodEvent *e)
{
    Q_D(QTextEdit);
#ifdef QT_KEYPAD_NAVIGATION
    if (d->control->textInteractionFlags() & Qt::TextEditable
        && QApplication::keypadNavigationEnabled()
        && !hasEditFocus())
        setEditFocus(true);
#endif
    d->sendControlEvent(e);
    ensureCursorVisible();
}

/*!\reimp
*/
void QTextEdit::scrollContentsBy(int dx, int dy)
{
    Q_D(QTextEdit);
    if (isRightToLeft())
        dx = -dx;
    d->viewport->scroll(dx, dy);
}

/*!\reimp
*/
QVariant QTextEdit::inputMethodQuery(Qt::InputMethodQuery property) const
{
    Q_D(const QTextEdit);
    QVariant v = d->control->inputMethodQuery(property);
    const QPoint offset(-d->horizontalOffset(), -d->verticalOffset());
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
void QTextEdit::focusInEvent(QFocusEvent *e)
{
    Q_D(QTextEdit);
    if (e->reason() == Qt::MouseFocusReason) {
        d->clickCausedFocus = 1;
    }
    QAbstractScrollArea::focusInEvent(e);
    d->sendControlEvent(e);
}

/*! \reimp
*/
void QTextEdit::focusOutEvent(QFocusEvent *e)
{
    Q_D(QTextEdit);
    QAbstractScrollArea::focusOutEvent(e);
    d->sendControlEvent(e);
}

/*! \reimp
*/
void QTextEdit::showEvent(QShowEvent *)
{
    Q_D(QTextEdit);
    if (!d->anchorToScrollToWhenVisible.isEmpty()) {
        scrollToAnchor(d->anchorToScrollToWhenVisible);
        d->anchorToScrollToWhenVisible.clear();
        d->showCursorOnInitialShow = false;
    } else if (d->showCursorOnInitialShow) {
        d->showCursorOnInitialShow = false;
        ensureCursorVisible();
    }
}

/*! \reimp
*/
void QTextEdit::changeEvent(QEvent *e)
{
    Q_D(QTextEdit);
    QAbstractScrollArea::changeEvent(e);
    if (e->type() == QEvent::ApplicationFontChange
        || e->type() == QEvent::FontChange) {
        d->control->document()->setDefaultFont(font());
    }  else if(e->type() == QEvent::ActivationChange) {
        if (!isActiveWindow())
            d->autoScrollTimer.stop();
    } else if (e->type() == QEvent::EnabledChange) {
        e->setAccepted(isEnabled());
        d->control->setPalette(palette());
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
void QTextEdit::wheelEvent(QWheelEvent *e)
{
    Q_D(QTextEdit);
    if (!(d->control->textInteractionFlags() & Qt::TextEditable)) {
        if (e->modifiers() & Qt::ControlModifier) {
            const int delta = e->delta();
            if (delta < 0)
                zoomOut();
            else if (delta > 0)
                zoomIn();
            return;
        }
    }
    QAbstractScrollArea::wheelEvent(e);
    updateMicroFocus();
}
#endif

#ifndef QT_NO_CONTEXTMENU
/*!  This function creates the standard context menu which is shown
  when the user clicks on the text edit with the right mouse
  button. It is called from the default contextMenuEvent() handler.
  The popup menu's ownership is transferred to the caller.

  We recommend that you use the createStandardContextMenu(QPoint) version instead
  which will enable the actions that are sensitive to where the user clicked.
*/

QMenu *QTextEdit::createStandardContextMenu()
{
    Q_D(QTextEdit);
    return d->control->createStandardContextMenu(QPointF(), this);
}

/*!
  \since 4.4
  This function creates the standard context menu which is shown
  when the user clicks on the text edit with the right mouse
  button. It is called from the default contextMenuEvent() handler
  and it takes the \a position of where the mouse click was.
  This can enable actions that are sensitive to the position where the user clicked.
  The popup menu's ownership is transferred to the caller.
*/

QMenu *QTextEdit::createStandardContextMenu(const QPoint &position)
{
    Q_D(QTextEdit);
    return d->control->createStandardContextMenu(position, this);
}
#endif // QT_NO_CONTEXTMENU

/*!
  returns a QTextCursor at position \a pos (in viewport coordinates).
*/
QTextCursor QTextEdit::cursorForPosition(const QPoint &pos) const
{
    Q_D(const QTextEdit);
    return d->control->cursorForPosition(d->mapToContents(pos));
}

/*!
  returns a rectangle (in viewport coordinates) that includes the
  \a cursor.
 */
QRect QTextEdit::cursorRect(const QTextCursor &cursor) const
{
    Q_D(const QTextEdit);
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
QRect QTextEdit::cursorRect() const
{
    Q_D(const QTextEdit);
    QRect r = d->control->cursorRect().toRect();
    r.translate(-d->horizontalOffset(),-d->verticalOffset());
    return r;
}


/*!
    Returns the reference of the anchor at position \a pos, or an
    empty string if no anchor exists at that point.
*/
QString QTextEdit::anchorAt(const QPoint& pos) const
{
    Q_D(const QTextEdit);
    return d->control->anchorAt(d->mapToContents(pos));
}

/*!
   \property QTextEdit::overwriteMode
   \since 4.1
   \brief whether text entered by the user will overwrite existing text

   As with many text editors, the text editor widget can be configured
   to insert or overwrite existing text with new text entered by the user.

   If this property is true, existing text is overwritten, character-for-character
   by new text; otherwise, text is inserted at the cursor position, displacing
   existing text.

   By default, this property is false (new text does not overwrite existing text).
*/

bool QTextEdit::overwriteMode() const
{
    Q_D(const QTextEdit);
    return d->control->overwriteMode();
}

void QTextEdit::setOverwriteMode(bool overwrite)
{
    Q_D(QTextEdit);
    d->control->setOverwriteMode(overwrite);
}

/*!
    \property QTextEdit::tabStopWidth
    \brief the tab stop width in pixels
    \since 4.1

    By default, this property contains a value of 80 pixels.
*/

int QTextEdit::tabStopWidth() const
{
    Q_D(const QTextEdit);
    return qRound(d->control->document()->defaultTextOption().tabStop());
}

void QTextEdit::setTabStopWidth(int width)
{
    Q_D(QTextEdit);
    QTextOption opt = d->control->document()->defaultTextOption();
    if (opt.tabStop() == width || width < 0)
        return;
    opt.setTabStop(width);
    d->control->document()->setDefaultTextOption(opt);
}

/*!
    \since 4.2
    \property QTextEdit::cursorWidth

    This property specifies the width of the cursor in pixels. The default value is 1.
*/
int QTextEdit::cursorWidth() const
{
    Q_D(const QTextEdit);
    return d->control->cursorWidth();
}

void QTextEdit::setCursorWidth(int width)
{
    Q_D(QTextEdit);
    d->control->setCursorWidth(width);
}

/*!
    \property QTextEdit::acceptRichText
    \brief whether the text edit accepts rich text insertions by the user
    \since 4.1

    When this propery is set to false text edit will accept only
    plain text input from the user. For example through clipboard or drag and drop.

    This property's default is true.
*/

bool QTextEdit::acceptRichText() const
{
    Q_D(const QTextEdit);
    return d->control->acceptRichText();
}

void QTextEdit::setAcceptRichText(bool accept)
{
    Q_D(QTextEdit);
    d->control->setAcceptRichText(accept);
}

/*!
    \class QTextEdit::ExtraSelection
    \since 4.2
    \brief The QTextEdit::ExtraSelection structure provides a way of specifying a
           character format for a given selection in a document
*/

/*!
    \variable QTextEdit::ExtraSelection::cursor
    A cursor that contains a selection in a QTextDocument
*/

/*!
    \variable QTextEdit::ExtraSelection::format
    A format that is used to specify a foreground or background brush/color
    for the selection.
*/

/*!
    \since 4.2
    This function allows temporarily marking certain regions in the document
    with a given color, specified as \a selections. This can be useful for
    example in a programming editor to mark a whole line of text with a given
    background color to indicate the existence of a breakpoint.

    \sa QTextEdit::ExtraSelection, extraSelections()
*/
void QTextEdit::setExtraSelections(const QList<ExtraSelection> &selections)
{
    Q_D(QTextEdit);
    d->control->setExtraSelections(selections);
}

/*!
    \since 4.2
    Returns previously set extra selections.

    \sa setExtraSelections()
*/
QList<QTextEdit::ExtraSelection> QTextEdit::extraSelections() const
{
    Q_D(const QTextEdit);
    return d->control->extraSelections();
}

/*!
    This function returns a new MIME data object to represent the contents
    of the text edit's current selection. It is called when the selection needs
    to be encapsulated into a new QMimeData object; for example, when a drag
    and drop operation is started, or when data is copyied to the clipboard.

    If you reimplement this function, note that the ownership of the returned
    QMimeData object is passed to the caller. The selection can be retrieved
    by using the textCursor() function.
*/
QMimeData *QTextEdit::createMimeDataFromSelection() const
{
    Q_D(const QTextEdit);
    return d->control->QTextControl::createMimeDataFromSelection();
}

/*!
    This function returns true if the contents of the MIME data object, specified
    by \a source, can be decoded and inserted into the document. It is called
    for example when during a drag operation the mouse enters this widget and it
    is necessary to determine whether it is possible to accept the drag and drop
    operation.

    Reimplement this function to enable drag and drop support for additional MIME types.
 */
bool QTextEdit::canInsertFromMimeData(const QMimeData *source) const
{
    Q_D(const QTextEdit);
    return d->control->QTextControl::canInsertFromMimeData(source);
}

/*!
    This function inserts the contents of the MIME data object, specified
    by \a source, into the text edit at the current cursor position. It is
    called whenever text is inserted as the result of a clipboard paste
    operation, or when the text edit accepts data from a drag and drop
    operation.

    Reimplement this function to enable drag and drop support for additional MIME types.
 */
void QTextEdit::insertFromMimeData(const QMimeData *source)
{
    Q_D(QTextEdit);
    d->control->QTextControl::insertFromMimeData(source);
}

/*!
    \property QTextEdit::readOnly
    \brief whether the text edit is read-only

    In a read-only text edit the user can only navigate through the
    text and select text; modifying the text is not possible.

    This property's default is false.
*/

bool QTextEdit::isReadOnly() const
{
    Q_D(const QTextEdit);
    return !(d->control->textInteractionFlags() & Qt::TextEditable);
}

void QTextEdit::setReadOnly(bool ro)
{
    Q_D(QTextEdit);
    Qt::TextInteractionFlags flags = Qt::NoTextInteraction;
    if (ro) {
        flags = Qt::TextSelectableByMouse;
#ifndef QT_NO_TEXTBROWSER
        if (qobject_cast<QTextBrowser *>(this))
            flags |= Qt::TextBrowserInteraction;
#endif
    } else {
        flags = Qt::TextEditorInteraction;
    }
    d->control->setTextInteractionFlags(flags);
    setAttribute(Qt::WA_InputMethodEnabled, shouldEnableInputMethod(this));
}

/*!
    \property QTextEdit::textInteractionFlags
    \since 4.2

    Specifies how the widget should interact with user input.

    The default value depends on whether the QTextEdit is read-only
    or editable, and whether it is a QTextBrowser or not.
*/

void QTextEdit::setTextInteractionFlags(Qt::TextInteractionFlags flags)
{
    Q_D(QTextEdit);
    d->control->setTextInteractionFlags(flags);
}

Qt::TextInteractionFlags QTextEdit::textInteractionFlags() const
{
    Q_D(const QTextEdit);
    return d->control->textInteractionFlags();
}

/*!
    Merges the properties specified in \a modifier into the current character
    format by calling QTextCursor::mergeCharFormat on the editor's cursor.
    If the editor has a selection then the properties of \a modifier are
    directly applied to the selection.

    \sa QTextCursor::mergeCharFormat()
 */
void QTextEdit::mergeCurrentCharFormat(const QTextCharFormat &modifier)
{
    Q_D(QTextEdit);
    d->control->mergeCurrentCharFormat(modifier);
}

/*!
    Sets the char format that is be used when inserting new text to \a
    format by calling QTextCursor::setCharFormat() on the editor's
    cursor.  If the editor has a selection then the char format is
    directly applied to the selection.
 */
void QTextEdit::setCurrentCharFormat(const QTextCharFormat &format)
{
    Q_D(QTextEdit);
    d->control->setCurrentCharFormat(format);
}

/*!
    Returns the char format that is used when inserting new text.
 */
QTextCharFormat QTextEdit::currentCharFormat() const
{
    Q_D(const QTextEdit);
    return d->control->currentCharFormat();
}

/*!
    \property QTextEdit::autoFormatting
    \brief the enabled set of auto formatting features

    The value can be any combination of the values in the
    AutoFormattingFlag enum.  The default is AutoNone. Choose
    AutoAll to enable all automatic formatting.

    Currently, the only automatic formatting feature provided is
    AutoBulletList; future versions of Qt may offer more.
*/

QTextEdit::AutoFormatting QTextEdit::autoFormatting() const
{
    Q_D(const QTextEdit);
    return d->autoFormatting;
}

void QTextEdit::setAutoFormatting(AutoFormatting features)
{
    Q_D(QTextEdit);
    d->autoFormatting = features;
}

/*!
    Convenience slot that inserts \a text at the current
    cursor position.

    It is equivalent to

    \snippet doc/src/snippets/code/src_gui_widgets_qtextedit.cpp 1
 */
void QTextEdit::insertPlainText(const QString &text)
{
    Q_D(QTextEdit);
    d->control->insertPlainText(text);
}

/*!
    Convenience slot that inserts \a text which is assumed to be of
    html formatting at the current cursor position.

    It is equivalent to:

    \snippet doc/src/snippets/code/src_gui_widgets_qtextedit.cpp 2

    \note When using this function with a style sheet, the style sheet will
    only apply to the current block in the document. In order to apply a style
    sheet throughout a document, use QTextDocument::setDefaultStyleSheet()
    instead.
 */
#ifndef QT_NO_TEXTHTMLPARSER
void QTextEdit::insertHtml(const QString &text)
{
    Q_D(QTextEdit);
    d->control->insertHtml(text);
}
#endif // QT_NO_TEXTHTMLPARSER

/*!
    Scrolls the text edit so that the anchor with the given \a name is
    visible; does nothing if the \a name is empty, or is already
    visible, or isn't found.
*/
void QTextEdit::scrollToAnchor(const QString &name)
{
    Q_D(QTextEdit);
    if (name.isEmpty())
        return;

    if (!isVisible()) {
        d->anchorToScrollToWhenVisible = name;
        return;
    }

    QPointF p = d->control->anchorPosition(name);
    const int newPosition = qRound(p.y());
    if ( d->vbar->maximum() < newPosition )
        d->_q_adjustScrollbars();
    d->vbar->setValue(newPosition);
}

/*!
    \fn QTextEdit::zoomIn(int range)

    Zooms in on the text by making the base font size \a range
    points larger and recalculating all font sizes to be the new size.
    This does not change the size of any images.

    \sa zoomOut()
*/
void QTextEdit::zoomIn(int range)
{
    QFont f = font();
    const int newSize = f.pointSize() + range;
    if (newSize <= 0)
        return;
    f.setPointSize(newSize);
    setFont(f);
}

/*!
    \fn QTextEdit::zoomOut(int range)

    \overload

    Zooms out on the text by making the base font size \a range points
    smaller and recalculating all font sizes to be the new size. This
    does not change the size of any images.

    \sa zoomIn()
*/
void QTextEdit::zoomOut(int range)
{
    zoomIn(-range);
}

/*!
    \since 4.2
    Moves the cursor by performing the given \a operation.

    If \a mode is QTextCursor::KeepAnchor, the cursor selects the text it moves over.
    This is the same effect that the user achieves when they hold down the Shift key
    and move the cursor with the cursor keys.

    \sa QTextCursor::movePosition()
*/
void QTextEdit::moveCursor(QTextCursor::MoveOperation operation, QTextCursor::MoveMode mode)
{
    Q_D(QTextEdit);
    d->control->moveCursor(operation, mode);
}

/*!
    \since 4.2
    Returns whether text can be pasted from the clipboard into the textedit.
*/
bool QTextEdit::canPaste() const
{
    Q_D(const QTextEdit);
    return d->control->canPaste();
}

#ifndef QT_NO_PRINTER
/*!
    \since 4.3
    Convenience function to print the text edit's document to the given \a printer. This
    is equivalent to calling the print method on the document directly except that this
    function also supports QPrinter::Selection as print range.

    \sa QTextDocument::print()
*/
void QTextEdit::print(QPrinter *printer) const
{
    Q_D(const QTextEdit);
    d->control->print(printer);
}
#endif // QT _NO_PRINTER

/*! \property QTextEdit::tabChangesFocus
  \brief whether \gui Tab changes focus or is accepted as input

  In some occasions text edits should not allow the user to input
  tabulators or change indentation using the \gui Tab key, as this breaks
  the focus chain. The default is false.

*/

bool QTextEdit::tabChangesFocus() const
{
    Q_D(const QTextEdit);
    return d->tabChangesFocus;
}

void QTextEdit::setTabChangesFocus(bool b)
{
    Q_D(QTextEdit);
    d->tabChangesFocus = b;
}

/*!
    \property QTextEdit::documentTitle
    \brief the title of the document parsed from the text.

    By default, for a newly-created, empty document, this property contains
    an empty string.
*/

/*!
    \property QTextEdit::lineWrapMode
    \brief the line wrap mode

    The default mode is WidgetWidth which causes words to be
    wrapped at the right edge of the text edit. Wrapping occurs at
    whitespace, keeping whole words intact. If you want wrapping to
    occur within words use setWordWrapMode(). If you set a wrap mode of
    FixedPixelWidth or FixedColumnWidth you should also call
    setLineWrapColumnOrWidth() with the width you want.

    \sa lineWrapColumnOrWidth
*/

QTextEdit::LineWrapMode QTextEdit::lineWrapMode() const
{
    Q_D(const QTextEdit);
    return d->lineWrap;
}

void QTextEdit::setLineWrapMode(LineWrapMode wrap)
{
    Q_D(QTextEdit);
    if (d->lineWrap == wrap)
        return;
    d->lineWrap = wrap;
    d->updateDefaultTextOption();
    d->relayoutDocument();
}

/*!
    \property QTextEdit::lineWrapColumnOrWidth
    \brief the position (in pixels or columns depending on the wrap mode) where text will be wrapped

    If the wrap mode is FixedPixelWidth, the value is the number of
    pixels from the left edge of the text edit at which text should be
    wrapped. If the wrap mode is FixedColumnWidth, the value is the
    column number (in character columns) from the left edge of the
    text edit at which text should be wrapped.

    By default, this property contains a value of 0.

    \sa lineWrapMode
*/

int QTextEdit::lineWrapColumnOrWidth() const
{
    Q_D(const QTextEdit);
    return d->lineWrapColumnOrWidth;
}

void QTextEdit::setLineWrapColumnOrWidth(int w)
{
    Q_D(QTextEdit);
    d->lineWrapColumnOrWidth = w;
    d->relayoutDocument();
}

/*!
    \property QTextEdit::wordWrapMode
    \brief the mode QTextEdit will use when wrapping text by words

    By default, this property is set to QTextOption::WrapAtWordBoundaryOrAnywhere.

    \sa QTextOption::WrapMode
*/

QTextOption::WrapMode QTextEdit::wordWrapMode() const
{
    Q_D(const QTextEdit);
    return d->wordWrap;
}

void QTextEdit::setWordWrapMode(QTextOption::WrapMode mode)
{
    Q_D(QTextEdit);
    if (mode == d->wordWrap)
        return;
    d->wordWrap = mode;
    d->updateDefaultTextOption();
}

/*!
    Finds the next occurrence of the string, \a exp, using the given
    \a options. Returns true if \a exp was found and changes the
    cursor to select the match; otherwise returns false.
*/
bool QTextEdit::find(const QString &exp, QTextDocument::FindFlags options)
{
    Q_D(QTextEdit);
    return d->control->find(exp, options);
}

/*!
    \fn void QTextEdit::copyAvailable(bool yes)

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
    \fn void QTextEdit::currentCharFormatChanged(const QTextCharFormat &f)

    This signal is emitted if the current character format has changed, for
    example caused by a change of the cursor position.

    The new format is \a f.

    \sa setCurrentCharFormat()
*/

/*!
    \fn void QTextEdit::selectionChanged()

    This signal is emitted whenever the selection changes.

    \sa copyAvailable()
*/

/*!
    \fn void QTextEdit::cursorPositionChanged()

    This signal is emitted whenever the position of the
    cursor changed.
*/

/*!
    \since 4.2

    Sets the text edit's \a text. The text can be plain text or HTML
    and the text edit will try to guess the right format.

    Use setHtml() or setPlainText() directly to avoid text edit's guessing.

    \sa toPlainText(), toHtml()
*/
void QTextEdit::setText(const QString &text)
{
    Q_D(QTextEdit);
    Qt::TextFormat format = d->textFormat;
    if (d->textFormat == Qt::AutoText)
        format = Qt::mightBeRichText(text) ? Qt::RichText : Qt::PlainText;
#ifndef QT_NO_TEXTHTMLPARSER
    if (format == Qt::RichText || format == Qt::LogText)
        setHtml(text);
    else
#endif
        setPlainText(text);
}

#ifdef QT3_SUPPORT
/*!
    Use the QTextCursor class instead.
*/
void QTextEdit::moveCursor(CursorAction action, QTextCursor::MoveMode mode)
{
    Q_D(QTextEdit);
    if (action == MovePageUp) {
        d->pageUpDown(QTextCursor::Up, mode);
        return;
    } else if (action == MovePageDown) {
        d->pageUpDown(QTextCursor::Down, mode);
        return;
    }

    QTextCursor cursor = d->control->textCursor();
    QTextCursor::MoveOperation op = QTextCursor::NoMove;
    switch (action) {
        case MoveBackward: op = QTextCursor::Left; break;
        case MoveForward: op = QTextCursor::Right; break;
        case MoveWordBackward: op = QTextCursor::WordLeft; break;
        case MoveWordForward: op = QTextCursor::WordRight; break;
        case MoveUp: op = QTextCursor::Up; break;
        case MoveDown: op = QTextCursor::Down; break;
        case MoveLineStart: op = QTextCursor::StartOfLine; break;
        case MoveLineEnd: op = QTextCursor::EndOfLine; break;
        case MoveHome: op = QTextCursor::Start; break;
        case MoveEnd: op = QTextCursor::End; break;
        default: return;
    }
    cursor.movePosition(op, mode);
    d->control->setTextCursor(cursor);
}

/*!
    Use the QTextCursor class instead.
*/
void QTextEdit::moveCursor(CursorAction action, bool select)
{
    moveCursor(action, select ? QTextCursor::KeepAnchor : QTextCursor::MoveAnchor);
}

/*!
    Executes keyboard action \a action.

    Use the QTextCursor class instead.

    \sa textCursor()
*/
void QTextEdit::doKeyboardAction(KeyboardAction action)
{
    Q_D(QTextEdit);
    QTextCursor cursor = d->control->textCursor();
    switch (action) {
        case ActionBackspace: cursor.deletePreviousChar(); break;
        case ActionDelete: cursor.deleteChar(); break;
        case ActionReturn: cursor.insertBlock(); break;
        case ActionKill: {
                QTextBlock block = cursor.block();
                if (cursor.position() == block.position() + block.length() - 2)
                    cursor.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor);
                else
                    cursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
                cursor.deleteChar();
                break;
            }
        case ActionWordBackspace:
            cursor.movePosition(QTextCursor::PreviousWord, QTextCursor::KeepAnchor);
            cursor.deletePreviousChar();
            break;
        case ActionWordDelete:
            cursor.movePosition(QTextCursor::EndOfWord, QTextCursor::KeepAnchor);
            cursor.deleteChar();
            break;
    }
    d->control->setTextCursor(cursor);
}

/*!
    Returns all the text in the text edit as plain text.
*/
QString QTextEdit::text() const
{
    Q_D(const QTextEdit);
    if (d->textFormat == Qt::RichText || d->textFormat == Qt::LogText || (d->textFormat == Qt::AutoText && d->preferRichText))
        return d->control->toHtml();
    else
        return d->control->toPlainText();
}


/*!
    Sets the text format to format \a f.

    \sa textFormat()
*/
void QTextEdit::setTextFormat(Qt::TextFormat f)
{
    Q_D(QTextEdit);
    d->textFormat = f;
}

/*!
    Returns the text format.

    \sa setTextFormat()
*/
Qt::TextFormat QTextEdit::textFormat() const
{
    Q_D(const QTextEdit);
    return d->textFormat;
}

#endif // QT3_SUPPORT

/*!
    Appends a new paragraph with \a text to the end of the text edit.

    \note The new paragraph appended will have the same character format and
    block format as the current paragraph, determined by the position of the cursor.

    \sa currentCharFormat(), QTextCursor::blockFormat()
*/

void QTextEdit::append(const QString &text)
{
    Q_D(QTextEdit);
    const bool atBottom = isReadOnly() ?  d->verticalOffset() >= d->vbar->maximum() :
            d->control->textCursor().atEnd();
    d->control->append(text);
    if (atBottom)
        d->vbar->setValue(d->vbar->maximum());
}

/*!
    Ensures that the cursor is visible by scrolling the text edit if
    necessary.
*/
void QTextEdit::ensureCursorVisible()
{
    Q_D(QTextEdit);
    d->control->ensureCursorVisible();
}

/*!
    \enum QTextEdit::KeyboardAction

    \compat

    \value ActionBackspace
    \value ActionDelete
    \value ActionReturn
    \value ActionKill
    \value ActionWordBackspace
    \value ActionWordDelete
*/

/*!
    \fn bool QTextEdit::find(const QString &exp, bool cs, bool wo)

    Use the find() overload that takes a QTextDocument::FindFlags
    argument.
*/

/*!
    \fn void QTextEdit::sync()

    Does nothing.
*/

/*!
    \fn void QTextEdit::setBold(bool b)

    Use setFontWeight() instead.
*/

/*!
    \fn void QTextEdit::setUnderline(bool b)

    Use setFontUnderline() instead.
*/

/*!
    \fn void QTextEdit::setItalic(bool i)

    Use setFontItalic() instead.
*/

/*!
    \fn void QTextEdit::setFamily(const QString &family)

    Use setFontFamily() instead.
*/

/*!
    \fn void QTextEdit::setPointSize(int size)

    Use setFontPointSize() instead.
*/

/*!
    \fn bool QTextEdit::italic() const

    Use fontItalic() instead.
*/

/*!
    \fn bool QTextEdit::bold() const

    Use fontWeight() >= QFont::Bold instead.
*/

/*!
    \fn bool QTextEdit::underline() const

    Use fontUnderline() instead.
*/

/*!
    \fn QString QTextEdit::family() const

    Use fontFamily() instead.
*/

/*!
    \fn int QTextEdit::pointSize() const

    Use int(fontPointSize()+0.5) instead.
*/

/*!
    \fn bool QTextEdit::hasSelectedText() const

    Use textCursor().hasSelection() instead.
*/

/*!
    \fn QString QTextEdit::selectedText() const

    Use textCursor().selectedText() instead.
*/

/*!
    \fn bool QTextEdit::isUndoAvailable() const

    Use document()->isUndoAvailable() instead.
*/

/*!
    \fn bool QTextEdit::isRedoAvailable() const

    Use document()->isRedoAvailable() instead.
*/

/*!
    \fn void QTextEdit::insert(const QString &text)

    Use insertPlainText() instead.
*/

/*!
    \fn bool QTextEdit::isModified() const

    Use document()->isModified() instead.
*/

/*!
    \fn QColor QTextEdit::color() const

    Use textColor() instead.
*/

/*!
    \fn void QTextEdit::textChanged()

    This signal is emitted whenever the document's content changes; for
    example, when text is inserted or deleted, or when formatting is applied.
*/

/*!
    \fn void QTextEdit::undoAvailable(bool available)

    This signal is emitted whenever undo operations become available
    (\a available is true) or unavailable (\a available is false).
*/

/*!
    \fn void QTextEdit::redoAvailable(bool available)

    This signal is emitted whenever redo operations become available
    (\a available is true) or unavailable (\a available is false).
*/

/*!
    \fn void QTextEdit::currentFontChanged(const QFont &font)

    Use currentCharFormatChanged() instead.
*/

/*!
    \fn void QTextEdit::currentColorChanged(const QColor &color)

    Use currentCharFormatChanged() instead.
*/

/*!
    \fn void QTextEdit::setModified(bool m)

    Use document->setModified() instead.
*/

/*!
    \fn void QTextEdit::setColor(const QColor &color)

    Use setTextColor() instead.
*/
#endif // QT_NO_TEXTEDIT

QT_END_NAMESPACE

#include "moc_qtextedit.cpp"
