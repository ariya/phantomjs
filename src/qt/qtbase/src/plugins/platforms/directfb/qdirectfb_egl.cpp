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

#include "qdirectfb_egl.h"
#include "qdirectfbwindow.h"
#include "qdirectfbscreen.h"
#include "qdirectfbeglhooks.h"

#include <QtGui/QOpenGLContext>
#include <qpa/qplatformopenglcontext.h>
#include <QtGui/QScreen>

#include <QtPlatformSupport/private/qeglplatformcontext_p.h>
#include <QtPlatformSupport/private/qeglconvenience_p.h>

#include <EGL/egl.h>

QT_BEGIN_NAMESPACE

#ifdef DIRECTFB_PLATFORM_HOOKS
extern QDirectFBEGLHooks platform_hook;
static QDirectFBEGLHooks *hooks = &platform_hook;
#else
static QDirectFBEGLHooks *hooks = 0;
#endif

/**
 * This provides OpenGL ES 2.0 integration with DirectFB. It assumes that
 * one can adapt a DirectFBSurface as a EGLSurface. It might need some vendor
 * init code in the QDirectFbScreenEGL class to initialize EGL and one might
 * need to provide a custom QDirectFbWindowEGL::format() to return the
 * QSurfaceFormat used by the device.
 */

class QDirectFbScreenEGL : public QDirectFbScreen {
public:
    QDirectFbScreenEGL(int display);
    ~QDirectFbScreenEGL();

    // EGL helper
    EGLDisplay eglDisplay();

private:
    void initializeEGL();
    void platformInit();
    void platformDestroy();

private:
    EGLDisplay m_eglDisplay;
};

class QDirectFbWindowEGL : public QDirectFbWindow {
public:
    QDirectFbWindowEGL(QWindow *tlw, QDirectFbInput *inputhandler);
    ~QDirectFbWindowEGL();

    void createDirectFBWindow();

    // EGL. Subclass it instead to have different GL integrations?
    EGLSurface eglSurface();

    QSurfaceFormat format() const;

private:
    EGLSurface m_eglSurface;
};

class QDirectFbEGLContext : public QEGLPlatformContext {
public:
    QDirectFbEGLContext(QDirectFbScreenEGL *screen, QOpenGLContext *context);

protected:
    EGLSurface eglSurfaceForPlatformSurface(QPlatformSurface *surface);

private:
    QDirectFbScreen *m_screen;
};

QDirectFbScreenEGL::QDirectFbScreenEGL(int display)
    : QDirectFbScreen(display)
    , m_eglDisplay(EGL_NO_DISPLAY)
{}

QDirectFbScreenEGL::~QDirectFbScreenEGL()
{
    platformDestroy();
}

EGLDisplay QDirectFbScreenEGL::eglDisplay()
{
    if (m_eglDisplay == EGL_NO_DISPLAY)
        initializeEGL();
    return m_eglDisplay;
}

void QDirectFbScreenEGL::initializeEGL()
{
    m_eglDisplay = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (m_eglDisplay == EGL_NO_DISPLAY)
        return;

    platformInit();

    EGLint major, minor;
    eglBindAPI(EGL_OPENGL_ES_API);
    eglInitialize(m_eglDisplay, &major, &minor);
    return;
}

void QDirectFbScreenEGL::platformInit()
{
    if (hooks)
        hooks->platformInit();
}

void QDirectFbScreenEGL::platformDestroy()
{
    if (hooks)
        hooks->platformDestroy();
}

QDirectFbWindowEGL::QDirectFbWindowEGL(QWindow *tlw, QDirectFbInput *input)
    : QDirectFbWindow(tlw, input)
    , m_eglSurface(EGL_NO_SURFACE)
{}

QDirectFbWindowEGL::~QDirectFbWindowEGL()
{
    if (m_eglSurface != EGL_NO_SURFACE) {
        QDirectFbScreenEGL *dfbScreen;
        dfbScreen = static_cast<QDirectFbScreenEGL*>(screen());
        eglDestroySurface(dfbScreen->eglDisplay(), m_eglSurface);
    }
}

