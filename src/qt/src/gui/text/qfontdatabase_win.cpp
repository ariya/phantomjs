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

#include "qt_windows.h"
#include <qmath.h>
#include <private/qapplication_p.h>
#include "qfont_p.h"
#include "qfontengine_p.h"
#include "qpaintdevice.h"
#include <private/qsystemlibrary_p.h>
#include "qabstractfileengine.h"
#include "qendian.h"

#if !defined(QT_NO_DIRECTWRITE)
#  include "qsettings.h"
#  include "qfontenginedirectwrite_p.h"
#endif

#ifdef Q_OS_WINCE
#   include <QTemporaryFile>
#endif

QT_BEGIN_NAMESPACE

extern HDC   shared_dc();                // common dc for all fonts

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

static HFONT stock_sysfont  = 0;

static bool localizedName(const QString &name)
{
    const QChar *c = name.unicode();
    for(int i = 0; i < name.length(); ++i) {
        if(c[i].unicode() >= 0x100)
            return true;
    }
    return false;
}

static inline quint16 getUShort(const unsigned char *p)
{
    quint16 val;
    val = *p++ << 8;
    val |= *p;

    return val;
}

static QString getEnglishName(const uchar *table, quint32 bytes)
{
    QString i18n_name;
    enum {
        NameRecordSize = 12,
        FamilyId = 1,
        MS_LangIdEnglish = 0x009
    };

    // get the name table
    quint16 count;
    quint16 string_offset;
    const unsigned char *names;

    int microsoft_id = -1;
    int apple_id = -1;
    int unicode_id = -1;

    if(getUShort(table) != 0)
        goto error;

    count = getUShort(table+2);
    string_offset = getUShort(table+4);
    names = table + 6;

    if(string_offset >= bytes || 6 + count*NameRecordSize > string_offset)
        goto error;

    for(int i = 0; i < count; ++i) {
        // search for the correct name entry

        quint16 platform_id = getUShort(names + i*NameRecordSize);
        quint16 encoding_id = getUShort(names + 2 + i*NameRecordSize);
        quint16 language_id = getUShort(names + 4 + i*NameRecordSize);
        quint16 name_id = getUShort(names + 6 + i*NameRecordSize);

        if(name_id != FamilyId)
            continue;

        enum {
            PlatformId_Unicode = 0,
            PlatformId_Apple = 1,
            PlatformId_Microsoft = 3
        };

        quint16 length = getUShort(names + 8 + i*NameRecordSize);
        quint16 offset = getUShort(names + 10 + i*NameRecordSize);
        if(DWORD(string_offset + offset + length) >= bytes)
            continue;

        if ((platform_id == PlatformId_Microsoft
            && (encoding_id == 0 || encoding_id == 1))
            && (language_id & 0x3ff) == MS_LangIdEnglish
            && microsoft_id == -1)
            microsoft_id = i;
            // not sure if encoding id 4 for Unicode is utf16 or ucs4...
        else if(platform_id == PlatformId_Unicode && encoding_id < 4 && unicode_id == -1)
            unicode_id = i;
        else if(platform_id == PlatformId_Apple && encoding_id == 0 && language_id == 0)
            apple_id = i;
    }
    {
        bool unicode = false;
        int id = -1;
        if(microsoft_id != -1) {
            id = microsoft_id;
            unicode = true;
        } else if(apple_id != -1) {
            id = apple_id;
            unicode = false;
        } else if (unicode_id != -1) {
            id = unicode_id;
            unicode = true;
        }
        if(id != -1) {
            quint16 length = getUShort(names + 8 + id*NameRecordSize);
            quint16 offset = getUShort(names + 10 + id*NameRecordSize);
            if(unicode) {
                // utf16

                length /= 2;
                i18n_name.resize(length);
                QChar *uc = (QChar *) i18n_name.unicode();
                const unsigned char *string = table + string_offset + offset;
                for(int i = 0; i < length; ++i)
                    uc[i] = getUShort(string + 2*i);
            } else {
                // Apple Roman

                i18n_name.resize(length);
                QChar *uc = (QChar *) i18n_name.unicode();
                const unsigned char *string = table + string_offset + offset;
                for(int i = 0; i < length; ++i)
                    uc[i] = QLatin1Char(string[i]);
            }
        }
    }
  error:
    //qDebug("got i18n name of '%s' for font '%s'", i18n_name.latin1(), familyName.toLocal8Bit().data());
    return i18n_name;
}

static QString getEnglishName(const QString &familyName)
{
    QString i18n_name;

    HDC hdc = GetDC( 0 );
    LOGFONT lf;
    memset(&lf, 0, sizeof(LOGFONT));
    memcpy(lf.lfFaceName, familyName.utf16(), qMin(LF_FACESIZE, familyName.length()) * sizeof(wchar_t));
    lf.lfCharSet = DEFAULT_CHARSET;
    HFONT hfont = CreateFontIndirect(&lf);

    if(!hfont) {
        ReleaseDC(0, hdc);
        return QString();
    }

    HGDIOBJ oldobj = SelectObject( hdc, hfont );

    const DWORD name_tag = MAKE_TAG( 'n', 'a', 'm', 'e' );

    // get the name table
    unsigned char *table = 0;

    DWORD bytes = GetFontData( hdc, name_tag, 0, 0, 0 );
    if ( bytes == GDI_ERROR ) {
        // ### Unused variable
        /* int err = GetLastError(); */
        goto error;
    }

    table = new unsigned char[bytes];
    GetFontData(hdc, name_tag, 0, table, bytes);
    if ( bytes == GDI_ERROR )
        goto error;

    i18n_name = getEnglishName(table, bytes);
error:
    delete [] table;
    SelectObject( hdc, oldobj );
    DeleteObject( hfont );
    ReleaseDC( 0, hdc );

    //qDebug("got i18n name of '%s' for font '%s'", i18n_name.latin1(), familyName.toLocal8Bit().data());
    return i18n_name;
}

