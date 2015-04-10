/***************************************************************************
**
** Copyright (C) 2011 - 2013 BlackBerry Limited. All rights reserved.
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

#include "qqnxglobal.h"

#include "qqnxscreen.h"
#include "qqnxwindow.h"
#include "qqnxcursor.h"

#include <QtCore/QThread>
#include <QtCore/QDebug>
#include <qpa/qwindowsysteminterface.h>

#include <errno.h>

#if defined(QQNXSCREEN_DEBUG)
#define qScreenDebug qDebug
#else
#define qScreenDebug QT_NO_QDEBUG_MACRO
#endif

#if defined(QQNX_PHYSICAL_SCREEN_WIDTH) && QQNX_PHYSICAL_SCREEN_WIDTH > 0 \
    && defined(QQNX_PHYSICAL_SCREEN_HEIGHT) && QQNX_PHYSICAL_SCREEN_HEIGHT > 0
#define QQNX_PHYSICAL_SCREEN_SIZE_DEFINED
#elif defined(QQNX_PHYSICAL_SCREEN_WIDTH) || defined(QQNX_PHYSICAL_SCREEN_HEIGHT)
#error Please define QQNX_PHYSICAL_SCREEN_WIDTH and QQNX_PHYSICAL_SCREEN_HEIGHT to values greater than zero
#endif

// The default z-order of a window (intended to be overlain) created by
// mmrender.
static const int MMRENDER_DEFAULT_ZORDER = -1;

// The maximum z-order at which a foreign window will be considered
// an underlay.
static const int MAX_UNDERLAY_ZORDER = MMRENDER_DEFAULT_ZORDER - 1;

QT_BEGIN_NAMESPACE

static QSize determineScreenSize(screen_display_t display, bool primaryScreen) {
    int val[2];

    const int result = screen_get_display_property_iv(display, SCREEN_PROPERTY_PHYSICAL_SIZE, val);
    Q_SCREEN_CHECKERROR(result, "Failed to query display physical size");
    if (result != 0) {
        return QSize(150, 90);
    }

    if (val[0] > 0 && val[1] > 0)
        return QSize(val[0], val[1]);

    qScreenDebug("QQnxScreen: screen_get_display_property_iv() reported an invalid "
                 "physical screen size (%dx%d). Falling back to QQNX_PHYSICAL_SCREEN_SIZE "
                 "environment variable.", val[0], val[1]);

    const QString envPhySizeStr = qgetenv("QQNX_PHYSICAL_SCREEN_SIZE");
    if (!envPhySizeStr.isEmpty()) {
        const QStringList envPhySizeStrList = envPhySizeStr.split(QLatin1Char(','));
        const int envWidth = envPhySizeStrList.size() == 2 ? envPhySizeStrList[0].toInt() : -1;
        const int envHeight = envPhySizeStrList.size() == 2 ? envPhySizeStrList[1].toInt() : -1;

        if (envWidth <= 0 || envHeight <= 0) {
            qFatal("QQnxScreen: The value of QQNX_PHYSICAL_SCREEN_SIZE must be in the format "
                   "\"width,height\" in mm, with width, height > 0. "
                   "Example: QQNX_PHYSICAL_SCREEN_SIZE=150,90");
            return QSize(150, 90);
        }

        return QSize(envWidth, envHeight);
    }

#if defined(QQNX_PHYSICAL_SCREEN_SIZE_DEFINED)
    const QSize defSize(QQNX_PHYSICAL_SCREEN_WIDTH, QQNX_PHYSICAL_SCREEN_HEIGHT);
    qWarning("QQnxScreen: QQNX_PHYSICAL_SCREEN_SIZE variable not set. Falling back to defines "
             "QQNX_PHYSICAL_SCREEN_WIDTH/QQNX_PHYSICAL_SCREEN_HEIGHT (%dx%d)",
             defSize.width(), defSize.height());
    return defSize;
#else
    if (primaryScreen)
        qFatal("QQnxScreen: QQNX_PHYSICAL_SCREEN_SIZE variable not set. "
               "Could not determine physical screen size.");
    return QSize(150, 90);
#endif
}

static QQnxWindow *findMultimediaWindow(const QList<QQnxWindow*> windows,
                                                    const QByteArray &mmWindowId)
{
    Q_FOREACH (QQnxWindow *sibling, windows) {
        if (sibling->mmRendererWindowName() == mmWindowId)
            return sibling;

        QQnxWindow *mmWindow = findMultimediaWindow(sibling->children(), mmWindowId);

        if (mmWindow)
            return mmWindow;
    }

    return 0;
}

static QQnxWindow *findMultimediaWindow(const QList<QQnxWindow*> windows,
                                                    screen_window_t mmWindowId)
{
    Q_FOREACH (QQnxWindow *sibling, windows) {
        if (sibling->mmRendererWindow() == mmWindowId)
            return sibling;

        QQnxWindow *mmWindow = findMultimediaWindow(sibling->children(), mmWindowId);

        if (mmWindow)
            return mmWindow;
    }

    return 0;
}

QQnxScreen::QQnxScreen(screen_context_t screenContext, screen_display_t display, bool primaryScreen)
    : m_screenContext(screenContext),
      m_display(display),
      m_rootWindow(0),
      m_primaryScreen(primaryScreen),
      m_keyboardHeight(0),
      m_nativeOrientation(Qt::PrimaryOrientation),
      m_coverWindow(0),
      m_cursor(new QQnxCursor())
{
    qScreenDebug() << Q_FUNC_INFO;
    // Cache initial orientation of this display
    int result = screen_get_display_property_iv(m_display, SCREEN_PROPERTY_ROTATION,
                                                &m_initialRotation);
    Q_SCREEN_CHECKERROR(result, "Failed to query display rotation");

    m_currentRotation = m_initialRotation;

    // Cache size of this display in pixels
    int val[2];
    Q_SCREEN_CRITICALERROR(screen_get_display_property_iv(m_display, SCREEN_PROPERTY_SIZE, val),
                        "Failed to query display size");

    m_currentGeometry = m_initialGeometry = QRect(0, 0, val[0], val[1]);

    char name[100];
    Q_SCREEN_CHECKERROR(screen_get_display_property_cv(m_display, SCREEN_PROPERTY_ID_STRING, 100,
                                                       name), "Failed to query display name");
    m_name = QString::fromUtf8(name);

    // Cache size of this display in millimeters. We have to take care of the orientation.
    // libscreen always reports the physical size dimensions as width and height in the
    // native orientation. Contrary to this, QPlatformScreen::physicalSize() expects the
    // returned dimensions to follow the current orientation.
    const QSize screenSize = determineScreenSize(m_display, primaryScreen);

    m_nativeOrientation = screenSize.width() >= screenSize.height() ? Qt::LandscapeOrientation : Qt::PortraitOrientation;

    const int angle = screen()->angleBetween(m_nativeOrientation, orientation());
    if (angle == 0 || angle == 180)
        m_currentPhysicalSize = m_initialPhysicalSize = screenSize;
    else
        m_currentPhysicalSize = m_initialPhysicalSize = screenSize.transposed();
}

QQnxScreen::~QQnxScreen()
{
    qScreenDebug() << Q_FUNC_INFO;
    Q_FOREACH (QQnxWindow *childWindow, m_childWindows)
        childWindow->setScreen(0);

    if (m_coverWindow)
        m_coverWindow->setScreen(0);

    delete m_cursor;
}

static int defaultDepth()
{
    qScreenDebug() << Q_FUNC_INFO;
    static int defaultDepth = 0;
    if (defaultDepth == 0) {
        // check if display depth was specified in environment variable;
        // use default value if no valid value found
        defaultDepth = qgetenv("QQNX_DISPLAY_DEPTH").toInt();
        if (defaultDepth != 16 && defaultDepth != 32)
            defaultDepth = 32;
    }
    return defaultDepth;
}

QRect QQnxScreen::availableGeometry() const
{
    qScreenDebug() << Q_FUNC_INFO;
    // available geometry = total geometry - keyboard
    return QRect(m_currentGeometry.x(), m_currentGeometry.y(),
                 m_currentGeometry.width(), m_currentGeometry.height() - m_keyboardHeight);
}

int QQnxScreen::depth() const
{
    return defaultDepth();
}

qreal QQnxScreen::refreshRate() const
{
    screen_display_mode_t displayMode;
    int result = screen_get_display_property_pv(m_display, SCREEN_PROPERTY_MODE, reinterpret_cast<void **>(&displayMode));
    if (result != 0) {
        qWarning("QQnxScreen: Failed to query screen mode. Using default value of 60Hz");
        return 60.0;
    }
    qScreenDebug() << Q_FUNC_INFO << "screen mode:" << endl
                   << "      width =" << displayMode.width << endl
                   << "     height =" << displayMode.height << endl
                   << "    refresh =" << displayMode.refresh << endl
                   << " interlaced =" << displayMode.interlaced;
    return static_cast<qreal>(displayMode.refresh);
}

Qt::ScreenOrientation QQnxScreen::nativeOrientation() const
{
    return m_nativeOrientation;
}

Qt::ScreenOrientation QQnxScreen::orientation() const
{
    Qt::ScreenOrientation orient;
    if (m_nativeOrientation == Qt::LandscapeOrientation) {
        // Landscape devices e.g. PlayBook
        if (m_currentRotation == 0)
            orient = Qt::LandscapeOrientation;
        else if (m_currentRotation == 90)
            orient = Qt::PortraitOrientation;
        else if (m_currentRotation == 180)
            orient = Qt::InvertedLandscapeOrientation;
        else
            orient = Qt::InvertedPortraitOrientation;
    } else {
        // Portrait devices e.g. Phones
        // ###TODO Check these on an actual phone device
        if (m_currentRotation == 0)
            orient = Qt::PortraitOrientation;
        else if (m_currentRotation == 90)
            orient = Qt::LandscapeOrientation;
        else if (m_currentRotation == 180)
            orient = Qt::InvertedPortraitOrientation;
        else
            orient = Qt::InvertedLandscapeOrientation;
    }
    qScreenDebug() << Q_FUNC_INFO << "orientation =" << orient;
    return orient;
}

/*!
    Check if the supplied angles are perpendicular to each other.
*/
static bool isOrthogonal(int angle1, int angle2)
{
    return ((angle1 - angle2) % 180) != 0;
}

