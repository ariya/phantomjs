/*
 * Copyright (C) 2008 Nokia Corporation and/or its subsidiary(-ies)
 *
 * This is part of HarfBuzz, an OpenType Layout engine library.
 *
 * Permission is hereby granted, without written agreement and without
 * license or royalty fees, to use, copy, modify, and distribute this
 * software and its documentation for any purpose, provided that the
 * above copyright notice and the following two paragraphs appear in
 * all copies of this software.
 *
 * IN NO EVENT SHALL THE COPYRIGHT HOLDER BE LIABLE TO ANY PARTY FOR
 * DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES
 * ARISING OUT OF THE USE OF THIS SOFTWARE AND ITS DOCUMENTATION, EVEN
 * IF THE COPYRIGHT HOLDER HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 *
 * THE COPYRIGHT HOLDER SPECIFICALLY DISCLAIMS ANY WARRANTIES, INCLUDING,
 * BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS FOR A PARTICULAR PURPOSE.  THE SOFTWARE PROVIDED HEREUNDER IS
 * ON AN "AS IS" BASIS, AND THE COPYRIGHT HOLDER HAS NO OBLIGATION TO
 * PROVIDE MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.
 */

#include <QtTest/QtTest>

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_TRUETYPE_TABLES_H

#include <harfbuzz-shaper.h>
#include <harfbuzz-global.h>
#include <harfbuzz-gpos.h>

static FT_Library freetype;

static FT_Face loadFace(const char *name)
{
    FT_Face face;
    char path[256];

    strcpy(path, SRCDIR);
    strcat(path, "/fonts/");
    strcat(path, name);

    if (FT_New_Face(freetype, path, /*index*/0, &face))
        return 0;
    return face;
}

static HB_UChar32 getChar(const HB_UChar16 *string, hb_uint32 length, hb_uint32 &i)
{
    HB_UChar32 ch;
    if (HB_IsHighSurrogate(string[i])
        && i < length - 1
        && HB_IsLowSurrogate(string[i + 1])) {
        ch = HB_SurrogateToUcs4(string[i], string[i + 1]);
        ++i;
    } else {
        ch = string[i];
    }
    return ch;
}

static HB_Bool hb_stringToGlyphs(HB_Font font, const HB_UChar16 *string, hb_uint32 length, HB_Glyph *glyphs, hb_uint32 *numGlyphs, HB_Bool /*rightToLeft*/)
{
    FT_Face face = (FT_Face)font->userData;
    if (length > *numGlyphs)
        return false;

    int glyph_pos = 0;
    for (hb_uint32 i = 0; i < length; ++i) {
        glyphs[glyph_pos] = FT_Get_Char_Index(face, getChar(string, length, i));
        ++glyph_pos;
    }

    *numGlyphs = glyph_pos;

    return true;
}

static void hb_getAdvances(HB_Font /*font*/, const HB_Glyph * /*glyphs*/, hb_uint32 numGlyphs, HB_Fixed *advances, int /*flags*/)
{
    for (hb_uint32 i = 0; i < numGlyphs; ++i)
        advances[i] = 0; // ### not tested right now
}

static HB_Bool hb_canRender(HB_Font font, const HB_UChar16 *string, hb_uint32 length)
{
    FT_Face face = (FT_Face)font->userData;

    for (hb_uint32 i = 0; i < length; ++i)
        if (!FT_Get_Char_Index(face, getChar(string, length, i)))
            return false;

    return true;
}

static HB_Error hb_getSFntTable(void *font, HB_Tag tableTag, HB_Byte *buffer, HB_UInt *length)
{
    FT_Face face = (FT_Face)font;
    FT_ULong ftlen = *length;
    FT_Error error = 0;

    if (!FT_IS_SFNT(face))
        return HB_Err_Invalid_Argument;

    error = FT_Load_Sfnt_Table(face, tableTag, 0, buffer, &ftlen);
    *length = ftlen;
    return (HB_Error)error;
}

HB_Error hb_getPointInOutline(HB_Font font, HB_Glyph glyph, int flags, hb_uint32 point, HB_Fixed *xpos, HB_Fixed *ypos, hb_uint32 *nPoints)
{
    HB_Error error = HB_Err_Ok;
    FT_Face face = (FT_Face)font->userData;

    int load_flags = (flags & HB_ShaperFlag_UseDesignMetrics) ? FT_LOAD_NO_HINTING : FT_LOAD_DEFAULT;

    if ((error = (HB_Error)FT_Load_Glyph(face, glyph, load_flags)))
        return error;

    if (face->glyph->format != ft_glyph_format_outline)
        return (HB_Error)HB_Err_Invalid_SubTable;

    *nPoints = face->glyph->outline.n_points;
    if (!(*nPoints))
        return HB_Err_Ok;

    if (point > *nPoints)
        return (HB_Error)HB_Err_Invalid_SubTable;

    *xpos = face->glyph->outline.points[point].x;
    *ypos = face->glyph->outline.points[point].y;

    return HB_Err_Ok;
}

void hb_getGlyphMetrics(HB_Font, HB_Glyph, HB_GlyphMetrics *metrics)
{
    // ###
    metrics->x = metrics->y = metrics->width = metrics->height = metrics->xOffset = metrics->yOffset = 0;
}

HB_Fixed hb_getFontMetric(HB_Font, HB_FontMetric )
{
    return 0; // ####
}

const HB_FontClass hb_fontClass = {
    hb_stringToGlyphs, hb_getAdvances, hb_canRender,
    hb_getPointInOutline, hb_getGlyphMetrics, hb_getFontMetric
};


//TESTED_CLASS=
//TESTED_FILES= gui/text/qscriptengine.cpp

class tst_QScriptEngine : public QObject
{
Q_OBJECT

public:
    tst_QScriptEngine();
    virtual ~tst_QScriptEngine();


public slots:
    void initTestCase();
    void cleanupTestCase();
private slots:
    void greek();

    void devanagari();
    void bengali();
    void gurmukhi();
    // gujarati missing
    void oriya();
    void tamil();
    void telugu();
    void kannada();
    void malayalam();
    void sinhala();

    void khmer();
    void nko();
    void linearB();
};

tst_QScriptEngine::tst_QScriptEngine()
{
}

tst_QScriptEngine::~tst_QScriptEngine()
{
}

void tst_QScriptEngine::initTestCase()
{
    FT_Init_FreeType(&freetype);
}

void tst_QScriptEngine::cleanupTestCase()
{
    FT_Done_FreeType(freetype);
}

class Shaper
{
public:
    Shaper(FT_Face face, HB_Script script, const QString &str);

