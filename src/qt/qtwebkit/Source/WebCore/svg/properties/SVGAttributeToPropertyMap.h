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

#ifndef SVGAttributeToPropertyMap_h
#define SVGAttributeToPropertyMap_h

#if ENABLE(SVG)
#include "SVGPropertyInfo.h"
#include <wtf/HashMap.h>

namespace WebCore {

class SVGAnimatedProperty;
class SVGElement;

class SVGAttributeToPropertyMap {
public:
    bool isEmpty() const { return m_map.isEmpty(); }

    void addProperties(const SVGAttributeToPropertyMap&);
    void addProperty(const SVGPropertyInfo*);

    // FIXME: To match WebKit coding style either these functions should have return values instead of out parameters,
    // or the word "get" should be added as a prefix to their names.
    void animatedPropertiesForAttribute(SVGElement* contextElement, const QualifiedName& attributeName, Vector<RefPtr<SVGAnimatedProperty> >&);
    void animatedPropertyTypeForAttribute(const QualifiedName& attributeName, Vector<AnimatedPropertyType>&);

    void synchronizeProperties(SVGElement* contextElement);
    bool synchronizeProperty(SVGElement* contextElement, const QualifiedName& attributeName);

private:
    void synchronizeProperty(SVGElement* contextElement, const QualifiedName& attributeName, const SVGPropertyInfo*);
    PassRefPtr<SVGAnimatedProperty> animatedProperty(SVGElement* contextElement, const QualifiedName& attributeName, const SVGPropertyInfo*);

    typedef Vector<const SVGPropertyInfo*> PropertiesVector;
    typedef HashMap<QualifiedName, OwnPtr<PropertiesVector> > AttributeToPropertiesMap;
    AttributeToPropertiesMap m_map;
};

}

#endif
#endif
