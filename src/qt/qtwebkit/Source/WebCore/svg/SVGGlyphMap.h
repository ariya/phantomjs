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
#include "SurrogatePairAwareTextIterator.h"
#include "SVGGlyph.h"

#include <wtf/HashMap.h>
#include <wtf/Vector.h>

namespace WebCore {

struct GlyphMapNode;
class SVGFontData;

typedef HashMap<UChar32, RefPtr<GlyphMapNode> > GlyphMapLayer;

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

    void addGlyph(const String& glyphName, const String& unicodeString, SVGGlyph glyph)
    {
        ASSERT(!glyphName.isEmpty() || !unicodeString.isEmpty());

        bool hasGlyphName = !glyphName.isEmpty();
        if (unicodeString.isEmpty()) {
            // Register named glyph in the named glyph map and in the glyph table.
            ASSERT(hasGlyphName);
            appendToGlyphTable(glyph);
            m_namedGlyphs.add(glyphName, glyph.tableEntry);
            return;
        }
    
        GlyphMapLayer* currentLayer = &m_rootLayer;
        RefPtr<GlyphMapNode> node;
        size_t length = unicodeString.length();

        UChar32 character = 0;
        unsigned clusterLength = 0;
        SurrogatePairAwareTextIterator textIterator(unicodeString.characters(), 0, length, length);
        while (textIterator.consume(character, clusterLength)) {
            node = currentLayer->get(character);
            if (!node) {
                node = GlyphMapNode::create();
                currentLayer->set(character, node);
            }
            currentLayer = &node->children;
            textIterator.advance(clusterLength);
        }

        if (!node)
            return;

        // Register glyph associated with an unicode string into the glyph map.
        node->glyphs.append(glyph);
        SVGGlyph& lastGlyph = node->glyphs.last();
        lastGlyph.priority = m_currentPriority++;
        lastGlyph.unicodeStringLength = length;
    
        // If the glyph is named, also add it to the named glyph name, and to the glyph table in both cases.
        appendToGlyphTable(lastGlyph);
        if (hasGlyphName)  
            m_namedGlyphs.add(glyphName, lastGlyph.tableEntry);
    }

    void appendToGlyphTable(SVGGlyph& glyph)
    {
        size_t tableEntry = m_glyphTable.size();
        ASSERT(tableEntry < std::numeric_limits<unsigned short>::max());

        // The first table entry starts with 1. 0 denotes an unknown glyph.
        glyph.tableEntry = tableEntry + 1;
        m_glyphTable.append(glyph);
    }

    static inline bool compareGlyphPriority(const SVGGlyph& first, const SVGGlyph& second)
    {
        return first.priority < second.priority;
    }

    void collectGlyphsForString(const String& string, Vector<SVGGlyph>& glyphs)
    {
        GlyphMapLayer* currentLayer = &m_rootLayer;

        const UChar* characters = string.characters();
        size_t length = string.length();

        UChar32 character = 0;
        unsigned clusterLength = 0;
        SurrogatePairAwareTextIterator textIterator(characters, 0, length, length);
        while (textIterator.consume(character, clusterLength)) {
            RefPtr<GlyphMapNode> node = currentLayer->get(character);
            if (!node)
                break;
            glyphs.appendVector(node->glyphs);
            currentLayer = &node->children;
            textIterator.advance(clusterLength);
        }

        std::sort(glyphs.begin(), glyphs.end(), compareGlyphPriority);
    }
    
    void clear() 
    { 
        m_rootLayer.clear();
        m_glyphTable.clear();
        m_currentPriority = 0;
    }

    const SVGGlyph& svgGlyphForGlyph(Glyph glyph) const
    {
        if (!glyph || glyph > m_glyphTable.size()) {
            DEFINE_STATIC_LOCAL(SVGGlyph, defaultGlyph, ());
            return defaultGlyph;
        }
        return m_glyphTable[glyph - 1];
    }

    const SVGGlyph& glyphIdentifierForGlyphName(const String& glyphName) const
    {
        return svgGlyphForGlyph(m_namedGlyphs.get(glyphName));
    }

private:
    GlyphMapLayer m_rootLayer;
    Vector<SVGGlyph, 256> m_glyphTable;
    HashMap<String, Glyph> m_namedGlyphs;
    int m_currentPriority;
};

}

#endif // ENABLE(SVG_FONTS)
#endif // SVGGlyphMap_h
