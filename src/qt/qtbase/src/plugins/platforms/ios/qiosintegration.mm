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

#include "qiosintegration.h"
#include "qioseventdispatcher.h"
#include "qiosglobal.h"
#include "qioswindow.h"
#include "qiosbackingstore.h"
#include "qiosscreen.h"
#include "qioscontext.h"
#include "qiosclipboard.h"
#include "qiosinputcontext.h"
#include "qiostheme.h"
#include "qiosservices.h"

#include <qpa/qplatformoffscreensurface.h>

#include <QtPlatformSupport/private/qcoretextfontdatabase_p.h>
#include <QtPlatformSupport/private/qmacmime_p.h>
#include <QDir>

#include <QtDebug>

QT_BEGIN_NAMESPACE

QIOSIntegration::QIOSIntegration()
    : m_fontDatabase(new QCoreTextFontDatabase)
    , m_clipboard(new QIOSClipboard)
    , m_inputContext(new QIOSInputContext)
    , m_screen(new QIOSScreen(QIOSScreen::MainScreen))
    , m_platformServices(new QIOSServices)
{
    if (![UIApplication sharedApplication]) {
        qWarning()
            << "Error: You are creating QApplication before calling UIApplicationMain.\n"
            << "If you are writing a native iOS application, and only want to use Qt for\n"
            << "parts of the application, a good place to create QApplication is from within\n"
            << "'applicationDidFinishLaunching' inside your UIApplication delegate.\n";
        exit(-1);
    }

    // Set current directory to app bundle folder
    QDir::setCurrent(QString::fromUtf8([[[NSBundle mainBundle] bundlePath] UTF8String]));

    screenAdded(m_screen);

    m_touchDevice = new QTouchDevice;
    m_touchDevice->setType(QTouchDevice::TouchScreen);
    m_touchDevice->setCapabilities(QTouchDevice::Position | QTouchDevice::NormalizedPosition);
    QWindowSystemInterface::registerTouchDevice(m_touchDevice);
    QMacInternalPasteboardMime::initializeMimeTypes();
}

QIOSIntegration::~QIOSIntegration()
{
    delete m_fontDatabase;
    m_fontDatabase = 0;

    delete m_clipboard;
    m_clipboard = 0;
    QMacInternalPasteboardMime::destroyMimeTypes();

    delete m_inputContext;
    m_inputContext = 0;

    delete m_screen;
    m_screen = 0;

    delete m_platformServices;
    m_platformServices = 0;
}

bool QIOSIntegration::hasCapability(Capability cap) const
{
    switch (cap) {
    case BufferQueueingOpenGL:
        return true;
    case OpenGL:
    case ThreadedOpenGL:
        return true;
    case ThreadedPixmaps:
        return true;
    case MultipleWindows:
        return true;
    case WindowManagement:
        return false;
    case ApplicationState:
        return true;
    default:
        return QPlatformIntegration::hasCapability(cap);
    }
}

QPlatformWindow *QIOSIntegration::createPlatformWindow(QWindow *window) const
{
    return new QIOSWindow(window);
}

// Used when the QWindow's surface type is set by the client to QSurface::RasterSurface
QPlatformBackingStore *QIOSIntegration::createPlatformBackingStore(QWindow *window) const
{
    return new QIOSBackingStore(window);
}

// Used when the QWindow's surface type is set by the client to QSurface::OpenGLSurface
QPlatformOpenGLContext *QIOSIntegration::createPlatformOpenGLContext(QOpenGLContext *context) const
{
    return new QIOSContext(context);
}

QPlatformOffscreenSurface *QIOSIntegration::createPlatformOffscreenSurface(QOffscreenSurface *surface) const
{
    return new QPlatformOffscreenSurface(surface);
}

QAbstractEventDispatcher *QIOSIntegration::createEventDispatcher() const
{
    if (isQtApplication())
        return new QIOSEventDispatcher;
    else
        return new QEventDispatcherCoreFoundation;
}

QPlatformFontDatabase * QIOSIntegration::fontDatabase() const
{
    return m_fontDatabase;
}

QPlatformClipboard *QIOSIntegration::clipboard() const
{
    return m_clipboard;
}

QPlatformInputContext *QIOSIntegration::inputContext() const
{
    return m_inputContext;
}

QPlatformServices *QIOSIntegration::services() const
{
    return m_platformServices;
}

QVariant QIOSIntegration::styleHint(StyleHint hint) const
{
    switch (hint) {
    case ShowIsMaximized:
        return true;
    case SetFocusOnTouchRelease:
        return true;
    default:
        return QPlatformIntegration::styleHint(hint);
    }
}

QStringList QIOSIntegration::themeNames() const
{
    return QStringList(QLatin1String(QIOSTheme::name));
}

QPlatformTheme *QIOSIntegration::createPlatformTheme(const QString &name) const
{
    if (name == QLatin1String(QIOSTheme::name))
        return new QIOSTheme;

    return QPlatformIntegration::createPlatformTheme(name);
}

QPlatformNativeInterface *QIOSIntegration::nativeInterface() const
{
    return const_cast<QIOSIntegration *>(this);
}

void *QIOSIntegration::nativeResourceForWindow(const QByteArray &resource, QWindow *window)
{
    if (!window || !window->handle())
        return 0;

    QByteArray lowerCaseResource = resource.toLower();

    QIOSWindow *platformWindow = static_cast<QIOSWindow *>(window->handle());

    if (lowerCaseResource == "uiview")
        return reinterpret_cast<void *>(platformWindow->winId());

    return 0;
}

QTouchDevice *QIOSIntegration::touchDevice()
{
    return m_touchDevice;
}

QT_END_NAMESPACE