void QQnxScreen::setRotation(int rotation)
{
    qScreenDebug() << Q_FUNC_INFO << "orientation =" << rotation;
    // Check if rotation changed
    // We only want to rotate if we are the primary screen
    if (m_currentRotation != rotation && isPrimaryScreen()) {
        // Update rotation of root window
        if (rootWindow())
            rootWindow()->setRotation(rotation);

        const QRect previousScreenGeometry = geometry();

        // Swap dimensions if we've rotated 90 or 270 from initial orientation
        if (isOrthogonal(m_initialRotation, rotation)) {
            m_currentGeometry = QRect(0, 0, m_initialGeometry.height(), m_initialGeometry.width());
            m_currentPhysicalSize = QSize(m_initialPhysicalSize.height(), m_initialPhysicalSize.width());
        } else {
            m_currentGeometry = QRect(0, 0, m_initialGeometry.width(), m_initialGeometry.height());
            m_currentPhysicalSize = m_initialPhysicalSize;
        }

        // Resize root window if we've rotated 90 or 270 from previous orientation
        if (isOrthogonal(m_currentRotation, rotation)) {
            qScreenDebug() << Q_FUNC_INFO << "resize, size =" << m_currentGeometry.size();
            if (rootWindow())
                rootWindow()->setGeometry(QRect(QPoint(0,0), m_currentGeometry.size()));

            resizeWindows(previousScreenGeometry);
        } else {
            // TODO: Find one global place to flush display updates
            // Force immediate display update if no geometry changes required
            screen_flush_context(nativeContext(), 0);
        }

        // Save new rotation
        m_currentRotation = rotation;

        // TODO: check if other screens are supposed to rotate as well and/or whether this depends
        // on if clone mode is being used.
        // Rotating only the primary screen is what we had in the navigator event handler before refactoring
        if (m_primaryScreen) {
            QWindowSystemInterface::handleScreenOrientationChange(screen(), orientation());
            QWindowSystemInterface::handleScreenGeometryChange(screen(), m_currentGeometry);
        }

        // Flush everything, so that the windows rotations are applied properly.
        // Needed for non-maximized windows
        screen_flush_context( m_screenContext, 0 );
    }
}

