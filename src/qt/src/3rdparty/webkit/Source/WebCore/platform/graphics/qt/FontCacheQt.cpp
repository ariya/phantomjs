/*
    Copyright (C) 2008 Nokia Corporation and/or its subsidiary(-ies)
    Copyright (C) 2008 Holger Hans Peter Freyther
    Copyright (C) 2006, 2008 Apple Inc. All rights reserved.
    Copyright (C) 2007 Nicholas Shanks <webkit@nickshanks.com>

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
#include "FontCache.h"

#include "FontDescription.h"
#include "FontPlatformData.h"
#include "Font.h"
#include "PlatformString.h"
#include <utility>
#include <wtf/ListHashSet.h>
#include <wtf/StdLibExtras.h>
#include <wtf/text/StringHash.h>

#include <QFont>
#if HAVE(QRAWFONT)
#include <QTextLayout>
#endif

using namespace WTF;

namespace WebCore {

void FontCache::platformInit()
{
}

#if HAVE(QRAWFONT)
static QRawFont rawFontForCharacters(const QString& string, const QFont& requestedFont)
{
    QFont font(requestedFont);
    font.setStyleStrategy(QFont::NoFontMerging);

    QTextLayout layout(string, font);
    layout.beginLayout();
    layout.createLine();
    layout.endLayout();

    QList<QGlyphs> glyphList = layout.glyphs();

    ASSERT(glyphList.size() == 1);

    const QGlyphs& glyphs(glyphList.at(0));
    QVector<quint32> glyphIndexes = glyphs.glyphIndexes();

    if (glyphIndexes.isEmpty())
        return QRawFont();

    return glyphs.font();
}
#endif

const SimpleFontData* FontCache::getFontDataForCharacters(const Font& font, const UChar* characters, int length)
{
#if HAVE(QRAWFONT)
    QString qstring = QString::fromRawData(reinterpret_cast<const QChar*>(characters), length);
    QRawFont computedFont = rawFontForCharacters(qstring, font.font());
    if (!computedFont.isValid())
        return 0;
    FontPlatformData alternateFont(computedFont);
    return getCachedFontData(&alternateFont);
#else
    Q_UNUSED(font);
    Q_UNUSED(characters);
    Q_UNUSED(length);
    return 0;
#endif
}

SimpleFontData* FontCache::getSimilarFontPlatformData(const Font& font)
{
    return 0;
}

SimpleFontData* FontCache::getLastResortFallbackFont(const FontDescription& fontDescription)
{
    const AtomicString fallbackFamily = QFont(fontDescription.family().family()).lastResortFamily();
    return getCachedFontData(new FontPlatformData(fontDescription, fallbackFamily));
}

void FontCache::getTraitsInFamily(const AtomicString&, Vector<unsigned>&)
{
}

FontPlatformData* FontCache::createFontPlatformData(const FontDescription& fontDescription, const AtomicString& familyName)
{
    return new FontPlatformData(fontDescription, familyName);
}

} // namespace WebCore
