/**
 * Copyright (C) 2001 Peter Kelly (pmk@post.com)
 * Copyright (C) 2001 Tobias Anton (anton@stud.fbi.fh-darmstadt.de)
 * Copyright (C) 2006 Samuel Weinig (sam.weinig@gmail.com)
 * Copyright (C) 2003, 2005, 2006, 2007 Apple Inc. All rights reserved.
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
#include "KeyboardEvent.h"

#include "Document.h"
#include "DOMWindow.h"
#include "EventDispatcher.h"
#include "EventNames.h"
#include "EventHandler.h"
#include "Frame.h"
#include "PlatformKeyboardEvent.h"
#include "Settings.h"

namespace WebCore {

static inline const AtomicString& eventTypeForKeyboardEventType(PlatformKeyboardEvent::Type type)
{
    switch (type) {
        case PlatformKeyboardEvent::KeyUp:
            return eventNames().keyupEvent;
        case PlatformKeyboardEvent::RawKeyDown:
            return eventNames().keydownEvent;
        case PlatformKeyboardEvent::Char:
            return eventNames().keypressEvent;
        case PlatformKeyboardEvent::KeyDown:
            // The caller should disambiguate the combined event into RawKeyDown or Char events.
            break;
    }
    ASSERT_NOT_REACHED();
    return eventNames().keydownEvent;
}

KeyboardEvent::KeyboardEvent()
    : m_keyLocation(DOM_KEY_LOCATION_STANDARD)
    , m_altGraphKey(false)
{
}

KeyboardEvent::KeyboardEvent(const PlatformKeyboardEvent& key, AbstractView* view)
    : UIEventWithKeyState(eventTypeForKeyboardEventType(key.type()),
                          true, true, view, 0, key.ctrlKey(), key.altKey(), key.shiftKey(), key.metaKey())
    , m_keyEvent(adoptPtr(new PlatformKeyboardEvent(key)))
    , m_keyIdentifier(key.keyIdentifier())
    , m_keyLocation(key.isKeypad() ? DOM_KEY_LOCATION_NUMPAD : DOM_KEY_LOCATION_STANDARD) // FIXME: differentiate right/left, too
    , m_altGraphKey(false)
{
}

KeyboardEvent::KeyboardEvent(const AtomicString& eventType, bool canBubble, bool cancelable, AbstractView *view,
                             const String &keyIdentifier,  unsigned keyLocation,
                             bool ctrlKey, bool altKey, bool shiftKey, bool metaKey, bool altGraphKey)
    : UIEventWithKeyState(eventType, canBubble, cancelable, view, 0, ctrlKey, altKey, shiftKey, metaKey)
    , m_keyIdentifier(keyIdentifier)
    , m_keyLocation(keyLocation)
    , m_altGraphKey(altGraphKey)
{
}

KeyboardEvent::~KeyboardEvent()
{
}

void KeyboardEvent::initKeyboardEvent(const AtomicString& type, bool canBubble, bool cancelable, AbstractView* view,
                                      const String &keyIdentifier, unsigned keyLocation,
                                      bool ctrlKey, bool altKey, bool shiftKey, bool metaKey, bool altGraphKey)
{
    if (dispatched())
        return;

    initUIEvent(type, canBubble, cancelable, view, 0);

    m_keyIdentifier = keyIdentifier;
    m_keyLocation = keyLocation;
    m_ctrlKey = ctrlKey;
    m_shiftKey = shiftKey;
    m_altKey = altKey;
    m_metaKey = metaKey;
    m_altGraphKey = altGraphKey;
}

bool KeyboardEvent::getModifierState(const String& keyIdentifier) const
{
    if (keyIdentifier == "Control")
        return ctrlKey();
    if (keyIdentifier == "Shift")
        return shiftKey();
    if (keyIdentifier == "Alt")
        return altKey();
    if (keyIdentifier == "Meta")
        return metaKey();
    return false;
}

int KeyboardEvent::keyCode() const
{
    // IE: virtual key code for keyup/keydown, character code for keypress
    // Firefox: virtual key code for keyup/keydown, zero for keypress
    // We match IE.
    if (!m_keyEvent)
        return 0;
    if (type() == eventNames().keydownEvent || type() == eventNames().keyupEvent)
        return m_keyEvent->windowsVirtualKeyCode();
    return charCode();
}

int KeyboardEvent::charCode() const
{
    // IE: not supported
    // Firefox: 0 for keydown/keyup events, character code for keypress
    // We match Firefox, unless in backward compatibility mode, where we always return the character code.
    bool backwardCompatibilityMode = false;
    if (view() && view()->frame())
        backwardCompatibilityMode = view()->frame()->eventHandler()->needsKeyboardEventDisambiguationQuirks();

    if (!m_keyEvent || (type() != eventNames().keypressEvent && !backwardCompatibilityMode))
        return 0;
    String text = m_keyEvent->text();
    return static_cast<int>(text.characterStartingAt(0));
}

bool KeyboardEvent::isKeyboardEvent() const
{
    return true;
}

int KeyboardEvent::which() const
{
    // Netscape's "which" returns a virtual key code for keydown and keyup, and a character code for keypress.
    // That's exactly what IE's "keyCode" returns. So they are the same for keyboard events.
    return keyCode();
}

KeyboardEvent* findKeyboardEvent(Event* event)
{
    for (Event* e = event; e; e = e->underlyingEvent())
        if (e->isKeyboardEvent())
            return static_cast<KeyboardEvent*>(e);
    return 0;
}

KeyboardEventDispatchMediator::KeyboardEventDispatchMediator(PassRefPtr<KeyboardEvent> event)
    : EventDispatchMediator(event)
{
}

bool KeyboardEventDispatchMediator::dispatchEvent(EventDispatcher* dispatcher) const
{
    // Make sure not to return true if we already took default action while handling the event.
    return EventDispatchMediator::dispatchEvent(dispatcher) && !event()->defaultHandled();
}

} // namespace WebCore
