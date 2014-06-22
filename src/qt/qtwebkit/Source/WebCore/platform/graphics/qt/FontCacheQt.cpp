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

#include "Font.h"
#include "FontDescription.h"
#include "FontPlatformData.h"
#include <utility>
#include <wtf/ListHashSet.h>
#include <wtf/StdLibExtras.h>
#include <wtf/text/StringHash.h>
#include <wtf/text/WTFString.h>

#include <QFont>
#include <QFontDatabase>
#include <QTextLayout>

using namespace WTF;

namespace WebCore {

void FontCache::platformInit()
{
}

static QRawFont rawFontForCharacters(const QString& string, const QRawFont& font)
{
    QTextLayout layout(string);
    layout.setRawFont(font);
    layout.beginLayout();
    layout.createLine();
    layout.endLayout();

    QList<QGlyphRun> glyphList = layout.glyphRuns();
    ASSERT(glyphList.size() <= 1);
    if (!glyphList.size())
        return QRawFont();

    const QGlyphRun& glyphs(glyphList.at(0));
    return glyphs.rawFont();
}

PassRefPtr<SimpleFontData> FontCache::systemFallbackForCharacters(const FontDescription&, const SimpleFontData* originalFontData, bool, const UChar* characters, int length)
{
    QString qstring = QString::fromRawData(reinterpret_cast<const QChar*>(characters), length);
    QRawFont computedFont = rawFontForCharacters(qstring, originalFontData->getQtRawFont());
    if (!computedFont.isValid())
        return 0;
    FontPlatformData alternateFont(computedFont);
    return getCachedFontData(&alternateFont, DoNotRetain);
}

PassRefPtr<SimpleFontData> FontCache::getLastResortFallbackFont(const FontDescription& fontDescription, ShouldRetain shouldRetain)
{
    const AtomicString fallbackFamily = QFont(fontDescription.firstFamily()).lastResortFamily();
    FontPlatformData platformData(fontDescription, fallbackFamily);
    return getCachedFontData(&platformData, shouldRetain);
}

void FontCache::getTraitsInFamily(const AtomicString&, Vector<unsigned>&)
{
}

PassOwnPtr<FontPlatformData> FontCache::createFontPlatformData(const FontDescription& fontDescription, const AtomicString& familyName)
{
    QFontDatabase db;
    if (!db.hasFamily(familyName))
        return nullptr;
    return adoptPtr(new FontPlatformData(fontDescription, familyName));
}

} // namespace WebCore
