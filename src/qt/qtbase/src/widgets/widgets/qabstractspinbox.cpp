/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtWidgets module of the Qt Toolkit.
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

#include <qplatformdefs.h>
#include <private/qabstractspinbox_p.h>
#include <private/qdatetimeparser_p.h>
#include <private/qlineedit_p.h>
#include <qabstractspinbox.h>

#ifndef QT_NO_SPINBOX

#include <qapplication.h>
#include <qstylehints.h>
#include <qclipboard.h>
#include <qdatetime.h>
#include <qdatetimeedit.h>
#include <qevent.h>
#include <qmenu.h>
#include <qpainter.h>
#include <qpalette.h>
#include <qstylepainter.h>
#include <qdebug.h>
#ifndef QT_NO_ACCESSIBILITY
# include <qaccessible.h>
#endif


//#define QABSTRACTSPINBOX_QSBDEBUG
#ifdef QABSTRACTSPINBOX_QSBDEBUG
#  define QASBDEBUG qDebug
#else
#  define QASBDEBUG if (false) qDebug
#endif

QT_BEGIN_NAMESPACE

/*!
    \class QAbstractSpinBox
    \brief The QAbstractSpinBox class provides a spinbox and a line edit to
    display values.

    \ingroup abstractwidgets
    \inmodule QtWidgets

    The class is designed as a common super class for widgets like
    QSpinBox, QDoubleSpinBox and QDateTimeEdit

    Here are the main properties of the class:

    \list 1

    \li \l text: The text that is displayed in the QAbstractSpinBox.

    \li \l alignment: The alignment of the text in the QAbstractSpinBox.

    \li \l wrapping: Whether the QAbstractSpinBox wraps from the
    minimum value to the maximum value and vica versa.

    \endlist

    QAbstractSpinBox provides a virtual stepBy() function that is
    called whenever the user triggers a step. This function takes an
    integer value to signify how many steps were taken. E.g. Pressing
    Qt::Key_Down will trigger a call to stepBy(-1).

    QAbstractSpinBox also provide a virtual function stepEnabled() to
    determine whether stepping up/down is allowed at any point. This
    function returns a bitset of StepEnabled.

    \sa QAbstractSlider, QSpinBox, QDoubleSpinBox, QDateTimeEdit,
        {Spin Boxes Example}
*/

/*!
    \enum QAbstractSpinBox::StepEnabledFlag

    \value StepNone
    \value StepUpEnabled
    \value StepDownEnabled
*/

/*!
  \fn void QAbstractSpinBox::editingFinished()

  This signal is emitted editing is finished. This happens when the
  spinbox loses focus and when enter is pressed.
*/

/*!
    Constructs an abstract spinbox with the given \a parent with default
    \l wrapping, and \l alignment properties.
*/

QAbstractSpinBox::QAbstractSpinBox(QWidget *parent)
    : QWidget(*new QAbstractSpinBoxPrivate, parent, 0)
{
    Q_D(QAbstractSpinBox);
    d->init();
}

/*!
    \internal
*/
QAbstractSpinBox::QAbstractSpinBox(QAbstractSpinBoxPrivate &dd, QWidget *parent)
    : QWidget(dd, parent, 0)
{
    Q_D(QAbstractSpinBox);
    d->init();
}

/*!
    Called when the QAbstractSpinBox is destroyed.
*/

QAbstractSpinBox::~QAbstractSpinBox()
{
}

/*!
    \enum QAbstractSpinBox::ButtonSymbols

    This enum type describes the symbols that can be displayed on the buttons
    in a spin box.

    \inlineimage qspinbox-updown.png
    \inlineimage qspinbox-plusminus.png

    \value UpDownArrows Little arrows in the classic style.
    \value PlusMinus \b{+} and \b{-} symbols.
    \value NoButtons Don't display buttons.

    \sa QAbstractSpinBox::buttonSymbols
*/

/*!
    \property QAbstractSpinBox::buttonSymbols

    \brief the current button symbol mode

    The possible values can be either \c UpDownArrows or \c PlusMinus.
    The default is \c UpDownArrows.

    Note that some styles might render PlusMinus and UpDownArrows
    identically.

    \sa ButtonSymbols
*/

QAbstractSpinBox::ButtonSymbols QAbstractSpinBox::buttonSymbols() const
{
    Q_D(const QAbstractSpinBox);
    return d->buttonSymbols;
}

void QAbstractSpinBox::setButtonSymbols(ButtonSymbols buttonSymbols)
{
    Q_D(QAbstractSpinBox);
    if (d->buttonSymbols != buttonSymbols) {
        d->buttonSymbols = buttonSymbols;
        d->updateEditFieldGeometry();
        update();
    }
}

/*!
    \property QAbstractSpinBox::text

    \brief the spin box's text, including any prefix and suffix

    There is no default text.
*/

QString QAbstractSpinBox::text() const
{
    return lineEdit()->displayText();
}


/*!
    \property QAbstractSpinBox::specialValueText
    \brief the special-value text

    If set, the spin box will display this text instead of a numeric
    value whenever the current value is equal to minimum(). Typical use
    is to indicate that this choice has a special (default) meaning.

    For example, if your spin box allows the user to choose a scale factor
    (or zoom level) for displaying an image, and your application is able
    to automatically choose one that will enable the image to fit completely
    within the display window, you can set up the spin box like this:

    \snippet widgets/spinboxes/window.cpp 3

    The user will then be able to choose a scale from 1% to 1000%
    or select "Auto" to leave it up to the application to choose. Your code
    must then interpret the spin box value of 0 as a request from the user
    to scale the image to fit inside the window.

    All values are displayed with the prefix and suffix (if set), \e
    except for the special value, which only shows the special value
    text. This special text is passed in the QSpinBox::valueChanged()
    signal that passes a QString.

    To turn off the special-value text display, call this function
    with an empty string. The default is no special-value text, i.e.
    the numeric value is shown as usual.

    If no special-value text is set, specialValueText() returns an
    empty string.
*/

QString QAbstractSpinBox::specialValueText() const
{
    Q_D(const QAbstractSpinBox);
    return d->specialValueText;
}

void QAbstractSpinBox::setSpecialValueText(const QString &specialValueText)
{
    Q_D(QAbstractSpinBox);

    d->specialValueText = specialValueText;
    d->cachedSizeHint = QSize(); // minimumSizeHint doesn't care about specialValueText
    d->clearCache();
    d->updateEdit();
}

/*!
    \property QAbstractSpinBox::wrapping

    \brief whether the spin box is circular.

    If wrapping is true stepping up from maximum() value will take you
    to the minimum() value and vica versa. Wrapping only make sense if
    you have minimum() and maximum() values set.

    \snippet code/src_gui_widgets_qabstractspinbox.cpp 0

    \sa QSpinBox::minimum(), QSpinBox::maximum()
*/

bool QAbstractSpinBox::wrapping() const
{
    Q_D(const QAbstractSpinBox);
    return d->wrapping;
}

void QAbstractSpinBox::setWrapping(bool wrapping)
{
    Q_D(QAbstractSpinBox);
    d->wrapping = wrapping;
}


/*!
    \property QAbstractSpinBox::readOnly
    \brief whether the spin box is read only.

    In read-only mode, the user can still copy the text to the
    clipboard, or drag and drop the text;
    but cannot edit it.

    The QLineEdit in the QAbstractSpinBox does not show a cursor in
    read-only mode.

    \sa QLineEdit::readOnly
*/

bool QAbstractSpinBox::isReadOnly() const
{
    Q_D(const QAbstractSpinBox);
    return d->readOnly;
}

void QAbstractSpinBox::setReadOnly(bool enable)
{
    Q_D(QAbstractSpinBox);
    d->readOnly = enable;
    d->edit->setReadOnly(enable);
    update();
}

