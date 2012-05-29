// Automatically generated from ../Source/JavaScriptCore/runtime/NumberConstructor.cpp using /Source/JavaScriptCore/create_hash_table. DO NOT EDIT!

#include "Lookup.h"

namespace JSC {
#if ENABLE(JIT)
#define THUNK_GENERATOR(generator) , generator
#else
#define THUNK_GENERATOR(generator)
#endif

static const struct HashTableValue numberTableValues[6] = {
   { "NaN", DontEnum|DontDelete|ReadOnly, (intptr_t)static_cast<PropertySlot::GetValueFunc>(numberConstructorNaNValue), (intptr_t)0 THUNK_GENERATOR(0) },
   { "NEGATIVE_INFINITY", DontEnum|DontDelete|ReadOnly, (intptr_t)static_cast<PropertySlot::GetValueFunc>(numberConstructorNegInfinity), (intptr_t)0 THUNK_GENERATOR(0) },
   { "POSITIVE_INFINITY", DontEnum|DontDelete|ReadOnly, (intptr_t)static_cast<PropertySlot::GetValueFunc>(numberConstructorPosInfinity), (intptr_t)0 THUNK_GENERATOR(0) },
   { "MAX_VALUE", DontEnum|DontDelete|ReadOnly, (intptr_t)static_cast<PropertySlot::GetValueFunc>(numberConstructorMaxValue), (intptr_t)0 THUNK_GENERATOR(0) },
   { "MIN_VALUE", DontEnum|DontDelete|ReadOnly, (intptr_t)static_cast<PropertySlot::GetValueFunc>(numberConstructorMinValue), (intptr_t)0 THUNK_GENERATOR(0) },
   { 0, 0, 0, 0 THUNK_GENERATOR(0) }
};

#undef THUNK_GENERATOR
extern JSC_CONST_HASHTABLE HashTable numberTable =
    { 16, 15, numberTableValues, 0 };
} // namespace
