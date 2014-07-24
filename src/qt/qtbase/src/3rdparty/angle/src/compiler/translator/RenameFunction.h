//
// Copyright (c) 2012 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#ifndef COMPILER_RENAME_FUNCTION
#define COMPILER_RENAME_FUNCTION

#include "compiler/translator/intermediate.h"

//
// Renames a function, including its declaration and any calls to it.
//
class RenameFunction : public TIntermTraverser
{
public:
    RenameFunction(const TString& oldFunctionName, const TString& newFunctionName)
    : TIntermTraverser(true, false, false)
    , mOldFunctionName(oldFunctionName)
    , mNewFunctionName(newFunctionName) {}

    virtual bool visitAggregate(Visit visit, TIntermAggregate* node)
    {
        TOperator op = node->getOp();
        if ((op == EOpFunction || op == EOpFunctionCall) && node->getName() == mOldFunctionName)
            node->setName(mNewFunctionName);
        return true;
    }

private:
    const TString mOldFunctionName;
    const TString mNewFunctionName;
};

#endif  // COMPILER_RENAME_FUNCTION
