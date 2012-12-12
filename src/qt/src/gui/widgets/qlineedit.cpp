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

#include "qlineedit.h"
#include "qlineedit_p.h"

#ifndef QT_NO_LINEEDIT
#include "qaction.h"
#include "qapplication.h"
#include "qclipboard.h"
#include "qdrag.h"
#include "qdrawutil.h"
#include "qevent.h"
#include "qfontmetrics.h"
#include "qmenu.h"
#include "qpainter.h"
#include "qpixmap.h"
#include "qpointer.h"
#include "qstringlist.h"
#include "qstyle.h"
#include "qstyleoption.h"
#include "qtimer.h"
#include "qvalidator.h"
#include "qvariant.h"
#include "qvector.h"
#include "qwhatsthis.h"
#include "qdebug.h"
#include "qtextedit.h"
#include <private/qtextedit_p.h>
#ifndef QT_NO_ACCESSIBILITY
#include "qaccessible.h"
#endif
#ifndef QT_NO_IM
#include "qinputcontext.h"
#include "qlist.h"
#endif
#include "qabstractitemview.h"
#include "private/qstylesheetstyle_p.h"

#ifndef QT_NO_SHORTCUT
#include "private/qapplication_p.h"
#include "private/qshortcutmap_p.h"
#include "qkeysequence.h"
#define ACCEL_KEY(k) (!qApp->d_func()->shortcutMap.hasShortcutForKeySequence(k) ? QLatin1Char('\t') + QString(QKeySequence(k)) : QString())
#else
#define ACCEL_KEY(k) QString()
#endif

#include <limits.h>

QT_BEGIN_NAMESPACE

#ifdef Q_WS_MAC
extern void qt_mac_secure_keyboard(bool); //qapplication_mac.cpp
#endif

/*!
    Initialize \a option with the values from this QLineEdit. This method
    is useful for subclasses when they need a QStyleOptionFrame or QStyleOptionFrameV2, but don't want
    to fill in all the information themselves. This function will check the version
    of the QStyleOptionFrame and fill in the additional values for a
    QStyleOptionFrameV2.

    \sa QStyleOption::initFrom()
*/
void QLineEdit::initStyleOption(QStyleOptionFrame *option) const
{
    if (!option)
        return;

    Q_D(const QLineEdit);
    option->initFrom(this);
    option->rect = contentsRect();
    option->lineWidth = d->frame ? style()->pixelMetric(QStyle::PM_DefaultFrameWidth, option, this)
                                 : 0;
    option->midLineWidth = 0;
    option->state |= QStyle::State_Sunken;
    if (d->control->isReadOnly())
        option->state |= QStyle::State_ReadOnly;
#ifdef QT_KEYPAD_NAVIGATION
    if (hasEditFocus())
        option->state |= QStyle::State_HasEditFocus;
#endif
    if (QStyleOptionFrameV2 *optionV2 = qstyleoption_cast<QStyleOptionFrameV2 *>(option))
        optionV2->features = QStyleOptionFrameV2::None;
}

/*!
    \class QLineEdit
    \brief The QLineEdit widget is a one-line text editor.

    \ingroup basicwidgets


    A line edit allows the user to enter and edit a single line of
    plain text with a useful collection of editing functions,
    including undo and redo, cut and paste, and drag and drop.

    By changing the echoMode() of a line edit, it can also be used as
    a "write-only" field, for inputs such as passwords.

    The length of the text can be constrained to maxLength(). The text
    can be arbitrarily constrained using a validator() or an
    inputMask(), or both. When switching between a validator and an input mask 
    on the same line edit, it is best to clear the validator or input mask to 
    prevent undefined behavior. 


    A related class is QTextEdit which allows multi-line, rich text
    editing.

    You can change the text with setText() or insert(). The text is
    retrieved with text(); the displayed text (which may be different,
    see \l{EchoMode}) is retrieved with displayText(). Text can be
    selected with setSelection() or selectAll(), and the selection can
    be cut(), copy()ied and paste()d. The text can be aligned with
    setAlignment().

    When the text changes the textChanged() signal is emitted; when
    the text changes other than by calling setText() the textEdited()
    signal is emitted; when the cursor is moved the
    cursorPositionChanged() signal is emitted; and when the Return or
    Enter key is pressed the returnPressed() signal is emitted.

    When editing is finished, either because the line edit lost focus
    or Return/Enter is pressed the editingFinished() signal is
    emitted.

    Note that if there is a validator set on the line edit, the
    returnPressed()/editingFinished() signals will only be emitted if
    the validator returns QValidator::Acceptable.

    By default, QLineEdits have a frame as specified by the Windows
    and Motif style guides; you can turn it off by calling
    setFrame(false).

    The default key bindings are described below. The line edit also
    provides a context menu (usually invoked by a right mouse click)
    that presents some of these editing options.
    \target desc
    \table
    \header \i Keypress \i Action
    \row \i Left Arrow \i Moves the cursor one character to the left.
    \row \i Shift+Left Arrow \i Moves and selects text one character to the left.
    \row \i Right Arrow \i Moves the cursor one character to the right.
    \row \i Shift+Right Arrow \i Moves and selects text one character to the right.
    \row \i Home \i Moves the cursor to the beginning of the line.
    \row \i End \i Moves the cursor to the end of the line.
    \row \i Backspace \i Deletes the character to the left of the cursor.
    \row \i Ctrl+Backspace \i Deletes the word to the left of the cursor.
    \row \i Delete \i Deletes the character to the right of the cursor.
    \row \i Ctrl+Delete \i Deletes the word to the right of the cursor.
    \row \i Ctrl+A \i Select all.
    \row \i Ctrl+C \i Copies the selected text to the clipboard.
    \row \i Ctrl+Insert \i Copies the selected text to the clipboard.
    \row \i Ctrl+K \i Deletes to the end of the line.
    \row \i Ctrl+V \i Pastes the clipboard text into line edit.
    \row \i Shift+Insert \i Pastes the clipboard text into line edit.
    \row \i Ctrl+X \i Deletes the selected text and copies it to the clipboard.
    \row \i Shift+Delete \i Deletes the selected text and copies it to the clipboard.
    \row \i Ctrl+Z \i Undoes the last operation.
    \row \i Ctrl+Y \i Redoes the last undone operation.
    \endtable

    Any other key sequence that represents a valid character, will
    cause the character to be inserted into the line edit.

    \table 100%
    \row \o \inlineimage macintosh-lineedit.png Screenshot of a Macintosh style line edit
         \o A line edit shown in the \l{Macintosh Style Widget Gallery}{Macintosh widget style}.
    \row \o \inlineimage windows-lineedit.png Screenshot of a Windows XP style line edit
         \o A line edit shown in the \l{Windows XP Style Widget Gallery}{Windows XP widget style}.
    \row \o \inlineimage plastique-lineedit.png Screenshot of a Plastique style line edit
         \o A line edit shown in the \l{Plastique Style Widget Gallery}{Plastique widget style}.
    \endtable

    \sa QTextEdit, QLabel, QComboBox, {fowler}{GUI Design Handbook: Field, Entry}, {Line Edits Example}
*/


/*!
    \fn void QLineEdit::textChanged(const QString &text)

    This signal is emitted whenever the text changes. The \a text
    argument is the new text.

    Unlike textEdited(), this signal is also emitted when the text is
    changed programmatically, for example, by calling setText().
*/

/*!
    \fn void QLineEdit::textEdited(const QString &text)

    This signal is emitted whenever the text is edited. The \a text
    argument is the new text.

    Unlike textChanged(), this signal is not emitted when the text is
    changed programmatically, for example, by calling setText().
*/

/*!
    \fn void QLineEdit::cursorPositionChanged(int old, int new)

    This signal is emitted whenever the cursor moves. The previous
    position is given by \a old, and the new position by \a new.

    \sa setCursorPosition(), cursorPosition()
*/

/*!
    \fn void QLineEdit::selectionChanged()

    This signal is emitted whenever the selection changes.

    \sa hasSelectedText(), selectedText()
*/

/*!
    Constructs a line edit with no text.

    The maximum text length is set to 32767 characters.

    The \a parent argument is sent to the QWidget constructor.

    \sa setText(), setMaxLength()
*/
QLineEdit::QLineEdit(QWidget* parent)
    : QWidget(*new QLineEditPrivate, parent,0)
{
    Q_D(QLineEdit);
    d->init(QString());
}

