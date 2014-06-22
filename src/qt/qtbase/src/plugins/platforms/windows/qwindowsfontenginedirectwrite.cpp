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

#ifndef QT_NO_DIRECTWRITE

#if _WIN32_WINNT < 0x0600
#undef _WIN32_WINNT
#define _WIN32_WINNT 0x0600
#endif

#include "qwindowsfontenginedirectwrite.h"
#include "qwindowsfontdatabase.h"
#include "qwindowscontext.h"

#include <QtCore/QSettings>
#include <QtCore/QtEndian>
#include <QtCore/QVarLengthArray>
#include <private/qstringiterator_p.h>

#include <dwrite.h>
#include <d2d1.h>

QT_BEGIN_NAMESPACE

// Convert from design units to logical pixels
#define DESIGN_TO_LOGICAL(DESIGN_UNIT_VALUE) \
    QFixed::fromReal((qreal(DESIGN_UNIT_VALUE) / qreal(m_unitsPerEm)) * fontDef.pixelSize)

namespace {

    class GeometrySink: public IDWriteGeometrySink
    {
    public:
        GeometrySink(QPainterPath *path) : m_path(path), m_refCount(0)
        {
            Q_ASSERT(m_path != 0);
        }

        IFACEMETHOD_(void, AddBeziers)(const D2D1_BEZIER_SEGMENT *beziers, UINT bezierCount);
        IFACEMETHOD_(void, AddLines)(const D2D1_POINT_2F *points, UINT pointCount);
        IFACEMETHOD_(void, BeginFigure)(D2D1_POINT_2F startPoint, D2D1_FIGURE_BEGIN figureBegin);
        IFACEMETHOD(Close)();
        IFACEMETHOD_(void, EndFigure)(D2D1_FIGURE_END figureEnd);
        IFACEMETHOD_(void, SetFillMode)(D2D1_FILL_MODE fillMode);
        IFACEMETHOD_(void, SetSegmentFlags)(D2D1_PATH_SEGMENT vertexFlags);

        IFACEMETHOD_(unsigned long, AddRef)();
        IFACEMETHOD_(unsigned long, Release)();
        IFACEMETHOD(QueryInterface)(IID const &riid, void **ppvObject);

    private:
        inline static QPointF fromD2D1_POINT_2F(const D2D1_POINT_2F &inp)
        {
            return QPointF(inp.x, inp.y);
        }

        unsigned long m_refCount;
        QPointF m_startPoint;
        QPainterPath *m_path;
    };

    void GeometrySink::AddBeziers(const D2D1_BEZIER_SEGMENT *beziers,
                                  UINT bezierCount)
    {
        for (uint i=0; i<bezierCount; ++i) {
            QPointF c1 = fromD2D1_POINT_2F(beziers[i].point1);
            QPointF c2 = fromD2D1_POINT_2F(beziers[i].point2);
            QPointF p2 = fromD2D1_POINT_2F(beziers[i].point3);

            m_path->cubicTo(c1, c2, p2);
        }
    }

    void GeometrySink::AddLines(const D2D1_POINT_2F *points, UINT pointsCount)
    {
        for (uint i=0; i<pointsCount; ++i)
            m_path->lineTo(fromD2D1_POINT_2F(points[i]));
    }

    void GeometrySink::BeginFigure(D2D1_POINT_2F startPoint,
                                   D2D1_FIGURE_BEGIN /*figureBegin*/)
    {
        m_startPoint = fromD2D1_POINT_2F(startPoint);
        m_path->moveTo(m_startPoint);
    }

    IFACEMETHODIMP GeometrySink::Close()
    {
        return E_NOTIMPL;
    }

    void GeometrySink::EndFigure(D2D1_FIGURE_END figureEnd)
    {
        if (figureEnd == D2D1_FIGURE_END_CLOSED)
            m_path->closeSubpath();
    }

    void GeometrySink::SetFillMode(D2D1_FILL_MODE fillMode)
    {
        m_path->setFillRule(fillMode == D2D1_FILL_MODE_ALTERNATE
                            ? Qt::OddEvenFill
                            : Qt::WindingFill);
    }

