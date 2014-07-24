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
#include "qioswindow.h"
#include "quiview.h"
#include "qioscontext.h"
#include "qiosinputcontext.h"
#include "qiosscreen.h"
#include "qiosapplicationdelegate.h"
#include "qiosviewcontroller.h"
#include "qiosintegration.h"
#include <QtGui/private/qguiapplication_p.h>
#include <QtGui/private/qwindow_p.h>
#include <qpa/qplatformintegration.h>

#import <QuartzCore/CAEAGLLayer.h>

#include <QtGui/QKeyEvent>
#include <qpa/qwindowsysteminterface.h>

#include <QtDebug>

// Include category as an alternative to using -ObjC (Apple QA1490)
#include "quiview_textinput.mm"

@implementation QUIView

@synthesize autocapitalizationType;
@synthesize autocorrectionType;
@synthesize enablesReturnKeyAutomatically;
@synthesize keyboardAppearance;
@synthesize keyboardType;
@synthesize returnKeyType;
@synthesize secureTextEntry;

+ (Class)layerClass
{
    return [CAEAGLLayer class];
}

-(id)initWithQIOSWindow:(QIOSWindow *)window
{
    if (self = [self initWithFrame:toCGRect(window->geometry())])
        m_qioswindow = window;

    return self;
}

- (id)initWithFrame:(CGRect)frame
{
    if ((self = [super initWithFrame:frame])) {
        // Set up EAGL layer
        CAEAGLLayer *eaglLayer = static_cast<CAEAGLLayer *>(self.layer);
        eaglLayer.opaque = TRUE;
        eaglLayer.drawableProperties = [NSDictionary dictionaryWithObjectsAndKeys:
            [NSNumber numberWithBool:YES], kEAGLDrawablePropertyRetainedBacking,
            kEAGLColorFormatRGBA8, kEAGLDrawablePropertyColorFormat, nil];

        if (isQtApplication())
            self.hidden = YES;

        self.multipleTouchEnabled = YES;
        m_inSendEventToFocusObject = NO;
    }

    return self;
}

- (void)willMoveToWindow:(UIWindow *)newWindow
{
    // UIKIt will normally set the scale factor of a view to match the corresponding
    // screen scale factor, but views backed by CAEAGLLayers need to do this manually.
    self.contentScaleFactor = newWindow && newWindow.screen ?
        newWindow.screen.scale : [[UIScreen mainScreen] scale];

    // FIXME: Allow the scale factor to be customized through QSurfaceFormat.
}

- (void)didAddSubview:(UIView *)subview
{
    if ([subview isKindOfClass:[QUIView class]])
        self.clipsToBounds = YES;
}

- (void)willRemoveSubview:(UIView *)subview
{
    for (UIView *view in self.subviews) {
        if (view != subview && [view isKindOfClass:[QUIView class]])
            return;
    }

    self.clipsToBounds = NO;
}

- (void)setNeedsDisplay
{
    [super setNeedsDisplay];

    // We didn't implement drawRect: so we have to manually
    // mark the layer as needing display.
    [self.layer setNeedsDisplay];
}

- (void)layoutSubviews
{
    // This method is the de facto way to know that view has been resized,
    // or otherwise needs invalidation of its buffers. Note though that we
    // do not get this callback when the view just changes its position, so
    // the position of our QWindow (and platform window) will only get updated
    // when the size is also changed.

    if (!CGAffineTransformIsIdentity(self.transform))
        qWarning() << m_qioswindow->window()
            << "is backed by a UIView that has a transform set. This is not supported.";

    // The original geometry requested by setGeometry() might be different
    // from what we end up with after applying window constraints.
    QRect requestedGeometry = m_qioswindow->geometry();

    QRect actualGeometry;
    if (m_qioswindow->window()->isTopLevel()) {
        UIWindow *uiWindow = self.window;
        UIView *rootView = uiWindow.rootViewController.view;
        CGRect rootViewPositionInRelationToRootViewController =
            [rootView convertRect:uiWindow.bounds fromView:uiWindow];

        actualGeometry = fromCGRect(CGRectOffset([self.superview convertRect:self.frame toView:rootView],
                                    -rootViewPositionInRelationToRootViewController.origin.x,
                                    -rootViewPositionInRelationToRootViewController.origin.y
                                    + rootView.bounds.origin.y)).toRect();
    } else {
        actualGeometry = fromCGRect(self.frame).toRect();
    }

    // Persist the actual/new geometry so that QWindow::geometry() can
    // be queried on the resize event.
    m_qioswindow->QPlatformWindow::setGeometry(actualGeometry);

    QRect previousGeometry = requestedGeometry != actualGeometry ?
            requestedGeometry : qt_window_private(m_qioswindow->window())->geometry;

    QWindowSystemInterface::handleGeometryChange(m_qioswindow->window(), actualGeometry, previousGeometry);
    QWindowSystemInterface::flushWindowSystemEvents();

    if (actualGeometry.size() != previousGeometry.size()) {
        // Trigger expose event on resize
        [self setNeedsDisplay];

        // A new size means we also need to resize the FBO's corresponding buffers,
        // but we defer that to when the application calls makeCurrent.
    }
}

