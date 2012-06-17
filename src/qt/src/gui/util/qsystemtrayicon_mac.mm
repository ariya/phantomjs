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

/****************************************************************************
**
** Copyright (c) 2007-2008, Apple, Inc.
**
** All rights reserved.
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are met:
**
**   * Redistributions of source code must retain the above copyright notice,
**     this list of conditions and the following disclaimer.
**
**   * Redistributions in binary form must reproduce the above copyright notice,
**     this list of conditions and the following disclaimer in the documentation
**     and/or other materials provided with the distribution.
**
**   * Neither the name of Apple, Inc. nor the names of its contributors
**     may be used to endorse or promote products derived from this software
**     without specific prior written permission.
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
** CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
** EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
** PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
** PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
** LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
** NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
** SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**
****************************************************************************/

#define QT_MAC_SYSTEMTRAY_USE_GROWL

#include <private/qt_cocoa_helpers_mac_p.h>
#include <private/qsystemtrayicon_p.h>
#include <qtemporaryfile.h>
#include <qimagewriter.h>
#include <qapplication.h>
#include <qdebug.h>
#include <qstyle.h>

#include <private/qt_mac_p.h>
#import <AppKit/AppKit.h>

QT_BEGIN_NAMESPACE
extern bool qt_mac_execute_apple_script(const QString &script, AEDesc *ret); //qapplication_mac.cpp
extern void qtsystray_sendActivated(QSystemTrayIcon *i, int r); //qsystemtrayicon.cpp
extern NSString *keySequenceToKeyEqivalent(const QKeySequence &accel); // qmenu_mac.mm
extern NSUInteger keySequenceModifierMask(const QKeySequence &accel);  // qmenu_mac.mm
extern Qt::MouseButton cocoaButton2QtButton(NSInteger buttonNum);
QT_END_NAMESPACE

QT_USE_NAMESPACE

@class QT_MANGLE_NAMESPACE(QNSMenu);
@class QT_MANGLE_NAMESPACE(QNSImageView);

@interface QT_MANGLE_NAMESPACE(QNSStatusItem) : NSObject {
    NSStatusItem *item;
    QSystemTrayIcon *icon;
    QSystemTrayIconPrivate *iconPrivate;
    QT_MANGLE_NAMESPACE(QNSImageView) *imageCell;
}
-(id)initWithIcon:(QSystemTrayIcon*)icon iconPrivate:(QSystemTrayIconPrivate *)iprivate;
-(void)dealloc;
-(QSystemTrayIcon*)icon;
-(NSStatusItem*)item;
-(QRectF)geometry;
- (void)triggerSelector:(id)sender button:(Qt::MouseButton)mouseButton;
- (void)doubleClickSelector:(id)sender;
@end

@interface QT_MANGLE_NAMESPACE(QNSImageView) : NSImageView {
    BOOL down;
    QT_MANGLE_NAMESPACE(QNSStatusItem) *parent;
}
-(id)initWithParent:(QT_MANGLE_NAMESPACE(QNSStatusItem)*)myParent;
-(QSystemTrayIcon*)icon;
-(void)menuTrackingDone:(NSNotification*)notification;
-(void)mousePressed:(NSEvent *)mouseEvent button:(Qt::MouseButton)mouseButton;
@end


#if MAC_OS_X_VERSION_MAX_ALLOWED <= MAC_OS_X_VERSION_10_5

@protocol NSMenuDelegate <NSObject>
-(void)menuNeedsUpdate:(NSMenu*)menu;
@end
#endif


@interface QT_MANGLE_NAMESPACE(QNSMenu) : NSMenu <NSMenuDelegate> {
    QMenu *qmenu;
}
-(QMenu*)menu;
-(id)initWithQMenu:(QMenu*)qmenu;
-(void)selectedAction:(id)item;
@end

