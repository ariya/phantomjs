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

#include "qiosapplicationdelegate.h"

#include "qiosintegration.h"
#include "qiosservices.h"
#include "qiosviewcontroller.h"
#include "qioswindow.h"

#include <QtGui/private/qguiapplication_p.h>
#include <qpa/qplatformintegration.h>

#include <QtCore/QtCore>

@implementation QIOSApplicationDelegate

@synthesize window;

- (BOOL)application:(UIApplication *)application didFinishLaunchingWithOptions:(NSDictionary *)launchOptions
{
    Q_UNUSED(application);
    Q_UNUSED(launchOptions);

    self.window = [[[UIWindow alloc] initWithFrame:[[UIScreen mainScreen] bounds]] autorelease];
    self.window.rootViewController = [[[QIOSViewController alloc] init] autorelease];

#if QT_IOS_DEPLOYMENT_TARGET_BELOW(__IPHONE_7_0)
    QSysInfo::MacVersion iosVersion = QSysInfo::MacintoshVersion;

    // We prefer to keep the root viewcontroller in fullscreen layout, so that
    // we don't have to compensate for the viewcontroller position. This also
    // gives us the same behavior on iOS 5/6 as on iOS 7, where full screen layout
    // is the only way.
    if (iosVersion < QSysInfo::MV_IOS_7_0)
        self.window.rootViewController.wantsFullScreenLayout = YES;

    // Use translucent statusbar by default on iOS6 iPhones (unless the user changed
    // the default in the Info.plist), so that windows placed under the stausbar are
    // still visible, just like on iOS7.
    if (iosVersion >= QSysInfo::MV_IOS_6_0 && iosVersion < QSysInfo::MV_IOS_7_0
        && [UIDevice currentDevice].userInterfaceIdiom == UIUserInterfaceIdiomPhone
        && [UIApplication sharedApplication].statusBarStyle == UIStatusBarStyleDefault)
        [[UIApplication sharedApplication] setStatusBarStyle:UIStatusBarStyleBlackTranslucent];
#endif

    self.window.hidden = NO;

    return YES;
}

- (BOOL)application:(UIApplication *)application openURL:(NSURL *)url sourceApplication:(NSString *)sourceApplication annotation:(id)annotation
{
    Q_UNUSED(application);
    Q_UNUSED(sourceApplication);
    Q_UNUSED(annotation);

    if (!QGuiApplication::instance())
        return NO;

    QIOSIntegration *iosIntegration = static_cast<QIOSIntegration *>(QGuiApplicationPrivate::platformIntegration());
    QIOSServices *iosServices = static_cast<QIOSServices *>(iosIntegration->services());

    return iosServices->handleUrl(QUrl::fromNSURL(url));
}

- (void)dealloc
{
    [window release];
    [super dealloc];
}

@end

