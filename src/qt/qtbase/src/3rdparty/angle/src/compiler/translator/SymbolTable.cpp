//
// Copyright (c) 2002-2013 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

//
// Symbol table for parsing.  Most functionaliy and main ideas
// are documented in the header file.
//

#if defined(_MSC_VER)
#pragma warning(disable: 4718)
#endif

#include "compiler/translator/SymbolTable.h"

#include <stdio.h>
#include <algorithm>

int TSymbolTable::uniqueIdCounter = 0;

//
// Functions have buried pointers to delete.
//
TFunction::~TFunction()
{
    for (TParamList::iterator i = parameters.begin(); i != parameters.end(); ++i)
        delete (*i).type;
}

//
// Symbol table levels are a map of pointers to symbols that have to be deleted.
//
TSymbolTableLevel::~TSymbolTableLevel()
{
    for (tLevel::iterator it = level.begin(); it != level.end(); ++it)
        delete (*it).second;
}

bool TSymbolTableLevel::insert(TSymbol *symbol)
{
    symbol->setUniqueId(TSymbolTable::nextUniqueId());

    // returning true means symbol was added to the table
    tInsertResult result = level.insert(tLevelPair(symbol->getMangledName(), symbol));

    return result.second;
}

TSymbol *TSymbolTableLevel::find(const TString &name) const
{
    tLevel::const_iterator it = level.find(name);
    if (it == level.end())
        return 0;
    else
        return (*it).second;
}

//
// Change all function entries in the table with the non-mangled name
// to be related to the provided built-in operation.  This is a low
// performance operation, and only intended for symbol tables that
// live across a large number of compiles.
//
void TSymbolTableLevel::relateToOperator(const char *name, TOperator op)
{
    for (tLevel::iterator it = level.begin(); it != level.end(); ++it)
    {
        if ((*it).second->isFunction())
        {
            TFunction *function = static_cast<TFunction*>((*it).second);
            if (function->getName() == name)
                function->relateToOperator(op);
        }
    }
}

//
// Change all function entries in the table with the non-mangled name
// to be related to the provided built-in extension. This is a low
// performance operation, and only intended for symbol tables that
// live across a large number of compiles.
//
void TSymbolTableLevel::relateToExtension(const char *name, const TString &ext)
{
    for (tLevel::iterator it = level.begin(); it != level.end(); ++it)
    {
        TSymbol *symbol = it->second;
        if (symbol->getName() == name)
            symbol->relateToExtension(ext);
    }
}

TSymbol::TSymbol(const TSymbol &copyOf)
{
    name = NewPoolTString(copyOf.name->c_str());
    uniqueId = copyOf.uniqueId;
}

TSymbol *TSymbolTable::find(const TString &name, int shaderVersion,
                            bool *builtIn, bool *sameScope) const
{
    int level = currentLevel();
    TSymbol *symbol;

    do
    {
        if (level == ESSL3_BUILTINS && shaderVersion != 300)
            level--;
        if (level == ESSL1_BUILTINS && shaderVersion != 100)
            level--;

        symbol = table[level]->find(name);
    }
    while (symbol == 0 && --level >= 0);

    if (builtIn)
        *builtIn = (level <= LAST_BUILTIN_LEVEL);
    if (sameScope)
        *sameScope = (level == currentLevel());

    return symbol;
}

TSymbol *TSymbolTable::findBuiltIn(
    const TString &name, int shaderVersion) const
{
    for (int level = LAST_BUILTIN_LEVEL; level >= 0; level--)
    {
        if (level == ESSL3_BUILTINS && shaderVersion != 300)
            level--;
        if (level == ESSL1_BUILTINS && shaderVersion != 100)
            level--;

        TSymbol *symbol = table[level]->find(name);

        if (symbol)
            return symbol;
    }

    return 0;
}

TSymbolTable::~TSymbolTable()
{
    while (table.size() > 0)
        pop();
}