/*!
    Constructs a line edit containing the text \a contents.

    The cursor position is set to the end of the line and the maximum
    text length to 32767 characters.

    The \a parent and argument is sent to the QWidget
    constructor.

    \sa text(), setMaxLength()
*/
QLineEdit::QLineEdit(const QString& contents, QWidget* parent)
    : QWidget(*new QLineEditPrivate, parent, 0)
{
    Q_D(QLineEdit);
    d->init(contents);
}


#ifdef QT3_SUPPORT
/*!
    Constructs a line edit with no text.

    The maximum text length is set to 32767 characters.

    The \a parent and \a name arguments are sent to the QWidget constructor.

    \sa setText(), setMaxLength()
*/
QLineEdit::QLineEdit(QWidget* parent, const char* name)
    : QWidget(*new QLineEditPrivate, parent,0)
{
    Q_D(QLineEdit);
    setObjectName(QString::fromAscii(name));
    d->init(QString());
}

/*!
    Constructs a line edit containing the text \a contents.

    The cursor position is set to the end of the line and the maximum
    text length to 32767 characters.

    The \a parent and \a name arguments are sent to the QWidget
    constructor.

    \sa text(), setMaxLength()
*/

QLineEdit::QLineEdit(const QString& contents, QWidget* parent, const char* name)
    : QWidget(*new QLineEditPrivate, parent, 0)
{
    Q_D(QLineEdit);
    setObjectName(QString::fromAscii(name));
    d->init(contents);
}

/*!
    Constructs a line edit with an input \a inputMask and the text \a
    contents.

    The cursor position is set to the end of the line and the maximum
    text length is set to the length of the mask (the number of mask
    characters and separators).

    The \a parent and \a name arguments are sent to the QWidget
    constructor.

    \sa setMask() text()
*/
QLineEdit::QLineEdit(const QString& contents, const QString &inputMask, QWidget* parent, const char* name)
    : QWidget(*new QLineEditPrivate, parent, 0)
{
    Q_D(QLineEdit);
    setObjectName(QString::fromAscii(name));
    d->init(contents);
    d->control->setInputMask(inputMask);
    d->control->moveCursor(d->control->nextMaskBlank(contents.length()));
}
#endif

/*!
    Destroys the line edit.
*/

QLineEdit::~QLineEdit()
{
}


/*!
    \property QLineEdit::text
    \brief the line edit's text

    Setting this property clears the selection, clears the undo/redo
    history, moves the cursor to the end of the line and resets the
    \l modified property to false. The text is not validated when
    inserted with setText().

    The text is truncated to maxLength() length.

    By default, this property contains an empty string.

    \sa insert(), clear()
*/
QString QLineEdit::text() const
{
    Q_D(const QLineEdit);
    return d->control->text();
}

void QLineEdit::setText(const QString& text)
{
    Q_D(QLineEdit);
    d->control->setText(text);
}

/*!
    \since 4.7

    \property QLineEdit::placeholderText
    \brief the line edit's placeholder text

    Setting this property makes the line edit display a grayed-out
    placeholder text as long as the text() is empty and the widget doesn't
    have focus.

    By default, this property contains an empty string.

    \sa text()
*/
QString QLineEdit::placeholderText() const
{
    Q_D(const QLineEdit);
    return d->placeholderText;
}

void QLineEdit::setPlaceholderText(const QString& placeholderText)
{
    Q_D(QLineEdit);
    if (d->placeholderText != placeholderText) {
        d->placeholderText = placeholderText;
        if (!hasFocus())
            update();
    }
}

/*!
    \property QLineEdit::displayText
    \brief the displayed text

    If \l echoMode is \l Normal this returns the same as text(); if
    \l EchoMode is \l Password or \l PasswordEchoOnEdit it returns a string of asterisks
    text().length() characters long, e.g. "******"; if \l EchoMode is
    \l NoEcho returns an empty string, "".

    By default, this property contains an empty string.

    \sa setEchoMode() text() EchoMode
*/

QString QLineEdit::displayText() const
{
    Q_D(const QLineEdit);
    return d->control->displayText();
}


/*!
    \property QLineEdit::maxLength
    \brief the maximum permitted length of the text

    If the text is too long, it is truncated at the limit.

    If truncation occurs any selected text will be unselected, the
    cursor position is set to 0 and the first part of the string is
    shown.

    If the line edit has an input mask, the mask defines the maximum
    string length.

    By default, this property contains a value of 32767.

    \sa inputMask
*/

int QLineEdit::maxLength() const
{
    Q_D(const QLineEdit);
    return d->control->maxLength();
}

void QLineEdit::setMaxLength(int maxLength)
{
    Q_D(QLineEdit);
    d->control->setMaxLength(maxLength);
}

/*!
    \property QLineEdit::frame
    \brief whether the line edit draws itself with a frame

    If enabled (the default) the line edit draws itself inside a
    frame, otherwise the line edit draws itself without any frame.
*/
bool QLineEdit::hasFrame() const
{
    Q_D(const QLineEdit);
    return d->frame;
}


void QLineEdit::setFrame(bool enable)
{
    Q_D(QLineEdit);
    d->frame = enable;
    update();
    updateGeometry();
}


/*!
    \enum QLineEdit::EchoMode

    This enum type describes how a line edit should display its
    contents.

    \value Normal   Display characters as they are entered. This is the
                    default.
    \value NoEcho   Do not display anything. This may be appropriate
                    for passwords where even the length of the
                    password should be kept secret.
    \value Password  Display asterisks instead of the characters
                    actually entered.
    \value PasswordEchoOnEdit Display characters as they are entered
                    while editing otherwise display asterisks.

    \sa setEchoMode() echoMode()
*/


/*!
    \property QLineEdit::echoMode
    \brief the line edit's echo mode

    The echo mode determines how the text entered in the line edit is
    displayed (or echoed) to the user.

    The most common setting is \l Normal, in which the text entered by the
    user is displayed verbatim, but QLineEdit also supports modes that allow
    the entered text to be suppressed or obscured: these include \l NoEcho,
    \l Password and \l PasswordEchoOnEdit.

    The widget's display and the ability to copy or drag the text is
    affected by this setting.

    By default, this property is set to \l Normal.

    \sa EchoMode displayText()
*/

QLineEdit::EchoMode QLineEdit::echoMode() const
{
    Q_D(const QLineEdit);
    return (EchoMode) d->control->echoMode();
}

void QLineEdit::setEchoMode(EchoMode mode)
{
    Q_D(QLineEdit);
    if (mode == (EchoMode)d->control->echoMode())
        return;
    Qt::InputMethodHints imHints = inputMethodHints();
    if (mode == Password || mode == NoEcho) {
        imHints |= Qt::ImhHiddenText;
    } else {
        imHints &= ~Qt::ImhHiddenText;
    }
    if (mode != Normal) {
        imHints |= (Qt::ImhNoAutoUppercase | Qt::ImhNoPredictiveText);
    } else {
        imHints &= ~(Qt::ImhNoAutoUppercase | Qt::ImhNoPredictiveText);
    }
    setInputMethodHints(imHints);
    d->control->setEchoMode(mode);
    update();
#ifdef Q_WS_MAC
    if (hasFocus())
        qt_mac_secure_keyboard(mode == Password || mode == NoEcho);
#endif
}


#ifndef QT_NO_VALIDATOR
/*!
    Returns a pointer to the current input validator, or 0 if no
    validator has been set.

    \sa setValidator()
*/

const QValidator * QLineEdit::validator() const
{
    Q_D(const QLineEdit);
    return d->control->validator();
}

/*!
    Sets this line edit to only accept input that the validator, \a v,
    will accept. This allows you to place any arbitrary constraints on
    the text which may be entered.

    If \a v == 0, setValidator() removes the current input validator.
    The initial setting is to have no input validator (i.e. any input
    is accepted up to maxLength()).

    \sa validator() QIntValidator QDoubleValidator QRegExpValidator
*/

void QLineEdit::setValidator(const QValidator *v)
{
    Q_D(QLineEdit);
    d->control->setValidator(v);
}
#endif // QT_NO_VALIDATOR

