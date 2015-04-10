/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 * Copyright (C) 2003, 2008, 2009 Apple Inc. All rights reserved.
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
#include "CDATASection.h"

#include "Document.h"

namespace WebCore {

inline CDATASection::CDATASection(Document* document, const String& data)
    : Text(document, data, CreateText)
{
}

PassRefPtr<CDATASection> CDATASection::create(Document* document, const String& data)
{
    return adoptRef(new CDATASection(document, data));
}

String CDATASection::nodeName() const
{
    return "#cdata-section";
}

Node::NodeType CDATASection::nodeType() const
{
    return CDATA_SECTION_NODE;
}

PassRefPtr<Node> CDATASection::cloneNode(bool /*deep*/)
{
    return create(document(), data());
}

bool CDATASection::childTypeAllowed(NodeType) const
{
    return false;
}

PassRefPtr<Text> CDATASection::virtualCreate(const String& data)
{
    return create(document(), data);
}

} // namespace WebCore
