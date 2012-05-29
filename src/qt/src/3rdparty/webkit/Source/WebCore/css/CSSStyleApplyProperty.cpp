/*
 * Copyright (C) 2011 Google Inc. All rights reserved.
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
#include "CSSStyleApplyProperty.h"

#include "CSSPrimitiveValueMappings.h"
#include "CSSStyleSelector.h"
#include "CSSValueList.h"
#include "Document.h"
#include "Element.h"
#include "RenderStyle.h"
#include <wtf/StdLibExtras.h>
#include <wtf/UnusedParam.h>

using namespace std;

namespace WebCore {

class ApplyPropertyExpanding : public ApplyPropertyBase {
public:
    ApplyPropertyExpanding(ApplyPropertyBase* one = 0, ApplyPropertyBase* two = 0, ApplyPropertyBase *three = 0, ApplyPropertyBase* four = 0)
    {
        m_propertyMap[0] = one;
        m_propertyMap[1] = two;
        m_propertyMap[2] = three;
        m_propertyMap[3] = four;
        m_propertyMap[4] = 0; // always null terminated
    }

    virtual void applyInheritValue(CSSStyleSelector* selector) const
    {
        for (ApplyPropertyBase* const* e = m_propertyMap; *e; e++)
            (*e)->applyInheritValue(selector);
    }

    virtual void applyInitialValue(CSSStyleSelector* selector) const
    {
        for (ApplyPropertyBase* const* e = m_propertyMap; *e; e++)
            (*e)->applyInitialValue(selector);
    }

    virtual void applyValue(CSSStyleSelector* selector, CSSValue* value) const
    {
        for (ApplyPropertyBase* const* e = m_propertyMap; *e; e++)
            (*e)->applyValue(selector, value);
    }
private:
    ApplyPropertyBase* m_propertyMap[5];
};

class ApplyPropertyExpandingSuppressValue : public ApplyPropertyExpanding {
public:
    ApplyPropertyExpandingSuppressValue(ApplyPropertyBase* one = 0, ApplyPropertyBase* two = 0, ApplyPropertyBase *three = 0, ApplyPropertyBase* four = 0)
        : ApplyPropertyExpanding(one, two, three, four) {}

    virtual void applyValue(CSSStyleSelector*, CSSValue*) const
    {
        ASSERT_NOT_REACHED();
    }
};

template <typename T>
class ApplyPropertyDefaultBase : public ApplyPropertyBase {
public:
    typedef T (RenderStyle::*GetterFunction)() const;
    typedef void (RenderStyle::*SetterFunction)(T);
    typedef T (*InitialFunction)();

    ApplyPropertyDefaultBase(GetterFunction getter, SetterFunction setter, InitialFunction initial)
        : m_getter(getter)
        , m_setter(setter)
        , m_initial(initial)
    {
    }

private:
    virtual void applyInheritValue(CSSStyleSelector* selector) const
    {
        (selector->style()->*m_setter)((selector->parentStyle()->*m_getter)());
    }

    virtual void applyInitialValue(CSSStyleSelector* selector) const
    {
        (selector->style()->*m_setter)((*m_initial)());
    }

protected:
    GetterFunction m_getter;
    SetterFunction m_setter;
    InitialFunction m_initial;
};


template <typename T>
class ApplyPropertyDefault : public ApplyPropertyDefaultBase<T> {
public:
    ApplyPropertyDefault(typename ApplyPropertyDefaultBase<T>::GetterFunction getter, typename ApplyPropertyDefaultBase<T>::SetterFunction setter, typename ApplyPropertyDefaultBase<T>::InitialFunction initial)
        : ApplyPropertyDefaultBase<T>(getter, setter, initial)
    {
    }

protected:
    virtual void applyValue(CSSStyleSelector* selector, CSSValue* value) const
    {
        if (value->isPrimitiveValue())
            (selector->style()->*ApplyPropertyDefaultBase<T>::m_setter)(*static_cast<CSSPrimitiveValue*>(value));
    }
};

enum ColorInherit {NoInheritFromParent = 0, InheritFromParent};
template <ColorInherit inheritColorFromParent>
class ApplyPropertyColor : public ApplyPropertyBase {
public:
    typedef const Color& (RenderStyle::*GetterFunction)() const;
    typedef void (RenderStyle::*SetterFunction)(const Color&);
    typedef const Color& (RenderStyle::*DefaultFunction)() const;
    typedef Color (*InitialFunction)();

    ApplyPropertyColor(GetterFunction getter, SetterFunction setter, DefaultFunction defaultFunction, InitialFunction initialFunction = 0)
        : m_getter(getter)
        , m_setter(setter)
        , m_default(defaultFunction)
        , m_initial(initialFunction)
    {
    }

private:
    virtual void applyInheritValue(CSSStyleSelector* selector) const
    {
        const Color& color = (selector->parentStyle()->*m_getter)();
        if (m_default && !color.isValid())
            (selector->style()->*m_setter)((selector->parentStyle()->*m_default)());
        else
            (selector->style()->*m_setter)(color);
    }

    virtual void applyInitialValue(CSSStyleSelector* selector) const
    {
        (selector->style()->*m_setter)(m_initial ? m_initial() : Color());
    }

    virtual void applyValue(CSSStyleSelector* selector, CSSValue* value) const
    {
        if (!value->isPrimitiveValue())
            return;

        CSSPrimitiveValue* primitiveValue = static_cast<CSSPrimitiveValue*>(value);
        if (inheritColorFromParent && primitiveValue->getIdent() == CSSValueCurrentcolor)
            applyInheritValue(selector);
        else
            (selector->style()->*m_setter)(selector->getColorFromPrimitiveValue(primitiveValue));
    }

    GetterFunction m_getter;
    SetterFunction m_setter;
    DefaultFunction m_default;
    InitialFunction m_initial;
};

// CSSPropertyDirection
class ApplyPropertyDirection : public ApplyPropertyDefault<TextDirection> {
public:
    ApplyPropertyDirection(GetterFunction getter, SetterFunction setter, InitialFunction initial)
        : ApplyPropertyDefault<TextDirection>(getter, setter, initial)
    {
    }

private:
    virtual void applyValue(CSSStyleSelector* selector, CSSValue* value) const
    {
        ApplyPropertyDefault<TextDirection>::applyValue(selector, value);
        Element* element = selector->element();
        if (element && selector->element() == element->document()->documentElement())
            element->document()->setDirectionSetOnDocumentElement(true);
    }
};

enum LengthAuto {AutoDisabled = 0, AutoEnabled = 1};
enum LengthIntrinsic {IntrinsicDisabled = 0, IntrinsicEnabled = 1};
enum LengthMinIntrinsic {MinIntrinsicDisabled = 0, MinIntrinsicEnabled = 1};
enum LengthNone {NoneDisabled = 0, NoneEnabled = 1};
enum LengthUndefined {UndefinedDisabled = 0, UndefinedEnabled = 1};
template <LengthAuto autoEnabled = AutoDisabled,
          LengthIntrinsic intrinsicEnabled = IntrinsicDisabled,
          LengthMinIntrinsic minIntrinsicEnabled = MinIntrinsicDisabled,
          LengthNone noneEnabled = NoneDisabled,
          LengthUndefined noneUndefined = UndefinedDisabled>
class ApplyPropertyLength : public ApplyPropertyDefaultBase<Length> {
public:
    ApplyPropertyLength(GetterFunction getter, SetterFunction setter, InitialFunction initial)
        : ApplyPropertyDefaultBase<Length>(getter, setter, initial)
    {
    }

private:
    virtual void applyValue(CSSStyleSelector* selector, CSSValue* value) const
    {
        if (!value->isPrimitiveValue())
            return;

        CSSPrimitiveValue* primitiveValue = static_cast<CSSPrimitiveValue*>(value);
        if (noneEnabled && primitiveValue->getIdent() == CSSValueNone)
            if (noneUndefined)
                (selector->style()->*m_setter)(Length(undefinedLength, Fixed));
            else
                (selector->style()->*m_setter)(Length());
        else if (intrinsicEnabled && primitiveValue->getIdent() == CSSValueIntrinsic)
            (selector->style()->*m_setter)(Length(Intrinsic));
        else if (minIntrinsicEnabled && primitiveValue->getIdent() == CSSValueMinIntrinsic)
            (selector->style()->*m_setter)(Length(MinIntrinsic));
        else if (autoEnabled && primitiveValue->getIdent() == CSSValueAuto)
            (selector->style()->*m_setter)(Length());
        else {
            int type = primitiveValue->primitiveType();
            if (CSSPrimitiveValue::isUnitTypeLength(type))
                (selector->style()->*m_setter)(Length(primitiveValue->computeLengthIntForLength(selector->style(), selector->rootElementStyle(), selector->style()->effectiveZoom()), Fixed, primitiveValue->isQuirkValue()));
            else if (type == CSSPrimitiveValue::CSS_PERCENTAGE)
                (selector->style()->*m_setter)(Length(primitiveValue->getDoubleValue(), Percent));
        }
    }
};

template <typename T>
class ApplyPropertyFillLayer : public ApplyPropertyBase {
public:
    ApplyPropertyFillLayer(CSSPropertyID propertyId, EFillLayerType fillLayerType, FillLayer* (RenderStyle::*accessLayers)(),
                           const FillLayer* (RenderStyle::*layers)() const, bool (FillLayer::*test)() const, T (FillLayer::*get)() const,
                           void (FillLayer::*set)(T), void (FillLayer::*clear)(), T (*initial)(EFillLayerType),
                           void (CSSStyleSelector::*mapFill)(CSSPropertyID, FillLayer*, CSSValue*))
        : m_propertyId(propertyId)
        , m_fillLayerType(fillLayerType)
        , m_accessLayers(accessLayers)
        , m_layers(layers)
        , m_test(test)
        , m_get(get)
        , m_set(set)
        , m_clear(clear)
        , m_initial(initial)
        , m_mapFill(mapFill)
    {
    }

private:
    virtual void applyInheritValue(CSSStyleSelector* selector) const
    {
        FillLayer* currChild = (selector->style()->*m_accessLayers)();
        FillLayer* prevChild = 0;
        const FillLayer* currParent = (selector->parentStyle()->*m_layers)();
        while (currParent && (currParent->*m_test)()) {
            if (!currChild) {
                /* Need to make a new layer.*/
                currChild = new FillLayer(m_fillLayerType);
                prevChild->setNext(currChild);
            }
            (currChild->*m_set)((currParent->*m_get)());
            prevChild = currChild;
            currChild = prevChild->next();
            currParent = currParent->next();
        }

        while (currChild) {
            /* Reset any remaining layers to not have the property set. */
            (currChild->*m_clear)();
            currChild = currChild->next();
        }
    }

    virtual void applyInitialValue(CSSStyleSelector* selector) const
    {
        FillLayer* currChild = (selector->style()->*m_accessLayers)();
        (currChild->*m_set)((*m_initial)(m_fillLayerType));
        for (currChild = currChild->next(); currChild; currChild = currChild->next())
            (currChild->*m_clear)();
    }

    virtual void applyValue(CSSStyleSelector* selector, CSSValue* value) const
    {
        FillLayer* currChild = (selector->style()->*m_accessLayers)();
        FillLayer* prevChild = 0;
        if (value->isValueList()) {
            /* Walk each value and put it into a layer, creating new layers as needed. */
            CSSValueList* valueList = static_cast<CSSValueList*>(value);
            for (unsigned int i = 0; i < valueList->length(); i++) {
                if (!currChild) {
                    /* Need to make a new layer to hold this value */
                    currChild = new FillLayer(m_fillLayerType);
                    prevChild->setNext(currChild);
                }
                (selector->*m_mapFill)(m_propertyId, currChild, valueList->itemWithoutBoundsCheck(i));
                prevChild = currChild;
                currChild = currChild->next();
            }
        } else {
            (selector->*m_mapFill)(m_propertyId, currChild, value);
            currChild = currChild->next();
        }
        while (currChild) {
            /* Reset all remaining layers to not have the property set. */
            (currChild->*m_clear)();
            currChild = currChild->next();
        }
    }