- (void)displayLayer:(CALayer *)layer
{
    Q_UNUSED(layer);
    Q_ASSERT(layer == self.layer);

    [self sendUpdatedExposeEvent];
}

- (void)sendUpdatedExposeEvent
{
    QRegion region;

    if (m_qioswindow->isExposed()) {
        QSize bounds = fromCGRect(self.layer.bounds).toRect().size();

        Q_ASSERT(m_qioswindow->geometry().size() == bounds);
        Q_ASSERT(self.hidden == !m_qioswindow->window()->isVisible());

        region = QRect(QPoint(), bounds);
    }

    QWindowSystemInterface::handleExposeEvent(m_qioswindow->window(), region);
    QWindowSystemInterface::flushWindowSystemEvents();
}

- (void)updateTouchList:(NSSet *)touches withState:(Qt::TouchPointState)state
{
    // We deliver touch events in global coordinates. But global in this respect
    // means the same coordinate system that we use for describing the geometry
    // of the top level QWindow we're inside. And that would be the coordinate
    // system of the superview of the UIView that backs that window:
    QPlatformWindow *topLevel = m_qioswindow;
    while (QPlatformWindow *topLevelParent = topLevel->parent())
        topLevel = topLevelParent;
    UIView *rootView = reinterpret_cast<UIView *>(topLevel->winId()).superview;
    CGSize rootViewSize = rootView.frame.size;

    foreach (UITouch *uiTouch, m_activeTouches.keys()) {
        QWindowSystemInterface::TouchPoint &touchPoint = m_activeTouches[uiTouch];
        if (![touches containsObject:uiTouch]) {
            touchPoint.state = Qt::TouchPointStationary;
        } else {
            touchPoint.state = state;
            touchPoint.pressure = (state == Qt::TouchPointReleased) ? 0.0 : 1.0;
            QPoint touchPos = fromCGPoint([uiTouch locationInView:rootView]).toPoint();
            touchPoint.area = QRectF(touchPos, QSize(0, 0));
            touchPoint.normalPosition = QPointF(touchPos.x() / rootViewSize.width, touchPos.y() / rootViewSize.height);
        }
    }
}

- (void) sendTouchEventWithTimestamp:(ulong)timeStamp
{
    // Send touch event synchronously
    QIOSIntegration *iosIntegration = static_cast<QIOSIntegration *>(QGuiApplicationPrivate::platformIntegration());
    QWindowSystemInterface::handleTouchEvent(m_qioswindow->window(), timeStamp, iosIntegration->touchDevice(), m_activeTouches.values());
    QWindowSystemInterface::flushWindowSystemEvents();
}

- (void)touchesBegan:(NSSet *)touches withEvent:(UIEvent *)event
{
    // UIKit generates [Began -> Moved -> Ended] event sequences for
    // each touch point. Internally we keep a hashmap of active UITouch
    // points to QWindowSystemInterface::TouchPoints, and assigns each TouchPoint
    // an id for use by Qt.
    for (UITouch *touch in touches) {
        Q_ASSERT(!m_activeTouches.contains(touch));
        m_activeTouches[touch].id = m_nextTouchId++;
    }

    if (m_activeTouches.size() == 1) {
        QPlatformWindow *topLevel = m_qioswindow;
        while (QPlatformWindow *p = topLevel->parent())
            topLevel = p;
        if (topLevel->window() != QGuiApplication::focusWindow())
            topLevel->requestActivateWindow();
    }

    [self updateTouchList:touches withState:Qt::TouchPointPressed];
    [self sendTouchEventWithTimestamp:ulong(event.timestamp * 1000)];
}