extern QFont::Weight weightFromInteger(int weight); // qfontdatabase.cpp

static
void addFontToDatabase(QString familyName, const QString &scriptName,
                       TEXTMETRIC *textmetric,
                       const FONTSIGNATURE *signature,
                       int type)
{
    const int script = -1;
    const QString foundryName;
    Q_UNUSED(script);

    bool italic = false;
    int weight;
    bool fixed;
    bool ttf;
    bool scalable;
    int size;

//    QString escript = QString::fromWCharArray(f->elfScript);
//    qDebug("script=%s", escript.latin1());

    NEWTEXTMETRIC *tm = (NEWTEXTMETRIC *)textmetric;
    fixed = !(tm->tmPitchAndFamily & TMPF_FIXED_PITCH);
    ttf = (tm->tmPitchAndFamily & TMPF_TRUETYPE);
    scalable = tm->tmPitchAndFamily & (TMPF_VECTOR|TMPF_TRUETYPE);
    size = scalable ? SMOOTH_SCALABLE : tm->tmHeight;
    italic = tm->tmItalic;
    weight = tm->tmWeight;

    // the "@family" fonts are just the same as "family". Ignore them.
    if (familyName[0] != QLatin1Char('@') && !familyName.startsWith(QLatin1String("WST_"))) {
        QtFontStyle::Key styleKey;
        styleKey.style = italic ? QFont::StyleItalic : QFont::StyleNormal;
        styleKey.weight = weightFromInteger(weight);

        QtFontFamily *family = privateDb()->family(familyName, true);

        if(ttf && localizedName(familyName) && family->english_name.isEmpty())
            family->english_name = getEnglishName(familyName);

        QtFontFoundry *foundry = family->foundry(foundryName, true);
        QtFontStyle *style = foundry->style(styleKey, QString(), true);
        style->smoothScalable = scalable;
        style->pixelSize( size, TRUE);

        // add fonts windows can generate for us:
        if (styleKey.weight <= QFont::DemiBold) {
            QtFontStyle::Key key(styleKey);
            key.weight = QFont::Bold;
            QtFontStyle *style = foundry->style(key, QString(), true);
            style->smoothScalable = scalable;
            style->pixelSize( size, TRUE);
        }
        if (styleKey.style != QFont::StyleItalic) {
            QtFontStyle::Key key(styleKey);
            key.style = QFont::StyleItalic;
            QtFontStyle *style = foundry->style(key, QString(), true);
            style->smoothScalable = scalable;
            style->pixelSize( size, TRUE);
        }
        if (styleKey.weight <= QFont::DemiBold && styleKey.style != QFont::StyleItalic) {
            QtFontStyle::Key key(styleKey);
            key.weight = QFont::Bold;
            key.style = QFont::StyleItalic;
            QtFontStyle *style = foundry->style(key, QString(), true);
            style->smoothScalable = scalable;
            style->pixelSize( size, TRUE);
        }

        family->fixedPitch = fixed;

        if (!family->writingSystemCheck && type & TRUETYPE_FONTTYPE) {
            quint32 unicodeRange[4] = {
                signature->fsUsb[0], signature->fsUsb[1],
                signature->fsUsb[2], signature->fsUsb[3]
            };
#ifdef Q_WS_WINCE
            if (signature->fsUsb[0] == 0) {
                // If the unicode ranges bit mask is zero then
                // EnumFontFamiliesEx failed to determine it properly.
                // In this case we just pretend that the font supports all languages.
                unicodeRange[0] = 0xbfffffff;   // second most significant bit must be zero
                unicodeRange[1] = 0xffffffff;
                unicodeRange[2] = 0xffffffff;
                unicodeRange[3] = 0xffffffff;
            }
#endif
            quint32 codePageRange[2] = {
                signature->fsCsb[0], signature->fsCsb[1]
            };
            QList<QFontDatabase::WritingSystem> systems = qt_determine_writing_systems_from_truetype_bits(unicodeRange, codePageRange);

            for (int i = 0; i < systems.count(); ++i) {
                QFontDatabase::WritingSystem writingSystem = systems.at(i);

                // ### Hack to work around problem with Thai text on Windows 7. Segoe UI contains
                // the symbol for Baht, and Windows thus reports that it supports the Thai script.
                // Since it's the default UI font on this platform, most widgets will be unable to
                // display Thai text by default. As a temporary work around, we special case Segoe UI
                // and remove the Thai script from its list of supported writing systems.
                if (writingSystem != QFontDatabase::Thai || familyName != QLatin1String("Segoe UI"))
                    family->writingSystems[writingSystem] = QtFontFamily::Supported;
            }
        } else if (!family->writingSystemCheck) {
            //qDebug("family='%s' script=%s", family->name.latin1(), script.latin1());
            if (scriptName == QLatin1String("Western")
                || scriptName == QLatin1String("Baltic")
                || scriptName == QLatin1String("Central European")
                || scriptName == QLatin1String("Turkish")
                || scriptName == QLatin1String("Vietnamese"))
                family->writingSystems[QFontDatabase::Latin] = QtFontFamily::Supported;
            else if (scriptName == QLatin1String("Thai"))
                family->writingSystems[QFontDatabase::Thai] = QtFontFamily::Supported;
            else if (scriptName == QLatin1String("Symbol")
                     || scriptName == QLatin1String("Other"))
                family->writingSystems[QFontDatabase::Symbol] = QtFontFamily::Supported;
            else if (scriptName == QLatin1String("OEM/Dos"))
                family->writingSystems[QFontDatabase::Latin] = QtFontFamily::Supported;
            else if (scriptName == QLatin1String("CHINESE_GB2312"))
                family->writingSystems[QFontDatabase::SimplifiedChinese] = QtFontFamily::Supported;
            else if (scriptName == QLatin1String("CHINESE_BIG5"))
                family->writingSystems[QFontDatabase::TraditionalChinese] = QtFontFamily::Supported;
            else if (scriptName == QLatin1String("Cyrillic"))
                family->writingSystems[QFontDatabase::Cyrillic] = QtFontFamily::Supported;
            else if (scriptName == QLatin1String("Hangul"))
                family->writingSystems[QFontDatabase::Korean] = QtFontFamily::Supported;
            else if (scriptName == QLatin1String("Hebrew"))
                family->writingSystems[QFontDatabase::Hebrew] = QtFontFamily::Supported;
            else if (scriptName == QLatin1String("Greek"))
                family->writingSystems[QFontDatabase::Greek] = QtFontFamily::Supported;
            else if (scriptName == QLatin1String("Japanese"))
                family->writingSystems[QFontDatabase::Japanese] = QtFontFamily::Supported;
            else if (scriptName == QLatin1String("Arabic"))
                family->writingSystems[QFontDatabase::Arabic] = QtFontFamily::Supported;
        }
    }
}

