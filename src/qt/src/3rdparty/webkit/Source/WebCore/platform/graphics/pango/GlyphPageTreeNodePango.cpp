/*
 * Copyright (C) 2006, 2007 Apple Inc. All rights reserved.
 * Copyright (C) 2006 Michael Emmel mike.emmel@gmail.com
 * Copyright (C) 2007 Alp Toker <alp.toker@collabora.co.uk>
 * Copyright (C) 2007 Pioneer Research Center USA, Inc.
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
#include <pango/pango-font.h>

namespace WebCore {

static PangoGlyph pango_font_get_glyph(PangoFont* font, PangoContext* context, gunichar wc)
{
    PangoGlyph result = 0;
    gchar buffer[7];

    gint  length = g_unichar_to_utf8(wc, buffer);
    g_return_val_if_fail(length, 0);

    GList* items = pango_itemize(context, buffer, 0, length, NULL, NULL);

    if (g_list_length(items) == 1) {
        PangoItem* item = reinterpret_cast<PangoItem*>(items->data);
        PangoFont* tmpFont = item->analysis.font;
        item->analysis.font = font;

        PangoGlyphString* glyphs = pango_glyph_string_new();
        pango_shape(buffer, length, &item->analysis, glyphs);

        item->analysis.font = tmpFont;

        if (glyphs->num_glyphs == 1)
            result = glyphs->glyphs[0].glyph;
        else
            g_warning("didn't get 1 glyph but %d", glyphs->num_glyphs);

        pango_glyph_string_free(glyphs);
    }

    g_list_foreach(items, (GFunc)pango_item_free, NULL);
    g_list_free(items);

    return result;
}

bool GlyphPage::fill(unsigned offset, unsigned length, UChar* buffer, unsigned bufferLength, const SimpleFontData* fontData)
{
    // The bufferLength will be greater than the glyph page size if the buffer has Unicode supplementary characters.
    // We won't support this for now.
    if (bufferLength > GlyphPage::size)
        return false;

    if (!fontData->platformData().m_font || fontData->platformData().m_font == reinterpret_cast<PangoFont*>(-1))
        return false;

    bool haveGlyphs = false;
    for (unsigned i = 0; i < length; i++) {
        Glyph glyph = pango_font_get_glyph(fontData->platformData().m_font, fontData->platformData().m_context, buffer[i]);
        if (!glyph)
            setGlyphDataForIndex(offset + i, 0, 0);
        else {
            setGlyphDataForIndex(offset + i, glyph, fontData);
            haveGlyphs = true;
        }
    }

    return haveGlyphs;
}

}
