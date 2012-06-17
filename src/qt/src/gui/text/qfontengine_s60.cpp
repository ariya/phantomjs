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

#include "qfontengine_s60_p.h"
#include "qtextengine_p.h"
#include "qendian.h"
#include "qglobal.h"
#include <private/qapplication_p.h>
#include "qimage.h"
#include <private/qt_s60_p.h>
#include <private/qpixmap_raster_symbian_p.h>

#include <e32base.h>
#include <e32std.h>
#include <eikenv.h>
#include <gdi.h>
#if defined(Q_SYMBIAN_HAS_GLYPHOUTLINE_API)
#include <graphics/gdi/gdiplatapi.h>
#endif // Q_SYMBIAN_HAS_GLYPHOUTLINE_API

// Replication of TGetFontTableParam & friends.
// There is unfortunately no compile time flag like SYMBIAN_FONT_TABLE_API
// that would help us to only replicate these things for Symbian versions
// that do not yet have the font table Api. Symbian's public SDK does
// generally not define any usable macros.
class QSymbianTGetFontTableParam
{
public:
    TUint32 iTag;
    TAny *iContent;
    TInt iLength;
};
const TUid QSymbianKFontGetFontTable      = {0x102872C1};
const TUid QSymbianKFontReleaseFontTable  = {0x2002AC24};

QT_BEGIN_NAMESPACE

QSymbianTypeFaceExtras::QSymbianTypeFaceExtras(CFont* cFont, COpenFont *openFont)
    : m_cFont(cFont)
    , m_symbolCMap(false)
    , m_openFont(openFont)
{
    if (!symbianFontTableApiAvailable()) {
        TAny *trueTypeExtension = NULL;
        m_openFont->ExtendedInterface(KUidOpenFontTrueTypeExtension, trueTypeExtension);
        m_trueTypeExtension = static_cast<MOpenFontTrueTypeExtension*>(trueTypeExtension);
        Q_ASSERT(m_trueTypeExtension);
    }
}

QSymbianTypeFaceExtras::~QSymbianTypeFaceExtras()
{
    if (symbianFontTableApiAvailable())
        S60->screenDevice()->ReleaseFont(m_cFont);
}

QByteArray QSymbianTypeFaceExtras::getSfntTable(uint tag) const
{
    if (symbianFontTableApiAvailable()) {
        QSymbianTGetFontTableParam fontTableParams = { tag, 0, 0 };
        if (m_cFont->ExtendedFunction(QSymbianKFontGetFontTable, &fontTableParams) == KErrNone) {
            const char* const fontTableContent =
                    static_cast<const char *>(fontTableParams.iContent);
            const QByteArray fontTable(fontTableContent, fontTableParams.iLength);
            m_cFont->ExtendedFunction(QSymbianKFontReleaseFontTable, &fontTableParams);
            return fontTable;
        }
        return QByteArray();
    } else {
        Q_ASSERT(m_trueTypeExtension->HasTrueTypeTable(tag));
        TInt error = KErrNone;
        TInt tableByteLength = 0;
        TAny *table = m_trueTypeExtension->GetTrueTypeTable(error, tag, &tableByteLength);
        Q_CHECK_PTR(table);
        const QByteArray result(static_cast<const char*>(table), tableByteLength);
        m_trueTypeExtension->ReleaseTrueTypeTable(table);
        return result;
    }
}

bool QSymbianTypeFaceExtras::getSfntTableData(uint tag, uchar *buffer, uint *length) const
{
    bool result = true;
    if (symbianFontTableApiAvailable()) {
        QSymbianTGetFontTableParam fontTableParams = { tag, 0, 0 };
        if (m_cFont->ExtendedFunction(QSymbianKFontGetFontTable, &fontTableParams) == KErrNone) {
            if (*length > 0 && *length < fontTableParams.iLength) {
                result = false; // Caller did not allocate enough memory
            } else {
                *length = fontTableParams.iLength;
                if (buffer)
                    memcpy(buffer, fontTableParams.iContent, fontTableParams.iLength);
            }
            m_cFont->ExtendedFunction(QSymbianKFontReleaseFontTable, &fontTableParams);
        } else {
            result = false;
        }
    } else {
        if (!m_trueTypeExtension->HasTrueTypeTable(tag))
            return false;

        TInt error = KErrNone;
        TInt tableByteLength;
        TAny *table = m_trueTypeExtension->GetTrueTypeTable(error, tag, &tableByteLength);
        Q_CHECK_PTR(table);

        if (error != KErrNone) {
            return false;
        } else if (*length > 0 && *length < tableByteLength) {
            result = false; // Caller did not allocate enough memory
        } else {
            *length = tableByteLength;
            if (buffer)
                memcpy(buffer, table, tableByteLength);
        }

        m_trueTypeExtension->ReleaseTrueTypeTable(table);
    }
    return result;
}

