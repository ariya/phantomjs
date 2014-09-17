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

#ifndef SVGPathSegListPropertyTearOff_h
#define SVGPathSegListPropertyTearOff_h

#if ENABLE(SVG)
#include "SVGAnimatedListPropertyTearOff.h"
#include "SVGPathSegList.h"

namespace WebCore {

class SVGPathElement;

class SVGPathSegListPropertyTearOff : public SVGListProperty<SVGPathSegList> {
public:
    typedef SVGListProperty<SVGPathSegList> Base;
    typedef SVGAnimatedListPropertyTearOff<SVGPathSegList> AnimatedListPropertyTearOff;
    typedef SVGPropertyTraits<SVGPathSegList>::ListItemType ListItemType;
    typedef PassRefPtr<SVGPathSeg> PassListItemType;

    static PassRefPtr<SVGPathSegListPropertyTearOff> create(AnimatedListPropertyTearOff* animatedProperty, SVGPropertyRole role, SVGPathSegRole pathSegRole)
    {
        ASSERT(animatedProperty);
        return adoptRef(new SVGPathSegListPropertyTearOff(animatedProperty, role, pathSegRole));
    }

    int removeItemFromList(const ListItemType& removeItem, bool shouldSynchronizeWrappers)
    {
        SVGPathSegList& values = m_animatedProperty->values();

        unsigned size = values.size();
        for (unsigned i = 0; i < size; ++i) {
            ListItemType& item = values.at(i);
            if (item != removeItem)
                continue;

            values.remove(i);

            if (shouldSynchronizeWrappers)
                commitChange();

            return i;
        }

        return -1;
    }

    // SVGList API
    void clear(ExceptionCode&);

    unsigned numberOfItems() const
    {
        SVGPathSegList& values = m_animatedProperty->values();
        return Base::numberOfItemsValues(values);
    }

    PassListItemType initialize(PassListItemType passNewItem, ExceptionCode& ec)
    {
        // Not specified, but FF/Opera do it this way, and it's just sane.
        if (!passNewItem) {
            ec = SVGException::SVG_WRONG_TYPE_ERR;
            return 0;
        }

        ListItemType newItem = passNewItem;
        SVGPathSegList& values = m_animatedProperty->values();
        return Base::initializeValues(values, newItem, ec);
    }

    PassListItemType getItem(unsigned index, ExceptionCode&);

    PassListItemType insertItemBefore(PassListItemType passNewItem, unsigned index, ExceptionCode& ec)
    {
        // Not specified, but FF/Opera do it this way, and it's just sane.
        if (!passNewItem) {
            ec = SVGException::SVG_WRONG_TYPE_ERR;
            return 0;
        }

        ListItemType newItem = passNewItem;
        SVGPathSegList& values = m_animatedProperty->values();
        return Base::insertItemBeforeValues(values, newItem, index, ec);
    }

    PassListItemType replaceItem(PassListItemType passNewItem, unsigned index, ExceptionCode& ec)
    {
        // Not specified, but FF/Opera do it this way, and it's just sane.
        if (!passNewItem) {
            ec = SVGException::SVG_WRONG_TYPE_ERR;
            return 0;
        }

        ListItemType newItem = passNewItem;
        SVGPathSegList& values = m_animatedProperty->values();
        return Base::replaceItemValues(values, newItem, index, ec);
    }

    PassListItemType removeItem(unsigned index, ExceptionCode&);

    PassListItemType appendItem(PassListItemType passNewItem, ExceptionCode& ec)
    {
        // Not specified, but FF/Opera do it this way, and it's just sane.
        if (!passNewItem) {
            ec = SVGException::SVG_WRONG_TYPE_ERR;
            return 0;
        }

        ListItemType newItem = passNewItem;
        SVGPathSegList& values = m_animatedProperty->values();
        return Base::appendItemValues(values, newItem, ec);
    }

private:
    SVGPathSegListPropertyTearOff(AnimatedListPropertyTearOff* animatedProperty, SVGPropertyRole role, SVGPathSegRole pathSegRole)
        : SVGListProperty<SVGPathSegList>(role)
        , m_animatedProperty(animatedProperty)
        , m_pathSegRole(pathSegRole)
    {
    }

    SVGPathElement* contextElement() const;

    virtual void commitChange()
    {
        SVGPathSegList& values = m_animatedProperty->values();
        values.commitChange(m_animatedProperty->contextElement());
    }

    virtual void processIncomingListItemValue(const ListItemType& newItem, unsigned* indexToModify);
    virtual void processIncomingListItemWrapper(RefPtr<ListItemTearOff>&, unsigned*)
    {
        ASSERT_NOT_REACHED();
    }

private:
    RefPtr<AnimatedListPropertyTearOff> m_animatedProperty;
    SVGPathSegRole m_pathSegRole;
};

}

#endif // ENABLE(SVG)
#endif // SVGListPropertyTearOff_h
