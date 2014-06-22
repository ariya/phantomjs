//
// Copyright (c) 2002-2010 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// SearchSymbol is an AST traverser to detect the use of a given symbol name
//

#ifndef COMPILER_SEARCHSYMBOL_H_
#define COMPILER_SEARCHSYMBOL_H_

#include "compiler/translator/intermediate.h"
#include "compiler/translator/ParseContext.h"

namespace sh
{
class SearchSymbol : public TIntermTraverser
{
  public:
    SearchSymbol(const TString &symbol);

    void traverse(TIntermNode *node);
    void visitSymbol(TIntermSymbol *symbolNode);

    bool foundMatch() const;

  protected:
    const TString &mSymbol;
    bool match;
};
}

#endif   // COMPILER_SEARCHSYMBOL_H_
