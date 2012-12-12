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

#include "qabstractbutton.h"
#include "qabstractitemview.h"
#include "qbuttongroup.h"
#include "qabstractbutton_p.h"
#include "qevent.h"
#include "qpainter.h"
#include "qapplication.h"
#include "qstyle.h"
#include "qaction.h"
#ifndef QT_NO_ACCESSIBILITY
#include "qaccessible.h"
#endif

QT_BEGIN_NAMESPACE

#define AUTO_REPEAT_DELAY  300
#define AUTO_REPEAT_INTERVAL 100

Q_GUI_EXPORT extern bool qt_tab_all_widgets;

/*!
    \class QAbstractButton

    \brief The QAbstractButton class is the abstract base class of
    button widgets, providing functionality common to buttons.

    \ingroup abstractwidgets

    This class implements an \e abstract button.
    Subclasses of this class handle user actions, and specify how the button
    is drawn.

    QAbstractButton provides support for both push buttons and checkable
    (toggle) buttons. Checkable buttons are implemented in the QRadioButton
    and QCheckBox classes. Push buttons are implemented in the
    QPushButton and QToolButton classes; these also provide toggle
    behavior if required.

    Any button can display a label containing text and an icon. setText()
    sets the text; setIcon() sets the icon. If a button is disabled, its label
    is changed to give the button a "disabled" appearance.

    If the button is a text button with a string containing an
    ampersand ('&'), QAbstractButton automatically creates a shortcut
    key. For example:

    \snippet doc/src/snippets/code/src_gui_widgets_qabstractbutton.cpp 0

    The \key Alt+C shortcut is assigned to the button, i.e., when the
    user presses \key Alt+C the button will call animateClick(). See
    the \l {QShortcut#mnemonic}{QShortcut} documentation for details
    (to display an actual ampersand, use '&&').

    You can also set a custom shortcut key using the setShortcut()
    function. This is useful mostly for buttons that do not have any
    text, because they have no automatic shortcut.

    \snippet doc/src/snippets/code/src_gui_widgets_qabstractbutton.cpp 1

    All of the buttons provided by Qt (QPushButton, QToolButton,
    QCheckBox, and QRadioButton) can display both \l text and \l{icon}{icons}.

    A button can be made the default button in a dialog are provided by
    QPushButton::setDefault() and QPushButton::setAutoDefault().

    QAbstractButton provides most of the states used for buttons:

    \list

    \o isDown() indicates whether the button is \e pressed down.

    \o isChecked() indicates whether the button is \e checked.  Only
    checkable buttons can be checked and unchecked (see below).

    \o isEnabled() indicates whether the button can be pressed by the
    user. \note As opposed to other widgets, buttons derived from
    QAbstractButton accepts mouse and context menu events
    when disabled.

    \o setAutoRepeat() sets whether the button will auto-repeat if the
    user holds it down. \l autoRepeatDelay and \l autoRepeatInterval
    define how auto-repetition is done.

    \o setCheckable() sets whether the button is a toggle button or not.

    \endlist

    The difference between isDown() and isChecked() is as follows.
    When the user clicks a toggle button to check it, the button is first
    \e pressed then released into the \e checked state. When the user
    clicks it again (to uncheck it), the button moves first to the
    \e pressed state, then to the \e unchecked state (isChecked() and
    isDown() are both false).

    QAbstractButton provides four signals:

    \list 1

    \o pressed() is emitted when the left mouse button is pressed while
    the mouse cursor is inside the button.

    \o released() is emitted when the left mouse button is released.

    \o clicked() is emitted when the button is first pressed and then
    released, when the shortcut key is typed, or when click() or
    animateClick() is called.

    \o toggled() is emitted when the state of a toggle button changes.

    \endlist

    To subclass QAbstractButton, you must reimplement at least
    paintEvent() to draw the button's outline and its text or pixmap. It
    is generally advisable to reimplement sizeHint() as well, and
    sometimes hitButton() (to determine whether a button press is within
    the button). For buttons with more than two states (like tri-state
    buttons), you will also have to reimplement checkStateSet() and
    nextCheckState().

    \sa QButtonGroup
*/

QAbstractButtonPrivate::QAbstractButtonPrivate(QSizePolicy::ControlType type)
    :
#ifndef QT_NO_SHORTCUT
    shortcutId(0),
#endif
    checkable(false), checked(false), autoRepeat(false), autoExclusive(false),
    down(false), blockRefresh(false), pressed(false),
#ifndef QT_NO_BUTTONGROUP
    group(0),
#endif
    autoRepeatDelay(AUTO_REPEAT_DELAY),
    autoRepeatInterval(AUTO_REPEAT_INTERVAL),
    controlType(type)
{}

#ifndef QT_NO_BUTTONGROUP

