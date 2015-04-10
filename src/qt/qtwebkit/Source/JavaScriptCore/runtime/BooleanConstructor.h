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

#ifndef BooleanConstructor_h
#define BooleanConstructor_h

#include "InternalFunction.h"

namespace JSC {

class BooleanPrototype;

class BooleanConstructor : public InternalFunction {
public:
    typedef InternalFunction Base;

    static BooleanConstructor* create(ExecState* exec, JSGlobalObject* globalObject, Structure* structure, BooleanPrototype* booleanPrototype)
    {
        BooleanConstructor* constructor = new (NotNull, allocateCell<BooleanConstructor>(*exec->heap())) BooleanConstructor(globalObject, structure);
        constructor->finishCreation(exec, booleanPrototype);
        return constructor;
    }

    static const ClassInfo s_info;

    static Structure* createStructure(VM& vm, JSGlobalObject* globalObject, JSValue prototype) 
    { 
        return Structure::create(vm, globalObject, prototype, TypeInfo(ObjectType, StructureFlags), &s_info); 
    }

protected:
    void finishCreation(ExecState*, BooleanPrototype*);

private:
    BooleanConstructor(JSGlobalObject*, Structure*);
    static ConstructType getConstructData(JSCell*, ConstructData&);
    static CallType getCallData(JSCell*, CallData&);
};

JSObject* constructBooleanFromImmediateBoolean(ExecState*, JSGlobalObject*, JSValue);
JSObject* constructBoolean(ExecState*, const ArgList&);

} // namespace JSC

#endif // BooleanConstructor_h
