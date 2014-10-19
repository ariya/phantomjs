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

#include "qmacdefines_mac.h"
#ifdef QT_MAC_USE_COCOA
#include <qaction.h>
#include <qcoreapplication.h>
#include <private/qcocoamenuloader_mac_p.h>
#include <private/qapplication_p.h>
#include <private/qt_mac_p.h>
#include <private/qmenubar_p.h>
#include <qmenubar.h>
#include <private/qt_cocoa_helpers_mac_p.h>

QT_FORWARD_DECLARE_CLASS(QCFString)
QT_FORWARD_DECLARE_CLASS(QString)

#ifndef QT_NO_TRANSLATION
    QT_BEGIN_NAMESPACE
    extern QString qt_mac_applicationmenu_string(int type);
    QT_END_NAMESPACE
#endif

QT_USE_NAMESPACE

@implementation QT_MANGLE_NAMESPACE(QCocoaMenuLoader)

- (void)awakeFromNib
{
    servicesItem = [[appMenu itemWithTitle:@"Services"] retain];
    hideAllOthersItem = [[appMenu itemWithTitle:@"Hide Others"] retain];
    showAllItem = [[appMenu itemWithTitle:@"Show All"] retain];

    // Get the names in the nib to match the app name set by Qt.
    const NSString *appName = reinterpret_cast<const NSString*>(QCFString::toCFStringRef(qAppName()));
    [quitItem setTitle:[[quitItem title] stringByReplacingOccurrencesOfString:@"NewApplication"
                                                                   withString:const_cast<NSString *>(appName)]];
    [hideItem setTitle:[[hideItem title] stringByReplacingOccurrencesOfString:@"NewApplication"
                                                                   withString:const_cast<NSString *>(appName)]];
    [aboutItem setTitle:[[aboutItem title] stringByReplacingOccurrencesOfString:@"NewApplication"
                                                                   withString:const_cast<NSString *>(appName)]];
    [appName release];
    // Disable the items that don't do anything. If someone associates a QAction with them
    // They should get synced back in.
    [preferencesItem setEnabled:NO];
    [preferencesItem setHidden:YES];
    [aboutItem setEnabled:NO];
    [aboutItem setHidden:YES];
}

- (void)ensureAppMenuInMenu:(NSMenu *)menu
{
    // The application menu is the menu in the menu bar that contains the
    // 'Quit' item. When changing menu bar (e.g when switching between
    // windows with different menu bars), we never recreate this menu, but
    // instead pull it out the current menu bar and place into the new one:
    NSMenu *mainMenu = [[NSApplication sharedApplication] mainMenu];
    if ([[NSApplication sharedApplication] mainMenu] == menu)
        return; // nothing to do (menu is the current menu bar)!

#ifndef QT_NAMESPACE
    Q_ASSERT(mainMenu);
#endif
    // Grab the app menu out of the current menu.
    int numItems = [mainMenu numberOfItems];
    NSMenuItem *oldAppMenuItem = 0;
    for (int i = 0; i < numItems; ++i) {
        NSMenuItem *item = [mainMenu itemAtIndex:i];
        if ([item submenu] == appMenu) {
            oldAppMenuItem = item;
            [oldAppMenuItem retain];
            [mainMenu removeItemAtIndex:i];
            break;
        }
    }

    if (oldAppMenuItem) {
        [oldAppMenuItem setSubmenu:nil];
        [oldAppMenuItem release];
        NSMenuItem *appMenuItem = [[NSMenuItem alloc] initWithTitle:@"Apple"
            action:nil keyEquivalent:@""];
        [appMenuItem setSubmenu:appMenu];
        [menu insertItem:appMenuItem atIndex:0];
    }
}

- (void)removeActionsFromAppMenu
{
    for (NSMenuItem *item in [appMenu itemArray])
        [item setTag:nil];
}

- (void)dealloc
{
    [servicesItem release];
    [hideAllOthersItem release];
    [showAllItem release];

    [lastAppSpecificItem release];
    [theMenu release];
    [appMenu release];
    [super dealloc];
}

- (NSMenu *)menu
{
    return [[theMenu retain] autorelease];
}

- (NSMenu *)applicationMenu
{
    return [[appMenu retain] autorelease];
}

