//
// Copyright (c) 2014 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// RewriteElseBlocks.cpp: Implementation for tree transform to change
//   all if-else blocks to if-if blocks.
//

#include "compiler/translator/RewriteElseBlocks.h"
#include "compiler/translator/NodeSearch.h"
#include "compiler/translator/SymbolTable.h"

namespace sh
{

TIntermSymbol *MakeNewTemporary(const TString &name, TBasicType type)
{
    TType variableType(type, EbpHigh, EvqInternal);
    return new TIntermSymbol(-1, name, variableType);
}

TIntermBinary *MakeNewBinary(TOperator op, TIntermTyped *left, TIntermTyped *right, const TType &resultType)
{
    TIntermBinary *binary = new TIntermBinary(op);
    binary->setLeft(left);
    binary->setRight(right);
    binary->setType(resultType);
    return binary;
}

TIntermUnary *MakeNewUnary(TOperator op, TIntermTyped *operand)
{
    TIntermUnary *unary = new TIntermUnary(op, operand->getType());
    unary->setOperand(operand);
    return unary;
}

bool ElseBlockRewriter::visitAggregate(Visit visit, TIntermAggregate *node)
{
    switch (node->getOp())
    {
      case EOpSequence:
        {
            for (size_t statementIndex = 0; statementIndex != node->getSequence().size(); statementIndex++)
            {
                TIntermNode *statement = node->getSequence()[statementIndex];
                TIntermSelection *selection = statement->getAsSelectionNode();
                if (selection && selection->getFalseBlock() != NULL)
                {
                    node->getSequence()[statementIndex] = rewriteSelection(selection);
                    delete selection;
                }
            }
        }
        break;

      default: break;
    }

    return true;
}

TIntermNode *ElseBlockRewriter::rewriteSelection(TIntermSelection *selection)
{
    ASSERT(selection->getFalseBlock() != NULL);

    TString temporaryName = "cond_" + str(mTemporaryIndex++);
    TIntermTyped *typedCondition = selection->getCondition()->getAsTyped();
    TType resultType(EbtBool, EbpUndefined);
    TIntermSymbol *conditionSymbolA = MakeNewTemporary(temporaryName, EbtBool);
    TIntermSymbol *conditionSymbolB = MakeNewTemporary(temporaryName, EbtBool);
    TIntermSymbol *conditionSymbolC = MakeNewTemporary(temporaryName, EbtBool);
    TIntermBinary *storeCondition = MakeNewBinary(EOpInitialize, conditionSymbolA,
                                                  typedCondition, resultType);
    TIntermUnary *negatedCondition = MakeNewUnary(EOpLogicalNot, conditionSymbolB);
    TIntermSelection *falseBlock = new TIntermSelection(negatedCondition,
                                                        selection->getFalseBlock(), NULL);
    TIntermSelection *newIfElse = new TIntermSelection(conditionSymbolC,
                                                       selection->getTrueBlock(), falseBlock);

    TIntermAggregate *declaration = new TIntermAggregate(EOpDeclaration);
    declaration->getSequence().push_back(storeCondition);

    TIntermAggregate *block = new TIntermAggregate(EOpSequence);
    block->getSequence().push_back(declaration);
    block->getSequence().push_back(newIfElse);

    return block;
}

void RewriteElseBlocks(TIntermNode *node)
{
    ElseBlockRewriter rewriter;
    node->traverse(&rewriter);
}

}
