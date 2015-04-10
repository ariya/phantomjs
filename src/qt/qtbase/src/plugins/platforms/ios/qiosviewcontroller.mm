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

#import "qiosviewcontroller.h"

#include <QtGui/QGuiApplication>
#include <QtGui/QWindow>
#include <QtGui/QScreen>

#include <QtGui/private/qwindow_p.h>

#include "qiosscreen.h"
#include "qiosglobal.h"
#include "qioswindow.h"

@implementation QIOSViewController

-(BOOL)shouldAutorotate
{
    // Until a proper orientation and rotation API is in place, we always auto rotate.
    // If auto rotation is not wanted, you would need to switch it off manually from Info.plist.
    return YES;
}

#if QT_IOS_PLATFORM_SDK_EQUAL_OR_ABOVE(__IPHONE_6_0)
-(NSUInteger)supportedInterfaceOrientations
{
    // We need to tell iOS that we support all orientations in order to set
    // status bar orientation when application content orientation changes.
    return UIInterfaceOrientationMaskAll;
}
#endif

#if QT_IOS_DEPLOYMENT_TARGET_BELOW(__IPHONE_6_0)
-(BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation
{
    Q_UNUSED(interfaceOrientation);
    return YES;
}
#endif

- (void)willAnimateRotationToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation duration:(NSTimeInterval)duration
{
    Q_UNUSED(duration);
    Q_UNUSED(interfaceOrientation);

    if (!QCoreApplication::instance())
        return; // FIXME: Store orientation for later (?)

    QIOSScreen *qiosScreen = static_cast<QIOSScreen *>(QGuiApplication::primaryScreen()->handle());
    qiosScreen->updateProperties();
}

#if QT_IOS_PLATFORM_SDK_EQUAL_OR_ABOVE(__IPHONE_7_0)
- (UIStatusBarStyle)preferredStatusBarStyle
{
    // Since we don't place anything behind the status bare by default, we
    // end up with a black area, so we have to enable the white text mode
    // of the iOS7 statusbar.
    return UIStatusBarStyleLightContent;

    // FIXME: Try to detect the content underneath the statusbar and choose
    // an appropriate style, and/or expose Qt APIs to control the style.
}
#endif

- (BOOL)prefersStatusBarHidden
{
    static bool hiddenFromPlist = infoPlistValue(@"UIStatusBarHidden", false);
    if (hiddenFromPlist)
        return YES;
    QWindow *focusWindow = QGuiApplication::focusWindow();
    if (!focusWindow)
        return [UIApplication sharedApplication].statusBarHidden;

    return qt_window_private(focusWindow)->topLevelWindow()->windowState() == Qt::WindowFullScreen;
}

@end

