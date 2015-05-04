/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the qmake spec of the Qt Toolkit.
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

#include "qeglfshooks.h"
#include <QtPlatformSupport/private/qdevicediscovery_p.h>
#include <QtCore/private/qcore_unix_p.h>
#include <QtCore/QScopedPointer>
#include <QtGui/qpa/qplatformwindow.h>

#include <xf86drm.h>
#include <xf86drmMode.h>
#include <gbm.h>

QT_USE_NAMESPACE

class QEglKmsHooks : public QEglFSHooks
{
public:
    QEglKmsHooks();

    void platformInit() Q_DECL_OVERRIDE;
    void platformDestroy() Q_DECL_OVERRIDE;
    EGLNativeDisplayType platformDisplay() const Q_DECL_OVERRIDE;
    QSizeF physicalScreenSize() const Q_DECL_OVERRIDE;
    QSize screenSize() const Q_DECL_OVERRIDE;
    int screenDepth() const Q_DECL_OVERRIDE;
    QSurfaceFormat surfaceFormatFor(const QSurfaceFormat &inputFormat) const Q_DECL_OVERRIDE;
    EGLNativeWindowType createNativeWindow(QPlatformWindow *platformWindow,
                                           const QSize &size,
                                           const QSurfaceFormat &format) Q_DECL_OVERRIDE;
    void destroyNativeWindow(EGLNativeWindowType window) Q_DECL_OVERRIDE;
    bool hasCapability(QPlatformIntegration::Capability cap) const Q_DECL_OVERRIDE;
    void waitForVSync() const Q_DECL_OVERRIDE;

    void waitForVSyncImpl();
    bool setup_kms();

    struct FrameBuffer {
        FrameBuffer() : fb(0) {}
        uint32_t fb;
    };
    FrameBuffer *framebufferForBufferObject(gbm_bo *bo);

private:
    // device bits
    QByteArray m_device;
    int m_dri_fd;
    gbm_device *m_gbm_device;

    // KMS bits
    drmModeConnector *m_drm_connector;
    drmModeEncoder *m_drm_encoder;
    drmModeModeInfo m_drm_mode;
    quint32 m_drm_crtc;

    // Drawing bits
    gbm_surface *m_gbm_surface;
};

static QEglKmsHooks kms_hooks;
QEglFSHooks *platformHooks = &kms_hooks;

QEglKmsHooks::QEglKmsHooks()
    : m_dri_fd(-1)
    , m_gbm_device(Q_NULLPTR)
    , m_drm_connector(Q_NULLPTR)
    , m_drm_encoder(Q_NULLPTR)
    , m_drm_crtc(0)
    , m_gbm_surface(Q_NULLPTR)
{

}

void QEglKmsHooks::platformInit()
{
    QDeviceDiscovery *d = QDeviceDiscovery::create(QDeviceDiscovery::Device_VideoMask);
    QStringList devices = d->scanConnectedDevices();
    d->deleteLater();

    if (devices.isEmpty())
        qFatal("Could not find DRM device!");

    m_device = devices.first().toLocal8Bit();
    m_dri_fd = qt_safe_open(m_device.constData(), O_RDWR | O_CLOEXEC);
    if (m_dri_fd == -1) {
        qErrnoWarning("Could not open DRM device %s", m_device.constData());
        qFatal("DRM device required, aborting.");
    }

    if (!setup_kms())
        qFatal("Could not set up KMS on device %s!", m_device.constData());

    m_gbm_device = gbm_create_device(m_dri_fd);
    if (!m_gbm_device)
        qFatal("Could not initialize gbm on device %s!", m_device.constData());
}

void QEglKmsHooks::platformDestroy()
{
    gbm_device_destroy(m_gbm_device);
    m_gbm_device = Q_NULLPTR;

    if (qt_safe_close(m_dri_fd) == -1)
        qErrnoWarning("Could not close DRM device %s", m_device.constData());

    m_dri_fd = -1;
}

EGLNativeDisplayType QEglKmsHooks::platformDisplay() const
{
    return static_cast<EGLNativeDisplayType>(m_gbm_device);
}

