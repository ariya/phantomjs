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

#ifndef QTEXTUREGLYPHCACHE_P_H
#define QTEXTUREGLYPHCACHE_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of other Qt classes.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <qhash.h>
#include <qimage.h>
#include <qobject.h>
#include <qtransform.h>

#include <private/qfontengineglyphcache_p.h>

#ifndef QT_DEFAULT_TEXTURE_GLYPH_CACHE_WIDTH
#define QT_DEFAULT_TEXTURE_GLYPH_CACHE_WIDTH 256
#endif

struct glyph_metrics_t;
typedef unsigned int glyph_t;


QT_BEGIN_NAMESPACE

class QTextItemInt;

class Q_GUI_EXPORT QTextureGlyphCache : public QFontEngineGlyphCache
{
public:
    QTextureGlyphCache(QFontEngine::GlyphFormat format, const QTransform &matrix)
        : QFontEngineGlyphCache(format, matrix), m_current_fontengine(0),
                                               m_w(0), m_h(0), m_cx(0), m_cy(0), m_currentRowHeight(0)
        { }

    virtual ~QTextureGlyphCache() { }

    struct GlyphAndSubPixelPosition
    {
        GlyphAndSubPixelPosition(glyph_t g, QFixed spp) : glyph(g), subPixelPosition(spp) {}

        bool operator==(const GlyphAndSubPixelPosition &other) const
        {
            return glyph == other.glyph && subPixelPosition == other.subPixelPosition;
        }

        glyph_t glyph;
        QFixed subPixelPosition;
    };

    struct Coord {
        int x;
        int y;
        int w;
        int h;

        int baseLineX;
        int baseLineY;

        bool isNull() const
        {
            return w == 0 || h == 0;
        }
    };

    bool populate(QFontEngine *fontEngine, int numGlyphs, const glyph_t *glyphs,
                  const QFixedPoint *positions);
    void fillInPendingGlyphs();

    virtual void createTextureData(int width, int height) = 0;
    virtual void resizeTextureData(int width, int height) = 0;
    virtual int glyphPadding() const { return 0; }

    virtual void fillTexture(const Coord &coord, glyph_t glyph, QFixed subPixelPosition) = 0;

    inline void createCache(int width, int height) {
        m_w = width;
        m_h = height;
        createTextureData(width, height);
    }

    inline void resizeCache(int width, int height)
    {
        resizeTextureData(width, height);
        m_w = width;
        m_h = height;
    }

    inline bool isNull() const { return m_h == 0; }

    QHash<GlyphAndSubPixelPosition, Coord> coords;
    virtual int maxTextureWidth() const { return QT_DEFAULT_TEXTURE_GLYPH_CACHE_WIDTH; }
    virtual int maxTextureHeight() const { return -1; }

    QImage textureMapForGlyph(glyph_t g, QFixed subPixelPosition) const;

protected:
    int calculateSubPixelPositionCount(glyph_t) const;

    QFontEngine *m_current_fontengine;
    QHash<GlyphAndSubPixelPosition, Coord> m_pendingGlyphs;

    int m_w; // image width
    int m_h; // image height
    int m_cx; // current x
    int m_cy; // current y
    int m_currentRowHeight; // Height of last row
};

inline uint qHash(const QTextureGlyphCache::GlyphAndSubPixelPosition &g)
{
    return (g.glyph << 8)  | (g.subPixelPosition * 10).round().toInt();
}


class Q_GUI_EXPORT QImageTextureGlyphCache : public QTextureGlyphCache
{
public:
    QImageTextureGlyphCache(QFontEngine::GlyphFormat format, const QTransform &matrix)
        : QTextureGlyphCache(format, matrix) { }
    virtual void createTextureData(int width, int height);
    virtual void resizeTextureData(int width, int height);
    virtual void fillTexture(const Coord &c, glyph_t glyph, QFixed subPixelPosition);

    inline const QImage &image() const { return m_image; }

private:
    QImage m_image;
};

QT_END_NAMESPACE

#endif
