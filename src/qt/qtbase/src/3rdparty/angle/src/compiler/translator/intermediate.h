//
// Copyright (c) 2002-2014 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#ifndef COMPILER_TRANSLATOR_LOCAL_INTERMEDIATE_H_
#define COMPILER_TRANSLATOR_LOCAL_INTERMEDIATE_H_

#include "compiler/translator/IntermNode.h"

struct TVectorFields
{
    int offsets[4];
    int num;
};

//
// Set of helper functions to help parse and build the tree.
//
class TInfoSink;
class TIntermediate
{
  public:
    POOL_ALLOCATOR_NEW_DELETE();
    TIntermediate(TInfoSink &i)
        : mInfoSink(i) { }

    TIntermSymbol *addSymbol(
        int id, const TString &, const TType &, const TSourceLoc &);
    TIntermTyped *addBinaryMath(
        TOperator op, TIntermTyped *left, TIntermTyped *right, const TSourceLoc &);
    TIntermTyped *addAssign(
        TOperator op, TIntermTyped *left, TIntermTyped *right, const TSourceLoc &);
    TIntermTyped *addIndex(
        TOperator op, TIntermTyped *base, TIntermTyped *index, const TSourceLoc &);
    TIntermTyped *addUnaryMath(
        TOperator op, TIntermNode *child, const TSourceLoc &);
    TIntermAggregate *growAggregate(
        TIntermNode *left, TIntermNode *right, const TSourceLoc &);
    TIntermAggregate *makeAggregate(TIntermNode *node, const TSourceLoc &);
    TIntermAggregate *setAggregateOperator(TIntermNode *, TOperator, const TSourceLoc &);
    TIntermNode *addSelection(TIntermTyped *cond, TIntermNodePair code, const TSourceLoc &);
    TIntermTyped *addSelection(
        TIntermTyped *cond, TIntermTyped *trueBlock, TIntermTyped *falseBlock, const TSourceLoc &);
    TIntermTyped *addComma(
        TIntermTyped *left, TIntermTyped *right, const TSourceLoc &);
    TIntermConstantUnion *addConstantUnion(ConstantUnion *, const TType &, const TSourceLoc &);
    // TODO(zmo): Get rid of default value.
    bool parseConstTree(const TSourceLoc &, TIntermNode *, ConstantUnion *,
                        TOperator, TType, bool singleConstantParam = false);
    TIntermNode *addLoop(TLoopType, TIntermNode *, TIntermTyped *, TIntermTyped *,
                         TIntermNode *, const TSourceLoc &);
    TIntermBranch *addBranch(TOperator, const TSourceLoc &);
    TIntermBranch *addBranch(TOperator, TIntermTyped *, const TSourceLoc &);
    TIntermTyped *addSwizzle(TVectorFields &, const TSourceLoc &);
    bool postProcess(TIntermNode *);
    void remove(TIntermNode *);
    void outputTree(TIntermNode *);

  private:
    void operator=(TIntermediate &); // prevent assignments

    TInfoSink & mInfoSink;
};

#endif  // COMPILER_TRANSLATOR_LOCAL_INTERMEDIATE_H_
