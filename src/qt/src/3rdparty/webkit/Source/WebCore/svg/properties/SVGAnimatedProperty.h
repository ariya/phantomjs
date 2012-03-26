/*
 * Copyright (C) Research In Motion Limited 2010. All rights reserved.
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

#ifndef SVGAnimatedProperty_h
#define SVGAnimatedProperty_h

#if ENABLE(SVG)
#include "QualifiedName.h"
#include "SVGAnimatedPropertyDescription.h"
#include "SVGElement.h"
#include <wtf/RefCounted.h>

namespace WebCore {

class SVGElement;
class SVGProperty;

class SVGAnimatedProperty : public RefCounted<SVGAnimatedProperty> {
public:
    SVGElement* contextElement() const { return m_contextElement.get(); }
    const QualifiedName& attributeName() const { return m_attributeName; }

    void commitChange()
    {
        ASSERT(m_contextElement);
        m_contextElement->invalidateSVGAttributes();
        m_contextElement->svgAttributeChanged(m_attributeName);
    }

    virtual bool isAnimatedListTearOff() const { return false; }

    // Caching facilities.
    typedef HashMap<SVGAnimatedPropertyDescription, RefPtr<SVGAnimatedProperty>, SVGAnimatedPropertyDescriptionHash, SVGAnimatedPropertyDescriptionHashTraits> Cache;

    virtual ~SVGAnimatedProperty()
    {
        // Remove wrapper from cache.
        Cache* cache = animatedPropertyCache();
        const Cache::const_iterator end = cache->end();
        for (Cache::const_iterator it = cache->begin(); it != end; ++it) {
            if (it->second == this) {
                cache->remove(it->first);
                break;
            }
        }
    }

    template<typename TearOffType, typename PropertyType>
    static PassRefPtr<TearOffType> lookupOrCreateWrapper(SVGElement* element, const QualifiedName& attributeName, const AtomicString& attributeIdentifier, PropertyType& property)
    {
        SVGAnimatedPropertyDescription key(element, attributeIdentifier);
        RefPtr<SVGAnimatedProperty> wrapper = animatedPropertyCache()->get(key);
        if (!wrapper) {
            wrapper = TearOffType::create(element, attributeName, property);
            animatedPropertyCache()->set(key, wrapper);
        }

        return static_pointer_cast<TearOffType>(wrapper).release();
    }

    template<typename TearOffType>
    static TearOffType* lookupWrapper(SVGElement* element, const AtomicString& attributeIdentifier)
    {
        SVGAnimatedPropertyDescription key(element, attributeIdentifier);
        return static_pointer_cast<TearOffType>(animatedPropertyCache()->get(key)).get();
    }

protected:
    SVGAnimatedProperty(SVGElement* contextElement, const QualifiedName& attributeName)
        : m_contextElement(contextElement)
        , m_attributeName(attributeName)
    {
    }

private:
    static Cache* animatedPropertyCache()
    {
        static Cache* s_cache = new Cache;                
        return s_cache;
    }

    RefPtr<SVGElement> m_contextElement;
    const QualifiedName& m_attributeName;
};

}

#endif // ENABLE(SVG)
#endif // SVGAnimatedProperty_h
