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
#include <runtime/JSProxy.h>

namespace WebCore {

    class DOMWindow;
    class Frame;

    class JSDOMWindowShell : public JSC::JSProxy {
        typedef JSC::JSProxy Base;
    public:
        JSDOMWindowShell(PassRefPtr<DOMWindow>, JSC::Structure*, DOMWrapperWorld*);
        static void destroy(JSCell*);

        JSDOMWindow* window() const { return JSC::jsCast<JSDOMWindow*>(target()); }
        void setWindow(JSC::VM&, JSDOMWindow*);
        void setWindow(PassRefPtr<DOMWindow>);

        static const JSC::ClassInfo s_info;

        DOMWindow* impl() const;

        static JSDOMWindowShell* create(PassRefPtr<DOMWindow> window, JSC::Structure* structure, DOMWrapperWorld* world) 
        {
            JSC::Heap& heap = JSDOMWindow::commonVM()->heap;
            JSDOMWindowShell* shell = new (NotNull, JSC::allocateCell<JSDOMWindowShell>(heap)) JSDOMWindowShell(structure, world);
            shell->finishCreation(*world->vm(), window);
            return shell; 
        }

        static JSC::Structure* createStructure(JSC::VM& vm, JSC::JSValue prototype) 
        {
            return JSC::Structure::create(vm, 0, prototype, JSC::TypeInfo(JSC::ProxyType, StructureFlags), &s_info);
        }

        DOMWrapperWorld* world() { return m_world.get(); }

    protected:
        JSDOMWindowShell(JSC::Structure*, DOMWrapperWorld*);
        void finishCreation(JSC::VM&, PassRefPtr<DOMWindow>);

        RefPtr<DOMWrapperWorld> m_world;
    };

    JSC::JSValue toJS(JSC::ExecState*, Frame*);
    JSDOMWindowShell* toJSDOMWindowShell(Frame*, DOMWrapperWorld*);

} // namespace WebCore

#endif // JSDOMWindowShell_h
