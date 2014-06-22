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

#include "qfontsubset_p.h"
#include <qdebug.h>
#include <qendian.h>
#include <qpainterpath.h>
#include "private/qpdf_p.h"
#include "private/qfunctions_p.h"

#include "qfontsubset_agl.cpp"

#include <algorithm>

QT_BEGIN_NAMESPACE

// This map is used for symbol fonts to get the correct glyph names for the latin range
static const unsigned short symbol_map[0x100] = {
    0x0000, 0x0001, 0x0002, 0x0003, 0x0004, 0x0005, 0x0006, 0x0007,
    0x0008, 0x0009, 0x000a, 0x000b, 0x000c, 0x000d, 0x000e, 0x000f,
    0x0010, 0x0011, 0x0012, 0x0013, 0x0014, 0x0015, 0x0016, 0x0017,
    0x0018, 0x0019, 0x001a, 0x001b, 0x001c, 0x001d, 0x001e, 0x001f,
    0x0020, 0x0021, 0x2200, 0x0023, 0x2203, 0x0025, 0x0026, 0x220b,
    0x0028, 0x0029, 0x2217, 0x002b, 0x002c, 0x2212, 0x002e, 0x002f,
    0x0030, 0x0031, 0x0032, 0x0033, 0x0034, 0x0035, 0x0036, 0x0037,
    0x0038, 0x0039, 0x003a, 0x003b, 0x003c, 0x003d, 0x003e, 0x003f,

    0x2245, 0x0391, 0x0392, 0x03a7, 0x0394, 0x0395, 0x03a6, 0x0393,
    0x0397, 0x0399, 0x03d1, 0x039a, 0x039b, 0x039c, 0x039d, 0x039f,
    0x03a0, 0x0398, 0x03a1, 0x03a3, 0x03a4, 0x03a5, 0x03c2, 0x03a9,
    0x039e, 0x03a8, 0x0396, 0x005b, 0x2234, 0x005d, 0x22a5, 0x005f,
    0xf8e5, 0x03b1, 0x03b2, 0x03c7, 0x03b4, 0x03b5, 0x03c6, 0x03b3,
    0x03b7, 0x03b9, 0x03d5, 0x03ba, 0x03bb, 0x03bc, 0x03bd, 0x03bf,
    0x03c0, 0x03b8, 0x03c1, 0x03c3, 0x03c4, 0x03c5, 0x03d6, 0x03c9,
    0x03be, 0x03c8, 0x03b6, 0x007b, 0x007c, 0x007d, 0x223c, 0x007f,

    0x0080, 0x0081, 0x0082, 0x0083, 0x0084, 0x0085, 0x0086, 0x0087,
    0x0088, 0x0089, 0x008a, 0x008b, 0x008c, 0x008d, 0x008e, 0x008f,
    0x0090, 0x0091, 0x0092, 0x0093, 0x0094, 0x0095, 0x0096, 0x0097,
    0x0098, 0x0099, 0x009a, 0x009b, 0x009c, 0x009d, 0x009e, 0x009f,
    0x20ac, 0x03d2, 0x2023, 0x2264, 0x2044, 0x221e, 0x0192, 0x2263,
    0x2666, 0x2665, 0x2660, 0x2194, 0x2190, 0x2191, 0x2192, 0x2193,
    0x00b0, 0x00b1, 0x2033, 0x2265, 0x00d7, 0x221d, 0x2202, 0x2022,
    0x00f7, 0x2260, 0x2261, 0x2248, 0x2026, 0xf8e6, 0xf8e7, 0x21b5,

    0x2135, 0x2111, 0x211c, 0x2118, 0x2297, 0x2295, 0x2205, 0x2229,
    0x222a, 0x2283, 0x2287, 0x2284, 0x2282, 0x2286, 0x2208, 0x2209,
    0x2220, 0x2207, 0xf6da, 0xf6d9, 0xf6db, 0x220f, 0x221a, 0x22c5,
    0x00ac, 0x2227, 0x2228, 0x21d4, 0x21d0, 0x21d1, 0x21d2, 0x21d3,
    0x25ca, 0x2329, 0xf8e8, 0xf8e9, 0xf8ea, 0x2211, 0xf8eb, 0xf8ec,
    0xf8ed, 0xf8ee, 0xf8ef, 0xf8f0, 0xf8f1, 0xf8f2, 0xf8f3, 0xf8f4,
    0x0000, 0x232a, 0x222b, 0x2320, 0xf8f5, 0x2321, 0xf8f6, 0xf8f7,
    0xf8f8, 0xf8f9, 0xf8fa, 0xf8fb, 0xf8fc, 0xf8fd, 0xf8fe, 0x0000
};

// ---------------------------- PS/PDF helper methods -----------------------------------

#ifndef QT_NO_PDF

QByteArray QFontSubset::glyphName(unsigned short unicode, bool symbol)
{
    if (symbol && unicode < 0x100)
        // map from latin1 to symbol
        unicode = symbol_map[unicode];

    const AGLEntry *r = std::lower_bound(unicode_to_agl_map, unicode_to_agl_map + unicode_to_agl_map_size, unicode);
    if ((r != unicode_to_agl_map + unicode_to_agl_map_size) && !(unicode < *r))
        return glyph_names + r->index;

    char buffer[8];
    buffer[0] = 'u';
    buffer[1] = 'n';
    buffer[2] = 'i';
    QPdf::toHex(unicode, buffer+3);
    return buffer;
}

QByteArray QFontSubset::glyphName(unsigned int glyph, const QVector<int> &reverseMap) const
{
    uint glyphIndex = glyph_indices[glyph];

    if (glyphIndex == 0)
        return "/.notdef";

    QByteArray ba;
    QPdf::ByteStream s(&ba);
    if (reverseMap[glyphIndex] && reverseMap[glyphIndex] < 0x10000) {
        s << '/' << glyphName(reverseMap[glyphIndex], false);
    } else {
        s << "/gl" << (int)glyphIndex;
    }
    return ba;
}