- (NSMenuItem *)quitMenuItem
{
    return [[quitItem retain] autorelease];
}

- (NSMenuItem *)preferencesMenuItem
{
    return [[preferencesItem retain] autorelease];
}

- (NSMenuItem *)aboutMenuItem
{
    return [[aboutItem retain] autorelease];
}

- (NSMenuItem *)aboutQtMenuItem
{
    return [[aboutQtItem retain] autorelease];
}

- (NSMenuItem *)hideMenuItem
{
    return [[hideItem retain] autorelease];
}

- (NSMenuItem *)appSpecificMenuItem
{
    // Create an App-Specific menu item, insert it into the menu and return
    // it as an autorelease item.
    NSMenuItem *item = [[NSMenuItem alloc] init];

    NSInteger location;
    if (lastAppSpecificItem == nil) {
        location = [appMenu indexOfItem:aboutQtItem];
    } else {
        location = [appMenu indexOfItem:lastAppSpecificItem];
        [lastAppSpecificItem release];
    }
    lastAppSpecificItem = item;  // Keep track of this for later (i.e., don't release it)
    [appMenu insertItem:item atIndex:location + 1];

    return [[item retain] autorelease];
}

- (BOOL) acceptsFirstResponder
{
    return YES;
}

- (void)terminate:(id)sender
{
    [[NSApplication sharedApplication] terminate:sender];
}

- (void)orderFrontStandardAboutPanel:(id)sender
{
    [[NSApplication sharedApplication] orderFrontStandardAboutPanel:sender];
}

- (void)hideOtherApplications:(id)sender
{
    [[NSApplication sharedApplication] hideOtherApplications:sender];
}

- (void)unhideAllApplications:(id)sender
{
    [[NSApplication sharedApplication] unhideAllApplications:sender];
}

- (void)hide:(id)sender
{
    [[NSApplication sharedApplication] hide:sender];
}

- (void)qtUpdateMenubar
{
    QMenuBarPrivate::macUpdateMenuBarImmediatly();
}

- (void)qtTranslateApplicationMenu
{
#ifndef QT_NO_TRANSLATION
    [servicesItem setTitle: qt_mac_QStringToNSString(qt_mac_applicationmenu_string(0))];
    [hideItem setTitle: qt_mac_QStringToNSString(qt_mac_applicationmenu_string(1).arg(qAppName()))];
    [hideAllOthersItem setTitle: qt_mac_QStringToNSString(qt_mac_applicationmenu_string(2))];
    [showAllItem setTitle: qt_mac_QStringToNSString(qt_mac_applicationmenu_string(3))];
    [preferencesItem setTitle: qt_mac_QStringToNSString(qt_mac_applicationmenu_string(4))];
    [quitItem setTitle: qt_mac_QStringToNSString(qt_mac_applicationmenu_string(5).arg(qAppName()))];
    [aboutItem setTitle: qt_mac_QStringToNSString(qt_mac_applicationmenu_string(6).arg(qAppName()))];
#endif
}

- (IBAction)qtDispatcherToQAction:(id)sender
{
    QScopedLoopLevelCounter loopLevelCounter(QApplicationPrivate::instance()->threadData);
    NSMenuItem *item = static_cast<NSMenuItem *>(sender);
    if (QAction *action = reinterpret_cast<QAction *>([item tag])) {
        action->trigger();
    } else if (item == quitItem) {
        // We got here because someone was once the quitItem, but it has been
        // abandoned (e.g., the menubar was deleted). In the meantime, just do
        // normal QApplication::quit().
        qApp->quit();
    }
}

 - (void)orderFrontCharacterPalette:(id)sender
 {
     [[NSApplication sharedApplication] orderFrontCharacterPalette:sender];
 }

- (BOOL)validateMenuItem:(NSMenuItem*)menuItem
{
    if ([menuItem action] == @selector(hide:)
        || [menuItem action] == @selector(hideOtherApplications:)
        || [menuItem action] == @selector(unhideAllApplications:)) {
        return [[NSApplication sharedApplication] validateMenuItem:menuItem];
    } else {
        return [menuItem isEnabled];
    }
}

@end
#endif // QT_MAC_USE_COCOA
