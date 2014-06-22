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

#ifndef QKMSSCREEN_H
#define QKMSSCREEN_H

#include <stddef.h>

#define EGL_EGLEXT_PROTOTYPES 1
#define GL_GLEXT_PROTOTYPES 1

extern "C" {
#include <gbm.h>
#include <xf86drmMode.h>
#include <xf86drm.h>
}

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <QtGui/qopengl.h>

#include <qpa/qplatformscreen.h>

QT_BEGIN_NAMESPACE

class QKmsCursor;
class QKmsDevice;
class QKmsContext;

class QKmsScreen : public QPlatformScreen
{
public:
    QKmsScreen(QKmsDevice *device, int connectorId);
    ~QKmsScreen();

    QRect geometry() const;
    int depth() const;
    QImage::Format format() const;
    QSizeF physicalSize() const;
    QPlatformCursor *cursor() const;

    quint32 crtcId() const { return m_crtcId; }
    QKmsDevice *device() const;

    void initializeWithFormat(const QSurfaceFormat &format);

    //Called by context for each screen
    void swapBuffers();
    void handlePageFlipped();

    EGLSurface eglSurface() const { return m_eglWindowSurface; }

    void waitForPageFlipComplete();

    static QSurfaceFormat tweakFormat(const QSurfaceFormat &format);

private:
    void performPageFlip();
    void initializeScreenMode();

    QKmsDevice *m_device;
    gbm_bo *m_current_bo;
    gbm_bo *m_next_bo;
    quint32 m_connectorId;

    quint32 m_crtcId;
    drmModeModeInfo m_mode;
    QRect m_geometry;
    QSizeF m_physicalSize;
    int m_depth;
    QImage::Format m_format;

    drmModeCrtcPtr m_oldCrtc;

    QKmsCursor *m_cursor;

    gbm_surface *m_gbmSurface;
    EGLSurface m_eglWindowSurface;

    bool m_modeSet;
};

QT_END_NAMESPACE

#endif // QKMSSCREEN_H
