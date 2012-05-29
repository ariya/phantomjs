/*
 *  Copyright (C) 2001 Peter Kelly (pmk@post.com)
 *  Copyright (C) 2003, 2008, 2009 Apple Inc. All rights reserved.
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

#ifndef JSEventListener_h
#define JSEventListener_h

#include "EventListener.h"
#include "JSDOMWindow.h"
#include <heap/Weak.h>

namespace WebCore {

    class JSDOMGlobalObject;

    class JSEventListener : public EventListener {
    public:
        static PassRefPtr<JSEventListener> create(JSC::JSObject* listener, JSC::JSObject* wrapper, bool isAttribute, DOMWrapperWorld* isolatedWorld)
        {
            return adoptRef(new JSEventListener(listener, wrapper, isAttribute, isolatedWorld));
        }

        static const JSEventListener* cast(const EventListener* listener)
        {
            return listener->type() == JSEventListenerType
                ? static_cast<const JSEventListener*>(listener)
                : 0;
        }

        virtual ~JSEventListener();

        virtual bool operator==(const EventListener& other);

        // Returns true if this event listener was created for an event handler attribute, like "onload" or "onclick".
        bool isAttribute() const { return m_isAttribute; }

        JSC::JSObject* jsFunction(ScriptExecutionContext*) const;
        DOMWrapperWorld* isolatedWorld() const { return m_isolatedWorld.get(); }

        JSC::JSObject* wrapper() const { return m_wrapper.get(); }
        void setWrapper(JSC::JSGlobalData& globalData, JSC::JSObject* wrapper) const { m_wrapper.set(globalData, wrapper, 0); }

    private:
        virtual JSC::JSObject* initializeJSFunction(ScriptExecutionContext*) const;
        virtual void visitJSFunction(JSC::SlotVisitor&);
        virtual bool virtualisAttribute() const;

    protected:
        JSEventListener(JSC::JSObject* function, JSC::JSObject* wrapper, bool isAttribute, DOMWrapperWorld* isolatedWorld);
        virtual void handleEvent(ScriptExecutionContext*, Event*);

    private:
        mutable JSC::WriteBarrier<JSC::JSObject> m_jsFunction;
        mutable JSC::Weak<JSC::JSObject> m_wrapper;

        bool m_isAttribute;
        RefPtr<DOMWrapperWorld> m_isolatedWorld;
    };

    inline JSC::JSObject* JSEventListener::jsFunction(ScriptExecutionContext* scriptExecutionContext) const
    {
        if (!m_jsFunction)
            m_jsFunction.set(*scriptExecutionContext->globalData(), m_wrapper.get(), initializeJSFunction(scriptExecutionContext));

        // Verify that we have a valid wrapper protecting our function from
        // garbage collection.
        ASSERT(m_wrapper || !m_jsFunction);
        if (!m_wrapper)
            return 0;

        // Try to verify that m_jsFunction wasn't recycled. (Not exact, since an
        // event listener can be almost anything, but this makes test-writing easier).
        ASSERT(!m_jsFunction || static_cast<JSC::JSCell*>(m_jsFunction.get())->isObject());

        return m_jsFunction.get();
    }

    // Creates a JS EventListener for an "onXXX" event attribute.
    inline PassRefPtr<JSEventListener> createJSAttributeEventListener(JSC::ExecState* exec, JSC::JSValue listener, JSC::JSObject* wrapper)
    {
        if (!listener.isObject())
            return 0;

        return JSEventListener::create(asObject(listener), wrapper, true, currentWorld(exec));
    }


} // namespace WebCore

#endif // JSEventListener_h