QT_BEGIN_NAMESPACE
class QSystemTrayIconSys
{
public:
    QSystemTrayIconSys(QSystemTrayIcon *icon, QSystemTrayIconPrivate *d) {
        QMacCocoaAutoReleasePool pool;
        item = [[QT_MANGLE_NAMESPACE(QNSStatusItem) alloc] initWithIcon:icon iconPrivate:d];
    }
    ~QSystemTrayIconSys() {
        QMacCocoaAutoReleasePool pool;
        [[[item item] view] setHidden: YES];
        [item release];
    }
    QT_MANGLE_NAMESPACE(QNSStatusItem) *item;
};

void QSystemTrayIconPrivate::install_sys()
{
    Q_Q(QSystemTrayIcon);
    if (!sys) {
        sys = new QSystemTrayIconSys(q, this);
        updateIcon_sys();
        updateMenu_sys();
        updateToolTip_sys();
    }
}

QRect QSystemTrayIconPrivate::geometry_sys() const
{
    if(sys) {
        const QRectF geom = [sys->item geometry];
        if(!geom.isNull())
            return geom.toRect();
    }
    return QRect();
}

void QSystemTrayIconPrivate::remove_sys()
{
    delete sys;
    sys = 0;
}

void QSystemTrayIconPrivate::updateIcon_sys()
{
    if(sys && !icon.isNull()) {
        QMacCocoaAutoReleasePool pool;
#ifndef QT_MAC_USE_COCOA
        const short scale = GetMBarHeight()-4;
#else
        CGFloat hgt = [[[NSApplication sharedApplication] mainMenu] menuBarHeight];
        const short scale = hgt - 4;
#endif
        NSImage *nsimage = static_cast<NSImage *>(qt_mac_create_nsimage(icon.pixmap(QSize(scale, scale))));
        [(NSImageView*)[[sys->item item] view] setImage: nsimage];
        [nsimage release];
    }
}

void QSystemTrayIconPrivate::updateMenu_sys()
{
    if(sys) {
        QMacCocoaAutoReleasePool pool;
        if(menu && !menu->isEmpty()) {
            [[sys->item item] setHighlightMode:YES];
        } else {
            [[sys->item item] setHighlightMode:NO];
        }
    }
}

void QSystemTrayIconPrivate::updateToolTip_sys()
{
    if(sys) {
        QMacCocoaAutoReleasePool pool;
        QCFString string(toolTip);
        [[[sys->item item] view] setToolTip:(NSString*)static_cast<CFStringRef>(string)];
    }
}

bool QSystemTrayIconPrivate::isSystemTrayAvailable_sys()
{
    return true;
}

bool QSystemTrayIconPrivate::supportsMessages_sys()
{
    return true;
}

