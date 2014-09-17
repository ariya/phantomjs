/****************************************************************************
**
** Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
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

#if _WIN32_WINNT < 0x0500
#undef _WIN32_WINNT
#define _WIN32_WINNT 0x0500
#endif

#include "qfontengine_p.h"
#include "qtextengine_p.h"
#include <qglobal.h>
#include "qt_windows.h"
#include <private/qapplication_p.h>

#include <private/qsystemlibrary_p.h>
#include <qpaintdevice.h>
#include <qpainter.h>
#include <limits.h>

#include <qendian.h>
#include <qmath.h>
#include <qthreadstorage.h>

#include <private/qunicodetables_p.h>
#include <qbitmap.h>

#include <private/qpainter_p.h>
#include "qpaintengine.h"
#include "qvarlengtharray.h"
#include <private/qpaintengine_raster_p.h>
#include <private/qnativeimage_p.h>

#if defined(Q_WS_WINCE)
#include "qguifunctions_wince.h"
#endif

//### mingw needed define
#ifndef TT_PRIM_CSPLINE
#define TT_PRIM_CSPLINE 3
#endif

#ifdef MAKE_TAG
#undef MAKE_TAG
#endif
// GetFontData expects the tags in little endian ;(
#define MAKE_TAG(ch1, ch2, ch3, ch4) (\
    (((quint32)(ch4)) << 24) | \
    (((quint32)(ch3)) << 16) | \
    (((quint32)(ch2)) << 8) | \
    ((quint32)(ch1)) \
   )

// common DC for all fonts

QT_BEGIN_NAMESPACE

class QtHDC
{
    HDC _hdc;
public:
    QtHDC()
    {
        HDC displayDC = GetDC(0);
        _hdc = CreateCompatibleDC(displayDC);
        ReleaseDC(0, displayDC);
    }
    ~QtHDC()
    {
        if (_hdc)
            DeleteDC(_hdc);
    }
    HDC hdc() const
    {
        return _hdc;
    }
};

#ifndef QT_NO_THREAD
Q_GLOBAL_STATIC(QThreadStorage<QtHDC *>, local_shared_dc)
HDC shared_dc()
{
    QtHDC *&hdc = local_shared_dc()->localData();
    if (!hdc)
        hdc = new QtHDC;
    return hdc->hdc();
}
#else
HDC shared_dc()
{
    return 0;
}
#endif

#ifndef Q_WS_WINCE
typedef BOOL (WINAPI *PtrGetCharWidthI)(HDC, UINT, UINT, LPWORD, LPINT);
static PtrGetCharWidthI ptrGetCharWidthI = 0;
static bool resolvedGetCharWidthI = false;

static void resolveGetCharWidthI()
{
    if (resolvedGetCharWidthI)
        return;

    QSystemLibrary gdi32(QLatin1String("gdi32"));
    ptrGetCharWidthI = (PtrGetCharWidthI)gdi32.resolve("GetCharWidthI");

    resolvedGetCharWidthI = true;
}
#endif // !defined(Q_WS_WINCE)

// defined in qtextengine_win.cpp
typedef void *SCRIPT_CACHE;
typedef HRESULT (WINAPI *fScriptFreeCache)(SCRIPT_CACHE *);
extern fScriptFreeCache ScriptFreeCache;

static inline quint32 getUInt(unsigned char *p)
{
    quint32 val;
    val = *p++ << 24;
    val |= *p++ << 16;
    val |= *p++ << 8;
    val |= *p;

    return val;
}

static inline quint16 getUShort(unsigned char *p)
{
    quint16 val;
    val = *p++ << 8;
    val |= *p;

    return val;
}

// general font engine

QFixed QFontEngineWin::lineThickness() const
{
    if(lineWidth > 0)
        return lineWidth;

    return QFontEngine::lineThickness();
}

static OUTLINETEXTMETRIC *getOutlineTextMetric(HDC hdc)
{
    int size;
    size = GetOutlineTextMetrics(hdc, 0, 0);
    OUTLINETEXTMETRIC *otm = (OUTLINETEXTMETRIC *)malloc(size);
    GetOutlineTextMetrics(hdc, size, otm);
    return otm;
}

void QFontEngineWin::getCMap()
{
    ttf = (bool)(tm.tmPitchAndFamily & TMPF_TRUETYPE);
    HDC hdc = shared_dc();
    SelectObject(hdc, hfont);
    bool symb = false;
    if (ttf) {
        cmapTable = getSfntTable(qbswap<quint32>(MAKE_TAG('c', 'm', 'a', 'p')));
        int size = 0;
        cmap = QFontEngine::getCMap(reinterpret_cast<const uchar *>(cmapTable.constData()),
                       cmapTable.size(), &symb, &size);
    }
    if (!cmap) {
        ttf = false;
        symb = false;
    }
    symbol = symb;
    designToDevice = 1;
    _faceId.index = 0;
    if(cmap) {
        OUTLINETEXTMETRIC *otm = getOutlineTextMetric(hdc);
        designToDevice = QFixed((int)otm->otmEMSquare)/int(otm->otmTextMetrics.tmHeight);
        unitsPerEm = otm->otmEMSquare;
        x_height = (int)otm->otmsXHeight;
        loadKerningPairs(designToDevice);
        _faceId.filename = QString::fromWCharArray((wchar_t *)((char *)otm + (quintptr)otm->otmpFullName)).toLatin1();
        lineWidth = otm->otmsUnderscoreSize;
        fsType = otm->otmfsType;
        free(otm);
    } else {
        unitsPerEm = tm.tmHeight;
    }
}


inline unsigned int getChar(const QChar *str, int &i, const int len)
{
    uint ucs4 = str[i].unicode();
    if (str[i].isHighSurrogate() && i < len-1 && str[i+1].isLowSurrogate()) {
        ++i;
        ucs4 = QChar::surrogateToUcs4(ucs4, str[i].unicode());
    }
    return ucs4;
}

int QFontEngineWin::getGlyphIndexes(const QChar *str, int numChars, QGlyphLayout *glyphs, bool mirrored) const
{
    int i = 0;
    int glyph_pos = 0;
    if (mirrored) {
#if defined(Q_WS_WINCE)
        {
#else
        if (symbol) {
            for (; i < numChars; ++i, ++glyph_pos) {
                unsigned int uc = getChar(str, i, numChars);
                glyphs->glyphs[glyph_pos] = getTrueTypeGlyphIndex(cmap, uc);
                if (!glyphs->glyphs[glyph_pos] && uc < 0x100)
                    glyphs->glyphs[glyph_pos] = getTrueTypeGlyphIndex(cmap, uc + 0xf000);
            }
        } else if (ttf) {
            for (; i < numChars; ++i, ++glyph_pos) {
                unsigned int uc = getChar(str, i, numChars);
                glyphs->glyphs[glyph_pos] = getTrueTypeGlyphIndex(cmap, QChar::mirroredChar(uc));
            }
        } else {
#endif
            wchar_t first = tm.tmFirstChar;
            wchar_t last = tm.tmLastChar;

            for (; i < numChars; ++i, ++glyph_pos) {
                uint ucs = QChar::mirroredChar(getChar(str, i, numChars));
                if (
#ifdef Q_WS_WINCE
                    tm.tmFirstChar > 60000 || // see line 375
#endif
                        ucs >= first && ucs <= last)
                    glyphs->glyphs[glyph_pos] = ucs;
                else
                    glyphs->glyphs[glyph_pos] = 0;
            }
        }
    } else {
#if defined(Q_WS_WINCE)
        {
#else
        if (symbol) {
            for (; i < numChars; ++i, ++glyph_pos) {
                unsigned int uc = getChar(str, i, numChars);
                glyphs->glyphs[i] = getTrueTypeGlyphIndex(cmap, uc);
                if(!glyphs->glyphs[glyph_pos] && uc < 0x100)
                    glyphs->glyphs[glyph_pos] = getTrueTypeGlyphIndex(cmap, uc + 0xf000);
            }
        } else if (ttf) {
            for (; i < numChars; ++i, ++glyph_pos) {
                unsigned int uc = getChar(str, i, numChars);
                glyphs->glyphs[glyph_pos] = getTrueTypeGlyphIndex(cmap, uc);
            }
        } else {
#endif
            wchar_t first = tm.tmFirstChar;
            wchar_t last = tm.tmLastChar;

            for (; i < numChars; ++i, ++glyph_pos) {
                uint uc = getChar(str, i, numChars);
                if (
#ifdef Q_WS_WINCE
                    tm.tmFirstChar > 60000 || // see comment in QFontEngineWin
#endif
                        uc >= first && uc <= last)
                    glyphs->glyphs[glyph_pos] = uc;
                else
                    glyphs->glyphs[glyph_pos] = 0;
            }
        }
    }
    glyphs->numGlyphs = glyph_pos;
    return glyph_pos;
}


QFontEngineWin::QFontEngineWin(const QString &name, HFONT _hfont, bool stockFont, LOGFONT lf)
{
    //qDebug("regular windows font engine created: font='%s', size=%d", name, lf.lfHeight);

    _name = name;

    cmap = 0;
    hfont = _hfont;
    logfont = lf;
    HDC hdc = shared_dc();
    SelectObject(hdc, hfont);
    this->stockFont = stockFont;
    fontDef.pixelSize = -lf.lfHeight;

    lbearing = SHRT_MIN;
    rbearing = SHRT_MIN;
    synthesized_flags = -1;
    lineWidth = -1;
    x_height = -1;

    BOOL res = GetTextMetrics(hdc, &tm);
    fontDef.fixedPitch = !(tm.tmPitchAndFamily & TMPF_FIXED_PITCH);
    if (!res) {
        qErrnoWarning("QFontEngineWin: GetTextMetrics failed");
        ZeroMemory(&tm, sizeof(TEXTMETRIC));
    }

    cache_cost = tm.tmHeight * tm.tmAveCharWidth * 2000;
    getCMap();

    widthCache = 0;
    widthCacheSize = 0;
    designAdvances = 0;
    designAdvancesSize = 0;

#ifndef Q_WS_WINCE
    if (!resolvedGetCharWidthI)
        resolveGetCharWidthI();
#endif
}

QFontEngineWin::~QFontEngineWin()
{
    if (designAdvances)
        free(designAdvances);

    if (widthCache)
        free(widthCache);

    // make sure we aren't by accident still selected
    SelectObject(shared_dc(), (HFONT)GetStockObject(SYSTEM_FONT));

    if (!stockFont) {
        if (!DeleteObject(hfont))
            qErrnoWarning("QFontEngineWin: failed to delete non-stock font...");
    }
}

HGDIOBJ QFontEngineWin::selectDesignFont() const
{
    LOGFONT f = logfont;
    f.lfHeight = unitsPerEm;
    HFONT designFont = CreateFontIndirect(&f);
    return SelectObject(shared_dc(), designFont);
}

bool QFontEngineWin::stringToCMap(const QChar *str, int len, QGlyphLayout *glyphs, int *nglyphs, QTextEngine::ShaperFlags flags) const
{
    if (*nglyphs < len) {
        *nglyphs = len;
        return false;
    }

    *nglyphs = getGlyphIndexes(str, len, glyphs, flags & QTextEngine::RightToLeft);

    if (flags & QTextEngine::GlyphIndicesOnly)
        return true;

    recalcAdvances(glyphs, flags);
    return true;
}

inline void calculateTTFGlyphWidth(HDC hdc, UINT glyph, int &width)
{
#if defined(Q_WS_WINCE)
    GetCharWidth32(hdc, glyph, glyph, &width);
#else
    if (ptrGetCharWidthI)
        ptrGetCharWidthI(hdc, glyph, 1, 0, &width);
#endif
}

void QFontEngineWin::recalcAdvances(QGlyphLayout *glyphs, QTextEngine::ShaperFlags flags) const
{
    HGDIOBJ oldFont = 0;
    HDC hdc = shared_dc();
    if (ttf && (flags & QTextEngine::DesignMetrics)) {
        for(int i = 0; i < glyphs->numGlyphs; i++) {
            unsigned int glyph = glyphs->glyphs[i];
            if(int(glyph) >= designAdvancesSize) {
                int newSize = (glyph + 256) >> 8 << 8;
                designAdvances = q_check_ptr((QFixed *)realloc(designAdvances,
                            newSize*sizeof(QFixed)));
                for(int i = designAdvancesSize; i < newSize; ++i)
                    designAdvances[i] = -1000000;
                designAdvancesSize = newSize;
            }
            if (designAdvances[glyph] < -999999) {
                if (!oldFont)
                    oldFont = selectDesignFont();

                int width = 0;
                calculateTTFGlyphWidth(hdc, glyph, width);
                designAdvances[glyph] = QFixed(width) / designToDevice;
            }
            glyphs->advances_x[i] = designAdvances[glyph];
            glyphs->advances_y[i] = 0;
        }
        if(oldFont)
            DeleteObject(SelectObject(hdc, oldFont));
    } else {
        for(int i = 0; i < glyphs->numGlyphs; i++) {
            unsigned int glyph = glyphs->glyphs[i];

            glyphs->advances_y[i] = 0;

            if (glyph >= widthCacheSize) {
                int newSize = (glyph + 256) >> 8 << 8;
                widthCache = q_check_ptr((unsigned char *)realloc(widthCache,
                            newSize*sizeof(QFixed)));
                memset(widthCache + widthCacheSize, 0, newSize - widthCacheSize);
                widthCacheSize = newSize;
            }
            glyphs->advances_x[i] = widthCache[glyph];
            // font-width cache failed
            if (glyphs->advances_x[i] == 0) {
                int width = 0;
                if (!oldFont)
                    oldFont = SelectObject(hdc, hfont);

                if (!ttf) {
                    QChar ch[2] = { ushort(glyph), 0 };
                    int chrLen = 1;
                    if (glyph > 0xffff) {
                        ch[0] = QChar::highSurrogate(glyph);
                        ch[1] = QChar::lowSurrogate(glyph);
                        ++chrLen;
                    }
                    SIZE size = {0, 0};
                    GetTextExtentPoint32(hdc, (wchar_t *)ch, chrLen, &size);
                    width = size.cx;
                } else {
                    calculateTTFGlyphWidth(hdc, glyph, width);
                }
                glyphs->advances_x[i] = width;
                // if glyph's within cache range, store it for later
                if (width > 0 && width < 0x100)
                    widthCache[glyph] = width;
            }
        }

        if (oldFont)
            SelectObject(hdc, oldFont);
    }
}

glyph_metrics_t QFontEngineWin::boundingBox(const QGlyphLayout &glyphs)
{
    if (glyphs.numGlyphs == 0)
        return glyph_metrics_t();

    QFixed w = 0;
    for (int i = 0; i < glyphs.numGlyphs; ++i)
        w += glyphs.effectiveAdvance(i);

    return glyph_metrics_t(0, -tm.tmAscent, w - lastRightBearing(glyphs), tm.tmHeight, w, 0);
}

#ifndef Q_WS_WINCE
bool QFontEngineWin::getOutlineMetrics(glyph_t glyph, const QTransform &t, glyph_metrics_t *metrics) const
{
    Q_ASSERT(metrics != 0);

    HDC hdc = shared_dc();

    GLYPHMETRICS gm;
    DWORD res = 0;
    MAT2 mat;
    mat.eM11.value = mat.eM22.value = 1;
    mat.eM11.fract = mat.eM22.fract = 0;
    mat.eM21.value = mat.eM12.value = 0;
    mat.eM21.fract = mat.eM12.fract = 0;

    if (t.type() > QTransform::TxTranslate) {
        // We need to set the transform using the HDC's world
        // matrix rather than using the MAT2 above, because the
        // results provided when transforming via MAT2 does not
        // match the glyphs that are drawn using a WorldTransform
        XFORM xform;
        xform.eM11 = t.m11();
        xform.eM12 = t.m12();
        xform.eM21 = t.m21();
        xform.eM22 = t.m22();
        xform.eDx = 0;
        xform.eDy = 0;
        SetGraphicsMode(hdc, GM_ADVANCED);
        SetWorldTransform(hdc, &xform);
    }

    uint format = GGO_METRICS;
    if (ttf)
        format |= GGO_GLYPH_INDEX;
    res = GetGlyphOutline(hdc, glyph, format, &gm, 0, 0, &mat);

    if (t.type() > QTransform::TxTranslate) {
        XFORM xform;
        xform.eM11 = xform.eM22 = 1;
        xform.eM12 = xform.eM21 = xform.eDx = xform.eDy = 0;
        SetWorldTransform(hdc, &xform);
        SetGraphicsMode(hdc, GM_COMPATIBLE);
    }

    if (res != GDI_ERROR) {
        *metrics = glyph_metrics_t(gm.gmptGlyphOrigin.x, -gm.gmptGlyphOrigin.y,
                                  (int)gm.gmBlackBoxX, (int)gm.gmBlackBoxY, gm.gmCellIncX, gm.gmCellIncY);
        return true;
    } else {
        return false;
    }
}
#endif

glyph_metrics_t QFontEngineWin::boundingBox(glyph_t glyph, const QTransform &t)
{
#ifndef Q_WS_WINCE
    HDC hdc = shared_dc();
    SelectObject(hdc, hfont);

    glyph_metrics_t glyphMetrics;
    bool success = getOutlineMetrics(glyph, t, &glyphMetrics);

    if (!ttf && !success) {
        // Bitmap fonts
        wchar_t ch = glyph;
        ABCFLOAT abc;
        GetCharABCWidthsFloat(hdc, ch, ch, &abc);
        int width = qRound(abc.abcfB);

        return glyph_metrics_t(QFixed::fromReal(abc.abcfA), -tm.tmAscent, width, tm.tmHeight, width, 0).transformed(t);
    }

    return glyphMetrics;
#else
    HDC hdc = shared_dc();
    HGDIOBJ oldFont = SelectObject(hdc, hfont);

    ABC abc;
    int width;
    int advance;
#ifdef GWES_MGTT    // true type fonts
    if (GetCharABCWidths(hdc, glyph, glyph, &abc)) {
        width = qAbs(abc.abcA) + abc.abcB + qAbs(abc.abcC);
        advance = abc.abcA + abc.abcB + abc.abcC;
    }
    else
#endif
#if defined(GWES_MGRAST) || defined(GWES_MGRAST2)   // raster fonts
    if (GetCharWidth32(hdc, glyph, glyph, &width)) {
        advance = width;
    }
    else
#endif
    {   // fallback
        width = tm.tmMaxCharWidth;
        advance = width;
    }

    SelectObject(hdc, oldFont);
    return glyph_metrics_t(0, -tm.tmAscent, width, tm.tmHeight, advance, 0).transformed(t);
#endif
}

QFixed QFontEngineWin::ascent() const
{
    return tm.tmAscent;
}

QFixed QFontEngineWin::descent() const
{
    // ### we substract 1 to even out the historical +1 in QFontMetrics's
    // ### height=asc+desc+1 equation. Fix in Qt5.
    return tm.tmDescent - 1;
}

QFixed QFontEngineWin::leading() const
{
    return tm.tmExternalLeading;
}


QFixed QFontEngineWin::xHeight() const
{
    if(x_height >= 0)
        return x_height;
    return QFontEngine::xHeight();
}

QFixed QFontEngineWin::averageCharWidth() const
{
    return tm.tmAveCharWidth;
}

qreal QFontEngineWin::maxCharWidth() const
{
    return tm.tmMaxCharWidth;
}

enum { max_font_count = 256 };
static const ushort char_table[] = {
        40,
        67,
        70,
        75,
        86,
        88,
        89,
        91,
        102,
        114,
        124,
        127,
        205,
        645,
        884,
        922,
        1070,
        12386,
        0
};

static const int char_table_entries = sizeof(char_table)/sizeof(ushort);

#ifndef Q_CC_MINGW
void QFontEngineWin::getGlyphBearings(glyph_t glyph, qreal *leftBearing, qreal *rightBearing)
{
    HDC hdc = shared_dc();
    SelectObject(hdc, hfont);

#ifndef Q_WS_WINCE
    if (ttf)
#endif

    {
        ABC abcWidths;
        GetCharABCWidthsI(hdc, glyph, 1, 0, &abcWidths);
        if (leftBearing)
            *leftBearing = abcWidths.abcA;
        if (rightBearing)
            *rightBearing = abcWidths.abcC;
    }

#ifndef Q_WS_WINCE
    else {
        QFontEngine::getGlyphBearings(glyph, leftBearing, rightBearing);
    }
#endif
}
#endif // Q_CC_MINGW

qreal QFontEngineWin::minLeftBearing() const
{
    if (lbearing == SHRT_MIN)
        minRightBearing(); // calculates both

    return lbearing;
}

qreal QFontEngineWin::minRightBearing() const
{
#ifdef Q_WS_WINCE
    if (rbearing == SHRT_MIN) {
        int ml = 0;
        int mr = 0;
        HDC hdc = shared_dc();
        SelectObject(hdc, hfont);
        if (ttf) {
            ABC *abc = 0;
            int n = tm.tmLastChar - tm.tmFirstChar;
            if (n <= max_font_count) {
                abc = new ABC[n+1];
                GetCharABCWidths(hdc, tm.tmFirstChar, tm.tmLastChar, abc);
            } else {
                abc = new ABC[char_table_entries+1];
                for(int i = 0; i < char_table_entries; i++)
                    GetCharABCWidths(hdc, char_table[i], char_table[i], abc+i);
                n = char_table_entries;
            }
            ml = abc[0].abcA;
            mr = abc[0].abcC;
            for (int i = 1; i < n; i++) {
                if (abc[i].abcA + abc[i].abcB + abc[i].abcC != 0) {
                    ml = qMin(ml,abc[i].abcA);
                    mr = qMin(mr,abc[i].abcC);
                }
            }
            delete [] abc;
        }
        lbearing = ml;
        rbearing = mr;
    }

    return rbearing;
#else
    if (rbearing == SHRT_MIN) {
        int ml = 0;
        int mr = 0;
        HDC hdc = shared_dc();
        SelectObject(hdc, hfont);
        if (ttf) {
            ABC *abc = 0;
            int n = tm.tmLastChar - tm.tmFirstChar;
            if (n <= max_font_count) {
                abc = new ABC[n+1];
                GetCharABCWidths(hdc, tm.tmFirstChar, tm.tmLastChar, abc);
            } else {
                abc = new ABC[char_table_entries+1];
                for(int i = 0; i < char_table_entries; i++)
                    GetCharABCWidths(hdc, char_table[i], char_table[i], abc + i);
                n = char_table_entries;
            }
            ml = abc[0].abcA;
            mr = abc[0].abcC;
            for (int i = 1; i < n; i++) {
                if (abc[i].abcA + abc[i].abcB + abc[i].abcC != 0) {
                    ml = qMin(ml,abc[i].abcA);
                    mr = qMin(mr,abc[i].abcC);
                }
            }
            delete [] abc;
        } else {
            ABCFLOAT *abc = 0;
            int n = tm.tmLastChar - tm.tmFirstChar+1;
            if (n <= max_font_count) {
                abc = new ABCFLOAT[n];
                GetCharABCWidthsFloat(hdc, tm.tmFirstChar, tm.tmLastChar, abc);
            } else {
                abc = new ABCFLOAT[char_table_entries];
                for(int i = 0; i < char_table_entries; i++)
                    GetCharABCWidthsFloat(hdc, char_table[i], char_table[i], abc+i);
                n = char_table_entries;
            }
            float fml = abc[0].abcfA;
            float fmr = abc[0].abcfC;
            for (int i=1; i<n; i++) {
                if (abc[i].abcfA + abc[i].abcfB + abc[i].abcfC != 0) {
                    fml = qMin(fml,abc[i].abcfA);
                    fmr = qMin(fmr,abc[i].abcfC);
                }
            }
            ml = int(fml - 0.9999);
            mr = int(fmr - 0.9999);
            delete [] abc;
        }
        lbearing = ml;
        rbearing = mr;
    }

    return rbearing;
#endif
}


const char *QFontEngineWin::name() const
{
    return 0;
}

bool QFontEngineWin::canRender(const QChar *string,  int len)
{
    if (symbol) {
        for (int i = 0; i < len; ++i) {
            unsigned int uc = getChar(string, i, len);
            if (getTrueTypeGlyphIndex(cmap, uc) == 0) {
                if (uc < 0x100) {
                    if (getTrueTypeGlyphIndex(cmap, uc + 0xf000) == 0)
                        return false;
                } else {
                    return false;
                }
            }
        }
    } else if (ttf) {
        for (int i = 0; i < len; ++i) {
            unsigned int uc = getChar(string, i, len);
            if (getTrueTypeGlyphIndex(cmap, uc) == 0)
                return false;
        }
    } else {
        while(len--) {
            if (tm.tmFirstChar > string->unicode() || tm.tmLastChar < string->unicode())
                return false;
        }
    }
    return true;
}

QFontEngine::Type QFontEngineWin::type() const
{
    return QFontEngine::Win;
}

static inline double qt_fixed_to_double(const FIXED &p) {
    return ((p.value << 16) + p.fract) / 65536.0;
}

static inline QPointF qt_to_qpointf(const POINTFX &pt, qreal scale) {
    return QPointF(qt_fixed_to_double(pt.x) * scale, -qt_fixed_to_double(pt.y) * scale);
}

#ifndef GGO_UNHINTED
#define GGO_UNHINTED 0x0100
#endif

static bool addGlyphToPath(glyph_t glyph, const QFixedPoint &position, HDC hdc,
                           QPainterPath *path, bool ttf, glyph_metrics_t *metric = 0, qreal scale = 1)
{
#if defined(Q_WS_WINCE)
    Q_UNUSED(glyph);
    Q_UNUSED(hdc);
#endif
    MAT2 mat;
    mat.eM11.value = mat.eM22.value = 1;
    mat.eM11.fract = mat.eM22.fract = 0;
    mat.eM21.value = mat.eM12.value = 0;
    mat.eM21.fract = mat.eM12.fract = 0;
    uint glyphFormat = GGO_NATIVE;

    if (ttf)
        glyphFormat |= GGO_GLYPH_INDEX;

    GLYPHMETRICS gMetric;
    memset(&gMetric, 0, sizeof(GLYPHMETRICS));
    int bufferSize = GDI_ERROR;
#if !defined(Q_WS_WINCE)
    bufferSize = GetGlyphOutline(hdc, glyph, glyphFormat, &gMetric, 0, 0, &mat);
#endif
    if ((DWORD)bufferSize == GDI_ERROR) {
        return false;
    }

    void *dataBuffer = new char[bufferSize];
    DWORD ret = GDI_ERROR;
#if !defined(Q_WS_WINCE)
    ret = GetGlyphOutline(hdc, glyph, glyphFormat, &gMetric, bufferSize, dataBuffer, &mat);
#endif
    if (ret == GDI_ERROR) {
        delete [](char *)dataBuffer;
        return false;
    }

    if(metric) {
        // #### obey scale
        *metric = glyph_metrics_t(gMetric.gmptGlyphOrigin.x, -gMetric.gmptGlyphOrigin.y,
                                  (int)gMetric.gmBlackBoxX, (int)gMetric.gmBlackBoxY,
                                  gMetric.gmCellIncX, gMetric.gmCellIncY);
    }

    int offset = 0;
    int headerOffset = 0;
    TTPOLYGONHEADER *ttph = 0;

    QPointF oset = position.toPointF();
    while (headerOffset < bufferSize) {
        ttph = (TTPOLYGONHEADER*)((char *)dataBuffer + headerOffset);

        QPointF lastPoint(qt_to_qpointf(ttph->pfxStart, scale));
        path->moveTo(lastPoint + oset);
        offset += sizeof(TTPOLYGONHEADER);
        TTPOLYCURVE *curve;
        while (offset<int(headerOffset + ttph->cb)) {
            curve = (TTPOLYCURVE*)((char*)(dataBuffer) + offset);
            switch (curve->wType) {
            case TT_PRIM_LINE: {
                for (int i=0; i<curve->cpfx; ++i) {
                    QPointF p = qt_to_qpointf(curve->apfx[i], scale) + oset;
                    path->lineTo(p);
                }
                break;
            }
            case TT_PRIM_QSPLINE: {
                const QPainterPath::Element &elm = path->elementAt(path->elementCount()-1);
                QPointF prev(elm.x, elm.y);
                QPointF endPoint;
                for (int i=0; i<curve->cpfx - 1; ++i) {
                    QPointF p1 = qt_to_qpointf(curve->apfx[i], scale) + oset;
                    QPointF p2 = qt_to_qpointf(curve->apfx[i+1], scale) + oset;
                    if (i < curve->cpfx - 2) {
                        endPoint = QPointF((p1.x() + p2.x()) / 2, (p1.y() + p2.y()) / 2);
                    } else {
                        endPoint = p2;
                    }

                    path->quadTo(p1, endPoint);
                    prev = endPoint;
                }

                break;
            }
            case TT_PRIM_CSPLINE: {
                for (int i=0; i<curve->cpfx; ) {
                    QPointF p2 = qt_to_qpointf(curve->apfx[i++], scale) + oset;
                    QPointF p3 = qt_to_qpointf(curve->apfx[i++], scale) + oset;
                    QPointF p4 = qt_to_qpointf(curve->apfx[i++], scale) + oset;
                    path->cubicTo(p2, p3, p4);
                }
                break;
            }
            default:
                qWarning("QFontEngineWin::addOutlineToPath, unhandled switch case");
            }
            offset += sizeof(TTPOLYCURVE) + (curve->cpfx-1) * sizeof(POINTFX);
        }
        path->closeSubpath();
        headerOffset += ttph->cb;
    }
    delete [] (char*)dataBuffer;

    return true;
}

void QFontEngineWin::addGlyphsToPath(glyph_t *glyphs, QFixedPoint *positions, int nglyphs,
                                     QPainterPath *path, QTextItem::RenderFlags)
{
    LOGFONT lf = logfont;
    // The sign must be negative here to make sure we match against character height instead of
    // hinted cell height. This ensures that we get linear matching, and we need this for
    // paths since we later on apply a scaling transform to the glyph outline to get the
    // font at the correct pixel size.
    lf.lfHeight = -unitsPerEm;
    lf.lfWidth = 0;
    HFONT hf = CreateFontIndirect(&lf);
    HDC hdc = shared_dc();
    HGDIOBJ oldfont = SelectObject(hdc, hf);

    for(int i = 0; i < nglyphs; ++i) {
        if (!addGlyphToPath(glyphs[i], positions[i], hdc, path, ttf, /*metric*/0,
                            qreal(fontDef.pixelSize) / unitsPerEm)) {
            // Some windows fonts, like "Modern", are vector stroke
            // fonts, which are reported as TMPF_VECTOR but do not
            // support GetGlyphOutline, and thus we set this bit so
            // that addOutLineToPath can check it and return safely...
            hasOutline = false;
            break;
        }
    }
    DeleteObject(SelectObject(hdc, oldfont));
}

