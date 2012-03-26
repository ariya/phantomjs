// Automatically generated from ../Source/JavaScriptCore/runtime/DatePrototype.cpp using /Source/JavaScriptCore/create_hash_table. DO NOT EDIT!

#include "Lookup.h"

namespace JSC {
#if ENABLE(JIT)
#define THUNK_GENERATOR(generator) , generator
#else
#define THUNK_GENERATOR(generator)
#endif

static const struct HashTableValue dateTableValues[47] = {
   { "toString", DontEnum|Function, (intptr_t)static_cast<NativeFunction>(dateProtoFuncToString), (intptr_t)0 THUNK_GENERATOR(0) },
   { "toISOString", DontEnum|Function, (intptr_t)static_cast<NativeFunction>(dateProtoFuncToISOString), (intptr_t)0 THUNK_GENERATOR(0) },
   { "toUTCString", DontEnum|Function, (intptr_t)static_cast<NativeFunction>(dateProtoFuncToUTCString), (intptr_t)0 THUNK_GENERATOR(0) },
   { "toDateString", DontEnum|Function, (intptr_t)static_cast<NativeFunction>(dateProtoFuncToDateString), (intptr_t)0 THUNK_GENERATOR(0) },
   { "toTimeString", DontEnum|Function, (intptr_t)static_cast<NativeFunction>(dateProtoFuncToTimeString), (intptr_t)0 THUNK_GENERATOR(0) },
   { "toLocaleString", DontEnum|Function, (intptr_t)static_cast<NativeFunction>(dateProtoFuncToLocaleString), (intptr_t)0 THUNK_GENERATOR(0) },
   { "toLocaleDateString", DontEnum|Function, (intptr_t)static_cast<NativeFunction>(dateProtoFuncToLocaleDateString), (intptr_t)0 THUNK_GENERATOR(0) },
   { "toLocaleTimeString", DontEnum|Function, (intptr_t)static_cast<NativeFunction>(dateProtoFuncToLocaleTimeString), (intptr_t)0 THUNK_GENERATOR(0) },
   { "valueOf", DontEnum|Function, (intptr_t)static_cast<NativeFunction>(dateProtoFuncGetTime), (intptr_t)0 THUNK_GENERATOR(0) },
   { "getTime", DontEnum|Function, (intptr_t)static_cast<NativeFunction>(dateProtoFuncGetTime), (intptr_t)0 THUNK_GENERATOR(0) },
   { "getFullYear", DontEnum|Function, (intptr_t)static_cast<NativeFunction>(dateProtoFuncGetFullYear), (intptr_t)0 THUNK_GENERATOR(0) },
   { "getUTCFullYear", DontEnum|Function, (intptr_t)static_cast<NativeFunction>(dateProtoFuncGetUTCFullYear), (intptr_t)0 THUNK_GENERATOR(0) },
   { "toGMTString", DontEnum|Function, (intptr_t)static_cast<NativeFunction>(dateProtoFuncToGMTString), (intptr_t)0 THUNK_GENERATOR(0) },
   { "getMonth", DontEnum|Function, (intptr_t)static_cast<NativeFunction>(dateProtoFuncGetMonth), (intptr_t)0 THUNK_GENERATOR(0) },
   { "getUTCMonth", DontEnum|Function, (intptr_t)static_cast<NativeFunction>(dateProtoFuncGetUTCMonth), (intptr_t)0 THUNK_GENERATOR(0) },
   { "getDate", DontEnum|Function, (intptr_t)static_cast<NativeFunction>(dateProtoFuncGetDate), (intptr_t)0 THUNK_GENERATOR(0) },
   { "getUTCDate", DontEnum|Function, (intptr_t)static_cast<NativeFunction>(dateProtoFuncGetUTCDate), (intptr_t)0 THUNK_GENERATOR(0) },
   { "getDay", DontEnum|Function, (intptr_t)static_cast<NativeFunction>(dateProtoFuncGetDay), (intptr_t)0 THUNK_GENERATOR(0) },
   { "getUTCDay", DontEnum|Function, (intptr_t)static_cast<NativeFunction>(dateProtoFuncGetUTCDay), (intptr_t)0 THUNK_GENERATOR(0) },
   { "getHours", DontEnum|Function, (intptr_t)static_cast<NativeFunction>(dateProtoFuncGetHours), (intptr_t)0 THUNK_GENERATOR(0) },
   { "getUTCHours", DontEnum|Function, (intptr_t)static_cast<NativeFunction>(dateProtoFuncGetUTCHours), (intptr_t)0 THUNK_GENERATOR(0) },
   { "getMinutes", DontEnum|Function, (intptr_t)static_cast<NativeFunction>(dateProtoFuncGetMinutes), (intptr_t)0 THUNK_GENERATOR(0) },
   { "getUTCMinutes", DontEnum|Function, (intptr_t)static_cast<NativeFunction>(dateProtoFuncGetUTCMinutes), (intptr_t)0 THUNK_GENERATOR(0) },
   { "getSeconds", DontEnum|Function, (intptr_t)static_cast<NativeFunction>(dateProtoFuncGetSeconds), (intptr_t)0 THUNK_GENERATOR(0) },
   { "getUTCSeconds", DontEnum|Function, (intptr_t)static_cast<NativeFunction>(dateProtoFuncGetUTCSeconds), (intptr_t)0 THUNK_GENERATOR(0) },
   { "getMilliseconds", DontEnum|Function, (intptr_t)static_cast<NativeFunction>(dateProtoFuncGetMilliSeconds), (intptr_t)0 THUNK_GENERATOR(0) },
   { "getUTCMilliseconds", DontEnum|Function, (intptr_t)static_cast<NativeFunction>(dateProtoFuncGetUTCMilliseconds), (intptr_t)0 THUNK_GENERATOR(0) },
   { "getTimezoneOffset", DontEnum|Function, (intptr_t)static_cast<NativeFunction>(dateProtoFuncGetTimezoneOffset), (intptr_t)0 THUNK_GENERATOR(0) },
   { "setTime", DontEnum|Function, (intptr_t)static_cast<NativeFunction>(dateProtoFuncSetTime), (intptr_t)1 THUNK_GENERATOR(0) },
   { "setMilliseconds", DontEnum|Function, (intptr_t)static_cast<NativeFunction>(dateProtoFuncSetMilliSeconds), (intptr_t)1 THUNK_GENERATOR(0) },
   { "setUTCMilliseconds", DontEnum|Function, (intptr_t)static_cast<NativeFunction>(dateProtoFuncSetUTCMilliseconds), (intptr_t)1 THUNK_GENERATOR(0) },
   { "setSeconds", DontEnum|Function, (intptr_t)static_cast<NativeFunction>(dateProtoFuncSetSeconds), (intptr_t)2 THUNK_GENERATOR(0) },
   { "setUTCSeconds", DontEnum|Function, (intptr_t)static_cast<NativeFunction>(dateProtoFuncSetUTCSeconds), (intptr_t)2 THUNK_GENERATOR(0) },
   { "setMinutes", DontEnum|Function, (intptr_t)static_cast<NativeFunction>(dateProtoFuncSetMinutes), (intptr_t)3 THUNK_GENERATOR(0) },
   { "setUTCMinutes", DontEnum|Function, (intptr_t)static_cast<NativeFunction>(dateProtoFuncSetUTCMinutes), (intptr_t)3 THUNK_GENERATOR(0) },
   { "setHours", DontEnum|Function, (intptr_t)static_cast<NativeFunction>(dateProtoFuncSetHours), (intptr_t)4 THUNK_GENERATOR(0) },
   { "setUTCHours", DontEnum|Function, (intptr_t)static_cast<NativeFunction>(dateProtoFuncSetUTCHours), (intptr_t)4 THUNK_GENERATOR(0) },
   { "setDate", DontEnum|Function, (intptr_t)static_cast<NativeFunction>(dateProtoFuncSetDate), (intptr_t)1 THUNK_GENERATOR(0) },
   { "setUTCDate", DontEnum|Function, (intptr_t)static_cast<NativeFunction>(dateProtoFuncSetUTCDate), (intptr_t)1 THUNK_GENERATOR(0) },
   { "setMonth", DontEnum|Function, (intptr_t)static_cast<NativeFunction>(dateProtoFuncSetMonth), (intptr_t)2 THUNK_GENERATOR(0) },
   { "setUTCMonth", DontEnum|Function, (intptr_t)static_cast<NativeFunction>(dateProtoFuncSetUTCMonth), (intptr_t)2 THUNK_GENERATOR(0) },
   { "setFullYear", DontEnum|Function, (intptr_t)static_cast<NativeFunction>(dateProtoFuncSetFullYear), (intptr_t)3 THUNK_GENERATOR(0) },
   { "setUTCFullYear", DontEnum|Function, (intptr_t)static_cast<NativeFunction>(dateProtoFuncSetUTCFullYear), (intptr_t)3 THUNK_GENERATOR(0) },
   { "setYear", DontEnum|Function, (intptr_t)static_cast<NativeFunction>(dateProtoFuncSetYear), (intptr_t)1 THUNK_GENERATOR(0) },
   { "getYear", DontEnum|Function, (intptr_t)static_cast<NativeFunction>(dateProtoFuncGetYear), (intptr_t)0 THUNK_GENERATOR(0) },
   { "toJSON", DontEnum|Function, (intptr_t)static_cast<NativeFunction>(dateProtoFuncToJSON), (intptr_t)1 THUNK_GENERATOR(0) },
   { 0, 0, 0, 0 THUNK_GENERATOR(0) }
};

#undef THUNK_GENERATOR
extern JSC_CONST_HASHTABLE HashTable dateTable =
    { 134, 127, dateTableValues, 0 };
} // namespace
