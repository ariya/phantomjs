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

#include "qapplication.h"
#include "qbitmap.h"
#include "qdesktopwidget.h"
#include "qdialog.h"
#include <private/qdialog_p.h>
#include "qdrawutil.h"
#include "qevent.h"
#include "qfontmetrics.h"
#include "qmenu.h"
#include "qstylepainter.h"
#include "qpixmap.h"
#include "qpointer.h"
#include "qpushbutton.h"
#include "qstyle.h"
#include "qstyleoption.h"
#include "qtoolbar.h"
#include "qdebug.h"
#include "qlayoutitem.h"
#include "qdialogbuttonbox.h"
#ifdef Q_WS_MAC
#include "private/qmacstyle_mac_p.h"
#include "private/qmacstyle_mac_p_p.h"
#endif // Q_WS_MAC

#ifndef QT_NO_ACCESSIBILITY
#include "qaccessible.h"
#endif

#include "private/qmenu_p.h"
#include "private/qpushbutton_p.h"

QT_BEGIN_NAMESPACE


/*!
    \class QPushButton
    \brief The QPushButton widget provides a command button.

    \ingroup basicwidgets
    \inmodule QtWidgets

    The push button, or command button, is perhaps the most commonly
    used widget in any graphical user interface. Push (click) a button
    to command the computer to perform some action, or to answer a
    question. Typical buttons are OK, Apply, Cancel, Close, Yes, No
    and Help.

    A command button is rectangular and typically displays a text
    label describing its action. A shortcut key can be specified by
    preceding the preferred character with an ampersand in the
    text. For example:

    \snippet code/src_gui_widgets_qpushbutton.cpp 0

    In this example the shortcut is \e{Alt+D}. See the \l
    {QShortcut#mnemonic}{QShortcut} documentation for details (to
    display an actual ampersand, use '&&').

    Push buttons display a textual label, and optionally a small
    icon. These can be set using the constructors and changed later
    using setText() and setIcon().  If the button is disabled, the
    appearance of the text and icon will be manipulated with respect
    to the GUI style to make the button look "disabled".

    A push button emits the signal clicked() when it is activated by
    the mouse, the Spacebar or by a keyboard shortcut. Connect to
    this signal to perform the button's action. Push buttons also
    provide less commonly used signals, for example pressed() and
    released().

    Command buttons in dialogs are by default auto-default buttons,
    i.e., they become the default push button automatically when they
    receive the keyboard input focus. A default button is a push
    button that is activated when the user presses the Enter or Return
    key in a dialog. You can change this with setAutoDefault(). Note
    that auto-default buttons reserve a little extra space which is
    necessary to draw a default-button indicator. If you do not want
    this space around your buttons, call setAutoDefault(false).

    Being so central, the button widget has grown to accommodate a
    great many variations in the past decade. The Microsoft style
    guide now shows about ten different states of Windows push buttons
    and the text implies that there are dozens more when all the
    combinations of features are taken into consideration.

    The most important modes or states are:
    \list
    \li Available or not (grayed out, disabled).
    \li Standard push button, toggling push button or menu button.
    \li On or off (only for toggling push buttons).
    \li Default or normal. The default button in a dialog can generally
       be "clicked" using the Enter or Return key.
    \li Auto-repeat or not.
    \li Pressed down or not.
    \endlist

    As a general rule, use a push button when the application or
    dialog window performs an action when the user clicks on it (such
    as Apply, Cancel, Close and Help) \e and when the widget is
    supposed to have a wide, rectangular shape with a text label.
    Small, typically square buttons that change the state of the
    window rather than performing an action (such as the buttons in
    the top-right corner of the QFileDialog) are not command buttons,
    but tool buttons. Qt provides a special class (QToolButton) for
    these buttons.

    If you need toggle behavior (see setCheckable()) or a button
    that auto-repeats the activation signal when being pushed down
    like the arrows in a scroll bar (see setAutoRepeat()), a command
    button is probably not what you want. When in doubt, use a tool
    button.

    A variation of a command button is a menu button. These provide
    not just one command, but several, since when they are clicked
    they pop up a menu of options. Use the method setMenu() to
    associate a popup menu with a push button.

    Other classes of buttons are option buttons (see QRadioButton) and
    check boxes (see QCheckBox).

    \table 100%
    \row \li \inlineimage macintosh-pushbutton.png Screenshot of a Macintosh style push button
         \li A push button shown in the \l{Macintosh Style Widget Gallery}{Macintosh widget style}.

         Note that when a button's width becomes smaller than 50 or
         its height becomes smaller than 30, the button's corners are
         changed from round to square. Use the setMinimumSize()
         function to prevent this behavior.

    \row \li \inlineimage windowsvista-pushbutton.png Screenshot of a Windows Vista style push button
         \li A push button shown in the \l{Windows Vista Style Widget Gallery}{Windows Vista widget style}.
    \row \li \inlineimage fusion-pushbutton.png Screenshot of a Fusion style push button
         \li A push button shown in the \l{Fusion Style Widget Gallery}{Fusion widget style}.
    \endtable

    In Qt, the QAbstractButton base class provides most of the modes
    and other API, and QPushButton provides GUI logic.
    See QAbstractButton for more information about the API.

    \sa QToolButton, QRadioButton, QCheckBox, {fowler}{GUI Design Handbook: Push Button}
*/

