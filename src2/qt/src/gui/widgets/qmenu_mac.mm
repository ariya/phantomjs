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

#include "qmenu.h"
#include "qhash.h"
#include <qdebug.h>
#include "qapplication.h"
#include <private/qt_mac_p.h>
#include "qregexp.h"
#include "qmainwindow.h"
#include "qdockwidget.h"
#include "qtoolbar.h"
#include "qevent.h"
#include "qstyle.h"
#include "qwidgetaction.h"
#include "qmacnativewidget_mac.h"

#include <private/qapplication_p.h>
#include <private/qcocoaapplication_mac_p.h>
#include <private/qmenu_p.h>
#include <private/qmenubar_p.h>
#include <private/qcocoamenuloader_mac_p.h>
#include <private/qcocoamenu_mac_p.h>
#include <private/qt_cocoa_helpers_mac_p.h>
#include <Cocoa/Cocoa.h>

QT_BEGIN_NAMESPACE

/*****************************************************************************
  QMenu debug facilities
 *****************************************************************************/

/*****************************************************************************
  QMenu globals
 *****************************************************************************/
bool qt_mac_no_menubar_merge = false;
bool qt_mac_quit_menu_item_enabled = true;
int qt_mac_menus_open_count = 0;

static OSMenuRef qt_mac_create_menu(QWidget *w);

#ifndef QT_MAC_USE_COCOA
static uint qt_mac_menu_static_cmd_id = 'QT00';
const UInt32 kMenuCreatorQt = 'cute';
enum {
    kMenuPropertyQAction = 'QAcT',
    kMenuPropertyQWidget = 'QWId',
    kMenuPropertyCausedQWidget = 'QCAU',
    kMenuPropertyMergeMenu = 'QApP',
    kMenuPropertyMergeList = 'QAmL',
    kMenuPropertyWidgetActionWidget = 'QWid',
    kMenuPropertyWidgetMenu = 'QWMe',

    kHICommandAboutQt = 'AOQT',
    kHICommandCustomMerge = 'AQt0'
};
#endif

static struct {
    QPointer<QMenuBar> qmenubar;
    bool modal;
} qt_mac_current_menubar = { 0, false };




/*****************************************************************************
  Externals
 *****************************************************************************/
extern OSViewRef qt_mac_hiview_for(const QWidget *w); //qwidget_mac.cpp
extern HIViewRef qt_mac_hiview_for(OSWindowRef w); //qwidget_mac.cpp
extern IconRef qt_mac_create_iconref(const QPixmap &px); //qpixmap_mac.cpp
extern QWidget * mac_keyboard_grabber; //qwidget_mac.cpp
extern bool qt_sendSpontaneousEvent(QObject*, QEvent*); //qapplication_xxx.cpp
RgnHandle qt_mac_get_rgn(); //qregion_mac.cpp
void qt_mac_dispose_rgn(RgnHandle r); //qregion_mac.cpp

/*****************************************************************************
  QMenu utility functions
 *****************************************************************************/
bool qt_mac_watchingAboutToShow(QMenu *menu)
{
    return menu && menu->receivers(SIGNAL(aboutToShow()));
}

static int qt_mac_CountMenuItems(OSMenuRef menu)
{
    if (menu) {
#ifndef QT_MAC_USE_COCOA
        int ret = 0;
        const int items = CountMenuItems(menu);
        for(int i = 0; i < items; i++) {
            MenuItemAttributes attr;
            if (GetMenuItemAttributes(menu, i+1, &attr) == noErr &&
               attr & kMenuItemAttrHidden)
                continue;
            ++ret;
        }
        return ret;
#else
        return [menu numberOfItems];
#endif
    }
    return 0;
}

static quint32 constructModifierMask(quint32 accel_key)
{
    quint32 ret = 0;
    const bool dontSwap = qApp->testAttribute(Qt::AA_MacDontSwapCtrlAndMeta);
#ifndef QT_MAC_USE_COCOA
    if ((accel_key & Qt::ALT) == Qt::ALT)
        ret |= kMenuOptionModifier;
    if ((accel_key & Qt::SHIFT) == Qt::SHIFT)
        ret |= kMenuShiftModifier;
    if (dontSwap) {
        if ((accel_key & Qt::META) != Qt::META)
            ret |= kMenuNoCommandModifier;
        if ((accel_key & Qt::CTRL) == Qt::CTRL)
            ret |= kMenuControlModifier;
    } else {
        if ((accel_key & Qt::CTRL) != Qt::CTRL)
            ret |= kMenuNoCommandModifier;
        if ((accel_key & Qt::META) == Qt::META)
            ret |= kMenuControlModifier;
    }
#else
    if ((accel_key & Qt::CTRL) == Qt::CTRL)
        ret |= (dontSwap ? NSControlKeyMask : NSCommandKeyMask);
    if ((accel_key & Qt::META) == Qt::META)
        ret |= (dontSwap ? NSCommandKeyMask : NSControlKeyMask);
    if ((accel_key & Qt::ALT) == Qt::ALT)
        ret |= NSAlternateKeyMask;
    if ((accel_key & Qt::SHIFT) == Qt::SHIFT)
        ret |= NSShiftKeyMask;
#endif
    return ret;
}

static void cancelAllMenuTracking()
{
#ifdef QT_MAC_USE_COCOA
    QMacCocoaAutoReleasePool pool;
    NSMenu *mainMenu = [NSApp mainMenu];
    [mainMenu cancelTracking];
    for (NSMenuItem *item in [mainMenu itemArray]) {
        if ([item submenu]) {
            [[item submenu] cancelTracking];
        }
    }
#else
    CancelMenuTracking(AcquireRootMenu(), true, 0);
#endif
}

static bool actualMenuItemVisibility(const QMenuBarPrivate::QMacMenuBarPrivate *mbp,
                                     const QMacMenuAction *action)
{
    bool visible = action->action->isVisible();
    if (visible && action->action->text() == QString(QChar(0x14)))
        return false;
    if (visible && action->action->menu() && !action->action->menu()->actions().isEmpty() &&
        !qt_mac_CountMenuItems(action->action->menu()->macMenu(mbp->apple_menu)) &&
        !qt_mac_watchingAboutToShow(action->action->menu())) {
        return false;
    }
    return visible;
}

#ifndef QT_MAC_USE_COCOA
bool qt_mac_activate_action(MenuRef menu, uint command, QAction::ActionEvent action_e, bool by_accel)
{
    //fire event
    QMacMenuAction *action = 0;
    if (GetMenuCommandProperty(menu, command, kMenuCreatorQt, kMenuPropertyQAction, sizeof(action), 0, &action) != noErr) {
        QMenuMergeList *list = 0;
        GetMenuItemProperty(menu, 0, kMenuCreatorQt, kMenuPropertyMergeList,
                            sizeof(list), 0, &list);
        if (!list && qt_mac_current_menubar.qmenubar && qt_mac_current_menubar.qmenubar->isNativeMenuBar()) {
            MenuRef apple_menu = qt_mac_current_menubar.qmenubar->d_func()->mac_menubar->apple_menu;
            GetMenuItemProperty(apple_menu, 0, kMenuCreatorQt, kMenuPropertyMergeList, sizeof(list), 0, &list);
            if (list)
                menu = apple_menu;
        }
        if (list) {
            for(int i = 0; i < list->size(); ++i) {
                QMenuMergeItem item = list->at(i);
                if (item.command == command && item.action) {
                    action = item.action;
                    break;
                }
            }
        }
        if (!action)
            return false;
    }

    if (action_e == QAction::Trigger && by_accel && action->ignore_accel) //no, not a real accel (ie tab)
        return false;

    // Unhighlight the highlighted menu item before triggering the action to
    // prevent items from staying highlighted while a modal dialog is shown.
    // This also fixed the problem that parentless modal dialogs leave
    // the menu item highlighted (since the menu bar is cleared for these types of dialogs).
    if (action_e == QAction::Trigger)
        HiliteMenu(0);

    action->action->activate(action_e);

    //now walk up firing for each "caused" widget (like in the platform independent menu)
    QWidget *caused = 0;
    if (action_e == QAction::Hover && GetMenuItemProperty(menu, 0, kMenuCreatorQt, kMenuPropertyCausedQWidget, sizeof(caused), 0, &caused) == noErr) {
        MenuRef caused_menu = 0;
        if (QMenu *qmenu2 = qobject_cast<QMenu*>(caused))
            caused_menu = qmenu2->macMenu();
        else if (QMenuBar *qmenubar2 = qobject_cast<QMenuBar*>(caused))
            caused_menu = qmenubar2->macMenu();
        else
            caused_menu = 0;
        while(caused_menu) {
            //fire
            QWidget *widget = 0;
            GetMenuItemProperty(caused_menu, 0, kMenuCreatorQt, kMenuPropertyQWidget, sizeof(widget), 0, &widget);
            if (QMenu *qmenu = qobject_cast<QMenu*>(widget)) {
                action->action->showStatusText(widget);
                emit qmenu->hovered(action->action);
            } else if (QMenuBar *qmenubar = qobject_cast<QMenuBar*>(widget)) {
                action->action->showStatusText(widget);
                emit qmenubar->hovered(action->action);
                break; //nothing more..
            }

            //walk up
            if (GetMenuItemProperty(caused_menu, 0, kMenuCreatorQt, kMenuPropertyCausedQWidget,
                                    sizeof(caused), 0, &caused) != noErr)
                break;
            if (QMenu *qmenu2 = qobject_cast<QMenu*>(caused))
                caused_menu = qmenu2->macMenu();
            else if (QMenuBar *qmenubar2 = qobject_cast<QMenuBar*>(caused))
                caused_menu = qmenubar2->macMenu();
            else
                caused_menu = 0;
        }
    }
    return true;
}

//lookup a QMacMenuAction in a menu
static int qt_mac_menu_find_action(MenuRef menu, MenuCommand cmd)
{
    MenuItemIndex ret_idx;
    MenuRef ret_menu;
    if (GetIndMenuItemWithCommandID(menu, cmd, 1, &ret_menu, &ret_idx) == noErr) {
        if (ret_menu == menu)
            return (int)ret_idx;
    }
    return -1;
}
static int qt_mac_menu_find_action(MenuRef menu, QMacMenuAction *action)
{
    return qt_mac_menu_find_action(menu, action->command);
}

typedef QMultiHash<OSMenuRef, EventHandlerRef> EventHandlerHash;
Q_GLOBAL_STATIC(EventHandlerHash, menu_eventHandlers_hash)

