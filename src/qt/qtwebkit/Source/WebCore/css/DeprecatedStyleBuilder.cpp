/*
 * Copyright (C) 2011 Google Inc. All rights reserved.
 * Copyright (C) 2013 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "DeprecatedStyleBuilder.h"

#include "BasicShapeFunctions.h"
#include "BasicShapes.h"
#include "CSSAspectRatioValue.h"
#include "CSSCalculationValue.h"
#include "CSSCursorImageValue.h"
#include "CSSPrimitiveValueMappings.h"
#include "CSSToStyleMap.h"
#include "CSSValueList.h"
#include "ClipPathOperation.h"
#include "CursorList.h"
#include "Document.h"
#include "Element.h"
#include "Pair.h"
#include "Rect.h"
#include "RenderObject.h"
#include "RenderStyle.h"
#include "RenderView.h"
#include "Settings.h"
#include "StyleResolver.h"
#include <wtf/StdLibExtras.h>

#if ENABLE(CSS_SHAPES)
#include "ShapeValue.h"
#endif

using namespace std;

namespace WebCore {

enum ExpandValueBehavior {SuppressValue = 0, ExpandValue};
template <ExpandValueBehavior expandValue, CSSPropertyID one = CSSPropertyInvalid, CSSPropertyID two = CSSPropertyInvalid, CSSPropertyID three = CSSPropertyInvalid, CSSPropertyID four = CSSPropertyInvalid, CSSPropertyID five = CSSPropertyInvalid>
class ApplyPropertyExpanding {
public:

    template <CSSPropertyID id>
    static inline void applyInheritValue(CSSPropertyID propertyID, StyleResolver* styleResolver)
    {
        if (id == CSSPropertyInvalid)
            return;

        const DeprecatedStyleBuilder& table = DeprecatedStyleBuilder::sharedStyleBuilder();
        const PropertyHandler& handler = table.propertyHandler(id);
        if (handler.isValid())
            handler.applyInheritValue(propertyID, styleResolver);
    }

    static void applyInheritValue(CSSPropertyID propertyID, StyleResolver* styleResolver)
    {
        applyInheritValue<one>(propertyID, styleResolver);
        applyInheritValue<two>(propertyID, styleResolver);
        applyInheritValue<three>(propertyID, styleResolver);
        applyInheritValue<four>(propertyID, styleResolver);
        applyInheritValue<five>(propertyID, styleResolver);
    }

    template <CSSPropertyID id>
    static inline void applyInitialValue(CSSPropertyID propertyID, StyleResolver* styleResolver)
    {
        if (id == CSSPropertyInvalid)
            return;

        const DeprecatedStyleBuilder& table = DeprecatedStyleBuilder::sharedStyleBuilder();
        const PropertyHandler& handler = table.propertyHandler(id);
        if (handler.isValid())
            handler.applyInitialValue(propertyID, styleResolver);
    }

    static void applyInitialValue(CSSPropertyID propertyID, StyleResolver* styleResolver)
    {
        applyInitialValue<one>(propertyID, styleResolver);
        applyInitialValue<two>(propertyID, styleResolver);
        applyInitialValue<three>(propertyID, styleResolver);
        applyInitialValue<four>(propertyID, styleResolver);
        applyInitialValue<five>(propertyID, styleResolver);
    }

    template <CSSPropertyID id>
    static inline void applyValue(CSSPropertyID propertyID, StyleResolver* styleResolver, CSSValue* value)
    {
        if (id == CSSPropertyInvalid)
            return;

        const DeprecatedStyleBuilder& table = DeprecatedStyleBuilder::sharedStyleBuilder();
        const PropertyHandler& handler = table.propertyHandler(id);
        if (handler.isValid())
            handler.applyValue(propertyID, styleResolver, value);
    }

    static void applyValue(CSSPropertyID propertyID, StyleResolver* styleResolver, CSSValue* value)
    {
        if (!expandValue)
            return;

        applyValue<one>(propertyID, styleResolver, value);
        applyValue<two>(propertyID, styleResolver, value);
        applyValue<three>(propertyID, styleResolver, value);
        applyValue<four>(propertyID, styleResolver, value);
        applyValue<five>(propertyID, styleResolver, value);
    }
    static PropertyHandler createHandler() { return PropertyHandler(&applyInheritValue, &applyInitialValue, &applyValue); }
};

template <typename GetterType, GetterType (RenderStyle::*getterFunction)() const, typename SetterType, void (RenderStyle::*setterFunction)(SetterType), typename InitialType, InitialType (*initialFunction)()>
class ApplyPropertyDefaultBase {
public:
    static void setValue(RenderStyle* style, SetterType value) { (style->*setterFunction)(value); }
    static GetterType value(RenderStyle* style) { return (style->*getterFunction)(); }
    static InitialType initial() { return (*initialFunction)(); }
    static void applyInheritValue(CSSPropertyID, StyleResolver* styleResolver) { setValue(styleResolver->style(), value(styleResolver->parentStyle())); }
    static void applyInitialValue(CSSPropertyID, StyleResolver* styleResolver) { setValue(styleResolver->style(), initial()); }
    static void applyValue(CSSPropertyID, StyleResolver*, CSSValue*) { }
    static PropertyHandler createHandler() { return PropertyHandler(&applyInheritValue, &applyInitialValue, &applyValue); }
};

template <typename GetterType, GetterType (RenderStyle::*getterFunction)() const, typename SetterType, void (RenderStyle::*setterFunction)(SetterType), typename InitialType, InitialType (*initialFunction)()>
class ApplyPropertyDefault {
public:
    static void setValue(RenderStyle* style, SetterType value) { (style->*setterFunction)(value); }
    static void applyValue(CSSPropertyID, StyleResolver* styleResolver, CSSValue* value)
    {
        if (value->isPrimitiveValue())
            setValue(styleResolver->style(), *static_cast<CSSPrimitiveValue*>(value));
    }
    static PropertyHandler createHandler()
    {
        PropertyHandler handler = ApplyPropertyDefaultBase<GetterType, getterFunction, SetterType, setterFunction, InitialType, initialFunction>::createHandler();
        return PropertyHandler(handler.inheritFunction(), handler.initialFunction(), &applyValue);
    }
};

template <typename NumberType, NumberType (RenderStyle::*getterFunction)() const, void (RenderStyle::*setterFunction)(NumberType), NumberType (*initialFunction)(), int idMapsToMinusOne = CSSValueAuto>
class ApplyPropertyNumber {
public:
    static void setValue(RenderStyle* style, NumberType value) { (style->*setterFunction)(value); }
    static void applyValue(CSSPropertyID, StyleResolver* styleResolver, CSSValue* value)
    {
        if (!value->isPrimitiveValue())
            return;

        CSSPrimitiveValue* primitiveValue = static_cast<CSSPrimitiveValue*>(value);
        if (primitiveValue->getValueID() == idMapsToMinusOne)
            setValue(styleResolver->style(), -1);
        else
            setValue(styleResolver->style(), primitiveValue->getValue<NumberType>(CSSPrimitiveValue::CSS_NUMBER));
    }
    static PropertyHandler createHandler()
    {
        PropertyHandler handler = ApplyPropertyDefaultBase<NumberType, getterFunction, NumberType, setterFunction, NumberType, initialFunction>::createHandler();
        return PropertyHandler(handler.inheritFunction(), handler.initialFunction(), &applyValue);
    }
};

template <StyleImage* (RenderStyle::*getterFunction)() const, void (RenderStyle::*setterFunction)(PassRefPtr<StyleImage>), StyleImage* (*initialFunction)(), CSSPropertyID property>
class ApplyPropertyStyleImage {
public:
    static void applyValue(CSSPropertyID, StyleResolver* styleResolver, CSSValue* value) { (styleResolver->style()->*setterFunction)(styleResolver->styleImage(property, value)); }
    static PropertyHandler createHandler()
    {
        PropertyHandler handler = ApplyPropertyDefaultBase<StyleImage*, getterFunction, PassRefPtr<StyleImage>, setterFunction, StyleImage*, initialFunction>::createHandler();
        return PropertyHandler(handler.inheritFunction(), handler.initialFunction(), &applyValue);
    }
};

enum AutoValueType {Number = 0, ComputeLength};
template <typename T, T (RenderStyle::*getterFunction)() const, void (RenderStyle::*setterFunction)(T), bool (RenderStyle::*hasAutoFunction)() const, void (RenderStyle::*setAutoFunction)(), AutoValueType valueType = Number, int autoIdentity = CSSValueAuto>
class ApplyPropertyAuto {
public:
    static void setValue(RenderStyle* style, T value) { (style->*setterFunction)(value); }
    static T value(RenderStyle* style) { return (style->*getterFunction)(); }
    static bool hasAuto(RenderStyle* style) { return (style->*hasAutoFunction)(); }
    static void setAuto(RenderStyle* style) { (style->*setAutoFunction)(); }

    static void applyInheritValue(CSSPropertyID, StyleResolver* styleResolver)
    {
        if (hasAuto(styleResolver->parentStyle()))
            setAuto(styleResolver->style());
        else
            setValue(styleResolver->style(), value(styleResolver->parentStyle()));
    }

    static void applyInitialValue(CSSPropertyID, StyleResolver* styleResolver) { setAuto(styleResolver->style()); }

    static void applyValue(CSSPropertyID, StyleResolver* styleResolver, CSSValue* value)
    {
        if (!value->isPrimitiveValue())
            return;

        CSSPrimitiveValue* primitiveValue = static_cast<CSSPrimitiveValue*>(value);
        if (primitiveValue->getValueID() == autoIdentity)
            setAuto(styleResolver->style());
        else if (valueType == Number)
            setValue(styleResolver->style(), *primitiveValue);
        else if (valueType == ComputeLength)
            setValue(styleResolver->style(), primitiveValue->computeLength<T>(styleResolver->style(), styleResolver->rootElementStyle(), styleResolver->style()->effectiveZoom()));
    }

    static PropertyHandler createHandler() { return PropertyHandler(&applyInheritValue, &applyInitialValue, &applyValue); }
};

class ApplyPropertyClip {
private:
    static Length convertToLength(StyleResolver* styleResolver, CSSPrimitiveValue* value)
    {
        return value->convertToLength<FixedIntegerConversion | PercentConversion | FractionConversion | AutoConversion>(styleResolver->style(), styleResolver->rootElementStyle(), styleResolver->style()->effectiveZoom());
    }
public:
    static void applyInheritValue(CSSPropertyID propertyID, StyleResolver* styleResolver)
    {
        RenderStyle* parentStyle = styleResolver->parentStyle();
        if (!parentStyle->hasClip())
            return applyInitialValue(propertyID, styleResolver);
        styleResolver->style()->setClip(parentStyle->clipTop(), parentStyle->clipRight(), parentStyle->clipBottom(), parentStyle->clipLeft());
        styleResolver->style()->setHasClip(true);
    }

    static void applyInitialValue(CSSPropertyID, StyleResolver* styleResolver)
    {
        styleResolver->style()->setClip(Length(), Length(), Length(), Length());
        styleResolver->style()->setHasClip(false);
    }

    static void applyValue(CSSPropertyID, StyleResolver* styleResolver, CSSValue* value)
    {
        if (!value->isPrimitiveValue())
            return;

        CSSPrimitiveValue* primitiveValue = static_cast<CSSPrimitiveValue*>(value);

        if (Rect* rect = primitiveValue->getRectValue()) {
            Length top = convertToLength(styleResolver, rect->top());
            Length right = convertToLength(styleResolver, rect->right());
            Length bottom = convertToLength(styleResolver, rect->bottom());
            Length left = convertToLength(styleResolver, rect->left());
            styleResolver->style()->setClip(top, right, bottom, left);
            styleResolver->style()->setHasClip(true);
        } else if (primitiveValue->getValueID() == CSSValueAuto) {
            styleResolver->style()->setClip(Length(), Length(), Length(), Length());
            styleResolver->style()->setHasClip(false);
        }
    }

    static PropertyHandler createHandler() { return PropertyHandler(&applyInheritValue, &applyInitialValue, &applyValue); }
};

enum ColorInherit {NoInheritFromParent = 0, InheritFromParent};
Color defaultInitialColor();
Color defaultInitialColor() { return Color(); }
template <ColorInherit inheritColorFromParent,
          Color (RenderStyle::*getterFunction)() const,
          void (RenderStyle::*setterFunction)(const Color&),
          void (RenderStyle::*visitedLinkSetterFunction)(const Color&),
          Color (RenderStyle::*defaultFunction)() const,
          Color (*initialFunction)() = &defaultInitialColor>
class ApplyPropertyColor {
public:
    static void applyInheritValue(CSSPropertyID, StyleResolver* styleResolver)
    {
        // Visited link style can never explicitly inherit from parent visited link style so no separate getters are needed.
        Color color = (styleResolver->parentStyle()->*getterFunction)();
        applyColorValue(styleResolver, color.isValid() ? color : (styleResolver->parentStyle()->*defaultFunction)());
    }

    static void applyInitialValue(CSSPropertyID, StyleResolver* styleResolver)
    {
        applyColorValue(styleResolver, initialFunction());
    }

    static void applyValue(CSSPropertyID propertyID, StyleResolver* styleResolver, CSSValue* value)
    {
        if (!value->isPrimitiveValue())
            return;

        CSSPrimitiveValue* primitiveValue = static_cast<CSSPrimitiveValue*>(value);
        if (inheritColorFromParent && primitiveValue->getValueID() == CSSValueCurrentcolor)
            applyInheritValue(propertyID, styleResolver);
        else {
            if (styleResolver->applyPropertyToRegularStyle())
                (styleResolver->style()->*setterFunction)(styleResolver->colorFromPrimitiveValue(primitiveValue));
            if (styleResolver->applyPropertyToVisitedLinkStyle())
                (styleResolver->style()->*visitedLinkSetterFunction)(styleResolver->colorFromPrimitiveValue(primitiveValue, /* forVisitedLink */ true));
        }
    }

    static void applyColorValue(StyleResolver* styleResolver, const Color& color)
    {
        if (styleResolver->applyPropertyToRegularStyle())
            (styleResolver->style()->*setterFunction)(color);
        if (styleResolver->applyPropertyToVisitedLinkStyle())
            (styleResolver->style()->*visitedLinkSetterFunction)(color);
    }

    static PropertyHandler createHandler() { return PropertyHandler(&applyInheritValue, &applyInitialValue, &applyValue); }
};

template <TextDirection (RenderStyle::*getterFunction)() const, void (RenderStyle::*setterFunction)(TextDirection), TextDirection (*initialFunction)()>
class ApplyPropertyDirection {
public:
    static void applyValue(CSSPropertyID propertyID, StyleResolver* styleResolver, CSSValue* value)
    {
        ApplyPropertyDefault<TextDirection, getterFunction, TextDirection, setterFunction, TextDirection, initialFunction>::applyValue(propertyID, styleResolver, value);
        Element* element = styleResolver->element();
        if (element && styleResolver->element() == element->document()->documentElement())
            element->document()->setDirectionSetOnDocumentElement(true);
    }

    static PropertyHandler createHandler()
    {
        PropertyHandler handler = ApplyPropertyDefault<TextDirection, getterFunction, TextDirection, setterFunction, TextDirection, initialFunction>::createHandler();
        return PropertyHandler(handler.inheritFunction(), handler.initialFunction(), &applyValue);
    }
};

enum LengthAuto { AutoDisabled = 0, AutoEnabled };
enum LengthLegacyIntrinsic { LegacyIntrinsicDisabled = 0, LegacyIntrinsicEnabled };
enum LengthIntrinsic { IntrinsicDisabled = 0, IntrinsicEnabled };
enum LengthNone { NoneDisabled = 0, NoneEnabled };
enum LengthUndefined { UndefinedDisabled = 0, UndefinedEnabled };
template <Length (RenderStyle::*getterFunction)() const,
          void (RenderStyle::*setterFunction)(Length),
          Length (*initialFunction)(),
          LengthAuto autoEnabled = AutoDisabled,
          LengthLegacyIntrinsic legacyIntrinsicEnabled = LegacyIntrinsicDisabled,
          LengthIntrinsic intrinsicEnabled = IntrinsicDisabled,
          LengthNone noneEnabled = NoneDisabled,
          LengthUndefined noneUndefined = UndefinedDisabled>
class ApplyPropertyLength {
public:
    static void setValue(RenderStyle* style, Length value) { (style->*setterFunction)(value); }
    static void applyValue(CSSPropertyID, StyleResolver* styleResolver, CSSValue* value)
    {
        if (!value->isPrimitiveValue())
            return;

        CSSPrimitiveValue* primitiveValue = static_cast<CSSPrimitiveValue*>(value);
        if (noneEnabled && primitiveValue->getValueID() == CSSValueNone) {
            if (noneUndefined)
                setValue(styleResolver->style(), Length(Undefined));
            else
                setValue(styleResolver->style(), Length());
        }
        if (legacyIntrinsicEnabled) {
            if (primitiveValue->getValueID() == CSSValueIntrinsic)
                setValue(styleResolver->style(), Length(Intrinsic));
            else if (primitiveValue->getValueID() == CSSValueMinIntrinsic)
                setValue(styleResolver->style(), Length(MinIntrinsic));
        }
        if (intrinsicEnabled) {
            if (primitiveValue->getValueID() == CSSValueWebkitMinContent)
                setValue(styleResolver->style(), Length(MinContent));
            else if (primitiveValue->getValueID() == CSSValueWebkitMaxContent)
                setValue(styleResolver->style(), Length(MaxContent));
            else if (primitiveValue->getValueID() == CSSValueWebkitFillAvailable)
                setValue(styleResolver->style(), Length(FillAvailable));
            else if (primitiveValue->getValueID() == CSSValueWebkitFitContent)
                setValue(styleResolver->style(), Length(FitContent));
        }

        if (autoEnabled && primitiveValue->getValueID() == CSSValueAuto)
            setValue(styleResolver->style(), Length());
        else if (primitiveValue->isLength()) {
            Length length = primitiveValue->computeLength<Length>(styleResolver->style(), styleResolver->rootElementStyle(), styleResolver->style()->effectiveZoom());
            length.setQuirk(primitiveValue->isQuirkValue());
            setValue(styleResolver->style(), length);
        } else if (primitiveValue->isPercentage())
            setValue(styleResolver->style(), Length(primitiveValue->getDoubleValue(), Percent));
        else if (primitiveValue->isCalculatedPercentageWithLength())
            setValue(styleResolver->style(), Length(primitiveValue->cssCalcValue()->toCalcValue(styleResolver->style(), styleResolver->rootElementStyle(), styleResolver->style()->effectiveZoom())));
        else if (primitiveValue->isViewportPercentageLength())
            setValue(styleResolver->style(), primitiveValue->viewportPercentageLength());
    }

    static PropertyHandler createHandler()
    {
        PropertyHandler handler = ApplyPropertyDefaultBase<Length, getterFunction, Length, setterFunction, Length, initialFunction>::createHandler();
        return PropertyHandler(handler.inheritFunction(), handler.initialFunction(), &applyValue);
    }
};

enum StringIdentBehavior { NothingMapsToNull = 0, MapNoneToNull, MapAutoToNull };
template <StringIdentBehavior identBehavior, const AtomicString& (RenderStyle::*getterFunction)() const, void (RenderStyle::*setterFunction)(const AtomicString&), const AtomicString& (*initialFunction)()>
class ApplyPropertyString {
public:
    static void setValue(RenderStyle* style, const AtomicString& value) { (style->*setterFunction)(value); }
    static void applyValue(CSSPropertyID, StyleResolver* styleResolver, CSSValue* value)
    {
        if (!value->isPrimitiveValue())
            return;
        CSSPrimitiveValue* primitiveValue = static_cast<CSSPrimitiveValue*>(value);
        if ((identBehavior == MapNoneToNull && primitiveValue->getValueID() == CSSValueNone)
            || (identBehavior == MapAutoToNull && primitiveValue->getValueID() == CSSValueAuto))
            setValue(styleResolver->style(), nullAtom);
        else
            setValue(styleResolver->style(), primitiveValue->getStringValue());
    }
    static PropertyHandler createHandler()
    {
        PropertyHandler handler = ApplyPropertyDefaultBase<const AtomicString&, getterFunction, const AtomicString&, setterFunction, const AtomicString&, initialFunction>::createHandler();
        return PropertyHandler(handler.inheritFunction(), handler.initialFunction(), &applyValue);
    }
};

