//
// Copyright (c) 2002-2012 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// UnfoldShortCircuit is an AST traverser to output short-circuiting operators as if-else statements
//

#ifndef COMPILER_UNFOLDSHORTCIRCUIT_H_
#define COMPILER_UNFOLDSHORTCIRCUIT_H_

#include "compiler/translator/intermediate.h"
#include "compiler/translator/ParseContext.h"

namespace sh
{
class OutputHLSL;

class UnfoldShortCircuit : public TIntermTraverser
{
  public:
    UnfoldShortCircuit(TParseContext &context, OutputHLSL *outputHLSL);

    void traverse(TIntermNode *node);
    bool visitBinary(Visit visit, TIntermBinary*);
    bool visitSelection(Visit visit, TIntermSelection *node);
    bool visitLoop(Visit visit, TIntermLoop *node);

    int getNextTemporaryIndex();

  protected:
    TParseContext &mContext;
    OutputHLSL *const mOutputHLSL;

    int mTemporaryIndex;
};
}

#endif   // COMPILER_UNFOLDSHORTCIRCUIT_H_