#ifndef QT_NO_COMPLETER
/*!
    \since 4.2

    Sets this line edit to provide auto completions from the completer, \a c.
    The completion mode is set using QCompleter::setCompletionMode().

    To use a QCompleter with a QValidator or QLineEdit::inputMask, you need to
    ensure that the model provided to QCompleter contains valid entries. You can
    use the QSortFilterProxyModel to ensure that the QCompleter's model contains
    only valid entries.

    If \a c == 0, setCompleter() removes the current completer, effectively
    disabling auto completion.

    \sa QCompleter
*/
void QLineEdit::setCompleter(QCompleter *c)
{
    Q_D(QLineEdit);
    if (c == d->control->completer())
        return;
    if (d->control->completer()) {
        disconnect(d->control->completer(), 0, this, 0);
        d->control->completer()->setWidget(0);
        if (d->control->completer()->parent() == this)
            delete d->control->completer();
    }
    d->control->setCompleter(c);
    if (!c)
        return;
    if (c->widget() == 0)
        c->setWidget(this);
    if (hasFocus()) {
        QObject::connect(d->control->completer(), SIGNAL(activated(QString)),
                         this, SLOT(setText(QString)));
        QObject::connect(d->control->completer(), SIGNAL(highlighted(QString)),
                         this, SLOT(_q_completionHighlighted(QString)));
    }
}

/*!
    \since 4.2

    Returns the current QCompleter that provides completions.
*/
QCompleter *QLineEdit::completer() const
{
    Q_D(const QLineEdit);
    return d->control->completer();
}

#endif // QT_NO_COMPLETER

/*!
    Returns a recommended size for the widget.

    The width returned, in pixels, is usually enough for about 15 to
    20 characters.
*/

QSize QLineEdit::sizeHint() const
{
    Q_D(const QLineEdit);
    ensurePolished();
    QFontMetrics fm(font());
    int h = qMax(fm.height(), 14) + 2*d->verticalMargin
            + d->topTextMargin + d->bottomTextMargin
            + d->topmargin + d->bottommargin;
    int w = fm.width(QLatin1Char('x')) * 17 + 2*d->horizontalMargin
            + d->leftTextMargin + d->rightTextMargin
            + d->leftmargin + d->rightmargin; // "some"
    QStyleOptionFrameV2 opt;
    initStyleOption(&opt);
    return (style()->sizeFromContents(QStyle::CT_LineEdit, &opt, QSize(w, h).
                                      expandedTo(QApplication::globalStrut()), this));
}


/*!
    Returns a minimum size for the line edit.

    The width returned is enough for at least one character.
*/

QSize QLineEdit::minimumSizeHint() const
{
    Q_D(const QLineEdit);
    ensurePolished();
    QFontMetrics fm = fontMetrics();
    int h = fm.height() + qMax(2*d->verticalMargin, fm.leading())
            + d->topmargin + d->bottommargin;
    int w = fm.maxWidth() + d->leftmargin + d->rightmargin;
    QStyleOptionFrameV2 opt;
    initStyleOption(&opt);
    return (style()->sizeFromContents(QStyle::CT_LineEdit, &opt, QSize(w, h).
                                      expandedTo(QApplication::globalStrut()), this));
}


/*!
    \property QLineEdit::cursorPosition
    \brief the current cursor position for this line edit

    Setting the cursor position causes a repaint when appropriate.

    By default, this property contains a value of 0.
*/

int QLineEdit::cursorPosition() const
{
    Q_D(const QLineEdit);
    return d->control->cursorPosition();
}

void QLineEdit::setCursorPosition(int pos)
{
    Q_D(QLineEdit);
    d->control->setCursorPosition(pos);
}

/*!
    Returns the cursor position under the point \a pos.
*/
// ### What should this do if the point is outside of contentsRect? Currently returns 0.
int QLineEdit::cursorPositionAt(const QPoint &pos)
{
    Q_D(QLineEdit);
    return d->xToPos(pos.x());
}


#ifdef QT3_SUPPORT
/*! \obsolete

    Use setText(), setCursorPosition() and setSelection() instead.
*/
bool QLineEdit::validateAndSet(const QString &newText, int newPos,
                                 int newMarkAnchor, int newMarkDrag)
{
    // The suggested functions above in the docs don't seem to validate,
    // below code tries to mimic previous behaviour.
    QString oldText = text();
    setText(newText);
    if(!hasAcceptableInput()){
        setText(oldText);
        return false;
    }
    int selstart = qMin(newMarkAnchor, newMarkDrag);
    int sellength = qAbs(newMarkAnchor - newMarkDrag);
    if (selstart == newPos) {
        selstart = qMax(newMarkAnchor, newMarkDrag);
        sellength = -sellength;
    }
    //setSelection also set the position
    setSelection(selstart, sellength);
    return true;
}
#endif //QT3_SUPPORT

/*!
    \property QLineEdit::alignment
    \brief the alignment of the line edit

    Both horizontal and vertical alignment is allowed here, Qt::AlignJustify
    will map to Qt::AlignLeft.

    By default, this property contains a combination of Qt::AlignLeft and Qt::AlignVCenter.

    \sa Qt::Alignment
*/

Qt::Alignment QLineEdit::alignment() const
{
    Q_D(const QLineEdit);
    return QFlag(d->alignment);
}

void QLineEdit::setAlignment(Qt::Alignment alignment)
{
    Q_D(QLineEdit);
    d->alignment = alignment;
    update();
}


/*!
    Moves the cursor forward \a steps characters. If \a mark is true
    each character moved over is added to the selection; if \a mark is
    false the selection is cleared.

    \sa cursorBackward()
*/

void QLineEdit::cursorForward(bool mark, int steps)
{
    Q_D(QLineEdit);
    d->control->cursorForward(mark, steps);
}


/*!
    Moves the cursor back \a steps characters. If \a mark is true each
    character moved over is added to the selection; if \a mark is
    false the selection is cleared.

    \sa cursorForward()
*/
void QLineEdit::cursorBackward(bool mark, int steps)
{
    cursorForward(mark, -steps);
}

/*!
    Moves the cursor one word forward. If \a mark is true, the word is
    also selected.

    \sa cursorWordBackward()
*/
void QLineEdit::cursorWordForward(bool mark)
{
    Q_D(QLineEdit);
    d->control->cursorWordForward(mark);
}

/*!
    Moves the cursor one word backward. If \a mark is true, the word
    is also selected.

    \sa cursorWordForward()
*/

void QLineEdit::cursorWordBackward(bool mark)
{
    Q_D(QLineEdit);
    d->control->cursorWordBackward(mark);
}


/*!
    If no text is selected, deletes the character to the left of the
    text cursor and moves the cursor one position to the left. If any
    text is selected, the cursor is moved to the beginning of the
    selected text and the selected text is deleted.

    \sa del()
*/
void QLineEdit::backspace()
{
    Q_D(QLineEdit);
    d->control->backspace();
}

/*!
    If no text is selected, deletes the character to the right of the
    text cursor. If any text is selected, the cursor is moved to the
    beginning of the selected text and the selected text is deleted.

    \sa backspace()
*/

void QLineEdit::del()
{
    Q_D(QLineEdit);
    d->control->del();
}

/*!
    Moves the text cursor to the beginning of the line unless it is
    already there. If \a mark is true, text is selected towards the
    first position; otherwise, any selected text is unselected if the
    cursor is moved.

    \sa end()
*/

void QLineEdit::home(bool mark)
{
    Q_D(QLineEdit);
    d->control->home(mark);
}

/*!
    Moves the text cursor to the end of the line unless it is already
    there. If \a mark is true, text is selected towards the last
    position; otherwise, any selected text is unselected if the cursor
    is moved.

    \sa home()
*/

void QLineEdit::end(bool mark)
{
    Q_D(QLineEdit);
    d->control->end(mark);
}


/*!
    \property QLineEdit::modified
    \brief whether the line edit's contents has been modified by the user

    The modified flag is never read by QLineEdit; it has a default value
    of false and is changed to true whenever the user changes the line
    edit's contents.

    This is useful for things that need to provide a default value but
    do not start out knowing what the default should be (perhaps it
    depends on other fields on the form). Start the line edit without
    the best default, and when the default is known, if modified()
    returns false (the user hasn't entered any text), insert the
    default value.

    Calling setText() resets the modified flag to false.
*/

bool QLineEdit::isModified() const
{
    Q_D(const QLineEdit);
    return d->control->isModified();
}

void QLineEdit::setModified(bool modified)
{
    Q_D(QLineEdit);
    d->control->setModified(modified);
}


/*!\fn QLineEdit::clearModified()

Use setModified(false) instead.

    \sa isModified()
*/


/*!
    \property QLineEdit::hasSelectedText
    \brief whether there is any text selected

    hasSelectedText() returns true if some or all of the text has been
    selected by the user; otherwise returns false.

    By default, this property is false.

    \sa selectedText()
*/


bool QLineEdit::hasSelectedText() const
{
    Q_D(const QLineEdit);
    return d->control->hasSelectedText();
}