template <LengthSize (RenderStyle::*getterFunction)() const, void (RenderStyle::*setterFunction)(LengthSize), LengthSize (*initialFunction)()>
class ApplyPropertyBorderRadius {
public:
    static void setValue(RenderStyle* style, LengthSize value) { (style->*setterFunction)(value); }
    static void applyValue(CSSPropertyID, StyleResolver* styleResolver, CSSValue* value)
    {
        if (!value->isPrimitiveValue())
            return;

        CSSPrimitiveValue* primitiveValue = static_cast<CSSPrimitiveValue*>(value);
        Pair* pair = primitiveValue->getPairValue();
        if (!pair || !pair->first() || !pair->second())
            return;

        Length radiusWidth;
        Length radiusHeight;
        if (pair->first()->isPercentage())
            radiusWidth = Length(pair->first()->getDoubleValue(), Percent);
        else if (pair->first()->isViewportPercentageLength())
            radiusWidth = pair->first()->viewportPercentageLength();
        else if (pair->first()->isCalculatedPercentageWithLength())
            radiusWidth = Length((pair->first()->cssCalcValue()->toCalcValue(styleResolver->style(), styleResolver->rootElementStyle(), styleResolver->style()->effectiveZoom())));
        else
            radiusWidth = pair->first()->computeLength<Length>(styleResolver->style(), styleResolver->rootElementStyle(), styleResolver->style()->effectiveZoom());
        if (pair->second()->isPercentage())
            radiusHeight = Length(pair->second()->getDoubleValue(), Percent);
        else if (pair->second()->isViewportPercentageLength())
            radiusHeight = pair->second()->viewportPercentageLength();
        else if (pair->second()->isCalculatedPercentageWithLength())
            radiusHeight = Length((pair->second()->cssCalcValue()->toCalcValue(styleResolver->style(), styleResolver->rootElementStyle(), styleResolver->style()->effectiveZoom())));
        else
            radiusHeight = pair->second()->computeLength<Length>(styleResolver->style(), styleResolver->rootElementStyle(), styleResolver->style()->effectiveZoom());
        int width = radiusWidth.value();
        int height = radiusHeight.value();
        if (width < 0 || height < 0)
            return;
        if (!width)
            radiusHeight = radiusWidth; // Null out the other value.
        else if (!height)
            radiusWidth = radiusHeight; // Null out the other value.

        LengthSize size(radiusWidth, radiusHeight);
        setValue(styleResolver->style(), size);
    }
    static PropertyHandler createHandler()
    {
        PropertyHandler handler = ApplyPropertyDefaultBase<LengthSize, getterFunction, LengthSize, setterFunction, LengthSize, initialFunction>::createHandler();
        return PropertyHandler(handler.inheritFunction(), handler.initialFunction(), &applyValue);
    }
};

template <typename T>
struct FillLayerAccessorTypes {
    typedef T Setter;
    typedef T Getter;
};

template <>
struct FillLayerAccessorTypes<StyleImage*> {
    typedef PassRefPtr<StyleImage> Setter;
    typedef StyleImage* Getter;
};

template <typename T,
          CSSPropertyID propertyId,
          EFillLayerType fillLayerType,
          FillLayer* (RenderStyle::*accessLayersFunction)(),
          const FillLayer* (RenderStyle::*layersFunction)() const,
          bool (FillLayer::*testFunction)() const,
          typename FillLayerAccessorTypes<T>::Getter (FillLayer::*getFunction)() const,
          void (FillLayer::*setFunction)(typename FillLayerAccessorTypes<T>::Setter),
          void (FillLayer::*clearFunction)(),
          typename FillLayerAccessorTypes<T>::Getter (*initialFunction)(EFillLayerType),
          void (CSSToStyleMap::*mapFillFunction)(CSSPropertyID, FillLayer*, CSSValue*)>
class ApplyPropertyFillLayer {
public:
    static void applyInheritValue(CSSPropertyID, StyleResolver* styleResolver)
    {
        FillLayer* currChild = (styleResolver->style()->*accessLayersFunction)();
        FillLayer* prevChild = 0;
        const FillLayer* currParent = (styleResolver->parentStyle()->*layersFunction)();
        while (currParent && (currParent->*testFunction)()) {
            if (!currChild) {
                /* Need to make a new layer.*/
                currChild = new FillLayer(fillLayerType);
                prevChild->setNext(currChild);
            }
            (currChild->*setFunction)((currParent->*getFunction)());
            prevChild = currChild;
            currChild = prevChild->next();
            currParent = currParent->next();
        }

        while (currChild) {
            /* Reset any remaining layers to not have the property set. */
            (currChild->*clearFunction)();
            currChild = currChild->next();
        }
    }

    static void applyInitialValue(CSSPropertyID, StyleResolver* styleResolver)
    {
        FillLayer* currChild = (styleResolver->style()->*accessLayersFunction)();
        (currChild->*setFunction)((*initialFunction)(fillLayerType));
        for (currChild = currChild->next(); currChild; currChild = currChild->next())
            (currChild->*clearFunction)();
    }

    static void applyValue(CSSPropertyID, StyleResolver* styleResolver, CSSValue* value)
    {
        FillLayer* currChild = (styleResolver->style()->*accessLayersFunction)();
        FillLayer* prevChild = 0;
        if (value->isValueList()
#if ENABLE(CSS_IMAGE_SET)
        && !value->isImageSetValue()
#endif
        ) {
            /* Walk each value and put it into a layer, creating new layers as needed. */
            CSSValueList* valueList = static_cast<CSSValueList*>(value);
            for (unsigned int i = 0; i < valueList->length(); i++) {
                if (!currChild) {
                    /* Need to make a new layer to hold this value */
                    currChild = new FillLayer(fillLayerType);
                    prevChild->setNext(currChild);
                }
                (styleResolver->styleMap()->*mapFillFunction)(propertyId, currChild, valueList->itemWithoutBoundsCheck(i));
                prevChild = currChild;
                currChild = currChild->next();
            }
        } else {
            (styleResolver->styleMap()->*mapFillFunction)(propertyId, currChild, value);
            currChild = currChild->next();
        }
        while (currChild) {
            /* Reset all remaining layers to not have the property set. */
            (currChild->*clearFunction)();
            currChild = currChild->next();
        }
    }

    static PropertyHandler createHandler() { return PropertyHandler(&applyInheritValue, &applyInitialValue, &applyValue); }
};

enum ComputeLengthNormal {NormalDisabled = 0, NormalEnabled};
enum ComputeLengthThickness {ThicknessDisabled = 0, ThicknessEnabled};
enum ComputeLengthSVGZoom {SVGZoomDisabled = 0, SVGZoomEnabled};
template <typename T,
          T (RenderStyle::*getterFunction)() const,
          void (RenderStyle::*setterFunction)(T),
          T (*initialFunction)(),
          ComputeLengthNormal normalEnabled = NormalDisabled,
          ComputeLengthThickness thicknessEnabled = ThicknessDisabled,
          ComputeLengthSVGZoom svgZoomEnabled = SVGZoomDisabled>
class ApplyPropertyComputeLength {
public:
    static void setValue(RenderStyle* style, T value) { (style->*setterFunction)(value); }
    static void applyValue(CSSPropertyID, StyleResolver* styleResolver, CSSValue* value)
    {
        // note: CSSPropertyLetter/WordSpacing right now sets to zero if it's not a primitive value for some reason...
        if (!value->isPrimitiveValue())
            return;

        CSSPrimitiveValue* primitiveValue = static_cast<CSSPrimitiveValue*>(value);

        CSSValueID ident = primitiveValue->getValueID();
        T length;
        if (normalEnabled && ident == CSSValueNormal) {
            length = 0;
        } else if (thicknessEnabled && ident == CSSValueThin) {
            length = 1;
        } else if (thicknessEnabled && ident == CSSValueMedium) {
            length = 3;
        } else if (thicknessEnabled && ident == CSSValueThick) {
            length = 5;
        } else if (ident == CSSValueInvalid) {
            float zoom = (svgZoomEnabled && styleResolver->useSVGZoomRules()) ? 1.0f : styleResolver->style()->effectiveZoom();

            // Any original result that was >= 1 should not be allowed to fall below 1.
            // This keeps border lines from vanishing.
            length = primitiveValue->computeLength<T>(styleResolver->style(), styleResolver->rootElementStyle(), zoom);
            if (zoom < 1.0f && length < 1.0) {
                T originalLength = primitiveValue->computeLength<T>(styleResolver->style(), styleResolver->rootElementStyle(), 1.0);
                if (originalLength >= 1.0)
                    length = 1.0;
            }

        } else {
            ASSERT_NOT_REACHED();
            length = 0;
        }

        setValue(styleResolver->style(), length);
    }
    static PropertyHandler createHandler()
    {
        PropertyHandler handler = ApplyPropertyDefaultBase<T, getterFunction, T, setterFunction, T, initialFunction>::createHandler();
        return PropertyHandler(handler.inheritFunction(), handler.initialFunction(), &applyValue);
    }
};

template <typename T, T (FontDescription::*getterFunction)() const, void (FontDescription::*setterFunction)(T), T initialValue>
class ApplyPropertyFont {
public:
    static void applyInheritValue(CSSPropertyID, StyleResolver* styleResolver)
    {
        FontDescription fontDescription = styleResolver->fontDescription();
        (fontDescription.*setterFunction)((styleResolver->parentFontDescription().*getterFunction)());
        styleResolver->setFontDescription(fontDescription);
    }

    static void applyInitialValue(CSSPropertyID, StyleResolver* styleResolver)
    {
        FontDescription fontDescription = styleResolver->fontDescription();
        (fontDescription.*setterFunction)(initialValue);
        styleResolver->setFontDescription(fontDescription);
    }

    static void applyValue(CSSPropertyID, StyleResolver* styleResolver, CSSValue* value)
    {
        if (!value->isPrimitiveValue())
            return;
        CSSPrimitiveValue* primitiveValue = static_cast<CSSPrimitiveValue*>(value);
        FontDescription fontDescription = styleResolver->fontDescription();
        (fontDescription.*setterFunction)(*primitiveValue);
        styleResolver->setFontDescription(fontDescription);
    }

    static PropertyHandler createHandler() { return PropertyHandler(&applyInheritValue, &applyInitialValue, &applyValue); }
};

class ApplyPropertyFontFamily {
public:
    static void applyInheritValue(CSSPropertyID, StyleResolver* styleResolver)
    {
        FontDescription fontDescription = styleResolver->style()->fontDescription();
        FontDescription parentFontDescription = styleResolver->parentStyle()->fontDescription();
        
        fontDescription.setGenericFamily(parentFontDescription.genericFamily());
        fontDescription.setFamilies(parentFontDescription.families());
        fontDescription.setIsSpecifiedFont(parentFontDescription.isSpecifiedFont());
        styleResolver->setFontDescription(fontDescription);
        return;
    }

    static void applyInitialValue(CSSPropertyID, StyleResolver* styleResolver)
    {
        FontDescription fontDescription = styleResolver->style()->fontDescription();
        FontDescription initialDesc = FontDescription();
        
        // We need to adjust the size to account for the generic family change from monospace to non-monospace.
        if (fontDescription.keywordSize() && fontDescription.useFixedDefaultSize())
            styleResolver->setFontSize(fontDescription, styleResolver->fontSizeForKeyword(styleResolver->document(), CSSValueXxSmall + fontDescription.keywordSize() - 1, false));
        fontDescription.setGenericFamily(initialDesc.genericFamily());
        if (!initialDesc.firstFamily().isEmpty())
            fontDescription.setFamilies(initialDesc.families());

        styleResolver->setFontDescription(fontDescription);
        return;
    }

    static void applyValue(CSSPropertyID, StyleResolver* styleResolver, CSSValue* value)
    {
        if (!value->isValueList())
            return;

        FontDescription fontDescription = styleResolver->style()->fontDescription();
        // Before mapping in a new font-family property, we should reset the generic family.
        bool oldFamilyUsedFixedDefaultSize = fontDescription.useFixedDefaultSize();
        fontDescription.setGenericFamily(FontDescription::NoFamily);

        Vector<AtomicString, 1> families;
        for (CSSValueListIterator i = value; i.hasMore(); i.advance()) {
            CSSValue* item = i.value();
            if (!item->isPrimitiveValue())
                continue;
            CSSPrimitiveValue* contentValue = static_cast<CSSPrimitiveValue*>(item);
            AtomicString face;
            if (contentValue->isString())
                face = contentValue->getStringValue();
            else if (Settings* settings = styleResolver->document()->settings()) {
                switch (contentValue->getValueID()) {
                case CSSValueWebkitBody:
                    face = settings->standardFontFamily();
                    break;
                case CSSValueSerif:
                    face = serifFamily;
                    fontDescription.setGenericFamily(FontDescription::SerifFamily);
                    break;
                case CSSValueSansSerif:
                    face = sansSerifFamily;
                    fontDescription.setGenericFamily(FontDescription::SansSerifFamily);
                    break;
                case CSSValueCursive:
                    face = cursiveFamily;
                    fontDescription.setGenericFamily(FontDescription::CursiveFamily);
                    break;
                case CSSValueFantasy:
                    face = fantasyFamily;
                    fontDescription.setGenericFamily(FontDescription::FantasyFamily);
                    break;
                case CSSValueMonospace:
                    face = monospaceFamily;
                    fontDescription.setGenericFamily(FontDescription::MonospaceFamily);
                    break;
                case CSSValueWebkitPictograph:
                    face = pictographFamily;
                    fontDescription.setGenericFamily(FontDescription::PictographFamily);
                    break;
                default:
                    break;
                }
            }

            if (face.isEmpty())
                continue;
            if (families.isEmpty())
                fontDescription.setIsSpecifiedFont(fontDescription.genericFamily() == FontDescription::NoFamily);
            families.append(face);
        }

        if (families.isEmpty())
            return;
        fontDescription.adoptFamilies(families);

        if (fontDescription.keywordSize() && fontDescription.useFixedDefaultSize() != oldFamilyUsedFixedDefaultSize)
            styleResolver->setFontSize(fontDescription, styleResolver->fontSizeForKeyword(styleResolver->document(), CSSValueXxSmall + fontDescription.keywordSize() - 1, !oldFamilyUsedFixedDefaultSize));

        styleResolver->setFontDescription(fontDescription);
    }

    static PropertyHandler createHandler() { return PropertyHandler(&applyInheritValue, &applyInitialValue, &applyValue); }
};

class ApplyPropertyFontSize {
private:
    // When the CSS keyword "larger" is used, this function will attempt to match within the keyword
    // table, and failing that, will simply multiply by 1.2.
    static float largerFontSize(float size)
    {
        // FIXME: Figure out where we fall in the size ranges (xx-small to xxx-large) and scale up to
        // the next size level.
        return size * 1.2f;
    }

    // Like the previous function, but for the keyword "smaller".
    static float smallerFontSize(float size)
    {
        // FIXME: Figure out where we fall in the size ranges (xx-small to xxx-large) and scale down to
        // the next size level.
        return size / 1.2f;
    }
public:
    static void applyInheritValue(CSSPropertyID, StyleResolver* styleResolver)
    {
        float size = styleResolver->parentStyle()->fontDescription().specifiedSize();

        if (size < 0)
            return;

        FontDescription fontDescription = styleResolver->style()->fontDescription();
        fontDescription.setKeywordSize(styleResolver->parentStyle()->fontDescription().keywordSize());
        styleResolver->setFontSize(fontDescription, size);
        styleResolver->setFontDescription(fontDescription);
        return;
    }

    static void applyInitialValue(CSSPropertyID, StyleResolver* styleResolver)
    {
        FontDescription fontDescription = styleResolver->style()->fontDescription();
        float size = styleResolver->fontSizeForKeyword(styleResolver->document(), CSSValueMedium, fontDescription.useFixedDefaultSize());

        if (size < 0)
            return;

        fontDescription.setKeywordSize(CSSValueMedium - CSSValueXxSmall + 1);
        styleResolver->setFontSize(fontDescription, size);
        styleResolver->setFontDescription(fontDescription);
        return;
    }

    static void applyValue(CSSPropertyID, StyleResolver* styleResolver, CSSValue* value)
    {
        if (!value->isPrimitiveValue())
            return;

        CSSPrimitiveValue* primitiveValue = static_cast<CSSPrimitiveValue*>(value);

        FontDescription fontDescription = styleResolver->style()->fontDescription();
        fontDescription.setKeywordSize(0);
        float parentSize = 0;
        bool parentIsAbsoluteSize = false;
        float size = 0;

        if (styleResolver->parentStyle()) {
            parentSize = styleResolver->parentStyle()->fontDescription().specifiedSize();
            parentIsAbsoluteSize = styleResolver->parentStyle()->fontDescription().isAbsoluteSize();
        }

        if (CSSValueID ident = primitiveValue->getValueID()) {
            // Keywords are being used.
            switch (ident) {
            case CSSValueXxSmall:
            case CSSValueXSmall:
            case CSSValueSmall:
            case CSSValueMedium:
            case CSSValueLarge:
            case CSSValueXLarge:
            case CSSValueXxLarge:
            case CSSValueWebkitXxxLarge:
                size = styleResolver->fontSizeForKeyword(styleResolver->document(), ident, fontDescription.useFixedDefaultSize());
                fontDescription.setKeywordSize(ident - CSSValueXxSmall + 1);
                break;
            case CSSValueLarger:
                size = largerFontSize(parentSize);
                break;
            case CSSValueSmaller:
                size = smallerFontSize(parentSize);
                break;
            default:
                return;
            }

            fontDescription.setIsAbsoluteSize(parentIsAbsoluteSize && (ident == CSSValueLarger || ident == CSSValueSmaller));
        } else {
            fontDescription.setIsAbsoluteSize(parentIsAbsoluteSize
                                              || !(primitiveValue->isPercentage() || primitiveValue->isFontRelativeLength()));
            if (primitiveValue->isLength())
                size = primitiveValue->computeLength<float>(styleResolver->parentStyle(), styleResolver->rootElementStyle(), 1.0, true);
            else if (primitiveValue->isPercentage())
                size = (primitiveValue->getFloatValue() * parentSize) / 100.0f;
            else if (primitiveValue->isCalculatedPercentageWithLength())
                size = primitiveValue->cssCalcValue()->toCalcValue(styleResolver->parentStyle(), styleResolver->rootElementStyle())->evaluate(parentSize);
            else if (primitiveValue->isViewportPercentageLength())
                size = valueForLength(primitiveValue->viewportPercentageLength(), 0, styleResolver->document()->renderView());
            else
                return;
        }

        if (size < 0)
            return;

        // Overly large font sizes will cause crashes on some platforms (such as Windows).
        // Cap font size here to make sure that doesn't happen.
        size = min(maximumAllowedFontSize, size);

        styleResolver->setFontSize(fontDescription, size);
        styleResolver->setFontDescription(fontDescription);
        return;
    }

    static PropertyHandler createHandler() { return PropertyHandler(&applyInheritValue, &applyInitialValue, &applyValue); }
};

class ApplyPropertyFontWeight {
public:
    static void applyValue(CSSPropertyID, StyleResolver* styleResolver, CSSValue* value)
    {
        if (!value->isPrimitiveValue())
            return;
        CSSPrimitiveValue* primitiveValue = static_cast<CSSPrimitiveValue*>(value);
        FontDescription fontDescription = styleResolver->fontDescription();
        switch (primitiveValue->getValueID()) {
        case CSSValueInvalid:
            ASSERT_NOT_REACHED();
            break;
        case CSSValueBolder:
            fontDescription.setWeight(fontDescription.bolderWeight());
            break;
        case CSSValueLighter:
            fontDescription.setWeight(fontDescription.lighterWeight());
            break;
        default:
            fontDescription.setWeight(*primitiveValue);
        }
        styleResolver->setFontDescription(fontDescription);
    }
    static PropertyHandler createHandler()
    {
        PropertyHandler handler = ApplyPropertyFont<FontWeight, &FontDescription::weight, &FontDescription::setWeight, FontWeightNormal>::createHandler();
        return PropertyHandler(handler.inheritFunction(), handler.initialFunction(), &applyValue);
    }
};

class ApplyPropertyFontVariantLigatures {
public:
    static void applyInheritValue(CSSPropertyID, StyleResolver* styleResolver)
    {
        const FontDescription& parentFontDescription = styleResolver->parentFontDescription();
        FontDescription fontDescription = styleResolver->fontDescription();

        fontDescription.setCommonLigaturesState(parentFontDescription.commonLigaturesState());
        fontDescription.setDiscretionaryLigaturesState(parentFontDescription.discretionaryLigaturesState());
        fontDescription.setHistoricalLigaturesState(parentFontDescription.historicalLigaturesState());

        styleResolver->setFontDescription(fontDescription);
    }

