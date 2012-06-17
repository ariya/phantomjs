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

#ifndef QFONTENGINE_S60_P_H
#define QFONTENGINE_S60_P_H

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

#include "qconfig.h"
#include <private/qfontengine_p.h>
#include "qsize.h"
#include <openfont.h>

// The glyph outline code is intentionally disabled. It will be reactivated as
// soon as the glyph outline API is backported from Symbian(^4) to Symbian(^3).
#if 0
#define Q_SYMBIAN_HAS_GLYPHOUTLINE_API
#endif

class CFont;

QT_BEGIN_NAMESPACE

// ..gives us access to truetype tables
class QSymbianTypeFaceExtras
{
public:
    QSymbianTypeFaceExtras(CFont* cFont, COpenFont *openFont = 0);
    ~QSymbianTypeFaceExtras();

    QByteArray getSfntTable(uint tag) const;
    bool getSfntTableData(uint tag, uchar *buffer, uint *length) const;
    const uchar *cmap() const;
    CFont *fontOwner() const;
    bool isSymbolCMap() const;
    QFixed unitsPerEm() const;
    static bool symbianFontTableApiAvailable();

private:
    CFont* m_cFont;
    mutable bool m_symbolCMap;
    mutable QByteArray m_cmapTable;
    mutable QFixed m_unitsPerEm;

    // m_openFont and m_openFont are used if Symbian does not provide
    // the Font Table API
    COpenFont *m_openFont;
    mutable MOpenFontTrueTypeExtension *m_trueTypeExtension;
};

class QFontEngineS60 : public QFontEngine
{
public:
    QFontEngineS60(const QFontDef &fontDef, const QSymbianTypeFaceExtras *extras);
    ~QFontEngineS60();

    QFixed emSquareSize() const;
    bool stringToCMap(const QChar *str, int len, QGlyphLayout *glyphs, int *nglyphs, QTextEngine::ShaperFlags flags) const;
    void recalcAdvances(QGlyphLayout *glyphs, QTextEngine::ShaperFlags flags) const;

    void addGlyphsToPath(glyph_t *glyphs, QFixedPoint *positions, int nglyphs,
                         QPainterPath *path, QTextItem::RenderFlags flags);

    QImage alphaMapForGlyph(glyph_t glyph);

    glyph_metrics_t boundingBox(const QGlyphLayout &glyphs);
    glyph_metrics_t boundingBox_const(glyph_t glyph) const; // Const correctnes quirk.
    glyph_metrics_t boundingBox(glyph_t glyph);

    QFixed ascent() const;
    QFixed descent() const;
    QFixed leading() const;
    qreal maxCharWidth() const;
    qreal minLeftBearing() const { return 0; }
    qreal minRightBearing() const { return 0; }

    QByteArray getSfntTable(uint tag) const;
    bool getSfntTableData(uint tag, uchar *buffer, uint *length) const;

    static qreal pixelsToPoints(qreal pixels, Qt::Orientation orientation = Qt::Horizontal);
    static qreal pointsToPixels(qreal points, Qt::Orientation orientation = Qt::Horizontal);

    const char *name() const;

    bool canRender(const QChar *string, int len);

    Type type() const;

    void getCharacterData(glyph_t glyph, TOpenFontCharMetrics& metrics, const TUint8*& bitmap, TSize& bitmapSize) const;
    void setFontScale(qreal scale);

private:
    friend class QFontPrivate;
    friend class QSymbianVGFontGlyphCache;

    QFixed glyphAdvance(HB_Glyph glyph) const;
    CFont *fontWithSize(qreal size) const;
    static void releaseFont(CFont *&font);

    const QSymbianTypeFaceExtras *m_extras;
    CFont* m_originalFont;
    const qreal m_originalFontSizeInPixels;
    CFont* m_scaledFont;
    qreal m_scaledFontSizeInPixels;
    CFont* m_activeFont;
};

class QFontEngineMultiS60 : public QFontEngineMulti
{
public:
    QFontEngineMultiS60(QFontEngine *first, int script, const QStringList &fallbackFamilies);
    void loadEngine(int at);

    int m_script;
    QStringList m_fallbackFamilies;
};

QT_END_NAMESPACE

#endif // QFONTENGINE_S60_P_H