    HB_FontRec hbFont;
    HB_ShaperItem shaper_item;
    QVarLengthArray<HB_Glyph> hb_glyphs;
    QVarLengthArray<HB_GlyphAttributes> hb_attributes;
    QVarLengthArray<HB_Fixed> hb_advances;
    QVarLengthArray<HB_FixedPoint> hb_offsets;
    QVarLengthArray<unsigned short> hb_logClusters;

};

Shaper::Shaper(FT_Face face, HB_Script script, const QString &str)
{
    HB_Face hbFace = HB_NewFace(face, hb_getSFntTable);

    hbFont.klass = &hb_fontClass;
    hbFont.userData = face;
    hbFont.x_ppem  = face->size->metrics.x_ppem;
    hbFont.y_ppem  = face->size->metrics.y_ppem;
    hbFont.x_scale = face->size->metrics.x_scale;
    hbFont.y_scale = face->size->metrics.y_scale;

    shaper_item.kerning_applied = false;
    shaper_item.string = reinterpret_cast<const HB_UChar16 *>(str.constData());
    shaper_item.stringLength = str.length();
    shaper_item.item.script = script;
    shaper_item.item.pos = 0;
    shaper_item.item.length = shaper_item.stringLength;
    shaper_item.item.bidiLevel = 0; // ###
    shaper_item.shaperFlags = 0;
    shaper_item.font = &hbFont;
    shaper_item.face = hbFace;
    shaper_item.num_glyphs = shaper_item.item.length;
    shaper_item.glyphIndicesPresent = false;
    shaper_item.initialGlyphCount = 0;


    while (1) {
        hb_glyphs.resize(shaper_item.num_glyphs);
        hb_attributes.resize(shaper_item.num_glyphs);
        hb_advances.resize(shaper_item.num_glyphs);
        hb_offsets.resize(shaper_item.num_glyphs);
        hb_logClusters.resize(shaper_item.num_glyphs);

        memset(hb_glyphs.data(), 0, hb_glyphs.size() * sizeof(HB_Glyph));
        memset(hb_attributes.data(), 0, hb_attributes.size() * sizeof(HB_GlyphAttributes));
        memset(hb_advances.data(), 0, hb_advances.size() * sizeof(HB_Fixed));
        memset(hb_offsets.data(), 0, hb_offsets.size() * sizeof(HB_FixedPoint));

        shaper_item.glyphs = hb_glyphs.data();
        shaper_item.attributes = hb_attributes.data();
        shaper_item.advances = hb_advances.data();
        shaper_item.offsets = hb_offsets.data();
        shaper_item.log_clusters = hb_logClusters.data();

        if (HB_ShapeItem(&shaper_item))
            break;
    }

    HB_FreeFace(hbFace);
}


static bool decomposedShaping(FT_Face face, HB_Script script, const QChar &ch)
{
    QString uc = QString().append(ch);
    Shaper shaper(face, script, uc);

    uc = uc.normalized(QString::NormalizationForm_D);
    Shaper decomposed(face, script, uc);

    if( shaper.shaper_item.num_glyphs != decomposed.shaper_item.num_glyphs )
        goto error;

    for (unsigned int i = 0; i < shaper.shaper_item.num_glyphs; ++i) {
        if ((shaper.shaper_item.glyphs[i]&0xffffff) != (decomposed.shaper_item.glyphs[i]&0xffffff))
            goto error;
    }
    return true;
 error:
    QString str = "";
    int i = 0;
    while (i < uc.length()) {
        str += QString("%1 ").arg(uc[i].unicode(), 4, 16);
        ++i;
    }
    qDebug("%s: decomposedShaping of char %4x failed\n    decomposedString: %s\n   nglyphs=%d, decomposed nglyphs %d",
           face->family_name,
           ch.unicode(), str.toLatin1().data(),
           shaper.shaper_item.num_glyphs,
           decomposed.shaper_item.num_glyphs);

    str = "";
    i = 0;
    while (i < shaper.shaper_item.num_glyphs) {
        str += QString("%1 ").arg(shaper.shaper_item.glyphs[i], 4, 16);
        ++i;
    }
    qDebug("    composed glyph result   = %s", str.toLatin1().constData());
    str = "";
    i = 0;
    while (i < decomposed.shaper_item.num_glyphs) {
        str += QString("%1 ").arg(decomposed.shaper_item.glyphs[i], 4, 16);
        ++i;
    }
    qDebug("    decomposed glyph result = %s", str.toLatin1().constData());
    return false;
}

struct ShapeTable {
    unsigned short unicode[16];
    unsigned short glyphs[16];
};

static bool shaping(FT_Face face, const ShapeTable *s, HB_Script script)
{
    Shaper shaper(face, script, QString::fromUtf16( s->unicode ));

    hb_uint32 nglyphs = 0;
    const unsigned short *g = s->glyphs;
    while ( *g ) {
	nglyphs++;
	g++;
    }

    if( nglyphs != shaper.shaper_item.num_glyphs )
	goto error;

    for (hb_uint32 i = 0; i < nglyphs; ++i) {
        if ((shaper.shaper_item.glyphs[i]&0xffffff) != s->glyphs[i])
	    goto error;
    }
    return true;
 error:
    QString str = "";
    const unsigned short *uc = s->unicode;
    while (*uc) {
	str += QString("%1 ").arg(*uc, 4, 16);
	++uc;
    }
    qDebug("%s: shaping of string %s failed, nglyphs=%d, expected %d",
           face->family_name,
           str.toLatin1().constData(),
           shaper.shaper_item.num_glyphs, nglyphs);

    str = "";
    hb_uint32 i = 0;
    while (i < shaper.shaper_item.num_glyphs) {
        str += QString("%1 ").arg(shaper.shaper_item.glyphs[i], 4, 16);
	++i;
    }
    qDebug("    glyph result = %s", str.toLatin1().constData());
    return false;
}


void tst_QScriptEngine::greek()
{
    FT_Face face = loadFace("DejaVuSans.ttf");
    if (face) {
        for (int uc = 0x1f00; uc <= 0x1fff; ++uc) {
            QString str;
            str.append(uc);
            if (str.normalized(QString::NormalizationForm_D).normalized(QString::NormalizationForm_C) != str) {
                //qDebug() << "skipping" << hex << uc;
                continue;
            }
            if (uc == 0x1fc1 || uc == 0x1fed)
                continue;
            QVERIFY( decomposedShaping(face, HB_Script_Greek, QChar(uc)) );
        }
        FT_Done_Face(face);
    } else {
        QSKIP("couln't find DejaVu Sans", SkipAll);
    }


    face = loadFace("SBL_grk.ttf");
    if (face) {
        for (int uc = 0x1f00; uc <= 0x1fff; ++uc) {
            QString str;
            str.append(uc);
            if (str.normalized(QString::NormalizationForm_D).normalized(QString::NormalizationForm_C) != str) {
                //qDebug() << "skipping" << hex << uc;
                continue;
            }
            if (uc == 0x1fc1 || uc == 0x1fed)
                continue;
            QVERIFY( decomposedShaping(face, HB_Script_Greek, QChar(uc)) );

        }

        const ShapeTable shape_table [] = {
            { { 0x3b1, 0x300, 0x313, 0x0 },
              { 0xb8, 0x3d3, 0x3c7, 0x0 } },
            { { 0x3b1, 0x313, 0x300, 0x0 },
              { 0xd4, 0x0 } },

            { {0}, {0} }
        };


        const ShapeTable *s = shape_table;
        while (s->unicode[0]) {
            QVERIFY( shaping(face, s, HB_Script_Greek) );
            ++s;
        }

        FT_Done_Face(face);
    } else {
        QSKIP("couln't find DejaVu Sans", SkipAll);
    }
}