/*!
  Resize the given window proportionally to the screen geometry
*/
void QQnxScreen::resizeNativeWidgetWindow(QQnxWindow *w, const QRect &previousScreenGeometry) const
{
    const qreal relativeX = static_cast<qreal>(w->geometry().topLeft().x()) / previousScreenGeometry.width();
    const qreal relativeY = static_cast<qreal>(w->geometry().topLeft().y()) / previousScreenGeometry.height();
    const qreal relativeWidth = static_cast<qreal>(w->geometry().width()) / previousScreenGeometry.width();
    const qreal relativeHeight = static_cast<qreal>(w->geometry().height()) / previousScreenGeometry.height();

    const QRect windowGeometry(relativeX * geometry().width(), relativeY * geometry().height(),
            relativeWidth * geometry().width(), relativeHeight * geometry().height());

    w->setGeometry(windowGeometry);
}

/*!
  Resize the given window to fit the screen geometry
*/
void QQnxScreen::resizeTopLevelWindow(QQnxWindow *w, const QRect &previousScreenGeometry) const
{
    QRect windowGeometry = w->geometry();

    const qreal relativeCenterX = static_cast<qreal>(w->geometry().center().x()) / previousScreenGeometry.width();
    const qreal relativeCenterY = static_cast<qreal>(w->geometry().center().y()) / previousScreenGeometry.height();
    const QPoint newCenter(relativeCenterX * geometry().width(), relativeCenterY * geometry().height());

    windowGeometry.moveCenter(newCenter);

    // adjust center position in case the window
    // is clipped
    if (!geometry().contains(windowGeometry)) {
        const int x1 = windowGeometry.x();
        const int y1 = windowGeometry.y();
        const int x2 = x1 + windowGeometry.width();
        const int y2 = y1 + windowGeometry.height();

        if (x1 < 0) {
            const int centerX = qMin(qAbs(x1) + windowGeometry.center().x(),
                                        geometry().center().x());

            windowGeometry.moveCenter(QPoint(centerX, windowGeometry.center().y()));
        }

        if (y1 < 0) {
            const int centerY = qMin(qAbs(y1) + windowGeometry.center().y(),
                                        geometry().center().y());

            windowGeometry.moveCenter(QPoint(windowGeometry.center().x(), centerY));
        }

        if (x2 > geometry().width()) {
            const int centerX = qMax(windowGeometry.center().x() - (x2 - geometry().width()),
                                        geometry().center().x());

            windowGeometry.moveCenter(QPoint(centerX, windowGeometry.center().y()));
        }

        if (y2 > geometry().height()) {
            const int centerY = qMax(windowGeometry.center().y() - (y2 - geometry().height()),
                                        geometry().center().y());

            windowGeometry.moveCenter(QPoint(windowGeometry.center().x(), centerY));
        }
    }

    // at this point, if the window is still clipped,
    // it means that it's too big to fit on the screen,
    // so we need to proportionally shrink it
    if (!geometry().contains(windowGeometry)) {
        QSize newSize = windowGeometry.size();
        newSize.scale(geometry().size(), Qt::KeepAspectRatio);
        windowGeometry.setSize(newSize);

        if (windowGeometry.x() < 0)
            windowGeometry.moveCenter(QPoint(geometry().center().x(), windowGeometry.center().y()));

        if (windowGeometry.y() < 0)
            windowGeometry.moveCenter(QPoint(windowGeometry.center().x(), geometry().center().y()));
    }

    w->setGeometry(windowGeometry);
}