static
int CALLBACK
storeFont(ENUMLOGFONTEX* f, NEWTEXTMETRICEX *textmetric, int type, LPARAM /*p*/)
{
    QString familyName = QString::fromWCharArray(f->elfLogFont.lfFaceName);
    QString script = QString::fromWCharArray(f->elfScript);

    FONTSIGNATURE signature = textmetric->ntmFontSig;

    // NEWTEXTMETRICEX is a NEWTEXTMETRIC, which according to the documentation is
    // identical to a TEXTMETRIC except for the last four members, which we don't use
    // anyway
    addFontToDatabase(familyName, script, (TEXTMETRIC *)textmetric, &signature, type);
    // keep on enumerating
    return 1;
}

static
void populate_database(const QString& fam)
{
    QFontDatabasePrivate *d = privateDb();
    if (!d)
        return;

    QtFontFamily *family = 0;
    if(!fam.isEmpty()) {
        family = d->family(fam);
        if(family && family->loaded)
            return;
    } else if (d->count) {
        return;
    }

    HDC dummy = GetDC(0);

    LOGFONT lf;
    lf.lfCharSet = DEFAULT_CHARSET;
    if (fam.isNull()) {
        lf.lfFaceName[0] = 0;
    } else {
        memcpy(lf.lfFaceName, fam.utf16(), sizeof(wchar_t) * qMin(fam.length() + 1, 32));  // 32 = Windows hard-coded
    }
    lf.lfPitchAndFamily = 0;

    EnumFontFamiliesEx(dummy, &lf,
        (FONTENUMPROC)storeFont, (LPARAM)privateDb(), 0);

    ReleaseDC(0, dummy);

    for (int i = 0; i < d->applicationFonts.count(); ++i) {
        QFontDatabasePrivate::ApplicationFont fnt = d->applicationFonts.at(i);
        if (!fnt.memoryFont)
            continue;
        for (int j = 0; j < fnt.families.count(); ++j) {
            const QString familyName = fnt.families.at(j);
            HDC hdc = GetDC(0);
            LOGFONT lf;
            memset(&lf, 0, sizeof(LOGFONT));
            memcpy(lf.lfFaceName, familyName.utf16(), sizeof(wchar_t) * qMin(LF_FACESIZE, familyName.size()));
            lf.lfCharSet = DEFAULT_CHARSET;
            HFONT hfont = CreateFontIndirect(&lf);
            HGDIOBJ oldobj = SelectObject(hdc, hfont);

            TEXTMETRIC textMetrics;
            GetTextMetrics(hdc, &textMetrics);

            addFontToDatabase(familyName, QString(),
                              &textMetrics,
                              &fnt.signatures.at(j),
                              TRUETYPE_FONTTYPE);

            SelectObject(hdc, oldobj);
            DeleteObject(hfont);
            ReleaseDC(0, hdc);
        }
    }

    if(!fam.isEmpty()) {
        family = d->family(fam);
        if(family) {
            if(!family->writingSystemCheck) {
            }
            family->loaded = true;
        }
    }
}

static void initializeDb()
{
    QFontDatabasePrivate *db = privateDb();
    if (!db || db->count)
        return;

    populate_database(QString());

#ifdef QFONTDATABASE_DEBUG
    // print the database
    for (int f = 0; f < db->count; f++) {
        QtFontFamily *family = db->families[f];
        qDebug("    %s: %p", qPrintable(family->name), family);
        populate_database(family->name);

#if 0
        qDebug("        scripts supported:");
        for (int i = 0; i < QUnicodeTables::ScriptCount; i++)
            if(family->writingSystems[i] & QtFontFamily::Supported)
                qDebug("            %d", i);
        for (int fd = 0; fd < family->count; fd++) {
            QtFontFoundry *foundry = family->foundries[fd];
            qDebug("        %s", foundry->name.latin1());
            for (int s = 0; s < foundry->count; s++) {
                QtFontStyle *style = foundry->styles[s];
                qDebug("            style: style=%d weight=%d smooth=%d",  style->key.style,
                       style->key.weight, style->smoothScalable );
                if(!style->smoothScalable) {
                    for(int i = 0; i < style->count; ++i) {
                        qDebug("                %d", style->pixelSizes[i].pixelSize);
                    }
                }
            }
        }
#endif
    }
#endif // QFONTDATABASE_DEBUG

}

static inline void load(const QString &family = QString(), int = -1)
{
    populate_database(family);
}





// --------------------------------------------------------------------------------------
// font loader
// --------------------------------------------------------------------------------------