void QFontEngineWin::addOutlineToPath(qreal x, qreal y, const QGlyphLayout &glyphs,
                                      QPainterPath *path, QTextItem::RenderFlags flags)
{
#if !defined(Q_WS_WINCE)
    if(tm.tmPitchAndFamily & (TMPF_TRUETYPE | TMPF_VECTOR)) {
        hasOutline = true;
        QFontEngine::addOutlineToPath(x, y, glyphs, path, flags);
        if (hasOutline)  {
            // has_outline is set to false if addGlyphToPath gets
            // false from GetGlyphOutline, meaning its not an outline
            // font.
            return;
        }
    }
#endif
    QFontEngine::addBitmapFontToPath(x, y, glyphs, path, flags);
}

QFontEngine::FaceId QFontEngineWin::faceId() const
{
    return _faceId;
}

QT_BEGIN_INCLUDE_NAMESPACE
#include <qdebug.h>
QT_END_INCLUDE_NAMESPACE

int QFontEngineWin::synthesized() const
{
    if(synthesized_flags == -1) {
        synthesized_flags = 0;
        if(ttf) {
            const DWORD HEAD = MAKE_TAG('h', 'e', 'a', 'd');
            HDC hdc = shared_dc();
            SelectObject(hdc, hfont);
            uchar data[4];
            GetFontData(hdc, HEAD, 44, &data, 4);
            USHORT macStyle = getUShort(data);
            if (tm.tmItalic && !(macStyle & 2))
                synthesized_flags = SynthesizedItalic;
            if (fontDef.stretch != 100 && ttf)
                synthesized_flags |= SynthesizedStretch;
            if (tm.tmWeight >= 500 && !(macStyle & 1))
                synthesized_flags |= SynthesizedBold;
            //qDebug() << "font is" << _name <<
            //    "it=" << (macStyle & 2) << fontDef.style << "flags=" << synthesized_flags;
        }
    }
    return synthesized_flags;
}