void tst_QScriptEngine::devanagari()
{
    {
        FT_Face face = loadFace("raghu.ttf");
        if (face) {
	    const ShapeTable shape_table [] = {
		// Ka
		{ { 0x0915, 0x0 },
		  { 0x0080, 0x0 } },
		// Ka Halant
		{ { 0x0915, 0x094d, 0x0 },
		  { 0x0080, 0x0051, 0x0 } },
		// Ka Halant Ka
		{ { 0x0915, 0x094d, 0x0915, 0x0 },
		  { 0x00c8, 0x0080, 0x0 } },
		// Ka MatraI
		{ { 0x0915, 0x093f, 0x0 },
		  { 0x01d1, 0x0080, 0x0 } },
		// Ra Halant Ka
		{ { 0x0930, 0x094d, 0x0915, 0x0 },
		  { 0x0080, 0x005b, 0x0 } },
		// Ra Halant Ka MatraI
		{ { 0x0930, 0x094d, 0x0915, 0x093f, 0x0 },
		  { 0x01d1, 0x0080, 0x005b, 0x0 } },
		// MatraI
		{ { 0x093f, 0x0 },
		  { 0x01d4, 0x029c, 0x0 } },
		// Ka Nukta
		{ { 0x0915, 0x093c, 0x0 },
		  { 0x00a4, 0x0 } },
		// Ka Halant Ra
		{ { 0x0915, 0x094d, 0x0930, 0x0 },
		  { 0x0110, 0x0 } },
		// Ka Halant Ra Halant Ka
		{ { 0x0915, 0x094d, 0x0930, 0x094d, 0x0915, 0x0 },
		  { 0x0158, 0x0080, 0x0 } },
		{ { 0x0930, 0x094d, 0x200d, 0x0 },
		  { 0x00e2, 0x0 } },
		{ { 0x0915, 0x094d, 0x0930, 0x094d, 0x200d, 0x0 },
		  { 0x0158, 0x0 } },

		{ {0}, {0} }
	    };


	    const ShapeTable *s = shape_table;
	    while (s->unicode[0]) {
		QVERIFY( shaping(face, s, HB_Script_Devanagari) );
		++s;
	    }

            FT_Done_Face(face);
	} else {
	    QSKIP("couln't find raghu.ttf", SkipAll);
	}
    }

    {
        FT_Face face = loadFace("mangal.ttf");
        if (face) {
	    const ShapeTable shape_table [] = {
		// Ka
		{ { 0x0915, 0x0 },
		  { 0x0080, 0x0 } },
		// Ka Halant
		{ { 0x0915, 0x094d, 0x0 },
		  { 0x0080, 0x0051, 0x0 } },
		// Ka Halant Ka
		{ { 0x0915, 0x094d, 0x0915, 0x0 },
		  { 0x00c8, 0x0080, 0x0 } },
		// Ka MatraI
		{ { 0x0915, 0x093f, 0x0 },
		  { 0x01d1, 0x0080, 0x0 } },
		// Ra Halant Ka
		{ { 0x0930, 0x094d, 0x0915, 0x0 },
		  { 0x0080, 0x005b, 0x0 } },
		// Ra Halant Ka MatraI
		{ { 0x0930, 0x094d, 0x0915, 0x093f, 0x0 },
		  { 0x01d1, 0x0080, 0x005b, 0x0 } },
		// MatraI
		{ { 0x093f, 0x0 },
		  { 0x01d4, 0x029c, 0x0 } },
		// Ka Nukta
		{ { 0x0915, 0x093c, 0x0 },
		  { 0x00a4, 0x0 } },
		// Ka Halant Ra
		{ { 0x0915, 0x094d, 0x0930, 0x0 },
		  { 0x0110, 0x0 } },
		// Ka Halant Ra Halant Ka
		{ { 0x0915, 0x094d, 0x0930, 0x094d, 0x0915, 0x0 },
		  { 0x0158, 0x0080, 0x0 } },

                { { 0x92b, 0x94d, 0x930, 0x0 },
                  { 0x125, 0x0 } },
                { { 0x92b, 0x93c, 0x94d, 0x930, 0x0 },
                  { 0x149, 0x0 } }, 
		{ {0}, {0} }
	    };

	    const ShapeTable *s = shape_table;
	    while (s->unicode[0]) {
		QVERIFY( shaping(face, s, HB_Script_Devanagari) );
		++s;
	    }

            FT_Done_Face(face);
	} else {
	    QSKIP("couldn't find mangal.ttf", SkipAll);
	}
    }
}