static void initFontInfo(QFontEngineWin *fe, const QFontDef &request, HDC fontHdc, int dpi)
{
    fe->fontDef = request;                                // most settings are equal

    HDC dc = ((request.styleStrategy & QFont::PreferDevice) && fontHdc) ? fontHdc : shared_dc();
    SelectObject(dc, fe->hfont);
    wchar_t n[64];
    GetTextFace(dc, 64, n);
    fe->fontDef.family = QString::fromWCharArray(n);
    fe->fontDef.fixedPitch = !(fe->tm.tmPitchAndFamily & TMPF_FIXED_PITCH);
    if (fe->fontDef.pointSize < 0) {
        fe->fontDef.pointSize = fe->fontDef.pixelSize * 72. / dpi;
    } else if (fe->fontDef.pixelSize == -1) {
        fe->fontDef.pixelSize = qRound(fe->fontDef.pointSize * dpi / 72.);
    }
}

#if !defined(QT_NO_DIRECTWRITE)
static void initFontInfo(QFontEngineDirectWrite *fe, const QFontDef &request,
                         int dpi, IDWriteFont *font)
{
    fe->fontDef = request;

    IDWriteFontFamily *fontFamily = NULL;
    HRESULT hr = font->GetFontFamily(&fontFamily);

    IDWriteLocalizedStrings *familyNames = NULL;
    if (SUCCEEDED(hr))
        hr = fontFamily->GetFamilyNames(&familyNames);

    UINT32 index = 0;
    BOOL exists = false;

    wchar_t localeName[LOCALE_NAME_MAX_LENGTH];

    if (SUCCEEDED(hr)) {
        int defaultLocaleSuccess = GetUserDefaultLocaleName(localeName, LOCALE_NAME_MAX_LENGTH);

        if (defaultLocaleSuccess)
            hr = familyNames->FindLocaleName(localeName, &index, &exists);

        if (SUCCEEDED(hr) && !exists)
            hr = familyNames->FindLocaleName(L"en-us", &index, &exists);
    }

    if (!exists)
        index = 0;

    UINT32 length = 0;
    if (SUCCEEDED(hr))
        hr = familyNames->GetStringLength(index, &length);

    wchar_t *name = new (std::nothrow) wchar_t[length+1];
    if (name == NULL)
        hr = E_OUTOFMEMORY;

    // Get the family name.
    if (SUCCEEDED(hr))
        hr = familyNames->GetString(index, name, length + 1);

    if (SUCCEEDED(hr))
        fe->fontDef.family = QString::fromWCharArray(name);

    delete[] name;
    if (familyNames != NULL)
        familyNames->Release();

    if (FAILED(hr))
        qErrnoWarning(hr, "initFontInfo: Failed to get family name");

    if (fe->fontDef.pointSize < 0)
        fe->fontDef.pointSize = fe->fontDef.pixelSize * 72. / dpi;
    else if (fe->fontDef.pixelSize == -1)
        fe->fontDef.pixelSize = qRound(fe->fontDef.pointSize * dpi / 72.);
}
#endif

static const char *other_tryFonts[] = {
    "Arial",
    "MS UI Gothic",
    "Gulim",
    "SimSun",
    "PMingLiU",
    "Arial Unicode MS",
    0
};

static const char *jp_tryFonts [] = {
    "MS UI Gothic",
    "Arial",
    "Gulim",
    "SimSun",
    "PMingLiU",
    "Arial Unicode MS",
    0
};

static const char *ch_CN_tryFonts [] = {
    "SimSun",
    "Arial",
    "PMingLiU",
    "Gulim",
    "MS UI Gothic",
    "Arial Unicode MS",
    0
};

static const char *ch_TW_tryFonts [] = {
    "PMingLiU",
    "Arial",
    "SimSun",
    "Gulim",
    "MS UI Gothic",
    "Arial Unicode MS",
    0
};

static const char *kr_tryFonts[] = {
    "Gulim",
    "Arial",
    "PMingLiU",
    "SimSun",
    "MS UI Gothic",
    "Arial Unicode MS",
    0
};

static const char **tryFonts = 0;

#if !defined(QT_NO_DIRECTWRITE)
static QString fontNameSubstitute(const QString &familyName)
{
    QLatin1String key("HKEY_LOCAL_MACHINE\\Software\\Microsoft\\Windows NT\\CurrentVersion\\"
                      "FontSubstitutes");
    return QSettings(key, QSettings::NativeFormat).value(familyName, familyName).toString();
}
#endif

static inline HFONT systemFont()
{
    if (stock_sysfont == 0)
        stock_sysfont = (HFONT)GetStockObject(SYSTEM_FONT);
    return stock_sysfont;
}

#if !defined(DEFAULT_GUI_FONT)
#define DEFAULT_GUI_FONT 17
#endif

