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

namespace
{

class ElseBlockRewriter : public TIntermTraverser
{
  public:
    ElseBlockRewriter();

  protected:
    bool visitAggregate(Visit visit, TIntermAggregate *aggregate);

  private:
    int mTemporaryIndex;
    const TType *mFunctionType;

    TIntermNode *rewriteSelection(TIntermSelection *selection);
};

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

ElseBlockRewriter::ElseBlockRewriter()
    : TIntermTraverser(true, false, true, false),
      mTemporaryIndex(0),
      mFunctionType(NULL)
{}

bool ElseBlockRewriter::visitAggregate(Visit visit, TIntermAggregate *node)
{
    switch (node->getOp())
    {
      case EOpSequence:
        if (visit == PostVisit)
        {
            for (size_t statementIndex = 0; statementIndex != node->getSequence()->size(); statementIndex++)
            {
                TIntermNode *statement = (*node->getSequence())[statementIndex];
                TIntermSelection *selection = statement->getAsSelectionNode();
                if (selection && selection->getFalseBlock() != NULL)
                {
                    // Check for if / else if
                    TIntermSelection *elseIfBranch = selection->getFalseBlock()->getAsSelectionNode();
                    if (elseIfBranch)
                    {
                        selection->replaceChildNode(elseIfBranch, rewriteSelection(elseIfBranch));
                        delete elseIfBranch;
                    }

                    (*node->getSequence())[statementIndex] = rewriteSelection(selection);
                    delete selection;
                }
            }
        }
        break;

      case EOpFunction:
        // Store the current function context (see comment below)
        mFunctionType = ((visit == PreVisit) ? &node->getType() : NULL);
        break;

      default: break;
    }

    return true;
}

TIntermNode *ElseBlockRewriter::rewriteSelection(TIntermSelection *selection)
{
    ASSERT(selection != NULL);

    TString temporaryName = "cond_" + str(mTemporaryIndex++);
    TIntermTyped *typedCondition = selection->getCondition()->getAsTyped();
    TType resultType(EbtBool, EbpUndefined);
    TIntermSymbol *conditionSymbolInit = MakeNewTemporary(temporaryName, EbtBool);
    TIntermBinary *storeCondition = MakeNewBinary(EOpInitialize, conditionSymbolInit,
                                                  typedCondition, resultType);
    TIntermNode *negatedElse = NULL;

    TIntermSelection *falseBlock = NULL;

    if (selection->getFalseBlock())
    {
        // crbug.com/346463
        // D3D generates error messages claiming a function has no return value, when rewriting
        // an if-else clause that returns something non-void in a function. By appending dummy
        // returns (that are unreachable) we can silence this compile error.
        if (mFunctionType && mFunctionType->getBasicType() != EbtVoid)
        {
            TString typeString = mFunctionType->getStruct() ? mFunctionType->getStruct()->name() :
                mFunctionType->getBasicString();
            TString rawText = "return (" + typeString + ")0";
            negatedElse = new TIntermRaw(*mFunctionType, rawText);
        }

        TIntermSymbol *conditionSymbolElse = MakeNewTemporary(temporaryName, EbtBool);
        TIntermUnary *negatedCondition = MakeNewUnary(EOpLogicalNot, conditionSymbolElse);
        falseBlock = new TIntermSelection(negatedCondition,
                                          selection->getFalseBlock(), negatedElse);
    }

    TIntermSymbol *conditionSymbolSel = MakeNewTemporary(temporaryName, EbtBool);
    TIntermSelection *newSelection = new TIntermSelection(conditionSymbolSel,
                                                          selection->getTrueBlock(), falseBlock);

    TIntermAggregate *declaration = new TIntermAggregate(EOpDeclaration);
    declaration->getSequence()->push_back(storeCondition);

    TIntermAggregate *block = new TIntermAggregate(EOpSequence);
    block->getSequence()->push_back(declaration);
    block->getSequence()->push_back(newSelection);

    return block;
}

}

void RewriteElseBlocks(TIntermNode *node)
{
    ElseBlockRewriter rewriter;
    node->traverse(&rewriter);
}

}
