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

#include "qiosapplicationstate.h"

#include <qpa/qwindowsysteminterface.h>
#include <QtCore/qcoreapplication.h>

#import <UIKit/UIKit.h>

@interface QIOSApplicationStateListener : NSObject
@end

@implementation QIOSApplicationStateListener

- (id) init
{
    self = [super init];
    if (self) {
        // Listen for application state changes.
        // Note: We use notifications rather than application delegate callbacks to
        // also support hybrid applications were QIOSApplicationDelegate is not in use.
        [[NSNotificationCenter defaultCenter]
            addObserver:self
            selector:@selector(applicationDidBecomeActive)
            name:UIApplicationDidBecomeActiveNotification
            object:nil];
        [[NSNotificationCenter defaultCenter]
            addObserver:self
            selector:@selector(applicationWillResignActive)
            name:UIApplicationWillResignActiveNotification
            object:nil];
        [[NSNotificationCenter defaultCenter]
            addObserver:self
            selector:@selector(applicationDidEnterBackground)
            name:UIApplicationDidEnterBackgroundNotification
            object:nil];

        // Update the current state now, since we have missed all the updates
        // posted from AppKit so far. But let QPA finish initialization first.
        dispatch_async(dispatch_get_main_queue(), ^{
            [self handleApplicationStateChanged:[UIApplication sharedApplication].applicationState];
        });
    }
    return self;
}

- (void) dealloc
{
    [[NSNotificationCenter defaultCenter]
        removeObserver:self
        name:UIApplicationDidBecomeActiveNotification
        object:nil];
    [[NSNotificationCenter defaultCenter]
        removeObserver:self
        name:UIApplicationWillResignActiveNotification
        object:nil];
    [[NSNotificationCenter defaultCenter]
        removeObserver:self
        name:UIApplicationDidEnterBackgroundNotification
        object:nil];
    [super dealloc];
}

- (void) applicationDidBecomeActive
{
    [self handleApplicationStateChanged:UIApplicationStateActive];
}

- (void) applicationWillResignActive
{
    // Note that UIApplication is still UIApplicationStateActive at this
    // point, but since there is no separate notification for the inactive
    // state, we report UIApplicationStateInactive now:
    [self handleApplicationStateChanged:UIApplicationStateInactive];
}

- (void) applicationDidEnterBackground
{
    [self handleApplicationStateChanged:UIApplicationStateBackground];
}

- (void) handleApplicationStateChanged:(UIApplicationState) uiApplicationState
{
    // We may receive application state changes after QCoreApplication has
    // gone down, as the block we schedule on the main queue keeps the
    // listener alive. In that case we just ignore the notification.
    if (!qApp)
        return;

    Qt::ApplicationState state;
    switch (uiApplicationState) {
    case UIApplicationStateActive:
        // The application is visible in front, and receiving events:
        state = Qt::ApplicationActive;
        break;
    case UIApplicationStateInactive:
        // The app is running in the foreground but is not receiving events. This
        // typically happens while transitioning to/from active/background, like
        // upon app launch or when receiving incoming calls:
        state = Qt::ApplicationInactive;
        break;
    case UIApplicationStateBackground:
        // Normally the app would enter this state briefly before it gets
        // suspeded (you have five seconds, according to Apple).
        // You can request more time and start a background task, which would
        // normally map closer to Qt::ApplicationHidden. But since we have no
        // API for doing that yet, we handle this state as "about to be suspended".
        // Note: A screen-shot for the SpringBoard will also be taken after this
        // call returns.
        state = Qt::ApplicationSuspended;
        break;
    }
    QWindowSystemInterface::handleApplicationStateChanged(state);
    QWindowSystemInterface::flushWindowSystemEvents();
}

@end

QT_BEGIN_NAMESPACE

QIOSApplicationState::QIOSApplicationState()
    : m_listener([[QIOSApplicationStateListener alloc] init])
{
}

QIOSApplicationState::~QIOSApplicationState()
{
    [m_listener release];
}

QT_END_NAMESPACE

