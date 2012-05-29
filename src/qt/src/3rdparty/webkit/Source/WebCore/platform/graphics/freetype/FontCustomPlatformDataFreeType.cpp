/*
 * Copyright (C) 2008 Alp Toker <alp@atoker.com>
 * Copyright (C) 2010 Igalia S.L.
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

#include "FontPlatformData.h"
#include "SharedBuffer.h"
#include "WOFFFileFormat.h"
#include <cairo-ft.h>
#include <cairo.h>

namespace WebCore {

static void releaseCustomFontData(void* data)
{
    static_cast<SharedBuffer*>(data)->deref();
}

FontCustomPlatformData::FontCustomPlatformData(FT_Face freeTypeFace, SharedBuffer* buffer)
    : m_freeTypeFace(freeTypeFace)
    , m_fontFace(cairo_ft_font_face_create_for_ft_face(freeTypeFace, 0))
{
    // FIXME Should we be setting some hinting options here?

    buffer->ref(); // This is balanced by the buffer->deref() in releaseCustomFontData.
    static cairo_user_data_key_t bufferKey;
    cairo_font_face_set_user_data(m_fontFace, &bufferKey, buffer,
         static_cast<cairo_destroy_func_t>(releaseCustomFontData));

    // Cairo doesn't do FreeType reference counting, so we need to ensure that when
    // this cairo_font_face_t is destroyed, it cleans up the FreeType face as well.
    static cairo_user_data_key_t freeTypeFaceKey;
    cairo_font_face_set_user_data(m_fontFace, &freeTypeFaceKey, freeTypeFace,
         reinterpret_cast<cairo_destroy_func_t>(FT_Done_Face));
}

FontCustomPlatformData::~FontCustomPlatformData()
{
    // m_freeTypeFace will be destroyed along with m_fontFace. See the constructor.
    cairo_font_face_destroy(m_fontFace);
}

FontPlatformData FontCustomPlatformData::fontPlatformData(int size, bool bold, bool italic, FontOrientation, TextOrientation, FontWidthVariant, FontRenderingMode)
{
    return FontPlatformData(m_fontFace, size, bold, italic);
}

FontCustomPlatformData* createFontCustomPlatformData(SharedBuffer* buffer)
{
    ASSERT_ARG(buffer, buffer);

    RefPtr<SharedBuffer> sfntBuffer;
    if (isWOFF(buffer)) {
        Vector<char> sfnt;
        if (!convertWOFFToSfnt(buffer, sfnt))
            return 0;

        sfntBuffer = SharedBuffer::adoptVector(sfnt);
        buffer = sfntBuffer.get();
    }

    static FT_Library library = 0;
    if (!library && FT_Init_FreeType(&library)) {
        library = 0;
        return 0;
    }

    FT_Face freeTypeFace;
    if (FT_New_Memory_Face(library, reinterpret_cast<const FT_Byte*>(buffer->data()), buffer->size(), 0, &freeTypeFace))
        return 0;
    return new FontCustomPlatformData(freeTypeFace, buffer);
}

bool FontCustomPlatformData::supportsFormat(const String& format)
{
    return equalIgnoringCase(format, "truetype") || equalIgnoringCase(format, "opentype") || equalIgnoringCase(format, "woff");
}

}
