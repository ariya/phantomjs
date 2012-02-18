// Automatically generated from ../Source/JavaScriptCore/runtime/ObjectConstructor.cpp using /Source/JavaScriptCore/create_hash_table. DO NOT EDIT!

#include "Lookup.h"

namespace JSC {
#if ENABLE(JIT)
#define THUNK_GENERATOR(generator) , generator
#else
#define THUNK_GENERATOR(generator)
#endif

static const struct HashTableValue objectConstructorTableValues[14] = {
   { "getPrototypeOf", DontEnum|Function, (intptr_t)static_cast<NativeFunction>(objectConstructorGetPrototypeOf), (intptr_t)1 THUNK_GENERATOR(0) },
   { "getOwnPropertyDescriptor", DontEnum|Function, (intptr_t)static_cast<NativeFunction>(objectConstructorGetOwnPropertyDescriptor), (intptr_t)2 THUNK_GENERATOR(0) },
   { "getOwnPropertyNames", DontEnum|Function, (intptr_t)static_cast<NativeFunction>(objectConstructorGetOwnPropertyNames), (intptr_t)1 THUNK_GENERATOR(0) },
   { "keys", DontEnum|Function, (intptr_t)static_cast<NativeFunction>(objectConstructorKeys), (intptr_t)1 THUNK_GENERATOR(0) },
   { "defineProperty", DontEnum|Function, (intptr_t)static_cast<NativeFunction>(objectConstructorDefineProperty), (intptr_t)3 THUNK_GENERATOR(0) },
   { "defineProperties", DontEnum|Function, (intptr_t)static_cast<NativeFunction>(objectConstructorDefineProperties), (intptr_t)2 THUNK_GENERATOR(0) },
   { "create", DontEnum|Function, (intptr_t)static_cast<NativeFunction>(objectConstructorCreate), (intptr_t)2 THUNK_GENERATOR(0) },
   { "seal", DontEnum|Function, (intptr_t)static_cast<NativeFunction>(objectConstructorSeal), (intptr_t)1 THUNK_GENERATOR(0) },
   { "freeze", DontEnum|Function, (intptr_t)static_cast<NativeFunction>(objectConstructorFreeze), (intptr_t)1 THUNK_GENERATOR(0) },
   { "preventExtensions", DontEnum|Function, (intptr_t)static_cast<NativeFunction>(objectConstructorPreventExtensions), (intptr_t)1 THUNK_GENERATOR(0) },
   { "isSealed", DontEnum|Function, (intptr_t)static_cast<NativeFunction>(objectConstructorIsSealed), (intptr_t)1 THUNK_GENERATOR(0) },
   { "isFrozen", DontEnum|Function, (intptr_t)static_cast<NativeFunction>(objectConstructorIsFrozen), (intptr_t)1 THUNK_GENERATOR(0) },
   { "isExtensible", DontEnum|Function, (intptr_t)static_cast<NativeFunction>(objectConstructorIsExtensible), (intptr_t)1 THUNK_GENERATOR(0) },
   { 0, 0, 0, 0 THUNK_GENERATOR(0) }
};

#undef THUNK_GENERATOR
extern JSC_CONST_HASHTABLE HashTable objectConstructorTable =
    { 34, 31, objectConstructorTableValues, 0 };
} // namespace
