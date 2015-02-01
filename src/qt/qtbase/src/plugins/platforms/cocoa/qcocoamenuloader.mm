/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
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

#include "qcocoamenuloader.h"

#include "messages.h"
#include "qcocoahelpers.h"
#include "qcocoamenubar.h"
#include "qcocoamenuitem.h"

#include <QtCore/private/qcore_mac_p.h>
#include <QtCore/private/qthread_p.h>
#include <QtCore/qcoreapplication.h>
#include <QtCore/qdir.h>
#include <QtCore/qstring.h>
#include <QtCore/qdebug.h>
#include <QtGui/private/qguiapplication_p.h>

QT_FORWARD_DECLARE_CLASS(QCFString)
QT_FORWARD_DECLARE_CLASS(QString)


QT_BEGIN_NAMESPACE

/*
    Loads and instantiates the main app menu from the menu nib file(s).

    The main app menu contains the Quit, Hide  About, Preferences entries, and
    The reason for having the nib file is that those can not be created
    programmatically. To ease deployment the nib files are stored in Qt resources
    and written to QDir::temp() before loading. (Earlier Qt versions used
    to require having the nib file in the Qt GUI framework.)
*/
void qt_mac_loadMenuNib(QCocoaMenuLoader *qtMenuLoader)
{
    // Create qt_menu.nib dir in temp.
    QDir temp = QDir::temp();
    temp.mkdir("qt_menu.nib");
    QString nibDir = temp.canonicalPath() + QLatin1String("/") + QLatin1String("qt_menu.nib/");
    if (!QDir(nibDir).exists()) {
        qWarning("qt_mac_loadMenuNib: could not create nib directory in temp");
        return;
    }

    // Copy nib files from resources to temp.
    QDir nibResource(":/qt-project.org/mac/qt_menu.nib/");
    if (!nibResource.exists()) {
        qWarning("qt_mac_loadMenuNib: could not load nib from resources");
        return;
    }
    foreach (const QFileInfo &file, nibResource.entryInfoList()) {
        QFile::copy(file.absoluteFilePath(), nibDir + QLatin1String("/") + file.fileName());
    }

    // Load and instantiate nib file from temp
    NSURL *nibUrl = [NSURL fileURLWithPath : QCFString::toNSString(nibDir)];
    NSNib *nib = [[NSNib alloc] initWithContentsOfURL : nibUrl];
    [nib autorelease];
    if(!nib) {
        qWarning("qt_mac_loadMenuNib: could not load nib from  temp");
        return;
    }
    bool ok = [nib instantiateNibWithOwner : qtMenuLoader topLevelObjects : nil];
    if (!ok) {
        qWarning("qt_mac_loadMenuNib: could not instantiate nib");
    }
}

QT_END_NAMESPACE

@implementation QCocoaMenuLoader

- (void)awakeFromNib
{
    servicesItem = [[appMenu itemWithTitle:@"Services"] retain];
    hideAllOthersItem = [[appMenu itemWithTitle:@"Hide Others"] retain];
    showAllItem = [[appMenu itemWithTitle:@"Show All"] retain];

    // Get the names in the nib to match the app name set by Qt.
    const NSString *appName = qt_mac_applicationName().toNSString();
    [quitItem setTitle:[[quitItem title] stringByReplacingOccurrencesOfString:@"NewApplication"
                                                                   withString:const_cast<NSString *>(appName)]];
    [hideItem setTitle:[[hideItem title] stringByReplacingOccurrencesOfString:@"NewApplication"
                                                                   withString:const_cast<NSString *>(appName)]];
    [aboutItem setTitle:[[aboutItem title] stringByReplacingOccurrencesOfString:@"NewApplication"
                                                                   withString:const_cast<NSString *>(appName)]];
    // Disable the items that don't do anything. If someone associates a QAction with them
    // They should get synced back in.
    [preferencesItem setEnabled:NO];
    [preferencesItem setHidden:YES];

    // should set this in the NIB
    [preferencesItem setTarget: self];
    [preferencesItem setAction: @selector(qtDispatcherToQPAMenuItem:)];

    [aboutItem setEnabled:NO];
    [aboutItem setHidden:YES];
}

