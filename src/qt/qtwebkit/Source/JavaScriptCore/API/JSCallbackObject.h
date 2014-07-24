/*
 * Copyright (C) 2006, 2007, 2008, 2010 Apple Inc. All rights reserved.
 * Copyright (C) 2007 Eric Seidel <eric@webkit.org>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#ifndef JSCallbackObject_h
#define JSCallbackObject_h

#include "JSObjectRef.h"
#include "JSValueRef.h"
#include "JSObject.h"
#include <wtf/PassOwnPtr.h>

namespace JSC {

struct JSCallbackObjectData : WeakHandleOwner {
    JSCallbackObjectData(void* privateData, JSClassRef jsClass)
        : privateData(privateData)
        , jsClass(jsClass)
    {
        JSClassRetain(jsClass);
    }
    
    ~JSCallbackObjectData()
    {
        JSClassRelease(jsClass);
    }
    
    JSValue getPrivateProperty(const Identifier& propertyName) const
    {
        if (!m_privateProperties)
            return JSValue();
        return m_privateProperties->getPrivateProperty(propertyName);
    }
    
    void setPrivateProperty(VM& vm, JSCell* owner, const Identifier& propertyName, JSValue value)
    {
        if (!m_privateProperties)
            m_privateProperties = adoptPtr(new JSPrivatePropertyMap);
        m_privateProperties->setPrivateProperty(vm, owner, propertyName, value);
    }
    
    void deletePrivateProperty(const Identifier& propertyName)
    {
        if (!m_privateProperties)
            return;
        m_privateProperties->deletePrivateProperty(propertyName);
    }

    void visitChildren(SlotVisitor& visitor)
    {
        if (!m_privateProperties)
            return;
        m_privateProperties->visitChildren(visitor);
    }

    void* privateData;
    JSClassRef jsClass;
    struct JSPrivatePropertyMap {
        JSValue getPrivateProperty(const Identifier& propertyName) const
        {
            PrivatePropertyMap::const_iterator location = m_propertyMap.find(propertyName.impl());
            if (location == m_propertyMap.end())
                return JSValue();
            return location->value.get();
        }
        
        void setPrivateProperty(VM& vm, JSCell* owner, const Identifier& propertyName, JSValue value)
        {
            WriteBarrier<Unknown> empty;
            m_propertyMap.add(propertyName.impl(), empty).iterator->value.set(vm, owner, value);
        }
        
        void deletePrivateProperty(const Identifier& propertyName)
        {
            m_propertyMap.remove(propertyName.impl());
        }

        void visitChildren(SlotVisitor& visitor)
        {
            for (PrivatePropertyMap::iterator ptr = m_propertyMap.begin(); ptr != m_propertyMap.end(); ++ptr) {
                if (ptr->value)
                    visitor.append(&ptr->value);
            }
        }

    private:
        typedef HashMap<RefPtr<StringImpl>, WriteBarrier<Unknown>, IdentifierRepHash> PrivatePropertyMap;
        PrivatePropertyMap m_propertyMap;
    };
    OwnPtr<JSPrivatePropertyMap> m_privateProperties;
    virtual void finalize(Handle<Unknown>, void*);
};

    
template <class Parent>
class JSCallbackObject : public Parent {
protected:
    JSCallbackObject(ExecState*, Structure*, JSClassRef, void* data);
    JSCallbackObject(VM&, JSClassRef, Structure*);

    void finishCreation(ExecState*);
    void finishCreation(VM&);

public:
    typedef Parent Base;

    static JSCallbackObject* create(ExecState* exec, JSGlobalObject* globalObject, Structure* structure, JSClassRef classRef, void* data)
    {
        ASSERT_UNUSED(globalObject, !structure->globalObject() || structure->globalObject() == globalObject);
        JSCallbackObject* callbackObject = new (NotNull, allocateCell<JSCallbackObject>(*exec->heap())) JSCallbackObject(exec, structure, classRef, data);
        callbackObject->finishCreation(exec);
        return callbackObject;
    }
    static JSCallbackObject<Parent>* create(VM&, JSClassRef, Structure*);

    static const bool needsDestruction;
    static void destroy(JSCell* cell)
    {
        static_cast<JSCallbackObject*>(cell)->JSCallbackObject::~JSCallbackObject();
    }

    void setPrivate(void* data);
    void* getPrivate();

    static const ClassInfo s_info;

    JSClassRef classRef() const { return m_callbackObjectData->jsClass; }
    bool inherits(JSClassRef) const;

    static Structure* createStructure(VM&, JSGlobalObject*, JSValue);
    
    JSValue getPrivateProperty(const Identifier& propertyName) const
    {
        return m_callbackObjectData->getPrivateProperty(propertyName);
    }
    
    void setPrivateProperty(VM& vm, const Identifier& propertyName, JSValue value)
    {
        m_callbackObjectData->setPrivateProperty(vm, this, propertyName, value);
    }
    
    void deletePrivateProperty(const Identifier& propertyName)
    {
        m_callbackObjectData->deletePrivateProperty(propertyName);
    }

    using Parent::methodTable;

protected:
    static const unsigned StructureFlags = ProhibitsPropertyCaching | OverridesGetOwnPropertySlot | InterceptsGetOwnPropertySlotByIndexEvenWhenLengthIsNotZero | ImplementsHasInstance | OverridesHasInstance | OverridesVisitChildren | OverridesGetPropertyNames | Parent::StructureFlags;

private:
    static String className(const JSObject*);

    static JSValue defaultValue(const JSObject*, ExecState*, PreferredPrimitiveType);

    static bool getOwnPropertySlot(JSCell*, ExecState*, PropertyName, PropertySlot&);
    static bool getOwnPropertySlotByIndex(JSCell*, ExecState*, unsigned propertyName, PropertySlot&);
    static bool getOwnPropertyDescriptor(JSObject*, ExecState*, PropertyName, PropertyDescriptor&);
    
    static void put(JSCell*, ExecState*, PropertyName, JSValue, PutPropertySlot&);
    static void putByIndex(JSCell*, ExecState*, unsigned, JSValue, bool shouldThrow);

    static bool deleteProperty(JSCell*, ExecState*, PropertyName);
    static bool deletePropertyByIndex(JSCell*, ExecState*, unsigned);

    static bool customHasInstance(JSObject*, ExecState*, JSValue);

    static void getOwnNonIndexPropertyNames(JSObject*, ExecState*, PropertyNameArray&, EnumerationMode);

    static ConstructType getConstructData(JSCell*, ConstructData&);
    static CallType getCallData(JSCell*, CallData&);

    static void visitChildren(JSCell* cell, SlotVisitor& visitor)
    {
        JSCallbackObject* thisObject = jsCast<JSCallbackObject*>(cell);
        ASSERT_GC_OBJECT_INHERITS((static_cast<Parent*>(thisObject)), &JSCallbackObject<Parent>::s_info);
        COMPILE_ASSERT(StructureFlags & OverridesVisitChildren, OverridesVisitChildrenWithoutSettingFlag);
        ASSERT(thisObject->Parent::structure()->typeInfo().overridesVisitChildren());
        Parent::visitChildren(thisObject, visitor);
        thisObject->m_callbackObjectData->visitChildren(visitor);
    }

    void init(ExecState*);
 
    static JSCallbackObject* asCallbackObject(JSValue);
 
    static EncodedJSValue JSC_HOST_CALL call(ExecState*);
    static EncodedJSValue JSC_HOST_CALL construct(ExecState*);
   
    JSValue getStaticValue(ExecState*, PropertyName);
    static JSValue staticFunctionGetter(ExecState*, JSValue, PropertyName);
    static JSValue callbackGetter(ExecState*, JSValue, PropertyName);

    OwnPtr<JSCallbackObjectData> m_callbackObjectData;
};

} // namespace JSC

// include the actual template class implementation
#include "JSCallbackObjectFunctions.h"

#endif // JSCallbackObject_h
