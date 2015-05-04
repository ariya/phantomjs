//
// Copyright (c) 2002-2014 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#include "compiler/translator/LoopInfo.h"

namespace
{

int EvaluateIntConstant(TIntermConstantUnion *node)
{
    ASSERT(node && node->getUnionArrayPointer());
    return node->getIConst(0);
}

int GetLoopIntIncrement(TIntermLoop *node)
{
    TIntermNode *expr = node->getExpression();
    // for expression has one of the following forms:
    //     loop_index++
    //     loop_index--
    //     loop_index += constant_expression
    //     loop_index -= constant_expression
    //     ++loop_index
    //     --loop_index
    // The last two forms are not specified in the spec, but I am assuming
    // its an oversight.
    TIntermUnary *unOp = expr->getAsUnaryNode();
    TIntermBinary *binOp = unOp ? NULL : expr->getAsBinaryNode();

    TOperator op = EOpNull;
    TIntermConstantUnion *incrementNode = NULL;
    if (unOp)
    {
        op = unOp->getOp();
    }
    else if (binOp)
    {
        op = binOp->getOp();
        ASSERT(binOp->getRight());
        incrementNode = binOp->getRight()->getAsConstantUnion();
        ASSERT(incrementNode);
    }

    int increment = 0;
    // The operator is one of: ++ -- += -=.
    switch (op)
    {
      case EOpPostIncrement:
      case EOpPreIncrement:
        ASSERT(unOp && !binOp);
        increment = 1;
        break;
      case EOpPostDecrement:
      case EOpPreDecrement:
        ASSERT(unOp && !binOp);
        increment = -1;
        break;
      case EOpAddAssign:
        ASSERT(!unOp && binOp);
        increment = EvaluateIntConstant(incrementNode);
        break;
      case EOpSubAssign:
        ASSERT(!unOp && binOp);
        increment = - EvaluateIntConstant(incrementNode);
        break;
      default:
        UNREACHABLE();
    }

    return increment;
}

}  // namespace anonymous

TLoopIndexInfo::TLoopIndexInfo()
    : mId(-1),
      mType(EbtVoid),
      mInitValue(0),
      mStopValue(0),
      mIncrementValue(0),
      mOp(EOpNull),
      mCurrentValue(0)
{
}

void TLoopIndexInfo::fillInfo(TIntermLoop *node)
{
    if (node == NULL)
        return;

    // Here we assume all the operations are valid, because the loop node is
    // already validated in ValidateLimitations.
    TIntermSequence *declSeq =
        node->getInit()->getAsAggregate()->getSequence();
    TIntermBinary *declInit = (*declSeq)[0]->getAsBinaryNode();
    TIntermSymbol *symbol = declInit->getLeft()->getAsSymbolNode();

    mId = symbol->getId();
    mType = symbol->getBasicType();

    if (mType == EbtInt)
    {
        TIntermConstantUnion* initNode = declInit->getRight()->getAsConstantUnion();
        mInitValue = EvaluateIntConstant(initNode);
        mCurrentValue = mInitValue;
        mIncrementValue = GetLoopIntIncrement(node);

        TIntermBinary* binOp = node->getCondition()->getAsBinaryNode();
        mStopValue = EvaluateIntConstant(
            binOp->getRight()->getAsConstantUnion());
        mOp = binOp->getOp();
    }
}

bool TLoopIndexInfo::satisfiesLoopCondition() const
{
    // Relational operator is one of: > >= < <= == or !=.
    switch (mOp)
    {
      case EOpEqual:
        return (mCurrentValue == mStopValue);
      case EOpNotEqual:
        return (mCurrentValue != mStopValue);
      case EOpLessThan:
        return (mCurrentValue < mStopValue);
      case EOpGreaterThan:
        return (mCurrentValue > mStopValue);
      case EOpLessThanEqual:
        return (mCurrentValue <= mStopValue);
      case EOpGreaterThanEqual:
        return (mCurrentValue >= mStopValue);
      default:
        UNREACHABLE();
        return false;
    }
}

TLoopInfo::TLoopInfo()
    : loop(NULL)
{
}

TLoopInfo::TLoopInfo(TIntermLoop *node)
    : loop(node)
{
    index.fillInfo(node);
}

TIntermLoop *TLoopStack::findLoop(TIntermSymbol *symbol)
{
    if (!symbol)
        return NULL;
    for (iterator iter = begin(); iter != end(); ++iter)
    {
        if (iter->index.getId() == symbol->getId())
            return iter->loop;
    }
    return NULL;
}

TLoopIndexInfo *TLoopStack::getIndexInfo(TIntermSymbol *symbol)
{
    if (!symbol)
        return NULL;
    for (iterator iter = begin(); iter != end(); ++iter)
    {
        if (iter->index.getId() == symbol->getId())
            return &(iter->index);
    }
    return NULL;
}

void TLoopStack::step()
{
    ASSERT(!empty());
    rbegin()->index.step();
}

bool TLoopStack::satisfiesLoopCondition()
{
    ASSERT(!empty());
    return rbegin()->index.satisfiesLoopCondition();
}

bool TLoopStack::needsToReplaceSymbolWithValue(TIntermSymbol *symbol)
{
    TIntermLoop *loop = findLoop(symbol);
    return loop && loop->getUnrollFlag();
}

int TLoopStack::getLoopIndexValue(TIntermSymbol *symbol)
{
    TLoopIndexInfo *info = getIndexInfo(symbol);
    ASSERT(info);
    return info->getCurrentValue();
}

void TLoopStack::push(TIntermLoop *loop)
{
    TLoopInfo info(loop);
    push_back(info);
}

void TLoopStack::pop()
{
    pop_back();
}

