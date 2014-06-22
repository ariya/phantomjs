/*
 * Copyright (C) 2008, 2012 Apple Inc. All Rights Reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#ifndef JSDOMGlobalObject_h
#define JSDOMGlobalObject_h

#include "PlatformExportMacros.h"
#include <runtime/JSGlobalObject.h>
#include <runtime/Operations.h>

namespace WebCore {

    class Document;
    class Event;
    class DOMWrapperWorld;
    class JSLazyEventListener;
    class JSEventListener;
    class ScriptExecutionContext;

    typedef HashMap<const JSC::ClassInfo*, JSC::WriteBarrier<JSC::Structure> > JSDOMStructureMap;
    typedef HashMap<const JSC::ClassInfo*, JSC::WriteBarrier<JSC::JSObject> > JSDOMConstructorMap;

    class JSDOMGlobalObject : public JSC::JSGlobalObject {
        typedef JSC::JSGlobalObject Base;
    protected:
        struct JSDOMGlobalObjectData;

        JSDOMGlobalObject(JSC::VM&, JSC::Structure*, PassRefPtr<DOMWrapperWorld>, const JSC::GlobalObjectMethodTable* = 0);
        static void destroy(JSC::JSCell*);
        void finishCreation(JSC::VM&);
        void finishCreation(JSC::VM&, JSC::JSObject*);

    public:
        JSDOMStructureMap& structures() { return m_structures; }
        JSDOMConstructorMap& constructors() { return m_constructors; }

        ScriptExecutionContext* scriptExecutionContext() const;

        // Make binding code generation easier.
        JSDOMGlobalObject* globalObject() { return this; }

        void setCurrentEvent(Event*);
        Event* currentEvent() const;

        static void visitChildren(JSC::JSCell*, JSC::SlotVisitor&);

        DOMWrapperWorld* world() { return m_world.get(); }

        static WEBKIT_EXPORTDATA const JSC::ClassInfo s_info;

        static JSC::Structure* createStructure(JSC::VM& vm, JSC::JSValue prototype)
        {
            return JSC::Structure::create(vm, 0, prototype, JSC::TypeInfo(JSC::GlobalObjectType, StructureFlags), &s_info);
        }

    protected:
        JSDOMStructureMap m_structures;
        JSDOMConstructorMap m_constructors;

        Event* m_currentEvent;
        RefPtr<DOMWrapperWorld> m_world;
    };

    template<class ConstructorClass>
    inline JSC::JSObject* getDOMConstructor(JSC::ExecState* exec, const JSDOMGlobalObject* globalObject)
    {
        if (JSC::JSObject* constructor = const_cast<JSDOMGlobalObject*>(globalObject)->constructors().get(&ConstructorClass::s_info).get())
            return constructor;
        JSC::JSObject* constructor = ConstructorClass::create(exec, ConstructorClass::createStructure(exec->vm(), const_cast<JSDOMGlobalObject*>(globalObject), globalObject->objectPrototype()), const_cast<JSDOMGlobalObject*>(globalObject));
        ASSERT(!const_cast<JSDOMGlobalObject*>(globalObject)->constructors().contains(&ConstructorClass::s_info));
        JSC::WriteBarrier<JSC::JSObject> temp;
        const_cast<JSDOMGlobalObject*>(globalObject)->constructors().add(&ConstructorClass::s_info, temp).iterator->value.set(exec->vm(), globalObject, constructor);
        return constructor;
    }

    JSDOMGlobalObject* toJSDOMGlobalObject(Document*, JSC::ExecState*);
    JSDOMGlobalObject* toJSDOMGlobalObject(ScriptExecutionContext*, JSC::ExecState*);

    JSDOMGlobalObject* toJSDOMGlobalObject(Document*, DOMWrapperWorld*);
    JSDOMGlobalObject* toJSDOMGlobalObject(ScriptExecutionContext*, DOMWrapperWorld*);

} // namespace WebCore

#endif // JSDOMGlobalObject_h
