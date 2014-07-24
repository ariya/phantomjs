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

#ifndef BooleanObject_h
#define BooleanObject_h

#include "JSWrapperObject.h"

namespace JSC {

class BooleanObject : public JSWrapperObject {
protected:
    JS_EXPORT_PRIVATE BooleanObject(VM&, Structure*);
    JS_EXPORT_PRIVATE void finishCreation(VM&);

public:
    typedef JSWrapperObject Base;

    static BooleanObject* create(VM& vm, Structure* structure)
    {
        BooleanObject* boolean = new (NotNull, allocateCell<BooleanObject>(vm.heap)) BooleanObject(vm, structure);
        boolean->finishCreation(vm);
        return boolean;
    }
        
    static JS_EXPORTDATA const ClassInfo s_info;
        
    static Structure* createStructure(VM& vm, JSGlobalObject* globalObject, JSValue prototype)
    {
        return Structure::create(vm, globalObject, prototype, TypeInfo(ObjectType, StructureFlags), &s_info);
    }
};

BooleanObject* asBooleanObject(JSValue);

inline BooleanObject* asBooleanObject(JSValue value)
{
    ASSERT(asObject(value)->inherits(&BooleanObject::s_info));
    return static_cast<BooleanObject*>(asObject(value));
}

} // namespace JSC

#endif // BooleanObject_h
