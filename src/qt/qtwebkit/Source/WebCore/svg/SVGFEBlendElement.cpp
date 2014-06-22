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
#include "SVGFEBlendElement.h"

#include "Attribute.h"
#include "FilterEffect.h"
#include "SVGElementInstance.h"
#include "SVGFilterBuilder.h"
#include "SVGNames.h"

namespace WebCore {

// Animated property definitions
DEFINE_ANIMATED_STRING(SVGFEBlendElement, SVGNames::inAttr, In1, in1)
DEFINE_ANIMATED_STRING(SVGFEBlendElement, SVGNames::in2Attr, In2, in2)
DEFINE_ANIMATED_ENUMERATION(SVGFEBlendElement, SVGNames::modeAttr, Mode, mode, BlendModeType)

BEGIN_REGISTER_ANIMATED_PROPERTIES(SVGFEBlendElement)
    REGISTER_LOCAL_ANIMATED_PROPERTY(in1)
    REGISTER_LOCAL_ANIMATED_PROPERTY(in2)
    REGISTER_LOCAL_ANIMATED_PROPERTY(mode)
    REGISTER_PARENT_ANIMATED_PROPERTIES(SVGFilterPrimitiveStandardAttributes)
END_REGISTER_ANIMATED_PROPERTIES

inline SVGFEBlendElement::SVGFEBlendElement(const QualifiedName& tagName, Document* document)
    : SVGFilterPrimitiveStandardAttributes(tagName, document)
    , m_mode(FEBLEND_MODE_NORMAL)
{
    ASSERT(hasTagName(SVGNames::feBlendTag));
    registerAnimatedPropertiesForSVGFEBlendElement();
}

PassRefPtr<SVGFEBlendElement> SVGFEBlendElement::create(const QualifiedName& tagName, Document* document)
{
    return adoptRef(new SVGFEBlendElement(tagName, document));
}

bool SVGFEBlendElement::isSupportedAttribute(const QualifiedName& attrName)
{
    DEFINE_STATIC_LOCAL(HashSet<QualifiedName>, supportedAttributes, ());
    if (supportedAttributes.isEmpty()) {
        supportedAttributes.add(SVGNames::modeAttr);
        supportedAttributes.add(SVGNames::inAttr);
        supportedAttributes.add(SVGNames::in2Attr);
    }
    return supportedAttributes.contains<SVGAttributeHashTranslator>(attrName);
}

void SVGFEBlendElement::parseAttribute(const QualifiedName& name, const AtomicString& value)
{
    if (!isSupportedAttribute(name)) {
        SVGFilterPrimitiveStandardAttributes::parseAttribute(name, value);
        return;
    }

    if (name == SVGNames::modeAttr) {
        BlendModeType propertyValue = SVGPropertyTraits<BlendModeType>::fromString(value);
        if (propertyValue > 0)
            setModeBaseValue(propertyValue);
        return;
    }

    if (name == SVGNames::inAttr) {
        setIn1BaseValue(value);
        return;
    }

    if (name == SVGNames::in2Attr) {
        setIn2BaseValue(value);
        return;
    }

    ASSERT_NOT_REACHED();
}

bool SVGFEBlendElement::setFilterEffectAttribute(FilterEffect* effect, const QualifiedName& attrName)
{
    FEBlend* blend = static_cast<FEBlend*>(effect);
    if (attrName == SVGNames::modeAttr)
        return blend->setBlendMode(mode());

    ASSERT_NOT_REACHED();
    return false;
}

void SVGFEBlendElement::svgAttributeChanged(const QualifiedName& attrName)
{
    if (!isSupportedAttribute(attrName)) {
        SVGFilterPrimitiveStandardAttributes::svgAttributeChanged(attrName);
        return;
    }

    SVGElementInstance::InvalidationGuard invalidationGuard(this);

    if (attrName == SVGNames::modeAttr) {
        primitiveAttributeChanged(attrName);
        return;
    }

    if (attrName == SVGNames::inAttr || attrName == SVGNames::in2Attr) {
        invalidate();
        return;
    }

    ASSERT_NOT_REACHED();
}

PassRefPtr<FilterEffect> SVGFEBlendElement::build(SVGFilterBuilder* filterBuilder, Filter* filter)
{
    FilterEffect* input1 = filterBuilder->getEffectById(in1());
    FilterEffect* input2 = filterBuilder->getEffectById(in2());

    if (!input1 || !input2)
        return 0;

    RefPtr<FilterEffect> effect = FEBlend::create(filter, mode());
    FilterEffectVector& inputEffects = effect->inputEffects();
    inputEffects.reserveCapacity(2);
    inputEffects.append(input1);
    inputEffects.append(input2);    
    return effect.release();
}

}

#endif // ENABLE(SVG)
