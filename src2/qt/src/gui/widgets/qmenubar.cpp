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

#include <qmenubar.h>

#include <qstyle.h>
#include <qlayout.h>
#include <qapplication.h>
#include <qdesktopwidget.h>
#ifndef QT_NO_ACCESSIBILITY
# include <qaccessible.h>
#endif
#include <qpainter.h>
#include <qstylepainter.h>
#include <qevent.h>
#include <qmainwindow.h>
#include <qtoolbar.h>
#include <qtoolbutton.h>
#include <qwhatsthis.h>
#ifdef Q_WS_X11
#include <qpluginloader.h>
#endif

#ifndef QT_NO_MENUBAR

#ifdef QT3_SUPPORT
#include <private/qaction_p.h>
#include <qmenudata.h>
#endif

#include "qmenu_p.h"
#include "qmenubar_p.h"
#include "qdebug.h"
#ifdef Q_WS_X11
#include "qmenubar_x11_p.h"
#endif

#ifdef Q_WS_WINCE
extern bool qt_wince_is_mobile(); //defined in qguifunctions_wce.cpp
#endif

#ifdef QT_SOFTKEYS_ENABLED
#include <private/qsoftkeymanager_p.h>
#endif

QT_BEGIN_NAMESPACE

class QMenuBarExtension : public QToolButton
{
public:
    explicit QMenuBarExtension(QWidget *parent);

    QSize sizeHint() const;
    void paintEvent(QPaintEvent *);
};

QMenuBarExtension::QMenuBarExtension(QWidget *parent)
    : QToolButton(parent)
{
    setObjectName(QLatin1String("qt_menubar_ext_button"));
    setAutoRaise(true);
#ifndef QT_NO_MENU
    setPopupMode(QToolButton::InstantPopup);
#endif
    setIcon(style()->standardIcon(QStyle::SP_ToolBarHorizontalExtensionButton, 0, parentWidget()));
}

void QMenuBarExtension::paintEvent(QPaintEvent *)
{
    QStylePainter p(this);
    QStyleOptionToolButton opt;
    initStyleOption(&opt);
    // We do not need to draw both extension arrows
    opt.features &= ~QStyleOptionToolButton::HasMenu;
    p.drawComplexControl(QStyle::CC_ToolButton, opt);
}


QSize QMenuBarExtension::sizeHint() const
{
    int ext = style()->pixelMetric(QStyle::PM_ToolBarExtensionExtent, 0, parentWidget());
    return QSize(ext, ext);
}


/*!
    \internal
*/
QAction *QMenuBarPrivate::actionAt(QPoint p) const
{
    for(int i = 0; i < actions.size(); ++i) {
        if(actionRect(actions.at(i)).contains(p))
            return actions.at(i);
    }
    return 0;
}

QRect QMenuBarPrivate::menuRect(bool extVisible) const
{
    Q_Q(const QMenuBar);

    int hmargin = q->style()->pixelMetric(QStyle::PM_MenuBarPanelWidth, 0, q);
    QRect result = q->rect();
    result.adjust(hmargin, 0, -hmargin, 0);

    if (extVisible) {
        if (q->isRightToLeft())
            result.setLeft(result.left() + extension->sizeHint().width());
        else
            result.setWidth(result.width() - extension->sizeHint().width());
    }

    if (leftWidget && leftWidget->isVisible()) {
        QSize sz = leftWidget->sizeHint();
        if (q->isRightToLeft())
            result.setRight(result.right() - sz.width());
        else
            result.setLeft(result.left() + sz.width());
    }

    if (rightWidget && rightWidget->isVisible()) {
        QSize sz = rightWidget->sizeHint();
        if (q->isRightToLeft())
            result.setLeft(result.left() + sz.width());
        else
            result.setRight(result.right() - sz.width());
    }

    return result;
}

bool QMenuBarPrivate::isVisible(QAction *action)
{
    return !hiddenActions.contains(action);
}

void QMenuBarPrivate::updateGeometries()
{
    Q_Q(QMenuBar);
    if(!itemsDirty)
        return;
    int q_width = q->width()-(q->style()->pixelMetric(QStyle::PM_MenuBarPanelWidth, 0, q)*2);
    int q_start = -1;
    if(
#ifdef Q_WS_X11
        platformMenuBar->allowCornerWidgets() &&
#endif
        (leftWidget || rightWidget)) {
        int vmargin = q->style()->pixelMetric(QStyle::PM_MenuBarVMargin, 0, q)
                      + q->style()->pixelMetric(QStyle::PM_MenuBarPanelWidth, 0, q);
        int hmargin = q->style()->pixelMetric(QStyle::PM_MenuBarHMargin, 0, q)
                      + q->style()->pixelMetric(QStyle::PM_MenuBarPanelWidth, 0, q);
        if (leftWidget && leftWidget->isVisible()) {
            QSize sz = leftWidget->sizeHint();
            q_width -= sz.width();
            q_start = sz.width();
            QPoint pos(hmargin, (q->height() - leftWidget->height()) / 2);
            QRect vRect = QStyle::visualRect(q->layoutDirection(), q->rect(), QRect(pos, sz));
            leftWidget->setGeometry(vRect);
        }
        if (rightWidget && rightWidget->isVisible()) {
            QSize sz = rightWidget->sizeHint();
            q_width -= sz.width();
            QPoint pos(q->width() - sz.width() - hmargin, vmargin);
            QRect vRect = QStyle::visualRect(q->layoutDirection(), q->rect(), QRect(pos, sz));
            rightWidget->setGeometry(vRect);
        }
    }

#ifdef Q_WS_MAC
    if(q->isNativeMenuBar()) {//nothing to see here folks, move along..
        itemsDirty = false;
        return;
    }
#endif
    calcActionRects(q_width, q_start);
    currentAction = 0;
#ifndef QT_NO_SHORTCUT
    if(
#ifdef Q_WS_X11
        !platformMenuBar->shortcutsHandledByNativeMenuBar() &&
#endif
        itemsDirty) {
        for(int j = 0; j < shortcutIndexMap.size(); ++j)
            q->releaseShortcut(shortcutIndexMap.value(j));
        shortcutIndexMap.resize(0); // faster than clear
        for(int i = 0; i < actions.count(); i++)
            shortcutIndexMap.append(q->grabShortcut(QKeySequence::mnemonic(actions.at(i)->text())));
    }
#endif
#ifdef Q_WS_X11
    if(q->isNativeMenuBar()) {//nothing to see here folks, move along..
        itemsDirty = false;
        return;
    }
#endif
    itemsDirty = false;

    hiddenActions.clear();
    //this is the menu rectangle without any extension
    QRect menuRect = this->menuRect(false);

    //we try to see if the actions will fit there
    bool hasHiddenActions = false;
    for (int i = 0; i < actions.count(); ++i) {
        const QRect &rect = actionRects.at(i);
        if (rect.isValid() && !menuRect.contains(rect)) {
            hasHiddenActions = true;
            break;
        }
    }

    //...and if not, determine the ones that fit on the menu with the extension visible
    if (hasHiddenActions) {
        menuRect = this->menuRect(true);
        for (int i = 0; i < actions.count(); ++i) {
            const QRect &rect = actionRects.at(i);
            if (rect.isValid() && !menuRect.contains(rect)) {
                hiddenActions.append(actions.at(i));
            }
        }
    }

    if (hiddenActions.count() > 0) {
        QMenu *pop = extension->menu();
        if (!pop) {
            pop = new QMenu(q);
            extension->setMenu(pop);
        }
        pop->clear();
        pop->addActions(hiddenActions);

        int vmargin = q->style()->pixelMetric(QStyle::PM_MenuBarVMargin, 0, q);
        int x = q->isRightToLeft()
                ? menuRect.left() - extension->sizeHint().width() + 1
                : menuRect.right();
        extension->setGeometry(x, vmargin, extension->sizeHint().width(), menuRect.height() - vmargin*2);
        extension->show();
    } else {
        extension->hide();
    }
    q->updateGeometry();
#ifdef QT3_SUPPORT
    if (parent) {
        QMenubarUpdatedEvent menubarUpdated(q);
        QApplication::sendEvent(parent, &menubarUpdated);
    }
#endif
}

QRect QMenuBarPrivate::actionRect(QAction *act) const
{
    const int index = actions.indexOf(act);

    //makes sure the geometries are up-to-date
    const_cast<QMenuBarPrivate*>(this)->updateGeometries();

    if (index < 0 || index >= actionRects.count())
        return QRect(); // that can happen in case of native menubar

    return actionRects.at(index);
}

void QMenuBarPrivate::focusFirstAction()
{
    if(!currentAction) {
        updateGeometries();
        int index = 0;
        while (index < actions.count() && actionRects.at(index).isNull()) ++index;
        if (index < actions.count())
            setCurrentAction(actions.at(index));
    }
}

void QMenuBarPrivate::setKeyboardMode(bool b)
{
    Q_Q(QMenuBar);
    if (b && !q->style()->styleHint(QStyle::SH_MenuBar_AltKeyNavigation, 0, q)) {
        setCurrentAction(0);
        return;
    }
    keyboardState = b;
    if(b) {
        QWidget *fw = QApplication::focusWidget();
        if (fw != q)
            keyboardFocusWidget = fw;
        focusFirstAction();
        q->setFocus(Qt::MenuBarFocusReason);
    } else {
        if(!popupState)
            setCurrentAction(0);
        if(keyboardFocusWidget) {
            if (QApplication::focusWidget() == q)
                keyboardFocusWidget->setFocus(Qt::MenuBarFocusReason);
            keyboardFocusWidget = 0;
        }
    }
    q->update();
}

