/*
 * Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies)
 * Copyright (C) 2010 Antonio Gomes <tonikitoo@webkit.org>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "StaticHashSetNodeList.h"

#include "Element.h"

namespace WebCore {

StaticHashSetNodeList::StaticHashSetNodeList(ListHashSet<RefPtr<Node> >& nodes)
{
    m_nodes.swap(nodes);
}

StaticHashSetNodeList::StaticHashSetNodeList()
{
}

StaticHashSetNodeList::~StaticHashSetNodeList()
{
}

unsigned StaticHashSetNodeList::length() const
{
    return m_nodes.size();
}

Node* StaticHashSetNodeList::item(unsigned index) const
{
    if (index < (unsigned) m_nodes.size()) {
        ListHashSet<RefPtr<Node> >::const_iterator it = m_nodes.begin();
        for (unsigned count = 0; count < index; ++it, ++count) { }
        return (*it).get();
    }

    return 0;
}

Node* StaticHashSetNodeList::itemWithName(const AtomicString& elementId) const
{
    ListHashSet<RefPtr<Node> >::const_iterator it = m_nodes.begin();
    ListHashSet<RefPtr<Node> >::const_iterator end = m_nodes.end();
    for ( ; it != end ; ++it) {
        Node* node = (*it).get();
        if (node->hasID() && static_cast<Element*>(node)->getIdAttribute() == elementId)
            return node;
    }

    return 0;
}

} // namespace WebCore