    void GeometrySink::SetSegmentFlags(D2D1_PATH_SEGMENT /*vertexFlags*/)
    {
        /* Not implemented */
    }

    IFACEMETHODIMP_(unsigned long) GeometrySink::AddRef()
    {
        return InterlockedIncrement(&m_refCount);
    }

    IFACEMETHODIMP_(unsigned long) GeometrySink::Release()
    {
        unsigned long newCount = InterlockedDecrement(&m_refCount);
        if (newCount == 0)
        {
            delete this;
            return 0;
        }

        return newCount;
    }

    IFACEMETHODIMP GeometrySink::QueryInterface(IID const &riid, void **ppvObject)
    {
        if (__uuidof(IDWriteGeometrySink) == riid) {
            *ppvObject = this;
        } else if (__uuidof(IUnknown) == riid) {
            *ppvObject = this;
        } else {
            *ppvObject = NULL;
            return E_FAIL;
        }

        AddRef();
        return S_OK;
    }

}

/*!
    \class QWindowsFontEngineDirectWrite
    \brief Windows font engine using Direct Write.
    \internal
    \ingroup qt-lighthouse-win

    Font engine for subpixel positioned text on Windows Vista
    (with platform update) and Windows 7. If selected during
    configuration, the engine will be selected only when the hinting
    preference of a font is set to None or Vertical hinting. The font
    database uses most of the same logic but creates a direct write
    font based on the LOGFONT rather than a GDI handle.

    The engine is currently regarded as experimental, meaning that code
    using it should do substantial testing to make sure it covers their
    use cases.

    Will probably be superseded by a common Free Type font engine in Qt 5.X.
*/

QWindowsFontEngineDirectWrite::QWindowsFontEngineDirectWrite(IDWriteFontFace *directWriteFontFace,
                                               qreal pixelSize,
                                               const QSharedPointer<QWindowsFontEngineData> &d)
    : QFontEngine(DirectWrite)
    , m_fontEngineData(d)
    , m_directWriteFontFace(directWriteFontFace)
    , m_directWriteBitmapRenderTarget(0)
    , m_lineThickness(-1)
    , m_unitsPerEm(-1)
    , m_ascent(-1)
    , m_descent(-1)
    , m_xHeight(-1)
    , m_lineGap(-1)
{
    qCDebug(lcQpaFonts) << __FUNCTION__ << pixelSize;

    Q_ASSERT(m_directWriteFontFace);

    m_fontEngineData->directWriteFactory->AddRef();
    m_directWriteFontFace->AddRef();

    fontDef.pixelSize = pixelSize;
    collectMetrics();
    cache_cost = (m_ascent.toInt() + m_descent.toInt()) * m_xHeight.toInt() * 2000;
}

QWindowsFontEngineDirectWrite::~QWindowsFontEngineDirectWrite()
{
    qCDebug(lcQpaFonts) << __FUNCTION__;

    m_fontEngineData->directWriteFactory->Release();
    m_directWriteFontFace->Release();

    if (m_directWriteBitmapRenderTarget != 0)
        m_directWriteBitmapRenderTarget->Release();
}

void QWindowsFontEngineDirectWrite::collectMetrics()
{
    DWRITE_FONT_METRICS metrics;

    m_directWriteFontFace->GetMetrics(&metrics);
    m_unitsPerEm = metrics.designUnitsPerEm;

    m_lineThickness = DESIGN_TO_LOGICAL(metrics.underlineThickness);
    m_ascent = DESIGN_TO_LOGICAL(metrics.ascent);
    m_descent = DESIGN_TO_LOGICAL(metrics.descent);
    m_xHeight = DESIGN_TO_LOGICAL(metrics.xHeight);
    m_lineGap = DESIGN_TO_LOGICAL(metrics.lineGap);
    m_underlinePosition = DESIGN_TO_LOGICAL(metrics.underlinePosition);
}

QFixed QWindowsFontEngineDirectWrite::underlinePosition() const
{
    if (m_underlinePosition > 0)
        return m_underlinePosition;
    else
        return QFontEngine::underlinePosition();
}

QFixed QWindowsFontEngineDirectWrite::lineThickness() const
{
    if (m_lineThickness > 0)
        return m_lineThickness;
    else
        return QFontEngine::lineThickness();
}

