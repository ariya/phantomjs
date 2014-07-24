/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtGui module of the Qt Toolkit.
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

#include "qopengltexturecache_p.h"
#include <qopenglfunctions.h>
#include <private/qopenglcontext_p.h>
#include <private/qimagepixmapcleanuphooks_p.h>
#include <qpa/qplatformpixmap.h>

QT_BEGIN_NAMESPACE

class QOpenGLTextureCacheWrapper
{
public:
    QOpenGLTextureCacheWrapper()
    {
        QImagePixmapCleanupHooks::instance()->addPlatformPixmapModificationHook(cleanupTexturesForPixmapData);
        QImagePixmapCleanupHooks::instance()->addPlatformPixmapDestructionHook(cleanupTexturesForPixmapData);
        QImagePixmapCleanupHooks::instance()->addImageHook(cleanupTexturesForCacheKey);
    }

    ~QOpenGLTextureCacheWrapper()
    {
        QImagePixmapCleanupHooks::instance()->removePlatformPixmapModificationHook(cleanupTexturesForPixmapData);
        QImagePixmapCleanupHooks::instance()->removePlatformPixmapDestructionHook(cleanupTexturesForPixmapData);
        QImagePixmapCleanupHooks::instance()->removeImageHook(cleanupTexturesForCacheKey);
    }

    QOpenGLTextureCache *cacheForContext(QOpenGLContext *context) {
        QMutexLocker lock(&m_mutex);
        return m_resource.value<QOpenGLTextureCache>(context);
    }

    static void cleanupTexturesForCacheKey(qint64 key);
    static void cleanupTexturesForPixmapData(QPlatformPixmap *pmd);

private:
    QOpenGLMultiGroupSharedResource m_resource;
    QMutex m_mutex;
};

Q_GLOBAL_STATIC(QOpenGLTextureCacheWrapper, qt_texture_caches)

QOpenGLTextureCache *QOpenGLTextureCache::cacheForContext(QOpenGLContext *context)
{
    return qt_texture_caches()->cacheForContext(context);
}

void QOpenGLTextureCacheWrapper::cleanupTexturesForCacheKey(qint64 key)
{
    QList<QOpenGLSharedResource *> resources = qt_texture_caches()->m_resource.resources();
    for (QList<QOpenGLSharedResource *>::iterator it = resources.begin(); it != resources.end(); ++it)
        static_cast<QOpenGLTextureCache *>(*it)->invalidate(key);
}

void QOpenGLTextureCacheWrapper::cleanupTexturesForPixmapData(QPlatformPixmap *pmd)
{
    cleanupTexturesForCacheKey(pmd->cacheKey());
}

QOpenGLTextureCache::QOpenGLTextureCache(QOpenGLContext *ctx)
    : QOpenGLSharedResource(ctx->shareGroup())
    , m_cache(64 * 1024) // 64 MB cache
{
}

QOpenGLTextureCache::~QOpenGLTextureCache()
{
}

GLuint QOpenGLTextureCache::bindTexture(QOpenGLContext *context, const QPixmap &pixmap)
{
    if (pixmap.isNull())
        return 0;
    QMutexLocker locker(&m_mutex);
    qint64 key = pixmap.cacheKey();

    // A QPainter is active on the image - take the safe route and replace the texture.
    if (!pixmap.paintingActive()) {
        QOpenGLCachedTexture *entry = m_cache.object(key);
        if (entry) {
            context->functions()->glBindTexture(GL_TEXTURE_2D, entry->id());
            return entry->id();
        }
    }

    GLuint id = bindTexture(context, key, pixmap.toImage());
    if (id > 0)
        QImagePixmapCleanupHooks::enableCleanupHooks(pixmap);

    return id;
}

// returns the highest number closest to v, which is a power of 2
// NB! assumes 32 bit ints
static int qt_next_power_of_two(int v)
{
    v--;
    v |= v >> 1;
    v |= v >> 2;
    v |= v >> 4;
    v |= v >> 8;
    v |= v >> 16;
    ++v;
    return v;
}

GLuint QOpenGLTextureCache::bindTexture(QOpenGLContext *context, const QImage &image)
{
    if (image.isNull())
        return 0;
    QMutexLocker locker(&m_mutex);
    qint64 key = image.cacheKey();

    // A QPainter is active on the image - take the safe route and replace the texture.
    if (!image.paintingActive()) {
        QOpenGLCachedTexture *entry = m_cache.object(key);
        if (entry) {
            context->functions()->glBindTexture(GL_TEXTURE_2D, entry->id());
            return entry->id();
        }
    }

    QImage img = image;
    if (!context->functions()->hasOpenGLFeature(QOpenGLFunctions::NPOTTextures)) {
        // Scale the pixmap if needed. GL textures needs to have the
        // dimensions 2^n+2(border) x 2^m+2(border), unless we're using GL
        // 2.0 or use the GL_TEXTURE_RECTANGLE texture target
        int tx_w = qt_next_power_of_two(image.width());
        int tx_h = qt_next_power_of_two(image.height());
        if (tx_w != image.width() || tx_h != image.height()) {
            img = img.scaled(tx_w, tx_h);
        }
    }

    GLuint id = bindTexture(context, key, img);
    if (id > 0)
        QImagePixmapCleanupHooks::enableCleanupHooks(image);

    return id;
}

GLuint QOpenGLTextureCache::bindTexture(QOpenGLContext *context, qint64 key, const QImage &image)
{
    GLuint id;
    QOpenGLFunctions *funcs = context->functions();
    funcs->glGenTextures(1, &id);
    funcs->glBindTexture(GL_TEXTURE_2D, id);

    QImage tx = image.convertToFormat(QImage::Format_RGBA8888_Premultiplied);

    funcs->glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, tx.width(), tx.height(), 0, GL_RGBA, GL_UNSIGNED_BYTE, const_cast<const QImage &>(tx).bits());

    int cost = tx.width() * tx.height() * 4 / 1024;
    m_cache.insert(key, new QOpenGLCachedTexture(id, context), cost);

    return id;
}

void QOpenGLTextureCache::invalidate(qint64 key)
{
    QMutexLocker locker(&m_mutex);
    m_cache.remove(key);
}

void QOpenGLTextureCache::invalidateResource()
{
    m_cache.clear();
}

void QOpenGLTextureCache::freeResource(QOpenGLContext *)
{
    Q_ASSERT(false); // the texture cache lives until the context group disappears
}

static void freeTexture(QOpenGLFunctions *funcs, GLuint id)
{
    funcs->glDeleteTextures(1, &id);
}

QOpenGLCachedTexture::QOpenGLCachedTexture(GLuint id, QOpenGLContext *context)
{
    m_resource = new QOpenGLSharedResourceGuard(context, id, freeTexture);
}

QT_END_NAMESPACE
