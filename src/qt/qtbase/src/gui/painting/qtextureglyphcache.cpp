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

#include <qmath.h>

#include "qtextureglyphcache_p.h"
#include "private/qfontengine_p.h"
#include "private/qnumeric_p.h"
#include "private/qnativeimage_p.h"

QT_BEGIN_NAMESPACE

// #define CACHE_DEBUG

// returns the highest number closest to v, which is a power of 2
// NB! assumes 32 bit ints
static inline int qt_next_power_of_two(int v)
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

int QTextureGlyphCache::calculateSubPixelPositionCount(glyph_t glyph) const
{
    // Test 12 different subpixel positions since it factors into 3*4 so it gives
    // the coverage we need.

    QList<QImage> images;
    for (int i=0; i<12; ++i) {
        QImage img = textureMapForGlyph(glyph, QFixed::fromReal(i / 12.0));

        if (images.isEmpty()) {
            QPainterPath path;
            QFixedPoint point;
            m_current_fontengine->addGlyphsToPath(&glyph, &point, 1, &path, QTextItem::RenderFlags());

            // Glyph is space, return 0 to indicate that we need to keep trying
            if (path.isEmpty())
                break;

            images.append(img);
        } else {
            bool found = false;
            for (int j=0; j<images.size(); ++j) {
                if (images.at(j) == img) {
                    found = true;
                    break;
                }
            }
            if (!found)
                images.append(img);
        }
    }

    return images.size();
}

bool QTextureGlyphCache::populate(QFontEngine *fontEngine, int numGlyphs, const glyph_t *glyphs,
                                                const QFixedPoint *positions)
{
#ifdef CACHE_DEBUG
    printf("Populating with %d glyphs\n", numGlyphs);
    qDebug() << " -> current transformation: " << m_transform;
#endif

    m_current_fontengine = fontEngine;
    const int padding = glyphPadding();
    const int paddingDoubled = padding * 2;

    bool supportsSubPixelPositions = fontEngine->supportsSubPixelPositions();
    if (fontEngine->m_subPixelPositionCount == 0) {
        if (!supportsSubPixelPositions) {
            fontEngine->m_subPixelPositionCount = 1;
        } else {
            int i = 0;
            while (fontEngine->m_subPixelPositionCount == 0 && i < numGlyphs)
                fontEngine->m_subPixelPositionCount = calculateSubPixelPositionCount(glyphs[i++]);
        }
    }

    if (m_cx == 0 && m_cy == 0) {
        m_cx = padding;
        m_cy = padding;
    }

    QHash<GlyphAndSubPixelPosition, Coord> listItemCoordinates;
    int rowHeight = 0;

    // check each glyph for its metrics and get the required rowHeight.
    for (int i=0; i < numGlyphs; ++i) {
        const glyph_t glyph = glyphs[i];

        QFixed subPixelPosition;
        if (supportsSubPixelPositions) {
            QFixed x = positions != 0 ? positions[i].x : QFixed();
            subPixelPosition = fontEngine->subPixelPositionForX(x);
        }

        if (coords.contains(GlyphAndSubPixelPosition(glyph, subPixelPosition)))
            continue;
        if (listItemCoordinates.contains(GlyphAndSubPixelPosition(glyph, subPixelPosition)))
            continue;

        // This is a rather crude hack, but it works.
        // The FreeType font engine is not capable of getting precise metrics for the alphamap
        // without first rasterizing the glyph. If we force the glyph to be rasterized before
        // we ask for the alphaMapBoundingBox(), the glyph will be loaded, rasterized and its
        // proper metrics will be cached and used later.
        if (fontEngine->hasInternalCaching()) {
            QImage *locked = fontEngine->lockedAlphaMapForGlyph(glyph, subPixelPosition, m_format);
            if (locked && !locked->isNull())
                fontEngine->unlockAlphaMapForGlyph();
        }

        glyph_metrics_t metrics = fontEngine->alphaMapBoundingBox(glyph, subPixelPosition, m_transform, m_format);

#ifdef CACHE_DEBUG
        printf("(%4x): w=%.2f, h=%.2f, xoff=%.2f, yoff=%.2f, x=%.2f, y=%.2f\n",
               glyph,
               metrics.width.toReal(),
               metrics.height.toReal(),
               metrics.xoff.toReal(),
               metrics.yoff.toReal(),
               metrics.x.toReal(),
               metrics.y.toReal());
#endif
        GlyphAndSubPixelPosition key(glyph, subPixelPosition);
        int glyph_width = metrics.width.ceil().toInt();
        int glyph_height = metrics.height.ceil().toInt();
        if (glyph_height == 0 || glyph_width == 0) {
            // Avoid multiple calls to boundingBox() for non-printable characters
            Coord c = { 0, 0, 0, 0, 0, 0 };
            coords.insert(key, c);
            continue;
        }
        // align to 8-bit boundary
        if (m_format == QFontEngine::Format_Mono)
            glyph_width = (glyph_width+7)&~7;

        Coord c = { 0, 0, // will be filled in later
                    glyph_width,
                    glyph_height, // texture coords
                    metrics.x.truncate(),
                    -metrics.y.truncate() }; // baseline for horizontal scripts

        listItemCoordinates.insert(key, c);
        rowHeight = qMax(rowHeight, glyph_height);
    }
    if (listItemCoordinates.isEmpty())
        return true;

    rowHeight += paddingDoubled;

    if (m_w == 0) {
        if (fontEngine->maxCharWidth() <= QT_DEFAULT_TEXTURE_GLYPH_CACHE_WIDTH)
            m_w = QT_DEFAULT_TEXTURE_GLYPH_CACHE_WIDTH;
        else
            m_w = qt_next_power_of_two(fontEngine->maxCharWidth());
    }

    // now actually use the coords and paint the wanted glyps into cache.
    QHash<GlyphAndSubPixelPosition, Coord>::iterator iter = listItemCoordinates.begin();
    int requiredWidth = m_w;
    while (iter != listItemCoordinates.end()) {
        Coord c = iter.value();

        m_currentRowHeight = qMax(m_currentRowHeight, c.h);

        if (m_cx + c.w + padding > requiredWidth) {
            int new_width = requiredWidth*2;
            while (new_width < m_cx + c.w + padding)
                new_width *= 2;
            if (new_width <= maxTextureWidth()) {
                requiredWidth = new_width;
            } else {
                // no room on the current line, start new glyph strip
                m_cx = padding;
                m_cy += m_currentRowHeight + paddingDoubled;
                m_currentRowHeight = c.h; // New row
            }
        }

        if (maxTextureHeight() > 0 && m_cy + c.h + padding > maxTextureHeight()) {
            // We can't make a cache of the required size, so we bail out
            return false;
        }

        c.x = m_cx;
        c.y = m_cy;

        coords.insert(iter.key(), c);
        m_pendingGlyphs.insert(iter.key(), c);

        m_cx += c.w + paddingDoubled;
        ++iter;
    }
    return true;

}

