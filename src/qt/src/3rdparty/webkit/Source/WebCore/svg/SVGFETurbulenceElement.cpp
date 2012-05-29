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
#include "SVGNames.h"
#include "SVGParserUtilities.h"

namespace WebCore {

// Animated property definitions
DEFINE_ANIMATED_NUMBER_MULTIPLE_WRAPPERS(SVGFETurbulenceElement, SVGNames::baseFrequencyAttr, baseFrequencyXIdentifier(), BaseFrequencyX, baseFrequencyX)
DEFINE_ANIMATED_NUMBER_MULTIPLE_WRAPPERS(SVGFETurbulenceElement, SVGNames::baseFrequencyAttr, baseFrequencyYIdentifier(), BaseFrequencyY, baseFrequencyY)
DEFINE_ANIMATED_INTEGER(SVGFETurbulenceElement, SVGNames::numOctavesAttr, NumOctaves, numOctaves)
DEFINE_ANIMATED_NUMBER(SVGFETurbulenceElement, SVGNames::seedAttr, Seed, seed)
DEFINE_ANIMATED_ENUMERATION(SVGFETurbulenceElement, SVGNames::stitchTilesAttr, StitchTiles, stitchTiles)
DEFINE_ANIMATED_ENUMERATION(SVGFETurbulenceElement, SVGNames::typeAttr, Type, type)

inline SVGFETurbulenceElement::SVGFETurbulenceElement(const QualifiedName& tagName, Document* document)
    : SVGFilterPrimitiveStandardAttributes(tagName, document)
    , m_numOctaves(1)
    , m_stitchTiles(SVG_STITCHTYPE_NOSTITCH)
    , m_type(FETURBULENCE_TYPE_TURBULENCE)
{
}

PassRefPtr<SVGFETurbulenceElement> SVGFETurbulenceElement::create(const QualifiedName& tagName, Document* document)
{
    return adoptRef(new SVGFETurbulenceElement(tagName, document));
}

const AtomicString& SVGFETurbulenceElement::baseFrequencyXIdentifier()
{
    DEFINE_STATIC_LOCAL(AtomicString, s_identifier, ("SVGBaseFrequencyX"));
    return s_identifier;
}

const AtomicString& SVGFETurbulenceElement::baseFrequencyYIdentifier()
{
    DEFINE_STATIC_LOCAL(AtomicString, s_identifier, ("SVGBaseFrequencyY"));
    return s_identifier;
}

void SVGFETurbulenceElement::parseMappedAttribute(Attribute* attr)
{
    const String& value = attr->value();
    if (attr->name() == SVGNames::typeAttr) {
        if (value == "fractalNoise")
            setTypeBaseValue(FETURBULENCE_TYPE_FRACTALNOISE);
        else if (value == "turbulence")
            setTypeBaseValue(FETURBULENCE_TYPE_TURBULENCE);
    } else if (attr->name() == SVGNames::stitchTilesAttr) {
        if (value == "stitch")
            setStitchTilesBaseValue(SVG_STITCHTYPE_STITCH);
        else if (value == "noStitch")
            setStitchTilesBaseValue(SVG_STITCHTYPE_NOSTITCH);
    } else if (attr->name() == SVGNames::baseFrequencyAttr) {
        float x, y;
        if (parseNumberOptionalNumber(value, x, y)) {
            setBaseFrequencyXBaseValue(x);
            setBaseFrequencyYBaseValue(y);
        }
    } else if (attr->name() == SVGNames::seedAttr)
        setSeedBaseValue(value.toFloat());
    else if (attr->name() == SVGNames::numOctavesAttr)
        setNumOctavesBaseValue(value.toUIntStrict());
    else
        SVGFilterPrimitiveStandardAttributes::parseMappedAttribute(attr);
}

bool SVGFETurbulenceElement::setFilterEffectAttribute(FilterEffect* effect, const QualifiedName& attrName)
{
    FETurbulence* turbulence = static_cast<FETurbulence*>(effect);
    if (attrName == SVGNames::typeAttr)
        return turbulence->setType(static_cast<TurbulenceType>(type()));
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
    SVGFilterPrimitiveStandardAttributes::svgAttributeChanged(attrName);

    if (attrName == SVGNames::baseFrequencyAttr
        || attrName == SVGNames::numOctavesAttr
        || attrName == SVGNames::seedAttr
        || attrName == SVGNames::stitchTilesAttr
        || attrName == SVGNames::typeAttr)
        primitiveAttributeChanged(attrName);
}

void SVGFETurbulenceElement::synchronizeProperty(const QualifiedName& attrName)
{
    SVGFilterPrimitiveStandardAttributes::synchronizeProperty(attrName);

    if (attrName == anyQName()) {
        synchronizeType();
        synchronizeStitchTiles();
        synchronizeBaseFrequencyX();
        synchronizeBaseFrequencyY();
        synchronizeSeed();
        synchronizeNumOctaves();
        return;
    }

    if (attrName == SVGNames::typeAttr)
        synchronizeType();
    else if (attrName == SVGNames::stitchTilesAttr)
        synchronizeStitchTiles();
    else if (attrName == SVGNames::baseFrequencyAttr) {
        synchronizeBaseFrequencyX();
        synchronizeBaseFrequencyY();
    } else if (attrName == SVGNames::seedAttr)
        synchronizeSeed();
    else if (attrName == SVGNames::numOctavesAttr)
        synchronizeNumOctaves();
}

AttributeToPropertyTypeMap& SVGFETurbulenceElement::attributeToPropertyTypeMap()
{
    DEFINE_STATIC_LOCAL(AttributeToPropertyTypeMap, s_attributeToPropertyTypeMap, ());
    return s_attributeToPropertyTypeMap;
}

void SVGFETurbulenceElement::fillAttributeToPropertyTypeMap()
{
    AttributeToPropertyTypeMap& attributeToPropertyTypeMap = this->attributeToPropertyTypeMap();

    SVGFilterPrimitiveStandardAttributes::fillPassedAttributeToPropertyTypeMap(attributeToPropertyTypeMap);
    attributeToPropertyTypeMap.set(SVGNames::baseFrequencyAttr, AnimatedNumberOptionalNumber);
    attributeToPropertyTypeMap.set(SVGNames::numOctavesAttr, AnimatedInteger);
    attributeToPropertyTypeMap.set(SVGNames::seedAttr, AnimatedNumber);
    attributeToPropertyTypeMap.set(SVGNames::stitchTilesAttr, AnimatedEnumeration);
    attributeToPropertyTypeMap.set(SVGNames::typeAttr, AnimatedEnumeration);
}

PassRefPtr<FilterEffect> SVGFETurbulenceElement::build(SVGFilterBuilder*, Filter* filter)
{
    if (baseFrequencyX() < 0 || baseFrequencyY() < 0)
        return 0;

    return FETurbulence::create(filter, static_cast<TurbulenceType>(type()), baseFrequencyX(), 
                baseFrequencyY(), numOctaves(), seed(), stitchTiles() == SVG_STITCHTYPE_STITCH);
}

}

#endif // ENABLE(SVG)