bool QWindowsFontEngineDirectWrite::getSfntTableData(uint tag, uchar *buffer, uint *length) const
{
    bool ret = false;

    const void *tableData = 0;
    UINT32 tableSize;
    void *tableContext = 0;
    BOOL exists;
    HRESULT hr = m_directWriteFontFace->TryGetFontTable(qbswap<quint32>(tag),
                                                        &tableData, &tableSize,
                                                        &tableContext, &exists);
    if (SUCCEEDED(hr)) {
        if (exists) {
            ret = true;
            if (buffer && *length >= tableSize)
                memcpy(buffer, tableData, tableSize);
            *length = tableSize;
            Q_ASSERT(int(*length) > 0);
        }
        m_directWriteFontFace->ReleaseFontTable(tableContext);
    } else {
        qErrnoWarning("%s: TryGetFontTable failed", __FUNCTION__);
    }

    return ret;
}

QFixed QWindowsFontEngineDirectWrite::emSquareSize() const
{
    if (m_unitsPerEm > 0)
        return m_unitsPerEm;
    else
        return QFontEngine::emSquareSize();
}

glyph_t QWindowsFontEngineDirectWrite::glyphIndex(uint ucs4) const
{
    UINT16 glyphIndex;

    HRESULT hr = m_directWriteFontFace->GetGlyphIndicesW(&ucs4, 1, &glyphIndex);
    if (FAILED(hr)) {
        qErrnoWarning("%s: glyphIndex failed", __FUNCTION__);
        glyphIndex = 0;
    }

    return glyphIndex;
}

bool QWindowsFontEngineDirectWrite::stringToCMap(const QChar *str, int len, QGlyphLayout *glyphs,
                                                 int *nglyphs, QFontEngine::ShaperFlags flags) const
{
    Q_ASSERT(glyphs->numGlyphs >= *nglyphs);
    if (*nglyphs < len) {
        *nglyphs = len;
        return false;
    }

    QVarLengthArray<UINT32> codePoints(len);
    int actualLength = 0;
    QStringIterator it(str, str + len);
    while (it.hasNext())
        codePoints[actualLength++] = it.next();

    QVarLengthArray<UINT16> glyphIndices(actualLength);
    HRESULT hr = m_directWriteFontFace->GetGlyphIndicesW(codePoints.data(), actualLength,
                                                         glyphIndices.data());
    if (FAILED(hr)) {
        qErrnoWarning("%s: GetGlyphIndicesW failed", __FUNCTION__);
        return false;
    }

    for (int i = 0; i < actualLength; ++i)
        glyphs->glyphs[i] = glyphIndices.at(i);

    *nglyphs = actualLength;
    glyphs->numGlyphs = actualLength;

    if (!(flags & GlyphIndicesOnly))
        recalcAdvances(glyphs, 0);

    return true;
}

void QWindowsFontEngineDirectWrite::recalcAdvances(QGlyphLayout *glyphs, QFontEngine::ShaperFlags) const
{
    QVarLengthArray<UINT16> glyphIndices(glyphs->numGlyphs);

    // ### Caching?
    for(int i=0; i<glyphs->numGlyphs; i++)
        glyphIndices[i] = UINT16(glyphs->glyphs[i]);

    QVarLengthArray<DWRITE_GLYPH_METRICS> glyphMetrics(glyphIndices.size());
    HRESULT hr = m_directWriteFontFace->GetDesignGlyphMetrics(glyphIndices.data(),
                                                              glyphIndices.size(),
                                                              glyphMetrics.data());
    if (SUCCEEDED(hr)) {
        for (int i = 0; i < glyphs->numGlyphs; ++i)
            glyphs->advances[i] = DESIGN_TO_LOGICAL(glyphMetrics[i].advanceWidth);
        if (fontDef.styleStrategy & QFont::ForceIntegerMetrics) {
            for (int i = 0; i < glyphs->numGlyphs; ++i)
                glyphs->advances[i] = glyphs->advances[i].round();
        }
    } else {
        qErrnoWarning("%s: GetDesignGlyphMetrics failed", __FUNCTION__);
    }
}