class QButtonGroupPrivate: public QObjectPrivate
{
    Q_DECLARE_PUBLIC(QButtonGroup)

public:
    QButtonGroupPrivate():exclusive(true){}
    QList<QAbstractButton *> buttonList;
    QPointer<QAbstractButton> checkedButton;
    void detectCheckedButton();
    void notifyChecked(QAbstractButton *button);
    bool exclusive;
    QMap<QAbstractButton*, int> mapping;
};

QButtonGroup::QButtonGroup(QObject *parent)
    : QObject(*new QButtonGroupPrivate, parent)
{
}

QButtonGroup::~QButtonGroup()
{
    Q_D(QButtonGroup);
    for (int i = 0; i < d->buttonList.count(); ++i)
        d->buttonList.at(i)->d_func()->group = 0;
}


bool QButtonGroup::exclusive() const
{
    Q_D(const QButtonGroup);
    return d->exclusive;
}

void QButtonGroup::setExclusive(bool exclusive)
{
    Q_D(QButtonGroup);
    d->exclusive = exclusive;
}


// TODO: Qt 5: Merge with addButton(QAbstractButton *button, int id)
void QButtonGroup::addButton(QAbstractButton *button)
{
    addButton(button, -1);
}

void QButtonGroup::addButton(QAbstractButton *button, int id)
{
    Q_D(QButtonGroup);
    if (QButtonGroup *previous = button->d_func()->group)
        previous->removeButton(button);
    button->d_func()->group = this;
    d->buttonList.append(button);
    if (id == -1) {
        QList<int> ids = d->mapping.values();
        if (ids.isEmpty())
           d->mapping[button] = -2;
        else {
            qSort(ids);
            d->mapping[button] = ids.first()-1;
        }
    } else {
        d->mapping[button] = id;
    }

    if (d->exclusive && button->isChecked())
        button->d_func()->notifyChecked();
}

void QButtonGroup::removeButton(QAbstractButton *button)
{
    Q_D(QButtonGroup);
    if (d->checkedButton == button) {
        d->detectCheckedButton();
    }
    if (button->d_func()->group == this) {
        button->d_func()->group = 0;
        d->buttonList.removeAll(button);
        d->mapping.remove(button);
    }
}

QList<QAbstractButton*> QButtonGroup::buttons() const
{
    Q_D(const QButtonGroup);
    return d->buttonList;
}

QAbstractButton *QButtonGroup::checkedButton() const
{
    Q_D(const QButtonGroup);
    return d->checkedButton;
}

QAbstractButton *QButtonGroup::button(int id) const
{
    Q_D(const QButtonGroup);
    return d->mapping.key(id);
}

void QButtonGroup::setId(QAbstractButton *button, int id)
{
    Q_D(QButtonGroup);
    if (button && id != -1)
        d->mapping[button] = id;
}

int QButtonGroup::id(QAbstractButton *button) const
{
    Q_D(const QButtonGroup);
    return d->mapping.value(button, -1);
}

int QButtonGroup::checkedId() const
{
    Q_D(const QButtonGroup);
    return d->mapping.value(d->checkedButton, -1);
}

// detect a checked button other than the current one
void QButtonGroupPrivate::detectCheckedButton()
{
    QAbstractButton *previous = checkedButton;
    checkedButton = 0;
    if (exclusive)
        return;
    for (int i = 0; i < buttonList.count(); i++) {
        if (buttonList.at(i) != previous && buttonList.at(i)->isChecked()) {
            checkedButton = buttonList.at(i);
            return;
        }
    }
}

#endif // QT_NO_BUTTONGROUP

QList<QAbstractButton *>QAbstractButtonPrivate::queryButtonList() const
{
#ifndef QT_NO_BUTTONGROUP
    if (group)
        return group->d_func()->buttonList;
#endif

    QList<QAbstractButton*>candidates = parent->findChildren<QAbstractButton *>();
    if (autoExclusive) {
        for (int i = candidates.count() - 1; i >= 0; --i) {
            QAbstractButton *candidate = candidates.at(i);
            if (!candidate->autoExclusive()
#ifndef QT_NO_BUTTONGROUP
                || candidate->group()
#endif
                )
                candidates.removeAt(i);
        }
    }
    return candidates;
}

QAbstractButton *QAbstractButtonPrivate::queryCheckedButton() const
{
#ifndef QT_NO_BUTTONGROUP
    if (group)
        return group->d_func()->checkedButton;
#endif

    Q_Q(const QAbstractButton);
    QList<QAbstractButton *> buttonList = queryButtonList();
    if (!autoExclusive || buttonList.count() == 1) // no group
        return 0;

    for (int i = 0; i < buttonList.count(); ++i) {
        QAbstractButton *b = buttonList.at(i);
        if (b->d_func()->checked && b != q)
            return b;
    }
    return checked  ? const_cast<QAbstractButton *>(q) : 0;
}

