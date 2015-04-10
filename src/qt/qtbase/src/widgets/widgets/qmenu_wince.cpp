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

//Native menubars are only supported for Windows Mobile not the standard SDK/generic WinCE
#ifdef Q_OS_WINCE
#include "qmenu.h"
#include "qt_windows.h"
#include "qapplication.h"
#include "qmainwindow.h"
#include "qtoolbar.h"
#include "qevent.h"
#include "qstyle.h"
#include "qdebug.h"
#include "qwidgetaction.h"
#include <private/qapplication_p.h>
#include <private/qmenu_p.h>
#include <private/qmenubar_p.h>

#include "qmenu_wince_resource_p.h"

#include <QtCore/qlibrary.h>
#include <commctrl.h>
#if Q_OS_WINCE_WM
#   include <windowsm.h>
#endif

#ifndef QT_NO_MENUBAR

#ifndef SHCMBF_EMPTYBAR
#define SHCMBF_EMPTYBAR 0x0001
#endif

#ifndef SHCMBM_GETSUBMENU
#define SHCMBM_GETSUBMENU (WM_USER + 401)
#endif

#ifdef Q_OS_WINCE_WM
#   define SHMBOF_NODEFAULT 0x00000001
#   define SHMBOF_NOTIFY    0x00000002
#   define SHCMBM_OVERRIDEKEY (WM_USER + 0x193)
#endif

extern bool qt_wince_is_smartphone();//defined in qguifunctions_wce.cpp
extern bool qt_wince_is_pocket_pc(); //defined in qguifunctions_wce.cpp

QT_BEGIN_NAMESPACE

static uint qt_wce_menu_static_cmd_id = 200;
static QList<QMenuBar*> nativeMenuBars;

struct qt_SHMENUBARINFO
{
    DWORD     cbSize;
    HWND      hwndParent;
    DWORD     dwFlags;
    UINT      nToolBarId;
    HINSTANCE hInstRes;
    int       nBmpId;
    int       cBmpImages;
    HWND      hwndMB;
    COLORREF  clrBk;
};

typedef BOOL (WINAPI *AygCreateMenuBar)(qt_SHMENUBARINFO*);
typedef HRESULT (WINAPI *AygEnableSoftKey)(HWND,UINT,BOOL,BOOL);

static bool aygResolved = false;
static AygCreateMenuBar ptrCreateMenuBar = 0;
static AygEnableSoftKey ptrEnableSoftKey = 0;

static void resolveAygLibs()
{
    if (!aygResolved) {
        aygResolved = true;
        QLibrary aygLib(QLatin1String("aygshell"));
        ptrCreateMenuBar = (AygCreateMenuBar) aygLib.resolve("SHCreateMenuBar");
        ptrEnableSoftKey = (AygEnableSoftKey) aygLib.resolve("SHEnableSoftkey");
    }
}

static void qt_wce_enable_soft_key(HWND handle, uint command)
{
    resolveAygLibs();
    if (ptrEnableSoftKey)
        ptrEnableSoftKey(handle, command, false, true);
}

static void qt_wce_disable_soft_key(HWND handle, uint command)
{
    resolveAygLibs();
    if (ptrEnableSoftKey)
        ptrEnableSoftKey(handle, command, false, false);
}

static void qt_wce_delete_action_list(QList<QWceMenuAction*> *list)
{
    for(QList<QWceMenuAction*>::Iterator it = list->begin(); it != list->end(); ++it) {
        QWceMenuAction *action = (*it);
        delete action;
        action = 0;
    }
    list->clear();
}

//search for first QuitRole in QMenuBar
static QAction* qt_wce_get_quit_action(QList<QAction *> actionItems)
{
    QAction *returnAction = 0;
    for (int i = 0; i < actionItems.size(); ++i) {
        QAction *action = actionItems.at(i);
        if (action->menuRole() == QAction::QuitRole)
            returnAction = action;
        else
            if (action->menu())
                returnAction = qt_wce_get_quit_action(action->menu()->actions());
        if (returnAction)
            return returnAction; //return first action found
    }
    return 0; //nothing found;
}

