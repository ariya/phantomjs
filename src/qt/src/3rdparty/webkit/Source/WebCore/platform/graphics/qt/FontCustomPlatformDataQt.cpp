/*
    Copyright (C) 2008 Nokia Corporation and/or its subsidiary(-ies)

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.

    This class provides all functionality needed for loading images, style sheets and html
    pages from the web. It has a memory cache for these objects.
*/
#include "config.h"
#include "FontCustomPlatformData.h"

#include "FontPlatformData.h"
#include "SharedBuffer.h"
#include "WOFFFileFormat.h"
#include <QFontDatabase>
#include <QMap>
#include <QStringList>

namespace WebCore {

FontCustomPlatformData::~FontCustomPlatformData()
{
#if !defined(Q_OS_MAC)
    // On Mac OS X, do not remove the font because it may crash.
    // For details, see issue 690 (http://code.google.com/p/phantomjs/issues/detail?id=690).
    QFontDatabase::removeApplicationFont(m_handle);
#endif
}

FontPlatformData FontCustomPlatformData::fontPlatformData(int size, bool syntheticBold, bool syntheticItalic, FontOrientation, TextOrientation, FontWidthVariant, FontRenderingMode)
{
    // Note that syntheticBold and syntheticItalic are only set by CSSFontSelector
    // (the sole user of this class) if the markup has requested bold or italic
    // (e.g. <i>text</i>) but the @font-face rule for the font does not specify support
    // for it (e.g. @font-face { font-family: "font"; font-style: normal;).

    QFont font;
    QFontDatabase db;
    
    QString family = QFontDatabase::applicationFontFamilies(m_handle)[0];
    font.setFamily(family);
    font.setPixelSize(size);
    if (m_bold || syntheticBold)
        font.setWeight(QFont::Bold);
    font.setItalic(m_italic || syntheticItalic);

    return FontPlatformData(font);
}

static QMap<QString, QStringList> currentStyles(const QStringList& families, const QFontDatabase& db)
{
    QMap<QString, QStringList> styles;
    for (int i = 0; i < families.size(); ++i)
        styles[families.at(i)].append(db.styles(families.at(i)));
    return styles;
}

static QStringList stylesAddedByFont(const QString& familyAdded, const QStringList& stylesAdded, const QMap<QString, QStringList>& styles)
{
    QStringList newStyles = stylesAdded;
    for (int i = 0; i < styles[familyAdded].size(); ++i)
        newStyles.removeAll(styles[familyAdded].at(i));
    return newStyles;
}

static bool strictlyItalicAddedByFont(const QString& familyAdded, const QStringList& stylesAdded, const QFontDatabase& db)
{
    bool italic = false;
    for (int i = 0; i < stylesAdded.size(); ++i) {
        if (db.italic(familyAdded, stylesAdded.at(i))) {
            italic = true;
        } else if (!db.bold(familyAdded, stylesAdded.at(i)))
            return false;
    }
    return italic;
}

static bool strictlyBoldAddedByFont(const QString& familyAdded, const QStringList& stylesAdded, const QFontDatabase& db)
{
    bool bold = false;
    for (int i = 0; i < stylesAdded.size(); ++i) {
        if (db.bold(familyAdded, stylesAdded.at(i))) {
            bold = true;
        } else if (!db.italic(familyAdded, stylesAdded.at(i)))
            return false;
    }
    return bold;
}

FontCustomPlatformData* createFontCustomPlatformData(SharedBuffer* buffer)
{
    ASSERT_ARG(buffer, buffer);
    QFontDatabase db;

    // The font families in the font database, and the styles in each,
    // before we load the new font in buffer.
    QStringList families = db.families();
    QMap<QString, QStringList> styles = currentStyles(families, db);

    RefPtr<SharedBuffer> sfntBuffer;
    if (isWOFF(buffer)) {
        Vector<char> sfnt;
        if (!convertWOFFToSfnt(buffer, sfnt))
            return 0;
        sfntBuffer = SharedBuffer::adoptVector(sfnt);
        buffer = sfntBuffer.get();
    }

    int id = QFontDatabase::addApplicationFontFromData(QByteArray(buffer->data(), buffer->size()));
    if (id == -1)
        return 0;

    QString familyAdded = QFontDatabase::applicationFontFamilies(id)[0];
    QStringList stylesAdded = db.styles(QFontDatabase::applicationFontFamilies(id)[0]);

    // If we already had the family of which this font is a member then
    // get the styles it added to the family
    if (families.contains(familyAdded))
        stylesAdded = stylesAddedByFont(familyAdded, stylesAdded, styles);

    Q_ASSERT(QFontDatabase::applicationFontFamilies(id).size() > 0);

    FontCustomPlatformData *data = new FontCustomPlatformData;
    data->m_handle = id;

    // If we have created a font that only has bold or italic styles (or both)
    // then we need to respect it's style(s) when we pass it back as
    // FontPlatformData above.
    data->m_italic = strictlyItalicAddedByFont(familyAdded, stylesAdded, db);
    data->m_bold = strictlyBoldAddedByFont(familyAdded, stylesAdded, db);
    return data;
}

bool FontCustomPlatformData::supportsFormat(const String& format)
{
    return equalIgnoringCase(format, "truetype") || equalIgnoringCase(format, "opentype") || equalIgnoringCase(format, "woff");
}

}
