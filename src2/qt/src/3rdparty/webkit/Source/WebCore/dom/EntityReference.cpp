/*
 * Copyright (C) 2000 Peter Kelly (pmk@post.com)
 * Copyright (C) 2006, 2008, 2009 Apple Inc. All rights reserved.
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
#include "EntityReference.h"

#include "Document.h"

namespace WebCore {

inline EntityReference::EntityReference(Document* document, const String& entityName)
    : ContainerNode(document)
    , m_entityName(entityName)
{
}

PassRefPtr<EntityReference> EntityReference::create(Document* document, const String& entityName)
{
    return adoptRef(new EntityReference(document, entityName));
}

String EntityReference::nodeName() const
{
    return m_entityName;
}

Node::NodeType EntityReference::nodeType() const
{
    return ENTITY_REFERENCE_NODE;
}

PassRefPtr<Node> EntityReference::cloneNode(bool)
{
    return create(document(), m_entityName);
}

} // namespace
