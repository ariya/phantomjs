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

inline SVGFEDropShadowElement::SVGFEDropShadowElement(const QualifiedName& tagName, Document* document)
    : SVGFilterPrimitiveStandardAttributes(tagName, document)
    , m_dx(2)
    , m_dy(2)
    , m_stdDeviationX(2)
    , m_stdDeviationY(2)
{
}

PassRefPtr<SVGFEDropShadowElement> SVGFEDropShadowElement::create(const QualifiedName& tagName, Document* document)
{
    return adoptRef(new SVGFEDropShadowElement(tagName, document));
}

const AtomicString& SVGFEDropShadowElement::stdDeviationXIdentifier()
{
    DEFINE_STATIC_LOCAL(AtomicString, s_identifier, ("SVGStdDeviationX"));
    return s_identifier;
}

const AtomicString& SVGFEDropShadowElement::stdDeviationYIdentifier()
{
    DEFINE_STATIC_LOCAL(AtomicString, s_identifier, ("SVGStdDeviationY"));
    return s_identifier;
}

void SVGFEDropShadowElement::setStdDeviation(float x, float y)
{
    setStdDeviationXBaseValue(x);
    setStdDeviationYBaseValue(y);
    invalidate();
}

void SVGFEDropShadowElement::parseMappedAttribute(Attribute* attr)
{
    const String& value = attr->value();
    if (attr->name() == SVGNames::stdDeviationAttr) {
        float x, y;
        if (parseNumberOptionalNumber(value, x, y)) {
            setStdDeviationXBaseValue(x);
            setStdDeviationYBaseValue(y);
        }
    } else if (attr->name() == SVGNames::inAttr)
        setIn1BaseValue(value);
    else if (attr->name() == SVGNames::dxAttr)
        setDxBaseValue(attr->value().toFloat());
    else if (attr->name() == SVGNames::dyAttr)
        setDyBaseValue(attr->value().toFloat());
    else
        SVGFilterPrimitiveStandardAttributes::parseMappedAttribute(attr);
}

void SVGFEDropShadowElement::svgAttributeChanged(const QualifiedName& attrName)
{
    SVGFilterPrimitiveStandardAttributes::svgAttributeChanged(attrName);
    
    if (attrName == SVGNames::inAttr
        || attrName == SVGNames::stdDeviationAttr
        || attrName == SVGNames::dxAttr
        || attrName == SVGNames::dyAttr)
        invalidate();
}

void SVGFEDropShadowElement::synchronizeProperty(const QualifiedName& attrName)
{
    SVGFilterPrimitiveStandardAttributes::synchronizeProperty(attrName);
    
    if (attrName == anyQName()) {
        synchronizeStdDeviationX();
        synchronizeStdDeviationY();
        synchronizeDx();
        synchronizeDy();
        synchronizeIn1();
        return;
    }
    
    if (attrName == SVGNames::stdDeviationAttr) {
        synchronizeStdDeviationX();
        synchronizeStdDeviationY();
    } else if (attrName == SVGNames::inAttr)
        synchronizeIn1();
    else if (attrName == SVGNames::dxAttr)
        synchronizeDx();
    else if (attrName == SVGNames::dyAttr)
        synchronizeDy();
}

PassRefPtr<FilterEffect> SVGFEDropShadowElement::build(SVGFilterBuilder* filterBuilder, Filter* filter)
{
    RenderObject* renderer = this->renderer();
    if (!renderer)
        return 0;
    
    ASSERT(renderer->style());
    ASSERT(renderer->style()->svgStyle());
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
