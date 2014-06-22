/*
 *  Copyright (C) 1999-2000 Harri Porten (porten@kde.org)
 *  Copyright (C) 2008 Apple Inc. All rights reserved.
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
 *
 */

#ifndef ErrorInstance_h
#define ErrorInstance_h

#include "JSObject.h"

namespace JSC {

    class ErrorInstance : public JSNonFinalObject {
    public:
        typedef JSNonFinalObject Base;

        static const ClassInfo s_info;

        static Structure* createStructure(VM& vm, JSGlobalObject* globalObject, JSValue prototype)
        {
            return Structure::create(vm, globalObject, prototype, TypeInfo(ErrorInstanceType, StructureFlags), &s_info);
        }

        static ErrorInstance* create(VM& vm, Structure* structure, const String& message)
        {
            ErrorInstance* instance = new (NotNull, allocateCell<ErrorInstance>(vm.heap)) ErrorInstance(vm, structure);
            instance->finishCreation(vm, message);
            return instance;
        }

        static ErrorInstance* create(ExecState* exec, Structure* structure, JSValue message)
        {
            return create(exec->vm(), structure, message.isUndefined() ? String() : message.toString(exec)->value(exec));
        }

        bool appendSourceToMessage() { return m_appendSourceToMessage; }
        void setAppendSourceToMessage() { m_appendSourceToMessage = true; }
        void clearAppendSourceToMessage() { m_appendSourceToMessage = false; }

    protected:
        explicit ErrorInstance(VM&, Structure*);

        void finishCreation(VM& vm, const String& message)
        {
            Base::finishCreation(vm);
            ASSERT(inherits(&s_info));
            if (!message.isNull())
                putDirect(vm, vm.propertyNames->message, jsString(&vm, message), DontEnum);
        }

        bool m_appendSourceToMessage;
    };

} // namespace JSC

#endif // ErrorInstance_h