void QTextureGlyphCache::fillInPendingGlyphs()
{
    if (m_pendingGlyphs.isEmpty())
        return;

    int requiredHeight = m_h;
    int requiredWidth = m_w; // Use a minimum size to avoid a lot of initial reallocations
    {
        QHash<GlyphAndSubPixelPosition, Coord>::iterator iter = m_pendingGlyphs.begin();
        while (iter != m_pendingGlyphs.end()) {
            Coord c = iter.value();
            requiredHeight = qMax(requiredHeight, c.y + c.h);
            requiredWidth = qMax(requiredWidth, c.x + c.w);
            ++iter;
        }
    }

    if (isNull() || requiredHeight > m_h || requiredWidth > m_w) {
        if (isNull())
            createCache(qt_next_power_of_two(requiredWidth), qt_next_power_of_two(requiredHeight));
        else
            resizeCache(qt_next_power_of_two(requiredWidth), qt_next_power_of_two(requiredHeight));
    }

    {
        QHash<GlyphAndSubPixelPosition, Coord>::iterator iter = m_pendingGlyphs.begin();
        while (iter != m_pendingGlyphs.end()) {
            GlyphAndSubPixelPosition key = iter.key();
            fillTexture(iter.value(), key.glyph, key.subPixelPosition);

            ++iter;
        }
    }

    m_pendingGlyphs.clear();
}

QImage QTextureGlyphCache::textureMapForGlyph(glyph_t g, QFixed subPixelPosition) const
{
    switch (m_format) {
    case QFontEngine::Format_A32:
        return m_current_fontengine->alphaRGBMapForGlyph(g, subPixelPosition, m_transform);
    case QFontEngine::Format_ARGB:
        return m_current_fontengine->bitmapForGlyph(g, subPixelPosition, m_transform);
    default:
        return m_current_fontengine->alphaMapForGlyph(g, subPixelPosition, m_transform);
    }
}

/************************************************************************
 * QImageTextureGlyphCache
 */

void QImageTextureGlyphCache::resizeTextureData(int width, int height)
{
    m_image = m_image.copy(0, 0, width, height);
}