QByteArray QFontSubset::widthArray() const
{
    Q_ASSERT(!widths.isEmpty());

    QFontEngine::Properties properties = fontEngine->properties();

    QByteArray width;
    QPdf::ByteStream s(&width);
    QFixed scale = QFixed(1000)/emSquare;

    QFixed defWidth = widths[0];
    //qDebug("defWidth=%d, scale=%f", defWidth.toInt(), scale.toReal());
    for (int i = 0; i < nGlyphs(); ++i) {
        if (defWidth != widths[i])
            defWidth = 0;
    }
    if (defWidth > 0) {
        s << "/DW " << (defWidth*scale).toInt();
    } else {
        s << "/W [";
        for (int g = 0; g < nGlyphs();) {
            QFixed w = widths[g];
            int start = g;
            int startLinear = 0;
            ++g;
            while (g < nGlyphs()) {
                QFixed nw = widths[g];
                if (nw == w) {
                if (!startLinear)
                    startLinear = g - 1;
                } else {
                    if (startLinear > 0 && g - startLinear >= 10)
                        break;
                    startLinear = 0;
                }
                w = nw;
                ++g;
            }
            // qDebug("start=%x startLinear=%x g-1=%x",start,startLinear,g-1);
            if (g - startLinear < 10)
                startLinear = 0;
            int endnonlinear = startLinear ? startLinear : g;
            // qDebug("    startLinear=%x endnonlinear=%x", startLinear,endnonlinear);
            if (endnonlinear > start) {
                s << start << '[';
                for (int i = start; i < endnonlinear; ++i)
                    s << (widths[i]*scale).toInt();
                s << "]\n";
            }
            if (startLinear)
                s << startLinear << g - 1 << (widths[startLinear]*scale).toInt() << '\n';
        }
        s << "]\n";
    }
    return width;
}

static void checkRanges(QPdf::ByteStream &ts, QByteArray &ranges, int &nranges)
{
    if (++nranges > 100) {
        ts << nranges << "beginbfrange\n"
           << ranges << "endbfrange\n";
        ranges = QByteArray();
        nranges = 0;
    }
}

QVector<int> QFontSubset::getReverseMap() const
{
    QVector<int> reverseMap(0x10000, 0);
    for (uint uc = 0; uc < 0x10000; ++uc) {
        int idx = glyph_indices.indexOf(fontEngine->glyphIndex(uc));
        if (idx >= 0 && !reverseMap.at(idx))
            reverseMap[idx] = uc;
    }
    return reverseMap;
}

QByteArray QFontSubset::createToUnicodeMap() const
{
    QVector<int> reverseMap = getReverseMap();

    QByteArray touc;
    QPdf::ByteStream ts(&touc);
    ts << "/CIDInit /ProcSet findresource begin\n"
        "12 dict begin\n"
        "begincmap\n"
        "/CIDSystemInfo << /Registry (Adobe) /Ordering (UCS) /Supplement 0 >> def\n"
        "/CMapName /Adobe-Identity-UCS def\n"
        "/CMapType 2 def\n"
        "1 begincodespacerange\n"
        "<0000> <FFFF>\n"
        "endcodespacerange\n";

    int nranges = 1;
    QByteArray ranges = "<0000> <0000> <0000>\n";
    QPdf::ByteStream s(&ranges);

    char buf[5];
    for (int g = 1; g < nGlyphs(); ) {
        int uc0 = reverseMap.at(g);
        if (!uc0) {
            ++g;
            continue;
        }
        int start = g;
        int startLinear = 0;
        ++g;
        while (g < nGlyphs()) {
            int uc = reverseMap[g];
            // cmaps can't have the high byte changing within one range, so we need to break on that as well
            if (!uc || (g>>8) != (start >> 8))
                break;
            if (uc == uc0 + 1) {
                if (!startLinear)
                    startLinear = g - 1;
            } else {
                if (startLinear > 0 && g - startLinear >= 10)
                    break;
                startLinear = 0;
            }
            uc0 = uc;
            ++g;
        }
        // qDebug("start=%x startLinear=%x g-1=%x",start,startLinear,g-1);
        if (g - startLinear < 10)
            startLinear = 0;
        int endnonlinear = startLinear ? startLinear : g;
        // qDebug("    startLinear=%x endnonlinear=%x", startLinear,endnonlinear);
        if (endnonlinear > start) {
            s << '<' << QPdf::toHex((ushort)start, buf) << "> <";
            s << QPdf::toHex((ushort)(endnonlinear - 1), buf) << "> ";
            if (endnonlinear == start + 1) {
                s << '<' << QPdf::toHex((ushort)reverseMap[start], buf) << ">\n";
            } else {
                s << '[';
                for (int i = start; i < endnonlinear; ++i) {
                    s << '<' << QPdf::toHex((ushort)reverseMap[i], buf) << "> ";
                }
                s << "]\n";
            }
            checkRanges(ts, ranges, nranges);
        }
        if (startLinear) {
            while (startLinear < g) {
                int len = g - startLinear;
                int uc_start = reverseMap[startLinear];
                int uc_end = uc_start + len - 1;
                if ((uc_end >> 8) != (uc_start >> 8))
                    len = 256 - (uc_start & 0xff);
                s << '<' << QPdf::toHex((ushort)startLinear, buf) << "> <";
                s << QPdf::toHex((ushort)(startLinear + len - 1), buf) << "> ";
                s << '<' << QPdf::toHex((ushort)reverseMap[startLinear], buf) << ">\n";
                checkRanges(ts, ranges, nranges);
                startLinear += len;
            }
        }
    }
    if (nranges) {
        ts << nranges << "beginbfrange\n"
           << ranges << "endbfrange\n";
    }
    ts << "endcmap\n"
        "CMapName currentdict /CMap defineresource pop\n"
        "end\n"
        "end\n";

    return touc;
}

int QFontSubset::addGlyph(int index)
{
    int idx = glyph_indices.indexOf(index);
    if (idx < 0) {
        idx = glyph_indices.size();
        glyph_indices.append(index);
    }
    return idx;
}

