/*
 * Copyright (C) 2006, 2007, 2008 Apple Inc.
 * Copyright (C) 2006 Michael Emmel mike.emmel@gmail.com
 * Copyright (C) 2007 Holger Hans Peter Freyther
 * Copyright (C) 2007 Pioneer Research Center USA, Inc.
 * All rights reserved.
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

#ifndef FontPlatformDataPango_h
#define FontPlatformDataPango_h

#include "FontDescription.h"
#include "FontOrientation.h"
#include "GlyphBuffer.h"
#include <cairo.h>
#include <pango/pangocairo.h>
#include <wtf/Forward.h>

namespace WebCore {

class FontPlatformData {
public:
    FontPlatformData(WTF::HashTableDeletedValueType)
        : m_context(0)
        , m_font(hashTableDeletedFontValue())
        , m_size(0)
        , m_syntheticBold(false)
        , m_syntheticOblique(false)
        , m_scaledFont(0)
        { }

    FontPlatformData()
        : m_context(0)
        , m_font(0)
        , m_size(0)
        , m_syntheticBold(false)
        , m_syntheticOblique(false)
        , m_scaledFont(0)
        { }

    FontPlatformData(const FontDescription&, const AtomicString& family);
    FontPlatformData(cairo_font_face_t* fontFace, float size, bool bold, bool italic);
    FontPlatformData(float size, bool bold, bool italic);
    FontPlatformData(const FontPlatformData&);
    ~FontPlatformData();

    static bool init();
    bool isFixedPitch();
    float size() const { return m_size; }
    void setSize(float size) { m_size = size; }
    bool syntheticBold() const { return m_syntheticBold; }
    bool syntheticOblique() const { return m_syntheticOblique; }

    FontOrientation orientation() const { return Horizontal; } // FIXME: Implement.
    void setOrientation(FontOrientation) { } // FIXME: Implement.

    cairo_scaled_font_t* scaledFont() const { return m_scaledFont; }

    unsigned hash() const
    {
        uintptr_t hashCodes[1] = { reinterpret_cast<uintptr_t>(m_scaledFont) };
        return StringHasher::hashMemory<sizeof(hashCodes)>(hashCodes);
    }

    bool operator==(const FontPlatformData&) const;
    FontPlatformData& operator=(const FontPlatformData&);
    bool isHashTableDeletedValue() const
    {
        return m_font == hashTableDeletedFontValue();
    }

#ifndef NDEBUG
    String description() const;
#endif

    static PangoFontMap* m_fontMap;
    static GHashTable* m_hashTable;
    PangoContext* m_context;
    PangoFont* m_font;
    float m_size;
    bool m_syntheticBold;
    bool m_syntheticOblique;
    cairo_scaled_font_t* m_scaledFont;
private:
    static PangoFont *hashTableDeletedFontValue() { return reinterpret_cast<PangoFont*>(-1); }
};

}

#endif // FontPlatformDataPango_h
