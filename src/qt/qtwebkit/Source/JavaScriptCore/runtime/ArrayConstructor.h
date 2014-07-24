/*
 *  Copyright (C) 1999-2000 Harri Porten (porten@kde.org)
 *  Copyright (C) 2007, 2008, 2011 Apple Inc. All rights reserved.
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

#ifndef ArrayConstructor_h
#define ArrayConstructor_h

#include "InternalFunction.h"

namespace JSC {

class ArrayAllocationProfile;
class ArrayPrototype;
class JSArray;

class ArrayConstructor : public InternalFunction {
public:
    typedef InternalFunction Base;

    static ArrayConstructor* create(ExecState* exec, JSGlobalObject* globalObject, Structure* structure, ArrayPrototype* arrayPrototype)
    {
        ArrayConstructor* constructor = new (NotNull, allocateCell<ArrayConstructor>(*exec->heap())) ArrayConstructor(globalObject, structure);
        constructor->finishCreation(exec, arrayPrototype);
        return constructor;
    }

    static const ClassInfo s_info;

    static Structure* createStructure(VM& vm, JSGlobalObject* globalObject, JSValue prototype)
    {
        return Structure::create(vm, globalObject, prototype, TypeInfo(ObjectType, StructureFlags), &s_info);
    }

protected:
    void finishCreation(ExecState*, ArrayPrototype*);
    static const unsigned StructureFlags = OverridesGetOwnPropertySlot | InternalFunction::StructureFlags;

private:
    ArrayConstructor(JSGlobalObject*, Structure*);
    static bool getOwnPropertySlot(JSCell*, ExecState*, PropertyName, PropertySlot&);

    static bool getOwnPropertyDescriptor(JSObject*, ExecState*, PropertyName, PropertyDescriptor&);

    static ConstructType getConstructData(JSCell*, ConstructData&);
    static CallType getCallData(JSCell*, CallData&);
};

JSObject* constructArrayWithSizeQuirk(ExecState*, ArrayAllocationProfile*, JSGlobalObject*, JSValue);

} // namespace JSC

#endif // ArrayConstructor_h