/*!
    \property QLineEdit::selectedText
    \brief the selected text

    If there is no selected text this property's value is
    an empty string.

    By default, this property contains an empty string.

    \sa hasSelectedText()
*/

QString QLineEdit::selectedText() const
{
    Q_D(const QLineEdit);
    return d->control->selectedText();
}

/*!
    selectionStart() returns the index of the first selected character in the
    line edit or -1 if no text is selected.

    \sa selectedText()
*/

int QLineEdit::selectionStart() const
{
    Q_D(const QLineEdit);
    return d->control->selectionStart();
}


#ifdef QT3_SUPPORT

/*!
    \fn void QLineEdit::lostFocus()

    This signal is emitted when the line edit has lost focus.

    Use editingFinished() instead
    \sa editingFinished(), returnPressed()
*/

/*!
    Use isModified() instead.
*/
bool QLineEdit::edited() const { return isModified(); }
/*!
    Use setModified()  or setText().
*/
void QLineEdit::setEdited(bool on) { setModified(on); }

/*!
    There exists no equivalent functionality in Qt 4.
*/
int QLineEdit::characterAt(int xpos, QChar *chr) const
{
    Q_D(const QLineEdit);
    int pos = d->xToPos(xpos + contentsRect().x() - d->hscroll + d->horizontalMargin);
    QString txt = d->control->text();
    if (chr && pos < (int) txt.length())
        *chr = txt.at(pos);
    return pos;

}

/*!
    Use selectedText() and selectionStart() instead.
*/
bool QLineEdit::getSelection(int *start, int *end)
{
    Q_D(QLineEdit);
    if (d->control->hasSelectedText() && start && end) {
        *start = selectionStart();
        *end = *start + selectedText().length();
        return true;
    }
    return false;
}
#endif


/*!
    Selects text from position \a start and for \a length characters.
    Negative lengths are allowed.

    \sa deselect() selectAll() selectedText()
*/

void QLineEdit::setSelection(int start, int length)
{
    Q_D(QLineEdit);
    if (start < 0 || start > (int)d->control->text().length()) {
        qWarning("QLineEdit::setSelection: Invalid start position (%d)", start);
        return;
    }

    d->control->setSelection(start, length);

    if (d->control->hasSelectedText()){
        QStyleOptionFrameV2 opt;
        initStyleOption(&opt);
        if (!style()->styleHint(QStyle::SH_BlinkCursorWhenTextSelected, &opt, this))
            d->setCursorVisible(false);
    }
}


/*!
    \property QLineEdit::undoAvailable
    \brief whether undo is available

    Undo becomes available once the user has modified the text in the line edit.

    By default, this property is false.
*/

bool QLineEdit::isUndoAvailable() const
{
    Q_D(const QLineEdit);
    return d->control->isUndoAvailable();
}

/*!
    \property QLineEdit::redoAvailable
    \brief whether redo is available

    Redo becomes available once the user has performed one or more undo operations
    on text in the line edit.

    By default, this property is false.
*/

bool QLineEdit::isRedoAvailable() const
{
    Q_D(const QLineEdit);
    return d->control->isRedoAvailable();
}

/*!
    \property QLineEdit::dragEnabled
    \brief whether the lineedit starts a drag if the user presses and
    moves the mouse on some selected text

    Dragging is disabled by default.
*/

bool QLineEdit::dragEnabled() const
{
    Q_D(const QLineEdit);
    return d->dragEnabled;
}

void QLineEdit::setDragEnabled(bool b)
{
    Q_D(QLineEdit);
    d->dragEnabled = b;
}


/*!
  \property QLineEdit::cursorMoveStyle
  \brief the movement style of cursor in this line edit
  \since 4.8

  When this property is set to Qt::VisualMoveStyle, the line edit will use visual
  movement style. Pressing the left arrow key will always cause the cursor to move
  left, regardless of the text's writing direction. The same behavior applies to
  right arrow key.

  When the property is Qt::LogicalMoveStyle (the default), within a LTR text block,
  increase cursor position when pressing left arrow key, decrease cursor position
  when pressing the right arrow key. If the text block is right to left, the opposite
  behavior applies.
*/

/*!
    \since 4.8

    Returns the movement style for the cursor in the line edit.
*/
Qt::CursorMoveStyle QLineEdit::cursorMoveStyle() const
{
    Q_D(const QLineEdit);
    return d->control->cursorMoveStyle();
}

/*!
    \since 4.8

    Sets the movement style for the cursor in the line edit to the given
    \a style.
*/
void QLineEdit::setCursorMoveStyle(Qt::CursorMoveStyle style)
{
    Q_D(QLineEdit);
    d->control->setCursorMoveStyle(style);
}

/*!
    \property QLineEdit::acceptableInput
    \brief whether the input satisfies the inputMask and the
    validator.

    By default, this property is true.

    \sa setInputMask(), setValidator()
*/
bool QLineEdit::hasAcceptableInput() const
{
    Q_D(const QLineEdit);
    return d->control->hasAcceptableInput();
}

/*!
    Sets the margins around the text inside the frame to have the
    sizes \a left, \a top, \a right, and \a bottom.
    \since 4.5

    See also getTextMargins().
*/
void QLineEdit::setTextMargins(int left, int top, int right, int bottom)
{
    Q_D(QLineEdit);
    d->leftTextMargin = left;
    d->topTextMargin = top;
    d->rightTextMargin = right;
    d->bottomTextMargin = bottom;
    updateGeometry();
    update();
}

/*!
    \since 4.6
    Sets the \a margins around the text inside the frame.

    See also textMargins().
*/
void QLineEdit::setTextMargins(const QMargins &margins)
{
    setTextMargins(margins.left(), margins.top(), margins.right(), margins.bottom());
}

/*!
    Returns the widget's text margins for \a left, \a top, \a right, and \a bottom.
    \since 4.5

    \sa setTextMargins()
*/
void QLineEdit::getTextMargins(int *left, int *top, int *right, int *bottom) const
{
    Q_D(const QLineEdit);
    if (left)
        *left = d->leftTextMargin;
    if (top)
        *top = d->topTextMargin;
    if (right)
        *right = d->rightTextMargin;
    if (bottom)
        *bottom = d->bottomTextMargin;
}

/*!
    \since 4.6
    Returns the widget's text margins.

    \sa setTextMargins()
*/
QMargins QLineEdit::textMargins() const
{
    Q_D(const QLineEdit);
    return QMargins(d->leftTextMargin, d->topTextMargin, d->rightTextMargin, d->bottomTextMargin);
}

/*!
    \property QLineEdit::inputMask
    \brief The validation input mask

    If no mask is set, inputMask() returns an empty string.

    Sets the QLineEdit's validation mask. Validators can be used
    instead of, or in conjunction with masks; see setValidator().

    Unset the mask and return to normal QLineEdit operation by passing
    an empty string ("") or just calling setInputMask() with no
    arguments.

    The table below shows the characters that can be used in an input mask.
    A space character, the default character for a blank, is needed for cases
    where a character is \e{permitted but not required}.

    \table
    \header \i Character \i Meaning
    \row \i \c A \i ASCII alphabetic character required. A-Z, a-z.
    \row \i \c a \i ASCII alphabetic character permitted but not required.
    \row \i \c N \i ASCII alphanumeric character required. A-Z, a-z, 0-9.
    \row \i \c n \i ASCII alphanumeric character permitted but not required.
    \row \i \c X \i Any character required.
    \row \i \c x \i Any character permitted but not required.
    \row \i \c 9 \i ASCII digit required. 0-9.
    \row \i \c 0 \i ASCII digit permitted but not required.
    \row \i \c D \i ASCII digit required. 1-9.
    \row \i \c d \i ASCII digit permitted but not required (1-9).
    \row \i \c # \i ASCII digit or plus/minus sign permitted but not required.
    \row \i \c H \i Hexadecimal character required. A-F, a-f, 0-9.
    \row \i \c h \i Hexadecimal character permitted but not required.
    \row \i \c B \i Binary character required. 0-1.
    \row \i \c b \i Binary character permitted but not required.
    \row \i \c > \i All following alphabetic characters are uppercased.
    \row \i \c < \i All following alphabetic characters are lowercased.
    \row \i \c ! \i Switch off case conversion.
    \row \i \tt{\\} \i Use \tt{\\} to escape the special
                           characters listed above to use them as
                           separators.
    \endtable

    The mask consists of a string of mask characters and separators,
    optionally followed by a semicolon and the character used for
    blanks. The blank characters are always removed from the text
    after editing.

    Examples:
    \table
    \header \i Mask \i Notes
    \row \i \c 000.000.000.000;_ \i IP address; blanks are \c{_}.
    \row \i \c HH:HH:HH:HH:HH:HH;_ \i MAC address
    \row \i \c 0000-00-00 \i ISO Date; blanks are \c space
    \row \i \c >AAAAA-AAAAA-AAAAA-AAAAA-AAAAA;# \i License number;
    blanks are \c - and all (alphabetic) characters are converted to
    uppercase.
    \endtable

    To get range control (e.g., for an IP address) use masks together
    with \link setValidator() validators\endlink.

    \sa maxLength
*/
QString QLineEdit::inputMask() const
{
    Q_D(const QLineEdit);
    return d->control->inputMask();
}

