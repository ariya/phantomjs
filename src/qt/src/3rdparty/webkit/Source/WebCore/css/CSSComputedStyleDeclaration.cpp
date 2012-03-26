/*
 * Copyright (C) 2004 Zack Rusin <zack@kde.org>
 * Copyright (C) 2004, 2005, 2006, 2007, 2008, 2009, 2010 Apple Inc. All rights reserved.
 * Copyright (C) 2007 Alexey Proskuryakov <ap@webkit.org>
 * Copyright (C) 2007 Nicholas Shanks <webkit@nickshanks.com>
 * Copyright (C) 2011 Sencha, Inc. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301  USA
 */

#include "config.h"
#include "CSSComputedStyleDeclaration.h"

#include "AnimationController.h"
#include "ContentData.h"
#include "CounterContent.h"
#include "CursorList.h"
#include "CSSBorderImageValue.h"
#include "CSSLineBoxContainValue.h"
#include "CSSMutableStyleDeclaration.h"
#include "CSSPrimitiveValue.h"
#include "CSSPrimitiveValueCache.h"
#include "CSSPrimitiveValueMappings.h"
#include "CSSProperty.h"
#include "CSSPropertyNames.h"
#include "CSSReflectValue.h"
#include "CSSSelector.h"
#include "CSSTimingFunctionValue.h"
#include "CSSValueList.h"
#include "Document.h"
#include "ExceptionCode.h"
#include "Rect.h"
#include "RenderBox.h"
#include "RenderLayer.h"
#include "ShadowValue.h"
#include "WebKitCSSTransformValue.h"

#if ENABLE(DASHBOARD_SUPPORT)
#include "DashboardRegion.h"
#endif