void QAbstractButtonPrivate::notifyChecked()
{
#ifndef QT_NO_BUTTONGROUP
    Q_Q(QAbstractButton);
    if (group) {
        QAbstractButton *previous = group->d_func()->checkedButton;
        group->d_func()->checkedButton = q;
        if (group->d_func()->exclusive && previous && previous != q)
            previous->nextCheckState();
    } else
#endif
    if (autoExclusive) {
        if (QAbstractButton *b = queryCheckedButton())
            b->setChecked(false);
    }
}

void QAbstractButtonPrivate::moveFocus(int key)
{
    QList<QAbstractButton *> buttonList = queryButtonList();;
#ifndef QT_NO_BUTTONGROUP
    bool exclusive = group ? group->d_func()->exclusive : autoExclusive;
#else
    bool exclusive = autoExclusive;
#endif
    QWidget *f = QApplication::focusWidget();
    QAbstractButton *fb = qobject_cast<QAbstractButton *>(f);
    if (!fb || !buttonList.contains(fb))
        return;
    
    QAbstractButton *candidate = 0;
    int bestScore = -1;
    QRect target = f->rect().translated(f->mapToGlobal(QPoint(0,0)));
    QPoint goal = target.center();
    uint focus_flag = qt_tab_all_widgets ? Qt::TabFocus : Qt::StrongFocus;

    for (int i = 0; i < buttonList.count(); ++i) {
        QAbstractButton *button = buttonList.at(i);
        if (button != f && button->window() == f->window() && button->isEnabled() && !button->isHidden() &&
            (autoExclusive || (button->focusPolicy() & focus_flag) == focus_flag)) {
            QRect buttonRect = button->rect().translated(button->mapToGlobal(QPoint(0,0)));
            QPoint p = buttonRect.center();

            //Priority to widgets that overlap on the same coordinate.
            //In that case, the distance in the direction will be used as significant score,
            //take also in account orthogonal distance in case two widget are in the same distance.
            int score;
            if ((buttonRect.x() < target.right() && target.x() < buttonRect.right())
                  && (key == Qt::Key_Up || key == Qt::Key_Down)) {
                //one item's is at the vertical of the other
                score = (qAbs(p.y() - goal.y()) << 16) + qAbs(p.x() - goal.x());
            } else if ((buttonRect.y() < target.bottom() && target.y() < buttonRect.bottom())
                        && (key == Qt::Key_Left || key == Qt::Key_Right) ) {
                //one item's is at the horizontal of the other
                score = (qAbs(p.x() - goal.x()) << 16) + qAbs(p.y() - goal.y());
            } else {
                score = (1 << 30) + (p.y() - goal.y()) * (p.y() - goal.y()) + (p.x() - goal.x()) * (p.x() - goal.x());
            }

            if (score > bestScore && candidate)
                continue;

            switch(key) {
            case Qt::Key_Up:
                if (p.y() < goal.y()) {
                    candidate = button;
                    bestScore = score;
                }
                break;
            case Qt::Key_Down:
                if (p.y() > goal.y()) {
                    candidate = button;
                    bestScore = score;
                }
                break;
            case Qt::Key_Left:
                if (p.x() < goal.x()) {
                    candidate = button;
                    bestScore = score;
                }
                break;
            case Qt::Key_Right:
                if (p.x() > goal.x()) {
                    candidate = button;
                    bestScore = score;
                }
                break;
            }
        }
    }

    if (exclusive
#ifdef QT_KEYPAD_NAVIGATION
        && !QApplication::keypadNavigationEnabled()
#endif
        && candidate
        && fb->d_func()->checked
        && candidate->d_func()->checkable)
        candidate->click();

    if (candidate) {
        if (key == Qt::Key_Up || key == Qt::Key_Left)
            candidate->setFocus(Qt::BacktabFocusReason);
        else
            candidate->setFocus(Qt::TabFocusReason);
    }
}

void QAbstractButtonPrivate::fixFocusPolicy()
{
    Q_Q(QAbstractButton);
#ifndef QT_NO_BUTTONGROUP
    if (!group && !autoExclusive)
#else
    if (!autoExclusive)
#endif
        return;

    QList<QAbstractButton *> buttonList = queryButtonList();
    for (int i = 0; i < buttonList.count(); ++i) {
        QAbstractButton *b = buttonList.at(i);
        if (!b->isCheckable())
            continue;
        b->setFocusPolicy((Qt::FocusPolicy) ((b == q || !q->isCheckable())
                                         ? (b->focusPolicy() | Qt::TabFocus)
                                         :  (b->focusPolicy() & ~Qt::TabFocus)));
    }
}

void QAbstractButtonPrivate::init()
{
    Q_Q(QAbstractButton);

    q->setFocusPolicy(Qt::FocusPolicy(q->style()->styleHint(QStyle::SH_Button_FocusPolicy)));
    q->setSizePolicy(QSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed, controlType));
    q->setAttribute(Qt::WA_WState_OwnSizePolicy, false);
    q->setForegroundRole(QPalette::ButtonText);
    q->setBackgroundRole(QPalette::Button);
}

