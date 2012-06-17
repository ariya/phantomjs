/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtGui module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QFONTENGINE_MAC_P_H
#define QFONTENGINE_MAC_P_H

#include <private/qfontengine_p.h>

#ifndef QT_MAC_USE_COCOA
class QFontEngineMacMulti;
class QFontEngineMac : public QFontEngine
{
    friend class QFontEngineMacMulti;
public:
    QFontEngineMac(ATSUStyle baseStyle, ATSUFontID fontID, const QFontDef &def, QFontEngineMacMulti *multiEngine = 0);
    virtual ~QFontEngineMac();

    virtual bool stringToCMap(const QChar *str, int len, QGlyphLayout *glyphs, int *numGlyphs, QTextEngine::ShaperFlags flags) const;
    virtual void recalcAdvances(QGlyphLayout *, QTextEngine::ShaperFlags) const;

    virtual glyph_metrics_t boundingBox(const QGlyphLayout &glyphs);
    virtual glyph_metrics_t boundingBox(glyph_t glyph);

    virtual QFixed ascent() const;
    virtual QFixed descent() const;
    virtual QFixed leading() const;
    virtual QFixed xHeight() const;
    virtual qreal maxCharWidth() const;
    virtual QFixed averageCharWidth() const;

    virtual void addGlyphsToPath(glyph_t *glyphs, QFixedPoint *positions, int numGlyphs,
                                 QPainterPath *path, QTextItem::RenderFlags);

    virtual const char *name() const { return "QFontEngineMac"; }

    virtual bool canRender(const QChar *string, int len);

    virtual int synthesized() const { return synthesisFlags; }

    virtual Type type() const { return QFontEngine::Mac; }

    void draw(CGContextRef ctx, qreal x, qreal y, const QTextItemInt &ti, int paintDeviceHeight);

    virtual FaceId faceId() const;
    virtual QByteArray getSfntTable(uint tag) const;
    virtual Properties properties() const;
    virtual void getUnscaledGlyph(glyph_t glyph, QPainterPath *path, glyph_metrics_t *metrics);
    virtual QImage alphaMapForGlyph(glyph_t);
    virtual QImage alphaRGBMapForGlyph(glyph_t, QFixed subPixelPosition, int margin, const QTransform &t);

private:
    QImage imageForGlyph(glyph_t glyph, int margin, bool colorful);

    ATSUFontID fontID;
    QCFType<CGFontRef> cgFont;
    ATSUStyle style;
    int synthesisFlags;
    mutable QGlyphLayout kashidaGlyph;
    QFontEngineMacMulti *multiEngine;
    mutable const unsigned char *cmap;
    mutable bool symbolCMap;
    mutable QByteArray cmapTable;
    CGAffineTransform transform;
    QFixed m_ascent;
    QFixed m_descent;
    QFixed m_leading;
    qreal m_maxCharWidth;
    QFixed m_xHeight;
    QFixed m_averageCharWidth;
};

class QFontEngineMacMulti : public QFontEngineMulti
{
    friend class QFontEngineMac;
public:
    // internal
    struct ShaperItem
    {
        inline ShaperItem() : string(0), from(0), length(0),
        log_clusters(0), charAttributes(0) {}

        const QChar *string;
        int from;
        int length;
        QGlyphLayout glyphs;
        unsigned short *log_clusters;
        const HB_CharAttributes *charAttributes;
        QTextEngine::ShaperFlags flags;
    };

    QFontEngineMacMulti(const ATSFontFamilyRef &atsFamily, const ATSFontRef &atsFontRef, const QFontDef &fontDef, bool kerning);
    virtual ~QFontEngineMacMulti();

    virtual bool stringToCMap(const QChar *str, int len, QGlyphLayout *glyphs, int *nglyphs, QTextEngine::ShaperFlags flags) const;
    bool stringToCMap(const QChar *str, int len, QGlyphLayout *glyphs, int *nglyphs, QTextEngine::ShaperFlags flags,
                      unsigned short *logClusters, const HB_CharAttributes *charAttributes, QScriptItem *) const;

    virtual void recalcAdvances(QGlyphLayout *, QTextEngine::ShaperFlags) const;
    virtual void doKerning(QGlyphLayout *, QTextEngine::ShaperFlags) const;

    virtual const char *name() const { return "ATSUI"; }

    virtual bool canRender(const QChar *string, int len);

    inline ATSUFontID macFontID() const { return fontID; }

protected:
    virtual void loadEngine(int at);

private:
    inline const QFontEngineMac *engineAt(int i) const
    { return static_cast<const QFontEngineMac *>(engines.at(i)); }

    bool stringToCMapInternal(const QChar *str, int len, QGlyphLayout *glyphs, int *nglyphs, QTextEngine::ShaperFlags flags, ShaperItem *item) const;

    int fontIndexForFontID(ATSUFontID id) const;

    ATSUFontID fontID;
    uint kerning : 1;

    mutable ATSUTextLayout textLayout;
    mutable ATSUStyle style;
    CGAffineTransform transform;
};
#endif //!QT_MAC_USE_COCOA

#endif // QFONTENGINE_MAC_P_H
