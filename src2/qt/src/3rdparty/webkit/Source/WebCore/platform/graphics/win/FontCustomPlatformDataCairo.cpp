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
#include "FontCustomPlatformDataCairo.h"

#include "SharedBuffer.h"
#include "FontPlatformData.h"

#include <cairo-win32.h>
#include <wtf/RetainPtr.h>


namespace WebCore {

FontCustomPlatformData::~FontCustomPlatformData()
{
   cairo_font_face_destroy(m_fontFace);
}

FontPlatformData FontCustomPlatformData::fontPlatformData(int size, bool bold, bool italic, FontOrientation, TextOrientation, FontWidthVariant)
{
    return FontPlatformData(m_fontFace, size, bold, italic);
}

static void releaseData(void* data)
{
    static_cast<SharedBuffer*>(data)->deref();
}

FontCustomPlatformData* createFontCustomPlatformData(SharedBuffer* buffer)
{
    ASSERT_ARG(buffer, buffer);

    buffer->ref();
    HFONT font = reinterpret_cast<HFONT>(buffer);
    cairo_font_face_t* fontFace = cairo_win32_font_face_create_for_hfont(font);
    if (!fontFace)
       return 0;

    static cairo_user_data_key_t bufferKey;
    cairo_font_face_set_user_data(fontFace, &bufferKey, buffer, releaseData);

    return new FontCustomPlatformData(fontFace);
}

bool FontCustomPlatformData::supportsFormat(const String& format)
{
    return equalIgnoringCase(format, "truetype") || equalIgnoringCase(format, "opentype");
}

}