- (void)touchesMoved:(NSSet *)touches withEvent:(UIEvent *)event
{
    [self updateTouchList:touches withState:Qt::TouchPointMoved];
    [self sendTouchEventWithTimestamp:ulong(event.timestamp * 1000)];
}

- (void)touchesEnded:(NSSet *)touches withEvent:(UIEvent *)event
{
    [self updateTouchList:touches withState:Qt::TouchPointReleased];
    [self sendTouchEventWithTimestamp:ulong(event.timestamp * 1000)];

    // Remove ended touch points from the active set:
    for (UITouch *touch in touches)
        m_activeTouches.remove(touch);
    if (m_activeTouches.isEmpty())
        m_nextTouchId = 0;
}

- (void)touchesCancelled:(NSSet *)touches withEvent:(UIEvent *)event
{
    if (!touches && m_activeTouches.isEmpty())
        return;

    if (!touches) {
        m_activeTouches.clear();
    } else {
        for (UITouch *touch in touches)
            m_activeTouches.remove(touch);

        Q_ASSERT_X(m_activeTouches.isEmpty(), Q_FUNC_INFO,
            "Subset of active touches cancelled by UIKit");
    }

    m_nextTouchId = 0;

    NSTimeInterval timestamp = event ? event.timestamp : [[NSProcessInfo processInfo] systemUptime];

    // Send cancel touch event synchronously
    QIOSIntegration *iosIntegration = static_cast<QIOSIntegration *>(QGuiApplicationPrivate::platformIntegration());
    QWindowSystemInterface::handleTouchCancelEvent(m_qioswindow->window(), ulong(timestamp * 1000), iosIntegration->touchDevice());
    QWindowSystemInterface::flushWindowSystemEvents();
}

@end

@implementation UIView (QIOS)

- (QWindow *)qwindow
{
    if ([self isKindOfClass:[QUIView class]]) {
        if (QIOSWindow *w = static_cast<QUIView *>(self)->m_qioswindow)
            return w->window();
    }
    return nil;
}

- (UIViewController *)viewController
{
    id responder = self;
    while ((responder = [responder nextResponder])) {
        if ([responder isKindOfClass:UIViewController.class])
            return responder;
    }
    return nil;
}

@end

QT_BEGIN_NAMESPACE

QIOSWindow::QIOSWindow(QWindow *window)
    : QPlatformWindow(window)
    , m_view([[QUIView alloc] initWithQIOSWindow:this])
    , m_windowLevel(0)
{
    connect(qGuiApp, &QGuiApplication::applicationStateChanged, this, &QIOSWindow::applicationStateChanged);

    setParent(QPlatformWindow::parent());

    // Resolve default window geometry in case it was not set before creating the
    // platform window. This picks up eg. minimum-size if set, and defaults to
    // the "maxmized" geometry (even though we're not in that window state).
    // FIXME: Detect if we apply a maximized geometry and send a window state
    // change event in that case.
    m_normalGeometry = initialGeometry(window, QPlatformWindow::geometry(),
        screen()->availableGeometry().width(), screen()->availableGeometry().height());

    setWindowState(window->windowState());
}

QIOSWindow::~QIOSWindow()
{
    // According to the UIResponder documentation, Cocoa Touch should react to system interruptions
    // that "might cause the view to be removed from the window" by sending touchesCancelled, but in
    // practice this doesn't seem to happen when removing the view from its superview. To ensure that
    // Qt's internal state for touch and mouse handling is kept consistent, we therefor have to force
    // cancellation of all touch events.
    [m_view touchesCancelled:0 withEvent:0];

    m_view->m_qioswindow = 0;
    [m_view removeFromSuperview];
    [m_view release];
}