QFixed QFontEngineWin::emSquareSize() const
{
    return unitsPerEm;
}

QFontEngine::Properties QFontEngineWin::properties() const
{
    LOGFONT lf = logfont;
    lf.lfHeight = unitsPerEm;
    HFONT hf = CreateFontIndirect(&lf);
    HDC hdc = shared_dc();
    HGDIOBJ oldfont = SelectObject(hdc, hf);
    OUTLINETEXTMETRIC *otm = getOutlineTextMetric(hdc);
    Properties p;
    p.emSquare = unitsPerEm;
    p.italicAngle = otm->otmItalicAngle;
    p.postscriptName = QString::fromWCharArray((wchar_t *)((char *)otm + (quintptr)otm->otmpFamilyName)).toLatin1();
    p.postscriptName += QString::fromWCharArray((wchar_t *)((char *)otm + (quintptr)otm->otmpStyleName)).toLatin1();
    p.postscriptName = QFontEngine::convertToPostscriptFontFamilyName(p.postscriptName);
    p.boundingBox = QRectF(otm->otmrcFontBox.left, -otm->otmrcFontBox.top,
                           otm->otmrcFontBox.right - otm->otmrcFontBox.left,
                           otm->otmrcFontBox.top - otm->otmrcFontBox.bottom);
    p.ascent = otm->otmAscent;
    p.descent = -otm->otmDescent;
    p.leading = (int)otm->otmLineGap;
    p.capHeight = 0;
    p.lineWidth = otm->otmsUnderscoreSize;
    free(otm);
    DeleteObject(SelectObject(hdc, oldfont));
    return p;
}