#endif // QT_NO_PDF

// ------------------------------ Truetype generation ----------------------------------------------

typedef qint16 F2DOT14;
typedef quint32 Tag;
typedef quint16 GlyphID;
typedef quint16 Offset;


class QTtfStream {
public:
    QTtfStream(QByteArray &ba) : data((uchar *)ba.data()) { start = data; }
    QTtfStream &operator <<(quint8 v) { *data = v; ++data; return *this; }
    QTtfStream &operator <<(quint16 v) { qToBigEndian(v, data); data += sizeof(v); return *this; }
    QTtfStream &operator <<(quint32 v) { qToBigEndian(v, data); data += sizeof(v); return *this; }
    QTtfStream &operator <<(qint8 v) { *data = quint8(v); ++data; return *this; }
    QTtfStream &operator <<(qint16 v) { qToBigEndian(v, data); data += sizeof(v); return *this; }
    QTtfStream &operator <<(qint32 v) { qToBigEndian(v, data); data += sizeof(v); return *this; }
    QTtfStream &operator <<(qint64 v) { qToBigEndian(v, data); data += sizeof(v); return *this; }

    int offset() const { return data - start; }
    void setOffset(int o) { data = start + o; }
    void align4() { while (offset() & 3) { *data = '\0'; ++data; } }
private:
    uchar *data;
    uchar *start;
};

struct QTtfTable {
    Tag tag;
    QByteArray data;
};
Q_DECLARE_TYPEINFO(QTtfTable, Q_MOVABLE_TYPE);


struct qttf_head_table {
    qint32 font_revision;
    quint16 flags;
    qint64 created;
    qint64 modified;
    qint16 xMin;
    qint16 yMin;
    qint16 xMax;
    qint16 yMax;
    quint16 macStyle;
    qint16 indexToLocFormat;
};


struct qttf_hhea_table {
    qint16 ascender;
    qint16 descender;
    qint16 lineGap;
    quint16 maxAdvanceWidth;
    qint16 minLeftSideBearing;
    qint16 minRightSideBearing;
    qint16 xMaxExtent;
    quint16 numberOfHMetrics;
};


struct qttf_maxp_table {
    quint16 numGlyphs;
    quint16 maxPoints;
    quint16 maxContours;
    quint16 maxCompositePoints;
    quint16 maxCompositeContours;
    quint16 maxComponentElements;
    quint16 maxComponentDepth;
};

struct qttf_name_table {
    QString copyright;
    QString family;
    QString subfamily;
    QString postscript_name;
};


static QTtfTable generateHead(const qttf_head_table &head);
static QTtfTable generateHhea(const qttf_hhea_table &hhea);
static QTtfTable generateMaxp(const qttf_maxp_table &maxp);
static QTtfTable generateName(const qttf_name_table &name);

struct qttf_font_tables
{
    qttf_head_table head;
    qttf_hhea_table hhea;
    qttf_maxp_table maxp;
};


struct QTtfGlyph {
    quint16 index;
    qint16 xMin;
    qint16 xMax;
    qint16 yMin;
    qint16 yMax;
    quint16 advanceWidth;
    qint16 lsb;
    quint16 numContours;
    quint16 numPoints;
    QByteArray data;
};
Q_DECLARE_TYPEINFO(QTtfGlyph, Q_MOVABLE_TYPE);

static QTtfGlyph generateGlyph(int index, const QPainterPath &path, qreal advance, qreal lsb, qreal ppem);
// generates glyf, loca and hmtx
static QList<QTtfTable> generateGlyphTables(qttf_font_tables &tables, const QList<QTtfGlyph> &_glyphs);

static QByteArray bindFont(const QList<QTtfTable>& _tables);


static quint32 checksum(const QByteArray &table)
{
    quint32 sum = 0;
    int offset = 0;
    const uchar *d = (uchar *)table.constData();
    while (offset <= table.size()-3) {
        sum += qFromBigEndian<quint32>(d + offset);
        offset += 4;
    }
    int shift = 24;
    quint32 x = 0;
    while (offset < table.size()) {
        x |= ((quint32)d[offset]) << shift;
        ++offset;
        shift -= 8;
    }
    sum += x;

    return sum;
}

static QTtfTable generateHead(const qttf_head_table &head)
{
    const int head_size = 54;
    QTtfTable t;
    t.tag = MAKE_TAG('h', 'e', 'a', 'd');
    t.data.resize(head_size);

    QTtfStream s(t.data);

// qint32  Table version number  0x00010000 for version 1.0.
// qint32  fontRevision  Set by font manufacturer.
    s << qint32(0x00010000)
      << head.font_revision
// quint32  checkSumAdjustment  To compute: set it to 0, sum the entire font as quint32, then store 0xB1B0AFBA - sum.
      << quint32(0)
// quint32  magicNumber  Set to 0x5F0F3CF5.
      << quint32(0x5F0F3CF5)
// quint16  flags  Bit 0: Baseline for font at y=0;
// Bit 1: Left sidebearing point at x=0;
// Bit 2: Instructions may depend on point size;
// Bit 3: Force ppem to integer values for all internal scaler math; may use fractional ppem sizes if this bit is clear;
// Bit 4: Instructions may alter advance width (the advance widths might not scale linearly);
// Bits 5-10: These should be set according to  Apple's specification . However, they are not implemented in OpenType.
// Bit 11: Font data is 'lossless,' as a result of having been compressed and decompressed with the Agfa MicroType Express engine.
// Bit 12: Font converted (produce compatible metrics)
// Bit 13: Font optimized for ClearType
// Bit 14: Reserved, set to 0
// Bit 15: Reserved, set to 0
      << quint16(0)

// quint16  unitsPerEm  Valid range is from 16 to 16384. This value should be a power of 2 for fonts that have TrueType outlines.
      << quint16(2048)
// qint64  created  Number of seconds since 12:00 midnight, January 1, 1904. 64-bit integer
      << head.created
// qint64  modified  Number of seconds since 12:00 midnight, January 1, 1904. 64-bit integer
      << head.modified
// qint16  xMin  For all glyph bounding boxes.
// qint16  yMin  For all glyph bounding boxes.
// qint16  xMax  For all glyph bounding boxes.
// qint16  yMax  For all glyph bounding boxes.
      << head.xMin
      << head.yMin
      << head.xMax
      << head.yMax
// quint16  macStyle  Bit 0: Bold (if set to 1);
// Bit 1: Italic (if set to 1)
// Bit 2: Underline (if set to 1)
// Bit 3: Outline (if set to 1)
// Bit 4: Shadow (if set to 1)
// Bit 5: Condensed (if set to 1)
// Bit 6: Extended (if set to 1)
// Bits 7-15: Reserved (set to 0).
      << head.macStyle
// quint16  lowestRecPPEM  Smallest readable size in pixels.
      << quint16(6) // just a wild guess
// qint16  fontDirectionHint   0: Fully mixed directional glyphs;
      << qint16(0)
// 1: Only strongly left to right;
// 2: Like 1 but also contains neutrals;
// -1: Only strongly right to left;
// -2: Like -1 but also contains neutrals. 1
// qint16  indexToLocFormat  0 for short offsets, 1 for long.
      << head.indexToLocFormat
// qint16  glyphDataFormat  0 for current format.
      << qint16(0);

    Q_ASSERT(s.offset() == head_size);
    return t;
}