/*!
    \property QAbstractSpinBox::keyboardTracking
    \brief whether keyboard tracking is enabled for the spinbox.
    \since 4.3

    If keyboard tracking is enabled (the default), the spinbox
    emits the valueChanged() signal while the new value is being
    entered from the keyboard.

    E.g. when the user enters the value 600 by typing 6, 0, and 0,
    the spinbox emits 3 signals with the values 6, 60, and 600
    respectively.

    If keyboard tracking is disabled, the spinbox doesn't emit the
    valueChanged() signal while typing. It emits the signal later,
    when the return key is pressed, when keyboard focus is lost, or
    when other spinbox functionality is used, e.g. pressing an arrow
    key.
*/

bool QAbstractSpinBox::keyboardTracking() const
{
    Q_D(const QAbstractSpinBox);
    return d->keyboardTracking;
}

void QAbstractSpinBox::setKeyboardTracking(bool enable)
{
    Q_D(QAbstractSpinBox);
    d->keyboardTracking = enable;
}

/*!
    \property QAbstractSpinBox::frame
    \brief whether the spin box draws itself with a frame

    If enabled (the default) the spin box draws itself inside a frame,
    otherwise the spin box draws itself without any frame.
*/

bool QAbstractSpinBox::hasFrame() const
{
    Q_D(const QAbstractSpinBox);
    return d->frame;
}


void QAbstractSpinBox::setFrame(bool enable)
{
    Q_D(QAbstractSpinBox);
    d->frame = enable;
    update();
    d->updateEditFieldGeometry();
}

/*!
    \property QAbstractSpinBox::accelerated
    \brief whether the spin box will accelerate the frequency of the steps when
    pressing the step Up/Down buttons.
    \since 4.2

    If enabled the spin box will increase/decrease the value faster
    the longer you hold the button down.
*/

void QAbstractSpinBox::setAccelerated(bool accelerate)
{
    Q_D(QAbstractSpinBox);
    d->accelerate = accelerate;

}
bool QAbstractSpinBox::isAccelerated() const
{
    Q_D(const QAbstractSpinBox);
    return d->accelerate;
}

/*!
     \property QAbstractSpinBox::showGroupSeparator
     \since 5.3


     This property holds whether a thousands separator is enabled. By default this
     property is false.
*/
bool QAbstractSpinBox::isGroupSeparatorShown() const
{
    Q_D(const QAbstractSpinBox);
    return d->showGroupSeparator;
}

void QAbstractSpinBox::setGroupSeparatorShown(bool shown)
{
    Q_D(QAbstractSpinBox);
    if (d->showGroupSeparator == shown)
        return;
    d->showGroupSeparator = shown;
    d->setValue(d->value, EmitIfChanged);
    updateGeometry();
}

/*!
    \enum QAbstractSpinBox::CorrectionMode

    This enum type describes the mode the spinbox will use to correct
    an \l{QValidator::}{Intermediate} value if editing finishes.

    \value CorrectToPreviousValue The spinbox will revert to the last
                                  valid value.

    \value CorrectToNearestValue The spinbox will revert to the nearest
                                 valid value.

    \sa correctionMode
*/

/*!
    \property QAbstractSpinBox::correctionMode
    \brief the mode to correct an \l{QValidator::}{Intermediate}
           value if editing finishes
    \since 4.2

    The default mode is QAbstractSpinBox::CorrectToPreviousValue.

    \sa acceptableInput, validate(), fixup()
*/
void QAbstractSpinBox::setCorrectionMode(CorrectionMode correctionMode)
{
    Q_D(QAbstractSpinBox);
    d->correctionMode = correctionMode;

}
QAbstractSpinBox::CorrectionMode QAbstractSpinBox::correctionMode() const
{
    Q_D(const QAbstractSpinBox);
    return d->correctionMode;
}


/*!
  \property QAbstractSpinBox::acceptableInput
  \brief whether the input satisfies the current validation
  \since 4.2

  \sa validate(), fixup(), correctionMode
*/

bool QAbstractSpinBox::hasAcceptableInput() const
{
    Q_D(const QAbstractSpinBox);
    return d->edit->hasAcceptableInput();
}

/*!
    \property QAbstractSpinBox::alignment
    \brief the alignment of the spin box

    Possible Values are Qt::AlignLeft, Qt::AlignRight, and Qt::AlignHCenter.

    By default, the alignment is Qt::AlignLeft

    Attempting to set the alignment to an illegal flag combination
    does nothing.

    \sa Qt::Alignment
*/

Qt::Alignment QAbstractSpinBox::alignment() const
{
    Q_D(const QAbstractSpinBox);

    return (Qt::Alignment)d->edit->alignment();
}

void QAbstractSpinBox::setAlignment(Qt::Alignment flag)
{
    Q_D(QAbstractSpinBox);

    d->edit->setAlignment(flag);
}

/*!
    Selects all the text in the spinbox except the prefix and suffix.
*/

void QAbstractSpinBox::selectAll()
{
    Q_D(QAbstractSpinBox);


    if (!d->specialValue()) {
        const int tmp = d->edit->displayText().size() - d->suffix.size();
        d->edit->setSelection(tmp, -(tmp - d->prefix.size()));
    } else {
        d->edit->selectAll();
    }
}

/*!
    Clears the lineedit of all text but prefix and suffix.
*/

void QAbstractSpinBox::clear()
{
    Q_D(QAbstractSpinBox);

    d->edit->setText(d->prefix + d->suffix);
    d->edit->setCursorPosition(d->prefix.size());
    d->cleared = true;
}

/*!
    Virtual function that determines whether stepping up and down is
    legal at any given time.

    The up arrow will be painted as disabled unless (stepEnabled() &
    StepUpEnabled) != 0.

    The default implementation will return (StepUpEnabled|
    StepDownEnabled) if wrapping is turned on. Else it will return
    StepDownEnabled if value is > minimum() or'ed with StepUpEnabled if
    value < maximum().

    If you subclass QAbstractSpinBox you will need to reimplement this function.

    \sa QSpinBox::minimum(), QSpinBox::maximum(), wrapping()
*/


QAbstractSpinBox::StepEnabled QAbstractSpinBox::stepEnabled() const
{
    Q_D(const QAbstractSpinBox);
    if (d->readOnly || d->type == QVariant::Invalid)
        return StepNone;
    if (d->wrapping)
        return StepEnabled(StepUpEnabled | StepDownEnabled);
    StepEnabled ret = StepNone;
    if (d->variantCompare(d->value, d->maximum) < 0) {
        ret |= StepUpEnabled;
    }
    if (d->variantCompare(d->value, d->minimum) > 0) {
        ret |= StepDownEnabled;
    }
    return ret;
}

/*!
   This virtual function is called by the QAbstractSpinBox to
   determine whether \a input is valid. The \a pos parameter indicates
   the position in the string. Reimplemented in the various
   subclasses.
*/

QValidator::State QAbstractSpinBox::validate(QString & /* input */, int & /* pos */) const
{
    return QValidator::Acceptable;
}

/*!
   This virtual function is called by the QAbstractSpinBox if the
   \a input is not validated to QValidator::Acceptable when Return is
   pressed or interpretText() is called. It will try to change the
   text so it is valid. Reimplemented in the various subclasses.
*/

void QAbstractSpinBox::fixup(QString & /* input */) const
{
}

/*!
  Steps up by one linestep
  Calling this slot is analogous to calling stepBy(1);
  \sa stepBy(), stepDown()
*/

void QAbstractSpinBox::stepUp()
{
    stepBy(1);
}

/*!
  Steps down by one linestep
  Calling this slot is analogous to calling stepBy(-1);
  \sa stepBy(), stepUp()
*/

