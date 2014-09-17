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

#include <qdialogbuttonbox.h>
#if defined(Q_WS_MAC)
#include <private/qt_mac_p.h>
#include <private/qcocoaintrospection_p.h>
#import <AppKit/AppKit.h>
#import <Foundation/Foundation.h>
#import <objc/objc-class.h>

QT_BEGIN_NAMESPACE
static QWidget *currentWindow = 0;
QT_END_NAMESPACE

QT_USE_NAMESPACE

@class QT_MANGLE_NAMESPACE(QNSPanelProxy);

@interface QT_MANGLE_NAMESPACE(QNSPanelProxy) : NSWindow {
}
- (id)initWithContentRect:(NSRect)contentRect styleMask:(NSUInteger)windowStyle
    backing:(NSBackingStoreType)bufferingType defer:(BOOL)deferCreation;
- (id)initWithContentRect:(NSRect)contentRect styleMask:(NSUInteger)windowStyle
    backing:(NSBackingStoreType)bufferingType defer:(BOOL)deferCreation screen:(NSScreen *)screen;
- (id)qt_fakeInitWithContentRect:(NSRect)contentRect styleMask:(NSUInteger)windowStyle
    backing:(NSBackingStoreType)bufferingType defer:(BOOL)deferCreation;
- (id)qt_fakeInitWithContentRect:(NSRect)contentRect styleMask:(NSUInteger)windowStyle
    backing:(NSBackingStoreType)bufferingType defer:(BOOL)deferCreation screen:(NSScreen *)screen;
@end

@implementation QT_MANGLE_NAMESPACE(QNSPanelProxy)
- (id)initWithContentRect:(NSRect)contentRect styleMask:(NSUInteger)windowStyle
      backing:(NSBackingStoreType)bufferingType defer:(BOOL)deferCreation
{
    // remove evil flag
    windowStyle &= ~NSUtilityWindowMask;
	self = [self qt_fakeInitWithContentRect:contentRect styleMask:windowStyle
	                                backing:bufferingType defer:deferCreation];
    return self;
}

- (id)initWithContentRect:(NSRect)contentRect styleMask:(NSUInteger)windowStyle
      backing:(NSBackingStoreType)bufferingType defer:(BOOL)deferCreation screen:(NSScreen *)screen
{
    // remove evil flag
    windowStyle &= ~NSUtilityWindowMask;
	return [self qt_fakeInitWithContentRect:contentRect styleMask:windowStyle
	                                backing:bufferingType defer:deferCreation screen:screen];
}

- (id)qt_fakeInitWithContentRect:(NSRect)contentRect styleMask:(NSUInteger)windowStyle
    backing:(NSBackingStoreType)bufferingType defer:(BOOL)deferCreation
{
    Q_UNUSED(contentRect);
    Q_UNUSED(windowStyle);
    Q_UNUSED(bufferingType);
    Q_UNUSED(deferCreation);
    return nil;
}

- (id)qt_fakeInitWithContentRect:(NSRect)contentRect styleMask:(NSUInteger)windowStyle
    backing:(NSBackingStoreType)bufferingType defer:(BOOL)deferCreation screen:(NSScreen *)screen
{
    Q_UNUSED(contentRect);
    Q_UNUSED(windowStyle);
    Q_UNUSED(bufferingType);
    Q_UNUSED(deferCreation);
    Q_UNUSED(screen);
    return nil;
}
@end

@class QT_MANGLE_NAMESPACE(QNSWindowProxy);

@interface QT_MANGLE_NAMESPACE(QNSWindowProxy) : NSWindow {
}
- (void)setTitle:(NSString *)title;
- (void)qt_fakeSetTitle:(NSString *)title;
@end

@implementation QT_MANGLE_NAMESPACE(QNSWindowProxy)
- (void)setTitle:(NSString *)title
{
    QCFString cftitle(currentWindow->windowTitle());

    // evil reverse engineering
    if ([title isEqualToString:@"Print"]
            || [title isEqualToString:@"Page Setup"]
            || [[self className] isEqualToString:@"PMPrintingWindow"])
        title = (NSString *)(static_cast<CFStringRef>(cftitle));
    return [self qt_fakeSetTitle:title];
}

- (void)qt_fakeSetTitle:(NSString *)title
{
    Q_UNUSED(title);
}
@end

QT_BEGIN_NAMESPACE

/*
    Intercept the NSColorPanel constructor if the shared
    color panel doesn't exist yet. What's going on here is
    quite wacky, because we want to override the NSPanel
    constructor and at the same time call the old NSPanel
    constructor. So what we do is we effectively rename the
    old NSPanel constructor qt_fakeInitWithContentRect:...
    and have the new one call the old one.
*/
void macStartInterceptNSPanelCtor()
{
    qt_cocoa_change_implementation(
            [NSPanel class],
            @selector(initWithContentRect:styleMask:backing:defer:),
            [QT_MANGLE_NAMESPACE(QNSPanelProxy) class],
            @selector(initWithContentRect:styleMask:backing:defer:),
            @selector(qt_fakeInitWithContentRect:styleMask:backing:defer:));
    qt_cocoa_change_implementation(
            [NSPanel class],
            @selector(initWithContentRect:styleMask:backing:defer:screen:),
            [QT_MANGLE_NAMESPACE(QNSPanelProxy) class],
            @selector(initWithContentRect:styleMask:backing:defer:screen:),
            @selector(qt_fakeInitWithContentRect:styleMask:backing:defer:screen:));
}

/*
    Restore things as they were.
*/
void macStopInterceptNSPanelCtor()
{
    qt_cocoa_change_back_implementation(
            [NSPanel class],
            @selector(initWithContentRect:styleMask:backing:defer:screen:),
            @selector(qt_fakeInitWithContentRect:styleMask:backing:defer:screen:));
    qt_cocoa_change_back_implementation(
            [NSPanel class],
            @selector(initWithContentRect:styleMask:backing:defer:),
            @selector(qt_fakeInitWithContentRect:styleMask:backing:defer:));
}

/*
    Intercept the NSPrintPanel and NSPageLayout setTitle: calls. The
    hack is similar as for NSColorPanel above.
*/
void macStartInterceptWindowTitle(QWidget *window)
{
    currentWindow = window;
    qt_cocoa_change_implementation(
            [NSWindow class],
            @selector(setTitle:),
            [QT_MANGLE_NAMESPACE(QNSWindowProxy) class],
            @selector(setTitle:),
            @selector(qt_fakeSetTitle:));
}

/*
    Restore things as they were.
*/
void macStopInterceptWindowTitle()
{
    currentWindow = 0;
    qt_cocoa_change_back_implementation(
            [NSWindow class],
            @selector(setTitle:),
            @selector(qt_fakeSetTitle:));
}

/*
    Doesn't really belong in here.
*/
NSButton *macCreateButton(const char *text, NSView *superview)
{
    static const NSRect buttonFrameRect = { { 0.0, 0.0 }, { 0.0, 0.0 } };

    NSButton *button = [[NSButton alloc] initWithFrame:buttonFrameRect];
    [button setButtonType:NSMomentaryLightButton];
    [button setBezelStyle:NSRoundedBezelStyle];
    [button setTitle:(NSString*)(CFStringRef)QCFString(QDialogButtonBox::tr(text)
                                                       .remove(QLatin1Char('&')))];
    [[button cell] setFont:[NSFont systemFontOfSize:
            [NSFont systemFontSizeForControlSize:NSRegularControlSize]]];
    [superview addSubview:button];
    return button;
}

QT_END_NAMESPACE

#endif