static QAction* qt_wce_get_quit_action(QList<QWceMenuAction*> actionItems)
{
    for (int i = 0; i < actionItems.size(); ++i) {
        if (actionItems.at(i)->action->menuRole() == QAction::QuitRole)
            return actionItems.at(i)->action;
        else if (actionItems.at(i)->action->menu()) {
            QAction *returnAction = qt_wce_get_quit_action(actionItems.at(i)->action->menu()->actions());
            if (returnAction)
                return returnAction;
        }
    }
    return 0;
}

static HMODULE qt_wce_get_module_handle()
{
    HMODULE module =  0; //handle to resources
    if (!(module = GetModuleHandle(L"QtGui4"))) //release dynamic
        if (!(module = GetModuleHandle(L"QtGuid4"))) //debug dynamic
            module = (HINSTANCE)qWinAppInst(); //static
    Q_ASSERT_X(module, "qt_wce_get_module_handle()", "cannot get handle to module?");
    return module;
}

static void qt_wce_change_command(HWND menuHandle, int item, int command)
{
TBBUTTONINFOA tbbi;
    memset(&tbbi,0,sizeof(tbbi));
    tbbi.cbSize = sizeof(tbbi);
    tbbi.dwMask = TBIF_COMMAND;
    tbbi.idCommand = command;
    SendMessage(menuHandle, TB_SETBUTTONINFO, item, (LPARAM)&tbbi);
}

static void qt_wce_rename_menu_item(HWND menuHandle, int item, const QString &newText)
{
    TBBUTTONINFOA tbbi;
    memset(&tbbi,0,sizeof(tbbi));
    tbbi.cbSize = sizeof(tbbi);
    tbbi.dwMask = TBIF_TEXT;
    QString text = newText;
    text.remove(QChar::fromLatin1('&'));
    tbbi.pszText = (LPSTR) text.utf16();
    SendMessage(menuHandle, TB_SETBUTTONINFO, item, (LPARAM)&tbbi);
}

static HWND qt_wce_create_menubar(HWND parentHandle, HINSTANCE resourceHandle, int toolbarID, int flags = 0)
{
    resolveAygLibs();

    if (ptrCreateMenuBar) {
        qt_SHMENUBARINFO mbi;
        memset(&mbi, 0, sizeof(qt_SHMENUBARINFO));
        mbi.cbSize     = sizeof(qt_SHMENUBARINFO);
        mbi.hwndParent = parentHandle;
        mbi.hInstRes   = resourceHandle;
        mbi.dwFlags = flags;
        mbi.nToolBarId = toolbarID;

        if (ptrCreateMenuBar(&mbi)) {
#ifdef Q_OS_WINCE_WM
            // Tell the menu bar that we want to override hot key behaviour.
            LPARAM lparam = MAKELPARAM(SHMBOF_NODEFAULT | SHMBOF_NOTIFY,
                                       SHMBOF_NODEFAULT | SHMBOF_NOTIFY);
            SendMessage(mbi.hwndMB, SHCMBM_OVERRIDEKEY, VK_TBACK, lparam);
#endif
            return mbi.hwndMB;
        }
    }
    return 0;
}

static void qt_wce_insert_action(HMENU menu, QWceMenuAction *action)
{
    Q_ASSERT_X(menu, "AppendMenu", "menu is 0");
    if (action->action->isVisible()) {
        int flags;
        action->action->isEnabled() ? flags = MF_ENABLED : flags = MF_GRAYED;

        QString text = action->action->iconText();
        text.remove(QChar::fromLatin1('&'));
        if (action->action->isSeparator()) {
            AppendMenu (menu, MF_SEPARATOR , 0, 0);
        }
        else if (action->action->menu()) {
            text.remove(QChar::fromLatin1('&'));
            AppendMenu (menu, MF_STRING | flags | MF_POPUP,
            (UINT) action->action->menu()->wceMenu(), reinterpret_cast<const wchar_t *> (text.utf16()));
        }
        else {
            AppendMenu (menu, MF_STRING | flags, action->command, reinterpret_cast<const wchar_t *> (text.utf16()));
        }
        if (action->action->isCheckable())
            if (action->action->isChecked())
                CheckMenuItem(menu, action->command, MF_BYCOMMAND | MF_CHECKED);
            else
                CheckMenuItem(menu, action->command, MF_BYCOMMAND | MF_UNCHECKED);
    }
}

