/*
 * Copyright (C) 2001 Peter Kelly (pmk@post.com)
 * Copyright (C) 2001 Tobias Anton (anton@stud.fbi.fh-darmstadt.de)
 * Copyright (C) 2006 Samuel Weinig (sam.weinig@gmail.com)
 * Copyright (C) 2003, 2004, 2005, 2006, 2008 Apple Inc. All rights reserved.
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

#ifndef MutationEvent_h
#define MutationEvent_h

#include "Event.h"
#include "Node.h"

namespace WebCore {

    class MutationEvent : public Event {
    public:
        virtual ~MutationEvent();

        enum attrChangeType {
            MODIFICATION    = 1,
            ADDITION        = 2,
            REMOVAL         = 3
        };

        static PassRefPtr<MutationEvent> create()
        {
            return adoptRef(new MutationEvent);
        }

        static PassRefPtr<MutationEvent> create(const AtomicString& type, bool canBubble, PassRefPtr<Node> relatedNode = 0,
            const String& prevValue = String(), const String& newValue = String(), const String& attrName = String(), unsigned short attrChange = 0)
        {
            return adoptRef(new MutationEvent(type, canBubble, false, relatedNode, prevValue, newValue, attrName, attrChange));
        }

        void initMutationEvent(const AtomicString& type, bool canBubble, bool cancelable, PassRefPtr<Node> relatedNode,
                               const String& prevValue, const String& newValue,
                               const String& attrName, unsigned short attrChange);

        Node* relatedNode() const { return m_relatedNode.get(); }
        String prevValue() const { return m_prevValue; }
        String newValue() const { return m_newValue; }
        String attrName() const { return m_attrName; }
        unsigned short attrChange() const { return m_attrChange; }

        virtual bool isMutationEvent() const;

    private:
        MutationEvent();
        MutationEvent(const AtomicString& type, bool canBubble, bool cancelable, PassRefPtr<Node> relatedNode,
                      const String& prevValue, const String& newValue,
                      const String& attrName, unsigned short attrChange);

        RefPtr<Node> m_relatedNode;
        String m_prevValue;
        String m_newValue;
        String m_attrName;
        unsigned short m_attrChange;
    };

} // namespace WebCore

#endif // MutationEvent_h
