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

#include "qmacdefines_mac.h"
#ifdef QT_MAC_USE_COCOA
#import <Cocoa/Cocoa.h>

QT_FORWARD_DECLARE_CLASS(QMenu)
QT_FORWARD_DECLARE_CLASS(QAction)

#if MAC_OS_X_VERSION_MAX_ALLOWED <= MAC_OS_X_VERSION_10_5

@protocol NSMenuDelegate <NSObject>
- (void)menu:(NSMenu*)menu willHighlightItem:(NSMenuItem*)item;
- (void)menuWillOpen:(NSMenu*)menu;
- (void)menuDidClose:(NSMenu*)menu;
- (BOOL)hasShortcut:(NSMenu *)menu forKey:(NSString *)key forModifiers:(NSUInteger)modifier
  whichItem:(NSMenuItem**)outItem;
@end

#endif

@interface QT_MANGLE_NAMESPACE(QCocoaMenu) : NSMenu <NSMenuDelegate>
{
    QMenu *qmenu;
    QAction *previousAction;
}
- (id)initWithQMenu:(QMenu*)menu;
- (BOOL)menuHasKeyEquivalent:(NSMenu *)menu forEvent:(NSEvent *)event target:(id *)target action:(SEL *)action;
- (NSInteger)indexOfItemWithTarget:(id)anObject andAction:(SEL)actionSelector;
@end
#endif

