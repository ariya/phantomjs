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
//#include <QDebug>

#include "qkmsscreen.h"
#include "qkmscursor.h"
#include "qkmsdevice.h"
#include "qkmscontext.h"

#include <QtPlatformSupport/private/qeglconvenience_p.h>

#include <QCoreApplication>
#include <QtDebug>

QT_BEGIN_NAMESPACE

//Fallback mode (taken from Wayland DRM demo compositor)
static drmModeModeInfo builtin_1024x768 = {
    63500, //clock
    1024, 1072, 1176, 1328, 0,
    768, 771, 775, 798, 0,
    59920,
    DRM_MODE_FLAG_NHSYNC | DRM_MODE_FLAG_PVSYNC,
    0,
    "1024x768"
};

QKmsScreen::QKmsScreen(QKmsDevice *device, int connectorId)
    : m_device(device),
      m_current_bo(0),
      m_next_bo(0),
      m_connectorId(connectorId),
      m_depth(32),
      m_format(QImage::Format_Invalid),
      m_eglWindowSurface(EGL_NO_SURFACE),
      m_modeSet(false)
{
    m_cursor = new QKmsCursor(this);
    initializeScreenMode();
}

QKmsScreen::~QKmsScreen()
{
    delete m_cursor;
    drmModeSetCrtc(m_device->fd(), m_oldCrtc->crtc_id, m_oldCrtc->buffer_id,
                   m_oldCrtc->x, m_oldCrtc->y,
                   &m_connectorId, 1, &m_oldCrtc->mode);
    drmModeFreeCrtc(m_oldCrtc);
    if (m_eglWindowSurface != EGL_NO_SURFACE)
        eglDestroySurface(m_device->eglDisplay(), m_eglWindowSurface);
    gbm_surface_destroy(m_gbmSurface);
}

QRect QKmsScreen::geometry() const
{
    return m_geometry;
}

int QKmsScreen::depth() const
{
    return m_depth;
}

QImage::Format QKmsScreen::format() const
{
    return m_format;
}

QSizeF QKmsScreen::physicalSize() const
{
    return m_physicalSize;
}

QPlatformCursor *QKmsScreen::cursor() const
{
    return m_cursor;
}

void QKmsScreen::initializeScreenMode()
{
    //Determine optimal mode for screen
    drmModeRes *resources = drmModeGetResources(m_device->fd());
    if (!resources)
        qFatal("drmModeGetResources failed");

    drmModeConnector *connector = drmModeGetConnector(m_device->fd(), m_connectorId);
    drmModeModeInfo *mode = 0;
    for (int i = 0; i < connector->count_modes; ++i) {
        if (connector->modes[i].type & DRM_MODE_TYPE_PREFERRED) {
            mode = &connector->modes[i];
            break;
        }
    }
    if (!mode)
        mode = &builtin_1024x768;

    drmModeEncoder *encoder = drmModeGetEncoder(m_device->fd(), connector->encoders[0]);
    if (encoder == 0)
        qFatal("No encoder for connector.");

    int i;
    for (i = 0; i < resources->count_crtcs; i++) {
        if (encoder->possible_crtcs & (1 << i))
            break;
    }
    if (i == resources->count_crtcs)
        qFatal("No usable crtc for encoder.");

    m_oldCrtc = drmModeGetCrtc(m_device->fd(), encoder->crtc_id);

    m_crtcId = resources->crtcs[i];
    m_mode = *mode;
    m_geometry = QRect(0, 0, m_mode.hdisplay, m_mode.vdisplay);
    qDebug() << "kms initialized with geometry" << m_geometry;
    m_depth = 32;
    m_format = QImage::Format_RGB32;
    m_physicalSize = QSizeF(connector->mmWidth, connector->mmHeight);

    m_gbmSurface = gbm_surface_create(m_device->gbmDevice(),
                                      m_mode.hdisplay, m_mode.vdisplay,
                                      GBM_BO_FORMAT_XRGB8888,
                                      GBM_BO_USE_SCANOUT | GBM_BO_USE_RENDERING);

    qDebug() << "created gbm surface" << m_gbmSurface << m_mode.hdisplay << m_mode.vdisplay;
    //Cleanup
    drmModeFreeEncoder(encoder);
    drmModeFreeConnector(connector);
    drmModeFreeResources(resources);
}