static QTtfTable generateHhea(const qttf_hhea_table &hhea)
{
    const int hhea_size = 36;
    QTtfTable t;
    t.tag = MAKE_TAG('h', 'h', 'e', 'a');
    t.data.resize(hhea_size);

    QTtfStream s(t.data);
// qint32  Table version number  0x00010000 for version 1.0.
    s << qint32(0x00010000)
// qint16  Ascender  Typographic ascent.  (Distance from baseline of highest ascender)
      << hhea.ascender
// qint16  Descender  Typographic descent.  (Distance from baseline of lowest descender)
      << hhea.descender
// qint16  LineGap  Typographic line gap.
// Negative LineGap values are treated as zero
// in Windows 3.1, System 6, and
// System 7.
      << hhea.lineGap
// quint16  advanceWidthMax  Maximum advance width value in 'hmtx' table.
      << hhea.maxAdvanceWidth
// qint16  minLeftSideBearing  Minimum left sidebearing value in 'hmtx' table.
      << hhea.minLeftSideBearing
// qint16  minRightSideBearing  Minimum right sidebearing value; calculated as Min(aw - lsb - (xMax - xMin)).
      << hhea.minRightSideBearing
// qint16  xMaxExtent  Max(lsb + (xMax - xMin)).
      << hhea.xMaxExtent
// qint16  caretSlopeRise  Used to calculate the slope of the cursor (rise/run); 1 for vertical.
      << qint16(1)
// qint16  caretSlopeRun  0 for vertical.
      << qint16(0)
// qint16  caretOffset  The amount by which a slanted highlight on a glyph needs to be shifted to produce the best appearance. Set to 0 for non-slanted fonts
      << qint16(0)
// qint16  (reserved)  set to 0
      << qint16(0)
// qint16  (reserved)  set to 0
      << qint16(0)
// qint16  (reserved)  set to 0
      << qint16(0)
// qint16  (reserved)  set to 0
      << qint16(0)
// qint16  metricDataFormat  0 for current format.
      << qint16(0)
// quint16  numberOfHMetrics  Number of hMetric entries in 'hmtx' table
      << hhea.numberOfHMetrics;

    Q_ASSERT(s.offset() == hhea_size);
    return t;
}


static QTtfTable generateMaxp(const qttf_maxp_table &maxp)
{
    const int maxp_size = 32;
    QTtfTable t;
    t.tag = MAKE_TAG('m', 'a', 'x', 'p');
    t.data.resize(maxp_size);

    QTtfStream s(t.data);

// qint32  Table version number  0x00010000 for version 1.0.
    s << qint32(0x00010000)
// quint16  numGlyphs  The number of glyphs in the font.
      << maxp.numGlyphs
// quint16  maxPoints  Maximum points in a non-composite glyph.
      << maxp.maxPoints
// quint16  maxContours  Maximum contours in a non-composite glyph.
      << maxp.maxContours
// quint16  maxCompositePoints  Maximum points in a composite glyph.
      << maxp.maxCompositePoints
// quint16  maxCompositeContours  Maximum contours in a composite glyph.
      << maxp.maxCompositeContours
// quint16  maxZones  1 if instructions do not use the twilight zone (Z0), or 2 if instructions do use Z0; should be set to 2 in most cases.
      << quint16(1) // we do not embed instructions
// quint16  maxTwilightPoints  Maximum points used in Z0.
      << quint16(0)
// quint16  maxStorage  Number of Storage Area locations.
      << quint16(0)
// quint16  maxFunctionDefs  Number of FDEFs.
      << quint16(0)
// quint16  maxInstructionDefs  Number of IDEFs.
      << quint16(0)
// quint16  maxStackElements  Maximum stack depth2.
      << quint16(0)
// quint16  maxSizeOfInstructions  Maximum byte count for glyph instructions.
      << quint16(0)
// quint16  maxComponentElements  Maximum number of components referenced at "top level" for any composite glyph.
      << maxp.maxComponentElements
// quint16  maxComponentDepth  Maximum levels of recursion; 1 for simple components.
      << maxp.maxComponentDepth;

    Q_ASSERT(s.offset() == maxp_size);
    return t;
}

struct QTtfNameRecord {
    quint16 nameId;
    QString value;
};

static QTtfTable generateName(const QList<QTtfNameRecord> &name);

static QTtfTable generateName(const qttf_name_table &name)
{
    QList<QTtfNameRecord> list;
    QTtfNameRecord rec;
    rec.nameId = 0;
    rec.value = name.copyright;
    list.append(rec);
    rec.nameId = 1;
    rec.value = name.family;
    list.append(rec);
    rec.nameId = 2;
    rec.value = name.subfamily;
    list.append(rec);
    rec.nameId = 4;
    rec.value = name.family;
    if (name.subfamily != QLatin1String("Regular"))
        rec.value += QLatin1Char(' ') + name.subfamily;
    list.append(rec);
    rec.nameId = 6;
    rec.value = name.postscript_name;
    list.append(rec);

    return generateName(list);
}