void tst_QScriptEngine::bengali()
{
    {
        FT_Face face = loadFace("AkaashNormal.ttf");
        if (face) {
	    const ShapeTable shape_table [] = {
		// Ka
		{ { 0x0995, 0x0 },
		  { 0x0151, 0x0 } },
		// Ka Halant
		{ { 0x0995, 0x09cd, 0x0 },
		  { 0x0151, 0x017d, 0x0 } },
		// Ka Halant Ka
		{ { 0x0995, 0x09cd, 0x0995, 0x0 },
		  { 0x019b, 0x0 } },
		// Ka MatraI
		{ { 0x0995, 0x09bf, 0x0 },
		  { 0x0173, 0x0151, 0x0 } },
		// Ra Halant Ka
		{ { 0x09b0, 0x09cd, 0x0995, 0x0 },
		  { 0x0151, 0x0276, 0x0 } },
		// Ra Halant Ka MatraI
		{ { 0x09b0, 0x09cd, 0x0995, 0x09bf, 0x0 },
		  { 0x0173, 0x0151, 0x0276, 0x0 } },
		// Ka Nukta
		{ { 0x0995, 0x09bc, 0x0 },
		  { 0x0151, 0x0171, 0x0 } },
		// Ka Halant Ra
		{ { 0x0995, 0x09cd, 0x09b0, 0x0 },
		  { 0x01f4, 0x0 } },
		// Ka Halant Ra Halant Ka
		{ { 0x0995, 0x09cd, 0x09b0, 0x09cd, 0x0995, 0x0 },
		  { 0x025c, 0x0276, 0x0151, 0x0 } },
		// Ya + Halant
		{ { 0x09af, 0x09cd, 0x0 },
		  { 0x016a, 0x017d, 0x0 } },
		// Da Halant Ya -> Da Ya-Phala
		{ { 0x09a6, 0x09cd, 0x09af, 0x0 },
		  { 0x01e5, 0x0 } },
		// A Halant Ya -> A Ya-phala
		{ { 0x0985, 0x09cd, 0x09af, 0x0 },
		  { 0x0145, 0x01cf, 0x0 } },
		// Na Halant Ka
		{ { 0x09a8, 0x09cd, 0x0995, 0x0 },
		  { 0x026f, 0x0151, 0x0 } },
		// Na Halant ZWNJ Ka
		{ { 0x09a8, 0x09cd, 0x200c, 0x0995, 0x0 },
		  { 0x0164, 0x017d, 0x0151, 0x0 } },
		// Na Halant ZWJ Ka
		{ { 0x09a8, 0x09cd, 0x200d, 0x0995, 0x0 },
		  { 0x026f, 0x0151, 0x0 } },
		// Ka Halant ZWNJ Ka
		{ { 0x0995, 0x09cd, 0x200c, 0x0995, 0x0 },
		  { 0x0151, 0x017d, 0x0151, 0x0 } },
		// Ka Halant ZWJ Ka
		{ { 0x0995, 0x09cd, 0x200d, 0x0995, 0x0 },
		  { 0x025c, 0x0151, 0x0 } },
		// Na Halant Ra
		{ { 0x09a8, 0x09cd, 0x09b0, 0x0 },
		  { 0x0207, 0x0 } },
		// Na Halant ZWNJ Ra
		{ { 0x09a8, 0x09cd, 0x200c, 0x09b0, 0x0 },
		  { 0x0164, 0x017d, 0x016b, 0x0 } },
		// Na Halant ZWJ Ra
		{ { 0x09a8, 0x09cd, 0x200d, 0x09b0, 0x0 },
		  { 0x026f, 0x016b, 0x0 } },
		// Na Halant Ba
		{ { 0x09a8, 0x09cd, 0x09ac, 0x0 },
		  { 0x022f, 0x0 } },
		// Na Halant ZWNJ Ba
		{ { 0x09a8, 0x09cd, 0x200c, 0x09ac, 0x0 },
		  { 0x0164, 0x017d, 0x0167, 0x0 } },
		// Na Halant ZWJ Ba
		{ { 0x09a8, 0x09cd, 0x200d, 0x09ac, 0x0 },
		  { 0x026f, 0x0167, 0x0 } },
		// Na Halant Dha
		{ { 0x09a8, 0x09cd, 0x09a7, 0x0 },
		  { 0x01d3, 0x0 } },
		// Na Halant ZWNJ Dha
		{ { 0x09a8, 0x09cd, 0x200c, 0x09a7, 0x0 },
		  { 0x0164, 0x017d, 0x0163, 0x0 } },
		// Na Halant ZWJ Dha
		{ { 0x09a8, 0x09cd, 0x200d, 0x09a7, 0x0 },
		  { 0x026f, 0x0163, 0x0 } },
		// Ra Halant Ka MatraAU
		{ { 0x09b0, 0x09cd, 0x0995, 0x09cc, 0x0 },
		  { 0x0179, 0x0151, 0x0276, 0x017e, 0x0 } },
		// Ra Halant Ba Halant Ba
		{ { 0x09b0, 0x09cd, 0x09ac, 0x09cd, 0x09ac, 0x0 },
		  { 0x0232, 0x0276, 0x0 } },
                { { 0x9b0, 0x9cd, 0x995, 0x9be, 0x982, 0x0 },
                  { 0x151, 0x276, 0x172, 0x143, 0x0 } },
                { { 0x9b0, 0x9cd, 0x995, 0x9be, 0x983, 0x0 },
                  { 0x151, 0x276, 0x172, 0x144, 0x0 } }, 
                // test decomposed two parts matras
                { { 0x995, 0x9c7, 0x9be, 0x0 },
                  { 0x179, 0x151, 0x172, 0x0 } },
                { { 0x995, 0x9c7, 0x9d7, 0x0 },
                  { 0x179, 0x151, 0x17e, 0x0 } },
                { { 0x9b0, 0x9cd, 0x9ad, 0x0 },
                  { 0x168, 0x276, 0x0 } },
                { { 0x9f0, 0x9cd, 0x9ad, 0x0 },
                  { 0x168, 0x276, 0x0 } },
                { { 0x9f1, 0x9cd, 0x9ad, 0x0 },
                  { 0x191, 0x17d, 0x168, 0x0 } },

                // Ra ZWJ Halant Ya
                { { 0x09b0, 0x200d, 0x09cd, 0x09af, 0x0 },
                  { 0x016b, 0x01cf, 0x0 } },

		{ {0}, {0} }
	    };


	    const ShapeTable *s = shape_table;
	    while (s->unicode[0]) {
		QVERIFY( shaping(face, s, HB_Script_Bengali) );
		++s;
	    }

            FT_Done_Face(face);
	} else {
	    QSKIP("couln't find AkaashNormal.ttf", SkipAll);
	}
    }
    {
        FT_Face face = loadFace("MuktiNarrow.ttf");
        if (face) {
	    const ShapeTable shape_table [] = {
		// Ka
		{ { 0x0995, 0x0 },
		  { 0x0073, 0x0 } },
		// Ka Halant
		{ { 0x0995, 0x09cd, 0x0 },
		  { 0x00b9, 0x0 } },
		// Ka Halant Ka
		{ { 0x0995, 0x09cd, 0x0995, 0x0 },
		  { 0x0109, 0x0 } },
		// Ka MatraI
		{ { 0x0995, 0x09bf, 0x0 },
		  { 0x0095, 0x0073, 0x0 } },
		// Ra Halant Ka
		{ { 0x09b0, 0x09cd, 0x0995, 0x0 },
		  { 0x0073, 0x00e1, 0x0 } },
		// Ra Halant Ka MatraI
		{ { 0x09b0, 0x09cd, 0x0995, 0x09bf, 0x0 },
		  { 0x0095, 0x0073, 0x00e1, 0x0 } },
		// MatraI
 		{ { 0x09bf, 0x0 },
		  { 0x0095, 0x01c8, 0x0 } },
		// Ka Nukta
		{ { 0x0995, 0x09bc, 0x0 },
		  { 0x0073, 0x0093, 0x0 } },
		// Ka Halant Ra
		{ { 0x0995, 0x09cd, 0x09b0, 0x0 },
		  { 0x00e5, 0x0 } },
		// Ka Halant Ra Halant Ka
                { { 0x995, 0x9cd, 0x9b0, 0x9cd, 0x995, 0x0 },
                  { 0x234, 0x24e, 0x73, 0x0 } }, 
		// Ya + Halant
		{ { 0x09af, 0x09cd, 0x0 },
		  { 0x00d2, 0x0 } },
		// Da Halant Ya -> Da Ya-Phala
		{ { 0x09a6, 0x09cd, 0x09af, 0x0 },
		  { 0x0084, 0x00e2, 0x0 } },
		// A Halant Ya -> A Ya-phala
		{ { 0x0985, 0x09cd, 0x09af, 0x0 },
		  { 0x0067, 0x00e2, 0x0 } },
		// Na Halant Ka
		{ { 0x09a8, 0x09cd, 0x0995, 0x0 },
		  { 0x0188, 0x0 } },
		// Na Halant ZWNJ Ka
                { { 0x9a8, 0x9cd, 0x200c, 0x995, 0x0 },
                  { 0xcc, 0x73, 0x0 } }, 
		// Na Halant ZWJ Ka
                { { 0x9a8, 0x9cd, 0x200d, 0x995, 0x0 },
                  { 0x247, 0x73, 0x0 } }, 
		// Ka Halant ZWNJ Ka
                { { 0x9a8, 0x9cd, 0x200d, 0x995, 0x0 },
                  { 0x247, 0x73, 0x0 } }, 
		// Ka Halant ZWJ Ka
                { { 0x9a8, 0x9cd, 0x200d, 0x995, 0x0 },
                  { 0x247, 0x73, 0x0 } }, 
		// Na Halant Ra
		{ { 0x09a8, 0x09cd, 0x09b0, 0x0 },
		  { 0x00f8, 0x0 } },
		// Na Halant ZWNJ Ra
		{ { 0x09a8, 0x09cd, 0x200c, 0x09b0, 0x0 },
		  { 0xcc, 0x8d, 0x0 } },
		// Na Halant ZWJ Ra
                { { 0x9a8, 0x9cd, 0x200d, 0x9b0, 0x0 },
                  { 0x247, 0x8d, 0x0 } }, 
		// Na Halant Ba
		{ { 0x09a8, 0x09cd, 0x09ac, 0x0 },
		  { 0x0139, 0x0 } },
		// Na Halant ZWNJ Ba
                { { 0x9a8, 0x9cd, 0x200c, 0x9ac, 0x0 },
                  { 0xcc, 0x89, 0x0 } }, 
		// Na Halant ZWJ Ba
                { { 0x9a8, 0x9cd, 0x200d, 0x9ac, 0x0 },
                  { 0x247, 0x89, 0x0 } }, 
		// Na Halant Dha
		{ { 0x09a8, 0x09cd, 0x09a7, 0x0 },
		  { 0x0145, 0x0 } },
		// Na Halant ZWNJ Dha
		{ { 0x09a8, 0x09cd, 0x200c, 0x09a7, 0x0 },
		  { 0xcc, 0x85, 0x0 } },
		// Na Halant ZWJ Dha
		{ { 0x09a8, 0x09cd, 0x200d, 0x09a7, 0x0 },
		  { 0x247, 0x85, 0x0 } },
		// Ra Halant Ka MatraAU
                { { 0x9b0, 0x9cd, 0x995, 0x9cc, 0x0 },
                  { 0x232, 0x73, 0xe1, 0xa0, 0x0 } }, 
		// Ra Halant Ba Halant Ba
		{ { 0x09b0, 0x09cd, 0x09ac, 0x09cd, 0x09ac, 0x0 },
		  { 0x013b, 0x00e1, 0x0 } },

                // Init feature for vowel sign E should only be
                // applied when it's initial character (QTBUG-13620)
                { { 0x09a8, 0x09c7, 0x0 },
                  { 0x0232, 0x0086, 0x0 } },
                { { 0x09a8, 0x09a8, 0x09c7, 0x0 },
                  { 0x0086, 0x009b, 0x0086, 0x0 } },

		{ {0}, {0} }
	    };


	    const ShapeTable *s = shape_table;
	    while (s->unicode[0]) {
		QVERIFY( shaping(face, s, HB_Script_Bengali) );
		++s;
	    }

            FT_Done_Face(face);
	} else {
	    QSKIP("couln't find MuktiNarrow.ttf", SkipAll);
	}
    }
    {
        FT_Face face = loadFace("LikhanNormal.ttf");
        if (face) {
	    const ShapeTable shape_table [] = {
		{ { 0x09a8, 0x09cd, 0x09af, 0x0 },
                  { 0x01ca, 0x0 } },
		{ { 0x09b8, 0x09cd, 0x09af, 0x0 },
                  { 0x020e, 0x0 } },
		{ { 0x09b6, 0x09cd, 0x09af, 0x0 },
                  { 0x01f4, 0x0 } },
		{ { 0x09b7, 0x09cd, 0x09af, 0x0 },
                  { 0x01fe, 0x0 } },
		{ { 0x09b0, 0x09cd, 0x09a8, 0x09cd, 0x200d, 0x0 },
                  { 0x10b, 0x167, 0x0 } },
                { { 0x9b0, 0x9cd, 0x9ad, 0x0 },
                  { 0xa1, 0x167, 0x0 } },
                { { 0x9f0, 0x9cd, 0x9ad, 0x0 },
                  { 0xa1, 0x167, 0x0 } },
                { { 0x9f1, 0x9cd, 0x9ad, 0x0 },
                  { 0x11c, 0xa1, 0x0 } },

		{ {0}, {0} }
	    };


	    const ShapeTable *s = shape_table;
	    while (s->unicode[0]) {
		QVERIFY( shaping(face, s, HB_Script_Bengali) );
		++s;
	    }

            FT_Done_Face(face);
	} else {
	    QSKIP("couln't find LikhanNormal.ttf", SkipAll);
	}
    }
}