namespace WebCore {

// List of all properties we know how to compute, omitting shorthands.
static const int computedProperties[] = {
    CSSPropertyBackgroundAttachment,
    CSSPropertyBackgroundClip,
    CSSPropertyBackgroundColor,
    CSSPropertyBackgroundImage,
    CSSPropertyBackgroundOrigin,
    CSSPropertyBackgroundPosition, // more-specific background-position-x/y are non-standard
    CSSPropertyBackgroundRepeat,
    CSSPropertyBackgroundSize,
    CSSPropertyBorderBottomColor,
    CSSPropertyBorderBottomLeftRadius,
    CSSPropertyBorderBottomRightRadius,
    CSSPropertyBorderBottomStyle,
    CSSPropertyBorderBottomWidth,
    CSSPropertyBorderCollapse,
    CSSPropertyBorderLeftColor,
    CSSPropertyBorderLeftStyle,
    CSSPropertyBorderLeftWidth,
    CSSPropertyBorderRightColor,
    CSSPropertyBorderRightStyle,
    CSSPropertyBorderRightWidth,
    CSSPropertyBorderTopColor,
    CSSPropertyBorderTopLeftRadius,
    CSSPropertyBorderTopRightRadius,
    CSSPropertyBorderTopStyle,
    CSSPropertyBorderTopWidth,
    CSSPropertyBottom,
    CSSPropertyBoxShadow,
    CSSPropertyBoxSizing,
    CSSPropertyCaptionSide,
    CSSPropertyClear,
    CSSPropertyClip,
    CSSPropertyColor,
    CSSPropertyCursor,
    CSSPropertyDirection,
    CSSPropertyDisplay,
    CSSPropertyEmptyCells,
    CSSPropertyFloat,
    CSSPropertyFontFamily,
    CSSPropertyFontSize,
    CSSPropertyFontStyle,
    CSSPropertyFontVariant,
    CSSPropertyFontWeight,
    CSSPropertyHeight,
    CSSPropertyLeft,
    CSSPropertyLetterSpacing,
    CSSPropertyLineHeight,
    CSSPropertyListStyleImage,
    CSSPropertyListStylePosition,
    CSSPropertyListStyleType,
    CSSPropertyMarginBottom,
    CSSPropertyMarginLeft,
    CSSPropertyMarginRight,
    CSSPropertyMarginTop,
    CSSPropertyMaxHeight,
    CSSPropertyMaxWidth,
    CSSPropertyMinHeight,
    CSSPropertyMinWidth,
    CSSPropertyOpacity,
    CSSPropertyOrphans,
    CSSPropertyOutlineColor,
    CSSPropertyOutlineStyle,
    CSSPropertyOutlineWidth,
    CSSPropertyOverflowX,
    CSSPropertyOverflowY,
    CSSPropertyPaddingBottom,
    CSSPropertyPaddingLeft,
    CSSPropertyPaddingRight,
    CSSPropertyPaddingTop,
    CSSPropertyPageBreakAfter,
    CSSPropertyPageBreakBefore,
    CSSPropertyPageBreakInside,
    CSSPropertyPointerEvents,
    CSSPropertyPosition,
    CSSPropertyResize,
    CSSPropertyRight,
    CSSPropertySpeak,
    CSSPropertyTableLayout,
    CSSPropertyTextAlign,
    CSSPropertyTextDecoration,
    CSSPropertyTextIndent,
    CSSPropertyTextRendering,
    CSSPropertyTextShadow,
    CSSPropertyTextOverflow,
    CSSPropertyTextTransform,
    CSSPropertyTop,
    CSSPropertyUnicodeBidi,
    CSSPropertyVerticalAlign,
    CSSPropertyVisibility,
    CSSPropertyWhiteSpace,
    CSSPropertyWidows,
    CSSPropertyWidth,
    CSSPropertyWordBreak,
    CSSPropertyWordSpacing,
    CSSPropertyWordWrap,
    CSSPropertyZIndex,
    CSSPropertyZoom,

    CSSPropertyWebkitAnimationDelay,
    CSSPropertyWebkitAnimationDirection,
    CSSPropertyWebkitAnimationDuration,
    CSSPropertyWebkitAnimationFillMode,
    CSSPropertyWebkitAnimationIterationCount,
    CSSPropertyWebkitAnimationName,
    CSSPropertyWebkitAnimationPlayState,
    CSSPropertyWebkitAnimationTimingFunction,
    CSSPropertyWebkitAppearance,
    CSSPropertyWebkitBackfaceVisibility,
    CSSPropertyWebkitBackgroundClip,
    CSSPropertyWebkitBackgroundComposite,
    CSSPropertyWebkitBackgroundOrigin,
    CSSPropertyWebkitBackgroundSize,
    CSSPropertyWebkitBorderFit,
    CSSPropertyWebkitBorderHorizontalSpacing,
    CSSPropertyWebkitBorderImage,
    CSSPropertyWebkitBorderVerticalSpacing,
    CSSPropertyWebkitBoxAlign,
    CSSPropertyWebkitBoxDirection,
    CSSPropertyWebkitBoxFlex,
    CSSPropertyWebkitBoxFlexGroup,
    CSSPropertyWebkitBoxLines,
    CSSPropertyWebkitBoxOrdinalGroup,
    CSSPropertyWebkitBoxOrient,
    CSSPropertyWebkitBoxPack,
    CSSPropertyWebkitBoxReflect,
    CSSPropertyWebkitBoxShadow,
    CSSPropertyWebkitColorCorrection,
    CSSPropertyWebkitColumnBreakAfter,
    CSSPropertyWebkitColumnBreakBefore,
    CSSPropertyWebkitColumnBreakInside,
    CSSPropertyWebkitColumnCount,
    CSSPropertyWebkitColumnGap,
    CSSPropertyWebkitColumnRuleColor,
    CSSPropertyWebkitColumnRuleStyle,
    CSSPropertyWebkitColumnRuleWidth,
    CSSPropertyWebkitColumnSpan,
    CSSPropertyWebkitColumnWidth,
#if ENABLE(DASHBOARD_SUPPORT)
    CSSPropertyWebkitDashboardRegion,
#endif
    CSSPropertyWebkitFontSmoothing,
    CSSPropertyWebkitHighlight,
    CSSPropertyWebkitHyphenateCharacter,
    CSSPropertyWebkitHyphenateLimitAfter,
    CSSPropertyWebkitHyphenateLimitBefore,
    CSSPropertyWebkitHyphens,
    CSSPropertyWebkitLineBoxContain,
    CSSPropertyWebkitLineBreak,
    CSSPropertyWebkitLineClamp,
    CSSPropertyWebkitLocale,
    CSSPropertyWebkitMarginBeforeCollapse,
    CSSPropertyWebkitMarginAfterCollapse,
    CSSPropertyWebkitMarqueeDirection,
    CSSPropertyWebkitMarqueeIncrement,
    CSSPropertyWebkitMarqueeRepetition,
    CSSPropertyWebkitMarqueeStyle,
    CSSPropertyWebkitMaskAttachment,
    CSSPropertyWebkitMaskBoxImage,
    CSSPropertyWebkitMaskClip,
    CSSPropertyWebkitMaskComposite,
    CSSPropertyWebkitMaskImage,
    CSSPropertyWebkitMaskOrigin,
    CSSPropertyWebkitMaskPosition,
    CSSPropertyWebkitMaskRepeat,
    CSSPropertyWebkitMaskSize,
    CSSPropertyWebkitNbspMode,
    CSSPropertyWebkitPerspective,
    CSSPropertyWebkitPerspectiveOrigin,
    CSSPropertyWebkitRtlOrdering,
    CSSPropertyWebkitTextCombine,
    CSSPropertyWebkitTextDecorationsInEffect,
    CSSPropertyWebkitTextEmphasisColor,
    CSSPropertyWebkitTextEmphasisPosition,
    CSSPropertyWebkitTextEmphasisStyle,
    CSSPropertyWebkitTextFillColor,
    CSSPropertyWebkitTextOrientation,
    CSSPropertyWebkitTextSecurity,
    CSSPropertyWebkitTextStrokeColor,
    CSSPropertyWebkitTextStrokeWidth,
    CSSPropertyWebkitTransform,
    CSSPropertyWebkitTransformOrigin,
    CSSPropertyWebkitTransformStyle,
    CSSPropertyWebkitTransitionDelay,
    CSSPropertyWebkitTransitionDuration,
    CSSPropertyWebkitTransitionProperty,
    CSSPropertyWebkitTransitionTimingFunction,
    CSSPropertyWebkitUserDrag,
    CSSPropertyWebkitUserModify,
    CSSPropertyWebkitUserSelect,
    CSSPropertyWebkitWritingMode

#if ENABLE(SVG)
    ,
    CSSPropertyClipPath,
    CSSPropertyClipRule,
    CSSPropertyMask,
    CSSPropertyFilter,
    CSSPropertyFloodColor,
    CSSPropertyFloodOpacity,
    CSSPropertyLightingColor,
    CSSPropertyStopColor,
    CSSPropertyStopOpacity,
    CSSPropertyColorInterpolation,
    CSSPropertyColorInterpolationFilters,
    CSSPropertyColorRendering,
    CSSPropertyFill,
    CSSPropertyFillOpacity,
    CSSPropertyFillRule,
    CSSPropertyImageRendering,
    CSSPropertyMarkerEnd,
    CSSPropertyMarkerMid,
    CSSPropertyMarkerStart,
    CSSPropertyShapeRendering,
    CSSPropertyStroke,
    CSSPropertyStrokeDasharray,
    CSSPropertyStrokeDashoffset,
    CSSPropertyStrokeLinecap,
    CSSPropertyStrokeLinejoin,
    CSSPropertyStrokeMiterlimit,
    CSSPropertyStrokeOpacity,
    CSSPropertyStrokeWidth,
    CSSPropertyAlignmentBaseline,
    CSSPropertyBaselineShift,
    CSSPropertyDominantBaseline,
    CSSPropertyKerning,
    CSSPropertyTextAnchor,
    CSSPropertyWritingMode,
    CSSPropertyGlyphOrientationHorizontal,
    CSSPropertyGlyphOrientationVertical,
    CSSPropertyWebkitSvgShadow,
    CSSPropertyVectorEffect
#endif
};

const unsigned numComputedProperties = WTF_ARRAY_LENGTH(computedProperties);

static int valueForRepeatRule(int rule)
{
    switch (rule) {
        case RepeatImageRule:
            return CSSValueRepeat;
        case RoundImageRule:
            return CSSValueRound;
        default:
            return CSSValueStretch;
    }
}
        
static PassRefPtr<CSSValue> valueForNinePieceImage(const NinePieceImage& image, CSSPrimitiveValueCache* primitiveValueCache)
{
    if (!image.hasImage())
        return primitiveValueCache->createIdentifierValue(CSSValueNone);
    
    // Image first.
    RefPtr<CSSValue> imageValue;
    if (image.image())
        imageValue = image.image()->cssValue();
    
    // Create the slices.
    RefPtr<CSSPrimitiveValue> top;
    if (image.slices().top().isPercent())
        top = primitiveValueCache->createValue(image.slices().top().value(), CSSPrimitiveValue::CSS_PERCENTAGE);
    else
        top = primitiveValueCache->createValue(image.slices().top().value(), CSSPrimitiveValue::CSS_NUMBER);
        
    RefPtr<CSSPrimitiveValue> right;
    if (image.slices().right().isPercent())
        right = primitiveValueCache->createValue(image.slices().right().value(), CSSPrimitiveValue::CSS_PERCENTAGE);
    else
        right = primitiveValueCache->createValue(image.slices().right().value(), CSSPrimitiveValue::CSS_NUMBER);
        
    RefPtr<CSSPrimitiveValue> bottom;
    if (image.slices().bottom().isPercent())
        bottom = primitiveValueCache->createValue(image.slices().bottom().value(), CSSPrimitiveValue::CSS_PERCENTAGE);
    else
        bottom = primitiveValueCache->createValue(image.slices().bottom().value(), CSSPrimitiveValue::CSS_NUMBER);
    
    RefPtr<CSSPrimitiveValue> left;
    if (image.slices().left().isPercent())
        left = primitiveValueCache->createValue(image.slices().left().value(), CSSPrimitiveValue::CSS_PERCENTAGE);
    else
        left = primitiveValueCache->createValue(image.slices().left().value(), CSSPrimitiveValue::CSS_NUMBER);

    RefPtr<Rect> rect = Rect::create();
    rect->setTop(top);
    rect->setRight(right);
    rect->setBottom(bottom);
    rect->setLeft(left);

    return CSSBorderImageValue::create(imageValue, rect, valueForRepeatRule(image.horizontalRule()), valueForRepeatRule(image.verticalRule()));
}

inline static PassRefPtr<CSSPrimitiveValue> zoomAdjustedPixelValue(int value, const RenderStyle* style, CSSPrimitiveValueCache* primitiveValueCache)
{
    return primitiveValueCache->createValue(adjustForAbsoluteZoom(value, style), CSSPrimitiveValue::CSS_PX);
}

inline static PassRefPtr<CSSPrimitiveValue> zoomAdjustedNumberValue(double value, const RenderStyle* style, CSSPrimitiveValueCache* primitiveValueCache)
{
    return primitiveValueCache->createValue(value / style->effectiveZoom(), CSSPrimitiveValue::CSS_NUMBER);
}

static PassRefPtr<CSSValue> zoomAdjustedPixelValueForLength(const Length& length, const RenderStyle* style, CSSPrimitiveValueCache* primitiveValueCache)
{
    if (length.isFixed())
        return zoomAdjustedPixelValue(length.value(), style, primitiveValueCache);
    return primitiveValueCache->createValue(length);
}

static PassRefPtr<CSSValue> valueForReflection(const StyleReflection* reflection, const RenderStyle* style, CSSPrimitiveValueCache* primitiveValueCache)
{
    if (!reflection)
        return primitiveValueCache->createIdentifierValue(CSSValueNone);

    RefPtr<CSSPrimitiveValue> offset;
    if (reflection->offset().isPercent())
        offset = primitiveValueCache->createValue(reflection->offset().percent(), CSSPrimitiveValue::CSS_PERCENTAGE);
    else
        offset = zoomAdjustedPixelValue(reflection->offset().value(), style, primitiveValueCache);
    
    return CSSReflectValue::create(reflection->direction(), offset.release(), valueForNinePieceImage(reflection->mask(), primitiveValueCache));
}

static PassRefPtr<CSSValue> getPositionOffsetValue(RenderStyle* style, int propertyID, CSSPrimitiveValueCache* primitiveValueCache)
{
    if (!style)
        return 0;

    Length l;
    switch (propertyID) {
        case CSSPropertyLeft:
            l = style->left();
            break;
        case CSSPropertyRight:
            l = style->right();
            break;
        case CSSPropertyTop:
            l = style->top();
            break;
        case CSSPropertyBottom:
            l = style->bottom();
            break;
        default:
            return 0;
    }

    if (style->position() == AbsolutePosition || style->position() == FixedPosition) {
        if (l.type() == WebCore::Fixed)
            return zoomAdjustedPixelValue(l.value(), style, primitiveValueCache);
        return primitiveValueCache->createValue(l);
    }

    if (style->position() == RelativePosition)
        // FIXME: It's not enough to simply return "auto" values for one offset if the other side is defined.
        // In other words if left is auto and right is not auto, then left's computed value is negative right().
        // So we should get the opposite length unit and see if it is auto.
        return primitiveValueCache->createValue(l);

    return primitiveValueCache->createIdentifierValue(CSSValueAuto);
}

PassRefPtr<CSSPrimitiveValue> CSSComputedStyleDeclaration::currentColorOrValidColor(RenderStyle* style, const Color& color) const
{
    // This function does NOT look at visited information, so that computed style doesn't expose that.
    CSSPrimitiveValueCache* primitiveValueCache = m_node->document()->cssPrimitiveValueCache().get();
    if (!color.isValid())
        return primitiveValueCache->createColorValue(style->color().rgb());
    return primitiveValueCache->createColorValue(color.rgb());
}

static PassRefPtr<CSSValue> getBorderRadiusCornerValue(LengthSize radius, const RenderStyle* style, CSSPrimitiveValueCache* primitiveValueCache)
{
    RefPtr<CSSValueList> list = CSSValueList::createSpaceSeparated();
    if (radius.width() == radius.height()) {
        if (radius.width().type() == Percent)
            return primitiveValueCache->createValue(radius.width().percent(), CSSPrimitiveValue::CSS_PERCENTAGE);
        return zoomAdjustedPixelValue(radius.width().value(), style, primitiveValueCache);
    }
    if (radius.width().type() == Percent)
        list->append(primitiveValueCache->createValue(radius.width().percent(), CSSPrimitiveValue::CSS_PERCENTAGE));
    else
        list->append(zoomAdjustedPixelValue(radius.width().value(), style, primitiveValueCache));
    if (radius.height().type() == Percent)
        list->append(primitiveValueCache->createValue(radius.height().percent(), CSSPrimitiveValue::CSS_PERCENTAGE));
    else
        list->append(zoomAdjustedPixelValue(radius.height().value(), style, primitiveValueCache));
    return list.release();
}

static IntRect sizingBox(RenderObject* renderer)
{
    if (!renderer->isBox())
        return IntRect();
    
    RenderBox* box = toRenderBox(renderer);
    return box->style()->boxSizing() == CONTENT_BOX ? box->contentBoxRect() : box->borderBoxRect();
}

static inline bool hasCompositedLayer(RenderObject* renderer)
{
    return renderer && renderer->hasLayer() && toRenderBoxModelObject(renderer)->layer()->isComposited();
}

static PassRefPtr<CSSValue> computedTransform(RenderObject* renderer, const RenderStyle* style, CSSPrimitiveValueCache* primitiveValueCache)
{
    if (!renderer || style->transform().operations().isEmpty())
        return primitiveValueCache->createIdentifierValue(CSSValueNone);
    
    IntRect box = sizingBox(renderer);

    TransformationMatrix transform;
    style->applyTransform(transform, box.size(), RenderStyle::ExcludeTransformOrigin);
    // Note that this does not flatten to an affine transform if ENABLE(3D_RENDERING) is off, by design.

    RefPtr<WebKitCSSTransformValue> transformVal;

    // FIXME: Need to print out individual functions (https://bugs.webkit.org/show_bug.cgi?id=23924)
    if (transform.isAffine()) {
        transformVal = WebKitCSSTransformValue::create(WebKitCSSTransformValue::MatrixTransformOperation);

        transformVal->append(primitiveValueCache->createValue(transform.a(), CSSPrimitiveValue::CSS_NUMBER));
        transformVal->append(primitiveValueCache->createValue(transform.b(), CSSPrimitiveValue::CSS_NUMBER));
        transformVal->append(primitiveValueCache->createValue(transform.c(), CSSPrimitiveValue::CSS_NUMBER));
        transformVal->append(primitiveValueCache->createValue(transform.d(), CSSPrimitiveValue::CSS_NUMBER));
        transformVal->append(zoomAdjustedNumberValue(transform.e(), style, primitiveValueCache));
        transformVal->append(zoomAdjustedNumberValue(transform.f(), style, primitiveValueCache));
    } else {
        transformVal = WebKitCSSTransformValue::create(WebKitCSSTransformValue::Matrix3DTransformOperation);

        transformVal->append(primitiveValueCache->createValue(transform.m11(), CSSPrimitiveValue::CSS_NUMBER));
        transformVal->append(primitiveValueCache->createValue(transform.m12(), CSSPrimitiveValue::CSS_NUMBER));
        transformVal->append(primitiveValueCache->createValue(transform.m13(), CSSPrimitiveValue::CSS_NUMBER));
        transformVal->append(primitiveValueCache->createValue(transform.m14(), CSSPrimitiveValue::CSS_NUMBER));

        transformVal->append(primitiveValueCache->createValue(transform.m21(), CSSPrimitiveValue::CSS_NUMBER));
        transformVal->append(primitiveValueCache->createValue(transform.m22(), CSSPrimitiveValue::CSS_NUMBER));
        transformVal->append(primitiveValueCache->createValue(transform.m23(), CSSPrimitiveValue::CSS_NUMBER));
        transformVal->append(primitiveValueCache->createValue(transform.m24(), CSSPrimitiveValue::CSS_NUMBER));

        transformVal->append(primitiveValueCache->createValue(transform.m31(), CSSPrimitiveValue::CSS_NUMBER));
        transformVal->append(primitiveValueCache->createValue(transform.m32(), CSSPrimitiveValue::CSS_NUMBER));
        transformVal->append(primitiveValueCache->createValue(transform.m33(), CSSPrimitiveValue::CSS_NUMBER));
        transformVal->append(primitiveValueCache->createValue(transform.m34(), CSSPrimitiveValue::CSS_NUMBER));

        transformVal->append(zoomAdjustedNumberValue(transform.m41(), style, primitiveValueCache));
        transformVal->append(zoomAdjustedNumberValue(transform.m42(), style, primitiveValueCache));
        transformVal->append(zoomAdjustedNumberValue(transform.m43(), style, primitiveValueCache));
        transformVal->append(primitiveValueCache->createValue(transform.m44(), CSSPrimitiveValue::CSS_NUMBER));
    }

    RefPtr<CSSValueList> list = CSSValueList::createSpaceSeparated();
    list->append(transformVal);

    return list.release();
}

static PassRefPtr<CSSValue> getDelayValue(const AnimationList* animList, CSSPrimitiveValueCache* primitiveValueCache)
{
    RefPtr<CSSValueList> list = CSSValueList::createCommaSeparated();
    if (animList) {
        for (size_t i = 0; i < animList->size(); ++i)
            list->append(primitiveValueCache->createValue(animList->animation(i)->delay(), CSSPrimitiveValue::CSS_S));
    } else {
        // Note that initialAnimationDelay() is used for both transitions and animations
        list->append(primitiveValueCache->createValue(Animation::initialAnimationDelay(), CSSPrimitiveValue::CSS_S));
    }
    return list.release();
}

static PassRefPtr<CSSValue> getDurationValue(const AnimationList* animList, CSSPrimitiveValueCache* primitiveValueCache)
{
    RefPtr<CSSValueList> list = CSSValueList::createCommaSeparated();
    if (animList) {
        for (size_t i = 0; i < animList->size(); ++i)
            list->append(primitiveValueCache->createValue(animList->animation(i)->duration(), CSSPrimitiveValue::CSS_S));
    } else {
        // Note that initialAnimationDuration() is used for both transitions and animations
        list->append(primitiveValueCache->createValue(Animation::initialAnimationDuration(), CSSPrimitiveValue::CSS_S));
    }
    return list.release();
}

static PassRefPtr<CSSValue> getTimingFunctionValue(const AnimationList* animList)
{
    RefPtr<CSSValueList> list = CSSValueList::createCommaSeparated();
    if (animList) {
        for (size_t i = 0; i < animList->size(); ++i) {
            const TimingFunction* tf = animList->animation(i)->timingFunction().get();
            if (tf->isCubicBezierTimingFunction()) {
                const CubicBezierTimingFunction* ctf = static_cast<const CubicBezierTimingFunction*>(tf);
                list->append(CSSCubicBezierTimingFunctionValue::create(ctf->x1(), ctf->y1(), ctf->x2(), ctf->y2()));
            } else if (tf->isStepsTimingFunction()) {
                const StepsTimingFunction* stf = static_cast<const StepsTimingFunction*>(tf);
                list->append(CSSStepsTimingFunctionValue::create(stf->numberOfSteps(), stf->stepAtStart()));
            } else {
                list->append(CSSLinearTimingFunctionValue::create());
            }
        }
    } else {
        // Note that initialAnimationTimingFunction() is used for both transitions and animations
        RefPtr<TimingFunction> tf = Animation::initialAnimationTimingFunction();
        if (tf->isCubicBezierTimingFunction()) {
            const CubicBezierTimingFunction* ctf = static_cast<const CubicBezierTimingFunction*>(tf.get());
            list->append(CSSCubicBezierTimingFunctionValue::create(ctf->x1(), ctf->y1(), ctf->x2(), ctf->y2()));
        } else if (tf->isStepsTimingFunction()) {
            const StepsTimingFunction* stf = static_cast<const StepsTimingFunction*>(tf.get());
            list->append(CSSStepsTimingFunctionValue::create(stf->numberOfSteps(), stf->stepAtStart()));
        } else {
            list->append(CSSLinearTimingFunctionValue::create());
        }
    }
    return list.release();
}

static PassRefPtr<CSSValue> createLineBoxContainValue(CSSPrimitiveValueCache* primitiveValueCache, unsigned lineBoxContain)
{
    if (!lineBoxContain)
        return primitiveValueCache->createIdentifierValue(CSSValueNone);
    return CSSLineBoxContainValue::create(lineBoxContain);
}

CSSComputedStyleDeclaration::CSSComputedStyleDeclaration(PassRefPtr<Node> n, bool allowVisitedStyle, const String& pseudoElementName)
    : m_node(n)
    , m_allowVisitedStyle(allowVisitedStyle)
{
    unsigned nameWithoutColonsStart = pseudoElementName[0] == ':' ? (pseudoElementName[1] == ':' ? 2 : 1) : 0;
    m_pseudoElementSpecifier = CSSSelector::pseudoId(CSSSelector::parsePseudoType(
        AtomicString(pseudoElementName.substring(nameWithoutColonsStart))));
}

CSSComputedStyleDeclaration::~CSSComputedStyleDeclaration()
{
}

String CSSComputedStyleDeclaration::cssText() const
{
    String result("");

    for (unsigned i = 0; i < numComputedProperties; i++) {
        if (i)
            result += " ";
        result += getPropertyName(static_cast<CSSPropertyID>(computedProperties[i]));
        result += ": ";
        result += getPropertyValue(computedProperties[i]);
        result += ";";
    }

    return result;
}

void CSSComputedStyleDeclaration::setCssText(const String&, ExceptionCode& ec)
{
    ec = NO_MODIFICATION_ALLOWED_ERR;
}

static int cssIdentifierForFontSizeKeyword(int keywordSize)
{
    ASSERT_ARG(keywordSize, keywordSize);
    ASSERT_ARG(keywordSize, keywordSize <= 8);
    return CSSValueXxSmall + keywordSize - 1;
}

PassRefPtr<CSSValue> CSSComputedStyleDeclaration::getFontSizeCSSValuePreferringKeyword() const
{
    if (!m_node)
        return 0;

    m_node->document()->updateLayoutIgnorePendingStylesheets();

    RefPtr<RenderStyle> style = m_node->computedStyle(m_pseudoElementSpecifier);
    if (!style)
        return 0;
    
    CSSPrimitiveValueCache* primitiveValueCache = m_node->document()->cssPrimitiveValueCache().get();

    if (int keywordSize = style->fontDescription().keywordSize())
        return primitiveValueCache->createIdentifierValue(cssIdentifierForFontSizeKeyword(keywordSize));


    return zoomAdjustedPixelValue(style->fontDescription().computedPixelSize(), style.get(), primitiveValueCache);
}

bool CSSComputedStyleDeclaration::useFixedFontDefaultSize() const
{
    if (!m_node)
        return false;

    RefPtr<RenderStyle> style = m_node->computedStyle(m_pseudoElementSpecifier);
    if (!style)
        return false;

    return style->fontDescription().useFixedDefaultSize();
}

PassRefPtr<CSSValue> CSSComputedStyleDeclaration::valueForShadow(const ShadowData* shadow, int id, RenderStyle* style) const
{
    CSSPrimitiveValueCache* primitiveValueCache = m_node->document()->cssPrimitiveValueCache().get();
    if (!shadow)
        return primitiveValueCache->createIdentifierValue(CSSValueNone);

    CSSPropertyID propertyID = static_cast<CSSPropertyID>(id);

    RefPtr<CSSValueList> list = CSSValueList::createCommaSeparated();
    for (const ShadowData* s = shadow; s; s = s->next()) {
        RefPtr<CSSPrimitiveValue> x = zoomAdjustedPixelValue(s->x(), style, primitiveValueCache);
        RefPtr<CSSPrimitiveValue> y = zoomAdjustedPixelValue(s->y(), style, primitiveValueCache);
        RefPtr<CSSPrimitiveValue> blur = zoomAdjustedPixelValue(s->blur(), style, primitiveValueCache);
        RefPtr<CSSPrimitiveValue> spread = propertyID == CSSPropertyTextShadow ? PassRefPtr<CSSPrimitiveValue>() : zoomAdjustedPixelValue(s->spread(), style, primitiveValueCache);
        RefPtr<CSSPrimitiveValue> style = propertyID == CSSPropertyTextShadow || s->style() == Normal ? PassRefPtr<CSSPrimitiveValue>() : primitiveValueCache->createIdentifierValue(CSSValueInset);
        RefPtr<CSSPrimitiveValue> color = primitiveValueCache->createColorValue(s->color().rgb());
        list->prepend(ShadowValue::create(x.release(), y.release(), blur.release(), spread.release(), style.release(), color.release()));
    }
    return list.release();
}

PassRefPtr<CSSValue> CSSComputedStyleDeclaration::getPropertyCSSValue(int propertyID) const
{
    return getPropertyCSSValue(propertyID, UpdateLayout);
}

static int identifierForFamily(const AtomicString& family)
{
    DEFINE_STATIC_LOCAL(AtomicString, cursiveFamily, ("-webkit-cursive")); 
    DEFINE_STATIC_LOCAL(AtomicString, fantasyFamily, ("-webkit-fantasy")); 
    DEFINE_STATIC_LOCAL(AtomicString, monospaceFamily, ("-webkit-monospace")); 
    DEFINE_STATIC_LOCAL(AtomicString, sansSerifFamily, ("-webkit-sans-serif")); 
    DEFINE_STATIC_LOCAL(AtomicString, serifFamily, ("-webkit-serif")); 
    if (family == cursiveFamily)
        return CSSValueCursive;
    if (family == fantasyFamily)
        return CSSValueFantasy;
    if (family == monospaceFamily)
        return CSSValueMonospace;
    if (family == sansSerifFamily)
        return CSSValueSansSerif;
    if (family == serifFamily)
        return CSSValueSerif;
    return 0;
}

static PassRefPtr<CSSPrimitiveValue> valueForFamily(const AtomicString& family, CSSPrimitiveValueCache* primitiveValueCache)
{
    if (int familyIdentifier = identifierForFamily(family))
        return primitiveValueCache->createIdentifierValue(familyIdentifier);
    return primitiveValueCache->createValue(family.string(), CSSPrimitiveValue::CSS_STRING);
}

static PassRefPtr<CSSValue> renderTextDecorationFlagsToCSSValue(int textDecoration, CSSPrimitiveValueCache* primitiveValueCache)
{
    RefPtr<CSSValueList> list = CSSValueList::createSpaceSeparated();
    if (textDecoration & UNDERLINE)
        list->append(primitiveValueCache->createIdentifierValue(CSSValueUnderline));
    if (textDecoration & OVERLINE)
        list->append(primitiveValueCache->createIdentifierValue(CSSValueOverline));
    if (textDecoration & LINE_THROUGH)
        list->append(primitiveValueCache->createIdentifierValue(CSSValueLineThrough));
    if (textDecoration & BLINK)
        list->append(primitiveValueCache->createIdentifierValue(CSSValueBlink));

    if (!list->length())
        return primitiveValueCache->createIdentifierValue(CSSValueNone);
    return list;
}

static PassRefPtr<CSSValue> fillRepeatToCSSValue(EFillRepeat xRepeat, EFillRepeat yRepeat, CSSPrimitiveValueCache* primitiveValueCache)
{
    // For backwards compatibility, if both values are equal, just return one of them. And
    // if the two values are equivalent to repeat-x or repeat-y, just return the shorthand.
    if (xRepeat == yRepeat)
        return primitiveValueCache->createValue(xRepeat);
    if (xRepeat == RepeatFill && yRepeat == NoRepeatFill)
        return primitiveValueCache->createIdentifierValue(CSSValueRepeatX);
    if (xRepeat == NoRepeatFill && yRepeat == RepeatFill)
        return primitiveValueCache->createIdentifierValue(CSSValueRepeatY);

    RefPtr<CSSValueList> list = CSSValueList::createSpaceSeparated();
    list->append(primitiveValueCache->createValue(xRepeat));
    list->append(primitiveValueCache->createValue(yRepeat));
    return list.release();
}

static PassRefPtr<CSSValue> fillSizeToCSSValue(const FillSize& fillSize, CSSPrimitiveValueCache* primitiveValueCache)
{
    if (fillSize.type == Contain)
        return primitiveValueCache->createIdentifierValue(CSSValueContain);

    if (fillSize.type == Cover)
        return primitiveValueCache->createIdentifierValue(CSSValueCover);

    RefPtr<CSSValueList> list = CSSValueList::createSpaceSeparated();
    list->append(primitiveValueCache->createValue(fillSize.size.width()));
    list->append(primitiveValueCache->createValue(fillSize.size.height()));
    return list.release();
}

static PassRefPtr<CSSValue> contentToCSSValue(const RenderStyle* style, CSSPrimitiveValueCache* primitiveValueCache)
{
    RefPtr<CSSValueList> list = CSSValueList::createSpaceSeparated();
    for (const ContentData* contentData = style->contentData(); contentData; contentData = contentData->next()) {
        if (contentData->isCounter()) {
            const CounterContent* counter = contentData->counter();
            ASSERT(counter);
            list->append(primitiveValueCache->createValue(counter->identifier(), CSSPrimitiveValue::CSS_COUNTER_NAME));
        } else if (contentData->isImage()) {
            const StyleImage* image = contentData->image();
            ASSERT(image);
            list->append(image->cssValue());
        } else if (contentData->isText())
            list->append(primitiveValueCache->createValue(contentData->text(), CSSPrimitiveValue::CSS_STRING));
    }
    return list.release();
}

static PassRefPtr<CSSValue> counterToCSSValue(const RenderStyle* style, int propertyID, CSSPrimitiveValueCache* primitiveValueCache)
{
    const CounterDirectiveMap* map = style->counterDirectives();
    if (!map)
        return 0;

    RefPtr<CSSValueList> list = CSSValueList::createSpaceSeparated();
    for (CounterDirectiveMap::const_iterator it = map->begin(); it != map->end(); ++it) {
        list->append(primitiveValueCache->createValue(it->first.get(), CSSPrimitiveValue::CSS_STRING));
        short number = propertyID == CSSPropertyCounterIncrement ? it->second.m_incrementValue : it->second.m_resetValue;
        list->append(primitiveValueCache->createValue((double)number, CSSPrimitiveValue::CSS_NUMBER));
    }
    return list.release();
}

static void logUnimplementedPropertyID(int propertyID)
{
    DEFINE_STATIC_LOCAL(HashSet<int>, propertyIDSet, ());
    if (!propertyIDSet.add(propertyID).second)
        return;

    LOG_ERROR("WebKit does not yet implement getComputedStyle for '%s'.", getPropertyName(static_cast<CSSPropertyID>(propertyID)));
} 

PassRefPtr<CSSValue> CSSComputedStyleDeclaration::getPropertyCSSValue(int propertyID, EUpdateLayout updateLayout) const
{
    Node* node = m_node.get();
    if (!node)
        return 0;

    // Make sure our layout is up to date before we allow a query on these attributes.
    if (updateLayout)
        node->document()->updateLayoutIgnorePendingStylesheets();

    RenderObject* renderer = node->renderer();

    RefPtr<RenderStyle> style;
    if (renderer && hasCompositedLayer(renderer) && AnimationController::supportsAcceleratedAnimationOfProperty(static_cast<CSSPropertyID>(propertyID))) {
        style = renderer->animation()->getAnimatedStyleForRenderer(renderer);
        if (m_pseudoElementSpecifier) {
            // FIXME: This cached pseudo style will only exist if the animation has been run at least once.
            style = style->getCachedPseudoStyle(m_pseudoElementSpecifier);
        }
    } else
        style = node->computedStyle(m_pseudoElementSpecifier);

    if (!style)
        return 0;
    
    CSSPrimitiveValueCache* primitiveValueCache = node->document()->cssPrimitiveValueCache().get();

    propertyID = CSSProperty::resolveDirectionAwareProperty(propertyID, style->direction(), style->writingMode());

    switch (static_cast<CSSPropertyID>(propertyID)) {
        case CSSPropertyInvalid:
            break;

        case CSSPropertyBackgroundColor:
            return primitiveValueCache->createColorValue(m_allowVisitedStyle? style->visitedDependentColor(CSSPropertyBackgroundColor).rgb() : style->backgroundColor().rgb());
        case CSSPropertyBackgroundImage:
        case CSSPropertyWebkitMaskImage: {
            const FillLayer* layers = propertyID == CSSPropertyWebkitMaskImage ? style->maskLayers() : style->backgroundLayers();
            if (!layers)
                return primitiveValueCache->createIdentifierValue(CSSValueNone);

            if (!layers->next()) {
                if (layers->image())
                    return layers->image()->cssValue();

                return primitiveValueCache->createIdentifierValue(CSSValueNone);
            }

            RefPtr<CSSValueList> list = CSSValueList::createCommaSeparated();
            for (const FillLayer* currLayer = layers; currLayer; currLayer = currLayer->next()) {
                if (currLayer->image())
                    list->append(currLayer->image()->cssValue());
                else
                    list->append(primitiveValueCache->createIdentifierValue(CSSValueNone));
            }
            return list.release();
        }
        case CSSPropertyBackgroundSize:
        case CSSPropertyWebkitBackgroundSize:
        case CSSPropertyWebkitMaskSize: {
            const FillLayer* layers = propertyID == CSSPropertyWebkitMaskSize ? style->maskLayers() : style->backgroundLayers();
            if (!layers->next())
                return fillSizeToCSSValue(layers->size(), primitiveValueCache);

            RefPtr<CSSValueList> list = CSSValueList::createCommaSeparated();
            for (const FillLayer* currLayer = layers; currLayer; currLayer = currLayer->next())
                list->append(fillSizeToCSSValue(currLayer->size(), primitiveValueCache));

            return list.release();
        }
        case CSSPropertyBackgroundRepeat:
        case CSSPropertyWebkitMaskRepeat: {
            const FillLayer* layers = propertyID == CSSPropertyWebkitMaskRepeat ? style->maskLayers() : style->backgroundLayers();
            if (!layers->next())
                return fillRepeatToCSSValue(layers->repeatX(), layers->repeatY(), primitiveValueCache);

            RefPtr<CSSValueList> list = CSSValueList::createCommaSeparated();
            for (const FillLayer* currLayer = layers; currLayer; currLayer = currLayer->next())
                list->append(fillRepeatToCSSValue(currLayer->repeatX(), currLayer->repeatY(), primitiveValueCache));

            return list.release();
        }
        case CSSPropertyWebkitBackgroundComposite:
        case CSSPropertyWebkitMaskComposite: {
            const FillLayer* layers = propertyID == CSSPropertyWebkitMaskComposite ? style->maskLayers() : style->backgroundLayers();
            if (!layers->next())
                return primitiveValueCache->createValue(layers->composite());

            RefPtr<CSSValueList> list = CSSValueList::createCommaSeparated();
            for (const FillLayer* currLayer = layers; currLayer; currLayer = currLayer->next())
                list->append(primitiveValueCache->createValue(currLayer->composite()));

            return list.release();
        }
        case CSSPropertyBackgroundAttachment:
        case CSSPropertyWebkitMaskAttachment: {
            const FillLayer* layers = propertyID == CSSPropertyWebkitMaskAttachment ? style->maskLayers() : style->backgroundLayers();
            if (!layers->next())
                return primitiveValueCache->createValue(layers->attachment());

            RefPtr<CSSValueList> list = CSSValueList::createCommaSeparated();
            for (const FillLayer* currLayer = layers; currLayer; currLayer = currLayer->next())
                list->append(primitiveValueCache->createValue(currLayer->attachment()));

            return list.release();
        }
        case CSSPropertyBackgroundClip:
        case CSSPropertyBackgroundOrigin:
        case CSSPropertyWebkitBackgroundClip:
        case CSSPropertyWebkitBackgroundOrigin:
        case CSSPropertyWebkitMaskClip:
        case CSSPropertyWebkitMaskOrigin: {
            const FillLayer* layers = (propertyID == CSSPropertyWebkitMaskClip || propertyID == CSSPropertyWebkitMaskOrigin) ? style->maskLayers() : style->backgroundLayers();
            bool isClip = propertyID == CSSPropertyBackgroundClip || propertyID == CSSPropertyWebkitBackgroundClip || propertyID == CSSPropertyWebkitMaskClip;
            if (!layers->next()) {
                EFillBox box = isClip ? layers->clip() : layers->origin();
                return primitiveValueCache->createValue(box);
            }
            
            RefPtr<CSSValueList> list = CSSValueList::createCommaSeparated();
            for (const FillLayer* currLayer = layers; currLayer; currLayer = currLayer->next()) {
                EFillBox box = isClip ? currLayer->clip() : currLayer->origin();
                list->append(primitiveValueCache->createValue(box));
            }

            return list.release();
        }
        case CSSPropertyBackgroundPosition:
        case CSSPropertyWebkitMaskPosition: {
            const FillLayer* layers = propertyID == CSSPropertyWebkitMaskPosition ? style->maskLayers() : style->backgroundLayers();
            if (!layers->next()) {
                RefPtr<CSSValueList> list = CSSValueList::createSpaceSeparated();
                list->append(primitiveValueCache->createValue(layers->xPosition()));
                list->append(primitiveValueCache->createValue(layers->yPosition()));
                return list.release();
            }

            RefPtr<CSSValueList> list = CSSValueList::createCommaSeparated();
            for (const FillLayer* currLayer = layers; currLayer; currLayer = currLayer->next()) {
                RefPtr<CSSValueList> positionList = CSSValueList::createSpaceSeparated();
                positionList->append(primitiveValueCache->createValue(currLayer->xPosition()));
                positionList->append(primitiveValueCache->createValue(currLayer->yPosition()));
                list->append(positionList);
            }

            return list.release();
        }
        case CSSPropertyBackgroundPositionX:
        case CSSPropertyWebkitMaskPositionX: {
            const FillLayer* layers = propertyID == CSSPropertyWebkitMaskPositionX ? style->maskLayers() : style->backgroundLayers();
            if (!layers->next())
                return primitiveValueCache->createValue(layers->xPosition());

            RefPtr<CSSValueList> list = CSSValueList::createCommaSeparated();
            for (const FillLayer* currLayer = layers; currLayer; currLayer = currLayer->next())
                list->append(primitiveValueCache->createValue(currLayer->xPosition()));

            return list.release();
        }
        case CSSPropertyBackgroundPositionY:
        case CSSPropertyWebkitMaskPositionY: {
            const FillLayer* layers = propertyID == CSSPropertyWebkitMaskPositionY ? style->maskLayers() : style->backgroundLayers();
            if (!layers->next())
                return primitiveValueCache->createValue(layers->yPosition());

            RefPtr<CSSValueList> list = CSSValueList::createCommaSeparated();
            for (const FillLayer* currLayer = layers; currLayer; currLayer = currLayer->next())
                list->append(primitiveValueCache->createValue(currLayer->yPosition()));

            return list.release();
        }
        case CSSPropertyBorderCollapse:
            if (style->borderCollapse())
                return primitiveValueCache->createIdentifierValue(CSSValueCollapse);
            return primitiveValueCache->createIdentifierValue(CSSValueSeparate);
        case CSSPropertyBorderSpacing: {
            RefPtr<CSSValueList> list = CSSValueList::createSpaceSeparated();
            list->append(zoomAdjustedPixelValue(style->horizontalBorderSpacing(), style.get(), primitiveValueCache));
            list->append(zoomAdjustedPixelValue(style->verticalBorderSpacing(), style.get(), primitiveValueCache));
            return list.release();
        }  
        case CSSPropertyWebkitBorderHorizontalSpacing:
            return zoomAdjustedPixelValue(style->horizontalBorderSpacing(), style.get(), primitiveValueCache);
        case CSSPropertyWebkitBorderVerticalSpacing:
            return zoomAdjustedPixelValue(style->verticalBorderSpacing(), style.get(), primitiveValueCache);
        case CSSPropertyBorderTopColor:
            return m_allowVisitedStyle ? primitiveValueCache->createColorValue(style->visitedDependentColor(CSSPropertyBorderTopColor).rgb()) : currentColorOrValidColor(style.get(), style->borderTopColor());
        case CSSPropertyBorderRightColor:
            return m_allowVisitedStyle ? primitiveValueCache->createColorValue(style->visitedDependentColor(CSSPropertyBorderRightColor).rgb()) : currentColorOrValidColor(style.get(), style->borderRightColor());
        case CSSPropertyBorderBottomColor:
            return m_allowVisitedStyle ? primitiveValueCache->createColorValue(style->visitedDependentColor(CSSPropertyBorderBottomColor).rgb()) : currentColorOrValidColor(style.get(), style->borderBottomColor());
        case CSSPropertyBorderLeftColor:
            return m_allowVisitedStyle ? primitiveValueCache->createColorValue(style->visitedDependentColor(CSSPropertyBorderLeftColor).rgb()) : currentColorOrValidColor(style.get(), style->borderLeftColor());
        case CSSPropertyBorderTopStyle:
            return primitiveValueCache->createValue(style->borderTopStyle());
        case CSSPropertyBorderRightStyle:
            return primitiveValueCache->createValue(style->borderRightStyle());
        case CSSPropertyBorderBottomStyle:
            return primitiveValueCache->createValue(style->borderBottomStyle());
        case CSSPropertyBorderLeftStyle:
            return primitiveValueCache->createValue(style->borderLeftStyle());
        case CSSPropertyBorderTopWidth:
            return zoomAdjustedPixelValue(style->borderTopWidth(), style.get(), primitiveValueCache);
        case CSSPropertyBorderRightWidth:
            return zoomAdjustedPixelValue(style->borderRightWidth(), style.get(), primitiveValueCache);
        case CSSPropertyBorderBottomWidth:
            return zoomAdjustedPixelValue(style->borderBottomWidth(), style.get(), primitiveValueCache);
        case CSSPropertyBorderLeftWidth:
            return zoomAdjustedPixelValue(style->borderLeftWidth(), style.get(), primitiveValueCache);
        case CSSPropertyBottom:
            return getPositionOffsetValue(style.get(), CSSPropertyBottom, primitiveValueCache);
        case CSSPropertyWebkitBoxAlign:
            return primitiveValueCache->createValue(style->boxAlign());
        case CSSPropertyWebkitBoxDirection:
            return primitiveValueCache->createValue(style->boxDirection());
        case CSSPropertyWebkitBoxFlex:
            return primitiveValueCache->createValue(style->boxFlex(), CSSPrimitiveValue::CSS_NUMBER);
        case CSSPropertyWebkitBoxFlexGroup:
            return primitiveValueCache->createValue(style->boxFlexGroup(), CSSPrimitiveValue::CSS_NUMBER);
        case CSSPropertyWebkitBoxLines:
            return primitiveValueCache->createValue(style->boxLines());
        case CSSPropertyWebkitBoxOrdinalGroup:
            return primitiveValueCache->createValue(style->boxOrdinalGroup(), CSSPrimitiveValue::CSS_NUMBER);
        case CSSPropertyWebkitBoxOrient:
            return primitiveValueCache->createValue(style->boxOrient());
        case CSSPropertyWebkitBoxPack: {
            EBoxAlignment boxPack = style->boxPack();
            ASSERT(boxPack != BSTRETCH);
            ASSERT(boxPack != BBASELINE);
            if (boxPack == BJUSTIFY || boxPack== BBASELINE)
                return 0;
            return primitiveValueCache->createValue(boxPack);
        }
        case CSSPropertyWebkitBoxReflect:
            return valueForReflection(style->boxReflect(), style.get(), primitiveValueCache);
        case CSSPropertyBoxShadow:
        case CSSPropertyWebkitBoxShadow:
            return valueForShadow(style->boxShadow(), propertyID, style.get());
        case CSSPropertyCaptionSide:
            return primitiveValueCache->createValue(style->captionSide());
        case CSSPropertyClear:
            return primitiveValueCache->createValue(style->clear());
        case CSSPropertyColor:
            return primitiveValueCache->createColorValue(m_allowVisitedStyle ? style->visitedDependentColor(CSSPropertyColor).rgb() : style->color().rgb());
        case CSSPropertyWebkitColumnCount:
            if (style->hasAutoColumnCount())
                return primitiveValueCache->createIdentifierValue(CSSValueAuto);
            return primitiveValueCache->createValue(style->columnCount(), CSSPrimitiveValue::CSS_NUMBER);
        case CSSPropertyWebkitColumnGap:
            if (style->hasNormalColumnGap())
                return primitiveValueCache->createIdentifierValue(CSSValueNormal);
            return zoomAdjustedPixelValue(style->columnGap(), style.get(), primitiveValueCache);
        case CSSPropertyWebkitColumnRuleColor:
            return m_allowVisitedStyle ? primitiveValueCache->createColorValue(style->visitedDependentColor(CSSPropertyOutlineColor).rgb()) : currentColorOrValidColor(style.get(), style->columnRuleColor());
        case CSSPropertyWebkitColumnRuleStyle:
            return primitiveValueCache->createValue(style->columnRuleStyle());
        case CSSPropertyWebkitColumnRuleWidth:
            return zoomAdjustedPixelValue(style->columnRuleWidth(), style.get(), primitiveValueCache);
        case CSSPropertyWebkitColumnSpan:
            if (style->columnSpan())
                return primitiveValueCache->createIdentifierValue(CSSValueAll);
            return primitiveValueCache->createValue(1, CSSPrimitiveValue::CSS_NUMBER);
        case CSSPropertyWebkitColumnBreakAfter:
            return primitiveValueCache->createValue(style->columnBreakAfter());
        case CSSPropertyWebkitColumnBreakBefore:
            return primitiveValueCache->createValue(style->columnBreakBefore());
        case CSSPropertyWebkitColumnBreakInside:
            return primitiveValueCache->createValue(style->columnBreakInside());
        case CSSPropertyWebkitColumnWidth:
            if (style->hasAutoColumnWidth())
                return primitiveValueCache->createIdentifierValue(CSSValueAuto);
            return zoomAdjustedPixelValue(style->columnWidth(), style.get(), primitiveValueCache);
        case CSSPropertyCursor: {
            RefPtr<CSSValueList> list;
            CursorList* cursors = style->cursors();
            if (cursors && cursors->size() > 0) {
                list = CSSValueList::createCommaSeparated();
                for (unsigned i = 0; i < cursors->size(); ++i)
                    if (StyleImage* image = cursors->at(i).image())
                        list->append(image->cssValue());
            }
            RefPtr<CSSValue> value = primitiveValueCache->createValue(style->cursor());
            if (list) {
                list->append(value);
                return list.release();
            }
            return value.release();
        }
        case CSSPropertyDirection:
            return primitiveValueCache->createValue(style->direction());
        case CSSPropertyDisplay:
            return primitiveValueCache->createValue(style->display());
        case CSSPropertyEmptyCells:
            return primitiveValueCache->createValue(style->emptyCells());
        case CSSPropertyFloat:
            return primitiveValueCache->createValue(style->floating());
        case CSSPropertyFontFamily: {
            const FontFamily& firstFamily = style->fontDescription().family();
            if (!firstFamily.next())
                return valueForFamily(firstFamily.family(), primitiveValueCache);
            RefPtr<CSSValueList> list = CSSValueList::createCommaSeparated();
            for (const FontFamily* family = &firstFamily; family; family = family->next())
                list->append(valueForFamily(family->family(), primitiveValueCache));
            return list.release();
        }
        case CSSPropertyFontSize:
            return zoomAdjustedPixelValue(style->fontDescription().computedPixelSize(), style.get(), primitiveValueCache);
        case CSSPropertyFontStyle:
            if (style->fontDescription().italic())
                return primitiveValueCache->createIdentifierValue(CSSValueItalic);
            return primitiveValueCache->createIdentifierValue(CSSValueNormal);
        case CSSPropertyFontVariant:
            if (style->fontDescription().smallCaps())
                return primitiveValueCache->createIdentifierValue(CSSValueSmallCaps);
            return primitiveValueCache->createIdentifierValue(CSSValueNormal);
        case CSSPropertyFontWeight:
            switch (style->fontDescription().weight()) {
                case FontWeight100:
                    return primitiveValueCache->createIdentifierValue(CSSValue100);
                case FontWeight200:
                    return primitiveValueCache->createIdentifierValue(CSSValue200);
                case FontWeight300:
                    return primitiveValueCache->createIdentifierValue(CSSValue300);
                case FontWeightNormal:
                    return primitiveValueCache->createIdentifierValue(CSSValueNormal);
                case FontWeight500:
                    return primitiveValueCache->createIdentifierValue(CSSValue500);
                case FontWeight600:
                    return primitiveValueCache->createIdentifierValue(CSSValue600);
                case FontWeightBold:
                    return primitiveValueCache->createIdentifierValue(CSSValueBold);
                case FontWeight800:
                    return primitiveValueCache->createIdentifierValue(CSSValue800);
                case FontWeight900:
                    return primitiveValueCache->createIdentifierValue(CSSValue900);
            }
            ASSERT_NOT_REACHED();
            return primitiveValueCache->createIdentifierValue(CSSValueNormal);
        case CSSPropertyHeight:
            if (renderer)
                return zoomAdjustedPixelValue(sizingBox(renderer).height(), style.get(), primitiveValueCache);
            return zoomAdjustedPixelValueForLength(style->height(), style.get(), primitiveValueCache);
        case CSSPropertyWebkitHighlight:
            if (style->highlight() == nullAtom)
                return primitiveValueCache->createIdentifierValue(CSSValueNone);
            return primitiveValueCache->createValue(style->highlight(), CSSPrimitiveValue::CSS_STRING);
        case CSSPropertyWebkitHyphens:
            return primitiveValueCache->createValue(style->hyphens());
        case CSSPropertyWebkitHyphenateCharacter:
            if (style->hyphenationString().isNull())
                return primitiveValueCache->createIdentifierValue(CSSValueAuto);
            return primitiveValueCache->createValue(style->hyphenationString(), CSSPrimitiveValue::CSS_STRING);
        case CSSPropertyWebkitHyphenateLimitAfter:
            if (style->hyphenationLimitAfter() < 0)
                return CSSPrimitiveValue::createIdentifier(CSSValueAuto);
            return CSSPrimitiveValue::create(style->hyphenationLimitAfter(), CSSPrimitiveValue::CSS_NUMBER);
        case CSSPropertyWebkitHyphenateLimitBefore:
            if (style->hyphenationLimitBefore() < 0)
                return CSSPrimitiveValue::createIdentifier(CSSValueAuto);
            return CSSPrimitiveValue::create(style->hyphenationLimitBefore(), CSSPrimitiveValue::CSS_NUMBER);
        case CSSPropertyWebkitBorderFit:
            if (style->borderFit() == BorderFitBorder)
                return primitiveValueCache->createIdentifierValue(CSSValueBorder);
            return primitiveValueCache->createIdentifierValue(CSSValueLines);
        case CSSPropertyLeft:
            return getPositionOffsetValue(style.get(), CSSPropertyLeft, primitiveValueCache);
        case CSSPropertyLetterSpacing:
            if (!style->letterSpacing())
                return primitiveValueCache->createIdentifierValue(CSSValueNormal);
            return zoomAdjustedPixelValue(style->letterSpacing(), style.get(), primitiveValueCache);
        case CSSPropertyWebkitLineClamp:
            if (style->lineClamp().isNone())
                return primitiveValueCache->createIdentifierValue(CSSValueNone);
            return primitiveValueCache->createValue(style->lineClamp().value(), style->lineClamp().isPercentage() ? CSSPrimitiveValue::CSS_PERCENTAGE : CSSPrimitiveValue::CSS_NUMBER);
        case CSSPropertyLineHeight: {
            Length length = style->lineHeight();
            if (length.isNegative())
                return primitiveValueCache->createIdentifierValue(CSSValueNormal);
            if (length.isPercent())
                // This is imperfect, because it doesn't include the zoom factor and the real computation
                // for how high to be in pixels does include things like minimum font size and the zoom factor.
                // On the other hand, since font-size doesn't include the zoom factor, we really can't do
                // that here either.
                return zoomAdjustedPixelValue(static_cast<int>(length.percent() * style->fontDescription().specifiedSize()) / 100, style.get(), primitiveValueCache);
            return zoomAdjustedPixelValue(length.value(), style.get(), primitiveValueCache);
        }
        case CSSPropertyListStyleImage:
            if (style->listStyleImage())
                return style->listStyleImage()->cssValue();
            return primitiveValueCache->createIdentifierValue(CSSValueNone);
        case CSSPropertyListStylePosition:
            return primitiveValueCache->createValue(style->listStylePosition());
        case CSSPropertyListStyleType:
            return primitiveValueCache->createValue(style->listStyleType());
        case CSSPropertyWebkitLocale:
            if (style->locale().isNull())
                return primitiveValueCache->createIdentifierValue(CSSValueAuto);
            return primitiveValueCache->createValue(style->locale(), CSSPrimitiveValue::CSS_STRING);
        case CSSPropertyMarginTop: {
            Length marginTop = style->marginTop();
            if (marginTop.isPercent())
                return primitiveValueCache->createValue(marginTop);
            return zoomAdjustedPixelValue(marginTop.value(), style.get(), primitiveValueCache);
        }
        case CSSPropertyMarginRight: {
            Length marginRight = style->marginRight();
            if (marginRight.isPercent())
                return primitiveValueCache->createValue(marginRight);
            return zoomAdjustedPixelValue(marginRight.value(), style.get(), primitiveValueCache);
        }
        case CSSPropertyMarginBottom: {
            Length marginBottom = style->marginBottom();
            if (marginBottom.isPercent())
                return primitiveValueCache->createValue(marginBottom);
            return zoomAdjustedPixelValue(marginBottom.value(), style.get(), primitiveValueCache);
        }
        case CSSPropertyMarginLeft: {
            Length marginLeft = style->marginLeft();
            if (marginLeft.isPercent())
                return primitiveValueCache->createValue(marginLeft);
            return zoomAdjustedPixelValue(marginLeft.value(), style.get(), primitiveValueCache);
        }
        case CSSPropertyWebkitMarqueeDirection:
            return primitiveValueCache->createValue(style->marqueeDirection());
        case CSSPropertyWebkitMarqueeIncrement:
            return primitiveValueCache->createValue(style->marqueeIncrement());
        case CSSPropertyWebkitMarqueeRepetition:
            if (style->marqueeLoopCount() < 0)
                return primitiveValueCache->createIdentifierValue(CSSValueInfinite);
            return primitiveValueCache->createValue(style->marqueeLoopCount(), CSSPrimitiveValue::CSS_NUMBER);
        case CSSPropertyWebkitMarqueeStyle:
            return primitiveValueCache->createValue(style->marqueeBehavior());
        case CSSPropertyWebkitUserModify:
            return primitiveValueCache->createValue(style->userModify());
        case CSSPropertyMaxHeight: {
            const Length& maxHeight = style->maxHeight();
            if (maxHeight.isFixed() && maxHeight.value() == undefinedLength)
                return primitiveValueCache->createIdentifierValue(CSSValueNone);
            return primitiveValueCache->createValue(maxHeight);
        }
        case CSSPropertyMaxWidth: {
            const Length& maxWidth = style->maxWidth();
            if (maxWidth.isFixed() && maxWidth.value() == undefinedLength)
                return primitiveValueCache->createIdentifierValue(CSSValueNone);
            return primitiveValueCache->createValue(maxWidth);
        }
        case CSSPropertyMinHeight:
            return primitiveValueCache->createValue(style->minHeight());
        case CSSPropertyMinWidth:
            return primitiveValueCache->createValue(style->minWidth());
        case CSSPropertyOpacity:
            return primitiveValueCache->createValue(style->opacity(), CSSPrimitiveValue::CSS_NUMBER);
        case CSSPropertyOrphans:
            return primitiveValueCache->createValue(style->orphans(), CSSPrimitiveValue::CSS_NUMBER);
        case CSSPropertyOutlineColor:
            return m_allowVisitedStyle ? primitiveValueCache->createColorValue(style->visitedDependentColor(CSSPropertyOutlineColor).rgb()) : currentColorOrValidColor(style.get(), style->outlineColor());
        case CSSPropertyOutlineOffset:
            return zoomAdjustedPixelValue(style->outlineOffset(), style.get(), primitiveValueCache);
        case CSSPropertyOutlineStyle:
            if (style->outlineStyleIsAuto())
                return primitiveValueCache->createIdentifierValue(CSSValueAuto);
            return primitiveValueCache->createValue(style->outlineStyle());
        case CSSPropertyOutlineWidth:
            return zoomAdjustedPixelValue(style->outlineWidth(), style.get(), primitiveValueCache);
        case CSSPropertyOverflow:
            return primitiveValueCache->createValue(max(style->overflowX(), style->overflowY()));
        case CSSPropertyOverflowX:
            return primitiveValueCache->createValue(style->overflowX());
        case CSSPropertyOverflowY:
            return primitiveValueCache->createValue(style->overflowY());
        case CSSPropertyPaddingTop:
            if (renderer && renderer->isBox())
                return zoomAdjustedPixelValue(toRenderBox(renderer)->paddingTop(false), style.get(), primitiveValueCache);
            return primitiveValueCache->createValue(style->paddingTop());
        case CSSPropertyPaddingRight:
            if (renderer && renderer->isBox())
                return zoomAdjustedPixelValue(toRenderBox(renderer)->paddingRight(false), style.get(), primitiveValueCache);
            return primitiveValueCache->createValue(style->paddingRight());
        case CSSPropertyPaddingBottom:
            if (renderer && renderer->isBox())
                return zoomAdjustedPixelValue(toRenderBox(renderer)->paddingBottom(false), style.get(), primitiveValueCache);
            return primitiveValueCache->createValue(style->paddingBottom());
        case CSSPropertyPaddingLeft:
            if (renderer && renderer->isBox())
                return zoomAdjustedPixelValue(toRenderBox(renderer)->paddingLeft(false), style.get(), primitiveValueCache);
            return primitiveValueCache->createValue(style->paddingLeft());
        case CSSPropertyPageBreakAfter:
            return primitiveValueCache->createValue(style->pageBreakAfter());
        case CSSPropertyPageBreakBefore:
            return primitiveValueCache->createValue(style->pageBreakBefore());
        case CSSPropertyPageBreakInside: {
            EPageBreak pageBreak = style->pageBreakInside();
            ASSERT(pageBreak != PBALWAYS);
            if (pageBreak == PBALWAYS)
                return 0;
            return primitiveValueCache->createValue(style->pageBreakInside());
        }
        case CSSPropertyPosition:
            return primitiveValueCache->createValue(style->position());
        case CSSPropertyRight:
            return getPositionOffsetValue(style.get(), CSSPropertyRight, primitiveValueCache);
        case CSSPropertyTableLayout:
            return primitiveValueCache->createValue(style->tableLayout());
        case CSSPropertyTextAlign:
            return primitiveValueCache->createValue(style->textAlign());
        case CSSPropertyTextDecoration:
            return renderTextDecorationFlagsToCSSValue(style->textDecoration(), primitiveValueCache);
        case CSSPropertyWebkitTextDecorationsInEffect:
            return renderTextDecorationFlagsToCSSValue(style->textDecorationsInEffect(), primitiveValueCache);
        case CSSPropertyWebkitTextFillColor:
            return currentColorOrValidColor(style.get(), style->textFillColor());
        case CSSPropertyWebkitTextEmphasisColor:
            return currentColorOrValidColor(style.get(), style->textEmphasisColor());
        case CSSPropertyWebkitTextEmphasisPosition:
            return primitiveValueCache->createValue(style->textEmphasisPosition());
        case CSSPropertyWebkitTextEmphasisStyle:
            switch (style->textEmphasisMark()) {
            case TextEmphasisMarkNone:
                return primitiveValueCache->createIdentifierValue(CSSValueNone);
            case TextEmphasisMarkCustom:
                return primitiveValueCache->createValue(style->textEmphasisCustomMark(), CSSPrimitiveValue::CSS_STRING);
            case TextEmphasisMarkAuto:
                ASSERT_NOT_REACHED();
                // Fall through
            case TextEmphasisMarkDot:
            case TextEmphasisMarkCircle:
            case TextEmphasisMarkDoubleCircle:
            case TextEmphasisMarkTriangle:
            case TextEmphasisMarkSesame: {
                RefPtr<CSSValueList> list = CSSValueList::createSpaceSeparated();
                list->append(primitiveValueCache->createValue(style->textEmphasisFill()));
                list->append(primitiveValueCache->createValue(style->textEmphasisMark()));
                return list.release();
            }
            }
        case CSSPropertyTextIndent:
            return primitiveValueCache->createValue(style->textIndent());
        case CSSPropertyTextShadow:
            return valueForShadow(style->textShadow(), propertyID, style.get());
        case CSSPropertyTextRendering:
            return primitiveValueCache->createValue(style->fontDescription().textRenderingMode());
        case CSSPropertyTextOverflow:
            if (style->textOverflow())
                return primitiveValueCache->createIdentifierValue(CSSValueEllipsis);
            return primitiveValueCache->createIdentifierValue(CSSValueClip);
        case CSSPropertyWebkitTextSecurity:
            return primitiveValueCache->createValue(style->textSecurity());
        case CSSPropertyWebkitTextSizeAdjust:
            if (style->textSizeAdjust())
                return primitiveValueCache->createIdentifierValue(CSSValueAuto);
            return primitiveValueCache->createIdentifierValue(CSSValueNone);
        case CSSPropertyWebkitTextStrokeColor:
            return currentColorOrValidColor(style.get(), style->textStrokeColor());
        case CSSPropertyWebkitTextStrokeWidth:
            return zoomAdjustedPixelValue(style->textStrokeWidth(), style.get(), primitiveValueCache);
        case CSSPropertyTextTransform:
            return primitiveValueCache->createValue(style->textTransform());
        case CSSPropertyTop:
            return getPositionOffsetValue(style.get(), CSSPropertyTop, primitiveValueCache);
        case CSSPropertyUnicodeBidi:
            return primitiveValueCache->createValue(style->unicodeBidi());
        case CSSPropertyVerticalAlign:
            switch (style->verticalAlign()) {
                case BASELINE:
                    return primitiveValueCache->createIdentifierValue(CSSValueBaseline);
                case MIDDLE:
                    return primitiveValueCache->createIdentifierValue(CSSValueMiddle);
                case SUB:
                    return primitiveValueCache->createIdentifierValue(CSSValueSub);
                case SUPER:
                    return primitiveValueCache->createIdentifierValue(CSSValueSuper);
                case TEXT_TOP:
                    return primitiveValueCache->createIdentifierValue(CSSValueTextTop);
                case TEXT_BOTTOM:
                    return primitiveValueCache->createIdentifierValue(CSSValueTextBottom);
                case TOP:
                    return primitiveValueCache->createIdentifierValue(CSSValueTop);
                case BOTTOM:
                    return primitiveValueCache->createIdentifierValue(CSSValueBottom);
                case BASELINE_MIDDLE:
                    return primitiveValueCache->createIdentifierValue(CSSValueWebkitBaselineMiddle);
                case LENGTH:
                    return primitiveValueCache->createValue(style->verticalAlignLength());
            }
            ASSERT_NOT_REACHED();
            return 0;
        case CSSPropertyVisibility:
            return primitiveValueCache->createValue(style->visibility());
        case CSSPropertyWhiteSpace:
            return primitiveValueCache->createValue(style->whiteSpace());
        case CSSPropertyWidows:
            return primitiveValueCache->createValue(style->widows(), CSSPrimitiveValue::CSS_NUMBER);
        case CSSPropertyWidth:
            if (renderer)
                return zoomAdjustedPixelValue(sizingBox(renderer).width(), style.get(), primitiveValueCache);
            return zoomAdjustedPixelValueForLength(style->width(), style.get(), primitiveValueCache);
        case CSSPropertyWordBreak:
            return primitiveValueCache->createValue(style->wordBreak());
        case CSSPropertyWordSpacing:
            return zoomAdjustedPixelValue(style->wordSpacing(), style.get(), primitiveValueCache);
        case CSSPropertyWordWrap:
            return primitiveValueCache->createValue(style->wordWrap());
        case CSSPropertyWebkitLineBreak:
            return primitiveValueCache->createValue(style->khtmlLineBreak());
        case CSSPropertyWebkitNbspMode:
            return primitiveValueCache->createValue(style->nbspMode());
        case CSSPropertyWebkitMatchNearestMailBlockquoteColor:
            return primitiveValueCache->createValue(style->matchNearestMailBlockquoteColor());
        case CSSPropertyResize:
            return primitiveValueCache->createValue(style->resize());
        case CSSPropertyWebkitFontSmoothing:
            return primitiveValueCache->createValue(style->fontDescription().fontSmoothing());
        case CSSPropertyZIndex:
            if (style->hasAutoZIndex())
                return primitiveValueCache->createIdentifierValue(CSSValueAuto);
            return primitiveValueCache->createValue(style->zIndex(), CSSPrimitiveValue::CSS_NUMBER);
        case CSSPropertyZoom:
            return primitiveValueCache->createValue(style->zoom(), CSSPrimitiveValue::CSS_NUMBER);
        case CSSPropertyBoxSizing:
            if (style->boxSizing() == CONTENT_BOX)
                return primitiveValueCache->createIdentifierValue(CSSValueContentBox);
            return primitiveValueCache->createIdentifierValue(CSSValueBorderBox);
#if ENABLE(DASHBOARD_SUPPORT)
        case CSSPropertyWebkitDashboardRegion:
        {
            const Vector<StyleDashboardRegion>& regions = style->dashboardRegions();
            unsigned count = regions.size();
            if (count == 1 && regions[0].type == StyleDashboardRegion::None)
                return primitiveValueCache->createIdentifierValue(CSSValueNone);

            RefPtr<DashboardRegion> firstRegion;
            DashboardRegion* previousRegion = 0;
            for (unsigned i = 0; i < count; i++) {
                RefPtr<DashboardRegion> region = DashboardRegion::create();
                StyleDashboardRegion styleRegion = regions[i];

                region->m_label = styleRegion.label;
                LengthBox offset = styleRegion.offset;
                region->setTop(zoomAdjustedPixelValue(offset.top().value(), style.get(), primitiveValueCache));
                region->setRight(zoomAdjustedPixelValue(offset.right().value(), style.get(), primitiveValueCache));
                region->setBottom(zoomAdjustedPixelValue(offset.bottom().value(), style.get(), primitiveValueCache));
                region->setLeft(zoomAdjustedPixelValue(offset.left().value(), style.get(), primitiveValueCache));
                region->m_isRectangle = (styleRegion.type == StyleDashboardRegion::Rectangle);
                region->m_isCircle = (styleRegion.type == StyleDashboardRegion::Circle);

                if (previousRegion)
                    previousRegion->m_next = region;
                else
                    firstRegion = region;
                previousRegion = region.get();
            }
            return primitiveValueCache->createValue(firstRegion.release());
        }
#endif
        case CSSPropertyWebkitAnimationDelay:
            return getDelayValue(style->animations(), primitiveValueCache);
        case CSSPropertyWebkitAnimationDirection: {
            RefPtr<CSSValueList> list = CSSValueList::createCommaSeparated();
            const AnimationList* t = style->animations();
            if (t) {
                for (size_t i = 0; i < t->size(); ++i) {
                    if (t->animation(i)->direction())
                        list->append(primitiveValueCache->createIdentifierValue(CSSValueAlternate));
                    else
                        list->append(primitiveValueCache->createIdentifierValue(CSSValueNormal));
                }
            } else
                list->append(primitiveValueCache->createIdentifierValue(CSSValueNormal));
            return list.release();
        }
        case CSSPropertyWebkitAnimationDuration:
            return getDurationValue(style->animations(), primitiveValueCache);
        case CSSPropertyWebkitAnimationFillMode: {
            RefPtr<CSSValueList> list = CSSValueList::createCommaSeparated();
            const AnimationList* t = style->animations();
            if (t) {
                for (size_t i = 0; i < t->size(); ++i) {
                    switch (t->animation(i)->fillMode()) {
                    case AnimationFillModeNone:
                        list->append(primitiveValueCache->createIdentifierValue(CSSValueNone));
                        break;
                    case AnimationFillModeForwards:
                        list->append(primitiveValueCache->createIdentifierValue(CSSValueForwards));
                        break;
                    case AnimationFillModeBackwards:
                        list->append(primitiveValueCache->createIdentifierValue(CSSValueBackwards));
                        break;
                    case AnimationFillModeBoth:
                        list->append(primitiveValueCache->createIdentifierValue(CSSValueBoth));
                        break;
                    }
                }
            } else
                list->append(primitiveValueCache->createIdentifierValue(CSSValueNone));
            return list.release();
        }
        case CSSPropertyWebkitAnimationIterationCount: {
            RefPtr<CSSValueList> list = CSSValueList::createCommaSeparated();
            const AnimationList* t = style->animations();
            if (t) {
                for (size_t i = 0; i < t->size(); ++i) {
                    int iterationCount = t->animation(i)->iterationCount();
                    if (iterationCount == Animation::IterationCountInfinite)
                        list->append(primitiveValueCache->createIdentifierValue(CSSValueInfinite));
                    else
                        list->append(primitiveValueCache->createValue(iterationCount, CSSPrimitiveValue::CSS_NUMBER));
                }
            } else
                list->append(primitiveValueCache->createValue(Animation::initialAnimationIterationCount(), CSSPrimitiveValue::CSS_NUMBER));
            return list.release();
        }
        case CSSPropertyWebkitAnimationName: {
            RefPtr<CSSValueList> list = CSSValueList::createCommaSeparated();
            const AnimationList* t = style->animations();
            if (t) {
                for (size_t i = 0; i < t->size(); ++i)
                    list->append(primitiveValueCache->createValue(t->animation(i)->name(), CSSPrimitiveValue::CSS_STRING));
            } else
                list->append(primitiveValueCache->createIdentifierValue(CSSValueNone));
            return list.release();
        }
        case CSSPropertyWebkitAnimationPlayState: {
            RefPtr<CSSValueList> list = CSSValueList::createCommaSeparated();
            const AnimationList* t = style->animations();
            if (t) {
                for (size_t i = 0; i < t->size(); ++i) {
                    int prop = t->animation(i)->playState();
                    if (prop == AnimPlayStatePlaying)
                        list->append(primitiveValueCache->createIdentifierValue(CSSValueRunning));
                    else
                        list->append(primitiveValueCache->createIdentifierValue(CSSValuePaused));
                }
            } else
                list->append(primitiveValueCache->createIdentifierValue(CSSValueRunning));
            return list.release();
        }
        case CSSPropertyWebkitAnimationTimingFunction:
            return getTimingFunctionValue(style->animations());
        case CSSPropertyWebkitAppearance:
            return primitiveValueCache->createValue(style->appearance());
        case CSSPropertyWebkitBackfaceVisibility:
            return primitiveValueCache->createIdentifierValue((style->backfaceVisibility() == BackfaceVisibilityHidden) ? CSSValueHidden : CSSValueVisible);
        case CSSPropertyWebkitBorderImage:
            return valueForNinePieceImage(style->borderImage(), primitiveValueCache);
        case CSSPropertyWebkitMaskBoxImage:
            return valueForNinePieceImage(style->maskBoxImage(), primitiveValueCache);
        case CSSPropertyWebkitFontSizeDelta:
            // Not a real style property -- used by the editing engine -- so has no computed value.
            break;
        case CSSPropertyWebkitMarginBottomCollapse:
        case CSSPropertyWebkitMarginAfterCollapse:
            return primitiveValueCache->createValue(style->marginAfterCollapse());
        case CSSPropertyWebkitMarginTopCollapse:
        case CSSPropertyWebkitMarginBeforeCollapse:
            return primitiveValueCache->createValue(style->marginBeforeCollapse());
        case CSSPropertyWebkitPerspective:
            if (!style->hasPerspective())
                return primitiveValueCache->createIdentifierValue(CSSValueNone);
            return zoomAdjustedPixelValue(style->perspective(), style.get(), primitiveValueCache);
        case CSSPropertyWebkitPerspectiveOrigin: {
            RefPtr<CSSValueList> list = CSSValueList::createSpaceSeparated();
            if (renderer) {
                IntRect box = sizingBox(renderer);
                list->append(zoomAdjustedPixelValue(style->perspectiveOriginX().calcMinValue(box.width()), style.get(), primitiveValueCache));
                list->append(zoomAdjustedPixelValue(style->perspectiveOriginY().calcMinValue(box.height()), style.get(), primitiveValueCache));
            }
            else {
                list->append(zoomAdjustedPixelValueForLength(style->perspectiveOriginX(), style.get(), primitiveValueCache));
                list->append(zoomAdjustedPixelValueForLength(style->perspectiveOriginY(), style.get(), primitiveValueCache));
                
            }
            return list.release();
        }
        case CSSPropertyWebkitRtlOrdering:
            if (style->visuallyOrdered())
                return primitiveValueCache->createIdentifierValue(CSSValueVisual);
            return primitiveValueCache->createIdentifierValue(CSSValueLogical);
        case CSSPropertyWebkitUserDrag:
            return primitiveValueCache->createValue(style->userDrag());
        case CSSPropertyWebkitUserSelect:
            return primitiveValueCache->createValue(style->userSelect());
        case CSSPropertyBorderBottomLeftRadius:
            return getBorderRadiusCornerValue(style->borderBottomLeftRadius(), style.get(), primitiveValueCache);
        case CSSPropertyBorderBottomRightRadius:
            return getBorderRadiusCornerValue(style->borderBottomRightRadius(), style.get(), primitiveValueCache);
        case CSSPropertyBorderTopLeftRadius:
            return getBorderRadiusCornerValue(style->borderTopLeftRadius(), style.get(), primitiveValueCache);
        case CSSPropertyBorderTopRightRadius:
            return getBorderRadiusCornerValue(style->borderTopRightRadius(), style.get(), primitiveValueCache);
        case CSSPropertyClip: {
            if (!style->hasClip())
                return primitiveValueCache->createIdentifierValue(CSSValueAuto);
            RefPtr<Rect> rect = Rect::create();
            rect->setTop(zoomAdjustedPixelValue(style->clip().top().value(), style.get(), primitiveValueCache));
            rect->setRight(zoomAdjustedPixelValue(style->clip().right().value(), style.get(), primitiveValueCache));
            rect->setBottom(zoomAdjustedPixelValue(style->clip().bottom().value(), style.get(), primitiveValueCache));
            rect->setLeft(zoomAdjustedPixelValue(style->clip().left().value(), style.get(), primitiveValueCache));
            return primitiveValueCache->createValue(rect.release());
        }
        case CSSPropertySpeak:
            return primitiveValueCache->createValue(style->speak());
        case CSSPropertyWebkitTransform:
            return computedTransform(renderer, style.get(), primitiveValueCache);
        case CSSPropertyWebkitTransformOrigin: {
            RefPtr<CSSValueList> list = CSSValueList::createSpaceSeparated();
            if (renderer) {
                IntRect box = sizingBox(renderer);
                list->append(zoomAdjustedPixelValue(style->transformOriginX().calcMinValue(box.width()), style.get(), primitiveValueCache));
                list->append(zoomAdjustedPixelValue(style->transformOriginY().calcMinValue(box.height()), style.get(), primitiveValueCache));
                if (style->transformOriginZ() != 0)
                    list->append(zoomAdjustedPixelValue(style->transformOriginZ(), style.get(), primitiveValueCache));
            } else {
                list->append(zoomAdjustedPixelValueForLength(style->transformOriginX(), style.get(), primitiveValueCache));
                list->append(zoomAdjustedPixelValueForLength(style->transformOriginY(), style.get(), primitiveValueCache));
                if (style->transformOriginZ() != 0)
                    list->append(zoomAdjustedPixelValue(style->transformOriginZ(), style.get(), primitiveValueCache));
            }
            return list.release();
        }
        case CSSPropertyWebkitTransformStyle:
            return primitiveValueCache->createIdentifierValue((style->transformStyle3D() == TransformStyle3DPreserve3D) ? CSSValuePreserve3d : CSSValueFlat);
        case CSSPropertyWebkitTransitionDelay:
            return getDelayValue(style->transitions(), primitiveValueCache);
        case CSSPropertyWebkitTransitionDuration:
            return getDurationValue(style->transitions(), primitiveValueCache);
        case CSSPropertyWebkitTransitionProperty: {
            RefPtr<CSSValueList> list = CSSValueList::createCommaSeparated();
            const AnimationList* t = style->transitions();
            if (t) {
                for (size_t i = 0; i < t->size(); ++i) {
                    int prop = t->animation(i)->property();
                    RefPtr<CSSValue> propertyValue;
                    if (prop == cAnimateNone)
                        propertyValue = primitiveValueCache->createIdentifierValue(CSSValueNone);
                    else if (prop == cAnimateAll)
                        propertyValue = primitiveValueCache->createIdentifierValue(CSSValueAll);
                    else
                        propertyValue = primitiveValueCache->createValue(getPropertyName(static_cast<CSSPropertyID>(prop)), CSSPrimitiveValue::CSS_STRING);
                    list->append(propertyValue);
                }
            } else
                list->append(primitiveValueCache->createIdentifierValue(CSSValueAll));
            return list.release();
        }
        case CSSPropertyWebkitTransitionTimingFunction:
            return getTimingFunctionValue(style->transitions());
        case CSSPropertyPointerEvents:
            return primitiveValueCache->createValue(style->pointerEvents());
        case CSSPropertyWebkitColorCorrection:
            return primitiveValueCache->createValue(style->colorSpace());
        case CSSPropertyWebkitWritingMode:
            return primitiveValueCache->createValue(style->writingMode());
        case CSSPropertyWebkitTextCombine:
            return primitiveValueCache->createValue(style->textCombine());
        case CSSPropertyWebkitTextOrientation:
            return CSSPrimitiveValue::create(style->fontDescription().textOrientation());
        case CSSPropertyWebkitLineBoxContain:
            return createLineBoxContainValue(primitiveValueCache, style->lineBoxContain());
        case CSSPropertyContent:
            return contentToCSSValue(style.get(), primitiveValueCache);
        case CSSPropertyCounterIncrement:
            return counterToCSSValue(style.get(), propertyID, primitiveValueCache);
        case CSSPropertyCounterReset:
            return counterToCSSValue(style.get(), propertyID, primitiveValueCache);
        
        /* Shorthand properties, currently not supported see bug 13658*/
        case CSSPropertyBackground:
        case CSSPropertyBorder:
        case CSSPropertyBorderBottom:
        case CSSPropertyBorderColor:
        case CSSPropertyBorderLeft:
        case CSSPropertyBorderRadius:
        case CSSPropertyBorderRight:
        case CSSPropertyBorderStyle:
        case CSSPropertyBorderTop:
        case CSSPropertyBorderWidth:
        case CSSPropertyFont:
        case CSSPropertyListStyle:
        case CSSPropertyMargin:
        case CSSPropertyOutline:
        case CSSPropertyPadding:
            break;

        /* Individual properties not part of the spec */
        case CSSPropertyBackgroundRepeatX:
        case CSSPropertyBackgroundRepeatY:
            break;

        /* Unimplemented CSS 3 properties (including CSS3 shorthand properties) */
        case CSSPropertyWebkitTextEmphasis:
        case CSSPropertyTextLineThrough:
        case CSSPropertyTextLineThroughColor:
        case CSSPropertyTextLineThroughMode:
        case CSSPropertyTextLineThroughStyle:
        case CSSPropertyTextLineThroughWidth:
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
            break;

        /* Directional properties are resolved by resolveDirectionAwareProperty() before the switch. */
        case CSSPropertyWebkitBorderEnd:
        case CSSPropertyWebkitBorderEndColor:
        case CSSPropertyWebkitBorderEndStyle:
        case CSSPropertyWebkitBorderEndWidth:
        case CSSPropertyWebkitBorderStart:
        case CSSPropertyWebkitBorderStartColor:
        case CSSPropertyWebkitBorderStartStyle:
        case CSSPropertyWebkitBorderStartWidth:
        case CSSPropertyWebkitBorderAfter:
        case CSSPropertyWebkitBorderAfterColor:
        case CSSPropertyWebkitBorderAfterStyle:
        case CSSPropertyWebkitBorderAfterWidth:
        case CSSPropertyWebkitBorderBefore:
        case CSSPropertyWebkitBorderBeforeColor:
        case CSSPropertyWebkitBorderBeforeStyle:
        case CSSPropertyWebkitBorderBeforeWidth:
        case CSSPropertyWebkitMarginEnd:
        case CSSPropertyWebkitMarginStart:
        case CSSPropertyWebkitMarginAfter:
        case CSSPropertyWebkitMarginBefore:
        case CSSPropertyWebkitPaddingEnd:
        case CSSPropertyWebkitPaddingStart:
        case CSSPropertyWebkitPaddingAfter:
        case CSSPropertyWebkitPaddingBefore:
        case CSSPropertyWebkitLogicalWidth:
        case CSSPropertyWebkitLogicalHeight:
        case CSSPropertyWebkitMinLogicalWidth:
        case CSSPropertyWebkitMinLogicalHeight:
        case CSSPropertyWebkitMaxLogicalWidth:
        case CSSPropertyWebkitMaxLogicalHeight:
            ASSERT_NOT_REACHED();
            break;

        /* Unimplemented @font-face properties */
        case CSSPropertyFontStretch:
        case CSSPropertySrc:
        case CSSPropertyUnicodeRange:
            break;

        /* Other unimplemented properties */
        case CSSPropertyPage: // for @page
        case CSSPropertyQuotes: // FIXME: needs implementation
        case CSSPropertySize: // for @page
            break;

        /* Unimplemented -webkit- properties */
        case CSSPropertyWebkitAnimation:
        case CSSPropertyWebkitBorderRadius:
        case CSSPropertyWebkitColumns:
        case CSSPropertyWebkitColumnRule:
        case CSSPropertyWebkitMarginCollapse:
        case CSSPropertyWebkitMarquee:
        case CSSPropertyWebkitMarqueeSpeed:
        case CSSPropertyWebkitMask:
        case CSSPropertyWebkitMaskRepeatX:
        case CSSPropertyWebkitMaskRepeatY:
        case CSSPropertyWebkitPerspectiveOriginX:
        case CSSPropertyWebkitPerspectiveOriginY:
        case CSSPropertyWebkitTextStroke:
        case CSSPropertyWebkitTransformOriginX:
        case CSSPropertyWebkitTransformOriginY:
        case CSSPropertyWebkitTransformOriginZ:
        case CSSPropertyWebkitTransition:
            break;
#if ENABLE(SVG)
        case CSSPropertyClipPath:
        case CSSPropertyClipRule:
        case CSSPropertyMask:
        case CSSPropertyEnableBackground:
        case CSSPropertyFilter:
        case CSSPropertyFloodColor:
        case CSSPropertyFloodOpacity:
        case CSSPropertyLightingColor:
        case CSSPropertyStopColor:
        case CSSPropertyStopOpacity:
        case CSSPropertyColorInterpolation:
        case CSSPropertyColorInterpolationFilters:
        case CSSPropertyColorProfile:
        case CSSPropertyColorRendering:
        case CSSPropertyFill:
        case CSSPropertyFillOpacity:
        case CSSPropertyFillRule:
        case CSSPropertyImageRendering:
        case CSSPropertyMarker:
        case CSSPropertyMarkerEnd:
        case CSSPropertyMarkerMid:
        case CSSPropertyMarkerStart:
        case CSSPropertyShapeRendering:
        case CSSPropertyStroke:
        case CSSPropertyStrokeDasharray:
        case CSSPropertyStrokeDashoffset:
        case CSSPropertyStrokeLinecap:
        case CSSPropertyStrokeLinejoin:
        case CSSPropertyStrokeMiterlimit:
        case CSSPropertyStrokeOpacity:
        case CSSPropertyStrokeWidth:
        case CSSPropertyAlignmentBaseline:
        case CSSPropertyBaselineShift:
        case CSSPropertyDominantBaseline:
        case CSSPropertyGlyphOrientationHorizontal:
        case CSSPropertyGlyphOrientationVertical:
        case CSSPropertyKerning:
        case CSSPropertyTextAnchor:
        case CSSPropertyVectorEffect:
        case CSSPropertyWritingMode:
        case CSSPropertyWebkitSvgShadow:
            return getSVGPropertyCSSValue(propertyID, DoNotUpdateLayout);
#endif
    }

    logUnimplementedPropertyID(propertyID);
    return 0;
}

String CSSComputedStyleDeclaration::getPropertyValue(int propertyID) const
{
    RefPtr<CSSValue> value = getPropertyCSSValue(propertyID);
    if (value)
        return value->cssText();
    return "";
}

bool CSSComputedStyleDeclaration::getPropertyPriority(int /*propertyID*/) const
{
    // All computed styles have a priority of false (not "important").
    return false;
}

String CSSComputedStyleDeclaration::removeProperty(int /*propertyID*/, ExceptionCode& ec)
{
    ec = NO_MODIFICATION_ALLOWED_ERR;
    return String();
}

void CSSComputedStyleDeclaration::setProperty(int /*propertyID*/, const String& /*value*/, bool /*important*/, ExceptionCode& ec)
{
    ec = NO_MODIFICATION_ALLOWED_ERR;
}

unsigned CSSComputedStyleDeclaration::virtualLength() const
{
    Node* node = m_node.get();
    if (!node)
        return 0;

    RenderStyle* style = node->computedStyle(m_pseudoElementSpecifier);
    if (!style)
        return 0;

    return numComputedProperties;
}

String CSSComputedStyleDeclaration::item(unsigned i) const
{
    if (i >= length())
        return "";

    return getPropertyName(static_cast<CSSPropertyID>(computedProperties[i]));
}

bool CSSComputedStyleDeclaration::cssPropertyMatches(const CSSProperty* property) const
{
    if (property->id() == CSSPropertyFontSize && property->value()->isPrimitiveValue() && m_node) {
        m_node->document()->updateLayoutIgnorePendingStylesheets();
        RenderStyle* style = m_node->computedStyle(m_pseudoElementSpecifier);
        if (style && style->fontDescription().keywordSize()) {
            int sizeValue = cssIdentifierForFontSizeKeyword(style->fontDescription().keywordSize());
            CSSPrimitiveValue* primitiveValue = static_cast<CSSPrimitiveValue*>(property->value());
            if (primitiveValue->primitiveType() == CSSPrimitiveValue::CSS_IDENT && primitiveValue->getIdent() == sizeValue)
                return true;
        }
    }

    return CSSStyleDeclaration::cssPropertyMatches(property);
}

PassRefPtr<CSSMutableStyleDeclaration> CSSComputedStyleDeclaration::copy() const
{
    return copyPropertiesInSet(computedProperties, numComputedProperties);
}

PassRefPtr<CSSMutableStyleDeclaration> CSSComputedStyleDeclaration::makeMutable()
{
    return copy();
}

} // namespace WebCore