void QDirectFbWindowEGL::createDirectFBWindow()
{
    // Use the default for the raster surface.
    if (window()->surfaceType() == QSurface::RasterSurface)
        return QDirectFbWindow::createDirectFBWindow();

    Q_ASSERT(!m_dfbWindow.data());

    DFBWindowDescription description;
    memset(&description, 0, sizeof(DFBWindowDescription));
    description.flags = DFBWindowDescriptionFlags(DWDESC_WIDTH | DWDESC_HEIGHT|
                                                  DWDESC_POSX | DWDESC_POSY|
                                                  DWDESC_PIXELFORMAT | DWDESC_SURFACE_CAPS);
    description.width = qMax(1, window()->width());
    description.height = qMax(1, window()->height());
    description.posx = window()->x();
    description.posy = window()->y();

    description.surface_caps = DSCAPS_GL;
    description.pixelformat = DSPF_RGB16;

    IDirectFBDisplayLayer *layer;
    layer = toDfbScreen(window())->dfbLayer();
    DFBResult result = layer->CreateWindow(layer, &description, m_dfbWindow.outPtr());
    if (result != DFB_OK)
        DirectFBError("QDirectFbWindow: failed to create window", result);

    m_dfbWindow->SetOpacity(m_dfbWindow.data(), 0xff);
    m_inputHandler->addWindow(m_dfbWindow.data(), window());
}

EGLSurface QDirectFbWindowEGL::eglSurface()
{
    if (m_eglSurface == EGL_NO_SURFACE) {
        QDirectFbScreenEGL *dfbScreen = static_cast<QDirectFbScreenEGL *>(screen());
        EGLConfig config = q_configFromGLFormat(dfbScreen->eglDisplay(), format(), true);
        m_eglSurface = eglCreateWindowSurface(dfbScreen->eglDisplay(), config, dfbSurface(), NULL);

        if (m_eglSurface == EGL_NO_SURFACE)
            eglGetError();
    }

    return m_eglSurface;
}

QSurfaceFormat QDirectFbWindowEGL::format() const
{
    return window()->requestedFormat();
}


QDirectFbEGLContext::QDirectFbEGLContext(QDirectFbScreenEGL *screen, QOpenGLContext *context)
    : QEGLPlatformContext(context->format(), context->shareHandle(), screen->eglDisplay())
    , m_screen(screen)
{}

EGLSurface QDirectFbEGLContext::eglSurfaceForPlatformSurface(QPlatformSurface *surface)
{
    QDirectFbWindowEGL *window = static_cast<QDirectFbWindowEGL*>(surface);
    return window->eglSurface();
}

QPlatformWindow *QDirectFbIntegrationEGL::createPlatformWindow(QWindow *window) const
{
    QDirectFbWindow *dfbWindow = new QDirectFbWindowEGL(window, m_input.data());
    dfbWindow->createDirectFBWindow();
    return dfbWindow;
}

QPlatformOpenGLContext *QDirectFbIntegrationEGL::createPlatformOpenGLContext(QOpenGLContext *context) const
{
    QDirectFbScreenEGL *screen;
    screen = static_cast<QDirectFbScreenEGL*>(context->screen()->handle());
    return new QDirectFbEGLContext(screen, context);
}

void QDirectFbIntegrationEGL::initializeScreen()
{
    m_primaryScreen.reset(new QDirectFbScreenEGL(0));
    screenAdded(m_primaryScreen.data());
}

bool QDirectFbIntegrationEGL::hasCapability(QPlatformIntegration::Capability cap) const
{
    // We assume that devices will have more and not less capabilities
    if (hooks && hooks->hasCapability(cap))
        return true;
    return QDirectFbIntegration::hasCapability(cap);
}

QT_END_NAMESPACE
