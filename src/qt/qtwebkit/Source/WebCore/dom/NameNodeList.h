/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 *           (C) 2001 Dirk Mueller (mueller@kde.org)
 * Copyright (C) 2004, 2007m 2008 Apple Inc. All rights reserved.
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
 *
 */

#ifndef NameNodeList_h
#define NameNodeList_h

#include "LiveNodeList.h"
#include <wtf/Forward.h>
#include <wtf/text/AtomicString.h>

namespace WebCore {

// NodeList which lists all Nodes in a Element with a given "name" attribute
class NameNodeList : public LiveNodeList {
public:
    static PassRefPtr<NameNodeList> create(PassRefPtr<Node> rootNode, CollectionType type, const AtomicString& name)
    {
        ASSERT_UNUSED(type, type == NameNodeListType);
        return adoptRef(new NameNodeList(rootNode, name));
    }

    virtual ~NameNodeList();

private:
    NameNodeList(PassRefPtr<Node> rootNode, const AtomicString& name);

    virtual bool nodeMatches(Element*) const;

    AtomicString m_name;
};

} // namespace WebCore

#endif // NameNodeList_h
