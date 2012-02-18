/*
    Copyright (C) 2008 Holger Hans Peter Freyther
    Copyright (C) 2009 Torch Mobile Inc. http://www.torchmobile.com/
    Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies)

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

*/

#include "config.h"
#include "FontPlatformData.h"

#include "PlatformString.h"

namespace WebCore {

static inline QFont::Weight toQFontWeight(FontWeight fontWeight)
{
    switch (fontWeight) {
    case FontWeight100:
    case FontWeight200:
        return QFont::Light; // QFont::Light == Weight of 25
    case FontWeight600:
        return QFont::DemiBold; // QFont::DemiBold == Weight of 63
    case FontWeight700:
    case FontWeight800:
        return QFont::Bold; // QFont::Bold == Weight of 75
    case FontWeight900:
        return QFont::Black; // QFont::Black == Weight of 87
    case FontWeight300:
    case FontWeight400:
    case FontWeight500:
    default:
        return QFont::Normal; // QFont::Normal == Weight of 50
    }
}

static inline bool isEmptyValue(const float size, const bool bold, const bool oblique)
{
     // this is the empty value by definition of the trait FontDataCacheKeyTraits
    return !bold && !oblique && size == 0.f;
}

FontPlatformData::FontPlatformData(float size, bool bold, bool oblique)
{
    if (!isEmptyValue(size, bold, oblique))
        m_data = adoptRef(new FontPlatformDataPrivate(size, bold, oblique));
}

FontPlatformData::FontPlatformData(const FontDescription& description, const AtomicString& familyName, int wordSpacing, int letterSpacing)
    : m_data(adoptRef(new FontPlatformDataPrivate()))
{
    QFont& font = m_data->font;
    int requestedSize = qRound(description.computedPixelSize());
    font.setFamily(familyName);
    font.setPixelSize(qRound(requestedSize));
    font.setItalic(description.italic());
    font.setWeight(toQFontWeight(description.weight()));
    font.setWordSpacing(wordSpacing);
    font.setLetterSpacing(QFont::AbsoluteSpacing, letterSpacing);
    const bool smallCaps = description.smallCaps();
    font.setCapitalization(smallCaps ? QFont::SmallCaps : QFont::MixedCase);
    font.setStyleStrategy(QFont::ForceIntegerMetrics);

    m_data->bold = font.bold();
    // WebKit allows font size zero but QFont does not. We will return
    // m_data->size if a font size of zero is requested and pixelSize()
    // otherwise.
    m_data->size = (!requestedSize) ? requestedSize : font.pixelSize();
#if HAVE(QRAWFONT)
    m_data->rawFont = QRawFont::fromFont(font, QFontDatabase::Any);
#endif
}

#if HAVE(QRAWFONT)
FontPlatformData::FontPlatformData(const FontPlatformData& other, float size)
    : m_data(adoptRef(new FontPlatformDataPrivate()))
{
    m_data->font = other.m_data->font;
    m_data->rawFont = other.m_data->rawFont;
    m_data->bold = other.m_data->bold;
    m_data->oblique = other.m_data->oblique;
    m_data->font.setPixelSize(size);
    m_data->rawFont.setPixelSize(size);
    m_data->size = size ? m_data->font.pixelSize() : 0;
}
#endif

bool FontPlatformData::operator==(const FontPlatformData& other) const
{
    if (m_data == other.m_data)
        return true;

    if (!m_data || !other.m_data || m_data->isDeletedValue || other.m_data->isDeletedValue)
        return false;

    const bool equals = (m_data->size == other.m_data->size
                         && m_data->bold == other.m_data->bold
                         && m_data->oblique == other.m_data->oblique
                         && m_data->font == other.m_data->font);
    return equals;
}

unsigned FontPlatformData::hash() const
{
    if (!m_data)
        return 0;
    if (m_data->isDeletedValue)
        return 1;
    return qHash(m_data->font.toString())
           ^ qHash(*reinterpret_cast<quint32*>(&m_data->size))
           ^ qHash(m_data->bold)
           ^ qHash(m_data->oblique);
}

#ifndef NDEBUG
String FontPlatformData::description() const
{
    return String();
}
#endif

}