static EventTypeSpec widget_in_menu_events[] = {
    { kEventClassMenu, kEventMenuMeasureItemWidth },
    { kEventClassMenu, kEventMenuMeasureItemHeight },
    { kEventClassMenu, kEventMenuDrawItem },
    { kEventClassMenu, kEventMenuCalculateSize }
};

static OSStatus qt_mac_widget_in_menu_eventHandler(EventHandlerCallRef er, EventRef event, void *)
{
    UInt32 ekind = GetEventKind(event);
    UInt32 eclass = GetEventClass(event);
    OSStatus result = eventNotHandledErr;
    switch (eclass) {
    case kEventClassMenu:
        switch (ekind) {
        default:
            break;
        case kEventMenuMeasureItemWidth: {
            MenuItemIndex item;
            GetEventParameter(event, kEventParamMenuItemIndex, typeMenuItemIndex,
                              0, sizeof(item), 0, &item);
            OSMenuRef menu;
            GetEventParameter(event, kEventParamDirectObject, typeMenuRef, 0, sizeof(menu), 0, &menu);
            QWidget *widget;
            if (GetMenuItemProperty(menu, item, kMenuCreatorQt, kMenuPropertyWidgetActionWidget,
                                 sizeof(widget), 0, &widget) == noErr) {
                short width = short(widget->sizeHint().width());
                SetEventParameter(event, kEventParamMenuItemWidth, typeSInt16,
                                  sizeof(short), &width);
                result = noErr;
            }
            break; }
        case kEventMenuMeasureItemHeight: {
            MenuItemIndex item;
            GetEventParameter(event, kEventParamMenuItemIndex, typeMenuItemIndex,
                              0, sizeof(item), 0, &item);
            OSMenuRef menu;
            GetEventParameter(event, kEventParamDirectObject, typeMenuRef, 0, sizeof(menu), 0, &menu);
            QWidget *widget;
            if (GetMenuItemProperty(menu, item, kMenuCreatorQt, kMenuPropertyWidgetActionWidget,
                                     sizeof(widget), 0, &widget) == noErr && widget) {
                short height = short(widget->sizeHint().height());
                SetEventParameter(event, kEventParamMenuItemHeight, typeSInt16,
                                  sizeof(short), &height);
                result = noErr;
            }
            break; }
        case kEventMenuDrawItem:
            result = noErr;
            break;
        case kEventMenuCalculateSize: {
            result = CallNextEventHandler(er, event);
            if (result == noErr) {
                OSMenuRef menu;
                GetEventParameter(event, kEventParamDirectObject, typeMenuRef, 0, sizeof(menu), 0, &menu);
                HIViewRef content;
                HIMenuGetContentView(menu, kThemeMenuTypePullDown, &content);
                UInt16 count = CountMenuItems(menu);
                for (MenuItemIndex i = 1; i <= count; ++i) {
                    QWidget *widget;
                    if (GetMenuItemProperty(menu, i, kMenuCreatorQt, kMenuPropertyWidgetActionWidget,
                            sizeof(widget), 0, &widget) == noErr && widget) {
                        RgnHandle itemRgn = qt_mac_get_rgn();
                        GetControlRegion(content, i, itemRgn);

                        Rect bounds;
                        GetRegionBounds( itemRgn, &bounds );
                        qt_mac_dispose_rgn(itemRgn);
                        widget->setGeometry(bounds.left, bounds.top,
                                            bounds.right - bounds.left, bounds.bottom - bounds.top);
                    }
                }
            }
            break; }
        }
    }
    return result;
}

//handling of events for menurefs created by Qt..
static EventTypeSpec menu_events[] = {
    { kEventClassCommand, kEventCommandProcess },
    { kEventClassMenu, kEventMenuTargetItem },
    { kEventClassMenu, kEventMenuOpening },
    { kEventClassMenu, kEventMenuClosed }
};

// Special case for kEventMenuMatchKey, see qt_mac_create_menu below.
static EventTypeSpec menu_menu_events[] = {
    { kEventClassMenu, kEventMenuMatchKey }
};

OSStatus qt_mac_menu_event(EventHandlerCallRef er, EventRef event, void *)
{
    QScopedLoopLevelCounter loopLevelCounter(QApplicationPrivate::instance()->threadData);

    bool handled_event = true;
    UInt32 ekind = GetEventKind(event), eclass = GetEventClass(event);
    switch(eclass) {
    case kEventClassCommand:
        if (ekind == kEventCommandProcess) {
            UInt32 context;
            GetEventParameter(event, kEventParamMenuContext, typeUInt32,
                              0, sizeof(context), 0, &context);
            HICommand cmd;
            GetEventParameter(event, kEventParamDirectObject, typeHICommand,
                              0, sizeof(cmd), 0, &cmd);
            if (!mac_keyboard_grabber && (context & kMenuContextKeyMatching)) {
                QMacMenuAction *action = 0;
                if (GetMenuCommandProperty(cmd.menu.menuRef, cmd.commandID, kMenuCreatorQt,
                                          kMenuPropertyQAction, sizeof(action), 0, &action) == noErr) {
                    QWidget *widget = 0;
                    if (qApp->activePopupWidget())
                        widget = (qApp->activePopupWidget()->focusWidget() ?
                                  qApp->activePopupWidget()->focusWidget() : qApp->activePopupWidget());
                    else if (QApplicationPrivate::focus_widget)
                        widget = QApplicationPrivate::focus_widget;
                    if (widget) {
                        int key = action->action->shortcut();
                        QKeyEvent accel_ev(QEvent::ShortcutOverride, (key & (~Qt::KeyboardModifierMask)),
                                           Qt::KeyboardModifiers(key & Qt::KeyboardModifierMask));
                        accel_ev.ignore();
                        qt_sendSpontaneousEvent(widget, &accel_ev);
                        if (accel_ev.isAccepted()) {
                            handled_event = false;
                            break;
                        }
                    }
                }
            }
            handled_event = qt_mac_activate_action(cmd.menu.menuRef, cmd.commandID,
                                                   QAction::Trigger, context & kMenuContextKeyMatching);
        }
        break;
    case kEventClassMenu: {
        MenuRef menu;
        GetEventParameter(event, kEventParamDirectObject, typeMenuRef, NULL, sizeof(menu), NULL, &menu);
        if (ekind == kEventMenuMatchKey) {
            // Don't activate any actions if we are showing a native modal dialog,
            // the key events should go to the dialog in this case.
            if (QApplicationPrivate::native_modal_dialog_active)
                return menuItemNotFoundErr;

             handled_event = false;
        } else if (ekind == kEventMenuTargetItem) {
            MenuCommand command;
            GetEventParameter(event, kEventParamMenuCommand, typeMenuCommand,
                              0, sizeof(command), 0, &command);
            handled_event = qt_mac_activate_action(menu, command, QAction::Hover, false);
        } else if (ekind == kEventMenuOpening || ekind == kEventMenuClosed) {
            qt_mac_menus_open_count += (ekind == kEventMenuOpening) ? 1 : -1;
            MenuRef mr;
            GetEventParameter(event, kEventParamDirectObject, typeMenuRef,
                              0, sizeof(mr), 0, &mr);

            QWidget *widget = 0;
            if (GetMenuItemProperty(mr, 0, kMenuCreatorQt, kMenuPropertyQWidget, sizeof(widget), 0, &widget) == noErr) {
                if (QMenu *qmenu = qobject_cast<QMenu*>(widget)) {
                    handled_event = true;
                    if (ekind == kEventMenuOpening) {
                        emit qmenu->aboutToShow();

                        int merged = 0;
                        const QMenuPrivate::QMacMenuPrivate *mac_menu = qmenu->d_func()->mac_menu;
                        const int ActionItemsCount = mac_menu->actionItems.size();
                        for(int i = 0; i < ActionItemsCount; ++i) {
                            QMacMenuAction *action = mac_menu->actionItems.at(i);
                            if (action->action->isSeparator()) {
                                bool hide = false;
                                if(!action->action->isVisible()) {
                                    hide = true;
                                } else if (merged && merged == i) {
                                    hide = true;
                                } else {
                                    for(int l = i+1; l < mac_menu->actionItems.size(); ++l) {
                                        QMacMenuAction *action = mac_menu->actionItems.at(l);
                                        if (action->merged) {
                                            hide = true;
                                        } else if (action->action->isSeparator()) {
                                            if (hide)
                                                break;
                                        } else if (!action->merged) {
                                            hide = false;
                                            break;
                                        }
                                    }
                                }

                                const int index = qt_mac_menu_find_action(mr, action);
                                if (hide) {
                                    ++merged;
                                    ChangeMenuItemAttributes(mr, index, kMenuItemAttrHidden, 0);
                                } else {
                                    ChangeMenuItemAttributes(mr, index, 0, kMenuItemAttrHidden);
                                }
                            } else if (action->merged) {
                                ++merged;
                            }
                        }
                    } else {
                        emit qmenu->aboutToHide();
                    }
                }
            }
        } else {
            handled_event = false;
        }
        break; }
    default:
        handled_event = false;
        break;
    }
    if (!handled_event) //let the event go through
        return CallNextEventHandler(er, event);
    return noErr; //we eat the event
}
static EventHandlerRef mac_menu_event_handler = 0;
static EventHandlerUPP mac_menu_eventUPP = 0;
static void qt_mac_cleanup_menu_event()
{
    if (mac_menu_event_handler) {
        RemoveEventHandler(mac_menu_event_handler);
        mac_menu_event_handler = 0;
    }
    if (mac_menu_eventUPP) {
        DisposeEventHandlerUPP(mac_menu_eventUPP);
        mac_menu_eventUPP = 0;
    }
}
static inline void qt_mac_create_menu_event_handler()
{
    if (!mac_menu_event_handler) {
        mac_menu_eventUPP = NewEventHandlerUPP(qt_mac_menu_event);
        InstallEventHandler(GetApplicationEventTarget(), mac_menu_eventUPP,
                            GetEventTypeCount(menu_events), menu_events, 0,
                            &mac_menu_event_handler);
        qAddPostRoutine(qt_mac_cleanup_menu_event);
    }
}


//enabling of commands
static void qt_mac_command_set_enabled(MenuRef menu, UInt32 cmd, bool b)
{
    if (cmd == kHICommandQuit)
        qt_mac_quit_menu_item_enabled = b;

    if (b) {
        EnableMenuCommand(menu, cmd);
        if (MenuRef dock_menu = GetApplicationDockTileMenu())
            EnableMenuCommand(dock_menu, cmd);
    } else {
        DisableMenuCommand(menu, cmd);
        if (MenuRef dock_menu = GetApplicationDockTileMenu())
            DisableMenuCommand(dock_menu, cmd);
    }
}