void QWindowsFontEngineDirectWrite::addGlyphsToPath(glyph_t *glyphs, QFixedPoint *positions, int nglyphs,
                                             QPainterPath *path, QTextItem::RenderFlags flags)
{
    QVarLengthArray<UINT16> glyphIndices(nglyphs);
    QVarLengthArray<DWRITE_GLYPH_OFFSET> glyphOffsets(nglyphs);
    QVarLengthArray<FLOAT> glyphAdvances(nglyphs);

    for (int i=0; i<nglyphs; ++i) {
        glyphIndices[i] = glyphs[i];
        glyphOffsets[i].advanceOffset  = positions[i].x.toReal();
        glyphOffsets[i].ascenderOffset = -positions[i].y.toReal();
        glyphAdvances[i] = 0.0;
    }

    GeometrySink geometrySink(path);
    HRESULT hr = m_directWriteFontFace->GetGlyphRunOutline(
                fontDef.pixelSize,
                glyphIndices.data(),
                glyphAdvances.data(),
                glyphOffsets.data(),
                nglyphs,
                false,
                flags & QTextItem::RightToLeft,
                &geometrySink
                );

    if (FAILED(hr))
        qErrnoWarning("%s: GetGlyphRunOutline failed", __FUNCTION__);
}

glyph_metrics_t QWindowsFontEngineDirectWrite::boundingBox(const QGlyphLayout &glyphs)
{
    if (glyphs.numGlyphs == 0)
        return glyph_metrics_t();

    bool round = fontDef.styleStrategy & QFont::ForceIntegerMetrics;

    QFixed w = 0;
    for (int i = 0; i < glyphs.numGlyphs; ++i) {
        w += round ? glyphs.effectiveAdvance(i).round() : glyphs.effectiveAdvance(i);

    }

    return glyph_metrics_t(0, -m_ascent, w - lastRightBearing(glyphs), m_ascent + m_descent, w, 0);
}

glyph_metrics_t QWindowsFontEngineDirectWrite::boundingBox(glyph_t g)
{
    UINT16 glyphIndex = g;

    DWRITE_GLYPH_METRICS glyphMetrics;
    HRESULT hr = m_directWriteFontFace->GetDesignGlyphMetrics(&glyphIndex, 1, &glyphMetrics);
    if (SUCCEEDED(hr)) {
        QFixed advanceWidth = DESIGN_TO_LOGICAL(glyphMetrics.advanceWidth);
        QFixed leftSideBearing = DESIGN_TO_LOGICAL(glyphMetrics.leftSideBearing);
        QFixed rightSideBearing = DESIGN_TO_LOGICAL(glyphMetrics.rightSideBearing);
        QFixed advanceHeight = DESIGN_TO_LOGICAL(glyphMetrics.advanceHeight);
        QFixed verticalOriginY = DESIGN_TO_LOGICAL(glyphMetrics.verticalOriginY);
        QFixed topSideBearing = DESIGN_TO_LOGICAL(glyphMetrics.topSideBearing);
        QFixed bottomSideBearing = DESIGN_TO_LOGICAL(glyphMetrics.bottomSideBearing);

        if (fontDef.styleStrategy & QFont::ForceIntegerMetrics) {
            advanceWidth = advanceWidth.round();
            advanceHeight = advanceHeight.round();
        }

        QFixed width = advanceWidth - leftSideBearing - rightSideBearing;
        QFixed height = advanceHeight - topSideBearing - bottomSideBearing;
        return glyph_metrics_t(leftSideBearing,
                               -verticalOriginY + topSideBearing,
                               width,
                               height,
                               advanceWidth,
                               advanceHeight);
    } else {
        qErrnoWarning("%s: GetDesignGlyphMetrics failed", __FUNCTION__);
    }

    return glyph_metrics_t();
}

QFixed QWindowsFontEngineDirectWrite::ascent() const
{
    return fontDef.styleStrategy & QFont::ForceIntegerMetrics
            ? m_ascent.round()
            : m_ascent;
}

QFixed QWindowsFontEngineDirectWrite::descent() const
{
    return fontDef.styleStrategy & QFont::ForceIntegerMetrics
           ? (m_descent - 1).round()
           : (m_descent - 1);
}

