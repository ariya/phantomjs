/*
 * (C) 1999-2003 Lars Knoll (knoll@kde.org)
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
 */

#include "config.h"
#include "CSSPropertyLonghand.h"

#include "CSSPropertyNames.h"
#include <wtf/HashMap.h>
#include <wtf/StdLibExtras.h>

namespace WebCore {

typedef HashMap<int, CSSPropertyLonghand> ShorthandMap;

static void initShorthandMap(ShorthandMap& shorthandMap)
{
    #define SET_SHORTHAND_MAP_ENTRY(map, propID, array) \
        map.set(propID, CSSPropertyLonghand(array, WTF_ARRAY_LENGTH(array)))

    // FIXME: The 'font' property has "shorthand nature" but is not parsed as a shorthand.

    // Do not change the order of the following four shorthands, and keep them together.
    static const int borderProperties[4][3] = {
        { CSSPropertyBorderTopColor, CSSPropertyBorderTopStyle, CSSPropertyBorderTopWidth },
        { CSSPropertyBorderRightColor, CSSPropertyBorderRightStyle, CSSPropertyBorderRightWidth },
        { CSSPropertyBorderBottomColor, CSSPropertyBorderBottomStyle, CSSPropertyBorderBottomWidth },
        { CSSPropertyBorderLeftColor, CSSPropertyBorderLeftStyle, CSSPropertyBorderLeftWidth }
    };
    SET_SHORTHAND_MAP_ENTRY(shorthandMap, CSSPropertyBorderTop, borderProperties[0]);
    SET_SHORTHAND_MAP_ENTRY(shorthandMap, CSSPropertyBorderRight, borderProperties[1]);
    SET_SHORTHAND_MAP_ENTRY(shorthandMap, CSSPropertyBorderBottom, borderProperties[2]);
    SET_SHORTHAND_MAP_ENTRY(shorthandMap, CSSPropertyBorderLeft, borderProperties[3]);

    shorthandMap.set(CSSPropertyBorder, CSSPropertyLonghand(borderProperties[0], sizeof(borderProperties) / sizeof(borderProperties[0][0])));

    static const int borderColorProperties[] = {
        CSSPropertyBorderTopColor,
        CSSPropertyBorderRightColor,
        CSSPropertyBorderBottomColor,
        CSSPropertyBorderLeftColor
    };
    SET_SHORTHAND_MAP_ENTRY(shorthandMap, CSSPropertyBorderColor, borderColorProperties);

    static const int borderStyleProperties[] = {
        CSSPropertyBorderTopStyle,
        CSSPropertyBorderRightStyle,
        CSSPropertyBorderBottomStyle,
        CSSPropertyBorderLeftStyle
    };
    SET_SHORTHAND_MAP_ENTRY(shorthandMap, CSSPropertyBorderStyle, borderStyleProperties);

    static const int borderWidthProperties[] = {
        CSSPropertyBorderTopWidth,
        CSSPropertyBorderRightWidth,
        CSSPropertyBorderBottomWidth,
        CSSPropertyBorderLeftWidth
    };
    SET_SHORTHAND_MAP_ENTRY(shorthandMap, CSSPropertyBorderWidth, borderWidthProperties);

    static const int backgroundPositionProperties[] = { CSSPropertyBackgroundPositionX, CSSPropertyBackgroundPositionY };
    SET_SHORTHAND_MAP_ENTRY(shorthandMap, CSSPropertyBackgroundPosition, backgroundPositionProperties);

    static const int backgroundRepeatProperties[] = { CSSPropertyBackgroundRepeatX, CSSPropertyBackgroundRepeatY };
    SET_SHORTHAND_MAP_ENTRY(shorthandMap, CSSPropertyBackgroundRepeat, backgroundRepeatProperties);

    static const int borderSpacingProperties[] = { CSSPropertyWebkitBorderHorizontalSpacing, CSSPropertyWebkitBorderVerticalSpacing };
    SET_SHORTHAND_MAP_ENTRY(shorthandMap, CSSPropertyBorderSpacing, borderSpacingProperties);

    static const int listStyleProperties[] = {
        CSSPropertyListStyleImage,
        CSSPropertyListStylePosition,
        CSSPropertyListStyleType
    };
    SET_SHORTHAND_MAP_ENTRY(shorthandMap, CSSPropertyListStyle, listStyleProperties);

    static const int marginProperties[] = {
        CSSPropertyMarginTop,
        CSSPropertyMarginRight,
        CSSPropertyMarginBottom,
        CSSPropertyMarginLeft
    };
    SET_SHORTHAND_MAP_ENTRY(shorthandMap, CSSPropertyMargin, marginProperties);

    static const int marginCollapseProperties[] = { CSSPropertyWebkitMarginBeforeCollapse, CSSPropertyWebkitMarginAfterCollapse };
    SET_SHORTHAND_MAP_ENTRY(shorthandMap, CSSPropertyWebkitMarginCollapse, marginCollapseProperties);

    static const int marqueeProperties[] = {
        CSSPropertyWebkitMarqueeDirection,
        CSSPropertyWebkitMarqueeIncrement,
        CSSPropertyWebkitMarqueeRepetition,
        CSSPropertyWebkitMarqueeStyle,
        CSSPropertyWebkitMarqueeSpeed
    };
    SET_SHORTHAND_MAP_ENTRY(shorthandMap, CSSPropertyWebkitMarquee, marqueeProperties);

    static const int outlineProperties[] = {
        CSSPropertyOutlineColor,
        CSSPropertyOutlineOffset,
        CSSPropertyOutlineStyle,
        CSSPropertyOutlineWidth
    };
    SET_SHORTHAND_MAP_ENTRY(shorthandMap, CSSPropertyOutline, outlineProperties);

    static const int paddingProperties[] = {
        CSSPropertyPaddingTop,
        CSSPropertyPaddingRight,
        CSSPropertyPaddingBottom,
        CSSPropertyPaddingLeft
    };
    SET_SHORTHAND_MAP_ENTRY(shorthandMap, CSSPropertyPadding, paddingProperties);

    static const int textStrokeProperties[] = { CSSPropertyWebkitTextStrokeColor, CSSPropertyWebkitTextStrokeWidth };
    SET_SHORTHAND_MAP_ENTRY(shorthandMap, CSSPropertyWebkitTextStroke, textStrokeProperties);

    static const int backgroundProperties[] = {
        CSSPropertyBackgroundAttachment,
        CSSPropertyBackgroundClip,
        CSSPropertyBackgroundColor,
        CSSPropertyBackgroundImage,
        CSSPropertyBackgroundOrigin,
        CSSPropertyBackgroundPositionX,
        CSSPropertyBackgroundPositionY,
        CSSPropertyBackgroundRepeatX,
        CSSPropertyBackgroundRepeatY
    };
    SET_SHORTHAND_MAP_ENTRY(shorthandMap, CSSPropertyBackground, backgroundProperties);

    static const int columnsProperties[] = { CSSPropertyWebkitColumnWidth, CSSPropertyWebkitColumnCount };
    SET_SHORTHAND_MAP_ENTRY(shorthandMap, CSSPropertyWebkitColumns, columnsProperties);

    static const int columnRuleProperties[] = {
        CSSPropertyWebkitColumnRuleColor,
        CSSPropertyWebkitColumnRuleStyle,
        CSSPropertyWebkitColumnRuleWidth
    };
    SET_SHORTHAND_MAP_ENTRY(shorthandMap, CSSPropertyWebkitColumnRule, columnRuleProperties);

    static const int overflowProperties[] = { CSSPropertyOverflowX, CSSPropertyOverflowY };
    SET_SHORTHAND_MAP_ENTRY(shorthandMap, CSSPropertyOverflow, overflowProperties);

    static const int borderRadiusProperties[] = {
        CSSPropertyBorderTopRightRadius,
        CSSPropertyBorderTopLeftRadius,
        CSSPropertyBorderBottomLeftRadius,
        CSSPropertyBorderBottomRightRadius
    };
    SET_SHORTHAND_MAP_ENTRY(shorthandMap, CSSPropertyBorderRadius, borderRadiusProperties);
    SET_SHORTHAND_MAP_ENTRY(shorthandMap, CSSPropertyWebkitBorderRadius, borderRadiusProperties);

    static const int maskPositionProperties[] = { CSSPropertyWebkitMaskPositionX, CSSPropertyWebkitMaskPositionY };
    SET_SHORTHAND_MAP_ENTRY(shorthandMap, CSSPropertyWebkitMaskPosition, maskPositionProperties);

    static const int maskRepeatProperties[] = { CSSPropertyWebkitMaskRepeatX, CSSPropertyWebkitMaskRepeatY };
    SET_SHORTHAND_MAP_ENTRY(shorthandMap, CSSPropertyWebkitMaskRepeat, maskRepeatProperties);

    static const int maskProperties[] = {
        CSSPropertyWebkitMaskAttachment,
        CSSPropertyWebkitMaskClip,
        CSSPropertyWebkitMaskImage,
        CSSPropertyWebkitMaskOrigin,
        CSSPropertyWebkitMaskPositionX,
        CSSPropertyWebkitMaskPositionY,
        CSSPropertyWebkitMaskRepeatX,
        CSSPropertyWebkitMaskRepeatY
    };
    SET_SHORTHAND_MAP_ENTRY(shorthandMap, CSSPropertyWebkitMask, maskProperties);

    static const int animationProperties[] = {
        CSSPropertyWebkitAnimationName,
        CSSPropertyWebkitAnimationDuration,
        CSSPropertyWebkitAnimationTimingFunction,
        CSSPropertyWebkitAnimationDelay,
        CSSPropertyWebkitAnimationIterationCount,
        CSSPropertyWebkitAnimationDirection,
        CSSPropertyWebkitAnimationFillMode
    };
    SET_SHORTHAND_MAP_ENTRY(shorthandMap, CSSPropertyWebkitAnimation, animationProperties);

    static const int transitionProperties[] = {
        CSSPropertyWebkitTransitionProperty,
        CSSPropertyWebkitTransitionDuration,
        CSSPropertyWebkitTransitionTimingFunction,
        CSSPropertyWebkitTransitionDelay
    };
    SET_SHORTHAND_MAP_ENTRY(shorthandMap, CSSPropertyWebkitTransition, transitionProperties);

    static const int transformOriginProperties[] = {
        CSSPropertyWebkitTransformOriginX,
        CSSPropertyWebkitTransformOriginY
    };
    SET_SHORTHAND_MAP_ENTRY(shorthandMap, CSSPropertyWebkitTransformOrigin, transformOriginProperties);
    
    static const int textEmphasisProperties[] = {
        CSSPropertyWebkitTextEmphasisColor,
        CSSPropertyWebkitTextEmphasisStyle
    };
    SET_SHORTHAND_MAP_ENTRY(shorthandMap, CSSPropertyWebkitTextEmphasis, textEmphasisProperties);
    
    #undef SET_SHORTHAND_MAP_ENTRY
}

CSSPropertyLonghand longhandForProperty(int propertyID)
{
    DEFINE_STATIC_LOCAL(ShorthandMap, shorthandMap, ());
    if (shorthandMap.isEmpty())
        initShorthandMap(shorthandMap);

    return shorthandMap.get(propertyID);
}


} // namespace WebCore
