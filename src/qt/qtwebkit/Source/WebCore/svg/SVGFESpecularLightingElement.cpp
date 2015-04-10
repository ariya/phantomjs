/*
 * Copyright (C) 2004, 2005, 2007 Nikolas Zimmermann <zimmermann@kde.org>
 * Copyright (C) 2004, 2005, 2006 Rob Buis <buis@kde.org>
 * Copyright (C) 2005 Oliver Hunt <oliver@nerget.com>
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
#include "SVGFESpecularLightingElement.h"

#include "Attribute.h"
#include "FilterEffect.h"
#include "RenderStyle.h"
#include "SVGColor.h"
#include "SVGElementInstance.h"
#include "SVGFELightElement.h"
#include "SVGFilterBuilder.h"
#include "SVGNames.h"
#include "SVGParserUtilities.h"

namespace WebCore {

// Animated property definitions
DEFINE_ANIMATED_STRING(SVGFESpecularLightingElement, SVGNames::inAttr, In1, in1)
DEFINE_ANIMATED_NUMBER(SVGFESpecularLightingElement, SVGNames::specularConstantAttr, SpecularConstant, specularConstant)
DEFINE_ANIMATED_NUMBER(SVGFESpecularLightingElement, SVGNames::specularExponentAttr, SpecularExponent, specularExponent)
DEFINE_ANIMATED_NUMBER(SVGFESpecularLightingElement, SVGNames::surfaceScaleAttr, SurfaceScale, surfaceScale)
DEFINE_ANIMATED_NUMBER_MULTIPLE_WRAPPERS(SVGFESpecularLightingElement, SVGNames::kernelUnitLengthAttr, kernelUnitLengthXIdentifier(), KernelUnitLengthX, kernelUnitLengthX)
DEFINE_ANIMATED_NUMBER_MULTIPLE_WRAPPERS(SVGFESpecularLightingElement, SVGNames::kernelUnitLengthAttr, kernelUnitLengthYIdentifier(), KernelUnitLengthY, kernelUnitLengthY)

BEGIN_REGISTER_ANIMATED_PROPERTIES(SVGFESpecularLightingElement)
    REGISTER_LOCAL_ANIMATED_PROPERTY(in1)
    REGISTER_LOCAL_ANIMATED_PROPERTY(specularConstant)
    REGISTER_LOCAL_ANIMATED_PROPERTY(specularExponent)
    REGISTER_LOCAL_ANIMATED_PROPERTY(surfaceScale)
    REGISTER_LOCAL_ANIMATED_PROPERTY(kernelUnitLengthX)
    REGISTER_LOCAL_ANIMATED_PROPERTY(kernelUnitLengthY)
    REGISTER_PARENT_ANIMATED_PROPERTIES(SVGFilterPrimitiveStandardAttributes)
END_REGISTER_ANIMATED_PROPERTIES

inline SVGFESpecularLightingElement::SVGFESpecularLightingElement(const QualifiedName& tagName, Document* document)
    : SVGFilterPrimitiveStandardAttributes(tagName, document)
    , m_specularConstant(1)
    , m_specularExponent(1)
    , m_surfaceScale(1)
{
    ASSERT(hasTagName(SVGNames::feSpecularLightingTag));
    registerAnimatedPropertiesForSVGFESpecularLightingElement();
}

PassRefPtr<SVGFESpecularLightingElement> SVGFESpecularLightingElement::create(const QualifiedName& tagName, Document* document)
{
    return adoptRef(new SVGFESpecularLightingElement(tagName, document));
}

const AtomicString& SVGFESpecularLightingElement::kernelUnitLengthXIdentifier()
{
    DEFINE_STATIC_LOCAL(AtomicString, s_identifier, ("SVGKernelUnitLengthX", AtomicString::ConstructFromLiteral));
    return s_identifier;
}

const AtomicString& SVGFESpecularLightingElement::kernelUnitLengthYIdentifier()
{
    DEFINE_STATIC_LOCAL(AtomicString, s_identifier, ("SVGKernelUnitLengthY", AtomicString::ConstructFromLiteral));
    return s_identifier;
}

bool SVGFESpecularLightingElement::isSupportedAttribute(const QualifiedName& attrName)
{
    DEFINE_STATIC_LOCAL(HashSet<QualifiedName>, supportedAttributes, ());
    if (supportedAttributes.isEmpty()) {
        supportedAttributes.add(SVGNames::inAttr);
        supportedAttributes.add(SVGNames::specularConstantAttr);
        supportedAttributes.add(SVGNames::specularExponentAttr);
        supportedAttributes.add(SVGNames::surfaceScaleAttr);
        supportedAttributes.add(SVGNames::kernelUnitLengthAttr);
    }
    return supportedAttributes.contains<SVGAttributeHashTranslator>(attrName);
}

void SVGFESpecularLightingElement::parseAttribute(const QualifiedName& name, const AtomicString& value)
{
    if (!isSupportedAttribute(name)) {
        SVGFilterPrimitiveStandardAttributes::parseAttribute(name, value);
        return;
    }

    if (name == SVGNames::inAttr) {
        setIn1BaseValue(value);
        return;
    }

    if (name == SVGNames::surfaceScaleAttr) {
        setSurfaceScaleBaseValue(value.toFloat());
        return;
    }

    if (name == SVGNames::specularConstantAttr) {
        setSpecularConstantBaseValue(value.toFloat());
        return;
    }

    if (name == SVGNames::specularExponentAttr) {
        setSpecularExponentBaseValue(value.toFloat());
        return;
    }

    if (name == SVGNames::kernelUnitLengthAttr) {
        float x, y;
        if (parseNumberOptionalNumber(value, x, y)) {
            setKernelUnitLengthXBaseValue(x);
            setKernelUnitLengthYBaseValue(y);
        }
        return;
    }

    ASSERT_NOT_REACHED();
}

bool SVGFESpecularLightingElement::setFilterEffectAttribute(FilterEffect* effect, const QualifiedName& attrName)
{
    FESpecularLighting* specularLighting = static_cast<FESpecularLighting*>(effect);

    if (attrName == SVGNames::lighting_colorAttr) {
        RenderObject* renderer = this->renderer();
        ASSERT(renderer);
        ASSERT(renderer->style());
        return specularLighting->setLightingColor(renderer->style()->svgStyle()->lightingColor());
    }
    if (attrName == SVGNames::surfaceScaleAttr)
        return specularLighting->setSurfaceScale(surfaceScale());
    if (attrName == SVGNames::specularConstantAttr)
        return specularLighting->setSpecularConstant(specularConstant());
    if (attrName == SVGNames::specularExponentAttr)
        return specularLighting->setSpecularExponent(specularExponent());

    LightSource* lightSource = const_cast<LightSource*>(specularLighting->lightSource());
    const SVGFELightElement* lightElement = SVGFELightElement::findLightElement(this);
    ASSERT(lightSource);
    ASSERT(lightElement);

    if (attrName == SVGNames::azimuthAttr)
        return lightSource->setAzimuth(lightElement->azimuth());
    if (attrName == SVGNames::elevationAttr)
        return lightSource->setElevation(lightElement->elevation());
    if (attrName == SVGNames::xAttr)
        return lightSource->setX(lightElement->x());
    if (attrName == SVGNames::yAttr)
        return lightSource->setY(lightElement->y());
    if (attrName == SVGNames::zAttr)
        return lightSource->setZ(lightElement->z());
    if (attrName == SVGNames::pointsAtXAttr)
        return lightSource->setPointsAtX(lightElement->pointsAtX());
    if (attrName == SVGNames::pointsAtYAttr)
        return lightSource->setPointsAtY(lightElement->pointsAtY());
    if (attrName == SVGNames::pointsAtZAttr)
        return lightSource->setPointsAtZ(lightElement->pointsAtZ());
    if (attrName == SVGNames::specularExponentAttr)
        return lightSource->setSpecularExponent(lightElement->specularExponent());
    if (attrName == SVGNames::limitingConeAngleAttr)
        return lightSource->setLimitingConeAngle(lightElement->limitingConeAngle());

    ASSERT_NOT_REACHED();
    return false;
}

void SVGFESpecularLightingElement::svgAttributeChanged(const QualifiedName& attrName)
{
    if (!isSupportedAttribute(attrName)) {
        SVGFilterPrimitiveStandardAttributes::svgAttributeChanged(attrName);
        return;
    }

    SVGElementInstance::InvalidationGuard invalidationGuard(this);
    
    if (attrName == SVGNames::surfaceScaleAttr
        || attrName == SVGNames::specularConstantAttr
        || attrName == SVGNames::specularExponentAttr
        || attrName == SVGNames::kernelUnitLengthAttr) {
        primitiveAttributeChanged(attrName);
        return;
    }

    if (attrName == SVGNames::inAttr) {
        invalidate();
        return;
    }

    ASSERT_NOT_REACHED();
}

void SVGFESpecularLightingElement::lightElementAttributeChanged(const SVGFELightElement* lightElement, const QualifiedName& attrName)
{
    if (SVGFELightElement::findLightElement(this) != lightElement)
        return;

    // The light element has different attribute names so attrName can identify the requested attribute.
    primitiveAttributeChanged(attrName);
}

PassRefPtr<FilterEffect> SVGFESpecularLightingElement::build(SVGFilterBuilder* filterBuilder, Filter* filter)
{
    FilterEffect* input1 = filterBuilder->getEffectById(in1());

    if (!input1)
        return 0;

    RefPtr<LightSource> lightSource = SVGFELightElement::findLightSource(this);
    if (!lightSource)
        return 0;

    RenderObject* renderer = this->renderer();
    if (!renderer)
        return 0;
    
    ASSERT(renderer->style());
    Color color = renderer->style()->svgStyle()->lightingColor();

    RefPtr<FilterEffect> effect = FESpecularLighting::create(filter, color, surfaceScale(), specularConstant(),
                                          specularExponent(), kernelUnitLengthX(), kernelUnitLengthY(), lightSource.release());
    effect->inputEffects().append(input1);
    return effect.release();
}

}

#endif // ENABLE(SVG)