void QAbstractButtonPrivate::refresh()
{
    Q_Q(QAbstractButton);

    if (blockRefresh)
        return;
    q->update();
#ifndef QT_NO_ACCESSIBILITY
    QAccessible::updateAccessibility(q, 0, QAccessible::StateChanged);
#endif
}

void QAbstractButtonPrivate::click()
{
    Q_Q(QAbstractButton);

    down = false;
    blockRefresh = true;
    bool changeState = true;
    if (checked && queryCheckedButton() == q) {
        // the checked button of an exclusive or autoexclusive group cannot be unchecked
#ifndef QT_NO_BUTTONGROUP
        if (group ? group->d_func()->exclusive : autoExclusive)
#else
        if (autoExclusive)
#endif
            changeState = false;
    }

    QPointer<QAbstractButton> guard(q);
    if (changeState) {
        q->nextCheckState();
        if (!guard)
            return;
    }
    blockRefresh = false;
    refresh();
    q->repaint(); //flush paint event before invoking potentially expensive operation
    QApplication::flush();
    if (guard)
        emitReleased();
    if (guard)
        emitClicked();
}

void QAbstractButtonPrivate::emitClicked()
{
    Q_Q(QAbstractButton);
    QPointer<QAbstractButton> guard(q);
    emit q->clicked(checked);
#ifndef QT_NO_BUTTONGROUP
    if (guard && group) {
        emit group->buttonClicked(group->id(q));
        if (guard && group)
            emit group->buttonClicked(q);
    }
#endif
}

void QAbstractButtonPrivate::emitPressed()
{
    Q_Q(QAbstractButton);
    QPointer<QAbstractButton> guard(q);
    emit q->pressed();
#ifndef QT_NO_BUTTONGROUP
    if (guard && group) {
        emit group->buttonPressed(group->id(q));
        if (guard && group)
            emit group->buttonPressed(q);
    }
#endif
}

void QAbstractButtonPrivate::emitReleased()
{
    Q_Q(QAbstractButton);
    QPointer<QAbstractButton> guard(q);
    emit q->released();
#ifndef QT_NO_BUTTONGROUP
    if (guard && group) {
        emit group->buttonReleased(group->id(q));
        if (guard && group)
            emit group->buttonReleased(q);
    }
#endif
}

/*!
    Constructs an abstract button with a \a parent.
*/
QAbstractButton::QAbstractButton(QWidget *parent)
    : QWidget(*new QAbstractButtonPrivate, parent, 0)
{
    Q_D(QAbstractButton);
    d->init();
}

/*!
    Destroys the button.
 */
 QAbstractButton::~QAbstractButton()
{
#ifndef QT_NO_BUTTONGROUP
    Q_D(QAbstractButton);
    if (d->group)
        d->group->removeButton(this);
#endif
}


/*! \internal
 */
QAbstractButton::QAbstractButton(QAbstractButtonPrivate &dd, QWidget *parent)
    : QWidget(dd, parent, 0)
{
    Q_D(QAbstractButton);
    d->init();
}

/*!
\property QAbstractButton::text
\brief the text shown on the button

If the button has no text, the text() function will return a an empty
string.

If the text contains an ampersand character ('&'), a shortcut is
automatically created for it. The character that follows the '&' will
be used as the shortcut key. Any previous shortcut will be
overwritten, or cleared if no shortcut is defined by the text. See the
\l {QShortcut#mnemonic}{QShortcut} documentation for details (to
display an actual ampersand, use '&&').

There is no default text.
*/

void QAbstractButton::setText(const QString &text)
{
    Q_D(QAbstractButton);
    if (d->text == text)
        return;
    d->text = text;
#ifndef QT_NO_SHORTCUT
    QKeySequence newMnemonic = QKeySequence::mnemonic(text);
    setShortcut(newMnemonic);
#endif
    d->sizeHint = QSize();
    update();
    updateGeometry();
#ifndef QT_NO_ACCESSIBILITY
    QAccessible::updateAccessibility(this, 0, QAccessible::NameChanged);
#endif
}

QString QAbstractButton::text() const
{
    Q_D(const QAbstractButton);
    return d->text;
}


/*!
  \property QAbstractButton::icon
  \brief the icon shown on the button

  The icon's default size is defined by the GUI style, but can be
  adjusted by setting the \l iconSize property.
*/
void QAbstractButton::setIcon(const QIcon &icon)
{
    Q_D(QAbstractButton);
    d->icon = icon;
    d->sizeHint = QSize();
    update();
    updateGeometry();
}

QIcon QAbstractButton::icon() const
{
    Q_D(const QAbstractButton);
    return d->icon;
}

#ifndef QT_NO_SHORTCUT
/*!
\property QAbstractButton::shortcut
\brief the mnemonic associated with the button
*/

void QAbstractButton::setShortcut(const QKeySequence &key)
{
    Q_D(QAbstractButton);
    if (d->shortcutId != 0)
        releaseShortcut(d->shortcutId);
    d->shortcut = key;
    d->shortcutId = grabShortcut(key);
}