const uchar *QSymbianTypeFaceExtras::cmap() const
{
    if (m_cmapTable.isNull()) {
        const QByteArray cmapTable = getSfntTable(MAKE_TAG('c', 'm', 'a', 'p'));
        int size = 0;
        const uchar *cmap = QFontEngine::getCMap(reinterpret_cast<const uchar *>
                (cmapTable.constData()), cmapTable.size(), &m_symbolCMap, &size);
        m_cmapTable = QByteArray(reinterpret_cast<const char *>(cmap), size);
    }
    return reinterpret_cast<const uchar *>(m_cmapTable.constData());
}

bool QSymbianTypeFaceExtras::isSymbolCMap() const
{
    return m_symbolCMap;
}

CFont *QSymbianTypeFaceExtras::fontOwner() const
{
    return m_cFont;
}

QFixed QSymbianTypeFaceExtras::unitsPerEm() const
{
    if (m_unitsPerEm.value() != 0)
        return m_unitsPerEm;
    const QByteArray head = getSfntTable(MAKE_TAG('h', 'e', 'a', 'd'));
    const int unitsPerEmOffset = 18;
    if (head.size() > unitsPerEmOffset + sizeof(quint16)) {
        const uchar* tableData = reinterpret_cast<const uchar*>(head.constData());
        const uchar* unitsPerEm = tableData + unitsPerEmOffset;
        m_unitsPerEm = qFromBigEndian<quint16>(unitsPerEm);
    } else {
        // Bitmap font? Corrupt font?
        // We return -1 and let the QFontEngineS60 return the pixel size.
        m_unitsPerEm = -1;
    }
    return m_unitsPerEm;
}

bool QSymbianTypeFaceExtras::symbianFontTableApiAvailable()
{
    enum FontTableApiAvailability {
        Unknown,
        Available,
        Unavailable
    };
    static FontTableApiAvailability availability =
            QSysInfo::symbianVersion() < QSysInfo::SV_SF_3 ?
                Unavailable : Unknown;
    if (availability == Unknown) {
        // Actually, we should ask CFeatureDiscovery::IsFeatureSupportedL()
        // with FfFontTable here. But since at the time of writing, the
        // FfFontTable flag check either gave false positives or false
        // negatives. Here comes an implicit check via CFont::ExtendedFunction.
        QSymbianTGetFontTableParam fontTableParams = {
            MAKE_TAG('O', 'S', '/', '2'), 0, 0 };
        QSymbianFbsHeapLock lock(QSymbianFbsHeapLock::Unlock);
        CFont *font;
        const TInt getFontErr = S60->screenDevice()->GetNearestFontInTwips(font, TFontSpec());
        Q_ASSERT(getFontErr == KErrNone);
        if (font->ExtendedFunction(QSymbianKFontGetFontTable, &fontTableParams) == KErrNone) {
            font->ExtendedFunction(QSymbianKFontReleaseFontTable, &fontTableParams);
            availability = Available;
        } else {
            availability = Unavailable;
        }
        S60->screenDevice()->ReleaseFont(font);
        lock.relock();
    }
    return availability == Available;
}

// duplicated from qfontengine_xyz.cpp
static inline unsigned int getChar(const QChar *str, int &i, const int len)
{
    uint ucs4 = str[i].unicode();
    if (str[i].isHighSurrogate() && i < len-1 && str[i+1].isLowSurrogate()) {
        ++i;
        ucs4 = QChar::surrogateToUcs4(ucs4, str[i].unicode());
    }
    return ucs4;
}

extern QString qt_symbian_fontNameWithAppFontMarker(const QString &fontName); // qfontdatabase_s60.cpp