void QImageTextureGlyphCache::createTextureData(int width, int height)
{
    switch (m_format) {
    case QFontEngine::Format_Mono:
        m_image = QImage(width, height, QImage::Format_Mono);
        break;
    case QFontEngine::Format_A8: {
        m_image = QImage(width, height, QImage::Format_Indexed8);
        m_image.fill(0);
        QVector<QRgb> colors(256);
        QRgb *it = colors.data();
        for (int i=0; i<256; ++i, ++it)
            *it = 0xff000000 | i | (i<<8) | (i<<16);
        m_image.setColorTable(colors);
        break;   }
    case QFontEngine::Format_A32:
        m_image = QImage(width, height, QImage::Format_RGB32);
        break;
    case QFontEngine::Format_ARGB:
        m_image = QImage(width, height, QImage::Format_ARGB32_Premultiplied);
        break;
    default:
        Q_UNREACHABLE();
    }
}

void QImageTextureGlyphCache::fillTexture(const Coord &c, glyph_t g, QFixed subPixelPosition)
{
    QImage mask = textureMapForGlyph(g, subPixelPosition);

#ifdef CACHE_DEBUG
    printf("fillTexture of %dx%d at %d,%d in the cache of %dx%d\n", c.w, c.h, c.x, c.y, m_image.width(), m_image.height());
    if (mask.width() > c.w || mask.height() > c.h) {
        printf("   ERROR; mask is bigger than reserved space! %dx%d instead of %dx%d\n", mask.width(), mask.height(), c.w,c.h);
        return;
    }
#endif

    if (m_format == QFontEngine::Format_A32
        || m_format == QFontEngine::Format_ARGB) {
        QImage ref(m_image.bits() + (c.x * 4 + c.y * m_image.bytesPerLine()),
                   qMax(mask.width(), c.w), qMax(mask.height(), c.h), m_image.bytesPerLine(),
                   m_image.format());
        QPainter p(&ref);
        p.setCompositionMode(QPainter::CompositionMode_Source);
        p.fillRect(0, 0, c.w, c.h, QColor(0,0,0,0)); // TODO optimize this
        p.drawImage(0, 0, mask);
        p.end();
    } else if (m_format == QFontEngine::Format_Mono) {
        if (mask.depth() > 1) {
            // TODO optimize this
            mask = mask.alphaChannel();
            mask.invertPixels();
            mask = mask.convertToFormat(QImage::Format_Mono);
        }

        int mw = qMin(mask.width(), c.w);
        int mh = qMin(mask.height(), c.h);
        uchar *d = m_image.bits();
        int dbpl = m_image.bytesPerLine();

        for (int y = 0; y < c.h; ++y) {
            uchar *dest = d + (c.y + y) *dbpl + c.x/8;

            if (y < mh) {
                uchar *src = mask.scanLine(y);
                for (int x = 0; x < c.w/8; ++x) {
                    if (x < (mw+7)/8)
                        dest[x] = src[x];
                    else
                        dest[x] = 0;
                }
            } else {
                for (int x = 0; x < c.w/8; ++x)
                    dest[x] = 0;
            }
        }
    } else { // A8
        int mw = qMin(mask.width(), c.w);
        int mh = qMin(mask.height(), c.h);
        uchar *d = m_image.bits();
        int dbpl = m_image.bytesPerLine();

        if (mask.depth() == 1) {
            for (int y = 0; y < c.h; ++y) {
                uchar *dest = d + (c.y + y) *dbpl + c.x;
                if (y < mh) {
                    uchar *src = (uchar *) mask.scanLine(y);
                    for (int x = 0; x < c.w; ++x) {
                        if (x < mw)
                            dest[x] = (src[x >> 3] & (1 << (7 - (x & 7)))) > 0 ? 255 : 0;
                    }
                }
            }
        } else if (mask.depth() == 8) {
            for (int y = 0; y < c.h; ++y) {
                uchar *dest = d + (c.y + y) *dbpl + c.x;
                if (y < mh) {
                    uchar *src = (uchar *) mask.scanLine(y);
                    for (int x = 0; x < c.w; ++x) {
                        if (x < mw)
                            dest[x] = src[x];
                    }
                }
            }
        }
    }

#ifdef CACHE_DEBUG
//     QPainter p(&m_image);
//     p.drawLine(
    int margin = m_current_fontengine ? m_current_fontengine->glyphMargin(m_format) : 0;
    QPoint base(c.x + margin, c.y + margin + c.baseLineY-1);
    if (m_image.rect().contains(base))
        m_image.setPixel(base, 255);
    m_image.save(QString::fromLatin1("cache-%1.png").arg(qint64(this)));
#endif
}

QT_END_NAMESPACE
