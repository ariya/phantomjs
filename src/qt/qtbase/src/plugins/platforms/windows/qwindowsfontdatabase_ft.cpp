/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
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

#include "qwindowsfontdatabase_ft.h"
#include "qwindowsfontdatabase.h"
#include "qwindowscontext.h"

#include <ft2build.h>
#include FT_TRUETYPE_TABLES_H

#include <QtCore/QDir>
#include <QtCore/QDirIterator>
#include <QtCore/QSettings>
#include <QtGui/private/qfontengine_ft_p.h>
#include <QtGui/QGuiApplication>
#include <QtGui/QFontDatabase>

#include <wchar.h>
#ifdef Q_OS_WINCE
#include <QtEndian>
#endif

QT_BEGIN_NAMESPACE

// convert 0 ~ 1000 integer to QFont::Weight
static inline QFont::Weight weightFromInteger(long weight)
{
    if (weight < 400)
        return QFont::Light;
    if (weight < 600)
        return QFont::Normal;
    if (weight < 700)
        return QFont::DemiBold;
    if (weight < 800)
        return QFont::Bold;
    return QFont::Black;
}

static inline QFontDatabase::WritingSystem writingSystemFromCharSet(uchar charSet)
{
    switch (charSet) {
    case ANSI_CHARSET:
    case EASTEUROPE_CHARSET:
    case BALTIC_CHARSET:
    case TURKISH_CHARSET:
        return QFontDatabase::Latin;
    case GREEK_CHARSET:
        return QFontDatabase::Greek;
    case RUSSIAN_CHARSET:
        return QFontDatabase::Cyrillic;
    case HEBREW_CHARSET:
        return QFontDatabase::Hebrew;
    case ARABIC_CHARSET:
        return QFontDatabase::Arabic;
    case THAI_CHARSET:
        return QFontDatabase::Thai;
    case GB2312_CHARSET:
        return QFontDatabase::SimplifiedChinese;
    case CHINESEBIG5_CHARSET:
        return QFontDatabase::TraditionalChinese;
    case SHIFTJIS_CHARSET:
        return QFontDatabase::Japanese;
    case HANGUL_CHARSET:
    case JOHAB_CHARSET:
        return QFontDatabase::Korean;
    case VIETNAMESE_CHARSET:
        return QFontDatabase::Vietnamese;
    case SYMBOL_CHARSET:
        return QFontDatabase::Symbol;
    default:
        break;
    }
    return QFontDatabase::Any;
}

static FontFile * createFontFile(const QString &fileName, int index)
{
    FontFile *fontFile = new FontFile;
    fontFile->fileName = fileName;
    fontFile->indexValue = index;
    return fontFile;
}

extern bool localizedName(const QString &name);
extern QString getEnglishName(const QString &familyName);