void QFontEngineWin::getUnscaledGlyph(glyph_t glyph, QPainterPath *path, glyph_metrics_t *metrics)
{
    LOGFONT lf = logfont;
    lf.lfHeight = unitsPerEm;
    int flags = synthesized();
    if(flags & SynthesizedItalic)
        lf.lfItalic = false;
    lf.lfWidth = 0;
    HFONT hf = CreateFontIndirect(&lf);
    HDC hdc = shared_dc();
    HGDIOBJ oldfont = SelectObject(hdc, hf);
    QFixedPoint p;
    p.x = 0;
    p.y = 0;
    addGlyphToPath(glyph, p, hdc, path, ttf, metrics);
    DeleteObject(SelectObject(hdc, oldfont));
}

bool QFontEngineWin::getSfntTableData(uint tag, uchar *buffer, uint *length) const
{
    if (!ttf)
        return false;
    HDC hdc = shared_dc();
    SelectObject(hdc, hfont);
    DWORD t = qbswap<quint32>(tag);
    *length = GetFontData(hdc, t, 0, buffer, *length);
    return *length != GDI_ERROR;
}

#if !defined(CLEARTYPE_QUALITY)
#    define CLEARTYPE_QUALITY       5
#endif

extern bool qt_cleartype_enabled;

QNativeImage *QFontEngineWin::drawGDIGlyph(HFONT font, glyph_t glyph, int margin,
                                           const QTransform &t, QImage::Format mask_format)
{
    Q_UNUSED(mask_format)
    glyph_metrics_t gm = boundingBox(glyph);

//     printf(" -> for glyph %4x\n", glyph);

    int gx = gm.x.toInt();
    int gy = gm.y.toInt();
    int iw = gm.width.toInt();
    int ih = gm.height.toInt();

    if (iw <= 0 || iw <= 0)
        return 0;

    bool has_transformation = t.type() > QTransform::TxTranslate;

#ifndef Q_WS_WINCE
    unsigned int options = ttf ? ETO_GLYPH_INDEX : 0;
    XFORM xform;

    if (has_transformation) {
        xform.eM11 = t.m11();
        xform.eM12 = t.m12();
        xform.eM21 = t.m21();
        xform.eM22 = t.m22();
        xform.eDx = margin;
        xform.eDy = margin;

        QtHDC qthdc;
        HDC hdc = qthdc.hdc();

        SetGraphicsMode(hdc, GM_ADVANCED);
        SetWorldTransform(hdc, &xform);
        HGDIOBJ old_font = SelectObject(hdc, font);

        int ggo_options = GGO_METRICS | (ttf ? GGO_GLYPH_INDEX : 0);
        GLYPHMETRICS tgm;
        MAT2 mat;
        memset(&mat, 0, sizeof(mat));
        mat.eM11.value = mat.eM22.value = 1;

        if (GetGlyphOutline(hdc, glyph, ggo_options, &tgm, 0, 0, &mat) == GDI_ERROR) {
            qWarning("QWinFontEngine: unable to query transformed glyph metrics...");
            return 0;
        }

        iw = tgm.gmBlackBoxX;
        ih = tgm.gmBlackBoxY;

        xform.eDx -= tgm.gmptGlyphOrigin.x;
        xform.eDy += tgm.gmptGlyphOrigin.y;

        SetGraphicsMode(hdc, GM_COMPATIBLE);
        SelectObject(hdc, old_font);
    }
#else // else winc
    unsigned int options = 0;
#ifdef DEBUG
    Q_ASSERT(!has_transformation);
#else
    Q_UNUSED(has_transformation);
#endif
#endif

    QNativeImage *ni = new QNativeImage(iw + 2 * margin + 4,
                                        ih + 2 * margin + 4,
                                        QNativeImage::systemFormat(), !qt_cleartype_enabled);

    /*If cleartype is enabled we use the standard system format even on Windows CE
      and not the special textbuffer format we have to use if cleartype is disabled*/

    ni->image.fill(0xffffffff);

    HDC hdc = ni->hdc;

    SelectObject(hdc, GetStockObject(NULL_BRUSH));
    SelectObject(hdc, GetStockObject(BLACK_PEN));
    SetTextColor(hdc, RGB(0,0,0));
    SetBkMode(hdc, TRANSPARENT);
    SetTextAlign(hdc, TA_BASELINE);

    HGDIOBJ old_font = SelectObject(hdc, font);

#ifndef Q_OS_WINCE
    if (has_transformation) {
        SetGraphicsMode(hdc, GM_ADVANCED);
        SetWorldTransform(hdc, &xform);
        ExtTextOut(hdc, 0, 0, options, 0, (LPCWSTR) &glyph, 1, 0);
    } else
#endif
    {
        ExtTextOut(hdc, -gx + margin, -gy + margin, options, 0, (LPCWSTR) &glyph, 1, 0);
    }
    
    SelectObject(hdc, old_font);
    return ni;
}