void QSystemTrayIconPrivate::showMessage_sys(const QString &title, const QString &message, QSystemTrayIcon::MessageIcon icon, int)
{

    if(sys) {
#ifdef QT_MAC_SYSTEMTRAY_USE_GROWL
        // Make sure that we have Growl installed on the machine we are running on.
        QCFType<CFURLRef> cfurl;
        OSStatus status = LSGetApplicationForInfo(kLSUnknownType, kLSUnknownCreator,
                                                  CFSTR("growlTicket"), kLSRolesAll, 0, &cfurl);
        if (status == kLSApplicationNotFoundErr)
            return;
        QCFType<CFBundleRef> bundle = CFBundleCreate(0, cfurl);

        if (CFStringCompare(CFBundleGetIdentifier(bundle), CFSTR("com.Growl.GrowlHelperApp"),
                    kCFCompareCaseInsensitive |  kCFCompareBackwards) != kCFCompareEqualTo)
            return;
        QPixmap notificationIconPixmap;
        if(icon == QSystemTrayIcon::Information)
            notificationIconPixmap = QApplication::style()->standardPixmap(QStyle::SP_MessageBoxInformation);
        else if(icon == QSystemTrayIcon::Warning)
            notificationIconPixmap = QApplication::style()->standardPixmap(QStyle::SP_MessageBoxWarning);
        else if(icon == QSystemTrayIcon::Critical)
            notificationIconPixmap = QApplication::style()->standardPixmap(QStyle::SP_MessageBoxCritical);
        QTemporaryFile notificationIconFile;
        QString notificationType(QLatin1String("Notification")), notificationIcon, notificationApp(QApplication::applicationName());
        if(notificationApp.isEmpty())
            notificationApp = QLatin1String("Application");
        if(!notificationIconPixmap.isNull() && notificationIconFile.open()) {
            QImageWriter writer(&notificationIconFile, "PNG");
            if(writer.write(notificationIconPixmap.toImage()))
                notificationIcon = QLatin1String("image from location \"file://") + notificationIconFile.fileName() + QLatin1String("\"");
        }
        const QString script(QLatin1String(
            "tell application \"GrowlHelperApp\"\n"
            "-- Make a list of all the notification types (all)\n"
            "set the allNotificationsList to {\"") + notificationType + QLatin1String("\"}\n"

            "-- Make a list of the notifications (enabled)\n"
            "set the enabledNotificationsList to {\"") + notificationType + QLatin1String("\"}\n"

            "-- Register our script with growl.\n"
            "register as application \"") + notificationApp + QLatin1String("\" all notifications allNotificationsList default notifications enabledNotificationsList\n"

            "--	Send a Notification...\n") +
            QLatin1String("notify with name \"") + notificationType +
            QLatin1String("\" title \"") + title +
            QLatin1String("\" description \"") + message +
            QLatin1String("\" application name \"") + notificationApp +
            QLatin1String("\" ")  + notificationIcon +
            QLatin1String("\nend tell"));
        qt_mac_execute_apple_script(script, 0);
#elif 0
        Q_Q(QSystemTrayIcon);
        NSView *v = [[sys->item item] view];
        NSWindow *w = [v window];
        w = [[sys->item item] window];
        qDebug() << w << v;
        QPoint p(qRound([w frame].origin.x), qRound([w frame].origin.y));
        qDebug() << p;
        QBalloonTip::showBalloon(icon, message, title, q, QPoint(0, 0), msecs);
#else
        Q_UNUSED(icon);
        Q_UNUSED(title);
        Q_UNUSED(message);
#endif
    }
}
QT_END_NAMESPACE

@implementation NSStatusItem (Qt)
@end

@implementation QT_MANGLE_NAMESPACE(QNSImageView)
-(id)initWithParent:(QT_MANGLE_NAMESPACE(QNSStatusItem)*)myParent {
    self = [super init];
    parent = myParent;
    down = NO;
    return self;
}

-(QSystemTrayIcon*)icon {
    return [parent icon];
}

-(void)menuTrackingDone:(NSNotification*)notification
{
    Q_UNUSED(notification);
    down = NO;

    if( ![self icon]->icon().isNull() ) {
#ifndef QT_MAC_USE_COCOA
        const short scale = GetMBarHeight()-4;
#else
        CGFloat hgt = [[[NSApplication sharedApplication] mainMenu] menuBarHeight];
        const short scale = hgt - 4;
#endif
        NSImage *nsimage = static_cast<NSImage *>(qt_mac_create_nsimage([self icon]->icon().pixmap(QSize(scale, scale))));
        [self setImage: nsimage];
        [nsimage release];
    }

    if([self icon]->contextMenu())
        [self icon]->contextMenu()->hide();

    [self setNeedsDisplay:YES];
}

