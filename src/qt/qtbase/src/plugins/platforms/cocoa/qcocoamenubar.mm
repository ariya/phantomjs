/****************************************************************************
**
** Copyright (C) 2012 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author James Turner <james.turner@kdab.com>
** Contact: http://www.qt-project.org/legal
**
** This file is part of the plugins of the Qt Toolkit.
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

#include <Cocoa/Cocoa.h>

#include "qcocoamenubar.h"
#include "qcocoawindow.h"
#include "qcocoamenuloader.h"
#include "qcocoaapplication.h" // for custom application category
#include "qcocoaautoreleasepool.h"

#include <QtGui/QGuiApplication>
#include <QtCore/QDebug>

static QList<QCocoaMenuBar*> static_menubars;

static inline QCocoaMenuLoader *getMenuLoader()
{
    return [NSApp QT_MANGLE_NAMESPACE(qt_qcocoamenuLoader)];
}


QCocoaMenuBar::QCocoaMenuBar() :
    m_window(0)
{
    static_menubars.append(this);

    m_nativeMenu = [[NSMenu alloc] init];
#ifdef QT_COCOA_ENABLE_MENU_DEBUG
    qDebug() << "Construct QCocoaMenuBar" << this << m_nativeMenu;
#endif
}

QCocoaMenuBar::~QCocoaMenuBar()
{
#ifdef QT_COCOA_ENABLE_MENU_DEBUG
    qDebug() << "~QCocoaMenuBar" << this;
#endif
    [m_nativeMenu release];
    static_menubars.removeOne(this);

    if (m_window && m_window->menubar() == this) {
        m_window->setMenubar(0);
        // Delete the children first so they do not cause
        // the native menu items to be hidden after
        // the menu bar was updated
        qDeleteAll(children());
        updateMenuBarImmediately();
    }
}

void QCocoaMenuBar::insertNativeMenu(QCocoaMenu *menu, QCocoaMenu *beforeMenu)
{
    QCocoaAutoReleasePool pool;

    if (beforeMenu) {
        NSUInteger nativeIndex = [m_nativeMenu indexOfItem:beforeMenu->nsMenuItem()];
        [m_nativeMenu insertItem: menu->nsMenuItem() atIndex: nativeIndex];
    } else {
        [m_nativeMenu addItem: menu->nsMenuItem()];
    }

    menu->setMenuBar(this);
    syncMenu(static_cast<QPlatformMenu *>(menu));
    [m_nativeMenu setSubmenu: menu->nsMenu() forItem: menu->nsMenuItem()];
}

void QCocoaMenuBar::insertMenu(QPlatformMenu *platformMenu, QPlatformMenu *before)
{
    QCocoaMenu *menu = static_cast<QCocoaMenu *>(platformMenu);
    QCocoaMenu *beforeMenu = static_cast<QCocoaMenu *>(before);
#ifdef QT_COCOA_ENABLE_MENU_DEBUG
    qDebug() << "QCocoaMenuBar" << this << "insertMenu" << menu << "before" << before;
#endif

    if (m_menus.contains(menu)) {
        qWarning() << Q_FUNC_INFO << "This menu already belongs to the menubar, remove it first";
        return;
    }

    if (beforeMenu && !m_menus.contains(beforeMenu)) {
        qWarning() << Q_FUNC_INFO << "The before menu does not belong to the menubar";
        return;
    }

    m_menus.insert(beforeMenu ? m_menus.indexOf(beforeMenu) : m_menus.size(), menu);
    if (!menu->menuBar())
        insertNativeMenu(menu, beforeMenu);
    if (m_window && m_window->window()->isActive())
        updateMenuBarImmediately();
}

void QCocoaMenuBar::removeNativeMenu(QCocoaMenu *menu)
{
    QCocoaAutoReleasePool pool;

    if (menu->menuBar() == this)
        menu->setMenuBar(0);
    NSUInteger realIndex = [m_nativeMenu indexOfItem:menu->nsMenuItem()];
    [m_nativeMenu removeItemAtIndex: realIndex];
}

void QCocoaMenuBar::removeMenu(QPlatformMenu *platformMenu)
{
    QCocoaMenu *menu = static_cast<QCocoaMenu *>(platformMenu);
    if (!m_menus.contains(menu)) {
        qWarning() << Q_FUNC_INFO << "Trying to remove a menu that does not belong to the menubar";
        return;
    }
    m_menus.removeOne(menu);
    removeNativeMenu(menu);
}

void QCocoaMenuBar::syncMenu(QPlatformMenu *menu)
{
    QCocoaMenu *cocoaMenu = static_cast<QCocoaMenu *>(menu);
    Q_FOREACH (QCocoaMenuItem *item, cocoaMenu->items())
        cocoaMenu->syncMenuItem(item);

    BOOL shouldHide = YES;
    if (cocoaMenu->isVisible()) {
        // If the NSMenu has no visble items, or only separators, we should hide it
        // on the menubar. This can happen after syncing the menu items since they
        // can be moved to other menus.
        for (NSMenuItem *item in [cocoaMenu->nsMenu() itemArray])
            if (![item isSeparatorItem] && ![item isHidden]) {
                shouldHide = NO;
                break;
            }
    }
    [cocoaMenu->nsMenuItem() setHidden:shouldHide];
}

void QCocoaMenuBar::handleReparent(QWindow *newParentWindow)
{
#ifdef QT_COCOA_ENABLE_MENU_DEBUG
    qDebug() << "QCocoaMenuBar" << this << "handleReparent" << newParentWindow;
#endif

    if (m_window)
        m_window->setMenubar(NULL);

    if (newParentWindow == NULL) {
        m_window = NULL;
    } else {
        newParentWindow->create();
        m_window = static_cast<QCocoaWindow*>(newParentWindow->handle());
        m_window->setMenubar(this);
    }

    updateMenuBarImmediately();
}

QCocoaWindow *QCocoaMenuBar::findWindowForMenubar()
{
    if (qApp->focusWindow())
        return static_cast<QCocoaWindow*>(qApp->focusWindow()->handle());

    return NULL;
}

QCocoaMenuBar *QCocoaMenuBar::findGlobalMenubar()
{
    foreach (QCocoaMenuBar *mb, static_menubars) {
        if (mb->m_window == NULL)
            return mb;
    }

    return NULL;
}

void QCocoaMenuBar::redirectKnownMenuItemsToFirstResponder()
{
    // QTBUG-17291: http://forums.macrumors.com/showthread.php?t=1249452
    // When a dialog is opened, shortcuts for actions inside the dialog (cut, paste, ...)
    // continue to go through the same menu items which claimed those shortcuts.
    // They are not keystrokes which we can intercept in any other way; the OS intercepts them.
    // The menu items had to be created by the application.  That's why we need roles
    // to identify those "special" menu items which can be useful even when non-Qt
    // native widgets are in focus.  When the native widget is focused it will be the
    // first responder, so the menu item needs to have its target be the first responder;
    // this is done by setting it to nil.

    // This function will find all menu items on all menus which have
    // "special" roles, set the target and also set the standard actions which
    // apply to those roles.  But afterwards it is necessary to call
    // resetKnownMenuItemsToQt() to put back the target and action so that
    // those menu items will go back to invoking their associated QActions.
    foreach (QCocoaMenuBar *mb, static_menubars)
        foreach (QCocoaMenu *m, mb->m_menus)
            foreach (QCocoaMenuItem *i, m->items()) {
                bool known = true;
                switch (i->effectiveRole()) {
                case QPlatformMenuItem::CutRole:
                    [i->nsItem() setAction:@selector(cut:)];
                    break;
                case QPlatformMenuItem::CopyRole:
                    [i->nsItem() setAction:@selector(copy:)];
                    break;
                case QPlatformMenuItem::PasteRole:
                    [i->nsItem() setAction:@selector(paste:)];
                    break;
                case QPlatformMenuItem::SelectAllRole:
                    [i->nsItem() setAction:@selector(selectAll:)];
                    break;
                // We may discover later that there are other roles/actions which
                // are meaningful to standard native widgets; they can be added.
                default:
                    known = false;
                    break;
                }
                if (known)
                    [i->nsItem() setTarget:nil];
            }
}

void QCocoaMenuBar::resetKnownMenuItemsToQt()
{
    // Undo the effect of redirectKnownMenuItemsToFirstResponder():
    // set the menu items' actions to itemFired and their targets to
    // the QCocoaMenuDelegate.
    updateMenuBarImmediately();
}

void QCocoaMenuBar::updateMenuBarImmediately()
{
    QCocoaAutoReleasePool pool;
    QCocoaMenuBar *mb = findGlobalMenubar();
    QCocoaWindow *cw = findWindowForMenubar();

    QWindow *win = cw ? cw->window() : 0;
    if (win && (win->flags() & Qt::Popup) == Qt::Popup)
        return; // context menus, comboboxes, etc. don't need to update the menubar

    if (cw && cw->menubar())
        mb = cw->menubar();

    if (!mb)
        return;

#ifdef QT_COCOA_ENABLE_MENU_DEBUG
    qDebug() << "QCocoaMenuBar" << "updateMenuBarImmediately" << cw;
#endif
    bool disableForModal = mb->shouldDisable(cw);
    // force a sync?
    foreach (QCocoaMenu *m, mb->m_menus) {
        mb->syncMenu(m);
        m->syncModalState(disableForModal);
    }

    // reparent shared menu items if necessary.
    // We browse the list in reverse order to be sure that the next items are redrawn before the current ones,
    // in this way we are sure that "beforeMenu" (see below) is part of the native menu before "m" is redraw
    for (int i = mb->m_menus.size() - 1; i >= 0; i--) {
        QCocoaMenu *m = mb->m_menus.at(i);
        QCocoaMenuBar *menuBar = m->menuBar();
        if (menuBar != mb) {
            QCocoaMenu *beforeMenu  = i < (mb->m_menus.size() - 1) ? mb->m_menus.at(i + 1) : 0;
            if (menuBar)
                menuBar->removeNativeMenu(m);
            mb->insertNativeMenu(m, beforeMenu);
        }
    }

    QCocoaMenuLoader *loader = getMenuLoader();
    [loader ensureAppMenuInMenu:mb->nsMenu()];

    NSMutableSet *mergedItems = [[NSMutableSet setWithCapacity:0] retain];
    foreach (QCocoaMenuItem *m, mb->merged()) {
        [mergedItems addObject:m->nsItem()];
        m->syncMerged();
    }

    // hide+disable all mergeable items we're not currently using
    for (NSMenuItem *mergeable in [loader mergeable]) {
        if (![mergedItems containsObject:mergeable]) {
            [mergeable setHidden:YES];
            [mergeable setEnabled:NO];
        }
    }

    [mergedItems release];
    [NSApp setMainMenu:mb->nsMenu()];
    [loader qtTranslateApplicationMenu];
}

QList<QCocoaMenuItem*> QCocoaMenuBar::merged() const
{
    QList<QCocoaMenuItem*> r;
    foreach (QCocoaMenu* menu, m_menus)
        r.append(menu->merged());

    return r;
}

bool QCocoaMenuBar::shouldDisable(QCocoaWindow *active) const
{
    if (active && (active->window()->modality() == Qt::NonModal))
        return false;

    if (m_window == active) {
        // modal window owns us, we should be enabled!
        return false;
    }

    QWindowList topWindows(qApp->topLevelWindows());
    // When there is an application modal window on screen, the entries of
    // the menubar should be disabled. The exception in Qt is that if the
    // modal window is the only window on screen, then we enable the menu bar.
    foreach (QWindow *w, topWindows) {
        if (w->isVisible() && w->modality() == Qt::ApplicationModal) {
            // check for other visible windows
            foreach (QWindow *other, topWindows) {
                if ((w != other) && (other->isVisible())) {
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
    }

    return true;
}

QPlatformMenu *QCocoaMenuBar::menuForTag(quintptr tag) const
{
    foreach (QCocoaMenu *menu, m_menus) {
        if (menu->tag() ==  tag)
            return menu;
    }

    return 0;
}

NSMenuItem *QCocoaMenuBar::itemForRole(QPlatformMenuItem::MenuRole r)
{
    foreach (QCocoaMenu *m, m_menus)
        foreach (QCocoaMenuItem *i, m->items())
            if (i->effectiveRole() == r)
                return i->nsItem();
    return Q_NULLPTR;
}