extern uint qt_pow_gamma[256];

QImage QFontEngineWin::alphaMapForGlyph(glyph_t glyph, const QTransform &xform)
{
    HFONT font = hfont;
    if (qt_cleartype_enabled) {
        LOGFONT lf = logfont;
        lf.lfQuality = ANTIALIASED_QUALITY;
        font = CreateFontIndirect(&lf);
    }
    QImage::Format mask_format = QNativeImage::systemFormat();
#ifndef Q_OS_WINCE
    mask_format = QImage::Format_RGB32;
#endif

    QNativeImage *mask = drawGDIGlyph(font, glyph, 0, xform, mask_format);
    if (mask == 0)
        return QImage();

    QImage indexed(mask->width(), mask->height(), QImage::Format_Indexed8);

    // ### This part is kinda pointless, but we'll crash later if we don't because some
    // code paths expects there to be colortables for index8-bit...
    QVector<QRgb> colors(256);
    for (int i=0; i<256; ++i)
        colors[i] = qRgba(0, 0, 0, i);
    indexed.setColorTable(colors);

    // Copy data... Cannot use QPainter here as GDI has messed up the
    // Alpha channel of the ni.image pixels...
    for (int y=0; y<mask->height(); ++y) {
        uchar *dest = indexed.scanLine(y);
        if (mask->image.format() == QImage::Format_RGB16) {
            const qint16 *src = (qint16 *) ((const QImage &) mask->image).scanLine(y);
            for (int x=0; x<mask->width(); ++x)
                dest[x] = 255 - qGray(src[x]);
        } else {
            const uint *src = (uint *) ((const QImage &) mask->image).scanLine(y);
            for (int x=0; x<mask->width(); ++x) {
#ifdef Q_OS_WINCE
                dest[x] = 255 - qGray(src[x]);
#else
                if (QNativeImage::systemFormat() == QImage::Format_RGB16)
                    dest[x] = 255 - qGray(src[x]);
                else
                    dest[x] = 255 - (qt_pow_gamma[qGray(src[x])] * 255. / 2047.);
#endif
            }
        }
    }

    // Cleanup...
    delete mask;
    if (qt_cleartype_enabled) {
        DeleteObject(font);
    }

    return indexed;
}

