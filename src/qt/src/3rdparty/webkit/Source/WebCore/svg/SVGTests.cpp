/*
 * Copyright (C) 2004, 2005, 2008 Nikolas Zimmermann <zimmermann@kde.org>
 * Copyright (C) 2004, 2005, 2006, 2007 Rob Buis <buis@kde.org>
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

#if ENABLE(SVG)
#include "SVGTests.h"

#include "Attribute.h"
#include "DOMImplementation.h"
#include "Language.h"
#include "SVGElement.h"
#include "SVGNames.h"
#include "SVGStringList.h"

namespace WebCore {

SVGTests::SVGTests()
    : m_requiredFeatures(SVGNames::requiredFeaturesAttr)
    , m_requiredExtensions(SVGNames::requiredExtensionsAttr)
    , m_systemLanguage(SVGNames::systemLanguageAttr)
{
}

bool SVGTests::hasExtension(const String&) const
{
    // FIXME: Implement me!
    return false;
}

bool SVGTests::isValid() const
{
    unsigned featuresSize = m_requiredFeatures.value.size();
    for (unsigned i = 0; i < featuresSize; ++i) {
        String value = m_requiredFeatures.value.at(i);
        if (value.isEmpty() || !DOMImplementation::hasFeature(value, String()))
            return false;
    }

    unsigned systemLanguageSize = m_systemLanguage.value.size();
    for (unsigned i = 0; i < systemLanguageSize; ++i) {
        String value = m_systemLanguage.value.at(i);
        if (value != defaultLanguage().substring(0, 2))
            return false;
    }

    if (!m_requiredExtensions.value.isEmpty())
        return false;

    return true;
}

bool SVGTests::parseMappedAttribute(Attribute* attr)
{
    if (attr->name() == SVGNames::requiredFeaturesAttr) {
        m_requiredFeatures.value.reset(attr->value());
        return true;
    }
    if (attr->name() == SVGNames::requiredExtensionsAttr) {
        m_requiredExtensions.value.reset(attr->value());
        return true;
    }
    if (attr->name() == SVGNames::systemLanguageAttr) {
        m_systemLanguage.value.reset(attr->value());
        return true;
    }
    
    return false;
}

bool SVGTests::isKnownAttribute(const QualifiedName& attrName)
{
    return attrName == SVGNames::requiredFeaturesAttr
        || attrName == SVGNames::requiredExtensionsAttr
        || attrName == SVGNames::systemLanguageAttr;
}

bool SVGTests::handleAttributeChange(const SVGElement* targetElement, const QualifiedName& attrName)
{
    if (!isKnownAttribute(attrName))
        return false;
    if (!targetElement->inDocument())
        return false;
    SVGElement* svgElement = const_cast<SVGElement*>(targetElement);
    ASSERT(svgElement);
    bool valid = svgElement->isValid();
    if (valid && !svgElement->attached())
        svgElement->attach();
    if (!valid && svgElement->attached())
        svgElement->detach();
    return true;
}

void SVGTests::synchronizeProperties(SVGElement* contextElement, const QualifiedName& attrName)
{
    if (attrName == anyQName()) {
        synchronizeRequiredFeatures(contextElement);
        synchronizeRequiredExtensions(contextElement);
        synchronizeSystemLanguage(contextElement);
        return;
    }

    if (attrName == SVGNames::requiredFeaturesAttr)
        synchronizeRequiredFeatures(contextElement);
    else if (attrName == SVGNames::requiredExtensionsAttr)
        synchronizeRequiredExtensions(contextElement);
    else if (attrName == SVGNames::systemLanguageAttr)
        synchronizeSystemLanguage(contextElement);
}

void SVGTests::synchronizeRequiredFeatures(SVGElement* contextElement)
{
    if (!m_requiredFeatures.shouldSynchronize)
        return;
    AtomicString value(m_requiredFeatures.value.valueAsString());
    SVGAnimatedPropertySynchronizer<true>::synchronize(contextElement, SVGNames::requiredFeaturesAttr, value);
}

void SVGTests::synchronizeRequiredExtensions(SVGElement* contextElement)
{
    if (!m_requiredExtensions.shouldSynchronize)
        return;
    AtomicString value(m_requiredExtensions.value.valueAsString());
    SVGAnimatedPropertySynchronizer<true>::synchronize(contextElement, SVGNames::requiredExtensionsAttr, value);
}

void SVGTests::synchronizeSystemLanguage(SVGElement* contextElement)
{
    if (!m_systemLanguage.shouldSynchronize)
        return;
    AtomicString value(m_systemLanguage.value.valueAsString());
    SVGAnimatedPropertySynchronizer<true>::synchronize(contextElement, SVGNames::systemLanguageAttr, value);
}

SVGStringList& SVGTests::requiredFeatures()
{
    m_requiredFeatures.shouldSynchronize = true;
    return m_requiredFeatures.value;
}

SVGStringList& SVGTests::requiredExtensions()
{
    m_requiredExtensions.shouldSynchronize = true;    
    return m_requiredExtensions.value;
}

SVGStringList& SVGTests::systemLanguage()
{
    m_systemLanguage.shouldSynchronize = true;
    return m_systemLanguage.value;
}

}

#endif // ENABLE(SVG)
