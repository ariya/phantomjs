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

#include "config.h"

#if ENABLE(SVG)
#include "SVGPathSegListPropertyTearOff.h"

#include "SVGAnimatedPathSegListPropertyTearOff.h"
#include "SVGNames.h"
#include "SVGPathElement.h"
#include "SVGPathSegWithContext.h"

namespace WebCore {

void SVGPathSegListPropertyTearOff::clearContextAndRoles()
{
    ASSERT(m_values);
    unsigned size = m_values->size();
    for (unsigned i = 0; i < size; ++i) {
        ListItemType item = m_values->at(i);
        static_cast<SVGPathSegWithContext*>(item.get())->setContextAndRole(0, PathSegUndefinedRole);
    }
}

void SVGPathSegListPropertyTearOff::clear(ExceptionCode& ec)
{
    ASSERT(m_values);
    if (m_values->isEmpty())
        return;

    clearContextAndRoles();
    SVGPathSegListPropertyTearOff::Base::clearValues(ec);
}

SVGPathSegListPropertyTearOff::PassListItemType SVGPathSegListPropertyTearOff::getItem(unsigned index, ExceptionCode& ec)
{
    ListItemType returnedItem = Base::getItemValues(index, ec);
    if (returnedItem) {
        ASSERT(static_cast<SVGPathSegWithContext*>(returnedItem.get())->contextElement() == contextElement());
        ASSERT(static_cast<SVGPathSegWithContext*>(returnedItem.get())->role() == m_pathSegRole);
    }
    return returnedItem.release();
}

SVGPathSegListPropertyTearOff::PassListItemType SVGPathSegListPropertyTearOff::replaceItem(PassListItemType passNewItem, unsigned index, ExceptionCode& ec)
{
    // Not specified, but FF/Opera do it this way, and it's just sane.
    if (!passNewItem) {
        ec = SVGException::SVG_WRONG_TYPE_ERR;
        return 0;
    }

    if (index < m_values->size()) {
        ListItemType replacedItem = m_values->at(index);
        ASSERT(replacedItem);
        static_cast<SVGPathSegWithContext*>(replacedItem.get())->setContextAndRole(0, PathSegUndefinedRole);
    }

    ListItemType newItem = passNewItem;
    return Base::replaceItemValues(newItem, index, ec);
}

SVGPathSegListPropertyTearOff::PassListItemType SVGPathSegListPropertyTearOff::removeItem(unsigned index, ExceptionCode& ec)
{
    SVGPathSegListPropertyTearOff::ListItemType removedItem = SVGPathSegListPropertyTearOff::Base::removeItemValues(index, ec);
    if (removedItem)
        static_cast<SVGPathSegWithContext*>(removedItem.get())->setContextAndRole(0, PathSegUndefinedRole);
    return removedItem.release();
}

SVGPathElement* SVGPathSegListPropertyTearOff::contextElement() const
{
    SVGElement* contextElement = m_animatedProperty->contextElement();
    ASSERT(contextElement);
    return toSVGPathElement(contextElement);
}

bool SVGPathSegListPropertyTearOff::processIncomingListItemValue(const ListItemType& newItem, unsigned* indexToModify)
{
    SVGPathSegWithContext* newItemWithContext = static_cast<SVGPathSegWithContext*>(newItem.get());
    SVGAnimatedProperty* animatedPropertyOfItem = newItemWithContext->animatedProperty();

    // Alter the role, after calling animatedProperty(), as that may influence the returned animated property.
    newItemWithContext->setContextAndRole(contextElement(), m_pathSegRole);

    if (!animatedPropertyOfItem)
        return true;

    // newItem belongs to a SVGPathElement, but its associated SVGAnimatedProperty is not an animated list tear off.
    // (for example: "pathElement.pathSegList.appendItem(pathElement.createSVGPathSegClosepath())")
    if (!animatedPropertyOfItem->isAnimatedListTearOff())
        return true;

    // Spec: If newItem is already in a list, it is removed from its previous list before it is inserted into this list.
    // 'newItem' is already living in another list. If it's not our list, synchronize the other lists wrappers after the removal.
    bool livesInOtherList = animatedPropertyOfItem != m_animatedProperty;
    SVGAnimatedPathSegListPropertyTearOff* propertyTearOff = static_cast<SVGAnimatedPathSegListPropertyTearOff*>(animatedPropertyOfItem);
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

}

#endif // ENABLE(SVG)