void QMenuBarPrivate::popupAction(QAction *action, bool activateFirst)
{
    Q_Q(QMenuBar);
    if(!action || !action->menu() || closePopupMode)
        return;
    popupState = true;
    if (action->isEnabled() && action->menu()->isEnabled()) {
        closePopupMode = 0;
        activeMenu = action->menu();
        activeMenu->d_func()->causedPopup.widget = q;
        activeMenu->d_func()->causedPopup.action = action;

        QRect adjustedActionRect = actionRect(action);
        QPoint pos(q->mapToGlobal(QPoint(adjustedActionRect.left(), adjustedActionRect.bottom() + 1)));
        QSize popup_size = activeMenu->sizeHint();

        //we put the popup menu on the screen containing the bottom-center of the action rect
        QRect screenRect = QApplication::desktop()->screenGeometry(pos + QPoint(adjustedActionRect.width() / 2, 0));
        pos = QPoint(qMax(pos.x(), screenRect.x()), qMax(pos.y(), screenRect.y()));

        const bool fitUp = (q->mapToGlobal(adjustedActionRect.topLeft()).y() >= popup_size.height());
        const bool fitDown = (pos.y() + popup_size.height() <= screenRect.bottom());
        const bool rtl = q->isRightToLeft();
        const int actionWidth = adjustedActionRect.width();

        if (!fitUp && !fitDown) { //we should shift the menu
            bool shouldShiftToRight = !rtl;
            if (rtl && popup_size.width() > pos.x())
                shouldShiftToRight = true;
            else if (actionWidth + popup_size.width() + pos.x() > screenRect.right())
                shouldShiftToRight = false;

            if (shouldShiftToRight) {
                pos.rx() += actionWidth + (rtl ? popup_size.width() : 0);
            } else {
                //shift to left
                if (!rtl)
                    pos.rx() -= popup_size.width();
            }
        } else if (rtl) {
            pos.rx() += actionWidth;
        }

        if(!defaultPopDown || (fitUp && !fitDown))
            pos.setY(qMax(screenRect.y(), q->mapToGlobal(QPoint(0, adjustedActionRect.top()-popup_size.height())).y()));
        activeMenu->popup(pos);
        if(activateFirst)
            activeMenu->d_func()->setFirstActionActive();
    }
    q->update(actionRect(action));
}

void QMenuBarPrivate::setCurrentAction(QAction *action, bool popup, bool activateFirst)
{
    if(currentAction == action && popup == popupState)
        return;

    autoReleaseTimer.stop();

    doChildEffects = (popup && !activeMenu);
    Q_Q(QMenuBar);
    QWidget *fw = 0;
    if(QMenu *menu = activeMenu) {
        activeMenu = 0;
        if (popup) {
            fw = q->window()->focusWidget();
            q->setFocus(Qt::NoFocusReason);
        }
        menu->hide();
    }

    if(currentAction)
        q->update(actionRect(currentAction));

    popupState = popup;
#ifndef QT_NO_STATUSTIP
    QAction *previousAction = currentAction;
#endif
    currentAction = action;
    if (action) {
        activateAction(action, QAction::Hover);
        if(popup)
            popupAction(action, activateFirst);
        q->update(actionRect(action));
#ifndef QT_NO_STATUSTIP
    }  else if (previousAction) {
        QString empty;
        QStatusTipEvent tip(empty);
        QApplication::sendEvent(q, &tip);
#endif
    }
    if (fw)
        fw->setFocus(Qt::NoFocusReason);
}

void QMenuBarPrivate::calcActionRects(int max_width, int start) const
{
    Q_Q(const QMenuBar);

    if(!itemsDirty)
        return;

    //let's reinitialize the buffer
    actionRects.resize(actions.count());
    actionRects.fill(QRect());

    const QStyle *style = q->style();

    const int itemSpacing = style->pixelMetric(QStyle::PM_MenuBarItemSpacing, 0, q);
    int max_item_height = 0, separator = -1, separator_start = 0, separator_len = 0;

    //calculate size
    const QFontMetrics fm = q->fontMetrics();
    const int hmargin = style->pixelMetric(QStyle::PM_MenuBarHMargin, 0, q),
              vmargin = style->pixelMetric(QStyle::PM_MenuBarVMargin, 0, q),
                icone = style->pixelMetric(QStyle::PM_SmallIconSize, 0, q);
    for(int i = 0; i < actions.count(); i++) {
        QAction *action = actions.at(i);
        if(!action->isVisible())
            continue;

        QSize sz;

        //calc what I think the size is..
        if(action->isSeparator()) {
            if (style->styleHint(QStyle::SH_DrawMenuBarSeparator, 0, q))
                separator = i;
            continue; //we don't really position these!
        } else {
            const QString s = action->text();
            QIcon is = action->icon();
            // If an icon is set, only the icon is visible
            if (!is.isNull())
                sz = sz.expandedTo(QSize(icone, icone));
            else if (!s.isEmpty())
                sz = fm.size(Qt::TextShowMnemonic, s);
        }

        //let the style modify the above size..
        QStyleOptionMenuItem opt;
        q->initStyleOption(&opt, action);
        sz = q->style()->sizeFromContents(QStyle::CT_MenuBarItem, &opt, sz, q);

        if(!sz.isEmpty()) {
            { //update the separator state
                int iWidth = sz.width() + itemSpacing;
                if(separator == -1)
                    separator_start += iWidth;
                else
                    separator_len += iWidth;
            }
            //maximum height
            max_item_height = qMax(max_item_height, sz.height());
            //append
            actionRects[i] = QRect(0, 0, sz.width(), sz.height());
        }
    }

    //calculate position
    const int fw = q->style()->pixelMetric(QStyle::PM_MenuBarPanelWidth, 0, q);
    int x = fw + ((start == -1) ? hmargin : start) + itemSpacing;
    int y = fw + vmargin;
    for(int i = 0; i < actions.count(); i++) {
        QRect &rect = actionRects[i];
        if (rect.isNull())
            continue;

        //resize
        rect.setHeight(max_item_height);

        //move
        if(separator != -1 && i >= separator) { //after the separator
            int left = (max_width - separator_len - hmargin - itemSpacing) + (x - separator_start - hmargin);
            if(left < separator_start) { //wrap
                separator_start = x = hmargin;
                y += max_item_height;
            }
            rect.moveLeft(left);
        } else {
            rect.moveLeft(x);
        }
        rect.moveTop(y);

        //keep moving along..
        x += rect.width() + itemSpacing;

        //make sure we follow the layout direction
        rect = QStyle::visualRect(q->layoutDirection(), q->rect(), rect);
    }
}

void QMenuBarPrivate::activateAction(QAction *action, QAction::ActionEvent action_e)
{
    Q_Q(QMenuBar);
    if (!action || !action->isEnabled())
        return;
    action->activate(action_e);
    if (action_e == QAction::Hover)
        action->showStatusText(q);

//     if(action_e == QAction::Trigger)
//         emit q->activated(action);
//     else if(action_e == QAction::Hover)
//         emit q->highlighted(action);
}


void QMenuBarPrivate::_q_actionTriggered()
{
    Q_Q(QMenuBar);
    if (QAction *action = qobject_cast<QAction *>(q->sender())) {
        emit q->triggered(action);
#ifdef QT3_SUPPORT
        emit q->activated(q->findIdForAction(action));
#endif
    }
}

void QMenuBarPrivate::_q_actionHovered()
{
    Q_Q(QMenuBar);
    if (QAction *action = qobject_cast<QAction *>(q->sender())) {
        emit q->hovered(action);
#ifndef QT_NO_ACCESSIBILITY
        if (QAccessible::isActive()) {
            int actionIndex = actions.indexOf(action);
            ++actionIndex;
            QAccessible::updateAccessibility(q, actionIndex, QAccessible::Focus);
            QAccessible::updateAccessibility(q, actionIndex, QAccessible::Selection);
        }
#endif //QT_NO_ACCESSIBILITY
#ifdef QT3_SUPPORT
        emit q->highlighted(q->findIdForAction(action));
#endif
    }
}

/*!
    Initialize \a option with the values from the menu bar and information from \a action. This method
    is useful for subclasses when they need a QStyleOptionMenuItem, but don't want
    to fill in all the information themselves.

    \sa QStyleOption::initFrom() QMenu::initStyleOption()
*/
void QMenuBar::initStyleOption(QStyleOptionMenuItem *option, const QAction *action) const
{
    if (!option || !action)
        return;
    Q_D(const QMenuBar);
    option->palette = palette();
    option->state = QStyle::State_None;
    if (isEnabled() && action->isEnabled())
        option->state |= QStyle::State_Enabled;
    else
        option->palette.setCurrentColorGroup(QPalette::Disabled);
    option->fontMetrics = fontMetrics();
    if (d->currentAction && d->currentAction == action) {
        option->state |= QStyle::State_Selected;
        if (d->popupState && !d->closePopupMode)
            option->state |= QStyle::State_Sunken;
    }
    if (hasFocus() || d->currentAction)
        option->state |= QStyle::State_HasFocus;
    option->menuRect = rect();
    option->menuItemType = QStyleOptionMenuItem::Normal;
    option->checkType = QStyleOptionMenuItem::NotCheckable;
    option->text = action->text();
    option->icon = action->icon();
}