QKeySequence QAbstractButton::shortcut() const
{
    Q_D(const QAbstractButton);
    return d->shortcut;
}
#endif // QT_NO_SHORTCUT

/*!
\property QAbstractButton::checkable
\brief whether the button is checkable

By default, the button is not checkable.

\sa checked
*/
void QAbstractButton::setCheckable(bool checkable)
{
    Q_D(QAbstractButton);
    if (d->checkable == checkable)
        return;

    d->checkable = checkable;
    d->checked = false;
}

bool QAbstractButton::isCheckable() const
{
    Q_D(const QAbstractButton);
    return d->checkable;
}

/*!
\property QAbstractButton::checked
\brief whether the button is checked

Only checkable buttons can be checked. By default, the button is unchecked.

\sa checkable
*/
void QAbstractButton::setChecked(bool checked)
{
    Q_D(QAbstractButton);
    if (!d->checkable || d->checked == checked) {
        if (!d->blockRefresh)
            checkStateSet();
        return;
    }

    if (!checked && d->queryCheckedButton() == this) {
        // the checked button of an exclusive or autoexclusive group cannot be  unchecked
#ifndef QT_NO_BUTTONGROUP
        if (d->group ? d->group->d_func()->exclusive : d->autoExclusive)
            return;
        if (d->group)
            d->group->d_func()->detectCheckedButton();
#else
        if (d->autoExclusive)
            return;
#endif
    }

    QPointer<QAbstractButton> guard(this);

    d->checked = checked;
    if (!d->blockRefresh)
        checkStateSet();
    d->refresh();

    if (guard && checked)
        d->notifyChecked();
    if (guard)
        emit toggled(checked);
}

bool QAbstractButton::isChecked() const
{
    Q_D(const QAbstractButton);
    return d->checked;
}

/*!
  \property QAbstractButton::down
  \brief whether the button is pressed down

  If this property is true, the button is pressed down. The signals
  pressed() and clicked() are not emitted if you set this property
  to true. The default is false.
*/

void QAbstractButton::setDown(bool down)
{
    Q_D(QAbstractButton);
    if (d->down == down)
        return;
    d->down = down;
    d->refresh();
    if (d->autoRepeat && d->down)
        d->repeatTimer.start(d->autoRepeatDelay, this);
    else
        d->repeatTimer.stop();
}

bool QAbstractButton::isDown() const
{
    Q_D(const QAbstractButton);
    return d->down;
}

/*!
\property QAbstractButton::autoRepeat
\brief whether autoRepeat is enabled

If autoRepeat is enabled, then the pressed(), released(), and clicked() signals are emitted at
regular intervals when the button is down. autoRepeat is off by default.
The initial delay and the repetition interval are defined in milliseconds by \l
autoRepeatDelay and \l autoRepeatInterval.

Note: If a button is pressed down by a shortcut key, then auto-repeat is enabled and timed by the
system and not by this class. The pressed(), released(), and clicked() signals will be emitted
like in the normal case.
*/

void QAbstractButton::setAutoRepeat(bool autoRepeat)
{
    Q_D(QAbstractButton);
    if (d->autoRepeat == autoRepeat)
        return;
    d->autoRepeat = autoRepeat;
    if (d->autoRepeat && d->down)
        d->repeatTimer.start(d->autoRepeatDelay, this);
    else
        d->repeatTimer.stop();
}

bool QAbstractButton::autoRepeat() const
{
    Q_D(const QAbstractButton);
    return d->autoRepeat;
}

/*!
    \property QAbstractButton::autoRepeatDelay
    \brief the initial delay of auto-repetition
    \since 4.2

    If \l autoRepeat is enabled, then autoRepeatDelay defines the initial
    delay in milliseconds before auto-repetition kicks in.

    \sa autoRepeat, autoRepeatInterval
*/

void QAbstractButton::setAutoRepeatDelay(int autoRepeatDelay)
{
    Q_D(QAbstractButton);
    d->autoRepeatDelay = autoRepeatDelay;
}

int QAbstractButton::autoRepeatDelay() const
{
    Q_D(const QAbstractButton);
    return d->autoRepeatDelay;
}

/*!
    \property QAbstractButton::autoRepeatInterval
    \brief the interval of auto-repetition
    \since 4.2

    If \l autoRepeat is enabled, then autoRepeatInterval defines the
    length of the auto-repetition interval in millisecons.

    \sa autoRepeat, autoRepeatDelay
*/

void QAbstractButton::setAutoRepeatInterval(int autoRepeatInterval)
{
    Q_D(QAbstractButton);
    d->autoRepeatInterval = autoRepeatInterval;
}

int QAbstractButton::autoRepeatInterval() const
{
    Q_D(const QAbstractButton);
    return d->autoRepeatInterval;
}



