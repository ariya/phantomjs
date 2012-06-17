/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the S60 port of the Qt Toolkit.
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

#include "qmenu.h"
#include "qapplication.h"
#include "qevent.h"
#include "qstyle.h"
#include "qdebug.h"
#include "qwidgetaction.h"
#include <private/qapplication_p.h>
#include <private/qmenu_p.h>
#include <private/qmenubar_p.h>
#include <private/qt_s60_p.h>
#include <QtCore/qlibrary.h>

#ifdef Q_WS_S60
#include <eikmenub.h>
#include <eikmenup.h>
#include <eikaufty.h>
#include <eikbtgpc.h>
#include <avkon.rsg>
#endif

#if !defined(QT_NO_MENUBAR) && defined(Q_WS_S60)

QT_BEGIN_NAMESPACE

typedef QMultiHash<QWidget *, QMenuBarPrivate *> MenuBarHash;
Q_GLOBAL_STATIC(MenuBarHash, menubars)

struct SymbianMenuItem
{
    int id;
    CEikMenuPaneItem::SData menuItemData;
    QList<SymbianMenuItem*> children;
    QAction* action;
};

Q_GLOBAL_STATIC_WITH_ARGS(QAction, contextAction, (0))

static QList<SymbianMenuItem*> symbianMenus;
static QList<QMenuBar*> nativeMenuBars;
static uint qt_symbian_menu_static_cmd_id = QT_SYMBIAN_FIRST_MENU_ITEM;
static QPointer<QWidget> widgetWithContextMenu;
static QList<QAction*> contextMenuActionList;
static QWidget* actionMenu = NULL;
static int contexMenuCommand=0;

bool menuExists()
{
    QWidget *w = qApp->activeWindow();
    QMenuBarPrivate *mb = menubars()->value(w);
    if ((!mb) && !menubars()->count())
        return false;
    return true;
}

static bool hasContextMenu(QWidget* widget)
{
    if (!widget)
        return false;
    const Qt::ContextMenuPolicy policy = widget->contextMenuPolicy();
    if (policy != Qt::NoContextMenu && policy != Qt::PreventContextMenu ) {
        return true;
    }
    return false;
}

static SymbianMenuItem* qt_symbian_find_menu(int id, const QList<SymbianMenuItem*> &parent)
{
    int index=0;
    while (index < parent.count()) {
        SymbianMenuItem* temp = parent[index];
        if (temp->menuItemData.iCascadeId == id)
           return temp;
        else if (temp->menuItemData.iCascadeId != 0) {
            SymbianMenuItem* result = qt_symbian_find_menu( id, temp->children);
            if (result)
                return result;
        }
        index++;
    }
    return 0;
}

static SymbianMenuItem* qt_symbian_find_menu_item(int id, const QList<SymbianMenuItem*> &parent)
{
    int index=0;
    while (index < parent.count()) {
        SymbianMenuItem* temp = parent[index];
        if (temp->menuItemData.iCascadeId != 0) {
            SymbianMenuItem* result = qt_symbian_find_menu_item( id, temp->children);
            if (result)
                return result;
        }
        else if (temp->menuItemData.iCommandId == id)
            return temp;
        index++;

    }
    return 0;
}

static void qt_symbian_insert_action(QSymbianMenuAction* action, QList<SymbianMenuItem*>* parent)
{
    if (action->action->isVisible()) {
        if (action->action->isSeparator())
            return;

        Q_ASSERT_X(action->command <= QT_SYMBIAN_LAST_MENU_ITEM, "qt_symbian_insert_action",
                "Too many menu actions");

        const int underlineShortCut = QApplication::style()->styleHint(QStyle::SH_UnderlineShortcut);
        QString actionText;
        if (underlineShortCut)
            actionText = action->action->text().left(CEikMenuPaneItem::SData::ENominalTextLength);
        else
            actionText = action->action->iconText().left(CEikMenuPaneItem::SData::ENominalTextLength);
        TPtrC menuItemText = qt_QString2TPtrC(actionText);
        if (action->action->menu()) {
            SymbianMenuItem* menuItem = new SymbianMenuItem();
            menuItem->menuItemData.iCascadeId = action->command;
            menuItem->menuItemData.iCommandId = action->command;
            menuItem->menuItemData.iFlags = 0;
            menuItem->menuItemData.iText = menuItemText;
            menuItem->action = action->action;
            if (action->action->menu()->actions().size() == 0 || !action->action->isEnabled() )
                menuItem->menuItemData.iFlags |= EEikMenuItemDimmed;
            parent->append(menuItem);

            if (action->action->menu()->actions().size() > 0) {
                for (int c2= 0; c2 < action->action->menu()->actions().size(); ++c2) {
                    QScopedPointer<QSymbianMenuAction> symbianAction2(new QSymbianMenuAction);
                    symbianAction2->action = action->action->menu()->actions().at(c2);
                    QMenu * menu = symbianAction2->action->menu();
                    symbianAction2->command = qt_symbian_menu_static_cmd_id++;
                    qt_symbian_insert_action(symbianAction2.data(), &(menuItem->children));
                }
            }

        } else {
            SymbianMenuItem* menuItem = new SymbianMenuItem();
            menuItem->menuItemData.iCascadeId = 0;
            menuItem->menuItemData.iCommandId = action->command;
            menuItem->menuItemData.iFlags = 0;
            menuItem->menuItemData.iText = menuItemText;
            menuItem->action = action->action;
            if (!action->action->isEnabled()){
                menuItem->menuItemData.iFlags += EEikMenuItemDimmed;
            }

            if (action->action->isCheckable()) {
                if (action->action->isChecked())
                    menuItem->menuItemData.iFlags += EEikMenuItemCheckBox | EEikMenuItemSymbolOn;
                else
                    menuItem->menuItemData.iFlags += EEikMenuItemCheckBox;
            }
            parent->append(menuItem);
        }
    }
}