/*!
    \class QMenuBar
    \brief The QMenuBar class provides a horizontal menu bar.

    \ingroup mainwindow-classes

    A menu bar consists of a list of pull-down menu items. You add
    menu items with addMenu(). For example, asuming that \c menubar
    is a pointer to a QMenuBar and \c fileMenu is a pointer to a
    QMenu, the following statement inserts the menu into the menu bar:
    \snippet doc/src/snippets/code/src_gui_widgets_qmenubar.cpp 0

    The ampersand in the menu item's text sets Alt+F as a shortcut for
    this menu. (You can use "\&\&" to get a real ampersand in the menu
    bar.)

    There is no need to lay out a menu bar. It automatically sets its
    own geometry to the top of the parent widget and changes it
    appropriately whenever the parent is resized.

    \section1 Usage

    In most main window style applications you would use the
    \l{QMainWindow::}{menuBar()} function provided in QMainWindow,
    adding \l{QMenu}s to the menu bar and adding \l{QAction}s to the
    pop-up menus.

    Example (from the \l{mainwindows/menus}{Menus} example):

    \snippet examples/mainwindows/menus/mainwindow.cpp 9

    Menu items may be removed with removeAction().

    Widgets can be added to menus by using instances of the QWidgetAction
    class to hold them. These actions can then be inserted into menus
    in the usual way; see the QMenu documentation for more details.

    \section1 Platform Dependent Look and Feel

    Different platforms have different requirements for the appearance
    of menu bars and their behavior when the user interacts with them.
    For example, Windows systems are often configured so that the
    underlined character mnemonics that indicate keyboard shortcuts
    for items in the menu bar are only shown when the \gui{Alt} key is
    pressed.

    \table

    \row \o \inlineimage plastique-menubar.png A menu bar shown in the
    Plastique widget style.

    \o The \l{QPlastiqueStyle}{Plastique widget style}, like most
    other styles, handles the \gui{Help} menu in the same way as it
    handles any other menu.

    \row \o \inlineimage motif-menubar.png A menu bar shown in the
    Motif widget style.

    \o The \l{QMotifStyle}{Motif widget style} treats \gui{Help} menus
    in a special way, placing them at right-hand end of the menu bar.

    \endtable

    \section1 QMenuBar on Mac OS X

    QMenuBar on Mac OS X is a wrapper for using the system-wide menu bar.
    If you have multiple menu bars in one dialog the outermost menu bar
    (normally inside a widget with widget flag Qt::Window) will
    be used for the system-wide menu bar.

    Qt for Mac OS X also provides a menu bar merging feature to make
    QMenuBar conform more closely to accepted Mac OS X menu bar layout.
    The merging functionality is based on string matching the title of
    a QMenu entry. These strings are translated (using QObject::tr())
    in the "QMenuBar" context. If an entry is moved its slots will still
    fire as if it was in the original place. The table below outlines
    the strings looked for and where the entry is placed if matched:

    \table
    \header \i String matches \i Placement \i Notes
    \row \i about.*
         \i Application Menu | About <application name>
         \i The application name is fetched from the \c {Info.plist} file
            (see note below). If this entry is not found no About item
            will appear in the Application Menu.
    \row \i config, options, setup, settings or preferences
         \i Application Menu | Preferences
         \i If this entry is not found the Settings item will be disabled
    \row \i quit or exit
         \i Application Menu | Quit <application name>
         \i If this entry is not found a default Quit item will be
            created to call QApplication::quit()
    \endtable

    You can override this behavior by using the QAction::menuRole()
    property.

    If you want all windows in a Mac application to share one menu
    bar, you must create a menu bar that does not have a parent.
    Create a parent-less menu bar this way:

    \snippet doc/src/snippets/code/src_gui_widgets_qmenubar.cpp 1

    \bold{Note:} Do \e{not} call QMainWindow::menuBar() to create the
    shared menu bar, because that menu bar will have the QMainWindow
    as its parent. That menu bar would only be displayed for the
    parent QMainWindow.

    \bold{Note:} The text used for the application name in the menu
    bar is obtained from the value set in the \c{Info.plist} file in
    the application's bundle. See \l{Deploying an Application on
    Mac OS X} for more information.

    \section1 QMenuBar on Windows CE

    QMenuBar on Windows CE is a wrapper for using the system-wide menu bar,
    similar to the Mac.  This feature is activated for Windows Mobile
    and integrates QMenuBar with the native soft keys. The left soft
    key can be controlled with QMenuBar::setDefaultAction() and the
    right soft key can be used to access the menu bar.

    The hovered() signal is not supported for the native menu
    integration. Also, it is not possible to display an icon in a
    native menu on Windows Mobile.

    \section1 Examples

    The \l{mainwindows/menus}{Menus} example shows how to use QMenuBar
    and QMenu.  The other \l{Main Window Examples}{main window
    application examples} also provide menus using these classes.

    \sa QMenu, QShortcut, QAction,
        {http://developer.apple.com/documentation/UserExperience/Conceptual/AppleHIGuidelines/XHIGIntro/XHIGIntro.html}{Introduction to Apple Human Interface Guidelines},
        {fowler}{GUI Design Handbook: Menu Bar}, {Menus Example}
*/


void QMenuBarPrivate::init()
{
    Q_Q(QMenuBar);
    q->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Minimum);
    q->setAttribute(Qt::WA_CustomWhatsThis);
#ifdef Q_WS_MAC
    macCreateMenuBar(q->parentWidget());
    if(mac_menubar)
        q->hide();
#endif
#ifdef Q_WS_WINCE
    if (qt_wince_is_mobile()) {
        wceCreateMenuBar(q->parentWidget());
        if(wce_menubar)
            q->hide();
    }
    else {
        QApplication::setAttribute(Qt::AA_DontUseNativeMenuBar, true);
    }
#endif
#ifdef Q_WS_X11
    platformMenuBar = qt_guiPlatformMenuBarFactory()->create();
    platformMenuBar->init(q);
#endif

    q->setBackgroundRole(QPalette::Button);
    oldWindow = oldParent = 0;
#ifdef QT3_SUPPORT
    doAutoResize = false;
#endif
#ifdef QT_SOFTKEYS_ENABLED
    menuBarAction = 0;
#endif
#ifdef Q_WS_X11
    cornerWidgetToolBar = 0;
    cornerWidgetContainer = 0;
#endif
    handleReparent();
    q->setMouseTracking(q->style()->styleHint(QStyle::SH_MenuBar_MouseTracking, 0, q));

    extension = new QMenuBarExtension(q);
    extension->setFocusPolicy(Qt::NoFocus);
    extension->hide();
}

//Gets the next action for keyboard navigation
QAction *QMenuBarPrivate::getNextAction(const int _start, const int increment) const
{
    Q_Q(const QMenuBar);
    const_cast<QMenuBarPrivate*>(this)->updateGeometries();
    bool allowActiveAndDisabled = q->style()->styleHint(QStyle::SH_Menu_AllowActiveAndDisabled, 0, q);
    const int start = (_start == -1 && increment == -1) ? actions.count() : _start;
    const int end =  increment == -1 ? 0 : actions.count() - 1;

    for (int i = start; i != end;) {
        i += increment;
        QAction *current = actions.at(i);
        if (!actionRects.at(i).isNull() && (allowActiveAndDisabled || current->isEnabled()))
            return current;
    }

    if (_start != -1) //let's try from the beginning or the end
        return getNextAction(-1, increment);

    return 0;
}

/*!
    Constructs a menu bar with parent \a parent.
*/
QMenuBar::QMenuBar(QWidget *parent) : QWidget(*new QMenuBarPrivate, parent, 0)
{
    Q_D(QMenuBar);
    d->init();
}

#ifdef QT3_SUPPORT
/*!
    Use one of the constructors that doesn't take the \a name
    argument and then use setObjectName() instead.
*/
QMenuBar::QMenuBar(QWidget *parent, const char *name) : QWidget(*new QMenuBarPrivate, parent, 0)
{
    Q_D(QMenuBar);
    d->init();
    setObjectName(QString::fromAscii(name));
}
#endif

/*!
    Destroys the menu bar.
*/
QMenuBar::~QMenuBar()
{
#ifdef Q_WS_MAC
    Q_D(QMenuBar);
    d->macDestroyMenuBar();
#endif
#ifdef Q_WS_WINCE
    Q_D(QMenuBar);
    if (qt_wince_is_mobile())
        d->wceDestroyMenuBar();
#endif
#ifdef Q_WS_S60
    Q_D(QMenuBar);
    d->symbianDestroyMenuBar();
#endif
#ifdef Q_WS_X11
    Q_D(QMenuBar);
    delete d->cornerWidgetToolBar;
#endif
}

/*!
    \overload

    This convenience function creates a new action with \a text.
    The function adds the newly created action to the menu's
    list of actions, and returns it.

    \sa QWidget::addAction(), QWidget::actions()
*/
QAction *QMenuBar::addAction(const QString &text)
{
    QAction *ret = new QAction(text, this);
    addAction(ret);
    return ret;
}

/*!
    \overload

    This convenience function creates a new action with the given \a
    text. The action's triggered() signal is connected to the \a
    receiver's \a member slot. The function adds the newly created
    action to the menu's list of actions and returns it.

    \sa QWidget::addAction(), QWidget::actions()
*/
QAction *QMenuBar::addAction(const QString &text, const QObject *receiver, const char* member)
{
    QAction *ret = new QAction(text, this);
    QObject::connect(ret, SIGNAL(triggered(bool)), receiver, member);
    addAction(ret);
    return ret;
}

/*!
  Appends a new QMenu with \a title to the menu bar. The menu bar
  takes ownership of the menu. Returns the new menu.

  \sa QWidget::addAction() QMenu::menuAction()
*/
QMenu *QMenuBar::addMenu(const QString &title)
{
    QMenu *menu = new QMenu(title, this);
    addAction(menu->menuAction());
    return menu;
}

/*!
  Appends a new QMenu with \a icon and \a title to the menu bar. The menu bar
  takes ownership of the menu. Returns the new menu.

  \sa QWidget::addAction() QMenu::menuAction()
*/
QMenu *QMenuBar::addMenu(const QIcon &icon, const QString &title)
{
    QMenu *menu = new QMenu(title, this);
    menu->setIcon(icon);
    addAction(menu->menuAction());
    return menu;
}

/*!
    Appends \a menu to the menu bar. Returns the menu's menuAction().

    \note The returned QAction object can be used to hide the corresponding
    menu.

    \sa QWidget::addAction() QMenu::menuAction()
*/
QAction *QMenuBar::addMenu(QMenu *menu)
{
    QAction *action = menu->menuAction();
    addAction(action);
    return action;
}

/*!
  Appends a separator to the menu.
*/
QAction *QMenuBar::addSeparator()
{
    QAction *ret = new QAction(this);
    ret->setSeparator(true);
    addAction(ret);
    return ret;
}