CFont *QFontEngineS60::fontWithSize(qreal size) const
{
    CFont *result = 0;
    const QString family = qt_symbian_fontNameWithAppFontMarker(QFontEngine::fontDef.family);
    TFontSpec fontSpec(qt_QString2TPtrC(family), TInt(size));
    fontSpec.iFontStyle.SetBitmapType(EAntiAliasedGlyphBitmap);
    fontSpec.iFontStyle.SetPosture(QFontEngine::fontDef.style == QFont::StyleNormal?EPostureUpright:EPostureItalic);
    fontSpec.iFontStyle.SetStrokeWeight(QFontEngine::fontDef.weight > QFont::Normal?EStrokeWeightBold:EStrokeWeightNormal);
    const TInt errorCode = S60->screenDevice()->GetNearestFontToDesignHeightInPixels(result, fontSpec);
    Q_ASSERT(result && (errorCode == 0));
    return result;
}

void QFontEngineS60::setFontScale(qreal scale)
{
    if (qFuzzyCompare(scale, qreal(1))) {
        if (!m_originalFont)
            m_originalFont = fontWithSize(m_originalFontSizeInPixels);
        m_activeFont = m_originalFont;
    } else {
        const qreal scaledFontSizeInPixels = m_originalFontSizeInPixels * scale;
        if (!m_scaledFont ||
                (TInt(scaledFontSizeInPixels) != TInt(m_scaledFontSizeInPixels))) {
            releaseFont(m_scaledFont);
            m_scaledFontSizeInPixels = scaledFontSizeInPixels;
            m_scaledFont = fontWithSize(m_scaledFontSizeInPixels);
        }
        m_activeFont = m_scaledFont;
    }
}

void QFontEngineS60::releaseFont(CFont *&font)
{
    if (font) {
        S60->screenDevice()->ReleaseFont(font);
        font = 0;
    }
}

QFontEngineS60::QFontEngineS60(const QFontDef &request, const QSymbianTypeFaceExtras *extras)
    : m_extras(extras)
    , m_originalFont(0)
    , m_originalFontSizeInPixels((request.pixelSize >= 0)?
            request.pixelSize:pointsToPixels(request.pointSize))
    , m_scaledFont(0)
    , m_scaledFontSizeInPixels(0)
    , m_activeFont(0)
{
    QFontEngine::fontDef = request;
    setFontScale(1.0);
    cache_cost = sizeof(QFontEngineS60);
}

QFontEngineS60::~QFontEngineS60()
{
    releaseFont(m_originalFont);
    releaseFont(m_scaledFont);
}

QFixed QFontEngineS60::emSquareSize() const
{
    const QFixed unitsPerEm = m_extras->unitsPerEm();
    return unitsPerEm.toInt() == -1 ?
                QFixed::fromReal(m_originalFontSizeInPixels) : unitsPerEm;
}

bool QFontEngineS60::stringToCMap(const QChar *characters, int len, QGlyphLayout *glyphs, int *nglyphs, QTextEngine::ShaperFlags flags) const
{
    if (*nglyphs < len) {
        *nglyphs = len;
        return false;
    }

    HB_Glyph *g = glyphs->glyphs;
    const unsigned char* cmap = m_extras->cmap();
    const bool isRtl = (flags & QTextEngine::RightToLeft);
    for (int i = 0; i < len; ++i) {
        const unsigned int uc = getChar(characters, i, len);
        *g++ = QFontEngine::getTrueTypeGlyphIndex(cmap,
                        (isRtl && !m_extras->isSymbolCMap()) ? QChar::mirroredChar(uc) : uc);
    }

    glyphs->numGlyphs = g - glyphs->glyphs;
    *nglyphs = glyphs->numGlyphs;

    if (flags & QTextEngine::GlyphIndicesOnly)
        return true;

    recalcAdvances(glyphs, flags);
    return true;
}

void QFontEngineS60::recalcAdvances(QGlyphLayout *glyphs, QTextEngine::ShaperFlags flags) const
{
    Q_UNUSED(flags);
    TOpenFontCharMetrics metrics;
    const TUint8 *glyphBitmapBytes;
    TSize glyphBitmapSize;
    for (int i = 0; i < glyphs->numGlyphs; i++) {
        getCharacterData(glyphs->glyphs[i], metrics, glyphBitmapBytes, glyphBitmapSize);
        glyphs->advances_x[i] = metrics.HorizAdvance();
        glyphs->advances_y[i] = 0;
    }
}

