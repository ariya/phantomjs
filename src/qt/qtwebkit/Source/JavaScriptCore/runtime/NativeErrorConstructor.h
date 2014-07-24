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

#ifndef NativeErrorConstructor_h
#define NativeErrorConstructor_h

#include "InternalFunction.h"
#include "NativeErrorPrototype.h"

namespace JSC {

    class ErrorInstance;
    class FunctionPrototype;
    class NativeErrorPrototype;

    class NativeErrorConstructor : public InternalFunction {
    public:
        typedef InternalFunction Base;

        static NativeErrorConstructor* create(ExecState* exec, JSGlobalObject* globalObject, Structure* structure, Structure* prototypeStructure, const String& name)
        {
            NativeErrorConstructor* constructor = new (NotNull, allocateCell<NativeErrorConstructor>(*exec->heap())) NativeErrorConstructor(globalObject, structure);
            constructor->finishCreation(exec, globalObject, prototypeStructure, name);
            return constructor;
        }
        
        static const ClassInfo s_info;

        static Structure* createStructure(VM& vm, JSGlobalObject* globalObject, JSValue prototype)
        {
            return Structure::create(vm, globalObject, prototype, TypeInfo(ObjectType, StructureFlags), &s_info);
        }

        Structure* errorStructure() { return m_errorStructure.get(); }

    protected:
        void finishCreation(ExecState* exec, JSGlobalObject* globalObject, Structure* prototypeStructure, const String& name)
        {
            Base::finishCreation(exec->vm(), name);
            ASSERT(inherits(&s_info));

            NativeErrorPrototype* prototype = NativeErrorPrototype::create(exec, globalObject, prototypeStructure, name, this);

            putDirect(exec->vm(), exec->propertyNames().length, jsNumber(1), DontDelete | ReadOnly | DontEnum); // ECMA 15.11.7.5
            putDirect(exec->vm(), exec->propertyNames().prototype, prototype, DontDelete | ReadOnly | DontEnum);
            m_errorStructure.set(exec->vm(), this, ErrorInstance::createStructure(exec->vm(), globalObject, prototype));
            ASSERT(m_errorStructure);
            ASSERT(m_errorStructure->isObject());
        }

    private:
        NativeErrorConstructor(JSGlobalObject*, Structure*);
        static const unsigned StructureFlags = OverridesVisitChildren | InternalFunction::StructureFlags;
        static ConstructType getConstructData(JSCell*, ConstructData&);
        static CallType getCallData(JSCell*, CallData&);
        static void visitChildren(JSCell*, SlotVisitor&);

        WriteBarrier<Structure> m_errorStructure;
    };

} // namespace JSC

#endif // NativeErrorConstructor_h
