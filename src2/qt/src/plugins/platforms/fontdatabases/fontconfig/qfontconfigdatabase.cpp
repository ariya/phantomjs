/****************************************************************************
**
** Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the plugins of the Qt Toolkit.
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

#include "qfontconfigdatabase.h"

#include <QtCore/QList>
#include <QtGui/private/qfont_p.h>

#include <QtCore/QElapsedTimer>

#include <QtGui/private/qapplication_p.h>
#include <QtGui/QPlatformScreen>

#include <QtGui/private/qfontengine_ft_p.h>
#include <QtGui/private/qfontengine_p.h>



#include <ft2build.h>
#include FT_TRUETYPE_TABLES_H

#include <fontconfig/fontconfig.h>

#define SimplifiedChineseCsbBit 18
#define TraditionalChineseCsbBit 20
#define JapaneseCsbBit 17
#define KoreanCsbBit 21

static inline bool requiresOpenType(int writingSystem)
{
    return ((writingSystem >= QFontDatabase::Syriac && writingSystem <= QFontDatabase::Sinhala)
            || writingSystem == QFontDatabase::Khmer || writingSystem == QFontDatabase::Nko);
}
static inline bool scriptRequiresOpenType(int script)
{
    return ((script >= QUnicodeTables::Syriac && script <= QUnicodeTables::Sinhala)
            || script == QUnicodeTables::Khmer || script == QUnicodeTables::Nko);
}

static int getFCWeight(int fc_weight)
{
    int qtweight = QFont::Black;
    if (fc_weight <= (FC_WEIGHT_LIGHT + FC_WEIGHT_MEDIUM) / 2)
        qtweight = QFont::Light;
    else if (fc_weight <= (FC_WEIGHT_MEDIUM + FC_WEIGHT_DEMIBOLD) / 2)
        qtweight = QFont::Normal;
    else if (fc_weight <= (FC_WEIGHT_DEMIBOLD + FC_WEIGHT_BOLD) / 2)
        qtweight = QFont::DemiBold;
    else if (fc_weight <= (FC_WEIGHT_BOLD + FC_WEIGHT_BLACK) / 2)
        qtweight = QFont::Bold;

    return qtweight;
}

static const char *specialLanguages[] = {
    "en", // Common
    "el", // Greek
    "ru", // Cyrillic
    "hy", // Armenian
    "he", // Hebrew
    "ar", // Arabic
    "syr", // Syriac
    "div", // Thaana
    "hi", // Devanagari
    "bn", // Bengali
    "pa", // Gurmukhi
    "gu", // Gujarati
    "or", // Oriya
    "ta", // Tamil
    "te", // Telugu
    "kn", // Kannada
    "ml", // Malayalam
    "si", // Sinhala
    "th", // Thai
    "lo", // Lao
    "bo", // Tibetan
    "my", // Myanmar
    "ka", // Georgian
    "ko", // Hangul
    "", // Ogham
    "", // Runic
    "km", // Khmer
    "" // N'Ko
};
enum { SpecialLanguageCount = sizeof(specialLanguages) / sizeof(const char *) };

static const ushort specialChars[] = {
    0, // English
    0, // Greek
    0, // Cyrillic
    0, // Armenian
    0, // Hebrew
    0, // Arabic
    0, // Syriac
    0, // Thaana
    0, // Devanagari
    0, // Bengali
    0, // Gurmukhi
    0, // Gujarati
    0, // Oriya
    0, // Tamil
    0xc15, // Telugu
    0xc95, // Kannada
    0xd15, // Malayalam
    0xd9a, // Sinhala
    0, // Thai
    0, // Lao
    0, // Tibetan
    0x1000, // Myanmar
    0, // Georgian
    0, // Hangul
    0x1681, // Ogham
    0x16a0, // Runic
    0,  // Khmer
    0x7ca // N'Ko
};
enum { SpecialCharCount = sizeof(specialChars) / sizeof(ushort) };

// this could become a list of all languages used for each writing
// system, instead of using the single most common language.
static const char *languageForWritingSystem[] = {
    0,     // Any
    "en",  // Latin
    "el",  // Greek
    "ru",  // Cyrillic
    "hy",  // Armenian
    "he",  // Hebrew
    "ar",  // Arabic
    "syr", // Syriac
    "div", // Thaana
    "hi",  // Devanagari
    "bn",  // Bengali
    "pa",  // Gurmukhi
    "gu",  // Gujarati
    "or",  // Oriya
    "ta",  // Tamil
    "te",  // Telugu
    "kn",  // Kannada
    "ml",  // Malayalam
    "si",  // Sinhala
    "th",  // Thai
    "lo",  // Lao
    "bo",  // Tibetan
    "my",  // Myanmar
    "ka",  // Georgian
    "km",  // Khmer
    "zh-cn", // SimplifiedChinese
    "zh-tw", // TraditionalChinese
    "ja",  // Japanese
    "ko",  // Korean
    "vi",  // Vietnamese
    0, // Symbol
    0, // Ogham
    0, // Runic
    0 // N'Ko
};
enum { LanguageCount = sizeof(languageForWritingSystem) / sizeof(const char *) };

// Unfortunately FontConfig doesn't know about some languages. We have to test these through the
// charset. The lists below contain the systems where we need to do this.
static const ushort sampleCharForWritingSystem[] = {
    0,     // Any
    0,  // Latin
    0,  // Greek
    0,  // Cyrillic
    0,  // Armenian
    0,  // Hebrew
    0,  // Arabic
    0, // Syriac
    0, // Thaana
    0,  // Devanagari
    0,  // Bengali
    0,  // Gurmukhi
    0,  // Gujarati
    0,  // Oriya
    0,  // Tamil
    0xc15,  // Telugu
    0xc95,  // Kannada
    0xd15,  // Malayalam
    0xd9a,  // Sinhala
    0,  // Thai
    0,  // Lao
    0,  // Tibetan
    0x1000,  // Myanmar
    0,  // Georgian
    0,  // Khmer
    0, // SimplifiedChinese
    0, // TraditionalChinese
    0,  // Japanese
    0,  // Korean
    0,  // Vietnamese
    0, // Symbol
    0x1681, // Ogham
    0x16a0, // Runic
    0x7ca // N'Ko
};
enum { SampleCharCount = sizeof(sampleCharForWritingSystem) / sizeof(ushort) };

// Newer FontConfig let's us sort out fonts that contain certain glyphs, but no
// open type tables for is directly. Do this so we don't pick some strange
// pseudo unicode font
static const char *openType[] = {
    0,     // Any
    0,  // Latin
    0,  // Greek
    0,  // Cyrillic
    0,  // Armenian
    0,  // Hebrew
    0,  // Arabic
    "syrc",  // Syriac
    "thaa",  // Thaana
    "deva",  // Devanagari
    "beng",  // Bengali
    "guru",  // Gurmukhi
    "gurj",  // Gujarati
    "orya",  // Oriya
    "taml",  // Tamil
    "telu",  // Telugu
    "knda",  // Kannada
    "mlym",  // Malayalam
    "sinh",  // Sinhala
    0,  // Thai
    0,  // Lao
    "tibt",  // Tibetan
    "mymr",  // Myanmar
    0,  // Georgian
    "khmr",  // Khmer
    0, // SimplifiedChinese
    0, // TraditionalChinese
    0,  // Japanese
    0,  // Korean
    0,  // Vietnamese
    0, // Symbol
    0, // Ogham
    0, // Runic
    "nko " // N'Ko
};

static const char *getFcFamilyForStyleHint(const QFont::StyleHint style)
{
    const char *stylehint = 0;
    switch (style) {
    case QFont::SansSerif:
        stylehint = "sans-serif";
        break;
    case QFont::Serif:
        stylehint = "serif";
        break;
    case QFont::TypeWriter:
        stylehint = "monospace";
        break;
    default:
        break;
    }
    return stylehint;
}

void QFontconfigDatabase::populateFontDatabase()
{
    FcFontSet  *fonts;

    QString familyName;
    FcChar8 *value = 0;
    int weight_value;
    int slant_value;
    int spacing_value;
    FcChar8 *file_value;
    int indexValue;
    FcChar8 *foundry_value;
    FcBool scalable;
    FcBool antialias;

    {
        FcObjectSet *os = FcObjectSetCreate();
        FcPattern *pattern = FcPatternCreate();
        const char *properties [] = {
            FC_FAMILY, FC_WEIGHT, FC_SLANT,
            FC_SPACING, FC_FILE, FC_INDEX,
            FC_LANG, FC_CHARSET, FC_FOUNDRY, FC_SCALABLE, FC_PIXEL_SIZE, FC_WEIGHT,
            FC_WIDTH,
#if FC_VERSION >= 20297
            FC_CAPABILITY,
#endif
            (const char *)0
        };
        const char **p = properties;
        while (*p) {
            FcObjectSetAdd(os, *p);
            ++p;
        }
        fonts = FcFontList(0, pattern, os);
        FcObjectSetDestroy(os);
        FcPatternDestroy(pattern);
    }

    for (int i = 0; i < fonts->nfont; i++) {
        if (FcPatternGetString(fonts->fonts[i], FC_FAMILY, 0, &value) != FcResultMatch)
            continue;
        //         capitalize(value);
        familyName = QString::fromUtf8((const char *)value);
        slant_value = FC_SLANT_ROMAN;
        weight_value = FC_WEIGHT_MEDIUM;
        spacing_value = FC_PROPORTIONAL;
        file_value = 0;
        indexValue = 0;
        scalable = FcTrue;


        if (FcPatternGetInteger (fonts->fonts[i], FC_SLANT, 0, &slant_value) != FcResultMatch)
            slant_value = FC_SLANT_ROMAN;
        if (FcPatternGetInteger (fonts->fonts[i], FC_WEIGHT, 0, &weight_value) != FcResultMatch)
            weight_value = FC_WEIGHT_MEDIUM;
        if (FcPatternGetInteger (fonts->fonts[i], FC_SPACING, 0, &spacing_value) != FcResultMatch)
            spacing_value = FC_PROPORTIONAL;
        if (FcPatternGetString (fonts->fonts[i], FC_FILE, 0, &file_value) != FcResultMatch)
            file_value = 0;
        if (FcPatternGetInteger (fonts->fonts[i], FC_INDEX, 0, &indexValue) != FcResultMatch)
            indexValue = 0;
        if (FcPatternGetBool(fonts->fonts[i], FC_SCALABLE, 0, &scalable) != FcResultMatch)
            scalable = FcTrue;
        if (FcPatternGetString(fonts->fonts[i], FC_FOUNDRY, 0, &foundry_value) != FcResultMatch)
            foundry_value = 0;
        if(FcPatternGetBool(fonts->fonts[i],FC_ANTIALIAS,0,&antialias) != FcResultMatch)
            antialias = true;

        QSupportedWritingSystems writingSystems;
        FcLangSet *langset = 0;
        FcResult res = FcPatternGetLangSet(fonts->fonts[i], FC_LANG, 0, &langset);
        if (res == FcResultMatch) {
            for (int i = 1; i < LanguageCount; ++i) {
                const FcChar8 *lang = (const FcChar8*) languageForWritingSystem[i];
                if (lang) {
                    FcLangResult langRes = FcLangSetHasLang(langset, lang);
                    if (langRes != FcLangDifferentLang)
                        writingSystems.setSupported(QFontDatabase::WritingSystem(i));
                }
            }
        } else {
            // we set Other to supported for symbol fonts. It makes no
            // sense to merge these with other ones, as they are
            // special in a way.
            writingSystems.setSupported(QFontDatabase::Other);
        }

        FcCharSet *cs = 0;
        res = FcPatternGetCharSet(fonts->fonts[i], FC_CHARSET, 0, &cs);
        if (res == FcResultMatch) {
            // some languages are not supported by FontConfig, we rather check the
            // charset to detect these
            for (int i = 1; i < SampleCharCount; ++i) {
                if (!sampleCharForWritingSystem[i])
                    continue;
                if (FcCharSetHasChar(cs, sampleCharForWritingSystem[i]))
                    writingSystems.setSupported(QFontDatabase::WritingSystem(i));
            }
        }

#if FC_VERSION >= 20297
        for (int j = 1; j < LanguageCount; ++j) {
            if (writingSystems.supported(QFontDatabase::WritingSystem(j))
                && requiresOpenType(j) && openType[j]) {
                FcChar8 *cap;
                res = FcPatternGetString (fonts->fonts[i], FC_CAPABILITY, 0, &cap);
                if (res != FcResultMatch || !strstr((const char *)cap, openType[j]))
                    writingSystems.setSupported(QFontDatabase::WritingSystem(j),false);
            }
        }
#endif

        FontFile *fontFile = new FontFile;
        fontFile->fileName = QLatin1String((const char *)file_value);
        fontFile->indexValue = indexValue;

        QFont::Style style = (slant_value == FC_SLANT_ITALIC)
                         ? QFont::StyleItalic
                         : ((slant_value == FC_SLANT_OBLIQUE)
                            ? QFont::StyleOblique
                            : QFont::StyleNormal);
        QFont::Weight weight = QFont::Weight(getFCWeight(weight_value));

        double pixel_size = 0;
        if (!scalable) {
            int width = 100;
            FcPatternGetInteger (fonts->fonts[i], FC_WIDTH, 0, &width);
            FcPatternGetDouble (fonts->fonts[i], FC_PIXEL_SIZE, 0, &pixel_size);
        }

        QFont::Stretch stretch = QFont::Unstretched;
        QPlatformFontDatabase::registerFont(familyName,QLatin1String((const char *)foundry_value),weight,style,stretch,antialias,scalable,pixel_size,writingSystems,fontFile);
//        qDebug() << familyName << (const char *)foundry_value << weight << style << &writingSystems << scalable << true << pixel_size;
    }

    FcFontSetDestroy (fonts);

    struct FcDefaultFont {
        const char *qtname;
        const char *rawname;
        bool fixed;
    };
    const FcDefaultFont defaults[] = {
        { "Serif", "serif", false },
        { "Sans Serif", "sans-serif", false },
        { "Monospace", "monospace", true },
        { 0, 0, false }
    };
    const FcDefaultFont *f = defaults;
    // aliases only make sense for 'common', not for any of the specials
    QSupportedWritingSystems ws;
    ws.setSupported(QFontDatabase::Latin);


    while (f->qtname) {
        registerFont(f->qtname,QLatin1String(""),QFont::Normal,QFont::StyleNormal,QFont::Unstretched,true,true,0,ws,0);
        registerFont(f->qtname,QLatin1String(""),QFont::Normal,QFont::StyleItalic,QFont::Unstretched,true,true,0,ws,0);
        registerFont(f->qtname,QLatin1String(""),QFont::Normal,QFont::StyleOblique,QFont::Unstretched,true,true,0,ws,0);
        ++f;
    }

    //Lighthouse has very lazy population of the font db. We want it to be initialized when
    //QApplication is constructed, so that the population procedure can do something like this to
    //set the default font
//    const FcDefaultFont *s = defaults;
//    QFont font("Sans Serif");
//    font.setPointSize(9);
//    QApplication::setFont(font);
}

QFontEngine *QFontconfigDatabase::fontEngine(const QFontDef &f, QUnicodeTables::Script script, void *usrPtr)
{
    if (!usrPtr)
        return 0;
    QFontDef fontDef = f;

    QFontEngineFT *engine;
    FontFile *fontfile = static_cast<FontFile *> (usrPtr);
    QFontEngine::FaceId fid;
    fid.filename = fontfile->fileName.toLocal8Bit();
    fid.index = fontfile->indexValue;

    //try and get the pattern
    FcPattern *pattern = FcPatternCreate();

    bool antialias = !(fontDef.styleStrategy & QFont::NoAntialias);
    QFontEngineFT::GlyphFormat format = antialias? QFontEngineFT::Format_A8 : QFontEngineFT::Format_Mono;

    engine = new QFontEngineFT(fontDef);

    FcValue value;
    value.type = FcTypeString;
        QByteArray cs = fontDef.family.toUtf8();
    value.u.s = (const FcChar8 *)cs.data();
    FcPatternAdd(pattern,FC_FAMILY,value,true);


    value.u.s = (const FcChar8 *)fid.filename.data();
    FcPatternAdd(pattern,FC_FILE,value,true);

    value.type = FcTypeInteger;
    value.u.i = fid.index;
    FcPatternAdd(pattern,FC_INDEX,value,true);

    QFontEngineFT::HintStyle default_hint_style;

    if (FcConfigSubstitute(0,pattern,FcMatchPattern)) {

        //hinting
        int hint_style = 0;
        if (FcPatternGetInteger (pattern, FC_HINT_STYLE, 0, &hint_style) == FcResultNoMatch)
            hint_style = QFontEngineFT::HintFull;
        switch (hint_style) {
        case FC_HINT_NONE:
            default_hint_style = QFontEngineFT::HintNone;
            break;
        case FC_HINT_SLIGHT:
            default_hint_style = QFontEngineFT::HintLight;
            break;
        case FC_HINT_MEDIUM:
            default_hint_style = QFontEngineFT::HintMedium;
            break;
        default:
            default_hint_style = QFontEngineFT::HintFull;
            break;
        }
    }

    engine->setDefaultHintStyle(default_hint_style);
    if (!engine->init(fid,antialias,format)) {
        delete engine;
        engine = 0;
        return engine;
    }
    if (engine->invalid()) {
        delete engine;
        engine = 0;
    } else if (scriptRequiresOpenType(script)) {
        HB_Face hbFace = engine->harfbuzzFace();
        if (!hbFace || !hbFace->supported_scripts[script]) {
            delete engine;
            engine = 0;
        }
    }

    return engine;
}

QStringList QFontconfigDatabase::fallbacksForFamily(const QString family, const QFont::Style &style, const QFont::StyleHint &styleHint, const QUnicodeTables::Script &script) const
{
    QStringList fallbackFamilies;
    FcPattern *pattern = FcPatternCreate();
    if (!pattern)
        return fallbackFamilies;

    FcValue value;
    value.type = FcTypeString;
    QByteArray cs = family.toUtf8();
    value.u.s = (const FcChar8 *)cs.data();
    FcPatternAdd(pattern,FC_FAMILY,value,true);

    int slant_value = FC_SLANT_ROMAN;
    if (style == QFont::StyleItalic)
        slant_value = FC_SLANT_ITALIC;
    else if (style == QFont::StyleOblique)
        slant_value = FC_SLANT_OBLIQUE;
    FcPatternAddInteger(pattern, FC_SLANT, slant_value);

    if (script != QUnicodeTables::Common && *specialLanguages[script] != '\0') {
        Q_ASSERT(script < QUnicodeTables::ScriptCount);
        FcLangSet *ls = FcLangSetCreate();
        FcLangSetAdd(ls, (const FcChar8*)specialLanguages[script]);
        FcPatternAddLangSet(pattern, FC_LANG, ls);
        FcLangSetDestroy(ls);
    }

    const char *stylehint = getFcFamilyForStyleHint(styleHint);
    if (stylehint) {
        value.u.s = (const FcChar8 *)stylehint;
        FcPatternAddWeak(pattern, FC_FAMILY, value, FcTrue);
    }

    FcConfigSubstitute(0, pattern, FcMatchPattern);
    FcConfigSubstitute(0, pattern, FcMatchFont);

    FcResult result = FcResultMatch;
    FcFontSet *fontSet = FcFontSort(0,pattern,FcFalse,0,&result);

    if (fontSet && result == FcResultMatch)
    {
        for (int i = 0; i < fontSet->nfont; i++) {
            FcChar8 *value = 0;
            if (FcPatternGetString(fontSet->fonts[i], FC_FAMILY, 0, &value) != FcResultMatch)
                continue;
            //         capitalize(value);
            QString familyName = QString::fromUtf8((const char *)value);
            if (!fallbackFamilies.contains(familyName,Qt::CaseInsensitive)) {
                fallbackFamilies << familyName;
            }

        }
    }
//    qDebug() << "fallbackFamilies for:" << family << fallbackFamilies;

    return fallbackFamilies;
}
