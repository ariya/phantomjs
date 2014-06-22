/*
 * Copyright (C) 2006, 2007 Apple Inc. All rights reserved.
 * Copyright (C) 2006 Michael Emmel mike.emmel@gmail.com
 * Copyright (C) 2007 Alp Toker <alp.toker@collabora.co.uk>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "GlyphPageTreeNode.h"

#include "SimpleFontData.h"
#include "UTF16UChar32Iterator.h"
#include <cairo-ft.h>
#include <cairo.h>
#include <fontconfig/fcfreetype.h>

namespace WebCore {

bool GlyphPage::fill(unsigned offset, unsigned length, UChar* buffer, unsigned bufferLength, const SimpleFontData* fontData)
{
    cairo_scaled_font_t* scaledFont = fontData->platformData().scaledFont();
    ASSERT(scaledFont);

    FT_Face face = cairo_ft_scaled_font_lock_face(scaledFont);
    if (!face)
        return false;

    bool haveGlyphs = false;
    UTF16UChar32Iterator iterator(buffer, bufferLength);
    for (unsigned i = 0; i < length; i++) {
        UChar32 character = iterator.next();
        if (character == iterator.end())
            break;

        Glyph glyph = FcFreeTypeCharIndex(face, character);
        if (!glyph)
            setGlyphDataForIndex(offset + i, 0, 0);
        else {
            setGlyphDataForIndex(offset + i, glyph, fontData);
            haveGlyphs = true;
        }
    }

    cairo_ft_scaled_font_unlock_face(scaledFont);
    return haveGlyphs;
}

}
