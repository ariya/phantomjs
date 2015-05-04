//
// Copyright (c) 2002-2014 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#ifndef COMPILER_TRANSLATOR_LOOP_INFO_H_
#define COMPILER_TRANSLATOR_LOOP_INFO_H_

#include "compiler/translator/IntermNode.h"

class TLoopIndexInfo
{
  public:
    TLoopIndexInfo();

    // If type is EbtInt, fill all fields of the structure with info
    // extracted from a loop node.
    // If type is not EbtInt, only fill id and type.
    void fillInfo(TIntermLoop *node);

    int getId() const { return mId; }
    void setId(int id) { mId = id; }
    TBasicType getType() const { return mType; }
    void setType(TBasicType type) { mType = type; }
    int getCurrentValue() const { return mCurrentValue; }

    void step() { mCurrentValue += mIncrementValue; }

    // Check if the current value satisfies the loop condition.
    bool satisfiesLoopCondition() const;

  private:
    int mId;
    TBasicType mType;  // Either EbtInt or EbtFloat

    // Below fields are only valid if the index's type is int.
    int mInitValue;
    int mStopValue;
    int mIncrementValue;
    TOperator mOp;
    int mCurrentValue;
};

struct TLoopInfo
{
    TLoopIndexInfo index;
    TIntermLoop *loop;

    TLoopInfo();
    TLoopInfo(TIntermLoop *node);
};

class TLoopStack : public TVector<TLoopInfo>
{
  public:
    // Search loop stack for a loop whose index matches the input symbol.
    TIntermLoop *findLoop(TIntermSymbol *symbol);

    // Find the loop index info in the loop stack by the input symbol.
    TLoopIndexInfo *getIndexInfo(TIntermSymbol *symbol);

    // Update the currentValue for the next loop iteration.
    void step();

    // Return false if loop condition is no longer satisfied.
    bool satisfiesLoopCondition();

    // Check if the symbol is the index of a loop that's unrolled.
    bool needsToReplaceSymbolWithValue(TIntermSymbol *symbol);

    // Return the current value of a given loop index symbol.
    int getLoopIndexValue(TIntermSymbol *symbol);

    void push(TIntermLoop *info);
    void pop();
};

#endif // COMPILER_TRANSLATOR_LOOP_INDEX_H_