-(void)mousePressed:(NSEvent *)mouseEvent button:(Qt::MouseButton)mouseButton
{
    down = YES;
    int clickCount = [mouseEvent clickCount];  
    [self setNeedsDisplay:YES];

#ifndef QT_MAC_USE_COCOA
    const short scale = GetMBarHeight()-4;
#else
    CGFloat hgt = [[[NSApplication sharedApplication] mainMenu] menuBarHeight];
    const short scale = hgt - 4;
#endif

    if (![self icon]->icon().isNull() ) {
        NSImage *nsaltimage = static_cast<NSImage *>(qt_mac_create_nsimage([self icon]->icon().pixmap(QSize(scale, scale), QIcon::Selected)));
        [self setImage: nsaltimage];
        [nsaltimage release];
    }

    if ((clickCount == 2)) {
        [self menuTrackingDone:nil];
        [parent doubleClickSelector:self];
    } else {
        [parent triggerSelector:self button:mouseButton];
    }
}

-(void)mouseDown:(NSEvent *)mouseEvent
{
    [self mousePressed:mouseEvent button:Qt::LeftButton];
}

-(void)mouseUp:(NSEvent *)mouseEvent
{
    Q_UNUSED(mouseEvent);
    [self menuTrackingDone:nil];
}

- (void)rightMouseDown:(NSEvent *)mouseEvent
{
    [self mousePressed:mouseEvent button:Qt::RightButton];
}

-(void)rightMouseUp:(NSEvent *)mouseEvent
{
    Q_UNUSED(mouseEvent);
    [self menuTrackingDone:nil];
}

- (void)otherMouseDown:(NSEvent *)mouseEvent
{
    [self mousePressed:mouseEvent button:cocoaButton2QtButton([mouseEvent buttonNumber])];
}

-(void)otherMouseUp:(NSEvent *)mouseEvent
{
    Q_UNUSED(mouseEvent);
    [self menuTrackingDone:nil];
}

-(void)drawRect:(NSRect)rect {
    [[parent item] drawStatusBarBackgroundInRect:rect withHighlight:down];
    [super drawRect:rect];
}
@end

@implementation QT_MANGLE_NAMESPACE(QNSStatusItem)

-(id)initWithIcon:(QSystemTrayIcon*)i iconPrivate:(QSystemTrayIconPrivate *)iPrivate
{
    self = [super init];
    if(self) {
        icon = i;
        iconPrivate = iPrivate;
        item = [[[NSStatusBar systemStatusBar] statusItemWithLength:NSSquareStatusItemLength] retain];
        imageCell = [[QT_MANGLE_NAMESPACE(QNSImageView) alloc] initWithParent:self];
        [item setView: imageCell];
    }
    return self;
}
-(void)dealloc {
    [[NSStatusBar systemStatusBar] removeStatusItem:item];
    [imageCell release];
    [item release];
    [super dealloc];

}

-(QSystemTrayIcon*)icon {
    return icon;
}

-(NSStatusItem*)item {
    return item;
}
-(QRectF)geometry {
    if(NSWindow *window = [[item view] window]) {
        NSRect screenRect = [[window screen] frame];
        NSRect windowRect = [window frame];
        return QRectF(windowRect.origin.x, screenRect.size.height-windowRect.origin.y-windowRect.size.height, windowRect.size.width, windowRect.size.height);
    }
    return QRectF();
}

- (void)triggerSelector:(id)sender button:(Qt::MouseButton)mouseButton {
    Q_UNUSED(sender);
    if (!icon)
        return;

    if (mouseButton == Qt::MidButton)
        qtsystray_sendActivated(icon, QSystemTrayIcon::MiddleClick);
    else
        qtsystray_sendActivated(icon, QSystemTrayIcon::Trigger);

    if (icon->contextMenu()) {
#ifndef QT_MAC_USE_COCOA
        [[[self item] view] removeAllToolTips];
        iconPrivate->updateToolTip_sys();
#endif
        NSMenu *m = [[QT_MANGLE_NAMESPACE(QNSMenu) alloc] initWithQMenu:icon->contextMenu()];
        [m setAutoenablesItems: NO];
        [[NSNotificationCenter defaultCenter] addObserver:imageCell
         selector:@selector(menuTrackingDone:)
             name:NSMenuDidEndTrackingNotification
                 object:m];
        [item popUpStatusItemMenu: m];
        [m release];
    }
}

