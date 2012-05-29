/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
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

#ifndef HashChangeEvent_h
#define HashChangeEvent_h

#include "Event.h"
#include "EventNames.h"

namespace WebCore {

class HashChangeEvent : public Event {
public:
    virtual bool isHashChangeEvent() const { return true; }

    static PassRefPtr<HashChangeEvent> create(const String& oldURL, const String& newURL)
    {
        return adoptRef(new HashChangeEvent(oldURL, newURL));
    }

    void initHashChangeEvent(const AtomicString& eventType, bool canBubble, bool cancelable, const String& oldURL, const String& newURL)
    {
        if (dispatched())
            return;

        initEvent(eventType, canBubble, cancelable);

        m_oldURL = oldURL;
        m_newURL = newURL;
    }

    const String& oldURL() const { return m_oldURL; }
    const String& newURL() const { return m_newURL; }

private:
    HashChangeEvent(const String& oldURL, const String& newURL)
        : Event(eventNames().hashchangeEvent, false, false)
        , m_oldURL(oldURL)
        , m_newURL(newURL)
    {}

    String m_oldURL;
    String m_newURL;
};

} // namespace WebCore

#endif // HashChangeEvent_h