#ifdef Q_SYMBIAN_HAS_GLYPHOUTLINE_API
static bool parseGlyphPathData(const char *dataStr, const char *dataEnd, QPainterPath &path,
                               qreal fontPixelSize, const QPointF &offset, bool hinted);
#endif //Q_SYMBIAN_HAS_GLYPHOUTLINE_API

void QFontEngineS60::addGlyphsToPath(glyph_t *glyphs, QFixedPoint *positions,
                                     int nglyphs, QPainterPath *path,
                                     QTextItem::RenderFlags flags)
{
#ifdef Q_SYMBIAN_HAS_GLYPHOUTLINE_API
    Q_UNUSED(flags)
    RGlyphOutlineIterator iterator;
    const TInt error = iterator.Open(*m_activeFont, glyphs, nglyphs);
    if (KErrNone != error)
        return;
    const qreal fontSizeInPixels = qreal(m_activeFont->HeightInPixels());
    int count = 0;
    do {
        const TUint8* outlineUint8 = iterator.Outline();
        const char* const outlineChar = reinterpret_cast<const char*>(outlineUint8);
        const char* const outlineEnd = outlineChar + iterator.OutlineLength();
        parseGlyphPathData(outlineChar, outlineEnd, *path, fontSizeInPixels,
                positions[count++].toPointF(), false);
    } while(KErrNone == iterator.Next() && count <= nglyphs);
    iterator.Close();
#else // Q_SYMBIAN_HAS_GLYPHOUTLINE_API
    QFontEngine::addGlyphsToPath(glyphs, positions, nglyphs, path, flags);
#endif //Q_SYMBIAN_HAS_GLYPHOUTLINE_API
}

QImage QFontEngineS60::alphaMapForGlyph(glyph_t glyph)
{
    // Note: On some Symbian versions (apparently <= Symbian^1), this
    // function will return gray values 0x00, 0x10 ... 0xe0, 0xf0 due
    // to a bug. The glyphs are nowhere perfectly opaque.
    // This has been fixed for Symbian^3.

    TOpenFontCharMetrics metrics;
    const TUint8 *glyphBitmapBytes;
    TSize glyphBitmapSize;
    getCharacterData(glyph, metrics, glyphBitmapBytes, glyphBitmapSize);
    QImage result(glyphBitmapBytes, glyphBitmapSize.iWidth, glyphBitmapSize.iHeight, glyphBitmapSize.iWidth, QImage::Format_Indexed8);
    result.setColorTable(grayPalette());
    return result;
}

glyph_metrics_t QFontEngineS60::boundingBox(const QGlyphLayout &glyphs)
{
   if (glyphs.numGlyphs == 0)
        return glyph_metrics_t();

    QFixed w = 0;
    for (int i = 0; i < glyphs.numGlyphs; ++i)
        w += glyphs.effectiveAdvance(i);

    return glyph_metrics_t(0, -ascent(), w - lastRightBearing(glyphs), ascent()+descent()+1, w, 0);
}

glyph_metrics_t QFontEngineS60::boundingBox_const(glyph_t glyph) const
{
    TOpenFontCharMetrics metrics;
    const TUint8 *glyphBitmapBytes;
    TSize glyphBitmapSize;
    getCharacterData(glyph, metrics, glyphBitmapBytes, glyphBitmapSize);
    const glyph_metrics_t result(
        metrics.HorizBearingX(),
        -metrics.HorizBearingY(),
        metrics.Width(),
        metrics.Height(),
        metrics.HorizAdvance(),
        0
    );
    return result;
}

glyph_metrics_t QFontEngineS60::boundingBox(glyph_t glyph)
{
    return boundingBox_const(glyph);
}

QFixed QFontEngineS60::ascent() const
{
    // Workaround for QTBUG-8013
    // Stroke based fonts may return an incorrect FontMaxAscent of 0.
    const QFixed ascent = m_originalFont->FontMaxAscent();
    return (ascent > 0) ? ascent : QFixed::fromReal(m_originalFontSizeInPixels) - descent();
}

