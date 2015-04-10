/*
 *  Copyright (C) 1999-2000 Harri Porten (porten@kde.org)
 *  Copyright (C) 2003, 2007, 2008 Apple Inc. All Rights Reserved.
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

#ifndef RegExpPrototype_h
#define RegExpPrototype_h

#include "RegExpObject.h"
#include "JSObject.h"

namespace JSC {

    class RegExpPrototype : public RegExpObject {
    public:
        typedef RegExpObject Base;

        static RegExpPrototype* create(ExecState* exec, JSGlobalObject* globalObject, Structure* structure, RegExp* regExp)
        {
            RegExpPrototype* prototype = new (NotNull, allocateCell<RegExpPrototype>(*exec->heap())) RegExpPrototype(globalObject, structure, regExp);
            prototype->finishCreation(globalObject);
            return prototype;
        }
        
        static const ClassInfo s_info;

        static Structure* createStructure(VM& vm, JSGlobalObject* globalObject, JSValue prototype)
        {
            return Structure::create(vm, globalObject, prototype, TypeInfo(ObjectType, StructureFlags), &s_info);
        }

    protected:
        RegExpPrototype(JSGlobalObject*, Structure*, RegExp*);
        static const unsigned StructureFlags = OverridesGetOwnPropertySlot | RegExpObject::StructureFlags;

    private:
        static bool getOwnPropertySlot(JSCell*, ExecState*, PropertyName, PropertySlot&);
        static bool getOwnPropertyDescriptor(JSObject*, ExecState*, PropertyName, PropertyDescriptor&);
    };

} // namespace JSC

#endif // RegExpPrototype_h
