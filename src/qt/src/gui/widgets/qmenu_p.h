/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtGui module of the Qt Toolkit.
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

#ifndef QMENU_P_H
#define QMENU_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include "QtGui/qmenubar.h"
#include "QtGui/qstyleoption.h"
#include "QtCore/qdatetime.h"
#include "QtCore/qmap.h"
#include "QtCore/qhash.h"
#include "QtCore/qbasictimer.h"
#include "private/qwidget_p.h"

#ifdef Q_WS_S60
class CEikMenuPane;
#define QT_SYMBIAN_FIRST_MENU_ITEM 32000
#define QT_SYMBIAN_LAST_MENU_ITEM 41999 // 10000 items ought to be enough for anybody...
#endif
QT_BEGIN_NAMESPACE

#ifndef QT_NO_MENU

#ifdef Q_WS_S60
void qt_symbian_next_menu_from_action(QWidget* actionContainer);
void qt_symbian_show_toplevel(CEikMenuPane* menuPane);
void qt_symbian_show_submenu(CEikMenuPane* menuPane, int id);
#endif // Q_WS_S60

class QTornOffMenu;
class QEventLoop;

#ifdef Q_WS_MAC
#  ifdef __OBJC__
QT_END_NAMESPACE
@class NSMenuItem;
QT_BEGIN_NAMESPACE
#  else
typedef void NSMenuItem;
#  endif //__OBJC__
struct QMacMenuAction {
    QMacMenuAction()
#ifndef QT_MAC_USE_COCOA
       : command(0)
#else
       : menuItem(0)
#endif
         , ignore_accel(0), merged(0), menu(0)
    {
    }
    ~QMacMenuAction();
#ifndef QT_MAC_USE_COCOA
    uint command;
#else
    NSMenuItem *menuItem;
#endif
    uchar ignore_accel : 1;
    uchar merged : 1;
    QPointer<QAction> action;
    OSMenuRef menu;
};

struct QMenuMergeItem
{
#ifndef QT_MAC_USE_COCOA
    inline QMenuMergeItem(MenuCommand c, QMacMenuAction *a) : command(c), action(a) { }
    MenuCommand command;
#else
    inline QMenuMergeItem(NSMenuItem *c, QMacMenuAction *a) : menuItem(c), action(a) { }
    NSMenuItem *menuItem;
#endif
    QMacMenuAction *action;
};
typedef QList<QMenuMergeItem> QMenuMergeList;
#endif

#ifdef Q_WS_WINCE
struct QWceMenuAction {
    uint command;
    QPointer<QAction> action;
    HMENU menuHandle;
    QWceMenuAction() : menuHandle(0), command(0) {}
};
#endif
#ifdef Q_WS_S60
struct QSymbianMenuAction {
    uint command;
    int parent;
    CEikMenuPane* menuPane;
    QPointer<QAction> action;
    QSymbianMenuAction() : command(0) {}
};
#endif

class QMenuPrivate : public QWidgetPrivate
{
    Q_DECLARE_PUBLIC(QMenu)
public:
    QMenuPrivate() : itemsDirty(0), maxIconWidth(0), tabWidth(0), ncols(0),
                      collapsibleSeparators(true), activationRecursionGuard(false), hasHadMouse(0), aboutToHide(0), motions(0),
                      currentAction(0),
#ifdef QT_KEYPAD_NAVIGATION
                      selectAction(0),
                      cancelAction(0),
#endif
                      scroll(0), eventLoop(0), tearoff(0), tornoff(0), tearoffHighlighted(0),
                      hasCheckableItems(0), sloppyAction(0), doChildEffects(false)
#ifdef QT3_SUPPORT
                      ,emitHighlighted(false)
#endif
#ifdef Q_WS_MAC
                      ,mac_menu(0)
#endif
#if defined(Q_WS_WINCE) && !defined(QT_NO_MENUBAR)
                      ,wce_menu(0)
#endif
#ifdef Q_WS_S60
                      ,symbian_menu(0)
#endif
    { }
    ~QMenuPrivate()
    {
        delete scroll;
#ifdef Q_WS_MAC
        delete mac_menu;
#endif
#if defined(Q_WS_WINCE) && !defined(QT_NO_MENUBAR)
        delete wce_menu;
#endif
#ifdef Q_WS_S60
        delete symbian_menu;
#endif

    }
    void init();

