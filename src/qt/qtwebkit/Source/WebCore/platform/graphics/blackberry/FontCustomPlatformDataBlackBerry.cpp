/*
 * Copyright (C) 2012, 2013 Research In Motion Limited. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "config.h"
#include "FontCustomPlatformData.h"

#include "FontPlatformData.h"
#include "ITypeUtils.h"
#include "OpenTypeSanitizer.h"
#include "SharedBuffer.h"

#include <BlackBerryPlatformGraphicsContext.h>
#include <fs_api.h>

namespace WebCore {

FontCustomPlatformData::FontCustomPlatformData(FILECHAR* fontName, PassRefPtr<SharedBuffer> buffer)
    : m_fontName(strdup(fontName))
    , m_buffer(buffer)
{
}

FontCustomPlatformData::~FontCustomPlatformData()
{
    FS_LONG ret = FS_delete_font(BlackBerry::Platform::Graphics::getIType(), const_cast<FILECHAR*>(m_fontName));
    ASSERT_UNUSED(ret, ret == SUCCESS);
    free(m_fontName);
}

FontPlatformData FontCustomPlatformData::fontPlatformData(int size, bool syntheticBold, bool syntheticItalic, FontOrientation orientation, FontWidthVariant widthVariant, FontRenderingMode)
{
    return FontPlatformData(m_fontName, size, syntheticBold, syntheticItalic, orientation, widthVariant);
}

bool FontCustomPlatformData::supportsFormat(const String& format)
{
    bool isSupported = equalIgnoringCase(format, "truetype") || equalIgnoringCase(format, "opentype");
#if USE(OPENTYPE_SANITIZER)
    isSupported = isSupported || equalIgnoringCase(format, "woff");
#endif
    return isSupported;
}

FontCustomPlatformData* createFontCustomPlatformData(SharedBuffer* buffer)
{
    ASSERT_ARG(buffer, buffer);

#if USE(OPENTYPE_SANITIZER)
    OpenTypeSanitizer sanitizer(buffer);
    RefPtr<SharedBuffer> transcodeBuffer = sanitizer.sanitize();
    if (!transcodeBuffer)
        return 0; // validation failed.
    buffer = transcodeBuffer.get();
#endif

    FILECHAR name[MAX_FONT_NAME_LEN+1];
    memset(name, 0, MAX_FONT_NAME_LEN+1);
    if (FS_load_font(BlackBerry::Platform::Graphics::getIType(), 0, const_cast<FS_BYTE*>(reinterpret_cast<const FS_BYTE*>(buffer->data())), 0, MAX_FONT_NAME_LEN, name) != SUCCESS)
        return 0;

    return new FontCustomPlatformData(name, buffer);
}

}