protected:
    CSSPropertyID m_propertyId;
    EFillLayerType m_fillLayerType;
    FillLayer* (RenderStyle::*m_accessLayers)();
    const FillLayer* (RenderStyle::*m_layers)() const;
    bool (FillLayer::*m_test)() const;
    T (FillLayer::*m_get)() const;
    void (FillLayer::*m_set)(T);
    void (FillLayer::*m_clear)();
    T (*m_initial)(EFillLayerType);
    void (CSSStyleSelector::*m_mapFill)(CSSPropertyID, FillLayer*, CSSValue*);
};

class ApplyPropertyWidth : public ApplyPropertyDefaultBase<unsigned short> {
public:
    ApplyPropertyWidth(GetterFunction getter, SetterFunction setter, InitialFunction initial)
        : ApplyPropertyDefaultBase<unsigned short>(getter, setter, initial)
    {
    }

private:
    virtual void applyValue(CSSStyleSelector* selector, CSSValue* value) const
    {
        if (!value->isPrimitiveValue())
            return;

        CSSPrimitiveValue* primitiveValue = static_cast<CSSPrimitiveValue*>(value);
        short width;
        switch (primitiveValue->getIdent()) {
        case CSSValueThin:
            width = 1;
            break;
        case CSSValueMedium:
            width = 3;
            break;
        case CSSValueThick:
            width = 5;
            break;
        case CSSValueInvalid:
            width = primitiveValue->computeLengthShort(selector->style(), selector->rootElementStyle(), selector->style()->effectiveZoom());
            // CSS2 box model does not allow negative lengths for borders.
            if (width < 0)
                return;
            break;
        default:
            return;
        }

        (selector->style()->*m_setter)(width);
    }
};