void QAbstractSpinBox::stepDown()
{
    stepBy(-1);
}
/*!
    Virtual function that is called whenever the user triggers a step.
    The \a steps parameter indicates how many steps were taken, e.g.
    Pressing Qt::Key_Down will trigger a call to stepBy(-1),
    whereas pressing Qt::Key_Prior will trigger a call to
    stepBy(10).

    If you subclass QAbstractSpinBox you must reimplement this
    function. Note that this function is called even if the resulting
    value will be outside the bounds of minimum and maximum. It's this
    function's job to handle these situations.
*/

void QAbstractSpinBox::stepBy(int steps)
{
    Q_D(QAbstractSpinBox);

    const QVariant old = d->value;
    QString tmp = d->edit->displayText();
    int cursorPos = d->edit->cursorPosition();
    bool dontstep = false;
    EmitPolicy e = EmitIfChanged;
    if (d->pendingEmit) {
        dontstep = validate(tmp, cursorPos) != QValidator::Acceptable;
        d->cleared = false;
        d->interpret(NeverEmit);
        if (d->value != old)
            e = AlwaysEmit;
    }
    if (!dontstep) {
        d->setValue(d->bound(d->value + (d->singleStep * steps), old, steps), e);
    } else if (e == AlwaysEmit) {
        d->emitSignals(e, old);
    }
    selectAll();
}

/*!
    This function returns a pointer to the line edit of the spin box.
*/

QLineEdit *QAbstractSpinBox::lineEdit() const
{
    Q_D(const QAbstractSpinBox);

    return d->edit;
}


/*!
    \fn void QAbstractSpinBox::setLineEdit(QLineEdit *lineEdit)

    Sets the line edit of the spinbox to be \a lineEdit instead of the
    current line edit widget. \a lineEdit can not be 0.

    QAbstractSpinBox takes ownership of the new lineEdit

    If QLineEdit::validator() for the \a lineEdit returns 0, the internal
    validator of the spinbox will be set on the line edit.
*/

void QAbstractSpinBox::setLineEdit(QLineEdit *lineEdit)
{
    Q_D(QAbstractSpinBox);

    if (!lineEdit) {
        Q_ASSERT(lineEdit);
        return;
    }
    delete d->edit;
    d->edit = lineEdit;
    if (!d->edit->validator())
        d->edit->setValidator(d->validator);

    if (d->edit->parent() != this)
        d->edit->setParent(this);

    d->edit->setFrame(false);
    d->edit->setFocusProxy(this);
    d->edit->setAcceptDrops(false);

    if (d->type != QVariant::Invalid) {
        connect(d->edit, SIGNAL(textChanged(QString)),
                this, SLOT(_q_editorTextChanged(QString)));
        connect(d->edit, SIGNAL(cursorPositionChanged(int,int)),
                this, SLOT(_q_editorCursorPositionChanged(int,int)));
    }
    d->updateEditFieldGeometry();
    d->edit->setContextMenuPolicy(Qt::NoContextMenu);

    if (isVisible())
        d->edit->show();
    if (isVisible())
        d->updateEdit();
}


/*!
    This function interprets the text of the spin box. If the value
    has changed since last interpretation it will emit signals.
*/

void QAbstractSpinBox::interpretText()
{
    Q_D(QAbstractSpinBox);
    d->interpret(EmitIfChanged);
}

/*
    Reimplemented in 4.6, so be careful.
 */
/*!
    \reimp
*/
QVariant QAbstractSpinBox::inputMethodQuery(Qt::InputMethodQuery query) const
{
    Q_D(const QAbstractSpinBox);
    return d->edit->inputMethodQuery(query);
}

/*!
    \reimp
*/

bool QAbstractSpinBox::event(QEvent *event)
{
    Q_D(QAbstractSpinBox);
    switch (event->type()) {
    case QEvent::FontChange:
    case QEvent::StyleChange:
        d->cachedSizeHint = d->cachedMinimumSizeHint = QSize();
        break;
    case QEvent::ApplicationLayoutDirectionChange:
    case QEvent::LayoutDirectionChange:
        d->updateEditFieldGeometry();
        break;
    case QEvent::HoverEnter:
    case QEvent::HoverLeave:
    case QEvent::HoverMove:
        if (const QHoverEvent *he = static_cast<const QHoverEvent *>(event))
            d->updateHoverControl(he->pos());
        break;
    case QEvent::ShortcutOverride:
        if (d->edit->event(event))
            return true;
        break;
#ifdef QT_KEYPAD_NAVIGATION
    case QEvent::EnterEditFocus:
    case QEvent::LeaveEditFocus:
        if (QApplication::keypadNavigationEnabled()) {
            const bool b = d->edit->event(event);
            d->edit->setSelection(d->edit->displayText().size() - d->suffix.size(),0);
            if (event->type() == QEvent::LeaveEditFocus)
                emit editingFinished();
            if (b)
                return true;
        }
        break;
#endif
    case QEvent::InputMethod:
        return d->edit->event(event);
    default:
        break;
    }
    return QWidget::event(event);
}

/*!
    \reimp
*/

void QAbstractSpinBox::showEvent(QShowEvent *)
{
    Q_D(QAbstractSpinBox);
    d->reset();

    if (d->ignoreUpdateEdit) {
        d->ignoreUpdateEdit = false;
    } else {
        d->updateEdit();
    }
}

/*!
    \reimp
*/

void QAbstractSpinBox::changeEvent(QEvent *event)
{
    Q_D(QAbstractSpinBox);

    switch (event->type()) {
        case QEvent::StyleChange:
            d->spinClickTimerInterval = style()->styleHint(QStyle::SH_SpinBox_ClickAutoRepeatRate, 0, this);
            d->spinClickThresholdTimerInterval =
                style()->styleHint(QStyle::SH_SpinBox_ClickAutoRepeatThreshold, 0, this);
            d->reset();
            d->updateEditFieldGeometry();
            break;
        case QEvent::EnabledChange:
            if (!isEnabled()) {
                d->reset();
            }
            break;
        case QEvent::ActivationChange:
            if (!isActiveWindow()){
                d->reset();
                if (d->pendingEmit) // pendingEmit can be true even if it hasn't changed.
                    d->interpret(EmitIfChanged); // E.g. 10 to 10.0
            }
            break;
        default:
            break;
    }
    QWidget::changeEvent(event);
}

/*!
    \reimp
*/

void QAbstractSpinBox::resizeEvent(QResizeEvent *event)
{
    Q_D(QAbstractSpinBox);
    QWidget::resizeEvent(event);

    d->updateEditFieldGeometry();
    update();
}

/*!
    \reimp
*/

QSize QAbstractSpinBox::sizeHint() const
{
    Q_D(const QAbstractSpinBox);
    if (d->cachedSizeHint.isEmpty()) {
        ensurePolished();

        const QFontMetrics fm(fontMetrics());
        int h = d->edit->sizeHint().height();
        int w = 0;
        QString s;
        QString fixedContent =  d->prefix + d->suffix + QLatin1Char(' ');
        s = d->textFromValue(d->minimum) + fixedContent;
        w = qMax(w, fm.width(s));
        s = d->textFromValue(d->maximum) + fixedContent;
        w = qMax(w, fm.width(s));
        if (d->specialValueText.size()) {
            s = d->specialValueText;
            w = qMax(w, fm.width(s));
        }
        w += 2; // cursor blinking space

        QStyleOptionSpinBox opt;
        initStyleOption(&opt);
        QSize hint(w, h);
        d->cachedSizeHint = style()->sizeFromContents(QStyle::CT_SpinBox, &opt, hint, this)
                            .expandedTo(QApplication::globalStrut());
    }
    return d->cachedSizeHint;
}