// ####### should probably generate Macintosh/Roman name entries as well
static QTtfTable generateName(const QList<QTtfNameRecord> &name)
{
    const int char_size = 2;

    QTtfTable t;
    t.tag = MAKE_TAG('n', 'a', 'm', 'e');

    const int name_size = 6 + 12*name.size();
    int string_size = 0;
    for (int i = 0; i < name.size(); ++i) {
        string_size += name.at(i).value.length()*char_size;
    }
    t.data.resize(name_size + string_size);

    QTtfStream s(t.data);
// quint16  format  Format selector (=0).
    s << quint16(0)
// quint16  count  Number of name records.
      << quint16(name.size())
// quint16  stringOffset  Offset to start of string storage (from start of table).
      << quint16(name_size);
// NameRecord  nameRecord[count]  The name records where count is the number of records.
// (Variable)

    int off = 0;
    for (int i = 0; i < name.size(); ++i) {
        int len = name.at(i).value.length()*char_size;
// quint16  platformID  Platform ID.
// quint16  encodingID  Platform-specific encoding ID.
// quint16  languageID  Language ID.
        s << quint16(3)
          << quint16(1)
          << quint16(0x0409) // en_US
// quint16  nameId  Name ID.
          << name.at(i).nameId
// quint16  length  String length (in bytes).
          << quint16(len)
// quint16  offset  String offset from start of storage area (in bytes).
          << quint16(off);
        off += len;
    }
    for (int i = 0; i < name.size(); ++i) {
        const QString &n = name.at(i).value;
        const ushort *uc = n.utf16();
        for (int i = 0; i < n.length(); ++i) {
            s << quint16(*uc);
            ++uc;
        }
    }
    return t;
}


enum Flags {
    OffCurve = 0,
    OnCurve = (1 << 0),
    XShortVector = (1 << 1),
    YShortVector = (1 << 2),
    Repeat = (1 << 3),
    XSame = (1 << 4),
    XShortPositive = (1 << 4),
    YSame = (1 << 5),
    YShortPositive = (1 << 5)
};
struct TTF_POINT {
    qint16 x;
    qint16 y;
    quint8 flags;
};
Q_DECLARE_TYPEINFO(TTF_POINT, Q_PRIMITIVE_TYPE);

static void convertPath(const QPainterPath &path, QList<TTF_POINT> *points, QList<int> *endPoints, qreal ppem)
{
    int numElements = path.elementCount();
    for (int i = 0; i < numElements - 1; ++i) {
        const QPainterPath::Element &e = path.elementAt(i);
        TTF_POINT p;
        p.x = qRound(e.x * 2048. / ppem);
        p.y = qRound(-e.y * 2048. / ppem);
        p.flags = 0;

        switch(e.type) {
        case QPainterPath::MoveToElement:
            if (i != 0) {
                // see if start and end points of the last contour agree
                int start = endPoints->size() ? endPoints->at(endPoints->size()-1) - 1 : 0;
                int end = points->size() - 1;
                if (points->at(end).x == points->at(start).x
                    && points->at(end).y == points->at(start).y)
                    points->takeLast();
                endPoints->append(points->size() - 1);
            }
            // fall through
        case QPainterPath::LineToElement:
            p.flags = OnCurve;
            break;
        case QPainterPath::CurveToElement: {
            // cubic bezier curve, we need to reduce to a list of quadratic curves
            TTF_POINT list[3*16 + 4]; // we need max 16 subdivisions
            list[3] = points->at(points->size() - 1);
            list[2] = p;
            const QPainterPath::Element &e2 = path.elementAt(++i);
            list[1].x = qRound(e2.x * 2048. / ppem);
            list[1].y = qRound(-e2.y * 2048. / ppem);
            const QPainterPath::Element &e3 = path.elementAt(++i);
            list[0].x = qRound(e3.x * 2048. / ppem);
            list[0].y = qRound(-e3.y * 2048. / ppem);

            TTF_POINT *base = list;

            bool try_reduce = points->size() > 1
                              && points->at(points->size() - 1).flags == OnCurve
                              && points->at(points->size() - 2).flags == OffCurve;
//             qDebug("generating beziers:");
            while (base >= list) {
                const int split_limit = 3;
//                 {
//                     qDebug("iteration:");
//                     TTF_POINT *x = list;
//                     while (x <= base + 3) {
//                         qDebug() << "    " << QPoint(x->x, x->y);
//                         ++x;
//                     }
//                 }
                Q_ASSERT(base - list < 3*16 + 1);
                // first see if we can easily reduce the cubic to a quadratic bezier curve
                int i1_x = base[1].x + ((base[1].x - base[0].x) >> 1);
                int i1_y = base[1].y + ((base[1].y - base[0].y) >> 1);
                int i2_x = base[2].x + ((base[2].x - base[3].x) >> 1);
                int i2_y = base[2].y + ((base[2].y - base[3].y) >> 1);
//                 qDebug() << "checking: i1=" << QPoint(i1_x, i1_y) << " i2=" << QPoint(i2_x, i2_y);
                if (qAbs(i1_x - i2_x) <= split_limit && qAbs(i1_y - i2_y) <= split_limit) {
                    // got a quadratic bezier curve
                    TTF_POINT np;
                    np.x = (i1_x + i2_x) >> 1;
                    np.y = (i1_y + i2_y) >> 1;
                    if (try_reduce) {
                        // see if we can optimize out the last onCurve point
                        int mx = (points->at(points->size() - 2).x + base[2].x) >> 1;
                        int my = (points->at(points->size() - 2).y + base[2].y) >> 1;
                        if (qAbs(mx - base[3].x) <= split_limit && qAbs(my = base[3].y) <= split_limit)
                            points->takeLast();
                        try_reduce = false;
                    }
                    np.flags = OffCurve;
                    points->append(np);
//                     qDebug() << "   appending offcurve point " << QPoint(np.x, np.y);
                    base -= 3;
                } else {
                    // need to split
//                     qDebug() << "  -> splitting";
                    qint16 a, b, c, d;
                    base[6].x = base[3].x;
                    c = base[1].x;
                    d = base[2].x;
                    base[1].x = a = ( base[0].x + c ) >> 1;
                    base[5].x = b = ( base[3].x + d ) >> 1;
                    c = ( c + d ) >> 1;
                    base[2].x = a = ( a + c ) >> 1;
                    base[4].x = b = ( b + c ) >> 1;
                    base[3].x = ( a + b ) >> 1;

                    base[6].y = base[3].y;
                    c = base[1].y;
                    d = base[2].y;
                    base[1].y = a = ( base[0].y + c ) >> 1;
                    base[5].y = b = ( base[3].y + d ) >> 1;
                    c = ( c + d ) >> 1;
                    base[2].y = a = ( a + c ) >> 1;
                    base[4].y = b = ( b + c ) >> 1;
                    base[3].y = ( a + b ) >> 1;
                    base += 3;
                }
            }
            p = list[0];
            p.flags = OnCurve;
            break;
        }
        case QPainterPath::CurveToDataElement:
            Q_ASSERT(false);
            break;
        }
//         qDebug() << "   appending oncurve point " << QPoint(p.x, p.y);
        points->append(p);
    }
    int start = endPoints->size() ? endPoints->at(endPoints->size()-1) + 1 : 0;
    int end = points->size() - 1;
    if (points->at(end).x == points->at(start).x
        && points->at(end).y == points->at(start).y)
        points->takeLast();
    endPoints->append(points->size() - 1);
}

