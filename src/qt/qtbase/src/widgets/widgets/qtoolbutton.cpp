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

#include "qtoolbutton.h"
#ifndef QT_NO_TOOLBUTTON

#include <qapplication.h>
#include <qdesktopwidget.h>
#include <qdrawutil.h>
#include <qevent.h>
#include <qicon.h>
#include <qmenu.h>
#include <qpainter.h>
#include <qpointer.h>
#include <qstyle.h>
#include <qstyleoption.h>
#include <qtooltip.h>
#include <qmainwindow.h>
#include <qtoolbar.h>
#include <qvariant.h>
#include <qstylepainter.h>
#include <private/qabstractbutton_p.h>
#include <private/qaction_p.h>
#include <private/qmenu_p.h>

QT_BEGIN_NAMESPACE

class QToolButtonPrivate : public QAbstractButtonPrivate
{
    Q_DECLARE_PUBLIC(QToolButton)
public:
    void init();
#ifndef QT_NO_MENU
    void _q_buttonPressed();
    void popupTimerDone();
    void _q_updateButtonDown();
    void _q_menuTriggered(QAction *);
#endif
    bool updateHoverControl(const QPoint &pos);
    void _q_actionTriggered();
    QStyle::SubControl newHoverControl(const QPoint &pos);
    QStyle::SubControl hoverControl;
    QRect hoverRect;
    QPointer<QAction> menuAction; //the menu set by the user (setMenu)
    QBasicTimer popupTimer;
    int delay;
    Qt::ArrowType arrowType;
    Qt::ToolButtonStyle toolButtonStyle;
    QToolButton::ToolButtonPopupMode popupMode;
    enum { NoButtonPressed=0, MenuButtonPressed=1, ToolButtonPressed=2 };
    uint buttonPressed : 2;
    uint menuButtonDown          : 1;
    uint autoRaise             : 1;
    uint repeat                : 1;
    QAction *defaultAction;
#ifndef QT_NO_MENU
    bool hasMenu() const;
    //workaround for task 177850
    QList<QAction *> actionsCopy;
#endif
};

#ifndef QT_NO_MENU
bool QToolButtonPrivate::hasMenu() const
{
    return ((defaultAction && defaultAction->menu())
            || (menuAction && menuAction->menu())
            || actions.size() > (defaultAction ? 1 : 0));
}
#endif

/*!
    \class QToolButton
    \brief The QToolButton class provides a quick-access button to
    commands or options, usually used inside a QToolBar.

    \ingroup basicwidgets
    \inmodule QtWidgets

    A tool button is a special button that provides quick-access to
    specific commands or options. As opposed to a normal command
    button, a tool button usually doesn't show a text label, but shows
    an icon instead.

    Tool buttons are normally created when new QAction instances are
    created with QToolBar::addAction() or existing actions are added
    to a toolbar with QToolBar::addAction(). It is also possible to
    construct tool buttons in the same way as any other widget, and
    arrange them alongside other widgets in layouts.

    One classic use of a tool button is to select tools; for example,
    the "pen" tool in a drawing program. This would be implemented
    by using a QToolButton as a toggle button (see setToggleButton()).

    QToolButton supports auto-raising. In auto-raise mode, the button
    draws a 3D frame only when the mouse points at it. The feature is
    automatically turned on when a button is used inside a QToolBar.
    Change it with setAutoRaise().

    A tool button's icon is set as QIcon. This makes it possible to
    specify different pixmaps for the disabled and active state. The
    disabled pixmap is used when the button's functionality is not
    available. The active pixmap is displayed when the button is
    auto-raised because the mouse pointer is hovering over it.

    The button's look and dimension is adjustable with
    setToolButtonStyle() and setIconSize(). When used inside a
    QToolBar in a QMainWindow, the button automatically adjusts to
    QMainWindow's settings (see QMainWindow::setToolButtonStyle() and
    QMainWindow::setIconSize()). Instead of an icon, a tool button can
    also display an arrow symbol, specified with
    \l{QToolButton::arrowType} {arrowType}.

    A tool button can offer additional choices in a popup menu. The
    popup menu can be set using setMenu(). Use setPopupMode() to
    configure the different modes available for tool buttons with a
    menu set. The default mode is DelayedPopupMode which is sometimes
    used with the "Back" button in a web browser.  After pressing and
    holding the button down for a while, a menu pops up showing a list
    of possible pages to jump to. The default delay is 600 ms; you can
    adjust it with setPopupDelay().

    \table 100%
    \row \li \inlineimage assistant-toolbar.png Qt Assistant's toolbar with tool buttons
    \row \li Qt Assistant's toolbar contains tool buttons that are associated
         with actions used in other parts of the main window.
    \endtable

    \sa QPushButton, QToolBar, QMainWindow, QAction,
        {fowler}{GUI Design Handbook: Push Button}
*/

