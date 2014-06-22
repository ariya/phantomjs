/*
 * Copyright (C) 1999 Antti Koivisto (koivisto@kde.org)
 * Copyright (C) 2004, 2005, 2006, 2007, 2008, 2009, 2010 Apple Inc. All rights reserved.
 * Copyright (C) 2011 Adobe Systems Incorporated. All rights reserved.
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
#include "RenderStyle.h"

#include "ContentData.h"
#include "CursorList.h"
#include "CSSPropertyNames.h"
#include "Font.h"
#include "FontSelector.h"
#include "Pagination.h"
#include "QuotesData.h"
#include "RenderArena.h"
#include "RenderObject.h"
#include "ScaleTransformOperation.h"
#include "ShadowData.h"
#include "StyleImage.h"
#include "StyleInheritedData.h"
#include "StyleResolver.h"
#if ENABLE(TOUCH_EVENTS)
#include "RenderTheme.h"
#endif
#include <wtf/MathExtras.h>
#include <wtf/StdLibExtras.h>
#include <algorithm>

#if ENABLE(TEXT_AUTOSIZING)
#include "TextAutosizer.h"
#endif

using namespace std;

namespace WebCore {

struct SameSizeAsBorderValue {
    RGBA32 m_color;
    unsigned m_width;
};

COMPILE_ASSERT(sizeof(BorderValue) == sizeof(SameSizeAsBorderValue), BorderValue_should_not_grow);

struct SameSizeAsRenderStyle : public RefCounted<SameSizeAsRenderStyle> {
    void* dataRefs[7];
    void* ownPtrs[1];
#if ENABLE(SVG)
    void* dataRefSvgStyle;
#endif
    struct InheritedFlags {
        unsigned m_bitfields[2];
    } inherited_flags;

    struct NonInheritedFlags {
        unsigned m_bitfields[2];
    } noninherited_flags;
};

COMPILE_ASSERT(sizeof(RenderStyle) == sizeof(SameSizeAsRenderStyle), RenderStyle_should_stay_small);

inline RenderStyle* defaultStyle()
{
    static RenderStyle* s_defaultStyle = RenderStyle::createDefaultStyle().leakRef();
    return s_defaultStyle;
}

PassRefPtr<RenderStyle> RenderStyle::create()
{
    return adoptRef(new RenderStyle());
}

PassRefPtr<RenderStyle> RenderStyle::createDefaultStyle()
{
    return adoptRef(new RenderStyle(true));
}

PassRefPtr<RenderStyle> RenderStyle::createAnonymousStyleWithDisplay(const RenderStyle* parentStyle, EDisplay display)
{
    RefPtr<RenderStyle> newStyle = RenderStyle::create();
    newStyle->inheritFrom(parentStyle);
    newStyle->inheritUnicodeBidiFrom(parentStyle);
    newStyle->setDisplay(display);
    return newStyle;
}

PassRefPtr<RenderStyle> RenderStyle::clone(const RenderStyle* other)
{
    return adoptRef(new RenderStyle(*other));
}

ALWAYS_INLINE RenderStyle::RenderStyle()
    : m_box(defaultStyle()->m_box)
    , visual(defaultStyle()->visual)
    , m_background(defaultStyle()->m_background)
    , surround(defaultStyle()->surround)
    , rareNonInheritedData(defaultStyle()->rareNonInheritedData)
    , rareInheritedData(defaultStyle()->rareInheritedData)
    , inherited(defaultStyle()->inherited)
#if ENABLE(SVG)
    , m_svgStyle(defaultStyle()->m_svgStyle)
#endif
{
    setBitDefaults(); // Would it be faster to copy this from the default style?
    COMPILE_ASSERT((sizeof(InheritedFlags) <= 8), InheritedFlags_does_not_grow);
    COMPILE_ASSERT((sizeof(NonInheritedFlags) <= 8), NonInheritedFlags_does_not_grow);
}

ALWAYS_INLINE RenderStyle::RenderStyle(bool)
{
    setBitDefaults();

    m_box.init();
    visual.init();
    m_background.init();
    surround.init();
    rareNonInheritedData.init();
    rareNonInheritedData.access()->m_deprecatedFlexibleBox.init();
    rareNonInheritedData.access()->m_flexibleBox.init();
    rareNonInheritedData.access()->m_marquee.init();
    rareNonInheritedData.access()->m_multiCol.init();
    rareNonInheritedData.access()->m_transform.init();
#if ENABLE(CSS_FILTERS)
    rareNonInheritedData.access()->m_filter.init();
#endif
    rareNonInheritedData.access()->m_grid.init();
    rareNonInheritedData.access()->m_gridItem.init();
    rareInheritedData.init();
    inherited.init();
#if ENABLE(SVG)
    m_svgStyle.init();
#endif
}

ALWAYS_INLINE RenderStyle::RenderStyle(const RenderStyle& o)
    : RefCounted<RenderStyle>()
    , m_box(o.m_box)
    , visual(o.visual)
    , m_background(o.m_background)
    , surround(o.surround)
    , rareNonInheritedData(o.rareNonInheritedData)
    , rareInheritedData(o.rareInheritedData)
    , inherited(o.inherited)
#if ENABLE(SVG)
    , m_svgStyle(o.m_svgStyle)
#endif
    , inherited_flags(o.inherited_flags)
    , noninherited_flags(o.noninherited_flags)
{
}

void RenderStyle::inheritFrom(const RenderStyle* inheritParent, IsAtShadowBoundary isAtShadowBoundary)
{
    if (isAtShadowBoundary == AtShadowBoundary) {
        // Even if surrounding content is user-editable, shadow DOM should act as a single unit, and not necessarily be editable
        EUserModify currentUserModify = userModify();
        rareInheritedData = inheritParent->rareInheritedData;
        setUserModify(currentUserModify);
    } else
        rareInheritedData = inheritParent->rareInheritedData;
    inherited = inheritParent->inherited;
    inherited_flags = inheritParent->inherited_flags;
#if ENABLE(SVG)
    if (m_svgStyle != inheritParent->m_svgStyle)
        m_svgStyle.access()->inheritFrom(inheritParent->m_svgStyle.get());
#endif
}

void RenderStyle::copyNonInheritedFrom(const RenderStyle* other)
{
    m_box = other->m_box;
    visual = other->visual;
    m_background = other->m_background;
    surround = other->surround;
    rareNonInheritedData = other->rareNonInheritedData;
    // The flags are copied one-by-one because noninherited_flags contains a bunch of stuff other than real style data.
    noninherited_flags._effectiveDisplay = other->noninherited_flags._effectiveDisplay;
    noninherited_flags._originalDisplay = other->noninherited_flags._originalDisplay;
    noninherited_flags._overflowX = other->noninherited_flags._overflowX;
    noninherited_flags._overflowY = other->noninherited_flags._overflowY;
    noninherited_flags._vertical_align = other->noninherited_flags._vertical_align;
    noninherited_flags._clear = other->noninherited_flags._clear;
    noninherited_flags._position = other->noninherited_flags._position;
    noninherited_flags._floating = other->noninherited_flags._floating;
    noninherited_flags._table_layout = other->noninherited_flags._table_layout;
    noninherited_flags._unicodeBidi = other->noninherited_flags._unicodeBidi;
    noninherited_flags._page_break_before = other->noninherited_flags._page_break_before;
    noninherited_flags._page_break_after = other->noninherited_flags._page_break_after;
    noninherited_flags._page_break_inside = other->noninherited_flags._page_break_inside;
    noninherited_flags.explicitInheritance = other->noninherited_flags.explicitInheritance;
#if ENABLE(SVG)
    if (m_svgStyle != other->m_svgStyle)
        m_svgStyle.access()->copyNonInheritedFrom(other->m_svgStyle.get());
#endif
    ASSERT(zoom() == initialZoom());
}

bool RenderStyle::operator==(const RenderStyle& o) const
{
    // compare everything except the pseudoStyle pointer
    return inherited_flags == o.inherited_flags
        && noninherited_flags == o.noninherited_flags
        && m_box == o.m_box
        && visual == o.visual
        && m_background == o.m_background
        && surround == o.surround
        && rareNonInheritedData == o.rareNonInheritedData
        && rareInheritedData == o.rareInheritedData
        && inherited == o.inherited
#if ENABLE(SVG)
        && m_svgStyle == o.m_svgStyle
#endif
            ;
}

bool RenderStyle::isStyleAvailable() const
{
    return this != StyleResolver::styleNotYetAvailable();
}

static inline int pseudoBit(PseudoId pseudo)
{
    return 1 << (pseudo - 1);
}

bool RenderStyle::hasAnyPublicPseudoStyles() const
{
    return PUBLIC_PSEUDOID_MASK & noninherited_flags._pseudoBits;
}

bool RenderStyle::hasPseudoStyle(PseudoId pseudo) const
{
    ASSERT(pseudo > NOPSEUDO);
    ASSERT(pseudo < FIRST_INTERNAL_PSEUDOID);
    return pseudoBit(pseudo) & noninherited_flags._pseudoBits;
}

void RenderStyle::setHasPseudoStyle(PseudoId pseudo)
{
    ASSERT(pseudo > NOPSEUDO);
    ASSERT(pseudo < FIRST_INTERNAL_PSEUDOID);
    noninherited_flags._pseudoBits |= pseudoBit(pseudo);
}

bool RenderStyle::hasUniquePseudoStyle() const
{
    if (!m_cachedPseudoStyles || styleType() != NOPSEUDO)
        return false;

    for (size_t i = 0; i < m_cachedPseudoStyles->size(); ++i) {
        RenderStyle* pseudoStyle = m_cachedPseudoStyles->at(i).get();
        if (pseudoStyle->unique())
            return true;
    }

    return false;
}

RenderStyle* RenderStyle::getCachedPseudoStyle(PseudoId pid) const
{
    if (!m_cachedPseudoStyles || !m_cachedPseudoStyles->size())
        return 0;

    if (styleType() != NOPSEUDO) 
        return 0;

    for (size_t i = 0; i < m_cachedPseudoStyles->size(); ++i) {
        RenderStyle* pseudoStyle = m_cachedPseudoStyles->at(i).get();
        if (pseudoStyle->styleType() == pid)
            return pseudoStyle;
    }

    return 0;
}

RenderStyle* RenderStyle::addCachedPseudoStyle(PassRefPtr<RenderStyle> pseudo)
{
    if (!pseudo)
        return 0;

    ASSERT(pseudo->styleType() > NOPSEUDO);

    RenderStyle* result = pseudo.get();

    if (!m_cachedPseudoStyles)
        m_cachedPseudoStyles = adoptPtr(new PseudoStyleCache);

    m_cachedPseudoStyles->append(pseudo);

    return result;
}

void RenderStyle::removeCachedPseudoStyle(PseudoId pid)
{
    if (!m_cachedPseudoStyles)
        return;
    for (size_t i = 0; i < m_cachedPseudoStyles->size(); ++i) {
        RenderStyle* pseudoStyle = m_cachedPseudoStyles->at(i).get();
        if (pseudoStyle->styleType() == pid) {
            m_cachedPseudoStyles->remove(i);
            return;
        }
    }
}

bool RenderStyle::inheritedNotEqual(const RenderStyle* other) const
{
    return inherited_flags != other->inherited_flags
           || inherited != other->inherited
#if ENABLE(SVG)
           || m_svgStyle->inheritedNotEqual(other->m_svgStyle.get())
#endif
           || rareInheritedData != other->rareInheritedData;
}

bool RenderStyle::inheritedDataShared(const RenderStyle* other) const
{
    // This is a fast check that only looks if the data structures are shared.
    return inherited_flags == other->inherited_flags
        && inherited.get() == other->inherited.get()
#if ENABLE(SVG)
        && m_svgStyle.get() == other->m_svgStyle.get()
#endif
        && rareInheritedData.get() == other->rareInheritedData.get();
}

static bool positionChangeIsMovementOnly(const LengthBox& a, const LengthBox& b, const Length& width)
{
    // If any unit types are different, then we can't guarantee
    // that this was just a movement.
    if (a.left().type() != b.left().type()
        || a.right().type() != b.right().type()
        || a.top().type() != b.top().type()
        || a.bottom().type() != b.bottom().type())
        return false;

    // Only one unit can be non-auto in the horizontal direction and
    // in the vertical direction.  Otherwise the adjustment of values
    // is changing the size of the box.
    if (!a.left().isIntrinsicOrAuto() && !a.right().isIntrinsicOrAuto())
        return false;
    if (!a.top().isIntrinsicOrAuto() && !a.bottom().isIntrinsicOrAuto())
        return false;
    // If our width is auto and left or right is specified then this 
    // is not just a movement - we need to resize to our container.
    if ((!a.left().isIntrinsicOrAuto() || !a.right().isIntrinsicOrAuto()) && width.isIntrinsicOrAuto())
        return false;

    // One of the units is fixed or percent in both directions and stayed
    // that way in the new style.  Therefore all we are doing is moving.
    return true;
}

bool RenderStyle::changeRequiresLayout(const RenderStyle* other, unsigned& changedContextSensitiveProperties) const
{
    if (m_box->width() != other->m_box->width()
        || m_box->minWidth() != other->m_box->minWidth()
        || m_box->maxWidth() != other->m_box->maxWidth()
        || m_box->height() != other->m_box->height()
        || m_box->minHeight() != other->m_box->minHeight()
        || m_box->maxHeight() != other->m_box->maxHeight())
        return true;

    if (m_box->verticalAlign() != other->m_box->verticalAlign() || noninherited_flags._vertical_align != other->noninherited_flags._vertical_align)
        return true;

    if (m_box->boxSizing() != other->m_box->boxSizing())
        return true;

    if (surround->margin != other->surround->margin)
        return true;

    if (surround->padding != other->surround->padding)
        return true;

    if (rareNonInheritedData.get() != other->rareNonInheritedData.get()) {
        if (rareNonInheritedData->m_appearance != other->rareNonInheritedData->m_appearance 
            || rareNonInheritedData->marginBeforeCollapse != other->rareNonInheritedData->marginBeforeCollapse
            || rareNonInheritedData->marginAfterCollapse != other->rareNonInheritedData->marginAfterCollapse
            || rareNonInheritedData->lineClamp != other->rareNonInheritedData->lineClamp
            || rareNonInheritedData->textOverflow != other->rareNonInheritedData->textOverflow)
            return true;

        if (rareNonInheritedData->m_regionFragment != other->rareNonInheritedData->m_regionFragment)
            return true;

        if (rareNonInheritedData->m_wrapFlow != other->rareNonInheritedData->m_wrapFlow
            || rareNonInheritedData->m_wrapThrough != other->rareNonInheritedData->m_wrapThrough
            || rareNonInheritedData->m_shapeMargin != other->rareNonInheritedData->m_shapeMargin
            || rareNonInheritedData->m_shapePadding != other->rareNonInheritedData->m_shapePadding)
            return true;

        if (rareNonInheritedData->m_deprecatedFlexibleBox.get() != other->rareNonInheritedData->m_deprecatedFlexibleBox.get()
            && *rareNonInheritedData->m_deprecatedFlexibleBox.get() != *other->rareNonInheritedData->m_deprecatedFlexibleBox.get())
            return true;

        if (rareNonInheritedData->m_flexibleBox.get() != other->rareNonInheritedData->m_flexibleBox.get()
            && *rareNonInheritedData->m_flexibleBox.get() != *other->rareNonInheritedData->m_flexibleBox.get())
            return true;
        if (rareNonInheritedData->m_order != other->rareNonInheritedData->m_order
            || rareNonInheritedData->m_alignContent != other->rareNonInheritedData->m_alignContent
            || rareNonInheritedData->m_alignItems != other->rareNonInheritedData->m_alignItems
            || rareNonInheritedData->m_alignSelf != other->rareNonInheritedData->m_alignSelf
            || rareNonInheritedData->m_justifyContent != other->rareNonInheritedData->m_justifyContent)
            return true;

        // FIXME: We should add an optimized form of layout that just recomputes visual overflow.
        if (!rareNonInheritedData->shadowDataEquivalent(*other->rareNonInheritedData.get()))
            return true;

        if (!rareNonInheritedData->reflectionDataEquivalent(*other->rareNonInheritedData.get()))
            return true;

        if (rareNonInheritedData->m_multiCol.get() != other->rareNonInheritedData->m_multiCol.get()
            && *rareNonInheritedData->m_multiCol.get() != *other->rareNonInheritedData->m_multiCol.get())
            return true;

        if (rareNonInheritedData->m_transform.get() != other->rareNonInheritedData->m_transform.get()
            && *rareNonInheritedData->m_transform.get() != *other->rareNonInheritedData->m_transform.get()) {
#if USE(ACCELERATED_COMPOSITING)
            changedContextSensitiveProperties |= ContextSensitivePropertyTransform;
            // Don't return; keep looking for another change
#else
            return true;
#endif
        }

        if (rareNonInheritedData->m_grid.get() != other->rareNonInheritedData->m_grid.get()
            || rareNonInheritedData->m_gridItem.get() != other->rareNonInheritedData->m_gridItem.get())
            return true;

#if !USE(ACCELERATED_COMPOSITING)
        if (rareNonInheritedData.get() != other->rareNonInheritedData.get()) {
            if (rareNonInheritedData->m_transformStyle3D != other->rareNonInheritedData->m_transformStyle3D
                || rareNonInheritedData->m_backfaceVisibility != other->rareNonInheritedData->m_backfaceVisibility
                || rareNonInheritedData->m_perspective != other->rareNonInheritedData->m_perspective
                || rareNonInheritedData->m_perspectiveOriginX != other->rareNonInheritedData->m_perspectiveOriginX
                || rareNonInheritedData->m_perspectiveOriginY != other->rareNonInheritedData->m_perspectiveOriginY)
                return true;
        }
#endif

#if ENABLE(DASHBOARD_SUPPORT)
        // If regions change, trigger a relayout to re-calc regions.
        if (rareNonInheritedData->m_dashboardRegions != other->rareNonInheritedData->m_dashboardRegions)
            return true;
#endif

#if ENABLE(CSS_SHAPES)
        if (rareNonInheritedData->m_shapeInside != other->rareNonInheritedData->m_shapeInside)
            return true;
#endif
    }

    if (rareInheritedData.get() != other->rareInheritedData.get()) {
        if (rareInheritedData->highlight != other->rareInheritedData->highlight
            || rareInheritedData->indent != other->rareInheritedData->indent
#if ENABLE(CSS3_TEXT)
            || rareInheritedData->m_textIndentLine != other->rareInheritedData->m_textIndentLine
#endif
            || rareInheritedData->m_effectiveZoom != other->rareInheritedData->m_effectiveZoom
            || rareInheritedData->wordBreak != other->rareInheritedData->wordBreak
            || rareInheritedData->overflowWrap != other->rareInheritedData->overflowWrap
            || rareInheritedData->nbspMode != other->rareInheritedData->nbspMode
            || rareInheritedData->lineBreak != other->rareInheritedData->lineBreak
            || rareInheritedData->textSecurity != other->rareInheritedData->textSecurity
            || rareInheritedData->hyphens != other->rareInheritedData->hyphens
            || rareInheritedData->hyphenationLimitBefore != other->rareInheritedData->hyphenationLimitBefore
            || rareInheritedData->hyphenationLimitAfter != other->rareInheritedData->hyphenationLimitAfter
            || rareInheritedData->hyphenationString != other->rareInheritedData->hyphenationString
            || rareInheritedData->locale != other->rareInheritedData->locale
            || rareInheritedData->m_rubyPosition != other->rareInheritedData->m_rubyPosition
            || rareInheritedData->textEmphasisMark != other->rareInheritedData->textEmphasisMark
            || rareInheritedData->textEmphasisPosition != other->rareInheritedData->textEmphasisPosition
            || rareInheritedData->textEmphasisCustomMark != other->rareInheritedData->textEmphasisCustomMark
            || rareInheritedData->m_textOrientation != other->rareInheritedData->m_textOrientation
            || rareInheritedData->m_tabSize != other->rareInheritedData->m_tabSize
            || rareInheritedData->m_lineBoxContain != other->rareInheritedData->m_lineBoxContain
            || rareInheritedData->m_lineGrid != other->rareInheritedData->m_lineGrid
#if ENABLE(CSS_IMAGE_RESOLUTION)
            || rareInheritedData->m_imageResolutionSource != other->rareInheritedData->m_imageResolutionSource
            || rareInheritedData->m_imageResolutionSnap != other->rareInheritedData->m_imageResolutionSnap
            || rareInheritedData->m_imageResolution != other->rareInheritedData->m_imageResolution
#endif
            || rareInheritedData->m_lineSnap != other->rareInheritedData->m_lineSnap
            || rareInheritedData->m_lineAlign != other->rareInheritedData->m_lineAlign
            || rareInheritedData->listStyleImage != other->rareInheritedData->listStyleImage)
            return true;

        if (!rareInheritedData->shadowDataEquivalent(*other->rareInheritedData.get()))
            return true;

        if (textStrokeWidth() != other->textStrokeWidth())
            return true;
    }

#if ENABLE(TEXT_AUTOSIZING)
    if (visual->m_textAutosizingMultiplier != other->visual->m_textAutosizingMultiplier)
        return true;
#endif

    if (inherited->line_height != other->inherited->line_height
        || inherited->font != other->inherited->font
        || inherited->horizontal_border_spacing != other->inherited->horizontal_border_spacing
        || inherited->vertical_border_spacing != other->inherited->vertical_border_spacing
        || inherited_flags._box_direction != other->inherited_flags._box_direction
        || inherited_flags.m_rtlOrdering != other->inherited_flags.m_rtlOrdering
        || noninherited_flags._position != other->noninherited_flags._position
        || noninherited_flags._floating != other->noninherited_flags._floating
        || noninherited_flags._originalDisplay != other->noninherited_flags._originalDisplay)
        return true;


    if (((int)noninherited_flags._effectiveDisplay) >= TABLE) {
        if (inherited_flags._border_collapse != other->inherited_flags._border_collapse
            || inherited_flags._empty_cells != other->inherited_flags._empty_cells
            || inherited_flags._caption_side != other->inherited_flags._caption_side
            || noninherited_flags._table_layout != other->noninherited_flags._table_layout)
            return true;

        // In the collapsing border model, 'hidden' suppresses other borders, while 'none'
        // does not, so these style differences can be width differences.
        if (inherited_flags._border_collapse
            && ((borderTopStyle() == BHIDDEN && other->borderTopStyle() == BNONE)
                || (borderTopStyle() == BNONE && other->borderTopStyle() == BHIDDEN)
                || (borderBottomStyle() == BHIDDEN && other->borderBottomStyle() == BNONE)
                || (borderBottomStyle() == BNONE && other->borderBottomStyle() == BHIDDEN)
                || (borderLeftStyle() == BHIDDEN && other->borderLeftStyle() == BNONE)
                || (borderLeftStyle() == BNONE && other->borderLeftStyle() == BHIDDEN)
                || (borderRightStyle() == BHIDDEN && other->borderRightStyle() == BNONE)
                || (borderRightStyle() == BNONE && other->borderRightStyle() == BHIDDEN)))
            return true;
    }

    if (noninherited_flags._effectiveDisplay == LIST_ITEM) {
        if (inherited_flags._list_style_type != other->inherited_flags._list_style_type
            || inherited_flags._list_style_position != other->inherited_flags._list_style_position)
            return true;
    }

    if (inherited_flags._text_align != other->inherited_flags._text_align
        || inherited_flags._text_transform != other->inherited_flags._text_transform
        || inherited_flags._direction != other->inherited_flags._direction
        || inherited_flags._white_space != other->inherited_flags._white_space
        || noninherited_flags._clear != other->noninherited_flags._clear
        || noninherited_flags._unicodeBidi != other->noninherited_flags._unicodeBidi)
        return true;

    // Check block flow direction.
    if (inherited_flags.m_writingMode != other->inherited_flags.m_writingMode)
        return true;

    // Check text combine mode.
    if (rareNonInheritedData->m_textCombine != other->rareNonInheritedData->m_textCombine)
        return true;

    // Overflow returns a layout hint.
    if (noninherited_flags._overflowX != other->noninherited_flags._overflowX
        || noninherited_flags._overflowY != other->noninherited_flags._overflowY)
        return true;

    // If our border widths change, then we need to layout.  Other changes to borders
    // only necessitate a repaint.
    if (borderLeftWidth() != other->borderLeftWidth()
        || borderTopWidth() != other->borderTopWidth()
        || borderBottomWidth() != other->borderBottomWidth()
        || borderRightWidth() != other->borderRightWidth())
        return true;

    // If the counter directives change, trigger a relayout to re-calculate counter values and rebuild the counter node tree.
    const CounterDirectiveMap* mapA = rareNonInheritedData->m_counterDirectives.get();
    const CounterDirectiveMap* mapB = other->rareNonInheritedData->m_counterDirectives.get();
    if (!(mapA == mapB || (mapA && mapB && *mapA == *mapB)))
        return true;

    if ((visibility() == COLLAPSE) != (other->visibility() == COLLAPSE))
        return true;

    if ((rareNonInheritedData->opacity == 1 && other->rareNonInheritedData->opacity < 1)
        || (rareNonInheritedData->opacity < 1 && other->rareNonInheritedData->opacity == 1)) {
        // FIXME: We would like to use SimplifiedLayout here, but we can't quite do that yet.
        // We need to make sure SimplifiedLayout can operate correctly on RenderInlines (we will need
        // to add a selfNeedsSimplifiedLayout bit in order to not get confused and taint every line).
        // In addition we need to solve the floating object issue when layers come and go. Right now
        // a full layout is necessary to keep floating object lists sane.
        return true;
    }

    const QuotesData* quotesDataA = rareInheritedData->quotes.get();
    const QuotesData* quotesDataB = other->rareInheritedData->quotes.get();
    if (!(quotesDataA == quotesDataB || (quotesDataA && quotesDataB && *quotesDataA == *quotesDataB)))
        return true;

    if (position() != StaticPosition) {
        if (surround->offset != other->surround->offset) {
            // FIXME: We would like to use SimplifiedLayout for relative positioning, but we can't quite do that yet.
            // We need to make sure SimplifiedLayout can operate correctly on RenderInlines (we will need
            // to add a selfNeedsSimplifiedLayout bit in order to not get confused and taint every line).
            if (position() != AbsolutePosition)
                return true;

            // Optimize for the case where a positioned layer is moving but not changing size.
            if (!positionChangeIsMovementOnly(surround->offset, other->surround->offset, m_box->width()))
                return true;
        }
    }
    
    return false;
}

bool RenderStyle::changeRequiresPositionedLayoutOnly(const RenderStyle* other, unsigned&) const
{
    if (position() == StaticPosition)
        return false;

    if (surround->offset != other->surround->offset) {
        // Optimize for the case where a positioned layer is moving but not changing size.
        if (position() == AbsolutePosition && positionChangeIsMovementOnly(surround->offset, other->surround->offset, m_box->width()))
            return true;
    }
    
    return false;
}

bool RenderStyle::changeRequiresLayerRepaint(const RenderStyle* other, unsigned& changedContextSensitiveProperties) const
{
    if (position() != StaticPosition) {
        if (m_box->zIndex() != other->m_box->zIndex()
            || m_box->hasAutoZIndex() != other->m_box->hasAutoZIndex()
            || visual->clip != other->visual->clip
            || visual->hasClip != other->visual->hasClip)
            return true;
    }

#if ENABLE(CSS_COMPOSITING)
    if (rareNonInheritedData->m_effectiveBlendMode != other->rareNonInheritedData->m_effectiveBlendMode)
        return true;
#endif

    if (rareNonInheritedData->opacity != other->rareNonInheritedData->opacity) {
#if USE(ACCELERATED_COMPOSITING)
        changedContextSensitiveProperties |= ContextSensitivePropertyOpacity;
        // Don't return; keep looking for another change.
#else
        return true;
#endif
    }

#if ENABLE(CSS_FILTERS)
    if (rareNonInheritedData->m_filter.get() != other->rareNonInheritedData->m_filter.get()
        && *rareNonInheritedData->m_filter.get() != *other->rareNonInheritedData->m_filter.get()) {
#if USE(ACCELERATED_COMPOSITING)
        changedContextSensitiveProperties |= ContextSensitivePropertyFilter;
        // Don't return; keep looking for another change.
#else
        return true;
#endif
    }
#endif

    if (rareNonInheritedData->m_mask != other->rareNonInheritedData->m_mask
        || rareNonInheritedData->m_maskBoxImage != other->rareNonInheritedData->m_maskBoxImage)
        return true;

    return false;
}

bool RenderStyle::changeRequiresRepaint(const RenderStyle* other, unsigned&) const
{
    if (inherited_flags._visibility != other->inherited_flags._visibility
        || inherited_flags.m_printColorAdjust != other->inherited_flags.m_printColorAdjust
        || inherited_flags._insideLink != other->inherited_flags._insideLink
        || surround->border != other->surround->border
        || !m_background->isEquivalentForPainting(*other->m_background)
        || rareInheritedData->userModify != other->rareInheritedData->userModify
        || rareInheritedData->userSelect != other->rareInheritedData->userSelect
        || rareNonInheritedData->userDrag != other->rareNonInheritedData->userDrag
        || rareNonInheritedData->m_borderFit != other->rareNonInheritedData->m_borderFit
        || rareInheritedData->m_imageRendering != other->rareInheritedData->m_imageRendering)
        return true;
        
    // FIXME: The current spec is being reworked to remove dependencies between exclusions and affected 
    // content. There's a proposal to use floats instead. In that case, wrap-shape should actually relayout 
    // the parent container. For sure, I will have to revisit this code, but for now I've added this in order 
    // to avoid having diff() == StyleDifferenceEqual where wrap-shapes actually differ.
    // Tracking bug: https://bugs.webkit.org/show_bug.cgi?id=62991
    if (rareNonInheritedData->m_shapeOutside != other->rareNonInheritedData->m_shapeOutside)
        return true;

    if (rareNonInheritedData->m_clipPath != other->rareNonInheritedData->m_clipPath)
        return true;

    return false;
}

bool RenderStyle::changeRequiresRepaintIfText(const RenderStyle* other, unsigned&) const
{
    if (inherited->color != other->inherited->color
        || inherited_flags._text_decorations != other->inherited_flags._text_decorations
        || visual->textDecoration != other->visual->textDecoration
#if ENABLE(CSS3_TEXT)
        || rareNonInheritedData->m_textDecorationStyle != other->rareNonInheritedData->m_textDecorationStyle
        || rareNonInheritedData->m_textDecorationColor != other->rareNonInheritedData->m_textDecorationColor
#endif // CSS3_TEXT
        || rareInheritedData->textFillColor != other->rareInheritedData->textFillColor
        || rareInheritedData->textStrokeColor != other->rareInheritedData->textStrokeColor
        || rareInheritedData->textEmphasisColor != other->rareInheritedData->textEmphasisColor
        || rareInheritedData->textEmphasisFill != other->rareInheritedData->textEmphasisFill)
        return true;

    return false;
}

bool RenderStyle::changeRequiresRecompositeLayer(const RenderStyle* other, unsigned&) const
{
#if USE(ACCELERATED_COMPOSITING)
    if (rareNonInheritedData.get() != other->rareNonInheritedData.get()) {
        if (rareNonInheritedData->m_transformStyle3D != other->rareNonInheritedData->m_transformStyle3D
            || rareNonInheritedData->m_backfaceVisibility != other->rareNonInheritedData->m_backfaceVisibility
            || rareNonInheritedData->m_perspective != other->rareNonInheritedData->m_perspective
            || rareNonInheritedData->m_perspectiveOriginX != other->rareNonInheritedData->m_perspectiveOriginX
            || rareNonInheritedData->m_perspectiveOriginY != other->rareNonInheritedData->m_perspectiveOriginY)
            return true;
    }
#endif
    return false;
}

StyleDifference RenderStyle::diff(const RenderStyle* other, unsigned& changedContextSensitiveProperties) const
{
    changedContextSensitiveProperties = ContextSensitivePropertyNone;

#if ENABLE(SVG)
    StyleDifference svgChange = StyleDifferenceEqual;
    if (m_svgStyle != other->m_svgStyle) {
        svgChange = m_svgStyle->diff(other->m_svgStyle.get());
        if (svgChange == StyleDifferenceLayout)
            return svgChange;
    }
#endif

    if (changeRequiresLayout(other, changedContextSensitiveProperties))
        return StyleDifferenceLayout;

#if ENABLE(SVG)
    // SVGRenderStyle::diff() might have returned StyleDifferenceRepaint, eg. if fill changes.
    // If eg. the font-size changed at the same time, we're not allowed to return StyleDifferenceRepaint,
    // but have to return StyleDifferenceLayout, that's why  this if branch comes after all branches
    // that are relevant for SVG and might return StyleDifferenceLayout.
    if (svgChange != StyleDifferenceEqual)
        return svgChange;
#endif

    if (changeRequiresPositionedLayoutOnly(other, changedContextSensitiveProperties))
        return StyleDifferenceLayoutPositionedMovementOnly;

    if (changeRequiresLayerRepaint(other, changedContextSensitiveProperties))
        return StyleDifferenceRepaintLayer;

    if (changeRequiresRepaint(other, changedContextSensitiveProperties))
        return StyleDifferenceRepaint;

#if USE(ACCELERATED_COMPOSITING)
    if (changeRequiresRecompositeLayer(other, changedContextSensitiveProperties))
        return StyleDifferenceRecompositeLayer;
#endif

    if (changeRequiresRepaintIfText(other, changedContextSensitiveProperties))
        return StyleDifferenceRepaintIfText;

    // Cursors are not checked, since they will be set appropriately in response to mouse events,
    // so they don't need to cause any repaint or layout.

    // Animations don't need to be checked either.  We always set the new style on the RenderObject, so we will get a chance to fire off
    // the resulting transition properly.
    return StyleDifferenceEqual;
}

bool RenderStyle::diffRequiresRepaint(const RenderStyle* style) const
{
    unsigned changedContextSensitiveProperties = 0;
    return changeRequiresRepaint(style, changedContextSensitiveProperties);
}

void RenderStyle::setClip(Length top, Length right, Length bottom, Length left)
{
    StyleVisualData* data = visual.access();
    data->clip.m_top = top;
    data->clip.m_right = right;
    data->clip.m_bottom = bottom;
    data->clip.m_left = left;
}

void RenderStyle::addCursor(PassRefPtr<StyleImage> image, const IntPoint& hotSpot)
{
    if (!rareInheritedData.access()->cursorData)
        rareInheritedData.access()->cursorData = CursorList::create();
    rareInheritedData.access()->cursorData->append(CursorData(image, hotSpot));
}

void RenderStyle::setCursorList(PassRefPtr<CursorList> other)
{
    rareInheritedData.access()->cursorData = other;
}

void RenderStyle::setQuotes(PassRefPtr<QuotesData> q)
{
    if (rareInheritedData->quotes == q || (rareInheritedData->quotes && q && *rareInheritedData->quotes == *q))
        return;

    rareInheritedData.access()->quotes = q;
}

void RenderStyle::clearCursorList()
{
    if (rareInheritedData->cursorData)
        rareInheritedData.access()->cursorData = 0;
}

void RenderStyle::clearContent()
{
    if (rareNonInheritedData->m_content)
        rareNonInheritedData.access()->m_content = nullptr;
}

void RenderStyle::appendContent(PassOwnPtr<ContentData> contentData)
{
    OwnPtr<ContentData>& content = rareNonInheritedData.access()->m_content;
    ContentData* lastContent = content.get();
    while (lastContent && lastContent->next())
        lastContent = lastContent->next();

    if (lastContent)
        lastContent->setNext(contentData);
    else
        content = contentData;
}

void RenderStyle::setContent(PassRefPtr<StyleImage> image, bool add)
{
    if (!image)
        return;
        
    if (add) {
        appendContent(ContentData::create(image));
        return;
    }

    rareNonInheritedData.access()->m_content = ContentData::create(image);
}

void RenderStyle::setContent(const String& string, bool add)
{
    OwnPtr<ContentData>& content = rareNonInheritedData.access()->m_content;
    if (add) {
        ContentData* lastContent = content.get();
        while (lastContent && lastContent->next())
            lastContent = lastContent->next();

        if (lastContent) {
            // We attempt to merge with the last ContentData if possible.
            if (lastContent->isText()) {
                TextContentData* textContent = static_cast<TextContentData*>(lastContent);
                textContent->setText(textContent->text() + string);
            } else
                lastContent->setNext(ContentData::create(string));

            return;
        }
    }

    content = ContentData::create(string);
}

void RenderStyle::setContent(PassOwnPtr<CounterContent> counter, bool add)
{
    if (!counter)
        return;

    if (add) {
        appendContent(ContentData::create(counter));
        return;
    }

    rareNonInheritedData.access()->m_content = ContentData::create(counter);
}

void RenderStyle::setContent(QuoteType quote, bool add)
{
    if (add) {
        appendContent(ContentData::create(quote));
        return;
    }

    rareNonInheritedData.access()->m_content = ContentData::create(quote);
}
    
inline bool requireTransformOrigin(const Vector<RefPtr<TransformOperation> >& transformOperations, RenderStyle::ApplyTransformOrigin applyOrigin)
{
    // transform-origin brackets the transform with translate operations.
    // Optimize for the case where the only transform is a translation, since the transform-origin is irrelevant
    // in that case.
    if (applyOrigin != RenderStyle::IncludeTransformOrigin)
        return false;

    unsigned size = transformOperations.size();
    for (unsigned i = 0; i < size; ++i) {
        TransformOperation::OperationType type = transformOperations[i]->getOperationType();
        if (type != TransformOperation::TRANSLATE_X
            && type != TransformOperation::TRANSLATE_Y
            && type != TransformOperation::TRANSLATE 
            && type != TransformOperation::TRANSLATE_Z
            && type != TransformOperation::TRANSLATE_3D)
            return true;
    }
    
    return false;
}

void RenderStyle::applyTransform(TransformationMatrix& transform, const LayoutSize& borderBoxSize, ApplyTransformOrigin applyOrigin) const
{
    // FIXME: when subpixel layout is supported (bug 71143) the body of this function could be replaced by
    // applyTransform(transform, FloatRect(FloatPoint(), borderBoxSize), applyOrigin);
    
    const Vector<RefPtr<TransformOperation> >& transformOperations = rareNonInheritedData->m_transform->m_operations.operations();
    bool applyTransformOrigin = requireTransformOrigin(transformOperations, applyOrigin);

    if (applyTransformOrigin)
        transform.translate3d(floatValueForLength(transformOriginX(), borderBoxSize.width()), floatValueForLength(transformOriginY(), borderBoxSize.height()), transformOriginZ());

    unsigned size = transformOperations.size();
    for (unsigned i = 0; i < size; ++i)
        transformOperations[i]->apply(transform, borderBoxSize);

    if (applyTransformOrigin)
        transform.translate3d(-floatValueForLength(transformOriginX(), borderBoxSize.width()), -floatValueForLength(transformOriginY(), borderBoxSize.height()), -transformOriginZ()); 
}

void RenderStyle::applyTransform(TransformationMatrix& transform, const FloatRect& boundingBox, ApplyTransformOrigin applyOrigin) const
{
    const Vector<RefPtr<TransformOperation> >& transformOperations = rareNonInheritedData->m_transform->m_operations.operations();
    bool applyTransformOrigin = requireTransformOrigin(transformOperations, applyOrigin);
    
    float offsetX = transformOriginX().type() == Percent ? boundingBox.x() : 0;
    float offsetY = transformOriginY().type() == Percent ? boundingBox.y() : 0;
    
    if (applyTransformOrigin) {
        transform.translate3d(floatValueForLength(transformOriginX(), boundingBox.width()) + offsetX,
                              floatValueForLength(transformOriginY(), boundingBox.height()) + offsetY,
                              transformOriginZ());
    }
    
    unsigned size = transformOperations.size();
    for (unsigned i = 0; i < size; ++i)
        transformOperations[i]->apply(transform, boundingBox.size());
    
    if (applyTransformOrigin) {
        transform.translate3d(-floatValueForLength(transformOriginX(), boundingBox.width()) - offsetX,
                              -floatValueForLength(transformOriginY(), boundingBox.height()) - offsetY,
                              -transformOriginZ());
    }
}

void RenderStyle::setPageScaleTransform(float scale)
{
    if (scale == 1)
        return;
    TransformOperations transform;
    transform.operations().append(ScaleTransformOperation::create(scale, scale, ScaleTransformOperation::SCALE));
    setTransform(transform);
    setTransformOriginX(Length(0, Fixed));
    setTransformOriginY(Length(0, Fixed));
}

void RenderStyle::setTextShadow(PassOwnPtr<ShadowData> shadowData, bool add)
{
    ASSERT(!shadowData || (!shadowData->spread() && shadowData->style() == Normal));

    StyleRareInheritedData* rareData = rareInheritedData.access();
    if (!add) {
        rareData->textShadow = shadowData;
        return;
    }

    shadowData->setNext(rareData->textShadow.release());
    rareData->textShadow = shadowData;
}

void RenderStyle::setBoxShadow(PassOwnPtr<ShadowData> shadowData, bool add)
{
    StyleRareNonInheritedData* rareData = rareNonInheritedData.access();
    if (!add) {
        rareData->m_boxShadow = shadowData;
        return;
    }

    shadowData->setNext(rareData->m_boxShadow.release());
    rareData->m_boxShadow = shadowData;
}

static RoundedRect::Radii calcRadiiFor(const BorderData& border, IntSize size, RenderView* renderView)
{
    return RoundedRect::Radii(
        IntSize(valueForLength(border.topLeft().width(), size.width(), renderView), 
                valueForLength(border.topLeft().height(), size.height(), renderView)),
        IntSize(valueForLength(border.topRight().width(), size.width(), renderView),
                valueForLength(border.topRight().height(), size.height(), renderView)),
        IntSize(valueForLength(border.bottomLeft().width(), size.width(), renderView), 
                valueForLength(border.bottomLeft().height(), size.height(), renderView)),
        IntSize(valueForLength(border.bottomRight().width(), size.width(), renderView), 
                valueForLength(border.bottomRight().height(), size.height(), renderView)));
}

static float calcConstraintScaleFor(const IntRect& rect, const RoundedRect::Radii& radii)
{
    // Constrain corner radii using CSS3 rules:
    // http://www.w3.org/TR/css3-background/#the-border-radius
    
    float factor = 1;
    unsigned radiiSum;

    // top
    radiiSum = static_cast<unsigned>(radii.topLeft().width()) + static_cast<unsigned>(radii.topRight().width()); // Casts to avoid integer overflow.
    if (radiiSum > static_cast<unsigned>(rect.width()))
        factor = min(static_cast<float>(rect.width()) / radiiSum, factor);

    // bottom
    radiiSum = static_cast<unsigned>(radii.bottomLeft().width()) + static_cast<unsigned>(radii.bottomRight().width());
    if (radiiSum > static_cast<unsigned>(rect.width()))
        factor = min(static_cast<float>(rect.width()) / radiiSum, factor);
    
    // left
    radiiSum = static_cast<unsigned>(radii.topLeft().height()) + static_cast<unsigned>(radii.bottomLeft().height());
    if (radiiSum > static_cast<unsigned>(rect.height()))
        factor = min(static_cast<float>(rect.height()) / radiiSum, factor);
    
    // right
    radiiSum = static_cast<unsigned>(radii.topRight().height()) + static_cast<unsigned>(radii.bottomRight().height());
    if (radiiSum > static_cast<unsigned>(rect.height()))
        factor = min(static_cast<float>(rect.height()) / radiiSum, factor);
    
    ASSERT(factor <= 1);
    return factor;
}

StyleImage* RenderStyle::listStyleImage() const { return rareInheritedData->listStyleImage.get(); }
void RenderStyle::setListStyleImage(PassRefPtr<StyleImage> v)
{
    if (rareInheritedData->listStyleImage != v)
        rareInheritedData.access()->listStyleImage = v;
}

Color RenderStyle::color() const { return inherited->color; }
Color RenderStyle::visitedLinkColor() const { return inherited->visitedLinkColor; }
void RenderStyle::setColor(const Color& v) { SET_VAR(inherited, color, v); }
void RenderStyle::setVisitedLinkColor(const Color& v) { SET_VAR(inherited, visitedLinkColor, v); }

short RenderStyle::horizontalBorderSpacing() const { return inherited->horizontal_border_spacing; }
short RenderStyle::verticalBorderSpacing() const { return inherited->vertical_border_spacing; }
void RenderStyle::setHorizontalBorderSpacing(short v) { SET_VAR(inherited, horizontal_border_spacing, v); }
void RenderStyle::setVerticalBorderSpacing(short v) { SET_VAR(inherited, vertical_border_spacing, v); }

RoundedRect RenderStyle::getRoundedBorderFor(const LayoutRect& borderRect, RenderView* renderView, bool includeLogicalLeftEdge, bool includeLogicalRightEdge) const
{
    IntRect snappedBorderRect(pixelSnappedIntRect(borderRect));
    RoundedRect roundedRect(snappedBorderRect);
    if (hasBorderRadius()) {
        RoundedRect::Radii radii = calcRadiiFor(surround->border, snappedBorderRect.size(), renderView);
        radii.scale(calcConstraintScaleFor(snappedBorderRect, radii));
        roundedRect.includeLogicalEdges(radii, isHorizontalWritingMode(), includeLogicalLeftEdge, includeLogicalRightEdge);
    }
    return roundedRect;
}

RoundedRect RenderStyle::getRoundedInnerBorderFor(const LayoutRect& borderRect, bool includeLogicalLeftEdge, bool includeLogicalRightEdge) const
{
    bool horizontal = isHorizontalWritingMode();

    int leftWidth = (!horizontal || includeLogicalLeftEdge) ? borderLeftWidth() : 0;
    int rightWidth = (!horizontal || includeLogicalRightEdge) ? borderRightWidth() : 0;
    int topWidth = (horizontal || includeLogicalLeftEdge) ? borderTopWidth() : 0;
    int bottomWidth = (horizontal || includeLogicalRightEdge) ? borderBottomWidth() : 0;

    return getRoundedInnerBorderFor(borderRect, topWidth, bottomWidth, leftWidth, rightWidth, includeLogicalLeftEdge, includeLogicalRightEdge);
}

RoundedRect RenderStyle::getRoundedInnerBorderFor(const LayoutRect& borderRect,
    int topWidth, int bottomWidth, int leftWidth, int rightWidth, bool includeLogicalLeftEdge, bool includeLogicalRightEdge) const
{
    LayoutRect innerRect(borderRect.x() + leftWidth, 
               borderRect.y() + topWidth, 
               borderRect.width() - leftWidth - rightWidth, 
               borderRect.height() - topWidth - bottomWidth);

    RoundedRect roundedRect(pixelSnappedIntRect(innerRect));

    if (hasBorderRadius()) {
        RoundedRect::Radii radii = getRoundedBorderFor(borderRect).radii();
        radii.shrink(topWidth, bottomWidth, leftWidth, rightWidth);
        roundedRect.includeLogicalEdges(radii, isHorizontalWritingMode(), includeLogicalLeftEdge, includeLogicalRightEdge);
    }
    return roundedRect;
}

static bool allLayersAreFixed(const FillLayer* layer)
{
    bool allFixed = true;
    
    for (const FillLayer* currLayer = layer; currLayer; currLayer = currLayer->next())
        allFixed &= (currLayer->image() && currLayer->attachment() == FixedBackgroundAttachment);

    return layer && allFixed;
}

bool RenderStyle::hasEntirelyFixedBackground() const
{
    return allLayersAreFixed(backgroundLayers());
}

const CounterDirectiveMap* RenderStyle::counterDirectives() const
{
    return rareNonInheritedData->m_counterDirectives.get();
}

CounterDirectiveMap& RenderStyle::accessCounterDirectives()
{
    OwnPtr<CounterDirectiveMap>& map = rareNonInheritedData.access()->m_counterDirectives;
    if (!map)
        map = adoptPtr(new CounterDirectiveMap);
    return *map;
}

const CounterDirectives RenderStyle::getCounterDirectives(const AtomicString& identifier) const
{
    if (const CounterDirectiveMap* directives = counterDirectives())
        return directives->get(identifier);
    return CounterDirectives();
}

const AtomicString& RenderStyle::hyphenString() const
{
    ASSERT(hyphens() != HyphensNone);

    const AtomicString& hyphenationString = rareInheritedData.get()->hyphenationString;
    if (!hyphenationString.isNull())
        return hyphenationString;

    // FIXME: This should depend on locale.
    DEFINE_STATIC_LOCAL(AtomicString, hyphenMinusString, (&hyphenMinus, 1));
    DEFINE_STATIC_LOCAL(AtomicString, hyphenString, (&hyphen, 1));
    return font().primaryFontHasGlyphForCharacter(hyphen) ? hyphenString : hyphenMinusString;
}

const AtomicString& RenderStyle::textEmphasisMarkString() const
{
    switch (textEmphasisMark()) {
    case TextEmphasisMarkNone:
        return nullAtom;
    case TextEmphasisMarkCustom:
        return textEmphasisCustomMark();
    case TextEmphasisMarkDot: {
        DEFINE_STATIC_LOCAL(AtomicString, filledDotString, (&bullet, 1));
        DEFINE_STATIC_LOCAL(AtomicString, openDotString, (&whiteBullet, 1));
        return textEmphasisFill() == TextEmphasisFillFilled ? filledDotString : openDotString;
    }
    case TextEmphasisMarkCircle: {
        DEFINE_STATIC_LOCAL(AtomicString, filledCircleString, (&blackCircle, 1));
        DEFINE_STATIC_LOCAL(AtomicString, openCircleString, (&whiteCircle, 1));
        return textEmphasisFill() == TextEmphasisFillFilled ? filledCircleString : openCircleString;
    }
    case TextEmphasisMarkDoubleCircle: {
        DEFINE_STATIC_LOCAL(AtomicString, filledDoubleCircleString, (&fisheye, 1));
        DEFINE_STATIC_LOCAL(AtomicString, openDoubleCircleString, (&bullseye, 1));
        return textEmphasisFill() == TextEmphasisFillFilled ? filledDoubleCircleString : openDoubleCircleString;
    }
    case TextEmphasisMarkTriangle: {
        DEFINE_STATIC_LOCAL(AtomicString, filledTriangleString, (&blackUpPointingTriangle, 1));
        DEFINE_STATIC_LOCAL(AtomicString, openTriangleString, (&whiteUpPointingTriangle, 1));
        return textEmphasisFill() == TextEmphasisFillFilled ? filledTriangleString : openTriangleString;
    }
    case TextEmphasisMarkSesame: {
        DEFINE_STATIC_LOCAL(AtomicString, filledSesameString, (&sesameDot, 1));
        DEFINE_STATIC_LOCAL(AtomicString, openSesameString, (&whiteSesameDot, 1));
        return textEmphasisFill() == TextEmphasisFillFilled ? filledSesameString : openSesameString;
    }
    case TextEmphasisMarkAuto:
        ASSERT_NOT_REACHED();
        return nullAtom;
    }

    ASSERT_NOT_REACHED();
    return nullAtom;
}

#if ENABLE(DASHBOARD_SUPPORT)
const Vector<StyleDashboardRegion>& RenderStyle::initialDashboardRegions()
{
    DEFINE_STATIC_LOCAL(Vector<StyleDashboardRegion>, emptyList, ());
    return emptyList;
}

const Vector<StyleDashboardRegion>& RenderStyle::noneDashboardRegions()
{
    DEFINE_STATIC_LOCAL(Vector<StyleDashboardRegion>, noneList, ());
    static bool noneListInitialized = false;

    if (!noneListInitialized) {
        StyleDashboardRegion region;
        region.label = "";
        region.offset.m_top  = Length();
        region.offset.m_right = Length();
        region.offset.m_bottom = Length();
        region.offset.m_left = Length();
        region.type = StyleDashboardRegion::None;
        noneList.append(region);
        noneListInitialized = true;
    }
    return noneList;
}
#endif

void RenderStyle::adjustAnimations()
{
    AnimationList* animationList = rareNonInheritedData->m_animations.get();
    if (!animationList)
        return;

    // Get rid of empty animations and anything beyond them
    for (size_t i = 0; i < animationList->size(); ++i) {
        if (animationList->animation(i)->isEmpty()) {
            animationList->resize(i);
            break;
        }
    }

    if (animationList->isEmpty()) {
        clearAnimations();
        return;
    }

    // Repeat patterns into layers that don't have some properties set.
    animationList->fillUnsetProperties();
}

void RenderStyle::adjustTransitions()
{
    AnimationList* transitionList = rareNonInheritedData->m_transitions.get();
    if (!transitionList)
        return;

    // Get rid of empty transitions and anything beyond them
    for (size_t i = 0; i < transitionList->size(); ++i) {
        if (transitionList->animation(i)->isEmpty()) {
            transitionList->resize(i);
            break;
        }
    }

    if (transitionList->isEmpty()) {
        clearTransitions();
        return;
    }

    // Repeat patterns into layers that don't have some properties set.
    transitionList->fillUnsetProperties();

    // Make sure there are no duplicate properties. This is an O(n^2) algorithm
    // but the lists tend to be very short, so it is probably ok
    for (size_t i = 0; i < transitionList->size(); ++i) {
        for (size_t j = i+1; j < transitionList->size(); ++j) {
            if (transitionList->animation(i)->property() == transitionList->animation(j)->property()) {
                // toss i
                transitionList->remove(i);
                j = i;
            }
        }
    }
}

AnimationList* RenderStyle::accessAnimations()
{
    if (!rareNonInheritedData.access()->m_animations)
        rareNonInheritedData.access()->m_animations = adoptPtr(new AnimationList());
    return rareNonInheritedData->m_animations.get();
}

AnimationList* RenderStyle::accessTransitions()
{
    if (!rareNonInheritedData.access()->m_transitions)
        rareNonInheritedData.access()->m_transitions = adoptPtr(new AnimationList());
    return rareNonInheritedData->m_transitions.get();
}

const Animation* RenderStyle::transitionForProperty(CSSPropertyID property) const
{
    if (transitions()) {
        for (size_t i = 0; i < transitions()->size(); ++i) {
            const Animation* p = transitions()->animation(i);
            if (p->animationMode() == Animation::AnimateAll || p->property() == property) {
                return p;
            }
        }
    }
    return 0;
}

const Font& RenderStyle::font() const { return inherited->font; }
const FontMetrics& RenderStyle::fontMetrics() const { return inherited->font.fontMetrics(); }
const FontDescription& RenderStyle::fontDescription() const { return inherited->font.fontDescription(); }
float RenderStyle::specifiedFontSize() const { return fontDescription().specifiedSize(); }
float RenderStyle::computedFontSize() const { return fontDescription().computedSize(); }
int RenderStyle::fontSize() const { return inherited->font.pixelSize(); }

int RenderStyle::wordSpacing() const { return inherited->font.wordSpacing(); }
int RenderStyle::letterSpacing() const { return inherited->font.letterSpacing(); }

bool RenderStyle::setFontDescription(const FontDescription& v)
{
    if (inherited->font.fontDescription() != v) {
        inherited.access()->font = Font(v, inherited->font.letterSpacing(), inherited->font.wordSpacing());
        return true;
    }
    return false;
}

Length RenderStyle::specifiedLineHeight() const { return inherited->line_height; }
Length RenderStyle::lineHeight() const
{
    const Length& lh = inherited->line_height;
#if ENABLE(TEXT_AUTOSIZING)
    // Unlike fontDescription().computedSize() and hence fontSize(), this is
    // recalculated on demand as we only store the specified line height.
    // FIXME: Should consider scaling the fixed part of any calc expressions
    // too, though this involves messily poking into CalcExpressionLength.
    float multiplier = textAutosizingMultiplier();
    if (multiplier > 1 && lh.isFixed())
        return Length(TextAutosizer::computeAutosizedFontSize(lh.value(), multiplier), Fixed);
#endif
    return lh;
}
void RenderStyle::setLineHeight(Length specifiedLineHeight) { SET_VAR(inherited, line_height, specifiedLineHeight); }

int RenderStyle::computedLineHeight(RenderView* renderView) const
{
    const Length& lh = lineHeight();

    // Negative value means the line height is not set. Use the font's built-in spacing.
    if (lh.isNegative())
        return fontMetrics().lineSpacing();

    if (lh.isPercent())
        return minimumValueForLength(lh, fontSize());

    if (lh.isViewportPercentage())
        return valueForLength(lh, 0, renderView);

    return lh.value();
}

void RenderStyle::setWordSpacing(int v) { inherited.access()->font.setWordSpacing(v); }
void RenderStyle::setLetterSpacing(int v) { inherited.access()->font.setLetterSpacing(v); }

void RenderStyle::setFontSize(float size)
{
    // size must be specifiedSize if Text Autosizing is enabled, but computedSize if text
    // zoom is enabled (if neither is enabled it's irrelevant as they're probably the same).

    ASSERT(std::isfinite(size));
    if (!std::isfinite(size) || size < 0)
        size = 0;
    else
        size = min(maximumAllowedFontSize, size);

    FontSelector* currentFontSelector = font().fontSelector();
    FontDescription desc(fontDescription());
    desc.setSpecifiedSize(size);
    desc.setComputedSize(size);

#if ENABLE(TEXT_AUTOSIZING)
    float multiplier = textAutosizingMultiplier();
    if (multiplier > 1) {
        float autosizedFontSize = TextAutosizer::computeAutosizedFontSize(size, multiplier);
        desc.setComputedSize(min(maximumAllowedFontSize, autosizedFontSize));
    }
#endif

    setFontDescription(desc);
    font().update(currentFontSelector);
}

void RenderStyle::getShadowExtent(const ShadowData* shadow, LayoutUnit &top, LayoutUnit &right, LayoutUnit &bottom, LayoutUnit &left) const
{
    top = 0;
    right = 0;
    bottom = 0;
    left = 0;

    for ( ; shadow; shadow = shadow->next()) {
        if (shadow->style() == Inset)
            continue;

        int extentAndSpread = shadow->paintingExtent() + shadow->spread();
        top = min<LayoutUnit>(top, shadow->y() - extentAndSpread);
        right = max<LayoutUnit>(right, shadow->x() + extentAndSpread);
        bottom = max<LayoutUnit>(bottom, shadow->y() + extentAndSpread);
        left = min<LayoutUnit>(left, shadow->x() - extentAndSpread);
    }
}

LayoutBoxExtent RenderStyle::getShadowInsetExtent(const ShadowData* shadow) const
{
    LayoutUnit top = 0;
    LayoutUnit right = 0;
    LayoutUnit bottom = 0;
    LayoutUnit left = 0;

    for ( ; shadow; shadow = shadow->next()) {
        if (shadow->style() == Normal)
            continue;

        int extentAndSpread = shadow->paintingExtent() + shadow->spread();
        top = max<LayoutUnit>(top, shadow->y() + extentAndSpread);
        right = min<LayoutUnit>(right, shadow->x() - extentAndSpread);
        bottom = min<LayoutUnit>(bottom, shadow->y() - extentAndSpread);
        left = max<LayoutUnit>(left, shadow->x() + extentAndSpread);
    }

    return LayoutBoxExtent(top, right, bottom, left);
}

void RenderStyle::getShadowHorizontalExtent(const ShadowData* shadow, LayoutUnit &left, LayoutUnit &right) const
{
    left = 0;
    right = 0;

    for ( ; shadow; shadow = shadow->next()) {
        if (shadow->style() == Inset)
            continue;

        int extentAndSpread = shadow->paintingExtent() + shadow->spread();
        left = min<LayoutUnit>(left, shadow->x() - extentAndSpread);
        right = max<LayoutUnit>(right, shadow->x() + extentAndSpread);
    }
}

void RenderStyle::getShadowVerticalExtent(const ShadowData* shadow, LayoutUnit &top, LayoutUnit &bottom) const
{
    top = 0;
    bottom = 0;

    for ( ; shadow; shadow = shadow->next()) {
        if (shadow->style() == Inset)
            continue;

        int extentAndSpread = shadow->paintingExtent() + shadow->spread();
        top = min<LayoutUnit>(top, shadow->y() - extentAndSpread);
        bottom = max<LayoutUnit>(bottom, shadow->y() + extentAndSpread);
    }
}

Color RenderStyle::colorIncludingFallback(int colorProperty, bool visitedLink) const
{
    Color result;
    EBorderStyle borderStyle = BNONE;
    switch (colorProperty) {
    case CSSPropertyBackgroundColor:
        return visitedLink ? visitedLinkBackgroundColor() : backgroundColor(); // Background color doesn't fall back.
    case CSSPropertyBorderLeftColor:
        result = visitedLink ? visitedLinkBorderLeftColor() : borderLeftColor();
        borderStyle = borderLeftStyle();
        break;
    case CSSPropertyBorderRightColor:
        result = visitedLink ? visitedLinkBorderRightColor() : borderRightColor();
        borderStyle = borderRightStyle();
        break;
    case CSSPropertyBorderTopColor:
        result = visitedLink ? visitedLinkBorderTopColor() : borderTopColor();
        borderStyle = borderTopStyle();
        break;
    case CSSPropertyBorderBottomColor:
        result = visitedLink ? visitedLinkBorderBottomColor() : borderBottomColor();
        borderStyle = borderBottomStyle();
        break;
    case CSSPropertyColor:
        result = visitedLink ? visitedLinkColor() : color();
        break;
    case CSSPropertyOutlineColor:
        result = visitedLink ? visitedLinkOutlineColor() : outlineColor();
        break;
    case CSSPropertyWebkitColumnRuleColor:
        result = visitedLink ? visitedLinkColumnRuleColor() : columnRuleColor();
        break;
#if ENABLE(CSS3_TEXT)
    case CSSPropertyWebkitTextDecorationColor:
        // Text decoration color fallback is handled in RenderObject::decorationColor.
        return visitedLink ? visitedLinkTextDecorationColor() : textDecorationColor();
#endif // CSS3_TEXT
    case CSSPropertyWebkitTextEmphasisColor:
        result = visitedLink ? visitedLinkTextEmphasisColor() : textEmphasisColor();
        break;
    case CSSPropertyWebkitTextFillColor:
        result = visitedLink ? visitedLinkTextFillColor() : textFillColor();
        break;
    case CSSPropertyWebkitTextStrokeColor:
        result = visitedLink ? visitedLinkTextStrokeColor() : textStrokeColor();
        break;
    default:
        ASSERT_NOT_REACHED();
        break;
    }

    if (!result.isValid()) {
        if (!visitedLink && (borderStyle == INSET || borderStyle == OUTSET || borderStyle == RIDGE || borderStyle == GROOVE))
            result.setRGB(238, 238, 238);
        else
            result = visitedLink ? visitedLinkColor() : color();
    }
    return result;
}

Color RenderStyle::visitedDependentColor(int colorProperty) const
{
    Color unvisitedColor = colorIncludingFallback(colorProperty, false);
    if (insideLink() != InsideVisitedLink)
        return unvisitedColor;

    Color visitedColor = colorIncludingFallback(colorProperty, true);

#if ENABLE(CSS3_TEXT)
    // Text decoration color validity is preserved (checked in RenderObject::decorationColor).
    if (colorProperty == CSSPropertyWebkitTextDecorationColor)
        return visitedColor;
#endif // CSS3_TEXT

    // FIXME: Technically someone could explicitly specify the color transparent, but for now we'll just
    // assume that if the background color is transparent that it wasn't set. Note that it's weird that
    // we're returning unvisited info for a visited link, but given our restriction that the alpha values
    // have to match, it makes more sense to return the unvisited background color if specified than it
    // does to return black. This behavior matches what Firefox 4 does as well.
    if (colorProperty == CSSPropertyBackgroundColor && visitedColor == Color::transparent)
        return unvisitedColor;

    // Take the alpha from the unvisited color, but get the RGB values from the visited color.
    return Color(visitedColor.red(), visitedColor.green(), visitedColor.blue(), unvisitedColor.alpha());
}

const BorderValue& RenderStyle::borderBefore() const
{
    switch (writingMode()) {
    case TopToBottomWritingMode:
        return borderTop();
    case BottomToTopWritingMode:
        return borderBottom();
    case LeftToRightWritingMode:
        return borderLeft();
    case RightToLeftWritingMode:
        return borderRight();
    }
    ASSERT_NOT_REACHED();
    return borderTop();
}

const BorderValue& RenderStyle::borderAfter() const
{
    switch (writingMode()) {
    case TopToBottomWritingMode:
        return borderBottom();
    case BottomToTopWritingMode:
        return borderTop();
    case LeftToRightWritingMode:
        return borderRight();
    case RightToLeftWritingMode:
        return borderLeft();
    }
    ASSERT_NOT_REACHED();
    return borderBottom();
}

const BorderValue& RenderStyle::borderStart() const
{
    if (isHorizontalWritingMode())
        return isLeftToRightDirection() ? borderLeft() : borderRight();
    return isLeftToRightDirection() ? borderTop() : borderBottom();
}

const BorderValue& RenderStyle::borderEnd() const
{
    if (isHorizontalWritingMode())
        return isLeftToRightDirection() ? borderRight() : borderLeft();
    return isLeftToRightDirection() ? borderBottom() : borderTop();
}

unsigned short RenderStyle::borderBeforeWidth() const
{
    switch (writingMode()) {
    case TopToBottomWritingMode:
        return borderTopWidth();
    case BottomToTopWritingMode:
        return borderBottomWidth();
    case LeftToRightWritingMode:
        return borderLeftWidth();
    case RightToLeftWritingMode:
        return borderRightWidth();
    }
    ASSERT_NOT_REACHED();
    return borderTopWidth();
}

unsigned short RenderStyle::borderAfterWidth() const
{
    switch (writingMode()) {
    case TopToBottomWritingMode:
        return borderBottomWidth();
    case BottomToTopWritingMode:
        return borderTopWidth();
    case LeftToRightWritingMode:
        return borderRightWidth();
    case RightToLeftWritingMode:
        return borderLeftWidth();
    }
    ASSERT_NOT_REACHED();
    return borderBottomWidth();
}

unsigned short RenderStyle::borderStartWidth() const
{
    if (isHorizontalWritingMode())
        return isLeftToRightDirection() ? borderLeftWidth() : borderRightWidth();
    return isLeftToRightDirection() ? borderTopWidth() : borderBottomWidth();
}

unsigned short RenderStyle::borderEndWidth() const
{
    if (isHorizontalWritingMode())
        return isLeftToRightDirection() ? borderRightWidth() : borderLeftWidth();
    return isLeftToRightDirection() ? borderBottomWidth() : borderTopWidth();
}

void RenderStyle::setMarginStart(Length margin)
{
    if (isHorizontalWritingMode()) {
        if (isLeftToRightDirection())
            setMarginLeft(margin);
        else
            setMarginRight(margin);
    } else {
        if (isLeftToRightDirection())
            setMarginTop(margin);
        else
            setMarginBottom(margin);
    }
}

void RenderStyle::setMarginEnd(Length margin)
{
    if (isHorizontalWritingMode()) {
        if (isLeftToRightDirection())
            setMarginRight(margin);
        else
            setMarginLeft(margin);
    } else {
        if (isLeftToRightDirection())
            setMarginBottom(margin);
        else
            setMarginTop(margin);
    }
}

TextEmphasisMark RenderStyle::textEmphasisMark() const
{
    TextEmphasisMark mark = static_cast<TextEmphasisMark>(rareInheritedData->textEmphasisMark);
    if (mark != TextEmphasisMarkAuto)
        return mark;

    if (isHorizontalWritingMode())
        return TextEmphasisMarkDot;

    return TextEmphasisMarkSesame;
}

#if ENABLE(TOUCH_EVENTS)
Color RenderStyle::initialTapHighlightColor()
{
    return RenderTheme::tapHighlightColor();
}
#endif

LayoutBoxExtent RenderStyle::imageOutsets(const NinePieceImage& image) const
{
    return LayoutBoxExtent(NinePieceImage::computeOutset(image.outset().top(), borderTopWidth()),
                           NinePieceImage::computeOutset(image.outset().right(), borderRightWidth()),
                           NinePieceImage::computeOutset(image.outset().bottom(), borderBottomWidth()),
                           NinePieceImage::computeOutset(image.outset().left(), borderLeftWidth()));
}

void RenderStyle::setBorderImageSource(PassRefPtr<StyleImage> image)
{
    if (surround->border.m_image.image() == image.get())
        return;
    surround.access()->border.m_image.setImage(image);
}

void RenderStyle::setBorderImageSlices(LengthBox slices)
{
    if (surround->border.m_image.imageSlices() == slices)
        return;
    surround.access()->border.m_image.setImageSlices(slices);
}

void RenderStyle::setBorderImageWidth(LengthBox slices)
{
    if (surround->border.m_image.borderSlices() == slices)
        return;
    surround.access()->border.m_image.setBorderSlices(slices);
}

void RenderStyle::setBorderImageOutset(LengthBox outset)
{
    if (surround->border.m_image.outset() == outset)
        return;
    surround.access()->border.m_image.setOutset(outset);
}

ShapeValue* RenderStyle::initialShapeInside()
{
    DEFINE_STATIC_LOCAL(RefPtr<ShapeValue>, sOutsideValue, (ShapeValue::createOutsideValue()));
    return sOutsideValue.get();
}

void RenderStyle::setColumnStylesFromPaginationMode(const Pagination::Mode& paginationMode)
{
    if (paginationMode == Pagination::Unpaginated)
        return;
        
    switch (paginationMode) {
    case Pagination::LeftToRightPaginated:
        setColumnAxis(HorizontalColumnAxis);
        if (isHorizontalWritingMode())
            setColumnProgression(isLeftToRightDirection() ? NormalColumnProgression : ReverseColumnProgression);
        else
            setColumnProgression(isFlippedBlocksWritingMode() ? ReverseColumnProgression : NormalColumnProgression);
        break;
    case Pagination::RightToLeftPaginated:
        setColumnAxis(HorizontalColumnAxis);
        if (isHorizontalWritingMode())
            setColumnProgression(isLeftToRightDirection() ? ReverseColumnProgression : NormalColumnProgression);
        else
            setColumnProgression(isFlippedBlocksWritingMode() ? NormalColumnProgression : ReverseColumnProgression);
        break;
    case Pagination::TopToBottomPaginated:
        setColumnAxis(VerticalColumnAxis);
        if (isHorizontalWritingMode())
            setColumnProgression(isFlippedBlocksWritingMode() ? ReverseColumnProgression : NormalColumnProgression);
        else
            setColumnProgression(isLeftToRightDirection() ? NormalColumnProgression : ReverseColumnProgression);
        break;
    case Pagination::BottomToTopPaginated:
        setColumnAxis(VerticalColumnAxis);
        if (isHorizontalWritingMode())
            setColumnProgression(isFlippedBlocksWritingMode() ? NormalColumnProgression : ReverseColumnProgression);
        else
            setColumnProgression(isLeftToRightDirection() ? ReverseColumnProgression : NormalColumnProgression);
        break;
    case Pagination::Unpaginated:
        ASSERT_NOT_REACHED();
        break;
    }
}

} // namespace WebCore
