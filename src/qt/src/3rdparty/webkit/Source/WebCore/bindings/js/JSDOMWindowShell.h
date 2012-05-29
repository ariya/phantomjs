/*
 * Copyright (C) 2008 Apple Inc. All rights reserved.
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

#ifndef JSDOMWindowShell_h
#define JSDOMWindowShell_h

#include "JSDOMWindow.h"

namespace WebCore {

    class DOMWindow;
    class Frame;

    class JSDOMWindowShell : public JSC::JSNonFinalObject {
        typedef JSC::JSNonFinalObject Base;
    public:
        JSDOMWindowShell(PassRefPtr<DOMWindow>, DOMWrapperWorld* world);
        virtual ~JSDOMWindowShell();

        JSDOMWindow* window() const { return m_window.get(); }
        void setWindow(JSC::JSGlobalData& globalData, JSDOMWindow* window)
        {
            ASSERT_ARG(window, window);
            m_window.set(globalData, this, window);
            setPrototype(globalData, window->prototype());
        }
        void setWindow(PassRefPtr<DOMWindow>);

        static const JSC::ClassInfo s_info;

        DOMWindow* impl() const;

        void* operator new(size_t);

        static JSC::Structure* createStructure(JSC::JSGlobalData& globalData, JSC::JSValue prototype) 
        {
            return JSC::Structure::create(globalData, prototype, JSC::TypeInfo(JSC::ObjectType, StructureFlags), AnonymousSlotCount, &s_info); 
        }

        DOMWrapperWorld* world() { return m_world.get(); }

    private:
        static const unsigned StructureFlags = JSC::OverridesGetOwnPropertySlot | JSC::OverridesVisitChildren | JSC::OverridesGetPropertyNames | Base::StructureFlags;

        virtual void visitChildren(JSC::SlotVisitor&);
        virtual JSC::UString className() const;
        virtual bool getOwnPropertySlot(JSC::ExecState*, const JSC::Identifier& propertyName, JSC::PropertySlot&);
        virtual bool getOwnPropertyDescriptor(JSC::ExecState*, const JSC::Identifier& propertyName, JSC::PropertyDescriptor&);
        virtual void put(JSC::ExecState*, const JSC::Identifier& propertyName, JSC::JSValue, JSC::PutPropertySlot&);
        virtual void putWithAttributes(JSC::ExecState*, const JSC::Identifier& propertyName, JSC::JSValue, unsigned attributes);
        virtual bool deleteProperty(JSC::ExecState*, const JSC::Identifier& propertyName);
        virtual void getPropertyNames(JSC::ExecState*, JSC::PropertyNameArray&, JSC::EnumerationMode mode = JSC::ExcludeDontEnumProperties);
        virtual void getOwnPropertyNames(JSC::ExecState*, JSC::PropertyNameArray&, JSC::EnumerationMode mode = JSC::ExcludeDontEnumProperties);
        virtual void defineGetter(JSC::ExecState*, const JSC::Identifier& propertyName, JSC::JSObject* getterFunction, unsigned attributes);
        virtual void defineSetter(JSC::ExecState*, const JSC::Identifier& propertyName, JSC::JSObject* setterFunction, unsigned attributes);
        virtual bool defineOwnProperty(JSC::ExecState*, const JSC::Identifier& propertyName, JSC::PropertyDescriptor&, bool shouldThrow);
        virtual JSC::JSValue lookupGetter(JSC::ExecState*, const JSC::Identifier& propertyName);
        virtual JSC::JSValue lookupSetter(JSC::ExecState*, const JSC::Identifier& propertyName);
        virtual JSC::JSObject* unwrappedObject();

        JSC::WriteBarrier<JSDOMWindow> m_window;
        RefPtr<DOMWrapperWorld> m_world;
    };

    JSC::JSValue toJS(JSC::ExecState*, Frame*);
    JSDOMWindowShell* toJSDOMWindowShell(Frame*, DOMWrapperWorld*);

} // namespace WebCore

#endif // JSDOMWindowShell_h