/*!
    \reimp
*/

QSize QAbstractSpinBox::minimumSizeHint() const
{
    Q_D(const QAbstractSpinBox);
    if (d->cachedMinimumSizeHint.isEmpty()) {
        //Use the prefix and range to calculate the minimumSizeHint
        ensurePolished();

        const QFontMetrics fm(fontMetrics());
        int h = d->edit->minimumSizeHint().height();
        int w = 0;

        QString s;
        QString fixedContent =  d->prefix + QLatin1Char(' ');
        s = d->textFromValue(d->minimum) + fixedContent;
        w = qMax(w, fm.width(s));
        s = d->textFromValue(d->maximum) + fixedContent;
        w = qMax(w, fm.width(s));

        if (d->specialValueText.size()) {
            s = d->specialValueText;
            w = qMax(w, fm.width(s));
        }
        w += 2; // cursor blinking space

        QStyleOptionSpinBox opt;
        initStyleOption(&opt);
        QSize hint(w, h);

        d->cachedMinimumSizeHint = style()->sizeFromContents(QStyle::CT_SpinBox, &opt, hint, this)
                                   .expandedTo(QApplication::globalStrut());
    }
    return d->cachedMinimumSizeHint;
}

/*!
    \reimp
*/

void QAbstractSpinBox::paintEvent(QPaintEvent *)
{
    QStyleOptionSpinBox opt;
    initStyleOption(&opt);
    QStylePainter p(this);
    p.drawComplexControl(QStyle::CC_SpinBox, opt);
}

/*!
    \reimp

    This function handles keyboard input.

    The following keys are handled specifically:
    \table
    \row \li Enter/Return
         \li This will reinterpret the text and emit a signal even if the value has not changed
         since last time a signal was emitted.
    \row \li Up
         \li This will invoke stepBy(1)
    \row \li Down
         \li This will invoke stepBy(-1)
    \row \li Page up
         \li This will invoke stepBy(10)
    \row \li Page down
         \li This will invoke stepBy(-10)
    \endtable
*/


void QAbstractSpinBox::keyPressEvent(QKeyEvent *event)
{
    Q_D(QAbstractSpinBox);

    if (!event->text().isEmpty() && d->edit->cursorPosition() < d->prefix.size())
        d->edit->setCursorPosition(d->prefix.size());

    int steps = 1;
    bool isPgUpOrDown = false;
    switch (event->key()) {
    case Qt::Key_PageUp:
    case Qt::Key_PageDown:
        steps *= 10;
        isPgUpOrDown = true;
    case Qt::Key_Up:
    case Qt::Key_Down: {
#ifdef QT_KEYPAD_NAVIGATION
        if (QApplication::keypadNavigationEnabled()) {
            // Reserve up/down for nav - use left/right for edit.
            if (!hasEditFocus() && (event->key() == Qt::Key_Up
                                    || event->key() == Qt::Key_Down)) {
                event->ignore();
                return;
            }
        }
#endif
        event->accept();
        const bool up = (event->key() == Qt::Key_PageUp || event->key() == Qt::Key_Up);
        if (!(stepEnabled() & (up ? StepUpEnabled : StepDownEnabled)))
            return;
        if (!up)
            steps *= -1;
        if (style()->styleHint(QStyle::SH_SpinBox_AnimateButton, 0, this)) {
            d->buttonState = (Keyboard | (up ? Up : Down));
        }
        if (d->spinClickTimerId == -1)
            stepBy(steps);
        if(event->isAutoRepeat() && !isPgUpOrDown) {
            if(d->spinClickThresholdTimerId == -1 && d->spinClickTimerId == -1) {
                d->updateState(up, true);
            }
        }
#ifndef QT_NO_ACCESSIBILITY
        QAccessibleValueChangeEvent event(this, d->value);
        QAccessible::updateAccessibility(&event);
#endif
        return;
    }
#ifdef QT_KEYPAD_NAVIGATION
    case Qt::Key_Left:
    case Qt::Key_Right:
        if (QApplication::keypadNavigationEnabled() && !hasEditFocus()) {
            event->ignore();
            return;
        }
        break;
    case Qt::Key_Back:
        if (QApplication::keypadNavigationEnabled() && !hasEditFocus()) {
            event->ignore();
            return;
        }
        break;
#endif
    case Qt::Key_Enter:
    case Qt::Key_Return:
        d->edit->d_func()->control->clearUndo();
        d->interpret(d->keyboardTracking ? AlwaysEmit : EmitIfChanged);
        selectAll();
        event->ignore();
        emit editingFinished();
        emit d->edit->returnPressed();
        return;

#ifdef QT_KEYPAD_NAVIGATION
    case Qt::Key_Select:
        if (QApplication::keypadNavigationEnabled()) {
            // Toggles between left/right moving cursor and inc/dec.
            setEditFocus(!hasEditFocus());
        }
        return;
#endif

    case Qt::Key_U:
        if (event->modifiers() & Qt::ControlModifier
            && QGuiApplication::platformName() == QLatin1String("xcb")) { // only X11
            event->accept();
            if (!isReadOnly())
                clear();
            return;
        }
        break;

    case Qt::Key_End:
    case Qt::Key_Home:
        if (event->modifiers() & Qt::ShiftModifier) {
            int currentPos = d->edit->cursorPosition();
            const QString text = d->edit->displayText();
            if (event->key() == Qt::Key_End) {
                if ((currentPos == 0 && !d->prefix.isEmpty()) || text.size() - d->suffix.size() <= currentPos) {
                    break; // let lineedit handle this
                } else {
                    d->edit->setSelection(currentPos, text.size() - d->suffix.size() - currentPos);
                }
            } else {
                if ((currentPos == text.size() && !d->suffix.isEmpty()) || currentPos <= d->prefix.size()) {
                    break; // let lineedit handle this
                } else {
                    d->edit->setSelection(currentPos, d->prefix.size() - currentPos);
                }
            }
            event->accept();
            return;
        }
        break;

    default:
#ifndef QT_NO_SHORTCUT
        if (event == QKeySequence::SelectAll) {
            selectAll();
            event->accept();
            return;
        }
#endif
        break;
    }

    d->edit->event(event);
    if (!isVisible())
        d->ignoreUpdateEdit = true;
}

/*!
    \reimp
*/

void QAbstractSpinBox::keyReleaseEvent(QKeyEvent *event)
{
    Q_D(QAbstractSpinBox);

    if (d->buttonState & Keyboard && !event->isAutoRepeat())  {
        d->reset();
    } else {
        d->edit->event(event);
    }
}

/*!
    \reimp
*/

#ifndef QT_NO_WHEELEVENT
void QAbstractSpinBox::wheelEvent(QWheelEvent *event)
{
    const int steps = (event->delta() > 0 ? 1 : -1);
    if (stepEnabled() & (steps > 0 ? StepUpEnabled : StepDownEnabled))
        stepBy(event->modifiers() & Qt::ControlModifier ? steps * 10 : steps);
    event->accept();
}
#endif


/*!
    \reimp
*/
void QAbstractSpinBox::focusInEvent(QFocusEvent *event)
{
    Q_D(QAbstractSpinBox);

    d->edit->event(event);
    if (event->reason() == Qt::TabFocusReason || event->reason() == Qt::BacktabFocusReason) {
        selectAll();
    }
    QWidget::focusInEvent(event);
}

/*!
    \reimp
*/

void QAbstractSpinBox::focusOutEvent(QFocusEvent *event)
{
    Q_D(QAbstractSpinBox);

    if (d->pendingEmit)
        d->interpret(EmitIfChanged);

    d->reset();
    d->edit->event(event);
    d->updateEdit();
    QWidget::focusOutEvent(event);

#ifdef QT_KEYPAD_NAVIGATION
    // editingFinished() is already emitted on LeaveEditFocus
    if (!QApplication::keypadNavigationEnabled())
#endif
    emit editingFinished();
}