static void getBounds(const QList<TTF_POINT> &points, qint16 *xmin, qint16 *xmax, qint16 *ymin, qint16 *ymax)
{
    *xmin = points.at(0).x;
    *xmax = *xmin;
    *ymin = points.at(0).y;
    *ymax = *ymin;

    for (int i = 1; i < points.size(); ++i) {
        *xmin = qMin(*xmin, points.at(i).x);
        *xmax = qMax(*xmax, points.at(i).x);
        *ymin = qMin(*ymin, points.at(i).y);
        *ymax = qMax(*ymax, points.at(i).y);
    }
}

static int convertToRelative(QList<TTF_POINT> *points)
{
    // convert points to relative and setup flags
//     qDebug() << "relative points:";
    qint16 prev_x = 0;
    qint16 prev_y = 0;
    int point_array_size = 0;
    for (int i = 0; i < points->size(); ++i) {
        const int x = points->at(i).x;
        const int y = points->at(i).y;
        TTF_POINT rel;
        rel.x = x - prev_x;
        rel.y = y - prev_y;
        rel.flags = points->at(i).flags;
        Q_ASSERT(rel.flags < 2);
        if (!rel.x) {
            rel.flags |= XSame;
        } else if (rel.x > 0 && rel.x < 256) {
            rel.flags |= XShortVector|XShortPositive;
            point_array_size++;
        } else if (rel.x < 0 && rel.x > -256) {
            rel.flags |= XShortVector;
            rel.x = -rel.x;
            point_array_size++;
        } else {
            point_array_size += 2;
        }
        if (!rel.y) {
            rel.flags |= YSame;
        } else if (rel.y > 0 && rel.y < 256) {
            rel.flags |= YShortVector|YShortPositive;
            point_array_size++;
        } else if (rel.y < 0 && rel.y > -256) {
            rel.flags |= YShortVector;
            rel.y = -rel.y;
            point_array_size++;
        } else {
            point_array_size += 2;
        }
        (*points)[i] = rel;
// #define toString(x) ((rel.flags & x) ? #x : "")
//         qDebug() << "    " << QPoint(rel.x, rel.y) << "flags="
//                  << toString(OnCurve) << toString(XShortVector)
//                  << (rel.flags & XShortVector ? toString(XShortPositive) : toString(XSame))
//                  << toString(YShortVector)
//                  << (rel.flags & YShortVector ? toString(YShortPositive) : toString(YSame));

        prev_x = x;
        prev_y = y;
    }
    return point_array_size;
}

static void getGlyphData(QTtfGlyph *glyph, const QList<TTF_POINT> &points, const QList<int> &endPoints, int point_array_size)
{
    const int max_size = 5*sizeof(qint16) // header
                         + endPoints.size()*sizeof(quint16) // end points of contours
                         + sizeof(quint16) // instruction length == 0
                         + points.size()*(1) // flags
                         + point_array_size; // coordinates

    glyph->data.resize(max_size);

    QTtfStream s(glyph->data);
    s << qint16(endPoints.size())
      << glyph->xMin << glyph->yMin << glyph->xMax << glyph->yMax;

    for (int i = 0; i < endPoints.size(); ++i)
        s << quint16(endPoints.at(i));
    s << quint16(0); // instruction length

    // emit flags
    for (int i = 0; i < points.size(); ++i)
        s << quint8(points.at(i).flags);
    // emit points
    for (int i = 0; i < points.size(); ++i) {
        quint8 flags = points.at(i).flags;
        qint16 x = points.at(i).x;

        if (flags & XShortVector)
            s << quint8(x);
        else if (!(flags & XSame))
            s << qint16(x);
    }
    for (int i = 0; i < points.size(); ++i) {
        quint8 flags = points.at(i).flags;
        qint16 y = points.at(i).y;

        if (flags & YShortVector)
            s << quint8(y);
        else if (!(flags & YSame))
            s << qint16(y);
    }

//     qDebug() << "offset=" << s.offset() << "max_size=" << max_size << "point_array_size=" << point_array_size;
    Q_ASSERT(s.offset() == max_size);

    glyph->numContours = endPoints.size();
    glyph->numPoints = points.size();
}