static bool qt_mac_auto_apple_menu(MenuCommand cmd)
{
    return (cmd == kHICommandPreferences || cmd == kHICommandQuit);
}

static void qt_mac_get_accel(quint32 accel_key, quint32 *modif, quint32 *key) {
    if (modif) {
        *modif = constructModifierMask(accel_key);
    }

    accel_key &= ~(Qt::MODIFIER_MASK | Qt::UNICODE_ACCEL);
    if (key) {
        *key = 0;
        if (accel_key == Qt::Key_Return)
            *key = kMenuReturnGlyph;
        else if (accel_key == Qt::Key_Enter)
            *key = kMenuEnterGlyph;
        else if (accel_key == Qt::Key_Tab)
            *key = kMenuTabRightGlyph;
        else if (accel_key == Qt::Key_Backspace)
            *key = kMenuDeleteLeftGlyph;
        else if (accel_key == Qt::Key_Delete)
            *key = kMenuDeleteRightGlyph;
        else if (accel_key == Qt::Key_Escape)
            *key = kMenuEscapeGlyph;
        else if (accel_key == Qt::Key_PageUp)
            *key = kMenuPageUpGlyph;
        else if (accel_key == Qt::Key_PageDown)
            *key = kMenuPageDownGlyph;
        else if (accel_key == Qt::Key_Up)
            *key = kMenuUpArrowGlyph;
        else if (accel_key == Qt::Key_Down)
            *key = kMenuDownArrowGlyph;
        else if (accel_key == Qt::Key_Left)
            *key = kMenuLeftArrowGlyph;
        else if (accel_key == Qt::Key_Right)
            *key = kMenuRightArrowGlyph;
        else if (accel_key == Qt::Key_CapsLock)
            *key = kMenuCapsLockGlyph;
        else if (accel_key >= Qt::Key_F1 && accel_key <= Qt::Key_F15)
            *key = (accel_key - Qt::Key_F1) + kMenuF1Glyph;
        else if (accel_key == Qt::Key_Home)
            *key = kMenuNorthwestArrowGlyph;
        else if (accel_key == Qt::Key_End)
            *key = kMenuSoutheastArrowGlyph;
    }
}
#else // Cocoa
static inline void syncNSMenuItemVisiblity(NSMenuItem *menuItem, bool actionVisibility)
{
    [menuItem setHidden:NO];
    [menuItem setHidden:YES];
    [menuItem setHidden:!actionVisibility];
}

static inline void syncNSMenuItemEnabled(NSMenuItem *menuItem, bool enabled)
{
    [menuItem setEnabled:NO];
    [menuItem setEnabled:YES];
    [menuItem setEnabled:enabled];
}

static inline void syncMenuBarItemsVisiblity(const QMenuBarPrivate::QMacMenuBarPrivate *mac_menubar)
{
    const QList<QMacMenuAction *> &menubarActions = mac_menubar->actionItems;
    for (int i = 0; i < menubarActions.size(); ++i) {
        const QMacMenuAction *action = menubarActions.at(i);
        syncNSMenuItemVisiblity(action->menuItem, actualMenuItemVisibility(mac_menubar, action));
    }
}

static inline QT_MANGLE_NAMESPACE(QCocoaMenuLoader) *getMenuLoader()
{
    return [NSApp QT_MANGLE_NAMESPACE(qt_qcocoamenuLoader)];
}

static NSMenuItem *createNSMenuItem(const QString &title)
{
    NSMenuItem *item = [[NSMenuItem alloc] 
                         initWithTitle:qt_mac_QStringToNSString(title)
                         action:@selector(qtDispatcherToQAction:) keyEquivalent:@""];
    [item setTarget:nil];
    return item;
}
#endif



// helper that recurses into a menu structure and en/dis-ables them
void qt_mac_set_modal_state_helper_recursive(OSMenuRef menu, OSMenuRef merge, bool on)
{
#ifndef QT_MAC_USE_COCOA
    for (int i = 0; i < CountMenuItems(menu); i++) {
        OSMenuRef submenu;
        GetMenuItemHierarchicalMenu(menu, i+1, &submenu);
        if (submenu != merge) {
            if (submenu)
                qt_mac_set_modal_state_helper_recursive(submenu, merge, on);
            if (on)
                DisableMenuItem(submenu, 0);
            else
                EnableMenuItem(submenu, 0);
        }
    }
#else
    bool modalWindowOnScreen = qApp->activeModalWidget() != 0;
    for (NSMenuItem *item in [menu itemArray]) {
        OSMenuRef submenu = [item submenu];
        if (submenu != merge) {
            if (submenu)
                qt_mac_set_modal_state_helper_recursive(submenu, merge, on);
            if (!on) {
                // The item should follow what the QAction has.
                if ([item tag]) {
                    QAction *action = reinterpret_cast<QAction *>([item tag]);
                    syncNSMenuItemEnabled(item, action->isEnabled());
                } else {
                    syncNSMenuItemEnabled(item, YES);
                }
                // We sneak in some extra code here to handle a menu problem:
                // If there is no window on screen, we cannot set 'nil' as
                // menu item target, because then cocoa will disable the item
                // (guess it assumes that there will be no first responder to
                // catch the trigger anyway?) OTOH, If we have a modal window,
                // then setting the menu loader as target will make cocoa not
                // deliver the trigger because the loader is then seen as modally
                // shaddowed). So either way there are shortcomings. Instead, we
                // decide the target as late as possible:
                [item setTarget:modalWindowOnScreen ? nil : getMenuLoader()];
            } else {
                syncNSMenuItemEnabled(item, NO);
            }
        }
    }
#endif
}

//toggling of modal state
static void qt_mac_set_modal_state(OSMenuRef menu, bool on)
{
#ifndef QT_MAC_USE_COCOA
    OSMenuRef merge = 0;
    GetMenuItemProperty(menu, 0, kMenuCreatorQt, kMenuPropertyMergeMenu,
            sizeof(merge), 0, &merge);

    qt_mac_set_modal_state_helper_recursive(menu, merge, on);

    UInt32 commands[] = { kHICommandQuit, kHICommandPreferences, kHICommandAbout, kHICommandAboutQt, 0 };
    for(int c = 0; commands[c]; c++) {
        bool enabled = !on;
        if (enabled) {
            QMacMenuAction *action = 0;
            GetMenuCommandProperty(menu, commands[c], kMenuCreatorQt, kMenuPropertyQAction,
                    sizeof(action), 0, &action);
            if (!action && merge) {
                GetMenuCommandProperty(merge, commands[c], kMenuCreatorQt, kMenuPropertyQAction,
                        sizeof(action), 0, &action);
                if (!action) {
                    QMenuMergeList *list = 0;
                    GetMenuItemProperty(merge, 0, kMenuCreatorQt, kMenuPropertyMergeList,
                            sizeof(list), 0, &list);
                    for(int i = 0; list && i < list->size(); ++i) {
                        QMenuMergeItem item = list->at(i);
                        if (item.command == commands[c] && item.action) {
                            action = item.action;
                            break;
                        }
                    }
                }
            }

            if (!action) {
                if (commands[c] != kHICommandQuit)
                    enabled = false;
            } else {
                enabled = action->action ? action->action->isEnabled() : 0;
            }
        }
        qt_mac_command_set_enabled(menu, commands[c], enabled);
    }
#else
    OSMenuRef merge = QMenuPrivate::mergeMenuHash.value(menu);
    qt_mac_set_modal_state_helper_recursive(menu, merge, on);
    // I'm ignoring the special items now, since they should get handled via a syncAction()
#endif
}

bool qt_mac_menubar_is_open()
{
    return qt_mac_menus_open_count > 0;
}

QMacMenuAction::~QMacMenuAction()
{
#ifdef QT_MAC_USE_COCOA
    [menu release];
    // Update the menu item if this action still owns it. For some items
    // (like 'Quit') ownership will be transferred between all menu bars...
    if (action && action.data() == reinterpret_cast<QAction *>([menuItem tag])) {
        QAction::MenuRole role = action->menuRole();
        // Check if the item is owned by Qt, and should be hidden to keep it from causing
        // problems. Do it for everything but the quit menu item since that should always
        // be visible.
        if (role > QAction::ApplicationSpecificRole && role < QAction::QuitRole) {
            [menuItem setHidden:YES];
        } else if (role == QAction::TextHeuristicRole
                   && menuItem != [getMenuLoader() quitMenuItem]) {
            [menuItem setHidden:YES];
        }
        [menuItem setTag:nil];
    }
    [menuItem release];
#endif
}

