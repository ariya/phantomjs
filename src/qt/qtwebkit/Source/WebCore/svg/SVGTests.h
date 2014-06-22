/*
 * Copyright (C) 2004, 2005, 2008 Nikolas Zimmermann <zimmermann@kde.org>
 * Copyright (C) 2004, 2005, 2006, 2010 Rob Buis <buis@kde.org>
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

#ifndef SVGTests_h
#define SVGTests_h

#if ENABLE(SVG)
#include "SVGAnimatedPropertyMacros.h"
#include "SVGStringList.h"

namespace WebCore {

class Attribute;
class QualifiedName;
class SVGElement;

class SVGTests {
public:
    SVGStringList& requiredFeatures();
    SVGStringList& requiredExtensions();
    SVGStringList& systemLanguage();

    bool hasExtension(const String&) const;
    bool isValid() const;

    bool parseAttribute(const QualifiedName&, const AtomicString&);
    bool isKnownAttribute(const QualifiedName&);

    void addSupportedAttributes(HashSet<QualifiedName>&);
    bool handleAttributeChange(SVGElement*, const QualifiedName&);

    static SVGAttributeToPropertyMap& attributeToPropertyMap();

protected:
    SVGTests();

    void synchronizeRequiredFeatures(SVGElement* contextElement);
    void synchronizeRequiredExtensions(SVGElement* contextElement);
    void synchronizeSystemLanguage(SVGElement* contextElement);

private:
    // Custom 'requiredFeatures' property
    static const SVGPropertyInfo* requiredFeaturesPropertyInfo();
    SVGSynchronizableAnimatedProperty<SVGStringList> m_requiredFeatures;

    // Custom 'requiredExtensions' property
    static const SVGPropertyInfo* requiredExtensionsPropertyInfo();
    SVGSynchronizableAnimatedProperty<SVGStringList> m_requiredExtensions;

    // Custom 'systemLanguage' property
    static const SVGPropertyInfo* systemLanguagePropertyInfo();
    SVGSynchronizableAnimatedProperty<SVGStringList> m_systemLanguage;
};

} // namespace WebCore

#endif // ENABLE(SVG)
#endif // SVGTests_h