/*!
    This convenience function creates a new separator action, i.e. an
    action with QAction::isSeparator() returning true. The function inserts
    the newly created action into this menu bar's list of actions before
    action \a before and returns it.

    \sa QWidget::insertAction(), addSeparator()
*/
QAction *QMenuBar::insertSeparator(QAction *before)
{
    QAction *action = new QAction(this);
    action->setSeparator(true);
    insertAction(before, action);
    return action;
}

/*!
  This convenience function inserts \a menu before action \a before
  and returns the menus menuAction().

  \sa QWidget::insertAction() addMenu()
*/
QAction *QMenuBar::insertMenu(QAction *before, QMenu *menu)
{
    QAction *action = menu->menuAction();
    insertAction(before, action);
    return action;
}

/*!
  Returns the QAction that is currently highlighted. A null pointer
  will be returned if no action is currently selected.
*/
QAction *QMenuBar::activeAction() const
{
    Q_D(const QMenuBar);
    return d->currentAction;
}

/*!
    \since 4.1

    Sets the currently highlighted action to \a act.
*/
void QMenuBar::setActiveAction(QAction *act)
{
    Q_D(QMenuBar);
    d->setCurrentAction(act, true, false);
}


/*!
    Removes all the actions from the menu bar.

    \note On Mac OS X, menu items that have been merged to the system
    menu bar are not removed by this function. One way to handle this
    would be to remove the extra actions yourself. You can set the
    \l{QAction::MenuRole}{menu role} on the different menus, so that
    you know ahead of time which menu items get merged and which do
    not. Then decide what to recreate or remove yourself.

    \sa removeAction()
*/
void QMenuBar::clear()
{
    QList<QAction*> acts = actions();
    for(int i = 0; i < acts.size(); i++)
        removeAction(acts[i]);
}

/*!
    \property QMenuBar::defaultUp
    \brief the popup orientation

    The default popup orientation. By default, menus pop "down" the
    screen. By setting the property to true, the menu will pop "up".
    You might call this for menus that are \e below the document to
    which they refer.

    If the menu would not fit on the screen, the other direction is
    used automatically.
*/
void QMenuBar::setDefaultUp(bool b)
{
    Q_D(QMenuBar);
    d->defaultPopDown = !b;
}

bool QMenuBar::isDefaultUp() const
{
    Q_D(const QMenuBar);
    return !d->defaultPopDown;
}

/*!
  \reimp
*/
void QMenuBar::resizeEvent(QResizeEvent *)
{
    Q_D(QMenuBar);
    d->itemsDirty = true;
    d->updateGeometries();
}

/*!
  \reimp
*/
void QMenuBar::paintEvent(QPaintEvent *e)
{
    Q_D(QMenuBar);
    QPainter p(this);
    QRegion emptyArea(rect());

    //draw the items
    for (int i = 0; i < d->actions.count(); ++i) {
        QAction *action = d->actions.at(i);
        QRect adjustedActionRect = d->actionRect(action);
        if (adjustedActionRect.isEmpty() || !d->isVisible(action))
            continue;
        if(!e->rect().intersects(adjustedActionRect))
            continue;

        emptyArea -= adjustedActionRect;
        QStyleOptionMenuItem opt;
        initStyleOption(&opt, action);
        opt.rect = adjustedActionRect;
        p.setClipRect(adjustedActionRect);
        style()->drawControl(QStyle::CE_MenuBarItem, &opt, &p, this);
    }
     //draw border
    if(int fw = style()->pixelMetric(QStyle::PM_MenuBarPanelWidth, 0, this)) {
        QRegion borderReg;
        borderReg += QRect(0, 0, fw, height()); //left
        borderReg += QRect(width()-fw, 0, fw, height()); //right
        borderReg += QRect(0, 0, width(), fw); //top
        borderReg += QRect(0, height()-fw, width(), fw); //bottom
        p.setClipRegion(borderReg);
        emptyArea -= borderReg;
        QStyleOptionFrame frame;
        frame.rect = rect();
        frame.palette = palette();
        frame.state = QStyle::State_None;
        frame.lineWidth = style()->pixelMetric(QStyle::PM_MenuBarPanelWidth);
        frame.midLineWidth = 0;
        style()->drawPrimitive(QStyle::PE_PanelMenuBar, &frame, &p, this);
    }
    p.setClipRegion(emptyArea);
    QStyleOptionMenuItem menuOpt;
    menuOpt.palette = palette();
    menuOpt.state = QStyle::State_None;
    menuOpt.menuItemType = QStyleOptionMenuItem::EmptyArea;
    menuOpt.checkType = QStyleOptionMenuItem::NotCheckable;
    menuOpt.rect = rect();
    menuOpt.menuRect = rect();
    style()->drawControl(QStyle::CE_MenuBarEmptyArea, &menuOpt, &p, this);
}

/*!
  \reimp
*/
void QMenuBar::setVisible(bool visible)
{
#ifdef Q_WS_X11
    Q_D(QMenuBar);
    d->platformMenuBar->setVisible(visible);
#else
#if defined(Q_WS_MAC) || defined(Q_OS_WINCE) || defined(Q_WS_S60)
    if (isNativeMenuBar()) {
#ifndef Q_WS_S60
        if (!visible)
            QWidget::setVisible(false);
#endif        
        return;
    }
#endif
    QWidget::setVisible(visible);
#endif // Q_WS_X11
}

/*!
  \reimp
*/
void QMenuBar::mousePressEvent(QMouseEvent *e)
{
    Q_D(QMenuBar);
    if(e->button() != Qt::LeftButton)
        return;

    d->mouseDown = true;

    QAction *action = d->actionAt(e->pos());
    if (!action || !d->isVisible(action)) {
        d->setCurrentAction(0);
#ifndef QT_NO_WHATSTHIS
        if (QWhatsThis::inWhatsThisMode())
            QWhatsThis::showText(e->globalPos(), d->whatsThis, this);
#endif
        return;
    }

    if(d->currentAction == action && d->popupState) {
        if(QMenu *menu = d->activeMenu) {
            d->activeMenu = 0;
            menu->hide();
        }
#ifdef Q_WS_WIN
        if((d->closePopupMode = style()->styleHint(QStyle::SH_MenuBar_DismissOnSecondClick)))
            update(d->actionRect(action));
#endif
    } else {
        d->setCurrentAction(action, true);
    }
}

/*!
  \reimp
*/
void QMenuBar::mouseReleaseEvent(QMouseEvent *e)
{
    Q_D(QMenuBar);
    if(e->button() != Qt::LeftButton || !d->mouseDown)
        return;

    d->mouseDown = false;
    QAction *action = d->actionAt(e->pos());
    if((d->closePopupMode && action == d->currentAction) || !action || !action->menu()) {
        //we set the current action before activating
        //so that we let the leave event set the current back to 0
        d->setCurrentAction(action, false);
        if(action)
            d->activateAction(action, QAction::Trigger);
    }
    d->closePopupMode = 0;
}

/*!
  \reimp
*/
void QMenuBar::keyPressEvent(QKeyEvent *e)
{
    Q_D(QMenuBar);
    d->updateGeometries();
    int key = e->key();
    if(isRightToLeft()) {  // in reverse mode open/close key for submenues are reversed
        if(key == Qt::Key_Left)
            key = Qt::Key_Right;
        else if(key == Qt::Key_Right)
            key = Qt::Key_Left;
    }
    if(key == Qt::Key_Tab) //means right
        key = Qt::Key_Right;
    else if(key == Qt::Key_Backtab) //means left
        key = Qt::Key_Left;

    bool key_consumed = false;
    switch(key) {
    case Qt::Key_Up:
    case Qt::Key_Down:
    case Qt::Key_Enter:
    case Qt::Key_Space:
    case Qt::Key_Return: {
        if(!style()->styleHint(QStyle::SH_MenuBar_AltKeyNavigation, 0, this) || !d->currentAction)
           break;
        if(d->currentAction->menu()) {
            d->popupAction(d->currentAction, true);
        } else if(key == Qt::Key_Enter || key == Qt::Key_Return || key == Qt::Key_Space) {
            d->activateAction(d->currentAction, QAction::Trigger);
            d->setCurrentAction(d->currentAction, false);
            d->setKeyboardMode(false);
        }
        key_consumed = true;
        break; }

    case Qt::Key_Right:
    case Qt::Key_Left: {
        if(d->currentAction) {
            int index = d->actions.indexOf(d->currentAction);
            if (QAction *nextAction = d->getNextAction(index, key == Qt::Key_Left ? -1 : +1)) {
                d->setCurrentAction(nextAction, d->popupState, true);
                key_consumed = true;
            }
        }
        break; }

    case Qt::Key_Escape:
        d->setCurrentAction(0);
        d->setKeyboardMode(false);
        key_consumed = true;
        break;

    default:
        key_consumed = false;
    }

    if(!key_consumed &&
       (!e->modifiers() ||
        (e->modifiers()&(Qt::MetaModifier|Qt::AltModifier))) && e->text().length()==1 && !d->popupState) {
        int clashCount = 0;
        QAction *first = 0, *currentSelected = 0, *firstAfterCurrent = 0;
        {
            QChar c = e->text()[0].toUpper();
            for(int i = 0; i < d->actions.size(); ++i) {
                if (d->actionRects.at(i).isNull())
                    continue;
                QAction *act = d->actions.at(i);
                QString s = act->text();
                if(!s.isEmpty()) {
                    int ampersand = s.indexOf(QLatin1Char('&'));
                    if(ampersand >= 0) {
                        if(s[ampersand+1].toUpper() == c) {
                            clashCount++;
                            if(!first)
                                first = act;
                            if(act == d->currentAction)
                                currentSelected = act;
                            else if (!firstAfterCurrent && currentSelected)
                                firstAfterCurrent = act;
                        }
                    }
                }
            }
        }
        QAction *next_action = 0;
        if(clashCount >= 1) {
            if(clashCount == 1 || !d->currentAction || (currentSelected && !firstAfterCurrent))
                next_action = first;
            else
                next_action = firstAfterCurrent;
        }
        if(next_action) {
            key_consumed = true;
            d->setCurrentAction(next_action, true, true);
        }
    }
    if(key_consumed)
        e->accept();
    else
        e->ignore();
}

