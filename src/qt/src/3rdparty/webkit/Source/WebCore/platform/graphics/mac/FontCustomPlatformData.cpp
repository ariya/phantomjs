/*
 * Copyright (C) 2007, 2008, 2010 Apple Inc. All rights reserved.
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
#include "OpenTypeSanitizer.h"
#include "SharedBuffer.h"
#include "WOFFFileFormat.h"
#include <ApplicationServices/ApplicationServices.h>

namespace WebCore {

FontCustomPlatformData::~FontCustomPlatformData()
{
#ifdef BUILDING_ON_LEOPARD
    if (m_atsContainer)
        ATSFontDeactivate(m_atsContainer, NULL, kATSOptionFlagsDefault);
#endif
    CGFontRelease(m_cgFont);
}

FontPlatformData FontCustomPlatformData::fontPlatformData(int size, bool bold, bool italic, FontOrientation orientation, TextOrientation textOrientation, FontWidthVariant widthVariant, FontRenderingMode)
{
    return FontPlatformData(m_cgFont, size, bold, italic, orientation, textOrientation, widthVariant);
}

FontCustomPlatformData* createFontCustomPlatformData(SharedBuffer* buffer)
{
    ASSERT_ARG(buffer, buffer);

#if ENABLE(OPENTYPE_SANITIZER)
    OpenTypeSanitizer sanitizer(buffer);
    RefPtr<SharedBuffer> transcodeBuffer = sanitizer.sanitize();
    if (!transcodeBuffer)
        return 0; // validation failed.
    buffer = transcodeBuffer.get();
#else
    RefPtr<SharedBuffer> sfntBuffer;
    if (isWOFF(buffer)) {
        Vector<char> sfnt;
        if (!convertWOFFToSfnt(buffer, sfnt))
            return 0;

        sfntBuffer = SharedBuffer::adoptVector(sfnt);
        buffer = sfntBuffer.get();
    }
#endif

    ATSFontContainerRef containerRef = 0;

    RetainPtr<CGFontRef> cgFontRef;

#ifndef BUILDING_ON_LEOPARD
    RetainPtr<CFDataRef> bufferData(AdoptCF, buffer->createCFData());
    RetainPtr<CGDataProviderRef> dataProvider(AdoptCF, CGDataProviderCreateWithCFData(bufferData.get()));

    cgFontRef.adoptCF(CGFontCreateWithDataProvider(dataProvider.get()));
    if (!cgFontRef)
        return 0;
#else
    // Use ATS to activate the font.

    // The value "3" means that the font is private and can't be seen by anyone else.
    ATSFontActivateFromMemory((void*)buffer->data(), buffer->size(), 3, kATSFontFormatUnspecified, NULL, kATSOptionFlagsDefault, &containerRef);
    if (!containerRef)
        return 0;
    ItemCount fontCount;
    ATSFontFindFromContainer(containerRef, kATSOptionFlagsDefault, 0, NULL, &fontCount);
    
    // We just support the first font in the list.
    if (fontCount == 0) {
        ATSFontDeactivate(containerRef, NULL, kATSOptionFlagsDefault);
        return 0;
    }
    
    ATSFontRef fontRef = 0;
    ATSFontFindFromContainer(containerRef, kATSOptionFlagsDefault, 1, &fontRef, NULL);
    if (!fontRef) {
        ATSFontDeactivate(containerRef, NULL, kATSOptionFlagsDefault);
        return 0;
    }
    
    cgFontRef.adoptCF(CGFontCreateWithPlatformFont(&fontRef));
    // Workaround for <rdar://problem/5675504>.
    if (cgFontRef && !CGFontGetNumberOfGlyphs(cgFontRef.get()))
        cgFontRef = 0;
    if (!cgFontRef) {
        ATSFontDeactivate(containerRef, NULL, kATSOptionFlagsDefault);
        return 0;
    }
#endif // !defined(BUILDING_ON_LEOPARD)

    return new FontCustomPlatformData(containerRef, cgFontRef.releaseRef());
}

bool FontCustomPlatformData::supportsFormat(const String& format)
{
    return equalIgnoringCase(format, "truetype") || equalIgnoringCase(format, "opentype") || equalIgnoringCase(format, "woff");
}

}