/*!
    \fn void QToolButton::triggered(QAction *action)

    This signal is emitted when the given \a action is triggered.

    The action may also be associated with other parts of the user interface,
    such as menu items and keyboard shortcuts. Sharing actions in this
    way helps make the user interface more consistent and is often less work
    to implement.
*/

/*!
    Constructs an empty tool button with parent \a
    parent.
*/
QToolButton::QToolButton(QWidget * parent)
    : QAbstractButton(*new QToolButtonPrivate, parent)
{
    Q_D(QToolButton);
    d->init();
}



/*  Set-up code common to all the constructors */

void QToolButtonPrivate::init()
{
    Q_Q(QToolButton);
    delay = q->style()->styleHint(QStyle::SH_ToolButton_PopupDelay, 0, q);
    defaultAction = 0;
#ifndef QT_NO_TOOLBAR
    if (qobject_cast<QToolBar*>(parent))
        autoRaise = true;
    else
#endif
        autoRaise = false;
    arrowType = Qt::NoArrow;
    menuButtonDown = false;
    popupMode = QToolButton::DelayedPopup;
    buttonPressed = QToolButtonPrivate::NoButtonPressed;

    toolButtonStyle = Qt::ToolButtonIconOnly;
    hoverControl = QStyle::SC_None;

    q->setFocusPolicy(Qt::TabFocus);
    q->setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed,
                                 QSizePolicy::ToolButton));

#ifndef QT_NO_MENU
    QObject::connect(q, SIGNAL(pressed()), q, SLOT(_q_buttonPressed()));
#endif

    setLayoutItemMargins(QStyle::SE_ToolButtonLayoutItem);

}

