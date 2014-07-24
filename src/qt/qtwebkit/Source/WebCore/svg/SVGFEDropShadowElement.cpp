/*
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

#if ENABLE(SVG) && ENABLE(FILTERS)
#include "SVGFEDropShadowElement.h"

#include "Attribute.h"
#include "RenderStyle.h"
#include "SVGElementInstance.h"
#include "SVGFilterBuilder.h"
#include "SVGNames.h"
#include "SVGParserUtilities.h"
#include "SVGRenderStyle.h"

namespace WebCore {

// Animated property definitions
DEFINE_ANIMATED_STRING(SVGFEDropShadowElement, SVGNames::inAttr, In1, in1)
DEFINE_ANIMATED_NUMBER(SVGFEDropShadowElement, SVGNames::dxAttr, Dx, dx)
DEFINE_ANIMATED_NUMBER(SVGFEDropShadowElement, SVGNames::dyAttr, Dy, dy)
DEFINE_ANIMATED_NUMBER_MULTIPLE_WRAPPERS(SVGFEDropShadowElement, SVGNames::stdDeviationAttr, stdDeviationXIdentifier(), StdDeviationX, stdDeviationX)
DEFINE_ANIMATED_NUMBER_MULTIPLE_WRAPPERS(SVGFEDropShadowElement, SVGNames::stdDeviationAttr, stdDeviationYIdentifier(), StdDeviationY, stdDeviationY)

BEGIN_REGISTER_ANIMATED_PROPERTIES(SVGFEDropShadowElement)
    REGISTER_LOCAL_ANIMATED_PROPERTY(in1)
    REGISTER_LOCAL_ANIMATED_PROPERTY(dx)
    REGISTER_LOCAL_ANIMATED_PROPERTY(dy)
    REGISTER_LOCAL_ANIMATED_PROPERTY(stdDeviationX)
    REGISTER_LOCAL_ANIMATED_PROPERTY(stdDeviationY)
    REGISTER_PARENT_ANIMATED_PROPERTIES(SVGFilterPrimitiveStandardAttributes)
END_REGISTER_ANIMATED_PROPERTIES

inline SVGFEDropShadowElement::SVGFEDropShadowElement(const QualifiedName& tagName, Document* document)
    : SVGFilterPrimitiveStandardAttributes(tagName, document)
    , m_dx(2)
    , m_dy(2)
    , m_stdDeviationX(2)
    , m_stdDeviationY(2)
{
    ASSERT(hasTagName(SVGNames::feDropShadowTag));
    registerAnimatedPropertiesForSVGFEDropShadowElement();
}

PassRefPtr<SVGFEDropShadowElement> SVGFEDropShadowElement::create(const QualifiedName& tagName, Document* document)
{
    return adoptRef(new SVGFEDropShadowElement(tagName, document));
}

const AtomicString& SVGFEDropShadowElement::stdDeviationXIdentifier()
{
    DEFINE_STATIC_LOCAL(AtomicString, s_identifier, ("SVGStdDeviationX", AtomicString::ConstructFromLiteral));
    return s_identifier;
}

const AtomicString& SVGFEDropShadowElement::stdDeviationYIdentifier()
{
    DEFINE_STATIC_LOCAL(AtomicString, s_identifier, ("SVGStdDeviationY", AtomicString::ConstructFromLiteral));
    return s_identifier;
}

void SVGFEDropShadowElement::setStdDeviation(float x, float y)
{
    setStdDeviationXBaseValue(x);
    setStdDeviationYBaseValue(y);
    invalidate();
}

bool SVGFEDropShadowElement::isSupportedAttribute(const QualifiedName& attrName)
{
    DEFINE_STATIC_LOCAL(HashSet<QualifiedName>, supportedAttributes, ());
    if (supportedAttributes.isEmpty()) {
        supportedAttributes.add(SVGNames::inAttr);
        supportedAttributes.add(SVGNames::dxAttr);
        supportedAttributes.add(SVGNames::dyAttr);
        supportedAttributes.add(SVGNames::stdDeviationAttr);
    }
    return supportedAttributes.contains<SVGAttributeHashTranslator>(attrName);
}

void SVGFEDropShadowElement::parseAttribute(const QualifiedName& name, const AtomicString& value)
{
    if (!isSupportedAttribute(name)) {
        SVGFilterPrimitiveStandardAttributes::parseAttribute(name, value);
        return;
    }

    if (name == SVGNames::stdDeviationAttr) {
        float x, y;
        if (parseNumberOptionalNumber(value, x, y)) {
            setStdDeviationXBaseValue(x);
            setStdDeviationYBaseValue(y);
        }
        return;
    }

    if (name == SVGNames::inAttr) {
        setIn1BaseValue(value);
        return;
    }

    if (name == SVGNames::dxAttr) {
        setDxBaseValue(value.toFloat());
        return;
    }

    if (name == SVGNames::dyAttr) {
        setDyBaseValue(value.toFloat());
        return;
    }

    ASSERT_NOT_REACHED();
}

void SVGFEDropShadowElement::svgAttributeChanged(const QualifiedName& attrName)
{
    if (!isSupportedAttribute(attrName)) {
        SVGFilterPrimitiveStandardAttributes::svgAttributeChanged(attrName);
        return;
    }

    SVGElementInstance::InvalidationGuard invalidationGuard(this);
    
    if (attrName == SVGNames::inAttr
        || attrName == SVGNames::stdDeviationAttr
        || attrName == SVGNames::dxAttr
        || attrName == SVGNames::dyAttr) {
        invalidate();
        return;
    }

    ASSERT_NOT_REACHED();
}

PassRefPtr<FilterEffect> SVGFEDropShadowElement::build(SVGFilterBuilder* filterBuilder, Filter* filter)
{
    RenderObject* renderer = this->renderer();
    if (!renderer)
        return 0;

    if (stdDeviationX() < 0 || stdDeviationY() < 0)
        return 0;

    ASSERT(renderer->style());
    const SVGRenderStyle* svgStyle = renderer->style()->svgStyle();
    
    Color color = svgStyle->floodColor();
    float opacity = svgStyle->floodOpacity();

    FilterEffect* input1 = filterBuilder->getEffectById(in1());
    if (!input1)
        return 0;

    RefPtr<FilterEffect> effect = FEDropShadow::create(filter, stdDeviationX(), stdDeviationY(), dx(), dy(), color, opacity);
    effect->inputEffects().append(input1);
    return effect.release();
}

}

#endif // ENABLE(SVG)
