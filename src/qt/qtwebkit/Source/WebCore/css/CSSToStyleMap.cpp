/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 2004-2005 Allan Sandfeld Jensen (kde@carewolf.com)
 * Copyright (C) 2006, 2007 Nicholas Shanks (webkit@nickshanks.com)
 * Copyright (C) 2005, 2006, 2007, 2008, 2009, 2010, 2011, 2012 Apple Inc. All rights reserved.
 * Copyright (C) 2007 Alexey Proskuryakov <ap@webkit.org>
 * Copyright (C) 2007, 2008 Eric Seidel <eric@webkit.org>
 * Copyright (C) 2008, 2009 Torch Mobile Inc. All rights reserved. (http://www.torchmobile.com/)
 * Copyright (c) 2011, Code Aurora Forum. All rights reserved.
 * Copyright (C) Research In Motion Limited 2011. All rights reserved.
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
#include "CSSToStyleMap.h"

#include "Animation.h"
#include "CSSBorderImageSliceValue.h"
#include "CSSPrimitiveValue.h"
#include "CSSPrimitiveValueMappings.h"
#include "CSSTimingFunctionValue.h"
#include "CSSValueKeywords.h"
#include "FillLayer.h"
#include "Pair.h"
#include "Rect.h"
#include "StyleResolver.h"

namespace WebCore {

RenderStyle* CSSToStyleMap::style() const
{
    return m_resolver->style();
}
    
RenderStyle* CSSToStyleMap::rootElementStyle() const
{
    return m_resolver->rootElementStyle();
}

bool CSSToStyleMap::useSVGZoomRules() const
{
    return m_resolver->useSVGZoomRules();
}
    
PassRefPtr<StyleImage> CSSToStyleMap::styleImage(CSSPropertyID propertyId, CSSValue* value)
{
    return m_resolver->styleImage(propertyId, value);
}

void CSSToStyleMap::mapFillAttachment(CSSPropertyID, FillLayer* layer, CSSValue* value)
{
    if (value->isInitialValue()) {
        layer->setAttachment(FillLayer::initialFillAttachment(layer->type()));
        return;
    }

    if (!value->isPrimitiveValue())
        return;

    CSSPrimitiveValue* primitiveValue = static_cast<CSSPrimitiveValue*>(value);
    switch (primitiveValue->getValueID()) {
    case CSSValueFixed:
        layer->setAttachment(FixedBackgroundAttachment);
        break;
    case CSSValueScroll:
        layer->setAttachment(ScrollBackgroundAttachment);
        break;
    case CSSValueLocal:
        layer->setAttachment(LocalBackgroundAttachment);
        break;
    default:
        return;
    }
}

void CSSToStyleMap::mapFillClip(CSSPropertyID, FillLayer* layer, CSSValue* value)
{
    if (value->isInitialValue()) {
        layer->setClip(FillLayer::initialFillClip(layer->type()));
        return;
    }

    if (!value->isPrimitiveValue())
        return;

    CSSPrimitiveValue* primitiveValue = static_cast<CSSPrimitiveValue*>(value);
    layer->setClip(*primitiveValue);
}

void CSSToStyleMap::mapFillComposite(CSSPropertyID, FillLayer* layer, CSSValue* value)
{
    if (value->isInitialValue()) {
        layer->setComposite(FillLayer::initialFillComposite(layer->type()));
        return;
    }

    if (!value->isPrimitiveValue())
        return;

    CSSPrimitiveValue* primitiveValue = static_cast<CSSPrimitiveValue*>(value);
    layer->setComposite(*primitiveValue);
}

void CSSToStyleMap::mapFillBlendMode(CSSPropertyID, FillLayer* layer, CSSValue* value)
{
    if (value->isInitialValue()) {
        layer->setBlendMode(FillLayer::initialFillBlendMode(layer->type()));
        return;
    }

    if (!value->isPrimitiveValue())
        return;

    CSSPrimitiveValue* primitiveValue = static_cast<CSSPrimitiveValue*>(value);
    layer->setBlendMode(*primitiveValue);
}

void CSSToStyleMap::mapFillOrigin(CSSPropertyID, FillLayer* layer, CSSValue* value)
{
    if (value->isInitialValue()) {
        layer->setOrigin(FillLayer::initialFillOrigin(layer->type()));
        return;
    }

    if (!value->isPrimitiveValue())
        return;

    CSSPrimitiveValue* primitiveValue = static_cast<CSSPrimitiveValue*>(value);
    layer->setOrigin(*primitiveValue);
}


void CSSToStyleMap::mapFillImage(CSSPropertyID property, FillLayer* layer, CSSValue* value)
{
    if (value->isInitialValue()) {
        layer->setImage(FillLayer::initialFillImage(layer->type()));
        return;
    }

    layer->setImage(styleImage(property, value));
}

void CSSToStyleMap::mapFillRepeatX(CSSPropertyID, FillLayer* layer, CSSValue* value)
{
    if (value->isInitialValue()) {
        layer->setRepeatX(FillLayer::initialFillRepeatX(layer->type()));
        return;
    }

    if (!value->isPrimitiveValue())
        return;

    CSSPrimitiveValue* primitiveValue = static_cast<CSSPrimitiveValue*>(value);
    layer->setRepeatX(*primitiveValue);
}

void CSSToStyleMap::mapFillRepeatY(CSSPropertyID, FillLayer* layer, CSSValue* value)
{
    if (value->isInitialValue()) {
        layer->setRepeatY(FillLayer::initialFillRepeatY(layer->type()));
        return;
    }

    if (!value->isPrimitiveValue())
        return;

    CSSPrimitiveValue* primitiveValue = static_cast<CSSPrimitiveValue*>(value);
    layer->setRepeatY(*primitiveValue);
}

void CSSToStyleMap::mapFillSize(CSSPropertyID, FillLayer* layer, CSSValue* value)
{
    if (!value->isPrimitiveValue()) {
        layer->setSizeType(SizeNone);
        return;
    }

    CSSPrimitiveValue* primitiveValue = static_cast<CSSPrimitiveValue*>(value);
    if (primitiveValue->getValueID() == CSSValueContain)
        layer->setSizeType(Contain);
    else if (primitiveValue->getValueID() == CSSValueCover)
        layer->setSizeType(Cover);
    else
        layer->setSizeType(SizeLength);

    LengthSize b = FillLayer::initialFillSizeLength(layer->type());

    if (value->isInitialValue() || primitiveValue->getValueID() == CSSValueContain || primitiveValue->getValueID() == CSSValueCover) {
        layer->setSizeLength(b);
        return;
    }

    float zoomFactor = style()->effectiveZoom();

    Length firstLength;
    Length secondLength;

    if (Pair* pair = primitiveValue->getPairValue()) {
        CSSPrimitiveValue* first = static_cast<CSSPrimitiveValue*>(pair->first());
        CSSPrimitiveValue* second = static_cast<CSSPrimitiveValue*>(pair->second());
        firstLength = first->convertToLength<AnyConversion>(style(), rootElementStyle(), zoomFactor);
        secondLength = second->convertToLength<AnyConversion>(style(), rootElementStyle(), zoomFactor);
    } else {
        firstLength = primitiveValue->convertToLength<AnyConversion>(style(), rootElementStyle(), zoomFactor);
        secondLength = Length();
    }

    if (firstLength.isUndefined() || secondLength.isUndefined())
        return;

    b.setWidth(firstLength);
    b.setHeight(secondLength);
    layer->setSizeLength(b);
}

void CSSToStyleMap::mapFillXPosition(CSSPropertyID propertyID, FillLayer* layer, CSSValue* value)
{
    if (value->isInitialValue()) {
        layer->setXPosition(FillLayer::initialFillXPosition(layer->type()));
        return;
    }

    if (!value->isPrimitiveValue())
        return;

    float zoomFactor = style()->effectiveZoom();

    CSSPrimitiveValue* primitiveValue = static_cast<CSSPrimitiveValue*>(value);
    Pair* pair = primitiveValue->getPairValue();
    if (pair) {
        ASSERT_UNUSED(propertyID, propertyID == CSSPropertyBackgroundPositionX || propertyID == CSSPropertyWebkitMaskPositionX);
        primitiveValue = pair->second();
    }

    Length length;
    if (primitiveValue->isLength())
        length = primitiveValue->computeLength<Length>(style(), rootElementStyle(), zoomFactor);
    else if (primitiveValue->isPercentage())
        length = Length(primitiveValue->getDoubleValue(), Percent);
    else if (primitiveValue->isCalculatedPercentageWithLength())
        length = Length(primitiveValue->cssCalcValue()->toCalcValue(style(), rootElementStyle(), zoomFactor));
    else if (primitiveValue->isViewportPercentageLength())
        length = primitiveValue->viewportPercentageLength();
    else
        return;

    layer->setXPosition(length);
    if (pair)
        layer->setBackgroundXOrigin(*(pair->first()));
}

void CSSToStyleMap::mapFillYPosition(CSSPropertyID propertyID, FillLayer* layer, CSSValue* value)
{
    if (value->isInitialValue()) {
        layer->setYPosition(FillLayer::initialFillYPosition(layer->type()));
        return;
    }

    if (!value->isPrimitiveValue())
        return;

    float zoomFactor = style()->effectiveZoom();

    CSSPrimitiveValue* primitiveValue = static_cast<CSSPrimitiveValue*>(value);
    Pair* pair = primitiveValue->getPairValue();
    if (pair) {
        ASSERT_UNUSED(propertyID, propertyID == CSSPropertyBackgroundPositionY || propertyID == CSSPropertyWebkitMaskPositionY);
        primitiveValue = pair->second();
    }

    Length length;
    if (primitiveValue->isLength())
        length = primitiveValue->computeLength<Length>(style(), rootElementStyle(), zoomFactor);
    else if (primitiveValue->isPercentage())
        length = Length(primitiveValue->getDoubleValue(), Percent);
    else if (primitiveValue->isCalculatedPercentageWithLength())
        length = Length(primitiveValue->cssCalcValue()->toCalcValue(style(), rootElementStyle(), zoomFactor));
    else if (primitiveValue->isViewportPercentageLength())
        length = primitiveValue->viewportPercentageLength();
    else
        return;

    layer->setYPosition(length);
    if (pair)
        layer->setBackgroundYOrigin(*(pair->first()));
}

void CSSToStyleMap::mapAnimationDelay(Animation* animation, CSSValue* value)
{
    if (value->isInitialValue()) {
        animation->setDelay(Animation::initialAnimationDelay());
        return;
    }

    if (!value->isPrimitiveValue())
        return;

    CSSPrimitiveValue* primitiveValue = static_cast<CSSPrimitiveValue*>(value);
    animation->setDelay(primitiveValue->computeTime<double, CSSPrimitiveValue::Seconds>());
}

void CSSToStyleMap::mapAnimationDirection(Animation* layer, CSSValue* value)
{
    if (value->isInitialValue()) {
        layer->setDirection(Animation::initialAnimationDirection());
        return;
    }

    if (!value->isPrimitiveValue())
        return;

    CSSPrimitiveValue* primitiveValue = static_cast<CSSPrimitiveValue*>(value);
    switch (primitiveValue->getValueID()) {
    case CSSValueNormal:
        layer->setDirection(Animation::AnimationDirectionNormal);
        break;
    case CSSValueAlternate:
        layer->setDirection(Animation::AnimationDirectionAlternate);
        break;
    case CSSValueReverse:
        layer->setDirection(Animation::AnimationDirectionReverse);
        break;
    case CSSValueAlternateReverse:
        layer->setDirection(Animation::AnimationDirectionAlternateReverse);
        break;
    default:
        break;
    }
}

void CSSToStyleMap::mapAnimationDuration(Animation* animation, CSSValue* value)
{
    if (value->isInitialValue()) {
        animation->setDuration(Animation::initialAnimationDuration());
        return;
    }

    if (!value->isPrimitiveValue())
        return;

    CSSPrimitiveValue* primitiveValue = static_cast<CSSPrimitiveValue*>(value);
    animation->setDuration(primitiveValue->computeTime<double, CSSPrimitiveValue::Seconds>());
}

void CSSToStyleMap::mapAnimationFillMode(Animation* layer, CSSValue* value)
{
    if (value->isInitialValue()) {
        layer->setFillMode(Animation::initialAnimationFillMode());
        return;
    }

    if (!value->isPrimitiveValue())
        return;

    CSSPrimitiveValue* primitiveValue = static_cast<CSSPrimitiveValue*>(value);
    switch (primitiveValue->getValueID()) {
    case CSSValueNone:
        layer->setFillMode(AnimationFillModeNone);
        break;
    case CSSValueForwards:
        layer->setFillMode(AnimationFillModeForwards);
        break;
    case CSSValueBackwards:
        layer->setFillMode(AnimationFillModeBackwards);
        break;
    case CSSValueBoth:
        layer->setFillMode(AnimationFillModeBoth);
        break;
    default:
        break;
    }
}

void CSSToStyleMap::mapAnimationIterationCount(Animation* animation, CSSValue* value)
{
    if (value->isInitialValue()) {
        animation->setIterationCount(Animation::initialAnimationIterationCount());
        return;
    }

    if (!value->isPrimitiveValue())
        return;

    CSSPrimitiveValue* primitiveValue = static_cast<CSSPrimitiveValue*>(value);
    if (primitiveValue->getValueID() == CSSValueInfinite)
        animation->setIterationCount(Animation::IterationCountInfinite);
    else
        animation->setIterationCount(primitiveValue->getFloatValue());
}

void CSSToStyleMap::mapAnimationName(Animation* layer, CSSValue* value)
{
    if (value->isInitialValue()) {
        layer->setName(Animation::initialAnimationName());
        return;
    }

    if (!value->isPrimitiveValue())
        return;

    CSSPrimitiveValue* primitiveValue = static_cast<CSSPrimitiveValue*>(value);
    if (primitiveValue->getValueID() == CSSValueNone)
        layer->setIsNoneAnimation(true);
    else
        layer->setName(primitiveValue->getStringValue());
}

void CSSToStyleMap::mapAnimationPlayState(Animation* layer, CSSValue* value)
{
    if (value->isInitialValue()) {
        layer->setPlayState(Animation::initialAnimationPlayState());
        return;
    }

    if (!value->isPrimitiveValue())
        return;

    CSSPrimitiveValue* primitiveValue = static_cast<CSSPrimitiveValue*>(value);
    EAnimPlayState playState = (primitiveValue->getValueID() == CSSValuePaused) ? AnimPlayStatePaused : AnimPlayStatePlaying;
    layer->setPlayState(playState);
}

void CSSToStyleMap::mapAnimationProperty(Animation* animation, CSSValue* value)
{
    if (value->isInitialValue()) {
        animation->setAnimationMode(Animation::AnimateAll);
        animation->setProperty(CSSPropertyInvalid);
        return;
    }

    if (!value->isPrimitiveValue())
        return;

    CSSPrimitiveValue* primitiveValue = static_cast<CSSPrimitiveValue*>(value);
    if (primitiveValue->getValueID() == CSSValueAll) {
        animation->setAnimationMode(Animation::AnimateAll);
        animation->setProperty(CSSPropertyInvalid);
    } else if (primitiveValue->getValueID() == CSSValueNone) {
        animation->setAnimationMode(Animation::AnimateNone);
        animation->setProperty(CSSPropertyInvalid);
    } else {
        animation->setAnimationMode(Animation::AnimateSingleProperty);
        animation->setProperty(primitiveValue->getPropertyID());
    }
}

void CSSToStyleMap::mapAnimationTimingFunction(Animation* animation, CSSValue* value)
{
    if (value->isInitialValue()) {
        animation->setTimingFunction(Animation::initialAnimationTimingFunction());
        return;
    }

    if (value->isPrimitiveValue()) {
        CSSPrimitiveValue* primitiveValue = static_cast<CSSPrimitiveValue*>(value);
        switch (primitiveValue->getValueID()) {
        case CSSValueLinear:
            animation->setTimingFunction(LinearTimingFunction::create());
            break;
        case CSSValueEase:
            animation->setTimingFunction(CubicBezierTimingFunction::create());
            break;
        case CSSValueEaseIn:
            animation->setTimingFunction(CubicBezierTimingFunction::create(CubicBezierTimingFunction::EaseIn));
            break;
        case CSSValueEaseOut:
            animation->setTimingFunction(CubicBezierTimingFunction::create(CubicBezierTimingFunction::EaseOut));
            break;
        case CSSValueEaseInOut:
            animation->setTimingFunction(CubicBezierTimingFunction::create(CubicBezierTimingFunction::EaseInOut));
            break;
        case CSSValueStepStart:
            animation->setTimingFunction(StepsTimingFunction::create(1, true));
            break;
        case CSSValueStepEnd:
            animation->setTimingFunction(StepsTimingFunction::create(1, false));
            break;
        default:
            break;
        }
        return;
    }

    if (value->isCubicBezierTimingFunctionValue()) {
        CSSCubicBezierTimingFunctionValue* cubicTimingFunction = static_cast<CSSCubicBezierTimingFunctionValue*>(value);
        animation->setTimingFunction(CubicBezierTimingFunction::create(cubicTimingFunction->x1(), cubicTimingFunction->y1(), cubicTimingFunction->x2(), cubicTimingFunction->y2()));
    } else if (value->isStepsTimingFunctionValue()) {
        CSSStepsTimingFunctionValue* stepsTimingFunction = static_cast<CSSStepsTimingFunctionValue*>(value);
        animation->setTimingFunction(StepsTimingFunction::create(stepsTimingFunction->numberOfSteps(), stepsTimingFunction->stepAtStart()));
    } else if (value->isLinearTimingFunctionValue())
        animation->setTimingFunction(LinearTimingFunction::create());
}

void CSSToStyleMap::mapNinePieceImage(CSSPropertyID property, CSSValue* value, NinePieceImage& image)
{
    // If we're not a value list, then we are "none" and don't need to alter the empty image at all.
    if (!value || !value->isValueList())
        return;

    // Retrieve the border image value.
    CSSValueList* borderImage = static_cast<CSSValueList*>(value);

    // Set the image (this kicks off the load).
    CSSPropertyID imageProperty;
    if (property == CSSPropertyWebkitBorderImage)
        imageProperty = CSSPropertyBorderImageSource;
    else if (property == CSSPropertyWebkitMaskBoxImage)
        imageProperty = CSSPropertyWebkitMaskBoxImageSource;
    else
        imageProperty = property;

    for (unsigned i = 0 ; i < borderImage->length() ; ++i) {
        CSSValue* current = borderImage->item(i);

        if (current->isImageValue() || current->isImageGeneratorValue()
#if ENABLE(CSS_IMAGE_SET)
            || current->isImageSetValue()
#endif
            )
            image.setImage(styleImage(imageProperty, current));
        else if (current->isBorderImageSliceValue())
            mapNinePieceImageSlice(current, image);
        else if (current->isValueList()) {
            CSSValueList* slashList = static_cast<CSSValueList*>(current);
            // Map in the image slices.
            if (slashList->item(0) && slashList->item(0)->isBorderImageSliceValue())
                mapNinePieceImageSlice(slashList->item(0), image);

            // Map in the border slices.
            if (slashList->item(1))
                image.setBorderSlices(mapNinePieceImageQuad(slashList->item(1)));

            // Map in the outset.
            if (slashList->item(2))
                image.setOutset(mapNinePieceImageQuad(slashList->item(2)));
        } else if (current->isPrimitiveValue()) {
            // Set the appropriate rules for stretch/round/repeat of the slices.
            mapNinePieceImageRepeat(current, image);
        }
    }

    if (property == CSSPropertyWebkitBorderImage) {
        // We have to preserve the legacy behavior of -webkit-border-image and make the border slices
        // also set the border widths. We don't need to worry about percentages, since we don't even support
        // those on real borders yet.
        if (image.borderSlices().top().isFixed())
            style()->setBorderTopWidth(image.borderSlices().top().value());
        if (image.borderSlices().right().isFixed())
            style()->setBorderRightWidth(image.borderSlices().right().value());
        if (image.borderSlices().bottom().isFixed())
            style()->setBorderBottomWidth(image.borderSlices().bottom().value());
        if (image.borderSlices().left().isFixed())
            style()->setBorderLeftWidth(image.borderSlices().left().value());
    }
}

void CSSToStyleMap::mapNinePieceImageSlice(CSSValue* value, NinePieceImage& image)
{
    if (!value || !value->isBorderImageSliceValue())
        return;

    // Retrieve the border image value.
    CSSBorderImageSliceValue* borderImageSlice = static_cast<CSSBorderImageSliceValue*>(value);

    // Set up a length box to represent our image slices.
    LengthBox box;
    Quad* slices = borderImageSlice->slices();
    if (slices->top()->isPercentage())
        box.m_top = Length(slices->top()->getDoubleValue(), Percent);
    else
        box.m_top = Length(slices->top()->getIntValue(CSSPrimitiveValue::CSS_NUMBER), Fixed);
    if (slices->bottom()->isPercentage())
        box.m_bottom = Length(slices->bottom()->getDoubleValue(), Percent);
    else
        box.m_bottom = Length((int)slices->bottom()->getFloatValue(CSSPrimitiveValue::CSS_NUMBER), Fixed);
    if (slices->left()->isPercentage())
        box.m_left = Length(slices->left()->getDoubleValue(), Percent);
    else
        box.m_left = Length(slices->left()->getIntValue(CSSPrimitiveValue::CSS_NUMBER), Fixed);
    if (slices->right()->isPercentage())
        box.m_right = Length(slices->right()->getDoubleValue(), Percent);
    else
        box.m_right = Length(slices->right()->getIntValue(CSSPrimitiveValue::CSS_NUMBER), Fixed);
    image.setImageSlices(box);

    // Set our fill mode.
    image.setFill(borderImageSlice->m_fill);
}

LengthBox CSSToStyleMap::mapNinePieceImageQuad(CSSValue* value)
{
    if (!value || !value->isPrimitiveValue())
        return LengthBox();

    // Get our zoom value.
    float zoom = useSVGZoomRules() ? 1.0f : style()->effectiveZoom();

    // Retrieve the primitive value.
    CSSPrimitiveValue* borderWidths = static_cast<CSSPrimitiveValue*>(value);

    // Set up a length box to represent our image slices.
    LengthBox box; // Defaults to 'auto' so we don't have to handle that explicitly below.
    Quad* slices = borderWidths->getQuadValue();
    if (slices->top()->isNumber())
        box.m_top = Length(slices->top()->getIntValue(), Relative);
    else if (slices->top()->isPercentage())
        box.m_top = Length(slices->top()->getDoubleValue(CSSPrimitiveValue::CSS_PERCENTAGE), Percent);
    else if (slices->top()->getValueID() != CSSValueAuto)
        box.m_top = slices->top()->computeLength<Length>(style(), rootElementStyle(), zoom);

    if (slices->right()->isNumber())
        box.m_right = Length(slices->right()->getIntValue(), Relative);
    else if (slices->right()->isPercentage())
        box.m_right = Length(slices->right()->getDoubleValue(CSSPrimitiveValue::CSS_PERCENTAGE), Percent);
    else if (slices->right()->getValueID() != CSSValueAuto)
        box.m_right = slices->right()->computeLength<Length>(style(), rootElementStyle(), zoom);

    if (slices->bottom()->isNumber())
        box.m_bottom = Length(slices->bottom()->getIntValue(), Relative);
    else if (slices->bottom()->isPercentage())
        box.m_bottom = Length(slices->bottom()->getDoubleValue(CSSPrimitiveValue::CSS_PERCENTAGE), Percent);
    else if (slices->bottom()->getValueID() != CSSValueAuto)
        box.m_bottom = slices->bottom()->computeLength<Length>(style(), rootElementStyle(), zoom);

    if (slices->left()->isNumber())
        box.m_left = Length(slices->left()->getIntValue(), Relative);
    else if (slices->left()->isPercentage())
        box.m_left = Length(slices->left()->getDoubleValue(CSSPrimitiveValue::CSS_PERCENTAGE), Percent);
    else if (slices->left()->getValueID() != CSSValueAuto)
        box.m_left = slices->left()->computeLength<Length>(style(), rootElementStyle(), zoom);

    return box;
}

void CSSToStyleMap::mapNinePieceImageRepeat(CSSValue* value, NinePieceImage& image)
{
    if (!value || !value->isPrimitiveValue())
        return;

    CSSPrimitiveValue* primitiveValue = static_cast<CSSPrimitiveValue*>(value);
    Pair* pair = primitiveValue->getPairValue();
    if (!pair || !pair->first() || !pair->second())
        return;

    CSSValueID firstIdentifier = pair->first()->getValueID();
    CSSValueID secondIdentifier = pair->second()->getValueID();

    ENinePieceImageRule horizontalRule;
    switch (firstIdentifier) {
    case CSSValueStretch:
        horizontalRule = StretchImageRule;
        break;
    case CSSValueRound:
        horizontalRule = RoundImageRule;
        break;
    case CSSValueSpace:
        horizontalRule = SpaceImageRule;
        break;
    default: // CSSValueRepeat
        horizontalRule = RepeatImageRule;
        break;
    }
    image.setHorizontalRule(horizontalRule);

    ENinePieceImageRule verticalRule;
    switch (secondIdentifier) {
    case CSSValueStretch:
        verticalRule = StretchImageRule;
        break;
    case CSSValueRound:
        verticalRule = RoundImageRule;
        break;
    case CSSValueSpace:
        verticalRule = SpaceImageRule;
        break;
    default: // CSSValueRepeat
        verticalRule = RepeatImageRule;
        break;
    }
    image.setVerticalRule(verticalRule);
}

};