void tst_QScriptEngine::gurmukhi()
{
    {
        FT_Face face = loadFace("lohit_pa.ttf");
        if (face) {
	    const ShapeTable shape_table [] = {
		{ { 0xA15, 0xA4D, 0xa39, 0x0 },
		  { 0x3b, 0x8b, 0x0 } },
		{ {0}, {0} }
	    };


	    const ShapeTable *s = shape_table;
	    while (s->unicode[0]) {
		QVERIFY( shaping(face, s, HB_Script_Gurmukhi) );
		++s;
	    }

            FT_Done_Face(face);
	} else {
	    QSKIP("couln't find lohit.punjabi.1.1.ttf", SkipAll);
	}
    }
}

void tst_QScriptEngine::oriya()
{
    {
        FT_Face face = loadFace("utkalm.ttf");
        if (face) {
	    const ShapeTable shape_table [] = {
                { { 0xb15, 0xb4d, 0xb24, 0xb4d, 0xb30, 0x0 },
                  { 0x150, 0x125, 0x0 } }, 
                { { 0xb24, 0xb4d, 0xb24, 0xb4d, 0xb2c, 0x0 },
                  { 0x151, 0x120, 0x0 } }, 
                { { 0xb28, 0xb4d, 0xb24, 0xb4d, 0xb2c, 0x0 },
                  { 0x152, 0x120, 0x0 } }, 
                { { 0xb28, 0xb4d, 0xb24, 0xb4d, 0xb2c, 0x0 },
                  { 0x152, 0x120, 0x0 } }, 
                { { 0xb28, 0xb4d, 0xb24, 0xb4d, 0xb30, 0x0 },
                  { 0x176, 0x0 } }, 
                { { 0xb38, 0xb4d, 0xb24, 0xb4d, 0xb30, 0x0 },
                  { 0x177, 0x0 } }, 
                { { 0xb28, 0xb4d, 0xb24, 0xb4d, 0xb30, 0xb4d, 0xb2f, 0x0 },
                  { 0x176, 0x124, 0x0 } }, 

                // QTBUG-13542
                { { 0x0b2c, 0x0b4d, 0x0b21, 0x0 },
                  { 0x0089, 0x00fc, 0x0 } },
                { { 0x0b36, 0x0b4d, 0x0b2b, 0x0 },
                  { 0x0092, 0x0105, 0x0 } },
                { { 0x0b36, 0x0b4d, 0x0b1f, 0x0 },
                  { 0x0092, 0x00fa, 0x0 } },
                { { 0x0b39, 0x0b4d, 0x0b1f, 0x0 },
                  { 0x0095, 0x00fa, 0x0 } },
                { { 0x0b15, 0x0b4d, 0x0b16, 0x0 },
                  { 0x0073, 0x00f1, 0x0 } },

                { {0}, {0} }

            };

	    const ShapeTable *s = shape_table;
	    while (s->unicode[0]) {
		QVERIFY( shaping(face, s, HB_Script_Oriya) );
		++s;
	    }

            FT_Done_Face(face);
	} else {
	    QSKIP("couln't find utkalm.ttf", SkipAll);
	}
    }
}