void QLineEdit::setInputMask(const QString &inputMask)
{
    Q_D(QLineEdit);
    d->control->setInputMask(inputMask);
}

/*!
    Selects all the text (i.e. highlights it) and moves the cursor to
    the end. This is useful when a default value has been inserted
    because if the user types before clicking on the widget, the
    selected text will be deleted.

    \sa setSelection() deselect()
*/

void QLineEdit::selectAll()
{
    Q_D(QLineEdit);
    d->control->selectAll();
}

/*!
    Deselects any selected text.

    \sa setSelection() selectAll()
*/

void QLineEdit::deselect()
{
    Q_D(QLineEdit);
    d->control->deselect();
}


/*!
    Deletes any selected text, inserts \a newText, and validates the
    result. If it is valid, it sets it as the new contents of the line
    edit.

    \sa setText(), clear()
*/
void QLineEdit::insert(const QString &newText)
{
//     q->resetInputContext(); //#### FIX ME IN QT
    Q_D(QLineEdit);
    d->control->insert(newText);
}

/*!
    Clears the contents of the line edit.

    \sa setText(), insert()
*/
void QLineEdit::clear()
{
    Q_D(QLineEdit);
    resetInputContext();
    d->control->clear();
}

/*!
    Undoes the last operation if undo is \link
    QLineEdit::undoAvailable available\endlink. Deselects any current
    selection, and updates the selection start to the current cursor
    position.
*/
void QLineEdit::undo()
{
    Q_D(QLineEdit);
    resetInputContext();
    d->control->undo();
}

/*!
    Redoes the last operation if redo is \link
    QLineEdit::redoAvailable available\endlink.
*/
void QLineEdit::redo()
{
    Q_D(QLineEdit);
    resetInputContext();
    d->control->redo();
}


/*!
    \property QLineEdit::readOnly
    \brief whether the line edit is read only.

    In read-only mode, the user can still copy the text to the
    clipboard, or drag and drop the text (if echoMode() is \l Normal),
    but cannot edit it.

    QLineEdit does not show a cursor in read-only mode.

    By default, this property is false.

    \sa setEnabled()
*/

bool QLineEdit::isReadOnly() const
{
    Q_D(const QLineEdit);
    return d->control->isReadOnly();
}

void QLineEdit::setReadOnly(bool enable)
{
    Q_D(QLineEdit);
    if (d->control->isReadOnly() != enable) {
        d->control->setReadOnly(enable);
        setAttribute(Qt::WA_MacShowFocusRect, !enable);
        setAttribute(Qt::WA_InputMethodEnabled, d->shouldEnableInputMethod());
#ifndef QT_NO_CURSOR
        setCursor(enable ? Qt::ArrowCursor : Qt::IBeamCursor);
#endif
        update();
    }
}


#ifndef QT_NO_CLIPBOARD
/*!
    Copies the selected text to the clipboard and deletes it, if there
    is any, and if echoMode() is \l Normal.

    If the current validator disallows deleting the selected text,
    cut() will copy without deleting.

    \sa copy() paste() setValidator()
*/

void QLineEdit::cut()
{
    if (hasSelectedText()) {
        copy();
        del();
    }
}


/*!
    Copies the selected text to the clipboard, if there is any, and if
    echoMode() is \l Normal.

    \sa cut() paste()
*/

void QLineEdit::copy() const
{
    Q_D(const QLineEdit);
    d->control->copy();
}

/*!
    Inserts the clipboard's text at the cursor position, deleting any
    selected text, providing the line edit is not \link
    QLineEdit::readOnly read-only\endlink.

    If the end result would not be acceptable to the current
    \link setValidator() validator\endlink, nothing happens.

    \sa copy() cut()
*/

void QLineEdit::paste()
{
    Q_D(QLineEdit);
    d->control->paste();
}

#endif // !QT_NO_CLIPBOARD

/*! \reimp
*/
bool QLineEdit::event(QEvent * e)
{
    Q_D(QLineEdit);
    if (e->type() == QEvent::Timer) {
        // should be timerEvent, is here for binary compatibility
        int timerId = ((QTimerEvent*)e)->timerId();
        if (false) {
#ifndef QT_NO_DRAGANDDROP
        } else if (timerId == d->dndTimer.timerId()) {
            d->drag();
#endif
        }
        else if (timerId == d->tripleClickTimer.timerId())
            d->tripleClickTimer.stop();
    } else if (e->type() == QEvent::ContextMenu) {
#ifndef QT_NO_IM
        if (d->control->composeMode())
            return true;
#endif
        //d->separate();
    } else if (e->type() == QEvent::WindowActivate) {
        QTimer::singleShot(0, this, SLOT(_q_handleWindowActivate()));
    }else if(e->type() == QEvent::ShortcutOverride){
        d->control->processEvent(e);
    } else if (e->type() == QEvent::KeyRelease) {
        d->control->setCursorBlinkPeriod(QApplication::cursorFlashTime());
    } else if (e->type() == QEvent::Show) {
        //In order to get the cursor blinking if QComboBox::setEditable is called when the combobox has focus
        if (hasFocus()) {
            d->control->setCursorBlinkPeriod(QApplication::cursorFlashTime());
            QStyleOptionFrameV2 opt;
            initStyleOption(&opt);
            if ((!hasSelectedText() && d->control->preeditAreaText().isEmpty())
                || style()->styleHint(QStyle::SH_BlinkCursorWhenTextSelected, &opt, this))
                d->setCursorVisible(true);
        }
    }

#ifdef QT_KEYPAD_NAVIGATION
    if (QApplication::keypadNavigationEnabled()) {
        if (e->type() == QEvent::EnterEditFocus) {
            end(false);
            d->setCursorVisible(true);
            d->control->setCursorBlinkPeriod(QApplication::cursorFlashTime());
        } else if (e->type() == QEvent::LeaveEditFocus) {
            d->setCursorVisible(false);
            d->control->setCursorBlinkPeriod(0);
            if (d->control->hasAcceptableInput() || d->control->fixup())
                emit editingFinished();
        }
    }
#endif
    return QWidget::event(e);
}

/*! \reimp
*/
void QLineEdit::mousePressEvent(QMouseEvent* e)
{
    Q_D(QLineEdit);
    if (d->sendMouseEventToInputContext(e))
        return;
    if (e->button() == Qt::RightButton)
        return;
#ifdef QT_KEYPAD_NAVIGATION
    if (QApplication::keypadNavigationEnabled() && !hasEditFocus()) {
        setEditFocus(true);
        // Get the completion list to pop up.
        if (d->control->completer())
            d->control->completer()->complete();
    }
#endif
    if (d->tripleClickTimer.isActive() && (e->pos() - d->tripleClick).manhattanLength() <
         QApplication::startDragDistance()) {
        selectAll();
        return;
    }
    bool mark = e->modifiers() & Qt::ShiftModifier;
    int cursor = d->xToPos(e->pos().x());
#ifndef QT_NO_DRAGANDDROP
    if (!mark && d->dragEnabled && d->control->echoMode() == Normal &&
         e->button() == Qt::LeftButton && d->control->inSelection(e->pos().x())) {
        d->dndPos = e->pos();
        if (!d->dndTimer.isActive())
            d->dndTimer.start(QApplication::startDragTime(), this);
    } else
#endif
    {
        d->control->moveCursor(cursor, mark);
    }
}

/*! \reimp
*/
void QLineEdit::mouseMoveEvent(QMouseEvent * e)
{
    Q_D(QLineEdit);
    if (d->sendMouseEventToInputContext(e))
        return;

    if (e->buttons() & Qt::LeftButton) {
#ifndef QT_NO_DRAGANDDROP
        if (d->dndTimer.isActive()) {
            if ((d->dndPos - e->pos()).manhattanLength() > QApplication::startDragDistance())
                d->drag();
        } else
#endif
        {
            d->control->moveCursor(d->xToPos(e->pos().x()), true);
        }
    }
}

