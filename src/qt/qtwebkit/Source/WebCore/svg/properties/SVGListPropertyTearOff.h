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

#ifndef SVGListPropertyTearOff_h
#define SVGListPropertyTearOff_h

#if ENABLE(SVG)
#include "SVGListProperty.h"

namespace WebCore {

template<typename PropertyType>
class SVGListPropertyTearOff : public SVGListProperty<PropertyType> {
public:
    typedef SVGListProperty<PropertyType> Base;
    typedef SVGListPropertyTearOff<PropertyType> Self;

    typedef typename SVGPropertyTraits<PropertyType>::ListItemType ListItemType;
    typedef SVGPropertyTearOff<ListItemType> ListItemTearOff;
    typedef PassRefPtr<ListItemTearOff> PassListItemTearOff;
    typedef SVGAnimatedListPropertyTearOff<PropertyType> AnimatedListPropertyTearOff;
    typedef typename SVGAnimatedListPropertyTearOff<PropertyType>::ListWrapperCache ListWrapperCache;

    using Base::m_role;
    using Base::m_values;
    using Base::m_wrappers;

    static PassRefPtr<Self> create(AnimatedListPropertyTearOff* animatedProperty, SVGPropertyRole role, PropertyType& values, ListWrapperCache& wrappers)
    {
        ASSERT(animatedProperty);
        return adoptRef(new Self(animatedProperty, role, values, wrappers));
    }

    int findItem(ListItemTearOff* item) const
    {
        ASSERT(m_values);
        ASSERT(m_wrappers);

        unsigned size = m_wrappers->size();
        ASSERT(size == m_values->size());
        for (size_t i = 0; i < size; ++i) {
            if (item == m_wrappers->at(i))
                return i;
        }

        return -1;
    }

    void removeItemFromList(size_t itemIndex, bool shouldSynchronizeWrappers)
    {
        ASSERT(m_values);
        ASSERT(m_wrappers);
        ASSERT(m_values->size() == m_wrappers->size());
        ASSERT_WITH_SECURITY_IMPLICATION(itemIndex < m_wrappers->size());

        RefPtr<ListItemTearOff>& item = m_wrappers->at(itemIndex);
        item->detachWrapper();
        m_wrappers->remove(itemIndex);
        m_values->remove(itemIndex);

        if (shouldSynchronizeWrappers)
            commitChange();
    }

    // SVGList API
    void clear(ExceptionCode& ec)
    {
        Base::clearValuesAndWrappers(ec);
    }

    PassListItemTearOff initialize(PassListItemTearOff passNewItem, ExceptionCode& ec)
    {
        return Base::initializeValuesAndWrappers(passNewItem, ec);
    }

    PassListItemTearOff getItem(unsigned index, ExceptionCode& ec)
    {
        return Base::getItemValuesAndWrappers(m_animatedProperty.get(), index, ec);
    }

    PassListItemTearOff insertItemBefore(PassListItemTearOff passNewItem, unsigned index, ExceptionCode& ec)
    {
        return Base::insertItemBeforeValuesAndWrappers(passNewItem, index, ec);
    }

    PassListItemTearOff replaceItem(PassListItemTearOff passNewItem, unsigned index, ExceptionCode& ec)
    {
        return Base::replaceItemValuesAndWrappers(passNewItem, index, ec);
    }

    PassListItemTearOff removeItem(unsigned index, ExceptionCode& ec)
    {
        return Base::removeItemValuesAndWrappers(m_animatedProperty.get(), index, ec);
    }

    PassListItemTearOff appendItem(PassListItemTearOff passNewItem, ExceptionCode& ec)
    {
        return Base::appendItemValuesAndWrappers(passNewItem, ec);
    }

protected:
    SVGListPropertyTearOff(AnimatedListPropertyTearOff* animatedProperty, SVGPropertyRole role, PropertyType& values, ListWrapperCache& wrappers)
        : SVGListProperty<PropertyType>(role, values, &wrappers)
        , m_animatedProperty(animatedProperty)
    {
    }

