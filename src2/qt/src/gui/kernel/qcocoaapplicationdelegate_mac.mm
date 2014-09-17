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

#include "qmacdefines_mac.h"
#ifdef QT_MAC_USE_COCOA

#import <private/qcocoaapplicationdelegate_mac_p.h>
#import <private/qcocoamenuloader_mac_p.h>
#import <private/qcocoaapplication_mac_p.h>
#include <private/qapplication_p.h>
#include <private/qt_mac_p.h>
#include <private/qt_cocoa_helpers_mac_p.h>
#include <private/qdesktopwidget_mac_p.h>
#include <qevent.h>
#include <qurl.h>
#include <qapplication.h>

QT_BEGIN_NAMESPACE
extern void onApplicationChangedActivation(bool); // qapplication_mac.mm
extern void qt_release_apple_event_handler(); //qapplication_mac.mm
extern QPointer<QWidget> qt_last_mouse_receiver; // qapplication_mac.cpp
extern QPointer<QWidget> qt_last_native_mouse_receiver; // qt_cocoa_helpers_mac.mm
extern QPointer<QWidget> qt_button_down; // qapplication_mac.cpp

QT_END_NAMESPACE

QT_FORWARD_DECLARE_CLASS(QDesktopWidgetImplementation)
QT_USE_NAMESPACE

static QT_MANGLE_NAMESPACE(QCocoaApplicationDelegate) *sharedCocoaApplicationDelegate = nil;

static void cleanupCocoaApplicationDelegate()
{
    [sharedCocoaApplicationDelegate release];
}

@implementation QT_MANGLE_NAMESPACE(QCocoaApplicationDelegate)

- (id)init
{
    self = [super init];
    if (self)
        inLaunch = true;
    return self;
}

- (void)dealloc
{
    sharedCocoaApplicationDelegate = nil;
    [dockMenu release];
    [qtMenuLoader release];
    if (reflectionDelegate) {
        [NSApp setDelegate:reflectionDelegate];
        [reflectionDelegate release];
    }
    [super dealloc];
}

+ (id)allocWithZone:(NSZone *)zone
{
    @synchronized(self) {
        if (sharedCocoaApplicationDelegate == nil) {
            sharedCocoaApplicationDelegate = [super allocWithZone:zone];
            return sharedCocoaApplicationDelegate;
            qAddPostRoutine(cleanupCocoaApplicationDelegate);
        }
    }
    return nil;
}

+ (QT_MANGLE_NAMESPACE(QCocoaApplicationDelegate)*)sharedDelegate
{
    @synchronized(self) {
        if (sharedCocoaApplicationDelegate == nil)
            [[self alloc] init];
    }
    return [[sharedCocoaApplicationDelegate retain] autorelease];
}

- (void)setDockMenu:(NSMenu*)newMenu
{
    [newMenu retain];
    [dockMenu release];
    dockMenu = newMenu;
}

- (NSMenu *)applicationDockMenu
{
    return [[dockMenu retain] autorelease];
}

- (QApplicationPrivate *)qAppPrivate
{
    return qtPrivate;
}

- (void)setQtPrivate:(QApplicationPrivate *)value
{
    qtPrivate = value;
}

- (void)setMenuLoader:(QT_MANGLE_NAMESPACE(QCocoaMenuLoader) *)menuLoader
{
    [menuLoader retain];
    [qtMenuLoader release];
    qtMenuLoader = menuLoader;
}

- (QT_MANGLE_NAMESPACE(QCocoaMenuLoader) *)menuLoader
{
    return [[qtMenuLoader retain] autorelease];
}

// This function will only be called when NSApp is actually running. Before
// that, the kAEQuitApplication Apple event will be sent to
// QApplicationPrivate::globalAppleEventProcessor in qapplication_mac.mm
- (NSApplicationTerminateReply)applicationShouldTerminate:(NSApplication *)sender
{
    Q_UNUSED(sender);
    // The reflection delegate gets precedence
    if (reflectionDelegate
        && [reflectionDelegate respondsToSelector:@selector(applicationShouldTerminate:)]) {
        return [reflectionDelegate applicationShouldTerminate:sender];
    }

    if (qtPrivate->canQuit()) {
        if (!startedQuit) {
            startedQuit = true;
            qAppInstance()->quit();
            startedQuit = false;
        }
    }

    if (qtPrivate->threadData->eventLoops.size() == 0) {
        // INVARIANT: No event loop is executing. This probably
        // means that Qt is used as a plugin, or as a part of a native
        // Cocoa application. In any case it should be fine to
        // terminate now:
        return NSTerminateNow;
    }

    return NSTerminateCancel;
}

- (void)applicationDidFinishLaunching:(NSNotification *)aNotification
{
    Q_UNUSED(aNotification);
    inLaunch = false;
    qt_release_apple_event_handler();
}

