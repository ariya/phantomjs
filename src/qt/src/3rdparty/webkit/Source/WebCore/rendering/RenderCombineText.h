/*
 * Copyright (C) 2011 Apple Inc. All rights reserved.
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

#ifndef RenderCombineText_h
#define RenderCombineText_h

#include "RenderText.h"

namespace WebCore {

class RenderCombineText : public RenderText {
public:
    RenderCombineText(Node*, PassRefPtr<StringImpl>);

    void combineText();
    void adjustTextOrigin(FloatPoint& textOrigin, const FloatRect& boxRect) const;
    void charactersToRender(int start, const UChar*& characters, int& length) const;
    bool isCombined() const { return m_isCombined; }
    float combinedTextWidth(const Font& font) const { return font.size(); }
    const Font& originalFont() const { return parent()->style()->font(); }

private:
    virtual bool isCombineText() const { return true; }
    virtual float width(unsigned from, unsigned length, const Font&, float xPosition, HashSet<const SimpleFontData*>* fallbackFonts = 0, GlyphOverflow* = 0) const;
    virtual const char* renderName() const { return "RenderCombineText"; }
    virtual void styleDidChange(StyleDifference, const RenderStyle* oldStyle);
    virtual void setTextInternal(PassRefPtr<StringImpl>);

    float m_combinedTextWidth;
    bool m_isCombined : 1;
    bool m_needsFontUpdate : 1;
};

inline RenderCombineText* toRenderCombineText(RenderObject* object)
{ 
    ASSERT(!object || object->isCombineText());
    return static_cast<RenderCombineText*>(object);
}

inline const RenderCombineText* toRenderCombineText(const RenderObject* object)
{ 
    ASSERT(!object || object->isCombineText());
    return static_cast<const RenderCombineText*>(object);
}

// This will catch anyone doing an unnecessary cast.
void toRenderCombineText(const RenderCombineText*);

} // namespace WebCore

#endif // RenderCombineText_h
