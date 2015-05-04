//
// Copyright (c) 2013 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#include "compiler/translator/FlagStd140Structs.h"

namespace sh
{

bool FlagStd140Structs::visitBinary(Visit visit, TIntermBinary *binaryNode)
{
    if (binaryNode->getRight()->getBasicType() == EbtStruct)
    {
        switch (binaryNode->getOp())
        {
          case EOpIndexDirectInterfaceBlock:
          case EOpIndexDirectStruct:
            if (isInStd140InterfaceBlock(binaryNode->getLeft()))
            {
                mFlaggedNodes.push_back(binaryNode);
            }
            break;

          default: break;
        }
        return false;
    }

    if (binaryNode->getOp() == EOpIndexDirectStruct)
    {
        return false;
    }

    return visit == PreVisit;
}

void FlagStd140Structs::visitSymbol(TIntermSymbol *symbol)
{
    if (isInStd140InterfaceBlock(symbol) && symbol->getBasicType() == EbtStruct)
    {
        mFlaggedNodes.push_back(symbol);
    }
}

bool FlagStd140Structs::isInStd140InterfaceBlock(TIntermTyped *node) const
{
    TIntermBinary *binaryNode = node->getAsBinaryNode();

    if (binaryNode)
    {
        return isInStd140InterfaceBlock(binaryNode->getLeft());
    }

    const TType &type = node->getType();

    // determine if we are in the standard layout
    const TInterfaceBlock *interfaceBlock = type.getInterfaceBlock();
    if (interfaceBlock)
    {
        return (interfaceBlock->blockStorage() == EbsStd140);
    }

    return false;
}

std::vector<TIntermTyped *> FlagStd140ValueStructs(TIntermNode *node)
{
    FlagStd140Structs flaggingTraversal;

    node->traverse(&flaggingTraversal);

    return flaggingTraversal.getFlaggedNodes();
}

}
