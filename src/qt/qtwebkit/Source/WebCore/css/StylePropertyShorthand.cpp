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
#include "StylePropertyShorthand.h"

#include <wtf/StdLibExtras.h>

namespace WebCore {

const StylePropertyShorthand& backgroundShorthand()
{
    static const CSSPropertyID backgroundProperties[] = {
        CSSPropertyBackgroundImage,
        CSSPropertyBackgroundPositionX,
        CSSPropertyBackgroundPositionY,
        CSSPropertyBackgroundSize,
        CSSPropertyBackgroundRepeatX,
        CSSPropertyBackgroundRepeatY,
        CSSPropertyBackgroundAttachment,
        CSSPropertyBackgroundOrigin,
        CSSPropertyBackgroundClip,
        CSSPropertyBackgroundColor
    };
    DEFINE_STATIC_LOCAL(StylePropertyShorthand, backgroundShorthand, (backgroundProperties, WTF_ARRAY_LENGTH(backgroundProperties)));
    return backgroundShorthand;
}

const StylePropertyShorthand& backgroundPositionShorthand()
{
    static const CSSPropertyID backgroundPositionProperties[] = { CSSPropertyBackgroundPositionX, CSSPropertyBackgroundPositionY };
    DEFINE_STATIC_LOCAL(StylePropertyShorthand, backgroundPositionLonghands, (backgroundPositionProperties, WTF_ARRAY_LENGTH(backgroundPositionProperties)));
    return backgroundPositionLonghands;
}

const StylePropertyShorthand& backgroundRepeatShorthand()
{
    static const CSSPropertyID backgroundRepeatProperties[] = { CSSPropertyBackgroundRepeatX, CSSPropertyBackgroundRepeatY };
    DEFINE_STATIC_LOCAL(StylePropertyShorthand, backgroundRepeatLonghands, (backgroundRepeatProperties, WTF_ARRAY_LENGTH(backgroundRepeatProperties)));
    return backgroundRepeatLonghands;
}

const StylePropertyShorthand& borderShorthand()
{
    // Do not change the order of the following four shorthands, and keep them together.
    static const CSSPropertyID borderProperties[4][3] = {
        { CSSPropertyBorderTopColor, CSSPropertyBorderTopStyle, CSSPropertyBorderTopWidth },
        { CSSPropertyBorderRightColor, CSSPropertyBorderRightStyle, CSSPropertyBorderRightWidth },
        { CSSPropertyBorderBottomColor, CSSPropertyBorderBottomStyle, CSSPropertyBorderBottomWidth },
        { CSSPropertyBorderLeftColor, CSSPropertyBorderLeftStyle, CSSPropertyBorderLeftWidth }
    };
    DEFINE_STATIC_LOCAL(StylePropertyShorthand, borderLonghands, (borderProperties[0], sizeof(borderProperties) / sizeof(borderProperties[0][0])));
    return borderLonghands;
}

const StylePropertyShorthand& borderAbridgedShorthand()
{
    static const CSSPropertyID borderAbridgedProperties[] = { CSSPropertyBorderWidth, CSSPropertyBorderStyle, CSSPropertyBorderColor };
    static const StylePropertyShorthand* propertiesForInitialization[] = {
        &borderWidthShorthand(),
        &borderStyleShorthand(),
        &borderColorShorthand(),
    };
    DEFINE_STATIC_LOCAL(StylePropertyShorthand, borderAbridgedLonghands,
        (borderAbridgedProperties, propertiesForInitialization, WTF_ARRAY_LENGTH(borderAbridgedProperties)));
    return borderAbridgedLonghands;
}

const StylePropertyShorthand& borderBottomShorthand()
{
    static const CSSPropertyID borderBottomProperties[] = { CSSPropertyBorderBottomWidth, CSSPropertyBorderBottomStyle, CSSPropertyBorderBottomColor };
    DEFINE_STATIC_LOCAL(StylePropertyShorthand, borderBottomLonghands, (borderBottomProperties, WTF_ARRAY_LENGTH(borderBottomProperties)));
    return borderBottomLonghands;
}

const StylePropertyShorthand& borderColorShorthand()
{
    static const CSSPropertyID borderColorProperties[] = {
        CSSPropertyBorderTopColor,
        CSSPropertyBorderRightColor,
        CSSPropertyBorderBottomColor,
        CSSPropertyBorderLeftColor
    };
    DEFINE_STATIC_LOCAL(StylePropertyShorthand, borderColorLonghands, (borderColorProperties, WTF_ARRAY_LENGTH(borderColorProperties)));
    return borderColorLonghands;
}

const StylePropertyShorthand& borderImageShorthand()
{
    static const CSSPropertyID borderImageProperties[] = {
        CSSPropertyBorderImageSource,
        CSSPropertyBorderImageSlice,
        CSSPropertyBorderImageWidth,
        CSSPropertyBorderImageOutset,
        CSSPropertyBorderImageRepeat
    };
    DEFINE_STATIC_LOCAL(StylePropertyShorthand, borderImageLonghands, (borderImageProperties, WTF_ARRAY_LENGTH(borderImageProperties)));
    return borderImageLonghands;
}

const StylePropertyShorthand& borderLeftShorthand()
{
    static const CSSPropertyID borderLeftProperties[] = { CSSPropertyBorderLeftWidth, CSSPropertyBorderLeftStyle, CSSPropertyBorderLeftColor };
    DEFINE_STATIC_LOCAL(StylePropertyShorthand, borderLeftLonghands, (borderLeftProperties, WTF_ARRAY_LENGTH(borderLeftProperties)));
    return borderLeftLonghands;
}

const StylePropertyShorthand& borderRadiusShorthand()
{
    static const CSSPropertyID borderRadiusProperties[] = {
        CSSPropertyBorderTopLeftRadius,
        CSSPropertyBorderTopRightRadius,
        CSSPropertyBorderBottomRightRadius,
        CSSPropertyBorderBottomLeftRadius
    };
    DEFINE_STATIC_LOCAL(StylePropertyShorthand, borderRadiusLonghands, (borderRadiusProperties, WTF_ARRAY_LENGTH(borderRadiusProperties)));
    return borderRadiusLonghands;
}

const StylePropertyShorthand& borderRightShorthand()
{
    static const CSSPropertyID borderRightProperties[] = { CSSPropertyBorderRightWidth, CSSPropertyBorderRightStyle, CSSPropertyBorderRightColor };
    DEFINE_STATIC_LOCAL(StylePropertyShorthand, borderRightLonghands, (borderRightProperties, WTF_ARRAY_LENGTH(borderRightProperties)));
    return borderRightLonghands;
}

const StylePropertyShorthand& borderSpacingShorthand()
{
    static const CSSPropertyID borderSpacingProperties[] = { CSSPropertyWebkitBorderHorizontalSpacing, CSSPropertyWebkitBorderVerticalSpacing };
    DEFINE_STATIC_LOCAL(StylePropertyShorthand, borderSpacingLonghands, (borderSpacingProperties, WTF_ARRAY_LENGTH(borderSpacingProperties)));
    return borderSpacingLonghands;
}

const StylePropertyShorthand& borderStyleShorthand()
{
    static const CSSPropertyID borderStyleProperties[] = {
        CSSPropertyBorderTopStyle,
        CSSPropertyBorderRightStyle,
        CSSPropertyBorderBottomStyle,
        CSSPropertyBorderLeftStyle
    };
    DEFINE_STATIC_LOCAL(StylePropertyShorthand, borderStyleLonghands, (borderStyleProperties, WTF_ARRAY_LENGTH(borderStyleProperties)));
    return borderStyleLonghands;
}

const StylePropertyShorthand& borderTopShorthand()
{
    static const CSSPropertyID borderTopProperties[] = { CSSPropertyBorderTopWidth, CSSPropertyBorderTopStyle, CSSPropertyBorderTopColor };
    DEFINE_STATIC_LOCAL(StylePropertyShorthand, borderTopLonghands, (borderTopProperties, WTF_ARRAY_LENGTH(borderTopProperties)));
    return borderTopLonghands;
}

const StylePropertyShorthand& borderWidthShorthand()
{
    static const CSSPropertyID borderWidthProperties[] = {
        CSSPropertyBorderTopWidth,
        CSSPropertyBorderRightWidth,
        CSSPropertyBorderBottomWidth,
        CSSPropertyBorderLeftWidth
    };
    DEFINE_STATIC_LOCAL(StylePropertyShorthand, borderWidthLonghands, (borderWidthProperties, WTF_ARRAY_LENGTH(borderWidthProperties)));
    return borderWidthLonghands;
}

const StylePropertyShorthand& listStyleShorthand()
{
    static const CSSPropertyID listStyleProperties[] = {
        CSSPropertyListStyleType,
        CSSPropertyListStylePosition,
        CSSPropertyListStyleImage
    };
    DEFINE_STATIC_LOCAL(StylePropertyShorthand, listStyleLonghands, (listStyleProperties, WTF_ARRAY_LENGTH(listStyleProperties)));
    return listStyleLonghands;
}

const StylePropertyShorthand& fontShorthand()
{
    static const CSSPropertyID fontProperties[] = {
        CSSPropertyFontFamily,
        CSSPropertyFontSize,
        CSSPropertyFontStyle,
        CSSPropertyFontVariant,
        CSSPropertyFontWeight,
        CSSPropertyLineHeight
    };
    DEFINE_STATIC_LOCAL(StylePropertyShorthand, fontLonghands, (fontProperties, WTF_ARRAY_LENGTH(fontProperties)));
    return fontLonghands;
}

const StylePropertyShorthand& marginShorthand()
{
    static const CSSPropertyID marginProperties[] = {
        CSSPropertyMarginTop,
        CSSPropertyMarginRight,
        CSSPropertyMarginBottom,
        CSSPropertyMarginLeft
    };
    DEFINE_STATIC_LOCAL(StylePropertyShorthand, marginLonghands, (marginProperties, WTF_ARRAY_LENGTH(marginProperties)));
    return marginLonghands;
}

const StylePropertyShorthand& outlineShorthand()
{
    static const CSSPropertyID outlineProperties[] = {
        CSSPropertyOutlineColor,
        CSSPropertyOutlineStyle,
        CSSPropertyOutlineWidth
    };
    DEFINE_STATIC_LOCAL(StylePropertyShorthand, outlineLonghands, (outlineProperties, WTF_ARRAY_LENGTH(outlineProperties)));
    return outlineLonghands;
}

const StylePropertyShorthand& overflowShorthand()
{
    static const CSSPropertyID overflowProperties[] = { CSSPropertyOverflowX, CSSPropertyOverflowY };
    DEFINE_STATIC_LOCAL(StylePropertyShorthand, overflowLonghands, (overflowProperties, WTF_ARRAY_LENGTH(overflowProperties)));
    return overflowLonghands;
}

const StylePropertyShorthand& paddingShorthand()
{
    static const CSSPropertyID paddingProperties[] = {
        CSSPropertyPaddingTop,
        CSSPropertyPaddingRight,
        CSSPropertyPaddingBottom,
        CSSPropertyPaddingLeft
    };
    DEFINE_STATIC_LOCAL(StylePropertyShorthand, paddingLonghands, (paddingProperties, WTF_ARRAY_LENGTH(paddingProperties)));
    return paddingLonghands;
}

const StylePropertyShorthand& transitionShorthand()
{
    static const CSSPropertyID transitionProperties[] = {
        CSSPropertyTransitionProperty,
        CSSPropertyTransitionDuration,
        CSSPropertyTransitionTimingFunction,
        CSSPropertyTransitionDelay
    };
    DEFINE_STATIC_LOCAL(StylePropertyShorthand, transitionLonghands, (transitionProperties, WTF_ARRAY_LENGTH(transitionProperties)));
    return transitionLonghands;
}

const StylePropertyShorthand& webkitAnimationShorthand()
{
    static const CSSPropertyID animationProperties[] = {
        CSSPropertyWebkitAnimationName,
        CSSPropertyWebkitAnimationDuration,
        CSSPropertyWebkitAnimationTimingFunction,
        CSSPropertyWebkitAnimationDelay,
        CSSPropertyWebkitAnimationIterationCount,
        CSSPropertyWebkitAnimationDirection,
        CSSPropertyWebkitAnimationFillMode
    };
    DEFINE_STATIC_LOCAL(StylePropertyShorthand, webkitAnimationLonghands, (animationProperties, WTF_ARRAY_LENGTH(animationProperties)));
    return webkitAnimationLonghands;
}

const StylePropertyShorthand& webkitAnimationShorthandForParsing()
{
    // When we parse the animation shorthand we need to look for animation-name
    // last because otherwise it might match against the keywords for fill mode,
    // timing functions and infinite iteration. This means that animation names
    // that are the same as keywords (e.g. 'forwards') won't always match in the
    // shorthand. In that case the authors should be using longhands (or
    // reconsidering their approach). This is covered by the animations spec
    // bug: https://www.w3.org/Bugs/Public/show_bug.cgi?id=14790
    // And in the spec (editor's draft) at:
    // http://dev.w3.org/csswg/css3-animations/#animation-shorthand-property
    static const CSSPropertyID animationPropertiesForParsing[] = {
        CSSPropertyWebkitAnimationDuration,
        CSSPropertyWebkitAnimationTimingFunction,
        CSSPropertyWebkitAnimationDelay,
        CSSPropertyWebkitAnimationIterationCount,
        CSSPropertyWebkitAnimationDirection,
        CSSPropertyWebkitAnimationFillMode,
        CSSPropertyWebkitAnimationName
    };

    DEFINE_STATIC_LOCAL(StylePropertyShorthand, webkitAnimationLonghandsForParsing, (animationPropertiesForParsing, WTF_ARRAY_LENGTH(animationPropertiesForParsing)));
    return webkitAnimationLonghandsForParsing;
}

const StylePropertyShorthand& webkitBorderAfterShorthand()
{
    static const CSSPropertyID borderAfterProperties[] = { CSSPropertyWebkitBorderAfterWidth, CSSPropertyWebkitBorderAfterStyle, CSSPropertyWebkitBorderAfterColor  };
    DEFINE_STATIC_LOCAL(StylePropertyShorthand, webkitBorderAfterLonghands, (borderAfterProperties, WTF_ARRAY_LENGTH(borderAfterProperties)));
    return webkitBorderAfterLonghands;
}

const StylePropertyShorthand& webkitBorderBeforeShorthand()
{
    static const CSSPropertyID borderBeforeProperties[] = { CSSPropertyWebkitBorderBeforeWidth, CSSPropertyWebkitBorderBeforeStyle, CSSPropertyWebkitBorderBeforeColor  };
    DEFINE_STATIC_LOCAL(StylePropertyShorthand, webkitBorderBeforeLonghands, (borderBeforeProperties, WTF_ARRAY_LENGTH(borderBeforeProperties)));
    return webkitBorderBeforeLonghands;
}

const StylePropertyShorthand& webkitBorderEndShorthand()
{
    static const CSSPropertyID borderEndProperties[] = { CSSPropertyWebkitBorderEndWidth, CSSPropertyWebkitBorderEndStyle, CSSPropertyWebkitBorderEndColor };
    DEFINE_STATIC_LOCAL(StylePropertyShorthand, webkitBorderEndLonghands, (borderEndProperties, WTF_ARRAY_LENGTH(borderEndProperties)));
    return webkitBorderEndLonghands;
}

const StylePropertyShorthand& webkitBorderStartShorthand()
{
    static const CSSPropertyID borderStartProperties[] = { CSSPropertyWebkitBorderStartWidth, CSSPropertyWebkitBorderStartStyle, CSSPropertyWebkitBorderStartColor };
    DEFINE_STATIC_LOCAL(StylePropertyShorthand, webkitBorderStartLonghands, (borderStartProperties, WTF_ARRAY_LENGTH(borderStartProperties)));
    return webkitBorderStartLonghands;
}

const StylePropertyShorthand& webkitColumnsShorthand()
{
    static const CSSPropertyID columnsProperties[] = { CSSPropertyWebkitColumnWidth, CSSPropertyWebkitColumnCount };
    DEFINE_STATIC_LOCAL(StylePropertyShorthand, webkitColumnsLonghands, (columnsProperties, WTF_ARRAY_LENGTH(columnsProperties)));
    return webkitColumnsLonghands;
}

const StylePropertyShorthand& webkitColumnRuleShorthand()
{
    static const CSSPropertyID columnRuleProperties[] = {
        CSSPropertyWebkitColumnRuleWidth,
        CSSPropertyWebkitColumnRuleStyle,
        CSSPropertyWebkitColumnRuleColor,
    };
    DEFINE_STATIC_LOCAL(StylePropertyShorthand, webkitColumnRuleLonghands, (columnRuleProperties, WTF_ARRAY_LENGTH(columnRuleProperties)));
    return webkitColumnRuleLonghands;
}

const StylePropertyShorthand& webkitFlexFlowShorthand()
{
    static const CSSPropertyID flexFlowProperties[] = { CSSPropertyWebkitFlexDirection, CSSPropertyWebkitFlexWrap };
    DEFINE_STATIC_LOCAL(StylePropertyShorthand, webkitFlexFlowLonghands, (flexFlowProperties, WTF_ARRAY_LENGTH(flexFlowProperties)));
    return webkitFlexFlowLonghands;
}

const StylePropertyShorthand& webkitFlexShorthand()
{
    static const CSSPropertyID flexProperties[] = { CSSPropertyWebkitFlexGrow, CSSPropertyWebkitFlexShrink, CSSPropertyWebkitFlexBasis };
    DEFINE_STATIC_LOCAL(StylePropertyShorthand, webkitFlexLonghands, (flexProperties, WTF_ARRAY_LENGTH(flexProperties)));
    return webkitFlexLonghands;
}

const StylePropertyShorthand& webkitMarginCollapseShorthand()
{
    static const CSSPropertyID marginCollapseProperties[] = { CSSPropertyWebkitMarginBeforeCollapse, CSSPropertyWebkitMarginAfterCollapse };
    DEFINE_STATIC_LOCAL(StylePropertyShorthand, webkitMarginCollapseLonghands, (marginCollapseProperties, WTF_ARRAY_LENGTH(marginCollapseProperties)));
    return webkitMarginCollapseLonghands;
}

const StylePropertyShorthand& webkitGridColumnShorthand()
{
    static const CSSPropertyID webkitGridColumnProperties[] = {
        CSSPropertyWebkitGridStart,
        CSSPropertyWebkitGridEnd
    };
    DEFINE_STATIC_LOCAL(StylePropertyShorthand, webkitGridColumnLonghands, (webkitGridColumnProperties, WTF_ARRAY_LENGTH(webkitGridColumnProperties)));
    return webkitGridColumnLonghands;

}

const StylePropertyShorthand& webkitGridRowShorthand()
{
    static const CSSPropertyID webkitGridRowProperties[] = {
        CSSPropertyWebkitGridBefore,
        CSSPropertyWebkitGridAfter
    };
    DEFINE_STATIC_LOCAL(StylePropertyShorthand, webkitGridRowLonghands, (webkitGridRowProperties, WTF_ARRAY_LENGTH(webkitGridRowProperties)));
    return webkitGridRowLonghands;

}

const StylePropertyShorthand& webkitMarqueeShorthand()
{
    static const CSSPropertyID marqueeProperties[] = {
        CSSPropertyWebkitMarqueeDirection,
        CSSPropertyWebkitMarqueeIncrement,
        CSSPropertyWebkitMarqueeRepetition,
        CSSPropertyWebkitMarqueeStyle,
        CSSPropertyWebkitMarqueeSpeed
    };
    DEFINE_STATIC_LOCAL(StylePropertyShorthand, webkitMarqueeLonghands, (marqueeProperties, WTF_ARRAY_LENGTH(marqueeProperties)));
    return webkitMarqueeLonghands;
}

const StylePropertyShorthand& webkitMaskShorthand()
{
    static const CSSPropertyID maskProperties[] = {
        CSSPropertyWebkitMaskImage,
        CSSPropertyWebkitMaskPositionX,
        CSSPropertyWebkitMaskPositionY,
        CSSPropertyWebkitMaskSize,
        CSSPropertyWebkitMaskRepeatX,
        CSSPropertyWebkitMaskRepeatY,
        CSSPropertyWebkitMaskOrigin,
        CSSPropertyWebkitMaskClip
    };
    DEFINE_STATIC_LOCAL(StylePropertyShorthand, webkitMaskLonghands, (maskProperties, WTF_ARRAY_LENGTH(maskProperties)));
    return webkitMaskLonghands;
}

const StylePropertyShorthand& webkitMaskPositionShorthand()
{
    static const CSSPropertyID maskPositionProperties[] = { CSSPropertyWebkitMaskPositionX, CSSPropertyWebkitMaskPositionY };
    DEFINE_STATIC_LOCAL(StylePropertyShorthand, webkitMaskPositionLonghands, (maskPositionProperties, WTF_ARRAY_LENGTH(maskPositionProperties)));
    return webkitMaskPositionLonghands;
}

const StylePropertyShorthand& webkitMaskRepeatShorthand()
{
    static const CSSPropertyID maskRepeatProperties[] = { CSSPropertyWebkitMaskRepeatX, CSSPropertyWebkitMaskRepeatY };
    DEFINE_STATIC_LOCAL(StylePropertyShorthand, webkitMaskRepeatLonghands, (maskRepeatProperties, WTF_ARRAY_LENGTH(maskRepeatProperties)));
    return webkitMaskRepeatLonghands;
}

const StylePropertyShorthand& webkitTextEmphasisShorthand()
{
    static const CSSPropertyID textEmphasisProperties[] = {
        CSSPropertyWebkitTextEmphasisStyle,
        CSSPropertyWebkitTextEmphasisColor
    };
    DEFINE_STATIC_LOCAL(StylePropertyShorthand, webkitTextEmphasisLonghands, (textEmphasisProperties, WTF_ARRAY_LENGTH(textEmphasisProperties)));
    return webkitTextEmphasisLonghands;
}

const StylePropertyShorthand& webkitTextStrokeShorthand()
{
    static const CSSPropertyID textStrokeProperties[] = { CSSPropertyWebkitTextStrokeWidth, CSSPropertyWebkitTextStrokeColor };
    DEFINE_STATIC_LOCAL(StylePropertyShorthand, webkitTextStrokeLonghands, (textStrokeProperties, WTF_ARRAY_LENGTH(textStrokeProperties)));
    return webkitTextStrokeLonghands;
}

const StylePropertyShorthand& webkitTransitionShorthand()
{
    static const CSSPropertyID transitionProperties[] = {
        CSSPropertyWebkitTransitionProperty,
        CSSPropertyWebkitTransitionDuration,
        CSSPropertyWebkitTransitionTimingFunction,
        CSSPropertyWebkitTransitionDelay
    };
    DEFINE_STATIC_LOCAL(StylePropertyShorthand, webkitTransitionLonghands, (transitionProperties, WTF_ARRAY_LENGTH(transitionProperties)));
    return webkitTransitionLonghands;
}

const StylePropertyShorthand& webkitTransformOriginShorthand()
{
    static const CSSPropertyID transformOriginProperties[] = {
        CSSPropertyWebkitTransformOriginX,
        CSSPropertyWebkitTransformOriginY,
        CSSPropertyWebkitTransformOriginZ
    };
    DEFINE_STATIC_LOCAL(StylePropertyShorthand, webkitTransformOriginLonghands, (transformOriginProperties, WTF_ARRAY_LENGTH(transformOriginProperties)));
    return webkitTransformOriginLonghands;
}

// Returns an empty list if the property is not a shorthand
const StylePropertyShorthand& shorthandForProperty(CSSPropertyID propertyID)
{
    switch (propertyID) {
    case CSSPropertyBackground:
        return backgroundShorthand();
    case CSSPropertyBackgroundPosition:
        return backgroundPositionShorthand();
    case CSSPropertyBackgroundRepeat:
        return backgroundRepeatShorthand();
    case CSSPropertyBorder:
        return borderShorthand();
    case CSSPropertyBorderBottom:
        return borderBottomShorthand();
    case CSSPropertyBorderColor:
        return borderColorShorthand();
    case CSSPropertyBorderImage:
        return borderImageShorthand();
    case CSSPropertyBorderLeft:
        return borderLeftShorthand();
    case CSSPropertyBorderRadius:
        return borderRadiusShorthand();
    case CSSPropertyBorderRight:
        return borderRightShorthand();
    case CSSPropertyBorderSpacing:
        return borderSpacingShorthand();
    case CSSPropertyBorderStyle:
        return borderStyleShorthand();
    case CSSPropertyBorderTop:
        return borderTopShorthand();
    case CSSPropertyBorderWidth:
        return borderWidthShorthand();
    case CSSPropertyListStyle:
        return listStyleShorthand();
    case CSSPropertyFont:
        return fontShorthand();
    case CSSPropertyMargin:
        return marginShorthand();
    case CSSPropertyOutline:
        return outlineShorthand();
    case CSSPropertyOverflow:
        return overflowShorthand();
    case CSSPropertyPadding:
        return paddingShorthand();
    case CSSPropertyTransition:
        return transitionShorthand();
    case CSSPropertyWebkitAnimation:
        return webkitAnimationShorthand();
    case CSSPropertyWebkitBorderAfter:
        return webkitBorderAfterShorthand();
    case CSSPropertyWebkitBorderBefore:
        return webkitBorderBeforeShorthand();
    case CSSPropertyWebkitBorderEnd:
        return webkitBorderEndShorthand();
    case CSSPropertyWebkitBorderStart:
        return webkitBorderStartShorthand();
    case CSSPropertyWebkitBorderRadius:
        return borderRadiusShorthand();
    case CSSPropertyWebkitColumns:
        return webkitColumnsShorthand();
    case CSSPropertyWebkitColumnRule:
        return webkitColumnRuleShorthand();
    case CSSPropertyWebkitFlex:
        return webkitFlexShorthand();
    case CSSPropertyWebkitFlexFlow:
        return webkitFlexFlowShorthand();
    case CSSPropertyWebkitGridColumn:
        return webkitGridColumnShorthand();
    case CSSPropertyWebkitGridRow:
        return webkitGridRowShorthand();
    case CSSPropertyWebkitMarginCollapse:
        return webkitMarginCollapseShorthand();
    case CSSPropertyWebkitMarquee:
        return webkitMarqueeShorthand();
    case CSSPropertyWebkitMask:
        return webkitMaskShorthand();
    case CSSPropertyWebkitMaskPosition:
        return webkitMaskPositionShorthand();
    case CSSPropertyWebkitMaskRepeat:
        return webkitMaskRepeatShorthand();
    case CSSPropertyWebkitTextEmphasis:
        return webkitTextEmphasisShorthand();
    case CSSPropertyWebkitTextStroke:
        return webkitTextStrokeShorthand();
    case CSSPropertyWebkitTransition:
        return webkitTransitionShorthand();
    case CSSPropertyWebkitTransformOrigin:
        return webkitTransformOriginShorthand();
    default: {
        DEFINE_STATIC_LOCAL(StylePropertyShorthand, emptyShorthand, ());
        return emptyShorthand;
    }
    }
}

bool isExpandedShorthand(CSSPropertyID id)
{
    // The system fonts bypass the normal style resolution by using RenderTheme,
    // thus we need to special case it here. FIXME: This is a violation of CSS 3 Fonts
    // as we should still be able to change the longhands.
    // DON'T ADD ANY SHORTHAND HERE UNLESS IT ISN'T ALWAYS EXPANDED AT PARSE TIME (which is wrong).
    if (id == CSSPropertyFont)
        return false;

    return shorthandForProperty(id).length();
}

} // namespace WebCore
