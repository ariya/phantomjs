/*
 *  Copyright (C) 2000 Harri Porten (porten@kde.org)
 *  Copyright (C) 2003, 2004, 2005, 2006, 2007, 2008, 2009 Apple Inc. All rights reseved.
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

#ifndef ScheduledAction_h
#define ScheduledAction_h

#include "JSDOMBinding.h"
#include <heap/Strong.h>
#include <heap/StrongInlines.h>
#include <runtime/JSCell.h>
#include <wtf/PassOwnPtr.h>
#include <wtf/Vector.h>
#include <wtf/text/WTFString.h>

namespace JSC {
    class JSGlobalObject;
}

namespace WebCore {

    class Document;
    class ContentSecurityPolicy;
    class ScriptExecutionContext;
    class WorkerGlobalScope;

   /* An action (either function or string) to be executed after a specified
    * time interval, either once or repeatedly. Used for window.setTimeout()
    * and window.setInterval()
    */
    class ScheduledAction {
        WTF_MAKE_NONCOPYABLE(ScheduledAction); WTF_MAKE_FAST_ALLOCATED;
    public:
        static PassOwnPtr<ScheduledAction> create(JSC::ExecState*, DOMWrapperWorld* isolatedWorld, ContentSecurityPolicy*);

        void execute(ScriptExecutionContext*);

    private:
        ScheduledAction(JSC::ExecState*, JSC::JSValue function, DOMWrapperWorld* isolatedWorld);
        ScheduledAction(const String& code, DOMWrapperWorld* isolatedWorld)
            : m_function(*isolatedWorld->vm())
            , m_code(code)
            , m_isolatedWorld(isolatedWorld)
        {
        }

        void executeFunctionInContext(JSC::JSGlobalObject*, JSC::JSValue thisValue, ScriptExecutionContext*);
        void execute(Document*);
#if ENABLE(WORKERS)
        void execute(WorkerGlobalScope*);
#endif

        JSC::Strong<JSC::Unknown> m_function;
        Vector<JSC::Strong<JSC::Unknown> > m_args;
        String m_code;
        RefPtr<DOMWrapperWorld> m_isolatedWorld;
    };

} // namespace WebCore

#endif // ScheduledAction_h