/*!
\property QAbstractButton::autoExclusive
\brief whether auto-exclusivity is enabled

If auto-exclusivity is enabled, checkable buttons that belong to the
same parent widget behave as if they were part of the same
exclusive button group. In an exclusive button group, only one button
can be checked at any time; checking another button automatically
unchecks the previously checked one.

The property has no effect on buttons that belong to a button
group.

autoExclusive is off by default, except for radio buttons.

\sa QRadioButton
*/
void QAbstractButton::setAutoExclusive(bool autoExclusive)
{
    Q_D(QAbstractButton);
    d->autoExclusive = autoExclusive;
}

bool QAbstractButton::autoExclusive() const
{
    Q_D(const QAbstractButton);
    return d->autoExclusive;
}

#ifndef QT_NO_BUTTONGROUP
/*!
  Returns the group that this button belongs to.

  If the button is not a member of any QButtonGroup, this function
  returns 0.

  \sa QButtonGroup
*/
QButtonGroup *QAbstractButton::group() const
{
    Q_D(const QAbstractButton);
    return d->group;
}
#endif // QT_NO_BUTTONGROUP

/*!
Performs an animated click: the button is pressed immediately, and
released \a msec milliseconds later (the default is 100 ms).

Calling this function again before the button was released will reset
the release timer.

All signals associated with a click are emitted as appropriate.

This function does nothing if the button is \link setEnabled()
disabled. \endlink

\sa click()
*/
void QAbstractButton::animateClick(int msec)
{
    if (!isEnabled())
        return;
    Q_D(QAbstractButton);
    if (d->checkable && focusPolicy() & Qt::ClickFocus)
        setFocus();
    setDown(true);
    repaint(); //flush paint event before invoking potentially expensive operation
    QApplication::flush();
    if (!d->animateTimer.isActive())
        d->emitPressed();
    d->animateTimer.start(msec, this);
}

/*!
Performs a click.

All the usual signals associated with a click are emitted as
appropriate. If the button is checkable, the state of the button is
toggled.

This function does nothing if the button is \link setEnabled()
disabled. \endlink

\sa animateClick()
 */
void QAbstractButton::click()
{
    if (!isEnabled())
        return;
    Q_D(QAbstractButton);
    QPointer<QAbstractButton> guard(this);
    d->down = true;
    d->emitPressed();
    if (guard) {
        d->down = false;
        nextCheckState();
        if (guard)
            d->emitReleased();
        if (guard)
            d->emitClicked();
    }
}

/*! \fn void QAbstractButton::toggle()

    Toggles the state of a checkable button.

     \sa checked
*/
void QAbstractButton::toggle()
{
    Q_D(QAbstractButton);
    setChecked(!d->checked);
}


/*! This virtual handler is called when setChecked() was called,
unless it was called from within nextCheckState(). It allows
subclasses to reset their intermediate button states.

\sa nextCheckState()
 */
void QAbstractButton::checkStateSet()
{
}

/*! This virtual handler is called when a button is clicked. The
default implementation calls setChecked(!isChecked()) if the button
isCheckable().  It allows subclasses to implement intermediate button
states.

\sa checkStateSet()
*/
void QAbstractButton::nextCheckState()
{
    if (isCheckable())
        setChecked(!isChecked());
}

/*!
Returns true if \a pos is inside the clickable button rectangle;
otherwise returns false.

By default, the clickable area is the entire widget. Subclasses
may reimplement this function to provide support for clickable
areas of different shapes and sizes.
*/
bool QAbstractButton::hitButton(const QPoint &pos) const
{
    return rect().contains(pos);
}

/*! \reimp */
bool QAbstractButton::event(QEvent *e)
{
    // as opposed to other widgets, disabled buttons accept mouse
    // events. This avoids surprising click-through scenarios
    if (!isEnabled()) {
        switch(e->type()) {
        case QEvent::TabletPress:
        case QEvent::TabletRelease:
        case QEvent::TabletMove:
        case QEvent::MouseButtonPress:
        case QEvent::MouseButtonRelease:
        case QEvent::MouseButtonDblClick:
        case QEvent::MouseMove:
        case QEvent::HoverMove:
        case QEvent::HoverEnter:
        case QEvent::HoverLeave:
        case QEvent::ContextMenu:
#ifndef QT_NO_WHEELEVENT
        case QEvent::Wheel:
#endif
            return true;
        default:
            break;
        }
    }

#ifndef QT_NO_SHORTCUT
    if (e->type() == QEvent::Shortcut) {
        Q_D(QAbstractButton);
        QShortcutEvent *se = static_cast<QShortcutEvent *>(e);
        if (d->shortcutId != se->shortcutId())
            return false;
        if (!se->isAmbiguous()) {
            if (!d->animateTimer.isActive())
                animateClick();
        } else {
            if (focusPolicy() != Qt::NoFocus)
                setFocus(Qt::ShortcutFocusReason);
            window()->setAttribute(Qt::WA_KeyboardFocusChange);
        }
        return true;
    }
#endif
    return QWidget::event(e);
}