/*!
    Initialize \a option with the values from this QToolButton. This method
    is useful for subclasses when they need a QStyleOptionToolButton, but don't want
    to fill in all the information themselves.

    \sa QStyleOption::initFrom()
*/
void QToolButton::initStyleOption(QStyleOptionToolButton *option) const
{
    if (!option)
        return;

    Q_D(const QToolButton);
    option->initFrom(this);
    bool forceNoText = false;
    option->iconSize = iconSize(); //default value

#ifndef QT_NO_TOOLBAR
    if (parentWidget()) {
        if (QToolBar *toolBar = qobject_cast<QToolBar *>(parentWidget())) {
            option->iconSize = toolBar->iconSize();
        }
    }
#endif // QT_NO_TOOLBAR

    if (!forceNoText)
        option->text = d->text;
    option->icon = d->icon;
    option->arrowType = d->arrowType;
    if (d->down)
        option->state |= QStyle::State_Sunken;
    if (d->checked)
        option->state |= QStyle::State_On;
    if (d->autoRaise)
        option->state |= QStyle::State_AutoRaise;
    if (!d->checked && !d->down)
        option->state |= QStyle::State_Raised;

    option->subControls = QStyle::SC_ToolButton;
    option->activeSubControls = QStyle::SC_None;

    option->features = QStyleOptionToolButton::None;
    if (d->popupMode == QToolButton::MenuButtonPopup) {
        option->subControls |= QStyle::SC_ToolButtonMenu;
        option->features |= QStyleOptionToolButton::MenuButtonPopup;
    }
    if (option->state & QStyle::State_MouseOver) {
        option->activeSubControls = d->hoverControl;
    }
    if (d->menuButtonDown) {
        option->state |= QStyle::State_Sunken;
        option->activeSubControls |= QStyle::SC_ToolButtonMenu;
    }
    if (d->down) {
        option->state |= QStyle::State_Sunken;
        option->activeSubControls |= QStyle::SC_ToolButton;
    }


    if (d->arrowType != Qt::NoArrow)
        option->features |= QStyleOptionToolButton::Arrow;
    if (d->popupMode == QToolButton::DelayedPopup)
        option->features |= QStyleOptionToolButton::PopupDelay;
#ifndef QT_NO_MENU
    if (d->hasMenu())
        option->features |= QStyleOptionToolButton::HasMenu;
#endif
    if (d->toolButtonStyle == Qt::ToolButtonFollowStyle) {
        option->toolButtonStyle = Qt::ToolButtonStyle(style()->styleHint(QStyle::SH_ToolButtonStyle, option, this));
    } else
        option->toolButtonStyle = d->toolButtonStyle;

    if (option->toolButtonStyle == Qt::ToolButtonTextBesideIcon) {
        // If the action is not prioritized, remove the text label to save space
        if (d->defaultAction && d->defaultAction->priority() < QAction::NormalPriority)
            option->toolButtonStyle = Qt::ToolButtonIconOnly;
    }

    if (d->icon.isNull() && d->arrowType == Qt::NoArrow && !forceNoText) {
        if (!d->text.isEmpty())
            option->toolButtonStyle = Qt::ToolButtonTextOnly;
        else if (option->toolButtonStyle != Qt::ToolButtonTextOnly)
            option->toolButtonStyle = Qt::ToolButtonIconOnly;
    }

    option->pos = pos();
    option->font = font();
}

/*!
    Destroys the object and frees any allocated resources.
*/

QToolButton::~QToolButton()
{
}

/*!
    \reimp
*/
QSize QToolButton::sizeHint() const
{
    Q_D(const QToolButton);
    if (d->sizeHint.isValid())
        return d->sizeHint;
    ensurePolished();

    int w = 0, h = 0;
    QStyleOptionToolButton opt;
    initStyleOption(&opt);

    QFontMetrics fm = fontMetrics();
    if (opt.toolButtonStyle != Qt::ToolButtonTextOnly) {
        QSize icon = opt.iconSize;
        w = icon.width();
        h = icon.height();
    }

    if (opt.toolButtonStyle != Qt::ToolButtonIconOnly) {
        QSize textSize = fm.size(Qt::TextShowMnemonic, text());
        textSize.setWidth(textSize.width() + fm.width(QLatin1Char(' '))*2);
        if (opt.toolButtonStyle == Qt::ToolButtonTextUnderIcon) {
            h += 4 + textSize.height();
            if (textSize.width() > w)
                w = textSize.width();
        } else if (opt.toolButtonStyle == Qt::ToolButtonTextBesideIcon) {
            w += 4 + textSize.width();
            if (textSize.height() > h)
                h = textSize.height();
        } else { // TextOnly
            w = textSize.width();
            h = textSize.height();
        }
    }

    opt.rect.setSize(QSize(w, h)); // PM_MenuButtonIndicator depends on the height
    if (d->popupMode == MenuButtonPopup)
        w += style()->pixelMetric(QStyle::PM_MenuButtonIndicator, &opt, this);

    d->sizeHint = style()->sizeFromContents(QStyle::CT_ToolButton, &opt, QSize(w, h), this).
                  expandedTo(QApplication::globalStrut());
    return d->sizeHint;
}

/*!
    \reimp
 */
QSize QToolButton::minimumSizeHint() const
{
    return sizeHint();
}

/*!
    \property QToolButton::toolButtonStyle
    \brief whether the tool button displays an icon only, text only,
    or text beside/below the icon.

    The default is Qt::ToolButtonIconOnly.

    To have the style of toolbuttons follow the system settings (as available
    in GNOME and KDE desktop environments), set this property to Qt::ToolButtonFollowStyle.

    QToolButton automatically connects this slot to the relevant
    signal in the QMainWindow in which is resides.
*/