    static void applyInitialValue(CSSPropertyID, StyleResolver* styleResolver)
    {
        FontDescription fontDescription = styleResolver->fontDescription();

        fontDescription.setCommonLigaturesState(FontDescription::NormalLigaturesState);
        fontDescription.setDiscretionaryLigaturesState(FontDescription::NormalLigaturesState);
        fontDescription.setHistoricalLigaturesState(FontDescription::NormalLigaturesState);

        styleResolver->setFontDescription(fontDescription);
    }

    static void applyValue(CSSPropertyID, StyleResolver* styleResolver, CSSValue* value)
    {
        FontDescription::LigaturesState commonLigaturesState = FontDescription::NormalLigaturesState;
        FontDescription::LigaturesState discretionaryLigaturesState = FontDescription::NormalLigaturesState;
        FontDescription::LigaturesState historicalLigaturesState = FontDescription::NormalLigaturesState;

        if (value->isValueList()) {
            CSSValueList* valueList = static_cast<CSSValueList*>(value);
            for (size_t i = 0; i < valueList->length(); ++i) {
                CSSValue* item = valueList->itemWithoutBoundsCheck(i);
                ASSERT(item->isPrimitiveValue());
                if (item->isPrimitiveValue()) {
                    CSSPrimitiveValue* primitiveValue = static_cast<CSSPrimitiveValue*>(item);
                    switch (primitiveValue->getValueID()) {
                    case CSSValueNoCommonLigatures:
                        commonLigaturesState = FontDescription::DisabledLigaturesState;
                        break;
                    case CSSValueCommonLigatures:
                        commonLigaturesState = FontDescription::EnabledLigaturesState;
                        break;
                    case CSSValueNoDiscretionaryLigatures:
                        discretionaryLigaturesState = FontDescription::DisabledLigaturesState;
                        break;
                    case CSSValueDiscretionaryLigatures:
                        discretionaryLigaturesState = FontDescription::EnabledLigaturesState;
                        break;
                    case CSSValueNoHistoricalLigatures:
                        historicalLigaturesState = FontDescription::DisabledLigaturesState;
                        break;
                    case CSSValueHistoricalLigatures:
                        historicalLigaturesState = FontDescription::EnabledLigaturesState;
                        break;
                    default:
                        ASSERT_NOT_REACHED();
                        break;
                    }
                }
            }
        }
#if !ASSERT_DISABLED
        else {
            ASSERT_WITH_SECURITY_IMPLICATION(value->isPrimitiveValue());
            ASSERT(static_cast<CSSPrimitiveValue*>(value)->getValueID() == CSSValueNormal);
        }
#endif

        FontDescription fontDescription = styleResolver->fontDescription();
        fontDescription.setCommonLigaturesState(commonLigaturesState);
        fontDescription.setDiscretionaryLigaturesState(discretionaryLigaturesState);
        fontDescription.setHistoricalLigaturesState(historicalLigaturesState);
        styleResolver->setFontDescription(fontDescription);
    }

    static PropertyHandler createHandler()
    {
        return PropertyHandler(&applyInheritValue, &applyInitialValue, &applyValue);
    }
};

enum BorderImageType { BorderImage = 0, BorderMask };
template <BorderImageType borderImageType,
          CSSPropertyID property,
          const NinePieceImage& (RenderStyle::*getterFunction)() const,
          void (RenderStyle::*setterFunction)(const NinePieceImage&)>
class ApplyPropertyBorderImage {
public:
    static void applyValue(CSSPropertyID, StyleResolver* styleResolver, CSSValue* value)
    {
        NinePieceImage image;
        if (borderImageType == BorderMask)
            image.setMaskDefaults();
        styleResolver->styleMap()->mapNinePieceImage(property, value, image);
        (styleResolver->style()->*setterFunction)(image);
    }

    static PropertyHandler createHandler()
    {
        PropertyHandler handler = ApplyPropertyDefaultBase<const NinePieceImage&, getterFunction, const NinePieceImage&, setterFunction, NinePieceImage, &RenderStyle::initialNinePieceImage>::createHandler();
        return PropertyHandler(handler.inheritFunction(), handler.initialFunction(), &applyValue);
    }
};

enum BorderImageModifierType { Outset, Repeat, Slice, Width };
template <BorderImageType type, BorderImageModifierType modifier>
class ApplyPropertyBorderImageModifier {
private:
    static inline const NinePieceImage& getValue(RenderStyle* style) { return type == BorderImage ? style->borderImage() : style->maskBoxImage(); }
    static inline void setValue(RenderStyle* style, const NinePieceImage& value) { return type == BorderImage ? style->setBorderImage(value) : style->setMaskBoxImage(value); }
public:
    static void applyInheritValue(CSSPropertyID, StyleResolver* styleResolver)
    {
        NinePieceImage image(getValue(styleResolver->style()));
        switch (modifier) {
        case Outset:
            image.copyOutsetFrom(getValue(styleResolver->parentStyle()));
            break;
        case Repeat:
            image.copyRepeatFrom(getValue(styleResolver->parentStyle()));
            break;
        case Slice:
            image.copyImageSlicesFrom(getValue(styleResolver->parentStyle()));
            break;
        case Width:
            image.copyBorderSlicesFrom(getValue(styleResolver->parentStyle()));
            break;
        }
        setValue(styleResolver->style(), image);
    }

    static void applyInitialValue(CSSPropertyID, StyleResolver* styleResolver)
    {
        NinePieceImage image(getValue(styleResolver->style()));
        switch (modifier) {
        case Outset:
            image.setOutset(LengthBox(0));
            break;
        case Repeat:
            image.setHorizontalRule(StretchImageRule);
            image.setVerticalRule(StretchImageRule);
            break;
        case Slice:
            // Masks have a different initial value for slices. Preserve the value of 0 for backwards compatibility.
            image.setImageSlices(type == BorderImage ? LengthBox(Length(100, Percent), Length(100, Percent), Length(100, Percent), Length(100, Percent)) : LengthBox());
            image.setFill(false);
            break;
        case Width:
            // Masks have a different initial value for widths. They use an 'auto' value rather than trying to fit to the border.
            image.setBorderSlices(type == BorderImage ? LengthBox(Length(1, Relative), Length(1, Relative), Length(1, Relative), Length(1, Relative)) : LengthBox());
            break;
        }
        setValue(styleResolver->style(), image);
    }

    static void applyValue(CSSPropertyID, StyleResolver* styleResolver, CSSValue* value)
    {
        NinePieceImage image(getValue(styleResolver->style()));
        switch (modifier) {
        case Outset:
            image.setOutset(styleResolver->styleMap()->mapNinePieceImageQuad(value));
            break;
        case Repeat:
            styleResolver->styleMap()->mapNinePieceImageRepeat(value, image);
            break;
        case Slice:
            styleResolver->styleMap()->mapNinePieceImageSlice(value, image);
            break;
        case Width:
            image.setBorderSlices(styleResolver->styleMap()->mapNinePieceImageQuad(value));
            break;
        }
        setValue(styleResolver->style(), image);
    }

    static PropertyHandler createHandler() { return PropertyHandler(&applyInheritValue, &applyInitialValue, &applyValue); }
};

template <CSSPropertyID id, StyleImage* (RenderStyle::*getterFunction)() const, void (RenderStyle::*setterFunction)(PassRefPtr<StyleImage>), StyleImage* (*initialFunction)()>
class ApplyPropertyBorderImageSource {
public:
    static void applyValue(CSSPropertyID, StyleResolver* styleResolver, CSSValue* value) { (styleResolver->style()->*setterFunction)(styleResolver->styleImage(id, value)); }
    static PropertyHandler createHandler()
    {
        PropertyHandler handler = ApplyPropertyDefaultBase<StyleImage*, getterFunction, PassRefPtr<StyleImage>, setterFunction, StyleImage*, initialFunction>::createHandler();
        return PropertyHandler(handler.inheritFunction(), handler.initialFunction(), &applyValue);
    }
};

enum CounterBehavior {Increment = 0, Reset};
template <CounterBehavior counterBehavior>
class ApplyPropertyCounter {
public:
    static void emptyFunction(CSSPropertyID, StyleResolver*) { }
    static void applyInheritValue(CSSPropertyID, StyleResolver* styleResolver)
    {
        CounterDirectiveMap& map = styleResolver->style()->accessCounterDirectives();
        CounterDirectiveMap& parentMap = styleResolver->parentStyle()->accessCounterDirectives();

        typedef CounterDirectiveMap::iterator Iterator;
        Iterator end = parentMap.end();
        for (Iterator it = parentMap.begin(); it != end; ++it) {
            CounterDirectives& directives = map.add(it->key, CounterDirectives()).iterator->value;
            if (counterBehavior == Reset) {
                directives.inheritReset(it->value);
            } else {
                directives.inheritIncrement(it->value);
            }
        }
    }
    static void applyValue(CSSPropertyID, StyleResolver* styleResolver, CSSValue* value)
    {
        bool setCounterIncrementToNone = counterBehavior == Increment && value->isPrimitiveValue() && static_cast<CSSPrimitiveValue*>(value)->getValueID() == CSSValueNone;

        if (!value->isValueList() && !setCounterIncrementToNone)
            return;

        CounterDirectiveMap& map = styleResolver->style()->accessCounterDirectives();
        typedef CounterDirectiveMap::iterator Iterator;

        Iterator end = map.end();
        for (Iterator it = map.begin(); it != end; ++it)
            if (counterBehavior == Reset)
                it->value.clearReset();
            else
                it->value.clearIncrement();
        
        if (setCounterIncrementToNone)
            return;
        
        CSSValueList* list = static_cast<CSSValueList*>(value);
        int length = list ? list->length() : 0;
        for (int i = 0; i < length; ++i) {
            CSSValue* currValue = list->itemWithoutBoundsCheck(i);
            if (!currValue->isPrimitiveValue())
                continue;

            Pair* pair = static_cast<CSSPrimitiveValue*>(currValue)->getPairValue();
            if (!pair || !pair->first() || !pair->second())
                continue;

            AtomicString identifier = static_cast<CSSPrimitiveValue*>(pair->first())->getStringValue();
            int value = static_cast<CSSPrimitiveValue*>(pair->second())->getIntValue();
            CounterDirectives& directives = map.add(identifier, CounterDirectives()).iterator->value;
            if (counterBehavior == Reset) {
                directives.setResetValue(value);
            } else {
                directives.addIncrementValue(value);
            }
        }
    }
    static PropertyHandler createHandler() { return PropertyHandler(&applyInheritValue, &emptyFunction, &applyValue); }
};


class ApplyPropertyCursor {
public:
    static void applyInheritValue(CSSPropertyID, StyleResolver* styleResolver)
    {
        styleResolver->style()->setCursor(styleResolver->parentStyle()->cursor());
        styleResolver->style()->setCursorList(styleResolver->parentStyle()->cursors());
    }

    static void applyInitialValue(CSSPropertyID, StyleResolver* styleResolver)
    {
        styleResolver->style()->clearCursorList();
        styleResolver->style()->setCursor(RenderStyle::initialCursor());
    }

    static void applyValue(CSSPropertyID, StyleResolver* styleResolver, CSSValue* value)
    {
        styleResolver->style()->clearCursorList();
        if (value->isValueList()) {
            CSSValueList* list = static_cast<CSSValueList*>(value);
            int len = list->length();
            styleResolver->style()->setCursor(CURSOR_AUTO);
            for (int i = 0; i < len; i++) {
                CSSValue* item = list->itemWithoutBoundsCheck(i);
                if (item->isCursorImageValue()) {
                    CSSCursorImageValue* image = static_cast<CSSCursorImageValue*>(item);
                    if (image->updateIfSVGCursorIsUsed(styleResolver->element())) // Elements with SVG cursors are not allowed to share style.
                        styleResolver->style()->setUnique();
                    styleResolver->style()->addCursor(styleResolver->styleImage(CSSPropertyCursor, image), image->hotSpot());
                } else if (item->isPrimitiveValue()) {
                    CSSPrimitiveValue* primitiveValue = static_cast<CSSPrimitiveValue*>(item);
                    if (primitiveValue->isValueID())
                        styleResolver->style()->setCursor(*primitiveValue);
                }
            }
        } else if (value->isPrimitiveValue()) {
            CSSPrimitiveValue* primitiveValue = static_cast<CSSPrimitiveValue*>(value);
            if (primitiveValue->isValueID() && styleResolver->style()->cursor() != ECursor(*primitiveValue))
                styleResolver->style()->setCursor(*primitiveValue);
        }
    }

    static PropertyHandler createHandler() { return PropertyHandler(&applyInheritValue, &applyInitialValue, &applyValue); }
};

class ApplyPropertyTextAlign {
public:
    static void applyValue(CSSPropertyID, StyleResolver* styleResolver, CSSValue* value)
    {
        if (!value->isPrimitiveValue())
            return;

        CSSPrimitiveValue* primitiveValue = static_cast<CSSPrimitiveValue*>(value);
        ASSERT(primitiveValue->isValueID());

        if (primitiveValue->getValueID() != CSSValueWebkitMatchParent)
            styleResolver->style()->setTextAlign(*primitiveValue);
        else if (styleResolver->parentStyle()->textAlign() == TASTART)
            styleResolver->style()->setTextAlign(styleResolver->parentStyle()->isLeftToRightDirection() ? LEFT : RIGHT);
        else if (styleResolver->parentStyle()->textAlign() == TAEND)
            styleResolver->style()->setTextAlign(styleResolver->parentStyle()->isLeftToRightDirection() ? RIGHT : LEFT);
        else
            styleResolver->style()->setTextAlign(styleResolver->parentStyle()->textAlign());
    }
    static PropertyHandler createHandler()
    {
        PropertyHandler handler = ApplyPropertyDefaultBase<ETextAlign, &RenderStyle::textAlign, ETextAlign, &RenderStyle::setTextAlign, ETextAlign, &RenderStyle::initialTextAlign>::createHandler();
        return PropertyHandler(handler.inheritFunction(), handler.initialFunction(), &applyValue);
    }
};

class ApplyPropertyTextDecoration {
public:
    static void applyValue(CSSPropertyID, StyleResolver* styleResolver, CSSValue* value)
    {
        TextDecoration t = RenderStyle::initialTextDecoration();
        for (CSSValueListIterator i(value); i.hasMore(); i.advance()) {
            CSSValue* item = i.value();
            ASSERT_WITH_SECURITY_IMPLICATION(item->isPrimitiveValue());
            t |= *static_cast<CSSPrimitiveValue*>(item);
        }
        styleResolver->style()->setTextDecoration(t);
    }
    static PropertyHandler createHandler()
    {
        PropertyHandler handler = ApplyPropertyDefaultBase<TextDecoration, &RenderStyle::textDecoration, TextDecoration, &RenderStyle::setTextDecoration, TextDecoration, &RenderStyle::initialTextDecoration>::createHandler();
        return PropertyHandler(handler.inheritFunction(), handler.initialFunction(), &applyValue);
    }
};

class ApplyPropertyMarqueeIncrement {
public:
    static void applyValue(CSSPropertyID, StyleResolver* styleResolver, CSSValue* value)
    {
        if (!value->isPrimitiveValue())
            return;

        CSSPrimitiveValue* primitiveValue = static_cast<CSSPrimitiveValue*>(value);
        if (primitiveValue->getValueID()) {
            switch (primitiveValue->getValueID()) {
            case CSSValueSmall:
                styleResolver->style()->setMarqueeIncrement(Length(1, Fixed)); // 1px.
                break;
            case CSSValueNormal:
                styleResolver->style()->setMarqueeIncrement(Length(6, Fixed)); // 6px. The WinIE default.
                break;
            case CSSValueLarge:
                styleResolver->style()->setMarqueeIncrement(Length(36, Fixed)); // 36px.
                break;
            default:
                break;
            }
        } else {
            Length marqueeLength = styleResolver->convertToIntLength(primitiveValue, styleResolver->style(), styleResolver->rootElementStyle());
            if (!marqueeLength.isUndefined())
                styleResolver->style()->setMarqueeIncrement(marqueeLength);
        }
    }
    static PropertyHandler createHandler()
    {
        PropertyHandler handler = ApplyPropertyLength<&RenderStyle::marqueeIncrement, &RenderStyle::setMarqueeIncrement, &RenderStyle::initialMarqueeIncrement>::createHandler();
        return PropertyHandler(handler.inheritFunction(), handler.initialFunction(), &applyValue);
    }
};

class ApplyPropertyMarqueeRepetition {
public:
    static void applyValue(CSSPropertyID, StyleResolver* styleResolver, CSSValue* value)
    {
        if (!value->isPrimitiveValue())
            return;

        CSSPrimitiveValue* primitiveValue = static_cast<CSSPrimitiveValue*>(value);
        if (primitiveValue->getValueID() == CSSValueInfinite)
            styleResolver->style()->setMarqueeLoopCount(-1); // -1 means repeat forever.
        else if (primitiveValue->isNumber())
            styleResolver->style()->setMarqueeLoopCount(primitiveValue->getIntValue());
    }
    static PropertyHandler createHandler()
    {
        PropertyHandler handler = ApplyPropertyDefault<int, &RenderStyle::marqueeLoopCount, int, &RenderStyle::setMarqueeLoopCount, int, &RenderStyle::initialMarqueeLoopCount>::createHandler();
        return PropertyHandler(handler.inheritFunction(), handler.initialFunction(), &applyValue);
    }
};

class ApplyPropertyMarqueeSpeed {
public:
    static void applyValue(CSSPropertyID, StyleResolver* styleResolver, CSSValue* value)
    {
        if (!value->isPrimitiveValue())
            return;

        CSSPrimitiveValue* primitiveValue = static_cast<CSSPrimitiveValue*>(value);
        if (CSSValueID ident = primitiveValue->getValueID()) {
            switch (ident) {
            case CSSValueSlow:
                styleResolver->style()->setMarqueeSpeed(500); // 500 msec.
                break;
            case CSSValueNormal:
                styleResolver->style()->setMarqueeSpeed(85); // 85msec. The WinIE default.
                break;
            case CSSValueFast:
                styleResolver->style()->setMarqueeSpeed(10); // 10msec. Super fast.
                break;
            default:
                break;
            }
        } else if (primitiveValue->isTime())
            styleResolver->style()->setMarqueeSpeed(primitiveValue->computeTime<int, CSSPrimitiveValue::Milliseconds>());
        else if (primitiveValue->isNumber()) // For scrollamount support.
            styleResolver->style()->setMarqueeSpeed(primitiveValue->getIntValue());
    }
    static PropertyHandler createHandler()
    {
        PropertyHandler handler = ApplyPropertyDefault<int, &RenderStyle::marqueeSpeed, int, &RenderStyle::setMarqueeSpeed, int, &RenderStyle::initialMarqueeSpeed>::createHandler();
        return PropertyHandler(handler.inheritFunction(), handler.initialFunction(), &applyValue);
    }
};

#if ENABLE(CSS3_TEXT)
class ApplyPropertyTextUnderlinePosition {
public:
    static void applyValue(CSSPropertyID, StyleResolver* styleResolver, CSSValue* value)
    {
        // This is true if value is 'auto' or 'alphabetic'.
        if (value->isPrimitiveValue()) {
            TextUnderlinePosition t = *static_cast<CSSPrimitiveValue*>(value);
            styleResolver->style()->setTextUnderlinePosition(t);
            return;
        }

        unsigned t = 0;
        for (CSSValueListIterator i(value); i.hasMore(); i.advance()) {
            CSSValue* item = i.value();
            ASSERT(item->isPrimitiveValue());
            TextUnderlinePosition t2 = *static_cast<CSSPrimitiveValue*>(item);
            t |= t2;
        }
        styleResolver->style()->setTextUnderlinePosition(static_cast<TextUnderlinePosition>(t));
    }
    static PropertyHandler createHandler()
    {
        PropertyHandler handler = ApplyPropertyDefaultBase<TextUnderlinePosition, &RenderStyle::textUnderlinePosition, TextUnderlinePosition, &RenderStyle::setTextUnderlinePosition, TextUnderlinePosition, &RenderStyle::initialTextUnderlinePosition>::createHandler();
        return PropertyHandler(handler.inheritFunction(), handler.initialFunction(), &applyValue);
    }
};
#endif // CSS3_TEXT