// Removes all items from the menu without destroying the handles.
static void qt_wce_clear_menu(HMENU hMenu)
{
    while (RemoveMenu(hMenu, 0, MF_BYPOSITION));
}

/*!
    \internal

    This function refreshes the native Windows CE menu.
*/

void QMenuBar::wceRefresh()
{
    for (int i = 0; i < nativeMenuBars.size(); ++i)
        nativeMenuBars.at(i)->d_func()->wceRefresh();
}

void QMenuBarPrivate::wceRefresh()
{
    DrawMenuBar(wce_menubar->menubarHandle);
}

/*!
    \internal

    This function sends native Windows CE commands to Qt menus.
*/

QAction* QMenu::wceCommands(uint command)
{
    Q_D(QMenu);
    return d->wceCommands(command);
}

/*!
    \internal

    This function sends native Windows CE commands to Qt menu bars
    and all their child menus.
*/

void QMenuBar::wceCommands(uint command)
{
    const HWND hwndActiveWindow = GetActiveWindow();
    for (int i = 0; i < nativeMenuBars.size(); ++i) {
        QMenuBarPrivate* nativeMenuBar = nativeMenuBars.at(i)->d_func();
        if (hwndActiveWindow == nativeMenuBar->wce_menubar->parentWindowHandle)
            nativeMenuBar->wceCommands(command);
    }
}

bool QMenuBarPrivate::wceEmitSignals(QList<QWceMenuAction*> actions, uint command)
{
    QAction *foundAction = 0;
    for (int i = 0; i < actions.size(); ++i) {
        QWceMenuAction *action = actions.at(i);
        if (action->action->menu()) {
            foundAction = action->action->menu()->wceCommands(command);
            if (foundAction)
                break;
        }
        else if (action->command == command) {
            action->action->activate(QAction::Trigger);
            return true;
        }
    }
    if (foundAction) {
        emit q_func()->triggered(foundAction);
        return true;
    }
    return false;
}

void QMenuBarPrivate::wceCommands(uint command)
{
    if (wceClassicMenu) {
        for (int i = 0; i < wce_menubar->actionItemsClassic.size(); ++i)
            wceEmitSignals(wce_menubar->actionItemsClassic.at(i), command);
    } else {
        if (wceEmitSignals(wce_menubar->actionItems, command)) {
            return;
        }
        else if (wce_menubar->leftButtonIsMenu) {//check if command is on the left quick button
            wceEmitSignals(wce_menubar->actionItemsLeftButton, command);
        }
        else if ((wce_menubar->leftButtonAction) && (command == wce_menubar->leftButtonCommand)) {
            emit q_func()->triggered(wce_menubar->leftButtonAction);
            wce_menubar->leftButtonAction->activate(QAction::Trigger);
        }
    }
}

QAction *QMenuPrivate::wceCommands(uint command)
{
    QAction *foundAction = 0;
    for (int i  = 0; i < wce_menu->actionItems.size(); ++i) {
        if (foundAction)
            break;
        QWceMenuAction *action = wce_menu->actionItems.at(i);
        if (action->action->menu()) {
            foundAction = action->action->menu()->d_func()->wceCommands(command);
        }
        else if (action->command == command) {
            activateAction(action->action, QAction::Trigger);
            return action->action;
        }
    }
    if (foundAction)
        emit q_func()->triggered(foundAction);
    return foundAction;
}

void QMenuBarPrivate::wceCreateMenuBar(QWidget *parent)
{
    Q_Q(QMenuBar);
    wce_menubar = new QWceMenuBarPrivate(this);

    wce_menubar->parentWindowHandle = parent ? QApplicationPrivate::getHWNDForWidget(parent) :
                                               QApplicationPrivate::getHWNDForWidget(q);
    wce_menubar->leftButtonAction = defaultAction;

    wce_menubar->menubarHandle = qt_wce_create_menubar(wce_menubar->parentWindowHandle, (HINSTANCE)qWinAppInst(), 0, SHCMBF_EMPTYBAR);
    Q_ASSERT_X(wce_menubar->menubarHandle, "wceCreateMenuBar", "cannot create empty menu bar");
    DrawMenuBar(wce_menubar->menubarHandle);
    nativeMenuBars.append(q);
    wceClassicMenu = (!qt_wince_is_smartphone() && !qt_wince_is_pocket_pc());
}