/*!
  \reimp
*/
void QMenuBar::mouseMoveEvent(QMouseEvent *e)
{
    Q_D(QMenuBar);
    if (!(e->buttons() & Qt::LeftButton))
        d->mouseDown = false;
    bool popupState = d->popupState || d->mouseDown;
    QAction *action = d->actionAt(e->pos());
    if ((action && d->isVisible(action)) || !popupState)
        d->setCurrentAction(action, popupState);
}

/*!
  \reimp
*/
void QMenuBar::leaveEvent(QEvent *)
{
    Q_D(QMenuBar);
    if((!hasFocus() && !d->popupState) ||
        (d->currentAction && d->currentAction->menu() == 0))
        d->setCurrentAction(0);
}

/*!
  \reimp
*/
void QMenuBar::actionEvent(QActionEvent *e)
{
    Q_D(QMenuBar);
    d->itemsDirty = true;
#ifdef Q_WS_X11
    d->platformMenuBar->actionEvent(e);
#endif
#if defined (Q_WS_MAC) || defined(Q_OS_WINCE) || defined(Q_WS_S60)
    if (isNativeMenuBar()) {
#ifdef Q_WS_MAC
        QMenuBarPrivate::QMacMenuBarPrivate *nativeMenuBar = d->mac_menubar;
#elif defined(Q_WS_S60)
        QMenuBarPrivate::QSymbianMenuBarPrivate *nativeMenuBar = d->symbian_menubar;
#else
        QMenuBarPrivate::QWceMenuBarPrivate *nativeMenuBar = d->wce_menubar;
#endif
        if (!nativeMenuBar)
            return;
        if(e->type() == QEvent::ActionAdded)
            nativeMenuBar->addAction(e->action(), e->before());
        else if(e->type() == QEvent::ActionRemoved)
            nativeMenuBar->removeAction(e->action());
        else if(e->type() == QEvent::ActionChanged)
            nativeMenuBar->syncAction(e->action());
    }
#endif

    if(e->type() == QEvent::ActionAdded) {
        connect(e->action(), SIGNAL(triggered()), this, SLOT(_q_actionTriggered()));
        connect(e->action(), SIGNAL(hovered()), this, SLOT(_q_actionHovered()));
    } else if(e->type() == QEvent::ActionRemoved) {
        e->action()->disconnect(this);
    }
    if (isVisible()) {
        d->updateGeometries();
        update();
    }
}

/*!
  \reimp
*/
void QMenuBar::focusInEvent(QFocusEvent *)
{
    Q_D(QMenuBar);
    if(d->keyboardState)
        d->focusFirstAction();
}

/*!
  \reimp
*/
void QMenuBar::focusOutEvent(QFocusEvent *)
{
    Q_D(QMenuBar);
    if(!d->popupState) {
        d->setCurrentAction(0);
        d->setKeyboardMode(false);
    }
}

/*!
  \reimp
 */
void QMenuBar::timerEvent (QTimerEvent *e)
{
    Q_D(QMenuBar);
    if (e->timerId() == d->autoReleaseTimer.timerId()) {
        d->autoReleaseTimer.stop();
        d->setCurrentAction(0);
    }
    QWidget::timerEvent(e);
}


void QMenuBarPrivate::handleReparent()
{
    Q_Q(QMenuBar);
    QWidget *newParent = q->parentWidget();
    //Note: if parent is reparented, then window may change even if parent doesn't

    // we need to install an event filter on parent, and remove the old one

    if (oldParent != newParent) {
        if (oldParent)
            oldParent->removeEventFilter(q);
        if (newParent)
            newParent->installEventFilter(q);
    }

    //we also need event filter on top-level (for shortcuts)
    QWidget *newWindow = newParent ? newParent->window() : 0;

    if (oldWindow != newWindow) {
        if (oldParent && oldParent != oldWindow)
            oldWindow->removeEventFilter(q);

        if (newParent && newParent != newWindow)
            newWindow->installEventFilter(q);
    }

#ifdef Q_WS_X11
    platformMenuBar->handleReparent(oldParent, newParent, oldWindow, newWindow);
#endif

    oldParent = newParent;
    oldWindow = newWindow;

#ifdef Q_WS_MAC
    if (q->isNativeMenuBar() && !macWidgetHasNativeMenubar(newParent)) {
        // If the new parent got a native menubar from before, keep that
        // menubar rather than replace it with this one (because a parents
        // menubar has precedence over children menubars).
        macDestroyMenuBar();
        macCreateMenuBar(newParent);
    }
#endif

#ifdef Q_WS_WINCE
    if (qt_wince_is_mobile() && wce_menubar)
        wce_menubar->rebuild();
#endif
#ifdef Q_WS_S60

    // Construct symbian_menubar when this code path is entered first time
    // and when newParent != NULL
    if (!symbian_menubar)
        symbianCreateMenuBar(newParent);

    // Reparent and rebuild menubar when parent is changed
    if (symbian_menubar) {
        if (oldParent != newParent)
            reparentMenuBar(oldParent, newParent);
        q->hide();
        symbian_menubar->rebuild();
    }

#ifdef QT_SOFTKEYS_ENABLED
    // Constuct menuBarAction when this code path is entered first time
    if (!menuBarAction) {
        if (newParent) {
            menuBarAction = QSoftKeyManager::createAction(QSoftKeyManager::MenuSoftKey, newParent);
            newParent->addAction(menuBarAction);
        }
    } else {
        // If reparenting i.e. we already have menuBarAction, remove it from old parent
        // and add for a new parent
        if (oldParent)
            oldParent->removeAction(menuBarAction);
        if (newParent)
            newParent->addAction(menuBarAction);
    }
#endif // QT_SOFTKEYS_ENABLED
#endif // Q_WS_S60
}

#ifdef QT3_SUPPORT
/*!
    Sets whether the menu bar should automatically resize itself
    when its parent widget is resized.

    This feature is provided to help porting to Qt 4. We recommend
    against using it in new code.

    \sa autoGeometry()
*/
void QMenuBar::setAutoGeometry(bool b)
{
    Q_D(QMenuBar);
    d->doAutoResize = b;
}

/*!
    Returns true if the menu bar automatically resizes itself
    when its parent widget is resized; otherwise returns false.

    This feature is provided to help porting to Qt 4. We recommend
    against using it in new code.

    \sa setAutoGeometry()
*/
bool QMenuBar::autoGeometry() const
{
    Q_D(const QMenuBar);
    return d->doAutoResize;
}
#endif

/*!
  \reimp
*/
void QMenuBar::changeEvent(QEvent *e)
{
    Q_D(QMenuBar);
    if(e->type() == QEvent::StyleChange) {
        d->itemsDirty = true;
        setMouseTracking(style()->styleHint(QStyle::SH_MenuBar_MouseTracking, 0, this));
        if(parentWidget())
            resize(parentWidget()->width(), heightForWidth(parentWidget()->width()));
        d->updateGeometries();
    } else if (e->type() == QEvent::ParentChange) {
        d->handleReparent();
    } else if (e->type() == QEvent::FontChange
               || e->type() == QEvent::ApplicationFontChange) {
        d->itemsDirty = true;
        d->updateGeometries();
#ifdef QT_SOFTKEYS_ENABLED
    } else if (e->type() == QEvent::LanguageChange) {
        if (d->menuBarAction)
            d->menuBarAction->setText(QSoftKeyManager::standardSoftKeyText(QSoftKeyManager::MenuSoftKey));
#endif
    }

    QWidget::changeEvent(e);
}

/*!
  \reimp
*/
bool QMenuBar::event(QEvent *e)
{
    Q_D(QMenuBar);
    switch (e->type()) {
    case QEvent::KeyPress: {
        QKeyEvent *ke = (QKeyEvent*)e;
#if 0
        if(!d->keyboardState) { //all keypresses..
            d->setCurrentAction(0);
            return ;
        }
#endif
        if(ke->key() == Qt::Key_Tab || ke->key() == Qt::Key_Backtab) {
            keyPressEvent(ke);
            return true;
        }

    } break;
#ifndef QT_NO_SHORTCUT
    case QEvent::Shortcut: {
        QShortcutEvent *se = static_cast<QShortcutEvent *>(e);
        int shortcutId = se->shortcutId();
        for(int j = 0; j < d->shortcutIndexMap.size(); ++j) {
            if (shortcutId == d->shortcutIndexMap.value(j))
                d->_q_internalShortcutActivated(j);
        }
    } break;
#endif
    case QEvent::Show:
#ifdef QT3_SUPPORT
        if(QWidget *p = parentWidget()) {
            // If itemsDirty == true, updateGeometries sends the MenubarUpdated event.
            if (!d->itemsDirty) {
                QMenubarUpdatedEvent menubarUpdated(this);
                QApplication::sendEvent(p, &menubarUpdated);
            }
        }
#endif
        d->_q_updateLayout();
    break;
    case QEvent::ShortcutOverride: {
        QKeyEvent *kev = static_cast<QKeyEvent*>(e);
        //we only filter out escape if there is a current action
        if (kev->key() == Qt::Key_Escape && d->currentAction) {
            e->accept();
            return true;
        }
    }
    break;

#ifdef QT3_SUPPORT
    case QEvent::Hide: {
        if(QWidget *p = parentWidget()) {
            QMenubarUpdatedEvent menubarUpdated(this);
            QApplication::sendEvent(p, &menubarUpdated);
        }
    } break;
#endif

#ifndef QT_NO_WHATSTHIS
    case QEvent::QueryWhatsThis:
        e->setAccepted(d->whatsThis.size());
        if (QAction *action = d->actionAt(static_cast<QHelpEvent*>(e)->pos())) {
            if (action->whatsThis().size() || action->menu())
                e->accept();
        }
        return true;
#endif
    case QEvent::LayoutDirectionChange:
        d->_q_updateLayout();
        break;
    default:
        break;
    }
    return QWidget::event(e);
}