/*!
    \property QPushButton::autoDefault
    \brief whether the push button is an auto default button

    If this property is set to true then the push button is an auto
    default button.

    In some GUI styles a default button is drawn with an extra frame
    around it, up to 3 pixels or more. Qt automatically keeps this
    space free around auto-default buttons, i.e., auto-default buttons
    may have a slightly larger size hint.

    This property's default is true for buttons that have a QDialog
    parent; otherwise it defaults to false.

    See the \l default property for details of how \l default and
    auto-default interact.
*/

/*!
    \property QPushButton::default
    \brief whether the push button is the default button

    Default and autodefault buttons decide what happens when the user
    presses enter in a dialog.

    A button with this property set to true (i.e., the dialog's
    \e default button,) will automatically be pressed when the user presses enter,
    with one exception: if an \a autoDefault button currently has focus, the autoDefault
    button is pressed. When the dialog has \l autoDefault buttons but no default button,
    pressing enter will press either the \l autoDefault button that currently has focus, or if no
    button has focus, the next \l autoDefault button in the focus chain.

    In a dialog, only one push button at a time can be the default
    button. This button is then displayed with an additional frame
    (depending on the GUI style).

    The default button behavior is provided only in dialogs. Buttons
    can always be clicked from the keyboard by pressing Spacebar when
    the button has focus.

    If the default property is set to false on the current default button
    while the dialog is visible, a new default will automatically be
    assigned the next time a push button in the dialog receives focus.

    This property's default is false.
*/

/*!
    \property QPushButton::flat
    \brief whether the button border is raised

    This property's default is false. If this property is set, most
    styles will not paint the button background unless the button is
    being pressed. setAutoFillBackground() can be used to ensure that
    the background is filled using the QPalette::Button brush.
*/

/*!
    Constructs a push button with no text and a \a parent.
*/

QPushButton::QPushButton(QWidget *parent)
    : QAbstractButton(*new QPushButtonPrivate, parent)
{
    Q_D(QPushButton);
    d->init();
}

/*!
    Constructs a push button with the parent \a parent and the text \a
    text.
*/

QPushButton::QPushButton(const QString &text, QWidget *parent)
    : QAbstractButton(*new QPushButtonPrivate, parent)
{
    Q_D(QPushButton);
    setText(text);
    d->init();
}


/*!
    Constructs a push button with an \a icon and a \a text, and a \a parent.

    Note that you can also pass a QPixmap object as an icon (thanks to
    the implicit type conversion provided by C++).

*/
QPushButton::QPushButton(const QIcon& icon, const QString &text, QWidget *parent)
    : QAbstractButton(*new QPushButtonPrivate, parent)
{
    Q_D(QPushButton);
    setText(text);
    setIcon(icon);
    d->init();
}

/*! \internal
 */
QPushButton::QPushButton(QPushButtonPrivate &dd, QWidget *parent)
    : QAbstractButton(dd, parent)
{
    Q_D(QPushButton);
    d->init();
}

/*!
    Destroys the push button.
*/
QPushButton::~QPushButton()
{
}

