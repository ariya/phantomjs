/*
 * Copyright (C) 2004, 2005, 2007 Nikolas Zimmermann <zimmermann@kde.org>
 * Copyright (C) 2004, 2005, 2006 Rob Buis <buis@kde.org>
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
#include "SVGFETurbulenceElement.h"

#include "Attribute.h"
#include "SVGElementInstance.h"
#include "SVGNames.h"
#include "SVGParserUtilities.h"

namespace WebCore {

// Animated property definitions
DEFINE_ANIMATED_NUMBER_MULTIPLE_WRAPPERS(SVGFETurbulenceElement, SVGNames::baseFrequencyAttr, baseFrequencyXIdentifier(), BaseFrequencyX, baseFrequencyX)
DEFINE_ANIMATED_NUMBER_MULTIPLE_WRAPPERS(SVGFETurbulenceElement, SVGNames::baseFrequencyAttr, baseFrequencyYIdentifier(), BaseFrequencyY, baseFrequencyY)
DEFINE_ANIMATED_INTEGER(SVGFETurbulenceElement, SVGNames::numOctavesAttr, NumOctaves, numOctaves)
DEFINE_ANIMATED_NUMBER(SVGFETurbulenceElement, SVGNames::seedAttr, Seed, seed)
DEFINE_ANIMATED_ENUMERATION(SVGFETurbulenceElement, SVGNames::stitchTilesAttr, StitchTiles, stitchTiles, SVGStitchOptions)
DEFINE_ANIMATED_ENUMERATION(SVGFETurbulenceElement, SVGNames::typeAttr, Type, type, TurbulenceType)

BEGIN_REGISTER_ANIMATED_PROPERTIES(SVGFETurbulenceElement)
    REGISTER_LOCAL_ANIMATED_PROPERTY(baseFrequencyX)
    REGISTER_LOCAL_ANIMATED_PROPERTY(baseFrequencyY)
    REGISTER_LOCAL_ANIMATED_PROPERTY(numOctaves)
    REGISTER_LOCAL_ANIMATED_PROPERTY(seed)
    REGISTER_LOCAL_ANIMATED_PROPERTY(stitchTiles)
    REGISTER_LOCAL_ANIMATED_PROPERTY(type)
    REGISTER_PARENT_ANIMATED_PROPERTIES(SVGFilterPrimitiveStandardAttributes)
END_REGISTER_ANIMATED_PROPERTIES

inline SVGFETurbulenceElement::SVGFETurbulenceElement(const QualifiedName& tagName, Document* document)
    : SVGFilterPrimitiveStandardAttributes(tagName, document)
    , m_numOctaves(1)
    , m_stitchTiles(SVG_STITCHTYPE_NOSTITCH)
    , m_type(FETURBULENCE_TYPE_TURBULENCE)
{
    ASSERT(hasTagName(SVGNames::feTurbulenceTag));
    registerAnimatedPropertiesForSVGFETurbulenceElement();
}

PassRefPtr<SVGFETurbulenceElement> SVGFETurbulenceElement::create(const QualifiedName& tagName, Document* document)
{
    return adoptRef(new SVGFETurbulenceElement(tagName, document));
}

const AtomicString& SVGFETurbulenceElement::baseFrequencyXIdentifier()
{
    DEFINE_STATIC_LOCAL(AtomicString, s_identifier, ("SVGBaseFrequencyX", AtomicString::ConstructFromLiteral));
    return s_identifier;
}

const AtomicString& SVGFETurbulenceElement::baseFrequencyYIdentifier()
{
    DEFINE_STATIC_LOCAL(AtomicString, s_identifier, ("SVGBaseFrequencyY", AtomicString::ConstructFromLiteral));
    return s_identifier;
}

bool SVGFETurbulenceElement::isSupportedAttribute(const QualifiedName& attrName)
{
    DEFINE_STATIC_LOCAL(HashSet<QualifiedName>, supportedAttributes, ());
    if (supportedAttributes.isEmpty()) {
        supportedAttributes.add(SVGNames::baseFrequencyAttr);
        supportedAttributes.add(SVGNames::numOctavesAttr);
        supportedAttributes.add(SVGNames::seedAttr);
        supportedAttributes.add(SVGNames::stitchTilesAttr);
        supportedAttributes.add(SVGNames::typeAttr);
    }
    return supportedAttributes.contains<SVGAttributeHashTranslator>(attrName);
}

void SVGFETurbulenceElement::parseAttribute(const QualifiedName& name, const AtomicString& value)
{
    if (!isSupportedAttribute(name)) {
        SVGFilterPrimitiveStandardAttributes::parseAttribute(name, value);
        return;
    }

    if (name == SVGNames::typeAttr) {
        TurbulenceType propertyValue = SVGPropertyTraits<TurbulenceType>::fromString(value);
        if (propertyValue > 0)
            setTypeBaseValue(propertyValue);
        return;
    }

    if (name == SVGNames::stitchTilesAttr) {
        SVGStitchOptions propertyValue = SVGPropertyTraits<SVGStitchOptions>::fromString(value);
        if (propertyValue > 0)
            setStitchTilesBaseValue(propertyValue);
        return;
    }

    if (name == SVGNames::baseFrequencyAttr) {
        float x, y;
        if (parseNumberOptionalNumber(value, x, y)) {
            setBaseFrequencyXBaseValue(x);
            setBaseFrequencyYBaseValue(y);
        }
        return;
    }

    if (name == SVGNames::seedAttr) {
        setSeedBaseValue(value.toFloat());
        return;
    }

    if (name == SVGNames::numOctavesAttr) {
        setNumOctavesBaseValue(value.string().toUIntStrict());
        return;
    }

    ASSERT_NOT_REACHED();
}

bool SVGFETurbulenceElement::setFilterEffectAttribute(FilterEffect* effect, const QualifiedName& attrName)
{
    FETurbulence* turbulence = static_cast<FETurbulence*>(effect);
    if (attrName == SVGNames::typeAttr)
        return turbulence->setType(type());
    if (attrName == SVGNames::stitchTilesAttr)
        return turbulence->setStitchTiles(stitchTiles());
    if (attrName == SVGNames::baseFrequencyAttr)
        return (turbulence->setBaseFrequencyX(baseFrequencyX()) || turbulence->setBaseFrequencyY(baseFrequencyY()));
    if (attrName == SVGNames::seedAttr)
        return turbulence->setSeed(seed());
    if (attrName == SVGNames::numOctavesAttr)
       return turbulence->setNumOctaves(numOctaves());

    ASSERT_NOT_REACHED();
    return false;
}

void SVGFETurbulenceElement::svgAttributeChanged(const QualifiedName& attrName)
{
    if (!isSupportedAttribute(attrName)) {
        SVGFilterPrimitiveStandardAttributes::svgAttributeChanged(attrName);
        return;
    }

    SVGElementInstance::InvalidationGuard invalidationGuard(this);
    
    if (attrName == SVGNames::baseFrequencyAttr
        || attrName == SVGNames::numOctavesAttr
        || attrName == SVGNames::seedAttr
        || attrName == SVGNames::stitchTilesAttr
        || attrName == SVGNames::typeAttr) {
        primitiveAttributeChanged(attrName);
        return;
    }

    ASSERT_NOT_REACHED();
}

PassRefPtr<FilterEffect> SVGFETurbulenceElement::build(SVGFilterBuilder*, Filter* filter)
{
    if (baseFrequencyX() < 0 || baseFrequencyY() < 0)
        return 0;
    return FETurbulence::create(filter, type(), baseFrequencyX(), baseFrequencyY(), numOctaves(), seed(), stitchTiles() == SVG_STITCHTYPE_STITCH);
}

}

#endif // ENABLE(SVG)