/*!
  \reimp
*/
bool QMenuBar::eventFilter(QObject *object, QEvent *event)
{
    Q_D(QMenuBar);
#ifdef Q_WS_X11
    if (d->platformMenuBar->menuBarEventFilter(object, event)) {
        return true;
    }
#endif
    if (object == parent() && object) {
#ifdef QT3_SUPPORT
        if (d->doAutoResize && event->type() == QEvent::Resize) {
            QResizeEvent *e = (QResizeEvent *)event;
            int w = e->size().width();
            setGeometry(0, y(), w, heightForWidth(w));
            return false;
        }
#endif
        if (event->type() == QEvent::ParentChange) //GrandparentChange
            d->handleReparent();
    }
    if (object == d->leftWidget || object == d->rightWidget) {
        switch (event->type()) {
        case QEvent::ShowToParent:
        case QEvent::HideToParent:
            d->_q_updateLayout();
            break;
        default:
            break;
        }
    }

    if (style()->styleHint(QStyle::SH_MenuBar_AltKeyNavigation, 0, this)) {
        if (d->altPressed) {
            switch (event->type()) {
            case QEvent::KeyPress:
            case QEvent::KeyRelease:
            {
                QKeyEvent *kev = static_cast<QKeyEvent*>(event);
                if (kev->key() == Qt::Key_Alt || kev->key() == Qt::Key_Meta) {
                    if (event->type() == QEvent::KeyPress) // Alt-press does not interest us, we have the shortcut-override event
                        break;
                    d->setKeyboardMode(!d->keyboardState);
                }
            }
            // fall through
            case QEvent::MouseButtonPress:
            case QEvent::MouseButtonRelease:
            case QEvent::MouseMove:
            case QEvent::FocusIn:
            case QEvent::FocusOut:
            case QEvent::ActivationChange:
                d->altPressed = false;
                qApp->removeEventFilter(this);
                break;
            default:
                break;
            }
        } else if (isVisible()) {
            if (event->type() == QEvent::ShortcutOverride) {
                QKeyEvent *kev = static_cast<QKeyEvent*>(event);
                if ((kev->key() == Qt::Key_Alt || kev->key() == Qt::Key_Meta)
                    && kev->modifiers() == Qt::AltModifier) {
                    d->altPressed = true;
                    qApp->installEventFilter(this);
                }
            }
        }
    }

    return false;
}

/*!
  Returns the QAction at \a pt. Returns 0 if there is no action at \a pt or if
the location has a separator.

    \sa addAction(), addSeparator()
*/
QAction *QMenuBar::actionAt(const QPoint &pt) const
{
    Q_D(const QMenuBar);
    return d->actionAt(pt);
}

/*!
  Returns the geometry of action \a act as a QRect.

    \sa actionAt()
*/
QRect QMenuBar::actionGeometry(QAction *act) const
{
    Q_D(const QMenuBar);
    return d->actionRect(act);
}

/*!
  \reimp
*/
QSize QMenuBar::minimumSizeHint() const
{
    Q_D(const QMenuBar);
#if defined(Q_WS_MAC) || defined(Q_WS_WINCE) || defined(Q_WS_S60) || defined(Q_WS_X11)
    const bool as_gui_menubar = !isNativeMenuBar();
#else
    const bool as_gui_menubar = true;
#endif

    ensurePolished();
    QSize ret(0, 0);
    const_cast<QMenuBarPrivate*>(d)->updateGeometries();
    const int hmargin = style()->pixelMetric(QStyle::PM_MenuBarHMargin, 0, this);
    const int vmargin = style()->pixelMetric(QStyle::PM_MenuBarVMargin, 0, this);
    int fw = style()->pixelMetric(QStyle::PM_MenuBarPanelWidth, 0, this);
    int spaceBelowMenuBar = style()->styleHint(QStyle::SH_MainWindow_SpaceBelowMenuBar, 0, this);
    if(as_gui_menubar) {
        int w = parentWidget() ? parentWidget()->width() : QApplication::desktop()->width();
        d->calcActionRects(w - (2 * fw), 0);
        for (int i = 0; ret.isNull() && i < d->actions.count(); ++i)
            ret = d->actionRects.at(i).size();
        if (!d->extension->isHidden())
            ret += QSize(d->extension->sizeHint().width(), 0);
        ret += QSize(2*fw + hmargin, 2*fw + vmargin);
    }
    int margin = 2*vmargin + 2*fw + spaceBelowMenuBar;
#ifdef Q_WS_X11
    if (d->platformMenuBar->allowCornerWidgets()) {
#endif
    if(d->leftWidget) {
        QSize sz = d->leftWidget->minimumSizeHint();
        ret.setWidth(ret.width() + sz.width());
        if(sz.height() + margin > ret.height())
            ret.setHeight(sz.height() + margin);
    }
    if(d->rightWidget) {
        QSize sz = d->rightWidget->minimumSizeHint();
        ret.setWidth(ret.width() + sz.width());
        if(sz.height() + margin > ret.height())
            ret.setHeight(sz.height() + margin);
    }
#ifdef Q_WS_X11
    }
#endif
    if(as_gui_menubar) {
        QStyleOptionMenuItem opt;
        opt.rect = rect();
        opt.menuRect = rect();
        opt.state = QStyle::State_None;
        opt.menuItemType = QStyleOptionMenuItem::Normal;
        opt.checkType = QStyleOptionMenuItem::NotCheckable;
        opt.palette = palette();
        return (style()->sizeFromContents(QStyle::CT_MenuBar, &opt,
                                         ret.expandedTo(QApplication::globalStrut()),
                                         this));
    }
    return ret;
}

/*!
  \reimp
*/
QSize QMenuBar::sizeHint() const
{
    Q_D(const QMenuBar);
#if defined(Q_WS_MAC) || defined(Q_WS_WINCE) || defined(Q_WS_S60) || defined(Q_WS_X11)
    const bool as_gui_menubar = !isNativeMenuBar();
#else
    const bool as_gui_menubar = true;
#endif


    ensurePolished();
    QSize ret(0, 0);
    const_cast<QMenuBarPrivate*>(d)->updateGeometries();
    const int hmargin = style()->pixelMetric(QStyle::PM_MenuBarHMargin, 0, this);
    const int vmargin = style()->pixelMetric(QStyle::PM_MenuBarVMargin, 0, this);
    int fw = style()->pixelMetric(QStyle::PM_MenuBarPanelWidth, 0, this);
    int spaceBelowMenuBar = style()->styleHint(QStyle::SH_MainWindow_SpaceBelowMenuBar, 0, this);
    if(as_gui_menubar) {
        const int w = parentWidget() ? parentWidget()->width() : QApplication::desktop()->width();
        d->calcActionRects(w - (2 * fw), 0);
        for (int i = 0; i < d->actionRects.count(); ++i) {
            const QRect &actionRect = d->actionRects.at(i);
            ret = ret.expandedTo(QSize(actionRect.x() + actionRect.width(), actionRect.y() + actionRect.height()));
        }
        //the action geometries already contain the top and left
        //margins. So we only need to add those from right and bottom.
        ret += QSize(fw + hmargin, fw + vmargin);
    }
    int margin = 2*vmargin + 2*fw + spaceBelowMenuBar;
#ifdef Q_WS_X11
    if(d->platformMenuBar->allowCornerWidgets()) {
#endif
    if(d->leftWidget) {
        QSize sz = d->leftWidget->sizeHint();
        ret.setWidth(ret.width() + sz.width());
        if(sz.height() + margin > ret.height())
            ret.setHeight(sz.height() + margin);
    }
    if(d->rightWidget) {
        QSize sz = d->rightWidget->sizeHint();
        ret.setWidth(ret.width() + sz.width());
        if(sz.height() + margin > ret.height())
            ret.setHeight(sz.height() + margin);
    }
#ifdef Q_WS_X11
    }
#endif
    if(as_gui_menubar) {
        QStyleOptionMenuItem opt;
        opt.rect = rect();
        opt.menuRect = rect();
        opt.state = QStyle::State_None;
        opt.menuItemType = QStyleOptionMenuItem::Normal;
        opt.checkType = QStyleOptionMenuItem::NotCheckable;
        opt.palette = palette();
        return (style()->sizeFromContents(QStyle::CT_MenuBar, &opt,
                                         ret.expandedTo(QApplication::globalStrut()),
                                         this));
    }
    return ret;
}

/*!
  \reimp
*/
int QMenuBar::heightForWidth(int) const
{
    Q_D(const QMenuBar);
#if defined(Q_WS_MAC) || defined(Q_WS_WINCE) || defined(Q_WS_S60) || defined(Q_WS_X11)
    const bool as_gui_menubar = !isNativeMenuBar();
#else
    const bool as_gui_menubar = true;
#endif

    const_cast<QMenuBarPrivate*>(d)->updateGeometries();
    int height = 0;
    const int vmargin = style()->pixelMetric(QStyle::PM_MenuBarVMargin, 0, this);
    int fw = style()->pixelMetric(QStyle::PM_MenuBarPanelWidth, 0, this);
    int spaceBelowMenuBar = style()->styleHint(QStyle::SH_MainWindow_SpaceBelowMenuBar, 0, this);
    if(as_gui_menubar) {
        for (int i = 0; i < d->actionRects.count(); ++i)
            height = qMax(height, d->actionRects.at(i).height());
        if (height) //there is at least one non-null item
            height += spaceBelowMenuBar;
        height += 2*fw;
        height += 2*vmargin;
    }
    int margin = 2*vmargin + 2*fw + spaceBelowMenuBar;
#ifdef Q_WS_X11
    if(d->platformMenuBar->allowCornerWidgets()) {
#endif
    if(d->leftWidget)
        height = qMax(d->leftWidget->sizeHint().height() + margin, height);
    if(d->rightWidget)
        height = qMax(d->rightWidget->sizeHint().height() + margin, height);
#ifdef Q_WS_X11
    }
#endif
    if(as_gui_menubar) {
        QStyleOptionMenuItem opt;
        opt.init(this);
        opt.menuRect = rect();
        opt.state = QStyle::State_None;
        opt.menuItemType = QStyleOptionMenuItem::Normal;
        opt.checkType = QStyleOptionMenuItem::NotCheckable;
        return style()->sizeFromContents(QStyle::CT_MenuBar, &opt, QSize(0, height), this).height(); //not pretty..
    }
    return height;
}

