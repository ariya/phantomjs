/*
 *  Copyright (C) 2006 Maks Orlovich
 *  Copyright (C) 2006, 2007, 2008, 2009 Apple Inc. All rights reserved.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301, USA.
 *
 */

#ifndef JSWrapperObject_h
#define JSWrapperObject_h

#include "JSDestructibleObject.h"

namespace JSC {

    // This class is used as a base for classes such as String,
    // Number, Boolean and Date which are wrappers for primitive types.
    class JSWrapperObject : public JSDestructibleObject {
    public:
        typedef JSDestructibleObject Base;

        static size_t allocationSize(size_t inlineCapacity)
        {
            ASSERT_UNUSED(inlineCapacity, !inlineCapacity);
            return sizeof(JSWrapperObject);
        }

        JSValue internalValue() const;
        void setInternalValue(VM&, JSValue);

        static Structure* createStructure(VM& vm, JSGlobalObject* globalObject, JSValue prototype) 
        { 
            return Structure::create(vm, globalObject, prototype, TypeInfo(ObjectType, StructureFlags), &s_info);
        }
        
        static ptrdiff_t internalValueOffset() { return OBJECT_OFFSETOF(JSWrapperObject, m_internalValue); }
        static ptrdiff_t internalValueCellOffset()
        {
#if USE(JSVALUE64)
            return internalValueOffset();
#else
            return internalValueOffset() + OBJECT_OFFSETOF(EncodedValueDescriptor, asBits.payload);
#endif
        }

    protected:
        explicit JSWrapperObject(VM&, Structure*);
        static const unsigned StructureFlags = OverridesVisitChildren | Base::StructureFlags;

        static void visitChildren(JSCell*, SlotVisitor&);

    private:
        WriteBarrier<Unknown> m_internalValue;
    };

    inline JSWrapperObject::JSWrapperObject(VM& vm, Structure* structure)
        : JSDestructibleObject(vm, structure)
    {
    }

    inline JSValue JSWrapperObject::internalValue() const
    {
        return m_internalValue.get();
    }

    inline void JSWrapperObject::setInternalValue(VM& vm, JSValue value)
    {
        ASSERT(value);
        ASSERT(!value.isObject());
        m_internalValue.set(vm, this, value);
    }

} // namespace JSC

#endif // JSWrapperObject_h
