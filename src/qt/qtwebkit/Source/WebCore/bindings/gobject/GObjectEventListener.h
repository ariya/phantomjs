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

#ifndef GObjectEventListener_h
#define GObjectEventListener_h

#include "EventListener.h"
#include "EventTarget.h"
#include <wtf/RefPtr.h>
#include <wtf/text/CString.h>

typedef struct _GObject GObject;
typedef void (*GCallback) (void);

namespace WebCore {

class GObjectEventListener : public EventListener {
public:

    static bool addEventListener(GObject* object, EventTarget* target, const char* domEventName, GCallback handler, bool useCapture, void* userData)
    {
        RefPtr<GObjectEventListener> listener(adoptRef(new GObjectEventListener(object, target, domEventName, handler, useCapture, userData)));
        return target->addEventListener(domEventName, listener.release(), useCapture);
    }

    static bool removeEventListener(GObject* object, EventTarget* target, const char* domEventName, GCallback handler, bool useCapture)
    {
        GObjectEventListener key(object, target, domEventName, handler, useCapture, 0);
        return target->removeEventListener(domEventName, &key, useCapture);
    }

    static void gobjectDestroyedCallback(GObjectEventListener* listener, GObject*)
    {
        listener->gobjectDestroyed();
    }

    static const GObjectEventListener* cast(const EventListener* listener)
    {
        return listener->type() == GObjectEventListenerType
            ? static_cast<const GObjectEventListener*>(listener)
            : 0;
    }

    virtual bool operator==(const EventListener& other);

private:
    GObjectEventListener(GObject*, EventTarget*, const char* domEventName, GCallback handler, bool capture, void* userData);
    ~GObjectEventListener();
    void gobjectDestroyed();

    virtual void handleEvent(ScriptExecutionContext*, Event*);

    GObject* m_object;

    // We do not need to keep a reference to the m_coreTarget, because
    // we only use it when the GObject and thus the m_coreTarget object is alive.
    EventTarget* m_coreTarget;
    CString m_domEventName;
    GCallback m_handler;
    bool m_capture;
    void* m_userData;
};
} // namespace WebCore

#endif