/*!
    \reimp
*/

void QAbstractSpinBox::closeEvent(QCloseEvent *event)
{
    Q_D(QAbstractSpinBox);

    d->reset();
    if (d->pendingEmit)
        d->interpret(EmitIfChanged);
    QWidget::closeEvent(event);
}

/*!
    \reimp
*/

void QAbstractSpinBox::hideEvent(QHideEvent *event)
{
    Q_D(QAbstractSpinBox);
    d->reset();
    if (d->pendingEmit)
        d->interpret(EmitIfChanged);
    QWidget::hideEvent(event);
}


/*!
    \reimp
*/

void QAbstractSpinBox::timerEvent(QTimerEvent *event)
{
    Q_D(QAbstractSpinBox);

    bool doStep = false;
    if (event->timerId() == d->spinClickThresholdTimerId) {
        killTimer(d->spinClickThresholdTimerId);
        d->spinClickThresholdTimerId = -1;
        d->effectiveSpinRepeatRate = d->buttonState & Keyboard
                                     ? qApp->styleHints()->keyboardAutoRepeatRate()
                                     : d->spinClickTimerInterval;
        d->spinClickTimerId = startTimer(d->effectiveSpinRepeatRate);
        doStep = true;
    } else if (event->timerId() == d->spinClickTimerId) {
        if (d->accelerate) {
            d->acceleration = d->acceleration + (int)(d->effectiveSpinRepeatRate * 0.05);
            if (d->effectiveSpinRepeatRate - d->acceleration >= 10) {
                killTimer(d->spinClickTimerId);
                d->spinClickTimerId = startTimer(d->effectiveSpinRepeatRate - d->acceleration);
            }
        }
        doStep = true;
    }

    if (doStep) {
        const StepEnabled st = stepEnabled();
        if (d->buttonState & Up) {
            if (!(st & StepUpEnabled)) {
                d->reset();
            } else {
                stepBy(1);
            }
        } else if (d->buttonState & Down) {
            if (!(st & StepDownEnabled)) {
                d->reset();
            } else {
                stepBy(-1);
            }
        }
        return;
    }
    QWidget::timerEvent(event);
    return;
}

/*!
    \reimp
*/

void QAbstractSpinBox::contextMenuEvent(QContextMenuEvent *event)
{
#ifdef QT_NO_CONTEXTMENU
    Q_UNUSED(event);
#else
    Q_D(QAbstractSpinBox);

    QPointer<QMenu> menu = d->edit->createStandardContextMenu();
    if (!menu)
        return;

    d->reset();

    QAction *selAll = new QAction(tr("&Select All"), menu);
    menu->insertAction(d->edit->d_func()->selectAllAction,
                      selAll);
    menu->removeAction(d->edit->d_func()->selectAllAction);
    menu->addSeparator();
    const uint se = stepEnabled();
    QAction *up = menu->addAction(tr("&Step up"));
    up->setEnabled(se & StepUpEnabled);
    QAction *down = menu->addAction(tr("Step &down"));
    down->setEnabled(se & StepDownEnabled);
    menu->addSeparator();

    const QPointer<QAbstractSpinBox> that = this;
    const QPoint pos = (event->reason() == QContextMenuEvent::Mouse)
        ? event->globalPos() : mapToGlobal(QPoint(event->pos().x(), 0)) + QPoint(width() / 2, height() / 2);
    const QAction *action = menu->exec(pos);
    delete static_cast<QMenu *>(menu);
    if (that && action) {
        if (action == up) {
            stepBy(1);
        } else if (action == down) {
            stepBy(-1);
        } else if (action == selAll) {
            selectAll();
        }
    }
    event->accept();
#endif // QT_NO_CONTEXTMENU
}

/*!
    \reimp
*/

void QAbstractSpinBox::mouseMoveEvent(QMouseEvent *event)
{
    Q_D(QAbstractSpinBox);

    d->updateHoverControl(event->pos());

    // If we have a timer ID, update the state
    if (d->spinClickTimerId != -1 && d->buttonSymbols != NoButtons) {
        const StepEnabled se = stepEnabled();
        if ((se & StepUpEnabled) && d->hoverControl == QStyle::SC_SpinBoxUp)
            d->updateState(true);
        else if ((se & StepDownEnabled) && d->hoverControl == QStyle::SC_SpinBoxDown)
            d->updateState(false);
        else
            d->reset();
        event->accept();
    }
}

/*!
    \reimp
*/

void QAbstractSpinBox::mousePressEvent(QMouseEvent *event)
{
    Q_D(QAbstractSpinBox);

    if (event->button() != Qt::LeftButton || d->buttonState != None) {
        return;
    }

    d->updateHoverControl(event->pos());
    event->accept();

    const StepEnabled se = (d->buttonSymbols == NoButtons) ? StepEnabled(StepNone) : stepEnabled();
    if ((se & StepUpEnabled) && d->hoverControl == QStyle::SC_SpinBoxUp) {
        d->updateState(true);
    } else if ((se & StepDownEnabled) && d->hoverControl == QStyle::SC_SpinBoxDown) {
        d->updateState(false);
    } else {
        event->ignore();
    }
}

/*!
    \reimp
*/
void QAbstractSpinBox::mouseReleaseEvent(QMouseEvent *event)
{
    Q_D(QAbstractSpinBox);

    if ((d->buttonState & Mouse) != 0)
        d->reset();
    event->accept();
}

// --- QAbstractSpinBoxPrivate ---

/*!
    \internal
    Constructs a QAbstractSpinBoxPrivate object
*/

QAbstractSpinBoxPrivate::QAbstractSpinBoxPrivate()
    : edit(0), type(QVariant::Invalid), spinClickTimerId(-1),
      spinClickTimerInterval(100), spinClickThresholdTimerId(-1), spinClickThresholdTimerInterval(-1),
      effectiveSpinRepeatRate(1), buttonState(None), cachedText(QLatin1String("\x01")),
      cachedState(QValidator::Invalid), pendingEmit(false), readOnly(false), wrapping(false),
      ignoreCursorPositionChanged(false), frame(true), accelerate(false), keyboardTracking(true),
      cleared(false), ignoreUpdateEdit(false), correctionMode(QAbstractSpinBox::CorrectToPreviousValue),
      acceleration(0), hoverControl(QStyle::SC_None), buttonSymbols(QAbstractSpinBox::UpDownArrows), validator(0),
      showGroupSeparator(0)
{
}

/*
   \internal
   Called when the QAbstractSpinBoxPrivate is destroyed
*/
QAbstractSpinBoxPrivate::~QAbstractSpinBoxPrivate()
{
}

/*!
    \internal
    Updates the old and new hover control. Does nothing if the hover
    control has not changed.
*/
bool QAbstractSpinBoxPrivate::updateHoverControl(const QPoint &pos)
{
    Q_Q(QAbstractSpinBox);
    QRect lastHoverRect = hoverRect;
    QStyle::SubControl lastHoverControl = hoverControl;
    bool doesHover = q->testAttribute(Qt::WA_Hover);
    if (lastHoverControl != newHoverControl(pos) && doesHover) {
        q->update(lastHoverRect);
        q->update(hoverRect);
        return true;
    }
    return !doesHover;
}

/*!
    \internal
    Returns the hover control at \a pos.
    This will update the hoverRect and hoverControl.
*/
QStyle::SubControl QAbstractSpinBoxPrivate::newHoverControl(const QPoint &pos)
{
    Q_Q(QAbstractSpinBox);

    QStyleOptionSpinBox opt;
    q->initStyleOption(&opt);
    opt.subControls = QStyle::SC_All;
    hoverControl = q->style()->hitTestComplexControl(QStyle::CC_SpinBox, &opt, pos, q);
    hoverRect = q->style()->subControlRect(QStyle::CC_SpinBox, &opt, hoverControl, q);
    return hoverControl;
}