QFixed QWindowsFontEngineDirectWrite::leading() const
{
    return fontDef.styleStrategy & QFont::ForceIntegerMetrics
           ? m_lineGap.round()
           : m_lineGap;
}

QFixed QWindowsFontEngineDirectWrite::xHeight() const
{
    return fontDef.styleStrategy & QFont::ForceIntegerMetrics
           ? m_xHeight.round()
           : m_xHeight;
}

qreal QWindowsFontEngineDirectWrite::maxCharWidth() const
{
    // ###
    return 0;
}

QImage QWindowsFontEngineDirectWrite::alphaMapForGlyph(glyph_t glyph, QFixed subPixelPosition)
{
    QImage im = imageForGlyph(glyph, subPixelPosition, 0, QTransform());

    QImage indexed(im.width(), im.height(), QImage::Format_Indexed8);
    QVector<QRgb> colors(256);
    for (int i=0; i<256; ++i)
        colors[i] = qRgba(0, 0, 0, i);
    indexed.setColorTable(colors);

    for (int y=0; y<im.height(); ++y) {
        uint *src = (uint*) im.scanLine(y);
        uchar *dst = indexed.scanLine(y);
        for (int x=0; x<im.width(); ++x) {
            *dst = 255 - (m_fontEngineData->pow_gamma[qGray(0xffffffff - *src)] * 255. / 2047.);
            ++dst;
            ++src;
        }
    }

    return indexed;
}

bool QWindowsFontEngineDirectWrite::supportsSubPixelPositions() const
{
    return true;
}

QImage QWindowsFontEngineDirectWrite::imageForGlyph(glyph_t t,
                                             QFixed subPixelPosition,
                                             int margin,
                                             const QTransform &xform)
{
    glyph_metrics_t metrics = QFontEngine::boundingBox(t, xform);
    int width = (metrics.width + margin * 2 + 4).ceil().toInt() ;
    int height = (metrics.height + margin * 2 + 4).ceil().toInt();

    UINT16 glyphIndex = t;
    FLOAT glyphAdvance = metrics.xoff.toReal();

    DWRITE_GLYPH_OFFSET glyphOffset;
    glyphOffset.advanceOffset = 0;
    glyphOffset.ascenderOffset = 0;

    DWRITE_GLYPH_RUN glyphRun;
    glyphRun.fontFace = m_directWriteFontFace;
    glyphRun.fontEmSize = fontDef.pixelSize;
    glyphRun.glyphCount = 1;
    glyphRun.glyphIndices = &glyphIndex;
    glyphRun.glyphAdvances = &glyphAdvance;
    glyphRun.isSideways = false;
    glyphRun.bidiLevel = 0;
    glyphRun.glyphOffsets = &glyphOffset;

    QFixed x = margin - metrics.x.floor() + subPixelPosition;
    QFixed y = margin - metrics.y.floor();

    DWRITE_MATRIX transform;
    transform.dx = x.toReal();
    transform.dy = y.toReal();
    transform.m11 = xform.m11();
    transform.m12 = xform.m12();
    transform.m21 = xform.m21();
    transform.m22 = xform.m22();

    IDWriteGlyphRunAnalysis *glyphAnalysis = NULL;
    HRESULT hr = m_fontEngineData->directWriteFactory->CreateGlyphRunAnalysis(
                &glyphRun,
                1.0f,
                &transform,
                DWRITE_RENDERING_MODE_CLEARTYPE_NATURAL_SYMMETRIC,
                DWRITE_MEASURING_MODE_NATURAL,
                0.0, 0.0,
                &glyphAnalysis
                );

    if (SUCCEEDED(hr)) {
        RECT rect;
        rect.left = 0;
        rect.top = 0;
        rect.right = width;
        rect.bottom = height;

        int size = width * height * 3;
        BYTE *alphaValues = new BYTE[size];
        memset(alphaValues, size, 0);

        hr = glyphAnalysis->CreateAlphaTexture(DWRITE_TEXTURE_CLEARTYPE_3x1,
                                               &rect,
                                               alphaValues,
                                               size);

        if (SUCCEEDED(hr)) {
            QImage img(width, height, QImage::Format_RGB32);
            img.fill(0xffffffff);

            for (int y=0; y<height; ++y) {
                uint *dest = reinterpret_cast<uint *>(img.scanLine(y));
                BYTE *src = alphaValues + width * 3 * y;

                for (int x=0; x<width; ++x) {
                    dest[x] = *(src) << 16
                            | *(src + 1) << 8
                            | *(src + 2);

                    src += 3;
                }
            }

            delete[] alphaValues;
            glyphAnalysis->Release();

            return img;
        } else {
            delete[] alphaValues;
            glyphAnalysis->Release();

            qErrnoWarning("%s: CreateAlphaTexture failed", __FUNCTION__);
        }

    } else {
        qErrnoWarning("%s: CreateGlyphRunAnalysis failed", __FUNCTION__);
    }

    return QImage();
}