/*! \reimp
*/
void QLineEdit::mouseReleaseEvent(QMouseEvent* e)
{
    Q_D(QLineEdit);
    if (d->sendMouseEventToInputContext(e))
        return;
#ifndef QT_NO_DRAGANDDROP
    if (e->button() == Qt::LeftButton) {
        if (d->dndTimer.isActive()) {
            d->dndTimer.stop();
            deselect();
            return;
        }
    }
#endif
#ifndef QT_NO_CLIPBOARD
    if (QApplication::clipboard()->supportsSelection()) {
        if (e->button() == Qt::LeftButton) {
            d->control->copy(QClipboard::Selection);
        } else if (!d->control->isReadOnly() && e->button() == Qt::MidButton) {
            deselect();
            insert(QApplication::clipboard()->text(QClipboard::Selection));
        }
    }
#endif

    if (!isReadOnly() && rect().contains(e->pos()))
        d->handleSoftwareInputPanel(e->button(), d->clickCausedFocus);
    d->clickCausedFocus = 0;
}

/*! \reimp
*/
void QLineEdit::mouseDoubleClickEvent(QMouseEvent* e)
{
    Q_D(QLineEdit);
    if (d->sendMouseEventToInputContext(e))
        return;
    if (e->button() == Qt::LeftButton) {
        d->control->selectWordAtPos(d->xToPos(e->pos().x()));
        d->tripleClickTimer.start(QApplication::doubleClickInterval(), this);
        d->tripleClick = e->pos();
    }
}

/*!
    \fn void  QLineEdit::returnPressed()

    This signal is emitted when the Return or Enter key is pressed.
    Note that if there is a validator() or inputMask() set on the line
    edit, the returnPressed() signal will only be emitted if the input
    follows the inputMask() and the validator() returns
    QValidator::Acceptable.
*/

/*!
    \fn void  QLineEdit::editingFinished()

    This signal is emitted when the Return or Enter key is pressed or
    the line edit loses focus. Note that if there is a validator() or
    inputMask() set on the line edit and enter/return is pressed, the
    editingFinished() signal will only be emitted if the input follows
    the inputMask() and the validator() returns QValidator::Acceptable.
*/

/*!
    Converts the given key press \a event into a line edit action.

    If Return or Enter is pressed and the current text is valid (or
    can be \link QValidator::fixup() made valid\endlink by the
    validator), the signal returnPressed() is emitted.

    The default key bindings are listed in the class's detailed
    description.
*/

void QLineEdit::keyPressEvent(QKeyEvent *event)
{
    Q_D(QLineEdit);
    #ifdef QT_KEYPAD_NAVIGATION
    bool select = false;
    switch (event->key()) {
        case Qt::Key_Select:
            if (QApplication::keypadNavigationEnabled()) {
                if (hasEditFocus()) {
                    setEditFocus(false);
                    if (d->control->completer() && d->control->completer()->popup()->isVisible())
                        d->control->completer()->popup()->hide();
                    select = true;
                }
            }
            break;
        case Qt::Key_Back:
        case Qt::Key_No:
            if (!QApplication::keypadNavigationEnabled() || !hasEditFocus()) {
                event->ignore();
                return;
            }
            break;
        default:
            if (QApplication::keypadNavigationEnabled()) {
                if (!hasEditFocus() && !(event->modifiers() & Qt::ControlModifier)) {
                    if (!event->text().isEmpty() && event->text().at(0).isPrint()
                        && !isReadOnly())
                        setEditFocus(true);
                    else {
                        event->ignore();
                        return;
                    }
                }
            }
    }



    if (QApplication::keypadNavigationEnabled() && !select && !hasEditFocus()) {
        setEditFocus(true);
        if (event->key() == Qt::Key_Select)
            return; // Just start. No action.
    }
#endif
    d->control->processKeyEvent(event);
    if (event->isAccepted()) {
        if (layoutDirection() != d->control->layoutDirection())
            setLayoutDirection(d->control->layoutDirection());
        d->control->setCursorBlinkPeriod(0);
    }
}

/*!
  \since 4.4

  Returns a rectangle that includes the lineedit cursor.
*/
QRect QLineEdit::cursorRect() const
{
    Q_D(const QLineEdit);
    return d->cursorRect();
}

/*! \reimp
 */
void QLineEdit::inputMethodEvent(QInputMethodEvent *e)
{
    Q_D(QLineEdit);
    if (d->control->isReadOnly()) {
        e->ignore();
        return;
    }

    if (echoMode() == PasswordEchoOnEdit && !d->control->passwordEchoEditing()) {
        // Clear the edit and reset to normal echo mode while entering input
        // method data; the echo mode switches back when the edit loses focus.
        // ### changes a public property, resets current content.
        d->updatePasswordEchoEditing(true);
        clear();
    }

#ifdef QT_KEYPAD_NAVIGATION
    // Focus in if currently in navigation focus on the widget
    // Only focus in on preedits, to allow input methods to
    // commit text as they focus out without interfering with focus
    if (QApplication::keypadNavigationEnabled()
        && hasFocus() && !hasEditFocus()
        && !e->preeditString().isEmpty())
        setEditFocus(true);
#endif

    d->control->processInputMethodEvent(e);

#ifndef QT_NO_COMPLETER
    if (!e->commitString().isEmpty())
        d->control->complete(Qt::Key_unknown);
#endif
}

/*!\reimp
*/
QVariant QLineEdit::inputMethodQuery(Qt::InputMethodQuery property) const
{
    Q_D(const QLineEdit);
    switch(property) {
    case Qt::ImMicroFocus:
        return d->cursorRect();
    case Qt::ImFont:
        return font();
    case Qt::ImCursorPosition:
        return QVariant(d->control->cursor());
    case Qt::ImSurroundingText:
        return QVariant(text());
    case Qt::ImCurrentSelection:
        return QVariant(selectedText());
    case Qt::ImMaximumTextLength:
        return QVariant(maxLength());
    case Qt::ImAnchorPosition:
        if (d->control->selectionStart() == d->control->selectionEnd())
            return QVariant(d->control->cursor());
        else if (d->control->selectionStart() == d->control->cursor())
            return QVariant(d->control->selectionEnd());
        else
            return QVariant(d->control->selectionStart());
    default:
        return QVariant();
    }
}

/*!\reimp
*/

void QLineEdit::focusInEvent(QFocusEvent *e)
{
    Q_D(QLineEdit);
    if (e->reason() == Qt::TabFocusReason ||
         e->reason() == Qt::BacktabFocusReason  ||
         e->reason() == Qt::ShortcutFocusReason) {
        if (!d->control->inputMask().isEmpty())
            d->control->moveCursor(d->control->nextMaskBlank(0));
        else if (!d->control->hasSelectedText())
            selectAll();
    } else if (e->reason() == Qt::MouseFocusReason) {
        d->clickCausedFocus = 1;
    }
#ifdef QT_KEYPAD_NAVIGATION
    if (!QApplication::keypadNavigationEnabled() || (hasEditFocus() && ( e->reason() == Qt::PopupFocusReason
#ifdef Q_OS_SYMBIAN
            || e->reason() == Qt::ActiveWindowFocusReason
#endif
            ))) {
#endif
    d->control->setCursorBlinkPeriod(QApplication::cursorFlashTime());
    QStyleOptionFrameV2 opt;
    initStyleOption(&opt);
    if((!hasSelectedText() && d->control->preeditAreaText().isEmpty())
       || style()->styleHint(QStyle::SH_BlinkCursorWhenTextSelected, &opt, this))
        d->setCursorVisible(true);
#ifdef Q_WS_MAC
    if (d->control->echoMode() == Password || d->control->echoMode() == NoEcho)
        qt_mac_secure_keyboard(true);
#endif
#ifdef QT_KEYPAD_NAVIGATION
        d->control->setCancelText(d->control->text());
    }
#endif
#ifndef QT_NO_COMPLETER
    if (d->control->completer()) {
        d->control->completer()->setWidget(this);
        QObject::connect(d->control->completer(), SIGNAL(activated(QString)),
                         this, SLOT(setText(QString)));
        QObject::connect(d->control->completer(), SIGNAL(highlighted(QString)),
                         this, SLOT(_q_completionHighlighted(QString)));
    }
#endif
    update();
}

/*!\reimp
*/

