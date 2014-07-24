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

#ifndef SVGStaticListPropertyTearOff_h
#define SVGStaticListPropertyTearOff_h

#if ENABLE(SVG)
#include "SVGListProperty.h"

namespace WebCore {

template<typename PropertyType>
class SVGStaticListPropertyTearOff : public SVGListProperty<PropertyType> {
public:
    typedef SVGListProperty<PropertyType> Base;

    typedef typename SVGPropertyTraits<PropertyType>::ListItemType ListItemType;
    typedef SVGPropertyTearOff<ListItemType> ListItemTearOff;

    using Base::m_role;
    using Base::m_values;

    static PassRefPtr<SVGStaticListPropertyTearOff<PropertyType> > create(SVGElement* contextElement, PropertyType& values)
    {
        ASSERT(contextElement);
        return adoptRef(new SVGStaticListPropertyTearOff<PropertyType>(contextElement, values));
    }

    // SVGList API
    void clear(ExceptionCode& ec)
    {
        Base::clearValues(ec);
    }

    ListItemType initialize(const ListItemType& newItem, ExceptionCode& ec)
    {
        return Base::initializeValues(newItem, ec);
    }

    ListItemType getItem(unsigned index, ExceptionCode& ec)
    {
        return Base::getItemValues(index, ec);
    }

    ListItemType insertItemBefore(const ListItemType& newItem, unsigned index, ExceptionCode& ec)
    {
        return Base::insertItemBeforeValues(newItem, index, ec);
    }

    ListItemType replaceItem(const ListItemType& newItem, unsigned index, ExceptionCode& ec)
    {
        return Base::replaceItemValues(newItem, index, ec);
    }

    ListItemType removeItem(unsigned index, ExceptionCode& ec)
    {
        return Base::removeItemValues(index, ec);
    }

    ListItemType appendItem(const ListItemType& newItem, ExceptionCode& ec)
    {
        return Base::appendItemValues(newItem, ec);
    }

private:
    SVGStaticListPropertyTearOff(SVGElement* contextElement, PropertyType& values)
        : SVGListProperty<PropertyType>(UndefinedRole, values, 0)
        , m_contextElement(contextElement)
    {
    }

    virtual bool isReadOnly() const
    {
        return m_role == AnimValRole;
    }

    virtual void commitChange()
    {
        ASSERT(m_values);
        m_values->commitChange(m_contextElement.get());
    }

    virtual bool processIncomingListItemValue(const ListItemType&, unsigned*)
    {
        // no-op for static lists
        return true;
    }

    virtual bool processIncomingListItemWrapper(RefPtr<ListItemTearOff>&, unsigned*)
    {
        ASSERT_NOT_REACHED();
        return true;
    }

private:
    RefPtr<SVGElement> m_contextElement;
};

}

#endif // ENABLE(SVG)
#endif // SVGStaticListPropertyTearOff_h
