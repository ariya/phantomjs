/*
 * Copyright (C) 2011 Brent Fulgham
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 */

#include "config.h"
#include "FontPlatformData.h"

#include "PlatformString.h"
#include <wtf/HashMap.h>
#include <wtf/RetainPtr.h>
#include <wtf/Vector.h>
#include <wtf/text/StringHash.h>

using namespace std;

namespace WebCore {

FontPlatformData::FontPlatformData(const FontPlatformData& source)
    : m_syntheticBold(source.m_syntheticBold)
    , m_syntheticOblique(source.m_syntheticOblique)
    , m_orientation(source.m_orientation)
    , m_textOrientation(source.m_textOrientation)
    , m_size(source.m_size)
    , m_widthVariant(source.m_widthVariant)
    , m_isColorBitmapFont(source.m_isColorBitmapFont)
{
    platformDataInit(source);
}

const FontPlatformData& FontPlatformData::operator=(const FontPlatformData& other)
{
    // Check for self-assignment.
    if (this == &other)
        return *this;

    m_syntheticBold = other.m_syntheticBold;
    m_syntheticOblique = other.m_syntheticOblique;
    m_orientation = other.m_orientation;
    m_textOrientation = other.m_textOrientation;
    m_size = other.m_size;
    m_widthVariant = other.m_widthVariant;
    m_isColorBitmapFont = other.m_isColorBitmapFont;

    return platformDataAssign(other);
}

}
