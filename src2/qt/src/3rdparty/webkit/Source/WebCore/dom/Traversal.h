/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 * Copyright (C) 2000 Frederik Holljen (frederik.holljen@hig.no)
 * Copyright (C) 2001 Peter Kelly (pmk@post.com)
 * Copyright (C) 2006 Samuel Weinig (sam.weinig@gmail.com)
 * Copyright (C) 2004, 2008 Apple Inc. All rights reserved.
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

#ifndef Traversal_h
#define Traversal_h

#include "ScriptState.h"
#include <wtf/RefPtr.h>

namespace WebCore {

    class Node;
    class NodeFilter;

    class Traversal {
    public:
        Node* root() const { return m_root.get(); }
        unsigned whatToShow() const { return m_whatToShow; }
        NodeFilter* filter() const { return m_filter.get(); }
        bool expandEntityReferences() const { return m_expandEntityReferences; }

    protected:
        Traversal(PassRefPtr<Node>, unsigned whatToShow, PassRefPtr<NodeFilter>, bool expandEntityReferences);
        short acceptNode(ScriptState*, Node*) const;

    private:
        RefPtr<Node> m_root;
        unsigned m_whatToShow;
        RefPtr<NodeFilter> m_filter;
        bool m_expandEntityReferences;
    };

} // namespace WebCore

#endif // Traversal_h