QSizeF QEglKmsHooks::physicalScreenSize() const
{
    return QSizeF(m_drm_connector->mmWidth,
                  m_drm_connector->mmHeight);
}

QSize QEglKmsHooks::screenSize() const
{
    return QSize(m_drm_mode.hdisplay,
                 m_drm_mode.vdisplay);
}

int QEglKmsHooks::screenDepth() const
{
    return 32;
}

QSurfaceFormat QEglKmsHooks::surfaceFormatFor(const QSurfaceFormat &inputFormat) const
{
    QSurfaceFormat format(inputFormat);
    format.setRenderableType(QSurfaceFormat::OpenGLES);
    format.setSwapBehavior(QSurfaceFormat::DoubleBuffer);
    format.setRedBufferSize(8);
    format.setGreenBufferSize(8);
    format.setBlueBufferSize(8);
    return format;
}

EGLNativeWindowType QEglKmsHooks::createNativeWindow(QPlatformWindow *platformWindow,
                                                     const QSize &size,
                                                     const QSurfaceFormat &format)
{
    Q_UNUSED(platformWindow);
    Q_UNUSED(size);
    Q_UNUSED(format);

    if (m_gbm_surface) {
        qWarning("Only single window apps supported!");
        return 0;
    }

    m_gbm_surface = gbm_surface_create(m_gbm_device,
                                       screenSize().width(),
                                       screenSize().height(),
                                       GBM_FORMAT_XRGB8888,
                                       GBM_BO_USE_SCANOUT | GBM_BO_USE_RENDERING);
    if (!m_gbm_surface)
        qFatal("Could not initialize GBM surface");

    return reinterpret_cast<EGLNativeWindowType>(m_gbm_surface);
}

void QEglKmsHooks::destroyNativeWindow(EGLNativeWindowType window)
{
    gbm_surface *surface = reinterpret_cast<gbm_surface *>(window);
    if (surface == m_gbm_surface)
        m_gbm_surface = Q_NULLPTR;
    gbm_surface_destroy(surface);
}

bool QEglKmsHooks::hasCapability(QPlatformIntegration::Capability cap) const
{
    switch (cap) {
    case QPlatformIntegration::ThreadedPixmaps:
    case QPlatformIntegration::OpenGL:
    case QPlatformIntegration::ThreadedOpenGL:
    case QPlatformIntegration::BufferQueueingOpenGL:
        return true;
    default:
        return false;
    }
}

static void gbm_bo_destroyed_callback(gbm_bo *bo, void *data)
{
    QEglKmsHooks::FrameBuffer *fb = static_cast<QEglKmsHooks::FrameBuffer *>(data);

    if (fb->fb) {
        gbm_device *device = gbm_bo_get_device(bo);
        drmModeRmFB(gbm_device_get_fd(device), fb->fb);
    }

    delete fb;
}

QEglKmsHooks::FrameBuffer *QEglKmsHooks::framebufferForBufferObject(gbm_bo *bo)
{
    {
        FrameBuffer *fb = static_cast<FrameBuffer *>(gbm_bo_get_user_data(bo));
        if (fb)
            return fb;
    }

    uint32_t width = gbm_bo_get_width(bo);
    uint32_t height = gbm_bo_get_height(bo);
    uint32_t stride = gbm_bo_get_stride(bo);
    uint32_t handle = gbm_bo_get_handle(bo).u32;

    QScopedPointer<FrameBuffer> fb(new FrameBuffer);

    int ret = drmModeAddFB(m_dri_fd, width, height, 24, 32,
                           stride, handle, &fb->fb);

    if (ret) {
        qWarning("Failed to create KMS FB!");
        return Q_NULLPTR;
    }

    gbm_bo_set_user_data(bo, fb.data(), gbm_bo_destroyed_callback);
    return fb.take();
}

static void page_flip_handler(int fd,
                              unsigned int sequence,
                              unsigned int tv_sec,
                              unsigned int tv_usec,
                              void *user_data)
{
    Q_UNUSED(fd);
    Q_UNUSED(sequence);
    Q_UNUSED(tv_sec);
    Q_UNUSED(tv_usec);

    // We are no longer flipping
    *static_cast<bool *>(user_data) = false;
}