#ifndef QT_MAC_USE_COCOA
static MenuCommand qt_mac_menu_merge_action(MenuRef merge, QMacMenuAction *action)
#else
static NSMenuItem *qt_mac_menu_merge_action(OSMenuRef merge, QMacMenuAction *action)
#endif
{
    if (qt_mac_no_menubar_merge || action->action->menu() || action->action->isSeparator()
            || action->action->menuRole() == QAction::NoRole)
        return 0;

    QString t = qt_mac_removeMnemonics(action->action->text().toLower());
    int st = t.lastIndexOf(QLatin1Char('\t'));
    if (st != -1)
        t.remove(st, t.length()-st);
    t.replace(QRegExp(QString::fromLatin1("\\.*$")), QLatin1String("")); //no ellipses
    //now the fun part
#ifndef QT_MAC_USE_COCOA
    MenuCommand ret = 0;
#else
    NSMenuItem *ret = 0;
    QT_MANGLE_NAMESPACE(QCocoaMenuLoader) *loader = getMenuLoader();
#endif
    switch (action->action->menuRole()) {
    case QAction::NoRole:
        ret = 0;
        break;
    case QAction::ApplicationSpecificRole:
#ifndef QT_MAC_USE_COCOA
        {
            QMenuMergeList *list = 0;
            if (GetMenuItemProperty(merge, 0, kMenuCreatorQt, kMenuPropertyMergeList,
                        sizeof(list), 0, &list) == noErr && list) {
                MenuCommand lastCustom = kHICommandCustomMerge;
                for(int i = 0; i < list->size(); ++i) {
                    QMenuMergeItem item = list->at(i);
                    if (item.command == lastCustom)
                        ++lastCustom;
                }
                ret = lastCustom;
            } else {
                // The list hasn't been created, so, must be the first one.
                ret = kHICommandCustomMerge;
            }
        }
#else
        ret = [loader appSpecificMenuItem];
#endif
        break;
    case QAction::AboutRole:
#ifndef QT_MAC_USE_COCOA
        ret = kHICommandAbout;
#else
        ret = [loader aboutMenuItem];
#endif
        break;
    case QAction::AboutQtRole:
#ifndef QT_MAC_USE_COCOA
        ret = kHICommandAboutQt;
#else
        ret = [loader aboutQtMenuItem];
#endif
        break;
    case QAction::QuitRole:
#ifndef QT_MAC_USE_COCOA
        ret = kHICommandQuit;
#else
        ret = [loader quitMenuItem];
#endif
        break;
    case QAction::PreferencesRole:
#ifndef QT_MAC_USE_COCOA
        ret = kHICommandPreferences;
#else
        ret = [loader preferencesMenuItem];
#endif
        break;
    case QAction::TextHeuristicRole: {
        QString aboutString = QMenuBar::tr("About").toLower();
        if (t.startsWith(aboutString) || t.endsWith(aboutString)) {
            if (t.indexOf(QRegExp(QString::fromLatin1("qt$"), Qt::CaseInsensitive)) == -1) {
#ifndef QT_MAC_USE_COCOA
                ret = kHICommandAbout;
#else
                ret = [loader aboutMenuItem];
#endif
            } else {
#ifndef QT_MAC_USE_COCOA
                ret = kHICommandAboutQt;
#else
                ret = [loader aboutQtMenuItem];
#endif
            }
        } else if (t.startsWith(QMenuBar::tr("Config").toLower())
                   || t.startsWith(QMenuBar::tr("Preference").toLower())
                   || t.startsWith(QMenuBar::tr("Options").toLower())
                   || t.startsWith(QMenuBar::tr("Setting").toLower())
                   || t.startsWith(QMenuBar::tr("Setup").toLower())) {
#ifndef QT_MAC_USE_COCOA
            ret = kHICommandPreferences;
#else
            ret = [loader preferencesMenuItem];
#endif
        } else if (t.startsWith(QMenuBar::tr("Quit").toLower())
                   || t.startsWith(QMenuBar::tr("Exit").toLower())) {
#ifndef QT_MAC_USE_COCOA
            ret = kHICommandQuit;
#else
            ret = [loader quitMenuItem];
#endif
        }
    }
        break;
    }

#ifndef QT_MAC_USE_COCOA
    QMenuMergeList *list = 0;
    if (GetMenuItemProperty(merge, 0, kMenuCreatorQt, kMenuPropertyMergeList,
                           sizeof(list), 0, &list) == noErr && list) {
        for(int i = 0; i < list->size(); ++i) {
            QMenuMergeItem item = list->at(i);
            if (item.command == ret && item.action)
                return 0;
        }
    }

    QAction *cmd_action = 0;
    if (GetMenuCommandProperty(merge, ret, kMenuCreatorQt, kMenuPropertyQAction,
                              sizeof(cmd_action), 0, &cmd_action) == noErr && cmd_action)
        return 0; //already taken
#else
    if (QMenuMergeList *list = QMenuPrivate::mergeMenuItemsHash.value(merge)) {
        for(int i = 0; i < list->size(); ++i) {
            const QMenuMergeItem &item = list->at(i);
            if (item.menuItem == ret && item.action)
                return 0;
        }
    }

#endif
    return ret;
}

static QString qt_mac_menu_merge_text(QMacMenuAction *action)
{
    QString ret;
    extern QString qt_mac_applicationmenu_string(int type);
#ifdef QT_MAC_USE_COCOA
    QT_MANGLE_NAMESPACE(QCocoaMenuLoader) *loader = getMenuLoader();
#endif
    if (action->action->menuRole() == QAction::ApplicationSpecificRole)
        ret = action->action->text();
#ifndef QT_MAC_USE_COCOA
    else if (action->command == kHICommandAbout)
        ret = qt_mac_applicationmenu_string(6).arg(qAppName());
    else if (action->command == kHICommandAboutQt)
        ret = QMenuBar::tr("About Qt");
    else if (action->command == kHICommandPreferences)
        ret = qt_mac_applicationmenu_string(4);
    else if (action->command == kHICommandQuit)
        ret = qt_mac_applicationmenu_string(5).arg(qAppName());
#else
    else if (action->menuItem == [loader aboutMenuItem]) {
        ret = qt_mac_applicationmenu_string(6).arg(qAppName());
    } else if (action->menuItem == [loader aboutQtMenuItem]) {
        if (action->action->text() == QString("About Qt"))
            ret = QMenuBar::tr("About Qt");
        else
            ret = action->action->text();
    } else if (action->menuItem == [loader preferencesMenuItem]) {
        ret = qt_mac_applicationmenu_string(4);
    } else if (action->menuItem == [loader quitMenuItem]) {
        ret = qt_mac_applicationmenu_string(5).arg(qAppName());
    }
#endif
    return ret;
}

static QKeySequence qt_mac_menu_merge_accel(QMacMenuAction *action)
{
    QKeySequence ret;
#ifdef QT_MAC_USE_COCOA
    QT_MANGLE_NAMESPACE(QCocoaMenuLoader) *loader = getMenuLoader();
#endif
    if (action->action->menuRole() == QAction::ApplicationSpecificRole)
        ret = action->action->shortcut();
#ifndef QT_MAC_USE_COCOA
    else if (action->command == kHICommandPreferences)
        ret = QKeySequence(QKeySequence::Preferences);
    else if (action->command == kHICommandQuit)
        ret = QKeySequence(QKeySequence::Quit);
#else
    else if (action->menuItem == [loader preferencesMenuItem])
        ret = QKeySequence(QKeySequence::Preferences);
    else if (action->menuItem == [loader quitMenuItem])
        ret = QKeySequence(QKeySequence::Quit);
#endif
    return ret;
}

void Q_GUI_EXPORT qt_mac_set_menubar_icons(bool b)
{ QApplication::instance()->setAttribute(Qt::AA_DontShowIconsInMenus, !b); }
void Q_GUI_EXPORT qt_mac_set_native_menubar(bool b)
{  QApplication::instance()->setAttribute(Qt::AA_DontUseNativeMenuBar, !b); }
void Q_GUI_EXPORT qt_mac_set_menubar_merge(bool b) { qt_mac_no_menubar_merge = !b; }

/*****************************************************************************
  QMenu bindings
 *****************************************************************************/
QMenuPrivate::QMacMenuPrivate::QMacMenuPrivate() : menu(0)
{
}

QMenuPrivate::QMacMenuPrivate::~QMacMenuPrivate()
{
#ifndef QT_MAC_USE_COCOA
    for(QList<QMacMenuAction*>::Iterator it = actionItems.begin(); it != actionItems.end(); ++it) {
        QMacMenuAction *action = (*it);
        RemoveMenuCommandProperty(action->menu, action->command, kMenuCreatorQt, kMenuPropertyQAction);
        if (action->merged) {
            QMenuMergeList *list = 0;
            GetMenuItemProperty(action->menu, 0, kMenuCreatorQt, kMenuPropertyMergeList,
                                sizeof(list), 0, &list);
            for(int i = 0; list && i < list->size(); ) {
                QMenuMergeItem item = list->at(i);
                if (item.action == action)
                    list->removeAt(i);
                else
                    ++i;
            }
        }
        delete action;
    }
    if (menu) {
        EventHandlerHash::iterator it = menu_eventHandlers_hash()->find(menu);
        while (it != menu_eventHandlers_hash()->end() && it.key() == menu) {
            RemoveEventHandler(it.value());
            ++it;
        }
        menu_eventHandlers_hash()->remove(menu);
        ReleaseMenu(menu);
    }
#else
    QMacCocoaAutoReleasePool pool;
    while (actionItems.size()) {
        QMacMenuAction *action = actionItems.takeFirst();
        if (QMenuMergeList *list = mergeMenuItemsHash.value(action->menu)) {
            int i = 0;
            while (i < list->size()) {
                const QMenuMergeItem &item = list->at(i);
                if (item.action == action)
                    list->removeAt(i);
                else
                    ++i;
            }
        }
        delete action;
    }
    mergeMenuHash.remove(menu);
    mergeMenuItemsHash.remove(menu);
    [menu release];
#endif
}

void
QMenuPrivate::QMacMenuPrivate::addAction(QAction *a, QMacMenuAction *before, QMenuPrivate *qmenu)
{
    QMacMenuAction *action = new QMacMenuAction;
    action->action = a;
    action->ignore_accel = 0;
    action->merged = 0;
    action->menu = 0;
#ifndef QT_MAC_USE_COCOA
    action->command = qt_mac_menu_static_cmd_id++;
#endif
    addAction(action, before, qmenu);
}

