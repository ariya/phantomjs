/*
 * Copyright (C) 2000 Lars Knoll (knoll@kde.org)
 *           (C) 2000 Antti Koivisto (koivisto@kde.org)
 *           (C) 2000 Dirk Mueller (mueller@kde.org)
 * Copyright (C) 2003, 2005, 2006, 2007, 2008 Apple Inc. All rights reserved.
 * Copyright (C) 2006 Graham Dennis (graham.dennis@gmail.com)
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

#ifndef StyleRareInheritedData_h
#define StyleRareInheritedData_h

#include "Color.h"
#include "Length.h"
#include <wtf/RefCounted.h>
#include <wtf/PassRefPtr.h>
#include <wtf/text/AtomicString.h>

#if ENABLE(CSS_VARIABLES)
#include "DataRef.h"
#include "StyleVariableData.h"
#endif

namespace WebCore {

class CursorList;
class QuotesData;
class ShadowData;
class StyleImage;

// This struct is for rarely used inherited CSS3, CSS2, and WebKit-specific properties.
// By grouping them together, we save space, and only allocate this object when someone
// actually uses one of these properties.
class StyleRareInheritedData : public RefCounted<StyleRareInheritedData> {
public:
    static PassRefPtr<StyleRareInheritedData> create() { return adoptRef(new StyleRareInheritedData); }
    PassRefPtr<StyleRareInheritedData> copy() const { return adoptRef(new StyleRareInheritedData(*this)); }
    ~StyleRareInheritedData();

    bool operator==(const StyleRareInheritedData& o) const;
    bool operator!=(const StyleRareInheritedData& o) const
    {
        return !(*this == o);
    }
    bool shadowDataEquivalent(const StyleRareInheritedData&) const;

    RefPtr<StyleImage> listStyleImage;

    Color textStrokeColor;
    float textStrokeWidth;
    Color textFillColor;
    Color textEmphasisColor;
    
    Color visitedLinkTextStrokeColor;
    Color visitedLinkTextFillColor;
    Color visitedLinkTextEmphasisColor;    

    OwnPtr<ShadowData> textShadow; // Our text shadow information for shadowed text drawing.
    AtomicString highlight; // Apple-specific extension for custom highlight rendering.
    
    RefPtr<CursorList> cursorData;
    Length indent;
    float m_effectiveZoom;

    // Paged media properties.
    short widows;
    short orphans;
    unsigned m_hasAutoWidows : 1;
    unsigned m_hasAutoOrphans : 1;
    
    unsigned textSecurity : 2; // ETextSecurity
    unsigned userModify : 2; // EUserModify (editing)
    unsigned wordBreak : 2; // EWordBreak
    unsigned overflowWrap : 1; // EOverflowWrap
    unsigned nbspMode : 1; // ENBSPMode
    unsigned lineBreak : 3; // LineBreak
    unsigned resize : 2; // EResize
    unsigned userSelect : 2; // EUserSelect
    unsigned colorSpace : 1; // ColorSpace
    unsigned speak : 3; // ESpeak
    unsigned hyphens : 2; // Hyphens
    unsigned textEmphasisFill : 1; // TextEmphasisFill
    unsigned textEmphasisMark : 3; // TextEmphasisMark
    unsigned textEmphasisPosition : 1; // TextEmphasisPosition
    unsigned m_textOrientation : 2; // TextOrientation
#if ENABLE(CSS3_TEXT)
    unsigned m_textIndentLine : 1; // TextIndentLine
    unsigned m_textIndentType : 1; // TextIndentType
#endif
    unsigned m_lineBoxContain: 7; // LineBoxContain
    // CSS Image Values Level 3
#if ENABLE(CSS_IMAGE_ORIENTATION)
    unsigned m_imageOrientation : 4; // ImageOrientationEnum
#endif
    unsigned m_imageRendering : 2; // EImageRendering
    unsigned m_lineSnap : 2; // LineSnap
    unsigned m_lineAlign : 1; // LineAlign
#if ENABLE(ACCELERATED_OVERFLOW_SCROLLING)
    unsigned useTouchOverflowScrolling: 1;
#endif
#if ENABLE(CSS_IMAGE_RESOLUTION)
    unsigned m_imageResolutionSource : 1; // ImageResolutionSource
    unsigned m_imageResolutionSnap : 1; // ImageResolutionSnap
#endif
#if ENABLE(CSS3_TEXT)
    unsigned m_textAlignLast : 3; // TextAlignLast
    unsigned m_textJustify : 3; // TextJustify
    unsigned m_textUnderlinePosition : 3; // TextUnderlinePosition
#endif // CSS3_TEXT
    unsigned m_rubyPosition : 1; // RubyPosition

    AtomicString hyphenationString;
    short hyphenationLimitBefore;
    short hyphenationLimitAfter;
    short hyphenationLimitLines;

    AtomicString locale;

    AtomicString textEmphasisCustomMark;
    RefPtr<QuotesData> quotes;

    AtomicString m_lineGrid;
    unsigned m_tabSize;

#if ENABLE(CSS_IMAGE_RESOLUTION)
    float m_imageResolution;
#endif

#if ENABLE(TOUCH_EVENTS)
    Color tapHighlightColor;
#endif

#if ENABLE(CSS_VARIABLES)
    DataRef<StyleVariableData> m_variables;
#endif

private:
    StyleRareInheritedData();
    StyleRareInheritedData(const StyleRareInheritedData&);
};

} // namespace WebCore

#endif // StyleRareInheritedData_h
