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
#include "qiosintegration.h"
#include "qiosscreen.h"
#include "qioswindow.h"
#include <qpa/qwindowsysteminterface.h>
#include "qiosapplicationdelegate.h"
#include "qiosviewcontroller.h"
#include "quiview.h"

#include <sys/sysctl.h>

// -------------------------------------------------------------------------

static QIOSScreen* qtPlatformScreenFor(UIScreen *uiScreen)
{
    foreach (QScreen *screen, QGuiApplication::screens()) {
        QIOSScreen *platformScreen = static_cast<QIOSScreen *>(screen->handle());
        if (platformScreen->uiScreen() == uiScreen)
            return platformScreen;
    }

    return 0;
}

@interface QIOSScreenTracker : NSObject
@end

@implementation QIOSScreenTracker

+ (void)load
{
    NSNotificationCenter *center = [NSNotificationCenter defaultCenter];
    [center addObserver:self selector:@selector(screenConnected:)
            name:UIScreenDidConnectNotification object:nil];
    [center addObserver:self selector:@selector(screenDisconnected:)
            name:UIScreenDidDisconnectNotification object:nil];
    [center addObserver:self selector:@selector(screenModeChanged:)
            name:UIScreenModeDidChangeNotification object:nil];
}

+ (void)screenConnected:(NSNotification*)notification
{
    QIOSIntegration *integration = QIOSIntegration::instance();
    Q_ASSERT_X(integration, Q_FUNC_INFO, "Screen connected before QIOSIntegration creation");

    integration->addScreen(new QIOSScreen([notification object]));
}

+ (void)screenDisconnected:(NSNotification*)notification
{
    QIOSScreen *screen = qtPlatformScreenFor([notification object]);
    Q_ASSERT_X(screen, Q_FUNC_INFO, "Screen disconnected that we didn't know about");

    delete screen;
}

+ (void)screenModeChanged:(NSNotification*)notification
{
    QIOSScreen *screen = qtPlatformScreenFor([notification object]);
    Q_ASSERT_X(screen, Q_FUNC_INFO, "Screen changed that we didn't know about");

    screen->updateProperties();
}

@end

// -------------------------------------------------------------------------

@interface QIOSOrientationListener : NSObject {
    @public
    QIOSScreen *m_screen;
}
- (id) initWithQIOSScreen:(QIOSScreen *)screen;
@end

@implementation QIOSOrientationListener

- (id) initWithQIOSScreen:(QIOSScreen *)screen
{
    self = [super init];
    if (self) {
        m_screen = screen;
        [[UIDevice currentDevice] beginGeneratingDeviceOrientationNotifications];
        [[NSNotificationCenter defaultCenter]
            addObserver:self
            selector:@selector(orientationChanged:)
            name:@"UIDeviceOrientationDidChangeNotification" object:nil];
    }
    return self;
}

- (void) dealloc
{
    [[UIDevice currentDevice] endGeneratingDeviceOrientationNotifications];
    [[NSNotificationCenter defaultCenter]
        removeObserver:self
        name:@"UIDeviceOrientationDidChangeNotification" object:nil];
    [super dealloc];
}

- (void) orientationChanged:(NSNotification *)notification
{
    Q_UNUSED(notification);

    UIDeviceOrientation deviceOrientation = [UIDevice currentDevice].orientation;
    switch (deviceOrientation) {
    case UIDeviceOrientationFaceUp:
    case UIDeviceOrientationFaceDown:
        // We ignore these events, as iOS will send events with the 'regular'
        // orientations alongside these two orientations.
        return;
    default:
        Qt::ScreenOrientation screenOrientation = toQtScreenOrientation(deviceOrientation);
        QWindowSystemInterface::handleScreenOrientationChange(m_screen->screen(), screenOrientation);
    }
}

@end

// -------------------------------------------------------------------------

/*!
    Returns the model identifier of the device.

    When running under the simulator, the identifier will not
    match the simulated device, but will be x86_64 or i386.
*/
static QString deviceModelIdentifier()
{
    static const char key[] = "hw.machine";

    size_t size;
    sysctlbyname(key, NULL, &size, NULL, 0);

    char value[size];
    sysctlbyname(key, &value, &size, NULL, 0);

    return QString::fromLatin1(value);
}