QDialog *QPushButtonPrivate::dialogParent() const
{
    Q_Q(const QPushButton);
    const QWidget *p = q;
    while (p && !p->isWindow()) {
        p = p->parentWidget();
        if (const QDialog *dialog = qobject_cast<const QDialog *>(p))
            return const_cast<QDialog *>(dialog);
    }
    return 0;
}

/*!
    Initialize \a option with the values from this QPushButton. This method is useful
    for subclasses when they need a QStyleOptionButton, but don't want to fill
    in all the information themselves.

    \sa QStyleOption::initFrom()
*/
void QPushButton::initStyleOption(QStyleOptionButton *option) const
{
    if (!option)
        return;

    Q_D(const QPushButton);
    option->initFrom(this);
    option->features = QStyleOptionButton::None;
    if (d->flat)
        option->features |= QStyleOptionButton::Flat;
#ifndef QT_NO_MENU
    if (d->menu)
        option->features |= QStyleOptionButton::HasMenu;
#endif
    if (autoDefault())
        option->features |= QStyleOptionButton::AutoDefaultButton;
    if (d->defaultButton)
        option->features |= QStyleOptionButton::DefaultButton;
    if (d->down || d->menuOpen)
        option->state |= QStyle::State_Sunken;
    if (d->checked)
        option->state |= QStyle::State_On;
    if (!d->flat && !d->down)
        option->state |= QStyle::State_Raised;
    option->text = d->text;
    option->icon = d->icon;
    option->iconSize = iconSize();
}

void QPushButton::setAutoDefault(bool enable)
{
    Q_D(QPushButton);
    uint state = enable ? QPushButtonPrivate::On : QPushButtonPrivate::Off;
    if (d->autoDefault != QPushButtonPrivate::Auto && d->autoDefault == state)
        return;
    d->autoDefault = state;
    d->sizeHint = QSize();
    update();
    updateGeometry();
}

bool QPushButton::autoDefault() const
{
    Q_D(const QPushButton);
    if(d->autoDefault == QPushButtonPrivate::Auto)
        return ( d->dialogParent() != 0 );
    return d->autoDefault;
}

void QPushButton::setDefault(bool enable)
{
    Q_D(QPushButton);
    if (d->defaultButton == enable)
        return;
    d->defaultButton = enable;
    if (d->defaultButton) {
        if (QDialog *dlg = d->dialogParent())
            dlg->d_func()->setMainDefault(this);
    }
    update();
#ifndef QT_NO_ACCESSIBILITY
    QAccessible::State s;
    s.defaultButton = true;
    QAccessibleStateChangeEvent event(this, s);
    QAccessible::updateAccessibility(&event);
#endif
}

bool QPushButton::isDefault() const
{
    Q_D(const QPushButton);
    return d->defaultButton;
}

/*!
    \reimp
*/
QSize QPushButton::sizeHint() const
{
    Q_D(const QPushButton);
    if (d->sizeHint.isValid() && d->lastAutoDefault == autoDefault())
        return d->sizeHint;
    d->lastAutoDefault = autoDefault();
    ensurePolished();

    int w = 0, h = 0;

    QStyleOptionButton opt;
    initStyleOption(&opt);

    // calculate contents size...
#ifndef QT_NO_ICON

    bool showButtonBoxIcons = qobject_cast<QDialogButtonBox*>(parentWidget())
                          && style()->styleHint(QStyle::SH_DialogButtonBox_ButtonsHaveIcons);

    if (!icon().isNull() || showButtonBoxIcons) {
        int ih = opt.iconSize.height();
        int iw = opt.iconSize.width() + 4;
        w += iw;
        h = qMax(h, ih);
    }
#endif
    QString s(text());
    bool empty = s.isEmpty();
    if (empty)
        s = QString::fromLatin1("XXXX");
    QFontMetrics fm = fontMetrics();
    QSize sz = fm.size(Qt::TextShowMnemonic, s);
    if(!empty || !w)
        w += sz.width();
    if(!empty || !h)
        h = qMax(h, sz.height());
    opt.rect.setSize(QSize(w, h)); // PM_MenuButtonIndicator depends on the height
#ifndef QT_NO_MENU
    if (menu())
        w += style()->pixelMetric(QStyle::PM_MenuButtonIndicator, &opt, this);
#endif
    d->sizeHint = (style()->sizeFromContents(QStyle::CT_PushButton, &opt, QSize(w, h), this).
                  expandedTo(QApplication::globalStrut()));
    return d->sizeHint;
}