/*!
  \internal
*/
void QMenuBarPrivate::_q_internalShortcutActivated(int id)
{
    Q_Q(QMenuBar);
    QAction *act = actions.at(id);
#ifdef Q_WS_X11
    if (q->isNativeMenuBar()) {
        platformMenuBar->popupAction(act);
    } else {
#endif
        setCurrentAction(act, true, true);
#ifdef Q_WS_X11
    }
#endif
    if (act && !act->menu()) {
        activateAction(act, QAction::Trigger);
        //100 is the same as the default value in QPushButton::animateClick
        autoReleaseTimer.start(100, q);
    } else if (act && q->style()->styleHint(QStyle::SH_MenuBar_AltKeyNavigation, 0, q)) {
        // When we open a menu using a shortcut, we should end up in keyboard state
        setKeyboardMode(true);
    }
}

void QMenuBarPrivate::_q_updateLayout()
{
    Q_Q(QMenuBar);
    itemsDirty = true;
    if (q->isVisible()) {
        updateGeometries();
        q->update();
    }
}

#ifdef Q_WS_X11
void QMenuBarPrivate::updateCornerWidgetToolBar()
{
    Q_Q(QMenuBar);
    if (!cornerWidgetToolBar) {
        QMainWindow *window = qobject_cast<QMainWindow *>(q->window());
        if (!window) {
            qWarning() << "Menubar parent is not a QMainWindow, not showing corner widgets";
            return;
        }
        cornerWidgetToolBar = window->addToolBar(QApplication::translate("QMenuBar", "Corner Toolbar"));
        cornerWidgetToolBar->setObjectName(QLatin1String("CornerToolBar"));
        cornerWidgetContainer = new QWidget;
        cornerWidgetToolBar->addWidget(cornerWidgetContainer);
        new QHBoxLayout(cornerWidgetContainer);
    } else {
        QLayout *layout = cornerWidgetContainer->layout();
        while (layout->count() > 0) {
            layout->takeAt(0);
        }
    }
    if (leftWidget) {
        leftWidget->setParent(cornerWidgetContainer);
        cornerWidgetContainer->layout()->addWidget(leftWidget);
    }
    if (rightWidget) {
        rightWidget->setParent(cornerWidgetContainer);
        cornerWidgetContainer->layout()->addWidget(rightWidget);
    }
}
#endif


/*!
    \fn void QMenuBar::setCornerWidget(QWidget *widget, Qt::Corner corner)

    This sets the given \a widget to be shown directly on the left of the first
    menu item, or on the right of the last menu item, depending on \a corner.

    The menu bar takes ownership of \a widget, reparenting it into the menu bar.
    However, if the \a corner already contains a widget, this previous widget
    will no longer be managed and will still be a visible child of the menu bar.

   \note Using a corner other than Qt::TopRightCorner or Qt::TopLeftCorner
    will result in a warning.
*/
void QMenuBar::setCornerWidget(QWidget *w, Qt::Corner corner)
{
    Q_D(QMenuBar);
    switch (corner) {
    case Qt::TopLeftCorner:
        if (d->leftWidget)
            d->leftWidget->removeEventFilter(this);
        d->leftWidget = w;
        break;
    case Qt::TopRightCorner:
        if (d->rightWidget)
            d->rightWidget->removeEventFilter(this);
        d->rightWidget = w;
        break;
    default:
        qWarning("QMenuBar::setCornerWidget: Only TopLeftCorner and TopRightCorner are supported");
        return;
    }

#ifdef Q_WS_X11
    if(!d->platformMenuBar->allowCornerWidgets()) {
        d->updateCornerWidgetToolBar();
    } else {
#endif
    if (w) {
        w->setParent(this);
        w->installEventFilter(this);
    }
#ifdef Q_WS_X11
    }
#endif

    d->_q_updateLayout();
}

/*!
    Returns the widget on the left of the first or on the right of the last menu
    item, depending on \a corner.

   \note Using a corner other than Qt::TopRightCorner or Qt::TopLeftCorner
    will result in a warning.
*/
QWidget *QMenuBar::cornerWidget(Qt::Corner corner) const
{
    Q_D(const QMenuBar);
    QWidget *w = 0;
    switch(corner) {
    case Qt::TopLeftCorner:
        w = d->leftWidget;
        break;
    case Qt::TopRightCorner:
        w = d->rightWidget;
        break;
    default:
        qWarning("QMenuBar::cornerWidget: Only TopLeftCorner and TopRightCorner are supported");
        break;
    }

    return w;
}

/*!
    \property QMenuBar::nativeMenuBar
    \brief Whether or not a menubar will be used as a native menubar on platforms that support it
    \since 4.6

    This property specifies whether or not the menubar should be used as a native menubar on platforms
    that support it. The currently supported platforms are Mac OS X and Windows CE. On these platforms
    if this property is true, the menubar is used in the native menubar and is not in the window of
    its parent, if false the menubar remains in the window. On other platforms the value of this
    attribute has no effect.

    The default is to follow whether the Qt::AA_DontUseNativeMenuBar attribute
    is set for the application. Explicitly settings this property overrides
    the presence (or abscence) of the attribute.

    \sa void-qt-mac-set-native-menubar-bool-enable
*/

void QMenuBar::setNativeMenuBar(bool nativeMenuBar)
{
    Q_D(QMenuBar);
#ifdef Q_WS_X11
    d->platformMenuBar->setNativeMenuBar(nativeMenuBar);
#else
    if (d->nativeMenuBar == -1 || (nativeMenuBar != bool(d->nativeMenuBar))) {
        d->nativeMenuBar = nativeMenuBar;
#ifdef Q_WS_MAC
        if (!d->nativeMenuBar) {
            extern void qt_mac_clear_menubar();
            qt_mac_clear_menubar();
            d->macDestroyMenuBar();
            const QList<QAction *> &menubarActions = actions();
            for (int i = 0; i < menubarActions.size(); ++i) {
                const QAction *action = menubarActions.at(i);
                if (QMenu *menu = action->menu()) {
                    delete menu->d_func()->mac_menu;
                    menu->d_func()->mac_menu = 0;
                }
            }
        } else {
            d->macCreateMenuBar(parentWidget());
        }
        macUpdateMenuBar();
	updateGeometry();
	if (!d->nativeMenuBar && parentWidget())
	    setVisible(true);
#endif
    }
#endif // Q_WS_X11
}

bool QMenuBar::isNativeMenuBar() const
{
    Q_D(const QMenuBar);
#ifdef Q_WS_X11
    return d->platformMenuBar->isNativeMenuBar();
#else
    if (d->nativeMenuBar == -1) {
        return !QApplication::instance()->testAttribute(Qt::AA_DontUseNativeMenuBar);
    }
    return d->nativeMenuBar;
#endif
}

/*!
  \since 4.4

  Sets the default action to \a act.

  The default action is assigned to the left soft key. The menu is assigned
  to the right soft key.

  Currently there is only support for the default action on Windows
  Mobile. On all other platforms this method is not available.

  \sa defaultAction()
*/

#ifdef Q_WS_WINCE
void QMenuBar::setDefaultAction(QAction *act)
{
    Q_D(QMenuBar);
    if (d->defaultAction == act)
        return;
#ifdef Q_WS_WINCE
    if (qt_wince_is_mobile())
        if (d->defaultAction) {
            disconnect(d->defaultAction, SIGNAL(changed()), this, SLOT(_q_updateDefaultAction()));
            disconnect(d->defaultAction, SIGNAL(destroyed()), this, SLOT(_q_updateDefaultAction()));
        }
#endif
    d->defaultAction = act;
#ifdef Q_WS_WINCE
    if (qt_wince_is_mobile())
        if (d->defaultAction) {
            connect(d->defaultAction, SIGNAL(changed()), this, SLOT(_q_updateDefaultAction()));
            connect(d->defaultAction, SIGNAL(destroyed()), this, SLOT(_q_updateDefaultAction()));
        }
    if (d->wce_menubar) {
        d->wce_menubar->rebuild();
    }
#endif
}

/*!
  \since 4.4

  Returns the current default action.

  \sa setDefaultAction()
*/
QAction *QMenuBar::defaultAction() const
{
    return d_func()->defaultAction;
}
#endif

/*!
    \fn void QMenuBar::triggered(QAction *action)

    This signal is emitted when an action in a menu belonging to this menubar
    is triggered as a result of a mouse click; \a action is the action that
    caused the signal to be emitted.

    \note QMenuBar has to have ownership of the QMenu in order this signal to work.

    Normally, you connect each menu action to a single slot using
    QAction::triggered(), but sometimes you will want to connect
    several items to a single slot (most often if the user selects
    from an array). This signal is useful in such cases.

    \sa hovered(), QAction::triggered()
*/

/*!
    \fn void QMenuBar::hovered(QAction *action)

    This signal is emitted when a menu action is highlighted; \a action
    is the action that caused the event to be sent.

    Often this is used to update status information.

    \sa triggered(), QAction::hovered()
*/


#ifdef QT3_SUPPORT
/*!
    Use style()->pixelMetric(QStyle::PM_MenuBarPanelWidth, this)
    instead.
*/
int QMenuBar::frameWidth() const
{
    return style()->pixelMetric(QStyle::PM_MenuBarPanelWidth, 0, this);
}