void TSymbolTable::insertBuiltIn(
    ESymbolLevel level, TType *rvalue, const char *name,
    TType *ptype1, TType *ptype2, TType *ptype3, TType *ptype4, TType *ptype5)
{
    if (ptype1->getBasicType() == EbtGSampler2D)
    {
        bool gvec4 = (rvalue->getBasicType() == EbtGVec4);
        insertBuiltIn(level, gvec4 ? new TType(EbtFloat, 4) : rvalue, name,
                      new TType(EbtSampler2D), ptype2, ptype3, ptype4, ptype5);
        insertBuiltIn(level, gvec4 ? new TType(EbtInt, 4) : rvalue, name,
                      new TType(EbtISampler2D), ptype2, ptype3, ptype4, ptype5);
        insertBuiltIn(level, gvec4 ? new TType(EbtUInt, 4) : rvalue, name,
                      new TType(EbtUSampler2D), ptype2, ptype3, ptype4, ptype5);
        return;
    }
    if (ptype1->getBasicType() == EbtGSampler3D)
    {
        bool gvec4 = (rvalue->getBasicType() == EbtGVec4);
        insertBuiltIn(level, gvec4 ? new TType(EbtFloat, 4) : rvalue, name,
                      new TType(EbtSampler3D), ptype2, ptype3, ptype4, ptype5);
        insertBuiltIn(level, gvec4 ? new TType(EbtInt, 4) : rvalue, name,
                      new TType(EbtISampler3D), ptype2, ptype3, ptype4, ptype5);
        insertBuiltIn(level, gvec4 ? new TType(EbtUInt, 4) : rvalue, name,
                      new TType(EbtUSampler3D), ptype2, ptype3, ptype4, ptype5);
        return;
    }
    if (ptype1->getBasicType() == EbtGSamplerCube)
    {
        bool gvec4 = (rvalue->getBasicType() == EbtGVec4);
        insertBuiltIn(level, gvec4 ? new TType(EbtFloat, 4) : rvalue, name,
                      new TType(EbtSamplerCube), ptype2, ptype3, ptype4, ptype5);
        insertBuiltIn(level, gvec4 ? new TType(EbtInt, 4) : rvalue, name,
                      new TType(EbtISamplerCube), ptype2, ptype3, ptype4, ptype5);
        insertBuiltIn(level, gvec4 ? new TType(EbtUInt, 4) : rvalue, name,
                      new TType(EbtUSamplerCube), ptype2, ptype3, ptype4, ptype5);
        return;
    }
    if (ptype1->getBasicType() == EbtGSampler2DArray)
    {
        bool gvec4 = (rvalue->getBasicType() == EbtGVec4);
        insertBuiltIn(level, gvec4 ? new TType(EbtFloat, 4) : rvalue, name,
                      new TType(EbtSampler2DArray), ptype2, ptype3, ptype4, ptype5);
        insertBuiltIn(level, gvec4 ? new TType(EbtInt, 4) : rvalue, name,
                      new TType(EbtISampler2DArray), ptype2, ptype3, ptype4, ptype5);
        insertBuiltIn(level, gvec4 ? new TType(EbtUInt, 4) : rvalue, name,
                      new TType(EbtUSampler2DArray), ptype2, ptype3, ptype4, ptype5);
        return;
    }

    TFunction *function = new TFunction(NewPoolTString(name), *rvalue);

    TType *types[] = {ptype1, ptype2, ptype3, ptype4, ptype5};
    for (size_t ii = 0; ii < sizeof(types) / sizeof(types[0]); ++ii)
    {
        if (types[ii])
        {
            TParameter param = {NULL, types[ii]};
            function->addParameter(param);
        }
    }

    insert(level, function);
}

TPrecision TSymbolTable::getDefaultPrecision(TBasicType type) const
{
    if (!SupportsPrecision(type))
        return EbpUndefined;

    // unsigned integers use the same precision as signed
    TBasicType baseType = (type == EbtUInt) ? EbtInt : type;

    int level = static_cast<int>(precisionStack.size()) - 1;
    assert(level >= 0); // Just to be safe. Should not happen.
    // If we dont find anything we return this. Should we error check this?
    TPrecision prec = EbpUndefined;
    while (level >= 0)
    {
        PrecisionStackLevel::iterator it = precisionStack[level]->find(baseType);
        if (it != precisionStack[level]->end())
        {
            prec = (*it).second;
            break;
        }
        level--;
    }
    return prec;
}
