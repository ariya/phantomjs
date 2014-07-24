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

#include "qopenglgradientcache_p.h"
#include <private/qdrawhelper_p.h>
#include <private/qopenglcontext_p.h>
#include <QtCore/qmutex.h>
#include <QtGui/qopenglfunctions.h>

QT_BEGIN_NAMESPACE

class QOpenGL2GradientCacheWrapper
{
public:
    QOpenGL2GradientCache *cacheForContext(QOpenGLContext *context) {
        QMutexLocker lock(&m_mutex);
        return m_resource.value<QOpenGL2GradientCache>(context);
    }

private:
    QOpenGLMultiGroupSharedResource m_resource;
    QMutex m_mutex;
};

Q_GLOBAL_STATIC(QOpenGL2GradientCacheWrapper, qt_gradient_caches)

QOpenGL2GradientCache::QOpenGL2GradientCache(QOpenGLContext *ctx)
    : QOpenGLSharedResource(ctx->shareGroup())
{
}

QOpenGL2GradientCache::~QOpenGL2GradientCache()
{
    cache.clear();
}

QOpenGL2GradientCache *QOpenGL2GradientCache::cacheForContext(QOpenGLContext *context)
{
    return qt_gradient_caches()->cacheForContext(context);
}

void QOpenGL2GradientCache::invalidateResource()
{
    QMutexLocker lock(&m_mutex);
    cache.clear();
}

void QOpenGL2GradientCache::freeResource(QOpenGLContext *)
{
    cleanCache();
}

void QOpenGL2GradientCache::cleanCache()
{
    QMutexLocker lock(&m_mutex);
    QOpenGLGradientColorTableHash::const_iterator it = cache.constBegin();
    QOpenGLFunctions *funcs = QOpenGLContext::currentContext()->functions();
    for (; it != cache.constEnd(); ++it) {
        const CacheInfo &cache_info = it.value();
        funcs->glDeleteTextures(1, &cache_info.texId);
    }
    cache.clear();
}

GLuint QOpenGL2GradientCache::getBuffer(const QGradient &gradient, qreal opacity)
{
    QMutexLocker lock(&m_mutex);
    quint64 hash_val = 0;

    QGradientStops stops = gradient.stops();
    for (int i = 0; i < stops.size() && i <= 2; i++)
        hash_val += stops[i].second.rgba();

    QOpenGLGradientColorTableHash::const_iterator it = cache.constFind(hash_val);

    if (it == cache.constEnd())
        return addCacheElement(hash_val, gradient, opacity);
    else {
        do {
            const CacheInfo &cache_info = it.value();
            if (cache_info.stops == stops && cache_info.opacity == opacity
                && cache_info.interpolationMode == gradient.interpolationMode())
            {
                return cache_info.texId;
            }
            ++it;
        } while (it != cache.constEnd() && it.key() == hash_val);
        // an exact match for these stops and opacity was not found, create new cache
        return addCacheElement(hash_val, gradient, opacity);
    }
}


GLuint QOpenGL2GradientCache::addCacheElement(quint64 hash_val, const QGradient &gradient, qreal opacity)
{
    QOpenGLFunctions *funcs = QOpenGLContext::currentContext()->functions();
    if (cache.size() == maxCacheSize()) {
        int elem_to_remove = qrand() % maxCacheSize();
        quint64 key = cache.keys()[elem_to_remove];

        // need to call glDeleteTextures on each removed cache entry:
        QOpenGLGradientColorTableHash::const_iterator it = cache.constFind(key);
        do {
            funcs->glDeleteTextures(1, &it.value().texId);
        } while (++it != cache.constEnd() && it.key() == key);
        cache.remove(key); // may remove more than 1, but OK
    }

    CacheInfo cache_entry(gradient.stops(), opacity, gradient.interpolationMode());
    uint buffer[1024];
    generateGradientColorTable(gradient, buffer, paletteSize(), opacity);
    funcs->glGenTextures(1, &cache_entry.texId);
    funcs->glBindTexture(GL_TEXTURE_2D, cache_entry.texId);
    funcs->glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, paletteSize(), 1,
                        0, GL_RGBA, GL_UNSIGNED_BYTE, buffer);
    return cache.insert(hash_val, cache_entry).value().texId;
}


//TODO: Let GL generate the texture using an FBO
void QOpenGL2GradientCache::generateGradientColorTable(const QGradient& gradient, uint *colorTable, int size, qreal opacity) const
{
    int pos = 0;
    QGradientStops s = gradient.stops();
    QVector<uint> colors(s.size());

    for (int i = 0; i < s.size(); ++i)
        colors[i] = s[i].second.rgba(); // Qt LIES! It returns ARGB (on little-endian AND on big-endian)

    bool colorInterpolation = (gradient.interpolationMode() == QGradient::ColorInterpolation);

    uint alpha = qRound(opacity * 256);
    uint current_color = ARGB_COMBINE_ALPHA(colors[0], alpha);
    qreal incr = 1.0 / qreal(size);
    qreal fpos = 1.5 * incr;
    colorTable[pos++] = ARGB2RGBA(qPremultiply(current_color));

    while (fpos <= s.first().first) {
        colorTable[pos] = colorTable[pos - 1];
        pos++;
        fpos += incr;
    }

    if (colorInterpolation)
        current_color = qPremultiply(current_color);

    for (int i = 0; i < s.size() - 1; ++i) {
        qreal delta = 1/(s[i+1].first - s[i].first);
        uint next_color = ARGB_COMBINE_ALPHA(colors[i+1], alpha);
        if (colorInterpolation)
            next_color = qPremultiply(next_color);

        while (fpos < s[i+1].first && pos < size) {
            int dist = int(256 * ((fpos - s[i].first) * delta));
            int idist = 256 - dist;
            if (colorInterpolation)
                colorTable[pos] = ARGB2RGBA(INTERPOLATE_PIXEL_256(current_color, idist, next_color, dist));
            else
                colorTable[pos] = ARGB2RGBA(qPremultiply(INTERPOLATE_PIXEL_256(current_color, idist, next_color, dist)));
            ++pos;
            fpos += incr;
        }
        current_color = next_color;
    }

    Q_ASSERT(s.size() > 0);

    uint last_color = ARGB2RGBA(qPremultiply(ARGB_COMBINE_ALPHA(colors[s.size() - 1], alpha)));
    for (;pos < size; ++pos)
        colorTable[pos] = last_color;

    // Make sure the last color stop is represented at the end of the table
    colorTable[size-1] = last_color;
}

QT_END_NAMESPACE