bool QIOSWindow::blockedByModal()
{
    QWindow *modalWindow = QGuiApplication::modalWindow();
    return modalWindow && modalWindow != window();
}

void QIOSWindow::setVisible(bool visible)
{
    m_view.hidden = !visible;
    [m_view setNeedsDisplay];

    if (!isQtApplication() || !window()->isTopLevel())
        return;

    // Since iOS doesn't do window management the way a Qt application
    // expects, we need to raise and activate windows ourselves:
    if (visible)
        updateWindowLevel();

    if (blockedByModal()) {
        if (visible)
            raise();
        return;
    }

    if (visible) {
        requestActivateWindow();
        static_cast<QIOSScreen *>(screen())->updateStatusBarVisibility();
    } else {
        // Activate top-most visible QWindow:
        NSArray *subviews = m_view.viewController.view.subviews;
        for (int i = int(subviews.count) - 1; i >= 0; --i) {
            UIView *view = [subviews objectAtIndex:i];
            if (!view.hidden) {
                QWindow *w = view.qwindow;
                if (w && w->isTopLevel()) {
                    static_cast<QIOSWindow *>(w->handle())->requestActivateWindow();
                    break;
                }
            }
        }
    }
}

void QIOSWindow::setGeometry(const QRect &rect)
{
    m_normalGeometry = rect;

    if (window()->windowState() != Qt::WindowNoState) {
        QPlatformWindow::setGeometry(rect);

        // The layout will realize the requested geometry was not applied, and
        // send geometry-change events that match the actual geometry.
        [m_view setNeedsLayout];

        if (window()->inherits("QWidgetWindow")) {
            // QWidget wrongly assumes that setGeometry resets the window
            // state back to Qt::NoWindowState, so we need to inform it that
            // that his is not the case by re-issuing the current window state.
            QWindowSystemInterface::handleWindowStateChanged(window(), window()->windowState());

            // It also needs to be told immediately that the geometry it requested
            // did not apply, otherwise it will continue on as if it did, instead
            // of waiting for a resize event.
            [m_view layoutIfNeeded];
        }

        return;
    }

    applyGeometry(rect);
}

void QIOSWindow::applyGeometry(const QRect &rect)
{
    // Geometry changes are asynchronous, but QWindow::geometry() is
    // expected to report back the 'requested geometry' until we get
    // a callback with the updated geometry from the window system.
    // The baseclass takes care of persisting this for us.
    QPlatformWindow::setGeometry(rect);

    if (window()->isTopLevel()) {
        // The QWindow is in QScreen coordinates, which maps to a possibly rotated root-view-controller.
        // Since the root-view-controller might be translated in relation to the UIWindow, we need to
        // check specifically for that and compensate. Also check if the root view has been scrolled
        // as a result of the keyboard being open.
        UIWindow *uiWindow = m_view.window;
        UIView *rootView = uiWindow.rootViewController.view;
        CGRect rootViewPositionInRelationToRootViewController =
            [rootView convertRect:uiWindow.bounds fromView:uiWindow];

        m_view.frame = CGRectOffset([m_view.superview convertRect:toCGRect(rect) fromView:rootView],
                rootViewPositionInRelationToRootViewController.origin.x,
                rootViewPositionInRelationToRootViewController.origin.y
                + rootView.bounds.origin.y);
    } else {
        // Easy, in parent's coordinates
        m_view.frame = toCGRect(rect);
    }

    // iOS will automatically trigger -[layoutSubviews:] for resize,
    // but not for move, so we force it just in case.
    [m_view setNeedsLayout];

    if (window()->inherits("QWidgetWindow"))
        [m_view layoutIfNeeded];
}

bool QIOSWindow::isExposed() const
{
    return qApp->applicationState() > Qt::ApplicationHidden
        && window()->isVisible() && !window()->geometry().isEmpty();
}

