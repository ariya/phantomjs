/*
 * Copyright (C) Research In Motion Limited 2011. All rights reserved.
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

#ifndef SVGTextRunRenderingContext_h
#define SVGTextRunRenderingContext_h

#include "Font.h"
#include "TextRun.h"

namespace WebCore {

class RenderObject;
class RenderSVGResource;

class SVGTextRunRenderingContext : public TextRun::RenderingContext {
public:
    static PassRefPtr<SVGTextRunRenderingContext> create(RenderObject* renderer)
    {
        return adoptRef(new SVGTextRunRenderingContext(renderer));
    }

    RenderObject* renderer() const { return m_renderer; }

#if ENABLE(SVG_FONTS)
    RenderSVGResource* activePaintingResource() const { return m_activePaintingResource; }
    void setActivePaintingResource(RenderSVGResource* object) { m_activePaintingResource = object; }

    virtual GlyphData glyphDataForCharacter(const Font&, const TextRun&, WidthIterator&, UChar32 character, bool mirror, int currentCharacter, unsigned& advanceLength);
    virtual void drawSVGGlyphs(GraphicsContext*, const TextRun&, const SimpleFontData*, const GlyphBuffer&, int from, int to, const FloatPoint&) const;
    virtual float floatWidthUsingSVGFont(const Font&, const TextRun&, int& charsConsumed, String& glyphName) const;
    virtual bool applySVGKerning(const SimpleFontData*, WidthIterator&, GlyphBuffer*, int from) const;
#endif

private:
    SVGTextRunRenderingContext(RenderObject* renderer)
        : m_renderer(renderer)
#if ENABLE(SVG_FONTS)
        , m_activePaintingResource(0)
#endif
    {
    }

    virtual ~SVGTextRunRenderingContext() { }

    RenderObject* m_renderer;

#if ENABLE(SVG_FONTS)
    RenderSVGResource* m_activePaintingResource;
#endif
};

inline bool textRunNeedsRenderingContext(const Font& font)
{
    // Only save the extra data if SVG Fonts are used, which depend on them.
    // FIXME: SVG Fonts won't work as segmented fonts at the moment, if that's fixed, we need to check for them as well below.
    ASSERT(font.primaryFont());
    return font.primaryFont()->isSVGFont();
}

} // namespace WebCore

#endif // SVGTextRunRenderingContext_h
