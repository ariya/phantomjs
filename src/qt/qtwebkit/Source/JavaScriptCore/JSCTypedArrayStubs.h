/*
 * Copyright (C) 2012 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#ifndef JSCTypedArrayStubs_h
#define JSCTypedArrayStubs_h

#include "JSDestructibleObject.h"
#include "ObjectPrototype.h"
#include "Operations.h"
#include <wtf/Float32Array.h>
#include <wtf/Float64Array.h>
#include <wtf/Forward.h>
#include <wtf/Int16Array.h>
#include <wtf/Int32Array.h>
#include <wtf/Int8Array.h>
#include <wtf/Uint16Array.h>
#include <wtf/Uint32Array.h>
#include <wtf/Uint8Array.h>
#include <wtf/Uint8ClampedArray.h>

namespace JSC {
    
#define TYPED_ARRAY(name, type) \
class JS##name##Array : public JSDestructibleObject { \
public: \
    typedef JSDestructibleObject Base; \
    static JS##name##Array* create(JSC::Structure* structure, JSGlobalObject* globalObject, PassRefPtr<name##Array> impl) \
    { \
        JS##name##Array* ptr = new (NotNull, JSC::allocateCell<JS##name##Array>(globalObject->vm().heap)) JS##name##Array(structure, globalObject, impl); \
        ptr->finishCreation(globalObject->vm()); \
        return ptr; \
    }\
\
    static bool getOwnPropertySlot(JSC::JSCell*, JSC::ExecState*, JSC::PropertyName propertyName, JSC::PropertySlot&);\
    static bool getOwnPropertyDescriptor(JSC::JSObject*, JSC::ExecState*, JSC::PropertyName propertyName, JSC::PropertyDescriptor&);\
    static bool getOwnPropertySlotByIndex(JSC::JSCell*, JSC::ExecState*, unsigned propertyName, JSC::PropertySlot&);\
    static void put(JSC::JSCell*, JSC::ExecState*, JSC::PropertyName propertyName, JSC::JSValue, JSC::PutPropertySlot&);\
    static void putByIndex(JSC::JSCell*, JSC::ExecState*, unsigned propertyName, JSC::JSValue, bool);\
    static const JSC::ClassInfo s_info;\
\
    static JSC::Structure* createStructure(JSC::VM& vm, JSC::JSGlobalObject* globalObject, JSC::JSValue prototype)\
    {\
        return JSC::Structure::create(vm, globalObject, prototype, JSC::TypeInfo(JSC::ObjectType, StructureFlags), &s_info);\
    }\
\
    static void getOwnPropertyNames(JSC::JSObject*, JSC::ExecState*, JSC::PropertyNameArray&, JSC::EnumerationMode mode = JSC::ExcludeDontEnumProperties);\
    static JSC::JSValue getConstructor(JSC::ExecState*, JSC::JSGlobalObject*);\
\
    static const JSC::TypedArrayType TypedArrayStorageType = JSC::TypedArray##name;\
    uint32_t m_storageLength;\
    type* m_storage;\
    RefPtr<name##Array> m_impl;\
protected:\
    JS##name##Array(JSC::Structure*, JSGlobalObject*, PassRefPtr<name##Array>);\
    void finishCreation(JSC::VM&);\
    static const unsigned StructureFlags = JSC::OverridesGetPropertyNames | JSC::InterceptsGetOwnPropertySlotByIndexEvenWhenLengthIsNotZero | JSC::OverridesGetOwnPropertySlot | Base::StructureFlags; \
    JSC::JSValue getByIndex(JSC::ExecState*, unsigned index);\
    void indexSetter(JSC::ExecState*, unsigned index, JSC::JSValue);\
};\
\
const ClassInfo JS##name##Array::s_info = { #name "Array" , &Base::s_info, 0, 0, CREATE_METHOD_TABLE(JS##name##Array) };\
\
JS##name##Array::JS##name##Array(Structure* structure, JSGlobalObject* globalObject, PassRefPtr<name##Array> impl)\
    : Base(globalObject->vm(), structure)\
    , m_impl(impl)\
{\
}\
\
void JS##name##Array::finishCreation(VM& vm)\
{\
    Base::finishCreation(vm);\
    TypedArrayDescriptor descriptor(&JS##name##Array::s_info, OBJECT_OFFSETOF(JS##name##Array, m_storage), OBJECT_OFFSETOF(JS##name##Array, m_storageLength));\
    vm.registerTypedArrayDescriptor(m_impl.get(), descriptor);\
    m_storage = m_impl->data();\
    m_storageLength = m_impl->length();\
    putDirect(vm, vm.propertyNames->length, jsNumber(m_storageLength), DontDelete | ReadOnly | DontEnum); \
    ASSERT(inherits(&s_info));\
}\
\
bool JS##name##Array::getOwnPropertySlot(JSCell* cell, ExecState* exec, PropertyName propertyName, PropertySlot& slot)\
{\
    JS##name##Array* thisObject = jsCast<JS##name##Array*>(cell);\
    ASSERT_GC_OBJECT_INHERITS(thisObject, &s_info);\
    unsigned index = propertyName.asIndex();\
    if (index < thisObject->m_storageLength) {\
        ASSERT(index != PropertyName::NotAnIndex);\
        slot.setValue(thisObject->getByIndex(exec, index));\
        return true;\
    }\
    return Base::getOwnPropertySlot(cell, exec, propertyName, slot);\
}\
\
bool JS##name##Array::getOwnPropertyDescriptor(JSObject* object, ExecState* exec, PropertyName propertyName, PropertyDescriptor& descriptor)\
{\
    JS##name##Array* thisObject = jsCast<JS##name##Array*>(object);\
    ASSERT_GC_OBJECT_INHERITS(thisObject, &s_info);\
    unsigned index = propertyName.asIndex();\
    if (index < thisObject->m_storageLength) {\
        ASSERT(index != PropertyName::NotAnIndex);\
        descriptor.setDescriptor(thisObject->getByIndex(exec, index), DontDelete);\
        return true;\
    }\
    return Base::getOwnPropertyDescriptor(object, exec, propertyName, descriptor);\
}\
\
bool JS##name##Array::getOwnPropertySlotByIndex(JSCell* cell, ExecState* exec, unsigned propertyName, PropertySlot& slot)\
{\
    JS##name##Array* thisObject = jsCast<JS##name##Array*>(cell);\
    ASSERT_GC_OBJECT_INHERITS(thisObject, &s_info);\
    if (propertyName < thisObject->m_storageLength) {\
        slot.setValue(thisObject->getByIndex(exec, propertyName));\
        return true;\
    }\
    return thisObject->methodTable()->getOwnPropertySlot(thisObject, exec, Identifier::from(exec, propertyName), slot);\
}\
\
void JS##name##Array::put(JSCell* cell, ExecState* exec, PropertyName propertyName, JSValue value, PutPropertySlot& slot)\
{\
    JS##name##Array* thisObject = jsCast<JS##name##Array*>(cell);\
    ASSERT_GC_OBJECT_INHERITS(thisObject, &s_info);\
    unsigned index = propertyName.asIndex();\
    if (index != PropertyName::NotAnIndex) {\
        thisObject->indexSetter(exec, index, value);\
        return;\
    }\
    Base::put(thisObject, exec, propertyName, value, slot);\
}\
\
void JS##name##Array::indexSetter(JSC::ExecState* exec, unsigned index, JSC::JSValue value) \
{\
    m_impl->set(index, value.toNumber(exec));\
}\
\
void JS##name##Array::putByIndex(JSCell* cell, ExecState* exec, unsigned propertyName, JSValue value, bool)\
{\
    JS##name##Array* thisObject = jsCast<JS##name##Array*>(cell);\
    ASSERT_GC_OBJECT_INHERITS(thisObject, &s_info);\
    thisObject->indexSetter(exec, propertyName, value);\
    return;\
}\
\
void JS##name##Array::getOwnPropertyNames(JSObject* object, ExecState* exec, PropertyNameArray& propertyNames, EnumerationMode mode)\
{\
    JS##name##Array* thisObject = jsCast<JS##name##Array*>(object);\
    ASSERT_GC_OBJECT_INHERITS(thisObject, &s_info);\
    for (unsigned i = 0; i < thisObject->m_storageLength; ++i)\
        propertyNames.add(Identifier::from(exec, i));\
    Base::getOwnPropertyNames(thisObject, exec, propertyNames, mode);\
}\
\
JSValue JS##name##Array::getByIndex(ExecState*, unsigned index)\
{\
    ASSERT_GC_OBJECT_INHERITS(this, &s_info);\
    type result = m_impl->item(index);\
    if (std::isnan((double)result))\
        return jsNaN();\
    return JSValue(result);\
}\
static EncodedJSValue JSC_HOST_CALL constructJS##name##Array(ExecState* callFrame) { \
    if (callFrame->argumentCount() < 1) \
        return JSValue::encode(jsUndefined()); \
    int32_t length = callFrame->argument(0).toInt32(callFrame); \
    if (length < 0) \
        return JSValue::encode(jsUndefined()); \
    Structure* structure = JS##name##Array::createStructure(callFrame->vm(), callFrame->lexicalGlobalObject(), callFrame->lexicalGlobalObject()->objectPrototype()); \
    RefPtr<name##Array> buffer = name##Array::create(length); \
    if (!buffer) \
        return throwVMError(callFrame, createRangeError(callFrame, "ArrayBuffer size is not a small enough positive integer.")); \
    return JSValue::encode(JS##name##Array::create(structure, callFrame->lexicalGlobalObject(), buffer.release())); \
}

TYPED_ARRAY(Uint8, uint8_t);
TYPED_ARRAY(Uint8Clamped, uint8_t);
TYPED_ARRAY(Uint16, uint16_t);
TYPED_ARRAY(Uint32, uint32_t);
TYPED_ARRAY(Int8, int8_t);
TYPED_ARRAY(Int16, int16_t);
TYPED_ARRAY(Int32, int32_t);
TYPED_ARRAY(Float32, float);
TYPED_ARRAY(Float64, double);

}

#endif