const CSSStyleApplyProperty& CSSStyleApplyProperty::sharedCSSStyleApplyProperty()
{
    DEFINE_STATIC_LOCAL(CSSStyleApplyProperty, cssStyleApplyPropertyInstance, ());
    return cssStyleApplyPropertyInstance;
}

CSSStyleApplyProperty::CSSStyleApplyProperty()
{
    for (int i = 0; i < numCSSProperties; ++i)
       m_propertyMap[i] = 0;

    setPropertyHandler(CSSPropertyColor, new ApplyPropertyColor<InheritFromParent>(&RenderStyle::color, &RenderStyle::setColor, 0, RenderStyle::initialColor));
    setPropertyHandler(CSSPropertyDirection, new ApplyPropertyDirection(&RenderStyle::direction, &RenderStyle::setDirection, RenderStyle::initialDirection));

    setPropertyHandler(CSSPropertyBackgroundAttachment, new ApplyPropertyFillLayer<EFillAttachment>(CSSPropertyBackgroundAttachment, BackgroundFillLayer, &RenderStyle::accessBackgroundLayers, &RenderStyle::backgroundLayers,
            &FillLayer::isAttachmentSet, &FillLayer::attachment, &FillLayer::setAttachment, &FillLayer::clearAttachment, &FillLayer::initialFillAttachment, &CSSStyleSelector::mapFillAttachment));
    setPropertyHandler(CSSPropertyBackgroundClip, new ApplyPropertyFillLayer<EFillBox>(CSSPropertyBackgroundClip, BackgroundFillLayer, &RenderStyle::accessBackgroundLayers, &RenderStyle::backgroundLayers,
            &FillLayer::isClipSet, &FillLayer::clip, &FillLayer::setClip, &FillLayer::clearClip, &FillLayer::initialFillClip, &CSSStyleSelector::mapFillClip));
    setPropertyHandler(CSSPropertyWebkitBackgroundClip, CSSPropertyBackgroundClip);
    setPropertyHandler(CSSPropertyWebkitBackgroundComposite, new ApplyPropertyFillLayer<CompositeOperator>(CSSPropertyWebkitBackgroundComposite, BackgroundFillLayer, &RenderStyle::accessBackgroundLayers, &RenderStyle::backgroundLayers,
            &FillLayer::isCompositeSet, &FillLayer::composite, &FillLayer::setComposite, &FillLayer::clearComposite, &FillLayer::initialFillComposite, &CSSStyleSelector::mapFillComposite));

    setPropertyHandler(CSSPropertyBackgroundImage, new ApplyPropertyFillLayer<StyleImage*>(CSSPropertyBackgroundImage, BackgroundFillLayer, &RenderStyle::accessBackgroundLayers, &RenderStyle::backgroundLayers,
                &FillLayer::isImageSet, &FillLayer::image, &FillLayer::setImage, &FillLayer::clearImage, &FillLayer::initialFillImage, &CSSStyleSelector::mapFillImage));

    setPropertyHandler(CSSPropertyBackgroundOrigin, new ApplyPropertyFillLayer<EFillBox>(CSSPropertyBackgroundOrigin, BackgroundFillLayer, &RenderStyle::accessBackgroundLayers, &RenderStyle::backgroundLayers,
            &FillLayer::isOriginSet, &FillLayer::origin, &FillLayer::setOrigin, &FillLayer::clearOrigin, &FillLayer::initialFillOrigin, &CSSStyleSelector::mapFillOrigin));
    setPropertyHandler(CSSPropertyWebkitBackgroundOrigin, CSSPropertyBackgroundOrigin);

    setPropertyHandler(CSSPropertyBackgroundPositionX, new ApplyPropertyFillLayer<Length>(CSSPropertyBackgroundPositionX, BackgroundFillLayer, &RenderStyle::accessBackgroundLayers, &RenderStyle::backgroundLayers,
                &FillLayer::isXPositionSet, &FillLayer::xPosition, &FillLayer::setXPosition, &FillLayer::clearXPosition, &FillLayer::initialFillXPosition, &CSSStyleSelector::mapFillXPosition));
    setPropertyHandler(CSSPropertyBackgroundPositionY, new ApplyPropertyFillLayer<Length>(CSSPropertyBackgroundPositionY, BackgroundFillLayer, &RenderStyle::accessBackgroundLayers, &RenderStyle::backgroundLayers,
                    &FillLayer::isYPositionSet, &FillLayer::yPosition, &FillLayer::setYPosition, &FillLayer::clearYPosition, &FillLayer::initialFillYPosition, &CSSStyleSelector::mapFillYPosition));
    setPropertyHandler(CSSPropertyBackgroundPosition, new ApplyPropertyExpandingSuppressValue(propertyHandler(CSSPropertyBackgroundPositionX), propertyHandler(CSSPropertyBackgroundPositionY)));

    setPropertyHandler(CSSPropertyBackgroundRepeatX, new ApplyPropertyFillLayer<EFillRepeat>(CSSPropertyBackgroundRepeatX, BackgroundFillLayer, &RenderStyle::accessBackgroundLayers, &RenderStyle::backgroundLayers,
                &FillLayer::isRepeatXSet, &FillLayer::repeatX, &FillLayer::setRepeatX, &FillLayer::clearRepeatX, &FillLayer::initialFillRepeatX, &CSSStyleSelector::mapFillRepeatX));
    setPropertyHandler(CSSPropertyBackgroundRepeatY, new ApplyPropertyFillLayer<EFillRepeat>(CSSPropertyBackgroundRepeatY, BackgroundFillLayer, &RenderStyle::accessBackgroundLayers, &RenderStyle::backgroundLayers,
                    &FillLayer::isRepeatYSet, &FillLayer::repeatY, &FillLayer::setRepeatY, &FillLayer::clearRepeatY, &FillLayer::initialFillRepeatY, &CSSStyleSelector::mapFillRepeatY));
    setPropertyHandler(CSSPropertyBackgroundRepeat, new ApplyPropertyExpandingSuppressValue(propertyHandler(CSSPropertyBackgroundRepeatX), propertyHandler(CSSPropertyBackgroundRepeatY)));

    setPropertyHandler(CSSPropertyBackgroundSize, new ApplyPropertyFillLayer<FillSize>(CSSPropertyBackgroundSize, BackgroundFillLayer, &RenderStyle::accessBackgroundLayers, &RenderStyle::backgroundLayers,
            &FillLayer::isSizeSet, &FillLayer::size, &FillLayer::setSize, &FillLayer::clearSize, &FillLayer::initialFillSize, &CSSStyleSelector::mapFillSize));
    setPropertyHandler(CSSPropertyWebkitBackgroundSize, CSSPropertyBackgroundSize);

    setPropertyHandler(CSSPropertyWebkitMaskAttachment, new ApplyPropertyFillLayer<EFillAttachment>(CSSPropertyWebkitMaskAttachment, MaskFillLayer, &RenderStyle::accessMaskLayers, &RenderStyle::maskLayers,
            &FillLayer::isAttachmentSet, &FillLayer::attachment, &FillLayer::setAttachment, &FillLayer::clearAttachment, &FillLayer::initialFillAttachment, &CSSStyleSelector::mapFillAttachment));
    setPropertyHandler(CSSPropertyWebkitMaskClip, new ApplyPropertyFillLayer<EFillBox>(CSSPropertyWebkitMaskClip, MaskFillLayer, &RenderStyle::accessMaskLayers, &RenderStyle::maskLayers,
            &FillLayer::isClipSet, &FillLayer::clip, &FillLayer::setClip, &FillLayer::clearClip, &FillLayer::initialFillClip, &CSSStyleSelector::mapFillClip));
    setPropertyHandler(CSSPropertyWebkitMaskComposite, new ApplyPropertyFillLayer<CompositeOperator>(CSSPropertyWebkitMaskComposite, MaskFillLayer, &RenderStyle::accessMaskLayers, &RenderStyle::maskLayers,
            &FillLayer::isCompositeSet, &FillLayer::composite, &FillLayer::setComposite, &FillLayer::clearComposite, &FillLayer::initialFillComposite, &CSSStyleSelector::mapFillComposite));

    setPropertyHandler(CSSPropertyWebkitMaskImage, new ApplyPropertyFillLayer<StyleImage*>(CSSPropertyWebkitMaskImage, MaskFillLayer, &RenderStyle::accessMaskLayers, &RenderStyle::maskLayers,
                &FillLayer::isImageSet, &FillLayer::image, &FillLayer::setImage, &FillLayer::clearImage, &FillLayer::initialFillImage, &CSSStyleSelector::mapFillImage));

    setPropertyHandler(CSSPropertyWebkitMaskOrigin, new ApplyPropertyFillLayer<EFillBox>(CSSPropertyWebkitMaskOrigin, MaskFillLayer, &RenderStyle::accessMaskLayers, &RenderStyle::maskLayers,
            &FillLayer::isOriginSet, &FillLayer::origin, &FillLayer::setOrigin, &FillLayer::clearOrigin, &FillLayer::initialFillOrigin, &CSSStyleSelector::mapFillOrigin));
    setPropertyHandler(CSSPropertyWebkitMaskSize, new ApplyPropertyFillLayer<FillSize>(CSSPropertyWebkitMaskSize, MaskFillLayer, &RenderStyle::accessMaskLayers, &RenderStyle::maskLayers,
            &FillLayer::isSizeSet, &FillLayer::size, &FillLayer::setSize, &FillLayer::clearSize, &FillLayer::initialFillSize, &CSSStyleSelector::mapFillSize));

    setPropertyHandler(CSSPropertyWebkitMaskPositionX, new ApplyPropertyFillLayer<Length>(CSSPropertyWebkitMaskPositionX, MaskFillLayer, &RenderStyle::accessMaskLayers, &RenderStyle::maskLayers,
                &FillLayer::isXPositionSet, &FillLayer::xPosition, &FillLayer::setXPosition, &FillLayer::clearXPosition, &FillLayer::initialFillXPosition, &CSSStyleSelector::mapFillXPosition));
    setPropertyHandler(CSSPropertyWebkitMaskPositionY, new ApplyPropertyFillLayer<Length>(CSSPropertyWebkitMaskPositionY, MaskFillLayer, &RenderStyle::accessMaskLayers, &RenderStyle::maskLayers,
                    &FillLayer::isYPositionSet, &FillLayer::yPosition, &FillLayer::setYPosition, &FillLayer::clearYPosition, &FillLayer::initialFillYPosition, &CSSStyleSelector::mapFillYPosition));
    setPropertyHandler(CSSPropertyWebkitMaskPosition, new ApplyPropertyExpandingSuppressValue(propertyHandler(CSSPropertyWebkitMaskPositionX), propertyHandler(CSSPropertyWebkitMaskPositionY)));

    setPropertyHandler(CSSPropertyWebkitMaskRepeatX, new ApplyPropertyFillLayer<EFillRepeat>(CSSPropertyWebkitMaskRepeatX, MaskFillLayer, &RenderStyle::accessMaskLayers, &RenderStyle::maskLayers,
                &FillLayer::isRepeatXSet, &FillLayer::repeatX, &FillLayer::setRepeatX, &FillLayer::clearRepeatX, &FillLayer::initialFillRepeatX, &CSSStyleSelector::mapFillRepeatX));
    setPropertyHandler(CSSPropertyWebkitMaskRepeatY, new ApplyPropertyFillLayer<EFillRepeat>(CSSPropertyWebkitMaskRepeatY, MaskFillLayer, &RenderStyle::accessMaskLayers, &RenderStyle::maskLayers,
                    &FillLayer::isRepeatYSet, &FillLayer::repeatY, &FillLayer::setRepeatY, &FillLayer::clearRepeatY, &FillLayer::initialFillRepeatY, &CSSStyleSelector::mapFillRepeatY));
    setPropertyHandler(CSSPropertyWebkitMaskRepeat, new ApplyPropertyExpandingSuppressValue(propertyHandler(CSSPropertyBackgroundRepeatX), propertyHandler(CSSPropertyBackgroundRepeatY)));

    setPropertyHandler(CSSPropertyBackgroundColor, new ApplyPropertyColor<NoInheritFromParent>(&RenderStyle::backgroundColor, &RenderStyle::setBackgroundColor, 0));
    setPropertyHandler(CSSPropertyBorderBottomColor, new ApplyPropertyColor<NoInheritFromParent>(&RenderStyle::borderBottomColor, &RenderStyle::setBorderBottomColor, &RenderStyle::color));
    setPropertyHandler(CSSPropertyBorderLeftColor, new ApplyPropertyColor<NoInheritFromParent>(&RenderStyle::borderLeftColor, &RenderStyle::setBorderLeftColor, &RenderStyle::color));
    setPropertyHandler(CSSPropertyBorderRightColor, new ApplyPropertyColor<NoInheritFromParent>(&RenderStyle::borderRightColor, &RenderStyle::setBorderRightColor, &RenderStyle::color));
    setPropertyHandler(CSSPropertyBorderTopColor, new ApplyPropertyColor<NoInheritFromParent>(&RenderStyle::borderTopColor, &RenderStyle::setBorderTopColor, &RenderStyle::color));

    setPropertyHandler(CSSPropertyBorderTopStyle, new ApplyPropertyDefault<EBorderStyle>(&RenderStyle::borderTopStyle, &RenderStyle::setBorderTopStyle, &RenderStyle::initialBorderStyle));
    setPropertyHandler(CSSPropertyBorderRightStyle, new ApplyPropertyDefault<EBorderStyle>(&RenderStyle::borderRightStyle, &RenderStyle::setBorderRightStyle, &RenderStyle::initialBorderStyle));
    setPropertyHandler(CSSPropertyBorderBottomStyle, new ApplyPropertyDefault<EBorderStyle>(&RenderStyle::borderBottomStyle, &RenderStyle::setBorderBottomStyle, &RenderStyle::initialBorderStyle));
    setPropertyHandler(CSSPropertyBorderLeftStyle, new ApplyPropertyDefault<EBorderStyle>(&RenderStyle::borderLeftStyle, &RenderStyle::setBorderLeftStyle, &RenderStyle::initialBorderStyle));

    setPropertyHandler(CSSPropertyBorderTopWidth, new ApplyPropertyWidth(&RenderStyle::borderTopWidth, &RenderStyle::setBorderTopWidth, &RenderStyle::initialBorderWidth));
    setPropertyHandler(CSSPropertyBorderRightWidth, new ApplyPropertyWidth(&RenderStyle::borderRightWidth, &RenderStyle::setBorderRightWidth, &RenderStyle::initialBorderWidth));
    setPropertyHandler(CSSPropertyBorderBottomWidth, new ApplyPropertyWidth(&RenderStyle::borderBottomWidth, &RenderStyle::setBorderBottomWidth, &RenderStyle::initialBorderWidth));
    setPropertyHandler(CSSPropertyBorderLeftWidth, new ApplyPropertyWidth(&RenderStyle::borderLeftWidth, &RenderStyle::setBorderLeftWidth, &RenderStyle::initialBorderWidth));
    setPropertyHandler(CSSPropertyOutlineWidth, new ApplyPropertyWidth(&RenderStyle::outlineWidth, &RenderStyle::setOutlineWidth, &RenderStyle::initialBorderWidth));
    setPropertyHandler(CSSPropertyWebkitColumnRuleWidth, new ApplyPropertyWidth(&RenderStyle::columnRuleWidth, &RenderStyle::setColumnRuleWidth, &RenderStyle::initialBorderWidth));

    setPropertyHandler(CSSPropertyOutlineColor, new ApplyPropertyColor<InheritFromParent>(&RenderStyle::outlineColor, &RenderStyle::setOutlineColor, &RenderStyle::color));

    setPropertyHandler(CSSPropertyOverflowX, new ApplyPropertyDefault<EOverflow>(&RenderStyle::overflowX, &RenderStyle::setOverflowX, &RenderStyle::initialOverflowX));
    setPropertyHandler(CSSPropertyOverflowY, new ApplyPropertyDefault<EOverflow>(&RenderStyle::overflowY, &RenderStyle::setOverflowY, &RenderStyle::initialOverflowY));
    setPropertyHandler(CSSPropertyOverflow, new ApplyPropertyExpanding(propertyHandler(CSSPropertyOverflowX), propertyHandler(CSSPropertyOverflowY)));

    setPropertyHandler(CSSPropertyWebkitColumnRuleColor, new ApplyPropertyColor<NoInheritFromParent>(&RenderStyle::columnRuleColor, &RenderStyle::setColumnRuleColor, &RenderStyle::color));
    setPropertyHandler(CSSPropertyWebkitTextEmphasisColor, new ApplyPropertyColor<NoInheritFromParent>(&RenderStyle::textEmphasisColor, &RenderStyle::setTextEmphasisColor, &RenderStyle::color));
    setPropertyHandler(CSSPropertyWebkitTextFillColor, new ApplyPropertyColor<NoInheritFromParent>(&RenderStyle::textFillColor, &RenderStyle::setTextFillColor, &RenderStyle::color));
    setPropertyHandler(CSSPropertyWebkitTextStrokeColor, new ApplyPropertyColor<NoInheritFromParent>(&RenderStyle::textStrokeColor, &RenderStyle::setTextStrokeColor, &RenderStyle::color));

    setPropertyHandler(CSSPropertyTop, new ApplyPropertyLength<AutoEnabled>(&RenderStyle::top, &RenderStyle::setTop, &RenderStyle::initialOffset));
    setPropertyHandler(CSSPropertyRight, new ApplyPropertyLength<AutoEnabled>(&RenderStyle::right, &RenderStyle::setRight, &RenderStyle::initialOffset));
    setPropertyHandler(CSSPropertyBottom, new ApplyPropertyLength<AutoEnabled>(&RenderStyle::bottom, &RenderStyle::setBottom, &RenderStyle::initialOffset));
    setPropertyHandler(CSSPropertyLeft, new ApplyPropertyLength<AutoEnabled>(&RenderStyle::left, &RenderStyle::setLeft, &RenderStyle::initialOffset));

    setPropertyHandler(CSSPropertyWidth, new ApplyPropertyLength<AutoEnabled, IntrinsicEnabled, MinIntrinsicEnabled>(&RenderStyle::width, &RenderStyle::setWidth, &RenderStyle::initialSize));
    setPropertyHandler(CSSPropertyHeight, new ApplyPropertyLength<AutoEnabled, IntrinsicEnabled, MinIntrinsicEnabled>(&RenderStyle::height, &RenderStyle::setHeight, &RenderStyle::initialSize));

    setPropertyHandler(CSSPropertyTextIndent, new ApplyPropertyLength<>(&RenderStyle::textIndent, &RenderStyle::setTextIndent, &RenderStyle::initialTextIndent));

    setPropertyHandler(CSSPropertyMaxHeight, new ApplyPropertyLength<AutoEnabled, IntrinsicEnabled, MinIntrinsicEnabled, NoneEnabled, UndefinedEnabled>(&RenderStyle::maxHeight, &RenderStyle::setMaxHeight, &RenderStyle::initialMaxSize));
    setPropertyHandler(CSSPropertyMaxWidth, new ApplyPropertyLength<AutoEnabled, IntrinsicEnabled, MinIntrinsicEnabled, NoneEnabled, UndefinedEnabled>(&RenderStyle::maxWidth, &RenderStyle::setMaxWidth, &RenderStyle::initialMaxSize));
    setPropertyHandler(CSSPropertyMinHeight, new ApplyPropertyLength<AutoEnabled, IntrinsicEnabled, MinIntrinsicEnabled>(&RenderStyle::minHeight, &RenderStyle::setMinHeight, &RenderStyle::initialMinSize));
    setPropertyHandler(CSSPropertyMinWidth, new ApplyPropertyLength<AutoEnabled, IntrinsicEnabled, MinIntrinsicEnabled>(&RenderStyle::minWidth, &RenderStyle::setMinWidth, &RenderStyle::initialMinSize));

    setPropertyHandler(CSSPropertyMarginTop, new ApplyPropertyLength<AutoEnabled>(&RenderStyle::marginTop, &RenderStyle::setMarginTop, &RenderStyle::initialMargin));
    setPropertyHandler(CSSPropertyMarginRight, new ApplyPropertyLength<AutoEnabled>(&RenderStyle::marginRight, &RenderStyle::setMarginRight, &RenderStyle::initialMargin));
    setPropertyHandler(CSSPropertyMarginBottom, new ApplyPropertyLength<AutoEnabled>(&RenderStyle::marginBottom, &RenderStyle::setMarginBottom, &RenderStyle::initialMargin));
    setPropertyHandler(CSSPropertyMarginLeft, new ApplyPropertyLength<AutoEnabled>(&RenderStyle::marginLeft, &RenderStyle::setMarginLeft, &RenderStyle::initialMargin));

    setPropertyHandler(CSSPropertyPaddingTop, new ApplyPropertyLength<>(&RenderStyle::paddingTop, &RenderStyle::setPaddingTop, &RenderStyle::initialPadding));
    setPropertyHandler(CSSPropertyPaddingRight, new ApplyPropertyLength<>(&RenderStyle::paddingRight, &RenderStyle::setPaddingRight, &RenderStyle::initialPadding));
    setPropertyHandler(CSSPropertyPaddingBottom, new ApplyPropertyLength<>(&RenderStyle::paddingBottom, &RenderStyle::setPaddingBottom, &RenderStyle::initialPadding));
    setPropertyHandler(CSSPropertyPaddingLeft, new ApplyPropertyLength<>(&RenderStyle::paddingLeft, &RenderStyle::setPaddingLeft, &RenderStyle::initialPadding));

    setPropertyHandler(CSSPropertyWebkitPerspectiveOriginX, new ApplyPropertyLength<>(&RenderStyle::perspectiveOriginX, &RenderStyle::setPerspectiveOriginX, &RenderStyle::initialPerspectiveOriginX));
    setPropertyHandler(CSSPropertyWebkitPerspectiveOriginY, new ApplyPropertyLength<>(&RenderStyle::perspectiveOriginY, &RenderStyle::setPerspectiveOriginY, &RenderStyle::initialPerspectiveOriginY));
    setPropertyHandler(CSSPropertyWebkitTransformOriginX, new ApplyPropertyLength<>(&RenderStyle::transformOriginX, &RenderStyle::setTransformOriginX, &RenderStyle::initialTransformOriginX));
    setPropertyHandler(CSSPropertyWebkitTransformOriginY, new ApplyPropertyLength<>(&RenderStyle::transformOriginY, &RenderStyle::setTransformOriginY, &RenderStyle::initialTransformOriginY));
}


}