void QMenuBarPrivate::wceDestroyMenuBar()
{
    Q_Q(QMenuBar);
    int index = nativeMenuBars.indexOf(q);
    nativeMenuBars.removeAt(index);
    if (wce_menubar) {
        delete wce_menubar;
        wce_menubar = 0;
    }
}

QMenuBarPrivate::QWceMenuBarPrivate::QWceMenuBarPrivate(QMenuBarPrivate *menubar)
:   menubarHandle(0), menuHandle(0), leftButtonMenuHandle(0),
    leftButtonAction(0), leftButtonIsMenu(false), d(menubar)
{
}

QMenuBarPrivate::QWceMenuBarPrivate::~QWceMenuBarPrivate()
{
    if (menubarHandle)
        DestroyWindow(menubarHandle);
    qt_wce_delete_action_list(&actionItems);
    qt_wce_delete_action_list(&actionItemsLeftButton);

    for (int i=0; i<actionItemsClassic.size(); ++i)
        if (!actionItemsClassic.value(i).empty())
            qt_wce_delete_action_list(&actionItemsClassic[i]);
    actionItemsClassic.clear();

    menubarHandle = 0;
    menuHandle = 0;
    leftButtonMenuHandle = 0;
    leftButtonCommand = 0;
    QMenuBar::wceRefresh();
}

QMenuPrivate::QWceMenuPrivate::QWceMenuPrivate()
:   menuHandle(0)
{
}

QMenuPrivate::QWceMenuPrivate::~QWceMenuPrivate()
{
    qt_wce_delete_action_list(&actionItems);
    if (menuHandle)
        DestroyMenu(menuHandle);
}

void QMenuPrivate::QWceMenuPrivate::addAction(QAction *a, QWceMenuAction *before)
{
    QWceMenuAction *action = new QWceMenuAction;
    action->action = a;
    action->command = qt_wce_menu_static_cmd_id++;
    addAction(action, before);
}

void QMenuPrivate::QWceMenuPrivate::addAction(QWceMenuAction *action, QWceMenuAction *before)
{
    if (!action)
        return;
    int before_index = actionItems.indexOf(before);
    if (before_index < 0) {
        before = 0;
        before_index = actionItems.size();
    }
    actionItems.insert(before_index, action);
    rebuild();
}

/*!
    \internal

    This function will return the HMENU used to create the native
    Windows CE menu bar bindings.
*/

HMENU QMenu::wceMenu()
{
    return d_func()->wceMenu();
}

HMENU QMenuPrivate::wceMenu()
{
    if (!wce_menu)
        wce_menu = new QWceMenuPrivate;
    if (!wce_menu->menuHandle)
        wce_menu->rebuild();
    return wce_menu->menuHandle;
}

void QMenuPrivate::QWceMenuPrivate::rebuild()
{
    if (!menuHandle)
        menuHandle = CreatePopupMenu();
    else
        qt_wce_clear_menu(menuHandle);

    for (int i = 0; i < actionItems.size(); ++i) {
        QWceMenuAction *action = actionItems.at(i);
        action->menuHandle = menuHandle;
        qt_wce_insert_action(menuHandle, action);
    }
    QMenuBar::wceRefresh();
}

void QMenuPrivate::QWceMenuPrivate::syncAction(QWceMenuAction *)
{
    rebuild();
}

void QMenuPrivate::QWceMenuPrivate::removeAction(QWceMenuAction *action)
{
    actionItems.removeAll(action);
    delete action;
    rebuild();
}

void QMenuBarPrivate::QWceMenuBarPrivate::addAction(QAction *a, QAction *before)
{
    QWceMenuAction *action = new QWceMenuAction;
    action->action = a;
    action->command = qt_wce_menu_static_cmd_id++;
    addAction(action, findAction(before));
}

