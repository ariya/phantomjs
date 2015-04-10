//
// Copyright (c) 2002-2011 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#ifndef COMPILER_DETECT_RECURSION_H_
#define COMPILER_DETECT_RECURSION_H_

#include "GLSLANG/ShaderLang.h"

#include <limits.h>
#include "compiler/translator/intermediate.h"
#include "compiler/translator/VariableInfo.h"

class TInfoSink;

// Traverses intermediate tree to detect function recursion.
class DetectCallDepth : public TIntermTraverser {
public:
    enum ErrorCode {
        kErrorMissingMain,
        kErrorRecursion,
        kErrorMaxDepthExceeded,
        kErrorNone
    };

    DetectCallDepth(TInfoSink& infoSync, bool limitCallStackDepth, int maxCallStackDepth);
    ~DetectCallDepth();

    virtual bool visitAggregate(Visit, TIntermAggregate*);

    bool checkExceedsMaxDepth(int depth);

    ErrorCode detectCallDepth();

private:
    class FunctionNode {
    public:
        static const int kInfiniteCallDepth = INT_MAX;

        FunctionNode(const TString& fname);

        const TString& getName() const;

        // If a function is already in the callee list, this becomes a no-op.
        void addCallee(FunctionNode* callee);

        // Returns kInifinityCallDepth if recursive function calls are detected.
        int detectCallDepth(DetectCallDepth* detectCallDepth, int depth);

        // Reset state.
        void reset();

    private:
        // mangled function name is unique.
        TString name;

        // functions that are directly called by this function.
        TVector<FunctionNode*> callees;

        Visit visit;
    };

    ErrorCode detectCallDepthForFunction(FunctionNode* func);
    FunctionNode* findFunctionByName(const TString& name);
    void resetFunctionNodes();

    TInfoSink& getInfoSink() { return infoSink; }

    TVector<FunctionNode*> functions;
    FunctionNode* currentFunction;
    TInfoSink& infoSink;
    int maxDepth;

    DetectCallDepth(const DetectCallDepth&);
    void operator=(const DetectCallDepth&);
};

#endif  // COMPILER_DETECT_RECURSION_H_