void deleteAll(QList<SymbianMenuItem*> *items)
{
    while (!items->isEmpty()) {
        SymbianMenuItem* temp = items->takeFirst();
        deleteAll(&temp->children);
        delete temp;
    }
}

static void rebuildMenu()
{
    widgetWithContextMenu = 0;
    QMenuBarPrivate *mb = 0;
    QWidget *w = qApp->activeWindow();
    QWidget* focusWidget = QApplication::focusWidget();
    if (focusWidget) {
        if (hasContextMenu(focusWidget))
            widgetWithContextMenu = focusWidget;
    }

    if (w) {
        mb = menubars()->value(w);
        qt_symbian_menu_static_cmd_id = QT_SYMBIAN_FIRST_MENU_ITEM;
        deleteAll( &symbianMenus );
        if (!mb)
            return;
        mb->symbian_menubar->rebuild();
    }
}

#ifdef Q_WS_S60
void qt_symbian_next_menu_from_action(QWidget *actionContainer)
{
    actionMenu = actionContainer;
}

void qt_symbian_show_toplevel( CEikMenuPane* menuPane)
{
    if (actionMenu) {
        QMenuBarPrivate *mb = 0;
        mb = menubars()->value(actionMenu);
        qt_symbian_menu_static_cmd_id = QT_SYMBIAN_FIRST_MENU_ITEM;
        deleteAll( &symbianMenus );
        Q_ASSERT(mb);
        mb->symbian_menubar->rebuild();
        for (int i = 0; i < symbianMenus.count(); ++i)
            QT_TRAP_THROWING(menuPane->AddMenuItemL(symbianMenus.at(i)->menuItemData));
        actionMenu = NULL;
        return;
    }

    if (!menuExists())
        return;
    rebuildMenu();
    for (int i = 0; i < symbianMenus.count(); ++i)
        QT_TRAP_THROWING(menuPane->AddMenuItemL(symbianMenus.at(i)->menuItemData));
}

void qt_symbian_show_submenu( CEikMenuPane* menuPane, int id)
{
    SymbianMenuItem* menu = qt_symbian_find_menu(id, symbianMenus);
    if (menu) {
        // Normally first AddMenuItemL call for menuPane will create the item array.
        // However if we don't have any items, we still need the item array. Otherwise
        // menupane will crash. That's why we create item array here manually, and
        // AddMenuItemL will then use the existing array.
        CEikMenuPane::CItemArray* itemArray = new CEikMenuPane::CItemArray;
        Q_CHECK_PTR(itemArray);
        menuPane->SetItemArray(itemArray);
        menuPane->SetItemArrayOwnedExternally(EFalse);

        for (int i = 0; i < menu->children.count(); ++i)
            QT_TRAP_THROWING(menuPane->AddMenuItemL(menu->children.at(i)->menuItemData));
    }
}
#endif // Q_WS_S60

int QMenuBarPrivate::symbianCommands(int command)
{
    int ret = 0;

    if (command == contexMenuCommand && !widgetWithContextMenu.isNull()) {
        QContextMenuEvent* event = new QContextMenuEvent(QContextMenuEvent::Keyboard, QPoint(0,0));
        QCoreApplication::postEvent(widgetWithContextMenu, event);
        ret = 1;
    }

    int size = nativeMenuBars.size();
    for (int i = 0; i < nativeMenuBars.size(); ++i) {
        SymbianMenuItem* menu = qt_symbian_find_menu_item(command, symbianMenus);
        if (!menu)
            continue;

        emit nativeMenuBars.at(i)->triggered(menu->action);
        menu->action->activate(QAction::Trigger);
        ret = 1;
        break;
    }

    return ret;
}

void QMenuBarPrivate::symbianCreateMenuBar(QWidget *parent)
{
    Q_Q(QMenuBar);
    if (parent) {
        if(parent->isWindow()) {
            menubars()->insert(q->window(), this);
            symbian_menubar = new QSymbianMenuBarPrivate(this);
            nativeMenuBars.append(q);
        } else {
            menubars()->insert(q->parentWidget(), this);
            symbian_menubar = new QSymbianMenuBarPrivate(this);
            nativeMenuBars.append(q);
        }
    }
}