#define SPI_GETFONTSMOOTHINGCONTRAST           0x200C
#define SPI_SETFONTSMOOTHINGCONTRAST           0x200D

QImage QFontEngineWin::alphaRGBMapForGlyph(glyph_t glyph, QFixed, int margin, const QTransform &t)
{
    HFONT font = hfont;

    int contrast;
    SystemParametersInfo(SPI_GETFONTSMOOTHINGCONTRAST, 0, &contrast, 0);
    SystemParametersInfo(SPI_SETFONTSMOOTHINGCONTRAST, 0, (void *) 1000, 0);

    QNativeImage *mask = drawGDIGlyph(font, glyph, margin, t, QImage::Format_RGB32);
    SystemParametersInfo(SPI_SETFONTSMOOTHINGCONTRAST, 0, (void *) contrast, 0);

    if (mask == 0)
        return QImage();

    // Gracefully handle the odd case when the display is 16-bit
    const QImage source = mask->image.depth() == 32
                          ? mask->image
                          : mask->image.convertToFormat(QImage::Format_RGB32);

    QImage rgbMask(mask->width(), mask->height(), QImage::Format_RGB32);
    for (int y=0; y<mask->height(); ++y) {
        uint *dest = (uint *) rgbMask.scanLine(y);
        const uint *src = (uint *) source.scanLine(y);
        for (int x=0; x<mask->width(); ++x) {
            dest[x] = 0xffffffff - (0x00ffffff & src[x]);
        }
    }

    delete mask;

    return rgbMask;
}

