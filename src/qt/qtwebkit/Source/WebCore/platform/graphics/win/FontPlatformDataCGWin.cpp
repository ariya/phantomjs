/*
 * This file is part of the internal font implementation. It should not be included by anyone other than
 * FontMac.cpp, FontWin.cpp and Font.cpp.
 *
 * Copyright (C) 2006, 2007, 2008, 2009 Apple Inc.
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

#include <ApplicationServices/ApplicationServices.h>
#include <WebKitSystemInterface/WebKitSystemInterface.h>
#include <wtf/HashMap.h>
#include <wtf/RetainPtr.h>
#include <wtf/Vector.h>
#include <wtf/text/StringHash.h>
#include <wtf/text/WTFString.h>

using std::min;

namespace WebCore {

static inline USHORT readBigEndianWord(const BYTE* word) { return (word[0] << 8) | word[1]; }

static CFStringRef getPostScriptName(CFStringRef faceName, HDC dc)
{
    const DWORD cMaxNameTableSize = 1024 * 1024;

    static HashMap<String, RetainPtr<CFStringRef> > nameMap;

    // Check our hash first.
    String faceString(faceName);
    RetainPtr<CFStringRef> result = nameMap.get(faceString);
    if (result)
        return result.get();

    // We need to obtain the PostScript name from the name table and use it instead,.
    DWORD bufferSize = GetFontData(dc, 'eman', 0, NULL, 0); // "name" backwards
    if (bufferSize == 0 || bufferSize == GDI_ERROR || bufferSize > cMaxNameTableSize)
        return NULL;
   
    Vector<BYTE> bufferVector(bufferSize);
    BYTE* buffer = bufferVector.data();
    if (GetFontData(dc, 'eman', 0, buffer, bufferSize) == GDI_ERROR)
        return NULL;

    if (bufferSize < 6)
        return NULL;

    USHORT numberOfRecords = readBigEndianWord(buffer + 2);
    UINT stringsOffset = readBigEndianWord(buffer + 4);
    if (bufferSize < stringsOffset)
        return NULL;

    BYTE* strings = buffer + stringsOffset;

    // Now walk each name record looking for a Mac or Windows PostScript name.
    UINT offset = 6;
    for (int i = 0; i < numberOfRecords; i++) {
        if (bufferSize < offset + 12)
            return NULL;

        USHORT platformID = readBigEndianWord(buffer + offset);
        USHORT encodingID = readBigEndianWord(buffer + offset + 2);
        USHORT languageID = readBigEndianWord(buffer + offset + 4);
        USHORT nameID = readBigEndianWord(buffer + offset + 6);
        USHORT length = readBigEndianWord(buffer + offset + 8);
        USHORT nameOffset = readBigEndianWord(buffer + offset + 10);

        if (platformID == 3 && encodingID == 1 && languageID == 0x409 && nameID == 6) {
            // This is a Windows PostScript name and is therefore UTF-16.
            // Pass Big Endian as the encoding.
            if (bufferSize < stringsOffset + nameOffset + length)
                return NULL;
            result = adoptCF(CFStringCreateWithBytes(NULL, strings + nameOffset, length, kCFStringEncodingUTF16BE, false));
            break;
        } else if (platformID == 1 && encodingID == 0 && languageID == 0 && nameID == 6) {
            // This is a Mac PostScript name and is therefore ASCII.
            // See http://developer.apple.com/textfonts/TTRefMan/RM06/Chap6name.html
            if (bufferSize < stringsOffset + nameOffset + length)
                return NULL;
            result = adoptCF(CFStringCreateWithBytes(NULL, strings + nameOffset, length, kCFStringEncodingASCII, false));
            break;
        }

        offset += 12;
    }

    if (result)
        nameMap.set(faceString, result);
    return result.get();
}

void FontPlatformData::platformDataInit(HFONT font, float size, HDC hdc, WCHAR* faceName)
{
    LOGFONT logfont;
    GetObject(font, sizeof(logfont), &logfont);
    m_cgFont = adoptCF(CGFontCreateWithPlatformFont(&logfont));
}

FontPlatformData::FontPlatformData(HFONT hfont, CGFontRef font, float size, bool bold, bool oblique, bool useGDI)
    : m_syntheticBold(bold)
    , m_syntheticOblique(oblique)
    , m_orientation(Horizontal)
    , m_size(size)
    , m_widthVariant(RegularWidth)
    , m_font(RefCountedGDIHandle<HFONT>::create(hfont))
    , m_cgFont(font)
    , m_isColorBitmapFont(false)
    , m_useGDI(useGDI)
{
}

FontPlatformData::~FontPlatformData()
{
}

void FontPlatformData::platformDataInit(const FontPlatformData& source)
{
    m_font = source.m_font;
    m_cgFont = source.m_cgFont;
    m_useGDI = source.m_useGDI;
}

const FontPlatformData& FontPlatformData::platformDataAssign(const FontPlatformData& other)
{
    m_font = other.m_font;
    m_cgFont = other.m_cgFont;
    m_useGDI = other.m_useGDI;

    return *this;
}

bool FontPlatformData::platformIsEqual(const FontPlatformData& other) const
{
    return m_font == other.m_font
        && m_cgFont == other.m_cgFont
        && m_useGDI == other.m_useGDI;
}

}