/*!
  Adjust windows to the new screen geometry.
*/
void QQnxScreen::resizeWindows(const QRect &previousScreenGeometry)
{
    resizeMaximizedWindows();

    Q_FOREACH (QQnxWindow *w, m_childWindows) {

        if (w->window()->windowState() & Qt::WindowFullScreen || w->window()->windowState() & Qt::WindowMaximized)
            continue;

        if (w->parent()) {
            // This is a native (non-alien) widget window
            resizeNativeWidgetWindow(w, previousScreenGeometry);
        } else {
            // This is a toplevel window
            resizeTopLevelWindow(w, previousScreenGeometry);
        }
    }
}

QQnxWindow *QQnxScreen::findWindow(screen_window_t windowHandle)
{
    Q_FOREACH (QQnxWindow *window, m_childWindows) {
        QQnxWindow * const result = window->findWindow(windowHandle);
        if (result)
            return result;
    }

    return 0;
}

void QQnxScreen::addWindow(QQnxWindow *window)
{
    qScreenDebug() << Q_FUNC_INFO << "window =" << window;

    if (m_childWindows.contains(window))
        return;

    if (window->window()->type() != Qt::CoverWindow) {
        // Ensure that the desktop window is at the bottom of the zorder.
        // If we do not do this then we may end up activating the desktop
        // when the navigator service gets an event that our window group
        // has been activated (see QQnxScreen::activateWindowGroup()).
        // Such a situation would strangely break focus handling due to the
        // invisible desktop widget window being layered on top of normal
        // windows
        if (window->window()->type() == Qt::Desktop)
            m_childWindows.push_front(window);
        else
            m_childWindows.push_back(window);
        updateHierarchy();
    } else {
#if defined(Q_OS_BLACKBERRY) && !defined(Q_OS_BLACKBERRY_TABLET)
        m_coverWindow = window;
#endif
    }
}

