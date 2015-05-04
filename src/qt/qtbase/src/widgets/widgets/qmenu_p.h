/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtWidgets module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia. For licensing terms and
** conditions see http://qt.digia.com/licensing. For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights. These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
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

#include "QtWidgets/qmenubar.h"
#include "QtWidgets/qstyleoption.h"
#include "QtCore/qdatetime.h"
#include "QtCore/qmap.h"
#include "QtCore/qhash.h"
#include "QtCore/qbasictimer.h"
#include "private/qwidget_p.h"

#include <qpa/qplatformmenu.h>

QT_BEGIN_NAMESPACE

#ifndef QT_NO_MENU

class QTornOffMenu;
class QEventLoop;

#ifdef Q_OS_WINCE
struct QWceMenuAction {
    uint command;
    QPointer<QAction> action;
    HMENU menuHandle;
    QWceMenuAction() : menuHandle(0), command(0) {}
};
#endif

class QMenuPrivate : public QWidgetPrivate
{
    Q_DECLARE_PUBLIC(QMenu)
public:
    QMenuPrivate() : itemsDirty(0), maxIconWidth(0), tabWidth(0), ncols(0),
                      collapsibleSeparators(true), toolTipsVisible(false),
                      activationRecursionGuard(false), hasHadMouse(0), aboutToHide(0), motions(0),
                      currentAction(0),
#ifdef QT_KEYPAD_NAVIGATION
                      selectAction(0),
                      cancelAction(0),
#endif
                      scroll(0), eventLoop(0), tearoff(0), tornoff(0), tearoffHighlighted(0),
                      hasCheckableItems(0), sloppyDelayTimer(0), sloppyAction(0), doChildEffects(false), platformMenu(0)

#if defined(Q_OS_WINCE) && !defined(QT_NO_MENUBAR)
                      ,wce_menu(0)
#endif
    { }
    ~QMenuPrivate()
    {
        delete scroll;
        if (!platformMenu.isNull() && !platformMenu->parent())
            delete platformMenu.data();
#if defined(Q_OS_WINCE) && !defined(QT_NO_MENUBAR)
        delete wce_menu;
#endif
    }
    void init();
    void setPlatformMenu(QPlatformMenu *menu);
    void syncPlatformMenu();
#ifdef Q_OS_OSX
    void moveWidgetToPlatformItem(QWidget *w, QPlatformMenuItem* item);
#endif

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
    uint toolTipsVisible : 1;
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
    void hideMenu(QMenu *menu);

    //index mappings
    inline QAction *actionAt(int i) const { return q_func()->actions().at(i); }
    inline int indexOf(QAction *act) const { return q_func()->actions().indexOf(act); }

    //tear off support
    uint tearoff : 1, tornoff : 1, tearoffHighlighted : 1;
    QPointer<QTornOffMenu> tornPopup;

    mutable bool hasCheckableItems;

    //sloppy selection
    int sloppyDelayTimer;
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
    void _q_platformMenuAboutToShow();

    bool hasMouseMoved(const QPoint &globalPos);

    void updateLayoutDirection();

    //menu fading/scrolling effects
    bool doChildEffects;

    QPointer<QPlatformMenu> platformMenu;

    QPointer<QAction> actionAboutToTrigger;

#if defined(Q_OS_WINCE) && !defined(QT_NO_MENUBAR)
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
    QPointer<QWidget> noReplayFor;
};

#endif // QT_NO_MENU

QT_END_NAMESPACE

#endif // QMENU_P_H