class ApplyPropertyLineHeight {
public:
    static void applyValue(CSSPropertyID, StyleResolver* styleResolver, CSSValue* value)
    {
        if (!value->isPrimitiveValue())
            return;

        CSSPrimitiveValue* primitiveValue = static_cast<CSSPrimitiveValue*>(value);
        Length lineHeight;

        if (primitiveValue->getValueID() == CSSValueNormal)
            lineHeight = RenderStyle::initialLineHeight();
        else if (primitiveValue->isLength()) {
            double multiplier = styleResolver->style()->effectiveZoom();
            if (Frame* frame = styleResolver->document()->frame())
                multiplier *= frame->textZoomFactor();
            lineHeight = primitiveValue->computeLength<Length>(styleResolver->style(), styleResolver->rootElementStyle(), multiplier);
        } else if (primitiveValue->isPercentage()) {
            // FIXME: percentage should not be restricted to an integer here.
            lineHeight = Length((styleResolver->style()->fontSize() * primitiveValue->getIntValue()) / 100, Fixed);
        } else if (primitiveValue->isNumber()) {
            // FIXME: number and percentage values should produce the same type of Length (ie. Fixed or Percent).
            lineHeight = Length(primitiveValue->getDoubleValue() * 100.0, Percent);
        } else if (primitiveValue->isViewportPercentageLength())
            lineHeight = primitiveValue->viewportPercentageLength();
        else
            return;
        styleResolver->style()->setLineHeight(lineHeight);
    }
    static PropertyHandler createHandler()
    {
        PropertyHandler handler = ApplyPropertyDefaultBase<Length, &RenderStyle::specifiedLineHeight, Length, &RenderStyle::setLineHeight, Length, &RenderStyle::initialLineHeight>::createHandler();
        return PropertyHandler(handler.inheritFunction(), handler.initialFunction(), &applyValue);
    }
};

class ApplyPropertyPageSize {
private:
    static Length mmLength(double mm) { return CSSPrimitiveValue::create(mm, CSSPrimitiveValue::CSS_MM)->computeLength<Length>(0, 0); }
    static Length inchLength(double inch) { return CSSPrimitiveValue::create(inch, CSSPrimitiveValue::CSS_IN)->computeLength<Length>(0, 0); }
    static bool getPageSizeFromName(CSSPrimitiveValue* pageSizeName, CSSPrimitiveValue* pageOrientation, Length& width, Length& height)
    {
        DEFINE_STATIC_LOCAL(Length, a5Width, (mmLength(148)));
        DEFINE_STATIC_LOCAL(Length, a5Height, (mmLength(210)));
        DEFINE_STATIC_LOCAL(Length, a4Width, (mmLength(210)));
        DEFINE_STATIC_LOCAL(Length, a4Height, (mmLength(297)));
        DEFINE_STATIC_LOCAL(Length, a3Width, (mmLength(297)));
        DEFINE_STATIC_LOCAL(Length, a3Height, (mmLength(420)));
        DEFINE_STATIC_LOCAL(Length, b5Width, (mmLength(176)));
        DEFINE_STATIC_LOCAL(Length, b5Height, (mmLength(250)));
        DEFINE_STATIC_LOCAL(Length, b4Width, (mmLength(250)));
        DEFINE_STATIC_LOCAL(Length, b4Height, (mmLength(353)));
        DEFINE_STATIC_LOCAL(Length, letterWidth, (inchLength(8.5)));
        DEFINE_STATIC_LOCAL(Length, letterHeight, (inchLength(11)));
        DEFINE_STATIC_LOCAL(Length, legalWidth, (inchLength(8.5)));
        DEFINE_STATIC_LOCAL(Length, legalHeight, (inchLength(14)));
        DEFINE_STATIC_LOCAL(Length, ledgerWidth, (inchLength(11)));
        DEFINE_STATIC_LOCAL(Length, ledgerHeight, (inchLength(17)));

        if (!pageSizeName)
            return false;

        switch (pageSizeName->getValueID()) {
        case CSSValueA5:
            width = a5Width;
            height = a5Height;
            break;
        case CSSValueA4:
            width = a4Width;
            height = a4Height;
            break;
        case CSSValueA3:
            width = a3Width;
            height = a3Height;
            break;
        case CSSValueB5:
            width = b5Width;
            height = b5Height;
            break;
        case CSSValueB4:
            width = b4Width;
            height = b4Height;
            break;
        case CSSValueLetter:
            width = letterWidth;
            height = letterHeight;
            break;
        case CSSValueLegal:
            width = legalWidth;
            height = legalHeight;
            break;
        case CSSValueLedger:
            width = ledgerWidth;
            height = ledgerHeight;
            break;
        default:
            return false;
        }

        if (pageOrientation) {
            switch (pageOrientation->getValueID()) {
            case CSSValueLandscape:
                std::swap(width, height);
                break;
            case CSSValuePortrait:
                // Nothing to do.
                break;
            default:
                return false;
            }
        }
        return true;
    }
public:
    static void applyInheritValue(CSSPropertyID, StyleResolver*) { }
    static void applyInitialValue(CSSPropertyID, StyleResolver*) { }
    static void applyValue(CSSPropertyID, StyleResolver* styleResolver, CSSValue* value)
    {
        styleResolver->style()->resetPageSizeType();
        Length width;
        Length height;
        PageSizeType pageSizeType = PAGE_SIZE_AUTO;
        CSSValueListInspector inspector(value);
        switch (inspector.length()) {
        case 2: {
            // <length>{2} | <page-size> <orientation>
            if (!inspector.first()->isPrimitiveValue() || !inspector.second()->isPrimitiveValue())
                return;
            CSSPrimitiveValue* first = static_cast<CSSPrimitiveValue*>(inspector.first());
            CSSPrimitiveValue* second = static_cast<CSSPrimitiveValue*>(inspector.second());
            if (first->isLength()) {
                // <length>{2}
                if (!second->isLength())
                    return;
                width = first->computeLength<Length>(styleResolver->style(), styleResolver->rootElementStyle());
                height = second->computeLength<Length>(styleResolver->style(), styleResolver->rootElementStyle());
            } else {
                // <page-size> <orientation>
                // The value order is guaranteed. See CSSParser::parseSizeParameter.
                if (!getPageSizeFromName(first, second, width, height))
                    return;
            }
            pageSizeType = PAGE_SIZE_RESOLVED;
            break;
        }
        case 1: {
            // <length> | auto | <page-size> | [ portrait | landscape]
            if (!inspector.first()->isPrimitiveValue())
                return;
            CSSPrimitiveValue* primitiveValue = static_cast<CSSPrimitiveValue*>(inspector.first());
            if (primitiveValue->isLength()) {
                // <length>
                pageSizeType = PAGE_SIZE_RESOLVED;
                width = height = primitiveValue->computeLength<Length>(styleResolver->style(), styleResolver->rootElementStyle());
            } else {
                switch (primitiveValue->getValueID()) {
                case 0:
                    return;
                case CSSValueAuto:
                    pageSizeType = PAGE_SIZE_AUTO;
                    break;
                case CSSValuePortrait:
                    pageSizeType = PAGE_SIZE_AUTO_PORTRAIT;
                    break;
                case CSSValueLandscape:
                    pageSizeType = PAGE_SIZE_AUTO_LANDSCAPE;
                    break;
                default:
                    // <page-size>
                    pageSizeType = PAGE_SIZE_RESOLVED;
                    if (!getPageSizeFromName(primitiveValue, 0, width, height))
                        return;
                }
            }
            break;
        }
        default:
            return;
        }
        styleResolver->style()->setPageSizeType(pageSizeType);
        styleResolver->style()->setPageSize(LengthSize(width, height));
    }
    static PropertyHandler createHandler() { return PropertyHandler(&applyInheritValue, &applyInitialValue, &applyValue); }
};

class ApplyPropertyTextEmphasisStyle {
public:
    static void applyInheritValue(CSSPropertyID, StyleResolver* styleResolver)
    {
        styleResolver->style()->setTextEmphasisFill(styleResolver->parentStyle()->textEmphasisFill());
        styleResolver->style()->setTextEmphasisMark(styleResolver->parentStyle()->textEmphasisMark());
        styleResolver->style()->setTextEmphasisCustomMark(styleResolver->parentStyle()->textEmphasisCustomMark());
    }

    static void applyInitialValue(CSSPropertyID, StyleResolver* styleResolver)
    {
        styleResolver->style()->setTextEmphasisFill(RenderStyle::initialTextEmphasisFill());
        styleResolver->style()->setTextEmphasisMark(RenderStyle::initialTextEmphasisMark());
        styleResolver->style()->setTextEmphasisCustomMark(RenderStyle::initialTextEmphasisCustomMark());
    }

    static void applyValue(CSSPropertyID, StyleResolver* styleResolver, CSSValue* value)
    {
        if (value->isValueList()) {
            CSSValueList* list = static_cast<CSSValueList*>(value);
            ASSERT(list->length() == 2);
            if (list->length() != 2)
                return;
            for (unsigned i = 0; i < 2; ++i) {
                CSSValue* item = list->itemWithoutBoundsCheck(i);
                if (!item->isPrimitiveValue())
                    continue;

                CSSPrimitiveValue* value = static_cast<CSSPrimitiveValue*>(item);
                if (value->getValueID() == CSSValueFilled || value->getValueID() == CSSValueOpen)
                    styleResolver->style()->setTextEmphasisFill(*value);
                else
                    styleResolver->style()->setTextEmphasisMark(*value);
            }
            styleResolver->style()->setTextEmphasisCustomMark(nullAtom);
            return;
        }

        if (!value->isPrimitiveValue())
            return;
        CSSPrimitiveValue* primitiveValue = static_cast<CSSPrimitiveValue*>(value);

        if (primitiveValue->isString()) {
            styleResolver->style()->setTextEmphasisFill(TextEmphasisFillFilled);
            styleResolver->style()->setTextEmphasisMark(TextEmphasisMarkCustom);
            styleResolver->style()->setTextEmphasisCustomMark(primitiveValue->getStringValue());
            return;
        }

        styleResolver->style()->setTextEmphasisCustomMark(nullAtom);

        if (primitiveValue->getValueID() == CSSValueFilled || primitiveValue->getValueID() == CSSValueOpen) {
            styleResolver->style()->setTextEmphasisFill(*primitiveValue);
            styleResolver->style()->setTextEmphasisMark(TextEmphasisMarkAuto);
        } else {
            styleResolver->style()->setTextEmphasisFill(TextEmphasisFillFilled);
            styleResolver->style()->setTextEmphasisMark(*primitiveValue);
        }
    }

    static PropertyHandler createHandler() { return PropertyHandler(&applyInheritValue, &applyInitialValue, &applyValue); }
};

template <typename T,
          T (Animation::*getterFunction)() const,
          void (Animation::*setterFunction)(T),
          bool (Animation::*testFunction)() const,
          void (Animation::*clearFunction)(),
          T (*initialFunction)(),
          void (CSSToStyleMap::*mapFunction)(Animation*, CSSValue*),
          AnimationList* (RenderStyle::*animationGetterFunction)(),
          const AnimationList* (RenderStyle::*immutableAnimationGetterFunction)() const>
class ApplyPropertyAnimation {
public:
    static void setValue(Animation* animation, T value) { (animation->*setterFunction)(value); }
    static T value(const Animation* animation) { return (animation->*getterFunction)(); }
    static bool test(const Animation* animation) { return (animation->*testFunction)(); }
    static void clear(Animation* animation) { (animation->*clearFunction)(); }
    static T initial() { return (*initialFunction)(); }
    static void map(StyleResolver* styleResolver, Animation* animation, CSSValue* value) { (styleResolver->styleMap()->*mapFunction)(animation, value); }
    static AnimationList* accessAnimations(RenderStyle* style) { return (style->*animationGetterFunction)(); }
    static const AnimationList* animations(RenderStyle* style) { return (style->*immutableAnimationGetterFunction)(); }

    static void applyInheritValue(CSSPropertyID, StyleResolver* styleResolver)
    {
        AnimationList* list = accessAnimations(styleResolver->style());
        const AnimationList* parentList = animations(styleResolver->parentStyle());
        size_t i = 0, parentSize = parentList ? parentList->size() : 0;
        for ( ; i < parentSize && test(parentList->animation(i)); ++i) {
            if (list->size() <= i)
                list->append(Animation::create());
            setValue(list->animation(i), value(parentList->animation(i)));
            list->animation(i)->setAnimationMode(parentList->animation(i)->animationMode());
        }

        /* Reset any remaining animations to not have the property set. */
        for ( ; i < list->size(); ++i)
            clear(list->animation(i));
    }

    static void applyInitialValue(CSSPropertyID propertyID, StyleResolver* styleResolver)
    {
        AnimationList* list = accessAnimations(styleResolver->style());
        if (list->isEmpty())
            list->append(Animation::create());
        setValue(list->animation(0), initial());
        if (propertyID == CSSPropertyWebkitTransitionProperty)
            list->animation(0)->setAnimationMode(Animation::AnimateAll);
        for (size_t i = 1; i < list->size(); ++i)
            clear(list->animation(i));
    }

    static void applyValue(CSSPropertyID, StyleResolver* styleResolver, CSSValue* value)
    {
        AnimationList* list = accessAnimations(styleResolver->style());
        size_t childIndex = 0;
        if (value->isValueList()) {
            /* Walk each value and put it into an animation, creating new animations as needed. */
            for (CSSValueListIterator i = value; i.hasMore(); i.advance()) {
                if (childIndex <= list->size())
                    list->append(Animation::create());
                map(styleResolver, list->animation(childIndex), i.value());
                ++childIndex;
            }
        } else {
            if (list->isEmpty())
                list->append(Animation::create());
            map(styleResolver, list->animation(childIndex), value);
            childIndex = 1;
        }
        for ( ; childIndex < list->size(); ++childIndex) {
            /* Reset all remaining animations to not have the property set. */
            clear(list->animation(childIndex));
        }
    }

    static PropertyHandler createHandler() { return PropertyHandler(&applyInheritValue, &applyInitialValue, &applyValue); }
};

class ApplyPropertyOutlineStyle {
public:
    static void applyInheritValue(CSSPropertyID propertyID, StyleResolver* styleResolver)
    {
        ApplyPropertyDefaultBase<OutlineIsAuto, &RenderStyle::outlineStyleIsAuto, OutlineIsAuto, &RenderStyle::setOutlineStyleIsAuto, OutlineIsAuto, &RenderStyle::initialOutlineStyleIsAuto>::applyInheritValue(propertyID, styleResolver);
        ApplyPropertyDefaultBase<EBorderStyle, &RenderStyle::outlineStyle, EBorderStyle, &RenderStyle::setOutlineStyle, EBorderStyle, &RenderStyle::initialBorderStyle>::applyInheritValue(propertyID, styleResolver);
    }

    static void applyInitialValue(CSSPropertyID propertyID, StyleResolver* styleResolver)
    {
        ApplyPropertyDefaultBase<OutlineIsAuto, &RenderStyle::outlineStyleIsAuto, OutlineIsAuto, &RenderStyle::setOutlineStyleIsAuto, OutlineIsAuto, &RenderStyle::initialOutlineStyleIsAuto>::applyInitialValue(propertyID, styleResolver);
        ApplyPropertyDefaultBase<EBorderStyle, &RenderStyle::outlineStyle, EBorderStyle, &RenderStyle::setOutlineStyle, EBorderStyle, &RenderStyle::initialBorderStyle>::applyInitialValue(propertyID, styleResolver);
    }

    static void applyValue(CSSPropertyID propertyID, StyleResolver* styleResolver, CSSValue* value)
    {
        ApplyPropertyDefault<OutlineIsAuto, &RenderStyle::outlineStyleIsAuto, OutlineIsAuto, &RenderStyle::setOutlineStyleIsAuto, OutlineIsAuto, &RenderStyle::initialOutlineStyleIsAuto>::applyValue(propertyID, styleResolver, value);
        ApplyPropertyDefault<EBorderStyle, &RenderStyle::outlineStyle, EBorderStyle, &RenderStyle::setOutlineStyle, EBorderStyle, &RenderStyle::initialBorderStyle>::applyValue(propertyID, styleResolver, value);
    }

    static PropertyHandler createHandler() { return PropertyHandler(&applyInheritValue, &applyInitialValue, &applyValue); }
};

class ApplyPropertyResize {
public:
    static void applyValue(CSSPropertyID, StyleResolver* styleResolver, CSSValue* value)
    {
        if (!value->isPrimitiveValue())
            return;

        CSSPrimitiveValue* primitiveValue = static_cast<CSSPrimitiveValue*>(value);

        EResize r = RESIZE_NONE;
        switch (primitiveValue->getValueID()) {
        case 0:
            return;
        case CSSValueAuto:
            if (Settings* settings = styleResolver->document()->settings())
                r = settings->textAreasAreResizable() ? RESIZE_BOTH : RESIZE_NONE;
            break;
        default:
            r = *primitiveValue;
        }
        styleResolver->style()->setResize(r);
    }

    static PropertyHandler createHandler()
    {
        PropertyHandler handler = ApplyPropertyDefaultBase<EResize, &RenderStyle::resize, EResize, &RenderStyle::setResize, EResize, &RenderStyle::initialResize>::createHandler();
        return PropertyHandler(handler.inheritFunction(), handler.initialFunction(), &applyValue);
    }
};

class ApplyPropertyVerticalAlign {
public:
    static void applyValue(CSSPropertyID, StyleResolver* styleResolver, CSSValue* value)
    {
        if (!value->isPrimitiveValue())
            return;

        CSSPrimitiveValue* primitiveValue = static_cast<CSSPrimitiveValue*>(value);

        if (primitiveValue->getValueID())
            return styleResolver->style()->setVerticalAlign(*primitiveValue);

        styleResolver->style()->setVerticalAlignLength(primitiveValue->convertToLength<FixedIntegerConversion | PercentConversion | CalculatedConversion | ViewportPercentageConversion>(styleResolver->style(), styleResolver->rootElementStyle(), styleResolver->style()->effectiveZoom()));
    }

    static PropertyHandler createHandler()
    {
        PropertyHandler handler = ApplyPropertyDefaultBase<EVerticalAlign, &RenderStyle::verticalAlign, EVerticalAlign, &RenderStyle::setVerticalAlign, EVerticalAlign, &RenderStyle::initialVerticalAlign>::createHandler();
        return PropertyHandler(handler.inheritFunction(), handler.initialFunction(), &applyValue);
    }
};

class ApplyPropertyAspectRatio {
public:
    static void applyInheritValue(CSSPropertyID, StyleResolver* styleResolver)
    {
        if (!styleResolver->parentStyle()->hasAspectRatio())
            return;
        styleResolver->style()->setHasAspectRatio(true);
        styleResolver->style()->setAspectRatioDenominator(styleResolver->parentStyle()->aspectRatioDenominator());
        styleResolver->style()->setAspectRatioNumerator(styleResolver->parentStyle()->aspectRatioNumerator());
    }

    static void applyInitialValue(CSSPropertyID, StyleResolver* styleResolver)
    {
        styleResolver->style()->setHasAspectRatio(RenderStyle::initialHasAspectRatio());
        styleResolver->style()->setAspectRatioDenominator(RenderStyle::initialAspectRatioDenominator());
        styleResolver->style()->setAspectRatioNumerator(RenderStyle::initialAspectRatioNumerator());
    }

    static void applyValue(CSSPropertyID, StyleResolver* styleResolver, CSSValue* value)
    {
        if (!value->isAspectRatioValue()) {
            styleResolver->style()->setHasAspectRatio(false);
            return;
        }
        CSSAspectRatioValue* aspectRatioValue = static_cast<CSSAspectRatioValue*>(value);
        styleResolver->style()->setHasAspectRatio(true);
        styleResolver->style()->setAspectRatioDenominator(aspectRatioValue->denominatorValue());
        styleResolver->style()->setAspectRatioNumerator(aspectRatioValue->numeratorValue());
    }

    static PropertyHandler createHandler()
    {
        return PropertyHandler(&applyInheritValue, &applyInitialValue, &applyValue);
    }
};

class ApplyPropertyZoom {
private:
    static void resetEffectiveZoom(StyleResolver* styleResolver)
    {
        // Reset the zoom in effect. This allows the setZoom method to accurately compute a new zoom in effect.
        styleResolver->setEffectiveZoom(styleResolver->parentStyle() ? styleResolver->parentStyle()->effectiveZoom() : RenderStyle::initialZoom());
    }

public:
    static void applyInheritValue(CSSPropertyID, StyleResolver* styleResolver)
    {
        resetEffectiveZoom(styleResolver);
        styleResolver->setZoom(styleResolver->parentStyle()->zoom());
    }

