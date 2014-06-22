/*
 *  Copyright (C) 2007-2009 Torch Mobile Inc.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301, USA.
 *
 */

#include "config.h"
#include "GlyphPageTreeNode.h"

#include "Font.h"
#include "FontCache.h"
#include "FontData.h"
#include "SimpleFontData.h"

namespace WebCore {

DWORD getKnownFontCodePages(const wchar_t* family);

typedef unsigned (*funcGetCharCodePages)(unsigned short c, unsigned& lastPos);
funcGetCharCodePages getCharCodePages = 0;

bool GlyphPage::fill(unsigned offset, unsigned length, UChar* buffer, unsigned bufferLength, const SimpleFontData* fontData)
{
    if (length != bufferLength)
        return false;

    if (fontData->platformData().hfont()) {
        DWORD fontCodePages = fontData->platformData().codePages();
        if (fontCodePages) {
            if (getCharCodePages) {
                unsigned lastPos = 0;
                for (unsigned i = 0; i < bufferLength; ++i) {
                    DWORD actualCodePages = getCharCodePages(buffer[i], lastPos);
                    if (!actualCodePages || (actualCodePages & fontCodePages))
                        setGlyphDataForIndex(offset + i, buffer[i], fontData);
                    else
                        setGlyphDataForIndex(offset + i, 0, 0);
                }
                return true;
            } else if (IMLangFontLinkType* langFontLink = fontCache()->getFontLinkInterface()) {
                for (unsigned i = 0; i < bufferLength; ++i) {
                    DWORD actualCodePages;
                    langFontLink->GetCharCodePages(buffer[i], &actualCodePages);
                    if (!actualCodePages || (actualCodePages & fontCodePages))
                        setGlyphDataForIndex(offset + i, buffer[i], fontData);
                    else
                        setGlyphDataForIndex(offset + i, 0, 0);
                }
                return true;
            }
        }
    }

    for (unsigned i = 0; i < length; ++i)
        setGlyphDataForIndex(offset + i, buffer[i], fontData);

    return true;
}

} // namespace WebCore