/*!
    \reimp
 */
QSize QPushButton::minimumSizeHint() const
{
    return sizeHint();
}


/*!\reimp
*/
void QPushButton::paintEvent(QPaintEvent *)
{
    QStylePainter p(this);
    QStyleOptionButton option;
    initStyleOption(&option);
    p.drawControl(QStyle::CE_PushButton, option);
}


/*! \reimp */
void QPushButton::keyPressEvent(QKeyEvent *e)
{
    Q_D(QPushButton);
    switch (e->key()) {
    case Qt::Key_Enter:
    case Qt::Key_Return:
        if (autoDefault() || d->defaultButton) {
            click();
            break;
        }
        // fall through
    default:
        QAbstractButton::keyPressEvent(e);
    }
}

/*!
    \reimp
*/
void QPushButton::focusInEvent(QFocusEvent *e)
{
    Q_D(QPushButton);
    if (e->reason() != Qt::PopupFocusReason && autoDefault() && !d->defaultButton) {
        d->defaultButton = true;
        QDialog *dlg = qobject_cast<QDialog*>(window());
        if (dlg)
            dlg->d_func()->setDefault(this);
    }
    QAbstractButton::focusInEvent(e);
}

/*!
    \reimp
*/
void QPushButton::focusOutEvent(QFocusEvent *e)
{
    Q_D(QPushButton);
    if (e->reason() != Qt::PopupFocusReason && autoDefault() && d->defaultButton) {
        QDialog *dlg = qobject_cast<QDialog*>(window());
        if (dlg)
            dlg->d_func()->setDefault(0);
        else
            d->defaultButton = false;
    }

    QAbstractButton::focusOutEvent(e);
#ifndef QT_NO_MENU
    if (d->menu && d->menu->isVisible())        // restore pressed status
        setDown(true);
#endif
}

#ifndef QT_NO_MENU
/*!
    Associates the popup menu \a menu with this push button. This
    turns the button into a menu button, which in some styles will
    produce a small triangle to the right of the button's text.

    Ownership of the menu is \e not transferred to the push button.

    \image fusion-pushbutton-menu.png Screenshot of a Fusion style push button with popup menu.
    A push button with popup menus shown in the \l{Fusion Style Widget Gallery}{Fusion widget style}.

    \sa menu()
*/
void QPushButton::setMenu(QMenu* menu)
{
    Q_D(QPushButton);
    if (menu == d->menu)
        return;

    if (menu && !d->menu) {
        connect(this, SIGNAL(pressed()), this, SLOT(_q_popupPressed()), Qt::UniqueConnection);
    }
    if (d->menu)
        removeAction(d->menu->menuAction());
    d->menu = menu;
    if (d->menu)
        addAction(d->menu->menuAction());

    d->resetLayoutItemMargins();
    d->sizeHint = QSize();
    update();
    updateGeometry();
}

/*!
    Returns the button's associated popup menu or 0 if no popup menu
    has been set.

    \sa setMenu()
*/
QMenu* QPushButton::menu() const
{
    Q_D(const QPushButton);
    return d->menu;
}

/*!
    Shows (pops up) the associated popup menu. If there is no such
    menu, this function does nothing. This function does not return
    until the popup menu has been closed by the user.
*/
void QPushButton::showMenu()
{
    Q_D(QPushButton);
    if (!d || !d->menu)
        return;
    setDown(true);
    d->_q_popupPressed();
}

void QPushButtonPrivate::_q_popupPressed()
{
    Q_Q(QPushButton);
    if (!down || !menu)
        return;

    menu->setNoReplayFor(q);

    QPoint menuPos = adjustedMenuPosition();

    QPointer<QPushButton> guard(q);
    QMenuPrivate::get(menu)->causedPopup.widget = guard;

    //Because of a delay in menu effects, we must keep track of the
    //menu visibility to avoid flicker on button release
    menuOpen = true;
    menu->exec(menuPos);
    if (guard) {
        menuOpen = false;
        q->setDown(false);
    }
}