void QQnxScreen::removeWindow(QQnxWindow *window)
{
    qScreenDebug() << Q_FUNC_INFO << "window =" << window;

    if (window != m_coverWindow) {
        const int numWindowsRemoved = m_childWindows.removeAll(window);
        if (window == m_rootWindow) //We just removed the root window
            m_rootWindow = 0; //TODO we need a new root window ;)
        if (numWindowsRemoved > 0)
            updateHierarchy();
    } else {
        m_coverWindow = 0;
    }
}

void QQnxScreen::raiseWindow(QQnxWindow *window)
{
    qScreenDebug() << Q_FUNC_INFO << "window =" << window;

    if (window != m_coverWindow) {
        removeWindow(window);
        m_childWindows.push_back(window);
    }
}

void QQnxScreen::lowerWindow(QQnxWindow *window)
{
    qScreenDebug() << Q_FUNC_INFO << "window =" << window;

    if (window != m_coverWindow) {
        removeWindow(window);
        m_childWindows.push_front(window);
    }
}

void QQnxScreen::updateHierarchy()
{
    qScreenDebug() << Q_FUNC_INFO;

    QList<QQnxWindow*>::const_iterator it;
    int result;
    int topZorder = 0;

    errno = 0;
    if (rootWindow()) {
        result = screen_get_window_property_iv(rootWindow()->nativeHandle(), SCREEN_PROPERTY_ZORDER, &topZorder);
        if (result != 0) { //This can happen if we use winId in QWidgets
            topZorder = 10;
            qWarning("QQnxScreen: failed to query root window z-order, errno=%d", errno);
        }
    } else {
        topZorder = 0;  //We do not need z ordering on the secondary screen, because only one window
                        //is supported there
    }

    topZorder++; // root window has the lowest z-order in the windowgroup

    int underlayZorder = -1;
    // Underlays sit immediately above the root window in the z-ordering
    Q_FOREACH (screen_window_t underlay, m_underlays) {
        // Do nothing when this fails. This can happen if we have stale windows in m_underlays,
        // which in turn can happen because a window was removed but we didn't get a notification
        // yet.
        screen_set_window_property_iv(underlay, SCREEN_PROPERTY_ZORDER, &underlayZorder);
        underlayZorder--;
    }

    // Normal Qt windows come next above the root window z-ordering
    for (it = m_childWindows.constBegin(); it != m_childWindows.constEnd(); ++it)
        (*it)->updateZorder(topZorder);

    // Finally overlays sit above all else in the z-ordering
    Q_FOREACH (screen_window_t overlay, m_overlays) {
        // No error handling, see underlay logic above
        screen_set_window_property_iv(overlay, SCREEN_PROPERTY_ZORDER, &topZorder);
        topZorder++;
    }

    // After a hierarchy update, we need to force a flush on all screens.
    // Right now, all screens share a context.
    screen_flush_context(m_screenContext, 0);
}

