//
// Copyright (c) 2002-2014 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#ifndef COMPILER_TRANSLATOR_REGENERATE_STRUCT_NAMES_H_
#define COMPILER_TRANSLATOR_REGENERATE_STRUCT_NAMES_H_

#include "compiler/translator/Intermediate.h"
#include "compiler/translator/SymbolTable.h"

#include <set>

class RegenerateStructNames : public TIntermTraverser
{
  public:
    RegenerateStructNames(const TSymbolTable &symbolTable,
                          int shaderVersion)
        : mSymbolTable(symbolTable),
          mShaderVersion(shaderVersion),
          mScopeDepth(0) {}

  protected:
    virtual void visitSymbol(TIntermSymbol *);
    virtual bool visitAggregate(Visit, TIntermAggregate *);

  private:
    const TSymbolTable &mSymbolTable;
    int mShaderVersion;

    // Indicating the depth of the current scope.
    // The global scope is 1.
    int mScopeDepth;

    // If a struct's declared globally, push its ID in this set.
    std::set<int> mDeclaredGlobalStructs;
};

#endif  // COMPILER_TRANSLATOR_REGENERATE_STRUCT_NAMES_H_