QPoint QPushButtonPrivate::adjustedMenuPosition()
{
    Q_Q(QPushButton);

    bool horizontal = true;
#if !defined(QT_NO_TOOLBAR)
    QToolBar *tb = qobject_cast<QToolBar*>(parent);
    if (tb && tb->orientation() == Qt::Vertical)
        horizontal = false;
#endif

    QWidgetItem item(q);
    QRect rect = item.geometry();
    rect.setRect(rect.x() - q->x(), rect.y() - q->y(), rect.width(), rect.height());

    QSize menuSize = menu->sizeHint();
    QPoint globalPos = q->mapToGlobal(rect.topLeft());
    int x = globalPos.x();
    int y = globalPos.y();
    if (horizontal) {
        if (globalPos.y() + rect.height() + menuSize.height() <= QApplication::desktop()->availableGeometry(q).height()) {
            y += rect.height();
        } else {
            y -= menuSize.height();
        }
        if (q->layoutDirection() == Qt::RightToLeft)
            x += rect.width() - menuSize.width();
    } else {
        if (globalPos.x() + rect.width() + menu->sizeHint().width() <= QApplication::desktop()->availableGeometry(q).width())
            x += rect.width();
        else
            x -= menuSize.width();
    }

    return QPoint(x,y);
}

#endif // QT_NO_MENU

void QPushButtonPrivate::resetLayoutItemMargins()
{
    Q_Q(QPushButton);
    QStyleOptionButton opt;
    q->initStyleOption(&opt);
    setLayoutItemMargins(QStyle::SE_PushButtonLayoutItem, &opt);
}

void QPushButton::setFlat(bool flat)
{
    Q_D(QPushButton);
    if (d->flat == flat)
        return;
    d->flat = flat;
    d->resetLayoutItemMargins();
    d->sizeHint = QSize();
    update();
    updateGeometry();
}

bool QPushButton::isFlat() const
{
    Q_D(const QPushButton);
    return d->flat;
}

/*! \reimp */
bool QPushButton::event(QEvent *e)
{
    Q_D(QPushButton);
    if (e->type() == QEvent::ParentChange) {
        if (QDialog *dialog = d->dialogParent()) {
            if (d->defaultButton)
                dialog->d_func()->setMainDefault(this);
        }
    } else if (e->type() == QEvent::StyleChange
#ifdef Q_OS_MAC
               || e->type() == QEvent::MacSizeChange
#endif
               ) {
        d->resetLayoutItemMargins();
        updateGeometry();
    } else if (e->type() == QEvent::PolishRequest) {
        updateGeometry();
    }
    return QAbstractButton::event(e);
}

#ifdef Q_WS_MAC
/*! \reimp */
bool QPushButton::hitButton(const QPoint &pos) const
{
    QStyleOptionButton opt;
    initStyleOption(&opt);
    if (qt_mac_buttonIsRenderedFlat(this, &opt))
        return QAbstractButton::hitButton(pos);

    // Now that we know we are using the native style, let's proceed.
    Q_D(const QPushButton);
    QPushButtonPrivate *nonConst = const_cast<QPushButtonPrivate *>(d);
    // In OSX buttons are round, which causes the hit method to be special.
    // We cannot simply relay on detecting if something is inside the rect or not,
    // we need to check if it is inside the "rounded area" or not. A point might
    // be inside the rect but not inside the rounded area.
    // Notice this method is only reimplemented for OSX.
    return nonConst->hitButton(pos);
}

bool QPushButtonPrivate::hitButton(const QPoint &pos)
{
    Q_Q(QPushButton);
    QRect roundedRect(q->rect().left() + QMacStylePrivate::PushButtonLeftOffset,
                      q->rect().top() + QMacStylePrivate::PushButtonContentPadding,
                      q->rect().width() - QMacStylePrivate::PushButtonRightOffset,
                      q->rect().height() - QMacStylePrivate::PushButtonBottomOffset);
    return roundedRect.contains(pos);
}
#endif // Q_WS_MAC


QT_END_NAMESPACE

#include "moc_qpushbutton.cpp"
