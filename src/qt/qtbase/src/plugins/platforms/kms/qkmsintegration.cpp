/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia. For licensing terms and
** conditions see http://qt.digia.com/licensing. For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights. These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qkmsintegration.h"
#include "qkmsdevice.h"
#include "qkmsscreen.h"
#include "qkmswindow.h"
#include "qkmsbackingstore.h"
#include "qkmscontext.h"
#include "qkmsnativeinterface.h"

#if !defined(QT_NO_EVDEV)
#include <QtPlatformSupport/private/qevdevmousemanager_p.h>
#include <QtPlatformSupport/private/qevdevkeyboardmanager_p.h>
#include <QtPlatformSupport/private/qevdevtouch_p.h>
#endif

#include <QtPlatformSupport/private/qgenericunixeventdispatcher_p.h>
#include <QtPlatformSupport/private/qgenericunixfontdatabase_p.h>
#include <QtPlatformSupport/private/qfbvthandler_p.h>
#include <QtPlatformSupport/private/qeglconvenience_p.h>

#include <QtGui/private/qguiapplication_p.h>
#include <QtGui/QOpenGLContext>
#include <QtGui/QScreen>
#include <QtGui/QOffscreenSurface>
#include <qpa/qplatformoffscreensurface.h>

QT_BEGIN_NAMESPACE

QKmsIntegration::QKmsIntegration()
    : QPlatformIntegration(),
      m_fontDatabase(new QGenericUnixFontDatabase()),
      m_nativeInterface(new QKmsNativeInterface),
      m_vtHandler(0),
      m_deviceDiscovery(0)
{
}

QKmsIntegration::~QKmsIntegration()
{
    delete m_deviceDiscovery;
    foreach (QKmsDevice *device, m_devices) {
        delete device;
    }
    foreach (QPlatformScreen *screen, m_screens) {
        delete screen;
    }
    delete m_fontDatabase;
    delete m_vtHandler;
}

void QKmsIntegration::initialize()
{
    qputenv("EGL_PLATFORM", "drm");
    m_vtHandler = new QFbVtHandler;

    m_deviceDiscovery = QDeviceDiscovery::create(QDeviceDiscovery::Device_DRM | QDeviceDiscovery::Device_DRM_PrimaryGPU, 0);
    if (m_deviceDiscovery) {
        QStringList devices = m_deviceDiscovery->scanConnectedDevices();
        foreach (QString device, devices)
            addDevice(device);

        connect(m_deviceDiscovery, SIGNAL(deviceDetected(QString)), this, SLOT(addDevice(QString)));
        connect(m_deviceDiscovery, SIGNAL(deviceRemoved(QString)), this, SLOT(removeDevice(QString)));
    }

#if !defined(QT_NO_EVDEV)
    new QEvdevKeyboardManager(QLatin1String("EvdevKeyboard"), QString() /* spec */, this);
    new QEvdevMouseManager(QLatin1String("EvdevMouse"), QString() /* spec */, this);
    new QEvdevTouchScreenHandlerThread(QString() /* spec */, this);
#endif
}

void QKmsIntegration::addDevice(const QString &deviceNode)
{
    m_devices.append(new QKmsDevice(deviceNode, this));
}

void QKmsIntegration::removeDevice(const QString &deviceNode)
{
    // TODO: support hot-plugging some day?
    Q_UNUSED(deviceNode);
}

bool QKmsIntegration::hasCapability(QPlatformIntegration::Capability cap) const
{
    switch (cap) {
    case ThreadedPixmaps: return true;
    case OpenGL: return true;
    case ThreadedOpenGL: return false;
    case RasterGLSurface: return true;
    default: return QPlatformIntegration::hasCapability(cap);
    }
}

QPlatformOpenGLContext *QKmsIntegration::createPlatformOpenGLContext(QOpenGLContext *context) const
{
    QKmsScreen *screen = static_cast<QKmsScreen *>(context->screen()->handle());
    return new QKmsContext(context, screen->device());
}

QPlatformWindow *QKmsIntegration::createPlatformWindow(QWindow *window) const
{
    QKmsWindow *w = new QKmsWindow(window);
    w->requestActivateWindow();
    return w;
}

QPlatformBackingStore *QKmsIntegration::createPlatformBackingStore(QWindow *window) const
{
    return new QKmsBackingStore(window);
}

// Neither a pbuffer nor a hidden QWindow is suitable. Just use an additional, small gbm surface.
QKmsOffscreenWindow::QKmsOffscreenWindow(EGLDisplay display, const QSurfaceFormat &format, QOffscreenSurface *offscreenSurface)
    : QPlatformOffscreenSurface(offscreenSurface)
    , m_format(format)
    , m_display(display)
    , m_surface(EGL_NO_SURFACE)
    , m_window(0)
{
    QKmsScreen *screen = static_cast<QKmsScreen *>(offscreenSurface->screen()->handle());
    m_window = gbm_surface_create(screen->device()->gbmDevice(),
                                  10, 10,
                                  GBM_FORMAT_XRGB8888,
                                  GBM_BO_USE_RENDERING);
    if (!m_window) {
        qWarning("QKmsOffscreenWindow: Failed to create native window");
        return;
    }

    EGLConfig config = q_configFromGLFormat(m_display, m_format);
    m_surface = eglCreateWindowSurface(m_display, config, m_window, 0);
    if (m_surface != EGL_NO_SURFACE)
        m_format = q_glFormatFromConfig(m_display, config);
}

QKmsOffscreenWindow::~QKmsOffscreenWindow()
{
    if (m_surface != EGL_NO_SURFACE)
        eglDestroySurface(m_display, m_surface);
    if (m_window)
        gbm_surface_destroy((gbm_surface *) m_window);
}

QPlatformOffscreenSurface *QKmsIntegration::createPlatformOffscreenSurface(QOffscreenSurface *surface) const
{
    QKmsScreen *screen = static_cast<QKmsScreen *>(surface->screen()->handle());
    return new QKmsOffscreenWindow(screen->device()->eglDisplay(), QKmsScreen::tweakFormat(surface->format()), surface);
}

QPlatformFontDatabase *QKmsIntegration::fontDatabase() const
{
    return m_fontDatabase;
}

void QKmsIntegration::addScreen(QKmsScreen *screen)
{
    m_screens.append(screen);
    screenAdded(screen);
}

QAbstractEventDispatcher *QKmsIntegration::createEventDispatcher() const
{
    return createUnixEventDispatcher();
}

QPlatformNativeInterface *QKmsIntegration::nativeInterface() const
{
    return m_nativeInterface;
}

QT_END_NAMESPACE
