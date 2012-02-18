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

#ifndef StyleRareNonInheritedData_h
#define StyleRareNonInheritedData_h

#include "CounterDirectives.h"
#include "CursorData.h"
#include "DataRef.h"
#include "FillLayer.h"
#include "LineClampValue.h"
#include "NinePieceImage.h"
#include "StyleTransformData.h"
#include <wtf/OwnPtr.h>
#include <wtf/PassRefPtr.h>
#include <wtf/Vector.h>

namespace WebCore {

class AnimationList;
class CSSStyleSelector;
class ShadowData;
class StyleFlexibleBoxData;
class StyleMarqueeData;
class StyleMultiColData;
class StyleReflection;
class StyleTransformData;

struct ContentData;
struct LengthSize;

#if ENABLE(DASHBOARD_SUPPORT)
struct StyleDashboardRegion;
#endif

// Page size type.
// StyleRareNonInheritedData::m_pageSize is meaningful only when 
// StyleRareNonInheritedData::m_pageSizeType is PAGE_SIZE_RESOLVED.
enum PageSizeType {
    PAGE_SIZE_AUTO, // size: auto
    PAGE_SIZE_AUTO_LANDSCAPE, // size: landscape
    PAGE_SIZE_AUTO_PORTRAIT, // size: portrait
    PAGE_SIZE_RESOLVED // Size is fully resolved.
};

// This struct is for rarely used non-inherited CSS3, CSS2, and WebKit-specific properties.
// By grouping them together, we save space, and only allocate this object when someone
// actually uses one of these properties.
class StyleRareNonInheritedData : public RefCounted<StyleRareNonInheritedData> {
public:
    static PassRefPtr<StyleRareNonInheritedData> create() { return adoptRef(new StyleRareNonInheritedData); }
    PassRefPtr<StyleRareNonInheritedData> copy() const { return adoptRef(new StyleRareNonInheritedData(*this)); }
    ~StyleRareNonInheritedData();
    
    bool operator==(const StyleRareNonInheritedData&) const;
    bool operator!=(const StyleRareNonInheritedData& o) const { return !(*this == o); }

    bool contentDataEquivalent(const StyleRareNonInheritedData& o) const;
    bool shadowDataEquivalent(const StyleRareNonInheritedData& o) const;
    bool reflectionDataEquivalent(const StyleRareNonInheritedData& o) const;
    bool animationDataEquivalent(const StyleRareNonInheritedData&) const;
    bool transitionDataEquivalent(const StyleRareNonInheritedData&) const;

    LineClampValue lineClamp; // An Apple extension.
#if ENABLE(DASHBOARD_SUPPORT)
    Vector<StyleDashboardRegion> m_dashboardRegions;
#endif
    float opacity; // Whether or not we're transparent.

    DataRef<StyleFlexibleBoxData> flexibleBox; // Flexible box properties 
    DataRef<StyleMarqueeData> marquee; // Marquee properties
    DataRef<StyleMultiColData> m_multiCol; //  CSS3 multicol properties
    DataRef<StyleTransformData> m_transform; // Transform properties (rotate, scale, skew, etc.)

    OwnPtr<ContentData> m_content;
    OwnPtr<CounterDirectiveMap> m_counterDirectives;

    unsigned userDrag : 2; // EUserDrag
    bool textOverflow : 1; // Whether or not lines that spill out should be truncated with "..."
    unsigned marginBeforeCollapse : 2; // EMarginCollapse
    unsigned marginAfterCollapse : 2; // EMarginCollapse
    unsigned matchNearestMailBlockquoteColor : 1; // EMatchNearestMailBlockquoteColor, FIXME: This property needs to be eliminated. It should never have been added.
    unsigned m_appearance : 6; // EAppearance
    unsigned m_borderFit : 1; // EBorderFit
    unsigned m_textCombine : 1; // CSS3 text-combine properties
    
    short m_counterIncrement;
    short m_counterReset;

#if USE(ACCELERATED_COMPOSITING)
    bool m_runningAcceleratedAnimation : 1;
#endif
    OwnPtr<ShadowData> m_boxShadow;  // For box-shadow decorations.
    
    RefPtr<StyleReflection> m_boxReflect;

    OwnPtr<AnimationList> m_animations;
    OwnPtr<AnimationList> m_transitions;

    FillLayer m_mask;
    NinePieceImage m_maskBoxImage;

    ETransformStyle3D m_transformStyle3D;
    EBackfaceVisibility m_backfaceVisibility;
    float m_perspective;
    Length m_perspectiveOriginX;
    Length m_perspectiveOriginY;

    LengthSize m_pageSize;
    PageSizeType m_pageSizeType;

private:
    StyleRareNonInheritedData();
    StyleRareNonInheritedData(const StyleRareNonInheritedData&);
};

} // namespace WebCore

#endif // StyleRareNonInheritedData_h