void
QMenuPrivate::QMacMenuPrivate::addAction(QMacMenuAction *action, QMacMenuAction *before, QMenuPrivate *qmenu)
{
#ifdef QT_MAC_USE_COCOA
    QMacCocoaAutoReleasePool pool;
    Q_UNUSED(qmenu);
#endif
    if (!action)
        return;
    int before_index = actionItems.indexOf(before);
    if (before_index < 0) {
        before = 0;
        before_index = actionItems.size();
    }
    actionItems.insert(before_index, action);

#ifndef QT_MAC_USE_COCOA
    int index = qt_mac_menu_find_action(menu, action);
#else
    [menu retain];
    [action->menu release];
#endif
    action->menu = menu;

    /* When the action is considered a mergable action it
       will stay that way, until removed.. */
    if (!qt_mac_no_menubar_merge) {
#ifndef QT_MAC_USE_COCOA
        MenuRef merge = 0;
        GetMenuItemProperty(menu, 0, kMenuCreatorQt, kMenuPropertyMergeMenu,
                            sizeof(merge), 0, &merge);
#else
        OSMenuRef merge = QMenuPrivate::mergeMenuHash.value(menu);
#endif
        if (merge) {
#ifndef QT_MAC_USE_COCOA
            if (MenuCommand cmd = qt_mac_menu_merge_action(merge, action)) {
                action->merged = 1;
                action->menu = merge;
                action->command = cmd;
                if (qt_mac_auto_apple_menu(cmd))
                    index = 0; //no need

                QMenuMergeList *list = 0;
                if (GetMenuItemProperty(merge, 0, kMenuCreatorQt, kMenuPropertyMergeList,
                                       sizeof(list), 0, &list) != noErr || !list) {
                    list = new QMenuMergeList;
                    SetMenuItemProperty(merge, 0, kMenuCreatorQt, kMenuPropertyMergeList,
                                        sizeof(list), &list);
                }
                list->append(QMenuMergeItem(cmd, action));
            }
#else
            if (NSMenuItem *cmd = qt_mac_menu_merge_action(merge, action)) {
                action->merged = 1;
                [merge retain];
                [action->menu release];
                action->menu = merge;
                [cmd retain];
                [cmd setAction:@selector(qtDispatcherToQAction:)];
                [cmd setTarget:nil];
                [action->menuItem release];
                action->menuItem = cmd;
                QMenuMergeList *list = QMenuPrivate::mergeMenuItemsHash.value(merge);
                if (!list) {
                    list = new QMenuMergeList;
                    QMenuPrivate::mergeMenuItemsHash.insert(merge, list);
                }
                list->append(QMenuMergeItem(cmd, action));
            }
#endif
        }
    }

#ifdef QT_MAC_USE_COCOA
    NSMenuItem *newItem = action->menuItem;
#endif
    if (
#ifndef QT_MAC_USE_COCOA
        index == -1
#else
        newItem == 0
#endif
       ) {
#ifndef QT_MAC_USE_COCOA
        index = before_index;
        MenuItemAttributes attr = kMenuItemAttrAutoRepeat;
#else
        newItem = createNSMenuItem(action->action->text());
        action->menuItem = newItem;
#endif
        if (before) {
#ifndef QT_MAC_USE_COCOA
            InsertMenuItemTextWithCFString(action->menu, 0, qMax(before_index, 0), attr, action->command);
#else
            [menu insertItem:newItem atIndex:qMax(before_index, 0)];
#endif
        } else {
#ifndef QT_MAC_USE_COCOA
            // Append the menu item to the menu. If it is a kHICommandAbout or a kHICommandAboutQt append
            // a separator also (to get a separator above "Preferences"), but make sure that we don't
            // add separators between two "about" items.

            // Build a set of all commands that could possibly be before the separator.
            QSet<MenuCommand> mergedItems;
            mergedItems.insert(kHICommandAbout);
            mergedItems.insert(kHICommandAboutQt);
            mergedItems.insert(kHICommandCustomMerge);

            QMenuMergeList *list = 0;
            if (GetMenuItemProperty(action->menu, 0, kMenuCreatorQt, kMenuPropertyMergeList,
                        sizeof(list), 0, &list) == noErr && list) {
                for (int i = 0; i < list->size(); ++i) {
                    MenuCommand command = list->at(i).command;
                    if (command > kHICommandCustomMerge) {
                        mergedItems.insert(command);
                    }
                }
            }

            const int itemCount = CountMenuItems(action->menu);
            MenuItemAttributes testattr;
            GetMenuItemAttributes(action->menu, itemCount , &testattr);
            if (mergedItems.contains(action->command)
                 && (testattr & kMenuItemAttrSeparator)) {
                InsertMenuItemTextWithCFString(action->menu, 0, qMax(itemCount - 1, 0), attr, action->command);
                index = itemCount;
            } else {
                MenuItemIndex tmpIndex;
                AppendMenuItemTextWithCFString(action->menu, 0, attr, action->command, &tmpIndex);
                index = tmpIndex;
                if (mergedItems.contains(action->command))
                    AppendMenuItemTextWithCFString(action->menu, 0, kMenuItemAttrSeparator, 0, &tmpIndex);
            }
#else
            [menu addItem:newItem];
#endif
        }

        QWidget *widget = qmenu ? qmenu->widgetItems.value(action->action) : 0;
        if (widget) {
#ifndef QT_MAC_USE_COCOA
            ChangeMenuAttributes(action->menu, kMenuAttrDoNotCacheImage, 0);
            attr = kMenuItemAttrCustomDraw;
            SetMenuItemProperty(action->menu, index, kMenuCreatorQt, kMenuPropertyWidgetActionWidget,
                                sizeof(QWidget *), &widget);
            HIViewRef content;
            HIMenuGetContentView(action->menu, kThemeMenuTypePullDown, &content);

            EventHandlerRef eventHandlerRef;
            InstallMenuEventHandler(action->menu, qt_mac_widget_in_menu_eventHandler,
                                    GetEventTypeCount(widget_in_menu_events),
                                    widget_in_menu_events, 0, &eventHandlerRef);
            menu_eventHandlers_hash()->insert(action->menu, eventHandlerRef);

            QWidget *menuWidget = 0;
            GetMenuItemProperty(action->menu, 0, kMenuCreatorQt, kMenuPropertyWidgetMenu,
                                sizeof(menuWidget), 0, &menuWidget);
            if(!menuWidget) {
                menuWidget = new QMacNativeWidget(content);
                SetMenuItemProperty(menu, 0, kMenuCreatorQt, kMenuPropertyWidgetMenu,
                                    sizeof(menuWidget), &menuWidget);
                menuWidget->show();
            }
            widget->setParent(menuWidget);
#else
            QMacNativeWidget *container = new QMacNativeWidget(0);
            container->resize(widget->sizeHint());
            widget->setAttribute(Qt::WA_LayoutUsesWidgetRect);
            widget->setParent(container);

            NSView *containerView = qt_mac_nativeview_for(container);
            [containerView setAutoresizesSubviews:YES];
            [containerView setAutoresizingMask:NSViewWidthSizable];
            [qt_mac_nativeview_for(widget) setAutoresizingMask:NSViewWidthSizable];

            [newItem setView:containerView];
            container->show();
#endif
            widget->show();
        }

    } else {
#ifndef QT_MAC_USE_COCOA
        qt_mac_command_set_enabled(action->menu, action->command, !QApplicationPrivate::modalState());
#else
        [newItem setEnabled:!QApplicationPrivate::modalState()];
#endif
    }
#ifndef QT_MAC_USE_COCOA
    SetMenuCommandProperty(action->menu, action->command, kMenuCreatorQt, kMenuPropertyQAction,
                           sizeof(action), &action);
#else
    [newItem setTag:long(static_cast<QAction *>(action->action))];
#endif
    syncAction(action);
}

// return an autoreleased string given a QKeySequence (currently only looks at the first one).
NSString *keySequenceToKeyEqivalent(const QKeySequence &accel)
{
    quint32 accel_key = (accel[0] & ~(Qt::MODIFIER_MASK | Qt::UNICODE_ACCEL));
    extern QChar qtKey2CocoaKey(Qt::Key key);
    QChar cocoa_key = qtKey2CocoaKey(Qt::Key(accel_key));
    if (cocoa_key.isNull())
        cocoa_key = QChar(accel_key).toLower().unicode();
    return [NSString stringWithCharacters:&cocoa_key.unicode() length:1];
}

// return the cocoa modifier mask for the QKeySequence (currently only looks at the first one).
NSUInteger keySequenceModifierMask(const QKeySequence &accel)
{
    return constructModifierMask(accel[0]);
}