/*! \reimp */
void QAbstractButton::mousePressEvent(QMouseEvent *e)
{
    Q_D(QAbstractButton);
    if (e->button() != Qt::LeftButton) {
        e->ignore();
        return;
    }
    if (hitButton(e->pos())) {
        setDown(true);
        d->pressed = true;
        repaint(); //flush paint event before invoking potentially expensive operation
        QApplication::flush();
        d->emitPressed();
        e->accept();
    } else {
        e->ignore();
    }
}

/*! \reimp */
void QAbstractButton::mouseReleaseEvent(QMouseEvent *e)
{
    Q_D(QAbstractButton);
    d->pressed = false;

    if (e->button() != Qt::LeftButton) {
        e->ignore();
        return;
    }

    if (!d->down) {
        e->ignore();
        return;
    }

    if (hitButton(e->pos())) {
        d->repeatTimer.stop();
        d->click();
        e->accept();
    } else {
        setDown(false);
        e->ignore();
    }
}

/*! \reimp */
void QAbstractButton::mouseMoveEvent(QMouseEvent *e)
{
    Q_D(QAbstractButton);
    if (!(e->buttons() & Qt::LeftButton) || !d->pressed) {
        e->ignore();
        return;
    }

    if (hitButton(e->pos()) != d->down) {
        setDown(!d->down);
        repaint(); //flush paint event before invoking potentially expensive operation
        QApplication::flush();
        if (d->down)
            d->emitPressed();
        else
            d->emitReleased();
        e->accept();
    } else if (!hitButton(e->pos())) {
        e->ignore();
    }
}

/*! \reimp */
void QAbstractButton::keyPressEvent(QKeyEvent *e)
{
    Q_D(QAbstractButton);
    bool next = true;
    switch (e->key()) {
    case Qt::Key_Enter:
    case Qt::Key_Return:
        e->ignore();
        break;
    case Qt::Key_Select:
    case Qt::Key_Space:
        if (!e->isAutoRepeat()) {
            setDown(true);
            repaint(); //flush paint event before invoking potentially expensive operation
            QApplication::flush();
            d->emitPressed();
        }
        break;
    case Qt::Key_Up:
    case Qt::Key_Left:
        next = false;
        // fall through
    case Qt::Key_Right:
    case Qt::Key_Down:
#ifdef QT_KEYPAD_NAVIGATION
        if ((QApplication::keypadNavigationEnabled()
                && (e->key() == Qt::Key_Left || e->key() == Qt::Key_Right))
                || (!QApplication::navigationMode() == Qt::NavigationModeKeypadDirectional
                || (e->key() == Qt::Key_Up || e->key() == Qt::Key_Down))) {
            e->ignore();
            return;
        }
#endif
        QWidget *pw;
        if (d->autoExclusive
#ifndef QT_NO_BUTTONGROUP
        || d->group
#endif
#ifndef QT_NO_ITEMVIEWS
        || ((pw = parentWidget()) && qobject_cast<QAbstractItemView *>(pw->parentWidget()))
#endif
        ) {
            // ### Using qobject_cast to check if the parent is a viewport of
            // QAbstractItemView is a crude hack, and should be revisited and
            // cleaned up when fixing task 194373. It's here to ensure that we
            // keep compatibility outside QAbstractItemView.
            d->moveFocus(e->key());
            if (hasFocus()) // nothing happend, propagate
                e->ignore();
        } else {
            focusNextPrevChild(next);
        }
        break;
    case Qt::Key_Escape:
        if (d->down) {
            setDown(false);
            repaint(); //flush paint event before invoking potentially expensive operation
            QApplication::flush();
            d->emitReleased();
            break;
        }
        // fall through
    default:
        e->ignore();
    }
}

/*! \reimp */
void QAbstractButton::keyReleaseEvent(QKeyEvent *e)
{
    Q_D(QAbstractButton);

    if (!e->isAutoRepeat())
        d->repeatTimer.stop();

    switch (e->key()) {
    case Qt::Key_Select:
    case Qt::Key_Space:
        if (!e->isAutoRepeat() && d->down)
            d->click();
        break;
    default:
        e->ignore();
    }
}

/*!\reimp
 */
void QAbstractButton::timerEvent(QTimerEvent *e)
{
    Q_D(QAbstractButton);
    if (e->timerId() == d->repeatTimer.timerId()) {
        d->repeatTimer.start(d->autoRepeatInterval, this);
        if (d->down) {
            QPointer<QAbstractButton> guard(this);
            nextCheckState();
            if (guard)
                d->emitReleased();
            if (guard)
                d->emitClicked();
            if (guard)
                d->emitPressed();
        }
    } else if (e->timerId() == d->animateTimer.timerId()) {
        d->animateTimer.stop();
        d->click();
    }
}

/*! \reimp */
void QAbstractButton::focusInEvent(QFocusEvent *e)
{
    Q_D(QAbstractButton);
#ifdef QT_KEYPAD_NAVIGATION
    if (!QApplication::keypadNavigationEnabled())
#endif
    d->fixFocusPolicy();
    QWidget::focusInEvent(e);
}