void QLineEdit::focusOutEvent(QFocusEvent *e)
{
    Q_D(QLineEdit);
    if (d->control->passwordEchoEditing()) {
        // Reset the echomode back to PasswordEchoOnEdit when the widget loses
        // focus.
        d->updatePasswordEchoEditing(false);
    }

    Qt::FocusReason reason = e->reason();
    if (reason != Qt::ActiveWindowFocusReason &&
        reason != Qt::PopupFocusReason)
        deselect();

    d->setCursorVisible(false);
    d->control->setCursorBlinkPeriod(0);
#ifdef QT_KEYPAD_NAVIGATION
    // editingFinished() is already emitted on LeaveEditFocus
    if (!QApplication::keypadNavigationEnabled())
#endif
    if (reason != Qt::PopupFocusReason
        || !(QApplication::activePopupWidget() && QApplication::activePopupWidget()->parentWidget() == this)) {
            if (hasAcceptableInput() || d->control->fixup())
                emit editingFinished();
#ifdef QT3_SUPPORT
        emit lostFocus();
#endif
    }
#ifdef Q_WS_MAC
    if (d->control->echoMode() == Password || d->control->echoMode() == NoEcho)
        qt_mac_secure_keyboard(false);
#endif
#ifdef QT_KEYPAD_NAVIGATION
    d->control->setCancelText(QString());
#endif
#ifndef QT_NO_COMPLETER
    if (d->control->completer()) {
        QObject::disconnect(d->control->completer(), 0, this, 0);
    }
#endif
    update();
}

/*!\reimp
*/
void QLineEdit::paintEvent(QPaintEvent *)
{
    Q_D(QLineEdit);
    QPainter p(this);

    QRect r = rect();
    QPalette pal = palette();

    QStyleOptionFrameV2 panel;
    initStyleOption(&panel);
    style()->drawPrimitive(QStyle::PE_PanelLineEdit, &panel, &p, this);
    r = style()->subElementRect(QStyle::SE_LineEditContents, &panel, this);
    r.setX(r.x() + d->leftTextMargin);
    r.setY(r.y() + d->topTextMargin);
    r.setRight(r.right() - d->rightTextMargin);
    r.setBottom(r.bottom() - d->bottomTextMargin);
    p.setClipRect(r);

    QFontMetrics fm = fontMetrics();
    Qt::Alignment va = QStyle::visualAlignment(d->control->layoutDirection(), QFlag(d->alignment));
    switch (va & Qt::AlignVertical_Mask) {
     case Qt::AlignBottom:
         d->vscroll = r.y() + r.height() - fm.height() - d->verticalMargin;
         break;
     case Qt::AlignTop:
         d->vscroll = r.y() + d->verticalMargin;
         break;
     default:
         //center
         d->vscroll = r.y() + (r.height() - fm.height() + 1) / 2;
         break;
    }
    QRect lineRect(r.x() + d->horizontalMargin, d->vscroll, r.width() - 2*d->horizontalMargin, fm.height());

    int minLB = qMax(0, -fm.minLeftBearing());
    int minRB = qMax(0, -fm.minRightBearing());

    if (d->control->text().isEmpty()) {
        if (!hasFocus() && !d->placeholderText.isEmpty()) {
            QColor col = pal.text().color();
            col.setAlpha(128);
            QPen oldpen = p.pen();
            p.setPen(col);
            lineRect.adjust(minLB, 0, 0, 0);
            QString elidedText = fm.elidedText(d->placeholderText, Qt::ElideRight, lineRect.width());
            p.drawText(lineRect, va, elidedText);
            p.setPen(oldpen);
            return;
        }
    }

    int cix = qRound(d->control->cursorToX());

    // horizontal scrolling. d->hscroll is the left indent from the beginning
    // of the text line to the left edge of lineRect. we update this value
    // depending on the delta from the last paint event; in effect this means
    // the below code handles all scrolling based on the textline (widthUsed,
    // minLB, minRB), the line edit rect (lineRect) and the cursor position
    // (cix).
    int widthUsed = qRound(d->control->naturalTextWidth()) + 1 + minRB;
    if ((minLB + widthUsed) <=  lineRect.width()) {
        // text fits in lineRect; use hscroll for alignment
        switch (va & ~(Qt::AlignAbsolute|Qt::AlignVertical_Mask)) {
        case Qt::AlignRight:
            d->hscroll = widthUsed - lineRect.width() + 1;
            break;
        case Qt::AlignHCenter:
            d->hscroll = (widthUsed - lineRect.width()) / 2;
            break;
        default:
            // Left
            d->hscroll = 0;
            break;
        }
        d->hscroll -= minLB;
    } else if (cix - d->hscroll >= lineRect.width()) {
        // text doesn't fit, cursor is to the right of lineRect (scroll right)
        d->hscroll = cix - lineRect.width() + 1;
    } else if (cix - d->hscroll < 0 && d->hscroll < widthUsed) {
        // text doesn't fit, cursor is to the left of lineRect (scroll left)
        d->hscroll = cix;
    } else if (widthUsed - d->hscroll < lineRect.width()) {
        // text doesn't fit, text document is to the left of lineRect; align
        // right
        d->hscroll = widthUsed - lineRect.width() + 1;
    } else {
        //in case the text is bigger than the lineedit, the hscroll can never be negative
        d->hscroll = qMax(0, d->hscroll);
    }

    // the y offset is there to keep the baseline constant in case we have script changes in the text.
    QPoint topLeft = lineRect.topLeft() - QPoint(d->hscroll, d->control->ascent() - fm.ascent());

    // draw text, selections and cursors
#ifndef QT_NO_STYLE_STYLESHEET
    if (QStyleSheetStyle* cssStyle = qobject_cast<QStyleSheetStyle*>(style())) {
        cssStyle->styleSheetPalette(this, &panel, &pal);
    }
#endif
    p.setPen(pal.text().color());

    int flags = QLineControl::DrawText;

#ifdef QT_KEYPAD_NAVIGATION
    if (!QApplication::keypadNavigationEnabled() || hasEditFocus())
#endif
    if (d->control->hasSelectedText() || (d->cursorVisible && !d->control->inputMask().isEmpty() && !d->control->isReadOnly())){
        flags |= QLineControl::DrawSelections;
        // Palette only used for selections/mask and may not be in sync
        if (d->control->palette() != pal
           || d->control->palette().currentColorGroup() != pal.currentColorGroup())
            d->control->setPalette(pal);
    }

    // Asian users see an IM selection text as cursor on candidate
    // selection phase of input method, so the ordinary cursor should be
    // invisible if we have a preedit string.
    if (d->cursorVisible && !d->control->isReadOnly())
        flags |= QLineControl::DrawCursor;

    d->control->setCursorWidth(style()->pixelMetric(QStyle::PM_TextCursorWidth));
    d->control->draw(&p, topLeft, r, flags);

}


#ifndef QT_NO_DRAGANDDROP
/*!\reimp
*/
void QLineEdit::dragMoveEvent(QDragMoveEvent *e)
{
    Q_D(QLineEdit);
    if (!d->control->isReadOnly() && e->mimeData()->hasFormat(QLatin1String("text/plain"))) {
        e->acceptProposedAction();
        d->control->moveCursor(d->xToPos(e->pos().x()), false);
        d->cursorVisible = true;
        update();
    }
}

/*!\reimp */
void QLineEdit::dragEnterEvent(QDragEnterEvent * e)
{
    QLineEdit::dragMoveEvent(e);
}

/*!\reimp */
void QLineEdit::dragLeaveEvent(QDragLeaveEvent *)
{
    Q_D(QLineEdit);
    if (d->cursorVisible) {
        d->cursorVisible = false;
        update();
    }
}

/*!\reimp */
void QLineEdit::dropEvent(QDropEvent* e)
{
    Q_D(QLineEdit);
    QString str = e->mimeData()->text();

    if (!str.isNull() && !d->control->isReadOnly()) {
        if (e->source() == this && e->dropAction() == Qt::CopyAction)
            deselect();
        int cursorPos = d->xToPos(e->pos().x());
        int selStart = cursorPos;
        int oldSelStart = d->control->selectionStart();
        int oldSelEnd = d->control->selectionEnd();
        d->control->moveCursor(cursorPos, false);
        d->cursorVisible = false;
        e->acceptProposedAction();
        insert(str);
        if (e->source() == this) {
            if (e->dropAction() == Qt::MoveAction) {
                if (selStart > oldSelStart && selStart <= oldSelEnd)
                    setSelection(oldSelStart, str.length());
                else if (selStart > oldSelEnd)
                    setSelection(selStart - str.length(), str.length());
                else
                    setSelection(selStart, str.length());
            } else {
                setSelection(selStart, str.length());
            }
        }
    } else {
        e->ignore();
        update();
    }
}

