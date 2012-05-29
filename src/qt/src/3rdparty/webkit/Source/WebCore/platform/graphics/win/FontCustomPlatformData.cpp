/*
 * Copyright (C) 2007, 2008, 2009, 2010 Apple Inc. All rights reserved.
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

#include "Base64.h"
#include "FontPlatformData.h"
#include "OpenTypeUtilities.h"
#include "SharedBuffer.h"
#include "SoftLinking.h"
#include "WOFFFileFormat.h"
#include <ApplicationServices/ApplicationServices.h>
#include <WebKitSystemInterface/WebKitSystemInterface.h>
#include <wtf/RetainPtr.h>

// From t2embapi.h, which is missing from the Microsoft Platform SDK.
typedef unsigned long(WINAPIV *READEMBEDPROC) (void*, void*, unsigned long);
struct TTLOADINFO;
#define TTLOAD_PRIVATE 0x00000001
#define LICENSE_PREVIEWPRINT 0x0004
#define E_NONE 0x0000L

namespace WebCore {

using namespace std;

SOFT_LINK_LIBRARY(T2embed);
SOFT_LINK(T2embed, TTLoadEmbeddedFont, LONG, __stdcall, (HANDLE* phFontReference, ULONG ulFlags, ULONG* pulPrivStatus, ULONG ulPrivs, ULONG* pulStatus, READEMBEDPROC lpfnReadFromStream, LPVOID lpvReadStream, LPWSTR szWinFamilyName, LPSTR szMacFamilyName, TTLOADINFO* pTTLoadInfo), (phFontReference, ulFlags,pulPrivStatus, ulPrivs, pulStatus, lpfnReadFromStream, lpvReadStream, szWinFamilyName, szMacFamilyName, pTTLoadInfo));
SOFT_LINK(T2embed, TTGetNewFontName, LONG, __stdcall, (HANDLE* phFontReference, LPWSTR szWinFamilyName, long cchMaxWinName, LPSTR szMacFamilyName, long cchMaxMacName), (phFontReference, szWinFamilyName, cchMaxWinName, szMacFamilyName, cchMaxMacName));
SOFT_LINK(T2embed, TTDeleteEmbeddedFont, LONG, __stdcall, (HANDLE hFontReference, ULONG ulFlags, ULONG* pulStatus), (hFontReference, ulFlags, pulStatus));

FontCustomPlatformData::~FontCustomPlatformData()
{
    if (m_fontReference) {
        if (m_name.isNull()) {
            ASSERT(T2embedLibrary());
            ULONG status;
            TTDeleteEmbeddedFont(m_fontReference, 0, &status);
        } else
            RemoveFontMemResourceEx(m_fontReference);
    }
}

FontPlatformData FontCustomPlatformData::fontPlatformData(int size, bool bold, bool italic, FontOrientation, TextOrientation, FontWidthVariant, FontRenderingMode renderingMode)
{
    ASSERT(m_fontReference);
    ASSERT(T2embedLibrary());

    LOGFONT& logFont = *static_cast<LOGFONT*>(malloc(sizeof(LOGFONT)));
    if (m_name.isNull())
        TTGetNewFontName(&m_fontReference, logFont.lfFaceName, LF_FACESIZE, 0, 0);
    else
        memcpy(logFont.lfFaceName, m_name.charactersWithNullTermination(), sizeof(logFont.lfFaceName[0]) * min(static_cast<size_t>(LF_FACESIZE), 1 + m_name.length()));

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

    RetainPtr<CGFontRef> cgFont(AdoptCF, CGFontCreateWithPlatformFont(&logFont));
    return FontPlatformData(hfont, cgFont.get(), size, bold, italic, renderingMode == AlternateRenderingMode);
}

// Streams the concatenation of a header and font data.
class EOTStream {
public:
    EOTStream(const EOTHeader& eotHeader, const SharedBuffer* fontData, size_t overlayDst, size_t overlaySrc, size_t overlayLength)
        : m_eotHeader(eotHeader)
        , m_fontData(fontData)
        , m_overlayDst(overlayDst)
        , m_overlaySrc(overlaySrc)
        , m_overlayLength(overlayLength)
        , m_offset(0)
        , m_inHeader(true)
    {
    }

    size_t read(void* buffer, size_t count);

private:
    const EOTHeader& m_eotHeader;
    const SharedBuffer* m_fontData;
    size_t m_overlayDst;
    size_t m_overlaySrc;
    size_t m_overlayLength;
    size_t m_offset;
    bool m_inHeader;
};

size_t EOTStream::read(void* buffer, size_t count)
{
    size_t bytesToRead = count;
    if (m_inHeader) {
        size_t bytesFromHeader = min(m_eotHeader.size() - m_offset, count);
        memcpy(buffer, m_eotHeader.data() + m_offset, bytesFromHeader);
        m_offset += bytesFromHeader;
        bytesToRead -= bytesFromHeader;
        if (m_offset == m_eotHeader.size()) {
            m_inHeader = false;
            m_offset = 0;
        }
    }
    if (bytesToRead && !m_inHeader) {
        size_t bytesFromData = min(m_fontData->size() - m_offset, bytesToRead);
        memcpy(buffer, m_fontData->data() + m_offset, bytesFromData);
        if (m_offset < m_overlayDst + m_overlayLength && m_offset + bytesFromData >= m_overlayDst) {
            size_t dstOffset = max<int>(m_overlayDst - m_offset, 0);
            size_t srcOffset = max<int>(0, m_offset - m_overlayDst);
            size_t bytesToCopy = min(bytesFromData - dstOffset, m_overlayLength - srcOffset);
            memcpy(reinterpret_cast<char*>(buffer) + dstOffset, m_fontData->data() + m_overlaySrc + srcOffset, bytesToCopy);
        }
        m_offset += bytesFromData;
        bytesToRead -= bytesFromData;
    }
    return count - bytesToRead;
}

static unsigned long WINAPIV readEmbedProc(void* stream, void* buffer, unsigned long length)
{
    return static_cast<EOTStream*>(stream)->read(buffer, length);
}

// Creates a unique and unpredictable font name, in order to avoid collisions and to
// not allow access from CSS.
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
    ASSERT(T2embedLibrary());

    RefPtr<SharedBuffer> sfntBuffer;
    if (isWOFF(buffer)) {
        Vector<char> sfnt;
        if (!convertWOFFToSfnt(buffer, sfnt))
            return 0;

        sfntBuffer = SharedBuffer::adoptVector(sfnt);
        buffer = sfntBuffer.get();
    }

    // Introduce the font to GDI. AddFontMemResourceEx cannot be used, because it will pollute the process's
    // font namespace (Windows has no API for creating an HFONT from data without exposing the font to the
    // entire process first). TTLoadEmbeddedFont lets us override the font family name, so using a unique name
    // we avoid namespace collisions.

    String fontName = createUniqueFontName();

    // TTLoadEmbeddedFont works only with Embedded OpenType (.eot) data, so we need to create an EOT header
    // and prepend it to the font data.
    EOTHeader eotHeader;
    size_t overlayDst;
    size_t overlaySrc;
    size_t overlayLength;
    if (!getEOTHeader(buffer, eotHeader, overlayDst, overlaySrc, overlayLength))
        return 0;

    HANDLE fontReference;
    ULONG privStatus;
    ULONG status;
    EOTStream eotStream(eotHeader, buffer, overlayDst, overlaySrc, overlayLength);

    LONG loadEmbeddedFontResult = TTLoadEmbeddedFont(&fontReference, TTLOAD_PRIVATE, &privStatus, LICENSE_PREVIEWPRINT, &status, readEmbedProc, &eotStream, const_cast<LPWSTR>(fontName.charactersWithNullTermination()), 0, 0);
    if (loadEmbeddedFontResult == E_NONE)
        fontName = String();
    else {
        fontReference = renameAndActivateFont(buffer, fontName);
        if (!fontReference)
            return 0;
    }

    return new FontCustomPlatformData(fontReference, fontName);
}

bool FontCustomPlatformData::supportsFormat(const String& format)
{
    return equalIgnoringCase(format, "truetype") || equalIgnoringCase(format, "opentype") || equalIgnoringCase(format, "woff");
}

}