- (void)doubleClickSelector:(id)sender {
    Q_UNUSED(sender);
    if(!icon)
        return;
    qtsystray_sendActivated(icon, QSystemTrayIcon::DoubleClick);
}

@end

class QSystemTrayIconQMenu : public QMenu
{
public:
    void doAboutToShow() { emit aboutToShow(); }
private:
    QSystemTrayIconQMenu();
};

@implementation QT_MANGLE_NAMESPACE(QNSMenu)
-(id)initWithQMenu:(QMenu*)qm {
    self = [super init];
    if(self) {
        self->qmenu = qm;
        [self setDelegate:self];
    }
    return self;
}
-(QMenu*)menu {
    return qmenu;
}
-(void)menuNeedsUpdate:(NSMenu*)nsmenu {
    QT_MANGLE_NAMESPACE(QNSMenu) *menu = static_cast<QT_MANGLE_NAMESPACE(QNSMenu) *>(nsmenu);
    emit static_cast<QSystemTrayIconQMenu*>(menu->qmenu)->doAboutToShow();
    for(int i = [menu numberOfItems]-1; i >= 0; --i)
        [menu removeItemAtIndex:i];
    QList<QAction*> actions = menu->qmenu->actions();;
    for(int i = 0; i < actions.size(); ++i) {
        const QAction *action = actions[i];
        if(!action->isVisible())
            continue;

        NSMenuItem *item = 0;
        bool needRelease = false;
        if(action->isSeparator()) {
            item = [NSMenuItem separatorItem];
        } else {
            item = [[NSMenuItem alloc] init];
            needRelease = true;
            QString text = action->text();
            QKeySequence accel = action->shortcut();
            {
                int st = text.lastIndexOf(QLatin1Char('\t'));
                if(st != -1) {
                    accel = QKeySequence(text.right(text.length()-(st+1)));
                    text.remove(st, text.length()-st);
                }
            }
            if(accel.count() > 1)
                text += QLatin1String(" (****)"); //just to denote a multi stroke shortcut

            [item setTitle:(NSString*)QCFString::toCFStringRef(qt_mac_removeMnemonics(text))];
            [item setEnabled:menu->qmenu->isEnabled() && action->isEnabled()];
            [item setState:action->isChecked() ? NSOnState : NSOffState];
            [item setToolTip:(NSString*)QCFString::toCFStringRef(action->toolTip())];
            const QIcon icon = action->icon();
            if(!icon.isNull()) {
#ifndef QT_MAC_USE_COCOA
                const short scale = GetMBarHeight();
#else
                const short scale = [[NSApp mainMenu] menuBarHeight];
#endif
                NSImage *nsimage = static_cast<NSImage *>(qt_mac_create_nsimage(icon.pixmap(QSize(scale, scale))));
                [item setImage: nsimage];
                [nsimage release];
            }
            if(action->menu()) {
                QT_MANGLE_NAMESPACE(QNSMenu) *sub = [[QT_MANGLE_NAMESPACE(QNSMenu) alloc] initWithQMenu:action->menu()];
                [item setSubmenu:sub];
            } else {
                [item setAction:@selector(selectedAction:)];
                [item setTarget:self];
            }
            if(!accel.isEmpty()) {
                [item setKeyEquivalent:keySequenceToKeyEqivalent(accel)];
                [item setKeyEquivalentModifierMask:keySequenceModifierMask(accel)];
            }
        }
        if(item)
            [menu addItem:item];
        if (needRelease)
            [item release];
    }
}
-(void)selectedAction:(id)a {
    const int activated = [self indexOfItem:a];
    QAction *action = 0;
    QList<QAction*> actions = qmenu->actions();
    for(int i = 0, cnt = 0; i < actions.size(); ++i) {
        if(actions.at(i)->isVisible() && (cnt++) == activated) {
            action = actions.at(i);
            break;
        }
    }
    if(action) {
        action->activate(QAction::Trigger);
    }
}
@end