void QMenuBarPrivate::symbianDestroyMenuBar()
{
    Q_Q(QMenuBar);
    int index = nativeMenuBars.indexOf(q);
    nativeMenuBars.removeAt(index);
    menubars()->remove(q->window(), this);
    menubars()->remove(q->parentWidget(), this);
    rebuildMenu();
    if (symbian_menubar)
        delete symbian_menubar;
    symbian_menubar = 0;
}

void QMenuBarPrivate::reparentMenuBar(QWidget *oldParent, QWidget *newParent)
{
    if (menubars()->contains(oldParent)) {
        QMenuBarPrivate *object = menubars()->take(oldParent);
        menubars()->insert(newParent, object);
    }
}

QMenuBarPrivate::QSymbianMenuBarPrivate::QSymbianMenuBarPrivate(QMenuBarPrivate *menubar)
{
    d = menubar;
}

QMenuBarPrivate::QSymbianMenuBarPrivate::~QSymbianMenuBarPrivate()
{
    qt_symbian_menu_static_cmd_id = QT_SYMBIAN_FIRST_MENU_ITEM;
    deleteAll( &symbianMenus );
    symbianMenus.clear();
    d = 0;
    rebuild();
}

QMenuPrivate::QSymbianMenuPrivate::QSymbianMenuPrivate()
{
}

QMenuPrivate::QSymbianMenuPrivate::~QSymbianMenuPrivate()
{
    qDeleteAll(actionItems);
}

void QMenuPrivate::QSymbianMenuPrivate::addAction(QAction *a, QSymbianMenuAction *before)
{
    QSymbianMenuAction *action = new QSymbianMenuAction;
    action->action = a;
    action->command = qt_symbian_menu_static_cmd_id++;
    addAction(action, before);
}

void QMenuPrivate::QSymbianMenuPrivate::addAction(QSymbianMenuAction *action, QSymbianMenuAction *before)
{
    if (!action)
        return;
    int before_index = actionItems.indexOf(before);
    if (before_index < 0) {
        before = 0;
        before_index = actionItems.size();
    }
    actionItems.insert(before_index, action);
}


void QMenuPrivate::QSymbianMenuPrivate::syncAction(QSymbianMenuAction *)
{
    rebuild();
}

void QMenuPrivate::QSymbianMenuPrivate::removeAction(QSymbianMenuAction *action)
{
    actionItems.removeAll(action);
    delete action;
    action = 0;
    rebuild();
}

void QMenuPrivate::QSymbianMenuPrivate::rebuild(bool)
{
}

void QMenuBarPrivate::QSymbianMenuBarPrivate::addAction(QAction *a, QAction *before)
{
    QSymbianMenuAction *action = new QSymbianMenuAction;
    action->action = a;
    action->command = qt_symbian_menu_static_cmd_id++;
    addAction(action, findAction(before));
}

void QMenuBarPrivate::QSymbianMenuBarPrivate::addAction(QSymbianMenuAction *action, QSymbianMenuAction *before)
{
    if (!action)
        return;
    int before_index = actionItems.indexOf(before);
    if (before_index < 0) {
        before = 0;
        before_index = actionItems.size();
    }
    actionItems.insert(before_index, action);
}

void QMenuBarPrivate::QSymbianMenuBarPrivate::syncAction(QSymbianMenuAction*)
{
    rebuild();
}

void QMenuBarPrivate::QSymbianMenuBarPrivate::removeAction(QSymbianMenuAction *action)
{
    actionItems.removeAll(action);
    delete action;
    rebuild();
}

void QMenuBarPrivate::QSymbianMenuBarPrivate::insertNativeMenuItems(const QList<QAction*> &actions)
{
    for (int i = 0; i <actions.size(); ++i) {
        QScopedPointer<QSymbianMenuAction> symbianActionTopLevel(new QSymbianMenuAction);
        symbianActionTopLevel->action = actions.at(i);
        symbianActionTopLevel->parent = 0;
        symbianActionTopLevel->command = qt_symbian_menu_static_cmd_id++;
        qt_symbian_insert_action(symbianActionTopLevel.data(), &symbianMenus);
    }
}



void QMenuBarPrivate::QSymbianMenuBarPrivate::rebuild()
{
    contexMenuCommand = 0;
    qt_symbian_menu_static_cmd_id = QT_SYMBIAN_FIRST_MENU_ITEM;
    deleteAll( &symbianMenus );
    if (d)
        insertNativeMenuItems(d->actions);

    contextMenuActionList.clear();
    if (widgetWithContextMenu) {
        contexMenuCommand = qt_symbian_menu_static_cmd_id; // Increased inside insertNativeMenuItems
        contextAction()->setText(QMenuBar::tr("Actions"));
        contextMenuActionList.append(contextAction());
        insertNativeMenuItems(contextMenuActionList);
    }
}
QT_END_NAMESPACE

#endif //QT_NO_MENUBAR
