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

#include "config.h"
#include "GlyphPageTreeNode.h"

#include "SimpleFontData.h"
#include <fs_api.h>

namespace WebCore {

template<typename T>
class WorldTypeScopedPtr {
public:
    typedef void (*DestroyFunction)(T*);

    WorldTypeScopedPtr(T* ptr, DestroyFunction destroy)
        : m_ptr(ptr)
        , m_destroy(destroy)
    {
        ASSERT(m_destroy);
    }
    ~WorldTypeScopedPtr()
    {
        if (m_ptr)
            (*m_destroy)(m_ptr);
    }

    T* get() { return m_ptr; }
private:
    T* m_ptr;
    DestroyFunction m_destroy;
};

bool GlyphPage::fill(unsigned offset, unsigned, UChar* buffer, unsigned bufferLength, const SimpleFontData* fontData)
{
    bool haveGlyphs = false;
    unsigned position = 0;
    int i = 0;
    while (position < bufferLength) {
        UChar32 character;
        unsigned nextPosition = position;
        U16_NEXT(buffer, nextPosition, bufferLength, character);

        FS_USHORT glyph = FS_map_char(fontData->platformData().font(), static_cast<FS_ULONG>(character));
        if (!glyph)
            setGlyphDataForIndex(offset + i, 0, 0);
        else {
            haveGlyphs = true;
            setGlyphDataForIndex(offset + i, glyph, fontData);
        }

        i++;
        position = nextPosition;
    }

    return haveGlyphs;
}

}
