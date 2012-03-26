// Automatically generated from ../Source/JavaScriptCore/runtime/ArrayPrototype.cpp using /Source/JavaScriptCore/create_hash_table. DO NOT EDIT!

#include "Lookup.h"

namespace JSC {
#if ENABLE(JIT)
#define THUNK_GENERATOR(generator) , generator
#else
#define THUNK_GENERATOR(generator)
#endif

static const struct HashTableValue arrayTableValues[22] = {
   { "toString", DontEnum|Function, (intptr_t)static_cast<NativeFunction>(arrayProtoFuncToString), (intptr_t)0 THUNK_GENERATOR(0) },
   { "toLocaleString", DontEnum|Function, (intptr_t)static_cast<NativeFunction>(arrayProtoFuncToLocaleString), (intptr_t)0 THUNK_GENERATOR(0) },
   { "concat", DontEnum|Function, (intptr_t)static_cast<NativeFunction>(arrayProtoFuncConcat), (intptr_t)1 THUNK_GENERATOR(0) },
   { "join", DontEnum|Function, (intptr_t)static_cast<NativeFunction>(arrayProtoFuncJoin), (intptr_t)1 THUNK_GENERATOR(0) },
   { "pop", DontEnum|Function, (intptr_t)static_cast<NativeFunction>(arrayProtoFuncPop), (intptr_t)0 THUNK_GENERATOR(0) },
   { "push", DontEnum|Function, (intptr_t)static_cast<NativeFunction>(arrayProtoFuncPush), (intptr_t)1 THUNK_GENERATOR(0) },
   { "reverse", DontEnum|Function, (intptr_t)static_cast<NativeFunction>(arrayProtoFuncReverse), (intptr_t)0 THUNK_GENERATOR(0) },
   { "shift", DontEnum|Function, (intptr_t)static_cast<NativeFunction>(arrayProtoFuncShift), (intptr_t)0 THUNK_GENERATOR(0) },
   { "slice", DontEnum|Function, (intptr_t)static_cast<NativeFunction>(arrayProtoFuncSlice), (intptr_t)2 THUNK_GENERATOR(0) },
   { "sort", DontEnum|Function, (intptr_t)static_cast<NativeFunction>(arrayProtoFuncSort), (intptr_t)1 THUNK_GENERATOR(0) },
   { "splice", DontEnum|Function, (intptr_t)static_cast<NativeFunction>(arrayProtoFuncSplice), (intptr_t)2 THUNK_GENERATOR(0) },
   { "unshift", DontEnum|Function, (intptr_t)static_cast<NativeFunction>(arrayProtoFuncUnShift), (intptr_t)1 THUNK_GENERATOR(0) },
   { "every", DontEnum|Function, (intptr_t)static_cast<NativeFunction>(arrayProtoFuncEvery), (intptr_t)1 THUNK_GENERATOR(0) },
   { "forEach", DontEnum|Function, (intptr_t)static_cast<NativeFunction>(arrayProtoFuncForEach), (intptr_t)1 THUNK_GENERATOR(0) },
   { "some", DontEnum|Function, (intptr_t)static_cast<NativeFunction>(arrayProtoFuncSome), (intptr_t)1 THUNK_GENERATOR(0) },
   { "indexOf", DontEnum|Function, (intptr_t)static_cast<NativeFunction>(arrayProtoFuncIndexOf), (intptr_t)1 THUNK_GENERATOR(0) },
   { "lastIndexOf", DontEnum|Function, (intptr_t)static_cast<NativeFunction>(arrayProtoFuncLastIndexOf), (intptr_t)1 THUNK_GENERATOR(0) },
   { "filter", DontEnum|Function, (intptr_t)static_cast<NativeFunction>(arrayProtoFuncFilter), (intptr_t)1 THUNK_GENERATOR(0) },
   { "reduce", DontEnum|Function, (intptr_t)static_cast<NativeFunction>(arrayProtoFuncReduce), (intptr_t)1 THUNK_GENERATOR(0) },
   { "reduceRight", DontEnum|Function, (intptr_t)static_cast<NativeFunction>(arrayProtoFuncReduceRight), (intptr_t)1 THUNK_GENERATOR(0) },
   { "map", DontEnum|Function, (intptr_t)static_cast<NativeFunction>(arrayProtoFuncMap), (intptr_t)1 THUNK_GENERATOR(0) },
   { 0, 0, 0, 0 THUNK_GENERATOR(0) }
};

#undef THUNK_GENERATOR
extern JSC_CONST_HASHTABLE HashTable arrayTable =
    { 65, 63, arrayTableValues, 0 };
} // namespace