void QEglKmsHooks::waitForVSync() const
{
    const_cast<QEglKmsHooks*>(this)->waitForVSyncImpl();
}

void QEglKmsHooks::waitForVSyncImpl()
{
    if (!m_gbm_surface) {
        qWarning("Cannot sync before platform init!");
        return;
    }

    if (!gbm_surface_has_free_buffers(m_gbm_surface)) {
        qWarning("Out of free GBM buffers!");
        return;
    }

    gbm_bo *front_buffer = gbm_surface_lock_front_buffer(m_gbm_surface);
    if (!front_buffer) {
        qWarning("Could not lock GBM surface front buffer!");
        return;
    }

    QEglKmsHooks::FrameBuffer *fb = framebufferForBufferObject(front_buffer);

    int ret = drmModeSetCrtc(m_dri_fd,
                             m_drm_crtc,
                             fb->fb,
                             0, 0,
                             &m_drm_connector->connector_id, 1,
                             &m_drm_mode);
    if (ret) {
        qErrnoWarning("Could not set DRM mode!");
        return;
    }

    bool flipping = true;
    ret = drmModePageFlip(m_dri_fd,
                          m_drm_encoder->crtc_id,
                          fb->fb,
                          DRM_MODE_PAGE_FLIP_EVENT,
                          &flipping);
    if (ret) {
        qErrnoWarning("Could not queue DRM page flip!");
        return;
    }

    drmEventContext drmEvent = {
        DRM_EVENT_CONTEXT_VERSION,
        Q_NULLPTR,          // vblank handler
        page_flip_handler   // page flip handler
    };

    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(m_dri_fd, &fds);

    time_t start, cur;
    time(&start);

    while (flipping && (time(&cur) < start + 1)) {
        timespec v;
        memset(&v, 0, sizeof(v));
        v.tv_sec = start + 1 - cur;

        ret = qt_safe_select(m_dri_fd + 1, &fds, Q_NULLPTR, Q_NULLPTR, &v);

        if (ret == 0) {
            // timeout
            break;
        } else if (ret == -1) {
            qErrnoWarning("Error while selecting on DRM fd");
            break;
        } else if (drmHandleEvent(m_dri_fd, &drmEvent)) {
            qWarning("Could not handle DRM event!");
        }
    }

    gbm_surface_release_buffer(m_gbm_surface, front_buffer);
}

bool QEglKmsHooks::setup_kms()
{
    drmModeRes *resources;
    drmModeConnector *connector;
    drmModeEncoder *encoder;
    quint32 crtc = 0;
    int i;

    resources = drmModeGetResources(m_dri_fd);
    if (!resources) {
       qWarning("drmModeGetResources failed");
       return false;
    }

    for (i = 0; i < resources->count_connectors; i++) {
       connector = drmModeGetConnector(m_dri_fd, resources->connectors[i]);
       if (connector == NULL)
          continue;

       if (connector->connection == DRM_MODE_CONNECTED &&
           connector->count_modes > 0) {
          break;
       }

       drmModeFreeConnector(connector);
    }

    if (i == resources->count_connectors) {
       qWarning("No currently active connector found.");
       return false;
    }

    for (i = 0; i < resources->count_encoders; i++) {
       encoder = drmModeGetEncoder(m_dri_fd, resources->encoders[i]);

       if (encoder == NULL)
          continue;

       if (encoder->encoder_id == connector->encoder_id)
          break;

       drmModeFreeEncoder(encoder);
    }

    for (int j = 0; j < resources->count_crtcs; j++) {
        if ((encoder->possible_crtcs & (1 << j))) {
            crtc = resources->crtcs[j];
            break;
        }
    }

    if (crtc == 0)
        qFatal("No suitable CRTC available");

    m_drm_connector = connector;
    m_drm_encoder = encoder;
    m_drm_mode = connector->modes[0];
    m_drm_crtc = crtc;

    drmModeFreeResources(resources);

    return true;
}