/*!
    \internal
    Strips any prefix/suffix from \a text.
*/

QString QAbstractSpinBoxPrivate::stripped(const QString &t, int *pos) const
{
    QString text = t;
    if (specialValueText.size() == 0 || text != specialValueText) {
        int from = 0;
        int size = text.size();
        bool changed = false;
        if (prefix.size() && text.startsWith(prefix)) {
            from += prefix.size();
            size -= from;
            changed = true;
        }
        if (suffix.size() && text.endsWith(suffix)) {
            size -= suffix.size();
            changed = true;
        }
        if (changed)
            text = text.mid(from, size);
    }

    const int s = text.size();
    text = text.trimmed();
    if (pos)
        (*pos) -= (s - text.size());
    return text;

}

void QAbstractSpinBoxPrivate::updateEditFieldGeometry()
{
    Q_Q(QAbstractSpinBox);
    QStyleOptionSpinBox opt;
    q->initStyleOption(&opt);
    opt.subControls = QStyle::SC_SpinBoxEditField;
    edit->setGeometry(q->style()->subControlRect(QStyle::CC_SpinBox, &opt,
                                                 QStyle::SC_SpinBoxEditField, q));
}
/*!
    \internal
    Returns \c true if a specialValueText has been set and the current value is minimum.
*/

bool QAbstractSpinBoxPrivate::specialValue() const
{
    return (value == minimum && !specialValueText.isEmpty());
}

/*!
    \internal Virtual function that emits signals when the value
    changes. Reimplemented in the different subclasses.
*/

void QAbstractSpinBoxPrivate::emitSignals(EmitPolicy, const QVariant &)
{
}

/*!
    \internal

    Slot connected to the line edit's textChanged(const QString &)
    signal.
*/

void QAbstractSpinBoxPrivate::_q_editorTextChanged(const QString &t)
{
    Q_Q(QAbstractSpinBox);

    if (keyboardTracking) {
        QString tmp = t;
        int pos = edit->cursorPosition();
        QValidator::State state = q->validate(tmp, pos);
        if (state == QValidator::Acceptable) {
            const QVariant v = valueFromText(tmp);
            setValue(v, EmitIfChanged, tmp != t);
            pendingEmit = false;
        } else {
            pendingEmit = true;
        }
    } else {
        pendingEmit = true;
    }
}

/*!
    \internal

    Virtual slot connected to the line edit's
    cursorPositionChanged(int, int) signal. Will move the cursor to a
    valid position if the new one is invalid. E.g. inside the prefix.
    Reimplemented in Q[Date|Time|DateTime]EditPrivate to account for
    the different sections etc.
*/

void QAbstractSpinBoxPrivate::_q_editorCursorPositionChanged(int oldpos, int newpos)
{
    if (!edit->hasSelectedText() && !ignoreCursorPositionChanged && !specialValue()) {
        ignoreCursorPositionChanged = true;

        bool allowSelection = true;
        int pos = -1;
        if (newpos < prefix.size() && newpos != 0) {
            if (oldpos == 0) {
                allowSelection = false;
                pos = prefix.size();
            } else {
                pos = oldpos;
            }
        } else if (newpos > edit->text().size() - suffix.size()
                   && newpos != edit->text().size()) {
            if (oldpos == edit->text().size()) {
                pos = edit->text().size() - suffix.size();
                allowSelection = false;
            } else {
                pos = edit->text().size();
            }
        }
        if (pos != -1) {
            const int selSize = edit->selectionStart() >= 0 && allowSelection
                                  ? (edit->selectedText().size()
                                     * (newpos < pos ? -1 : 1)) - newpos + pos
                                  : 0;

            const QSignalBlocker blocker(edit);
            if (selSize != 0) {
                edit->setSelection(pos - selSize, selSize);
            } else {
                edit->setCursorPosition(pos);
            }
        }
        ignoreCursorPositionChanged = false;
    }
}

/*!
    \internal

    Initialises the QAbstractSpinBoxPrivate object.
*/

void QAbstractSpinBoxPrivate::init()
{
    Q_Q(QAbstractSpinBox);

    q->setLineEdit(new QLineEdit(q));
    edit->setObjectName(QLatin1String("qt_spinbox_lineedit"));
    validator = new QSpinBoxValidator(q, this);
    edit->setValidator(validator);

    QStyleOptionSpinBox opt;
    q->initStyleOption(&opt);
    spinClickTimerInterval = q->style()->styleHint(QStyle::SH_SpinBox_ClickAutoRepeatRate, &opt, q);
    spinClickThresholdTimerInterval = q->style()->styleHint(QStyle::SH_SpinBox_ClickAutoRepeatThreshold, &opt, q);
    q->setFocusPolicy(Qt::WheelFocus);
    q->setSizePolicy(QSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed, QSizePolicy::SpinBox));
    q->setAttribute(Qt::WA_InputMethodEnabled);

    q->setAttribute(Qt::WA_MacShowFocusRect);
}

/*!
    \internal

    Resets the state of the spinbox. E.g. the state is set to
    (Keyboard|Up) if Key up is currently pressed.
*/

void QAbstractSpinBoxPrivate::reset()
{
    Q_Q(QAbstractSpinBox);

    buttonState = None;
    if (q) {
        if (spinClickTimerId != -1)
            q->killTimer(spinClickTimerId);
        if (spinClickThresholdTimerId != -1)
            q->killTimer(spinClickThresholdTimerId);
        spinClickTimerId = spinClickThresholdTimerId = -1;
        acceleration = 0;
        q->update();
    }
}

/*!
    \internal

    Updates the state of the spinbox.
*/

void QAbstractSpinBoxPrivate::updateState(bool up, bool fromKeyboard /* = false */)
{
    Q_Q(QAbstractSpinBox);
    if ((up && (buttonState & Up)) || (!up && (buttonState & Down)))
        return;
    reset();
    if (q && (q->stepEnabled() & (up ? QAbstractSpinBox::StepUpEnabled
                                  : QAbstractSpinBox::StepDownEnabled))) {
        spinClickThresholdTimerId = q->startTimer(spinClickThresholdTimerInterval);
        buttonState = (up ? Up : Down) | (fromKeyboard ? Keyboard : Mouse);
        q->stepBy(up ? 1 : -1);
#ifndef QT_NO_ACCESSIBILITY
        QAccessibleValueChangeEvent event(q, value);
        QAccessible::updateAccessibility(&event);
#endif
    }
}


/*!
    Initialize \a option with the values from this QSpinBox. This method
    is useful for subclasses when they need a QStyleOptionSpinBox, but don't want
    to fill in all the information themselves.

    \sa QStyleOption::initFrom()
*/
void QAbstractSpinBox::initStyleOption(QStyleOptionSpinBox *option) const
{
    if (!option)
        return;

    Q_D(const QAbstractSpinBox);
    option->initFrom(this);
    option->activeSubControls = QStyle::SC_None;
    option->buttonSymbols = d->buttonSymbols;
    option->subControls = QStyle::SC_SpinBoxFrame | QStyle::SC_SpinBoxEditField;
    if (d->buttonSymbols != QAbstractSpinBox::NoButtons) {
        option->subControls |= QStyle::SC_SpinBoxUp | QStyle::SC_SpinBoxDown;
        if (d->buttonState & Up) {
            option->activeSubControls = QStyle::SC_SpinBoxUp;
        } else if (d->buttonState & Down) {
            option->activeSubControls = QStyle::SC_SpinBoxDown;
        }
    }

    if (d->buttonState) {
        option->state |= QStyle::State_Sunken;
    } else {
        option->activeSubControls = d->hoverControl;
    }

    option->stepEnabled = style()->styleHint(QStyle::SH_SpinControls_DisableOnBounds)
                      ? stepEnabled()
                      : (QAbstractSpinBox::StepDownEnabled|QAbstractSpinBox::StepUpEnabled);

    option->frame = d->frame;
}