void QQnxScreen::adjustOrientation()
{
    if (!m_primaryScreen)
        return;

    bool ok = false;
    const int rotation = qgetenv("ORIENTATION").toInt(&ok);

    if (ok)
        setRotation(rotation);
}

QPlatformCursor * QQnxScreen::cursor() const
{
    return m_cursor;
}

void QQnxScreen::keyboardHeightChanged(int height)
{
    if (height == m_keyboardHeight)
        return;

    m_keyboardHeight = height;

    QWindowSystemInterface::handleScreenAvailableGeometryChange(screen(), availableGeometry());
}

void QQnxScreen::addOverlayWindow(screen_window_t window)
{
    m_overlays.append(window);
    updateHierarchy();
}

void QQnxScreen::addUnderlayWindow(screen_window_t window)
{
    m_underlays.append(window);
    updateHierarchy();
}

void QQnxScreen::addMultimediaWindow(const QByteArray &id, screen_window_t window)
{
    // find the QnxWindow this mmrenderer window is related to
    QQnxWindow *mmWindow = findMultimediaWindow(m_childWindows, id);

    if (!mmWindow)
        return;

    mmWindow->setMMRendererWindow(window);

    updateHierarchy();
}

void QQnxScreen::removeOverlayOrUnderlayWindow(screen_window_t window)
{
    const int numRemoved = m_overlays.removeAll(window) + m_underlays.removeAll(window);
    if (numRemoved > 0) {
        updateHierarchy();
        Q_EMIT foreignWindowClosed(window);
    }
}

void QQnxScreen::newWindowCreated(void *window)
{
    Q_ASSERT(thread() == QThread::currentThread());
    const screen_window_t windowHandle = reinterpret_cast<screen_window_t>(window);
    screen_display_t display = 0;
    if (screen_get_window_property_pv(windowHandle, SCREEN_PROPERTY_DISPLAY, (void**)&display) != 0) {
        qWarning("QQnx: Failed to get screen for window, errno=%d", errno);
        return;
    }

    int zorder;
    if (screen_get_window_property_iv(windowHandle, SCREEN_PROPERTY_ZORDER, &zorder) != 0) {
        qWarning("QQnx: Failed to get z-order for window, errno=%d", errno);
        zorder = 0;
    }

    char windowNameBuffer[256] = { 0 };
    QByteArray windowName;

    if (screen_get_window_property_cv(windowHandle, SCREEN_PROPERTY_ID_STRING,
                sizeof(windowNameBuffer) - 1, windowNameBuffer) != 0) {
        qWarning("QQnx: Failed to get id for window, errno=%d", errno);
    }

    windowName = QByteArray(windowNameBuffer);

    if (display == nativeDisplay()) {
        // A window was created on this screen. If we don't know about this window yet, it means
        // it was not created by Qt, but by some foreign library like the multimedia renderer, which
        // creates an overlay window when playing a video.
        //
        // Treat all foreign windows as overlays, underlays or as windows
        // created by the BlackBerry QtMultimedia plugin.
        //
        // In the case of the BlackBerry QtMultimedia plugin, we need to
        // "attach" the foreign created mmrenderer window to the correct
        // platform window (usually the one belonging to QVideoWidget) to
        // ensure proper z-ordering.
        //
        // Otherwise, assume that if a foreign window already has a Z-Order both negative and
        // less than the default Z-Order installed by mmrender on windows it creates,
        // the windows should be treated as an underlay. Otherwise, we treat it as an overlay.
        if (!windowName.isEmpty() && windowName.startsWith("MmRendererVideoWindowControl")) {
            addMultimediaWindow(windowName, windowHandle);
        } else if (!findWindow(windowHandle)) {
            if (zorder <= MAX_UNDERLAY_ZORDER)
                addUnderlayWindow(windowHandle);
            else
                addOverlayWindow(windowHandle);
            Q_EMIT foreignWindowCreated(windowHandle);
        }
    }
}

