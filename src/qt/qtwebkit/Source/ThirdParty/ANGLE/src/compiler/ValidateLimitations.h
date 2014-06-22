//
// Copyright (c) 2010 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#include "GLSLANG/ShaderLang.h"
#include "compiler/intermediate.h"

class TInfoSinkBase;

struct TLoopInfo {
    struct TIndex {
        int id;  // symbol id.
    } index;
    TIntermLoop* loop;
};
typedef TVector<TLoopInfo> TLoopStack;

// Traverses intermediate tree to ensure that the shader does not exceed the
// minimum functionality mandated in GLSL 1.0 spec, Appendix A.
class ValidateLimitations : public TIntermTraverser {
public:
    ValidateLimitations(ShShaderType shaderType, TInfoSinkBase& sink);

    int numErrors() const { return mNumErrors; }

    virtual bool visitBinary(Visit, TIntermBinary*);
    virtual bool visitUnary(Visit, TIntermUnary*);
    virtual bool visitAggregate(Visit, TIntermAggregate*);
    virtual bool visitLoop(Visit, TIntermLoop*);

private:
    void error(TSourceLoc loc, const char *reason, const char* token);

    bool withinLoopBody() const;
    bool isLoopIndex(const TIntermSymbol* symbol) const;
    bool validateLoopType(TIntermLoop* node);
    bool validateForLoopHeader(TIntermLoop* node, TLoopInfo* info);
    bool validateForLoopInit(TIntermLoop* node, TLoopInfo* info);
    bool validateForLoopCond(TIntermLoop* node, TLoopInfo* info);
    bool validateForLoopExpr(TIntermLoop* node, TLoopInfo* info);
    // Returns true if none of the loop indices is used as the argument to
    // the given function out or inout parameter.
    bool validateFunctionCall(TIntermAggregate* node);
    bool validateOperation(TIntermOperator* node, TIntermNode* operand);

    // Returns true if indexing does not exceed the minimum functionality
    // mandated in GLSL 1.0 spec, Appendix A, Section 5.
    bool isConstExpr(TIntermNode* node);
    bool isConstIndexExpr(TIntermNode* node);
    bool validateIndexing(TIntermBinary* node);

    ShShaderType mShaderType;
    TInfoSinkBase& mSink;
    int mNumErrors;
    TLoopStack mLoopStack;
};

