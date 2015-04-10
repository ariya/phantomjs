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

#ifndef SVGAnimatedListPropertyTearOff_h
#define SVGAnimatedListPropertyTearOff_h

#if ENABLE(SVG)
#include "SVGAnimatedProperty.h"
#include "SVGListPropertyTearOff.h"
#include "SVGStaticListPropertyTearOff.h"

namespace WebCore {

template<typename PropertyType>
class SVGPropertyTearOff;

template<typename PropertyType>
class SVGAnimatedListPropertyTearOff : public SVGAnimatedProperty {
public:
    typedef typename SVGPropertyTraits<PropertyType>::ListItemType ListItemType;
    typedef SVGPropertyTearOff<ListItemType> ListItemTearOff;
    typedef Vector<RefPtr<ListItemTearOff> > ListWrapperCache;
    typedef SVGListProperty<PropertyType> ListProperty;
    typedef SVGListPropertyTearOff<PropertyType> ListPropertyTearOff;
    typedef PropertyType ContentType;

    virtual ListProperty* baseVal()
    {
        if (!m_baseVal)
            m_baseVal = ListPropertyTearOff::create(this, BaseValRole, m_values, m_wrappers);
        return static_cast<ListProperty*>(m_baseVal.get());
    }

    virtual ListProperty* animVal()
    {
        if (!m_animVal)
            m_animVal = ListPropertyTearOff::create(this, AnimValRole, m_values, m_wrappers);
        return static_cast<ListProperty*>(m_animVal.get());
    }

    virtual bool isAnimatedListTearOff() const { return true; }

    int findItem(SVGProperty* property) const
    {
        // This should ever be called for our baseVal, as animVal can't modify the list.
        // It's safe to cast to ListPropertyTearOff here as all classes inheriting from us supply their own removeItemFromList() method.
        typedef SVGPropertyTearOff<typename SVGPropertyTraits<PropertyType>::ListItemType> ListItemTearOff;
        return static_cast<ListPropertyTearOff*>(m_baseVal.get())->findItem(static_cast<ListItemTearOff*>(property));
    }

    void removeItemFromList(size_t itemIndex, bool shouldSynchronizeWrappers)
    {
        // This should ever be called for our baseVal, as animVal can't modify the list.
        // It's safe to cast to ListPropertyTearOff here as all classes inheriting from us supply their own removeItemFromList() method.
        static_cast<ListPropertyTearOff*>(m_baseVal.get())->removeItemFromList(itemIndex, shouldSynchronizeWrappers);
    }

    void detachListWrappers(unsigned newListSize)
    {
        ListProperty::detachListWrappersAndResize(&m_wrappers, newListSize);
    }

    PropertyType& currentAnimatedValue()
    {
        ASSERT(m_isAnimating);
        ASSERT(m_animVal);
        return static_cast<ListProperty*>(m_animVal.get())->values();
    }

    const PropertyType& currentBaseValue() const
    {
        return m_values;
    }

    void animationStarted(PropertyType* newAnimVal, bool shouldOwnValues = false)
    {
        ASSERT(!m_isAnimating);
        ASSERT(newAnimVal);
        ASSERT(m_values.size() == m_wrappers.size());
        ASSERT(m_animatedWrappers.isEmpty());

        // Switch to new passed in value type & new wrappers list. The new wrappers list must be created for the new value.
        if (!newAnimVal->isEmpty())
            m_animatedWrappers.fill(0, newAnimVal->size());

        ListProperty* animVal = static_cast<ListProperty*>(this->animVal());
        animVal->setValuesAndWrappers(newAnimVal, &m_animatedWrappers, shouldOwnValues);
        ASSERT(animVal->values().size() == animVal->wrappers().size());
        ASSERT(animVal->wrappers().size() == m_animatedWrappers.size());
        m_isAnimating = true;
    }

    void animationEnded()
    {
        ASSERT(m_isAnimating);
        ASSERT(m_animVal);
        ASSERT(m_values.size() == m_wrappers.size());

        ListProperty* animVal = static_cast<ListProperty*>(m_animVal.get());
        ASSERT(animVal->values().size() == animVal->wrappers().size());
        ASSERT(animVal->wrappers().size() == m_animatedWrappers.size());

        animVal->setValuesAndWrappers(&m_values, &m_wrappers, false);
        ASSERT(animVal->values().size() == animVal->wrappers().size());
        ASSERT(animVal->wrappers().size() == m_wrappers.size());

        m_animatedWrappers.clear();
        m_isAnimating = false;
    }

    void synchronizeWrappersIfNeeded()
    {
        // Eventually the wrapper list needs synchronization because any SVGAnimateLengthList::calculateAnimatedValue() call may
        // mutate the length of our values() list, and thus the wrapper() cache needs synchronization, to have the same size.
        // Also existing wrappers which point directly at elements in the existing SVGLengthList have to be detached (so a copy
        // of them is created, so existing animVal variables in JS are kept-alive). If we'd detach them later the underlying
        // SVGLengthList was already mutated, and our list item wrapper tear offs would point nowhere. Assertions would fire.
        ListProperty* animVal = static_cast<ListProperty*>(m_animVal.get());
        animVal->detachListWrappers(animVal->values().size());

        ASSERT(animVal->values().size() == animVal->wrappers().size());
        ASSERT(animVal->wrappers().size() == m_animatedWrappers.size());
    }

    void animValWillChange()
    {
        ASSERT(m_isAnimating);
        ASSERT(m_animVal);
        ASSERT(m_values.size() == m_wrappers.size());
        synchronizeWrappersIfNeeded();
    }

    void animValDidChange()
    {
        ASSERT(m_isAnimating);
        ASSERT(m_animVal);
        ASSERT(m_values.size() == m_wrappers.size());
        synchronizeWrappersIfNeeded();
    }

    static PassRefPtr<SVGAnimatedListPropertyTearOff<PropertyType> > create(SVGElement* contextElement, const QualifiedName& attributeName, AnimatedPropertyType animatedPropertyType, PropertyType& values)
    {
        ASSERT(contextElement);
        return adoptRef(new SVGAnimatedListPropertyTearOff<PropertyType>(contextElement, attributeName, animatedPropertyType, values));
    }

protected:
    SVGAnimatedListPropertyTearOff(SVGElement* contextElement, const QualifiedName& attributeName, AnimatedPropertyType animatedPropertyType, PropertyType& values)
        : SVGAnimatedProperty(contextElement, attributeName, animatedPropertyType)
        , m_values(values)
    {
        if (!values.isEmpty())
            m_wrappers.fill(0, values.size());
    }

    PropertyType& m_values;

    ListWrapperCache m_wrappers;
    ListWrapperCache m_animatedWrappers;

    RefPtr<SVGProperty> m_baseVal;
    RefPtr<SVGProperty> m_animVal;
};

}

#endif // ENABLE(SVG)
#endif // SVGAnimatedListPropertyTearOff_h