/*! \reimp */
void QAbstractButton::focusOutEvent(QFocusEvent *e)
{
    Q_D(QAbstractButton);
    if (e->reason() != Qt::PopupFocusReason)
        d->down = false;
    QWidget::focusOutEvent(e);
}

/*! \reimp */
void QAbstractButton::changeEvent(QEvent *e)
{
    Q_D(QAbstractButton);
    switch (e->type()) {
    case QEvent::EnabledChange:
        if (!isEnabled())
            setDown(false);
        break;
    default:
        d->sizeHint = QSize();
        break;
    }
    QWidget::changeEvent(e);
}

/*!
    \fn void QAbstractButton::paintEvent(QPaintEvent *e)
    \reimp
*/

/*!
    \fn void QAbstractButton::pressed()

    This signal is emitted when the button is pressed down.

    \sa released(), clicked()
*/

/*!
    \fn void QAbstractButton::released()

    This signal is emitted when the button is released.

    \sa pressed(), clicked(), toggled()
*/

/*!
\fn void QAbstractButton::clicked(bool checked)

This signal is emitted when the button is activated (i.e. pressed down
then released while the mouse cursor is inside the button), when the
shortcut key is typed, or when click() or animateClick() is called.
Notably, this signal is \e not emitted if you call setDown(),
setChecked() or toggle().

If the button is checkable, \a checked is true if the button is
checked, or false if the button is unchecked.

\sa pressed(), released(), toggled()
*/

/*!
\fn void QAbstractButton::toggled(bool checked)

This signal is emitted whenever a checkable button changes its state.
\a checked is true if the button is checked, or false if the button is
unchecked.

This may be the result of a user action, click() slot activation,
or because setChecked() was called.

The states of buttons in exclusive button groups are updated before this
signal is emitted. This means that slots can act on either the "off"
signal or the "on" signal emitted by the buttons in the group whose
states have changed.

For example, a slot that reacts to signals emitted by newly checked
buttons but which ignores signals from buttons that have been unchecked
can be implemented using the following pattern:

\snippet doc/src/snippets/code/src_gui_widgets_qabstractbutton.cpp 2

Button groups can be created using the QButtonGroup class, and
updates to the button states monitored with the
\l{QButtonGroup::buttonClicked()} signal.

\sa checked, clicked()
*/

/*!
    \property QAbstractButton::iconSize
    \brief the icon size used for this button.

    The default size is defined by the GUI style. This is a maximum
    size for the icons. Smaller icons will not be scaled up.
*/

QSize QAbstractButton::iconSize() const
{
    Q_D(const QAbstractButton);
    if (d->iconSize.isValid())
        return d->iconSize;
    int e = style()->pixelMetric(QStyle::PM_ButtonIconSize, 0, this);
    return QSize(e, e);
}

void QAbstractButton::setIconSize(const QSize &size)
{
    Q_D(QAbstractButton);
    if (d->iconSize == size)
        return;

    d->iconSize = size;
    d->sizeHint = QSize();
    updateGeometry();
    if (isVisible()) {
        update();
    }
}


#ifdef QT3_SUPPORT
/*!
    Use icon() instead.
*/
QIcon *QAbstractButton::iconSet() const
{
    Q_D(const QAbstractButton);
    if (!d->icon.isNull())
        return const_cast<QIcon *>(&d->icon);
    return 0;
}

/*!
    Use QAbstractButton(QWidget *) instead.

    Call setObjectName() if you want to specify an object name, and
    setParent() if you want to set the window flags.
*/
QAbstractButton::QAbstractButton(QWidget *parent, const char *name, Qt::WindowFlags f)
    : QWidget(*new QAbstractButtonPrivate, parent, f)
{
    Q_D(QAbstractButton);
    setObjectName(QString::fromAscii(name));
    d->init();
}

/*! \fn bool QAbstractButton::isOn() const

    Use isChecked() instead.
*/

/*!
    \fn QPixmap *QAbstractButton::pixmap() const

    This compatibility function always returns 0.

    Use icon() instead.
*/

/*! \fn void QAbstractButton::setPixmap(const QPixmap &p)

    Use setIcon() instead.
*/

/*! \fn void QAbstractButton::setIconSet(const QIcon &icon)

    Use setIcon() instead.
*/

/*! \fn void QAbstractButton::setOn(bool b)

    Use setChecked() instead.
*/

/*! \fn bool QAbstractButton::isToggleButton() const

    Use isCheckable() instead.
*/

/*!
    \fn void QAbstractButton::setToggleButton(bool b)

    Use setCheckable() instead.
*/

/*! \fn void QAbstractButton::setAccel(const QKeySequence &key)

    Use setShortcut() instead.
*/

/*! \fn QKeySequence QAbstractButton::accel() const

    Use shortcut() instead.
*/
#endif

QT_END_NAMESPACE