// From qfontdatabase_win.cpp
extern QFontEngine *qt_load_font_engine_win(const QFontDef &request);
QFontEngine *QFontEngineWin::cloneWithSize(qreal pixelSize) const
{
    QFontDef request = fontDef;
    QString actualFontName = request.family;
    if (!uniqueFamilyName.isEmpty())
        request.family = uniqueFamilyName;
    request.pixelSize = pixelSize;

    QFontEngine *fontEngine = qt_load_font_engine_win(request);
    if (fontEngine != NULL)
        fontEngine->fontDef.family = actualFontName;

    return fontEngine;
}

// -------------------------------------- Multi font engine

QFontEngineMultiWin::QFontEngineMultiWin(QFontEngine *first, const QStringList &fallbacks)
        : QFontEngineMulti(fallbacks.size()+1),
          fallbacks(fallbacks)
{
    engines[0] = first;
    first->ref.ref();
    fontDef = engines[0]->fontDef;
    cache_cost = first->cache_cost;
}

void QFontEngineMultiWin::loadEngine(int at)
{
    Q_ASSERT(at < engines.size());
    Q_ASSERT(engines.at(at) == 0);

    QString fam = fallbacks.at(at-1);

    LOGFONT lf = static_cast<QFontEngineWin *>(engines.at(0))->logfont;
    memcpy(lf.lfFaceName, fam.utf16(), sizeof(wchar_t) * qMin(fam.length() + 1, 32));  // 32 = Windows hard-coded
    HFONT hfont = CreateFontIndirect(&lf);

    bool stockFont = false;
    if (hfont == 0) {
        hfont = (HFONT)GetStockObject(ANSI_VAR_FONT);
        stockFont = true;
    }
    engines[at] = new QFontEngineWin(fam, hfont, stockFont, lf);
    engines[at]->ref.ref();
    engines[at]->fontDef = fontDef;

    // TODO: increase cost in QFontCache for the font engine loaded here
}

QT_END_NAMESPACE
