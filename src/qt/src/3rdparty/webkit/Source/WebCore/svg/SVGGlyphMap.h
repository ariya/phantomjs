/*
 * Copyright (C) 2008 Apple Inc. All rights reserved.
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
 */

#ifndef SVGGlyphMap_h
#define SVGGlyphMap_h

#if ENABLE(SVG_FONTS)
#include "SVGGlyphElement.h"

namespace WebCore {

struct GlyphMapNode;

typedef HashMap<UChar, RefPtr<GlyphMapNode> > GlyphMapLayer;

struct GlyphMapNode : public RefCounted<GlyphMapNode> {
private:
    GlyphMapNode() { }
public:
    static PassRefPtr<GlyphMapNode> create() { return adoptRef(new GlyphMapNode); }

    Vector<SVGGlyph> glyphs;

    GlyphMapLayer children;
};

class SVGGlyphMap {

public:
    SVGGlyphMap() : m_currentPriority(0) { }

    void add(const String& string, const SVGGlyph& glyph) 
    {
        size_t len = string.length();
        GlyphMapLayer* currentLayer = &m_rootLayer;

        RefPtr<GlyphMapNode> node;
        for (size_t i = 0; i < len; ++i) {
            UChar curChar = string[i];
            node = currentLayer->get(curChar);
            if (!node) {
                node = GlyphMapNode::create();
                currentLayer->set(curChar, node);
            }
            currentLayer = &node->children;
        }

        if (node) {
            node->glyphs.append(glyph);
            node->glyphs.last().priority = m_currentPriority++;
            node->glyphs.last().unicodeStringLength = len;
            node->glyphs.last().isValid = true;
        }
    }

    static inline bool compareGlyphPriority(const SVGGlyph& first, const SVGGlyph& second)
    {
        return first.priority < second.priority;
    }

    void get(const String& string, Vector<SVGGlyph>& glyphs)
    {
        GlyphMapLayer* currentLayer = &m_rootLayer;

        for (size_t i = 0; i < string.length(); ++i) {
            UChar curChar = string[i];
            RefPtr<GlyphMapNode> node = currentLayer->get(curChar);
            if (!node)
                break;
            glyphs.append(node->glyphs);
            currentLayer = &node->children;
        }
        std::sort(glyphs.begin(), glyphs.end(), compareGlyphPriority);
    }

    void clear() 
    { 
        m_rootLayer.clear(); 
        m_currentPriority = 0;
    }

private:
    GlyphMapLayer m_rootLayer;
    int m_currentPriority;
};

}

#endif // ENABLE(SVG_FONTS)


#endif // SVGGlyphMap_h
