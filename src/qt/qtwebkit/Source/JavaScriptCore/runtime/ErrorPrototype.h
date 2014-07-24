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

#ifndef ErrorPrototype_h
#define ErrorPrototype_h

#include "ErrorInstance.h"

namespace JSC {

    class ObjectPrototype;

    class ErrorPrototype : public ErrorInstance {
    public:
        typedef ErrorInstance Base;

        static ErrorPrototype* create(ExecState* exec, JSGlobalObject* globalObject, Structure* structure)
        {
            ErrorPrototype* prototype = new (NotNull, allocateCell<ErrorPrototype>(*exec->heap())) ErrorPrototype(exec, structure);
            prototype->finishCreation(exec, globalObject);
            return prototype;
        }
        
        static const ClassInfo s_info;

        static Structure* createStructure(VM& vm, JSGlobalObject* globalObject, JSValue prototype)
        {
            return Structure::create(vm, globalObject, prototype, TypeInfo(ErrorInstanceType, StructureFlags), &s_info);
        }

    protected:
        ErrorPrototype(ExecState*, Structure*);
        void finishCreation(ExecState*, JSGlobalObject*);

        static const unsigned StructureFlags = OverridesGetOwnPropertySlot | ErrorInstance::StructureFlags;

    private:
        static bool getOwnPropertySlot(JSCell*, ExecState*, PropertyName, PropertySlot&);
        static bool getOwnPropertyDescriptor(JSObject*, ExecState*, PropertyName, PropertyDescriptor&);
    };

} // namespace JSC

#endif // ErrorPrototype_h