    static QMenuPrivate *get(QMenu *m) { return m->d_func(); }
    int scrollerHeight() const;

    //item calculations
    mutable uint itemsDirty : 1;
    mutable uint maxIconWidth, tabWidth;
    QRect actionRect(QAction *) const;

    mutable QVector<QRect> actionRects;
    mutable QHash<QAction *, QWidget *> widgetItems;
    void updateActionRects() const;
    void updateActionRects(const QRect &screen) const;
    QRect popupGeometry(const QWidget *widget) const;
    QRect popupGeometry(int screen = -1) const;
    mutable uint ncols : 4; //4 bits is probably plenty
    uint collapsibleSeparators : 1;
    QSize adjustMenuSizeForScreen(const QRect & screen);
    int getLastVisibleAction() const;

    bool activationRecursionGuard;

    //selection
    static QMenu *mouseDown;
    QPoint mousePopupPos;
    uint hasHadMouse : 1;
    uint aboutToHide : 1;
    int motions;
    QAction *currentAction;
#ifdef QT_KEYPAD_NAVIGATION
    QAction *selectAction;
    QAction *cancelAction;
#endif
    QBasicTimer menuDelayTimer;
    enum SelectionReason {
        SelectedFromKeyboard,
        SelectedFromElsewhere
    };
    QWidget *topCausedWidget() const;
    QAction *actionAt(QPoint p) const;
    void setFirstActionActive();
    void setCurrentAction(QAction *, int popup = -1, SelectionReason reason = SelectedFromElsewhere, bool activateFirst = false);
    void popupAction(QAction *, int, bool);
    void setSyncAction();

    //scrolling support
    struct QMenuScroller {
        enum ScrollLocation { ScrollStay, ScrollBottom, ScrollTop, ScrollCenter };
        enum ScrollDirection { ScrollNone=0, ScrollUp=0x01, ScrollDown=0x02 };
        uint scrollFlags : 2, scrollDirection : 2;
        int scrollOffset;
        QBasicTimer scrollTimer;

        QMenuScroller() : scrollFlags(ScrollNone), scrollDirection(ScrollNone), scrollOffset(0) { }
        ~QMenuScroller() { }
    } *scroll;
    void scrollMenu(QMenuScroller::ScrollLocation location, bool active=false);
    void scrollMenu(QMenuScroller::ScrollDirection direction, bool page=false, bool active=false);
    void scrollMenu(QAction *action, QMenuScroller::ScrollLocation location, bool active=false);

    //synchronous operation (ie exec())
    QEventLoop *eventLoop;
    QPointer<QAction> syncAction;

    //search buffer
    QString searchBuffer;
    QBasicTimer searchBufferTimer;

    //passing of mouse events up the parent hierarchy
    QPointer<QMenu> activeMenu;
    bool mouseEventTaken(QMouseEvent *);

    //used to walk up the popup list
    struct QMenuCaused {
        QPointer<QWidget> widget;
        QPointer<QAction> action;
    };
    virtual QList<QPointer<QWidget> > calcCausedStack() const;
    QMenuCaused causedPopup;
    void hideUpToMenuBar();
    void hideMenu(QMenu *menu, bool justRegister = false);

    //index mappings
    inline QAction *actionAt(int i) const { return q_func()->actions().at(i); }
    inline int indexOf(QAction *act) const { return q_func()->actions().indexOf(act); }

    //tear off support
    uint tearoff : 1, tornoff : 1, tearoffHighlighted : 1;
    QPointer<QTornOffMenu> tornPopup;

    mutable bool hasCheckableItems;

    //sloppy selection
    static int sloppyDelayTimer;
    mutable QAction *sloppyAction;
    QRegion sloppyRegion;

    //default action
    QPointer<QAction> defaultAction;

    QAction *menuAction;
    QAction *defaultMenuAction;

    void setOverrideMenuAction(QAction *);
    void _q_overrideMenuActionDestroyed();

