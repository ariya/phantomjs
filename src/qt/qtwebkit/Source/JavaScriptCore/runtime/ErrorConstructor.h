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

#ifndef ErrorConstructor_h
#define ErrorConstructor_h

#include "ErrorInstance.h"
#include "InternalFunction.h"

namespace JSC {

    class ErrorPrototype;

    class ErrorConstructor : public InternalFunction {
    public:
        typedef InternalFunction Base;

        static ErrorConstructor* create(ExecState* exec, JSGlobalObject* globalObject, Structure* structure, ErrorPrototype* errorPrototype)
        {
            ErrorConstructor* constructor = new (NotNull, allocateCell<ErrorConstructor>(*exec->heap())) ErrorConstructor(globalObject, structure);
            constructor->finishCreation(exec, errorPrototype);
            return constructor;
        }

        static const ClassInfo s_info;

        static Structure* createStructure(VM& vm, JSGlobalObject* globalObject, JSValue prototype) 
        { 
            return Structure::create(vm, globalObject, prototype, TypeInfo(ObjectType, StructureFlags), &s_info); 
        }

    protected:
        void finishCreation(ExecState*, ErrorPrototype*);
        
    private:
        ErrorConstructor(JSGlobalObject*, Structure*);
        static ConstructType getConstructData(JSCell*, ConstructData&);
        static CallType getCallData(JSCell*, CallData&);
    };

} // namespace JSC

#endif // ErrorConstructor_h