static QFontEngine *loadEngine(int script, const QFontDef &request,
                               HDC fontHdc, int dpi, bool rawMode,
                               const QtFontDesc *desc,
                               const QStringList &family_list)
{
    LOGFONT lf;
    memset(&lf, 0, sizeof(LOGFONT));

    bool useDevice = (request.styleStrategy & QFont::PreferDevice) && fontHdc;

    HDC hdc = shared_dc();
    QString font_name = desc != 0 ? desc->family->name : request.family;

    if (useDevice) {
        hdc = fontHdc;
        font_name = request.family;
    }

    bool stockFont = false;
    bool preferClearTypeAA = false;

    HFONT hfont = 0;


#if !defined(QT_NO_DIRECTWRITE)
    bool useDirectWrite = (request.hintingPreference == QFont::PreferNoHinting)
                       || (request.hintingPreference == QFont::PreferVerticalHinting);
    IDWriteFont *directWriteFont = 0;
#else
    bool useDirectWrite = false;
#endif

    if (rawMode) {                        // will choose a stock font
        int f, deffnt = SYSTEM_FONT;
        QString fam = desc != 0 ? desc->family->name.toLower() : request.family.toLower();
        if (fam == QLatin1String("default"))
            f = deffnt;
        else if (fam == QLatin1String("system"))
            f = SYSTEM_FONT;
#ifndef Q_WS_WINCE
        else if (fam == QLatin1String("system_fixed"))
            f = SYSTEM_FIXED_FONT;
        else if (fam == QLatin1String("ansi_fixed"))
            f = ANSI_FIXED_FONT;
        else if (fam == QLatin1String("ansi_var"))
            f = ANSI_VAR_FONT;
        else if (fam == QLatin1String("device_default"))
            f = DEVICE_DEFAULT_FONT;
        else if (fam == QLatin1String("oem_fixed"))
            f = OEM_FIXED_FONT;
#endif
        else if (fam[0] == QLatin1Char('#'))
            f = fam.right(fam.length()-1).toInt();
        else
            f = deffnt;
        hfont = (HFONT)GetStockObject(f);
        if (!hfont) {
            qErrnoWarning("QFontEngine::loadEngine: GetStockObject failed");
            hfont = systemFont();
        }
        stockFont = true;
    } else {

        int hint = FF_DONTCARE;
        switch (request.styleHint) {
            case QFont::Helvetica:
                hint = FF_SWISS;
                break;
            case QFont::Times:
                hint = FF_ROMAN;
                break;
            case QFont::Courier:
                hint = FF_MODERN;
                break;
            case QFont::OldEnglish:
                hint = FF_DECORATIVE;
                break;
            case QFont::System:
                hint = FF_MODERN;
                break;
            default:
                break;
        }

        lf.lfHeight = -qRound(request.pixelSize);
        lf.lfWidth                = 0;
        lf.lfEscapement        = 0;
        lf.lfOrientation        = 0;
        if (desc == 0 || desc->style->key.weight == 50)
            lf.lfWeight = FW_DONTCARE;
        else
            lf.lfWeight = (desc->style->key.weight*900)/99;
        lf.lfItalic         = (desc != 0 && desc->style->key.style != QFont::StyleNormal);
        lf.lfCharSet        = DEFAULT_CHARSET;

        int strat = OUT_DEFAULT_PRECIS;
        if (request.styleStrategy & QFont::PreferBitmap) {
            strat = OUT_RASTER_PRECIS;
#ifndef Q_WS_WINCE
        } else if (request.styleStrategy & QFont::PreferDevice) {
            strat = OUT_DEVICE_PRECIS;
        } else if (request.styleStrategy & QFont::PreferOutline) {
            strat = OUT_OUTLINE_PRECIS;
        } else if (request.styleStrategy & QFont::ForceOutline) {
            strat = OUT_TT_ONLY_PRECIS;
#endif
        }

        lf.lfOutPrecision   = strat;

        int qual = DEFAULT_QUALITY;

        if (request.styleStrategy & QFont::PreferMatch)
            qual = DRAFT_QUALITY;
#ifndef Q_WS_WINCE
        else if (request.styleStrategy & QFont::PreferQuality)
            qual = PROOF_QUALITY;
#endif

        if (request.styleStrategy & QFont::PreferAntialias) {
            if (QSysInfo::WindowsVersion >= QSysInfo::WV_XP) {
                qual = CLEARTYPE_QUALITY;
                preferClearTypeAA = true;
            } else {
                qual = ANTIALIASED_QUALITY;
            }
        } else if (request.styleStrategy & QFont::NoAntialias) {
            qual = NONANTIALIASED_QUALITY;
        }

        lf.lfQuality        = qual;

        lf.lfClipPrecision  = CLIP_DEFAULT_PRECIS;
        lf.lfPitchAndFamily = DEFAULT_PITCH | hint;

        QString fam = font_name;

        if(fam.isEmpty())
            fam = QLatin1String("MS Sans Serif");

        if ((fam == QLatin1String("MS Sans Serif"))
            && (request.style == QFont::StyleItalic || (-lf.lfHeight > 18 && -lf.lfHeight != 24))) {
            fam = QLatin1String("Arial"); // MS Sans Serif has bearing problems in italic, and does not scale
        }
        if (fam == QLatin1String("Courier") && !(request.styleStrategy & QFont::PreferBitmap))
            fam = QLatin1String("Courier New");

        memcpy(lf.lfFaceName, fam.utf16(), sizeof(wchar_t) * qMin(fam.length() + 1, 32));  // 32 = Windows hard-coded

        hfont = CreateFontIndirect(&lf);
        if (!hfont)
            qErrnoWarning("QFontEngine::loadEngine: CreateFontIndirect failed");

        stockFont = (hfont == 0);
        bool ttf = false;
        int avWidth = 0;
        BOOL res;
        HGDIOBJ oldObj = SelectObject(hdc, hfont);

        TEXTMETRIC tm;
        res = GetTextMetrics(hdc, &tm);
        avWidth = tm.tmAveCharWidth;
        ttf = tm.tmPitchAndFamily & TMPF_TRUETYPE;
        SelectObject(hdc, oldObj);

        if (!ttf || !useDirectWrite) {
            useDirectWrite = false;

            if (hfont && (!ttf || request.stretch != 100)) {
                DeleteObject(hfont);
                if (!res)
                    qErrnoWarning("QFontEngine::loadEngine: GetTextMetrics failed");
                lf.lfWidth = avWidth * request.stretch/100;
                hfont = CreateFontIndirect(&lf);
                if (!hfont)
                    qErrnoWarning("QFontEngine::loadEngine: CreateFontIndirect with stretch failed");
            }

#ifndef Q_WS_WINCE
            if (hfont == 0) {
                hfont = (HFONT)GetStockObject(ANSI_VAR_FONT);
                stockFont = true;
            }
#else
            if (hfont == 0) {
                hfont = (HFONT)GetStockObject(SYSTEM_FONT);
                stockFont = true;
            }
#endif

        }

#if !defined(QT_NO_DIRECTWRITE)
        else {
            // Default to false for DirectWrite (and re-enable once/if everything
            // turns out okay)
            useDirectWrite = false;

            QFontDatabasePrivate *db = privateDb();
            if (db->directWriteFactory == 0) {
                HRESULT hr = DWriteCreateFactory(
                            DWRITE_FACTORY_TYPE_SHARED,
                            __uuidof(IDWriteFactory),
                            reinterpret_cast<IUnknown **>(&db->directWriteFactory)
                            );
                if (FAILED(hr)) {
                    qErrnoWarning("QFontEngine::loadEngine: DWriteCreateFactory failed");
                } else {
                    hr = db->directWriteFactory->GetGdiInterop(&db->directWriteGdiInterop);
                    if (FAILED(hr))
                        qErrnoWarning("QFontEngine::loadEngine: GetGdiInterop failed");
                }
            }

            if (db->directWriteGdiInterop != 0) {
                QString nameSubstitute = fontNameSubstitute(QString::fromWCharArray(lf.lfFaceName));
                memcpy(lf.lfFaceName, nameSubstitute.utf16(),
                       sizeof(wchar_t) * qMin(nameSubstitute.length() + 1, LF_FACESIZE));

                HRESULT hr = db->directWriteGdiInterop->CreateFontFromLOGFONT(
                            &lf,
                            &directWriteFont);
                if (FAILED(hr)) {
#ifndef QT_NO_DEBUG
                    qErrnoWarning("QFontEngine::loadEngine: CreateFontFromLOGFONT failed "
                                  "for %ls (0x%lx)",
                                  lf.lfFaceName, hr);
#endif
                } else {
                    DeleteObject(hfont);
                    useDirectWrite = true;
                }
            }
        }
#endif

    }

    QFontEngine *fe = 0;
    if (!useDirectWrite)  {
        QFontEngineWin *few = new QFontEngineWin(font_name, hfont, stockFont, lf);
        if (preferClearTypeAA)
            few->glyphFormat = QFontEngineGlyphCache::Raster_RGBMask;

        // Also check for OpenType tables when using complex scripts
        // ### TODO: This only works for scripts that require OpenType. More generally
        // for scripts that do not require OpenType we should just look at the list of
        // supported writing systems in the font's OS/2 table.
        if (scriptRequiresOpenType(script)) {
            HB_Face hbFace = few->harfbuzzFace();
            if (!hbFace || !hbFace->supported_scripts[script]) {
                FM_DEBUG("  OpenType support missing for script\n");
                delete few;
                return 0;
            }
        }

        initFontInfo(few, request, fontHdc, dpi);
        fe = few;
    }

#if !defined(QT_NO_DIRECTWRITE)
    else {
        QFontDatabasePrivate *db = privateDb();

        IDWriteFontFace *directWriteFontFace = NULL;
        HRESULT hr = directWriteFont->CreateFontFace(&directWriteFontFace);
        if (SUCCEEDED(hr)) {
            QFontEngineDirectWrite *fedw = new QFontEngineDirectWrite(db->directWriteFactory,
                                                                      directWriteFontFace,
                                                                      request.pixelSize);

            initFontInfo(fedw, request, dpi, directWriteFont);

            fe = fedw;
        } else {
            qErrnoWarning(hr, "QFontEngine::loadEngine: CreateFontFace failed");
        }
    }

    if (directWriteFont != 0)
        directWriteFont->Release();
#endif

    if(script == QUnicodeTables::Common
       && !(request.styleStrategy & QFont::NoFontMerging)
       && desc != 0
       && !(desc->family->writingSystems[QFontDatabase::Symbol] & QtFontFamily::Supported)) {
        if(!tryFonts) {
            LANGID lid = GetUserDefaultLangID();
            switch( lid&0xff ) {
            case LANG_CHINESE: // Chinese (Taiwan)
                if ( lid == 0x0804 ) // Taiwan
                    tryFonts = ch_TW_tryFonts;
                else
                    tryFonts = ch_CN_tryFonts;
                break;
            case LANG_JAPANESE:
                tryFonts = jp_tryFonts;
                break;
            case LANG_KOREAN:
                tryFonts = kr_tryFonts;
                break;
            default:
                tryFonts = other_tryFonts;
                break;
            }
        }
        QStringList fm = QFontDatabase().families();
        QStringList list = family_list;
        const char **tf = tryFonts;
        while(tf && *tf) {
            if(fm.contains(QLatin1String(*tf)))
                list << QLatin1String(*tf);
            ++tf;
        }
        QFontEngine *mfe = new QFontEngineMultiWin(fe, list);
        mfe->fontDef = fe->fontDef;
        fe = mfe;
    }
    return fe;
}