    static void applyInitialValue(CSSPropertyID, StyleResolver* styleResolver)
    {
        resetEffectiveZoom(styleResolver);
        styleResolver->setZoom(RenderStyle::initialZoom());
    }

    static void applyValue(CSSPropertyID, StyleResolver* styleResolver, CSSValue* value)
    {
        ASSERT_WITH_SECURITY_IMPLICATION(value->isPrimitiveValue());
        CSSPrimitiveValue* primitiveValue = static_cast<CSSPrimitiveValue*>(value);

        if (primitiveValue->getValueID() == CSSValueNormal) {
            resetEffectiveZoom(styleResolver);
            styleResolver->setZoom(RenderStyle::initialZoom());
        } else if (primitiveValue->getValueID() == CSSValueReset) {
            styleResolver->setEffectiveZoom(RenderStyle::initialZoom());
            styleResolver->setZoom(RenderStyle::initialZoom());
        } else if (primitiveValue->getValueID() == CSSValueDocument) {
            float docZoom = styleResolver->rootElementStyle() ? styleResolver->rootElementStyle()->zoom() : RenderStyle::initialZoom();
            styleResolver->setEffectiveZoom(docZoom);
            styleResolver->setZoom(docZoom);
        } else if (primitiveValue->isPercentage()) {
            resetEffectiveZoom(styleResolver);
            if (float percent = primitiveValue->getFloatValue())
                styleResolver->setZoom(percent / 100.0f);
        } else if (primitiveValue->isNumber()) {
            resetEffectiveZoom(styleResolver);
            if (float number = primitiveValue->getFloatValue())
                styleResolver->setZoom(number);
        }
    }

    static PropertyHandler createHandler()
    {
        return PropertyHandler(&applyInheritValue, &applyInitialValue, &applyValue);
    }
};

class ApplyPropertyDisplay {
private:
    static inline bool isValidDisplayValue(StyleResolver* styleResolver, EDisplay displayPropertyValue)
    {
#if ENABLE(SVG)
        if (styleResolver->element() && styleResolver->element()->isSVGElement() && styleResolver->style()->styleType() == NOPSEUDO)
            return (displayPropertyValue == INLINE || displayPropertyValue == BLOCK || displayPropertyValue == NONE);
#else
        UNUSED_PARAM(styleResolver);
        UNUSED_PARAM(displayPropertyValue);
#endif
        return true;
    }
public:
    static void applyInheritValue(CSSPropertyID, StyleResolver* styleResolver)
    {
        EDisplay display = styleResolver->parentStyle()->display();
        if (!isValidDisplayValue(styleResolver, display))
            return;
        styleResolver->style()->setDisplay(display);
    }

    static void applyInitialValue(CSSPropertyID, StyleResolver* styleResolver)
    {
        styleResolver->style()->setDisplay(RenderStyle::initialDisplay());
    }

    static void applyValue(CSSPropertyID, StyleResolver* styleResolver, CSSValue* value)
    {
        if (!value->isPrimitiveValue())
            return;

        EDisplay display = *static_cast<CSSPrimitiveValue*>(value);

        if (!isValidDisplayValue(styleResolver, display))
            return;

        styleResolver->style()->setDisplay(display);
    }

    static PropertyHandler createHandler()
    {
        return PropertyHandler(&applyInheritValue, &applyInitialValue, &applyValue);
    }
};

template <ClipPathOperation* (RenderStyle::*getterFunction)() const, void (RenderStyle::*setterFunction)(PassRefPtr<ClipPathOperation>), ClipPathOperation* (*initialFunction)()>
class ApplyPropertyClipPath {
public:
    static void setValue(RenderStyle* style, PassRefPtr<ClipPathOperation> value) { (style->*setterFunction)(value); }
    static void applyValue(CSSPropertyID, StyleResolver* styleResolver, CSSValue* value)
    {
        if (value->isPrimitiveValue()) {
            CSSPrimitiveValue* primitiveValue = static_cast<CSSPrimitiveValue*>(value);
            if (primitiveValue->getValueID() == CSSValueNone)
                setValue(styleResolver->style(), 0);
            else if (primitiveValue->isShape()) {
                setValue(styleResolver->style(), ShapeClipPathOperation::create(basicShapeForValue(styleResolver->style(), styleResolver->rootElementStyle(), primitiveValue->getShapeValue())));
            }
#if ENABLE(SVG)
            else if (primitiveValue->primitiveType() == CSSPrimitiveValue::CSS_URI) {
                String cssURLValue = primitiveValue->getStringValue();
                KURL url = styleResolver->document()->completeURL(cssURLValue);
                // FIXME: It doesn't work with forward or external SVG references (see https://bugs.webkit.org/show_bug.cgi?id=90405)
                setValue(styleResolver->style(), ReferenceClipPathOperation::create(cssURLValue, url.fragmentIdentifier()));
            }
#endif
        }
    }
    static PropertyHandler createHandler()
    {
        PropertyHandler handler = ApplyPropertyDefaultBase<ClipPathOperation*, getterFunction, PassRefPtr<ClipPathOperation>, setterFunction, ClipPathOperation*, initialFunction>::createHandler();
        return PropertyHandler(handler.inheritFunction(), handler.initialFunction(), &applyValue);
    }
};

#if ENABLE(CSS_SHAPES)
template <ShapeValue* (RenderStyle::*getterFunction)() const, void (RenderStyle::*setterFunction)(PassRefPtr<ShapeValue>), ShapeValue* (*initialFunction)()>
class ApplyPropertyShape {
public:
    static void setValue(RenderStyle* style, PassRefPtr<ShapeValue> value) { (style->*setterFunction)(value); }
    static void applyValue(CSSPropertyID property, StyleResolver* styleResolver, CSSValue* value)
    {
        if (value->isPrimitiveValue()) {
            CSSPrimitiveValue* primitiveValue = static_cast<CSSPrimitiveValue*>(value);
            if (primitiveValue->getValueID() == CSSValueAuto)
                setValue(styleResolver->style(), 0);
            // FIXME Bug 102571: Layout for the value 'outside-shape' is not yet implemented
            else if (primitiveValue->getValueID() == CSSValueOutsideShape)
                setValue(styleResolver->style(), ShapeValue::createOutsideValue());
            else if (primitiveValue->isShape()) {
                RefPtr<ShapeValue> shape = ShapeValue::createShapeValue(basicShapeForValue(styleResolver->style(), styleResolver->rootElementStyle(), primitiveValue->getShapeValue()));
                setValue(styleResolver->style(), shape.release());
            }
        } else if (value->isImageValue()) {
            RefPtr<ShapeValue> shape = ShapeValue::createImageValue(styleResolver->styleImage(property, value));
            setValue(styleResolver->style(), shape.release());
        }

    }
    static PropertyHandler createHandler()
    {
        PropertyHandler handler = ApplyPropertyDefaultBase<ShapeValue*, getterFunction, PassRefPtr<ShapeValue>, setterFunction, ShapeValue*, initialFunction>::createHandler();
        return PropertyHandler(handler.inheritFunction(), handler.initialFunction(), &applyValue);
    }
};
#endif

#if ENABLE(CSS_IMAGE_RESOLUTION)
class ApplyPropertyImageResolution {
public:
    static void applyInheritValue(CSSPropertyID propertyID, StyleResolver* styleResolver)
    {
        ApplyPropertyDefaultBase<ImageResolutionSource, &RenderStyle::imageResolutionSource, ImageResolutionSource, &RenderStyle::setImageResolutionSource, ImageResolutionSource, &RenderStyle::initialImageResolutionSource>::applyInheritValue(propertyID, styleResolver);
        ApplyPropertyDefaultBase<ImageResolutionSnap, &RenderStyle::imageResolutionSnap, ImageResolutionSnap, &RenderStyle::setImageResolutionSnap, ImageResolutionSnap, &RenderStyle::initialImageResolutionSnap>::applyInheritValue(propertyID, styleResolver);
        ApplyPropertyDefaultBase<float, &RenderStyle::imageResolution, float, &RenderStyle::setImageResolution, float, &RenderStyle::initialImageResolution>::applyInheritValue(propertyID, styleResolver);
    }

    static void applyInitialValue(CSSPropertyID propertyID, StyleResolver* styleResolver)
    {
        ApplyPropertyDefaultBase<ImageResolutionSource, &RenderStyle::imageResolutionSource, ImageResolutionSource, &RenderStyle::setImageResolutionSource, ImageResolutionSource, &RenderStyle::initialImageResolutionSource>::applyInitialValue(propertyID, styleResolver);
        ApplyPropertyDefaultBase<ImageResolutionSnap, &RenderStyle::imageResolutionSnap, ImageResolutionSnap, &RenderStyle::setImageResolutionSnap, ImageResolutionSnap, &RenderStyle::initialImageResolutionSnap>::applyInitialValue(propertyID, styleResolver);
        ApplyPropertyDefaultBase<float, &RenderStyle::imageResolution, float, &RenderStyle::setImageResolution, float, &RenderStyle::initialImageResolution>::applyInitialValue(propertyID, styleResolver);
    }

    static void applyValue(CSSPropertyID, StyleResolver* styleResolver, CSSValue* value)
    {
        if (!value->isValueList())
            return;
        CSSValueList* valueList = static_cast<CSSValueList*>(value);
        ImageResolutionSource source = RenderStyle::initialImageResolutionSource();
        ImageResolutionSnap snap = RenderStyle::initialImageResolutionSnap();
        double resolution = RenderStyle::initialImageResolution();
        for (size_t i = 0; i < valueList->length(); i++) {
            CSSValue* item = valueList->itemWithoutBoundsCheck(i);
            if (!item->isPrimitiveValue())
                continue;
            CSSPrimitiveValue* primitiveValue = static_cast<CSSPrimitiveValue*>(item);
            if (primitiveValue->getValueID() == CSSValueFromImage)
                source = ImageResolutionFromImage;
            else if (primitiveValue->getValueID() == CSSValueSnap)
                snap = ImageResolutionSnapPixels;
            else
                resolution = primitiveValue->getDoubleValue(CSSPrimitiveValue::CSS_DPPX);
        }
        styleResolver->style()->setImageResolutionSource(source);
        styleResolver->style()->setImageResolutionSnap(snap);
        styleResolver->style()->setImageResolution(resolution);
    }

    static PropertyHandler createHandler()
    {
        return PropertyHandler(&applyInheritValue, &applyInitialValue, &applyValue);
    }
};
#endif

class ApplyPropertyTextIndent {
public:
    static void applyInheritValue(CSSPropertyID, StyleResolver* styleResolver)
    {
        styleResolver->style()->setTextIndent(styleResolver->parentStyle()->textIndent());
#if ENABLE(CSS3_TEXT)
        styleResolver->style()->setTextIndentLine(styleResolver->parentStyle()->textIndentLine());
        styleResolver->style()->setTextIndentType(styleResolver->parentStyle()->textIndentType());
#endif
    }

    static void applyInitialValue(CSSPropertyID, StyleResolver* styleResolver)
    {
        styleResolver->style()->setTextIndent(RenderStyle::initialTextIndent());
#if ENABLE(CSS3_TEXT)
        styleResolver->style()->setTextIndentLine(RenderStyle::initialTextIndentLine());
        styleResolver->style()->setTextIndentType(RenderStyle::initialTextIndentType());
#endif
    }

    static void applyValue(CSSPropertyID, StyleResolver* styleResolver, CSSValue* value)
    {
        if (!value->isValueList())
            return;

        Length lengthOrPercentageValue;
#if ENABLE(CSS3_TEXT)
        TextIndentLine textIndentLineValue = RenderStyle::initialTextIndentLine();
        TextIndentType textIndentTypeValue = RenderStyle::initialTextIndentType();
#endif
        CSSValueList* valueList = static_cast<CSSValueList*>(value);
        for (size_t i = 0; i < valueList->length(); ++i) {
            CSSValue* item = valueList->itemWithoutBoundsCheck(i);
            if (!item->isPrimitiveValue())
                continue;

            CSSPrimitiveValue* primitiveValue = static_cast<CSSPrimitiveValue*>(item);
            if (!primitiveValue->getValueID())
                lengthOrPercentageValue = primitiveValue->convertToLength<FixedIntegerConversion | PercentConversion | CalculatedConversion | ViewportPercentageConversion>(styleResolver->style(), styleResolver->rootElementStyle(), styleResolver->style()->effectiveZoom());
#if ENABLE(CSS3_TEXT)
            else if (primitiveValue->getValueID() == CSSValueWebkitEachLine)
                textIndentLineValue = TextIndentEachLine;
            else if (primitiveValue->getValueID() == CSSValueWebkitHanging)
                textIndentTypeValue = TextIndentHanging;
#endif
        }

        ASSERT(!lengthOrPercentageValue.isUndefined());
        styleResolver->style()->setTextIndent(lengthOrPercentageValue);
#if ENABLE(CSS3_TEXT)
        styleResolver->style()->setTextIndentLine(textIndentLineValue);
        styleResolver->style()->setTextIndentType(textIndentTypeValue);
#endif
    }

    static PropertyHandler createHandler()
    {
        return PropertyHandler(&applyInheritValue, &applyInitialValue, &applyValue);
    }
};

const DeprecatedStyleBuilder& DeprecatedStyleBuilder::sharedStyleBuilder()
{
    DEFINE_STATIC_LOCAL(DeprecatedStyleBuilder, styleBuilderInstance, ());
    return styleBuilderInstance;
}

