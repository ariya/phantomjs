//
// Copyright (c) 2010 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#include "compiler/translator/IntermNode.h"
#include "compiler/translator/LoopInfo.h"

class TInfoSinkBase;

// Traverses intermediate tree to ensure that the shader does not exceed the
// minimum functionality mandated in GLSL 1.0 spec, Appendix A.
class ValidateLimitations : public TIntermTraverser
{
  public:
    ValidateLimitations(sh::GLenum shaderType, TInfoSinkBase &sink);

    int numErrors() const { return mNumErrors; }

    virtual bool visitBinary(Visit, TIntermBinary *);
    virtual bool visitUnary(Visit, TIntermUnary *);
    virtual bool visitAggregate(Visit, TIntermAggregate *);
    virtual bool visitLoop(Visit, TIntermLoop *);

  private:
    void error(TSourceLoc loc, const char *reason, const char *token);

    bool withinLoopBody() const;
    bool isLoopIndex(TIntermSymbol *symbol);
    bool validateLoopType(TIntermLoop *node);

    bool validateForLoopHeader(TIntermLoop *node);
    // If valid, return the index symbol id; Otherwise, return -1.
    int validateForLoopInit(TIntermLoop *node);
    bool validateForLoopCond(TIntermLoop *node, int indexSymbolId);
    bool validateForLoopExpr(TIntermLoop *node, int indexSymbolId);

    // Returns true if none of the loop indices is used as the argument to
    // the given function out or inout parameter.
    bool validateFunctionCall(TIntermAggregate *node);
    bool validateOperation(TIntermOperator *node, TIntermNode *operand);

    // Returns true if indexing does not exceed the minimum functionality
    // mandated in GLSL 1.0 spec, Appendix A, Section 5.
    bool isConstExpr(TIntermNode *node);
    bool isConstIndexExpr(TIntermNode *node);
    bool validateIndexing(TIntermBinary *node);

    sh::GLenum mShaderType;
    TInfoSinkBase &mSink;
    int mNumErrors;
    TLoopStack mLoopStack;
};

