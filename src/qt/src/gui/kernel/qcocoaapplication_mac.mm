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

#include <qglobal.h>
#ifdef QT_MAC_USE_COCOA
#include <private/qcocoaapplication_mac_p.h>
#include <private/qcocoaapplicationdelegate_mac_p.h>
#include <private/qt_cocoa_helpers_mac_p.h>
#include <private/qcocoaintrospection_p.h>

QT_USE_NAMESPACE

@implementation NSApplication (QT_MANGLE_NAMESPACE(QApplicationIntegration))

- (void)QT_MANGLE_NAMESPACE(qt_setDockMenu):(NSMenu *)newMenu
{
    [[QT_MANGLE_NAMESPACE(QCocoaApplicationDelegate) sharedDelegate] setDockMenu:newMenu];
}

- (QApplicationPrivate *)QT_MANGLE_NAMESPACE(qt_qappPrivate)
{
    return [[QT_MANGLE_NAMESPACE(QCocoaApplicationDelegate) sharedDelegate] qAppPrivate];
}

- (QT_MANGLE_NAMESPACE(QCocoaMenuLoader) *)QT_MANGLE_NAMESPACE(qt_qcocoamenuLoader)
{
    return [[QT_MANGLE_NAMESPACE(QCocoaApplicationDelegate) sharedDelegate] menuLoader];
}

- (int)QT_MANGLE_NAMESPACE(qt_validModesForFontPanel):(NSFontPanel *)fontPanel
{
    Q_UNUSED(fontPanel);
    // only display those things that QFont can handle
    return NSFontPanelFaceModeMask
            | NSFontPanelSizeModeMask
            | NSFontPanelCollectionModeMask
            | NSFontPanelUnderlineEffectModeMask
            | NSFontPanelStrikethroughEffectModeMask;
}

- (void)qt_sendPostedMessage:(NSEvent *)event
{
    // WARNING: data1 and data2 is truncated to from 64-bit to 32-bit on OS 10.5! 
    // That is why we need to split the address in two parts:
    quint64 lower = [event data1];
    quint64 upper = [event data2];
    QCocoaPostMessageArgs *args = reinterpret_cast<QCocoaPostMessageArgs *>(lower | (upper << 32));
    // Special case for convenience: if the argument is an NSNumber, we unbox it directly.
    // Use NSValue instead if this behaviour is unwanted.
    id a1 = ([args->arg1 isKindOfClass:[NSNumber class]]) ? (id)[args->arg1 intValue] : args->arg1;
    id a2 = ([args->arg2 isKindOfClass:[NSNumber class]]) ? (id)[args->arg2 intValue] : args->arg2;
    switch (args->argCount) {
    case 0:
        [args->target performSelector:args->selector];
        break;
    case 1:
        [args->target performSelector:args->selector withObject:a1];
        break;
    case 3:
        [args->target performSelector:args->selector withObject:a1 withObject:a2];
        break;
    }

    delete args;
}

- (BOOL)qt_filterEvent:(NSEvent *)event
{
    if (qApp->macEventFilter(0, reinterpret_cast<EventRef>(event)))
        return true;

    if ([event type] == NSApplicationDefined) {
        switch ([event subtype]) {
            case QtCocoaEventSubTypePostMessage:
                [NSApp qt_sendPostedMessage:event];
                return true;
            default:
                break;
        }
    }
    return false;
}

@end

@implementation QNSApplication

- (void)qt_sendEvent_original:(NSEvent *)event
{
    Q_UNUSED(event);
    // This method will only be used as a signature
    // template for the method we add into NSApplication
    // containing the original [NSApplication sendEvent:] implementation
}

- (void)qt_sendEvent_replacement:(NSEvent *)event
{
    // This method (or its implementation to be precise) will
    // be called instead of sendEvent if redirection occurs.
    // 'self' will then be an instance of NSApplication
    // (and not QNSApplication)
    if (![NSApp qt_filterEvent:event])
        [self qt_sendEvent_original:event];
}

- (void)sendEvent:(NSEvent *)event
{
    // This method will be called if
    // no redirection occurs
    if (![NSApp qt_filterEvent:event])
        [super sendEvent:event];
}

- (void)qtDispatcherToQAction:(id)sender
{
    // Forward actions sendt from the menu bar (e.g. quit) to the menu loader.
    // Having this method here means that we are the last stop in the responder
    // chain, and that we are able to handle menu actions even when no window is
    // visible on screen. Note: If Qt is used as a plugin, Qt will not use a 
    // native menu bar. Hence, we will also not need to do any redirection etc. as 
    // we do with sendEvent.
    [[NSApp QT_MANGLE_NAMESPACE(qt_qcocoamenuLoader)] qtDispatcherToQAction:sender];
}

@end

QT_BEGIN_NAMESPACE

void qt_redirectNSApplicationSendEvent()
{
    if ([NSApp isMemberOfClass:[QNSApplication class]]) {
        // No need to change implementation since Qt
        // already controls a subclass of NSApplication
        return;
    }

    // Change the implementation of [NSApplication sendEvent] to the
    // implementation of qt_sendEvent_replacement found in QNSApplication.
    // And keep the old implementation that gets overwritten inside a new
    // method 'qt_sendEvent_original' that we add to NSApplication
    qt_cocoa_change_implementation(
            [NSApplication class],
            @selector(sendEvent:),
            [QNSApplication class],
            @selector(qt_sendEvent_replacement:),
            @selector(qt_sendEvent_original:));
 }

QT_END_NAMESPACE
#endif