- (void)application:(NSApplication *)sender openFiles:(NSArray *)filenames
{
    for (NSString *fileName in filenames) {
        QString qtFileName = qt_mac_NSStringToQString(fileName);
        if (inLaunch) {
            // We need to be careful because Cocoa will be nice enough to take
            // command line arguments and send them to us as events. Given the history
            // of Qt Applications, this will result in behavior people don't want, as
            // they might be doing the opening themselves with the command line parsing.
            if (qApp->arguments().contains(qtFileName))
                continue;
        }
        QFileOpenEvent foe(qtFileName);
        qt_sendSpontaneousEvent(qAppInstance(), &foe);
    }

    if (reflectionDelegate &&
        [reflectionDelegate respondsToSelector:@selector(application:openFiles:)])
        [reflectionDelegate application:sender openFiles:filenames];
}

- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication *)sender
{
    // If we have a reflection delegate, that will get to call the shots.
    if (reflectionDelegate
        && [reflectionDelegate respondsToSelector:
                            @selector(applicationShouldTerminateAfterLastWindowClosed:)])
        return [reflectionDelegate applicationShouldTerminateAfterLastWindowClosed:sender];
    return NO; // Someday qApp->quitOnLastWindowClosed(); when QApp and NSApp work closer together.
}


- (void)applicationDidBecomeActive:(NSNotification *)notification
{
    if (reflectionDelegate
        && [reflectionDelegate respondsToSelector:@selector(applicationDidBecomeActive:)])
        [reflectionDelegate applicationDidBecomeActive:notification];

    onApplicationChangedActivation(true);

    if (!QWidget::mouseGrabber()){
        // Update enter/leave immidiatly, don't wait for a move event. But only
        // if no grab exists (even if the grab points to this widget, it seems, ref X11)
        QPoint qlocal, qglobal;
        QWidget *widgetUnderMouse = 0;
        qt_mac_getTargetForMouseEvent(0, QEvent::Enter, qlocal, qglobal, 0, &widgetUnderMouse);
        QApplicationPrivate::dispatchEnterLeave(widgetUnderMouse, 0);
        qt_last_mouse_receiver = widgetUnderMouse;
        qt_last_native_mouse_receiver = widgetUnderMouse ?
            (widgetUnderMouse->internalWinId() ? widgetUnderMouse : widgetUnderMouse->nativeParentWidget()) : 0;
    }
}

- (void)applicationDidResignActive:(NSNotification *)notification
{
    if (reflectionDelegate
        && [reflectionDelegate respondsToSelector:@selector(applicationDidResignActive:)])
        [reflectionDelegate applicationDidResignActive:notification];

    onApplicationChangedActivation(false);

    if (!QWidget::mouseGrabber())
        QApplicationPrivate::dispatchEnterLeave(0, qt_last_mouse_receiver);
    qt_last_mouse_receiver = 0;
    qt_last_native_mouse_receiver = 0;
    qt_button_down = 0;
}

- (void)applicationDidChangeScreenParameters:(NSNotification *)notification
{
    Q_UNUSED(notification);
    QDesktopWidgetImplementation::instance()->onResize();
}

- (void)setReflectionDelegate:(NSObject <NSApplicationDelegate> *)oldDelegate
{
    [oldDelegate retain];
    [reflectionDelegate release];
    reflectionDelegate = oldDelegate;
}

- (NSMethodSignature *)methodSignatureForSelector:(SEL)aSelector
{
    NSMethodSignature *result = [super methodSignatureForSelector:aSelector];
    if (!result && reflectionDelegate) {
        result = [reflectionDelegate methodSignatureForSelector:aSelector];
    }
    return result;
}

- (BOOL)respondsToSelector:(SEL)aSelector
{
    BOOL result = [super respondsToSelector:aSelector];
    if (!result && reflectionDelegate)
        result = [reflectionDelegate respondsToSelector:aSelector];
    return result;
}

- (void)forwardInvocation:(NSInvocation *)invocation
{
    SEL invocationSelector = [invocation selector];
    if (reflectionDelegate && [reflectionDelegate respondsToSelector:invocationSelector])
        [invocation invokeWithTarget:reflectionDelegate];
    else
        [self doesNotRecognizeSelector:invocationSelector];
}

- (void)getUrl:(NSAppleEventDescriptor *)event withReplyEvent:(NSAppleEventDescriptor *)replyEvent
{
    Q_UNUSED(replyEvent);

    NSString *urlString = [[event paramDescriptorForKeyword:keyDirectObject] stringValue];
    QUrl url(qt_mac_NSStringToQString(urlString));
    QFileOpenEvent qtEvent(url);
    qt_sendSpontaneousEvent(qAppInstance(), &qtEvent);
}

- (void)appleEventQuit:(NSAppleEventDescriptor *)event withReplyEvent:(NSAppleEventDescriptor *)replyEvent
{
    Q_UNUSED(event);
    Q_UNUSED(replyEvent);
    [NSApp terminate:self];
}

- (void)qtDispatcherToQAction:(id)sender
{
    [[NSApp QT_MANGLE_NAMESPACE(qt_qcocoamenuLoader)] qtDispatcherToQAction:sender];
}

@end
#endif
