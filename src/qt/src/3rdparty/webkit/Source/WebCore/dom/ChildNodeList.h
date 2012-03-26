/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 *           (C) 2001 Dirk Mueller (mueller@kde.org)
 * Copyright (C) 2004, 2007 Apple Inc. All rights reserved.
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

#ifndef ChildNodeList_h
#define ChildNodeList_h

#include "DynamicNodeList.h"
#include <wtf/PassRefPtr.h>

namespace WebCore {

    class ChildNodeList : public DynamicNodeList {
    public:
        static PassRefPtr<ChildNodeList> create(PassRefPtr<Node> rootNode, Caches* caches)
        {
            return adoptRef(new ChildNodeList(rootNode, caches));
        }

        virtual unsigned length() const;
        virtual Node* item(unsigned index) const;

    protected:
        ChildNodeList(PassRefPtr<Node> rootNode, Caches*);

        virtual bool nodeMatches(Element*) const;
    };

} // namespace WebCore

#endif // ChildNodeList_h
