/*
 *  Copyright (C) 1999-2000 Harri Porten (porten@kde.org)
 *  Copyright (C) 2003, 2007, 2008, 2012 Apple Inc. All Rights Reserved.
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

#ifndef RegExpObject_h
#define RegExpObject_h

#include "JSObject.h"
#include "RegExp.h"

namespace JSC {
    
    class RegExpObject : public JSNonFinalObject {
    public:
        typedef JSNonFinalObject Base;

        static RegExpObject* create(ExecState* exec, JSGlobalObject* globalObject, Structure* structure, RegExp* regExp)
        {
            RegExpObject* object = new (NotNull, allocateCell<RegExpObject>(*exec->heap())) RegExpObject(globalObject, structure, regExp);
            object->finishCreation(globalObject);
            return object;
        }
        
        static RegExpObject* create(VM& vm, JSGlobalObject* globalObject, Structure* structure, RegExp* regExp)
        {
            RegExpObject* object = new (NotNull, allocateCell<RegExpObject>(vm.heap)) RegExpObject(globalObject, structure, regExp);
            object->finishCreation(globalObject);
            return object;
        }

        void setRegExp(VM& vm, RegExp* r) { m_regExp.set(vm, this, r); }
        RegExp* regExp() const { return m_regExp.get(); }

        void setLastIndex(ExecState* exec, size_t lastIndex)
        {
            m_lastIndex.setWithoutWriteBarrier(jsNumber(lastIndex));
            if (LIKELY(m_lastIndexIsWritable))
                m_lastIndex.setWithoutWriteBarrier(jsNumber(lastIndex));
            else
                throwTypeError(exec, StrictModeReadonlyPropertyWriteError);
        }
        void setLastIndex(ExecState* exec, JSValue lastIndex, bool shouldThrow)
        {
            if (LIKELY(m_lastIndexIsWritable))
                m_lastIndex.set(exec->vm(), this, lastIndex);
            else if (shouldThrow)
                throwTypeError(exec, StrictModeReadonlyPropertyWriteError);
        }
        JSValue getLastIndex() const
        {
            return m_lastIndex.get();
        }

        bool test(ExecState* exec, JSString* string) { return match(exec, string); }
        JSValue exec(ExecState*, JSString*);

        static bool getOwnPropertySlot(JSCell*, ExecState*, PropertyName, PropertySlot&);
        static bool getOwnPropertyDescriptor(JSObject*, ExecState*, PropertyName, PropertyDescriptor&);
        static void put(JSCell*, ExecState*, PropertyName, JSValue, PutPropertySlot&);

        static JS_EXPORTDATA const ClassInfo s_info;

        static Structure* createStructure(VM& vm, JSGlobalObject* globalObject, JSValue prototype)
        {
            return Structure::create(vm, globalObject, prototype, TypeInfo(ObjectType, StructureFlags), &s_info);
        }

    protected:
        JS_EXPORT_PRIVATE RegExpObject(JSGlobalObject*, Structure*, RegExp*);
        JS_EXPORT_PRIVATE void finishCreation(JSGlobalObject*);

        static const unsigned StructureFlags = OverridesVisitChildren | OverridesGetOwnPropertySlot | Base::StructureFlags;

        static void visitChildren(JSCell*, SlotVisitor&);

        JS_EXPORT_PRIVATE static bool deleteProperty(JSCell*, ExecState*, PropertyName);
        JS_EXPORT_PRIVATE static void getOwnNonIndexPropertyNames(JSObject*, ExecState*, PropertyNameArray&, EnumerationMode);
        JS_EXPORT_PRIVATE static void getPropertyNames(JSObject*, ExecState*, PropertyNameArray&, EnumerationMode);
        JS_EXPORT_PRIVATE static bool defineOwnProperty(JSObject*, ExecState*, PropertyName, PropertyDescriptor&, bool shouldThrow);

    private:
        MatchResult match(ExecState*, JSString*);

        WriteBarrier<RegExp> m_regExp;
        WriteBarrier<Unknown> m_lastIndex;
        bool m_lastIndexIsWritable;
    };

    RegExpObject* asRegExpObject(JSValue);

    inline RegExpObject* asRegExpObject(JSValue value)
    {
        ASSERT(asObject(value)->inherits(&RegExpObject::s_info));
        return static_cast<RegExpObject*>(asObject(value));
    }

} // namespace JSC

#endif // RegExpObject_h