QFontEngine *qt_load_font_engine_win(const QFontDef &request)
{
    // From qfont.cpp
    extern int qt_defaultDpi();

    QFontCache::Key key(request, QUnicodeTables::Common);
    QFontEngine *fe = QFontCache::instance()->findEngine(key);
    if (fe != 0)
        return fe;
    else
        return loadEngine(QUnicodeTables::Common, request, 0, qt_defaultDpi(), false, 0,
                          QStringList());
}

const char *styleHint(const QFontDef &request)
{
    const char *stylehint = 0;
    switch (request.styleHint) {
    case QFont::SansSerif:
        stylehint = "Arial";
        break;
    case QFont::Serif:
        stylehint = "Times New Roman";
        break;
    case QFont::TypeWriter:
        stylehint = "Courier New";
        break;
    default:
        if (request.fixedPitch)
            stylehint = "Courier New";
        break;
    }
    return stylehint;
}

static QFontEngine *loadWin(const QFontPrivate *d, int script, const QFontDef &req)
{
    // list of families to try
    QStringList family_list = familyList(req);

    const char *stylehint = styleHint(d->request);
    if (stylehint)
        family_list << QLatin1String(stylehint);

    // append the default fallback font for the specified script
    // family_list << ... ; ###########

    // add the default family
    QString defaultFamily = QApplication::font().family();
    if (! family_list.contains(defaultFamily))
        family_list << defaultFamily;

    // add QFont::defaultFamily() to the list, for compatibility with
    // previous versions
    family_list << QApplication::font().defaultFamily();

    // null family means find the first font matching the specified script
    family_list << QString();

    QtFontDesc desc;
    QFontEngine *fe = 0;
    QList<int> blacklistedFamilies;

    while (!fe) {
        for (int i = 0; i < family_list.size(); ++i) {
            QString family, foundry;
            parseFontName(family_list.at(i), foundry, family);
            FM_DEBUG("loadWin: >>>>>>>>>>>>>>trying to match '%s'", family.toLatin1().data());
            QT_PREPEND_NAMESPACE(match)(script, req, family, foundry, -1, &desc, blacklistedFamilies);
            if (desc.family)
                break;
        }
        if (!desc.family)
            break;
        fe = loadEngine(script, req, d->hdc, d->dpi, d->rawMode, &desc, family_list);
        if (!fe)
            blacklistedFamilies.append(desc.familyIndex);
    }
    return fe;
}

