/*
 * This file is part of the internal font implementation.  It should not be included by anyone other than
 * FontMac.cpp, FontWin.cpp and Font.cpp.
 *
 * Copyright (C) 2006, 2007 Apple Inc.
 * Copyright (C) 2007-2008 Torch Mobile, Inc.
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

#ifndef FontPlatformData_h
#define FontPlatformData_h

#include "FontDescription.h"
#include "FontOrientation.h"
#include <wtf/Forward.h>
#include <wtf/Noncopyable.h>
#include <wtf/text/StringImpl.h>

typedef struct tagTEXTMETRICW TEXTMETRIC;
typedef struct tagLOGFONTW LOGFONT;

namespace WebCore {

    class FontPlatformPrivateData;

    class FontPlatformData {

    public:

        FontPlatformData(): m_private(0) {}
        FontPlatformData(float size, bool bold, bool oblique);

        // Used for deleted values in the font cache's hash tables.
        FontPlatformData(WTF::HashTableDeletedValueType) : m_private((FontPlatformPrivateData*)1) {}
        bool isHashTableDeletedValue() const { return (unsigned)m_private == 1; }

        FontPlatformData(const FontDescription& fontDescription, const AtomicString& family, bool useDefaultFontIfNotPresent = true);

        ~FontPlatformData();

        FontPlatformData(const FontPlatformData& o) : m_private(0) { operator=(o); }
        FontPlatformData& operator=(const FontPlatformData& o);

        int isValid() const { return reinterpret_cast<unsigned>(m_private) & ~1; }
        HFONT hfont() const;
        const TEXTMETRIC& metrics() const;
        bool isSystemFont() const;
        int size() const;
        unsigned hash() const { return (unsigned)m_private; }
        const FontDescription& fontDescription() const;
        const AtomicString& family() const;
        bool operator==(const FontPlatformData& other) const {     return m_private == other.m_private; }
        HFONT getScaledFontHandle(int height, int width) const;
        const LOGFONT& logFont() const;
        bool isDisabled() const;
        bool discardFontHandle();
        DWORD codePages() const;

        static bool isSongTiSupported();
        static bool mapKnownFont(DWORD codePages, String& family);
        static DWORD getKnownFontCodePages(const wchar_t* family);
        static const String& defaultFontFamily();
        static LONG adjustedGDIFontWeight(LONG gdiFontWeight, const String& family);

        FontOrientation orientation() const { return Horizontal; } // FIXME: Implement.
        void setOrientation(FontOrientation) { } // FIXME: Implement.

#ifndef NDEBUG
        String description() const;
#endif

    private:
        FontPlatformPrivateData* m_private;
    };

}

#endif