static QTtfGlyph generateGlyph(int index, const QPainterPath &path, qreal advance, qreal lsb, qreal ppem)
{
    QList<TTF_POINT> points;
    QList<int> endPoints;
    QTtfGlyph glyph;
    glyph.index = index;
    glyph.advanceWidth = qRound(advance * 2048. / ppem);
    glyph.lsb = qRound(lsb * 2048. / ppem);

    if (!path.elementCount()) {
        //qDebug("glyph %d is empty", index);
        lsb = 0;
        glyph.xMin = glyph.xMax = glyph.yMin = glyph.yMax = 0;
        glyph.numContours = 0;
        glyph.numPoints = 0;
        return glyph;
    }

    convertPath(path, &points, &endPoints, ppem);

//     qDebug() << "number of contours=" << endPoints.size();
//     for (int i = 0; i < points.size(); ++i)
//         qDebug() << "  point[" << i << "] = " << QPoint(points.at(i).x, points.at(i).y) << " flags=" << points.at(i).flags;
//     qDebug() << "endPoints:";
//     for (int i = 0; i < endPoints.size(); ++i)
//         qDebug() << endPoints.at(i);

    getBounds(points, &glyph.xMin, &glyph.xMax, &glyph.yMin, &glyph.yMax);
    int point_array_size = convertToRelative(&points);
    getGlyphData(&glyph, points, endPoints, point_array_size);
    return glyph;
}

Q_STATIC_GLOBAL_OPERATOR bool operator <(const QTtfGlyph &g1, const QTtfGlyph &g2)
{
    return g1.index < g2.index;
}

static QList<QTtfTable> generateGlyphTables(qttf_font_tables &tables, const QList<QTtfGlyph> &_glyphs)
{
    const int max_size_small = 65536*2;
    QList<QTtfGlyph> glyphs = _glyphs;
    std::sort(glyphs.begin(), glyphs.end());

    Q_ASSERT(tables.maxp.numGlyphs == glyphs.at(glyphs.size()-1).index + 1);
    int nGlyphs = tables.maxp.numGlyphs;

    int glyf_size = 0;
    for (int i = 0; i < glyphs.size(); ++i)
        glyf_size += (glyphs.at(i).data.size() + 3) & ~3;

    tables.head.indexToLocFormat = glyf_size < max_size_small ? 0 : 1;
    tables.hhea.numberOfHMetrics = nGlyphs;

    QTtfTable glyf;
    glyf.tag = MAKE_TAG('g', 'l', 'y', 'f');

    QTtfTable loca;
    loca.tag = MAKE_TAG('l', 'o', 'c', 'a');
    loca.data.resize(glyf_size < max_size_small ? (nGlyphs+1)*sizeof(quint16) : (nGlyphs+1)*sizeof(quint32));
    QTtfStream ls(loca.data);

    QTtfTable hmtx;
    hmtx.tag = MAKE_TAG('h', 'm', 't', 'x');
    hmtx.data.resize(nGlyphs*4);
    QTtfStream hs(hmtx.data);

    int pos = 0;
    for (int i = 0; i < nGlyphs; ++i) {
        int gpos = glyf.data.size();
        quint16 advance = 0;
        qint16 lsb = 0;

        if (glyphs[pos].index == i) {
            // emit glyph
//             qDebug("emitting glyph %d: size=%d", i, glyphs.at(i).data.size());
            glyf.data += glyphs.at(pos).data;
            while (glyf.data.size() & 1)
                glyf.data.append('\0');
            advance = glyphs.at(pos).advanceWidth;
            lsb = glyphs.at(pos).lsb;
            ++pos;
        }
        if (glyf_size < max_size_small) {
            // use short loca format
            ls << quint16(gpos>>1);
        } else {
            // use long loca format
            ls << quint32(gpos);
        }
        hs << advance
           << lsb;
    }
    if (glyf_size < max_size_small) {
        // use short loca format
        ls << quint16(glyf.data.size()>>1);
    } else {
        // use long loca format
        ls << quint32(glyf.data.size());
    }

    Q_ASSERT(loca.data.size() == ls.offset());
    Q_ASSERT(hmtx.data.size() == hs.offset());

    QList<QTtfTable> list;
    list.append(glyf);
    list.append(loca);
    list.append(hmtx);
    return list;
}

Q_STATIC_GLOBAL_OPERATOR bool operator <(const QTtfTable &t1, const QTtfTable &t2)
{
    return t1.tag < t2.tag;
}

static QByteArray bindFont(const QList<QTtfTable>& _tables)
{
    QList<QTtfTable> tables = _tables;

    std::sort(tables.begin(), tables.end());

    QByteArray font;
    const int header_size = sizeof(qint32) + 4*sizeof(quint16);
    const int directory_size = 4*sizeof(quint32)*tables.size();
    font.resize(header_size + directory_size);

    int log2 = 0;
    int pow = 1;
    int n = tables.size() >> 1;
    while (n) {
        ++log2;
        pow <<= 1;
        n >>= 1;
    }

    quint32 head_offset = 0;
    {
        QTtfStream f(font);
// Offset Table
// Type  Name  Description
//   qint32  sfnt version  0x00010000 for version 1.0.
//   quint16   numTables  Number of tables.
//   quint16   searchRange  (Maximum power of 2 <= numTables) x 16.
//   quint16   entrySelector  Log2(maximum power of 2 <= numTables).
//   quint16   rangeShift  NumTables x 16-searchRange.
        f << qint32(0x00010000)
          << quint16(tables.size())
          << quint16(16*pow)
          << quint16(log2)
          << quint16(16*(tables.size() - pow));

// Table Directory
// Type  Name  Description
//   quint32  tag  4 -byte identifier.
//   quint32  checkSum  CheckSum for this table.
//   quint32  offset  Offset from beginning of TrueType font file.
//   quint32  length  Length of this table.
        quint32 table_offset = header_size + directory_size;
        for (int i = 0; i < tables.size(); ++i) {
            const QTtfTable &t = tables.at(i);
            const quint32 size = (t.data.size() + 3) & ~3;
            if (t.tag == MAKE_TAG('h', 'e', 'a', 'd'))
                head_offset = table_offset;
            f << t.tag
              << checksum(t.data)
              << table_offset
              << t.data.size();
            table_offset += size;
#define TAG(x) char(t.tag >> 24) << char((t.tag >> 16) & 0xff) << char((t.tag >> 8) & 0xff) << char(t.tag & 0xff)
            //qDebug() << "table " << TAG(t.tag) << "has size " << t.data.size() << "stream at " << f.offset();
        }
    }
    for (int i = 0; i < tables.size(); ++i) {
        const QByteArray &t = tables.at(i).data;
        font += t;
        int s = t.size();
        while (s & 3) { font += '\0'; ++s; }
    }

    if (!head_offset) {
        qWarning("QFontSubset: Font misses 'head' table");
        return QByteArray();
    }

    // calculate the fonts checksum and qToBigEndian into 'head's checksum_adjust
    quint32 checksum_adjust = 0xB1B0AFBA - checksum(font);
    qToBigEndian(checksum_adjust, (uchar *)font.data() + head_offset + 8);

    return font;
}


