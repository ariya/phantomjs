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

#include "BasicShapes.h"
#include "ClipPathOperation.h"
#include "CounterDirectives.h"
#include "CursorData.h"
#include "DataRef.h"
#include "FillLayer.h"
#include "LineClampValue.h"
#include "NinePieceImage.h"
#include "ShapeValue.h"
#include <wtf/OwnPtr.h>
#include <wtf/PassRefPtr.h>
#include <wtf/Vector.h>

namespace WebCore {

class AnimationList;
class ShadowData;
class StyleDeprecatedFlexibleBoxData;
#if ENABLE(CSS_FILTERS)
class StyleFilterData;
#endif
class StyleFlexibleBoxData;
class StyleGridData;
class StyleGridItemData;
class StyleMarqueeData;
class StyleMultiColData;
class StyleReflection;
class StyleResolver;
class StyleTransformData;

class ContentData;
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

    bool contentDataEquivalent(const StyleRareNonInheritedData&) const;
    bool counterDataEquivalent(const StyleRareNonInheritedData&) const;
    bool shadowDataEquivalent(const StyleRareNonInheritedData&) const;
    bool reflectionDataEquivalent(const StyleRareNonInheritedData&) const;
    bool animationDataEquivalent(const StyleRareNonInheritedData&) const;
    bool transitionDataEquivalent(const StyleRareNonInheritedData&) const;

    float opacity; // Whether or not we're transparent.

    float m_aspectRatioDenominator;
    float m_aspectRatioNumerator;

    float m_perspective;
    Length m_perspectiveOriginX;
    Length m_perspectiveOriginY;

    LineClampValue lineClamp; // An Apple extension.
#if ENABLE(DASHBOARD_SUPPORT)
    Vector<StyleDashboardRegion> m_dashboardRegions;
#endif
#if ENABLE(DRAGGABLE_REGION)
    DraggableRegionMode m_draggableRegionMode;
#endif

    DataRef<StyleDeprecatedFlexibleBoxData> m_deprecatedFlexibleBox; // Flexible box properties
    DataRef<StyleFlexibleBoxData> m_flexibleBox;
    DataRef<StyleMarqueeData> m_marquee; // Marquee properties
    DataRef<StyleMultiColData> m_multiCol; //  CSS3 multicol properties
    DataRef<StyleTransformData> m_transform; // Transform properties (rotate, scale, skew, etc.)

#if ENABLE(CSS_FILTERS)
    DataRef<StyleFilterData> m_filter; // Filter operations (url, sepia, blur, etc.)
#endif

    DataRef<StyleGridData> m_grid;
    DataRef<StyleGridItemData> m_gridItem;

    OwnPtr<ContentData> m_content;
    OwnPtr<CounterDirectiveMap> m_counterDirectives;

    OwnPtr<ShadowData> m_boxShadow;  // For box-shadow decorations.
    
    RefPtr<StyleReflection> m_boxReflect;

    OwnPtr<AnimationList> m_animations;
    OwnPtr<AnimationList> m_transitions;

    FillLayer m_mask;
    NinePieceImage m_maskBoxImage;

    LengthSize m_pageSize;

    RefPtr<ShapeValue> m_shapeInside;
    RefPtr<ShapeValue> m_shapeOutside;
    Length m_shapeMargin;
    Length m_shapePadding;

    RefPtr<ClipPathOperation> m_clipPath;

#if ENABLE(CSS3_TEXT)
    Color m_textDecorationColor;
    Color m_visitedLinkTextDecorationColor;
#endif // CSS3_TEXT
    Color m_visitedLinkBackgroundColor;
    Color m_visitedLinkOutlineColor;
    Color m_visitedLinkBorderLeftColor;
    Color m_visitedLinkBorderRightColor;
    Color m_visitedLinkBorderTopColor;
    Color m_visitedLinkBorderBottomColor;

    int m_order;

    AtomicString m_flowThread;
    AtomicString m_regionThread;
    unsigned m_regionFragment : 1; // RegionFragment

    unsigned m_regionBreakAfter : 2; // EPageBreak
    unsigned m_regionBreakBefore : 2; // EPageBreak
    unsigned m_regionBreakInside : 2; // EPageBreak

    unsigned m_pageSizeType : 2; // PageSizeType
    unsigned m_transformStyle3D : 1; // ETransformStyle3D
    unsigned m_backfaceVisibility : 1; // EBackfaceVisibility

    unsigned m_alignContent : 3; // EAlignContent
    unsigned m_alignItems : 3; // EAlignItems
    unsigned m_alignSelf : 3; // EAlignItems
    unsigned m_justifyContent : 3; // EJustifyContent

    unsigned userDrag : 2; // EUserDrag
    unsigned textOverflow : 1; // Whether or not lines that spill out should be truncated with "..."
    unsigned marginBeforeCollapse : 2; // EMarginCollapse
    unsigned marginAfterCollapse : 2; // EMarginCollapse
    unsigned m_appearance : 6; // EAppearance
    unsigned m_borderFit : 1; // EBorderFit
    unsigned m_textCombine : 1; // CSS3 text-combine properties

#if ENABLE(CSS3_TEXT)
    unsigned m_textDecorationStyle : 3; // TextDecorationStyle
#endif // CSS3_TEXT
    unsigned m_wrapFlow: 3; // WrapFlow
    unsigned m_wrapThrough: 1; // WrapThrough

#if USE(ACCELERATED_COMPOSITING)
    unsigned m_runningAcceleratedAnimation : 1;
#endif

    unsigned m_hasAspectRatio : 1; // Whether or not an aspect ratio has been specified.

#if ENABLE(CSS_COMPOSITING)
    unsigned m_effectiveBlendMode: 5; // EBlendMode
#endif

private:
    StyleRareNonInheritedData();
    StyleRareNonInheritedData(const StyleRareNonInheritedData&);
};

} // namespace WebCore

#endif // StyleRareNonInheritedData_h