void QMenuBarPrivate::QWceMenuBarPrivate::addAction(QWceMenuAction *action, QWceMenuAction *before)
{
    if (!action)
        return;
    int before_index = actionItems.indexOf(before);
    if (before_index < 0) {
        before = 0;
        before_index = actionItems.size();
    }
    actionItems.insert(before_index, action);
    rebuild();
}

void QMenuBarPrivate::QWceMenuBarPrivate::syncAction(QWceMenuAction*)
{
    QMenuBar::wceRefresh();
    rebuild();
}

void QMenuBarPrivate::QWceMenuBarPrivate::removeAction(QWceMenuAction *action)
{
    actionItems.removeAll(action);
    delete action;
    rebuild();
}

void QMenuBarPrivate::_q_updateDefaultAction()
{
    if (wce_menubar)
        wce_menubar->rebuild();
}

void QMenuBarPrivate::QWceMenuBarPrivate::rebuild()
{
    d->q_func()->resize(0,0);
    parentWindowHandle = d->q_func()->parentWidget() ? QApplicationPrivate::getHWNDForWidget(d->q_func()->parentWidget()) :
                                                       QApplicationPrivate::getHWNDForWidget(d->q_func());
    if (d->wceClassicMenu) {
        QList<QAction*> actions = d->actions;
        int maxEntries;
        int resourceHandle;
        if (actions.size() < 5) {
            maxEntries = 4;
            resourceHandle = IDR_MAIN_MENU3;
        } else if (actions.size() < 7) {
            maxEntries = 6;
            resourceHandle = IDR_MAIN_MENU4;
        }
        else {
          maxEntries = 8;
          resourceHandle = IDR_MAIN_MENU5;
        }
        Q_ASSERT_X(menubarHandle, "rebuild !created", "menubar already deleted");
        qt_wce_clear_menu(menuHandle);
        DestroyWindow(menubarHandle);
        menubarHandle = qt_wce_create_menubar(parentWindowHandle, qt_wce_get_module_handle(), resourceHandle);
        Q_ASSERT_X(menubarHandle, "rebuild classic menu", "cannot create menubar from resource");
        DrawMenuBar(menubarHandle);
        QList<int> menu_ids;
        QList<int> item_ids;
        menu_ids << IDM_MENU1 << IDM_MENU2 << IDM_MENU3 << IDM_MENU4 << IDM_MENU5 << IDM_MENU6 << IDM_MENU7 << IDM_MENU8;
        item_ids << IDM_ITEM1 << IDM_ITEM2 << IDM_ITEM3 << IDM_ITEM4 << IDM_ITEM5 << IDM_ITEM6 << IDM_ITEM7 << IDM_ITEM8;

        for (int i = 0; i < actionItemsClassic.size(); ++i)
          if (!actionItemsClassic.value(i).empty())
            qt_wce_delete_action_list(&actionItemsClassic[i]);
        actionItemsClassic.clear();

        for (int i = 0; i < actions.size(); ++i) {
            qt_wce_rename_menu_item(menubarHandle, menu_ids.at(i), actions.at(i)->text());
            QList<QAction *> subActions = actions.at(i)->menu()->actions();
            HMENU subMenuHandle = (HMENU) SendMessage(menubarHandle, SHCMBM_GETSUBMENU,0 , menu_ids.at(i));
            DeleteMenu(subMenuHandle, item_ids.at(i), MF_BYCOMMAND);
            for (int c = 0; c < subActions.size(); ++c) {
                QList<QWceMenuAction*> list;
                actionItemsClassic.append(list);
                QWceMenuAction *action = new QWceMenuAction;
                action->action = subActions.at(c);
                action->command = qt_wce_menu_static_cmd_id++;
                action->menuHandle = subMenuHandle;
                actionItemsClassic.last().append(action);
                qt_wce_insert_action(subMenuHandle, action);
            }
        }
        for (int i = actions.size();i<maxEntries;++i) {
            qt_wce_rename_menu_item(menubarHandle, menu_ids.at(i), QString());
            qt_wce_disable_soft_key(menubarHandle, menu_ids.at(i));
        }
    } else {
        leftButtonAction = d->defaultAction;
        if (!leftButtonAction)
            leftButtonAction = qt_wce_get_quit_action(actionItems);

        leftButtonIsMenu = (leftButtonAction && leftButtonAction->menu());
        Q_ASSERT_X(menubarHandle, "rebuild !created", "menubar already deleted");
        qt_wce_clear_menu(menuHandle);
        DestroyWindow(menubarHandle);
        if (leftButtonIsMenu) {
            menubarHandle = qt_wce_create_menubar(parentWindowHandle, qt_wce_get_module_handle(), IDR_MAIN_MENU2);
            Q_ASSERT_X(menubarHandle, "rebuild !created left menubar", "cannot create menubar from resource");
            menuHandle = (HMENU) SendMessage(menubarHandle, SHCMBM_GETSUBMENU,0,IDM_MENU);
            Q_ASSERT_X(menuHandle, "rebuild !created", "IDM_MENU not found - invalid resource?");
            DeleteMenu(menuHandle, IDM_ABOUT, MF_BYCOMMAND);
            leftButtonMenuHandle = (HMENU) SendMessage(menubarHandle, SHCMBM_GETSUBMENU,0,IDM_LEFTMENU);
            Q_ASSERT_X(leftButtonMenuHandle, "rebuild !created", "IDM_LEFTMENU not found - invalid resource?");
            DeleteMenu(leftButtonMenuHandle, IDM_VIEW, MF_BYCOMMAND);
        } else {
            menubarHandle = qt_wce_create_menubar(parentWindowHandle, qt_wce_get_module_handle(), IDR_MAIN_MENU);
            Q_ASSERT_X(menubarHandle, "rebuild !created no left menubar", "cannot create menubar from resource");
            menuHandle = (HMENU) SendMessage(menubarHandle, SHCMBM_GETSUBMENU,0,IDM_MENU);
            Q_ASSERT_X(menuHandle, "rebuild !created", "IDM_MENU not found - invalid resource?");
            DeleteMenu(menuHandle, IDM_ABOUT, MF_BYCOMMAND);
            leftButtonMenuHandle = 0;
            leftButtonCommand = qt_wce_menu_static_cmd_id++;
            qt_wce_change_command(menubarHandle, IDM_EXIT, leftButtonCommand);
        }

        if (actionItems.size() == 0) {
            qt_wce_rename_menu_item(menubarHandle, IDM_MENU, QLatin1String(""));
            qt_wce_disable_soft_key(menubarHandle, IDM_MENU);
        }
        for (int i = 0; i < actionItems.size(); ++i) {
            QWceMenuAction *action = actionItems.at(i);
            action->menuHandle = menuHandle;
            qt_wce_insert_action(menuHandle, action);
        }
        if (!leftButtonIsMenu) {
            if (leftButtonAction) {
                qt_wce_rename_menu_item(menubarHandle, leftButtonCommand, leftButtonAction->text());
                qt_wce_enable_soft_key(menubarHandle, leftButtonCommand);
            } else {
                qt_wce_rename_menu_item(menubarHandle, leftButtonCommand, QLatin1String(""));
                qt_wce_disable_soft_key(menubarHandle, leftButtonCommand);
            }
        } else {
            qt_wce_rename_menu_item(menubarHandle, IDM_LEFTMENU, leftButtonAction->text());
            QList<QAction *> actions = leftButtonAction->menu()->actions();
            qt_wce_delete_action_list(&actionItemsLeftButton);
            for (int i=0; i<actions.size(); ++i) {
                QWceMenuAction *action = new QWceMenuAction;
                action->action = actions.at(i);
                action->command = qt_wce_menu_static_cmd_id++;
                action->menuHandle = leftButtonMenuHandle;
                actionItemsLeftButton.append(action);
                qt_wce_insert_action(leftButtonMenuHandle, action);
            }
        }
    }
    DrawMenuBar(menubarHandle);
}

QT_END_NAMESPACE

#endif //QT_NO_MENUBAR
#endif //Q_OS_WINCE