/*
  PDF requires the following tables:

  head, hhea, loca, maxp, cvt , prep, glyf, hmtx, fpgm

  This means we don't have to add a os/2, post or name table. cvt , prep and fpgm could be empty
  if really required.
*/

QByteArray QFontSubset::toTruetype() const
{
    qttf_font_tables font;
    memset(&font, 0, sizeof(qttf_font_tables));

    qreal ppem = fontEngine->fontDef.pixelSize;
#define TO_TTF(x) qRound(x * 2048. / ppem)
    QList<QTtfGlyph> glyphs;

    QFontEngine::Properties properties = fontEngine->properties();
    // initialize some stuff needed in createWidthArray
    emSquare = 2048;
    widths.resize(nGlyphs());

    // head table
    font.head.font_revision = 0x00010000;
    font.head.flags = (1 << 2) | (1 << 4);
    font.head.created = 0; // ###
    font.head.modified = 0; // ###
    font.head.xMin = SHRT_MAX;
    font.head.xMax = SHRT_MIN;
    font.head.yMin = SHRT_MAX;
    font.head.yMax = SHRT_MIN;
    font.head.macStyle = (fontEngine->fontDef.weight > QFont::Normal) ? 1 : 0;
    font.head.macStyle |= (fontEngine->fontDef.styleHint != QFont::StyleNormal) ? 1 : 0;

    // hhea table
    font.hhea.ascender = qRound(properties.ascent);
    font.hhea.descender = -qRound(properties.descent);
    font.hhea.lineGap = qRound(properties.leading);
    font.hhea.maxAdvanceWidth = TO_TTF(fontEngine->maxCharWidth());
    font.hhea.minLeftSideBearing = TO_TTF(fontEngine->minLeftBearing());
    font.hhea.minRightSideBearing = TO_TTF(fontEngine->minRightBearing());
    font.hhea.xMaxExtent = SHRT_MIN;

    font.maxp.numGlyphs = 0;
    font.maxp.maxPoints = 0;
    font.maxp.maxContours = 0;
    font.maxp.maxCompositePoints = 0;
    font.maxp.maxCompositeContours = 0;
    font.maxp.maxComponentElements = 0;
    font.maxp.maxComponentDepth = 0;
    font.maxp.numGlyphs = nGlyphs();



    uint sumAdvances = 0;
    for (int i = 0; i < nGlyphs(); ++i) {
        glyph_t g = glyph_indices.at(i);
        QPainterPath path;
        glyph_metrics_t metric;
        fontEngine->getUnscaledGlyph(g, &path, &metric);
        if (noEmbed) {
            path = QPainterPath();
            if (g == 0)
                path.addRect(QRectF(0, 0, 1000, 1000));
        }
        QTtfGlyph glyph = generateGlyph(i, path, metric.xoff.toReal(), metric.x.toReal(), properties.emSquare.toReal());

        font.head.xMin = qMin(font.head.xMin, glyph.xMin);
        font.head.xMax = qMax(font.head.xMax, glyph.xMax);
        font.head.yMin = qMin(font.head.yMin, glyph.yMin);
        font.head.yMax = qMax(font.head.yMax, glyph.yMax);

        font.hhea.xMaxExtent = qMax(font.hhea.xMaxExtent, (qint16)(glyph.lsb + glyph.xMax - glyph.xMin));

        font.maxp.maxPoints = qMax(font.maxp.maxPoints, glyph.numPoints);
        font.maxp.maxContours = qMax(font.maxp.maxContours, glyph.numContours);

        if (glyph.xMax > glyph.xMin)
            sumAdvances += glyph.xMax - glyph.xMin;

//         qDebug("adding glyph %d size=%d", glyph.index, glyph.data.size());
        glyphs.append(glyph);
        widths[i] = glyph.advanceWidth;
    }


    QList<QTtfTable> tables = generateGlyphTables(font, glyphs);
    tables.append(generateHead(font.head));
    tables.append(generateHhea(font.hhea));
    tables.append(generateMaxp(font.maxp));
    // name
    QTtfTable name_table;
    name_table.tag = MAKE_TAG('n', 'a', 'm', 'e');
    if (!noEmbed)
        name_table.data = fontEngine->getSfntTable(name_table.tag);
    if (name_table.data.isEmpty()) {
        qttf_name_table name;
        if (noEmbed)
            name.copyright = QLatin1String("Fake font");
        else
            name.copyright = QLatin1String(properties.copyright);
        name.family = fontEngine->fontDef.family;
        name.subfamily = QLatin1String("Regular"); // ######
        name.postscript_name = QLatin1String(properties.postscriptName);
        name_table = generateName(name);
    }
    tables.append(name_table);

    if (!noEmbed) {
        QTtfTable os2;
        os2.tag = MAKE_TAG('O', 'S', '/', '2');
        os2.data = fontEngine->getSfntTable(os2.tag);
        if (!os2.data.isEmpty())
            tables.append(os2);
    }

    return bindFont(tables);
}

QT_END_NAMESPACE