void QIOSWindow::setWindowState(Qt::WindowState state)
{
    // Update the QWindow representation straight away, so that
    // we can update the statusbar visibility based on the new
    // state before applying geometry changes.
    qt_window_private(window())->windowState = state;

    if (window()->isTopLevel() && window()->isVisible() && window()->isActive())
        static_cast<QIOSScreen *>(screen())->updateStatusBarVisibility();

    switch (state) {
    case Qt::WindowNoState:
        applyGeometry(m_normalGeometry);
        break;
    case Qt::WindowMaximized:
        applyGeometry(screen()->availableGeometry());
        break;
    case Qt::WindowFullScreen:
        applyGeometry(screen()->geometry());
        break;
    case Qt::WindowMinimized:
        applyGeometry(QRect());
        break;
    case Qt::WindowActive:
        Q_UNREACHABLE();
    default:
        Q_UNREACHABLE();
    }
}

void QIOSWindow::setParent(const QPlatformWindow *parentWindow)
{
    if (parentWindow) {
        UIView *parentView = reinterpret_cast<UIView *>(parentWindow->winId());
        [parentView addSubview:m_view];
    } else if (isQtApplication()) {
        for (UIWindow *uiWindow in [[UIApplication sharedApplication] windows]) {
            if (uiWindow.screen == static_cast<QIOSScreen *>(screen())->uiScreen()) {
                [uiWindow.rootViewController.view addSubview:m_view];
                break;
            }
        }
    }
}

void QIOSWindow::requestActivateWindow()
{
    // Note that several windows can be active at the same time if they exist in the same
    // hierarchy (transient children). But only one window can be QGuiApplication::focusWindow().
    // Dispite the name, 'requestActivateWindow' means raise and transfer focus to the window:
    if (blockedByModal())
        return;

    [m_view.window makeKeyWindow];

    if (window()->isTopLevel())
        raise();

    QWindowSystemInterface::handleWindowActivated(window());
}

void QIOSWindow::raiseOrLower(bool raise)
{
    // Re-insert m_view at the correct index among its sibling views
    // (QWindows) according to their current m_windowLevel:
    if (!isQtApplication())
        return;

    NSArray *subviews = m_view.superview.subviews;
    if (subviews.count == 1)
        return;

    for (int i = int(subviews.count) - 1; i >= 0; --i) {
        UIView *view = static_cast<UIView *>([subviews objectAtIndex:i]);
        if (view.hidden || view == m_view || !view.qwindow)
            continue;
        int level = static_cast<QIOSWindow *>(view.qwindow->handle())->m_windowLevel;
        if (m_windowLevel > level || (raise && m_windowLevel == level)) {
            [m_view.superview insertSubview:m_view aboveSubview:view];
            return;
        }
    }
    [m_view.superview insertSubview:m_view atIndex:0];
}

void QIOSWindow::updateWindowLevel()
{
    Qt::WindowType type = windowType();

    if (type == Qt::ToolTip)
        m_windowLevel = 120;
    else if (window()->flags() & Qt::WindowStaysOnTopHint)
        m_windowLevel = 100;
    else if (window()->isModal())
        m_windowLevel = 40;
    else if (type == Qt::Popup)
        m_windowLevel = 30;
    else if (type == Qt::SplashScreen)
        m_windowLevel = 20;
    else if (type == Qt::Tool)
        m_windowLevel = 10;
    else
        m_windowLevel = 0;

    // A window should be in at least the same m_windowLevel as its parent:
    QWindow *transientParent = window()->transientParent();
    QIOSWindow *transientParentWindow = transientParent ? static_cast<QIOSWindow *>(transientParent->handle()) : 0;
    if (transientParentWindow)
        m_windowLevel = qMax(transientParentWindow->m_windowLevel, m_windowLevel);
}

void QIOSWindow::handleContentOrientationChange(Qt::ScreenOrientation orientation)
{
    // Keep the status bar in sync with content orientation. This will ensure
    // that the task bar (and associated gestures) are aligned correctly:
    UIInterfaceOrientation uiOrientation = UIInterfaceOrientation(fromQtScreenOrientation(orientation));
    [[UIApplication sharedApplication] setStatusBarOrientation:uiOrientation animated:NO];
}

void QIOSWindow::applicationStateChanged(Qt::ApplicationState)
{
    if (window()->isExposed() != isExposed())
        [m_view sendUpdatedExposeEvent];
}

qreal QIOSWindow::devicePixelRatio() const
{
    return m_view.contentScaleFactor;
}

#include "moc_qioswindow.cpp"

QT_END_NAMESPACE
