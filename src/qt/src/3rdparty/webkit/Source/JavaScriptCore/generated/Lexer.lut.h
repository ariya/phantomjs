// Automatically generated from ../Source/JavaScriptCore/parser/Keywords.table using /Source/JavaScriptCore/create_hash_table. DO NOT EDIT!

#include "Lookup.h"

namespace JSC {
#if ENABLE(JIT)
#define THUNK_GENERATOR(generator) , generator
#else
#define THUNK_GENERATOR(generator)
#endif

static const struct HashTableValue mainTableValues[37] = {
   { "null", 0, (intptr_t)(NULLTOKEN), (intptr_t)0 THUNK_GENERATOR(0) },
   { "true", 0, (intptr_t)(TRUETOKEN), (intptr_t)0 THUNK_GENERATOR(0) },
   { "false", 0, (intptr_t)(FALSETOKEN), (intptr_t)0 THUNK_GENERATOR(0) },
   { "break", 0, (intptr_t)(BREAK), (intptr_t)0 THUNK_GENERATOR(0) },
   { "case", 0, (intptr_t)(CASE), (intptr_t)0 THUNK_GENERATOR(0) },
   { "catch", 0, (intptr_t)(CATCH), (intptr_t)0 THUNK_GENERATOR(0) },
   { "const", 0, (intptr_t)(CONSTTOKEN), (intptr_t)0 THUNK_GENERATOR(0) },
   { "default", 0, (intptr_t)(DEFAULT), (intptr_t)0 THUNK_GENERATOR(0) },
   { "finally", 0, (intptr_t)(FINALLY), (intptr_t)0 THUNK_GENERATOR(0) },
   { "for", 0, (intptr_t)(FOR), (intptr_t)0 THUNK_GENERATOR(0) },
   { "instanceof", 0, (intptr_t)(INSTANCEOF), (intptr_t)0 THUNK_GENERATOR(0) },
   { "new", 0, (intptr_t)(NEW), (intptr_t)0 THUNK_GENERATOR(0) },
   { "var", 0, (intptr_t)(VAR), (intptr_t)0 THUNK_GENERATOR(0) },
   { "continue", 0, (intptr_t)(CONTINUE), (intptr_t)0 THUNK_GENERATOR(0) },
   { "function", 0, (intptr_t)(FUNCTION), (intptr_t)0 THUNK_GENERATOR(0) },
   { "return", 0, (intptr_t)(RETURN), (intptr_t)0 THUNK_GENERATOR(0) },
   { "void", 0, (intptr_t)(VOIDTOKEN), (intptr_t)0 THUNK_GENERATOR(0) },
   { "delete", 0, (intptr_t)(DELETETOKEN), (intptr_t)0 THUNK_GENERATOR(0) },
   { "if", 0, (intptr_t)(IF), (intptr_t)0 THUNK_GENERATOR(0) },
   { "this", 0, (intptr_t)(THISTOKEN), (intptr_t)0 THUNK_GENERATOR(0) },
   { "do", 0, (intptr_t)(DO), (intptr_t)0 THUNK_GENERATOR(0) },
   { "while", 0, (intptr_t)(WHILE), (intptr_t)0 THUNK_GENERATOR(0) },
   { "else", 0, (intptr_t)(ELSE), (intptr_t)0 THUNK_GENERATOR(0) },
   { "in", 0, (intptr_t)(INTOKEN), (intptr_t)0 THUNK_GENERATOR(0) },
   { "switch", 0, (intptr_t)(SWITCH), (intptr_t)0 THUNK_GENERATOR(0) },
   { "throw", 0, (intptr_t)(THROW), (intptr_t)0 THUNK_GENERATOR(0) },
   { "try", 0, (intptr_t)(TRY), (intptr_t)0 THUNK_GENERATOR(0) },
   { "typeof", 0, (intptr_t)(TYPEOF), (intptr_t)0 THUNK_GENERATOR(0) },
   { "with", 0, (intptr_t)(WITH), (intptr_t)0 THUNK_GENERATOR(0) },
   { "debugger", 0, (intptr_t)(DEBUGGER), (intptr_t)0 THUNK_GENERATOR(0) },
   { "class", 0, (intptr_t)(RESERVED), (intptr_t)0 THUNK_GENERATOR(0) },
   { "enum", 0, (intptr_t)(RESERVED), (intptr_t)0 THUNK_GENERATOR(0) },
   { "export", 0, (intptr_t)(RESERVED), (intptr_t)0 THUNK_GENERATOR(0) },
   { "extends", 0, (intptr_t)(RESERVED), (intptr_t)0 THUNK_GENERATOR(0) },
   { "import", 0, (intptr_t)(RESERVED), (intptr_t)0 THUNK_GENERATOR(0) },
   { "super", 0, (intptr_t)(RESERVED), (intptr_t)0 THUNK_GENERATOR(0) },
   { 0, 0, 0, 0 THUNK_GENERATOR(0) }
};

#undef THUNK_GENERATOR
extern JSC_CONST_HASHTABLE HashTable mainTable =
    { 133, 127, mainTableValues, 0 };
} // namespace
