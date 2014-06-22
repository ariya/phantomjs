/*
 * Copyright (C) 2007, 2008 Apple Computer, Inc.
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
#include "FontCustomPlatformData.h"

#include "OpenTypeUtilities.h"
#include "SharedBuffer.h"
#include "FontPlatformData.h"

#include <cairo-win32.h>
#include <wtf/RetainPtr.h>
#include <wtf/text/Base64.h>

namespace WebCore {

FontCustomPlatformData::~FontCustomPlatformData()
{
    if (m_fontReference)
        RemoveFontMemResourceEx(m_fontReference);
}

FontPlatformData FontCustomPlatformData::fontPlatformData(int size, bool bold, bool italic, FontOrientation, FontWidthVariant, FontRenderingMode renderingMode)
{
    LOGFONT logFont;
    memset(&logFont, 0, sizeof(LOGFONT));
    wcsncpy(logFont.lfFaceName, m_name.charactersWithNullTermination().data(), LF_FACESIZE - 1);

    logFont.lfHeight = -size;
    if (renderingMode == NormalRenderingMode)
        logFont.lfHeight *= 32;
    logFont.lfWidth = 0;
    logFont.lfEscapement = 0;
    logFont.lfOrientation = 0;
    logFont.lfUnderline = false;
    logFont.lfStrikeOut = false;
    logFont.lfCharSet = DEFAULT_CHARSET;
    logFont.lfOutPrecision = OUT_TT_ONLY_PRECIS;
    logFont.lfQuality = CLEARTYPE_QUALITY;
    logFont.lfPitchAndFamily = DEFAULT_PITCH | FF_DONTCARE;
    logFont.lfItalic = italic;
    logFont.lfWeight = bold ? 700 : 400;

    HFONT hfont = CreateFontIndirect(&logFont);

    cairo_font_face_t* fontFace = cairo_win32_font_face_create_for_hfont(hfont);

    FontPlatformData fontPlatformData(hfont, fontFace, size, bold, italic);

    cairo_font_face_destroy(fontFace);

    return fontPlatformData;
}

static String createUniqueFontName()
{
    GUID fontUuid;
    CoCreateGuid(&fontUuid);

    String fontName = base64Encode(reinterpret_cast<char*>(&fontUuid), sizeof(fontUuid));
    ASSERT(fontName.length() < LF_FACESIZE);
    return fontName;
}

FontCustomPlatformData* createFontCustomPlatformData(SharedBuffer* buffer)
{
    ASSERT_ARG(buffer, buffer);

    String fontName = createUniqueFontName();
    HANDLE fontReference = renameAndActivateFont(buffer, fontName);

    if (!fontReference)
        return 0;

    return new FontCustomPlatformData(fontReference, fontName);
}

bool FontCustomPlatformData::supportsFormat(const String& format)
{
    return equalIgnoringCase(format, "truetype") || equalIgnoringCase(format, "opentype");
}

}
