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

#include "qiosglobal.h"
#include "qiosapplicationdelegate.h"
#include "qiosviewcontroller.h"
#include "qiosscreen.h"

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(lcQpaInputMethods, "qt.qpa.input.methods");

bool isQtApplication()
{
    // Returns \c true if the plugin is in full control of the whole application. This means
    // that we control the application delegate and the top view controller, and can take
    // actions that impacts all parts of the application. The opposite means that we are
    // embedded inside a native iOS application, and should be more focused on playing along
    // with native UIControls, and less inclined to change structures that lies outside the
    // scope of our QWindows/UIViews.
    static bool isQt = ([[UIApplication sharedApplication].delegate isKindOfClass:[QIOSApplicationDelegate class]]);
    return isQt;
}

CGRect toCGRect(const QRectF &rect)
{
    return CGRectMake(rect.x(), rect.y(), rect.width(), rect.height());
}

QRectF fromCGRect(const CGRect &rect)
{
    return QRectF(rect.origin.x, rect.origin.y, rect.size.width, rect.size.height);
}

CGPoint toCGPoint(const QPointF &point)
{
    return CGPointMake(point.x(), point.y());
}

QPointF fromCGPoint(const CGPoint &point)
{
    return QPointF(point.x, point.y);
}

Qt::ScreenOrientation toQtScreenOrientation(UIDeviceOrientation uiDeviceOrientation)
{
    Qt::ScreenOrientation qtOrientation;
    switch (uiDeviceOrientation) {
    case UIDeviceOrientationPortraitUpsideDown:
        qtOrientation = Qt::InvertedPortraitOrientation;
        break;
    case UIDeviceOrientationLandscapeLeft:
        qtOrientation = Qt::LandscapeOrientation;
        break;
    case UIDeviceOrientationLandscapeRight:
        qtOrientation = Qt::InvertedLandscapeOrientation;
        break;
    case UIDeviceOrientationFaceUp:
    case UIDeviceOrientationFaceDown:
        // FIXME: Use cached device orientation, or fall back to interface orientation
        qtOrientation = Qt::PortraitOrientation;
        break;
    default:
        qtOrientation = Qt::PortraitOrientation;
        break;
    }
    return qtOrientation;
}

UIDeviceOrientation fromQtScreenOrientation(Qt::ScreenOrientation qtOrientation)
{
    UIDeviceOrientation uiOrientation;
    switch (qtOrientation) {
    case Qt::LandscapeOrientation:
        uiOrientation = UIDeviceOrientationLandscapeLeft;
        break;
    case Qt::InvertedLandscapeOrientation:
        uiOrientation = UIDeviceOrientationLandscapeRight;
        break;
    case Qt::InvertedPortraitOrientation:
        uiOrientation = UIDeviceOrientationPortraitUpsideDown;
        break;
    case Qt::PrimaryOrientation:
    case Qt::PortraitOrientation:
    default:
        uiOrientation = UIDeviceOrientationPortrait;
        break;
    }
    return uiOrientation;
}

int infoPlistValue(NSString* key, int defaultValue)
{
    static NSBundle *bundle = [NSBundle mainBundle];
    NSNumber* value = [bundle objectForInfoDictionaryKey:key];
    return value ? [value intValue] : defaultValue;
}

// -------------------------------------------------------------------------

@interface QtFirstResponderEvent : UIEvent
@property (nonatomic, strong) id firstResponder;
@end

@implementation QtFirstResponderEvent
- (void) dealloc
{
    self.firstResponder = 0;
    [super dealloc];
}
@end


@implementation UIView (QtFirstResponder)
- (UIView*)qt_findFirstResponder
{
    if ([self isFirstResponder])
        return self;

    for (UIView *subview in self.subviews) {
        if (UIView *firstResponder = [subview qt_findFirstResponder])
            return firstResponder;
    }

    return nil;
}
@end

@implementation UIResponder (QtFirstResponder)

+(id)currentFirstResponder
{
    QtFirstResponderEvent *event = [[[QtFirstResponderEvent alloc] init] autorelease];
    [[UIApplication sharedApplication] sendAction:@selector(qt_findFirstResponder:event:) to:nil from:nil forEvent:event];
    return event.firstResponder;
}

- (void)qt_findFirstResponder:(id)sender event:(QtFirstResponderEvent *)event
{
    Q_UNUSED(sender);

    if ([self isKindOfClass:[UIView class]])
        event.firstResponder = [static_cast<UIView *>(self) qt_findFirstResponder];
    else
        event.firstResponder = [self isFirstResponder] ? self : nil;
}
@end

FirstResponderCandidate::FirstResponderCandidate(UIResponder *responder)
    : QScopedValueRollback<UIResponder *>(s_firstResponderCandidate, responder)
{
}

UIResponder *FirstResponderCandidate::s_firstResponderCandidate = 0;

QT_END_NAMESPACE