int QMenuBar::insertAny(const QIcon *icon, const QString *text, const QObject *receiver, const char *member,
                        const QKeySequence *shortcut, const QMenu *popup, int id, int index)
{
    QAction *act = popup ? popup->menuAction() : new QAction(this);
    if(id != -1)
        static_cast<QMenuItem*>(act)->setId(id);
    if(icon)
        act->setIcon(*icon);
    if(text)
        act->setText(*text);
    if(shortcut)
        act->setShortcut(*shortcut);
    if(receiver && member)
        QObject::connect(act, SIGNAL(triggered(bool)), receiver, member);
    if(index == -1 || index >= actions().count())
        addAction(act);
    else
        insertAction(actions().value(index), act);
    return findIdForAction(act);
}

/*!
    \since 4.2

    Use addSeparator() or insertAction() instead.

    \oldcode
        menuBar->insertSeparator();
    \newcode
        menuBar->addSeparator();
    \endcode
*/
int QMenuBar::insertSeparator(int index)
{
    QAction *act = new QAction(this);
    act->setSeparator(true);
    if(index == -1 || index >= actions().count())
        addAction(act);
    else
        insertAction(actions().value(index), act);
    return findIdForAction(act);
}

/*!
    Use QAction::setData() instead.
*/
bool QMenuBar::setItemParameter(int id, int param)
{
    if(QAction *act = findActionForId(id)) {
        act->d_func()->param = param;
        return true;
    }
    return false;
}

/*!
    Use QAction::data() instead.
*/
int QMenuBar::itemParameter(int id) const
{
    if(QAction *act = findActionForId(id))
        return act->d_func()->param;
    return id;
}

QAction *QMenuBar::findActionForId(int id) const
{
    QList<QAction *> list = actions();
    for (int i = 0; i < list.size(); ++i) {
        QAction *act = list.at(i);
        if (findIdForAction(act) == id)
            return act;
    }
    return 0;
}

int QMenuBar::findIdForAction(QAction *act) const
{
    Q_ASSERT(act);
    return act->d_func()->id;
}
#endif

/*!
    \enum QMenuBar::Separator

    \compat

    \value Never
    \value InWindowsStyle

*/

/*!
    \fn void QMenuBar::addAction(QAction *action)
    \overload

    Appends the action \a action to the menu bar's list of actions.

    \sa QMenu::addAction(), QWidget::addAction(), QWidget::actions()
*/

/*!
    \fn uint QMenuBar::count() const

    Use actions().count() instead.
*/

/*!
    \fn int QMenuBar::insertItem(const QString &text, const QObject *receiver, const char* member, const QKeySequence& shortcut, int id, int index)

    Use one of the insertAction() or addAction() overloads instead.
*/

/*!
    \fn int QMenuBar::insertItem(const QIcon& icon, const QString &text, const QObject *receiver, const char* member, const QKeySequence& shortcut, int id, int index)

    Use one of the insertAction() or addAction() overloads instead.
*/

/*!
    \fn int QMenuBar::insertItem(const QPixmap &pixmap, const QObject *receiver, const char* member, const QKeySequence& shortcut, int id, int index)

    Use one of the insertAction(), addAction(), insertMenu(), or
    addMenu() overloads instead.
*/

/*!
    \fn int QMenuBar::insertItem(const QString &text, int id, int index)

    Use one of the insertAction() or addAction() overloads instead.
*/

/*!
    \fn int QMenuBar::insertItem(const QIcon& icon, const QString &text, int id, int index)

    Use one of the insertAction(), addAction(), insertMenu(), or
    addMenu() overloads instead.
*/

/*!
    \fn int QMenuBar::insertItem(const QString &text, QMenu *popup, int id, int index)

    Use one of the insertMenu(), or addMenu() overloads instead.
*/

/*!
    \fn int QMenuBar::insertItem(const QIcon& icon, const QString &text, QMenu *popup, int id, int index)

    Use one of the insertMenu(), or addMenu() overloads instead.
*/

/*!
    \fn int QMenuBar::insertItem(const QPixmap &pixmap, int id, int index)

    Use one of the insertAction(), addAction(), insertMenu(), or
    addMenu() overloads instead.
*/

/*!
    \fn int QMenuBar::insertItem(const QPixmap &pixmap, QMenu *popup, int id, int index)

    Use one of the insertMenu(), or addMenu() overloads instead.
*/

/*!
    \fn void QMenuBar::removeItem(int id)

    Use removeAction() instead.
*/

/*!
    \fn void QMenuBar::removeItemAt(int index)

    Use removeAction() instead.
*/

/*!
    \fn QKeySequence QMenuBar::accel(int id) const

    Use shortcut() on the relevant QAction instead.
*/

/*!
    \fn void QMenuBar::setAccel(const QKeySequence& key, int id)

    Use setShortcut() on the relevant QAction instead.
*/

/*!
    \fn QIcon QMenuBar::iconSet(int id) const

    Use icon() on the relevant QAction instead.
*/

/*!
    \fn QString QMenuBar::text(int id) const

    Use text() on the relevant QAction instead.
*/

/*!
    \fn QPixmap QMenuBar::pixmap(int id) const

    Use QPixmap(icon()) on the relevant QAction instead.
*/

/*!
    \fn void QMenuBar::setWhatsThis(int id, const QString &w)

    Use setWhatsThis() on the relevant QAction instead.
*/

/*!
    \fn QString QMenuBar::whatsThis(int id) const

    Use whatsThis() on the relevant QAction instead.
*/

/*!
    \fn void QMenuBar::changeItem(int id, const QString &text)

    Use setText() on the relevant QAction instead.
*/

/*!
    \fn void QMenuBar::changeItem(int id, const QPixmap &pixmap)

    Use setText() on the relevant QAction instead.
*/

/*!
    \fn void QMenuBar::changeItem(int id, const QIcon &icon, const QString &text)

    Use setIcon() and setText() on the relevant QAction instead.
*/

/*!
    \fn bool QMenuBar::isItemActive(int id) const

    Use activeAction() instead.
*/

/*!
    \fn bool QMenuBar::isItemEnabled(int id) const

    Use isEnabled() on the relevant QAction instead.
*/

/*!
    \fn void QMenuBar::setItemEnabled(int id, bool enable)

    Use setEnabled() on the relevant QAction instead.
*/

/*!
    \fn bool QMenuBar::isItemChecked(int id) const

    Use isChecked() on the relevant QAction instead.
*/

/*!
    \fn void QMenuBar::setItemChecked(int id, bool check)

    Use setChecked() on the relevant QAction instead.
*/

/*!
    \fn bool QMenuBar::isItemVisible(int id) const

    Use isVisible() on the relevant QAction instead.
*/

/*!
    \fn void QMenuBar::setItemVisible(int id, bool visible)

    Use setVisible() on the relevant QAction instead.
*/

/*!
    \fn int QMenuBar::indexOf(int id) const

    Use actions().indexOf(action) on the relevant QAction instead.
*/

/*!
    \fn int QMenuBar::idAt(int index) const

    Use actions instead.
*/

/*!
    \fn void QMenuBar::activateItemAt(int index)

    Use activate() on the relevant QAction instead.
*/

/*!
    \fn bool QMenuBar::connectItem(int id, const QObject *receiver, const char* member)

    Use connect() on the relevant QAction instead.
*/

/*!
    \fn bool QMenuBar::disconnectItem(int id,const QObject *receiver, const char* member)

    Use disconnect() on the relevant QAction instead.
*/

/*!
    \fn QMenuItem *QMenuBar::findItem(int id) const

    Use actions instead.
*/

/*!
    \fn Separator QMenuBar::separator() const

    This function is provided only to make old code compile.
*/

/*!
    \fn void QMenuBar::setSeparator(Separator sep)

    This function is provided only to make old code compile.
*/

/*!
    \fn QRect QMenuBar::itemRect(int index)

    Use actionGeometry() on the relevant QAction instead.
*/

/*!
    \fn int QMenuBar::itemAtPos(const QPoint &p)

    There is no equivalent way to achieve this in Qt 4.
*/

/*!
    \fn void QMenuBar::activated(int itemId);

    Use triggered() instead.
*/

/*!
    \fn void QMenuBar::highlighted(int itemId);

    Use hovered() instead.
*/

/*!
    \fn void QMenuBar::setFrameRect(QRect)
    \internal
*/

/*!
    \fn QRect QMenuBar::frameRect() const
    \internal
*/
/*!
    \enum QMenuBar::DummyFrame
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
    \fn void QMenuBar::setFrameShadow(DummyFrame)
    \internal
*/

/*!
    \fn DummyFrame QMenuBar::frameShadow() const
    \internal
*/

/*!
    \fn void QMenuBar::setFrameShape(DummyFrame)
    \internal
*/

/*!
    \fn DummyFrame QMenuBar::frameShape() const
    \internal
*/

/*!
    \fn void QMenuBar::setFrameStyle(int)
    \internal
*/

/*!
    \fn int QMenuBar::frameStyle() const
    \internal
*/

/*!
    \fn void QMenuBar::setLineWidth(int)
    \internal
*/

/*!
    \fn int QMenuBar::lineWidth() const
    \internal
*/

/*!
    \fn void QMenuBar::setMargin(int margin)
    Sets the width of the margin around the contents of the widget to \a margin.

    Use QWidget::setContentsMargins() instead.
    \sa margin(), QWidget::setContentsMargins()
*/

/*!
    \fn int QMenuBar::margin() const
    Returns the width of the margin around the contents of the widget.

    Use QWidget::getContentsMargins() instead.
    \sa setMargin(), QWidget::getContentsMargins()
*/

/*!
    \fn void QMenuBar::setMidLineWidth(int)
    \internal
*/

/*!
    \fn int QMenuBar::midLineWidth() const
    \internal
*/

// for private slots


QT_END_NAMESPACE

#include <moc_qmenubar.cpp>

#endif // QT_NO_MENUBAR