static bool addFontToDatabase(const QString &familyName, uchar charSet,
                              const TEXTMETRIC *textmetric,
                              const FONTSIGNATURE *signature,
                              int type)
{
    typedef QPair<QString, QStringList> FontKey;

    // the "@family" fonts are just the same as "family". Ignore them.
    if (familyName.isEmpty() || familyName.at(0) == QLatin1Char('@') || familyName.startsWith(QStringLiteral("WST_")))
        return false;

    const int separatorPos = familyName.indexOf(QStringLiteral("::"));
    const QString faceName =
            separatorPos != -1 ? familyName.left(separatorPos) : familyName;
    const QString fullName =
            separatorPos != -1 ? familyName.mid(separatorPos + 2) : QString();
    static const int SMOOTH_SCALABLE = 0xffff;
    const QString foundryName; // No such concept.
    const NEWTEXTMETRIC *tm = (NEWTEXTMETRIC *)textmetric;
    const bool fixed = !(tm->tmPitchAndFamily & TMPF_FIXED_PITCH);
    const bool ttf = (tm->tmPitchAndFamily & TMPF_TRUETYPE);
    const bool scalable = tm->tmPitchAndFamily & (TMPF_VECTOR|TMPF_TRUETYPE);
    const int size = scalable ? SMOOTH_SCALABLE : tm->tmHeight;
    const QFont::Style style = tm->tmItalic ? QFont::StyleItalic : QFont::StyleNormal;
    const bool antialias = false;
    const QFont::Weight weight = weightFromInteger(tm->tmWeight);
    const QFont::Stretch stretch = QFont::Unstretched;

#ifndef QT_NO_DEBUG_OUTPUT
    if (QWindowsContext::verbose > 2) {
        QString message;
        QTextStream str(&message);
        str << __FUNCTION__ << ' ' << familyName << ' ' << charSet << " TTF=" << ttf;
        if (type & DEVICE_FONTTYPE)
            str << " DEVICE";
        if (type & RASTER_FONTTYPE)
            str << " RASTER";
        if (type & TRUETYPE_FONTTYPE)
            str << " TRUETYPE";
        str << " scalable=" << scalable << " Size=" << size
                << " Style=" << style << " Weight=" << weight
                << " stretch=" << stretch;
        qCDebug(lcQpaFonts) << message;
    }
#endif

    QString englishName;
    if (ttf && localizedName(faceName))
        englishName = getEnglishName(faceName);

    QSupportedWritingSystems writingSystems;
    if (type & TRUETYPE_FONTTYPE) {
        Q_ASSERT(signature);
        quint32 unicodeRange[4] = {
            signature->fsUsb[0], signature->fsUsb[1],
            signature->fsUsb[2], signature->fsUsb[3]
        };
        quint32 codePageRange[2] = {
            signature->fsCsb[0], signature->fsCsb[1]
        };
        writingSystems = QPlatformFontDatabase::writingSystemsFromTrueTypeBits(unicodeRange, codePageRange);
        // ### Hack to work around problem with Thai text on Windows 7. Segoe UI contains
        // the symbol for Baht, and Windows thus reports that it supports the Thai script.
        // Since it's the default UI font on this platform, most widgets will be unable to
        // display Thai text by default. As a temporary work around, we special case Segoe UI
        // and remove the Thai script from its list of supported writing systems.
        if (writingSystems.supported(QFontDatabase::Thai) &&
                faceName == QStringLiteral("Segoe UI"))
            writingSystems.setSupported(QFontDatabase::Thai, false);
    } else {
        const QFontDatabase::WritingSystem ws = writingSystemFromCharSet(charSet);
        if (ws != QFontDatabase::Any)
            writingSystems.setSupported(ws);
    }

#ifndef Q_OS_WINCE
    const QSettings fontRegistry(QStringLiteral("HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Fonts"),
                                QSettings::NativeFormat);

    static QVector<FontKey> allFonts;
    if (allFonts.isEmpty()) {
        const QStringList allKeys = fontRegistry.allKeys();
        allFonts.reserve(allKeys.size());
        const QString trueType = QStringLiteral("(TrueType)");
        const QRegExp sizeListMatch(QStringLiteral("\\s(\\d+,)+\\d+"));
        foreach (const QString &key, allKeys) {
            QString realKey = key;
            realKey.remove(trueType);
            realKey.remove(sizeListMatch);
            QStringList fonts;
            const QStringList fontNames = realKey.trimmed().split(QLatin1Char('&'));
            foreach (const QString &fontName, fontNames)
                fonts.push_back(fontName.trimmed());
            allFonts.push_back(FontKey(key, fonts));
        }
    }

    QString value;
    int index = 0;
    for (int k = 0; k < allFonts.size(); ++k) {
        const FontKey &fontKey = allFonts.at(k);
        for (int i = 0; i < fontKey.second.length(); ++i) {
            const QString &font = fontKey.second.at(i);
            if (font == faceName || fullName == font || englishName == font) {
                value = fontRegistry.value(fontKey.first).toString();
                index = i;
                break;
            }
        }
        if (!value.isEmpty())
            break;
    }
#else
    QString value;
    int index = 0;

    static QHash<QString, QString> fontCache;

    if (fontCache.isEmpty()) {
        QSettings settings(QSettings::SystemScope, QStringLiteral("Qt-Project"), QStringLiteral("Qtbase"));
        settings.beginGroup(QStringLiteral("CEFontCache"));

        foreach (const QString &fontName, settings.allKeys()) {
            const QString fontFileName = settings.value(fontName).toString();
            fontCache.insert(fontName, fontFileName);
        }

        settings.endGroup(); // CEFontCache
    }

    value = fontCache.value(faceName);

    //Fallback if we haven't cached the font yet or the font got removed/renamed iterate again over all fonts
    if (value.isEmpty() || !QFile::exists(value)) {
        QSettings settings(QSettings::SystemScope, QStringLiteral("Qt-Project"), QStringLiteral("Qtbase"));
        settings.beginGroup(QStringLiteral("CEFontCache"));

        //empty the cache first, as it seems that it is dirty
        foreach (const QString &fontName, settings.allKeys())
            settings.remove(fontName);

        QDirIterator it(QStringLiteral("/Windows"), QStringList(QStringLiteral("*.ttf")), QDir::Files | QDir::Hidden | QDir::System);

        while (it.hasNext()) {
            const QString fontFile = it.next();
            const QString fontName = QBasicFontDatabase::fontNameFromTTFile(fontFile);
            if (fontName.isEmpty())
                continue;
            fontCache.insert(fontName, fontFile);
            settings.setValue(fontName, fontFile);

            if (localizedName(fontName)) {
                QString englishFontName = getEnglishName(fontName);
                fontCache.insert(englishFontName, fontFile);
                settings.setValue(englishFontName, fontFile);
            }
        }

        value = fontCache.value(faceName);

        settings.endGroup(); // CEFontCache
    }
#endif

    if (value.isEmpty())
        return false;

    if (!QDir::isAbsolutePath(value))
#ifndef Q_OS_WINCE
        value.prepend(QFile::decodeName(qgetenv("windir") + "\\Fonts\\"));
#else
        value.prepend(QFile::decodeName("/Windows/"));
#endif

    QPlatformFontDatabase::registerFont(faceName, QString(), foundryName, weight, style, stretch,
        antialias, scalable, size, fixed, writingSystems, createFontFile(value, index));

    // add fonts windows can generate for us:
    if (weight <= QFont::DemiBold)
        QPlatformFontDatabase::registerFont(faceName, QString(), foundryName, QFont::Bold, style, stretch,
                                            antialias, scalable, size, fixed, writingSystems, createFontFile(value, index));

    if (style != QFont::StyleItalic)
        QPlatformFontDatabase::registerFont(faceName, QString(), foundryName, weight, QFont::StyleItalic, stretch,
                                            antialias, scalable, size, fixed, writingSystems, createFontFile(value, index));

    if (weight <= QFont::DemiBold && style != QFont::StyleItalic)
        QPlatformFontDatabase::registerFont(faceName, QString(), foundryName, QFont::Bold, QFont::StyleItalic, stretch,
                                            antialias, scalable, size, fixed, writingSystems, createFontFile(value, index));

    if (!englishName.isEmpty())
        QPlatformFontDatabase::registerAliasToFontFamily(faceName, englishName);

    return true;
}