#endif // QT_NO_DRAGANDDROP

#ifndef QT_NO_CONTEXTMENU
/*!
    Shows the standard context menu created with
    createStandardContextMenu().

    If you do not want the line edit to have a context menu, you can set
    its \l contextMenuPolicy to Qt::NoContextMenu. If you want to
    customize the context menu, reimplement this function. If you want
    to extend the standard context menu, reimplement this function, call
    createStandardContextMenu() and extend the menu returned.

    \snippet doc/src/snippets/code/src_gui_widgets_qlineedit.cpp 0

    The \a event parameter is used to obtain the position where
    the mouse cursor was when the event was generated.

    \sa setContextMenuPolicy()
*/
void QLineEdit::contextMenuEvent(QContextMenuEvent *event)
{
    if (QMenu *menu = createStandardContextMenu()) {
        menu->setAttribute(Qt::WA_DeleteOnClose);
        menu->popup(event->globalPos());
    }
}

#if defined(Q_WS_WIN) || defined(Q_WS_X11)
    extern bool qt_use_rtl_extensions;
#endif

/*!  This function creates the standard context menu which is shown
        when the user clicks on the line edit with the right mouse
        button. It is called from the default contextMenuEvent() handler.
        The popup menu's ownership is transferred to the caller.
*/

QMenu *QLineEdit::createStandardContextMenu()
{
    Q_D(QLineEdit);
    QMenu *popup = new QMenu(this);
    popup->setObjectName(QLatin1String("qt_edit_menu"));
    QAction *action = 0;

    if (!isReadOnly()) {
        action = popup->addAction(QLineEdit::tr("&Undo") + ACCEL_KEY(QKeySequence::Undo));
        action->setEnabled(d->control->isUndoAvailable());
        connect(action, SIGNAL(triggered()), SLOT(undo()));

        action = popup->addAction(QLineEdit::tr("&Redo") + ACCEL_KEY(QKeySequence::Redo));
        action->setEnabled(d->control->isRedoAvailable());
        connect(action, SIGNAL(triggered()), SLOT(redo()));

        popup->addSeparator();
    }

#ifndef QT_NO_CLIPBOARD
    if (!isReadOnly()) {
        action = popup->addAction(QLineEdit::tr("Cu&t") + ACCEL_KEY(QKeySequence::Cut));
        action->setEnabled(!d->control->isReadOnly() && d->control->hasSelectedText()
                && d->control->echoMode() == QLineEdit::Normal);
        connect(action, SIGNAL(triggered()), SLOT(cut()));
    }

    action = popup->addAction(QLineEdit::tr("&Copy") + ACCEL_KEY(QKeySequence::Copy));
    action->setEnabled(d->control->hasSelectedText()
            && d->control->echoMode() == QLineEdit::Normal);
    connect(action, SIGNAL(triggered()), SLOT(copy()));

    if (!isReadOnly()) {
        action = popup->addAction(QLineEdit::tr("&Paste") + ACCEL_KEY(QKeySequence::Paste));
        action->setEnabled(!d->control->isReadOnly() && !QApplication::clipboard()->text().isEmpty());
        connect(action, SIGNAL(triggered()), SLOT(paste()));
    }
#endif

    if (!isReadOnly()) {
        action = popup->addAction(QLineEdit::tr("Delete"));
        action->setEnabled(!d->control->isReadOnly() && !d->control->text().isEmpty() && d->control->hasSelectedText());
        connect(action, SIGNAL(triggered()), d->control, SLOT(_q_deleteSelected()));
    }

    if (!popup->isEmpty())
        popup->addSeparator();

    action = popup->addAction(QLineEdit::tr("Select All") + ACCEL_KEY(QKeySequence::SelectAll));
    action->setEnabled(!d->control->text().isEmpty() && !d->control->allSelected());
    d->selectAllAction = action;
    connect(action, SIGNAL(triggered()), SLOT(selectAll()));

#if !defined(QT_NO_IM)
    QInputContext *qic = inputContext();
    if (qic) {
        QList<QAction *> imActions = qic->actions();
        for (int i = 0; i < imActions.size(); ++i)
            popup->addAction(imActions.at(i));
    }
#endif

#if defined(Q_WS_WIN) || defined(Q_WS_X11)
    if (!d->control->isReadOnly() && qt_use_rtl_extensions) {
#else
    if (!d->control->isReadOnly()) {
#endif
        popup->addSeparator();
        QUnicodeControlCharacterMenu *ctrlCharacterMenu = new QUnicodeControlCharacterMenu(this, popup);
        popup->addMenu(ctrlCharacterMenu);
    }
    return popup;
}
#endif // QT_NO_CONTEXTMENU

/*! \reimp */
void QLineEdit::changeEvent(QEvent *ev)
{
    Q_D(QLineEdit);
    switch(ev->type())
    {
    case QEvent::ActivationChange:
        if (!palette().isEqual(QPalette::Active, QPalette::Inactive))
            update();
        break;
    case QEvent::FontChange:
        d->control->setFont(font());
        break;
    case QEvent::StyleChange:
        {
            QStyleOptionFrameV2 opt;
            initStyleOption(&opt);
            d->control->setPasswordCharacter(style()->styleHint(QStyle::SH_LineEdit_PasswordCharacter, &opt, this));
        }
        update();
        break;
    default:
        break;
    }
    QWidget::changeEvent(ev);
}

/*!
    \fn void QLineEdit::repaintArea(int a, int b)

    Use update() instead.
*/

/*!
    \fn void QLineEdit::cursorLeft(bool mark, int steps)

    Use cursorForward() with a negative number of steps instead. For
    example, cursorForward(mark, -steps).
*/

/*!
    \fn void QLineEdit::cursorRight(bool mark, int steps)

    Use cursorForward() instead.
*/

/*!
    \fn bool QLineEdit::frame() const

    Use hasFrame() instead.
*/

/*!
    \fn void QLineEdit::clearValidator()

    Use setValidator(0) instead.
*/

/*!
    \fn bool QLineEdit::hasMarkedText() const

    Use hasSelectedText() instead.
*/

/*!
    \fn QString QLineEdit::markedText() const

    Use selectedText() instead.
*/

/*!
    \fn void QLineEdit::setFrameRect(QRect)
    \internal
*/

/*!
    \fn QRect QLineEdit::frameRect() const
    \internal
*/
/*!
    \enum QLineEdit::DummyFrame
    \internal

    \value Box
    \value Sunken
    \value Plain
    \value Raised
    \value MShadow
    \value NoFrame
    \value Panel
    \value StyledPanel
    \value HLine
    \value VLine
    \value GroupBoxPanel
    \value WinPanel
    \value ToolBarPanel
    \value MenuBarPanel
    \value PopupPanel
    \value LineEditPanel
    \value TabWidgetPanel
    \value MShape
*/

/*!
    \fn void QLineEdit::setFrameShadow(DummyFrame)
    \internal
*/

/*!
    \fn DummyFrame QLineEdit::frameShadow() const
    \internal
*/

/*!
    \fn void QLineEdit::setFrameShape(DummyFrame)
    \internal
*/

/*!
    \fn DummyFrame QLineEdit::frameShape() const
    \internal
*/

/*!
    \fn void QLineEdit::setFrameStyle(int)
    \internal
*/

/*!
    \fn int QLineEdit::frameStyle() const
    \internal
*/

/*!
    \fn int QLineEdit::frameWidth() const
    \internal
*/

/*!
    \fn void QLineEdit::setLineWidth(int)
    \internal
*/

/*!
    \fn int QLineEdit::lineWidth() const
    \internal
*/

/*!
    \fn void QLineEdit::setMargin(int margin)
    Sets the width of the margin around the contents of the widget to \a margin.

    Use QWidget::setContentsMargins() instead.
    \sa margin(), QWidget::setContentsMargins()
*/

/*!
    \fn int QLineEdit::margin() const
    Returns the width of the margin around the contents of the widget.

    Use QWidget::getContentsMargins() instead.
    \sa setMargin(), QWidget::getContentsMargins()
*/

/*!
    \fn void QLineEdit::setMidLineWidth(int)
    \internal
*/

/*!
    \fn int QLineEdit::midLineWidth() const
    \internal
*/

QT_END_NAMESPACE

#include "moc_qlineedit.cpp"

#endif // QT_NO_LINEEDIT
