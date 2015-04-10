/*
 *  Copyright (C) 2008 Apple Inc. All Rights Reserved.
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

#ifndef RegExpMatchesArray_h
#define RegExpMatchesArray_h

#include "JSArray.h"
#include "JSGlobalObject.h"
#include "RegExpObject.h"

namespace JSC {

    class RegExpMatchesArray : public JSArray {
    private:
        RegExpMatchesArray(VM&, Butterfly*, JSGlobalObject*, JSString*, RegExp*, MatchResult);

        enum ReifiedState { ReifiedNone, ReifiedMatch, ReifiedAll };

    public:
        typedef JSArray Base;

        static RegExpMatchesArray* create(ExecState*, JSString*, RegExp*, MatchResult);

        JSString* leftContext(ExecState*);
        JSString* rightContext(ExecState*);

        static const ClassInfo s_info;

        static Structure* createStructure(VM& vm, JSGlobalObject* globalObject, JSValue prototype)
        {
            return Structure::create(vm, globalObject, prototype, TypeInfo(ObjectType, StructureFlags), &s_info, ArrayWithSlowPutArrayStorage);
        }

        static void visitChildren(JSCell*, SlotVisitor&);

    protected:
        void finishCreation(VM&);

        static const unsigned StructureFlags = OverridesGetOwnPropertySlot | OverridesVisitChildren | OverridesGetPropertyNames | Base::StructureFlags;

    private:
        ALWAYS_INLINE void reifyAllPropertiesIfNecessary(ExecState* exec)
        {
            if (m_state != ReifiedAll)
                reifyAllProperties(exec);
        }

        ALWAYS_INLINE void reifyMatchPropertyIfNecessary(ExecState* exec)
        {
            if (m_state == ReifiedNone)
                reifyMatchProperty(exec);
        }

        static bool getOwnPropertySlot(JSCell* cell, ExecState* exec, PropertyName propertyName, PropertySlot& slot)
        {
            RegExpMatchesArray* thisObject = jsCast<RegExpMatchesArray*>(cell);
            thisObject->reifyAllPropertiesIfNecessary(exec);
            return JSArray::getOwnPropertySlot(thisObject, exec, propertyName, slot);
        }

        static bool getOwnPropertySlotByIndex(JSCell* cell, ExecState* exec, unsigned propertyName, PropertySlot& slot)
        {
            RegExpMatchesArray* thisObject = jsCast<RegExpMatchesArray*>(cell);
            if (propertyName)
                thisObject->reifyAllPropertiesIfNecessary(exec);
            else
                thisObject->reifyMatchPropertyIfNecessary(exec);
            return JSArray::getOwnPropertySlotByIndex(thisObject, exec, propertyName, slot);
        }

        static bool getOwnPropertyDescriptor(JSObject* object, ExecState* exec, PropertyName propertyName, PropertyDescriptor& descriptor)
        {
            RegExpMatchesArray* thisObject = jsCast<RegExpMatchesArray*>(object);
            thisObject->reifyAllPropertiesIfNecessary(exec);
            return JSArray::getOwnPropertyDescriptor(thisObject, exec, propertyName, descriptor);
        }

        static void put(JSCell* cell, ExecState* exec, PropertyName propertyName, JSValue v, PutPropertySlot& slot)
        {
            RegExpMatchesArray* thisObject = jsCast<RegExpMatchesArray*>(cell);
            thisObject->reifyAllPropertiesIfNecessary(exec);
            JSArray::put(thisObject, exec, propertyName, v, slot);
        }
        
        static void putByIndex(JSCell* cell, ExecState* exec, unsigned propertyName, JSValue v, bool shouldThrow)
        {
            RegExpMatchesArray* thisObject = jsCast<RegExpMatchesArray*>(cell);
            thisObject->reifyAllPropertiesIfNecessary(exec);
            JSArray::putByIndex(thisObject, exec, propertyName, v, shouldThrow);
        }

        static bool deleteProperty(JSCell* cell, ExecState* exec, PropertyName propertyName)
        {
            RegExpMatchesArray* thisObject = jsCast<RegExpMatchesArray*>(cell);
            thisObject->reifyAllPropertiesIfNecessary(exec);
            return JSArray::deleteProperty(thisObject, exec, propertyName);
        }

        static bool deletePropertyByIndex(JSCell* cell, ExecState* exec, unsigned propertyName)
        {
            RegExpMatchesArray* thisObject = jsCast<RegExpMatchesArray*>(cell);
            thisObject->reifyAllPropertiesIfNecessary(exec);
            return JSArray::deletePropertyByIndex(thisObject, exec, propertyName);
        }

        static void getOwnPropertyNames(JSObject* object, ExecState* exec, PropertyNameArray& arr, EnumerationMode mode = ExcludeDontEnumProperties)
        {
            RegExpMatchesArray* thisObject = jsCast<RegExpMatchesArray*>(object);
            thisObject->reifyAllPropertiesIfNecessary(exec);
            JSArray::getOwnPropertyNames(thisObject, exec, arr, mode);
        }

        static bool defineOwnProperty(JSObject* object, ExecState* exec, PropertyName propertyName, PropertyDescriptor& descriptor, bool shouldThrow)
        {
            RegExpMatchesArray* thisObject = jsCast<RegExpMatchesArray*>(object);
            thisObject->reifyAllPropertiesIfNecessary(exec);
            return JSArray::defineOwnProperty(object, exec, propertyName, descriptor, shouldThrow);
        }

        void reifyAllProperties(ExecState*);
        void reifyMatchProperty(ExecState*);

        WriteBarrier<JSString> m_input;
        WriteBarrier<RegExp> m_regExp;
        MatchResult m_result;
        ReifiedState m_state;
};

}

#endif // RegExpMatchesArray_h