void QFontDatabase::load(const QFontPrivate *d, int script)
{
    // sanity checks
    if (!qApp)
        qWarning("QFontDatabase::load: Must construct QApplication first");
    Q_ASSERT(script >= 0 && script < QUnicodeTables::ScriptCount);

    // normalize the request to get better caching
    QFontDef req = d->request;
    if (req.pixelSize <= 0)
        req.pixelSize = floor((100.0 * req.pointSize * d->dpi) / 72. + 0.5) / 100;
    if (req.pixelSize < 1)
        req.pixelSize = 1;
    if (req.weight == 0)
        req.weight = QFont::Normal;
    if (req.stretch == 0)
        req.stretch = 100;

    QFontCache::Key key(req, d->rawMode ? QUnicodeTables::Common : script, d->screen);
    if (!d->engineData)
        getEngineData(d, key);

    // the cached engineData could have already loaded the engine we want
    if (d->engineData->engines[script])
        return;

    QFontEngine *fe = QFontCache::instance()->findEngine(key);

    // set it to the actual pointsize, so QFontInfo will do the right thing
    if (req.pointSize < 0)
        req.pointSize = req.pixelSize*72./d->dpi;

    if (!fe) {
        if (qt_enable_test_font && req.family == QLatin1String("__Qt__Box__Engine__")) {
            fe = new QTestFontEngine(req.pixelSize);
            fe->fontDef = req;
        } else {
            QMutexLocker locker(fontDatabaseMutex());
            if (!privateDb()->count)
                initializeDb();
            fe = loadWin(d, script, req);
        }
        if (!fe) {
            fe = new QFontEngineBox(req.pixelSize);
            fe->fontDef = QFontDef();
        }
    }
    d->engineData->engines[script] = fe;
    fe->ref.ref();
    QFontCache::instance()->insertEngine(key, fe);
}

#if !defined(FR_PRIVATE)
#define FR_PRIVATE 0x10
#endif

typedef int (WINAPI *PtrAddFontResourceExW)(LPCWSTR, DWORD, PVOID);
typedef HANDLE (WINAPI *PtrAddFontMemResourceEx)(PVOID, DWORD, PVOID, DWORD *);
typedef BOOL (WINAPI *PtrRemoveFontResourceExW)(LPCWSTR, DWORD, PVOID);
typedef BOOL (WINAPI *PtrRemoveFontMemResourceEx)(HANDLE);

static QList<quint32> getTrueTypeFontOffsets(const uchar *fontData)
{
    QList<quint32> offsets;
    const quint32 headerTag = *reinterpret_cast<const quint32 *>(fontData);
    if (headerTag != MAKE_TAG('t', 't', 'c', 'f')) {
        if (headerTag != MAKE_TAG(0, 1, 0, 0)
            && headerTag != MAKE_TAG('O', 'T', 'T', 'O')
            && headerTag != MAKE_TAG('t', 'r', 'u', 'e')
            && headerTag != MAKE_TAG('t', 'y', 'p', '1'))
            return offsets;
        offsets << 0;
        return offsets;
    }
    const quint32 numFonts = qFromBigEndian<quint32>(fontData + 8);
    for (uint i = 0; i < numFonts; ++i) {
        offsets << qFromBigEndian<quint32>(fontData + 12 + i * 4);
    }
    return offsets;
}

static void getFontTable(const uchar *fileBegin, const uchar *data, quint32 tag, const uchar **table, quint32 *length)
{
    const quint16 numTables = qFromBigEndian<quint16>(data + 4);
    for (uint i = 0; i < numTables; ++i) {
        const quint32 offset = 12 + 16 * i;
        if (*reinterpret_cast<const quint32 *>(data + offset) == tag) {
            *table = fileBegin + qFromBigEndian<quint32>(data + offset + 8);
            *length = qFromBigEndian<quint32>(data + offset + 12);
            return;
        }
    }
    *table = 0;
    *length = 0;
    return;
}

static void getFamiliesAndSignatures(const QByteArray &fontData, QFontDatabasePrivate::ApplicationFont *appFont)
{
    const uchar *data = reinterpret_cast<const uchar *>(fontData.constData());

    QList<quint32> offsets = getTrueTypeFontOffsets(data);
    if (offsets.isEmpty())
        return;

    for (int i = 0; i < offsets.count(); ++i) {
        const uchar *font = data + offsets.at(i);
        const uchar *table;
        quint32 length;
        getFontTable(data, font, MAKE_TAG('n', 'a', 'm', 'e'), &table, &length);
        if (!table)
            continue;
        QString name = getEnglishName(table, length);
        if (name.isEmpty())
            continue;

        appFont->families << name;
        FONTSIGNATURE signature;
        getFontTable(data, font, MAKE_TAG('O', 'S', '/', '2'), &table, &length);
        if (table && length >= 86) {
            // See also qfontdatabase_mac.cpp, offsets taken from OS/2 table in the TrueType spec
            signature.fsUsb[0] = qFromBigEndian<quint32>(table + 42);
            signature.fsUsb[1] = qFromBigEndian<quint32>(table + 46);
            signature.fsUsb[2] = qFromBigEndian<quint32>(table + 50);
            signature.fsUsb[3] = qFromBigEndian<quint32>(table + 54);

            signature.fsCsb[0] = qFromBigEndian<quint32>(table + 78);
            signature.fsCsb[1] = qFromBigEndian<quint32>(table + 82);
        } else {
            memset(&signature, 0, sizeof(signature));
        }
        appFont->signatures << signature;
    }
}