/*!
    \property QToolButton::arrowType
    \brief whether the button displays an arrow instead of a normal icon

    This displays an arrow as the icon for the QToolButton.

    By default, this property is set to Qt::NoArrow.
*/

Qt::ToolButtonStyle QToolButton::toolButtonStyle() const
{
    Q_D(const QToolButton);
    return d->toolButtonStyle;
}

Qt::ArrowType QToolButton::arrowType() const
{
    Q_D(const QToolButton);
    return d->arrowType;
}


void QToolButton::setToolButtonStyle(Qt::ToolButtonStyle style)
{
    Q_D(QToolButton);
    if (d->toolButtonStyle == style)
        return;

    d->toolButtonStyle = style;
    d->sizeHint = QSize();
    updateGeometry();
    if (isVisible()) {
        update();
    }
}

void QToolButton::setArrowType(Qt::ArrowType type)
{
    Q_D(QToolButton);
    if (d->arrowType == type)
        return;

    d->arrowType = type;
    d->sizeHint = QSize();
    updateGeometry();
    if (isVisible()) {
        update();
    }
}

/*!
    \fn void QToolButton::paintEvent(QPaintEvent *event)

    Paints the button in response to the paint \a event.
*/
void QToolButton::paintEvent(QPaintEvent *)
{
    QStylePainter p(this);
    QStyleOptionToolButton opt;
    initStyleOption(&opt);
    p.drawComplexControl(QStyle::CC_ToolButton, opt);
}

/*!
    \reimp
 */
void QToolButton::actionEvent(QActionEvent *event)
{
    Q_D(QToolButton);
    QAction *action = event->action();
    switch (event->type()) {
    case QEvent::ActionChanged:
        if (action == d->defaultAction)
            setDefaultAction(action); // update button state
        break;
    case QEvent::ActionAdded:
        connect(action, SIGNAL(triggered()), this, SLOT(_q_actionTriggered()));
        break;
    case QEvent::ActionRemoved:
        if (d->defaultAction == action)
            d->defaultAction = 0;
#ifndef QT_NO_MENU
        if (action == d->menuAction)
            d->menuAction = 0;
#endif
        action->disconnect(this);
        break;
    default:
        ;
    }
    QAbstractButton::actionEvent(event);
}

QStyle::SubControl QToolButtonPrivate::newHoverControl(const QPoint &pos)
{
    Q_Q(QToolButton);
    QStyleOptionToolButton opt;
    q->initStyleOption(&opt);
    opt.subControls = QStyle::SC_All;
    hoverControl = q->style()->hitTestComplexControl(QStyle::CC_ToolButton, &opt, pos, q);
    if (hoverControl == QStyle::SC_None)
        hoverRect = QRect();
    else
        hoverRect = q->style()->subControlRect(QStyle::CC_ToolButton, &opt, hoverControl, q);
    return hoverControl;
}

