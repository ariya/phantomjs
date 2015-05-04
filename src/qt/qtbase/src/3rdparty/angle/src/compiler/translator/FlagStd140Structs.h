//
// Copyright (c) 2013 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#ifndef COMPILER_FLAGSTD140STRUCTS_H_
#define COMPILER_FLAGSTD140STRUCTS_H_

#include "compiler/translator/IntermNode.h"

namespace sh
{

// This class finds references to nested structs of std140 blocks that access
// the nested struct "by value", where the padding added in the translator
// conflicts with the "natural" unpadded type.
class FlagStd140Structs : public TIntermTraverser
{
  public:
    const std::vector<TIntermTyped *> getFlaggedNodes() const { return mFlaggedNodes; }

  protected:
    virtual bool visitBinary(Visit visit, TIntermBinary *binaryNode);
    virtual void visitSymbol(TIntermSymbol *symbol);

  private:
    bool isInStd140InterfaceBlock(TIntermTyped *node) const;

    std::vector<TIntermTyped *> mFlaggedNodes;
};

std::vector<TIntermTyped *> FlagStd140ValueStructs(TIntermNode *node);

}

#endif // COMPILER_FLAGSTD140STRUCTS_H_