void
QMenuPrivate::QMacMenuPrivate::syncAction(QMacMenuAction *action)
{
    if (!action)
        return;

#ifndef QT_MAC_USE_COCOA
    const int index = qt_mac_menu_find_action(action->menu, action);
    if (index == -1)
        return;
#else
    NSMenuItem *item = action->menuItem;
    if (!item)
        return;
#endif

#ifndef QT_MAC_USE_COCOA
    if (!action->action->isVisible()) {
        ChangeMenuItemAttributes(action->menu, index, kMenuItemAttrHidden, 0);
        return;
    }
    ChangeMenuItemAttributes(action->menu, index, 0, kMenuItemAttrHidden);
#else
    QMacCocoaAutoReleasePool pool;
    NSMenu *menu = [item menu];
    bool actionVisible = action->action->isVisible();
    [item setHidden:!actionVisible];
    if (!actionVisible)
        return;
#endif

#ifndef QT_MAC_USE_COCOA
    if (action->action->isSeparator()) {
        ChangeMenuItemAttributes(action->menu, index, kMenuItemAttrSeparator, 0);
        return;
    }
    ChangeMenuItemAttributes(action->menu, index, 0, kMenuItemAttrSeparator);
#else
    int itemIndex = [menu indexOfItem:item];
    Q_ASSERT(itemIndex != -1);
    if (action->action->isSeparator()) {
        action->menuItem = [NSMenuItem separatorItem];
        [action->menuItem retain];
        [menu insertItem: action->menuItem atIndex:itemIndex];
        [menu removeItem:item];
        [item release];
        item = action->menuItem;
        return;
    } else if ([item isSeparatorItem]) {
        // I'm no longer a separator...
        action->menuItem = createNSMenuItem(action->action->text());
        [menu insertItem:action->menuItem atIndex:itemIndex];
        [menu removeItem:item];
        [item release];
        item = action->menuItem;
    }
#endif

    //find text (and accel)
    action->ignore_accel = 0;
    QString text = action->action->text();
    QKeySequence accel = action->action->shortcut();
    {
        int st = text.lastIndexOf(QLatin1Char('\t'));
        if (st != -1) {
            action->ignore_accel = 1;
            accel = QKeySequence(text.right(text.length()-(st+1)));
            text.remove(st, text.length()-st);
        }
    }
    {
        QString cmd_text = qt_mac_menu_merge_text(action);
        if (!cmd_text.isEmpty()) {
            text = cmd_text;
            accel = qt_mac_menu_merge_accel(action);
        }
    }
    // Show multiple key sequences as part of the menu text.
    if (accel.count() > 1)
        text += QLatin1String(" (") + accel.toString(QKeySequence::NativeText) + QLatin1String(")");

    QString finalString = qt_mac_removeMnemonics(text);

#ifndef QT_MAC_USE_COCOA
    MenuItemDataRec data;
    memset(&data, '\0', sizeof(data));

    //Carbon text
    data.whichData |= kMenuItemDataCFString;
    QCFString cfstring(finalString);  // Hold the reference to the end of the function.
    data.cfText = cfstring;

    // Carbon enabled
    data.whichData |= kMenuItemDataEnabled;
    data.enabled = action->action->isEnabled();
    // Carbon icon
    data.whichData |= kMenuItemDataIconHandle;
    if (!action->action->icon().isNull()
            && action->action->isIconVisibleInMenu()) {
        data.iconType = kMenuIconRefType;
        data.iconHandle = (Handle)qt_mac_create_iconref(action->action->icon().pixmap(16, QIcon::Normal));
    } else {
        data.iconType = kMenuNoIcon;
    }
    if (action->action->font().resolve()) { // Carbon font
        if (action->action->font().bold())
            data.style |= bold;
        if (action->action->font().underline())
            data.style |= underline;
        if (action->action->font().italic())
            data.style |= italic;
        if (data.style)
            data.whichData |= kMenuItemDataStyle;
        data.whichData |= kMenuItemDataFontID;
        data.fontID = action->action->font().macFontID();
    }
#else
    // Cocoa Font and title
    if (action->action->font().resolve()) {
        const QFont &actionFont = action->action->font();
        NSFont *customMenuFont = [NSFont fontWithName:qt_mac_QStringToNSString(actionFont.family())
                                  size:actionFont.pointSize()];
        NSArray *keys = [NSArray arrayWithObjects:NSFontAttributeName, nil];
        NSArray *objects = [NSArray arrayWithObjects:customMenuFont, nil];
        NSDictionary *attributes = [NSDictionary dictionaryWithObjects:objects forKeys:keys];
        NSAttributedString *str = [[[NSAttributedString alloc] initWithString:qt_mac_QStringToNSString(finalString)
                                 attributes:attributes] autorelease];
       [item setAttributedTitle: str];
    } else {
        [item setTitle: qt_mac_QStringToNSString(finalString)];
    }

    if (action->action->menuRole() == QAction::AboutRole || action->action->menuRole() == QAction::QuitRole)
        [item setTitle:qt_mac_QStringToNSString(text)];
    else
        [item setTitle:qt_mac_QStringToNSString(qt_mac_removeMnemonics(text))];

    // Cocoa Enabled
    [item setEnabled: action->action->isEnabled()];

    // Cocoa icon
    NSImage *nsimage = 0;
    if (!action->action->icon().isNull() && action->action->isIconVisibleInMenu()) {
        nsimage = static_cast<NSImage *>(qt_mac_create_nsimage(action->action->icon().pixmap(16, QIcon::Normal)));
    }
    [item setImage:nsimage];
    [nsimage release];
#endif

    if (action->action->menu()) { //submenu
#ifndef QT_MAC_USE_COCOA
        data.whichData |= kMenuItemDataSubmenuHandle;
        data.submenuHandle = action->action->menu()->macMenu();
        QWidget *caused = 0;
        GetMenuItemProperty(action->menu, 0, kMenuCreatorQt, kMenuPropertyQWidget, sizeof(caused), 0, &caused);
        SetMenuItemProperty(data.submenuHandle, 0, kMenuCreatorQt, kMenuPropertyCausedQWidget, sizeof(caused), &caused);
#else
        NSMenu *subMenu  = static_cast<NSMenu *>(action->action->menu()->macMenu());
        if ([subMenu supermenu] && [subMenu supermenu] != [item menu]) {
            // The menu is already a sub-menu of another one. Cocoa will throw an exception,
            // in such cases. For the time being, a new QMenu with same set of actions is the
            // only workaround.
            action->action->setEnabled(false);
        } else {
            [item setSubmenu:subMenu];
        }
        [item setAction:nil];
#endif
    } else { //respect some other items
#ifndef QT_MAC_USE_COCOA
        //shortcuts (say we are setting them all so that we can also clear them).
        data.whichData |= kMenuItemDataCmdKey;
        data.whichData |= kMenuItemDataCmdKeyModifiers;
        data.whichData |= kMenuItemDataCmdKeyGlyph;
        if (accel.count() == 1) {
            qt_mac_get_accel(accel[0], (quint32*)&data.cmdKeyModifiers, (quint32*)&data.cmdKeyGlyph);
            if (data.cmdKeyGlyph == 0)
                data.cmdKey = (UniChar)accel[0];
        }
#else
        [item setSubmenu:0];
        if ([item action] == nil)
            [item setAction:@selector(qtDispatcherToQAction:)];
        // No key equivalent set for multiple key QKeySequence.
        if (accel.count() == 1) {
            [item setKeyEquivalent:keySequenceToKeyEqivalent(accel)];
            [item setKeyEquivalentModifierMask:keySequenceModifierMask(accel)];
        } else {
            [item setKeyEquivalent:@""];
            [item setKeyEquivalentModifierMask:NSCommandKeyMask];
        }
#endif
    }
#ifndef QT_MAC_USE_COCOA
    //mark glyph
    data.whichData |= kMenuItemDataMark;
    if (action->action->isChecked()) {
#if 0
        if (action->action->actionGroup() &&
           action->action->actionGroup()->isExclusive())
            data.mark = diamondMark;
        else
#endif
            data.mark = checkMark;
    } else {
        data.mark = noMark;
    }

    //actually set it
    SetMenuItemData(action->menu, action->command, true, &data);

    // Free up memory
    if (data.iconHandle)
        ReleaseIconRef(IconRef(data.iconHandle));
#else
    //mark glyph
    [item setState:action->action->isChecked() ?  NSOnState : NSOffState];
#endif
}

void
QMenuPrivate::QMacMenuPrivate::removeAction(QMacMenuAction *action)
{
    if (!action)
        return;
#ifndef QT_MAC_USE_COCOA
    if (action->command == kHICommandQuit || action->command == kHICommandPreferences)
        qt_mac_command_set_enabled(action->menu, action->command, false);
    else
        DeleteMenuItem(action->menu, qt_mac_menu_find_action(action->menu, action));
#else
    QMacCocoaAutoReleasePool pool;
    if (action->merged) {
        if (reinterpret_cast<QAction *>([action->menuItem tag]) == action->action) {
            QT_MANGLE_NAMESPACE(QCocoaMenuLoader) *loader = getMenuLoader();
            [action->menuItem setEnabled:false];
            if (action->menuItem != [loader quitMenuItem]
                && action->menuItem != [loader preferencesMenuItem]) {
                [[action->menuItem menu] removeItem:action->menuItem];
            }
            if (QMenuMergeList *list = mergeMenuItemsHash.value(action->menu)) {
                int i = 0;
                while (i < list->size()) {
                    const QMenuMergeItem &item = list->at(i);
                    if (item.action == action)
                        list->removeAt(i);
                    else
                        ++i;
                }
            }
        }
    } else {
        [[action->menuItem menu] removeItem:action->menuItem];
    }
#endif
    actionItems.removeAll(action);
}

OSMenuRef
QMenuPrivate::macMenu(OSMenuRef merge)
{
    Q_UNUSED(merge);
    Q_Q(QMenu);
    if (mac_menu && mac_menu->menu)
        return mac_menu->menu;
    if (!mac_menu)
        mac_menu = new QMacMenuPrivate;
    mac_menu->menu = qt_mac_create_menu(q);
    if (merge) {
#ifndef QT_MAC_USE_COCOA
        SetMenuItemProperty(mac_menu->menu, 0, kMenuCreatorQt, kMenuPropertyMergeMenu, sizeof(merge), &merge);
#else
        mergeMenuHash.insert(mac_menu->menu, merge);
#endif
    }
    QList<QAction*> items = q->actions();
    for(int i = 0; i < items.count(); i++)
        mac_menu->addAction(items[i], 0, this);
    syncSeparatorsCollapsible(collapsibleSeparators);
    return mac_menu->menu;
}

/*!
  \internal
*/
void
QMenuPrivate::syncSeparatorsCollapsible(bool collapse)
{
#ifndef QT_MAC_USE_COCOA
    if (collapse)
        ChangeMenuAttributes(mac_menu->menu, kMenuAttrCondenseSeparators, 0);
    else
        ChangeMenuAttributes(mac_menu->menu, 0, kMenuAttrCondenseSeparators);
#else
    qt_mac_menu_collapseSeparators(mac_menu->menu, collapse);
#endif
}



#ifndef QT_MAC_USE_COCOA
/*!
  \internal
*/
void QMenuPrivate::setMacMenuEnabled(bool enable)
{
    if (!macMenu(0))
        return;

    QMacCocoaAutoReleasePool pool;
    if (enable) {
        for (int i = 0; i < mac_menu->actionItems.count(); ++i) {
            QMacMenuAction *menuItem = mac_menu->actionItems.at(i);
            if (menuItem && menuItem->action && menuItem->action->isEnabled()) {
                // Only enable those items which contains an enabled QAction.
                // i == 0 -> the menu itself, hence i + 1 for items.
                EnableMenuItem(mac_menu->menu, i + 1);
            }
        }
    } else {
        DisableAllMenuItems(mac_menu->menu);
    }
}
#endif

/*!
    \internal

    This function will return the OSMenuRef used to create the native menu bar
    bindings.

    If Qt is built against Carbon, the OSMenuRef is a MenuRef that can be used
    with Carbon's Menu Manager API.

    If Qt is built against Cocoa, the OSMenuRef is a NSMenu pointer.

    \warning This function is not portable.

    \sa QMenuBar::macMenu()
*/
OSMenuRef QMenu::macMenu(OSMenuRef merge) { return d_func()->macMenu(merge); }

/*****************************************************************************
  QMenuBar bindings
 *****************************************************************************/
typedef QHash<QWidget *, QMenuBar *> MenuBarHash;
Q_GLOBAL_STATIC(MenuBarHash, menubars)
static QMenuBar *fallback = 0;
QMenuBarPrivate::QMacMenuBarPrivate::QMacMenuBarPrivate() : menu(0), apple_menu(0)
{
}

QMenuBarPrivate::QMacMenuBarPrivate::~QMacMenuBarPrivate()
{
    for(QList<QMacMenuAction*>::Iterator it = actionItems.begin(); it != actionItems.end(); ++it)
        delete (*it);
#ifndef QT_MAC_USE_COCOA
    if (apple_menu) {
        QMenuMergeList *list = 0;
        GetMenuItemProperty(apple_menu, 0, kMenuCreatorQt, kMenuPropertyMergeList,
                            sizeof(list), 0, &list);
        if (list) {
            RemoveMenuItemProperty(apple_menu, 0, kMenuCreatorQt, kMenuPropertyMergeList);
            delete list;
        }
        ReleaseMenu(apple_menu);
    }
    if (menu)
        ReleaseMenu(menu);
#else
    [apple_menu release];
    [menu release];
#endif
}

void
QMenuBarPrivate::QMacMenuBarPrivate::addAction(QAction *a, QAction *before)
{
    if (a->isSeparator() || !menu)
        return;
    QMacMenuAction *action = new QMacMenuAction;
    action->action = a;
    action->ignore_accel = 1;
#ifndef QT_MAC_USE_COCOA
    action->command = qt_mac_menu_static_cmd_id++;
#endif
    addAction(action, findAction(before));
}