QIOSScreen::QIOSScreen(UIScreen *screen)
    : QPlatformScreen()
    , m_uiScreen(screen)
    , m_uiWindow(0)
    , m_orientationListener(0)
{
    if (screen == [UIScreen mainScreen]) {
        QString deviceIdentifier = deviceModelIdentifier();

        if (deviceIdentifier == QLatin1String("iPhone2,1") /* iPhone 3GS */
            || deviceIdentifier == QLatin1String("iPod3,1") /* iPod touch 3G */) {
            m_depth = 18;
        } else {
            m_depth = 24;
        }

        if ([[UIDevice currentDevice] userInterfaceIdiom] == UIUserInterfaceIdiomPad
            && !deviceIdentifier.contains(QRegularExpression("^iPad2,[567]$")) /* excluding iPad Mini */) {
            m_unscaledDpi = 132;
        } else {
            m_unscaledDpi = 163; // Regular iPhone DPI
        }
    } else {
        // External display, hard to say
        m_depth = 24;
        m_unscaledDpi = 96;
    }

    for (UIWindow *existingWindow in [[UIApplication sharedApplication] windows]) {
        if (existingWindow.screen == m_uiScreen) {
            m_uiWindow = [m_uiWindow retain];
            break;
        }
    }

    if (!m_uiWindow) {
        // Create a window and associated view-controller that we can use
        m_uiWindow = [[UIWindow alloc] initWithFrame:[m_uiScreen bounds]];
        m_uiWindow.rootViewController = [[[QIOSViewController alloc] initWithQIOSScreen:this] autorelease];

        // FIXME: Only do once windows are added to the screen, and for any screen
        if (screen == [UIScreen mainScreen]) {
            m_uiWindow.screen = m_uiScreen;
            m_uiWindow.hidden = NO;
        }
    }

    updateProperties();
}

QIOSScreen::~QIOSScreen()
{
    [m_orientationListener release];
    [m_uiWindow release];
}

void QIOSScreen::updateProperties()
{
    QRect previousGeometry = m_geometry;
    QRect previousAvailableGeometry = m_availableGeometry;

    UIView *rootView = m_uiWindow.rootViewController.view;

    m_geometry = fromCGRect([rootView convertRect:m_uiScreen.bounds fromView:m_uiWindow]).toRect();
    m_availableGeometry = fromCGRect([rootView convertRect:m_uiScreen.applicationFrame fromView:m_uiWindow]).toRect();

    if (QSysInfo::MacintoshVersion >= QSysInfo::MV_IOS_8_0 && ![m_uiWindow.rootViewController shouldAutorotate]) {
        // Setting the statusbar orientation (content orientation) on iOS8+ will result in the UIScreen
        // updating its geometry and available geometry, which in the case of content orientation is not
        // what we want. We want to reflect the screen geometry based on the locked orientation, and
        // adjust the available geometry based on the repositioned status bar for the current status
        // bar orientation.

        Qt::ScreenOrientation lockedOrientation = toQtScreenOrientation(UIDeviceOrientation(rootView.qtViewController.lockedOrientation));
        Qt::ScreenOrientation contenOrientation = toQtScreenOrientation(UIDeviceOrientation([UIApplication sharedApplication].statusBarOrientation));

        QTransform transform = screen()->transformBetween(lockedOrientation, contenOrientation, m_geometry).inverted();

        m_geometry = transform.mapRect(m_geometry);
        m_availableGeometry = transform.mapRect(m_availableGeometry);
    }

    if (m_geometry != previousGeometry || m_availableGeometry != previousAvailableGeometry) {
        const qreal millimetersPerInch = 25.4;
        m_physicalSize = QSizeF(m_geometry.size()) / m_unscaledDpi * millimetersPerInch;

        QWindowSystemInterface::handleScreenGeometryChange(screen(), m_geometry, m_availableGeometry);
    }
}

QRect QIOSScreen::geometry() const
{
    return m_geometry;
}

QRect QIOSScreen::availableGeometry() const
{
    return m_availableGeometry;
}

int QIOSScreen::depth() const
{
    return m_depth;
}

QImage::Format QIOSScreen::format() const
{
    return QImage::Format_ARGB32_Premultiplied;
}

QSizeF QIOSScreen::physicalSize() const
{
    return m_physicalSize;
}

QDpi QIOSScreen::logicalDpi() const
{
    return QDpi(72, 72);
}

qreal QIOSScreen::devicePixelRatio() const
{
    return [m_uiScreen scale];
}

Qt::ScreenOrientation QIOSScreen::nativeOrientation() const
{
    CGRect nativeBounds =
#if QT_IOS_PLATFORM_SDK_EQUAL_OR_ABOVE(__IPHONE_8_0)
        QSysInfo::MacintoshVersion >= QSysInfo::MV_IOS_8_0 ? m_uiScreen.nativeBounds :
#endif
        m_uiScreen.bounds;

    // All known iOS devices have a native orientation of portrait, but to
    // be on the safe side we compare the width and height of the bounds.
    return nativeBounds.size.width >= nativeBounds.size.height ?
        Qt::LandscapeOrientation : Qt::PortraitOrientation;
}

Qt::ScreenOrientation QIOSScreen::orientation() const
{
    return toQtScreenOrientation([UIDevice currentDevice].orientation);
}

void QIOSScreen::setOrientationUpdateMask(Qt::ScreenOrientations mask)
{
    if (m_orientationListener && mask == Qt::PrimaryOrientation) {
        [m_orientationListener release];
        m_orientationListener = 0;
    } else if (!m_orientationListener) {
        m_orientationListener = [[QIOSOrientationListener alloc] initWithQIOSScreen:this];
    }
}

UIScreen *QIOSScreen::uiScreen() const
{
    return m_uiScreen;
}

UIWindow *QIOSScreen::uiWindow() const
{
    return m_uiWindow;
}

#include "moc_qiosscreen.cpp"

QT_END_NAMESPACE
