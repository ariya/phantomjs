//
// Copyright (c) 2012 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Contains analysis utilities for dealing with HLSL's lack of support for
// the use of intrinsic functions which (implicitly or explicitly) compute
// gradients of functions with discontinuities. 
//

#include "compiler/translator/DetectDiscontinuity.h"

#include "compiler/translator/ParseContext.h"

namespace sh
{

// Detect Loop Discontinuity

bool DetectLoopDiscontinuity::traverse(TIntermNode *node)
{
    mLoopDepth = 0;
    mLoopDiscontinuity = false;
    node->traverse(this);
    return mLoopDiscontinuity;
}

bool DetectLoopDiscontinuity::visitLoop(Visit visit, TIntermLoop *loop)
{
    if (visit == PreVisit)
    {
        ++mLoopDepth;
    }
    else if (visit == PostVisit)
    {
        --mLoopDepth;
    }

    return true;
}

bool DetectLoopDiscontinuity::visitBranch(Visit visit, TIntermBranch *node)
{
    if (mLoopDiscontinuity)
    {
        return false;
    }

    if (!mLoopDepth)
    {
        return true;
    }

    switch (node->getFlowOp())
    {
      case EOpKill:
        break;
      case EOpBreak:
      case EOpContinue:
      case EOpReturn:
        mLoopDiscontinuity = true;
        break;
      default: UNREACHABLE();
    }

    return !mLoopDiscontinuity;
}

bool DetectLoopDiscontinuity::visitAggregate(Visit visit, TIntermAggregate *node)
{
    return !mLoopDiscontinuity;
}

bool containsLoopDiscontinuity(TIntermNode *node)
{
    DetectLoopDiscontinuity detectLoopDiscontinuity;
    return detectLoopDiscontinuity.traverse(node);
}

// Detect Any Loop

bool DetectAnyLoop::traverse(TIntermNode *node)
{
    mHasLoop = false;
    node->traverse(this);
    return mHasLoop;
}

bool DetectAnyLoop::visitLoop(Visit visit, TIntermLoop *loop)
{
    mHasLoop = true;
    return false;
}

// The following definitions stop all traversal when we have found a loop
bool DetectAnyLoop::visitBinary(Visit, TIntermBinary *)
{
    return !mHasLoop;
}

bool DetectAnyLoop::visitUnary(Visit, TIntermUnary *)
{
    return !mHasLoop;
}

bool DetectAnyLoop::visitSelection(Visit, TIntermSelection *)
{
    return !mHasLoop;
}

bool DetectAnyLoop::visitAggregate(Visit, TIntermAggregate *)
{
    return !mHasLoop;
}

bool DetectAnyLoop::visitBranch(Visit, TIntermBranch *)
{
    return !mHasLoop;
}

bool containsAnyLoop(TIntermNode *node)
{
    DetectAnyLoop detectAnyLoop;
    return detectAnyLoop.traverse(node);
}

// Detect Gradient Operation

bool DetectGradientOperation::traverse(TIntermNode *node)
{
    mGradientOperation = false;
    node->traverse(this);
    return mGradientOperation;
}

bool DetectGradientOperation::visitUnary(Visit visit, TIntermUnary *node)
{
    if (mGradientOperation)
    {
        return false;
    }

    switch (node->getOp())
    {
      case EOpDFdx:
      case EOpDFdy:
        mGradientOperation = true;
      default:
        break;
    }

    return !mGradientOperation;
}

bool DetectGradientOperation::visitAggregate(Visit visit, TIntermAggregate *node)
{
    if (mGradientOperation)
    {
        return false;
    }

    if (node->getOp() == EOpFunctionCall)
    {
        if (!node->isUserDefined())
        {
            TString name = TFunction::unmangleName(node->getName());

            if (name == "texture2D" ||
                name == "texture2DProj" ||
                name == "textureCube")
            {
                mGradientOperation = true;
            }
        }
        else
        {
            // When a user defined function is called, we have to
            // conservatively assume it to contain gradient operations
            mGradientOperation = true;
        }
    }

    return !mGradientOperation;
}

bool containsGradientOperation(TIntermNode *node)
{
    DetectGradientOperation detectGradientOperation;
    return detectGradientOperation.traverse(node);
}
}