void
QMenuBarPrivate::QMacMenuBarPrivate::addAction(QMacMenuAction *action, QMacMenuAction *before)
{
    if (!action || !menu)
        return;

    int before_index = actionItems.indexOf(before);
    if (before_index < 0) {
        before = 0;
        before_index = actionItems.size();
    }
    actionItems.insert(before_index, action);

    MenuItemIndex index = actionItems.size()-1;

    action->menu = menu;
#ifdef QT_MAC_USE_COCOA
    QMacCocoaAutoReleasePool pool;
    [action->menu retain];
    NSMenuItem *newItem = createNSMenuItem(action->action->text());
    action->menuItem = newItem;
#endif
    if (before) {
#ifndef QT_MAC_USE_COCOA
        InsertMenuItemTextWithCFString(action->menu, 0, qMax(1, before_index+1), 0, action->command);
#else
        [menu insertItem:newItem atIndex:qMax(1, before_index + 1)];
#endif
        index = before_index;
    } else {
#ifndef QT_MAC_USE_COCOA
        AppendMenuItemTextWithCFString(action->menu, 0, 0, action->command, &index);
#else
        [menu addItem:newItem];
#endif
    }
#ifndef QT_MAC_USE_COCOA
    SetMenuItemProperty(action->menu, index, kMenuCreatorQt, kMenuPropertyQAction, sizeof(action),
                        &action);
#else
    [newItem setTag:long(static_cast<QAction *>(action->action))];
#endif
    syncAction(action);
}

void
QMenuBarPrivate::QMacMenuBarPrivate::syncAction(QMacMenuAction *action)
{
    if (!action || !menu)
        return;
#ifndef QT_MAC_USE_COCOA
    const int index = qt_mac_menu_find_action(action->menu, action);
#else
    QMacCocoaAutoReleasePool pool;
    NSMenuItem *item = action->menuItem;
#endif

    OSMenuRef submenu = 0;
    bool release_submenu = false;
    if (action->action->menu()) {
        if ((submenu = action->action->menu()->macMenu(apple_menu))) {
#ifndef QT_MAC_USE_COCOA
            QWidget *caused = 0;
            GetMenuItemProperty(action->menu, 0, kMenuCreatorQt, kMenuPropertyQWidget, sizeof(caused), 0, &caused);
            SetMenuItemProperty(submenu, 0, kMenuCreatorQt, kMenuPropertyCausedQWidget, sizeof(caused), &caused);
#else
            if ([submenu supermenu] && [submenu supermenu] != [item menu])
                return;
            else
                [item setSubmenu:submenu];
#endif
        }
#ifndef QT_MAC_USE_COCOA
    } else { // create a submenu to act as menu
        release_submenu = true;
        CreateNewMenu(0, 0, &submenu);
#endif
    }

    if (submenu) {
        bool visible = actualMenuItemVisibility(this, action);
#ifndef QT_MAC_USE_COCOA
        SetMenuItemHierarchicalMenu(action->menu, index, submenu);
        SetMenuTitleWithCFString(submenu, QCFString(qt_mac_removeMnemonics(action->action->text())));
        if (visible)
            ChangeMenuAttributes(submenu, 0, kMenuAttrHidden);
        else
            ChangeMenuAttributes(submenu, kMenuAttrHidden, 0);
#else
        [item setSubmenu: submenu];
        [submenu setTitle:qt_mac_QStringToNSString(qt_mac_removeMnemonics(action->action->text()))];
        syncNSMenuItemVisiblity(item, visible);
        syncNSMenuItemEnabled(item, action->action->isEnabled());
#endif
        if (release_submenu) { //no pointers to it
#ifndef QT_MAC_USE_COCOA
            ReleaseMenu(submenu);
#else
            [submenu release];
#endif
        }
    } else {
        qWarning("QMenu: No OSMenuRef created for popup menu");
    }
}

void
QMenuBarPrivate::QMacMenuBarPrivate::removeAction(QMacMenuAction *action)
{
    if (!action || !menu)
        return;
#ifndef QT_MAC_USE_COCOA
    DeleteMenuItem(action->menu, qt_mac_menu_find_action(action->menu, action));
#else
    QMacCocoaAutoReleasePool pool;
    [action->menu removeItem:action->menuItem];
#endif
    actionItems.removeAll(action);
}

bool QMenuBarPrivate::macWidgetHasNativeMenubar(QWidget *widget)
{
    // This function is different from q->isNativeMenuBar(), as
    // it returns true only if a native menu bar is actually
    // _created_.
    if (!widget)
        return false;
    return menubars()->contains(widget->window());
}

void
QMenuBarPrivate::macCreateMenuBar(QWidget *parent)
{
    Q_Q(QMenuBar);
    static int dontUseNativeMenuBar = -1;
    // We call the isNativeMenuBar function here
    // because that will make sure that local overrides
    // are dealt with correctly. q->isNativeMenuBar() will, if not
    // overridden, depend on the attribute Qt::AA_DontUseNativeMenuBar:
    bool qt_mac_no_native_menubar = !q->isNativeMenuBar();
    if (qt_mac_no_native_menubar == false && dontUseNativeMenuBar < 0) {
        // The menubar is set to be native. Let's check (one time only
        // for all menubars) if this is OK with the rest of the environment.
        // As a result, Qt::AA_DontUseNativeMenuBar is set. NB: the application
        // might still choose to not respect, or change, this flag.
        bool isPlugin = QApplication::testAttribute(Qt::AA_MacPluginApplication);
        bool environmentSaysNo = !qgetenv("QT_MAC_NO_NATIVE_MENUBAR").isEmpty();
        dontUseNativeMenuBar = isPlugin || environmentSaysNo;
        QApplication::instance()->setAttribute(Qt::AA_DontUseNativeMenuBar, dontUseNativeMenuBar);
        qt_mac_no_native_menubar = !q->isNativeMenuBar();
    }
    if (qt_mac_no_native_menubar == false) {
        // INVARIANT: Use native menubar.
        extern void qt_event_request_menubarupdate(); //qapplication_mac.cpp
        qt_event_request_menubarupdate();
        if (!parent && !fallback) {
            fallback = q;
            mac_menubar = new QMacMenuBarPrivate;
        } else if (parent && parent->isWindow()) {
            menubars()->insert(q->window(), q);
            mac_menubar = new QMacMenuBarPrivate;
        }
    }
}

void QMenuBarPrivate::macDestroyMenuBar()
{
    Q_Q(QMenuBar);
    QMacCocoaAutoReleasePool pool;
    if (fallback == q)
        fallback = 0;
    delete mac_menubar;
    QWidget *tlw = q->window();
    menubars()->remove(tlw);
    mac_menubar = 0;

    if (!qt_mac_current_menubar.qmenubar || qt_mac_current_menubar.qmenubar == q) {
#ifdef QT_MAC_USE_COCOA
        QT_MANGLE_NAMESPACE(QCocoaMenuLoader) *loader = getMenuLoader();
        [loader removeActionsFromAppMenu];
#else
        cancelAllMenuTracking();
#endif
        extern void qt_event_request_menubarupdate(); //qapplication_mac.cpp
        qt_event_request_menubarupdate();
    }
}

OSMenuRef QMenuBarPrivate::macMenu()
{
    Q_Q(QMenuBar);
    if (!q->isNativeMenuBar() || !mac_menubar) {
        return 0;
    } else if (!mac_menubar->menu) {
        mac_menubar->menu = qt_mac_create_menu(q);
#ifdef QT_MAC_USE_COCOA
        [mac_menubar->menu setAutoenablesItems:NO];
#endif
        ProcessSerialNumber mine, front;
        if (GetCurrentProcess(&mine) == noErr && GetFrontProcess(&front) == noErr) {
            if (!qt_mac_no_menubar_merge && !mac_menubar->apple_menu) {
                mac_menubar->apple_menu = qt_mac_create_menu(q);
#ifndef QT_MAC_USE_COCOA
                MenuItemIndex index;
                AppendMenuItemTextWithCFString(mac_menubar->menu, 0, 0, 0, &index);

                SetMenuTitleWithCFString(mac_menubar->apple_menu, QCFString(QString(QChar(0x14))));
                SetMenuItemHierarchicalMenu(mac_menubar->menu, index, mac_menubar->apple_menu);
                SetMenuItemProperty(mac_menubar->apple_menu, 0, kMenuCreatorQt, kMenuPropertyQWidget, sizeof(q), &q);
#else
                [mac_menubar->apple_menu setTitle:qt_mac_QStringToNSString(QString(QChar(0x14)))];
                NSMenuItem *apple_menuItem = [[NSMenuItem alloc] init];
                [apple_menuItem setSubmenu:mac_menubar->menu];
                [mac_menubar->apple_menu addItem:apple_menuItem];
                [apple_menuItem release];
#endif
            }
            if (mac_menubar->apple_menu) {
#ifndef QT_MAC_USE_COCOA
                SetMenuItemProperty(mac_menubar->menu, 0, kMenuCreatorQt, kMenuPropertyMergeMenu,
                                    sizeof(mac_menubar->apple_menu), &mac_menubar->apple_menu);
#else
                QMenuPrivate::mergeMenuHash.insert(mac_menubar->menu, mac_menubar->apple_menu);
#endif
            }
            QList<QAction*> items = q->actions();
            for(int i = 0; i < items.count(); i++)
                mac_menubar->addAction(items[i]);
        }
    }
    return mac_menubar->menu;
}

/*!
    \internal

    This function will return the OSMenuRef used to create the native menu bar
    bindings. This OSMenuRef is then set as the root menu for the Menu
    Manager.

    \warning This function is not portable.

    \sa QMenu::macMenu()
*/
OSMenuRef QMenuBar::macMenu() { return d_func()->macMenu(); }

/* !
    \internal
    Ancestor function that crosses windows (QWidget::isAncestorOf
    only considers widgets within the same window).
*/
static bool qt_mac_is_ancestor(QWidget* possibleAncestor, QWidget *child)
{
    if (!possibleAncestor)
        return false;

    QWidget * current = child->parentWidget();
    while (current != 0) {
        if (current == possibleAncestor)
            return true;
        current = current->parentWidget();
    }
    return false;
}

/* !
    \internal
    Returns true if the entries of menuBar should be disabled,
    based on the modality type of modalWidget.
*/
static bool qt_mac_should_disable_menu(QMenuBar *menuBar)
{
    QWidget *modalWidget = qApp->activeModalWidget();
    if (!modalWidget)
        return false;

    if (menuBar && menuBar == menubars()->value(modalWidget))
        // The menu bar is owned by the modal widget.
        // In that case we should enable it:
        return false;

    // When there is an application modal window on screen, the entries of
    // the menubar should be disabled. The exception in Qt is that if the
    // modal window is the only window on screen, then we enable the menu bar.
    QWidget *w = modalWidget;
    QWidgetList topLevelWidgets = QApplication::topLevelWidgets();
    while (w) {
        if (w->isVisible() && w->windowModality() == Qt::ApplicationModal) {
            for (int i=0; i<topLevelWidgets.size(); ++i) {
                QWidget *top = topLevelWidgets.at(i);
                if (w != top && top->isVisible()) {
                    // INVARIANT: we found another visible window
                    // on screen other than our modalWidget. We therefore
                    // disable the menu bar to follow normal modality logic:
                    return true;
                }
            }
            // INVARIANT: We have only one window on screen that happends
            // to be application modal. We choose to enable the menu bar
            // in that case to e.g. enable the quit menu item.
            return false;
        }
        w = w->parentWidget();
    }

    // INVARIANT: modalWidget is window modal. Disable menu entries
    // if the menu bar belongs to an ancestor of modalWidget. If menuBar
    // is nil, we understand it as the default menu bar set by the nib:
    return menuBar ? qt_mac_is_ancestor(menuBar->parentWidget(), modalWidget) : false;
}

