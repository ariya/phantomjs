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

#include <QtGui/QOpenGLShaderProgram>
#include <QtGui/QOpenGLContext>
#include <QtGui/QPainter>

#include "qeglplatformbackingstore_p.h"
#include "qeglcompositor_p.h"
#include "qeglplatformwindow_p.h"
#include "qeglplatformscreen_p.h"

QT_BEGIN_NAMESPACE

/*!
    \class QEGLPlatformBackingStore
    \brief A backing store implementation for EGL and GLES.
    \since 5.2
    \internal
    \ingroup qpa

    This implementation uploads raster-rendered widget windows into
    textures and composites them onto a single native window using
    QEGLCompositor. This means that multiple top-level widgets are
    supported without creating actual native windows for each of them.

    The class is ready to be used as-is, the default
    QEGLPlatformIntegration::createPlatformBackingStore()
    implementation creates an instance which is ready to be used
    without further customization.

    If QEGLCompositor is not suitable, this backing store
    implementation can also be used without it. In this case a
    subclass must reimplement composite() and schedule an update in
    its custom compositor when this function is called. The textures
    are accessible via QEGLPlatformWindow::texture().
*/

QEGLPlatformBackingStore::QEGLPlatformBackingStore(QWindow *window)
    : QPlatformBackingStore(window),
      m_window(static_cast<QEGLPlatformWindow *>(window->handle())),
      m_bsTexture(0),
      m_textures(new QPlatformTextureList),
      m_lockedWidgetTextures(0)
{
    m_window->setBackingStore(this);
}

QEGLPlatformBackingStore::~QEGLPlatformBackingStore()
{
    delete m_textures;
}

QPaintDevice *QEGLPlatformBackingStore::paintDevice()
{
    return &m_image;
}

void QEGLPlatformBackingStore::updateTexture()
{
    if (!m_bsTexture) {
        glGenTextures(1, &m_bsTexture);
        glBindTexture(GL_TEXTURE_2D, m_bsTexture);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, m_image.width(), m_image.height(), 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
    } else {
        glBindTexture(GL_TEXTURE_2D, m_bsTexture);
    }

    if (!m_dirty.isNull()) {
        QRegion fixed;
        QRect imageRect = m_image.rect();

        foreach (const QRect &rect, m_dirty.rects()) {
            // intersect with image rect to be sure
            QRect r = imageRect & rect;

            // if the rect is wide enough it's cheaper to just
            // extend it instead of doing an image copy
            if (r.width() >= imageRect.width() / 2) {
                r.setX(0);
                r.setWidth(imageRect.width());
            }

            fixed |= r;
        }

        foreach (const QRect &rect, fixed.rects()) {
            // if the sub-rect is full-width we can pass the image data directly to
            // OpenGL instead of copying, since there's no gap between scanlines
            if (rect.width() == imageRect.width()) {
                glTexSubImage2D(GL_TEXTURE_2D, 0, 0, rect.y(), rect.width(), rect.height(), GL_RGBA, GL_UNSIGNED_BYTE,
                                m_image.constScanLine(rect.y()));
            } else {
                glTexSubImage2D(GL_TEXTURE_2D, 0, rect.x(), rect.y(), rect.width(), rect.height(), GL_RGBA, GL_UNSIGNED_BYTE,
                    m_image.copy(rect).constBits());
            }
        }

        m_dirty = QRegion();
    }
}

void QEGLPlatformBackingStore::flush(QWindow *window, const QRegion &region, const QPoint &offset)
{
    // Called for ordinary raster windows. This is rare since RasterGLSurface
    // support is claimed which leads to having all QWidget windows marked as
    // RasterGLSurface instead of just Raster. These go through
    // compositeAndFlush() instead of this function.

    Q_UNUSED(region);
    Q_UNUSED(offset);

    QEGLPlatformScreen *screen = static_cast<QEGLPlatformScreen *>(m_window->screen());
    QEGLPlatformWindow *dstWin = screen->compositingWindow();
    if (!dstWin || !dstWin->isRaster())
        return;

    screen->compositingContext()->makeCurrent(dstWin->window());
    updateTexture();
    m_textures->clear();
    m_textures->appendTexture(Q_NULLPTR, m_bsTexture, window->geometry());
    composite(screen->compositingContext(), dstWin);
}

void QEGLPlatformBackingStore::composeAndFlush(QWindow *window, const QRegion &region, const QPoint &offset,
                                               QPlatformTextureList *textures, QOpenGLContext *context,
                                               bool translucentBackground)
{
    // QOpenGLWidget content provided as textures. The raster content should go on top.

    Q_UNUSED(region);
    Q_UNUSED(offset);
    Q_UNUSED(context);
    Q_UNUSED(translucentBackground);

    QEGLPlatformScreen *screen = static_cast<QEGLPlatformScreen *>(m_window->screen());
    QEGLPlatformWindow *dstWin = screen->compositingWindow();
    if (!dstWin || !dstWin->isRaster())
        return;

    screen->compositingContext()->makeCurrent(dstWin->window());

    m_textures->clear();
    for (int i = 0; i < textures->count(); ++i)
        m_textures->appendTexture(textures->widget(i), textures->textureId(i), textures->geometry(i), textures->flags(i));

    updateTexture();
    m_textures->appendTexture(Q_NULLPTR, m_bsTexture, window->geometry());

    textures->lock(true);
    m_lockedWidgetTextures = textures;

    composite(screen->compositingContext(), dstWin);
}

void QEGLPlatformBackingStore::composite(QOpenGLContext *context, QEGLPlatformWindow *window)
{
    QEGLCompositor::instance()->schedule(context, window);
}

void QEGLPlatformBackingStore::composited()
{
    if (m_lockedWidgetTextures) {
        QPlatformTextureList *textureList = m_lockedWidgetTextures;
        m_lockedWidgetTextures = 0; // may reenter so null before unlocking
        textureList->lock(false);
    }
}

void QEGLPlatformBackingStore::beginPaint(const QRegion &region)
{
    m_dirty |= region;

    if (m_image.hasAlphaChannel()) {
        QPainter p(&m_image);
        p.setCompositionMode(QPainter::CompositionMode_Source);
        foreach (const QRect &r, region.rects())
            p.fillRect(r, Qt::transparent);
    }
}

void QEGLPlatformBackingStore::resize(const QSize &size, const QRegion &staticContents)
{
    Q_UNUSED(staticContents);

    QEGLPlatformScreen *screen = static_cast<QEGLPlatformScreen *>(m_window->screen());
    QEGLPlatformWindow *dstWin = screen->compositingWindow();
    if (!dstWin || (!dstWin->isRaster() && dstWin->window()->surfaceType() != QSurface::RasterGLSurface))
        return;

    m_image = QImage(size, QImage::Format_RGBA8888);

    m_window->create();

    screen->compositingContext()->makeCurrent(dstWin->window());
    if (m_bsTexture) {
        glDeleteTextures(1, &m_bsTexture);
        m_bsTexture = 0;
    }
}

QImage QEGLPlatformBackingStore::toImage() const
{
    return m_image;
}

QT_END_NAMESPACE