QSurfaceFormat QKmsScreen::tweakFormat(const QSurfaceFormat &format)
{
    QSurfaceFormat fmt = format;
    fmt.setRedBufferSize(8);
    fmt.setGreenBufferSize(8);
    fmt.setBlueBufferSize(8);
    if (fmt.alphaBufferSize() != -1)
        fmt.setAlphaBufferSize(8);
    return fmt;
}

void QKmsScreen::initializeWithFormat(const QSurfaceFormat &format)
{
    EGLDisplay display = m_device->eglDisplay();
    EGLConfig config = q_configFromGLFormat(display, tweakFormat(format), true);

    m_eglWindowSurface = eglCreateWindowSurface(display, config, (EGLNativeWindowType)m_gbmSurface, NULL);
    qDebug() << "created window surface";
}

void QKmsScreen::swapBuffers()
{
    eglSwapBuffers(m_device->eglDisplay(), m_eglWindowSurface);

    m_next_bo = gbm_surface_lock_front_buffer(m_gbmSurface);
    if (!m_next_bo)
        qFatal("kms: Failed to lock front buffer");

    performPageFlip();
}

void QKmsScreen::performPageFlip()
{
    if (!m_next_bo)
        return;

    uint32_t width = gbm_bo_get_width(m_next_bo);
    uint32_t height = gbm_bo_get_height(m_next_bo);
    uint32_t stride = gbm_bo_get_stride(m_next_bo);
    uint32_t handle = gbm_bo_get_handle(m_next_bo).u32;

    uint32_t fb_id;
    int ret = drmModeAddFB(m_device->fd(), width, height, 24, 32,
                           stride, handle, &fb_id);
    if (ret) {
        qFatal("kms: Failed to create fb: fd %d, w %d, h %d, stride %d, handle %d, ret %d",
               m_device->fd(), width, height, stride, handle, ret);
    }

    if (!m_modeSet) {
        //Set the Mode of the screen.
        int ret = drmModeSetCrtc(m_device->fd(), m_crtcId, fb_id,
                0, 0, &m_connectorId, 1, &m_mode);
        if (ret)
            qFatal("failed to set mode");
        m_modeSet = true;

        // Initialize cursor

        static int hideCursor = qgetenv("QT_QPA_KMS_HIDECURSOR").toInt();
        if (!hideCursor) {
            QCursor cursor(Qt::ArrowCursor);
            m_cursor->changeCursor(&cursor, 0);
        }
    }

    int pageFlipStatus = drmModePageFlip(m_device->fd(), m_crtcId,
                                         fb_id,
                                         DRM_MODE_PAGE_FLIP_EVENT, this);
    if (pageFlipStatus)
    {
        qWarning("Pageflip status: %d", pageFlipStatus);
        gbm_surface_release_buffer(m_gbmSurface, m_next_bo);
        m_next_bo = 0;
    }
}

void QKmsScreen::handlePageFlipped()
{
    if (m_current_bo)
        gbm_surface_release_buffer(m_gbmSurface, m_current_bo);

    m_current_bo = m_next_bo;
    m_next_bo = 0;
}

QKmsDevice * QKmsScreen::device() const
{
    return m_device;
}

void QKmsScreen::waitForPageFlipComplete()
{
    while (m_next_bo) {
#if 0
        //Check manually if there is something to be read on the device
        //as there are senarios where the signal is not received (starvation)
        fd_set fdSet;
        timeval timeValue;
        int returnValue;

        FD_ZERO(&fdSet);
        FD_SET(m_device->fd(), &fdSet);
        timeValue.tv_sec = 0;
        timeValue.tv_usec = 1000;

        returnValue = select(1, &fdSet, 0, 0, &timeValue);
        printf("select returns %d\n", returnValue);
#endif

        m_device->handlePageFlipCompleted();
    }
}


QT_END_NAMESPACE