static QWidget *findWindowThatShouldDisplayMenubar()
{
    QWidget *w = qApp->activeWindow();
    if (!w) {
        // We have no active window on screen. Try to
        // find a window from the list of top levels:
        QWidgetList tlws = QApplication::topLevelWidgets();
        for(int i = 0; i < tlws.size(); ++i) {
            QWidget *tlw = tlws.at(i);
            if ((tlw->isVisible() && tlw->windowType() != Qt::Tool &&
                tlw->windowType() != Qt::Popup)) {
                w = tlw;
                break;
            }
        }
    }
    return w;
}

static QMenuBar *findMenubarForWindow(QWidget *w)
{
    QMenuBar *mb = 0;
    if (w) {
        mb = menubars()->value(w);
#ifndef QT_NO_MAINWINDOW
        QDockWidget *dw = qobject_cast<QDockWidget *>(w);
        if (!mb && dw) {
            QMainWindow *mw = qobject_cast<QMainWindow *>(dw->parentWidget());
            if (mw && (mb = menubars()->value(mw)))
                w = mw;
        }
#endif
        while(w && !mb)
            mb = menubars()->value((w = w->parentWidget()));
    }

    if (!mb) {
        // We could not find a menu bar for the window. Lets
        // check if we have a global (parentless) menu bar instead:
        mb = fallback;
    }

    return mb;
}

void qt_mac_clear_menubar()
{
    if (QApplication::testAttribute(Qt::AA_MacPluginApplication))
        return;

#ifndef QT_MAC_USE_COCOA
    MenuRef clear_menu = 0;
    if (CreateNewMenu(0, 0, &clear_menu) == noErr) {
        SetRootMenu(clear_menu);
        ReleaseMenu(clear_menu);
    } else {
        qWarning("QMenu: Internal error at %s:%d", __FILE__, __LINE__);
    }
    ClearMenuBar();
    qt_mac_command_set_enabled(0, kHICommandPreferences, false);
    InvalMenuBar();
#else
    QMacCocoaAutoReleasePool pool;
    QT_MANGLE_NAMESPACE(QCocoaMenuLoader) *loader = getMenuLoader();
    NSMenu *menu = [loader menu];
    [loader ensureAppMenuInMenu:menu];
    [NSApp setMainMenu:menu];
    const bool modal = qt_mac_should_disable_menu(0);
    if (qt_mac_current_menubar.qmenubar || modal != qt_mac_current_menubar.modal)
        qt_mac_set_modal_state(menu, modal);
    qt_mac_current_menubar.qmenubar = 0;
    qt_mac_current_menubar.modal = modal;
#endif
}

/*!
  \internal

  This function will update the current menu bar and set it as the
  active menu bar in the Menu Manager.

  \warning This function is not portable.

  \sa QMenu::macMenu(), QMenuBar::macMenu()
*/
bool QMenuBar::macUpdateMenuBar()
{
#ifdef QT_MAC_USE_COCOA
    QMacCocoaAutoReleasePool pool;
    qt_cocoaPostMessage(getMenuLoader(), @selector(qtUpdateMenubar));
    return true;
#else
    return QMenuBarPrivate::macUpdateMenuBarImmediatly();
#endif
}

bool QMenuBarPrivate::macUpdateMenuBarImmediatly()
{
    bool ret = false;
    cancelAllMenuTracking();
    QWidget *w = findWindowThatShouldDisplayMenubar();
    QMenuBar *mb = findMenubarForWindow(w);
    extern bool qt_mac_app_fullscreen; //qapplication_mac.mm

    // We need to see if we are in full screen mode, if so we need to
    // switch the full screen mode to be able to show or hide the menubar.
    if(w && mb) {
        // This case means we are creating a menubar, check if full screen
        if(w->isFullScreen()) {
            // Ok, switch to showing the menubar when hovering over it.
            SetSystemUIMode(kUIModeAllHidden, kUIOptionAutoShowMenuBar);
            qt_mac_app_fullscreen = true;
        }
    } else if(w) {
        // Removing a menubar
        if(w->isFullScreen()) {
            // Ok, switch to not showing the menubar when hovering on it
            SetSystemUIMode(kUIModeAllHidden, 0);
            qt_mac_app_fullscreen = true;
        }
    }

    if (mb && mb->isNativeMenuBar()) {
        bool modal = QApplicationPrivate::modalState();
#ifdef QT_MAC_USE_COCOA
        QMacCocoaAutoReleasePool pool;
#endif
        if (OSMenuRef menu = mb->macMenu()) {
#ifndef QT_MAC_USE_COCOA
            SetRootMenu(menu);
#else
            QT_MANGLE_NAMESPACE(QCocoaMenuLoader) *loader = getMenuLoader();
            [loader ensureAppMenuInMenu:menu];
            [NSApp setMainMenu:menu];
            syncMenuBarItemsVisiblity(mb->d_func()->mac_menubar);

            if (OSMenuRef tmpMerge = QMenuPrivate::mergeMenuHash.value(menu)) {
                if (QMenuMergeList *mergeList
                        = QMenuPrivate::mergeMenuItemsHash.value(tmpMerge)) {
                    const int mergeListSize = mergeList->size();

                    for (int i = 0; i < mergeListSize; ++i) {
                        const QMenuMergeItem &mergeItem = mergeList->at(i);
                        // Ideally we would call QMenuPrivate::syncAction, but that requires finding
                        // the original QMen and likely doing more work than we need.
                        // For example, enabled is handled below.
                        [mergeItem.menuItem setTag:reinterpret_cast<long>(
                                                    static_cast<QAction *>(mergeItem.action->action))];
                        [mergeItem.menuItem setHidden:!(mergeItem.action->action->isVisible())];
                    }
                }
            }
#endif
            // Check if menu is modally shaddowed and should  be disabled:
            modal = qt_mac_should_disable_menu(mb);
            if (mb != qt_mac_current_menubar.qmenubar || modal != qt_mac_current_menubar.modal)
                qt_mac_set_modal_state(menu, modal);
        }
        qt_mac_current_menubar.qmenubar = mb;
        qt_mac_current_menubar.modal = modal;
        ret = true;
    } else if (qt_mac_current_menubar.qmenubar && qt_mac_current_menubar.qmenubar->isNativeMenuBar()) {
        // INVARIANT: The currently active menu bar (if any) is not native. But we do have a
        // native menu bar from before. So we need to decide whether or not is should be enabled:
        const bool modal = qt_mac_should_disable_menu(qt_mac_current_menubar.qmenubar);
        if (modal != qt_mac_current_menubar.modal) {
            ret = true;
            if (OSMenuRef menu = qt_mac_current_menubar.qmenubar->macMenu()) {
#ifndef QT_MAC_USE_COCOA
                SetRootMenu(menu);
#else
                QT_MANGLE_NAMESPACE(QCocoaMenuLoader) *loader = getMenuLoader();
                [loader ensureAppMenuInMenu:menu];
                [NSApp setMainMenu:menu];
                syncMenuBarItemsVisiblity(qt_mac_current_menubar.qmenubar->d_func()->mac_menubar);
#endif
                qt_mac_set_modal_state(menu, modal);
            }
            qt_mac_current_menubar.modal = modal;
        }
    }

    if (!ret) {
        qt_mac_clear_menubar();
    }
    return ret;
}

QHash<OSMenuRef, OSMenuRef> QMenuPrivate::mergeMenuHash;
QHash<OSMenuRef, QMenuMergeList*> QMenuPrivate::mergeMenuItemsHash;

bool QMenuPrivate::QMacMenuPrivate::merged(const QAction *action) const
{
#ifndef QT_MAC_USE_COCOA
    MenuRef merge = 0;
    GetMenuItemProperty(menu, 0, kMenuCreatorQt, kMenuPropertyMergeMenu,
            sizeof(merge), 0, &merge);
    if (merge) {
        QMenuMergeList *list = 0;
        if (GetMenuItemProperty(merge, 0, kMenuCreatorQt, kMenuPropertyMergeList,
                    sizeof(list), 0, &list) == noErr && list) {
            for(int i = 0; i < list->size(); ++i) {
                QMenuMergeItem item = list->at(i);
                if (item.action->action == action)
                    return true;
            }
        }
    }
#else
    if (OSMenuRef merge = mergeMenuHash.value(menu)) {
        if (QMenuMergeList *list = mergeMenuItemsHash.value(merge)) {
            for(int i = 0; i < list->size(); ++i) {
                const QMenuMergeItem &item = list->at(i);
                if (item.action->action == action)
                    return true;
            }
        }
    }
#endif
    return false;
}

//creation of the OSMenuRef
static OSMenuRef qt_mac_create_menu(QWidget *w)
{
    OSMenuRef ret;
#ifndef QT_MAC_USE_COCOA
    ret = 0;
    if (CreateNewMenu(0, 0, &ret) == noErr) {
        qt_mac_create_menu_event_handler();
        SetMenuItemProperty(ret, 0, kMenuCreatorQt, kMenuPropertyQWidget, sizeof(w), &w);

        // kEventMenuMatchKey is only sent to the menu itself and not to
        // the application, install a separate handler for that event.
        EventHandlerRef eventHandlerRef;
        InstallMenuEventHandler(ret, qt_mac_menu_event,
                                GetEventTypeCount(menu_menu_events),
                                menu_menu_events, 0, &eventHandlerRef);
        menu_eventHandlers_hash()->insert(ret, eventHandlerRef);
    } else {
        qWarning("QMenu: Internal error");
    }
#else
    if (QMenu *qmenu = qobject_cast<QMenu *>(w)){
        ret = [[QT_MANGLE_NAMESPACE(QCocoaMenu) alloc] initWithQMenu:qmenu];
    } else {
        ret = [[NSMenu alloc] init];
    }
#endif
    return ret;
}



QT_END_NAMESPACE

