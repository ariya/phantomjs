//
// Copyright (c) 2002-2013 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#include "compiler/translator/InitializeVariables.h"
#include "compiler/translator/compilerdebug.h"

namespace
{

TIntermConstantUnion* constructFloatConstUnionNode(const TType& type)
{
    TType myType = type;
    unsigned char size = myType.getNominalSize();
    if (myType.isMatrix())
        size *= size;
    ConstantUnion *u = new ConstantUnion[size];
    for (int ii = 0; ii < size; ++ii)
        u[ii].setFConst(0.0f);

    myType.clearArrayness();
    myType.setQualifier(EvqConst);
    TIntermConstantUnion *node = new TIntermConstantUnion(u, myType);
    return node;
}

TIntermConstantUnion* constructIndexNode(int index)
{
    ConstantUnion *u = new ConstantUnion[1];
    u[0].setIConst(index);

    TType type(EbtInt, EbpUndefined, EvqConst, 1);
    TIntermConstantUnion *node = new TIntermConstantUnion(u, type);
    return node;
}

}  // namespace anonymous

bool InitializeVariables::visitAggregate(Visit visit, TIntermAggregate* node)
{
    bool visitChildren = !mCodeInserted;
    switch (node->getOp())
    {
      case EOpSequence:
        break;
      case EOpFunction:
      {
        // Function definition.
        ASSERT(visit == PreVisit);
        if (node->getName() == "main(")
        {
            TIntermSequence &sequence = node->getSequence();
            ASSERT((sequence.size() == 1) || (sequence.size() == 2));
            TIntermAggregate *body = NULL;
            if (sequence.size() == 1)
            {
                body = new TIntermAggregate(EOpSequence);
                sequence.push_back(body);
            }
            else
            {
                body = sequence[1]->getAsAggregate();
            }
            ASSERT(body);
            insertInitCode(body->getSequence());
            mCodeInserted = true;
        }
        break;
      }
      default:
        visitChildren = false;
        break;
    }
    return visitChildren;
}

void InitializeVariables::insertInitCode(TIntermSequence& sequence)
{
    for (size_t ii = 0; ii < mVariables.size(); ++ii)
    {
        const InitVariableInfo& varInfo = mVariables[ii];

        if (varInfo.type.isArray())
        {
            for (int index = varInfo.type.getArraySize() - 1; index >= 0; --index)
            {
                TIntermBinary *assign = new TIntermBinary(EOpAssign);
                sequence.insert(sequence.begin(), assign);

                TIntermBinary *indexDirect = new TIntermBinary(EOpIndexDirect);
                TIntermSymbol *symbol = new TIntermSymbol(0, varInfo.name, varInfo.type);
                indexDirect->setLeft(symbol);
                TIntermConstantUnion *indexNode = constructIndexNode(index);
                indexDirect->setRight(indexNode);

                assign->setLeft(indexDirect);

                TIntermConstantUnion *zeroConst = constructFloatConstUnionNode(varInfo.type);
                assign->setRight(zeroConst);
            }
        }
        else
        {
            TIntermBinary *assign = new TIntermBinary(EOpAssign);
            sequence.insert(sequence.begin(), assign);
            TIntermSymbol *symbol = new TIntermSymbol(0, varInfo.name, varInfo.type);
            assign->setLeft(symbol);
            TIntermConstantUnion *zeroConst = constructFloatConstUnionNode(varInfo.type);
            assign->setRight(zeroConst);
        }

    }
}