    //firing of events
    void activateAction(QAction *, QAction::ActionEvent, bool self=true);
    void activateCausedStack(const QList<QPointer<QWidget> > &, QAction *, QAction::ActionEvent, bool);

    void _q_actionTriggered();
    void _q_actionHovered();

    bool hasMouseMoved(const QPoint &globalPos);

    void updateLayoutDirection();

    //menu fading/scrolling effects
    bool doChildEffects;

#ifdef Q_WS_MAC
    //mac menu binding
    struct QMacMenuPrivate {
        QList<QMacMenuAction*> actionItems;
        OSMenuRef menu;
        QMacMenuPrivate();
        ~QMacMenuPrivate();

        bool merged(const QAction *action) const;
        void addAction(QAction *, QMacMenuAction* =0, QMenuPrivate *qmenu = 0);
        void addAction(QMacMenuAction *, QMacMenuAction* =0, QMenuPrivate *qmenu = 0);
        void syncAction(QMacMenuAction *);
        inline void syncAction(QAction *a) { syncAction(findAction(a)); }
        void removeAction(QMacMenuAction *);
        inline void removeAction(QAction *a) { removeAction(findAction(a)); }
        inline QMacMenuAction *findAction(QAction *a) {
            for(int i = 0; i < actionItems.size(); i++) {
                QMacMenuAction *act = actionItems[i];
                if(a == act->action)
                    return act;
            }
            return 0;
        }
    } *mac_menu;
    OSMenuRef macMenu(OSMenuRef merge);
    void setMacMenuEnabled(bool enable = true);
    void syncSeparatorsCollapsible(bool collapsible);
    static QHash<OSMenuRef, OSMenuRef> mergeMenuHash;
    static QHash<OSMenuRef, QMenuMergeList*> mergeMenuItemsHash;
#endif

    QPointer<QAction> actionAboutToTrigger;
#ifdef QT3_SUPPORT
    bool emitHighlighted;
#endif

#if defined(Q_WS_WINCE) && !defined(QT_NO_MENUBAR)
    struct QWceMenuPrivate {
        QList<QWceMenuAction*> actionItems;
        HMENU menuHandle;
        QWceMenuPrivate();
        ~QWceMenuPrivate();
        void addAction(QAction *, QWceMenuAction* =0);
        void addAction(QWceMenuAction *, QWceMenuAction* =0);
        void syncAction(QWceMenuAction *);
        inline void syncAction(QAction *a) { syncAction(findAction(a)); }
        void removeAction(QWceMenuAction *);
        void rebuild();
        inline void removeAction(QAction *a) { removeAction(findAction(a)); }
        inline QWceMenuAction *findAction(QAction *a) {
            for(int i = 0; i < actionItems.size(); i++) {
                QWceMenuAction *act = actionItems[i];
                if(a == act->action)
                    return act;
            }
            return 0;
        }
    } *wce_menu;
    HMENU wceMenu();
    QAction* wceCommands(uint command);
#endif
#if defined(Q_WS_S60)
    struct QSymbianMenuPrivate {
        QList<QSymbianMenuAction*> actionItems;
        QSymbianMenuPrivate();
        ~QSymbianMenuPrivate();
        void addAction(QAction *, QSymbianMenuAction* =0);
        void addAction(QSymbianMenuAction *, QSymbianMenuAction* =0);
        void syncAction(QSymbianMenuAction *);
        inline void syncAction(QAction *a) { syncAction(findAction(a)); }
        void removeAction(QSymbianMenuAction *);
        void rebuild(bool reCreate = false);
        inline void removeAction(QAction *a) { removeAction(findAction(a)); }
        inline QSymbianMenuAction *findAction(QAction *a) {
            for(int i = 0; i < actionItems.size(); i++) {
                QSymbianMenuAction *act = actionItems[i];
                if(a == act->action)
                    return act;
            }
            return 0;
        }
    } *symbian_menu;
#endif
    QPointer<QWidget> noReplayFor;
};

#endif // QT_NO_MENU

QT_END_NAMESPACE

#endif // QMENU_P_H
