/*
 * Copyright (C) 1999 Antti Koivisto (koivisto@kde.org)
 * Copyright (C) 2004, 2005, 2006, 2007, 2008, 2009, 2010 Apple Inc. All rights reserved.
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
#include "StyleRareInheritedData.h"

#include "CursorList.h"
#include "QuotesData.h"
#include "RenderStyle.h"
#include "RenderStyleConstants.h"
#include "ShadowData.h"

namespace WebCore {

StyleRareInheritedData::StyleRareInheritedData()
    : textStrokeWidth(RenderStyle::initialTextStrokeWidth())
    , indent(RenderStyle::initialTextIndent())
    , m_effectiveZoom(RenderStyle::initialZoom())
    , widows(RenderStyle::initialWidows())
    , orphans(RenderStyle::initialOrphans())
    , textSecurity(RenderStyle::initialTextSecurity())
    , userModify(READ_ONLY)
    , wordBreak(RenderStyle::initialWordBreak())
    , wordWrap(RenderStyle::initialWordWrap())
    , nbspMode(NBNORMAL)
    , khtmlLineBreak(LBNORMAL)
    , textSizeAdjust(RenderStyle::initialTextSizeAdjust())
    , resize(RenderStyle::initialResize())
    , userSelect(RenderStyle::initialUserSelect())
    , colorSpace(ColorSpaceDeviceRGB)
    , speak(SpeakNormal)
    , hyphens(HyphensManual)
    , textEmphasisFill(TextEmphasisFillFilled)
    , textEmphasisMark(TextEmphasisMarkNone)
    , textEmphasisPosition(TextEmphasisPositionOver)
    , m_lineBoxContain(RenderStyle::initialLineBoxContain())
    , hyphenationLimitBefore(-1)
    , hyphenationLimitAfter(-1)
{
}

StyleRareInheritedData::StyleRareInheritedData(const StyleRareInheritedData& o)
    : RefCounted<StyleRareInheritedData>()
    , textStrokeColor(o.textStrokeColor)
    , textStrokeWidth(o.textStrokeWidth)
    , textFillColor(o.textFillColor)
    , textEmphasisColor(o.textEmphasisColor)
    , textShadow(o.textShadow ? adoptPtr(new ShadowData(*o.textShadow)) : nullptr)
    , highlight(o.highlight)
    , cursorData(o.cursorData)
    , indent(o.indent)
    , m_effectiveZoom(o.m_effectiveZoom)
    , widows(o.widows)
    , orphans(o.orphans)
    , textSecurity(o.textSecurity)
    , userModify(o.userModify)
    , wordBreak(o.wordBreak)
    , wordWrap(o.wordWrap)
    , nbspMode(o.nbspMode)
    , khtmlLineBreak(o.khtmlLineBreak)
    , textSizeAdjust(o.textSizeAdjust)
    , resize(o.resize)
    , userSelect(o.userSelect)
    , colorSpace(o.colorSpace)
    , speak(o.speak)
    , hyphens(o.hyphens)
    , textEmphasisFill(o.textEmphasisFill)
    , textEmphasisMark(o.textEmphasisMark)
    , textEmphasisPosition(o.textEmphasisPosition)
    , m_lineBoxContain(o.m_lineBoxContain)
    , hyphenationString(o.hyphenationString)
    , hyphenationLimitBefore(o.hyphenationLimitBefore)
    , hyphenationLimitAfter(o.hyphenationLimitAfter)
    , locale(o.locale)
    , textEmphasisCustomMark(o.textEmphasisCustomMark)
{
}

StyleRareInheritedData::~StyleRareInheritedData()
{
}

static bool cursorDataEquivalent(const CursorList* c1, const CursorList* c2)
{
    if (c1 == c2)
        return true;
    if ((!c1 && c2) || (c1 && !c2))
        return false;
    return (*c1 == *c2);
}

bool StyleRareInheritedData::operator==(const StyleRareInheritedData& o) const
{
    return textStrokeColor == o.textStrokeColor
        && textStrokeWidth == o.textStrokeWidth
        && textFillColor == o.textFillColor
        && textEmphasisColor == o.textEmphasisColor
        && shadowDataEquivalent(o)
        && highlight == o.highlight
        && cursorDataEquivalent(cursorData.get(), o.cursorData.get())
        && indent == o.indent
        && m_effectiveZoom == o.m_effectiveZoom
        && widows == o.widows
        && orphans == o.orphans
        && textSecurity == o.textSecurity
        && userModify == o.userModify
        && wordBreak == o.wordBreak
        && wordWrap == o.wordWrap
        && nbspMode == o.nbspMode
        && khtmlLineBreak == o.khtmlLineBreak
        && textSizeAdjust == o.textSizeAdjust
        && resize == o.resize
        && userSelect == o.userSelect
        && colorSpace == o.colorSpace
        && speak == o.speak
        && hyphens == o.hyphens
        && hyphenationLimitBefore == o.hyphenationLimitBefore
        && hyphenationLimitAfter == o.hyphenationLimitAfter
        && textEmphasisFill == o.textEmphasisFill
        && textEmphasisMark == o.textEmphasisMark
        && textEmphasisPosition == o.textEmphasisPosition
        && m_lineBoxContain == o.m_lineBoxContain
        && hyphenationString == o.hyphenationString
        && locale == o.locale
        && textEmphasisCustomMark == o.textEmphasisCustomMark
        && *quotes == *o.quotes;
}

bool StyleRareInheritedData::shadowDataEquivalent(const StyleRareInheritedData& o) const
{
    if ((!textShadow && o.textShadow) || (textShadow && !o.textShadow))
        return false;
    if (textShadow && o.textShadow && (*textShadow != *o.textShadow))
        return false;
    return true;
}

} // namespace WebCore
