/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 * Copyright (C) 2000 Frederik Holljen (frederik.holljen@hig.no)
 * Copyright (C) 2001 Peter Kelly (pmk@post.com)
 * Copyright (C) 2006 Samuel Weinig (sam.weinig@gmail.com)
 * Copyright (C) 2004, 2008, 2009 Apple Inc. All rights reserved.
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

#ifndef NodeFilterCondition_h
#define NodeFilterCondition_h

#include "ScriptState.h"
#include <wtf/RefCounted.h>

namespace JSC {

class SlotVisitor;

}

namespace WebCore {

    class Node;

    class NodeFilterCondition : public RefCounted<NodeFilterCondition> {
    public:
        virtual ~NodeFilterCondition() { }
        virtual short acceptNode(ScriptState*, Node*) const = 0;
        virtual void visitAggregate(JSC::SlotVisitor&) { }
    };

} // namespace WebCore

#endif // NodeFilterCondition_h