void tst_QScriptEngine::tamil()
{
    {
        FT_Face face = loadFace("akruti1.ttf");
        if (face) {
	    const ShapeTable shape_table [] = {
		{ { 0x0b95, 0x0bc2, 0x0 },
		  { 0x004e, 0x0 } },
		{ { 0x0bae, 0x0bc2, 0x0 },
		  { 0x009e, 0x0 } },
		{ { 0x0b9a, 0x0bc2, 0x0 },
		  { 0x0058, 0x0 } },
		{ { 0x0b99, 0x0bc2, 0x0 },
		  { 0x0053, 0x0 } },
		{ { 0x0bb0, 0x0bc2, 0x0 },
		  { 0x00a8, 0x0 } },
		{ { 0x0ba4, 0x0bc2, 0x0 },
		  { 0x008e, 0x0 } },
		{ { 0x0b9f, 0x0bc2, 0x0 },
		  { 0x0062, 0x0 } },
		{ { 0x0b95, 0x0bc6, 0x0 },
		  { 0x000a, 0x0031, 0x0 } },
		{ { 0x0b95, 0x0bca, 0x0 },
		  { 0x000a, 0x0031, 0x0007, 0x0 } },
		{ { 0x0b95, 0x0bc6, 0x0bbe, 0x0 },
		  { 0x000a, 0x0031, 0x007, 0x0 } },
		{ { 0x0b95, 0x0bcd, 0x0bb7, 0x0 },
		  { 0x0049, 0x0 } },
		{ { 0x0b95, 0x0bcd, 0x0bb7, 0x0bca, 0x0 },
		  { 0x000a, 0x0049, 0x007, 0x0 } },
		{ { 0x0b95, 0x0bcd, 0x0bb7, 0x0bc6, 0x0bbe, 0x0 },
		  { 0x000a, 0x0049, 0x007, 0x0 } },
		{ { 0x0b9f, 0x0bbf, 0x0 },
		  { 0x005f, 0x0 } },
		{ { 0x0b9f, 0x0bc0, 0x0 },
		  { 0x0060, 0x0 } },
		{ { 0x0bb2, 0x0bc0, 0x0 },
		  { 0x00ab, 0x0 } },
		{ { 0x0bb2, 0x0bbf, 0x0 },
		  { 0x00aa, 0x0 } },
		{ { 0x0bb0, 0x0bcd, 0x0 },
		  { 0x00a4, 0x0 } },
		{ { 0x0bb0, 0x0bbf, 0x0 },
		  { 0x00a5, 0x0 } },
		{ { 0x0bb0, 0x0bc0, 0x0 },
		  { 0x00a6, 0x0 } },
		{ { 0x0b83, 0x0 },
		  { 0x0025, 0x0 } },
		{ { 0x0b83, 0x0b95, 0x0 },
		  { 0x0025, 0x0031, 0x0 } },

		{ {0}, {0} }
	    };


	    const ShapeTable *s = shape_table;
	    while (s->unicode[0]) {
		QVERIFY( shaping(face, s, HB_Script_Tamil) );
		++s;
	    }

            FT_Done_Face(face);
	} else {
	    QSKIP("couln't find akruti1.ttf", SkipAll);
	}
    }
}


void tst_QScriptEngine::telugu()
{
    {
        FT_Face face = loadFace("Pothana2000.ttf");
        if (face) {
	    const ShapeTable shape_table [] = {
                { { 0xc15, 0xc4d, 0x0 },
                  { 0xbb, 0x0 } }, 
                { { 0xc15, 0xc4d, 0xc37, 0x0 },
                  { 0x4b, 0x0 } }, 
                { { 0xc15, 0xc4d, 0xc37, 0xc4d, 0x0 },
                  { 0xe0, 0x0 } }, 
                { { 0xc15, 0xc4d, 0xc37, 0xc4d, 0xc23, 0x0 },
                  { 0x4b, 0x91, 0x0 } }, 
                { { 0xc15, 0xc4d, 0xc30, 0x0 },
                  { 0x5a, 0xb2, 0x0 } }, 
                { { 0xc15, 0xc4d, 0xc30, 0xc4d, 0x0 },
                  { 0xbb, 0xb2, 0x0 } }, 
                { { 0xc15, 0xc4d, 0xc30, 0xc4d, 0xc15, 0x0 },
                  { 0x5a, 0xb2, 0x83, 0x0 } }, 
                { { 0xc15, 0xc4d, 0xc30, 0xc3f, 0x0 },
                  { 0xe2, 0xb2, 0x0 } }, 
                { { 0xc15, 0xc4d, 0xc15, 0xc48, 0x0 },
                  { 0xe6, 0xb3, 0x83, 0x0 } },
                { { 0xc15, 0xc4d, 0xc30, 0xc48, 0x0 },
                  { 0xe6, 0xb3, 0x9f, 0x0 } }, 
                { { 0xc15, 0xc46, 0xc56, 0x0 },
                  { 0xe6, 0xb3, 0x0 } },
                { {0}, {0} }
            };

	    const ShapeTable *s = shape_table;
	    while (s->unicode[0]) {
		QVERIFY( shaping(face, s, HB_Script_Telugu) );
		++s;
	    }

            FT_Done_Face(face);
	} else {
	    QSKIP("couln't find Pothana2000.ttf", SkipAll);
	}
    }
}


