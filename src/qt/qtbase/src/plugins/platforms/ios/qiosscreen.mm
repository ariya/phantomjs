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
#include "qiosscreen.h"
#include "qioswindow.h"
#include <qpa/qwindowsysteminterface.h>
#include "qiosapplicationdelegate.h"
#include "qiosviewcontroller.h"

#include <sys/sysctl.h>

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

QIOSScreen::QIOSScreen(unsigned int screenIndex)
    : QPlatformScreen()
    , m_uiScreen([[UIScreen screens] count] > screenIndex
        ? [[UIScreen screens] objectAtIndex:screenIndex]
        : [UIScreen mainScreen])
    , m_orientationListener(0)
{
    QString deviceIdentifier = deviceModelIdentifier();

    if (deviceIdentifier == QStringLiteral("iPhone2,1") /* iPhone 3GS */
        || deviceIdentifier == QStringLiteral("iPod3,1") /* iPod touch 3G */) {
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

    connect(qGuiApp, &QGuiApplication::focusWindowChanged, this, &QIOSScreen::updateStatusBarVisibility);

    updateProperties();
}

QIOSScreen::~QIOSScreen()
{
    [m_orientationListener release];
}

void QIOSScreen::updateProperties()
{
    UIWindow *uiWindow = 0;
    for (uiWindow in [[UIApplication sharedApplication] windows]) {
        if (uiWindow.screen == m_uiScreen)
            break;
    }

    bool inPortrait = UIInterfaceOrientationIsPortrait(uiWindow.rootViewController.interfaceOrientation);
    QRect geometry = inPortrait ? fromCGRect(m_uiScreen.bounds).toRect()
        : QRect(m_uiScreen.bounds.origin.x, m_uiScreen.bounds.origin.y,
            m_uiScreen.bounds.size.height, m_uiScreen.bounds.size.width);

    if (geometry != m_geometry) {
        m_geometry = geometry;

        const qreal millimetersPerInch = 25.4;
        m_physicalSize = QSizeF(m_geometry.size()) / m_unscaledDpi * millimetersPerInch;

        QWindowSystemInterface::handleScreenGeometryChange(screen(), m_geometry);
    }

    QRect availableGeometry = geometry;

    CGSize applicationFrameSize = m_uiScreen.applicationFrame.size;
    int statusBarHeight = geometry.height() - (inPortrait ? applicationFrameSize.height : applicationFrameSize.width);

    availableGeometry.adjust(0, statusBarHeight, 0, 0);

    if (availableGeometry != m_availableGeometry) {
        m_availableGeometry = availableGeometry;
        QWindowSystemInterface::handleScreenAvailableGeometryChange(screen(), m_availableGeometry);
    }

    if (screen())
        layoutWindows();
}

void QIOSScreen::updateStatusBarVisibility()
{
    if (!isQtApplication())
        return;

    QWindow *focusWindow = QGuiApplication::focusWindow();

    // If we don't have a focus window we leave the status
    // bar as is, so that the user can activate a new window
    // with the same window state without the status bar jumping
    // back and forth.
    if (!focusWindow)
        return;

    UIView *view = reinterpret_cast<UIView *>(focusWindow->handle()->winId());
    QIOSViewController *viewController = static_cast<QIOSViewController *>(view.viewController);

    bool currentStatusBarVisibility = [UIApplication sharedApplication].statusBarHidden;
    if (viewController.prefersStatusBarHidden == currentStatusBarVisibility)
        return;

#if QT_IOS_PLATFORM_SDK_EQUAL_OR_ABOVE(__IPHONE_7_0)
    if (QSysInfo::MacintoshVersion >= QSysInfo::MV_IOS_7_0) {
        [viewController setNeedsStatusBarAppearanceUpdate];
        dispatch_async(dispatch_get_main_queue(), ^{
            updateProperties();
        });
    } else
#endif
    {
        [[UIApplication sharedApplication]
            setStatusBarHidden:[viewController prefersStatusBarHidden]
            withAnimation:UIStatusBarAnimationNone];

        updateProperties();
    }
}

void QIOSScreen::layoutWindows()
{
    QList<QWindow*> windows = QGuiApplication::topLevelWindows();

    const QRect oldGeometry = screen()->geometry();
    const QRect oldAvailableGeometry = screen()->availableGeometry();
    const QRect newGeometry = geometry();
    const QRect newAvailableGeometry = availableGeometry();

    for (int i = 0; i < windows.size(); ++i) {
        QWindow *window = windows.at(i);

        if (platformScreenForWindow(window) != this)
            continue;

        QIOSWindow *platformWindow = static_cast<QIOSWindow *>(window->handle());
        if (!platformWindow)
            continue;

        // FIXME: Handle more complex cases of no-state and/or child windows when rotating

        if (window->windowState() & Qt::WindowFullScreen
                || (window->windowState() & Qt::WindowNoState && window->geometry() == oldGeometry))
            platformWindow->applyGeometry(newGeometry);
        else if (window->windowState() & Qt::WindowMaximized
                || (window->windowState() & Qt::WindowNoState && window->geometry() == oldAvailableGeometry))
            platformWindow->applyGeometry(newAvailableGeometry);
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
    // A UIScreen stays in the native orientation, regardless of rotation
    return m_uiScreen.bounds.size.width >= m_uiScreen.bounds.size.height ?
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

#include "moc_qiosscreen.cpp"

QT_END_NAMESPACE
