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
#import <private/qcocoawindow_mac_p.h>
#import <private/qcocoawindowdelegate_mac_p.h>
#import <private/qcocoaview_mac_p.h>
#import <private/qt_cocoa_helpers_mac_p.h>
#import <private/qcocoawindowcustomthemeframe_mac_p.h>
#import <private/qcocoaapplication_mac_p.h>
#import <private/qdnd_p.h>
#import <private/qmultitouch_mac_p.h>

#include <QtGui/QWidget>

QT_FORWARD_DECLARE_CLASS(QWidget);
QT_USE_NAMESPACE

@implementation NSWindow (QT_MANGLE_NAMESPACE(QWidgetIntegration))

- (id)QT_MANGLE_NAMESPACE(qt_initWithQWidget):(QWidget*)widget contentRect:(NSRect)rect styleMask:(NSUInteger)mask
{
    self = [self initWithContentRect:rect styleMask:mask backing:NSBackingStoreBuffered defer:YES];
    if (self) {
        [[QT_MANGLE_NAMESPACE(QCocoaWindowDelegate) sharedDelegate] becomeDelegteForWindow:self widget:widget];
        [self setReleasedWhenClosed:NO];
    }
    return self;
}

- (QWidget *)QT_MANGLE_NAMESPACE(qt_qwidget)
{
    QWidget *widget = 0;
    if ([self delegate] == [QT_MANGLE_NAMESPACE(QCocoaWindowDelegate) sharedDelegate])
        widget = [[QT_MANGLE_NAMESPACE(QCocoaWindowDelegate) sharedDelegate] qt_qwidgetForWindow:self];
    return widget;
}

@end

@implementation QT_MANGLE_NAMESPACE(QCocoaWindow)

/***********************************************************************
  Copy and Paste between QCocoaWindow and QCocoaPanel
  This is a bit unfortunate, but thanks to the dynamic dispatch we
  have to duplicate this code or resort to really silly forwarding methods
**************************************************************************/
#include "qcocoasharedwindowmethods_mac_p.h"

@end
#endif
