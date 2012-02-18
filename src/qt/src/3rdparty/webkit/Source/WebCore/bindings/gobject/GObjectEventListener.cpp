/*
 *  Copyright (C) 2010, 2011 Igalia S.L.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "config.h"
#include "GObjectEventListener.h"

#include "Event.h"
#include "EventListener.h"
#include "webkit/WebKitDOMEvent.h"
#include "webkit/WebKitDOMEventPrivate.h"
#include <glib-object.h>
#include <glib.h>
#include <wtf/HashMap.h>

namespace WebCore {

typedef void (*GObjectEventListenerCallback)(GObject*, WebKitDOMEvent*, void*);

GObjectEventListener::GObjectEventListener(GObject* object, EventTarget* target, const char* domEventName, GCallback handler, bool capture, void* userData)
    : EventListener(GObjectEventListenerType)
    , m_object(object)
    , m_coreTarget(target)
    , m_domEventName(domEventName)
    , m_handler(handler)
    , m_capture(capture)
    , m_userData(userData)
{
    ASSERT(m_coreTarget);
    g_object_weak_ref(object, reinterpret_cast<GWeakNotify>(GObjectEventListener::gobjectDestroyedCallback), this);
}

GObjectEventListener::~GObjectEventListener()
{
    if (!m_coreTarget)
        return;
    g_object_weak_unref(m_object, reinterpret_cast<GWeakNotify>(GObjectEventListener::gobjectDestroyedCallback), this);
}

void GObjectEventListener::gobjectDestroyed()
{
    ASSERT(m_coreTarget);

    // We must set m_coreTarget to null, because removeEventListener
    // may call the destructor as a side effect and we must be in the
    // proper state to prevent g_object_weak_unref.
    EventTarget* target = m_coreTarget;
    m_coreTarget = 0;
    target->removeEventListener(m_domEventName.data(), this, m_capture);
}

void GObjectEventListener::handleEvent(ScriptExecutionContext*, Event* event)
{
    WebKitDOMEvent* gobjectEvent = WEBKIT_DOM_EVENT(WebKit::kit(event));
    reinterpret_cast<GObjectEventListenerCallback>(m_handler)(m_object, gobjectEvent, m_userData);
    g_object_unref(gobjectEvent);
}

bool GObjectEventListener::operator==(const EventListener& listener)
{
    if (const GObjectEventListener* gobjectEventListener = GObjectEventListener::cast(&listener))
        return m_object == gobjectEventListener->m_object && m_handler == gobjectEventListener->m_handler;

    return false;
}

}
