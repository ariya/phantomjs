//
// Copyright (c) 2011 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#include "compiler/intermediate.h"

struct TLoopIndexInfo {
    int id;
    int initValue;
    int stopValue;
    int incrementValue;
    TOperator op;
    int currentValue;
};

class ForLoopUnroll {
public:
    ForLoopUnroll() { }

    void FillLoopIndexInfo(TIntermLoop* node, TLoopIndexInfo& info);

    // Update the info.currentValue for the next loop iteration.
    void Step();

    // Return false if loop condition is no longer satisfied.
    bool SatisfiesLoopCondition();

    // Check if the symbol is the index of a loop that's unrolled.
    bool NeedsToReplaceSymbolWithValue(TIntermSymbol* symbol);

    // Return the current value of a given loop index symbol.
    int GetLoopIndexValue(TIntermSymbol* symbol);

    void Push(TLoopIndexInfo& info);
    void Pop();

    static void MarkForLoopsWithIntegerIndicesForUnrolling(TIntermNode* root);

private:
    int getLoopIncrement(TIntermLoop* node);

    int evaluateIntConstant(TIntermConstantUnion* node);

    TVector<TLoopIndexInfo> mLoopIndexStack;
};

