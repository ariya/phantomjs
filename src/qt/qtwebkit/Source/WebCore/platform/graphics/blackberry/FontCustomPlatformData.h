/*
 * Copyright (C) 2012 Research In Motion Limited. All rights reserved.
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
#ifndef FontCustomPlatformData_h
#define FontCustomPlatformData_h

#include "FontOrientation.h"
#include "FontRenderingMode.h"
#include "FontWidthVariant.h"
#include <wtf/Noncopyable.h>
#include <wtf/text/WTFString.h>

typedef char FILECHAR;
struct FS_STATE;

namespace WebCore {

class FontPlatformData;
class MonotypeFont;
class SharedBuffer;

struct FontCustomPlatformData {
    WTF_MAKE_NONCOPYABLE(FontCustomPlatformData);
public:
    FontCustomPlatformData(FILECHAR* fontName, PassRefPtr<SharedBuffer>);
    ~FontCustomPlatformData();

    FontPlatformData fontPlatformData(int size, bool syntheticBold, bool syntheticItalic, FontOrientation = Horizontal,
        FontWidthVariant = RegularWidth, FontRenderingMode = NormalRenderingMode);

    static bool supportsFormat(const String&);

    FILECHAR* m_fontName;
    RefPtr<SharedBuffer> m_buffer;
};

FontCustomPlatformData* createFontCustomPlatformData(SharedBuffer*);

} // namespace WebCore

#endif // FontCustomPlatformData_h
