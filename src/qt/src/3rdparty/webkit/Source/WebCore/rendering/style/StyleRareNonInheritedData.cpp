/*
 * Copyright (C) 1999 Antti Koivisto (koivisto@kde.org)
 * Copyright (C) 2004, 2005, 2006, 2007, 2008 Apple Inc. All rights reserved.
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
#include "StyleRareNonInheritedData.h"

#include "CSSStyleSelector.h"
#include "ContentData.h"
#include "RenderCounter.h"
#include "RenderStyle.h"
#include "ShadowData.h"
#include "StyleImage.h"

namespace WebCore {

StyleRareNonInheritedData::StyleRareNonInheritedData()
    : lineClamp(RenderStyle::initialLineClamp())
    , opacity(RenderStyle::initialOpacity())
    , userDrag(RenderStyle::initialUserDrag())
    , textOverflow(RenderStyle::initialTextOverflow())
    , marginBeforeCollapse(MCOLLAPSE)
    , marginAfterCollapse(MCOLLAPSE)
    , matchNearestMailBlockquoteColor(RenderStyle::initialMatchNearestMailBlockquoteColor())
    , m_appearance(RenderStyle::initialAppearance())
    , m_borderFit(RenderStyle::initialBorderFit())
    , m_textCombine(RenderStyle::initialTextCombine())
    , m_counterIncrement(0)
    , m_counterReset(0)
#if USE(ACCELERATED_COMPOSITING)
    , m_runningAcceleratedAnimation(false)
#endif
    , m_mask(FillLayer(MaskFillLayer))
    , m_transformStyle3D(RenderStyle::initialTransformStyle3D())
    , m_backfaceVisibility(RenderStyle::initialBackfaceVisibility())
    , m_perspective(RenderStyle::initialPerspective())
    , m_perspectiveOriginX(RenderStyle::initialPerspectiveOriginX())
    , m_perspectiveOriginY(RenderStyle::initialPerspectiveOriginY())
    , m_pageSize()
    , m_pageSizeType(PAGE_SIZE_AUTO)
{
}

StyleRareNonInheritedData::StyleRareNonInheritedData(const StyleRareNonInheritedData& o)
    : RefCounted<StyleRareNonInheritedData>()
    , lineClamp(o.lineClamp)
    , opacity(o.opacity)
    , flexibleBox(o.flexibleBox)
    , marquee(o.marquee)
    , m_multiCol(o.m_multiCol)
    , m_transform(o.m_transform)
    , userDrag(o.userDrag)
    , textOverflow(o.textOverflow)
    , marginBeforeCollapse(o.marginBeforeCollapse)
    , marginAfterCollapse(o.marginAfterCollapse)
    , matchNearestMailBlockquoteColor(o.matchNearestMailBlockquoteColor)
    , m_appearance(o.m_appearance)
    , m_borderFit(o.m_borderFit)
    , m_textCombine(o.m_textCombine)
    , m_counterIncrement(o.m_counterIncrement)
    , m_counterReset(o.m_counterReset)
#if USE(ACCELERATED_COMPOSITING)
    , m_runningAcceleratedAnimation(o.m_runningAcceleratedAnimation)
#endif
    , m_boxShadow(o.m_boxShadow ? adoptPtr(new ShadowData(*o.m_boxShadow)) : nullptr)
    , m_boxReflect(o.m_boxReflect)
    , m_animations(o.m_animations ? adoptPtr(new AnimationList(*o.m_animations)) : nullptr)
    , m_transitions(o.m_transitions ? adoptPtr(new AnimationList(*o.m_transitions)) : nullptr)
    , m_mask(o.m_mask)
    , m_maskBoxImage(o.m_maskBoxImage)
    , m_transformStyle3D(o.m_transformStyle3D)
    , m_backfaceVisibility(o.m_backfaceVisibility)
    , m_perspective(o.m_perspective)
    , m_perspectiveOriginX(o.m_perspectiveOriginX)
    , m_perspectiveOriginY(o.m_perspectiveOriginY)
    , m_pageSize(o.m_pageSize)
    , m_pageSizeType(o.m_pageSizeType)
{
}

StyleRareNonInheritedData::~StyleRareNonInheritedData()
{
}

bool StyleRareNonInheritedData::operator==(const StyleRareNonInheritedData& o) const
{
    return lineClamp == o.lineClamp
#if ENABLE(DASHBOARD_SUPPORT)
        && m_dashboardRegions == o.m_dashboardRegions
#endif
        && opacity == o.opacity
        && flexibleBox == o.flexibleBox
        && marquee == o.marquee
        && m_multiCol == o.m_multiCol
        && m_transform == o.m_transform
        && contentDataEquivalent(o)
        && m_counterDirectives == o.m_counterDirectives
        && userDrag == o.userDrag
        && textOverflow == o.textOverflow
        && marginBeforeCollapse == o.marginBeforeCollapse
        && marginAfterCollapse == o.marginAfterCollapse
        && matchNearestMailBlockquoteColor == o.matchNearestMailBlockquoteColor
        && m_appearance == o.m_appearance
        && m_borderFit == o.m_borderFit
        && m_textCombine == o.m_textCombine
        && m_counterIncrement == o.m_counterIncrement
        && m_counterReset == o.m_counterReset
#if USE(ACCELERATED_COMPOSITING)
        && !m_runningAcceleratedAnimation && !o.m_runningAcceleratedAnimation
#endif
        && shadowDataEquivalent(o)
        && reflectionDataEquivalent(o)
        && animationDataEquivalent(o)
        && transitionDataEquivalent(o)
        && m_mask == o.m_mask
        && m_maskBoxImage == o.m_maskBoxImage
        && (m_transformStyle3D == o.m_transformStyle3D)
        && (m_backfaceVisibility == o.m_backfaceVisibility)
        && (m_perspective == o.m_perspective)
        && (m_perspectiveOriginX == o.m_perspectiveOriginX)
        && (m_perspectiveOriginY == o.m_perspectiveOriginY)
        && (m_pageSize == o.m_pageSize)
        && (m_pageSizeType == o.m_pageSizeType)
        ;
}

bool StyleRareNonInheritedData::contentDataEquivalent(const StyleRareNonInheritedData& o) const
{
    ContentData* c1 = m_content.get();
    ContentData* c2 = o.m_content.get();

    while (c1 && c2) {
        if (!c1->dataEquivalent(*c2))
            return false;
        c1 = c1->next();
        c2 = c2->next();
    }

    return !c1 && !c2;
}

bool StyleRareNonInheritedData::shadowDataEquivalent(const StyleRareNonInheritedData& o) const
{
    if ((!m_boxShadow && o.m_boxShadow) || (m_boxShadow && !o.m_boxShadow))
        return false;
    if (m_boxShadow && o.m_boxShadow && (*m_boxShadow != *o.m_boxShadow))
        return false;
    return true;
}

bool StyleRareNonInheritedData::reflectionDataEquivalent(const StyleRareNonInheritedData& o) const
{
    if (m_boxReflect != o.m_boxReflect) {
        if (!m_boxReflect || !o.m_boxReflect)
            return false;
        return *m_boxReflect == *o.m_boxReflect;
    }
    return true;

}

bool StyleRareNonInheritedData::animationDataEquivalent(const StyleRareNonInheritedData& o) const
{
    if ((!m_animations && o.m_animations) || (m_animations && !o.m_animations))
        return false;
    if (m_animations && o.m_animations && (*m_animations != *o.m_animations))
        return false;
    return true;
}

bool StyleRareNonInheritedData::transitionDataEquivalent(const StyleRareNonInheritedData& o) const
{
    if ((!m_transitions && o.m_transitions) || (m_transitions && !o.m_transitions))
        return false;
    if (m_transitions && o.m_transitions && (*m_transitions != *o.m_transitions))
        return false;
    return true;
}

} // namespace WebCore