DeprecatedStyleBuilder::DeprecatedStyleBuilder()
{
    for (int i = 0; i < numCSSProperties; ++i)
        m_propertyMap[i] = PropertyHandler();

    // Please keep CSS property list in alphabetical order.
    setPropertyHandler(CSSPropertyBackgroundAttachment, ApplyPropertyFillLayer<EFillAttachment, CSSPropertyBackgroundAttachment, BackgroundFillLayer, &RenderStyle::accessBackgroundLayers, &RenderStyle::backgroundLayers, &FillLayer::isAttachmentSet, &FillLayer::attachment, &FillLayer::setAttachment, &FillLayer::clearAttachment, &FillLayer::initialFillAttachment, &CSSToStyleMap::mapFillAttachment>::createHandler());
    setPropertyHandler(CSSPropertyBackgroundClip, ApplyPropertyFillLayer<EFillBox, CSSPropertyBackgroundClip, BackgroundFillLayer, &RenderStyle::accessBackgroundLayers, &RenderStyle::backgroundLayers, &FillLayer::isClipSet, &FillLayer::clip, &FillLayer::setClip, &FillLayer::clearClip, &FillLayer::initialFillClip, &CSSToStyleMap::mapFillClip>::createHandler());
    setPropertyHandler(CSSPropertyBackgroundColor, ApplyPropertyColor<NoInheritFromParent, &RenderStyle::backgroundColor, &RenderStyle::setBackgroundColor, &RenderStyle::setVisitedLinkBackgroundColor, &RenderStyle::invalidColor>::createHandler());
    setPropertyHandler(CSSPropertyBackgroundImage, ApplyPropertyFillLayer<StyleImage*, CSSPropertyBackgroundImage, BackgroundFillLayer, &RenderStyle::accessBackgroundLayers, &RenderStyle::backgroundLayers, &FillLayer::isImageSet, &FillLayer::image, &FillLayer::setImage, &FillLayer::clearImage, &FillLayer::initialFillImage, &CSSToStyleMap::mapFillImage>::createHandler());
    setPropertyHandler(CSSPropertyBackgroundOrigin, ApplyPropertyFillLayer<EFillBox, CSSPropertyBackgroundOrigin, BackgroundFillLayer, &RenderStyle::accessBackgroundLayers, &RenderStyle::backgroundLayers, &FillLayer::isOriginSet, &FillLayer::origin, &FillLayer::setOrigin, &FillLayer::clearOrigin, &FillLayer::initialFillOrigin, &CSSToStyleMap::mapFillOrigin>::createHandler());
    setPropertyHandler(CSSPropertyBackgroundPositionX, ApplyPropertyFillLayer<Length, CSSPropertyBackgroundPositionX, BackgroundFillLayer, &RenderStyle::accessBackgroundLayers, &RenderStyle::backgroundLayers, &FillLayer::isXPositionSet, &FillLayer::xPosition, &FillLayer::setXPosition, &FillLayer::clearXPosition, &FillLayer::initialFillXPosition, &CSSToStyleMap::mapFillXPosition>::createHandler());
    setPropertyHandler(CSSPropertyBackgroundPositionY, ApplyPropertyFillLayer<Length, CSSPropertyBackgroundPositionY, BackgroundFillLayer, &RenderStyle::accessBackgroundLayers, &RenderStyle::backgroundLayers, &FillLayer::isYPositionSet, &FillLayer::yPosition, &FillLayer::setYPosition, &FillLayer::clearYPosition, &FillLayer::initialFillYPosition, &CSSToStyleMap::mapFillYPosition>::createHandler());
    setPropertyHandler(CSSPropertyBackgroundRepeatX, ApplyPropertyFillLayer<EFillRepeat, CSSPropertyBackgroundRepeatX, BackgroundFillLayer, &RenderStyle::accessBackgroundLayers, &RenderStyle::backgroundLayers, &FillLayer::isRepeatXSet, &FillLayer::repeatX, &FillLayer::setRepeatX, &FillLayer::clearRepeatX, &FillLayer::initialFillRepeatX, &CSSToStyleMap::mapFillRepeatX>::createHandler());
    setPropertyHandler(CSSPropertyBackgroundRepeatY, ApplyPropertyFillLayer<EFillRepeat, CSSPropertyBackgroundRepeatY, BackgroundFillLayer, &RenderStyle::accessBackgroundLayers, &RenderStyle::backgroundLayers, &FillLayer::isRepeatYSet, &FillLayer::repeatY, &FillLayer::setRepeatY, &FillLayer::clearRepeatY, &FillLayer::initialFillRepeatY, &CSSToStyleMap::mapFillRepeatY>::createHandler());
    setPropertyHandler(CSSPropertyBackgroundSize, ApplyPropertyFillLayer<FillSize, CSSPropertyBackgroundSize, BackgroundFillLayer, &RenderStyle::accessBackgroundLayers, &RenderStyle::backgroundLayers, &FillLayer::isSizeSet, &FillLayer::size, &FillLayer::setSize, &FillLayer::clearSize, &FillLayer::initialFillSize, &CSSToStyleMap::mapFillSize>::createHandler());
    setPropertyHandler(CSSPropertyBorderBottomColor, ApplyPropertyColor<NoInheritFromParent, &RenderStyle::borderBottomColor, &RenderStyle::setBorderBottomColor, &RenderStyle::setVisitedLinkBorderBottomColor, &RenderStyle::color>::createHandler());
    setPropertyHandler(CSSPropertyBorderBottomLeftRadius, ApplyPropertyBorderRadius<&RenderStyle::borderBottomLeftRadius, &RenderStyle::setBorderBottomLeftRadius, &RenderStyle::initialBorderRadius>::createHandler());
    setPropertyHandler(CSSPropertyBorderBottomRightRadius, ApplyPropertyBorderRadius<&RenderStyle::borderBottomRightRadius, &RenderStyle::setBorderBottomRightRadius, &RenderStyle::initialBorderRadius>::createHandler());
    setPropertyHandler(CSSPropertyBorderBottomStyle, ApplyPropertyDefault<EBorderStyle, &RenderStyle::borderBottomStyle, EBorderStyle, &RenderStyle::setBorderBottomStyle, EBorderStyle, &RenderStyle::initialBorderStyle>::createHandler());
    setPropertyHandler(CSSPropertyBorderBottomWidth, ApplyPropertyComputeLength<unsigned, &RenderStyle::borderBottomWidth, &RenderStyle::setBorderBottomWidth, &RenderStyle::initialBorderWidth, NormalDisabled, ThicknessEnabled>::createHandler());
    setPropertyHandler(CSSPropertyBorderCollapse, ApplyPropertyDefault<EBorderCollapse, &RenderStyle::borderCollapse, EBorderCollapse, &RenderStyle::setBorderCollapse, EBorderCollapse, &RenderStyle::initialBorderCollapse>::createHandler());
    setPropertyHandler(CSSPropertyBorderImageOutset, ApplyPropertyBorderImageModifier<BorderImage, Outset>::createHandler());
    setPropertyHandler(CSSPropertyBorderImageRepeat, ApplyPropertyBorderImageModifier<BorderImage, Repeat>::createHandler());
    setPropertyHandler(CSSPropertyBorderImageSlice, ApplyPropertyBorderImageModifier<BorderImage, Slice>::createHandler());
    setPropertyHandler(CSSPropertyBorderImageSource, ApplyPropertyBorderImageSource<CSSPropertyBorderImageSource, &RenderStyle::borderImageSource, &RenderStyle::setBorderImageSource, &RenderStyle::initialBorderImageSource>::createHandler());
    setPropertyHandler(CSSPropertyBorderImageWidth, ApplyPropertyBorderImageModifier<BorderImage, Width>::createHandler());
    setPropertyHandler(CSSPropertyBorderLeftColor, ApplyPropertyColor<NoInheritFromParent, &RenderStyle::borderLeftColor, &RenderStyle::setBorderLeftColor, &RenderStyle::setVisitedLinkBorderLeftColor, &RenderStyle::color>::createHandler());
    setPropertyHandler(CSSPropertyBorderLeftStyle, ApplyPropertyDefault<EBorderStyle, &RenderStyle::borderLeftStyle, EBorderStyle, &RenderStyle::setBorderLeftStyle, EBorderStyle, &RenderStyle::initialBorderStyle>::createHandler());
    setPropertyHandler(CSSPropertyBorderLeftWidth, ApplyPropertyComputeLength<unsigned, &RenderStyle::borderLeftWidth, &RenderStyle::setBorderLeftWidth, &RenderStyle::initialBorderWidth, NormalDisabled, ThicknessEnabled>::createHandler());
    setPropertyHandler(CSSPropertyBorderRightColor, ApplyPropertyColor<NoInheritFromParent, &RenderStyle::borderRightColor, &RenderStyle::setBorderRightColor, &RenderStyle::setVisitedLinkBorderRightColor, &RenderStyle::color>::createHandler());
    setPropertyHandler(CSSPropertyBorderRightStyle, ApplyPropertyDefault<EBorderStyle, &RenderStyle::borderRightStyle, EBorderStyle, &RenderStyle::setBorderRightStyle, EBorderStyle, &RenderStyle::initialBorderStyle>::createHandler());
    setPropertyHandler(CSSPropertyBorderRightWidth, ApplyPropertyComputeLength<unsigned, &RenderStyle::borderRightWidth, &RenderStyle::setBorderRightWidth, &RenderStyle::initialBorderWidth, NormalDisabled, ThicknessEnabled>::createHandler());
    setPropertyHandler(CSSPropertyBorderTopColor, ApplyPropertyColor<NoInheritFromParent, &RenderStyle::borderTopColor, &RenderStyle::setBorderTopColor, &RenderStyle::setVisitedLinkBorderTopColor, &RenderStyle::color>::createHandler());
    setPropertyHandler(CSSPropertyBorderTopLeftRadius, ApplyPropertyBorderRadius<&RenderStyle::borderTopLeftRadius, &RenderStyle::setBorderTopLeftRadius, &RenderStyle::initialBorderRadius>::createHandler());
    setPropertyHandler(CSSPropertyBorderTopRightRadius, ApplyPropertyBorderRadius<&RenderStyle::borderTopRightRadius, &RenderStyle::setBorderTopRightRadius, &RenderStyle::initialBorderRadius>::createHandler());
    setPropertyHandler(CSSPropertyBorderTopStyle, ApplyPropertyDefault<EBorderStyle, &RenderStyle::borderTopStyle, EBorderStyle, &RenderStyle::setBorderTopStyle, EBorderStyle, &RenderStyle::initialBorderStyle>::createHandler());
    setPropertyHandler(CSSPropertyBorderTopWidth, ApplyPropertyComputeLength<unsigned, &RenderStyle::borderTopWidth, &RenderStyle::setBorderTopWidth, &RenderStyle::initialBorderWidth, NormalDisabled, ThicknessEnabled>::createHandler());
    setPropertyHandler(CSSPropertyBottom, ApplyPropertyLength<&RenderStyle::bottom, &RenderStyle::setBottom, &RenderStyle::initialOffset, AutoEnabled>::createHandler());
    setPropertyHandler(CSSPropertyBoxSizing, ApplyPropertyDefault<EBoxSizing, &RenderStyle::boxSizing, EBoxSizing, &RenderStyle::setBoxSizing, EBoxSizing, &RenderStyle::initialBoxSizing>::createHandler());
    setPropertyHandler(CSSPropertyCaptionSide, ApplyPropertyDefault<ECaptionSide, &RenderStyle::captionSide, ECaptionSide, &RenderStyle::setCaptionSide, ECaptionSide, &RenderStyle::initialCaptionSide>::createHandler());
    setPropertyHandler(CSSPropertyClear, ApplyPropertyDefault<EClear, &RenderStyle::clear, EClear, &RenderStyle::setClear, EClear, &RenderStyle::initialClear>::createHandler());
    setPropertyHandler(CSSPropertyClip, ApplyPropertyClip::createHandler());
    setPropertyHandler(CSSPropertyColor, ApplyPropertyColor<InheritFromParent, &RenderStyle::color, &RenderStyle::setColor, &RenderStyle::setVisitedLinkColor, &RenderStyle::invalidColor, RenderStyle::initialColor>::createHandler());
    setPropertyHandler(CSSPropertyCounterIncrement, ApplyPropertyCounter<Increment>::createHandler());
    setPropertyHandler(CSSPropertyCounterReset, ApplyPropertyCounter<Reset>::createHandler());
    setPropertyHandler(CSSPropertyCursor, ApplyPropertyCursor::createHandler());
    setPropertyHandler(CSSPropertyDirection, ApplyPropertyDirection<&RenderStyle::direction, &RenderStyle::setDirection, RenderStyle::initialDirection>::createHandler());
    setPropertyHandler(CSSPropertyDisplay, ApplyPropertyDisplay::createHandler());
    setPropertyHandler(CSSPropertyEmptyCells, ApplyPropertyDefault<EEmptyCell, &RenderStyle::emptyCells, EEmptyCell, &RenderStyle::setEmptyCells, EEmptyCell, &RenderStyle::initialEmptyCells>::createHandler());
    setPropertyHandler(CSSPropertyFloat, ApplyPropertyDefault<EFloat, &RenderStyle::floating, EFloat, &RenderStyle::setFloating, EFloat, &RenderStyle::initialFloating>::createHandler());
    setPropertyHandler(CSSPropertyFontFamily, ApplyPropertyFontFamily::createHandler());
    setPropertyHandler(CSSPropertyFontSize, ApplyPropertyFontSize::createHandler());
    setPropertyHandler(CSSPropertyFontStyle, ApplyPropertyFont<FontItalic, &FontDescription::italic, &FontDescription::setItalic, FontItalicOff>::createHandler());
    setPropertyHandler(CSSPropertyFontVariant, ApplyPropertyFont<FontSmallCaps, &FontDescription::smallCaps, &FontDescription::setSmallCaps, FontSmallCapsOff>::createHandler());
    setPropertyHandler(CSSPropertyFontWeight, ApplyPropertyFontWeight::createHandler());
    setPropertyHandler(CSSPropertyHeight, ApplyPropertyLength<&RenderStyle::height, &RenderStyle::setHeight, &RenderStyle::initialSize, AutoEnabled, LegacyIntrinsicEnabled, IntrinsicDisabled, NoneDisabled, UndefinedDisabled>::createHandler());
#if ENABLE(CSS_IMAGE_ORIENTATION)
    setPropertyHandler(CSSPropertyImageOrientation, ApplyPropertyDefault<ImageOrientationEnum, &RenderStyle::imageOrientation, ImageOrientationEnum, &RenderStyle::setImageOrientation, ImageOrientationEnum, &RenderStyle::initialImageOrientation>::createHandler());
#endif
    setPropertyHandler(CSSPropertyImageRendering, ApplyPropertyDefault<EImageRendering, &RenderStyle::imageRendering, EImageRendering, &RenderStyle::setImageRendering, EImageRendering, &RenderStyle::initialImageRendering>::createHandler());
#if ENABLE(CSS_IMAGE_RESOLUTION)
    setPropertyHandler(CSSPropertyImageResolution, ApplyPropertyImageResolution::createHandler());
#endif
    setPropertyHandler(CSSPropertyLeft, ApplyPropertyLength<&RenderStyle::left, &RenderStyle::setLeft, &RenderStyle::initialOffset, AutoEnabled>::createHandler());
    setPropertyHandler(CSSPropertyLetterSpacing, ApplyPropertyComputeLength<int, &RenderStyle::letterSpacing, &RenderStyle::setLetterSpacing, &RenderStyle::initialLetterWordSpacing, NormalEnabled, ThicknessDisabled, SVGZoomEnabled>::createHandler());
    setPropertyHandler(CSSPropertyLineHeight, ApplyPropertyLineHeight::createHandler());
    setPropertyHandler(CSSPropertyListStyleImage, ApplyPropertyStyleImage<&RenderStyle::listStyleImage, &RenderStyle::setListStyleImage, &RenderStyle::initialListStyleImage, CSSPropertyListStyleImage>::createHandler());
    setPropertyHandler(CSSPropertyListStylePosition, ApplyPropertyDefault<EListStylePosition, &RenderStyle::listStylePosition, EListStylePosition, &RenderStyle::setListStylePosition, EListStylePosition, &RenderStyle::initialListStylePosition>::createHandler());
    setPropertyHandler(CSSPropertyListStyleType, ApplyPropertyDefault<EListStyleType, &RenderStyle::listStyleType, EListStyleType, &RenderStyle::setListStyleType, EListStyleType, &RenderStyle::initialListStyleType>::createHandler());
    setPropertyHandler(CSSPropertyMarginBottom, ApplyPropertyLength<&RenderStyle::marginBottom, &RenderStyle::setMarginBottom, &RenderStyle::initialMargin, AutoEnabled>::createHandler());
    setPropertyHandler(CSSPropertyMarginLeft, ApplyPropertyLength<&RenderStyle::marginLeft, &RenderStyle::setMarginLeft, &RenderStyle::initialMargin, AutoEnabled>::createHandler());
    setPropertyHandler(CSSPropertyMarginRight, ApplyPropertyLength<&RenderStyle::marginRight, &RenderStyle::setMarginRight, &RenderStyle::initialMargin, AutoEnabled>::createHandler());
    setPropertyHandler(CSSPropertyMarginTop, ApplyPropertyLength<&RenderStyle::marginTop, &RenderStyle::setMarginTop, &RenderStyle::initialMargin, AutoEnabled>::createHandler());
    setPropertyHandler(CSSPropertyMaxHeight, ApplyPropertyLength<&RenderStyle::maxHeight, &RenderStyle::setMaxHeight, &RenderStyle::initialMaxSize, AutoEnabled, LegacyIntrinsicEnabled, IntrinsicDisabled, NoneEnabled, UndefinedEnabled>::createHandler());
    setPropertyHandler(CSSPropertyMaxWidth, ApplyPropertyLength<&RenderStyle::maxWidth, &RenderStyle::setMaxWidth, &RenderStyle::initialMaxSize, AutoEnabled, LegacyIntrinsicEnabled, IntrinsicEnabled, NoneEnabled, UndefinedEnabled>::createHandler());
    setPropertyHandler(CSSPropertyMinHeight, ApplyPropertyLength<&RenderStyle::minHeight, &RenderStyle::setMinHeight, &RenderStyle::initialMinSize, AutoEnabled, LegacyIntrinsicEnabled, IntrinsicDisabled>::createHandler());
    setPropertyHandler(CSSPropertyMinWidth, ApplyPropertyLength<&RenderStyle::minWidth, &RenderStyle::setMinWidth, &RenderStyle::initialMinSize, AutoEnabled, LegacyIntrinsicEnabled, IntrinsicEnabled>::createHandler());
    setPropertyHandler(CSSPropertyOpacity, ApplyPropertyDefault<float, &RenderStyle::opacity, float, &RenderStyle::setOpacity, float, &RenderStyle::initialOpacity>::createHandler());
    setPropertyHandler(CSSPropertyOrphans, ApplyPropertyAuto<short, &RenderStyle::orphans, &RenderStyle::setOrphans, &RenderStyle::hasAutoOrphans, &RenderStyle::setHasAutoOrphans>::createHandler());
    setPropertyHandler(CSSPropertyOutlineColor, ApplyPropertyColor<NoInheritFromParent, &RenderStyle::outlineColor, &RenderStyle::setOutlineColor, &RenderStyle::setVisitedLinkOutlineColor, &RenderStyle::color>::createHandler());
    setPropertyHandler(CSSPropertyOutlineOffset, ApplyPropertyComputeLength<int, &RenderStyle::outlineOffset, &RenderStyle::setOutlineOffset, &RenderStyle::initialOutlineOffset>::createHandler());
    setPropertyHandler(CSSPropertyOutlineStyle, ApplyPropertyOutlineStyle::createHandler());
    setPropertyHandler(CSSPropertyOutlineWidth, ApplyPropertyComputeLength<unsigned short, &RenderStyle::outlineWidth, &RenderStyle::setOutlineWidth, &RenderStyle::initialOutlineWidth, NormalDisabled, ThicknessEnabled>::createHandler());
    setPropertyHandler(CSSPropertyOverflowWrap, ApplyPropertyDefault<EOverflowWrap, &RenderStyle::overflowWrap, EOverflowWrap, &RenderStyle::setOverflowWrap, EOverflowWrap, &RenderStyle::initialOverflowWrap>::createHandler());
    setPropertyHandler(CSSPropertyOverflowX, ApplyPropertyDefault<EOverflow, &RenderStyle::overflowX, EOverflow, &RenderStyle::setOverflowX, EOverflow, &RenderStyle::initialOverflowX>::createHandler());
    setPropertyHandler(CSSPropertyOverflowY, ApplyPropertyDefault<EOverflow, &RenderStyle::overflowY, EOverflow, &RenderStyle::setOverflowY, EOverflow, &RenderStyle::initialOverflowY>::createHandler());
    setPropertyHandler(CSSPropertyPaddingBottom, ApplyPropertyLength<&RenderStyle::paddingBottom, &RenderStyle::setPaddingBottom, &RenderStyle::initialPadding>::createHandler());
    setPropertyHandler(CSSPropertyPaddingLeft, ApplyPropertyLength<&RenderStyle::paddingLeft, &RenderStyle::setPaddingLeft, &RenderStyle::initialPadding>::createHandler());
    setPropertyHandler(CSSPropertyPaddingRight, ApplyPropertyLength<&RenderStyle::paddingRight, &RenderStyle::setPaddingRight, &RenderStyle::initialPadding>::createHandler());
    setPropertyHandler(CSSPropertyPaddingTop, ApplyPropertyLength<&RenderStyle::paddingTop, &RenderStyle::setPaddingTop, &RenderStyle::initialPadding>::createHandler());
    setPropertyHandler(CSSPropertyPageBreakAfter, ApplyPropertyDefault<EPageBreak, &RenderStyle::pageBreakAfter, EPageBreak, &RenderStyle::setPageBreakAfter, EPageBreak, &RenderStyle::initialPageBreak>::createHandler());
    setPropertyHandler(CSSPropertyPageBreakBefore, ApplyPropertyDefault<EPageBreak, &RenderStyle::pageBreakBefore, EPageBreak, &RenderStyle::setPageBreakBefore, EPageBreak, &RenderStyle::initialPageBreak>::createHandler());
    setPropertyHandler(CSSPropertyPageBreakInside, ApplyPropertyDefault<EPageBreak, &RenderStyle::pageBreakInside, EPageBreak, &RenderStyle::setPageBreakInside, EPageBreak, &RenderStyle::initialPageBreak>::createHandler());
    setPropertyHandler(CSSPropertyPointerEvents, ApplyPropertyDefault<EPointerEvents, &RenderStyle::pointerEvents, EPointerEvents, &RenderStyle::setPointerEvents, EPointerEvents, &RenderStyle::initialPointerEvents>::createHandler());
    setPropertyHandler(CSSPropertyPosition, ApplyPropertyDefault<EPosition, &RenderStyle::position, EPosition, &RenderStyle::setPosition, EPosition, &RenderStyle::initialPosition>::createHandler());
    setPropertyHandler(CSSPropertyResize, ApplyPropertyResize::createHandler());
    setPropertyHandler(CSSPropertyRight, ApplyPropertyLength<&RenderStyle::right, &RenderStyle::setRight, &RenderStyle::initialOffset, AutoEnabled>::createHandler());
    setPropertyHandler(CSSPropertySize, ApplyPropertyPageSize::createHandler());
    setPropertyHandler(CSSPropertySpeak, ApplyPropertyDefault<ESpeak, &RenderStyle::speak, ESpeak, &RenderStyle::setSpeak, ESpeak, &RenderStyle::initialSpeak>::createHandler());
    setPropertyHandler(CSSPropertyTableLayout, ApplyPropertyDefault<ETableLayout, &RenderStyle::tableLayout, ETableLayout, &RenderStyle::setTableLayout, ETableLayout, &RenderStyle::initialTableLayout>::createHandler());
    setPropertyHandler(CSSPropertyTabSize, ApplyPropertyDefault<unsigned, &RenderStyle::tabSize, unsigned, &RenderStyle::setTabSize, unsigned, &RenderStyle::initialTabSize>::createHandler());
    setPropertyHandler(CSSPropertyTextAlign, ApplyPropertyTextAlign::createHandler());
    setPropertyHandler(CSSPropertyTextDecoration, ApplyPropertyTextDecoration::createHandler());
#if ENABLE(CSS3_TEXT)
    setPropertyHandler(CSSPropertyWebkitTextDecorationLine, ApplyPropertyTextDecoration::createHandler());
    setPropertyHandler(CSSPropertyWebkitTextDecorationStyle, ApplyPropertyDefault<TextDecorationStyle, &RenderStyle::textDecorationStyle, TextDecorationStyle, &RenderStyle::setTextDecorationStyle, TextDecorationStyle, &RenderStyle::initialTextDecorationStyle>::createHandler());
    setPropertyHandler(CSSPropertyWebkitTextDecorationColor, ApplyPropertyColor<NoInheritFromParent, &RenderStyle::textDecorationColor, &RenderStyle::setTextDecorationColor, &RenderStyle::setVisitedLinkTextDecorationColor, &RenderStyle::color>::createHandler());
    setPropertyHandler(CSSPropertyWebkitTextAlignLast, ApplyPropertyDefault<TextAlignLast, &RenderStyle::textAlignLast, TextAlignLast, &RenderStyle::setTextAlignLast, TextAlignLast, &RenderStyle::initialTextAlignLast>::createHandler());
    setPropertyHandler(CSSPropertyWebkitTextJustify, ApplyPropertyDefault<TextJustify, &RenderStyle::textJustify, TextJustify, &RenderStyle::setTextJustify, TextJustify, &RenderStyle::initialTextJustify>::createHandler());
    setPropertyHandler(CSSPropertyWebkitTextUnderlinePosition, ApplyPropertyTextUnderlinePosition::createHandler());
#endif // CSS3_TEXT
    setPropertyHandler(CSSPropertyTextIndent, ApplyPropertyTextIndent::createHandler());
    setPropertyHandler(CSSPropertyTextOverflow, ApplyPropertyDefault<TextOverflow, &RenderStyle::textOverflow, TextOverflow, &RenderStyle::setTextOverflow, TextOverflow, &RenderStyle::initialTextOverflow>::createHandler());
    setPropertyHandler(CSSPropertyTextRendering, ApplyPropertyFont<TextRenderingMode, &FontDescription::textRenderingMode, &FontDescription::setTextRenderingMode, AutoTextRendering>::createHandler());
    setPropertyHandler(CSSPropertyTextTransform, ApplyPropertyDefault<ETextTransform, &RenderStyle::textTransform, ETextTransform, &RenderStyle::setTextTransform, ETextTransform, &RenderStyle::initialTextTransform>::createHandler());
    setPropertyHandler(CSSPropertyTop, ApplyPropertyLength<&RenderStyle::top, &RenderStyle::setTop, &RenderStyle::initialOffset, AutoEnabled>::createHandler());
    setPropertyHandler(CSSPropertyUnicodeBidi, ApplyPropertyDefault<EUnicodeBidi, &RenderStyle::unicodeBidi, EUnicodeBidi, &RenderStyle::setUnicodeBidi, EUnicodeBidi, &RenderStyle::initialUnicodeBidi>::createHandler());
    setPropertyHandler(CSSPropertyVerticalAlign, ApplyPropertyVerticalAlign::createHandler());
    setPropertyHandler(CSSPropertyVisibility, ApplyPropertyDefault<EVisibility, &RenderStyle::visibility, EVisibility, &RenderStyle::setVisibility, EVisibility, &RenderStyle::initialVisibility>::createHandler());
    setPropertyHandler(CSSPropertyWebkitAnimationDelay, ApplyPropertyAnimation<double, &Animation::delay, &Animation::setDelay, &Animation::isDelaySet, &Animation::clearDelay, &Animation::initialAnimationDelay, &CSSToStyleMap::mapAnimationDelay, &RenderStyle::accessAnimations, &RenderStyle::animations>::createHandler());
    setPropertyHandler(CSSPropertyWebkitAnimationDirection, ApplyPropertyAnimation<Animation::AnimationDirection, &Animation::direction, &Animation::setDirection, &Animation::isDirectionSet, &Animation::clearDirection, &Animation::initialAnimationDirection, &CSSToStyleMap::mapAnimationDirection, &RenderStyle::accessAnimations, &RenderStyle::animations>::createHandler());
    setPropertyHandler(CSSPropertyWebkitAnimationDuration, ApplyPropertyAnimation<double, &Animation::duration, &Animation::setDuration, &Animation::isDurationSet, &Animation::clearDuration, &Animation::initialAnimationDuration, &CSSToStyleMap::mapAnimationDuration, &RenderStyle::accessAnimations, &RenderStyle::animations>::createHandler());
    setPropertyHandler(CSSPropertyWebkitAnimationFillMode, ApplyPropertyAnimation<unsigned, &Animation::fillMode, &Animation::setFillMode, &Animation::isFillModeSet, &Animation::clearFillMode, &Animation::initialAnimationFillMode, &CSSToStyleMap::mapAnimationFillMode, &RenderStyle::accessAnimations, &RenderStyle::animations>::createHandler());
    setPropertyHandler(CSSPropertyWebkitAnimationIterationCount, ApplyPropertyAnimation<double, &Animation::iterationCount, &Animation::setIterationCount, &Animation::isIterationCountSet, &Animation::clearIterationCount, &Animation::initialAnimationIterationCount, &CSSToStyleMap::mapAnimationIterationCount, &RenderStyle::accessAnimations, &RenderStyle::animations>::createHandler());
    setPropertyHandler(CSSPropertyWebkitAnimationName, ApplyPropertyAnimation<const String&, &Animation::name, &Animation::setName, &Animation::isNameSet, &Animation::clearName, &Animation::initialAnimationName, &CSSToStyleMap::mapAnimationName, &RenderStyle::accessAnimations, &RenderStyle::animations>::createHandler());
    setPropertyHandler(CSSPropertyWebkitAnimationPlayState, ApplyPropertyAnimation<EAnimPlayState, &Animation::playState, &Animation::setPlayState, &Animation::isPlayStateSet, &Animation::clearPlayState, &Animation::initialAnimationPlayState, &CSSToStyleMap::mapAnimationPlayState, &RenderStyle::accessAnimations, &RenderStyle::animations>::createHandler());
    setPropertyHandler(CSSPropertyWebkitAnimationTimingFunction, ApplyPropertyAnimation<const PassRefPtr<TimingFunction>, &Animation::timingFunction, &Animation::setTimingFunction, &Animation::isTimingFunctionSet, &Animation::clearTimingFunction, &Animation::initialAnimationTimingFunction, &CSSToStyleMap::mapAnimationTimingFunction, &RenderStyle::accessAnimations, &RenderStyle::animations>::createHandler());
    setPropertyHandler(CSSPropertyWebkitAppearance, ApplyPropertyDefault<ControlPart, &RenderStyle::appearance, ControlPart, &RenderStyle::setAppearance, ControlPart, &RenderStyle::initialAppearance>::createHandler());
    setPropertyHandler(CSSPropertyWebkitAspectRatio, ApplyPropertyAspectRatio::createHandler());
    setPropertyHandler(CSSPropertyWebkitBackfaceVisibility, ApplyPropertyDefault<EBackfaceVisibility, &RenderStyle::backfaceVisibility, EBackfaceVisibility, &RenderStyle::setBackfaceVisibility, EBackfaceVisibility, &RenderStyle::initialBackfaceVisibility>::createHandler());
    setPropertyHandler(CSSPropertyWebkitBackgroundBlendMode, ApplyPropertyFillLayer<BlendMode, CSSPropertyWebkitBackgroundBlendMode, BackgroundFillLayer, &RenderStyle::accessBackgroundLayers, &RenderStyle::backgroundLayers, &FillLayer::isBlendModeSet, &FillLayer::blendMode, &FillLayer::setBlendMode, &FillLayer::clearBlendMode, &FillLayer::initialFillBlendMode, &CSSToStyleMap::mapFillBlendMode>::createHandler());
    setPropertyHandler(CSSPropertyWebkitBackgroundClip, CSSPropertyBackgroundClip);
    setPropertyHandler(CSSPropertyWebkitBackgroundComposite, ApplyPropertyFillLayer<CompositeOperator, CSSPropertyWebkitBackgroundComposite, BackgroundFillLayer, &RenderStyle::accessBackgroundLayers, &RenderStyle::backgroundLayers, &FillLayer::isCompositeSet, &FillLayer::composite, &FillLayer::setComposite, &FillLayer::clearComposite, &FillLayer::initialFillComposite, &CSSToStyleMap::mapFillComposite>::createHandler());
    setPropertyHandler(CSSPropertyWebkitBackgroundOrigin, CSSPropertyBackgroundOrigin);
    setPropertyHandler(CSSPropertyWebkitBackgroundSize, CSSPropertyBackgroundSize);
#if ENABLE(CSS_COMPOSITING)
    setPropertyHandler(CSSPropertyWebkitBlendMode, ApplyPropertyDefault<BlendMode, &RenderStyle::blendMode, BlendMode, &RenderStyle::setBlendMode, BlendMode, &RenderStyle::initialBlendMode>::createHandler());
#endif
    setPropertyHandler(CSSPropertyWebkitBorderFit, ApplyPropertyDefault<EBorderFit, &RenderStyle::borderFit, EBorderFit, &RenderStyle::setBorderFit, EBorderFit, &RenderStyle::initialBorderFit>::createHandler());
    setPropertyHandler(CSSPropertyWebkitBorderHorizontalSpacing, ApplyPropertyComputeLength<short, &RenderStyle::horizontalBorderSpacing, &RenderStyle::setHorizontalBorderSpacing, &RenderStyle::initialHorizontalBorderSpacing>::createHandler());
    setPropertyHandler(CSSPropertyWebkitBorderImage, ApplyPropertyBorderImage<BorderImage, CSSPropertyWebkitBorderImage, &RenderStyle::borderImage, &RenderStyle::setBorderImage>::createHandler());
    setPropertyHandler(CSSPropertyWebkitBorderVerticalSpacing, ApplyPropertyComputeLength<short, &RenderStyle::verticalBorderSpacing, &RenderStyle::setVerticalBorderSpacing, &RenderStyle::initialVerticalBorderSpacing>::createHandler());
    setPropertyHandler(CSSPropertyWebkitBoxAlign, ApplyPropertyDefault<EBoxAlignment, &RenderStyle::boxAlign, EBoxAlignment, &RenderStyle::setBoxAlign, EBoxAlignment, &RenderStyle::initialBoxAlign>::createHandler());
#if ENABLE(CSS_BOX_DECORATION_BREAK)
    setPropertyHandler(CSSPropertyWebkitBoxDecorationBreak, ApplyPropertyDefault<EBoxDecorationBreak, &RenderStyle::boxDecorationBreak, EBoxDecorationBreak, &RenderStyle::setBoxDecorationBreak, EBoxDecorationBreak, &RenderStyle::initialBoxDecorationBreak>::createHandler());
#endif
    setPropertyHandler(CSSPropertyWebkitBoxDirection, ApplyPropertyDefault<EBoxDirection, &RenderStyle::boxDirection, EBoxDirection, &RenderStyle::setBoxDirection, EBoxDirection, &RenderStyle::initialBoxDirection>::createHandler());
    setPropertyHandler(CSSPropertyWebkitBoxFlex, ApplyPropertyDefault<float, &RenderStyle::boxFlex, float, &RenderStyle::setBoxFlex, float, &RenderStyle::initialBoxFlex>::createHandler());
    setPropertyHandler(CSSPropertyWebkitBoxFlexGroup, ApplyPropertyDefault<unsigned int, &RenderStyle::boxFlexGroup, unsigned int, &RenderStyle::setBoxFlexGroup, unsigned int, &RenderStyle::initialBoxFlexGroup>::createHandler());
    setPropertyHandler(CSSPropertyWebkitBoxLines, ApplyPropertyDefault<EBoxLines, &RenderStyle::boxLines, EBoxLines, &RenderStyle::setBoxLines, EBoxLines, &RenderStyle::initialBoxLines>::createHandler());
    setPropertyHandler(CSSPropertyWebkitBoxOrdinalGroup, ApplyPropertyDefault<unsigned int, &RenderStyle::boxOrdinalGroup, unsigned int, &RenderStyle::setBoxOrdinalGroup, unsigned int, &RenderStyle::initialBoxOrdinalGroup>::createHandler());
    setPropertyHandler(CSSPropertyWebkitBoxOrient, ApplyPropertyDefault<EBoxOrient, &RenderStyle::boxOrient, EBoxOrient, &RenderStyle::setBoxOrient, EBoxOrient, &RenderStyle::initialBoxOrient>::createHandler());
    setPropertyHandler(CSSPropertyWebkitBoxPack, ApplyPropertyDefault<EBoxPack, &RenderStyle::boxPack, EBoxPack, &RenderStyle::setBoxPack, EBoxPack, &RenderStyle::initialBoxPack>::createHandler());
    setPropertyHandler(CSSPropertyWebkitColorCorrection, ApplyPropertyDefault<ColorSpace, &RenderStyle::colorSpace, ColorSpace, &RenderStyle::setColorSpace, ColorSpace, &RenderStyle::initialColorSpace>::createHandler());
    setPropertyHandler(CSSPropertyWebkitColumnAxis, ApplyPropertyDefault<ColumnAxis, &RenderStyle::columnAxis, ColumnAxis, &RenderStyle::setColumnAxis, ColumnAxis, &RenderStyle::initialColumnAxis>::createHandler());
    setPropertyHandler(CSSPropertyWebkitColumnBreakAfter, ApplyPropertyDefault<EPageBreak, &RenderStyle::columnBreakAfter, EPageBreak, &RenderStyle::setColumnBreakAfter, EPageBreak, &RenderStyle::initialPageBreak>::createHandler());
    setPropertyHandler(CSSPropertyWebkitColumnBreakBefore, ApplyPropertyDefault<EPageBreak, &RenderStyle::columnBreakBefore, EPageBreak, &RenderStyle::setColumnBreakBefore, EPageBreak, &RenderStyle::initialPageBreak>::createHandler());
    setPropertyHandler(CSSPropertyWebkitColumnBreakInside, ApplyPropertyDefault<EPageBreak, &RenderStyle::columnBreakInside, EPageBreak, &RenderStyle::setColumnBreakInside, EPageBreak, &RenderStyle::initialPageBreak>::createHandler());
    setPropertyHandler(CSSPropertyWebkitColumnCount, ApplyPropertyAuto<unsigned short, &RenderStyle::columnCount, &RenderStyle::setColumnCount, &RenderStyle::hasAutoColumnCount, &RenderStyle::setHasAutoColumnCount>::createHandler());
    setPropertyHandler(CSSPropertyWebkitColumnGap, ApplyPropertyAuto<float, &RenderStyle::columnGap, &RenderStyle::setColumnGap, &RenderStyle::hasNormalColumnGap, &RenderStyle::setHasNormalColumnGap, ComputeLength, CSSValueNormal>::createHandler());
    setPropertyHandler(CSSPropertyWebkitColumnProgression, ApplyPropertyDefault<ColumnProgression, &RenderStyle::columnProgression, ColumnProgression, &RenderStyle::setColumnProgression, ColumnProgression, &RenderStyle::initialColumnProgression>::createHandler());
    setPropertyHandler(CSSPropertyWebkitColumnRuleColor, ApplyPropertyColor<NoInheritFromParent, &RenderStyle::columnRuleColor, &RenderStyle::setColumnRuleColor, &RenderStyle::setVisitedLinkColumnRuleColor, &RenderStyle::color>::createHandler());
    setPropertyHandler(CSSPropertyWebkitColumnRuleWidth, ApplyPropertyComputeLength<unsigned short, &RenderStyle::columnRuleWidth, &RenderStyle::setColumnRuleWidth, &RenderStyle::initialColumnRuleWidth, NormalDisabled, ThicknessEnabled>::createHandler());
    setPropertyHandler(CSSPropertyWebkitColumnSpan, ApplyPropertyDefault<ColumnSpan, &RenderStyle::columnSpan, ColumnSpan, &RenderStyle::setColumnSpan, ColumnSpan, &RenderStyle::initialColumnSpan>::createHandler());
    setPropertyHandler(CSSPropertyWebkitColumnRuleStyle, ApplyPropertyDefault<EBorderStyle, &RenderStyle::columnRuleStyle, EBorderStyle, &RenderStyle::setColumnRuleStyle, EBorderStyle, &RenderStyle::initialBorderStyle>::createHandler());
    setPropertyHandler(CSSPropertyWebkitColumnWidth, ApplyPropertyAuto<float, &RenderStyle::columnWidth, &RenderStyle::setColumnWidth, &RenderStyle::hasAutoColumnWidth, &RenderStyle::setHasAutoColumnWidth, ComputeLength>::createHandler());
#if ENABLE(CURSOR_VISIBILITY)
    setPropertyHandler(CSSPropertyWebkitCursorVisibility, ApplyPropertyDefault<CursorVisibility, &RenderStyle::cursorVisibility, CursorVisibility, &RenderStyle::setCursorVisibility, CursorVisibility, &RenderStyle::initialCursorVisibility>::createHandler());
#endif
    setPropertyHandler(CSSPropertyWebkitAlignContent, ApplyPropertyDefault<EAlignContent, &RenderStyle::alignContent, EAlignContent, &RenderStyle::setAlignContent, EAlignContent, &RenderStyle::initialAlignContent>::createHandler());
    setPropertyHandler(CSSPropertyWebkitAlignItems, ApplyPropertyDefault<EAlignItems, &RenderStyle::alignItems, EAlignItems, &RenderStyle::setAlignItems, EAlignItems, &RenderStyle::initialAlignItems>::createHandler());
    setPropertyHandler(CSSPropertyWebkitAlignSelf, ApplyPropertyDefault<EAlignItems, &RenderStyle::alignSelf, EAlignItems, &RenderStyle::setAlignSelf, EAlignItems, &RenderStyle::initialAlignSelf>::createHandler());
    setPropertyHandler(CSSPropertyWebkitFlexBasis, ApplyPropertyLength<&RenderStyle::flexBasis, &RenderStyle::setFlexBasis, &RenderStyle::initialFlexBasis, AutoEnabled>::createHandler());
    setPropertyHandler(CSSPropertyWebkitFlexDirection, ApplyPropertyDefault<EFlexDirection, &RenderStyle::flexDirection, EFlexDirection, &RenderStyle::setFlexDirection, EFlexDirection, &RenderStyle::initialFlexDirection>::createHandler());
    setPropertyHandler(CSSPropertyWebkitFlexGrow, ApplyPropertyDefault<float, &RenderStyle::flexGrow, float, &RenderStyle::setFlexGrow, float, &RenderStyle::initialFlexGrow>::createHandler());
    setPropertyHandler(CSSPropertyWebkitFlexShrink, ApplyPropertyDefault<float, &RenderStyle::flexShrink, float, &RenderStyle::setFlexShrink, float, &RenderStyle::initialFlexShrink>::createHandler());
    setPropertyHandler(CSSPropertyWebkitFlexWrap, ApplyPropertyDefault<EFlexWrap, &RenderStyle::flexWrap, EFlexWrap, &RenderStyle::setFlexWrap, EFlexWrap, &RenderStyle::initialFlexWrap>::createHandler());
    setPropertyHandler(CSSPropertyWebkitGridAutoFlow, ApplyPropertyDefault<GridAutoFlow, &RenderStyle::gridAutoFlow, GridAutoFlow, &RenderStyle::setGridAutoFlow, GridAutoFlow, &RenderStyle::initialGridAutoFlow>::createHandler());
    setPropertyHandler(CSSPropertyWebkitJustifyContent, ApplyPropertyDefault<EJustifyContent, &RenderStyle::justifyContent, EJustifyContent, &RenderStyle::setJustifyContent, EJustifyContent, &RenderStyle::initialJustifyContent>::createHandler());
    setPropertyHandler(CSSPropertyWebkitOrder, ApplyPropertyDefault<int, &RenderStyle::order, int, &RenderStyle::setOrder, int, &RenderStyle::initialOrder>::createHandler());
#if ENABLE(CSS_REGIONS)
    setPropertyHandler(CSSPropertyWebkitFlowFrom, ApplyPropertyString<MapNoneToNull, &RenderStyle::regionThread, &RenderStyle::setRegionThread, &RenderStyle::initialRegionThread>::createHandler());
    setPropertyHandler(CSSPropertyWebkitFlowInto, ApplyPropertyString<MapNoneToNull, &RenderStyle::flowThread, &RenderStyle::setFlowThread, &RenderStyle::initialFlowThread>::createHandler());
#endif
    setPropertyHandler(CSSPropertyWebkitFontKerning, ApplyPropertyFont<FontDescription::Kerning, &FontDescription::kerning, &FontDescription::setKerning, FontDescription::AutoKerning>::createHandler());
    setPropertyHandler(CSSPropertyWebkitFontSmoothing, ApplyPropertyFont<FontSmoothingMode, &FontDescription::fontSmoothing, &FontDescription::setFontSmoothing, AutoSmoothing>::createHandler());
    setPropertyHandler(CSSPropertyWebkitFontVariantLigatures, ApplyPropertyFontVariantLigatures::createHandler());
    setPropertyHandler(CSSPropertyWebkitHighlight, ApplyPropertyString<MapNoneToNull, &RenderStyle::highlight, &RenderStyle::setHighlight, &RenderStyle::initialHighlight>::createHandler());
    setPropertyHandler(CSSPropertyWebkitHyphenateCharacter, ApplyPropertyString<MapAutoToNull, &RenderStyle::hyphenationString, &RenderStyle::setHyphenationString, &RenderStyle::initialHyphenationString>::createHandler());
    setPropertyHandler(CSSPropertyWebkitHyphenateLimitAfter, ApplyPropertyNumber<short, &RenderStyle::hyphenationLimitAfter, &RenderStyle::setHyphenationLimitAfter, &RenderStyle::initialHyphenationLimitAfter>::createHandler());
    setPropertyHandler(CSSPropertyWebkitHyphenateLimitBefore, ApplyPropertyNumber<short, &RenderStyle::hyphenationLimitBefore, &RenderStyle::setHyphenationLimitBefore, &RenderStyle::initialHyphenationLimitBefore>::createHandler());
    setPropertyHandler(CSSPropertyWebkitHyphenateLimitLines, ApplyPropertyNumber<short, &RenderStyle::hyphenationLimitLines, &RenderStyle::setHyphenationLimitLines, &RenderStyle::initialHyphenationLimitLines, CSSValueNoLimit>::createHandler());
    setPropertyHandler(CSSPropertyWebkitHyphens, ApplyPropertyDefault<Hyphens, &RenderStyle::hyphens, Hyphens, &RenderStyle::setHyphens, Hyphens, &RenderStyle::initialHyphens>::createHandler());
    setPropertyHandler(CSSPropertyWebkitLineAlign, ApplyPropertyDefault<LineAlign, &RenderStyle::lineAlign, LineAlign, &RenderStyle::setLineAlign, LineAlign, &RenderStyle::initialLineAlign>::createHandler());
    setPropertyHandler(CSSPropertyWebkitLineBreak, ApplyPropertyDefault<LineBreak, &RenderStyle::lineBreak, LineBreak, &RenderStyle::setLineBreak, LineBreak, &RenderStyle::initialLineBreak>::createHandler());
    setPropertyHandler(CSSPropertyWebkitLineClamp, ApplyPropertyDefault<const LineClampValue&, &RenderStyle::lineClamp, LineClampValue, &RenderStyle::setLineClamp, LineClampValue, &RenderStyle::initialLineClamp>::createHandler());
    setPropertyHandler(CSSPropertyWebkitLineGrid, ApplyPropertyString<MapNoneToNull, &RenderStyle::lineGrid, &RenderStyle::setLineGrid, &RenderStyle::initialLineGrid>::createHandler());
    setPropertyHandler(CSSPropertyWebkitLineSnap, ApplyPropertyDefault<LineSnap, &RenderStyle::lineSnap, LineSnap, &RenderStyle::setLineSnap, LineSnap, &RenderStyle::initialLineSnap>::createHandler());
    setPropertyHandler(CSSPropertyWebkitMarginAfterCollapse, ApplyPropertyDefault<EMarginCollapse, &RenderStyle::marginAfterCollapse, EMarginCollapse, &RenderStyle::setMarginAfterCollapse, EMarginCollapse, &RenderStyle::initialMarginAfterCollapse>::createHandler());
    setPropertyHandler(CSSPropertyWebkitMarginBeforeCollapse, ApplyPropertyDefault<EMarginCollapse, &RenderStyle::marginBeforeCollapse, EMarginCollapse, &RenderStyle::setMarginBeforeCollapse, EMarginCollapse, &RenderStyle::initialMarginBeforeCollapse>::createHandler());
    setPropertyHandler(CSSPropertyWebkitMarginBottomCollapse, CSSPropertyWebkitMarginAfterCollapse);
    setPropertyHandler(CSSPropertyWebkitMarginTopCollapse, CSSPropertyWebkitMarginBeforeCollapse);
    setPropertyHandler(CSSPropertyWebkitMarqueeDirection, ApplyPropertyDefault<EMarqueeDirection, &RenderStyle::marqueeDirection, EMarqueeDirection, &RenderStyle::setMarqueeDirection, EMarqueeDirection, &RenderStyle::initialMarqueeDirection>::createHandler());
    setPropertyHandler(CSSPropertyWebkitMarqueeIncrement, ApplyPropertyMarqueeIncrement::createHandler());
    setPropertyHandler(CSSPropertyWebkitMarqueeRepetition, ApplyPropertyMarqueeRepetition::createHandler());
    setPropertyHandler(CSSPropertyWebkitMarqueeSpeed, ApplyPropertyMarqueeSpeed::createHandler());
    setPropertyHandler(CSSPropertyWebkitMarqueeStyle, ApplyPropertyDefault<EMarqueeBehavior, &RenderStyle::marqueeBehavior, EMarqueeBehavior, &RenderStyle::setMarqueeBehavior, EMarqueeBehavior, &RenderStyle::initialMarqueeBehavior>::createHandler());
    setPropertyHandler(CSSPropertyWebkitMaskBoxImage, ApplyPropertyBorderImage<BorderMask, CSSPropertyWebkitMaskBoxImage, &RenderStyle::maskBoxImage, &RenderStyle::setMaskBoxImage>::createHandler());
    setPropertyHandler(CSSPropertyWebkitMaskBoxImageOutset, ApplyPropertyBorderImageModifier<BorderMask, Outset>::createHandler());
    setPropertyHandler(CSSPropertyWebkitMaskBoxImageRepeat, ApplyPropertyBorderImageModifier<BorderMask, Repeat>::createHandler());
    setPropertyHandler(CSSPropertyWebkitMaskBoxImageSlice, ApplyPropertyBorderImageModifier<BorderMask, Slice>::createHandler());
    setPropertyHandler(CSSPropertyWebkitMaskBoxImageSource, ApplyPropertyBorderImageSource<CSSPropertyWebkitMaskBoxImageSource, &RenderStyle::maskBoxImageSource, &RenderStyle::setMaskBoxImageSource, &RenderStyle::initialMaskBoxImageSource>::createHandler());
    setPropertyHandler(CSSPropertyWebkitMaskBoxImageWidth, ApplyPropertyBorderImageModifier<BorderMask, Width>::createHandler());
    setPropertyHandler(CSSPropertyWebkitMaskClip, ApplyPropertyFillLayer<EFillBox, CSSPropertyWebkitMaskClip, MaskFillLayer, &RenderStyle::accessMaskLayers, &RenderStyle::maskLayers, &FillLayer::isClipSet, &FillLayer::clip, &FillLayer::setClip, &FillLayer::clearClip, &FillLayer::initialFillClip, &CSSToStyleMap::mapFillClip>::createHandler());
    setPropertyHandler(CSSPropertyWebkitMaskComposite, ApplyPropertyFillLayer<CompositeOperator, CSSPropertyWebkitMaskComposite, MaskFillLayer, &RenderStyle::accessMaskLayers, &RenderStyle::maskLayers, &FillLayer::isCompositeSet, &FillLayer::composite, &FillLayer::setComposite, &FillLayer::clearComposite, &FillLayer::initialFillComposite, &CSSToStyleMap::mapFillComposite>::createHandler());
    setPropertyHandler(CSSPropertyWebkitMaskImage, ApplyPropertyFillLayer<StyleImage*, CSSPropertyWebkitMaskImage, MaskFillLayer, &RenderStyle::accessMaskLayers, &RenderStyle::maskLayers, &FillLayer::isImageSet, &FillLayer::image, &FillLayer::setImage, &FillLayer::clearImage, &FillLayer::initialFillImage, &CSSToStyleMap::mapFillImage>::createHandler());
    setPropertyHandler(CSSPropertyWebkitMaskOrigin, ApplyPropertyFillLayer<EFillBox, CSSPropertyWebkitMaskOrigin, MaskFillLayer, &RenderStyle::accessMaskLayers, &RenderStyle::maskLayers, &FillLayer::isOriginSet, &FillLayer::origin, &FillLayer::setOrigin, &FillLayer::clearOrigin, &FillLayer::initialFillOrigin, &CSSToStyleMap::mapFillOrigin>::createHandler());
    setPropertyHandler(CSSPropertyWebkitMaskPositionX, ApplyPropertyFillLayer<Length, CSSPropertyWebkitMaskPositionX, MaskFillLayer, &RenderStyle::accessMaskLayers, &RenderStyle::maskLayers, &FillLayer::isXPositionSet, &FillLayer::xPosition, &FillLayer::setXPosition, &FillLayer::clearXPosition, &FillLayer::initialFillXPosition, &CSSToStyleMap::mapFillXPosition>::createHandler());
    setPropertyHandler(CSSPropertyWebkitMaskPositionY, ApplyPropertyFillLayer<Length, CSSPropertyWebkitMaskPositionY, MaskFillLayer, &RenderStyle::accessMaskLayers, &RenderStyle::maskLayers, &FillLayer::isYPositionSet, &FillLayer::yPosition, &FillLayer::setYPosition, &FillLayer::clearYPosition, &FillLayer::initialFillYPosition, &CSSToStyleMap::mapFillYPosition>::createHandler());
    setPropertyHandler(CSSPropertyWebkitMaskRepeatX, ApplyPropertyFillLayer<EFillRepeat, CSSPropertyWebkitMaskRepeatX, MaskFillLayer, &RenderStyle::accessMaskLayers, &RenderStyle::maskLayers, &FillLayer::isRepeatXSet, &FillLayer::repeatX, &FillLayer::setRepeatX, &FillLayer::clearRepeatX, &FillLayer::initialFillRepeatX, &CSSToStyleMap::mapFillRepeatX>::createHandler());
    setPropertyHandler(CSSPropertyWebkitMaskRepeatY, ApplyPropertyFillLayer<EFillRepeat, CSSPropertyWebkitMaskRepeatY, MaskFillLayer, &RenderStyle::accessMaskLayers, &RenderStyle::maskLayers, &FillLayer::isRepeatYSet, &FillLayer::repeatY, &FillLayer::setRepeatY, &FillLayer::clearRepeatY, &FillLayer::initialFillRepeatY, &CSSToStyleMap::mapFillRepeatY>::createHandler());
    setPropertyHandler(CSSPropertyWebkitMaskSize, ApplyPropertyFillLayer<FillSize, CSSPropertyWebkitMaskSize, MaskFillLayer, &RenderStyle::accessMaskLayers, &RenderStyle::maskLayers, &FillLayer::isSizeSet, &FillLayer::size, &FillLayer::setSize, &FillLayer::clearSize, &FillLayer::initialFillSize, &CSSToStyleMap::mapFillSize>::createHandler());
    setPropertyHandler(CSSPropertyWebkitNbspMode, ApplyPropertyDefault<ENBSPMode, &RenderStyle::nbspMode, ENBSPMode, &RenderStyle::setNBSPMode, ENBSPMode, &RenderStyle::initialNBSPMode>::createHandler());
    setPropertyHandler(CSSPropertyWebkitPerspectiveOrigin, ApplyPropertyExpanding<SuppressValue, CSSPropertyWebkitPerspectiveOriginX, CSSPropertyWebkitPerspectiveOriginY>::createHandler());
    setPropertyHandler(CSSPropertyWebkitPerspectiveOriginX, ApplyPropertyLength<&RenderStyle::perspectiveOriginX, &RenderStyle::setPerspectiveOriginX, &RenderStyle::initialPerspectiveOriginX>::createHandler());
    setPropertyHandler(CSSPropertyWebkitPerspectiveOriginY, ApplyPropertyLength<&RenderStyle::perspectiveOriginY, &RenderStyle::setPerspectiveOriginY, &RenderStyle::initialPerspectiveOriginY>::createHandler());
    setPropertyHandler(CSSPropertyWebkitPrintColorAdjust, ApplyPropertyDefault<PrintColorAdjust, &RenderStyle::printColorAdjust, PrintColorAdjust, &RenderStyle::setPrintColorAdjust, PrintColorAdjust, &RenderStyle::initialPrintColorAdjust>::createHandler());
#if ENABLE(CSS_REGIONS)
    setPropertyHandler(CSSPropertyWebkitRegionBreakAfter, ApplyPropertyDefault<EPageBreak, &RenderStyle::regionBreakAfter, EPageBreak, &RenderStyle::setRegionBreakAfter, EPageBreak, &RenderStyle::initialPageBreak>::createHandler());
    setPropertyHandler(CSSPropertyWebkitRegionBreakBefore, ApplyPropertyDefault<EPageBreak, &RenderStyle::regionBreakBefore, EPageBreak, &RenderStyle::setRegionBreakBefore, EPageBreak, &RenderStyle::initialPageBreak>::createHandler());
    setPropertyHandler(CSSPropertyWebkitRegionBreakInside, ApplyPropertyDefault<EPageBreak, &RenderStyle::regionBreakInside, EPageBreak, &RenderStyle::setRegionBreakInside, EPageBreak, &RenderStyle::initialPageBreak>::createHandler());
    setPropertyHandler(CSSPropertyWebkitRegionFragment, ApplyPropertyDefault<RegionFragment, &RenderStyle::regionFragment, RegionFragment, &RenderStyle::setRegionFragment, RegionFragment, &RenderStyle::initialRegionFragment>::createHandler());
#endif
    setPropertyHandler(CSSPropertyWebkitRtlOrdering, ApplyPropertyDefault<Order, &RenderStyle::rtlOrdering, Order, &RenderStyle::setRTLOrdering, Order, &RenderStyle::initialRTLOrdering>::createHandler());
    setPropertyHandler(CSSPropertyWebkitRubyPosition, ApplyPropertyDefault<RubyPosition, &RenderStyle::rubyPosition, RubyPosition, &RenderStyle::setRubyPosition, RubyPosition, &RenderStyle::initialRubyPosition>::createHandler());
    setPropertyHandler(CSSPropertyWebkitTextCombine, ApplyPropertyDefault<TextCombine, &RenderStyle::textCombine, TextCombine, &RenderStyle::setTextCombine, TextCombine, &RenderStyle::initialTextCombine>::createHandler());
    setPropertyHandler(CSSPropertyWebkitTextEmphasisColor, ApplyPropertyColor<NoInheritFromParent, &RenderStyle::textEmphasisColor, &RenderStyle::setTextEmphasisColor, &RenderStyle::setVisitedLinkTextEmphasisColor, &RenderStyle::color>::createHandler());
    setPropertyHandler(CSSPropertyWebkitTextEmphasisPosition, ApplyPropertyDefault<TextEmphasisPosition, &RenderStyle::textEmphasisPosition, TextEmphasisPosition, &RenderStyle::setTextEmphasisPosition, TextEmphasisPosition, &RenderStyle::initialTextEmphasisPosition>::createHandler());
    setPropertyHandler(CSSPropertyWebkitTextEmphasisStyle, ApplyPropertyTextEmphasisStyle::createHandler());
    setPropertyHandler(CSSPropertyWebkitTextFillColor, ApplyPropertyColor<NoInheritFromParent, &RenderStyle::textFillColor, &RenderStyle::setTextFillColor, &RenderStyle::setVisitedLinkTextFillColor, &RenderStyle::color>::createHandler());
    setPropertyHandler(CSSPropertyWebkitTextSecurity, ApplyPropertyDefault<ETextSecurity, &RenderStyle::textSecurity, ETextSecurity, &RenderStyle::setTextSecurity, ETextSecurity, &RenderStyle::initialTextSecurity>::createHandler());
    setPropertyHandler(CSSPropertyWebkitTextStrokeColor, ApplyPropertyColor<NoInheritFromParent, &RenderStyle::textStrokeColor, &RenderStyle::setTextStrokeColor, &RenderStyle::setVisitedLinkTextStrokeColor, &RenderStyle::color>::createHandler());
    setPropertyHandler(CSSPropertyWebkitTransformOriginX, ApplyPropertyLength<&RenderStyle::transformOriginX, &RenderStyle::setTransformOriginX, &RenderStyle::initialTransformOriginX>::createHandler());
    setPropertyHandler(CSSPropertyWebkitTransformOriginY, ApplyPropertyLength<&RenderStyle::transformOriginY, &RenderStyle::setTransformOriginY, &RenderStyle::initialTransformOriginY>::createHandler());
    setPropertyHandler(CSSPropertyWebkitTransformOriginZ, ApplyPropertyComputeLength<float, &RenderStyle::transformOriginZ, &RenderStyle::setTransformOriginZ, &RenderStyle::initialTransformOriginZ>::createHandler());
    setPropertyHandler(CSSPropertyWebkitTransformStyle, ApplyPropertyDefault<ETransformStyle3D, &RenderStyle::transformStyle3D, ETransformStyle3D, &RenderStyle::setTransformStyle3D, ETransformStyle3D, &RenderStyle::initialTransformStyle3D>::createHandler());
    setPropertyHandler(CSSPropertyWebkitTransitionDelay, ApplyPropertyAnimation<double, &Animation::delay, &Animation::setDelay, &Animation::isDelaySet, &Animation::clearDelay, &Animation::initialAnimationDelay, &CSSToStyleMap::mapAnimationDelay, &RenderStyle::accessTransitions, &RenderStyle::transitions>::createHandler());
    setPropertyHandler(CSSPropertyWebkitTransitionDuration, ApplyPropertyAnimation<double, &Animation::duration, &Animation::setDuration, &Animation::isDurationSet, &Animation::clearDuration, &Animation::initialAnimationDuration, &CSSToStyleMap::mapAnimationDuration, &RenderStyle::accessTransitions, &RenderStyle::transitions>::createHandler());
    setPropertyHandler(CSSPropertyWebkitTransitionProperty, ApplyPropertyAnimation<CSSPropertyID, &Animation::property, &Animation::setProperty, &Animation::isPropertySet, &Animation::clearProperty, &Animation::initialAnimationProperty, &CSSToStyleMap::mapAnimationProperty, &RenderStyle::accessTransitions, &RenderStyle::transitions>::createHandler());
    setPropertyHandler(CSSPropertyWebkitTransitionTimingFunction, ApplyPropertyAnimation<const PassRefPtr<TimingFunction>, &Animation::timingFunction, &Animation::setTimingFunction, &Animation::isTimingFunctionSet, &Animation::clearTimingFunction, &Animation::initialAnimationTimingFunction, &CSSToStyleMap::mapAnimationTimingFunction, &RenderStyle::accessTransitions, &RenderStyle::transitions>::createHandler());
    setPropertyHandler(CSSPropertyWebkitUserDrag, ApplyPropertyDefault<EUserDrag, &RenderStyle::userDrag, EUserDrag, &RenderStyle::setUserDrag, EUserDrag, &RenderStyle::initialUserDrag>::createHandler());
    setPropertyHandler(CSSPropertyWebkitUserModify, ApplyPropertyDefault<EUserModify, &RenderStyle::userModify, EUserModify, &RenderStyle::setUserModify, EUserModify, &RenderStyle::initialUserModify>::createHandler());
    setPropertyHandler(CSSPropertyWebkitUserSelect, ApplyPropertyDefault<EUserSelect, &RenderStyle::userSelect, EUserSelect, &RenderStyle::setUserSelect, EUserSelect, &RenderStyle::initialUserSelect>::createHandler());
    setPropertyHandler(CSSPropertyWebkitClipPath, ApplyPropertyClipPath<&RenderStyle::clipPath, &RenderStyle::setClipPath, &RenderStyle::initialClipPath>::createHandler());

#if ENABLE(CSS_EXCLUSIONS)
    setPropertyHandler(CSSPropertyWebkitWrapFlow, ApplyPropertyDefault<WrapFlow, &RenderStyle::wrapFlow, WrapFlow, &RenderStyle::setWrapFlow, WrapFlow, &RenderStyle::initialWrapFlow>::createHandler());
    setPropertyHandler(CSSPropertyWebkitWrapThrough, ApplyPropertyDefault<WrapThrough, &RenderStyle::wrapThrough, WrapThrough, &RenderStyle::setWrapThrough, WrapThrough, &RenderStyle::initialWrapThrough>::createHandler());
#endif
#if ENABLE(CSS_SHAPES)
    setPropertyHandler(CSSPropertyWebkitShapeMargin, ApplyPropertyLength<&RenderStyle::shapeMargin, &RenderStyle::setShapeMargin, &RenderStyle::initialShapeMargin>::createHandler());
    setPropertyHandler(CSSPropertyWebkitShapePadding, ApplyPropertyLength<&RenderStyle::shapePadding, &RenderStyle::setShapePadding, &RenderStyle::initialShapePadding>::createHandler());
    setPropertyHandler(CSSPropertyWebkitShapeInside, ApplyPropertyShape<&RenderStyle::shapeInside, &RenderStyle::setShapeInside, &RenderStyle::initialShapeInside>::createHandler());
    setPropertyHandler(CSSPropertyWebkitShapeOutside, ApplyPropertyShape<&RenderStyle::shapeOutside, &RenderStyle::setShapeOutside, &RenderStyle::initialShapeOutside>::createHandler());
#endif
    setPropertyHandler(CSSPropertyWhiteSpace, ApplyPropertyDefault<EWhiteSpace, &RenderStyle::whiteSpace, EWhiteSpace, &RenderStyle::setWhiteSpace, EWhiteSpace, &RenderStyle::initialWhiteSpace>::createHandler());
    setPropertyHandler(CSSPropertyWidows, ApplyPropertyAuto<short, &RenderStyle::widows, &RenderStyle::setWidows, &RenderStyle::hasAutoWidows, &RenderStyle::setHasAutoWidows>::createHandler());
    setPropertyHandler(CSSPropertyWidth, ApplyPropertyLength<&RenderStyle::width, &RenderStyle::setWidth, &RenderStyle::initialSize, AutoEnabled, LegacyIntrinsicEnabled, IntrinsicEnabled, NoneDisabled, UndefinedDisabled>::createHandler());
    setPropertyHandler(CSSPropertyWordBreak, ApplyPropertyDefault<EWordBreak, &RenderStyle::wordBreak, EWordBreak, &RenderStyle::setWordBreak, EWordBreak, &RenderStyle::initialWordBreak>::createHandler());
    setPropertyHandler(CSSPropertyWordSpacing, ApplyPropertyComputeLength<int, &RenderStyle::wordSpacing, &RenderStyle::setWordSpacing, &RenderStyle::initialLetterWordSpacing, NormalEnabled, ThicknessDisabled, SVGZoomEnabled>::createHandler());
    // UAs must treat 'word-wrap' as an alternate name for the 'overflow-wrap' property. So using the same handlers.
    setPropertyHandler(CSSPropertyWordWrap, ApplyPropertyDefault<EOverflowWrap, &RenderStyle::overflowWrap, EOverflowWrap, &RenderStyle::setOverflowWrap, EOverflowWrap, &RenderStyle::initialOverflowWrap>::createHandler());
    setPropertyHandler(CSSPropertyZIndex, ApplyPropertyAuto<int, &RenderStyle::zIndex, &RenderStyle::setZIndex, &RenderStyle::hasAutoZIndex, &RenderStyle::setHasAutoZIndex>::createHandler());
    setPropertyHandler(CSSPropertyZoom, ApplyPropertyZoom::createHandler());
}


}