#ifdef Q_OS_WINCE
static QByteArray getFntTable(HFONT hfont, uint tag)
{
    HDC hdc = GetDC(0);
    HGDIOBJ oldFont = SelectObject(hdc, hfont);
    quint32 t = qFromBigEndian<quint32>(tag);
    QByteArray buffer;

    DWORD length = GetFontData(hdc, t, 0, NULL, 0);
    if (length != GDI_ERROR) {
        buffer.resize(length);
        GetFontData(hdc, t, 0, reinterpret_cast<uchar *>(buffer.data()), length);
    }
    SelectObject(hdc, oldFont);
    return buffer;
}
#endif

static int QT_WIN_CALLBACK storeFont(ENUMLOGFONTEX* f, NEWTEXTMETRICEX *textmetric,
                                     int type, LPARAM namesSetIn)
{
    typedef QSet<QString> StringSet;
    const QString familyName = QString::fromWCharArray(f->elfLogFont.lfFaceName)
                               + QStringLiteral("::")
                               + QString::fromWCharArray(f->elfFullName);
    const uchar charSet = f->elfLogFont.lfCharSet;

#ifndef Q_OS_WINCE
    const FONTSIGNATURE signature = textmetric->ntmFontSig;
#else
    FONTSIGNATURE signature;
    QByteArray table;

    if (type & TRUETYPE_FONTTYPE) {
        HFONT hfont = CreateFontIndirect(&f->elfLogFont);
        table = getFntTable(hfont, MAKE_TAG('O', 'S', '/', '2'));
        DeleteObject((HGDIOBJ)hfont);
    }

    if (table.length() >= 86) {
        // See also qfontdatabase_mac.cpp, offsets taken from OS/2 table in the TrueType spec
        uchar *tableData = reinterpret_cast<uchar *>(table.data());

        signature.fsUsb[0] = qFromBigEndian<quint32>(tableData + 42);
        signature.fsUsb[1] = qFromBigEndian<quint32>(tableData + 46);
        signature.fsUsb[2] = qFromBigEndian<quint32>(tableData + 50);
        signature.fsUsb[3] = qFromBigEndian<quint32>(tableData + 54);

        signature.fsCsb[0] = qFromBigEndian<quint32>(tableData + 78);
        signature.fsCsb[1] = qFromBigEndian<quint32>(tableData + 82);
    } else {
        memset(&signature, 0, sizeof(signature));
    }
#endif

    // NEWTEXTMETRICEX is a NEWTEXTMETRIC, which according to the documentation is
    // identical to a TEXTMETRIC except for the last four members, which we don't use
    // anyway
    if (addFontToDatabase(familyName, charSet, (TEXTMETRIC *)textmetric, &signature, type))
        reinterpret_cast<StringSet *>(namesSetIn)->insert(familyName);

    // keep on enumerating
    return 1;
}

