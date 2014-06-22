//
// Copyright (c) 2012 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Contains analysis utilities for dealing with HLSL's lack of support for
// the use of intrinsic functions which (implicitly or explicitly) compute
// gradients of functions with discontinuities. 
//

#ifndef COMPILER_DETECTDISCONTINUITY_H_
#define COMPILER_DETECTDISCONTINUITY_H_

#include "compiler/translator/intermediate.h"

namespace sh
{
// Checks whether a loop can run for a variable number of iterations
class DetectLoopDiscontinuity : public TIntermTraverser
{
  public:
    bool traverse(TIntermNode *node);

  protected:
    bool visitBranch(Visit visit, TIntermBranch *node);
    bool visitLoop(Visit visit, TIntermLoop *loop);
    bool visitAggregate(Visit visit, TIntermAggregate *node);

    int mLoopDepth;
    bool mLoopDiscontinuity;
};

bool containsLoopDiscontinuity(TIntermNode *node);

// Checks for intrinsic functions which compute gradients
class DetectGradientOperation : public TIntermTraverser
{
  public:
    bool traverse(TIntermNode *node);

  protected:
    bool visitUnary(Visit visit, TIntermUnary *node);
    bool visitAggregate(Visit visit, TIntermAggregate *node);

    bool mGradientOperation;
};

bool containsGradientOperation(TIntermNode *node);

}

#endif   // COMPILER_DETECTDISCONTINUITY_H_
