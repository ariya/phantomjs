/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 *           (C) 2001 Peter Kelly (pmk@post.com)
 *           (C) 2001 Dirk Mueller (mueller@kde.org)
 * Copyright (C) 2004, 2005, 2006, 2007 Apple Inc. All rights reserved.
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
#include "Attribute.h"

#include "Attr.h"
#include "Element.h"
#include <wtf/HashMap.h>
#include <wtf/UnusedParam.h>

namespace WebCore {

typedef HashMap<Attribute*, Attr*> AttributeAttrMap;
static AttributeAttrMap& attributeAttrMap()
{
    DEFINE_STATIC_LOCAL(AttributeAttrMap, map, ());
    return map;
}

PassRefPtr<Attribute> Attribute::clone() const
{
    return adoptRef(new Attribute(m_name, m_value, m_isMappedAttribute, m_styleDecl.get()));
}

Attr* Attribute::attr() const
{
    if (m_hasAttr) {
        ASSERT(attributeAttrMap().contains(const_cast<Attribute*>(this)));
        return attributeAttrMap().get(const_cast<Attribute*>(this));
    }

    ASSERT(!attributeAttrMap().contains(const_cast<Attribute*>(this)));
    return 0;
}

PassRefPtr<Attr> Attribute::createAttrIfNeeded(Element* e)
{
    RefPtr<Attr> r;
    if (m_hasAttr) {
        ASSERT(attributeAttrMap().contains(this));
        r = attributeAttrMap().get(this);
    } else {
        ASSERT(!attributeAttrMap().contains(this));
        r = Attr::create(e, e->document(), this); // This will end up calling Attribute::bindAttr.
        ASSERT(attributeAttrMap().contains(this));
    }

    return r.release();
}

void Attribute::bindAttr(Attr* attr)
{
    ASSERT(!m_hasAttr);
    ASSERT(!attributeAttrMap().contains(this));
    attributeAttrMap().set(this, attr);
    m_hasAttr = true;
}

void Attribute::unbindAttr(Attr* attr)
{
    ASSERT(m_hasAttr);
    ASSERT(attributeAttrMap().contains(this));
    ASSERT_UNUSED(attr, attributeAttrMap().get(this) == attr);
    attributeAttrMap().remove(this);
    m_hasAttr = false;
}

} // namespace WebCore