    virtual bool isReadOnly() const
    {
        if (m_role == AnimValRole)
            return true;
        if (m_animatedProperty && m_animatedProperty->isReadOnly())
            return true;
        return false;
    }

    virtual void commitChange()
    {
        ASSERT(m_values);
        ASSERT(m_wrappers);

        // Update existing wrappers, as the index in the values list has changed.
        unsigned size = m_wrappers->size();
        ASSERT(size == m_values->size());
        for (unsigned i = 0; i < size; ++i) {
            ListItemTearOff* item = m_wrappers->at(i).get();
            if (!item)
                continue;
            item->setAnimatedProperty(m_animatedProperty.get());
            item->setValue(m_values->at(i));
        }

        m_animatedProperty->commitChange();
    }

    virtual bool processIncomingListItemValue(const ListItemType&, unsigned*)
    {
        ASSERT_NOT_REACHED();
        return true;
    }

    virtual bool processIncomingListItemWrapper(RefPtr<ListItemTearOff>& newItem, unsigned* indexToModify)
    {
        SVGAnimatedProperty* animatedPropertyOfItem = newItem->animatedProperty();

        // newItem has been created manually, it doesn't belong to any SVGElement.
        // (for example: "textElement.x.baseVal.appendItem(svgsvgElement.createSVGLength())")
        if (!animatedPropertyOfItem)
            return true;

        // newItem belongs to a SVGElement, but its associated SVGAnimatedProperty is not an animated list tear off.
        // (for example: "textElement.x.baseVal.appendItem(rectElement.width.baseVal)")
        if (!animatedPropertyOfItem->isAnimatedListTearOff()) {
            // We have to copy the incoming newItem, as we're not allowed to insert this tear off as is into our wrapper cache.
            // Otherwhise we'll end up having two SVGAnimatedPropertys that operate on the same SVGPropertyTearOff. Consider the example above:
            // SVGRectElements SVGAnimatedLength 'width' property baseVal points to the same tear off object
            // that's inserted into SVGTextElements SVGAnimatedLengthList 'x'. textElement.x.baseVal.getItem(0).value += 150 would
            // mutate the rectElement width _and_ the textElement x list. That's obviously wrong, take care of that.
            newItem = ListItemTearOff::create(newItem->propertyReference());
            return true;
        }

        // Spec: If newItem is already in a list, it is removed from its previous list before it is inserted into this list.
        // 'newItem' is already living in another list. If it's not our list, synchronize the other lists wrappers after the removal.
        bool livesInOtherList = animatedPropertyOfItem != m_animatedProperty;
        AnimatedListPropertyTearOff* propertyTearOff = static_cast<AnimatedListPropertyTearOff*>(animatedPropertyOfItem);
        int indexToRemove = propertyTearOff->findItem(newItem.get());
        ASSERT(indexToRemove != -1);

        // Do not remove newItem if already in this list at the target index.
        if (!livesInOtherList && indexToModify && static_cast<unsigned>(indexToRemove) == *indexToModify)
            return false;

        propertyTearOff->removeItemFromList(indexToRemove, livesInOtherList);

        if (!indexToModify)
            return true;

        // If the item lived in our list, adjust the insertion index.
        if (!livesInOtherList) {
            unsigned& index = *indexToModify;
            // Spec: If the item is already in this list, note that the index of the item to (replace|insert before) is before the removal of the item.
            if (static_cast<unsigned>(indexToRemove) < index)
                --index;
        }

        return true;
    }

    // Back pointer to the animated property that created us
    // For example (text.x.baseVal): m_animatedProperty points to the 'x' SVGAnimatedLengthList object
    RefPtr<AnimatedListPropertyTearOff> m_animatedProperty;
};

}

#endif // ENABLE(SVG)
#endif // SVGListPropertyTearOff_h