bool QToolButtonPrivate::updateHoverControl(const QPoint &pos)
{
    Q_Q(QToolButton);
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

void QToolButtonPrivate::_q_actionTriggered()
{
    Q_Q(QToolButton);
    if (QAction *action = qobject_cast<QAction *>(q->sender()))
        emit q->triggered(action);
}

/*!
    \reimp
 */
void QToolButton::enterEvent(QEvent * e)
{
    Q_D(QToolButton);
    if (d->autoRaise)
        update();
    if (d->defaultAction)
        d->defaultAction->hover();
    QAbstractButton::enterEvent(e);
}


/*!
    \reimp
 */
void QToolButton::leaveEvent(QEvent * e)
{
    Q_D(QToolButton);
    if (d->autoRaise)
        update();

    QAbstractButton::leaveEvent(e);
}


/*!
    \reimp
 */
void QToolButton::timerEvent(QTimerEvent *e)
{
#ifndef QT_NO_MENU
    Q_D(QToolButton);
    if (e->timerId() == d->popupTimer.timerId()) {
        d->popupTimerDone();
        return;
    }
#endif
    QAbstractButton::timerEvent(e);
}


/*!
    \reimp
*/
void QToolButton::changeEvent(QEvent *e)
{
#ifndef QT_NO_TOOLBAR
    Q_D(QToolButton);
    if (e->type() == QEvent::ParentChange) {
        if (qobject_cast<QToolBar*>(parentWidget()))
            d->autoRaise = true;
    } else if (e->type() == QEvent::StyleChange
#ifdef Q_OS_MAC
               || e->type() == QEvent::MacSizeChange
#endif
               ) {
        d->delay = style()->styleHint(QStyle::SH_ToolButton_PopupDelay, 0, this);
        d->setLayoutItemMargins(QStyle::SE_ToolButtonLayoutItem);
    }
#endif
    QAbstractButton::changeEvent(e);
}

/*!
    \reimp
*/
void QToolButton::mousePressEvent(QMouseEvent *e)
{
    Q_D(QToolButton);
#ifndef QT_NO_MENU
    QStyleOptionToolButton opt;
    initStyleOption(&opt);
    if (e->button() == Qt::LeftButton && (d->popupMode == MenuButtonPopup)) {
        QRect popupr = style()->subControlRect(QStyle::CC_ToolButton, &opt,
                                               QStyle::SC_ToolButtonMenu, this);
        if (popupr.isValid() && popupr.contains(e->pos())) {
            d->buttonPressed = QToolButtonPrivate::MenuButtonPressed;
            showMenu();
            return;
        }
    }
#endif
    d->buttonPressed = QToolButtonPrivate::ToolButtonPressed;
    QAbstractButton::mousePressEvent(e);
}

/*!
    \reimp
*/
void QToolButton::mouseReleaseEvent(QMouseEvent *e)
{
    Q_D(QToolButton);
    QAbstractButton::mouseReleaseEvent(e);
    d->buttonPressed = QToolButtonPrivate::NoButtonPressed;
}

/*!
    \reimp
*/
bool QToolButton::hitButton(const QPoint &pos) const
{
    Q_D(const QToolButton);
    if(QAbstractButton::hitButton(pos))
        return (d->buttonPressed != QToolButtonPrivate::MenuButtonPressed);
    return false;
}


#ifndef QT_NO_MENU
/*!
    Associates the given \a menu with this tool button.

    The menu will be shown according to the button's \l popupMode.

    Ownership of the menu is not transferred to the tool button.

    \sa menu()
*/
void QToolButton::setMenu(QMenu* menu)
{
    Q_D(QToolButton);

    if (d->menuAction)
        removeAction(d->menuAction);

    if (menu) {
        d->menuAction = menu->menuAction();
        addAction(d->menuAction);
    } else {
        d->menuAction = 0;
    }
    update();
}

/*!
    Returns the associated menu, or 0 if no menu has been defined.

    \sa setMenu()
*/
QMenu* QToolButton::menu() const
{
    Q_D(const QToolButton);
    if (d->menuAction)
        return d->menuAction->menu();
    return 0;
}

/*!
    Shows (pops up) the associated popup menu. If there is no such
    menu, this function does nothing. This function does not return
    until the popup menu has been closed by the user.
*/
void QToolButton::showMenu()
{
    Q_D(QToolButton);
    if (!d->hasMenu()) {
        d->menuButtonDown = false;
        return; // no menu to show
    }
    // prevent recursions spinning another event loop
    if (d->menuButtonDown)
        return;


    d->menuButtonDown = true;
    repaint();
    d->popupTimer.stop();
    d->popupTimerDone();
}

void QToolButtonPrivate::_q_buttonPressed()
{
    Q_Q(QToolButton);
    if (!hasMenu())
        return; // no menu to show
    if (popupMode == QToolButton::MenuButtonPopup)
        return;
    else if (delay > 0 && !popupTimer.isActive() && popupMode == QToolButton::DelayedPopup)
        popupTimer.start(delay, q);
    else if (delay == 0 || popupMode == QToolButton::InstantPopup)
        q->showMenu();
}

void QToolButtonPrivate::popupTimerDone()
{
    Q_Q(QToolButton);
    popupTimer.stop();
    if (!menuButtonDown && !down)
        return;

    menuButtonDown = true;
    QPointer<QMenu> actualMenu;
    bool mustDeleteActualMenu = false;
    if(menuAction) {
        actualMenu = menuAction->menu();
    } else if (defaultAction && defaultAction->menu()) {
        actualMenu = defaultAction->menu();
    } else {
        actualMenu = new QMenu(q);
        mustDeleteActualMenu = true;
        for(int i = 0; i < actions.size(); i++)
            actualMenu->addAction(actions.at(i));
    }
    repeat = q->autoRepeat();
    q->setAutoRepeat(false);
    bool horizontal = true;
#if !defined(QT_NO_TOOLBAR)
    QToolBar *tb = qobject_cast<QToolBar*>(parent);
    if (tb && tb->orientation() == Qt::Vertical)
        horizontal = false;
#endif
    QPoint p;
    QRect screen = QApplication::desktop()->availableGeometry(q);
    QSize sh = ((QToolButton*)(QMenu*)actualMenu)->receivers(SIGNAL(aboutToShow()))? QSize() : actualMenu->sizeHint();
    QRect rect = q->rect();
    if (horizontal) {
        if (q->isRightToLeft()) {
            if (q->mapToGlobal(QPoint(0, rect.bottom())).y() + sh.height() <= screen.height()) {
                p = q->mapToGlobal(rect.bottomRight());
            } else {
                p = q->mapToGlobal(rect.topRight() - QPoint(0, sh.height()));
            }
            p.rx() -= sh.width();
        } else {
            if (q->mapToGlobal(QPoint(0, rect.bottom())).y() + sh.height() <= screen.height()) {
                p = q->mapToGlobal(rect.bottomLeft());
            } else {
                p = q->mapToGlobal(rect.topLeft() - QPoint(0, sh.height()));
            }
        }
    } else {
        if (q->isRightToLeft()) {
            if (q->mapToGlobal(QPoint(rect.left(), 0)).x() - sh.width() <= screen.x()) {
                p = q->mapToGlobal(rect.topRight());
            } else {
                p = q->mapToGlobal(rect.topLeft());
                p.rx() -= sh.width();
            }
        } else {
            if (q->mapToGlobal(QPoint(rect.right(), 0)).x() + sh.width() <= screen.right()) {
                p = q->mapToGlobal(rect.topRight());
            } else {
                p = q->mapToGlobal(rect.topLeft() - QPoint(sh.width(), 0));
            }
        }
    }
    p.rx() = qMax(screen.left(), qMin(p.x(), screen.right() - sh.width()));
    p.ry() += 1;
    QPointer<QToolButton> that = q;
    actualMenu->setNoReplayFor(q);
    if (!mustDeleteActualMenu) //only if action are not in this widget
        QObject::connect(actualMenu, SIGNAL(triggered(QAction*)), q, SLOT(_q_menuTriggered(QAction*)));
    QObject::connect(actualMenu, SIGNAL(aboutToHide()), q, SLOT(_q_updateButtonDown()));
    actualMenu->d_func()->causedPopup.widget = q;
    actualMenu->d_func()->causedPopup.action = defaultAction;
    actionsCopy = q->actions(); //(the list of action may be modified in slots)
    actualMenu->exec(p);

    if (!that)
        return;

    QObject::disconnect(actualMenu, SIGNAL(aboutToHide()), q, SLOT(_q_updateButtonDown()));
    if (mustDeleteActualMenu)
        delete actualMenu;
    else
        QObject::disconnect(actualMenu, SIGNAL(triggered(QAction*)), q, SLOT(_q_menuTriggered(QAction*)));

    actionsCopy.clear();

    if (repeat)
        q->setAutoRepeat(true);
}

void QToolButtonPrivate::_q_updateButtonDown()
{
    Q_Q(QToolButton);
    menuButtonDown = false;
    if (q->isDown())
        q->setDown(false);
    else
        q->repaint();
}

void QToolButtonPrivate::_q_menuTriggered(QAction *action)
{
    Q_Q(QToolButton);
    if (action && !actionsCopy.contains(action))
        emit q->triggered(action);
}
#endif // QT_NO_MENU


#ifndef QT_NO_MENU
/*! \enum QToolButton::ToolButtonPopupMode

    Describes how a menu should be popped up for tool buttons that has
    a menu set or contains a list of actions.

    \value DelayedPopup After pressing and holding the tool button
    down for a certain amount of time (the timeout is style dependant,
    see QStyle::SH_ToolButton_PopupDelay), the menu is displayed.  A
    typical application example is the "back" button in some web
    browsers's tool bars. If the user clicks it, the browser simply
    browses back to the previous page.  If the user presses and holds
    the button down for a while, the tool button shows a menu
    containing the current history list

    \value MenuButtonPopup In this mode the tool button displays a
    special arrow to indicate that a menu is present. The menu is
    displayed when the arrow part of the button is pressed.

    \value InstantPopup The menu is displayed, without delay, when
    the tool button is pressed. In this mode, the button's own action
    is not triggered.
*/

/*!
    \property QToolButton::popupMode
    \brief describes the way that popup menus are used with tool buttons

    By default, this property is set to \l DelayedPopup.
*/

void QToolButton::setPopupMode(ToolButtonPopupMode mode)
{
    Q_D(QToolButton);
    d->popupMode = mode;
}

QToolButton::ToolButtonPopupMode QToolButton::popupMode() const
{
    Q_D(const QToolButton);
    return d->popupMode;
}
#endif

/*!
    \property QToolButton::autoRaise
    \brief whether auto-raising is enabled or not.

    The default is disabled (i.e. false).

    This property is currently ignored on Mac OS X when using QMacStyle.
*/
void QToolButton::setAutoRaise(bool enable)
{
    Q_D(QToolButton);
    d->autoRaise = enable;

    update();
}

bool QToolButton::autoRaise() const
{
    Q_D(const QToolButton);
    return d->autoRaise;
}

/*!
  Sets the default action to \a action.

  If a tool button has a default action, the action defines the
  button's properties like text, icon, tool tip, etc.
 */
void QToolButton::setDefaultAction(QAction *action)
{
    Q_D(QToolButton);
#ifndef QT_NO_MENU
    bool hadMenu = false;
    hadMenu = d->hasMenu();
#endif
    d->defaultAction = action;
    if (!action)
        return;
    if (!actions().contains(action))
        addAction(action);
    setText(action->iconText());
    setIcon(action->icon());
#ifndef QT_NO_TOOLTIP
    setToolTip(action->toolTip());
#endif
#ifndef QT_NO_STATUSTIP
    setStatusTip(action->statusTip());
#endif
#ifndef QT_NO_WHATSTHIS
    setWhatsThis(action->whatsThis());
#endif
#ifndef QT_NO_MENU
    if (action->menu() && !hadMenu) {
        // new 'default' popup mode defined introduced by tool bar. We
        // should have changed QToolButton's default instead. Do that
        // in 4.2.
        setPopupMode(QToolButton::MenuButtonPopup);
    }
#endif
    setCheckable(action->isCheckable());
    setChecked(action->isChecked());
    setEnabled(action->isEnabled());
    if (action->d_func()->fontSet)
        setFont(action->font());
}


/*!
  Returns the default action.

  \sa setDefaultAction()
 */
QAction *QToolButton::defaultAction() const
{
    Q_D(const QToolButton);
    return d->defaultAction;
}



/*!
  \reimp
 */
void QToolButton::nextCheckState()
{
    Q_D(QToolButton);
    if (!d->defaultAction)
        QAbstractButton::nextCheckState();
    else
        d->defaultAction->trigger();
}

/*! \reimp */
bool QToolButton::event(QEvent *event)
{
    switch(event->type()) {
    case QEvent::HoverEnter:
    case QEvent::HoverLeave:
    case QEvent::HoverMove:
    if (const QHoverEvent *he = static_cast<const QHoverEvent *>(event))
        d_func()->updateHoverControl(he->pos());
        break;
    default:
        break;
    }
    return QAbstractButton::event(event);
}

QT_END_NAMESPACE

#include "moc_qtoolbutton.cpp"

#endif