void tst_QScriptEngine::kannada()
{
    {
        FT_Face face = loadFace("Sampige.ttf");
        if (face) {
	    const ShapeTable shape_table [] = {
		{ { 0x0ca8, 0x0ccd, 0x0ca8, 0x0 },
		  { 0x0049, 0x00ba, 0x0 } },
		{ { 0x0ca8, 0x0ccd, 0x0ca1, 0x0 },
		  { 0x0049, 0x00b3, 0x0 } },
		{ { 0x0caf, 0x0cc2, 0x0 },
		  { 0x004f, 0x005d, 0x0 } },
		{ { 0x0ce0, 0x0 },
		  { 0x006a, 0x0 } },
		{ { 0x0ce6, 0x0ce7, 0x0ce8, 0x0 },
		  { 0x006b, 0x006c, 0x006d, 0x0 } },
		{ { 0x0cb5, 0x0ccb, 0x0 },
		  { 0x015f, 0x0067, 0x0 } },
		{ { 0x0cb0, 0x0ccd, 0x0cae, 0x0 },
		  { 0x004e, 0x0082, 0x0 } },
		{ { 0x0cb0, 0x0ccd, 0x0c95, 0x0 },
		  { 0x0036, 0x0082, 0x0 } },
		{ { 0x0c95, 0x0ccd, 0x0cb0, 0x0 },
		  { 0x0036, 0x00c1, 0x0 } },
		{ { 0x0cb0, 0x0ccd, 0x200d, 0x0c95, 0x0 },
		  { 0x0050, 0x00a7, 0x0 } },

                // Kaphala
                { { 0x0cb0, 0x200d, 0x0ccd, 0x0c95, 0x0 },
                  { 0x0050, 0x00a7, 0x0 } },

		{ {0}, {0} }
	    };


	    const ShapeTable *s = shape_table;
	    while (s->unicode[0]) {
		QVERIFY( shaping(face, s, HB_Script_Kannada) );
		++s;
	    }

            FT_Done_Face(face);
	} else {
	    QSKIP("couln't find Sampige.ttf", SkipAll);
	}
    }
    {
        FT_Face face = loadFace("tunga.ttf");
        if (face) {
	    const ShapeTable shape_table [] = {
		{ { 0x0cb7, 0x0cc6, 0x0 },
		  { 0x00b0, 0x006c, 0x0 } },
		{ { 0x0cb7, 0x0ccd, 0x0 },
		  { 0x0163, 0x0 } },
                { { 0xc95, 0xcbf, 0xcd5, 0x0 },
                  { 0x114, 0x73, 0x0 } },
                { { 0xc95, 0xcc6, 0xcd5, 0x0 },
                  { 0x90, 0x6c, 0x73, 0x0 } },
                { { 0xc95, 0xcc6, 0xcd6, 0x0 },
                  { 0x90, 0x6c, 0x74, 0x0 } },
                { { 0xc95, 0xcc6, 0xcc2, 0x0 },
                  { 0x90, 0x6c, 0x69, 0x0 } },
                { { 0xc95, 0xcca, 0xcd5, 0x0 },
                  { 0x90, 0x6c, 0x69, 0x73, 0x0 } },


		{ {0}, {0} }
	    };


	    const ShapeTable *s = shape_table;
	    while (s->unicode[0]) {
		QVERIFY( shaping(face, s, HB_Script_Kannada) );
		++s;
	    }

            FT_Done_Face(face);
	} else {
	    QSKIP("couln't find tunga.ttf", SkipAll);
	}
    }
}



void tst_QScriptEngine::malayalam()
{
    {
        FT_Face face = loadFace("AkrutiMal2Normal.ttf");
        if (face) {
	    const ShapeTable shape_table [] = {
		{ { 0x0d15, 0x0d46, 0x0 },
		  { 0x005e, 0x0034, 0x0 } },
		{ { 0x0d15, 0x0d47, 0x0 },
		  { 0x005f, 0x0034, 0x0 } },
		{ { 0x0d15, 0x0d4b, 0x0 },
		  { 0x005f, 0x0034, 0x0058, 0x0 } },
		{ { 0x0d15, 0x0d48, 0x0 },
		  { 0x0060, 0x0034, 0x0 } },
		{ { 0x0d15, 0x0d4a, 0x0 },
		  { 0x005e, 0x0034, 0x0058, 0x0 } },
		{ { 0x0d30, 0x0d4d, 0x0d15, 0x0 },
		  { 0x009e, 0x0034, 0x0 } },
		{ { 0x0d15, 0x0d4d, 0x0d35, 0x0 },
		  { 0x0034, 0x007a, 0x0 } },
		{ { 0x0d15, 0x0d4d, 0x0d2f, 0x0 },
		  { 0x0034, 0x00a2, 0x0 } },
		{ { 0x0d1f, 0x0d4d, 0x0d1f, 0x0 },
		  { 0x0069, 0x0 } },
		{ { 0x0d26, 0x0d4d, 0x0d26, 0x0 },
		  { 0x0074, 0x0 } },
		{ { 0x0d30, 0x0d4d, 0x0 },
		  { 0x009e, 0x0 } },
		{ { 0x0d30, 0x0d4d, 0x200c, 0x0 },
		  { 0x009e, 0x0 } },
		{ { 0x0d30, 0x0d4d, 0x200d, 0x0 },
		  { 0x009e, 0x0 } },
                { { 0xd15, 0xd46, 0xd3e, 0x0 },
                  { 0x5e, 0x34, 0x58, 0x0 } },
                { { 0xd15, 0xd47, 0xd3e, 0x0 },
                  { 0x5f, 0x34, 0x58, 0x0 } },
                { { 0xd15, 0xd46, 0xd57, 0x0 },
                  { 0x5e, 0x34, 0x65, 0x0 } },
                { { 0xd15, 0xd57, 0x0 },
                  { 0x34, 0x65, 0x0 } },
                { { 0xd1f, 0xd4d, 0xd1f, 0xd41, 0xd4d, 0x0 },
                  { 0x69, 0x5b, 0x64, 0x0 } },

		{ {0}, {0} }
	    };


	    const ShapeTable *s = shape_table;
	    while (s->unicode[0]) {
		QVERIFY( shaping(face, s, HB_Script_Malayalam) );
		++s;
	    }

            FT_Done_Face(face);
	} else {
	    QSKIP("couln't find AkrutiMal2Normal.ttf", SkipAll);
	}
    }

    {
        FT_Face face = loadFace("Rachana.ttf");
        if (face) {
            const ShapeTable shape_table [] = {
                { { 0xd37, 0xd4d, 0xd1f, 0xd4d, 0xd30, 0xd40, 0x0 },
                  { 0x385, 0xa3, 0x0 } },
                { { 0xd2f, 0xd4d, 0xd15, 0xd4d, 0xd15, 0xd41, 0x0 },
                  { 0x2ff, 0x0 } },
                { { 0xd33, 0xd4d, 0xd33, 0x0 },
                  { 0x3f8, 0x0 } },
                { { 0xd2f, 0xd4d, 0xd15, 0xd4d, 0xd15, 0xd41, 0x0 },
                  { 0x2ff, 0x0 } },
                { { 0xd30, 0xd4d, 0x200d, 0xd35, 0xd4d, 0xd35, 0x0 },
                  { 0xf3, 0x350, 0x0 } },

                { {0}, {0} }
            };


            const ShapeTable *s = shape_table;
            while (s->unicode[0]) {
                QVERIFY( shaping(face, s, HB_Script_Malayalam) );
                ++s;
            }

            FT_Done_Face(face);
        } else {
            QSKIP("couln't find Rachana.ttf", SkipAll);
        }
    }

}

