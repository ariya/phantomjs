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

#ifndef NativeErrorPrototype_h
#define NativeErrorPrototype_h

#include "ErrorPrototype.h"

namespace JSC {
    class NativeErrorConstructor;

    class NativeErrorPrototype : public ErrorPrototype {
    private:
        NativeErrorPrototype(ExecState*, Structure*);
    
    public:
        typedef ErrorPrototype Base;

        static NativeErrorPrototype* create(ExecState* exec, JSGlobalObject* globalObject, Structure* structure, const String& name, NativeErrorConstructor* constructor)
        {
            NativeErrorPrototype* prototype = new (NotNull, allocateCell<NativeErrorPrototype>(*exec->heap())) NativeErrorPrototype(exec, structure);
            prototype->finishCreation(exec, globalObject, name, constructor);
            return prototype;
        }

    protected:
        void finishCreation(ExecState*, JSGlobalObject*, const String& nameAndMessage, NativeErrorConstructor*);
    };

} // namespace JSC

#endif // NativeErrorPrototype_h