- (void)ensureAppMenuInMenu:(NSMenu *)menu
{
    // The application menu is the menu in the menu bar that contains the
    // 'Quit' item. When changing menu bar (e.g when switching between
    // windows with different menu bars), we never recreate this menu, but
    // instead pull it out the current menu bar and place into the new one:
    NSMenu *mainMenu = [NSApp mainMenu];
    if ([NSApp mainMenu] == menu)
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
        [item setTag:0];
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

- (NSMenuItem *)appSpecificMenuItem:(NSInteger)tag
{
    NSMenuItem *item = [appMenu itemWithTag:tag];

    // No reason to create the item if it already exists. See QTBUG-27202.
    if (item)
        return [[item retain] autorelease];

    // Create an App-Specific menu item, insert it into the menu and return
    // it as an autorelease item.
    item = [[NSMenuItem alloc] init];

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
    [NSApp terminate:sender];
}

- (void)orderFrontStandardAboutPanel:(id)sender
{
    [NSApp orderFrontStandardAboutPanel:sender];
}

- (void)hideOtherApplications:(id)sender
{
    [NSApp hideOtherApplications:sender];
}

- (void)unhideAllApplications:(id)sender
{
    [NSApp unhideAllApplications:sender];
}

- (void)hide:(id)sender
{
    [NSApp hide:sender];
}

- (void)qtTranslateApplicationMenu
{

#ifndef QT_NO_TRANSLATION
    [servicesItem setTitle: QCFString::toNSString(qt_mac_applicationmenu_string(0))];
    [hideItem setTitle: QCFString::toNSString(qt_mac_applicationmenu_string(1).arg(qt_mac_applicationName()))];
    [hideAllOthersItem setTitle: QCFString::toNSString(qt_mac_applicationmenu_string(2))];
    [showAllItem setTitle: QCFString::toNSString(qt_mac_applicationmenu_string(3))];
    [preferencesItem setTitle: QCFString::toNSString(qt_mac_applicationmenu_string(4))];
    [quitItem setTitle: QCFString::toNSString(qt_mac_applicationmenu_string(5).arg(qt_mac_applicationName()))];
    [aboutItem setTitle: QCFString::toNSString(qt_mac_applicationmenu_string(6).arg(qt_mac_applicationName()))];
#endif
}

- (IBAction)qtDispatcherToQPAMenuItem:(id)sender
{
    NSMenuItem *item = static_cast<NSMenuItem *>(sender);
    if (item == quitItem) {
        // We got here because someone was once the quitItem, but it has been
        // abandoned (e.g., the menubar was deleted). In the meantime, just do
        // normal QApplication::quit().
        qApp->quit();
        return;
    }

    if ([item tag]) {
        QCocoaMenuItem *cocoaItem = reinterpret_cast<QCocoaMenuItem *>([item tag]);
        QScopedLoopLevelCounter loopLevelCounter(QGuiApplicationPrivate::instance()->threadData);
        cocoaItem->activated();
    }
}

- (void)orderFrontCharacterPalette:(id)sender
{
    [NSApp orderFrontCharacterPalette:sender];
}

- (BOOL)validateMenuItem:(NSMenuItem*)menuItem
{
    if ([menuItem action] == @selector(hide:)
        || [menuItem action] == @selector(hideOtherApplications:)
        || [menuItem action] == @selector(unhideAllApplications:)) {
        return [NSApp validateMenuItem:menuItem];
    } else if ([menuItem tag]) {
        QCocoaMenuItem *cocoaItem = reinterpret_cast<QCocoaMenuItem *>([menuItem tag]);
        return cocoaItem->isEnabled();
    } else {
        return [menuItem isEnabled];
    }
}

- (NSArray*) mergeable
{
    // don't include the quitItem here, since we want it always visible and enabled regardless
    return [NSArray arrayWithObjects:preferencesItem, aboutItem, aboutQtItem, lastAppSpecificItem, nil];
}

@end