void tst_QScriptEngine::sinhala()
{
    {
        FT_Face face = loadFace("FM-MalithiUW46.ttf");
        if (face) {
            const ShapeTable shape_table [] = {
                { { 0xd9a, 0xdd9, 0xdcf, 0x0 },
                  { 0x4a, 0x61, 0x42, 0x0 } },
                { { 0xd9a, 0xdd9, 0xddf, 0x0 },
                  { 0x4a, 0x61, 0x50, 0x0 } },
                { { 0xd9a, 0xdd9, 0xdca, 0x0 },
                  { 0x4a, 0x62, 0x0 } },
                { { 0xd9a, 0xddc, 0xdca, 0x0 },
                  { 0x4a, 0x61, 0x42, 0x41, 0x0 } },
                { { 0xd9a, 0xdda, 0x0 },
                  { 0x4a, 0x62, 0x0 } },
                { { 0xd9a, 0xddd, 0x0 },
                  { 0x4a, 0x61, 0x42, 0x41, 0x0 } },
                { {0}, {0} }
            };

            const ShapeTable *s = shape_table;
            while (s->unicode[0]) {
                QVERIFY( shaping(face, s, HB_Script_Sinhala) );
                ++s;
            }

            FT_Done_Face(face);
        } else {
            QSKIP("couln't find FM-MalithiUW46.ttf", SkipAll);
        }
    }
}


void tst_QScriptEngine::khmer()
{
    {
        FT_Face face = loadFace("KhmerOS.ttf");
        if (face) {
	    const ShapeTable shape_table [] = {
		{ { 0x179a, 0x17cd, 0x0 },
		  { 0x24c, 0x27f, 0x0 } },
		{ { 0x179f, 0x17c5, 0x0 },
		  { 0x273, 0x203, 0x0 } },
		{ { 0x1790, 0x17d2, 0x1784, 0x17c3, 0x0 },
		  { 0x275, 0x242, 0x182, 0x0 } },
		{ { 0x179a, 0x0 },
		  { 0x24c, 0x0 } },
		{ { 0x1781, 0x17d2, 0x1798, 0x17c2, 0x0 },
		  { 0x274, 0x233, 0x197, 0x0 } },
		{ { 0x1798, 0x17b6, 0x0 },
		  { 0x1cb, 0x0 } },
		{ { 0x179a, 0x17b8, 0x0 },
		  { 0x24c, 0x26a, 0x0 } },
		{ { 0x1787, 0x17b6, 0x0 },
		  { 0x1ba, 0x0 } },
		{ { 0x1798, 0x17d2, 0x1796, 0x17bb, 0x0 },
		  { 0x24a, 0x195, 0x26d, 0x0 } },
		{ {0}, {0} }
	    };


	    const ShapeTable *s = shape_table;
	    while (s->unicode[0]) {
		QVERIFY( shaping(face, s, HB_Script_Khmer) );
		++s;
	    }

            FT_Done_Face(face);
	} else {
	    QSKIP("couln't find KhmerOS.ttf", SkipAll);
	}
    }
}

void tst_QScriptEngine::nko()
{
    {
        FT_Face face = loadFace("DejaVuSans.ttf");
        if (face) {
	    const ShapeTable shape_table [] = {
                { { 0x7ca, 0x0 },
                  { 0x5c1, 0x0 } },
                { { 0x7ca, 0x7ca, 0x0 },
                  { 0x14db, 0x14d9, 0x0 } },
                { { 0x7ca, 0x7fa, 0x7ca, 0x0 },
                  { 0x14db, 0x5ec, 0x14d9, 0x0 } },
                { { 0x7ca, 0x7f3, 0x7ca, 0x0 },
                  { 0x14db, 0x5e7, 0x14d9, 0x0 } },
                { { 0x7ca, 0x7f3, 0x7fa, 0x7ca, 0x0 },
                  { 0x14db, 0x5e7, 0x5ec, 0x14d9, 0x0 } },
                { {0}, {0} }
	    };


	    const ShapeTable *s = shape_table;
	    while (s->unicode[0]) {
                QVERIFY( shaping(face, s, HB_Script_Nko) );
		++s;
	    }

            FT_Done_Face(face);
	} else {
	    QSKIP("couln't find DejaVuSans.ttf", SkipAll);
	}
    }
}


void tst_QScriptEngine::linearB()
{
    {
        FT_Face face = loadFace("penuture.ttf");
        if (face) {
	    const ShapeTable shape_table [] = {
		{ { 0xd800, 0xdc01, 0xd800, 0xdc02, 0xd800, 0xdc03,  0 },
                  { 0x5, 0x6, 0x7, 0 } },
		{ {0}, {0} }
	    };


	    const ShapeTable *s = shape_table;
	    while (s->unicode[0]) {
		QVERIFY( shaping(face, s, HB_Script_Common) );
		++s;
	    }

            FT_Done_Face(face);
	} else {
	    QSKIP("couln't find PENUTURE.TTF", SkipAll);
	}
    }
}


QTEST_MAIN(tst_QScriptEngine)
#include "main.moc"