/*!
    \internal

    Bounds \a val to be within minimum and maximum. Also tries to be
    clever about setting it at min and max depending on what it was
    and what direction it was changed etc.
*/

QVariant QAbstractSpinBoxPrivate::bound(const QVariant &val, const QVariant &old, int steps) const
{
    QVariant v = val;
    if (!wrapping || steps == 0 || old.isNull()) {
        if (variantCompare(v, minimum) < 0) {
            v = wrapping ? maximum : minimum;
        }
        if (variantCompare(v, maximum) > 0) {
            v = wrapping ? minimum : maximum;
        }
    } else {
        const bool wasMin = old == minimum;
        const bool wasMax = old == maximum;
        const int oldcmp = variantCompare(v, old);
        const int maxcmp = variantCompare(v, maximum);
        const int mincmp = variantCompare(v, minimum);
        const bool wrapped = (oldcmp > 0 && steps < 0) || (oldcmp < 0 && steps > 0);
        if (maxcmp > 0) {
            v = ((wasMax && !wrapped && steps > 0) || (steps < 0 && !wasMin && wrapped))
                ? minimum : maximum;
        } else if (wrapped && (maxcmp > 0 || mincmp < 0)) {
            v = ((wasMax && steps > 0) || (!wasMin && steps < 0)) ? minimum : maximum;
        } else if (mincmp < 0) {
            v = (!wasMax && !wasMin ? minimum : maximum);
        }
    }

    return v;
}

/*!
    \internal

    Sets the value of the spin box to \a val. Depending on the value
    of \a ep it will also emit signals.
*/

void QAbstractSpinBoxPrivate::setValue(const QVariant &val, EmitPolicy ep,
                                       bool doUpdate)
{
    Q_Q(QAbstractSpinBox);
    const QVariant old = value;
    value = bound(val);
    pendingEmit = false;
    cleared = false;
    if (doUpdate) {
        updateEdit();
    }
    q->update();

    if (ep == AlwaysEmit || (ep == EmitIfChanged && old != value)) {
        emitSignals(ep, old);
    }
}

/*!
    \internal

    Updates the line edit to reflect the current value of the spin box.
*/

void QAbstractSpinBoxPrivate::updateEdit()
{
    Q_Q(QAbstractSpinBox);
    if (type == QVariant::Invalid)
        return;
    const QString newText = specialValue() ? specialValueText : prefix + textFromValue(value) + suffix;
    if (newText == edit->displayText() || cleared)
        return;

    const bool empty = edit->text().isEmpty();
    int cursor = edit->cursorPosition();
    int selsize = edit->selectedText().size();
    const QSignalBlocker blocker(edit);
    edit->setText(newText);

    if (!specialValue()) {
        cursor = qBound(prefix.size(), cursor, edit->displayText().size() - suffix.size());

        if (selsize > 0) {
            edit->setSelection(cursor, selsize);
        } else {
            edit->setCursorPosition(empty ? prefix.size() : cursor);
        }
    }
    q->update();
}

/*!
    \internal

    Convenience function to set min/max values.
*/

void QAbstractSpinBoxPrivate::setRange(const QVariant &min, const QVariant &max)
{
    Q_Q(QAbstractSpinBox);

    clearCache();
    minimum = min;
    maximum = (variantCompare(min, max) < 0 ? max : min);
    cachedSizeHint = QSize();
    cachedMinimumSizeHint = QSize(); // minimumSizeHint cares about min/max

    reset();
    if (!(bound(value) == value)) {
        setValue(bound(value), EmitIfChanged);
    } else if (value == minimum && !specialValueText.isEmpty()) {
        updateEdit();
    }

    q->updateGeometry();
}

/*!
    \internal

    Convenience function to get a variant of the right type.
*/

QVariant QAbstractSpinBoxPrivate::getZeroVariant() const
{
    QVariant ret;
    switch (type) {
    case QVariant::Int: ret = QVariant((int)0); break;
    case QVariant::Double: ret = QVariant((double)0.0); break;
    default: break;
    }
    return ret;
}

/*!
    \internal

    Virtual method called that calls the public textFromValue()
    functions in the subclasses. Needed to change signature from
    QVariant to int/double/QDateTime etc. Used when needing to display
    a value textually.

    This method is reimeplemented in the various subclasses.
*/

QString QAbstractSpinBoxPrivate::textFromValue(const QVariant &) const
{
    return QString();
}

/*!
    \internal

    Virtual method called that calls the public valueFromText()
    functions in the subclasses. Needed to change signature from
    QVariant to int/double/QDateTime etc. Used when needing to
    interpret a string as another type.

    This method is reimeplemented in the various subclasses.
*/

QVariant QAbstractSpinBoxPrivate::valueFromText(const QString &) const
{
    return QVariant();
}
/*!
    \internal

    Interprets text and emits signals. Called when the spinbox needs
    to interpret the text on the lineedit.
*/

void QAbstractSpinBoxPrivate::interpret(EmitPolicy ep)
{
    Q_Q(QAbstractSpinBox);
    if (type == QVariant::Invalid || cleared)
        return;

    QVariant v = getZeroVariant();
    bool doInterpret = true;
    QString tmp = edit->displayText();
    int pos = edit->cursorPosition();
    const int oldpos = pos;

    if (q->validate(tmp, pos) != QValidator::Acceptable) {
        const QString copy = tmp;
        q->fixup(tmp);
        QASBDEBUG() << "QAbstractSpinBoxPrivate::interpret() text '"
                    << edit->displayText()
                    << "' >> '" << copy << '\''
                    << "' >> '" << tmp << '\'';

        doInterpret = tmp != copy && (q->validate(tmp, pos) == QValidator::Acceptable);
        if (!doInterpret) {
            v = (correctionMode == QAbstractSpinBox::CorrectToNearestValue
                 ? variantBound(minimum, v, maximum) : value);
        }
    }
    if (doInterpret) {
        v = valueFromText(tmp);
    }
    clearCache();
    setValue(v, ep, true);
    if (oldpos != pos)
        edit->setCursorPosition(pos);
}

void QAbstractSpinBoxPrivate::clearCache() const
{
    cachedText.clear();
    cachedValue.clear();
    cachedState = QValidator::Acceptable;
}


// --- QSpinBoxValidator ---

/*!
    \internal
    Constructs a QSpinBoxValidator object
*/

QSpinBoxValidator::QSpinBoxValidator(QAbstractSpinBox *qp, QAbstractSpinBoxPrivate *dp)
    : QValidator(qp), qptr(qp), dptr(dp)
{
    setObjectName(QLatin1String("qt_spinboxvalidator"));
}

/*!
    \internal

    Checks for specialValueText, prefix, suffix and calls
    the virtual QAbstractSpinBox::validate function.
*/

QValidator::State QSpinBoxValidator::validate(QString &input, int &pos) const
{
    if (dptr->specialValueText.size() > 0 && input == dptr->specialValueText)
        return QValidator::Acceptable;

    if (!dptr->prefix.isEmpty() && !input.startsWith(dptr->prefix)) {
        input.prepend(dptr->prefix);
        pos += dptr->prefix.length();
    }

    if (!dptr->suffix.isEmpty() && !input.endsWith(dptr->suffix))
        input.append(dptr->suffix);

    return qptr->validate(input, pos);
}
/*!
    \internal
    Calls the virtual QAbstractSpinBox::fixup function.
*/

