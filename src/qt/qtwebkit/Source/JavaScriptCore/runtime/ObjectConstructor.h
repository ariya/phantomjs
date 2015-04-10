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

#ifndef ObjectConstructor_h
#define ObjectConstructor_h

#include "InternalFunction.h"
#include "JSGlobalObject.h"
#include "ObjectPrototype.h"

namespace JSC {

    class ObjectPrototype;

    class ObjectConstructor : public InternalFunction {
    public:
        typedef InternalFunction Base;

        static ObjectConstructor* create(ExecState* exec, JSGlobalObject* globalObject, Structure* structure, ObjectPrototype* objectPrototype)
        {
            ObjectConstructor* constructor = new (NotNull, allocateCell<ObjectConstructor>(*exec->heap())) ObjectConstructor(globalObject, structure);
            constructor->finishCreation(exec, objectPrototype);
            return constructor;
        }

        static bool getOwnPropertySlot(JSCell*, ExecState*, PropertyName, PropertySlot&);
        static bool getOwnPropertyDescriptor(JSObject*, ExecState*, PropertyName, PropertyDescriptor&);

        static const ClassInfo s_info;

        static Structure* createStructure(VM& vm, JSGlobalObject* globalObject, JSValue prototype)
        {
            return Structure::create(vm, globalObject, prototype, TypeInfo(ObjectType, StructureFlags), &s_info);
        }

    protected:
        void finishCreation(ExecState*, ObjectPrototype*);
        static const unsigned StructureFlags = OverridesGetOwnPropertySlot | InternalFunction::StructureFlags;

    private:
        ObjectConstructor(JSGlobalObject*, Structure*);
        static ConstructType getConstructData(JSCell*, ConstructData&);
        static CallType getCallData(JSCell*, CallData&);
    };

    inline JSObject* constructEmptyObject(ExecState* exec, Structure* structure)
    {
        return JSFinalObject::create(exec, structure);
    }

    inline JSObject* constructEmptyObject(ExecState* exec, JSObject* prototype, unsigned inlineCapacity)
    {
        JSGlobalObject* globalObject = exec->lexicalGlobalObject();
        PrototypeMap& prototypeMap = globalObject->vm().prototypeMap;
        Structure* structure = prototypeMap.emptyObjectStructureForPrototype(
            prototype, inlineCapacity);
        return constructEmptyObject(exec, structure);
    }

    inline JSObject* constructEmptyObject(ExecState* exec, JSObject* prototype)
    {
        return constructEmptyObject(exec, prototype, JSFinalObject::defaultInlineCapacity());
    }

    inline JSObject* constructEmptyObject(ExecState* exec)
    {
        return constructEmptyObject(exec, exec->lexicalGlobalObject()->objectPrototype());
    }

} // namespace JSC

#endif // ObjectConstructor_h