QFixed QFontEngineS60::descent() const
{
    return m_originalFont->FontMaxDescent();
}

QFixed QFontEngineS60::leading() const
{
    return 0;
}

qreal QFontEngineS60::maxCharWidth() const
{
    return m_originalFont->MaxCharWidthInPixels();
}

const char *QFontEngineS60::name() const
{
    return "QFontEngineS60";
}

bool QFontEngineS60::canRender(const QChar *string, int len)
{
    const unsigned char *cmap = m_extras->cmap();
    for (int i = 0; i < len; ++i) {
        const unsigned int uc = getChar(string, i, len);
        if (QFontEngine::getTrueTypeGlyphIndex(cmap, uc) == 0)
            return false;
    }
    return true;
}

QByteArray QFontEngineS60::getSfntTable(uint tag) const
{
    return m_extras->getSfntTable(tag);
}

bool QFontEngineS60::getSfntTableData(uint tag, uchar *buffer, uint *length) const
{
    return m_extras->getSfntTableData(tag, buffer, length);
}

QFontEngine::Type QFontEngineS60::type() const
{
    return QFontEngine::S60FontEngine;
}

void QFontEngineS60::getCharacterData(glyph_t glyph, TOpenFontCharMetrics& metrics, const TUint8*& bitmap, TSize& bitmapSize) const
{
    // Setting the most significant bit tells GetCharacterData
    // that 'code' is a Glyph ID, rather than a UTF-16 value
    const TUint specialCode = (TUint)glyph | 0x80000000;

    const CFont::TCharacterDataAvailability availability =
            m_activeFont->GetCharacterData(specialCode, metrics, bitmap, bitmapSize);
    const glyph_t fallbackGlyph = '?';
    if (availability != CFont::EAllCharacterData) {
        const CFont::TCharacterDataAvailability fallbackAvailability =
                m_activeFont->GetCharacterData(fallbackGlyph, metrics, bitmap, bitmapSize);
        Q_ASSERT(fallbackAvailability == CFont::EAllCharacterData);
    }
}

#ifdef Q_SYMBIAN_HAS_GLYPHOUTLINE_API
static inline void skipSpacesAndComma(const char* &str, const char* const strEnd)
{
    while (str <= strEnd && (*str == ' ' || *str == ','))
        ++str;
}

static bool parseGlyphPathData(const char *svgPath, const char *svgPathEnd, QPainterPath &path,
                               qreal fontPixelSize, const QPointF &offset, bool hinted)
{
    Q_UNUSED(hinted)
    QPointF p1, p2, firstSubPathPoint;
    qreal *elementValues[] =
        {&p1.rx(), &p1.ry(), &p2.rx(), &p2.ry()};
    const int unitsPerEm = 2048; // See: http://en.wikipedia.org/wiki/Em_%28typography%29
    const qreal resizeFactor = fontPixelSize / unitsPerEm;

    while (svgPath < svgPathEnd) {
        skipSpacesAndComma(svgPath, svgPathEnd);
        const char pathElem = *svgPath++;
        skipSpacesAndComma(svgPath, svgPathEnd);

        if (pathElem != 'Z') {
            char *endStr = 0;
            int elementValuesCount = 0;
            for (int i = 0; i < 4; ++i) { // 4 = size of elementValues[]
                qreal coordinateValue = strtod(svgPath, &endStr);
                if (svgPath == endStr)
                    break;
                if (i % 2) // Flip vertically
                    coordinateValue = -coordinateValue;
                *elementValues[i] = coordinateValue * resizeFactor;
                elementValuesCount++;
                svgPath = endStr;
                skipSpacesAndComma(svgPath, svgPathEnd);
            }
            p1 += offset;
            if (elementValuesCount == 2)
                p2 = firstSubPathPoint;
            else
                p2 += offset;
        }

        switch (pathElem) {
        case 'M':
            firstSubPathPoint = p1;
            path.moveTo(p1);
            break;
        case 'Z':
            path.closeSubpath();
            break;
        case 'L':
            path.lineTo(p1);
            break;
        case 'Q':
            path.quadTo(p1, p2);
            break;
        default:
            return false;
        }
    }
    return true;
}
#endif // Q_SYMBIAN_HAS_GLYPHOUTLINE_API

QT_END_NAMESPACE
