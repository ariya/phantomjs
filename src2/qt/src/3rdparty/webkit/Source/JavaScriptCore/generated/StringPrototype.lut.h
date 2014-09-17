// Automatically generated from ../Source/JavaScriptCore/runtime/StringPrototype.cpp using /Source/JavaScriptCore/create_hash_table. DO NOT EDIT!

#include "Lookup.h"

namespace JSC {
#if ENABLE(JIT)
#define THUNK_GENERATOR(generator) , generator
#else
#define THUNK_GENERATOR(generator)
#endif

static const struct HashTableValue stringTableValues[36] = {
   { "toString", DontEnum|Function, (intptr_t)static_cast<NativeFunction>(stringProtoFuncToString), (intptr_t)0 THUNK_GENERATOR(0) },
   { "valueOf", DontEnum|Function, (intptr_t)static_cast<NativeFunction>(stringProtoFuncToString), (intptr_t)0 THUNK_GENERATOR(0) },
   { "charAt", DontEnum|Function, (intptr_t)static_cast<NativeFunction>(stringProtoFuncCharAt), (intptr_t)1 THUNK_GENERATOR(charAtThunkGenerator) },
   { "charCodeAt", DontEnum|Function, (intptr_t)static_cast<NativeFunction>(stringProtoFuncCharCodeAt), (intptr_t)1 THUNK_GENERATOR(charCodeAtThunkGenerator) },
   { "concat", DontEnum|Function, (intptr_t)static_cast<NativeFunction>(stringProtoFuncConcat), (intptr_t)1 THUNK_GENERATOR(0) },
   { "indexOf", DontEnum|Function, (intptr_t)static_cast<NativeFunction>(stringProtoFuncIndexOf), (intptr_t)1 THUNK_GENERATOR(0) },
   { "lastIndexOf", DontEnum|Function, (intptr_t)static_cast<NativeFunction>(stringProtoFuncLastIndexOf), (intptr_t)1 THUNK_GENERATOR(0) },
   { "match", DontEnum|Function, (intptr_t)static_cast<NativeFunction>(stringProtoFuncMatch), (intptr_t)1 THUNK_GENERATOR(0) },
   { "replace", DontEnum|Function, (intptr_t)static_cast<NativeFunction>(stringProtoFuncReplace), (intptr_t)2 THUNK_GENERATOR(0) },
   { "search", DontEnum|Function, (intptr_t)static_cast<NativeFunction>(stringProtoFuncSearch), (intptr_t)1 THUNK_GENERATOR(0) },
   { "slice", DontEnum|Function, (intptr_t)static_cast<NativeFunction>(stringProtoFuncSlice), (intptr_t)2 THUNK_GENERATOR(0) },
   { "split", DontEnum|Function, (intptr_t)static_cast<NativeFunction>(stringProtoFuncSplit), (intptr_t)2 THUNK_GENERATOR(0) },
   { "substr", DontEnum|Function, (intptr_t)static_cast<NativeFunction>(stringProtoFuncSubstr), (intptr_t)2 THUNK_GENERATOR(0) },
   { "substring", DontEnum|Function, (intptr_t)static_cast<NativeFunction>(stringProtoFuncSubstring), (intptr_t)2 THUNK_GENERATOR(0) },
   { "toLowerCase", DontEnum|Function, (intptr_t)static_cast<NativeFunction>(stringProtoFuncToLowerCase), (intptr_t)0 THUNK_GENERATOR(0) },
   { "toUpperCase", DontEnum|Function, (intptr_t)static_cast<NativeFunction>(stringProtoFuncToUpperCase), (intptr_t)0 THUNK_GENERATOR(0) },
   { "localeCompare", DontEnum|Function, (intptr_t)static_cast<NativeFunction>(stringProtoFuncLocaleCompare), (intptr_t)1 THUNK_GENERATOR(0) },
   { "toLocaleLowerCase", DontEnum|Function, (intptr_t)static_cast<NativeFunction>(stringProtoFuncToLowerCase), (intptr_t)0 THUNK_GENERATOR(0) },
   { "toLocaleUpperCase", DontEnum|Function, (intptr_t)static_cast<NativeFunction>(stringProtoFuncToUpperCase), (intptr_t)0 THUNK_GENERATOR(0) },
   { "big", DontEnum|Function, (intptr_t)static_cast<NativeFunction>(stringProtoFuncBig), (intptr_t)0 THUNK_GENERATOR(0) },
   { "small", DontEnum|Function, (intptr_t)static_cast<NativeFunction>(stringProtoFuncSmall), (intptr_t)0 THUNK_GENERATOR(0) },
   { "blink", DontEnum|Function, (intptr_t)static_cast<NativeFunction>(stringProtoFuncBlink), (intptr_t)0 THUNK_GENERATOR(0) },
   { "bold", DontEnum|Function, (intptr_t)static_cast<NativeFunction>(stringProtoFuncBold), (intptr_t)0 THUNK_GENERATOR(0) },
   { "fixed", DontEnum|Function, (intptr_t)static_cast<NativeFunction>(stringProtoFuncFixed), (intptr_t)0 THUNK_GENERATOR(0) },
   { "italics", DontEnum|Function, (intptr_t)static_cast<NativeFunction>(stringProtoFuncItalics), (intptr_t)0 THUNK_GENERATOR(0) },
   { "strike", DontEnum|Function, (intptr_t)static_cast<NativeFunction>(stringProtoFuncStrike), (intptr_t)0 THUNK_GENERATOR(0) },
   { "sub", DontEnum|Function, (intptr_t)static_cast<NativeFunction>(stringProtoFuncSub), (intptr_t)0 THUNK_GENERATOR(0) },
   { "sup", DontEnum|Function, (intptr_t)static_cast<NativeFunction>(stringProtoFuncSup), (intptr_t)0 THUNK_GENERATOR(0) },
   { "fontcolor", DontEnum|Function, (intptr_t)static_cast<NativeFunction>(stringProtoFuncFontcolor), (intptr_t)1 THUNK_GENERATOR(0) },
   { "fontsize", DontEnum|Function, (intptr_t)static_cast<NativeFunction>(stringProtoFuncFontsize), (intptr_t)1 THUNK_GENERATOR(0) },
   { "anchor", DontEnum|Function, (intptr_t)static_cast<NativeFunction>(stringProtoFuncAnchor), (intptr_t)1 THUNK_GENERATOR(0) },
   { "link", DontEnum|Function, (intptr_t)static_cast<NativeFunction>(stringProtoFuncLink), (intptr_t)1 THUNK_GENERATOR(0) },
   { "trim", DontEnum|Function, (intptr_t)static_cast<NativeFunction>(stringProtoFuncTrim), (intptr_t)0 THUNK_GENERATOR(0) },
   { "trimLeft", DontEnum|Function, (intptr_t)static_cast<NativeFunction>(stringProtoFuncTrimLeft), (intptr_t)0 THUNK_GENERATOR(0) },
   { "trimRight", DontEnum|Function, (intptr_t)static_cast<NativeFunction>(stringProtoFuncTrimRight), (intptr_t)0 THUNK_GENERATOR(0) },
   { 0, 0, 0, 0 THUNK_GENERATOR(0) }
};

#undef THUNK_GENERATOR
extern JSC_CONST_HASHTABLE HashTable stringTable =
    { 133, 127, stringTableValues, 0 };
} // namespace