QImage QWindowsFontEngineDirectWrite::alphaRGBMapForGlyph(glyph_t t,
                                                          QFixed subPixelPosition,
                                                          const QTransform &xform)
{
    QImage mask = imageForGlyph(t,
                                subPixelPosition,
                                glyphMargin(QFontEngine::Format_A32),
                                xform);

    return mask.depth() == 32
           ? mask
           : mask.convertToFormat(QImage::Format_RGB32);
}

QFontEngine *QWindowsFontEngineDirectWrite::cloneWithSize(qreal pixelSize) const
{
    QFontEngine *fontEngine = new QWindowsFontEngineDirectWrite(m_directWriteFontFace,
                                                                pixelSize, m_fontEngineData);

    fontEngine->fontDef = fontDef;
    fontEngine->fontDef.pixelSize = pixelSize;

    return fontEngine;
}

void QWindowsFontEngineDirectWrite::initFontInfo(const QFontDef &request,
                                                 int dpi, IDWriteFont *font)
{
    fontDef = request;

    IDWriteFontFamily *fontFamily = NULL;
    HRESULT hr = font->GetFontFamily(&fontFamily);

    IDWriteLocalizedStrings *familyNames = NULL;
    if (SUCCEEDED(hr))
        hr = fontFamily->GetFamilyNames(&familyNames);

    UINT32 index = 0;

    if (SUCCEEDED(hr)) {
        BOOL exists = false;

        wchar_t localeName[LOCALE_NAME_MAX_LENGTH];
        int defaultLocaleSuccess = GetUserDefaultLocaleName(localeName, LOCALE_NAME_MAX_LENGTH);
        if (defaultLocaleSuccess)
            hr = familyNames->FindLocaleName(localeName, &index, &exists);

        if (SUCCEEDED(hr) && !exists)
            hr = familyNames->FindLocaleName(L"en-us", &index, &exists);

        if (!exists)
            index = 0;
    }

    // Get the family name.
    if (SUCCEEDED(hr)) {
        UINT32 length = 0;

        hr = familyNames->GetStringLength(index, &length);

        if (SUCCEEDED(hr)) {
            QVarLengthArray<wchar_t, 128> name(length+1);

            hr = familyNames->GetString(index, name.data(), name.size());

            if (SUCCEEDED(hr))
                fontDef.family = QString::fromWCharArray(name.constData());
        }
    }

    if (familyNames != NULL)
        familyNames->Release();
    if (fontFamily)
        fontFamily->Release();

    if (FAILED(hr))
        qErrnoWarning(hr, "initFontInfo: Failed to get family name");

    if (fontDef.pointSize < 0)
        fontDef.pointSize = fontDef.pixelSize * 72. / dpi;
    else if (fontDef.pixelSize == -1)
        fontDef.pixelSize = qRound(fontDef.pointSize * dpi / 72.);
}

QString QWindowsFontEngineDirectWrite::fontNameSubstitute(const QString &familyName)
{
    static const char keyC[] = "HKEY_LOCAL_MACHINE\\Software\\Microsoft\\Windows NT\\CurrentVersion\\"
                               "FontSubstitutes";
    return QSettings(QLatin1String(keyC), QSettings::NativeFormat).value(familyName, familyName).toString();
}

QT_END_NAMESPACE

#endif // QT_NO_DIRECTWRITE