void QSpinBoxValidator::fixup(QString &input) const
{
    qptr->fixup(input);
}

// --- global ---

/*!
    \internal
    Adds two variants together and returns the result.
*/

QVariant operator+(const QVariant &arg1, const QVariant &arg2)
{
    QVariant ret;
    if (arg1.type() != arg2.type())
        qWarning("QAbstractSpinBox: Internal error: Different types (%s vs %s) (%s:%d)",
                 arg1.typeName(), arg2.typeName(), __FILE__, __LINE__);
    switch (arg1.type()) {
    case QVariant::Int: {
        const int int1 = arg1.toInt();
        const int int2 = arg2.toInt();
        if (int1 > 0 && (int2 >= INT_MAX - int1)) {
            // The increment overflows
            ret = QVariant(INT_MAX);
        } else if (int1 < 0 && (int2 <= INT_MIN - int1)) {
            // The increment underflows
            ret = QVariant(INT_MIN);
        } else {
            ret = QVariant(int1 + int2);
        }
        break;
    }
    case QVariant::Double: ret = QVariant(arg1.toDouble() + arg2.toDouble()); break;
    case QVariant::DateTime: {
        QDateTime a2 = arg2.toDateTime();
        QDateTime a1 = arg1.toDateTime().addDays(QDATETIMEEDIT_DATETIME_MIN.daysTo(a2));
        a1.setTime(a1.time().addMSecs(QTime().msecsTo(a2.time())));
        ret = QVariant(a1);
    }
    default: break;
    }
    return ret;
}


/*!
    \internal
    Subtracts two variants and returns the result.
*/

QVariant operator-(const QVariant &arg1, const QVariant &arg2)
{
    QVariant ret;
    if (arg1.type() != arg2.type())
        qWarning("QAbstractSpinBox: Internal error: Different types (%s vs %s) (%s:%d)",
                 arg1.typeName(), arg2.typeName(), __FILE__, __LINE__);
    switch (arg1.type()) {
    case QVariant::Int: ret = QVariant(arg1.toInt() - arg2.toInt()); break;
    case QVariant::Double: ret = QVariant(arg1.toDouble() - arg2.toDouble()); break;
    case QVariant::DateTime: {
        QDateTime a1 = arg1.toDateTime();
        QDateTime a2 = arg2.toDateTime();
        int days = a2.daysTo(a1);
        int secs = a2.secsTo(a1);
        int msecs = qMax(0, a1.time().msec() - a2.time().msec());
        if (days < 0 || secs < 0 || msecs < 0) {
            ret = arg1;
        } else {
            QDateTime dt = a2.addDays(days).addSecs(secs);
            if (msecs > 0)
                dt.setTime(dt.time().addMSecs(msecs));
            ret = QVariant(dt);
        }
    }
    default: break;
    }
    return ret;
}

/*!
    \internal
    Multiplies \a arg1 by \a multiplier and returns the result.
*/

QVariant operator*(const QVariant &arg1, double multiplier)
{
    QVariant ret;

    switch (arg1.type()) {
    case QVariant::Int:
        ret = static_cast<int>(qBound<double>(INT_MIN, arg1.toInt() * multiplier, INT_MAX));
        break;
    case QVariant::Double: ret = QVariant(arg1.toDouble() * multiplier); break;
    case QVariant::DateTime: {
        double days = QDATETIMEEDIT_DATE_MIN.daysTo(arg1.toDateTime().date()) * multiplier;
        int daysInt = (int)days;
        days -= daysInt;
        long msecs = (long)((QDATETIMEEDIT_TIME_MIN.msecsTo(arg1.toDateTime().time()) * multiplier)
                            + (days * (24 * 3600 * 1000)));
        ret = QDateTime(QDate().addDays(int(days)), QTime().addMSecs(msecs));
        break;
    }
    default: ret = arg1; break;
    }

    return ret;
}



double operator/(const QVariant &arg1, const QVariant &arg2)
{
    double a1 = 0;
    double a2 = 0;

    switch (arg1.type()) {
    case QVariant::Int:
        a1 = (double)arg1.toInt();
        a2 = (double)arg2.toInt();
        break;
    case QVariant::Double:
        a1 = arg1.toDouble();
        a2 = arg2.toDouble();
        break;
    case QVariant::DateTime:
        a1 = QDATETIMEEDIT_DATE_MIN.daysTo(arg1.toDate());
        a2 = QDATETIMEEDIT_DATE_MIN.daysTo(arg2.toDate());
        a1 += (double)QDATETIMEEDIT_TIME_MIN.msecsTo(arg1.toDateTime().time()) / (long)(3600 * 24 * 1000);
        a2 += (double)QDATETIMEEDIT_TIME_MIN.msecsTo(arg2.toDateTime().time()) / (long)(3600 * 24 * 1000);
    default: break;
    }

    return (a1 != 0 && a2 != 0) ? (a1 / a2) : 0.0;
}

int QAbstractSpinBoxPrivate::variantCompare(const QVariant &arg1, const QVariant &arg2)
{
    switch (arg2.type()) {
    case QVariant::Date:
        Q_ASSERT_X(arg1.type() == QVariant::Date, "QAbstractSpinBoxPrivate::variantCompare",
                   qPrintable(QString::fromLatin1("Internal error 1 (%1)").
                              arg(QString::fromLatin1(arg1.typeName()))));
        if (arg1.toDate() == arg2.toDate()) {
            return 0;
        } else if (arg1.toDate() < arg2.toDate()) {
            return -1;
        } else {
            return 1;
        }
    case QVariant::Time:
        Q_ASSERT_X(arg1.type() == QVariant::Time, "QAbstractSpinBoxPrivate::variantCompare",
                   qPrintable(QString::fromLatin1("Internal error 2 (%1)").
                              arg(QString::fromLatin1(arg1.typeName()))));
        if (arg1.toTime() == arg2.toTime()) {
            return 0;
        } else if (arg1.toTime() < arg2.toTime()) {
            return -1;
        } else {
            return 1;
        }


    case QVariant::DateTime:
        if (arg1.toDateTime() == arg2.toDateTime()) {
            return 0;
        } else if (arg1.toDateTime() < arg2.toDateTime()) {
            return -1;
        } else {
            return 1;
        }
    case QVariant::Int:
        if (arg1.toInt() == arg2.toInt()) {
            return 0;
        } else if (arg1.toInt() < arg2.toInt()) {
            return -1;
        } else {
            return 1;
        }
    case QVariant::Double:
        if (arg1.toDouble() == arg2.toDouble()) {
            return 0;
        } else if (arg1.toDouble() < arg2.toDouble()) {
            return -1;
        } else {
            return 1;
        }
    case QVariant::Invalid:
        if (arg2.type() == QVariant::Invalid)
            return 0;
    default:
        Q_ASSERT_X(0, "QAbstractSpinBoxPrivate::variantCompare",
                   qPrintable(QString::fromLatin1("Internal error 3 (%1 %2)").
                              arg(QString::fromLatin1(arg1.typeName())).
                              arg(QString::fromLatin1(arg2.typeName()))));
    }
    return -2;
}

QVariant QAbstractSpinBoxPrivate::variantBound(const QVariant &min,
                                               const QVariant &value,
                                               const QVariant &max)
{
    Q_ASSERT(variantCompare(min, max) <= 0);
    if (variantCompare(min, value) < 0) {
        const int compMax = variantCompare(value, max);
        return (compMax < 0 ? value : max);
    } else {
        return min;
    }
}


QT_END_NAMESPACE

#include "moc_qabstractspinbox.cpp"

#endif // QT_NO_SPINBOX
