/*
 * Copyright (C) 2011, 2012 Apple Inc. All rights reserved.
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

#ifndef JSValueInlines_h
#define JSValueInlines_h

#include "InternalFunction.h"
#include "JSCJSValue.h"
#include "JSCellInlines.h"
#include "JSFunction.h"

namespace JSC {

ALWAYS_INLINE int32_t JSValue::toInt32(ExecState* exec) const
{
    if (isInt32())
        return asInt32();
    return JSC::toInt32(toNumber(exec));
}

inline uint32_t JSValue::toUInt32(ExecState* exec) const
{
    // See comment on JSC::toUInt32, above.
    return toInt32(exec);
}

inline bool JSValue::isUInt32() const
{
    return isInt32() && asInt32() >= 0;
}

inline uint32_t JSValue::asUInt32() const
{
    ASSERT(isUInt32());
    return asInt32();
}

inline double JSValue::asNumber() const
{
    ASSERT(isNumber());
    return isInt32() ? asInt32() : asDouble();
}

inline JSValue jsNaN()
{
    return JSValue(QNaN);
}

inline JSValue::JSValue(char i)
{
    *this = JSValue(static_cast<int32_t>(i));
}

inline JSValue::JSValue(unsigned char i)
{
    *this = JSValue(static_cast<int32_t>(i));
}

inline JSValue::JSValue(short i)
{
    *this = JSValue(static_cast<int32_t>(i));
}

inline JSValue::JSValue(unsigned short i)
{
    *this = JSValue(static_cast<int32_t>(i));
}

inline JSValue::JSValue(unsigned i)
{
    if (static_cast<int32_t>(i) < 0) {
        *this = JSValue(EncodeAsDouble, static_cast<double>(i));
        return;
    }
    *this = JSValue(static_cast<int32_t>(i));
}

inline JSValue::JSValue(long i)
{
    if (static_cast<int32_t>(i) != i) {
        *this = JSValue(EncodeAsDouble, static_cast<double>(i));
        return;
    }
    *this = JSValue(static_cast<int32_t>(i));
}

inline JSValue::JSValue(unsigned long i)
{
    if (static_cast<uint32_t>(i) != i) {
        *this = JSValue(EncodeAsDouble, static_cast<double>(i));
        return;
    }
    *this = JSValue(static_cast<uint32_t>(i));
}

inline JSValue::JSValue(long long i)
{
    if (static_cast<int32_t>(i) != i) {
        *this = JSValue(EncodeAsDouble, static_cast<double>(i));
        return;
    }
    *this = JSValue(static_cast<int32_t>(i));
}

inline JSValue::JSValue(unsigned long long i)
{
    if (static_cast<uint32_t>(i) != i) {
        *this = JSValue(EncodeAsDouble, static_cast<double>(i));
        return;
    }
    *this = JSValue(static_cast<uint32_t>(i));
}

inline JSValue::JSValue(double d)
{
    const int32_t asInt32 = static_cast<int32_t>(d);
    if (asInt32 != d || (!asInt32 && std::signbit(d))) { // true for -0.0
        *this = JSValue(EncodeAsDouble, d);
        return;
    }
    *this = JSValue(static_cast<int32_t>(d));
}

inline EncodedJSValue JSValue::encode(JSValue value)
{
    return value.u.asInt64;
}

inline JSValue JSValue::decode(EncodedJSValue encodedJSValue)
{
    JSValue v;
    v.u.asInt64 = encodedJSValue;
    return v;
}

#if USE(JSVALUE32_64)
inline JSValue::JSValue()
{
    u.asBits.tag = EmptyValueTag;
    u.asBits.payload = 0;
}

inline JSValue::JSValue(JSNullTag)
{
    u.asBits.tag = NullTag;
    u.asBits.payload = 0;
}
    
inline JSValue::JSValue(JSUndefinedTag)
{
    u.asBits.tag = UndefinedTag;
    u.asBits.payload = 0;
}
    
inline JSValue::JSValue(JSTrueTag)
{
    u.asBits.tag = BooleanTag;
    u.asBits.payload = 1;
}
    
inline JSValue::JSValue(JSFalseTag)
{
    u.asBits.tag = BooleanTag;
    u.asBits.payload = 0;
}

inline JSValue::JSValue(HashTableDeletedValueTag)
{
    u.asBits.tag = DeletedValueTag;
    u.asBits.payload = 0;
}

inline JSValue::JSValue(JSCell* ptr)
{
    if (ptr)
        u.asBits.tag = CellTag;
    else
        u.asBits.tag = EmptyValueTag;
    u.asBits.payload = reinterpret_cast<int32_t>(ptr);
}

inline JSValue::JSValue(const JSCell* ptr)
{
    if (ptr)
        u.asBits.tag = CellTag;
    else
        u.asBits.tag = EmptyValueTag;
    u.asBits.payload = reinterpret_cast<int32_t>(const_cast<JSCell*>(ptr));
}

inline JSValue::operator bool() const
{
    ASSERT(tag() != DeletedValueTag);
    return tag() != EmptyValueTag;
}

inline bool JSValue::operator==(const JSValue& other) const
{
    return u.asInt64 == other.u.asInt64;
}

inline bool JSValue::operator!=(const JSValue& other) const
{
    return u.asInt64 != other.u.asInt64;
}

inline bool JSValue::isEmpty() const
{
    return tag() == EmptyValueTag;
}

inline bool JSValue::isUndefined() const
{
    return tag() == UndefinedTag;
}

inline bool JSValue::isNull() const
{
    return tag() == NullTag;
}

inline bool JSValue::isUndefinedOrNull() const
{
    return isUndefined() || isNull();
}

inline bool JSValue::isCell() const
{
    return tag() == CellTag;
}

inline bool JSValue::isInt32() const
{
    return tag() == Int32Tag;
}

inline bool JSValue::isDouble() const
{
    return tag() < LowestTag;
}

inline bool JSValue::isTrue() const
{
    return tag() == BooleanTag && payload();
}

inline bool JSValue::isFalse() const
{
    return tag() == BooleanTag && !payload();
}

inline uint32_t JSValue::tag() const
{
    return u.asBits.tag;
}
    
inline int32_t JSValue::payload() const
{
    return u.asBits.payload;
}
    
inline int32_t JSValue::asInt32() const
{
    ASSERT(isInt32());
    return u.asBits.payload;
}
    
inline double JSValue::asDouble() const
{
    ASSERT(isDouble());
    return u.asDouble;
}
    
ALWAYS_INLINE JSCell* JSValue::asCell() const
{
    ASSERT(isCell());
    return reinterpret_cast<JSCell*>(u.asBits.payload);
}

ALWAYS_INLINE JSValue::JSValue(EncodeAsDoubleTag, double d)
{
    u.asDouble = d;
}

inline JSValue::JSValue(int i)
{
    u.asBits.tag = Int32Tag;
    u.asBits.payload = i;
}

#if ENABLE(LLINT_C_LOOP)
inline JSValue::JSValue(int32_t tag, int32_t payload)
{
    u.asBits.tag = tag;
    u.asBits.payload = payload;
}
#endif

inline bool JSValue::isNumber() const
{
    return isInt32() || isDouble();
}

inline bool JSValue::isBoolean() const
{
    return isTrue() || isFalse();
}

inline bool JSValue::asBoolean() const
{
    ASSERT(isBoolean());
    return payload();
}

#else // !USE(JSVALUE32_64) i.e. USE(JSVALUE64)

// 0x0 can never occur naturally because it has a tag of 00, indicating a pointer value, but a payload of 0x0, which is in the (invalid) zero page.
inline JSValue::JSValue()
{
    u.asInt64 = ValueEmpty;
}

// 0x4 can never occur naturally because it has a tag of 00, indicating a pointer value, but a payload of 0x4, which is in the (invalid) zero page.
inline JSValue::JSValue(HashTableDeletedValueTag)
{
    u.asInt64 = ValueDeleted;
}

inline JSValue::JSValue(JSCell* ptr)
{
    u.asInt64 = reinterpret_cast<uintptr_t>(ptr);
}

inline JSValue::JSValue(const JSCell* ptr)
{
    u.asInt64 = reinterpret_cast<uintptr_t>(const_cast<JSCell*>(ptr));
}

inline JSValue::operator bool() const
{
    return u.asInt64;
}

inline bool JSValue::operator==(const JSValue& other) const
{
    return u.asInt64 == other.u.asInt64;
}

inline bool JSValue::operator!=(const JSValue& other) const
{
    return u.asInt64 != other.u.asInt64;
}

inline bool JSValue::isEmpty() const
{
    return u.asInt64 == ValueEmpty;
}

inline bool JSValue::isUndefined() const
{
    return asValue() == JSValue(JSUndefined);
}

inline bool JSValue::isNull() const
{
    return asValue() == JSValue(JSNull);
}

inline bool JSValue::isTrue() const
{
    return asValue() == JSValue(JSTrue);
}

inline bool JSValue::isFalse() const
{
    return asValue() == JSValue(JSFalse);
}

inline bool JSValue::asBoolean() const
{
    ASSERT(isBoolean());
    return asValue() == JSValue(JSTrue);
}

inline int32_t JSValue::asInt32() const
{
    ASSERT(isInt32());
    return static_cast<int32_t>(u.asInt64);
}

inline bool JSValue::isDouble() const
{
    return isNumber() && !isInt32();
}

inline JSValue::JSValue(JSNullTag)
{
    u.asInt64 = ValueNull;
}
    
inline JSValue::JSValue(JSUndefinedTag)
{
    u.asInt64 = ValueUndefined;
}

inline JSValue::JSValue(JSTrueTag)
{
    u.asInt64 = ValueTrue;
}

inline JSValue::JSValue(JSFalseTag)
{
    u.asInt64 = ValueFalse;
}

inline bool JSValue::isUndefinedOrNull() const
{
    // Undefined and null share the same value, bar the 'undefined' bit in the extended tag.
    return (u.asInt64 & ~TagBitUndefined) == ValueNull;
}

inline bool JSValue::isBoolean() const
{
    return (u.asInt64 & ~1) == ValueFalse;
}

inline bool JSValue::isCell() const
{
    return !(u.asInt64 & TagMask);
}

inline bool JSValue::isInt32() const
{
    return (u.asInt64 & TagTypeNumber) == TagTypeNumber;
}

inline int64_t reinterpretDoubleToInt64(double value)
{
    return bitwise_cast<int64_t>(value);
}
inline double reinterpretInt64ToDouble(int64_t value)
{
    return bitwise_cast<double>(value);
}

ALWAYS_INLINE JSValue::JSValue(EncodeAsDoubleTag, double d)
{
    u.asInt64 = reinterpretDoubleToInt64(d) + DoubleEncodeOffset;
}

inline JSValue::JSValue(int i)
{
    u.asInt64 = TagTypeNumber | static_cast<uint32_t>(i);
}

inline double JSValue::asDouble() const
{
    ASSERT(isDouble());
    return reinterpretInt64ToDouble(u.asInt64 - DoubleEncodeOffset);
}

inline bool JSValue::isNumber() const
{
    return u.asInt64 & TagTypeNumber;
}

ALWAYS_INLINE JSCell* JSValue::asCell() const
{
    ASSERT(isCell());
    return u.ptr;
}

#endif // USE(JSVALUE64)

inline bool JSValue::isString() const
{
    return isCell() && asCell()->isString();
}

inline bool JSValue::isPrimitive() const
{
    return !isCell() || asCell()->isString();
}

inline bool JSValue::isGetterSetter() const
{
    return isCell() && asCell()->isGetterSetter();
}

inline bool JSValue::isObject() const
{
    return isCell() && asCell()->isObject();
}

inline bool JSValue::getString(ExecState* exec, String& s) const
{
    return isCell() && asCell()->getString(exec, s);
}

inline String JSValue::getString(ExecState* exec) const
{
    return isCell() ? asCell()->getString(exec) : String();
}

template <typename Base> String HandleConverter<Base, Unknown>::getString(ExecState* exec) const
{
    return jsValue().getString(exec);
}

inline JSObject* JSValue::getObject() const
{
    return isCell() ? asCell()->getObject() : 0;
}

ALWAYS_INLINE bool JSValue::getUInt32(uint32_t& v) const
{
    if (isInt32()) {
        int32_t i = asInt32();
        v = static_cast<uint32_t>(i);
        return i >= 0;
    }
    if (isDouble()) {
        double d = asDouble();
        v = static_cast<uint32_t>(d);
        return v == d;
    }
    return false;
}

inline JSValue JSValue::toPrimitive(ExecState* exec, PreferredPrimitiveType preferredType) const
{
    return isCell() ? asCell()->toPrimitive(exec, preferredType) : asValue();
}

inline bool JSValue::getPrimitiveNumber(ExecState* exec, double& number, JSValue& value)
{
    if (isInt32()) {
        number = asInt32();
        value = *this;
        return true;
    }
    if (isDouble()) {
        number = asDouble();
        value = *this;
        return true;
    }
    if (isCell())
        return asCell()->getPrimitiveNumber(exec, number, value);
    if (isTrue()) {
        number = 1.0;
        value = *this;
        return true;
    }
    if (isFalse() || isNull()) {
        number = 0.0;
        value = *this;
        return true;
    }
    ASSERT(isUndefined());
    number = QNaN;
    value = *this;
    return true;
}

ALWAYS_INLINE double JSValue::toNumber(ExecState* exec) const
{
    if (isInt32())
        return asInt32();
    if (isDouble())
        return asDouble();
    return toNumberSlowCase(exec);
}

inline JSObject* JSValue::toObject(ExecState* exec) const
{
    return isCell() ? asCell()->toObject(exec, exec->lexicalGlobalObject()) : toObjectSlowCase(exec, exec->lexicalGlobalObject());
}

inline JSObject* JSValue::toObject(ExecState* exec, JSGlobalObject* globalObject) const
{
    return isCell() ? asCell()->toObject(exec, globalObject) : toObjectSlowCase(exec, globalObject);
}

inline bool JSValue::isFunction() const
{
    return isCell() && (asCell()->inherits(&JSFunction::s_info) || asCell()->inherits(&InternalFunction::s_info));
}

// this method is here to be after the inline declaration of JSCell::inherits
inline bool JSValue::inherits(const ClassInfo* classInfo) const
{
    return isCell() && asCell()->inherits(classInfo);
}

inline JSObject* JSValue::toThisObject(ExecState* exec) const
{
    return isCell() ? asCell()->methodTable()->toThisObject(asCell(), exec) : toThisObjectSlowCase(exec);
}

inline JSValue JSValue::get(ExecState* exec, PropertyName propertyName) const
{
    PropertySlot slot(asValue());
    return get(exec, propertyName, slot);
}

inline JSValue JSValue::get(ExecState* exec, PropertyName propertyName, PropertySlot& slot) const
{
    if (UNLIKELY(!isCell())) {
        JSObject* prototype = synthesizePrototype(exec);
        if (!prototype->getPropertySlot(exec, propertyName, slot))
            return jsUndefined();
        return slot.getValue(exec, propertyName);
    }
    JSCell* cell = asCell();
    while (true) {
        if (cell->fastGetOwnPropertySlot(exec, propertyName, slot))
            return slot.getValue(exec, propertyName);
        JSValue prototype = asObject(cell)->prototype();
        if (!prototype.isObject())
            return jsUndefined();
        cell = asObject(prototype);
    }
}

inline JSValue JSValue::get(ExecState* exec, unsigned propertyName) const
{
    PropertySlot slot(asValue());
    return get(exec, propertyName, slot);
}

inline JSValue JSValue::get(ExecState* exec, unsigned propertyName, PropertySlot& slot) const
{
    if (UNLIKELY(!isCell())) {
        JSObject* prototype = synthesizePrototype(exec);
        if (!prototype->getPropertySlot(exec, propertyName, slot))
            return jsUndefined();
        return slot.getValue(exec, propertyName);
    }
    JSCell* cell = const_cast<JSCell*>(asCell());
    while (true) {
        if (cell->methodTable()->getOwnPropertySlotByIndex(cell, exec, propertyName, slot))
            return slot.getValue(exec, propertyName);
        JSValue prototype = asObject(cell)->prototype();
        if (!prototype.isObject())
            return jsUndefined();
        cell = prototype.asCell();
    }
}

inline void JSValue::put(ExecState* exec, PropertyName propertyName, JSValue value, PutPropertySlot& slot)
{
    if (UNLIKELY(!isCell())) {
        putToPrimitive(exec, propertyName, value, slot);
        return;
    }
    asCell()->methodTable()->put(asCell(), exec, propertyName, value, slot);
}

inline void JSValue::putByIndex(ExecState* exec, unsigned propertyName, JSValue value, bool shouldThrow)
{
    if (UNLIKELY(!isCell())) {
        putToPrimitiveByIndex(exec, propertyName, value, shouldThrow);
        return;
    }
    asCell()->methodTable()->putByIndex(asCell(), exec, propertyName, value, shouldThrow);
}

inline JSValue JSValue::structureOrUndefined() const
{
    if (isCell())
        return JSValue(asCell()->structure());
    return jsUndefined();
}

// ECMA 11.9.3
inline bool JSValue::equal(ExecState* exec, JSValue v1, JSValue v2)
{
    if (v1.isInt32() && v2.isInt32())
        return v1 == v2;

    return equalSlowCase(exec, v1, v2);
}

ALWAYS_INLINE bool JSValue::equalSlowCaseInline(ExecState* exec, JSValue v1, JSValue v2)
{
    do {
        if (v1.isNumber() && v2.isNumber())
            return v1.asNumber() == v2.asNumber();

        bool s1 = v1.isString();
        bool s2 = v2.isString();
        if (s1 && s2)
            return asString(v1)->value(exec) == asString(v2)->value(exec);

        if (v1.isUndefinedOrNull()) {
            if (v2.isUndefinedOrNull())
                return true;
            if (!v2.isCell())
                return false;
            return v2.asCell()->structure()->masqueradesAsUndefined(exec->lexicalGlobalObject());
        }

        if (v2.isUndefinedOrNull()) {
            if (!v1.isCell())
                return false;
            return v1.asCell()->structure()->masqueradesAsUndefined(exec->lexicalGlobalObject());
        }

        if (v1.isObject()) {
            if (v2.isObject())
                return v1 == v2;
            JSValue p1 = v1.toPrimitive(exec);
            if (exec->hadException())
                return false;
            v1 = p1;
            if (v1.isInt32() && v2.isInt32())
                return v1 == v2;
            continue;
        }

        if (v2.isObject()) {
            JSValue p2 = v2.toPrimitive(exec);
            if (exec->hadException())
                return false;
            v2 = p2;
            if (v1.isInt32() && v2.isInt32())
                return v1 == v2;
            continue;
        }

        if (s1 || s2) {
            double d1 = v1.toNumber(exec);
            double d2 = v2.toNumber(exec);
            return d1 == d2;
        }

        if (v1.isBoolean()) {
            if (v2.isNumber())
                return static_cast<double>(v1.asBoolean()) == v2.asNumber();
        } else if (v2.isBoolean()) {
            if (v1.isNumber())
                return v1.asNumber() == static_cast<double>(v2.asBoolean());
        }

        return v1 == v2;
    } while (true);
}

// ECMA 11.9.3
ALWAYS_INLINE bool JSValue::strictEqualSlowCaseInline(ExecState* exec, JSValue v1, JSValue v2)
{
    ASSERT(v1.isCell() && v2.isCell());

    if (v1.asCell()->isString() && v2.asCell()->isString())
        return asString(v1)->value(exec) == asString(v2)->value(exec);

    return v1 == v2;
}

inline bool JSValue::strictEqual(ExecState* exec, JSValue v1, JSValue v2)
{
    if (v1.isInt32() && v2.isInt32())
        return v1 == v2;

    if (v1.isNumber() && v2.isNumber())
        return v1.asNumber() == v2.asNumber();

    if (!v1.isCell() || !v2.isCell())
        return v1 == v2;

    return strictEqualSlowCaseInline(exec, v1, v2);
}

inline TriState JSValue::pureToBoolean() const
{
    if (isInt32())
        return asInt32() ? TrueTriState : FalseTriState;
    if (isDouble())
        return isNotZeroAndOrdered(asDouble()) ? TrueTriState : FalseTriState; // false for NaN
    if (isCell())
        return asCell()->pureToBoolean();
    return isTrue() ? TrueTriState : FalseTriState;
}

} // namespace JSC

#endif // JSValueInlines_h

