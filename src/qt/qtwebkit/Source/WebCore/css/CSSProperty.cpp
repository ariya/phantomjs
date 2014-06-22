/**
 * (C) 1999-2003 Lars Knoll (knoll@kde.org)
 * Copyright (C) 2004, 2005, 2006 Apple Computer, Inc.
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
#include "CSSProperty.h"

#include "CSSValueList.h"
#include "RenderStyleConstants.h"
#include "StylePropertyShorthand.h"

#if ENABLE(CSS_VARIABLES)
#include "CSSVariableValue.h"
#endif

#include <wtf/text/StringBuilder.h>

namespace WebCore {

struct SameSizeAsCSSProperty {
    uint32_t bitfields;
    void* value;
};

COMPILE_ASSERT(sizeof(CSSProperty) == sizeof(SameSizeAsCSSProperty), CSSProperty_should_stay_small);

void CSSProperty::wrapValueInCommaSeparatedList()
{
    RefPtr<CSSValue> value = m_value.release();
    m_value = CSSValueList::createCommaSeparated();
    static_cast<CSSValueList*>(m_value.get())->append(value.release());
}

enum LogicalBoxSide { BeforeSide, EndSide, AfterSide, StartSide };
enum PhysicalBoxSide { TopSide, RightSide, BottomSide, LeftSide };

static CSSPropertyID resolveToPhysicalProperty(TextDirection direction, WritingMode writingMode, LogicalBoxSide logicalSide, const StylePropertyShorthand& shorthand)
{
    if (direction == LTR) {
        if (writingMode == TopToBottomWritingMode) {
            // The common case. The logical and physical box sides match.
            // Left = Start, Right = End, Before = Top, After = Bottom
            return shorthand.properties()[logicalSide];
        }

        if (writingMode == BottomToTopWritingMode) {
            // Start = Left, End = Right, Before = Bottom, After = Top.
            switch (logicalSide) {
            case StartSide:
                return shorthand.properties()[LeftSide];
            case EndSide:
                return shorthand.properties()[RightSide];
            case BeforeSide:
                return shorthand.properties()[BottomSide];
            default:
                return shorthand.properties()[TopSide];
            }
        }

        if (writingMode == LeftToRightWritingMode) {
            // Start = Top, End = Bottom, Before = Left, After = Right.
            switch (logicalSide) {
            case StartSide:
                return shorthand.properties()[TopSide];
            case EndSide:
                return shorthand.properties()[BottomSide];
            case BeforeSide:
                return shorthand.properties()[LeftSide];
            default:
                return shorthand.properties()[RightSide];
            }
        }

        // Start = Top, End = Bottom, Before = Right, After = Left
        switch (logicalSide) {
        case StartSide:
            return shorthand.properties()[TopSide];
        case EndSide:
            return shorthand.properties()[BottomSide];
        case BeforeSide:
            return shorthand.properties()[RightSide];
        default:
            return shorthand.properties()[LeftSide];
        }
    }

    if (writingMode == TopToBottomWritingMode) {
        // Start = Right, End = Left, Before = Top, After = Bottom
        switch (logicalSide) {
        case StartSide:
            return shorthand.properties()[RightSide];
        case EndSide:
            return shorthand.properties()[LeftSide];
        case BeforeSide:
            return shorthand.properties()[TopSide];
        default:
            return shorthand.properties()[BottomSide];
        }
    }

    if (writingMode == BottomToTopWritingMode) {
        // Start = Right, End = Left, Before = Bottom, After = Top
        switch (logicalSide) {
        case StartSide:
            return shorthand.properties()[RightSide];
        case EndSide:
            return shorthand.properties()[LeftSide];
        case BeforeSide:
            return shorthand.properties()[BottomSide];
        default:
            return shorthand.properties()[TopSide];
        }
    }

    if (writingMode == LeftToRightWritingMode) {
        // Start = Bottom, End = Top, Before = Left, After = Right
        switch (logicalSide) {
        case StartSide:
            return shorthand.properties()[BottomSide];
        case EndSide:
            return shorthand.properties()[TopSide];
        case BeforeSide:
            return shorthand.properties()[LeftSide];
        default:
            return shorthand.properties()[RightSide];
        }
    }

    // Start = Bottom, End = Top, Before = Right, After = Left
    switch (logicalSide) {
    case StartSide:
        return shorthand.properties()[BottomSide];
    case EndSide:
        return shorthand.properties()[TopSide];
    case BeforeSide:
        return shorthand.properties()[RightSide];
    default:
        return shorthand.properties()[LeftSide];
    }
}

enum LogicalExtent { LogicalWidth, LogicalHeight };

static CSSPropertyID resolveToPhysicalProperty(WritingMode writingMode, LogicalExtent logicalSide, const CSSPropertyID* properties)
{
    if (writingMode == TopToBottomWritingMode || writingMode == BottomToTopWritingMode)
        return properties[logicalSide];
    return logicalSide == LogicalWidth ? properties[1] : properties[0];
}

static const StylePropertyShorthand& borderDirections()
{
    static const CSSPropertyID properties[4] = { CSSPropertyBorderTop, CSSPropertyBorderRight, CSSPropertyBorderBottom, CSSPropertyBorderLeft };
    DEFINE_STATIC_LOCAL(StylePropertyShorthand, borderDirections, (properties, WTF_ARRAY_LENGTH(properties)));
    return borderDirections;
}

CSSPropertyID CSSProperty::resolveDirectionAwareProperty(CSSPropertyID propertyID, TextDirection direction, WritingMode writingMode)
{
    switch (propertyID) {
    case CSSPropertyWebkitMarginEnd:
        return resolveToPhysicalProperty(direction, writingMode, EndSide, marginShorthand());
    case CSSPropertyWebkitMarginStart:
        return resolveToPhysicalProperty(direction, writingMode, StartSide, marginShorthand());
    case CSSPropertyWebkitMarginBefore:
        return resolveToPhysicalProperty(direction, writingMode, BeforeSide, marginShorthand());
    case CSSPropertyWebkitMarginAfter:
        return resolveToPhysicalProperty(direction, writingMode, AfterSide, marginShorthand());
    case CSSPropertyWebkitPaddingEnd:
        return resolveToPhysicalProperty(direction, writingMode, EndSide, paddingShorthand());
    case CSSPropertyWebkitPaddingStart:
        return resolveToPhysicalProperty(direction, writingMode, StartSide, paddingShorthand());
    case CSSPropertyWebkitPaddingBefore:
        return resolveToPhysicalProperty(direction, writingMode, BeforeSide, paddingShorthand());
    case CSSPropertyWebkitPaddingAfter:
        return resolveToPhysicalProperty(direction, writingMode, AfterSide, paddingShorthand());
    case CSSPropertyWebkitBorderEnd:
        return resolveToPhysicalProperty(direction, writingMode, EndSide, borderDirections());
    case CSSPropertyWebkitBorderStart:
        return resolveToPhysicalProperty(direction, writingMode, StartSide, borderDirections());
    case CSSPropertyWebkitBorderBefore:
        return resolveToPhysicalProperty(direction, writingMode, BeforeSide, borderDirections());
    case CSSPropertyWebkitBorderAfter:
        return resolveToPhysicalProperty(direction, writingMode, AfterSide, borderDirections());
    case CSSPropertyWebkitBorderEndColor:
        return resolveToPhysicalProperty(direction, writingMode, EndSide, borderColorShorthand());
    case CSSPropertyWebkitBorderStartColor:
        return resolveToPhysicalProperty(direction, writingMode, StartSide, borderColorShorthand());
    case CSSPropertyWebkitBorderBeforeColor:
        return resolveToPhysicalProperty(direction, writingMode, BeforeSide, borderColorShorthand());
    case CSSPropertyWebkitBorderAfterColor:
        return resolveToPhysicalProperty(direction, writingMode, AfterSide, borderColorShorthand());
    case CSSPropertyWebkitBorderEndStyle:
        return resolveToPhysicalProperty(direction, writingMode, EndSide, borderStyleShorthand());
    case CSSPropertyWebkitBorderStartStyle:
        return resolveToPhysicalProperty(direction, writingMode, StartSide, borderStyleShorthand());
    case CSSPropertyWebkitBorderBeforeStyle:
        return resolveToPhysicalProperty(direction, writingMode, BeforeSide, borderStyleShorthand());
    case CSSPropertyWebkitBorderAfterStyle:
        return resolveToPhysicalProperty(direction, writingMode, AfterSide, borderStyleShorthand());
    case CSSPropertyWebkitBorderEndWidth:
        return resolveToPhysicalProperty(direction, writingMode, EndSide, borderWidthShorthand());
    case CSSPropertyWebkitBorderStartWidth:
        return resolveToPhysicalProperty(direction, writingMode, StartSide, borderWidthShorthand());
    case CSSPropertyWebkitBorderBeforeWidth:
        return resolveToPhysicalProperty(direction, writingMode, BeforeSide, borderWidthShorthand());
    case CSSPropertyWebkitBorderAfterWidth:
        return resolveToPhysicalProperty(direction, writingMode, AfterSide, borderWidthShorthand());
    case CSSPropertyWebkitLogicalWidth: {
        const CSSPropertyID properties[2] = { CSSPropertyWidth, CSSPropertyHeight };
        return resolveToPhysicalProperty(writingMode, LogicalWidth, properties);
    }
    case CSSPropertyWebkitLogicalHeight: {
        const CSSPropertyID properties[2] = { CSSPropertyWidth, CSSPropertyHeight };
        return resolveToPhysicalProperty(writingMode, LogicalHeight, properties);
    }
    case CSSPropertyWebkitMinLogicalWidth: {
        const CSSPropertyID properties[2] = { CSSPropertyMinWidth, CSSPropertyMinHeight };
        return resolveToPhysicalProperty(writingMode, LogicalWidth, properties);
    }
    case CSSPropertyWebkitMinLogicalHeight: {
        const CSSPropertyID properties[2] = { CSSPropertyMinWidth, CSSPropertyMinHeight };
        return resolveToPhysicalProperty(writingMode, LogicalHeight, properties);
    }
    case CSSPropertyWebkitMaxLogicalWidth: {
        const CSSPropertyID properties[2] = { CSSPropertyMaxWidth, CSSPropertyMaxHeight };
        return resolveToPhysicalProperty(writingMode, LogicalWidth, properties);
    }
    case CSSPropertyWebkitMaxLogicalHeight: {
        const CSSPropertyID properties[2] = { CSSPropertyMaxWidth, CSSPropertyMaxHeight };
        return resolveToPhysicalProperty(writingMode, LogicalHeight, properties);
    }
    default:
        return propertyID;
    }
}

bool CSSProperty::isInheritedProperty(CSSPropertyID propertyID)
{
    switch (propertyID) {
    case CSSPropertyBorderCollapse:
    case CSSPropertyBorderSpacing:
    case CSSPropertyCaptionSide:
    case CSSPropertyColor:
    case CSSPropertyCursor:
    case CSSPropertyDirection:
    case CSSPropertyEmptyCells:
    case CSSPropertyFont:
    case CSSPropertyFontFamily:
    case CSSPropertyFontSize:
    case CSSPropertyFontStyle:
    case CSSPropertyFontVariant:
    case CSSPropertyFontWeight:
#if ENABLE(CSS_IMAGE_ORIENTATION)
    case CSSPropertyImageOrientation:
#endif
    case CSSPropertyImageRendering:
#if ENABLE(CSS_IMAGE_RESOLUTION)
    case CSSPropertyImageResolution:
#endif
    case CSSPropertyLetterSpacing:
    case CSSPropertyLineHeight:
    case CSSPropertyListStyle:
    case CSSPropertyListStyleImage:
    case CSSPropertyListStyleType:
    case CSSPropertyListStylePosition:
    case CSSPropertyOrphans:
    case CSSPropertyPointerEvents:
    case CSSPropertyQuotes:
    case CSSPropertyResize:
    case CSSPropertySpeak:
    case CSSPropertyTabSize:
    case CSSPropertyTextAlign:
    case CSSPropertyTextDecoration:
    case CSSPropertyTextIndent:
    case CSSPropertyTextRendering:
    case CSSPropertyTextShadow:
    case CSSPropertyTextTransform:
#if ENABLE(CSS_VARIABLES)
    case CSSPropertyVariable:
#endif
    case CSSPropertyVisibility:
    case CSSPropertyWebkitAspectRatio:
    case CSSPropertyWebkitBorderHorizontalSpacing:
    case CSSPropertyWebkitBorderVerticalSpacing:
    case CSSPropertyWebkitBoxDirection:
    case CSSPropertyWebkitColorCorrection:
#if ENABLE(CURSOR_VISIBILITY)
    case CSSPropertyWebkitCursorVisibility:
#endif
    case CSSPropertyWebkitFontFeatureSettings:
    case CSSPropertyWebkitFontKerning:
    case CSSPropertyWebkitFontSmoothing:
    case CSSPropertyWebkitFontVariantLigatures:
    case CSSPropertyWebkitLocale:
    case CSSPropertyWebkitHighlight:
    case CSSPropertyWebkitHyphenateCharacter:
    case CSSPropertyWebkitHyphenateLimitAfter:
    case CSSPropertyWebkitHyphenateLimitBefore:
    case CSSPropertyWebkitHyphenateLimitLines:
    case CSSPropertyWebkitHyphens:
    case CSSPropertyWebkitLineAlign:
    case CSSPropertyWebkitLineBoxContain:
    case CSSPropertyWebkitLineBreak:
    case CSSPropertyWebkitLineGrid:
    case CSSPropertyWebkitLineSnap:
    case CSSPropertyWebkitNbspMode:
#if ENABLE(ACCELERATED_OVERFLOW_SCROLLING)
    case CSSPropertyWebkitOverflowScrolling:
#endif
    case CSSPropertyWebkitPrintColorAdjust:
    case CSSPropertyWebkitRtlOrdering:
    case CSSPropertyWebkitRubyPosition:
    case CSSPropertyWebkitTextCombine:
#if ENABLE(CSS3_TEXT)
    case CSSPropertyWebkitTextDecorationLine:
    case CSSPropertyWebkitTextAlignLast:
    case CSSPropertyWebkitTextJustify:
    case CSSPropertyWebkitTextUnderlinePosition:
#endif // CSS3_TEXT
    case CSSPropertyWebkitTextDecorationsInEffect:
    case CSSPropertyWebkitTextEmphasis:
    case CSSPropertyWebkitTextEmphasisColor:
    case CSSPropertyWebkitTextEmphasisPosition:
    case CSSPropertyWebkitTextEmphasisStyle:
    case CSSPropertyWebkitTextFillColor:
    case CSSPropertyWebkitTextOrientation:
    case CSSPropertyWebkitTextSecurity:
    case CSSPropertyWebkitTextStroke:
    case CSSPropertyWebkitTextStrokeColor:
    case CSSPropertyWebkitTextStrokeWidth:
    case CSSPropertyWebkitUserModify:
    case CSSPropertyWebkitUserSelect:
    case CSSPropertyWebkitWritingMode:
    case CSSPropertyWhiteSpace:
    case CSSPropertyWidows:
    case CSSPropertyWordBreak:
    case CSSPropertyWordSpacing:
    case CSSPropertyWordWrap:
#if ENABLE(SVG)
    case CSSPropertyClipRule:
    case CSSPropertyColorInterpolation:
    case CSSPropertyColorInterpolationFilters:
    case CSSPropertyColorRendering:
    case CSSPropertyFill:
    case CSSPropertyFillOpacity:
    case CSSPropertyFillRule:
    case CSSPropertyGlyphOrientationHorizontal:
    case CSSPropertyGlyphOrientationVertical:
    case CSSPropertyKerning:
    case CSSPropertyMarker:
    case CSSPropertyMarkerEnd:
    case CSSPropertyMarkerMid:
    case CSSPropertyMarkerStart:
    case CSSPropertyStroke:
    case CSSPropertyStrokeDasharray:
    case CSSPropertyStrokeDashoffset:
    case CSSPropertyStrokeLinecap:
    case CSSPropertyStrokeLinejoin:
    case CSSPropertyStrokeMiterlimit:
    case CSSPropertyStrokeOpacity:
    case CSSPropertyStrokeWidth:
    case CSSPropertyShapeRendering:
    case CSSPropertyTextAnchor:
    case CSSPropertyWritingMode:
#endif
#if ENABLE(TOUCH_EVENTS)
    case CSSPropertyWebkitTapHighlightColor:
#endif
        return true;
    case CSSPropertyDisplay:
    case CSSPropertyZoom:
    case CSSPropertyBackground:
    case CSSPropertyBackgroundAttachment:
    case CSSPropertyBackgroundClip:
    case CSSPropertyBackgroundColor:
    case CSSPropertyBackgroundImage:
    case CSSPropertyBackgroundOrigin:
    case CSSPropertyBackgroundPosition:
    case CSSPropertyBackgroundPositionX:
    case CSSPropertyBackgroundPositionY:
    case CSSPropertyBackgroundRepeat:
    case CSSPropertyBackgroundRepeatX:
    case CSSPropertyBackgroundRepeatY:
    case CSSPropertyBackgroundSize:
    case CSSPropertyBorder:
    case CSSPropertyBorderBottom:
    case CSSPropertyBorderBottomColor:
    case CSSPropertyBorderBottomLeftRadius:
    case CSSPropertyBorderBottomRightRadius:
    case CSSPropertyBorderBottomStyle:
    case CSSPropertyBorderBottomWidth:
    case CSSPropertyBorderColor:
    case CSSPropertyBorderImage:
    case CSSPropertyBorderImageOutset:
    case CSSPropertyBorderImageRepeat:
    case CSSPropertyBorderImageSlice:
    case CSSPropertyBorderImageSource:
    case CSSPropertyBorderImageWidth:
    case CSSPropertyBorderLeft:
    case CSSPropertyBorderLeftColor:
    case CSSPropertyBorderLeftStyle:
    case CSSPropertyBorderLeftWidth:
    case CSSPropertyBorderRadius:
    case CSSPropertyBorderRight:
    case CSSPropertyBorderRightColor:
    case CSSPropertyBorderRightStyle:
    case CSSPropertyBorderRightWidth:
    case CSSPropertyBorderStyle:
    case CSSPropertyBorderTop:
    case CSSPropertyBorderTopColor:
    case CSSPropertyBorderTopLeftRadius:
    case CSSPropertyBorderTopRightRadius:
    case CSSPropertyBorderTopStyle:
    case CSSPropertyBorderTopWidth:
    case CSSPropertyBorderWidth:
    case CSSPropertyBottom:
    case CSSPropertyBoxShadow:
    case CSSPropertyBoxSizing:
    case CSSPropertyClear:
    case CSSPropertyClip:
    case CSSPropertyContent:
    case CSSPropertyCounterIncrement:
    case CSSPropertyCounterReset:
    case CSSPropertyFloat:
    case CSSPropertyFontStretch:
#if ENABLE(CSS_SHADERS)
    case CSSPropertyGeometry:
#endif
    case CSSPropertyHeight:
    case CSSPropertyLeft:
    case CSSPropertyMargin:
    case CSSPropertyMarginBottom:
    case CSSPropertyMarginLeft:
    case CSSPropertyMarginRight:
    case CSSPropertyMarginTop:
    case CSSPropertyMaxHeight:
    case CSSPropertyMaxWidth:
    case CSSPropertyMinHeight:
    case CSSPropertyMinWidth:
    case CSSPropertyOpacity:
    case CSSPropertyOutline:
    case CSSPropertyOutlineColor:
    case CSSPropertyOutlineOffset:
    case CSSPropertyOutlineStyle:
    case CSSPropertyOutlineWidth:
    case CSSPropertyOverflow:
    case CSSPropertyOverflowWrap:
    case CSSPropertyOverflowX:
    case CSSPropertyOverflowY:
    case CSSPropertyPadding:
    case CSSPropertyPaddingBottom:
    case CSSPropertyPaddingLeft:
    case CSSPropertyPaddingRight:
    case CSSPropertyPaddingTop:
    case CSSPropertyPage:
    case CSSPropertyPageBreakAfter:
    case CSSPropertyPageBreakBefore:
    case CSSPropertyPageBreakInside:
    case CSSPropertyPosition:
    case CSSPropertyRight:
    case CSSPropertySize:
    case CSSPropertySrc:
    case CSSPropertyTableLayout:
    case CSSPropertyTextLineThrough:
    case CSSPropertyTextLineThroughColor:
    case CSSPropertyTextLineThroughMode:
    case CSSPropertyTextLineThroughStyle:
    case CSSPropertyTextLineThroughWidth:
    case CSSPropertyTextOverflow:
    case CSSPropertyTextOverline:
    case CSSPropertyTextOverlineColor:
    case CSSPropertyTextOverlineMode:
    case CSSPropertyTextOverlineStyle:
    case CSSPropertyTextOverlineWidth:
    case CSSPropertyTextUnderline:
    case CSSPropertyTextUnderlineColor:
    case CSSPropertyTextUnderlineMode:
    case CSSPropertyTextUnderlineStyle:
    case CSSPropertyTextUnderlineWidth:
    case CSSPropertyTop:
    case CSSPropertyTransition:
    case CSSPropertyTransitionDelay:
    case CSSPropertyTransitionDuration:
    case CSSPropertyTransitionProperty:
    case CSSPropertyTransitionTimingFunction:
    case CSSPropertyUnicodeBidi:
    case CSSPropertyUnicodeRange:
    case CSSPropertyVerticalAlign:
    case CSSPropertyWidth:
    case CSSPropertyZIndex:
    case CSSPropertyWebkitAnimation:
    case CSSPropertyWebkitAnimationDelay:
    case CSSPropertyWebkitAnimationDirection:
    case CSSPropertyWebkitAnimationDuration:
    case CSSPropertyWebkitAnimationFillMode:
    case CSSPropertyWebkitAnimationIterationCount:
    case CSSPropertyWebkitAnimationName:
    case CSSPropertyWebkitAnimationPlayState:
    case CSSPropertyWebkitAnimationTimingFunction:
    case CSSPropertyWebkitAppearance:
    case CSSPropertyWebkitBackfaceVisibility:
    case CSSPropertyWebkitBackgroundBlendMode:
    case CSSPropertyWebkitBackgroundClip:
    case CSSPropertyWebkitBackgroundComposite:
    case CSSPropertyWebkitBackgroundOrigin:
    case CSSPropertyWebkitBackgroundSize:
    case CSSPropertyWebkitBorderAfter:
    case CSSPropertyWebkitBorderAfterColor:
    case CSSPropertyWebkitBorderAfterStyle:
    case CSSPropertyWebkitBorderAfterWidth:
    case CSSPropertyWebkitBorderBefore:
    case CSSPropertyWebkitBorderBeforeColor:
    case CSSPropertyWebkitBorderBeforeStyle:
    case CSSPropertyWebkitBorderBeforeWidth:
    case CSSPropertyWebkitBorderEnd:
    case CSSPropertyWebkitBorderEndColor:
    case CSSPropertyWebkitBorderEndStyle:
    case CSSPropertyWebkitBorderEndWidth:
    case CSSPropertyWebkitBorderFit:
    case CSSPropertyWebkitBorderImage:
    case CSSPropertyWebkitBorderRadius:
    case CSSPropertyWebkitBorderStart:
    case CSSPropertyWebkitBorderStartColor:
    case CSSPropertyWebkitBorderStartStyle:
    case CSSPropertyWebkitBorderStartWidth:
    case CSSPropertyWebkitBoxAlign:
#if ENABLE(CSS_BOX_DECORATION_BREAK)
    case CSSPropertyWebkitBoxDecorationBreak:
#endif
    case CSSPropertyWebkitBoxFlex:
    case CSSPropertyWebkitBoxFlexGroup:
    case CSSPropertyWebkitBoxLines:
    case CSSPropertyWebkitBoxOrdinalGroup:
    case CSSPropertyWebkitBoxOrient:
    case CSSPropertyWebkitBoxPack:
    case CSSPropertyWebkitBoxReflect:
    case CSSPropertyWebkitBoxShadow:
    case CSSPropertyWebkitClipPath:
    case CSSPropertyWebkitColumnAxis:
    case CSSPropertyWebkitColumnBreakAfter:
    case CSSPropertyWebkitColumnBreakBefore:
    case CSSPropertyWebkitColumnBreakInside:
    case CSSPropertyWebkitColumnCount:
    case CSSPropertyWebkitColumnGap:
    case CSSPropertyWebkitColumnProgression:
    case CSSPropertyWebkitColumnRule:
    case CSSPropertyWebkitColumnRuleColor:
    case CSSPropertyWebkitColumnRuleStyle:
    case CSSPropertyWebkitColumnRuleWidth:
    case CSSPropertyWebkitColumnSpan:
    case CSSPropertyWebkitColumnWidth:
    case CSSPropertyWebkitColumns:
#if ENABLE(CSS_FILTERS)
    case CSSPropertyWebkitFilter:
#endif
#if ENABLE(CSS_COMPOSITING)
    case CSSPropertyWebkitBlendMode:
#endif
    case CSSPropertyWebkitAlignContent:
    case CSSPropertyWebkitAlignItems:
    case CSSPropertyWebkitAlignSelf:
    case CSSPropertyWebkitFlex:
    case CSSPropertyWebkitFlexBasis:
    case CSSPropertyWebkitFlexDirection:
    case CSSPropertyWebkitFlexFlow:
    case CSSPropertyWebkitFlexGrow:
    case CSSPropertyWebkitFlexShrink:
    case CSSPropertyWebkitFlexWrap:
    case CSSPropertyWebkitJustifyContent:
    case CSSPropertyWebkitOrder:
    case CSSPropertyWebkitFontSizeDelta:
    case CSSPropertyWebkitGridAutoColumns:
    case CSSPropertyWebkitGridAutoFlow:
    case CSSPropertyWebkitGridAutoRows:
    case CSSPropertyWebkitGridDefinitionColumns:
    case CSSPropertyWebkitGridDefinitionRows:
    case CSSPropertyWebkitGridStart:
    case CSSPropertyWebkitGridEnd:
    case CSSPropertyWebkitGridBefore:
    case CSSPropertyWebkitGridAfter:
    case CSSPropertyWebkitGridColumn:
    case CSSPropertyWebkitGridRow:
    case CSSPropertyWebkitLineClamp:
    case CSSPropertyWebkitLogicalWidth:
    case CSSPropertyWebkitLogicalHeight:
    case CSSPropertyWebkitMarginAfterCollapse:
    case CSSPropertyWebkitMarginBeforeCollapse:
    case CSSPropertyWebkitMarginBottomCollapse:
    case CSSPropertyWebkitMarginTopCollapse:
    case CSSPropertyWebkitMarginCollapse:
    case CSSPropertyWebkitMarginAfter:
    case CSSPropertyWebkitMarginBefore:
    case CSSPropertyWebkitMarginEnd:
    case CSSPropertyWebkitMarginStart:
    case CSSPropertyWebkitMarquee:
    case CSSPropertyWebkitMarqueeDirection:
    case CSSPropertyWebkitMarqueeIncrement:
    case CSSPropertyWebkitMarqueeRepetition:
    case CSSPropertyWebkitMarqueeSpeed:
    case CSSPropertyWebkitMarqueeStyle:
    case CSSPropertyWebkitMask:
    case CSSPropertyWebkitMaskBoxImage:
    case CSSPropertyWebkitMaskBoxImageOutset:
    case CSSPropertyWebkitMaskBoxImageRepeat:
    case CSSPropertyWebkitMaskBoxImageSlice:
    case CSSPropertyWebkitMaskBoxImageSource:
    case CSSPropertyWebkitMaskBoxImageWidth:
    case CSSPropertyWebkitMaskClip:
    case CSSPropertyWebkitMaskComposite:
    case CSSPropertyWebkitMaskImage:
    case CSSPropertyWebkitMaskOrigin:
    case CSSPropertyWebkitMaskPosition:
    case CSSPropertyWebkitMaskPositionX:
    case CSSPropertyWebkitMaskPositionY:
    case CSSPropertyWebkitMaskRepeat:
    case CSSPropertyWebkitMaskRepeatX:
    case CSSPropertyWebkitMaskRepeatY:
    case CSSPropertyWebkitMaskSize:
    case CSSPropertyWebkitMaxLogicalWidth:
    case CSSPropertyWebkitMaxLogicalHeight:
    case CSSPropertyWebkitMinLogicalWidth:
    case CSSPropertyWebkitMinLogicalHeight:
    case CSSPropertyWebkitPaddingAfter:
    case CSSPropertyWebkitPaddingBefore:
    case CSSPropertyWebkitPaddingEnd:
    case CSSPropertyWebkitPaddingStart:
    case CSSPropertyWebkitPerspective:
    case CSSPropertyWebkitPerspectiveOrigin:
    case CSSPropertyWebkitPerspectiveOriginX:
    case CSSPropertyWebkitPerspectiveOriginY:
#if ENABLE(CSS3_TEXT)
    case CSSPropertyWebkitTextDecorationStyle:
    case CSSPropertyWebkitTextDecorationColor:
#endif // CSS3_TEXT
    case CSSPropertyWebkitTransform:
    case CSSPropertyWebkitTransformOrigin:
    case CSSPropertyWebkitTransformOriginX:
    case CSSPropertyWebkitTransformOriginY:
    case CSSPropertyWebkitTransformOriginZ:
    case CSSPropertyWebkitTransformStyle:
    case CSSPropertyWebkitTransition:
    case CSSPropertyWebkitTransitionDelay:
    case CSSPropertyWebkitTransitionDuration:
    case CSSPropertyWebkitTransitionProperty:
    case CSSPropertyWebkitTransitionTimingFunction:
    case CSSPropertyWebkitUserDrag:
#if ENABLE(CSS_REGIONS)
    case CSSPropertyWebkitFlowInto:
    case CSSPropertyWebkitFlowFrom:
    case CSSPropertyWebkitRegionBreakAfter:
    case CSSPropertyWebkitRegionBreakBefore:
    case CSSPropertyWebkitRegionBreakInside:
    case CSSPropertyWebkitRegionFragment:
#endif
#if ENABLE(CSS_EXCLUSIONS)
    case CSSPropertyWebkitWrapFlow:
    case CSSPropertyWebkitWrapThrough:
#endif
#if ENABLE(CSS_SHAPES)
    case CSSPropertyWebkitShapeMargin:
    case CSSPropertyWebkitShapePadding:
    case CSSPropertyWebkitShapeInside:
    case CSSPropertyWebkitShapeOutside:
#endif
#if ENABLE(CSS_SHADERS)
    case CSSPropertyMix:
    case CSSPropertyParameters:
#endif
#if ENABLE(SVG)
    case CSSPropertyClipPath:
    case CSSPropertyMask:
    case CSSPropertyMaskType:
    case CSSPropertyEnableBackground:
    case CSSPropertyFilter:
    case CSSPropertyFloodColor:
    case CSSPropertyFloodOpacity:
    case CSSPropertyLightingColor:
    case CSSPropertyStopColor:
    case CSSPropertyStopOpacity:
    case CSSPropertyColorProfile:
    case CSSPropertyAlignmentBaseline:
    case CSSPropertyBaselineShift:
    case CSSPropertyDominantBaseline:
    case CSSPropertyVectorEffect:
    case CSSPropertyBufferedRendering:
    case CSSPropertyWebkitSvgShadow:
#endif
#if ENABLE(DASHBOARD_SUPPORT)
    case CSSPropertyWebkitDashboardRegion:
#endif
#if ENABLE(DRAGGABLE_REGION)
    case CSSPropertyWebkitAppRegion:
#endif
#if ENABLE(CSS_DEVICE_ADAPTATION)
    case CSSPropertyMaxZoom:
    case CSSPropertyMinZoom:
    case CSSPropertyOrientation:
    case CSSPropertyUserZoom:
#endif
        return false;
    case CSSPropertyInvalid:
        ASSERT_NOT_REACHED();
        return false;
    }
    ASSERT_NOT_REACHED();
    return false;
}

} // namespace WebCore