void QQnxScreen::windowClosed(void *window)
{
    Q_ASSERT(thread() == QThread::currentThread());
    const screen_window_t windowHandle = reinterpret_cast<screen_window_t>(window);

    QQnxWindow *mmWindow = findMultimediaWindow(m_childWindows, windowHandle);

    if (mmWindow)
        mmWindow->clearMMRendererWindow();
    else
        removeOverlayOrUnderlayWindow(windowHandle);
}

void QQnxScreen::windowGroupStateChanged(const QByteArray &id, Qt::WindowState state)
{
    qScreenDebug() << Q_FUNC_INFO;

    if (!rootWindow() || id != rootWindow()->groupName())
        return;

    QWindow * const window = rootWindow()->window();

    if (!window)
        return;

    QWindowSystemInterface::handleWindowStateChanged(window, state);
}

void QQnxScreen::activateWindowGroup(const QByteArray &id)
{
    qScreenDebug() << Q_FUNC_INFO;

    if (!rootWindow() || id != rootWindow()->groupName())
        return;

    QWindow * const window = rootWindow()->window();

    if (!window)
        return;

    Q_FOREACH (QQnxWindow *childWindow, m_childWindows)
        childWindow->setExposed(true);

    if (m_coverWindow)
        m_coverWindow->setExposed(false);
}

void QQnxScreen::deactivateWindowGroup(const QByteArray &id)
{
    qScreenDebug() << Q_FUNC_INFO;

    if (!rootWindow() || id != rootWindow()->groupName())
        return;

    if (m_coverWindow)
        m_coverWindow->setExposed(true);

    Q_FOREACH (QQnxWindow *childWindow, m_childWindows)
        childWindow->setExposed(false);
}

QQnxWindow *QQnxScreen::rootWindow() const
{
    return m_rootWindow;
}

void QQnxScreen::setRootWindow(QQnxWindow *window)
{
    // Optionally disable the screen power save
    bool ok = false;
    const int disablePowerSave = qgetenv("QQNX_DISABLE_POWER_SAVE").toInt(&ok);
    if (ok && disablePowerSave) {
        const int mode = SCREEN_IDLE_MODE_KEEP_AWAKE;
        int result = screen_set_window_property_iv(window->nativeHandle(), SCREEN_PROPERTY_IDLE_MODE, &mode);
        if (result != 0)
            qWarning("QQnxRootWindow: failed to disable power saving mode");
    }
    m_rootWindow = window;
}

QWindow * QQnxScreen::topMostChildWindow() const
{
    if (!m_childWindows.isEmpty()) {

        // We're picking up the last window of the list here
        // because this list is ordered by stacking order.
        // Last window is effectively the one on top.
        return m_childWindows.last()->window();
    }

    return 0;
}

QT_END_NAMESPACE
