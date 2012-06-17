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

#ifndef QCOCOAWINDOW_MAC_P
#define QCOCOAWINDOW_MAC_P

#ifdef QT_MAC_USE_COCOA
#include "qmacdefines_mac.h"
#import <Cocoa/Cocoa.h>
#include <private/qapplication_p.h>
#include <private/qbackingstore_p.h>

enum { QtMacCustomizeWindow = 1 << 21 }; // This will one day be run over by

QT_FORWARD_DECLARE_CLASS(QWidget);
QT_FORWARD_DECLARE_CLASS(QStringList);
QT_FORWARD_DECLARE_CLASS(QCocoaDropData);

@interface NSWindow (QtCoverForHackWithCategory)
+ (Class)frameViewClassForStyleMask:(NSUInteger)styleMask;
@end

@interface NSWindow (QT_MANGLE_NAMESPACE(QWidgetIntegration))
- (id)QT_MANGLE_NAMESPACE(qt_initWithQWidget):(QWidget *)widget contentRect:(NSRect)rect styleMask:(NSUInteger)mask;
- (QWidget *)QT_MANGLE_NAMESPACE(qt_qwidget);
@end

@interface NSWindow (QtIntegration)
- (NSDragOperation)draggingEntered:(id <NSDraggingInfo>)sender;
- (NSDragOperation)draggingUpdated:(id <NSDraggingInfo>)sender;
- (void)draggingExited:(id <NSDraggingInfo>)sender;
- (BOOL)performDragOperation:(id <NSDraggingInfo>)sender;
@end

@interface QT_MANGLE_NAMESPACE(QCocoaWindow) : NSWindow {
    QStringList *currentCustomDragTypes;
    QCocoaDropData *dropData;
    NSInteger dragEnterSequence;
}

+ (Class)frameViewClassForStyleMask:(NSUInteger)styleMask;
- (void)registerDragTypes;
- (void)drawRectOriginal:(NSRect)rect;

@end
#endif

#endif