void QWindowsFontDatabaseFT::populateFontDatabase()
{
    m_families.clear();
    populate(); // Called multiple times.
    // Work around EnumFontFamiliesEx() not listing the system font, see below.
    const QString sysFontFamily = QGuiApplication::font().family();
    if (!m_families.contains(sysFontFamily))
         populate(sysFontFamily);
}

/*!
    \brief Populate font database using EnumFontFamiliesEx().

    Normally, leaving the name empty should enumerate
    all fonts, however, system fonts like "MS Shell Dlg 2"
    are only found when specifying the name explicitly.
*/

void QWindowsFontDatabaseFT::populate(const QString &family)
    {

    qCDebug(lcQpaFonts) << __FUNCTION__ << m_families.size() << family;

    HDC dummy = GetDC(0);
    LOGFONT lf;
    lf.lfCharSet = DEFAULT_CHARSET;
    if (family.size() >= LF_FACESIZE) {
        qWarning("%s: Unable to enumerate family '%s'.",
                 __FUNCTION__, qPrintable(family));
        return;
    }
    wmemcpy(lf.lfFaceName, reinterpret_cast<const wchar_t*>(family.utf16()),
            family.size() + 1);
    lf.lfPitchAndFamily = 0;
    EnumFontFamiliesEx(dummy, &lf, (FONTENUMPROC)storeFont,
                       (LPARAM)&m_families, 0);
    ReleaseDC(0, dummy);
}

QFontEngine * QWindowsFontDatabaseFT::fontEngine(const QFontDef &fontDef, void *handle)
{
    QFontEngine *fe = QBasicFontDatabase::fontEngine(fontDef, handle);
    qCDebug(lcQpaFonts) << __FUNCTION__ << "FONTDEF" << fontDef.family << fe << handle;
    return fe;
}

QFontEngine *QWindowsFontDatabaseFT::fontEngine(const QByteArray &fontData, qreal pixelSize, QFont::HintingPreference hintingPreference)
{
    QFontEngine *fe = QBasicFontDatabase::fontEngine(fontData, pixelSize, hintingPreference);
    qCDebug(lcQpaFonts) << __FUNCTION__ << "FONTDATA" << fontData << pixelSize << hintingPreference << fe;
    return fe;
}

QStringList QWindowsFontDatabaseFT::fallbacksForFamily(const QString &family, QFont::Style style, QFont::StyleHint styleHint, QChar::Script script) const
{
    QStringList result = QPlatformFontDatabase::fallbacksForFamily(family, style, styleHint, script);
    if (!result.isEmpty())
        return result;

    switch (styleHint) {
        case QFont::Times:
            result << QString::fromLatin1("Times New Roman");
            break;
        case QFont::Courier:
            result << QString::fromLatin1("Courier New");
            break;
        case QFont::Monospace:
            result << QString::fromLatin1("Courier New");
            break;
        case QFont::Cursive:
            result << QString::fromLatin1("Comic Sans MS");
            break;
        case QFont::Fantasy:
            result << QString::fromLatin1("Impact");
            break;
        case QFont::Decorative:
            result << QString::fromLatin1("Old English");
            break;
        case QFont::Helvetica:
        case QFont::System:
        default:
            result << QString::fromLatin1("Arial");
    }

#ifdef Q_OS_WINCE
    QSettings settings(QLatin1String("HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\FontLink\\SystemLink"), QSettings::NativeFormat);
    const QStringList fontList = settings.value(family).toStringList();
    foreach (const QString &fallback, fontList) {
        const int sep = fallback.indexOf(QLatin1Char(','));
        if (sep > 0)
            result << fallback.mid(sep + 1);
    }
#endif

    result.append(QWindowsFontDatabase::extraTryFontsForFamily(family));

    qCDebug(lcQpaFonts) << __FUNCTION__ << family << style << styleHint
        << script << result << m_families;

    return result;
}
QString QWindowsFontDatabaseFT::fontDir() const
{
    const QString result = QLatin1String(qgetenv("windir")) + QLatin1String("/Fonts");//QPlatformFontDatabase::fontDir();
    qCDebug(lcQpaFonts) << __FUNCTION__ << result;
    return result;
}

QFont QWindowsFontDatabaseFT::defaultFont() const
{
    return QWindowsFontDatabase::systemDefaultFont();
}

QT_END_NAMESPACE