static void registerFont(QFontDatabasePrivate::ApplicationFont *fnt)
{
    if(!fnt->data.isEmpty()) {
#ifndef Q_OS_WINCE
        PtrAddFontMemResourceEx ptrAddFontMemResourceEx = (PtrAddFontMemResourceEx)QSystemLibrary::resolve(QLatin1String("gdi32"),
                                                                                                     "AddFontMemResourceEx");
        if (!ptrAddFontMemResourceEx)
            return;
#endif
        getFamiliesAndSignatures(fnt->data, fnt);
        if (fnt->families.isEmpty())
            return;

#ifdef Q_OS_WINCE
        HANDLE handle = 0;

        {
#ifdef QT_NO_TEMPORARYFILE
           wchar_t lpBuffer[MAX_PATH];
           GetTempPath(MAX_PATH, lpBuffer);
           QString s = QString::fromWCharArray(lpBuffer);
           QFile tempfile(s + QLatin1String("/font") + QString::number(GetTickCount()) + QLatin1String(".ttf"));
           if (!tempfile.open(QIODevice::ReadWrite))
#else
            QTemporaryFile tempfile(QLatin1String("XXXXXXXX.ttf"));
            if (!tempfile.open())
#endif // QT_NO_TEMPORARYFILE
                return;
            if (tempfile.write(fnt->data) == -1)
                return;

#ifndef QT_NO_TEMPORARYFILE
            tempfile.setAutoRemove(false);
#endif
            fnt->fileName = QFileInfo(tempfile.fileName()).absoluteFilePath();
        }

        if (AddFontResource((LPCWSTR)fnt->fileName.utf16()) == 0) {
            QFile(fnt->fileName).remove();
            return;
        }
#else
        DWORD dummy = 0;
        HANDLE handle = ptrAddFontMemResourceEx((void *)fnt->data.constData(), fnt->data.size(), 0,
                                                &dummy);
        if (handle == 0)
            return;
#endif // Q_OS_WINCE

        fnt->handle = handle;
        fnt->data = QByteArray();
        fnt->memoryFont = true;
    } else {
        QFile f(fnt->fileName);
        if (!f.open(QIODevice::ReadOnly))
            return;
        QByteArray data = f.readAll();
        f.close();
        getFamiliesAndSignatures(data, fnt);

#ifdef Q_OS_WINCE
        QFileInfo fileinfo(fnt->fileName);
        fnt->fileName = fileinfo.absoluteFilePath();
        if (AddFontResource((LPCWSTR)fnt->fileName.utf16()) == 0)
            return;
#else
        PtrAddFontResourceExW ptrAddFontResourceExW = (PtrAddFontResourceExW)QSystemLibrary::resolve(QLatin1String("gdi32"),
                                                                                               "AddFontResourceExW");
        if (!ptrAddFontResourceExW
            || ptrAddFontResourceExW((wchar_t*)fnt->fileName.utf16(), FR_PRIVATE, 0) == 0)
            return;
#endif // Q_OS_WINCE

        fnt->memoryFont = false;
    }
}

bool QFontDatabase::removeApplicationFont(int handle)
{
    QMutexLocker locker(fontDatabaseMutex());

    QFontDatabasePrivate *db = privateDb();
    if (handle < 0 || handle >= db->applicationFonts.count())
        return false;

    const QFontDatabasePrivate::ApplicationFont font = db->applicationFonts.at(handle);
    db->applicationFonts[handle] = QFontDatabasePrivate::ApplicationFont();
    if (font.memoryFont) {
#ifdef Q_OS_WINCE
        bool removeSucceeded = RemoveFontResource((LPCWSTR)font.fileName.utf16());
        QFile tempfile(font.fileName);
        tempfile.remove();
        if (!removeSucceeded)
            return false;
#else
        PtrRemoveFontMemResourceEx ptrRemoveFontMemResourceEx = (PtrRemoveFontMemResourceEx)QSystemLibrary::resolve(QLatin1String("gdi32"),
                                                                                                              "RemoveFontMemResourceEx");
        if (!ptrRemoveFontMemResourceEx
            || !ptrRemoveFontMemResourceEx(font.handle))
            return false;
#endif // Q_OS_WINCE
    } else {
#ifdef Q_OS_WINCE
        if (!RemoveFontResource((LPCWSTR)font.fileName.utf16()))
            return false;
#else
        PtrRemoveFontResourceExW ptrRemoveFontResourceExW = (PtrRemoveFontResourceExW)QSystemLibrary::resolve(QLatin1String("gdi32"),
                                                                                                        "RemoveFontResourceExW");
        if (!ptrRemoveFontResourceExW
            || !ptrRemoveFontResourceExW((LPCWSTR)font.fileName.utf16(), FR_PRIVATE, 0))
            return false;
#endif // Q_OS_WINCE
    }

    db->invalidate();
    return true;
}

bool QFontDatabase::removeAllApplicationFonts()
{
    QMutexLocker locker(fontDatabaseMutex());

    QFontDatabasePrivate *db = privateDb();
    for (int i = 0; i < db->applicationFonts.count(); ++i)
        if (!removeApplicationFont(i))
            return false;
    return true;
}

bool QFontDatabase::supportsThreadedFontRendering()
{
    return true;
}

QT_END_NAMESPACE
