/*
    Copyright (C) 2008, 2009, 2010, 2011 Nokia Corporation and/or its subsidiary(-ies)
    Copyright (C) 2008 Holger Hans Peter Freyther

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
#include "SimpleFontData.h"

#if HAVE(QRAWFONT)
#include "NotImplemented.h"
#else
#include <QFontMetricsF>
#endif

namespace WebCore {

void SimpleFontData::determinePitch()
{
#if HAVE(QRAWFONT)
    notImplemented();
    m_treatAsFixedPitch = false;
#else
    m_treatAsFixedPitch = m_platformData.font().fixedPitch();
#endif
}

#if HAVE(QRAWFONT)

static const float smallCapsFraction = 0.7;
static const float emphasisMarkFraction = 0.5;

bool SimpleFontData::containsCharacters(const UChar* characters, int length) const
{
    QRawFont rawFont(m_platformData.rawFont());

    for (int i = 0; i < length; ++i) {
        if (!rawFont.supportsCharacter(static_cast<QChar>(characters[i])))
            return false;
    }

    return true;
}

float SimpleFontData::platformWidthForGlyph(Glyph glyph) const
{
    if (!platformData().size())
        return 0;

    QVector<quint32> glyphIndexes;
    glyphIndexes.append(glyph);
    QVector<QPointF> advances = platformData().rawFont().advancesForGlyphIndexes(glyphIndexes);
    ASSERT(!advances.isEmpty());
    return advances.at(0).x();
}

SimpleFontData* SimpleFontData::scaledFontData(const FontDescription& fontDescription, float scaleFactor) const
{
    const float scaledSize = lroundf(fontDescription.computedSize() * scaleFactor);
    return new SimpleFontData(FontPlatformData(m_platformData, scaledSize), isCustomFont(), false);
}

SimpleFontData* SimpleFontData::smallCapsFontData(const FontDescription& fontDescription) const
{
    if (!m_derivedFontData)
        m_derivedFontData = DerivedFontData::create(isCustomFont());
    if (!m_derivedFontData->smallCaps)
        m_derivedFontData->smallCaps = scaledFontData(fontDescription, smallCapsFraction);

    return m_derivedFontData->smallCaps.get();
}

SimpleFontData* SimpleFontData::emphasisMarkFontData(const FontDescription& fontDescription) const
{
    if (!m_derivedFontData)
        m_derivedFontData = DerivedFontData::create(isCustomFont());
    if (!m_derivedFontData->emphasisMark)
        m_derivedFontData->emphasisMark = scaledFontData(fontDescription, emphasisMarkFraction);

    return m_derivedFontData->emphasisMark.get();
}

FloatRect SimpleFontData::platformBoundsForGlyph(Glyph) const
{
    notImplemented();
    return FloatRect();
}
#else
bool SimpleFontData::containsCharacters(const UChar*, int) const
{
    return true;
}

void SimpleFontData::platformGlyphInit()
{
    if (!m_platformData.size())
        return;
    m_spaceGlyph = 0;
    determinePitch();
    m_missingGlyphData.fontData = this;
    m_missingGlyphData.glyph = 0;
}
#endif

void SimpleFontData::platformInit()
{
    if (!m_platformData.size()) {
         m_fontMetrics.reset();
         m_avgCharWidth = 0;
         m_maxCharWidth = 0;
         return;
    }

#if HAVE(QRAWFONT)
    QRawFont rawFont(m_platformData.rawFont());
    float descent = rawFont.descent();
    float ascent = rawFont.ascent();
    float xHeight = rawFont.xHeight();
    float lineSpacing = ascent + descent + rawFont.leading() + 1;

    QVector<quint32> indexes = rawFont.glyphIndexesForString(QLatin1String(" "));
    QVector<QPointF> advances = rawFont.advancesForGlyphIndexes(indexes);
    float spaceWidth = advances.at(0).x();
#else
    QFontMetricsF fm(m_platformData.font());
    float descent = fm.descent();
    float ascent = fm.ascent();
    float xHeight = fm.xHeight();
    float lineSpacing = fm.lineSpacing();
    float spaceWidth = fm.width(QLatin1Char(' '));
#endif

    // Qt subtracts 1 from the descent to account for the baseline,
    // we add it back here to get correct metrics for WebKit.
    descent += 1;

    // The line spacing should always be >= (ascent + descent), but this
    // may be false in some cases due to misbehaving platform libraries.
    // Workaround from SimpleFontPango.cpp and SimpleFontFreeType.cpp
    if (lineSpacing < ascent + descent)
        lineSpacing = ascent + descent;

    // QFontMetricsF::leading() may return negative values on platforms
    // such as FreeType. Calculate the line gap manually instead.
    float lineGap = lineSpacing - ascent - descent;

    m_fontMetrics.setAscent(ascent);
    m_fontMetrics.setDescent(descent);
    m_fontMetrics.setLineSpacing(lineSpacing);
    m_fontMetrics.setXHeight(xHeight);
    m_fontMetrics.setLineGap(lineGap);
    m_spaceWidth = spaceWidth;
}

void SimpleFontData::platformCharWidthInit()
{
    if (!m_platformData.size())
        return;
#if HAVE(QRAWFONT)
    QRawFont rawFont(m_platformData.rawFont());
    m_avgCharWidth = rawFont.averageCharWidth();
    m_maxCharWidth = rawFont.maxCharWidth();
#else
    QFontMetricsF fm(m_platformData.font());
    m_avgCharWidth = fm.averageCharWidth();
    m_maxCharWidth = fm.maxWidth();
#endif
}

void SimpleFontData::platformDestroy()
{
}

}
