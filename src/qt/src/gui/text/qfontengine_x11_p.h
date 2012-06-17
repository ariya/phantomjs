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

#ifndef QFONTENGINE_X11_P_H
#define QFONTENGINE_X11_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//
#include <private/qt_x11_p.h>

#include <private/qfontengine_ft_p.h>

QT_BEGIN_NAMESPACE

class QFreetypeFace;

// --------------------------------------------------------------------------

class QFontEngineMultiXLFD : public QFontEngineMulti
{
public:
    QFontEngineMultiXLFD(const QFontDef &r, const QList<int> &l, int s);
    ~QFontEngineMultiXLFD();

    void loadEngine(int at);

private:
    QList<int> encodings;
    int screen;
    QFontDef request;
};

/**
 * \internal The font engine for X Logical Font Description (XLFD) fonts, which is for X11 systems without freetype.
 */
class QFontEngineXLFD : public QFontEngine
{
public:
    QFontEngineXLFD(XFontStruct *f, const QByteArray &name, int mib);
    ~QFontEngineXLFD();

    virtual QFontEngine::FaceId faceId() const;
    QFontEngine::Properties properties() const;
    virtual void getUnscaledGlyph(glyph_t glyph, QPainterPath *path, glyph_metrics_t *metrics);
    virtual bool getSfntTableData(uint tag, uchar *buffer, uint *length) const;
    virtual int synthesized() const;

    virtual bool stringToCMap(const QChar *str, int len, QGlyphLayout *glyphs, int *nglyphs,
                      QTextEngine::ShaperFlags flags) const;
    virtual void recalcAdvances(QGlyphLayout *, QTextEngine::ShaperFlags) const;

    virtual glyph_metrics_t boundingBox(const QGlyphLayout &glyphs);
    virtual glyph_metrics_t boundingBox(glyph_t glyph);

    virtual void addOutlineToPath(qreal x, qreal y, const QGlyphLayout &glyphs, QPainterPath *path, QTextItem::RenderFlags);
    virtual QFixed ascent() const;
    virtual QFixed descent() const;
    virtual QFixed leading() const;
    virtual qreal maxCharWidth() const;
    virtual qreal minLeftBearing() const;
    virtual qreal minRightBearing() const;
    virtual QImage alphaMapForGlyph(glyph_t);

    virtual inline Type type() const
    { return QFontEngine::XLFD; }

    virtual bool canRender(const QChar *string, int len);
    virtual const char *name() const;

    inline XFontStruct *fontStruct() const
    { return _fs; }

#ifndef QT_NO_FREETYPE
    FT_Face non_locked_face() const;
    glyph_t glyphIndexToFreetypeGlyphIndex(glyph_t g) const;
#endif
    uint toUnicode(glyph_t g) const;

private:
    QBitmap bitmapForGlyphs(const QGlyphLayout &glyphs, const glyph_metrics_t &metrics, QTextItem::RenderFlags flags = 0);

    XFontStruct *_fs;
    QByteArray _name;
    QTextCodec *_codec;
    int _cmap;
    int lbearing, rbearing;
    mutable QFontEngine::FaceId face_id;
    mutable QFreetypeFace *freetype;
    mutable int synth;
};

#ifndef QT_NO_FONTCONFIG

class Q_GUI_EXPORT QFontEngineMultiFT : public QFontEngineMulti
{
public:
    QFontEngineMultiFT(QFontEngine *fe, FcPattern *firstEnginePattern, FcPattern *p, int s, const QFontDef &request);
    ~QFontEngineMultiFT();

    void loadEngine(int at);

private:
    QFontDef request;
    FcPattern *pattern;
    FcPattern *firstEnginePattern;
    FcFontSet *fontSet;
    int screen;
    int firstFontIndex; // first font in fontset
};

class Q_GUI_EXPORT QFontEngineX11FT : public QFontEngineFT
{
public:
    explicit QFontEngineX11FT(const QFontDef &fontDef) : QFontEngineFT(fontDef) {}
    explicit QFontEngineX11FT(FcPattern *pattern, const QFontDef &fd, int screen);
    ~QFontEngineX11FT();

    QFontEngine *cloneWithSize(qreal pixelSize) const;

#ifndef QT_NO_XRENDER
    int xglyph_format;
#endif

protected:
    virtual bool uploadGlyphToServer(QGlyphSet *set, uint glyphid, Glyph *g, GlyphInfo *info, int glyphDataSize) const;
    virtual unsigned long allocateServerGlyphSet();
    virtual void freeServerGlyphSet(unsigned long id);
};

#endif // QT_NO_FONTCONFIG

QT_END_NAMESPACE

#endif // QFONTENGINE_X11_P_H
